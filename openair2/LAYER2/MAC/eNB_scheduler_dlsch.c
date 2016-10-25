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

#include "SIMULATION/TOOLS/defs.h" // for taus

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "T.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
//#define DEBUG_eNB_SCHEDULER 1



//------------------------------------------------------------------------------
void
add_ue_dlsch_info(
  module_id_t module_idP,
  int CC_id,
  int UE_id,
  sub_frame_t subframeP,
  UE_DLSCH_STATUS status
)
//------------------------------------------------------------------------------
{

  eNB_dlsch_info[module_idP][CC_id][UE_id].rnti             = UE_RNTI(module_idP,UE_id);
  //  eNB_dlsch_info[module_idP][CC_id][ue_mod_idP].weight           = weight;
  eNB_dlsch_info[module_idP][CC_id][UE_id].subframe         = subframeP;
  eNB_dlsch_info[module_idP][CC_id][UE_id].status           = status;

  eNB_dlsch_info[module_idP][CC_id][UE_id].serving_num++;

}

//------------------------------------------------------------------------------
int
schedule_next_dlue(
  module_id_t module_idP,
  int CC_id,
  sub_frame_t subframeP
)
//------------------------------------------------------------------------------
{

  int next_ue;
  UE_list_t *UE_list=&eNB_mac_inst[module_idP].UE_list;

  for (next_ue=UE_list->head; next_ue>=0; next_ue=UE_list->next[next_ue] ) {
    if  (eNB_dlsch_info[module_idP][CC_id][next_ue].status == S_DL_WAITING) {
      return next_ue;
    }
  }

  for (next_ue=UE_list->head; next_ue>=0; next_ue=UE_list->next[next_ue] ) {
    if  (eNB_dlsch_info[module_idP][CC_id][next_ue].status == S_DL_BUFFERED) {
      eNB_dlsch_info[module_idP][CC_id][next_ue].status = S_DL_WAITING;
    }
  }

  return(-1);//next_ue;

}

//------------------------------------------------------------------------------
unsigned char
generate_dlsch_header(
  unsigned char* mac_header,
  unsigned char num_sdus,
  unsigned short *sdu_lengths,
  unsigned char *sdu_lcids,
  unsigned char drx_cmd,
  short timing_advance_cmd,
  unsigned char *ue_cont_res_id,
  unsigned char short_padding,
  unsigned short post_padding
)
//------------------------------------------------------------------------------
{

  SCH_SUBHEADER_FIXED *mac_header_ptr = (SCH_SUBHEADER_FIXED *)mac_header;
  uint8_t first_element=0,last_size=0,i;
  uint8_t mac_header_control_elements[16],*ce_ptr;

  ce_ptr = &mac_header_control_elements[0];

  // compute header components

  if ((short_padding == 1) || (short_padding == 2)) {
    mac_header_ptr->R    = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    first_element=1;
    last_size=1;
  }

  if (short_padding == 2) {
    mac_header_ptr->E = 1;
    mac_header_ptr++;
    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    last_size=1;
  }

  if (drx_cmd != 255) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element=1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = DRX_CMD;
    last_size=1;
  }

  if (timing_advance_cmd != 0) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element=1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = TIMING_ADV_CMD;
    last_size=1;
    //    msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);
    ((TIMING_ADVANCE_CMD *)ce_ptr)->R=0;
    ((TIMING_ADVANCE_CMD *)ce_ptr)->TA=(timing_advance_cmd+31)&0x3f;
    LOG_D(MAC,"timing advance =%d (%d)\n",timing_advance_cmd,((TIMING_ADVANCE_CMD *)ce_ptr)->TA);
    ce_ptr+=sizeof(TIMING_ADVANCE_CMD);
    //msg("offset %d\n",ce_ptr-mac_header_control_elements);
  }

  if (ue_cont_res_id) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      /*
      printf("[eNB][MAC] last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
      */
      mac_header_ptr++;
    } else {
      first_element=1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = UE_CONT_RES;
    last_size=1;

    LOG_T(MAC,"[eNB ][RAPROC] Generate contention resolution msg: %x.%x.%x.%x.%x.%x\n",
          ue_cont_res_id[0],
          ue_cont_res_id[1],
          ue_cont_res_id[2],
          ue_cont_res_id[3],
          ue_cont_res_id[4],
          ue_cont_res_id[5]);

    memcpy(ce_ptr,ue_cont_res_id,6);
    ce_ptr+=6;
    // msg("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  }

  //msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

  for (i=0; i<num_sdus; i++) {
    LOG_T(MAC,"[eNB] Generate DLSCH header num sdu %d len sdu %d\n",num_sdus, sdu_lengths[i]);

    if (first_element>0) {
      mac_header_ptr->E = 1;
      /*msg("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
      */
      mac_header_ptr+=last_size;
      //msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);
    } else {
      first_element=1;
    }

    if (sdu_lengths[i] < 128) {
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->R    = 0;
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->E    = 0;
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F    = 0;
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->LCID = sdu_lcids[i];
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L    = (unsigned char)sdu_lengths[i];
      last_size=2;
    } else {
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->R    = 0;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->E    = 0;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->F    = 1;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->LCID = sdu_lcids[i];
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_MSB    = ((unsigned short) sdu_lengths[i]>>8)&0x7f;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_LSB    = (unsigned short) sdu_lengths[i]&0xff;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->padding   = 0x00;
      last_size=3;
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[eNB] generate long sdu, size %x (MSB %x, LSB %x)\n",
            sdu_lengths[i],
            ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_MSB,
            ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_LSB);
#endif
    }
  }

  /*

    printf("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

    printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
    ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
    ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
    ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);


    if (((SCH_SUBHEADER_FIXED*)mac_header_ptr)->LCID < UE_CONT_RES) {
    if (((SCH_SUBHEADER_SHORT*)mac_header_ptr)->F == 0)
    printf("F = 0, sdu len (L field) %d\n",(((SCH_SUBHEADER_SHORT*)mac_header_ptr)->L));
    else
    printf("F = 1, sdu len (L field) %d\n",(((SCH_SUBHEADER_LONG*)mac_header_ptr)->L));
    }
  */
  if (post_padding>0) {// we have lots of padding at the end of the packet
    mac_header_ptr->E = 1;
    mac_header_ptr+=last_size;
    // add a padding element
    mac_header_ptr->R    = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    mac_header_ptr++;
  } else { // no end of packet padding
    // last SDU subhead is of fixed type (sdu length implicitly to be computed at UE)
    mac_header_ptr++;
  }

  //msg("After subheaders %d\n",(uint8_t*)mac_header_ptr - mac_header);

  if ((ce_ptr-mac_header_control_elements) > 0) {
    // printf("Copying %d bytes for control elements\n",ce_ptr-mac_header_control_elements);
    memcpy((void*)mac_header_ptr,mac_header_control_elements,ce_ptr-mac_header_control_elements);
    mac_header_ptr+=(unsigned char)(ce_ptr-mac_header_control_elements);
  }

  //msg("After CEs %d\n",(uint8_t*)mac_header_ptr - mac_header);

  return((unsigned char*)mac_header_ptr - mac_header);

}

