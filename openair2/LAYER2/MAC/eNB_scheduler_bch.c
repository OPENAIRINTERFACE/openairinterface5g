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

/*! \file eNB_scheduler_bch.c
 * \brief procedures related to eNB for the BCH transport channel
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

// NEED TO ADD schedule_SI_BR for SIB1_BR and SIB23_BR
// CCE_allocation_infeasible to be done for EPDCCH/MPDCCH

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

#define size_Sj25 2
int Sj25[size_Sj25] = { 0, 3 };

#define size_Sj50 6
int Sj50[size_Sj50] = { 0, 1, 2, 5, 6, 7 };

#define size_Sj75 10
int Sj75[size_Sj75] = { 0, 1, 2, 3, 4, 7, 8, 9, 10, 11 };

#define size_Sj100 14
int Sj100[size_Sj100] = { 0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15 };

int SIB1_BR_TBS_table[6] = { 208, 256, 328, 504, 712, 936 };

//------------------------------------------------------------------------------
void
schedule_SIB1_BR(module_id_t module_idP,
		 frame_t frameP, sub_frame_t subframeP)
//------------------------------------------------------------------------------
{
    int8_t bcch_sdu_length;
    int CC_id;
    eNB_MAC_INST *eNB = RC.mac[module_idP];
    COMMON_channels_t *cc;
    uint8_t *vrb_map;
    int first_rb = -1;
    int N_RB_DL;
    nfapi_dl_config_request_pdu_t *dl_config_pdu;
    nfapi_tx_request_pdu_t *TX_req;
    nfapi_dl_config_request_body_t *dl_req;
    int m, i, N_S_NB;
    int *Sj;
    int n_NB = 0;
    int TBS;
    int k = 0, rvidx;
    uint16_t sfn_sf = frameP<<4|subframeP;

    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

	cc = &eNB->common_channels[CC_id];
	vrb_map = (void *) &cc->vrb_map;
	N_RB_DL = to_prb(cc->mib->message.dl_Bandwidth);
	dl_req = &eNB->DL_req[CC_id].dl_config_request_body;

	int foffset = cc->physCellId & 1;
	int sfoffset = (cc->tdd_Config == NULL) ? 0 : 1;

	// Time-domain scheduling
	if (cc->mib->message.schedulingInfoSIB1_BR_r13 == 0)
	    continue;
	else
	    switch ((cc->mib->message.schedulingInfoSIB1_BR_r13 - 1) % 3) {
	    case 0:		// repetition 4
		k = (frameP >> 1) & 3;
		if ((subframeP != (4 + sfoffset))
		    || ((frameP & 1) != foffset))
		    continue;
		break;
	    case 1:		// repetition 8
		k = frameP & 3;
		AssertFatal(N_RB_DL > 15,
			    "SIB1-BR repetition 8 not allowed for N_RB_DL= %d\n",
			    N_RB_DL);
		if ((foffset == 0) && (subframeP != (4 + sfoffset)))
		    continue;
		else if ((foffset == 1)
			 && (subframeP != ((9 + sfoffset) % 10)))
		    continue;
		break;
	    case 2:		// repetition 16
		k = ((10 * frameP) + subframeP) & 3;
		AssertFatal(N_RB_DL > 15,
			    "SIB1-BR repetition 16 not allowed for N_RB_DL= %d\n",
			    N_RB_DL);
		if ((sfoffset == 1)
		    && ((subframeP != 0) || (subframeP != 5)))
		    continue;
		else if ((sfoffset == 0) && (foffset == 0)
			 && (subframeP != 4) && (subframeP != 9))
		    continue;
		else if ((sfoffset == 0) && (foffset == 1)
			 && (subframeP != 0) && (subframeP != 9))
		    continue;
		break;
	    }
	// if we get here we have to schedule SIB1_BR in this frame/subframe

	// keep counter of SIB1_BR repetitions in 8 frame period to choose narrowband on which to transmit
	if ((frameP & 7) == 0)
	    cc->SIB1_BR_cnt = 0;
	else
	    cc->SIB1_BR_cnt++;

	// Frequency-domain scheduling
	switch (N_RB_DL) {
	case 6:
	case 15:
	default:
	    m = 1;
	    n_NB = 0;
	    N_S_NB = 0;
	    Sj = NULL;
	    break;
	case 25:
	    m = 2;
	    N_S_NB = 2;
	    Sj = Sj25;
	    break;
	case 50:
	    m = 2;
	    N_S_NB = 6;
	    Sj = Sj50;
	    break;
	case 75:
	    m = 4;
	    N_S_NB = 10;
	    Sj = Sj75;
	    break;
	case 100:
	    m = 4;
	    N_S_NB = 14;
	    Sj = Sj100;
	    break;
	}
	// Note: definition of k above and rvidx from 36.321 section 5.3.1
	rvidx = (((3 * k) >> 1) + (k & 1)) & 3;

	i = cc->SIB1_BR_cnt & (m - 1);

	n_NB = Sj[((cc->physCellId % N_S_NB) + (i * N_S_NB / m)) % N_S_NB];


	bcch_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, BCCH_SIB1_BR, 0xFFFF, 1, &cc->BCCH_BR_pdu[0].payload[0], 0);	// not used in this case

	AssertFatal(cc->mib->message.schedulingInfoSIB1_BR_r13 < 19,
		    "schedulingInfoSIB1_BR_r13 %d > 18\n",
		    (int) cc->mib->message.schedulingInfoSIB1_BR_r13);

	AssertFatal(bcch_sdu_length > 0,
		    "RRC returned 0 bytes for SIB1-BR\n");

	TBS =
	    SIB1_BR_TBS_table[(cc->mib->message.schedulingInfoSIB1_BR_r13 -
			       1) / 3] >> 3;

	AssertFatal(bcch_sdu_length <= TBS,
		    "length returned by RRC %d is not compatible with the TBS %d from MIB\n",
		    bcch_sdu_length, TBS);

	if ((frameP & 1023) < 200)
	    LOG_D(MAC,
		  "[eNB %d] Frame %d Subframe %d: SIB1_BR->DLSCH CC_id %d, Received %d bytes, scheduling on NB %d (i %d,m %d,N_S_NB %d)  rvidx %d\n",
		  module_idP, frameP, subframeP, CC_id, bcch_sdu_length,
		  n_NB, i, m, N_S_NB, rvidx);

	// allocate all 6 PRBs in narrowband for SIB1_BR

	first_rb = narrowband_to_first_rb(cc, n_NB);

	vrb_map[first_rb] = 1;
	vrb_map[first_rb + 1] = 1;
	vrb_map[first_rb + 2] = 1;
	vrb_map[first_rb + 3] = 1;
	vrb_map[first_rb + 4] = 1;
	vrb_map[first_rb + 5] = 1;

	dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	memset((void *) dl_config_pdu, 0,
	       sizeof(nfapi_dl_config_request_pdu_t));
	dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
	dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dlsch_pdu));
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.length = TBS;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index = eNB->pdu_index[CC_id];
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti = 0xFFFF;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type = 2;	// format 1A/1B/1D
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;	// localized
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding = getRIV(N_RB_DL, first_rb, 6);
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation = 2;	//QPSK
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version = rvidx;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks = 1;	// first block
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme = (cc->p_eNB == 1) ? 0 : 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers = 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands = 1;
	//  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity = 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa = 4;	// 0 dB
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap = 0;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb = get_subbandsize(cc->mib->message.dl_Bandwidth);	// ignored
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode = (cc->p_eNB == 1) ? 1 : 2;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband = 1;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector = 1;
	// Rel10 fields
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start = 3;
	// Rel13 fields
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG;
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type = 1;	// CEModeA UE
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type = 0;	// SIB1-BR
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io = 0xFFFF;	// absolute SFx

	//  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ; 
	dl_req->number_pdu++;
        dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;

	// Program TX Request
	TX_req = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus];
	TX_req->pdu_length = bcch_sdu_length;
	TX_req->pdu_index = eNB->pdu_index[CC_id]++;
	TX_req->num_segments = 1;
	TX_req->segments[0].segment_length = bcch_sdu_length;
	TX_req->segments[0].segment_data = cc->BCCH_BR_pdu[0].payload;
        eNB->TX_req[CC_id].sfn_sf = sfn_sf;
        eNB->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
        eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
        eNB->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;

	if (opt_enabled == 1) {
	    trace_pdu(DIRECTION_DOWNLINK,
		      &cc->BCCH_BR_pdu[0].payload[0],
		      bcch_sdu_length,
		      0xffff, WS_SI_RNTI, 0xffff, eNB->frame, eNB->subframe, 0, 0);
	    LOG_D(OPT,
		  "[eNB %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
		  module_idP, frameP, CC_id, 0xffff, bcch_sdu_length);
	}
	if (cc->tdd_Config != NULL) {	//TDD
	    LOG_D(MAC,
		  "[eNB] Frame %d : Scheduling BCCH-BR 0->DLSCH (TDD) for CC_id %d SIB1-BR %d bytes\n",
		  frameP, CC_id, bcch_sdu_length);
	} else {
	    LOG_D(MAC,
		  "[eNB] Frame %d : Scheduling BCCH-BR 0->DLSCH (FDD) for CC_id %d SIB1-BR %d bytes\n",
		  frameP, CC_id, bcch_sdu_length);
	}
    }
}

int si_WindowLength_BR_r13tab[LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__si_WindowLength_BR_r13_spare] = { 20, 40, 60, 80, 120, 160, 200 };
int si_TBS_r13tab[LTE_SchedulingInfo_BR_r13__si_TBS_r13_b936 + 1] = { 152, 208, 256, 328, 408, 504, 600, 712, 808, 936 };

//------------------------------------------------------------------------------
void
schedule_SI_BR(module_id_t module_idP, frame_t frameP,
	       sub_frame_t subframeP)
//------------------------------------------------------------------------------
{
  int8_t                                  bcch_sdu_length;
  int                                     CC_id;
  eNB_MAC_INST                            *eNB = RC.mac[module_idP];
  COMMON_channels_t                       *cc;
  uint8_t                                 *vrb_map;
  int                                     first_rb = -1;
  int                                     N_RB_DL;
  nfapi_dl_config_request_pdu_t           *dl_config_pdu;
  nfapi_tx_request_pdu_t                  *TX_req;
  nfapi_dl_config_request_body_t          *dl_req;
  int                                     i;
  int                                     rvidx;
  int                                     absSF = (frameP*10)+subframeP;


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    cc              = &eNB->common_channels[CC_id];
    vrb_map         = (void*)&cc->vrb_map;
    N_RB_DL         = to_prb(cc->mib->message.dl_Bandwidth);
    dl_req          = &eNB->DL_req[CC_id].dl_config_request_body;

    // Time-domain scheduling
    if (cc->mib->message.schedulingInfoSIB1_BR_r13==0) continue;
    else  {


      AssertFatal(cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13!=NULL,
		  "sib_v13ext->bandwidthReducedAccessRelatedInfo_r13 is null\n");

      LTE_SchedulingInfoList_BR_r13_t *schedulingInfoList_BR_r13 = cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->schedulingInfoList_BR_r13;
      AssertFatal(schedulingInfoList_BR_r13!=NULL,
		  "sib_v13ext->schedulingInfoList_BR_r13 is null\n");

      LTE_SchedulingInfoList_t *schedulingInfoList = cc->schedulingInfoList;
      AssertFatal(schedulingInfoList_BR_r13->list.count==schedulingInfoList->list.count,
		  "schedulingInfolist_BR.r13->list.count %d != schedulingInfoList.list.count %d\n",
		  schedulingInfoList_BR_r13->list.count,schedulingInfoList->list.count);

      AssertFatal(cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->si_WindowLength_BR_r13<=LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__si_WindowLength_BR_r13_ms200,
		  "si_WindowLength_BR_r13 %d > %d\n",
		  (int)cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->si_WindowLength_BR_r13,
		  LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__si_WindowLength_BR_r13_ms200);

      // check that SI frequency-hopping is disabled
      AssertFatal(cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->si_HoppingConfigCommon_r13==LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__si_HoppingConfigCommon_r13_off,
		  "Deactivate SI_HoppingConfigCommon_r13 in configuration file, not supported for now\n");
      long si_WindowLength_BR_r13   = si_WindowLength_BR_r13tab[cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->si_WindowLength_BR_r13];

      long si_RepetitionPattern_r13 = cc->sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->si_RepetitionPattern_r13;
      AssertFatal(si_RepetitionPattern_r13<=LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__si_RepetitionPattern_r13_every8thRF,
		  "si_RepetitionPattern_r13 %d > %d\n",
		  (int)si_RepetitionPattern_r13,
		  LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__si_RepetitionPattern_r13_every8thRF);
      // cycle through SIB list

      for (i=0;i<schedulingInfoList_BR_r13->list.count;i++) {
        long si_Periodicity = schedulingInfoList->list.array[i]->si_Periodicity;
        long si_Narrowband_r13 = schedulingInfoList_BR_r13->list.array[i]->si_Narrowband_r13;
        long si_TBS_r13 = si_TBS_r13tab[schedulingInfoList_BR_r13->list.array[i]->si_TBS_r13];

        // check if the SI is to be scheduled now
        int period_in_sf;
        if ((si_Periodicity >= 0) && (si_Periodicity < 25)) {
          // 2^i * 80 subframes, note: si_Periodicity is 2^i * 80ms
          period_in_sf = 80 << ((int) si_Periodicity);
        } else if (si_Periodicity < 0) {
          period_in_sf = 80;
        } else if (si_Periodicity > 24) {
          period_in_sf = 80 << 24;
        }
        int sf_mod_period = absSF % period_in_sf;
        int k = sf_mod_period & 3;
        // Note: definition of k and rvidx from 36.321 section 5.3.1
        rvidx = (((3 * k) >> 1) + (k & 1)) & 3;

        if ((sf_mod_period < si_WindowLength_BR_r13)
            && ((frameP & (((1 << si_RepetitionPattern_r13) - 1))) == 0)) {	// this SIB is to be scheduled

          bcch_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, BCCH_SI_BR + i, 0xFFFF, 1, &cc->BCCH_BR_pdu[i + 1].payload[0], 0);	// not used in this case


          AssertFatal(bcch_sdu_length > 0,
          "RRC returned 0 bytes for SI-BR %d\n", i);

          if (bcch_sdu_length > 0) {
            AssertFatal(bcch_sdu_length <= (si_TBS_r13 >> 3),
                        "RRC provided bcch with length %d > %d (si_TBS_r13 %d)\n",
                        bcch_sdu_length,
                        (int) (si_TBS_r13 >> 3),
                        (int) schedulingInfoList_BR_r13->list.array[i]->si_TBS_r13);

            // allocate all 6 PRBs in narrowband for SIB1_BR

            // check that SIB1 didn't take this narrowband
            if (vrb_map[first_rb] > 0) continue;

            first_rb = narrowband_to_first_rb(cc,si_Narrowband_r13 - 1);
            vrb_map[first_rb] = 1;
            vrb_map[first_rb + 1] = 1;
            vrb_map[first_rb + 2] = 1;
            vrb_map[first_rb + 4] = 1;
            vrb_map[first_rb + 5] = 1;

            if ((frameP&1023) < 200)
              LOG_D(MAC,"[eNB %d] Frame %d Subframe %d: SI_BR->DLSCH CC_id %d, Narrowband %d rvidx %d (sf_mod_period %d : si_WindowLength_BR_r13 %d : si_RepetitionPattern_r13 %d) bcch_sdu_length %d\n",
                   module_idP,frameP,subframeP,CC_id,(int)si_Narrowband_r13-1,rvidx,
                   sf_mod_period,(int)si_WindowLength_BR_r13,(int)si_RepetitionPattern_r13,
                   bcch_sdu_length);


            //// Rel10 fields (for PDSCH starting symbol)
            //dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start = cc[CC_id].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
            //// Rel13 fields
            //                  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG;
            //dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type = 1;	// CEModeA UE
            //dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type = 1;	// SI-BR
            //dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io = absSF - sf_mod_period;

            ////  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
            //dl_req->number_pdu++;
            //                  dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;

            dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
            memset((void *) dl_config_pdu, 0, sizeof(nfapi_dl_config_request_pdu_t));
            dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
            dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.length                                 = si_TBS_r13>>3;
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_id];
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = 0xFFFF;
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;   // format 1A/1B/1D
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;   // localized
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL,first_rb,6);
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation                             = 2; //QPSK
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version                     = rvidx;
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
            // Rel10 fields (for PDSCH starting symbol)
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start                           = cc[CC_id].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
            // Rel13 fields
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type                               = 1; // CEModeA UE
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type                    = 1; // SI-BR
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io            = absSF - sf_mod_period;

            //	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
            dl_req->number_pdu++;

            // Program TX Request
            TX_req                                                                = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus];
            TX_req->pdu_length                                                    = bcch_sdu_length;
            TX_req->pdu_index                                                     = eNB->pdu_index[CC_id]++;
            TX_req->num_segments                                                  = 1;
            TX_req->segments[0].segment_length                                    = bcch_sdu_length;
            TX_req->segments[0].segment_data                                      = cc->BCCH_BR_pdu[i+1].payload;
            eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;

            if (opt_enabled == 1) {
                trace_pdu(DIRECTION_DOWNLINK,
                    &cc->BCCH_BR_pdu[i + 1].payload[0],
                    bcch_sdu_length,
                    0xffff,
                    WS_SI_RNTI,
                    0xffff, eNB->frame, eNB->subframe, 0,
                    0);
                LOG_D(OPT, "[eNB %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
                      module_idP, frameP, CC_id, 0xffff, bcch_sdu_length);
            }
            if (cc->tdd_Config != NULL) {	//TDD
                LOG_D(MAC, "[eNB] Frame %d : Scheduling BCCH-BR %d->DLSCH (TDD) for CC_id %d SI-BR %d bytes\n",
                      frameP, i, CC_id, bcch_sdu_length);
            } else {
                LOG_D(MAC, "[eNB] Frame %d : Scheduling BCCH-BR %d->DLSCH (FDD) for CC_id %d SI-BR %d bytes\n",
                      frameP, i, CC_id, bcch_sdu_length);
            }
          }
        }		// scheduling in current frame/subframe
	    }			//for SI List
    }			// eMTC is activated
  }				// CC_id
  return;
}
#endif

void
schedule_mib(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
    eNB_MAC_INST *eNB = RC.mac[module_idP];
    COMMON_channels_t *cc;
    nfapi_dl_config_request_pdu_t *dl_config_pdu;
    nfapi_tx_request_pdu_t *TX_req;
    int mib_sdu_length;
    int CC_id;
    nfapi_dl_config_request_t *dl_config_request;
    nfapi_dl_config_request_body_t *dl_req;
    uint16_t sfn_sf = frameP << 4 | subframeP;

    AssertFatal(subframeP == 0, "Subframe must be 0\n");
    AssertFatal((frameP & 3) == 0, "Frame must be a multiple of 4\n");

    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

	dl_config_request = &eNB->DL_req[CC_id];
	dl_req = &dl_config_request->dl_config_request_body;
	cc = &eNB->common_channels[CC_id];

	mib_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, MIBCH, 0xFFFF, 1, &cc->MIB_pdu.payload[0], 0);	// not used in this case

	LOG_D(MAC, "Frame %d, subframe %d: BCH PDU length %d\n", frameP, subframeP, mib_sdu_length);

	if (mib_sdu_length > 0) {

	    LOG_D(MAC, "Frame %d, subframe %d: Adding BCH PDU in position %d (length %d)\n", frameP, subframeP, dl_req->number_pdu, mib_sdu_length);

	    if ((frameP & 1023) < 40)
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
		LOG_D(MAC,
		      "[eNB %d] Frame %d : MIB->BCH  CC_id %d, Received %d bytes (cc->mib->message.schedulingInfoSIB1_BR_r13 %d)\n",
		      module_idP, frameP, CC_id, mib_sdu_length,
		      (int) cc->mib->message.schedulingInfoSIB1_BR_r13);
#else
		LOG_D(MAC,
		      "[eNB %d] Frame %d : MIB->BCH  CC_id %d, Received %d bytes\n",
		      module_idP, frameP, CC_id, mib_sdu_length);
#endif

	    dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	    memset((void *) dl_config_pdu, 0,
		   sizeof(nfapi_dl_config_request_pdu_t));
	    dl_config_pdu->pdu_type                                = NFAPI_DL_CONFIG_BCH_PDU_TYPE, dl_config_pdu->pdu_size =
		2 + sizeof(nfapi_dl_config_bch_pdu);
            dl_config_pdu->bch_pdu.bch_pdu_rel8.tl.tag             = NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG;
	    dl_config_pdu->bch_pdu.bch_pdu_rel8.length             = mib_sdu_length;
	    dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index          = eNB->pdu_index[CC_id];
	    dl_config_pdu->bch_pdu.bch_pdu_rel8.transmission_power = 6000;
            dl_req->tl.tag                                         = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
	    dl_req->number_pdu++;

            dl_config_request->header.message_id = NFAPI_DL_CONFIG_REQUEST;
            dl_config_request->sfn_sf = sfn_sf;

	    LOG_D(MAC, "eNB->DL_req[0].number_pdu %d (%p)\n", dl_req->number_pdu, &dl_req->number_pdu);
	    // DL request

	    TX_req = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus];
	    TX_req->pdu_length = 3;
	    TX_req->pdu_index = eNB->pdu_index[CC_id]++;
	    TX_req->num_segments = 1;
	    TX_req->segments[0].segment_length = 3;
	    TX_req->segments[0].segment_data = cc[CC_id].MIB_pdu.payload;
	    eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
            eNB->TX_req[CC_id].sfn_sf = sfn_sf;
            eNB->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
            eNB->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
	}
    }
}


//------------------------------------------------------------------------------
void
schedule_SI(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
//------------------------------------------------------------------------------
{

    int8_t bcch_sdu_length;
    int mcs = -1;
    int CC_id;
    eNB_MAC_INST *eNB = RC.mac[module_idP];
    COMMON_channels_t *cc;
    uint8_t *vrb_map;
    int first_rb = -1;
    int N_RB_DL;
    nfapi_dl_config_request_t      *dl_config_request;
    nfapi_dl_config_request_pdu_t  *dl_config_pdu;
    nfapi_tx_request_pdu_t *TX_req;
    nfapi_dl_config_request_body_t *dl_req;
    uint16_t sfn_sf = frameP << 4 | subframeP;

    start_meas(&eNB->schedule_si);

    // Only schedule LTE System Information in subframe 5
    if (subframeP == 5) {

	for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

	    cc = &eNB->common_channels[CC_id];
	    vrb_map = (void *) &cc->vrb_map;
	    N_RB_DL = to_prb(cc->mib->message.dl_Bandwidth);
            dl_config_request = &eNB->DL_req[CC_id];
	    dl_req = &eNB->DL_req[CC_id].dl_config_request_body;


	    bcch_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, BCCH, 0xFFFF,1, &cc->BCCH_pdu.payload[0], 0);	// not used in this case

	    if (bcch_sdu_length > 0) {
		LOG_D(MAC, "[eNB %d] Frame %d : BCCH->DLSCH CC_id %d, Received %d bytes \n", module_idP, frameP, CC_id, bcch_sdu_length);

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
		vrb_map[first_rb + 1] = 1;
		vrb_map[first_rb + 2] = 1;
		vrb_map[first_rb + 3] = 1;

		// Get MCS for length of SI, 3 PRBs
		if (bcch_sdu_length <= 7) {
		    mcs = 0;
		} else if (bcch_sdu_length <= 11) {
		    mcs = 1;
		} else if (bcch_sdu_length <= 18) {
		    mcs = 2;
		} else if (bcch_sdu_length <= 22) {
		    mcs = 3;
		} else if (bcch_sdu_length <= 26) {
		    mcs = 4;
		} else if (bcch_sdu_length <= 28) {
		    mcs = 5;
		} else if (bcch_sdu_length <= 32) {
		    mcs = 6;
		} else if (bcch_sdu_length <= 41) {
		    mcs = 7;
		} else if (bcch_sdu_length <= 49) {
		    mcs = 8;
		} else if (bcch_sdu_length <= 59) {
                    mcs = 9;
                } 
                else AssertFatal(1==0,"Cannot Assign mcs for bcch_sdu_length %d (max mcs 9)\n",bcch_sdu_length);

		dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
		memset((void *) dl_config_pdu, 0,
		       sizeof(nfapi_dl_config_request_pdu_t));
		dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
		dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
                dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format            = NFAPI_DL_DCI_FORMAT_1A;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level     = 4;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                  = 0xFFFF;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 2;	// S-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000;	// equal to RS power

		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process          = 0;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                   = 1;	// no TPC
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1  = 0;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                 = mcs;
		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1  = 0;

		dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = getRIV(N_RB_DL, first_rb, 4);
                dl_config_request->sfn_sf = sfn_sf;

		if (!CCE_allocation_infeasible(module_idP, CC_id, 0, subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level, SI_RNTI)) {
		    LOG_D(MAC, "Frame %d: Subframe %d : Adding common DCI for S_RNTI\n", frameP, subframeP);
		    dl_req->number_dci++;
		    dl_req->number_pdu++;
		    dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
		    memset((void *) dl_config_pdu, 0,
			   sizeof(nfapi_dl_config_request_pdu_t));
		    dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
		    dl_config_pdu->pdu_size                                                        = (uint8_t) (2 + sizeof(nfapi_dl_config_dlsch_pdu));
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_id];
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.tl.tag                                 = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.length                                 = bcch_sdu_length;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = 0xFFFF;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;	// format 1A/1B/1D
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;	// localized
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL, first_rb, 4);
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation                             = 2;	//QPSK
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version                     = 0;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks                       = 1;	// first block
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag  = 0;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme                    = (cc->p_eNB == 1) ? 0 : 1;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers                       = 1;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands                     = 1;
		    //    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity                   = 1;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa                                     = 4;	// 0 dB
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index               = 0;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap                                   = 0;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb                                   = get_subbandsize(cc->mib->message.dl_Bandwidth);	// ignored
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode                      = (cc->p_eNB == 1) ? 1 : 2;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband                 = 1;
		    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector                          = 1;
		    //    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ; 
		    dl_req->number_pdu++;

                    // Rel10 fields
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.tl.tag                                = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG;
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start                           = 3;
                    // Rel13 fields
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.tl.tag                                = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG;
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type                               = 0;   // regular UE
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type                    = 2;        // not BR
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io            = 0xFFFF;   // absolute SF

                    dl_config_request->header.message_id                                           = NFAPI_DL_CONFIG_REQUEST;
                    dl_config_request->sfn_sf                                                      = sfn_sf;

		    // Program TX Request
		    TX_req = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus];
		    TX_req->pdu_length = bcch_sdu_length;
		    TX_req->pdu_index = eNB->pdu_index[CC_id]++;
		    TX_req->num_segments = 1;
		    TX_req->segments[0].segment_length = bcch_sdu_length;
		    TX_req->segments[0].segment_data = cc->BCCH_pdu.payload;
		    eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
                    eNB->TX_req[CC_id].sfn_sf = sfn_sf;
                    eNB->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
                    eNB->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;

		} else {
		    LOG_E(MAC,
			  "[eNB %d] CCid %d Frame %d, subframe %d : Cannot add DCI 1A for SI\n",
			  module_idP, CC_id, frameP, subframeP);
		}

                T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(0xffff),
                  T_INT(frameP), T_INT(subframeP), T_INT(0), T_BUFFER(cc->BCCH_pdu.payload, bcch_sdu_length));

		if (opt_enabled == 1) {
		    trace_pdu(DIRECTION_DOWNLINK,
			      &cc->BCCH_pdu.payload[0],
			      bcch_sdu_length,
			      0xffff,
			     WS_SI_RNTI, 0xffff, eNB->frame, eNB->subframe, 0, 0);
		    LOG_D(OPT,
			  "[eNB %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
			  module_idP, frameP, CC_id, 0xffff,
			  bcch_sdu_length);
		}
		if (cc->tdd_Config != NULL) {	//TDD
		    LOG_D(MAC,
			  "[eNB] Frame %d : Scheduling BCCH->DLSCH (TDD) for CC_id %d SI %d bytes (mcs %d, rb 3)\n",
			  frameP, CC_id, bcch_sdu_length, mcs);
		} else {
		    LOG_D(MAC, "[eNB] Frame %d : Scheduling BCCH->DLSCH (FDD) for CC_id %d SI %d bytes (mcs %d, rb 3)\n", frameP, CC_id, bcch_sdu_length, mcs);
		}


		eNB->eNB_stats[CC_id].total_num_bcch_pdu += 1;
		eNB->eNB_stats[CC_id].bcch_buffer = bcch_sdu_length;
		eNB->eNB_stats[CC_id].total_bcch_buffer += bcch_sdu_length;
		eNB->eNB_stats[CC_id].bcch_mcs = mcs;
//printf("SI %d.%d\n", frameP, subframeP);/////////////////////////////////////////******************************
	    } else {

		//LOG_D(MAC,"[eNB %d] Frame %d : BCCH not active \n",Mod_id,frame);
	    }
	}
    }
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
    schedule_SIB1_BR(module_idP, frameP, subframeP);
    schedule_SI_BR(module_idP, frameP, subframeP);
#endif

  stop_meas(&eNB->schedule_si);
}
