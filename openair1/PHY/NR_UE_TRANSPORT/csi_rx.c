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
* FILENAME    :  csi_rx.c
*
* MODULE      :
*
* DESCRIPTION :  function to receive the channel state information
*
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "executables/nr-softmodem-common.h"
#include "nr_transport_proto_ue.h"
#include "PHY/phy_extern_nr_ue.h"
#include "common/utils/nr/nr_common.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_ESTIMATION/filt16a_32.h"

// 10*log10(pow(2,30))
#define pow_2_30_dB 90

// Additional memory allocation, because of applying the filter and the memory offset to ensure memory alignment
#define FILTER_MARGIN 32

//#define NR_CSIRS_DEBUG
//#define NR_CSIIM_DEBUG

void nr_det_A_MF_2x2(int32_t *a_mf_00,
                     int32_t *a_mf_01,
                     int32_t *a_mf_10,
                     int32_t *a_mf_11,
                     int32_t *det_fin,
                     const unsigned short nb_rb) {

  int16_t nr_conjug2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1} ;

  simde__m128i ad_re_128, bc_re_128, det_re_128;

  simde__m128i *a_mf_00_128 = (simde__m128i *)a_mf_00;
  simde__m128i *a_mf_01_128 = (simde__m128i *)a_mf_01;
  simde__m128i *a_mf_10_128 = (simde__m128i *)a_mf_10;
  simde__m128i *a_mf_11_128 = (simde__m128i *)a_mf_11;
  simde__m128i *det_fin_128 = (simde__m128i *)det_fin;

  for (int rb = 0; rb<3*nb_rb; rb++) {

    //complex multiplication (I_a+jQ_a)(I_d+jQ_d) = (I_aI_d - Q_aQ_d) + j(Q_aI_d + I_aQ_d)
    //The imag part is often zero, we compute only the real part
    ad_re_128 = simde_mm_sign_epi16(a_mf_00_128[0],*(simde__m128i*)&nr_conjug2[0]);
    ad_re_128 = simde_mm_madd_epi16(ad_re_128,a_mf_11_128[0]); //Re: I_a0*I_d0 - Q_a1*Q_d1

    //complex multiplication (I_b+jQ_b)(I_c+jQ_c) = (I_bI_c - Q_bQ_c) + j(Q_bI_c + I_bQ_c)
    //The imag part is often zero, we compute only the real part
    bc_re_128 = simde_mm_sign_epi16(a_mf_01_128[0],*(simde__m128i*)&nr_conjug2[0]);
    bc_re_128 = simde_mm_madd_epi16(bc_re_128,a_mf_10_128[0]); //Re: I_b0*I_c0 - Q_b1*Q_c1

    det_re_128 = simde_mm_sub_epi32(ad_re_128, bc_re_128);

    //det in Q30 format
    det_fin_128[0] = simde_mm_abs_epi32(det_re_128);

    det_fin_128+=1;
    a_mf_00_128+=1;
    a_mf_01_128+=1;
    a_mf_10_128+=1;
    a_mf_11_128+=1;
  }
  simde_mm_empty();
  simde_m_empty();
}

void nr_squared_matrix_element(int32_t *a,
                               int32_t *a_sq,
                               const unsigned short nb_rb) {
  simde__m128i *a_128 = (simde__m128i *)a;
  simde__m128i *a_sq_128 = (simde__m128i *)a_sq;
  for (int rb=0; rb<3*nb_rb; rb++) {
    a_sq_128[0] = simde_mm_madd_epi16(a_128[0], a_128[0]);
    a_sq_128+=1;
    a_128+=1;
  }
  simde_mm_empty();
  simde_m_empty();
}

void nr_numer_2x2(int32_t *a_00_sq,
                  int32_t *a_01_sq,
                  int32_t *a_10_sq,
                  int32_t *a_11_sq,
                  int32_t *num_fin,
                  const unsigned short nb_rb) {
  simde__m128i *a_00_sq_128 = (simde__m128i *)a_00_sq;
  simde__m128i *a_01_sq_128 = (simde__m128i *)a_01_sq;
  simde__m128i *a_10_sq_128 = (simde__m128i *)a_10_sq;
  simde__m128i *a_11_sq_128 = (simde__m128i *)a_11_sq;
  simde__m128i *num_fin_128 = (simde__m128i *)num_fin;
  for (int rb=0; rb<3*nb_rb; rb++) {
    simde__m128i sq_a_plus_sq_d_128 = simde_mm_add_epi32(a_00_sq_128[0], a_11_sq_128[0]);
    simde__m128i sq_b_plus_sq_c_128 = simde_mm_add_epi32(a_01_sq_128[0], a_10_sq_128[0]);
    num_fin_128[0] = simde_mm_add_epi32(sq_a_plus_sq_d_128, sq_b_plus_sq_c_128);
    num_fin_128+=1;
    a_00_sq_128+=1;
    a_01_sq_128+=1;
    a_10_sq_128+=1;
    a_11_sq_128+=1;
  }
  simde_mm_empty();
  simde_m_empty();
}

bool is_csi_rs_in_symbol(const fapi_nr_dl_config_csirs_pdu_rel15_t csirs_config_pdu, const int symbol) {

  bool ret = false;

  // 38.211-Table 7.4.1.5.3-1: CSI-RS locations within a slot
  switch(csirs_config_pdu.row){
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 9:
      if(symbol == csirs_config_pdu.symb_l0) {
        ret = true;
      }
      break;
    case 5:
    case 7:
    case 8:
    case 10:
    case 11:
    case 12:
      if(symbol == csirs_config_pdu.symb_l0 || symbol == (csirs_config_pdu.symb_l0+1) ) {
        ret = true;
      }
      break;
    case 13:
    case 14:
    case 16:
    case 17:
      if(symbol == csirs_config_pdu.symb_l0 || symbol == (csirs_config_pdu.symb_l0+1) ||
          symbol == csirs_config_pdu.symb_l1 || symbol == (csirs_config_pdu.symb_l1+1)) {
        ret = true;
      }
      break;
    case 15:
    case 18:
      if(symbol == csirs_config_pdu.symb_l0 || symbol == (csirs_config_pdu.symb_l0+1) || symbol == (csirs_config_pdu.symb_l0+2) ) {
        ret = true;
      }
      break;
    default:
      AssertFatal(0==1, "Row %d is not valid for CSI Table 7.4.1.5.3-1\n", csirs_config_pdu.row);
  }

  return ret;
}

