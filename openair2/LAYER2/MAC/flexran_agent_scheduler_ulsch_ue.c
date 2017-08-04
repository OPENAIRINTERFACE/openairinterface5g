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

/*! \file eNB_scheduler_ulsch.c
 * \brief FlexRAN eNB procedures for the ULSCH transport channel
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 1.0
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

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "T.h"

/* number of active slices for  past and current time*/
int n_active_slices_uplink = 2;
int n_active_slices_uplink_current = 2;

/* RB share for each slice for past and current time*/
float slice_percentage_uplink[MAX_NUM_SLICES] = {0.5, 0.5, 0.0, 0.0};
float slice_percentage_current_uplink[MAX_NUM_SLICES] = {0.5, 0.5, 0.0, 0.0};
float total_slice_percentage_uplink = 0;

// MAX MCS for each slice for past and current time
int slice_maxmcs_uplink[MAX_NUM_SLICES] = {16, 16, 16, 16};
int slice_maxmcs_current_uplink[MAX_NUM_SLICES] = {28, 28, 28, 28};

/*resource blocks allowed*/
uint16_t                nb_rbs_allowed_slice_uplink[MAX_NUM_CCs][MAX_NUM_SLICES];      
/*Slice Update */
int update_ul_scheduler[MAX_NUM_SLICES] = {1, 1, 1, 1};
int update_ul_scheduler_current[MAX_NUM_SLICES] = {1, 1, 1, 1};

 /* Slice Function Pointer */
slice_scheduler_ul slice_sched_ul[MAX_NUM_SLICES] = {0};

/* name of available scheduler*/
char *ul_scheduler_type[MAX_NUM_SLICES] = {"flexran_schedule_ue_ul_spec_embb",
					   "flexran_schedule_ue_ul_spec_urllc",
					   "flexran_schedule_ue_ul_spec_mmtc",
					   "flexran_schedule_ue_ul_spec_be"      // best effort 
};


uint16_t flexran_nb_rbs_allowed_slice_uplink(float rb_percentage, int total_rbs){
  return  (uint16_t) floor(rb_percentage * total_rbs); 
}


void _assign_max_mcs_min_rb(module_id_t module_idP, int slice_id, int frameP, sub_frame_t subframeP, uint16_t *first_rb)
{

  int                i;
  uint16_t           n,UE_id;
  uint8_t            CC_id;
  rnti_t             rnti           = -1;
  int                mcs;
  int                rb_table_index=0,tbs,tx_power;
  eNB_MAC_INST       *eNB = &eNB_mac_inst[module_idP];
  UE_list_t          *UE_list = &eNB->UE_list;

  UE_TEMPLATE       *UE_template;
  LTE_DL_FRAME_PARMS   *frame_parms;

   
  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    if (UE_list->active[i] != TRUE) continue;

    rnti = UE_RNTI(module_idP,i);

    if (rnti==NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!phy_stats_exist(module_idP, rnti))
      continue;

    if (UE_list->UE_sched_ctrl[i].phr_received == 1)
      mcs = 20; // if we've received the power headroom information the UE, we can go to maximum mcs
    else
      mcs = 10; // otherwise, limit to QPSK PUSCH

    UE_id = i;

    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];

      if (CC_id >= MAX_NUM_CCs) {
        LOG_E( MAC, "CC_id %u should be < %u, loop n=%u < numactiveULCCs[%u]=%u",
               CC_id,
               MAX_NUM_CCs,
               n,
               UE_id,
               UE_list->numactiveULCCs[UE_id]);
      }

      AssertFatal( CC_id < MAX_NUM_CCs, "CC_id %u should be < %u, loop n=%u < numactiveULCCs[%u]=%u",
                   CC_id,
                   MAX_NUM_CCs,
                   n,
                   UE_id,
                   UE_list->numactiveULCCs[UE_id]);
      frame_parms=mac_xface->get_lte_frame_parms(module_idP,CC_id);
      UE_template = &UE_list->UE_template[CC_id][UE_id];
      nb_rbs_allowed_slice_uplink[CC_id][UE_id] = flexran_nb_rbs_allowed_slice_uplink(slice_percentage_uplink[UE_id], flexran_get_N_RB_UL(module_idP, CC_id));
      // if this UE has UL traffic
      if (UE_template->ul_total_buffer > 0 ) {

        tbs = mac_xface->get_TBS_UL(mcs,3);  // 1 or 2 PRB with cqi enabled does not work well!
        // fixme: set use_srs flag
        tx_power= mac_xface->estimate_ue_tx_power(tbs,rb_table[rb_table_index],0,frame_parms->Ncp,0);

        while ((((UE_template->phr_info - tx_power) < 0 ) || (tbs > UE_template->ul_total_buffer))&&
               (mcs > 3)) {
          // LOG_I(MAC,"UE_template->phr_info %d tx_power %d mcs %d\n", UE_template->phr_info,tx_power, mcs);
          mcs--;
          tbs = mac_xface->get_TBS_UL(mcs,rb_table[rb_table_index]);
          tx_power = mac_xface->estimate_ue_tx_power(tbs,rb_table[rb_table_index],0,frame_parms->Ncp,0); // fixme: set use_srs
        }

        while ((tbs < UE_template->ul_total_buffer) &&
               (rb_table[rb_table_index]<(nb_rbs_allowed_slice_uplink[CC_id][UE_id]-first_rb[CC_id])) &&
               ((UE_template->phr_info - tx_power) > 0) &&
               (rb_table_index < 32 )) {
          //  LOG_I(MAC,"tbs %d ul buffer %d rb table %d max ul rb %d\n", tbs, UE_template->ul_total_buffer, rb_table[rb_table_index], frame_parms->N_RB_UL-first_rb[CC_id]);
          rb_table_index++;
          tbs = mac_xface->get_TBS_UL(mcs,rb_table[rb_table_index]);
          tx_power = mac_xface->estimate_ue_tx_power(tbs,rb_table[rb_table_index],0,frame_parms->Ncp,0);
        }

        UE_template->ue_tx_power = tx_power;

        if (rb_table[rb_table_index]>(nb_rbs_allowed_slice_uplink[CC_id][UE_id]-first_rb[CC_id]-1)) {
          rb_table_index--;
        }

        // 1 or 2 PRB with cqi enabled does not work well!
  if (rb_table[rb_table_index]<3) {
          rb_table_index=2; //3PRB
        }

        UE_template->pre_assigned_mcs_ul=mcs;
        UE_template->pre_allocated_rb_table_index_ul=rb_table_index;
        UE_template->pre_allocated_nb_rb_ul= rb_table[rb_table_index];
        LOG_D(MAC,"[eNB %d] frame %d subframe %d: for UE %d CC %d: pre-assigned mcs %d, pre-allocated rb_table[%d]=%d RBs (phr %d, tx power %d)\n",
              module_idP, frameP, subframeP, UE_id, CC_id,
              UE_template->pre_assigned_mcs_ul,
              UE_template->pre_allocated_rb_table_index_ul,
              UE_template->pre_allocated_nb_rb_ul,
              UE_template->phr_info,tx_power);
      } else {
        UE_template->pre_allocated_rb_table_index_ul=-1;
        UE_template->pre_allocated_nb_rb_ul=0;
      }
    }
  }
}