//------------------------------------------------------------------------------
void
set_ul_DAI(
  int module_idP,
  int UE_idP,
  int CC_idP,
  int frameP,
  int subframeP,
  LTE_DL_FRAME_PARMS*  frame_parms[MAX_NUM_CCs]
)
//------------------------------------------------------------------------------
{

  eNB_MAC_INST         *eNB      = &eNB_mac_inst[module_idP];
  UE_list_t            *UE_list  = &eNB->UE_list;
  unsigned char         DAI;

  if (frame_parms[CC_idP]->frame_type == TDD) {
    DAI = (UE_list->UE_template[CC_idP][UE_idP].DAI-1)&3;
    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframe %d: DAI %d for UE %d\n",module_idP,CC_idP,frameP,subframeP,DAI,UE_idP);
    // Save DAI for Format 0 DCI

    switch (frame_parms[CC_idP]->tdd_config) {
    case 0:
      //      if ((subframeP==0)||(subframeP==1)||(subframeP==5)||(subframeP==6))
      break;

    case 1:
      switch (subframeP) {
      case 1:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[7] = DAI;
        break;

      case 4:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[8] = DAI;
        break;

      case 6:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[2] = DAI;
        break;

      case 9:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[3] = DAI;
        break;
      }

    case 2:
      //      if ((subframeP==3)||(subframeP==8))
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    case 3:

      //if ((subframeP==6)||(subframeP==8)||(subframeP==0)) {
      //  LOG_D(MAC,"schedule_ue_spec: setting UL DAI to %d for subframeP %d => %d\n",DAI,subframeP, ((subframeP+8)%10)>>1);
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul[((subframeP+8)%10)>>1] = DAI;
      //}
      switch (subframeP) {
      case 5:
      case 6:
      case 1:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[2] = DAI;
        break;

      case 7:
      case 8:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[3] = DAI;
        break;

      case 9:
      case 0:
        UE_list->UE_template[CC_idP][UE_idP].DAI_ul[4] = DAI;
        break;

      default:
        break;
      }

      break;

    case 4:
      //      if ((subframeP==8)||(subframeP==9))
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    case 5:
      //      if (subframeP==8)
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    case 6:
      //      if ((subframeP==1)||(subframeP==4)||(subframeP==6)||(subframeP==9))
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    default:
      break;
    }
  }
}