int nr_get_csi_rs_signal(const PHY_VARS_NR_UE *ue,
                         const UE_nr_rxtx_proc_t *proc,
                         const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                         const nr_csi_info_t *nr_csi_info,
                         const uint8_t N_cdm_groups,
                         const uint8_t CDM_group_size,
                         const uint8_t k_prime,
                         const uint8_t l_prime,
                         const uint8_t *j_cdm,
                         const uint8_t *k_overline,
                         const uint8_t *l_overline,
                         int32_t csi_rs_received_signal[][ue->frame_parms.samples_per_slot_wCP],
                         uint32_t *rsrp,
                         int *rsrp_dBm,
                         c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]) {

  const NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  uint16_t meas_count = 0;
  uint32_t rsrp_sum = 0;

  for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      // for freq density 0.5 checks if even or odd RB
      if(csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }

      for (int cdm_id = 0; cdm_id < N_cdm_groups; cdm_id++) {
        for (int s = 0; s < CDM_group_size; s++)  {

          // loop over frequency resource elements within a group
          for (int kp = 0; kp <= k_prime; kp++) {

            uint16_t k = (frame_parms->first_carrier_offset + (rb*NR_NB_SC_PER_RB)+k_overline[cdm_id] + kp) % frame_parms->ofdm_symbol_size;

            // loop over time resource elements within a group
            for (int lp = 0; lp <= l_prime; lp++) {
              uint16_t symb = lp + l_overline[cdm_id];
              uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;
              c16_t *rx_signal = &rxdataF[ant_rx][symbol_offset];
              c16_t *rx_csi_rs_signal = (c16_t*)&csi_rs_received_signal[ant_rx][symbol_offset];
              rx_csi_rs_signal[k].r = rx_signal[k].r;
              rx_csi_rs_signal[k].i = rx_signal[k].i;

              rsrp_sum += (((int32_t)(rx_csi_rs_signal[k].r)*rx_csi_rs_signal[k].r) +
                           ((int32_t)(rx_csi_rs_signal[k].i)*rx_csi_rs_signal[k].i));

              meas_count++;

#ifdef NR_CSIRS_DEBUG
              int dataF_offset = proc->nr_slot_rx*ue->frame_parms.samples_per_slot_wCP;
              uint16_t port_tx = s+j_cdm[cdm_id]*CDM_group_size;
              c16_t *tx_csi_rs_signal = (c16_t*)&nr_csi_info->csi_rs_generated_signal[port_tx][symbol_offset+dataF_offset];
              LOG_I(NR_PHY, "l,k (%2d,%4d) |\tport_tx %d (%4d,%4d)\tant_rx %d (%4d,%4d)\n",
                    symb,
                    k,
                    port_tx+3000,
                    tx_csi_rs_signal[k].r,
                    tx_csi_rs_signal[k].i,
                    ant_rx,
                    rx_csi_rs_signal[k].r,
                    rx_csi_rs_signal[k].i);
#endif
            }
          }
        }
      }
    }
  }


  *rsrp = rsrp_sum/meas_count;
  *rsrp_dBm = dB_fixed(*rsrp) + 30 - pow_2_30_dB
      - ((int)openair0_cfg[0].rx_gain[0] - (int)openair0_cfg[0].rx_gain_offset[0]) - dB_fixed(ue->frame_parms.ofdm_symbol_size);

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "RSRP = %i (%i dBm)\n", *rsrp, *rsrp_dBm);
#endif

  return 0;
}

uint32_t calc_power_csirs(const uint16_t *x, const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu)
{
  uint64_t sum_x = 0;
  uint64_t sum_x2 = 0;
  uint16_t size = 0;
  for (int rb = 0; rb < csirs_config_pdu->nr_of_rbs; rb++) {
    if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != ((rb + csirs_config_pdu->start_rb) % 2)) {
      continue;
    }
    sum_x = sum_x + x[rb];
    sum_x2 = sum_x2 + x[rb] * x[rb];
    size++;
  }
  return sum_x2 / size - (sum_x / size) * (sum_x / size);
}

int nr_csi_rs_channel_estimation(const PHY_VARS_NR_UE *ue,
                                 const UE_nr_rxtx_proc_t *proc,
                                 const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                                 const nr_csi_info_t *nr_csi_info,
                                 const int32_t **csi_rs_generated_signal,
                                 const int32_t csi_rs_received_signal[][ue->frame_parms.samples_per_slot_wCP],
                                 const uint8_t N_cdm_groups,
                                 const uint8_t CDM_group_size,
                                 const uint8_t k_prime,
                                 const uint8_t l_prime,
                                 const uint8_t N_ports,
                                 const uint8_t *j_cdm,
                                 const uint8_t *k_overline,
                                 const uint8_t *l_overline,
                                 uint8_t mem_offset,
                                 int32_t csi_rs_ls_estimated_channel[][N_ports][ue->frame_parms.ofdm_symbol_size],
                                 int32_t csi_rs_estimated_channel_freq[][N_ports][ue->frame_parms.ofdm_symbol_size + FILTER_MARGIN],
                                 int16_t *log2_re,
                                 int16_t *log2_maxh,
                                 uint32_t *noise_power) {

  const NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  const int dataF_offset = proc->nr_slot_rx*ue->frame_parms.samples_per_slot_wCP;
  *noise_power = 0;
  int maxh = 0;
  int count = 0;

  for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {

    /// LS channel estimation

    for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
      memset(csi_rs_ls_estimated_channel[ant_rx][port_tx], 0, frame_parms->ofdm_symbol_size*sizeof(int32_t));
    }

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      // for freq density 0.5 checks if even or odd RB
      if(csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }

      for (int cdm_id = 0; cdm_id < N_cdm_groups; cdm_id++) {
        for (int s = 0; s < CDM_group_size; s++)  {

          uint16_t port_tx = s+j_cdm[cdm_id]*CDM_group_size;

          // loop over frequency resource elements within a group
          for (int kp = 0; kp <= k_prime; kp++) {

            uint16_t kinit = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
            uint16_t k = kinit + k_overline[cdm_id] + kp;

            // loop over time resource elements within a group
            for (int lp = 0; lp <= l_prime; lp++) {
              uint16_t symb = lp + l_overline[cdm_id];
              uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;
              c16_t *tx_csi_rs_signal = (c16_t*)&csi_rs_generated_signal[port_tx][symbol_offset+dataF_offset];
              c16_t *rx_csi_rs_signal = (c16_t*)&csi_rs_received_signal[ant_rx][symbol_offset];
              c16_t *csi_rs_ls_estimated_channel16 = (c16_t*)&csi_rs_ls_estimated_channel[ant_rx][port_tx][0];

              int16_t csi_rs_ls_estimated_channel_re = (int16_t)(((int32_t)tx_csi_rs_signal[k].r*rx_csi_rs_signal[k].r + (int32_t)tx_csi_rs_signal[k].i*rx_csi_rs_signal[k].i)>>nr_csi_info->csi_rs_generated_signal_bits);
              int16_t csi_rs_ls_estimated_channel_im = (int16_t)(((int32_t)tx_csi_rs_signal[k].r*rx_csi_rs_signal[k].i - (int32_t)tx_csi_rs_signal[k].i*rx_csi_rs_signal[k].r)>>nr_csi_info->csi_rs_generated_signal_bits);

              // This is not just the LS estimation for each (k,l), but also the sum of the different contributions
              // for the sake of optimizing the memory used.
              csi_rs_ls_estimated_channel16[kinit].r += csi_rs_ls_estimated_channel_re;
              csi_rs_ls_estimated_channel16[kinit].i += csi_rs_ls_estimated_channel_im;
            }
          }
        }
      }
    }

