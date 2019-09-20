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

  nfapi_nr_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;
  RU_proc_t  *proc  = &ru->proc;
  RU_feptx_t *feptx = proc->feptx;

  PHY_VARS_gNB *gNB;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;

  int slot = tti_tx;
  int i    = 0;
  int j    = 0;
  int ret  = 0;
  int nb_antenna_ports = 8;
  int ofdm_mask_full   = (1<<(ru->nb_tx*2))-1;

  start_meas(&ru->ofdm_total_stats);

  for(j=0; j<fp->symbols_per_slot; ++j){

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 1);
    start_meas(&ru->txdataF_copy_stats);
    if (ru->num_gNB == 1){
      gNB = ru->gNB_list[0];
      cfg = &gNB->gNB_config;
      if (nr_slot_select(cfg,tti_tx) == SF_UL) return;

      for(i=0; i<nb_antenna_ports; ++i){
        memcpy((void*)&ru->common.txdataF[i][j],
           (void*)&gNB->common_vars.txdataF[i][j],
           fp->ofdm_symbol_size*sizeof(int32_t));
      }
    }//num_gNB == 1
    //printf("~~~~~~~~~~~memery copy index: nb_antenna_ports = %d, samples_per_slot_wCP = %d\n", nb_antenna_ports, fp->samples_per_slot_wCP);
    stop_meas(&ru->txdataF_copy_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 0);


    if (nr_slot_select(cfg,slot) == SF_UL) return;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

    if (nr_slot_select(cfg,slot)==SF_DL) {
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
          //printf("~~~~~~~~~~~~waking up thread %d with physical antenna %d, slot %d, symbol %d, total logical antenna port %d \n", feptx[i].index, feptx[i].aa, feptx[i].slot, feptx[i].symbol, feptx[i].nb_antenna_ports);
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

    bw  = ru->beam_weights[0];
    start_meas(&ru->precoding_stats);
    nr_beam_precoding(ru->common.txdataF,
                        ru->common.txdataF_BF,
                        fp,
                        bw,
                        slot,
                        l+start,
                        aa,
                        nb_antenna_ports);
    stop_meas(&ru->precoding_stats);

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

void nr_init_feptx_thread(RU_t *ru) {

  RU_proc_t  *proc  = &ru->proc;
  RU_feptx_t *feptx = proc->feptx;
  int i = 0;

  for(i=0; i<16; i++){
    feptx[i].instance_cnt_feptx         = -1;
    
    pthread_mutex_init( &feptx[i].mutex_feptx, NULL);
    pthread_cond_init( &feptx[i].cond_feptx, NULL);

    threadCreate(&feptx[i].pthread_feptx, nr_feptx_thread, (void*)&feptx[i], "feptx", -1, OAI_PRIORITY_RT);
  }

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

    nr_feptx0(ru,slot,0,fp->symbols_per_slot,aa);

  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );
  stop_meas(&ru->ofdm_mod_stats);

  LOG_D(PHY,"feptx_ofdm (TXPATH): frame %d, slot %d: txp (time %p) %d dB, txp (freq) %d dB\n",
	frame_tx,slot,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_slot)),
	dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));

}


static void *nr_feptx_prec_thread(void *param) {

  RU_prec_t *prec                = (RU_prec_t *) param;
  RU_t *ru;
  NR_DL_FRAME_PARMS *fp;
  int symbol;
  int p;
  int aa;
  int32_t *bw;
  int32_t **txdataF;
  int32_t **txdataF_BF;

  while(!oai_exit)
  {
    if (wait_on_condition(&prec->mutex_feptx_prec,&prec->cond_feptx_prec,&prec->instance_cnt_feptx_prec,"NR feptx prec thread")<0) break;
    ru                   = prec->ru;
    symbol               = prec->symbol;
    p                    = prec->p;
    aa                   = prec->aa;
    fp                   = ru->nr_frame_parms;
    bw                   = ru->beam_weights[0][p][aa];
    txdataF              = ru->common.txdataF;
    txdataF_BF           = ru->common.txdataF_BF;
    multadd_cpx_vector((int16_t*)&txdataF[p][symbol*fp->ofdm_symbol_size],
			 (int16_t*)bw, 
			 (int16_t*)&txdataF_BF[aa][symbol*fp->ofdm_symbol_size], 
			 0, 
			 fp->ofdm_symbol_size, 
			 15);

    if (release_thread(&prec->mutex_feptx_prec,&prec->instance_cnt_feptx_prec,"NR feptx thread")<0) break;
  }
  return 0;
}

