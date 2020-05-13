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

#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "common/utils/LOG/log.h"
#include "PHY/sse_intrin.h"

//#define k1 1000
#define k1 ((long long int) 1000)
#define k2 ((long long int) (1024-k1))

//#define DEBUG_MEAS_RRC
//#define DEBUG_MEAS_UE
//#define DEBUG_RANK_EST

#if 0
int16_t cond_num_threshold = 0;

void print_shorts(char *s,short *x)
{


  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d\n",s,
         x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7]
        );

}
void print_ints(char *s,int *x)
{


  printf("%s  : %d,%d,%d,%d\n",s,
         x[0],x[1],x[2],x[3]
        );

}
#endif

int16_t get_nr_PL(PHY_VARS_NR_UE *ue,uint8_t gNB_index)
{

  /*
  if (ue->frame_parms.mode1_flag == 1)
    RSoffset = 6;
  else
    RSoffset = 3;
  */

 /* LOG_D(PHY,"get_nr_PL : rsrp %f dBm/RE (%f), eNB power %d dBm/RE\n", 
        (1.0*dB_fixed_times10(ue->measurements.rsrp[eNB_index])-(10.0*ue->rx_total_gain_dB))/10.0,
        10*log10((double)ue->measurements.rsrp[eNB_index]),
        ue->frame_parms.pdsch_config_common.referenceSignalPower);*/

  return((int16_t)(((10*ue->rx_total_gain_dB) - dB_fixed_times10(ue->measurements.rsrp[gNB_index]))/10));
                    //        dB_fixed_times10(RSoffset*12*ue_g[Mod_id][CC_id]->frame_parms.N_RB_DL) +
                    //(ue->frame_parms.pdsch_config_common.referenceSignalPower*10))/10));
}




void nr_ue_measurements(PHY_VARS_NR_UE *ue,
                         unsigned int subframe_offset,
                         unsigned char N0_symbol,
                         unsigned char abstraction_flag,
                         unsigned char rank_adaptation,
                         uint8_t subframe)
{
  int aarx,aatx,eNB_id=0; //,gain_offset=0;
  //int rx_power[NUMBER_OF_CONNECTED_eNB_MAX];

/*#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch0_128, *dl_ch1_128;
#elif defined(__arm__)
  int16x8_t *dl_ch0_128, *dl_ch1_128get_PL;
#endif*/

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  //unsigned int limit, subband;
  //int nb_subbands, subband_size, last_subband_size, *dl_ch0, *dl_ch1, N_RB_DL = frame_parms->N_RB_DL;

  int ch_offset, rank_tm3_tm4 = 0;

  ue->measurements.nb_antennas_rx = frame_parms->nb_antennas_rx;
  
  /*int16_t *dl_ch;
  dl_ch = (int16_t *)&ue->pdsch_vars[ue->current_thread_id[subframe]][0]->dl_ch_estimates[eNB_id][ch_offset];*/

  ch_offset = ue->frame_parms.ofdm_symbol_size*2;

//printf("testing measurements\n");
  // signal measurements

  for (eNB_id=0; eNB_id<ue->n_connected_eNB; eNB_id++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++) {
        ue->measurements.rx_spatial_power[eNB_id][aatx][aarx] =
          (signal_energy_nodc(&ue->pdsch_vars[ue->current_thread_id[subframe]][0]->dl_ch_estimates[eNB_id][ch_offset],
                              (50*12)));
        //- ue->measurements.n0_power[aarx];

        if (ue->measurements.rx_spatial_power[eNB_id][aatx][aarx]<0)
          ue->measurements.rx_spatial_power[eNB_id][aatx][aarx] = 0; //ue->measurements.n0_power[aarx];

        ue->measurements.rx_spatial_power_dB[eNB_id][aatx][aarx] = (unsigned short) dB_fixed(ue->measurements.rx_spatial_power[eNB_id][aatx][aarx]);

        if (aatx==0)
          ue->measurements.rx_power[eNB_id][aarx] = ue->measurements.rx_spatial_power[eNB_id][aatx][aarx];
        else
          ue->measurements.rx_power[eNB_id][aarx] += ue->measurements.rx_spatial_power[eNB_id][aatx][aarx];
      } //aatx

      ue->measurements.rx_power_dB[eNB_id][aarx] = (unsigned short) dB_fixed(ue->measurements.rx_power[eNB_id][aarx]);

      if (aarx==0)
        ue->measurements.rx_power_tot[eNB_id] = ue->measurements.rx_power[eNB_id][aarx];
      else
        ue->measurements.rx_power_tot[eNB_id] += ue->measurements.rx_power[eNB_id][aarx];
    } //aarx

    ue->measurements.rx_power_tot_dB[eNB_id] = (unsigned short) dB_fixed(ue->measurements.rx_power_tot[eNB_id]);

  } //eNB_id
  
  //printf("ue measurement addr dlch %p\n", dl_ch);

  eNB_id=0;

  if (ue->transmission_mode[eNB_id]!=4 && ue->transmission_mode[eNB_id]!=3)
    ue->measurements.rank[eNB_id] = 0;
  else
    ue->measurements.rank[eNB_id] = rank_tm3_tm4;
  //  printf ("tx mode %d\n", ue->transmission_mode[eNB_id]);
  //  printf ("rank %d\n", ue->PHY_measurements.rank[eNB_id]);

  // filter to remove jitter
  if (ue->init_averaging == 0) {
    for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++)
      ue->measurements.rx_power_avg[eNB_id] = (int)
          (((k1*((long long int)(ue->measurements.rx_power_avg[eNB_id]))) +
            (k2*((long long int)(ue->measurements.rx_power_tot[eNB_id]))))>>10);

    //LOG_I(PHY,"Noise Power Computation: k1 %d k2 %d n0 avg %d n0 tot %d\n", k1, k2, ue->measurements.n0_power_avg,
     //   ue->measurements.n0_power_tot);
    ue->measurements.n0_power_avg = (int)
        (((k1*((long long int) (ue->measurements.n0_power_avg))) +
          (k2*((long long int) (ue->measurements.n0_power_tot))))>>10);
  } else {
    for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++)
      ue->measurements.rx_power_avg[eNB_id] = ue->measurements.rx_power_tot[eNB_id];

    ue->measurements.n0_power_avg = ue->measurements.n0_power_tot;
    ue->init_averaging = 0;
  }

  for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++) {
    ue->measurements.rx_power_avg_dB[eNB_id] = dB_fixed( ue->measurements.rx_power_avg[eNB_id]);
    ue->measurements.wideband_cqi_tot[eNB_id] = dB_fixed2(ue->measurements.rx_power_tot[eNB_id],ue->measurements.n0_power_tot);
    ue->measurements.wideband_cqi_avg[eNB_id] = dB_fixed2(ue->measurements.rx_power_avg[eNB_id],ue->measurements.n0_power_avg);
    ue->measurements.rx_rssi_dBm[eNB_id] = ue->measurements.rx_power_avg_dB[eNB_id] - ue->rx_total_gain_dB;
//#ifdef DEBUG_MEAS_UE
      LOG_D(PHY,"[eNB %d] Subframe %d, RSSI %d dBm, RSSI (digital) %d dB, WBandCQI %d dB, rxPwrAvg %d, n0PwrAvg %d\n",
            eNB_id,
            subframe,
            ue->measurements.rx_rssi_dBm[eNB_id],
            ue->measurements.rx_power_avg_dB[eNB_id],
            ue->measurements.wideband_cqi_avg[eNB_id],
            ue->measurements.rx_power_avg[eNB_id],
            ue->measurements.n0_power_tot);
//#endif
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}