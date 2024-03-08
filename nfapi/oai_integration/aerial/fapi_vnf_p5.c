/*
* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The OpenAirInterface Software Alliance licenses this file to You under
* the OAI Public License, Version 1.1  (the "License"); you may not use this file
* except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.openairinterface.org/?page_id=698
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*-------------------------------------------------------------------------------
* For more information about the OpenAirInterface (OAI) Software Alliance:
*      contact@openairinterface.org
 */

/*! \file fapi/oai-integration/fapi_vnf_p5.c
* \brief OAI FAPI VNF P5 message handler procedures
* \author Ruben S. Silva
* \date 2023
* \version 0.1
* \company OpenAirInterface Software Alliance
* \email: contact@openairinterface.org, rsilva@allbesmart.pt
* \note
* \warning
 */
#ifdef ENABLE_AERIAL
#include "fapi_vnf_p5.h"
#include "fapi_vnf_p7.h"
#include "nfapi/open-nFAPI/nfapi/src/nfapi_p5.c"
#include "nfapi/open-nFAPI/vnf/inc/vnf_p7.h"

extern RAN_CONTEXT_t RC;
extern UL_RCC_IND_t UL_RCC_INFO;
extern int single_thread_flag;
extern uint16_t sf_ahead;
extern uint16_t slot_ahead;


static pthread_t vnf_aerial_p7_start_pthread;
void *aerial_vnf_nr_aerial_p7_start_thread(void *ptr)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);
  pthread_setname_np(pthread_self(), "VNF_P7_AERIAL");
  nfapi_vnf_p7_config_t *config = (nfapi_vnf_p7_config_t *)ptr;
  aerial_nfapi_nr_vnf_p7_start(config);
  return config;
}

void *aerial_vnf_nr_p7_thread_start(void *ptr)
{
  // set_thread_priority(79);
  int s;
  cpu_set_t cpuset;

  CPU_SET(8, &cpuset);
  s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
    printf("failed to set afinity\n");

  set_priority(79);

  pthread_attr_t ptAttr;
  if (pthread_attr_setschedpolicy(&ptAttr, SCHED_RR) != 0) {
    printf("Failed to set pthread sched policy SCHED_RR\n");
  }

  pthread_attr_setinheritsched(&ptAttr, PTHREAD_EXPLICIT_SCHED);
  struct sched_param thread_params;
  thread_params.sched_priority = 20;

  if (pthread_attr_setschedparam(&ptAttr, &thread_params) != 0) {
    printf("failed to set sched param\n");
  }

  init_queue(&gnb_rach_ind_queue);
  init_queue(&gnb_rx_ind_queue);
  init_queue(&gnb_crc_ind_queue);
  init_queue(&gnb_uci_ind_queue);
  init_queue(&gnb_slot_ind_queue);

  vnf_p7_info *p7_vnf = (vnf_p7_info *)ptr;
  p7_vnf->config->port = p7_vnf->local_port;
  p7_vnf->config->sync_indication = &aerial_phy_sync_indication;
  p7_vnf->config->slot_indication = &aerial_phy_slot_indication;
  p7_vnf->config->harq_indication = &aerial_phy_harq_indication;
  p7_vnf->config->nr_crc_indication = &aerial_phy_nr_crc_indication;
  p7_vnf->config->nr_rx_data_indication = &aerial_phy_nr_rx_data_indication;
  p7_vnf->config->nr_rach_indication = &aerial_phy_nr_rach_indication;
  p7_vnf->config->nr_uci_indication = &aerial_phy_nr_uci_indication;
  p7_vnf->config->srs_indication = &aerial_phy_srs_indication;
  p7_vnf->config->sr_indication = &aerial_phy_sr_indication;
  p7_vnf->config->cqi_indication = &aerial_phy_cqi_indication;
  p7_vnf->config->lbt_dl_indication = &aerial_phy_lbt_dl_indication;
  p7_vnf->config->nb_harq_indication = &aerial_phy_nb_harq_indication;
  p7_vnf->config->nrach_indication = &aerial_phy_nrach_indication;
  p7_vnf->config->nr_slot_indication = &aerial_phy_nr_slot_indication;
  p7_vnf->config->nr_srs_indication = &aerial_phy_nr_srs_indication;
  p7_vnf->config->malloc = &aerial_vnf_allocate;
  p7_vnf->config->free = &aerial_vnf_deallocate;
  p7_vnf->config->vendor_ext = &aerial_phy_vendor_ext;
  p7_vnf->config->user_data = p7_vnf;
  p7_vnf->mac->user_data = p7_vnf;
  p7_vnf->config->codec_config.unpack_p7_vendor_extension = &aerial_phy_unpack_p7_vendor_extension;
  p7_vnf->config->codec_config.pack_p7_vendor_extension = &aerial_phy_pack_p7_vendor_extension;
  p7_vnf->config->codec_config.unpack_vendor_extension_tlv = &aerial_phy_unpack_vendor_extension_tlv;
  p7_vnf->config->codec_config.pack_vendor_extension_tlv = &aerial_phy_pack_vendor_extension_tlv;
  p7_vnf->config->codec_config.allocate = &aerial_vnf_allocate;
  p7_vnf->config->codec_config.deallocate = &aerial_vnf_deallocate;
  p7_vnf->config->allocate_p7_vendor_ext = &aerial_phy_allocate_p7_vendor_ext;
  p7_vnf->config->deallocate_p7_vendor_ext = &aerial_phy_deallocate_p7_vendor_ext;
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Creating VNF NFAPI P7 start thread %s\n", __FUNCTION__);
  pthread_create(&vnf_aerial_p7_start_pthread, NULL, &aerial_vnf_nr_aerial_p7_start_thread, p7_vnf->config);
  return 0;
}


