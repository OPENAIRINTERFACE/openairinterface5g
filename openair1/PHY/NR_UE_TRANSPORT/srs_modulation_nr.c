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

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"

#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
//#define SRS_DEBUG


uint16_t group_number_hopping(int slot_number,
                              uint8_t n_ID_SRS,
                              uint8_t l0,
                              uint8_t l_line) {

  // Pseudo-random sequence c(i) defined by TS 38.211 - Section 5.2.1
  uint32_t cinit = n_ID_SRS;
  uint8_t c_last_index = 8 * (slot_number * N_SYMB_SLOT + l0 + l_line) + 7;
  uint32_t *c_sequence =  calloc(c_last_index+1, sizeof(uint32_t));
  pseudo_random_sequence(c_last_index+1, c_sequence, cinit);

  // TS 38.211 - 6.4.1.4.2 Sequence generation
  uint32_t f_gh = 0;
  for (int m = 0; m <= 7; m++) {
    f_gh += c_sequence[8 * (slot_number * N_SYMB_SLOT + l0 + l_line) + m] << m;
  }
  f_gh = f_gh%30;
  uint8_t u = (f_gh + n_ID_SRS)%U_GROUP_NUMBER;

  free(c_sequence);
  return u;
}

uint16_t sequence_number_hopping(int slot_number,
                                 uint8_t n_ID_SRS,
                                 uint16_t M_sc_b_SRS,
                                 uint8_t l0,
                                 uint8_t l_line) {
  uint16_t v = 0;
  if (M_sc_b_SRS > 6 * NR_NB_SC_PER_RB) {
    // Pseudo-random sequence c(i) defined by TS 38.211 - Section 5.2.1
    uint32_t cinit = n_ID_SRS;
    uint8_t c_last_index = (slot_number * N_SYMB_SLOT + l0 + l_line);
    uint32_t *c_sequence =  calloc(c_last_index+1, sizeof(uint32_t));
    pseudo_random_sequence(c_last_index+1,  c_sequence, cinit);
    // TS 38.211 - 6.4.1.4.2 Sequence generation
    v = c_sequence[c_last_index];
    free(c_sequence);
  }
  return v;
}

uint16_t compute_F_b(frame_t frame_number,
                     slot_t slot_number,
                     uint16_t slots_per_frame,
                     uint8_t N_symb_SRS,
                     uint8_t B_SRS,
                     uint8_t C_SRS,
                     uint8_t b_hop,
                     uint8_t R,
                     uint16_t T_offset,
                     uint16_t T_SRS,
                     resourceType_t resource_type,
                     uint8_t l_line,
                     uint8_t b) {

  // Compute the number of SRS transmissions
  uint16_t n_SRS = 0;
  if (resource_type == aperiodic) {
    n_SRS = l_line / R;
  } else {
    n_SRS = ((slots_per_frame*frame_number + slot_number - T_offset)/T_SRS)*(N_symb_SRS/R)+(l_line / R);
  }

  uint16_t product_N_b = 1;
  for (unsigned int b_prime = b_hop; b_prime < B_SRS; b_prime++) {
    if (b_prime != b_hop) {
      product_N_b *= srs_bandwidth_config[C_SRS][b_prime][1];
    }
  }

  uint16_t F_b = 0;
  uint8_t N_b = srs_bandwidth_config[C_SRS][b][1];
  if (N_b & 1) { // Nb odd
    F_b = (N_b/2)*(n_SRS/product_N_b);
  } else { // Nb even
    uint16_t product_N_b_B_SRS = product_N_b;
    product_N_b_B_SRS *= srs_bandwidth_config[C_SRS][B_SRS][1]; /* product for b_hop to b */
    F_b = (N_b/2)*((n_SRS%product_N_b_B_SRS)/product_N_b) + ((n_SRS%product_N_b_B_SRS)/2*product_N_b);
  }

  return F_b;
}

