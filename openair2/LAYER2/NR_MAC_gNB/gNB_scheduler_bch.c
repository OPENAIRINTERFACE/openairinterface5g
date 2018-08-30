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

/*! \file gNB_scheduler_bch.c
 * \brief procedures related to eNB for the BCH transport channel
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 * @ingroup _mac

 */

#include "assertions.h"
#include "LAYER2/NR_MAC_gNB/mac.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/NR/nr_rrc_extern.h"


//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP){

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc;
  
  nfapi_nr_dl_config_request_t      *dl_config_request;
  nfapi_nr_dl_config_request_body_t *dl_req;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_pdu;
  nfapi_tx_request_pdu_t            *TX_req;

  int mib_sdu_length;
  int CC_id;

  uint16_t sfn_sf = frameP << 4 | subframeP;

  AssertFatal(subframeP == 0, "Subframe must be 0\n");
  AssertFatal((frameP & 7) == 0, "Frame must be a multiple of 8\n");

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    dl_config_request = &gNB->DL_req[CC_id];
    dl_req = &dl_config_request->dl_config_request_body;
    cc = &gNB->common_channels[CC_id];

    mib_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, MIBCH, 1, &cc->MIB_pdu.payload[0]); // not used in this case

    LOG_D(MAC, "Frame %d, subframe %d: BCH PDU length %d\n", frameP, subframeP, mib_sdu_length);

    if (mib_sdu_length > 0) {

      LOG_D(MAC, "Frame %d, subframe %d: Adding BCH PDU in position %d (length %d)\n", frameP, subframeP, dl_req->number_pdu, mib_sdu_length);

      if ((frameP & 1023) < 80){
        LOG_D(MAC,"[gNB %d] Frame %d : MIB->BCH  CC_id %d, Received %d bytes\n",module_idP, frameP, CC_id, mib_sdu_length);
      }

      dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_nr_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type      = NFAPI_NR_DL_CONFIG_BCH_PDU_TYPE;
      dl_config_pdu->pdu_size      =2 + sizeof(nfapi_nr_dl_config_bch_pdu_rel15_t);
      dl_config_pdu->bch_pdu.bch_pdu_rel15.tl.tag             = NFAPI_NR_DL_CONFIG_REQUEST_BCH_PDU_REL15_TAG;
      dl_config_pdu->bch_pdu.bch_pdu_rel15.length             = mib_sdu_length;
      dl_config_pdu->bch_pdu.bch_pdu_rel15.pdu_index          = gNB->pdu_index[CC_id];
      dl_config_pdu->bch_pdu.bch_pdu_rel15.transmission_power = 6000;
      dl_req->tl.tag                            = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
      dl_req->number_pdu++;
      dl_config_request->header.message_id = NFAPI_DL_CONFIG_REQUEST;
      dl_config_request->sfn_sf = sfn_sf;

      LOG_D(MAC, "gNB->DL_req[0].number_pdu %d (%p)\n", dl_req->number_pdu, &dl_req->number_pdu);
      // DL request

      TX_req = &gNB->TX_req[CC_id].tx_request_body.tx_pdu_list[gNB->TX_req[CC_id].tx_request_body.number_of_pdus];
      TX_req->pdu_length = 3;
      TX_req->pdu_index = gNB->pdu_index[CC_id]++;
      TX_req->num_segments = 1;
      TX_req->segments[0].segment_length = 3;
      TX_req->segments[0].segment_data = cc[CC_id].MIB_pdu.payload;
      gNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
      gNB->TX_req[CC_id].sfn_sf = sfn_sf;
      gNB->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
      gNB->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
    }
  }
}


