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

#ifndef PGM_LINK_H_
#define PGM_LINK_H_

/* Define prototypes only if enabled */
#if defined(ENABLE_PGM_TRANSPORT)
void bypass_tx_nack(unsigned int frame, unsigned int next_slot);

int pgm_oai_init(char *if_name);

int pgm_recv_msg(int group, uint8_t *buffer, uint32_t length,
                 unsigned int frame, unsigned int next_slot);

int pgm_link_send_msg(int group, uint8_t *data, uint32_t len);

#endif

#endif /* PGM_LINK_H_ */
