/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
//#include "SCHED/defs.h"
//#include "SCHED/extern.h"
//#include "defs.h"
//#include "extern.h"


#include "common_lib.h"

#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"

extern openair0_config_t openair0_cfg[];
static  nfapi_nr_config_request_t config_t;
static  nfapi_nr_config_request_t* config =&config_t;
int cnt=0;
/* forward declarations */
void set_default_frame_parms_single(nfapi_nr_config_request_t *config, NR_DL_FRAME_PARMS *frame_parms);

//#define DEBUG_INITIAL_SYNCH

int nr_pbch_detection(PHY_VARS_NR_UE *ue, runmode_t mode)
{
  printf("nr pbch detec RB_DL %d\n", ue->frame_parms.N_RB_DL);
  uint8_t l,pbch_decoded,frame_mod4,pbch_tx_ant,dummy;
  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  char phich_resource[6];
  int ret =-1;

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync: starting PBCH detection (rx_offset %d)\n",ue->Mod_id,
        ue->rx_offset);
#endif

  int nb_prefix_samples0 = frame_parms->nb_prefix_samples0;
  frame_parms->nb_prefix_samples0 = 0;

  //symbol 1
  nr_slot_fep(ue,
    	   5,
    	   0,
    	   ue->rx_offset,
    	   0,
    	   1,
		   NR_PBCH_EST);

  //symbol 2
  nr_slot_fep(ue,
    	   6,
    	   0,
    	   ue->rx_offset,
    	   0,
    	   1,
		   NR_PBCH_EST);

  //symbol 3
  nr_slot_fep(ue,
    	   7,
    	   0,
    	   ue->rx_offset,
    	   0,
    	   1,
		   NR_PBCH_EST);

  frame_parms->nb_prefix_samples0 = nb_prefix_samples0;

  pbch_decoded = 0;

  printf("pbch_detection nid_cell %d\n",frame_parms->Nid_cell);
  
  //for (frame_mod4=0; frame_mod4<4; frame_mod4++) {
    ret = nr_rx_pbch(ue,
						  &ue->proc.proc_rxtx[0],
                          ue->pbch_vars[0],
                          frame_parms,
                          0,
                          SISO,
                          ue->high_speed_flag,
                          frame_mod4);

    pbch_decoded = 0; //to be updated
//      break;
  //}


  if (ret==0) {

    frame_parms->nb_antenna_ports_eNB = 1; //pbch_tx_ant;

    // set initial transmission mode to 1 or 2 depending on number of detected TX antennas
    //frame_parms->mode1_flag = (pbch_tx_ant==1);
    // openair_daq_vars.dlsch_transmission_mode = (pbch_tx_ant>1) ? 2 : 1;


    // flip byte endian on 24-bits for MIB
    //    dummy = ue->pbch_vars[0]->decoded_output[0];
    //    ue->pbch_vars[0]->decoded_output[0] = ue->pbch_vars[0]->decoded_output[2];
    //    ue->pbch_vars[0]->decoded_output[2] = dummy;

    for(int i=0; i<RX_NB_TH;i++)
    {

        ue->proc.proc_rxtx[i].frame_tx = ue->proc.proc_rxtx[0].frame_rx;
    }
#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"[UE%d] Initial sync: pbch decoded sucessfully mode1_flag %d, tx_ant %d, frame %d\n",
          ue->Mod_id,
          frame_parms->mode1_flag,
          pbch_tx_ant,
          ue->proc.proc_rxtx[0].frame_rx,);
#endif
    return(0);
  } else {
    return(-1);
  }

}

char duplex_string[2][4] = {"FDD","TDD"};
char prefix_string[2][9] = {"NORMAL","EXTENDED"};

