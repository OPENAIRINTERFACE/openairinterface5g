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
#if 0
struct timespec t;
static struct timespec old;
clock_gettime(CLOCK_REALTIME, &t);
printf("store_ul %d.%ld (%ld)\n", (int)(t.tv_sec % 60), t.tv_nsec, t.tv_nsec - old.tv_nsec);
old = t;
#endif

  int a = ul->antenna;

  if (ul->slot != bs->next_slot[a] ||
      ul->symbol != bs->next_symbol[a]) {
    printf("%s: error, antenna %d expected frame.sl.symbol %d.%d.%d, got %d.%d.%d\n",
           __FUNCTION__,
           a, bs->expected_benetel_frame[a], bs->next_slot[a], bs->next_symbol[a],
           ul->frame, ul->slot, ul->symbol);
  }

  /* fill missing data with 0s */
  while (ul->slot != bs->next_slot[a] ||
         ul->symbol != bs->next_symbol[a]) {
    lock_ul_buffer(bs->buffers, bs->next_slot[a]);
    if (bs->buffers->ul_busy[a][bs->next_slot[a]] & (1 << bs->next_symbol[a])) {
      printf("%s: warning, antenna %d UL overflow (sl.symbol %d.%d)\n", __FUNCTION__,
             a, bs->next_slot[a], bs->next_symbol[a]);
    }

    memset(bs->buffers->ul[a][bs->next_slot[a]] + bs->next_symbol[a] * 1272*4,
           0, 1272*4);
    bs->buffers->ul_busy[a][bs->next_slot[a]] |= (1 << bs->next_symbol[a]);
    signal_ul_buffer(bs->buffers, bs->next_slot[a]);
    unlock_ul_buffer(bs->buffers, bs->next_slot[a]);

    bs->next_symbol[a]++;
    if (bs->next_symbol[a] == 14) {
      bs->next_symbol[a] = 0;
      bs->next_slot[a] = (bs->next_slot[a] + 1) % 20;
      if (bs->next_slot[a] == 0) {
        bs->expected_benetel_frame[a]++;
        bs->expected_benetel_frame[a] &= 255;
      }
    }
  }

  lock_ul_buffer(bs->buffers, bs->next_slot[a]);
  if (bs->buffers->ul_busy[a][bs->next_slot[a]] & (1 << bs->next_symbol[a])) {
    printf("%s: warning, antenna %d UL overflow (sl.symbol %d.%d)\n", __FUNCTION__,
           a, bs->next_slot[a], bs->next_symbol[a]);
  }

  memcpy(bs->buffers->ul[a][bs->next_slot[a]] + bs->next_symbol[a] * 1272*4,
         ul->iq, 1272*4);
  bs->buffers->ul_busy[a][bs->next_slot[a]] |= (1 << bs->next_symbol[a]);
  signal_ul_buffer(bs->buffers, bs->next_slot[a]);
  unlock_ul_buffer(bs->buffers, bs->next_slot[a]);

  bs->next_symbol[a]++;
  if (bs->next_symbol[a] == 14) {
    bs->next_symbol[a] = 0;
    bs->next_slot[a] = (bs->next_slot[a] + 1) % 20;
    if (bs->next_slot[a] == 0) {
      bs->expected_benetel_frame[a]++;
      bs->expected_benetel_frame[a] &= 255;
    }
  }
}

void store_prach(benetel_t *bs, int frame, int slot, void *data)
{
  static int last_frame = -1;
  static int last_slot = -1;
  /* hack: antenna number is always 0, discard second packet with same f/sf */
  //if (frame == last_frame && slot == last_slot) return;
  if (frame == last_frame && slot == last_slot) { printf("got f.sl %d.%d twice\n", frame, slot); return; }
  last_frame = frame;
  last_slot = slot;

  lock_ul_buffer(bs->buffers, slot);
  if (bs->buffers->prach_busy[slot]) {
    printf("store_prach: fatal: previous prach buffer not processed\n");
    unlock_ul_buffer(bs->buffers, slot);
    return;
  }
  bs->buffers->prach_busy[slot] = 1;
  memcpy(bs->buffers->prach[slot], data, 839*4);
  signal_ul_buffer(bs->buffers, slot);
  unlock_ul_buffer(bs->buffers, slot);

}

void *benetel_start_dpdk(char *ifname, shared_buffers *buffers, char *dpdk_main_command_line);

void *benetel_start(char *ifname, shared_buffers *buffers, char *dpdk_main_command_line)
{
  if (!strcmp(ifname, "dpdk"))
    return benetel_start_dpdk(ifname, buffers, dpdk_main_command_line);
  printf("benetel: fatal: interface %s not supported, only dpdpk is supported\n", ifname);
  exit(1);
}
