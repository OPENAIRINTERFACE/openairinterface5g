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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nfapi_interface.h"
#include "nfapi_vnf_interface.h"
#include "nfapi.h"
#include "vendor_ext.h"

#include "nfapi_vnf.h"


#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

typedef struct {
  uint8_t enabled;
  uint32_t rx_port;
  uint32_t tx_port;
  char tx_addr[80];
} udp_data;

typedef struct {
  uint16_t index;
  uint16_t id;
  uint8_t rfs[2];
  uint8_t excluded_rfs[2];

  udp_data udp;

  char local_addr[80];
  int local_port;

  char* remote_addr;
  int remote_port;

  uint8_t duplex_mode;
  uint16_t dl_channel_bw_support;
  uint16_t ul_channel_bw_support;
  uint8_t num_dl_layers_supported;
  uint8_t num_ul_layers_supported;
  uint16_t release_supported;
  uint8_t nmm_modes_supported;

  uint8_t dl_ues_per_subframe;
  uint8_t ul_ues_per_subframe;

  uint8_t first_subframe_ind;

  // timing information recevied from the vnf
  uint8_t timing_window;
  uint8_t timing_info_mode;
  uint8_t timing_info_period;

} phy_info;

typedef struct {
  uint16_t index;
  uint16_t band;
  int16_t max_transmit_power;
  int16_t min_transmit_power;
  uint8_t num_antennas_supported;
  uint32_t min_downlink_frequency;
  uint32_t max_downlink_frequency;
  uint32_t max_uplink_frequency;
  uint32_t min_uplink_frequency;
} rf_info;

typedef struct {

  int release;
  phy_info phys[2];
  rf_info rfs[2];

  uint8_t sync_mode;
  uint8_t location_mode;
  uint8_t location_coordinates[6];
  uint32_t dl_config_timing;
  uint32_t ul_config_timing;
  uint32_t tx_timing;
  uint32_t hi_dci0_timing;

  uint16_t max_phys;
  uint16_t max_total_bw;
  uint16_t max_total_dl_layers;
  uint16_t max_total_ul_layers;
  uint8_t shared_bands;
  uint8_t shared_pa;
  int16_t max_total_power;
  uint8_t oui;

  uint8_t wireshark_test_mode;

} pnf_info;

typedef struct mac mac_t;

typedef struct mac {

	void* user_data;

	void (*dl_config_req)(mac_t* mac, nfapi_dl_config_request_t* req);
	void (*ul_config_req)(mac_t* mac, nfapi_ul_config_request_t* req);
	void (*hi_dci0_req)(mac_t* mac, nfapi_hi_dci0_request_t* req);
	void (*tx_req)(mac_t* mac, nfapi_tx_request_t* req);
} mac_t;

typedef struct {

  int local_port;
  char local_addr[80];

  unsigned timing_window;
  unsigned periodic_timing_enabled;
  unsigned aperiodic_timing_enabled;
  unsigned periodic_timing_period;

  // This is not really the right place if we have multiple PHY, 
  // should be part of the phy struct
  udp_data udp;

  uint8_t thread_started;

  nfapi_vnf_p7_config_t* config;

  mac_t* mac;

} vnf_p7_info;

typedef struct {

  uint8_t wireshark_test_mode;
  pnf_info pnfs[2];
  vnf_p7_info p7_vnfs[2];

} vnf_info;

int vnf_pack_vendor_extension_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* codec) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "vnf_pack_vendor_extension_tlv\n");
  nfapi_tl_t* tlv = (nfapi_tl_t*)ve;
  switch(tlv->tag) {
    case VENDOR_EXT_TLV_2_TAG:
      {
        //NFAPI_TRACE(NFAPI_TRACE_INFO, "Packing VENDOR_EXT_TLV_2\n");
        vendor_ext_tlv_2* ve = (vendor_ext_tlv_2*)tlv;
        if(!push32(ve->dummy, ppWritePackedMsg, end))
          return 0;
        return 1;
      }
      break;
  }
  return -1;
}

int vnf_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMessage, uint8_t *end, void** ve, nfapi_p4_p5_codec_config_t* codec) {
  return -1;
}

void install_schedule_handlers(IF_Module_t *if_inst);
extern int single_thread_flag;
extern void init_eNB_afterRU(void);
extern uint16_t sf_ahead;

void oai_create_enb(void) {

  int bodge_counter=0;
  PHY_VARS_eNB *eNB = RC.eNB[0][0];

  printf("[VNF] RC.eNB[0][0]. Mod_id:%d CC_id:%d nb_CC[0]:%d abstraction_flag:%d single_thread_flag:%d td:%p te:%p if_inst:%p\n", eNB->Mod_id, eNB->CC_id, RC.nb_CC[0], eNB->abstraction_flag, eNB->single_thread_flag, eNB->td, eNB->te, eNB->if_inst);

  eNB->Mod_id  = bodge_counter;
  eNB->CC_id   = bodge_counter;
  eNB->abstraction_flag   = 0;
  eNB->single_thread_flag = 0;//single_thread_flag;
  eNB->td                   = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
  eNB->te                   = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;

  RC.nb_CC[bodge_counter] = 1;

  if (eNB->if_inst==0) {
    eNB->if_inst = IF_Module_init(bodge_counter);
  }

  // This will cause phy_config_request to be installed. That will result in RRC configuring the PHY
  // that will result in eNB->configured being set to TRUE.
  // See we need to wait for that to happen otherwise the NFAPI message exchanges won't contain the right parameter values
  if (RC.eNB[0][0]->if_inst==0 || RC.eNB[0][0]->if_inst->PHY_config_req==0 || RC.eNB[0][0]->if_inst->schedule_response==0) {

    printf("RC.eNB[0][0]->if_inst->PHY_config_req is not installed - install it\n");
    install_schedule_handlers(RC.eNB[0][0]->if_inst);
  }

  do {
    printf("%s() Waiting for eNB to become configured (by RRC/PHY) - need to wait otherwise NFAPI messages won't contain correct values\n", __FUNCTION__);
    usleep(50000);
  } while(eNB->configured != 1);

  printf("%s() eNB is now configured\n", __FUNCTION__);
}

