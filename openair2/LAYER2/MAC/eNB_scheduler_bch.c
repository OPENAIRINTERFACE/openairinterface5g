/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file eNB_scheduler_bch.c
 * \brief procedures related to eNB for the BCH transport channel
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1


// NEED TO ADD schedule_SI_BR for SIB1_BR and SIB23_BR
// CCE_allocation_infeasible to be done for EPDCCH/MPDCCH

//------------------------------------------------------------------------------
void
schedule_SI(
  module_id_t   module_idP,
  frame_t       frameP,
  sub_frame_t   subframeP)

//------------------------------------------------------------------------------
{

  int8_t                         bcch_sdu_length;
  int                            mcs = -1;
  int                            CC_id;
  eNB_MAC_INST                   *eNB = RC.mac[module_idP];
  COMMON_channels_t              *cc;
  uint8_t                        *vrb_map;
  int                            first_rb = -1;
  int                            N_RB_DL;
  nfapi_dl_config_request_pdu_t  *dl_config_pdu;
  nfapi_tx_request_pdu_t         *TX_req;
  nfapi_dl_config_request_body_t *dl_req;
  start_meas(&eNB->schedule_si);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    cc              = &eNB->common_channels[CC_id];
    vrb_map         = (void*)&cc->vrb_map;
    N_RB_DL         = to_prb(cc->mib->message.dl_Bandwidth); 
    dl_req          = &eNB->DL_req[CC_id].dl_config_request_body;


    bcch_sdu_length = mac_rrc_data_req(module_idP,
                                       CC_id,
                                       frameP,
                                       BCCH,1,
                                       &cc->BCCH_pdu.payload[0],
                                       1,
                                       module_idP,
                                       0); // not used in this case

    if (bcch_sdu_length > 0) {
      LOG_D(MAC,"[eNB %d] Frame %d : BCCH->DLSCH CC_id %d, Received %d bytes \n",module_idP,frameP,CC_id,bcch_sdu_length);

      // Allocate 4 PRBs in a random location
      /*
      while (1) {
	first_rb = (unsigned char)(taus()%(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL-4));
	if ((vrb_map[first_rb] != 1) && 
	    (vrb_map[first_rb+1] != 1) && 
	    (vrb_map[first_rb+2] != 1) && 
	    (vrb_map[first_rb+3] != 1))
	  break;
      }
      */
      switch (N_RB_DL) {
      case 6:
	first_rb = 0;
	break;
      case 15:
	first_rb = 6;
	break;
      case 25:
	first_rb = 11;
	break;
      case 50:
	first_rb = 23;
	break;
      case 100:
	first_rb = 48;
	break;
      }

      vrb_map[first_rb] = 1;
      vrb_map[first_rb+1] = 1;
      vrb_map[first_rb+2] = 1;
      vrb_map[first_rb+3] = 1;

      // Get MCS for length of SI, 3 PRBs
      if (bcch_sdu_length <= 7) {
        mcs=0;
      } else if (bcch_sdu_length <= 11) {
        mcs=1;
      } else if (bcch_sdu_length <= 18) {
        mcs=2;
      } else if (bcch_sdu_length <= 22) {
        mcs=3;
      } else if (bcch_sdu_length <= 26) {
        mcs=4;
      } else if (bcch_sdu_length <= 28) {
        mcs=5;
      } else if (bcch_sdu_length <= 32) {
        mcs=6;
      } else if (bcch_sdu_length <= 41) {
        mcs=7;
      } else if (bcch_sdu_length <= 49) {
        mcs=8;
      }


      dl_config_pdu                                                         = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
      memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                               = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
      dl_config_pdu->pdu_size                                               = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level           = 4;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                        = 0xFFFF;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                   = 2;    // S-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power          = 6000; // equal to RS power
      
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                = 0;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                         = 1; // no TPC
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1        = 1;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = mcs;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1        = 0;

      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding       = getRIV(N_RB_DL,first_rb,4);      

      if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,SI_RNTI)) {
	LOG_I(MAC,"Frame %d: Subframe %d : Adding common DCI for S_RNTI\n",
	      frameP,subframeP);
	dl_req->number_dci++;
	dl_req->number_pdu++;
	dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
	memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE; 
	dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_id];
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = 0xFFFF;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;   // format 1A/1B/1D
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;   // localized
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL,first_rb,4);
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation                             = 2; //QPSK
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version                     = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks                       = 1;// first block
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag  = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme                    = (cc->p_eNB==1 ) ? 0 : 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers                       = 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands                     = 1;
	//	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity                   = 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa                                     = 4; // 0 dB
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index               = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap                                   = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb                                   = get_subbandsize(cc->mib->message.dl_Bandwidth); // ignored
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode                      = (cc->p_eNB==1 ) ? 1 : 2;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband                 = 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector                          = 1;
	//	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ; 
	dl_req->number_pdu++;

	// Program TX Request
	TX_req                                                                = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus]; 
	TX_req->pdu_length                                                    = bcch_sdu_length;
	TX_req->pdu_index                                                     = eNB->pdu_index[CC_id]++;
	TX_req->num_segments                                                  = 1;
	TX_req->segments[0].segment_length                                    = bcch_sdu_length;
	TX_req->segments[0].segment_data                                      = cc->BCCH_pdu.payload;
	eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;

      }
      else {
	LOG_E(MAC,"[eNB %d] CCid %d Frame %d, subframe %d : Cannot add DCI 1A for SI\n",module_idP, CC_id,frameP,subframeP);
      }

      if (opt_enabled == 1) {
        trace_pdu(1,
                  &cc->BCCH_pdu.payload[0],
                  bcch_sdu_length,
                  0xffff,
                  4,
                  0xffff,
                  eNB->frame,
                  eNB->subframe,
                  0,
                  0);
	LOG_D(OPT,"[eNB %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
	    module_idP, frameP, CC_id, 0xffff, bcch_sdu_length);
      }
      if (cc->tdd_Config!=NULL) { //TDD
        LOG_D(MAC,"[eNB] Frame %d : Scheduling BCCH->DLSCH (TDD) for CC_id %d SI %d bytes (mcs %d, rb 3)\n",
              frameP,
              CC_id,
              bcch_sdu_length,
              mcs);
      } else {
        LOG_D(MAC,"[eNB] Frame %d : Scheduling BCCH->DLSCH (FDD) for CC_id %d SI %d bytes (mcs %d, rb 3)\n",
              frameP,
              CC_id,
              bcch_sdu_length,
              mcs);
      }


      eNB->eNB_stats[CC_id].total_num_bcch_pdu+=1;
      eNB->eNB_stats[CC_id].bcch_buffer=bcch_sdu_length;
      eNB->eNB_stats[CC_id].total_bcch_buffer+=bcch_sdu_length;
      eNB->eNB_stats[CC_id].bcch_mcs=mcs;
    } else {

      //LOG_D(MAC,"[eNB %d] Frame %d : BCCH not active \n",Mod_id,frame);
    }
  }

  // this might be misleading when bcch is inactive
  stop_meas(&eNB->schedule_si);
  return;
}

