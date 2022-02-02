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

#include "openair2/LAYER2/MAC/mac_extern.h"
#include "openair2/LAYER2/MAC/mac.h"
#include "openair2/LAYER2/MAC/mac_proto.h"
#include "openair1/SCHED_UE/sched_UE.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_interface.h"
#include "openair2/PHY_INTERFACE/phy_stub_UE.h"
#include "openair2/ENB_APP/L1_paramdef.h"
#include "openair2/ENB_APP/enb_paramdef.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"
#include "common/config/config_load_configmodule.h"
#include "common/config/config_userapi.h"
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "openair1/PHY/defs_gNB.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface_scf.h"
#include "openair1/PHY/LTE_TRANSPORT/transport_common.h"
#include "softmodem-common.h"

extern int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind);
void configure_nfapi_pnf(char *vnf_ip_addr,
                         int vnf_p5_port,
                         char *pnf_ip_addr,
                         int pnf_p7_port,
                         int vnf_p7_port);

UL_IND_t *UL_INFO = NULL;

queue_t dl_config_req_tx_req_queue;
queue_t ul_config_req_queue;
queue_t hi_dci0_req_queue;

FILL_UL_INFO_MUTEX_t fill_ul_mutex;

int current_sfn_sf;
sem_t sfn_semaphore;

static sf_rnti_mcs_s sf_rnti_mcs[NUM_NFAPI_SUBFRAME];

static int ue_tx_sock_descriptor = -1;
static int ue_rx_sock_descriptor = -1;
static int get_mcs_from_sinr(float sinr);
static int get_cqi_from_mcs(void);
static void read_channel_param(const nfapi_dl_config_request_pdu_t * pdu, int sf, int index);
static bool did_drop_transport_block(int sf, uint16_t rnti);
static float get_bler_val(uint8_t mcs, int sinr);
static bool should_drop_transport_block(int sf, uint16_t rnti);
static void save_dci_pdu_for_crnti(nfapi_dl_config_request_t *dl_config_req);

extern nfapi_tx_request_pdu_t* tx_request_pdu[1023][NUM_NFAPI_SUBFRAME][10]; //TODO: NFAPI_TX_MAX_PDU for last dim? Check nfapi_pnf.c ln. 81
//extern int timer_subframe;
//extern int timer_frame;

extern UE_RRC_INST *UE_rrc_inst;
extern uint16_t sf_ahead;

static eth_params_t         stub_eth_params;
void Msg1_transmitted(module_id_t module_idP,uint8_t CC_id,frame_t frameP, uint8_t eNB_id);
void Msg3_transmitted(module_id_t module_idP,uint8_t CC_id,frame_t frameP, uint8_t eNB_id);

void fill_rx_indication_UE_MAC(module_id_t Mod_id,
                               int frame,
                               int subframe,
                               UL_IND_t *UL_INFO,
                               uint8_t *ulsch_buffer,
                               uint16_t buflen,
                               uint16_t rnti,
                               int index,
                               nfapi_ul_config_request_t *ul_config_req) {
  nfapi_rx_indication_pdu_t *pdu;
  int timing_advance_update;

  pthread_mutex_lock(&fill_ul_mutex.rx_mutex);

  UL_INFO->rx_ind.header.message_id = NFAPI_RX_ULSCH_INDICATION;
  UL_INFO->rx_ind.sfn_sf = frame << 4 | subframe;
  UL_INFO->rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
  UL_INFO->rx_ind.vendor_extension = ul_config_req->vendor_extension;

  assert(UL_INFO->rx_ind.rx_indication_body.number_of_pdus <= NFAPI_RX_IND_MAX_PDU);
  pdu = &UL_INFO->rx_ind.rx_indication_body
             .rx_pdu_list[UL_INFO->rx_ind.rx_indication_body.number_of_pdus];
  // pdu = &UL_INFO->rx_ind.rx_indication_body.rx_pdu_list[index];

  // pdu->rx_ue_information.handle          = eNB->ulsch[UE_id]->handle;
  pdu->rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti = rnti;
  pdu->rx_indication_rel8.tl.tag = NFAPI_RX_INDICATION_REL8_TAG;
  pdu->rx_indication_rel8.length = buflen;
  pdu->rx_indication_rel8.offset = 1;
  pdu->rx_indication_rel9.tl.tag = NFAPI_RX_INDICATION_REL9_TAG;
  pdu->rx_indication_rel9.timing_advance_r9 = 0;

  // ulsch_buffer is necessary to keep its value.
  assert(buflen <= NFAPI_RX_IND_DATA_MAX);
  memcpy(pdu->rx_ind_data, ulsch_buffer, buflen);
  LOG_I(MAC, "buflen of rx_ind pdu_data = %u SFN.SF: %d.%d\n", buflen,
        frame, subframe);
  // estimate timing advance for MAC
  timing_advance_update = 0; // Don't know what to put here
  pdu->rx_indication_rel8.timing_advance = timing_advance_update;

  int SNRtimes10 = 640;

  if (SNRtimes10 < -640)
    pdu->rx_indication_rel8.ul_cqi = 0;
  else if (SNRtimes10 > 635)
    pdu->rx_indication_rel8.ul_cqi = 255;
  else
    pdu->rx_indication_rel8.ul_cqi = (640 + SNRtimes10) / 5;

  UL_INFO->rx_ind.rx_indication_body.number_of_pdus++;
  UL_INFO->rx_ind.sfn_sf = frame << 4 | subframe;

  pthread_mutex_unlock(&fill_ul_mutex.rx_mutex);
}

void fill_sr_indication_UE_MAC(int Mod_id,
                               int frame,
                               int subframe,
                               UL_IND_t *UL_INFO,
                               uint16_t rnti,
                               nfapi_ul_config_request_t *ul_config_req) {
  pthread_mutex_lock(&fill_ul_mutex.sr_mutex);

  nfapi_sr_indication_t *sr_ind = &UL_INFO->sr_ind;
  nfapi_sr_indication_body_t *sr_ind_body = &sr_ind->sr_indication_body;
  assert(sr_ind_body->number_of_srs <= NFAPI_SR_IND_MAX_PDU);
  nfapi_sr_indication_pdu_t *pdu = &sr_ind_body->sr_pdu_list[sr_ind_body->number_of_srs];
  UL_INFO->sr_ind.vendor_extension = ul_config_req->vendor_extension;

  sr_ind->sfn_sf = frame << 4 | subframe;
  sr_ind->header.message_id = NFAPI_RX_SR_INDICATION;

  sr_ind_body->tl.tag = NFAPI_SR_INDICATION_BODY_TAG;

  pdu->instance_length = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti = rnti; // UE_mac_inst[Mod_id].crnti

  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
  pdu->ul_cqi_information.channel = 0;

  int SNRtimes10 = 640;
  if (SNRtimes10 < -640)
    pdu->ul_cqi_information.ul_cqi = 0;
  else if (SNRtimes10 > 635)
    pdu->ul_cqi_information.ul_cqi = 255;
  else
    pdu->ul_cqi_information.ul_cqi = (640 + SNRtimes10) / 5;

  // UL_INFO->rx_ind.rx_indication_body.number_of_pdus++;
  sr_ind_body->number_of_srs++;

  pthread_mutex_unlock(&fill_ul_mutex.sr_mutex);
}

void fill_crc_indication_UE_MAC(int Mod_id,
                                int frame,
                                int subframe,
                                UL_IND_t *UL_INFO,
                                uint8_t crc_flag,
                                int index,
                                uint16_t rnti,
                                nfapi_ul_config_request_t *ul_config_req) {
  pthread_mutex_lock(&fill_ul_mutex.crc_mutex);
  LOG_D(MAC, "fill crc_indication num_crcs: %u\n",
        UL_INFO->crc_ind.crc_indication_body.number_of_crcs);
  assert(UL_INFO->crc_ind.crc_indication_body.number_of_crcs < NUMBER_OF_UE_MAX);
  nfapi_crc_indication_pdu_t *pdu =
      &UL_INFO->crc_ind.crc_indication_body
           .crc_pdu_list[UL_INFO->crc_ind.crc_indication_body.number_of_crcs];

  UL_INFO->crc_ind.sfn_sf = frame << 4 | subframe;
  UL_INFO->crc_ind.vendor_extension = ul_config_req->vendor_extension;
  UL_INFO->crc_ind.header.message_id = NFAPI_CRC_INDICATION;
  UL_INFO->crc_ind.crc_indication_body.tl.tag = NFAPI_CRC_INDICATION_BODY_TAG;

  pdu->instance_length = 0;
  pdu->rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;

  pdu->rx_ue_information.rnti = rnti;
  pdu->crc_indication_rel8.tl.tag = NFAPI_CRC_INDICATION_REL8_TAG;
  pdu->crc_indication_rel8.crc_flag = crc_flag;

  UL_INFO->crc_ind.crc_indication_body.number_of_crcs++;

  LOG_D(PHY,
        "%s() rnti:%04x pdus:%d\n",
        __FUNCTION__,
        pdu->rx_ue_information.rnti,
        UL_INFO->crc_ind.crc_indication_body.number_of_crcs);

  pthread_mutex_unlock(&fill_ul_mutex.crc_mutex);
}

void fill_rach_indication_UE_MAC(int Mod_id,
                                 int frame,
                                 int subframe,
                                 UL_IND_t *UL_INFO,
                                 uint8_t ra_PreambleIndex,
                                 uint16_t ra_RNTI) {
  pthread_mutex_lock(&fill_ul_mutex.rach_mutex);

  UL_INFO->rach_ind.rach_indication_body.number_of_preambles = 1;

  UL_INFO->rach_ind.header.message_id = NFAPI_RACH_INDICATION;
  UL_INFO->rach_ind.sfn_sf = frame << 4 | subframe;
  UL_INFO->rach_ind.vendor_extension = NULL;

  UL_INFO->rach_ind.rach_indication_body.tl.tag = NFAPI_RACH_INDICATION_BODY_TAG;

  const int np = UL_INFO->rach_ind.rach_indication_body.number_of_preambles;
  UL_INFO->rach_ind.rach_indication_body.preamble_list =
      calloc(np, sizeof(nfapi_preamble_pdu_t));
  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.tl.tag =
      NFAPI_PREAMBLE_REL8_TAG;
  UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
      .preamble_rel8.timing_advance = 0; // Not sure about that

  // The two following should get extracted from the call to
  // get_prach_resources().
  UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
      .preamble_rel8.preamble = ra_PreambleIndex;
  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.rnti =
      ra_RNTI;
  // UL_INFO->rach_ind.rach_indication_body.number_of_preambles++;

  UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
      .preamble_rel13.rach_resource_type = 0;
  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].instance_length = 0;

  LOG_I(PHY,
        "UE Filling NFAPI indication for RACH : TA %d, Preamble %d, rnti %x, "
        "rach_resource_type %d\n",
        UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
            .preamble_rel8.timing_advance,
        UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
            .preamble_rel8.preamble,
        UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
            .preamble_rel8.rnti,
        UL_INFO->rach_ind.rach_indication_body.preamble_list[0]
            .preamble_rel13.rach_resource_type);

  // This function is currently defined only in the nfapi-RU-RAU-split so we
  // should call it when we merge with that branch.
  if (NFAPI_MODE == NFAPI_MODE_STANDALONE_PNF) {
    send_standalone_msg(UL_INFO, UL_INFO->rach_ind.header.message_id);
  } else {
    oai_nfapi_rach_ind(&UL_INFO->rach_ind);
  }

  free(UL_INFO->rach_ind.rach_indication_body.preamble_list);

  pthread_mutex_unlock(&fill_ul_mutex.rach_mutex);
}

