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

void nr_feptx0(RU_t *ru,int tti_tx,int first_symbol, int num_symbols, int aa) {

  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;

  unsigned int slot_offset,slot_offsetF;
  int slot = tti_tx;


  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+(first_symbol!=0?1:0) , 1 );

  slot_offset  = slot*fp->samples_per_slot;
  slot_offsetF = first_symbol*fp->ofdm_symbol_size;


  if (first_symbol>0) slot_offset += (fp->ofdm_symbol_size*first_symbol) + (fp->nb_prefix_samples0) + (fp->nb_prefix_samples*(first_symbol-1));

  LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d (first_symbol %d num_symbols %d)\n",ru->proc.frame_tx, ru->proc.tti_tx,slot,first_symbol,num_symbols);

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
  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+(first_symbol!=0?1:0), 0);
}

void nr_feptx_ofdm_2thread(RU_t *ru,int frame_tx,int tti_tx) {

  nfapi_nr_config_request_scf_t *cfg = &ru->gNB_list[0]->gNB_config;
  RU_proc_t  *proc  = &ru->proc;
  RU_feptx_t *feptx = proc->feptx;

  PHY_VARS_gNB *gNB;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;
  
  int slot = tti_tx;

  int i    = 0;
  int j    = 0;
  int aa   = 0;
  int ret  = 0;
  int nb_antenna_ports = fp->N_ssb;
  int ofdm_mask_full   = (1<<(ru->nb_tx*2))-1;
  int txdataF_offset   = ((tti_tx%2)*fp->samples_per_slot_wCP);

  if (nr_slot_select(cfg,frame_tx,slot) == NR_UPLINK_SLOT) return;
  for (aa=0; aa<fp->Lmax; aa++) {
    memset(ru->common.txdataF[aa],0,fp->samples_per_slot_wCP*sizeof(int32_t));
  }

  start_meas(&ru->ofdm_total_stats);

  AssertFatal(ru->num_gNB == 1, "ru->num_gNB = %d != 1\n",ru->num_gNB);
  for(j=0; j<fp->symbols_per_slot; ++j){

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC+j , 1);
    start_meas(&ru->txdataF_copy_stats);
    if (ru->num_gNB == 1){
      gNB = ru->gNB_list[0];

      for(i=0; i<nb_antenna_ports; ++i){
        memcpy((void*)&ru->common.txdataF[i][j*fp->ofdm_symbol_size],
           (void*)&gNB->common_vars.txdataF[i][j*fp->ofdm_symbol_size + txdataF_offset],
           fp->ofdm_symbol_size*sizeof(int32_t));
      }
    }//num_gNB == 1
    stop_meas(&ru->txdataF_copy_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC+j , 0);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );


    if (nr_slot_select(cfg,frame_tx,slot)==NR_DOWNLINK_SLOT) {
      // If this is not an S-tti
      for(i=0; i<ru->nb_tx; ++i){
        if(j%2 == 0){
          while(feptx[i].instance_cnt_feptx != -1){
            usleep(5);
          }
          AssertFatal((ret=pthread_mutex_lock(&feptx[i].mutex_feptx))==0,"mutex_lock return %d\n",ret);
          feptx[i].aa                      = i;
          feptx[i].index                   = i;
          feptx[i].ru                      = ru;
          feptx[i].symbol                  = j;
          feptx[i].slot                    = slot;
          feptx[i].nb_antenna_ports        = nb_antenna_ports;
          feptx[i].instance_cnt_feptx      = 0;
          AssertFatal(pthread_cond_signal(&feptx[i].cond_feptx) == 0,"ERROR pthread_cond_signal for feptx_ofdm_thread\n");
          AssertFatal((ret=pthread_mutex_unlock(&feptx[i].mutex_feptx))==0,"mutex_lock returns %d\n",ret);
        }
        else{
          while(feptx[i+ru->nb_tx].instance_cnt_feptx != -1){
            usleep(5);
          }
          AssertFatal((ret=pthread_mutex_lock(&feptx[i+ru->nb_tx].mutex_feptx))==0,"mutex_lock return %d\n",ret);
          feptx[i+ru->nb_tx].aa                      = i;
          feptx[i+ru->nb_tx].index                   = i+ru->nb_tx;
          feptx[i+ru->nb_tx].ru                      = ru;
          feptx[i+ru->nb_tx].symbol                  = j;
          feptx[i+ru->nb_tx].slot                    = slot;
          feptx[i+ru->nb_tx].nb_antenna_ports        = nb_antenna_ports;
          feptx[i+ru->nb_tx].instance_cnt_feptx      = 0;
          AssertFatal(pthread_cond_signal(&feptx[i+ru->nb_tx].cond_feptx) == 0,"ERROR pthread_cond_signal for feptx_ofdm_thread\n");
          AssertFatal((ret=pthread_mutex_unlock(&feptx[i+ru->nb_tx].mutex_feptx))==0,"mutex_lock returns %d\n",ret);
        }
      }
       
    }//if == SF_DL
    else {
      proc->feptx_mask = ofdm_mask_full;
    }
  }//j<fp->symbols_per_slot

  // wait all process to finish
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_feptx))==0,"mutex_lock return %d\n",ret);
  while (proc->feptx_mask != ofdm_mask_full) {
    // most of the time the thread is waiting here
    // proc->instance_cnt_rxtx is -1
    pthread_cond_wait(&proc->cond_feptx,&proc->mutex_feptx); // this unlocks mutex_rxtx while waiting and then locks it again
  }
  proc->feptx_mask = 0;
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_feptx))==0,"mutex_lock return %d\n",ret);

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RU_TX_OFDM_MASK, proc->feptx_mask );
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );

  //write_output

  stop_meas(&ru->ofdm_total_stats);

}

