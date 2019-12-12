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

#ifndef _NR_RLC_SDU_H_
#define _NR_RLC_SDU_H_

typedef struct nr_rlc_sdu_t {
  int sn;
  int upper_layer_id;
  char *data;
  int size;
  int retx_count;

  int ref_count;      /* incremented each time the SDU is segmented */
} nr_rlc_sdu_t;

typedef struct nr_rlc_sdu_segment_t {
  nr_rlc_sdu_t *sdu;
  int size;
  int so;
  int is_first;
  int is_last;
  struct nr_rlc_sdu_segment_t *next;
} nr_rlc_sdu_segment_t;

nr_rlc_sdu_segment_t *nr_rlc_new_sdu(
    char *buffer, int size,
    int upper_layer_id);
void nr_rlc_free_sdu_segment(nr_rlc_sdu_segment_t *sdu);
void nr_rlc_sdu_segment_list_append(nr_rlc_sdu_segment_t **list,
                                    nr_rlc_sdu_segment_t **end,
                                    nr_rlc_sdu_segment_t *sdu);
nr_rlc_sdu_segment_t *nr_rlc_sdu_segment_list_add(
    int (*sn_compare)(void *, int, int), void *sn_compare_data,
    nr_rlc_sdu_segment_t *list, nr_rlc_sdu_segment_t *sdu_segment);
void nr_rlc_free_sdu_segment_list(nr_rlc_sdu_segment_t *l);

#endif /* _NR_RLC_SDU_H_ */
