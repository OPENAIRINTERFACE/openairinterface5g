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

/*! \file eNB_scheduler_primitives.c
 * \brief primitives used by eNB for BCH, RACH, ULSCH, DLSCH scheduling
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

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

int to_prb(int dl_Bandwidth) {
  int prbmap[6] = {6,15,25,50,75,100};

  AssertFatal(dl_Bandwidth < 6,"dl_Bandwidth is 0..5\n");
  return(prbmap[dl_Bandwidth]);
}

int to_rbg(int dl_Bandwidth) {
  int rbgmap[6] = {6,8,13,17,19,25};

  AssertFatal(dl_Bandwidth < 6,"dl_Bandwidth is 0..5\n");
  return(rbgmap[dl_Bandwidth]);
}


int get_phich_resource_times6(COMMON_channels_t *cc) {
  int phichmap[4] = {1,3,6,12};
  AssertFatal(cc!=NULL,"cc is null\n");
  AssertFatal(cc->mib!=NULL,"cc->mib is null\n");
  AssertFatal((cc->mib->message.phich_Config.phich_Resource>=0) && 
	      (cc->mib->message.phich_Config.phich_Resource<4),
	      "phich_Resource %d not in 0..3\n",(int)cc->mib->message.phich_Config.phich_Resource);

  return(phichmap[cc->mib->message.phich_Config.phich_Resource]);
}

uint16_t mac_computeRIV(uint16_t N_RB_DL,uint16_t RBstart,uint16_t Lcrbs) {

  uint16_t RIV;

  if (Lcrbs<=(1+(N_RB_DL>>1)))
    RIV = (N_RB_DL*(Lcrbs-1)) + RBstart;
  else
    RIV = (N_RB_DL*(N_RB_DL+1-Lcrbs)) + (N_RB_DL-1-RBstart);

  return(RIV);
}

void get_Msg3alloc(COMMON_channels_t *cc,
		   sub_frame_t current_subframe,
		   frame_t current_frame,
		   frame_t *frame,
		   sub_frame_t *subframe)
{

  // Fill in other TDD Configuration!!!!

  if (cc->tdd_Config==NULL) { // FDD
    *subframe = current_subframe+6;

    if (*subframe>9) {
      *subframe = *subframe-10;
      *frame = (current_frame+1) & 1023;
    } else {
      *frame=current_frame;
    }
  } else { // TDD
    if (cc->tdd_Config->subframeAssignment == 1) {
      switch (current_subframe) {

      case 0:
        *subframe = 7;
        *frame = current_frame;
        break;

      case 4:
        *subframe = 2;
        *frame = (current_frame+1) & 1023;
        break;

      case 5:
        *subframe = 2;
        *frame = (current_frame+1) & 1023;
        break;

      case 9:
        *subframe = 7;
        *frame = (current_frame+1) & 1023;
        break;
      }
    } else if (cc->tdd_Config->subframeAssignment == 3) {
      switch (current_subframe) {

      case 0:
      case 5:
      case 6:
        *subframe = 2;
        *frame = (current_frame+1) & 1023;
        break;

      case 7:
        *subframe = 3;
        *frame = (current_frame+1) & 1023;
        break;

      case 8:
        *subframe = 4;
        *frame = (current_frame+1) & 1023;
        break;

      case 9:
        *subframe = 2;
        *frame = (current_frame+2) & 1023;
        break;
      }
    } else if (cc->tdd_Config->subframeAssignment == 4) {
        switch (current_subframe) {

        case 0:
        case 4:
        case 5:
        case 6:
          *subframe = 2;
          *frame = (current_frame+1) & 1023;
          break;

        case 7:
          *subframe = 3;
          *frame = (current_frame+1) & 1023;
          break;

        case 8:
        case 9:
          *subframe = 2;
          *frame = (current_frame+2) & 1023;
          break;
        }
      } else if (cc->tdd_Config->subframeAssignment == 5) {
          switch (current_subframe) {

          case 0:
          case 4:
          case 5:
          case 6:
            *subframe = 2;
            *frame = (current_frame+1) & 1023;
            break;

          case 7:
          case 8:
          case 9:
            *subframe = 2;
            *frame = (current_frame+2) & 1023;
            break;
          }
        }
  }
}



void get_Msg3allocret(COMMON_channels_t *cc,
		      sub_frame_t current_subframe,
		      frame_t current_frame,
		      frame_t *frame,
		      sub_frame_t *subframe)
{
  if (cc->tdd_Config == NULL) { //FDD
    /* always retransmit in n+8 */
    *subframe = current_subframe + 8;

    if (*subframe > 9) {
      *subframe = *subframe - 10;
      *frame = (current_frame + 1) & 1023;
    } else {
      *frame = current_frame;
    }
  } else {
    if (cc->tdd_Config->subframeAssignment == 1) {
      // original PUSCH in 2, PHICH in 6 (S), ret in 2
      // original PUSCH in 3, PHICH in 9, ret in 3
      // original PUSCH in 7, PHICH in 1 (S), ret in 7
      // original PUSCH in 8, PHICH in 4, ret in 8
      *frame = (current_frame+1) & 1023;
    } else if (cc->tdd_Config->subframeAssignment == 3) {
      // original PUSCH in 2, PHICH in 8, ret in 2 next frame
      // original PUSCH in 3, PHICH in 9, ret in 3 next frame
      // original PUSCH in 4, PHICH in 0, ret in 4 next frame
      *frame=(current_frame+1) & 1023;
    } else if (cc->tdd_Config->subframeAssignment == 4) {
        // original PUSCH in 2, PHICH in 8, ret in 2 next frame
        // original PUSCH in 3, PHICH in 9, ret in 3 next frame
        *frame=(current_frame+1) & 1023;
    } else if (cc->tdd_Config->subframeAssignment == 5) {
        // original PUSCH in 2, PHICH in 8, ret in 2 next frame
        *frame=(current_frame+1) & 1023;
    }
  }
}