static void *nr_feptx_thread(void *param) {

  RU_feptx_t *feptx = (RU_feptx_t *)param;
  RU_t       *ru;
  int         aa, slot, start, l, nb_antenna_ports, ret;
  int32_t ***bw;
  NR_DL_FRAME_PARMS *fp;
  int ofdm_mask_full;


  while (!oai_exit) {

    ret = 0;
    if (wait_on_condition(&feptx->mutex_feptx,&feptx->cond_feptx,&feptx->instance_cnt_feptx,"NR feptx thread")<0) break;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+feptx->index+1 , 1 );

    ru    = feptx->ru;
    slot  = feptx->slot;
    aa    = feptx->aa;
    l     = feptx->symbol;
    fp    = ru->nr_frame_parms;
    start = feptx->symbol;
    nb_antenna_ports = feptx->nb_antenna_ports;
    ofdm_mask_full   = (1<<(ru->nb_tx*2))-1;


    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 1);
    start_meas(&ru->precoding_stats);
    if (ru->nb_tx == 1) {
      for (int p=0; p<fp->Lmax; p++) {
        if ((fp->L_ssb >> p) & 0x01){
          memcpy((void*)&ru->common.txdataF_BF[0][l*fp->ofdm_symbol_size],
                 (void*)&ru->common.txdataF[p][l*fp->ofdm_symbol_size],
                 fp->ofdm_symbol_size*sizeof(int32_t));
        }
      }
    }
    else {
      bw  = ru->beam_weights[0];
      nr_beam_precoding(ru->common.txdataF,
                        ru->common.txdataF_BF,
                        fp,
                        bw,
                        slot,
                        l,
                        aa,
                        nb_antenna_ports);
    }
    stop_meas(&ru->precoding_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 0);


    start_meas(&ru->ofdm_mod_stats);
    nr_feptx0(ru,slot,start,1,aa);
    stop_meas(&ru->ofdm_mod_stats);

    if (release_thread(&feptx->mutex_feptx,&feptx->instance_cnt_feptx,"NR feptx thread")<0) break;

    if(l >= fp->symbols_per_slot -2){
      AssertFatal((ret=pthread_mutex_lock(&ru->proc.mutex_feptx))==0,"mutex_lock return %d\n",ret);
      ru->proc.feptx_mask |= 1<<(feptx->index);
      if(ru->proc.feptx_mask == ofdm_mask_full)
        AssertFatal(pthread_cond_signal(&ru->proc.cond_feptx) == 0,"ERROR pthread_cond_signal for precoding and ofdm finish\n");
      AssertFatal((ret=pthread_mutex_unlock(&ru->proc.mutex_feptx))==0,"mutex_lock returns %d\n",ret);
    }
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RU_TX_OFDM_MASK, ru->proc.feptx_mask );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+feptx->index+1 , 0 );
  }
  return(NULL);
}


