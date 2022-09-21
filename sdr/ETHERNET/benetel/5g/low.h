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

#ifndef _BENETEL_5G_LOW_H_
#define _BENETEL_5G_LOW_H_

#include "shared_buffers.h"

typedef struct {
  shared_buffers *buffers;
  /* [2] is for two antennas */
  int            next_slot[2];
  int            next_symbol[2];
  int            expected_benetel_frame[2];
  char           *dpdk_main_command_line;
} benetel_t;

typedef struct {
  int frame;
  int slot;
  int symbol;
  int antenna;
  unsigned char iq[5088];
} ul_packet_t;

void *benetel_start(char *ifname, shared_buffers *buffers, char *dpdk_main_command_line);

void store_ul(benetel_t *bs, ul_packet_t *ul);
void store_prach(benetel_t *bs, int frame, int slot, void *data);

#endif /* _BENETEL_5G_LOW_H_ */
