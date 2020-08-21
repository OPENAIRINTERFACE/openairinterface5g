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

/* \file NR_IF_Module.c
 * \brief functions for NR UE FAPI-like interface
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_nr_UE.h"
#include "NR_IF_Module.h"
#include "NR_MAC_UE/mac_proto.h"
#include "assertions.h"
#include "NR_MAC_UE/mac_extern.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "executables/softmodem-common.h"

#include <stdio.h>

#define MAX_IF_MODULES 100

static nr_ue_if_module_t *nr_ue_if_module_inst[MAX_IF_MODULES];

//  L2 Abstraction Layer
int handle_bcch_bch(module_id_t module_id, int cc_id, unsigned int gNB_index, uint8_t *pduP, unsigned int additional_bits, uint32_t ssb_index, uint32_t ssb_length, uint16_t cell_id){

  return nr_ue_decode_mib(module_id,
			  cc_id,
			  gNB_index,
			  additional_bits,
			  ssb_length,  //  Lssb = 64 is not support    
			  ssb_index,
			  pduP, 
			  cell_id);

}

//  L2 Abstraction Layer
int handle_bcch_dlsch(module_id_t module_id, int cc_id, unsigned int gNB_index, uint32_t sibs_mask, uint8_t *pduP, uint32_t pdu_len){

  return 0;
}
//  L2 Abstraction Layer
int handle_dci(module_id_t module_id, int cc_id, unsigned int gNB_index, fapi_nr_dci_indication_pdu_t *dci){

  //printf("handle_dci: rnti %x,dci_type %d\n",rnti,dci_type);
  return nr_ue_process_dci_indication_pdu(module_id, cc_id, gNB_index, dci);

}
//  L2 Abstraction Layer
int8_t handle_dlsch (module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_indication_t *dci_ind, uint8_t *pduP, uint32_t pdu_len, frame_t frame, int slot, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  LOG_D(MAC, "handle_dlsch at MAC layer \n");
  //if (IS_SOFTMODEM_NOS1 || IS_SOFTMODEM_RFSIM)
  // sdu should be processed even when is S1 mode because data and timing advance updates are transmitted by the UE
  nr_ue_send_sdu(module_id, cc_id, frame, slot,
                 pduP,
                 pdu_len,
                 gNB_index,
                 ul_time_alignment);

  return 0;
  /*
  return nr_ue_process_dlsch(module_id,
                             cc_id,
                             gNB_index,
                             dci_ind,
                             pduP,
                             pdu_len);
  */
}

int8_t handle_rar (nr_downlink_indication_t *dl_info){

  LOG_D(MAC, "handling RAR at MAC layer \n");
  nr_process_rar (dl_info);
  return 0;

}

int nr_ue_ul_indication(nr_uplink_indication_t *ul_info){

  NR_UE_L2_STATE_t ret;
  module_id_t module_id = ul_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  // clean previous FAPI messages
  mac->tx_request.number_of_pdus = 0;
  mac->ul_config_request.number_pdus = 0;
  mac->dl_config_request.number_pdus = 0;
  // clean previous FAPI messages

  ret = nr_ue_scheduler(NULL, ul_info);

  if (is_nr_UL_slot(mac->scc, ul_info->slot_tx) && get_softmodem_params()->do_ra){
    nr_ue_prach_scheduler(module_id, ul_info->frame_tx, ul_info->slot_tx);
  }

  switch(ret){
  case UE_CONNECTION_OK:
    break;
  case UE_CONNECTION_LOST:
    break;
  case UE_PHY_RESYNCH:
    break;
  case UE_PHY_HO_PRACH:
    break;
  default:
    break;
  }

  mac->if_module->scheduled_response(&mac->scheduled_response);

  return 0;
}

