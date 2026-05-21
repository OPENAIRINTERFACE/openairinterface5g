/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
*/

/*!
* \brief OAI MAC/Aerial L1 interface file using FAPI over shared memory
*/

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdatomic.h>
#include <sys/queue.h>
#include <sys/epoll.h>
#include "fapi_nvIPC.h"
#include <nfapi_vnf.h>
#include <nr_fapi_p5.h>
#include <nr_fapi_p7_utils.h>
#include "nfapi_interface.h"
#include "nfapi.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi/open-nFAPI/vnf/inc/vnf_p7.h"
#include "system.h"
#include "fapi_vnf_p7.h"

#define MAX_EVENTS 10
#define RECV_BUF_LEN 8192
char cpu_buf_recv[RECV_BUF_LEN];

uint16_t sfn = 0, slot = 0;
nv_ipc_t *ipc;
static nv_ipc_config_t nv_ipc_config;
static int cpu_msg_buf_size = 0;
static int cpu_data_buf_size = 0;
static int cpu_large_buf_size = 0;

void nvIPC_Stop()
{
  LOG_I(NR_MAC, "Received STOP.indication\n");
  ((vnf_t *)get_config())->terminate = true;
}

void nvIPC_send_stop_request()
{
  nfapi_nr_stop_request_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST, .header.phy_id = 0};
  LOG_I(NR_MAC, "Sending NFAPI STOP.request\n");
  nfapi_nr_vnf_stop_req(get_config(), get_config()->pnf_list->p5_idx, &req);
}


static uint16_t old_sfn = 0;
static uint16_t old_slot = 0;
////////////////////////////////////////////////////////////////////////
// Handle an RX message
static int ipc_handle_rx_msg(nv_ipc_msg_t *msg)
{
  if (msg == NULL) {
    LOG_E(NFAPI_VNF, "%s: ERROR: buffer is empty\n", __func__);
    return -1;
  }

  uint8_t *pReadPackedMessage = msg->msg_buf;
  uint8_t *end = msg->msg_buf + msg->msg_len;

  // unpack FAPI messages and handle them
  nfapi_vnf_config_t * vnf_config = get_config();
  if (vnf_config != 0) {
    // first, unpack the header
    fapi_message_header_t fapi_msg;
    if (!(pull8(&pReadPackedMessage, &fapi_msg.num_msg, end) && pull8(&pReadPackedMessage, &fapi_msg.opaque_handle, end)
          && pull16(&pReadPackedMessage, &fapi_msg.message_id, end)
          && pull32(&pReadPackedMessage, &fapi_msg.message_length, end))) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FAPI message header unpack failed\n");
      return -1;
    }

   vnf_p7_t *vnf_p7_config = (vnf_p7_t *)((vnf_info *)vnf_config->user_data)->p7_vnfs->config;
    switch (fapi_msg.message_id) {
      case NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE ... NFAPI_NR_PHY_MSG_TYPE_ERROR_INDICATION:
        vnf_nr_handle_p4_p5_message(msg->msg_buf, msg->msg_len, 1, vnf_config);
        break;
      // P7 Messages
      case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION: {
        uint8_t *pReadData = msg->data_buf;
        int dataBufLen = msg->data_len;
        uint8_t *data_end = msg->data_buf + dataBufLen;
        nfapi_nr_rx_data_indication_t ind;
        ind.header.message_id = fapi_msg.message_id;
        ind.header.message_length = fapi_msg.message_length;
        aerial_unpack_nr_rx_data_indication(
            &pReadPackedMessage,
            end,
            &pReadData,
            data_end,
            &ind,
            &vnf_p7_config->_public.codec_config);
        NFAPI_TRACE(NFAPI_TRACE_INFO, "%s: Handling RX Indication\n", __FUNCTION__);
        if (vnf_p7_config->_public.nr_rx_data_indication) {
          (vnf_p7_config->_public.nr_rx_data_indication)(&ind);
          for (int i = 0; i < ind.number_of_pdus; ++i) {
            free(ind.pdu_list[i].pdu);
          }
          free(ind.pdu_list);
        }
        break;
      }

      case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION: {
        uint8_t *pReadData = msg->data_buf;
        int dataBufLen = msg->data_len;
        uint8_t *data_end = msg->data_buf + dataBufLen;
        nfapi_nr_srs_indication_t ind;
        aerial_unpack_nr_srs_indication(&pReadPackedMessage, end, &pReadData, data_end, &ind);
        if (vnf_p7_config->_public.nr_srs_indication) {
          (vnf_p7_config->_public.nr_srs_indication)(&ind);
        }
        break;
      }

      case NFAPI_NR_PHY_MSG_TYPE_SLOT_INDICATION: {
        nfapi_nr_slot_indication_scf_t ind;
        if (!vnf_p7_config->_public.unpack_func(msg->msg_buf, msg->msg_len, &ind, sizeof(ind), &vnf_p7_config->_public.codec_config)) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_DEBUG, "%s: Handling NR SLOT Indication\n", __FUNCTION__);
          // check if the sfn/slot unpacked come wrong at any time, should be old + 1 (slot 0 -- 19, sfn 0 -- 1023)
          // add 1 to current sfn number
          uint16_t old_slot_plus = ((old_slot + 1) % 20);
          uint16_t old_sfn_plus = old_slot_plus == 0 ? ((old_sfn + 1) % 1024) : old_sfn;
          if (old_slot_plus != ind.slot || old_sfn_plus != ind.sfn) {
            LOG_E(NFAPI_VNF,
                  "\n============================================================================\n"
                  "sfn slot doesn't match unpacked one! L2->L1 %d.%d  vs L1->L2 %d.%d  \n"
                  "============================================================================\n",
                  old_sfn,
                  old_slot,
                  ind.sfn,
                  ind.slot);
          }
          old_sfn = ind.sfn;
          old_slot = ind.slot;
          if (vnf_p7_config->_public.nr_slot_indication) {
            (vnf_p7_config->_public.nr_slot_indication)(&ind);
          }
        }
        break;
      }
      case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
      case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
      case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION: {
        vnf_nr_handle_p7_message(msg->msg_buf, msg->msg_len, vnf_p7_config);
        break;
      }
      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P5 Unknown message ID %d\n", __FUNCTION__, fapi_msg.message_id);

        break;
      }
    }
  }
  return 0;
}

