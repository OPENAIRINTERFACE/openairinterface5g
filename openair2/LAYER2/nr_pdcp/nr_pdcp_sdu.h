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

#ifndef _NR_PDCP_SDU_H_
#define _NR_PDCP_SDU_H_

#include <stdint.h>

typedef struct nr_pdcp_sdu_t {
  uint32_t             count;
  char                 *buffer;
  int                  size;
  struct nr_pdcp_sdu_t *next;
} nr_pdcp_sdu_t;

nr_pdcp_sdu_t *nr_pdcp_new_sdu(uint32_t count, char *buffer, int size);
nr_pdcp_sdu_t *nr_pdcp_sdu_list_add(nr_pdcp_sdu_t *l, nr_pdcp_sdu_t *sdu);
int nr_pdcp_sdu_in_list(nr_pdcp_sdu_t *l, uint32_t count);
void nr_pdcp_free_sdu(nr_pdcp_sdu_t *sdu);

#endif /* _NR_PDCP_SDU_H_ */
