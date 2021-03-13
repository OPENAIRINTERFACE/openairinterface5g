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
 
/*! \function feptx_prec
 * \brief Implementation of precoding for beamforming in one eNB
 * \author TY Hsu, SY Yeh(fdragon), TH Wang(Judy)
 * \date 2018
 * \version 0.1
 * \company ISIP@NCTU and Eurecom
 * \email: tyhsu@cs.nctu.edu.tw,fdragon.cs96g@g2.nctu.edu.tw,Tsu-Han.Wang@eurecom.fr
 * \note
 * \warning
 */

/*! \function feptx0
 * \brief Implementation of ofdm encoding for FeMBMS profile in one eNB
 * \author J. Morgade
 * \date 2020
 * \version 0.1
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */



#include "PHY/defs_eNB.h"
#include "SCHED/sched_eNB.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"

#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "assertions.h"
#include "common/utils/system.h"
#include "msc.h"

#include <time.h>

extern int oai_exit;


void feptx0(RU_t *ru,
            int slot)
{
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  //int dummy_tx_b[7680*2] __attribute__((aligned(32)));

  unsigned int aa, slot_offset;
  int slot_sizeF = (fp->ofdm_symbol_size) * ((fp->Ncp==1) ? 6 : 7);
  int subframe = ru->proc.tti_tx;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+(slot&1) , 1 );

  slot_offset = slot*(fp->samples_per_tti>>1); //slot_offset = subframe*fp->samples_per_tti + (slot*(fp->samples_per_tti>>1));

  //LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d\n",ru->proc.frame_tx, ru->proc.tti_tx,slot);

  for (aa=0; aa<ru->nb_tx; aa++) {
    if (fp->Ncp == EXTENDED) {
      PHY_ofdm_mod(&ru->common.txdataF_BF[aa][(slot&1)*slot_sizeF],
                   (int*)&ru->common.txdata[aa][slot_offset],
                   fp->ofdm_symbol_size,
                   6,
                   fp->nb_prefix_samples,
                   CYCLIC_PREFIX);
    } else {
      if(is_fembms_pmch_subframe(ru->proc.frame_tx,subframe,fp)){
        if((slot&1)==0){//just use one slot chance
               PHY_ofdm_mod(&ru->common.txdataF_BF[aa][(slot&1)*slot_sizeF],
                       (int*)&ru->common.txdata[aa][slot_offset],
                       fp->ofdm_symbol_size_khz_1dot25,
                       1,
                       fp->ofdm_symbol_size_khz_1dot25>>2,
                       CYCLIC_PREFIX);
               LOG_D(HW,"Generating PMCH FeMBMS TX subframe %d %d\n",subframe,fp->ofdm_symbol_size_khz_1dot25);
      	}
      } else if(is_pmch_subframe(ru->proc.frame_tx,subframe,fp)){
        if ((slot&1) == 0) {//just use one slot chance
          normal_prefix_mod(&ru->common.txdataF_BF[aa][(slot&1)*slot_sizeF],
                            (int*)&ru->common.txdata[aa][slot_offset],
                            2,
                            fp);
          PHY_ofdm_mod(&ru->common.txdataF_BF[aa][(slot&1)*slot_sizeF+fp->ofdm_symbol_size*2],
                       (int*)&ru->common.txdata[aa][slot_offset+((fp->ofdm_symbol_size>>2)*2+fp->ofdm_symbol_size*2)],
                       fp->ofdm_symbol_size,
                       10,
                       fp->ofdm_symbol_size>>2,
                       CYCLIC_PREFIX);
          LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d F(%d) t(%d) IS PMCH(%d)\n",
                    ru->proc.frame_tx,
                    ru->proc.tti_tx,
                    slot,
                    slot*slot_sizeF+fp->ofdm_symbol_size*2,
                    slot_offset+((fp->ofdm_symbol_size>>2)*2+fp->ofdm_symbol_size*2),
                    is_pmch_subframe(ru->proc.frame_tx,subframe,fp));
        }
      } else {
         /* AssertFatal(ru->generate_dmrs_sync==1 && (fp->frame_type != TDD || ru->is_slave == 1),
                        "ru->generate_dmrs_sync should not be set, frame_type %d, is_slave %d\n",
                        fp->frame_type,ru->is_slave);
         */
	      int num_symb = 7;

	      if (subframe_select(fp,subframe) == SF_S)
                num_symb = fp->dl_symbols_in_S_subframe+1;

	      if (ru->generate_dmrs_sync == 1 && slot == 2 && aa==0) {
            //int32_t dmrs[ru->frame_parms.ofdm_symbol_size*14] __attribute__((aligned(32)));
            //int32_t *dmrsp[2] ={dmrs,NULL}; //{&dmrs[(3-ru->frame_parms.Ncp)*ru->frame_parms.ofdm_symbol_size],NULL};
                  generate_drs_pusch((PHY_VARS_UE *)NULL,
                                     (UE_rxtx_proc_t*)NULL,
                                     fp,
                                     ru->common.txdataF_BF,
                                     0,
                                     AMP,
                                     0,
                                     0,
                                     fp->N_RB_DL,
                                     aa);
	      } 
              normal_prefix_mod(&ru->common.txdataF_BF[aa][(slot&1)*slot_sizeF],
                                (int*)&ru->common.txdata[aa][slot_offset],
                                num_symb,
                                fp);
      }
    }

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
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+(slot&1), 0);
}


