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
#include <string.h>
#include "common_lib.h"
#include "radio/ETHERNET/ethernet_lib.h"
#include "oran_isolate.h"
#include "oran-init.h"
#include "xran_fh_o_du.h"
#include "xran_sync_api.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "openair1/PHY/defs_gNB.h"
#include "common/utils/threadPool/thread-pool.h"
#include "oaioran.h"

typedef struct {
  eth_state_t e;
  rru_config_msg_type_t last_msg;
  int capabilities_sent;
  void *oran_priv;
} oran_eth_state_t;

notifiedFIFO_t oran_sync_fifo;

int trx_oran_start(openair0_device *device)
{
  printf("ORAN: %s\n", __FUNCTION__);

  oran_eth_state_t *s = device->priv;

  // Start ORAN
  if (xran_start(s->oran_priv) != 0) {
    printf("%s:%d:%s: Start ORAN failed ... Exit\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  } else {
    printf("Start ORAN. Done\n");
  }
  return 0;
}

void trx_oran_end(openair0_device *device)
{
  printf("ORAN: %s\n", __FUNCTION__);
  oran_eth_state_t *s = device->priv;
  xran_close(s->oran_priv);
}

int trx_oran_stop(openair0_device *device)
{
  printf("ORAN: %s\n", __FUNCTION__);
  oran_eth_state_t *s = device->priv;
  xran_stop(s->oran_priv);
  return (0);
}

int trx_oran_set_freq(openair0_device *device, openair0_config_t *openair0_cfg)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return (0);
}

int trx_oran_set_gains(openair0_device *device, openair0_config_t *openair0_cfg)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return (0);
}

int trx_oran_get_stats(openair0_device *device)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return (0);
}

int trx_oran_reset_stats(openair0_device *device)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return (0);
}

int ethernet_tune(openair0_device *device, unsigned int option, int value)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return 0;
}

int trx_oran_write_raw(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return 0;
}

int trx_oran_read_raw(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc)
{
  printf("ORAN: %s\n", __FUNCTION__);
  return 0;
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

  if (t < 0 || t > 11)
    return "UNKNOWN";
  return s[t];
}

int trx_oran_ctlsend(openair0_device *device, void *msg, ssize_t msg_len)
{
  RRU_CONFIG_msg_t *rru_config_msg = msg;
  oran_eth_state_t *s = device->priv;

  printf("ORAN: %s\n", __FUNCTION__);

  printf("    rru_config_msg->type %d [%s]\n", rru_config_msg->type, msg_type(rru_config_msg->type));

  s->last_msg = rru_config_msg->type;

  return msg_len;
}

int trx_oran_ctlrecv(openair0_device *device, void *msg, ssize_t msg_len)
{
  RRU_CONFIG_msg_t *rru_config_msg = msg;
  oran_eth_state_t *s = device->priv;

  printf("ORAN: %s\n", __FUNCTION__);

  if (s->last_msg == RAU_tick && s->capabilities_sent == 0) {
    printf("ORAN ctrlrcv RRU_tick received and send capabilities hard coded\n");
    RRU_capabilities_t *cap;
    rru_config_msg->type = RRU_capabilities;
    rru_config_msg->len = sizeof(RRU_CONFIG_msg_t) - MAX_RRU_CONFIG_SIZE + sizeof(RRU_capabilities_t);
    // Fill RRU capabilities (see openair1/PHY/defs_RU.h)
    // For now they are hard coded - try to retreive the params from openari device

    cap = (RRU_capabilities_t *)&rru_config_msg->msg[0];
    cap->FH_fmt = OAI_IF4p5_only;
    cap->num_bands = 1;
    cap->band_list[0] = 78;
    // cap->num_concurrent_bands             = 1; component carriers
    cap->nb_rx[0] = 1; // device->openair0_cfg->rx_num_channels;
    cap->nb_tx[0] = 1; // device->openair0_cfg->tx_num_channels;
    cap->max_pdschReferenceSignalPower[0] = -27;
    cap->max_rxgain[0] = 90;
    cap->N_RB_DL[0] = 106;
    cap->N_RB_UL[0] = 106;

    s->capabilities_sent = 1;

    return rru_config_msg->len;
  }
  if (s->last_msg == RRU_config) {
    printf("Oran RRU_config\n");
    rru_config_msg->type = RRU_config_ok;
  }
  return 0;
}

