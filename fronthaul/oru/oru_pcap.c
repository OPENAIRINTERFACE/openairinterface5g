/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "oru_pcap.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <rte_byteorder.h>
#include <rte_ether.h>
#include <rte_mbuf.h>

#include "xran_pkt_api.h"

#include "common/utils/LOG/log.h"

#define ETHER_TYPE_ECPRI 0xAEFE

/* Design: FH pcap capture must never block the O-RU real-time path.
 * Sync fwrite/fflush on the packet path previously stalled the ORU, so capture
 * is asynchronous: the hot path only memcpy's into a fixed ring, a dedicated
 * writer thread owns all fwrite/fflush/fclose, and ring-full drops are
 * best-effort. ORU_PCAP_CHAN filters are documented in the header.
 */
#define ORU_PCAP_LINKTYPE_ETHERNET 1
#define ORU_PCAP_FILE_BUF_SIZE (1u << 20) /* 1 MiB stdio buffer */
#define ORU_PCAP_RING_SIZE 256
enum {
  ORU_PCAP_CHAN_ALL = 0, /* DL+UL C/U-plane, no temporal gate */
  ORU_PCAP_CHAN_PUSCH = 1, /* PUSCH UL only */
  ORU_PCAP_CHAN_PUSCH_PRACH = 2, /* PUSCH free; PRACH after first PUSCH C-plane */
  ORU_PCAP_CHAN_RX = 3, /* every DU->RU frame (DL C/U plus UL and PRACH C-plane) */
  ORU_PCAP_CHAN_DL = 4, /* DU->RU DL C-plane and DL U-plane only */
};

typedef struct {
  uint32_t ts_sec;
  uint32_t ts_usec;
  uint32_t captured_len;
  uint32_t original_len;
} oru_pcap_slot_t;

static FILE *oru_pcap_fp = NULL;
static char oru_pcap_file_buf[ORU_PCAP_FILE_BUF_SIZE];
static oru_pcap_slot_t oru_pcap_ring[ORU_PCAP_RING_SIZE];
static uint8_t *oru_pcap_ring_data = NULL;
static uint32_t oru_pcap_frame_snaplen = 0;
static pthread_mutex_t oru_pcap_enq_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t oru_pcap_widx = 0; /* under enq_lock */
static _Atomic uint32_t oru_pcap_ridx = 0;
static uint32_t oru_pcap_queued = 0; /* successful enqueues; under enq_lock */
static _Atomic uint32_t oru_pcap_dropped = 0;
static _Atomic uint32_t oru_pcap_written = 0;
static uint32_t oru_pcap_max = 20000;
static time_t oru_pcap_arm_time = 0; /* CLOCK_REALTIME second to start at; 0 = immediately */
static int oru_pcap_chan = ORU_PCAP_CHAN_ALL;
static _Atomic bool oru_pcap_enabled = false;
static bool oru_pcap_init_done = false;
static pthread_t oru_pcap_thread;
static _Atomic bool oru_pcap_pusch_seen = false;
static int oru_pcap_prach_eaxc_offset = 0;
static struct xran_eaxcid_config oru_pcap_eaxcid_config = {.mask_cuPortId = 0xF000,
                                                           .mask_bandSectorId = 0x0F00,
                                                           .mask_ccId = 0x00F0,
                                                           .mask_ruPortId = 0x000F,
                                                           .bit_cuPortId = 12,
                                                           .bit_bandSectorId = 8,
                                                           .bit_ccId = 4,
                                                           .bit_ruPortId = 0};

static bool oru_pcap_inactive(void)
{
  return !atomic_load_explicit(&oru_pcap_enabled, memory_order_relaxed);
}

