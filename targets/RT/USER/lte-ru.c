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

/* these variables have to be defined before including ENB_APP/enb_paramdef.h */
static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};

#include "ENB_APP/enb_paramdef.h"
#include "common/config/config_userapi.h"

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


extern void  phy_init_RU(RU_t*);
extern void  phy_free_RU(RU_t*);

void init_RU(char*,clock_source_t clock_source,clock_source_t time_source);
void stop_RU(int nb_ru);
void do_ru_sync(RU_t *ru);

void configure_ru(int idx,
		  void *arg);

void configure_rru(int idx,
		   void *arg);

int attach_rru(RU_t *ru);

int connect_rau(RU_t *ru);

extern uint16_t sf_ahead;

/*************************************************************/
/* Functions to attach and configure RRU                     */

extern void wait_eNBs(void);

char ru_states[6][9] = {"RU_IDLE","RU_CONFIG","RU_READY","RU_RUN","RU_ERROR","RU_SYNC"};

int send_tick(RU_t *ru){
  
  RRU_CONFIG_msg_t rru_config_msg;

  rru_config_msg.type = RAU_tick; 
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE;

  LOG_I(PHY,"Sending RAU tick to RRU %d\n",ru->idx);
  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
	      "RU %d cannot access remote radio\n",ru->idx);

  return 0;
}