// is this supposed to generate a slot or a subframe???
// seems to be hardcoded to numerology 1 (2 slots=1 subframe)
void nr_feptx_ofdm(RU_t *ru,int frame_tx,int tti_tx) {
     
  nfapi_nr_config_request_scf_t *cfg = &ru->gNB_list[0]->gNB_config;
  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  int cyclic_prefix_type = NFAPI_CP_NORMAL;

  unsigned int aa=0;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((cyclic_prefix_type == 1) ? 12 : 14);
  int slot = tti_tx;
  int *txdata = &ru->common.txdata[aa][slot*fp->samples_per_slot];

  if (nr_slot_select(fp,frame_tx,slot) == NR_UPLINK_SLOT) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );
  start_meas(&ru->ofdm_mod_stats);


    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

  nr_feptx0(ru,slot,0,NR_NUMBER_OF_SYMBOLS_PER_SLOT,aa);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );
  stop_meas(&ru->ofdm_mod_stats);

  LOG_D(PHY,"feptx_ofdm (TXPATH): frame %d, slot %d: txp (time %p) %d dB, txp (freq) %d dB\n",
	frame_tx,slot,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_slot)),
	dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));

}

void nr_init_feptx_thread(RU_t *ru) {

  RU_proc_t  *proc  = &ru->proc;
  RU_feptx_t *feptx = proc->feptx;
  int i = 0;

  for(i=0; i<16; i++){
    feptx[i].instance_cnt_feptx         = -1;
    
    pthread_mutex_init( &feptx[i].mutex_feptx, NULL);
    pthread_cond_init( &feptx[i].cond_feptx, NULL);

    threadCreate(&feptx[i].pthread_feptx, nr_feptx_thread, (void*)&feptx[i], "feptx", -1, OAI_PRIORITY_RT);
    LOG_I(PHY,"init feptx thread %d\n", i);
  }

}


void nr_feptx_prec(RU_t *ru,int frame_tx,int tti_tx) {

  int l,aa;
  PHY_VARS_gNB **gNB_list = ru->gNB_list,*gNB;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;
  int32_t ***bw;
  int i=0;
  int slot_tx = tti_tx;

  start_meas(&ru->precoding_stats);
  if (ru->num_gNB == 1){
    gNB = gNB_list[0];

    if (nr_slot_select(fp,frame_tx,slot_tx) == NR_UPLINK_SLOT) return;

    for(i=0; i<fp->Lmax; ++i)
      memcpy((void*)ru->common.txdataF[i],
           (void*)gNB->common_vars.txdataF[i],
           fp->samples_per_slot_wCP*sizeof(int32_t));

    if (ru->nb_tx == 1) {
    
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 1);

      //      AssertFatal(fp->N_ssb==ru->nb_tx,"Attempting to transmit %d SSB while Nb_tx = %d",fp->N_ssb,ru->nb_tx);

      for (int p=0; p<fp->Lmax; p++) {
        if ((fp->L_ssb >> p) & 0x01){
          memcpy((void*)ru->common.txdataF_BF[0],
                 (void*)ru->common.txdataF[p],
                 fp->samples_per_slot_wCP*sizeof(int32_t));
        }
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 0);
    }// if (ru->nb_tx == 1)
    else {
      bw  = ru->beam_weights[0];
      for (l=0;l<fp->symbols_per_slot;l++) {
        for (aa=0;aa<ru->nb_tx;aa++) {
          nr_beam_precoding(ru->common.txdataF,
                            ru->common.txdataF_BF,
                            fp,
                            bw,
                            tti_tx,
                            l,
                            aa,
                            fp->Lmax);
        }// for (aa=0;aa<ru->nb_tx;aa++)
      }// for (l=0;l<fp->symbols_per_slot;l++)
    }// if (ru->nb_tx == 1)
  }// if (ru->num_gNB == 1)
  stop_meas(&ru->precoding_stats);
}