uint16_t compute_n_b(frame_t frame_number,
                     slot_t slot_number,
                     uint16_t slots_per_frame,
                     uint8_t N_symb_SRS,
                     uint8_t B_SRS,
                     uint8_t C_SRS,
                     uint8_t b_hop,
                     uint8_t n_RRC,
                     uint8_t R,
                     uint16_t T_offset,
                     uint16_t T_SRS,
                     resourceType_t resource_type,
                     uint8_t l_line,
                     uint8_t b) {

  uint8_t N_b = srs_bandwidth_config[C_SRS][b][1];
  uint16_t m_SRS_b = srs_bandwidth_config[C_SRS][B_SRS][0];

  uint16_t n_b = 0;
  if (b_hop >= B_SRS) {
    n_b = (4 * n_RRC/m_SRS_b)%N_b;
  } else {
    if (b <= b_hop) {
      n_b = (4 * n_RRC/m_SRS_b)%N_b;
    } else {
      // Compute the hopping offset Fb
      uint16_t F_b = compute_F_b(frame_number, slot_number, slots_per_frame, N_symb_SRS, B_SRS, C_SRS, b_hop, R,
                                 T_offset, T_SRS, resource_type, l_line, b);
      n_b = (F_b + (4 * n_RRC/m_SRS_b))%N_b;
    }
  }

  return n_b;
}

