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

#include "low.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void store_ul(benetel_t *bs, ul_packet_t *ul)
{
  int a = ul->antenna;

  if (ul->subframe != bs->next_subframe[a] ||
      ul->symbol != bs->next_symbol[a]) {
    printf("%s: fatal, antenna %d expected frame.sf.symbol %d.%d.%d, got %d.%d.%d\n",
           __FUNCTION__,
           a,
           bs->expected_benetel_frame[a], bs->next_subframe[a], bs->next_symbol[a],
           ul->frame, ul->subframe, ul->symbol);
    exit(1);
  }

  lock_ul_buffer(bs->buffers, bs->next_subframe[a]);
  if (bs->buffers->ul_busy[a][bs->next_subframe[a]] & (1 << bs->next_symbol[a])) {
    printf("%s: warning, UL overflow (sf.symbol %d.%d)\n", __FUNCTION__,
           bs->next_subframe[a], bs->next_symbol[a]);
  }
  memcpy(bs->buffers->ul[a][bs->next_subframe[a]] + bs->next_symbol[a] * 1200*4,
         ul->iq, 1200*4);
  bs->buffers->ul_busy[a][bs->next_subframe[a]] |= (1 << bs->next_symbol[a]);
  signal_ul_buffer(bs->buffers, bs->next_subframe[a]);
  unlock_ul_buffer(bs->buffers, bs->next_subframe[a]);

  bs->next_symbol[a]++;
  if (bs->next_symbol[a] == 14) {
    bs->next_symbol[a] = 0;
    bs->next_subframe[a] = (bs->next_subframe[a] + 1) % 10;
    if (bs->next_subframe[a] == 0) {
      bs->expected_benetel_frame[a]++;
      bs->expected_benetel_frame[a] &= 255;
    }
  }
}

void store_prach(benetel_t *bs, int frame, int subframe, void *data)
{
  static int last_frame = -1;
  static int last_subframe = -1;
  /* hack: antenna number is always 0, discard second packet with same f/sf */
  if (frame == last_frame && subframe == last_subframe) return;
  last_frame = frame;
  last_subframe = subframe;

  lock_ul_buffer(bs->buffers, subframe);
  if (bs->buffers->prach_busy[subframe]) {
    printf("store_prach: fatal: previous prach buffer not processed\n");
    unlock_ul_buffer(bs->buffers, subframe);
    return;
  }
  bs->buffers->prach_busy[subframe] = 1;
  memcpy(bs->buffers->prach[subframe], data, 849*4);
  signal_ul_buffer(bs->buffers, subframe);
  unlock_ul_buffer(bs->buffers, subframe);

}

void *benetel_start_dpdk(char *ifname, shared_buffers *buffers, char *dpdk_main_command_line);

void *benetel_start(char *ifname, shared_buffers *buffers, char *dpdk_main_command_line)
{
  if (!strcmp(ifname, "dpdk"))
    return benetel_start_dpdk(ifname, buffers, dpdk_main_command_line);
  printf("benetel: fatal: interface %s not supported, only dpdpk is supported\n", ifname);
  exit(1);
}
