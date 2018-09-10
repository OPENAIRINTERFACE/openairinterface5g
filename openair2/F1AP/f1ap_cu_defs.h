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

#include <stdint.h>

#include "queue.h"
#include "tree.h"

#include "sctp_eNB_defs.h"

#ifndef CU_F1AP_DEFS_H_
#define CU_F1AP_DEFS_H_

struct cu_f1ap_instance_s;
typedef struct du_f1ap_instance_s {
  /* Next f1ap du association.
   * Only used for virtual mode.
   */

  /* For virtual mode, mod_id as defined in the rest of the L1/L2 stack */
  instance_t instance;

  // F1_Setup_Req payload
  uint32_t gNB_CU_id;
  char *gNB_CU_name;

  /* Unique eNB_id to identify the eNB within EPC.
   * In our case the eNB is a macro eNB so the id will be 20 bits long.
   * For Home eNB id, this field should be 28 bits long.
   */
  uint32_t eNB_id;

  /* Tracking area code */
  uint16_t tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t  mcc;
  uint16_t  mnc;
  uint8_t   mnc_digit_length;

} cu_f1ap_instance_t;

#endif /* CU_F1AP_DEFS_H_ */
