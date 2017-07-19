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
// eMTC notes
// Implements MSG2 and MSG4
// fill_rar for eMTC
// new RA_templates for eMTC UEs, CCCH_BR instead of CCCH
// resource allocation for format 1A (4->6 PRBs)
// initiate_ra_proc: add detection of BR/CE UEs

void check_and_add_msg3(module_id_t module_idP,frame_t frameP, sub_frame_t subframeP) {

  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  RA_TEMPLATE                     *RA_template;
  COMMON_channels_t               *cc  = eNB->common_channels;
  uint8_t                         i;
  int msg3_prog_subframe,msg3_prog_frame;
  int CC_id;
  nfapi_ul_config_request_pdu_t  *ul_config_pdu;
  nfapi_ul_config_request_body_t *ul_req;
  nfapi_hi_dci0_request_body_t   *hi_dci0_req;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    hi_dci0_req   = &eNB->HI_DCI0_req[CC_id].hi_dci0_request_body;
    ul_req        = &eNB->UL_req[CC_id].ul_config_request_body;

    for (i=0; i<NB_RA_PROC_MAX; i++) {

      RA_template = (RA_TEMPLATE *)&cc[CC_id].RA_template[i];

      if (RA_template->RA_active == TRUE) {


	msg3_prog_subframe =  (RA_template->Msg3_subframe + 6)%10;
	
	if (RA_template->Msg3_subframe<4) msg3_prog_frame=(RA_template->Msg3_frame+1023)&1023;
	else                              msg3_prog_frame=RA_template->Msg3_frame;
        LOG_I(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d CC_id %d RA %d is active, Msg3 in (%d,%d), programmed in (%d,%d)\n",
              module_idP,frameP,subframeP,CC_id,i,RA_template->Msg3_frame,RA_template->Msg3_subframe,
	      msg3_prog_frame,msg3_prog_subframe);

	if ((msg3_prog_frame==frameP) && 
	    (msg3_prog_subframe==subframeP)) {
	  LOG_I(MAC,"Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d)\n",
		frameP,subframeP,RA_template->Msg3_frame,RA_template->Msg3_subframe);
	  eNB->UL_req[CC_id].sfn_sf                                                      = (RA_template->Msg3_frame<<3) + RA_template->Msg3_subframe;
	  if (RA_template->msg3_round == 0) { // program ULSCH
	    ul_config_pdu                                                                  = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus]; 
	    
	    memset((void*)ul_config_pdu,0,sizeof(nfapi_ul_config_request_pdu_t));
	    ul_config_pdu->pdu_type                                                        = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE; 
	    ul_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_ul_config_ulsch_pdu));
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.handle                                 = eNB->ul_handle++;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti                                   = RA_template->rnti;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.resource_block_start                   = 1;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks              = 1;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.modulation_type                        = 2;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.cyclic_shift_2_for_drms                = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_enabled_flag         = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_bits                 = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.new_data_indication                    = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.redundancy_version                     = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number                    = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.ul_tx_mode                             = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.current_tx_nb                          = 0;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.n_srs                                  = 1;
	    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size                                   = get_TBS_UL(10,ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks);
	    ul_req->number_of_pdus++;
	  }
	  else { // program HI
	    hi_dci0_pdu                                                         = &hi_dci0_req->hi_dci0_pdu_list[hi_dci0_req->number_of_dci+hi_dci0_req->number_of_hi]; 	
	    memset((void*)hi_dci0_pdu,0,sizeof(nfapi_hi_dci0_request_pdu_t));
	    hi_dci0_pdu->pdu_type                                               = NFAPI_HI_DCI0_HI_PDU_TYPE; 
	    hi_dci0_pdu->pdu_size                                               = 2+sizeof(nfapi_hi_dci0_hi_pdu);
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start                = 1; // note this is hard-coded like in fill_rar
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms             = 0;
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value                            = 0;
	    hi_dci0_req->number_of_hi++;
	    
	    LOG_I(MAC,"[eNB %d][PUSCH-RA %x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) RA %d (mcs %d, first rb %d, nb_rb %d,round %d)\n",
		  module_idP,RA_template[i].rnti,CC_id,frameP,subframeP,i,10,
		  1,1,
		  RA_template[i].msg3_round-1);
	  }
	}
      }
    }
  }
}

