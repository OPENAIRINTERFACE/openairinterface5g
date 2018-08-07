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

/*! \file PHY/LTE_UE_TRANSPORT/rar_tools_ue.c
* \brief Routine for filling the PUSCH/ULSCH data structures based on a random-access response (RAR) SDU from MAC.  Note this is for UE 
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#include "SCHED_UE/sched_UE.h"
#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "transport_proto_ue.h"

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

int8_t delta_PUSCH_msg2[8] = {-6,-4,-2,0,2,4,6,8};

int generate_ue_ulsch_params_from_rar(PHY_VARS_UE *ue,
				      UE_rxtx_proc_t *proc,
                                      unsigned char eNB_id )
{

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_RAR,VCD_FUNCTION_IN);

  //  RA_HEADER_RAPID *rarh = (RA_HEADER_RAPID *)rar_pdu;
  uint8_t transmission_mode = ue->transmission_mode[eNB_id];
  unsigned char *rar_pdu = ue->dlsch_ra[eNB_id]->harq_processes[0]->b;
  unsigned char subframe = ue->ulsch_Msg3_subframe[eNB_id];
  LTE_UE_ULSCH_t *ulsch  = ue->ulsch[eNB_id];
  PHY_MEASUREMENTS *meas = &ue->measurements;

  LTE_DL_FRAME_PARMS *frame_parms =  &ue->frame_parms;
  //  int current_dlsch_cqi = ue->current_dlsch_cqi[eNB_id];

  uint8_t *rar = (uint8_t *)(rar_pdu+1);
  uint8_t harq_pid = subframe2harq_pid(frame_parms,proc->frame_tx,subframe);
  uint16_t rballoc;
  uint8_t cqireq;
  uint16_t *RIV2nb_rb_LUT, *RIV2first_rb_LUT;
  uint16_t RIV_max = 0;

  LOG_D(PHY,"[eNB][RAPROC] Frame %d: generate_ue_ulsch_params_from_rar: subframe %d (harq_pid %d)\n",proc->frame_tx,subframe,harq_pid);

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
    DevParam(frame_parms->N_RB_DL, eNB_id, harq_pid);
    break;
  }



  ulsch->harq_processes[harq_pid]->TPC                                   = (rar[3]>>2)&7;//rar->TPC;

  rballoc = (((uint16_t)(rar[1]&7))<<7)|(rar[2]>>1);
  cqireq=rar[3]&1;

  if (rballoc>RIV_max) {
    LOG_D(PHY,"rar_tools.c: ERROR: rb_alloc (%x) > RIV_max\n",rballoc);
    return(-1);
  }

  ulsch->harq_processes[harq_pid]->first_rb                              = RIV2first_rb_LUT[rballoc];
  ulsch->harq_processes[harq_pid]->nb_rb                                 = RIV2nb_rb_LUT[rballoc];

  AssertFatal(ulsch->harq_processes[harq_pid]->nb_rb >0, "nb_rb == 0\n");

  ulsch->power_offset = ue_power_offsets[ulsch->harq_processes[harq_pid]->nb_rb];

  AssertFatal(ulsch->harq_processes[harq_pid]->nb_rb <= 6,"unlikely rb count for RAR grant : nb_rb > 6\n");

  //  ulsch->harq_processes[harq_pid]->Ndi                                   = 1;
  if (ulsch->harq_processes[harq_pid]->round == 0)
    ulsch->harq_processes[harq_pid]->status = ACTIVE;

  if (cqireq==1) {
    ulsch->O_RI                                  = 1;

    if (meas->rank[eNB_id] == 1) {
      ulsch->uci_format                          = wideband_cqi_rank2_2A;
      ulsch->O                                   = sizeof_wideband_cqi_rank2_2A_5MHz;
      ulsch->o_RI[0]                             = 1;
    } else {
      ulsch->uci_format                          = wideband_cqi_rank1_2A;
      ulsch->O                                   = sizeof_wideband_cqi_rank1_2A_5MHz;
      ulsch->o_RI[0]                             = 0;
    }

    ulsch->uci_format = HLC_subband_cqi_nopmi;
    fill_CQI(ulsch,meas,eNB_id,0,ue->frame_parms.N_RB_DL,0, transmission_mode,ue->sinr_eff);

    if (((proc->frame_tx % 100) == 0) || (proc->frame_tx < 10))
      print_CQI(ulsch->o,ulsch->uci_format,eNB_id,ue->frame_parms.N_RB_DL);
  } else {
    ulsch->O_RI                                = 0;
    ulsch->O                                   = 0;
  }

  ulsch->harq_processes[harq_pid]->O_ACK                                  = 0;//2;

  ulsch->beta_offset_cqi_times8                  = 18;
  ulsch->beta_offset_ri_times8                   = 10;
  ulsch->beta_offset_harqack_times8              = 16;

  ulsch->Nsymb_pusch                             = 12-(frame_parms->Ncp<<1);
  ulsch->rnti = (((uint16_t)rar[4])<<8)+rar[5];  //rar->t_crnti;

  if (ulsch->harq_processes[harq_pid]->round == 0) {
    ulsch->harq_processes[harq_pid]->status = ACTIVE;
    ulsch->harq_processes[harq_pid]->rvidx = 0;
    ulsch->harq_processes[harq_pid]->mcs         = ((rar[2]&1)<<3)|(rar[3]>>5);
    ulsch->harq_processes[harq_pid]->TPC         = (rar[3]>>2)&7;
    //ulsch->harq_processes[harq_pid]->TBS         = dlsch_tbs25[ulsch->harq_processes[harq_pid]->mcs][ulsch->harq_processes[harq_pid]->nb_rb-1];
    ulsch->harq_processes[harq_pid]->TBS         = TBStable[get_I_TBS_UL(ulsch->harq_processes[harq_pid]->mcs)][ulsch->harq_processes[harq_pid]->nb_rb-1];
    ulsch->harq_processes[harq_pid]->Msc_initial   = 12*ulsch->harq_processes[harq_pid]->nb_rb;
    ulsch->harq_processes[harq_pid]->Nsymb_initial = 9;
    ulsch->harq_processes[harq_pid]->round = 0;
  } else {
    ulsch->harq_processes[harq_pid]->rvidx = 0;
    ulsch->harq_processes[harq_pid]->round++;
  }

  // initialize power control based on PRACH power
  ulsch->f_pusch = delta_PUSCH_msg2[ulsch->harq_processes[harq_pid]->TPC] +
                   get_deltaP_rampup(ue->Mod_id,ue->CC_id);
  LOG_I(PHY,"[UE %d][PUSCH PC] Initializing f_pusch to %d dB, TPC %d (delta_PUSCH_msg2 %d dB), deltaP_rampup %d dB\n",
        ue->Mod_id,ulsch->f_pusch,ulsch->harq_processes[harq_pid]->TPC,delta_PUSCH_msg2[ulsch->harq_processes[harq_pid]->TPC],
        get_deltaP_rampup(ue->Mod_id,ue->CC_id));


  //#ifdef DEBUG_RAR
  LOG_D(PHY,"ulsch ra (UE): harq_pid %d\n",harq_pid);
  LOG_D(PHY,"ulsch ra (UE): NBRB     %d\n",ulsch->harq_processes[harq_pid]->nb_rb);
  LOG_D(PHY,"ulsch ra (UE): first_rb %x\n",ulsch->harq_processes[harq_pid]->first_rb);
  LOG_D(PHY,"ulsch ra (UE): nb_rb    %d\n",ulsch->harq_processes[harq_pid]->nb_rb);
  LOG_D(PHY,"ulsch ra (UE): round    %d\n",ulsch->harq_processes[harq_pid]->round);
  LOG_D(PHY,"ulsch ra (UE): TBS      %d\n",ulsch->harq_processes[harq_pid]->TBS);
  LOG_D(PHY,"ulsch ra (UE): mcs      %d\n",ulsch->harq_processes[harq_pid]->mcs);
  LOG_D(PHY,"ulsch ra (UE): TPC      %d\n",ulsch->harq_processes[harq_pid]->TPC);
  LOG_D(PHY,"ulsch ra (UE): O        %d\n",ulsch->O);
  LOG_D(PHY,"ulsch ra (UE): ORI      %d\n",ulsch->O_RI);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_RAR,VCD_FUNCTION_OUT);

  //#endif
  return(0);
}