void nr_fep0(RU_t *ru, int first_half) {
  
  uint8_t start_symbol, end_symbol, l, aa;
  RU_proc_t *proc       = &ru->proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  
  if(first_half == 1) {
    start_symbol = 0;
    end_symbol = NR_SYMBOLS_PER_SLOT/2;
  } else {
    start_symbol = NR_SYMBOLS_PER_SLOT/2;
    end_symbol = NR_SYMBOLS_PER_SLOT;
  }

  LOG_D(PHY,"In fep0 for slot = %d, first_half = %d, start_symbol = %d, end_symbol = %d\n", proc->tti_rx, first_half, start_symbol, end_symbol);
  //  printf("fep0: slot %d\n",slot);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+proc->tti_rx, 1);

  // remove_7_5_kHz(ru,(slot&1)+(proc->tti_rx<<1));
  for (l = start_symbol; l < end_symbol; l++) {
    for (aa = 0; aa < fp->nb_antennas_rx; aa++) {
      nr_slot_fep_ul(fp,
                     ru->common.rxdata[aa],
                     ru->common.rxdataF[aa],
                     l,
                     proc->tti_rx,
                     0,
                     0);
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+proc->tti_rx, 0);
}


static void *nr_feprx_thread(void *param) {

  RU_t *ru = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;


  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"NR feprx thread")<0) break;

    if (oai_exit) break;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX1, 1 );

    nr_fep0(ru, 0);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX1, 0 );

    if (release_thread(&proc->mutex_fep,&proc->instance_cnt_fep,"nr_feprx thread")<0) break;

    if (pthread_cond_signal(&proc->cond_fep) != 0) {
      printf("[gNB] ERROR pthread_cond_signal for nr_feprx thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
  }

  return NULL;
}


void nr_init_feprx_thread(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_fep = -1;
    
  pthread_mutex_init( &proc->mutex_fep, NULL);
  pthread_cond_init( &proc->cond_fep, NULL);

  threadCreate(&proc->pthread_fep, nr_feprx_thread, (void*)ru, "feprx", -1, OAI_PRIORITY_RT);
}


void nr_fep_full_2thread(RU_t *ru, int slot) {

  RU_proc_t *proc = &ru->proc;

  struct timespec wait;

  // NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;

  // if ((fp->frame_type == TDD) &&
     // (subframe_select(fp,proc->tti_rx) != NR_UPLINK_SLOT)) return;

  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );
  start_meas(&ru->ofdm_demod_stats);

  if (pthread_mutex_timedlock(&proc->mutex_fep,&wait) != 0) {
    printf("[RU] ERROR pthread_mutex_lock for nr_feprx thread (IC %d)\n", proc->instance_cnt_fep);
    exit_fun( "error locking mutex_fep" );
    return;
  }

  if (proc->instance_cnt_fep==0) {
    printf("[RU] FEP thread busy\n");
    exit_fun("FEP thread busy");
    pthread_mutex_unlock( &proc->mutex_fep );
    return;
  }
  
  ++proc->instance_cnt_fep;


  if (pthread_cond_signal(&proc->cond_fep) != 0) {
    printf("[RU] ERROR pthread_cond_signal for nr_feprx thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return;
  }
  //start_meas(&ru->ofdm_demod_wakeup_stats);
  
  pthread_mutex_unlock( &proc->mutex_fep );

  // call second slot in this symbol
  nr_fep0(ru, 1);

  start_meas(&ru->ofdm_demod_wait_stats);
  wait_on_busy_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"nr_feprx thread");  
  stop_meas(&ru->ofdm_demod_wait_stats);
  if(opp_enabled == 1 && ru->ofdm_demod_wakeup_stats.p_time>30*3000){
    print_meas_now(&ru->ofdm_demod_wakeup_stats,"fep wakeup",stderr);
    printf("delay in fep wait on codition in frame_rx: %d  subframe_rx: %d \n",proc->frame_rx,proc->tti_rx);
  }

  stop_meas(&ru->ofdm_demod_stats);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 0 );
}



void nr_fep_full(RU_t *ru, int slot) {

  RU_proc_t *proc = &ru->proc;
  int l, aa;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;

  // if ((fp->frame_type == TDD) && 
     // (subframe_select(fp,proc->tti_rx) != NR_UPLINK_SLOT)) return;

  start_meas(&ru->ofdm_demod_stats);
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );

  // remove_7_5_kHz(ru,proc->tti_rx<<1);
  // remove_7_5_kHz(ru,1+(proc->tti_rx<<1));

  for (l = 0; l < fp->symbols_per_slot; l++) {
    for (aa = 0; aa < fp->nb_antennas_rx; aa++) {
      nr_slot_fep_ul(fp,
                     ru->common.rxdata[aa],
                     ru->common.rxdataF[aa],
                     l,
                     proc->tti_rx,
                     0,
                     0);
    }
  }

  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 0 );
  stop_meas(&ru->ofdm_demod_stats);
  
  
}