void _ulsch_scheduler_pre_processor(module_id_t module_idP,
                                   int slice_id,                                                     
                                   int frameP,
                                   sub_frame_t subframeP,
                                   uint16_t *first_rb)
{

  int16_t            i;
  uint16_t           UE_id,n,r;
  uint8_t            CC_id, round, harq_pid;
  uint16_t           nb_allocated_rbs[MAX_NUM_CCs][NUMBER_OF_UE_MAX],total_allocated_rbs[MAX_NUM_CCs],average_rbs_per_user[MAX_NUM_CCs];
  uint16_t          nb_rbs_allowed_slice_uplink[MAX_NUM_CCs][MAX_NUM_SLICES];
  int16_t            total_remaining_rbs[MAX_NUM_CCs];
  uint16_t           max_num_ue_to_be_scheduled=0,total_ue_count=0;
  rnti_t             rnti= -1;
  UE_list_t          *UE_list = &eNB_mac_inst[module_idP].UE_list;
  UE_TEMPLATE        *UE_template = 0;
  LTE_DL_FRAME_PARMS   *frame_parms = 0;
  UE_sched_ctrl *ue_sched_ctl;
  

  //LOG_I(MAC,"assign max mcs min rb\n");
  // maximize MCS and then allocate required RB according to the buffer occupancy with the limit of max available UL RB
  _assign_max_mcs_min_rb(module_idP, slice_id, frameP, subframeP, first_rb);

  //LOG_I(MAC,"sort ue \n");
  // sort ues
  sort_ue_ul (module_idP,frameP, subframeP);


  // we need to distribute RBs among UEs
  // step1:  reset the vars
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    total_allocated_rbs[CC_id]=0;
    total_remaining_rbs[CC_id]=0;
    average_rbs_per_user[CC_id]=0;

    for (i=UE_list->head_ul; i>=0; i=UE_list->next_ul[i]) {
      nb_allocated_rbs[CC_id][i]=0;
    }
  }

  //LOG_I(MAC,"step2 \n");
  // step 2: calculate the average rb per UE
  total_ue_count =0;
  max_num_ue_to_be_scheduled=0;

  for (i=UE_list->head_ul; i>=0; i=UE_list->next_ul[i]) {

    rnti = UE_RNTI(module_idP,i);

    if (rnti==NOT_A_RNTI)
      continue;

    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;

    if (!phy_stats_exist(module_idP, rnti))
      continue;

    UE_id = i;

    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      UE_template = &UE_list->UE_template[CC_id][UE_id];
      average_rbs_per_user[CC_id]=0;
      frame_parms = mac_xface->get_lte_frame_parms(module_idP,CC_id);

      if (UE_template->pre_allocated_nb_rb_ul > 0) {
        total_ue_count+=1;
      }
      /*
      if((mac_xface->get_nCCE_max(module_idP,CC_id,3,subframeP) - nCCE_to_be_used[CC_id])  > (1<<aggregation)) {
        nCCE_to_be_used[CC_id] = nCCE_to_be_used[CC_id] + (1<<aggregation);
        max_num_ue_to_be_scheduled+=1;
  }*/

      max_num_ue_to_be_scheduled+=1;

     nb_rbs_allowed_slice_uplink[CC_id][UE_id] = flexran_nb_rbs_allowed_slice_uplink(slice_percentage_uplink[UE_id], flexran_get_N_RB_UL(module_idP, CC_id));

      if (total_ue_count == 0) {
        average_rbs_per_user[CC_id] = 0;
      } else if (total_ue_count == 1 ) { // increase the available RBs, special case,
        average_rbs_per_user[CC_id] = nb_rbs_allowed_slice_uplink[CC_id][i]-first_rb[CC_id]+1;
      } else if( (total_ue_count <= (nb_rbs_allowed_slice_uplink[CC_id][i]-first_rb[CC_id])) &&
                 (total_ue_count <= max_num_ue_to_be_scheduled)) {
        average_rbs_per_user[CC_id] = (uint16_t) floor((nb_rbs_allowed_slice_uplink[CC_id][i]-first_rb[CC_id])/total_ue_count);
      } else if (max_num_ue_to_be_scheduled > 0 ) {
        average_rbs_per_user[CC_id] = (uint16_t) floor((nb_rbs_allowed_slice_uplink[CC_id][i]-first_rb[CC_id])/max_num_ue_to_be_scheduled);
      } else {
        average_rbs_per_user[CC_id]=1;
        LOG_W(MAC,"[eNB %d] frame %d subframe %d: UE %d CC %d: can't get average rb per user (should not be here)\n",
              module_idP,frameP,subframeP,UE_id,CC_id);
      }
    }
  }
  if (total_ue_count > 0)
    LOG_D(MAC,"[eNB %d] Frame %d subframe %d: total ue to be scheduled %d/%d\n",
    module_idP, frameP, subframeP,total_ue_count, max_num_ue_to_be_scheduled);

  //LOG_D(MAC,"step3\n");

  // step 3: assigne RBS
  for (i=UE_list->head_ul; i>=0; i=UE_list->next_ul[i]) {
    rnti = UE_RNTI(module_idP,i);

    if (rnti==NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!phy_stats_exist(module_idP, rnti))
      continue;

    UE_id = i;

    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];      
      ue_sched_ctl->max_allowed_rbs[CC_id]=nb_rbs_allowed_slice_uplink[CC_id][UE_id];
      // mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,openair_harq_UL);
      flexran_get_harq(module_idP, CC_id, UE_id, frameP, subframeP, &harq_pid, &round, openair_harq_UL);
      if(round>0) {
        nb_allocated_rbs[CC_id][UE_id] = UE_list->UE_template[CC_id][UE_id].nb_rb_ul[harq_pid];
      } else {
        nb_allocated_rbs[CC_id][UE_id] = cmin(UE_list->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul, average_rbs_per_user[CC_id]);
      }

      total_allocated_rbs[CC_id]+= nb_allocated_rbs[CC_id][UE_id];

    }
  }

  // step 4: assigne the remaining RBs and set the pre_allocated rbs accordingly
  for(r=0; r<2; r++) {

    for (i=UE_list->head_ul; i>=0; i=UE_list->next_ul[i]) {
      rnti = UE_RNTI(module_idP,i);

      if (rnti==NOT_A_RNTI)
        continue;
      if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
  continue;
      if (!phy_stats_exist(module_idP, rnti))
        continue;

      UE_id = i;

      for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
        // This is the actual CC_id in the list
        CC_id = UE_list->ordered_ULCCids[n][UE_id];
        UE_template = &UE_list->UE_template[CC_id][UE_id];        
        frame_parms = mac_xface->get_lte_frame_parms(module_idP,CC_id);
        total_remaining_rbs[CC_id]=nb_rbs_allowed_slice_uplink[CC_id][UE_id] - first_rb[CC_id] - total_allocated_rbs[CC_id];

        if (total_ue_count == 1 ) {
          total_remaining_rbs[CC_id]+=1;
        }

        if ( r == 0 ) {
          while ( (UE_template->pre_allocated_nb_rb_ul > 0 ) &&
                  (nb_allocated_rbs[CC_id][UE_id] < UE_template->pre_allocated_nb_rb_ul) &&
                  (total_remaining_rbs[CC_id] > 0)) {
            nb_allocated_rbs[CC_id][UE_id] = cmin(nb_allocated_rbs[CC_id][UE_id]+1,UE_template->pre_allocated_nb_rb_ul);
            total_remaining_rbs[CC_id]--;
            total_allocated_rbs[CC_id]++;
          }
        } else {
          UE_template->pre_allocated_nb_rb_ul= nb_allocated_rbs[CC_id][UE_id];
          LOG_D(MAC,"******************UL Scheduling Information for UE%d CC_id %d ************************\n",UE_id, CC_id);
          LOG_D(MAC,"[eNB %d] total RB allocated for UE%d CC_id %d  = %d\n", module_idP, UE_id, CC_id, UE_template->pre_allocated_nb_rb_ul);
        }
      }
    }
  }

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms= mac_xface->get_lte_frame_parms(module_idP,CC_id);

    if (total_allocated_rbs[CC_id]>0) {
      LOG_D(MAC,"[eNB %d] total RB allocated for all UEs = %d/%d\n", module_idP, total_allocated_rbs[CC_id], nb_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id]);
    }
  }
}