#ifdef NR_CSIRS_DEBUG
    for(int symb = 0; symb < NR_SYMBOLS_PER_SLOT; symb++) {
      if(!is_csi_rs_in_symbol(*csirs_config_pdu,symb)) {
        continue;
      }
      for(int k = 0; k<frame_parms->ofdm_symbol_size; k++) {
        LOG_I(NR_PHY, "l,k (%2d,%4d) | ", symb, k);
        for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
          uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;
          c16_t *tx_csi_rs_signal = (c16_t*)&csi_rs_generated_signal[port_tx][symbol_offset+dataF_offset];
          c16_t *rx_csi_rs_signal = (c16_t*)&csi_rs_received_signal[ant_rx][symbol_offset];
          c16_t *csi_rs_ls_estimated_channel16 = (c16_t*)&csi_rs_ls_estimated_channel[ant_rx][port_tx][0];
          printf("port_tx %d --> ant_rx %d, tx (%4d,%4d), rx (%4d,%4d), ls (%4d,%4d) | ",
                 port_tx+3000, ant_rx,
                 tx_csi_rs_signal[k].r, tx_csi_rs_signal[k].i,
                 rx_csi_rs_signal[k].r, rx_csi_rs_signal[k].i,
                 csi_rs_ls_estimated_channel16[k].r, csi_rs_ls_estimated_channel16[k].i);
        }
        printf("\n");
      }
    }
#endif

    /// Channel interpolation

    for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
      memset(csi_rs_estimated_channel_freq[ant_rx][port_tx], 0, (frame_parms->ofdm_symbol_size + FILTER_MARGIN) * sizeof(int32_t));
    }

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      // for freq density 0.5 checks if even or odd RB
      if(csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }

      count++;

      uint16_t k = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
      uint16_t k_offset = k + mem_offset;
      for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
        int16_t *csi_rs_ls_estimated_channel16 = (int16_t*)&csi_rs_ls_estimated_channel[ant_rx][port_tx][k];
        int16_t *csi_rs_estimated_channel16 = (int16_t *)&csi_rs_estimated_channel_freq[ant_rx][port_tx][k_offset];
        if( (k == 0) || (k == frame_parms->first_carrier_offset) ) { // Start of OFDM symbol case or first occupied subcarrier case
          multadd_real_vector_complex_scalar(filt24_start, csi_rs_ls_estimated_channel16, csi_rs_estimated_channel16, 24);
        } else if( ( (k + NR_NB_SC_PER_RB) >= frame_parms->ofdm_symbol_size) ||
                   (rb == (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs-1)) ) { // End of OFDM symbol case or Last occupied subcarrier case
          multadd_real_vector_complex_scalar(filt24_end, csi_rs_ls_estimated_channel16, csi_rs_estimated_channel16 - 3*sizeof(uint64_t), 24);
        } else { // Middle case
          multadd_real_vector_complex_scalar(filt24_middle, csi_rs_ls_estimated_channel16, csi_rs_estimated_channel16 - 3*sizeof(uint64_t), 24);
        }
      }
    }

    /// Power noise estimation
    uint16_t noise_real[frame_parms->nb_antennas_rx][N_ports][csirs_config_pdu->nr_of_rbs];
    uint16_t noise_imag[frame_parms->nb_antennas_rx][N_ports][csirs_config_pdu->nr_of_rbs];
    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {
      if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }
      uint16_t k = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
      uint16_t k_offset = k + mem_offset;
      for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
        c16_t *csi_rs_ls_estimated_channel16 = (c16_t*)&csi_rs_ls_estimated_channel[ant_rx][port_tx][k];
        c16_t *csi_rs_estimated_channel16 = (c16_t *)&csi_rs_estimated_channel_freq[ant_rx][port_tx][k_offset];
        noise_real[ant_rx][port_tx][rb-csirs_config_pdu->start_rb] = abs(csi_rs_ls_estimated_channel16->r-csi_rs_estimated_channel16->r);
        noise_imag[ant_rx][port_tx][rb-csirs_config_pdu->start_rb] = abs(csi_rs_ls_estimated_channel16->i-csi_rs_estimated_channel16->i);
        maxh = cmax3(maxh, abs(csi_rs_estimated_channel16->r), abs(csi_rs_estimated_channel16->i));
      }
    }
    for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
      *noise_power += (calc_power_csirs(noise_real[ant_rx][port_tx], csirs_config_pdu) + calc_power_csirs(noise_imag[ant_rx][port_tx],csirs_config_pdu));
    }

#ifdef NR_CSIRS_DEBUG
    for(int k = 0; k<frame_parms->ofdm_symbol_size; k++) {
      int rb = k >= frame_parms->first_carrier_offset ?
               (k - frame_parms->first_carrier_offset)/NR_NB_SC_PER_RB :
               (k + frame_parms->ofdm_symbol_size - frame_parms->first_carrier_offset)/NR_NB_SC_PER_RB;
      LOG_I(NR_PHY, "(k = %4d) |\t", k);
      for(uint16_t port_tx = 0; port_tx<N_ports; port_tx++) {
        c16_t *csi_rs_ls_estimated_channel16 = (c16_t*)&csi_rs_ls_estimated_channel[ant_rx][port_tx][0];
        c16_t *csi_rs_estimated_channel16 = (c16_t *)&csi_rs_estimated_channel_freq[ant_rx][port_tx][mem_offset];
        printf("Channel port_tx %d --> ant_rx %d : ls (%4d,%4d), int (%4d,%4d), noise (%4d,%4d) | ",
               port_tx+3000, ant_rx,
               csi_rs_ls_estimated_channel16[k].r, csi_rs_ls_estimated_channel16[k].i,
               csi_rs_estimated_channel16[k].r, csi_rs_estimated_channel16[k].i,
               rb >= csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs ? 0 : noise_real[ant_rx][port_tx][rb-csirs_config_pdu->start_rb],
               rb >= csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs ? 0 : noise_imag[ant_rx][port_tx][rb-csirs_config_pdu->start_rb]);
      }
      printf("\n");
    }
