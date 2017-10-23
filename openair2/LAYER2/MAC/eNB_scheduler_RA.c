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

/*! \file eNB_scheduler_RA.c
 * \brief primitives used for random access
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "platform_types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "msc.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"

#include "LAYER2/MAC/proto.h"
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

#include "SIMULATION/TOOLS/defs.h" // for taus

#include "T.h"

void add_msg3(module_id_t module_idP,int CC_id, RA_TEMPLATE *RA_template, frame_t frameP, sub_frame_t subframeP) {

  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  COMMON_channels_t               *cc  = &eNB->common_channels[CC_id];
  uint8_t                         j;
  nfapi_ul_config_request_t      *ul_req;
  nfapi_ul_config_request_pdu_t  *ul_config_pdu;
  nfapi_ul_config_request_body_t *ul_req_body;
  nfapi_hi_dci0_request_body_t   *hi_dci0_req;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;

  uint8_t rvseq[4] = {0,2,3,1};


  hi_dci0_req   = &eNB->HI_DCI0_req[CC_id].hi_dci0_request_body;
  ul_req        = &eNB->UL_req_tmp[CC_id][RA_template->Msg3_subframe];
  ul_req_body   = &ul_req->ul_config_request_body;
  AssertFatal(RA_template->RA_active == TRUE,"RA is not active for RA %X\n",RA_template->rnti);

#ifdef Rel14
  if (RA_template->rach_resource_type>0) {
    LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d CE level %d is active, Msg3 in (%d,%d)\n",
	  module_idP,frameP,subframeP,CC_id,RA_template->rach_resource_type-1,
	  RA_template->Msg3_frame,RA_template->Msg3_subframe);
    LOG_D(MAC,"Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d)\n",
	  frameP,subframeP,RA_template->Msg3_frame,RA_template->Msg3_subframe,RA_template->msg3_nb_rb,RA_template->msg3_round);

    ul_config_pdu                                                                  = &ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus]; 
    
    memset((void*)ul_config_pdu,0,sizeof(nfapi_ul_config_request_pdu_t));
    ul_config_pdu->pdu_type                                                        = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE; 
    ul_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_ul_config_ulsch_pdu));
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.handle                                 = eNB->ul_handle++;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti                                   = RA_template->rnti;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.resource_block_start                   = narrowband_to_first_rb(cc,RA_template->msg34_narrowband)+RA_template->msg3_first_rb;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks              = RA_template->msg3_nb_rb;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.modulation_type                        = 2;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.cyclic_shift_2_for_drms                = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_enabled_flag         = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_bits                 = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.new_data_indication                    = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.redundancy_version                     = rvseq[RA_template->msg3_round];
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number                    = ((10*RA_template->Msg3_frame)+RA_template->Msg3_subframe)&7;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.ul_tx_mode                             = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.current_tx_nb                          = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.n_srs                                  = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size                                   = get_TBS_UL(RA_template->msg3_mcs,
												RA_template->msg3_nb_rb);
    // Re13 fields
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.ue_type                               = RA_template->rach_resource_type>2 ? 2 : 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.total_number_of_repetitions           = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.repetition_number                     = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.initial_transmission_sf_io            = (RA_template->Msg3_frame*10)+RA_template->Msg3_subframe;
    ul_req_body->number_of_pdus++;
  } //  if (RA_template->rach_resource_type>0) {	 
  else
#endif
    {
      LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d RA is active, Msg3 in (%d,%d)\n",
	    module_idP,frameP,subframeP,CC_id,RA_template->Msg3_frame,RA_template->Msg3_subframe);
	    
      LOG_D(MAC,"Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d,%d)\n",
	    frameP,subframeP,RA_template->Msg3_frame,RA_template->Msg3_subframe,
	    RA_template->msg3_nb_rb,RA_template->msg3_first_rb,RA_template->msg3_round);
      
      ul_config_pdu                                                                  = &ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus]; 
      
      memset((void*)ul_config_pdu,0,sizeof(nfapi_ul_config_request_pdu_t));
      ul_config_pdu->pdu_type                                                        = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE; 
      ul_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_ul_config_ulsch_pdu));
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.handle                                 = eNB->ul_handle++;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti                                   = RA_template->rnti;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.resource_block_start                   = RA_template->msg3_first_rb;
      AssertFatal(RA_template->msg3_nb_rb > 0, "nb_rb = 0\n");
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks              = RA_template->msg3_nb_rb;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.modulation_type                        = 2;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.cyclic_shift_2_for_drms                = 0;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_enabled_flag         = 0;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_bits                 = 0;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.new_data_indication                    = 0;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.redundancy_version                     = rvseq[RA_template->msg3_round];
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number                    = subframe2harqpid(cc,RA_template->Msg3_frame,RA_template->Msg3_subframe);
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.ul_tx_mode                             = 0;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.current_tx_nb                          = 0;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.n_srs                                  = 1;
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size                                   = get_TBS_UL(10,RA_template->msg3_nb_rb);
      ul_req_body->number_of_pdus++;
      // save UL scheduling information for preprocessor
      for (j=0;j<RA_template->msg3_nb_rb;j++) cc->vrb_map_UL[RA_template->msg3_first_rb+j]=1;
      
      
      if (RA_template->msg3_round != 0) { // program HI too
	hi_dci0_pdu                                                         = &hi_dci0_req->hi_dci0_pdu_list[hi_dci0_req->number_of_dci+hi_dci0_req->number_of_hi]; 	
	memset((void*)hi_dci0_pdu,0,sizeof(nfapi_hi_dci0_request_pdu_t));
	hi_dci0_pdu->pdu_type                                               = NFAPI_HI_DCI0_HI_PDU_TYPE; 
	hi_dci0_pdu->pdu_size                                               = 2+sizeof(nfapi_hi_dci0_hi_pdu);
	hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start                = RA_template->msg3_first_rb; 
	hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms             = 0;
	hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value                            = 0;
	hi_dci0_req->number_of_hi++;
	// save UL scheduling information for preprocessor
	for (j=0;j<RA_template->msg3_nb_rb;j++) cc->vrb_map_UL[RA_template->msg3_first_rb+j]=1;
	
	LOG_D(MAC,"[eNB %d][PUSCH-RA %x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) RA (mcs %d, first rb %d, nb_rb %d,round %d)\n",
	      module_idP,RA_template->rnti,CC_id,frameP,subframeP,10,
	      1,1,
	      RA_template->msg3_round-1);
      } //       if (RA_template->msg3_round != 0) { // program HI too
    } // non-BL/CE UE case
}

void generate_Msg2(module_id_t module_idP,int CC_idP,frame_t frameP,sub_frame_t subframeP,RA_TEMPLATE *RA_template) {

  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  COMMON_channels_t               *cc  = eNB->common_channels;
 
  uint8_t                         *vrb_map;
  int                             first_rb;
  int                             N_RB_DL;
  nfapi_dl_config_request_pdu_t   *dl_config_pdu;
  nfapi_tx_request_pdu_t          *TX_req;
  nfapi_dl_config_request_body_t *dl_req;

  vrb_map       = cc[CC_idP].vrb_map;
  dl_req        = &eNB->DL_req[CC_idP].dl_config_request_body;
  dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
  N_RB_DL       = to_prb(cc[CC_idP].mib->message.dl_Bandwidth);  

#ifdef Rel14
  int rmax            = 0;
  int rep             = 0; 
  int reps            = 0;
  int num_nb          = 0;

  first_rb        = 0;
  struct PRACH_ConfigSIB_v1310 *ext4_prach;
  PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13;
  PRACH_ParametersCE_r13_t *p[4]={NULL,NULL,NULL,NULL};

  uint16_t absSF      = (10*frameP)+subframeP;
  uint16_t absSF_Msg2 = (10*RA_template->Msg2_frame)+RA_template->Msg2_subframe;

  if (absSF>absSF_Msg2) return; // we're not ready yet, need to be to start ==  
  
  if (cc[CC_idP].radioResourceConfigCommon_BR) {

    ext4_prach                 = cc[CC_idP].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;
        
    switch (prach_ParametersListCE_r13->list.count) {
    case 4:
      p[3]=prach_ParametersListCE_r13->list.array[3];
    case 3:
      p[2]=prach_ParametersListCE_r13->list.array[2];
    case 2:
      p[1]=prach_ParametersListCE_r13->list.array[1];
    case 1:
      p[0]=prach_ParametersListCE_r13->list.array[0];
      break;
    default:
      AssertFatal(1==0,"Illegal count for prach_ParametersListCE_r13 %d\n",(int)prach_ParametersListCE_r13->list.count);
      break;
    }
  }

  if (RA_template->rach_resource_type > 0) {
    
    // This uses an MPDCCH Type 2 common allocation according to Section 9.1.5 36-213
    // Parameters:
    //    p=2+4 PRB set (number of PRB pairs 3)
    //    rmax = mpdcch-NumRepetition-RA-r13 => Table 9.1.5-3
    //    if CELevel = 0,1 => Table 9.1.5-1b for MPDCCH candidates
    //    if CELevel = 2,3 => Table 9.1.5-2b for MPDCCH candidates
    //    distributed transmission
    
    // rmax from SIB2 information
    AssertFatal(rmax<9,"rmax>8!\n");
    rmax           = 1<<p[RA_template->rach_resource_type-1]->mpdcch_NumRepetition_RA_r13;
    // choose r1 by default for RAR (Table 9.1.5-5)
    rep            = 0; 
    // get actual repetition count from Table 9.1.5-3
    reps           = (rmax<=8)?(1<<rep):(rmax>>(3-rep));
    // get narrowband according to higher-layer config 
    num_nb = p[RA_template->rach_resource_type-1]->mpdcch_NarrowbandsToMonitor_r13.list.count;
    RA_template->msg2_narrowband = *p[RA_template->rach_resource_type-1]->mpdcch_NarrowbandsToMonitor_r13.list.array[RA_template->preamble_index%num_nb];
    first_rb = narrowband_to_first_rb(&cc[CC_idP],RA_template->msg2_narrowband);
    
    if ((RA_template->msg2_mpdcch_repetition_cnt == 0) &&
	(mpdcch_sf_condition(eNB,CC_idP,frameP,subframeP,rmax,TYPE2,-1)>0)){
      // MPDCCH configuration for RAR
      LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, Programming MPDCCH %d repetitions\n",
	    module_idP,frameP,subframeP,reps);
      
      
      memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                                                  = NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE; 
      dl_config_pdu->pdu_size                                                                  = (uint8_t)(2+sizeof(nfapi_dl_config_mpdcch_pdu));
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_format                                    = (RA_template->rach_resource_type > 1) ? 11 : 10;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_narrow_band                            = RA_template->msg2_narrowband;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_prb_pairs                           = 6;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_assignment                     = 0; // Note: this can be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_tansmission_type                       = 1; // imposed (9.1.5 in 213) for Type 2 Common search space  
      AssertFatal(cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13!=NULL,
		  "cc[CC_id].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13 is null\n");
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.start_symbol                                  = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ecce_index                                    = 0;  // Note: this should be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.aggregation_level                             = 16; // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti_type                                     = 2;  // RA-RNTI
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti                                          = RA_template->RA_rnti;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ce_mode                                       = (RA_template->rach_resource_type < 3) ? 1 : 2;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.drms_scrambling_init                          = cc[CC_idP].physCellId;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.initial_transmission_sf_io                    = (frameP*10)+subframeP;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.transmission_power                            = 6000; // 0dB
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_coding                         = getRIV(6,0,6);  // Note: still to be checked if it should not be (getRIV(N_RB_DL,first_rb,6)) : Check nFAPI specifications and what is done L1 with this parameter
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs                                           = 4; // adjust according to size of RAR, 208 bits with N1A_PRB=3
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pdsch_reptition_levels                        = 4; // fix to 4 for now
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.redundancy_version                            = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.new_data_indicator                            = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_process                                  = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi_length                                   = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi                                          = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi_flag                                      = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi                                           = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_resource_offset                          = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_subframe_repetition_number                = rep; 
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpc                                           = 1;// N1A_PRB=3 (36.212); => 208 bits for mcs=4, choose mcs according t message size TBD
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index_length              = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index                     = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.allocate_prach_flag                           = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.preamble_index                                = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.prach_mask_index                              = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.starting_ce_level                             = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.srs_request                                   = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity_flag    = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity         = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.frequency_hopping_enabled_flag                = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.paging_direct_indication_differentiation_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.direct_indication                             = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.total_dci_length_including_padding            = 0; // this is not needed by OAI L1, but should be filled in
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports                    = 1;
      RA_template->msg2_mpdcch_repetition_cnt++;
      dl_req->number_pdu++;
      RA_template->Msg2_subframe = (RA_template->Msg2_subframe+9)%10;

    } //repetition_count==0 && SF condition met
    if (RA_template->msg2_mpdcch_repetition_cnt>0) { // we're in a stream of repetitions
      LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, MPDCCH repetition %d\n",
	    module_idP,frameP,subframeP,RA_template->msg2_mpdcch_repetition_cnt);

      if (RA_template->msg2_mpdcch_repetition_cnt==reps) { // this is the last mpdcch repetition
	if (cc[CC_idP].tdd_Config==NULL) { // FDD case
	  // wait 2 subframes for PDSCH transmission
	  if (subframeP>7) RA_template->Msg2_frame = (frameP+1)&1023;
	  else             RA_template->Msg2_frame = frameP;
	  RA_template->Msg2_subframe               = (subframeP+2)%10; // +2 is the "n+x" from Section 7.1.11  in 36.213
	}
	else {
	  AssertFatal(1==0,"TDD case not done yet\n");
	}
      }// mpdcch_repetition_count == reps
      RA_template->msg2_mpdcch_repetition_cnt++;	      

      if ((RA_template->Msg2_frame == frameP) && (RA_template->Msg2_subframe == subframeP)) {
	// Program PDSCH
	LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, Programming PDSCH\n",
	      module_idP,frameP,subframeP);
	RA_template->generate_rar = 0;	      
	
	dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
	memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE; 
	dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_idP];
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = RA_template->RA_rnti;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;   
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;   // localized
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL,first_rb,6);
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

	// Rel10 fields
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start                           = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
	// Rel13 fields
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type                               = (RA_template->rach_resource_type < 3) ? 1 : 2;;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type                    = 2;  // not SI message
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io            = (10*frameP)+subframeP;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.drms_table_flag                       = 0;
	dl_req->number_pdu++;
	
	// Program UL processing for Msg3, same as regular LTE
	get_Msg3alloc(&cc[CC_idP],subframeP,frameP,&RA_template->Msg3_frame,&RA_template->Msg3_subframe);
	add_msg3(module_idP,CC_idP, RA_template,frameP,subframeP);
	fill_rar_br(eNB,CC_idP,RA_template,frameP,subframeP,cc[CC_idP].RAR_pdu.payload,RA_template->rach_resource_type-1);
	// DL request
	eNB->TX_req[CC_idP].sfn_sf                                            = (frameP<<4)+subframeP;
	TX_req                                                                = &eNB->TX_req[CC_idP].tx_request_body.tx_pdu_list[eNB->TX_req[CC_idP].tx_request_body.number_of_pdus]; 		     
	TX_req->pdu_length                                                    = 7;  // This should be changed if we have more than 1 preamble 
	TX_req->pdu_index                                                     = eNB->pdu_index[CC_idP]++;
	TX_req->num_segments                                                  = 1;
	TX_req->segments[0].segment_length                                    = 7;
	TX_req->segments[0].segment_data                                      = cc[CC_idP].RAR_pdu.payload;
	eNB->TX_req[CC_idP].tx_request_body.number_of_pdus++;
      }
    }      
    
  }		
  else
#endif
    {

      if ((RA_template->Msg2_frame == frameP) && (RA_template->Msg2_subframe == subframeP)) {
	LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generating RAR DCI, RA_active %d format 1A (%d,%d))\n",
	      module_idP, CC_idP, frameP, subframeP,
	      RA_template->RA_active,
	      
	      RA_template->RA_dci_fmt1,
	      RA_template->RA_dci_size_bits1);

	// Allocate 4 PRBS starting in RB 0
	first_rb = 0;
	vrb_map[first_rb] = 1;
	vrb_map[first_rb+1] = 1;
	vrb_map[first_rb+2] = 1;
	vrb_map[first_rb+3] = 1;
	
	memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	dl_config_pdu->pdu_type                                                          = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
	dl_config_pdu->pdu_size                                                          = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                             = NFAPI_DL_DCI_FORMAT_1A;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level                      = 4;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                                   = RA_template->RA_rnti;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                              = 2;    // RA-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power                     = 6000; // equal to RS power
	
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                           = 0;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                                    = 1; // no TPC
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1                   = 1;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                                  = 0;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1                   = 0;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
	
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL,first_rb,4);      

	// This checks if the above DCI allocation is feasible in current subframe
	if (!CCE_allocation_infeasible(module_idP,CC_idP,0,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,RA_template->RA_rnti)) {
	  LOG_D(MAC,"Frame %d: Subframe %d : Adding common DCI for RA_RNTI %x\n",
		frameP,subframeP,RA_template->RA_rnti);
	  dl_req->number_dci++;
	  dl_req->number_pdu++;
	  
	  dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
	  memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	  dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE; 
	  dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_idP];
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = RA_template->RA_rnti;
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
	  
	  // Program UL processing for Msg3
	  get_Msg3alloc(&cc[CC_idP],subframeP,frameP,&RA_template->Msg3_frame,&RA_template->Msg3_subframe);

	  LOG_D(MAC,"Frame %d, Subframe %d: Setting Msg3 reception for Frame %d Subframe %d\n",
		frameP,subframeP,RA_template->Msg3_frame,RA_template->Msg3_subframe);
	  
	  fill_rar(module_idP,CC_idP,frameP,cc[CC_idP].RAR_pdu.payload,N_RB_DL,7);
	  add_msg3(module_idP,CC_idP, RA_template,frameP,subframeP);

	  // DL request
	  eNB->TX_req[CC_idP].sfn_sf                                             = (frameP<<4)+subframeP;
	  TX_req                                                                = &eNB->TX_req[CC_idP].tx_request_body.tx_pdu_list[eNB->TX_req[CC_idP].tx_request_body.number_of_pdus]; 		     
	  TX_req->pdu_length                                                    = 7;  // This should be changed if we have more than 1 preamble 
	  TX_req->pdu_index                                                     = eNB->pdu_index[CC_idP]++;
	  TX_req->num_segments                                                  = 1;
	  TX_req->segments[0].segment_length                                    = 7;
	  TX_req->segments[0].segment_data                                      = cc[CC_idP].RAR_pdu.payload;
	  eNB->TX_req[CC_idP].tx_request_body.number_of_pdus++;
	} // PDCCH CCE allocation is feasible
      } // Msg2 frame/subframe condition
    } // else BL/CE
}

void generate_Msg4(module_id_t module_idP,int CC_idP,frame_t frameP,sub_frame_t subframeP,RA_TEMPLATE *RA_template){


  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  COMMON_channels_t               *cc  = eNB->common_channels;
  int16_t                         rrc_sdu_length;
  int                             UE_id           = -1;
  uint16_t                        msg4_padding;
  uint16_t                        msg4_post_padding;
  uint16_t                        msg4_header;

  uint8_t                         *vrb_map;
  int                             first_rb;
  int                             N_RB_DL;
  nfapi_dl_config_request_pdu_t   *dl_config_pdu;
  nfapi_ul_config_request_pdu_t   *ul_config_pdu;
  nfapi_tx_request_pdu_t          *TX_req;
  UE_list_t                       *UE_list=&eNB->UE_list;
  nfapi_dl_config_request_body_t *dl_req;
  nfapi_ul_config_request_body_t *ul_req;
  uint8_t                         lcid;
  uint8_t                         offset;


#ifdef Rel14
  int rmax            = 0;
  int rep             = 0; 
  int reps            = 0;


  first_rb        = 0;
  struct PRACH_ConfigSIB_v1310 *ext4_prach;
  struct PUCCH_ConfigCommon_v1310 *ext4_pucch;
  PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13;
  struct N1PUCCH_AN_InfoList_r13 *pucch_N1PUCCH_AN_InfoList_r13;
  PRACH_ParametersCE_r13_t *p[4]={NULL,NULL,NULL,NULL};
  int pucchreps[4]={1,1,1,1};
  int n1pucchan[4]={0,0,0,0};

  if (cc[CC_idP].radioResourceConfigCommon_BR) {

    ext4_prach                    = cc[CC_idP].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    ext4_pucch                    = cc[CC_idP].radioResourceConfigCommon_BR->ext4->pucch_ConfigCommon_v1310;
    prach_ParametersListCE_r13    = &ext4_prach->prach_ParametersListCE_r13;
    pucch_N1PUCCH_AN_InfoList_r13 = ext4_pucch->n1PUCCH_AN_InfoList_r13;
    AssertFatal(prach_ParametersListCE_r13!=NULL,"prach_ParametersListCE_r13 is null\n");
    AssertFatal(pucch_N1PUCCH_AN_InfoList_r13!=NULL,"pucch_N1PUCCH_AN_InfoList_r13 is null\n");
    // check to verify CE-Level compatibility in SIB2_BR
    AssertFatal(prach_ParametersListCE_r13->list.count == pucch_N1PUCCH_AN_InfoList_r13->list.count,
		"prach_ParametersListCE_r13->list.count!= pucch_N1PUCCH_AN_InfoList_r13->list.count\n");

    switch (prach_ParametersListCE_r13->list.count) {
    case 4:
      p[3]         = prach_ParametersListCE_r13->list.array[3];
      n1pucchan[3] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[3]; 
      AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level3_r13!=NULL,"pucch_NumRepetitionCE_Msg4_Level3 shouldn't be NULL\n");
      pucchreps[3] = (int)(4<<*ext4_pucch->pucch_NumRepetitionCE_Msg4_Level3_r13);
      
    case 3:
      p[2]         = prach_ParametersListCE_r13->list.array[2];
      n1pucchan[2] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[2]; 
      AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13!=NULL,"pucch_NumRepetitionCE_Msg4_Level2 shouldn't be NULL\n");
      pucchreps[2] = (int)(4<<*ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13);
    case 2:
      p[1]         =prach_ParametersListCE_r13->list.array[1];
      n1pucchan[1] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[1]; 
      AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13!=NULL,"pucch_NumRepetitionCE_Msg4_Level1 shouldn't be NULL\n");
      pucchreps[1] = (int)(1<<*ext4_pucch->pucch_NumRepetitionCE_Msg4_Level1_r13);
    case 1:
      p[0]         = prach_ParametersListCE_r13->list.array[0];
      n1pucchan[0] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[0]; 
      AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13!=NULL,"pucch_NumRepetitionCE_Msg4_Level0 shouldn't be NULL\n");
      pucchreps[0] = (int)(1<<*ext4_pucch->pucch_NumRepetitionCE_Msg4_Level0_r13);
    default:
      AssertFatal(1==0,"Illegal count for prach_ParametersListCE_r13 %d\n",prach_ParametersListCE_r13->list.count);
    }
  }
#endif

  vrb_map       = cc[CC_idP].vrb_map;
  
  dl_req        = &eNB->DL_req[CC_idP].dl_config_request_body;
  dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
  N_RB_DL       = to_prb(cc[CC_idP].mib->message.dl_Bandwidth);

  UE_id = find_UE_id(module_idP,RA_template->rnti);
  AssertFatal(UE_id>=0,"Can't find UE for t-crnti\n");
  
  // set HARQ process round to 0 for this UE
  
  if (cc->tdd_Config) RA_template->harq_pid = ((frameP*10)+subframeP)%10;
  else RA_template->harq_pid = ((frameP*10)+subframeP)&7;

  
  // Get RRCConnectionSetup for Piggyback
  rrc_sdu_length = mac_rrc_data_req(module_idP,
				    CC_idP,
				    frameP,
				    CCCH,
				    1, // 1 transport block
				    &cc[CC_idP].CCCH_pdu.payload[0],
				    ENB_FLAG_YES,
				    module_idP,
				    0); // not used in this case
  
  AssertFatal(rrc_sdu_length>0,
	      "[MAC][eNB Scheduler] CCCH not allocated\n");
  
  
  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: UE_id %d, rrc_sdu_length %d\n",
	module_idP, CC_idP, frameP, subframeP,UE_id, rrc_sdu_length);
  
  
#ifdef Rel14
  if (RA_template->rach_resource_type>0) {
    
    // Generate DCI + repetitions first
    // This uses an MPDCCH Type 2 allocation according to Section 9.1.5 36-213, Type2 common allocation according to Table 7.1-8 (36-213)
    // Parameters:
    //    p=2+4 PRB set (number of PRB pairs 6)
    //    rmax = mpdcch-NumRepetition-RA-r13 => Table 9.1.5-3
    //    if CELevel = 0,1 => Table 9.1.5-1b for MPDCCH candidates
    //    if CELevel = 2,3 => Table 9.1.5-2b for MPDCCH candidates
    //    distributed transmission
    
    // rmax from SIB2 information
    rmax           = p[RA_template->rach_resource_type-1]->mpdcch_NumRepetition_RA_r13;
    AssertFatal(rmax>=4,"choose rmax>=4 for enough repeititions, or reduce rep to 1 or 2\n");

    // choose r3 by default for Msg4 (this is ok from table 9.1.5-3 for rmax = >=4, if we choose rmax <4 it has to be less
    rep            = 2; 
    // get actual repetition count from Table 9.1.5-3
    reps           = (rmax<=8)?(1<<rep):(rmax>>(3-rep));
    // get first narrowband
    first_rb = narrowband_to_first_rb(&cc[CC_idP],RA_template->msg34_narrowband);

    if ((RA_template->msg4_mpdcch_repetition_cnt == 0) &&
    (mpdcch_sf_condition(eNB,CC_idP,frameP,subframeP,rmax,TYPE2,-1)>0)){
      // MPDCCH configuration for RAR
      
      memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                                                  = NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE; 
      dl_config_pdu->pdu_size                                                                  = (uint8_t)(2+sizeof(nfapi_dl_config_mpdcch_pdu));
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_format                                    = (RA_template->rach_resource_type > 1) ? 11 : 10;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_narrow_band                            = RA_template->msg34_narrowband;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_prb_pairs                           = 6;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_assignment                     = 0; // Note: this can be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_tansmission_type                       = 1;
      AssertFatal(cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13!=NULL,
		  "cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13 is null\n");
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.start_symbol                                  = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ecce_index                                    = 0;  // Note: this should be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.aggregation_level                             = 16; // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti_type                                     = 0;  // t-C-RNTI
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti                                          = RA_template->RA_rnti;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ce_mode                                       = (RA_template->rach_resource_type < 3) ? 1 : 2;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.drms_scrambling_init                          = cc[CC_idP].physCellId;  /// Check this is still N_id_cell for type2 common
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.initial_transmission_sf_io                    = (frameP*10)+subframeP;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.transmission_power                            = 6000; // 0dB
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_coding                         = getRIV(6,0,6);// check if not getRIV(N_RB_DL,first_rb,6);
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs                                           = 4; // adjust according to size of Msg4, 208 bits with N1A_PRB=3
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pdsch_reptition_levels                        = 4; // fix to 4 for now
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.redundancy_version                            = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.new_data_indicator                            = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_process                                  = RA_template->harq_pid;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi_length                                   = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi                                          = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi_flag                                      = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi                                           = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_resource_offset                          = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_subframe_repetition_number                = rep; 
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpc                                           = 1;// N1A_PRB=3; => 208 bits
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index_length              = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index                     = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.allocate_prach_flag                           = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.preamble_index                                = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.prach_mask_index                              = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.starting_ce_level                             = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.srs_request                                   = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity_flag    = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity         = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.frequency_hopping_enabled_flag                = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.paging_direct_indication_differentiation_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.direct_indication                             = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.total_dci_length_including_padding            = 0; // this is not needed by OAI L1, but should be filled in
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports                    = 1;
      RA_template->msg4_mpdcch_repetition_cnt++;
      dl_req->number_pdu++;
      
    } //repetition_count==0 && SF condition met
    else if (RA_template->msg4_mpdcch_repetition_cnt>0) { // we're in a stream of repetitions
      RA_template->msg4_mpdcch_repetition_cnt++;	      
      if (RA_template->msg4_mpdcch_repetition_cnt==reps) { // this is the last mpdcch repetition
	if (cc[CC_idP].tdd_Config==NULL) { // FDD case
	  // wait 2 subframes for PDSCH transmission
	  if (subframeP>7) RA_template->Msg4_frame = (frameP+1)&1023;
	  else             RA_template->Msg4_frame = frameP;
	  RA_template->Msg4_subframe               = (subframeP+2)%10; 
	}
	else {
	  AssertFatal(1==0,"TDD case not done yet\n");
	}
      } // mpdcch_repetition_count == reps
      if ((RA_template->Msg4_frame == frameP) && (RA_template->Msg4_subframe == subframeP)) {
	
	// Program PDSCH
	
	LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating Msg4 BR with RRC Piggyback (ce_level %d RNTI %x)\n",
	      module_idP, CC_idP, frameP, subframeP,RA_template->rach_resource_type-1,RA_template->rnti);

	AssertFatal(1==0,"Msg4 generation not finished for BL/CE UE\n");
	dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
	memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE; 
	dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_idP];
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = RA_template->rnti;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;   // format 1A/1B/1D
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;   // localized
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL,first_rb,6);  // check that this isn't getRIV(6,0,6)
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

	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start                           = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
	
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type                               = (RA_template->rach_resource_type < 3) ? 1 : 2;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type                    = 2;  // not SI message
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io            = (10*frameP)+subframeP;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.drms_table_flag                       = 0;
	dl_req->number_pdu++;
	
	RA_template->generate_Msg4=0;
	RA_template->wait_ack_Msg4=1;

	lcid=0;
	
	UE_list->UE_sched_ctrl[UE_id].round[CC_idP][RA_template->harq_pid] = 0;
	msg4_header = 1+6+1;  // CR header, CR CE, SDU header
	
	if ((RA_template->msg4_TBsize - rrc_sdu_length - msg4_header) <= 2) {
	  msg4_padding = RA_template->msg4_TBsize - rrc_sdu_length - msg4_header;
	  msg4_post_padding = 0;
	} else {
	  msg4_padding = 0;
	  msg4_post_padding = RA_template->msg4_TBsize - rrc_sdu_length - msg4_header -1;
	}
	
	LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d subframeP %d Msg4 : TBS %d, sdu_len %d, msg4_header %d, msg4_padding %d, msg4_post_padding %d\n",
	      module_idP,CC_idP,frameP,subframeP,RA_template->msg4_TBsize,rrc_sdu_length,msg4_header,msg4_padding,msg4_post_padding);
	DevAssert( UE_id != UE_INDEX_INVALID ); // FIXME not sure how to gracefully return
	// CHECK THIS: &cc[CC_idP].CCCH_pdu.payload[0]
	offset = generate_dlsch_header((unsigned char*)eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0],
				       1,                           //num_sdus
				       (unsigned short*)&rrc_sdu_length,             //
				       &lcid,                       // sdu_lcid
				       255,                         // no drx
				       31,                          // no timing advance
				       RA_template->cont_res_id,    // contention res id
				       msg4_padding,                // no padding
				       msg4_post_padding);
	
	memcpy((void*)&eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0][(unsigned char)offset],
	       &cc[CC_idP].CCCH_pdu.payload[0],
	       rrc_sdu_length);
	
	// DL request
	eNB->TX_req[CC_idP].sfn_sf                                             = (frameP<<4)+subframeP;
	TX_req                                                                = &eNB->TX_req[CC_idP].tx_request_body.tx_pdu_list[eNB->TX_req[CC_idP].tx_request_body.number_of_pdus]; 		     	      
	TX_req->pdu_length                                                    = rrc_sdu_length;
	TX_req->pdu_index                                                     = eNB->pdu_index[CC_idP]++;
	TX_req->num_segments                                                  = 1;
	TX_req->segments[0].segment_length                                    = rrc_sdu_length;
	TX_req->segments[0].segment_data                                      = eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0];
	eNB->TX_req[CC_idP].tx_request_body.number_of_pdus++;

	// Program ACK/NAK for Msg4 PDSCH
	int absSF = (RA_template->Msg3_frame*10)+RA_template->Msg3_subframe;
	// see Section 10.2 from 36.213
	int ackNAK_absSF = absSF + reps + 4;
	AssertFatal(reps>2,"Have to handle programming of ACK when PDSCH repetitions is > 2\n");
	ul_req        = &eNB->UL_req_tmp[CC_idP][ackNAK_absSF%10].ul_config_request_body;
	ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus]; 

	ul_config_pdu->pdu_type                                                                     = NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE; 
	ul_config_pdu->pdu_size                                                                     = (uint8_t)(2+sizeof(nfapi_ul_config_uci_harq_pdu));
	ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.handle                       = 0; // don't know how to use this
	ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti                         = RA_template->rnti;
	ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.ue_type                     = (RA_template->rach_resource_type < 3) ? 1 : 2;
	ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.empty_symbols               = 0;
	ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.total_number_of_repetitions = pucchreps[RA_template->rach_resource_type-1];
	ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.repetition_number           = 0;
	// Note need to keep sending this across reptitions!!!! Not really for PUCCH, to ask small-cell forum, we'll see for the other messages, maybe parameters change across repetitions and FAPI has to provide for that
	if (cc[CC_idP].tdd_Config==NULL) { // FDD case
	  ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel8_fdd.n_pucch_1_0   = n1pucchan[RA_template->rach_resource_type-1];
	  // NOTE: How to fill in the rest of the n_pucch_1_0 information 213 Section 10.1.2.1 in the general case
	  // = N_ECCE_q + Delta_ARO + n1pucchan[ce_level]
	  // higher in the MPDCCH configuration, N_ECCE_q is hard-coded to 0, and harq resource offset to 0 =>
	  // Delta_ARO = 0 from Table 10.1.2.1-1
	  ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel8_fdd.harq_size     = 1;  // 1-bit ACK/NAK
	}
	else {
	  AssertFatal(1==0,"PUCCH configuration for ACK/NAK not handled yet for TDD BL/CE case\n");
	}
	ul_req->number_of_pdus++;
	T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_idP), T_INT(RA_template->rnti), T_INT(frameP), T_INT(subframeP),
	  T_INT(0 /*harq_pid always 0?*/), T_BUFFER(&eNB->UE_list.DLSCH_pdu[CC_idP][0][UE_id].payload[0], RA_template->msg4_TBsize));
	
	if (opt_enabled==1) {
	  trace_pdu(1, (uint8_t *)eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0],
		    rrc_sdu_length, UE_id, 3, UE_RNTI(module_idP, UE_id),
		    eNB->frame, eNB->subframe,0,0);
	  LOG_D(OPT,"[eNB %d][DLSCH] CC_id %d Frame %d trace pdu for rnti %x with size %d\n",
		module_idP, CC_idP, frameP, UE_RNTI(module_idP,UE_id), rrc_sdu_length);
	}
      } // Msg4 frame/subframe
    } // msg4_mpdcch_repetition_count
  } // rach_resource_type > 0 
  else
