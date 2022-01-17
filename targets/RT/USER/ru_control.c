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
/*! \file ru_control.c
 * \brief Top-level threads for RU entity
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2018
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

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include "msc.h"

#include "PHY/types.h"

#include "PHY/defs_common.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all


#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "SCHED/sched_eNB.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/INIT/phy_init.h"

#include "common/utils/LOG/log.h"


int attach_rru(RU_t *ru);
void configure_ru(RU_t *ru, void *arg);
void configure_rru(RU_t *ru, void *arg);
void fill_rf_config(RU_t *ru, char *rf_config_file);
void* ru_thread_control( void* param );

extern int setup_RU_buffers(RU_t *ru);
extern void reset_proc(RU_t *ru);
extern void  phy_init_RU(RU_t*);

extern const char ru_states[6][9];
const char rru_format_options[4][20] = {"OAI_IF5_only","OAI_IF4p5_only","OAI_IF5_and_IF4p5","MBP_IF5"};
const char rru_formats[3][20] = {"OAI_IF5","MBP_IF5","OAI_IF4p5"};
const char ru_if_formats[4][20] = {"LOCAL_RF","REMOTE_OAI_IF5","REMOTE_MBP_IF5","REMOTE_OAI_IF4p5"};

extern int oai_exit;
extern void wait_eNBs(void);

int send_tick(RU_t *ru)
{
  RRU_CONFIG_msg_t rru_config_msg;

  rru_config_msg.type = RAU_tick; 
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE;

  LOG_I(PHY,"Sending RAU tick to RRU %d\n",ru->idx);
  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
	      "RU %d cannot access remote radio\n",ru->idx);

  return 0;
}


int send_config(RU_t *ru,
                RRU_CONFIG_msg_t rru_config_msg)
{
  rru_config_msg.type = RRU_config;
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_config_t);

  LOG_I(PHY,"Sending Configuration to RRU %d (num_bands %d,band0 %d,txfreq %u,rxfreq %u,att_tx %d,att_rx %d,N_RB_DL %d,N_RB_UL %d,3/4FS %d, prach_FO %d, prach_CI %d\n",
    ru->idx,
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


int send_capab(RU_t *ru)
{
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
    AssertFatal(1==0,"RU_function is unknown %d\n",ru->function);
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


int attach_rru(RU_t *ru)
{
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
  configure_ru(ru,
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


int connect_rau(RU_t *ru)
{
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
    AssertFatal(1==0,"RU_function is unknown %d\n",ru->function);
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
      
      configure_rru(ru,
		    (void*)&rru_config_msg.msg[0]);
      configuration_received = 1;
    }
  }
  return 0;
}


int check_capabilities(RU_t *ru,
                       RRU_capabilities_t *cap)
{
  FH_fmt_options_t fmt = cap->FH_fmt;

  int i;
  int found_band=0;

  LOG_I(PHY,"RRU %d, num_bands %d, looking for band %d\n",ru->idx,cap->num_bands,ru->frame_parms->eutra_band);
  for (i=0;i<cap->num_bands;i++) {
    LOG_I(PHY,"band %d on RRU %d\n",cap->band_list[i],ru->idx);
    if (ru->frame_parms->eutra_band == cap->band_list[i]) {
      found_band=1;
      break;
    }
  }

  if (found_band == 0) {
    LOG_I(PHY,"Couldn't find target EUTRA band %d on RRU %d\n",ru->frame_parms->eutra_band,ru->idx);
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

void configure_ru(RU_t *ru,
                  void *arg)
{
  RRU_config_t       *config       = (RRU_config_t *)arg;
  RRU_capabilities_t *capabilities = (RRU_capabilities_t*)arg;
  int ret;

  LOG_I(PHY, "Received capabilities from RRU %d\n",ru->idx);


  if (capabilities->FH_fmt < MAX_FH_FMTs) LOG_I(PHY, "RU FH options %s\n",rru_format_options[capabilities->FH_fmt]);

  AssertFatal((ret=check_capabilities(ru,capabilities)) == 0,
	      "Cannot configure RRU %d, check_capabilities returned %d\n", ru->idx,ret);
  // take antenna capabilities of RRU
  ru->nb_tx                      = capabilities->nb_tx[0];
  ru->nb_rx                      = capabilities->nb_rx[0];

  // Pass configuration to RRU
  LOG_I(PHY, "Using %s fronthaul (%d), band %d \n",ru_if_formats[ru->if_south],ru->if_south,ru->frame_parms->eutra_band);
  // wait for configuration 
  config->FH_fmt                 = ru->if_south;
  config->num_bands              = 1;
  config->band_list[0]           = ru->frame_parms->eutra_band;
  config->tx_freq[0]             = ru->frame_parms->dl_CarrierFreq;
  config->rx_freq[0]             = ru->frame_parms->ul_CarrierFreq;
  config->tdd_config[0]          = ru->frame_parms->tdd_config;
  config->tdd_config_S[0]        = ru->frame_parms->tdd_config_S;
  config->att_tx[0]              = ru->att_tx;
  config->att_rx[0]              = ru->att_rx;
  config->N_RB_DL[0]             = ru->frame_parms->N_RB_DL;
  config->N_RB_UL[0]             = ru->frame_parms->N_RB_UL;
  config->threequarter_fs[0]     = ru->frame_parms->threequarter_fs;
  if (ru->if_south==REMOTE_IF4p5) {
    config->prach_FreqOffset[0]  = ru->frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset;
    config->prach_ConfigIndex[0] = ru->frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
    LOG_I(PHY,"REMOTE_IF4p5: prach_FrequOffset %d, prach_ConfigIndex %d\n",
	  config->prach_FreqOffset[0],config->prach_ConfigIndex[0]);
    
    int i;
    for (i=0;i<4;i++) {
      config->emtc_prach_CElevel_enable[0][i]  = ru->frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i];
      config->emtc_prach_FreqOffset[0][i]      = ru->frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[i];
      config->emtc_prach_ConfigIndex[0][i]     = ru->frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i];
    }
  }

  init_frame_parms(ru->frame_parms,1);
  phy_init_RU(ru);
}

void configure_rru(RU_t *ru,
                   void *arg)
{
  RRU_config_t *config = (RRU_config_t *)arg;

  ru->frame_parms->eutra_band                                               = config->band_list[0];
  ru->frame_parms->dl_CarrierFreq                                           = config->tx_freq[0];
  ru->frame_parms->ul_CarrierFreq                                           = config->rx_freq[0];
  if (ru->frame_parms->dl_CarrierFreq == ru->frame_parms->ul_CarrierFreq) {
	LOG_I(PHY,"Setting RRU to TDD frame type\n");
    ru->frame_parms->frame_type                                            = TDD;
    ru->frame_parms->tdd_config                                            = config->tdd_config[0];
    ru->frame_parms->tdd_config_S                                          = config->tdd_config_S[0];
  }
  else ru->frame_parms->frame_type                                         = FDD;

  ru->att_tx                                                               = config->att_tx[0];
  ru->att_rx                                                               = config->att_rx[0];
  ru->frame_parms->N_RB_DL                                                  = config->N_RB_DL[0];
  ru->frame_parms->N_RB_UL                                                  = config->N_RB_UL[0];
  ru->frame_parms->threequarter_fs                                          = config->threequarter_fs[0];
  ru->frame_parms->pdsch_config_common.referenceSignalPower                 = ru->max_pdschReferenceSignalPower-config->att_tx[0];

  if (ru->function==NGFI_RRU_IF4p5) {
    ru->frame_parms->att_rx = ru->att_rx;
    ru->frame_parms->att_tx = ru->att_tx;
    LOG_I(PHY,"Setting ru->function to NGFI_RRU_IF4p5, prach_FrequOffset %d, prach_ConfigIndex %d, att (%d,%d)\n",
	  config->prach_FreqOffset[0],config->prach_ConfigIndex[0],ru->att_tx,ru->att_rx);
    ru->frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset  = config->prach_FreqOffset[0];
    ru->frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex = config->prach_ConfigIndex[0];
    for (int i=0;i<4;i++) {
      ru->frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i] = config->emtc_prach_CElevel_enable[0][i];
      ru->frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[i]     = config->emtc_prach_FreqOffset[0][i];
      ru->frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i]    = config->emtc_prach_ConfigIndex[0][i];
    }
  }
  
  init_frame_parms(ru->frame_parms,1);
  fill_rf_config(ru,ru->rf_config_file);
  phy_init_RU(ru);
}

static int send_update_rru(RU_t * ru, LTE_DL_FRAME_PARMS * fp){
  //ssize_t      msg_len/*,len*/;
  int i;
  RRU_CONFIG_msg_t rru_config_msg;
  memset((void *)&rru_config_msg,0,sizeof(rru_config_msg));
  rru_config_msg.type = RRU_config_update;
  rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_config_t);
  LOG_I(PHY,"Sending RAU tick to RRU %d %lu bytes\n",ru->idx,rru_config_msg.len);

  RRU_config_t       *config       = (RRU_config_t *)&rru_config_msg.msg[0];
  config->num_MBSFN_config=fp->num_MBSFN_config;
  for(i=0; i < fp->num_MBSFN_config; i++){
       config->MBSFN_config[i] = fp->MBSFN_config[i];
       LOG_W(PHY,"Configuration send to RAU (num MBSFN %d, MBSFN_SubframeConfig[%d] pattern is %x )\n",config->num_MBSFN_config,i,config->MBSFN_config[i].mbsfn_SubframeConfig);

  }
  AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),
                "RU %d cannot access remote radio\n",ru->idx);
  return 0;
}