static void *oru_pcap_writer(void *arg)
{
  (void)arg;
  while (true) {
    uint32_t r = atomic_load_explicit(&oru_pcap_ridx, memory_order_relaxed);
    uint32_t w;
    bool capped;
    pthread_mutex_lock(&oru_pcap_enq_lock);
    w = oru_pcap_widx;
    capped = oru_pcap_queued >= oru_pcap_max;
    pthread_mutex_unlock(&oru_pcap_enq_lock);

    if (r == w) {
      if (capped)
        break;
      struct timespec sleep_ts = {.tv_sec = 0, .tv_nsec = 1000 * 1000}; /* 1 ms */
      nanosleep(&sleep_ts, NULL);
      continue;
    }

    oru_pcap_slot_t *slot = &oru_pcap_ring[r % ORU_PCAP_RING_SIZE];
    struct __attribute__((packed)) {
      uint32_t ts_sec, ts_usec, incl_len, orig_len;
    } ph = {slot->ts_sec, slot->ts_usec, slot->captured_len, slot->original_len};
    if (oru_pcap_fp != NULL) {
      fwrite(&ph, sizeof(ph), 1, oru_pcap_fp);
      const uint8_t *data = oru_pcap_ring_data + (r % ORU_PCAP_RING_SIZE) * oru_pcap_frame_snaplen;
      fwrite(data, 1, slot->captured_len, oru_pcap_fp);
    }
    atomic_store_explicit(&oru_pcap_ridx, r + 1, memory_order_release);
    uint32_t n = atomic_fetch_add_explicit(&oru_pcap_written, 1, memory_order_relaxed) + 1;
    /* Periodic flush so the dump is inspectable while capture runs. */
    if (oru_pcap_fp != NULL && (n % 32u) == 0u)
      fflush(oru_pcap_fp);
    if (n >= oru_pcap_max)
      break;
  }

  if (oru_pcap_fp != NULL) {
    fflush(oru_pcap_fp);
    fclose(oru_pcap_fp);
    oru_pcap_fp = NULL;
  }
  uint32_t dropped = atomic_load_explicit(&oru_pcap_dropped, memory_order_relaxed);
  uint32_t written = atomic_load_explicit(&oru_pcap_written, memory_order_relaxed);
  atomic_store_explicit(&oru_pcap_enabled, false, memory_order_release);
  LOG_A(HW, "ORU pcap writer done: wrote %u packets, dropped %u\n", written, dropped);
  return NULL;
}