uint8_t subframe2harqpid(COMMON_channels_t *cc,frame_t frame,sub_frame_t subframe)
{
  uint8_t ret = 255;

  AssertFatal(cc!=NULL,"cc is null\n");

  if (cc->tdd_Config == NULL) { // FDD
    ret = (((frame<<1)+subframe)&7);
  } else {

    switch (cc->tdd_Config->subframeAssignment) {

    case 1:
      if ((subframe==2) ||
          (subframe==3) ||
          (subframe==7) ||
          (subframe==8))
        switch (subframe) {
        case 2:
        case 3:
          ret = (subframe-2);
          break;

        case 7:
        case 8:
          ret = (subframe-5);
          break;

        default:
	  AssertFatal(1==0,"subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframe,(int)cc->tdd_Config->subframeAssignment);
          break;
        }

      break;

    case 2:
      AssertFatal((subframe==2) || (subframe==7),
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframe,(int)cc->tdd_Config->subframeAssignment);
       
      ret = (subframe/7);
      break;

    case 3:
      AssertFatal((subframe>1) && (subframe<5), 
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframe,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframe-2);
      break;

    case 4:
      AssertFatal((subframe>1) && (subframe<4),
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframe,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframe-2);
      break;

    case 5:
      AssertFatal(subframe==2,
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframe,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframe-2);
      break;

    default:
      AssertFatal(1==0,"subframe2_harq_pid, Unsupported TDD mode %d\n",(int)cc->tdd_Config->subframeAssignment);
    }
  }
  return ret;
}

uint8_t get_Msg3harqpid(COMMON_channels_t *cc,
			frame_t frame,
			sub_frame_t current_subframe)
{

  uint8_t ul_subframe=0;
  uint32_t ul_frame=0;

  if (cc->tdd_Config == NULL) { // FDD
    ul_subframe = (current_subframe>3) ? (current_subframe-4) : (current_subframe+6);
    ul_frame    = (current_subframe>3) ? ((frame+1)&1023) : frame;
  } else {
    switch (cc->tdd_Config->subframeAssignment) {
    case 1:
      switch (current_subframe) {

      case 9:
      case 0:
        ul_subframe = 7;
        break;

      case 5:
      case 7:
        ul_subframe = 2;
        break;

      }

      break;

    case 3:
      switch (current_subframe) {

      case 0:
      case 5:
      case 6:
        ul_subframe = 2;
        break;

      case 7:
        ul_subframe = 3;
        break;

      case 8:
        ul_subframe = 4;
        break;

      case 9:
        ul_subframe = 2;
        break;
      }

      break;

    case 4:
      switch (current_subframe) {

      case 0:
      case 5:
      case 6:
      case 8:
      case 9:
        ul_subframe = 2;
        break;

      case 7:
        ul_subframe = 3;
        break;
      }

      break;

    case 5:
      ul_subframe =2;
      break;

    default:
      LOG_E(PHY,"get_Msg3_harq_pid: Unsupported TDD configuration %d\n",(int)cc->tdd_Config->subframeAssignment);
      AssertFatal(1==0,"get_Msg3_harq_pid: Unsupported TDD configuration");
      break;
    }
  }

  return(subframe2harqpid(cc,ul_frame,ul_subframe));

}

int is_UL_sf(COMMON_channels_t *ccP,sub_frame_t subframeP)
{

  // if FDD return dummy value
  if (ccP->tdd_Config == NULL)
    return(0);

  switch (ccP->tdd_Config->subframeAssignment) {

  case 1:
    switch (subframeP) {
    case 0:
    case 4:
    case 5:
    case 9:
      return(0);
      break;

    case 2:
    case 3:
    case 7:
    case 8:
      return(1);
      break;

    default:
      return(0);
      break;
    }
    break;

  case 3:
    if  ((subframeP<=1) || (subframeP>=5))
      return(0);
    else if ((subframeP>1) && (subframeP < 5))
      return(1);
    else AssertFatal(1==0,"Unknown subframe number\n");
    break;

  case 4:
    if  ((subframeP<=1) || (subframeP>=4))
      return(0);
    else if ((subframeP>1) && (subframeP < 4))
      return(1);
    else AssertFatal(1==0,"Unknown subframe number\n");
    break;
    
  case 5:
    if  ((subframeP<=1) || (subframeP>=3))
      return(0);
    else if ((subframeP>1) && (subframeP < 3))
      return(1);
    else AssertFatal(1==0,"Unknown subframe number\n");
    break;

  default:
    AssertFatal(1==0,"subframe %d Unsupported TDD configuration %d\n",
		subframeP,ccP->tdd_Config->subframeAssignment);
    break;

  }
}

#ifdef Rel14

int get_numnarrowbands(long dl_Bandwidth) {
  int nb_tab[6] = {1,2,4,8,12,16};

  AssertFatal(dl_Bandwidth<7 || dl_Bandwidth>=0,"dl_Bandwidth not in [0..6]\n");
  return(nb_tab[dl_Bandwidth]);
}

int get_numnarrowbandbits(long dl_Bandwidth) {
  int nbbits_tab[6] = {0,1,2,3,4,4};

  AssertFatal(dl_Bandwidth<7 || dl_Bandwidth>=0,"dl_Bandwidth not in [0..6]\n");
  return(nbbits_tab[dl_Bandwidth]);
}

//This implements the frame/subframe condition for first subframe of MPDCCH transmission (Section 9.1.5 36.213, Rel 13/14)
int startSF_fdd_RA_times2[8] = {2,3,4,5,8,10,16,20};
int startSF_tdd_RA[7]        = {1,2,4,5,8,10,20};

int mpdcch_sf_condition(eNB_MAC_INST *eNB,int CC_id, frame_t frameP,sub_frame_t subframeP,int rmax,MPDCCH_TYPES_t mpdcch_type) {

  struct PRACH_ConfigSIB_v1310 *ext4_prach = eNB->common_channels[CC_id].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
  int T;

  switch (mpdcch_type) {
  case TYPE0:
    AssertFatal(1==0,"MPDCCH Type 0 not handled yet\n");
    break;
  case TYPE1:
    AssertFatal(1==0,"MPDCCH Type 1 not handled yet\n");
    break;
  case TYPE1A:
    AssertFatal(1==0,"MPDCCH Type 1A not handled yet\n");
    break;
  case TYPE2: // RAR
    AssertFatal(ext4_prach->mpdcch_startSF_CSS_RA_r13!=NULL,
		"mpdcch_startSF_CSS_RA_r13 is null\n");
    if (eNB->common_channels[CC_id].tdd_Config==NULL) //FDD
      T = rmax*startSF_fdd_RA_times2[ext4_prach->mpdcch_startSF_CSS_RA_r13->choice.fdd_r13]>>1;
    else //TDD
      T = rmax*startSF_tdd_RA[ext4_prach->mpdcch_startSF_CSS_RA_r13->choice.tdd_r13];
    break;
  case TYPE2A:
    AssertFatal(1==0,"MPDCCH Type 2A not handled yet\n");
    break;
  case TYPEUESPEC:
    AssertFatal(1==0,"MPDCCH Type UESPEC not handled yet\n");
    break;
  default:
    return(0);
  }

  if (((10*frameP) + subframeP)%T == 0) return(1);
  else return(0);

}


#endif

//------------------------------------------------------------------------------
void init_ue_sched_info(void)
//------------------------------------------------------------------------------
{
  module_id_t i,j,k;

  for (i=0; i<NUMBER_OF_eNB_MAX; i++) {
    for (k=0; k<MAX_NUM_CCs; k++) {
      for (j=0; j<NUMBER_OF_UE_MAX; j++) {
        // init DL
        eNB_dlsch_info[i][k][j].weight           = 0;
        eNB_dlsch_info[i][k][j].subframe         = 0;
        eNB_dlsch_info[i][k][j].serving_num      = 0;
        eNB_dlsch_info[i][k][j].status           = S_DL_NONE;
        // init UL
        eNB_ulsch_info[i][k][j].subframe         = 0;
        eNB_ulsch_info[i][k][j].serving_num      = 0;
        eNB_ulsch_info[i][k][j].status           = S_UL_NONE;
      }
    }
  }
}



//------------------------------------------------------------------------------
unsigned char get_ue_weight(module_id_t module_idP, int CC_idP, int ue_idP)
//------------------------------------------------------------------------------
{

  return(eNB_dlsch_info[module_idP][CC_idP][ue_idP].weight);

}

//------------------------------------------------------------------------------
int find_UE_id(module_id_t mod_idP, rnti_t rntiP)
//------------------------------------------------------------------------------
{
  int UE_id;
  UE_list_t *UE_list = &RC.mac[mod_idP]->UE_list;

  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
    if (UE_list->active[UE_id] != TRUE) continue;
    if (UE_list->UE_template[UE_PCCID(mod_idP,UE_id)][UE_id].rnti==rntiP) {
      return(UE_id);
    }
  }

  return(-1);
}

//------------------------------------------------------------------------------
int find_RA_id(module_id_t mod_idP, int CC_idP, rnti_t rntiP)
//------------------------------------------------------------------------------
{
  int RA_id;
  AssertFatal(RC.mac[mod_idP],"RC.mac[%d] is null\n",mod_idP);
  
  RA_TEMPLATE *RA_template = (RA_TEMPLATE *)&RC.mac[mod_idP]->common_channels[CC_idP].RA_template[0];

 

  for (RA_id = 0; RA_id < NB_RA_PROC_MAX; RA_id++)
    if (RA_template[RA_id].RA_active==TRUE && 
        RA_template[RA_id].wait_ack_Msg4 == 0 &&
	RA_template[RA_id].rnti == rntiP) return(RA_id);
  return(-1);
}

//------------------------------------------------------------------------------
int UE_num_active_CC(UE_list_t *listP,int ue_idP)
//------------------------------------------------------------------------------
{
  return(listP->numactiveCCs[ue_idP]);
}

//------------------------------------------------------------------------------
int UE_PCCID(module_id_t mod_idP,int ue_idP)
//------------------------------------------------------------------------------
{
  return(RC.mac[mod_idP]->UE_list.pCC_id[ue_idP]);
}

//------------------------------------------------------------------------------
rnti_t UE_RNTI(module_id_t mod_idP, int ue_idP)
//------------------------------------------------------------------------------
{

  rnti_t rnti = RC.mac[mod_idP]->UE_list.UE_template[UE_PCCID(mod_idP,ue_idP)][ue_idP].rnti;

  if (rnti>0) {
    return (rnti);
  }

  LOG_D(MAC,"[eNB %d] Couldn't find RNTI for UE %d\n",mod_idP,ue_idP);
  //display_backtrace();
  return(NOT_A_RNTI);
}