//------------------------------------------------------------------------------
void
schedule_ue_spec(
  module_id_t   module_idP,
  frame_t       frameP,
  sub_frame_t   subframeP,
  int*          mbsfn_flag
)
//------------------------------------------------------------------------------
{

  uint8_t               CC_id;
  int                   UE_id;
  int                   N_RBG[MAX_NUM_CCs];
  unsigned char         aggregation;
  mac_rlc_status_resp_t rlc_status;
  unsigned char         header_len_dcch=0, header_len_dcch_tmp=0; 
  unsigned char         header_len_dtch=0, header_len_dtch_tmp=0, header_len_dtch_last=0; 
  unsigned char         ta_len=0;
  unsigned char         sdu_lcids[NB_RB_MAX],lcid,offset,num_sdus=0;
  uint16_t              nb_rb,nb_rb_temp,total_nb_available_rb[MAX_NUM_CCs],nb_available_rb;
  uint16_t              TBS,j,sdu_lengths[NB_RB_MAX],rnti,padding=0,post_padding=0;
  unsigned char         dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  unsigned char         round            = 0;
  unsigned char         harq_pid         = 0;
  void                 *DLSCH_dci        = NULL;
  LTE_eNB_UE_stats     *eNB_UE_stats     = NULL;
  uint16_t              sdu_length_total = 0;
  //  uint8_t               dl_pow_off[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  //  unsigned char         rballoc_sub_UE[MAX_NUM_CCs][NUMBER_OF_UE_MAX][N_RBG_MAX];
  //  uint16_t              pre_nb_available_rbs[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  int                   mcs;
  uint16_t              min_rb_unit[MAX_NUM_CCs];
  eNB_MAC_INST         *eNB      = &eNB_mac_inst[module_idP];
  UE_list_t            *UE_list  = &eNB->UE_list;
  LTE_DL_FRAME_PARMS   *frame_parms[MAX_NUM_CCs];
  int                   continue_flag=0;
  int32_t                 normalized_rx_power, target_rx_power;
  int32_t                 tpc=1;
  static int32_t          tpc_accumulated=0;
  UE_sched_ctrl           *ue_sched_ctl;
  int i;

  if (UE_list->head==-1) {
    return;
  }

  start_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,VCD_FUNCTION_IN);

  //weight = get_ue_weight(module_idP,UE_id);
  aggregation = 1; // set to the maximum aggregation level

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    min_rb_unit[CC_id]=get_min_rb_unit(module_idP,CC_id);
    frame_parms[CC_id] = mac_xface->get_lte_frame_parms(module_idP,CC_id);
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

  /// CALLING Pre_Processor for downlink scheduling (Returns estimation of RBs required by each UE and the allocation on sub-band)

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_IN);
  start_meas(&eNB->schedule_dlsch_preprocessor);
  dlsch_scheduler_pre_processor(module_idP,
                                frameP,
                                subframeP,
                                N_RBG,
                                mbsfn_flag);
  stop_meas(&eNB->schedule_dlsch_preprocessor);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_OUT);


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d\n",CC_id);

    if (mbsfn_flag[CC_id]>0)
      continue;

    for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
      continue_flag=0; // reset the flag to allow allocation for the remaining UEs
      rnti = UE_RNTI(module_idP,UE_id);
      eNB_UE_stats = mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti);
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

      if (rnti==NOT_A_RNTI) {
        LOG_D(MAC,"Cannot find rnti for UE_id %d (num_UEs %d)\n",UE_id,UE_list->num_UEs);
        // mac_xface->macphy_exit("Cannot find rnti for UE_id");
        continue_flag=1;
      }

      if (eNB_UE_stats==NULL) {
        LOG_D(MAC,"[eNB] Cannot find eNB_UE_stats\n");
        //  mac_xface->macphy_exit("[MAC][eNB] Cannot find eNB_UE_stats\n");
        continue_flag=1;
      }

      if ((ue_sched_ctl->pre_nb_available_rbs[CC_id] == 0) ||  // no RBs allocated 
	  CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,aggregation,rnti)
	  ) {
        LOG_D(MAC,"[eNB %d] Frame %d : no RB allocated for UE %d on CC_id %d: continue \n",
              module_idP, frameP, UE_id, CC_id);
        //if(mac_xface->get_transmission_mode(module_idP,rnti)==5)
        continue_flag=1; //to next user (there might be rbs availiable for other UEs in TM5
        // else
        //  break;
      }

      if (frame_parms[CC_id]->frame_type == TDD)  {
        set_ue_dai (subframeP,
                    frame_parms[CC_id]->tdd_config,
                    UE_id,
                    CC_id,
                    UE_list);
        // update UL DAI after DLSCH scheduling
        set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP,frame_parms);
      }

      if (continue_flag == 1 ) {
        add_ue_dlsch_info(module_idP,
                          CC_id,
                          UE_id,
                          subframeP,
                          S_DL_NONE);
        continue;
      }

      nb_available_rb = ue_sched_ctl->pre_nb_available_rbs[CC_id];
      harq_pid = ue_sched_ctl->harq_pid[CC_id];
      round = ue_sched_ctl->round[CC_id];
      UE_list->eNB_UE_stats[CC_id][UE_id].crnti= rnti;
      UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status=mac_eNB_get_rrc_status(module_idP,rnti);
      UE_list->eNB_UE_stats[CC_id][UE_id].harq_pid = harq_pid; 
      UE_list->eNB_UE_stats[CC_id][UE_id].harq_round = round;

      sdu_length_total=0;
      num_sdus=0;

      /*
      DevCheck(((eNB_UE_stats->DL_cqi[0] < MIN_CQI_VALUE) || (eNB_UE_stats->DL_cqi[0] > MAX_CQI_VALUE)),
      eNB_UE_stats->DL_cqi[0], MIN_CQI_VALUE, MAX_CQI_VALUE);
      */
      eNB_UE_stats->dlsch_mcs1 = cqi_to_mcs[eNB_UE_stats->DL_cqi[0]];
      eNB_UE_stats->dlsch_mcs1 = eNB_UE_stats->dlsch_mcs1;//cmin(eNB_UE_stats->dlsch_mcs1, openair_daq_vars.target_ue_dl_mcs);


      // store stats
      UE_list->eNB_UE_stats[CC_id][UE_id].dl_cqi= eNB_UE_stats->DL_cqi[0];

      // initializing the rb allocation indicator for each UE
      for(j=0; j<frame_parms[CC_id]->N_RBG; j++) {
        UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = 0;
      }

      LOG_D(MAC,"[eNB %d] Frame %d: Scheduling UE %d on CC_id %d (rnti %x, harq_pid %d, round %d, rb %d, cqi %d, mcs %d, rrc %d)\n",
            module_idP, frameP, UE_id,CC_id,rnti,harq_pid, round,nb_available_rb,
            eNB_UE_stats->DL_cqi[0], eNB_UE_stats->dlsch_mcs1,
	    UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status);


      // Note this code is for a specific DCI format
      DLSCH_dci = (void *)UE_list->UE_template[CC_id][UE_id].DLSCH_DCI[harq_pid];


      /* process retransmission  */

      if (round > 0) {

        if (frame_parms[CC_id]->frame_type == TDD) {
          UE_list->UE_template[CC_id][UE_id].DAI++;
          update_ul_dci(module_idP,CC_id,rnti,UE_list->UE_template[CC_id][UE_id].DAI);
          LOG_D(MAC,"DAI update: CC_id %d subframeP %d: UE %d, DAI %d\n",
                CC_id,subframeP,UE_id,UE_list->UE_template[CC_id][UE_id].DAI);
        }

        // get freq_allocation
        nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];

        if (nb_rb <= nb_available_rb) {

          if(nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
            for(j=0; j<frame_parms[CC_id]->N_RBG; j++) { // for indicating the rballoc for each sub-band
              UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
          } else {
            nb_rb_temp = nb_rb;
            j = 0;

            while((nb_rb_temp > 0) && (j<frame_parms[CC_id]->N_RBG)) {
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

              j = j+1;
            }
          }

          nb_available_rb -= nb_rb;
          aggregation = process_ue_cqi(module_idP,UE_id);


          PHY_vars_eNB_g[module_idP][CC_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
          PHY_vars_eNB_g[module_idP][CC_id]->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];

          for(j=0; j<frame_parms[CC_id]->N_RBG; j++) {
            PHY_vars_eNB_g[module_idP][CC_id]->mu_mimo_mode[UE_id].rballoc_sub[j] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];
          }

          switch (mac_xface->get_transmission_mode(module_idP,CC_id,rnti)) {
          case 1:
          case 2:
          default:
            switch (frame_parms[CC_id]->N_RB_DL) {
            case 6:
              if (frame_parms[CC_id]->frame_type == TDD) {
                //        ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->rv       = round&3;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,(UE_list->UE_template[CC_id][UE_id].DAI-1),
                      ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->mcs);
              } else {
                //        ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->rv       = round&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->mcs);

              }

              break;

            case 25:
              if (frame_parms[CC_id]->frame_type == TDD) {
                //        ((DCI1_5MHz_TDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rv       = round&3;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,(UE_list->UE_template[CC_id][UE_id].DAI-1),
                      ((DCI1_5MHz_TDD_t*)DLSCH_dci)->mcs);
              } else {
                //        ((DCI1_5MHz_FDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rv       = round&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,((DCI1_5MHz_FDD_t*)DLSCH_dci)->mcs);

              }

              break;

            case 50:
              if (frame_parms[CC_id]->frame_type == TDD) {
                //        ((DCI1_10MHz_TDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->rv       = round&3;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,(UE_list->UE_template[CC_id][UE_id].DAI-1),
                      ((DCI1_10MHz_TDD_t*)DLSCH_dci)->mcs);
              } else {
                //        ((DCI1_10MHz_FDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->rv       = round&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,((DCI1_10MHz_FDD_t*)DLSCH_dci)->mcs);

              }

              break;

            case 100:
              if (frame_parms[CC_id]->frame_type == TDD) {
                //        ((DCI1_20MHz_TDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->rv       = round&3;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,(UE_list->UE_template[CC_id][UE_id].DAI-1),
                      ((DCI1_20MHz_TDD_t*)DLSCH_dci)->mcs);
              } else {
                //        ((DCI1_20MHz_FDD_t*)DLSCH_dci)->ndi      = 0;
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->rv       = round&3;
                LOG_D(MAC,"[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
                      module_idP,CC_id,harq_pid,round,((DCI1_20MHz_FDD_t*)DLSCH_dci)->mcs);

              }

              break;
            }

            break;

          case 4:
            //    if (nb_rb>10) {
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 0;
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
            // }
            //else {
            //  ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->ndi1 = 0;
            // ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            // ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->rv1 = round&3;
            // ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
            // }
            break;

          case 5:
            // if(nb_rb>10){
            //((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->mcs = eNB_UE_stats->DL_cqi[0]<<1;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->ndi = 0;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rv = round&3;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;

            if(ue_sched_ctl->dl_pow_off[CC_id] == 2) {
              ue_sched_ctl->dl_pow_off[CC_id] = 1;
            }

            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dl_power_off = ue_sched_ctl->dl_pow_off[CC_id];
            // }
            break;

          case 6:
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->ndi = 0;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rv = round&3;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dl_power_off = 1;//dl_pow_off[UE_id];
            break;
          }

          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_SCHEDULED);

          //eNB_UE_stats->dlsch_trials[round]++;
          UE_list->eNB_UE_stats[CC_id][UE_id].num_retransmission+=1;
          UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx=nb_rb;
          UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_retx+=nb_rb;
          UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1=eNB_UE_stats->dlsch_mcs1;
          UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2=eNB_UE_stats->dlsch_mcs1;
        } else {
          LOG_D(MAC,"[eNB %d] Frame %d CC_id %d : don't schedule UE %d, its retransmission takes more resources than we have\n",
                module_idP, frameP, CC_id, UE_id);
        }
      } else { /* This is a potentially new SDU opportunity */

        rlc_status.bytes_in_buffer = 0;
        // Now check RLC information to compute number of required RBs
        // get maximum TBS size for RLC request
        //TBS = mac_xface->get_TBS(eNB_UE_stats->DL_cqi[0]<<1,nb_available_rb);
        TBS = mac_xface->get_TBS_DL(eNB_UE_stats->dlsch_mcs1,nb_available_rb);
        // check first for RLC data on DCCH
        // add the length for  all the control elements (timing adv, drx, etc) : header + payload

        ta_len = (ue_sched_ctl->ta_update!=0) ? 2 : 0;

        header_len_dcch = 2; // 2 bytes DCCH SDU subheader

        if ( TBS-ta_len-header_len_dcch > 0 ) {
          rlc_status = mac_rlc_status_ind(
                         module_idP,
                         rnti,
			 module_idP,
                         frameP,
                         ENB_FLAG_YES,
                         MBMS_FLAG_NO,
                         DCCH,
                         (TBS-ta_len-header_len_dcch)); // transport block set size

          sdu_lengths[0]=0;

          if (rlc_status.bytes_in_buffer > 0) {  // There is DCCH to transmit
            LOG_D(MAC,"[eNB %d] Frame %d, DL-DCCH->DLSCH CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
                  module_idP,frameP,CC_id,TBS-header_len_dcch);
            sdu_lengths[0] += mac_rlc_data_req(
                                module_idP,
                                rnti,
				module_idP,
                                frameP,
                                ENB_FLAG_YES,
                                MBMS_FLAG_NO,
                                DCCH,
                                (char *)&dlsch_buffer[sdu_lengths[0]]);

            T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP), T_INT(subframeP),
              T_INT(harq_pid), T_INT(DCCH), T_INT(sdu_lengths[0]));

            LOG_D(MAC,"[eNB %d][DCCH] CC_id %d Got %d bytes from RLC\n",module_idP,CC_id,sdu_lengths[0]);
            sdu_length_total = sdu_lengths[0];
            sdu_lcids[0] = DCCH;
            UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH]+=1;
            UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH]+=sdu_lengths[0];
            num_sdus = 1;