void oru_pcap_init_from_env(int prach_eaxc_offset, uint16_t mtu)
{
  if (oru_pcap_init_done)
    return;
  oru_pcap_init_done = true;
  oru_pcap_prach_eaxc_offset = prach_eaxc_offset;

  const char *path = getenv("ORU_PCAP_PATH");
  if (path == NULL || path[0] == '\0')
    return;
  if (mtu == 0) {
    LOG_E(HW, "ORU_PCAP_PATH is set but the configured fronthaul MTU is zero\n");
    return;
  }
  /* DPDK's configured MTU excludes L2 overhead. Captured mbufs exclude the
   * Ethernet FCS but can retain one VLAN header, so this is the largest frame
   * the pcap needs to retain. It is also written as the pcap file snaplen. */
  oru_pcap_frame_snaplen = (uint32_t)mtu + sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr);
  oru_pcap_ring_data = malloc((size_t)ORU_PCAP_RING_SIZE * oru_pcap_frame_snaplen);
  if (oru_pcap_ring_data == NULL) {
    LOG_E(HW, "Could not allocate ORU pcap ring (%u x %u bytes)\n", ORU_PCAP_RING_SIZE, oru_pcap_frame_snaplen);
    return;
  }
  const char *max_env = getenv("ORU_PCAP_MAX_PACKETS");
  if (max_env != NULL && max_env[0] != '\0')
    oru_pcap_max = (uint32_t)strtoul(max_env, NULL, 10);
  /* The O-RU comes up before the DU sends DL, so an immediate capture spends its whole
   * budget on the startup phase (PRACH C-plane + O-RU UL). Delay to reach steady state. */
  unsigned long delay_s = 0;
  const char *delay_env = getenv("ORU_PCAP_START_DELAY");
  if (delay_env != NULL && delay_env[0] != '\0')
    delay_s = strtoul(delay_env, NULL, 10);
  if (delay_s > 0) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    oru_pcap_arm_time = now.tv_sec + (time_t)delay_s;
  }
  const char *chan_env = getenv("ORU_PCAP_CHAN");
  const char *chan_name = "all";
  if (chan_env != NULL && chan_env[0] != '\0') {
    if (strcasecmp(chan_env, "pusch") == 0) {
      oru_pcap_chan = ORU_PCAP_CHAN_PUSCH;
      chan_name = "pusch";
    } else if (strcasecmp(chan_env, "pusch_prach") == 0) {
      oru_pcap_chan = ORU_PCAP_CHAN_PUSCH_PRACH;
      chan_name = "pusch_prach";
    } else if (strcasecmp(chan_env, "rx") == 0) {
      oru_pcap_chan = ORU_PCAP_CHAN_RX;
      chan_name = "rx";
    } else if (strcasecmp(chan_env, "dl") == 0) {
      oru_pcap_chan = ORU_PCAP_CHAN_DL;
      chan_name = "dl";
    } else if (strcasecmp(chan_env, "all") == 0) {
      oru_pcap_chan = ORU_PCAP_CHAN_ALL;
      chan_name = "all";
    } else {
      LOG_W(HW, "ORU_PCAP_CHAN=%s unknown (use all|rx|dl|pusch|pusch_prach); defaulting to all\n", chan_env);
    }
  }
  oru_pcap_fp = fopen(path, "wb");
  if (oru_pcap_fp == NULL) {
    LOG_E(HW, "ORU_PCAP_PATH=%s could not be opened: %s\n", path, strerror(errno));
    free(oru_pcap_ring_data);
    oru_pcap_ring_data = NULL;
    return;
  }
  setvbuf(oru_pcap_fp, oru_pcap_file_buf, _IOFBF, sizeof(oru_pcap_file_buf));
  struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t ver_major, ver_minor;
    int32_t thiszone;
    uint32_t sigfigs, snaplen, network;
  } gh = {0xa1b2c3d4, 2, 4, 0, 0, oru_pcap_frame_snaplen, ORU_PCAP_LINKTYPE_ETHERNET};
  fwrite(&gh, sizeof(gh), 1, oru_pcap_fp);
  /* Header alone lives in the 1 MiB stdio buffer; flush so a live 24-byte file
   * shows capture is armed even when no UL C-plane is accepted yet. */
  fflush(oru_pcap_fp);

  if (pthread_create(&oru_pcap_thread, NULL, oru_pcap_writer, NULL) != 0) {
    LOG_E(HW, "ORU pcap writer thread failed: %s\n", strerror(errno));
    fclose(oru_pcap_fp);
    oru_pcap_fp = NULL;
    free(oru_pcap_ring_data);
    oru_pcap_ring_data = NULL;
    return;
  }
  pthread_detach(oru_pcap_thread);
  atomic_store_explicit(&oru_pcap_enabled, true, memory_order_release);
  const char *gate_msg = oru_pcap_chan == ORU_PCAP_CHAN_PUSCH_PRACH ? "PRACH after first PUSCH C-plane"
                         : oru_pcap_chan == ORU_PCAP_CHAN_PUSCH     ? "PUSCH UL only"
                         : oru_pcap_chan == ORU_PCAP_CHAN_RX        ? "all DU->RU frames, no temporal gate"
                         : oru_pcap_chan == ORU_PCAP_CHAN_DL        ? "DL C/U-plane only, no temporal gate"
                                                                    : "DL+UL C/U-plane, no temporal gate";
  LOG_A(HW,
        "ORU pcap dump enabled -> %s (max %u packets; snaplen %u; chan %s; start delay %lus; async ring %u; %s)\n",
        path,
        oru_pcap_max,
        oru_pcap_frame_snaplen,
        chan_name,
        delay_s,
        ORU_PCAP_RING_SIZE,
        gate_msg);
}