/*
 * Main Uplink Slicing 
 *
 */

void
flexran_schedule_ue_ul_spec_default(mid_t   mod_id,
				 uint32_t      frame,
				 uint32_t      cooperation_flag,
				 int           subframe,
				 unsigned char sched_subframe,
				 Protocol__FlexranMessage **ul_info)
//------------------------------------------------------------------------------
{
  int i=0;
  
  flexran_agent_mac_create_empty_ul_config(mod_id, ul_info);
   
  for (i = 0; i < n_active_slices_uplink; i++) {
    
    // Load any updated functions
    if (update_ul_scheduler[i] > 0 ) {
      slice_sched_ul[i] = dlsym(NULL, ul_scheduler_type[i]); 
      update_ul_scheduler[i] = 0;
      update_ul_scheduler_current[i] = 0;
      slice_percentage_current_uplink[i]= slice_percentage_uplink[i];
      total_slice_percentage_uplink+=slice_percentage_uplink[i];
      LOG_N(MAC,"update ul scheduler slice %d\n", i);
    }
 
    // check if the number of slices has changed, and log 
    if (n_active_slices_uplink_current != n_active_slices_uplink ){
      if ((n_active_slices_uplink > 0) && (n_active_slices_uplink <= MAX_NUM_SLICES)) {
	LOG_N(MAC,"[eNB %d]frame %d subframe %d: number of active slices has changed: %d-->%d\n",
	      mod_id, frame, subframe, n_active_slices_uplink_current, n_active_slices_uplink);

	n_active_slices_uplink_current = n_active_slices_uplink;

      } else {
	LOG_W(MAC,"invalid number of slices %d, revert to the previous value %d\n",n_active_slices_uplink, n_active_slices_uplink_current);
	n_active_slices_uplink = n_active_slices_uplink_current;
      }
    }
    
    // check if the slice rb share has changed, and log the console
    if (slice_percentage_current_uplink[i] != slice_percentage_uplink[i]){
 //      if ((slice_percentage_uplink[i] >= 0.0) && (slice_percentage_uplink[i] <= 1.0)){
	// if ((total_slice_percentage_uplink - slice_percentage_current_uplink[i]  + slice_percentage_uplink[i]) <= 1.0) {
	//   total_slice_percentage_uplink=total_slice_percentage_uplink - slice_percentage_current_uplink[i]  + slice_percentage_uplink[i];
	  LOG_N(MAC,"[eNB %d][SLICE %d] frame %d subframe %d: total percentage %f, slice RB percentage has changed: %f-->%f\n",
		mod_id, i, frame, subframe, total_slice_percentage_uplink, slice_percentage_current_uplink[i], slice_percentage_uplink[i]);

	  slice_percentage_current_uplink[i] = slice_percentage_uplink[i];

	// } else {
	//   LOG_W(MAC,"[eNB %d][SLICE %d] invalid total RB share (%f->%f), revert the previous value (%f->%f)\n",
	// 	mod_id,i,  
	// 	total_slice_percentage_uplink,
	// 	total_slice_percentage_uplink - slice_percentage_current_uplink[i]  + slice_percentage_uplink[i],
	// 	slice_percentage_uplink[i],slice_percentage_current_uplink[i]);

	//   slice_percentage_uplink[i]= slice_percentage_current_uplink[i];

	// }
 //      } else {
	// LOG_W(MAC,"[eNB %d][SLICE %d] invalid slice RB share, revert the previous value (%f->%f)\n",mod_id, i,  slice_percentage_uplink[i],slice_percentage_current_uplink[i]);

	// slice_percentage_uplink[i]= slice_percentage_current_uplink[i];

      // }
    }
  
    // check if a new scheduler, and log the console
    if (update_ul_scheduler_current[i] != update_ul_scheduler[i]){
      LOG_N(MAC,"[eNB %d][SLICE %d] frame %d subframe %d: DL scheduler for this slice is updated: %s \n",
	    mod_id, i, frame, subframe, ul_scheduler_type[i]);

      update_ul_scheduler_current[i] = update_ul_scheduler[i];
      
    }

    // Run each enabled slice-specific schedulers one by one
    //LOG_N(MAC,"[eNB %d]frame %d subframe %d slice %d: calling the scheduler\n", mod_id, frame, subframe,i);
    

    

  }

    slice_sched_ul[0](mod_id, frame, cooperation_flag, subframe, sched_subframe,ul_info);
  
}

