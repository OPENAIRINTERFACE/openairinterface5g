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

#ifndef OAIORAN_H
#define OAIORAN_H

#include <stdint.h>
#include "xran_fh_o_du.h"

typedef struct {
  uint32_t tti;
  uint32_t sl;
  uint32_t f;
} oran_sync_info_t;

void oai_xran_fh_rx_callback(void *pCallbackTag, xran_status_t status);
int oai_physide_dl_tti_call_back(void *param);

#endif /* OAIORAN_H */
