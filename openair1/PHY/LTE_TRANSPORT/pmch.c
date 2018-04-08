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

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "PHY/sse_intrin.h"

// Mask for identifying subframe for MBMS
#define MBSFN_TDD_SF3 0x80// for TDD
#define MBSFN_TDD_SF4 0x40
#define MBSFN_TDD_SF7 0x20
#define MBSFN_TDD_SF8 0x10
#define MBSFN_TDD_SF9 0x08

#include "PHY/defs.h"

#define MBSFN_FDD_SF1 0x80// for FDD
#define MBSFN_FDD_SF2 0x40
#define MBSFN_FDD_SF3 0x20
#define MBSFN_FDD_SF6 0x10
#define MBSFN_FDD_SF7 0x08
#define MBSFN_FDD_SF8 0x04



int is_pmch_subframe(uint32_t frame, int subframe, LTE_DL_FRAME_PARMS *frame_parms)
{

  uint32_t period;
  uint8_t i;

  //  LOG_D(PHY,"is_pmch_subframe: frame %d, subframe %d, num_MBSFN_config %d\n",
  //  frame,subframe,frame_parms->num_MBSFN_config);

  for (i=0; i<frame_parms->num_MBSFN_config; i++) {  // we have at least one MBSFN configuration
    period = 1<<frame_parms->MBSFN_config[i].radioframeAllocationPeriod;

    if ((frame % period) == frame_parms->MBSFN_config[i].radioframeAllocationOffset) {
      if (frame_parms->MBSFN_config[i].fourFrames_flag == 0) {
        if (frame_parms->frame_type == FDD) {
          switch (subframe) {

          case 1:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_FDD_SF1) > 0)
              return(1);

            break;

          case 2:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_FDD_SF2) > 0)
              return(1);

            break;

          case 3:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_FDD_SF3) > 0)
              return(1);

            break;

          case 6:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_FDD_SF6) > 0)
              return(1);

            break;

          case 7:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_FDD_SF7) > 0)
              return(1);

            break;

          case 8:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_FDD_SF8) > 0)
              return(1);

            break;
          }
        } else  {
          switch (subframe) {
          case 3:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_TDD_SF3) > 0)
              return(1);

            break;

          case 4:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_TDD_SF4) > 0)
              return(1);

            break;

          case 7:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_TDD_SF7) > 0)
              return(1);

            break;

          case 8:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_TDD_SF8) > 0)
              return(1);

            break;

          case 9:
            if ((frame_parms->MBSFN_config[i].mbsfn_SubframeConfig & MBSFN_TDD_SF9) > 0)
              return(1);

            break;
          }
        }

      } else { // handle 4 frames case

      }
    }
  }

  return(0);
}

void fill_eNB_dlsch_MCH(PHY_VARS_eNB *eNB,int mcs,int ndi,int rvidx)
{

  LTE_eNB_DLSCH_t *dlsch = eNB->dlsch_MCH;
  LTE_DL_FRAME_PARMS *frame_parms=&eNB->frame_parms;

  //  dlsch->rnti   = M_RNTI;
  dlsch->harq_processes[0]->mcs   = mcs;
  //  dlsch->harq_processes[0]->Ndi   = ndi;
  dlsch->harq_processes[0]->rvidx = rvidx;
  dlsch->harq_processes[0]->Nl    = 1;
  dlsch->harq_processes[0]->TBS   = TBStable[get_I_TBS(dlsch->harq_processes[0]->mcs)][frame_parms->N_RB_DL-1];
  //  dlsch->harq_ids[subframe]       = 0;
  dlsch->harq_processes[0]->nb_rb = frame_parms->N_RB_DL;

  switch(frame_parms->N_RB_DL) {
  case 6:
    dlsch->harq_processes[0]->rb_alloc[0] = 0x3f;
    break;

  case 25:
    dlsch->harq_processes[0]->rb_alloc[0] = 0x1ffffff;
    break;

  case 50:
    dlsch->harq_processes[0]->rb_alloc[0] = 0xffffffff;
    dlsch->harq_processes[0]->rb_alloc[1] = 0x3ffff;
    break;

  case 100:
    dlsch->harq_processes[0]->rb_alloc[0] = 0xffffffff;
    dlsch->harq_processes[0]->rb_alloc[1] = 0xffffffff;
    dlsch->harq_processes[0]->rb_alloc[2] = 0xffffffff;
    dlsch->harq_processes[0]->rb_alloc[3] = 0xf;
    break;
  }

  if (eNB->abstraction_flag) {
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].cntl.pmch_flag=1;
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].num_pmch=1; // assumption: there is always one pmch in each SF
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].num_common_dci=0;
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].num_ue_spec_dci=0;
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].dlsch_type[0]=5;// put at the reserved position for PMCH
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].harq_pid[0]=0;
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].ue_id[0]=255;//broadcast
    eNB_transport_info[eNB->Mod_id][eNB->CC_id].tbs[0]=dlsch->harq_processes[0]->TBS>>3;
  }

}


void generate_mch(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,uint8_t *a)
{

  int G;
  int subframe = proc->subframe_tx;
  int frame    = proc->frame_tx;

  if (eNB->abstraction_flag != 0) {
    if (eNB_transport_info_TB_index[eNB->Mod_id][eNB->CC_id]!=0)
      printf("[PHY][EMU] PMCH transport block position is different than zero %d \n", eNB_transport_info_TB_index[eNB->Mod_id][eNB->CC_id]);

    memcpy(eNB->dlsch_MCH->harq_processes[0]->b,
           a,
           eNB->dlsch_MCH->harq_processes[0]->TBS>>3);
    LOG_D(PHY, "[eNB %d] dlsch_encoding_emul pmch , tbs is %d \n",
          eNB->Mod_id,
          eNB->dlsch_MCH->harq_processes[0]->TBS>>3);

    memcpy(&eNB_transport_info[eNB->Mod_id][eNB->CC_id].transport_blocks[eNB_transport_info_TB_index[eNB->Mod_id][eNB->CC_id]],
           a,
           eNB->dlsch_MCH->harq_processes[0]->TBS>>3);
    eNB_transport_info_TB_index[eNB->Mod_id][eNB->CC_id]+= eNB->dlsch_MCH->harq_processes[0]->TBS>>3;//=eNB_transport_info[eNB->Mod_id].tbs[0];
  } else {
    G = get_G(&eNB->frame_parms,
              eNB->frame_parms.N_RB_DL,
              eNB->dlsch_MCH->harq_processes[0]->rb_alloc,
              get_Qm(eNB->dlsch_MCH->harq_processes[0]->mcs),1,
              2,proc->frame_tx,subframe,0);

    generate_mbsfn_pilot(eNB,proc,
                         eNB->common_vars.txdataF,
                         AMP);


    AssertFatal(dlsch_encoding(eNB,
		       a,
                       1,
                       eNB->dlsch_MCH,
                       proc->frame_tx,
                       subframe,
                       &eNB->dlsch_rate_matching_stats,
                       &eNB->dlsch_turbo_encoding_stats,
                       &eNB->dlsch_interleaving_stats)==0,
		"problem in dlsch_encoding");

    dlsch_scrambling(&eNB->frame_parms,1,eNB->dlsch_MCH,0,G,0,frame,subframe<<1);


    mch_modulation(eNB->common_vars.txdataF,
                   AMP,
                   subframe,
                   &eNB->frame_parms,
                   eNB->dlsch_MCH);
  }

}

