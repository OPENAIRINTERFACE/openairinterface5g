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

#ifndef __PHY_NR_REFSIG_NR_MOD_TABLE__H__
#define __PHY_NR_REFSIG_NR_MOD_TABLE__H__

#define NR_MOD_TABLE_SIZE_SHORT 686
#define NR_MOD_TABLE_BPSK_OFFSET 1
#define NR_MOD_TABLE_QPSK_OFFSET 3
#define NR_MOD_TABLE_QAM16_OFFSET 7
#define NR_MOD_TABLE_QAM64_OFFSET 23
#define NR_MOD_TABLE_QAM256_OFFSET 87

extern short nr_qpsk_mod_table[8];

extern int32_t nr_16qam_mod_table[16];
#if defined(__SSE2__)
extern __m128i nr_qpsk_byte_mod_table[2048];
#endif

extern int64_t nr_16qam_byte_mod_table[1024];

extern int64_t nr_64qam_mod_table[4096];

extern int32_t nr_256qam_mod_table[512];
#endif