//------------------------------------------------------------------------------
boolean_t is_UE_active(module_id_t mod_idP, int ue_idP)
//------------------------------------------------------------------------------
{
  return(RC.mac[mod_idP]->UE_list.active[ue_idP]);
}

/*
uint8_t find_active_UEs(module_id_t module_idP,int CC_id){

  module_id_t        ue_mod_id      = 0;
  rnti_t        rnti         = 0;
  uint8_t            nb_active_ue = 0;

  for (ue_mod_id=0;ue_mod_id<NUMBER_OF_UE_MAX;ue_mod_id++) {

      if (((rnti=eNB_mac_inst[module_idP][CC_id].UE_template[ue_mod_id].rnti) !=0)&&(eNB_mac_inst[module_idP][CC_id].UE_template[ue_mod_id].ul_active==TRUE)){

          if (mac_xface->get_eNB_UE_stats(module_idP,rnti) != NULL){ // check at the phy enb_ue state for this rnti
      nb_active_ue++;
          }
          else { // this ue is removed at the phy => remove it at the mac as well
      mac_remove_ue(module_idP, CC_id, ue_mod_id);
          }
      }
  }
  return(nb_active_ue);
}
*/


// get aggregation (L) form phy for a give UE
unsigned char get_aggregation (uint8_t bw_index, uint8_t cqi, uint8_t dci_fmt)
{
  unsigned char aggregation=3;
  
  switch (dci_fmt){
    
  case format0:
    aggregation = cqi2fmt0_agg[bw_index][cqi];
    break;
  case format1:
  case format1A:
  case format1B:
  case format1D:
    aggregation = cqi2fmt1x_agg[bw_index][cqi]; 
    break;
  case format2:
  case format2A:
  case format2B:
  case format2C:
  case format2D:
    aggregation = cqi2fmt2x_agg[bw_index][cqi]; 
    break;
  case format1C:
  case format1E_2A_M10PRB:
  case format3:
  case format3A:
  case format4:
  default:
    LOG_W(MAC,"unsupported DCI format %d\n",dci_fmt);
  }

   LOG_D(MAC,"Aggregation level %d (cqi %d, bw_index %d, format %d)\n", 
   	1<<aggregation, cqi,bw_index,dci_fmt);
   
  return 1<<aggregation;
}

void dump_ue_list(UE_list_t *listP, int ul_flag)
{
  int j;

  if ( ul_flag == 0 ) {
    for (j=listP->head; j>=0; j=listP->next[j]) {
      LOG_T(MAC,"node %d => %d\n",j,listP->next[j]);
    }
  } else {
    for (j=listP->head_ul; j>=0; j=listP->next_ul[j]) {
      LOG_T(MAC,"node %d => %d\n",j,listP->next_ul[j]);
    }
  }
}

int add_new_ue(module_id_t mod_idP, int cc_idP, rnti_t rntiP,int harq_pidP)
{
  int UE_id;
  int i, j;

  UE_list_t *UE_list = &RC.mac[mod_idP]->UE_list;

  LOG_D(MAC,"[eNB %d, CC_id %d] Adding UE with rnti %x (next avail %d, num_UEs %d)\n",mod_idP,cc_idP,rntiP,UE_list->avail,UE_list->num_UEs);
  dump_ue_list(UE_list,0);

  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    if (UE_list->active[i] == TRUE) continue;
    UE_id = i;
    UE_list->UE_template[cc_idP][UE_id].rnti       = rntiP;
    UE_list->UE_template[cc_idP][UE_id].configured = FALSE;
    UE_list->numactiveCCs[UE_id]                   = 1;
    UE_list->numactiveULCCs[UE_id]                 = 1;
    UE_list->pCC_id[UE_id]                         = cc_idP;
    UE_list->ordered_CCids[0][UE_id]               = cc_idP;
    UE_list->ordered_ULCCids[0][UE_id]             = cc_idP;
    UE_list->num_UEs++;
    UE_list->active[UE_id]                         = TRUE;
    memset((void*)&UE_list->UE_sched_ctrl[UE_id],0,sizeof(UE_sched_ctrl));
    memset((void*)&UE_list->eNB_UE_stats[cc_idP][UE_id],0,sizeof(eNB_UE_STATS));

    for (j=0; j<8; j++) {
      UE_list->UE_template[cc_idP][UE_id].oldNDI[j]    = (j==0)?1:0;   // 1 because first transmission is with format1A (Msg4) for harq_pid 0
      UE_list->UE_template[cc_idP][UE_id].oldNDI_UL[j] = (j==harq_pidP)?0:1; // 1st transmission is with Msg3;
    }

    eNB_ulsch_info[mod_idP][cc_idP][UE_id].status = S_UL_WAITING;
    eNB_dlsch_info[mod_idP][cc_idP][UE_id].status = S_DL_WAITING;
    LOG_D(MAC,"[eNB %d] Add UE_id %d on Primary CC_id %d: rnti %x\n",mod_idP,UE_id,cc_idP,rntiP);
    dump_ue_list(UE_list,0);
    return(UE_id);
  }

  printf("MAC: cannot add new UE for rnti %x\n", rntiP);
  LOG_E(MAC,"error in add_new_ue(), could not find space in UE_list, Dumping UE list\n");
  dump_ue_list(UE_list,0);
  return(-1);
}

//------------------------------------------------------------------------------
int rrc_mac_remove_ue(module_id_t mod_idP,rnti_t rntiP) 
//------------------------------------------------------------------------------
{
  int i;
  UE_list_t *UE_list = &RC.mac[mod_idP]->UE_list;
  int UE_id = find_UE_id(mod_idP,rntiP);
  int pCC_id;

  if (UE_id == -1) {
printf("MAC: cannot remove UE rnti %x\n", rntiP);
    LOG_W(MAC,"rrc_mac_remove_ue: UE %x not found\n", rntiP);
    mac_phy_remove_ue(mod_idP, rntiP);
    return 0;
  }

  pCC_id = UE_PCCID(mod_idP,UE_id);

  LOG_I(MAC,"Removing UE %d from Primary CC_id %d (rnti %x)\n",UE_id,pCC_id, rntiP);
  dump_ue_list(UE_list,0);

  UE_list->active[UE_id] = FALSE;
  UE_list->num_UEs--;

  // clear all remaining pending transmissions
  UE_list->UE_template[pCC_id][UE_id].bsr_info[LCGID0]  = 0;
  UE_list->UE_template[pCC_id][UE_id].bsr_info[LCGID1]  = 0;
  UE_list->UE_template[pCC_id][UE_id].bsr_info[LCGID2]  = 0;
  UE_list->UE_template[pCC_id][UE_id].bsr_info[LCGID3]  = 0;

  UE_list->UE_template[pCC_id][UE_id].ul_SR             = 0;
  UE_list->UE_template[pCC_id][UE_id].rnti              = NOT_A_RNTI;
  UE_list->UE_template[pCC_id][UE_id].ul_active         = FALSE;
  eNB_ulsch_info[mod_idP][pCC_id][UE_id].rnti                        = NOT_A_RNTI;
  eNB_ulsch_info[mod_idP][pCC_id][UE_id].status                      = S_UL_NONE;
  eNB_dlsch_info[mod_idP][pCC_id][UE_id].rnti                        = NOT_A_RNTI;
  eNB_dlsch_info[mod_idP][pCC_id][UE_id].status                      = S_DL_NONE;

  mac_phy_remove_ue(mod_idP,rntiP);

  // check if this has an RA process active
  RA_TEMPLATE *RA_template;
  for (i=0;i<NB_RA_PROC_MAX;i++) {
    RA_template = (RA_TEMPLATE *)&RC.mac[mod_idP]->common_channels[pCC_id].RA_template[i];
    if (RA_template->rnti == rntiP){
      RA_template->RA_active=FALSE;
      RA_template->generate_rar=0;
      RA_template->generate_Msg4=0;
      RA_template->wait_ack_Msg4=0;
      RA_template->timing_offset=0;
      RA_template->RRC_timer=20;
      RA_template->rnti = 0;
      //break;
    }
  }

  return 0;
}



