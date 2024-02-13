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

/*! \file PHY/NR_TRANSPORT/srs_rx.c
 * \brief Top-level routines for getting the SRS physical channel
 * \date 2021
 * \version 1.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "PHY/INIT/nr_phy_init.h"
#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_RU.h"
#include "PHY/CODING/nrSmallBlock/nr_small_block_defs.h"
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
#include "common/utils/LOG/log.h"

#include "nfapi/oai_integration/vendor_ext.h"

#include "T.h"

//#define SRS_DEBUG

void nr_fill_srs(PHY_VARS_gNB *gNB, frame_t frame, slot_t slot, nfapi_nr_srs_pdu_t *srs_pdu)
{
  bool found = false;
  for (int i = 0; i < gNB->max_nb_srs; i++) {
    NR_gNB_SRS_t *srs = &gNB->srs[i];
    if (srs->active == 0) {
      found = true;
      srs->frame = frame;
      srs->slot = slot;
      srs->active = 1;
      memcpy((void *)&srs->srs_pdu, (void *)srs_pdu, sizeof(nfapi_nr_srs_pdu_t));
      break;
    }
  }
  AssertFatal(found, "SRS list is full\n");
}

int nr_get_srs_signal(PHY_VARS_gNB *gNB,
                      frame_t frame,
                      slot_t slot,
                      nfapi_nr_srs_pdu_t *srs_pdu,
                      nr_srs_info_t *nr_srs_info,
                      int32_t srs_received_signal[][gNB->frame_parms.ofdm_symbol_size*(1<<srs_pdu->num_symbols)]) {

#ifdef SRS_DEBUG
  LOG_I(NR_PHY,"Calling %s function\n", __FUNCTION__);
#endif

  c16_t **rxdataF = gNB->common_vars.rxdataF;
  const NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;

  const uint16_t n_symbols = (slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot; // number of symbols until this slot
  const uint8_t l0 = frame_parms->symbols_per_slot - 1 - srs_pdu->time_start_position;  // starting symbol in this slot
  const uint64_t symbol_offset = (n_symbols+l0)*frame_parms->ofdm_symbol_size;
  const uint64_t subcarrier_offset = frame_parms->first_carrier_offset + srs_pdu->bwp_start*NR_NB_SC_PER_RB;

  const uint8_t N_ap = 1<<srs_pdu->num_ant_ports;
  const uint8_t N_symb_SRS = 1<<srs_pdu->num_symbols;
  const uint8_t K_TC = 2<<srs_pdu->comb_size;
  const uint16_t M_sc_b_SRS = srs_bandwidth_config[srs_pdu->config_index][srs_pdu->bandwidth_index][0] * NR_NB_SC_PER_RB/K_TC;

  int32_t *rx_signal;
  bool no_srs_signal = true;
  for (int ant = 0; ant < frame_parms->nb_antennas_rx; ant++) {

    memset(srs_received_signal[ant], 0, frame_parms->ofdm_symbol_size*sizeof(int32_t));
    rx_signal = (int32_t *)&rxdataF[ant][symbol_offset];

    for (int p_index = 0; p_index < N_ap; p_index++) {

#ifdef SRS_DEBUG
      LOG_I(NR_PHY,"===== UE port %d --> gNB Rx antenna %i =====\n", p_index, ant);
#endif

      for (int l_line = 0; l_line < N_symb_SRS; l_line++) {

#ifdef SRS_DEBUG
        LOG_I(NR_PHY,":::::::: OFDM symbol %d ::::::::\n", l0+l_line);
#endif

        uint16_t subcarrier = subcarrier_offset + nr_srs_info->k_0_p[p_index][l_line];
        if (subcarrier>frame_parms->ofdm_symbol_size) {
          subcarrier -= frame_parms->ofdm_symbol_size;
        }
        uint16_t l_line_offset = l_line*frame_parms->ofdm_symbol_size;

        for (int k = 0; k < M_sc_b_SRS; k++) {

          srs_received_signal[ant][l_line_offset+subcarrier] = rx_signal[l_line_offset+subcarrier];

          if (rx_signal[l_line_offset+subcarrier] != 0) {
            no_srs_signal = false;
          }

#ifdef SRS_DEBUG
          int subcarrier_log = subcarrier-subcarrier_offset;
          if(subcarrier_log < 0) {
            subcarrier_log = subcarrier_log + frame_parms->ofdm_symbol_size;
          }
          if(subcarrier_log%12 == 0) {
            LOG_I(NR_PHY,"------------ %d ------------\n", subcarrier_log/12);
          }
          LOG_I(NR_PHY,"(%i)  \t%i\t%i\n",
                subcarrier_log,
                (int16_t)(srs_received_signal[ant][l_line_offset+subcarrier]&0xFFFF),
                (int16_t)((srs_received_signal[ant][l_line_offset+subcarrier]>>16)&0xFFFF));
#endif

          // Subcarrier increment
          subcarrier += K_TC;
          if (subcarrier >= frame_parms->ofdm_symbol_size) {
            subcarrier=subcarrier-frame_parms->ofdm_symbol_size;
          }

        } // for (int k = 0; k < M_sc_b_SRS; k++)
      } // for (int l_line = 0; l_line < N_symb_SRS; l_line++)
    } // for (int p_index = 0; p_index < N_ap; p_index++)
  } // for (int ant = 0; ant < frame_parms->nb_antennas_rx; ant++)

  if (no_srs_signal) {
    LOG_W(NR_PHY, "No SRS signal\n");
    return -1;
  } else {
    return 0;
  }
}