#endif

  }

  *noise_power /= (frame_parms->nb_antennas_rx*N_ports);
  *log2_maxh = log2_approx(maxh-1);
  *log2_re = log2_approx(count-1);

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "Noise power estimation based on CSI-RS: %i\n", *noise_power);
#endif

  return 0;
}

int nr_csi_rs_ri_estimation(const PHY_VARS_NR_UE *ue,
                            const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                            const nr_csi_info_t *nr_csi_info,
                            const uint8_t N_ports,
                            uint8_t mem_offset,
                            int32_t csi_rs_estimated_channel_freq[][N_ports][ue->frame_parms.ofdm_symbol_size + FILTER_MARGIN],
                            const int16_t log2_maxh,
                            uint8_t *rank_indicator) {

  const NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  const int16_t cond_dB_threshold = 5;
  int count = 0;
  *rank_indicator = 0;

  if (ue->frame_parms.nb_antennas_rx == 1 || N_ports == 1) {
    return 0;
  } else if( !(ue->frame_parms.nb_antennas_rx == 2 && N_ports == 2) ) {
    LOG_W(NR_PHY, "Rank indicator computation is not implemented for %i x %i system\n",
          ue->frame_parms.nb_antennas_rx, N_ports);
    return -1;
  }

  /* Example 2x2: Hh x H =
  *            | conjch00 conjch10 | x | ch00 ch01 | = | conjch00*ch00+conjch10*ch10 conjch00*ch01+conjch10*ch11 |
  *            | conjch01 conjch11 |   | ch10 ch11 |   | conjch01*ch00+conjch11*ch10 conjch01*ch01+conjch11*ch11 |
  */

  int32_t csi_rs_estimated_conjch_ch[frame_parms->nb_antennas_rx][N_ports][frame_parms->nb_antennas_rx][N_ports][frame_parms->ofdm_symbol_size + FILTER_MARGIN] __attribute__((aligned(32)));
  int32_t csi_rs_estimated_A_MF[N_ports][N_ports][frame_parms->ofdm_symbol_size + FILTER_MARGIN] __attribute__((aligned(32)));
  int32_t csi_rs_estimated_A_MF_sq[N_ports][N_ports][frame_parms->ofdm_symbol_size + FILTER_MARGIN] __attribute__((aligned(32)));
  int32_t csi_rs_estimated_determ_fin[frame_parms->ofdm_symbol_size + FILTER_MARGIN] __attribute__((aligned(32)));
  int32_t csi_rs_estimated_numer_fin[frame_parms->ofdm_symbol_size + FILTER_MARGIN] __attribute__((aligned(32)));
  const uint8_t sum_shift = 1; // log2(2x2) = 2, which is a shift of 1 bit
  
  for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

    if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
      continue;
    }
    uint16_t k = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
    uint16_t k_offset = k + mem_offset;

    for (int ant_rx_conjch = 0; ant_rx_conjch < frame_parms->nb_antennas_rx; ant_rx_conjch++) {
      for(uint16_t port_tx_conjch = 0; port_tx_conjch < N_ports; port_tx_conjch++) {
        for (int ant_rx_ch = 0; ant_rx_ch < frame_parms->nb_antennas_rx; ant_rx_ch++) {
          for(uint16_t port_tx_ch = 0; port_tx_ch < N_ports; port_tx_ch++) {

            // conjch x ch computation
            nr_conjch0_mult_ch1(&csi_rs_estimated_channel_freq[ant_rx_conjch][port_tx_conjch][k_offset],
                                &csi_rs_estimated_channel_freq[ant_rx_ch][port_tx_ch][k_offset],
                                &csi_rs_estimated_conjch_ch[ant_rx_conjch][port_tx_conjch][ant_rx_ch][port_tx_ch][k_offset],
                                1,
                                log2_maxh);

            // construct Hh x H elements
            if(ant_rx_conjch == ant_rx_ch) {
              nr_a_sum_b(
                  (c16_t *)&csi_rs_estimated_A_MF[port_tx_conjch][port_tx_ch][k_offset], (c16_t *)&csi_rs_estimated_conjch_ch[ant_rx_conjch][port_tx_conjch][ant_rx_ch][port_tx_ch][k_offset], 1);
            }
          }
        }
      }
    }

    // compute the determinant of A_MF (denominator)
    nr_det_A_MF_2x2(&csi_rs_estimated_A_MF[0][0][k_offset],
                    &csi_rs_estimated_A_MF[0][1][k_offset],
                    &csi_rs_estimated_A_MF[1][0][k_offset],
                    &csi_rs_estimated_A_MF[1][1][k_offset],
                    &csi_rs_estimated_determ_fin[k_offset],
                    1);

    // compute the square of A_MF (numerator)
    nr_squared_matrix_element(&csi_rs_estimated_A_MF[0][0][k_offset], &csi_rs_estimated_A_MF_sq[0][0][k_offset], 1);
    nr_squared_matrix_element(&csi_rs_estimated_A_MF[0][1][k_offset], &csi_rs_estimated_A_MF_sq[0][1][k_offset], 1);
    nr_squared_matrix_element(&csi_rs_estimated_A_MF[1][0][k_offset], &csi_rs_estimated_A_MF_sq[1][0][k_offset], 1);
    nr_squared_matrix_element(&csi_rs_estimated_A_MF[1][1][k_offset], &csi_rs_estimated_A_MF_sq[1][1][k_offset], 1);
    nr_numer_2x2(&csi_rs_estimated_A_MF_sq[0][0][k_offset],
                 &csi_rs_estimated_A_MF_sq[0][1][k_offset],
                 &csi_rs_estimated_A_MF_sq[1][0][k_offset],
                 &csi_rs_estimated_A_MF_sq[1][1][k_offset],
                 &csi_rs_estimated_numer_fin[k_offset],
                 1);