/* Hot-path enqueue: copy one Ethernet frame into the ring. Never fwrite here.
 * ecpri_only payloads start at the eCPRI header, so a synthetic Ethernet header is
 * prepended; otherwise the frame is stored verbatim (keeps MACs and any VLAN tag). */
static void oru_pcap_enqueue(const uint8_t *payload, uint32_t payload_len, bool ecpri_only)
{
  if (payload == NULL || payload_len == 0 || oru_pcap_inactive())
    return;

  uint32_t original_len = ecpri_only ? payload_len + (uint32_t)sizeof(struct rte_ether_hdr) : payload_len;
  uint32_t captured_len = original_len < oru_pcap_frame_snaplen ? original_len : oru_pcap_frame_snaplen;

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  if (ts.tv_sec < oru_pcap_arm_time)
    return;

  pthread_mutex_lock(&oru_pcap_enq_lock);
  if (!atomic_load_explicit(&oru_pcap_enabled, memory_order_relaxed) || oru_pcap_queued >= oru_pcap_max) {
    pthread_mutex_unlock(&oru_pcap_enq_lock);
    return;
  }
  uint32_t r = atomic_load_explicit(&oru_pcap_ridx, memory_order_acquire);
  if (oru_pcap_widx - r >= ORU_PCAP_RING_SIZE) {
    pthread_mutex_unlock(&oru_pcap_enq_lock);
    atomic_fetch_add_explicit(&oru_pcap_dropped, 1, memory_order_relaxed);
    return;
  }

  oru_pcap_slot_t *slot = &oru_pcap_ring[oru_pcap_widx % ORU_PCAP_RING_SIZE];
  uint8_t *slot_data = oru_pcap_ring_data + (oru_pcap_widx % ORU_PCAP_RING_SIZE) * oru_pcap_frame_snaplen;
  slot->ts_sec = (uint32_t)ts.tv_sec;
  slot->ts_usec = (uint32_t)(ts.tv_nsec / 1000);
  slot->captured_len = captured_len;
  slot->original_len = original_len;

  if (ecpri_only) {
    memset(slot_data, 0, 12);
    slot_data[12] = (ETHER_TYPE_ECPRI >> 8) & 0xff;
    slot_data[13] = ETHER_TYPE_ECPRI & 0xff;
    memcpy(slot_data + 14, payload, captured_len - sizeof(struct rte_ether_hdr));
  } else {
    memcpy(slot_data, payload, captured_len);
  }

  oru_pcap_widx++;
  oru_pcap_queued++;
  bool hit_max = oru_pcap_queued >= oru_pcap_max;
  pthread_mutex_unlock(&oru_pcap_enq_lock);

  if (hit_max)
    atomic_store_explicit(&oru_pcap_enabled, false, memory_order_release);
}

bool oru_pcap_mbuf_is_prach(struct rte_mbuf *mbuf)
{
  if (mbuf == NULL)
    return false;
  const uint8_t *frame = rte_pktmbuf_mtod(mbuf, const uint8_t *);
  uint32_t len = rte_pktmbuf_pkt_len(mbuf);
  if (len < sizeof(struct rte_ether_hdr) + sizeof(struct xran_ecpri_hdr))
    return false;
  const struct xran_ecpri_hdr *ecpri = (const struct xran_ecpri_hdr *)(frame + sizeof(struct rte_ether_hdr));
  uint8_t ant_id = 0;
  xran_decompose_cid(ecpri->ecpri_xtc_id, &oru_pcap_eaxcid_config, NULL, NULL, NULL, &ant_id);
  return ant_id >= (uint8_t)oru_pcap_prach_eaxc_offset;
}

