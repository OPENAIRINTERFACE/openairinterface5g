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

#include <stdio.h>

#include "common_lib.h"
#include "ethernet_lib.h"
#include "shared_buffers.h"
#include "low.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

typedef struct {
  eth_state_t           e;
  shared_buffers        buffers;
  rru_config_msg_type_t last_msg;
  int                   capabilities_sent;
  void                  *benetel_priv;
  char                  *dpdk_main_command_line;
} benetel_eth_state_t;

int trx_benetel_start(openair0_device *device)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return 0;
}


void trx_benetel_end(openair0_device *device)
{
  printf("BENETEL: %s\n", __FUNCTION__);
}


int trx_benetel_stop(openair0_device *device)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return(0);
}


int trx_benetel_set_freq(openair0_device* device, openair0_config_t *openair0_cfg)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return(0);
}


int trx_benetel_set_gains(openair0_device* device,
                          openair0_config_t *openair0_cfg)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return(0);
}


int trx_benetel_get_stats(openair0_device* device)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return(0);
}


int trx_benetel_reset_stats(openair0_device* device)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return(0);
}


int ethernet_tune(openair0_device *device,
                  unsigned int option,
                  int value)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return 0;
}

int trx_benetel_write_raw(openair0_device *device,
                          openair0_timestamp timestamp,
                          void **buff, int nsamps, int cc, int flags)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return nsamps*4;
}

int trx_benetel_read_raw(openair0_device *device,
                         openair0_timestamp *timestamp,
                         void **buff, int nsamps, int cc)
{
  printf("BENETEL: %s\n", __FUNCTION__);
  return nsamps*4;
}

char *msg_type(int t)
{
  static char *s[12] = {
    "RAU_tick",
    "RRU_capabilities",
    "RRU_config",
    "RRU_config_ok",
    "RRU_start",
    "RRU_stop",
    "RRU_sync_ok",
    "RRU_frame_resynch",
    "RRU_MSG_max_num",
    "RRU_check_sync",
    "RRU_config_update",
    "RRU_config_update_ok",
  };

  if (t < 0 || t > 11) return "UNKNOWN";
  return s[t];
}

int trx_benetel_ctlsend(openair0_device *device, void *msg, ssize_t msg_len)
{
  RRU_CONFIG_msg_t *rru_config_msg = msg;
  benetel_eth_state_t *s = device->priv;

  printf("BENETEL: %s\n", __FUNCTION__);

  printf("    rru_config_msg->type %d [%s]\n", rru_config_msg->type,
         msg_type(rru_config_msg->type));

  s->last_msg = rru_config_msg->type;

  return msg_len;
}

int trx_benetel_ctlrecv(openair0_device *device, void *msg, ssize_t msg_len)
{
  RRU_CONFIG_msg_t *rru_config_msg = msg;
  benetel_eth_state_t *s = device->priv;

  printf("BENETEL: %s\n", __FUNCTION__);

  if (s->last_msg == RAU_tick && s->capabilities_sent == 0) {
    RRU_capabilities_t *cap;
    rru_config_msg->type = RRU_capabilities;
    rru_config_msg->len  = sizeof(RRU_CONFIG_msg_t)-MAX_RRU_CONFIG_SIZE+sizeof(RRU_capabilities_t);
    cap = (RRU_capabilities_t*)&rru_config_msg->msg[0];
    cap->FH_fmt                           = OAI_IF4p5_only;
    cap->num_bands                        = 1;
    cap->band_list[0]                     = 7;
    cap->nb_rx[0]                         = device->openair0_cfg->rx_num_channels;
    cap->nb_tx[0]                         = device->openair0_cfg->tx_num_channels;
    cap->max_pdschReferenceSignalPower[0] = -27;
    cap->max_rxgain[0]                    = 90;

    s->capabilities_sent = 1;

    return rru_config_msg->len;
  }
  if (s->last_msg == RRU_config) {
    rru_config_msg->type = RRU_config_ok;
    return 0;
  }
  if (s->last_msg == RRU_start) {
    s->benetel_priv = benetel_start(s->e.if_name, &s->buffers, s->dpdk_main_command_line);
  }
  /* correct? */
  while (1) pause();

  return 0;
}

