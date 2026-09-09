/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include "common/utils/LOG/log.h"
#include "common/utils/ocp_itti/intertask_interface.h" // this pulls in all of OAI+4G+5G
#include "openair3/SCTP/sctp_eNB_task.h"
#include "openair2/E1AP/e1ap.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "openair2/E1AP/lib/e1ap_interface_management.h"
#include "common/config/config_userapi.h"
#include "openair2/E1AP/lib/e1ap_bearer_context_management.h"

configmodule_interface_t *uniqCfg;

/** QFI for NG-U GTP PDU Session Container */
#define TEST_QOS_FLOW_ID 1

#define DL 0
#define UL 1

int nr_rlc_get_available_tx_space(const rnti_t rntiP, const logical_chan_id_t channel_idP) { abort(); return 0; } /* in GTP */
softmodem_params_t *get_softmodem_params(void) { static softmodem_params_t p = {0}; return &p; }; /* in ITTI */
void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req) { abort(); } /* CU-UP */
void e1_bearer_context_modif(const e1ap_bearer_mod_req_t *req) { abort(); } /* CU-UP */
void e1_bearer_context_mod_confirm(const e1ap_bearer_mod_confirm_t *conf) { abort(); } /* CU-UP */
void e1_bearer_release_cmd(const e1ap_bearer_release_cmd_t *cmd) { abort(); } /* CU-UP */
void e1_reset(void) { abort(); } /* CU-UP */
instance_t *N3GTPUInst = NULL; /* CU-UP */

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  LOG_E(GNB_APP, "error at %s:%d:%s: %s\n", file, line, function, s);
  abort();
}

/* @brief Dummy function to pass a fptr to ITTI for staring RRC queue, without
 * actually handling any messages. Functions called by main() will send/receive
 * ITTI messages to E1, as if it was RRC. */
static void *cuup_tester_rrc(void *)
{
  itti_mark_task_ready(TASK_RRC_GNB);
  LOG_I(GNB_APP, "created RRC queue\n");
  return NULL;
}

/* @brief Initialize E1 interface (as if it was CU-CP) */
static void init_e1()
{
  net_ip_address_t ip = {.ipv4 = 1, .ipv4_address = "127.0.0.1" };
  e1ap_net_config_t conf = { .CUCP_e1_ip_address = ip };
  MessageDef *msg = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_REGISTER_REQ);
  e1ap_net_config_t *e1ap_nc = &E1AP_REGISTER_REQ(msg).net_config;
  *e1ap_nc = conf;
  itti_send_msg_to_task(TASK_CUCP_E1, 0, msg);
}

/* @brief Setup CU-UP connection and retrieve parameters for further
 * connection.
 *
 * Waits for E1 Setup Request, sends response, and returns SCTP association ID
 * and NSSAI used by this CU-UP.
 */
static void setup_cuup(sctp_assoc_t *assoc_id, e1ap_nssai_t *nssai)
{
  /* Wait for E1AP setup request, store CU-UP info */
  MessageDef *itti_req = NULL;
  itti_receive_msg(TASK_RRC_GNB, &itti_req);
  MessagesIds id = ITTI_MSG_ID(itti_req);
  AssertFatal(id == E1AP_SETUP_REQ, "expected E1AP setup request, received %s instead\n", itti_get_message_name(id));
  *assoc_id = itti_req->ittiMsgHeader.originInstance;
  e1ap_setup_req_t *req = &E1AP_SETUP_REQ(itti_req);
  long t_id = req->transac_id;
  DevAssert(req->supported_plmns = 1);
  DevAssert(req->plmn[0].slice != NULL);
  *nssai = *req->plmn[0].slice;
  DevAssert(assoc_id != 0);
  free_e1ap_cuup_setup_request(req);
  itti_free(TASK_GNB_APP, itti_req);

  // acknowledge the E1 setup request with a response
  MessageDef *itti_resp = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_SETUP_RESP);
  itti_resp->ittiMsgHeader.originInstance = *assoc_id;
  e1ap_setup_resp_t *resp = &E1AP_SETUP_RESP(itti_resp);
  resp->transac_id = t_id;
  itti_send_msg_to_task(TASK_CUCP_E1, 0, itti_resp);
}

