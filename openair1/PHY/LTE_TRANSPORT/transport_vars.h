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

#ifndef __TRANSPORT_VARS_H
#define __TRANSPORT_VARS_H

#include "dlsch_tbs.h"
#include "dlsch_tbs_full.h"
static const uint16_t dftsizes[34] = {12,  24,  36,  48,  60,  72,  96,  108, 120, 144, 180, 192, 216, 240, 288,  300,  324,
                                      360, 384, 432, 480, 540, 576, 600, 648, 720, 768, 864, 900, 960, 972, 1080, 1152, 1200};

static const unsigned char ue_power_offsets[25] = {14, 11, 9, 8, 7, 6, 6, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0};

extern int qam64_table[8], qam16_table[4], qpsk_table[2];

static const unsigned char cs_ri_normal[4] = {1, 4, 7, 10};
static const unsigned char cs_ri_extended[4] = {0, 3, 5, 8};
static const unsigned char cs_ack_normal[4] = {2, 3, 8, 9};
static const unsigned char cs_ack_extended[4] = {1, 2, 6, 7};

//unsigned short scfdma_amps[25] = {0,5120,3620,2956,2560,2290,2090,1935,1810,1706,1619,1544,1478,1420,1368,1322,1280,1242,1207,1175,1145,1117,1092,1068,1045,1024};

static const uint8_t wACK[5][4] = {{1, 1, 1, 1}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 1}, {0, 0, 0, 0}};
static const uint32_t bitrev_cc_dci[32] = {1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31, 0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30};

static const uint8_t pcfich_b[4][32] = {{0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1},
                                        {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0},
                                        {1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1},
                                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

extern short *ul_ref_sigs[30][2][34];
extern short *ul_ref_sigs_rx[30][2][34];
extern int16_t *d0_sss, *d5_sss;

#endif
