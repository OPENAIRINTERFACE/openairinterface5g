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

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#include "targets/RT/USER/rt_wrapper.h"

// RU OFDM Modulator gNodeB

extern openair0_config_t openair0_cfg[MAX_CARDS];

extern int oai_exit;

void nr_feptx0(RU_t *ru,int slot) {

  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;

  unsigned int aa,slot_offset;
  int slot_sizeF = fp->ofdm_symbol_size * fp->symbols_per_slot;
  int subframe = ru->proc.subframe_tx;


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 1 );

  slot_offset = subframe*fp->samples_per_subframe + (slot*(fp->samples_per_subframe / fp->slots_per_subframe));

  LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d\n",ru->proc.frame_tx, ru->proc.subframe_tx,slot);

  for (aa=0; aa<ru->nb_tx; aa++) {
     nr_normal_prefix_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
                       (int*)&ru->common.txdata[aa][slot_offset],
                       fp->symbols_per_slot,
                       fp);

  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 0);
}

void nr_feptx_ofdm_2thread(RU_t *ru) {

  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  nfapi_nr_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;
  RU_proc_t *proc = &ru->proc;
  struct timespec wait;
  int subframe = ru->proc.subframe_tx;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  start_meas(&ru->ofdm_mod_stats);

  if (nr_subframe_select(cfg,subframe) == SF_UL) return;

  // this copy should be done in the precoding thread (currently inactive)
  for (int aa=0;aa<ru->nb_tx;aa++)
  memcpy((void*)ru->common.txdataF_BF[aa],
   (void*)&ru->gNB_list[0]->common_vars.txdataF[aa][subframe*fp->samples_per_subframe_wCP],
   fp->samples_per_subframe_wCP*sizeof(int32_t));

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

  if (nr_subframe_select(cfg,subframe)==SF_DL) {
    // If this is not an S-subframe
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
    
    
    if (pthread_cond_signal(&proc->cond_feptx) != 0) {
      printf("[RU] ERROR pthread_cond_signal for feptx thread\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }
    
    pthread_mutex_unlock( &proc->mutex_feptx );
  }

  // call first slot in this thread
  nr_feptx0(ru,0);
  wait_on_busy_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"NR feptx thread");

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );

  //write_output

  stop_meas(&ru->ofdm_mod_stats);

}

static void *nr_feptx_thread(void *param) {

  RU_t *ru = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;

  thread_top_init("nr_feptx_thread",0,870000,1000000,1000000);

  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"NR feptx thread")<0) break;
    nr_feptx0(ru,1);
    if (release_thread(&proc->mutex_feptx,&proc->instance_cnt_feptx,"NR feptx thread")<0) break;

    if (pthread_cond_signal(&proc->cond_feptx) != 0) {
      printf("[gNB] ERROR pthread_cond_signal for NR feptx thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
  }
  return(NULL);
}

void nr_init_feptx_thread(RU_t *ru,pthread_attr_t *attr_feptx) {

  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_feptx         = -1;
    
  pthread_mutex_init( &proc->mutex_feptx, NULL);
  pthread_cond_init( &proc->cond_feptx, NULL);

  pthread_create(&proc->pthread_feptx, attr_feptx, nr_feptx_thread, (void*)ru);


}