void schedule_mib(module_id_t   module_idP,
		  frame_t       frameP,
		  sub_frame_t   subframeP) {

  eNB_MAC_INST                   *eNB = RC.mac[module_idP];
  COMMON_channels_t              *cc;
  nfapi_dl_config_request_pdu_t  *dl_config_pdu;
  nfapi_tx_request_pdu_t         *TX_req;
  int mib_sdu_length;
  int CC_id;
  nfapi_dl_config_request_body_t *dl_req;

  AssertFatal(subframeP==0,"Subframe must be 0\n");
  AssertFatal((frameP&3)==0,"Frame must be a multiple of 4\n");

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    dl_req          = &eNB->DL_req[CC_id].dl_config_request_body;
    cc              = &eNB->common_channels[CC_id];

    mib_sdu_length = mac_rrc_data_req(module_idP,
				      CC_id,
				      frameP,
				      MIBCH,1,
				      &cc->MIB_pdu.payload[0],
				      1,
				      module_idP,
				      0); // not used in this case

    LOG_I(MAC,"Frame %d, subframe %d: BCH PDU length %d\n",
	  frameP,subframeP,mib_sdu_length);

    if (mib_sdu_length > 0) {

      LOG_I(MAC,"Frame %d, subframe %d: Adding BCH PDU in position %d (length %d)\n",
	    frameP,subframeP,dl_req->number_pdu,mib_sdu_length);

      dl_config_pdu                                                         = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
      memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                               = NFAPI_DL_CONFIG_BCH_PDU_TYPE,
      dl_config_pdu->pdu_size                                               = 2+sizeof(nfapi_dl_config_bch_pdu);
      dl_config_pdu->bch_pdu.bch_pdu_rel8.length                            = mib_sdu_length;
      dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index                         = eNB->pdu_index[CC_id];
      dl_config_pdu->bch_pdu.bch_pdu_rel8.transmission_power                = 6000;
      dl_req->number_pdu++;

      LOG_I(MAC,"eNB->DL_req[0].number_pdu %d (%p)\n",
	    dl_req->number_pdu,&dl_req->number_pdu);
      // DL request

      TX_req                                                                = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus]; 
      TX_req->pdu_length                                                    = 3;
      TX_req->pdu_index                                                     = eNB->pdu_index[CC_id]++;
      TX_req->num_segments                                                  = 1;
      TX_req->segments[0].segment_length                                    = 0;
      TX_req->segments[0].segment_data                                      = cc[CC_id].MIB_pdu.payload;
      eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
    }
  }
}

 
