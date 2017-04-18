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
#include "../../ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"

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


void init_RU(const char*);
void stop_RU(RU_t *ru);
void do_ru_sync(RU_t *ru);



/*************************************************************/
/* Southbound Fronthaul functions, RCC/RAU                   */

// southbound IF5 fronthaul for 16-bit OAI format
static inline void fh_if5_south_out(RU_t *ru) {
  if (ru == RC.ru[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  send_IF5(ru, ru->proc.timestamp_tx, ru->proc.subframe_tx, &ru->seqno, IF5_RRH_GW_DL);
}

// southbound IF5 fronthaul for Mobipass packet format
static inline void fh_if5_mobipass_south_out(RU_t *ru) {
  if (ru == RC.ru[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  send_IF5(ru, ru->proc.timestamp_tx, ru->proc.subframe_tx, &ru->seqno, IF5_MOBIPASS); 
}

// southbound IF4p5 fronthaul
static inline void fh_if4p5_south_out(RU_t *ru) {
  if (ru == RC.ru[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  LOG_I(PHY,"Sending IF4p5 for frame %d subframe %d\n",ru->proc.frame_tx,ru->proc.subframe_tx);
  send_IF4p5(ru,ru->proc.frame_tx, ru->proc.subframe_tx, IF4p5_PDLFFT, 0);
}

/*************************************************************/
/* Input Fronthaul from south RCC/RAU                        */

// Synchronous if5 from south 
void fh_if5_south_in(RU_t *ru,int *frame, int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  RU_proc_t *proc = &ru->proc;

  recv_IF5(ru, &proc->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

  proc->frame_rx    = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx = (proc->timestamp_rx / fp->samples_per_tti)%10;
  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
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
  RU_proc_t *proc = &ru->proc;
  int f,sf;


  uint16_t packet_type;
  uint32_t symbol_number=0;
  uint32_t symbol_mask_full;
  int      prach_received=0;

  if ((fp->frame_type == TDD) && (subframe_select(fp,*subframe)==SF_S))  
    symbol_mask_full = (1<<fp->ul_symbols_in_S_subframe)-1;   
  else     
    symbol_mask_full = (1<<fp->symbols_per_tti)-1; 


  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &f, &sf, &packet_type, &symbol_number);

    if (packet_type == IF4p5_PULFFT) {
      proc->symbol_mask[sf] = proc->symbol_mask[sf] | (1<<symbol_number);

    } else if (packet_type == IF4p5_PULTICK) {           
      if ((proc->first_rx==0) && (f!=*frame)) 	
	LOG_E(PHY,"rx_fh_if4p5: PULTICK received frame %d != expected %d\n",f,*frame);       
      if ((proc->first_rx==0) && (sf!=*subframe)) 	
	LOG_E(PHY,"rx_fh_if4p5: PULTICK received subframe %d != expected %d (first_rx %d)\n",sf,*subframe,proc->first_rx);       
      break;     
    } else if (packet_type == IF4p5_PRACH) {
      if (ru->do_prach==1) {
	proc->subframe_prach = sf;
	proc->frame_prach    = f;
	do_prach_ru(ru);
      }
    }

  } while(proc->symbol_mask[*subframe] != symbol_mask_full);    

  //caculate timestamp_rx, timestamp_tx based on frame and subframe
  proc->subframe_rx = sf;
  proc->frame_rx    = f;
  proc->timestamp_rx = ((proc->frame_rx * 10)  + proc->subframe_rx ) * fp->samples_per_tti ;
  proc->timestamp_tx = proc->timestamp_rx +  (4*fp->samples_per_tti);
  proc->subframe_tx = (sf+4)%10;
  proc->frame_tx    = (sf>5) ? (f+1)&1023 : f;

 
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

  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU, f );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_RU, sf );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, proc->frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_RU, proc->subframe_tx );
  }

  proc->symbol_mask[sf] = 0;  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  
}

// Dummy FH from south for getting synchronization from master RU
void fh_slave_south_in(RU_t *ru,int *frame,int *subframe) {
  // This case is for synchronization to another thread
  // it just waits for an external event.  The actual rx_fh is handle by the asynchronous RX thread
  RU_proc_t *proc=&ru->proc;

  if (wait_on_condition(&proc->mutex_FH,&proc->cond_FH,&proc->instance_cnt_FH,"fh_slave_south_in") < 0)
    return;

  release_thread(&proc->mutex_FH,&proc->instance_cnt_FH,"rx_fh_slave_south_in");

  
}

// asynchronous inbound if5 fronthaul from south (Mobipass)
void fh_if5_south_asynch_in_mobipass(RU_t *ru,int *frame,int *subframe) {

  RU_proc_t *proc       = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;

  recv_IF5(ru, &proc->timestamp_rx, *subframe, IF5_MOBIPASS); 
  pthread_mutex_lock(&proc->mutex_asynch_rxtx);
  int offset_mobipass = 40120;
  pthread_mutex_lock(&proc->mutex_asynch_rxtx);
  proc->subframe_rx = ((proc->timestamp_rx-offset_mobipass)/fp->samples_per_tti)%10;
  proc->frame_rx    = ((proc->timestamp_rx-offset_mobipass)/(fp->samples_per_tti*10))&1023;

  proc->subframe_rx = (proc->timestamp_rx/fp->samples_per_tti)%10;
  proc->frame_rx    = (proc->timestamp_rx/(10*fp->samples_per_tti))&1023;

  if (proc->first_rx == 1) {
    proc->first_rx =2;
    *subframe = proc->subframe_rx;
    *frame    = proc->frame_rx; 
    LOG_E(PHY,"[Mobipass]timestamp_rx:%llu, frame_rx %d, subframe: %d\n",(unsigned long long int)proc->timestamp_rx,proc->frame_rx,proc->subframe_rx);
  }
  else {
    if (proc->subframe_rx != *subframe) {
        proc->first_rx++;
	LOG_E(PHY,"[Mobipass]timestamp:%llu, subframe_rx %d is not what we expect %d, first_rx:%d\n",(unsigned long long int)proc->timestamp_rx, proc->subframe_rx,*subframe, proc->first_rx);
      //exit_fun("Exiting");
    }
    if (proc->frame_rx != *frame) {
        proc->first_rx++;
       LOG_E(PHY,"[Mobipass]timestamp:%llu, frame_rx %d is not what we expect %d, first_rx:%d\n",(unsigned long long int)proc->timestamp_rx,proc->frame_rx,*frame, proc->first_rx);  
     // exit_fun("Exiting");
    }
    // temporary solution
      *subframe = proc->subframe_rx;
      *frame    = proc->frame_rx;
  }

  pthread_mutex_unlock(&proc->mutex_asynch_rxtx);


} // eNodeB_3GPP_BBU 

// asynchronous inbound if4p5 fronthaul from south
void fh_if4p5_south_asynch_in(RU_t *ru,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  RU_proc_t *proc       = &ru->proc;

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
void fh_if4p5_north_in(RU_t *ru,int *frame,int *subframe) {

  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;
  uint16_t packet_type;


  /// **** incoming IF4p5 from remote RCC/RAU **** ///             
  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<ru->frame_parms.symbols_per_tti)-1;
  
  do { 
    recv_IF4p5(ru, frame, subframe, &packet_type, &symbol_number);
    symbol_mask = symbol_mask | (1<<symbol_number);
  } while (symbol_mask != symbol_mask_full); 

  // dump VCD output for first RU in list
  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, *frame );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_RU, *subframe );
  }
}