#ifdef DEBUG_eNB_SCHEDULER
            LOG_T(MAC,"[eNB %d][DCCH] CC_id %d Got %d bytes :",module_idP,CC_id,sdu_lengths[0]);

            for (j=0; j<sdu_lengths[0]; j++) {
              LOG_T(MAC,"%x ",dlsch_buffer[j]);
            }

            LOG_T(MAC,"\n");
#endif
          } else {
            header_len_dcch = 0;
            sdu_length_total = 0;
          }
        }

        // check for DCCH1 and update header information (assume 2 byte sub-header)
        if (TBS-ta_len-header_len_dcch-sdu_length_total > 0 ) {
          rlc_status = mac_rlc_status_ind(
                         module_idP,
                         rnti,
			 module_idP,
                         frameP,
                         ENB_FLAG_YES,
                         MBMS_FLAG_NO,
                         DCCH+1,
                         (TBS-ta_len-header_len_dcch-sdu_length_total)); // transport block set size less allocations for timing advance and
          // DCCH SDU

          if (rlc_status.bytes_in_buffer > 0) {
            LOG_D(MAC,"[eNB %d], Frame %d, DCCH1->DLSCH, CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
                  module_idP,frameP,CC_id,TBS-header_len_dcch-sdu_length_total);
            sdu_lengths[num_sdus] += mac_rlc_data_req(
                                       module_idP,
                                       rnti,
				       module_idP,
                                       frameP,
                                       ENB_FLAG_YES,
                                       MBMS_FLAG_NO,
                                       DCCH+1,
                                       (char *)&dlsch_buffer[sdu_lengths[num_sdus]]);

            T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP), T_INT(subframeP),
              T_INT(harq_pid), T_INT(DCCH+1), T_INT(sdu_lengths[num_sdus]));

            sdu_lcids[num_sdus] = DCCH1;
            sdu_length_total += sdu_lengths[num_sdus];
            header_len_dcch += 2;
            UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH1]+=1;
            UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH1]+=sdu_lengths[num_sdus];
	    num_sdus++;
#ifdef DEBUG_eNB_SCHEDULER
            LOG_T(MAC,"[eNB %d][DCCH1] CC_id %d Got %d bytes :",module_idP,CC_id,sdu_lengths[num_sdus]);

            for (j=0; j<sdu_lengths[num_sdus]; j++) {
              LOG_T(MAC,"%x ",dlsch_buffer[j]);
            }

            LOG_T(MAC,"\n");