/*************************************************************************
*
* NAME :         generate_srs_nr
*
* PARAMETERS :   pointer to srs config pdu
*                pointer to transmit buffer
*                amplitude scaling for this physical signal
*                slot number of transmission
* RETURN :       0  if srs sequence has been successfully generated
*                -1 if sequence can not be properly generated
*
* DESCRIPTION :  generate/map srs symbol into transmit buffer
*                See TS 38.211 - Section 6.4.1.4 Sounding reference signal
*
***************************************************************************/
int generate_srs_nr(nfapi_nr_srs_pdu_t *srs_config_pdu,
                    NR_DL_FRAME_PARMS *frame_parms,
                    int32_t **txdataF,
                    uint16_t symbol_offset,
                    nr_srs_info_t *nr_srs_info,
                    int16_t amp,
                    frame_t frame_number,
                    slot_t slot_number) {

#ifdef SRS_DEBUG
  LOG_I(NR_PHY,"Calling %s function\n", __FUNCTION__);
#endif

  // SRS config parameters
  uint8_t B_SRS = srs_config_pdu->bandwidth_index;
  uint8_t C_SRS = srs_config_pdu->config_index;
  uint8_t b_hop = srs_config_pdu->frequency_hopping;
  uint8_t K_TC = 2<<srs_config_pdu->comb_size;
  uint8_t K_TC_overbar = srs_config_pdu->comb_offset;
  uint8_t n_SRS_cs = srs_config_pdu->cyclic_shift;
  uint8_t n_ID_SRS = srs_config_pdu->sequence_id;
  uint8_t n_shift = srs_config_pdu->frequency_position;       // It adjusts the SRS allocation to align with the common resource block grid in multiples of four
  uint8_t n_RRC = srs_config_pdu->frequency_shift;
  uint8_t groupOrSequenceHopping = srs_config_pdu->group_or_sequence_hopping;
  uint8_t l_offset = srs_config_pdu->time_start_position;
  uint16_t T_SRS = srs_config_pdu->t_srs;
  uint16_t T_offset = srs_config_pdu->t_offset;
  uint8_t R = 1<<srs_config_pdu->num_repetitions;
  uint8_t N_ap = 1<<srs_config_pdu->num_ant_ports;            // Number of antenna port for transmission
  uint8_t N_symb_SRS = 1<<srs_config_pdu->num_symbols;        // Number of consecutive OFDM symbols
  uint8_t l0 = frame_parms->symbols_per_slot - 1 - l_offset;  // Starting symbol position in the time domain
  uint8_t n_SRS_cs_max = srs_max_number_cs[srs_config_pdu->comb_size];
  uint16_t m_SRS_b = srs_bandwidth_config[C_SRS][B_SRS][0];   // Number of resource blocks
  uint16_t M_sc_b_SRS = m_SRS_b * NR_NB_SC_PER_RB/K_TC;       // Length of the SRS sequence

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
  LOG_I(NR_PHY,"n_SRS_cs_max = %i\n", n_SRS_cs_max);
  LOG_I(NR_PHY,"m_SRS_b = %i\n", m_SRS_b);
  LOG_I(NR_PHY,"M_sc_b_SRS = %i\n", M_sc_b_SRS);
#endif

  // Validation of SRS config parameters

  if (R == 0) {
    LOG_E(NR_PHY, "generate_srs: this parameter repetition factor %d is not consistent !\n", R);
    return -1;
  } else if (R > N_symb_SRS) {
    LOG_E(NR_PHY, "generate_srs: R %d can not be greater than N_symb_SRS %d !\n", R, N_symb_SRS);
    return -1;
  }

  if (n_SRS_cs >= n_SRS_cs_max) {
    LOG_E(NR_PHY, "generate_srs: inconsistent parameter n_SRS_cs %d >=  n_SRS_cs_max %d !\n", n_SRS_cs, n_SRS_cs_max);
    return -1;
  }

  if (T_SRS == 0) {
    LOG_E(NR_PHY, "generate_srs: inconsistent parameter T_SRS %d can not be equal to zero !\n", T_SRS);
    return -1;
  } else {
    int index = 0;
    while (srs_periodicity[index] != T_SRS) {
      index++;
      if (index == SRS_PERIODICITY) {
        LOG_E(NR_PHY, "generate_srs: inconsistent parameter T_SRS %d not specified !\n", T_SRS);
        return -1;
      }
    }
  }

  // Variable initialization
  if(nr_srs_info) {
    nr_srs_info->srs_generated_signal_bits = log2_approx(amp);
  }
  uint64_t subcarrier_offset = frame_parms->first_carrier_offset + srs_config_pdu->bwp_start*NR_NB_SC_PER_RB;
  double sqrt_N_ap = sqrt(N_ap);
  uint16_t n_b[B_SRS_NUMBER];

  // Find index of table which is for this SRS length
  uint16_t M_sc_b_SRS_index = 0;
  while((ul_allocated_re[M_sc_b_SRS_index] != M_sc_b_SRS) && (M_sc_b_SRS_index < SRS_SB_CONF)){
    M_sc_b_SRS_index++;
  }

  // SRS sequence generation and mapping, TS 38.211 - Section 6.4.1.4
  for (int p_index = 0; p_index < N_ap; p_index++) {

#ifdef SRS_DEBUG
    LOG_I(NR_PHY,"============ port %d ============\n", p_index);
#endif

    uint16_t n_SRS_cs_i = (n_SRS_cs +  (n_SRS_cs_max * (SRS_antenna_port[p_index] - 1000)/N_ap))%n_SRS_cs_max;
    double alpha_i = 2 * M_PI * ((double)n_SRS_cs_i / (double)n_SRS_cs_max);

#ifdef SRS_DEBUG
    LOG_I(NR_PHY,"n_SRS_cs_i = %i\n", n_SRS_cs_i);
    LOG_I(NR_PHY,"alpha_i = %f\n", alpha_i);
#endif

    for (int l_line = 0; l_line < N_symb_SRS; l_line++) {

#ifdef SRS_DEBUG
      LOG_I(NR_PHY,":::::::: OFDM symbol %d ::::::::\n", l0+l_line);
#endif

      // Set group and sequence numbers (u,v) per OFDM symbol
      uint16_t u = 0;
      uint16_t v = 0;
      switch(groupOrSequenceHopping) {
        case neitherHopping:
          u = n_ID_SRS%U_GROUP_NUMBER;
          v = 0;
          break;
        case groupHopping:
          u = group_number_hopping(slot_number, n_ID_SRS, l0, l_line);
          v = 0;
          break;
        case sequenceHopping:
          u = n_ID_SRS%U_GROUP_NUMBER;
          v = sequence_number_hopping(slot_number, n_ID_SRS, M_sc_b_SRS, l0, l_line);
          break;
        default:
          LOG_E(NR_PHY, "generate_srs: unknown hopping setting %d !\n", groupOrSequenceHopping);
          return -1;
      }

#ifdef SRS_DEBUG
      LOG_I(NR_PHY,"u = %i\n", u);
      LOG_I(NR_PHY,"v = %i\n", v);
#endif

      // Compute the frequency position index n_b
      uint16_t sum_n_b = 0;
      for (int b = 0; b <= B_SRS; b++) {
        n_b[b] = compute_n_b(frame_number, slot_number, frame_parms->slots_per_frame, N_symb_SRS, B_SRS,
                             C_SRS, b_hop, n_RRC, R, T_offset, T_SRS, srs_config_pdu->resource_type, l_line, b);

        sum_n_b += n_b[b];

#ifdef SRS_DEBUG
        LOG_I(NR_PHY,"n_b[%i] = %i\n", b, n_b[b]);
#endif

      }

#ifdef SRS_DEBUG
      LOG_I(NR_PHY,"sum_n_b = %i\n", sum_n_b);
#endif

      // Compute the frequency-domain starting position
      uint8_t K_TC_p = 0;
      if((n_SRS_cs >= n_SRS_cs_max/2)&&(n_SRS_cs < n_SRS_cs_max)&&(N_ap == 4) && ((SRS_antenna_port[p_index] == 1001) || (SRS_antenna_port[p_index] == 1003))) {
        K_TC_p = (K_TC_overbar + K_TC/2)%K_TC;
      } else {
        K_TC_p = K_TC_overbar;
      }
      uint8_t k_l_offset = 0; // If the SRS is configured by the IE SRS-PosResource-r16, the quantity k_l_offset is
                              // given by TS 38.211 - Table 6.4.1.4.3-2, otherwise k_l_offset = 0.
      uint8_t k_0_overbar_p = (n_shift*NR_NB_SC_PER_RB + (K_TC_p+k_l_offset))%K_TC;
      uint8_t k_0_p = k_0_overbar_p + K_TC*M_sc_b_SRS*sum_n_b;
      nr_srs_info->k_0_p[p_index][l_line] = k_0_p;

#ifdef SRS_DEBUG
      LOG_I(NR_PHY,"K_TC_p = %i\n", K_TC_p);
      LOG_I(NR_PHY,"k_0_overbar_p = %i\n", k_0_overbar_p);
      LOG_I(NR_PHY,"k_0_p = %i\n", k_0_p);
#endif

      uint16_t subcarrier = subcarrier_offset + k_0_p;
      if (subcarrier>frame_parms->ofdm_symbol_size) {
        subcarrier -= frame_parms->ofdm_symbol_size;
      }
      uint16_t l_line_offset = l_line*frame_parms->ofdm_symbol_size;

      // For each port, and for each OFDM symbol, here it is computed and mapped an SRS sequence with M_sc_b_SRS symbols
      for (int k = 0; k < M_sc_b_SRS; k++) {

        double shift_real = cos(alpha_i * k);
        double shift_imag = sin(alpha_i * k);
        int16_t r_overbar_real = rv_ul_ref_sig[u][v][M_sc_b_SRS_index][k<<1];
        int16_t r_overbar_imag = rv_ul_ref_sig[u][v][M_sc_b_SRS_index][(k<<1)+1];

        // cos(x+y) = cos(x)cos(y) - sin(x)sin(y)
        double r_real = (shift_real*r_overbar_real - shift_imag*r_overbar_imag) / sqrt_N_ap;

        // sin(x+y) = sin(x)cos(y) + cos(x)sin(y)
        double r_imag = (shift_imag*r_overbar_real + shift_real*r_overbar_imag) / sqrt_N_ap;

        int32_t r_real_amp = ((int32_t) round((double) amp * r_real)) >> 15;
        int32_t r_imag_amp = ((int32_t) round((double) amp * r_imag)) >> 15;

#ifdef SRS_DEBUG
        int subcarrier_log = subcarrier-subcarrier_offset;
        if(subcarrier_log < 0) {
          subcarrier_log = subcarrier_log + frame_parms->ofdm_symbol_size;
        }
        if(subcarrier_log%12 == 0) {
          LOG_I(NR_PHY,"------------ %d ------------\n", subcarrier_log/12);
        }
        LOG_I(NR_PHY,"(%d)  \t%i\t%i\n", subcarrier_log, (int16_t)(r_real_amp&0xFFFF), (int16_t)(r_imag_amp&0xFFFF));
#endif

        *(c16_t *)&txdataF[p_index][symbol_offset + l_line_offset + subcarrier] = (c16_t){r_real_amp, r_imag_amp};

        // Subcarrier increment
        subcarrier += K_TC;
        if (subcarrier >= frame_parms->ofdm_symbol_size) {
          subcarrier=subcarrier-frame_parms->ofdm_symbol_size;
        }

      } // for (int k = 0; k < M_sc_b_SRS; k++)
    } // for (int l_line = 0; l_line < N_symb_SRS; l_line++)
  } // for (int p_index = 0; p_index < N_ap; p_index++)

  return 0;
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
int ue_srs_procedures_nr(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc, c16_t **txdataF)
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

  if (generate_srs_nr(srs_config_pdu,
                      frame_parms,
                      (int32_t **)txdataF,
                      symbol_offset,
                      ue->nr_srs_info,
                      AMP,
                      proc->frame_tx,
                      proc->nr_slot_tx)
      == 0) {
    return 0;
  } else {
    return -1;
  }
}