static void *feptx_thread(void *param)
{
  RU_t *ru         = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;
  
  LOG_I(PHY,"thread feptx created \n");

  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"feptx thread")<0) break;  
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+1 , 1 );

    if (oai_exit) break;
    //stop_meas(&ru->ofdm_mod_wakeup_stats);
    feptx0(ru,proc->slot_feptx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+1 , 0 );

    if (release_thread(&proc->mutex_feptx,&proc->instance_cnt_feptx,"feptx thread")<0) break;

    if (pthread_cond_signal(&proc->cond_feptx) != 0) {
      printf("[eNB] ERROR pthread_cond_signal for feptx thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
	/*if(opp_enabled == 1 && ru->ofdm_mod_wakeup_stats.p_time>30*3000){
      print_meas_now(&ru->ofdm_mod_wakeup_stats,"fep wakeup",stderr);
      printf("delay in fep wakeup in frame_tx: %d  subframe_rx: %d \n",proc->frame_tx,proc->tti_tx);
    }*/
  }
  return(NULL);
}


void feptx_ofdm_2thread(RU_t *ru,
                        int frame,
                        int subframe)
{
  LTE_DL_FRAME_PARMS *fp=ru->frame_parms;
  RU_proc_t *proc = &ru->proc;
  struct timespec wait;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  start_meas(&ru->ofdm_mod_stats);

  if (subframe_select(fp,subframe) == SF_UL) return;

  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM , 1 );

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
   
    proc->slot_feptx=1+(subframe<<1); 
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
  feptx0(ru,(subframe<<1));
  start_meas(&ru->ofdm_mod_wait_stats);
  wait_on_busy_condition(&proc->mutex_feptx,&proc->cond_feptx,&proc->instance_cnt_feptx,"feptx thread");  
  stop_meas(&ru->ofdm_mod_wait_stats);
  /*if(opp_enabled == 1 && ru->ofdm_mod_wait_stats.p_time>30*3000){
    print_meas_now(&ru->ofdm_mod_wait_stats,"fep wakeup",stderr);
    printf("delay in feptx wait on condition in frame_rx: %d  subframe_rx: %d \n", proc->frame_tx, proc->tti_tx);
  }*/

  stop_meas(&ru->ofdm_mod_stats);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+ru->idx , 0 );
}


void feptx_ofdm(RU_t *ru,
                int frame,
                int subframe)
{
  LTE_DL_FRAME_PARMS *fp=ru->frame_parms;
  unsigned int aa, slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i, j, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size) * ((fp->Ncp==1) ? 6 : 7);
  int len,len2;
  int16_t *txdata;

//  int CC_id = ru->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+ru->idx , 1 );
  slot_offset_F = 0;

  slot_offset = subframe*fp->samples_per_tti;

  if ((subframe_select(fp,subframe)==SF_DL)|| ((subframe_select(fp,subframe)==SF_S)))
  {
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
       if(is_pmch_subframe(frame,subframe,fp)/*subframe==1*/){
        normal_prefix_mod(&ru->common.txdataF_BF[aa][0],
                          dummy_tx_b,
                          2,
                          fp);
        PHY_ofdm_mod(&ru->common.txdataF_BF[aa][fp->ofdm_symbol_size*2],
                        dummy_tx_b+((fp->ofdm_symbol_size>>2)*2+fp->ofdm_symbol_size*2),
                     fp->ofdm_symbol_size,
                     10,
                     fp->ofdm_symbol_size>>2,
                    CYCLIC_PREFIX);
        }else{
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
	   frame,subframe,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_tti)),
	   dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+ru->idx , 0 );
}


