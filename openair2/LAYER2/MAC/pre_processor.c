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

/*! \file pre_processor.c
 * \brief eNB scheduler preprocessing fuction prior to scheduling
 * \author Navid Nikaein and Ankit Bhamri
 * \date 2013 - 2014
 * \email navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#define _GNU_SOURCE
#include <stdlib.h>

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
#include "rlc.h"



#define DEBUG_eNB_SCHEDULER 1
#define DEBUG_HEADER_PARSING 1
//#define DEBUG_PACKET_TRACE 1

//#define ICIC 0

/*
  #ifndef USER_MODE
  #define msg debug_msg
  #endif
*/

/* this function checks that get_eNB_UE_stats returns
 * a non-NULL pointer for all the active CCs of an UE
 */
int phy_stats_exist(module_id_t Mod_id, int rnti)
{
  int CC_id;
  int i;
  int UE_id          = find_UE_id(Mod_id, rnti);
  UE_list_t *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  if (UE_id == -1) {
    LOG_W(MAC, "[eNB %d] UE %x not found, should be there (in phy_stats_exist)\n",
	  Mod_id, rnti);
    return 0;
  }
  if (UE_list->numactiveCCs[UE_id] == 0) {
    LOG_W(MAC, "[eNB %d] UE %x has no active CC (in phy_stats_exist)\n",
	  Mod_id, rnti);
    return 0;
  }
  for (i = 0; i < UE_list->numactiveCCs[UE_id]; i++) {
    CC_id = UE_list->ordered_CCids[i][UE_id];
    if (mac_xface->get_eNB_UE_stats(Mod_id, CC_id, rnti) == NULL)
      return 0;
  }
  return 1;
}