void fh_if5_north_asynch_in(RU_t *ru,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  RU_proc_t *proc        = &ru->proc;
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
  RU_proc_t *proc        = &ru->proc;

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

  proc->subframe_tx = subframe_tx;

  if (ru->feptx_ofdm) ru->feptx_ofdm(ru);
  if (ru->fh_south_out) ru->fh_south_out(ru);
} 

void fh_if5_north_out(RU_t *ru) {

  RU_proc_t *proc=&ru->proc;
  uint8_t seqno=0;

  /// **** send_IF5 of rxdata to BBU **** ///       
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 1 );  
  send_IF5(ru, proc->timestamp_rx, proc->subframe_rx, &seqno, IF5_RRH_GW_UL);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 0 );          

}

// RRU IF4p5 northbound interface (RX)
void fh_if4p5_north_out(RU_t *ru) {

  RU_proc_t *proc=&ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  const int subframe     = proc->subframe_rx;
  if (ru->idx==0) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_RU, proc->subframe_rx );

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) {
    /// **** in TDD during DL send_IF4 of ULTICK to RCC **** ///
    send_IF4p5(ru, proc->frame_rx, proc->subframe_rx, IF4p5_PULTICK, 0);
    return;
  }
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 1 ); 

  AssertFatal(ru->feprx!=NULL,"No northbound FEP function, exiting\n");
  if (ru->feprx) { 
    LOG_D(PHY,"Doing FEP/IF4p5 for frame %d, subframe %d\n",proc->frame_rx,proc->subframe_rx);
    ru->feprx(ru);
    send_IF4p5(ru, proc->frame_rx, proc->subframe_rx, IF4p5_PULFFT, 0);
  }

  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_RU_FEPRX, 0 );
}
void rx_rf(RU_t *ru,int *frame,int *subframe) {

  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *rxp[fp->nb_antennas_rx];
  unsigned int rxs;
  int i;

    
  for (i=0; i<fp->nb_antennas_rx; i++)
    rxp[i] = (void*)&ru->common.rxdata[i][*subframe*fp->samples_per_tti];
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );

  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
				   &(proc->timestamp_rx),
				   rxp,
				   fp->samples_per_tti,
				   fp->nb_antennas_rx);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
 
  if (proc->first_rx == 1)
    ru->ts_offset = proc->timestamp_rx;
 
  proc->frame_rx     = ((proc->timestamp_rx-ru->ts_offset) / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx  = ((proc->timestamp_rx-ru->ts_offset) / fp->samples_per_tti)%10;
  // synchronize first reception to frame 0 subframe 0

  proc->timestamp_tx = proc->timestamp_rx+(4*fp->samples_per_tti);
  proc->subframe_tx  = (proc->subframe_rx+4)%10;
  proc->frame_tx     = (proc->subframe_rx>5) ? (proc->frame_rx+1)&1023 : proc->frame_rx;

  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",(long long unsigned int)proc->timestamp_rx,proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",(long long unsigned int)proc->timestamp_rx,proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  //printf("timestamp_rx %lu, frame %d(%d), subframe %d(%d)\n",ru->timestamp_rx,proc->frame_rx,frame,proc->subframe_rx,subframe);
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  
  if (rxs != fp->samples_per_tti)
    exit_fun( "problem receiving samples" );
  

  
}