int nr_initial_sync(PHY_VARS_NR_UE *ue, runmode_t mode)
{

  int32_t sync_pos, sync_pos2, k_ssb, N_ssb_crb, sync_pos_slot;
  int32_t metric_fdd_ncp=0;
  uint8_t phase_fdd_ncp;

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int ret=-1;
  int aarx,rx_power=0;
  //nfapi_nr_config_request_t* config;

  /*offset parameters to be updated from higher layer */
  k_ssb =0;
  N_ssb_crb = 0;

  /*#ifdef OAI_USRP
  __m128i *rxdata128;
  #endif*/
  //  LOG_I(PHY,"**************************************************************\n");
  // First try FDD normal prefix
  frame_parms->Ncp=NORMAL;
  frame_parms->frame_type=TDD;
  set_default_frame_parms_single(config,frame_parms);
  nr_init_frame_parms_ue(frame_parms);
  LOG_D(PHY,"nr_initial sync ue RB_DL %d\n", ue->frame_parms.N_RB_DL);
  /*
  write_output("rxdata0.m","rxd0",ue->common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);
  exit(-1);
  */

  /*   Initial synchronisation
   *
  *                                 1 radio frame = 10 ms
  *     <--------------------------------------------------------------------------->
  *     -----------------------------------------------------------------------------
  *     |                                 Received UE data buffer                    |
  *     ----------------------------------------------------------------------------
  *                     --------------------------
  *     <-------------->| pss | pbch | sss | pbch |
  *                     --------------------------
  *          sync_pos            SS/PBCH block
  */
  cnt++;
  if (cnt >100){
	  cnt =0;
  /* process pss search on received buffer */
  sync_pos = pss_synchro_nr(ue, NO_RATE_CHANGE);

  sync_pos_slot = (frame_parms->samples_per_tti>>1) - 3*(frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);

  if (sync_pos >= frame_parms->nb_prefix_samples)
      sync_pos2 = sync_pos - frame_parms->nb_prefix_samples;
    else
      sync_pos2 = sync_pos + FRAME_LENGTH_COMPLEX_SAMPLES - frame_parms->nb_prefix_samples;

  /* offset is used by sss serach as it is returned from pss search */
  if (sync_pos2 >= sync_pos_slot)
      ue->rx_offset = sync_pos2 - sync_pos_slot;
    else
      ue->rx_offset = FRAME_LENGTH_COMPLEX_SAMPLES + sync_pos2 - sync_pos_slot;

  LOG_D(PHY,"sync_pos %d sync_pos_slot %d rx_offset %d\n",sync_pos,sync_pos_slot, ue->rx_offset);


  //  write_output("rxdata1.m","rxd1",ue->common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync : Estimated PSS position %d, Nid2 %d\n", ue->Mod_id, sync_pos,ue->common_vars.eNb_id);
#endif

  /* check that SSS/PBCH block is continuous inside the received buffer */
  if (sync_pos < (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->ttis_per_subframe*frame_parms->samples_per_tti - (NB_SYMBOLS_PBCH * frame_parms->ofdm_symbol_size))) {

#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"Calling sss detection (normal CP)\n");
#endif

    rx_sss_nr(ue,&metric_fdd_ncp,&phase_fdd_ncp);

    //set_default_frame_parms_single(config,&ue->frame_parms);
    nr_init_frame_parms_ue(config,&ue->frame_parms);

    nr_gold_pbch(ue);
    ret = nr_pbch_detection(ue,mode);
    
    nr_gold_pdcch(ue,0, 2);
    nr_slot_fep(ue,0, 0, ue->rx_offset, 1, 1, NR_PDCCH_EST);
    nr_slot_fep(ue,1, 0, ue->rx_offset, 1, 1, NR_PDCCH_EST);
	

    LOG_I(PHY,"[UE  %d] AUTOTEST Cell Sync : frame = %d, rx_offset %d, freq_offset %d \n",
              ue->Mod_id,
              ue->proc.proc_rxtx[0].frame_rx,
              ue->rx_offset,
              ue->common_vars.freq_offset );

    //ret = -1;  //to be deleted
    //   write_output("rxdata2.m","rxd2",ue->common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);

#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"FDD Normal prefix: CellId %d metric %d, phase %d, flip %d, pbch %d\n",
          frame_parms->Nid_cell,metric_fdd_ncp,phase_fdd_ncp,flip_fdd_ncp,ret);
#endif
  }
  else {
#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"FDD Normal prefix: SSS error condition: sync_pos %d, sync_pos_slot %d\n", sync_pos, sync_pos_slot);
#endif
  }
  }
  else {
	  ret = -1;
  }

  /* Consider this is a false detection if the offset is > 1000 Hz */
  if( (abs(ue->common_vars.freq_offset) > 150) && (ret == 0) )
  {
	  ret=-1;
#if DISABLE_LOG_X
	  printf("Ignore MIB with high freq offset [%d Hz] estimation \n",ue->common_vars.freq_offset);
#else
	  LOG_E(HW, "Ignore MIB with high freq offset [%d Hz] estimation \n",ue->common_vars.freq_offset);
#endif
  }

  if (ret==0) {  // PBCH found so indicate sync to higher layers and configure frame parameters

    //#ifdef DEBUG_INITIAL_SYNCH
#if DISABLE_LOG_X
    printf("[UE%d] In synch, rx_offset %d samples\n",ue->Mod_id, ue->rx_offset);
#else
    LOG_I(PHY, "[UE%d] In synch, rx_offset %d samples\n",ue->Mod_id, ue->rx_offset);
#endif
    //#endif

    if (ue->UE_scan_carrier == 0) {

    #if UE_AUTOTEST_TRACE
      LOG_I(PHY,"[UE  %d] AUTOTEST Cell Sync : frame = %d, rx_offset %d, freq_offset %d \n",
              ue->Mod_id,
              ue->proc.proc_rxtx[0].frame_rx,
              ue->rx_offset,
              ue->common_vars.freq_offset );
    #endif

// send sync status to higher layers later when timing offset converge to target timing
#if OAISIM
      if (ue->mac_enabled==1) {
	LOG_I(PHY,"[UE%d] Sending synch status to higher layers\n",ue->Mod_id);
	//mac_resynch();
	mac_xface->dl_phy_sync_success(ue->Mod_id,ue->proc.proc_rxtx[0].frame_rx,0,1);//ue->common_vars.eNb_id);
	ue->UE_mode[0] = PRACH;
      }
      else {
	ue->UE_mode[0] = PUSCH;
      }
#endif

      ue->pbch_vars[0]->pdu_errors_conseq=0;

    }

#if DISABLE_LOG_X
    printf("[UE %d] Frame %d RRC Measurements => rssi %3.1f dBm (dig %3.1f dB, gain %d), N0 %d dBm,  rsrp %3.1f dBm/RE, rsrq %3.1f dB\n",ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  10*log10(ue->measurements.rssi)-ue->rx_total_gain_dB,
	  10*log10(ue->measurements.rssi),
	  ue->rx_total_gain_dB,
	  ue->measurements.n0_power_tot_dBm,
	  10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB,
	  (10*log10(ue->measurements.rsrq[0])));


    printf("[UE %d] Frame %d MIB Information => %s, %s, NidCell %d, N_RB_DL %d, PHICH DURATION %d, PHICH RESOURCE %s, TX_ANT %d\n",
	  ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  duplex_string[ue->frame_parms.frame_type],
	  prefix_string[ue->frame_parms.Ncp],
	  ue->frame_parms.Nid_cell,
	  ue->frame_parms.N_RB_DL,
	  ue->frame_parms.phich_config_common.phich_duration,
	  phich_string[ue->frame_parms.phich_config_common.phich_resource],
	  ue->frame_parms.nb_antenna_ports_eNB);
#else
    LOG_I(PHY, "[UE %d] Frame %d RRC Measurements => rssi %3.1f dBm (dig %3.1f dB, gain %d), N0 %d dBm,  rsrp %3.1f dBm/RE, rsrq %3.1f dB\n",ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  10*log10(ue->measurements.rssi)-ue->rx_total_gain_dB,
	  10*log10(ue->measurements.rssi),
	  ue->rx_total_gain_dB,
	  ue->measurements.n0_power_tot_dBm,
	  10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB,
	  (10*log10(ue->measurements.rsrq[0])));

/*    LOG_I(PHY, "[UE %d] Frame %d MIB Information => %s, %s, NidCell %d, N_RB_DL %d, PHICH DURATION %d, PHICH RESOURCE %s, TX_ANT %d\n",
	  ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  duplex_string[ue->frame_parms.frame_type],
	  prefix_string[ue->frame_parms.Ncp],
	  ue->frame_parms.Nid_cell,
	  ue->frame_parms.N_RB_DL,
	  ue->frame_parms.phich_config_common.phich_duration,
	  phich_string[ue->frame_parms.phich_config_common.phich_resource],
	  ue->frame_parms.nb_antenna_ports_eNB);*/
#endif

#if defined(OAI_USRP) || defined(EXMIMO) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
#  if DISABLE_LOG_X
    printf("[UE %d] Frame %d Measured Carrier Frequency %.0f Hz (offset %d Hz)\n",
	  ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  openair0_cfg[0].rx_freq[0]-ue->common_vars.freq_offset,
	  ue->common_vars.freq_offset);
#  else
    LOG_I(PHY, "[UE %d] Frame %d Measured Carrier Frequency %.0f Hz (offset %d Hz)\n",
	  ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  openair0_cfg[0].rx_freq[0]-ue->common_vars.freq_offset,
	  ue->common_vars.freq_offset);
#  endif
#endif
  } else {
#ifdef DEBUG_INITIAL_SYNC
    LOG_I(PHY,"[UE%d] Initial sync : PBCH not ok\n",ue->Mod_id);
    LOG_I(PHY,"[UE%d] Initial sync : Estimated PSS position %d, Nid2 %d\n",ue->Mod_id,sync_pos,ue->common_vars.eNb_id);
    /*      LOG_I(PHY,"[UE%d] Initial sync: (metric fdd_ncp %d (%d), metric fdd_ecp %d (%d), metric_tdd_ncp %d (%d), metric_tdd_ecp %d (%d))\n",
          ue->Mod_id,
          metric_fdd_ncp,Nid_cell_fdd_ncp,
          metric_fdd_ecp,Nid_cell_fdd_ecp,
          metric_tdd_ncp,Nid_cell_tdd_ncp,
          metric_tdd_ecp,Nid_cell_tdd_ecp);*/
    LOG_I(PHY,"[UE%d] Initial sync : Estimated Nid_cell %d, Frame_type %d\n",ue->Mod_id,
          frame_parms->Nid_cell,frame_parms->frame_type);
#endif

    ue->UE_mode[0] = NOT_SYNCHED;
    ue->pbch_vars[0]->pdu_errors_last=ue->pbch_vars[0]->pdu_errors;
    ue->pbch_vars[0]->pdu_errors++;
    ue->pbch_vars[0]->pdu_errors_conseq++;

  }

  // gain control
  if (ret!=0) { //we are not synched, so we cannot use rssi measurement (which is based on channel estimates)
    rx_power = 0;

    // do a measurement on the best guess of the PSS
    //for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
    //  rx_power += signal_energy(&ue->common_vars.rxdata[aarx][sync_pos2],
	//			frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples);

    /*
    // do a measurement on the full frame
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
      rx_power += signal_energy(&ue->common_vars.rxdata[aarx][0],
				frame_parms->samples_per_subframe*10);
    */

    // we might add a low-pass filter here later
    ue->measurements.rx_power_avg[0] = rx_power/frame_parms->nb_antennas_rx;

    ue->measurements.rx_power_avg_dB[0] = dB_fixed(ue->measurements.rx_power_avg[0]);

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync : Estimated power: %d dB\n",ue->Mod_id,ue->measurements.rx_power_avg_dB[0] );
#endif

#ifndef OAI_USRP
#ifndef OAI_BLADERF
#ifndef OAI_LMSSDR
#ifndef OAI_ADRV9371_ZC706
  //phy_adjust_gain(ue,ue->measurements.rx_power_avg_dB[0],0);
#endif
#endif
#endif
#endif

  }
  else {

#ifndef OAI_USRP
#ifndef OAI_BLADERF
#ifndef OAI_LMSSDR
#ifndef OAI_ADRV9371_ZC706
  //phy_adjust_gain(ue,dB_fixed(ue->measurements.rssi),0);
#endif
#endif
#endif
#endif

  }

  //  exit_fun("debug exit");
  return ret;
}

