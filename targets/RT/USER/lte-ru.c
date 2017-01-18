/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file lte-enb.c
 * \brief Top-level threads for eNodeB
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include "rt_wrapper.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include "msc.h"

#include "PHY/types.h"

#include "PHY/defs.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all


#include "../../ARCH/COMMON/common_lib.h"


#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "PHY/extern.h"
#include "SCHED/extern.h"
#include "LAYER2/MAC/extern.h"

#include "../../SIMU/USER/init_lte.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif
#include "UTIL/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
#include "UTIL/OTG/otg_extern.h"
#endif

#if defined(ENABLE_ITTI)
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#ifdef PDCP_USE_NETLINK
#   include "SIMULATION/ETH_TRANSPORT/proto.h"
#endif
# endif
#endif

#include "T.h"

extern volatile int                    oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];


static int                      time_offset[4] = {0,0,0,0};

void init_RU(eNB_func_t node_function, RU_if_in_t ru_if_in[], RU_if_timing_t ru_if_timing[], eth_params_t *eth_params);
void stop_RU(RU_t *ru);



// RU OFDM Modulator, used in IF4p5 RRU, RCC/RAU with IF5, eNodeB

void do_OFDM_mod_rt(int subframe,RU_t *ru) {
     
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i,j, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((fp->Ncp==1) ? 6 : 7);
  int len,len2;
  int16_t *txdata;
//  int CC_id = ru->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_SFGEN , 1 );

  slot_offset_F = (subframe<<1)*slot_sizeF;

  slot_offset = subframe*fp->samples_per_tti;

  if ((subframe_select(fp,subframe)==SF_DL)||
      ((subframe_select(fp,subframe)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

    for (aa=0; aa<ru->nb_tx; aa++) {
      if (fp->Ncp == EXTENDED) {
        PHY_ofdm_mod(&ru->ru_time.txdataF[aa][slot_offset_F],
                     dummy_tx_b,
                     fp->ofdm_symbol_size,
                     6,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
        PHY_ofdm_mod(&ru->ru_time.txdataF[aa][slot_offset_F+slot_sizeF],
                     dummy_tx_b+(fp->samples_per_tti>>1),
                     fp->ofdm_symbol_size,
                     6,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
      } else {
        normal_prefix_mod(&ru->ru_time.txdataF[aa][slot_offset_F],
                          dummy_tx_b,
                          7,
                          fp);
	// if S-subframe generate first slot only
	if (subframe_select(fp,subframe) == SF_DL) 
	  normal_prefix_mod(&ru->ru_time.txdataF[aa][slot_offset_F+slot_sizeF],
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
      
      if (slot_offset+time_offset[aa]<0) {
	txdata = (int16_t*)&ru->ru_time.txdata[aa][(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)+tx_offset];
        len2 = -(slot_offset+time_offset[aa]);
	len2 = (len2>len) ? len : len2;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
	if (len2<len) {
	  txdata = (int16_t*)&ru->ru_time.txdata[aa][0];
	  for (j=0; i<(len<<1); i++,j++) {
	    txdata[j++] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	  }
	}
      }  
      else if ((slot_offset+time_offset[aa]+len)>(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)) {
	tx_offset = (int)slot_offset+time_offset[aa];
	txdata = (int16_t*)&ru->ru_time.txdata[aa][tx_offset];
	len2 = -tx_offset+LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
	txdata = (int16_t*)&ru->ru_time.txdata[aa][0];
	for (j=0; i<(len<<1); i++,j++) {
	  txdata[j++] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
      }
      else {
	tx_offset = (int)slot_offset+time_offset[aa];
	txdata = (int16_t*)&ru->ru_time.txdata[aa][tx_offset];

	for (i=0; i<(len<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
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
     if ((((fp->tdd_config==0) ||
	   (fp->tdd_config==1) ||
	   (fp->tdd_config==2) ||
	   (fp->tdd_config==6)) && 
	   (subframe==0)) || (subframe==5)) {
       // turn on tx switch N_TA_offset before
       //LOG_D(HW,"subframe %d, time to switch to tx (N_TA_offset %d, slot_offset %d) \n",subframe,ru->N_TA_offset,slot_offset);
       for (i=0; i<ru->N_TA_offset; i++) {
         tx_offset = (int)slot_offset+time_offset[aa]+i-ru->N_TA_offset/2;
         if (tx_offset<0)
           tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	 
         if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti))
           tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	 
         ru->ru_time.txdata[aa][tx_offset] = 0x00000000;
       }
     }
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_SFGEN , 0 );
}

/*************************************************************/
/* Southbound Fronthaul functions, RCC/RAU                   */

// southbound IF5 fronthaul for 16-bit OAI format
static inline void fh_if5_south_out(RU_t *ru) {
  if (ru == RC.ru_list[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  send_IF5(eNB, ru->proc.timestamp_txp ru->proc.subframe_tx, &ru->seqno, IF5_RRH_GW_DL);
}

// southbound IF5 fronthaul for Mobipass packet format
static inline void fh_if5_mobipass_south_out(RU_t *ru) {
  if (ru == RC.ru_list[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  send_IF5(eNB, ru->proc.timestamp_tx, ru->proc.subframe_tx, &ru->seqno, IF5_MOBIPASS); 
}

// southbound IF4p5 fronthaul
static inline void fh_if4p5_south_out(RU_t *ru) {
  if (ru == RC.ru_list[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );    
  send_IF4p5(eNB,proc->frame_tx, proc->subframe_tx, IF4p5_PDLFFT, 0);
}

/*************************************************************/
/* Input Fronthaul from south RCC/RAU                        */

// Synchronous if5 from south 
void fh_if5_south_in(RU_t *ru,int *frame, int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  ru_proc_t *proc = &ru->proc;

  recv_IF5(ru, &ru->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

  proc->frame_rx    = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx = (proc->timestamp_rx / fp->samples_per_tti)%10;
  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }      
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );

}

// Synchronous if4p5 from south 
void fh_if4p5_south_in(RU_t *ru,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  ru_proc_t *proc = &ru->proc;

  int prach_rx;

  uint16_t packet_type;
  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;

  symbol_mask = 0;
  symbol_mask_full = (1<<fp->symbols_per_tti)-1;
  prach_rx = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &proc->frame_rx, &proc->subframe_rx, &packet_type, &symbol_number);

    if (packet_type == IF4p5_PULFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
      prach_rx = (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)>0) ? 1 : 0;                            
    } else if (packet_type == IF4p5_PRACH) {
      prach_rx = 0;
    }

  } while( (symbol_mask != symbol_mask_full) || (prach_rx == 1));    

  //caculate timestamp_rx, timestamp_tx based on frame and subframe
   proc->timestamp_rx = ((proc->frame_rx * 10)  + proc->subframe_rx ) * fp->samples_per_tti ;
   proc->timestamp_tx = proc->timestamp_rx +  (4*fp->samples_per_tti);
 
 
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  
}

// Dummy FH from south for getting synchronization from master RU
void fh_slave_south_in(RU_t *ru,int *frame,int *subframe) {
  // This case is for synchronization to another thread
  // it just waits for an external event.  The actual rx_fh is handle by the asynchronous RX thread
  ru_proc_t *proc=&ru->proc;

  if (wait_on_condition(&proc->mutex_FH,&proc->cond_FH,&proc->instance_cnt_FH,"fh_slave_south_in") < 0)
    return;

  release_thread(&proc->mutex_FH,&proc->instance_cnt_FH,"rx_fh_slave_south_in");

  
}

// asynchronous inbound if5 fronthaul from south
void fh_if5_south_asynch_in(RU_t *ru,int *frame,int *subframe) {

  eNB_proc_t *proc       = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  recv_IF5(eNB, &ru->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

  proc->subframe_rx = (ru->timestamp_rx/fp->samples_per_tti)%10;
  proc->frame_rx    = (ru->timestamp_rx/(10*fp->samples_per_tti))&1023;

  if (proc->first_rx != 0) {
    proc->first_rx = 0;
    *subframe = proc->subframe_rx;
    *frame    = proc->frame_rx; 
  }
  else {
    if (proc->subframe_rx != *subframe) {
      LOG_E(PHY,"subframe_rx %d is not what we expect %d\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"subframe_rx %d is not what we expect %d\n",proc->frame_rx,*frame);  
      exit_fun("Exiting");
    }
  }
} // eNodeB_3GPP_BBU 

// asynchronous inbound if4p5 fronthaul from south
void fh_if4p5_south_asynch_in(RU_t *ru,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  ru_proc_t *proc       = &ru->proc;

  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,symbol_mask_full,prach_rx;


  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<fp->symbols_per_tti)-1;
  prach_rx = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &proc->frame_rx, &proc->subframe_rx, &packet_type, &symbol_number);
    if (proc->first_rx != 0) {
      *frame = proc->frame_rx;
      *subframe = proc->subframe_rx;
      proc->first_rx = 0;
    }
    else {
      if (proc->frame_rx != *frame) {
	LOG_E(PHY,"frame_rx %d is not what we expect %d\n",proc->frame_rx,*frame);
	exit_fun("Exiting");
      }
      if (proc->subframe_rx != *subframe) {
	LOG_E(PHY,"subframe_rx %d is not what we expect %d\n",proc->subframe_rx,*subframe);
	exit_fun("Exiting");
      }
    }
    if (packet_type == IF4p5_PULFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
      prach_rx = (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)>0) ? 1 : 0;                            
    } else if (packet_type == IF4p5_PRACH) {
      prach_rx = 0;
    }
  } while( (symbol_mask != symbol_mask_full) || (prach_rx == 1));    
} 





/*************************************************************/
/* Input Fronthaul from North RRU                            */
  
// RRU IF4p5 TX fronthaul receiver. Assumes an if_device on input and if or rf device on output 
// receives one subframe's worth of IF4p5 OFDM symbols and OFDM modulates
void fh_if4p5_north_in(RU_t *ru) {

  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;
  uint16_t packet_type;
  ru_proc_t = &ru->proc;

  // dump VCD output for first RU in list
  if (ru == RC.ru_list[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB, proc->frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB, proc->subframe_tx );
  }
  /// **** incoming IF4p5 from remote RCC/RAU **** ///             
  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<ru->frame_parms.symbols_per_tti)-1;
  
  do { 
    recv_IF4p5(ru, &proc->frame_tx, &proc->subframe_tx, &packet_type, &symbol_number);
    symbol_mask = symbol_mask | (1<<symbol_number);
  } while (symbol_mask != symbol_mask_full); 
}

void fh_if5_north_in(RU_t *ru) {
  

  if (ru == RC.ru_list[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB, ru->proc.frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB, ru->proc.subframe_tx );
  }

  /// **** recv_IF5 of txdata from BBU **** ///       
  recv_IF5(ru, &ru->timestamp_tx, proc->subframe_tx, IF5_RRH_GW_DL);

}

void fh_if5_north_asynch_in(RU_t *ru,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  ru_proc_t *proc        = &ru->proc;
  int subframe_tx,frame_tx;
  openair0_timestamp timestamp_tx;

  recv_IF5(ru, &timestamp_tx, *subframe, IF5_RRH_GW_DL); 
      //      printf("Received subframe %d (TS %llu) from RCC\n",subframe_tx,timestamp_tx);

  subframe_tx = (timestamp_tx/fp->samples_per_tti)%10;
  frame_tx    = (timestamp_tx/(fp->samples_per_tti*10))&1023;

  if (proc->first_tx != 0) {
    *subframe = subframe_tx;
    *frame    = frame_tx;
    proc->first_tx = 0;
  }
  else {
    if (subframe_tx != *subframe) {
      LOG_E(PHY,"subframe_tx %d is not what we expect %d\n",subframe_tx,*subframe);
      exit_fun("Exiting");
    }
    if (frame_tx != *frame) { 
      LOG_E(PHY,"frame_tx %d is not what we expect %d\n",frame_tx,*frame);
      exit_fun("Exiting");
    }
  }
}

void fh_if4p5_north_asynch_in(RU_t *ru,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  ru_proc_t *proc        = &ru->proc;

  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,symbol_mask_full;
  int subframe_tx,frame_tx;

  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<fp->symbols_per_tti)-1;

  do {   
    recv_IF4p5(ru, &frame_tx, &subframe_tx, &packet_type, &symbol_number);
    if (proc->first_tx != 0) {
      *frame    = frame_tx;
      *subframe = subframe_tx;
      proc->first_tx = 0;
    }
    else {
      if (frame_tx != *frame) {
	LOG_E(PHY,"frame_tx %d is not what we expect %d\n",frame_tx,*frame);
	exit_fun("Exiting");
      }
      if (subframe_tx != *subframe) {
	LOG_E(PHY,"subframe_tx %d is not what we expect %d\n",subframe_tx,*subframe);
	exit_fun("Exiting");
      }
    }
    if (packet_type == IF4p5_PDLFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
    }
    else {
      LOG_E(PHY,"Illegal IF4p5 packet type (should only be IF4p5_PDLFFT%d\n",packet_type);
      exit_fun("Exiting");
    }
  } while (symbol_mask != symbol_mask_full);    
  
  do_OFDM_mod_rt(subframe_tx, eNB);
} 

void rx_rf(RU_t *ru,int *frame,int *subframe) {

  ru_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *rxp[fp->nb_antennas_rx],*txp[fp->nb_antennas_tx]; 
  unsigned int rxs,txs;
  int i;
  int tx_sfoffset = 3;//(eNB->single_thread_flag == 1) ? 3 : 3;
  if (proc->first_rx==0) {
    
    // Transmit TX buffer based on timestamp from RX
    //    printf("trx_write -> USRP TS %llu (sf %d)\n", (ru->timestamp_rx+(3*fp->samples_per_tti)),(proc->subframe_rx+2)%10);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (ru->timestamp_rx+(tx_sfoffset*fp->samples_per_tti)-openair0_cfg[0].tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
    // prepare tx buffer pointers
	
    for (i=0; i<fp->nb_antennas_tx; i++)
      txp[i] = (void*)&ru->ru_time.txdata[0][i][((proc->subframe_rx+tx_sfoffset)%10)*fp->samples_per_tti];
    
    txs = eNB->rfdevice.trx_write_func(&ru->rfdevice,
				       ru->timestamp_rx+(tx_sfoffset*fp->samples_per_tti)-openair0_cfg[0].tx_sample_advance,
				       txp,
				       fp->samples_per_tti,
				       fp->nb_antennas_tx,
				       1);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );
    
    
    
    if (txs !=  fp->samples_per_tti) {
      LOG_E(PHY,"TX : Timeout (sent %d/%d)\n",txs, fp->samples_per_tti);
      exit_fun( "problem transmitting samples" );
    }	
  }
  
  for (i=0; i<fp->nb_antennas_rx; i++)
    rxp[i] = (void*)&ru->ru_time.rxdata[0][i][*subframe*fp->samples_per_tti];
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );

  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
				   &(ru->timestamp_rx),
				   rxp,
				   fp->samples_per_tti,
				   fp->nb_antennas_rx);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
 
  if (proc->first_rx == 1)
    ru->ts_offset = ru->timestamp_rx;
 
  proc->frame_rx    = ((ru->timestamp_rx-eNB->ts_offset) / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx = ((ru->timestamp_rx-eNB->ts_offset) / fp->samples_per_tti)%10;
  // synchronize first reception to frame 0 subframe 0

  ru->timestamp_tx = ru->timestamp_rx+(4*fp->samples_per_tti);
  //printf("trx_read <- USRP TS %llu (sf %d, f %d, first_rx %d)\n", ru->timestamp_rx,proc->subframe_rx,proc->frame_rx,proc->first_rx);  
  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",ru->timestamp_rx,proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",ru->timestamp_rx,proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  //printf("timestamp_rx %lu, frame %d(%d), subframe %d(%d)\n",ru->timestamp_rx,proc->frame_rx,frame,proc->subframe_rx,subframe);
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, ru->timestamp_rx&0xffffffff );
  
  if (rxs != fp->samples_per_tti)
    exit_fun( "problem receiving samples" );
  

  
}


/*!
 * \brief The Asynchronous RX/TX FH thread of RAU/RCC/eNB/RRU.
 * This handles the RX FH for an asynchronous RRU/UE
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* ru_thread_asynch_rxtx( void* param ) {

  static int ru_thread_asynch_rxtx_status;

  eNB_proc_t *proc = (ru_proc_t*)param;
  RU_t *ru         = PHY_vars_eNB_g[0][proc->CC_id];


  int subframe=0, frame=0; 

  thread_top_init("thread_asynch",1,870000L,1000000L,1000000L);

  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe

  wait_sync("thread_asynch");

  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe
  printf( "waiting for devices (eNB_thread_asynch_rx)\n");

  wait_on_condition(&proc->mutex_asynch_rxtx,&proc->cond_asynch_rxtx,&proc->instance_cnt_asynch_rxtx,"thread_asynch");

  printf( "devices ok (eNB_thread_asynch_rx)\n");


  while (!oai_exit) { 
   
    if (oai_exit) break;   

    if (subframe==9) { 
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }      

    if (eNB->fh_asynch) eNB->fh_asynch(eNB,&frame,&subframe);
    else AssertFatal(1==0, "Unknown eNB->node_function %d",eNB->node_function);
    
  }

  ru_thread_asynch_rxtx_status=0;
  return(&ru_thread_asynch_rxtx_status);
}




// wakeup local eNB process

int wakeup_rxtx(eNB_proc_t *proc,eNB_rxtx_proc_t *proc_rxtx,LTE_DL_FRAME_PARMS *fp) {

  int i;
  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  /* accept some delay in processing - up to 5ms */
  for (i = 0; i < 10 && proc_rxtx->instance_cnt_rxtx == 0; i++) {
    LOG_W( PHY,"[eNB] Frame %d, eNB RXn-TXnp4 thread busy!! (cnt_rxtx %i)\n", proc_rxtx->frame_tx, proc_rxtx->instance_cnt_rxtx);
    usleep(500);
  }
  if (proc_rxtx->instance_cnt_rxtx == 0) {
    exit_fun( "TX thread busy" );
    return(-1);
  }

  // wake up TX for subframe n+4
  // lock the TX mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&proc_rxtx->mutex_rxtx,&wait) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB RXTX thread %d (IC %d)\n", proc_rxtx->subframe_rx&1,proc_rxtx->instance_cnt_rxtx );
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }
  
  ++proc_rxtx->instance_cnt_rxtx;
  
  // We have just received and processed the common part of a subframe, say n. 
  // TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired 
  // transmitted timestamp of the next TX slot (first).
  // The last (TS_rx mod samples_per_frame) was n*samples_per_tti, 
  // we want to generate subframe (n+4), so TS_tx = TX_rx+4*samples_per_tti,
  // and proc->subframe_tx = proc->subframe_rx+4
  proc_rxtx->timestamp_tx = proc->timestamp_rx + (4*fp->samples_per_tti);
  proc_rxtx->frame_rx     = proc->frame_rx;
  proc_rxtx->subframe_rx  = proc->subframe_rx;
  proc_rxtx->frame_tx     = (proc_rxtx->subframe_rx > 5) ? (proc_rxtx->frame_rx+1)&1023 : proc_rxtx->frame_rx;
  proc_rxtx->subframe_tx  = (proc_rxtx->subframe_rx + 4)%10;
  
  // the thread can now be woken up
  if (pthread_cond_signal(&proc_rxtx->cond_rxtx) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB RXn-TXnp4 thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &proc_rxtx->mutex_rxtx );

  return(0);
}

void wakeup_slaves(ru_proc_t *proc) {

  int i;
  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  
  for (i=0;i<proc->num_slaves;i++) {
    ru_proc_t *slave_proc = proc->slave_proc[i];
    // wake up slave FH thread
    // lock the FH mutex and make sure the thread is ready
    if (pthread_mutex_timedlock(&slave_proc->mutex_FH,&wait) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB CCid %d slave CCid %d (IC %d)\n",proc->CC_id,slave_proc->CC_id);
      exit_fun( "error locking mutex_rxtx" );
      break;
    }
    
    int cnt_slave            = ++slave_proc->instance_cnt_FH;
    slave_proc->frame_rx     = proc->frame_rx;
    slave_proc->subframe_rx  = proc->subframe_rx;
    slave_proc->timestamp_rx = proc->timestamp_rx;
    slave_proc->timestamp_tx = proc->timestamp_tx; 

    pthread_mutex_unlock( &slave_proc->mutex_FH );
    
    if (cnt_slave == 0) {
      // the thread was presumably waiting where it should and can now be woken up
      if (pthread_cond_signal(&slave_proc->cond_FH) != 0) {
	LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB CCid %d, slave CCid %d\n",proc->CC_id,slave_proc->CC_id);
          exit_fun( "ERROR pthread_cond_signal" );
	  break;
      }
    } else {
      LOG_W( PHY,"[RU] Frame %d, slave CC_id %d thread busy!! (cnt_FH %i)\n",slave_proc->frame_rx,slave_proc->CC_id, cnt_slave);
      exit_fun( "FH thread busy" );
      break;
    }             
  }
}

/*!
 * \brief The prach receive thread of RU.
 * \param param is a \ref ru_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* ru_thread_prach( void* param ) {
  static int ru_thread_prach_status;

  eNB_proc_t *proc = (eNB_proc_t*)param;
  PHY_VARS_eNB *eNB= PHY_vars_eNB_g[0][proc->CC_id];

  // set default return value
  eNB_thread_prach_status = 0;

  thread_top_init("eNB_thread_prach",1,500000L,1000000L,20000000L);

  while (!oai_exit) {
    
    if (oai_exit) break;

    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;
    
    prach_procedures(eNB);
    
    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;
  }

  printf( "Exiting eNB thread PRACH\n");

  eNB_thread_prach_status = 0;
  return &eNB_thread_prach_status;
}

void do_ru_sync(RU_t *ru) {

  LTE_DL_FRAME_PARMS *fp  = &ru->frame_parms;
  ru_proc_t *proc         = ru->proc;

  void *rxp[2],*rxp2[2];
  int32_t dummy_rx[ru->nb_rx][fp->samples_per_tti] __attribute__((aligned(32)));

  // initialize the synchronization buffer to the common_vars.rxdata
  for (int i=0;i<ru->nb_rx;i++)
    rxp[i] = &ru->ru_time.rxdata[i][0];

  // if FDD, switch RX on DL frequency
  if (ru->rfdevice == NULL) AssertFatal(1==0,"RU doesn't have a local RF device\n");
  
  double temp_freq1 = ru->rfdevice.openair0_cfg->rx_freq[0];
  double temp_freq2 = ru->rfdevice.openair0_cfg->tx_freq[0];
  for (i=0;i<4;i++) {
    ru->rfdevice.openair0_cfg->rx_freq[i] = ru->rfdevice.openair0_cfg->tx_freq[i];
    ru->rfdevice.openair0_cfg->tx_freq[i] = temp_freq1;
  }
  ru->rfdevice.trx_set_freq_func(&ru->rfdevice,ru->rfdevice.openair0_cfg,0);
  
  while ((ru->in_synch ==0)&&(!oai_exit)) {
    // read in frame
    rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
				     &(proc->timestamp_rx),
				     rxp,
				     fp->samples_per_tti*10,
				     ru->nb_rx);
    // wakeup synchronization processing thread
    wakeup_synch(ru);
    ic=0;
    
    while ((ic>=0)&&(!oai_exit)) {
      // continuously read in frames, 1ms at a time, 
      // until we are done with the synchronization procedure
      
      for (i=0; i<fp->nb_antennas_rx; i++)
	rxp2[i] = (void*)&dummy_rx[i][0];
      for (i=0;i<10;i++)
	rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
					 &(proc->timestamp_rx),
					 rxp2,
					 fp->samples_per_tti,
					 ru->nb_rx);
      pthread_mutex_lock(&eNB->proc.mutex_synch);
      ic = eNB->proc.instance_cnt_synch;
      pthread_mutex_unlock(&eNB->proc.mutex_synch);
    } // ic>=0
  } // in_synch==0
    // read in rx_offset samples
  LOG_I(PHY,"Resynchronizing by %d samples\n",ru->rx_offset);
  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
				   &(proc->timestamp_rx),
				   rxp,
				   ru->rx_offset,
				   fp->nb_antennas_rx);
  for (i=0;i<4;i++) {
    ru->rfdevice.openair0_cfg->rx_freq[i] = temp_freq1;
    ru->rfdevice.openair0_cfg->tx_freq[i] = temp_freq2;
  }

  ru->rfdevice.trx_set_freq_func(&ru->rfdevice,ru->rfdevice.openair0_cfg,0);

}

static void* ru_thread( void* param ) {

  static int ru_thread_status;

  RU_t *ru                = (RU_t*)param;
  LTE_DL_FRAME_PARMS *fp  = &ru->frame_parms;
  ru_proc_t *proc         = ru->proc;

  int subframe=0, frame=0; 

  // set default return value
  ru_thread_status = 0;


  // set default return value
  thread_top_init("ru_thread",0,870000,1000000,1000000);

  wait_sync("ru_thread");

  // wakeup asnych_rxtx thread because the devices are ready at this point
  pthread_mutex_lock(&proc->mutex_asynch_rxtx);
  proc->instance_cnt_asynch_rxtx=0;
  pthread_mutex_unlock(&proc->mutex_asynch_rxtx);
  pthread_cond_signal(&proc->cond_asynch_rxtx);

  
  // if this is a slave RRU, try to synchronize on the DL frequency
  if ((ru->is_slave) && (ru->RU_if_in == LOCAL_RF)) do_rru_synch(ru);
 
  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  while (!oai_exit) {

    // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
    // They are set on the first rx/tx in the underly FH routines.
    if (subframe==9) { 
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }      

    LOG_D(PHY,"RU thread %p (proc %p), frame %d (%p), subframe %d (%p)\n",
	  pthread_self(), proc, frame,&frame,subframe,&subframe);
 
    // synchronization on input FH interface, acquire signals/data and block
    if (ru->fh_south_in) ru->rx_south_in(ru,&frame,&subframe);
    else AssertFatal(1==0, "No fronthaul interface at south port");

    // do RX front-end processing (frequency-shift, dft) if needed
    if (ru->fep_rx) ru->fep_rx(ru);

    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));

    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    wakeup_slaves(proc);

    // wakeup all eNB processes waiting for this RU
    if (ru->eNB_list) wakeup_eNBs(ru);

    // wait until eNBs are finished subframe RX n and TX n+4
    wait_on_condition(&proc->mutex_eNBs,&proc->cond_eNBs,&proc->instance_cnt_eNBs,"ru_thread");

    // do TX front-end processing if needed (precoding and/or IDFTs)
    if (ru->fep_tx) ru->fep_tx(ru);

    // do outgoing fronthaul if needed
    if (ru->fh_south_out) ru->fh_south_out(ru,&frame,&subframe);
  }
  

  printf( "Exiting ru_thread \n");
 
  ru_thread_status = 0;
  return &ru_thread_status;

}


// This thread run the initial synchronization like a UE
void *ru_thread_synch(void *arg) {

  RU_t *ru = (RU_t*)arg;
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;
  int32_t sync_pos,sync_pos2;
  uint32_t peak_val;
  uint32_t sync_corr[307200] __attribute__((aligned(32)));



  thread_top_init("ru_thread_synch",0,5000000,10000000,10000000);

  wait_sync("ru_thread_synch");

  // initialize variables for PSS detection
  lte_sync_time_init(&eNB->frame_parms);

  while (!oai_exit) {

    // wait to be woken up
    if (wait_on_condition(&ru->proc.mutex_synch,&ru->proccont_synch,&ru->proc.instance_cnt_synch,"ru_thread_synch")<0) break;

    // if we're not in synch, then run initial synch
    if (ru->in_synch == 0) { 
      // run intial synch like UE
      LOG_I(PHY,"Running initial synchronization\n");
      
      sync_pos = lte_sync_time_eNB(ru->ru_time.rxdata[0],
				   fp,
				   fp->samples_per_tti*5,
				   &peak_val,
				   sync_corr);
      LOG_I(PHY,"RU synch: %d, val %d\n",sync_pos,peak_val);

      if (sync_pos >= 0) {
	if (sync_pos >= fp->nb_prefix_samples)
	  sync_pos2 = sync_pos - fp->nb_prefix_samples;
	else
	  sync_pos2 = sync_pos + (fp->samples_per_tti*10) - fp->nb_prefix_samples;
	
	if (fp->frame_type == FDD) {
	  
	  // PSS is hypothesized in last symbol of first slot in Frame
	  int sync_pos_slot = (fp->samples_per_tti>>1) - fp->ofdm_symbol_size - fp->nb_prefix_samples;
	  
	  if (sync_pos2 >= sync_pos_slot)
	    ru->rx_offset = sync_pos2 - sync_pos_slot;
	  else
	    ru->rx_offset = (fp->samples_per_tti*10) + sync_pos2 - sync_pos_slot;
	}
	else {
	  
	}

	LOG_I(PHY,"Estimated sync_pos %d, peak_val %d => timing offset %d\n",sync_pos,peak_val,ru->rx_offset);
	
	/*
	if ((peak_val > 300000) && (sync_pos > 0)) {
	//      if (sync_pos++ > 3) {
	write_output("ru_sync.m","sync",(void*)&sync_corr[0],fp->samples_per_tti*5,1,2);
	write_output("ru_rx.m","rxs",(void*)ru->ru_time.rxdata[0][0],fp->samples_per_tti*10,1,1);
	exit(-1);
	}
	*/
	ru->in_synch=1;
      }
    }

    if (release_thread(&ru->proc.mutex_synch,&ru->proc.instance_cnt_synch,"ru_synch_thread") < 0) break;
  } // oai_exit

  lte_sync_time_free();

}