void tx_rf(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *txp[fp->nb_antennas_tx]; 
  unsigned int txs;
  int i;

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (proc->timestamp_tx-ru->openair0_cfg.tx_sample_advance)&0xffffffff );
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
  // prepare tx buffer pointers
  
  for (i=0; i<fp->nb_antennas_tx; i++)
    txp[i] = (void*)&ru->common.txdata[i][proc->subframe_tx*fp->samples_per_tti];
  
  txs = ru->rfdevice.trx_write_func(&ru->rfdevice,
				    proc->timestamp_tx-ru->openair0_cfg.tx_sample_advance,
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


/*!
 * \brief The Asynchronous RX/TX FH thread of RAU/RCC/eNB/RRU.
 * This handles the RX FH for an asynchronous RRU/UE
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* ru_thread_asynch_rxtx( void* param ) {

  static int ru_thread_asynch_rxtx_status;

  RU_t *ru         = (RU_t*)param;
  RU_proc_t *proc  = &ru->proc;



  int subframe=0, frame=0; 

  thread_top_init("ru_thread_asynch_rxtx",1,870000L,1000000L,1000000L);

  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe

  wait_sync("ru_thread_asynch_rxtx");

  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe
  printf( "waiting for devices (ru_thread_asynch_rx)\n");

  wait_on_condition(&proc->mutex_asynch_rxtx,&proc->cond_asynch_rxtx,&proc->instance_cnt_asynch_rxtx,"thread_asynch");

  printf( "devices ok (ru_thread_asynch_rx)\n");


  while (!oai_exit) { 
   
    if (oai_exit) break;   

    if (subframe==9) { 
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }      
    LOG_I(PHY,"ru_thread_asynch_rxtx: Waiting on incoming fronthaul\n");
    // asynchronous receive from south (Mobipass)
    if (ru->fh_south_asynch_in) ru->fh_south_asynch_in(ru,&frame,&subframe);
    // asynchronous receive from north (RRU IF4/IF5)
    else if (ru->fh_north_asynch_in) ru->fh_north_asynch_in(ru,&frame,&subframe);
    else AssertFatal(1==0,"Unknown function in ru_thread_asynch_rxtx\n");
  }

  ru_thread_asynch_rxtx_status=0;
  return(&ru_thread_asynch_rxtx_status);
}




void wakeup_slaves(RU_proc_t *proc) {

  int i;
  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  
  for (i=0;i<proc->num_slaves;i++) {
    RU_proc_t *slave_proc = proc->slave_proc[i];
    // wake up slave FH thread
    // lock the FH mutex and make sure the thread is ready
    if (pthread_mutex_timedlock(&slave_proc->mutex_FH,&wait) != 0) {
      LOG_E( PHY, "ERROR pthread_mutex_lock for RU %d slave %d (IC %d)\n",proc->ru->idx,slave_proc->ru->idx,slave_proc->instance_cnt_FH);
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
	LOG_E( PHY, "ERROR pthread_cond_signal for RU %d, slave RU %d\n",proc->ru->idx,slave_proc->ru->idx);
          exit_fun( "ERROR pthread_cond_signal" );
	  break;
      }
    } else {
      LOG_W( PHY,"[RU] Frame %d, slave %d thread busy!! (cnt_FH %i)\n",slave_proc->frame_rx,slave_proc->ru->idx, cnt_slave);
      exit_fun( "FH thread busy" );
      break;
    }             
  }
}

/*!
 * \brief The prach receive thread of RU.
 * \param param is a \ref RU_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* ru_thread_prach( void* param ) {

  static int ru_thread_prach_status;

  RU_t *ru        = (RU_t*)param;
  RU_proc_t *proc = (RU_proc_t*)&ru->proc;

  // set default return value
  ru_thread_prach_status = 0;

  thread_top_init("ru_thread_prach",1,500000L,1000000L,20000000L);

  while (!oai_exit) {
    
    if (oai_exit) break;

    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"ru_prach_thread") < 0) break;
    
    rx_prach(NULL,
	     ru,
             NULL,
             NULL,
             proc->frame_prach,
             0);
    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"ru_prach_thread") < 0) break;
  }

  printf( "Exiting RU thread PRACH\n");

  ru_thread_prach_status = 0;
  return &ru_thread_prach_status;
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
  
  ++ru->proc.instance_cnt_synch;
  
  // the thread can now be woken up
  if (pthread_cond_signal(&ru->proc.cond_synch) != 0) {
    LOG_E( PHY, "[RU] ERROR pthread_cond_signal for RU synch thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &ru->proc.mutex_synch );

  return(0);
}

void do_ru_synch(RU_t *ru) {

  LTE_DL_FRAME_PARMS *fp  = &ru->frame_parms;
  RU_proc_t *proc         = &ru->proc;
  int i;
  void *rxp[2],*rxp2[2];
  int32_t dummy_rx[ru->nb_rx][fp->samples_per_tti] __attribute__((aligned(32)));
  int rxs;
  int ic;

  // initialize the synchronization buffer to the common_vars.rxdata
  for (int i=0;i<ru->nb_rx;i++)
    rxp[i] = &ru->common.rxdata[i][0];

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
    if (rxs != fp->samples_per_tti*10) LOG_E(PHY,"requested %d samples, got %d\n",fp->samples_per_tti*10,rxs);
 
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
      pthread_mutex_lock(&ru->proc.mutex_synch);
      ic = ru->proc.instance_cnt_synch;
      pthread_mutex_unlock(&ru->proc.mutex_synch);
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



void wakeup_eNBs(RU_t *ru) {

  int i;
  PHY_VARS_eNB **eNB_list = ru->eNB_list;

  if (ru->num_eNB==1) {
    // call eNB function directly

    char string[20];
    sprintf(string,"Incoming RU %d",ru->idx);
    LOG_D(PHY,"RU %d Waking up eNB\n",ru->idx);
    ru->eNB_top(eNB_list[0],ru->proc.frame_rx,ru->proc.subframe_rx,string);
  }
  else {

    for (i=0;i<ru->num_eNB;i++)
      if (ru->wakeup_rxtx(eNB_list[i],ru->proc.frame_rx,ru->proc.subframe_rx) < 0)
	LOG_E(PHY,"could not wakeup eNB rxtx process for subframe %d\n", ru->proc.subframe_rx);
  }
}

static inline int wakeup_prach(RU_t *ru) {

  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  if (pthread_mutex_timedlock(&ru->proc.mutex_prach,&wait) !=0) {
    LOG_E( PHY, "[RU] ERROR pthread_mutex_lock for RU prach thread (IC %d)\n", ru->proc.instance_cnt_prach);
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }
  ++ru->proc.instance_cnt_prach;
  ru->proc.frame_prach    = ru->proc.frame_rx;
  ru->proc.subframe_prach = ru->proc.subframe_rx;
  
  // the thread can now be woken up
  if (pthread_cond_signal(&ru->proc.cond_prach) != 0) {
    LOG_E( PHY, "[RU] ERROR pthread_cond_signal for RU prach thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &ru->proc.mutex_prach );

  return(0);
}

static void* ru_thread( void* param ) {

  static int ru_thread_status;

  RU_t *ru                = (RU_t*)param;
  RU_proc_t *proc         = &ru->proc;
  LTE_DL_FRAME_PARMS *fp  = &ru->frame_parms;

  int subframe=0, frame=0; 
  // set default return value
  ru_thread_status = 0;


  // set default return value
  thread_top_init("ru_thread",0,870000,1000000,1000000);

  LOG_I(PHY,"Starting RU %d (%s,%s),\n",ru->idx,eNB_functions[ru->function],eNB_timing[ru->if_timing]);


  // Start IF device if any
  if (ru->start_if) {
    LOG_I(PHY,"Starting IF interface for RU %d\n",ru->idx);
    if (ru->start_if(ru,NULL) != 0) {
      LOG_E(PHY,"Could not start the IF device\n");
    }
    else { // wakeup the top thread to configure RU parameters
      LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
      pthread_mutex_lock(&RC.ru_mutex);
      RC.ru_mask &= ~(1<<ru->idx);
      pthread_cond_signal(&RC.ru_cond);
      pthread_mutex_unlock(&RC.ru_mutex);
    }
  }

  wait_sync("ru_thread");


  // Start RF device if any
  if (ru->start_rf) {
    if (ru->start_rf(ru) != 0)
      LOG_E(HW,"Could not start the RF device\n");
    else LOG_I(PHY,"RU %d rf device ready\n",ru->idx);
  }
  else LOG_I(PHY,"RU %d no rf device\n",ru->idx);


  // if an asnych_rxtx thread exists
  // wakeup the thread because the devices are ready at this point
 
  if ((ru->fh_south_asynch_in)||(ru->fh_north_asynch_in)) {
    pthread_mutex_lock(&proc->mutex_asynch_rxtx);
    proc->instance_cnt_asynch_rxtx=0;
    pthread_mutex_unlock(&proc->mutex_asynch_rxtx);
    pthread_cond_signal(&proc->cond_asynch_rxtx);
  }
  else LOG_I(PHY,"RU %d no asynch_south interface\n",ru->idx);

  // if this is a slave RRU, try to synchronize on the DL frequency
  if ((ru->is_slave) && (ru->if_south == LOCAL_RF)) do_ru_synch(ru);


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

    LOG_D(PHY,"RU thread (proc %p), frame %d (%p), subframe %d (%p)\n",
	  proc, frame,&frame,subframe,&subframe);


    // synchronization on input FH interface, acquire signals/data and block
    if (ru->fh_south_in) ru->fh_south_in(ru,&frame,&subframe);
    else AssertFatal(1==0, "No fronthaul interface at south port");


    LOG_D(PHY,"RU thread (proc %p), received frame %d (%p), subframe %d (%p)\n",
	  proc, proc->frame_tx,&frame,subframe,&subframe);
 
    if ((ru->do_prach>0) && (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)>0))
      wakeup_prach(ru);



    // adjust for timing offset between RU
    if (ru->idx!=0) proc->frame_tx = (proc->frame_tx+proc->frame_offset)&1023;


    // do RX front-end processing (frequency-shift, dft) if needed
    if (ru->feprx) ru->feprx(ru);


    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));

    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    wakeup_slaves(proc);

    // wakeup all eNB processes waiting for this RU
    if (ru->num_eNB>0) wakeup_eNBs(ru);


    // wait until eNBs are finished subframe RX n and TX n+4
    wait_on_condition(&proc->mutex_eNBs,&proc->cond_eNBs,&proc->instance_cnt_eNBs,"ru_thread");


    // do TX front-end processing if needed (precoding and/or IDFTs)
    if (ru->feptx_prec) ru->feptx_prec(ru);
   
    // do OFDM if needed
    if ((ru->fh_north_asynch_in == NULL) && (ru->feptx_ofdm)) ru->feptx_ofdm(ru);
    // do outgoing fronthaul (south) if needed
    if ((ru->fh_north_asynch_in == NULL) && (ru->fh_south_out)) ru->fh_south_out(ru);



    if (ru->fh_north_out) ru->fh_north_out(ru);
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
  static int ru_thread_synch_status;


  thread_top_init("ru_thread_synch",0,5000000,10000000,10000000);

  wait_sync("ru_thread_synch");

  // initialize variables for PSS detection
  lte_sync_time_init(&ru->frame_parms);

  while (!oai_exit) {

    // wait to be woken up
    if (wait_on_condition(&ru->proc.mutex_synch,&ru->proc.cond_synch,&ru->proc.instance_cnt_synch,"ru_thread_synch")<0) break;

    // if we're not in synch, then run initial synch
    if (ru->in_synch == 0) { 
      // run intial synch like UE
      LOG_I(PHY,"Running initial synchronization\n");
      
      sync_pos = lte_sync_time_eNB(ru->common.rxdata,
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

  ru_thread_synch_status = 0;
  return &ru_thread_synch_status;

}


 
int start_if(struct RU_t_s *ru,struct PHY_VARS_eNB_s *eNB) {
  return(ru->ifdevice.trx_start_func(&ru->ifdevice));
}

int start_rf(RU_t *ru) {
  return(ru->rfdevice.trx_start_func(&ru->rfdevice));
}

extern void fep_full(RU_t *ru);
extern void fep_full_2thread(RU_t *ru);
extern void feptx_ofdm(RU_t *ru);
extern void feptx_prec(RU_t *ru);

void init_RU_proc(RU_t *ru) {
   
  int i=0;
  RU_proc_t *proc;
  pthread_attr_t *attr_FH=NULL,*attr_prach=NULL,*attr_asynch=NULL,*attr_synch=NULL;
  //pthread_attr_t *attr_fep=NULL;

  char name[100];

#ifndef OCP_FRAMEWORK
  LOG_I(PHY,"Initializing RU %d (%s,%s),\n",ru->idx,eNB_functions[ru->function],eNB_timing[ru->if_timing]);
#endif
  proc = &ru->proc;
  memset((void*)proc,0,sizeof(RU_proc_t));

  proc->ru = ru;
  proc->instance_cnt_prach       = -1;
  proc->instance_cnt_synch       = -1;
  proc->instance_cnt_FH          = -1;
  proc->instance_cnt_asynch_rxtx = -1;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->num_slaves               = 0;
  for (i=0;i<10;i++) proc->symbol_mask[i]=0;
  
  pthread_mutex_init( &proc->mutex_prach, NULL);
  pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);
  pthread_mutex_init( &proc->mutex_synch,NULL);
  pthread_mutex_init( &proc->mutex_FH,NULL);
  
  pthread_cond_init( &proc->cond_prach, NULL);
  pthread_cond_init( &proc->cond_FH, NULL);
  pthread_cond_init( &proc->cond_asynch_rxtx, NULL);
  pthread_cond_init( &proc->cond_synch,NULL);
  
  pthread_attr_init( &proc->attr_FH);
  pthread_attr_init( &proc->attr_prach);
  pthread_attr_init( &proc->attr_synch);
  pthread_attr_init( &proc->attr_asynch_rxtx);
  pthread_attr_init( &proc->attr_fep);
  
  
#ifndef DEADLINE_SCHEDULER
  attr_FH     = &proc->attr_FH;
  attr_prach  = &proc->attr_prach;
  attr_synch  = &proc->attr_synch;
  attr_asynch = &proc->attr_asynch_rxtx;
  //  attr_fep    = &proc->attr_fep;
#endif
  
  pthread_create( &proc->pthread_FH, attr_FH, ru_thread, (void*)ru );

  if (ru->function == NGFI_RRU_IF4p5) {
    pthread_create( &proc->pthread_prach, attr_prach, ru_thread_prach, (void*)ru );
  
    if (ru->is_slave == 1) pthread_create( &proc->pthread_synch, attr_synch, ru_thread_synch, (void*)ru);
    
    
    if ((ru->if_timing == synch_to_other) ||
	(ru->function == NGFI_RRU_IF5) ||
	(ru->function == NGFI_RRU_IF4p5))
      
      
      pthread_create( &proc->pthread_asynch_rxtx, attr_asynch, ru_thread_asynch_rxtx, (void*)ru );
    
    
    
    snprintf( name, sizeof(name), "ru_thread_FH %d", ru->idx );
    pthread_setname_np( proc->pthread_FH, name );
    
  }
  
  
}

/* this function maps the RU tx and rx buffers to the available rf chains.
   Each rf chain is is addressed by the card number and the chain on the card. The
   rf_map specifies for each antenna port, on which rf chain the mapping should start. Multiple
   antennas are mapped to successive RF chains on the same card. */
int setup_RU_buffers(RU_t *ru) {

  int i,j; 
  int card,ant;

  //uint16_t N_TA_offset = 0;

  LTE_DL_FRAME_PARMS *frame_parms;
  
  if (ru) {
    frame_parms = &ru->frame_parms;
    printf("setup_RU_buffers: frame_parms = %p\n",frame_parms);
  } else {
    printf("RU[%d] not initialized\n", ru->idx);
    return(-1);
  }
  
  /*
    if (frame_parms->frame_type == TDD) {
    if (frame_parms->N_RB_DL == 100)
    N_TA_offset = 624;
    else if (frame_parms->N_RB_DL == 50)
    N_TA_offset = 624/2;
    else if (frame_parms->N_RB_DL == 25)
    N_TA_offset = 624/4;
    }
  */
  
  
  if (ru->openair0_cfg.mmapped_dma == 1) {
    // replace RX signal buffers with mmaped HW versions
    
    for (i=0; i<ru->nb_rx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %d, rx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.rxdata[i]);
      ru->common.rxdata[i] = ru->openair0_cfg.rxbase[ru->rf_map.chain+ant];
      
      printf("rxdata[%d] @ %p\n",i,ru->common.rxdata[i]);
      for (j=0; j<16; j++) {
	printf("rxbuffer %d: %x\n",j,ru->common.rxdata[i][j]);
	ru->common.rxdata[i][j] = 16-j;
      }
    }
    
    for (i=0; i<ru->nb_tx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %d, tx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.txdata[i]);
      ru->common.txdata[i] = ru->openair0_cfg.txbase[ru->rf_map.chain+ant];
      
      printf("txdata[%d] @ %p\n",i,ru->common.txdata[i]);
      
      for (j=0; j<16; j++) {
	printf("txbuffer %d: %x\n",j,ru->common.txdata[i][j]);
	ru->common.txdata[i][j] = 16-j;
      }
    }
  }
  else {  // not memory-mapped DMA 
    //nothing to do, everything already allocated in lte_init
  }
  return(0);
}