void oai_enb_init(void) {

  printf("%s() About to call init_eNB_afterRU()\n", __FUNCTION__);
  init_eNB_afterRU();
}

int pnf_connection_indication_cb(nfapi_vnf_config_t* config, int p5_idx) {

  printf("[VNF] pnf connection indication idx:%d\n", p5_idx);

  oai_create_enb();

  nfapi_pnf_param_request_t req;
  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_PNF_PARAM_REQUEST;
  nfapi_vnf_pnf_param_req(config, p5_idx, &req);
  return 0;
}

int pnf_disconnection_indication_cb(nfapi_vnf_config_t* config, int p5_idx) {
  printf("[VNF] pnf disconnection indication idx:%d\n", p5_idx);

  vnf_info* vnf = (vnf_info*)(config->user_data);

  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;

  vnf_p7_info* p7_vnf = vnf->p7_vnfs;
  nfapi_vnf_p7_del_pnf((p7_vnf->config), phy->id);

  return 0;
}

int pnf_param_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_response_t* resp) {

  printf("[VNF] pnf param response idx:%d error:%d\n", p5_idx, resp->error_code);

  vnf_info* vnf = (vnf_info*)(config->user_data);

  pnf_info *pnf = vnf->pnfs;

  for(int i = 0; i < resp->pnf_phy.number_of_phys; ++i)
  {
    phy_info phy;
    phy.index = resp->pnf_phy.phy[i].phy_config_index;

    printf("[VNF] (PHY:%d) phy_config_idx:%d\n", i, resp->pnf_phy.phy[i].phy_config_index);

    nfapi_vnf_allocate_phy(config, p5_idx, &(phy.id));

    for(int j = 0; j < resp->pnf_phy.phy[i].number_of_rfs; ++j)
    {
      printf("[VNF] (PHY:%d) (RF%d) %d\n", i, j, resp->pnf_phy.phy[i].rf_config[j].rf_config_index);
      phy.rfs[0] = resp->pnf_phy.phy[i].rf_config[j].rf_config_index;
    }

    pnf->phys[0] = phy;
  }

  for(int i = 0; i < resp->pnf_rf.number_of_rfs; ++i) {
    rf_info rf;
    rf.index = resp->pnf_rf.rf[i].rf_config_index;

    printf("[VNF] (RF:%d) rf_config_idx:%d\n", i, resp->pnf_rf.rf[i].rf_config_index);

    pnf->rfs[0] = rf;
  }

  nfapi_pnf_config_request_t req;
  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_PNF_CONFIG_REQUEST;

  req.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
  req.pnf_phy_rf_config.number_phy_rf_config_info = 2; // DJP pnf.phys.size();
  printf("DJP:Hard coded num phy rf to 2\n");

  for(unsigned i = 0; i < 2; ++i) {
    req.pnf_phy_rf_config.phy_rf_config[i].phy_id = pnf->phys[i].id;
    req.pnf_phy_rf_config.phy_rf_config[i].phy_config_index = pnf->phys[i].index;
    req.pnf_phy_rf_config.phy_rf_config[i].rf_config_index = pnf->phys[i].rfs[0];
  }

  nfapi_vnf_pnf_config_req(config, p5_idx, &req);

  return 0;
}

int pnf_config_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_response_t* resp) {

  printf("[VNF] pnf config response idx:%d resp[header[phy_id:%u message_id:%02x message_length:%u]]\n", p5_idx, resp->header.phy_id, resp->header.message_id, resp->header.message_length);

  if(1) {
      nfapi_pnf_start_request_t req;
      memset(&req, 0, sizeof(req));
      req.header.phy_id = resp->header.phy_id;
      req.header.message_id = NFAPI_PNF_START_REQUEST;
      nfapi_vnf_pnf_start_req(config, p5_idx, &req);
  } else {
    // Rather than send the pnf_start_request we will demonstrate
    // sending a vendor extention message. The start request will be
    // send when the vendor extension response is received 

    //vnf_info* vnf = (vnf_info*)(config->user_data);
    vendor_ext_p5_req req;
    memset(&req, 0, sizeof(req));
    req.header.message_id = P5_VENDOR_EXT_REQ;
    req.dummy1 = 45;
    req.dummy2 = 1977;
    nfapi_vnf_vendor_extension(config, p5_idx, &req.header);
  }
  return 0;
}

