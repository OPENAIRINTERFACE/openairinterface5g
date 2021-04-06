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

#ifndef _NR_PDCP_INTEGRITY_NIA2_H_
#define _NR_PDCP_INTEGRITY_NIA2_H_

void *nr_pdcp_integrity_nia2_init(unsigned char *integrity_key);

void nr_pdcp_integrity_nia2_integrity(void *integrity_context,
                            unsigned char *out,
                            unsigned char *buffer, int length,
                            int bearer, int count, int direction);

void nr_pdcp_integrity_nia2_free_integrity(void *integrity_context);

#endif /* _NR_PDCP_INTEGRITY_NIA2_H_ */
