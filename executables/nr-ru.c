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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/sysinfo.h>
#include <math.h>

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "common/utils/assertions.h"
#include "common/utils/system.h"

#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "PHY/types.h"
#include "PHY/defs_nr_common.h"
#include "PHY/phy_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/INIT/phy_init.h"
#include "SCHED_NR/sched_nr.h"

#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include <executables/softmodem-common.h>

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif

/* these variables have to be defined before including ENB_APP/enb_paramdef.h and GNB_APP/gnb_paramdef.h */
static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};
static int DEFBFW[] = {0x00007fff};

//static int DEFNRBANDS[] = {7};
//static int DEFGNBS[] = {0};

#include "ENB_APP/enb_paramdef.h"
#include "GNB_APP/gnb_paramdef.h"
#include "common/config/config_userapi.h"

#ifndef OPENAIR2
  #include "UTIL/OTG/otg_extern.h"
#endif

#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include <openair1/PHY/TOOLS/phy_scope_interface.h>


#include "T.h"
#include "nfapi_interface.h"

extern volatile int oai_exit;


extern void  nr_phy_free_RU(RU_t *);
extern void  nr_phy_config_request(NR_PHY_Config_t *gNB);
#include "executables/thread-common.h"
//extern PARALLEL_CONF_t get_thread_parallel_conf(void);
//extern WORKER_CONF_t   get_thread_worker_conf(void);

void init_NR_RU(char *);
void stop_RU(int nb_ru);
void do_ru_sync(RU_t *ru);

void configure_ru(int idx, void *arg);
void configure_rru(int idx, void *arg);
int attach_rru(RU_t *ru);
int connect_rau(RU_t *ru);
static void NRRCconfig_RU(void);

uint16_t sf_ahead;
uint16_t slot_ahead;
uint16_t sl_ahead;

extern int emulate_rf;
extern int numerology;

/*************************************************************/
/* Functions to attach and configure RRU                     */

extern void wait_gNBs(void);

int attach_rru(RU_t *ru) {
  ssize_t      msg_len,len;
  RRU_CONFIG_msg_t rru_config_msg;
  int received_capabilities=0;
  wait_gNBs();

  // Wait for capabilities
  while (received_capabilities==0) {
    memset((void *)&rru_config_msg,0,sizeof(rru_config_msg));
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
    } else if (rru_config_msg.type == RRU_capabilities) {
      AssertFatal(rru_config_msg.len==msg_len,"Received capabilities with incorrect length (%d!=%d)\n",(int)rru_config_msg.len,(int)msg_len);
      LOG_I(PHY,"Received capabilities from RRU %d (len %d/%d, num_bands %d,max_pdschReferenceSignalPower %d, max_rxgain %d, nb_tx %d, nb_rx %d)\n",ru->idx,
            (int)rru_config_msg.len,(int)msg_len,
            ((RRU_capabilities_t *)&rru_config_msg.msg[0])->num_bands,
            ((RRU_capabilities_t *)&rru_config_msg.msg[0])->max_pdschReferenceSignalPower[0],
            ((RRU_capabilities_t *)&rru_config_msg.msg[0])->max_rxgain[0],
            ((RRU_capabilities_t *)&rru_config_msg.msg[0])->nb_tx[0],
            ((RRU_capabilities_t *)&rru_config_msg.msg[0])->nb_rx[0]);
      received_capabilities=1;
    } else {
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

  if ((len = ru->ifdevice.trx_ctlrecv_func(&ru->ifdevice,
             &rru_config_msg,
             msg_len))<0) {
    LOG_I(PHY,"Waiting for RRU %d\n",ru->idx);
  } else if (rru_config_msg.type == RRU_config_ok) {
    LOG_I(PHY, "RRU_config_ok received\n");
  } else {
    LOG_E(PHY,"Received incorrect message %d from RRU %d\n",rru_config_msg.type,ru->idx);
  }

  return 0;
}

int connect_rau(RU_t *ru) {
  RRU_CONFIG_msg_t   rru_config_msg;
  ssize_t            msg_len;
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
    } else {
      if (rru_config_msg.type == RAU_tick) {
        LOG_I(PHY,"Tick received from RAU\n");
        tick_received = 1;
      } else LOG_E(PHY,"Received erroneous message (%d)from RAU, expected RAU_tick\n",rru_config_msg.type);
    }
  }

  // send capabilities
  rru_config_msg.type = RRU_capabilities;
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_capabilities_t);
  cap                 = (RRU_capabilities_t *)&rru_config_msg.msg[0];
  LOG_I(PHY,"Sending Capabilities (len %d, num_bands %d,max_pdschReferenceSignalPower %d, max_rxgain %d, nb_tx %d, nb_rx %d)\n",
        (int)rru_config_msg.len,ru->num_bands,ru->max_pdschReferenceSignalPower,ru->max_rxgain,ru->nb_tx,ru->nb_rx);

  switch (ru->function) {
    case NGFI_RRU_IF4p5:
      cap->FH_fmt                                 = OAI_IF4p5_only;
      break;

    case NGFI_RRU_IF5:
      cap->FH_fmt                                 = OAI_IF5_only;
      break;

    case MBP_RRU_IF5:
      cap->FH_fmt                                 = MBP_IF5;
      break;

    default:
      AssertFatal(1==0,"RU_function is unknown %d\n",RC.ru[0]->function);
      break;
  }

  cap->num_bands                                  = ru->num_bands;

  for (i=0; i<ru->num_bands; i++) {
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
    } else {
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
                    (void *)&rru_config_msg.msg[0]);
      configuration_received = 1;
    }
  }

  return 0;
}
/*************************************************************/
/* Southbound Fronthaul functions, RCC/RAU                   */

// southbound IF5 fronthaul for 16-bit OAI format
void fh_if5_south_out(RU_t *ru, int frame, int slot, uint64_t timestamp) {
  if (ru == RC.ru[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );

  send_IF5(ru, timestamp, slot, &ru->seqno, IF5_RRH_GW_DL);
}

// southbound IF4p5 fronthaul
void fh_if4p5_south_out(RU_t *ru, int frame, int slot, uint64_t timestamp) {
  nfapi_nr_config_request_scf_t *cfg = &ru->config;

  if (ru == RC.ru[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );

  LOG_D(PHY,"Sending IF4p5 for frame %d subframe %d\n",ru->proc.frame_tx,ru->proc.tti_tx);

  if ((nr_slot_select(cfg,ru->proc.frame_tx,ru->proc.tti_tx)&NR_DOWNLINK_SLOT) > 0)
    send_IF4p5(ru,frame, slot, IF4p5_PDLFFT);
}

/*************************************************************/
/* Input Fronthaul from south RCC/RAU                        */

// Synchronous if5 from south

uint64_t ts_rx[20];
void fh_if5_south_in(RU_t *ru,
                     int *frame,
                     int *tti) {
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  RU_proc_t *proc = &ru->proc;
  recv_IF5(ru, &proc->timestamp_rx, *tti, IF5_RRH_GW_UL);
  if (proc->first_rx == 1) ru->ts_offset = proc->timestamp_rx;
  proc->frame_rx    = ((proc->timestamp_rx-ru->ts_offset) / (fp->samples_per_subframe*10))&1023;
  proc->tti_rx = fp->get_slot_from_timestamp(proc->timestamp_rx-ru->ts_offset,fp);
//(idx_sf * fp->slots_per_subframe + (int)round((float)(proc->timestamp_rx % fp->samples_per_subframe) / fp->samples_per_slot0))%(fp->slots_per_frame);
  ts_rx[*tti] = proc->timestamp_rx;
  LOG_D(PHY,"IF5 %d.%d => RX %d.%d first_rx %d\n",*frame,*tti,proc->frame_rx,proc->tti_rx,proc->first_rx); 

  if (proc->first_rx == 0) {
    if (proc->tti_rx != *tti) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->tti_rx %d, subframe %d)\n",proc->tti_rx,*tti);
      exit_fun("Exiting");
    }

    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d proc->tti_rx %d tti %d)\n",proc->frame_rx,*frame,proc->tti_rx,*tti);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *tti = proc->tti_rx;
  }

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
}

// Synchronous if4p5 from south
void fh_if4p5_south_in(RU_t *ru,
                       int *frame,
                       int *slot) {
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  RU_proc_t *proc = &ru->proc;
  int f,sl;
  uint16_t packet_type;
  uint32_t symbol_number=0;
  uint32_t symbol_mask_full=0;

  /*
    if ((fp->frame_type == TDD) && (subframe_select(fp,*slot)==SF_S))
      symbol_mask_full = (1<<fp->ul_symbols_in_S_subframe)-1;
    else
      symbol_mask_full = (1<<fp->symbols_per_slot)-1;

    AssertFatal(proc->symbol_mask[*slot]==0,"rx_fh_if4p5: proc->symbol_mask[%d] = %x\n",*slot,proc->symbol_mask[*slot]);*/
  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &f, &sl, &packet_type, &symbol_number);

    if (packet_type == IF4p5_PULFFT) proc->symbol_mask[sl] = proc->symbol_mask[sl] | (1<<symbol_number);
    else if (packet_type == IF4p5_PULTICK) {
      if ((proc->first_rx==0) && (f!=*frame)) LOG_E(PHY,"rx_fh_if4p5: PULTICK received frame %d != expected %d\n",f,*frame);

      if ((proc->first_rx==0) && (sl!=*slot)) LOG_E(PHY,"rx_fh_if4p5: PULTICK received subframe %d != expected %d (first_rx %d)\n",sl,*slot,proc->first_rx);

      break;
    } else if (packet_type == IF4p5_PRACH) {
      // nothing in RU for RAU
    }

    LOG_D(PHY,"rx_fh_if4p5: subframe %d symbol mask %x\n",*slot,proc->symbol_mask[sl]);
  } while(proc->symbol_mask[sl] != symbol_mask_full);

  //caculate timestamp_rx, timestamp_tx based on frame and subframe
  proc->tti_rx   = sl;
  proc->frame_rx = f;
  proc->timestamp_rx = (proc->frame_rx * fp->samples_per_subframe * 10)  + fp->get_samples_slot_timestamp(proc->tti_rx, fp, 0);
  //  proc->timestamp_tx = proc->timestamp_rx +  (4*fp->samples_per_subframe);
  proc->tti_tx   = (sl+(fp->slots_per_subframe*sf_ahead))%fp->slots_per_frame;
  proc->frame_tx = (sl>(fp->slots_per_frame-1-(fp->slots_per_subframe*sf_ahead))) ? (f+1)&1023 : f;

  if (proc->first_rx == 0) {
    if (proc->tti_rx != *slot) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->tti_rx %d, subframe %d)\n",proc->tti_rx,*slot);
      exit_fun("Exiting");
    }

    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *slot = proc->tti_rx;
  }

  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU, f );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_RX0_RU,  sl);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, proc->frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, proc->tti_tx );
  }

  proc->symbol_mask[proc->tti_rx] = 0;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  LOG_D(PHY,"RU %d: fh_if4p5_south_in sleeping ...\n",ru->idx);
}