void benetel_fh_if4p5_south_in(RU_t *ru,
                               int *frame,
                               int *subframe)
{
  benetel_eth_state_t *s = ru->ifdevice.priv;
  PHY_VARS_eNB **eNB_list = ru->eNB_list, *eNB;
  LTE_DL_FRAME_PARMS *fp;
  int symbol;
  int32_t *rxdata;
  int antenna;

  lock_ul_buffer(&s->buffers, *subframe);
next:
  while (!((s->buffers.ul_busy[0][*subframe] == 0x3fff &&
            s->buffers.ul_busy[1][*subframe] == 0x3fff) ||
           s->buffers.prach_busy[*subframe] == 1))
    wait_ul_buffer(&s->buffers, *subframe);
  if (s->buffers.prach_busy[*subframe] == 1) {
    int i;
    int antenna = 0;
    uint16_t *in;
    uint16_t *out;
    in = (uint16_t *)s->buffers.prach[*subframe];
    out = (uint16_t *)ru->prach_rxsigF[0][antenna];
    for (i = 0; i < 840*2; i++)
      out[i] = ntohs(in[i]);
    s->buffers.prach_busy[*subframe] = 0;
    ru->wakeup_prach_eNB(ru->eNB_list[0], ru, *frame, *subframe);
    goto next;
  }

  eNB = eNB_list[0];
  fp  = &eNB->frame_parms;
  for (antenna = 0; antenna < ru->nb_rx; antenna++) {
    for (symbol = 0; symbol < 14; symbol++) {
      int i;
      uint16_t *p = (uint16_t *)(&s->buffers.ul[antenna][*subframe][symbol*1200*4]);
      for (i = 0; i < 1200*2; i++) {
        p[i] = htons(p[i]);
      }
      rxdata = &ru->common.rxdataF[antenna][symbol * fp->ofdm_symbol_size];
#if 1
      memcpy(rxdata + 2048 - 600,
             &s->buffers.ul[antenna][*subframe][symbol*1200*4],
             600 * 4);
      memcpy(rxdata,
             &s->buffers.ul[antenna][*subframe][symbol*1200*4] + 600*4,
             600 * 4);
#endif
    }
  }

  s->buffers.ul_busy[0][*subframe] = 0;
  s->buffers.ul_busy[1][*subframe] = 0;
  signal_ul_buffer(&s->buffers, *subframe);
  unlock_ul_buffer(&s->buffers, *subframe);

  //printf("BENETEL: %s (f.sf %d.%d)\n", __FUNCTION__, *frame, *subframe);

  RU_proc_t *proc = &ru->proc;
  extern uint16_t sf_ahead;
  int f = *frame;
  int sf = *subframe;

  //calculate timestamp_rx, timestamp_tx based on frame and subframe
  proc->tti_rx       = sf;
  proc->frame_rx     = f;
  proc->timestamp_rx = ((proc->frame_rx * 10)  + proc->tti_rx ) * fp->samples_per_tti ;

  if (get_nprocs()<=4) {
    proc->tti_tx   = (sf+sf_ahead)%10;
    proc->frame_tx = (sf>(9-sf_ahead)) ? (f+1)&1023 : f;
  }

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_IF4P5_SOUTH_IN_RU+ru->idx,f);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_IF4P5_SOUTH_IN_RU+ru->idx,sf);
  proc->symbol_mask[sf] = 0;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff);

}