int send_config(RU_t *ru, RRU_CONFIG_msg_t rru_config_msg){


  rru_config_msg.type = RRU_config;
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_config_t);

  LOG_I(PHY,"Sending Configuration to RRU %d (num_bands %d,band0 %d,txfreq %u,rxfreq %u,att_tx %d,att_rx %d,N_RB_DL %d,N_RB_UL %d,3/4FS %d, prach_FO %d, prach_CI %d\n",ru->idx,
	((RRU_config_t *)&rru_config_msg.msg[0])->num_bands,
	((RRU_config_t *)&rru_config_msg.msg[0])->band_list[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->tx_freq[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->rx_freq[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->att_tx[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->att_rx[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_DL[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_UL[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->threequarter_fs[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->prach_FreqOffset[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->prach_ConfigIndex[0]);


  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
	      "RU %d failed send configuration to remote radio\n",ru->idx);

  return 0;

}

int send_capab(RU_t *ru){

  RRU_CONFIG_msg_t rru_config_msg; 
  RRU_capabilities_t *cap;
  int i=0;

  rru_config_msg.type = RRU_capabilities; 
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_capabilities_t);
  cap                 = (RRU_capabilities_t*)&rru_config_msg.msg[0];
  LOG_I(PHY,"Sending Capabilities (len %d, num_bands %d,max_pdschReferenceSignalPower %d, max_rxgain %d, nb_tx %d, nb_rx %d)\n",
	(int)rru_config_msg.len,ru->num_bands,ru->max_pdschReferenceSignalPower,ru->max_rxgain,ru->nb_tx,ru->nb_rx);
  switch (ru->function) {
  case NGFI_RRU_IF4p5:
    cap->FH_fmt                                   = OAI_IF4p5_only;
    break;
  case NGFI_RRU_IF5:
    cap->FH_fmt                                   = OAI_IF5_only;
    break;
  case MBP_RRU_IF5:
    cap->FH_fmt                                   = MBP_IF5;
    break;
  default:
    AssertFatal(1==0,"RU_function is unknown %d\n",RC.ru[0]->function);
    break;
  }
  cap->num_bands                                  = ru->num_bands;
  for (i=0;i<ru->num_bands;i++) {
    LOG_I(PHY,"Band %d: nb_rx %d nb_tx %d pdschReferenceSignalPower %d rxgain %d\n",
	  ru->band[i],ru->nb_rx,ru->nb_tx,ru->max_pdschReferenceSignalPower,ru->max_rxgain);
    cap->band_list[i]                             = ru->band[i];
    cap->nb_rx[i]                                 = ru->nb_rx;
    cap->nb_tx[i]                                 = ru->nb_tx;
    cap->max_pdschReferenceSignalPower[i]         = ru->max_pdschReferenceSignalPower;
    cap->max_rxgain[i]                            = ru->max_rxgain;
  }
  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
	      "RU %d failed send capabilities to RAU\n",ru->idx);

  return 0;

}

int attach_rru(RU_t *ru) {
  
  ssize_t      msg_len,len;
  RRU_CONFIG_msg_t rru_config_msg;
  int received_capabilities=0;

  wait_eNBs();
  // Wait for capabilities
  while (received_capabilities==0) {
    
    memset((void*)&rru_config_msg,0,sizeof(rru_config_msg));
    rru_config_msg.type = RAU_tick; 
    rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE;
    LOG_I(PHY,"Sending RAU tick to RRU %d\n",ru->idx);
    AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
		"RU %d cannot access remote radio\n",ru->idx);

    msg_len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_capabilities_t);

    // wait for answer with timeout  
    if ((len = ru->ifdevice.trx_ctlrecv_func(&ru->ifdevice,
					     &rru_config_msg,
					     msg_len))<0) {
      LOG_I(PHY,"Waiting for RRU %d\n",ru->idx);     
    }
    else if (rru_config_msg.type == RRU_capabilities) {
      AssertFatal(rru_config_msg.len==msg_len,"Received capabilities with incorrect length (%d!=%d)\n",(int)rru_config_msg.len,(int)msg_len);
      LOG_I(PHY,"Received capabilities from RRU %d (len %d/%d, num_bands %d,max_pdschReferenceSignalPower %d, max_rxgain %d, nb_tx %d, nb_rx %d)\n",ru->idx,
	    (int)rru_config_msg.len,(int)msg_len,
	    ((RRU_capabilities_t*)&rru_config_msg.msg[0])->num_bands,
	    ((RRU_capabilities_t*)&rru_config_msg.msg[0])->max_pdschReferenceSignalPower[0],
	    ((RRU_capabilities_t*)&rru_config_msg.msg[0])->max_rxgain[0],
	    ((RRU_capabilities_t*)&rru_config_msg.msg[0])->nb_tx[0],
	    ((RRU_capabilities_t*)&rru_config_msg.msg[0])->nb_rx[0]);
      received_capabilities=1;
    }
    else {
      LOG_E(PHY,"Received incorrect message %d from RRU %d\n",rru_config_msg.type,ru->idx); 
    }
  }
  configure_ru(ru->idx,
	       (RRU_capabilities_t *)&rru_config_msg.msg[0]);
		    
  rru_config_msg.type = RRU_config;
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_config_t);
  LOG_I(PHY,"Sending Configuration to RRU %d (num_bands %d,band0 %d,txfreq %u,rxfreq %u,att_tx %d,att_rx %d,N_RB_DL %d,N_RB_UL %d,3/4FS %d, prach_FO %d, prach_CI %d)\n",ru->idx,
	((RRU_config_t *)&rru_config_msg.msg[0])->num_bands,
	((RRU_config_t *)&rru_config_msg.msg[0])->band_list[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->tx_freq[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->rx_freq[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->att_tx[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->att_rx[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_DL[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_UL[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->threequarter_fs[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->prach_FreqOffset[0],
	((RRU_config_t *)&rru_config_msg.msg[0])->prach_ConfigIndex[0]);


  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
	      "RU %d failed send configuration to remote radio\n",ru->idx);

  return 0;
}

int connect_rau(RU_t *ru) {

  RRU_CONFIG_msg_t   rru_config_msg;
  ssize_t	     msg_len;
  int                tick_received          = 0;
  int                configuration_received = 0;
  RRU_capabilities_t *cap;
  int                i;
  int                len;

  // wait for RAU_tick
  while (tick_received == 0) {

    msg_len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE;

    if ((len = ru->ifdevice.trx_ctlrecv_func(&ru->ifdevice,
					     &rru_config_msg,
					     msg_len))<0) {
      LOG_I(PHY,"Waiting for RAU\n");     
    }
    else {
      if (rru_config_msg.type == RAU_tick) {
	LOG_I(PHY,"Tick received from RAU\n");
	tick_received = 1;
      }
      else LOG_E(PHY,"Received erroneous message (%d)from RAU, expected RAU_tick\n",rru_config_msg.type);
    }
  }

  // send capabilities

  rru_config_msg.type = RRU_capabilities; 
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_capabilities_t);
  cap                 = (RRU_capabilities_t*)&rru_config_msg.msg[0];
  LOG_I(PHY,"Sending Capabilities (len %d, num_bands %d,max_pdschReferenceSignalPower %d, max_rxgain %d, nb_tx %d, nb_rx %d)\n",
	(int)rru_config_msg.len,ru->num_bands,ru->max_pdschReferenceSignalPower,ru->max_rxgain,ru->nb_tx,ru->nb_rx);
  switch (ru->function) {
  case NGFI_RRU_IF4p5:
    cap->FH_fmt                                   = OAI_IF4p5_only;
    break;
  case NGFI_RRU_IF5:
    cap->FH_fmt                                   = OAI_IF5_only;
    break;
  case MBP_RRU_IF5:
    cap->FH_fmt                                   = MBP_IF5;
    break;
  default:
    AssertFatal(1==0,"RU_function is unknown %d\n",RC.ru[0]->function);
    break;
  }
  cap->num_bands                                  = ru->num_bands;
  for (i=0;i<ru->num_bands;i++) {
    LOG_I(PHY,"Band %d: nb_rx %d nb_tx %d pdschReferenceSignalPower %d rxgain %d\n",
	  ru->band[i],ru->nb_rx,ru->nb_tx,ru->max_pdschReferenceSignalPower,ru->max_rxgain);
    cap->band_list[i]                             = ru->band[i];
    cap->nb_rx[i]                                 = ru->nb_rx;
    cap->nb_tx[i]                                 = ru->nb_tx;
    cap->max_pdschReferenceSignalPower[i]         = ru->max_pdschReferenceSignalPower;
    cap->max_rxgain[i]                            = ru->max_rxgain;
  }
  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
	      "RU %d failed send capabilities to RAU\n",ru->idx);

  // wait for configuration
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_config_t);
  while (configuration_received == 0) {

    if ((len = ru->ifdevice.trx_ctlrecv_func(&ru->ifdevice,
					     &rru_config_msg,
					     rru_config_msg.len))<0) {
      LOG_I(PHY,"Waiting for configuration from RAU\n");     
    }    
    else {
      LOG_I(PHY,"Configuration received from RAU  (num_bands %d,band0 %d,txfreq %u,rxfreq %u,att_tx %d,att_rx %d,N_RB_DL %d,N_RB_UL %d,3/4FS %d, prach_FO %d, prach_CI %d)\n",
	    ((RRU_config_t *)&rru_config_msg.msg[0])->num_bands,
	    ((RRU_config_t *)&rru_config_msg.msg[0])->band_list[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->tx_freq[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->rx_freq[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->att_tx[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->att_rx[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_DL[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_UL[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->threequarter_fs[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->prach_FreqOffset[0],
	    ((RRU_config_t *)&rru_config_msg.msg[0])->prach_ConfigIndex[0]);
      
      configure_rru(ru->idx,
		    (void*)&rru_config_msg.msg[0]);
      configuration_received = 1;
    }
  }
  return 0;
}
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
  LOG_D(PHY,"Sending IF4p5 for frame %d subframe %d\n",ru->proc.frame_tx,ru->proc.subframe_tx);
  if (subframe_select(&ru->frame_parms,ru->proc.subframe_tx)!=SF_UL) 
    send_IF4p5(ru,ru->proc.frame_tx, ru->proc.subframe_tx, IF4p5_PDLFFT);
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

  if ((fp->frame_type == TDD) && (subframe_select(fp,*subframe)==SF_S))  
    symbol_mask_full = (1<<fp->ul_symbols_in_S_subframe)-1;   
  else     
    symbol_mask_full = (1<<fp->symbols_per_tti)-1; 

  AssertFatal(proc->symbol_mask[*subframe]==0,"rx_fh_if4p5: proc->symbol_mask[%d] = %x\n",*subframe,proc->symbol_mask[*subframe]);
  do { 
    recv_IF4p5(ru, &f, &sf, &packet_type, &symbol_number);
    if (oai_exit == 1 || ru->cmd== STOP_RU) break;
    if (packet_type == IF4p5_PULFFT) proc->symbol_mask[sf] = proc->symbol_mask[sf] | (1<<symbol_number);
    else if (packet_type == IF4p5_PULTICK) {           
      if ((proc->first_rx==0) && (f!=*frame)) LOG_E(PHY,"rx_fh_if4p5: PULTICK received frame %d != expected %d (RU %d)\n",f,*frame, ru->idx);       
      if ((proc->first_rx==0) && (sf!=*subframe)) LOG_E(PHY,"rx_fh_if4p5: PULTICK received subframe %d != expected %d (first_rx %d)\n",sf,*subframe,proc->first_rx);       
      break;     
      
    } else if (packet_type == IF4p5_PRACH) {
      // nothing in RU for RAU
    }
    LOG_D(PHY,"rx_fh_if4p5: subframe %d symbol mask %x\n",*subframe,proc->symbol_mask[*subframe]);
  } while(proc->symbol_mask[*subframe] != symbol_mask_full);    

  //caculate timestamp_rx, timestamp_tx based on frame and subframe
  proc->subframe_rx  = sf;
  proc->frame_rx     = f;
  proc->timestamp_rx = ((proc->frame_rx * 10)  + proc->subframe_rx ) * fp->samples_per_tti ;
  //  proc->timestamp_tx = proc->timestamp_rx +  (4*fp->samples_per_tti);
  proc->subframe_tx  = (sf+sf_ahead)%10;
  proc->frame_tx     = (sf>(9-sf_ahead)) ? (f+1)&1023 : f;
 
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    if (ru->cmd != WAIT_RESYNCH && proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
    else if (ru->cmd == WAIT_RESYNCH && proc->frame_rx != *frame){
       ru->cmd=EMPTY;
       *frame=proc->frame_rx; 
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
  LOG_D(PHY,"RU %d: fh_if4p5_south_in returning ...\n",ru->idx);
  //  usleep(100);
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
  uint32_t symbol_number,symbol_mask,prach_rx;
  uint32_t got_prach_info=0;

  symbol_number = 0;
  symbol_mask   = (1<<fp->symbols_per_tti)-1;
  prach_rx      = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &proc->frame_rx, &proc->subframe_rx, &packet_type, &symbol_number);
    if (ru->cmd == STOP_RU) break;
    // grab first prach information for this new subframe
    if (got_prach_info==0) {
      prach_rx       = is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx);
      got_prach_info = 1;
    }
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
    if      (packet_type == IF4p5_PULFFT)       symbol_mask &= (~(1<<symbol_number));
    else if (packet_type == IF4p5_PRACH)        prach_rx    &= (~0x1);
#ifdef Rel14
    else if (packet_type == IF4p5_PRACH_BR_CE0) prach_rx    &= (~0x2);
    else if (packet_type == IF4p5_PRACH_BR_CE1) prach_rx    &= (~0x4);
    else if (packet_type == IF4p5_PRACH_BR_CE2) prach_rx    &= (~0x8);
    else if (packet_type == IF4p5_PRACH_BR_CE3) prach_rx    &= (~0x10);
#endif
  } while( (symbol_mask > 0) || (prach_rx >0));   // haven't received all PUSCH symbols and PRACH information 
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

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, proc->frame_tx );
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_RU, proc->subframe_tx );
  
  if (proc->first_tx != 0) {
    *subframe = subframe_tx;
    *frame    = frame_tx;
    proc->first_tx = 0;
  }
  else {
    AssertFatal(subframe_tx == *subframe,
                "subframe_tx %d is not what we expect %d\n",subframe_tx,*subframe);
    AssertFatal(frame_tx == *frame, 
                "frame_tx %d is not what we expect %d\n",frame_tx,*frame);
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
  symbol_mask_full = ((subframe_select(fp,*subframe) == SF_S) ? (1<<fp->dl_symbols_in_S_subframe) : (1<<fp->symbols_per_tti))-1;
  do {   
    recv_IF4p5(ru, &frame_tx, &subframe_tx, &packet_type, &symbol_number);
    if (ru->cmd == STOP_RU){
      LOG_E(PHY,"Got STOP_RU\n");
      pthread_mutex_lock(&proc->mutex_ru);
      proc->instance_cnt_ru = -1;
      pthread_mutex_unlock(&proc->mutex_ru);
      ru->cmd=STOP_RU;
      return;
    } 
    if ((subframe_select(fp,subframe_tx) == SF_DL) && (symbol_number == 0)) start_meas(&ru->rx_fhaul);
    LOG_D(PHY,"subframe %d (%d): frame %d, subframe %d, symbol %d\n",
	  *subframe,subframe_select(fp,*subframe),frame_tx,subframe_tx,symbol_number);
    if (proc->first_tx != 0) {
      *frame    = frame_tx;
      *subframe = subframe_tx;
      proc->first_tx = 0;
      symbol_mask_full = ((subframe_select(fp,*subframe) == SF_S) ? (1<<fp->dl_symbols_in_S_subframe) : (1<<fp->symbols_per_tti))-1;
    }
    else {
      AssertFatal(frame_tx == *frame,
	          "frame_tx %d is not what we expect %d\n",frame_tx,*frame);
      AssertFatal(subframe_tx == *subframe,
		  "subframe_tx %d is not what we expect %d\n",subframe_tx,*subframe);
    }
    if (packet_type == IF4p5_PDLFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
    }
    else AssertFatal(1==0,"Illegal IF4p5 packet type (should only be IF4p5_PDLFFT got %d\n",packet_type);
  } while (symbol_mask != symbol_mask_full);    

  if (subframe_select(fp,subframe_tx) == SF_DL) stop_meas(&ru->rx_fhaul);

  proc->subframe_tx  = subframe_tx;
  proc->frame_tx     = frame_tx;

  if ((frame_tx == 0)&&(subframe_tx == 0)) proc->frame_tx_unwrap += 1024;

  proc->timestamp_tx = ((((uint64_t)frame_tx + (uint64_t)proc->frame_tx_unwrap) * 10) + (uint64_t)subframe_tx) * (uint64_t)fp->samples_per_tti;

  LOG_D(PHY,"RU %d/%d TST %llu, frame %d, subframe %d\n",ru->idx,0,(long long unsigned int)proc->timestamp_tx,frame_tx,subframe_tx);
  // dump VCD output for first RU in list
  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_RU, subframe_tx );
  }


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

  LOG_D(PHY,"Sending IF4p5_PULFFT SFN.SF %d.%d\n",proc->frame_rx,proc->subframe_rx);
  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) {
    /// **** in TDD during DL send_IF4 of ULTICK to RCC **** ///
    send_IF4p5(ru, proc->frame_rx, proc->subframe_rx, IF4p5_PULTICK);
    return;
  }

  start_meas(&ru->tx_fhaul);
  send_IF4p5(ru, proc->frame_rx, proc->subframe_rx, IF4p5_PULFFT);
  stop_meas(&ru->tx_fhaul);

}

void rx_rf(RU_t *ru,int *frame,int *subframe) {

  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *rxp[ru->nb_rx];
  unsigned int rxs;
  int i;
  openair0_timestamp ts,old_ts;
  int resynch=0;
  
  for (i=0; i<ru->nb_rx; i++)
    rxp[i] = (void*)&ru->common.rxdata[i][*subframe*fp->samples_per_tti];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );

  old_ts = proc->timestamp_rx;

  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
				   &ts,
				   rxp,
				   fp->samples_per_tti,
				   ru->nb_rx);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );

  if (ru->cmd==RU_FRAME_RESYNCH) {
    LOG_I(PHY,"Applying frame resynch %d => %d\n",*frame,ru->cmdval);
    if (proc->frame_rx>ru->cmdval) ru->ts_offset += (proc->frame_rx - ru->cmdval)*fp->samples_per_tti*10;
    else ru->ts_offset -= (-proc->frame_rx + ru->cmdval)*fp->samples_per_tti*10;
    *frame = ru->cmdval;
    ru->cmd=EMPTY;
    resynch=1;
  }
 
  proc->timestamp_rx = ts-ru->ts_offset;

  //AssertFatal(rxs == fp->samples_per_tti,
  //"rx_rf: Asked for %d samples, got %d from USRP\n",fp->samples_per_tti,rxs);
  if (rxs != fp->samples_per_tti) LOG_E(PHY, "rx_rf: Asked for %d samples, got %d from USRP\n",fp->samples_per_tti,rxs);

  if (proc->first_rx == 1) {
    ru->ts_offset = proc->timestamp_rx;
    proc->timestamp_rx = 0;
  } 
  else if (resynch==0 && (proc->timestamp_rx - old_ts != fp->samples_per_tti)) {
    LOG_I(PHY,"rx_rf: rfdevice timing drift of %"PRId64" samples (ts_off %"PRId64")\n",proc->timestamp_rx - old_ts - fp->samples_per_tti,ru->ts_offset);
    ru->ts_offset += (proc->timestamp_rx - old_ts - fp->samples_per_tti);
    proc->timestamp_rx = ts-ru->ts_offset;
  }

  proc->frame_rx     = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx  = (proc->timestamp_rx / fp->samples_per_tti)%10;
  // synchronize first reception to frame 0 subframe 0

  if (ru->fh_north_asynch_in == NULL) {
     proc->timestamp_tx = proc->timestamp_rx+(sf_ahead*fp->samples_per_tti);
     proc->subframe_tx  = (proc->subframe_rx+sf_ahead)%10;
     proc->frame_tx     = (proc->subframe_rx>(9-sf_ahead)) ? (proc->frame_rx+1)&1023 : proc->frame_rx;
  } 
  LOG_D(PHY,"RU %d/%d TS %llu (off %d), frame %d, subframe %d\n",
	ru->idx, 
	0, 
	(unsigned long long int)proc->timestamp_rx,
	(int)ru->ts_offset,proc->frame_rx,proc->subframe_rx);

  // dump VCD output for first RU in list
  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU, proc->frame_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_RU, proc->subframe_rx );
    if (ru->fh_north_asynch_in == NULL) {
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, proc->frame_tx );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_RU, proc->subframe_tx );
    }
  }
  
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
    {
      //exit_fun( "problem receiving samples" );
      LOG_E(PHY, "problem receiving samples");
    }
}


void tx_rf(RU_t *ru) {

  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *txp[ru->nb_tx]; 
  unsigned int txs;
  int i;

  T(T_ENB_PHY_OUTPUT_SIGNAL, T_INT(0), T_INT(0), T_INT(proc->frame_tx), T_INT(proc->subframe_tx),
    T_INT(0), T_BUFFER(&ru->common.txdata[0][proc->subframe_tx * fp->samples_per_tti], fp->samples_per_tti * 4));

  lte_subframe_t SF_type     = subframe_select(fp,proc->subframe_tx%10);
  lte_subframe_t prevSF_type = subframe_select(fp,(proc->subframe_tx+9)%10);
  lte_subframe_t nextSF_type = subframe_select(fp,(proc->subframe_tx+1)%10);
  int sf_extension = 0;

  if ((SF_type == SF_DL) ||
      (SF_type == SF_S)) {
    
    
    int siglen=fp->samples_per_tti,flags=1;
    
    if (SF_type == SF_S) {
      siglen = fp->dl_symbols_in_S_subframe*(fp->ofdm_symbol_size+fp->nb_prefix_samples0);
      flags=3; // end of burst
    }
    if ((fp->frame_type == TDD) &&
	(SF_type == SF_DL)&&
	(prevSF_type == SF_UL) &&
	(nextSF_type == SF_DL)) { 
      flags = 2; // start of burst
      sf_extension = ru->N_TA_offset<<1;
    }
    
    if ((fp->frame_type == TDD) &&
	(SF_type == SF_DL)&&
	(prevSF_type == SF_UL) &&
	(nextSF_type == SF_UL)) {
      flags = 4; // start of burst and end of burst (only one DL SF between two UL)
      sf_extension = ru->N_TA_offset<<1;
    } 

    for (i=0; i<ru->nb_tx; i++)
      txp[i] = (void*)&ru->common.txdata[i][(proc->subframe_tx*fp->samples_per_tti)-sf_extension];

    
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (proc->timestamp_tx-ru->openair0_cfg.tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
    // prepare tx buffer pointers
    
    txs = ru->rfdevice.trx_write_func(&ru->rfdevice,
				      proc->timestamp_tx+ru->ts_offset-ru->openair0_cfg.tx_sample_advance-sf_extension,
				      txp,
				      siglen+sf_extension,
				      ru->nb_tx,
				      flags);
    
    LOG_D(PHY,"[TXPATH] RU %d tx_rf, writing to TS %llu, frame %d, unwrapped_frame %d, subframe %d\n",ru->idx,
	  (long long unsigned int)proc->timestamp_tx,proc->frame_tx,proc->frame_tx_unwrap,proc->subframe_tx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );
    
    
    AssertFatal(txs ==  siglen+sf_extension,"TX : Timeout (sent %d/%d)\n",txs, siglen);

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
  LOG_I(PHY, "waiting for devices (ru_thread_asynch_rxtx)\n");

  wait_on_condition(&proc->mutex_asynch_rxtx,&proc->cond_asynch_rxtx,&proc->instance_cnt_asynch_rxtx,"thread_asynch");

  LOG_I(PHY, "devices ok (ru_thread_asynch_rxtx)\n");


  while (!oai_exit) { 
   
    if (oai_exit) break;   

    if (ru->state != RU_RUN) {
      subframe=0;
      frame=0;
      usleep(1000);
    }
    else {
      if (subframe==9) { 
         subframe=0;
         frame++;
         frame&=1023;
       } else {
         subframe++;
       }      
       LOG_D(PHY,"ru_thread_asynch_rxtx: Waiting on incoming fronthaul\n");
       // asynchronous receive from south (Mobipass)
       if (ru->fh_south_asynch_in) ru->fh_south_asynch_in(ru,&frame,&subframe);
       // asynchronous receive from north (RRU IF4/IF5)
       else if (ru->fh_north_asynch_in) {
         if (subframe_select(&ru->frame_parms,subframe)!=SF_UL)
	   ru->fh_north_asynch_in(ru,&frame,&subframe);
       }
       else AssertFatal(1==0,"Unknown function in ru_thread_asynch_rxtx\n");
    }
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

  while (RC.ru_mask>0 && ru->function!=eNodeB_3GPP) {
    usleep(1e6);
    LOG_D(PHY,"%s() RACH waiting for RU to be configured\n", __FUNCTION__);
  }
  LOG_I(PHY,"%s() RU configured - RACH processing thread running\n", __FUNCTION__);

  while (!oai_exit) {
    
    if (oai_exit) break;
    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"ru_prach_thread") < 0) break;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_RU_PRACH_RX, 1 );      
    if (ru->eNB_list[0]){
      prach_procedures(
		       ru->eNB_list[0]
#ifdef Rel14
		       ,0
#endif
		       );
    }
    else {
      rx_prach(NULL,
	       ru,
	       NULL,
	       NULL,
	       NULL,
	       proc->frame_prach,
	       0
#ifdef Rel14
	       ,0
#endif
	       );
    } 
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_RU_PRACH_RX, 0 );      
    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"ru_prach_thread") < 0) break;
  }

  LOG_I(PHY, "Exiting RU thread PRACH\n");

  ru_thread_prach_status = 0;
  return &ru_thread_prach_status;
}