/* @brief get E1AP bearer setup request based on parameters */
static e1ap_bearer_setup_req_t get_breq(uint32_t ue_id, long pdu_id, long drb_id, const e1ap_nssai_t *nssai, const UP_TL_information_t *tnl)
{
  e1ap_bearer_setup_req_t bearer_req = {
    .gNB_cu_cp_ue_id = ue_id,
    .secInfo.cipheringAlgorithm = 0, /* no ciphering/NEA0 */
    .secInfo.integrityProtectionAlgorithm = 0, /* no integrity/NIA0 */
    /* encryptionKey not needed */
    /* integrityProtectionKey not needed */
    .numPDUSessions = 1,
  };
  bearer_req.pduSession = calloc_or_fail(1, sizeof(*bearer_req.pduSession));
  pdu_session_to_setup_t *pdu = &bearer_req.pduSession[0];
  pdu->sessionId = pdu_id;
  pdu->sessionType = E1AP_PDU_Session_Type_ipv4;
  pdu->nssai = *nssai;
  pdu->securityIndication.integrityProtectionIndication = SECURITY_NOT_NEEDED;
  pdu->securityIndication.confidentialityProtectionIndication = SECURITY_NOT_NEEDED;
  pdu->UP_TL_information = *tnl;
  pdu->numDRB2Setup = 1;
  DRB_nGRAN_to_setup_t *drb = &pdu->DRBnGRanList[0];
  drb->id = drb_id;
  drb->numQosFlow2Setup = 1;
  drb->qosFlows[0] = (qos_flow_to_setup_t){
      .qfi = TEST_QOS_FLOW_ID,
      .qos_params.alloc_reten_priority.priority_level = E1AP_PriorityLevel_highest,
      .qos_params.alloc_reten_priority.preemption_capability = E1AP_Pre_emptionCapability_shall_not_trigger_pre_emption,
      .qos_params.alloc_reten_priority.preemption_vulnerability = E1AP_Pre_emptionVulnerability_not_pre_emptable,
      .qos_params.qos_characteristics.qos_type = NON_DYNAMIC,
      .qos_params.qos_characteristics.non_dynamic.fiveqi = 9,
  };
  /* Enable SDAP headers: tests QFI-aware forwarding, so UL needs SDAP QFI marking,
   * and DL payload received on F1-U includes SDAP+PDCP headers. */
  drb->sdap_config.sDAP_Header_UL = true;
  drb->sdap_config.sDAP_Header_DL = true;
  drb->numCellGroups = 1;

  return bearer_req;
}

/* @brief check for packet loss by verifing the sequence number. Can handle
 * packet losses and reordering.
 * @param payload the sequence number of the current packet
 * @param expected the expected sequence number
 * @param [inout] pointer to number of lost packets, which will be updated by
 * the difference between payload and expected.
 * @return the next expected packet number. */
static uint32_t estimate_packetloss(const uint32_t payload, uint32_t expected, uint32_t *lost)
{
  if (payload == expected) {
    /* all good */
    return expected + 1;
  } else if (payload > expected) {
    /* there was packet loss, assume all packets in between have been lost */
    uint32_t diff = payload - expected;
    *lost += diff;
    LOG_W(GNB_APP, "packet loss: expected %d received %d diff %d (total loss %d)\n", expected, payload, diff, *lost);
    return payload + 1;
  } else { /* payload < expected */
     /* reordered: we assumed current packet to be lost, so reduce the
      * loss for this packet as it arrived in the end. */
    *lost -= 1;
    LOG_W(GNB_APP, "packet reordering occurred: expected %d received %d\n", expected, payload);
    return expected; /* we still wait for the next packet */
  }
}

static struct ue_stat {
  uint32_t count;
  uint32_t lost;
  uint64_t received;
} ue_stat[10][2] = {0};
/* @brief ulReceived callback for F1-U
 *
 * Checks for packet loss by verifying that the first 4 bytes are corresponding
 * to a continuous 4-byte (word) enumeration. */
static bool recv_ng(protocol_ctxt_t *ctxt,
                    const ue_id_t ue_id,
                    const srb_flag_t flag,
                    const mui_t mui,
                    const confirm_t confirm,
                    const sdu_size_t size,
                    unsigned char *const buf,
                    const pdcp_transmission_mode_t modeP,
                    const uint32_t *sourceL2Id,
                    const uint32_t *destinationL2Id,
                    const uint8_t qfi,
                    const bool rqi,
                    const int pdusession_id)
{
  uint32_t *payload = (uint32_t *) buf;
  DevAssert(size % 4 == 0);
  struct ue_stat *s = &ue_stat[ue_id][UL];
  s->count = estimate_packetloss(*payload, s->count, &s->lost);
  s->received += size;

  return true;
}

