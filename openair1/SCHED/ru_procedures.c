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


#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
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
#include "msc.h"

#include <time.h>

#include "targets/RT/USER/rt_wrapper.h"

extern int oai_exit;



void feptx0(RU_t *ru, L1_rxtx_proc_t *proc, int slot) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  //int dummy_tx_b[7680*2] __attribute__((aligned(32)));

  unsigned int aa,slot_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((fp->Ncp==1) ? 6 : 7);
  int subframe = proc->subframe_tx;


  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 1 );

  slot_offset = subframe*fp->samples_per_tti + (slot*(fp->samples_per_tti>>1));

  //LOG_D(PHY,"SFN/SF:RU:TX:%d/%d Generating slot %d\n",ru->proc.frame_tx, ru->proc.subframe_tx,slot);

  for (aa=0; aa<ru->nb_tx; aa++) {
    if (fp->Ncp == EXTENDED) PHY_ofdm_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
					                      (int*)&ru->common.txdata[aa][slot_offset],
					                      fp->ofdm_symbol_size,
					                      6,
					                      fp->nb_prefix_samples,
					                      CYCLIC_PREFIX);
    else {
     /* AssertFatal(ru->generate_dmrs_sync==1 && (fp->frame_type != TDD || ru->is_slave == 1),
		  "ru->generate_dmrs_sync should not be set, frame_type %d, is_slave %d\n",
		  fp->frame_type,ru->is_slave);
*/
      int num_symb = 7;

      if (subframe_select(fp,subframe) == SF_S) num_symb=fp->dl_symbols_in_S_subframe+1;
   
      if (ru->generate_dmrs_sync == 1 && slot == 0 && subframe == 1 && aa==0) {
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
      normal_prefix_mod(&ru->common.txdataF_BF[aa][slot*slot_sizeF],
                        (int*)&ru->common.txdata[aa][slot_offset],
                        num_symb,
                        fp);

       
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
  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+slot , 0);
}

void feptx_ofdm(RU_t *ru, L1_rxtx_proc_t* proc) {
     
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i,j, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((fp->Ncp==1) ? 6 : 7);
  int len,len2;
  int16_t *txdata;
  int subframe = proc->subframe_tx;

//  int CC_id = ru->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+ru->idx , 1 );
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
     LOG_D(PHY,"feptx_ofdm (TXPATH): subframe %d: txp (time %p) %d dB, txp (freq) %d dB\n",
	   subframe,txdata,dB_fixed(signal_energy((int32_t*)txdata,fp->samples_per_tti)),
	   dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_OFDM+ru->idx , 0 );
}

void feptx_prec(RU_t *ru, L1_rxtx_proc_t * proc) {
	
  int l,i,aa,p;
  int subframe = proc->subframe_tx;
  PHY_VARS_eNB **eNB_list = ru->eNB_list,*eNB; 
  LTE_DL_FRAME_PARMS *fp;

  /* fdragon
  int l,i,aa;
  PHY_VARS_eNB **eNB_list = ru->eNB_list,*eNB;
  LTE_DL_FRAME_PARMS *fp;
  int32_t ***bw;
  int subframe = ru->proc.subframe_tx;
  */


  if (ru->num_eNB == 1) {

    eNB = eNB_list[0];
    fp  = &eNB->frame_parms;
    LTE_eNB_PDCCH *pdcch_vars = &eNB->pdcch_vars[subframe&1]; 
    
    if (subframe_select(fp,subframe) == SF_UL) return;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPTX_PREC+ru->idx , 1);

    for (aa=0;aa<ru->nb_tx;aa++) {
      memset(ru->common.txdataF_BF[aa],0,sizeof(int32_t)*fp->ofdm_symbol_size*fp->symbols_per_tti);
      for (p=0;p<NB_ANTENNA_PORTS_ENB;p++) {
	if (ru->do_precoding == 0) {	
	  if (p==0) 
	    memcpy((void*)ru->common.txdataF_BF[aa],
	    (void*)&eNB->common_vars.txdataF[aa][subframe*fp->symbols_per_tti*fp->ofdm_symbol_size],
	    sizeof(int32_t)*fp->ofdm_symbol_size*fp->symbols_per_tti);
	} else {
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
	} // ru->do_precoding!=0	
      } // for (p=0...)
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

void fep0(RU_t *ru,L1_rxtx_proc_t *proc, int slot) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  int l;

    //printf("fep0: slot %d\n",slot);

  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+slot, 1);
  remove_7_5_kHz(ru,(slot&1)+(proc->subframe_rx<<1));
  for (l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(ru,
		l,
		(slot&1),
		0
		);
  }
  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+slot, 0);
}

void fep_full(RU_t *ru, L1_rxtx_proc_t *proc) {

  int l;
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  if ((fp->frame_type == TDD) && 
     (subframe_select(fp,proc->subframe_rx) != SF_UL)) return;

  start_meas(&ru->ofdm_demod_stats);
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+ru->idx, 1 );

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
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX+ru->idx, 0 );
  stop_meas(&ru->ofdm_demod_stats);
  
  
}

