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

#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "SCHED/sched_eNB.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"


#include "assertions.h"
#include "msc.h"

#include <time.h>

#include "targets/RT/USER/rt_wrapper.h"

extern int oai_exit;


void feptx0(RU_t *ru,int slot) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  //int dummy_tx_b[7680*2] __attribute__((aligned(32)));

  unsigned int aa,slot_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((fp->Ncp==1) ? 6 : 7);
  int subframe = ru->proc.subframe_tx;


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 1 );

  slot_offset = subframe*fp->samples_per_tti + (slot*(fp->samples_per_tti>>1));

  //LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d\n",ru->proc.frame_tx, ru->proc.subframe_tx,slot);

  for (aa=0; aa<ru->nb_tx; aa++) {
    if (fp->Ncp == EXTENDED) PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
					                      (int*)&ru->common.txdata[aa][slot_offset],
					                      fp->ofdm_symbol_size,
					                      6,
					                      fp->nb_prefix_samples,
					                      CYCLIC_PREFIX);
    else                     normal_prefix_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
					                           (int*)&ru->common.txdata[aa][slot_offset],
					                           7,
					                           fp);
    
   /* 
    len = fp->samples_per_tti>>1;

    
    if ((slot_offset+len)>(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)) {
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
/*    if ((slot == 0) &&
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

static void *feptx_thread(void *param) {

  RU_t *ru = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  
  thread_top_init("feptx_thread",1,85000,120000,500000);
  pthread_setname_np( pthread_self(),"feptx processing");
  LOG_I(PHY,"thread feptx created id=%ld\n", syscall(__NR_gettid));
  //CPU_SET(6, &cpuset);
  //pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  //wait_sync("feptx_thread");

  

  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"feptx thread")<0) break;  
    if (oai_exit) break;
    //stop_meas(&ru->ofdm_mod_wakeup_stats);
    feptx0(ru,1);
    if (release_thread(&proc->mutex_feptx,&proc->instance_cnt_feptx,"feptx thread")<0) break;

    if (pthread_cond_signal(&proc->cond_feptx) != 0) {
      printf("[eNB] ERROR pthread_cond_signal for feptx thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
	/*if(opp_enabled == 1 && ru->ofdm_mod_wakeup_stats.p_time>30*3000){
      print_meas_now(&ru->ofdm_mod_wakeup_stats,"fep wakeup",stderr);
      printf("delay in fep wakeup in frame_tx: %d  subframe_rx: %d \n",proc->frame_tx,proc->subframe_tx);
    }*/
  }



  return(NULL);
}

void feptx_ofdm_2thread(RU_t *ru) {

  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;
  RU_proc_t *proc = &ru->proc;
  struct timespec wait;
  int subframe = ru->proc.subframe_tx;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );
  start_meas(&ru->ofdm_mod_stats);

  if (subframe_select(fp,subframe) == SF_UL) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

  if (subframe_select(fp,subframe)==SF_DL) {
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
	//start_meas(&ru->ofdm_mod_wakeup_stats);
    
    pthread_mutex_unlock( &proc->mutex_feptx );
  }

  // call first slot in this thread
  feptx0(ru,0);
  start_meas(&ru->ofdm_mod_wait_stats);
  wait_on_busy_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"feptx thread");  
  stop_meas(&ru->ofdm_mod_wait_stats);
  /*if(opp_enabled == 1 && ru->ofdm_mod_wait_stats.p_time>30*3000){
    print_meas_now(&ru->ofdm_mod_wait_stats,"fep wakeup",stderr);
    printf("delay in feptx wait on codition in frame_rx: %d  subframe_rx: %d \n",proc->frame_tx,proc->subframe_tx);
  }*/

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );

  stop_meas(&ru->ofdm_mod_stats);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );

}