#endif

	  }
        }

	// assume the max dtch header size, and adjust it later
	header_len_dtch=0;
	header_len_dtch_last=0; // the header length of the last mac sdu
	// lcid has to be sorted before the actual allocation (similar struct as ue_list).
	for (lcid=NB_RB_MAX-1; lcid>=DTCH ; lcid--){
	  // TBD: check if the lcid is active
	  
	  header_len_dtch+=3; 
	  header_len_dtch_last=3;
	  LOG_D(MAC,"[eNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (tbs %d, len %d)\n",
		module_idP,frameP,lcid,TBS,
		TBS-ta_len-header_len_dcch-sdu_length_total-header_len_dtch);
	  
	  if (TBS-ta_len-header_len_dcch-sdu_length_total-header_len_dtch > 0 ) { // NN: > 2 ? 
	    rlc_status = mac_rlc_status_ind(module_idP,
					    rnti,
					    module_idP,
					    frameP,
					    ENB_FLAG_YES,
					    MBMS_FLAG_NO,
					    lcid,
					    TBS-ta_len-header_len_dcch-sdu_length_total-header_len_dtch);
	   

	    if (rlc_status.bytes_in_buffer > 0) {
	      
	      LOG_D(MAC,"[eNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d)\n",
		    module_idP,frameP,TBS-header_len_dcch-sdu_length_total-header_len_dtch,lcid, header_len_dtch);
	      sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
						       rnti,
						       module_idP,
						       frameP,
						       ENB_FLAG_YES,
						       MBMS_FLAG_NO,
						       lcid,
						       (char*)&dlsch_buffer[sdu_length_total]);
	      T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP), T_INT(subframeP),
              T_INT(harq_pid), T_INT(lcid), T_INT(sdu_lengths[num_sdus]));

	      LOG_D(MAC,"[eNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n",module_idP,sdu_lengths[num_sdus],lcid);
	      sdu_lcids[num_sdus] = lcid;
	      sdu_length_total += sdu_lengths[num_sdus];
	      UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[lcid]+=1;
	      UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[lcid]+=sdu_lengths[num_sdus];
	      if (sdu_lengths[num_sdus] < 128) {
		header_len_dtch--;
		header_len_dtch_last--;
	      }
	      num_sdus++;
	    } // no data for this LCID
	    else {
	      header_len_dtch-=3;
	    }
	  } // no TBS left
	  else {
	    header_len_dtch-=3;
	    break; 
	  }
	}
	if (header_len_dtch == 0 )
	  header_len_dtch_last= 0;
	// there is at least one SDU 
	// if (num_sdus > 0 ){
	if ((sdu_length_total + header_len_dcch + header_len_dtch )> 0) {
	  
	  // Now compute number of required RBs for total sdu length
	  // Assume RAH format 2
	  // adjust  header lengths
	  header_len_dcch_tmp = header_len_dcch;
	  header_len_dtch_tmp = header_len_dtch;
	  if (header_len_dtch==0) {
	    header_len_dcch = (header_len_dcch >0) ? 1 : 0;//header_len_dcch;  // remove length field
	  } else {
	    header_len_dtch_last-=1; // now use it to find how many bytes has to be removed for the last MAC SDU 
	    header_len_dtch = (header_len_dtch > 0) ? header_len_dtch - header_len_dtch_last  :header_len_dtch;     // remove length field for the last SDU
	  }

          mcs = eNB_UE_stats->dlsch_mcs1;

          if (mcs==0) {
            nb_rb = 4;  // don't let the TBS get too small
          } else {
            nb_rb=min_rb_unit[CC_id];
          }

          TBS = mac_xface->get_TBS_DL(mcs,nb_rb);

          while (TBS < (sdu_length_total + header_len_dcch + header_len_dtch + ta_len))  {
            nb_rb += min_rb_unit[CC_id];  //

            if (nb_rb>nb_available_rb) { // if we've gone beyond the maximum number of RBs
              // (can happen if N_RB_DL is odd)
              TBS = mac_xface->get_TBS_DL(eNB_UE_stats->dlsch_mcs1,nb_available_rb);
              nb_rb = nb_available_rb;
              break;
            }

            TBS = mac_xface->get_TBS_DL(eNB_UE_stats->dlsch_mcs1,nb_rb);
          }

          if(nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
            for(j=0; j<frame_parms[CC_id]->N_RBG; j++) { // for indicating the rballoc for each sub-band
              UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
          } else {
            nb_rb_temp = nb_rb;
            j = 0;

            while((nb_rb_temp > 0) && (j<frame_parms[CC_id]->N_RBG)) {
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

          PHY_vars_eNB_g[module_idP][CC_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
          PHY_vars_eNB_g[module_idP][CC_id]->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];

          for(j=0; j<frame_parms[CC_id]->N_RBG; j++) {
            PHY_vars_eNB_g[module_idP][CC_id]->mu_mimo_mode[UE_id].rballoc_sub[j] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];

          }

          // decrease mcs until TBS falls below required length
          while ((TBS > (sdu_length_total + header_len_dcch + header_len_dtch + ta_len)) && (mcs>0)) {
            mcs--;
            TBS = mac_xface->get_TBS_DL(mcs,nb_rb);
          }

          // if we have decreased too much or we don't have enough RBs, increase MCS
          while ((TBS < (sdu_length_total + header_len_dcch + header_len_dtch + ta_len)) && ((( ue_sched_ctl->dl_pow_off[CC_id]>0) && (mcs<28))
                 || ( (ue_sched_ctl->dl_pow_off[CC_id]==0) && (mcs<=15)))) {
            mcs++;
            TBS = mac_xface->get_TBS_DL(mcs,nb_rb);
          }

          LOG_D(MAC,"dlsch_mcs before and after the rate matching = (%d, %d)\n",eNB_UE_stats->dlsch_mcs1, mcs);

#ifdef DEBUG_eNB_SCHEDULER
          LOG_D(MAC,"[eNB %d] CC_id %d Generated DLSCH header (mcs %d, TBS %d, nb_rb %d)\n",
                module_idP,CC_id,mcs,TBS,nb_rb);
          // msg("[MAC][eNB ] Reminder of DLSCH with random data %d %d %d %d \n",
          //  TBS, sdu_length_total, offset, TBS-sdu_length_total-offset);
#endif

          if ((TBS - header_len_dcch - header_len_dtch - sdu_length_total - ta_len) <= 2) {
            padding = (TBS - header_len_dcch - header_len_dtch - sdu_length_total - ta_len);
            post_padding = 0;
          } else {
            padding = 0;

            // adjust the header len
            if (header_len_dtch==0) {
              header_len_dcch = header_len_dcch_tmp;
            } else { //if (( header_len_dcch==0)&&((header_len_dtch==1)||(header_len_dtch==2)))
              header_len_dtch = header_len_dtch_tmp;
            }

            post_padding = TBS - sdu_length_total - header_len_dcch - header_len_dtch - ta_len ; // 1 is for the postpadding header
          }


          offset = generate_dlsch_header((unsigned char*)UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                                         // offset = generate_dlsch_header((unsigned char*)eNB_mac_inst[0].DLSCH_pdu[0][0].payload[0],
                                         num_sdus,              //num_sdus
                                         sdu_lengths,  //
                                         sdu_lcids,
                                         255,                                   // no drx
                                         ue_sched_ctl->ta_update, // timing advance
                                         NULL,                                  // contention res id
                                         padding,
                                         post_padding);

          //#ifdef DEBUG_eNB_SCHEDULER
          if (ue_sched_ctl->ta_update) {
            LOG_I(MAC,
                  "[eNB %d][DLSCH] Frame %d Generate header for UE_id %d on CC_id %d: sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,timing advance value : %d, padding %d,post_padding %d,(mcs %d, TBS %d, nb_rb %d),header_dcch %d, header_dtch %d\n",
                  module_idP,frameP, UE_id, CC_id, sdu_length_total,num_sdus,sdu_lengths[0],sdu_lcids[0],offset,
                  ue_sched_ctl->ta_update,padding,post_padding,mcs,TBS,nb_rb,header_len_dcch,header_len_dtch);
	  }
          //#endif
#ifdef DEBUG_eNB_SCHEDULER
          LOG_T(MAC,"[eNB %d] First 16 bytes of DLSCH : \n");

          for (i=0; i<16; i++) {
            LOG_T(MAC,"%x.",dlsch_buffer[i]);
          }

          LOG_T(MAC,"\n");
#endif
          // cycle through SDUs and place in dlsch_buffer
          memcpy(&UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset],dlsch_buffer,sdu_length_total);
          // memcpy(&eNB_mac_inst[0].DLSCH_pdu[0][0].payload[0][offset],dcch_buffer,sdu_lengths[0]);

          // fill remainder of DLSCH with random data
          for (j=0; j<(TBS-sdu_length_total-offset); j++) {
            UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset+sdu_length_total+j] = (char)(taus()&0xff);
          }

          //eNB_mac_inst[0].DLSCH_pdu[0][0].payload[0][offset+sdu_lengths[0]+j] = (char)(taus()&0xff);
          if (opt_enabled == 1) {
            trace_pdu(1, (uint8_t *)UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                      TBS, module_idP, 3, UE_RNTI(module_idP,UE_id),
                      eNB->frame, eNB->subframe,0,0);
            LOG_D(OPT,"[eNB %d][DLSCH] CC_id %d Frame %d  rnti %x  with size %d\n",
                  module_idP, CC_id, frameP, UE_RNTI(module_idP,UE_id), TBS);
          }

          T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP), T_INT(subframeP),
            T_INT(harq_pid), T_BUFFER(UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0], TBS));

          aggregation = process_ue_cqi(module_idP,UE_id);
          UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid] = nb_rb;

          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_SCHEDULED);
          // store stats
          eNB->eNB_stats[CC_id].dlsch_bytes_tx+=sdu_length_total;
          eNB->eNB_stats[CC_id].dlsch_pdus_tx+=1;

          UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used = nb_rb;
          UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used += nb_rb;
          UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1=eNB_UE_stats->dlsch_mcs1;
          UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2=mcs;
          UE_list->eNB_UE_stats[CC_id][UE_id].TBS = TBS;

          UE_list->eNB_UE_stats[CC_id][UE_id].overhead_bytes= TBS- sdu_length_total;
          UE_list->eNB_UE_stats[CC_id][UE_id].total_sdu_bytes+= sdu_length_total;
          UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes+= TBS;
          UE_list->eNB_UE_stats[CC_id][UE_id].total_num_pdus+=1;

          if (frame_parms[CC_id]->frame_type == TDD) {
            UE_list->UE_template[CC_id][UE_id].DAI++;
            //  printf("DAI update: subframeP %d: UE %d, DAI %d\n",subframeP,UE_id,UE_list->UE_template[CC_id][UE_id].DAI);
//#warning only for 5MHz channel
            update_ul_dci(module_idP,CC_id,rnti,UE_list->UE_template[CC_id][UE_id].DAI);
          }

	  // do PUCCH power control
          // this is the normalized RX power
	  eNB_UE_stats =  mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti);
	  normalized_rx_power = eNB_UE_stats->Po_PUCCH_dBm; 
	  target_rx_power = mac_xface->get_target_pucch_rx_power(module_idP,CC_id) + 20;
	    
          // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame*10+UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe;
          if (((framex10psubframe+10)<=(frameP*10+subframeP)) || //normal case
	      ((framex10psubframe>(frameP*10+subframeP)) && (((10240-framex10psubframe+frameP*10+subframeP)>=10)))) //frame wrap-around
	    if (eNB_UE_stats->Po_PUCCH_update == 1) { 
	      eNB_UE_stats->Po_PUCCH_update = 0;

	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame=frameP;
	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe=subframeP;
	      
	      if (normalized_rx_power>(target_rx_power+1)) {
		tpc = 0; //-1
		tpc_accumulated--;
	      } else if (normalized_rx_power<(target_rx_power-1)) {
		tpc = 2; //+1
		tpc_accumulated++;
	      } else {
		tpc = 1; //0
	      }
	      /*	      
	      LOG_I(MAC,"[eNB %d] DLSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
		    module_idP,frameP, subframeP,harq_pid,tpc,
		    tpc_accumulated,normalized_rx_power,target_rx_power);*/

	    } // Po_PUCCH has been updated 
	    else {
	      tpc = 1; //0
	    } // time to do TPC update 
	  else {
	    tpc = 1; //0
	  }

          switch (mac_xface->get_transmission_mode(module_idP,CC_id,rnti)) {
          case 1:
          case 2:
          default:
            if (frame_parms[CC_id]->frame_type == TDD) {
              switch (frame_parms[CC_id]->N_RB_DL) {
              case 6:
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->TPC = tpc;
                ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                break;

              case 25:
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->TPC = tpc;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                LOG_D(MAC,"Format1 DCI: harq_pid %d, ndi %d\n",harq_pid,((DCI1_5MHz_TDD_t*)DLSCH_dci)->ndi);
                break;

              case 50:
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->TPC = tpc;
                ((DCI1_10MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                break;

              case 100:
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->TPC = tpc;
                ((DCI1_20MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                break;

              default:
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->TPC = tpc;
                ((DCI1_5MHz_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                break;
              }
            } else {
              switch (frame_parms[CC_id]->N_RB_DL) {
              case 6:
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->TPC = tpc;
                break;

              case 25:
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->TPC = tpc;
                break;

              case 50:
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_10MHz_FDD_t*)DLSCH_dci)->TPC = tpc;
                break;

              case 100:
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_20MHz_FDD_t*)DLSCH_dci)->TPC = tpc;
                break;

              default:
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->mcs = mcs;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->ndi = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rv = 0;
                ((DCI1_5MHz_FDD_t*)DLSCH_dci)->TPC = tpc;
                break;
              }
            }

            break;

          case 3:
            if (frame_parms[CC_id]->frame_type == TDD) {
              switch (frame_parms[CC_id]->N_RB_DL) {
              case 6:
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->rv1 = 0;

                // deactivate TB2
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->rv2 = 1;

                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
                ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->TPC      = tpc;
                break;

              case 25:
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rv1 = 0;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
		((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rv2 = 1;

                LOG_D(MAC,"Format1 DCI: harq_pid %d, ndi %d\n",harq_pid,((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->ndi1);
                break;

              case 50:
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->rv1 = 0;
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
		((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->rv2 = 1;
                break;

              case 100:
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->rv1 = 0;
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
		((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->rv2 = 1;
                break;

              default:
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rv1 = 0;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->dai      = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
		((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rv2 = 1;
                break;
              }
            } else {
              switch (frame_parms[CC_id]->N_RB_DL) {
              case 6:
                ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->rv1 = 0;
		((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->rv2 = 1;
                break;

              case 25:
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rv1 = 0;
		((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rv2 = 1;
                break;

              case 50:
                ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->rv1 = 0;
		((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->TPC = tpc;
                // deactivate TB2
                ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->rv2 = 1;
                break;

              case 100:
                ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->rv1 = 0;
		((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->rv2 = 1;
                break;

              default:
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->mcs1 = mcs;
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->harq_pid = harq_pid;
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->ndi1 = 1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rv1 = 0;
		((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->TPC = tpc;

                // deactivate TB2
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->mcs2 = 0;
                ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rv2 = 1;
                break;
              }
            }

            break;

          case 4:
            //  if (nb_rb>10) {
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->mcs1 = mcs;
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->ndi1 = 1;
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->rv1 = round&3;
            ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	    ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->TPC = tpc;

            //}
            /* else {
               ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->mcs1 = eNB_UE_stats->DL_cqi[0];
               ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
               ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->ndi1 = 1;
               ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->rv1 = round&3;
               ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->tpmi = 5;
               ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
               }*/
            break;

          case 5:

            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->mcs = mcs;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->ndi = 1;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rv = round&3;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
	    ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->TPC = tpc;

            if(ue_sched_ctl->dl_pow_off[CC_id] == 2) {
              ue_sched_ctl->dl_pow_off[CC_id] = 1;
            }

            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dl_power_off = ue_sched_ctl->dl_pow_off[CC_id];
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->tpmi = 5;
            break;

          case 6:
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->mcs = mcs;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->harq_pid = harq_pid;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->ndi = 1;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rv = round&3;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dai = (UE_list->UE_template[CC_id][UE_id].DAI-1)&3;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->dl_power_off = 1;
            ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->tpmi = 5;
	    ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->TPC = tpc;

            break;
          }

          // Toggle NDI for next time
          LOG_D(MAC,"CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
                CC_id, frameP,subframeP,UE_id,
                UE_list->UE_template[CC_id][UE_id].rnti,harq_pid,UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]);
          UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]=1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
        } else {  // There is no data from RLC or MAC header, so don't schedule

        }
      }

      if (frame_parms[CC_id]->frame_type == TDD) {
        set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP,frame_parms);
      }

    } // UE_id loop
  }  // CC_id loop


  stop_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,VCD_FUNCTION_OUT);

}

//------------------------------------------------------------------------------
void
fill_DLSCH_dci(
	       module_id_t module_idP,
	       frame_t frameP,
	       sub_frame_t subframeP,
	       int* mbsfn_flagP
	       )
//------------------------------------------------------------------------------
{

  // loop over all allocated UEs and compute frequency allocations for PDSCH
  int   UE_id = -1;
  uint8_t            /* first_rb, */ nb_rb=3;
  rnti_t        rnti;
  //unsigned char *vrb_map;
  uint8_t            rballoc_sub[25];
  //uint8_t number_of_subbands=13;

  //unsigned char round;
  unsigned char harq_pid;
  void         *DLSCH_dci=NULL;
  DCI_PDU      *DCI_pdu;
  int           i;
  //void         *BCCH_alloc_pdu;
  int           size_bits,size_bytes;
  int CC_id;
  eNB_MAC_INST *eNB  =&eNB_mac_inst[module_idP];
  UE_list_t    *UE_list = &eNB->UE_list;
  //RA_TEMPLATE  *RA_template;

  start_meas(&eNB->fill_DLSCH_dci);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_FILL_DLSCH_DCI,VCD_FUNCTION_IN);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC,"Doing fill DCI for CC_id %d\n",CC_id);

    if (mbsfn_flagP[CC_id]>0)
      continue;

    DCI_pdu         = &eNB->common_channels[CC_id].DCI_pdu;
    

    // UE specific DCIs
    for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
      LOG_T(MAC,"CC_id %d, UE_id: %d => status %d\n",CC_id,UE_id,eNB_dlsch_info[module_idP][CC_id][UE_id].status);

      if (eNB_dlsch_info[module_idP][CC_id][UE_id].status == S_DL_SCHEDULED) {

        // clear scheduling flag
        eNB_dlsch_info[module_idP][CC_id][UE_id].status = S_DL_WAITING;
        rnti = UE_RNTI(module_idP,UE_id);
	//        mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,0);
	harq_pid = UE_list->UE_sched_ctrl[UE_id].harq_pid[CC_id];
        nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];

        DLSCH_dci = (void *)UE_list->UE_template[CC_id][UE_id].DLSCH_DCI[harq_pid];


        /// Synchronizing rballoc with rballoc_sub
        for(i=0; i<PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RBG; i++) {
          rballoc_sub[i] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][i];
        }

        switch(mac_xface->get_transmission_mode(module_idP,CC_id,rnti)) {
        default:

        case 1:

        case 2:
          LOG_D(MAC,"[eNB %d] CC_id %d Adding UE %d spec DCI for %d PRBS \n",module_idP, CC_id, UE_id, nb_rb);

          if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
            switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
            case 6:
              ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_1_5MHz_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI1_1_5MHz_TDD_t);
              size_bits  = sizeof_DCI1_1_5MHz_TDD_t;
              break;

            case 25:
              ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI1_5MHz_TDD_t);
              size_bits  = sizeof_DCI1_5MHz_TDD_t;
              break;

            case 50:
              ((DCI1_10MHz_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_10MHz_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI1_10MHz_TDD_t);
              size_bits  = sizeof_DCI1_10MHz_TDD_t;
              break;

            case 100:
              ((DCI1_20MHz_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_20MHz_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI1_20MHz_TDD_t);
              size_bits  = sizeof_DCI1_20MHz_TDD_t;
              break;

            default:
              ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_5MHz_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI1_5MHz_TDD_t);
              size_bits  = sizeof_DCI1_5MHz_TDD_t;
              break;
            }


          } else {
            switch(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
            case 6:
              ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_1_5MHz_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI1_1_5MHz_FDD_t);
              size_bits=sizeof_DCI1_1_5MHz_FDD_t;
              break;

            case 25:
              ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI1_5MHz_FDD_t);
              size_bits=sizeof_DCI1_5MHz_FDD_t;
              break;

            case 50:
              ((DCI1_10MHz_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_10MHz_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI1_10MHz_FDD_t);
              size_bits=sizeof_DCI1_10MHz_FDD_t;
              break;

            case 100:
              ((DCI1_20MHz_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_20MHz_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI1_20MHz_FDD_t);
              size_bits=sizeof_DCI1_20MHz_FDD_t;
              break;

            default:
              ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI1_5MHz_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI1_5MHz_FDD_t);
              size_bits=sizeof_DCI1_5MHz_FDD_t;
              break;
            }
          }

          add_ue_spec_dci(DCI_pdu,
                          DLSCH_dci,
                          rnti,
                          size_bytes,
                          process_ue_cqi (module_idP,UE_id),//aggregation,
                          size_bits,
                          format1,
                          0);

          break;

        case 3:
          LOG_D(MAC,"[eNB %d] CC_id %d Adding Format 2A UE %d spec DCI for %d PRBS (rb alloc: %x) \n",
                module_idP, CC_id, UE_id, nb_rb);

          if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
            switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
            case 6:
              ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_1_5MHz_2A_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI2A_1_5MHz_2A_TDD_t);
              size_bits  = sizeof_DCI2A_1_5MHz_2A_TDD_t;
              break;

            case 25:
              ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI2A_5MHz_2A_TDD_t);
              size_bits  = sizeof_DCI2A_5MHz_2A_TDD_t;
              break;

            case 50:
              ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_10MHz_2A_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI2A_10MHz_2A_TDD_t);
              size_bits  = sizeof_DCI2A_10MHz_2A_TDD_t;
              break;

            case 100:
              ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_20MHz_2A_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI2A_20MHz_2A_TDD_t);
              size_bits  = sizeof_DCI2A_20MHz_2A_TDD_t;
              break;

            default:
              ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_5MHz_2A_TDD_t*)DLSCH_dci)->rah = 0;
              size_bytes = sizeof(DCI2A_5MHz_2A_TDD_t);
              size_bits  = sizeof_DCI2A_5MHz_2A_TDD_t;
              break;
            }


          } else {
            switch(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
            case 6:
              ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_1_5MHz_2A_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI2A_1_5MHz_2A_FDD_t);
              size_bits=sizeof_DCI2A_1_5MHz_2A_FDD_t;
              break;

            case 25:
              ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI2A_5MHz_2A_FDD_t);
              size_bits=sizeof_DCI2A_5MHz_2A_FDD_t;
              break;

            case 50:
              ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_10MHz_2A_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI2A_10MHz_2A_FDD_t);
              size_bits=sizeof_DCI2A_10MHz_2A_FDD_t;
              break;

            case 100:
              ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_20MHz_2A_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI2A_20MHz_2A_FDD_t);
              size_bits=sizeof_DCI2A_20MHz_2A_FDD_t;
              break;

            default:
              ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
              ((DCI2A_5MHz_2A_FDD_t*)DLSCH_dci)->rah = 0;
              size_bytes=sizeof(DCI2A_5MHz_2A_FDD_t);
              size_bits=sizeof_DCI2A_5MHz_2A_FDD_t;
              break;
            }
          }

          add_ue_spec_dci(DCI_pdu,
                          DLSCH_dci,
                          rnti,
                          size_bytes,
                          process_ue_cqi (module_idP,UE_id),//aggregation,
                          size_bits,
                          format2A,
                          0);

          break;

        case 4:

          //if (nb_rb>10) {
          // DCI format 2_2A
          ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
          ((DCI2_5MHz_2A_TDD_t*)DLSCH_dci)->rah = 0;
          add_ue_spec_dci(DCI_pdu,
                          DLSCH_dci,
                          rnti,
                          sizeof(DCI2_5MHz_2A_TDD_t),
                          process_ue_cqi (module_idP,UE_id),//aggregation,
                          sizeof_DCI2_5MHz_2A_TDD_t,
                          format2,
                          0);
          /*}
            else {
            ((DCI2_5MHz_2A_L10PRB_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
            add_ue_spec_dci(DCI_pdu,
            DLSCH_dci,
            rnti,
            sizeof(DCI2_5MHz_2A_L10PRB_TDD_t),
            2,//aggregation,
            sizeof_DCI2_5MHz_2A_L10PRB_TDD_t,
            format2_2A_L10PRB);
            }*/
          break;

        case 5:
          ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
          ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rah = 0;

          add_ue_spec_dci(DCI_pdu,
                          DLSCH_dci,
                          rnti,
                          sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t),
                          process_ue_cqi (module_idP,UE_id),//aggregation,
                          sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t,
                          format1E_2A_M10PRB,
                          0);
          break;

        case 6:
          ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rballoc = allocate_prbs_sub(nb_rb,rballoc_sub);
          ((DCI1E_5MHz_2A_M10PRB_TDD_t*)DLSCH_dci)->rah = 0;

          add_ue_spec_dci(DCI_pdu,
                          DLSCH_dci,
                          rnti,
                          sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t),
                          process_ue_cqi (module_idP,UE_id),//aggregation
                          sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t,
                          format1E_2A_M10PRB,
                          0);
          break;

        }
      }
    }

  }

  stop_meas(&eNB->fill_DLSCH_dci);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_FILL_DLSCH_DCI,VCD_FUNCTION_OUT);
}