/* @brief Set up the NG-U (north-bound) GTP tunnel, and inform CU-UP via E1 */
static up_params_t setup_cuup_ue_ng(sctp_assoc_t assoc_id, const e1ap_nssai_t *nssai, uint32_t ue_id, instance_t gtp_inst, const char *ip)
{
  long pdu_id = 1;
  long drb_id = pdu_id + 3;
  in_addr_t addr_lo;
  inet_pton(AF_INET, ip, &addr_lo);

  /* create the local tunnel (i.e., as if we were UPF) with N3-U bearer mappings */
  transport_layer_addr_t null_addr = {.length = 32};
  /* Intentionally disable non-SDAP callback on NG-U: this test must fail if packets arrive without QFI. */
  teid_t teid_lo = newGtpuCreateTunnel(gtp_inst, ue_id, pdu_id, pdu_id, -1, null_addr, NULL, recv_ng, NULL);
  UP_TL_information_t tnl = {.teId = teid_lo};
  memcpy(&tnl.tlAddress, &addr_lo, 4);

  /* Create a new bearer */
  MessageDef *itti_breq = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_BEARER_CONTEXT_SETUP_REQ);
  itti_breq->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_setup_req_t *breq = &E1AP_BEARER_CONTEXT_SETUP_REQ(itti_breq);
  *breq = get_breq(ue_id, pdu_id, drb_id, nssai, &tnl);
  itti_send_msg_to_task(TASK_CUCP_E1, 0, itti_breq);

  /* Wait for the ack */
  MessageDef *itti_bresp;
  itti_receive_msg(TASK_RRC_GNB, &itti_bresp);
  MessagesIds id = ITTI_MSG_ID(itti_bresp);
  AssertFatal(id == E1AP_BEARER_CONTEXT_SETUP_RESP, "expected E1AP bearer context setup response, received %s instead\n", itti_get_message_name(id));
  DevAssert(assoc_id == itti_bresp->ittiMsgHeader.originInstance);
  e1ap_bearer_setup_resp_t *bresp = &E1AP_BEARER_CONTEXT_SETUP_RESP(itti_bresp);
  DevAssert(bresp->gNB_cu_cp_ue_id == ue_id);
  DevAssert(bresp->gNB_cu_up_ue_id == ue_id);
  DevAssert(bresp->numPDUSessions == 1);
  teid_t teid_rm = bresp->pduSession[0].tl_info.teId;
  in_addr_t addr_rm;
  memcpy(&addr_rm, &bresp->pduSession[0].tl_info.tlAddress, 4);
  DevAssert(bresp->pduSession[0].numDRBSetup == 1);
  DRB_nGRAN_setup_t *drb = &bresp->pduSession[0].DRBnGRanList[0];
  DevAssert(drb->id == drb_id);
  DevAssert(drb->numUpParam == 1);
  up_params_t f1_up = drb->UpParamList[0];
  free_e1ap_context_setup_response(bresp);
  itti_free(TASK_GNB_APP, itti_bresp);

  /* print diagnostics */
  char ip_lo[32] = {0};
  inet_ntop(AF_INET, &addr_lo, ip_lo, sizeof(ip_lo));
  char ip_rm[32] = {0};
  inet_ntop(AF_INET, &addr_rm, ip_rm, sizeof(ip_rm));
  GtpuUpdateTunnelOutgoingAddressAndTeid(gtp_inst, ue_id, pdu_id, addr_rm, teid_rm);
  LOG_I(GNB_APP, "CU-UP created NG-U, local %s/%x remote %s/%x\n", ip_lo, teid_lo, ip_rm, teid_rm);

  inet_ntop(AF_INET, &f1_up.tl_info.tlAddress, ip_rm, sizeof(ip_rm));
  LOG_I(GNB_APP, "CU-UP created F1-U, remove %s/%x\n", ip_rm, f1_up.tl_info.teId);
  return f1_up;
}