void schedule_RA(module_id_t module_idP,frame_t frameP, sub_frame_t subframeP,unsigned char Msg3_subframe)
{

  int                             CC_id;
  eNB_MAC_INST                    *eNB = RC.mac[module_idP];
  COMMON_channels_t               *cc  = eNB->common_channels;
  RA_TEMPLATE                     *RA_template;
  uint8_t                         i;
  int16_t                         rrc_sdu_length;
  uint8_t                         lcid;
  uint8_t                         offset;
  int                             UE_id           = -1;
  int16_t                         TBsize          = -1;
  uint16_t                        msg4_padding;
  uint16_t                        msg4_post_padding;
  uint16_t                        msg4_header;
  uint8_t                         *vrb_map;
  int                             first_rb;
  int                             N_RB_DL;
  nfapi_dl_config_request_pdu_t   *dl_config_pdu;
  nfapi_tx_request_pdu_t          *TX_req;
  UE_list_t                       *UE_list=&eNB->UE_list;
  int round;
  nfapi_dl_config_request_body_t *dl_req;

  start_meas(&eNB->schedule_ra);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    vrb_map       = cc[CC_id].vrb_map;

    dl_req        = &eNB->DL_req[CC_id].dl_config_request_body;
    dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
    N_RB_DL       = to_prb(cc[CC_id].mib->message.dl_Bandwidth);

    for (i=0; i<NB_RA_PROC_MAX; i++) {

      RA_template = (RA_TEMPLATE *)&cc[CC_id].RA_template[i];

      if (RA_template->RA_active == TRUE) {

        LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d RA %d is active (generate RAR %d, generate_Msg4 %d, wait_ack_Msg4 %d, rnti %x)\n",
              module_idP,CC_id,i,RA_template->generate_rar,RA_template->generate_Msg4,RA_template->wait_ack_Msg4, RA_template->rnti);

        if (RA_template->generate_rar == 1) {

          LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generating RAR DCI (proc %d), RA_active %d format 1A (%d,%d))\n",
                module_idP, CC_id, frameP, subframeP,i,
                RA_template->RA_active,
                RA_template->RA_dci_fmt1,
                RA_template->RA_dci_size_bits1);

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

	  if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,RA_template->RA_rnti)) {
	    LOG_D(MAC,"Frame %d: Subframe %d : Adding common DCI for RA_RNTI %x\n",
                  frameP,subframeP,RA_template->RA_rnti);
	    dl_req->number_dci++;
	    dl_req->number_pdu++;

	    dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
	    memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	    dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE; 
	    dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
	    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_id];
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
	    get_Msg3alloc(&cc[CC_id],subframeP,frameP,&RA_template->Msg3_frame,&RA_template->Msg3_subframe);


	    fill_rar(module_idP,CC_id,frameP,cc[CC_id].RAR_pdu.payload,N_RB_DL,7);
	    // DL request
	    eNB->TX_req[CC_id].sfn_sf                                             = (frameP<<3)+subframeP;
	    TX_req                                                                = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus]; 		     
	    TX_req->pdu_length                                                    = 7;  // This should be changed if we have more than 1 preamble 
	    TX_req->pdu_index                                                     = eNB->pdu_index[CC_id]++;
	    TX_req->num_segments                                                  = 1;
	    TX_req->segments[0].segment_length                                    = 7;
	    TX_req->segments[0].segment_data                                      = cc[CC_id].RAR_pdu.payload;
	    eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
	  }
        } else if (RA_template->generate_Msg4 == 1) {

          // check for Msg4 Message
          UE_id = find_UE_id(module_idP,RA_template->rnti);
	  AssertFatal(UE_id>=0,"Can't find UE for t-crnti\n");


	  // Get RRCConnectionSetup for Piggyback
	  rrc_sdu_length = mac_rrc_data_req(module_idP,
					    CC_id,
					    frameP,
					    CCCH,
					    1, // 1 transport block
					    &cc[CC_id].CCCH_pdu.payload[0],
					    ENB_FLAG_YES,
					    module_idP,
					    0); // not used in this case
	  
	  AssertFatal(rrc_sdu_length>=0,
			"[MAC][eNB Scheduler] CCCH not allocated\n");
	

          LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: UE_id %d, rrc_sdu_length %d\n",
                module_idP, CC_id, frameP, subframeP,UE_id, rrc_sdu_length);

          if (rrc_sdu_length>0) {
            LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating Msg4 with RRC Piggyback (RA proc %d, RNTI %x)\n",
                  module_idP, CC_id, frameP, subframeP,i,RA_template->rnti);

	    first_rb=0;

	    vrb_map[first_rb] = 1;
	    vrb_map[first_rb+1] = 1;
	    vrb_map[first_rb+2] = 1;
	    vrb_map[first_rb+3] = 1;


	    memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	    dl_config_pdu->pdu_type                                                          = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
	    dl_config_pdu->pdu_size                                                          = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                             = NFAPI_DL_DCI_FORMAT_1A;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level                      = 4;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                                   = RA_template->rnti;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                              = 1;    // C-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power                     = 6000; // equal to RS power
	    
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                           = 0;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                                    = 1; // no TPC
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1                   = 1;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1                   = 0;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;

            // Compute MCS for 3 PRB
            msg4_header = 1+6+1;  // CR header, CR CE, SDU header


	    if ((rrc_sdu_length+msg4_header) <= 22) {
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 4;
	      TBsize = 22;
	    } else if ((rrc_sdu_length+msg4_header) <= 28) {
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 5;
	      TBsize = 28;
	    } else if ((rrc_sdu_length+msg4_header) <= 32) {
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 6;
	      TBsize = 32;
	    } else if ((rrc_sdu_length+msg4_header) <= 41) {
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 7;
	      TBsize = 41;
	    } else if ((rrc_sdu_length+msg4_header) <= 49) {
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 8;
	      TBsize = 49;
	    } else if ((rrc_sdu_length+msg4_header) <= 57) {
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 9;
	      TBsize = 57;
	    }

	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding= getRIV(N_RB_DL,first_rb,4);

	    if (!CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,RA_template->rnti)) {
	      dl_req->number_dci++;
	      dl_req->number_pdu++;
		      
	      RA_template->generate_Msg4=0;
	      RA_template->wait_ack_Msg4=1;
	      RA_template->RA_active = FALSE;
	      lcid=0;

	      // set HARQ process 0 round to 0 for this UE
	      UE_list->UE_sched_ctrl[UE_id].round[CC_id] = 0;

	      if ((TBsize - rrc_sdu_length - msg4_header) <= 2) {
		msg4_padding = TBsize - rrc_sdu_length - msg4_header;
		msg4_post_padding = 0;
	      } else {
		msg4_padding = 0;
		msg4_post_padding = TBsize - rrc_sdu_length - msg4_header -1;
	      }
	      
	      LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d subframeP %d Msg4 : TBS %d, sdu_len %d, msg4_header %d, msg4_padding %d, msg4_post_padding %d\n",
		    module_idP,CC_id,frameP,subframeP,TBsize,rrc_sdu_length,msg4_header,msg4_padding,msg4_post_padding);
	      DevAssert( UE_id != UE_INDEX_INVALID ); // FIXME not sure how to gracefully return
	      // CHECK THIS: &cc[CC_id].CCCH_pdu.payload[0]
	      offset = generate_dlsch_header((unsigned char*)eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0],
					     1,                           //num_sdus
					     (unsigned short*)&rrc_sdu_length,             //
					     &lcid,                       // sdu_lcid
					     255,                         // no drx
					     0,                           // no timing advance
					     RA_template->cont_res_id,  // contention res id
					     msg4_padding,                // no padding
					     msg4_post_padding);
	      
	      memcpy((void*)&eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0][(unsigned char)offset],
		     &cc[CC_id].CCCH_pdu.payload[0],
		     rrc_sdu_length);

	    // DL request
	      eNB->TX_req[CC_id].sfn_sf                                             = (frameP<<3)+subframeP;
	      TX_req                                                                = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus]; 		     	      
	      TX_req->pdu_length                                                    = rrc_sdu_length;
	      TX_req->pdu_index                                                     = eNB->pdu_index[CC_id]++;
	      TX_req->num_segments                                                  = 1;
	      TX_req->segments[0].segment_length                                    = rrc_sdu_length;
	      TX_req->segments[0].segment_data                                      = eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0];
	      eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
	      
              T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(RA_template->rnti), T_INT(frameP), T_INT(subframeP),
                T_INT(0 /*harq_pid always 0?*/), T_BUFFER(&eNB->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0], TBsize));
	      
	      if (opt_enabled==1) {
		trace_pdu(1, (uint8_t *)eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0],
			  rrc_sdu_length, UE_id, 3, UE_RNTI(module_idP, UE_id),
			  eNB->frame, eNB->subframe,0,0);
		LOG_D(OPT,"[eNB %d][DLSCH] CC_id %d Frame %d trace pdu for rnti %x with size %d\n",
		      module_idP, CC_id, frameP, UE_RNTI(module_idP,UE_id), rrc_sdu_length);
	      }
	      
	    }
	  }

          //try here
        }

      } else if (RA_template->wait_ack_Msg4==1) {
	// check HARQ status and retransmit if necessary
	LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Checking if Msg4 was acknowledged: \tn",
	      module_idP,CC_id,frameP,subframeP);
	// Get candidate harq_pid from PHY

	UE_id = find_UE_id(module_idP,RA_template->rnti);
	AssertFatal(UE_id>=0,"Can't find UE for t-crnti\n");
	round = UE_list->UE_sched_ctrl[UE_id].round[CC_id];

	if (round>0) {
	  //RA_template->wait_ack_Msg4++;
	  // we have to schedule a retransmission
	  
	  first_rb=0;
	  vrb_map[first_rb] = 1;
	  vrb_map[first_rb+1] = 1;
	  vrb_map[first_rb+2] = 1;
	  vrb_map[first_rb+3] = 1;

	  memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	  dl_config_pdu->pdu_type                                                          = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
	  dl_config_pdu->pdu_size                                                          = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                             = NFAPI_DL_DCI_FORMAT_1A;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level                      = 4;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                                   = RA_template->rnti;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                              = 1;    // C-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power                     = 6000; // equal to RS power
	  
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                           = 0;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                                    = 1; // no TPC
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1                   = 1;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1                   = 0;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
	  
	  // Compute MCS for 3 PRB
	  msg4_header = 1+6+1;  // CR header, CR CE, SDU header
	  
	  
	  if ((rrc_sdu_length+msg4_header) <= 22) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 4;
	    TBsize = 22;
	  } else if ((rrc_sdu_length+msg4_header) <= 28) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 5;
	    TBsize = 28;
	  } else if ((rrc_sdu_length+msg4_header) <= 32) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 6;
	    TBsize = 32;
	  } else if ((rrc_sdu_length+msg4_header) <= 41) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 7;
	    TBsize = 41;
	  } else if ((rrc_sdu_length+msg4_header) <= 49) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 8;
	    TBsize = 49;
	  } else if ((rrc_sdu_length+msg4_header) <= 57) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = 9;
	    TBsize = 57;
	  }
	  
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding= getRIV(N_RB_DL,first_rb,4);
	  
	  if (!CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,RA_template->rnti)) {
	    dl_req->number_dci++;
	    dl_req->number_pdu++;
		  
	    LOG_I(MAC,"msg4 retransmission for rnti %x (round %d) fsf %d/%d\n", RA_template->rnti, round, frameP, subframeP);
	  }
	  else
	    LOG_I(MAC,"msg4 retransmission for rnti %x (round %d) fsf %d/%d CCE allocation failed!\n", RA_template->rnti, round, frameP, subframeP);
	  LOG_W(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Msg4 not acknowledged, adding ue specific dci (rnti %x) for RA (Msg4 Retransmission)\n",
		module_idP,CC_id,frameP,subframeP,RA_template->rnti);
	} else {
	  LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d : Msg4 acknowledged\n",module_idP,CC_id,frameP,subframeP);
	  RA_template->wait_ack_Msg4=0;
	  RA_template->RA_active=FALSE;
	  UE_id = find_UE_id(module_idP,RA_template->rnti);
	  DevAssert( UE_id != -1 );
	  eNB->UE_list.UE_template[UE_PCCID(module_idP,UE_id)][UE_id].configured=TRUE;
	}
      }
    } // for i=0 .. N_RA_PROC-1 
  } // CC_id

  stop_meas(&eNB->schedule_ra);
}

// handles the event of MSG1 reception
void initiate_ra_proc(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframeP,uint16_t preamble_index,int16_t timing_offset,uint16_t ra_rnti)
{

  uint8_t i;
  RA_TEMPLATE *RA_template = (RA_TEMPLATE *)&RC.mac[module_idP]->common_channels[CC_id].RA_template[0];

  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Initiating RA procedure for preamble index %d\n",module_idP,CC_id,frameP,preamble_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC,1);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC,0);

  for (i=0; i<NB_RA_PROC_MAX; i++) {
    if (RA_template[i].RA_active==FALSE &&
        RA_template[i].wait_ack_Msg4 == 0) {
      int loop = 0;
      RA_template[i].RA_active         = TRUE;
      RA_template[i].generate_rar      = 1;
      RA_template[i].generate_Msg4     = 0;
      RA_template[i].wait_ack_Msg4     = 0;
      RA_template[i].timing_offset     = timing_offset;
      RA_template[i].preamble_subframe = subframeP;
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
      LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Activating RAR generation for process %d, rnti %x, RA_active %d\n",
            module_idP,CC_id,frameP,i,RA_template[i].rnti,
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