// This function stores the downlink buffer for all the logical channels
void store_dlsch_buffer (module_id_t Mod_id,
                         frame_t     frameP,
                         sub_frame_t subframeP)
{

  int                   UE_id,i;
  rnti_t                rnti;
  mac_rlc_status_resp_t rlc_status;
  UE_list_t             *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  UE_TEMPLATE           *UE_template;

  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
    if (UE_list->active[UE_id] != TRUE) continue;

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

      rlc_status = mac_rlc_status_ind(Mod_id,rnti, Mod_id,frameP,subframeP,ENB_FLAG_YES,MBMS_FLAG_NO,i,0 );
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
void assign_rbs_required (module_id_t Mod_id,
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
  LTE_DL_FRAME_PARMS   *frame_parms[MAX_NUM_CCs];

  // clear rb allocations across all CC_ids
  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
    if (UE_list->active[UE_id] != TRUE) continue;

    pCCid = UE_PCCID(Mod_id,UE_id);
    rnti = UE_list->UE_template[pCCid][UE_id].rnti;

    /* skip UE not present in PHY (for any of its active CCs) */
    if (!phy_stats_exist(Mod_id, rnti))
      continue;

    //update CQI information across component carriers
    for (n=0; n<UE_list->numactiveCCs[UE_id]; n++) {

      CC_id = UE_list->ordered_CCids[n][UE_id];
      frame_parms[CC_id] = mac_xface->get_lte_frame_parms(Mod_id,CC_id);
      eNB_UE_stats[CC_id] = mac_xface->get_eNB_UE_stats(Mod_id,CC_id,rnti);
      /*
      DevCheck(((eNB_UE_stats[CC_id]->DL_cqi[0] < MIN_CQI_VALUE) || (eNB_UE_stats[CC_id]->DL_cqi[0] > MAX_CQI_VALUE)),
      eNB_UE_stats[CC_id]->DL_cqi[0], MIN_CQI_VALUE, MAX_CQI_VALUE);
      */
      eNB_UE_stats[CC_id]->dlsch_mcs1=cqi_to_mcs[eNB_UE_stats[CC_id]->DL_cqi[0]];

      eNB_UE_stats[CC_id]->dlsch_mcs1 = eNB_UE_stats[CC_id]->dlsch_mcs1;//cmin(eNB_UE_stats[CC_id]->dlsch_mcs1,openair_daq_vars.target_ue_dl_mcs);

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

    /*
    if ((mac_get_rrc_status(Mod_id,1,UE_id) < RRC_RECONFIGURED)){  // If we still don't have a default radio bearer
      nb_rbs_required[pCCid][UE_id] = PHY_vars_eNB_g[Mod_id][pCCid]->frame_parms.N_RB_DL;
      continue;
    }
    */
    /* NN --> RK
     * check the index of UE_template"
     */
    //    if (UE_list->UE_template[UE_id]->dl_buffer_total> 0) {
    if (UE_list->UE_template[pCCid][UE_id].dl_buffer_total> 0) {
      LOG_D(MAC,"[preprocessor] assign RB for UE %d\n",UE_id);

      for (i=0; i<UE_list->numactiveCCs[UE_id]; i++) {
        CC_id = UE_list->ordered_CCids[i][UE_id];
        frame_parms[CC_id] = mac_xface->get_lte_frame_parms(Mod_id,CC_id);
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

          if (nb_rbs_required[CC_id][UE_id] > frame_parms[CC_id]->N_RB_DL) {
            TBS = mac_xface->get_TBS_DL(eNB_UE_stats[CC_id]->dlsch_mcs1,frame_parms[CC_id]->N_RB_DL);
            nb_rbs_required[CC_id][UE_id] = frame_parms[CC_id]->N_RB_DL;
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

int maxround(module_id_t Mod_id,uint16_t rnti,int frame,sub_frame_t subframe,uint8_t ul_flag )
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
// it returns -1 if the UE is not found in PHY layer (get_eNB_UE_stats gives NULL)
int maxcqi(module_id_t Mod_id,int32_t UE_id)
{

  LTE_eNB_UE_stats *eNB_UE_stats = NULL;
  UE_list_t *UE_list = &eNB_mac_inst[Mod_id].UE_list;
  int CC_id,n;
  int CQI = 0;

  for (n=0; n<UE_list->numactiveCCs[UE_id]; n++) {
    CC_id = UE_list->ordered_CCids[n][UE_id];
    eNB_UE_stats = mac_xface->get_eNB_UE_stats(Mod_id,CC_id,UE_RNTI(Mod_id,UE_id));

    if (eNB_UE_stats==NULL) {
      /* the UE may have been removed in the PHY layer, don't exit */
      //mac_xface->macphy_exit("maxcqi: could not get eNB_UE_stats\n");
      return -1;
    }

    if (eNB_UE_stats->DL_cqi[0] > CQI) {
      CQI = eNB_UE_stats->DL_cqi[0];
    }
  }

  return(CQI);
}

struct sort_ue_dl_params {
  int Mod_idP;
  int frameP;
  int subframeP;
};

static int ue_dl_compare(const void *_a, const void *_b, void *_params)
{
  struct sort_ue_dl_params *params = _params;
  UE_list_t *UE_list = &eNB_mac_inst[params->Mod_idP].UE_list;

  int UE_id1 = *(const int *)_a;
  int UE_id2 = *(const int *)_b;

  int rnti1  = UE_RNTI(params->Mod_idP, UE_id1);
  int pCC_id1 = UE_PCCID(params->Mod_idP, UE_id1);
  int round1 = maxround(params->Mod_idP, rnti1, params->frameP, params->subframeP, 1);

  int rnti2  = UE_RNTI(params->Mod_idP, UE_id2);
  int pCC_id2 = UE_PCCID(params->Mod_idP, UE_id2);
  int round2 = maxround(params->Mod_idP, rnti2, params->frameP, params->subframeP, 1);

  int cqi1 = maxcqi(params->Mod_idP, UE_id1);
  int cqi2 = maxcqi(params->Mod_idP, UE_id2);

  if (round1 > round2) return -1;
  if (round1 < round2) return 1;

  if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[1] + UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[2] >
      UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[1] + UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[2])
    return -1;
  if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[1] + UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[2] <
      UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[1] + UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[2])
    return 1;

  if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_head_sdu_creation_time_max >
      UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_head_sdu_creation_time_max)
    return -1;
  if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_head_sdu_creation_time_max <
      UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_head_sdu_creation_time_max)
    return 1;

  if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_total >
      UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_total)
    return -1;
  if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_total <
      UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_total)
    return 1;

  if (cqi1 > cqi2) return -1;
  if (cqi1 < cqi2) return 1;

  return 0;
#if 0
  /* The above order derives from the following.  */
      if(round2 > round1) { // Check first if one of the UEs has an active HARQ process which needs service and swap order
        swap_UEs(UE_list,UE_id1,UE_id2,0);
      } else if (round2 == round1) {
        // RK->NN : I guess this is for fairness in the scheduling. This doesn't make sense unless all UEs have the same configuration of logical channels.  This should be done on the sum of all information that has to be sent.  And still it wouldn't ensure fairness.  It should be based on throughput seen by each UE or maybe using the head_sdu_creation_time, i.e. swap UEs if one is waiting longer for service.
        //  for(j=0;j<MAX_NUM_LCID;j++){
        //    if (eNB_mac_inst[Mod_id][pCC_id1].UE_template[UE_id1].dl_buffer_info[j] <
        //      eNB_mac_inst[Mod_id][pCC_id2].UE_template[UE_id2].dl_buffer_info[j]){

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
#endif
}

// This fuction sorts the UE in order their dlsch buffer and CQI
void sort_UEs (module_id_t Mod_idP,
               int         frameP,
               sub_frame_t subframeP)
{
  int               i;
  int               list[NUMBER_OF_UE_MAX];
  int               list_size = 0;
  int               rnti;
  struct sort_ue_dl_params params = { Mod_idP, frameP, subframeP };

  UE_list_t *UE_list = &eNB_mac_inst[Mod_idP].UE_list;

  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    rnti = UE_RNTI(Mod_idP, i);
    if (rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!phy_stats_exist(Mod_idP, rnti))
      continue;
    list[list_size] = i;
    list_size++;
  }

  qsort_r(list, list_size, sizeof(int), ue_dl_compare, &params);

  if (list_size) {
    for (i = 0; i < list_size-1; i++)
      UE_list->next[list[i]] = list[i+1];
    UE_list->next[list[list_size-1]] = -1;
    UE_list->head = list[0];
  } else {
    UE_list->head = -1;
  }

#if 0


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
      if (!phy_stats_exist(Mod_idP, rnti1))
        continue;
      pCC_id1 = UE_PCCID(Mod_idP,UE_id1);
      cqi1    = maxcqi(Mod_idP,UE_id1); //
      round1  = maxround(Mod_idP,rnti1,frameP,subframeP,0);

      UE_id2 = ii;
      rnti2 = UE_RNTI(Mod_idP,UE_id2);
      if(rnti2 == NOT_A_RNTI)
        continue;
      if (UE_list->UE_sched_ctrl[UE_id2].ul_out_of_sync == 1)
	continue;
      if (!phy_stats_exist(Mod_idP, rnti2))
        continue;
      cqi2    = maxcqi(Mod_idP,UE_id2);
      round2  = maxround(Mod_idP,rnti2,frameP,subframeP,0);  //mac_xface->get_ue_active_harq_pid(Mod_id,rnti2,subframe,&harq_pid2,&round2,0);
      pCC_id2 = UE_PCCID(Mod_idP,UE_id2);

      if(round2 > round1) { // Check first if one of the UEs has an active HARQ process which needs service and swap order
        swap_UEs(UE_list,UE_id1,UE_id2,0);
      } else if (round2 == round1) {
        // RK->NN : I guess this is for fairness in the scheduling. This doesn't make sense unless all UEs have the same configuration of logical channels.  This should be done on the sum of all information that has to be sent.  And still it wouldn't ensure fairness.  It should be based on throughput seen by each UE or maybe using the head_sdu_creation_time, i.e. swap UEs if one is waiting longer for service.
        //  for(j=0;j<MAX_NUM_LCID;j++){
        //    if (eNB_mac_inst[Mod_id][pCC_id1].UE_template[UE_id1].dl_buffer_info[j] <
        //      eNB_mac_inst[Mod_id][pCC_id2].UE_template[UE_id2].dl_buffer_info[j]){

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
#endif
}




// This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
void dlsch_scheduler_pre_processor (module_id_t   Mod_id,
                                    frame_t       frameP,
                                    sub_frame_t   subframeP,
                                    int           N_RBG[MAX_NUM_CCs],
                                    int           *mbsfn_flag)
{

  unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],harq_pid=0,round=0,total_ue_count;
  unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX];
  int                     UE_id, i; 
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
  //  int rrc_status           = RRC_IDLE;