void
flexran_schedule_ue_ul_spec_embb(mid_t  mod_id,
			                    frame_t frame, 
			                    unsigned char cooperation_flag,
                		        uint32_t      subframe,
			                    unsigned char sched_subframe,
			                    Protocol__FlexranMessage **ul_info)

{
  flexran_agent_schedule_ulsch_ue_spec(mod_id,
				                      frame,
				                      cooperation_flag,
				                      subframe,
				                      sched_subframe,
				                      ul_info);
  
}


void flexran_agent_schedule_ulsch_ue_spec(mid_t module_idP, 
		    frame_t frameP,
		    unsigned char cooperation_flag,
		    sub_frame_t subframeP, 
		    unsigned char sched_subframe,
        Protocol__FlexranMessage **ul_info) {


  uint16_t first_rb[MAX_NUM_CCs],i;
  int CC_id;
  eNB_MAC_INST *eNB=&eNB_mac_inst[module_idP];

  start_meas(&eNB->schedule_ulsch);


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    //leave out first RB for PUCCH
    first_rb[CC_id] = 1;

    // UE data info;
    // check which UE has data to transmit
    // function to decide the scheduling
    // e.g. scheduling_rslt = Greedy(granted_UEs, nb_RB)

    // default function for default scheduling
    //

    // output of scheduling, the UE numbers in RBs, where it is in the code???
    // check if RA (Msg3) is active in this subframeP, if so skip the PRBs used for Msg3
    // Msg3 is using 1 PRB so we need to increase first_rb accordingly
    // not sure about the break (can there be more than 1 active RA procedure?)

    for (i=0; i<NB_RA_PROC_MAX; i++) {
      if ((eNB->common_channels[CC_id].RA_template[i].RA_active == TRUE) &&
          (eNB->common_channels[CC_id].RA_template[i].generate_rar == 0) &&
          (eNB->common_channels[CC_id].RA_template[i].generate_Msg4 == 0) &&
          (eNB->common_channels[CC_id].RA_template[i].wait_ack_Msg4 == 0) &&
          (eNB->common_channels[CC_id].RA_template[i].Msg3_subframe == sched_subframe)) {
        first_rb[CC_id]++;
        eNB->common_channels[CC_id].RA_template[i].Msg3_subframe = -1;
        break;
      }
    }

    /*
    if (mac_xface->is_prach_subframe(&(mac_xface->lte_frame_parms),frameP,subframeP)) {
      first_rb[CC_id] = (mac_xface->get_prach_prb_offset(&(mac_xface->lte_frame_parms),
    */

  }

  flexran_agent_schedule_ulsch_rnti(module_idP, cooperation_flag, frameP, subframeP, sched_subframe,first_rb);

  stop_meas(&eNB->schedule_ulsch);

}