#endif
    { // This is normal LTE case
      if ((RA_template->Msg4_frame == frameP) && (RA_template->Msg4_subframe == subframeP)) {	      
	LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating Msg4 with RRC Piggyback (RNTI %x)\n",
	      module_idP, CC_idP, frameP, subframeP,RA_template->rnti);
	
	/// Choose first 4 RBs for Msg4, should really check that these are free!
	first_rb=0;
	
	vrb_map[first_rb] = 1;
	vrb_map[first_rb+1] = 1;
	vrb_map[first_rb+2] = 1;
	vrb_map[first_rb+3] = 1;
	
	
	// Compute MCS/TBS for 3 PRB (coded on 4 vrb)
	msg4_header = 1+6+1;  // CR header, CR CE, SDU header
	
	if ((rrc_sdu_length+msg4_header) <= 22) {
	  RA_template->msg4_mcs                       = 4;
	  RA_template->msg4_TBsize = 22;
	} else if ((rrc_sdu_length+msg4_header) <= 28) {
	  RA_template->msg4_mcs                       = 5;
	  RA_template->msg4_TBsize = 28;
	} else if ((rrc_sdu_length+msg4_header) <= 32) {
	  RA_template->msg4_mcs                       = 6;
	  RA_template->msg4_TBsize = 32;
	} else if ((rrc_sdu_length+msg4_header) <= 41) {
	  RA_template->msg4_mcs                       = 7;
	  RA_template->msg4_TBsize = 41;
	} else if ((rrc_sdu_length+msg4_header) <= 49) {
	  RA_template->msg4_mcs                       = 8;
	  RA_template->msg4_TBsize = 49;
	} else if ((rrc_sdu_length+msg4_header) <= 57) {
	  RA_template->msg4_mcs    = 9;
	  RA_template->msg4_TBsize = 57;
	}

	fill_nfapi_dl_dci_1A(dl_config_pdu,
			     4,                           // aggregation_level
			     RA_template->rnti,           // rnti
			     1,                           // rnti_type, CRNTI
			     RA_template->harq_pid,       // harq_process
			     1,                           // tpc, none
			     getRIV(N_RB_DL,first_rb,4),  // resource_block_coding
			     RA_template->msg4_mcs,       // mcs
			     1,                           // ndi
			     0,                           // rv
			     0);                          // vrb_flag
    LOG_D(MAC,"Frame %d, subframe %d: Msg4 DCI pdu_num %d (rnti %x,rnti_type %d,harq_pid %d, resource_block_coding (%p) %d\n",
		  frameP,
	      subframeP,
          dl_req->number_pdu,
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti,
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type,
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process,
          &dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding,
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding);
    AssertFatal(dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding < 8192,
				"resource_block_coding %u < 8192\n",
                dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding);	
	if (!CCE_allocation_infeasible(module_idP,CC_idP,1,
				       subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
				       RA_template->rnti)) {
	  dl_req->number_dci++;
	  dl_req->number_pdu++;
	  
	  RA_template->generate_Msg4=0;
	  RA_template->wait_ack_Msg4=1;
	  
	  // increment Absolute subframe by 8 for Msg4 retransmission
	  LOG_D(MAC,"Frame %d, Subframe %d: Preparing for Msg4 retransmission currently %d.%d\n",
		frameP,subframeP,RA_template->Msg4_frame,RA_template->Msg4_subframe);
	  if (RA_template->Msg4_subframe > 1) RA_template->Msg4_frame++;
	  RA_template->Msg4_frame&=1023;
	  RA_template->Msg4_subframe = (RA_template->Msg4_subframe+8)%10;
	  LOG_D(MAC,"Frame %d, Subframe %d: Msg4 retransmission in %d.%d\n",
		frameP,subframeP,RA_template->Msg4_frame,RA_template->Msg4_subframe);
	  lcid=0;
	  
	  // put HARQ process round to 0
	  if (cc->tdd_Config) RA_template->harq_pid = ((frameP*10)+subframeP)%10;
	  else RA_template->harq_pid = ((frameP*10)+subframeP)&7;
	  UE_list->UE_sched_ctrl[UE_id].round[CC_idP][RA_template->harq_pid] = 0;
	  
	  if ((RA_template->msg4_TBsize - rrc_sdu_length - msg4_header) <= 2) {
	    msg4_padding = RA_template->msg4_TBsize - rrc_sdu_length - msg4_header;
	    msg4_post_padding = 0;
	  } else {
	    msg4_padding = 0;
	    msg4_post_padding = RA_template->msg4_TBsize - rrc_sdu_length - msg4_header -1;
	  }
	  
	  LOG_D(MAC,"[eNB %d][RAPROC] CC_idP %d Frame %d subframeP %d Msg4 : TBS %d, sdu_len %d, msg4_header %d, msg4_padding %d, msg4_post_padding %d\n",
		module_idP,CC_idP,frameP,subframeP,RA_template->msg4_TBsize,rrc_sdu_length,msg4_header,msg4_padding,msg4_post_padding);
	  DevAssert( UE_id != UE_INDEX_INVALID ); // FIXME not sure how to gracefully return
	  // CHECK THIS: &cc[CC_idP].CCCH_pdu.payload[0]
	  offset = generate_dlsch_header((unsigned char*)eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0],
					 1,                           //num_sdus
					 (unsigned short*)&rrc_sdu_length,             //
					 &lcid,                       // sdu_lcid
					 255,                         // no drx
					 31,                          // no timing advance
					 RA_template->cont_res_id,    // contention res id
					 msg4_padding,                // no padding
					 msg4_post_padding);
	  
	  memcpy((void*)&eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0][(unsigned char)offset],
		 &cc[CC_idP].CCCH_pdu.payload[0],
		 rrc_sdu_length);
	  
	  // DLSCH Config
	  fill_nfapi_dlsch_config(eNB,
				  dl_req,
				  RA_template->msg4_TBsize,
				  eNB->pdu_index[CC_idP],
				  RA_template->rnti,
				  2,                           // resource_allocation_type : format 1A/1B/1D
				  0,                           // virtual_resource_block_assignment_flag : localized
				  getRIV(N_RB_DL,first_rb,4),  // resource_block_coding : RIV, 4 PRB
				  2,                           // modulation: QPSK
				  0,                           // redundancy version
				  1,                           // transport_blocks
				  0,                           // transport_block_to_codeword_swap_flag (0)
				  (cc->p_eNB==1 ) ? 0 : 1,     // transmission_scheme
				  1,                           // number of layers
				  1,                           // number of subbands
				  //0,                         // codebook index 
				  1,                           // ue_category_capacity
				  4,                           // pa: 0 dB
				  0,                           // delta_power_offset_index
				  0,                           // ngap
				  1,                           // NPRB = 3 like in DCI
				  (cc->p_eNB==1 ) ? 1 : 2,     // transmission mode
				  1,                           // num_bf_prb_per_subband
				  1);                          // num_bf_vector
      LOG_D(MAC,"Filled DLSCH config, pdu number %d, non-dci pdu_index %d\n",dl_req->number_pdu,eNB->pdu_index[CC_idP]);

	  // DL request
	  eNB->TX_req[CC_idP].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_idP].tx_request_body,
							 (frameP*10)+subframeP,
							 rrc_sdu_length,
							 eNB->pdu_index[CC_idP],
							 eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0]); 
	  eNB->pdu_index[CC_idP]++;

	  LOG_D(MAC,"Filling UCI ACK/NAK information, cce_idx %d\n",dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
	  // Program PUCCH1a for ACK/NAK
	  // Program ACK/NAK for Msg4 PDSCH
	  fill_nfapi_uci_acknak(module_idP,
				CC_idP,
				RA_template->rnti,
				(frameP*10)+subframeP,
				dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);

	  
	  T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_idP), T_INT(RA_template->rnti), T_INT(frameP), T_INT(subframeP),
	    T_INT(0 /*harq_pid always 0?*/), T_BUFFER(&eNB->UE_list.DLSCH_pdu[CC_idP][0][UE_id].payload[0], RA_template->msg4_TBsize));
	  
	  if (opt_enabled==1) {
	    trace_pdu(1, (uint8_t *)eNB->UE_list.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0],
		      rrc_sdu_length, UE_id, 3, UE_RNTI(module_idP, UE_id),
		      eNB->frame, eNB->subframe,0,0);
	    LOG_D(OPT,"[eNB %d][DLSCH] CC_id %d Frame %d trace pdu for rnti %x with size %d\n",
		  module_idP, CC_idP, frameP, UE_RNTI(module_idP,UE_id), rrc_sdu_length);
	  }
	  
	} // CCE Allocation feasible
      } // msg4 frame/subframe
    } // else rach_resource_type
}

