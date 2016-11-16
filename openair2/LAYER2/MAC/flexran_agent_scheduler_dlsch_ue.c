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

/*! \file flexran_agent_scheduler_dlsch_ue.c
 * \brief procedures related to eNB for the DLSCH transport channel
 * \author Xenofon Foukas, Navid Nikaein and Raymond Knopp
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

#include "LAYER2/MAC/flexran_agent_mac_proto.h"
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

#include "ENB_APP/flexran_agent_defs.h"

#include "pdcp.h"

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "flexran_agent_mac.h"

#include "SIMULATION/TOOLS/defs.h" // for taus

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG


//------------------------------------------------------------------------------
void
flexran_schedule_ue_spec_default(mid_t   mod_id,
				 uint32_t      frame,
				 uint32_t      subframe,
				 int           *mbsfn_flag,
				 Protocol__FlexranMessage **dl_info)
//------------------------------------------------------------------------------
{
  uint8_t               CC_id;
  int                   UE_id;
  int                   N_RBG[MAX_NUM_CCs];
  unsigned char         aggregation;
  mac_rlc_status_resp_t rlc_status;
  unsigned char         header_len = 0, header_len_tmp = 0, ta_len = 0;
  uint16_t              nb_rb, nb_rb_temp, total_nb_available_rb[MAX_NUM_CCs], nb_available_rb;
  uint16_t              TBS, j, rnti, padding=0, post_padding=0;
  unsigned char         round            = 0;
  unsigned char         harq_pid         = 0;
  void                 *DLSCH_dci        = NULL;
  uint16_t              sdu_length_total = 0;
  int                   mcs, mcs_tmp;
  uint16_t              min_rb_unit[MAX_NUM_CCs];
  eNB_MAC_INST         *eNB      = &eNB_mac_inst[mod_id];
  /* TODO: Must move the helper structs to scheduler implementation */
  UE_list_t            *UE_list  = &eNB->UE_list;
  int32_t                 normalized_rx_power, target_rx_power;
  int32_t                 tpc = 1;
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
  
  flexran_agent_mac_create_empty_dl_config(mod_id, dl_info);
  
  if (UE_list->head==-1) {
    return;
  }
  
  start_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,VCD_FUNCTION_IN);

  //weight = get_ue_weight(module_idP,UE_id);
  aggregation = 2; // set to the maximum aggregation level

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    min_rb_unit[CC_id] = get_min_rb_unit(mod_id, CC_id);
    // get number of PRBs less those used by common channels
    total_nb_available_rb[CC_id] = flexran_get_N_RB_DL(mod_id, CC_id);
    for (i=0;i < flexran_get_N_RB_DL(mod_id, CC_id); i++)
      if (eNB->common_channels[CC_id].vrb_map[i] != 0)
	total_nb_available_rb[CC_id]--;
    
    N_RBG[CC_id] = flexran_get_N_RBG(mod_id, CC_id);

    // store the global enb stats:
    eNB->eNB_stats[CC_id].num_dlactive_UEs =  UE_list->num_UEs;
    eNB->eNB_stats[CC_id].available_prbs =  total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].total_available_prbs +=  total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].dlsch_bytes_tx=0;
    eNB->eNB_stats[CC_id].dlsch_pdus_tx=0;
  }

   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_IN);

   start_meas(&eNB->schedule_dlsch_preprocessor);
   _dlsch_scheduler_pre_processor(mod_id,
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
      rnti = flexran_get_ue_crnti(mod_id, UE_id);
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

      if (rnti==NOT_A_RNTI) {
        LOG_D(MAC,"Cannot find rnti for UE_id %d (num_UEs %d)\n", UE_id,UE_list->num_UEs);
        // mac_xface->macphy_exit("Cannot find rnti for UE_id");
        continue;
      }

      if (flexran_get_ue_crnti(mod_id, UE_id) == NOT_A_RNTI) {
        LOG_D(MAC,"[eNB] Cannot find UE\n");
        //  mac_xface->macphy_exit("[MAC][eNB] Cannot find eNB_UE_stats\n");
        continue;
      }

      if ((ue_sched_ctl->pre_nb_available_rbs[CC_id] == 0) ||  // no RBs allocated 
	  CCE_allocation_infeasible(mod_id, CC_id, 0, subframe, aggregation, rnti)) {
        LOG_D(MAC,"[eNB %d] Frame %d : no RB allocated for UE %d on CC_id %d: continue \n",
              mod_id, frame, UE_id, CC_id);
        //if(mac_xface->get_transmission_mode(module_idP,rnti)==5)
        continue; //to next user (there might be rbs availiable for other UEs in TM5
        // else
        //  break;
      }

      if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD)  {
        set_ue_dai (subframe,
                    flexran_get_subframe_assignment(mod_id, CC_id),
                    UE_id,
                    CC_id,
                    UE_list);
        //TODO: update UL DAI after DLSCH scheduling
        //set_ul_DAI(mod_id, UE_id, CC_id, frame, subframe,frame_parms);
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
      flexran_get_harq(mod_id, CC_id, UE_id, frame, subframe, &harq_pid, &round);
      sdu_length_total=0;
      mcs = cqi_to_mcs[flexran_get_ue_wcqi(mod_id, UE_id)];

#ifdef EXMIMO

       if (mac_xface->get_transmission_mode(mod_id, CC_id, rnti) == 5) {
	  mcs = cqi_to_mcs[flexran_get_ue_wcqi(mod_id, UE_id)];
	  mcs =  cmin(mcs,16);
       }

#endif

      // initializing the rb allocation indicator for each UE
       for(j = 0; j < flexran_get_N_RBG(mod_id, CC_id); j++) {
        UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = 0;
	rballoc_sub[j] = 0;
      }

      /* LOG_D(MAC,"[eNB %d] Frame %d: Scheduling UE %d on CC_id %d (rnti %x, harq_pid %d, round %d, rb %d, cqi %d, mcs %d, rrc %d)\n", */
      /*       mod_id, frame, UE_id, CC_id, rnti, harq_pid, round, nb_available_rb, */
      /*       eNB_UE_stats->DL_cqi[0], mcs, */
      /*       UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status); */

      dl_dci = (Protocol__FlexDlDci*) malloc(sizeof(Protocol__FlexDlDci));
      protocol__flex_dl_dci__init(dl_dci);
      dl_data[num_ues_added]->dl_dci = dl_dci;

      
      dl_dci->has_rnti = 1;
      dl_dci->rnti = rnti;
      dl_dci->has_harq_process = 1;
      dl_dci->harq_process = harq_pid;
      
      /* process retransmission  */

      if (round > 0) {

	if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
	  UE_list->UE_template[CC_id][UE_id].DAI++;
	  update_ul_dci(mod_id, CC_id, rnti, UE_list->UE_template[CC_id][UE_id].DAI);
	  LOG_D(MAC,"DAI update: CC_id %d subframeP %d: UE %d, DAI %d\n",
		CC_id, subframe,UE_id,UE_list->UE_template[CC_id][UE_id].DAI);
	}

	mcs = UE_list->UE_template[CC_id][UE_id].mcs[harq_pid];

	  // get freq_allocation
	nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
	  
	/*TODO: Must add this to FlexRAN agent API */
	dci_tbs = mac_xface->get_TBS_DL(mcs, nb_rb);

	if (nb_rb <= nb_available_rb) {
	  
	  if(nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
	    for(j = 0; j < flexran_get_N_RBG(mod_id, CC_id); j++) { // for indicating the rballoc for each sub-band
	      UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
	  } else {
	    nb_rb_temp = nb_rb;
	    j = 0;

	    while((nb_rb_temp > 0) && (j < flexran_get_N_RBG(mod_id, CC_id))) {
	      if(ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 1) {
		UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
		
		if((j == flexran_get_N_RBG(mod_id, CC_id) - 1) &&
		   ((flexran_get_N_RB_DL(mod_id, CC_id) == 25)||
		    (flexran_get_N_RB_DL(mod_id, CC_id) == 50))) {
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
	  
	  for(j=0; j < flexran_get_N_RBG(mod_id, CC_id); j++) {
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

	// TODO : Need to prioritize DRBs
	// Loop through the UE logical channels (DCCH, DCCH1, DTCH for now)
	for (j = 1; j < NB_RB_MAX; j++) {
	  header_len+=3;

	  // Need to see if we have space for data from this channel
	  if (dci_tbs - ta_len - header_len - sdu_length_total > 0) {
	     LOG_D(MAC, "[TEST]Requested %d bytes from RLC buffer on channel %d during first call\n", dci_tbs-ta_len-header_len);
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
	       /* if (j == 1 || j == 2) { */
	       /*  data_to_request+=0; 
	       /* } */
	       LOG_D(MAC, "[TEST]Will request %d from channel %d\n", data_to_request, j);
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
	       sdu_length_total += data_to_request;
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
            for(j = 0; j < flexran_get_N_RBG(mod_id, CC_id); j++) { // for indicating the rballoc for each sub-band
              UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
          } else {
	    nb_rb_temp = nb_rb;
            j = 0;
	    LOG_D(MAC, "[TEST]Will only partially fill the bitmap\n");
	    while((nb_rb_temp > 0) && (j < flexran_get_N_RBG(mod_id, CC_id))) {
              if(ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 1) {
                UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
                if ((j == flexran_get_N_RBG(mod_id, CC_id) - 1) &&
                    ((flexran_get_N_RB_DL(mod_id, CC_id) == 25)||
                     (flexran_get_N_RB_DL(mod_id, CC_id) == 50))) {
                  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id] + 1;
                } else {
                  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id];
                }
              }
              j = j+1;
            }
	  }
	  
	  PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
          PHY_vars_eNB_g[mod_id][CC_id]->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];

	  for(j = 0; j < flexran_get_N_RBG(mod_id, CC_id); j++) {
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

	  if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
            UE_list->UE_template[CC_id][UE_id].DAI++;
            //  printf("DAI update: subframeP %d: UE %d, DAI %d\n",subframeP,UE_id,UE_list->UE_template[CC_id][UE_id].DAI);
	    //#warning only for 5MHz channel
            update_ul_dci(mod_id, CC_id, rnti, UE_list->UE_template[CC_id][UE_id].DAI);
          }

	  // do PUCCH power control
          // this is the normalized RX power
	  normalized_rx_power = flexran_get_p0_pucch_dbm(mod_id,UE_id, CC_id); //eNB_UE_stats->Po_PUCCH_dBm; 
	  target_rx_power = flexran_get_p0_nominal_pucch(mod_id, CC_id) + 10; //mac_xface->get_target_pucch_rx_power(mod_id, CC_id) + 10;

	  // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame*10+UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe;

	  if (((framex10psubframe+10)<=(frame*10+subframe)) || //normal case
	      ((framex10psubframe>(frame*10+subframe)) && (((10240-framex10psubframe+frame*10+subframe)>=10)))) //frame wrap-around
	    if (flexran_get_p0_pucch_status(mod_id, UE_id, CC_id) == 1) {
	      flexran_update_p0_pucch(mod_id, UE_id, CC_id);
	      
	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame = frame;
	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe = subframe;
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
	  if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
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
	  if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
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
	  if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
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
	  if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
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
	  if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
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
      
      if (flexran_get_duplex_mode(mod_id, CC_id) == PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD) {
        
	/* TODO */
	//set_ul_DAI(mod_id, UE_id, CC_id, frame, subframe, frame_parms);
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

// This function stores the downlink buffer for all the logical channels
void _store_dlsch_buffer (module_id_t Mod_id,
			  frame_t     frameP,
			  sub_frame_t subframeP)
{

  int                   UE_id,i;
  rnti_t                rnti;
  mac_rlc_status_resp_t rlc_status;
  UE_list_t             *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  UE_TEMPLATE           *UE_template;

  for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {

    UE_template = &UE_list->UE_template[UE_PCCID(Mod_id,UE_id)][UE_id];

    // clear logical channel interface variables
    UE_template->dl_buffer_total = 0;
    UE_template->dl_pdus_total = 0;

    for(i=0; i< MAX_NUM_LCID; i++) {
      UE_template->dl_buffer_info[i]=0;
      UE_template->dl_pdus_in_buffer[i]=0;
      UE_template->dl_buffer_head_sdu_creation_time[i]=0;
      UE_template->dl_buffer_head_sdu_remaining_size_to_send[i]=0;
    }

    rnti = UE_RNTI(Mod_id,UE_id);

    for(i=0; i< MAX_NUM_LCID; i++) { // loop over all the logical channels

      rlc_status = mac_rlc_status_ind(Mod_id,rnti, Mod_id,frameP,ENB_FLAG_YES,MBMS_FLAG_NO,i,0 );
      UE_template->dl_buffer_info[i] = rlc_status.bytes_in_buffer; //storing the dlsch buffer for each logical channel
      UE_template->dl_pdus_in_buffer[i] = rlc_status.pdus_in_buffer;
      UE_template->dl_buffer_head_sdu_creation_time[i] = rlc_status.head_sdu_creation_time ;
      UE_template->dl_buffer_head_sdu_creation_time_max = cmax(UE_template->dl_buffer_head_sdu_creation_time_max,
          rlc_status.head_sdu_creation_time );
      UE_template->dl_buffer_head_sdu_remaining_size_to_send[i] = rlc_status.head_sdu_remaining_size_to_send;
      UE_template->dl_buffer_head_sdu_is_segmented[i] = rlc_status.head_sdu_is_segmented;
      UE_template->dl_buffer_total += UE_template->dl_buffer_info[i];//storing the total dlsch buffer
      UE_template->dl_pdus_total   += UE_template->dl_pdus_in_buffer[i];

#ifdef DEBUG_eNB_SCHEDULER

      /* note for dl_buffer_head_sdu_remaining_size_to_send[i] :
       * 0 if head SDU has not been segmented (yet), else remaining size not already segmented and sent
       */
      if (UE_template->dl_buffer_info[i]>0)
        LOG_D(MAC,
              "[eNB %d] Frame %d Subframe %d : RLC status for UE %d in LCID%d: total of %d pdus and size %d, head sdu queuing time %d, remaining size %d, is segmeneted %d \n",
              Mod_id, frameP, subframeP, UE_id,
              i, UE_template->dl_pdus_in_buffer[i],UE_template->dl_buffer_info[i],
              UE_template->dl_buffer_head_sdu_creation_time[i],
              UE_template->dl_buffer_head_sdu_remaining_size_to_send[i],
              UE_template->dl_buffer_head_sdu_is_segmented[i]
             );

#endif

    }

    //#ifdef DEBUG_eNB_SCHEDULER
    if ( UE_template->dl_buffer_total>0)
      LOG_D(MAC,"[eNB %d] Frame %d Subframe %d : RLC status for UE %d : total DL buffer size %d and total number of pdu %d \n",
            Mod_id, frameP, subframeP, UE_id,
            UE_template->dl_buffer_total,
            UE_template->dl_pdus_total
           );

    //#endif
  }
}


// This function returns the estimated number of RBs required by each UE for downlink scheduling
void _assign_rbs_required (module_id_t Mod_id,
                          frame_t     frameP,
                          sub_frame_t subframe,
                          uint16_t    nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
                          int         min_rb_unit[MAX_NUM_CCs])
{


  rnti_t           rnti;
  uint16_t         TBS = 0;
  LTE_eNB_UE_stats *eNB_UE_stats[MAX_NUM_CCs];
  int              UE_id,n,i,j,CC_id,pCCid,tmp;
  UE_list_t        *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  //  UE_TEMPLATE           *UE_template;

  // clear rb allocations across all CC_ids
  for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
    pCCid = UE_PCCID(Mod_id,UE_id);
    rnti = UE_list->UE_template[pCCid][UE_id].rnti;

    //update CQI information across component carriers
    for (n=0; n<UE_list->numactiveCCs[UE_id]; n++) {
      CC_id = UE_list->ordered_CCids[n][UE_id];
      eNB_UE_stats[CC_id] = mac_xface->get_eNB_UE_stats(Mod_id,CC_id,rnti);
      eNB_UE_stats[CC_id]->dlsch_mcs1=cqi_to_mcs[flexran_get_ue_wcqi(Mod_id, UE_id)];
    }

    // provide the list of CCs sorted according to MCS
    for (i=0; i<UE_list->numactiveCCs[UE_id]; i++) {
      for (j=i+1; j<UE_list->numactiveCCs[UE_id]; j++) {
        DevAssert( j < MAX_NUM_CCs );

        if (eNB_UE_stats[UE_list->ordered_CCids[i][UE_id]]->dlsch_mcs1 >
            eNB_UE_stats[UE_list->ordered_CCids[j][UE_id]]->dlsch_mcs1) {
          tmp = UE_list->ordered_CCids[i][UE_id];
          UE_list->ordered_CCids[i][UE_id] = UE_list->ordered_CCids[j][UE_id];
          UE_list->ordered_CCids[j][UE_id] = tmp;
        }
      }
    }

    /* NN --> RK
     * check the index of UE_template"
     */
    if (UE_list->UE_template[pCCid][UE_id].dl_buffer_total> 0) {
      LOG_D(MAC,"[preprocessor] assign RB for UE %d\n",UE_id);

      for (i=0; i<UE_list->numactiveCCs[UE_id]; i++) {
        CC_id = UE_list->ordered_CCids[i][UE_id];
	eNB_UE_stats[CC_id] = mac_xface->get_eNB_UE_stats(Mod_id,CC_id,rnti);

        if (eNB_UE_stats[CC_id]->dlsch_mcs1==0) {
          nb_rbs_required[CC_id][UE_id] = 4;  // don't let the TBS get too small
        } else {
          nb_rbs_required[CC_id][UE_id] = min_rb_unit[CC_id];
        }

        TBS = mac_xface->get_TBS_DL(eNB_UE_stats[CC_id]->dlsch_mcs1,nb_rbs_required[CC_id][UE_id]);

        LOG_D(MAC,"[preprocessor] start RB assignement for UE %d CC_id %d dl buffer %d (RB unit %d, MCS %d, TBS %d) \n",
              UE_id, CC_id, UE_list->UE_template[pCCid][UE_id].dl_buffer_total,
              nb_rbs_required[CC_id][UE_id],eNB_UE_stats[CC_id]->dlsch_mcs1,TBS);

        /* calculating required number of RBs for each UE */
        while (TBS < UE_list->UE_template[pCCid][UE_id].dl_buffer_total)  {
          nb_rbs_required[CC_id][UE_id] += min_rb_unit[CC_id];

          if (nb_rbs_required[CC_id][UE_id] > flexran_get_N_RB_DL(Mod_id, CC_id)) {
            TBS = mac_xface->get_TBS_DL(eNB_UE_stats[CC_id]->dlsch_mcs1, flexran_get_N_RB_DL(Mod_id, CC_id));
            nb_rbs_required[CC_id][UE_id] = flexran_get_N_RB_DL(Mod_id, CC_id);
            break;
          }

          TBS = mac_xface->get_TBS_DL(eNB_UE_stats[CC_id]->dlsch_mcs1,nb_rbs_required[CC_id][UE_id]);
        } // end of while

        LOG_D(MAC,"[eNB %d] Frame %d: UE %d on CC %d: RB unit %d,  nb_required RB %d (TBS %d, mcs %d)\n",
              Mod_id, frameP,UE_id, CC_id,  min_rb_unit[CC_id], nb_rbs_required[CC_id][UE_id], TBS, eNB_UE_stats[CC_id]->dlsch_mcs1);
      }
    }
  }
}

// This function scans all CC_ids for a particular UE to find the maximum round index of its HARQ processes
int _maxround(module_id_t Mod_id,uint16_t rnti,int frame,sub_frame_t subframe,uint8_t ul_flag )
{

  uint8_t round,round_max=0,UE_id;
  int CC_id;
  UE_list_t *UE_list = &eNB_mac_inst[Mod_id].UE_list;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    UE_id = find_UE_id(Mod_id,rnti);
    round    = UE_list->UE_sched_ctrl[UE_id].round[CC_id];
    if (round > round_max) {
      round_max = round;
    }
  }

  return round_max;
}

// This function scans all CC_ids for a particular UE to find the maximum DL CQI
int _maxcqi(module_id_t Mod_id,int32_t UE_id)
{

  LTE_eNB_UE_stats *eNB_UE_stats = NULL;
  UE_list_t *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  int CC_id,n;
  int CQI = 0;

  for (n=0; n<UE_list->numactiveCCs[UE_id]; n++) {
    CC_id = UE_list->ordered_CCids[n][UE_id];
    eNB_UE_stats = mac_xface->get_eNB_UE_stats(Mod_id,CC_id,UE_RNTI(Mod_id,UE_id));

    if (eNB_UE_stats==NULL) {
      mac_xface->macphy_exit("maxcqi: could not get eNB_UE_stats\n");
      return 0; // not reached
    }

    if (eNB_UE_stats->DL_cqi[0] > CQI) {
      CQI = eNB_UE_stats->DL_cqi[0];
    }
  }

  return(CQI);
}


// This fuction sorts the UE in order their dlsch buffer and CQI
void _sort_UEs (module_id_t Mod_idP,
               int         frameP,
               sub_frame_t subframeP)
{


  int               UE_id1,UE_id2;
  int               pCC_id1,pCC_id2;
  int               cqi1,cqi2,round1,round2;
  int               i=0,ii=0;//,j=0;
  rnti_t            rnti1,rnti2;

  UE_list_t *UE_list = &eNB_mac_inst[Mod_idP].UE_list;

  for (i=UE_list->head; i>=0; i=UE_list->next[i]) {

    for(ii=UE_list->next[i]; ii>=0; ii=UE_list->next[ii]) {

      UE_id1  = i;
      rnti1 = UE_RNTI(Mod_idP,UE_id1);
      if(rnti1 == NOT_A_RNTI)
	continue;
      if (UE_list->UE_sched_ctrl[UE_id1].ul_out_of_sync == 1)
	continue;
      pCC_id1 = UE_PCCID(Mod_idP,UE_id1);
      cqi1    = _maxcqi(Mod_idP,UE_id1); //
      round1  = _maxround(Mod_idP,rnti1,frameP,subframeP,0);

      UE_id2 = ii;
      rnti2 = UE_RNTI(Mod_idP,UE_id2);
      if(rnti2 == NOT_A_RNTI)
        continue;
      if (UE_list->UE_sched_ctrl[UE_id2].ul_out_of_sync == 1)
	continue;
      cqi2    = _maxcqi(Mod_idP,UE_id2);
      round2  = _maxround(Mod_idP,rnti2,frameP,subframeP,0);  //mac_xface->get_ue_active_harq_pid(Mod_id,rnti2,subframe,&harq_pid2,&round2,0);
      pCC_id2 = UE_PCCID(Mod_idP,UE_id2);

      if(round2 > round1) { // Check first if one of the UEs has an active HARQ process which needs service and swap order
        swap_UEs(UE_list,UE_id1,UE_id2,0);
      } else if (round2 == round1) {
        // RK->NN : I guess this is for fairness in the scheduling. This doesn't make sense unless all UEs have the same configuration of logical channels.  This should be done on the sum of all information that has to be sent.  And still it wouldn't ensure fairness.  It should be based on throughput seen by each UE or maybe using the head_sdu_creation_time, i.e. swap UEs if one is waiting longer for service.

        // first check the buffer status for SRB1 and SRB2
        if ( (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[1] + UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[2]) <
             (UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[1] + UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[2])   ) {
          swap_UEs(UE_list,UE_id1,UE_id2,0);
        } else if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_head_sdu_creation_time_max <
                   UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_head_sdu_creation_time_max   ) {
          swap_UEs(UE_list,UE_id1,UE_id2,0);
        } else if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_total <
                   UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_total   ) {
          swap_UEs(UE_list,UE_id1,UE_id2,0);
        } else if (cqi1 < cqi2) {
          swap_UEs(UE_list,UE_id1,UE_id2,0);
        }
      }
    }
  }
}

// This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
void _dlsch_scheduler_pre_processor (module_id_t   Mod_id,
                                    frame_t       frameP,
                                    sub_frame_t   subframeP,
                                    int           N_RBG[MAX_NUM_CCs],
                                    int           *mbsfn_flag)
{

  unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX], harq_pid=0, total_ue_count;
  unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX];
  int                     UE_id, i;
  unsigned char round = 0;
  uint16_t                ii,j;
  uint16_t                nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  uint16_t                nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  uint16_t                nb_rbs_required_remaining_1[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  uint16_t                average_rbs_per_user[MAX_NUM_CCs] = {0};
  rnti_t             rnti;
  int                min_rb_unit[MAX_NUM_CCs];
  uint16_t r1=0;
  uint8_t CC_id;
  UE_list_t *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  LTE_DL_FRAME_PARMS   *frame_parms[MAX_NUM_CCs] = {0};

  int transmission_mode = 0;
  UE_sched_ctrl *ue_sched_ctl;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    
    if (mbsfn_flag[CC_id]>0)  // If this CC is allocated for MBSFN skip it here
      continue;
    
    frame_parms[CC_id] = mac_xface->get_lte_frame_parms(Mod_id,CC_id);
    
    
    min_rb_unit[CC_id]=get_min_rb_unit(Mod_id,CC_id);
    
    for (i=UE_list->head; i>=0; i=UE_list->next[i]) {
      UE_id = i;
      // Initialize scheduling information for all active UEs
      
      
      _dlsch_scheduler_pre_processor_reset(Mod_id,
					   UE_id,
					   CC_id,
					   frameP,
					   subframeP,
					   N_RBG[CC_id],
					   nb_rbs_required,
					   nb_rbs_required_remaining,
					   rballoc_sub,
					   MIMO_mode_indicator);

    }
  }
  
  // Store the DLSCH buffer for each logical channel
  _store_dlsch_buffer (Mod_id,frameP,subframeP);

  // Calculate the number of RBs required by each UE on the basis of logical channel's buffer
  _assign_rbs_required (Mod_id,frameP,subframeP,nb_rbs_required,min_rb_unit);

  // Sorts the user on the basis of dlsch logical channel buffer and CQI
  _sort_UEs (Mod_id,frameP,subframeP);

  total_ue_count = 0;

  // loop over all active UEs
  for (i=UE_list->head; i>=0; i=UE_list->next[i]) {
    rnti = flexran_get_ue_crnti(Mod_id, i);
    if(rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    UE_id = i;

    // if there is no available harq_process, skip the UE
    if (UE_list->UE_sched_ctrl[UE_id].harq_pid[CC_id]<0)
      continue;

    for (ii=0; ii < UE_num_active_CC(UE_list,UE_id); ii++) {
      CC_id = UE_list->ordered_CCids[ii][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      flexran_get_harq(Mod_id, CC_id, UE_id, frameP, subframeP, &harq_pid, &round);

      average_rbs_per_user[CC_id]=0;

      frame_parms[CC_id] = mac_xface->get_lte_frame_parms(Mod_id,CC_id);

      //      mac_xface->get_ue_active_harq_pid(Mod_id,CC_id,rnti,frameP,subframeP,&harq_pid,&round,0);

      if(round>0) {
        nb_rbs_required[CC_id][UE_id] = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
      }

      //nb_rbs_required_remaining[UE_id] = nb_rbs_required[UE_id];
      if (nb_rbs_required[CC_id][UE_id] > 0) {
        total_ue_count = total_ue_count + 1;
      }


      // hypotetical assignement
      /*
       * If schedule is enabled and if the priority of the UEs is modified
       * The average rbs per logical channel per user will depend on the level of
       * priority. Concerning the hypothetical assignement, we should assign more
       * rbs to prioritized users. Maybe, we can do a mapping between the
       * average rbs per user and the level of priority or multiply the average rbs
       * per user by a coefficient which represents the degree of priority.
       */

      if (total_ue_count == 0) {
        average_rbs_per_user[CC_id] = 0;
      } else if( (min_rb_unit[CC_id] * total_ue_count) <= (frame_parms[CC_id]->N_RB_DL) ) {
        average_rbs_per_user[CC_id] = (uint16_t) floor(frame_parms[CC_id]->N_RB_DL/total_ue_count);
      } else {
        average_rbs_per_user[CC_id] = min_rb_unit[CC_id]; // consider the total number of use that can be scheduled UE
      }
    }
  }

  // note: nb_rbs_required is assigned according to total_buffer_dl
  // extend nb_rbs_required to capture per LCID RB required
  for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
    rnti = UE_RNTI(Mod_id,i);

    for (ii=0; ii<UE_num_active_CC(UE_list,i); ii++) {
      CC_id = UE_list->ordered_CCids[ii][i];

      // control channel
      if (mac_eNB_get_rrc_status(Mod_id,rnti) < RRC_RECONFIGURED) {
        nb_rbs_required_remaining_1[CC_id][i] = nb_rbs_required[CC_id][i];
      } else {
        nb_rbs_required_remaining_1[CC_id][i] = cmin(average_rbs_per_user[CC_id],nb_rbs_required[CC_id][i]);

      }
    }
  }

  //Allocation to UEs is done in 2 rounds,
  // 1st stage: average number of RBs allocated to each UE
  // 2nd stage: remaining RBs are allocated to high priority UEs
  for(r1=0; r1<2; r1++) {

    for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
      for (ii=0; ii<UE_num_active_CC(UE_list,i); ii++) {
        CC_id = UE_list->ordered_CCids[ii][i];

        if(r1 == 0) {
          nb_rbs_required_remaining[CC_id][i] = nb_rbs_required_remaining_1[CC_id][i];
        } else { // rb required based only on the buffer - rb allloctaed in the 1st round + extra reaming rb form the 1st round
          nb_rbs_required_remaining[CC_id][i] = nb_rbs_required[CC_id][i]-nb_rbs_required_remaining_1[CC_id][i]+nb_rbs_required_remaining[CC_id][i];
        }

        if (nb_rbs_required[CC_id][i]> 0 )
          LOG_D(MAC,"round %d : nb_rbs_required_remaining[%d][%d]= %d (remaining_1 %d, required %d,  pre_nb_available_rbs %d, N_RBG %d, rb_unit %d)\n",
                r1, CC_id, i,
                nb_rbs_required_remaining[CC_id][i],
                nb_rbs_required_remaining_1[CC_id][i],
                nb_rbs_required[CC_id][i],
                UE_list->UE_sched_ctrl[i].pre_nb_available_rbs[CC_id],
                N_RBG[CC_id],
                min_rb_unit[CC_id]);

      }
    }

    if (total_ue_count > 0 ) {
      for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
        UE_id = i;

        for (ii=0; ii<UE_num_active_CC(UE_list,UE_id); ii++) {
          CC_id = UE_list->ordered_CCids[ii][UE_id];
	  ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
	  flexran_get_harq(Mod_id, CC_id, UE_id, frameP, subframeP, &harq_pid, &round);	  
          rnti = UE_RNTI(Mod_id,UE_id);

          // LOG_D(MAC,"UE %d rnti 0x\n", UE_id, rnti );
          if(rnti == NOT_A_RNTI)
            continue;
	  if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
	    continue;

          transmission_mode = mac_xface->get_transmission_mode(Mod_id,CC_id,rnti);
          //rrc_status = mac_eNB_get_rrc_status(Mod_id,rnti);
          /* 1st allocate for the retx */

          // retransmission in data channels
          // control channel in the 1st transmission
          // data channel for all TM
          LOG_T(MAC,"calling dlsch_scheduler_pre_processor_allocate .. \n ");
          _dlsch_scheduler_pre_processor_allocate (Mod_id,
						   UE_id,
						   CC_id,
						   N_RBG[CC_id],
						   transmission_mode,
						   min_rb_unit[CC_id],
						   frame_parms[CC_id]->N_RB_DL,
						   nb_rbs_required,
						   nb_rbs_required_remaining,
						   rballoc_sub,
						   MIMO_mode_indicator);
        }
      }
    } // total_ue_count
  } // end of for for r1 and r2

  for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
    UE_id = i;
    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

    for (ii=0; ii<UE_num_active_CC(UE_list,UE_id); ii++) {
      CC_id = UE_list->ordered_CCids[ii][UE_id];
      //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].dl_pow_off = dl_pow_off[UE_id];

      if (ue_sched_ctl->pre_nb_available_rbs[CC_id] > 0 ) {
        LOG_D(MAC,"******************DL Scheduling Information for UE%d ************************\n",UE_id);
        LOG_D(MAC,"dl power offset UE%d = %d \n",UE_id,ue_sched_ctl->dl_pow_off[CC_id]);
        LOG_D(MAC,"***********RB Alloc for every subband for UE%d ***********\n",UE_id);

        for(j=0; j<N_RBG[CC_id]; j++) {
          //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].rballoc_sub[i] = rballoc_sub_UE[CC_id][UE_id][i];
          LOG_D(MAC,"RB Alloc for UE%d and Subband%d = %d\n",UE_id,j,ue_sched_ctl->rballoc_sub_UE[CC_id][j]);
        }

        //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = pre_nb_available_rbs[CC_id][UE_id];
        LOG_D(MAC,"Total RBs allocated for UE%d = %d\n",UE_id,ue_sched_ctl->pre_nb_available_rbs[CC_id]);
      }
    }
  }
}

#define SF05_LIMIT 1

void _dlsch_scheduler_pre_processor_reset (int module_idP,
					   int UE_id,
					   uint8_t  CC_id,
					   int frameP,
					   int subframeP,					  
					   int N_RBG,
					   uint16_t nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					   uint16_t nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					   unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
					   unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX]) {
  int i,j;
  UE_list_t *UE_list=&eNB_mac_inst[module_idP].UE_list;
  UE_sched_ctrl *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
  rnti_t rnti = UE_RNTI(module_idP,UE_id);
  uint8_t *vrb_map = eNB_mac_inst[module_idP].common_channels[CC_id].vrb_map;
  int RBGsize = PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL/N_RBG;
#ifdef SF05_LIMIT
  //int subframe05_limit=0;
  int sf05_upper=-1,sf05_lower=-1;
#endif
  LTE_eNB_UE_stats *eNB_UE_stats = mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti);
  
  flexran_update_TA(module_idP, UE_id, CC_id);

  if (UE_id==0) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_TIMING_ADVANCE,ue_sched_ctl->ta_update);
  }
  nb_rbs_required[CC_id][UE_id]=0;
  ue_sched_ctl->pre_nb_available_rbs[CC_id] = 0;
  ue_sched_ctl->dl_pow_off[CC_id] = 2;
  nb_rbs_required_remaining[CC_id][UE_id] = 0;