int aerial_pnf_nr_connection_indication_cb(nfapi_vnf_config_t *config, int p5_idx)
{
  printf("[VNF] pnf connection indication idx:%d\n", p5_idx);
  // in aerial, send first CONFIG.request, not PARAM.request
  nfapi_nr_config_request_scf_t conf_req;
  memset(&conf_req, 0, sizeof(conf_req));
  conf_req.header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST;
  printf("Try to send first CONFIG.request\n");
  aerial_nr_send_config_request(config, p5_idx);
  return 0;
}

int aerial_pnf_disconnection_indication_cb(nfapi_vnf_config_t *config, int p5_idx)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] pnf disconnection indication idx:%d\n", p5_idx);
  vnf_info *vnf = (vnf_info *)(config->user_data);
  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;
  nfapi_vnf_p7_del_pnf((p7_vnf->config), phy->id);
  return 0;
}

int aerial_pnf_nr_param_resp_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_nr_pnf_param_response_t *resp)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] pnf param response idx:%d error:%d\n", p5_idx, resp->error_code);
  vnf_info *vnf = (vnf_info *)(config->user_data);
  pnf_info *pnf = vnf->pnfs;

  for (int i = 0; i < resp->pnf_phy.number_of_phys; ++i) {
    phy_info phy;
    memset(&phy, 0, sizeof(phy));
    phy.index = resp->pnf_phy.phy[i].phy_config_index;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] (PHY:%d) phy_config_idx:%d\n", i, resp->pnf_phy.phy[i].phy_config_index);
    nfapi_vnf_allocate_phy(config, p5_idx, &(phy.id));

    for (int j = 0; j < resp->pnf_phy.phy[i].number_of_rfs; ++j) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] (PHY:%d) (RF%d) %d\n", i, j, resp->pnf_phy.phy[i].rf_config[j].rf_config_index);
      phy.rfs[0] = resp->pnf_phy.phy[i].rf_config[j].rf_config_index;
    }

    pnf->phys[0] = phy;
  }
  nfapi_nr_pnf_config_request_t req;
  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_PNF_CONFIG_REQUEST;
  req.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
  req.pnf_phy_rf_config.number_phy_rf_config_info = 2; // DJP pnf.phys.size();
  NFAPI_TRACE(NFAPI_TRACE_INFO, "DJP:Hard coded num phy rf to 2\n");

  for (unsigned i = 0; i < 2; ++i) {
    req.pnf_phy_rf_config.phy_rf_config[i].phy_id = pnf->phys[i].id;
    req.pnf_phy_rf_config.phy_rf_config[i].phy_config_index = pnf->phys[i].index;
    req.pnf_phy_rf_config.phy_rf_config[i].rf_config_index = pnf->phys[i].rfs[0];
  }

  nfapi_nr_vnf_pnf_config_req(config, p5_idx, &req);
  return 0;
}

