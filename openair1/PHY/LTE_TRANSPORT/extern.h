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

extern unsigned int dlsch_tbs25[27][25],TBStable[27][110],TBStable1C[32];
extern unsigned short lte_cqi_eff1024[16];
extern char lte_cqi_snr_dB[15];
extern short conjugate[8],conjugate2[8];
extern short minus_one[8];
extern short minus_one[8];
extern short *ul_ref_sigs[30][2][33];
extern short *ul_ref_sigs_rx[30][2][33];
extern unsigned short dftsizes[33];
extern unsigned short ref_primes[33];

extern int qam64_table[8],qam16_table[4];

extern unsigned char cs_ri_normal[4];
extern unsigned char cs_ri_extended[4];
extern unsigned char cs_ack_normal[4];
extern unsigned char cs_ack_extended[4];


extern unsigned char ue_power_offsets[25];

extern unsigned short scfdma_amps[26];

extern char dci_format_strings[15][13];

extern int16_t d0_sss[504*62],d5_sss[504*62];

extern uint8_t wACK[5][4];
extern int8_t wACK_RX[5][4];
