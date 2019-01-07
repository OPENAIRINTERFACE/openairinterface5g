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

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <errno.h>
#include <linux/sysctl.h>
#include <sys/sysctl.h>

#include "common_lib.h"
#include "ethernet_lib.h"

#include "mobipass.h"
#include "queues.h"

struct mobipass_header {
  uint16_t flags;
  uint16_t fifo_status;
  unsigned char seqno;
  unsigned char ack;
  uint32_t word0;
  uint32_t timestamp;
} __attribute__((__packed__));

int mobipass_start(openair0_device *device) { init_mobipass(device->priv); return 0; }
int mobipass_request(openair0_device *device, void *msg, ssize_t msg_len) { abort(); return 0; }
int mobipass_reply(openair0_device *device, void *msg, ssize_t msg_len) { abort(); return 0; }
int mobipass_get_stats(openair0_device* device) { return 0; }
int mobipass_reset_stats(openair0_device* device) { return 0; }
void mobipass_end(openair0_device *device) {}
int mobipass_stop(openair0_device *device) { return 0; }
int mobipass_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) { return 0; }
int mobipass_set_gains(openair0_device* device, openair0_config_t *openair0_cfg) { return 0; }

int mobipass_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {
  mobipass_state_t *mobi = device->priv;
  struct mobipass_header *mh = (struct mobipass_header *)(((char *)buff[0]) + 14);
  mobi->mobipass_write_last_timestamp += 640;
  mobi->mobipass_write_last_timestamp %= mobi->samples_per_1024_frames;
  mh->timestamp = htonl(ntohl(mh->timestamp) % mobi->samples_per_1024_frames);
  if (mobi->mobipass_write_last_timestamp != ntohl(mh->timestamp))
    { printf("mobipass: ERROR: bad timestamp wanted %d got %d\n", mobi->mobipass_write_last_timestamp, ntohl(mh->timestamp)); exit(1); }
//printf("__write nsamps %d timestamps %ld seqno %d (packet timestamp %d)\n", nsamps, timestamp, mh->seqno, ntohl(mh->timestamp));
  if (nsamps != 640) abort();
  enqueue_to_mobipass(mobi->qstate, buff[0]);
  return nsamps;
}

int mobipass_read(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {
  mobipass_state_t *mobi = device->priv;
//printf("__read nsamps %d return timestamp %d\n", nsamps, ts);
  *timestamp = htonl(mobi->mobipass_read_ts);
  mobi->mobipass_read_ts += nsamps;
  mobi->mobipass_read_ts %= mobi->samples_per_1024_frames;
  if (nsamps != 640) { printf("mobipass: ERROR: bad nsamps %d, should be 640\n", nsamps); fflush(stdout); abort(); }

  dequeue_from_mobipass(mobi->qstate, ntohl(*timestamp), buff[0]);

#if 1
  struct mobipass_header *mh = (struct mobipass_header *)(((char *)buff[0]) + 14);
  mh->flags = 0;
  mh->fifo_status = 0;
  mh->seqno = mobi->mobipass_read_seqno++;
  mh->ack = 0;
  mh->word0 = 0;
  mh->timestamp = htonl(mobi->mobipass_read_ts);
#endif

  return nsamps;
}

/* this is the only function in the library that is visible from outside
 * because in CMakeLists.txt we use -fvisibility=hidden
 */
__attribute__((__visibility__("default")))
int transport_init(openair0_device *device, openair0_config_t *openair0_cfg,
        eth_params_t * eth_params )
{
  //init_mobipass();

  mobipass_state_t *mobi = (mobipass_state_t*)malloc(sizeof(mobipass_state_t));
  memset(mobi, 0, sizeof(mobipass_state_t));

  if (eth_params->transp_preference != 4) goto err;
  if (eth_params->if_compress != 0) goto err;

  /* only 50 PRBs handled for the moment */
  if (openair0_cfg[0].sample_rate != 15360000) {
    printf("mobipass: ERROR: only 50 PRBs supported\n");
    exit(1);
  }

  mobi->eth.flags = ETH_RAW_IF5_MOBIPASS;
  mobi->eth.compression = NO_COMPRESS;
  device->Mod_id           = 0;//num_devices_eth++;
  device->transp_type      = ETHERNET_TP;

  device->trx_start_func   = mobipass_start;
  device->trx_request_func = mobipass_request;
  device->trx_reply_func   = mobipass_reply;
  device->trx_get_stats_func   = mobipass_get_stats;
  device->trx_reset_stats_func = mobipass_reset_stats;
  device->trx_end_func         = mobipass_end;
  device->trx_stop_func        = mobipass_stop;
  device->trx_set_freq_func = mobipass_set_freq;
  device->trx_set_gains_func = mobipass_set_gains;
  device->trx_write_func   = mobipass_write;
  device->trx_read_func    = mobipass_read;

  device->priv = mobi;

  mobi->eth.if_name = strdup(eth_params->local_if_name);
  if (mobi->eth.if_name == NULL) abort();

  if (sscanf(eth_params->my_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
          &mobi->eth_local[0],
          &mobi->eth_local[1],
          &mobi->eth_local[2],
          &mobi->eth_local[3],
          &mobi->eth_local[4],
          &mobi->eth_local[5]) != 6) {
    printf("mobipass: ERROR: bad local ethernet address '%s', check configuration file\n",
           eth_params->my_addr);
    exit(1);
  }

  if (sscanf(eth_params->remote_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
          &mobi->eth_remote[0],
          &mobi->eth_remote[1],
          &mobi->eth_remote[2],
          &mobi->eth_remote[3],
          &mobi->eth_remote[4],
          &mobi->eth_remote[5]) != 6) {
    printf("mobipass: ERROR: bad remote ethernet address '%s', check configuration file\n",
           eth_params->remote_addr);
    exit(1);
  }

  /* note: this only works for 50 PRBs */
  mobi->samples_per_1024_frames = 7680*2*10*1024;

  /* TX starts at subframe 4, let's pretend we are at the right position */
  /* note: this only works for 50 PRBs */
  mobi->mobipass_write_last_timestamp = 4*7680*2-640;

  /* device specific */
  openair0_cfg[0].iq_rxrescale = 15;//rescale iqs
  openair0_cfg[0].iq_txshift = eth_params->iq_txshift;// shift
  openair0_cfg[0].tx_sample_advance = eth_params->tx_sample_advance;

  /* this is useless, I think */
  if (device->host_type == BBU_HOST) {
    /*Note scheduling advance values valid only for case 7680000 */    
    switch ((int)openair0_cfg[0].sample_rate) {
    case 30720000:
      openair0_cfg[0].samples_per_packet    = 3840;     
      break;
    case 23040000:     
      openair0_cfg[0].samples_per_packet    = 2880;
      break;
    case 15360000:
      openair0_cfg[0].samples_per_packet    = 1920;      
      break;
    case 7680000:
      openair0_cfg[0].samples_per_packet    = 960;     
      break;
    case 1920000:
      openair0_cfg[0].samples_per_packet    = 240;     
      break;
    default:
      printf("mobipass: ERROR: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
      exit(-1);
      break;
    }
  }

  device->openair0_cfg=&openair0_cfg[0];

  return 0;

err:
  printf("mobipass: ERROR: bad configuration file?\n");
  exit(1);
}