int aerial_pnf_nr_config_resp_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_nr_pnf_config_response_t *resp)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "[VNF] pnf config response idx:%d resp[header[phy_id:%u message_id:%02x message_length:%u]]\n",
              p5_idx,
              resp->header.phy_id,
              resp->header.message_id,
              resp->header.message_length);

  if (1) {
    nfapi_nr_pnf_start_request_t req;
    memset(&req, 0, sizeof(req));
    req.header.phy_id = resp->header.phy_id;
    req.header.message_id = NFAPI_PNF_START_REQUEST;
    nfapi_nr_vnf_pnf_start_req(config, p5_idx, &req);
  } else {
    // Rather than send the pnf_start_request we will demonstrate
    // sending a vendor extention message. The start request will be
    // send when the vendor extension response is received
    // vnf_info* vnf = (vnf_info*)(config->user_data);
    vendor_ext_p5_req req;
    memset(&req, 0, sizeof(req));
    req.header.message_id = P5_VENDOR_EXT_REQ;
    req.dummy1 = 45;
    req.dummy2 = 1977;
    nfapi_vnf_vendor_extension(config, p5_idx, &req.header);
  }

  return 0;
}

int aerial_pnf_nr_start_resp_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_nr_pnf_start_response_t *resp)
{
  vnf_info *vnf = (vnf_info *)(config->user_data);
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;
  pnf_info *pnf = vnf->pnfs;
  nfapi_nr_param_request_scf_t req;
  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "[VNF] pnf start response idx:%d config:%p user_data:%p p7_vnf[config:%p thread_started:%d]\n",
              p5_idx,
              config,
              config->user_data,
              vnf->p7_vnfs[0].config,
              vnf->p7_vnfs[0].thread_started);

  if (p7_vnf->thread_started == 0) {
    pthread_t vnf_p7_thread;
    pthread_create(&vnf_p7_thread, NULL, &aerial_vnf_nr_p7_thread_start, p7_vnf);
    p7_vnf->thread_started = 1;
  } else {
    // P7 thread already running.
  }

  // start all the phys in the pnf.
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Sending NFAPI_VNF_PARAM_REQUEST phy_id:%d\n", pnf->phys[0].id);
  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST;
  req.header.phy_id = pnf->phys[0].id;
  nfapi_nr_vnf_param_req(config, p5_idx, &req);
  return 0;
}

