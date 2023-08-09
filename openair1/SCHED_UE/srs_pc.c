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

/*! \file srs_pc.c
 * \brief Implementation of UE SRS Power Control procedures from 36.213 LTE specifications (Section
 * \author H. Bilel
 * \date 2016
 * \version 0.1
 * \company TCL
 * \email: haithem.bilel@alcatelOneTouch.com
 * \note
 * \warning
 */

#include "PHY/defs_UE.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "PHY/phy_extern_ue.h"
#include "PHY/phy_extern.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "openair1/SCHED/sched_common_extern.h"

void srs_power_cntl(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t *pnb_rb_srs, uint8_t abstraction_flag)
{

  int16_t PL;
  int8_t  p0_NominalPUSCH;
  int8_t  p0_UE_PUSCH;
  int8_t  Psrs_offset;
  int16_t P_srs;
  int16_t f_pusch;
  uint8_t alpha;
  uint8_t Msrs = 0;
  

  SOUNDINGRS_UL_CONFIG_DEDICATED *psoundingrs_ul_config_dedicated = &ue->soundingrs_ul_config_dedicated[eNB_id];
  LTE_DL_FRAME_PARMS             *pframe_parms                    = &ue->frame_parms;
  
  uint8_t Bsrs  = psoundingrs_ul_config_dedicated->srs_Bandwidth;
  uint8_t Csrs  = pframe_parms->soundingrs_ul_config_common.srs_BandwidthConfig;
  LOG_D(PHY," SRS Power Control; AbsSubframe %d.%d, eNB_id %d, N_RB_UL %d, srs_Bandwidth %d, srs_BandwidthConfig %d \n",proc->frame_tx,proc->subframe_tx,eNB_id,pframe_parms->N_RB_UL,Bsrs,Csrs);
  
  if (pframe_parms->N_RB_UL < 41)
  {
    Msrs    = Nb_6_40[Csrs][Bsrs];
  } 
  else if (pframe_parms->N_RB_UL < 61)
  {
    Msrs    = Nb_41_60[Csrs][Bsrs];
  } 
  else if (pframe_parms->N_RB_UL < 81)
  {
    Msrs    = Nb_61_80[Csrs][Bsrs];
  } 
  else if (pframe_parms->N_RB_UL <111)
  {
    Msrs    = Nb_81_110[Csrs][Bsrs];
  }

  // SRS Power control 36.213 5.1.3.1
  // P_srs   =  P_srs_offset+ 10log10(Msrs) + P_opusch(j) + alpha*PL + f(i))

  p0_NominalPUSCH = ue->frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
  p0_UE_PUSCH     = ue->ul_power_control_dedicated[eNB_id].p0_UE_PUSCH;
  Psrs_offset     = (ue->ul_power_control_dedicated[eNB_id].pSRS_Offset - 3);


  f_pusch     = ue->ulsch[eNB_id]->f_pusch;
  alpha       = alpha_lut[ue->frame_parms.ul_power_control_config_common.alpha];
  PL          = get_PL(ue->Mod_id,ue->CC_id,eNB_id);

  LOG_D(PHY," SRS Power Control; eNB_id %d, p0_NominalPUSCH %d, p0_UE_PUSCH %d, alpha %d \n",eNB_id,p0_NominalPUSCH,p0_UE_PUSCH,alpha);
  LOG_D(PHY," SRS Power Control; eNB_id %d, pSRS_Offset[dB] %d, Msrs %d, PL %d, f_pusch %d \n",eNB_id,Psrs_offset,Msrs,PL,f_pusch);

  P_srs  = (p0_NominalPUSCH + p0_UE_PUSCH) + Psrs_offset + f_pusch;
  P_srs += (((int32_t)alpha * (int32_t)PL) + hundred_times_log10_NPRB[Msrs-1])/100 ;
  
  ue->ulsch[eNB_id]->Po_SRS = P_srs;
  if(ue->ulsch[eNB_id]->Po_SRS > ue->tx_power_max_dBm)
  {
      ue->ulsch[eNB_id]->Po_SRS = ue->tx_power_max_dBm;
  }

  pnb_rb_srs[0]             = Msrs;
  LOG_D(PHY," SRS Power Control; eNB_id %d, Psrs_pc[dBm] %d, Pcmax[dBm] %d, Psrs[dBm] %d\n",eNB_id,P_srs,ue->tx_power_max_dBm,ue->ulsch[eNB_id]->Po_SRS);
}