#ifdef Rel14
static void* ru_thread_prach_br( void* param ) {

  static int ru_thread_prach_status;

  RU_t *ru        = (RU_t*)param;
  RU_proc_t *proc = (RU_proc_t*)&ru->proc;

  // set default return value
  ru_thread_prach_status = 0;

  thread_top_init("ru_thread_prach_br",1,500000L,1000000L,20000000L);

  while (!oai_exit) {
    
    if (oai_exit) break;
    if (wait_on_condition(&proc->mutex_prach_br,&proc->cond_prach_br,&proc->instance_cnt_prach_br,"ru_prach_thread_br") < 0) break;
    rx_prach(NULL,
	     ru,
	     NULL,
             NULL,
             NULL,
             proc->frame_prach_br,
             0,
	     1);
    if (release_thread(&proc->mutex_prach_br,&proc->instance_cnt_prach_br,"ru_prach_thread_br") < 0) break;
  }

  LOG_I(PHY, "Exiting RU thread PRACH BR\n");

  ru_thread_prach_status = 0;
  return &ru_thread_prach_status;
}
#endif

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
  RRU_CONFIG_msg_t rru_config_msg;

  // initialize the synchronization buffer to the common_vars.rxdata
  for (int i=0;i<ru->nb_rx;i++)
    rxp[i] = &ru->common.rxdata[i][0];

  /* double temp_freq1 = ru->rfdevice.openair0_cfg->rx_freq[0];
     double temp_freq2 = ru->rfdevice.openair0_cfg->tx_freq[0];
     for (i=0;i<4;i++) {
     ru->rfdevice.openair0_cfg->rx_freq[i] = ru->rfdevice.openair0_cfg->tx_freq[i];
     ru->rfdevice.openair0_cfg->tx_freq[i] = temp_freq1;
     }
     ru->rfdevice.trx_set_freq_func(&ru->rfdevice,ru->rfdevice.openair0_cfg,0);
  */

  LOG_I(PHY,"Entering synch routine\n");
  
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
      
      for (i=0; i<ru->nb_rx; i++)
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
				   ru->nb_rx);
  /*for (i=0;i<4;i++) {
    ru->rfdevice.openair0_cfg->rx_freq[i] = temp_freq1;
    ru->rfdevice.openair0_cfg->tx_freq[i] = temp_freq2;
    }

    ru->rfdevice.trx_set_freq_func(&ru->rfdevice,ru->rfdevice.openair0_cfg,0);
  */
  ru->state    = RU_RUN;
  // Send RRU_sync_ok
  rru_config_msg.type = RRU_sync_ok;
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t); // TODO: set to correct msg len

  LOG_I(PHY,"Sending RRU_sync_ok to RAU\n");
  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),"Failed to send msg to RAU %d\n",ru->idx);

  LOG_I(PHY,"Exiting synch routine\n");
}



void wakeup_eNBs(RU_t *ru) {

  int i;
  PHY_VARS_eNB **eNB_list = ru->eNB_list;
  PHY_VARS_eNB *eNB=eNB_list[0];
  eNB_proc_t *proc = &eNB->proc;
  struct timespec t;

  LOG_D(PHY,"wakeup_eNBs (num %d) for RU %d (state %s)ru->eNB_top:%p\n",ru->num_eNB,ru->idx, ru_states[ru->state],ru->eNB_top);

  if (ru->num_eNB==1 && ru->eNB_top!=0) {
    // call eNB function directly

    char string[20];
    sprintf(string,"Incoming RU %d",ru->idx);
    
    pthread_mutex_lock(&proc->mutex_RU);
    LOG_I(PHY,"Frame %d, Subframe %d: RU %d done (wait_cnt %d),RU_mask[%d] %x\n",
          ru->proc.frame_rx,ru->proc.subframe_rx,ru->idx,ru->wait_cnt,ru->proc.subframe_rx,proc->RU_mask[ru->proc.subframe_rx]);

    if (proc->RU_mask[ru->proc.subframe_rx] == 0){
      clock_gettime(CLOCK_MONOTONIC,&proc->t[ru->proc.subframe_rx]);
      //start_meas(&proc->ru_arrival_time);
     
      LOG_D(PHY,"RU %d starting timer for frame %d subframe %d\n",ru->idx, ru->proc.frame_rx,ru->proc.subframe_rx);
    }
    for (i=0;i<eNB->num_RU;i++) {
      LOG_D(PHY,"RU %d has frame %d and subframe %d, state %s\n",eNB->RU_list[i]->idx,eNB->RU_list[i]->proc.frame_rx, eNB->RU_list[i]->proc.subframe_rx, ru_states[eNB->RU_list[i]->state]);
      if (ru == eNB->RU_list[i]) {
//	AssertFatal((proc->RU_mask&(1<<i)) == 0, "eNB %d frame %d, subframe %d : previous information from RU %d (num_RU %d,mask %x) has not been served yet!\n",eNB->Mod_id,ru->proc.frame_rx,ru->proc.subframe_rx,ru->idx,eNB->num_RU,proc->RU_mask);
        proc->RU_mask[ru->proc.subframe_rx] |= (1<<i);
      }else if (eNB->RU_list[i]->state == RU_SYNC || eNB->RU_list[i]->wait_cnt > 0){
      	proc->RU_mask[ru->proc.subframe_rx] |= (1<<i);
      }
    }
    clock_gettime(CLOCK_MONOTONIC,&t);
    LOG_I(PHY,"RU mask is now %x, time is %lu\n",proc->RU_mask[ru->proc.subframe_rx], t.tv_nsec - proc->t[ru->proc.subframe_rx].tv_nsec);

    if (proc->RU_mask[ru->proc.subframe_rx] == (1<<eNB->num_RU)-1) {
      LOG_D(PHY,"Reseting mask frame %d, subframe %d, this is RU %d\n",ru->proc.frame_rx, ru->proc.subframe_rx, ru->idx);
      proc->RU_mask[ru->proc.subframe_rx] = 0;
      clock_gettime(CLOCK_MONOTONIC,&t);
      //stop_meas(&proc->ru_arrival_time);
      AssertFatal(t.tv_nsec < proc->t[ru->proc.subframe_rx].tv_nsec+5000000,
                  "Time difference for subframe %d (Frame %d) => %lu > 5ms, this is RU %d\n",
                  ru->proc.subframe_rx, ru->proc.frame_rx,t.tv_nsec - proc->t[ru->proc.subframe_rx].tv_nsec, ru->idx);
    }
    
    pthread_mutex_unlock(&proc->mutex_RU);
    LOG_D(PHY,"wakeup eNB top for for subframe %d\n", ru->proc.subframe_rx);
    ru->eNB_top(eNB_list[0],ru->proc.frame_rx,ru->proc.subframe_rx,string);
  }
  else { // multiple eNB case for later

    LOG_D(PHY,"ru->num_eNB:%d\n", ru->num_eNB);

    for (i=0;i<ru->num_eNB;i++)
      {
	LOG_D(PHY,"ru->wakeup_rxtx:%p\n", ru->wakeup_rxtx);

	if (ru->wakeup_rxtx!=0 && ru->wakeup_rxtx(eNB_list[i],ru) < 0)
	  {
	    LOG_E(PHY,"could not wakeup eNB rxtx process for subframe %d\n", ru->proc.subframe_rx);
	  }
      }
  }
}

