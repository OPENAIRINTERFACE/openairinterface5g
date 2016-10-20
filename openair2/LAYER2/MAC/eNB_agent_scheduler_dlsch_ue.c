/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file eNB_agent_scheduler_dlsch_ue.c
 * \brief procedures related to eNB for the DLSCH transport channel
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/enb_agent_mac_proto.h"
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

#include "ENB_APP/enb_agent_defs.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "enb_agent_mac.h"

#include "SIMULATION/TOOLS/defs.h" // for taus

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG

//------------------------------------------------------------------------------
void
schedule_ue_spec_default(
  mid_t   mod_id,
  uint32_t      frame,
  uint32_t      subframe,
  int           *mbsfn_flag,
  Protocol__FlexranMessage **dl_info
)
//------------------------------------------------------------------------------
{
  uint8_t               CC_id;
  int                   UE_id;
  int                   N_RBG[MAX_NUM_CCs];
  unsigned char         aggregation;
  mac_rlc_status_resp_t rlc_status;
  unsigned char         header_len_dcch=0, header_len_dcch_tmp=0, header_len_dtch=0, header_len_dtch_tmp=0, ta_len=0;
  unsigned char header_len = 0, header_len_tmp = 0;
  uint16_t              nb_rb, nb_rb_temp, total_nb_available_rb[MAX_NUM_CCs], nb_available_rb;
  uint16_t              TBS, j, rnti, padding=0, post_padding=0;
  unsigned char         round            = 0;
  unsigned char         harq_pid         = 0;
  void                 *DLSCH_dci        = NULL;
  LTE_eNB_UE_stats     *eNB_UE_stats     = NULL;
  uint16_t              sdu_length_total = 0;
  int                   mcs, mcs_tmp;
  uint16_t              min_rb_unit[MAX_NUM_CCs];
  eNB_MAC_INST         *eNB      = &eNB_mac_inst[mod_id];
  UE_list_t            *UE_list  = &eNB->UE_list;
  LTE_DL_FRAME_PARMS   *frame_parms[MAX_NUM_CCs];
  int32_t                 normalized_rx_power, target_rx_power;
  int32_t                 tpc=1;
  static int32_t          tpc_accumulated=0;
  UE_sched_ctrl           *ue_sched_ctl;

  Protocol__FlexDlData *dl_data[NUM_MAX_UE];
  int num_ues_added = 0;
  int channels_added = 0;

  Protocol__FlexDlDci *dl_dci;
  Protocol__FlexRlcPdu *rlc_pdus[11];
  uint32_t *ce_bitmap;
  Protocol__FlexRlcPdu **rlc_pdu;
  int num_tb;
  uint32_t ce_flags = 0;

  uint8_t            rballoc_sub[25];
  int i;
  uint32_t data_to_request;
  uint32_t dci_tbs;
  uint8_t ue_has_transmission = 0;
  uint32_t ndi;
  
  enb_agent_mac_create_empty_dl_config(mod_id, dl_info);
  
  if (UE_list->head==-1) {
    return;
  }
  
  start_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,VCD_FUNCTION_IN);

  //weight = get_ue_weight(module_idP,UE_id);
  aggregation = 2; // set to the maximum aggregation level

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    min_rb_unit[CC_id] = get_min_rb_unit(mod_id, CC_id);
    frame_parms[CC_id] = mac_xface->get_lte_frame_parms(mod_id, CC_id);
    // get number of PRBs less those used by common channels
    total_nb_available_rb[CC_id] = frame_parms[CC_id]->N_RB_DL;
    for (i=0;i<frame_parms[CC_id]->N_RB_DL;i++)
      if (eNB->common_channels[CC_id].vrb_map[i]!=0)
	total_nb_available_rb[CC_id]--;
    
    N_RBG[CC_id] = frame_parms[CC_id]->N_RBG;

    // store the global enb stats:
    eNB->eNB_stats[CC_id].num_dlactive_UEs =  UE_list->num_UEs;
    eNB->eNB_stats[CC_id].available_prbs =  total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].total_available_prbs +=  total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].dlsch_bytes_tx=0;
    eNB->eNB_stats[CC_id].dlsch_pdus_tx=0;
  }

   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_IN);

   start_meas(&eNB->schedule_dlsch_preprocessor);
   dlsch_scheduler_pre_processor(mod_id,
				 frame,
				 subframe,
				 N_RBG,
				 mbsfn_flag);
   stop_meas(&eNB->schedule_dlsch_preprocessor);
   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_OUT);

   for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d\n",CC_id);

    if (mbsfn_flag[CC_id]>0)
      continue;

    for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
      rnti = UE_RNTI(mod_id, UE_id);
      eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id, CC_id, rnti);
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

      if (rnti==NOT_A_RNTI) {
        LOG_D(MAC,"Cannot find rnti for UE_id %d (num_UEs %d)\n",UE_id,UE_list->num_UEs);
        // mac_xface->macphy_exit("Cannot find rnti for UE_id");
        continue;
      }

      if (eNB_UE_stats==NULL) {
        LOG_D(MAC,"[eNB] Cannot find eNB_UE_stats\n");
        //  mac_xface->macphy_exit("[MAC][eNB] Cannot find eNB_UE_stats\n");
        continue;
      }

      if ((ue_sched_ctl->pre_nb_available_rbs[CC_id] == 0) ||  // no RBs allocated 
	  CCE_allocation_infeasible(mod_id, CC_id, 0, subframe, aggregation, rnti)
	  ) {
        LOG_D(MAC,"[eNB %d] Frame %d : no RB allocated for UE %d on CC_id %d: continue \n",
              mod_id, frame, UE_id, CC_id);
        //if(mac_xface->get_transmission_mode(module_idP,rnti)==5)
        continue; //to next user (there might be rbs availiable for other UEs in TM5
        // else
        //  break;
      }

      if (frame_parms[CC_id]->frame_type == TDD)  {
        set_ue_dai (subframe,
                    frame_parms[CC_id]->tdd_config,
                    UE_id,
                    CC_id,
                    UE_list);
        // update UL DAI after DLSCH scheduling
        set_ul_DAI(mod_id, UE_id, CC_id, frame, subframe,frame_parms);
      }

      channels_added = 0;

      // After this point all the UEs will be scheduled
      dl_data[num_ues_added] = (Protocol__FlexDlData *) malloc(sizeof(Protocol__FlexDlData));
      protocol__flex_dl_data__init(dl_data[num_ues_added]);
      dl_data[num_ues_added]->has_rnti = 1;
      dl_data[num_ues_added]->rnti = rnti;
      dl_data[num_ues_added]->n_rlc_pdu = 0;
      dl_data[num_ues_added]->has_serv_cell_index = 1;
      dl_data[num_ues_added]->serv_cell_index = CC_id;
      
      nb_available_rb = ue_sched_ctl->pre_nb_available_rbs[CC_id];
      harq_pid = ue_sched_ctl->harq_pid[CC_id];
      round = ue_sched_ctl->round[CC_id];
      
      sdu_length_total=0;

      /*
      DevCheck(((eNB_UE_stats->DL_cqi[0] < MIN_CQI_VALUE) || (eNB_UE_stats->DL_cqi[0] > MAX_CQI_VALUE)),
      eNB_UE_stats->DL_cqi[0], MIN_CQI_VALUE, MAX_CQI_VALUE);
      */

      mcs = cqi_to_mcs[eNB_UE_stats->DL_cqi[0]];
      //      mcs = cmin(mcs, openair_daq_vars.target_ue_dl_mcs);

