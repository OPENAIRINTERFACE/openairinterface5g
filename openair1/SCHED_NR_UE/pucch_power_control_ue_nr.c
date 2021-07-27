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

/************************************************************************
*
* MODULE      :  PUCCH power control for UE NR
*
* DESCRIPTION :  functions related to PUCCH Transmit power
*                TS 38.213 7.2.1 UE behaviour
*
**************************************************************************/

#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#include "SCHED_NR_UE/pucch_power_control_ue_nr.h"
#include <openair1/PHY/LTE_ESTIMATION/lte_estimation.h>
#include <openair1/PHY/NR_UE_ESTIMATION/nr_estimation.h>

/**************** defines **************************************/

/**************** variables **************************************/


/**************** functions **************************************/


/*******************************************************************
*
* NAME :         get_pucch_tx_power_ue
*
* PARAMETERS :   ue context
*                gNB_id identity
*                slots for rx and tx
*                pucch_format_nr_t pucch format
*                nb_of_prbs number of prb allocated to pucch
*                N_sc_ctrl_RB subcarrier control rb related to current pucch format
*                N_symb_PUCCH number of pucch symbols excluding those reserved for dmrs
*                O_UCI number of bits for UCI Uplink Control Information
*                O_SR number of bits for SR scheduling Request
*                O_UCI number of  bits for CSI Channel State Information
*                O_ACK number of bits for HARQ-ACK
*                O_CRC number of bits for CRC
*                n_HARQ_ACK use for obtaining a PUCCH transmission power
*
* RETURN :       pucch power level in dBm
*
* DESCRIPTION :  determines pucch transmission power in dBm
*                TS 38.213 7.2.1 UE behaviour
*
*********************************************************************/