int prev(UE_list_t *listP, int nodeP, int ul_flag)
{
  int j;

  if (ul_flag == 0 ) {
    if (nodeP==listP->head) {
      return(nodeP);
    }

    for (j=listP->head; j>=0; j=listP->next[j]) {
      if (listP->next[j]==nodeP) {
        return(j);
    }
    }
  } else {
    if (nodeP==listP->head_ul) {
      return(nodeP);
    }

    for (j=listP->head_ul; j>=0; j=listP->next_ul[j]) {
      if (listP->next_ul[j]==nodeP) {
        return(j);
      }
    }
  }

  LOG_E(MAC,"error in prev(), could not find previous to %d in UE_list %s, should never happen, Dumping UE list\n",
        nodeP, (ul_flag == 0)? "DL" : "UL");
  dump_ue_list(listP, ul_flag);


  return(-1);
}

void swap_UEs(UE_list_t *listP,int nodeiP, int nodejP, int ul_flag)
{

  int prev_i,prev_j,next_i,next_j;

  LOG_T(MAC,"Swapping UE %d,%d\n",nodeiP,nodejP);
  dump_ue_list(listP,ul_flag);

  prev_i = prev(listP,nodeiP,ul_flag);
  prev_j = prev(listP,nodejP,ul_flag);

  AssertFatal((prev_i>=0) && (prev_j>=0),
	      "swap_UEs: problem");

  if (ul_flag == 0) {
    next_i = listP->next[nodeiP];
    next_j = listP->next[nodejP];
  } else {
    next_i = listP->next_ul[nodeiP];
    next_j = listP->next_ul[nodejP];
  }

  LOG_T(MAC,"[%s] next_i %d, next_i, next_j %d, head %d \n",
        (ul_flag == 0)? "DL" : "UL",
        next_i,next_j,listP->head);

  if (ul_flag == 0 ) {

    if (next_i == nodejP) {   // case ... p(i) i j n(j) ... => ... p(j) j i n(i) ...
      LOG_T(MAC,"Case ... p(i) i j n(j) ... => ... p(j) j i n(i) ...\n");

      listP->next[nodeiP] = next_j;
      listP->next[nodejP] = nodeiP;

      if (nodeiP==listP->head) { // case i j n(j)
        listP->head = nodejP;
      } else {
        listP->next[prev_i] = nodejP;
      }
    } else if (next_j == nodeiP) {  // case ... p(j) j i n(i) ... => ... p(i) i j n(j) ...
      LOG_T(MAC,"Case ... p(j) j i n(i) ... => ... p(i) i j n(j) ...\n");
      listP->next[nodejP] = next_i;
      listP->next[nodeiP] = nodejP;

      if (nodejP==listP->head) { // case j i n(i)
        listP->head = nodeiP;
      } else {
        listP->next[prev_j] = nodeiP;
      }
    } else {  // case ...  p(i) i n(i) ... p(j) j n(j) ...
      listP->next[nodejP] = next_i;
      listP->next[nodeiP] = next_j;


      if (nodeiP==listP->head) {
        LOG_T(MAC,"changing head to %d\n",nodejP);
        listP->head=nodejP;
        listP->next[prev_j] = nodeiP;
      } else if (nodejP==listP->head) {
        LOG_D(MAC,"changing head to %d\n",nodeiP);
        listP->head=nodeiP;
        listP->next[prev_i] = nodejP;
      } else {
        listP->next[prev_i] = nodejP;
        listP->next[prev_j] = nodeiP;
      }
    }
  } else { // ul_flag

    if (next_i == nodejP) {   // case ... p(i) i j n(j) ... => ... p(j) j i n(i) ...
      LOG_T(MAC,"[UL] Case ... p(i) i j n(j) ... => ... p(j) j i n(i) ...\n");

      listP->next_ul[nodeiP] = next_j;
      listP->next_ul[nodejP] = nodeiP;

      if (nodeiP==listP->head_ul) { // case i j n(j)
        listP->head_ul = nodejP;
      } else {
        listP->next_ul[prev_i] = nodejP;
      }
    } else if (next_j == nodeiP) {  // case ... p(j) j i n(i) ... => ... p(i) i j n(j) ...
      LOG_T(MAC,"[UL]Case ... p(j) j i n(i) ... => ... p(i) i j n(j) ...\n");
      listP->next_ul[nodejP] = next_i;
      listP->next_ul[nodeiP] = nodejP;

      if (nodejP==listP->head_ul) { // case j i n(i)
        listP->head_ul = nodeiP;
      } else {
        listP->next_ul[prev_j] = nodeiP;
      }
    } else {  // case ...  p(i) i n(i) ... p(j) j n(j) ...

      listP->next_ul[nodejP] = next_i;
      listP->next_ul[nodeiP] = next_j;


      if (nodeiP==listP->head_ul) {
        LOG_T(MAC,"[UL]changing head to %d\n",nodejP);
        listP->head_ul=nodejP;
        listP->next_ul[prev_j] = nodeiP;
      } else if (nodejP==listP->head_ul) {
        LOG_T(MAC,"[UL]changing head to %d\n",nodeiP);
        listP->head_ul=nodeiP;
        listP->next_ul[prev_i] = nodejP;
      } else {
        listP->next_ul[prev_i] = nodejP;
        listP->next_ul[prev_j] = nodeiP;
      }
    }
  }

  LOG_T(MAC,"After swap\n");
  dump_ue_list(listP,ul_flag);
}








/*
  #if defined(Rel10) || defined(Rel14)
  unsigned char generate_mch_header( unsigned char *mac_header,
  unsigned char num_sdus,
  unsigned short *sdu_lengths,
  unsigned char *sdu_lcids,
  unsigned char msi,
  unsigned char short_padding,
  unsigned short post_padding) {

  SCH_SUBHEADER_FIXED *mac_header_ptr = (SCH_SUBHEADER_FIXED *)mac_header;
  uint8_t first_element=0,last_size=0,i;
  uint8_t mac_header_control_elements[2*num_sdus],*ce_ptr;

  ce_ptr = &mac_header_control_elements[0];

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

  // SUBHEADER for MSI CE
  if (msi != 0) {// there is MSI MAC Control Element
  if (first_element>0) {
  mac_header_ptr->E = 1;
  mac_header_ptr+=last_size;
  }
  else {
  first_element = 1;
  }
  if (num_sdus*2 < 128) {
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->R    = 0;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->E    = 0;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F    = 0;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->LCID = MCH_SCHDL_INFO;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L    = num_sdus*2;
  last_size=2;
  }
  else {
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->R    = 0;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->E    = 0;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->F    = 1;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->LCID = MCH_SCHDL_INFO;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L    = (num_sdus*2)&0x7fff;
  last_size=3;
  }
  // Create the MSI MAC Control Element here
  }

  // SUBHEADER for MAC SDU (MCCH+MTCHs)
  for (i=0;i<num_sdus;i++) {
  if (first_element>0) {
  mac_header_ptr->E = 1;
  mac_header_ptr+=last_size;
  }
  else {
  first_element = 1;
  }
  if (sdu_lengths[i] < 128) {
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->R    = 0;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->E    = 0;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F    = 0;
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->LCID = sdu_lcids[i];
  ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L    = (unsigned char)sdu_lengths[i];
  last_size=2;
  }
  else {
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->R    = 0;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->E    = 0;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->F    = 1;
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->LCID = sdu_lcids[i];
  ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L    = (unsigned short) sdu_lengths[i]&0x7fff;
  last_size=3;
  }
  }

  if (post_padding>0) {// we have lots of padding at the end of the packet
  mac_header_ptr->E = 1;
  mac_header_ptr+=last_size;
  // add a padding element
  mac_header_ptr->R    = 0;
  mac_header_ptr->E    = 0;
  mac_header_ptr->LCID = SHORT_PADDING;
  mac_header_ptr++;
  }
  else { // no end of packet padding
  // last SDU subhead is of fixed type (sdu length implicitly to be computed at UE)
  mac_header_ptr++;
  }

  // Copy MSI Control Element to the end of the MAC Header if it presents
  if ((ce_ptr-mac_header_control_elements) > 0) {
  // printf("Copying %d bytes for control elements\n",ce_ptr-mac_header_control_elements);
  memcpy((void*)mac_header_ptr,mac_header_control_elements,ce_ptr-mac_header_control_elements);
  mac_header_ptr+=(unsigned char)(ce_ptr-mac_header_control_elements);
  }

  return((unsigned char*)mac_header_ptr - mac_header);
  }
  #endif
 */