static inline int wakeup_prach_ru(RU_t *ru) {

  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  if (pthread_mutex_timedlock(&ru->proc.mutex_prach,&wait) !=0) {
    LOG_E( PHY, "[RU] ERROR pthread_mutex_lock for RU prach thread (IC %d)\n", ru->proc.instance_cnt_prach);
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }
  if (ru->proc.instance_cnt_prach==-1) {
    ++ru->proc.instance_cnt_prach;
    ru->proc.frame_prach    = ru->proc.frame_rx;
    ru->proc.subframe_prach = ru->proc.subframe_rx;

    // DJP - think prach_procedures() is looking at eNB frame_prach
    if (ru->eNB_list[0]) {
      ru->eNB_list[0]->proc.frame_prach = ru->proc.frame_rx;
      ru->eNB_list[0]->proc.subframe_prach = ru->proc.subframe_rx;
    }
    LOG_D(PHY,"RU %d: waking up PRACH thread\n",ru->idx);
    // the thread can now be woken up
    AssertFatal(pthread_cond_signal(&ru->proc.cond_prach) == 0, "ERROR pthread_cond_signal for RU prach thread\n");
  }
  else LOG_W(PHY,"RU prach thread busy, skipping\n");
  pthread_mutex_unlock( &ru->proc.mutex_prach );

  return(0);
}

#ifdef Rel14
static inline int wakeup_prach_ru_br(RU_t *ru) {

  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  if (pthread_mutex_timedlock(&ru->proc.mutex_prach_br,&wait) !=0) {
    LOG_E( PHY, "[RU] ERROR pthread_mutex_lock for RU prach thread BR (IC %d)\n", ru->proc.instance_cnt_prach_br);
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }
  if (ru->proc.instance_cnt_prach_br==-1) {
    ++ru->proc.instance_cnt_prach_br;
    ru->proc.frame_prach_br    = ru->proc.frame_rx;
    ru->proc.subframe_prach_br = ru->proc.subframe_rx;

    LOG_D(PHY,"RU %d: waking up PRACH thread\n",ru->idx);
    // the thread can now be woken up
    AssertFatal(pthread_cond_signal(&ru->proc.cond_prach_br) == 0, "ERROR pthread_cond_signal for RU prach thread BR\n");
  }
  else LOG_W(PHY,"RU prach thread busy, skipping\n");
  pthread_mutex_unlock( &ru->proc.mutex_prach_br );

  return(0);
}
#endif