// asynchronous inbound if4p5 fronthaul from south
void fh_if4p5_south_asynch_in(RU_t *ru,int *frame,int *slot) {
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  RU_proc_t *proc       = &ru->proc;
  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,prach_rx;
  //  uint32_t got_prach_info=0;
  symbol_number = 0;
  symbol_mask   = (1<<(fp->symbols_per_slot))-1;
  prach_rx      = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &proc->frame_rx, &proc->tti_rx, &packet_type, &symbol_number);

    // grab first prach information for this new subframe
    /*if (got_prach_info==0) {
      prach_rx       = is_prach_subframe(fp, proc->frame_rx, proc->tti_rx);
      got_prach_info = 1;
    }*/
    if (proc->first_rx != 0) {
      *frame = proc->frame_rx;
      *slot = proc->tti_rx;
      proc->first_rx = 0;
    } else {
      if (proc->frame_rx != *frame) {
        LOG_E(PHY,"frame_rx %d is not what we expect %d\n",proc->frame_rx,*frame);
        exit_fun("Exiting");
      }

      if (proc->tti_rx != *slot) {
        LOG_E(PHY,"tti_rx %d is not what we expect %d\n",proc->tti_rx,*slot);
        exit_fun("Exiting");
      }
    }

    if      (packet_type == IF4p5_PULFFT)       symbol_mask &= (~(1<<symbol_number));
    else if (packet_type == IF4p5_PRACH)        prach_rx    &= (~0x1);
  } while( (symbol_mask > 0) || (prach_rx >0));   // haven't received all PUSCH symbols and PRACH information
}





/*************************************************************/
/* Input Fronthaul from North RRU                            */

// RRU IF4p5 TX fronthaul receiver. Assumes an if_device on input and if or rf device on output
// receives one subframe's worth of IF4p5 OFDM symbols and OFDM modulates
void fh_if4p5_north_in(RU_t *ru,int *frame,int *slot) {
  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;
  uint16_t packet_type;
  /// **** incoming IF4p5 from remote RCC/RAU **** ///
  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<(ru->nr_frame_parms->symbols_per_slot))-1;

  do {
    recv_IF4p5(ru, frame, slot, &packet_type, &symbol_number);
    symbol_mask = symbol_mask | (1<<symbol_number);
  } while (symbol_mask != symbol_mask_full);

  // dump VCD output for first RU in list
  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, *frame );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, *slot );
  }
}

void fh_if5_north_asynch_in(RU_t *ru,int *frame,int *slot) {
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  RU_proc_t *proc        = &ru->proc;
  int tti_tx,frame_tx;
  openair0_timestamp timestamp_tx;
  recv_IF5(ru, &timestamp_tx, *slot, IF5_RRH_GW_DL);
  //      printf("Received subframe %d (TS %llu) from RCC\n",tti_tx,timestamp_tx);
  frame_tx    = (timestamp_tx / (fp->samples_per_subframe*10))&1023;
  uint32_t idx_sf = timestamp_tx / fp->samples_per_subframe;
  tti_tx = (idx_sf * fp->slots_per_subframe + (int)round((float)(timestamp_tx % fp->samples_per_subframe) / fp->samples_per_slot0))%(fp->slots_per_frame);

  if (proc->first_tx != 0) {
    *slot = tti_tx;
    *frame    = frame_tx;
    proc->first_tx = 0;
  } else {
    AssertFatal(tti_tx == *slot,
                "tti_tx %d is not what we expect %d\n",tti_tx,*slot);
    AssertFatal(frame_tx == *frame,
                "frame_tx %d is not what we expect %d\n",frame_tx,*frame);
  }
}

void fh_if4p5_north_asynch_in(RU_t *ru,int *frame,int *slot) {
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &ru->config;
  RU_proc_t *proc        = &ru->proc;
  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,symbol_mask_full=0;
  int slot_tx,frame_tx;
  LOG_D(PHY, "%s(ru:%p frame, subframe)\n", __FUNCTION__, ru);
  symbol_number = 0;
  symbol_mask = 0;

  //  symbol_mask_full = ((subframe_select(fp,*slot) == SF_S) ? (1<<fp->dl_symbols_in_S_subframe) : (1<<fp->symbols_per_slot))-1;
  do {
    recv_IF4p5(ru, &frame_tx, &slot_tx, &packet_type, &symbol_number);

    if (((nr_slot_select(cfg,frame_tx,slot_tx) & NR_DOWNLINK_SLOT) > 0) && (symbol_number == 0)) start_meas(&ru->rx_fhaul);

    LOG_D(PHY,"slot %d (%d): frame %d, slot %d, symbol %d\n",
          *slot,nr_slot_select(cfg,frame_tx,*slot),frame_tx,slot_tx,symbol_number);

    if (proc->first_tx != 0) {
      *frame         = frame_tx;
      *slot          = slot_tx;
      proc->first_tx = 0;
      //symbol_mask_full = ((subframe_select(fp,*slot) == SF_S) ? (1<<fp->dl_symbols_in_S_subframe) : (1<<fp->symbols_per_slot))-1;
    } else {
      AssertFatal(frame_tx == *frame,
                  "frame_tx %d is not what we expect %d\n",frame_tx,*frame);
      AssertFatal(slot_tx == *slot,
                  "slot_tx %d is not what we expect %d\n",slot_tx,*slot);
    }

    if (packet_type == IF4p5_PDLFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
    } else AssertFatal(1==0,"Illegal IF4p5 packet type (should only be IF4p5_PDLFFT%d\n",packet_type);
  } while (symbol_mask != symbol_mask_full);

  if ((nr_slot_select(cfg,frame_tx,slot_tx) & NR_DOWNLINK_SLOT)>0) stop_meas(&ru->rx_fhaul);

  proc->tti_tx = slot_tx;
  proc->frame_tx = frame_tx;

  if ((frame_tx == 0)&&(slot_tx == 0)) proc->frame_tx_unwrap += 1024;

  proc->timestamp_tx = (((uint64_t)frame_tx + (uint64_t)proc->frame_tx_unwrap) * fp->samples_per_subframe * 10) + fp->get_samples_slot_timestamp(slot_tx, fp, 0);
  LOG_D(PHY,"RU %d/%d TST %llu, frame %d, subframe %d\n",ru->idx,0,(long long unsigned int)proc->timestamp_tx,frame_tx,slot_tx);

  // dump VCD output for first RU in list
  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, slot_tx );
  }

  if (ru->feptx_ofdm) ru->feptx_ofdm(ru,frame_tx,slot_tx);

  if (ru->fh_south_out) ru->fh_south_out(ru,frame_tx,slot_tx,proc->timestamp_tx);
}

void fh_if5_north_out(RU_t *ru) {
  RU_proc_t *proc=&ru->proc;
  uint8_t seqno=0;
  /// **** send_IF5 of rxdata to BBU **** ///
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 1 );
  send_IF5(ru, proc->timestamp_rx, proc->tti_rx, &seqno, IF5_RRH_GW_UL);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 0 );
}

// RRU IF4p5 northbound interface (RX)
void fh_if4p5_north_out(RU_t *ru) {
  RU_proc_t *proc=&ru->proc;

  //NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  //const int subframe     = proc->tti_rx;
  if (ru->idx==0) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_RX0_RU, proc->tti_rx );

  /*
    if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) {
      /// **** in TDD during DL send_IF4 of ULTICK to RCC **** ///
      send_IF4p5(ru, proc->frame_rx, proc->tti_rx, IF4p5_PULTICK);
      return;
    }*/
  start_meas(&ru->tx_fhaul);
  send_IF4p5(ru, proc->frame_rx, proc->tti_rx, IF4p5_PULFFT);
  stop_meas(&ru->tx_fhaul);
}

void *emulatedRF_thread(void *param) {
  RU_proc_t *proc = (RU_proc_t *) param;
  int microsec = 500; // length of time to sleep, in miliseconds
  struct timespec req = {0};
  req.tv_sec = 0;
  req.tv_nsec = (numerology>0)? ((microsec * 1000L)/numerology):(microsec * 1000L)*2;
  wait_sync("emulatedRF_thread");

  while(!oai_exit) {
    nanosleep(&req, (struct timespec *)NULL);
    pthread_mutex_lock(&proc->mutex_emulateRF);
    ++proc->instance_cnt_emulateRF;
    pthread_mutex_unlock(&proc->mutex_emulateRF);
    pthread_cond_signal(&proc->cond_emulateRF);
  }

  return 0;
}