int aerial_nr_param_resp_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_nr_param_response_scf_t *resp)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Received NFAPI_PARAM_RESP idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);
  vnf_info *vnf = (vnf_info *)(config->user_data);
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;
  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;
  struct sockaddr_in pnf_p7_sockaddr;
  nfapi_nr_config_request_scf_t *req = &RC.nrmac[0]->config[0]; // check
  phy->remote_port = resp->nfapi_config.p7_pnf_port.value;
  // phy->remote_port = 32123;//resp->nfapi_config.p7_pnf_port.value;
  memcpy(&pnf_p7_sockaddr.sin_addr.s_addr, &(resp->nfapi_config.p7_pnf_address_ipv4.address[0]), 4);
  phy->remote_addr = inet_ntoa(pnf_p7_sockaddr.sin_addr);
  // for now just 1
  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "[VNF] %d.%d pnf p7 %s:%d timing %u %u %u %u\n",
              p5_idx,
              phy->id,
              phy->remote_addr,
              phy->remote_port,
              p7_vnf->timing_window,
              p7_vnf->periodic_timing_period,
              p7_vnf->aperiodic_timing_enabled,
              p7_vnf->periodic_timing_period);
  req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST;
  req->header.phy_id = phy->id;
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Send NFAPI_CONFIG_REQUEST\n");
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "\n NR bandP =%d\n",req->nfapi_config.rf_bands.rf_band[0]);

  req->nfapi_config.p7_vnf_port.tl.tag = NFAPI_NR_NFAPI_P7_VNF_PORT_TAG;
  req->nfapi_config.p7_vnf_port.value = p7_vnf->local_port;
  req->num_tlv++;
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] DJP local_port:%d\n", ntohs(p7_vnf->local_port));
  req->nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
  struct sockaddr_in vnf_p7_sockaddr;
  vnf_p7_sockaddr.sin_addr.s_addr = inet_addr(p7_vnf->local_addr);
  memcpy(&(req->nfapi_config.p7_vnf_address_ipv4.address[0]), &vnf_p7_sockaddr.sin_addr.s_addr, 4);
  req->num_tlv++;
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] DJP local_addr:%s\n", p7_vnf->local_addr);
  req->nfapi_config.timing_window.tl.tag = NFAPI_NR_NFAPI_TIMING_WINDOW_TAG;
  req->nfapi_config.timing_window.value = p7_vnf->timing_window;
  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "\n[VNF]Timing window tag : %d Timing window:%u\n",
              NFAPI_NR_NFAPI_TIMING_WINDOW_TAG,
              p7_vnf->timing_window);
  req->num_tlv++;

  if (p7_vnf->periodic_timing_enabled || p7_vnf->aperiodic_timing_enabled) {
    req->nfapi_config.timing_info_mode.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG;
    req->nfapi_config.timing_info_mode.value = (p7_vnf->aperiodic_timing_enabled << 1) | (p7_vnf->periodic_timing_enabled);
    req->num_tlv++;

    if (p7_vnf->periodic_timing_enabled) {
      req->nfapi_config.timing_info_period.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG;
      req->nfapi_config.timing_info_period.value = p7_vnf->periodic_timing_period;
      req->num_tlv++;
    }
  }
  // TODO: Assign tag and value for P7 message offsets
  req->nfapi_config.dl_tti_timing_offset.tl.tag = NFAPI_NR_NFAPI_DL_TTI_TIMING_OFFSET;
  req->nfapi_config.ul_tti_timing_offset.tl.tag = NFAPI_NR_NFAPI_UL_TTI_TIMING_OFFSET;
  req->nfapi_config.ul_dci_timing_offset.tl.tag = NFAPI_NR_NFAPI_UL_DCI_TIMING_OFFSET;
  req->nfapi_config.tx_data_timing_offset.tl.tag = NFAPI_NR_NFAPI_TX_DATA_TIMING_OFFSET;

  vendor_ext_tlv_2 ve2;
  memset(&ve2, 0, sizeof(ve2));
  ve2.tl.tag = VENDOR_EXT_TLV_2_TAG;
  ve2.dummy = 2016;
  req->vendor_extension = &ve2.tl;
  nfapi_nr_vnf_config_req(config, p5_idx, req);
  printf("[VNF] Sent NFAPI_VNF_CONFIG_REQ num_tlv:%u\n", req->num_tlv);
  return 0;
}

int aerial_nr_send_config_request(nfapi_vnf_config_t *config, int p5_idx)
{
  vnf_info *vnf = (vnf_info *)(config->user_data);
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;
  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;

  nfapi_nr_config_request_scf_t *req = &RC.nrmac[0]->config[0]; //&RC.gNB[0]->gNB_config; // check

  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "[VNF] %d.%d pnf p7 %s:%d timing %u %u %u %u\n",
              p5_idx,
              phy->id,
              phy->remote_addr,
              phy->remote_port,
              p7_vnf->timing_window,
              p7_vnf->periodic_timing_period,
              p7_vnf->aperiodic_timing_enabled,
              p7_vnf->periodic_timing_period);
  req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST;
  req->header.phy_id = phy->id;
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Send NFAPI_CONFIG_REQUEST\n");


  vnf_t *_this = (vnf_t *)(config);

  nfapi_vnf_phy_info_t *vnf_phy = nfapi_vnf_phy_info_list_find(config, req->header.phy_id);

  if (vnf_phy == NULL) {
    printf("%s failed to find phy information phy_id:%d\n", __FUNCTION__, req->header.phy_id);
    return -1;
  }

  nfapi_p4_p5_message_header_t *msg = &req->header;
  uint16_t msg_len = sizeof(nfapi_nr_config_request_scf_t);
  nfapi_p4_p5_message_header_t *msgFAPI = calloc(1, msg_len);
  memcpy(msgFAPI, &req->header, msg_len);
  uint8_t tx_messagebufferFAPI[sizeof(_this->tx_message_buffer)];
  int packedMessageLengthFAPI = -1;
  packedMessageLengthFAPI =
      fapi_nr_p5_message_pack(msgFAPI, msg_len, tx_messagebufferFAPI, sizeof(tx_messagebufferFAPI), &_this->_public.codec_config);

  aerial_send_P5_msg(tx_messagebufferFAPI, packedMessageLengthFAPI, msg);

  return 0;
}