// this is for RU with local RF unit
void fill_rf_config(RU_t *ru,const char *rf_config_file) {

  int i;

  LTE_DL_FRAME_PARMS *fp   = &ru->frame_parms;
  openair0_config_t *cfg   = &ru->openair0_cfg;

  if(fp->N_RB_DL == 100) {
    if (fp->threequarter_fs) {
      cfg->sample_rate=23.04e6;
      cfg->samples_per_frame = 230400; 
      cfg->tx_bw = 10e6;
      cfg->rx_bw = 10e6;
    }
    else {
      cfg->sample_rate=30.72e6;
      cfg->samples_per_frame = 307200; 
      cfg->tx_bw = 10e6;
      cfg->rx_bw = 10e6;
    }
  } else if(fp->N_RB_DL == 50) {
    cfg->sample_rate=15.36e6;
    cfg->samples_per_frame = 153600;
    cfg->tx_bw = 5e6;
    cfg->rx_bw = 5e6;
  } else if (fp->N_RB_DL == 25) {
    cfg->sample_rate=7.68e6;
    cfg->samples_per_frame = 76800;
    cfg->tx_bw = 2.5e6;
    cfg->rx_bw = 2.5e6;
  } else if (fp->N_RB_DL == 6) {
    cfg->sample_rate=1.92e6;
    cfg->samples_per_frame = 19200;
    cfg->tx_bw = 1.5e6;
    cfg->rx_bw = 1.5e6;
  }

  if (fp->frame_type==TDD)
    cfg->duplex_mode = duplex_mode_TDD;
  else //FDD
    cfg->duplex_mode = duplex_mode_FDD;

  cfg->Mod_id = 0;
  cfg->num_rb_dl=fp->N_RB_DL;
  cfg->tx_num_channels=ru->nb_tx;
  cfg->rx_num_channels=ru->nb_rx;
  
  for (i=0; i<ru->nb_tx; i++) {
    
    cfg->tx_freq[i] = (double)fp->dl_CarrierFreq;
    cfg->rx_freq[i] = (double)fp->ul_CarrierFreq;

    cfg->tx_gain[i] = (double)fp->att_tx;
    cfg->rx_gain[i] = (double)fp->att_rx;

    cfg->configFilename = rf_config_file;
    printf("channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
	   i, cfg->tx_gain[i],
	   cfg->rx_gain[i],
	   cfg->tx_freq[i],
	   cfg->rx_freq[i]);
  }
}

