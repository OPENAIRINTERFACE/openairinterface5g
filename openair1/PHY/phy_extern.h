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

#ifndef __PHY_EXTERN_H__
#define __PHY_EXTERN_H__

#include "PHY/defs_common.h"

#include "PHY/LTE_TRANSPORT/transport_vars.h"
#include "PHY/defs_RU.h"

extern int number_of_cards;

static const short conjugate[8] __attribute__((aligned(16))) = {-1, 1, -1, 1, -1, 1, -1, 1};
static const short conjugate2[8] __attribute__((aligned(16))) = {1, -1, 1, -1, 1, -1, 1, -1};

static const short primary_synch0[144] = {
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      32767,  0,      -26120, -19785, 11971,  -30502,
    -24020, -22288, 32117,  6492,   31311,  9658,   -16384, -28378, 25100,  -21063, -7292,  -31946, 20429,  25618,  14948,  29158,
    11971,  -30502, 31311,  9658,   25100,  -21063, -16384, 28377,  -24020, 22287,  32117,  6492,   -7292,  31945,  20429,  25618,
    -26120, -19785, -16384, -28378, -16384, 28377,  -26120, -19785, -32402, 4883,   31311,  -9659,  32117,  6492,   -7292,  -31946,
    32767,  -1,     25100,  -21063, -24020, 22287,  -32402, 4883,   -32402, 4883,   -24020, 22287,  25100,  -21063, 32767,  -1,
    -7292,  -31946, 32117,  6492,   31311,  -9659,  -32402, 4883,   -26120, -19785, -16384, 28377,  -16384, -28378, -26120, -19785,
    20429,  25618,  -7292,  31945,  32117,  6492,   -24020, 22287,  -16384, 28377,  25100,  -21063, 31311,  9658,   11971,  -30502,
    14948,  29158,  20429,  25618,  -7292,  -31946, 25100,  -21063, -16384, -28378, 31311,  9658,   32117,  6492,   -24020, -22288,
    11971,  -30502, -26120, -19785, 32767,  0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0};
static const short primary_synch1[144] = {
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      32767,  0,      -31754, -8086,  -24020, -22288,
    2448,   32675,  -26120, 19784,  27073,  18458,  -16384, 28377,  25100,  21062,  -29523, 14217,  -7292,  31945,  -13477, -29868,
    -24020, -22288, 27073,  18458,  25100,  21062,  -16384, -28378, 2448,   -32676, -26120, 19784,  -29523, -14218, -7292,  31945,
    -31754, -8086,  -16384, 28377,  -16384, -28378, -31754, -8086,  31311,  -9659,  27073,  -18459, -26120, 19784,  -29523, 14217,
    32767,  -1,     25100,  21062,  2448,   -32676, 31311,  -9659,  31311,  -9659,  2448,   -32676, 25100,  21062,  32767,  0,
    -29523, 14217,  -26120, 19784,  27073,  -18459, 31311,  -9659,  -31754, -8086,  -16384, -28378, -16384, 28377,  -31754, -8086,
    -7292,  31945,  -29523, -14218, -26120, 19784,  2448,   -32676, -16384, -28378, 25100,  21062,  27073,  18458,  -24020, -22288,
    -13477, -29868, -7292,  31945,  -29523, 14217,  25100,  21062,  -16384, 28377,  27073,  18458,  -26120, 19784,  2448,   32675,
    -24020, -22288, -31754, -8086,  32767,  0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0};
static const short primary_synch2[144] = {
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      32767,  0,      -31754, 8085,   -24020, 22287,
    2448,   -32676, -26120, -19785, 27073,  -18459, -16384, -28378, 25100,  -21063, -29523, -14218, -7292,  -31946, -13477, 29867,
    -24020, 22287,  27073,  -18459, 25100,  -21063, -16384, 28377,  2448,   32675,  -26120, -19785, -29523, 14217,  -7292,  -31946,
    -31754, 8085,   -16384, -28378, -16384, 28377,  -31754, 8085,   31311,  9658,   27073,  18458,  -26120, -19785, -29523, -14218,
    32767,  0,      25100,  -21063, 2448,   32675,  31311,  9658,   31311,  9658,   2448,   32675,  25100,  -21063, 32767,  0,
    -29523, -14218, -26120, -19785, 27073,  18458,  31311,  9658,   -31754, 8085,   -16384, 28377,  -16384, -28378, -31754, 8085,
    -7292,  -31946, -29523, 14217,  -26120, -19785, 2448,   32675,  -16384, 28377,  25100,  -21063, 27073,  -18459, -24020, 22287,
    -13477, 29867,  -7292,  -31946, -29523, -14218, 25100,  -21063, -16384, -28378, 27073,  -18459, -26120, -19785, 2448,   -32676,
    -24020, 22287,  -31754, 8085,   32767,  -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0};

