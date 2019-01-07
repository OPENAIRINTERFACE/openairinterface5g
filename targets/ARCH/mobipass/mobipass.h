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

#ifndef _MOBIPASS_H_
#define _MOBIPASS_H_

#include <stdint.h>
#include "ethernet_lib.h"

typedef struct {
  /* this has to come first */
  eth_state_t eth;

  void *qstate;

  uint8_t eth_local[6];
  uint8_t eth_remote[6];

  int samples_per_1024_frames;

  /* variables used by the function interface.c:mobipass_read */
  uint32_t mobipass_read_ts;
  unsigned char mobipass_read_seqno;

  /* variables used by the function interface.c:mobipass_write */
  uint32_t mobipass_write_last_timestamp;

  /* variables used by the function mobipass.c:[init_time|synch_time] */
  uint64_t t0;

  /* variables used by the function mobipass.c:synch_time */
  uint32_t synch_time_last_ts;
  uint64_t synch_time_mega_ts;

  /* sock is used in mobipass.c */
  int sock;
} mobipass_state_t;

void init_mobipass(mobipass_state_t *mobi);

#endif /* _MOBIPASS_H_ */
