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
#include "mac_proto.h"
#include "assertions.h"
#include "LAYER2/NR_MAC_UE/mac_extern.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "executables/nr-softmodem.h"

#include <stdio.h>

#define MAX_IF_MODULES 100

static nr_ue_if_module_t *nr_ue_if_module_inst[MAX_IF_MODULES];

//  L2 Abstraction Layer
int handle_bcch_bch(UE_nr_rxtx_proc_t *proc, module_id_t module_id, int cc_id, unsigned int gNB_index, uint8_t *pduP, unsigned int additional_bits, uint32_t ssb_index, uint32_t ssb_length, uint16_t cell_id){

  return nr_ue_decode_mib( proc,
		           module_id,
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
int handle_dci(module_id_t module_id, int cc_id, unsigned int gNB_index, fapi_nr_dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_type){

  //printf("handle_dci: rnti %x,dci_type %d\n",rnti,dci_type);
  return nr_ue_process_dci(module_id, cc_id, gNB_index, dci, rnti, dci_type);

}
//  L2 Abstraction Layer
int8_t handle_dlsch (module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_indication_t *dci_ind, uint8_t *pduP, uint32_t pdu_len, frame_t frame, int slot, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  if (IS_SOFTMODEM_NOS1 || IS_SOFTMODEM_RFSIM)
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

int nr_ue_ul_indication(nr_uplink_indication_t *ul_info){

  NR_UE_L2_STATE_t ret;
  module_id_t module_id = ul_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  // clean previous FAPI messages
  mac->tx_request.number_of_pdus = 0;
  mac->ul_config_request.number_pdus = 0;
  mac->dl_config_request.number_pdus = 0;
  // clean previous FAPI messages

  ret = nr_ue_scheduler(
			ul_info->module_id,
			ul_info->gNB_index,
			ul_info->cc_id,
			ul_info->frame,
			ul_info->slot,
			ul_info->ssb_index, 
			0, 0); //  TODO check tx/rx frame/slot is need for NR version

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
      fapi_nr_dci_pdu_rel15_t *dci = &dl_info->dci_ind->dci_list[i].dci;

      ret_mask |= (handle_dci(
			      dl_info->module_id,
			      dl_info->cc_id,
			      dl_info->gNB_index,
			      dci, 
			      (dl_info->dci_ind->dci_list+i)->rnti, 
			      (dl_info->dci_ind->dci_list+i)->dci_format)) << FAPI_NR_DCI_IND;

      AssertFatal( nr_ue_if_module_inst[module_id] != NULL, "IF module is void!\n" );
      nr_ue_if_module_inst[module_id]->scheduled_response(&mac->scheduled_response);


      /*switch((dl_info->dci_ind->dci_list+i)->dci_type){
	case FAPI_NR_DCI_TYPE_0_0:
	case FAPI_NR_DCI_TYPE_0_1:
	case FAPI_NR_DCI_TYPE_1_1:
	case FAPI_NR_DCI_TYPE_2_0:
	case FAPI_NR_DCI_TYPE_2_1:
	case FAPI_NR_DCI_TYPE_2_2:
	case FAPI_NR_DCI_TYPE_2_3:
	AssertFatal(1==0, "Not yet support at this moment!\n");
	break;
                
	case FAPI_NR_DCI_TYPE_1_0:
                    
	dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;

	//  mapping into DL_CONFIG_REQ for DL-SCH
	fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
                    
	dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = 0x0000;   //  TX RNTI: UE-spec
	memcpy(dlsch_config_pdu, dci, sizeof(fapi_nr_dci_pdu_rel15_t));

	dl_config->number_pdus = dl_config->number_pdus + 1;

	ret_mask |= (handle_dci(
	dl_info->module_id,
	dl_info->cc_id,
	dl_info->gNB_index,
	dci, 
	(dl_info->dci_ind->dci_list+i)->rnti, 
	(dl_info->dci_ind->dci_list+i)->dci_type)) << FAPI_NR_DCI_IND;

                    

	break;

	default:
	break;
	}*/

      //(dl_info->dci_list+i)->rnti
            
            


    }
  }

  if(dl_info->rx_ind != NULL){
    LOG_I(MAC,"[L2][IF MODULE][DL INDICATION][RX_IND], Number of PDUs: %d \n", dl_info->rx_ind->number_pdus);
    for(i=0; i<dl_info->rx_ind->number_pdus; ++i){
      switch(dl_info->rx_ind->rx_indication_body[i].pdu_type){
      case FAPI_NR_RX_PDU_TYPE_MIB:
	ret_mask |= (handle_bcch_bch( dl_info->proc,
				     dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
				     (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.pdu,
				     (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.additional_bits,
				     (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.ssb_index,
				     (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.ssb_length,
				     (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.cell_id )) << FAPI_NR_RX_PDU_TYPE_MIB;
    	  LOG_I(MAC,"[L2][IF MODULE][DL INDICATION][RX_IND], MIB case Number of PDUs: %d \n", dl_info->rx_ind->number_pdus);
	/*ret_mask |= (handle_bcch_bch( dl_info->proc,
					     dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
					     dl_info->rx_ind->rx_indication_body[i].mib_pdu.pdu,
					     dl_info->rx_ind->rx_indication_body[i].mib_pdu.additional_bits,
					     dl_info->rx_ind->rx_indication_body[i].mib_pdu.ssb_index,
					     dl_info->rx_ind->rx_indication_body[i].mib_pdu.ssb_length,
					     dl_info->rx_ind->rx_indication_body[i].mib_pdu.cell_id )) << FAPI_NR_RX_PDU_TYPE_MIB;*/
	break;
      case FAPI_NR_RX_PDU_TYPE_SIB:
	ret_mask |= (handle_bcch_dlsch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
				       (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.sibs_mask,
				       (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.pdu,
				       (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.pdu_length )) << FAPI_NR_RX_PDU_TYPE_SIB;
	break;
      case FAPI_NR_RX_PDU_TYPE_DLSCH:
	//                    ret_mask |= (0) << FAPI_NR_RX_PDU_TYPE_DLSCH;
	ret_mask |= (handle_dlsch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index, dl_info->dci_ind,
				  (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.pdu,
				  (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.pdu_length, dl_info->frame, dl_info->slot,ul_time_alignment)) << FAPI_NR_RX_PDU_TYPE_DLSCH;

    	  LOG_I(MAC,"[L2][IF MODULE][DL INDICATION][RX_IND], DLSCH case Number of PDUs: %d \n", dl_info->rx_ind->number_pdus);

	  /*
	  // dl_config structure just stores what was received - not really needed
	  dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
	  dl_config->number_pdus = dl_config->number_pdus + 1;
	  */

	/*ret_mask |= (handle_dlsch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index, dl_info->dci_ind,
					  dl_info->rx_ind->rx_indication_body[i].pdsch_pdu.pdu,
					  dl_info->rx_ind->rx_indication_body[i].pdsch_pdu.pdu_length, dl_info->frame, dl_info->slot)) << FAPI_NR_RX_PDU_TYPE_DLSCH;

	*/
	break;
      default:

	break;

      }
    }
  }

  //clean up nr_downlink_indication_t *dl_info
  dl_info->rx_ind = NULL;
  dl_info->dci_ind = NULL;

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
  
  //  Type0 PDCCH search space
  dl_config->number_pdus =  1;
  dl_config->dl_config_list[0].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.rnti = 0x1234;	//	to be set
  
  uint64_t mask = 0x0;
  uint16_t num_rbs=24;
  uint16_t rb_offset=0;
  uint16_t cell_id=0;
  uint16_t num_symbols=2;
  for(int i=0; i<(num_rbs/6); ++i){   //  38.331 Each bit corresponds a group of 6 RBs
    mask = mask >> 1;
    mask = mask | 0x100000000000;
  }
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.frequency_domain_resource = mask;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.rb_offset = rb_offset;  //  additional parameter other than coreset
  
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.duration = num_symbols;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.cce_reg_mapping_type =CCE_REG_MAPPING_TYPE_NON_INTERLEAVED;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.cce_reg_interleaved_reg_bundle_size = 0;   //  L 38.211 7.3.2.2
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.cce_reg_interleaved_interleaver_size = 0;  //  R 38.211 7.3.2.2
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.cce_reg_interleaved_shift_index = cell_id;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.precoder_granularity = PRECODER_GRANULARITY_SAME_AS_REG_BUNDLE;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.pdcch_dmrs_scrambling_id = cell_id;
  
  uint32_t number_of_search_space_per_slot=1;
  uint32_t first_symbol_index=0;
  uint32_t search_space_duration=0;  //  element of search space
  uint32_t coreset_duration;  //  element of coreset
  
  coreset_duration = num_symbols * number_of_search_space_per_slot;
  
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.number_of_candidates[0] = table_38213_10_1_1_c2[0];
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.number_of_candidates[1] = table_38213_10_1_1_c2[1];
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.number_of_candidates[2] = table_38213_10_1_1_c2[2];   //  CCE aggregation level = 4
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.number_of_candidates[3] = table_38213_10_1_1_c2[3];   //  CCE aggregation level = 8
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.number_of_candidates[4] = table_38213_10_1_1_c2[4];   //  CCE aggregation level = 16
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.duration = search_space_duration;
  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.monitoring_symbols_within_slot = (0x3fff << first_symbol_index) & (0x3fff >> (14-coreset_duration-first_symbol_index)) & 0x3fff;

  dl_config->dl_config_list[0].dci_config_pdu.dci_config_rel15.N_RB_BWP = UE_mac->initial_bwp_dl.N_RB;

  
  return 0;
}