void benetel_fh_if4p5_south_out(RU_t *ru,
                                int frame,
                                int subframe,
                                uint64_t timestamp)
{
  benetel_eth_state_t *s = ru->ifdevice.priv;
  PHY_VARS_eNB **eNB_list = ru->eNB_list, *eNB;
  LTE_DL_FRAME_PARMS *fp;
  int symbol;
  int32_t *txdata;
  int aa;

  //printf("BENETEL: %s (f.sf %d.%d ts %ld)\n", __FUNCTION__, frame, subframe, timestamp);

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );

  lock_dl_buffer(&s->buffers, subframe);
  if (s->buffers.dl_busy[0][subframe] ||
      s->buffers.dl_busy[1][subframe]) {
    printf("%s: fatal: DL buffer busy for subframe %d\n", __FUNCTION__, subframe);
     unlock_dl_buffer(&s->buffers, subframe);
    return;
  }
  eNB = eNB_list[0];
  fp  = &eNB->frame_parms;
  if (ru->num_eNB != 1 || fp->ofdm_symbol_size != 2048 ||
      fp->Ncp != NORMAL || fp->symbols_per_tti != 14) {
    printf("%s:%d:%s: unsupported configuration\n",
           __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  for (aa = 0; aa < ru->nb_tx; aa++) {
    for (symbol = 0; symbol < 14; symbol++) {
      txdata = &ru->common.txdataF_BF[aa][symbol * fp->ofdm_symbol_size];
#if 1
      memcpy(&s->buffers.dl[aa][subframe][symbol*1200*4],
             txdata + 2048 - 600,
             600 * 4);
      memcpy(&s->buffers.dl[aa][subframe][symbol*1200*4] + 600*4,
             txdata + 1,
             600 * 4);
#endif
      int i;
      uint16_t *p = (uint16_t *)(&s->buffers.dl[aa][subframe][symbol*1200*4]);
      for (i = 0; i < 1200*2; i++) {
        p[i] = htons(p[i]);
      }
    }
  }

  s->buffers.dl_busy[0][subframe] = 0x3fff;
  s->buffers.dl_busy[1][subframe] = 0x3fff;
  unlock_dl_buffer(&s->buffers, subframe);

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_IF4P5_SOUTH_OUT_RU+ru->idx, ru->proc.frame_tx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_IF4P5_SOUTH_OUT_RU+ru->idx, ru->proc.tti_tx);

}

void *get_internal_parameter(char *name)
{
  printf("BENETEL: %s\n", __FUNCTION__);

  if (!strcmp(name, "fh_if4p5_south_in"))
    return benetel_fh_if4p5_south_in;
  if (!strcmp(name, "fh_if4p5_south_out"))
    return benetel_fh_if4p5_south_out;

  return NULL;
}

__attribute__((__visibility__("default")))
int transport_init(openair0_device *device,
                   openair0_config_t *openair0_cfg,
                   eth_params_t * eth_params )
{
  benetel_eth_state_t *eth;

  printf("BENETEL: %s\n", __FUNCTION__);

  device->Mod_id               = 0;
  device->transp_type          = ETHERNET_TP;
  device->trx_start_func       = trx_benetel_start;
  device->trx_get_stats_func   = trx_benetel_get_stats;
  device->trx_reset_stats_func = trx_benetel_reset_stats;
  device->trx_end_func         = trx_benetel_end;
  device->trx_stop_func        = trx_benetel_stop;
  device->trx_set_freq_func    = trx_benetel_set_freq;
  device->trx_set_gains_func   = trx_benetel_set_gains;

  device->trx_write_func       = trx_benetel_write_raw;
  device->trx_read_func        = trx_benetel_read_raw;

  device->trx_ctlsend_func     = trx_benetel_ctlsend;
  device->trx_ctlrecv_func     = trx_benetel_ctlrecv;

  device->get_internal_parameter = get_internal_parameter;

  eth = calloc(1, sizeof(benetel_eth_state_t));
  if (eth == NULL) {
    AssertFatal(0==1, "out of memory\n");
  }

  eth->e.flags = ETH_RAW_IF4p5_MODE;
  eth->e.compression = NO_COMPRESS;
  eth->e.if_name = eth_params->local_if_name;
  device->priv = eth;
  device->openair0_cfg=&openair0_cfg[0];

  if (openair0_cfg->sdr_addrs == NULL) {
    printf("benetel: fatal: sdr_addrs not set in configuration file\n");
    exit(1);
  }

  eth->dpdk_main_command_line = strdup(openair0_cfg->sdr_addrs);
  if (eth->dpdk_main_command_line == NULL) {
    AssertFatal(0==1, "out of memory\n");
  }

  eth->last_msg = -1;

  init_buffers(&eth->buffers);

  return 0;
}
