/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file PHY/LTE_TRANSPORT/initial_sync.c
* \brief Routines for initial UE synchronization procedure (PSS,SSS,PBCH and frame format detection)
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,kaltenberger@eurecom.fr
* \note
* \warning
*/
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "defs.h"
#include "extern.h"
#ifdef EXMIMO
#include "gain_control.h"
#endif

#if defined(OAI_USRP) || defined(EXMIMO) || defined(OAI_LMSSDR)
#include "common_lib.h"
extern openair0_config_t openair0_cfg[];
#endif
#define DEBUG_INITIAL_SYNCH

int pbch_detection(PHY_VARS_UE *phy_vars_ue, runmode_t mode) 
{

uint8_t l,pbch_decoded,frame_mod4,pbch_tx_ant,dummy;
LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_ue->lte_frame_parms;
char phich_resource[6];

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync: starting PBCH detection (rx_offset %d)\n",phy_vars_ue->Mod_id,
        phy_vars_ue->rx_offset);
#endif

  for (l=0; l<frame_parms->symbols_per_tti/2; l++) {

    slot_fep(phy_vars_ue,
	     l,
	     0,
	     phy_vars_ue->rx_offset,
	     0,
	     1);
  }  
  for (l=0; l<frame_parms->symbols_per_tti/2; l++) {

    slot_fep(phy_vars_ue,
	     l,
	     1,
	     phy_vars_ue->rx_offset,
	     0,
	     1);
  }  
  slot_fep(phy_vars_ue,
	   0,
	   2,
	   phy_vars_ue->rx_offset,
	   0,
	   1);

  lte_ue_measurements(phy_vars_ue,
		      phy_vars_ue->rx_offset,
		      0,
		      0);
  
  
  if (phy_vars_ue->lte_frame_parms.frame_type == TDD) {
    ue_rrc_measurements(phy_vars_ue,
			1,
			0);
  }
  else {
    ue_rrc_measurements(phy_vars_ue,
			0,
			0);
  }
#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE %d] RX RSSI %d dBm, digital (%d, %d) dB, linear (%d, %d), avg rx power %d dB (%d lin), RX gain %d dB\n",
        phy_vars_ue->Mod_id,
        phy_vars_ue->PHY_measurements.rx_rssi_dBm[0] - ((phy_vars_ue->lte_frame_parms.nb_antennas_rx==2) ? 3 : 0),
        phy_vars_ue->PHY_measurements.rx_power_dB[0][0],
        phy_vars_ue->PHY_measurements.rx_power_dB[0][1],
        phy_vars_ue->PHY_measurements.rx_power[0][0],
        phy_vars_ue->PHY_measurements.rx_power[0][1],
        phy_vars_ue->PHY_measurements.rx_power_avg_dB[0],
        phy_vars_ue->PHY_measurements.rx_power_avg[0],
        phy_vars_ue->rx_total_gain_dB);

  LOG_I(PHY,"[UE %d] N0 %d dBm digital (%d, %d) dB, linear (%d, %d), avg noise power %d dB (%d lin)\n",
        phy_vars_ue->Mod_id,
        phy_vars_ue->PHY_measurements.n0_power_tot_dBm,
        phy_vars_ue->PHY_measurements.n0_power_dB[0],
        phy_vars_ue->PHY_measurements.n0_power_dB[1],
        phy_vars_ue->PHY_measurements.n0_power[0],
        phy_vars_ue->PHY_measurements.n0_power[1],
        phy_vars_ue->PHY_measurements.n0_power_avg_dB,
        phy_vars_ue->PHY_measurements.n0_power_avg);