void fill_ulsch_cqi_indication_UE_MAC(int Mod_id,
                                      uint16_t frame,
                                      uint8_t subframe,
                                      UL_IND_t *UL_INFO,
                                      uint16_t rnti) {
  pthread_mutex_lock(&fill_ul_mutex.cqi_mutex);
  LOG_D(MAC, "num_cqis: %u in fill_ulsch_cqi_indication_UE_MAC\n",
        UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis);
  assert(UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis <= NFAPI_CQI_IND_MAX_PDU);
  nfapi_cqi_indication_pdu_t *pdu =
      &UL_INFO->cqi_ind.cqi_indication_body
           .cqi_pdu_list[UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis];
  nfapi_cqi_indication_raw_pdu_t *raw_pdu =
      &UL_INFO->cqi_ind.cqi_indication_body.cqi_raw_pdu_list
           [UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis];

  UL_INFO->cqi_ind.sfn_sf = frame << 4 | subframe;
  // because of nfapi_vnf.c:733, set message id to 0, not
  // NFAPI_RX_CQI_INDICATION;
  UL_INFO->cqi_ind.header.message_id = NFAPI_RX_CQI_INDICATION;
  UL_INFO->cqi_ind.cqi_indication_body.tl.tag = NFAPI_CQI_INDICATION_BODY_TAG;

  pdu->rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti = rnti;
  // Since we assume that CRC flag is always 0 (ACK) I guess that data_offset
  // should always be 0.
  pdu->cqi_indication_rel8.data_offset = 0;

  pdu->cqi_indication_rel8.tl.tag = NFAPI_CQI_INDICATION_REL8_TAG;
  pdu->cqi_indication_rel8.length = 1;
  pdu->cqi_indication_rel8.ri = 0;

  pdu->cqi_indication_rel8.timing_advance = 0;
  // pdu->cqi_indication_rel8.number_of_cc_reported = 1;
  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
  pdu->ul_cqi_information.channel = 1;

  // eNB_scheduler_primitives.c:4839: the upper four bits seem to be the CQI
  const int cqi = get_cqi_from_mcs();
  raw_pdu->pdu[0] = cqi << 4;

  UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis++;

  pthread_mutex_unlock(&fill_ul_mutex.cqi_mutex);
}

void fill_ulsch_harq_indication_UE_MAC(
    int Mod_id,
    int frame,
    int subframe,
    UL_IND_t *UL_INFO,
    nfapi_ul_config_ulsch_harq_information *harq_information,
    uint16_t rnti,
    nfapi_ul_config_request_t *ul_config_req) {
  pthread_mutex_lock(&fill_ul_mutex.harq_mutex);

  assert(UL_INFO->harq_ind.harq_indication_body.number_of_harqs <= NFAPI_HARQ_IND_MAX_PDU);
  nfapi_harq_indication_pdu_t *pdu =
      &UL_INFO->harq_ind.harq_indication_body.harq_pdu_list
           [UL_INFO->harq_ind.harq_indication_body.number_of_harqs];

  UL_INFO->harq_ind.header.message_id = NFAPI_HARQ_INDICATION;
  UL_INFO->harq_ind.sfn_sf = frame << 4 | subframe;
  UL_INFO->harq_ind.vendor_extension = ul_config_req->vendor_extension;

  UL_INFO->harq_ind.harq_indication_body.tl.tag =
      NFAPI_HARQ_INDICATION_BODY_TAG;

  pdu->instance_length = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti = rnti;

  // For now we consider only FDD
  // if (eNB->frame_parms.frame_type == FDD) {
  pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
  pdu->harq_indication_fdd_rel13.mode = 0;
  pdu->harq_indication_fdd_rel13.number_of_ack_nack =
      harq_information->harq_information_rel10.harq_size;

  // Could this be wrong? Is the number_of_ack_nack field equivalent to O_ACK?
  // pdu->harq_indication_fdd_rel13.number_of_ack_nack = ulsch_harq->O_ACK;

  for (int i = 0; i < harq_information->harq_information_rel10.harq_size; i++) {
    pdu->harq_indication_fdd_rel13.harq_tb_n[i] = 1; // Assume always ACK (No NACK or DTX)
  }

  UL_INFO->harq_ind.harq_indication_body.number_of_harqs++;

  pthread_mutex_unlock(&fill_ul_mutex.harq_mutex);
}

void fill_uci_harq_indication_UE_MAC(int Mod_id,
			      int frame,
			      int subframe,
			      UL_IND_t *UL_INFO,
			      nfapi_ul_config_harq_information *harq_information,
			      uint16_t rnti,
            nfapi_ul_config_request_t *ul_config_req) {
  pthread_mutex_lock(&fill_ul_mutex.harq_mutex);

  nfapi_harq_indication_t *ind = &UL_INFO->harq_ind;
  nfapi_harq_indication_body_t *body = &ind->harq_indication_body;
  assert(UL_INFO->harq_ind.harq_indication_body.number_of_harqs <= NFAPI_HARQ_IND_MAX_PDU);
  nfapi_harq_indication_pdu_t *pdu =
      &body->harq_pdu_list[UL_INFO->harq_ind.harq_indication_body
                               .number_of_harqs];

  UL_INFO->harq_ind.vendor_extension = ul_config_req->vendor_extension;

  ind->sfn_sf = frame << 4 | subframe;
  ind->header.message_id = NFAPI_HARQ_INDICATION;

  body->tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
  pdu->rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;

  pdu->instance_length = 0; // don't know what to do with this
  pdu->rx_ue_information.rnti = rnti;

  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;

  int SNRtimes10 = 640;  // TODO: Replace with EpiSci SNR * 10

  if (SNRtimes10 < -640)
    pdu->ul_cqi_information.ul_cqi = 0;
  else if (SNRtimes10 > 635)
    pdu->ul_cqi_information.ul_cqi = 255;
  else
    pdu->ul_cqi_information.ul_cqi = (640 + SNRtimes10) / 5;
  pdu->ul_cqi_information.channel = 0;
  if (harq_information->harq_information_rel9_fdd.tl.tag
      == NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL9_FDD_TAG) {
    if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 0)
        && (harq_information->harq_information_rel9_fdd.harq_size == 1)) {
      pdu->harq_indication_fdd_rel13.tl.tag =
          NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
      pdu->harq_indication_fdd_rel13.mode = 0;
      pdu->harq_indication_fdd_rel13.number_of_ack_nack = 1;

      // AssertFatal(harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[0] == 4,
      // "harq_ack[0] is %d, should be 1,2 or 4\n",harq_ack[0]);
      pdu->harq_indication_fdd_rel13.harq_tb_n[0] =
          1; // Assuming always an ACK (No NACK or DTX)

      // TODO: Fix ack/dtx -- needed for 5G
      // 1.) if received dl_config_req (with c-rnti) store this info and corresponding subframe.
      // 2.) if receiving ul_config_req for uci ack/nack or ulsch ack/nak in subframe n
      //     go look to see if dl_config_req (with c-rnti) was received in subframe (n - 4)
      // 3.) if the answer to #2 is yes then send ACK IF NOT send DTX
      if (did_drop_transport_block(((subframe+6) % 10), rnti))  // TODO:  Handle DTX.  Also discuss handling PDCCH
      {
        pdu->harq_indication_fdd_rel13.harq_tb_n[0] = 2;
        LOG_I(PHY, "Setting HARQ No ACK - Channel Model\n");
      }

    } else if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 0)
               && (harq_information->harq_information_rel9_fdd.harq_size
                   == 2)) {
      pdu->harq_indication_fdd_rel13.tl.tag =
          NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
      pdu->harq_indication_fdd_rel13.mode = 0;
      pdu->harq_indication_fdd_rel13.number_of_ack_nack = 2;
      pdu->harq_indication_fdd_rel13.harq_tb_n[0] =
          1; // Assuming always an ACK (No NACK or DTX)
      pdu->harq_indication_fdd_rel13.harq_tb_n[1] =
          1; // Assuming always an ACK (No NACK or DTX)
    }
  } else if (harq_information->harq_information_rel10_tdd.tl.tag
             == NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL10_TDD_TAG) {
    if ((harq_information->harq_information_rel10_tdd.ack_nack_mode == 0)
        && (harq_information->harq_information_rel10_tdd.harq_size == 1)) {
      pdu->harq_indication_tdd_rel13.tl.tag =
          NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
      pdu->harq_indication_tdd_rel13.mode = 0;
      pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
      pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;

    } else if ((harq_information->harq_information_rel10_tdd.ack_nack_mode == 1)
               && (harq_information->harq_information_rel10_tdd.harq_size
                   == 2)) {
      pdu->harq_indication_tdd_rel13.tl.tag =
          NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
      pdu->harq_indication_tdd_rel13.mode = 0;
      pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
      pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;
      pdu->harq_indication_tdd_rel13.harq_data[1].bundling.value_0 = 1;
    }
  } else
    AssertFatal(1 == 0, "only format 1a/b for now, received \n");

  UL_INFO->harq_ind.harq_indication_body.number_of_harqs++;
  LOG_D(PHY,
        "Incremented eNB->UL_INFO.harq_ind.number_of_harqs:%d\n",
        UL_INFO->harq_ind.harq_indication_body.number_of_harqs);

  pthread_mutex_unlock(&fill_ul_mutex.harq_mutex);
}