// this is for RU with local RF unit
void fill_rf_config(RU_t *ru, char *rf_config_file) {

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
  else AssertFatal(1==0,"Unknown N_RB_DL %d\n",fp->N_RB_DL);


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

    cfg->tx_gain[i] = ru->att_tx;
    cfg->rx_gain[i] = ru->max_rxgain-ru->att_rx;

    cfg->configFilename = rf_config_file;
    printf("channel %d, Setting tx_gain offset %f, rx_gain offset %f, tx_freq %f, rx_freq %f\n",
	   i, cfg->tx_gain[i],
	   cfg->rx_gain[i],
	   cfg->tx_freq[i],
	   cfg->rx_freq[i]);
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
  
  
  if (frame_parms->frame_type == TDD) {
    if      (frame_parms->N_RB_DL == 100) ru->N_TA_offset = 624;
    else if (frame_parms->N_RB_DL == 50)  ru->N_TA_offset = 624/2;
    else if (frame_parms->N_RB_DL == 25)  ru->N_TA_offset = 624/4;
  } 
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

static void* ru_stats_thread(void* param) {

  RU_t               *ru      = (RU_t*)param;

  PHY_VARS_eNB **eNB_list = ru->eNB_list;
  PHY_VARS_eNB *eNB=eNB_list[0];
  eNB_proc_t *proc = &eNB->proc;

  wait_sync("ru_stats_thread");

  while (!oai_exit) {
    sleep(1);
    if (opp_enabled == 1) {
     // print_meas(&proc->ru_arrival_time,"ru_arrival_time",NULL,NULL);
      if (ru->feprx) print_meas(&ru->ofdm_demod_stats,"feprx",NULL,NULL);
      if (ru->feptx_ofdm) print_meas(&ru->ofdm_mod_stats,"feptx_ofdm",NULL,NULL);
      if (ru->fh_north_asynch_in) print_meas(&ru->rx_fhaul,"rx_fhaul",NULL,NULL);
      if (ru->fh_north_out) {
	print_meas(&ru->tx_fhaul,"tx_fhaul",NULL,NULL);
	print_meas(&ru->compression,"compression",NULL,NULL);
	print_meas(&ru->transport,"transport",NULL,NULL);
      }
    }
    else break;
  }
  return(NULL);
}


void reset_proc(RU_t *ru);

static void* ru_thread_control( void* param ) {


  RU_t               *ru      = (RU_t*)param;
  RU_proc_t          *proc    = &ru->proc;
  RRU_CONFIG_msg_t   rru_config_msg;
  ssize_t	     msg_len;
  int                len, ret;


  // Start IF device if any
  if (ru->start_if) {
    LOG_I(PHY,"Starting IF interface for RU %d\n",ru->idx);
    AssertFatal(
	ru->start_if(ru,NULL) 	== 0, "Could not start the IF device\n");

    if (ru->if_south != LOCAL_RF) wait_eNBs();
  }

  
  ru->state = (ru->function==eNodeB_3GPP)? RU_RUN : RU_IDLE;
  LOG_I(PHY,"Control channel ON for RU %d\n", ru->idx);

  while (!oai_exit) // Change the cond
    {
      msg_len  = sizeof(RRU_CONFIG_msg_t); // TODO : check what should be the msg len

      if (ru->state == RU_IDLE && ru->if_south != LOCAL_RF)
	send_tick(ru);

	
      if ((len = ru->ifdevice.trx_ctlrecv_func(&ru->ifdevice,
					       &rru_config_msg,
					       msg_len))<0) {
	LOG_D(PHY,"Waiting msg for RU %d\n", ru->idx);     
      }
      else
	{
	  switch(rru_config_msg.type)
	    {
	    case RAU_tick:  // RRU
	      if (ru->if_south != LOCAL_RF){
		LOG_I(PHY,"Received Tick msg...Ignoring\n");
	      }else{
		LOG_I(PHY,"Tick received from RAU\n");
				
		if (send_capab(ru) == 0) ru->state = RU_CONFIG;
	      }				
	      break;

	    case RRU_capabilities: // RAU
	      if (ru->if_south == LOCAL_RF) LOG_E(PHY,"Received RRU_capab msg...Ignoring\n");
	      
	      else{
		msg_len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_capabilities_t);

		AssertFatal(rru_config_msg.len==msg_len,"Received capabilities with incorrect length (%d!=%d)\n",(int)rru_config_msg.len,(int)msg_len);
		LOG_I(PHY,"Received capabilities from RRU %d (len %d/%d, num_bands %d,max_pdschReferenceSignalPower %d, max_rxgain %d, nb_tx %d, nb_rx %d)\n",ru->idx,
		      (int)rru_config_msg.len,(int)msg_len,
		      ((RRU_capabilities_t*)&rru_config_msg.msg[0])->num_bands,
		      ((RRU_capabilities_t*)&rru_config_msg.msg[0])->max_pdschReferenceSignalPower[0],
		      ((RRU_capabilities_t*)&rru_config_msg.msg[0])->max_rxgain[0],
		      ((RRU_capabilities_t*)&rru_config_msg.msg[0])->nb_tx[0],
		      ((RRU_capabilities_t*)&rru_config_msg.msg[0])->nb_rx[0]);

		configure_ru(ru->idx,(RRU_capabilities_t *)&rru_config_msg.msg[0]);

		// send config
		if (send_config(ru,rru_config_msg) == 0) ru->state = RU_CONFIG;
	      }

	      break;
				
	    case RRU_config: // RRU
	      if (ru->if_south == LOCAL_RF){
		LOG_I(PHY,"Configuration received from RAU  (num_bands %d,band0 %d,txfreq %u,rxfreq %u,att_tx %d,att_rx %d,N_RB_DL %d,N_RB_UL %d,3/4FS %d, prach_FO %d, prach_CI %d)\n",
		      ((RRU_config_t *)&rru_config_msg.msg[0])->num_bands,
		      ((RRU_config_t *)&rru_config_msg.msg[0])->band_list[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->tx_freq[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->rx_freq[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->att_tx[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->att_rx[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_DL[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->N_RB_UL[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->threequarter_fs[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->prach_FreqOffset[0],
		      ((RRU_config_t *)&rru_config_msg.msg[0])->prach_ConfigIndex[0]);
	      
		configure_rru(ru->idx, (void*)&rru_config_msg.msg[0]);

 					  
		fill_rf_config(ru,ru->rf_config_file);
		init_frame_parms(&ru->frame_parms,1);
		ru->frame_parms.nb_antennas_rx = ru->nb_rx;
		phy_init_RU(ru);
					 
		//if (ru->is_slave == 1) lte_sync_time_init(&ru->frame_parms);

		if (ru->rfdevice.is_init != 1){
			ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
		}

		
		AssertFatal((ru->rfdevice.trx_config_func(&ru->rfdevice,&ru->openair0_cfg)==0), 
				"Failed to configure RF device for RU %d\n",ru->idx);




		if (setup_RU_buffers(ru)!=0) {
		  printf("Exiting, cannot initialize RU Buffers\n");
		  exit(-1);
		}

		// send CONFIG_OK

		rru_config_msg.type = RRU_config_ok; 
		rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t);
		LOG_I(PHY,"Sending CONFIG_OK to RAU %d\n", ru->idx);

		AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
			    "RU %d failed send CONFIG_OK to RAU\n",ru->idx);
                reset_proc(ru);
		ru->state = RU_READY;
	      } else LOG_E(PHY,"Received RRU_config msg...Ignoring\n");
	      	
	      break;	

	    case RRU_config_ok: // RAU
	      if (ru->if_south == LOCAL_RF) LOG_E(PHY,"Received RRU_config_ok msg...Ignoring\n");
	      else{

		if (setup_RU_buffers(ru)!=0) {
		  printf("Exiting, cannot initialize RU Buffers\n");
		  exit(-1);
		}

		// Set state to RUN for Master RU, Others on SYNC
		ru->state = (ru->is_slave == 1) ? RU_SYNC : RU_RUN ;
		ru->in_synch = 0;
					

		LOG_I(PHY, "Signaling main thread that RU %d (is_slave %d) is ready in state %s\n",ru->idx,ru->is_slave,ru_states[ru->state]);
		pthread_mutex_lock(&RC.ru_mutex);
		RC.ru_mask &= ~(1<<ru->idx);
		pthread_cond_signal(&RC.ru_cond);
		pthread_mutex_unlock(&RC.ru_mutex);
					  
		wait_sync("ru_thread_control");

		// send start
		rru_config_msg.type = RRU_start;
		rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t); // TODO: set to correct msg len

 
		LOG_I(PHY,"Sending Start to RRU %d\n", ru->idx);
		AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),"Failed to send msg to RU %d\n",ru->idx);

					
		pthread_mutex_lock(&proc->mutex_ru);
		proc->instance_cnt_ru = 1;
		pthread_mutex_unlock(&proc->mutex_ru);
		if (pthread_cond_signal(&proc->cond_ru_thread) != 0) {
		  LOG_E( PHY, "ERROR pthread_cond_signal for RU %d\n",ru->idx);
		  exit_fun( "ERROR pthread_cond_signal" );
		  break;
		}
	      }		
	      break;

	    case RRU_start: // RRU
	      if (ru->if_south == LOCAL_RF){
		LOG_I(PHY,"Start received from RAU\n");
				
		if (ru->state == RU_READY){

		  LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
		  pthread_mutex_lock(&RC.ru_mutex);
		  RC.ru_mask &= ~(1<<ru->idx);
		  pthread_cond_signal(&RC.ru_cond);
		  pthread_mutex_unlock(&RC.ru_mutex);
						  
		  wait_sync("ru_thread_control");
						
		  ru->state = (ru->is_slave == 1) ? RU_SYNC : RU_RUN ;
	          ru->cmd   = EMPTY;			
		  pthread_mutex_lock(&proc->mutex_ru);
		  proc->instance_cnt_ru = 1;
		  pthread_mutex_unlock(&proc->mutex_ru);
		  if (pthread_cond_signal(&proc->cond_ru_thread) != 0) {
		    LOG_E( PHY, "ERROR pthread_cond_signal for RU %d\n",ru->idx);
		    exit_fun( "ERROR pthread_cond_signal" );
		    break;
		  }
		}
		else LOG_E(PHY,"RRU not ready, cannot start\n"); 
		
	      } else LOG_E(PHY,"Received RRU_start msg...Ignoring\n");
	      

	      break;

	    case RRU_sync_ok: //RAU
	      if (ru->if_south == LOCAL_RF) LOG_E(PHY,"Received RRU_config_ok msg...Ignoring\n");
	      else{
		if (ru->is_slave == 1){
                  LOG_I(PHY,"Received RRU_sync_ok from RRU %d\n",ru->idx);
		  // Just change the state of the RRU to unblock ru_thread()
		  ru->state = RU_RUN;		
		}else LOG_E(PHY,"Received RRU_sync_ok from a master RRU...Ignoring\n"); 	
	      }
	      break;
            case RRU_frame_resynch: //RRU
              if (ru->if_south != LOCAL_RF) LOG_E(PHY,"Received RRU frame resynch message, should not happen in RAU\n");
              else {
                 LOG_I(PHY,"Received RRU_frame_resynch command\n");
                 ru->cmd = RU_FRAME_RESYNCH;
                 ru->cmdval = ((uint16_t*)&rru_config_msg.msg[0])[0];
                 LOG_I(PHY,"Received Frame Resynch messaage with value %d\n",ru->cmdval);
              }
              break;

	    case RRU_stop: // RRU
	      if (ru->if_south == LOCAL_RF){
		LOG_I(PHY,"Stop received from RAU\n");

		if (ru->state == RU_RUN || ru->state == RU_ERROR){

		  LOG_I(PHY,"Stopping RRU\n");
		  ru->state = RU_READY;
		  // TODO: stop ru_thread
		}else{
		  LOG_I(PHY,"RRU not running, can't stop\n"); 
		}
	      }else LOG_E(PHY,"Received RRU_stop msg...Ignoring\n");
	      
	      break;

	    default:
	      if (ru->if_south != LOCAL_RF){
		if (ru->state == RU_IDLE){
		  // Keep sending TICK
		  send_tick(ru);
		}
	      }

	      break;
	    } // switch	
	} //else


    }//while
  return(NULL);
}


static void* ru_thread( void* param ) {


  RU_t               *ru      = (RU_t*)param;
  RU_proc_t          *proc    = &ru->proc;
  LTE_DL_FRAME_PARMS *fp      = &ru->frame_parms;
  int                subframe =9;
  int                frame    =1023; 
  int			resynch_done = 0;



  // set default return value
  thread_top_init("ru_thread",0,870000,1000000,1000000);

  LOG_I(PHY,"Starting RU %d (%s,%s),\n",ru->idx,eNB_functions[ru->function],eNB_timing[ru->if_timing]);

	
  while (!oai_exit) {
  
    if (ru->if_south != LOCAL_RF && ru->is_slave==1) ru->wait_cnt = 100;
    else                          ru->wait_cnt = 0;

    // wait to be woken up
    if (ru->function!=eNodeB_3GPP) {
      if (wait_on_condition(&ru->proc.mutex_ru,&ru->proc.cond_ru_thread,&ru->proc.instance_cnt_ru,"ru_thread")<0) break;
    }
    else wait_sync("ru_thread");
	  
    if (ru->is_slave == 0) AssertFatal(ru->state == RU_RUN,"ru-%d state = %s != RU_RUN\n",ru->idx,ru_states[ru->state]);
    else if (ru->is_slave == 1) AssertFatal(ru->state == RU_SYNC || ru->state == RU_RUN,"ru %d state = %s != RU_SYNC or RU_RUN\n",ru->idx,ru_states[ru->state]);  
    // Start RF device if any
    if (ru->start_rf) {
      if (ru->start_rf(ru) != 0)
	LOG_E(HW,"Could not start the RF device\n");
      else LOG_I(PHY,"RU %d rf device ready\n",ru->idx);
    }
    else LOG_D(PHY,"RU %d no rf device\n",ru->idx);


    // if an asnych_rxtx thread exists
    // wakeup the thread because the devices are ready at this point
	
    LOG_D(PHY,"Locking asynch mutex\n"); 
    if ((ru->fh_south_asynch_in)||(ru->fh_north_asynch_in)) {
      pthread_mutex_lock(&proc->mutex_asynch_rxtx);
      proc->instance_cnt_asynch_rxtx=0;
      pthread_mutex_unlock(&proc->mutex_asynch_rxtx);
      pthread_cond_signal(&proc->cond_asynch_rxtx);
    }
    else LOG_D(PHY,"RU %d no asynch_south interface\n",ru->idx);

    // if this is a slave RRU, try to synchronize on the DL frequency
    if ((ru->is_slave == 1) && (ru->if_south == LOCAL_RF)) do_ru_synch(ru);

  
    LOG_D(PHY,"Starting steady-state operation\n");
    // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
    while (ru->state == RU_RUN) {

      // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
      // They are set on the first rx/tx in the underly FH routines.
      if (subframe==9) { 
	subframe=0;
	frame++;
	frame&=1023;
      } else {
	subframe++;
      }      


      // synchronization on input FH interface, acquire signals/data and block
      if (ru->stop_rf && ru->cmd == STOP_RU) {
	ru->stop_rf(ru);
	ru->state = RU_IDLE;
	ru->cmd   = EMPTY;
	LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
	break;
      }
      else if (ru->cmd == STOP_RU) {
	ru->state = RU_IDLE;
	ru->cmd   = EMPTY;
	LOG_I(PHY,"RU %d stopped\n",ru->idx);
	break;
      }
      if (oai_exit == 1) break;
 
      if (ru->fh_south_in && ru->state == RU_RUN ) ru->fh_south_in(ru,&frame,&subframe);
      else AssertFatal(1==0, "No fronthaul interface at south port");
      if (ru->wait_cnt > 0) {
         ru->wait_cnt--;

	 LOG_I(PHY,"RU thread %d, frame %d, subframe %d, wait_cnt %d \n",ru->idx, frame, subframe, ru->wait_cnt);

         if (ru->if_south!=LOCAL_RF && ru->wait_cnt <=20 && subframe == 5 && frame != RC.ru[0]->proc.frame_rx && resynch_done == 0) {
           // Send RRU_frame adjust
           RRU_CONFIG_msg_t rru_config_msg;
           rru_config_msg.type = RRU_frame_resynch;
           rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t); // TODO: set to correct msg len
           ((uint16_t*)&rru_config_msg.msg[0])[0] = RC.ru[0]->proc.frame_rx;
           ru->cmd=WAIT_RESYNCH;
           LOG_D(PHY,"Sending Frame Resynch %d to RRU %d\n", RC.ru[0]->proc.frame_rx,ru->idx);
           AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),"Failed to send msg to RAU\n");
	   resynch_done=1;
         }
      }
      if (ru->wait_cnt == 0)  {

        LOG_D(PHY,"RU thread %d, frame %d, subframe %d \n",
              ru->idx,frame,subframe);


        if ((ru->do_prach>0) && (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)==1)) {
  	  wakeup_prach_ru(ru);
        }
#ifdef Rel14
        else if ((ru->do_prach>0) && (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)>1)) {
	  wakeup_prach_ru_br(ru);
        }
#endif

        // adjust for timing offset between RU
        if (ru->idx!=0) proc->frame_tx = (proc->frame_tx+proc->frame_offset)&1023;

        // At this point, all information for subframe has been received on FH interface
        // If this proc is to provide synchronization, do so
        wakeup_slaves(proc);

        // do RX front-end processing (frequency-shift, dft) if needed
        if (ru->feprx) ru->feprx(ru);

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
    }

  } // while !oai_exit
  

  printf( "Exiting ru_thread \n");

  if (ru->stop_rf != NULL) {
    if (ru->stop_rf(ru) != 0)
      LOG_E(HW,"Could not stop the RF device\n");
    else LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
  }

  return NULL;

}