void flexran_agent_schedule_ulsch_rnti(module_id_t   module_idP,
                         unsigned char cooperation_flag,
                         frame_t       frameP,
                         sub_frame_t   subframeP,
                         unsigned char sched_subframe,
                         uint16_t     *first_rb)
{

  int                UE_id;
  uint8_t            aggregation    = 2;
  rnti_t             rnti           = -1;
  uint8_t            round          = 0;
  uint8_t            harq_pid       = 0;
  void              *ULSCH_dci      = NULL;
  LTE_eNB_UE_stats  *eNB_UE_stats   = NULL;
  DCI_PDU           *DCI_pdu;
  uint8_t                 status         = 0;
  uint8_t                 rb_table_index = -1;
  uint16_t                TBS = 0;
  //  int32_t                buffer_occupancy=0;
  uint32_t                cqi_req,cshift,ndi,mcs=0,rballoc,tpc;
  int32_t                 normalized_rx_power, target_rx_power=-90;
  static int32_t          tpc_accumulated=0;

  int n,CC_id = 0;
  eNB_MAC_INST      *eNB=&eNB_mac_inst[module_idP];
  UE_list_t         *UE_list=&eNB->UE_list;
  UE_TEMPLATE       *UE_template;
  UE_sched_ctrl     *UE_sched_ctrl;

  //  int                rvidx_tab[4] = {0,2,3,1};
  LTE_DL_FRAME_PARMS   *frame_parms;
  int drop_ue=0;

  //  LOG_I(MAC,"entering ulsch preprocesor\n");


  /*TODO*/
  int slice_id = 0;

  
  _ulsch_scheduler_pre_processor(module_idP,
                                slice_id,  
                                frameP,
                                subframeP,
                                first_rb);

  //  LOG_I(MAC,"exiting ulsch preprocesor\n");

  // loop over all active UEs
  for (UE_id=UE_list->head_ul; UE_id>=0; UE_id=UE_list->next_ul[UE_id]) {

    // don't schedule if Msg4 is not received yet
    if (UE_list->UE_template[UE_PCCID(module_idP,UE_id)][UE_id].configured==FALSE) {
      LOG_I(MAC,"[eNB %d] frame %d subfarme %d, UE %d: not configured, skipping UE scheduling \n", 
	    module_idP,frameP,subframeP,UE_id);
      continue;
    }

    rnti = flexran_get_ue_crnti(module_idP, UE_id);

    if (rnti==NOT_A_RNTI) {
      LOG_W(MAC,"[eNB %d] frame %d subfarme %d, UE %d: no RNTI \n", module_idP,frameP,subframeP,UE_id);
      continue;
    }

    /* let's drop the UE if get_eNB_UE_stats returns NULL when calling it with any of the UE's active UL CCs */
    /* TODO: refine? */
    drop_ue = 0;
    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      if (mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti) == NULL) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: no PHY context\n", module_idP,frameP,subframeP,UE_id,rnti,CC_id);
        drop_ue = 1;
        break;
      }
    }
    if (drop_ue == 1) {
/* we can't come here, ulsch_scheduler_pre_processor won't put in the list a UE with no PHY context */
abort();
      /* TODO: this is a hack. Sometimes the UE has no PHY context but
       * is still present in the MAC with 'ul_failure_timer' = 0 and
       * 'ul_out_of_sync' = 0. It seems wrong and the UE stays there forever. Let's
       * start an UL out of sync procedure in this case.
       * The root cause of this problem has to be found and corrected.
       * In the meantime, this hack...
       */
      if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer == 0 &&
          UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 0) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: UE in weird state, let's put it 'out of sync'\n",
              module_idP,frameP,subframeP,UE_id,rnti,CC_id);
        // inform RRC of failure and clear timer
        mac_eNB_rrc_ul_failure(module_idP,CC_id,frameP,subframeP,rnti);
        UE_list->UE_sched_ctrl[UE_id].ul_failure_timer=0;
        UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync=1;
      }
      continue;
    }

    // loop over all active UL CC_ids for this UE
    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      frame_parms = mac_xface->get_lte_frame_parms(module_idP,CC_id);
      eNB_UE_stats = mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti);

      aggregation=get_aggregation(get_bw_index(module_idP,CC_id), 
				  eNB_UE_stats->DL_cqi[0],
				  format0);
      
      if (CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,aggregation,rnti)) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: not enough nCCE\n", module_idP,frameP,subframeP,UE_id,rnti,CC_id);
        continue; // break;
      } else{
	LOG_D(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d mode %s: aggregation level %d\n", 
	      module_idP,frameP,subframeP,UE_id,rnti,CC_id, mode_string[eNB_UE_stats->mode], 1<<aggregation);
      }


      if (eNB_UE_stats->mode == PUSCH) { // ue has a ulsch channel

        DCI_pdu = &eNB->common_channels[CC_id].DCI_pdu;
        UE_template   = &UE_list->UE_template[CC_id][UE_id];
        UE_sched_ctrl = &UE_list->UE_sched_ctrl[UE_id];

        if (flexran_get_harq(module_idP, CC_id, UE_id, frameP, subframeP, &harq_pid, &round, openair_harq_UL) == -1 ) {
          LOG_W(MAC,"[eNB %d] Scheduler Frame %d, subframeP %d: candidate harq_pid from PHY for UE %d CC %d RNTI %x\n",
                module_idP,frameP,subframeP, UE_id, CC_id, rnti);
          continue;
        } else
          LOG_T(MAC,"[eNB %d] Frame %d, subframeP %d, UE %d CC %d : got harq pid %d  round %d (rnti %x,mode %s)\n",
                module_idP,frameP,subframeP,UE_id,CC_id, harq_pid, round,rnti,mode_string[eNB_UE_stats->mode]);

	PHY_vars_eNB_g[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP] = UE_template->ul_total_buffer;
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BO,PHY_vars_eNB_g[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP]);	
        if (((UE_is_to_be_scheduled(module_idP,CC_id,UE_id)>0)) || (round>0))// || ((frameP%10)==0))
          // if there is information on bsr of DCCH, DTCH or if there is UL_SR, or if there is a packet to retransmit, or we want to schedule a periodic feedback every 10 frames
        {
	  LOG_D(MAC,"[eNB %d][PUSCH] Frame %d subframe %d Scheduling UE %d/%x in round %d(SR %d,UL_inactivity timer %d,UL_failure timer %d)\n",
		module_idP,frameP,subframeP,UE_id,rnti,round,UE_template->ul_SR,
		UE_sched_ctrl->ul_inactivity_timer,
		UE_sched_ctrl->ul_failure_timer);
          // reset the scheduling request
          UE_template->ul_SR = 0;
          // status = mac_eNB_get_rrc_status(module_idP,rnti);
          status = flexran_get_rrc_status(module_idP, rnti);

	  if (status < RRC_CONNECTED)
	    cqi_req = 0;
	  else if (UE_sched_ctrl->cqi_req_timer>30) {
	    cqi_req = 1;
	    UE_sched_ctrl->cqi_req_timer=0;
	  }
	  else
	    cqi_req = 0;

          //power control
          //compute the expected ULSCH RX power (for the stats)

          // this is the normalized RX power and this should be constant (regardless of mcs
          normalized_rx_power = eNB_UE_stats->UL_rssi[0];
          target_rx_power = mac_xface->get_target_pusch_rx_power(module_idP,CC_id);

          // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_template->pusch_tpc_tx_frame*10+UE_template->pusch_tpc_tx_subframe;
          if (((framex10psubframe+10)<=(frameP*10+subframeP)) || //normal case
	      ((framex10psubframe>(frameP*10+subframeP)) && (((10240-framex10psubframe+frameP*10+subframeP)>=10)))) //frame wrap-around
	    {
	    UE_template->pusch_tpc_tx_frame=frameP;
	    UE_template->pusch_tpc_tx_subframe=subframeP;
            if (normalized_rx_power>(target_rx_power+1)) {
              tpc = 0; //-1
              tpc_accumulated--;
            } else if (normalized_rx_power<(target_rx_power-1)) {
              tpc = 2; //+1
              tpc_accumulated++;
            } else {
              tpc = 1; //0
            }
          } else {
            tpc = 1; //0
          }

	  if (tpc!=1) {
	    LOG_D(MAC,"[eNB %d] ULSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
		  module_idP,frameP,subframeP,harq_pid,tpc,
		  tpc_accumulated,normalized_rx_power,target_rx_power);
	  }

          // new transmission
          if (round==0) {

            ndi = 1-UE_template->oldNDI_UL[harq_pid];
            UE_template->oldNDI_UL[harq_pid]=ndi;
	    UE_list->eNB_UE_stats[CC_id][UE_id].normalized_rx_power=normalized_rx_power;
	    UE_list->eNB_UE_stats[CC_id][UE_id].target_rx_power=target_rx_power;
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1=UE_template->pre_assigned_mcs_ul;
            mcs = UE_template->pre_assigned_mcs_ul;//cmin (UE_template->pre_assigned_mcs_ul, openair_daq_vars.target_ue_ul_mcs); // adjust, based on user-defined MCS
            if (UE_template->pre_allocated_rb_table_index_ul >=0) {
              rb_table_index=UE_template->pre_allocated_rb_table_index_ul;
            } else {
	      mcs=10;//cmin (10, openair_daq_vars.target_ue_ul_mcs);
              rb_table_index=5; // for PHR
	    }

            UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2=mcs;
	    //            buffer_occupancy = UE_template->ul_total_buffer;

            while (((rb_table[rb_table_index]>(nb_rbs_allowed_slice_uplink[CC_id][UE_id]-1-first_rb[CC_id])) ||
		    (rb_table[rb_table_index]>45)) &&
                   (rb_table_index>0)) {
              rb_table_index--;
            }

            TBS = mac_xface->get_TBS_UL(mcs,rb_table[rb_table_index]);
	    UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx+=rb_table[rb_table_index];
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS=TBS;
	    //            buffer_occupancy -= TBS;
            rballoc = mac_xface->computeRIV(frame_parms->N_RB_UL,
                                            first_rb[CC_id],
                                            rb_table[rb_table_index]);

            T(T_ENB_MAC_UE_UL_SCHEDULE, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP),
              T_INT(subframeP), T_INT(harq_pid), T_INT(mcs), T_INT(first_rb[CC_id]), T_INT(rb_table[rb_table_index]),
              T_INT(TBS), T_INT(ndi));

	    if (mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED)
	      LOG_I(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE %d (mcs %d, first rb %d, nb_rb %d, rb_table_index %d, TBS %d, harq_pid %d)\n",
		    module_idP,harq_pid,rnti,CC_id,frameP,subframeP,UE_id,mcs,
		    first_rb[CC_id],rb_table[rb_table_index],
		    rb_table_index,TBS,harq_pid);

	    // bad indices : 20 (40 PRB), 21 (45 PRB), 22 (48 PRB)
            // increment for next UE allocation
            first_rb[CC_id]+=rb_table[rb_table_index];
            //store for possible retransmission
            UE_template->nb_rb_ul[harq_pid] = rb_table[rb_table_index];
	    UE_sched_ctrl->ul_scheduled |= (1<<harq_pid);
	    if (UE_id == UE_list->head)
	      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SCHEDULED,UE_sched_ctrl->ul_scheduled);

	    // adjust total UL buffer status by TBS, wait for UL sdus to do final update
	    LOG_D(MAC,"[eNB %d] CC_id %d UE %d/%x : adjusting ul_total_buffer, old %d, TBS %d\n", module_idP,CC_id,UE_id,rnti,UE_template->ul_total_buffer,TBS);
	    if (UE_template->ul_total_buffer > TBS)
	      UE_template->ul_total_buffer -= TBS;
	    else
	      UE_template->ul_total_buffer = 0;
	    LOG_D(MAC,"ul_total_buffer, new %d\n", UE_template->ul_total_buffer);
	    // Cyclic shift for DM RS
	    cshift = 0;// values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
	    	    
	    if (frame_parms->frame_type == TDD) {
	      switch (frame_parms->N_RB_UL) {
	      case 6:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_1_5MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_1_5MHz_TDD_1_6_t,
				format0,
				0);
		break;
		
	      default:
	      case 25:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_5MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_5MHz_TDD_1_6_t,
				format0,
				0);
		break;
		
	      case 50:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_10MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_10MHz_TDD_1_6_t,
				format0,
				0);
		break;
		
	      case 100:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_20MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_20MHz_TDD_1_6_t,
				format0,
				0);
		break;
	      }
	    } // TDD
	    else { //FDD
	      switch (frame_parms->N_RB_UL) {
	      case 25:
	      default:
		
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_5MHz_FDD_t),
				aggregation,
				sizeof_DCI0_5MHz_FDD_t,
				format0,
				0);
		break;
		
	      case 6:
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_1_5MHz_FDD_t),
				aggregation,
				sizeof_DCI0_1_5MHz_FDD_t,
				format0,
				0);
		break;
		
	      case 50:
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_10MHz_FDD_t),
				aggregation,
				sizeof_DCI0_10MHz_FDD_t,
				format0,
				0);
		break;
		
	      case 100:
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_20MHz_FDD_t),
				aggregation,
				sizeof_DCI0_20MHz_FDD_t,
				format0,
				0);
		break;
		
	      }
	    }


	    add_ue_ulsch_info(module_idP,
			      CC_id,
			      UE_id,
			      subframeP,
			      S_UL_SCHEDULED);
	    
	    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0\n", module_idP,CC_id,frameP,subframeP,UE_id);