int check_capabilities(RU_t *ru,RRU_capabilities_t *cap) {

  FH_fmt_options_t fmt = cap->FH_fmt;

  int i;
  int found_band=0;

  LOG_I(PHY,"RRU %d, num_bands %d, looking for band %d\n",ru->idx,cap->num_bands,ru->frame_parms.eutra_band);
  for (i=0;i<cap->num_bands;i++) {
    LOG_I(PHY,"band %d on RRU %d\n",cap->band_list[i],ru->idx);
    if (ru->frame_parms.eutra_band == cap->band_list[i]) {
      found_band=1;
      break;
    }
  }

  if (found_band == 0) {
    LOG_I(PHY,"Couldn't find target EUTRA band %d on RRU %d\n",ru->frame_parms.eutra_band,ru->idx);
    return(-1);
  }

  switch (ru->if_south) {
  case LOCAL_RF:
    AssertFatal(1==0, "This RU should not have a local RF, exiting\n");
    return(0);
    break;
  case REMOTE_IF5:
    if (fmt == OAI_IF5_only || fmt == OAI_IF5_and_IF4p5) return(0);
    break;
  case REMOTE_IF4p5:
    if (fmt == OAI_IF4p5_only || fmt == OAI_IF5_and_IF4p5) return(0);
    break;
  case REMOTE_MBP_IF5:
    if (fmt == MBP_IF5) return(0);
    break;
  default:
    LOG_I(PHY,"No compatible Fronthaul interface found for RRU %d\n", ru->idx);
    return(-1);
  }

}