#ifdef NR_CSIRS_DEBUG
    for(uint16_t port_tx_conjch = 0; port_tx_conjch < N_ports; port_tx_conjch++) {
      for(uint16_t port_tx_ch = 0; port_tx_ch < N_ports; port_tx_ch++) {
        c16_t *csi_rs_estimated_A_MF_k = (c16_t *) &csi_rs_estimated_A_MF[port_tx_conjch][port_tx_ch][k_offset];
        LOG_I(NR_PHY, "(%i) csi_rs_estimated_A_MF[%i][%i] = (%i, %i)\n",
              k, port_tx_conjch, port_tx_ch, csi_rs_estimated_A_MF_k->r, csi_rs_estimated_A_MF_k->i);
        c16_t *csi_rs_estimated_A_MF_sq_k = (c16_t *) &csi_rs_estimated_A_MF_sq[port_tx_conjch][port_tx_ch][k_offset];
        LOG_I(NR_PHY, "(%i) csi_rs_estimated_A_MF_sq[%i][%i] = (%i, %i)\n",
              k, port_tx_conjch, port_tx_ch, csi_rs_estimated_A_MF_sq_k->r, csi_rs_estimated_A_MF_sq_k->i);
      }
    }
    LOG_I(NR_PHY, "(%i) csi_rs_estimated_determ_fin = %i\n", k, csi_rs_estimated_determ_fin[k_offset]);
    LOG_I(NR_PHY, "(%i) csi_rs_estimated_numer_fin = %i\n", k, csi_rs_estimated_numer_fin[k_offset]>>sum_shift);
#endif

    // compute the conditional number
    for (int sc_idx=0; sc_idx < NR_NB_SC_PER_RB; sc_idx++) {
      int8_t csi_rs_estimated_denum_db = dB_fixed(csi_rs_estimated_determ_fin[k_offset + sc_idx]);
      int8_t csi_rs_estimated_numer_db = dB_fixed(csi_rs_estimated_numer_fin[k_offset + sc_idx]>>sum_shift);
      int8_t cond_db = csi_rs_estimated_numer_db - csi_rs_estimated_denum_db;

#ifdef NR_CSIRS_DEBUG
      LOG_I(NR_PHY, "csi_rs_estimated_denum_db = %i\n", csi_rs_estimated_denum_db);
      LOG_I(NR_PHY, "csi_rs_estimated_numer_db = %i\n", csi_rs_estimated_numer_db);
      LOG_I(NR_PHY, "cond_db = %i\n", cond_db);
#endif

      if (cond_db < cond_dB_threshold) {
        count++;
      } else {
        count--;
      }
    }
  }

  // conditional number is lower than cond_dB_threshold in half on more REs
  if (count > 0) {
    *rank_indicator = 1;
  }

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "count = %i\n", count);
  LOG_I(NR_PHY, "rank = %i\n", (*rank_indicator)+1);
#endif

  return 0;
}

int nr_csi_rs_pmi_estimation(const PHY_VARS_NR_UE *ue,
                             const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                             const nr_csi_info_t *nr_csi_info,
                             const uint8_t N_ports,
                             uint8_t mem_offset,
                             const int32_t csi_rs_estimated_channel_freq[][N_ports][ue->frame_parms.ofdm_symbol_size + FILTER_MARGIN],
                             const uint32_t interference_plus_noise_power,
                             const uint8_t rank_indicator,
                             const int16_t log2_re,
                             uint8_t *i1,
                             uint8_t *i2,
                             uint32_t *precoded_sinr_dB) {

  const NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  // i1 is a three-element vector in the form of [i11 i12 i13], when CodebookType is specified as 'Type1SinglePanel'.
  // Note that i13 is not applicable when the number of transmission layers is one of {1, 5, 6, 7, 8}.
  // i2, for 'Type1SinglePanel' codebook type, it is a scalar when PMIMode is specified as 'wideband', and when PMIMode
  // is specified as 'subband' or when PRGSize, the length of the i2 vector equals to the number of subbands or PRGs.
  // Note that when the number of CSI-RS ports is 2, the applicable codebook type is 'Type1SinglePanel'. In this case,
  // the precoding matrix is obtained by a single index (i2 field here) based on TS 38.214 Table 5.2.2.2.1-1.
  // The first column is applicable if the UE is reporting a Rank = 1, whereas the second column is applicable if the
  // UE is reporting a Rank = 2.

  if(N_ports == 1 || interference_plus_noise_power == 0) {
    return 0;
  }

  if(rank_indicator == 0 || rank_indicator == 1) {

    int32_t sum_re[4] = {0};
    int32_t sum_im[4] = {0};
    int32_t sum2_re[4] = {0};
    int32_t sum2_im[4] = {0};
    int32_t tested_precoded_sinr[4] = {0};

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }
      uint16_t k = (frame_parms->first_carrier_offset + rb * NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
      uint16_t k_offset = k + mem_offset;
      for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {
        c16_t *csi_rs_estimated_channel_p0 = (c16_t *) &csi_rs_estimated_channel_freq[ant_rx][0][k_offset];
        c16_t *csi_rs_estimated_channel_p1 = (c16_t *) &csi_rs_estimated_channel_freq[ant_rx][1][k_offset];

        // H_p0 + 1*H_p1 = (H_p0_re + H_p1_re) + 1j*(H_p0_im + H_p1_im)
        sum_re[0] += (csi_rs_estimated_channel_p0->r+csi_rs_estimated_channel_p1->r);
        sum_im[0] += (csi_rs_estimated_channel_p0->i+csi_rs_estimated_channel_p1->i);
        sum2_re[0] += ((csi_rs_estimated_channel_p0->r+csi_rs_estimated_channel_p1->r)*(csi_rs_estimated_channel_p0->r+csi_rs_estimated_channel_p1->r))>>log2_re;
        sum2_im[0] += ((csi_rs_estimated_channel_p0->i+csi_rs_estimated_channel_p1->i)*(csi_rs_estimated_channel_p0->i+csi_rs_estimated_channel_p1->i))>>log2_re;

        // H_p0 + 1j*H_p1 = (H_p0_re - H_p1_im) + 1j*(H_p0_im + H_p1_re)
        sum_re[1] += (csi_rs_estimated_channel_p0->r-csi_rs_estimated_channel_p1->i);
        sum_im[1] += (csi_rs_estimated_channel_p0->i+csi_rs_estimated_channel_p1->r);
        sum2_re[1] += ((csi_rs_estimated_channel_p0->r-csi_rs_estimated_channel_p1->i)*(csi_rs_estimated_channel_p0->r-csi_rs_estimated_channel_p1->i))>>log2_re;
        sum2_im[1] += ((csi_rs_estimated_channel_p0->i+csi_rs_estimated_channel_p1->r)*(csi_rs_estimated_channel_p0->i+csi_rs_estimated_channel_p1->r))>>log2_re;

        // H_p0 - 1*H_p1 = (H_p0_re - H_p1_re) + 1j*(H_p0_im - H_p1_im)
        sum_re[2] += (csi_rs_estimated_channel_p0->r-csi_rs_estimated_channel_p1->r);
        sum_im[2] += (csi_rs_estimated_channel_p0->i-csi_rs_estimated_channel_p1->i);
        sum2_re[2] += ((csi_rs_estimated_channel_p0->r-csi_rs_estimated_channel_p1->r)*(csi_rs_estimated_channel_p0->r-csi_rs_estimated_channel_p1->r))>>log2_re;
        sum2_im[2] += ((csi_rs_estimated_channel_p0->i-csi_rs_estimated_channel_p1->i)*(csi_rs_estimated_channel_p0->i-csi_rs_estimated_channel_p1->i))>>log2_re;

        // H_p0 - 1j*H_p1 = (H_p0_re + H_p1_im) + 1j*(H_p0_im - H_p1_re)
        sum_re[3] += (csi_rs_estimated_channel_p0->r+csi_rs_estimated_channel_p1->i);
        sum_im[3] += (csi_rs_estimated_channel_p0->i-csi_rs_estimated_channel_p1->r);
        sum2_re[3] += ((csi_rs_estimated_channel_p0->r+csi_rs_estimated_channel_p1->i)*(csi_rs_estimated_channel_p0->r+csi_rs_estimated_channel_p1->i))>>log2_re;
        sum2_im[3] += ((csi_rs_estimated_channel_p0->i-csi_rs_estimated_channel_p1->r)*(csi_rs_estimated_channel_p0->i-csi_rs_estimated_channel_p1->r))>>log2_re;
      }
    }

    // We should perform >>nr_csi_info->log2_re here for all terms, but since sum2_re and sum2_im can be high values,
    // we performed this above.
    for(int p = 0; p<4; p++) {
      int32_t power_re = sum2_re[p] - (sum_re[p]>>log2_re)*(sum_re[p]>>log2_re);
      int32_t power_im = sum2_im[p] - (sum_im[p]>>log2_re)*(sum_im[p]>>log2_re);
      tested_precoded_sinr[p] = (power_re+power_im)/(int32_t)interference_plus_noise_power;
    }

    if(rank_indicator == 0) {
      for(int tested_i2 = 0; tested_i2 < 4; tested_i2++) {
        if(tested_precoded_sinr[tested_i2] > tested_precoded_sinr[i2[0]]) {
          i2[0] = tested_i2;
        }
      }
      *precoded_sinr_dB = dB_fixed(tested_precoded_sinr[i2[0]]);
    } else {
      i2[0] = tested_precoded_sinr[0]+tested_precoded_sinr[2] > tested_precoded_sinr[1]+tested_precoded_sinr[3] ? 0 : 1;
      *precoded_sinr_dB = dB_fixed((tested_precoded_sinr[i2[0]] + tested_precoded_sinr[i2[0]+2])>>1);
    }

  } else {
    LOG_W(NR_PHY, "PMI computation is not implemented for rank indicator %i\n", rank_indicator+1);
    return -1;
  }

  return 0;
}