/* @brief get E1AP bearer modification request based on parameters */
static e1ap_bearer_mod_req_t get_bmod(uint32_t ue_id, long pdu_id, up_params_t up)
{
  e1ap_bearer_mod_req_t bearer_mod = {
    .gNB_cu_cp_ue_id = ue_id,
    .gNB_cu_up_ue_id = ue_id,
    .numPDUSessionsMod = 1,
  };
  bearer_mod.pduSessionMod = calloc_or_fail(1, sizeof(*bearer_mod.pduSessionMod));
  pdu_session_to_mod_t *pdum = &bearer_mod.pduSessionMod[0];
  pdum->sessionId = pdu_id;
  pdum->numDRB2Modify = 1;
  DRB_nGRAN_to_mod_t *drb = &pdum->DRBnGRanModList[0];
  drb->id = 4;
  drb->numDlUpParam = 1;
  drb->DlUpParamList[0] = up;

  return bearer_mod;
}

/* @brief Received callback for F1-U
 *
 * Checks for packet loss by verifying that the first 4 bytes are corresponding
 * to a continuous 4-byte (word) enumeration. */
static bool recv_f1(protocol_ctxt_t *ctxt,
                    const srb_flag_t flag,
                    const rb_id_t rb,
                    const mui_t mui,
                    const confirm_t confirm,
                    const sdu_size_t size,
                    unsigned char *const buf,
                    const pdcp_transmission_mode_t modeP,
                    const uint32_t *sourceL2Id,
                    const uint32_t *destinationL2Id)
{
  /* F1-U DL payload contains PDCP (2 bytes) + SDAP (1 byte) header */
  int skip_bytes = 3;
  uint32_t payload;
  memcpy(&payload, &buf[skip_bytes], sizeof(uint32_t));
  DevAssert((size - skip_bytes) % 4 == 0);
  int ue_id = ctxt->rntiMaybeUEid;
  struct ue_stat *s = &ue_stat[ue_id][DL];
  s->count = estimate_packetloss(payload, s->count, &s->lost);
  s->received += size - skip_bytes;

  return true;
}

/* @brief Set up the F1-U (south-bound) GTP tunnel, and inform CU-UP via E1 */
static void setup_cuup_ue_f1(sctp_assoc_t assoc_id, uint32_t ue_id, instance_t gtp_inst, const char *ip, up_params_t rm)
{
  int pdu_id = 1;
  int drb_id = pdu_id + 3;

  /* create tunnel (i.e., as if we were DU) */
  transport_layer_addr_t addr = {.length = 32};
  memcpy(addr.buffer, &rm.tl_info.tlAddress, 4);
  teid_t teid_lo = newGtpuCreateTunnel(gtp_inst, ue_id, drb_id, drb_id, rm.tl_info.teId, addr, recv_f1, NULL, NULL);

  up_params_t lo = {.tl_info.teId = teid_lo, .cell_group_id = rm.cell_group_id};
  inet_pton(AF_INET, ip, &lo.tl_info.tlAddress);

  /* modify existing tunnel via E1 to pass in F1-U TEID */
  MessageDef *itti_bmod = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_BEARER_CONTEXT_MODIFICATION_REQ);
  itti_bmod->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_mod_req_t *bmod = &E1AP_BEARER_CONTEXT_MODIFICATION_REQ(itti_bmod);
  *bmod = get_bmod(ue_id, pdu_id, lo);
  itti_send_msg_to_task(TASK_CUCP_E1, 0, itti_bmod);

  /* Wait for the ack */
  MessageDef *itti_bmodr;
  itti_receive_msg(TASK_RRC_GNB, &itti_bmodr);
  MessagesIds id = ITTI_MSG_ID(itti_bmodr);
  AssertFatal(id == E1AP_BEARER_CONTEXT_MODIFICATION_RESP, "expected E1AP bearer context modification response, received %s instead\n", itti_get_message_name(id));
  //DevAssert(assoc_id == itti_bmodr->ittiMsgHeader.originInstance);
  e1ap_bearer_modif_resp_t *bmodr = &E1AP_BEARER_CONTEXT_MODIFICATION_RESP(itti_bmodr);
  free_e1ap_context_mod_response(bmodr);
  itti_free(TASK_GNB_APP, itti_bmodr);

  /* print diagnostics */
  char ip_lo[32] = {0};
  inet_ntop(AF_INET, &lo.tl_info.tlAddress, ip_lo, sizeof(ip_lo));
  char ip_rm[32] = {0};
  inet_ntop(AF_INET, &rm.tl_info.tlAddress, ip_rm, sizeof(ip_rm));
  LOG_I(GNB_APP, "CU-UP created F1-U, local %s/%x remote %s/%x\n", ip_lo, lo.tl_info.teId, ip_rm, rm.tl_info.teId);
}