#ifdef TM5
  int harq_pid1=0,harq_pid2=0;
  int round1=0,round2=0;
  int UE_id2;
  uint16_t                i1,i2,i3;
  rnti_t             rnti1,rnti2;
  LTE_eNB_UE_stats  *eNB_UE_stats1 = NULL;
  LTE_eNB_UE_stats  *eNB_UE_stats2 = NULL;
  UE_sched_ctrl *ue_sched_ctl1,*ue_sched_ctl2;
#endif

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    if (mbsfn_flag[CC_id]>0)  // If this CC is allocated for MBSFN skip it here
      continue;

    frame_parms[CC_id] = mac_xface->get_lte_frame_parms(Mod_id,CC_id);


    min_rb_unit[CC_id]=get_min_rb_unit(Mod_id,CC_id);

    for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
      if (UE_list->active[i] != TRUE) continue;

      UE_id = i;
      // Initialize scheduling information for all active UEs
      


      dlsch_scheduler_pre_processor_reset(Mod_id,
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
  store_dlsch_buffer (Mod_id,frameP,subframeP);



  // Calculate the number of RBs required by each UE on the basis of logical channel's buffer
  assign_rbs_required (Mod_id,frameP,subframeP,nb_rbs_required,min_rb_unit);



  // Sorts the user on the basis of dlsch logical channel buffer and CQI
  sort_UEs (Mod_id,frameP,subframeP);



  total_ue_count =0;

  // loop over all active UEs
  for (i=UE_list->head; i>=0; i=UE_list->next[i]) {
    rnti = UE_RNTI(Mod_id,i);

    if(rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!phy_stats_exist(Mod_id, rnti))
      continue;
    UE_id = i;

    for (ii=0; ii<UE_num_active_CC(UE_list,UE_id); ii++) {
      CC_id = UE_list->ordered_CCids[ii][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      harq_pid = ue_sched_ctl->harq_pid[CC_id];
      round    = ue_sched_ctl->round[CC_id];

      // if there is no available harq_process, skip the UE
      if (UE_list->UE_sched_ctrl[UE_id].harq_pid[CC_id]<0)
        continue;

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

    if(rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!phy_stats_exist(Mod_id, rnti))
      continue;

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
	  harq_pid = ue_sched_ctl->harq_pid[CC_id];
	  round    = ue_sched_ctl->round[CC_id];

          rnti = UE_RNTI(Mod_id,UE_id);

          // LOG_D(MAC,"UE %d rnti 0x\n", UE_id, rnti );
          if(rnti == NOT_A_RNTI)
            continue;
	  if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
	    continue;
          if (!phy_stats_exist(Mod_id, rnti))
            continue;

          transmission_mode = mac_xface->get_transmission_mode(Mod_id,CC_id,rnti);
	  //          mac_xface->get_ue_active_harq_pid(Mod_id,CC_id,rnti,frameP,subframeP,&harq_pid,&round,0);
          //rrc_status = mac_eNB_get_rrc_status(Mod_id,rnti);
          /* 1st allocate for the retx */

          // retransmission in data channels
          // control channel in the 1st transmission
          // data channel for all TM
          LOG_T(MAC,"calling dlsch_scheduler_pre_processor_allocate .. \n ");
          dlsch_scheduler_pre_processor_allocate (Mod_id,
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

#ifdef TM5

          // data chanel TM5: to be revisted
          if ((round == 0 )  &&
              (transmission_mode == 5)  &&
              (ue_sched_ctl->dl_pow_off[CC_id] != 1)) {

            for(j=0; j<N_RBG[CC_id]; j+=2) {

              if( (((j == (N_RBG[CC_id]-1))&& (rballoc_sub[CC_id][j] == 0) && (ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 0))  ||
                   ((j < (N_RBG[CC_id]-1)) && (rballoc_sub[CC_id][j+1] == 0) && (ue_sched_ctl->rballoc_sub_UE[CC_id][j+1] == 0)) ) &&
                  (nb_rbs_required_remaining[CC_id][UE_id]>0)) {

                for (ii = UE_list->next[i+1]; ii >=0; ii=UE_list->next[ii]) {

                  UE_id2 = ii;
                  rnti2 = UE_RNTI(Mod_id,UE_id2);
		  ue_sched_ctl2 = &UE_list->UE_sched_ctrl[UE_id2];
		  harq_pid2 = ue_sched_ctl2->harq_pid[CC_id];
		  round2    = ue_sched_ctl2->round[CC_id];
                  if(rnti2 == NOT_A_RNTI)
                    continue;
		  if (UE_list->UE_sched_ctrl[UE_id2].ul_out_of_sync == 1)
		    continue;
                  if (!phy_stats_exist(Mod_idP, rnti2))
                    continue;

                  eNB_UE_stats2 = mac_xface->get_eNB_UE_stats(Mod_id,CC_id,rnti2);
                  //mac_xface->get_ue_active_harq_pid(Mod_id,CC_id,rnti2,frameP,subframeP,&harq_pid2,&round2,0);

                  if ((mac_eNB_get_rrc_status(Mod_id,rnti2) >= RRC_RECONFIGURED) &&
                      (round2==0) &&
                      (mac_xface->get_transmission_mode(Mod_id,CC_id,rnti2)==5) &&
                      (ue_sched_ctl->dl_pow_off[CC_id] != 1)) {

                    if( (((j == (N_RBG[CC_id]-1)) && (ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 0)) ||
                         ((j < (N_RBG[CC_id]-1)) && (ue_sched_ctl->rballoc_sub_UE[CC_id][j+1] == 0))  ) &&
                        (nb_rbs_required_remaining[CC_id][UE_id2]>0)) {

                      if((((eNB_UE_stats2->DL_pmi_single^eNB_UE_stats1->DL_pmi_single)<<(14-j))&0xc000)== 0x4000) { //MU-MIMO only for 25 RBs configuration

                        rballoc_sub[CC_id][j] = 1;
                        ue_sched_ctl->rballoc_sub_UE[CC_id][j] = 1;
                        ue_sched_ctl2->rballoc_sub_UE[CC_id][j] = 1;
                        MIMO_mode_indicator[CC_id][j] = 0;

                        if (j< N_RBG[CC_id]-1) {
                          rballoc_sub[CC_id][j+1] = 1;
                          ue_sched_ctl->rballoc_sub_UE[CC_id][j+1] = 1;
                          ue_sched_ctl2->rballoc_sub_UE[CC_id][j+1] = 1;
                          MIMO_mode_indicator[CC_id][j+1] = 0;
                        }

                        ue_sched_ctl->dl_pow_off[CC_id] = 0;
                        ue_sched_ctl2->dl_pow_off[CC_id] = 0;


                        if ((j == N_RBG[CC_id]-1) &&
                            ((PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms.N_RB_DL == 25) ||
                             (PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms.N_RB_DL == 50))) {
			  
                          nb_rbs_required_remaining[CC_id][UE_id] = nb_rbs_required_remaining[CC_id][UE_id] - min_rb_unit[CC_id]+1;
                          ue_sched_ctl->pre_nb_available_rbs[CC_id] = ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit[CC_id]-1;
                          nb_rbs_required_remaining[CC_id][UE_id2] = nb_rbs_required_remaining[CC_id][UE_id2] - min_rb_unit[CC_id]+1;
                          ue_sched_ctl2->pre_nb_available_rbs[CC_id] = ue_sched_ctl2->pre_nb_available_rbs[CC_id] + min_rb_unit[CC_id]-1;
                        } else {
                          
			  nb_rbs_required_remaining[CC_id][UE_id] = nb_rbs_required_remaining[CC_id][UE_id] - 4;
                          ue_sched_ctl->pre_nb_available_rbs[CC_id] = ue_sched_ctl->pre_nb_available_rbs[CC_id] + 4;
                          nb_rbs_required_remaining[CC_id][UE_id2] = nb_rbs_required_remaining[CC_id][UE_id2] - 4;
                          ue_sched_ctl2->pre_nb_available_rbs[CC_id] = ue_sched_ctl2->pre_nb_available_rbs[CC_id] + 4;
                        }

                        break;
                      }
                    }
                  }
                }
              }
            }
          }

#endif
        }
      }
    } // total_ue_count
  } // end of for for r1 and r2

#ifdef TM5

  // This has to be revisited!!!!
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    i1=0;
    i2=0;
    i3=0;

    for (j=0; j<N_RBG[CC_id]; j++) {
      if(MIMO_mode_indicator[CC_id][j] == 2) {
        i1 = i1+1;
      } else if(MIMO_mode_indicator[CC_id][j] == 1) {
        i2 = i2+1;
      } else if(MIMO_mode_indicator[CC_id][j] == 0) {
        i3 = i3+1;
      }
    }

    if((i1 < N_RBG[CC_id]) && (i2>0) && (i3==0)) {
      PHY_vars_eNB_g[Mod_id][CC_id]->check_for_SUMIMO_transmissions = PHY_vars_eNB_g[Mod_id][CC_id]->check_for_SUMIMO_transmissions + 1;
    }

    if(i3 == N_RBG[CC_id] && i1==0 && i2==0) {
      PHY_vars_eNB_g[Mod_id][CC_id]->FULL_MUMIMO_transmissions = PHY_vars_eNB_g[Mod_id][CC_id]->FULL_MUMIMO_transmissions + 1;
    }

    if((i1 < N_RBG[CC_id]) && (i3 > 0)) {
      PHY_vars_eNB_g[Mod_id][CC_id]->check_for_MUMIMO_transmissions = PHY_vars_eNB_g[Mod_id][CC_id]->check_for_MUMIMO_transmissions + 1;
    }

    PHY_vars_eNB_g[Mod_id][CC_id]->check_for_total_transmissions = PHY_vars_eNB_g[Mod_id][CC_id]->check_for_total_transmissions + 1;

  }