// This thread run the initial synchronization like a UE
void *ru_thread_synch(void *arg) {

  RU_t *ru = (RU_t*)arg;
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;
  int32_t sync_pos,sync_pos2;
  uint32_t peak_val;
  uint32_t sync_corr[307200] __attribute__((aligned(32)));
  static int ru_thread_synch_status=0;
  int cnt=0;

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
      LOG_I(PHY,"RU synch cnt %d: %d, val %d\n",cnt,sync_pos,peak_val);
      cnt++;
      if (sync_pos >= 0) {
	if (sync_pos >= fp->nb_prefix_samples)
	  sync_pos2 = sync_pos - fp->nb_prefix_samples;
	else
	  sync_pos2 = sync_pos + (fp->samples_per_tti*10) - fp->nb_prefix_samples;
	int sync_pos_slot;
	if (fp->frame_type == FDD) {
	  
	  // PSS is hypothesized in last symbol of first slot in Frame
	  sync_pos_slot = (fp->samples_per_tti>>1) - fp->ofdm_symbol_size - fp->nb_prefix_samples;
	}
	else {
	  // PSS is hypothesized in 2nd symbol of third slot in Frame (S-subframe)
	  sync_pos_slot = fp->samples_per_tti +
	    (fp->ofdm_symbol_size<<1) +
	    fp->nb_prefix_samples0 +
	    (fp->nb_prefix_samples);
	}	  
	if (sync_pos2 >= sync_pos_slot)
	  ru->rx_offset = sync_pos2 - sync_pos_slot;
	else
	  ru->rx_offset = (fp->samples_per_tti*10) + sync_pos2 - sync_pos_slot;
	

	LOG_I(PHY,"Estimated sync_pos %d, peak_val %d => timing offset %d\n",sync_pos,peak_val,ru->rx_offset);
	
	/*
	  if ((peak_val > 300000) && (sync_pos > 0)) {
	  //      if (sync_pos++ > 3) {
	  write_output("ru_sync.m","sync",(void*)&sync_corr[0],fp->samples_per_tti*5,1,2);
	  write_output("ru_rx.m","rxs",(void*)ru->ru_time.rxdata[0][0],fp->samples_per_tti*10,1,1);
	  exit(-1);
	  }
	*/
	ru->in_synch = 1;

      } // symc_pos > 0
      else {
	if (cnt>1000) {
	  write_output("ru_sync.m","sync",(void*)&sync_corr[0],fp->samples_per_tti*5,1,2);
	  write_output("ru_rx.m","rxs",(void*)ru->common.rxdata[0],fp->samples_per_tti*10,1,1);
          exit(1);
        } 
      }
    } // ru->in_synch==0

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

int stop_rf(RU_t *ru)
{
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  return 0;
}

extern void fep_full(RU_t *ru);
extern void ru_fep_full_2thread(RU_t *ru);
extern void feptx_ofdm(RU_t *ru);
extern void feptx_ofdm_2thread(RU_t *ru);
extern void feptx_prec(RU_t *ru);
extern void init_fep_thread(RU_t *ru,pthread_attr_t *attr);
extern void init_feptx_thread(RU_t *ru,pthread_attr_t *attr);

void reset_proc(RU_t *ru) {

  int i=0;
  RU_proc_t *proc;

  AssertFatal(ru != NULL, "ru is null\n");
  proc = &ru->proc;

  proc->ru = ru;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->frame_tx_unwrap          = 0;

  for (i=0;i<10;i++) proc->symbol_mask[i]=0;
}

void init_RU_proc(RU_t *ru) {
   
  int i=0;
  RU_proc_t *proc;
  pthread_attr_t *attr_FH=NULL,*attr_prach=NULL,*attr_asynch=NULL,*attr_synch=NULL, *attr_ctrl=NULL;
  //pthread_attr_t *attr_fep=NULL;
#ifdef Rel14
  pthread_attr_t *attr_prach_br=NULL;
#endif
  char name[100];

#ifndef OCP_FRAMEWORK
  LOG_I(PHY,"Initializing RU proc %d (%s,%s),\n",ru->idx,eNB_functions[ru->function],eNB_timing[ru->if_timing]);
#endif
  proc = &ru->proc;
  memset((void*)proc,0,sizeof(RU_proc_t));

  proc->ru = ru;
  proc->instance_cnt_prach       = -1;
  proc->instance_cnt_synch       = -1;     ;
  proc->instance_cnt_FH          = -1;
  proc->instance_cnt_asynch_rxtx = -1;
  proc->instance_cnt_ru 	 = -1;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->num_slaves               = 0;
  proc->frame_tx_unwrap          = 0;

  for (i=0;i<10;i++) proc->symbol_mask[i]=0;
  
  pthread_mutex_init( &proc->mutex_prach, NULL);
  pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);
  pthread_mutex_init( &proc->mutex_synch,NULL);
  pthread_mutex_init( &proc->mutex_FH,NULL);
  pthread_mutex_init( &proc->mutex_eNBs, NULL);
  pthread_mutex_init( &proc->mutex_ru,NULL);
  
  pthread_cond_init( &proc->cond_prach, NULL);
  pthread_cond_init( &proc->cond_FH, NULL);
  pthread_cond_init( &proc->cond_asynch_rxtx, NULL);
  pthread_cond_init( &proc->cond_synch,NULL);
  pthread_cond_init( &proc->cond_eNBs, NULL);
  pthread_cond_init( &proc->cond_ru_thread,NULL);
  
  pthread_attr_init( &proc->attr_FH);
  pthread_attr_init( &proc->attr_prach);
  pthread_attr_init( &proc->attr_synch);
  pthread_attr_init( &proc->attr_asynch_rxtx);
  pthread_attr_init( &proc->attr_fep);

#ifdef Rel14
  proc->instance_cnt_prach_br       = -1;
  pthread_mutex_init( &proc->mutex_prach_br, NULL);
  pthread_cond_init( &proc->cond_prach_br, NULL);
  pthread_attr_init( &proc->attr_prach_br);
#endif  
  
#ifndef DEADLINE_SCHEDULER
  attr_FH        = &proc->attr_FH;
  attr_prach     = &proc->attr_prach;
  attr_synch     = &proc->attr_synch;
  attr_asynch    = &proc->attr_asynch_rxtx;
#ifdef Rel14
  attr_prach_br  = &proc->attr_prach_br;
#endif
#endif
  
  if (ru->function!=eNodeB_3GPP) pthread_create( &proc->pthread_ctrl, attr_ctrl, ru_thread_control, (void*)ru );
  

  if (ru->function == NGFI_RRU_IF4p5) {
    pthread_create( &proc->pthread_prach, attr_prach, ru_thread_prach, (void*)ru );
#ifdef Rel14  
    pthread_create( &proc->pthread_prach_br, attr_prach_br, ru_thread_prach_br, (void*)ru );
#endif
    if (ru->is_slave == 1) pthread_create( &proc->pthread_synch, attr_synch, ru_thread_synch, (void*)ru);
    
    
    if ((ru->if_timing == synch_to_other) ||
	(ru->function == NGFI_RRU_IF5) ||
	(ru->function == NGFI_RRU_IF4p5)) pthread_create( &proc->pthread_asynch_rxtx, attr_asynch, ru_thread_asynch_rxtx, (void*)ru );
    
    
  }
  else if (ru->function == eNodeB_3GPP && ru->if_south == LOCAL_RF) { // DJP - need something else to distinguish between monolithic and PNF
    LOG_I(PHY,"%s() DJP - added creation of pthread_prach\n", __FUNCTION__);
    pthread_create( &proc->pthread_prach, attr_prach, ru_thread_prach, (void*)ru );
    ru->state=RU_RUN;
    fill_rf_config(ru,ru->rf_config_file);
    init_frame_parms(&ru->frame_parms,1);
    ru->frame_parms.nb_antennas_rx = ru->nb_rx;
    phy_init_RU(ru);


    openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);

   if (setup_RU_buffers(ru)!=0) {
      printf("Exiting, cannot initialize RU Buffers\n");
      exit(-1);
   }

  }

  if (get_nprocs()>=2) { 
    if (ru->feprx) init_fep_thread(ru,NULL); 
    if (ru->feptx_ofdm) init_feptx_thread(ru,NULL);
  } 
  if (opp_enabled == 1) pthread_create(&ru->ru_stats_thread,NULL,ru_stats_thread,(void*)ru); 
 
  pthread_create( &proc->pthread_FH, attr_FH, ru_thread, (void*)ru );
  snprintf( name, sizeof(name), "ru_thread_FH %d", ru->idx );
  pthread_setname_np( proc->pthread_FH, name );

  if (ru->function == eNodeB_3GPP) {
    usleep(10000);
    LOG_I(PHY, "Signaling main thread that RU %d (is_slave %d) is ready in state %s\n",ru->idx,ru->is_slave,ru_states[ru->state]);
    pthread_mutex_lock(&RC.ru_mutex);
    RC.ru_mask &= ~(1<<ru->idx);
    pthread_cond_signal(&RC.ru_cond);
    pthread_mutex_unlock(&RC.ru_mutex);
  }
}