void nr_feptx_ofdm(RU_t *ru) {
     
  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  nfapi_nr_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;

  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i,j, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((cfg->subframe_config.dl_cyclic_prefix_type.value == 1) ? 12 : 14);
  int len,len2;
  int16_t *txdata;
  int subframe = ru->proc.subframe_tx;

//  int CC_id = ru->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

  slot_offset_F = 0;

  slot_offset = subframe*fp->samples_per_subframe;

  if ((nr_subframe_select(cfg,subframe)==SF_DL)||
      ((nr_subframe_select(cfg,subframe)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

    start_meas(&ru->ofdm_mod_stats);

    for (aa=0; aa<ru->nb_tx; aa++) {
      if (cfg->subframe_config.dl_cyclic_prefix_type.value == 1) {
        PHY_ofdm_mod(&ru->common.txdataF_BF[aa][0],
                     dummy_tx_b,
                     fp->ofdm_symbol_size,
                     12,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
        PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot_sizeF],
                     dummy_tx_b+(fp->samples_per_subframe / fp->slots_per_subframe),
                     fp->ofdm_symbol_size,
                     12,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
      } else {
        nr_normal_prefix_mod(&ru->common.txdataF_BF[aa][slot_offset_F],
                          dummy_tx_b,
                          14,
                          fp);
	// if S-subframe generate first slot only
	if (nr_subframe_select(cfg,subframe) == SF_DL) 
	  nr_normal_prefix_mod(&ru->common.txdataF_BF[aa][slot_offset_F+slot_sizeF],
			    dummy_tx_b+(fp->samples_per_subframe / fp->slots_per_subframe),
			    14,
			    fp);
      }

      // if S-subframe generate first slot only
      if (nr_subframe_select(cfg,subframe) == SF_S)
	len = fp->samples_per_subframe / fp->slots_per_subframe;
      else
	len = fp->samples_per_subframe;
      
      if (slot_offset<0) {
	txdata = (int16_t*)&ru->common.txdata[aa][(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_subframe)+tx_offset];
        len2 = -(slot_offset);
	len2 = (len2>len) ? len : len2;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i];
	}
	if (len2<len) {
	  txdata = (int16_t*)&ru->common.txdata[aa][0];
	  for (j=0; i<(len<<1); i++,j++) {
	    txdata[j++] = ((int16_t*)dummy_tx_b)[i];
	  }
	}
      }  
      else if ((slot_offset+len)>(fp->samples_per_frame)) {
	tx_offset = (int)slot_offset;
	txdata = (int16_t*)&ru->common.txdata[aa][tx_offset];
	len2 = -tx_offset + fp->samples_per_frame;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i];
	}
	txdata = (int16_t*)&ru->common.txdata[aa][0];
	for (j=0; i<(len<<1); i++,j++) {
          txdata[j++] = ((int16_t*)dummy_tx_b)[i];
	}
      }
      else {
	//LOG_D(PHY,"feptx_ofdm: Writing to position %d\n",slot_offset);
	tx_offset = (int)slot_offset;
	txdata = (int16_t*)&ru->common.txdata[aa][tx_offset];

	for (i=0; i<(len<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i];
	}
      }
      
     // if S-subframe switch to RX in second subframe
      /*
     if (subframe_select(fp,subframe) == SF_S) {
       for (i=0; i<len; i++) {
	 ru->common_vars.txdata[0][aa][tx_offset++] = 0x00010001;
       }
     }
      */
     /*if ((fp->frame_type == TDD) && 
         ((fp->tdd_config==0) ||
	   (fp->tdd_config==1) ||
	   (fp->tdd_config==2) ||
	   (fp->tdd_config==6)) && 
	     ((subframe==0) || (subframe==5))) {
       // turn on tx switch N_TA_offset before
       //LOG_D(HW,"subframe %d, time to switch to tx (N_TA_offset %d, slot_offset %d) \n",subframe,ru->N_TA_offset,slot_offset);
       for (i=0; i<ru->N_TA_offset; i++) {
         tx_offset = (int)slot_offset+i-ru->N_TA_offset/2;
         if (tx_offset<0)
           tx_offset += fp->samples_per_frame;
	 
         if (tx_offset>=(fp->samples_per_frame))
           tx_offset -= fp->samples_per_frame;
	 
         ru->common.txdata[aa][tx_offset] = 0x00000000;
       }
     }*/
     stop_meas(&ru->ofdm_mod_stats);
     LOG_D(PHY,"feptx_ofdm (TXPATH): frame %d, subframe %d: txp (time %p) %d dB, txp (freq) %d dB\n",
	   ru->proc.frame_tx,subframe,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_subframe)),
	   dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );
}