int aerial_nr_config_resp_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_nr_config_response_scf_t *resp)
{
  nfapi_nr_start_request_scf_t req;
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Received NFAPI_CONFIG_RESP idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);
  LOG_I(NFAPI_VNF, "Received CONFIG.response, gNB is ready! \n");

  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_NR_PHY_MSG_TYPE_START_REQUEST;
  req.header.phy_id = resp->header.phy_id;
  nfapi_nr_vnf_start_req(config, p5_idx, &req);
  return 0;
}

int aerial_nr_start_resp_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_nr_start_response_scf_t *resp)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Received NFAPI_START_RESP idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);
  vnf_info *vnf = (vnf_info *)(config->user_data);
  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;

  nfapi_vnf_p7_add_pnf((p7_vnf->config), phy->remote_addr, phy->remote_port, phy->id);
  return 0;
}

int aerial_vendor_ext_cb(nfapi_vnf_config_t *config, int p5_idx, nfapi_p4_p5_message_header_t *msg)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] %s\n", __FUNCTION__);

  switch (msg->message_id) {
    case P5_VENDOR_EXT_RSP: {
      vendor_ext_p5_rsp *rsp = (vendor_ext_p5_rsp *)msg;
      NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] P5_VENDOR_EXT_RSP error_code:%d\n", rsp->error_code);
      // send the start request
      nfapi_pnf_start_request_t req;
      memset(&req, 0, sizeof(req));
      req.header.message_id = NFAPI_PNF_START_REQUEST;
      nfapi_vnf_pnf_start_req(config, p5_idx, &req);
    } break;
  }

  return 0;
}

int aerial_vnf_unpack_vendor_extension_tlv(nfapi_tl_t *tl,
                                           uint8_t **ppReadPackedMessage,
                                           uint8_t *end,
                                           void **ve,
                                           nfapi_p4_p5_codec_config_t *codec)
{
  return -1;
}

int aerial_vnf_pack_vendor_extension_tlv(void *vext, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *codec)
{
  nfapi_tl_t *tlv = (nfapi_tl_t *)vext;

  switch (tlv->tag) {
    case VENDOR_EXT_TLV_2_TAG: {
      vendor_ext_tlv_2 *ve = (vendor_ext_tlv_2 *)tlv;

      if (!push32(ve->dummy, ppWritePackedMsg, end))
        return 0;

      return 1;
    } break;
  }

  return -1;
}

int aerial_vnf_unpack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t *header,
                                             uint8_t **ppReadPackedMessage,
                                             uint8_t *end,
                                             nfapi_p4_p5_codec_config_t *codec)
{
  if (header->message_id == P5_VENDOR_EXT_RSP) {
    vendor_ext_p5_rsp *req = (vendor_ext_p5_rsp *)(header);
    return (!pull16(ppReadPackedMessage, &req->error_code, end));
  }

  return 0;
}

int aerial_vnf_pack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t *header,
                                           uint8_t **ppWritePackedMsg,
                                           uint8_t *end,
                                           nfapi_p4_p5_codec_config_t *codec)
{
  if (header->message_id == P5_VENDOR_EXT_REQ) {
    vendor_ext_p5_req *req = (vendor_ext_p5_req *)(header);
    return (!(push16(req->dummy1, ppWritePackedMsg, end) && push16(req->dummy2, ppWritePackedMsg, end)));
  }

  return 0;
}