#ifdef SF05_LIMIT  
  switch (N_RBG) {
  case 6:
    sf05_lower=0;
    sf05_upper=5;
    break;
  case 8:
    sf05_lower=2;
    sf05_upper=5;
    break;
  case 13:
    sf05_lower=4;
    sf05_upper=7;
    break;
  case 17:
    sf05_lower=7;
    sf05_upper=9;
    break;
  case 25:
    sf05_lower=11;
    sf05_upper=13;
    break;
  }
#endif
  // Initialize Subbands according to VRB map
  for (i=0; i<N_RBG; i++) {
    ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 0;
    rballoc_sub[CC_id][i] = 0;
#ifdef SF05_LIMIT
    // for avoiding 6+ PRBs around DC in subframe 0-5 (avoid excessive errors)

    if ((subframeP==0 || subframeP==5) && 
	(i>=sf05_lower && i<=sf05_upper))
      rballoc_sub[CC_id][i]=1;
#endif
    // for SI-RNTI,RA-RNTI and P-RNTI allocations
    for (j=0;j<RBGsize;j++) {
      if (vrb_map[j+(i*RBGsize)]!=0)  {
	rballoc_sub[CC_id][i] = 1;
	LOG_D(MAC,"Frame %d, subframe %d : vrb %d allocated\n",frameP,subframeP,j+(i*RBGsize));
	break;
      }
    }
    LOG_D(MAC,"Frame %d Subframe %d CC_id %d RBG %i : rb_alloc %d\n",frameP,subframeP,CC_id,i,rballoc_sub[CC_id][i]);
    MIMO_mode_indicator[CC_id][i] = 2;
  }
}


