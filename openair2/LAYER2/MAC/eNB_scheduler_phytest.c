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

/*! \file eNB_scheduler_dlsch.c
 * \brief procedures related to eNB for the DLSCH transport channel
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "SCHED/sched_eNB.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "SIMULATION/TOOLS/sim.h" // for taus

#include "T.h"

extern RAN_CONTEXT_t RC;

//------------------------------------------------------------------------------
void
schedule_ue_spec_phy_test(
  module_id_t   module_idP,
  frame_t       frameP,
  sub_frame_t   subframeP,
  int*          mbsfn_flag
)
//------------------------------------------------------------------------------
{
  uint8_t                        CC_id;
  int                            UE_id=0;
  uint16_t                       N_RB_DL;
  uint16_t                       TBS;
  uint16_t                       nb_rb;

  unsigned char                  harq_pid  = (frameP*10+subframeP)%8;
  uint16_t                       rnti      = 0x1235;
  uint32_t                       rb_alloc  = 0x1FFFFF;
  int32_t                        tpc       = 1;
  int32_t                        mcs       = 28;
  int32_t                        cqi       = 15;
  int32_t                        ndi       = (frameP*10+subframeP)/8;
  int32_t                        dai       = 0;

  eNB_MAC_INST                   *eNB      = RC.mac[module_idP];
  COMMON_channels_t              *cc       = eNB->common_channels;
  nfapi_dl_config_request_body_t *dl_req;
  nfapi_dl_config_request_pdu_t  *dl_config_pdu;

  N_RB_DL         = to_prb(cc->mib->message.dl_Bandwidth);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d\n",CC_id);

    dl_req        = &eNB->DL_req[CC_id].dl_config_request_body;

    if (mbsfn_flag[CC_id]>0)
      continue;

    nb_rb = conv_nprb(0,rb_alloc,N_RB_DL);
    TBS = get_TBS_DL(mcs,nb_rb);

    LOG_D(MAC,"schedule_ue_spec_phy_test: subframe %d/%d: nb_rb=%d, TBS=%d, mcs=%d harq_pid=%d (rb_alloc=%x, N_RB_DL=%d) pdu_number = %d \n", frameP, subframeP, nb_rb, TBS, mcs, harq_pid, rb_alloc, N_RB_DL, dl_req->number_pdu);

    dl_config_pdu                                                         = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
    memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
    dl_config_pdu->pdu_type                                               = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
    dl_config_pdu->pdu_size                                               = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level           = get_aggregation(get_bw_index(module_idP,CC_id),cqi,format1);
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type    = 0;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding       = rb_alloc;

    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                        = rnti;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                   = 1;    // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power          = 6000; // equal to RS power
    
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                = harq_pid;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                         = tpc; // dont adjust power when retransmitting
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1        = ndi;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = mcs;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1        = 0;
    //deactivate second codeword
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_2                       = 0;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2        = 1;
    
    if (cc[CC_id].tdd_Config != NULL) { //TDD
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = dai;
      LOG_D(MAC,"[eNB %d] Initial transmission CC_id %d : harq_pid %d, dai %d, mcs %d\n",
	    module_idP,CC_id,harq_pid,dai,mcs);
    } else {
      LOG_D(MAC,"[eNB %d] Initial transmission CC_id %d : harq_pid %d, mcs %d\n",
	    module_idP,CC_id,harq_pid,mcs);
      
    }
    LOG_D(MAC,"Checking feasibility pdu %d (new sdu)\n",dl_req->number_pdu);
    if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,rnti)) {
      
      
      //ue_sched_ctl->round[CC_id][harq_pid] = 0;
      dl_req->number_dci++;
      dl_req->number_pdu++;
      eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
      //eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;
      
      // Toggle NDI for next time
      /*
      LOG_D(MAC,"CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
	    CC_id, frameP,subframeP,UE_id,
	    rnti,harq_pid,UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]);
      
      UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]=1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
      UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid] = mcs;
      UE_list->UE_template[CC_id][UE_id].oldmcs2[harq_pid] = 0;
      AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated!=NULL,"physicalConfigDedicated is NULL\n");
      AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated!=NULL,"physicalConfigDedicated->pdsch_ConfigDedicated is NULL\n");
      */

      
      fill_nfapi_dlsch_config(eNB,
			      dl_req,
			      TBS,
			      eNB->pdu_index[CC_id],
			      rnti,
			      0, // type 0 allocation from 7.1.6 in 36.213
			      0, // virtual_resource_block_assignment_flag
			      rb_alloc, // resource_block_coding
			      getQm(mcs),
			      0, // redundancy version
			      1, // transport blocks
			      0, // transport block to codeword swap flag
			      cc[CC_id].p_eNB == 1 ? 0 : 1, // transmission_scheme
			      1, // number of layers
			      1, // number of subbands
			      //			     uint8_t codebook_index,
			      4, // UE category capacity
			      LTE_PDSCH_ConfigDedicated__p_a_dB0,
			      0, // delta_power_offset for TM5
			      0, // ngap
			      0, // nprb
			      cc[CC_id].p_eNB == 1 ? 1 : 2, // transmission mode
			      0, //number of PRBs treated as one subband, not used here
			      0 // number of beamforming vectors, not used here
			      );  

      eNB->TX_req[CC_id].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_id].tx_request_body,
						    (frameP*10)+subframeP,
						    TBS,
						    eNB->pdu_index[CC_id],
						    eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0]);
    }
    else {
      LOG_W(MAC,"[eNB_scheduler_phytest] DCI allocation infeasible!\n");
    }
  }
}