/* @brief release the UE over E1, and delete corresponding GTP tunnels */
static void e1_release_ue(sctp_assoc_t assoc_id, uint32_t ue_id, instance_t f1_inst, instance_t ng_inst)
{
  MessageDef *itti_cmd = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_BEARER_CONTEXT_RELEASE_CMD);
  itti_cmd->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_release_cmd_t *cmd = &E1AP_BEARER_CONTEXT_RELEASE_CMD(itti_cmd);
  cmd->gNB_cu_cp_ue_id = cmd->gNB_cu_up_ue_id = ue_id;
  cmd->cause.type = E1AP_CAUSE_RADIO_NETWORK;
  cmd->cause.value = E1AP_RADIO_CAUSE_NORMAL_RELEASE;
  itti_send_msg_to_task(TASK_CUCP_E1, 0, itti_cmd);

  /* Wait for the ack */
  MessageDef *itti_cplt;
  itti_receive_msg(TASK_RRC_GNB, &itti_cplt);
  MessagesIds id = ITTI_MSG_ID(itti_cplt);
  AssertFatal(id == E1AP_BEARER_CONTEXT_RELEASE_CPLT, "expected E1AP bearer context release complete, received %s instead\n", itti_get_message_name(id));
  itti_free(TASK_GNB_APP, itti_cplt);

  newGtpuDeleteAllTunnels(f1_inst, ue_id);
  newGtpuDeleteAllTunnels(ng_inst, ue_id);
}

/* @brief initialize GTP interface for IP:port */
static instance_t init_gtp(const char *ip, uint16_t port)
{
  openAddr_t iface = {0};
  DevAssert(strlen(ip) <= sizeof(iface.originHost));
  strcpy(iface.originHost, ip);
  snprintf(iface.originService, sizeof(iface.originService), "%d", port);
  snprintf(iface.destinationService, sizeof(iface.destinationService), "%d", port);
  return gtpv1Init(iface);
}

/* @brief calculate time difference of two struct timespec */
static float time_diff(struct timespec s, struct timespec e)
{
  //printf("start %ld.%ld end %ld.%ld\n", s.tv_sec, s.tv_nsec, e.tv_sec, e.tv_nsec);
  float dur = e.tv_sec - s.tv_sec + (e.tv_nsec - s.tv_nsec) / 1000000000.f;
  //printf("duration %f s %f us\n", dur, dur * 1000000.f);
  return dur * 1000000.f;
}

enum pdcp_len {PDCP_SN_LEN_12, PDCP_SN_LEN_18};
/* @brief generate hardcoded PDCP header in case of UL transmission */
static void write_pdcp_header(uint32_t sn, enum pdcp_len len, uint8_t *buf)
{
  DevAssert(len == PDCP_SN_LEN_12);
  /* see nr_pdcp_entity_process_sdu() or 38.323 sec 6.2.2.2 */
  buf[0] = 0x80 | ((sn >> 8) & 0xf);
  buf[1] = sn & 0xff;
  //printf("%02x %02x\n", buf[0], buf[1]);
}

/* @brief the configuration for a thread to send GTP traffic */
struct thr_data {
  /// pthread ID to join this thread
  pthread_t t;

  /* in */
  /// GTP instance to send on
  instance_t inst;
  /// UE ID to be used
  ue_id_t ue_id;
  /// flag to decide if PDCP/SDAP header is to be prepended
  bool is_ul;
  /// length of data packet
  unsigned int data_len;
  /// target throughput to reach
  uint64_t throughput;
  /// duration of the data traffic to last
  unsigned int duration;

  /* out */
  /// exact measured transmission time
  float total_time;
  /// exact measured data that has been sent
  uint64_t total_data;
};
/* @brief Main traffic generation/transmission function.
 *
 * The function calculates the amount of packets to be sent during each TTI
 * (one millisecond) (as a float, to send the exact amount of packets). It then
 * enters a loop, in which (1) it sends the packets of the current TTI, possibly
 * including additional packets if they could not be sent in the last TTI, (2)
 * measure the time since the last TTI boundary, and sleep, if necessary, to
 * adjust to the exact throughput (3) update for additional packets to send if
 * the last TTI took too long. The last point ensure that we send exactly an
 * exact amount of data, possibly taking longer than specified.
 *
 * @param v pointer to generation function of type `struct thr_data`.
 * @return always NULL */