void feptx_ofdm(RU_t *ru) {
     
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i,j, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((fp->Ncp==1) ? 6 : 7);
  int len,len2;
  int16_t *txdata;
  int subframe = ru->proc.subframe_tx;

//  int CC_id = ru->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );
  slot_offset_F = 0;

  slot_offset = subframe*fp->samples_per_tti;

  if ((subframe_select(fp,subframe)==SF_DL)||
      ((subframe_select(fp,subframe)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

    start_meas(&ru->ofdm_mod_stats);

    for (aa=0; aa<ru->nb_tx; aa++) {
      if (fp->Ncp == EXTENDED) {
        PHY_ofdm_mod(&ru->common.txdataF_BF[aa][0],
                     dummy_tx_b,
                     fp->ofdm_symbol_size,
                     6,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
        PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot_sizeF],
                     dummy_tx_b+(fp->samples_per_tti>>1),
                     fp->ofdm_symbol_size,
                     6,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
      } else {
        normal_prefix_mod(&ru->common.txdataF_BF[aa][slot_offset_F],
                          dummy_tx_b,
                          7,
                          fp);
	// if S-subframe generate first slot only
	if (subframe_select(fp,subframe) == SF_DL) 
	  normal_prefix_mod(&ru->common.txdataF_BF[aa][slot_offset_F+slot_sizeF],
			    dummy_tx_b+(fp->samples_per_tti>>1),
			    7,
			    fp);
      }

      // if S-subframe generate first slot only
      if (subframe_select(fp,subframe) == SF_S)
	len = fp->samples_per_tti>>1;
      else
	len = fp->samples_per_tti;
      /*
      for (i=0;i<len;i+=4) {
	dummy_tx_b[i] = 0x100;
	dummy_tx_b[i+1] = 0x01000000;
	dummy_tx_b[i+2] = 0xff00;
	dummy_tx_b[i+3] = 0xff000000;
	}*/
      
      if (slot_offset<0) {
	txdata = (int16_t*)&ru->common.txdata[aa][(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)+tx_offset];
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
      else if ((slot_offset+len)>(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)) {
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

//     if ((fp->frame_type == TDD) &&
//         ((fp->tdd_config==0) ||
//	   (fp->tdd_config==1) ||
//	   (fp->tdd_config==2) ||
//	   (fp->tdd_config==6)) &&
//	     ((subframe==0) || (subframe==5))) {
//       // turn on tx switch N_TA_offset before
//       //LOG_D(HW,"subframe %d, time to switch to tx (N_TA_offset %d, slot_offset %d) \n",subframe,ru->N_TA_offset,slot_offset);
//       for (i=0; i<ru->N_TA_offset; i++) {
//         tx_offset = (int)slot_offset+i-ru->N_TA_offset/2;
//         if (tx_offset<0)
//           tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
//
//         if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti))
//           tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
//
//         ru->common.txdata[aa][tx_offset] = 0x00000000;
//       }
//     }

     stop_meas(&ru->ofdm_mod_stats);
     LOG_D(PHY,"feptx_ofdm (TXPATH): frame %d, subframe %d: txp (time %p) %d dB, txp (freq) %d dB\n",
	   ru->proc.frame_tx,subframe,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_tti)),
	   dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 0 );
}

void feptx_prec(RU_t *ru) {

  int l,i,aa;
  PHY_VARS_eNB **eNB_list = ru->eNB_list,*eNB;
  LTE_DL_FRAME_PARMS *fp;
  int32_t ***bw;
  int subframe = ru->proc.subframe_tx;

  if (ru->num_eNB == 1) {
    eNB = eNB_list[0];
    fp  = &eNB->frame_parms;
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 1);

    for (aa=0;aa<ru->nb_tx;aa++)
      memcpy((void*)ru->common.txdataF_BF[aa],
	     (void*)&eNB->common_vars.txdataF[aa][subframe*fp->symbols_per_tti*fp->ofdm_symbol_size],
	     fp->symbols_per_tti*fp->ofdm_symbol_size*sizeof(int32_t));

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC , 0);
  }
  else {
    for (i=0;i<ru->num_eNB;i++) {
      eNB = eNB_list[i];
      fp  = &eNB->frame_parms;
      bw  = ru->beam_weights[i];
      
      for (l=0;l<fp->symbols_per_tti;l++) {
	for (aa=0;aa<ru->nb_tx;aa++) {
	  beam_precoding(eNB->common_vars.txdataF,
			 ru->common.txdataF_BF,
			 fp,
			 bw,
			 subframe<<1,
			 l,
			 aa);
	}
      }
    }
  }
}

void fep0(RU_t *ru,int slot) {

  RU_proc_t *proc       = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  int l;

  //  printf("fep0: slot %d\n",slot);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+slot, 1);

  remove_7_5_kHz(ru,(slot&1)+(proc->subframe_rx<<1));
  for (l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(ru,
		l,
		(slot&1),
		0
		);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+slot, 0);
}



static void *fep_thread(void *param) {

  RU_t *ru = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;

  thread_top_init("fep_thread",1,100000,120000,5000000);
  pthread_setname_np( pthread_self(),"fep processing");
  LOG_I(PHY,"thread fep created id=%ld\n", syscall(__NR_gettid));

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  //CPU_SET(2, &cpuset);
  //pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  //wait_sync("fep_thread");

  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"fep thread")<0) break; 
    if (oai_exit) break;
	//stop_meas(&ru->ofdm_demod_wakeup_stats);
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX1, 1 ); 
    fep0(ru,0);
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX1, 0 ); 
    if (release_thread(&proc->mutex_fep,&proc->instance_cnt_fep,"fep thread")<0) break;

    if (pthread_cond_signal(&proc->cond_fep) != 0) {
      printf("[eNB] ERROR pthread_cond_signal for fep thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
    /*if(opp_enabled == 1 && ru->ofdm_demod_wakeup_stats.p_time>30*3000){
      print_meas_now(&ru->ofdm_demod_wakeup_stats,"fep wakeup",stderr);
      printf("delay in fep wakeup in frame_rx: %d  subframe_rx: %d \n",proc->frame_rx,proc->subframe_rx);
    }*/
  }



  return(NULL);
}

