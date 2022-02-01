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

/*! \file PHY/LTE_TRANSPORT/print_stats.c
* \brief PHY statstic logging function
* \author R. Knopp, F. Kaltenberger, navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
* \note
* \warning
*/

#include "PHY/LTE_TRANSPORT/proto.h"
#include "targets/RT/USER/lte-softmodem.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/extern.h"

  #include "openair2/LAYER2/MAC/proto.h"
  #include "openair2/RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

extern int mac_get_rrc_status(uint8_t Mod_id,uint8_t eNB_flag,uint8_t index);

#include "common_lib.h"
extern openair0_config_t openair0_cfg[];


int dump_ue_stats(PHY_VARS_UE *ue, UE_rxtx_proc_t *proc,char *buffer, int length, runmode_t mode, int input_level_dBm) {
  uint8_t eNB=0;
  uint32_t RRC_status;
  int len=length;
  int harq_pid,round;

  if (ue==NULL)
    return 0;

  if ((mode == normal_txrx) || (mode == no_L2_connect)) {
    len += sprintf(&buffer[len], "[UE_PROC] UE %d, RNTI %x\n",ue->Mod_id, ue->pdcch_vars[0][0]->crnti);
    len += sprintf(&buffer[len],"[UE PROC] RSRP[0] %.2f dBm/RE, RSSI %.2f dBm, RSRQ[0] %.2f dB, N0 %d dBm/RE (NF %.1f dB)\n",
                   10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB,
                   10*log10(ue->measurements.rssi)-ue->rx_total_gain_dB,
                   10*log10(ue->measurements.rsrq[0]),
                   ue->measurements.n0_power_tot_dBm,
                   (double)ue->measurements.n0_power_tot_dBm+132.24);
    /*
    len += sprintf(&buffer[len],
                   "[UE PROC] Frame count: %d\neNB0 RSSI %d dBm/RE (%d dB, %d dB)\neNB1 RSSI %d dBm/RE (%d dB, %d dB)neNB2 RSSI %d dBm/RE (%d dB, %d dB)\nN0 %d dBm/RE, %f dBm/%dPRB (%d dB, %d dB)\n",
                   proc->frame_rx,
                   ue->measurements.rx_rssi_dBm[0],
                   ue->measurements.rx_power_dB[0][0],
                   ue->measurements.rx_power_dB[0][1],
                   ue->measurements.rx_rssi_dBm[1],
                   ue->measurements.rx_power_dB[1][0],
                   ue->measurements.rx_power_dB[1][1],
                   ue->measurements.rx_rssi_dBm[2],
                   ue->measurements.rx_power_dB[2][0],
                   ue->measurements.rx_power_dB[2][1],
                   ue->measurements.n0_power_tot_dBm,
                   ue->measurements.n0_power_tot_dBm+10*log10(12*ue->frame_parms.N_RB_DL),
                   ue->frame_parms.N_RB_DL,
                   ue->measurements.n0_power_dB[0],
                   ue->measurements.n0_power_dB[1]);
    */
#ifdef EXMIMO
    len += sprintf(&buffer[len], "[UE PROC] RX Gain %d dB (LNA %d, vga %d dB)\n",ue->rx_total_gain_dB, openair0_cfg[0].rxg_mode[0],(int)openair0_cfg[0].rx_gain[0]);
#endif
    len += sprintf(&buffer[len], "[UE PROC] RX Gain %d dB\n",ue->rx_total_gain_dB);
    len += sprintf(&buffer[len], "[UE_PROC] Frequency offset %d Hz, estimated carrier frequency %f Hz\n",ue->common_vars.freq_offset,openair0_cfg[0].rx_freq[0]-ue->common_vars.freq_offset);
    len += sprintf(&buffer[len], "[UE PROC] UE mode = %s (%d)\n",mode_string[ue->UE_mode[0]],ue->UE_mode[0]);
    len += sprintf(&buffer[len], "[UE PROC] timing_advance = %d\n",ue->timing_advance);

    if (ue->UE_mode[0]==PUSCH) {
      len += sprintf(&buffer[len], "[UE PROC] Po_PUSCH = %d dBm (PL %d dB, Po_NOMINAL_PUSCH %d dBm, PHR %d dB)\n",
                     ue->ulsch[0]->Po_PUSCH,
                     get_PL(ue->Mod_id,ue->CC_id,0),
                     ue->frame_parms.ul_power_control_config_common.p0_NominalPUSCH,
                     ue->ulsch[0]->PHR);
      len += sprintf(&buffer[len], "[UE PROC] Po_PUCCH = %d dBm (Po_NOMINAL_PUCCH %d dBm, g_pucch %d dB)\n",
                     get_PL(ue->Mod_id,ue->CC_id,0)+
                     ue->frame_parms.ul_power_control_config_common.p0_NominalPUCCH+
                     ue->dlsch[0][0][0]->g_pucch,
                     ue->frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
                     ue->dlsch[0][0][0]->g_pucch);
    }

    //for (eNB=0;eNB<NUMBER_OF_eNB_MAX;eNB++) {
    for (eNB=0; eNB<1; eNB++) {
      len += sprintf(&buffer[len], "[UE PROC] RX spatial power eNB%d: [%d %d; %d %d] dB\n",
                     eNB,
                     ue->measurements.rx_spatial_power_dB[eNB][0][0],
                     ue->measurements.rx_spatial_power_dB[eNB][0][1],
                     ue->measurements.rx_spatial_power_dB[eNB][1][0],
                     ue->measurements.rx_spatial_power_dB[eNB][1][1]);
      len += sprintf(&buffer[len], "[UE PROC] RX total power eNB%d: %d dB, avg: %d dB\n",eNB,ue->measurements.rx_power_tot_dB[eNB],ue->measurements.rx_power_avg_dB[eNB]);
      len += sprintf(&buffer[len], "[UE PROC] RX total power lin: %d, avg: %d, RX total noise lin: %d, avg: %d\n",ue->measurements.rx_power_tot[eNB],
                     ue->measurements.rx_power_avg[eNB], ue->measurements.n0_power_tot, ue->measurements.n0_power_avg);
      len += sprintf(&buffer[len], "[UE PROC] effective SINR %.2f dB\n",ue->sinr_eff);
      len += sprintf(&buffer[len], "[UE PROC] Wideband CQI eNB %d: %d dB, avg: %d dB\n",eNB,ue->measurements.wideband_cqi_tot[eNB],ue->measurements.wideband_cqi_avg[eNB]);

      switch (ue->frame_parms.N_RB_DL) {
        case 6:
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 0): [%d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][0][0],
                         ue->measurements.subband_cqi_dB[eNB][0][1],
                         ue->measurements.subband_cqi_dB[eNB][0][2],
                         ue->measurements.subband_cqi_dB[eNB][0][3],
                         ue->measurements.subband_cqi_dB[eNB][0][4],
                         ue->measurements.subband_cqi_dB[eNB][0][5]);
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 1): [%d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][1][0],
                         ue->measurements.subband_cqi_dB[eNB][1][1],
                         ue->measurements.subband_cqi_dB[eNB][1][2],
                         ue->measurements.subband_cqi_dB[eNB][1][3],
                         ue->measurements.subband_cqi_dB[eNB][1][4],
                         ue->measurements.subband_cqi_dB[eNB][1][5]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 0): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][0],
                         ue->measurements.subband_pmi_im[eNB][0][0],
                         ue->measurements.subband_pmi_re[eNB][1][0],
                         ue->measurements.subband_pmi_im[eNB][1][0],
                         ue->measurements.subband_pmi_re[eNB][2][0],
                         ue->measurements.subband_pmi_im[eNB][2][0],
                         ue->measurements.subband_pmi_re[eNB][3][0],
                         ue->measurements.subband_pmi_im[eNB][3][0],
                         ue->measurements.subband_pmi_re[eNB][4][0],
                         ue->measurements.subband_pmi_im[eNB][4][0],
                         ue->measurements.subband_pmi_re[eNB][5][0],
                         ue->measurements.subband_pmi_im[eNB][5][0]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 1): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][1],
                         ue->measurements.subband_pmi_im[eNB][0][1],
                         ue->measurements.subband_pmi_re[eNB][1][1],
                         ue->measurements.subband_pmi_im[eNB][1][1],
                         ue->measurements.subband_pmi_re[eNB][2][1],
                         ue->measurements.subband_pmi_im[eNB][2][1],
                         ue->measurements.subband_pmi_re[eNB][3][1],
                         ue->measurements.subband_pmi_im[eNB][3][1],
                         ue->measurements.subband_pmi_re[eNB][4][1],
                         ue->measurements.subband_pmi_im[eNB][4][1],
                         ue->measurements.subband_pmi_re[eNB][5][1],
                         ue->measurements.subband_pmi_im[eNB][5][1]);
          len += sprintf(&buffer[len], "[UE PROC] PMI Antenna selection eNB%d : [%d %d %d %d %d %d]\n",
                         eNB,
                         ue->measurements.selected_rx_antennas[eNB][0],
                         ue->measurements.selected_rx_antennas[eNB][1],
                         ue->measurements.selected_rx_antennas[eNB][2],
                         ue->measurements.selected_rx_antennas[eNB][3],
                         ue->measurements.selected_rx_antennas[eNB][4],
                         ue->measurements.selected_rx_antennas[eNB][5]);
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (max): %jx\n",eNB,pmi2hex_2Ar1(quantize_subband_pmi(&ue->measurements,eNB,6)));
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (both): %jx,%jx\n",eNB,
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,0,6)),
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,1,6)));
          break;

        case 25:
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 0): [%d %d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][0][0],
                         ue->measurements.subband_cqi_dB[eNB][0][1],
                         ue->measurements.subband_cqi_dB[eNB][0][2],
                         ue->measurements.subband_cqi_dB[eNB][0][3],
                         ue->measurements.subband_cqi_dB[eNB][0][4],
                         ue->measurements.subband_cqi_dB[eNB][0][5],
                         ue->measurements.subband_cqi_dB[eNB][0][6]);
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 1): [%d %d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][1][0],
                         ue->measurements.subband_cqi_dB[eNB][1][1],
                         ue->measurements.subband_cqi_dB[eNB][1][2],
                         ue->measurements.subband_cqi_dB[eNB][1][3],
                         ue->measurements.subband_cqi_dB[eNB][1][4],
                         ue->measurements.subband_cqi_dB[eNB][1][5],
                         ue->measurements.subband_cqi_dB[eNB][1][6]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 0): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][0],
                         ue->measurements.subband_pmi_im[eNB][0][0],
                         ue->measurements.subband_pmi_re[eNB][1][0],
                         ue->measurements.subband_pmi_im[eNB][1][0],
                         ue->measurements.subband_pmi_re[eNB][2][0],
                         ue->measurements.subband_pmi_im[eNB][2][0],
                         ue->measurements.subband_pmi_re[eNB][3][0],
                         ue->measurements.subband_pmi_im[eNB][3][0],
                         ue->measurements.subband_pmi_re[eNB][4][0],
                         ue->measurements.subband_pmi_im[eNB][4][0],
                         ue->measurements.subband_pmi_re[eNB][5][0],
                         ue->measurements.subband_pmi_im[eNB][5][0],
                         ue->measurements.subband_pmi_re[eNB][6][0],
                         ue->measurements.subband_pmi_im[eNB][6][0]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 1): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][1],
                         ue->measurements.subband_pmi_im[eNB][0][1],
                         ue->measurements.subband_pmi_re[eNB][1][1],
                         ue->measurements.subband_pmi_im[eNB][1][1],
                         ue->measurements.subband_pmi_re[eNB][2][1],
                         ue->measurements.subband_pmi_im[eNB][2][1],
                         ue->measurements.subband_pmi_re[eNB][3][1],
                         ue->measurements.subband_pmi_im[eNB][3][1],
                         ue->measurements.subband_pmi_re[eNB][4][1],
                         ue->measurements.subband_pmi_im[eNB][4][1],
                         ue->measurements.subband_pmi_re[eNB][5][1],
                         ue->measurements.subband_pmi_im[eNB][5][1],
                         ue->measurements.subband_pmi_re[eNB][6][1],
                         ue->measurements.subband_pmi_im[eNB][6][1]);
          len += sprintf(&buffer[len], "[UE PROC] PMI Antenna selection eNB%d : [%d %d %d %d %d %d %d]\n",
                         eNB,
                         ue->measurements.selected_rx_antennas[eNB][0],
                         ue->measurements.selected_rx_antennas[eNB][1],
                         ue->measurements.selected_rx_antennas[eNB][2],
                         ue->measurements.selected_rx_antennas[eNB][3],
                         ue->measurements.selected_rx_antennas[eNB][4],
                         ue->measurements.selected_rx_antennas[eNB][5],
                         ue->measurements.selected_rx_antennas[eNB][6]);
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (max): %jx\n",eNB,pmi2hex_2Ar1(quantize_subband_pmi(&ue->measurements,eNB,7)));
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (both): %jx,%jx\n",eNB,
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,0,7)),
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,1,7)));
          break;

        case 50:
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 0): [%d %d %d %d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][0][0],
                         ue->measurements.subband_cqi_dB[eNB][0][1],
                         ue->measurements.subband_cqi_dB[eNB][0][2],
                         ue->measurements.subband_cqi_dB[eNB][0][3],
                         ue->measurements.subband_cqi_dB[eNB][0][4],
                         ue->measurements.subband_cqi_dB[eNB][0][5],
                         ue->measurements.subband_cqi_dB[eNB][0][6],
                         ue->measurements.subband_cqi_dB[eNB][0][7],
                         ue->measurements.subband_cqi_dB[eNB][0][8]);
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 1): [%d %d %d %d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][1][0],
                         ue->measurements.subband_cqi_dB[eNB][1][1],
                         ue->measurements.subband_cqi_dB[eNB][1][2],
                         ue->measurements.subband_cqi_dB[eNB][1][3],
                         ue->measurements.subband_cqi_dB[eNB][1][4],
                         ue->measurements.subband_cqi_dB[eNB][1][5],
                         ue->measurements.subband_cqi_dB[eNB][1][6],
                         ue->measurements.subband_cqi_dB[eNB][1][7],
                         ue->measurements.subband_cqi_dB[eNB][1][8]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 0): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][0],
                         ue->measurements.subband_pmi_im[eNB][0][0],
                         ue->measurements.subband_pmi_re[eNB][1][0],
                         ue->measurements.subband_pmi_im[eNB][1][0],
                         ue->measurements.subband_pmi_re[eNB][2][0],
                         ue->measurements.subband_pmi_im[eNB][2][0],
                         ue->measurements.subband_pmi_re[eNB][3][0],
                         ue->measurements.subband_pmi_im[eNB][3][0],
                         ue->measurements.subband_pmi_re[eNB][4][0],
                         ue->measurements.subband_pmi_im[eNB][4][0],
                         ue->measurements.subband_pmi_re[eNB][5][0],
                         ue->measurements.subband_pmi_im[eNB][5][0],
                         ue->measurements.subband_pmi_re[eNB][6][0],
                         ue->measurements.subband_pmi_im[eNB][6][0],
                         ue->measurements.subband_pmi_re[eNB][7][0],
                         ue->measurements.subband_pmi_im[eNB][7][0],
                         ue->measurements.subband_pmi_re[eNB][8][0],
                         ue->measurements.subband_pmi_im[eNB][8][0]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 1): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][1],
                         ue->measurements.subband_pmi_im[eNB][0][1],
                         ue->measurements.subband_pmi_re[eNB][1][1],
                         ue->measurements.subband_pmi_im[eNB][1][1],
                         ue->measurements.subband_pmi_re[eNB][2][1],
                         ue->measurements.subband_pmi_im[eNB][2][1],
                         ue->measurements.subband_pmi_re[eNB][3][1],
                         ue->measurements.subband_pmi_im[eNB][3][1],
                         ue->measurements.subband_pmi_re[eNB][4][1],
                         ue->measurements.subband_pmi_im[eNB][4][1],
                         ue->measurements.subband_pmi_re[eNB][5][1],
                         ue->measurements.subband_pmi_im[eNB][5][1],
                         ue->measurements.subband_pmi_re[eNB][6][1],
                         ue->measurements.subband_pmi_im[eNB][6][1],
                         ue->measurements.subband_pmi_re[eNB][7][1],
                         ue->measurements.subband_pmi_im[eNB][7][1],
                         ue->measurements.subband_pmi_re[eNB][8][1],
                         ue->measurements.subband_pmi_im[eNB][8][1]);
          len += sprintf(&buffer[len], "[UE PROC] PMI Antenna selection eNB%d : [%d %d %d %d %d %d %d %d %d]\n",
                         eNB,
                         ue->measurements.selected_rx_antennas[eNB][0],
                         ue->measurements.selected_rx_antennas[eNB][1],
                         ue->measurements.selected_rx_antennas[eNB][2],
                         ue->measurements.selected_rx_antennas[eNB][3],
                         ue->measurements.selected_rx_antennas[eNB][4],
                         ue->measurements.selected_rx_antennas[eNB][5],
                         ue->measurements.selected_rx_antennas[eNB][6],
                         ue->measurements.selected_rx_antennas[eNB][7],
                         ue->measurements.selected_rx_antennas[eNB][8]);
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (max): %jx\n",eNB,pmi2hex_2Ar1(quantize_subband_pmi(&ue->measurements,eNB,9)));
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (both): %jx,%jx\n",eNB,
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,0,9)),
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,1,9)));
          break;

        case 100:
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 0): [%d %d %d %d %d %d %d %d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][0][0],
                         ue->measurements.subband_cqi_dB[eNB][0][1],
                         ue->measurements.subband_cqi_dB[eNB][0][2],
                         ue->measurements.subband_cqi_dB[eNB][0][3],
                         ue->measurements.subband_cqi_dB[eNB][0][4],
                         ue->measurements.subband_cqi_dB[eNB][0][5],
                         ue->measurements.subband_cqi_dB[eNB][0][6],
                         ue->measurements.subband_cqi_dB[eNB][0][7],
                         ue->measurements.subband_cqi_dB[eNB][0][8],
                         ue->measurements.subband_cqi_dB[eNB][0][9],
                         ue->measurements.subband_cqi_dB[eNB][0][10],
                         ue->measurements.subband_cqi_dB[eNB][0][11],
                         ue->measurements.subband_cqi_dB[eNB][0][12]);
          len += sprintf(&buffer[len], "[UE PROC] Subband CQI eNB%d (Ant 1): [%d %d %d %d %d %d %d %d %d %d %d %d %d] dB\n",
                         eNB,
                         ue->measurements.subband_cqi_dB[eNB][1][0],
                         ue->measurements.subband_cqi_dB[eNB][1][1],
                         ue->measurements.subband_cqi_dB[eNB][1][2],
                         ue->measurements.subband_cqi_dB[eNB][1][3],
                         ue->measurements.subband_cqi_dB[eNB][1][4],
                         ue->measurements.subband_cqi_dB[eNB][1][5],
                         ue->measurements.subband_cqi_dB[eNB][1][6],
                         ue->measurements.subband_cqi_dB[eNB][1][7],
                         ue->measurements.subband_cqi_dB[eNB][1][8],
                         ue->measurements.subband_cqi_dB[eNB][1][9],
                         ue->measurements.subband_cqi_dB[eNB][1][10],
                         ue->measurements.subband_cqi_dB[eNB][1][11],
                         ue->measurements.subband_cqi_dB[eNB][1][12]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 0): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][0],
                         ue->measurements.subband_pmi_im[eNB][0][0],
                         ue->measurements.subband_pmi_re[eNB][1][0],
                         ue->measurements.subband_pmi_im[eNB][1][0],
                         ue->measurements.subband_pmi_re[eNB][2][0],
                         ue->measurements.subband_pmi_im[eNB][2][0],
                         ue->measurements.subband_pmi_re[eNB][3][0],
                         ue->measurements.subband_pmi_im[eNB][3][0],
                         ue->measurements.subband_pmi_re[eNB][4][0],
                         ue->measurements.subband_pmi_im[eNB][4][0],
                         ue->measurements.subband_pmi_re[eNB][5][0],
                         ue->measurements.subband_pmi_im[eNB][5][0],
                         ue->measurements.subband_pmi_re[eNB][6][0],
                         ue->measurements.subband_pmi_im[eNB][6][0],
                         ue->measurements.subband_pmi_re[eNB][7][0],
                         ue->measurements.subband_pmi_im[eNB][7][0],
                         ue->measurements.subband_pmi_re[eNB][8][0],
                         ue->measurements.subband_pmi_im[eNB][8][0],
                         ue->measurements.subband_pmi_re[eNB][9][0],
                         ue->measurements.subband_pmi_im[eNB][9][0],
                         ue->measurements.subband_pmi_re[eNB][10][0],
                         ue->measurements.subband_pmi_im[eNB][10][0],
                         ue->measurements.subband_pmi_re[eNB][11][0],
                         ue->measurements.subband_pmi_im[eNB][11][0],
                         ue->measurements.subband_pmi_re[eNB][12][0],
                         ue->measurements.subband_pmi_im[eNB][12][0]);
          len += sprintf(&buffer[len], "[UE PROC] Subband PMI eNB%d (Ant 1): [(%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) (%d %d)]\n",
                         eNB,
                         ue->measurements.subband_pmi_re[eNB][0][1],
                         ue->measurements.subband_pmi_im[eNB][0][1],
                         ue->measurements.subband_pmi_re[eNB][1][1],
                         ue->measurements.subband_pmi_im[eNB][1][1],
                         ue->measurements.subband_pmi_re[eNB][2][1],
                         ue->measurements.subband_pmi_im[eNB][2][1],
                         ue->measurements.subband_pmi_re[eNB][3][1],
                         ue->measurements.subband_pmi_im[eNB][3][1],
                         ue->measurements.subband_pmi_re[eNB][4][1],
                         ue->measurements.subband_pmi_im[eNB][4][1],
                         ue->measurements.subband_pmi_re[eNB][5][1],
                         ue->measurements.subband_pmi_im[eNB][5][1],
                         ue->measurements.subband_pmi_re[eNB][6][1],
                         ue->measurements.subband_pmi_im[eNB][6][1],
                         ue->measurements.subband_pmi_re[eNB][7][1],
                         ue->measurements.subband_pmi_im[eNB][7][1],
                         ue->measurements.subband_pmi_re[eNB][8][1],
                         ue->measurements.subband_pmi_im[eNB][8][1],
                         ue->measurements.subband_pmi_re[eNB][9][1],
                         ue->measurements.subband_pmi_im[eNB][9][1],
                         ue->measurements.subband_pmi_re[eNB][10][1],
                         ue->measurements.subband_pmi_im[eNB][10][1],
                         ue->measurements.subband_pmi_re[eNB][11][1],
                         ue->measurements.subband_pmi_im[eNB][11][1],
                         ue->measurements.subband_pmi_re[eNB][12][1],
                         ue->measurements.subband_pmi_im[eNB][12][1]);
          len += sprintf(&buffer[len], "[UE PROC] PMI Antenna selection eNB%d : [%d %d %d %d %d %d %d %d %d %d %d %d %d]\n",
                         eNB,
                         ue->measurements.selected_rx_antennas[eNB][0],
                         ue->measurements.selected_rx_antennas[eNB][1],
                         ue->measurements.selected_rx_antennas[eNB][2],
                         ue->measurements.selected_rx_antennas[eNB][3],
                         ue->measurements.selected_rx_antennas[eNB][4],
                         ue->measurements.selected_rx_antennas[eNB][5],
                         ue->measurements.selected_rx_antennas[eNB][6],
                         ue->measurements.selected_rx_antennas[eNB][7],
                         ue->measurements.selected_rx_antennas[eNB][8],
                         ue->measurements.selected_rx_antennas[eNB][9],
                         ue->measurements.selected_rx_antennas[eNB][10],
                         ue->measurements.selected_rx_antennas[eNB][11],
                         ue->measurements.selected_rx_antennas[eNB][12]);
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (max): %jx\n",eNB,pmi2hex_2Ar1(quantize_subband_pmi(&ue->measurements,eNB,13)));
          len += sprintf(&buffer[len], "[UE PROC] Quantized PMI eNB %d (both): %jx,%jx\n",eNB,
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,0,13)),
                         pmi2hex_2Ar1(quantize_subband_pmi2(&ue->measurements,eNB,1,13)));
          break;
      }

      RRC_status = mac_UE_get_rrc_status(ue->Mod_id, 0);
      len += sprintf(&buffer[len],"[UE PROC] RRC status = %d\n",RRC_status);
      len += sprintf(&buffer[len], "[UE PROC] Transmission Mode %d \n",ue->transmission_mode[eNB]);
      len += sprintf(&buffer[len], "[UE PROC] PBCH err conseq %d, PBCH error total %d, PBCH FER %d\n",
                     ue->pbch_vars[eNB]->pdu_errors_conseq,
                     ue->pbch_vars[eNB]->pdu_errors,
                     ue->pbch_vars[eNB]->pdu_fer);

      if (ue->transmission_mode[eNB] == 6)
        len += sprintf(&buffer[len], "[UE PROC] Mode 6 Wideband CQI eNB %d : %d dB\n",eNB,ue->measurements.precoded_cqi_dB[eNB][0]);

      for (harq_pid=0; harq_pid<8; harq_pid++) {
        len+=sprintf(&buffer[len],"[UE PROC] eNB %d: CW 0 harq_pid %d, mcs %d:",eNB,harq_pid,ue->dlsch[0][0][0]->harq_processes[harq_pid]->mcs);

        for (round=0; round<8; round++)
          len+=sprintf(&buffer[len],"%d/%d ",
                       ue->dlsch[0][0][0]->harq_processes[harq_pid]->errors[round],
                       ue->dlsch[0][0][0]->harq_processes[harq_pid]->trials[round]);

        len+=sprintf(&buffer[len],"\n");
      }

      if (ue->dlsch[0][0] && ue->dlsch[0][0][0] && ue->dlsch[0][0][1]) {
        len += sprintf(&buffer[len], "[UE PROC] Saved PMI for DLSCH eNB %d : %jx (%p)\n",eNB,pmi2hex_2Ar1(ue->dlsch[0][0][0]->pmi_alloc),ue->dlsch[0][0][0]);
        len += sprintf(&buffer[len], "[UE PROC] eNB %d: dl_power_off = %d\n",eNB,ue->dlsch[0][0][0]->harq_processes[0]->dl_power_off);

        for (harq_pid=0; harq_pid<8; harq_pid++) {
          len+=sprintf(&buffer[len],"[UE PROC] eNB %d: CW 1 harq_pid %d, mcs %d:",eNB,harq_pid,ue->dlsch[0][0][1]->harq_processes[0]->mcs);

          for (round=0; round<8; round++)
            len+=sprintf(&buffer[len],"%d/%d ",
                         ue->dlsch[0][0][1]->harq_processes[harq_pid]->errors[round],
                         ue->dlsch[0][0][1]->harq_processes[harq_pid]->trials[round]);

          len+=sprintf(&buffer[len],"\n");
        }
      }

      len += sprintf(&buffer[len], "[UE PROC] DLSCH Total %d, Error %d, FER %d\n",ue->dlsch_received[0],ue->dlsch_errors[0],ue->dlsch_fer[0]);
      len += sprintf(&buffer[len], "[UE PROC] DLSCH (SI) Total %d, Error %d\n",ue->dlsch_SI_received[0],ue->dlsch_SI_errors[0]);
      len += sprintf(&buffer[len], "[UE PROC] DLSCH (RA) Total %d, Error %d\n",ue->dlsch_ra_received[0],ue->dlsch_ra_errors[0]);
      int i=0;

      //len += sprintf(&buffer[len], "[UE PROC] MCH  Total %d\n", ue->dlsch_mch_received[0]);
      for(i=0; i <ue->frame_parms.num_MBSFN_config; i++ ) {
        len += sprintf(&buffer[len], "[UE PROC] MCH (MCCH MBSFN %d) Total %d, Error %d, Trials %d\n",
                       i, ue->dlsch_mcch_received[i][0],ue->dlsch_mcch_errors[i][0],ue->dlsch_mcch_trials[i][0]);
        len += sprintf(&buffer[len], "[UE PROC] MCH (MTCH MBSFN %d) Total %d, Error %d, Trials %d\n",
                       i, ue->dlsch_mtch_received[i][0],ue->dlsch_mtch_errors[i][0],ue->dlsch_mtch_trials[i][0]);
      }

      len += sprintf(&buffer[len], "[UE PROC] DLSCH Bitrate %dkbps\n",(ue->bitrate[0]/1000));
      len += sprintf(&buffer[len], "[UE PROC] Total Received Bits %dkbits\n",(ue->total_received_bits[0]/1000));
      len += sprintf(&buffer[len], "[UE PROC] IA receiver %d\n",ue->use_ia_receiver);
    }
  } else {
    len += sprintf(&buffer[len], "[UE PROC] Frame count: %d, RSSI %3.2f dB (%d dB, %d dB), N0 %3.2f dB (%d dB, %d dB)\n",
                   proc->frame_rx,
                   10*log10(ue->measurements.rssi),
                   ue->measurements.rx_power_dB[0][0],
                   ue->measurements.rx_power_dB[0][1],
                   10*log10(ue->measurements.n0_power_tot),
                   ue->measurements.n0_power_dB[0],
                   ue->measurements.n0_power_dB[1]);
#ifdef EXMIMO
    ue->rx_total_gain_dB = ((int)(10*log10(ue->measurements.rssi)))-input_level_dBm;
    len += sprintf(&buffer[len], "[UE PROC] rxg_mode %d, input level (set by user) %d dBm, VGA gain %d dB ==> total gain %3.2f dB, noise figure %3.2f dB\n",
                   openair0_cfg[0].rxg_mode[0],
                   input_level_dBm,
                   (int)openair0_cfg[0].rx_gain[0],
                   10*log10(ue->measurements.rssi)-input_level_dBm,
                   10*log10(ue->measurements.n0_power_tot)-ue->rx_total_gain_dB+105);
#endif
  }

  len += sprintf(&buffer[len],"EOF\n");
  return len;
} // is_clusterhead