int get_cpu_msg_buf_size()
{
  return cpu_msg_buf_size;
}
int get_cpu_data_buf_size()
{
  return cpu_data_buf_size;
}

bool allocate_msg(nv_ipc_msg_t *send_msg)
{
  return ipc->tx_allocate(ipc, send_msg, 0) == 0;
}
void release_msg(nv_ipc_msg_t *send_msg)
{
  ipc->tx_release(ipc, send_msg);
}

static uint16_t read_le16_u8(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le32_u8(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void write_le16_u8(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
}

static void write_le32_u8(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

static uint32_t align4_u32(uint32_t v)
{
  return (v + 3u) & ~3u;
}

static bool widen_config_request_tlv_lengths_for_aerial(uint8_t *buf, uint32_t *msg_len, uint32_t buf_cap)
{
  enum { FAPI_HEADER_LEN = 8, CONFIG_NUM_TLVS_OFFSET = 8, CONFIG_TLV_START_OFFSET = 9, TLV16_HEADER_LEN = 4, TLV32_HEADER_LEN = 6 };

  if (buf == NULL || msg_len == NULL || *msg_len < CONFIG_TLV_START_OFFSET) {
    return false;
  }

  const uint32_t old_len = *msg_len;
  const uint8_t advertised_tlv_count = buf[CONFIG_NUM_TLVS_OFFSET];
  uint32_t tlv_offsets[256];
  uint16_t tlv_lengths[256];
  uint8_t tlv_count = 0;
  uint32_t src_off = CONFIG_TLV_START_OFFSET;

  while (src_off < old_len) {
    if (tlv_count == UINT8_MAX) {
      LOG_E(NFAPI_VNF, "CONFIG.request TLV16 parse found more than 255 TLVs old_len=%u\n", old_len);
      return false;
    }
    if (src_off + TLV16_HEADER_LEN > old_len) {
      LOG_E(NFAPI_VNF, "CONFIG.request TLV16 parse failed at tlv=%u offset=%u old_len=%u\n", tlv_count, src_off, old_len);
      return false;
    }
    const uint16_t value_len = read_le16_u8(buf + src_off + 2);
    const uint32_t padded_value_len = align4_u32(value_len);
    if (src_off + TLV16_HEADER_LEN + padded_value_len > old_len) {
      LOG_E(NFAPI_VNF,
            "CONFIG.request TLV16 length overflow at tlv=%u tag=0x%04x len=%u offset=%u old_len=%u\n",
            tlv_count,
            read_le16_u8(buf + src_off),
            value_len,
            src_off,
            old_len);
      return false;
    }
    tlv_offsets[tlv_count] = src_off;
    tlv_lengths[tlv_count] = value_len;
    src_off += TLV16_HEADER_LEN + padded_value_len;
    tlv_count++;
  }

  if (advertised_tlv_count != tlv_count) {
    LOG_W(NFAPI_VNF,
          "CONFIG.request TLV count corrected before Aerial conversion: advertised=%u actual=%u msg_len=%u\n",
          advertised_tlv_count,
          tlv_count,
          old_len);
    buf[CONFIG_NUM_TLVS_OFFSET] = tlv_count;
  }

  const uint32_t growth = 2u * tlv_count;
  const uint32_t new_len = old_len + growth;
  if (new_len > buf_cap) {
    LOG_E(NFAPI_VNF, "CONFIG.request TLV32 conversion needs %u bytes, CPU_MSG buffer has %u\n", new_len, buf_cap);
    return false;
  }

  /* Aerial 26-1 cuPHY is built with SCF_FAPI_10_04, where scf_fapi_tl_t is
   * tag:u16 + length:u32.  OAI's pack_nr_tlv()/hand-coded CONFIG TLVs still
   * serialize tag:u16 + length:u16.  Convert only this L2->L1 CONFIG.request
   * boundary in-place, working backwards so overlapping memmove is safe. */
  for (int32_t i = (int32_t)tlv_count - 1; i >= 0; --i) {
    const uint32_t old_tlv_off = tlv_offsets[i];
    const uint32_t new_tlv_off = old_tlv_off + (2u * (uint32_t)i);
    const uint16_t tag = read_le16_u8(buf + old_tlv_off);
    const uint16_t value_len = tlv_lengths[i];
    const uint32_t padded_value_len = align4_u32(value_len);

    memmove(buf + new_tlv_off + TLV32_HEADER_LEN, buf + old_tlv_off + TLV16_HEADER_LEN, padded_value_len);
    write_le16_u8(buf + new_tlv_off, tag);
    write_le32_u8(buf + new_tlv_off + 2, value_len);
  }

  const uint32_t old_body_len = read_le32_u8(buf + 4);
  if (old_body_len + FAPI_HEADER_LEN != old_len) {
    LOG_W(NFAPI_VNF, "CONFIG.request body length mismatch before TLV32 conversion: body=%u msg_len=%u\n", old_body_len, old_len);
  }
  const uint32_t new_body_len = new_len - FAPI_HEADER_LEN;
  write_le32_u8(buf + 4, new_body_len);
  *msg_len = new_len;
  LOG_I(NFAPI_VNF,
        "CONFIG.request TLV32 conversion: tlvs=%u old_len=%u new_len=%u body_len=%u->%u\n",
        tlv_count,
        old_len,
        new_len,
        old_body_len,
        new_body_len);
  return true;
}

bool send_nvipc_msg(nv_ipc_msg_t *send_msg)
{
  int send_retval = ipc->tx_send_msg(ipc, send_msg);
  if (send_retval != 0) {
    LOG_E(NFAPI_VNF, "%s error: send TX message failed Error: %d\n", __FUNCTION__, send_retval);
    ipc->tx_release(ipc, send_msg);
    return false;
  }

  ipc->notify(ipc, 1); // notify that there's 1 message in queue
  return true;
}

bool aerial_nr_send_p5_message(vnf_t *vnf, uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t *msg, uint32_t msg_len)
{
  nfapi_vnf_pnf_info_t *pnf = nfapi_vnf_pnf_list_find(&(vnf->_public), p5_idx);

  if (pnf) {
    // Create the message
    nv_ipc_msg_t send_msg = {.msg_id = msg->message_id,
                             .cell_id = 0,
                             // By default, P5 uses only message pool.
                             .data_pool = NV_IPC_MEMPOOL_CPU_MSG,
                             .data_len = 0,
                             .data_buf = NULL};

    bool has_separate_dbt_payload = false;
    nfapi_nr_config_request_scf_t *config_req = NULL;
    if (msg->message_id == NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST) {
      config_req = (nfapi_nr_config_request_scf_t *)msg;
      has_separate_dbt_payload = config_req->dbt_config.num_dig_beams > 0;
      if (has_separate_dbt_payload) {
        send_msg.data_pool = NV_IPC_MEMPOOL_CPU_LARGE;
      }
    }

    // Allocate the message
    if (!allocate_msg(&send_msg)) {
      return false;
    }

    int packedMessageLengthFAPI = -1;
    // Pack directly into it
    if ((packedMessageLengthFAPI =
             vnf->_public.pack_func(msg, msg_len, send_msg.msg_buf, get_cpu_msg_buf_size(), &vnf->_public.codec_config))
        < 0) {
      release_msg(&send_msg);
      return false;
    }
    if (packedMessageLengthFAPI <= 0) {
      LOG_E(NFAPI_VNF, "Error packing message 0x%02x\n", msg->message_id);
      release_msg(&send_msg);
      return false;
    }
    // fapi_nr_p5_message_pack() returns the complete packed FAPI message length,
    // including the 8-byte FAPI header.  Passing +8 makes cuPHY see an
    // over-long nvIPC message and reject CONFIG.request length validation.
    send_msg.msg_len = packedMessageLengthFAPI;

    if (msg->message_id == NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST) {
      uint32_t converted_len = (uint32_t)send_msg.msg_len;
      if (!widen_config_request_tlv_lengths_for_aerial((uint8_t *)send_msg.msg_buf, &converted_len, (uint32_t)get_cpu_msg_buf_size())) {
        LOG_E(NFAPI_VNF, "Failed to convert CONFIG.request TLV lengths for Aerial SCF_FAPI_10_04\n");
        release_msg(&send_msg);
        return false;
      }
      send_msg.msg_len = (int)converted_len;
      packedMessageLengthFAPI = (int)converted_len;
    }

    if (has_separate_dbt_payload) {
      AssertFatal(send_msg.data_buf != NULL, "CONFIG.request DBT path: data buffer is NULL\n");
      AssertFatal(cpu_large_buf_size > 0, "CONFIG.request DBT path: CPU_LARGE buffer size is not set\n");
      uint8_t *write_ptr = send_msg.data_buf;
      uint8_t *buffer_end = send_msg.data_buf + cpu_large_buf_size;
      nfapi_nr_dbt_tlv_ve_t dbt_tlv = {
        .tl.tag = NFAPI_NR_CONFIG_BEAMFORMING_TABLE_TAG,
        .value = config_req->dbt_config
      };
      bool ok = pack_dbt_table_tlv_value(&dbt_tlv, &write_ptr, buffer_end);
      if (!ok) {
        LOG_E(NFAPI_VNF, "Failed to pack CONFIG.request DBT payload into separate buffer\n");
        release_msg(&send_msg);
        return false;
      }
      send_msg.data_len = (uint32_t)(write_ptr - (uint8_t *)send_msg.data_buf);
      LOG_I(NFAPI_VNF,
            "CONFIG.request DBT sent in separate pool: num_beams=%u num_txrus=%u data_len=%d pool=%d\n",
            config_req->dbt_config.num_dig_beams,
            config_req->dbt_config.num_txrus,
            send_msg.data_len,
            send_msg.data_pool);
    }

    // Send
    return send_nvipc_msg(&send_msg);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find pnf info for p5_idx:%d\n", __FUNCTION__, p5_idx);
    return false;
  }
}

// Always allocate message buffer, but allocate data buffer only when data_len > 0
static int aerial_recv_msg(nv_ipc_msg_t *recv_msg)
{
  if (ipc == NULL) {
    return -1;
  }
  recv_msg->msg_buf = NULL;
  recv_msg->data_buf = NULL;

  // Allocate buffer for TX message
  if (ipc->rx_recv_msg(ipc, recv_msg) < 0) {
    LOG_D(NFAPI_VNF, "%s: no more message available\n", __func__);
    return -1;
  }
  LOG_D(NFAPI_VNF,
         "recv: cell_id=%d msg_id=0x%02X msg_len=%d data_len=%d data_pool=%d\n",
         recv_msg->cell_id,
         recv_msg->msg_id,
         recv_msg->msg_len,
         recv_msg->data_len,
         recv_msg->data_pool);
  ipc_handle_rx_msg(recv_msg);

  // Release buffer of RX message
  int release_retval = ipc->rx_release(ipc, recv_msg);
  if (release_retval != 0) {
    LOG_E(NFAPI_VNF, "%s error: release RX buffer failed Error: %d\n", __FUNCTION__, release_retval);
    return release_retval;
  }
  return 0;
}

void *epoll_recv_task(void *arg)
{
  UNUSED(arg);
  struct epoll_event ev, events[MAX_EVENTS];

  LOG_D(NFAPI_VNF,"Aerial recv task start \n");

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    LOG_E(NFAPI_VNF, "%s epoll_create failed\n", __func__);
  }

  int ipc_rx_event_fd = ipc->get_fd(ipc);
  ev.events = EPOLLIN;
  ev.data.fd = ipc_rx_event_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
    LOG_E(NFAPI_VNF, "%s epoll_ctl failed\n", __func__);
  }
  // From here on out the thread is ready to receive data, simulate the reception
  // of a PARAM.response to get the VNF to send a CONFIG.request
  nfapi_nr_param_response_scf_t resp_msg = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE};
  nfapi_vnf_config_t * vnf_config = get_config();
  vnf_config->nr_param_resp(vnf_config, 1, &resp_msg);
  while (((vnf_t *)vnf_config)->terminate == false) {
    LOG_D(NFAPI_VNF, "%s: epoll_wait fd_rx=%d ...\n", __func__, ipc_rx_event_fd);

    int nfds;
    do {
      // epoll_wait() may return EINTR when get unexpected signal SIGSTOP from system
      nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    } while (nfds == -1 && errno == EINTR);

    if (nfds < 0) {
      LOG_E(NFAPI_VNF, "epoll_wait failed: epoll_fd=%d nfds=%d err=%d - %s\n", epoll_fd, nfds, errno, strerror(errno));
    }

    int n = 0;
    for (n = 0; n < nfds; ++n) {
      if (events[n].data.fd == ipc_rx_event_fd) {
        ipc->get_value(ipc);
        nv_ipc_msg_t recv_msg;
        while (aerial_recv_msg(&recv_msg) == 0);
      }
    }
  }
  close(epoll_fd);
  ipc->ipc_destroy(ipc);
  return NULL;
}