// This has to be updated to include BSR information
uint8_t UE_is_to_be_scheduled(module_id_t module_idP,int CC_id,uint8_t UE_id)
{

  UE_TEMPLATE *UE_template    = &RC.mac[module_idP]->UE_list.UE_template[CC_id][UE_id];
  UE_sched_ctrl *UE_sched_ctl = &RC.mac[module_idP]->UE_list.UE_sched_ctrl[UE_id];


  // do not schedule UE if UL is not working
  if (UE_sched_ctl->ul_failure_timer>0)
    return(0);
  if (UE_sched_ctl->ul_out_of_sync>0)
    return(0);

  LOG_D(MAC,"[eNB %d][PUSCH] Checking UL requirements UE %d/%x\n",module_idP,UE_id,UE_RNTI(module_idP,UE_id));

  if ((UE_template->bsr_info[LCGID0]>0) ||
      (UE_template->bsr_info[LCGID1]>0) ||
      (UE_template->bsr_info[LCGID2]>0) ||
      (UE_template->bsr_info[LCGID3]>0) ||
      (UE_template->ul_SR>0) || // uplink scheduling request
      ((UE_sched_ctl->ul_inactivity_timer>20)&&
       (UE_sched_ctl->ul_scheduled==0))||  // every 2 frames when RRC_CONNECTED
      ((UE_sched_ctl->ul_inactivity_timer>10)&&
       (UE_sched_ctl->ul_scheduled==0)&&
       (mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP,UE_id)) < RRC_CONNECTED))) // every Frame when not RRC_CONNECTED
    { 

    LOG_D(MAC,"[eNB %d][PUSCH] UE %d/%x should be scheduled\n",module_idP,UE_id,UE_RNTI(module_idP,UE_id));
    return(1);
  } else {
    return(0);
  }
}


uint8_t get_tmode(module_id_t module_idP,int CC_idP,int UE_idP) {

  eNB_MAC_INST         *eNB                                = RC.mac[module_idP];
  COMMON_channels_t    *cc                                 = &eNB->common_channels[CC_idP];

  struct PhysicalConfigDedicated  *physicalConfigDedicated = eNB->UE_list.physicalConfigDedicated[CC_idP][UE_idP];

  if (physicalConfigDedicated == NULL ) { // RRCConnectionSetup not received by UE yet
    AssertFatal(cc->p_eNB<=2,"p_eNB is %d, should be <2\n",cc->p_eNB);
    return(cc->p_eNB);
  }
  else {
    AssertFatal(physicalConfigDedicated->antennaInfo!=NULL,
		"antennaInfo is null for CCId %d, UEid %d\n",CC_idP,UE_idP);
    
    AssertFatal(physicalConfigDedicated->antennaInfo->present != PhysicalConfigDedicated__antennaInfo_PR_NOTHING, 
		"antennaInfo (mod_id %d, CC_id %d) is set to NOTHING\n",module_idP,CC_idP);
    
    if (physicalConfigDedicated->antennaInfo->present == PhysicalConfigDedicated__antennaInfo_PR_explicitValue) {
      return(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode);
    }
    else if (physicalConfigDedicated->antennaInfo->present == PhysicalConfigDedicated__antennaInfo_PR_defaultValue) {
      AssertFatal(cc->p_eNB<=2,"p_eNB is %d, should be <2\n",cc->p_eNB);
      return(cc->p_eNB);
    }
    else AssertFatal(1==0,"Shouldn't be here\n");
  }
}