void handle_nfapi_ul_pdu_UE_MAC(module_id_t Mod_id,
                                nfapi_ul_config_request_pdu_t *ul_config_pdu,
                                uint16_t frame,
                                uint8_t subframe,
                                uint8_t srs_present,
                                int index,
                                nfapi_ul_config_request_t *ul_config_req) {
  if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE) {
    LOG_D(PHY,
          "Applying UL config for UE, rnti %x for frame %d, subframe %d\n",
          (ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8).rnti,
          frame,
          subframe);
    uint8_t ulsch_buffer[5477] __attribute__((aligned(32)));
    uint16_t buflen = ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size;

    uint16_t rnti = ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti;
    uint8_t access_mode = SCHEDULED_ACCESS;
    if (buflen > 0) {
      if (UE_mac_inst[Mod_id].first_ULSCH_Tx == 1) { // Msg3 case
        LOG_D(MAC,
              "handle_nfapi_ul_pdu_UE_MAC 2.2, Mod_id:%d, SFN/SF: %d/%d \n",
              Mod_id,
              frame,
              subframe);
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti,
                                  ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  UE_mac_inst[Mod_id].RA_prach_resources.Msg3,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
        Msg3_transmitted(Mod_id, 0, frame, 0);
        //  Modification
        UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
        UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;

        // This should be done after the reception of the respective hi_dci0
        // UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
      } else {
        ue_get_sdu(Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  ulsch_buffer,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
      }
    }
  }

  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE) {
    // AssertFatal((UE_id =
    // find_ulsch(ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti,eNB,SEARCH_EXIST_OR_FREE))>=0,
    //            "No available UE ULSCH for rnti
    //            %x\n",ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti);
    uint8_t ulsch_buffer[5477] __attribute__((aligned(32)));
    uint16_t buflen =
        ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.size;
    nfapi_ul_config_ulsch_harq_information *ulsch_harq_information =
        &ul_config_pdu->ulsch_harq_pdu.harq_information;
    uint16_t rnti = ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti;
    uint8_t access_mode = SCHEDULED_ACCESS;
    if (buflen > 0) {
      if (UE_mac_inst[Mod_id].first_ULSCH_Tx == 1) {
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  UE_mac_inst[Mod_id].RA_prach_resources.Msg3,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
        Msg3_transmitted(Mod_id, 0, frame, 0);
        // UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
        // Modification
        UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
        UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
      } else {
        ue_get_sdu(Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  ulsch_buffer,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
      }
    }
    if (ulsch_harq_information != NULL)
      fill_ulsch_harq_indication_UE_MAC(
          Mod_id, frame, subframe, UL_INFO, ulsch_harq_information, rnti, ul_config_req);

  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE) {
    uint8_t ulsch_buffer[5477] __attribute__((aligned(32)));
    uint16_t buflen = ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.size;

    uint16_t rnti = ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti;
    uint8_t access_mode = SCHEDULED_ACCESS;
    if (buflen > 0) {
      if (UE_mac_inst[Mod_id].first_ULSCH_Tx == 1) { // Msg3 case
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  UE_mac_inst[Mod_id].RA_prach_resources.Msg3,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
        Msg3_transmitted(Mod_id, 0, frame, 0);
        // UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
        // Modification
        UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
        UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
      } else {
        ue_get_sdu(Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  ulsch_buffer,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
        fill_ulsch_cqi_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti);
      }
    }
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE) {
    uint8_t ulsch_buffer[5477] __attribute__((aligned(32)));
    uint16_t buflen = ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.size;
    nfapi_ul_config_ulsch_harq_information *ulsch_harq_information =
        &ul_config_pdu->ulsch_cqi_harq_ri_pdu.harq_information;

    uint16_t rnti = ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti;
    uint8_t access_mode = SCHEDULED_ACCESS;
    if (buflen > 0) {
      if (UE_mac_inst[Mod_id].first_ULSCH_Tx == 1) { // Msg3 case
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  UE_mac_inst[Mod_id].RA_prach_resources.Msg3,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
        Msg3_transmitted(Mod_id, 0, frame, 0);
        // UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
        // Modification
        UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
        UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
      } else {
        ue_get_sdu(Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
        fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti, ul_config_req);
        fill_rx_indication_UE_MAC(Mod_id,
                                  frame,
                                  subframe,
                                  UL_INFO,
                                  ulsch_buffer,
                                  buflen,
                                  rnti,
                                  index,
                                  ul_config_req);
        fill_ulsch_cqi_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti);
      }
    }

    if (ulsch_harq_information != NULL)
      fill_ulsch_harq_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_harq_information, rnti, ul_config_req);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) {
    uint16_t rnti = ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti;

    nfapi_ul_config_harq_information *ulsch_harq_information = &ul_config_pdu->uci_harq_pdu.harq_information;
    if (ulsch_harq_information != NULL)
      fill_uci_harq_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_harq_information, rnti, ul_config_req);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE) {
    AssertFatal(1 == 0, "NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE not handled yet\n");
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE) {
    AssertFatal(1 == 0,
                "NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE not handled yet\n");
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE) {
    AssertFatal(1 == 0,
                "NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE not handled yet\n");
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE) {
    uint16_t rnti =
        ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti;

    if (ue_get_SR(Mod_id, 0, frame, 0, rnti, subframe))
      fill_sr_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti, ul_config_req);

  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE) {
    // AssertFatal((UE_id =
    // find_uci(rel8->rnti,proc->frame_tx,proc->subframe_tx,eNB,SEARCH_EXIST_OR_FREE))>=0,
    //            "No available UE UCI for rnti
    //            %x\n",ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel8.rnti);

    uint16_t rnti =
        ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel8.rnti;

    // We fill the sr_indication only if ue_get_sr() would normally instruct PHY
    // to send a SR.
    if (ue_get_SR(Mod_id, 0, frame, 0, rnti, subframe))
      fill_sr_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti, ul_config_req);

    nfapi_ul_config_harq_information *ulsch_harq_information =
        &ul_config_pdu->uci_sr_harq_pdu.harq_information;
    if (ulsch_harq_information != NULL)
      fill_uci_harq_indication_UE_MAC(
          Mod_id, frame, subframe, UL_INFO, ulsch_harq_information, rnti, ul_config_req);
  }
}

int ul_config_req_UE_MAC(nfapi_ul_config_request_t *req,
                         int timer_frame,
                         int timer_subframe,
                         module_id_t Mod_id) {
  LOG_D(PHY,
        "[PNF] UL_CONFIG_REQ %s() sfn_sf:%d pdu:%d "
        "rach_prach_frequency_resources:%d srs_present:%u\n",
        __FUNCTION__,
        NFAPI_SFNSF2DEC(req->sfn_sf),
        req->ul_config_request_body.number_of_pdus,
        req->ul_config_request_body.rach_prach_frequency_resources,
        req->ul_config_request_body.srs_present);

  LOG_D(MAC, "ul_config_req Frame: %d Subframe: %d Proxy Frame: %u Subframe: %u\n",
        NFAPI_SFNSF2SFN(req->sfn_sf), NFAPI_SFNSF2SF(req->sfn_sf),
        timer_frame, timer_subframe);
  int sfn = NFAPI_SFNSF2SFN(req->sfn_sf);
  int sf = NFAPI_SFNSF2SF(req->sfn_sf);

  LOG_D(MAC,
        "ul_config_req_UE_MAC() TOTAL NUMBER OF UL_CONFIG PDUs: %d, SFN/SF: "
        "%d/%d \n",
        req->ul_config_request_body.number_of_pdus,
        timer_frame,
        timer_subframe);

  const rnti_t rnti = UE_mac_inst[Mod_id].crnti;
  for (int i = 0; i < req->ul_config_request_body.number_of_pdus; i++) {
    nfapi_ul_config_request_pdu_t* pdu = &req->ul_config_request_body.ul_config_pdu_list[i];
    const int pdu_type = pdu->pdu_type;
    if (   (pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE
            && pdu->ulsch_pdu.ulsch_pdu_rel8.rnti == rnti)
        || (pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE
            && pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == rnti)
        || (pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE
            && pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == rnti)
        || (pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE
            && pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == rnti)
        || (pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE
            && pdu->uci_cqi_harq_pdu.ue_information.ue_information_rel8.rnti == rnti)
        || (pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE
            && pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti == rnti)
        || (pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE
            && pdu->uci_cqi_sr_harq_pdu.ue_information.ue_information_rel8.rnti == rnti)) {
      handle_nfapi_ul_pdu_UE_MAC(
          Mod_id, pdu, sfn, sf, req->ul_config_request_body.srs_present, i, req);
    } else {
      LOG_D(MAC, "UNKNOWN UL_CONFIG_REQ PDU_TYPE or RNTI not matching pdu type: %d\n", pdu_type);
    }
  }

  return 0;
}

int tx_req_UE_MAC(nfapi_tx_request_t *req) {
  LOG_D(PHY,
        "%s() SFN/SF:%d/%d PDUs:%d\n",
        __FUNCTION__,
        NFAPI_SFNSF2SFN(req->sfn_sf),
        NFAPI_SFNSF2SF(req->sfn_sf),
        req->tx_request_body.number_of_pdus);

  for (int i = 0; i < req->tx_request_body.number_of_pdus; i++) {
    LOG_D(PHY,
          "%s() SFN/SF:%d/%d number_of_pdus:%d [PDU:%d] pdu_length:%d "
          "pdu_index:%d num_segments:%d\n",
          __FUNCTION__,
          NFAPI_SFNSF2SFN(req->sfn_sf),
          NFAPI_SFNSF2SF(req->sfn_sf),
          req->tx_request_body.number_of_pdus,
          i,
          req->tx_request_body.tx_pdu_list[i].pdu_length,
          req->tx_request_body.tx_pdu_list[i].pdu_index,
          req->tx_request_body.tx_pdu_list[i].num_segments);
  }

  return 0;
}