//------------------------------------------------------------------------------
unsigned char*
get_dlsch_sdu(
  module_id_t module_idP,
  int CC_id,
  frame_t frameP,
  rnti_t rntiP,
  uint8_t TBindex
)
//------------------------------------------------------------------------------
{

  int UE_id;
  eNB_MAC_INST *eNB=&eNB_mac_inst[module_idP];

  if (rntiP==SI_RNTI) {
    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d Get DLSCH sdu for BCCH \n", module_idP, CC_id, frameP);

    return((unsigned char *)&eNB->common_channels[CC_id].BCCH_pdu.payload[0]);
  }

  UE_id = find_UE_id(module_idP,rntiP);

  if (UE_id != -1) {
    LOG_D(MAC,"[eNB %d] Frame %d:  CC_id %d Get DLSCH sdu for rnti %x => UE_id %d\n",module_idP,frameP,CC_id,rntiP,UE_id);
    return((unsigned char *)&eNB->UE_list.DLSCH_pdu[CC_id][TBindex][UE_id].payload[0]);
  } else {
    LOG_E(MAC,"[eNB %d] Frame %d: CC_id %d UE with RNTI %x does not exist\n", module_idP,frameP,CC_id,rntiP);
    return NULL;
  }

}


//------------------------------------------------------------------------------
void
update_ul_dci(
  module_id_t module_idP,
  uint8_t CC_id,
  rnti_t rnti,
  uint8_t dai)