static bool oru_pcap_pusch_gate_open(void)
{
  return atomic_load_explicit(&oru_pcap_pusch_seen, memory_order_relaxed);
}

void oru_pcap_write_uplane(struct rte_mbuf *mbuf, bool is_prach)
{
  if (mbuf == NULL || oru_pcap_inactive())
    return;
  bool chan_ok = false;
  switch (oru_pcap_chan) {
    case ORU_PCAP_CHAN_ALL:
      chan_ok = true;
      break;
    case ORU_PCAP_CHAN_PUSCH:
      chan_ok = !is_prach;
      break;
    case ORU_PCAP_CHAN_PUSCH_PRACH:
      /* PUSCH always; PRACH only after first PUSCH C-plane. */
      chan_ok = !is_prach || oru_pcap_pusch_gate_open();
      break;
    case ORU_PCAP_CHAN_RX:
    case ORU_PCAP_CHAN_DL:
      /* Receive-side only: the O-RU's own uplink is not captured. */
      break;
    default:
      break;
  }
  if (chan_ok)
    oru_pcap_enqueue(rte_pktmbuf_mtod(mbuf, const uint8_t *), rte_pktmbuf_pkt_len(mbuf), false);
}

/* Locate the eCPRI header of a raw RX frame, skipping an optional VLAN tag.
 * Returns NULL when the frame is not eCPRI or is too short to classify. */
static const struct xran_ecpri_hdr *oru_pcap_rx_ecpri_hdr(const uint8_t *frame, uint32_t len)
{
  uint32_t off = (uint32_t)sizeof(struct rte_ether_hdr);
  if (len < off)
    return NULL;
  uint16_t ethertype = ((uint16_t)frame[12] << 8) | frame[13];
  if (ethertype == 0x8100) {
    off += (uint32_t)sizeof(struct rte_vlan_hdr);
    if (len < off)
      return NULL;
    ethertype = ((uint16_t)frame[off - 2] << 8) | frame[off - 1];
  }
  if (ethertype != ETHER_TYPE_ECPRI || len < off + sizeof(struct xran_ecpri_hdr))
    return NULL;
  return (const struct xran_ecpri_hdr *)(frame + off);
}

/* True for DL C-plane and DL U-plane. The DU also sends UL and PRACH C-plane on
 * this link, which carry uplink grants rather than downlink data. */
static bool oru_pcap_rx_frame_is_dl(const uint8_t *frame, uint32_t len)
{
  const struct xran_ecpri_hdr *ecpri = oru_pcap_rx_ecpri_hdr(frame, len);
  if (ecpri == NULL)
    return false;
  /* Every U-plane frame from the DU is downlink; the O-RU sources the uplink IQ. */
  if (ecpri->cmnhdr.bits.ecpri_mesg_type == ECPRI_IQ_DATA)
    return true;
  if (ecpri->cmnhdr.bits.ecpri_mesg_type != ECPRI_RT_CONTROL_DATA)
    return false;
  const uint8_t *apphdr_start = (const uint8_t *)ecpri + sizeof(struct xran_ecpri_hdr);
  if (apphdr_start + sizeof(struct xran_cp_radioapp_common_header) > frame + len)
    return false;
  const struct xran_cp_radioapp_common_header *apphdr = (const struct xran_cp_radioapp_common_header *)apphdr_start;
  uint32_t bits = rte_be_to_cpu_32(apphdr->field.all_bits);
  return ((bits >> 31) & 1u) == (uint32_t)XRAN_DIR_DL;
}