void dl_config_req_UE_MAC_dci(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *dci,
                              nfapi_dl_config_request_pdu_t *dlsch,
                              int num_ue,
                              nfapi_tx_req_pdu_list_t *tx_req_pdu_list) {
  DevAssert(dci->pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE);
  DevAssert(dlsch->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE);

  const rnti_t rnti = dci->dci_dl_pdu.dci_dl_pdu_rel8.rnti;
  const int rnti_type = dci->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type;
  if (rnti != dlsch->dlsch_pdu.dlsch_pdu_rel8.rnti) {
    LOG_E(MAC,
          "%s(): sfn/sf %d.%d DLSCH PDU RNTI %x does not match DCI RNTI %x\n",
          __func__, sfn, sf, rnti, dlsch->dlsch_pdu.dlsch_pdu_rel8.rnti);
    return;
  }

  const int pdu_index = dlsch->dlsch_pdu.dlsch_pdu_rel8.pdu_index;
  if (pdu_index < 0 || pdu_index >= tx_req_pdu_list->num_pdus) {
    LOG_E(MAC,
          "%s(): Problem with receiving data: "
          "sfn/sf:%d.%d PDU size:%d, TX_PDU index: %d\n",
          __func__,
          sfn, sf, dci->pdu_size, dlsch->dlsch_pdu.dlsch_pdu_rel8.pdu_index);
    return;
  }

  LOG_I(MAC, "%s() rnti value: 0x%x rnti type: %d\n", __func__,
        rnti, rnti_type);
  if (rnti_type == 1) { // C-RNTI (Normal DLSCH case)
    for (int ue_id = 0; ue_id < num_ue; ue_id++) {
      if (UE_mac_inst[ue_id].crnti == rnti) {
        LOG_D(MAC,
              "%s() Received data: sfn/sf:%d.%d "
              "size:%d, TX_PDU index: %d, tx_req_num_elems: %d \n",
              __func__,
              sfn, sf, dci->pdu_size,
              dlsch->dlsch_pdu.dlsch_pdu_rel8.pdu_index,
              tx_req_pdu_list->num_pdus);
        if (node_number > 0 || !should_drop_transport_block(sf, dci->dci_dl_pdu.dci_dl_pdu_rel8.rnti))
        {
          ue_send_sdu(ue_id, 0, sfn, sf,
              tx_req_pdu_list->pdus[pdu_index].segments[0].segment_data,
              tx_req_pdu_list->pdus[pdu_index].segments[0].segment_length,
              0);
        }
        else
        {
          LOG_I(MAC, "Transport Block discarded - ue_send_sdu not called. sf: %d", sf);
        }
        return;
      }
    }
  } else if (rnti_type == 2) {
    if (rnti == 0xFFFF) { /* SI-RNTI */
      for (int ue_id = 0; ue_id < num_ue; ue_id++) {
        if (UE_mac_inst[ue_id].UE_mode[0] == NOT_SYNCHED)
          continue;
        ue_decode_si(ue_id, 0, sfn, 0,
            tx_req_pdu_list->pdus[pdu_index].segments[0].segment_data,
            tx_req_pdu_list->pdus[pdu_index].segments[0].segment_length);
      }
    } else if (rnti == 0xFFFE) { /* PI-RNTI */
      for (int ue_id = 0; ue_id < num_ue; ue_id++) {
        LOG_I(MAC, "%s() Received paging message: sfn/sf:%d.%d\n",
              __func__, sfn, sf);

        ue_decode_p(ue_id, 0, sfn, 0,
                    tx_req_pdu_list->pdus[pdu_index].segments[0].segment_data,
                    tx_req_pdu_list->pdus[pdu_index].segments[0].segment_length);
      }
    } else if (rnti == 0x0002) { /* RA-RNTI */
      for (int ue_id = 0; ue_id < num_ue; ue_id++) {
        if (UE_mac_inst[ue_id].UE_mode[0] != RA_RESPONSE) {
          LOG_D(MAC, "UE %d not awaiting RAR, is in mode %d\n",
                ue_id, UE_mac_inst[ue_id].UE_mode[0]);
          continue;
        }
        // RNTI parameter not actually used. Provided only to comply with
        // existing function definition.  Not sure about parameters to fill
        // the preamble index.
        const rnti_t ra_rnti = UE_mac_inst[ue_id].RA_prach_resources.ra_RNTI;
        DevAssert(ra_rnti == 0x0002);
        if (UE_mac_inst[ue_id].UE_mode[0] != PUSCH
            && UE_mac_inst[ue_id].RA_prach_resources.Msg3 != NULL
            && ra_rnti == dlsch->dlsch_pdu.dlsch_pdu_rel8.rnti) {
          LOG_E(MAC,
                "%s(): Received RAR, PreambleIndex: %d\n",
                __func__, UE_mac_inst[ue_id].RA_prach_resources.ra_PreambleIndex);

          ue_process_rar(ue_id, 0, sfn,
              ra_rnti, //RA-RNTI
              tx_req_pdu_list->pdus[pdu_index].segments[0].segment_data,
              &UE_mac_inst[ue_id].crnti, //t-crnti
              UE_mac_inst[ue_id].RA_prach_resources.ra_PreambleIndex,
              tx_req_pdu_list->pdus[pdu_index].segments[0].segment_data);
          // UE_mac_inst[ue_id].UE_mode[0] = RA_RESPONSE;
          LOG_I(MAC, "setting UE_MODE now: %d\n", UE_mac_inst[ue_id].UE_mode[0]);
          // Expecting an UL_CONFIG_ULSCH_PDU to enable Msg3 Txon (first
          // ULSCH Txon for the UE)
          UE_mac_inst[ue_id].first_ULSCH_Tx = 1;
        }
      }
    }
    //else if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_BCH_PDU_TYPE) {
    //  // BCH case: Last parameter is 1 if first time synchronization and zero
    //  // otherwise.  Not sure which value to put for our case.
    //  if (UE_mac_inst[Mod_id].UE_mode[0] == NOT_SYNCHED){
    //    dl_phy_sync_success(Mod_id, sfn, 0, 1);
    //    LOG_E(MAC,
    //          "%s(): Received MIB: UE_mode: %d, sfn/sf: %d.%d\n",
    //          __func__,
    //          UE_mac_inst[Mod_id].UE_mode[0],
    //          sfn,
    //          sf);
    //    UE_mac_inst[Mod_id].UE_mode[0] = PRACH;
    //  } else {
    //    dl_phy_sync_success(Mod_id, sfn, 0, 0);
    //  }
    //} else if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_MCH_PDU_TYPE){
    //    if (UE_mac_inst[Mod_id].UE_mode[0] == NOT_SYNCHED) {
    //       /* this check is in the code before refactoring, but I don't know
    //        * why. Leave it in here for the moment */
    //       continue;
    //    }
    //    nfapi_dl_config_request_pdu_t *dl_config_pdu_tmp = &dl_config_pdu_list[i];
    //    const int pdu_index = dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index;
    //    ue_send_mch_sdu(Mod_id, 0, sfn,
    //        tx_request_pdu_list[pdu_index].segments[0].segment_data,
    //        tx_request_pdu_list[pdu_index].segments[0].segment_length,
    //        0,0);

    //}
    else {
      LOG_W(MAC, "can not handle special RNTI %x\n", rnti);
    }
  }
}

void dl_config_req_UE_MAC_bch(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *bch,
                              int num_ue) {
  DevAssert(bch->pdu_type == NFAPI_DL_CONFIG_BCH_PDU_TYPE);

  for (int ue_id = 0; ue_id < num_ue; ue_id++) {
    if (UE_mac_inst[ue_id].UE_mode[0] == NOT_SYNCHED){
      dl_phy_sync_success(ue_id, sfn, 0, 1);
      LOG_E(MAC,
            "%s(): Received MIB: UE_mode: %d, sfn/sf: %d.%d\n",
            __func__,
            UE_mac_inst[ue_id].UE_mode[0],
            sfn,
            sf);
      UE_mac_inst[ue_id].UE_mode[0] = PRACH;
    } else {
      dl_phy_sync_success(ue_id, sfn, 0, 0);
    }
  }
}

void dl_config_req_UE_MAC_mch(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *mch,
                              int num_ue,
                              nfapi_tx_req_pdu_list_t *tx_req_pdu_list) {
  DevAssert(mch->pdu_type == NFAPI_DL_CONFIG_MCH_PDU_TYPE);

  for (int ue_id = 0; ue_id < num_ue; ue_id++) {
    if (UE_mac_inst[ue_id].UE_mode[0] == NOT_SYNCHED){
	 LOG_D(MAC,
            "%s(): Received MCH in NOT_SYNCHED: UE_mode: %d, sfn/sf: %d.%d\n",
            __func__,
            UE_mac_inst[ue_id].UE_mode[0],
            sfn,
            sf);
	return;

    } else {
	 const int pdu_index = mch->mch_pdu.mch_pdu_rel8.pdu_index;
	if (pdu_index < 0 || pdu_index >= tx_req_pdu_list->num_pdus) {
    	LOG_E(MAC,
          "%s(): Problem with receiving data: "
          "sfn/sf:%d.%d PDU size:%d, TX_PDU index: %d\n",
          __func__,
          sfn, sf, mch->pdu_size, mch->mch_pdu.mch_pdu_rel8.pdu_index);
    	return;
  	}
        ue_send_mch_sdu(ue_id, 0, sfn,
            tx_req_pdu_list->pdus[pdu_index].segments[0].segment_data,
            tx_req_pdu_list->pdus[pdu_index].segments[0].segment_length,
            0,0);
    }
  }
}

void hi_dci0_req_UE_MAC(int sfn,
                        int sf,
                        nfapi_hi_dci0_request_pdu_t* hi_dci0,
                        int num_ue) {
  if (hi_dci0->pdu_type != NFAPI_HI_DCI0_DCI_PDU_TYPE)
    return;

  const nfapi_hi_dci0_dci_pdu_rel8_t *dci = &hi_dci0->dci_pdu.dci_pdu_rel8;
  if (!dci->cqi_csi_request)
    return;
  for (int ue_id = 0; ue_id < num_ue; ue_id++) {
    if (dci->rnti == UE_mac_inst[ue_id].crnti) {
      return;
    }
  }
}

// The following set of memcpy functions should be getting called as callback
// functions from pnf_p7_subframe_ind.

static bool is_my_ul_config_req(nfapi_ul_config_request_t *req)
{
  bool is_my_rnti = false;
  const rnti_t rnti = UE_mac_inst[0].crnti; // 0 for standalone pnf mode. TODO: Make this more clear - Andrew
  for (int i = 0; i < req->ul_config_request_body.number_of_pdus; i++)
  {
    nfapi_ul_config_request_pdu_t *pdu = &req->ul_config_request_body.ul_config_pdu_list[i];
    const int pdu_type = pdu->pdu_type;
    if ((pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE && pdu->ulsch_pdu.ulsch_pdu_rel8.rnti == rnti) ||
        (pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE && pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == rnti) ||
        (pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE && pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == rnti) ||
        (pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE && pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == rnti) ||
        (pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE && pdu->uci_cqi_harq_pdu.ue_information.ue_information_rel8.rnti == rnti) ||
        (pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE && pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti == rnti) ||
        (pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE && pdu->uci_cqi_sr_harq_pdu.ue_information.ue_information_rel8.rnti == rnti))
    {
      is_my_rnti = true;
      break;
    }
    else
    {
      LOG_D(MAC, "UNKNOWN UL_CONFIG_REQ PDU_TYPE: %d or RNTI is not mine \n", pdu_type);
    }
  }

  return is_my_rnti;
}

