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

/*! \file PHY/LTE_TRANSPORT/rar_tools.c
* \brief Routine for filling the PUSCH/ULSCH data structures based on a random-access response (RAR) SDU from MAC.  Note this is for eNB. 
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "SCHED/sched_eNB.h"
#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "assertions.h"

extern uint16_t RIV2nb_rb_LUT6[32];
extern uint16_t RIV2first_rb_LUT6[32];
extern uint16_t RIV2nb_rb_LUT25[512];
extern uint16_t RIV2first_rb_LUT25[512];
extern uint16_t RIV2nb_rb_LUT50[1600];
extern uint16_t RIV2first_rb_LUT50[1600];
extern uint16_t RIV2nb_rb_LUT100[6000];
extern uint16_t RIV2first_rb_LUT100[600];

extern uint16_t RIV_max6,RIV_max25,RIV_max50,RIV_max100;

//#define DEBUG_RAR


int generate_eNB_ulsch_params_from_rar(PHY_VARS_eNB *eNB,
				       unsigned char *rar_pdu,
                                       uint32_t frame,
                                       unsigned char subframe){



  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;

  //  RA_HEADER_RAPID *rarh = (RA_HEADER_RAPID *)rar_pdu;
  //  RAR_PDU *rar = (RAR_PDU *)(rar_pdu+1);
  uint8_t *rar = (uint8_t *)(rar_pdu+1);
  uint8_t harq_pid = get_Msg3_harq_pid(frame_parms,frame,subframe);
  uint16_t rballoc;
  uint8_t cqireq;
  uint16_t *RIV2nb_rb_LUT, *RIV2first_rb_LUT;
  uint16_t RIV_max;
  uint16_t use_srs=0;
  uint16_t rnti;
  LTE_eNB_ULSCH_t *ulsch;
  LTE_UL_eNB_HARQ_t *ulsch_harq;
  int8_t UE_id;

  LOG_I(PHY,"[eNB][RAPROC] generate_eNB_ulsch_params_from_rar: subframe %d (harq_pid %d)\n",subframe,harq_pid);

  rnti = (((uint16_t)rar[4])<<8)+rar[5];

  AssertFatal((UE_id = find_ulsch(rnti,eNB,SEARCH_EXIST_OR_FREE))>=0,
	      "Cannot get UE id for RAR (rnti %x)\n",rnti);

  ulsch = eNB->ulsch[UE_id];
  ulsch_harq = ulsch->harq_processes[harq_pid];

  switch (frame_parms->N_RB_DL) {
  case 6:
    RIV2nb_rb_LUT     = &RIV2nb_rb_LUT6[0];
    RIV2first_rb_LUT  = &RIV2first_rb_LUT6[0];
    RIV_max           = RIV_max6;
    break;

  case 25:
    RIV2nb_rb_LUT     = &RIV2nb_rb_LUT25[0];
    RIV2first_rb_LUT  = &RIV2first_rb_LUT25[0];
    RIV_max           = RIV_max25;
    break;

  case 50:
    RIV2nb_rb_LUT     = &RIV2nb_rb_LUT50[0];
    RIV2first_rb_LUT  = &RIV2first_rb_LUT50[0];
    RIV_max           = RIV_max50;
    break;

  case 100:
    RIV2nb_rb_LUT     = &RIV2nb_rb_LUT100[0];
    RIV2first_rb_LUT  = &RIV2first_rb_LUT100[0];
    RIV_max           = RIV_max100;
    break;

  default:
    DevParam(frame_parms->N_RB_DL, harq_pid, 0);
    break;
  }


  rballoc = (((uint16_t)(rar[1]&7))<<7)|(rar[2]>>1);

  if (rballoc>RIV_max) {
    LOG_E(PHY,"[eNB]dci_tools.c: ERROR: rb_alloc (%x)> RIV_max\n",rballoc);
    return(-1);
  }

  ulsch_harq->rar_alloc          = 1;
  ulsch_harq->first_rb           = RIV2first_rb_LUT[rballoc];
  ulsch_harq->nb_rb              = RIV2nb_rb_LUT[rballoc];
  //  ulsch_harq->Ndi                = 1;

  cqireq = rar[3]&1;

  if (cqireq==1) {
    ulsch_harq->Or2                                   = sizeof_wideband_cqi_rank2_2A_5MHz;
    ulsch_harq->Or1                                   = sizeof_wideband_cqi_rank1_2A_5MHz;
    ulsch_harq->O_RI                                  = 1;
  } else {
    ulsch_harq->O_RI                                  = 0;//1;
    ulsch_harq->Or2                                   = 0;
    ulsch_harq->Or1                                   = 0;

  }

  ulsch_harq->O_ACK                                 = 0;//2;
  ulsch->beta_offset_cqi_times8                = 18;
  ulsch->beta_offset_ri_times8                 = 10;
  ulsch->beta_offset_harqack_times8            = 16;

  ulsch->rnti = rnti;
  ulsch->harq_mask = 1<<harq_pid;

  //  if (ulsch_harq->round == 0) {
    ulsch_harq->status = ACTIVE;
    ulsch_harq->rvidx = 0;
    uint8_t mcs               = ((rar[2]&1)<<3)|(rar[3]>>5);
    ulsch_harq->TBS           = TBStable[get_I_TBS_UL(mcs)][ulsch_harq->nb_rb-1];
    ulsch_harq->Qm            = get_Qm_ul(mcs);
    ulsch_harq->Msc_initial   = 12*ulsch_harq->nb_rb;
    ulsch_harq->Nsymb_initial = 9;
    ulsch_harq->round = 0;
    /*  } else {
    ulsch_harq->rvidx = 0;
    ulsch_harq->round++;
    }*/


  ulsch->Msg3_active = 1;
	      
  get_Msg3_alloc(frame_parms,
		 subframe,
		 frame,
		 &ulsch_harq->frame,
		 &ulsch_harq->subframe);

  LOG_I(PHY,"Programming msg3 reception in (%d,%d) mcs:%d TBS:%d Qm:%d Mcs_intial:%d Nsymb_intial:%d round:%d\n",
      ulsch_harq->frame,ulsch_harq->subframe,
      mcs, ulsch_harq->TBS, ulsch_harq->Qm, ulsch_harq->Msc_initial, ulsch_harq->Nsymb_initial, ulsch_harq->round);
  use_srs = is_srs_occasion_common(frame_parms,ulsch_harq->frame,ulsch_harq->subframe);
  ulsch_harq->Nsymb_pusch = 12-(frame_parms->Ncp<<1)-(use_srs==0?0:1);
  ulsch_harq->srs_active                            = use_srs;

#ifdef DEBUG_RAR
  LOG_D(PHY,"ulsch ra (eNB): harq_pid %d\n",harq_pid);
  LOG_D(PHY,"ulsch ra (eNB): NBRB     %d\n",ulsch_harq->nb_rb);
  LOG_D(PHY,"ulsch ra (eNB): rballoc  %x\n",ulsch_harq->first_rb);
  LOG_D(PHY,"ulsch ra (eNB): harq_pid %d\n",harq_pid);
  LOG_D(PHY,"ulsch ra (eNB): round    %d\n",ulsch_harq->round);
  LOG_D(PHY,"ulsch ra (eNB): TBS      %d\n",ulsch_harq->TBS);
  LOG_D(PHY,"ulsch ra (eNB): mcs      %d\n",ulsch_harq->Msc_initial);
  LOG_D(PHY,"ulsch ra (eNB): Or1      %d\n",ulsch_harq->Or1);
  LOG_D(PHY,"ulsch ra (eNB): ORI      %d\n",ulsch_harq->O_RI);
#endif
  return(0);
}