void feptx_prec(RU_t *ru,
                int frame,
                int subframe)
{
  int l,i,aa;
  PHY_VARS_eNB **eNB_list = ru->eNB_list, *eNB;
  LTE_DL_FRAME_PARMS *fp;

  if (ru->num_eNB == 1)
  {
    eNB = eNB_list[0];
    fp  = &eNB->frame_parms;
    LTE_eNB_PDCCH *pdcch_vars = &eNB->pdcch_vars[subframe&1]; 
    
    if (subframe_select(fp,subframe) == SF_UL) return;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC+ru->idx , 1);

    for (aa=0;aa<ru->nb_tx;aa++) {
      if (ru->do_precoding == 0) {
          memcpy((void*)ru->common.txdataF_BF[aa],
                 (void*)&eNB->common_vars.txdataF[aa%fp->nb_antenna_ports_eNB][subframe*fp->symbols_per_tti*fp->ofdm_symbol_size],
                 sizeof(int32_t)*fp->ofdm_symbol_size*fp->symbols_per_tti);

      }
      else {
         memset(ru->common.txdataF_BF[aa],0,sizeof(int32_t)*fp->ofdm_symbol_size*fp->symbols_per_tti);
         for (int p=0;p<NB_ANTENNA_PORTS_ENB;p++) {
    	    if (p<fp->nb_antenna_ports_eNB) {
	        // For the moment this does nothing different than below, except ignore antenna ports 5,7,8.
	        for (l=0;l<pdcch_vars->num_pdcch_symbols;l++)
	          beam_precoding(eNB->common_vars.txdataF,
                             ru->common.txdataF_BF,
                             subframe,
                             fp,
                             ru->beam_weights,
                             l,
                             aa,
                             p,
                             eNB->Mod_id);
	    } //if (p<fp->nb_antenna_ports_eNB)
	  
	      // PDSCH region
    	    if (p<fp->nb_antenna_ports_eNB || p==5 || p==7 || p==8) {
	        for (l=pdcch_vars->num_pdcch_symbols;l<fp->symbols_per_tti;l++) {
	          beam_precoding(eNB->common_vars.txdataF,
                             ru->common.txdataF_BF,
                             subframe,
                             fp,
                             ru->beam_weights,
                             l,
                             aa,
                             p,
                             eNB->Mod_id);
	        } // for (l=pdcch_vars ....)
	      } // if (p<fp->nb_antenna_ports_eNB) ...
           } // for (p=0...)
        } // if do_precoding
    } // for (aa=0 ...)
    
    if(ru->idx<2)
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC+ru->idx , 0);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC+ru->idx , 0);
  }
  else {
    AssertFatal(1==0,"Handling of multi-L1 case not ready yet\n");
    for (i=0;i<ru->num_eNB;i++) {
      eNB = eNB_list[i];
      fp  = &eNB->frame_parms;
      
      for (l=0;l<fp->symbols_per_tti;l++) {
	    for (aa=0;aa<ru->nb_tx;aa++) {
	      beam_precoding(eNB->common_vars.txdataF,
                         ru->common.txdataF_BF,
                         subframe,
                         fp,
                         ru->beam_weights,
                         subframe<<1,
                         l,
                         aa,
                         eNB->Mod_id);
	    }
      }
    }
  }
}


void fep0(RU_t *ru,
          int slot)
{
  RU_proc_t *proc        = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;

  //printf("fep0: slot %d\n",slot);

  remove_7_5_kHz(ru,(slot&1)+(proc->tti_rx<<1));
  for (int l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(ru,
                l,
                (slot&1),
                0);
  }
  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+slot, 0);
}


static void *fep_thread(void *param)
{
  RU_t *ru = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;

  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"fep thread")<0) break; 
    if (oai_exit) break;
	//stop_meas(&ru->ofdm_demod_wakeup_stats);
	//VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX1, 1 ); 
    fep0(ru,0);
	//VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX1, 0 ); 
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


void init_feptx_thread(RU_t *ru,
                       pthread_attr_t *attr_feptx)
{
  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_feptx         = -1;
    
  pthread_mutex_init( &proc->mutex_feptx, NULL);
  pthread_cond_init( &proc->cond_feptx, NULL);

  threadCreate(&proc->pthread_feptx, feptx_thread, (void*)ru, "feptx", -1, OAI_PRIORITY_RT);
}