void oran_fh_if4p5_south_in(RU_t *ru, int *frame, int *slot)
{
  ru_info_t ru_info;
  ru_info.nb_rx = ru->nb_rx;
  ru_info.rxdataF = ru->common.rxdataF;
  ru_info.prach_buf = ru->prach_rxsigF[0]; // index: [prach_oca][ant_id]

  RU_proc_t *proc = &ru->proc;
  extern uint16_t sl_ahead;
  int f, sl;
  LOG_D(PHY, "Read rxdataF %p,%p\n", ru_info.rxdataF[0], ru_info.rxdataF[1]);
  start_meas(&ru->rx_fhaul);
  int ret = xran_fh_rx_read_slot(&ru_info, &f, &sl);
  stop_meas(&ru->rx_fhaul);
  LOG_D(PHY, "Read %d.%d rxdataF %p,%p\n", f, sl, ru_info.rxdataF[0], ru_info.rxdataF[1]);
  if (ret != 0) {
    printf("ORAN: %d.%d ORAN_fh_if4p5_south_in ERROR in RX function \n", f, sl);
  }

  int slots_per_frame = 10 << (ru->openair0_cfg.nr_scs_for_raster);
  proc->tti_rx = sl;
  proc->frame_rx = f;
  proc->tti_tx = (sl + sl_ahead) % slots_per_frame;
  proc->frame_tx = (sl > (slots_per_frame - 1 - sl_ahead)) ? (f + 1) & 1023 : f;

  if (proc->first_rx == 0) {
    if (proc->tti_rx != *slot) {
      LOG_E(PHY,
            "Received Time doesn't correspond to the time we think it is (slot mismatch, received %d.%d, expected %d.%d)\n",
            proc->frame_rx,
            proc->tti_rx,
            *frame,
            *slot);
      *slot = proc->tti_rx;
    }

    if (proc->frame_rx != *frame) {
      LOG_E(PHY,
            "Received Time doesn't correspond to the time we think it is (frame mismatch, %d.%d , expected %d.%d)\n",
            proc->frame_rx,
            proc->tti_rx,
            *frame,
            *slot);
      *frame = proc->frame_rx;
    }
  } else {
    proc->first_rx = 0;
    LOG_I(PHY, "before adjusting, OAI: frame=%d slot=%d, XRAN: frame=%d slot=%d\n", *frame, *slot, proc->frame_rx, proc->tti_rx);
    *frame = proc->frame_rx;
    *slot = proc->tti_rx;
    LOG_I(PHY, "After adjusting, OAI: frame=%d slot=%d, XRAN: frame=%d slot=%d\n", *frame, *slot, proc->frame_rx, proc->tti_rx);
  }
}

void oran_fh_if4p5_south_out(RU_t *ru, int frame, int slot, uint64_t timestamp)
{
  start_meas(&ru->tx_fhaul);
  ru_info_t ru_info;
  ru_info.nb_tx = ru->nb_tx;
  ru_info.txdataF_BF = ru->common.txdataF_BF;
  // printf("south_out:\tframe=%d\tslot=%d\ttimestamp=%ld\n",frame,slot,timestamp);

  int ret = xran_fh_tx_send_slot(&ru_info, frame, slot, timestamp);
  if (ret != 0) {
    printf("ORAN: ORAN_fh_if4p5_south_out ERROR in TX function \n");
  }
  stop_meas(&ru->tx_fhaul);
}

void *get_internal_parameter(char *name)
{
  printf("ORAN: %s\n", __FUNCTION__);

  if (!strcmp(name, "fh_if4p5_south_in"))
    return (void *)oran_fh_if4p5_south_in;
  if (!strcmp(name, "fh_if4p5_south_out"))
    return (void *)oran_fh_if4p5_south_out;

  return NULL;
}

__attribute__((__visibility__("default"))) int transport_init(openair0_device *device,
                                                              openair0_config_t *openair0_cfg,
                                                              eth_params_t *eth_params)
{
  oran_eth_state_t *eth;

  device->Mod_id = 0;
  device->transp_type = ETHERNET_TP;
  device->trx_start_func = trx_oran_start;
  device->trx_get_stats_func = trx_oran_get_stats;
  device->trx_reset_stats_func = trx_oran_reset_stats;
  device->trx_end_func = trx_oran_end;
  device->trx_stop_func = trx_oran_stop;
  device->trx_set_freq_func = trx_oran_set_freq;
  device->trx_set_gains_func = trx_oran_set_gains;

  device->trx_write_func = trx_oran_write_raw;
  device->trx_read_func = trx_oran_read_raw;

  device->trx_ctlsend_func = trx_oran_ctlsend;
  device->trx_ctlrecv_func = trx_oran_ctlrecv;

  device->get_internal_parameter = get_internal_parameter;

  eth = (oran_eth_state_t *)calloc(1, sizeof(oran_eth_state_t));
  if (eth == NULL) {
    AssertFatal(0 == 1, "out of memory\n");
  }

  eth->e.flags = ETH_RAW_IF4p5_MODE;
  eth->e.compression = NO_COMPRESS;
  eth->e.if_name = eth_params->local_if_name;
  eth->oran_priv = NULL; // define_oran_pointer();
  device->priv = eth;
  device->openair0_cfg = &openair0_cfg[0];

  eth->last_msg = (rru_config_msg_type_t)-1;

  printf("ORAN: %s\n", __FUNCTION__);

  initNotifiedFIFO(&oran_sync_fifo);
  eth->oran_priv = oai_oran_initialize(openair0_cfg);
  AssertFatal(eth->oran_priv != NULL, "can not initialize fronthaul");
  // create message queues for ORAN sync
  return 0;
}