nfapi_p4_p5_message_header_t *aerial_vnf_allocate_p4_p5_vendor_ext(uint16_t message_id, uint16_t *msg_size)
{
  if (message_id == P5_VENDOR_EXT_RSP) {
    *msg_size = sizeof(vendor_ext_p5_rsp);
    return (nfapi_p4_p5_message_header_t *)malloc(sizeof(vendor_ext_p5_rsp));
  }

  return 0;
}

void aerial_vnf_deallocate_p4_p5_vendor_ext(nfapi_p4_p5_message_header_t *header)
{
  free(header);
}

void *aerial_vnf_p5_allocate(size_t size)
{
  // return (void*)memory_pool::allocate(size);
  return (void *)malloc(size);
}

void aerial_vnf_p5_deallocate(void *ptr)
{
  // memory_pool::deallocate((uint8_t*)ptr);
  free(ptr);
}

static vnf_info aerial_vnf;
void aerial_configure_nr_fapi_vnf()
{
  // TODO: Implement for FAPI
  // nfapi_setmode(NFAPI_MODE_VNF);
  memset(&aerial_vnf, 0, sizeof(aerial_vnf));
  memset(aerial_vnf.p7_vnfs, 0, sizeof(aerial_vnf.p7_vnfs));
  aerial_vnf.p7_vnfs[0].timing_window = 30;
  aerial_vnf.p7_vnfs[0].periodic_timing_enabled = 0;
  aerial_vnf.p7_vnfs[0].aperiodic_timing_enabled = 0;
  aerial_vnf.p7_vnfs[0].periodic_timing_period = 10;
  aerial_vnf.p7_vnfs[0].config = nfapi_vnf_p7_config_create();
  aerial_vnf.p7_vnfs[0].mac = (mac_t *)malloc(sizeof(mac_t));
  nfapi_vnf_config_t *config = nfapi_vnf_config_create();
  config->malloc = malloc;
  config->free = free;
  config->vnf_ipv4 = 1;
  config->vnf_ipv6 = 0;
  config->pnf_list = 0;
  config->phy_list = 0;

  config->pnf_nr_connection_indication = &aerial_pnf_nr_connection_indication_cb;
  config->pnf_disconnect_indication = &aerial_pnf_disconnection_indication_cb;

  config->pnf_nr_param_resp = &aerial_pnf_nr_param_resp_cb;
  config->pnf_nr_config_resp = &aerial_pnf_nr_config_resp_cb;
  config->pnf_nr_start_resp = &aerial_pnf_nr_start_resp_cb;
  config->nr_param_resp = &aerial_nr_param_resp_cb;
  config->nr_config_resp = &aerial_nr_config_resp_cb;
  config->nr_start_resp = &aerial_nr_start_resp_cb;
  config->vendor_ext = &aerial_vendor_ext_cb;
  config->user_data = &aerial_vnf;
  // To allow custom vendor extentions to be added to nfapi
  config->codec_config.unpack_vendor_extension_tlv = &aerial_vnf_unpack_vendor_extension_tlv;
  config->codec_config.pack_vendor_extension_tlv = &aerial_vnf_pack_vendor_extension_tlv;
  config->codec_config.unpack_p4_p5_vendor_extension = &aerial_vnf_unpack_p4_p5_vendor_extension;
  config->codec_config.pack_p4_p5_vendor_extension = &aerial_vnf_pack_p4_p5_vendor_extension;
  config->allocate_p4_p5_vendor_ext = &aerial_vnf_allocate_p4_p5_vendor_ext;
  config->deallocate_p4_p5_vendor_ext = &aerial_vnf_deallocate_p4_p5_vendor_ext;
  config->codec_config.allocate = &aerial_vnf_allocate;
  config->codec_config.deallocate = &aerial_vnf_deallocate;
  memset(&UL_RCC_INFO, 0, sizeof(UL_RCC_IND_t));
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Creating VNF NFAPI start thread %s\n", __FUNCTION__);
  set_config(config);
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Created VNF NFAPI start thread %s\n", __FUNCTION__);
  nfapi_vnf_pnf_info_t *pnf = (nfapi_vnf_pnf_info_t *)malloc(sizeof(nfapi_vnf_pnf_info_t));
  NFAPI_TRACE(NFAPI_TRACE_INFO, "MALLOC nfapi_vnf_pnf_info_t for pnf_list pnf:%p\n", pnf);
  memset(pnf, 0, sizeof(nfapi_vnf_pnf_info_t));
  pnf->p5_idx = 1;
  pnf->connected = 1;
  // Add needed parameters

  vnf_info *vnf = (vnf_info *)(config->user_data);
  pnf_info *pnf_info = vnf->pnfs;

  for (int i = 0; i < 1; ++i) {
    phy_info phy;
    memset(&phy, 0, sizeof(phy));
    phy.index = 0;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] (PHY:%d) phy_config_idx:%d\n", i, 0);
    nfapi_vnf_allocate_phy(config, 1, &(phy.id));

    for (int j = 0; j < 1; ++j) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] (PHY:%d) (RF%d) %d\n", i, j, 0);
      phy.rfs[0] = 0;
    }

    pnf_info->phys[0] = phy;
  }


  nfapi_vnf_pnf_list_add(config, pnf);

  vnf_p7_info *p7_vnf = vnf->p7_vnfs;

  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "[VNF] pnf start response idx:%d config:%p user_data:%p p7_vnf[config:%p thread_started:%d]\n",
              1,
              config,
              config->user_data,
              vnf->p7_vnfs[0].config,
              vnf->p7_vnfs[0].thread_started);

  if (p7_vnf->thread_started == 0) {
    pthread_t vnf_p7_thread;
    pthread_create(&vnf_p7_thread, NULL, &aerial_vnf_nr_p7_thread_start, p7_vnf);
    p7_vnf->thread_started = 1;
  } else {
    // P7 thread already running.
  }
}
uint8_t aerial_unpack_nr_param_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  return unpack_nr_param_response(ppReadPackedMsg, end, msg, config);
}
uint8_t aerial_unpack_nr_config_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  return unpack_nr_config_response(ppReadPackedMsg, end, msg, config);
}