void init_fep_thread(RU_t *ru,
                     pthread_attr_t *attr_fep)
{
  RU_proc_t *proc = &ru->proc;

  proc->instance_cnt_fep         = -1;
    
  pthread_mutex_init( &proc->mutex_fep, NULL);
  pthread_cond_init( &proc->cond_fep, NULL);

  threadCreate(&proc->pthread_fep, fep_thread, (void*)ru, "fep", -1, OAI_PRIORITY_RT);
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


void ru_fep_full_2thread(RU_t *ru,
                         int subframe)
{
  RU_proc_t *proc = &ru->proc;
  //PHY_VARS_eNB *eNB = RC.eNB[0][0];
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_CALIBRATION *calibration = &ru->calibration;
  RRU_CONFIG_msg_t rru_config_msg;
  int check_sync_pos;

  struct timespec wait;

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe) == SF_DL)) return;

  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 );

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+ru->idx, 1 );
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
    printf("delay in fep wait on condition in frame_rx: %d  subframe_rx: %d \n", proc->frame_rx, proc->tti_rx);
  }

  if (proc->tti_rx/*proc->subframe_rx*/==1 && ru->is_slave==1/* && ru->state == RU_CHECK_SYNC*/)
  {
    LOG_I(PHY,"Running check synchronization procedure for frame %d\n", proc->frame_rx);
  	ulsch_extract_rbs_single(ru->common.rxdataF,
                             calibration->rxdataF_ext,
                             0,
                             fp->N_RB_DL,
                             3%(fp->symbols_per_tti/2),// l = symbol within slot
                             3/(fp->symbols_per_tti/2),// Ns = slot number
                             fp);
        
	lte_ul_channel_estimation_RRU(fp,
                                  calibration->drs_ch_estimates,
                                  calibration->drs_ch_estimates_time,
                                  calibration->rxdataF_ext,
                                  fp->N_RB_DL, //N_rb_alloc,
                                  proc->frame_rx,
								  proc->tti_rx,//proc->subframe_rx,
                                  0,//u = 0..29
                                  0,//v = 0,1
                                  /*eNB->ulsch[ru->idx]->cyclicShift,cyclic_shift,0..7*/0,
                                  3,//l,
                                  0,//interpolate,
                                  0 /*eNB->ulsch[ru->idx]->rnti rnti or ru->ulsch[eNB_id]->rnti*/);

	check_sync_pos = lte_est_timing_advance_pusch(ru->frame_parms, ru->calibration.drs_ch_estimates_time); 
        if (ru->state == RU_CHECK_SYNC) {
          if ((check_sync_pos >= 0 && check_sync_pos<8) || (check_sync_pos < 0 && check_sync_pos>-8)) {
//    		  LOG_I(PHY,"~~~~~~~~~~~    check_sync_pos %d, frame %d, cnt %d\n",check_sync_pos,proc->frame_rx,ru->wait_check); 
                  ru->wait_check++;
          }
          LOG_I(PHY,"~~~~~~~~~~~    check_sync_pos %d, frame %d, cnt %d\n",check_sync_pos,proc->frame_rx,ru->wait_check);

          if (ru->wait_check==20) { 
            ru->state = RU_RUN;
            ru->wait_check = 0;
            // Send RRU_sync_ok
            rru_config_msg.type = RRU_sync_ok;
            rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t); // TODO: set to correct msg len
            LOG_I(PHY,"Sending RRU_sync_ok to RAU\n");
            AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),"Failed to send msg to RAU %d\n",ru->idx);
/*
                //LOG_I(PHY,"~~~~~~~~~ RU_RUN\n");
          	LOG_M("dmrs_time.m","dmrstime",calibration->drs_ch_estimates_time[0], (fp->ofdm_symbol_size),1,1);
		LOG_M("rxdataF_ext.m","rxdataFext",&calibration->rxdataF_ext[0][36*fp->N_RB_DL], 12*(fp->N_RB_DL),1,1);		
		//LOG_M("drs_seq0.m","drsseq0",ul_ref_sigs_rx[0][0][23],600,1,1);
		LOG_M("rxdata.m","rxdata",&ru->common.rxdata[0][0], fp->samples_per_tti*2,1,1);
		exit(-1);*/
          }
        } else if (ru->state == RU_RUN) {
       	// check for synchronization error
       	  if (check_sync_pos >= 8 || check_sync_pos<=-8) {
            LOG_E(PHY,"~~~~~~~~~~~~~~ check_sync_pos %d, frame %d ---> LOST SYNC-EXIT\n", check_sync_pos, proc->frame_rx);
            LOG_M("rxdata.m","rxdata",&ru->common.rxdata[0][0], fp->samples_per_tti*2,1,1);
            exit(-1);
          }
        } else {
            AssertFatal(1==0,"Should not get here\n");
        }
  }

  stop_meas(&ru->ofdm_demod_stats);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+ru->idx, 0 );
}


void fep_full(RU_t *ru,
              int subframe)
{
  int l;
  LTE_DL_FRAME_PARMS *fp=ru->frame_parms;

  if ((fp->frame_type == TDD) && 
     (subframe_select(fp,subframe) != SF_UL)) return;
  start_meas(&ru->ofdm_demod_stats);
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+ru->idx, 1 );

  remove_7_5_kHz(ru,subframe<<1);
  remove_7_5_kHz(ru,1+(subframe<<1));

  for (l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(ru,
                l,
                0,
                0);
    slot_fep_ul(ru,
                l,
                1,
                0);
  }
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+ru->idx, 0 );
  stop_meas(&ru->ofdm_demod_stats);
}


void do_prach_ru(RU_t *ru)
{
  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp=ru->frame_parms;

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