#endif

  pbch_decoded = 0;

  for (frame_mod4=0; frame_mod4<4; frame_mod4++) {
    pbch_tx_ant = rx_pbch(&phy_vars_ue->lte_ue_common_vars,
                          phy_vars_ue->lte_ue_pbch_vars[0],
                          frame_parms,
                          0,
                          SISO,
                          phy_vars_ue->high_speed_flag,
                          frame_mod4);

    if ((pbch_tx_ant>0) && (pbch_tx_ant<=2)) {
      pbch_decoded = 1;
      break;
    }

    pbch_tx_ant = rx_pbch(&phy_vars_ue->lte_ue_common_vars,
                          phy_vars_ue->lte_ue_pbch_vars[0],
                          frame_parms,
                          0,
                          ALAMOUTI,
                          phy_vars_ue->high_speed_flag,
                          frame_mod4);

    if ((pbch_tx_ant>0) && (pbch_tx_ant<=2)) {
      pbch_decoded = 1;
      break;
    }
  }


  if (pbch_decoded) {

    frame_parms->nb_antennas_tx_eNB = pbch_tx_ant;

    // set initial transmission mode to 1 or 2 depending on number of detected TX antennas
    frame_parms->mode1_flag = (pbch_tx_ant==1);
    // openair_daq_vars.dlsch_transmission_mode = (pbch_tx_ant>1) ? 2 : 1;


    // flip byte endian on 24-bits for MIB
    //    dummy = phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[0];
    //    phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[0] = phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[2];
    //    phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[2] = dummy;

    // now check for Bandwidth of Cell
    dummy = (phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[2]>>5)&7;

    switch (dummy) {

    case 0 :
      frame_parms->N_RB_DL = 6;
      break;

    case 1 :
      frame_parms->N_RB_DL = 15;
      break;

    case 2 :
      frame_parms->N_RB_DL = 25;
      break;

    case 3 :
      frame_parms->N_RB_DL = 50;
      break;

    case 4 :
      frame_parms->N_RB_DL = 75;
      break;

    case 5:
      frame_parms->N_RB_DL = 100;
      break;

    default:
      LOG_E(PHY,"[UE%d] Initial sync: PBCH decoding: Unknown N_RB_DL\n",phy_vars_ue->Mod_id);
      return -1;
      break;
    }


    // now check for PHICH parameters
    frame_parms->phich_config_common.phich_duration = (PHICH_DURATION_t)((phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[2]>>4)&1);
    dummy = (phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[2]>>2)&3;

    switch (dummy) {
    case 0:
      frame_parms->phich_config_common.phich_resource = oneSixth;
      sprintf(phich_resource,"1/6");
      break;

    case 1:
      frame_parms->phich_config_common.phich_resource = half;
      sprintf(phich_resource,"1/2");
      break;

    case 2:
      frame_parms->phich_config_common.phich_resource = one;
      sprintf(phich_resource,"1");
      break;

    case 3:
      frame_parms->phich_config_common.phich_resource = two;
      sprintf(phich_resource,"2");
      break;

    default:
      LOG_E(PHY,"[UE%d] Initial sync: Unknown PHICH_DURATION\n",phy_vars_ue->Mod_id);
      return -1;
      break;
    }

    phy_vars_ue->frame_rx =   (((phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[2]&3)<<6) + (phy_vars_ue->lte_ue_pbch_vars[0]->decoded_output[1]>>2))<<2;
    phy_vars_ue->frame_rx += frame_mod4;

#ifndef USER_MODE
    // one frame delay
    phy_vars_ue->frame_rx ++;
#endif
    phy_vars_ue->frame_tx = phy_vars_ue->frame_rx;
#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"[UE%d] Initial sync: pbch decoded sucessfully mode1_flag %d, tx_ant %d, frame %d, N_RB_DL %d, phich_duration %d, phich_resource %s!\n",
          phy_vars_ue->Mod_id,
          frame_parms->mode1_flag,
          pbch_tx_ant,
          phy_vars_ue->frame_rx,
          frame_parms->N_RB_DL,
          frame_parms->phich_config_common.phich_duration,
          phich_resource);  //frame_parms->phich_config_common.phich_resource);
#endif
    return(0);
  } else {
    return(-1);
  }

}

