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

/***********************************************************************
*
* FILENAME    :  srs_modulation_nr_nr.c
*
* MODULE      :
*
* DESCRIPTION :  function to set uplink reference symbols
*                see TS 38211 6.4.1.4 Sounding reference signal
*
************************************************************************/

#include <stdio.h>
#include <math.h>

#define DEFINE_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
#include "PHY/impl_defs_nr.h"
#undef DEFINE_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H

#include "PHY/defs_nr_UE.h"
//#include "extern.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"

#define DEFINE_VARIABLES_SRS_MODULATION_NR_H
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
#undef DEFINE_VARIABLES_SRS_MODULATION_NR_H

/*******************************************************************
*
* NAME :         generate_srs
*
* PARAMETERS :   pointer to resource set
*                pointer to transmit buffer
*                amplitude scaling for this physical signal
*                slot number of transmission
* RETURN :       0  if srs sequence has been successfully generated
*                -1 if sequence can not be properly generated
*
* DESCRIPTION :  generate/map srs symbol into transmit buffer
*                see TS 38211 6.4.1.4 Sounding reference signal
*
*                FFS_TODO_NR
*                Current supported configuration:
*                - single resource per resource set
*                - single port antenna
*                - 1 symbol for SRS
*                - no symbol offset
*                - periodic mode
*                - no hopping
*                - no carrier switching
*                - no antenna switching*
*
*********************************************************************/
int32_t generate_srs_nr(SRS_ResourceSet_t *p_srs_resource_set,
                        NR_DL_FRAME_PARMS *frame_parms,
                        int32_t *txptr,
                        int16_t amp,
                        UE_nr_rxtx_proc_t *proc)
{
  uint8_t n_SRS_cs_max;
  uint8_t u, v_nu;
  uint32_t f_gh = 0;
  SRS_Resource_t *p_SRS_Resource;
  int frame_number = proc->frame_tx;
  int slot_number = proc->nr_slot_tx;
  uint16_t n_SRS, n_SRS_cs_i;
  double alpha_i;
  uint8_t K_TC_p;
  uint16_t n_b[B_SRS_NUMBER], F_b, subcarrier;
  uint8_t N_b, k_0_overbar_p;

  if (p_srs_resource_set->p_srs_ResourceList[0] == NULL) {
    LOG_E(PHY,"generate_srs: No resource associated with the SRS resource set!\n");
    return (-1);
  }
  else {
    if (p_srs_resource_set->number_srs_Resource <= MAX_NR_OF_SRS_RESOURCES_PER_SET) {
      p_SRS_Resource = p_srs_resource_set->p_srs_ResourceList[0];
    }
    else {
      LOG_E(PHY,"generate_srs: resource number of this resource set %d exceeds maximum supported value %d!\n", p_srs_resource_set->number_srs_Resource, MAX_NR_OF_SRS_RESOURCES_PER_SET);
      return (-1);
    }
  }

  if (p_srs_resource_set->resourceType != periodic) {
    LOG_E(PHY,"generate_srs: only SRS periodic is supported up to now!\n");
    return (-1);
  }
  /* get parameters from SRS resource configuration */
  uint8_t B_SRS  = p_SRS_Resource->freqHopping_b_SRS;
  uint8_t C_SRS  = p_SRS_Resource->freqHopping_c_SRS;
  uint8_t b_hop = p_SRS_Resource->freqHopping_b_hop;
  uint8_t K_TC = p_SRS_Resource->transmissionComb;
  uint8_t K_TC_overbar = p_SRS_Resource->combOffset;    /* FFS_TODO_NR is this parameter for K_TC_overbar ?? */
  uint8_t n_SRS_cs = p_SRS_Resource->cyclicShift;
  uint8_t n_ID_SRS = p_SRS_Resource->sequenceId;
  uint8_t n_shift = p_SRS_Resource->freqDomainPosition; /* it adjusts the SRS allocation to align with the common resource block grid in multiples of four */
  uint8_t n_RRC = p_SRS_Resource->freqDomainShift;
  uint8_t groupOrSequenceHopping = p_SRS_Resource->groupOrSequenceHopping;

  uint8_t l_offset = p_SRS_Resource->resourceMapping_startPosition;

  uint16_t T_SRS = srs_period[p_SRS_Resource->SRS_Periodicity];
  uint16_t T_offset = p_SRS_Resource->SRS_Offset;;                   /* FFS_TODO_NR to check interface with RRC */
  uint8_t R = p_SRS_Resource->resourceMapping_repetitionFactor;

  /* TS 38.211 6.4.1.4.1 SRS resource */
  uint8_t N_ap = (uint8_t)p_SRS_Resource->nrof_SrsPorts;            /* antenna port for transmission */
  uint8_t N_symb_SRS = p_SRS_Resource->resourceMapping_nrofSymbols; /* consecutive OFDM symbols */
  uint8_t l0 = N_SYMB_SLOT - 1 - l_offset;                          /* starting position in the time domain */
  uint8_t k_0_p;                                                    /* frequency domain starting position */

  if (N_ap != port1) {
    LOG_E(PHY, "generate_srs: this number of antenna ports %d is not yet supported!\n", N_ap);
    return (-1);
  }
  if (N_symb_SRS != 1) {
    LOG_E(PHY, "generate_srs: this number of srs symbol  %d is not yet supported!\n", N_symb_SRS);
  	return (-1);
  }
  if (groupOrSequenceHopping != neitherHopping) {
    LOG_E(PHY, "generate_srs: sequence hopping is not yet supported!\n");
    return (-1);
  }
  if (R == 0) {
    LOG_E(PHY, "generate_srs: this parameter repetition factor %d is not consistent !\n", R);
    return (-1);
  }
  else if (R > N_symb_SRS) {
    LOG_E(PHY, "generate_srs: R %d can not be greater than N_symb_SRS %d !\n", R, N_symb_SRS);
    return (-1);
  }
  /* see 38211 6.4.1.4.2 Sequence generation */
  if (K_TC == 4) {
    n_SRS_cs_max = 12;
    // delta = 2;     /* delta = log2(K_TC) */
  }
  else if (K_TC == 2) {
    n_SRS_cs_max = 8;
    // delta = 1;     /* delta = log2(K_TC) */
  }
  else {
	LOG_E(PHY, "generate_srs: SRS unknown value for K_TC %d !\n", K_TC);
    return (-1);
  }
  if (n_SRS_cs >= n_SRS_cs_max) {
    LOG_E(PHY, "generate_srs: inconsistent parameter n_SRS_cs %d >=  n_SRS_cs_max %d !\n", n_SRS_cs, n_SRS_cs_max);
    return (-1);
  }
  if (T_SRS == 0) {
    LOG_E(PHY, "generate_srs: inconsistent parameter T_SRS %d can not be equal to zero !\n", T_SRS);
    return (-1);
  }
  else
  {
    int index = 0;
    while (srs_periodicity[index] != T_SRS) {
      index++;
      if (index == SRS_PERIODICITY) {
        LOG_E(PHY, "generate_srs: inconsistent parameter T_SRS %d not specified !\n", T_SRS);
        return (-1);
      }
    }
  }

  uint16_t m_SRS_b = srs_bandwidth_config[C_SRS][B_SRS][0];     /* m_SRS_b is given by TS 38211 clause 6.4.1.4.3 */
  uint16_t M_sc_b_SRS = m_SRS_b * N_SC_RB/K_TC;                 /* length of the sounding reference signal sequence */

  /* for each antenna ports for transmission */
  for (int p_index = 0; p_index < N_ap; p_index++) {

  /* see TS 38.211 6.4.1.4.2 Sequence generation */

    n_SRS_cs_i = (n_SRS_cs +  (n_SRS_cs_max * (SRS_antenna_port[p_index] - 1000)/N_ap))%n_SRS_cs_max;
    alpha_i = 2 * M_PI * ((double)n_SRS_cs_i / (double)n_SRS_cs_max);

    /* for each SRS symbol which should be managed by SRS configuration */
    /* from TS 38.214 6.2.1.1 UE SRS frequency hopping procedure */
    /* A UE may be configured to transmit an SRS resource on  adjacent symbols within the last six symbols of a slot, */
    /* where all antenna ports of the SRS resource are mapped to each symbol of the resource */


    uint8_t l = p_index;
    if (l >= N_symb_SRS) {
      LOG_E(PHY, "generate_srs: number of antenna ports %d and number of srs symbols %d are different !\n", N_ap, N_symb_SRS);
    }

    switch(groupOrSequenceHopping) {
      case neitherHopping:
      {
        f_gh = 0;
        v_nu = 0;
        break;
      }
      case groupHopping:
      {
        uint8_t c_last_index = 8 * (slot_number * N_SYMB_SLOT + l0 + l) + 7; /* compute maximum value of the random sequence */
        uint32_t *c_sequence =  calloc(c_last_index+1, sizeof(uint32_t));
        uint32_t cinit = n_ID_SRS/30;
        pseudo_random_sequence(c_last_index+1,  c_sequence, cinit);
        for (int m = 0; m < 8; m++) {
          f_gh += c_sequence[8 * (slot_number * N_SYMB_SLOT + l0 + l) + m]<<m;
        }
        free(c_sequence);
        f_gh = f_gh%30;
        v_nu = 0;
        break;
      }
      case sequenceHopping:
      {
        f_gh = 0;
        if (M_sc_b_SRS > 6 * N_SC_RB) {
          uint8_t c_last_index = (slot_number * N_SYMB_SLOT + l0 + l); /* compute maximum value of the random sequence */
          uint32_t *c_sequence =  calloc(c_last_index+1, sizeof(uint32_t));
          uint32_t cinit = n_ID_SRS;
          pseudo_random_sequence(c_last_index+1,  c_sequence, cinit);
          v_nu = c_sequence[c_last_index];
          free(c_sequence);
        }
        else {
          v_nu = 0;
        }
        break;
      }
      default:
      {
        LOG_E(PHY, "generate_srs: unknown hopping setting %d !\n", groupOrSequenceHopping);
        return (-1);
      }
    }

    /* u is the sequence-group number defined in section 6.4.1.4.1 */
    u = (f_gh + n_ID_SRS)%U_GROUP_NUMBER; /* 30 */

    /* TS 38.211 6.4.1.4.3  Mapping to physical resources */

    if((n_SRS_cs >= n_SRS_cs_max/2)&&(n_SRS_cs < n_SRS_cs_max)&&(N_ap == 4) && ((SRS_antenna_port[p_index] == 1001) || (SRS_antenna_port[p_index] == 1003))) {
      K_TC_p = (K_TC_overbar + K_TC/2)%K_TC;
    }
    else {
      K_TC_p = K_TC_overbar;
    }

    for (int b = 0; b <= B_SRS; b++) {
      N_b = srs_bandwidth_config[C_SRS][b][1];
      if (b_hop >= B_SRS) {
        n_b[b] = (4 * n_RRC/m_SRS_b)%N_b; /* frequency hopping is disabled and the frequency position index n_b remains constant */
      }
      else {
        if (b == b_hop) {
          N_b = 1;
        }
        /* periodicity and offset */
        if (p_srs_resource_set->resourceType == aperiodic) {
          n_SRS = l/R;
        }
        else {
          int8_t N_slot_frame = frame_parms->slots_per_frame;
          n_SRS = ((N_slot_frame*frame_number + slot_number - T_offset)/T_SRS)*(N_symb_SRS/R)+(l/R);
        }

        uint16_t product_N_b = 1;  /* for b_hop to b-1 */
        for (unsigned int b_prime = b_hop; b_prime < B_SRS; b_prime++) {  /* product for b_hop to b-1 */
          if (b_prime != b_hop) {
            product_N_b *= srs_bandwidth_config[C_SRS][b_prime][1];
          }
        }

        if (N_b & 1) {      /* Nb odd */
          F_b = (N_b/2)*(n_SRS/product_N_b);
        }
        else {              /* Nb even */
          uint16_t product_N_b_B_SRS = product_N_b;
          product_N_b_B_SRS *= srs_bandwidth_config[C_SRS][B_SRS][1]; /* product for b_hop to b */
          F_b = (N_b/2)*((n_SRS%product_N_b_B_SRS)/product_N_b) + ((n_SRS%product_N_b_B_SRS)/2*product_N_b);
        }

        if (b <= b_hop) {
          n_b[b] = (4 * n_RRC/m_SRS_b)%N_b;
        }
        else {
          n_b[b] = (F_b + (4 * n_RRC/m_SRS_b))%N_b;
        }
      }
    }

    k_0_overbar_p = n_shift * N_SC_RB + K_TC_p;

    k_0_p = k_0_overbar_p; /* it is the frequency-domain starting position */

    for (int b = 0; b <= B_SRS; b++) {
      k_0_p += K_TC * M_sc_b_SRS * n_b[b];
    }

    subcarrier = (frame_parms->first_carrier_offset) + k_0_p;
    if (subcarrier>frame_parms->ofdm_symbol_size) {
      subcarrier -= frame_parms->ofdm_symbol_size;
    }

    /* find index of table which is for this srs length */
    unsigned int M_sc_b_SRS_index = 0;
    while((ul_allocated_re[M_sc_b_SRS_index] != M_sc_b_SRS) && (M_sc_b_SRS_index < SRS_SB_CONF)){
      M_sc_b_SRS_index++;
    }

    if (ul_allocated_re[M_sc_b_SRS_index] != M_sc_b_SRS) {
      LOG_E(PHY, "generate_srs: srs uplink allocation %d can not be found! \n", M_sc_b_SRS);
      return (-1);
    }

#if 0

    char output_file[255];
    char sequence_name[255];
    sprintf(output_file, "rv_seq_%d_%d_%d.m", u, v_nu, ul_allocated_re[M_sc_b_SRS_index]);
    sprintf(sequence_name, "rv_seq_%d_%d_%d", u, v_nu, ul_allocated_re[M_sc_b_SRS_index]);
    write_output(output_file, sequence_name,  rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index], ul_allocated_re[M_sc_b_SRS_index], 1, 1);

#endif

    for (int k=0; k < M_sc_b_SRS; k++) {

      double real_shift = cos(alpha_i*k);
      double imag_shift = sin(alpha_i*k);

      /* cos(x + y) = cos(x)cos(y) - sin(x)sin(y) */
      double real = ((real_shift*rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index][2*k]) - (imag_shift*rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index][2*k+1]))/sqrt(N_ap);
      /* sin(x + y) = sin(x)cos(y) + cos(x)sin(y) */
      double imag = ((imag_shift*rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index][2*k]) + (real_shift*rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index][2*k+1]))/sqrt(N_ap);

      int32_t real_amp = ((int32_t)round((double) amp * real)) >> 15;
      int32_t imag_amp = ((int32_t)round((double) amp * imag)) >> 15;

