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

/*! \file ra_procedures.c
 * \brief Routines for UE MAC-layer power control procedures
 * \author Francesco Mani
 * \date 2023
 * \version 0.1
 * \email: email@francescomani.it
 * \note
 * \warning
 */

#include "LAYER2/NR_MAC_UE/mac_proto.h"

// Implementation of 6.2.4 Configured ransmitted power
// 3GPP TS 38.101-1 version 16.5.0 Release 16
// -
// The UE is allowed to set its configured maximum output power PCMAX,f,c for carrier f of serving cell c in each slot.
// The configured maximum output power PCMAX,f,c is set within the following bounds: PCMAX_L,f,c <=  PCMAX,f,c <=  PCMAX_H,f,c
// -
// Measurement units:
// - p_max:              dBm
// - delta_TC_c:         dB
// - P_powerclass:       dBm
// - delta_P_powerclass: dB
// - MPR_c:              dB
// - delta_MPR_c:        dB
// - delta_T_IB_c        dB
// - delta_rx_SRS        dB
// note:
// - Assuming:
// -- Powerclass 3 capable UE (which is default power class unless otherwise stated)
// -- Maximum power reduction (MPR_c) for power class 3
// -- no additional MPR (A_MPR_c)
int nr_get_Pcmax(NR_UE_MAC_INST_t *mac, int Qm, bool powerBoostPi2BPSK, int scs, int N_RB_UL, bool is_transform_precoding, int n_prbs, int start_prb)
{
  int nr_band = mac->nr_band;
  if(mac->frequency_range == FR1) {

    //TODO configure P-MAX from the upper layers according to 38.331
    int p_powerclass = 23; // dBm assuming poweclass 3 UE
    int p_emax = mac->p_Max != INT_MIN ? mac->p_Max : p_powerclass;
    int delta_P_powerclass = 0; // for powerclass 2 needs to be changed
    if(mac->p_Max && Qm == 1 && powerBoostPi2BPSK && (nr_band == 40 || nr_band == 41 || nr_band == 77 || nr_band == 78 || nr_band == 79)) {
      p_emax += 3;
      delta_P_powerclass -= 3;
    }

    // TODO to be set for CA and DC
    int delta_T_IB = 0;

    // TODO in case of band 41 and PRB allocation within 4MHz of the upper or lower limit of the band -> delta_TC = 1.5
    if(nr_band == 41)
      LOG_E(NR_MAC, "Need to implement delta_TC for band 41\n");
    int delta_TC = 0;

    float MPR = 0;
    frame_type_t frame_type = get_frame_type(nr_band, scs);
    if(compare_relative_ul_channel_bw(nr_band, scs, N_RB_UL, frame_type)) {
      int rb_low = (n_prbs / 2) > 1 ? (n_prbs / 2) : 1;
      int rb_high = N_RB_UL - rb_low - n_prbs;
      bool is_inner_rb = start_prb >= rb_low && start_prb <= rb_high && n_prbs <= ((N_RB_UL / 2) + (N_RB_UL & 1));
      // Table 6.2.2-1 in 38.101
      switch (Qm) {
        case 1 :
          AssertFatal(false, "MPR for Pi/2 BPSK not implemented yet\n");
          break;
        case 2 :
          if (is_transform_precoding) {
            if(!is_inner_rb)
              MPR = 1;
          }
          else {
            if(is_inner_rb)
              MPR = 1.5;
            else
              MPR = 3;
          }
          break;
        case 4 :
          if (is_transform_precoding) {
            if(is_inner_rb)
              MPR = 1;
            else
              MPR = 2;
          }
          else {
            if(is_inner_rb)
              MPR = 2;
            else
              MPR = 3;
          }
          break;
        case 6 :
          if (is_transform_precoding)
            MPR = 2.5;
          else
            MPR = 3.5;
          break;
        case 8 :
          if (is_transform_precoding)
            MPR = 4.5;
          else
            MPR = 6.5;
          break;
          break;
        default:
          AssertFatal(false, "Invalid Qm %d\n", Qm);
      }
    }

    int A_MPR = 0; // TODO too complicated to implement for now (see 6.2.3 in 38.101-1)
    int delta_rx_SRS = 0; // TODO for SRS
    int P_MPR = 0; // to ensure compliance with applicable electromagnetic energy absorption requirements

    float total_reduction = (MPR > A_MPR ? MPR : A_MPR) + delta_T_IB + delta_TC + delta_rx_SRS;
    if (P_MPR > total_reduction)
      total_reduction = P_MPR;
    int pcmax_high, pcmax_low;
    if(mac->p_Max) {
      pcmax_high = p_emax < (p_powerclass - delta_P_powerclass) ? p_emax : (p_powerclass - delta_P_powerclass);
      pcmax_low = (p_emax - delta_TC) < (p_powerclass - delta_P_powerclass - total_reduction) ?
                  (p_emax - delta_TC) : (p_powerclass - delta_P_powerclass - total_reduction);
    }
    else {
      pcmax_high = p_powerclass - delta_P_powerclass;
      pcmax_low = p_powerclass - delta_P_powerclass - total_reduction;
    }
    // TODO we need a strategy to select a value between minimum and maximum allowed PC_max
    int pcmax = (pcmax_low + pcmax_high) / 2;
    LOG_D(MAC, "Configured maximum output power:  %d dBm <= PCMAX %d dBm <= %d dBm \n", pcmax_low, pcmax, pcmax_high);
    return pcmax;
  }
  else {
    // FR2 TODO it is even more complex because it is radiated power
    return 23;
  }
}