int memcpy_ul_config_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_ul_config_request_t *req)
{
  if (!is_my_ul_config_req(req)) return 0;

  nfapi_ul_config_request_t *p = malloc(sizeof(nfapi_ul_config_request_t));

  p->sfn_sf = req->sfn_sf;
  p->vendor_extension = req->vendor_extension;

  p->ul_config_request_body.number_of_pdus = req->ul_config_request_body.number_of_pdus;
  p->ul_config_request_body.rach_prach_frequency_resources = req->ul_config_request_body.rach_prach_frequency_resources;
  p->ul_config_request_body.srs_present = req->ul_config_request_body.srs_present;

  p->ul_config_request_body.tl.tag = req->ul_config_request_body.tl.tag;
  p->ul_config_request_body.tl.length = req->ul_config_request_body.tl.length;

  p->ul_config_request_body.ul_config_pdu_list =
      calloc(req->ul_config_request_body.number_of_pdus,
             sizeof(nfapi_ul_config_request_pdu_t));
  for (int i = 0; i < p->ul_config_request_body.number_of_pdus; i++)
  {
    p->ul_config_request_body.ul_config_pdu_list[i] =
        req->ul_config_request_body.ul_config_pdu_list[i];
  }

  if (!put_queue(&ul_config_req_queue, p))
  {
    free(p);
  }

  return 0;
}

void nfapi_free_tx_req_pdu_list(nfapi_tx_req_pdu_list_t *list)
{
  // free all the p[i].segments[j].segment_data memory
  int num_pdus = list->num_pdus;
  nfapi_tx_request_pdu_t *pdus = list->pdus;
  for (int i = 0; i < num_pdus; i++) {
    int num_segments = pdus[i].num_segments;
    for (int j = 0; j < num_segments; j++) {
      free(pdus[i].segments[j].segment_data);
      pdus[i].segments[j].segment_data = NULL;
    }
  }
  free(list);
}

static bool is_my_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req)
{
  bool is_my_rnti = false;
  const rnti_t rnti = UE_mac_inst[0].crnti; // This is 0 representing the num_ues is 1
  nfapi_hi_dci0_request_body_t *hi_dci0_body = &hi_dci0_req->hi_dci0_request_body;
  for (int i = 0; i < hi_dci0_body->number_of_dci + hi_dci0_body->number_of_hi; i++)
  {
    nfapi_hi_dci0_request_pdu_t *hi_dci0 = &hi_dci0_body->hi_dci0_pdu_list[i];
    if (hi_dci0->pdu_type != NFAPI_HI_DCI0_DCI_PDU_TYPE || hi_dci0->dci_pdu.dci_pdu_rel8.cqi_csi_request)
    {
      continue;
    }
    if (hi_dci0->dci_pdu.dci_pdu_rel8.rnti == rnti)
    {
      is_my_rnti = true;
      break;
    }
  }

  return is_my_rnti;
}

int memcpy_hi_dci0_req (L1_rxtx_proc_t *proc,
			nfapi_pnf_p7_config_t* pnf_p7,
			nfapi_hi_dci0_request_t* req) {

  if (!is_my_hi_dci0_req(req))
  {
    LOG_I(MAC, "Filtering hi_dci0_req\n");
    return 0;
  }
  nfapi_hi_dci0_request_t *p = (nfapi_hi_dci0_request_t *)malloc(sizeof(nfapi_hi_dci0_request_t));
	//if(req!=0){


  p->sfn_sf = req->sfn_sf;
  p->vendor_extension = req->vendor_extension;

  p->hi_dci0_request_body.number_of_dci = req->hi_dci0_request_body.number_of_dci;
  p->hi_dci0_request_body.number_of_hi = req->hi_dci0_request_body.number_of_hi;
  p->hi_dci0_request_body.sfnsf = req->hi_dci0_request_body.sfnsf;

  // UE_mac_inst[Mod_id].p->hi_dci0_request_body.tl =
  // req->hi_dci0_request_body.tl;
  p->hi_dci0_request_body.tl.tag = req->hi_dci0_request_body.tl.tag;
  p->hi_dci0_request_body.tl.length = req->hi_dci0_request_body.tl.length;

  int total_pdus = p->hi_dci0_request_body.number_of_dci
                   + p->hi_dci0_request_body.number_of_hi;

  p->hi_dci0_request_body.hi_dci0_pdu_list =
      calloc(total_pdus, sizeof(nfapi_hi_dci0_request_pdu_t));

  for (int i = 0; i < total_pdus; i++) {
    p->hi_dci0_request_body.hi_dci0_pdu_list[i] = req->hi_dci0_request_body.hi_dci0_pdu_list[i];
    // LOG_I(MAC, "Original hi_dci0 req. type:%d, Copy type: %d
    // \n",req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type,
    // UE_mac_inst[Mod_id].p->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type);
  }

  if (!put_queue(&hi_dci0_req_queue, p)) {
    free(p);
  }
  LOG_I(MAC, "DCI0 QUEUE: %zu\n", hi_dci0_req_queue.num_items);
  return 0;
}