void _dlsch_scheduler_pre_processor_allocate (module_id_t   Mod_id,
					      int           UE_id,
					      uint8_t       CC_id,
					      int           N_RBG,
					      int           transmission_mode,
					      int           min_rb_unit,
					      uint8_t       N_RB_DL,
					      uint16_t      nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					      uint16_t      nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					      unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
					      unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX]) {
  int i;
  UE_list_t *UE_list=&eNB_mac_inst[Mod_id].UE_list;
  UE_sched_ctrl *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

  for(i=0; i<N_RBG; i++) {

    if((rballoc_sub[CC_id][i] == 0)           &&
        (ue_sched_ctl->rballoc_sub_UE[CC_id][i] == 0) &&
        (nb_rbs_required_remaining[CC_id][UE_id]>0)   &&
        (ue_sched_ctl->pre_nb_available_rbs[CC_id] < nb_rbs_required[CC_id][UE_id])) {

      // if this UE is not scheduled for TM5
      if (ue_sched_ctl->dl_pow_off[CC_id] != 0 )  {

	if ((i == N_RBG-1) && ((N_RB_DL == 25) || (N_RB_DL == 50))) {
	  rballoc_sub[CC_id][i] = 1;
	  ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;
	  MIMO_mode_indicator[CC_id][i] = 1;
	  if (transmission_mode == 5 ) {
	    ue_sched_ctl->dl_pow_off[CC_id] = 1;
	  }   
	  nb_rbs_required_remaining[CC_id][UE_id] = nb_rbs_required_remaining[CC_id][UE_id] - min_rb_unit+1;
          ue_sched_ctl->pre_nb_available_rbs[CC_id] = ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit - 1;
        } else {
	  if (nb_rbs_required_remaining[CC_id][UE_id] >=  min_rb_unit){
	    rballoc_sub[CC_id][i] = 1;
	    ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;
	    MIMO_mode_indicator[CC_id][i] = 1;
	    if (transmission_mode == 5 ) {
	      ue_sched_ctl->dl_pow_off[CC_id] = 1;
	    }
	    nb_rbs_required_remaining[CC_id][UE_id] = nb_rbs_required_remaining[CC_id][UE_id] - min_rb_unit;
	    ue_sched_ctl->pre_nb_available_rbs[CC_id] = ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit;
	  }
	}
      } // dl_pow_off[CC_id][UE_id] ! = 0
    }
  }
}