int wake_eNB_rxtx(PHY_VARS_eNB *eNB, uint16_t sfn, uint16_t sf) {

  eNB_proc_t *proc=&eNB->proc;

  eNB_rxtx_proc_t *proc_rxtx=&proc->proc_rxtx[sf&1];

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  //printf("%s(eNB:%p, sfn:%d, sf:%d)\n", __FUNCTION__, eNB, sfn, sf);

  //int i;
  struct timespec wait;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

#if 0
  /* accept some delay in processing - up to 5ms */
  for (i = 0; i < 10 && proc_rxtx->instance_cnt_rxtx == 0; i++) {
    LOG_W( PHY,"[eNB] sfn/sf:%d:%d proc_rxtx[%d]:TXsfn:%d/%d eNB RXn-TXnp4 thread busy!! (cnt_rxtx %i)\n", sfn, sf, sf&1, proc_rxtx->frame_tx, proc_rxtx->subframe_tx, proc_rxtx->instance_cnt_rxtx);
    usleep(500);
  }
  if (proc_rxtx->instance_cnt_rxtx == 0) {
    exit_fun( "TX thread busy" );
    return(-1);
  }
#endif

  // wake up TX for subframe n+sf_ahead
  // lock the TX mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&proc_rxtx->mutex_rxtx,&wait) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB RXTX thread %d (IC %d)\n", proc_rxtx->subframe_rx&1,proc_rxtx->instance_cnt_rxtx );
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }

  {
    static uint16_t old_sf = 0;
    static uint16_t old_sfn = 0;

    proc->subframe_rx = old_sf;
    proc->frame_rx = old_sfn;

    // Try to be 1 frame back
    old_sf = sf;
    old_sfn = sfn;

    if (old_sf == 0 && old_sfn % 100==0) LOG_W( PHY,"[eNB] sfn/sf:%d%d old_sfn/sf:%d%d proc[rx:%d%d]\n", sfn, sf, old_sfn, old_sf, proc->frame_rx, proc->subframe_rx);
  }

  ++proc_rxtx->instance_cnt_rxtx;

  //LOG_D( PHY,"[VNF-subframe_ind] sfn/sf:%d:%d proc[frame_rx:%d subframe_rx:%d] proc_rxtx->instance_cnt_rxtx:%d \n", sfn, sf, proc->frame_rx, proc->subframe_rx, proc_rxtx->instance_cnt_rxtx);

  // We have just received and processed the common part of a subframe, say n.
  // TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired
  // transmitted timestamp of the next TX slot (first).
  // The last (TS_rx mod samples_per_frame) was n*samples_per_tti,
  // we want to generate subframe (n+N), so TS_tx = TX_rx+N*samples_per_tti,
  // and proc->subframe_tx = proc->subframe_rx+sf_ahead
  proc_rxtx->timestamp_tx = proc->timestamp_rx + (sf_ahead*fp->samples_per_tti);
  proc_rxtx->frame_rx     = proc->frame_rx;
  proc_rxtx->subframe_rx  = proc->subframe_rx;
  proc_rxtx->frame_tx     = (proc_rxtx->subframe_rx > (9-sf_ahead)) ? (proc_rxtx->frame_rx+1)&1023 : proc_rxtx->frame_rx;
  proc_rxtx->subframe_tx  = (proc_rxtx->subframe_rx + sf_ahead)%10;

  //LOG_D(PHY, "sfn/sf:%d%d proc[rx:%d%d] proc_rxtx[instance_cnt_rxtx:%d rx:%d%d] About to wake rxtx thread\n\n", sfn, sf, proc->frame_rx, proc->subframe_rx, proc_rxtx->instance_cnt_rxtx, proc_rxtx->frame_rx, proc_rxtx->subframe_rx);

  // the thread can now be woken up
  if (pthread_cond_signal(&proc_rxtx->cond_rxtx) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB RXn-TXnp4 thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }

  //LOG_D(PHY,"%s() About to attempt pthread_mutex_unlock\n", __FUNCTION__);
  pthread_mutex_unlock( &proc_rxtx->mutex_rxtx );
  //LOG_D(PHY,"%s() UNLOCKED pthread_mutex_unlock\n", __FUNCTION__);

  return(0);
}

extern pthread_cond_t nfapi_sync_cond;
extern pthread_mutex_t nfapi_sync_mutex;
extern int nfapi_sync_var;

int phy_sync_indication(struct nfapi_vnf_p7_config* config, uint8_t sync) {

  printf("[VNF] SYNC %s\n", sync==1 ? "ACHIEVED" : "LOST");
  
  if (sync==1 && nfapi_sync_var!=0) {

    printf("[VNF] Signal to OAI main code that it can go\n");
    pthread_mutex_lock(&nfapi_sync_mutex);
    nfapi_sync_var=0;
    pthread_cond_broadcast(&nfapi_sync_cond);
    pthread_mutex_unlock(&nfapi_sync_mutex);
  }

  return(0);
}

int phy_subframe_indication(struct nfapi_vnf_p7_config* config, uint16_t phy_id, uint16_t sfn_sf) {

  static uint8_t first_time = 1;
  if (first_time) {
    printf("[VNF] subframe indication %d\n", NFAPI_SFNSF2DEC(sfn_sf));
    first_time = 0;
  }

  if (RC.eNB && RC.eNB[0][0]->configured) {
    uint16_t sfn = NFAPI_SFNSF2SFN(sfn_sf);
    uint16_t sf = NFAPI_SFNSF2SF(sfn_sf);

    //LOG_D(PHY,"[VNF] subframe indication sfn_sf:%d sfn:%d sf:%d\n", sfn_sf, sfn, sf);

    wake_eNB_rxtx(RC.eNB[0][0], sfn, sf);
  } else {
    printf("[VNF] %s() RC.eNB:%p\n", __FUNCTION__, RC.eNB);
    if (RC.eNB) printf("RC.eNB[0][0]->configured:%d\n", RC.eNB[0][0]->configured);
  }

  return 0;
}

int phy_rach_indication(struct nfapi_vnf_p7_config* config, nfapi_rach_indication_t* ind) {

  LOG_D(MAC, "%s() NFAPI SFN/SF:%d number_of_preambles:%u\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->rach_indication_body.number_of_preambles);

  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];

  printf("[VNF] RACH_IND eNB:%p sfn_sf:%d number_of_preambles:%d\n", eNB, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->rach_indication_body.number_of_preambles);

  pthread_mutex_lock(&eNB->UL_INFO_mutex);

  eNB->UL_INFO.rach_ind = *ind;
  eNB->UL_INFO.rach_ind.rach_indication_body.preamble_list = eNB->preamble_list;

  for (int i=0;i<ind->rach_indication_body.number_of_preambles;i++) {
    if (ind->rach_indication_body.preamble_list[i].preamble_rel8.tl.tag == NFAPI_PREAMBLE_REL8_TAG) {

      printf("preamble[%d]: rnti:%02x preamble:%d timing_advance:%d\n", 
          i,
          ind->rach_indication_body.preamble_list[i].preamble_rel8.rnti,
          ind->rach_indication_body.preamble_list[i].preamble_rel8.preamble,
          ind->rach_indication_body.preamble_list[i].preamble_rel8.timing_advance
          );
    }

    if(ind->rach_indication_body.preamble_list[i].preamble_rel13.tl.tag == NFAPI_PREAMBLE_REL13_TAG) {
      printf("RACH PREAMBLE REL13 present\n");
    }

    eNB->preamble_list[i] = ind->rach_indication_body.preamble_list[i];
  }
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_rach_ind(p7_vnf->mac, ind);
  return 1;
}