// monitor the p7 endpoints and the timing loop and
// send indications to mac
int aerial_nfapi_nr_vnf_p7_start(nfapi_vnf_p7_config_t *config)
{
  if (config == 0)
    return -1;

  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;

  // Create p7 receive udp port
  // todo : this needs updating for Ipv6

  NFAPI_TRACE(NFAPI_TRACE_INFO, "Initialising VNF P7 port:%u\n", config->port);


  struct timespec ref_time;
  clock_gettime(CLOCK_MONOTONIC, &ref_time);
  uint8_t setup_done = 0;
  while (vnf_p7->terminate == 0) {
    if (setup_done == 0) {
      struct timespec curr_time;
      clock_gettime(CLOCK_MONOTONIC, &curr_time);
      uint8_t setup_time = curr_time.tv_sec - ref_time.tv_sec;
      if (setup_time > 3) {
        setup_done = 1;
      }
    }

  }
  NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p7 socket\n");
  close(vnf_p7->socket);

  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() returning\n", __FUNCTION__);

  return 0;
}

int fapi_nr_p5_message_pack(void *pMessageBuf, uint32_t messageBufLen, void *pPackedBuf, uint32_t packedBufLen, nfapi_p4_p5_codec_config_t *config){

  nfapi_p4_p5_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *pWritePackedMessage = pPackedBuf;

  uint32_t packedMsgLen;
  //uint16_t packedMsgLen16;

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 Pack supplied pointers are null\n");
    return -1;
  }
  uint8_t *pPackMessageEnd = pPackedBuf + packedBufLen;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint8_t *pPacketBodyField = &pWritePackedMessage[8];
  uint8_t *pPacketBodyFieldStart = &pWritePackedMessage[8];

  pack_nr_p5_message_body(pMessageHeader, &pPacketBodyField, pPackMessageEnd, config);

  // PHY API message header
  push8(1, &pWritePackedMessage, pPackMessageEnd); // Number of messages
  push8(0, &pWritePackedMessage, pPackMessageEnd); // Opaque handle

  // PHY API Message structure
  push16(pMessageHeader->message_id, &pWritePackedMessage, pPackMessageEnd); // Message type ID

  if(1==1) {
    // check for a valid message length
    packedMsgLen = get_packed_msg_len((uintptr_t)pPacketBodyFieldStart, (uintptr_t)pPacketBodyField);
    packedMsgLen-=1;
    if(pMessageHeader->message_id == NFAPI_NR_PHY_MSG_TYPE_START_REQUEST){
      //START.request doesn't have a body, length is 0
      packedMsgLen = 0;
    }else if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message 0x%02x length error %d, buffer supplied %d\n",pMessageHeader->message_id, packedMsgLen, packedBufLen);
      return -1;
    } else {

    }

    // Update the message length in the header
    if(!push32(packedMsgLen, &pPackedLengthField, pPackMessageEnd))
      return -1;

    // return the packed length
    return (packedMsgLen);
  } else {
    // Failed to pack the meassage
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 Failed to pack message\n");
    return -1;
  }

}