void check_Msg4_retransmission(module_id_t module_idP,int CC_idP,frame_t frameP,sub_frame_t subframeP,RA_TEMPLATE *RA_template) {
 
  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  COMMON_channels_t               *cc  = eNB->common_channels;
  int                             UE_id           = -1;
  uint8_t                         *vrb_map;
  int                             first_rb;
  int                             N_RB_DL;
  nfapi_dl_config_request_pdu_t   *dl_config_pdu;
  UE_list_t                       *UE_list=&eNB->UE_list;
  nfapi_dl_config_request_body_t *dl_req;

  int                             round;
  /*
#ifdef Rel14
  COMMON_channels_t               *cc  = eNB->common_channels;

  int rmax            = 0;
  int rep             = 0; 
  int reps            = 0;

  first_rb        = 0;
  struct PRACH_ConfigSIB_v1310 *ext4_prach;
  PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13;
  PRACH_ParametersCE_r13_t *p[4];

  if (cc[CC_idP].radioResourceConfigCommon_BR) {

    ext4_prach                 = cc[CC_idP].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;
        
    switch (prach_ParametersListCE_r13->list.count) {
    case 4:
      p[3]=prach_ParametersListCE_r13->list.array[3];
    case 3:
      p[2]=prach_ParametersListCE_r13->list.array[2];
    case 2:
      p[1]=prach_ParametersListCE_r13->list.array[1];
    case 1:
      p[0]=prach_ParametersListCE_r13->list.array[0];
    default:
      AssertFatal(1==0,"Illegal count for prach_ParametersListCE_r13 %d\n",prach_ParametersListCE_r13->list.count);
    }
  }
#endif
  */

  // check HARQ status and retransmit if necessary

  
  UE_id = find_UE_id(module_idP,RA_template->rnti);
  AssertFatal(UE_id>=0,"Can't find UE for t-crnti\n");

  round = UE_list->UE_sched_ctrl[UE_id].round[CC_idP][RA_template->harq_pid];
  vrb_map       = cc[CC_idP].vrb_map;
  
  dl_req        = &eNB->DL_req[CC_idP].dl_config_request_body;
  dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
  N_RB_DL       = to_prb(cc[CC_idP].mib->message.dl_Bandwidth);
  
  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Checking if Msg4 for harq_pid %d was acknowledged (round %d)\n",
	module_idP,CC_idP,frameP,subframeP,RA_template->harq_pid,round);

  if (round!=8) {
    
#ifdef Rel14
    if (RA_template->rach_resource_type>0) {
      AssertFatal(1==0,"Msg4 Retransmissions not handled yet for BL/CE UEs\n");
    }
    else
#endif 
      {
	if ( (RA_template->Msg4_frame == frameP) && (RA_template->Msg4_subframe == subframeP)) {	       
	  
	  //RA_template->wait_ack_Msg4++;
	  // we have to schedule a retransmission
	  
	  first_rb=0;
	  vrb_map[first_rb] = 1;
	  vrb_map[first_rb+1] = 1;
	  vrb_map[first_rb+2] = 1;
	  vrb_map[first_rb+3] = 1;
	  
	  fill_nfapi_dl_dci_1A(dl_config_pdu,
			       4,                           // aggregation_level
			       RA_template->rnti,           // rnti
			       1,                           // rnti_type, CRNTI
			       RA_template->harq_pid,       // harq_process
			       1,                           // tpc, none
			       getRIV(N_RB_DL,first_rb,4),  // resource_block_coding
			       RA_template->msg4_mcs,       // mcs
			       1,                           // ndi
			       round&3,                       // rv
			       0);                          // vrb_flag
	  
	  if (!CCE_allocation_infeasible(module_idP,CC_idP,1,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,RA_template->rnti)) {
	    dl_req->number_dci++;
	    dl_req->number_pdu++;
	    
	    LOG_D(MAC,"msg4 retransmission for rnti %x (round %d) fsf %d/%d\n", RA_template->rnti, round, frameP, subframeP);
	    	  // DLSCH Config
	    fill_nfapi_dlsch_config(eNB,
				    dl_req,
				    RA_template->msg4_TBsize,
				    -1                           /* retransmission, no pdu_index */,
				    RA_template->rnti,
				    2,                           // resource_allocation_type : format 1A/1B/1D
				    0,                           // virtual_resource_block_assignment_flag : localized
				    getRIV(N_RB_DL,first_rb,4),  // resource_block_coding : RIV, 4 PRB
				    2,                           // modulation: QPSK
				    round&3,                     // redundancy version
				    1,                           // transport_blocks
				    0,                           // transport_block_to_codeword_swap_flag (0)
				    (cc->p_eNB==1 ) ? 0 : 1,     // transmission_scheme
				    1,                           // number of layers
				    1,                           // number of subbands
				    //0,                         // codebook index 
				    1,                           // ue_category_capacity
				    4,                           // pa: 0 dB
				    0,                           // delta_power_offset_index
				    0,                           // ngap
				    1,                           // NPRB = 3 like in DCI
				    (cc->p_eNB==1 ) ? 1 : 2,     // transmission mode
				    1,                           // num_bf_prb_per_subband
				    1);                          // num_bf_vector
	  }
	  else
	    LOG_D(MAC,"msg4 retransmission for rnti %x (round %d) fsf %d/%d CCE allocation failed!\n", RA_template->rnti, round, frameP, subframeP);
	  
	  
	  // Program PUCCH1a for ACK/NAK
	  

	  fill_nfapi_uci_acknak(module_idP,CC_idP,
				RA_template->rnti,
				(frameP*10)+subframeP,
				dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
	  
	  // prepare frame for retransmission
	  if (RA_template->Msg4_subframe>1) RA_template->Msg4_frame++;
	  RA_template->Msg4_frame&=1023;
	  RA_template->Msg4_subframe=(RA_template->Msg4_subframe+8)%10;

	  LOG_W(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Msg4 not acknowledged, adding ue specific dci (rnti %x) for RA (Msg4 Retransmission round %d in %d.%d)\n",
		module_idP,CC_idP,frameP,subframeP,RA_template->rnti,round,RA_template->Msg4_frame,RA_template->Msg4_subframe);

	} // Msg4 frame/subframe
      } // regular LTE case
  } else {
    LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d : Msg4 acknowledged\n",module_idP,CC_idP,frameP,subframeP);
    RA_template->wait_ack_Msg4=0;
    RA_template->RA_active=FALSE;
    UE_id = find_UE_id(module_idP,RA_template->rnti);
    DevAssert( UE_id != -1 );
    eNB->UE_list.UE_template[UE_PCCID(module_idP,UE_id)][UE_id].configured=TRUE;
  }
} 