int8_t get_ULharq(module_id_t module_idP,int CC_idP,uint16_t frameP,uint8_t subframeP) {

  uint8_t           ret       = -1;
  eNB_MAC_INST      *eNB      = RC.mac[module_idP];
  COMMON_channels_t *cc       = &eNB->common_channels[CC_idP];

  if (cc->tdd_Config==NULL) { // FDD
    ret = (((frameP<<1)+subframeP)&7);
  } else {

    switch (cc->tdd_Config->subframeAssignment) {

    case 1:
      if ((subframeP==2) ||
          (subframeP==3) ||
          (subframeP==7) ||
          (subframeP==8))
        switch (subframeP) {
        case 2:
        case 3:
          ret = (subframeP-2);
          break;

        case 7:
        case 8:
          ret = (subframeP-5);
          break;

        default:
          AssertFatal(1==0,"subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframeP,(int)cc->tdd_Config->subframeAssignment);
          break;
        }

      break;

    case 2:
      AssertFatal((subframeP==2) || (subframeP==7),
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframeP,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframeP/7);
      break;

    case 3:
      AssertFatal((subframeP>1) && (subframeP<5),
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframeP,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframeP-2);
      break;

    case 4:
      AssertFatal((subframeP>1) && (subframeP<4),
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframeP,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframeP-2);
      break;

    case 5:
      AssertFatal(subframeP==2,
		  "subframe2_harq_pid, Illegal subframe %d for TDD mode %d\n",subframeP,(int)cc->tdd_Config->subframeAssignment);
      ret = (subframeP-2);
      break;

    default:
      AssertFatal(1==0,"subframe2_harq_pid, Unsupported TDD mode %d\n",(int)cc->tdd_Config->subframeAssignment);
      break;
    }
  }

  AssertFatal(ret!=-1,
	      "invalid harq_pid(%d) at SFN/SF = %d/%d\n", (int8_t)ret, frameP, subframeP);
  return ret;
}


uint16_t getRIV(uint16_t N_RB_DL,uint16_t RBstart,uint16_t Lcrbs)
{

  uint16_t RIV;

  if (Lcrbs<=(1+(N_RB_DL>>1)))
    RIV = (N_RB_DL*(Lcrbs-1)) + RBstart;
  else
    RIV = (N_RB_DL*(N_RB_DL+1-Lcrbs)) + (N_RB_DL-1-RBstart);

  return(RIV);
}

uint32_t allocate_prbs(int UE_id,unsigned char nb_rb, int N_RB_DL, uint32_t *rballoc)
{

  int i;
  uint32_t rballoc_dci=0;
  unsigned char nb_rb_alloc=0;

  for (i=0; i<(N_RB_DL-2); i+=2) {
    if (((*rballoc>>i)&3)==0) {
      *rballoc |= (3<<i);
      rballoc_dci |= (1<<((12-i)>>1));
      nb_rb_alloc+=2;
    }

    if (nb_rb_alloc==nb_rb) {
      return(rballoc_dci);
    }
  }

  if ((N_RB_DL&1)==1) {
    if ((*rballoc>>(N_RB_DL-1)&1)==0) {
      *rballoc |= (1<<(N_RB_DL-1));
      rballoc_dci |= 1;
    }
  }

  return(rballoc_dci);
}

int get_bw_index(module_id_t module_id, uint8_t CC_id)
{

  int bw_index=0;
 
  int N_RB_DL = to_prb(RC.mac[module_id]->common_channels[CC_id].mib->message.dl_Bandwidth);

  switch (N_RB_DL) {
  case 6: // 1.4 MHz
    bw_index=0;
    break;

  case 25: // 5HMz
    bw_index=1;
    break;

  case 50: // 10HMz
    bw_index=2;
    break;

  case 100: // 20HMz
    bw_index=3;
    break;

  default:
    bw_index=1;
    LOG_W(MAC,"[eNB %d] N_RB_DL %d unknown for CC_id %d, setting bw_index to 1\n", module_id, N_RB_DL,CC_id);
    break;
  }

  return bw_index;
}

int get_min_rb_unit(module_id_t module_id, uint8_t CC_id)
{

  int min_rb_unit=0;
  int N_RB_DL = to_prb(RC.mac[module_id]->common_channels[CC_id].mib->message.dl_Bandwidth);

  switch (N_RB_DL) {
  case 6: // 1.4 MHz
    min_rb_unit=1;
    break;

  case 25: // 5HMz
    min_rb_unit=2;
    break;

  case 50: // 10HMz
    min_rb_unit=3;
    break;

  case 100: // 20HMz
    min_rb_unit=4;
    break;

  default:
    min_rb_unit=2;
    LOG_W(MAC,"[eNB %d] N_DL_RB %d unknown for CC_id %d, setting min_rb_unit to 2\n",
          module_id, N_RB_DL, CC_id);
    break;
  }

  return min_rb_unit;
}

uint32_t allocate_prbs_sub(int nb_rb, int N_RB_DL, int N_RBG, uint8_t *rballoc)
{

  int check=0;//check1=0,check2=0;
  uint32_t rballoc_dci=0;
  //uint8_t number_of_subbands=13;

  LOG_T(MAC,"*****Check1RBALLOC****: %d%d%d%d (nb_rb %d,N_RBG %d)\n",
        rballoc[3],rballoc[2],rballoc[1],rballoc[0],nb_rb,N_RBG);

  while((nb_rb >0) && (check < N_RBG)) {
    //printf("rballoc[%d] %d\n",check,rballoc[check]);
    if(rballoc[check] == 1) {
      rballoc_dci |= (1<<((N_RBG-1)-check));

      switch (N_RB_DL) {
      case 6:
        nb_rb--;
        break;

      case 25:
        if ((check == N_RBG-1)) {
          nb_rb--;
        } else {
          nb_rb-=2;
        }

        break;

      case 50:
        if ((check == N_RBG-1)) {
          nb_rb-=2;
        } else {
          nb_rb-=3;
        }

        break;

      case 100:
        nb_rb-=4;
        break;
      }
    }

    //      printf("rb_alloc %x\n",rballoc_dci);
    check = check+1;
    //    check1 = check1+2;
  }

  // rballoc_dci = (rballoc_dci)&(0x1fff);
  LOG_T(MAC,"*********RBALLOC : %x\n",rballoc_dci);
  // exit(-1);
  return (rballoc_dci);
}

int get_subbandsize(uint8_t dl_Bandwidth) {
  uint8_t ss[6] = {6,4,4,6,8,8};

  AssertFatal(dl_Bandwidth<6,"dl_Bandwidth %d is out of bounds\n",dl_Bandwidth);

  return(ss[dl_Bandwidth]);
}
int get_nb_subband(int N_RB_DL)
{

  int nb_sb=0;

  switch (N_RB_DL) {
  case 6:
    nb_sb=0;
    break;

  case 15:
    nb_sb = 4;  // sb_size =4

  case 25:
    nb_sb = 7; // sb_size =4, 1 sb with 1PRB, 6 with 2 RBG, each has 2 PRBs
    break;

  case 50:    // sb_size =6
    nb_sb = 9;
    break;

  case 75:  // sb_size =8
    nb_sb = 10;
    break;

  case 100: // sb_size =8 , 1 sb with 1 RBG + 12 sb with 2RBG, each RBG has 4 PRBs
    nb_sb = 13;
    break;

  default:
    nb_sb=0;
    break;
  }

  return nb_sb;

}

void init_CCE_table(int module_idP,int CC_idP)
{
  memset(RC.mac[module_idP]->CCE_table[CC_idP],0,800*sizeof(int));
} 


int get_nCCE_offset(int *CCE_table,
		    const unsigned char L, 
		    const int nCCE, 
		    const int common_dci, 
		    const unsigned short rnti, 
		    const unsigned char subframe)
{

  int search_space_free,m,nb_candidates = 0,l,i;
  unsigned int Yk;
   /*
    printf("CCE Allocation: ");
    for (i=0;i<nCCE;i++)
    printf("%d.",CCE_table[i]);
    printf("\n");
  */
  if (common_dci == 1) {
    // check CCE(0 ... L-1)
    nb_candidates = (L==4) ? 4 : 2;
    nb_candidates = min(nb_candidates,nCCE/L);

    //    printf("Common DCI nb_candidates %d, L %d\n",nb_candidates,L);

    for (m = nb_candidates-1 ; m >=0 ; m--) {

      search_space_free = 1;
      for (l=0; l<L; l++) {

	//	printf("CCE_table[%d] %d\n",(m*L)+l,CCE_table[(m*L)+l]);
        if (CCE_table[(m*L) + l] == 1) {
          search_space_free = 0;
          break;
        }
      }
     
      if (search_space_free == 1) {

	//	printf("returning %d\n",m*L);

        for (l=0; l<L; l++)
          CCE_table[(m*L)+l]=1;
        return(m*L);
      }
    }

    return(-1);

  } else { // Find first available in ue specific search space
    // according to procedure in Section 9.1.1 of 36.213 (v. 8.6)
    // compute Yk
    Yk = (unsigned int)rnti;

    for (i=0; i<=subframe; i++)
      Yk = (Yk*39827)%65537;

    Yk = Yk % (nCCE/L);


    switch (L) {
    case 1:
    case 2:
      nb_candidates = 6;
      break;

    case 4:
    case 8:
      nb_candidates = 2;
      break;

    default:
      DevParam(L, nCCE, rnti);
      break;
    }


    LOG_D(MAC,"rnti %x, Yk = %d, nCCE %d (nCCE/L %d),nb_cand %d\n",rnti,Yk,nCCE,nCCE/L,nb_candidates);

    for (m = 0 ; m < nb_candidates ; m++) {
      search_space_free = 1;

      for (l=0; l<L; l++) {
        if (CCE_table[(((Yk+m)%(nCCE/L))*L) + l] == 1) {
          search_space_free = 0;
          break;
        }
      }

      if (search_space_free == 1) {
        for (l=0; l<L; l++)
          CCE_table[(((Yk+m)%(nCCE/L))*L)+l]=1;

        return(((Yk+m)%(nCCE/L))*L);
      }
    }

    return(-1);
  }
}

void dump_CCE_table(int *CCE_table,const int nCCE,const unsigned short rnti,const int subframe,int L) {

  int nb_candidates = 0,i;
  unsigned int Yk;
  
  printf("CCE 0: ");
  for (i=0;i<nCCE;i++) {
    printf("%1d.",CCE_table[i]);
    if ((i&7) == 7)
      printf("\n CCE %d: ",i);
  }

  Yk = (unsigned int)rnti;
  
  for (i=0; i<=subframe; i++)
    Yk = (Yk*39827)%65537;
  
  Yk = Yk % (nCCE/L);
  
  
  switch (L) {
  case 1:
  case 2:
    nb_candidates = 6;
    break;
    
  case 4:
  case 8:
    nb_candidates = 2;
    break;
    
  default:
    DevParam(L, nCCE, rnti);
    break;
  }
  
  
  printf("rnti %x, Yk*L = %d, nCCE %d (nCCE/L %d),nb_cand*L %d\n",rnti,Yk*L,nCCE,nCCE/L,nb_candidates*L);

}



uint16_t getnquad(COMMON_channels_t *cc, uint8_t num_pdcch_symbols,uint8_t mi)
{

  uint16_t Nreg=0;

  AssertFatal(cc!=NULL,"cc is null\n");
  AssertFatal(cc->mib!=NULL,"cc->mib is null\n");

  int N_RB_DL        = to_prb(cc->mib->message.dl_Bandwidth);
  int phich_resource = get_phich_resource_times6(cc); 

  uint8_t Ngroup_PHICH = (phich_resource*N_RB_DL)/48;

  if (((phich_resource*N_RB_DL)%48) > 0)
    Ngroup_PHICH++;

  if (cc->Ncp == 1) {
    Ngroup_PHICH<<=1;
  }

  Ngroup_PHICH*=mi;

  if ((num_pdcch_symbols>0) && (num_pdcch_symbols<4))
    switch (N_RB_DL) {
    case 6:
      Nreg=12+(num_pdcch_symbols-1)*18;
      break;

    case 25:
      Nreg=50+(num_pdcch_symbols-1)*75;
      break;

    case 50:
      Nreg=100+(num_pdcch_symbols-1)*150;
      break;

    case 100:
      Nreg=200+(num_pdcch_symbols-1)*300;
      break;

    default:
      return(0);
    }

  //   printf("Nreg %d (%d)\n",Nreg,Nreg - 4 - (3*Ngroup_PHICH));
  return(Nreg - 4 - (3*Ngroup_PHICH));
}

uint16_t getnCCE(COMMON_channels_t *cc, uint8_t num_pdcch_symbols, uint8_t mi)
{
  AssertFatal(cc!=NULL,"cc is null\n");
  return(getnquad(cc,num_pdcch_symbols,mi)/9);
}

uint8_t getmi(COMMON_channels_t *cc,int subframe) {

  AssertFatal(cc!=NULL,"cc is null\n");

  // for FDD
  if (cc->tdd_Config==NULL) // FDD
    return 1;

  // for TDD
  switch (cc->tdd_Config->subframeAssignment) {

  case 0:
    if ((subframe==0) || (subframe==5))
      return(2);
    else return(1);

    break;

  case 1:
    if ((subframe==0) || (subframe==5))
      return(0);
    else return(1);

    break;

  case 2:
    if ((subframe==3) || (subframe==8))
      return(1);
    else return(0);

    break;

  case 3:
    if ((subframe==0) || (subframe==8) || (subframe==9))
      return(1);
    else return(0);

    break;

  case 4:
    if ((subframe==8) || (subframe==9))
      return(1);
    else return(0);

    break;

  case 5:
    if (subframe==8)
      return(1);
    else return(0);

    break;

  case 6:
    return(1);
    break;

  default:
    return(0);
  }
}

uint16_t get_nCCE_max(COMMON_channels_t *cc, int num_pdcch_symbols,int subframe)
{
  AssertFatal(cc!=NULL,"cc is null\n");
  return(getnCCE(cc,num_pdcch_symbols,
		 getmi(cc,subframe))); 
}
 
// Allocate the CCEs
int allocate_CCEs(int module_idP,
		  int CC_idP,
		  int subframeP,
		  int test_onlyP) {


  int                                 *CCE_table           = RC.mac[module_idP]->CCE_table[CC_idP];
  nfapi_dl_config_request_body_t      *DL_req              = &RC.mac[module_idP]->DL_req[CC_idP].dl_config_request_body;
  nfapi_hi_dci0_request_body_t        *HI_DCI0_req         = &RC.mac[module_idP]->HI_DCI0_req[CC_idP].hi_dci0_request_body;
  nfapi_dl_config_request_pdu_t       *dl_config_pdu       = &DL_req->dl_config_pdu_list[0];
  nfapi_hi_dci0_request_pdu_t         *hi_dci0_pdu         = &HI_DCI0_req->hi_dci0_pdu_list[0];
  int                                 nCCE_max             = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],1,subframeP);
  int fCCE;
  int i,j,idci;
  int nCCE=0;

  LOG_D(MAC,"Allocate CCEs subframe %d, test %d : (DL %d,UL %d)\n",subframeP,test_onlyP,DL_req->number_dci,HI_DCI0_req->number_of_dci);
  DL_req->number_pdcch_ofdm_symbols=1;

