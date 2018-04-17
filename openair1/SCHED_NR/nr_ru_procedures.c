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

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/defs.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#include "targets/RT/USER/rt_wrapper.h"

// RU OFDM Modulator gNodeB

extern openair0_config_t openair0_cfg[MAX_CARDS];

extern int oai_exit;

void nr_feptx0(RU_t *ru,int slot) {

  NR_DL_FRAME_PARMS *fp = &ru->nr_frame_parms;
  nfapi_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;

  unsigned int aa,slot_offset;
  int i, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((cfg->subframe_config.dl_cyclic_prefix_type.value == 1) ? 12 : 14);
  int subframe = ru->proc.subframe_tx;


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 1 );

  slot_offset = subframe*fp->samples_per_subframe + (slot*(fp->samples_per_subframe / fp->slots_per_subframe));

  LOG_I(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d\n",ru->proc.frame_tx, ru->proc.subframe_tx,slot);

  for (aa=0; aa<ru->nb_tx; aa++) {
    if (cfg->subframe_config.dl_cyclic_prefix_type.value == 1) PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
					                      (int*)&ru->common.txdata[aa][slot_offset],
					                      fp->ofdm_symbol_size,
					                      12,
					                      fp->nb_prefix_samples,
					                      CYCLIC_PREFIX);
    else                     normal_prefix_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
					                           (int*)&ru->common.txdata[aa][slot_offset],
					                           14,
					                           fp);
    
   /* 
    len = fp->samples_per_subframe / fp->slots_per_subframe;

    
    if ((slot_offset+len)>(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_subframe)) {
      tx_offset = (int)slot_offset;
      txdata = (int16_t*)&ru->common.txdata[aa][tx_offset];
      len2 = -tx_offset+LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
      for (i=0; i<(len2<<1); i++) {
	txdata[i] = ((int16_t*)dummy_tx_b)[i];
      }
      txdata = (int16_t*)&ru->common.txdata[aa][0];
      for (j=0; i<(len<<1); i++,j++) {
	txdata[j++] = ((int16_t*)dummy_tx_b)[i];
      }
    }
    else {
      tx_offset = (int)slot_offset;
      txdata = (int16_t*)&ru->common.txdata[aa][tx_offset];
      memcpy((void*)txdata,(void*)dummy_tx_b,len<<2);   
    }
*/
    // TDD: turn on tx switch N_TA_offset before by setting buffer in these samples to 0    
    /*if ((slot == 0) &&
        (fp->frame_type == TDD) && 
        ((fp->tdd_config==0) ||
         (fp->tdd_config==1) ||
         (fp->tdd_config==2) ||
         (fp->tdd_config==6)) &&  
        ((subframe==0) || (subframe==5))) {
      for (i=0; i<ru->N_TA_offset; i++) {
	tx_offset = (int)slot_offset+i-ru->N_TA_offset/2;
	if (tx_offset<0)
	  tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	
	if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti))
	  tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	
	ru->common.txdata[aa][tx_offset] = 0x00000000;
      }
    }*/
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 0);
}

void nr_feptx_ofdm_2thread(RU_t *ru) {

  NR_DL_FRAME_PARMS *fp=&ru->nr_frame_parms;
  RU_proc_t *proc = &ru->proc;
  struct timespec wait;
  int subframe = ru->proc.subframe_tx;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  start_meas(&ru->ofdm_mod_stats);

  if (nr_subframe_select(fp,subframe) == SF_UL) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

  if (nr_subframe_select(fp,subframe)==SF_DL) {
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
  wait_on_busy_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"feptx thread");  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );

  stop_meas(&ru->ofdm_mod_stats);

}


void nr_feptx_ofdm(RU_t *ru) {
     
  NR_DL_FRAME_PARMS *fp=&ru->nr_frame_parms;
  nfapi_config_request_t *cfg = &ru->gNB_list[0]->gNB_config;

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

  if ((nr_subframe_select(fp,subframe)==SF_DL)||
      ((nr_subframe_select(fp,subframe)==SF_S))) {
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
        normal_prefix_mod(&ru->common.txdataF_BF[aa][slot_offset_F],
                          dummy_tx_b,
                          14,
                          fp);
	// if S-subframe generate first slot only
	if (subframe_select(fp,subframe) == SF_DL) 
	  nr_normal_prefix_mod(&ru->common.txdataF_BF[aa][slot_offset_F+slot_sizeF],
			    dummy_tx_b+(fp->samples_per_subframe / fp->slots_per_subframe),
			    14,
			    fp);
      }

      // if S-subframe generate first slot only
      if (subframe_select(fp,subframe) == SF_S)
	len = fp->samples_per_subframe / fp->slots_per_subframe;
      else
	len = fp->samples_per_subframe;
      /*
      for (i=0;i<len;i+=4) {
	dummy_tx_b[i] = 0x100;
	dummy_tx_b[i+1] = 0x01000000;
	dummy_tx_b[i+2] = 0xff00;
	dummy_tx_b[i+3] = 0xff000000;
	}*/
      
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