#ifdef EXMIMO

       if (mac_xface->get_transmission_mode(mod_id, CC_id, rnti) == 5) {
	  mcs = cqi_to_mcs[eNB_UE_stats->DL_cqi[0]];
	  mcs =  cmin(mcs,16);
       }

#endif

      // initializing the rb allocation indicator for each UE
      for(j = 0; j < frame_parms[CC_id]->N_RBG; j++) {
        UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = 0;
	rballoc_sub[j] = 0;
      }

      LOG_D(MAC,"[eNB %d] Frame %d: Scheduling UE %d on CC_id %d (rnti %x, harq_pid %d, round %d, rb %d, cqi %d, mcs %d, rrc %d)\n",
            mod_id, frame, UE_id, CC_id, rnti, harq_pid, round, nb_available_rb,
            eNB_UE_stats->DL_cqi[0], mcs,
            UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status);

      dl_dci = (Protocol__FlexDlDci*) malloc(sizeof(Protocol__FlexDlDci));
      protocol__flex_dl_dci__init(dl_dci);
      dl_data[num_ues_added]->dl_dci = dl_dci;

      
      dl_dci->has_rnti = 1;
      dl_dci->rnti = rnti;
      dl_dci->has_harq_process = 1;
      dl_dci->harq_process = harq_pid;
      
      /* process retransmission  */

      if (round > 0) {

	if (frame_parms[CC_id]->frame_type == TDD) {
	  UE_list->UE_template[CC_id][UE_id].DAI++;
	  update_ul_dci(mod_id, CC_id, rnti, UE_list->UE_template[CC_id][UE_id].DAI);
	  LOG_D(MAC,"DAI update: CC_id %d subframeP %d: UE %d, DAI %d\n",
		CC_id, subframe,UE_id,UE_list->UE_template[CC_id][UE_id].DAI);
	}

	mcs = UE_list->UE_template[CC_id][UE_id].mcs[harq_pid];

	  // get freq_allocation
	nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
	  
	dci_tbs = mac_xface->get_TBS_DL(mcs, nb_rb);

	if (nb_rb <= nb_available_rb) {
	  
	  if(nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
	    for(j = 0; j < frame_parms[CC_id]->N_RBG; j++) { // for indicating the rballoc for each sub-band
	      UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
	  } else {
	    nb_rb_temp = nb_rb;
	    j = 0;

	    while((nb_rb_temp > 0) && (j < frame_parms[CC_id]->N_RBG)) {
	      if(ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 1) {
		UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
		
		if((j == frame_parms[CC_id]->N_RBG-1) &&
		   ((frame_parms[CC_id]->N_RB_DL == 25)||
		    (frame_parms[CC_id]->N_RB_DL == 50))) {
		  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id]+1;
		} else {
		  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id];
		}
	      }
	      j = j + 1;
	    }
	  }

	  nb_available_rb -= nb_rb;
	  aggregation = process_ue_cqi(mod_id, UE_id);
	  
	  PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
	  PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];
	  
	  for(j=0; j<frame_parms[CC_id]->N_RBG; j++) {
	    PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].rballoc_sub[j] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];
	    rballoc_sub[j] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];
	  }

	  // Keep the old NDI, do not toggle
	  ndi = UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
	  tpc = UE_list->UE_template[CC_id][UE_id].oldTPC[harq_pid];
	  UE_list->UE_template[CC_id][UE_id].mcs[harq_pid] = mcs;

	  ue_has_transmission = 1;
	  num_ues_added++;
	} else {
	  LOG_D(MAC,"[eNB %d] Frame %d CC_id %d : don't schedule UE %d, its retransmission takes more resources than we have\n",
                mod_id, frame, CC_id, UE_id);
	  ue_has_transmission = 0;
	}
	//End of retransmission
      } else { /* This is a potentially new SDU opportunity */
	rlc_status.bytes_in_buffer = 0;
        // Now check RLC information to compute number of required RBs
        // get maximum TBS size for RLC request
        //TBS = mac_xface->get_TBS(eNB_UE_stats->DL_cqi[0]<<1,nb_available_rb);
        TBS = mac_xface->get_TBS_DL(mcs, nb_available_rb);
	dci_tbs = TBS;

        // check first for RLC data on DCCH
        // add the length for  all the control elements (timing adv, drx, etc) : header + payload

	ta_len = (ue_sched_ctl->ta_update!=0) ? 2 : 0;
	
	dl_data[num_ues_added]->n_ce_bitmap = 2;
	dl_data[num_ues_added]->ce_bitmap = (uint32_t *) malloc(sizeof(uint32_t) * 2);
	
	if (ta_len > 0) {
	  ce_flags |= PROTOCOL__FLEX_CE_TYPE__FLPCET_TA;
	}

	/*TODO: Add other flags if DRX and other CE are required*/
	
	// Add the control element flags to the flexran message
	dl_data[num_ues_added]->ce_bitmap[0] = ce_flags;
	dl_data[num_ues_added]->ce_bitmap[1] = ce_flags;

	header_len_dcch = 2; // 2 bytes DCCH SDU subheader

	// TODO : Need to prioritize DRBs
	// Loop through the UE logical channels (DCCH, DCCH1, DTCH for now)
	for (j = 1; j < NB_RB_MAX; j++) {
	  header_len+=3;

	  // Need to see if we have space for data from this channel
	  if (dci_tbs - ta_len - header_len > 0) {
	     LOG_D(MAC, "[TEST]Requested %d bytes from RLC buffer if channel %d during first call\n", dci_tbs-ta_len-header_len);
	     //If we have space, we need to see how much data we can request at most (if any available)
	     rlc_status = mac_rlc_status_ind(mod_id,
					     rnti,
					     mod_id,
					     frame,
					     ENB_FLAG_YES,
					     MBMS_FLAG_NO,
					     j,
					     (dci_tbs-ta_len-header_len)); // transport block set size

	     //If data are available in channel j
	     if (rlc_status.bytes_in_buffer > 0) {
	       LOG_D(MAC, "[TEST]Have %d bytes in DCCH buffer during first call\n", rlc_status.bytes_in_buffer);
	       //Fill in as much as possible
	       data_to_request = cmin(dci_tbs-ta_len-header_len, rlc_status.bytes_in_buffer);
	       if (data_to_request < 128) { //The header will be one byte less
		 header_len--;
	       }
	       if (j == 1 || j == 2) {
	        data_to_request++; //It is not correct but fixes some RLC bug for DCCH
	       }
	       LOG_D(MAC, "[TEST]Will request %d from DCCH\n", data_to_request);
	       rlc_pdus[channels_added] = (Protocol__FlexRlcPdu *) malloc(sizeof(Protocol__FlexRlcPdu));
	       protocol__flex_rlc_pdu__init(rlc_pdus[channels_added]);
	       rlc_pdus[channels_added]->n_rlc_pdu_tb = 2;
	       rlc_pdus[channels_added]->rlc_pdu_tb = (Protocol__FlexRlcPduTb **) malloc(sizeof(Protocol__FlexRlcPduTb *) * 2);
	       rlc_pdus[channels_added]->rlc_pdu_tb[0] = (Protocol__FlexRlcPduTb *) malloc(sizeof(Protocol__FlexRlcPduTb));
	       protocol__flex_rlc_pdu_tb__init(rlc_pdus[channels_added]->rlc_pdu_tb[0]);
	       rlc_pdus[channels_added]->rlc_pdu_tb[0]->has_logical_channel_id = 1;
	       rlc_pdus[channels_added]->rlc_pdu_tb[0]->logical_channel_id = j;
	       rlc_pdus[channels_added]->rlc_pdu_tb[0]->has_size = 1;
	       rlc_pdus[channels_added]->rlc_pdu_tb[0]->size = data_to_request;
	       rlc_pdus[channels_added]->rlc_pdu_tb[1] = (Protocol__FlexRlcPduTb *) malloc(sizeof(Protocol__FlexRlcPduTb));
	       protocol__flex_rlc_pdu_tb__init(rlc_pdus[channels_added]->rlc_pdu_tb[1]);
	       rlc_pdus[channels_added]->rlc_pdu_tb[1]->has_logical_channel_id = 1;
	       rlc_pdus[channels_added]->rlc_pdu_tb[1]->logical_channel_id = j;
	       rlc_pdus[channels_added]->rlc_pdu_tb[1]->has_size = 1;
	       rlc_pdus[channels_added]->rlc_pdu_tb[1]->size = data_to_request;
	       dl_data[num_ues_added]->n_rlc_pdu++;
	       channels_added++;
	       //Set this to the max value that we might request
	       sdu_length_total = data_to_request;
	     } else {
	       //Take back the assumption of a header for this channel
	       header_len -= 3;
	     } //End rlc_status.bytes_in_buffer <= 0
	  } //end of if dci_tbs - ta_len - header_len > 0
	} // End of iterating the logical channels
	
	// Add rlc_pdus to the dl_data message
	dl_data[num_ues_added]->rlc_pdu = (Protocol__FlexRlcPdu **) malloc(sizeof(Protocol__FlexRlcPdu *) *
									  dl_data[num_ues_added]->n_rlc_pdu);
	for (i = 0; i < dl_data[num_ues_added]->n_rlc_pdu; i++) {
	  dl_data[num_ues_added]->rlc_pdu[i] = rlc_pdus[i];
	}
	
	// there is a payload
        if (( dl_data[num_ues_added]->n_rlc_pdu > 0)) {
	  // Now compute number of required RBs for total sdu length
          // Assume RAH format 2
          // adjust  header lengths
	  header_len_tmp = header_len;

	  if (header_len == 2 || header_len == 3) { //Only one SDU, remove length field
	    header_len = 1;
	  } else { //Remove length field from the last SDU
	    header_len--;
	  }

	  mcs_tmp = mcs;
	  if (mcs_tmp == 0) {
            nb_rb = 4;  // don't let the TBS get too small
          } else {
            nb_rb=min_rb_unit[CC_id];
          }

	  LOG_D(MAC,"[TEST]The initial number of resource blocks was %d\n", nb_rb);
	  LOG_D(MAC,"[TEST] The initial mcs was %d\n", mcs_tmp);

	  TBS = mac_xface->get_TBS_DL(mcs_tmp, nb_rb);
	  LOG_D(MAC,"[TEST]The TBS during rate matching was %d\n", TBS);

	  while (TBS < (sdu_length_total + header_len + ta_len))  {
            nb_rb += min_rb_unit[CC_id];  //
	    LOG_D(MAC, "[TEST]Had to increase the number of RBs\n");
            if (nb_rb > nb_available_rb) { // if we've gone beyond the maximum number of RBs
              // (can happen if N_RB_DL is odd)
              TBS = mac_xface->get_TBS_DL(mcs_tmp, nb_available_rb);
              nb_rb = nb_available_rb;
              break;
            }

            TBS = mac_xface->get_TBS_DL(mcs_tmp, nb_rb);
          }

	  if(nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
	    LOG_D(MAC, "[TEST]We had the exact number of rbs. Time to fill the rballoc subband\n");
            for(j = 0; j < frame_parms[CC_id]->N_RBG; j++) { // for indicating the rballoc for each sub-band
              UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
          } else {
	    nb_rb_temp = nb_rb;
            j = 0;
	    LOG_D(MAC, "[TEST]Will only partially fill the bitmap\n");
	    while((nb_rb_temp > 0) && (j < frame_parms[CC_id]->N_RBG)) {
              if(ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 1) {
                UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
                if ((j == frame_parms[CC_id]->N_RBG-1) &&
                    ((frame_parms[CC_id]->N_RB_DL == 25)||
                     (frame_parms[CC_id]->N_RB_DL == 50))) {
                  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id]+1;
                } else {
                  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id];
                }
              }
              j = j+1;
            }
	  }
	  
	  PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
          PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];

	  for(j = 0; j < frame_parms[CC_id]->N_RBG; j++) {
            PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].rballoc_sub[j] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];
          }

	  // decrease mcs until TBS falls below required length
          while ((TBS > (sdu_length_total + header_len + ta_len)) && (mcs_tmp > 0)) {
            mcs_tmp--;
            TBS = mac_xface->get_TBS_DL(mcs_tmp, nb_rb);
          }

	  // if we have decreased too much or we don't have enough RBs, increase MCS
          while ((TBS < (sdu_length_total + header_len + ta_len)) &&
		 ((( ue_sched_ctl->dl_pow_off[CC_id] > 0) && (mcs_tmp < 28))											     || ( (ue_sched_ctl->dl_pow_off[CC_id]==0) && (mcs_tmp <= 15)))) {
            mcs_tmp++;
            TBS = mac_xface->get_TBS_DL(mcs_tmp, nb_rb);
          }

	  dci_tbs = TBS;
	  mcs = mcs_tmp;

	  aggregation = process_ue_cqi(mod_id,UE_id);
	  dl_dci->has_aggr_level = 1;
	  dl_dci->aggr_level = aggregation;
	  
          UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid] = nb_rb;

	  if (frame_parms[CC_id]->frame_type == TDD) {
            UE_list->UE_template[CC_id][UE_id].DAI++;
            //  printf("DAI update: subframeP %d: UE %d, DAI %d\n",subframeP,UE_id,UE_list->UE_template[CC_id][UE_id].DAI);
#warning only for 5MHz channel
            update_ul_dci(mod_id, CC_id, rnti, UE_list->UE_template[CC_id][UE_id].DAI);
          }

	  // do PUCCH power control
          // this is the normalized RX power
	  eNB_UE_stats =  mac_xface->get_eNB_UE_stats(mod_id, CC_id, rnti);
	  normalized_rx_power = eNB_UE_stats->Po_PUCCH_dBm; 
	  target_rx_power = mac_xface->get_target_pucch_rx_power(mod_id, CC_id) + 10;

	  // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame*10+UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe;

	  if (((framex10psubframe+10)<=(frame*10+subframe)) || //normal case
	      ((framex10psubframe>(frame*10+subframe)) && (((10240-framex10psubframe+frame*10+subframe)>=10)))) //frame wrap-around
	    if (eNB_UE_stats->Po_PUCCH_update == 1) { 
	      eNB_UE_stats->Po_PUCCH_update = 0;

	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame=frame;
	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe=subframe;
	      if (normalized_rx_power>(target_rx_power+1)) {
		tpc = 0; //-1
		tpc_accumulated--;
	      } else if (normalized_rx_power<(target_rx_power-1)) {
		tpc = 2; //+1
		tpc_accumulated++;
	      } else {
		tpc = 1; //0
	      }
	      LOG_D(MAC,"[eNB %d] DLSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
		    mod_id, frame, subframe, harq_pid, tpc,
		    tpc_accumulated, normalized_rx_power, target_rx_power);
	    } // Po_PUCCH has been updated 
	    else {
	      tpc = 1; //0
	    } // time to do TPC update 
	  else {
	    tpc = 1; //0
	  }

	  for(i=0; i<PHY_vars_eNB_g[mod_id][CC_id]->frame_parms.N_RBG; i++) {
	    rballoc_sub[i] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][i];
          }	

	   // Toggle NDI
          LOG_D(MAC,"CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
                CC_id, frame, subframe, UE_id,
                UE_list->UE_template[CC_id][UE_id].rnti,harq_pid, UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]);
          UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]= 1 - UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
	  ndi =  UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
	  
	  UE_list->UE_template[CC_id][UE_id].mcs[harq_pid] = mcs;
	  UE_list->UE_template[CC_id][UE_id].oldTPC[harq_pid] = tpc;

	  // Increase the pointer for the number of scheduled UEs
	  num_ues_added++;
	  ue_has_transmission = 1;
	}  else { // There is no data from RLC or MAC header, so don't schedule
	  ue_has_transmission = 0;
	}
      } // End of new scheduling
      
      // If we has transmission or retransmission
      if (ue_has_transmission) {
	switch (mac_xface->get_transmission_mode(mod_id, CC_id, rnti)) {
	case 1:
	case 2:
	default:
	  dl_dci->has_res_alloc = 1;
	  dl_dci->res_alloc = 0;
	  dl_dci->has_vrb_format = 1;
	  dl_dci->vrb_format = PROTOCOL__FLEX_VRB_FORMAT__FLVRBF_LOCALIZED;
	  dl_dci->has_format = 1;
	  dl_dci->format = PROTOCOL__FLEX_DCI_FORMAT__FLDCIF_1;
	  dl_dci->has_rb_bitmap = 1;
	  dl_dci->rb_bitmap = allocate_prbs_sub(nb_rb, rballoc_sub);
	  dl_dci->has_rb_shift = 1;
	  dl_dci->rb_shift = 0;
	  dl_dci->n_ndi = 1;
	  dl_dci->ndi = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_ndi);
	  dl_dci->ndi[0] = ndi;
	  dl_dci->n_rv = 1;
	  dl_dci->rv = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_rv);
	  dl_dci->rv[0] = round & 3;
	  dl_dci->has_tpc = 1;
	  dl_dci->tpc = tpc;
	  dl_dci->n_mcs = 1;
	  dl_dci->mcs = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_mcs);
	  dl_dci->mcs[0] = mcs;
	  dl_dci->n_tbs_size = 1;
	  dl_dci->tbs_size = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_tbs_size);
	  dl_dci->tbs_size[0] = dci_tbs;
	  if (frame_parms[CC_id]->frame_type == TDD) {
	    dl_dci->has_dai = 1;
	    dl_dci->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	  }
	  break;
	case 3:
	  dl_dci->has_res_alloc = 1;
	  dl_dci->res_alloc = 0;
	  dl_dci->has_vrb_format = 1;
	  dl_dci->vrb_format = PROTOCOL__FLEX_VRB_FORMAT__FLVRBF_LOCALIZED;
	  dl_dci->has_format = 1;
	  dl_dci->format = PROTOCOL__FLEX_DCI_FORMAT__FLDCIF_2A;
	  dl_dci->has_rb_bitmap = 1;
	  dl_dci->rb_bitmap = allocate_prbs_sub(nb_rb, rballoc_sub);
	  dl_dci->has_rb_shift = 1;
	  dl_dci->rb_shift = 0;
	  dl_dci->n_ndi = 2;
	  dl_dci->ndi = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_ndi);
	  dl_dci->ndi[0] = ndi;
	  dl_dci->ndi[1] = ndi;
	  dl_dci->n_rv = 2;
	  dl_dci->rv = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_rv);
	  dl_dci->rv[0] = round & 3;
	  dl_dci->rv[1] = round & 3;
	  dl_dci->has_tpc = 1;
	  dl_dci->tpc = tpc;
	  dl_dci->n_mcs = 2;
	  dl_dci->mcs = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_mcs);
	  dl_dci->mcs[0] = mcs;
	  dl_dci->mcs[1] = mcs;
	  dl_dci->n_tbs_size = 2;
	  dl_dci->tbs_size = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_tbs_size);
	  dl_dci->tbs_size[0] = dci_tbs;
	  dl_dci->tbs_size[1] = dci_tbs;
	  if (frame_parms[CC_id]->frame_type == TDD) {
	    dl_dci->has_dai = 1;
	    dl_dci->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	  }
	  break;
	case 4:
	  dl_dci->has_res_alloc = 1;
	  dl_dci->res_alloc = 0;
	  dl_dci->has_vrb_format = 1;
	  dl_dci->vrb_format = PROTOCOL__FLEX_VRB_FORMAT__FLVRBF_LOCALIZED;
	  dl_dci->has_format = 1;
	  dl_dci->format = PROTOCOL__FLEX_DCI_FORMAT__FLDCIF_2A;
	  dl_dci->has_rb_bitmap = 1;
	  dl_dci->rb_bitmap = allocate_prbs_sub(nb_rb, rballoc_sub);
	  dl_dci->has_rb_shift = 1;
	  dl_dci->rb_shift = 0;
	  dl_dci->n_ndi = 2;
	  dl_dci->ndi = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_ndi);
	  dl_dci->ndi[0] = ndi;
	  dl_dci->ndi[1] = ndi;
	  dl_dci->n_rv = 2;
	  dl_dci->rv = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_rv);
	  dl_dci->rv[0] = round & 3;
	  dl_dci->rv[1] = round & 3;
	  dl_dci->has_tpc = 1;
	  dl_dci->tpc = tpc;
	  dl_dci->n_mcs = 2;
	  dl_dci->mcs = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_mcs);
	  dl_dci->mcs[0] = mcs;
	  dl_dci->mcs[1] = mcs;
	  dl_dci->n_tbs_size = 2;
	  dl_dci->tbs_size = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_tbs_size);
	  dl_dci->tbs_size[0] = dci_tbs;
	  dl_dci->tbs_size[1] = dci_tbs;
	  if (frame_parms[CC_id]->frame_type == TDD) {
	    dl_dci->has_dai = 1;
	    dl_dci->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	  }
	  break;
	case 5:
	  dl_dci->has_res_alloc = 1;
	  dl_dci->res_alloc = 0;
	  dl_dci->has_vrb_format = 1;
	  dl_dci->vrb_format = PROTOCOL__FLEX_VRB_FORMAT__FLVRBF_LOCALIZED;
	  dl_dci->has_format = 1;
	  dl_dci->format = PROTOCOL__FLEX_DCI_FORMAT__FLDCIF_1D;
	  dl_dci->has_rb_bitmap = 1;
	  dl_dci->rb_bitmap = allocate_prbs_sub(nb_rb, rballoc_sub);
	  dl_dci->has_rb_shift = 1;
	  dl_dci->rb_shift = 0;
	  dl_dci->n_ndi = 1;
	  dl_dci->ndi = 1;
	  dl_dci->ndi[0] = ndi;
	  dl_dci->n_rv = 1;
	  dl_dci->rv = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_rv);
	  dl_dci->rv[0] = round & 3;
	  dl_dci->has_tpc = 1;
	  dl_dci->tpc = tpc;
	  dl_dci->n_mcs = 1;
	  dl_dci->mcs = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_mcs);
	  dl_dci->mcs[0] = mcs;
	  dl_dci->n_tbs_size = 1;
	  dl_dci->tbs_size = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_tbs_size);
	  dl_dci->tbs_size[0] = dci_tbs;
	  if (frame_parms[CC_id]->frame_type == TDD) {
	    dl_dci->has_dai = 1;
	    dl_dci->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	  }
	  
	  if(ue_sched_ctl->dl_pow_off[CC_id] == 2) {
	    ue_sched_ctl->dl_pow_off[CC_id] = 1;
	  }
	  
	  dl_dci->has_dl_power_offset = 1;
	  dl_dci->dl_power_offset = ue_sched_ctl->dl_pow_off[CC_id];
	  dl_dci->has_precoding_info = 1;
	  dl_dci->precoding_info = 5; // Is this right??
	  
	  break;
	case 6:
	  dl_dci->has_res_alloc = 1;
	  dl_dci->res_alloc = 0;
	  dl_dci->has_vrb_format = 1;
	  dl_dci->vrb_format = PROTOCOL__FLEX_VRB_FORMAT__FLVRBF_LOCALIZED;
	  dl_dci->has_format = 1;
	  dl_dci->format = PROTOCOL__FLEX_DCI_FORMAT__FLDCIF_1D;
	  dl_dci->has_rb_bitmap = 1;
	  dl_dci->rb_bitmap = allocate_prbs_sub(nb_rb, rballoc_sub);
	  dl_dci->has_rb_shift = 1;
	  dl_dci->rb_shift = 0;
	  dl_dci->n_ndi = 1;
	  dl_dci->ndi = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_ndi);
	  dl_dci->ndi[0] = ndi;
	  dl_dci->n_rv = 1;
	  dl_dci->rv = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_rv);
	  dl_dci->rv[0] = round & 3;
	  dl_dci->has_tpc = 1;
	  dl_dci->tpc = tpc;
	  dl_dci->n_mcs = 1;
	  dl_dci->mcs = (uint32_t *) malloc(sizeof(uint32_t) * dl_dci->n_mcs);
	  dl_dci->mcs[0] = mcs;
	  if (frame_parms[CC_id]->frame_type == TDD) {
	    dl_dci->has_dai = 1;
	    dl_dci->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	  }

	  dl_dci->has_dl_power_offset = 1;
	  dl_dci->dl_power_offset = ue_sched_ctl->dl_pow_off[CC_id];
	  dl_dci->has_precoding_info = 1;
	  dl_dci->precoding_info = 5; // Is this right??
	  break;
	}
      }
      
      if (frame_parms[CC_id]->frame_type == TDD) {
        set_ul_DAI(mod_id, UE_id, CC_id, frame, subframe, frame_parms);
      }
    } // UE_id loop
   } // CC_id loop

   // Add all the dl_data elements to the flexran message
   (*dl_info)->dl_mac_config_msg->n_dl_ue_data = num_ues_added;
   (*dl_info)->dl_mac_config_msg->dl_ue_data = (Protocol__FlexDlData **) malloc(sizeof(Protocol__FlexDlData *) * num_ues_added);
   for (i = 0; i < num_ues_added; i++) {
     (*dl_info)->dl_mac_config_msg->dl_ue_data[i] = dl_data[i];
   }
   
   stop_meas(&eNB->schedule_dlsch);
   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,VCD_FUNCTION_OUT);
}