void create_recv_thread(int8_t affinity)
{
  pthread_t thread_id;
  threadCreate(&thread_id, epoll_recv_task, NULL, "vnf_nvipc_aerial", affinity, OAI_PRIORITY_RT_MAX);
}

int load_hard_code_config(nv_ipc_config_t *config, int module_type, nv_ipc_transport_t _transport)
{
  // Create configuration
  config->ipc_transport = _transport;
  if (set_nv_ipc_default_config(config, module_type) < 0) {
    LOG_E(NFAPI_VNF, "%s: set configuration failed\n", __func__);
    return -1;
  }

  int test_cuda_device_id = -1;
  LOG_D(NFAPI_VNF,"CUDA device ID configured : %d \n", test_cuda_device_id);
  config->transport_config.shm.cuda_device_id = test_cuda_device_id;
  if (test_cuda_device_id >= 0) {
    config->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].pool_len = 128;
    config->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len = 1024;
    config->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len = 4096;
  }

  return 0;
}

int nvIPC_Init(nvipc_params_t nvipc_params_s)
{
  // Want to use transport SHM, type epoll, module secondary (reads the created shm from cuphycontroller)
  load_hard_code_config(&nv_ipc_config, NV_IPC_MODULE_SECONDARY, NV_IPC_TRANSPORT_SHM);
  // save the mempool sizes for later use in packing
  cpu_msg_buf_size = nv_ipc_get_buf_size(&nv_ipc_config, NV_IPC_MEMPOOL_CPU_MSG);
  cpu_data_buf_size = nv_ipc_get_buf_size(&nv_ipc_config, NV_IPC_MEMPOOL_CPU_DATA);
  cpu_large_buf_size = nv_ipc_get_buf_size(&nv_ipc_config, NV_IPC_MEMPOOL_CPU_LARGE);

  // Create nv_ipc_t instance
  LOG_I(NFAPI_VNF, "%s: creating IPC interface with prefix %s\n", __func__, nvipc_params_s.nvipc_shm_prefix);
  strcpy(nv_ipc_config.transport_config.shm.prefix, nvipc_params_s.nvipc_shm_prefix);
  if ((ipc = create_nv_ipc_interface(&nv_ipc_config)) == NULL) {
    LOG_E(NFAPI_VNF, "%s: create IPC interface failed\n", __func__);
    return -1;
  }
  LOG_I(NFAPI_VNF, "%s: create IPC interface successful\n", __func__);
  sleep(1);
  create_recv_thread(nvipc_params_s.nvipc_poll_core);
  return 0;
}

