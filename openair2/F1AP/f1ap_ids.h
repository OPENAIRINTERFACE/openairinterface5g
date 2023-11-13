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

/* "standalone" module to store a "secondary" UE ID for each UE in DU/CU.
 * Separate from the rest of F1, as it is also relevant for monolithic. */

#ifndef F1AP_IDS_H_
#define F1AP_IDS_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>

typedef struct f1_ue_data_t {
  uint32_t secondary_ue;
  sctp_assoc_t e1_assoc_id;
  sctp_assoc_t du_assoc_id; // only used at the CU
} f1_ue_data_t;

void cu_init_f1_ue_data(void);
bool cu_add_f1_ue_data(uint32_t ue_id, const f1_ue_data_t *data);
bool cu_exists_f1_ue_data(uint32_t ue_id);
f1_ue_data_t cu_get_f1_ue_data(uint32_t ue_id);
bool cu_remove_f1_ue_data(uint32_t ue_id);

void du_init_f1_ue_data(void);
bool du_add_f1_ue_data(uint32_t ue_id, const f1_ue_data_t *data);
bool du_exists_f1_ue_data(uint32_t ue_id);
f1_ue_data_t du_get_f1_ue_data(uint32_t ue_id);
bool du_remove_f1_ue_data(uint32_t ue_id);

#endif /* F1AP_IDS_H_ */