try_again:
  init_CCE_table(module_idP,CC_idP);
  nCCE=0;

  for (i=0,idci=0;i<DL_req->number_pdu;i++) {
    // allocate DL common DCIs first
    if ((dl_config_pdu[i].pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)&&
	(dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti_type==2)) {
      LOG_D(MAC,"Trying to allocate COMMON DCI %d/%d (%d,%d) : rnti %x, aggreg %d nCCE %d / %d (num_pdcch_symbols %d)\n",
	    idci,DL_req->number_dci+HI_DCI0_req->number_of_dci,
	    DL_req->number_dci,HI_DCI0_req->number_of_dci,
	    dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti,
	    dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
	    nCCE,nCCE_max, DL_req->number_pdcch_ofdm_symbols);
    
      if (nCCE + (dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level) > nCCE_max) {
	if (DL_req->number_pdcch_ofdm_symbols == 3)
	  goto failed;
	DL_req->number_pdcch_ofdm_symbols++;
	nCCE_max = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],DL_req->number_pdcch_ofdm_symbols,subframeP);
	goto try_again;
      }
      
      // number of CCEs left can potentially hold this allocation
      fCCE = get_nCCE_offset(CCE_table,
			     dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
			     nCCE_max,
			     1,
			     dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti,
			     subframeP);
      if (fCCE == -1) {
	if (DL_req->number_pdcch_ofdm_symbols == 3) {
	  LOG_D(MAC,"subframe %d: Dropping Allocation for RNTI %x\n",
		subframeP,dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti);
	  for (j=0;j<=i;j++){
	    if (dl_config_pdu[j].pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)
	      LOG_D(MAC,"DCI %d/%d (%d,%d) : rnti %x dci format %d, aggreg %d nCCE %d / %d (num_pdcch_symbols %d)\n",
		    j,DL_req->number_dci+HI_DCI0_req->number_of_dci,
		    DL_req->number_dci,HI_DCI0_req->number_of_dci,
		    dl_config_pdu[j].dci_dl_pdu.dci_dl_pdu_rel8.rnti,
		    dl_config_pdu[j].dci_dl_pdu.dci_dl_pdu_rel8.dci_format,
		    dl_config_pdu[j].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
		    nCCE,nCCE_max,DL_req->number_pdcch_ofdm_symbols);
	  }
	  //dump_CCE_table(CCE_table,nCCE_max,subframeP,dci_alloc->rnti,1<<dci_alloc->L);
	  goto failed;
	}
	DL_req->number_pdcch_ofdm_symbols++;
	nCCE_max = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],DL_req->number_pdcch_ofdm_symbols,subframeP);
	goto try_again;
      } // fCCE==-1
      
      // the allocation is feasible, rnti rule passes
      nCCE += dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level;
      LOG_D(MAC,"Allocating at nCCE %d\n",fCCE);
      if (test_onlyP == 0) {
	dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.cce_idx=fCCE;
	LOG_D(MAC,"Allocate COMMON DCI CCEs subframe %d, test %d => L %d fCCE %d\n",subframeP,test_onlyP,dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,fCCE);
      }
      idci++;
    }
  } // for i = 0 ... num_DL_DCIs

  // no try to allocate UL DCIs
  for (i=0;i<HI_DCI0_req->number_of_dci+HI_DCI0_req->number_of_hi;i++) {

    // allocate UL DCIs 
    if (hi_dci0_pdu[i].pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE) {

      LOG_D(MAC,"Trying to allocate format 0 DCI %d/%d (%d,%d) : rnti %x, aggreg %d nCCE %d / %d (num_pdcch_symbols %d)\n",
	    idci,DL_req->number_dci+HI_DCI0_req->number_of_dci,
	    DL_req->number_dci,HI_DCI0_req->number_of_dci,
	    hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.rnti,hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.aggregation_level,
	    nCCE,nCCE_max, DL_req->number_pdcch_ofdm_symbols);
   
      if (nCCE + (hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.aggregation_level) > nCCE_max) {
	if (DL_req->number_pdcch_ofdm_symbols == 3)
	  goto failed;
	DL_req->number_pdcch_ofdm_symbols++;
	nCCE_max = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],DL_req->number_pdcch_ofdm_symbols,subframeP);
	goto try_again;
      }
      
      // number of CCEs left can potentially hold this allocation
      fCCE = get_nCCE_offset(CCE_table,
			     hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.aggregation_level,
			     nCCE_max,
			     1,
			     hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.rnti,
			     subframeP);
      if (fCCE == -1) {
	if (DL_req->number_pdcch_ofdm_symbols == 3) {
	  LOG_D(MAC,"subframe %d: Dropping Allocation for RNTI %x\n",
		subframeP,hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.rnti);
	  for (j=0;j<=i;j++){
	    if (hi_dci0_pdu[j].pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE)
	      LOG_D(MAC,"DCI %d/%d (%d,%d) : rnti %x dci format %d, aggreg %d nCCE %d / %d (num_pdcch_symbols %d)\n",
		    j,DL_req->number_dci+HI_DCI0_req->number_of_dci,
		    DL_req->number_dci,HI_DCI0_req->number_of_dci,
		    hi_dci0_pdu[j].dci_pdu.dci_pdu_rel8.rnti,
		    hi_dci0_pdu[j].dci_pdu.dci_pdu_rel8.dci_format,
		    hi_dci0_pdu[j].dci_pdu.dci_pdu_rel8.aggregation_level,
		    nCCE,nCCE_max,DL_req->number_pdcch_ofdm_symbols);
	  }
	  //dump_CCE_table(CCE_table,nCCE_max,subframeP,dci_alloc->rnti,1<<dci_alloc->L);
	  goto failed;
	}
	DL_req->number_pdcch_ofdm_symbols++;
	nCCE_max = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],DL_req->number_pdcch_ofdm_symbols,subframeP);
	goto try_again;
      } // fCCE==-1
      
      // the allocation is feasible, rnti rule passes
      nCCE += hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.aggregation_level;
      LOG_D(MAC,"Allocating at nCCE %d\n",fCCE);
      if (test_onlyP == 0) {
	hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.cce_index=fCCE;
	LOG_D(MAC,"Allocate CCEs subframe %d, test %d\n",subframeP,test_onlyP);
      }
      idci++;
    }
  } // for i = 0 ... num_UL_DCIs

  for (i=0;i<DL_req->number_pdu;i++) {
    // allocate DL common DCIs first
    if ((dl_config_pdu[i].pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)&&
	(dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti_type==1)) {
      LOG_D(MAC,"Trying to allocate DL UE-SPECIFIC DCI %d/%d (%d,%d) : rnti %x, aggreg %d nCCE %d / %d (num_pdcch_symbols %d)\n",
	    idci,DL_req->number_dci+HI_DCI0_req->number_of_dci,
	    DL_req->number_dci,HI_DCI0_req->number_of_dci,
	    dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti,dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
	    nCCE,nCCE_max, DL_req->number_pdcch_ofdm_symbols);
   
      if (nCCE + (dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level) > nCCE_max) {
	if (DL_req->number_pdcch_ofdm_symbols == 3)
	  goto failed;
	DL_req->number_pdcch_ofdm_symbols++;
	nCCE_max = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],DL_req->number_pdcch_ofdm_symbols,subframeP);
	goto try_again;
      }
      
      // number of CCEs left can potentially hold this allocation
      fCCE = get_nCCE_offset(CCE_table,
			     dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
			     nCCE_max,
			     1,
			     dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti,
			     subframeP);
      if (fCCE == -1) {
	if (DL_req->number_pdcch_ofdm_symbols == 3) {
	  LOG_I(MAC,"subframe %d: Dropping Allocation for RNTI %x\n",
		subframeP,dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti);
	  for (j=0;j<=i;j++){
	    if (dl_config_pdu[j].pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)
	      LOG_I(MAC,"DCI %d/%d (%d,%d) : rnti %x dci format %d, aggreg %d nCCE %d / %d (num_pdcch_symbols %d)\n",
		    j,DL_req->number_dci+HI_DCI0_req->number_of_dci,
		    DL_req->number_dci,HI_DCI0_req->number_of_dci,
		    dl_config_pdu[j].dci_dl_pdu.dci_dl_pdu_rel8.rnti,
		    dl_config_pdu[j].dci_dl_pdu.dci_dl_pdu_rel8.dci_format,
		    dl_config_pdu[j].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
		    nCCE,nCCE_max,DL_req->number_pdcch_ofdm_symbols);
	  }
	  //dump_CCE_table(CCE_table,nCCE_max,subframeP,dci_alloc->rnti,1<<dci_alloc->L);
	  goto failed;
	}
	DL_req->number_pdcch_ofdm_symbols++;
	nCCE_max = get_nCCE_max(&RC.mac[module_idP]->common_channels[CC_idP],DL_req->number_pdcch_ofdm_symbols,subframeP);
	goto try_again;
      } // fCCE==-1
      
      // the allocation is feasible, rnti rule passes
      nCCE += dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level;
      LOG_D(MAC,"Allocating at nCCE %d\n",fCCE);
      if (test_onlyP == 0) {
	dl_config_pdu[i].dci_dl_pdu.dci_dl_pdu_rel8.cce_idx=fCCE;
	LOG_D(MAC,"Allocate CCEs subframe %d, test %d\n",subframeP,test_onlyP);
      }
      idci++;
    }
  } // for i = 0 ... num_DL_DCIs

  return 0;

