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

#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "transport_eNB.h"
#include "transport_proto.h"
#include "transport_common_proto.h"
// Mask for identifying subframe for MBMS
#define MBSFN_TDD_SF3 0x80// for TDD
#define MBSFN_TDD_SF4 0x40
#define MBSFN_TDD_SF7 0x20
#define MBSFN_TDD_SF8 0x10
#define MBSFN_TDD_SF9 0x08



#define MBSFN_FDD_SF1 0x80// for FDD
#define MBSFN_FDD_SF2 0x40
#define MBSFN_FDD_SF3 0x20
#define MBSFN_FDD_SF6 0x10
#define MBSFN_FDD_SF7 0x08
#define MBSFN_FDD_SF8 0x04




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

}


void generate_mch(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,uint8_t *a)
{

  int G;
  int subframe = proc->subframe_tx;
  int frame    = proc->frame_tx;

  G = get_G(&eNB->frame_parms,
	    eNB->frame_parms.N_RB_DL,
	    eNB->dlsch_MCH->harq_processes[0]->rb_alloc,
	    get_Qm(eNB->dlsch_MCH->harq_processes[0]->mcs),1,
	    2,proc->frame_tx,subframe,0);
  
  eNB->dlsch_MCH->harq_processes[0]->Qm = get_Qm(eNB->dlsch_MCH->harq_processes[0]->mcs);
 
  generate_mbsfn_pilot(eNB,proc,
		       eNB->common_vars.txdataF,
		       AMP);
  
  AssertFatal(eNB->dlsch_MCH->harq_processes[0]->pdu != NULL, "attempt to encode a NULL harq PDU\n");
  
  AssertFatal(dlsch_encoding(eNB,
			    // a,
			    eNB->dlsch_MCH->harq_processes[0]->pdu,
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