int phy_harq_indication(struct nfapi_vnf_p7_config* config, nfapi_harq_indication_t* ind) {

  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];

  LOG_D(MAC, "%s() NFAPI SFN/SF:%d number_of_harqs:%u\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->harq_indication_body.number_of_harqs);

  pthread_mutex_lock(&eNB->UL_INFO_mutex);

  eNB->UL_INFO.harq_ind = *ind;
  eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list = eNB->harq_pdu_list;

  for (int i=0; i<ind->harq_indication_body.number_of_harqs; i++) {
    memcpy(&eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list[i], &ind->harq_indication_body.harq_pdu_list[i], sizeof(eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list[i]));
  }

  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_harq_ind(p7_vnf->mac, ind);

  return 1;
}

int phy_crc_indication(struct nfapi_vnf_p7_config* config, nfapi_crc_indication_t* ind) {

  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];

  pthread_mutex_lock(&eNB->UL_INFO_mutex);

  eNB->UL_INFO.crc_ind = *ind;

  nfapi_crc_indication_t *dest_ind = &eNB->UL_INFO.crc_ind;
  nfapi_crc_indication_pdu_t *dest_pdu_list = eNB->crc_pdu_list;

  *dest_ind = *ind;
  dest_ind->crc_indication_body.crc_pdu_list = dest_pdu_list;

  if (ind->crc_indication_body.number_of_crcs==0)
    LOG_D(MAC, "%s() NFAPI SFN/SF:%d IND:number_of_crcs:%u UL_INFO:crcs:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->crc_indication_body.number_of_crcs, eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs);

  for (int i=0; i<ind->crc_indication_body.number_of_crcs; i++) {
    memcpy(&dest_ind->crc_indication_body.crc_pdu_list[i], &ind->crc_indication_body.crc_pdu_list[i], sizeof(ind->crc_indication_body.crc_pdu_list[0]));

    LOG_D(MAC, "%s() NFAPI SFN/SF:%d CRC_IND:number_of_crcs:%u UL_INFO:crcs:%d PDU[%d] rnti:%04x UL_INFO:rnti:%04x\n", 
        __FUNCTION__,
        NFAPI_SFNSF2DEC(ind->sfn_sf), ind->crc_indication_body.number_of_crcs, eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs,
        i, 
        ind->crc_indication_body.crc_pdu_list[i].rx_ue_information.rnti, 
        eNB->UL_INFO.crc_ind.crc_indication_body.crc_pdu_list[i].rx_ue_information.rnti);
  }

  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_crc_ind(p7_vnf->mac, ind);

  return 1;
}

int phy_rx_indication(struct nfapi_vnf_p7_config* config, nfapi_rx_indication_t* ind) {

  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];

  if (ind->rx_indication_body.number_of_pdus==0) {
    LOG_D(MAC, "%s() NFAPI SFN/SF:%d number_of_pdus:%u\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->rx_indication_body.number_of_pdus);
  }

  pthread_mutex_lock(&eNB->UL_INFO_mutex);

  nfapi_rx_indication_t *dest_ind = &eNB->UL_INFO.rx_ind;
  nfapi_rx_indication_pdu_t *dest_pdu_list = eNB->rx_pdu_list;

  *dest_ind = *ind;
  dest_ind->rx_indication_body.rx_pdu_list = dest_pdu_list;

  for(int i=0; i<ind->rx_indication_body.number_of_pdus; i++) {
    nfapi_rx_indication_pdu_t *dest_pdu = &dest_ind->rx_indication_body.rx_pdu_list[i];
    nfapi_rx_indication_pdu_t *src_pdu = &ind->rx_indication_body.rx_pdu_list[i];

    memcpy(dest_pdu, src_pdu, sizeof(*src_pdu));

    // DJP - TODO FIXME - intentional memory leak
    dest_pdu->data = malloc(dest_pdu->rx_indication_rel8.length);

    memcpy(dest_pdu->data, src_pdu->data, dest_pdu->rx_indication_rel8.length);

    LOG_D(PHY, "%s() NFAPI SFN/SF:%d PDUs:%d [PDU:%d] handle:%d rnti:%04x length:%d offset:%d ul_cqi:%d ta:%d data:%p\n",
        __FUNCTION__,
        NFAPI_SFNSF2DEC(ind->sfn_sf), ind->rx_indication_body.number_of_pdus, i,
        dest_pdu->rx_ue_information.handle,
        dest_pdu->rx_ue_information.rnti,
        dest_pdu->rx_indication_rel8.length,
        dest_pdu->rx_indication_rel8.offset,
        dest_pdu->rx_indication_rel8.ul_cqi,
        dest_pdu->rx_indication_rel8.timing_advance,
        dest_pdu->data
        );
  }

  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_rx_ind(p7_vnf->mac, ind);
  return 1;
}
int phy_srs_indication(struct nfapi_vnf_p7_config* config, nfapi_srs_indication_t* ind) {
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_srs_ind(p7_vnf->mac, ind);
  return 1;
}

