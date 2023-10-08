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

#ifndef _ORAN_H_
#define _ORAN_H_

#include "shared_buffers.h"
#include "common_lib.h"
void oran_fh_if4p5_south_out(RU_t *ru, int frame, int slot, uint64_t timestamp);

void oran_fh_if4p5_south_in(RU_t *ru, int *frame, int *slot);

int transport_init(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t *eth_params);

typedef struct {
  eth_state_t e;
  shared_buffers buffers;
  rru_config_msg_type_t last_msg;
  int capabilities_sent;
  void *oran_priv;
} oran_eth_state_t;
#endif /* _ORAN_H_ */