#endif

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

void dlsch_scheduler_pre_processor_reset (int module_idP,
					  int UE_id,
					  uint8_t  CC_id,
					  int frameP,
					  int subframeP,					  
					  int N_RBG,
					  uint16_t nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					  uint16_t nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					  unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
					  unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX])
  
{
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
  if (eNB_UE_stats == NULL) return;

  // initialize harq_pid and round

  if (eNB_UE_stats == NULL)
    return;

  mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,
				    frameP,subframeP,
				    &ue_sched_ctl->harq_pid[CC_id],
				    &ue_sched_ctl->round[CC_id],
				    openair_harq_DL);
  if (ue_sched_ctl->ta_timer == 0) {

    // WE SHOULD PROTECT the eNB_UE_stats with a mutex here ...

    ue_sched_ctl->ta_timer = 20;  // wait 20 subframes before taking TA measurement from PHY
    switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
    case 6:
      ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update;
      break;
      
    case 15:
      ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/2;
      break;
      
    case 25:
      ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/4;
      break;
      
    case 50:
      ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/8;
      break;
      
    case 75:
      ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/12;
      break;
      
    case 100:
      if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.threequarter_fs == 0)
	ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/16;
      else
	ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/12;
      break;
    }
    // clear the update in case PHY does not have a new measurement after timer expiry
    eNB_UE_stats->timing_advance_update =  0;
  }
  else {
    ue_sched_ctl->ta_timer--;
    ue_sched_ctl->ta_update =0; // don't trigger a timing advance command
  }
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