int oai_fapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req)
{
  nfapi_vnf_p7_config_t *p7_config = aerial_vnf.p7_vnfs[0].config;

  ul_tti_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ul_tti_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST;

  // int retval = nfapi_vnf_p7_ul_tti_req(p7_config, ul_tti_req);
  int retval = fapi_nr_pack_and_send_p7_message((vnf_p7_t *)p7_config, &ul_tti_req->header);

  if (retval != 0) {
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
  nfapi_vnf_p7_config_t *p7_config = aerial_vnf.p7_vnfs[0].config;
  ul_dci_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ul_dci_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST;
  // int retval = nfapi_vnf_p7_ul_dci_req(p7_config, ul_dci_req);
  int retval = fapi_nr_pack_and_send_p7_message((vnf_p7_t *)p7_config, &ul_dci_req->header);
  if (retval != 0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    ul_dci_req->numPdus = 0;
  }
  return retval;
}

int oai_fapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req)
{
  nfapi_vnf_p7_config_t *p7_config = aerial_vnf.p7_vnfs[0].config;
  tx_data_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  tx_data_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST;
  // LOG_D(PHY, "[VNF] %s() TX_REQ sfn_sf:%d number_of_pdus:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(tx_data_req->SFN),
  // tx_data_req->Number_of_PDUs); int retval = nfapi_vnf_p7_tx_data_req(p7_config, tx_data_req);
  int retval = fapi_nr_pack_and_send_p7_message((vnf_p7_t *)p7_config, &tx_data_req->header);
  if (retval != 0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    tx_data_req->Number_of_PDUs = 0;
  }

  return retval;
}

int oai_fapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req)
{
  nfapi_vnf_p7_config_t *p7_config = aerial_vnf.p7_vnfs[0].config;
  dl_config_req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST;
  dl_config_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  int retval = fapi_nr_pack_and_send_p7_message((vnf_p7_t *)p7_config, &dl_config_req->header);
  dl_config_req->dl_tti_request_body.nPDUs = 0;
  dl_config_req->dl_tti_request_body.nGroup = 0;

  if (retval != 0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  }
  return retval;
}

int oai_fapi_send_end_request(int cell, uint32_t frame, uint32_t slot){
  nfapi_vnf_p7_config_t *p7_config = aerial_vnf.p7_vnfs[0].config;
  nfapi_nr_slot_indication_scf_t *nr_slot_resp = CALLOC(1, sizeof(*nr_slot_resp));
  nr_slot_resp->header.message_id = 0x8F;
  nr_slot_resp->sfn = frame;
  nr_slot_resp->slot = slot;
  int retval = fapi_nr_pack_and_send_p7_message((vnf_p7_t *)p7_config, &nr_slot_resp->header);
  if (retval != 0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  }
  return retval;
}
#endif