int phy_sr_indication(struct nfapi_vnf_p7_config* config, nfapi_sr_indication_t* ind) {

  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];

  LOG_D(MAC, "%s() NFAPI SFN/SF:%d srs:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->sr_indication_body.number_of_srs);

  pthread_mutex_lock(&eNB->UL_INFO_mutex);

  nfapi_sr_indication_t *dest_ind = &eNB->UL_INFO.sr_ind;
  nfapi_sr_indication_pdu_t *dest_pdu_list = eNB->sr_pdu_list;

  *dest_ind = *ind;
  dest_ind->sr_indication_body.sr_pdu_list = dest_pdu_list;

  LOG_D(MAC,"%s() eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs:%d\n", __FUNCTION__, eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs);

  for (int i=0;i<eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs;i++) {
    nfapi_sr_indication_pdu_t *dest_pdu = &dest_ind->sr_indication_body.sr_pdu_list[i];
    nfapi_sr_indication_pdu_t *src_pdu = &ind->sr_indication_body.sr_pdu_list[i];

    LOG_D(MAC, "SR_IND[PDU:%d][rnti:%x cqi:%d channel:%d]\n", i, src_pdu->rx_ue_information.rnti, src_pdu->ul_cqi_information.ul_cqi, src_pdu->ul_cqi_information.channel);

    memcpy(dest_pdu, src_pdu, sizeof(*src_pdu));
  }

  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_sr_ind(p7_vnf->mac, ind);

  return 1;
}

int phy_cqi_indication(struct nfapi_vnf_p7_config* config, nfapi_cqi_indication_t* ind) {

  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_cqi_ind(p7_vnf->mac, ind);
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];

  LOG_D(MAC, "%s() NFAPI SFN/SF:%d number_of_cqis:%u\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->cqi_indication_body.number_of_cqis);

  pthread_mutex_lock(&eNB->UL_INFO_mutex);

  eNB->UL_INFO.cqi_ind = ind->cqi_indication_body;

  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  return 1;
}

int phy_lbt_dl_indication(struct nfapi_vnf_p7_config* config, nfapi_lbt_dl_indication_t* ind) {
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_lbt_dl_ind(p7_vnf->mac, ind);
  return 1;
}

int phy_nb_harq_indication(struct nfapi_vnf_p7_config* config, nfapi_nb_harq_indication_t* ind) {
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_nb_harq_ind(p7_vnf->mac, ind);
  return 1;
}

int phy_nrach_indication(struct nfapi_vnf_p7_config* config, nfapi_nrach_indication_t* ind) {
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  //mac_nrach_ind(p7_vnf->mac, ind);
  return 1;
}

void* vnf_allocate(size_t size) {
  //return (void*)memory_pool::allocate(size);
  return (void*)malloc(size);
}

void vnf_deallocate(void* ptr) {
  //memory_pool::deallocate((uint8_t*)ptr);
  free(ptr);
}

extern void nfapi_log(char *file, char *func, int line, int comp, int level, const char* format, va_list args);

void vnf_trace(nfapi_trace_level_t nfapi_level, const char* message, ...) {

  va_list args;
  int oai_level;

  if (nfapi_level==NFAPI_TRACE_ERROR)
  {
    oai_level = LOG_ERR;
  }
  else if (nfapi_level==NFAPI_TRACE_WARN)
  {
    oai_level = LOG_WARNING;
  }
  else if (nfapi_level==NFAPI_TRACE_NOTE)
  {
    oai_level = LOG_INFO;
  }
  else if (nfapi_level==NFAPI_TRACE_INFO)
  {
    oai_level = LOG_INFO;
  }
  else
  {
    oai_level = LOG_INFO;
  }

  va_start(args, message);
  nfapi_log("FILE>", "FUNC", 999, PHY, oai_level, message, args);
  va_end(args);
}

int phy_vendor_ext(struct nfapi_vnf_p7_config* config, nfapi_p7_message_header_t* msg) {

  if(msg->message_id == P7_VENDOR_EXT_IND)
  {
    //vendor_ext_p7_ind* ind = (vendor_ext_p7_ind*)msg;
    //printf("[VNF] vendor_ext (error_code:%d)\n", ind->error_code);
  }
  else
  {
    printf("[VNF] unknown %02x\n", msg->message_id);
  }
  return 0;
}

nfapi_p7_message_header_t* phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t* msg_size) {
  if(message_id == P7_VENDOR_EXT_IND)
  {
    *msg_size = sizeof(vendor_ext_p7_ind);
    return (nfapi_p7_message_header_t*)malloc(sizeof(vendor_ext_p7_ind));
  }
  return 0;
}

void phy_deallocate_p7_vendor_ext(nfapi_p7_message_header_t* header) {
  free(header);
}

int phy_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMessage, uint8_t *end, void** ve, nfapi_p7_codec_config_t* codec) {

  (void)tl;
  (void)ppReadPackedMessage;
  (void)ve;
  return -1;
}

int phy_pack_vendor_extension_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t* codec) {

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "phy_pack_vendor_extension_tlv\n");

  nfapi_tl_t* tlv = (nfapi_tl_t*)ve;
  switch(tlv->tag) {
    case VENDOR_EXT_TLV_1_TAG:
      {
        //NFAPI_TRACE(NFAPI_TRACE_INFO, "Packing VENDOR_EXT_TLV_1\n");
        vendor_ext_tlv_1* ve = (vendor_ext_tlv_1*)tlv;
        if(!push32(ve->dummy, ppWritePackedMsg, end))
          return 0;
        return 1;
      }
      break;
    default:
      return -1;
      break;
  }
}

int phy_unpack_p7_vendor_extension(nfapi_p7_message_header_t* header, uint8_t** ppReadPackedMessage, uint8_t *end, nfapi_p7_codec_config_t* config) {

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P7_VENDOR_EXT_IND) {
    vendor_ext_p7_ind* req = (vendor_ext_p7_ind*)(header);
    if(!pull16(ppReadPackedMessage, &req->error_code, end))
      return 0;
  }

  return 1;
}