int nr_csi_rs_cqi_estimation(const uint32_t precoded_sinr,
                             uint8_t *cqi) {

  *cqi = 0;

  // Default SINR table for an AWGN channel for SISO scenario, considering 0.1 BLER condition and TS 38.214 Table 5.2.2.1-2
  if(precoded_sinr>0 && precoded_sinr<=2) {
    *cqi = 4;
  } else if(precoded_sinr==3) {
    *cqi = 5;
  } else if(precoded_sinr>3 && precoded_sinr<=5) {
    *cqi = 6;
  } else if(precoded_sinr>5 && precoded_sinr<=7) {
    *cqi = 7;
  } else if(precoded_sinr>7 && precoded_sinr<=9) {
    *cqi = 8;
  } else if(precoded_sinr==10) {
    *cqi = 9;
  } else if(precoded_sinr>10 && precoded_sinr<=12) {
    *cqi = 10;
  } else if(precoded_sinr>12 && precoded_sinr<=15) {
    *cqi = 11;
  } else if(precoded_sinr==16) {
    *cqi = 12;
  } else if(precoded_sinr>16 && precoded_sinr<=18) {
    *cqi = 13;
  } else if(precoded_sinr==19) {
    *cqi = 14;
  } else if(precoded_sinr>19) {
    *cqi = 15;
  }

  return 0;
}

int nr_csi_im_power_estimation(const PHY_VARS_NR_UE *ue,
                               const UE_nr_rxtx_proc_t *proc,
                               const fapi_nr_dl_config_csiim_pdu_rel15_t *csiim_config_pdu,
                               uint32_t *interference_plus_noise_power,
                               c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]) {

  const NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  const uint16_t end_rb = csiim_config_pdu->start_rb + csiim_config_pdu->nr_of_rbs > csiim_config_pdu->bwp_size ?
                          csiim_config_pdu->bwp_size : csiim_config_pdu->start_rb + csiim_config_pdu->nr_of_rbs;

  int32_t count = 0;
  int32_t sum_re = 0;
  int32_t sum_im = 0;
  int32_t sum2_re = 0;
  int32_t sum2_im = 0;

  int l_csiim[4] = {-1, -1, -1, -1};

  for(int symb_idx = 0; symb_idx < 4; symb_idx++) {

    uint8_t symb = csiim_config_pdu->l_csiim[symb_idx];
    bool done = false;
    for (int symb_idx2 = 0; symb_idx2 < symb_idx; symb_idx2++) {
      if (l_csiim[symb_idx2] == symb) {
        done = true;
      }
    }

    if (done) {
      continue;
    }

    l_csiim[symb_idx] = symb;
    uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;

    for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {

      c16_t *rx_signal = &rxdataF[ant_rx][symbol_offset];

      for (int rb = csiim_config_pdu->start_rb; rb < end_rb; rb++) {

        uint16_t sc0_offset = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;

        for (int sc_idx = 0; sc_idx<4; sc_idx++) {

          uint16_t sc = sc0_offset + csiim_config_pdu->k_csiim[sc_idx];
          if (sc >= frame_parms->ofdm_symbol_size) {
            sc -= frame_parms->ofdm_symbol_size;
          }

#ifdef NR_CSIIM_DEBUG
          LOG_I(NR_PHY, "(ant_rx %i, sc %i) real %i, imag %i\n", ant_rx, sc, rx_signal[sc].r, rx_signal[sc].i);
#endif

          sum_re += rx_signal[sc].r;
          sum_im += rx_signal[sc].i;
          sum2_re += rx_signal[sc].r*rx_signal[sc].r;
          sum2_im += rx_signal[sc].i*rx_signal[sc].i;
          count++;
        }
      }
    }
  }

  int32_t power_re = sum2_re/count - (sum_re/count)*(sum_re/count);
  int32_t power_im = sum2_im/count - (sum_im/count)*(sum_im/count);

  *interference_plus_noise_power = power_re + power_im;

#ifdef NR_CSIIM_DEBUG
  LOG_I(NR_PHY, "interference_plus_noise_power based on CSI-IM = %i\n", *interference_plus_noise_power);
#endif

  return 0;
}