static void *sender_thread(void *v)
{
  struct thr_data *d = v;

  // UL F1-U payload overhead: PDCP header (2 bytes) + SDAP header (1 byte)
  size_t oh = d->is_ul ? 3 : 0;
  size_t packet_len = d->data_len + oh;
  uint8_t buf[packet_len];
  memset(buf, 0, packet_len);
  /* SDAP UL data PDU: bits[5:0]=QFI (TEST_QOS_FLOW_ID=1), bit6=R=0, bit7=DC=0 (data PDU) */
  buf[2] = TEST_QOS_FLOW_ID;
  uint32_t *payload = (uint32_t *)(buf + oh);
  uint64_t to_send = (float)d->throughput / 8.0 * d->duration;
  const float pps_1ms = (float) d->throughput / 8.0 / d->data_len / 1000;
  float pps_1ms_nextwin = pps_1ms;
  int bearer = d->is_ul ? 4 : 1;
  const int dl_qfi = TEST_QOS_FLOW_ID;

  //printf("sending %ld Mbps for %ld s => data %ld, %.3f pps\n", (float) d->throughput / 1000000.0, d->duration, to_send, pps_1ms);

  int i_1ms = 0;
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  size_t packet_count = 0;
  while (true) {
    int pps_now = pps_1ms_nextwin;
    pps_1ms_nextwin = pps_1ms + pps_1ms_nextwin - (float) pps_now;
    //printf("this window send %d packets next window %.3f (pps_1ms %.3f)\n", pps_now, pps_1ms_nextwin, pps_1ms);
    for (uint32_t i = 0; i < pps_now; ++i) {
      if (d->is_ul)
        write_pdcp_header(packet_count, PDCP_SN_LEN_12, buf);
      memcpy(payload, &packet_count, sizeof(packet_count));
      packet_count++;
      if (d->is_ul) { // UL direction: F1-U tunnel
        gtpv1uSendDirect(d->inst, d->ue_id, bearer, (uint8_t *)buf, packet_len, false, false);
      } else { // DL direction: NG-U tunnel with SDAP enabled
        gtpv1uSendDirectWithQFI(d->inst, d->ue_id, bearer, dl_qfi, (uint8_t *)buf, packet_len);
      }
      d->total_data += d->data_len;
    }

    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    float diff_us = time_diff(start, t);
    int us_passed = diff_us - i_1ms*1000;
    int iter_advance = us_passed / 1000 + 1;
    int wait_usec = 1000 - us_passed;
    //printf("diff %.3fus i_1ms %d us_passed %d iter %d wait %d (%.3f %%)\n", diff_us, i_1ms, us_passed, iter_advance, wait_usec, 100.0 * d->total_data / to_send);
    DevAssert(iter_advance > 0);
    if (wait_usec > 0)
      usleep(wait_usec);

    if (iter_advance > 1) /* we have to send more in the next window */
      pps_1ms_nextwin += (iter_advance - 1) * pps_1ms_nextwin;
    i_1ms += iter_advance;
    if (d->total_data >= to_send)
      break;
  }
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  d->total_time = time_diff(start, end) / 1000000.f;
  return NULL;
}

/* @brief print statistics of a UE (amount of data, achieved rate, if data was
 * lost). */
static bool print_stats(const char *pref, int ue_id, uint64_t total_data, float duration, const struct ue_stat *s)
{
  float tthr = 8.0 * total_data / duration / 1000000.f;
  float rthr = 8.0 * s->received / duration / 1000000.f;
  LOG_I(GNB_APP, "%d %s sent %ld in %.3f s received %ld bytes => rates [Mbps] transmit %.3f receive %.3f\n",
         ue_id,
         pref,
         total_data,
         duration,
         s->received,
         tthr,
         rthr);
  if (total_data == s->received) {
    DevAssert(s->lost == 0);
    LOG_A(GNB_APP, "%d %s received all data sent\n", ue_id, pref);
    return true;
  } else {
    uint64_t m = total_data - s->received;
    LOG_W(GNB_APP, "%d %s warning: missing %ld bytes (%.3f %%, %d packets)\n", ue_id, pref, m, 100.f * m / total_data, s->lost);
    return false;
  }
}

