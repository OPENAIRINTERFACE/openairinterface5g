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

#include "phy_init.h"
#include "SCHED/sched_common.h"
#include "PHY/phy_extern.h"
#include "SIMULATION/TOOLS/sim.h"
/*#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "MBSFN-SubframeConfigList.h"*/
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include <math.h>
#include "openair1/PHY/defs_RU.h"

int nr_phy_init_RU(RU_t *ru) {

  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  int i,j;
  int p;
  int re;

  LOG_I(PHY,"Initializing RU signal buffers (if_south %s) nb_tx %d\n",ru_if_types[ru->if_south],ru->nb_tx);

  nfapi_nr_config_request_scf_t *cfg;
  ru->nb_log_antennas=0;
  for (int n=0;n<RC.nb_nr_L1_inst;n++) {
    cfg = &RC.gNB[n]->gNB_config;
    if (cfg->carrier_config.num_tx_ant.value > ru->nb_log_antennas) ru->nb_log_antennas = cfg->carrier_config.num_tx_ant.value;   
  }
  AssertFatal(ru->nb_log_antennas > 0 && ru->nb_log_antennas < 13, "ru->nb_log_antennas %d ! \n",ru->nb_log_antennas);
  if (ru->if_south <= REMOTE_IF5) { // this means REMOTE_IF5 or LOCAL_RF, so allocate memory for time-domain signals 
    // Time-domain signals
    ru->common.txdata        = (int32_t**)malloc16(ru->nb_tx*sizeof(int32_t*));
    ru->common.rxdata        = (int32_t**)malloc16(ru->nb_rx*sizeof(int32_t*) );


    for (i=0; i<ru->nb_tx; i++) {
      // Allocate 10 subframes of I/Q TX signal data (time) if not
      ru->common.txdata[i]  = (int32_t*)malloc16_clear( fp->samples_per_frame*sizeof(int32_t) );

      LOG_I(PHY,"[INIT] common.txdata[%d] = %p (%lu bytes)\n",i,ru->common.txdata[i],
	     fp->samples_per_frame*sizeof(int32_t));

    }
    for (i=0;i<ru->nb_rx;i++) {
      ru->common.rxdata[i] = (int32_t*)malloc16_clear( fp->samples_per_frame*sizeof(int32_t) );
    }
  } // IF5 or local RF
  else {
    //    LOG_I(PHY,"No rxdata/txdata for RU\n");
    ru->common.txdata        = (int32_t**)NULL;
    ru->common.rxdata        = (int32_t**)NULL;

  }
  if (ru->function != NGFI_RRU_IF5) { // we need to do RX/TX RU processing
    LOG_I(PHY,"nb_tx %d\n",ru->nb_tx);
    ru->common.rxdata_7_5kHz = (int32_t**)malloc16(ru->nb_rx*sizeof(int32_t*) );
    for (i=0;i<ru->nb_rx;i++) {
      ru->common.rxdata_7_5kHz[i] = (int32_t*)malloc16_clear( 2*fp->samples_per_subframe*2*sizeof(int32_t) );
      LOG_I(PHY,"rxdata_7_5kHz[%d] %p for RU %d\n",i,ru->common.rxdata_7_5kHz[i],ru->idx);
    }
  

    // allocate precoding input buffers (TX)
    ru->common.txdataF = (int32_t **)malloc16(ru->nb_tx*sizeof(int32_t*));
    for(i=0; i< ru->nb_tx; ++i)  ru->common.txdataF[i] = (int32_t*)malloc16_clear(fp->samples_per_frame_wCP*sizeof(int32_t)); // [hna] samples_per_frame without CP

    // allocate IFFT input buffers (TX)
    ru->common.txdataF_BF = (int32_t **)malloc16(ru->nb_tx*sizeof(int32_t*));
    LOG_I(PHY,"[INIT] common.txdata_BF= %p (%lu bytes)\n",ru->common.txdataF_BF,
	  ru->nb_tx*sizeof(int32_t*));
    for (i=0; i<ru->nb_tx; i++) {
      ru->common.txdataF_BF[i] = (int32_t*)malloc16_clear(fp->samples_per_subframe_wCP*sizeof(int32_t) );
      LOG_I(PHY,"txdataF_BF[%d] %p for RU %d\n",i,ru->common.txdataF_BF[i],ru->idx);
    }
    // allocate FFT output buffers (RX)
    ru->common.rxdataF     = (int32_t**)malloc16(ru->nb_rx*sizeof(int32_t*) );
    for (i=0; i<ru->nb_rx; i++) {    
      // allocate 2 subframes of I/Q signal data (frequency)
      ru->common.rxdataF[i] = (int32_t*)malloc16_clear(sizeof(int32_t)*(2*fp->samples_per_subframe_wCP) ); 
      LOG_I(PHY,"rxdataF[%d] %p for RU %d\n",i,ru->common.rxdataF[i],ru->idx);
    }

    /* number of elements of an array X is computed as sizeof(X) / sizeof(X[0]) */
    //    AssertFatal(ru->nb_rx <= sizeof(ru->prach_rxsigF) / sizeof(ru->prach_rxsigF[0]),
    //		"nb_antennas_rx too large");
    ru->prach_rxsigF = (int16_t**)malloc(ru->nb_rx * sizeof(int16_t*));

    for (i=0; i<ru->nb_rx; i++) {
      ru->prach_rxsigF[i] = (int16_t*)malloc16_clear( fp->ofdm_symbol_size*12*2*sizeof(int16_t) );
      LOG_D(PHY,"[INIT] prach_vars->rxsigF[%d] = %p\n",i,ru->prach_rxsigF[i]);
    }
    
    AssertFatal(RC.nb_nr_L1_inst <= NUMBER_OF_eNB_MAX,"gNB instances %d > %d\n",
		RC.nb_nr_L1_inst,NUMBER_OF_gNB_MAX);

    LOG_E(PHY,"[INIT] %s() RC.nb_nr_L1_inst:%d \n", __FUNCTION__, RC.nb_nr_L1_inst);
    
    int beam_count = 0;
    if (ru->nb_log_antennas>1) {
      for (p=0;p<ru->nb_log_antennas;p++) {
        if ((fp->L_ssb >> p) & 0x01)
          beam_count++;
      }
      AssertFatal(ru->nb_bfw==(beam_count*ru->nb_tx),"Number of beam weights from config file is %d while the expected number is %d",ru->nb_bfw,(beam_count*ru->nb_tx));
    
      int l_ind = 0;
      for (i=0; i<RC.nb_nr_L1_inst; i++) {
        for (p=0;p<ru->nb_log_antennas;p++) {
          if ((fp->L_ssb >> p) & 0x01)  {
	    ru->beam_weights[i][p] = (int32_t **)malloc16_clear(ru->nb_tx*sizeof(int32_t*));
	    for (j=0; j<ru->nb_tx; j++) {
	      ru->beam_weights[i][p][j] = (int32_t *)malloc16_clear(fp->ofdm_symbol_size*sizeof(int32_t));
              for (re=0; re<fp->ofdm_symbol_size; re++) 
		ru->beam_weights[i][p][j][re] = ru->bw_list[i][l_ind];
              //printf("Beam Weight %08x for beam %d and tx %d\n",ru->bw_list[i][l_ind],p,j);
              l_ind++; 
  	    } // for j
	  } // for p
        }
      } //for i
    }
  } // !=IF5

  ru->common.sync_corr = (uint32_t*)malloc16_clear( LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(uint32_t)*fp->samples_per_subframe_wCP );

  return(0);
}