void rx_rf(RU_t *ru,int *frame,int *slot) {
  RU_proc_t *proc = &ru->proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  void *rxp[ru->nb_rx];
  unsigned int rxs;
  int i;
  uint32_t samples_per_slot = fp->get_samples_per_slot(*slot,fp);
  uint32_t samples_per_slot_prev ;
  openair0_timestamp ts,old_ts;
  AssertFatal(*slot<fp->slots_per_frame && *slot>=0, "slot %d is illegal (%d)\n",*slot,fp->slots_per_frame);

  for (i=0; i<ru->nb_rx; i++)
    rxp[i] = (void *)&ru->common.rxdata[i][fp->get_samples_slot_timestamp(*slot,fp,0)];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );
  old_ts = proc->timestamp_rx;
  LOG_D(PHY,"Reading %d samples for slot %d (%p)\n",samples_per_slot,*slot,rxp[0]);

  if(emulate_rf) {
    wait_on_condition(&proc->mutex_emulateRF,&proc->cond_emulateRF,&proc->instance_cnt_emulateRF,"emulatedRF_thread");
    release_thread(&proc->mutex_emulateRF,&proc->instance_cnt_emulateRF,"emulatedRF_thread");
    rxs = samples_per_slot;
    ts = old_ts + rxs;
  } else {
    rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                     &ts,
                                     rxp,
                                     samples_per_slot,
                                     ru->nb_rx);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
  proc->timestamp_rx = ts-ru->ts_offset;

  //AssertFatal(rxs == fp->samples_per_subframe,
  //"rx_rf: Asked for %d samples, got %d from USRP\n",fp->samples_per_subframe,rxs);
  if (rxs != samples_per_slot) LOG_E(PHY, "rx_rf: Asked for %d samples, got %d from USRP\n",samples_per_slot,rxs);

  if (proc->first_rx == 1) {
    ru->ts_offset = proc->timestamp_rx;
    proc->timestamp_rx = 0;
  } else {
    samples_per_slot_prev = fp->get_samples_per_slot((*slot-1)%fp->slots_per_frame,fp);

    if (proc->timestamp_rx - old_ts != samples_per_slot_prev) {
      LOG_D(PHY,"rx_rf: rfdevice timing drift of %"PRId64" samples (ts_off %"PRId64")\n",proc->timestamp_rx - old_ts - samples_per_slot_prev,ru->ts_offset);
      ru->ts_offset += (proc->timestamp_rx - old_ts - samples_per_slot_prev);
      proc->timestamp_rx = ts-ru->ts_offset;
    }
  }

  proc->frame_rx    = (proc->timestamp_rx / (fp->samples_per_subframe*10))&1023;
  proc->tti_rx = fp->get_slot_from_timestamp(proc->timestamp_rx,fp);
  // synchronize first reception to frame 0 subframe 0
  LOG_D(PHY,"RU %d/%d TS %llu (off %d), frame %d, slot %d.%d / %d\n",
        ru->idx,
        0,
        (unsigned long long int)proc->timestamp_rx,
        (int)ru->ts_offset,proc->frame_rx,proc->tti_rx,proc->tti_tx,fp->slots_per_frame);

  // dump VCD output for first RU in list
  if (ru == RC.ru[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU, proc->frame_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_RX0_RU, proc->tti_rx );
  }

  if (proc->first_rx == 0) {
    if (proc->tti_rx != *slot) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->tti_rx %d, slot %d)\n",(long long unsigned int)proc->timestamp_rx,proc->tti_rx,*slot);
      exit_fun("Exiting");
    }

    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d, proc->tti_rx %d, slot %d)\n",(long long unsigned int)proc->timestamp_rx,proc->frame_rx,
            *frame,proc->tti_rx,*slot);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *slot  = proc->tti_rx;
  }

  //printf("timestamp_rx %lu, frame %d(%d), subframe %d(%d)\n",ru->timestamp_rx,proc->frame_rx,frame,proc->tti_rx,subframe);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );

  if (rxs != samples_per_slot) {
    //exit_fun( "problem receiving samples" );
    LOG_E(PHY, "problem receiving samples\n");
  }
}


void tx_rf(RU_t *ru,int frame,int slot, uint64_t timestamp) {
  RU_proc_t *proc = &ru->proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &ru->config;
  void *txp[ru->nb_tx];
  unsigned int txs;
  int i,txsymb;
  T(T_ENB_PHY_OUTPUT_SIGNAL, T_INT(0), T_INT(0), T_INT(frame), T_INT(slot),
    T_INT(0), T_BUFFER(&ru->common.txdata[0][fp->get_samples_slot_timestamp(slot,fp,0)], fp->samples_per_subframe * 4));
  int slot_type         = nr_slot_select(cfg,frame,slot%fp->slots_per_frame);
  int prevslot_type     = nr_slot_select(cfg,frame,(slot+(fp->slots_per_frame-1))%fp->slots_per_frame);
  int nextslot_type     = nr_slot_select(cfg,frame,(slot+1)%fp->slots_per_frame);
  int sf_extension  = 0;                 //sf_extension = ru->sf_extension;
  int siglen=fp->get_samples_per_slot(slot,fp);
  int flags=1;

  //nr_subframe_t SF_type     = nr_slot_select(cfg,slot%fp->slots_per_frame);
  if (slot_type == NR_DOWNLINK_SLOT || slot_type == NR_MIXED_SLOT || IS_SOFTMODEM_RFSIM) {
    if (cfg->cell_config.frame_duplex_type.value == TDD) {
      if(slot_type == NR_MIXED_SLOT) {
        txsymb = 0;

        for(int symbol_count = 0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
          if (cfg->tdd_table.max_tdd_periodicity_list[slot].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value == 0)
            txsymb++;
        }

        AssertFatal(txsymb>0,"illegal txsymb %d\n",txsymb);

        if(slot%(fp->slots_per_subframe/2))
          siglen = txsymb * (fp->ofdm_symbol_size + fp->nb_prefix_samples);
        else
          siglen = (fp->ofdm_symbol_size + fp->nb_prefix_samples0) + (txsymb - 1) * (fp->ofdm_symbol_size + fp->nb_prefix_samples);

        //+ ru->end_of_burst_delay;
        flags = 3; // end of burst
      }

      if (slot_type == NR_DOWNLINK_SLOT && prevslot_type == NR_UPLINK_SLOT)
        flags = 2; // start of burst

      if (slot_type == NR_DOWNLINK_SLOT && nextslot_type == NR_UPLINK_SLOT)
        flags = 3; // end of burst
    }

    if (fp->freq_range==nr_FR2) {
      // the beam index is written in bits 8-10 of the flags
      // bit 11 enables the gpio programming
      // currently we switch beams every 10 slots (should = 1 TDD period in FR2) and we take the beam index of the first symbol of the first slot of this period
      int beam=0;

      if (slot%10==0) {
        if ( ru->common.beam_id && (ru->common.beam_id[0][slot*fp->symbols_per_slot] < 8)) {
          beam = ru->common.beam_id[0][slot*fp->symbols_per_slot] | 8;
        }
      }

      /*
      if (slot==0 || slot==40) beam=0|8;
      if (slot==10 || slot==50) beam=1|8;
      if (slot==20 || slot==60) beam=2|8;
      if (slot==30 || slot==70) beam=3|8;
      */
      flags |= beam<<8;
      LOG_D(HW,"slot %d, beam %d\n",slot,ru->common.beam_id[0][slot*fp->symbols_per_slot]);
    }

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_WRITE_FLAGS, flags );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, frame );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, slot );

    for (i=0; i<ru->nb_tx; i++)
      txp[i] = (void *)&ru->common.txdata[i][fp->get_samples_slot_timestamp(slot,fp,0)-sf_extension];
    
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (timestamp-ru->openair0_cfg.tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
      // prepare tx buffer pointers
    txs = ru->rfdevice.trx_write_func(&ru->rfdevice,
                                      timestamp+ru->ts_offset-ru->openair0_cfg.tx_sample_advance-sf_extension,
                                      txpsiglen+sf_extension,
                                      siglen+sf_extension,
                                      ru->nb_tx,
                                      flags);
    LOG_D(PHY,"[TXPATH] RU %d aa %d tx_rf, writing to TS %llu, frame %d, unwrapped_frame %d, slot %d, returned %d, E %f\n",ru->idx,i,
	  (long long unsigned int)timestamp,frame,proc->frame_tx_unwrap,slot, txs,10*log10((double)signal_energy(txp[0],siglen+sf_extension)));
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );
      //AssertFatal(txs == 0,"trx write function error %d\n", txs);
  }
}