char rru_format_options[4][20] = {"OAI_IF5_only","OAI_IF4p5_only","OAI_IF5_and_IF4p5","MBP_IF5"};
char rru_formats[3][20] = {"OAI_IF5","MBP_IF5","OAI_IF4p5"};

void configure_ru(int idx,
		  void *arg) {

  RU_t               *ru           = RC.ru[idx];
  RRU_config_t       *config       = (RRU_config_t *)arg;
  RRU_capabilities_t *capabilities = (RRU_capabilities_t*)arg;
  int ret;

  LOG_I(PHY, "Received capabilities from RRU %d\n",idx);


  if (capabilities->FH_fmt < MAX_FH_FMTs) LOG_I(PHY, "RU FH options %s\n",rru_format_options[capabilities->FH_fmt]);
  if ((ret=check_capabilities(ru,capabilities)) == 0) {
    // Pass configuration to RRU
    LOG_I(PHY, "Using %s fronthaul, band %d \n",rru_formats[ru->if_south],ru->frame_parms.eutra_band);
    config->FH_fmt                 = ru->if_south;
    config->num_bands              = 1;
    config->band_list[0]           = ru->frame_parms.eutra_band;
    config->tx_freq[0]             = ru->frame_parms.dl_CarrierFreq;      
    config->rx_freq[0]             = ru->frame_parms.ul_CarrierFreq;      
    config->att_tx[0]              = ru->att_tx;
    config->att_rx[0]              = ru->att_rx;
    config->N_RB_DL[0]             = ru->frame_parms.N_RB_DL;
    config->N_RB_UL[0]             = ru->frame_parms.N_RB_UL;
    config->threequarter_fs[0]     = ru->frame_parms.threequarter_fs;
    if (ru->if_south==REMOTE_IF4p5) {
      config->prach_FreqOffset[0]  = ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
      config->prach_ConfigIndex[0] = ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
    }
    // take antenna capabilities of RRU
    ru->nb_tx                      = capabilities->nb_tx[0];
    ru->nb_rx                      = capabilities->nb_rx[0];
  }
  else {
    LOG_I(PHY,"Cannot configure RRU %d, check_capabilities returned %d\n", idx,ret);
  }

  init_frame_parms(&ru->frame_parms,1);
  phy_init_RU(ru);

  return(0);
}