void init_feptx_thread(RU_t *ru,pthread_attr_t *attr_feptx) {

  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_feptx         = -1;
    
  pthread_mutex_init( &proc->mutex_feptx, NULL);
  pthread_cond_init( &proc->cond_feptx, NULL);

  pthread_create(&proc->pthread_feptx, attr_feptx, feptx_thread, (void*)ru);


}

void init_fep_thread(RU_t *ru,pthread_attr_t *attr_fep) {

  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_fep         = -1;
    
  pthread_mutex_init( &proc->mutex_fep, NULL);
  pthread_cond_init( &proc->cond_fep, NULL);

  pthread_create(&proc->pthread_fep, attr_fep, fep_thread, (void*)ru);


}

extern void kill_fep_thread(RU_t *ru)
{
  RU_proc_t *proc = &ru->proc;
  pthread_mutex_lock( &proc->mutex_fep );
  proc->instance_cnt_fep         = 0;
  pthread_cond_signal(&proc->cond_fep);
  pthread_mutex_unlock( &proc->mutex_fep );
  LOG_D(PHY, "Joining pthread_fep\n");
  pthread_join(proc->pthread_fep, NULL);
  pthread_mutex_destroy( &proc->mutex_fep );
  pthread_cond_destroy( &proc->cond_fep );
}

extern void kill_feptx_thread(RU_t *ru)
{
  RU_proc_t *proc = &ru->proc;
  pthread_mutex_lock( &proc->mutex_feptx );
  proc->instance_cnt_feptx         = 0;
  pthread_cond_signal(&proc->cond_feptx);
  pthread_mutex_unlock( &proc->mutex_feptx );
  LOG_D(PHY, "Joining pthread_feptx\n");
  pthread_join(proc->pthread_feptx, NULL);
  pthread_mutex_destroy( &proc->mutex_feptx );
  pthread_cond_destroy( &proc->cond_feptx );
}

void ru_fep_full_2thread(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;

  struct timespec wait;

  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  if ((fp->frame_type == TDD) &&
     (subframe_select(fp,proc->subframe_rx) != SF_UL)) return;

  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );
  start_meas(&ru->ofdm_demod_stats);

  if (pthread_mutex_timedlock(&proc->mutex_fep,&wait) != 0) {
    printf("[RU] ERROR pthread_mutex_lock for fep thread (IC %d)\n", proc->instance_cnt_fep);
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
    printf("[RU] ERROR pthread_cond_signal for fep thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return;
  }
  //start_meas(&ru->ofdm_demod_wakeup_stats);
  
  pthread_mutex_unlock( &proc->mutex_fep );

  // call second slot in this symbol
  fep0(ru,1);

  start_meas(&ru->ofdm_demod_wait_stats);
  wait_on_busy_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"fep thread");  
  stop_meas(&ru->ofdm_demod_wait_stats);
  if(opp_enabled == 1 && ru->ofdm_demod_wakeup_stats.p_time>30*3000){
    print_meas_now(&ru->ofdm_demod_wakeup_stats,"fep wakeup",stderr);
    printf("delay in fep wait on codition in frame_rx: %d  subframe_rx: %d \n",proc->frame_rx,proc->subframe_rx);
  }

  stop_meas(&ru->ofdm_demod_stats);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 0 );
}



void fep_full(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;
  int l;
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  if ((fp->frame_type == TDD) && 
     (subframe_select(fp,proc->subframe_rx) != SF_UL)) return;

  start_meas(&ru->ofdm_demod_stats);
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );

  remove_7_5_kHz(ru,proc->subframe_rx<<1);
  remove_7_5_kHz(ru,1+(proc->subframe_rx<<1));

  for (l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(ru,
		l,
		0,
		0
		);
    slot_fep_ul(ru,
		l,
		1,
		0
		);
  }
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 0 );
  stop_meas(&ru->ofdm_demod_stats);
  
  
}


void do_prach_ru(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,proc->frame_prach,proc->subframe_prach)>0) { 
    //accept some delay in processing - up to 5ms
    int i;
    for (i = 0; i < 10 && proc->instance_cnt_prach == 0; i++) {
      LOG_W(PHY,"Frame %d Subframe %d, PRACH thread busy (IC %d)!!\n", proc->frame_prach,proc->subframe_prach,
	    proc->instance_cnt_prach);
      usleep(500);
    }
    if (proc->instance_cnt_prach == 0) {
      exit_fun( "PRACH thread busy" );
      return;
    }
    
    // wake up thread for PRACH RX
    if (pthread_mutex_lock(&proc->mutex_prach) != 0) {
      LOG_E( PHY, "ERROR pthread_mutex_lock for PRACH thread (IC %d)\n", proc->instance_cnt_prach );
      exit_fun( "error locking mutex_prach" );
      return;
    }
    
    ++proc->instance_cnt_prach;
    
    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach) != 0) {
      LOG_E( PHY, "ERROR pthread_cond_signal for PRACH thread\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }
    
    pthread_mutex_unlock( &proc->mutex_prach );
  }

}