int wakeup_synch(RU_t *ru){

  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  // wake up synch thread
  // lock the synch mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&ru->proc.mutex_synch,&wait) != 0) {
    LOG_E( PHY, "[RU] ERROR pthread_mutex_lock for RU synch thread (IC %d)\n", ru->proc.instance_cnt_synch );
    exit_fun( "error locking mutex_synch" );
    return(-1);
  }
  
  ++RU->proc.instance_cnt_synch;
  
  // the thread can now be woken up
  if (pthread_cond_signal(&ru->proc.cond_synch) != 0) {
    LOG_E( PHY, "[RU] ERROR pthread_cond_signal for RU synch thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &RU->proc.mutex_synch );

  return(0);
}

 
int start_if(RU_t *ru) {
  return(ru->ifdevice.trx_start_func(&ru->ifdevice));
}

int start_rf(RU_t *ru) {
  return(ru->rfdevice.trx_start_func(&ru->rfdevice));
}

extern void ru_fep_if5(PHY_VARS_eNB *eNB);
extern void ru_fep_full(PHY_VARS_eNB *eNB);
extern void ru_fep_full_2thread(PHY_VARS_eNB *eNB);
extern void do_prach(PHY_VARS_eNB *eNB,RU_t *ru);

void init_RU(eNB_func_t node_function, RU_if_in_t ru_if_in[], RU_if_timing_t ru_if_timing[], eth_params_t *eth_params) {
  
  int ru_id;

  for (ru_id=0;ru_id<rc->nb_RU;ru_id++) {
    ru               = &RC.ru_desc[ru_id];
    ru->RU_if_in     = ru_if_in[ru_id];
    ru->RU_if_timing = ru_if_timing[ru_id];
    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s)\n",ru_id,ru_if_types[ru_if_in[ru_id]],eNB_timing[ru_timing[ru_id]]);
    
    switch (ru->RU_if_in) {
    case LOCAL_RF:   // this is an RU with integrated RF (RRU, eNB)
      if (node_function ==  NGFI_RRU_IF5) {                 // IF5 RRU
	ru->do_prach              = NULL;                   // no prach processing
	ru->fh_north_in           = NULL;                   // no synchronous incoming fronthaul from north
	ru->fh_north_out          = fh_if5_north_out        // need only to do send_IF5 on reception
	ru->fh_north_asynch_in    = fh_if5_north_in;        // TX packets come asynchronously 
	ru->fep_rx                = NULL;                   // nothing (this is a time-domain signal)
	ru->fep_tx                = NULL;                   // nothing (this is a time-domain signal)
	ru->start_if              = start_if;               // need to start the if interface for if5
	ru->ifdevice.host_type    = RRH_HOST;
	ru->rfdevice.host_type    = RRH_HOST;
      }
      else if (node_function == NGFI_RRU_IF4p5) {
	ru->do_prach              = do_prach;               // IF4p5 needs to do part of prach processing in RRU
	ru->fh_north_in           = NULL;                   // no synchronous incoming fronthaul from north
	ru->fh_north_out          = fh_if4p5_north_out;     // send_IF4p5 on reception
	ru->fh_north_asynch_in    = fh_if4p5_north_in;      // TX packets come asynchronously
	ru->fep_rx                = fep_dft;                // RX DFTs
	ru->fep_tx                = fep_idft;               // this is fep with idft only (no precoding in RRU)
	ru->start_if              = start_if;               // need to start the if interface for if4p5
	ru->ifdevice.host_type    = RRH_HOST;
	ru->rfdevice.host_type    = RRH_HOST;
      }
      else if (node_function == eNodeB_3GPP) {              
	ru->do_prach             = NULL;                    // prach is done completely in eNB processing
	ru->fep_rx               = fep_dft;                 // RX DFTs
	ru->fep_tx               = fep_idft_prec;           // this is fep with idft and precoding
	ru->fh_north_in          = NULL;                    // no incoming fronthaul from north
	ru->fh_north_out         = NULL;                    // no outgoing fronthaul to north
	ru->start_if             = NULL;                    // no if interface
	ru->rfdevice.host_type   = BBU_HOST;
      }
      ru->fh_south_in            = rx_rf;                               // local synchronous RF RX
      ru->fh_south_out           = NULL;                                // nothing connected directly to radio
      ru->start_rf               = start_rf;                            // need to start the local RF interface

      ret = openair0_device_load(&ru->rfdevice, &openair0_cfg[ru_id]);
      if (setup_RU_buffers(rc,ru_id,&openair0_cfg[ru_id])!=0) {
	printf("Exiting, cannot initialize eNodeB Buffers\n");
	exit(-1);
      }
      break;

    case REMOTE_IF5: // the remote unit is IF5 RRU
      ru->do_prach              = NULL;                       // no prach processing in RU
      ru->fep_rx                = fep_dft;                    // this is frequency-shift + DFTs
      ru->fep_tx                = fep_prec_idft;              // need to do transmit Precoding + IDFTs 
      if (ru->RU_if_timing == synch_to_other) {
	ru->fh_south_in         = fh_slave_south_in;          // synchronize to master
	ru->fh_south_out        = fh_if5_mobipass_south_out;  // use send_IF5 for mobipass
	ru->fh_south_asynch_in  = fh_if5_asynch_UL;    // UL is asynchronous
      }
      else {
	ru->fh_south_in         = fh_if5_south_in;     // synchronous IF5 reception
	ru->fh_south_out        = fh_if5_south_out;    // synchronous IF5 transmission
	ru->fh_south_asynch_in  = NULL;                // no asynchronous UL
      }
      ru->start_rf             = NULL;                 // no local RF
      ru->start_if             = start_if;             // need to start if interface for IF5 
      ru->ifdevice.host_type   = BBU_HOST;

      ret = openair0_transport_load(&ru->ifdevice, &openair0_cfg[ru_id], (eth_params+ru_id));
      printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
      if (ret<0) {
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
      break;

    case REMOTE_IF4p5:
      ru->do_prach              = NULL;                // no prach processing in RU
      ru->fep_rx                = NULL;                // DFTs
      ru->fep_tx                = fep_prec;            // need to do transmit Precoding (no IDFTs)
      ru->fh_south_in           = fh_if4p5_south_in;   // synchronous IF4p5 reception
      ru->fh_south_out          = fh_if4p5_south_out;  // synchronous IF4p5 transmission
      ru->fh_south_asynch_in    = (ru->RU_if_timing == synch_to_other) ? fh_if4p5_south_in : NULL;                // asynchronous UL if synch_to_other
      
      ru->start_rf              = NULL;                // no local RF
      ru->start_if              = start_if;            // need to start if interface for IF4p5 
      ru->fh_asynch             = fh_if5_asynch_DL;
      ru->ifdevice.host_type    = BBU_HOST;

      ret = openair0_transport_load(&ru->ifdevice, &openair0_cfg[ru_id], (eth_params+ru_id));
      printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
      if (ret<0) {
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
      
      malloc_IF4p5_buffer(eNB);
      
      break;

    case REMOTE_IF1pp:
      LOG_E(PHY,"RU with IF1pp not supported yet\n");
      break;

    } // switch on interface type 

  } // for ru_id

  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] eNB threads created\n");
  

}




void stop_ru(RU_t *ru) {

  printf("Stopping RU %p processing threads\n",(void*)ru);
  
}