extern unsigned char NB_RU;


static const char NB_functions[7][20] = {
    "eNodeB_3GPP",
    "eNodeB_3GPP_BBU",
    "NGFI_RAU_IF4p5",
    "NGFI_RRU_IF5",
    "NGFI_RRU_IF4p5",
    "gNodeB_3GPP",
};
static const char NB_timing[2][20] = {"synch_to_ext_device", "synch_to_other"};
static const char ru_if_types[MAX_RU_IF_TYPES][20] = {"local RF", "IF5 RRU", "IF5 Mobipass", "IF4p5 RRU", "IF1pp RRU"};

extern int16_t unscrambling_lut[65536*16];
extern uint8_t scrambling_lut[65536*16];

static const unsigned short Nb_6_40[8][4] = {
  {36,12,4,4},
  {32,16,8,4},
  {24,4,4,4},
  {20,4,4,4},
  {16,4,4,4},
  {12,4,4,4},
  {8,4,4,4},
  {4,4,4,4}
};
static const unsigned short Nb_41_60[8][4] = {
  {48,24,12,4},
  {48,16,8,4},
  {40,20,4,4},
  {36,12,4,4},
  {32,16,8,4},
  {24,4,4,4},
  {20,4,4,4},
  {16,4,4,4}
};
static const unsigned short Nb_61_80[8][4] = {
  {72,24,12,4},
  {64,32,16,4},
  {60,20,4,4},
  {48,24,12,4},
  {48,16,8,4},
  {40,20,4,4},
  {36,12,4,4},
  {32,16,8,4}
};
static const unsigned short Nb_81_110[8][4] = {
  {96,48,24,4},
  {96,32,16,4},
  {80,40,20,4},
  {72,24,12,4},
  {64,32,16,4},
  {60,20,4,4},
  {48,24,12,4},
  {48,16,8,4}
};

static const uint8_t alpha_lut[8] = {0, 40, 50, 60, 70, 80, 90, 100};

extern uint32_t current_dlsch_cqi;

// Table 8.6.3-3 36.213
static const uint16_t beta_cqi[16] = {0, // reserved
                                      0, // reserved
                                      9, // 1.125
                                      10, // 1.250
                                      11, // 1.375
                                      13, // 1.625
                                      14, // 1.750
                                      16, // 2.000
                                      18, // 2.250
                                      20, // 2.500
                                      23, // 2.875
                                      25, // 3.125
                                      28, // 3.500
                                      32, // 4.000
                                      40, // 5.000
                                      50}; // 6.250

// Table 8.6.3-2 36.213
static const uint16_t beta_ri[16] = {10, // 1.250
                                     13, // 1.625
                                     16, // 2.000
                                     20, // 2.500
                                     25, // 3.125
                                     32, // 4.000
                                     40, // 5.000
                                     50, // 6.250
                                     64, // 8.000
                                     80, // 10.000
                                     101, // 12.625
                                     127, // 15.875
                                     160, // 20.000
                                     0, // reserved
                                     0, // reserved
                                     0}; // reserved

// Table 8.6.3-2 36.213
static const uint16_t beta_ack[16] = {16, // 2.000
                                      20, // 2.500
                                      25, // 3.125
                                      32, // 4.000
                                      40, // 5.000
                                      50, // 6.250
                                      64, // 8.000
                                      80, // 10.000
                                      101, // 12.625
                                      127, // 15.875
                                      160, // 20.000
                                      248, // 31.000
                                      400, // 50.000
                                      640, // 80.000
                                      808}; // 126.00
// TS 36.211 5.3 scrambling
static const int8_t delta_PUSCH_abs[4] = {-4, -1, 1, 4};
static const int8_t delta_PUSCH_acc[4] = {-1, 0, 1, 3};

extern uint8_t max_turbo_iterations;
extern double cpuf;

#endif /*__PHY_EXTERN_H__ */

