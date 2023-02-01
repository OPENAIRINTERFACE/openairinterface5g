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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "nr_pdcp_e1_api.h"

void e1_add_drb(int is_gnb,
                uint64_t ue_id,
                struct NR_DRB_ToAddMod *s,
                int ciphering_algorithm,
                int integrity_algorithm,
                unsigned char *ciphering_key,
                unsigned char *integrity_key)
{
  add_drb_am(is_gnb, ue_id, s, ciphering_algorithm, integrity_algorithm,
             ciphering_key, integrity_key);
  LOG_I(PDCP, "%s:%s:%d: added DRB for UE ID %ld\n", __FILE__, __FUNCTION__, __LINE__, ue_id);
}

void nr_pdcp_e1_add_drbs(eNB_flag_t enb_flag,
                         uint64_t ue_id,
                         NR_DRB_ToAddModList_t  *const drb2add_list,
                         const uint8_t  security_modeP,
                         uint8_t        *const kUPenc,
                         uint8_t        *const kUPint) {

  if (drb2add_list != NULL) {
    for (int i = 0; i < drb2add_list->list.count; i++) {
      e1_add_drb(enb_flag, ue_id, drb2add_list->list.array[i],
              security_modeP & 0x0f, (security_modeP >> 4) & 0x0f,
              kUPenc, kUPint);
    }
  } else
    LOG_W(PDCP, "%s with void list\n", __FUNCTION__);
  if (kUPenc)
    free(kUPenc);
  if (kUPint)
    free(kUPint);
}