// this is for RU with local RF unit
void fill_rf_config(RU_t *ru, char *rf_config_file) {
  int i;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;
  nfapi_nr_config_request_scf_t *config = &ru->config; //tmp index
  openair0_config_t *cfg   = &ru->openair0_cfg;
  int mu = config->ssb_config.scs_common.value;
  int N_RB = config->carrier_config.dl_grid_size[config->ssb_config.scs_common.value].value;

  if (mu == NR_MU_0) {
    switch(N_RB) {
    case 270:
      if (fp->threequarter_fs) {
        cfg->sample_rate=92.16e6;
        cfg->samples_per_frame = 921600;
        cfg->tx_bw = 50e6;
        cfg->rx_bw = 50e6;
      } else {
        cfg->sample_rate=61.44e6;
        cfg->samples_per_frame = 614400;
        cfg->tx_bw = 50e6;
        cfg->rx_bw = 50e6;
      }
    case 216:
      if (fp->threequarter_fs) {
        cfg->sample_rate=46.08e6;
        cfg->samples_per_frame = 460800;
        cfg->tx_bw = 40e6;
        cfg->rx_bw = 40e6;
      }
      else {
        cfg->sample_rate=61.44e6;
        cfg->samples_per_frame = 614400;
        cfg->tx_bw = 40e6;
        cfg->rx_bw = 40e6;
      }
      break; 
    case 160: //30 MHz
    case 133: //25 MHz
      if (fp->threequarter_fs) {
        AssertFatal(1==0,"N_RB %d cannot use 3/4 sampling\n",N_RB);
      }
      else {
        cfg->sample_rate=30.72e6;
        cfg->samples_per_frame = 307200;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      }
    case 106:
      if (fp->threequarter_fs) {
        cfg->sample_rate=23.04e6;
        cfg->samples_per_frame = 230400;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      }
      else {
        cfg->sample_rate=30.72e6;
        cfg->samples_per_frame = 307200;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      }
      break;
    case 52:
      if (fp->threequarter_fs) {
        cfg->sample_rate=11.52e6;
        cfg->samples_per_frame = 115200;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      }
      else {
        cfg->sample_rate=15.36e6;
        cfg->samples_per_frame = 153600;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      }
    case 25:
      if (fp->threequarter_fs) {
        cfg->sample_rate=5.76e6;
        cfg->samples_per_frame = 57600;
        cfg->tx_bw = 5e6;
        cfg->rx_bw = 5e6;
      }
      else {
        cfg->sample_rate=7.68e6;
        cfg->samples_per_frame = 76800;
        cfg->tx_bw = 5e6;
        cfg->rx_bw = 5e6;
      }
      break;
    default:
      AssertFatal(0==1,"N_RB %d not yet supported for numerology %d\n",N_RB,mu);
    }
  } else if (mu == NR_MU_1) {
    switch(N_RB) {

    case 273:
      if (fp->threequarter_fs) {
        cfg->sample_rate=184.32e6;
        cfg->samples_per_frame = 1843200;
        cfg->tx_bw = 100e6;
        cfg->rx_bw = 100e6;
      } else {
        cfg->sample_rate=122.88e6;
        cfg->samples_per_frame = 1228800;
        cfg->tx_bw = 100e6;
        cfg->rx_bw = 100e6;
      }
      break;
    case 217:
      if (fp->threequarter_fs) {
        cfg->sample_rate=92.16e6;
        cfg->samples_per_frame = 921600;
        cfg->tx_bw = 80e6;
        cfg->rx_bw = 80e6;
      } else {
        cfg->sample_rate=122.88e6;
        cfg->samples_per_frame = 1228800;
        cfg->tx_bw = 80e6;
        cfg->rx_bw = 80e6;
      }
      break;
    case 162 :
      if (fp->threequarter_fs) {
        AssertFatal(1==0,"N_RB %d cannot use 3/4 sampling\n",N_RB);
      }
      else {
        cfg->sample_rate=61.44e6;
        cfg->samples_per_frame = 614400;
        cfg->tx_bw = 60e6;
        cfg->rx_bw = 60e6;
      }

      break;

    case 133 :
      if (fp->threequarter_fs) {
	AssertFatal(1==0,"N_RB %d cannot use 3/4 sampling\n",N_RB);
      }
      else {
        cfg->sample_rate=61.44e6;
        cfg->samples_per_frame = 614400;
        cfg->tx_bw = 50e6;
        cfg->rx_bw = 50e6;
      }

      break;
    case 106:
      if (fp->threequarter_fs) {
        cfg->sample_rate=46.08e6;
        cfg->samples_per_frame = 460800;
        cfg->tx_bw = 40e6;
        cfg->rx_bw = 40e6;
      }
      else {
        cfg->sample_rate=61.44e6;
        cfg->samples_per_frame = 614400;
        cfg->tx_bw = 40e6;
        cfg->rx_bw = 40e6;
      }
     break;
    case 51:
      if (fp->threequarter_fs) {
        cfg->sample_rate=23.04e6;
        cfg->samples_per_frame = 230400;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      }
      else {
        cfg->sample_rate=30.72e6;
        cfg->samples_per_frame = 307200;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      }
      break;
    case 24:
      if (fp->threequarter_fs) {
        cfg->sample_rate=11.52e6;
        cfg->samples_per_frame = 115200;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      }
      else {
        cfg->sample_rate=15.36e6;
        cfg->samples_per_frame = 153600;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      }
      break;
    default:
      AssertFatal(0==1,"N_RB %d not yet supported for numerology %d\n",N_RB,mu);
    }
  } else if (mu == NR_MU_3) {
    switch(N_RB) {
      case 66:
        if (fp->threequarter_fs) {
          cfg->sample_rate=184.32e6;
          cfg->samples_per_frame = 1843200;
          cfg->tx_bw = 100e6;
          cfg->rx_bw = 100e6;
        } else {
          cfg->sample_rate = 122.88e6;
          cfg->samples_per_frame = 1228800;
          cfg->tx_bw = 100e6;
          cfg->rx_bw = 100e6;
        }

        break;

      case 32:
        if (fp->threequarter_fs) {
          cfg->sample_rate=92.16e6;
          cfg->samples_per_frame = 921600;
          cfg->tx_bw = 50e6;
          cfg->rx_bw = 50e6;
        } else {
          cfg->sample_rate=61.44e6;
          cfg->samples_per_frame = 614400;
          cfg->tx_bw = 50e6;
          cfg->rx_bw = 50e6;
        }

        break;

      default:
        AssertFatal(0==1,"N_RB %d not yet supported for numerology %d\n",N_RB,mu);
    }
  } else {
    AssertFatal(0 == 1,"Numerology %d not supported for the moment\n",mu);
  }

  if (config->cell_config.frame_duplex_type.value==TDD)
    cfg->duplex_mode = duplex_mode_TDD;
  else //FDD
    cfg->duplex_mode = duplex_mode_FDD;

  cfg->Mod_id = 0;
  cfg->num_rb_dl=N_RB;
  cfg->tx_num_channels=ru->nb_tx;
  cfg->rx_num_channels=ru->nb_rx;

  for (i=0; i<ru->nb_tx; i++) {
    if (ru->if_frequency == 0) {
      cfg->tx_freq[i] = (double)fp->dl_CarrierFreq;
      cfg->rx_freq[i] = (double)fp->ul_CarrierFreq;
    } else if (ru->if_freq_offset) {
      cfg->tx_freq[i] = (double)(ru->if_frequency);
      cfg->rx_freq[i] = (double)(ru->if_frequency + ru->if_freq_offset);
      LOG_I(PHY, "Setting IF TX frequency to %lu Hz with IF RX frequency offset %d Hz\n", ru->if_frequency, ru->if_freq_offset);
    } else {
      cfg->tx_freq[i] = (double)ru->if_frequency;
      cfg->rx_freq[i] = (double)(ru->if_frequency+fp->ul_CarrierFreq-fp->dl_CarrierFreq);
    }

    cfg->tx_gain[i] = ru->att_tx;
    cfg->rx_gain[i] = ru->max_rxgain-ru->att_rx;
    cfg->configFilename = rf_config_file;
    LOG_I(PHY, "Channel %d: setting tx_gain offset %f, rx_gain offset %f, tx_freq %lu Hz, rx_freq %lu Hz\n",
          i, cfg->tx_gain[i],
          cfg->rx_gain[i],
          (unsigned long)cfg->tx_freq[i],
          (unsigned long)cfg->rx_freq[i]);
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
  NR_DL_FRAME_PARMS *fp;
  nfapi_nr_config_request_scf_t *config = &ru->config;

  if (ru) {
    fp = ru->nr_frame_parms;
    printf("setup_RU_buffers: frame_parms = %p\n",fp);
  } else {
    printf("ru pointer is NULL\n");
    return(-1);
  }

  int mu = config->ssb_config.scs_common.value;
  int N_RB = config->carrier_config.dl_grid_size[config->ssb_config.scs_common.value].value;

  if (config->cell_config.frame_duplex_type.value == TDD) {
    int N_TA_offset =  config->carrier_config.uplink_frequency.value < 6000000 ? 400 : 431; // reference samples  for 25600Tc @ 30.72 Ms/s for FR1, same @ 61.44 Ms/s for FR2
    double factor=1;

    switch (mu) {
      case 0: //15 kHz scs
        AssertFatal(N_TA_offset == 400, "scs_common 15kHz only for FR1\n");
        factor = fp->samples_per_subframe / 30720.0;
        break;

      case 1: //30 kHz sc
        AssertFatal(N_TA_offset == 400, "scs_common 30kHz only for FR1\n");
        factor = fp->samples_per_subframe / 30720.0;
        break;

      case 2: //60 kHz scs
        AssertFatal(1==0, "scs_common should not be 60 kHz\n");
        break;

      case 3: //120 kHz scs
        AssertFatal(N_TA_offset == 431, "scs_common 120kHz only for FR2\n");
        factor = fp->samples_per_subframe / 61440.0;
        break;

      case 4: //240 kHz scs
        AssertFatal(N_TA_offset == 431, "scs_common 240kHz only for FR2\n");
        factor = fp->samples_per_subframe / 61440.0;
        break;

      default:
        AssertFatal(1==0, "Invalid scs_common!\n");
    }

    ru->N_TA_offset = (int)(N_TA_offset * factor);
    LOG_I(PHY,"RU %d Setting N_TA_offset to %d samples (factor %f, UL Freq %d, N_RB %d, mu %d)\n",ru->idx,ru->N_TA_offset,factor,
          config->carrier_config.uplink_frequency.value, N_RB, mu);
  } else ru->N_TA_offset = 0;

  if (ru->openair0_cfg.mmapped_dma == 1) {
    // replace RX signal buffers with mmaped HW versions
    for (i=0; i<ru->nb_rx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %u, rx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
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
      printf("Mapping RU id %u, tx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.txdata[i]);
      ru->common.txdata[i] = ru->openair0_cfg.txbase[ru->rf_map.chain+ant];
      printf("txdata[%d] @ %p\n",i,ru->common.txdata[i]);

      for (j=0; j<16; j++) {
        printf("txbuffer %d: %x\n",j,ru->common.txdata[i][j]);
        ru->common.txdata[i][j] = 16-j;
      }
    }
  } else { // not memory-mapped DMA
    //nothing to do, everything already allocated in lte_init
  }

  return(0);
}

void *ru_stats_thread(void *param) {
  RU_t               *ru      = (RU_t *)param;
  wait_sync("ru_stats_thread");

  while (!oai_exit) {
    sleep(1);

    if (opp_enabled == 1) {
      if (ru->feprx) print_meas(&ru->ofdm_demod_stats,"feprx",NULL,NULL);

      if (ru->feptx_ofdm) {
        print_meas(&ru->precoding_stats,"feptx_prec",NULL,NULL);
        print_meas(&ru->txdataF_copy_stats,"txdataF_copy",NULL,NULL);
        print_meas(&ru->ofdm_mod_stats,"feptx_ofdm",NULL,NULL);
        print_meas(&ru->ofdm_total_stats,"feptx_total",NULL,NULL);
      }

      if (ru->fh_north_asynch_in) print_meas(&ru->rx_fhaul,"rx_fhaul",NULL,NULL);

      print_meas(&ru->tx_fhaul,"tx_fhaul",NULL,NULL);

      if (ru->fh_north_out) {
        print_meas(&ru->compression,"compression",NULL,NULL);
        print_meas(&ru->transport,"transport",NULL,NULL);
      }
    }
  }

  return(NULL);
}

void ru_tx_func(void *param) {
  processingData_RU_t *info = (processingData_RU_t *) param;
  RU_t *ru = info->ru;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  int frame_tx = info->frame_tx;
  int slot_tx = info->slot_tx;
  int print_frame = 8;
  char filename[40];

  // do TX front-end processing if needed (precoding and/or IDFTs)
  if (ru->feptx_prec) ru->feptx_prec(ru,frame_tx,slot_tx);

  // do OFDM with/without TX front-end processing  if needed
  if ((ru->fh_north_asynch_in == NULL) && (ru->feptx_ofdm)) ru->feptx_ofdm(ru,frame_tx,slot_tx);

  if(!emulate_rf) {
    // do outgoing fronthaul (south) if needed
    if ((ru->fh_north_asynch_in == NULL) && (ru->fh_south_out)) ru->fh_south_out(ru,frame_tx,slot_tx,info->timestamp_tx);

    if (ru->fh_north_out) ru->fh_north_out(ru);
  } else {
    if(frame_tx == print_frame) {
      for (int i=0; i<ru->nb_tx; i++) {
        if(slot_tx == 0) {
          sprintf(filename,"gNBdataF_frame%d_sl%d.m", print_frame, slot_tx);
          LOG_M(filename,"txdataF_frame",&ru->gNB_list[0]->common_vars.txdataF[i][0],fp->samples_per_frame_wCP, 1, 1);
          sprintf(filename,"tx%ddataF_frame%d_sl%d.m", i, print_frame, slot_tx);
          LOG_M(filename,"txdataF_frame",&ru->common.txdataF[i][0],fp->samples_per_frame_wCP, 1, 1);
          sprintf(filename,"tx%ddataF_BF_frame%d_sl%d.m", i, print_frame, slot_tx);
          LOG_M(filename,"txdataF_BF_frame",&ru->common.txdataF_BF[i][0],fp->samples_per_subframe_wCP, 1, 1);
        }

        if(slot_tx == 9) {
          sprintf(filename,"tx%ddata_frame%d.m", i, print_frame);
          LOG_M(filename,"txdata_frame",&ru->common.txdata[i][0],fp->samples_per_frame, 1, 1);
          sprintf(filename,"tx%ddata_frame%d.dat", i, print_frame);
          FILE *output_fd = fopen(filename,"w");

          if (output_fd) {
            fwrite(&ru->common.txdata[i][0],
                   sizeof(int32_t),
                   fp->samples_per_frame,
                   output_fd);
            fclose(output_fd);
          } else {
            LOG_E(PHY,"Cannot write to file %s\n",filename);
          }
        }//if(slot_tx == 9)
      }//for (i=0; i<ru->nb_tx; i++)
    }//if(frame_tx == print_frame)
  }//else  emulate_rf
}

void *ru_thread( void *param ) {
  static int ru_thread_status;
  RU_t               *ru      = (RU_t *)param;
  RU_proc_t          *proc    = &ru->proc;
  NR_DL_FRAME_PARMS  *fp      = ru->nr_frame_parms;
  PHY_VARS_gNB       *gNB     = RC.gNB[0];
  int                ret;
  int                slot     = fp->slots_per_frame-1;
  int                frame    = 1023;
  char               threadname[40];
  int                aa;
  nfapi_nr_config_request_scf_t *cfg = &ru->config;
  // set default return value
  ru_thread_status = 0;
  // set default return value
  sprintf(threadname,"ru_thread %u",ru->idx);
  LOG_I(PHY,"Starting RU %d (%s,%s),\n",ru->idx,NB_functions[ru->function],NB_timing[ru->if_timing]);
  memcpy((void *)&ru->config,(void *)&RC.gNB[0]->gNB_config,sizeof(ru->config));

  if(emulate_rf) {
    fill_rf_config(ru,ru->rf_config_file);
    nr_init_frame_parms(&ru->config, fp);
    nr_dump_frame_parms(fp);
    nr_phy_init_RU(ru);

    if (setup_RU_buffers(ru)!=0) {
      printf("Exiting, cannot initialize RU Buffers\n");
      exit(-1);
    }
  } else {
    nr_init_frame_parms(&ru->config, fp);
    nr_dump_frame_parms(fp);
    fill_rf_config(ru,ru->rf_config_file);
    nr_phy_init_RU(ru);

    // Start IF device if any
    if (ru->nr_start_if) {
      LOG_I(PHY,"Starting IF interface for RU %d\n",ru->idx);
      AssertFatal(ru->nr_start_if(ru,NULL) == 0, "Could not start the IF device\n");

      if (ru->has_ctrl_prt > 0) {
        if (ru->if_south == LOCAL_RF) ret = connect_rau(ru);
        else ret = attach_rru(ru);
  
        AssertFatal(ret==0,"Cannot connect to remote radio\n");
      }

    }
    else if (ru->if_south == LOCAL_RF) { // configure RF parameters only
      ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
      AssertFatal(ret==0,"Cannot connect to local radio\n");
    }

    if (setup_RU_buffers(ru)!=0) {
      printf("Exiting, cannot initialize RU Buffers\n");
      exit(-1);
    }
  }

  sf_ahead = (uint16_t) ceil((float)6/(0x01<<fp->numerology_index));
  LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
  pthread_mutex_lock(&RC.ru_mutex);
  RC.ru_mask &= ~(1<<ru->idx);
  pthread_cond_signal(&RC.ru_cond);
  pthread_mutex_unlock(&RC.ru_mutex);
  wait_sync("ru_thread");
  notifiedFIFO_elt_t *msg = newNotifiedFIFO_elt(sizeof(processingData_L1_t),0,gNB->resp_L1,rx_func);
  notifiedFIFO_elt_t *msgL1Tx = newNotifiedFIFO_elt(sizeof(processingData_L1_t),0,gNB->resp_L1_tx,tx_func);
  notifiedFIFO_elt_t *msgRUTx = newNotifiedFIFO_elt(sizeof(processingData_L1_t),0,gNB->resp_RU_tx,ru_tx_func);
  processingData_L1_t *syncMsg;
  notifiedFIFO_elt_t *res;
  pushNotifiedFIFO(gNB->resp_L1,msg); // to unblock the process in the beginning
  pushNotifiedFIFO(gNB->resp_L1_tx,msgL1Tx); // to unblock the process in the beginning
  pushNotifiedFIFO(gNB->resp_RU_tx,msgRUTx); // to unblock the process in the beginning

  if(!emulate_rf) {
    // Start RF device if any
    if (ru->start_rf) {
      if (ru->start_rf(ru) != 0)
        LOG_E(HW,"Could not start the RF device\n");
      else LOG_I(PHY,"RU %d rf device ready\n",ru->idx);
    } else LOG_I(PHY,"RU %d no rf device\n",ru->idx);

    // start trx write thread
    if(usrp_tx_thread == 1) {
      if (ru->start_write_thread) {
        if(ru->start_write_thread(ru) != 0) {
          LOG_E(HW,"Could not start tx write thread\n");
        } else {
          LOG_I(PHY,"tx write thread ready\n");
        }
      }
    }
  }

  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  while (!oai_exit) {
    // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
    // They are set on the first rx/tx in the underly FH routines.
    if (slot==(fp->slots_per_frame-1)) {
      slot=0;
      frame++;
      frame&=1023;
    } else {
      slot++;
    }

    // synchronization on input FH interface, acquire signals/data and block
    LOG_D(PHY,"[RU_thread] read data: frame_rx = %d, tti_rx = %d\n", frame, slot);

    if (ru->fh_south_in) ru->fh_south_in(ru,&frame,&slot);
    else AssertFatal(1==0, "No fronthaul interface at south port");

    proc->timestamp_tx = proc->timestamp_rx + (sf_ahead*fp->samples_per_subframe);
    proc->frame_tx     = (proc->tti_rx > (fp->slots_per_frame-1-(fp->slots_per_subframe*sf_ahead))) ? (proc->frame_rx+1)&1023 : proc->frame_rx;
    proc->tti_tx      = (proc->tti_rx + (fp->slots_per_subframe*sf_ahead))%fp->slots_per_frame;
    LOG_D(PHY,"AFTER fh_south_in - SFN/SL:%d%d RU->proc[RX:%d.%d TX:%d.%d] RC.gNB[0]:[RX:%d%d TX(SFN):%d]\n",
          frame,slot,
          proc->frame_rx,proc->tti_rx,
          proc->frame_tx,proc->tti_tx,
          RC.gNB[0]->proc.frame_rx,RC.gNB[0]->proc.slot_rx,
          RC.gNB[0]->proc.frame_tx);

    if (ru->idx!=0) proc->frame_tx = (proc->frame_tx+proc->frame_offset)&1023;

    // do RX front-end processing (frequency-shift, dft) if needed
    int slot_type = nr_slot_select(cfg,proc->frame_rx,proc->tti_rx);

    if (slot_type == NR_UPLINK_SLOT || slot_type == NR_MIXED_SLOT) {
      if (ru->feprx) {
        ru->feprx(ru,proc->tti_rx);
        //LOG_M("rxdata.m","rxs",ru->common.rxdata[0],1228800,1,1);
        LOG_D(PHY,"RU proc: frame_rx = %d, tti_rx = %d\n", proc->frame_rx, proc->tti_rx);
/*
        LOG_D(PHY,"Copying rxdataF from RU to gNB\n");

        for (aa=0; aa<ru->nb_rx; aa++)
          memcpy((void *)RC.gNB[0]->common_vars.rxdataF[aa],
                 (void *)ru->common.rxdataF[aa], fp->symbols_per_slot*fp->ofdm_symbol_size*sizeof(int32_t));
*/
        if (IS_SOFTMODEM_DOSCOPE && RC.gNB[0]->scopeData)
          ((scopeData_t *)RC.gNB[0]->scopeData)->slotFunc(ru->common.rxdataF[0],proc->tti_rx, RC.gNB[0]->scopeData);

        // Do PRACH RU processing
        int prach_id=find_nr_prach_ru(ru,proc->frame_rx,proc->tti_rx,SEARCH_EXIST);
        uint8_t prachStartSymbol,N_dur;

        if (prach_id>=0) {
          T(T_GNB_PHY_PRACH_INPUT_SIGNAL, T_INT(proc->frame_rx), T_INT(proc->tti_rx), T_INT(0),
            T_BUFFER(&ru->common.rxdata[0][fp->get_samples_slot_timestamp(proc->tti_rx-1,fp,0)]/*-ru->N_TA_offset*/, fp->get_samples_per_slot(proc->tti_rx,fp)*4*2));
          N_dur = get_nr_prach_duration(ru->prach_list[prach_id].fmt);

          /*
          get_nr_prach_info_from_index(ru->config.prach_config.prach_ConfigurationIndex.value,
                     proc->frame_rx,proc->tti_rx,
                     ru->config.carrier_config.dl_frequency.value,
                     fp->numerology_index,
                     fp->frame_type,
                     &format,
                     &start_symbol,
                     &N_t_slot,
                     &N_dur,
                     &RA_sfn_index,
                     &N_RA_slot,
                     &config_period);
          */
          for (int prach_oc = 0; prach_oc<ru->prach_list[prach_id].num_prach_ocas; prach_oc++) {
            prachStartSymbol = ru->prach_list[prach_id].prachStartSymbol+prach_oc*N_dur;
            //comment FK: the standard 38.211 section 5.3.2 has one extra term +14*N_RA_slot. This is because there prachStartSymbol is given wrt to start of the 15kHz slot or 60kHz slot. Here we work slot based, so this function is anyway only called in slots where there is PRACH. Its up to the MAC to schedule another PRACH PDU in the case there are there N_RA_slot \in {0,1}.
            rx_nr_prach_ru(ru,
                           ru->prach_list[prach_id].fmt, //could also use format
                           ru->prach_list[prach_id].numRA,
                           prachStartSymbol,
                           prach_oc,
                           proc->frame_rx,proc->tti_rx);
          }

          free_nr_ru_prach_entry(ru,prach_id);
        }
      }
    }

    // At this point, all information for subframe has been received on FH interface
    res = pullTpool(gNB->resp_L1, gNB->threadPool);
    syncMsg = (processingData_L1_t *)NotifiedFifoData(res);
    syncMsg->gNB = gNB;
    syncMsg->frame_rx = proc->frame_rx;
    syncMsg->slot_rx = proc->tti_rx;
    syncMsg->frame_tx = proc->frame_tx;
    syncMsg->slot_tx = proc->tti_tx;
    syncMsg->timestamp_tx = proc->timestamp_tx;
    res->key = proc->tti_rx;
    pushTpool(gNB->threadPool, res);
  }

  printf( "Exiting ru_thread \n");

  if (ru->stop_rf != NULL) {
    if (ru->stop_rf(ru) != 0)
      LOG_E(HW,"Could not stop the RF device\n");
    else LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
  }

  delNotifiedFIFO_elt(msg);
  delNotifiedFIFO_elt(msgL1Tx);
  delNotifiedFIFO_elt(msgRUTx);
  ru_thread_status = 0;
  return &ru_thread_status;
}

int start_streaming(RU_t *ru) {
  LOG_I(PHY,"Starting streaming on third-party RRU\n");
  return(ru->ifdevice.thirdparty_startstreaming(&ru->ifdevice));
}

int nr_start_if(struct RU_t_s *ru, struct PHY_VARS_gNB_s *gNB) {
  return(ru->ifdevice.trx_start_func(&ru->ifdevice));
}

int start_rf(RU_t *ru) {
  return(ru->rfdevice.trx_start_func(&ru->rfdevice));
}

int stop_rf(RU_t *ru) {
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  return 0;
}

int start_write_thread(RU_t *ru) {
  return(ru->rfdevice.trx_write_init(&ru->rfdevice));
}

void init_RU_proc(RU_t *ru) {
  int i=0;
  RU_proc_t *proc;
  LOG_I(PHY,"Initializing RU proc %d (%s,%s),\n",ru->idx,NB_functions[ru->function],NB_timing[ru->if_timing]);
  proc = &ru->proc;
  memset((void *)proc,0,sizeof(RU_proc_t));
  proc->ru = ru;
  proc->instance_cnt_emulateRF   = -1;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->num_slaves               = 0;
  proc->frame_tx_unwrap          = 0;
  proc->feptx_mask               = 0;

  for (i=0; i<10; i++) proc->symbol_mask[i]=0;

  pthread_mutex_init( &proc->mutex_emulateRF,NULL);
  pthread_cond_init( &proc->cond_emulateRF, NULL);
  threadCreate( &proc->pthread_FH, ru_thread, (void *)ru, "thread_FH", -1, OAI_PRIORITY_RT_MAX );

  if(emulate_rf)
    threadCreate( &proc->pthread_emulateRF, emulatedRF_thread, (void *)proc, "emulateRF", -1, OAI_PRIORITY_RT );

  if (get_thread_worker_conf() == WORKER_ENABLE) {
    if (ru->feprx) nr_init_feprx_thread(ru);

    if (ru->feptx_ofdm) nr_init_feptx_thread(ru);
  }

  if (opp_enabled == 1) threadCreate(&ru->ru_stats_thread,ru_stats_thread,(void *)ru, "emulateRF", -1, OAI_PRIORITY_RT_LOW);
}

void kill_NR_RU_proc(int inst) {
  RU_t *ru = RC.ru[inst];
  RU_proc_t *proc = &ru->proc;
  LOG_D(PHY, "Joining pthread_FH\n");
  pthread_join(proc->pthread_FH, NULL);

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
}

int check_capabilities(RU_t *ru,RRU_capabilities_t *cap) {
  FH_fmt_options_t fmt = cap->FH_fmt;
  int i;
  int found_band=0;
  LOG_I(PHY,"RRU %d, num_bands %d, looking for band %d\n",ru->idx,cap->num_bands,ru->nr_frame_parms->nr_band);

  for (i=0; i<cap->num_bands; i++) {
    LOG_I(PHY,"band %d on RRU %d\n",cap->band_list[i],ru->idx);

    if (ru->nr_frame_parms->nr_band == cap->band_list[i]) {
      found_band=1;
      break;
    }
  }

  if (found_band == 0) {
    LOG_I(PHY,"Couldn't find target NR band %d on RRU %d\n",ru->nr_frame_parms->nr_band,ru->idx);
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
  RRU_capabilities_t *capabilities = (RRU_capabilities_t *)arg;
  nfapi_nr_config_request_scf_t *cfg = &ru->config;
  int ret;
  LOG_I(PHY, "Received capabilities from RRU %d\n",idx);

  if (capabilities->FH_fmt < MAX_FH_FMTs) LOG_I(PHY, "RU FH options %s\n",rru_format_options[capabilities->FH_fmt]);

  ret = check_capabilities(ru,capabilities);
  AssertFatal(ret == 0, "Cannot configure RRU %d, check_capabilities returned %d\n", idx, ret);
  // take antenna capabilities of RRU
  ru->nb_tx                      = capabilities->nb_tx[0];
  ru->nb_rx                      = capabilities->nb_rx[0];
  // Pass configuration to RRU
  LOG_I(PHY, "Using %s fronthaul (%d), band %d \n",ru_if_formats[ru->if_south],ru->if_south,ru->nr_frame_parms->nr_band);
  // wait for configuration
  config->FH_fmt                 = ru->if_south;
  config->num_bands              = 1;
  config->band_list[0]           = ru->nr_frame_parms->nr_band;
  config->tx_freq[0]             = ru->nr_frame_parms->dl_CarrierFreq;
  config->rx_freq[0]             = ru->nr_frame_parms->ul_CarrierFreq;
  //config->tdd_config[0]          = ru->nr_frame_parms->tdd_config;
  //config->tdd_config_S[0]        = ru->nr_frame_parms->tdd_config_S;
  config->att_tx[0]              = ru->att_tx;
  config->att_rx[0]              = ru->att_rx;
  config->N_RB_DL[0]             = cfg->carrier_config.dl_grid_size[cfg->ssb_config.scs_common.value].value;
  config->N_RB_UL[0]             = cfg->carrier_config.dl_grid_size[cfg->ssb_config.scs_common.value].value;
  config->threequarter_fs[0]     = ru->nr_frame_parms->threequarter_fs;
  /*  if (ru->if_south==REMOTE_IF4p5) {
      config->prach_FreqOffset[0]  = ru->nr_frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset;
      config->prach_ConfigIndex[0] = ru->nr_frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
      LOG_I(PHY,"REMOTE_IF4p5: prach_FrequOffset %d, prach_ConfigIndex %d\n",
      config->prach_FreqOffset[0],config->prach_ConfigIndex[0]);*/
  nr_init_frame_parms(&ru->config, ru->nr_frame_parms);
  nr_phy_init_RU(ru);
}

void configure_rru(int idx,
                   void *arg) {
  RRU_config_t *config     = (RRU_config_t *)arg;
  RU_t         *ru         = RC.ru[idx];
  nfapi_nr_config_request_scf_t *cfg = &ru->config;
  ru->nr_frame_parms->nr_band                                             = config->band_list[0];
  ru->nr_frame_parms->dl_CarrierFreq                                      = config->tx_freq[0];
  ru->nr_frame_parms->ul_CarrierFreq                                      = config->rx_freq[0];

  if (ru->nr_frame_parms->dl_CarrierFreq == ru->nr_frame_parms->ul_CarrierFreq) {
    cfg->cell_config.frame_duplex_type.value                       = TDD;
    //ru->nr_frame_parms->tdd_config                                        = config->tdd_config[0];
    //ru->nr_frame_parms->tdd_config_S                                      = config->tdd_config_S[0];
  } else
    cfg->cell_config.frame_duplex_type.value                       = FDD;

  ru->att_tx                                                              = config->att_tx[0];
  ru->att_rx                                                              = config->att_rx[0];
  int mu = cfg->ssb_config.scs_common.value;
  cfg->carrier_config.dl_grid_size[mu].value                       = config->N_RB_DL[0];
  cfg->carrier_config.dl_grid_size[mu].value                       = config->N_RB_UL[0];
  ru->nr_frame_parms->threequarter_fs                                     = config->threequarter_fs[0];

  //ru->nr_frame_parms->pdsch_config_common.referenceSignalPower                 = ru->max_pdschReferenceSignalPower-config->att_tx[0];
  if (ru->function==NGFI_RRU_IF4p5) {
    ru->nr_frame_parms->att_rx = ru->att_rx;
    ru->nr_frame_parms->att_tx = ru->att_tx;
    /*
        LOG_I(PHY,"Setting ru->function to NGFI_RRU_IF4p5, prach_FrequOffset %d, prach_ConfigIndex %d, att (%d,%d)\n",
        config->prach_FreqOffset[0],config->prach_ConfigIndex[0],ru->att_tx,ru->att_rx);
        ru->nr_frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset  = config->prach_FreqOffset[0];
        ru->nr_frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex = config->prach_ConfigIndex[0]; */
  }

  fill_rf_config(ru,ru->rf_config_file);
  nr_init_frame_parms(&ru->config, ru->nr_frame_parms);
  nr_phy_init_RU(ru);
}

/*
void init_precoding_weights(PHY_VARS_gNB *gNB) {

  int layer,ru_id,aa,re,ue,tb;
  LTE_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  RU_t *ru;
  LTE_gNB_DLSCH_t *dlsch;

  // init precoding weigths
  for (ue=0;ue<NUMBER_OF_UE_MAX;ue++) {
    for (tb=0;tb<2;tb++) {
      dlsch = gNB->dlsch[ue][tb];
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
}*/

void set_function_spec_param(RU_t *ru) {
  int ret;

  switch (ru->if_south) {
    case LOCAL_RF:   // this is an RU with integrated RF (RRU, gNB)
      if (ru->function ==  NGFI_RRU_IF5) {                 // IF5 RRU
        ru->do_prach              = 0;                      // no prach processing in RU
        ru->fh_north_in           = NULL;                   // no shynchronous incoming fronthaul from north
        ru->fh_north_out          = fh_if5_north_out;       // need only to do send_IF5  reception
        ru->fh_south_out          = tx_rf;                  // send output to RF
        ru->fh_north_asynch_in    = fh_if5_north_asynch_in; // TX packets come asynchronously
        ru->feprx                 = NULL;                   // nothing (this is a time-domain signal)
        ru->feptx_ofdm            = NULL;                   // nothing (this is a time-domain signal)
        ru->feptx_prec            = NULL;                   // nothing (this is a time-domain signal)
        ru->nr_start_if           = nr_start_if;            // need to start the if interface for if5
        ru->ifdevice.host_type    = RRU_HOST;
        ru->rfdevice.host_type    = RRU_HOST;
        ru->ifdevice.eth_params   = &ru->eth_params;
        reset_meas(&ru->rx_fhaul);
        reset_meas(&ru->tx_fhaul);
        reset_meas(&ru->compression);
        reset_meas(&ru->transport);
        ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
        printf("openair0_transport_init returns %d for ru_id %u\n", ret, ru->idx);

        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }
      } else if (ru->function == NGFI_RRU_IF4p5) {
        ru->do_prach              = 1;                        // do part of prach processing in RU
        ru->fh_north_in           = NULL;                     // no synchronous incoming fronthaul from north
        ru->fh_north_out          = fh_if4p5_north_out;       // send_IF4p5 on reception
        ru->fh_south_out          = tx_rf;                    // send output to RF
        ru->fh_north_asynch_in    = fh_if4p5_north_asynch_in; // TX packets come asynchronously
        ru->feprx                 = (get_thread_worker_conf() == WORKER_ENABLE) ? nr_fep_full_2thread : nr_fep_full;     // RX DFTs
        ru->feptx_ofdm            = (get_thread_worker_conf() == WORKER_ENABLE) ? nr_feptx_ofdm_2thread : nr_feptx_ofdm; // this is fep with idft only (no precoding in RRU)
        ru->feptx_prec            = NULL;
        ru->nr_start_if           = nr_start_if;              // need to start the if interface for if4p5
        ru->ifdevice.host_type    = RRU_HOST;
        ru->rfdevice.host_type    = RRU_HOST;
        ru->ifdevice.eth_params   = &ru->eth_params;
        reset_meas(&ru->rx_fhaul);
        reset_meas(&ru->tx_fhaul);
        reset_meas(&ru->compression);
        reset_meas(&ru->transport);
        ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
        printf("openair0_transport_init returns %d for ru_id %u\n", ret, ru->idx);

        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }

        malloc_IF4p5_buffer(ru);
      } else if (ru->function == gNodeB_3GPP) {
        ru->do_prach             = 0;                       // no prach processing in RU
        ru->feprx                = (get_thread_worker_conf() == WORKER_ENABLE) ? nr_fep_full_2thread   : nr_fep_full;                // RX DFTs
        ru->feptx_ofdm           = (get_thread_worker_conf() == WORKER_ENABLE) ? nr_feptx_ofdm_2thread : nr_feptx_ofdm;              // this is fep with idft and precoding
        ru->feptx_prec           = (get_thread_worker_conf() == WORKER_ENABLE) ? NULL                  : nr_feptx_prec;           // this is fep with idft and precoding
        ru->fh_north_in          = NULL;                    // no incoming fronthaul from north
        ru->fh_north_out         = NULL;                    // no outgoing fronthaul to north
        ru->nr_start_if          = NULL;                    // no if interface
        ru->rfdevice.host_type   = RAU_HOST;
        ru->fh_south_in            = rx_rf;                 // local synchronous RF RX
        ru->fh_south_out           = tx_rf;                 // local synchronous RF TX
        ru->start_rf               = start_rf;              // need to start the local RF interface
        ru->stop_rf                = stop_rf;
        ru->start_write_thread     = start_write_thread;                  // starting RF TX in different thread
        printf("configuring ru_id %u (start_rf %p)\n", ru->idx, start_rf);
      }

      /*
      printf("configuring ru_id %u (start_rf %p)\n", ru->idx, start_rf);
      fill_rf_config(ru,rf_config_file);
      init_frame_parms(&ru->frame_parms,1);
      nr_phy_init_RU(ru);

      ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
      if (setup_RU_buffers(ru)!=0) {
      printf("Exiting, cannot initialize RU Buffers\n");
      exit(-1);
      }
      */
      break;

    case REMOTE_IF5: // the remote unit is IF5 RRU
      ru->do_prach               = 0;
      ru->feprx                  = (get_thread_worker_conf() == WORKER_ENABLE) ? nr_fep_full_2thread   : nr_fep_full;     // this is frequency-shift + DFTs
      ru->feptx_prec             = (get_thread_worker_conf() == WORKER_ENABLE) ? NULL                  : nr_feptx_prec;          // need to do transmit Precoding + IDFTs
      ru->feptx_ofdm             = (get_thread_worker_conf() == WORKER_ENABLE) ? nr_feptx_ofdm_2thread : nr_feptx_ofdm; // need to do transmit Precoding + IDFTs
      ru->fh_south_in            = fh_if5_south_in;     // synchronous IF5 reception
      ru->fh_south_out           = fh_if5_south_out;    // synchronous IF5 transmission
      ru->fh_south_asynch_in     = NULL;                // no asynchronous UL
      ru->start_rf               = ru->eth_params.transp_preference == ETH_UDP_IF5_ECPRI_MODE ? start_streaming : NULL;
      ru->stop_rf                = NULL;
      ru->start_write_thread     = NULL;
      ru->nr_start_if            = nr_start_if;         // need to start if interface for IF5
      ru->ifdevice.host_type     = RAU_HOST;
      ru->ifdevice.eth_params    = &ru->eth_params;
      ru->ifdevice.configure_rru = configure_ru;
      ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
      printf("openair0_transport_init returns %d for ru_id %u\n", ret, ru->idx);

      if (ret<0) {
        printf("Exiting, cannot initialize transport protocol\n");
        exit(-1);
      }

      break;

    case REMOTE_IF4p5:
      ru->do_prach               = 0;
      ru->feprx                  = NULL;                // DFTs
      ru->feptx_prec             = nr_feptx_prec;          // Precoding operation
      ru->feptx_ofdm             = NULL;                // no OFDM mod
      ru->fh_south_in            = fh_if4p5_south_in;   // synchronous IF4p5 reception
      ru->fh_south_out           = fh_if4p5_south_out;  // synchronous IF4p5 transmission
      ru->fh_south_asynch_in     = (ru->if_timing == synch_to_other) ? fh_if4p5_south_in : NULL;                // asynchronous UL if synch_to_other
      ru->fh_north_out           = NULL;
      ru->fh_north_asynch_in     = NULL;
      ru->start_rf               = NULL;                // no local RF
      ru->stop_rf                = NULL;
      ru->start_write_thread     = NULL;
      ru->nr_start_if            = nr_start_if;         // need to start if interface for IF4p5
      ru->ifdevice.host_type     = RAU_HOST;
      ru->ifdevice.eth_params    = &ru->eth_params;
      ru->ifdevice.configure_rru = configure_ru;
      ret = openair0_transport_load(&ru->ifdevice, &ru->openair0_cfg, &ru->eth_params);
      printf("openair0_transport_init returns %d for ru_id %u\n", ret, ru->idx);

      if (ret<0) {
        printf("Exiting, cannot initialize transport protocol\n");
        exit(-1);
      }

      if (ru->ifdevice.get_internal_parameter != NULL) {
        void *t = ru->ifdevice.get_internal_parameter("fh_if4p5_south_in");

        if (t != NULL)
          ru->fh_south_in = t;

        t = ru->ifdevice.get_internal_parameter("fh_if4p5_south_out");

        if (t != NULL)
          ru->fh_south_out = t;
      }

      malloc_IF4p5_buffer(ru);
      break;

    default:
      LOG_E(PHY,"RU with invalid or unknown southbound interface type %d\n",ru->if_south);
      break;
  } // switch on interface type
}

void init_NR_RU(char *rf_config_file) {
  int ru_id;
  RU_t *ru;
  PHY_VARS_gNB *gNB_RC;
  PHY_VARS_gNB *gNB0= (PHY_VARS_gNB *)NULL;
  NR_DL_FRAME_PARMS *fp = (NR_DL_FRAME_PARMS *)NULL;
  int i;
  // create status mask
  RC.ru_mask = 0;
  pthread_mutex_init(&RC.ru_mutex,NULL);
  pthread_cond_init(&RC.ru_cond,NULL);
  // read in configuration file)
  printf("configuring RU from file\n");
  NRRCconfig_RU();
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",RC.nb_nr_L1_inst,RC.nb_RU,get_nprocs());
  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);

  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    LOG_D(PHY,"Process RC.ru[%d]\n",ru_id);
    ru                 = RC.ru[ru_id];
    ru->rf_config_file = rf_config_file;
    ru->idx            = ru_id;
    ru->ts_offset      = 0;
    // use gNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!

    if (ru->num_gNB > 0) {
      LOG_D(PHY, "%s() RC.ru[%d].num_gNB:%d ru->gNB_list[0]:%p RC.gNB[0]:%p rf_config_file:%s\n", __FUNCTION__, ru_id, ru->num_gNB, ru->gNB_list[0], RC.gNB[0], ru->rf_config_file);

      if (ru->gNB_list[0] == 0) {
        LOG_E(PHY,"%s() DJP - ru->gNB_list ru->num_gNB are not initialized - so do it manually\n", __FUNCTION__);
        ru->gNB_list[0] = RC.gNB[0];
        ru->num_gNB=1;
        //
        // DJP - feptx_prec() / feptx_ofdm() parses the gNB_list (based on num_gNB) and copies the txdata_F to txdata in RU
        //
      } else {
        LOG_E(PHY,"DJP - delete code above this %s:%d\n", __FILE__, __LINE__);
      }
    }

    gNB_RC           = RC.gNB[0];
    gNB0             = ru->gNB_list[0];
    fp               = ru->nr_frame_parms;
    LOG_D(PHY, "RU FUnction:%d ru->if_south:%d\n", ru->function, ru->if_south);

    if (gNB0) {
      if ((ru->function != NGFI_RRU_IF5) && (ru->function != NGFI_RRU_IF4p5))
        AssertFatal(gNB0!=NULL,"gNB0 is null!\n");

      if (gNB0 && gNB_RC) {
        LOG_I(PHY,"Copying frame parms from gNB in RC to gNB %d in ru %d and frame_parms in ru\n",gNB0->Mod_id,ru->idx);
        memset((void *)fp, 0, sizeof(NR_DL_FRAME_PARMS));
        memcpy((void *)fp,&gNB_RC->frame_parms,sizeof(NR_DL_FRAME_PARMS));
        memcpy((void *)&gNB0->frame_parms,(void *)&gNB_RC->frame_parms,sizeof(NR_DL_FRAME_PARMS));
        // attach all RU to all gNBs in its list/
        LOG_D(PHY,"ru->num_gNB:%d gNB0->num_RU:%d\n", ru->num_gNB, gNB0->num_RU);

        for (i=0; i<ru->num_gNB; i++) {
          gNB0 = ru->gNB_list[i];
          gNB0->RU_list[gNB0->num_RU++] = ru;
        }
      }
    }

    //LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s,%d)\n",ru_id,ru_if_types[ru->if_south],NB_timing[ru->if_timing],ru->function);
    set_function_spec_param(ru);
    LOG_I(PHY,"Starting ru_thread %d\n",ru_id);
    init_RU_proc(ru);
  } // for ru_id

  //  sleep(1);
  LOG_D(HW,"[nr-softmodem.c] RU threads created\n");
}


