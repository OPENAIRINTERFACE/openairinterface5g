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
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"

#define DEFINE_VARIABLES_SRS_MODULATION_NR_H
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
#undef DEFINE_VARIABLES_SRS_MODULATION_NR_H

//#define SRS_DEBUG

/*******************************************************************
*
* NAME :         generate_srs
*
* PARAMETERS :   pointer to srs config pdu
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
int generate_srs_nr(nfapi_nr_srs_pdu_t *srs_config_pdu,
                    NR_DL_FRAME_PARMS *frame_parms,
                    int32_t *txptr,
                    nr_srs_info_t *nr_srs_info,
                    int16_t amp,
                    int frame_number,
                    int slot_number)
{
  uint8_t n_SRS_cs_max;
  uint8_t u;
  uint8_t v_nu;
  uint32_t f_gh = 0;
  uint16_t n_SRS;
  uint16_t n_SRS_cs_i;
  double alpha_i;
  uint8_t K_TC_p;
  uint16_t n_b[B_SRS_NUMBER];
  uint16_t F_b;
  uint16_t subcarrier;
  uint8_t N_b;
  uint8_t k_0_overbar_p;

  // get parameters from srs_config_pdu
  uint8_t B_SRS  = srs_config_pdu->bandwidth_index;
  uint8_t C_SRS  = srs_config_pdu->config_index;
  uint8_t b_hop = srs_config_pdu->frequency_hopping;
  uint8_t K_TC = 2<<srs_config_pdu->comb_size;
  uint8_t K_TC_overbar = srs_config_pdu->comb_offset;         // FFS_TODO_NR is this parameter for K_TC_overbar ??
  uint8_t n_SRS_cs = srs_config_pdu->cyclic_shift;
  uint8_t n_ID_SRS = srs_config_pdu->sequence_id;
  uint8_t n_shift = srs_config_pdu->frequency_position;       // it adjusts the SRS allocation to align with the common resource block grid in multiples of four
  uint8_t n_RRC = srs_config_pdu->frequency_shift;
  uint8_t groupOrSequenceHopping = srs_config_pdu->group_or_sequence_hopping;
  uint8_t l_offset = srs_config_pdu->time_start_position;
  uint16_t T_SRS = srs_config_pdu->t_srs;
  uint16_t T_offset = srs_config_pdu->t_offset;               // FFS_TODO_NR to check interface with RRC
  uint8_t R = 1<<srs_config_pdu->num_repetitions;
  uint8_t N_ap = 1<<srs_config_pdu->num_ant_ports;            // antenna port for transmission
  uint8_t N_symb_SRS = 1<<srs_config_pdu->num_symbols;        // consecutive OFDM symbols
  uint8_t l0 = frame_parms->symbols_per_slot - 1 - l_offset;  // starting position in the time domain
  uint8_t k_0_p;                                              // frequency domain starting position

  uint64_t subcarrier_offset = frame_parms->first_carrier_offset + srs_config_pdu->bwp_start*N_SC_RB;

  if(nr_srs_info) {
    nr_srs_info->n_symbs = 0;
    nr_srs_info->srs_generated_signal_bits = log2_approx(amp);
  }

#ifdef SRS_DEBUG
  LOG_I(NR_PHY,"Frame = %i, slot = %i\n", frame_number, slot_number);
  LOG_I(NR_PHY,"B_SRS = %i\n", B_SRS);
  LOG_I(NR_PHY,"C_SRS = %i\n", C_SRS);
  LOG_I(NR_PHY,"b_hop = %i\n", b_hop);
  LOG_I(NR_PHY,"K_TC = %i\n", K_TC);
  LOG_I(NR_PHY,"K_TC_overbar = %i\n", K_TC_overbar);
  LOG_I(NR_PHY,"n_SRS_cs = %i\n", n_SRS_cs);
  LOG_I(NR_PHY,"n_ID_SRS = %i\n", n_ID_SRS);
  LOG_I(NR_PHY,"n_shift = %i\n", n_shift);
  LOG_I(NR_PHY,"n_RRC = %i\n", n_RRC);
  LOG_I(NR_PHY,"groupOrSequenceHopping = %i\n", groupOrSequenceHopping);
  LOG_I(NR_PHY,"l_offset = %i\n", l_offset);
  LOG_I(NR_PHY,"T_SRS = %i\n", T_SRS);
  LOG_I(NR_PHY,"T_offset = %i\n", T_offset);
  LOG_I(NR_PHY,"R = %i\n", R);
  LOG_I(NR_PHY,"N_ap = %i\n", N_ap);
  LOG_I(NR_PHY,"N_symb_SRS = %i\n", N_symb_SRS);
  LOG_I(NR_PHY,"l0 = %i\n", l0);
#endif

  if (N_ap != port1) {
    LOG_E(NR_PHY, "generate_srs: this number of antenna ports %d is not yet supported!\n", N_ap);
    return (-1);
  }
  if (N_symb_SRS != 1) {
    LOG_E(NR_PHY, "generate_srs: this number of srs symbol  %d is not yet supported!\n", N_symb_SRS);
  	return (-1);
  }
  if (groupOrSequenceHopping != neitherHopping) {
    LOG_E(NR_PHY, "generate_srs: sequence hopping is not yet supported!\n");
    return (-1);
  }
  if (R == 0) {
    LOG_E(NR_PHY, "generate_srs: this parameter repetition factor %d is not consistent !\n", R);
    return (-1);
  }
  else if (R > N_symb_SRS) {
    LOG_E(NR_PHY, "generate_srs: R %d can not be greater than N_symb_SRS %d !\n", R, N_symb_SRS);
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
	LOG_E(NR_PHY, "generate_srs: SRS unknown value for K_TC %d !\n", K_TC);
    return (-1);
  }
  if (n_SRS_cs >= n_SRS_cs_max) {
    LOG_E(NR_PHY, "generate_srs: inconsistent parameter n_SRS_cs %d >=  n_SRS_cs_max %d !\n", n_SRS_cs, n_SRS_cs_max);
    return (-1);
  }
  if (T_SRS == 0) {
    LOG_E(NR_PHY, "generate_srs: inconsistent parameter T_SRS %d can not be equal to zero !\n", T_SRS);
    return (-1);
  }
  else
  {
    int index = 0;
    while (srs_periodicity[index] != T_SRS) {
      index++;
      if (index == SRS_PERIODICITY) {
        LOG_E(NR_PHY, "generate_srs: inconsistent parameter T_SRS %d not specified !\n", T_SRS);
        return (-1);
      }
    }
  }

  uint16_t m_SRS_b = srs_bandwidth_config[C_SRS][B_SRS][0];     /* m_SRS_b is given by TS 38211 clause 6.4.1.4.3 */
  uint16_t M_sc_b_SRS = m_SRS_b * N_SC_RB/K_TC;                 /* length of the sounding reference signal sequence */

  /* for each antenna ports for transmission */
  for (int p_index = 0; p_index < N_ap; p_index++) {

    uint8_t ofdm_symbol = 0;

  /* see TS 38.211 6.4.1.4.2 Sequence generation */

    n_SRS_cs_i = (n_SRS_cs +  (n_SRS_cs_max * (SRS_antenna_port[p_index] - 1000)/N_ap))%n_SRS_cs_max;
    alpha_i = 2 * M_PI * ((double)n_SRS_cs_i / (double)n_SRS_cs_max);

    /* for each SRS symbol which should be managed by SRS configuration */
    /* from TS 38.214 6.2.1.1 UE SRS frequency hopping procedure */
    /* A UE may be configured to transmit an SRS resource on  adjacent symbols within the last six symbols of a slot, */
    /* where all antenna ports of the SRS resource are mapped to each symbol of the resource */

    uint8_t l = p_index;
    if (l >= N_symb_SRS) {
      LOG_E(NR_PHY, "generate_srs: number of antenna ports %d and number of srs symbols %d are different !\n", N_ap, N_symb_SRS);
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
        LOG_E(NR_PHY, "generate_srs: unknown hopping setting %d !\n", groupOrSequenceHopping);
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
        if (srs_config_pdu->resource_type == aperiodic) {
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

    subcarrier = subcarrier_offset + k_0_p;
    if (subcarrier>frame_parms->ofdm_symbol_size) {
      subcarrier -= frame_parms->ofdm_symbol_size;
    }

    /* find index of table which is for this srs length */
    unsigned int M_sc_b_SRS_index = 0;
    while((ul_allocated_re[M_sc_b_SRS_index] != M_sc_b_SRS) && (M_sc_b_SRS_index < SRS_SB_CONF)){
      M_sc_b_SRS_index++;
    }

    if (ul_allocated_re[M_sc_b_SRS_index] != M_sc_b_SRS) {
      LOG_E(NR_PHY, "generate_srs: srs uplink allocation %d can not be found! \n", M_sc_b_SRS);
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

      txptr[subcarrier+ofdm_symbol*frame_parms->ofdm_symbol_size] = (real_amp & 0xFFFF) + ((imag_amp<<16)&0xFFFF0000);

      if(nr_srs_info) {
        nr_srs_info->subcarrier_idx[nr_srs_info->n_symbs] = subcarrier + ofdm_symbol*frame_parms->ofdm_symbol_size - subcarrier_offset;
        nr_srs_info->n_symbs++;
      }

#ifdef SRS_DEBUG
      if( (subcarrier+ofdm_symbol*frame_parms->ofdm_symbol_size-subcarrier_offset)%12 == 0 ) {
        LOG_I(NR_PHY,"------------ %lu ------------\n",
              (subcarrier+ofdm_symbol*frame_parms->ofdm_symbol_size-subcarrier_offset)/12);
      }
      LOG_I(NR_PHY,"(%lu)  \t%i\t%i\n",
            subcarrier+ofdm_symbol*frame_parms->ofdm_symbol_size-subcarrier_offset,
            real_amp&0xFFFF,
            imag_amp&0xFFFF);
#endif

      subcarrier += (K_TC); /* subcarrier increment */

      if (subcarrier >= frame_parms->ofdm_symbol_size) {
        subcarrier=subcarrier-frame_parms->ofdm_symbol_size;
        ofdm_symbol++;
      }

    }
    /* process next symbol */
    //txptr = txptr + frame_parms->ofdm_symbol_size;
  }

  return (0);
}

/*******************************************************************
*
* NAME :         ue_srs_procedures_nr
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
int ue_srs_procedures_nr(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id)
{

  if(!ue->srs_vars[0]->active) {
    return -1;
  }
  ue->srs_vars[0]->active = false;

  nfapi_nr_srs_pdu_t *srs_config_pdu = (nfapi_nr_srs_pdu_t*)&ue->srs_vars[0]->srs_config_pdu;

#ifdef SRS_DEBUG
  LOG_I(NR_PHY,"Frame = %i, slot = %i\n", proc->frame_tx, proc->nr_slot_tx);
  LOG_I(NR_PHY,"srs_config_pdu->rnti = 0x%04x\n", srs_config_pdu->rnti);
  LOG_I(NR_PHY,"srs_config_pdu->handle = %u\n", srs_config_pdu->handle);
  LOG_I(NR_PHY,"srs_config_pdu->bwp_size = %u\n", srs_config_pdu->bwp_size);
  LOG_I(NR_PHY,"srs_config_pdu->bwp_start = %u\n", srs_config_pdu->bwp_start);
  LOG_I(NR_PHY,"srs_config_pdu->subcarrier_spacing = %u\n", srs_config_pdu->subcarrier_spacing);
  LOG_I(NR_PHY,"srs_config_pdu->cyclic_prefix = %u (0: Normal; 1: Extended)\n", srs_config_pdu->cyclic_prefix);
  LOG_I(NR_PHY,"srs_config_pdu->num_ant_ports = %u (0 = 1 port, 1 = 2 ports, 2 = 4 ports)\n", srs_config_pdu->num_ant_ports);
  LOG_I(NR_PHY,"srs_config_pdu->num_symbols = %u (0 = 1 symbol, 1 = 2 symbols, 2 = 4 symbols)\n", srs_config_pdu->num_symbols);
  LOG_I(NR_PHY,"srs_config_pdu->num_repetitions = %u (0 = 1, 1 = 2, 2 = 4)\n", srs_config_pdu->num_repetitions);
  LOG_I(NR_PHY,"srs_config_pdu->time_start_position = %u\n", srs_config_pdu->time_start_position);
  LOG_I(NR_PHY,"srs_config_pdu->config_index = %u\n", srs_config_pdu->config_index);
  LOG_I(NR_PHY,"srs_config_pdu->sequence_id = %u\n", srs_config_pdu->sequence_id);
  LOG_I(NR_PHY,"srs_config_pdu->bandwidth_index = %u\n", srs_config_pdu->bandwidth_index);
  LOG_I(NR_PHY,"srs_config_pdu->comb_size = %u (0 = comb size 2, 1 = comb size 4, 2 = comb size 8)\n", srs_config_pdu->comb_size);
  LOG_I(NR_PHY,"srs_config_pdu->comb_offset = %u\n", srs_config_pdu->comb_offset);
  LOG_I(NR_PHY,"srs_config_pdu->cyclic_shift = %u\n", srs_config_pdu->cyclic_shift);
  LOG_I(NR_PHY,"srs_config_pdu->frequency_position = %u\n", srs_config_pdu->frequency_position);
  LOG_I(NR_PHY,"srs_config_pdu->frequency_shift = %u\n", srs_config_pdu->frequency_shift);
  LOG_I(NR_PHY,"srs_config_pdu->frequency_hopping = %u\n", srs_config_pdu->frequency_hopping);
  LOG_I(NR_PHY,"srs_config_pdu->group_or_sequence_hopping = %u (0 = No hopping, 1 = Group hopping groupOrSequenceHopping, 2 = Sequence hopping)\n", srs_config_pdu->group_or_sequence_hopping);
  LOG_I(NR_PHY,"srs_config_pdu->resource_type = %u (0: aperiodic, 1: semi-persistent, 2: periodic)\n", srs_config_pdu->resource_type);
  LOG_I(NR_PHY,"srs_config_pdu->t_srs = %u\n", srs_config_pdu->t_srs);
  LOG_I(NR_PHY,"srs_config_pdu->t_offset = %u\n", srs_config_pdu->t_offset);
#endif

  NR_DL_FRAME_PARMS *frame_parms = &(ue->frame_parms);
  uint16_t symbol_offset = (frame_parms->symbols_per_slot - 1 - srs_config_pdu->time_start_position)*frame_parms->ofdm_symbol_size;

  if (generate_srs_nr(srs_config_pdu, frame_parms, &ue->common_vars.txdataF[gNB_id][symbol_offset], ue->nr_srs_info,
                      AMP, proc->frame_tx, proc->nr_slot_tx) == 0) {
    return 0;
  } else {
    return -1;
  }
}