void dlsch_scheduler_pre_processor_allocate (module_id_t   Mod_id,
    int           UE_id,
    uint8_t       CC_id,
    int           N_RBG,
    int           transmission_mode,
    int           min_rb_unit,
    uint8_t       N_RB_DL,
    uint16_t      nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
    uint16_t      nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
    unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
    unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX])
{

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


/// ULSCH PRE_PROCESSOR


void ulsch_scheduler_pre_processor(module_id_t module_idP,
                                   int frameP,
                                   sub_frame_t subframeP,
                                   uint16_t *first_rb)
{

  int16_t            i;
  uint16_t           UE_id,n,r;
  uint8_t            CC_id, round, harq_pid;
  uint16_t           nb_allocated_rbs[MAX_NUM_CCs][NUMBER_OF_UE_MAX],total_allocated_rbs[MAX_NUM_CCs],average_rbs_per_user[MAX_NUM_CCs];
  int16_t            total_remaining_rbs[MAX_NUM_CCs];
  uint16_t           max_num_ue_to_be_scheduled=0,total_ue_count=0;
  rnti_t             rnti= -1;
  UE_list_t          *UE_list = &eNB_mac_inst[module_idP].UE_list;
  UE_TEMPLATE        *UE_template = 0;
  LTE_DL_FRAME_PARMS   *frame_parms = 0;


  //LOG_I(MAC,"assign max mcs min rb\n");
  // maximize MCS and then allocate required RB according to the buffer occupancy with the limit of max available UL RB
  assign_max_mcs_min_rb(module_idP,frameP, subframeP, first_rb);

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

      if (total_ue_count == 0) {
        average_rbs_per_user[CC_id] = 0;
      } else if (total_ue_count == 1 ) { // increase the available RBs, special case,
        average_rbs_per_user[CC_id] = frame_parms->N_RB_UL-first_rb[CC_id]+1;
      } else if( (total_ue_count <= (frame_parms->N_RB_DL-first_rb[CC_id])) &&
                 (total_ue_count <= max_num_ue_to_be_scheduled)) {
        average_rbs_per_user[CC_id] = (uint16_t) floor((frame_parms->N_RB_UL-first_rb[CC_id])/total_ue_count);
      } else if (max_num_ue_to_be_scheduled > 0 ) {
        average_rbs_per_user[CC_id] = (uint16_t) floor((frame_parms->N_RB_UL-first_rb[CC_id])/max_num_ue_to_be_scheduled);
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

      mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,openair_harq_UL);

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
        total_remaining_rbs[CC_id]=frame_parms->N_RB_UL - first_rb[CC_id] - total_allocated_rbs[CC_id];

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
      LOG_D(MAC,"[eNB %d] total RB allocated for all UEs = %d/%d\n", module_idP, total_allocated_rbs[CC_id], frame_parms->N_RB_UL - first_rb[CC_id]);
    }
  }
}