void schedule_RA(module_id_t module_idP,frame_t frameP, sub_frame_t subframeP)
{

  int                             CC_id;
  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  COMMON_channels_t               *cc  = eNB->common_channels;
  RA_TEMPLATE                     *RA_template;
  uint8_t                         i;

  start_meas(&eNB->schedule_ra);


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    // skip UL component carriers if TDD
    if (is_UL_sf(&cc[CC_id],subframeP)==1) continue;


    for (i=0; i<NB_RA_PROC_MAX; i++) {

      RA_template = (RA_TEMPLATE *)&cc[CC_id].RA_template[i];

      if (RA_template->RA_active == TRUE) {

        LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d RA %d is active (generate RAR %d, generate_Msg4 %d, wait_ack_Msg4 %d, rnti %x)\n",
              module_idP,frameP,subframeP,CC_id,i,RA_template->generate_rar,RA_template->generate_Msg4,RA_template->wait_ack_Msg4, RA_template->rnti);

        if      (RA_template->generate_rar == 1)  generate_Msg2(module_idP,CC_id,frameP,subframeP,RA_template);
	else if (RA_template->generate_Msg4 == 1) generate_Msg4(module_idP,CC_id,frameP,subframeP,RA_template);
 	else if (RA_template->wait_ack_Msg4==1)   check_Msg4_retransmission(module_idP,CC_id,frameP,subframeP,RA_template);


      } // RA_active == TRUE
    } // for i=0 .. N_RA_PROC-1 
  } // CC_id
  
  stop_meas(&eNB->schedule_ra);
}