void nr_phy_free_RU(RU_t *ru)
{
  int i,j;
  int p;

  LOG_I(PHY, "Feeing RU signal buffers (if_south %s) nb_tx %d\n", ru_if_types[ru->if_south], ru->nb_tx);

  free_and_zero(ru->nr_frame_parms);
  free_and_zero(ru->frame_parms);

  if (ru->if_south <= REMOTE_IF5) { // this means REMOTE_IF5 or LOCAL_RF, so free memory for time-domain signals
    for (i = 0; i < ru->nb_tx; i++) free_and_zero(ru->common.txdata[i]);
    for (i = 0; i < ru->nb_rx; i++) free_and_zero(ru->common.rxdata[i]);
    free_and_zero(ru->common.txdata);
    free_and_zero(ru->common.rxdata);
  } // else: IF5 or local RF -> nothing to free()

  if (ru->function != NGFI_RRU_IF5) { // we need to do RX/TX RU processing
    for (i = 0; i < ru->nb_rx; i++) free_and_zero(ru->common.rxdata_7_5kHz[i]);
    free_and_zero(ru->common.rxdata_7_5kHz);

    // free beamforming input buffers (TX)
    for (i = 0; i < 15; i++) free_and_zero(ru->common.txdataF[i]);
    free_and_zero(ru->common.txdataF);

    // free IFFT input buffers (TX)
    for (i = 0; i < ru->nb_tx; i++) free_and_zero(ru->common.txdataF_BF[i]);
    free_and_zero(ru->common.txdataF_BF);

    // free FFT output buffers (RX)
    for (i = 0; i < ru->nb_rx; i++) free_and_zero(ru->common.rxdataF[i]);
    free_and_zero(ru->common.rxdataF);

    for (i = 0; i < ru->nb_rx; i++) {
      free_and_zero(ru->prach_rxsigF[i]);
    }

    for (i = 0; i < RC.nb_nr_L1_inst; i++) {
      for (p = 0; p < 15; p++) {
	  for (j=0; j<ru->nb_tx; j++) free_and_zero(ru->beam_weights[i][p][j]);
	  free_and_zero(ru->beam_weights[i][p]);
      }
    }
  }
  free_and_zero(ru->common.sync_corr);
}