void assign_max_mcs_min_rb(module_id_t module_idP,int frameP, sub_frame_t subframeP, uint16_t *first_rb)
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
               (rb_table[rb_table_index]<(frame_parms->N_RB_UL-first_rb[CC_id])) &&
               ((UE_template->phr_info - tx_power) > 0) &&
               (rb_table_index < 32 )) {
          //  LOG_I(MAC,"tbs %d ul buffer %d rb table %d max ul rb %d\n", tbs, UE_template->ul_total_buffer, rb_table[rb_table_index], frame_parms->N_RB_UL-first_rb[CC_id]);
          rb_table_index++;
          tbs = mac_xface->get_TBS_UL(mcs,rb_table[rb_table_index]);
          tx_power = mac_xface->estimate_ue_tx_power(tbs,rb_table[rb_table_index],0,frame_parms->Ncp,0);
        }

        UE_template->ue_tx_power = tx_power;

        if (rb_table[rb_table_index]>(frame_parms->N_RB_UL-first_rb[CC_id]-1)) {
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

struct sort_ue_ul_params {
  int module_idP;
  int frameP;
  int subframeP;
};

static int ue_ul_compare(const void *_a, const void *_b, void *_params)
{
  struct sort_ue_ul_params *params = _params;
  UE_list_t *UE_list = &eNB_mac_inst[params->module_idP].UE_list;

  int UE_id1 = *(const int *)_a;
  int UE_id2 = *(const int *)_b;

  int rnti1  = UE_RNTI(params->module_idP, UE_id1);
  int pCCid1 = UE_PCCID(params->module_idP, UE_id1);
  int round1 = maxround(params->module_idP, rnti1, params->frameP, params->subframeP, 1);

  int rnti2  = UE_RNTI(params->module_idP, UE_id2);
  int pCCid2 = UE_PCCID(params->module_idP, UE_id2);
  int round2 = maxround(params->module_idP, rnti2, params->frameP, params->subframeP, 1);

  if (round1 > round2) return -1;
  if (round1 < round2) return 1;

  if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] > UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0])
    return -1;
  if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] < UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0])
    return 1;

  if (UE_list->UE_template[pCCid1][UE_id1].ul_total_buffer > UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer)
    return -1;
  if (UE_list->UE_template[pCCid1][UE_id1].ul_total_buffer < UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer)
    return 1;

  if (UE_list->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul > UE_list->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
    return -1;
  if (UE_list->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul < UE_list->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
    return 1;

  return 0;

#if 0
  /* The above order derives from the following.
   * The last case is not handled: "if (UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer > 0 )"
   * I don't think it makes a big difference.
   */
      if(round2 > round1) {
        swap_UEs(UE_list,UE_id1,UE_id2,1);
      } else if (round2 == round1) {
        if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] < UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0]) {
          swap_UEs(UE_list,UE_id1,UE_id2,1);
        } else if (UE_list->UE_template[pCCid1][UE_id1].ul_total_buffer <  UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer) {
          swap_UEs(UE_list,UE_id1,UE_id2,1);
        } else if (UE_list->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul <  UE_list->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul) {
          if (UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer > 0 ) {
            swap_UEs(UE_list,UE_id1,UE_id2,1);
          }
        }
      }