int16_t get_pucch_tx_power_ue(PHY_VARS_NR_UE *ue,
                              uint8_t gNB_id,
                              UE_nr_rxtx_proc_t *proc,
                              pucch_format_nr_t pucch_format,
                              int nb_of_prbs,
                              int N_sc_ctrl_RB,
                              int N_symb_PUCCH,
                              int O_UCI,
                              int O_SR,
                              int O_CSI,
                              int O_ACK,
                              int O_CRC,
                              int n_HARQ_ACK) {

  int16_t P_O_NOMINAL_PUCCH = ue->pucch_config_common_nr[gNB_id].p0_nominal;
  PUCCH_PowerControl_t *power_config = &ue->pucch_config_dedicated_nr[gNB_id].pucch_PowerControl;
  int16_t P_O_UE_PUCCH;
  int16_t G_b_f_c = 0;

  if (ue->pucch_config_dedicated_nr[gNB_id].spatial_Relation_Info[0] != NULL) {  /* FFS TODO NR */
    LOG_E(PHY,"PUCCH Spatial relation infos are not yet implemented : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
    return (PUCCH_POWER_DEFAULT);
  }

  if (power_config->p0_Set[0] != NULL) {
    P_O_UE_PUCCH = power_config->p0_Set[0]->p0_PUCCH_Value; /* get from index 0 if no spatial relation set */
    G_b_f_c = 0;
  }
  else {
    G_b_f_c = ue->dlsch[proc->thread_id][gNB_id][0]->g_pucch;
    LOG_D(PHY,"PUCCH Transmit power control command not yet implemented for NR : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
    return (PUCCH_POWER_DEFAULT);
  }

  int P_O_PUCCH = P_O_NOMINAL_PUCCH + P_O_UE_PUCCH;

  int16_t PL = get_nr_PL(ue->Mod_id, ue->CC_id, gNB_id); /* LTE function because NR path loss not yet implemented FFS TODO NR */

  int16_t delta_F_PUCCH =  power_config->deltaF_PUCCH_f[pucch_format];

  int DELTA_TF;
  uint16_t N_ref_PUCCH;

  /* computing of pucch transmission power adjustment */
  switch (pucch_format) {
    case pucch_format0_nr:
    {
      N_ref_PUCCH = 2;
      DELTA_TF = 10 * log10(N_ref_PUCCH/N_symb_PUCCH);
      break;
    }
    case pucch_format1_nr:
    {
      N_ref_PUCCH = N_SYMB_SLOT;
      DELTA_TF = 10 * log10(N_ref_PUCCH/N_symb_PUCCH);
      break;
    }
    case pucch_format2_nr:
    case pucch_format3_nr:
    case pucch_format4_nr:
    {
      float N_RE = nb_of_prbs * N_sc_ctrl_RB * N_symb_PUCCH;
      float K1 = 6;
      /* initial phase so no higher layer parameters */
      if (ue->UE_mode[gNB_id] != PUSCH) {
        if (O_ACK == 0) {
          n_HARQ_ACK = 0;
        }
        else {
          n_HARQ_ACK = 1;
        }
      }
      if (O_UCI < 12) {

        DELTA_TF = 10 * log10((double)(((K1 * (n_HARQ_ACK + O_SR + O_CSI))/N_RE)));
      }
      else {
       float K2 = 2.4;
       float BPRE = (O_ACK + O_SR + O_CSI + O_CRC)/N_RE;
       DELTA_TF = 10 * log10((double)(pow(2,(K2*BPRE)) - 1));
      }
      break;
    }
    default:
    {
      LOG_E(PHY,"PUCCH unknown pucch format : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
      return (0);
    }
  }

  if (power_config->twoPUCCH_PC_AdjustmentStates > 1) {
    LOG_E(PHY,"PUCCH power control adjustment states with 2 states not yet implemented : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
    return (PUCCH_POWER_DEFAULT);
  }

#if 0
  int k2;

  /* response to a detection by the UE of a DCI format 1_0 or DCI format 1_1 */
  //int K_PUCCH = 0;
  if (O_ACK != 0) {
    /* it assumes that PDCCH is in the first symbol of receive slot FFS TDDO NR */
    //int slots_gap = (proc->nr_slot_tx > proc->nr_slot_rx ? (proc->nr_slot_tx - proc->nr_slot_rx - 1) : ((proc->nr_slot_tx + ue->frame_parms.slots_per_subframe) - proc->nr_slot_rx - 1));
    //K_PUCCH = (slots_gap * (ue->frame_parms.symbols_per_tti)) - 1;
  }
  else {
    /* field k2 is not present - to check k2 of pucch from upper layer FFS TDDO NR */
    if (ue->pusch_config.pusch_TimeDomainResourceAllocation[0] == NULL) {
      if (ue->frame_parms.numerology_index == 0) {
        k2 = 1;
      }
      else {
        k2 = ue->frame_parms.numerology_index;
      }
    }
    else
    {
      /* get minimum value of k2 */
      int i = 0;
      int k2_min = 32;  /* max value of k2 */
      do {
        k2 = ue->pusch_config.pusch_TimeDomainResourceAllocation[i]->k2;
        if (k2 < k2_min) {
          k2_min = k2;
        }
        i++;
        if (i >= MAX_NR_OF_UL_ALLOCATIONS) {
          break;
        }
       } while(ue->pusch_config.pusch_TimeDomainResourceAllocation[i] != NULL);
      k2 = k2_min;
    }
    //K_PUCCH = N_SYMB_SLOT * k2; /* the product of a number of symbols per slot and the minimum of the values provided by higher layer parameter k2 */
  }
#endif

  int contributor = (10 * log10((double)(pow(2,(ue->frame_parms.numerology_index)) * nb_of_prbs)));

  int16_t pucch_power = P_O_PUCCH + contributor + PL + delta_F_PUCCH + DELTA_TF + G_b_f_c;

  if (pucch_power > ue->tx_power_max_dBm) {
    pucch_power = ue->tx_power_max_dBm;
  }

  NR_TST_PHY_PRINTF("PUCCH ( Tx power : %d dBm ) ( 10Log(...) : %d ) ( from Path Loss : %d ) ( delta_F_PUCCH : %d ) ( DELTA_TF : %d ) ( G_b_f_c : %d ) \n",
                                    pucch_power,            contributor,            PL,                    delta_F_PUCCH,    DELTA_TF,        G_b_f_c);

  return (pucch_power);
}