failed:
  return -1;
}

boolean_t CCE_allocation_infeasible(int module_idP,
				    int CC_idP,
				    int format_flag,
				    int subframe,
				    int aggregation,
				    int rnti) {

  nfapi_dl_config_request_body_t *DL_req       = &RC.mac[module_idP]->DL_req[CC_idP].dl_config_request_body;
  nfapi_dl_config_request_pdu_t* dl_config_pdu = &DL_req->dl_config_pdu_list[DL_req->number_pdu];
  nfapi_hi_dci0_request_body_t *HI_DCI0_req    = &RC.mac[module_idP]->HI_DCI0_req[CC_idP].hi_dci0_request_body;
  nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu     = &HI_DCI0_req->hi_dci0_pdu_list[HI_DCI0_req->number_of_dci+HI_DCI0_req->number_of_hi]; 
  //DCI_ALLOC_t *dci_alloc;
  int ret;
  boolean_t res=FALSE;

  if (format_flag!=2) { // DL DCI
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti              = rnti;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = aggregation;
    DL_req->number_pdu++;
    ret = allocate_CCEs(module_idP,CC_idP,subframe,1);
    if (ret==-1)
      res = TRUE;
    DL_req->number_pdu--;
  }
  else if (format_flag == 2) { // ue-specific UL DCI
    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti             = rnti;
    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
    HI_DCI0_req->number_of_dci++;
    ret = allocate_CCEs(module_idP,CC_idP,subframe,1);
    if (ret==-1)
      res = TRUE;
    HI_DCI0_req->number_of_dci--;
  }
  return(res);
}

void SR_indication(module_id_t mod_idP, int cc_idP, frame_t frameP, rnti_t rntiP, sub_frame_t subframeP)
{
 
  int UE_id = find_UE_id(mod_idP, rntiP);
  UE_list_t *UE_list = &RC.mac[mod_idP]->UE_list;
 
  if (UE_id  != -1) {
    if (mac_eNB_get_rrc_status(mod_idP,UE_RNTI(mod_idP,UE_id)) < RRC_CONNECTED)
      LOG_D(MAC,"[eNB %d][SR %x] Frame %d subframeP %d Signaling SR for UE %d on CC_id %d\n",mod_idP,rntiP,frameP,subframeP, UE_id,cc_idP);
    UE_list->UE_template[cc_idP][UE_id].ul_SR = 1;
    UE_list->UE_template[cc_idP][UE_id].ul_active = TRUE;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SR_INDICATION,1);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SR_INDICATION,0);
  } else {
    //     AssertFatal(0, "find_UE_id(%u,rnti %d) not found", enb_mod_idP, rntiP);
    //    AssertError(0, 0, "Frame %d: find_UE_id(%u,rnti %d) not found\n", frameP, enb_mod_idP, rntiP);
    LOG_D(MAC,"[eNB %d][SR %x] Frame %d subframeP %d Signaling SR for UE %d (unknown UEid) on CC_id %d\n",mod_idP,rntiP,frameP,subframeP, UE_id,cc_idP);
  }
}

void UL_failure_indication(module_id_t mod_idP, int cc_idP, frame_t frameP, rnti_t rntiP, sub_frame_t subframeP)
{

  int UE_id = find_UE_id(mod_idP, rntiP);
  UE_list_t *UE_list = &RC.mac[mod_idP]->UE_list;

  if (UE_id  != -1) {
    LOG_I(MAC,"[eNB %d][UE %d/%x] Frame %d subframeP %d Signaling UL Failure for UE %d on CC_id %d (timer %d)\n",
	  mod_idP,UE_id,rntiP,frameP,subframeP, UE_id,cc_idP,
	  UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);
    if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer == 0)
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer=1;
  } else {
    //     AssertFatal(0, "find_UE_id(%u,rnti %d) not found", enb_mod_idP, rntiP);
    //    AssertError(0, 0, "Frame %d: find_UE_id(%u,rnti %d) not found\n", frameP, enb_mod_idP, rntiP);
    LOG_W(MAC,"[eNB %d][SR %x] Frame %d subframeP %d Signaling UL Failure for UE %d (unknown UEid) on CC_id %d\n",mod_idP,rntiP,frameP,subframeP, UE_id,cc_idP);
  }
}