int phy_pack_p7_vendor_extension(nfapi_p7_message_header_t* header, uint8_t** ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t* config) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P7_VENDOR_EXT_REQ) {
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
    vendor_ext_p7_req* req = (vendor_ext_p7_req*)(header);
    if(!(push16(req->dummy1, ppWritePackedMsg, end) &&
          push16(req->dummy2, ppWritePackedMsg, end)))
      return 0;
  }
  return 1;
}

int vnf_pack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t* header, uint8_t** ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* codec) {

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P5_VENDOR_EXT_REQ) {
    vendor_ext_p5_req* req = (vendor_ext_p5_req*)(header);
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s %d %d\n", __FUNCTION__, req->dummy1, req->dummy2);
    return (!(push16(req->dummy1, ppWritePackedMsg, end) &&
          push16(req->dummy2, ppWritePackedMsg, end)));
  }
  return 0;
}

static pthread_t vnf_start_pthread;
static pthread_t vnf_p7_start_pthread;

void* vnf_p7_start_thread(void *ptr) {

  printf("%s()\n", __FUNCTION__);

  pthread_setname_np(pthread_self(), "VNF_P7");

  nfapi_vnf_p7_config_t* config = (nfapi_vnf_p7_config_t*)ptr;

  nfapi_vnf_p7_start(config);

  return config;
}

void set_thread_priority(int priority);

void* vnf_p7_thread_start(void* ptr) {

  set_thread_priority(79);

  vnf_p7_info* p7_vnf = (vnf_p7_info*)ptr;

  p7_vnf->config->port = p7_vnf->local_port;
  p7_vnf->config->sync_indication = &phy_sync_indication;
  p7_vnf->config->subframe_indication = &phy_subframe_indication;
  p7_vnf->config->harq_indication = &phy_harq_indication;
  p7_vnf->config->crc_indication = &phy_crc_indication;
  p7_vnf->config->rx_indication = &phy_rx_indication;
  p7_vnf->config->rach_indication = &phy_rach_indication;
  p7_vnf->config->srs_indication = &phy_srs_indication;
  p7_vnf->config->sr_indication = &phy_sr_indication;
  p7_vnf->config->cqi_indication = &phy_cqi_indication;
  p7_vnf->config->lbt_dl_indication = &phy_lbt_dl_indication;
  p7_vnf->config->nb_harq_indication = &phy_nb_harq_indication;
  p7_vnf->config->nrach_indication = &phy_nrach_indication;
  p7_vnf->config->malloc = &vnf_allocate;
  p7_vnf->config->free = &vnf_deallocate;

  p7_vnf->config->trace = &vnf_trace;

  p7_vnf->config->vendor_ext = &phy_vendor_ext;
  p7_vnf->config->user_data = p7_vnf;

  p7_vnf->mac->user_data = p7_vnf;

  p7_vnf->config->codec_config.unpack_p7_vendor_extension = &phy_unpack_p7_vendor_extension;
  p7_vnf->config->codec_config.pack_p7_vendor_extension = &phy_pack_p7_vendor_extension;
  p7_vnf->config->codec_config.unpack_vendor_extension_tlv = &phy_unpack_vendor_extension_tlv;
  p7_vnf->config->codec_config.pack_vendor_extension_tlv = &phy_pack_vendor_extension_tlv;
  p7_vnf->config->codec_config.allocate = &vnf_allocate;
  p7_vnf->config->codec_config.deallocate = &vnf_deallocate;

  p7_vnf->config->allocate_p7_vendor_ext = &phy_allocate_p7_vendor_ext;
  p7_vnf->config->deallocate_p7_vendor_ext = &phy_deallocate_p7_vendor_ext;

  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Creating VNF NFAPI start thread %s\n", __FUNCTION__);
  pthread_create(&vnf_p7_start_pthread, NULL, &vnf_p7_start_thread, p7_vnf->config);

  return 0;
}

int pnf_start_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_response_t* resp) {

  vnf_info* vnf = (vnf_info*)(config->user_data);
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;
  pnf_info *pnf = vnf->pnfs;
  nfapi_param_request_t req;

  printf("[VNF] pnf start response idx:%d config:%p user_data:%p p7_vnf[config:%p thread_started:%d]\n", p5_idx, config, config->user_data, vnf->p7_vnfs[0].config, vnf->p7_vnfs[0].thread_started);

  if(p7_vnf->thread_started == 0) {

    pthread_t vnf_p7_thread;
    pthread_create(&vnf_p7_thread, NULL, &vnf_p7_thread_start, p7_vnf);
    p7_vnf->thread_started = 1;
  }
  else
  {
    // P7 thread already running. 
  }

  // start all the phys in the pnf.
  printf("[VNF] Sending NFAPI_PARAM_REQUEST phy_id:%d\n", pnf->phys[0].id);

  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_PARAM_REQUEST;
  req.header.phy_id = pnf->phys[0].id;

  nfapi_vnf_param_req(config, p5_idx, &req);

  return 0;
}

extern uint32_t to_earfcn(int eutra_bandP,uint32_t dl_CarrierFreq,uint32_t bw);