void nr_feptx_prec_control(RU_t *ru,int frame,int tti_tx) {

  int ret    = 0;
  int i      = 0;
  int symbol = 0;
  int p      = 0;
  int aa     = 0;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;
  int nb_antenna_ports    = fp->Lmax; // for now logical antenna ports corresponds to SSB
  RU_prec_t *prec         = ru->proc.prec;
  PHY_VARS_gNB **gNB_list = ru->gNB_list,*gNB;

  gNB = gNB_list[0];

  start_meas(&ru->precoding_stats);
  for(i=0; i<nb_antenna_ports; ++i)
    memcpy((void*)ru->common.txdataF[i],
           (void*)gNB->common_vars.txdataF[i],
           fp->samples_per_slot_wCP*sizeof(int32_t));

  for(symbol = 0; symbol < fp->symbols_per_slot; ++symbol){
    for(p=0; p<nb_antenna_ports; p++){
      for(aa=0;aa<ru->nb_tx;aa++){
        if ((fp->L_ssb >> p) & 0x01){
          while(1){
            if(prec[i].instance_cnt_feptx_prec == -1){
              AssertFatal((ret=pthread_mutex_lock(&prec[i].mutex_feptx_prec))==0,"mutex_lock return %d\n",ret);
              prec[i].instance_cnt_feptx_prec = 0;
              prec[i].symbol                  = symbol;
              prec[i].p                       = p;
              prec[i].aa                      = aa;
              prec[i].index                   = i;
              prec[i].ru                      = ru;
              AssertFatal(pthread_cond_signal(&prec[i].cond_feptx_prec) == 0,"ERROR pthread_cond_signal for gNB_L1_thread\n");
              AssertFatal((ret=pthread_mutex_unlock(&prec[i].mutex_feptx_prec))==0,"mutex_lock returns %d\n",ret);
              i = (i+1) % 16;
              break;
            }
            i = (i+1) % 16;
          }
        }//(frame_params->Lssb >> p) & 0x01
      }//aa
    }//p
  }//symbol
  
  i = 0;
  while(1){
    if(prec[i].instance_cnt_feptx_prec == -1) ++i;
    if(i == 16) break;
  }

  stop_meas(&ru->precoding_stats);
}

void nr_feptx_prec(RU_t *ru,int frame,int tti_tx) {

  int l,aa;
  PHY_VARS_gNB **gNB_list = ru->gNB_list,*gNB;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;
  nfapi_nr_config_request_t *cfg;
  int32_t ***bw;
  int i=0;

  start_meas(&ru->precoding_stats);
  if (ru->num_gNB == 1){
    gNB = gNB_list[0];
    cfg = &gNB->gNB_config;
    if (nr_slot_select(cfg,tti_tx) == SF_UL) return;

    for(i=0; i<fp->Lmax; ++i)
      memcpy((void*)ru->common.txdataF[i],
           (void*)gNB->common_vars.txdataF[i],
           fp->samples_per_slot_wCP*sizeof(int32_t));

    if (ru->nb_tx == 1) {
    
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 1);

      AssertFatal(fp->N_ssb==ru->nb_tx,"Attempting to transmit %d SSB while Nb_tx = %d",fp->N_ssb,ru->nb_tx);

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

void nr_init_feptx_prec_thread(RU_t *ru){

  RU_proc_t *proc = &ru->proc;
  RU_prec_t *prec = proc->prec;
  int i=0;

  for(i=0; i<16; ++i){
    prec[i].instance_cnt_feptx_prec         = -1;
    pthread_mutex_init( &prec[i].mutex_feptx_prec, NULL);
    pthread_cond_init( &prec[i].cond_feptx_prec, NULL);

    threadCreate(&prec[i].pthread_feptx_prec, nr_feptx_prec_thread, (void*)&prec[i], "nr_feptx_prec", -1, OAI_PRIORITY_RT);
  }
}