//------------------------------------------------------------------------------
{

  DCI_PDU             *DCI_pdu   = &eNB_mac_inst[module_idP].common_channels[CC_id].DCI_pdu;
  int                  i;
  DCI0_5MHz_TDD_1_6_t *ULSCH_dci = NULL;;

  if (mac_xface->frame_parms->frame_type == TDD) {
    for (i=0; i<DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci; i++) {
      ULSCH_dci = (DCI0_5MHz_TDD_1_6_t *)DCI_pdu->dci_alloc[i].dci_pdu;

      if ((DCI_pdu->dci_alloc[i].format == format0) && (DCI_pdu->dci_alloc[i].rnti == rnti)) {
        ULSCH_dci->dai = (dai-1)&3;
      }
    }
  }

  //  printf("Update UL DCI: DAI %d\n",dai);
}


//------------------------------------------------------------------------------
void set_ue_dai(
  sub_frame_t   subframeP,
  uint8_t       tdd_config,
  int           UE_id,
  uint8_t       CC_id,
  UE_list_t*     UE_list
)
//------------------------------------------------------------------------------
{

  switch (tdd_config) {
  case 0:
    if ((subframeP==0)||(subframeP==1)||(subframeP==3)||(subframeP==5)||(subframeP==6)||(subframeP==8)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 1:
    if ((subframeP==0)||(subframeP==4)||(subframeP==5)||(subframeP==9)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 2:
    if ((subframeP==4)||(subframeP==5)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 3:
    if ((subframeP==5)||(subframeP==7)||(subframeP==9)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 4:
    if ((subframeP==0)||(subframeP==6)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 5:
    if (subframeP==9) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 6:
    if ((subframeP==0)||(subframeP==1)||(subframeP==5)||(subframeP==6)||(subframeP==9)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  default:
    UE_list->UE_template[CC_id][UE_id].DAI = 0;
    LOG_N(MAC,"unknow TDD config %d\n",tdd_config);
    break;
  }
}