void kill_RU_proc(int inst)
{
  RU_t *ru = RC.ru[inst];
  RU_proc_t *proc = &ru->proc;

  pthread_mutex_lock(&proc->mutex_FH);
  proc->instance_cnt_FH = 0;
  pthread_mutex_unlock(&proc->mutex_FH);
  pthread_cond_signal(&proc->cond_FH);

  pthread_mutex_lock(&proc->mutex_prach);
  proc->instance_cnt_prach = 0;
  pthread_mutex_unlock(&proc->mutex_prach);
  pthread_cond_signal(&proc->cond_prach);

#ifdef Rel14
  pthread_mutex_lock(&proc->mutex_prach_br);
  proc->instance_cnt_prach_br = 0;
  pthread_mutex_unlock(&proc->mutex_prach_br);
  pthread_cond_signal(&proc->cond_prach_br);
#endif

  pthread_mutex_lock(&proc->mutex_synch);
  proc->instance_cnt_synch = 0;
  pthread_mutex_unlock(&proc->mutex_synch);
  pthread_cond_signal(&proc->cond_synch);

  pthread_mutex_lock(&proc->mutex_eNBs);
  proc->instance_cnt_eNBs = 0;
  pthread_mutex_unlock(&proc->mutex_eNBs);
  pthread_cond_signal(&proc->cond_eNBs);

  pthread_mutex_lock(&proc->mutex_asynch_rxtx);
  proc->instance_cnt_asynch_rxtx = 0;
  pthread_mutex_unlock(&proc->mutex_asynch_rxtx);
  pthread_cond_signal(&proc->cond_asynch_rxtx);

  LOG_D(PHY, "Joining pthread_FH\n");
  pthread_join(proc->pthread_FH, NULL);
  if (ru->function == NGFI_RRU_IF4p5) {
    LOG_D(PHY, "Joining pthread_prach\n");
    pthread_join(proc->pthread_prach, NULL);
#ifdef Rel14
    LOG_D(PHY, "Joining pthread_prach_br\n");
    pthread_join(proc->pthread_prach_br, NULL);
#endif
    if (ru->is_slave) {
      LOG_D(PHY, "Joining pthread_\n");
      pthread_join(proc->pthread_synch, NULL);
    }

    if ((ru->if_timing == synch_to_other) ||
        (ru->function == NGFI_RRU_IF5) ||
        (ru->function == NGFI_RRU_IF4p5)) {
      LOG_D(PHY, "Joining pthread_asynch_rxtx\n");
      pthread_join(proc->pthread_asynch_rxtx, NULL);
    }
  }
  if (get_nprocs() >= 2) {
    if (ru->feprx) {
      pthread_mutex_lock(&proc->mutex_fep);
      proc->instance_cnt_fep = 0;
      pthread_mutex_unlock(&proc->mutex_fep);
      pthread_cond_signal(&proc->cond_fep);
      LOG_D(PHY, "Joining pthread_fep\n");
      pthread_join(proc->pthread_fep, NULL);
      pthread_mutex_destroy(&proc->mutex_fep);
      pthread_cond_destroy(&proc->cond_fep);
    }
    if (ru->feptx_ofdm) {
      pthread_mutex_lock(&proc->mutex_feptx);
      proc->instance_cnt_feptx = 0;
      pthread_mutex_unlock(&proc->mutex_feptx);
      pthread_cond_signal(&proc->cond_feptx);
      LOG_D(PHY, "Joining pthread_feptx\n");
      pthread_join(proc->pthread_feptx, NULL);
      pthread_mutex_destroy(&proc->mutex_feptx);
      pthread_cond_destroy(&proc->cond_feptx);
    }
  }
  if (opp_enabled) {
    LOG_D(PHY, "Joining ru_stats_thread\n");
    pthread_join(ru->ru_stats_thread, NULL);
  }

  pthread_mutex_destroy(&proc->mutex_prach);
  pthread_mutex_destroy(&proc->mutex_asynch_rxtx);
  pthread_mutex_destroy(&proc->mutex_synch);
  pthread_mutex_destroy(&proc->mutex_FH);
  pthread_mutex_destroy(&proc->mutex_eNBs);

  pthread_cond_destroy(&proc->cond_prach);
  pthread_cond_destroy(&proc->cond_FH);
  pthread_cond_destroy(&proc->cond_asynch_rxtx);
  pthread_cond_destroy(&proc->cond_synch);
  pthread_cond_destroy(&proc->cond_eNBs);

  pthread_attr_destroy(&proc->attr_FH);
  pthread_attr_destroy(&proc->attr_prach);
  pthread_attr_destroy(&proc->attr_synch);
  pthread_attr_destroy(&proc->attr_asynch_rxtx);
  pthread_attr_destroy(&proc->attr_fep);

#ifdef Rel14
  pthread_mutex_destroy(&proc->mutex_prach_br);
  pthread_cond_destroy(&proc->cond_prach_br);
  pthread_attr_destroy(&proc->attr_prach_br);
#endif
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

  return(-1);
}


char rru_format_options[4][20] = {"OAI_IF5_only","OAI_IF4p5_only","OAI_IF5_and_IF4p5","MBP_IF5"};

char rru_formats[3][20] = {"OAI_IF5","MBP_IF5","OAI_IF4p5"};
char ru_if_formats[4][20] = {"LOCAL_RF","REMOTE_OAI_IF5","REMOTE_MBP_IF5","REMOTE_OAI_IF4p5"};

void configure_ru(int idx,
		  void *arg) {

  RU_t               *ru           = RC.ru[idx];
  RRU_config_t       *config       = (RRU_config_t *)arg;
  RRU_capabilities_t *capabilities = (RRU_capabilities_t*)arg;
  int ret;

  LOG_I(PHY, "Received capabilities from RRU %d\n",idx);


  if (capabilities->FH_fmt < MAX_FH_FMTs) LOG_I(PHY, "RU FH options %s\n",rru_format_options[capabilities->FH_fmt]);

  AssertFatal((ret=check_capabilities(ru,capabilities)) == 0,
	      "Cannot configure RRU %d, check_capabilities returned %d\n", idx,ret);
  // take antenna capabilities of RRU
  ru->nb_tx                      = capabilities->nb_tx[0];
  ru->nb_rx                      = capabilities->nb_rx[0];

  // Pass configuration to RRU
  LOG_I(PHY, "Using %s fronthaul (%d), band %d \n",ru_if_formats[ru->if_south],ru->if_south,ru->frame_parms.eutra_band);
  // wait for configuration 
  config->FH_fmt                 = ru->if_south;
  config->num_bands              = 1;
  config->band_list[0]           = ru->frame_parms.eutra_band;
  config->tx_freq[0]             = ru->frame_parms.dl_CarrierFreq;      
  config->rx_freq[0]             = ru->frame_parms.ul_CarrierFreq;      
  config->tdd_config[0]          = ru->frame_parms.tdd_config;
  config->tdd_config_S[0]        = ru->frame_parms.tdd_config_S;
  config->att_tx[0]              = ru->att_tx;
  config->att_rx[0]              = ru->att_rx;
  config->N_RB_DL[0]             = ru->frame_parms.N_RB_DL;
  config->N_RB_UL[0]             = ru->frame_parms.N_RB_UL;
  config->threequarter_fs[0]     = ru->frame_parms.threequarter_fs;
  if (ru->if_south==REMOTE_IF4p5) {
    config->prach_FreqOffset[0]  = ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
    config->prach_ConfigIndex[0] = ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
    LOG_I(PHY,"REMOTE_IF4p5: prach_FrequOffset %d, prach_ConfigIndex %d\n",
	  config->prach_FreqOffset[0],config->prach_ConfigIndex[0]);
    
#ifdef Rel14
    int i;
    for (i=0;i<4;i++) {
      config->emtc_prach_CElevel_enable[0][i]  = ru->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i];
      config->emtc_prach_FreqOffset[0][i]      = ru->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[i];
      config->emtc_prach_ConfigIndex[0][i]     = ru->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i];
    }
#endif
  }

  init_frame_parms(&ru->frame_parms,1);
  phy_init_RU(ru);
}

void configure_rru(int idx,
		   void *arg) {

  RRU_config_t *config = (RRU_config_t *)arg;
  RU_t         *ru         = RC.ru[idx];

  ru->frame_parms.eutra_band                                               = config->band_list[0];
  ru->frame_parms.dl_CarrierFreq                                           = config->tx_freq[0];
  ru->frame_parms.ul_CarrierFreq                                           = config->rx_freq[0];
  if (ru->frame_parms.dl_CarrierFreq == ru->frame_parms.ul_CarrierFreq) {
    ru->frame_parms.frame_type                                            = TDD;
    ru->frame_parms.tdd_config                                            = config->tdd_config[0];
    ru->frame_parms.tdd_config_S                                          = config->tdd_config_S[0]; 
  }
  else
    ru->frame_parms.frame_type                                            = FDD;
  ru->att_tx                                                               = config->att_tx[0];
  ru->att_rx                                                               = config->att_rx[0];
  ru->frame_parms.N_RB_DL                                                  = config->N_RB_DL[0];
  ru->frame_parms.N_RB_UL                                                  = config->N_RB_UL[0];
  ru->frame_parms.threequarter_fs                                          = config->threequarter_fs[0];
  ru->frame_parms.pdsch_config_common.referenceSignalPower                 = ru->max_pdschReferenceSignalPower-config->att_tx[0];
  if (ru->function==NGFI_RRU_IF4p5) {
    ru->frame_parms.att_rx = ru->att_rx;
    ru->frame_parms.att_tx = ru->att_tx;

    LOG_I(PHY,"Setting ru->function to NGFI_RRU_IF4p5, prach_FrequOffset %d, prach_ConfigIndex %d, att (%d,%d)\n",
	  config->prach_FreqOffset[0],config->prach_ConfigIndex[0],ru->att_tx,ru->att_rx);
    ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset  = config->prach_FreqOffset[0]; 
    ru->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex = config->prach_ConfigIndex[0]; 
#ifdef Rel14
    for (int i=0;i<4;i++) {
      ru->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i] = config->emtc_prach_CElevel_enable[0][i];
      ru->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[i]     = config->emtc_prach_FreqOffset[0][i];
      ru->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i]    = config->emtc_prach_ConfigIndex[0][i];
    }
#endif
  }
  
  init_frame_parms(&ru->frame_parms,1);

  fill_rf_config(ru,ru->rf_config_file);


//  phy_init_RU(ru);

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
	int nb_tx=0;
	for (ru_id=0;ru_id<RC.nb_RU;ru_id++) { 
	  ru = RC.ru[ru_id];
	  nb_tx+=ru->nb_tx;
	}
	dlsch->ue_spec_bf_weights[layer] = (int32_t**)malloc16(nb_tx*sizeof(int32_t*));
	  
	for (aa=0; aa<nb_tx; aa++) {
	  dlsch->ue_spec_bf_weights[layer][aa] = (int32_t *)malloc16(fp->ofdm_symbol_size*sizeof(int32_t));
	  for (re=0;re<fp->ofdm_symbol_size; re++) {
	    dlsch->ue_spec_bf_weights[layer][aa][re] = 0x00007fff;
	  }
	}	
      }
    }
  }
}