int configure_rru(int idx,
		  void *arg) {

  RRU_config_t *config = (RRU_config_t *)arg;
  RU_t         *ru         = RC.ru[idx];

  ru->frame_parms.eutra_band                                               = config->band_list[0];
  ru->frame_parms.dl_CarrierFreq                                           = config->tx_freq[0];
  ru->frame_parms.ul_CarrierFreq                                           = config->rx_freq[0];
  ru->att_tx                                                               = config->att_tx[0];
  ru->att_rx                                                               = config->att_rx[0];
  ru->frame_parms.N_RB_DL                                                  = config->N_RB_DL[0];
  ru->frame_parms.N_RB_UL                                                  = config->N_RB_UL[0];
  ru->frame_parms.threequarter_fs                                          = config->threequarter_fs[0];
  ru->frame_parms.pdsch_config_common.referenceSignalPower                 = ru->max_pdschReferenceSignalPower-config->att_tx[0];
  if (ru->function==NGFI_RRU_IF4p5) {
    ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset  = config->prach_FreqOffset[0]; 
    ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex = config->prach_ConfigIndex[0]; 
  }
  
  init_frame_parms(&ru->frame_parms,1);


  phy_init_RU(ru);

  return(0);
}

void init_precoding_weights(PHY_VARS_eNB *eNB) {

  int layer,ru_id,aa,re,ue,tb;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  RU_t *ru;
  LTE_eNB_DLSCH_t *dlsch;

  // init precoding weigths
  for (ue=0;ue<NUMBER_OF_UE_MAX;ue++) {
    for (tb=0;tb<2;tb++) {
      dlsch = eNB->dlsch[ue][tb];
      for (layer=0; layer<4; layer++) {
	for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {
	  ru = RC.ru[ru_id];
	  dlsch->ue_spec_bf_weights[ru_id][layer] = (int32_t**)malloc16(ru->nb_tx*sizeof(int32_t*));
	  
	  for (aa=0; aa<ru->nb_tx; aa++) {
	    dlsch->ue_spec_bf_weights[ru_id][layer][aa] = (int32_t *)malloc16(fp->ofdm_symbol_size*sizeof(int32_t));
	    for (re=0;re<fp->ofdm_symbol_size; re++) {
	      dlsch->ue_spec_bf_weights[ru_id][layer][aa][re] = 0x00007fff;
	    }
	  }
	}
      }
    }
  }
}