void oru_pcap_write_rx_frame(struct rte_mbuf *mbuf)
{
  /* Must be called before the RX path strips the Ethernet header: the frame is
   * stored verbatim so DL is identifiable by source MAC / VLAN. */
  if (mbuf == NULL || oru_pcap_inactive())
    return;
  if (oru_pcap_chan != ORU_PCAP_CHAN_ALL && oru_pcap_chan != ORU_PCAP_CHAN_RX && oru_pcap_chan != ORU_PCAP_CHAN_DL)
    return;
  const uint8_t *frame = rte_pktmbuf_mtod(mbuf, const uint8_t *);
  uint32_t len = rte_pktmbuf_pkt_len(mbuf);
  if (oru_pcap_chan == ORU_PCAP_CHAN_DL && !oru_pcap_rx_frame_is_dl(frame, len))
    return;
  oru_pcap_enqueue(frame, len, false);
}

static void oru_pcap_write_cplane_bytes(const uint8_t *ecpri, uint32_t len)
{
  if (ecpri == NULL || len == 0 || oru_pcap_inactive())
    return;
  oru_pcap_enqueue(ecpri, len, true);
}

static bool oru_pcap_want_cplane(const uint8_t *data, uint32_t len)
{
  /* all/rx/dl dump whole RX frames via oru_pcap_write_rx_frame; avoid double-write here. */
  if (oru_pcap_chan == ORU_PCAP_CHAN_ALL || oru_pcap_chan == ORU_PCAP_CHAN_RX || oru_pcap_chan == ORU_PCAP_CHAN_DL)
    return false;
  const uint32_t ecpri_len = (uint32_t)sizeof(struct xran_ecpri_hdr);
  if (len < ecpri_len + sizeof(struct xran_cp_radioapp_common_header))
    return false;
  const struct xran_cp_radioapp_common_header *apphdr = (const struct xran_cp_radioapp_common_header *)(data + ecpri_len);
  const uint8_t section_type = apphdr->sectionType;
  if (section_type == XRAN_CP_SECTIONTYPE_3) {
    /* PRACH C-plane for pusch_prach only, and only after the first PUSCH. */
    return oru_pcap_chan == ORU_PCAP_CHAN_PUSCH_PRACH && oru_pcap_pusch_gate_open();
  }
  if (section_type != XRAN_CP_SECTIONTYPE_1)
    return false;
  uint32_t bits = rte_be_to_cpu_32(apphdr->field.all_bits);
  const uint32_t data_direction = (bits >> 31) & 1u;
  if (data_direction != (uint32_t)XRAN_DIR_UL)
    return false;
  /* PUSCH C-plane: ungated in both pusch and pusch_prach; it opens the PRACH gate. */
  return oru_pcap_chan == ORU_PCAP_CHAN_PUSCH || oru_pcap_chan == ORU_PCAP_CHAN_PUSCH_PRACH;
}

void oru_pcap_cplane_begin(struct rte_mbuf *pkt, oru_pcap_cplane_snap_t *snap)
{
  if (snap == NULL)
    return;
  snap->ok = false;
  snap->len = 0;
  if (pkt == NULL || oru_pcap_inactive())
    return;
  uint32_t len = rte_pktmbuf_pkt_len(pkt);
  const uint8_t *src = rte_pktmbuf_mtod(pkt, const uint8_t *);
  if (len == 0 || len > sizeof(snap->data) || !oru_pcap_want_cplane(src, len))
    return;
  memcpy(snap->data, src, len);
  snap->len = len;
  snap->ok = true;
}

void oru_pcap_cplane_commit_pusch(oru_pcap_cplane_snap_t *snap)
{
  atomic_store_explicit(&oru_pcap_pusch_seen, true, memory_order_relaxed);
  if (snap != NULL && snap->ok)
    oru_pcap_write_cplane_bytes(snap->data, snap->len);
  if (snap != NULL)
    snap->ok = false;
}

void oru_pcap_cplane_commit_prach(oru_pcap_cplane_snap_t *snap)
{
  if (snap != NULL && snap->ok)
    oru_pcap_write_cplane_bytes(snap->data, snap->len);
  if (snap != NULL)
    snap->ok = false;
}