void set_function_spec_param(RU_t *ru)
{
  int ret;

  switch (ru->if_south) {
  case LOCAL_RF:   // this is an RU with integrated RF (RRU, eNB)
    if (ru->function ==  NGFI_RRU_IF5) {                 // IF5 RRU
      ru->do_prach              = 0;                      // no prach processing in RU
      ru->fh_north_in           = NULL;                   // no shynchronous incoming fronthaul from north
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
      reset_meas(&ru->rx_fhaul);
      reset_meas(&ru->tx_fhaul);
      reset_meas(&ru->compression);
      reset_meas(&ru->transport);

      ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
      printf("openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);
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
      ru->feprx                 = (get_nprocs()<=2) ? fep_full :ru_fep_full_2thread;                 // RX DFTs
      ru->feptx_ofdm            = (get_nprocs()<=2) ? feptx_ofdm : feptx_ofdm_2thread;               // this is fep with idft only (no precoding in RRU)
      ru->feptx_prec            = NULL;
      ru->start_if              = start_if;                 // need to start the if interface for if4p5
      ru->ifdevice.host_type    = RRU_HOST;
      ru->rfdevice.host_type    = RRU_HOST;
      ru->ifdevice.eth_params   = &ru->eth_params;
      reset_meas(&ru->rx_fhaul);
      reset_meas(&ru->tx_fhaul);
      reset_meas(&ru->compression);
      reset_meas(&ru->transport);

      ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
      printf("openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);
      if (ret<0) {
        printf("Exiting, cannot initialize transport protocol\n");
        exit(-1);
      }
      malloc_IF4p5_buffer(ru);
    }
    else if (ru->function == eNodeB_3GPP) {
      ru->do_prach             = 0;                       // no prach processing in RU
      ru->feprx                = (get_nprocs()<=2) ? fep_full : ru_fep_full_2thread;                // RX DFTs
      ru->feptx_ofdm           = (get_nprocs()<=2) ? feptx_ofdm : feptx_ofdm_2thread;              // this is fep with idft and precoding
      ru->feptx_prec           = feptx_prec;              // this is fep with idft and precoding
      ru->fh_north_in          = NULL;                    // no incoming fronthaul from north
      ru->fh_north_out         = NULL;                    // no outgoing fronthaul to north
      ru->start_if             = NULL;                    // no if interface
      ru->rfdevice.host_type   = RAU_HOST;
    }
    ru->fh_south_in            = rx_rf;                               // local synchronous RF RX
    ru->fh_south_out           = tx_rf;                               // local synchronous RF TX
    ru->start_rf               = start_rf;                            // need to start the local RF interface
    ru->stop_rf                = stop_rf;
    printf("configuring ru_id %d (start_rf %p)\n", ru->idx, start_rf);
    /*
      if (ru->function == eNodeB_3GPP) { // configure RF parameters only for 3GPP eNodeB, we need to get them from RAU otherwise
      fill_rf_config(ru,rf_config_file);
      init_frame_parms(&ru->frame_parms,1);
      phy_init_RU(ru);
      }

      ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
      if (setup_RU_buffers(ru)!=0) {
      printf("Exiting, cannot initialize RU Buffers\n");
      exit(-1);
      }*/
    break;

  case REMOTE_IF5: // the remote unit is IF5 RRU
    ru->do_prach               = 0;
    ru->feprx                  = (get_nprocs()<=2) ? fep_full : fep_full;                   // this is frequency-shift + DFTs
    ru->feptx_prec             = feptx_prec;                 // need to do transmit Precoding + IDFTs
    ru->feptx_ofdm             = (get_nprocs()<=2) ? feptx_ofdm : feptx_ofdm_2thread;                 // need to do transmit Precoding + IDFTs
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
    ru->stop_rf                = NULL;
    ru->start_if               = start_if;             // need to start if interface for IF5
    ru->ifdevice.host_type     = RAU_HOST;
    ru->ifdevice.eth_params    = &ru->eth_params;
    ru->ifdevice.configure_rru = configure_ru;

    ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
    printf("openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);
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
    ru->stop_rf                = NULL;
    ru->start_if               = start_if;            // need to start if interface for IF4p5
    ru->ifdevice.host_type     = RAU_HOST;
    ru->ifdevice.eth_params    = &ru->eth_params;
    ru->ifdevice.configure_rru = configure_ru;

    ret = openair0_transport_load(&ru->ifdevice, &ru->openair0_cfg, &ru->eth_params);
    printf("openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);
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
}

extern void RCconfig_RU(void);

void init_RU(char *rf_config_file, clock_source_t clock_source,clock_source_t time_source) {
  
  int ru_id;
  RU_t *ru;
  PHY_VARS_eNB *eNB0= (PHY_VARS_eNB *)NULL;
  int i;
  int CC_id;

  // create status mask
  RC.ru_mask = 0;
  pthread_mutex_init(&RC.ru_mutex,NULL);
  pthread_cond_init(&RC.ru_cond,NULL);

  // read in configuration file)
  printf("configuring RU from file\n");
  RCconfig_RU();
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",RC.nb_L1_inst,RC.nb_RU,get_nprocs());

  if (RC.nb_CC != 0)
    for (i=0;i<RC.nb_L1_inst;i++) 
      for (CC_id=0;CC_id<RC.nb_CC[i];CC_id++) RC.eNB[i][CC_id]->num_RU=0;

  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);
  for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {
    LOG_D(PHY,"Process RC.ru[%d]\n",ru_id);
    ru               = RC.ru[ru_id];
    ru->rf_config_file = rf_config_file;
    ru->idx          = ru_id;              
    ru->ts_offset    = 0;
    ru->in_synch     = (ru->is_slave == 1) ? 0 : 1;
    ru->cmd	     = EMPTY;
    // use eNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!
    ru->openair0_cfg.clock_source  = clock_source;
    ru->openair0_cfg.time_source = time_source;
    ;
    
    eNB0             = ru->eNB_list[0];
    LOG_D(PHY, "RU FUnction:%d ru->if_south:%d\n", ru->function, ru->if_south);
    LOG_D(PHY, "eNB0:%p\n", eNB0);
    if (eNB0)
      {
	if ((ru->function != NGFI_RRU_IF5) && (ru->function != NGFI_RRU_IF4p5))
	  AssertFatal(eNB0!=NULL,"eNB0 is null!\n");

	if (eNB0) {
	  LOG_I(PHY,"Copying frame parms from eNB %d to ru %d\n",eNB0->Mod_id,ru->idx);
	  memcpy((void*)&ru->frame_parms,(void*)&eNB0->frame_parms,sizeof(LTE_DL_FRAME_PARMS));

	  // attach all RU to all eNBs in its list/
	  LOG_D(PHY,"ru->num_eNB:%d eNB0->num_RU:%d\n", ru->num_eNB, eNB0->num_RU);
	  for (i=0;i<ru->num_eNB;i++) {
	    eNB0 = ru->eNB_list[i];
	    eNB0->RU_list[eNB0->num_RU++] = ru;
	  }
	}
      }
    //    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s,%d)\n",ru_id,ru_if_types[ru->if_south],eNB_timing[ru->if_timing],ru->function);

    set_function_spec_param(ru);
    LOG_I(PHY,"Starting ru_thread %d\n",ru_id);

    init_RU_proc(ru);





  } // for ru_id

  //  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] RU threads created\n");
  

}



 


void stop_RU(int nb_ru)
{
  for (int inst = 0; inst < nb_ru; inst++) {
    LOG_I(PHY, "Stopping RU %d processing threads\n", inst);
    kill_RU_proc(inst);
  }
}


/* --------------------------------------------------------*/
/* from here function to use configuration module          */
void RCconfig_RU(void) {
  
  int               j                             = 0;
  int               i                             = 0;

  
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};


  config_getlist( &RUParamList,RUParams,sizeof(RUParams)/sizeof(paramdef_t), NULL);  

  
  if ( RUParamList.numelt > 0) {

    RC.ru = (RU_t**)malloc(RC.nb_RU*sizeof(RU_t*));
   



    RC.ru_mask=(1<<NB_RU) - 1;
    printf("Set RU mask to %lx\n",RC.ru_mask);

    for (j = 0; j < RC.nb_RU; j++) {

      RC.ru[j]                                    = (RU_t*)malloc(sizeof(RU_t));
      memset((void*)RC.ru[j],0,sizeof(RU_t));
      RC.ru[j]->idx                                 = j;

      printf("Creating RC.ru[%d]:%p\n", j, RC.ru[j]);

      RC.ru[j]->if_timing                           = synch_to_ext_device;
      if (RC.nb_L1_inst >0)
        RC.ru[j]->num_eNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
      else
	RC.ru[j]->num_eNB                           = 0;
      for (i=0;i<RC.ru[j]->num_eNB;i++) RC.ru[j]->eNB_list[i] = RC.eNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]][0];     


      if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
	if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
	  RC.ru[j]->if_south                        = LOCAL_RF;
	  RC.ru[j]->function                        = eNodeB_3GPP;
          RC.ru[j]->state                           = RU_RUN;
	  printf("Setting function for RU %d to eNodeB_3GPP\n",j);
        }
        else { 
          RC.ru[j]->eth_params.local_if_name            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));    
          RC.ru[j]->eth_params.my_addr                  = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr)); 
          RC.ru[j]->eth_params.remote_addr              = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
          RC.ru[j]->eth_params.my_portc                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.remote_portc             = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.my_portd                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
          RC.ru[j]->eth_params.remote_portd             = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

	  if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_UDP_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF5 (udp)\n",j);
	  } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_RAW_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF5 (raw)\n",j);
	  } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF4p5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_UDP_IF4p5_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF4p5 (udp)\n",j);
	  } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF4p5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_RAW_IF4p5_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF4p5 (raw)\n",j);
	  }
          printf("RU %d is_slave=%s\n",j,*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr));
          if (strcmp(*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr), "yes") == 0) RC.ru[j]->is_slave=1;
          else RC.ru[j]->is_slave=0;
	}
	RC.ru[j]->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
	RC.ru[j]->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
	RC.ru[j]->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;
	for (i=0;i<RC.ru[j]->num_bands;i++) RC.ru[j]->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i]; 
      } //strcmp(local_rf, "yes") == 0
      else {
	printf("RU %d: Transport %s\n",j,*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr));

        RC.ru[j]->eth_params.local_if_name	      = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));    
        RC.ru[j]->eth_params.my_addr		      = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr)); 
        RC.ru[j]->eth_params.remote_addr	      = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.my_portc		      = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.remote_portc	      = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.my_portd		      = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
        RC.ru[j]->eth_params.remote_portd	      = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);
	if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF5;
	  RC.ru[j]->function                     = NGFI_RAU_IF5;
	  RC.ru[j]->eth_params.transp_preference = ETH_UDP_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF5;
	  RC.ru[j]->function                     = NGFI_RAU_IF5;
	  RC.ru[j]->eth_params.transp_preference = ETH_RAW_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF4p5;
	  RC.ru[j]->function                     = NGFI_RAU_IF4p5;
	  RC.ru[j]->eth_params.transp_preference = ETH_UDP_IF4p5_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF4p5;
	  RC.ru[j]->function                     = NGFI_RAU_IF4p5;
	  RC.ru[j]->eth_params.transp_preference = ETH_RAW_IF4p5_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if5_mobipass") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF5;
	  RC.ru[j]->function                     = NGFI_RAU_IF5;
	  RC.ru[j]->if_timing                    = synch_to_other;
	  RC.ru[j]->eth_params.transp_preference = ETH_RAW_IF5_MOBIPASS;
	}
        if (strcmp(*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr), "yes") == 0) RC.ru[j]->is_slave=1;
        else RC.ru[j]->is_slave=0;
      }  /* strcmp(local_rf, "yes") != 0 */

      RC.ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      RC.ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      
      RC.ru[j]->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
      RC.ru[j]->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);
    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;	    
  } // setting != NULL

  return;
  
}
