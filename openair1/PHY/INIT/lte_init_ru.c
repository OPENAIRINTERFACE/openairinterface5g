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

#include "defs.h"
#include "SCHED/defs.h"
#include "PHY/extern.h"
#include "SIMULATION/TOOLS/defs.h"
#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "LAYER2/MAC/extern.h"
#include "MBSFN-SubframeConfigList.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include <math.h>

int phy_init_RU(RU_t *ru) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  int i,j;
  int p;
  int re;

  LOG_I(PHY,"Initializing RU signal buffers (if_south %s) nb_tx %d\n",ru_if_types[ru->if_south],ru->nb_tx);

  if (ru->if_south <= REMOTE_IF5) { // this means REMOTE_IF5 or LOCAL_RF, so allocate memory for time-domain signals 
    // Time-domain signals
    ru->common.txdata        = (int32_t**)malloc16(ru->nb_tx*sizeof(int32_t*));
    ru->common.rxdata        = (int32_t**)malloc16(ru->nb_rx*sizeof(int32_t*) );


    for (i=0; i<ru->nb_tx; i++) {
      // Allocate 10 subframes of I/Q TX signal data (time) if not
      ru->common.txdata[i]  = (int32_t*)malloc16_clear( fp->samples_per_tti*10*sizeof(int32_t) );

      LOG_I(PHY,"[INIT] common.txdata[%d] = %p (%lu bytes)\n",i,ru->common.txdata[i],
	     fp->samples_per_tti*10*sizeof(int32_t));

    }
    for (i=0;i<ru->nb_rx;i++) {
      ru->common.rxdata[i] = (int32_t*)malloc16_clear( fp->samples_per_tti*10*sizeof(int32_t) );
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
      ru->common.rxdata_7_5kHz[i] = (int32_t*)malloc16_clear( 2*fp->samples_per_tti*2*sizeof(int32_t) );
      LOG_I(PHY,"rxdata_7_5kHz[%d] %p for RU %d\n",i,ru->common.rxdata_7_5kHz[i],ru->idx);
    }
  

    // allocate IFFT input buffers (TX)
    ru->common.txdataF_BF = (int32_t **)malloc16(ru->nb_tx*sizeof(int32_t*));
    LOG_I(PHY,"[INIT] common.txdata_BF= %p (%lu bytes)\n",ru->common.txdataF_BF,
	  ru->nb_tx*sizeof(int32_t*));
    for (i=0; i<ru->nb_tx; i++) {
      ru->common.txdataF_BF[i] = (int32_t*)malloc16_clear(fp->symbols_per_tti*fp->ofdm_symbol_size*sizeof(int32_t) );
      LOG_I(PHY,"txdataF_BF[%d] %p for RU %d\n",i,ru->common.txdataF_BF[i],ru->idx);
    }
    // allocate FFT output buffers (RX)
    ru->common.rxdataF     = (int32_t**)malloc16(ru->nb_rx*sizeof(int32_t*) );
    for (i=0; i<ru->nb_rx; i++) {    
      // allocate 2 subframes of I/Q signal data (frequency)
      ru->common.rxdataF[i] = (int32_t*)malloc16_clear(sizeof(int32_t)*(2*fp->ofdm_symbol_size*fp->symbols_per_tti) ); 
      LOG_I(PHY,"rxdataF[%d] %p for RU %d\n",i,ru->common.rxdataF[i],ru->idx);
    }

    /* number of elements of an array X is computed as sizeof(X) / sizeof(X[0]) */
    //    AssertFatal(ru->nb_rx <= sizeof(ru->prach_rxsigF) / sizeof(ru->prach_rxsigF[0]),
    //		"nb_antennas_rx too large");
    ru->prach_rxsigF = (int16_t**)malloc(ru->nb_rx * sizeof(int16_t*));
    for (j=0;j<4;j++) ru->prach_rxsigF_br[j] = (int16_t**)malloc(ru->nb_rx * sizeof(int16_t*));

    for (i=0; i<ru->nb_rx; i++) {
      ru->prach_rxsigF[i] = (int16_t*)malloc16_clear( fp->ofdm_symbol_size*12*2*sizeof(int16_t) );
      LOG_D(PHY,"[INIT] prach_vars->rxsigF[%d] = %p\n",i,ru->prach_rxsigF[i]);
#ifdef Rel14
      for (j=0;j<4;j++) {
	ru->prach_rxsigF_br[j][i] = (int16_t*)malloc16_clear( fp->ofdm_symbol_size*12*2*sizeof(int16_t) );
	LOG_D(PHY,"[INIT] prach_vars_br->rxsigF[%d] = %p\n",i,ru->prach_rxsigF_br[j][i]);
      }
#endif
    }
    
    AssertFatal(RC.nb_L1_inst <= NUMBER_OF_eNB_MAX,"eNB instances %d > %d\n",
		RC.nb_L1_inst,NUMBER_OF_eNB_MAX);

    LOG_E(PHY,"[INIT] %s() RC.nb_L1_inst:%d \n", __FUNCTION__, RC.nb_L1_inst);

    for (i=0; i<RC.nb_L1_inst; i++) {
      for (p=0;p<15;p++) {
        LOG_D(PHY,"[INIT] %s() nb_antenna_ports_eNB:%d \n", __FUNCTION__, ru->eNB_list[i]->frame_parms.nb_antenna_ports_eNB);
	if (p<ru->eNB_list[i]->frame_parms.nb_antenna_ports_eNB || p==5) {
          LOG_D(PHY,"[INIT] %s() DO BEAM WEIGHTS nb_antenna_ports_eNB:%d nb_tx:%d\n", __FUNCTION__, ru->eNB_list[i]->frame_parms.nb_antenna_ports_eNB, ru->nb_tx);
	  ru->beam_weights[i][p] = (int32_t **)malloc16_clear(ru->nb_tx*sizeof(int32_t*));
	  for (j=0; j<ru->nb_tx; j++) {
	    ru->beam_weights[i][p][j] = (int32_t *)malloc16_clear(fp->ofdm_symbol_size*sizeof(int32_t));
	    // antenna ports 0-3 are mapped on antennas 0-3
	    // antenna port 4 is mapped on antenna 0
	    // antenna ports 5-14 are mapped on all antennas 
	    if (((p<4) && (p==j)) || ((p==4) && (j==0))) {
	      for (re=0; re<fp->ofdm_symbol_size; re++) 
              {
		ru->beam_weights[i][p][j][re] = 0x00007fff; 

                //LOG_D(PHY,"[INIT] lte_common_vars->beam_weights[%d][%d][%d][%d] = %d\n", i,p,j,re,ru->beam_weights[i][p][j][re]);
              }
	    }
	    else if (p>4) {
	      for (re=0; re<fp->ofdm_symbol_size; re++) 
              {
		ru->beam_weights[i][p][j][re] = 0x00007fff/ru->nb_tx; 
                //LOG_D(PHY,"[INIT] lte_common_vars->beam_weights[%d][%d][%d][%d] = %d\n", i,p,j,re,ru->beam_weights[i][p][j][re]);
              }
	    }  
	    //LOG_D(PHY,"[INIT] lte_common_vars->beam_weights[%d][%d] = %p (%lu bytes)\n", i,j,ru->beam_weights[i][p][j], fp->ofdm_symbol_size*sizeof(int32_t)); 
	  } // for (j=0
	} // if (p<ru
      } // for p
    } //for i
  } // !=IF5
  ru->common.sync_corr = (uint32_t*)malloc16_clear( LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(uint32_t)*fp->samples_per_tti );

  return(0);
}