int nr_ue_csi_im_procedures(PHY_VARS_NR_UE *ue,
                            const UE_nr_rxtx_proc_t *proc,
                            c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  int gNB_id = proc->gNB_id;
  if(!ue->csiim_vars[gNB_id]->active) {
    return -1;
  }

  const fapi_nr_dl_config_csiim_pdu_rel15_t *csiim_config_pdu = (fapi_nr_dl_config_csiim_pdu_rel15_t*)&ue->csiim_vars[gNB_id]->csiim_config_pdu;

#ifdef NR_CSIIM_DEBUG
  LOG_I(NR_PHY, "csiim_config_pdu->bwp_size = %i\n", csiim_config_pdu->bwp_size);
  LOG_I(NR_PHY, "csiim_config_pdu->bwp_start = %i\n", csiim_config_pdu->bwp_start);
  LOG_I(NR_PHY, "csiim_config_pdu->subcarrier_spacing = %i\n", csiim_config_pdu->subcarrier_spacing);
  LOG_I(NR_PHY, "csiim_config_pdu->start_rb = %i\n", csiim_config_pdu->start_rb);
  LOG_I(NR_PHY, "csiim_config_pdu->nr_of_rbs = %i\n", csiim_config_pdu->nr_of_rbs);
  LOG_I(NR_PHY, "csiim_config_pdu->k_csiim = %i.%i.%i.%i\n", csiim_config_pdu->k_csiim[0], csiim_config_pdu->k_csiim[1], csiim_config_pdu->k_csiim[2], csiim_config_pdu->k_csiim[3]);
  LOG_I(NR_PHY, "csiim_config_pdu->l_csiim = %i.%i.%i.%i\n", csiim_config_pdu->l_csiim[0], csiim_config_pdu->l_csiim[1], csiim_config_pdu->l_csiim[2], csiim_config_pdu->l_csiim[3]);
#endif

  nr_csi_im_power_estimation(ue, proc, csiim_config_pdu, &ue->nr_csi_info->interference_plus_noise_power, rxdataF);
  ue->nr_csi_info->csi_im_meas_computed = true;

  return 0;
}

static nfapi_nr_dl_tti_csi_rs_pdu_rel15_t convert_csirs_pdu(const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu)
{
  nfapi_nr_dl_tti_csi_rs_pdu_rel15_t dl_tti_csi_rs_pdu;
  dl_tti_csi_rs_pdu.subcarrier_spacing = csirs_config_pdu->subcarrier_spacing;
  dl_tti_csi_rs_pdu.cyclic_prefix = csirs_config_pdu->cyclic_prefix;
  dl_tti_csi_rs_pdu.start_rb = csirs_config_pdu->start_rb;
  dl_tti_csi_rs_pdu.nr_of_rbs = csirs_config_pdu->nr_of_rbs;
  dl_tti_csi_rs_pdu.csi_type = csirs_config_pdu->csi_type;
  dl_tti_csi_rs_pdu.row = csirs_config_pdu->row;
  dl_tti_csi_rs_pdu.freq_domain = csirs_config_pdu->freq_domain;
  dl_tti_csi_rs_pdu.symb_l0 = csirs_config_pdu->symb_l0;
  dl_tti_csi_rs_pdu.symb_l1 = csirs_config_pdu->symb_l1;
  dl_tti_csi_rs_pdu.cdm_type = csirs_config_pdu->cdm_type;
  dl_tti_csi_rs_pdu.freq_density = csirs_config_pdu->freq_density;
  dl_tti_csi_rs_pdu.scramb_id = csirs_config_pdu->scramb_id;
  dl_tti_csi_rs_pdu.power_control_offset = csirs_config_pdu->power_control_offset;
  dl_tti_csi_rs_pdu.power_control_offset_ss = csirs_config_pdu->power_control_offset_ss;
  return dl_tti_csi_rs_pdu;
}

