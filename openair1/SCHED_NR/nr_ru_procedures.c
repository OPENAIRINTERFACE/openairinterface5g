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

/*! \file ru_procedures.c
 * \brief Implementation of RU procedures
 * \author R. Knopp, F. Kaltenberger, N. Nikaein, X. Foukas
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,navid.nikaein@eurecom.fr, x.foukas@sms.ed.ac.uk
 * \note
 * \warning
 */

#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "sched_nr.h"
#include "PHY/MODULATION/modulation_common.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/log.h"
#include "common/utils/system.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>
// RU OFDM Modulator gNodeB

extern openair0_config_t openair0_cfg[MAX_CARDS];

extern int oai_exit;

void nr_feptx0(RU_t *ru,int tti_tx,int first_symbol, int num_symbols) {

  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;

  unsigned int aa,slot_offset,slot_offsetF;
  int slot = tti_tx;


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+(first_symbol!=0?1:0) , 1 );

  slot_offset  = slot*fp->samples_per_slot;
  slot_offsetF = first_symbol*fp->ofdm_symbol_size;

  if (first_symbol>0) slot_offset += (fp->ofdm_symbol_size*first_symbol) + (fp->nb_prefix_samples0) + (fp->nb_prefix_samples*(first_symbol-1));

  LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d (first_symbol %d num_symbols %d)\n",ru->proc.frame_tx, ru->proc.tti_tx,slot,first_symbol,num_symbols);

  for (aa=0; aa<ru->nb_tx; aa++) {
    if (fp->Ncp == 1) {
      PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot_offsetF],
		   (int*)&ru->common.txdata[aa][slot_offset],
		   fp->ofdm_symbol_size,
		   num_symbols,
		   fp->nb_prefix_samples,
		   CYCLIC_PREFIX);
    }
    else {
      if (first_symbol==0) {
	PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot_offsetF],
		     (int*)&ru->common.txdata[aa][slot_offset],
                     fp->ofdm_symbol_size,
                     1,
                     fp->nb_prefix_samples0,
                     CYCLIC_PREFIX);
	PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot_offsetF+fp->ofdm_symbol_size],
		     (int*)&ru->common.txdata[aa][slot_offset+fp->nb_prefix_samples0+fp->ofdm_symbol_size],
                     fp->ofdm_symbol_size,
                     num_symbols-1,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
      }
      else {
	PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot_offsetF],
		     (int*)&ru->common.txdata[aa][slot_offset],
                     fp->ofdm_symbol_size,
                     num_symbols,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
      }
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+(first_symbol!=0?1:0), 0);
}

void nr_feptx_ofdm_2thread(RU_t *ru,int frame_tx,int tti_tx) {

  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  nfapi_nr_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;
  RU_proc_t *proc = &ru->proc;
  struct timespec wait;
  int slot = tti_tx;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  start_meas(&ru->ofdm_mod_stats);

  if (nr_slot_select(cfg,slot) == SF_UL) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

  if (nr_slot_select(cfg,slot)==SF_DL) {
    // If this is not an S-tti
    if (pthread_mutex_timedlock(&proc->mutex_feptx,&wait) != 0) {
      printf("[RU] ERROR pthread_mutex_lock for feptx thread (IC %d)\n", proc->instance_cnt_feptx);
      exit_fun( "error locking mutex_feptx" );
      return;
    }
    
    if (proc->instance_cnt_feptx==0) {
      printf("[RU] FEPtx thread busy\n");
      exit_fun("FEPtx thread busy");
      pthread_mutex_unlock( &proc->mutex_feptx );
      return;
    }
    
    ++proc->instance_cnt_feptx;
    // slot to pass to worker thread
    proc->slot_feptx = slot;
    pthread_mutex_unlock( &proc->mutex_feptx );
  
    
    if (pthread_cond_signal(&proc->cond_feptx) != 0) {
      printf("[RU] ERROR pthread_cond_signal for feptx thread\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }
    
  }

  // call first half-slot in this thread
  nr_feptx0(ru,slot,0,fp->symbols_per_slot>>1);
  wait_on_busy_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"NR feptx thread");

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );

  //write_output

  stop_meas(&ru->ofdm_mod_stats);

}

static void *nr_feptx_thread(void *param) {

  RU_t *ru = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;


  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"NR feptx thread")<0) break;
    int slot=proc->slot_feptx;
    if (release_thread(&proc->mutex_feptx,&proc->instance_cnt_feptx,"NR feptx thread")<0) break;

    nr_feptx0(ru,slot,ru->nr_frame_parms->symbols_per_slot>>1,ru->nr_frame_parms->symbols_per_slot>>1);

    if (pthread_cond_signal(&proc->cond_feptx) != 0) {
      LOG_E(PHY,"[gNB] ERROR pthread_cond_signal for NR feptx thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
  }
  return(NULL);
}

void nr_init_feptx_thread(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_feptx         = -1;
    
  pthread_mutex_init( &proc->mutex_feptx, NULL);
  pthread_cond_init( &proc->cond_feptx, NULL);

  threadCreate(&proc->pthread_feptx, nr_feptx_thread, (void*)ru, "feptx", -1, OAI_PRIORITY_RT);


}

// is this supposed to generate a slot or a subframe???
// seems to be hardcoded to numerology 1 (2 slots=1 subframe)
void nr_feptx_ofdm(RU_t *ru,int frame_tx,int tti_tx) {
     
  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  nfapi_nr_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;

  unsigned int aa=0;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((cfg->subframe_config.dl_cyclic_prefix_type.value == 1) ? 12 : 14);
  int slot = tti_tx;
  int *txdata = &ru->common.txdata[aa][slot*fp->samples_per_slot];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );
  start_meas(&ru->ofdm_mod_stats);

  if ((nr_slot_select(cfg,slot)==SF_DL)||
      ((nr_slot_select(cfg,slot)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

    nr_feptx0(ru,slot,0,fp->symbols_per_slot);

  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );
  stop_meas(&ru->ofdm_mod_stats);

  LOG_D(PHY,"feptx_ofdm (TXPATH): frame %d, slot %d: txp (time %p) %d dB, txp (freq) %d dB\n",
	frame_tx,slot,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_slot)),
	dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));

}



void nr_feptx_prec(RU_t *ru,int frame,int tti_tx) {

  int l,aa;
  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  int32_t ***bw;

  if (ru->nb_tx == 1) {
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 1);

    memcpy((void*)ru->common.txdataF_BF[0],
	   (void*)ru->gNB_list[0]->common_vars.txdataF[0][tti_tx*fp->symbols_per_slot*fp->ofdm_symbol_size],
	   fp->samples_per_slot_wCP*sizeof(int32_t));

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 0);
  }
  else {
    bw  = ru->beam_weights[0];
    for (l=0;l<fp->symbols_per_slot;l++) {
      for (aa=0;aa<ru->nb_tx;aa++) {
	nr_beam_precoding(ru->gNB_list[0]->common_vars.txdataF,
	  		  ru->common.txdataF_BF,
			  fp,
			  bw,
			  tti_tx,
			  l,
			  aa);
	
      }
    }
  }
}