/* @brief start a thread if specified through template, and in direction of
 * given GTP bearer.
 * @param template specifies if thread is to be started (throughput and
 * duration are positive). If so, the template data is stored inside the
 * thread.
 * @param thr place to store thread data
 * @param b the GTP bearer to use */
static struct thr_data *start_thread(const struct thr_data *template, struct thr_data *thr, instance_t inst, ue_id_t ue_id)
{
  if (template->throughput == 0 || template->duration == 0)
    return NULL;

  *thr = *template;
  thr->inst = inst;
  thr->ue_id = ue_id;
  pthread_create(&thr->t, NULL, sender_thread, thr);
  return thr;
}

/* @brief print progress of a thread specified in thr_data, and prepend DL/UL
 * based on dir. data access to thread data is not private, but we only read,
 * so don't care. */
static bool print_progress(const int dir, const struct thr_data *obs)
{
  if (!obs)
    return false;

  uint64_t target = (uint64_t) obs->throughput / 8 * obs->duration;
  float progress = (float)obs->total_data / target;
  printf("   %s sent %3.f %%", dir == DL ? "DL" : "UL", 100.0 * progress);
  return progress < 0.99;
}

int main(int argc, char *argv[])
{
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }
  logInit();
  itti_init(TASK_MAX, tasks_info);

  unsigned int n_ue;
  unsigned int duration_s;
  unsigned int packet_size;
  uint64_t target_rate_dl;
  uint64_t target_rate_ul;
  char *ng_ip = malloc_or_fail(100);
  uint16_t ng_port;
  char *f1_ip = malloc_or_fail(100);
  uint16_t f1_port;

  paramdef_t param_def[] = {
      {"n", "number of UEs\n", 0, .uptr = &n_ue, .defuintval = 1, TYPE_UINT},
      {"t", "duration of test (s)\n", 0, .uptr = &duration_s, .defuintval = 10, TYPE_UINT},
      {"p", "packet size\n", 0, .uptr = &packet_size, .defuintval = 1400, TYPE_UINT},
      {"d", "DL target rate (Mbps, per UE)\n", 0, .u64ptr = &target_rate_dl, .defint64val = 60, TYPE_UINT64},
      {"u", "UL target rate (Mbps, per UE)\n", 0, .u64ptr = &target_rate_ul, .defint64val = 60, TYPE_UINT64},
      {"ng-ip", "Tester NG-U IP\n", 0, .strptr = &ng_ip, .defstrval = "127.0.0.1", TYPE_STRING, 100},
      {"ng-port", "Tester NG-U port\n", 0, .u16ptr = &ng_port, .defuintval = 2152, TYPE_UINT16},
      {"f1-ip", "Tester F1-U IP\n", 0, .strptr = &f1_ip, .defstrval = "127.0.0.1", TYPE_STRING, 100},
      {"f1-port", "Tester F1-U port\n", 0, .u16ptr = &f1_port, .defuintval = 2153, TYPE_UINT16},
  };
  config_get(uniqCfg, param_def, sizeofArray(param_def), NULL);
  // on command line, target rate is in Mbps, convert to bps
  target_rate_dl *= 1000000LL;
  target_rate_ul *= 1000000LL;
  LOG_I(GNB_APP,
        "CU-UP load tester: running test for\n"
        "- Number of UEs %d\n"
        "- Duration %d seconds\n"
        "- Packet size %d\n"
        "- Target DL rate %.3f Mbps (per UE)\n"
        "- Target UL rate %.3f Mbps (per UE)\n"
        "- Tester endpoint for CU-UP NG-U: %s:%d\n"
        "- Tester endpoint for CU-UP F1-U: %s:%d\n",
        n_ue, duration_s, packet_size,
        (float) target_rate_dl / 1e6,
        (float) target_rate_ul / 1e6,
        ng_ip, ng_port, f1_ip, f1_port);

  struct thr_data dl_template = {.is_ul = false, .data_len = packet_size, .throughput = target_rate_dl, .duration = duration_s};
  struct thr_data ul_template = {.is_ul = true, .data_len = packet_size, .throughput = target_rate_ul, .duration = duration_s};

  /* init GTP for NG-U comm with CU-UP */
  instance_t ng_inst = init_gtp(ng_ip, ng_port);
  AssertFatal(ng_inst > 0, "could not initialize GTP interface on %s:%d towards CU-UP NG-U\n", ng_ip, ng_port);

  /* init GTP for F1-U comm with CU-UP */
  instance_t f1_inst = init_gtp(f1_ip, f1_port);
  AssertFatal(f1_inst > 0, "could not initialize GTP interface on %s:%d towards CU-UP F1-U\n", f1_ip, f1_port);

  /* Start relevant services: SCTP, CUCP_E1: E1; GTP not actively used; RRC to
   * register the queue so we can receive what E1 sends us back */
  int rc;
  rc = itti_create_task(TASK_SCTP, sctp_eNB_task, NULL);
  AssertFatal(rc >= 0, "Create task for SCTP failed\n");
  rc = itti_create_task(TASK_CUCP_E1, E1AP_CUCP_task, NULL);
  AssertFatal(rc >= 0, "Create task for CUUP E1 failed\n");
  rc = itti_create_task(TASK_GTPV1_U, gtpv1uTask, NULL);
  AssertFatal(rc >= 0, "Create task for GTPV1U failed\n");
  rc = itti_create_task(TASK_RRC_GNB, cuup_tester_rrc, NULL);
  AssertFatal(rc >= 0, "Create task for RRC failed\n");
  init_e1();

  /* Set up CU-UP connection: waits for CU-UP-initiated SCTP+E1 setup request */
  sctp_assoc_t assoc_id;
  e1ap_nssai_t nssai;
  setup_cuup(&assoc_id, &nssai);

  /* Set up UE contexts in CU-UP. the below two calls correspond to
   * - setup_cuup_ue_ng(): E1 bearer context setup <=> completely sets up NG-U
   *   in CU-UP, gets TEID for F1-U UL
   * - setup_cuup_ue_f1(): E1 bearer context modification <=> send TEID for
   *   F1-U DL (as if we had talked to the DU in the middle) */
  for (int ue = 0; ue < n_ue; ++ue) {
    up_params_t f1_up = setup_cuup_ue_ng(assoc_id, &nssai, ue, ng_inst, ng_ip);
    setup_cuup_ue_f1(assoc_id, ue, f1_inst, f1_ip, f1_up);
  }

  usleep(1000);

  /* for each UE, start DL&UL data streams in dedicated thread, if configured */
  struct thr_data thrd[n_ue][2];
  memset(thrd, 0, sizeof(thrd));
  struct thr_data *obs_dl = NULL, *obs_ul = NULL;
  for (int ue = 0; ue < n_ue; ++ue) {
    obs_dl = start_thread(&dl_template, &thrd[ue][DL], ng_inst, ue);
    obs_ul = start_thread(&ul_template, &thrd[ue][UL], f1_inst, ue);
  }
  /* show progress of one thread (all take the same time) */
  while (obs_dl || obs_ul) {
    bool dlrunning = print_progress(DL, obs_dl);
    bool ulrunning = print_progress(UL, obs_ul);
    printf("\r");
    fflush(stdout);
    if (!dlrunning && !ulrunning)
      break;
    usleep(100000);
  }
  /* join threads, if applicable */
  for (int ue = 0; ue < n_ue; ++ue) {
    struct thr_data *dl = &thrd[ue][DL];
    if (dl->t)
      pthread_join(dl->t, NULL);
    struct thr_data *ul = &thrd[ue][UL];
    if (ul->t)
      pthread_join(ul->t, NULL);
  }

  /* wait a bit, to ensure all data is received after having sent it above */
  sleep(1);

  for (int ue = 0; ue < n_ue; ++ue) {
    e1_release_ue(assoc_id, ue, f1_inst, ng_inst);
  }

  /* show stats of all threads */
  bool success = true;
  for (int ue = 0; ue < n_ue; ++ue) {
    struct thr_data *thrddl = &thrd[ue][DL];
    struct ue_stat *statdl = &ue_stat[ue][DL];
    if (thrddl->t)
      if (!print_stats("DL", ue, thrddl->total_data, thrddl->total_time, statdl))
        success = false;
    struct thr_data *thrdul = &thrd[ue][UL];
    struct ue_stat *statul = &ue_stat[ue][UL];
    if (thrdul->t)
      if (!print_stats("UL", ue, thrdul->total_data, thrdul->total_time, statul))
        success = false;
  }

  free(ng_ip);
  free(f1_ip);

  if (success)
    LOG_A(GNB_APP, "test succeeded\n");
  else
    LOG_W(GNB_APP, "TEST FAILED\n");
  return success ? 0 : 1;
}
