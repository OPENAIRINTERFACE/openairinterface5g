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

#ifndef _NR_PDCP_ENTITY_H_
#define _NR_PDCP_ENTITY_H_

typedef struct nr_pdcp_entity_t {
  /* functions provided by the PDCP module */
  void (*recv_pdu)(struct nr_pdcp_entity_t *entity, char *buffer, int size);
  void (*recv_sdu)(struct nr_pdcp_entity_t *entity, char *buffer, int size,
                   int sdu_id);
  void (*delete)(struct nr_pdcp_entity_t *entity);
  void (*set_integrity_key)(struct nr_pdcp_entity_t *entity, char *key);

  /* callbacks provided to the PDCP module */
  void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                      char *buf, int size);
  void *deliver_sdu_data;
  void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                      char *buf, int size, int sdu_id);
  void *deliver_pdu_data;
  int tx_hfn;
  int next_nr_pdcp_tx_sn;
  int maximum_nr_pdcp_sn;

  /* security */
  int has_ciphering;
  int has_integrity;
  int ciphering_algorithm;
  int integrity_algorithm;
  unsigned char ciphering_key[16];
  unsigned char integrity_key[16];
  void *security_context;
  void (*cipher)(void *security_context,
                 unsigned char *buffer, int length,
                 int bearer, int count, int direction);
  void (*free_security)(void *security_context);
  /* security algorithms need to know uplink/downlink information
   * which is reverse for gnb and ue, so we need to know if this
   * pdcp entity is for a gnb or an ue
   */
  int is_gnb;
} nr_pdcp_entity_t;

nr_pdcp_entity_t *new_nr_pdcp_entity_srb(
    int is_gnb, int rb_id,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size),
    void *deliver_sdu_data,
    void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size, int sdu_id),
    void *deliver_pdu_data);

nr_pdcp_entity_t *new_nr_pdcp_entity_drb_am(
    int is_gnb, int rb_id,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size),
    void *deliver_sdu_data,
    void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size, int sdu_id),
    void *deliver_pdu_data,
    int sn_size,
    int t_reordering,
    int discard_timer,
    int ciphering_algorithm,
    int integrity_algorithm,
    unsigned char *ciphering_key,
    unsigned char *integrity_key);

void nr_DRB_preconfiguration(uint16_t crnti);

#endif /* _NR_PDCP_ENTITY_H_ */
