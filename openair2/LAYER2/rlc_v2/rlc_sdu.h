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

#ifndef _RLC_SDU_H_
#define _RLC_SDU_H_

typedef struct rlc_sdu_t {
  int upper_layer_id;
  char *data;
  int size;
  /* next_byte indicates the starting byte to use to construct a new PDU */
  int next_byte;
  int acked_bytes;
  struct rlc_sdu_t *next;
} rlc_sdu_t;

rlc_sdu_t *rlc_new_sdu(char *buffer, int size, int upper_layer_id);
void rlc_free_sdu(rlc_sdu_t *sdu);
void rlc_sdu_list_add(rlc_sdu_t **list, rlc_sdu_t **end, rlc_sdu_t *sdu);

#endif /* _RLC_SDU_H_ */
