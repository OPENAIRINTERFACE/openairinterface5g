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

/*! \file nr_ue_measurements.c
 * \brief UE measurements routines
 * \author  R. Knopp, G. Casati, K. Saaifan
 * \date 2020
 * \version 0.1
 * \company Eurecom, Fraunhofer IIS
 * \email: knopp@eurecom.fr, guido.casati@iis.fraunhofer.de, khodr.saaifan@iis.fraunhofer.de
 * \note
 * \warning
 */

#include "executables/softmodem-common.h"
#include "executables/nr-softmodem-common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "common/utils/LOG/log.h"
#include "PHY/sse_intrin.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"

//#define k1 1000
#define k1 ((long long int) 1000)
#define k2 ((long long int) (1024-k1))

//#define DEBUG_MEAS_RRC
//#define DEBUG_MEAS_UE
//#define DEBUG_RANK_EST

// Returns the pathloss in dB for the active UL BWP on the selected carrier based on the DL RS associated with the PRACH transmission
// computation according to clause 7.4 (Physical random access channel) of 3GPP TS 38.213 version 16.3.0 Release 16
// Assumptions:
// - PRACH transmission from a UE is not in response to a detection of a PDCCH order by the UE
// Measurement units:
// - referenceSignalPower:   dBm/RE (average EPRE of the resources elements that carry secondary synchronization signals in dBm)
int16_t get_nr_PL(uint8_t Mod_id, uint8_t CC_id, uint8_t gNB_index){

  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];
  int16_t pathloss;

  if (get_softmodem_params()->do_ra){

    long referenceSignalPower = ue->nrUE_config.ssb_config.ss_pbch_power;

    pathloss = (int16_t)(referenceSignalPower - 30 + ue->rx_total_gain_dB);

    LOG_D(MAC, "In %s: pathloss %d dB, UE RX total gain %d dB, referenceSignalPower %ld dBm/RE (%f mW), RSRP %d dBm (%f mW)\n",
      __FUNCTION__,
      pathloss,
      ue->rx_total_gain_dB,
      referenceSignalPower,
      pow(10, referenceSignalPower/10),
      ue->measurements.rsrp_dBm[gNB_index],
      pow(10, ue->measurements.rsrp_dBm[gNB_index]/10));

  } else {

    pathloss = ((int16_t)(((10*ue->rx_total_gain_dB) - dB_fixed_times10(ue->measurements.rsrp[gNB_index]))/10));

  }

  return pathloss;

}

uint32_t get_nr_rx_total_gain_dB (module_id_t Mod_id,uint8_t CC_id)
{

  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->rx_total_gain_dB;

  return 0xFFFFFFFF;
}


float_t get_nr_RSRP(module_id_t Mod_id,uint8_t CC_id,uint8_t gNB_index)
{

  AssertFatal(PHY_vars_UE_g!=NULL,"PHY_vars_UE_g is null\n");
  AssertFatal(PHY_vars_UE_g[Mod_id]!=NULL,"PHY_vars_UE_g[%d] is null\n",Mod_id);
  AssertFatal(PHY_vars_UE_g[Mod_id][CC_id]!=NULL,"PHY_vars_UE_g[%d][%d] is null\n",Mod_id,CC_id);

  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return (10*log10(ue->measurements.rsrp[gNB_index])-
	    get_nr_rx_total_gain_dB(Mod_id,0) -
	    10*log10(20*12));
  return -140.0;
}