char phich_string[13][4] = {"","1/6","","1/2","","","one","","","","","","two"};
char duplex_string[2][4] = {"FDD","TDD"};
char prefix_string[2][9] = {"NORMAL","EXTENDED"};

int initial_sync(PHY_VARS_UE *phy_vars_ue, runmode_t mode)
{

  int32_t sync_pos,sync_pos2,sync_pos_slot;
  int32_t metric_fdd_ncp=0,metric_fdd_ecp=0,metric_tdd_ncp=0,metric_tdd_ecp=0;
  uint8_t phase_fdd_ncp,phase_fdd_ecp,phase_tdd_ncp,phase_tdd_ecp;
  uint8_t flip_fdd_ncp,flip_fdd_ecp,flip_tdd_ncp,flip_tdd_ecp;
  //  uint16_t Nid_cell_fdd_ncp=0,Nid_cell_fdd_ecp=0,Nid_cell_tdd_ncp=0,Nid_cell_tdd_ecp=0;
  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_ue->lte_frame_parms;
  int ret=-1;
  int aarx,rx_power=0;
  /*#ifdef OAI_USRP
  __m128i *rxdata128;
  #endif*/
  //  LOG_I(PHY,"**************************************************************\n");
  // First try FDD normal prefix
  frame_parms->Ncp=NORMAL;
  frame_parms->frame_type=FDD;
  init_frame_parms(frame_parms,1);
  /*
  write_output("rxdata0.m","rxd0",phy_vars_ue->lte_ue_common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);
  exit(-1);
  */
  sync_pos = lte_sync_time(phy_vars_ue->lte_ue_common_vars.rxdata,
                           frame_parms,
                           (int *)&phy_vars_ue->lte_ue_common_vars.eNb_id);

  //  write_output("rxdata1.m","rxd1",phy_vars_ue->lte_ue_common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);
  if (sync_pos >= frame_parms->nb_prefix_samples)
    sync_pos2 = sync_pos - frame_parms->nb_prefix_samples;
  else
    sync_pos2 = sync_pos + FRAME_LENGTH_COMPLEX_SAMPLES - frame_parms->nb_prefix_samples;

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync : Estimated PSS position %d, Nid2 %d\n",phy_vars_ue->Mod_id,sync_pos,phy_vars_ue->lte_ue_common_vars.eNb_id);
#endif

  // SSS detection

  // PSS is hypothesized in last symbol of first slot in Frame
  sync_pos_slot = (frame_parms->samples_per_tti>>1) - frame_parms->ofdm_symbol_size - frame_parms->nb_prefix_samples;

  if (sync_pos2 >= sync_pos_slot)
    phy_vars_ue->rx_offset = sync_pos2 - sync_pos_slot;
  else
    phy_vars_ue->rx_offset = FRAME_LENGTH_COMPLEX_SAMPLES + sync_pos2 - sync_pos_slot;

  if (((sync_pos2 - sync_pos_slot) >=0 ) &&
      ((sync_pos2 - sync_pos_slot) < ((FRAME_LENGTH_COMPLEX_SAMPLES-frame_parms->samples_per_tti/2)))) {
#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"Calling sss detection (FDD normal CP)\n");
#endif
    rx_sss(phy_vars_ue,&metric_fdd_ncp,&flip_fdd_ncp,&phase_fdd_ncp);
    frame_parms->nushift  = frame_parms->Nid_cell%6;

    if (flip_fdd_ncp==1)
      phy_vars_ue->rx_offset += (FRAME_LENGTH_COMPLEX_SAMPLES>>1);

    init_frame_parms(&phy_vars_ue->lte_frame_parms,1);
    lte_gold(frame_parms,phy_vars_ue->lte_gold_table[0],frame_parms->Nid_cell);
    ret = pbch_detection(phy_vars_ue,mode);
    //   write_output("rxdata2.m","rxd2",phy_vars_ue->lte_ue_common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);

#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"FDD Normal prefix: CellId %d metric %d, phase %d, flip %d, pbch %d\n",
          frame_parms->Nid_cell,metric_fdd_ncp,phase_fdd_ncp,flip_fdd_ncp,ret);
#endif
  } else {
#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"FDD Normal prefix: SSS error condition: sync_pos %d, sync_pos_slot %d\n", sync_pos, sync_pos_slot);
#endif
  }


  if (ret==-1) {

    // Now FDD extended prefix
    frame_parms->Ncp=EXTENDED;
    frame_parms->frame_type=FDD;
    init_frame_parms(frame_parms,1);

    if (sync_pos < frame_parms->nb_prefix_samples)
      sync_pos2 = sync_pos + FRAME_LENGTH_COMPLEX_SAMPLES - frame_parms->nb_prefix_samples;
    else
      sync_pos2 = sync_pos - frame_parms->nb_prefix_samples;

    // PSS is hypothesized in last symbol of first slot in Frame
    sync_pos_slot = (frame_parms->samples_per_tti>>1) - frame_parms->ofdm_symbol_size - (frame_parms->nb_prefix_samples);

    if (sync_pos2 >= sync_pos_slot)
      phy_vars_ue->rx_offset = sync_pos2 - sync_pos_slot;
    else
      phy_vars_ue->rx_offset = FRAME_LENGTH_COMPLEX_SAMPLES + sync_pos2 - sync_pos_slot;

    //msg("nb_prefix_samples %d, rx_offset %d\n",frame_parms->nb_prefix_samples,phy_vars_ue->rx_offset);

    if (((sync_pos2 - sync_pos_slot) >=0 ) &&
        ((sync_pos2 - sync_pos_slot) < ((FRAME_LENGTH_COMPLEX_SAMPLES-frame_parms->samples_per_tti/2)))) {

      rx_sss(phy_vars_ue,&metric_fdd_ecp,&flip_fdd_ecp,&phase_fdd_ecp);
      frame_parms->nushift  = frame_parms->Nid_cell%6;

      if (flip_fdd_ecp==1)
        phy_vars_ue->rx_offset += (FRAME_LENGTH_COMPLEX_SAMPLES>>1);

      init_frame_parms(&phy_vars_ue->lte_frame_parms,1);
      lte_gold(frame_parms,phy_vars_ue->lte_gold_table[0],frame_parms->Nid_cell);
      ret = pbch_detection(phy_vars_ue,mode);
      //     write_output("rxdata3.m","rxd3",phy_vars_ue->lte_ue_common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);
#ifdef DEBUG_INITIAL_SYNCH
      LOG_I(PHY,"FDD Extended prefix: CellId %d metric %d, phase %d, flip %d, pbch %d\n",
            frame_parms->Nid_cell,metric_fdd_ecp,phase_fdd_ecp,flip_fdd_ecp,ret);
#endif
    } else {
#ifdef DEBUG_INITIAL_SYNCH
      LOG_I(PHY,"FDD Extended prefix: SSS error condition: sync_pos %d, sync_pos_slot %d\n", sync_pos, sync_pos_slot);
#endif
    }

    if (ret==-1) {
      // Now TDD normal prefix
      frame_parms->Ncp=NORMAL;
      frame_parms->frame_type=TDD;
      init_frame_parms(frame_parms,1);

      if (sync_pos >= frame_parms->nb_prefix_samples)
        sync_pos2 = sync_pos - frame_parms->nb_prefix_samples;
      else
        sync_pos2 = sync_pos + FRAME_LENGTH_COMPLEX_SAMPLES - frame_parms->nb_prefix_samples;

      // PSS is hypothesized in 2nd symbol of third slot in Frame (S-subframe)
      sync_pos_slot = frame_parms->samples_per_tti +
                      (frame_parms->ofdm_symbol_size<<1) +
                      frame_parms->nb_prefix_samples0 +
                      (frame_parms->nb_prefix_samples);

      if (sync_pos2 >= sync_pos_slot)
        phy_vars_ue->rx_offset = sync_pos2 - sync_pos_slot;
      else
        phy_vars_ue->rx_offset = (FRAME_LENGTH_COMPLEX_SAMPLES>>1) + sync_pos2 - sync_pos_slot;

      /*if (((sync_pos2 - sync_pos_slot) >=0 ) &&
      ((sync_pos2 - sync_pos_slot) < ((FRAME_LENGTH_COMPLEX_SAMPLES-frame_parms->samples_per_tti/2)))) {*/


      rx_sss(phy_vars_ue,&metric_tdd_ncp,&flip_tdd_ncp,&phase_tdd_ncp);

      if (flip_tdd_ncp==1)
        phy_vars_ue->rx_offset += (FRAME_LENGTH_COMPLEX_SAMPLES>>1);

      frame_parms->nushift  = frame_parms->Nid_cell%6;
      init_frame_parms(&phy_vars_ue->lte_frame_parms,1);

      lte_gold(frame_parms,phy_vars_ue->lte_gold_table[0],frame_parms->Nid_cell);
      ret = pbch_detection(phy_vars_ue,mode);
      //      write_output("rxdata4.m","rxd4",phy_vars_ue->lte_ue_common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);

#ifdef DEBUG_INITIAL_SYNCH
      LOG_I(PHY,"TDD Normal prefix: CellId %d metric %d, phase %d, flip %d, pbch %d\n",
            frame_parms->Nid_cell,metric_tdd_ncp,phase_tdd_ncp,flip_tdd_ncp,ret);
#endif
      /*}
          else {
      #ifdef DEBUG_INITIAL_SYNCH
              LOG_I(PHY,"TDD Normal prefix: SSS error condition: sync_pos %d, sync_pos_slot %d\n", sync_pos, sync_pos_slot);
      #endif
      }*/


      if (ret==-1) {
        // Now TDD extended prefix
        frame_parms->Ncp=EXTENDED;
        frame_parms->frame_type=TDD;
        init_frame_parms(frame_parms,1);
        sync_pos2 = sync_pos - frame_parms->nb_prefix_samples;

        if (sync_pos >= frame_parms->nb_prefix_samples)
          sync_pos2 = sync_pos - frame_parms->nb_prefix_samples;
        else
          sync_pos2 = sync_pos + FRAME_LENGTH_COMPLEX_SAMPLES - frame_parms->nb_prefix_samples;

        // PSS is hypothesized in 2nd symbol of third slot in Frame (S-subframe)
        sync_pos_slot = frame_parms->samples_per_tti + (frame_parms->ofdm_symbol_size<<1) + (frame_parms->nb_prefix_samples<<1);

        if (sync_pos2 >= sync_pos_slot)
          phy_vars_ue->rx_offset = sync_pos2 - sync_pos_slot;
        else
          phy_vars_ue->rx_offset = (FRAME_LENGTH_COMPLEX_SAMPLES>>1) + sync_pos2 - sync_pos_slot;

        /*if (((sync_pos2 - sync_pos_slot) >=0 ) &&
          ((sync_pos2 - sync_pos_slot) < ((FRAME_LENGTH_COMPLEX_SAMPLES-frame_parms->samples_per_tti/2)))) {*/

        rx_sss(phy_vars_ue,&metric_tdd_ecp,&flip_tdd_ecp,&phase_tdd_ecp);
        frame_parms->nushift  = frame_parms->Nid_cell%6;

        if (flip_tdd_ecp==1)
          phy_vars_ue->rx_offset += (FRAME_LENGTH_COMPLEX_SAMPLES>>1);

        init_frame_parms(&phy_vars_ue->lte_frame_parms,1);
        lte_gold(frame_parms,phy_vars_ue->lte_gold_table[0],frame_parms->Nid_cell);
        ret = pbch_detection(phy_vars_ue,mode);

	//	write_output("rxdata5.m","rxd5",phy_vars_ue->lte_ue_common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);
#ifdef DEBUG_INITIAL_SYNCH
        LOG_I(PHY,"TDD Extended prefix: CellId %d metric %d, phase %d, flip %d, pbch %d\n",
              frame_parms->Nid_cell,metric_tdd_ecp,phase_tdd_ecp,flip_tdd_ecp,ret);
#endif
        /*}
        else {
        #ifdef DEBUG_INITIAL_SYNCH
          LOG_I(PHY,"TDD Extended prefix: SSS error condition: sync_pos %d, sync_pos_slot %d\n", sync_pos, sync_pos_slot);
        #endif
        }*/

      }
    }
  }

  if (ret==0) {  // PBCH found so indicate sync to higher layers and configure frame parameters

#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"[UE%d] In synch, rx_offset %d samples\n",phy_vars_ue->Mod_id, phy_vars_ue->rx_offset);
#endif

    if (phy_vars_ue->UE_scan_carrier == 0) {
      if (phy_vars_ue->mac_enabled==1) {
	LOG_I(PHY,"[UE%d] Sending synch status to higher layers\n",phy_vars_ue->Mod_id);
	//mac_resynch();
	mac_xface->dl_phy_sync_success(phy_vars_ue->Mod_id,phy_vars_ue->frame_rx,0,1);//phy_vars_ue->lte_ue_common_vars.eNb_id);
	phy_vars_ue->UE_mode[0] = PRACH;
      }
      else {
	phy_vars_ue->UE_mode[0] = PUSCH;
      }

      generate_pcfich_reg_mapping(frame_parms);
      generate_phich_reg_mapping(frame_parms);
      //    init_prach625(frame_parms);

      //phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors=0;
      phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors_conseq=0;
    //phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors_last=0;
    }

    LOG_I(PHY,"[UE %d] Frame %d RRC Measurements => rssi %3.1f dBm (dig %3.1f dB, gain %d), N0 %d dBm,  rsrp %3.1f dBm/RE, rsrq %3.1f dB\n",phy_vars_ue->Mod_id,
	  phy_vars_ue->frame_rx,
	  10*log10(phy_vars_ue->PHY_measurements.rssi)-phy_vars_ue->rx_total_gain_dB,
	  10*log10(phy_vars_ue->PHY_measurements.rssi),
	  phy_vars_ue->rx_total_gain_dB,
	  phy_vars_ue->PHY_measurements.n0_power_tot_dBm,
	  10*log10(phy_vars_ue->PHY_measurements.rsrp[0])-phy_vars_ue->rx_total_gain_dB,
	  (10*log10(phy_vars_ue->PHY_measurements.rsrq[0])));
    
    
    LOG_I(PHY,"[UE %d] Frame %d MIB Information => %s, %s, NidCell %d, N_RB_DL %d, PHICH DURATION %d, PHICH RESOURCE %s, TX_ANT %d\n",
	  phy_vars_ue->Mod_id,
	  phy_vars_ue->frame_rx,
	  duplex_string[phy_vars_ue->lte_frame_parms.frame_type],
	  prefix_string[phy_vars_ue->lte_frame_parms.Ncp],
	  phy_vars_ue->lte_frame_parms.Nid_cell,
	  phy_vars_ue->lte_frame_parms.N_RB_DL,
	  phy_vars_ue->lte_frame_parms.phich_config_common.phich_duration,
	  phich_string[phy_vars_ue->lte_frame_parms.phich_config_common.phich_resource],
	  phy_vars_ue->lte_frame_parms.nb_antennas_tx_eNB);
#if defined(OAI_USRP) || defined(EXMIMO) || defined(OAI_LMSSDR)
    LOG_I(PHY,"[UE %d] Frame %d Measured Carrier Frequency %.0f Hz (offset %d Hz)\n",
	  phy_vars_ue->Mod_id,
	  phy_vars_ue->frame_rx,
	  openair0_cfg[0].rx_freq[0]-phy_vars_ue->lte_ue_common_vars.freq_offset,
	  phy_vars_ue->lte_ue_common_vars.freq_offset);
#endif
  } else {
#ifdef DEBUG_INITIAL_SYNC
    LOG_I(PHY,"[UE%d] Initial sync : PBCH not ok\n",phy_vars_ue->Mod_id);
    LOG_I(PHY,"[UE%d] Initial sync : Estimated PSS position %d, Nid2 %d\n",phy_vars_ue->Mod_id,sync_pos,phy_vars_ue->lte_ue_common_vars.eNb_id);
    /*      LOG_I(PHY,"[UE%d] Initial sync: (metric fdd_ncp %d (%d), metric fdd_ecp %d (%d), metric_tdd_ncp %d (%d), metric_tdd_ecp %d (%d))\n",
          phy_vars_ue->Mod_id,
          metric_fdd_ncp,Nid_cell_fdd_ncp,
          metric_fdd_ecp,Nid_cell_fdd_ecp,
          metric_tdd_ncp,Nid_cell_tdd_ncp,
          metric_tdd_ecp,Nid_cell_tdd_ecp);*/
    LOG_I(PHY,"[UE%d] Initial sync : Estimated Nid_cell %d, Frame_type %d\n",phy_vars_ue->Mod_id,
          frame_parms->Nid_cell,frame_parms->frame_type);
#endif

    phy_vars_ue->UE_mode[0] = NOT_SYNCHED;
    phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors_last=phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors;
    phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors++;
    phy_vars_ue->lte_ue_pbch_vars[0]->pdu_errors_conseq++;

  }

  // gain control
  if (ret!=0) { //we are not synched, so we cannot use rssi measurement (which is based on channel estimates)
    rx_power = 0;

    // do a measurement on the best guess of the PSS
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
      rx_power += signal_energy(&phy_vars_ue->lte_ue_common_vars.rxdata[aarx][sync_pos2],
				frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples);
    
    /*
    // do a measurement on the full frame
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
      rx_power += signal_energy(&phy_vars_ue->lte_ue_common_vars.rxdata[aarx][0],
				frame_parms->samples_per_tti*10);
    */

    // we might add a low-pass filter here later
    phy_vars_ue->PHY_measurements.rx_power_avg[0] = rx_power/frame_parms->nb_antennas_rx; 

    phy_vars_ue->PHY_measurements.rx_power_avg_dB[0] = dB_fixed(phy_vars_ue->PHY_measurements.rx_power_avg[0]);

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync : Estimated power: %d dB\n",phy_vars_ue->Mod_id,phy_vars_ue->PHY_measurements.rx_power_avg_dB[0] );
#endif

#ifdef EXMIMO
  if ((openair_daq_vars.rx_gain_mode == DAQ_AGC_ON) &&
      (mode != rx_calib_ue) && (mode != rx_calib_ue_med) && (mode != rx_calib_ue_byp) )
    //phy_adjust_gain(phy_vars_ue,0);
    gain_control_all(phy_vars_ue->PHY_measurements.rx_power_avg_dB[0],0);

#else
#ifndef OAI_USRP
#ifndef OAI_BLADERF 
#ifndef OAI_LMSSDR
  phy_adjust_gain(phy_vars_ue,phy_vars_ue->PHY_measurements.rx_power_avg_dB[0],0);
#endif
#endif
#endif
#endif
  }
  else {
#ifdef EXMIMO
  if ((openair_daq_vars.rx_gain_mode == DAQ_AGC_ON) &&
      (mode != rx_calib_ue) && (mode != rx_calib_ue_med) && (mode != rx_calib_ue_byp) )
    //phy_adjust_gain(phy_vars_ue,0);
    gain_control_all(dB_fixed(phy_vars_ue->PHY_measurements.rssi),0);

#else
#ifndef OAI_USRP
#ifndef OAI_BLADERF 
#ifndef OAI_LMSSDR
  phy_adjust_gain(phy_vars_ue,dB_fixed(phy_vars_ue->PHY_measurements.rssi),0);
#endif
#endif
#endif
#endif
  }

  //  exit_fun("debug exit");
  return ret;
}