#if 0
      printf("subcarrier %5d srs[%3d] r: %4d i: %4d reference[%d][%d][%d] r: %6d i: %6d\n", subcarrier, k, real_amp, imag_amp,
                                                                                            u, v_nu, M_sc_b_SRS_index,
                                                                                            rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index][2*k],
                                                                                            rv_ul_ref_sig[u][v_nu][M_sc_b_SRS_index][2*k+1]);
#endif

      txptr[subcarrier] = (real_amp & 0xFFFF) + ((imag_amp<<16)&0xFFFF0000);

      subcarrier += (K_TC); /* subcarrier increment */

      if (subcarrier >= frame_parms->ofdm_symbol_size)
        subcarrier=subcarrier-frame_parms->ofdm_symbol_size;
    }
    /* process next symbol */
    txptr = txptr + frame_parms->ofdm_symbol_size;
  }

  return (0);
}

/*******************************************************************
*
* NAME :         is_srs_period_nr
*
* PARAMETERS :   pointer to resource set
*                pointer to transmit buffer
*                amplitude scaling for this physical signal
*                slot number of transmission
* RETURN :        0  if it is a valid slot for transmitting srs
*                -1 if srs should not be transmitted
*
* DESCRIPTION :  for periodic,
*
*********************************************************************/
int is_srs_period_nr(SRS_Resource_t *p_SRS_Resource, NR_DL_FRAME_PARMS *frame_parms, int frame_tx, int slot_tx)
{
  uint16_t T_SRS = srs_period[p_SRS_Resource->SRS_Periodicity];
  uint16_t T_offset = p_SRS_Resource->SRS_Offset; /* FFS_TODO_NR to check interface */

  if (T_offset > T_SRS) {
    LOG_E(PHY,"is_srs_occasion_nr: T_offset %d is greater than T_SRS %d!\n", T_offset, T_SRS);
    return (-1);
  }

  int16_t N_slot_frame = frame_parms->slots_per_frame;
  if ((N_slot_frame*frame_tx + slot_tx - T_offset)%T_SRS == 0) {
    return (0);
  }
  else {
    return (-1);
  }
}