void nr_ue_measurements(PHY_VARS_NR_UE *ue,
                        UE_nr_rxtx_proc_t *proc,
                        uint8_t slot)
{
  int aarx, aatx, gNB_id = 0;
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int ch_offset = frame_parms->ofdm_symbol_size*2;
  NR_UE_DLSCH_t *dlsch = ue->dlsch[proc->thread_id][gNB_id][0];
  uint8_t harq_pid = dlsch->current_harq_pid;
  int N_RB_DL = dlsch->harq_processes[harq_pid]->nb_rb;

  ue->measurements.nb_antennas_rx = frame_parms->nb_antennas_rx;

  // signal measurements
  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++){

    ue->measurements.rx_power_tot[gNB_id] = 0;

    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++){

      ue->measurements.rx_power[gNB_id][aarx] = 0;

      for (aatx = 0; aatx < frame_parms->nb_antenna_ports_gNB; aatx++){

        ue->measurements.rx_spatial_power[gNB_id][aatx][aarx] = (signal_energy_nodc(&ue->pdsch_vars[proc->thread_id][0]->dl_ch_estimates[gNB_id][ch_offset], N_RB_DL*NR_NB_SC_PER_RB));

        if (ue->measurements.rx_spatial_power[gNB_id][aatx][aarx]<0)
          ue->measurements.rx_spatial_power[gNB_id][aatx][aarx] = 0;

        ue->measurements.rx_spatial_power_dB[gNB_id][aatx][aarx] = (unsigned short) dB_fixed(ue->measurements.rx_spatial_power[gNB_id][aatx][aarx]);
        ue->measurements.rx_power[gNB_id][aarx] += ue->measurements.rx_spatial_power[gNB_id][aatx][aarx];

      }

      ue->measurements.rx_power_dB[gNB_id][aarx] = (unsigned short) dB_fixed(ue->measurements.rx_power[gNB_id][aarx]);
      ue->measurements.rx_power_tot[gNB_id] += ue->measurements.rx_power[gNB_id][aarx];

    }

    ue->measurements.rx_power_tot_dB[gNB_id] = (unsigned short) dB_fixed(ue->measurements.rx_power_tot[gNB_id]);

  }

  // filter to remove jitter
  if (ue->init_averaging == 0) {

    for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++)
      ue->measurements.rx_power_avg[gNB_id] = (int)(((k1*((long long int)(ue->measurements.rx_power_avg[gNB_id]))) + (k2*((long long int)(ue->measurements.rx_power_tot[gNB_id])))) >> 10);

    ue->measurements.n0_power_avg = (int)(((k1*((long long int) (ue->measurements.n0_power_avg))) + (k2*((long long int) (ue->measurements.n0_power_tot))))>>10);

    LOG_D(PHY, "Noise Power Computation: k1 %lld k2 %lld n0 avg %u n0 tot %u\n", k1, k2, ue->measurements.n0_power_avg, ue->measurements.n0_power_tot);

  } else {

    for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++)
      ue->measurements.rx_power_avg[gNB_id] = ue->measurements.rx_power_tot[gNB_id];

    ue->measurements.n0_power_avg = ue->measurements.n0_power_tot;
    ue->init_averaging = 0;

  }

  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {

    ue->measurements.rx_power_avg_dB[gNB_id] = dB_fixed( ue->measurements.rx_power_avg[gNB_id]);
    ue->measurements.wideband_cqi_tot[gNB_id] = dB_fixed2(ue->measurements.rx_power_tot[gNB_id], ue->measurements.n0_power_tot);
    ue->measurements.wideband_cqi_avg[gNB_id] = dB_fixed2(ue->measurements.rx_power_avg[gNB_id], ue->measurements.n0_power_avg);
    ue->measurements.rx_rssi_dBm[gNB_id] = ue->measurements.rx_power_avg_dB[gNB_id] + 30 - 10*log10(pow(2, 30)) - ((int)openair0_cfg[0].rx_gain[0] - (int)openair0_cfg[0].rx_gain_offset[0]) - dB_fixed(ue->frame_parms.ofdm_symbol_size);

    LOG_D(PHY, "[gNB %d] Slot %d, RSSI %d dB (%d dBm/RE), WBandCQI %d dB, rxPwrAvg %d, n0PwrAvg %d\n",
      gNB_id,
      slot,
      ue->measurements.rx_power_avg_dB[gNB_id],
      ue->measurements.rx_rssi_dBm[gNB_id],
      ue->measurements.wideband_cqi_avg[gNB_id],
      ue->measurements.rx_power_avg[gNB_id],
      ue->measurements.n0_power_tot);
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

// This function implements:
// - SS reference signal received power (SS-RSRP) as per clause 5.1.1 of 3GPP TS 38.215 version 16.3.0 Release 16
// - no Layer 3 filtering implemented (no filterCoefficient provided from RRC)
// Todo:
// - Layer 3 filtering according to clause 5.5.3.2 of 3GPP TS 38.331 version 16.2.0 Release 16
// Measurement units:
// - RSRP:    W (dBW)
// - RX Gain  dB
void nr_ue_rsrp_measurements(PHY_VARS_NR_UE *ue,
                             uint8_t gNB_id,
                             UE_nr_rxtx_proc_t *proc,
                             uint8_t slot,
                             uint8_t abstraction_flag)
{
  int aarx;
  int nb_re;
  int k_start = 55;
  int k_end   = 183;
  unsigned int ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  uint8_t           l_sss = ue->symbol_offset + 2;

  if (ssb_offset>= ue->frame_parms.ofdm_symbol_size){

    ssb_offset -= ue->frame_parms.ofdm_symbol_size;

  }

  ue->measurements.rsrp[gNB_id] = 0;

  if (abstraction_flag == 0) {

    LOG_D(PHY, "In %s: [UE %d] slot %d l_sss %d ssb_offset %d\n", __FUNCTION__, ue->Mod_id, slot, l_sss, ssb_offset);

    nb_re = 0;

    for (aarx = 0; aarx < ue->frame_parms.nb_antennas_rx; aarx++) {

      int16_t *rxF_sss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].rxdataF[aarx][(l_sss*ue->frame_parms.ofdm_symbol_size) + ssb_offset];

      for(int k = k_start; k < k_end; k++){

        #ifdef DEBUG_MEAS_UE
        LOG_I(PHY, "In %s rxF_sss %d %d\n", __FUNCTION__, rxF_sss[k*2], rxF_sss[k*2 + 1]);
        #endif

        ue->measurements.rsrp[gNB_id] += (((int32_t)rxF_sss[k*2]*rxF_sss[k*2]) + ((int32_t)rxF_sss[k*2 + 1]*rxF_sss[k*2 + 1]));

        nb_re++;

      }
    }

    ue->measurements.rsrp[gNB_id] /= nb_re;

  } else {

    ue->measurements.rsrp[gNB_id] = -93;

  }

  ue->measurements.rsrp_filtered[gNB_id] = ue->measurements.rsrp[gNB_id];

  ue->measurements.rsrp_dBm[gNB_id] = 10*log10(ue->measurements.rsrp[gNB_id]) + 30 - 10*log10(pow(2,30)) - ((int)openair0_cfg[0].rx_gain[0] - (int)openair0_cfg[0].rx_gain_offset[0]) - dB_fixed(ue->frame_parms.ofdm_symbol_size);

  LOG_D(PHY, "In %s: [UE %d] slot %d SS-RSRP: %d dBm/RE (%d)\n",
    __FUNCTION__,
    ue->Mod_id,
    slot,
    ue->measurements.rsrp_dBm[gNB_id],
    ue->measurements.rsrp[gNB_id]);

}