void nr_ue_csi_rs_procedures(PHY_VARS_NR_UE *ue,
                             const UE_nr_rxtx_proc_t *proc,
                             c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{

  int gNB_id = proc->gNB_id;
  if(!ue->csirs_vars[gNB_id]->active) {
    return;
  }

  const fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu = &ue->csirs_vars[gNB_id]->csirs_config_pdu;

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "csirs_config_pdu->subcarrier_spacing = %i\n", csirs_config_pdu->subcarrier_spacing);
  LOG_I(NR_PHY, "csirs_config_pdu->cyclic_prefix = %i\n", csirs_config_pdu->cyclic_prefix);
  LOG_I(NR_PHY, "csirs_config_pdu->start_rb = %i\n", csirs_config_pdu->start_rb);
  LOG_I(NR_PHY, "csirs_config_pdu->nr_of_rbs = %i\n", csirs_config_pdu->nr_of_rbs);
  LOG_I(NR_PHY, "csirs_config_pdu->csi_type = %i (0:TRS, 1:CSI-RS NZP, 2:CSI-RS ZP)\n", csirs_config_pdu->csi_type);
  LOG_I(NR_PHY, "csirs_config_pdu->row = %i\n", csirs_config_pdu->row);
  LOG_I(NR_PHY, "csirs_config_pdu->freq_domain = %i\n", csirs_config_pdu->freq_domain);
  LOG_I(NR_PHY, "csirs_config_pdu->symb_l0 = %i\n", csirs_config_pdu->symb_l0);
  LOG_I(NR_PHY, "csirs_config_pdu->symb_l1 = %i\n", csirs_config_pdu->symb_l1);
  LOG_I(NR_PHY, "csirs_config_pdu->cdm_type = %i\n", csirs_config_pdu->cdm_type);
  LOG_I(NR_PHY, "csirs_config_pdu->freq_density = %i (0: dot5 (even RB), 1: dot5 (odd RB), 2: one, 3: three)\n", csirs_config_pdu->freq_density);
  LOG_I(NR_PHY, "csirs_config_pdu->scramb_id = %i\n", csirs_config_pdu->scramb_id);
  LOG_I(NR_PHY, "csirs_config_pdu->power_control_offset = %i\n", csirs_config_pdu->power_control_offset);
  LOG_I(NR_PHY, "csirs_config_pdu->power_control_offset_ss = %i\n", csirs_config_pdu->power_control_offset_ss);
#endif

  if(csirs_config_pdu->measurement_bitmap == 0) {
    LOG_E(NR_PHY, "Handling of CSI-RS for tracking not handled yet at PHY\n");
    return;
  }

  const NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int32_t csi_rs_received_signal[frame_parms->nb_antennas_rx][frame_parms->samples_per_slot_wCP];
  uint8_t N_cdm_groups = 0;
  uint8_t CDM_group_size = 0;
  uint8_t k_prime = 0;
  uint8_t l_prime = 0;
  uint8_t N_ports = 0;
  uint8_t j_cdm[16];
  uint8_t k_overline[16];
  uint8_t l_overline[16];
  int16_t log2_re = 0;
  int16_t log2_maxh = 0;
  uint32_t rsrp = 0;
  int rsrp_dBm = 0;
  uint32_t noise_power = 0;
  uint8_t rank_indicator = 0;
  uint32_t precoded_sinr_dB = 0;
  uint8_t cqi = 0;
  uint8_t i1[3] = {0};
  uint8_t i2[1] = {0};
  nfapi_nr_dl_tti_csi_rs_pdu_rel15_t csi_params = convert_csirs_pdu(csirs_config_pdu);
  nr_generate_csi_rs(frame_parms,
                     ue->nr_csi_info->csi_rs_generated_signal,
                     AMP,
                     ue->nr_csi_info,
                     &csi_params,
                     proc->nr_slot_rx,
                     &N_cdm_groups,
                     &CDM_group_size,
                     &k_prime,
                     &l_prime,
                     &N_ports,
                     j_cdm,
                     k_overline,
                     l_overline);

  int32_t csi_rs_ls_estimated_channel[frame_parms->nb_antennas_rx][N_ports][frame_parms->ofdm_symbol_size];
  int32_t csi_rs_estimated_channel_freq[frame_parms->nb_antennas_rx][N_ports][frame_parms->ofdm_symbol_size + FILTER_MARGIN];

  // (long)&csi_rs_estimated_channel_freq[0][0][frame_parms->first_carrier_offset] & 0x1F gives us the remainder of the integer division by 32 of the memory address
  // By subtracting the previous value of 32, we know how much is left to have a multiple of 32.
  // Doing >> 2 <=> /sizeof(int32_t), we know what is the index offset of the array.
  uint8_t mem_offset = (((32 - ((long)&csi_rs_estimated_channel_freq[0][0][frame_parms->first_carrier_offset])) & 0x1F) >> 2);

  nr_get_csi_rs_signal(ue,
                       proc,
                       csirs_config_pdu,
                       ue->nr_csi_info,
                       N_cdm_groups,
                       CDM_group_size,
                       k_prime,
                       l_prime,
                       j_cdm,
                       k_overline,
                       l_overline,
                       csi_rs_received_signal,
                       &rsrp,
                       &rsrp_dBm,
                       rxdataF);


  // if we need to measure only RSRP no need to do channel estimation
  if (csirs_config_pdu->measurement_bitmap > 1)
    nr_csi_rs_channel_estimation(ue,
                                 proc,
                                 csirs_config_pdu,
                                 ue->nr_csi_info,
                                 (const int32_t **) ue->nr_csi_info->csi_rs_generated_signal,
                                 csi_rs_received_signal,
                                 N_cdm_groups,
                                 CDM_group_size,
                                 k_prime,
                                 l_prime,
                                 N_ports,
                                 j_cdm,
                                 k_overline,
                                 l_overline,
                                 mem_offset,
                                 csi_rs_ls_estimated_channel,
                                 csi_rs_estimated_channel_freq,
                                 &log2_re,
                                 &log2_maxh,
                                 &noise_power);

  // bit 1 in bitmap to indicate RI measurment
  if (csirs_config_pdu->measurement_bitmap & 2) {
    nr_csi_rs_ri_estimation(ue,
                            csirs_config_pdu,
                            ue->nr_csi_info,
                            N_ports,
                            mem_offset,
                            csi_rs_estimated_channel_freq,
                            log2_maxh,
                            &rank_indicator);
  }

  // bit 3 in bitmap to indicate RI measurment
  if (csirs_config_pdu->measurement_bitmap & 8) {
    nr_csi_rs_pmi_estimation(ue,
                             csirs_config_pdu,
                             ue->nr_csi_info,
                             N_ports,
                             mem_offset,
                             csi_rs_estimated_channel_freq,
                             ue->nr_csi_info->csi_im_meas_computed ? ue->nr_csi_info->interference_plus_noise_power : noise_power,
                             rank_indicator,
                             log2_re,
                             i1,
                             i2,
                             &precoded_sinr_dB);

    // bit 4 in bitmap to indicate RI measurment
    if(csirs_config_pdu->measurement_bitmap & 16)
      nr_csi_rs_cqi_estimation(precoded_sinr_dB, &cqi);
  }

  switch (csirs_config_pdu->measurement_bitmap) {
    case 1 :
      LOG_I(NR_PHY, "RSRP = %i dBm\n", rsrp_dBm);
      break;
    case 26 :
      LOG_I(NR_PHY, "RI = %i i1 = %i.%i.%i, i2 = %i, SINR = %i dB, CQI = %i\n",
            rank_indicator + 1, i1[0], i1[1], i1[2], i2[0], precoded_sinr_dB, cqi);
      break;
    case 27 :
      LOG_I(NR_PHY, "RSRP = %i dBm, RI = %i i1 = %i.%i.%i, i2 = %i, SINR = %i dB, CQI = %i\n",
            rsrp_dBm, rank_indicator + 1, i1[0], i1[1], i1[2], i2[0], precoded_sinr_dB, cqi);
      break;
    default :
      AssertFatal(false, "Not supported measurement configuration\n");
  }

  // Send CSI measurements to MAC
  fapi_nr_csirs_measurements_t csirs_measurements;
  csirs_measurements.rsrp = rsrp;
  csirs_measurements.rsrp_dBm = rsrp_dBm;
  csirs_measurements.rank_indicator = rank_indicator;
  csirs_measurements.i1 = *i1;
  csirs_measurements.i2 = *i2;
  csirs_measurements.cqi = cqi;
  csirs_measurements.radiolink_monitoring = RLM_no_monitoring; // TODO do be activated in case of RLM based on CSI-RS
  nr_downlink_indication_t dl_indication;
  fapi_nr_rx_indication_t rx_ind = {0};
  nr_fill_dl_indication(&dl_indication, NULL, &rx_ind, proc, ue, NULL);
  nr_fill_rx_indication(&rx_ind, FAPI_NR_CSIRS_IND, ue, NULL, NULL, 1, proc, (void *)&csirs_measurements, NULL);
  if (ue->if_inst && ue->if_inst->dl_indication)
    ue->if_inst->dl_indication(&dl_indication);

  return;
}