int oai_fapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req)
{
  nfapi_vnf_p7_config_t *p7_config = get_p7_vnf_config();

  ul_tti_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ul_tti_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST;

  bool retval = p7_config->send_p7_msg(get_p7_vnf(), &ul_tti_req->header);

  if (!retval) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    // Reset number of PDUs so that it is not resent
    ul_tti_req->n_pdus = 0;
    ul_tti_req->n_group = 0;
    ul_tti_req->n_ulcch = 0;
    ul_tti_req->n_ulsch = 0;
  }
  return retval;
}

int oai_fapi_ul_dci_req(nfapi_nr_ul_dci_request_t *ul_dci_req)
{
  nfapi_vnf_p7_config_t *p7_config = get_p7_vnf_config();
  ul_dci_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ul_dci_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST;

  bool retval = p7_config->send_p7_msg(get_p7_vnf(), &ul_dci_req->header);
  if (!retval) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    ul_dci_req->numPdus = 0;
  }
  return retval;
}

int oai_fapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req)
{
  nfapi_vnf_p7_config_t *p7_config = get_p7_vnf_config();
  tx_data_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  tx_data_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST;

  bool retval = p7_config->send_p7_msg(get_p7_vnf(), &tx_data_req->header);
  if (!retval) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    tx_data_req->Number_of_PDUs = 0;
  }

  return retval;
}

int oai_fapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req)
{
  nfapi_vnf_p7_config_t *p7_config = get_p7_vnf_config();
  dl_config_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST;
  dl_config_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

  bool retval = p7_config->send_p7_msg(get_p7_vnf(), &dl_config_req->header);
  dl_config_req->dl_tti_request_body.nPDUs = 0;
  dl_config_req->dl_tti_request_body.nGroup = 0;

  if (!retval) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  }
  return retval;
}

int oai_fapi_send_end_request(uint32_t frame, uint32_t slot)
{
  nfapi_vnf_p7_config_t *p7_config = get_p7_vnf_config();
  nfapi_nr_slot_indication_scf_t nr_slot_resp = {.header.message_id = 0x8F, .sfn = frame, .slot = slot};

  bool retval = p7_config->send_p7_msg(get_p7_vnf(), &nr_slot_resp.header);
  if (!retval) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  }
  return retval;
}