void phy_free_RU(RU_t *ru)
{
  int i,j;
  int p;

  LOG_I(PHY, "Feeing RU signal buffers (if_south %s) nb_tx %d\n", ru_if_types[ru->if_south], ru->nb_tx);

  if (ru->if_south <= REMOTE_IF5) { // this means REMOTE_IF5 or LOCAL_RF, so free memory for time-domain signals
    for (i = 0; i < ru->nb_tx; i++) free_and_zero(ru->common.txdata[i]);
    for (i = 0; i < ru->nb_rx; i++) free_and_zero(ru->common.rxdata[i]);
    free_and_zero(ru->common.txdata);
    free_and_zero(ru->common.rxdata);
  } // else: IF5 or local RF -> nothing to free()

  if (ru->function != NGFI_RRU_IF5) { // we need to do RX/TX RU processing
    for (i = 0; i < ru->nb_rx; i++) free_and_zero(ru->common.rxdata_7_5kHz[i]);
    free_and_zero(ru->common.rxdata_7_5kHz);

    // free IFFT input buffers (TX)
    for (i = 0; i < ru->nb_tx; i++) free_and_zero(ru->common.txdataF_BF[i]);
    free_and_zero(ru->common.txdataF_BF);

    // free FFT output buffers (RX)
    for (i = 0; i < ru->nb_rx; i++) free_and_zero(ru->common.rxdataF[i]);
    free_and_zero(ru->common.rxdataF);

    for (i = 0; i < ru->nb_rx; i++) {
      free_and_zero(ru->prach_rxsigF[i]);
#ifdef Rel14
      for (j = 0; j < 4; j++) free_and_zero(ru->prach_rxsigF_br[j][i]);
#endif
    }
    for (j = 0; j < 4; j++) free_and_zero(ru->prach_rxsigF_br[j]);
    free_and_zero(ru->prach_rxsigF);
    /* ru->prach_rxsigF_br is not allocated -> don't free */

    for (i = 0; i < RC.nb_L1_inst; i++) {
      for (p = 0; p < 15; p++) {
	if (p < ru->eNB_list[i]->frame_parms.nb_antenna_ports_eNB || p == 5) {
	  for (j=0; j<ru->nb_tx; j++) free_and_zero(ru->beam_weights[i][p][j]);
	  free_and_zero(ru->beam_weights[i][p]);
	}
      }
    }
  }
  free_and_zero(ru->common.sync_corr);
}