/*
int dump_eNB_stats(PHY_VARS_eNB *eNB, char* buffer, int length)
{

  unsigned int success=0;
  uint8_t eNB_id,UE_id,i,j,number_of_cards_l=1;
  uint32_t ulsch_errors=0,dlsch_errors=0;
  uint32_t ulsch_round_attempts[4]= {0,0,0,0},ulsch_round_errors[4]= {0,0,0,0};
  uint32_t dlsch_round_attempts[4]= {0,0,0,0},dlsch_round_errors[4]= {0,0,0,0};
  uint32_t UE_id_mac, RRC_status;
  eNB_rxtx_proc_t *proc = &eNB->proc.proc_rxtx[0];
  if (eNB==NULL)
    return 0;

  int len = length;

  //  if(eNB->frame==0){
  eNB->total_dlsch_bitrate = 0;//eNB->UE_stats[UE_id].dlsch_bitrate + eNB->total_dlsch_bitrate;
  eNB->total_transmitted_bits = 0;// eNB->UE_stats[UE_id].total_transmitted_bits +  eNB->total_transmitted_bits;
  eNB->total_system_throughput = 0;//eNB->UE_stats[UE_id].total_transmitted_bits + eNB->total_system_throughput;
  // }

  for (eNB_id=0; eNB_id<number_of_cards_l; eNB_id++) {
    len += sprintf(&buffer[len],"eNB %d/%d Frame %d: RX Gain %d dB, I0 %d dBm (%d,%d) dB \n",
                   eNB_id,number_of_cards_l,
                   proc->frame_tx,
                   eNB->rx_total_gain_dB,
                   eNB->measurements.n0_power_tot_dBm,
                   eNB->measurements.n0_power_dB[0],
                   eNB->measurements.n0_power_dB[1]);

    len += sprintf(&buffer[len],"PRB I0 (%X.%X.%X.%X): ",
       eNB->rb_mask_ul[0],
       eNB->rb_mask_ul[1],eNB->rb_mask_ul[2],eNB->rb_mask_ul[3]);

    for (i=0; i<eNB->frame_parms.N_RB_UL; i++) {
      len += sprintf(&buffer[len],"%4d ",
                     eNB->measurements.n0_subband_power_tot_dBm[i]);
      if ((i>0) && ((i%25) == 0))
  len += sprintf(&buffer[len],"\n");
    }
    len += sprintf(&buffer[len],"\n");
    len += sprintf(&buffer[len],"\nPERFORMANCE PARAMETERS\n");

    for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
      if (eNB &&
    (eNB->dlsch!=NULL) &&
    (eNB->dlsch[(uint8_t)UE_id]!=NULL) &&
    (eNB->dlsch[(uint8_t)UE_id][0]->rnti>0)&&
    (eNB->UE_stats[UE_id].mode == PUSCH)) {

        eNB->total_dlsch_bitrate = eNB->UE_stats[UE_id].dlsch_bitrate + eNB->total_dlsch_bitrate;
        eNB->total_transmitted_bits = eNB->UE_stats[UE_id].total_TBS + eNB->total_transmitted_bits;

        //eNB->total_system_throughput = eNB->UE_stats[UE_id].total_transmitted_bits + eNB->total_system_throughput;

  for (i=0; i<8; i++)
    success = success + (eNB->UE_stats[UE_id].dlsch_trials[i][0] - eNB->UE_stats[UE_id].dlsch_l2_errors[i]);



  len += sprintf(&buffer[len],"Total DLSCH %d kbits / %d frames ",(eNB->total_transmitted_bits/1000),proc->frame_tx+1);
  len += sprintf(&buffer[len],"Total DLSCH throughput %d kbps ",(eNB->total_dlsch_bitrate/1000));
  len += sprintf(&buffer[len],"Total DLSCH trans %d / %d frames\n",success,proc->frame_tx+1);
  //len += sprintf(&buffer[len],"[eNB PROC] FULL MU-MIMO Transmissions/Total Transmissions = %d/%d\n",eNB->FULL_MUMIMO_transmissions,eNB->check_for_total_transmissions);
  //len += sprintf(&buffer[len],"[eNB PROC] MU-MIMO Transmissions/Total Transmissions = %d/%d\n",eNB->check_for_MUMIMO_transmissions,eNB->check_for_total_transmissions);
  //len += sprintf(&buffer[len],"[eNB PROC] SU-MIMO Transmissions/Total Transmissions = %d/%d\n",eNB->check_for_SUMIMO_transmissions,eNB->check_for_total_transmissions);

  len += sprintf(&buffer[len],"UE %d (%x) Power: (%d,%d) dB, Po_PUSCH: (%d,%d) dBm, Po_PUCCH (%d/%d) dBm, Po_PUCCH1 (%d,%d) dBm,  PUCCH1 Thres %d dBm \n",
           UE_id,
           eNB->UE_stats[UE_id].crnti,
           dB_fixed(eNB->pusch_vars[UE_id]->ulsch_power[0]),
           dB_fixed(eNB->pusch_vars[UE_id]->ulsch_power[1]),
           eNB->UE_stats[UE_id].UL_rssi[0],
           eNB->UE_stats[UE_id].UL_rssi[1],
           dB_fixed(eNB->UE_stats[UE_id].Po_PUCCH/eNB->frame_parms.N_RB_UL)-eNB->rx_total_gain_dB,
           eNB->frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
           dB_fixed(eNB->UE_stats[UE_id].Po_PUCCH1_below/eNB->frame_parms.N_RB_UL)-eNB->rx_total_gain_dB,
           dB_fixed(eNB->UE_stats[UE_id].Po_PUCCH1_above/eNB->frame_parms.N_RB_UL)-eNB->rx_total_gain_dB,
           PUCCH1_THRES+eNB->measurements.n0_power_tot_dBm-dB_fixed(eNB->frame_parms.N_RB_UL));

  len+= sprintf(&buffer[len],"DL mcs %d, UL mcs %d, UL rb %d, delta_TF %d, ",
          eNB->dlsch[(uint8_t)UE_id][0]->harq_processes[0]->mcs,
          eNB->ulsch[(uint8_t)UE_id]->harq_processes[0]->mcs,
          eNB->ulsch[(uint8_t)UE_id]->harq_processes[0]->nb_rb,
          eNB->ulsch[(uint8_t)UE_id]->harq_processes[0]->delta_TF);

  len += sprintf(&buffer[len],"Wideband CQI: (%d,%d) dB\n",
           eNB->measurements.wideband_cqi_dB[UE_id][0],
           eNB->measurements.wideband_cqi_dB[UE_id][1]);

  len += sprintf(&buffer[len],"DL TM %d, DL_cqi %d, DL_pmi_single %jx ",
           eNB->transmission_mode[UE_id],
           eNB->UE_stats[UE_id].DL_cqi[0],
           pmi2hex_2Ar1(eNB->UE_stats[UE_id].DL_pmi_single));

  len += sprintf(&buffer[len],"Timing advance %d samples (%d 16Ts), update %d ",
           eNB->UE_stats[UE_id].UE_timing_offset,
           eNB->UE_stats[UE_id].UE_timing_offset>>2,
           eNB->UE_stats[UE_id].timing_advance_update);

  len += sprintf(&buffer[len],"Mode = %s(%d) ",
           mode_string[eNB->UE_stats[UE_id].mode],
           eNB->UE_stats[UE_id].mode);
  UE_id_mac = find_UE_id(eNB->Mod_id,eNB->dlsch[(uint8_t)UE_id][0]->rnti);

  if (UE_id_mac != -1) {
    RRC_status = mac_eNB_get_rrc_status(eNB->Mod_id,eNB->dlsch[(uint8_t)UE_id][0]->rnti);
    len += sprintf(&buffer[len],"UE_id_mac = %d, RRC status = %d\n",UE_id_mac,RRC_status);
  } else
    len += sprintf(&buffer[len],"UE_id_mac = -1\n");

        len += sprintf(&buffer[len],"SR received/total: %d/%d (diff %d)\n",
                       eNB->UE_stats[UE_id].sr_received,
                       eNB->UE_stats[UE_id].sr_total,
                       eNB->UE_stats[UE_id].sr_total-eNB->UE_stats[UE_id].sr_received);

  len += sprintf(&buffer[len],"DL Subband CQI: ");

  int nb_sb;
  switch (eNB->frame_parms.N_RB_DL) {
  case 6:
    nb_sb=0;
    break;
  case 15:
    nb_sb = 4;
  case 25:
    nb_sb = 7;
    break;
  case 50:
    nb_sb = 9;
    break;
  case 75:
    nb_sb = 10;
    break;
  case 100:
    nb_sb = 13;
    break;
  default:
    nb_sb=0;
    break;
  }
  for (i=0; i<nb_sb; i++)
    len += sprintf(&buffer[len],"%2d ",
       eNB->UE_stats[UE_id].DL_subband_cqi[0][i]);
  len += sprintf(&buffer[len],"\n");



        ulsch_errors = 0;

        for (j=0; j<4; j++) {
          ulsch_round_attempts[j]=0;
          ulsch_round_errors[j]=0;
        }

        len += sprintf(&buffer[len],"ULSCH errors/attempts per harq (per round): \n");

        for (i=0; i<8; i++) {
          len += sprintf(&buffer[len],"   harq %d: %d/%d (fer %d) (%d/%d, %d/%d, %d/%d, %d/%d) ",
                         i,
                         eNB->UE_stats[UE_id].ulsch_errors[i],
                         eNB->UE_stats[UE_id].ulsch_decoding_attempts[i][0],
                         eNB->UE_stats[UE_id].ulsch_round_fer[i][0],
                         eNB->UE_stats[UE_id].ulsch_round_errors[i][0],
                         eNB->UE_stats[UE_id].ulsch_decoding_attempts[i][0],
                         eNB->UE_stats[UE_id].ulsch_round_errors[i][1],
                         eNB->UE_stats[UE_id].ulsch_decoding_attempts[i][1],
                         eNB->UE_stats[UE_id].ulsch_round_errors[i][2],
                         eNB->UE_stats[UE_id].ulsch_decoding_attempts[i][2],
                         eNB->UE_stats[UE_id].ulsch_round_errors[i][3],
                         eNB->UE_stats[UE_id].ulsch_decoding_attempts[i][3]);
    if ((i&1) == 1)
      len += sprintf(&buffer[len],"\n");

          ulsch_errors+=eNB->UE_stats[UE_id].ulsch_errors[i];

          for (j=0; j<4; j++) {
            ulsch_round_attempts[j]+=eNB->UE_stats[UE_id].ulsch_decoding_attempts[i][j];
            ulsch_round_errors[j]+=eNB->UE_stats[UE_id].ulsch_round_errors[i][j];
          }
        }

        len += sprintf(&buffer[len],"ULSCH errors/attempts total %d/%d (%d/%d, %d/%d, %d/%d, %d/%d)\n",
                       ulsch_errors,ulsch_round_attempts[0],

                       ulsch_round_errors[0],ulsch_round_attempts[0],
                       ulsch_round_errors[1],ulsch_round_attempts[1],
                       ulsch_round_errors[2],ulsch_round_attempts[2],
                       ulsch_round_errors[3],ulsch_round_attempts[3]);

        dlsch_errors = 0;

        for (j=0; j<4; j++) {
          dlsch_round_attempts[j]=0;
          dlsch_round_errors[j]=0;
        }

        len += sprintf(&buffer[len],"DLSCH errors/attempts per harq (per round): \n");

        for (i=0; i<8; i++) {
          len += sprintf(&buffer[len],"   harq %d: %d/%d (%d/%d/%d, %d/%d/%d, %d/%d/%d, %d/%d/%d) ",
                         i,
                         eNB->UE_stats[UE_id].dlsch_l2_errors[i],
                         eNB->UE_stats[UE_id].dlsch_trials[i][0],
                         eNB->UE_stats[UE_id].dlsch_ACK[i][0],
                         eNB->UE_stats[UE_id].dlsch_NAK[i][0],
                         eNB->UE_stats[UE_id].dlsch_trials[i][0],
                         eNB->UE_stats[UE_id].dlsch_ACK[i][1],
                         eNB->UE_stats[UE_id].dlsch_NAK[i][1],
                         eNB->UE_stats[UE_id].dlsch_trials[i][1],
                         eNB->UE_stats[UE_id].dlsch_ACK[i][2],
                         eNB->UE_stats[UE_id].dlsch_NAK[i][2],
                         eNB->UE_stats[UE_id].dlsch_trials[i][2],
                         eNB->UE_stats[UE_id].dlsch_ACK[i][3],
                         eNB->UE_stats[UE_id].dlsch_NAK[i][3],
                         eNB->UE_stats[UE_id].dlsch_trials[i][3]);
    if ((i&1) == 1)
      len += sprintf(&buffer[len],"\n");


          dlsch_errors+=eNB->UE_stats[UE_id].dlsch_l2_errors[i];

          for (j=0; j<4; j++) {
            dlsch_round_attempts[j]+=eNB->UE_stats[UE_id].dlsch_trials[i][j];
            dlsch_round_errors[j]+=eNB->UE_stats[UE_id].dlsch_NAK[i][j];
          }
        }

        len += sprintf(&buffer[len],"DLSCH errors/attempts total %d/%d (%d/%d, %d/%d, %d/%d, %d/%d): \n",
                       dlsch_errors,dlsch_round_attempts[0],
                       dlsch_round_errors[0],dlsch_round_attempts[0],
                       dlsch_round_errors[1],dlsch_round_attempts[1],
                       dlsch_round_errors[2],dlsch_round_attempts[2],
                       dlsch_round_errors[3],dlsch_round_attempts[3]);


        len += sprintf(&buffer[len],"DLSCH total bits from MAC: %dkbit ",(eNB->UE_stats[UE_id].total_TBS_MAC)/1000);
        len += sprintf(&buffer[len],"DLSCH total bits ack'ed: %dkbit ",(eNB->UE_stats[UE_id].total_TBS)/1000);
        len += sprintf(&buffer[len],"DLSCH Average throughput (100 frames): %dkbps\n",(eNB->UE_stats[UE_id].dlsch_bitrate/1000));
  //        len += sprintf(&buffer[len],"[eNB PROC] Transmission Mode %d\n",eNB->transmission_mode[UE_id]);
      }
    }

    len += sprintf(&buffer[len],"\n");
  }

  len += sprintf(&buffer[len],"EOF\n");

  return len;
}
*/