/*******************************************************************
*
* NAME :         ue_srs_procedure_nr
*
* PARAMETERS :   pointer to ue context
*                pointer to rxtx context*
*
* RETURN :        0 if it is a valid slot for transmitting srs
*                -1 if srs should not be transmitted
*
* DESCRIPTION :  ue srs procedure
*                send srs according to current configuration
*
*********************************************************************/
int ue_srs_procedure_nr(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t eNB_id)
{
  NR_DL_FRAME_PARMS *frame_parms = &(ue->frame_parms);
  SRS_NR *p_srs_nr = &(ue->frame_parms.srs_nr);
  SRS_ResourceSet_t *p_srs_resource_set = frame_parms->srs_nr.p_SRS_ResourceSetList[p_srs_nr->active_srs_Resource_Set];
  int generate_srs = 0;

  /* is there any resource set which has been configurated ? */
  if (p_srs_nr->number_srs_Resource_Set != 0) {

    /* what is the current active resource set ? */
    if (p_srs_nr->active_srs_Resource_Set > MAX_NR_OF_SRS_RESOURCE_SET) {
      LOG_W(PHY,"phy_procedures_UE_TX: srs active %d greater than maximum %d!\n", p_srs_nr->active_srs_Resource_Set, MAX_NR_OF_SRS_RESOURCE_SET);
    }
    else {
      /* SRS resource set configurated ? */
      if (p_srs_resource_set != NULL) {

        SRS_Resource_t *p_srs_resource = frame_parms->srs_nr.p_SRS_ResourceSetList[p_srs_nr->active_srs_Resource_Set]->p_srs_ResourceList[0];

        /* SRS resource configurated ? */
        if (p_srs_resource != NULL) {
          if (p_srs_resource_set->resourceType == periodic) {
            if (is_srs_period_nr(p_srs_resource, frame_parms, proc->frame_tx, proc->nr_slot_tx) == 0) {
              generate_srs = 1;
            }
          }
        }
        else {
          LOG_W(PHY,"phy_procedures_UE_TX: no configurated srs resource!\n");
        }
      }
    }
  }
  if (generate_srs == 1) {
    int16_t txptr = AMP;
    uint16_t nsymb = (ue->frame_parms.Ncp==0) ? 14:12;
    uint16_t symbol_offset = (int)ue->frame_parms.ofdm_symbol_size*((proc->nr_slot_tx*nsymb)+(nsymb-1));
    if (generate_srs_nr(p_srs_resource_set, frame_parms, &ue->common_vars.txdataF[eNB_id][symbol_offset], txptr, proc) == 0) {
      return 0;
    }
    else
    {
      return (-1);
    }
  }
  else {
    return (-1);
  }
}