void init_RU(const char *rf_config_file) {
  
  int ru_id;
  RU_t *ru;
  int ret;
  PHY_VARS_eNB *eNB0;
  int i;
  int CC_id;

  // create status mask
  RC.ru_mask = 0;
  pthread_mutex_init(&RC.ru_mutex,NULL);
  pthread_cond_init(&RC.ru_cond,NULL);

  // read in configuration file
  RCconfig_RU();

  for (i=0;i<RC.nb_inst;i++) 
    for (CC_id=0;CC_id<RC.nb_CC[i];CC_id++) RC.eNB[i][CC_id]->num_RU=0;

  for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {
    ru               = RC.ru[ru_id];
    ru->idx          = ru_id;              
    ru->ts_offset    = 0;
    // use eNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!

    
    eNB0             = ru->eNB_list[0];

    if (eNB0) memcpy((void*)&ru->frame_parms,(void*)&eNB0->frame_parms,sizeof(LTE_DL_FRAME_PARMS));

    // attach all RU to all eNBs in its list/
    for (i=0;i<ru->num_eNB;i++) {
      eNB0 = ru->eNB_list[i];
      eNB0->RU_list[eNB0->num_RU++] = ru;
    }
    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s,%d)\n",ru_id,ru_if_types[ru->if_south],eNB_timing[ru->if_timing],ru->function);

    
    switch (ru->if_south) {
    case LOCAL_RF:   // this is an RU with integrated RF (RRU, eNB)
      if (ru->function ==  NGFI_RRU_IF5) {                 // IF5 RRU
	ru->do_prach              = 0;                      // no prach processing in RU
	ru->fh_north_in           = NULL;                   // no synchronous incoming fronthaul from north
	ru->fh_north_out          = fh_if5_north_out;       // need only to do send_IF5  reception
	ru->fh_south_out          = tx_rf;                  // send output to RF
	ru->fh_north_asynch_in    = fh_if5_north_asynch_in; // TX packets come asynchronously 
	ru->feprx                 = NULL;                   // nothing (this is a time-domain signal)
	ru->feptx_ofdm            = NULL;                   // nothing (this is a time-domain signal)
	ru->feptx_prec            = NULL;                   // nothing (this is a time-domain signal)
	ru->start_if              = start_if;               // need to start the if interface for if5
	ru->ifdevice.host_type    = RRU_HOST;
	ru->rfdevice.host_type    = RRU_HOST;
	ru->ifdevice.eth_params   = &ru->eth_params;

	ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
	printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
	if (ret<0) {
	  printf("Exiting, cannot initialize transport protocol\n");
	  exit(-1);
	}
      }
      else if (ru->function == NGFI_RRU_IF4p5) {
	ru->do_prach              = 1;                        // do part of prach processing in RU
	ru->fh_north_in           = NULL;                     // no synchronous incoming fronthaul from north
	ru->fh_north_out          = fh_if4p5_north_out;       // send_IF4p5 on reception
	ru->fh_south_out          = tx_rf;                    // send output to RF
	ru->fh_north_asynch_in    = fh_if4p5_north_asynch_in; // TX packets come asynchronously
	ru->feprx                 = fep_full;                 // RX DFTs
	ru->feptx_ofdm            = feptx_ofdm;               // this is fep with idft only (no precoding in RRU)
	ru->feptx_prec            = NULL;
	ru->start_if              = start_if;                 // need to start the if interface for if4p5
	ru->ifdevice.host_type    = RRU_HOST;
	ru->rfdevice.host_type    = RRU_HOST;
	ru->ifdevice.eth_params   = &ru->eth_params;

	ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
	printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
	if (ret<0) {
	  printf("Exiting, cannot initialize transport protocol\n");
	  exit(-1);
	}
	malloc_IF4p5_buffer(ru);
      }
      else if (ru->function == eNodeB_3GPP) {  
	ru->do_prach             = 0;                       // no prach processing in RU            
	ru->feprx                = fep_full;                // RX DFTs
	ru->feptx_ofdm           = feptx_ofdm;              // this is fep with idft and precoding
	ru->feptx_prec           = feptx_prec;              // this is fep with idft and precoding
	ru->fh_north_in          = NULL;                    // no incoming fronthaul from north
	ru->fh_north_out         = NULL;                    // no outgoing fronthaul to north
	ru->start_if             = NULL;                    // no if interface
	ru->rfdevice.host_type   = RAU_HOST;
      }
      ru->fh_south_in            = rx_rf;                               // local synchronous RF RX
      ru->fh_south_out           = tx_rf;                               // local synchronous RF TX
      ru->start_rf               = start_rf;                            // need to start the local RF interface
      ru->ifdevice.configure_rru = configure_rru;
      fill_rf_config(ru,rf_config_file);
      

      ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
      if (setup_RU_buffers(ru)!=0) {
	printf("Exiting, cannot initialize RU Buffers\n");
	exit(-1);
      }
      break;

    case REMOTE_IF5: // the remote unit is IF5 RRU
      ru->do_prach               = 0;
      ru->feprx                  = fep_full;                   // this is frequency-shift + DFTs
      ru->feptx_prec             = feptx_prec;                 // need to do transmit Precoding + IDFTs 
      ru->feptx_ofdm             = feptx_ofdm;                 // need to do transmit Precoding + IDFTs 
      if (ru->if_timing == synch_to_other) {
	ru->fh_south_in          = fh_slave_south_in;                  // synchronize to master
	ru->fh_south_out         = fh_if5_mobipass_south_out;          // use send_IF5 for mobipass
	ru->fh_south_asynch_in   = fh_if5_south_asynch_in_mobipass;    // UL is asynchronous
      }
      else {
	ru->fh_south_in          = fh_if5_south_in;     // synchronous IF5 reception
	ru->fh_south_out         = fh_if5_south_out;    // synchronous IF5 transmission
	ru->fh_south_asynch_in   = NULL;                // no asynchronous UL
      }
      ru->start_rf               = NULL;                 // no local RF
      ru->start_if               = start_if;             // need to start if interface for IF5 
      ru->ifdevice.host_type     = RAU_HOST;
      ru->ifdevice.eth_params    = &ru->eth_params;
      ru->ifdevice.configure_rru = configure_ru;

      ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
      printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
      if (ret<0) {
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
      break;

    case REMOTE_IF4p5:
      ru->do_prach               = 0;
      ru->feprx                  = NULL;                // DFTs
      ru->feptx_prec             = feptx_prec;          // Precoding operation
      ru->feptx_ofdm             = NULL;                // no OFDM mod
      ru->fh_south_in            = fh_if4p5_south_in;   // synchronous IF4p5 reception
      ru->fh_south_out           = fh_if4p5_south_out;  // synchronous IF4p5 transmission
      ru->fh_south_asynch_in     = (ru->if_timing == synch_to_other) ? fh_if4p5_south_in : NULL;                // asynchronous UL if synch_to_other
      ru->fh_north_out           = NULL;
      ru->fh_north_asynch_in     = NULL;
      ru->start_rf               = NULL;                // no local RF
      ru->start_if               = start_if;            // need to start if interface for IF4p5 
      ru->ifdevice.host_type     = RAU_HOST;
      ru->ifdevice.eth_params    = &ru->eth_params;
      ru->ifdevice.configure_rru = configure_ru;

      ret = openair0_transport_load(&ru->ifdevice, &ru->openair0_cfg, &ru->eth_params);
      printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
      if (ret<0) {
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
      
      malloc_IF4p5_buffer(ru);
      
      break;

    default:
      LOG_E(PHY,"RU with invalid or unknown southbound interface type %d\n",ru->if_south);
      break;
    } // switch on interface type 

    LOG_I(PHY,"Starting ru_thread %d\n",ru_id);

    init_RU_proc(ru);



  } // for ru_id

  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] RU threads created\n");
  

}




void stop_ru(RU_t *ru) {

  printf("Stopping RU %p processing threads\n",(void*)ru);
  
}

