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

/*! \file pusch_pc.c
 * \brief Implementation of UE PUSCH Power Control procedures from 36.213 LTE specifications (Section
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "sched_UE.h"
#include "SCHED/sched_common_extern.h"
#include "PHY/defs_UE.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "PHY/phy_extern_ue.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "nfapi/oai_integration/vendor_ext.h"


int16_t get_hundred_times_delta_IF(PHY_VARS_UE *ue,uint8_t eNB_id,uint8_t harq_pid) {
  uint32_t Nre = 2*ue->ulsch[eNB_id]->harq_processes[harq_pid]->Nsymb_initial *
                 ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb*12;

  if (Nre==0)
    return(0);

  uint32_t MPR_x100 = 100*ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS/Nre;
  // Note: MPR=is the effective spectral efficiency of the PUSCH
  // FK 20140908 sumKr is only set after the ulsch_encoding
  uint16_t beta_offset_pusch = (ue->ulsch[eNB_id]->harq_processes[harq_pid]->control_only == 1) ?
                               ue->ulsch[eNB_id]->beta_offset_cqi_times8:8;

  if (ue->ul_power_control_dedicated[eNB_id].deltaMCS_Enabled == 1) {
    // This is the formula from Section 5.1.1.1 in 36.213 10*log10(deltaIF_PUSCH = (2^(MPR*Ks)-1)*beta_offset_pusch)
    return(hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_times10((beta_offset_pusch)>>3));
  } else {
    return(0);
  }
}



uint8_t alpha_lut[8] = {0,40,50,60,70,80,90,100};

void pusch_power_cntl(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t j, uint8_t abstraction_flag) {
  uint8_t harq_pid = subframe2harq_pid(&ue->frame_parms,
                                       proc->frame_tx,
                                       proc->subframe_tx);
  uint8_t nb_rb = ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb;
  int16_t PL;
  // P_pusch = 10*log10(nb_rb + P_opusch(j)+ alpha(u)*PL + delta_TF(i) + f(i))
  //
  // P_opusch(0) = P_oPTR + deltaP_Msg3 if PUSCH is transporting Msg3
  // else
  // P_opusch(0) = PO_NOMINAL_PUSCH(j) + P_O_UE_PUSCH(j)
  PL = get_PL(ue->Mod_id,ue->CC_id,eNB_id);
  ue->ulsch[eNB_id]->Po_PUSCH = (hundred_times_log10_NPRB[nb_rb-1]+
                                 get_hundred_times_delta_IF(ue,eNB_id,harq_pid) +
                                 100*ue->ulsch[eNB_id]->f_pusch)/100;

  if(ue->ulsch_Msg3_active[eNB_id] == 1) {  // Msg3 PUSCH
    ue->ulsch[eNB_id]->Po_PUSCH += (get_Po_NOMINAL_PUSCH(ue->Mod_id,0) + PL);
    LOG_I(PHY,"[UE  %d][RAPROC] frame %d, subframe %d: Msg3 Po_PUSCH %d dBm (%d,%d,100*PL=%d,%d,%d)\n",
          ue->Mod_id,proc->frame_tx,proc->subframe_tx,ue->ulsch[eNB_id]->Po_PUSCH,
          100*get_Po_NOMINAL_PUSCH(ue->Mod_id,0),
          hundred_times_log10_NPRB[nb_rb-1],
          100*PL,
          get_hundred_times_delta_IF(ue,eNB_id,harq_pid),
          100*ue->ulsch[eNB_id]->f_pusch);
  } else if (j==0) { // SPS PUSCH
  } else if (j==1) { // Normal PUSCH
    ue->ulsch[eNB_id]->Po_PUSCH +=  ((alpha_lut[ue->frame_parms.ul_power_control_config_common.alpha]*PL)/100);
    ue->ulsch[eNB_id]->Po_PUSCH +=  ue->frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
    ue->ulsch[eNB_id]->PHR       =  ue->tx_power_max_dBm-ue->ulsch[eNB_id]->Po_PUSCH;

    if (ue->ulsch[eNB_id]->PHR < -23)
      ue->ulsch[eNB_id]->PHR = -23;
    else if (ue->ulsch[eNB_id]->PHR > 40)
      ue->ulsch[eNB_id]->PHR = 40;

    LOG_D(PHY,"[UE  %d][PUSCH %d] AbsSubframe %d.%d: nb_rb: %d, Po_PUSCH %d dBm : tx power max %d , Po_NOMINAL_PUSCH %d,log10(NPRB) %f,PHR %d, PL %d, alpha*PL %f,delta_IF %f,f_pusch %d\n",
          ue->Mod_id,harq_pid,proc->frame_tx,proc->subframe_tx,nb_rb,
          ue->ulsch[eNB_id]->Po_PUSCH,
          ue->tx_power_max_dBm,
          ue->frame_parms.ul_power_control_config_common.p0_NominalPUSCH,
          hundred_times_log10_NPRB[nb_rb-1]/100.0,
          ue->ulsch[eNB_id]->PHR,
          PL,
          alpha_lut[ue->frame_parms.ul_power_control_config_common.alpha]*PL/100.0,
          get_hundred_times_delta_IF(ue,eNB_id,harq_pid)/100.0,
          ue->ulsch[eNB_id]->f_pusch);
  }
}

int8_t get_PHR(uint8_t Mod_id, uint8_t CC_id,uint8_t eNB_index) {
  if(NFAPI_MODE!=NFAPI_UE_STUB_PNF)
    return PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->PHR;
  else
    return 40; // l1l2 simulator => ideal conditions
}