int param_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_response_t* resp) {

  printf("[VNF] Received NFAPI_PARAM_RESP idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);

  vnf_info* vnf = (vnf_info*)(config->user_data);
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;

  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;
  struct sockaddr_in pnf_p7_sockaddr;
  nfapi_config_request_t *req = &RC.mac[0]->config[0];

  phy->remote_port = resp->nfapi_config.p7_pnf_port.value;

  memcpy(&pnf_p7_sockaddr.sin_addr.s_addr, &(resp->nfapi_config.p7_pnf_address_ipv4.address[0]), 4);

  phy->remote_addr = inet_ntoa(pnf_p7_sockaddr.sin_addr);

  // for now just 1

  printf("[VNF] %d.%d pnf p7 %s:%d timing %d %d %d %d\n", p5_idx, phy->id, phy->remote_addr, phy->remote_port, p7_vnf->timing_window, p7_vnf->periodic_timing_period, p7_vnf->aperiodic_timing_enabled, p7_vnf->periodic_timing_period);

  req->header.message_id = NFAPI_CONFIG_REQUEST;
  req->header.phy_id = phy->id;

  printf("[VNF] Send NFAPI_CONFIG_REQUEST\n");

  req->nfapi_config.p7_vnf_port.tl.tag = NFAPI_NFAPI_P7_VNF_PORT_TAG;
  req->nfapi_config.p7_vnf_port.value = p7_vnf->local_port;
  req->num_tlv++;

  printf("[VNF] DJP local_port:%d\n", p7_vnf->local_port);

  req->nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
  struct sockaddr_in vnf_p7_sockaddr;
  vnf_p7_sockaddr.sin_addr.s_addr = inet_addr(p7_vnf->local_addr);
  memcpy(&(req->nfapi_config.p7_vnf_address_ipv4.address[0]), &vnf_p7_sockaddr.sin_addr.s_addr, 4);
  req->num_tlv++;
  printf("[VNF] DJP local_addr:%s\n", p7_vnf->local_addr);

  req->nfapi_config.timing_window.tl.tag = NFAPI_NFAPI_TIMING_WINDOW_TAG;
  req->nfapi_config.timing_window.value = p7_vnf->timing_window;
  printf("[VNF] Timing window:%d\n", p7_vnf->timing_window);
  req->num_tlv++;

  if(p7_vnf->periodic_timing_enabled || p7_vnf->aperiodic_timing_enabled) {

    req->nfapi_config.timing_info_mode.tl.tag = NFAPI_NFAPI_TIMING_INFO_MODE_TAG;
    req->nfapi_config.timing_info_mode.value = (p7_vnf->aperiodic_timing_enabled << 1) | (p7_vnf->periodic_timing_enabled);
    req->num_tlv++;

    if(p7_vnf->periodic_timing_enabled) {

      req->nfapi_config.timing_info_period.tl.tag = NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG;
      req->nfapi_config.timing_info_period.value = p7_vnf->periodic_timing_period;
      req->num_tlv++;
    }
  }

  vendor_ext_tlv_2 ve2;
  memset(&ve2, 0, sizeof(ve2));
  ve2.tl.tag = VENDOR_EXT_TLV_2_TAG;
  ve2.dummy = 2016;
  req->vendor_extension = &ve2.tl;

  nfapi_vnf_config_req(config, p5_idx, req);
  printf("[VNF] Sent NFAPI_CONFIG_REQ num_tlv:%u\n",req->num_tlv);

  return 0;
}

int config_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_response_t* resp) {

  nfapi_start_request_t req;

  printf("[VNF] Received NFAPI_CONFIG_RESP idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);

  printf("[VNF] Calling oai_enb_init()\n");
  oai_enb_init();

  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_START_REQUEST;
  req.header.phy_id = resp->header.phy_id;
  nfapi_vnf_start_req(config, p5_idx, &req);

  printf("[VNF] Send NFAPI_START_REQUEST idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);

  return 0;
}

int start_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_response_t* resp) {

  printf("[VNF] Received NFAPI_START_RESP idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);

  vnf_info* vnf = (vnf_info*)(config->user_data);

  pnf_info *pnf = vnf->pnfs;
  phy_info *phy = pnf->phys;
  vnf_p7_info *p7_vnf = vnf->p7_vnfs;
  nfapi_vnf_p7_add_pnf((p7_vnf->config), phy->remote_addr, phy->remote_port, phy->id);

  return 0;
}

int vendor_ext_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg) {

  printf("[VNF] %s\n", __FUNCTION__);

  switch(msg->message_id) {
    case P5_VENDOR_EXT_RSP:
      {
        vendor_ext_p5_rsp* rsp = (vendor_ext_p5_rsp*)msg;
        printf("[VNF] P5_VENDOR_EXT_RSP error_code:%d\n", rsp->error_code);

        // send the start request

        nfapi_pnf_start_request_t req;
        memset(&req, 0, sizeof(req));
        req.header.message_id = NFAPI_PNF_START_REQUEST;
        nfapi_vnf_pnf_start_req(config, p5_idx, &req);
      }
      break;
  }

  return 0;
}

int vnf_unpack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t* header, uint8_t** ppReadPackedMessage, uint8_t *end, nfapi_p4_p5_codec_config_t* codec) {

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P5_VENDOR_EXT_RSP)
  {
    vendor_ext_p5_rsp* req = (vendor_ext_p5_rsp*)(header);
    return(!pull16(ppReadPackedMessage, &req->error_code, end));
  }
  return 0;
}

nfapi_p4_p5_message_header_t* vnf_allocate_p4_p5_vendor_ext(uint16_t message_id, uint16_t* msg_size) {

  if(message_id == P5_VENDOR_EXT_RSP) {
    *msg_size = sizeof(vendor_ext_p5_rsp);
    return (nfapi_p4_p5_message_header_t*)malloc(sizeof(vendor_ext_p5_rsp));
  }
  return 0;
}

void vnf_deallocate_p4_p5_vendor_ext(nfapi_p4_p5_message_header_t* header) {
  free(header);
}

nfapi_vnf_config_t *config = 0;

void vnf_start_thread(void* ptr) {

  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] VNF NFAPI thread - nfapi_vnf_start()%s\n", __FUNCTION__);

  pthread_setname_np(pthread_self(), "VNF");

  config = (nfapi_vnf_config_t*)ptr;

  nfapi_vnf_start(config);
}