void* ru_thread_control( void* param )
{
  RU_t               *ru      = (RU_t*)param;
  RU_proc_t          *proc    = &ru->proc;
  RRU_CONFIG_msg_t   rru_config_msg;
  ssize_t            msg_len;
  int                len;
  int 		     ru_sf_update=0; // SF config update flag (MBSFN)

  // Start IF device if any
  if (ru->start_if) {
    LOG_I(PHY,"Starting IF interface for RU %d\n",ru->idx);
    AssertFatal(
		ru->start_if(ru,NULL) 	== 0, "Could not start the IF device\n");

    if (ru->if_south != LOCAL_RF) wait_eNBs();
  }

  
  ru->state = (ru->function==eNodeB_3GPP || ru->if_south == REMOTE_IF5)? RU_RUN : RU_IDLE;
  LOG_I(PHY,"Control channel ON for RU %d\n", ru->idx);

  while (!oai_exit) // Change the cond
    {
      msg_len  = sizeof(RRU_CONFIG_msg_t); // TODO : check what should be the msg len

      if (ru->state == RU_IDLE && ru->if_south != LOCAL_RF)
	send_tick(ru);

      if (ru->state == RU_RUN && ru->if_south != LOCAL_RF){
	LTE_DL_FRAME_PARMS *fp = &ru->eNB_list[0]->frame_parms;	
	LOG_D(PHY,"Check MBSFN SF changes\n");
	if(fp->num_MBSFN_config != ru_sf_update){
		ru_sf_update = fp->num_MBSFN_config;
		LOG_W(PHY,"RU SF should be updated ... calling send_update_rru(ru)\n");
		send_update_rru(ru,fp);
	}
      }

	
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

		configure_ru(ru,(RRU_capabilities_t *)&rru_config_msg.msg[0]);

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
	      
		ru->frame_parms = calloc(1, sizeof(*ru->frame_parms));
		configure_rru(ru, (void*)&rru_config_msg.msg[0]);

 					  
		fill_rf_config(ru,ru->rf_config_file);
		init_frame_parms(ru->frame_parms,1);
		ru->frame_parms->nb_antennas_rx = ru->nb_rx;
		phy_init_RU(ru);
					 
		//if (ru->is_slave == 1) lte_sync_time_init(&ru->frame_parms);

		if (ru->rfdevice.is_init != 1) openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
		
		if (ru->rfdevice.trx_config_func) AssertFatal((ru->rfdevice.trx_config_func(&ru->rfdevice,&ru->openair0_cfg)==0), 
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
		pthread_mutex_lock(ru->ru_mutex);
		*ru->ru_mask &= ~(1<<ru->idx);
		pthread_cond_signal(ru->ru_cond);
		pthread_mutex_unlock(ru->ru_mutex);
					  
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
	    case RRU_config_update: //RRU
	      if (ru->if_south == LOCAL_RF){
		LOG_W(PHY,"Configuration update received from RAU \n");

	    	msg_len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_config_t);

		LOG_W(PHY,"New MBSFN config received from RAU --- num_MBSFN_config %d\n",((RRU_config_t *)&rru_config_msg.msg[0])->num_MBSFN_config);
	        ru->frame_parms->num_MBSFN_config = ((RRU_config_t *)&rru_config_msg.msg[0])->num_MBSFN_config;
	        for(int i=0; i < ((RRU_config_t *)&rru_config_msg.msg[0])->num_MBSFN_config; i++){
		 ru->frame_parms->MBSFN_config[i].mbsfn_SubframeConfig=((RRU_config_t *)&rru_config_msg.msg[0])->MBSFN_config[i].mbsfn_SubframeConfig;
		  LOG_W(PHY,"Configuration received from RAU (num MBSFN %d, MBSFN_SubframeConfig[%d] pattern is %x )\n",
		       ((RRU_config_t *)&rru_config_msg.msg[0])->num_MBSFN_config,
		       i,
		       ((RRU_config_t *)&rru_config_msg.msg[0])->MBSFN_config[i].mbsfn_SubframeConfig
		       );
		}
	      } else LOG_E(PHY,"Received RRU_config msg...Ignoring\n");
		break;
	    case RRU_config_update_ok: //RAU
	      if (ru->if_south == LOCAL_RF) LOG_E(PHY,"Received RRU_config_update_ok msg...Ignoring\n");
	      else{
			LOG_W(PHY,"Received RRU_config_update_ok msg...\n");
	      }		
	      break;

	    case RRU_start: // RRU
	      if (ru->if_south == LOCAL_RF){
		LOG_I(PHY,"Start received from RAU\n");
				
		if (ru->state == RU_READY){

		  LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
		  pthread_mutex_lock(ru->ru_mutex);
		  *ru->ru_mask &= ~(1<<ru->idx);
		  pthread_cond_signal(ru->ru_cond);
		  pthread_mutex_unlock(ru->ru_mutex);
						  
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
	      if (ru->if_south == LOCAL_RF) LOG_E(PHY,"Received RRU_sync_ok msg...Ignoring\n");
	      else{
		if (ru->is_slave == 1){
                  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Received RRU_sync_ok from RRU %d\n",ru->idx);
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
		LOG_I(PHY,"Received Frame Resynch message with value %d\n",ru->cmdval);
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