int nr_ue_dl_indication(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  int32_t i;
  uint32_t ret_mask = 0x0;
  module_id_t module_id = dl_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
  fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request;

  if (!dl_info->dci_ind && !dl_info->rx_ind) {
    // UL indication to schedule DCI reception
    nr_ue_scheduler(dl_info, NULL);
  } else {
    // UL indication after reception of DCI or DL PDU
    dl_config->number_pdus = 0;
    ul_config->number_pdus = 0;
    //hook up pointers
    mac->scheduled_response.dl_config = dl_config;
    mac->scheduled_response.ul_config = ul_config;
    mac->scheduled_response.tx_request = &mac->tx_request;
    mac->scheduled_response.module_id = dl_info->module_id;
    mac->scheduled_response.CC_id = dl_info->cc_id;
    mac->scheduled_response.frame = dl_info->frame;
    mac->scheduled_response.slot = dl_info->slot;

    if(dl_info->dci_ind != NULL){
      LOG_D(MAC,"[L2][IF MODULE][DL INDICATION][DCI_IND]\n");
      for(i=0; i<dl_info->dci_ind->number_of_dcis; ++i){
        LOG_D(MAC,">>>NR_IF_Module i=%d, dl_info->dci_ind->number_of_dcis=%d\n",i,dl_info->dci_ind->number_of_dcis);

        ret_mask |= (handle_dci(dl_info->module_id,
                                dl_info->cc_id,
                                dl_info->gNB_index,
                                dl_info->dci_ind->dci_list+i)<< FAPI_NR_DCI_IND);

        AssertFatal( nr_ue_if_module_inst[module_id] != NULL, "IF module is void!\n" );
        nr_ue_if_module_inst[module_id]->scheduled_response(&mac->scheduled_response);
      }
    }

    if(dl_info->rx_ind != NULL){

      LOG_D(MAC,"[L2][IF MODULE][DL INDICATION][RX_IND], Number of PDUs: %d \n", dl_info->rx_ind->number_pdus);

      for(i=0; i<dl_info->rx_ind->number_pdus; ++i){

        switch(dl_info->rx_ind->rx_indication_body[i].pdu_type){

        case FAPI_NR_RX_PDU_TYPE_MIB:
          ret_mask |= (handle_bcch_bch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.pdu,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.additional_bits,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.ssb_index,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.ssb_length,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.cell_id)) << FAPI_NR_RX_PDU_TYPE_MIB;

          LOG_D(MAC,"[L2][IF MODULE][DL INDICATION][RX_IND], MIB case Number of PDUs: %d \n", dl_info->rx_ind->number_pdus);

          break;

        case FAPI_NR_RX_PDU_TYPE_SIB:

          ret_mask |= (handle_bcch_dlsch(dl_info->module_id,
                       dl_info->cc_id, dl_info->gNB_index,
                      (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.sibs_mask,
                      (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.pdu,
                      (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.pdu_length)) << FAPI_NR_RX_PDU_TYPE_SIB;

          break;

        case FAPI_NR_RX_PDU_TYPE_DLSCH:

          ret_mask |= (handle_dlsch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
                       dl_info->dci_ind,
                      (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.pdu,
                      (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.pdu_length,
                       dl_info->frame,
                       dl_info->slot,
                       ul_time_alignment)) << FAPI_NR_RX_PDU_TYPE_DLSCH;

          LOG_D(MAC,"[L2][IF MODULE][DL INDICATION][RX_IND], DLSCH case Number of PDUs: %d \n", dl_info->rx_ind->number_pdus);

          break;

        case FAPI_NR_RX_PDU_TYPE_RAR:

          ret_mask |= (handle_rar(dl_info)) << FAPI_NR_RX_PDU_TYPE_RAR;

          break;

        default:
        break;
        }
      }
    }

    //clean up nr_downlink_indication_t *dl_info
    dl_info->rx_ind = NULL;
    dl_info->dci_ind = NULL;

  }
  return 0;
}

nr_ue_if_module_t *nr_ue_if_module_init(uint32_t module_id){

  if (nr_ue_if_module_inst[module_id] == NULL) {
    nr_ue_if_module_inst[module_id] = (nr_ue_if_module_t *)malloc(sizeof(nr_ue_if_module_t));
    memset((void*)nr_ue_if_module_inst[module_id],0,sizeof(nr_ue_if_module_t));

    nr_ue_if_module_inst[module_id]->cc_mask=0;
    nr_ue_if_module_inst[module_id]->current_frame = 0;
    nr_ue_if_module_inst[module_id]->current_slot = 0;
    nr_ue_if_module_inst[module_id]->phy_config_request = nr_ue_phy_config_request;
    nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response;
    nr_ue_if_module_inst[module_id]->dl_indication = nr_ue_dl_indication;
    nr_ue_if_module_inst[module_id]->ul_indication = nr_ue_ul_indication;
  }

  return nr_ue_if_module_inst[module_id];
}

int nr_ue_if_module_kill(uint32_t module_id) {

  if (nr_ue_if_module_inst[module_id] != NULL){
    free(nr_ue_if_module_inst[module_id]);
  } 
  return 0;
}

int nr_ue_dcireq(nr_dcireq_t *dcireq) {
  
  fapi_nr_dl_config_request_t *dl_config=&dcireq->dl_config_req;
  NR_UE_MAC_INST_t *UE_mac = get_mac_inst(0);

  dl_config->sfn=UE_mac->dl_config_request.sfn;
  dl_config->slot=UE_mac->dl_config_request.slot;
  dl_config->number_pdus=0;

  //printf(" UE_mac->dl_config_request.slot %d VS dcireq->slot %d \n", UE_mac->dl_config_request.slot, dcireq->slot);

  LOG_D(PHY, "Entering UE DCI configuration frame %d slot %d \n", dcireq->frame, dcireq->slot);

  ue_dci_configuration(UE_mac, dl_config, dcireq->frame, dcireq->slot);

  return 0;
}