// This function computes the received noise power
// Measurement units:
// - psd_awgn (AWGN power spectral density):     dBm/Hz
void nr_ue_rrc_measurements(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            uint8_t slot){

  uint8_t k;
  int aarx, nb_nulls;
  int16_t *rxF_sss;
  uint8_t k_left = 48;
  uint8_t k_right = 183;
  uint8_t k_length = 8;
  uint8_t l_sss = ue->symbol_offset + 2;
  unsigned int ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  double rx_gain = openair0_cfg[0].rx_gain[0];
  double rx_gain_offset = openair0_cfg[0].rx_gain_offset[0];

  ue->measurements.n0_power_tot = 0;

  LOG_D(PHY, "In %s doing measurements for ssb_offset %d l_sss %d \n", __FUNCTION__, ssb_offset, l_sss);

  for (aarx = 0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    nb_nulls = 0;
    ue->measurements.n0_power[aarx] = 0;
    rxF_sss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].rxdataF[aarx][(l_sss*ue->frame_parms.ofdm_symbol_size) + ssb_offset];

    //-ve spectrum from SSS
    for(k = k_left; k < k_left + k_length; k++){

      #ifdef DEBUG_MEAS_RRC
      LOG_I(PHY, "In %s -rxF_sss %d %d\n", __FUNCTION__, rxF_sss[k*2], rxF_sss[k*2 + 1]);
      #endif

      ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[k*2]*rxF_sss[k*2]) + ((int32_t)rxF_sss[k*2 + 1]*rxF_sss[k*2 + 1]));
      nb_nulls++;

    }

    //+ve spectrum from SSS
    for(k = k_right; k < k_right + k_length; k++){

      #ifdef DEBUG_MEAS_RRC
      LOG_I(PHY, "In %s +rxF_sss %d %d\n", __FUNCTION__, rxF_sss[k*2], rxF_sss[k*2 + 1]);
      #endif

      ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[k*2]*rxF_sss[k*2]) + ((int32_t)rxF_sss[k*2 + 1]*rxF_sss[k*2 + 1]));
      nb_nulls++;

    }

    ue->measurements.n0_power[aarx] /= nb_nulls;
    ue->measurements.n0_power_dB[aarx] = (unsigned short) dB_fixed(ue->measurements.n0_power[aarx]);
    ue->measurements.n0_power_tot += ue->measurements.n0_power[aarx];

  }

  ue->measurements.n0_power_tot_dB = (unsigned short) dB_fixed(ue->measurements.n0_power_tot/aarx);

  #ifdef DEBUG_MEAS_RRC
  const int psd_awgn = -174;
  const int scs = 15000 * (1 << ue->frame_parms.numerology_index);
  const int nf_usrp = ue->measurements.n0_power_tot_dB + 3 + 30 - ((int)rx_gain - (int)rx_gain_offset) - 10 * log10(pow(2, 30)) - (psd_awgn + dB_fixed(scs) + dB_fixed(ue->frame_parms.ofdm_symbol_size));
  LOG_D(PHY, "In [%s][slot:%d] NF USRP %d dB\n", __FUNCTION__, slot, nf_usrp);
  #endif

  LOG_D(PHY, "In [%s][slot:%d] Noise Level %d (digital level %d dB, noise power spectral density %f dBm/RE)\n",
    __FUNCTION__,
    slot,
    ue->measurements.n0_power_tot,
    ue->measurements.n0_power_tot_dB,
    ue->measurements.n0_power_tot_dB + 30 - 10*log10(pow(2, 30)) - dB_fixed(ue->frame_parms.ofdm_symbol_size) - ((int)rx_gain - (int)rx_gain_offset));

}