void schedule_ulsch_phy_test(module_id_t module_idP,frame_t frameP,sub_frame_t subframeP)
{
  uint16_t first_rb[MAX_NUM_CCs];
  int               UE_id = 0;
  uint8_t           aggregation    = 2;
  rnti_t            rnti           = 0x1235;
  uint8_t           mcs            = 20;
  uint8_t           harq_pid       = 0;
  uint32_t          cqi_req = 0,cshift,ndi,tpc = 1;
  int32_t           snr;
  int32_t           target_snr = 10; /* TODO: target_rx_power was 178, what to put? is it meaningful? */
  int               CC_id = 0;
  int               nb_rb = 24;
  int               N_RB_UL;
  eNB_MAC_INST      *mac = RC.mac[module_idP];
  COMMON_channels_t *cc  = &mac->common_channels[0];
  UE_list_t         *UE_list=&mac->UE_list;
  UE_TEMPLATE       *UE_template;
  UE_sched_ctrl_t     *UE_sched_ctrl;
  int               sched_frame=frameP;
  int               sched_subframe = (subframeP+4)%10;
  uint16_t          ul_req_index;

  if (sched_subframe<subframeP) sched_frame++;

  nfapi_hi_dci0_request_t        *hi_dci0_req = &mac->HI_DCI0_req[CC_id][subframeP];
  nfapi_hi_dci0_request_body_t   *hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;

  //nfapi_ul_config_request_pdu_t  *ul_config_pdu = &ul_req->ul_config_pdu_list[0];;
  nfapi_ul_config_request_body_t *ul_req       = &mac->UL_req[CC_id].ul_config_request_body;

  N_RB_UL         = to_prb(cc->mib->message.dl_Bandwidth);
  switch(N_RB_UL){
  case 100:
    nb_rb = 96;
    break;
  case 50:
    nb_rb = 48;
    break;
  case 25:
    nb_rb = 24;
    break;
  }

  mac->UL_req[CC_id].sfn_sf   = (sched_frame<<4) + sched_subframe;
  hi_dci0_req->sfn_sf = (frameP << 4) + subframeP;
  
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    //rnti = UE_RNTI(module_idP,UE_id);
    //leave out first RB for PUCCH
    first_rb[CC_id] = 1;
  // loop over all active UEs

      //      if (eNB_UE_stats->mode == PUSCH) { // ue has a ulsch channel

      UE_template   = &UE_list->UE_template[CC_id][UE_id];
      UE_sched_ctrl = &UE_list->UE_sched_ctrl[UE_id];
      harq_pid      = subframe2harqpid(&cc[CC_id],sched_frame,sched_subframe);


      RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP] = UE_template->TBS_UL[harq_pid];

	  

      //power control
      //compute the expected ULSCH RX power (for the stats)
	  
      // this is the snr and this should be constant (regardless of mcs)
      snr = (5 * UE_sched_ctrl->pusch_snr[CC_id] - 640) / 10;
	  
      // new transmission
	  
      ndi = 1-UE_template->oldNDI_UL[harq_pid];
      UE_template->oldNDI_UL[harq_pid]=ndi;
	  UE_list->eNB_UE_stats[CC_id][UE_id].snr = snr;
	  UE_list->eNB_UE_stats[CC_id][UE_id].target_snr = target_snr;
	  UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1 = mcs;
      UE_template->mcs_UL[harq_pid] = mcs;//cmin (UE_template->pre_assigned_mcs_ul, openair_daq_vars.target_ue_ul_mcs); // adjust, based on user-defined MCS
      UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2 = mcs;
	  //            buffer_occupancy = UE_template->ul_total_buffer;

	    
	    
      UE_template->TBS_UL[harq_pid] = get_TBS_UL(mcs,nb_rb);
	  UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx += nb_rb;
	  UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS = get_TBS_UL(mcs,nb_rb);
	  //            buffer_occupancy -= TBS;


	    
	  // bad indices : 20 (40 PRB), 21 (45 PRB), 22 (48 PRB)
      //store for possible retransmission
      UE_template->nb_rb_ul[harq_pid]    = nb_rb;
      UE_template->first_rb_ul[harq_pid] = first_rb[CC_id];
	    
	  UE_sched_ctrl->ul_scheduled |= (1<<harq_pid);
	    
	  // adjust total UL buffer status by TBS, wait for UL sdus to do final update
	  //UE_template->ul_total_buffer = UE_template->TBS_UL[harq_pid];
	  // Cyclic shift for DM RS
	  cshift = 0;// values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
	  // save it for a potential retransmission
      UE_template->cshift[harq_pid] = cshift;	    

	  hi_dci0_pdu                                                         = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
	  memset((void*)hi_dci0_pdu,0,sizeof(nfapi_hi_dci0_request_pdu_t));
	  hi_dci0_pdu->pdu_type                                               = NFAPI_HI_DCI0_DCI_PDU_TYPE; 
	  hi_dci0_pdu->pdu_size                                               = 2+sizeof(nfapi_hi_dci0_dci_pdu);
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dci_format                        = NFAPI_UL_DCI_FORMAT_0;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level                 = aggregation;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti                              = rnti;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.transmission_power                = 6000;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.resource_block_start              = first_rb[CC_id];
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.number_of_resource_block          = nb_rb;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.mcs_1                             = mcs;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms           = cshift;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag    = 0;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.new_data_indication_1             = ndi;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tpc                               = tpc;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cqi_csi_request                   = cqi_req;
	  hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dl_assignment_index               = UE_template->DAI_ul[sched_subframe];
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.harq_pid                          = harq_pid;


	  hi_dci0_req_body->number_of_dci++;
	    
	  ul_req_index = 0;
            for(ul_req_index = 0;ul_req_index < ul_req->number_of_pdus;ul_req_index++){
              if(ul_req->ul_config_pdu_list[ul_req_index].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE){
                LOG_D(MAC,"Frame %d, Subframe %d:rnti %x ul_req_index %d Switched UCI HARQ to ULSCH HARQ(first)\n",frameP,subframeP,rnti,ul_req_index);
                break;
              }
            }

	  // Add UL_config PDUs
	  fill_nfapi_ulsch_config_request_rel8(& ul_req->ul_config_pdu_list[ul_req_index],
						 cqi_req,
						 cc,
						 0,//UE_template->physicalConfigDedicated,
						 get_tmode(module_idP,CC_id,UE_id),
						 mac->ul_handle,//eNB->ul_handle,
						 rnti,
						 first_rb[CC_id], // resource_block_start
						 nb_rb, // number_of_resource_blocks
						 mcs,
						 cshift, // cyclic_shift_2_for_drms
						 0, // frequency_hopping_enabled_flag
						 0, // frequency_hopping_bits
						 ndi, // new_data_indication
						 0, // redundancy_version
						 harq_pid, // harq_process_number
						 0, // ul_tx_mode
						 0, // current_tx_nb
						 0, // n_srs
						 get_TBS_UL(mcs,nb_rb)
						 );
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
	  if (UE_template->rach_resource_type>0) { // This is a BL/CE UE allocation
	    fill_nfapi_ulsch_config_request_emtc(&ul_req->ul_config_pdu_list[ul_req_index],
						   UE_template->rach_resource_type>2 ? 2 : 1,
						   1, //total_number_of_repetitions
						   1, //repetition_number
						   (frameP*10)+subframeP);
	    }
#endif
	  ul_req->number_of_pdus = 1;
	  mac->ul_handle++;
	    
	    
	  	
	  add_ue_ulsch_info(module_idP,
			      CC_id,
			      UE_id,
			      subframeP,
			      S_UL_SCHEDULED);
	    
            // increment first rb for next UE allocation
       first_rb[CC_id]+= nb_rb;
	    
	  	  
  } // loop of CC_id
}