#endif
}

void sort_ue_ul (module_id_t module_idP,int frameP, sub_frame_t subframeP)
{
  int               i;
  int               list[NUMBER_OF_UE_MAX];
  int               list_size = 0;
  int               rnti;
  struct sort_ue_ul_params params = { module_idP, frameP, subframeP };

  UE_list_t *UE_list = &eNB_mac_inst[module_idP].UE_list;

  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    rnti = UE_RNTI(module_idP, i);
    if (rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!phy_stats_exist(module_idP, rnti))
      continue;
    list[list_size] = i;
    list_size++;
  }

  qsort_r(list, list_size, sizeof(int), ue_ul_compare, &params);

  if (list_size) {
    for (i = 0; i < list_size-1; i++)
      UE_list->next_ul[list[i]] = list[i+1];
    UE_list->next_ul[list[list_size-1]] = -1;
    UE_list->head_ul = list[0];
  } else {
    UE_list->head_ul = -1;
  }

#if 0
  int               UE_id1,UE_id2;
  int               pCCid1,pCCid2;
  int               round1,round2;
  int               i=0,ii=0;
  rnti_t            rnti1,rnti2;

  UE_list_t *UE_list = &eNB_mac_inst[module_idP].UE_list;

  for (i=UE_list->head_ul; i>=0; i=UE_list->next_ul[i]) {

    //LOG_I(MAC,"sort ue ul i %d\n",i);
    for (ii=UE_list->next_ul[i]; ii>=0; ii=UE_list->next_ul[ii]) {
      //LOG_I(MAC,"sort ul ue 2 ii %d\n",ii);
 
      UE_id1  = i;
      rnti1 = UE_RNTI(module_idP,UE_id1);
      
      if(rnti1 == NOT_A_RNTI)
	continue;
      if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
	continue;
      if (!phy_stats_exist(module_idP, rnti1))
        continue;

      pCCid1 = UE_PCCID(module_idP,UE_id1);
      round1  = maxround(module_idP,rnti1,frameP,subframeP,1);
      
      UE_id2  = ii;
      rnti2 = UE_RNTI(module_idP,UE_id2);
      
      if(rnti2 == NOT_A_RNTI)
        continue;
      if (UE_list->UE_sched_ctrl[UE_id2].ul_out_of_sync == 1)
	continue;
      if (!phy_stats_exist(module_idP, rnti2))
        continue;

      pCCid2 = UE_PCCID(module_idP,UE_id2);
      round2  = maxround(module_idP,rnti2,frameP,subframeP,1);

      if(round2 > round1) {
        swap_UEs(UE_list,UE_id1,UE_id2,1);
      } else if (round2 == round1) {
        if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] < UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0]) {
          swap_UEs(UE_list,UE_id1,UE_id2,1);
        } else if (UE_list->UE_template[pCCid1][UE_id1].ul_total_buffer <  UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer) {
          swap_UEs(UE_list,UE_id1,UE_id2,1);
        } else if (UE_list->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul <  UE_list->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul) {
          if (UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer > 0 ) {
            swap_UEs(UE_list,UE_id1,UE_id2,1);
          }
        }
      }
    }
  }
#endif
}