void stop_RU(int nb_ru) {
  for (int inst = 0; inst < nb_ru; inst++) {
    LOG_I(PHY, "Stopping RU %d processing threads\n", inst);
    kill_NR_RU_proc(inst);
  }
}


/* --------------------------------------------------------*/
/* from here function to use configuration module          */
static void NRRCconfig_RU(void) {
  int i = 0, j = 0;
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist( &RUParamList, RUParams, sizeof(RUParams)/sizeof(paramdef_t), NULL);

  if ( RUParamList.numelt > 0) {
    RC.ru = (RU_t **)malloc(RC.nb_RU*sizeof(RU_t *));
    RC.ru_mask=(1<<NB_RU) - 1;
    printf("Set RU mask to %lx\n",RC.ru_mask);

    for (j = 0; j < RC.nb_RU; j++) {
      RC.ru[j]                                      = (RU_t *)malloc(sizeof(RU_t));
      memset((void *)RC.ru[j],0,sizeof(RU_t));
      RC.ru[j]->idx                                 = j;
      RC.ru[j]->nr_frame_parms                      = (NR_DL_FRAME_PARMS *)malloc(sizeof(NR_DL_FRAME_PARMS));
      RC.ru[j]->frame_parms                         = (LTE_DL_FRAME_PARMS *)malloc(sizeof(LTE_DL_FRAME_PARMS));
      printf("Creating RC.ru[%d]:%p\n", j, RC.ru[j]);
      RC.ru[j]->if_timing                           = synch_to_ext_device;

      if (RC.nb_nr_L1_inst >0)
        RC.ru[j]->num_gNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
      else
        RC.ru[j]->num_gNB                           = 0;

      for (i=0; i<RC.ru[j]->num_gNB; i++) RC.ru[j]->gNB_list[i] = RC.gNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]];

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_ADDRS)) {
        RC.ru[j]->openair0_cfg.sdr_addrs = strdup(*(RUParamList.paramarray[j][RU_SDR_ADDRS].strptr));
      }

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_CLK_SRC)) {
        if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "internal") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = internal;
          LOG_I(PHY, "RU clock source set as internal\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "external") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = external;
          LOG_I(PHY, "RU clock source set as external\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "gpsdo") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = gpsdo;
          LOG_I(PHY, "RU clock source set as gpsdo\n");
        } else {
          LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
        }
      } else {
        LOG_I(PHY,"Setting clock source to internal\n");
        RC.ru[j]->openair0_cfg.clock_source = internal;
      }

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_TME_SRC)) {
        if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "internal") == 0) {
          RC.ru[j]->openair0_cfg.time_source = internal;
          LOG_I(PHY, "RU time source set as internal\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "external") == 0) {
          RC.ru[j]->openair0_cfg.time_source = external;
          LOG_I(PHY, "RU time source set as external\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "gpsdo") == 0) {
          RC.ru[j]->openair0_cfg.time_source = gpsdo;
          LOG_I(PHY, "RU time source set as gpsdo\n");
        } else {
          LOG_E(PHY, "Erroneous RU time source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
        }
      } else {
        LOG_I(PHY,"Setting time source to internal\n");
        RC.ru[j]->openair0_cfg.time_source = internal;
      }

      if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
        if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
          RC.ru[j]->if_south                        = LOCAL_RF;
          RC.ru[j]->function                        = gNodeB_3GPP;
          printf("Setting function for RU %d to gNodeB_3GPP\n",j);
        } else {
          RC.ru[j]->eth_params.local_if_name           = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
          RC.ru[j]->eth_params.my_addr                 = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
          RC.ru[j]->eth_params.remote_addr             = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
          RC.ru[j]->eth_params.my_portc                = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.remote_portc            = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.my_portd                = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
          RC.ru[j]->eth_params.remote_portd            = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

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
        }

        RC.ru[j]->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
        RC.ru[j]->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
        RC.ru[j]->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;

        for (i=0; i<RC.ru[j]->num_bands; i++) RC.ru[j]->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i];
      } //strcmp(local_rf, "yes") == 0
      else {
        printf("RU %d: Transport %s\n",j,*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr));
        RC.ru[j]->eth_params.local_if_name = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
        RC.ru[j]->eth_params.my_addr       = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.remote_addr   = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.my_portc      = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.remote_portc  = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.my_portd      = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
        RC.ru[j]->eth_params.remote_portd  = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

        if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
          RC.ru[j]->if_south                     = REMOTE_IF5;
          RC.ru[j]->function                     = NGFI_RAU_IF5;
          RC.ru[j]->eth_params.transp_preference = ETH_UDP_MODE;
        } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_ecpri_if5") == 0) {
          RC.ru[j]->if_south                     = REMOTE_IF5;
          RC.ru[j]->function                     = NGFI_RAU_IF5;
          RC.ru[j]->eth_params.transp_preference = ETH_UDP_IF5_ECPRI_MODE;
        } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_ecpri_if5") == 0) {
          RC.ru[j]->if_south                     = REMOTE_IF5;
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
        }
      }  /* strcmp(local_rf, "yes") != 0 */

      RC.ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      RC.ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      RC.ru[j]->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
      RC.ru[j]->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);
      RC.ru[j]->if_frequency                      = *(RUParamList.paramarray[j][RU_IF_FREQUENCY].u64ptr);
      RC.ru[j]->if_freq_offset                    = *(RUParamList.paramarray[j][RU_IF_FREQ_OFFSET].iptr);
      RC.ru[j]->do_precoding                      = *(RUParamList.paramarray[j][RU_DO_PRECODING].iptr);

      if (config_isparamset(RUParamList.paramarray[j], RU_BF_WEIGHTS_LIST_IDX)) {
        RC.ru[j]->nb_bfw = RUParamList.paramarray[j][RU_BF_WEIGHTS_LIST_IDX].numelt;

        for (i=0; i<RC.ru[j]->num_gNB; i++)  {
          RC.ru[j]->bw_list[i] = (int32_t *)malloc16_clear((RC.ru[j]->nb_bfw)*sizeof(int32_t));

          for (int b=0; b<RC.ru[j]->nb_bfw; b++) RC.ru[j]->bw_list[i][b] = RUParamList.paramarray[j][RU_BF_WEIGHTS_LIST_IDX].iptr[b];
        }
      }
    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;
  } // setting != NULL

  return;
}