static vnf_info vnf;
extern uint8_t nfapi_mode;
/*------------------------------------------------------------------------------*/
void configure_nfapi_vnf(char *vnf_addr, int vnf_p5_port)
{
  nfapi_mode = 2;

  memset(&vnf, 0, sizeof(vnf));

  memset(vnf.p7_vnfs, 0, sizeof(vnf.p7_vnfs));

  vnf.p7_vnfs[0].timing_window = 32;
  vnf.p7_vnfs[0].periodic_timing_enabled = 1;
  vnf.p7_vnfs[0].aperiodic_timing_enabled = 0;
  vnf.p7_vnfs[0].periodic_timing_period = 10;

  vnf.p7_vnfs[0].config = nfapi_vnf_p7_config_create();
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] %s() vnf.p7_vnfs[0].config:%p VNF ADDRESS:%s:%d\n", __FUNCTION__, vnf.p7_vnfs[0].config, vnf_addr, vnf_p5_port);

  strcpy(vnf.p7_vnfs[0].local_addr, vnf_addr);
  vnf.p7_vnfs[0].local_port = 50001;
  vnf.p7_vnfs[0].mac = (mac_t*)malloc(sizeof(mac_t));

  nfapi_vnf_config_t* config = nfapi_vnf_config_create();

  config->malloc = malloc;
  config->free = free;
  config->trace = &vnf_trace;

  config->vnf_p5_port = vnf_p5_port;
  config->vnf_ipv4 = 1;
  config->vnf_ipv6 = 0;

  config->pnf_list = 0;
  config->phy_list = 0;

  config->pnf_connection_indication = &pnf_connection_indication_cb;
  config->pnf_disconnect_indication = &pnf_disconnection_indication_cb;
  config->pnf_param_resp = &pnf_param_resp_cb;
  config->pnf_config_resp = &pnf_config_resp_cb;
  config->pnf_start_resp = &pnf_start_resp_cb;
  config->param_resp = &param_resp_cb;
  config->config_resp = &config_resp_cb;
  config->start_resp = &start_resp_cb;
  config->vendor_ext = &vendor_ext_cb;

  config->user_data = &vnf;

  // To allow custom vendor extentions to be added to nfapi
  config->codec_config.unpack_vendor_extension_tlv = &vnf_unpack_vendor_extension_tlv;
  config->codec_config.pack_vendor_extension_tlv = &vnf_pack_vendor_extension_tlv;

  config->codec_config.unpack_p4_p5_vendor_extension = &vnf_unpack_p4_p5_vendor_extension;
  config->codec_config.pack_p4_p5_vendor_extension = &vnf_pack_p4_p5_vendor_extension;
  config->allocate_p4_p5_vendor_ext = &vnf_allocate_p4_p5_vendor_ext;
  config->deallocate_p4_p5_vendor_ext = &vnf_deallocate_p4_p5_vendor_ext;
  config->codec_config.allocate = &vnf_allocate;
  config->codec_config.deallocate = &vnf_deallocate;

  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Creating VNF NFAPI start thread %s\n", __FUNCTION__);
  pthread_create(&vnf_start_pthread, NULL, (void*)&vnf_start_thread, config);
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Created VNF NFAPI start thread %s\n", __FUNCTION__);
}

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req)
{
  nfapi_vnf_p7_config_t *p7_config = vnf.p7_vnfs[0].config;

  dl_config_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

  int retval = nfapi_vnf_p7_dl_config_req(p7_config, dl_config_req);

  dl_config_req->dl_config_request_body.number_pdcch_ofdm_symbols           = 1;
  dl_config_req->dl_config_request_body.number_dci                          = 0;
  dl_config_req->dl_config_request_body.number_pdu                          = 0;
  dl_config_req->dl_config_request_body.number_pdsch_rnti                   = 0;

  if (retval!=0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  }
  return retval;
}

int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req)
{
  nfapi_vnf_p7_config_t *p7_config = vnf.p7_vnfs[0].config;

  tx_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

  //LOG_D(PHY, "[VNF] %s() TX_REQ sfn_sf:%d number_of_pdus:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(tx_req->sfn_sf), tx_req->tx_request_body.number_of_pdus);

  int retval = nfapi_vnf_p7_tx_req(p7_config, tx_req);


  if (retval!=0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    tx_req->tx_request_body.number_of_pdus = 0;
  }
  return retval;
}

int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req) {
  nfapi_vnf_p7_config_t *p7_config = vnf.p7_vnfs[0].config;

  hi_dci0_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

  //LOG_D(PHY, "[VNF] %s() HI_DCI0_REQ sfn_sf:%d dci:%d hi:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(hi_dci0_req->sfn_sf), hi_dci0_req->hi_dci0_request_body.number_of_dci, hi_dci0_req->hi_dci0_request_body.number_of_hi);

  int retval = nfapi_vnf_p7_hi_dci0_req(p7_config, hi_dci0_req);

  if (retval!=0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    hi_dci0_req->hi_dci0_request_body.number_of_hi = 0;
    hi_dci0_req->hi_dci0_request_body.number_of_dci = 0;
  }
  return retval;
}

int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) {

  nfapi_vnf_p7_config_t *p7_config = vnf.p7_vnfs[0].config;

  ul_config_req->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

  //LOG_D(PHY, "[VNF] %s() header message_id:%02x\n", __FUNCTION__, ul_config_req->header.message_id);

  //LOG_D(PHY, "[VNF] %s() UL_CONFIG sfn_sf:%d PDUs:%d rach_prach_frequency_resources:%d srs_present:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(ul_config_req->sfn_sf), ul_config_req->ul_config_request_body.number_of_pdus, ul_config_req->ul_config_request_body.rach_prach_frequency_resources, ul_config_req->ul_config_request_body.srs_present);

  int retval = nfapi_vnf_p7_ul_config_req(p7_config, ul_config_req);

  if (retval!=0) {
    LOG_E(PHY, "%s() Problem sending retval:%d\n", __FUNCTION__, retval);
  } else {
    // Reset number of PDUs so that it is not resent
    ul_config_req->ul_config_request_body.number_of_pdus = 0;
    ul_config_req->ul_config_request_body.rach_prach_frequency_resources = 0;
    ul_config_req->ul_config_request_body.srs_present = 0;
  }
  return retval;
}