#ifdef DEBUG
	    dump_dci(frame_parms, &DCI_pdu->dci_alloc[DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci-1]);
#endif
	    
          }
	  else {
            T(T_ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP),
              T_INT(subframeP), T_INT(harq_pid), T_INT(mcs), T_INT(first_rb[CC_id]), T_INT(rb_table[rb_table_index]),
              T_INT(round));

            LOG_D(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) UE %d (mcs %d, first rb %d, nb_rb %d, rb_table_index %d, TBS %d, harq_pid %d,round %d)\n",
                  module_idP,harq_pid,rnti,CC_id,frameP,subframeP,UE_id,mcs,
                  first_rb[CC_id],rb_table[rb_table_index],
                  rb_table_index,TBS,harq_pid,round);
	  }/* 
	  else if (round > 0) { //we schedule a retransmission

            ndi = UE_template->oldNDI_UL[harq_pid];

            if ((round&3)==0) {
              mcs = openair_daq_vars.target_ue_ul_mcs;
            } else {
              mcs = rvidx_tab[round&3] + 28; //not correct for round==4!

            }

            LOG_I(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE retransmission (mcs %d, first rb %d, nb_rb %d, harq_pid %d, round %d)\n",
                  module_idP,UE_id,rnti,CC_id,frameP,subframeP,mcs,
                  first_rb[CC_id],UE_template->nb_rb_ul[harq_pid],
		  harq_pid, round);

            rballoc = mac_xface->computeRIV(frame_parms->N_RB_UL,
                                            first_rb[CC_id],
                                            UE_template->nb_rb_ul[harq_pid]);
            first_rb[CC_id]+=UE_template->nb_rb_ul[harq_pid];  // increment for next UE allocation
         
	    UE_list->eNB_UE_stats[CC_id][UE_id].num_retransmission_rx+=1;
	    UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx_rx=UE_template->nb_rb_ul[harq_pid];
	    UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx+=UE_template->nb_rb_ul[harq_pid];
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1=mcs;
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2=mcs;
	  }
	   */

        } // UE_is_to_be_scheduled
      } // UE is in PUSCH
    } // loop over UE_id
  } // loop of CC_id
}