void UE_config_stub_pnf(void) {
  int j;
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST, NULL, 0};

  config_getlist(&L1_ParamList, L1_Params, sizeof(L1_Params) / sizeof(paramdef_t), NULL);
  if (L1_ParamList.numelt > 0) {
    for (j = 0; j < L1_ParamList.numelt; j++) {
      // nb_L1_CC = *(L1_ParamList.paramarray[j][L1_CC_IDX].uptr); // Number of
      // component carriers is of no use for the
      // phy_stub mode UE pnf. Maybe we can completely skip it.

      if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {
        sf_ahead = 4; // Need 4 subframe gap between RX and TX
      }
      // Right now that we have only one UE (thread) it is ok to put the
      // eth_params in the UE_mac_inst. Later I think we have to change that to
      // attribute eth_params to a global element for all the UEs.
      else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        stub_eth_params.local_if_name = strdup(
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
        stub_eth_params.my_addr = strdup(
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
        stub_eth_params.remote_addr = strdup(
            *(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
        stub_eth_params.my_portc =
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
        stub_eth_params.remote_portc =
            *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
        stub_eth_params.my_portd =
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
        stub_eth_params.remote_portd =
            *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
        stub_eth_params.transp_preference = ETH_UDP_MODE;

        if (NFAPI_MODE!=NFAPI_UE_STUB_PNF)
          continue;

        sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2
        // configure_nfapi_pnf(UE_mac_inst[0].eth_params_n.remote_addr,
        // UE_mac_inst[0].eth_params_n.remote_portc,
        // UE_mac_inst[0].eth_params_n.my_addr,
        // UE_mac_inst[0].eth_params_n.my_portd,
        // UE_mac_inst[0].eth_params_n.remote_portd);
        configure_nfapi_pnf(stub_eth_params.remote_addr,
                            stub_eth_params.remote_portc,
                            stub_eth_params.my_addr,
                            stub_eth_params.my_portd,
                            stub_eth_params.remote_portd);
      }
    }
  }
}

void ue_init_standalone_socket(int tx_port, int rx_port)
{
  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(tx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (inet_pton(server_address.sin_family, stub_eth_params.remote_addr, &server_address.sin_addr) <= 0)
    {
      LOG_E(MAC, "Invalid standalone PNF Address\n");
      close(sd);
      return;
    }

    // Using connect to use send() instead of sendto()
    if (connect(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_tx_sock_descriptor == -1);
    ue_tx_sock_descriptor = sd;
  }

  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(rx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (bind(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_rx_sock_descriptor == -1);
    ue_rx_sock_descriptor = sd;
  }
}

void *ue_standalone_pnf_task(void *context)
{
  struct sockaddr_in server_address;
  socklen_t addr_len = sizeof(server_address);
  char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
  int sd = ue_rx_sock_descriptor;
  assert(sd > 0);

  nfapi_tx_request_t tx_req;
  nfapi_dl_config_request_t dl_config_req;
  bool tx_req_valid = false;
  bool dl_config_req_valid = false;
  while (true)
  {
    ssize_t len = recvfrom(sd, buffer, sizeof(buffer), MSG_TRUNC, (struct sockaddr *)&server_address, &addr_len);
    if (len == -1)
    {
      LOG_E(MAC, "reading from standalone pnf sctp socket failed \n");
      continue;
    }
    if (len > sizeof(buffer))
    {
      LOG_E(MAC, "%s(%d). Message truncated. %zd\n", __FUNCTION__, __LINE__, len);
      continue;
    }

    /* First we'll check for possible messages from the proxy. We do this by checking
       the length of the message. This works because sizeof(uint16_t) < sizeof(nfapi_p7_message_header_t)
       and sizeof(phy_channel_params_t) < sizeof(nfapi_p7_message_header_t) and
       sizeof(uint16_t) != sizeof(phy_channel_params_t). */
    if (len == sizeof(uint16_t))
    {
      uint16_t sfn_sf = 0;
      memcpy((void *)&sfn_sf, buffer, sizeof(sfn_sf));
      current_sfn_sf = sfn_sf;

      if (sem_post(&sfn_semaphore) != 0)
      {
        LOG_E(MAC, "sem_post() error\n");
        abort();
      }
    }
    else if (len == sizeof(phy_channel_params_t))
    {
      phy_channel_params_t ch_info;
      memcpy(&ch_info, buffer, sizeof(phy_channel_params_t));
      current_sfn_sf = ch_info.sfn_sf;
      if (sem_post(&sfn_semaphore) != 0)
      {
        LOG_E(MAC, "sem_post() error\n");
        abort();
      }
      uint16_t sf = ch_info.sfn_sf & 15;
      assert(sf < 10);

      sf_rnti_mcs[sf].sinr = ch_info.sinr;
      LOG_D(MAC, "Received_SINR = %f\n",ch_info.sinr);
    }
    else
    {
      nfapi_p7_message_header_t header;
      if (nfapi_p7_message_header_unpack((void *)buffer, len, &header, sizeof(header), NULL) < 0)
      {
        LOG_E(MAC, "Header unpack failed for standalone pnf\n");
        continue;
      }
      switch (header.message_id)
      {
      case NFAPI_DL_CONFIG_REQUEST:
      {
        if (dl_config_req_valid)
        {
          LOG_W(MAC, "Received consecutive dl_config_reqs. Previous dl_config_req frame: %u, subframe: %u\n",
                dl_config_req.sfn_sf >> 4, dl_config_req.sfn_sf & 15);
        }
        if (nfapi_p7_message_unpack((void *)buffer, len, &dl_config_req,
                                    sizeof(dl_config_req), NULL) < 0)
        {
          LOG_E(MAC, "Message dl_config_req failed to unpack\n");
          break;
        }

        LOG_I(MAC, "dl_config_req Frame: %u Subframe: %u\n", dl_config_req.sfn_sf >> 4,
              dl_config_req.sfn_sf & 15);

        dl_config_req_valid = true;

        save_dci_pdu_for_crnti(&dl_config_req);

        if (tx_req_valid)
        {
          if (dl_config_req.sfn_sf != tx_req.sfn_sf)
          {
              LOG_W(MAC, "sfnsf mismatch. dl_config_req Frame: %u Subframe: %u, Discarding tx_req Frame: %u Subframe: %u\n",
                    dl_config_req.sfn_sf >> 4, dl_config_req.sfn_sf & 15,
                    tx_req.sfn_sf >> 4, tx_req.sfn_sf & 15);
              tx_req_valid = false;
              break;
          }
          enqueue_dl_config_req_tx_req(&dl_config_req, &tx_req);
          dl_config_req_valid = false;
          tx_req_valid = false;
        }

        break;
      }
      case NFAPI_TX_REQUEST:
      {
        if (tx_req_valid)
        {
          LOG_W(MAC, "Received consecutive tx_reqs. Previous tx_req frame: %u, subframe: %u\n",
                tx_req.sfn_sf >> 4, tx_req.sfn_sf & 15);
        }
        if (nfapi_p7_message_unpack((void *)buffer, len, &tx_req,
                                    sizeof(tx_req), NULL) < 0)
        {
          LOG_E(MAC, "Message tx_req failed to unpack\n");
          break;
        }

        LOG_I(MAC, "tx_req Frame: %u Subframe: %u\n", tx_req.sfn_sf >> 4,
              tx_req.sfn_sf & 15);

        tx_req_valid = true;
        if (dl_config_req_valid)
        {
          if (dl_config_req.sfn_sf != tx_req.sfn_sf)
          {
              LOG_W(MAC, "sfnsf mismatch. Discarding dl_config_req Frame: %u Subframe: %u, tx_req Frame: %u Subframe: %u\n",
                    dl_config_req.sfn_sf >> 4, dl_config_req.sfn_sf & 15,
                    tx_req.sfn_sf >> 4, tx_req.sfn_sf & 15);
              dl_config_req_valid = false;
              break;
          }
          enqueue_dl_config_req_tx_req(&dl_config_req, &tx_req);
          dl_config_req_valid = false;
          tx_req_valid = false;
        }

        break;
      }
      case NFAPI_HI_DCI0_REQUEST:
      {
        nfapi_hi_dci0_request_t hi_dci0_req;
        // lock this hi_dci0_req
        if (nfapi_p7_message_unpack((void *)buffer, len, &hi_dci0_req,
                                    sizeof(hi_dci0_req), NULL) < 0)
        {
          LOG_E(MAC, "Message hi_dci0_req failed to unpack\n");
          break;
        }

        // check to see if hi_dci0_req is null
        memcpy_hi_dci0_req(NULL, NULL, &hi_dci0_req);

        break;
      }
      case NFAPI_UL_CONFIG_REQUEST:
      {
        nfapi_ul_config_request_t ul_config_req;
        // lock this ul_config_req
        if (nfapi_p7_message_unpack((void *)buffer, len, &ul_config_req,
                                    sizeof(ul_config_req), NULL) < 0)
        {
          LOG_E(MAC, "Message ul_config_req failed to unpack\n");
          break;
        }

        // check to see if ul_config_req is null
        memcpy_ul_config_req(NULL, NULL, &ul_config_req);

        break;
      }
      default:
        LOG_E(MAC, "Case Statement has no corresponding nfapi message\n");
        break;
      }
    }
  }
}

static void save_dci_pdu_for_crnti(nfapi_dl_config_request_t *dl_config_req)
{
  int count_sent = 0;
  int number_of_dci = dl_config_req->dl_config_request_body.number_dci;
  int number_of_pdu = dl_config_req->dl_config_request_body.number_pdu;
  if (number_of_pdu <= 0)
  {
    LOG_E(MAC, "%s: dl_config_req pdu size <= 0\n", __FUNCTION__);
    abort();
  }
  if (number_of_dci > 0)
  {
    for (int n = 0; n < number_of_pdu; n++)
    {
      const nfapi_dl_config_request_pdu_t *current_pdu_list = &dl_config_req->dl_config_request_body.dl_config_pdu_list[n];
      int rnti_type = current_pdu_list->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type;
      if (current_pdu_list->pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE && rnti_type == 1 && count_sent < number_of_dci)
      {
        read_channel_param(current_pdu_list, (dl_config_req->sfn_sf & 15), count_sent);
        count_sent++;
      }
    }
  }
}

void *memcpy_dl_config_req_standalone(nfapi_dl_config_request_t *dl_config_req)
{
  nfapi_dl_config_request_t *p = malloc(sizeof(nfapi_dl_config_request_t));

  p->sfn_sf = dl_config_req->sfn_sf;

  p->vendor_extension = dl_config_req->vendor_extension;

  p->dl_config_request_body.number_dci = dl_config_req->dl_config_request_body.number_dci;
  p->dl_config_request_body.number_pdcch_ofdm_symbols = dl_config_req->dl_config_request_body.number_pdcch_ofdm_symbols;
  p->dl_config_request_body.number_pdsch_rnti = dl_config_req->dl_config_request_body.number_pdsch_rnti;
  p->dl_config_request_body.number_pdu = dl_config_req->dl_config_request_body.number_pdu;

  p->dl_config_request_body.tl.tag = dl_config_req->dl_config_request_body.tl.tag;
  p->dl_config_request_body.tl.length = dl_config_req->dl_config_request_body.tl.length;

  p->dl_config_request_body.dl_config_pdu_list =
      calloc(dl_config_req->dl_config_request_body.number_pdu,
             sizeof(nfapi_dl_config_request_pdu_t));
  for (int i = 0; i < p->dl_config_request_body.number_pdu; i++) {
    p->dl_config_request_body.dl_config_pdu_list[i] =
        dl_config_req->dl_config_request_body.dl_config_pdu_list[i];
  }
  return p;
}

void *memcpy_tx_req_standalone(nfapi_tx_request_t *tx_req)
{
  int num_pdus = tx_req->tx_request_body.number_of_pdus;

  nfapi_tx_req_pdu_list_t *list = calloc(1, sizeof(nfapi_tx_req_pdu_list_t) + num_pdus * sizeof(nfapi_tx_request_pdu_t));
  list->num_pdus = num_pdus;
  nfapi_tx_request_pdu_t *p = list->pdus;

  for (int i = 0; i < num_pdus; i++)
  {
    p[i].num_segments = tx_req->tx_request_body.tx_pdu_list[i].num_segments;
    p[i].pdu_index = tx_req->tx_request_body.tx_pdu_list[i].pdu_index;
    p[i].pdu_length = tx_req->tx_request_body.tx_pdu_list[i].pdu_length;
    for (int j = 0; j < tx_req->tx_request_body.tx_pdu_list[i].num_segments; j++)
    {
      p[i].segments[j].segment_length = tx_req->tx_request_body.tx_pdu_list[i].segments[j].segment_length;
      if (p[i].segments[j].segment_length > 0)
      {
        p[i].segments[j].segment_data = calloc(
            p[i].segments[j].segment_length, sizeof(uint8_t));
        memcpy(p[i].segments[j].segment_data,
               tx_req->tx_request_body.tx_pdu_list[i].segments[j].segment_data,
               p[i].segments[j].segment_length);
      }
    }
  }
  return list;
}

static bool is_my_dl_config_req(const nfapi_dl_config_request_t *req)
{

  const rnti_t my_rnti = UE_mac_inst[0].crnti;
  int num_pdus = req->dl_config_request_body.number_pdu;
  // look through list of pdus for rnti type 1 with my_rnti (normal dlsch case)
  for (int i = 0; i < num_pdus; i++)
  {
    nfapi_dl_config_request_pdu_t *pdu = &req->dl_config_request_body.dl_config_pdu_list[i];
    const int pdu_type = pdu->pdu_type;
    if (pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)
    {
      const rnti_t dci_rnti = pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti;
      const int rnti_type = pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type;
      if (rnti_type == 1 && dci_rnti == my_rnti)
      {
        return true;
      }
    }
    else if(pdu_type != NFAPI_DL_CONFIG_DLSCH_PDU_TYPE)
    {
      return true; // Because these two pdu_types are coupled and we have to accept all other pdu_types
    }
  }

  // Look for broadcasted rnti types
  for (int i = 0; i < num_pdus; i++)
  {
    nfapi_dl_config_request_pdu_t *pdu = &req->dl_config_request_body.dl_config_pdu_list[i];
    const int pdu_type = pdu->pdu_type;
    if (pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)
    {
      const int rnti_type = pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type;
      if (rnti_type != 1)
      {
        return true;
      }
    }
  }


  return false;
}

void enqueue_dl_config_req_tx_req(nfapi_dl_config_request_t *dl_config_req, nfapi_tx_request_t *tx_req)
{
  if (!is_my_dl_config_req(dl_config_req))
  {
    LOG_I(MAC, "Filtering dl_config_req and tx_req\n");
    return;
  }

  nfapi_dl_config_request_t *dl_config_req_temp = memcpy_dl_config_req_standalone(dl_config_req);
  nfapi_tx_req_pdu_list_t *tx_req_temp = memcpy_tx_req_standalone(tx_req);
  LOG_I(MAC, "This is the num_pdus for tx_req: %d\n", tx_req_temp->num_pdus);
  LOG_I(MAC, "This is the num_pdus for dl_config_req and the sfn_sf: %d, %d:%d\n", dl_config_req_temp->dl_config_request_body.number_pdu,
        NFAPI_SFNSF2SFN(dl_config_req_temp->sfn_sf), NFAPI_SFNSF2SF(dl_config_req_temp->sfn_sf));

  nfapi_dl_config_req_tx_req_t *req = malloc(sizeof(nfapi_dl_config_req_tx_req_t));
  req->dl_config_req = dl_config_req_temp;
  req->tx_req_pdu_list = tx_req_temp;

  if (!put_queue(&dl_config_req_tx_req_queue, req))
  {
    free(req->dl_config_req);
    nfapi_free_tx_req_pdu_list(req->tx_req_pdu_list);
    free(req);
  }
}


__attribute__((unused))
static void print_rx_ind(nfapi_rx_indication_t *p)
{
  printf("Printing RX_IND fields\n");
  printf("header.message_id: %u\n", p->header.message_id);
  printf("header.phy_id: %u\n", p->header.phy_id);
  printf("header.message_id: %u\n", p->header.message_id);
  printf("header.m_segment_sequence: %u\n", p->header.m_segment_sequence);
  printf("header.checksum: %u\n", p->header.checksum);
  printf("header.transmit_timestamp: %u\n", p->header.transmit_timestamp);
  printf("sfn_sf: %u\n", p->sfn_sf);
  printf("rx_indication_body.tl.tag: 0x%x\n", p->rx_indication_body.tl.tag);
  printf("rx_indication_body.tl.length: %u\n", p->rx_indication_body.tl.length);
  printf("rx_indication_body.number_of_pdus: %u\n", p->rx_indication_body.number_of_pdus);

  assert(p->rx_indication_body.number_of_pdus <= NFAPI_RX_IND_MAX_PDU);
  nfapi_rx_indication_pdu_t *pdu = p->rx_indication_body.rx_pdu_list;
  for (int i = 0; i < p->rx_indication_body.number_of_pdus; i++)
  {
    printf("pdu %d nfapi_rx_ue_information.tl.tag: 0x%x\n", i, pdu->rx_ue_information.tl.tag);
    printf("pdu %d nfapi_rx_ue_information.tl.length: %u\n", i, pdu->rx_ue_information.tl.length);
    printf("pdu %d nfapi_rx_ue_information.handle: %u\n", i, pdu->rx_ue_information.handle);
    printf("pdu %d nfapi_rx_ue_information.rnti: %u\n", i, pdu->rx_ue_information.rnti);
    printf("pdu %d nfapi_rx_indication_rel8.tl.tag: 0x%x\n", i, pdu->rx_indication_rel8.tl.tag);
    printf("pdu %d nfapi_rx_indication_rel8.tl.length: %u\n", i, pdu->rx_indication_rel8.tl.length);
    printf("pdu %d nfapi_rx_indication_rel8.length: %u\n", i, pdu->rx_indication_rel8.length);
    printf("pdu %d nfapi_rx_indication_rel8.offset: %u\n", i, pdu->rx_indication_rel8.offset);
    printf("pdu %d nfapi_rx_indication_rel8.ul_cqi: %u\n", i, pdu->rx_indication_rel8.ul_cqi);
    printf("pdu %d nfapi_rx_indication_rel8.timing_advance: %u\n", i, pdu->rx_indication_rel8.timing_advance);
    printf("pdu %d nfapi_rx_indication_rel9.tl.tag: 0x%x\n", i, pdu->rx_indication_rel9.tl.tag);
    printf("pdu %d nfapi_rx_indication_rel9.tl.length: %u\n", i, pdu->rx_indication_rel9.tl.length);
    printf("pdu %d nfapi_rx_indication_rel9.timing_advance_r9: %u\n", i, pdu->rx_indication_rel9.timing_advance_r9);
  }

  fflush(stdout);
}

  void send_standalone_msg(UL_IND_t *UL, nfapi_message_id_e msg_type)
  {
    int encoded_size = -1;
    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];

    switch (msg_type)
    {
    case NFAPI_RACH_INDICATION:
      encoded_size = nfapi_p7_message_pack(&UL->rach_ind, buffer, sizeof(buffer), NULL);
      LOG_A(MAC, "RACH_IND sent to Proxy, Size: %d Frame %d Subframe %d\n", encoded_size,
            NFAPI_SFNSF2SFN(UL->rach_ind.sfn_sf), NFAPI_SFNSF2SF(UL->rach_ind.sfn_sf));
      break;
    case NFAPI_CRC_INDICATION:
      encoded_size = nfapi_p7_message_pack(&UL->crc_ind, buffer, sizeof(buffer), NULL);
      LOG_I(MAC, "CRC_IND sent to Proxy, Size: %d Frame %d Subframe %d num_crcs: %u\n", encoded_size,
            NFAPI_SFNSF2SFN(UL->crc_ind.sfn_sf), NFAPI_SFNSF2SF(UL->crc_ind.sfn_sf),
            UL->crc_ind.crc_indication_body.number_of_crcs);
      break;
    case NFAPI_RX_ULSCH_INDICATION:
      encoded_size = nfapi_p7_message_pack(&UL->rx_ind, buffer, sizeof(buffer), NULL);
      LOG_I(MAC, "RX_IND sent to Proxy, Size: %d Frame %d Subframe %d rx_ind.tl.length: %u num_pdus: %u\n",
            encoded_size, NFAPI_SFNSF2SFN(UL->rx_ind.sfn_sf), NFAPI_SFNSF2SF(UL->rx_ind.sfn_sf),
            UL->rx_ind.rx_indication_body.tl.length, UL->rx_ind.rx_indication_body.number_of_pdus);
      break;
    case NFAPI_RX_CQI_INDICATION:
      encoded_size = nfapi_p7_message_pack(&UL->cqi_ind, buffer, sizeof(buffer), NULL);
      LOG_I(MAC, "CQI_IND sent to Proxy, Size: %d num_cqis: %u\n", encoded_size,
            UL->cqi_ind.cqi_indication_body.number_of_cqis);
      break;
    case NFAPI_HARQ_INDICATION:
      encoded_size = nfapi_p7_message_pack(&UL->harq_ind, buffer, sizeof(buffer), NULL);
      LOG_I(MAC, "HARQ_IND sent to Proxy, Size: %d Frame %d Subframe %d\n", encoded_size,
            NFAPI_SFNSF2SFN(UL->harq_ind.sfn_sf), NFAPI_SFNSF2SF(UL->harq_ind.sfn_sf));
      break;
    case NFAPI_RX_SR_INDICATION:
      encoded_size = nfapi_p7_message_pack(&UL->sr_ind, buffer, sizeof(buffer), NULL);
      LOG_I(MAC, "SR_IND sent to Proxy, Size: %d\n", encoded_size);
      break;
    default:
      LOG_I(MAC, "%s Unknown Message msg_type :: %u\n", __func__, msg_type);
      return;
    }
    if (encoded_size < 0)
    {
      LOG_E(MAC, "standalone pack failed\n");
      return;
    }
    if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
    {
      LOG_E(MAC, "Send Proxy UE failed\n");
      return;
    }
  }

  void send_standalone_dummy()
  {
    static const uint16_t dummy[] = {0, 0};
    if (send(ue_tx_sock_descriptor, dummy, sizeof(dummy), 0) < 0)
    {
      LOG_E(MAC, "send dummy to OAI UE failed: %s\n", strerror(errno));
      return;
    }
  }

const char *dl_pdu_type_to_string(uint8_t pdu_type)
{
  switch (pdu_type)
  {
    case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE:
      return "DCI_DL_PDU_TYPE";
    case NFAPI_DL_CONFIG_BCH_PDU_TYPE:
      return "BCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_MCH_PDU_TYPE:
      return "MCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE:
      return "DLSCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_PCH_PDU_TYPE:
      return "PCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_PRS_PDU_TYPE:
      return "PRS_PDU_TYPE";
    case NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE:
      return "CSI_RS_PDU_TYPE";
    case NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE:
      return "EPDCCH_DL_PDU_TYPE";
    case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE:
      return "MPDCCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_NBCH_PDU_TYPE:
      return "NBCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE:
      return "NPDCCH_PDU_TYPE";
    case NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE:
      return "NDLSCH_PDU_TYPE";
    default:
      LOG_E(MAC, "%s No corresponding PDU for type: %u\n", __func__, pdu_type);
      return "UNKNOWN";
  }
}

const char *ul_pdu_type_to_string(uint8_t pdu_type)
{
  switch (pdu_type)
  {
    case NFAPI_UL_CONFIG_ULSCH_PDU_TYPE:
      return "UL_CONFIG_ULSCH_PDU_TYPE";
    case NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE:
      return "UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE";
    case NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE:
      return "UL_CONFIG_ULSCH_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE:
      return "UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE:
      return "UL_CONFIG_UCI_CQI_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE:
      return "UCI_SR_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE:
      return "UCI_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE:
      return "UCI_SR_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE:
      return "UCI_CQI_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE:
      return "UCI_CQI_SR_PDU_TYPE";
    case NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE:
      return "UCI_CQI_SR_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_SRS_PDU_TYPE:
      return "SRS_PDU_TYPE";
    case NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE:
      return "HARQ_BUFFER_PDU_TYPE";
    case NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE:
      return "PDU_TYPE";
    case NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE:
      return "ULSCH_UCI_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE:
      return "ULSCH_CSI_UCI_HARQ_PDU_TYPE";
    case NFAPI_UL_CONFIG_NULSCH_PDU_TYPE:
      return "NULSCH_PDU_TYPE";
    case NFAPI_UL_CONFIG_NRACH_PDU_TYPE:
      return "NRACH_PDU_TYPE";
    default:
      LOG_E(MAC, "%s No corresponding PDU for type: %u\n", __func__, pdu_type);
      return "UNKNOWN";
  }
}

char *nfapi_dl_config_req_to_string(nfapi_dl_config_request_t *req)
{
    const size_t max_result = 1024;
    uint16_t num_pdus = req->dl_config_request_body.number_pdu;
    int subframe = req->sfn_sf & 15;
    int frame = req->sfn_sf >> 4;
    char *result = malloc(max_result);
    snprintf(result, max_result, "num_pdus=%u Frame=%d Subframe=%d",
        num_pdus, frame, subframe);
    for (size_t i = 0; i < num_pdus; ++i)
    {
        int len = strlen(result);
        if (len >= max_result - 1)
        {
            break;
        }
        snprintf(result + len, max_result - len, " pdu_type=%s",
            dl_pdu_type_to_string(req->dl_config_request_body.dl_config_pdu_list[i].pdu_type));
    }
    return result;
}

char *nfapi_ul_config_req_to_string(nfapi_ul_config_request_t *req)
{
    const size_t max_result = 1024;
    uint16_t num_pdus = req->ul_config_request_body.number_of_pdus;
    int subframe = req->sfn_sf & 15;
    int frame = req->sfn_sf >> 4;
    char *result = malloc(max_result);
    snprintf(result, max_result, "num_pdus=%u Frame=%d Subframe=%d",
        num_pdus, frame, subframe);
    for (size_t i = 0; i < num_pdus; ++i)
    {
        int len = strlen(result);
        if (len >= max_result - 1)
        {
            break;
        }
        snprintf(result + len, max_result - len, " pdu_type=%s",
            ul_pdu_type_to_string(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type));
    }
    return result;
}

/* Dummy functions*/

  void handle_nfapi_hi_dci0_dci_pdu(
      PHY_VARS_eNB * eNB,
      int frame,
      int subframe,
      L1_rxtx_proc_t *proc,
      nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu)
  {
  }

  void handle_nfapi_hi_dci0_hi_pdu(
      PHY_VARS_eNB * eNB,
      int frame,
      int subframe,
      L1_rxtx_proc_t *proc,
      nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu)
  {
  }

  void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB * eNB,
                               int frame,
                               int subframe,
                               L1_rxtx_proc_t *proc,
                               nfapi_dl_config_request_pdu_t *dl_config_pdu)
  {
  }

  void handle_nfapi_bch_pdu(PHY_VARS_eNB * eNB,
                            L1_rxtx_proc_t * proc,
                            nfapi_dl_config_request_pdu_t * dl_config_pdu,
                            uint8_t * sdu)
  {
  }

  void handle_nfapi_dlsch_pdu(PHY_VARS_eNB * eNB,
                              int frame,
                              int subframe,
                              L1_rxtx_proc_t *proc,
                              nfapi_dl_config_request_pdu_t *dl_config_pdu,
                              uint8_t codeword_index,
                              uint8_t *sdu)
  {
  }

  void handle_nfapi_ul_pdu(PHY_VARS_eNB * eNB,
                           L1_rxtx_proc_t * proc,
                           nfapi_ul_config_request_pdu_t * ul_config_pdu,
                           uint16_t frame,
                           uint8_t subframe,
                           uint8_t srs_present)
  {
  }
  void handle_nfapi_mch_pdu(PHY_VARS_eNB * eNB,
                            L1_rxtx_proc_t * proc,
                            nfapi_dl_config_request_pdu_t * dl_config_pdu,
                            uint8_t * sdu)
  {
  }

  void phy_config_request(PHY_Config_t * phy_config)
  {
  }

  void phy_config_update_sib2_request(PHY_Config_t * phy_config)
  {
  }

  void phy_config_update_sib13_request(PHY_Config_t * phy_config)
  {
  }

  uint32_t from_earfcn(int eutra_bandP, uint32_t dl_earfcn)
  {
    return (0);
  }

  int32_t get_uldl_offset(int eutra_bandP)
  {
    return (0);
  }

  int l1_north_init_eNB(void)
  {
    return 0;
  }

  void init_eNB_afterRU(void)
  {
  }

static int get_mcs_from_sinr(float sinr)
{
  if (sinr < (bler_data[0].bler_table[0][0]))
  {
    LOG_I(MAC, "The SINR was found is smaller than first MCS table\n");
    return 0;
  }

  if (sinr > (bler_data[NUM_MCS-1].bler_table[bler_data[NUM_MCS-1].length - 1][0]))
  {
    LOG_I(MAC, "The SINR was found is larger than last MCS table\n");
    return NUM_MCS-1;
  }

  for (int n = NUM_MCS-1; n >= 0; n--)
  {
    CHECK_INDEX(bler_data, n);
    float largest_sinr = (bler_data[n].bler_table[bler_data[n].length - 1][0]);
    float smallest_sinr = (bler_data[n].bler_table[0][0]);
    if (sinr < largest_sinr && sinr > smallest_sinr)
    {
      LOG_I(MAC, "The SINR was found in MCS %d table\n", n);
      return n;
    }
  }
  LOG_E(MAC, "Unable to get an MCS value.\n");
  abort();
}

static int get_cqi_from_mcs(void)
{
  static const int mcs_to_cqi[] = {0, 1, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15};
  assert(NUM_ELEMENTS(mcs_to_cqi) == NUM_MCS);
  int sf = 0;
  while(sf < NUM_NFAPI_SUBFRAME)
  {
    if (sf_rnti_mcs[sf].latest)
    {
      int pdu_size = sf_rnti_mcs[sf].pdu_size;
      if (pdu_size <= 0)
      {
        LOG_E(MAC, "%s: sf_rnti_mcs[%d].pdu_size = 0\n", __FUNCTION__, sf);
        abort();
      }

      CHECK_INDEX(sf_rnti_mcs[sf].mcs, pdu_size);
      int mcs = get_mcs_from_sinr(sf_rnti_mcs[sf].sinr);
      CHECK_INDEX(mcs_to_cqi, mcs);
      int cqi = mcs_to_cqi[mcs];
      LOG_I(MAC, "SINR: %f -> MCS: %d -> CQI: %d\n", sf_rnti_mcs[sf].sinr, mcs, cqi);
      return cqi;
    }
    sf++;
  }
  LOG_E(MAC, "Unable to get CQI value because no MCS found\n");
  abort();
}

static void read_channel_param(const nfapi_dl_config_request_pdu_t * pdu, int sf, int index)
{
  if (pdu == NULL)
  {
    LOG_E(MAC,"PDU NULL\n");
    abort();
  }

  /* This function is executed for every dci pdu type in a dl_config_req. We save
     the assocaited MCS and RNTI value for each. The 'index' passed in is a count of
     how many times we have called this function for a particular dl_config_req. It
     allows us to save MCS/RNTI data in correct indicies when multiple dci's are received.*/
  for (int i = 0; i < NUM_NFAPI_SUBFRAME; i++)
  {
    if (i == sf)
    {
      CHECK_INDEX(sf_rnti_mcs[sf].rnti, index);
      CHECK_INDEX(sf_rnti_mcs[sf].mcs, index);
      CHECK_INDEX(sf_rnti_mcs[sf].drop_flag, index);
      sf_rnti_mcs[sf].rnti[index] = pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti;
      sf_rnti_mcs[sf].mcs[index] = pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1;
      sf_rnti_mcs[sf].drop_flag[index] = false;
      sf_rnti_mcs[sf].pdu_size = index+1; //index starts at 0 so we incrament to get num of pdus
      sf_rnti_mcs[sf].latest = true;
      LOG_I(MAC, "Adding MCS %d and RNTI %x for sf_rnti_mcs[%d]\n", sf_rnti_mcs[sf].mcs[index], sf_rnti_mcs[sf].rnti[index], sf);
    }
    else
    {
      sf_rnti_mcs[i].latest = false;
    }
  }
  return;
}

static bool did_drop_transport_block(int sf, uint16_t rnti)
{
  int pdu_size = sf_rnti_mcs[sf].pdu_size;
  if(pdu_size <= 0)
  {
    LOG_E(MAC, "Problem, the PDU size is <= 0. We dropped this packet\n");
    return true;
  }
  CHECK_INDEX(sf_rnti_mcs[sf].rnti, pdu_size);
  CHECK_INDEX(sf_rnti_mcs[sf].drop_flag, pdu_size);
  for (int n = 0; n < pdu_size; n++)
  {
    if (sf_rnti_mcs[sf].rnti[n] == rnti && sf_rnti_mcs[sf].drop_flag[n])
    {
      return true;
    }
  }
  return false;
}

static float get_bler_val(uint8_t mcs, int sinr)
{
  // 4th col = dropped packets, 5th col = total packets
  float bler_val = 0.0;
  CHECK_INDEX(bler_data, mcs);
  if (sinr < (int)(bler_data[mcs].bler_table[0][0] * 10))
  {
    LOG_I(MAC, "MCS %d table. SINR is smaller than lowest SINR, bler_val is set based on lowest SINR in table\n", mcs);
    bler_val = bler_data[mcs].bler_table[0][4] / bler_data[mcs].bler_table[0][5];
    return bler_val;
  }
  if (sinr > (int)(bler_data[mcs].bler_table[bler_data[mcs].length - 1][0] * 10))
  {
    LOG_I(MAC, "MCS %d table. SINR is greater than largest SINR. bler_val is set based on largest SINR in table\n", mcs);
    bler_val = bler_data[mcs].bler_table[(bler_data[mcs].length - 1)][4] / bler_data[mcs].bler_table[(bler_data[mcs].length - 1)][5];
    return bler_val;
  }
  // Loop through bler table to find sinr_index
  for (int i = 0; i < bler_data[mcs].length; i++)
  {
    int temp_sinr = (int)(bler_data[mcs].bler_table[i][0] * 10);
    if (temp_sinr == sinr)
    {
      bler_val = bler_data[mcs].bler_table[i][4] / bler_data[mcs].bler_table[i][5];
      return bler_val;
    }
    // Linear interpolation when SINR is between indices
    if (temp_sinr > sinr)
    {
      float bler_val1 = bler_data[mcs].bler_table[i - 1][4] / bler_data[mcs].bler_table[i - 1][5];
      float bler_val2 = bler_data[mcs].bler_table[i][4] / bler_data[mcs].bler_table[i][5];
      return ((bler_val1 + bler_val2) / 2);
    }
  }
  LOG_E(MAC, "NO SINR INDEX FOUND!\n");
  abort();

}

static inline bool is_channel_modeling(void)
{
  /* TODO: For now we enable channel modeling based on the node_number.
     Replace with a command line option to enable/disable channel modeling.
     The LTE UE will crash when channel modeling is conducted for NSA
     mode. It does not crash for LTE mode. We have not implemented channel
     modeling for NSA mode yet. For now, we ensure only do do chanel modeling
     in LTE mode. */
  return node_number == 0 && !get_softmodem_params()->nsa;
}

static bool should_drop_transport_block(int sf, uint16_t rnti)
{
  if (!is_channel_modeling())
  {
    return false;
  }

  /* We want to avoid dropping setup messages because this would be pathological.
     This assumes were in standalone_pnf mode where
     UE_rrc_inst[0] is module_id = 0 and Info[0] is eNB_index = 0. */
  UE_STATE_t state = UE_rrc_inst[0].Info[0].State;
  if (state < RRC_CONNECTED)
  {
    LOG_I(MAC, "Not dropping because state: %d", state);
    return false;
  }

  /* Get block error rate (bler_val) from table based on every saved
     MCS and SINR to be used as the cutoff rate for dropping packets.
     Generate random uniform vairable to compare against bler_val. */
  int pdu_size = sf_rnti_mcs[sf].pdu_size;
  assert(sf < 10 && sf >= 0);
  assert(pdu_size > 0);
  CHECK_INDEX(sf_rnti_mcs[sf].rnti, pdu_size);
  CHECK_INDEX(sf_rnti_mcs[sf].mcs, pdu_size);
  CHECK_INDEX(sf_rnti_mcs[sf].drop_flag, pdu_size);
  int n = 0;
  uint8_t mcs = 99;
  for (n = 0; n < pdu_size; n++)
  {
    if (sf_rnti_mcs[sf].rnti[n] == rnti)
    {
      mcs = sf_rnti_mcs[sf].mcs[n];
    }
    if (mcs != 99)
    {
      /* Use MCS to get the bler value. Since there can be multiple MCS
         values for a particular subframe, we verify that all PDUs are not
         flagged for drop before returning. If even one is flagged for drop
         we return immediately because we drop the entire packet. */
      float bler_val = get_bler_val(mcs, ((int)(sf_rnti_mcs[sf].sinr * 10)));
      double drop_cutoff = ((double) rand() / (RAND_MAX));
      assert(drop_cutoff <= 1);
      LOG_I(MAC, "SINR = %f, Bler_val = %f, MCS = %"PRIu8"\n", sf_rnti_mcs[sf].sinr, bler_val, sf_rnti_mcs[sf].mcs[n]);
      if (drop_cutoff <= bler_val)
      {
        sf_rnti_mcs[sf].drop_flag[n] = true;
        LOG_D(MAC, "We are dropping this packet. Bler_val = %f, MCS = %"PRIu8", sf = %d\n", bler_val, sf_rnti_mcs[sf].mcs[n], sf);
        return sf_rnti_mcs[sf].drop_flag[n];
      }
    }

  }

  if (mcs == 99)
  {
    LOG_E(MAC, "NO MCS Found for rnti %x. sf_rnti_mcs[%d].mcs[%d] \n", rnti, sf, n);
    abort();
  }
  return false;
}