// handles the event of MSG1 reception
void initiate_ra_proc(module_id_t module_idP, 
		      int CC_id,
		      frame_t frameP, 
		      sub_frame_t subframeP,
		      uint16_t preamble_index,
		      int16_t timing_offset,
		      uint16_t ra_rnti
#ifdef Rel14
		      ,
		      uint8_t rach_resource_type
#endif
		      )
{

  uint8_t i;

  COMMON_channels_t   *cc  = &RC.mac[module_idP]->common_channels[CC_id];
  RA_TEMPLATE *RA_template = &cc->RA_template[0];

  struct PRACH_ConfigSIB_v1310 *ext4_prach = NULL;
  PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = NULL;

  if (cc->radioResourceConfigCommon_BR && cc->radioResourceConfigCommon_BR->ext4) {
    ext4_prach=cc->radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    prach_ParametersListCE_r13= &ext4_prach->prach_ParametersListCE_r13;
  }
  LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, Subframe %d  Initiating RA procedure for preamble index %d\n",module_idP,CC_id,frameP,subframeP,preamble_index);
#ifdef Rel14
  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, Subframe %d  PRACH resource type %d\n",module_idP,CC_id,frameP,subframeP,rach_resource_type);
#endif

  if (prach_ParametersListCE_r13 && 
      prach_ParametersListCE_r13->list.count<rach_resource_type) {
    LOG_E(MAC,"[eNB %d][RAPROC] CC_id %d Received impossible PRACH resource type %d, only %d CE levels configured\n",
	  module_idP,CC_id,
	  rach_resource_type,
	  (int)prach_ParametersListCE_r13->list.count);
    return;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC,1);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC,0);

  for (i=0; i<NB_RA_PROC_MAX; i++) {
    if (RA_template[i].RA_active==FALSE &&
        RA_template[i].wait_ack_Msg4 == 0) {
      int loop = 0;
      LOG_D(MAC,"Frame %d, Subframe %d: Activating RA process %d\n",frameP,subframeP,i);
      RA_template[i].RA_active          = TRUE;
      RA_template[i].generate_rar       = 1;
      RA_template[i].generate_Msg4      = 0;
      RA_template[i].wait_ack_Msg4      = 0;
      RA_template[i].timing_offset      = timing_offset;
      RA_template[i].preamble_subframe  = subframeP;
#ifdef Rel14
      RA_template[i].rach_resource_type = rach_resource_type;
      RA_template[i].msg2_mpdcch_repetition_cnt = 0;		      
      RA_template[i].msg4_mpdcch_repetition_cnt = 0;		      
#endif
      RA_template[i].Msg2_frame         = frameP+((subframeP>5)?1:0);
      RA_template[i].Msg2_subframe      = (subframeP+4)%10;
      /* TODO: find better procedure to allocate RNTI */
      do {
        RA_template[i].rnti = taus();
        loop++;
      } while (loop != 100 &&
               /* TODO: this is not correct, the rnti may be in use without
                * being in the MAC yet. To be refined.
                */
               !(find_UE_id(module_idP, RA_template[i].rnti) == -1 &&
                 /* 1024 and 60000 arbirarily chosen, not coming from standard */
                 RA_template[i].rnti >= 1024 && RA_template[i].rnti < 60000));
      if (loop == 100) { printf("%s:%d:%s: FATAL ERROR! contact the authors\n", __FILE__, __LINE__, __FUNCTION__); abort(); }
      RA_template[i].RA_rnti        = ra_rnti;
      RA_template[i].preamble_index = preamble_index;
      LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Activating RAR generation in Frame %d, subframe %d for process %d, rnti %x, RA_active %d\n",
            module_idP,CC_id,frameP,
	    RA_template[i].Msg2_frame,
	    RA_template[i].Msg2_subframe,
	    i,RA_template[i].rnti,
            RA_template[i].RA_active);

      return;
    }
  }

  LOG_E(MAC,"[eNB %d][RAPROC] FAILURE: CC_id %d Frame %d Initiating RA procedure for preamble index %d\n",module_idP,CC_id,frameP,preamble_index);
}

void cancel_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, rnti_t rnti)
{
  unsigned char i;
  RA_TEMPLATE *RA_template = (RA_TEMPLATE *)&RC.mac[module_idP]->common_channels[CC_id].RA_template[0];

  MSC_LOG_EVENT(MSC_PHY_ENB, "RA Cancelling procedure ue %"PRIx16" ", rnti);
  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Cancelling RA procedure for UE rnti %x\n",module_idP,CC_id,frameP,rnti);

  for (i=0; i<NB_RA_PROC_MAX; i++) {
    if (rnti == RA_template[i].rnti) {
      RA_template[i].RA_active=FALSE;
      RA_template[i].generate_rar=0;
      RA_template[i].generate_Msg4=0;
      RA_template[i].wait_ack_Msg4=0;
      RA_template[i].timing_offset=0;
      RA_template[i].RRC_timer=20;
      RA_template[i].rnti = 0;
      RA_template[i].msg3_round = 0;
    }
  }
}

