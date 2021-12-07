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

/*! \file nr_mac_common.c
 * \brief Common MAC/PHY functions for NR UE and gNB
 * \author  Florian Kaltenberger and Raymond Knopp
 * \date 2019
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: florian.kalteberger@eurecom.fr, raymond.knopp@eurecom.fr
 * @ingroup _mac

 */

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/nr/nr_common.h"
#include <limits.h>

#define reserved 0xffff


void reverse_n_bits(uint8_t *value, uint16_t bitlen) {
  uint16_t j;
  uint8_t i;
  for(j = bitlen - 1,i = 0; j > i; j--, i++) {
    if(((*value>>j)&1) != ((*value>>i)&1)) {
      *value ^= (1<<j);
      *value ^= (1<<i);
    }
  }
}

//38.321 Table 6.1.3.1-1
const uint32_t NR_SHORT_BSR_TABLE[NR_SHORT_BSR_TABLE_SIZE] = {
    0,    10,    14,    20,    28,     38,     53,     74,
  102,   142,   198,   276,   384,    535,    745,   1038,
 1446,  2014,  2806,  3909,  5446,   7587,  10570,  14726,
20516, 28581, 39818, 55474, 77284, 107669, 150000, 300000
};

//38.321 Table 6.1.3.1-2
const uint32_t NR_LONG_BSR_TABLE[NR_LONG_BSR_TABLE_SIZE] ={
       0,       10,       11,       12,       13,       14,       15,       16,       17,       18,       19,       20,       22,       23,        25,         26,
      28,       30,       32,       34,       36,       38,       40,       43,       46,       49,       52,       55,       59,       62,        66,         71,
      75,       80,       85,       91,       97,      103,      110,      117,      124,      132,      141,      150,      160,      170,       181,        193,
     205,      218,      233,      248,      264,      281,      299,      318,      339,      361,      384,      409,      436,      464,       494,        526,
     560,      597,      635,      677,      720,      767,      817,      870,      926,      987,     1051,     1119,     1191,     1269,      1351,       1439,
    1532,     1631,     1737,     1850,     1970,     2098,     2234,     2379,     2533,     2698,     2873,     3059,     3258,     3469,      3694,       3934,
    4189,     4461,     4751,     5059,     5387,     5737,     6109,     6506,     6928,     7378,     7857,     8367,     8910,     9488,     10104,      10760,
   11458,    12202,    12994,    13838,    14736,    15692,    16711,    17795,    18951,    20181,    21491,    22885,    24371,    25953,     27638,      29431,
   31342,    33376,    35543,    37850,    40307,    42923,    45709,    48676,    51836,    55200,    58784,    62599,    66663,    70990,     75598,      80505,
   85730,    91295,    97221,   103532,   110252,   117409,   125030,   133146,   141789,   150992,   160793,   171231,   182345,   194182,    206786,     220209,
  234503,   249725,   265935,   283197,   301579,   321155,   342002,   364202,   387842,   413018,   439827,   468377,   498780,   531156,    565634,     602350,
  641449,   683087,   727427,   774645,   824928,   878475,   935498,   996222,  1060888,  1129752,  1203085,  1281179,  1364342,  1452903,   1547213,    1647644,
 1754595,  1868488,  1989774,  2118933,  2256475,  2402946,  2558924,  2725027,  2901912,  3090279,  3290873,  3504487,  3731968,  3974215,   4232186,    4506902,
 4799451,  5110989,  5442750,  5796046,  6172275,  6572925,  6999582,  7453933,  7937777,  8453028,  9001725,  9586039, 10208280, 10870913,  11576557,   12328006,
13128233, 13980403, 14887889, 15854280, 16883401, 17979324, 19146385, 20389201, 21712690, 23122088, 24622972, 26221280, 27923336, 29735875,  31666069,   33721553,
35910462, 38241455, 40723756, 43367187, 46182206, 49179951, 52372284, 55771835, 59392055, 63247269, 67352729, 71724679, 76380419, 81338368, 162676736, 4294967295
};

// start symbols for SSB types A,B,C,D,E
uint16_t symbol_ssb_AC[8]={2,8,16,22,30,36,44,50};
uint16_t symbol_ssb_BD[64]={4,8,16,20,32,36,44,48,60,64,72,76,88,92,100,104,144,148,156,160,172,176,184,188,200,204,212,216,228,232,240,244,284,288,
                            296,300,312,316,324,328,340,344,352,356,368,372,380,384,424,428,436,440,452,456,464,468,480,484,492,496,508,512,520,524};
uint16_t symbol_ssb_E[64]={8,12,16,20,32,36,40,44,64,68,72,76,88,92,96,100,120,124,128,132,144,148,152,156,176,180,184,188,200,204,208,212,288,292,
                           296,300,312,316,320,324,344,348,352,356,368,372,376,380,400,404,408,412,424,428,432,436,456,460,464,468,480,484,488,492};

const uint8_t nr_slots_per_frame[5] = {10, 20, 40, 80, 160};

// Table 6.3.3.1-5 (38.211) NCS for preamble formats with delta_f_RA = 1.25 KHz
uint16_t NCS_unrestricted_delta_f_RA_125[16] = {0,13,15,18,22,26,32,38,46,59,76,93,119,167,279,419};
uint16_t NCS_restricted_TypeA_delta_f_RA_125[15]   = {15,18,22,26,32,38,46,55,68,82,100,128,158,202,237}; // high-speed case set Type A
uint16_t NCS_restricted_TypeB_delta_f_RA_125[13]   = {15,18,22,26,32,38,46,55,68,82,100,118,137}; // high-speed case set Type B

// Table 6.3.3.1-6 (38.211) NCS for preamble formats with delta_f_RA = 5 KHz
uint16_t NCS_unrestricted_delta_f_RA_5[16] = {0,13,26,33,38,41,49,55,64,76,93,119,139,209,279,419};
uint16_t NCS_restricted_TypeA_delta_f_RA_5[16]   = {36,57,72,81,89,94,103,112,121,132,137,152,173,195,216,237}; // high-speed case set Type A
uint16_t NCS_restricted_TypeB_delta_f_RA_5[14]   = {36,57,60,63,65,68,71,77,81,85,97,109,122,137}; // high-speed case set Type B

// Table 6.3.3.1-7 (38.211) NCS for preamble formats with delta_f_RA = 15 * 2mu KHz where mu = {0,1,2,3}
uint16_t NCS_unrestricted_delta_f_RA_15[16] = {0,2,4,6,8,10,12,13,15,17,19,23,27,34,46,69};


//	specification mapping talbe, table_38$x_$y_$z_c$a
//	- $x: specification
//	- $y: subclause-major
//	- $z: subclause-minor
//	- $a: ($a)th of column in table, start from zero
const int32_t table_38213_13_1_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, reserved}; // index 15 reserved
const int32_t table_38213_13_1_c2[16] = {24, 24, 24, 24, 24, 24, 48, 48, 48, 48, 48, 48, 96, 96, 96, reserved}; // index 15 reserved
const int32_t table_38213_13_1_c3[16] = { 2,  2,  2,  3,  3,  3,  1,  1,  2,  2,  3,  3,  1,  2,  3, reserved}; // index 15 reserved
const int32_t table_38213_13_1_c4[16] = { 0,  2,  4,  0,  2,  4, 12, 16, 12, 16, 12, 16, 38, 38, 38, reserved}; // index 15 reserved

const int32_t table_38213_13_2_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, reserved, reserved}; // index 14-15 reserved
const int32_t table_38213_13_2_c2[16] = {24, 24, 24, 24, 24, 24, 24, 24, 48, 48, 48, 48, 48, 48, reserved, reserved}; // index 14-15 reserved
const int32_t table_38213_13_2_c3[16] = { 2,  2,  2,  2,  3,  3,  3,  3,  1,  1,  2,  2,  3,  3, reserved, reserved}; // index 14-15 reserved
const int32_t table_38213_13_2_c4[16] = { 5,  6,  7,  8,  5,  6,  7,  8, 18, 20, 18, 20, 18, 20, reserved, reserved}; // index 14-15 reserved

const int32_t table_38213_13_3_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_3_c2[16] = {48, 48, 48, 48, 48, 48, 96, 96, 96, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_3_c3[16] = { 1,  1,  2,  2,  3,  3,  1,  2,  3, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_3_c4[16] = { 2,  6,  2,  6,  2,  6, 28, 28, 28, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved

const int32_t table_38213_13_4_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const int32_t table_38213_13_4_c2[16] = {24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 48, 48, 48, 48, 48, 48};
const int32_t table_38213_13_4_c3[16] = { 2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  1,  1,  1,  2,  2,  2};
const int32_t table_38213_13_4_c4[16] = { 0,  1,  2,  3,  4,  0,  1,  2,  3,  4, 12, 14, 16, 12, 14, 16};

const int32_t table_38213_13_5_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_5_c2[16] = {48, 48, 48, 96, 96, 96, 96, 96, 96, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_5_c3[16] = { 1,  2,  3,  1,  1,  2,  2,  3,  3, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_5_c4[16] = { 4,  4,  4,  0, 56,  0, 56,  0, 56, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved

const int32_t table_38213_13_6_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved
const int32_t table_38213_13_6_c2[16] = {24, 24, 24, 24, 48, 48, 48, 48, 48, 48, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved
const int32_t table_38213_13_6_c3[16] = { 2,  2,  3,  3,  1,  1,  2,  2,  3,  3, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved
const int32_t table_38213_13_6_c4[16] = { 0,  4,  0,  4,  0, 28,  0, 28,  0, 28, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved

const int32_t table_38213_13_7_c1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, reserved, reserved, reserved, reserved}; // index 12-15 reserved
const int32_t table_38213_13_7_c2[16] = {48, 48, 48, 48, 48, 48, 96, 96, 48, 48, 96, 96, reserved, reserved, reserved, reserved}; // index 12-15 reserved
const int32_t table_38213_13_7_c3[16] = { 1,  1,  2,  2,  3,  3,  1,  2,  1,  1,  1,  1, reserved, reserved, reserved, reserved}; // index 12-15 reserved
const int32_t table_38213_13_7_c4[16] = { 0,  8,  0,  8,  0,  8, 28, 28,-41, 49,-41, 97, reserved, reserved, reserved, reserved}; // index 12-15 reserved, condition A as default

const int32_t table_38213_13_8_c1[16] = { 1,  1,  1,  1,  3,  3,  3,  3, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved
const int32_t table_38213_13_8_c2[16] = {24, 24, 48, 48, 24, 24, 48, 48, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved
const int32_t table_38213_13_8_c3[16] = { 2,  2,  1,  2,  2,  2,  2,  2, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved
const int32_t table_38213_13_8_c4[16] = { 0,  4, 14, 14,-20, 24,-20, 48, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved, condition A as default

const int32_t table_38213_13_9_c1[16] = {1, 1, 1, 1, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved
const int32_t table_38213_13_9_c2[16] = {96, 96, 96, 96, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved
const int32_t table_38213_13_9_c3[16] = { 1,  1,  2,  2, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved
const int32_t table_38213_13_9_c4[16] = { 0, 16,  0, 16, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved

const int32_t table_38213_13_10_c1[16] = {1, 1, 1, 1, 2, 2, 2, 2, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 08-15 reserved
const int32_t table_38213_13_10_c2[16] = {48, 48, 48, 48, 24, 24, 48, 48, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 08-15 reserved
const int32_t table_38213_13_10_c3[16] = { 1,  1,  2,  2,  1,  1,  1,  1, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 08-15 reserved
const int32_t table_38213_13_10_c4[16] = { 0,  8,  0,  8,-41, 25,-41, 49, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 08-15 reserved, condition A as default

const float   table_38213_13_11_c1[16] = { 0,  0,  2,  2,  5,  5,  7,  7,  0,  5,  0,  0,  2,  2,  5,  5};	//	O
const int32_t table_38213_13_11_c2[16] = { 1,  2,  1,  2,  1,  2,  1,  2,  1,  1,  1,  1,  1,  1,  1,  1};
const float   table_38213_13_11_c3[16] = { 1, 0.5f, 1, 0.5f, 1, 0.5f, 1, 0.5f,  1,  1,  1,  1,  1,  1,  1,  1};	//	M
const int32_t table_38213_13_11_c4[16] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  1,  2,  1,  2};	// i is even as default

const float   table_38213_13_12_c1[16] = { 0, 0, 2.5f, 2.5f, 5, 5, 0, 2.5f, 5, 7.5f, 7.5f, 7.5f, 0, 5, reserved, reserved}; // O, index 14-15 reserved
const int32_t table_38213_13_12_c2[16] = { 1,  2,  1,  2,  1,  2,  2,  2,  2,  1,  2,  2,  1,  1,  reserved,  reserved}; // index 14-15 reserved
const float   table_38213_13_12_c3[16] = { 1, 0.5f, 1, 0.5f, 1, 0.5f, 0.5f, 0.5f, 0.5f, 1, 0.5f, 0.5f, 1, 1,  reserved,  reserved}; // M, index 14-15 reserved

const int32_t table_38213_10_1_1_c2[5] = { 0, 0, 4, 2, 1 };

// for PDSCH from TS 38.214 subclause 5.1.2.1.1
const uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[16][4]={
    {1,0,2,12},   // row index 1
    {1,0,2,10},   // row index 2
    {1,0,2,9},    // row index 3
    {1,0,2,7},    // row index 4
    {1,0,2,5},    // row index 5
    {0,0,9,4},    // row index 6
    {0,0,4,4},    // row index 7
    {0,0,5,7},    // row index 8
    {0,0,5,2},    // row index 9
    {0,0,9,2},    // row index 10
    {0,0,12,2},   // row index 11
    {1,0,1,13},   // row index 12
    {1,0,1,6},    // row index 13
    {1,0,2,4},    // row index 14
    {0,0,4,7},    // row index 15
    {0,0,8,4}     // row index 16
};
const uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[16][4]={
    {1,0,3,11},   // row index 1
    {1,0,3,9},    // row index 2
    {1,0,3,8},    // row index 3
    {1,0,3,6},    // row index 4
    {1,0,3,4},    // row index 5
    {0,0,10,4},   // row index 6
    {0,0,6,4},    // row index 7
    {0,0,5,7},    // row index 8
    {0,0,5,2},    // row index 9
    {0,0,9,2},    // row index 10
    {0,0,12,2},   // row index 11
    {1,0,1,13},   // row index 12
    {1,0,1,6},    // row index 13
    {1,0,2,4},    // row index 14
    {0,0,4,7},    // row index 15
    {0,0,8,4}     // row index 16
};
const uint8_t table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos2[16][4]={
    {1,0,2,6},    // row index 1
    {1,0,2,10},   // row index 2
    {1,0,2,9},    // row index 3
    {1,0,2,7},    // row index 4
    {1,0,2,5},    // row index 5
    {0,0,6,4},    // row index 6
    {0,0,4,4},    // row index 7
    {0,0,5,6},    // row index 8
    {0,0,5,2},    // row index 9
    {0,0,9,2},    // row index 10
    {0,0,10,2},   // row index 11
    {1,0,1,11},   // row index 12
    {1,0,1,6},    // row index 13
    {1,0,2,4},    // row index 14
    {0,0,4,6},    // row index 15
    {0,0,8,4}     // row index 16
};
const uint8_t table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos3[16][4]={
    {1,0,3,5},    // row index 1
    {1,0,3,9},    // row index 2
    {1,0,3,8},    // row index 3
    {1,0,3,6},    // row index 4
    {1,0,3,4},    // row index 5
    {0,0,8,2},    // row index 6
    {0,0,6,4},    // row index 7
    {0,0,5,6},    // row index 8
    {0,0,5,2},    // row index 9
    {0,0,9,2},    // row index 10
    {0,0,10,2},   // row index 11
    {1,0,1,11},   // row index 12
    {1,0,1,6},    // row index 13
    {1,0,2,4},    // row index 14
    {0,0,4,6},    // row index 15
    {0,0,8,4}     // row index 16
};
const uint8_t table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos2[16][4]={
    {0,0,2,2},    // row index 1
    {0,0,4,2},    // row index 2
    {0,0,6,2},    // row index 3
    {0,0,8,2},    // row index 4
    {0,0,10,2},   // row index 5
    {0,1,2,2},    // row index 6
    {0,1,4,2},    // row index 7
    {0,0,2,4},    // row index 8
    {0,0,4,4},    // row index 9
    {0,0,6,4},    // row index 10
    {0,0,8,4},    // row index 11
    {0,0,10,4},   // row index 12
    {0,0,2,7},    // row index 13
    {1,0,2,12},   // row index 14
    {0,1,2,4},    // row index 15
    {0,0,0,0}     // row index 16
};
const uint8_t table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos3[16][4]={
    {0,0,2,2},    // row index 1
    {0,0,4,2},    // row index 2
    {0,0,6,2},    // row index 3
    {0,0,8,2},    // row index 4
    {0,0,10,2},   // row index 5
    {0,1,2,2},    // row index 6
    {0,1,4,2},    // row index 7
    {0,0,2,4},    // row index 8
    {0,0,4,4},    // row index 9
    {0,0,6,4},    // row index 10
    {0,0,8,4},    // row index 11
    {0,0,10,4},   // row index 12
    {0,0,2,7},    // row index 13
    {1,0,3,11},   // row index 14
    {0,1,2,4},    // row index 15
    {0,0,0,0}     // row index 16
};
const uint8_t table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos2[16][4]={
    {0,0,2,2},  // row index 1
    {0,0,4,2},  // row index 2
    {0,0,6,2},  // row index 3
    {0,0,8,2},  // row index 4
    {0,0,10,2}, // row index 5
    {0,0,0,0},  // row index 6
    {0,0,0,0},  // row index 7
    {0,0,2,4},  // row index 8
    {0,0,4,4},  // row index 9
    {0,0,6,4},  // row index 10
    {0,0,8,4},  // row index 11
    {0,0,10,4}, // row index 12
    {0,0,2,7},  // row index 13
    {1,0,2,12},  // row index 14
    {1,0,0,6},  // row index 15
    {1,0,2,6}   // row index 16
};
const uint8_t table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos3[16][4]={
    {0,0,2,2},  // row index 1
    {0,0,4,2},  // row index 2
    {0,0,6,2},  // row index 3
    {0,0,8,2},  // row index 4
    {0,0,10,2}, // row index 5
    {0,0,0,0},  // row index 6
    {0,0,0,0},  // row index 7
    {0,0,2,4},  // row index 8
    {0,0,4,4},  // row index 9
    {0,0,6,4},  // row index 10
    {0,0,8,4},  // row index 11
    {0,0,10,4}, // row index 12
    {0,0,2,7},  // row index 13
    {1,0,3,11},  // row index 14
    {1,0,0,6},  // row index 15
    {1,0,2,6}   // row index 16
};

void get_info_from_tda_tables(int default_abc,
                              int tda,
                              int dmrs_TypeA_Position,
                              int normal_CP,
                              int *startSymbolIndex,
                              int *nrOfSymbols) {
  int k0 = 0;
  int is_mapping_typeA = 1;
  switch(default_abc){
    case 1:
      if (normal_CP){
        if (dmrs_TypeA_Position){
          is_mapping_typeA = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[tda][0];
          k0 = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[tda][1];
          *startSymbolIndex = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[tda][2];
          *nrOfSymbols = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[tda][3];
        }
        else{
          is_mapping_typeA = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[tda][0];
          k0 = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[tda][1];
          *startSymbolIndex = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[tda][2];
          *nrOfSymbols = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[tda][3];
        }
      }
      else{
        if (dmrs_TypeA_Position){
          is_mapping_typeA = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos3[tda][0];
          k0 = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos3[tda][1];
          *startSymbolIndex = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos3[tda][2];
          *nrOfSymbols = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos3[tda][3];
        }
        else{
          is_mapping_typeA = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos2[tda][0];
          k0 = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos2[tda][1];
          *startSymbolIndex = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos2[tda][2];
          *nrOfSymbols = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP_dmrs_typeA_pos2[tda][3];
        }
      }
      break;
    case 2:
      if (dmrs_TypeA_Position){
        is_mapping_typeA = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos3[tda][0];
        k0 = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos3[tda][1];
        *startSymbolIndex = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos3[tda][2];
        *nrOfSymbols = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos3[tda][3];
      }
      else{
        is_mapping_typeA = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos2[tda][0];
        k0 = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos2[tda][1];
        *startSymbolIndex = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos2[tda][2];
        *nrOfSymbols = table_5_1_2_1_1_4_time_dom_res_alloc_B_dmrs_typeA_pos2[tda][3];
      }
      break;
    case 3:
      if (dmrs_TypeA_Position){
        is_mapping_typeA = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos3[tda][0];
        k0 = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos3[tda][1];
        *startSymbolIndex = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos3[tda][2];
        *nrOfSymbols = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos3[tda][3];
      }
      else{
        is_mapping_typeA = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos2[tda][0];
        k0 = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos2[tda][1];
        *startSymbolIndex = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos2[tda][2];
        *nrOfSymbols = table_5_1_2_1_1_5_time_dom_res_alloc_C_dmrs_typeA_pos2[tda][3];
      }
      break;
    default:
     AssertFatal(1==0,"Invalid default time domaing allocation type\n");
  }
  AssertFatal(k0==0,"Only k0 = 0 is supported\n");
  AssertFatal(is_mapping_typeA==1,"Only mapping type A is currently supported\n");
}

const char *prachfmt[]={"0","1","2","3", "A1","A2","A3","B1","B4","C0","C2","A1/B1","A2/B2","A3/B3"};

uint16_t get_NCS(uint8_t index, uint16_t format0, uint8_t restricted_set_config) {

  LOG_D(MAC,"get_NCS: indx %d,format0 %d, restriced_set_config %d\n",
	index,format0,restricted_set_config);

  if (format0 < 3) {
    switch(restricted_set_config){
      case 0:
        return(NCS_unrestricted_delta_f_RA_125[index]);
      case 1:
        return(NCS_restricted_TypeA_delta_f_RA_125[index]);
      case 2:
        return(NCS_restricted_TypeB_delta_f_RA_125[index]);
    default:
      AssertFatal(1==0,"Invalid restricted set config value %d",restricted_set_config);
    }
  }
  else {
    if (format0 == 3) {
      switch(restricted_set_config){
        case 0:
          return(NCS_unrestricted_delta_f_RA_5[index]);
        case 1:
          return(NCS_restricted_TypeA_delta_f_RA_5[index]);
        case 2:
          return(NCS_restricted_TypeB_delta_f_RA_5[index]);
      default:
        AssertFatal(1==0,"Invalid restricted set config value %d",restricted_set_config);
      }
    }
    else
       return(NCS_unrestricted_delta_f_RA_15[index]);
  }
}

//38.211 Table 6.3.3.2-1
int16_t table_6_3_3_2_1[16][5] = {
//Length_RA, delta_f_RA_PRACH, delta_f_PUSCH, N_RA_RB, kbar
{ 839,        1.25,             15,            6,      7},
{ 839,        1.25,             30,            3,      1},
{ 839,        1.25,             60,            2,    133},
{ 839,           5,             15,           24,     12},
{ 839,           5,             30,           12,     10},
{ 839,           5,             60,            6,      7},
{ 139,          15,             15,           12,      2},
{ 139,          15,             30,            6,      2},
{ 139,          15,             60,            3,      2},
{ 139,          30,             15,           24,      2},
{ 139,          30,             30,           12,      2},
{ 139,          30,             60,            6,      2},
{ 139,          60,             60,           12,      2},
{ 139,          60,            120,            6,      2},
{ 139,         120,             60,           24,      2},
{ 139,         120,            120,           12,      2}
};

/* Function to get number of RBs required for prach occasion based on
 * 38.211 Table 6.3.3.2-1 */
int16_t get_N_RA_RB (int delta_f_RA_PRACH,int delta_f_PUSCH) {
	
	int8_t index = 0;
	switch(delta_f_RA_PRACH) {
			case 0 : index = 6;
		          if (delta_f_PUSCH == 0)
			          index += 0;
		          else if(delta_f_PUSCH == 1)
			          index += 1;
		          else
			          index += 2;
							break;

		case 1 : index = 9;
		         if (delta_f_PUSCH == 0)
			         index += 0;
		         else if(delta_f_PUSCH == 1)
		           index += 1;
		         else
			          index += 2;
	           break;

		case 2 : index = 11;
		         if (delta_f_PUSCH == 2)
			         index += 0;
		         else
			         index += 1;
		         break;		
		
		case 3: index = 13;
		          if (delta_f_PUSCH == 2)
			          index += 0;
		          else
			          index += 1;
		          break;

		default : index = 10;/*30khz prach scs and 30khz pusch scs*/
				
	}
  
	return table_6_3_3_2_1[index][3];
}	
// Table 6.3.3.2-2: Random access configurations for FR1 and paired spectrum/supplementary uplink
// the column 5, (SFN_nbr is a bitmap where we set bit to '1' in the position of the subframe where the RACH can be sent.
// E.g. in row 4, and column 5 we have set value 512 ('1000000000') which means RACH can be sent at subframe 9.
// E.g. in row 20 and column 5 we have set value 66  ('0001000010') which means RACH can be sent at subframe 1 or 6
int64_t table_6_3_3_2_2_prachConfig_Index [256][9] = {
//format,   format,       x,          y,        SFN_nbr,   star_symb,   slots_sfn,    occ_slot,  duration
{0,          -1,          16,         1,          2,          0,         1,         1,          0},          // (subframe number)           1
{0,          -1,          16,         1,          16,         0,         1,         1,          0},          // (subframe number)           4
{0,          -1,          16,         1,          128,        0,         1,         1,          0},          // (subframe number)           7
{0,          -1,          16,         1,          512,        0,         1,         1,          0},          // (subframe number)           9
{0,          -1,          8,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{0,          -1,          8,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{0,          -1,          8,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{0,          -1,          8,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{0,          -1,          4,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{0,          -1,          4,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{0,          -1,          4,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{0,          -1,          4,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{0,          -1,          2,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{0,          -1,          2,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{0,          -1,          2,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{0,          -1,          2,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{0,          -1,          1,          0,          2,          0,         1,         1,          0},          // (subframe number)           1
{0,          -1,          1,          0,          16,         0,         1,         1,          0},          // (subframe number)           4
{0,          -1,          1,          0,          128,        0,         1,         1,          0},          // (subframe number)           7
{0,          -1,          1,          0,          66,         0,         1,         1,          0},          // (subframe number)           1,6
{0,          -1,          1,          0,          132,        0,         1,         1,          0},          // (subframe number)           2,7
{0,          -1,          1,          0,          264,        0,         1,         1,          0},          // (subframe number)           3,8
{0,          -1,          1,          0,          146,        0,         1,         1,          0},          // (subframe number)           1,4,7
{0,          -1,          1,          0,          292,        0,         1,         1,          0},          // (subframe number)           2,5,8
{0,          -1,          1,          0,          584,        0,         1,         1,          0},          // (subframe number)           3, 6, 9
{0,          -1,          1,          0,          341,        0,         1,         1,          0},          // (subframe number)           0,2,4,6,8
{0,          -1,          1,          0,          682,        0,         1,         1,          0},          // (subframe number)           1,3,5,7,9
{0,          -1,          1,          0,          1023,       0,         1,         1,          0},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{1,          -1,          16,         1,          2,          0,         1,         1,          0},          // (subframe number)           1
{1,          -1,          16,         1,          16,         0,         1,         1,          0},          // (subframe number)           4
{1,          -1,          16,         1,          128,        0,         1,         1,          0},          // (subframe number)           7
{1,          -1,          16,         1,          512,        0,         1,         1,          0},          // (subframe number)           9
{1,          -1,          8,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{1,          -1,          8,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{1,          -1,          8,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{1,          -1,          8,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{1,          -1,          4,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{1,          -1,          4,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{1,          -1,          4,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{1,          -1,          4,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{1,          -1,          2,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{1,          -1,          2,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{1,          -1,          2,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{1,          -1,          2,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{1,          -1,          1,          0,          2,          0,         1,         1,          0},          // (subframe number)           1
{1,          -1,          1,          0,          16,         0,         1,         1,          0},          // (subframe number)           4
{1,          -1,          1,          0,          128,        0,         1,         1,          0},          // (subframe number)           7
{1,          -1,          1,          0,          66,         0,         1,         1,          0},          // (subframe number)           1,6
{1,          -1,          1,          0,          132,        0,         1,         1,          0},          // (subframe number)           2,7
{1,          -1,          1,          0,          264,        0,         1,         1,          0},          // (subframe number)           3,8
{1,          -1,          1,          0,          146,        0,         1,         1,          0},          // (subframe number)           1,4,7
{1,          -1,          1,          0,          292,        0,         1,         1,          0},          // (subframe number)           2,5,8
{1,          -1,          1,          0,          584,        0,         1,         1,          0},          // (subframe number)           3,6,9
{2,          -1,          16,         1,          2,          0,         1,         1,          0},          // (subframe number)           1
{2,          -1,          8,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{2,          -1,          4,          0,          2,          0,         1,         1,          0},          // (subframe number)           1
{2,          -1,          2,          0,          2,          0,         1,         1,          0},          // (subframe number)           1
{2,          -1,          2,          0,          32,         0,         1,         1,          0},          // (subframe number)           5
{2,          -1,          1,          0,          2,          0,         1,         1,          0},          // (subframe number)           1
{2,          -1,          1,          0,          32,         0,         1,         1,          0},          // (subframe number)           5
{3,          -1,          16,         1,          2,          0,         1,         1,          0},          // (subframe number)           1
{3,          -1,          16,         1,          16,         0,         1,         1,          0},          // (subframe number)           4
{3,          -1,          16,         1,          128,        0,         1,         1,          0},          // (subframe number)           7
{3,          -1,          16,         1,          512,        0,         1,         1,          0},          // (subframe number)           9
{3,          -1,          8,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{3,          -1,          8,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{3,          -1,          8,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{3,          -1,          4,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{3,          -1,          4,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{3,          -1,          4,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{3,          -1,          4,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{3,          -1,          2,          1,          2,          0,         1,         1,          0},          // (subframe number)           1
{3,          -1,          2,          1,          16,         0,         1,         1,          0},          // (subframe number)           4
{3,          -1,          2,          1,          128,        0,         1,         1,          0},          // (subframe number)           7
{3,          -1,          2,          1,          512,        0,         1,         1,          0},          // (subframe number)           9
{3,          -1,          1,          0,          2,          0,         1,         1,          0},          // (subframe number)           1
{3,          -1,          1,          0,          16,         0,         1,         1,          0},          // (subframe number)           4
{3,          -1,          1,          0,          128,        0,         1,         1,          0},          // (subframe number)           7
{3,          -1,          1,          0,          66,         0,         1,         1,          0},          // (subframe number)           1,6
{3,          -1,          1,          0,          132,        0,         1,         1,          0},          // (subframe number)           2,7
{3,          -1,          1,          0,          264,        0,         1,         1,          0},          // (subframe number)           3,8
{3,          -1,          1,          0,          146,        0,         1,         1,          0},          // (subframe number)           1,4,7
{3,          -1,          1,          0,          292,        0,         1,         1,          0},          // (subframe number)           2,5,8
{3,          -1,          1,          0,          584,        0,         1,         1,          0},          // (subframe number)           3, 6, 9
{3,          -1,          1,          0,          341,        0,         1,         1,          0},          // (subframe number)           0,2,4,6,8
{3,          -1,          1,          0,          682,        0,         1,         1,          0},          // (subframe number)           1,3,5,7,9
{3,          -1,          1,          0,          1023,       0,         1,         1,          0},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa1,       -1,          16,         0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          16,         1,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          8,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          8,          1,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          4,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          4,          1,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          4,          0,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          2,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          2,          0,          2,          0,          2,          6,          2},          // (subframe number)           1
{0xa1,       -1,          2,          0,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          2,          0,          128,        0,          2,          6,          2},          // (subframe number)           7
{0xa1,       -1,          1,          0,          16,         0,          1,          6,          2},          // (subframe number)           4
{0xa1,       -1,          1,          0,          66,         0,          1,          6,          2},          // (subframe number)           1,6
{0xa1,       -1,          1,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          1,          0,          2,          0,          2,          6,          2},          // (subframe number)           1
{0xa1,       -1,          1,          0,          128,        0,          2,          6,          2},          // (subframe number)           7
{0xa1,       -1,          1,          0,          132,        0,          2,          6,          2},          // (subframe number)           2,7
{0xa1,       -1,          1,          0,          146,        0,          2,          6,          2},          // (subframe number)           1,4,7
{0xa1,       -1,          1,          0,          341,        0,          2,          6,          2},          // (subframe number)           0,2,4,6,8
{0xa1,       -1,          1,          0,          1023,       0,          2,          6,          2},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa1,       -1,          1,          0,          682,        0,          2,          6,          2},          // (subframe number)           1,3,5,7,9
{0xa1,       0xb1,        2,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xa1,       0xb1,        2,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xa1,       0xb1,        1,          0,          16,         0,          1,          7,          2},          // (subframe number)           4
{0xa1,       0xb1,        1,          0,          66,         0,          1,          7,          2},          // (subframe number)           1,6
{0xa1,       0xb1,        1,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xa1,       0xb1,        1,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xa1,       0xb1,        1,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xa1,       0xb1,        1,          0,          146,        0,          2,          7,          2},          // (subframe number)           1,4,7
{0xa1,       0xb1,        1,          0,          341,        0,          2,          7,          2},          // (subframe number)           0,2,4,6,8
{0xa2,       -1,          16,         1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          16,         1,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          8,          1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          8,          1,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          4,          0,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          4,          0,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          2,          1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          2,          0,          2,          0,          2,          3,          4},          // (subframe number)           1
{0xa2,       -1,          2,          0,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          2,          0,          128,        0,          2,          3,          4},          // (subframe number)           7
{0xa2,       -1,          1,          0,          16,         0,          1,          3,          4},          // (subframe number)           4
{0xa2,       -1,          1,          0,          66,         0,          1,          3,          4},          // (subframe number)           1,6
{0xa2,       -1,          1,          0,          528,        0,          1,          3,          4},          // (subframe number)           4,9
{0xa2,       -1,          1,          0,          2,          0,          2,          3,          4},          // (subframe number)           1
{0xa2,       -1,          1,          0,          128,        0,          2,          3,          4},          // (subframe number)           7
{0xa2,       -1,          1,          0,          132,        0,          2,          3,          4},          // (subframe number)           2,7
{0xa2,       -1,          1,          0,          146,        0,          2,          3,          4},          // (subframe number)           1,4,7
{0xa2,       -1,          1,          0,          341,        0,          2,          3,          4},          // (subframe number)           0,2,4,6,8
{0xa2,       -1,          1,          0,          1023,       0,          2,          3,          4},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa2,       -1,          1,          0,          682,        0,          2,          3,          4},          // (subframe number)           1,3,5,7,9
{0xa2,       0xb2,        2,          1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       0xb2,        2,          0,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       0xb2,        1,          0,          16,         0,          1,          3,          4},          // (subframe number)           4
{0xa2,       0xb2,        1,          0,          66,         0,          1,          3,          4},          // (subframe number)           1,6
{0xa2,       0xb2,        1,          0,          528,        0,          1,          3,          4},          // (subframe number)           4,9
{0xa2,       0xb2,        1,          0,          2,          0,          2,          3,          4},          // (subframe number)           1
{0xa2,       0xb2,        1,          0,          128,        0,          2,          3,          4},          // (subframe number)           7
{0xa2,       0xb2,        1,          0,          146,        0,          2,          3,          4},          // (subframe number)           1,4,7
{0xa2,       0xb2,        1,          0,          341,        0,          2,          3,          4},          // (subframe number)           0,2,4,6,8
{0xa2,       0xb2,        1,          0,          1023,       0,          2,          3,          4},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa3,       -1,          16,         1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          16,         1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          8,          1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          8,          1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          4,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          4,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          2,          1,          580,        0,          2,          2,          6},          // (subframe number)           2,6,9
{0xa3,       -1,          2,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xa3,       -1,          2,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          2,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xa3,       -1,          1,          0,          16,         0,          1,          2,          6},          // (subframe number)           4
{0xa3,       -1,          1,          0,          66,         0,          1,          2,          6},          // (subframe number)           1,6
{0xa3,       -1,          1,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          1,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xa3,       -1,          1,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xa3,       -1,          1,          0,          132,        0,          2,          2,          6},          // (subframe number)           2,7
{0xa3,       -1,          1,          0,          146,        0,          2,          2,          6},          // (subframe number)           1,4,7
{0xa3,       -1,          1,          0,          341,        0,          2,          2,          6},          // (subframe number)           0,2,4,6,8
{0xa3,       -1,          1,          0,          1023,       0,          2,          2,          6},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa3,       -1,          1,          0,          682,        0,          2,          2,          6},          // (subframe number)           1,3,5,7,9
{0xa3,       0xb3,        2,          1,          580,        0,          2,          2,          6},          // (subframe number)           2,6,9
{0xa3,       0xb3,        2,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       0xb3,        1,          0,          16,         0,          1,          2,          6},          // (subframe number)           4
{0xa3,       0xb3,        1,          0,          66,         0,          1,          2,          6},          // (subframe number)           1,6
{0xa3,       0xb3,        1,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       0xb3,        1,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xa3,       0xb3,        1,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xa3,       0xb3,        1,          0,          146,        0,          2,          2,          6},          // (subframe number)           1,4,7
{0xa3,       0xb3,        1,          0,          341,        0,          2,          2,          6},          // (subframe number)           0,2,4,6,8
{0xa3,       0xb3,        1,          0,          1023,       0,          2,          2,          6},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xb1,       -1,          16,         0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          16,         1,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          8,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          8,          1,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          4,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          4,          1,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          4,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          2,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          2,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xb1,       -1,          2,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          2,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xb1,       -1,          1,          0,          16,         0,          1,          7,          2},          // (subframe number)           4
{0xb1,       -1,          1,          0,          66,         0,          1,          7,          2},          // (subframe number)           1,6
{0xb1,       -1,          1,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          1,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xb1,       -1,          1,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xb1,       -1,          1,          0,          132,        0,          2,          7,          2},          // (subframe number)           2,7
{0xb1,       -1,          1,          0,          146,        0,          2,          7,          2},          // (subframe number)           1,4,7
{0xb1,       -1,          1,          0,          341,        0,          2,          7,          2},          // (subframe number)           0,2,4,6,8
{0xb1,       -1,          1,          0,          1023,       0,          2,          7,          2},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xb1,       -1,          1,          0,          682,        0,          2,          7,          2},          // (subframe number)           1,3,5,7,9
{0xb4,       -1,          16,         0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          16,         1,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          8,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          8,          1,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          4,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          4,          0,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          4,          1,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          2,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          2,          0,          2,          0,          2,          1,          12},         // (subframe number)           1
{0xb4,       -1,          2,          0,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          2,          0,          128,        0,          2,          1,          12},         // (subframe number)           7
{0xb4,       -1,          1,          0,          2,          0,          2,          1,          12},         // (subframe number)           1
{0xb4,       -1,          1,          0,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          1,          0,          128,        0,          2,          1,          12},         // (subframe number)           7
{0xb4,       -1,          1,          0,          66,         0,          2,          1,          12},         // (subframe number)           1,6
{0xb4,       -1,          1,          0,          132,        0,          2,          1,          12},         // (subframe number)           2,7
{0xb4,       -1,          1,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          1,          0,          146,        0,          2,          1,          12},         // (subframe number)           1,4,7
{0xb4,       -1,          1,          0,          341,        0,          2,          1,          12},         // (subframe number)           0,2,4,6,8
{0xb4,       -1,          1,          0,          1023,       0,          2,          1,          12},         // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xb4,       -1,          1,          0,          682,        0,          2,          1,          12},         // (subframe number)           1,3,5,7,9
{0xc0,       -1,          8,          1,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xc0,       -1,          4,          1,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xc0,       -1,          4,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xc0,       -1,          2,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xc0,       -1,          2,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xc0,       -1,          2,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xc0,       -1,          2,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xc0,       -1,          1,          0,          16,         0,          1,          7,          2},          // (subframe number)           4
{0xc0,       -1,          1,          0,          66,         0,          1,          7,          2},          // (subframe number)           1,6
{0xc0,       -1,          1,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xc0,       -1,          1,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xc0,       -1,          1,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xc0,       -1,          1,          0,          132,        0,          2,          7,          2},          // (subframe number)           2,7
{0xc0,       -1,          1,          0,          146,        0,          2,          7,          2},          // (subframe number)           1,4,7
{0xc0,       -1,          1,          0,          341,        0,          2,          7,          2},          // (subframe number)           0,2,4,6,8
{0xc0,       -1,          1,          0,          1023,       0,          2,          7,          2},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xc0,       -1,          1,          0,          682,        0,          2,          7,          2},          // (subframe number)           1,3,5,7,9
{0xc2,       -1,          16,         1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          16,         1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          8,          1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          8,          1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          4,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          4,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          2,          1,          580,        0,          2,          2,          6},          // (subframe number)           2,6,9
{0xc2,       -1,          2,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xc2,       -1,          2,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          2,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xc2,       -1,          1,          0,          16,         0,          1,          2,          6},          // (subframe number)           4
{0xc2,       -1,          1,          0,          66,         0,          1,          2,          6},          // (subframe number)           1,6
{0xc2,       -1,          1,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          1,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xc2,       -1,          1,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xc2,       -1,          1,          0,          132,        0,          2,          2,          6},          // (subframe number)           2,7
{0xc2,       -1,          1,          0,          146,        0,          2,          2,          6},          // (subframe number)           1,4,7
{0xc2,       -1,          1,          0,          341,        0,          2,          2,          6},          // (subframe number)           0,2,4,6,8
{0xc2,       -1,          1,          0,          1023,       0,          2,          2,          6},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xc2,       -1,          1,          0,          682,        0,          2,          2,          6}                    // (subframe number)           1,3,5,7,9
};
// Table 6.3.3.2-3: Random access configurations for FR1 and unpaired spectrum
int64_t table_6_3_3_2_3_prachConfig_Index [256][9] = {
//format,     format,      x,         y,     SFN_nbr,   star_symb,   slots_sfn,  occ_slot,  duration
{0,            -1,         16,        1,         512,         0,        1,        1,         0},         // (subrame number 9)
{0,            -1,         8,         1,         512,         0,        1,        1,         0},         // (subrame number 9)
{0,            -1,         4,         1,         512,         0,        1,        1,         0},         // (subrame number 9)
{0,            -1,         2,         0,         512,         0,        1,        1,         0},         // (subrame number 9)
{0,            -1,         2,         1,         512,         0,        1,        1,         0},         // (subrame number 9)
{0,            -1,         2,         0,         16,          0,        1,        1,         0},         // (subrame number 4)
{0,            -1,         2,         1,         16,          0,        1,        1,         0},         // (subrame number 4)
{0,            -1,         1,         0,         512,         0,        1,        1,         0},         // (subrame number 9)
{0,            -1,         1,         0,         256,         0,        1,        1,         0},         // (subrame number 8)
{0,            -1,         1,         0,         128,         0,        1,        1,         0},         // (subrame number 7)
{0,            -1,         1,         0,         64,          0,        1,        1,         0},         // (subrame number 6)
{0,            -1,         1,         0,         32,          0,        1,        1,         0},         // (subrame number 5)
{0,            -1,         1,         0,         16,          0,        1,        1,         0},         // (subrame number 4)
{0,            -1,         1,         0,         8,           0,        1,        1,         0},         // (subrame number 3)
{0,            -1,         1,         0,         4,           0,        1,        1,         0},         // (subrame number 2)
{0,            -1,         1,         0,         66,          0,        1,        1,         0},         // (subrame number 1,6)
{0,            -1,         1,         0,         66,          7,        1,        1,         0},         // (subrame number 1,6)
{0,            -1,         1,         0,         528,         0,        1,        1,         0},         // (subrame number 4,9)
{0,            -1,         1,         0,         264,         0,        1,        1,         0},         // (subrame number 3,8)
{0,            -1,         1,         0,         132,         0,        1,        1,         0},         // (subrame number 2,7)
{0,            -1,         1,         0,         768,         0,        1,        1,         0},         // (subrame number 8,9)
{0,            -1,         1,         0,         784,         0,        1,        1,         0},         // (subrame number 4,8,9)
{0,            -1,         1,         0,         536,         0,        1,        1,         0},         // (subrame number 3,4,9)
{0,            -1,         1,         0,         896,         0,        1,        1,         0},         // (subrame number 7,8,9)
{0,            -1,         1,         0,         792,         0,        1,        1,         0},         // (subrame number 3,4,8,9)
{0,            -1,         1,         0,         960,         0,        1,        1,         0},         // (subrame number 6,7,8,9)
{0,            -1,         1,         0,         594,         0,        1,        1,         0},         // (subrame number 1,4,6,9)
{0,            -1,         1,         0,         682,         0,        1,        1,         0},         // (subrame number 1,3,5,7,9)
{1,            -1,         16,        1,         128,         0,        1,        1,         0},         // (subrame number 7)
{1,            -1,         8,         1,         128,         0,        1,        1,         0},         // (subrame number 7)
{1,            -1,         4,         1,         128,         0,        1,        1,         0},         // (subrame number 7)
{1,            -1,         2,         0,         128,         0,        1,        1,         0},         // (subrame number 7)
{1,            -1,         2,         1,         128,         0,        1,        1,         0},         // (subrame number 7)
{1,            -1,         1,         0,         128,         0,        1,        1,         0},         // (subrame number 7)
{2,            -1,         16,        1,         64,          0,        1,        1,         0},         // (subrame number 6)
{2,            -1,         8,         1,         64,          0,        1,        1,         0},         // (subrame number 6)
{2,            -1,         4,         1,         64,          0,        1,        1,         0},         // (subrame number 6)
{2,            -1,         2,         0,         64,          7,        1,        1,         0},         // (subrame number 6)
{2,            -1,         2,         1,         64,          7,        1,        1,         0},         // (subrame number 6)
{2,            -1,         1,         0,         64,          7,        1,        1,         0},         // (subrame number 6)
{3,            -1,         16,        1,         512,         0,        1,        1,         0},         // (subrame number 9)
{3,            -1,         8,         1,         512,         0,        1,        1,         0},         // (subrame number 9)
{3,            -1,         4,         1,         512,         0,        1,        1,         0},         // (subrame number 9)
{3,            -1,         2,         0,         512,         0,        1,        1,         0},         // (subrame number 9)
{3,            -1,         2,         1,         512,         0,        1,        1,         0},         // (subrame number 9)
{3,            -1,         2,         0,         16,          0,        1,        1,         0},         // (subrame number 4)
{3,            -1,         2,         1,         16,          0,        1,        1,         0},         // (subrame number 4)
{3,            -1,         1,         0,         512,         0,        1,        1,         0},         // (subrame number 9)
{3,            -1,         1,         0,         256,         0,        1,        1,         0},         // (subrame number 8)
{3,            -1,         1,         0,         128,         0,        1,        1,         0},         // (subrame number 7)
{3,            -1,         1,         0,         64,          0,        1,        1,         0},         // (subrame number 6)
{3,            -1,         1,         0,         32,          0,        1,        1,         0},         // (subrame number 5)
{3,            -1,         1,         0,         16,          0,        1,        1,         0},         // (subrame number 4)
{3,            -1,         1,         0,         8,           0,        1,        1,         0},         // (subrame number 3)
{3,            -1,         1,         0,         4,           0,        1,        1,         0},         // (subrame number 2)
{3,            -1,         1,         0,         66,          0,        1,        1,         0},         // (subrame number 1,6)
{3,            -1,         1,         0,         66,          7,        1,        1,         0},         // (subrame number 1,6)
{3,            -1,         1,         0,         528,         0,        1,        1,         0},         // (subrame number 4,9)
{3,            -1,         1,         0,         264,         0,        1,        1,         0},         // (subrame number 3,8)
{3,            -1,         1,         0,         132,         0,        1,        1,         0},         // (subrame number 2,7)
{3,            -1,         1,         0,         768,         0,        1,        1,         0},         // (subrame number 8,9)
{3,            -1,         1,         0,         784,         0,        1,        1,         0},         // (subrame number 4,8,9)
{3,            -1,         1,         0,         536,         0,        1,        1,         0},         // (subrame number 3,4,9)
{3,            -1,         1,         0,         896,         0,        1,        1,         0},         // (subrame number 7,8,9)
{3,            -1,         1,         0,         792,         0,        1,        1,         0},         // (subrame number 3,4,8,9)
{3,            -1,         1,         0,         594,         0,        1,        1,         0},         // (subrame number 1,4,6,9)
{3,            -1,         1,         0,         682,         0,        1,        1,         0},         // (subrame number 1,3,5,7,9)
{0xa1,         -1,         16,        1,         512,         0,         2,         6,         2},         // (subrame number 9)
{0xa1,         -1,         8,         1,         512,         0,         2,         6,         2},         // (subrame number 9)
{0xa1,         -1,         4,         1,         512,         0,         1,         6,         2},         // (subrame number 9)
{0xa1,         -1,         2,         1,         512,         0,         1,         6,         2},         // (subrame number 9)
{0xa1,         -1,         2,         1,         528,         7,         1,         3,         2},         // (subrame number 4,9)
{0xa1,         -1,         2,         1,         640,         7,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         -1,         2,         1,         640,         0,         1,         6,         2},         // (subrame number 7,9)
{0xa1,         -1,         2,         1,         768,         0,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         -1,         2,         1,         528,         0,         2,         6,         2},         // (subrame number 4,9)
{0xa1,         -1,         2,         1,         924,         0,         1,         6,         2},         // (subrame number 2,3,4,7,8,9)
{0xa1,         -1,         1,         0,         512,         0,         2,         6,         2},         // (subrame number 9)
{0xa1,         -1,         1,         0,         512,         7,         1,         3,         2},         // (subrame number 9)
{0xa1,         -1,         1,         0,         512,         0,         1,         6,         2},         // (subrame number 9)
{0xa1,         -1,         1,         0,         768,         0,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         -1,         1,         0,         528,         0,         1,         6,         2},         // (subrame number 4,9)
{0xa1,         -1,         1,         0,         640,         7,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         -1,         1,         0,         792,         0,         1,         6,         2},         // (subrame number 3,4,8,9)
{0xa1,         -1,         1,         0,         792,         0,         2,         6,         2},         // (subrame number 3,4,8,9)
{0xa1,         -1,         1,         0,         682,         0,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xa1,         -1,         1,         0,         1023,        7,         1,         3,         2},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa2,         -1,         16,        1,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         -1,         8,         1,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         -1,         4,         1,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         -1,         2,         1,         640,         0,         1,         3,         4},         // (subrame number 7,9)
{0xa2,         -1,         2,         1,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         -1,         2,         1,         640,         9,         1,         1,         4},         // (subrame number 7,9)
{0xa2,         -1,         2,         1,         528,         9,         1,         1,         4},         // (subrame number 4,9)
{0xa2,         -1,         2,         1,         528,         0,         2,         3,         4},         // (subrame number 4,9)
{0xa2,         -1,         16,        1,         924,         0,         1,         3,         4},         // (subrame number 2,3,4,7,8,9)
{0xa2,         -1,         1,         0,         4,           0,         1,         3,         4},         // (subrame number 2)
{0xa2,         -1,         1,         0,         128,         0,         1,         3,         4},         // (subrame number 7)
{0xa2,         -1,         2,         1,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         512,         9,         1,         1,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         132,         0,         1,         3,         4},         // (subrame number 2,7)
{0xa2,         -1,         1,         0,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         -1,         1,         0,         528,         0,         1,         3,         4},         // (subrame number 4,9)
{0xa2,         -1,         1,         0,         640,         9,         1,         1,         4},         // (subrame number 7,9)
{0xa2,         -1,         1,         0,         792,         0,         1,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         -1,         1,         0,         792,         0,         2,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         -1,         1,         0,         682,         0,         1,         3,         4},         // (subrame number 1,3,5,7,9)
{0xa2,         -1,         1,         0,         1023,        9,         1,         1,         4},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa3,         -1,         16,        1,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         -1,         8,         1,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         -1,         4,         1,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         -1,         2,         1,         528,         7,         1,         1,         6},         // (subrame number 4,9)
{0xa3,         -1,         2,         1,         640,         7,         1,         1,         6},         // (subrame number 7,9)
{0xa3,         -1,         2,         1,         640,         0,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         -1,         2,         1,         528,         0,         2,         2,         6},         // (subrame number 4,9)
{0xa3,         -1,         2,         1,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         -1,         2,         1,         924,         0,         1,         2,         6},         // (subrame number 2,3,4,7,8,9)
{0xa3,         -1,         1,         0,         4,           0,         1,         2,         6},         // (subrame number 2)
{0xa3,         -1,         1,         0,         128,         0,         1,         2,         6},         // (subrame number 7)
{0xa3,         -1,         2,         1,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         512,         7,         1,         1,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         132,         0,         1,         2,         6},         // (subrame number 2,7)
{0xa3,         -1,         1,         0,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         -1,         1,         0,         528,         0,         1,         2,         6},         // (subrame number 4,9)
{0xa3,         -1,         1,         0,         640,         7,         1,         1,         6},         // (subrame number 7,9)
{0xa3,         -1,         1,         0,         792,         0,         1,         2,         6},         // (subrame number 3,4,8,9)
{0xa3,         -1,         1,         0,         792,         0,         2,         2,         6},         // (subrame number 3,4,8,9)
{0xa3,         -1,         1,         0,         682,         0,         1,         2,         6},         // (subrame number 1,3,5,7,9)
{0xa3,         -1,         1,         0,         1023,        7,         1,         1,         6},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xb1,         -1,         4,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xb1,         -1,         2,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xb1,         -1,         2,         1,         640,         2,         1,         6,         2},         // (subrame number 7,9)
{0xb1,         -1,         2,         1,         528,         8,         1,         3,         2},         // (subrame number 4,9)
{0xb1,         -1,         2,         1,         528,         2,         2,         6,         2},         // (subrame number 4,9)
{0xb1,         -1,         1,         0,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xb1,         -1,         1,         0,         512,         8,         1,         3,         2},         // (subrame number 9)
{0xb1,         -1,         1,         0,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xb1,         -1,         1,         0,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xb1,         -1,         1,         0,         528,         2,         1,         6,         2},         // (subrame number 4,9)
{0xb1,         -1,         1,         0,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xb1,         -1,         1,         0,         682,         2,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xb4,         -1,         16,        1,         512,         0,         2,         1,         12},        // (subrame number 9)
{0xb4,         -1,         8,         1,         512,         0,         2,         1,         12},        // (subrame number 9)
{0xb4,         -1,         4,         1,         512,         2,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         2,         1,         512,         0,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         2,         1,         512,         2,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         2,         1,         640,         2,         1,         1,         12},        // (subrame number 7,9)
{0xb4,         -1,         2,         1,         528,         2,         1,         1,         12},        // (subrame number 4,9)
{0xb4,         -1,         2,         1,         528,         0,         2,         1,         12},        // (subrame number 4,9)
{0xb4,         -1,         2,         1,         768,         0,         2,         1,         12},        // (subrame number 8,9)
{0xb4,         -1,         2,         1,         924,         0,         1,         1,         12},        // (subrame number 2,3,4,7,8,9)
{0xb4,         -1,         1,         0,         2,           0,         1,         1,         12},        // (subrame number 1)
{0xb4,         -1,         1,         0,         4,           0,         1,         1,         12},        // (subrame number 2)
{0xb4,         -1,         1,         0,         16,          0,         1,         1,         12},        // (subrame number 4)
{0xb4,         -1,         1,         0,         128,         0,         1,         1,         12},        // (subrame number 7)
{0xb4,         -1,         1,         0,         512,         0,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         1,         0,         512,         2,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         1,         0,         512,         0,         2,         1,         12},        // (subrame number 9)
{0xb4,         -1,         1,         0,         528,         2,         1,         1,         12},        // (subrame number 4,9)
{0xb4,         -1,         1,         0,         640,         2,         1,         1,         12},        // (subrame number 7,9)
{0xb4,         -1,         1,         0,         768,         0,         2,         1,         12},        // (subrame number 8,9)
{0xb4,         -1,         1,         0,         792,         2,         1,         1,         12},        // (subrame number 3,4,8,9)
{0xb4,         -1,         1,         0,         682,         2,         1,         1,         12},        // (subrame number 1,3,5,7,9)
{0xb4,         -1,         1,         0,         1023,        0,         2,         1,         12},        // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xb4,         -1,         1,         0,         1023,        2,         1,         1,         12},        // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xc0,         -1,         16,        1,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xc0,         -1,         8,         1,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xc0,         -1,         4,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xc0,         -1,         2,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xc0,         -1,         2,         1,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xc0,         -1,         2,         1,         640,         2,         1,         6,         2},         // (subrame number 7,9)
{0xc0,         -1,         2,         1,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xc0,         -1,         2,         1,         528,         8,         1,         3,         2},         // (subrame number 4,9)
{0xc0,         -1,         2,         1,         528,         2,         2,         6,         2},         // (subrame number 4,9)
{0xc0,         -1,         2,         1,         924,         2,         1,         6,         2},         // (subrame number 2,3,4,7,8,9)
{0xc0,         -1,         1,         0,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xc0,         -1,         1,         0,         512,         8,         1,         3,         2},         // (subrame number 9)
{0xc0,         -1,         1,         0,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xc0,         -1,         1,         0,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xc0,         -1,         1,         0,         528,         2,         1,         6,         2},         // (subrame number 4,9)
{0xc0,         -1,         1,         0,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xc0,         -1,         1,         0,         792,         2,         1,         6,         2},         // (subrame number 3,4,8,9)
{0xc0,         -1,         1,         0,         792,         2,         2,         6,         2},         // (subrame number 3,4,8,9)
{0xc0,         -1,         1,         0,         682,         2,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xc0,         -1,         1,         0,         1023,        8,         1,         3,         2},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xc2,         -1,         16,        1,         512,         2,         2,         2,         6},         // (subrame number 9)
{0xc2,         -1,         8,         1,         512,         2,         2,         2,         6},         // (subrame number 9)
{0xc2,         -1,         4,         1,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xc2,         -1,         2,         1,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xc2,         -1,         2,         1,         768,         2,         2,         2,         6},         // (subrame number 8,9)
{0xc2,         -1,         2,         1,         640,         2,         1,         2,         6},         // (subrame number 7,9)
{0xc2,         -1,         2,         1,         640,         8,         1,         1,         6},         // (subrame number 7,9)
{0xc2,         -1,         2,         1,         528,         8,         1,         1,         6},         // (subrame number 4,9)
{0xc2,         -1,         2,         1,         528,         2,         2,         2,         6},         // (subrame number 4,9)
{0xc2,         -1,         2,         1,         924,         2,         1,         2,         6},         // (subrame number 2,3,4,7,8,9)
{0xc2,         -1,         8,         1,         512,         8,         2,         1,         6},         // (subrame number 9)
{0xc2,         -1,         4,         1,         512,         8,         1,         1,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         512,         2,         2,         2,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         512,         8,         1,         1,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         768,         2,         2,         2,         6},         // (subrame number 8,9)
{0xc2,         -1,         1,         0,         528,         2,         1,         2,         6},         // (subrame number 4,9)
{0xc2,         -1,         1,         0,         640,         8,         1,         1,         6},         // (subrame number 7,9)
{0xc2,         -1,         1,         0,         792,         2,         1,         2,         6},         // (subrame number 3,4,8,9)
{0xc2,         -1,         1,         0,         792,         2,         2,         2,         6},         // (subrame number 3,4,8,9)
{0xc2,         -1,         1,         0,         682,         2,         1,         2,         6},         // (subrame number 1,3,5,7,9)
{0xc2,         -1,         1,         0,         1023,        8,         1,         1,         6},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa1,         0xb1,       2,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xa1,         0xb1,       2,         1,         528,         8,         1,         3,         2},         // (subrame number 4,9)
{0xa1,         0xb1,       2,         1,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         0xb1,       2,         1,         640,         2,         1,         6,         2},         // (subrame number 7,9)
{0xa1,         0xb1,       2,         1,         528,         2,         2,         6,         2},         // (subrame number 4,9)
{0xa1,         0xb1,       2,         1,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         0xb1,       1,         0,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xa1,         0xb1,       1,         0,         512,         8,         1,         3,         2},         // (subrame number 9)
{0xa1,         0xb1,       1,         0,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xa1,         0xb1,       1,         0,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         0xb1,       1,         0,         528,         2,         1,         6,         2},         // (subrame number 4,9)
{0xa1,         0xb1,       1,         0,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         0xb1,       1,         0,         792,         2,         2,         6,         2},         // (subrame number 3,4,8,9)
{0xa1,         0xb1,       1,         0,         682,         2,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xa1,         0xb1,       1,         0,         1023,        8,         1,         3,         2},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa2,         0xb2,       2,         1,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         0xb2,       2,         1,         528,         6,         1,         2,         4},         // (subrame number 4,9)
{0xa2,         0xb2,       2,         1,         640,         6,         1,         2,         4},         // (subrame number 7,9)
{0xa2,         0xb2,       2,         1,         528,         0,         2,         3,         4},         // (subrame number 4,9)
{0xa2,         0xb2,       2,         1,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         0xb2,       1,         0,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         0xb2,       1,         0,         512,         6,         1,         2,         4},         // (subrame number 9)
{0xa2,         0xb2,       1,         0,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         0xb2,       1,         0,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         0xb2,       1,         0,         528,         0,         1,         3,         4},         // (subrame number 4,9)
{0xa2,         0xb2,       1,         0,         640,         6,         1,         2,         4},         // (subrame number 7,9)
{0xa2,         0xb2,       1,         0,         792,         0,         1,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         0xb2,       1,         0,         792,         0,         2,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         0xb2,       1,         0,         682,         0,         1,         3,         4},         // (subrame number 1,3,5,7,9)
{0xa2,         0xb2,       1,         0,         1023,        6,         1,         2,         4},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa3,         0xb3,       2,         1,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       2,         1,         528,         2,         1,         2,         6},         // (subrame number 4,9)
{0xa3,         0xb3,       2,         1,         640,         0,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         0xb3,       2,         1,         640,         2,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         0xb3,       2,         1,         528,         0,         2,         2,         6},         // (subrame number 4,9)
{0xa3,         0xb3,       2,         1,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         0xb3,       1,         0,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       1,         0,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       1,         0,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       1,         0,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         0xb3,       1,         0,         528,         0,         1,         2,         6},         // (subrame number 4,9)
{0xa3,         0xb3,       1,         0,         640,         2,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         0xb3,       1,         0,         792,         0,         2,         2,         6},         // (subrame number 3,4,8,9)
{0xa3,         0xb3,       1,         0,         682,         0,         1,         2,         6},         // (subrame number 1,3,5,7,9)
{0xa3,         0xb3,       1,         0,         1023,        2,         1,         2,         6}          // (subrame number 0,1,2,3,4,5,6,7,8,9)
};
// Table 6.3.3.2-4: Random access configurations for FR2 and unpaired spectrum
int64_t table_6_3_3_2_4_prachConfig_Index [256][10] = {
//format,      format,       x,          y,           y,              SFN_nbr,       star_symb,   slots_sfn,  occ_slot,  duration
{0xa1,          -1,          16,         1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          16,         1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          8,          1,           2,          550293209600,          0,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xa1,          -1,          8,          1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          8,          1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          4,          1,          -1,          567489872400,          0,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          4,          1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          4,          1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          2,          1,          -1,          551911719040,          0,          2,          6,          2},          // (subframe number :7,15,23,31,39)
{0xa1,          -1,          2,          1,          -1,          567489872400,          0,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          2,          1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          2,          1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          549756338176,          7,          1,          3,          2},          // (subframe number :19,39)
{0xa1,          -1,          1,          0,          -1,          168,                   0,          1,          6,          2},          // (subframe number :3,5,7)
{0xa1,          -1,          1,          0,          -1,          567489331200,          7,          1,          3,          2},          // (subframe number :24,29,34,39)
{0xa1,          -1,          1,          0,          -1,          550293209600,          7,          2,          3,          2},          // (subframe number :9,19,29,39)
{0xa1,          -1,          1,          0,          -1,          687195422720,          0,          1,          6,          2},          // (subframe number :17,19,37,39)
{0xa1,          -1,          1,          0,          -1,          550293209600,          0,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xa1,          -1,          1,          0,          -1,          567489872400,          0,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          1,          0,          -1,          567489872400,          7,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          1,          0,          -1,          10920,                 7,          1,          3,          2},          // (subframe number :3,5,7,9,11,13)
{0xa1,          -1,          1,          0,          -1,          586405642240,          7,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          551911719040,          0,          1,          6,          2},          // (subframe number :7,15,23,31,39)
{0xa1,          -1,          1,          0,          -1,          586405642240,          0,          1,          6,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          965830828032,          7,          2,          3,          2},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xa1,          -1,          1,          0,          -1,          586406201480,          7,          1,          3,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          733007751850,          0,          1,          6,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xa1,          -1,          1,          0,          -1,          1099511627775,         7,          1,          3,          2},          // (subframe number :0,1,2,…,39)
{0xa2,          -1,          16,         1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          16,         1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          8,          1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          8,          1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          8,          1,           2,          550293209600,          0,          2,          3,          4},          // (subframe number :9,19,29,39)
{0xa2,          -1,          4,          1,          -1,          567489872400,          0,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          4,          1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          4,          1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          2,          1,          -1,          551911719040,          0,          2,          3,          4},          // (subframe number :7,15,23,31,39)
{0xa2,          -1,          2,          1,          -1,          567489872400,          0,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          2,          1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          2,          1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          549756338176,          5,          1,          2,          4},          // (subframe number :19,39)
{0xa2,          -1,          1,          0,          -1,          168,                   0,          1,          3,          4},          // (subframe number :3,5,7)
{0xa2,          -1,          1,          0,          -1,          567489331200,          5,          1,          2,          4},          // (subframe number :24,29,34,39)
{0xa2,          -1,          1,          0,          -1,          550293209600,          5,          2,          2,          4},          // (subframe number :9,19,29,39)
{0xa2,          -1,          1,          0,          -1,          687195422720,          0,          1,          3,          4},          // (subframe number :17,19,37,39)
{0xa2,          -1,          1,          0,          -1,          550293209600,          0,          2,          3,          4},          // (subframe number :9, 19, 29, 39)
{0xa2,          -1,          1,          0,          -1,          551911719040,          0,          1,          3,          4},          // (subframe number :7,15,23,31,39)
{0xa2,          -1,          1,          0,          -1,          586405642240,          5,          1,          2,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          586405642240,          0,          1,          3,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          10920,                 5,          1,          2,          4},          // (subframe number :3,5,7,9,11,13)
{0xa2,          -1,          1,          0,          -1,          10920,                 0,          1,          3,          4},          // (subframe number :3,5,7,9,11,13)
{0xa2,          -1,          1,          0,          -1,          567489872400,          5,          1,          2,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          1,          0,          -1,          567489872400,          0,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          1,          0,          -1,          965830828032,          5,          2,          2,          4},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xa2,          -1,          1,          0,          -1,          586406201480,          5,          1,          2,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          733007751850,          0,          1,          3,          4},          // (subframe number :1,3,5,7,…,37,39)
{0xa2,          -1,          1,          0,          -1,          1099511627775,         5,          1,          2,          4},          // (subframe number :0,1,2,…,39)
{0xa3,          -1,          16,         1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          16,         1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          8,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          8,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          8,          1,           2,          550293209600,          0,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          -1,          4,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          4,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          4,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          2,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          2,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          2,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          549756338176,          7,          1,          1,          6},          // (subframe number :19,39)
{0xa3,          -1,          1,          0,          -1,          168,                   0,          1,          2,          6},          // (subframe number :3,5,7)
{0xa3,          -1,          1,          0,          -1,          10752,                 2,          1,          2,          6},          // (subframe number :9,11,13)
{0xa3,          -1,          1,          0,          -1,          567489331200,          7,          1,          1,          6},          // (subframe number :24,29,34,39)
{0xa3,          -1,          1,          0,          -1,          550293209600,          7,          2,          1,          6},          // (subframe number :9,19,29,39)
{0xa3,          -1,          1,          0,          -1,          687195422720,          0,          1,          2,          6},          // (subframe number :17,19,37,39)
{0xa3,          -1,          1,          0,          -1,          550293209600,          0,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          -1,          1,          0,          -1,          551911719040,          0,          1,          2,          6},          // (subframe number :7,15,23,31,39)
{0xa3,          -1,          1,          0,          -1,          586405642240,          7,          1,          1,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          586405642240,          0,          1,          2,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          10920,                 0,          1,          2,          6},          // (subframe number :3,5,7,9,11,13)
{0xa3,          -1,          1,          0,          -1,          10920,                 7,          1,          1,          6},          // (subframe number :3,5,7,9,11,13)
{0xa3,          -1,          1,          0,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          1,          0,          -1,          567489872400,          7,          1,          1,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          1,          0,          -1,          965830828032,          7,          2,          1,          6},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xa3,          -1,          1,          0,          -1,          586406201480,          7,          1,          1,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          733007751850,          0,          1,          2,          6},          // (subframe number :1,3,5,7,…,37,39)
{0xa3,          -1,          1,          0,          -1,          1099511627775,         7,          1,          1,          6},          // (subframe number :0,1,2,…,39)
{0xb1,          -1,          16,         1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          8,          1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          8,          1,           2,          550293209600,          2,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xb1,          -1,          4,          1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          2,          1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          2,          1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          549756338176,          8,          1,          3,          2},          // (subframe number :19,39)
{0xb1,          -1,          1,          0,          -1,          168,                   2,          1,          6,          2},          // (subframe number :3,5,7)
{0xb1,          -1,          1,          0,          -1,          567489331200,          8,          1,          3,          2},          // (subframe number :24,29,34,39)
{0xb1,          -1,          1,          0,          -1,          550293209600,          8,          2,          3,          2},          // (subframe number :9,19,29,39)
{0xb1,          -1,          1,          0,          -1,          687195422720,          2,          1,          6,          2},          // (subframe number :17,19,37,39)
{0xb1,          -1,          1,          0,          -1,          550293209600,          2,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xb1,          -1,          1,          0,          -1,          551911719040,          2,          1,          6,          2},          // (subframe number :7,15,23,31,39)
{0xb1,          -1,          1,          0,          -1,          586405642240,          8,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          586405642240,          2,          1,          6,          2},          // (subframe number :23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          10920,                 8,          1,          3,          2},          // (subframe number :3,5,7,9,11,13)
{0xb1,          -1,          1,          0,          -1,          567489872400,          8,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          1,          0,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          1,          0,          -1,          586406201480,          8,          1,          3,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          965830828032,          8,          2,          3,          2},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xb1,          -1,          1,          0,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          733007751850,          2,          1,          6,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xb1,          -1,          1,          0,          -1,          1099511627775,         8,          1,          3,          2},          // (subframe number :0,1,2,…,39)
{0xb4,          -1,          16,         1,           2,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          16,         1,           2,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          8,          1,           2,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          8,          1,           2,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          8,          1,           2,          550293209600,          0,          2,          1,          12},         // (subframe number :9,19,29,39)
{0xb4,          -1,          4,          1,          -1,          567489872400,          0,          1,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          4,          1,          -1,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          4,          1,           2,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          2,          1,          -1,          551911719040,          2,          2,          1,          12},         // (subframe number :7,15,23,31,39)
{0xb4,          -1,          2,          1,          -1,          567489872400,          0,          1,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          2,          1,          -1,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          2,          1,          -1,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          549756338176,          2,          2,          1,          12},         // (subframe number :19, 39)
{0xb4,          -1,          1,          0,          -1,          687195422720,          0,          1,          1,          12},         // (subframe number :17, 19, 37, 39)
{0xb4,          -1,          1,          0,          -1,          567489331200,          2,          1,          1,          12},         // (subframe number :24,29,34,39)
{0xb4,          -1,          1,          0,          -1,          550293209600,          2,          2,          1,          12},         // (subframe number :9,19,29,39)
{0xb4,          -1,          1,          0,          -1,          550293209600,          0,          2,          1,          12},         // (subframe number :9,19,29,39)
{0xb4,          -1,          1,          0,          -1,          551911719040,          0,          1,          1,          12},         // (subframe number :7,15,23,31,39)
{0xb4,          -1,          1,          0,          -1,          551911719040,          0,          2,          1,          12},         // (subframe number :7,15,23,31,39)
{0xb4,          -1,          1,          0,          -1,          586405642240,          0,          1,          1,          12},         // (subframe number :23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          586405642240,          2,          2,          1,          12},         // (subframe number :23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          698880,                0,          1,          1,          12},         // (subframe number :9,11,13,15,17,19)
{0xb4,          -1,          1,          0,          -1,          10920,                 2,          1,          1,          12},         // (subframe number :3,5,7,9,11,13)
{0xb4,          -1,          1,          0,          -1,          567489872400,          0,          1,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          1,          0,          -1,          567489872400,          2,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          1,          0,          -1,          965830828032,          2,          2,          1,          12},         // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xb4,          -1,          1,          0,          -1,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          586406201480,          2,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          44739240,              2,          1,          1,          12},         // (subframe number :3, 5, 7, …, 23,25)
{0xb4,          -1,          1,          0,          -1,          44739240,              0,          2,          1,          12},         // (subframe number :3, 5, 7, …, 23,25)
{0xb4,          -1,          1,          0,          -1,          733007751850,          0,          1,          1,          12},         // (subframe number :1,3,5,7,…,37,39)
{0xb4,          -1,          1,          0,          -1,          1099511627775,         2,          1,          1,          12},         // (subframe number :0, 1, 2,…, 39)
{0xc0,          -1,          16,         1,          -1,          567489872400,          0,          2,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          16,         1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          8,          1,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          8,          1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          8,          1,           2,          550293209600,          0,          2,          7,          2},          // (subframe number :9,19,29,39)
{0xc0,          -1,          4,          1,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          4,          1,          -1,          567489872400,          0,          2,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          4,          1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          2,          1,          -1,          551911719040,          0,          2,          7,          2},          // (subframe number :7,15,23,31,39)
{0xc0,          -1,          2,          1,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          2,          1,          -1,          567489872400,          0,          2,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          2,          1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          549756338176,          8,          1,          3,          2},          // (subframe number :19,39)
{0xc0,          -1,          1,          0,          -1,          168,                   0,          1,          7,          2},          // (subframe number :3,5,7)
{0xc0,          -1,          1,          0,          -1,          567489331200,          8,          1,          3,          2},          // (subframe number :24,29,34,39)
{0xc0,          -1,          1,          0,          -1,          550293209600,          8,          2,          3,          2},          // (subframe number :9,19,29,39)
{0xc0,          -1,          1,          0,          -1,          687195422720,          0,          1,          7,          2},          // (subframe number :17,19,37,39)
{0xc0,          -1,          1,          0,          -1,          550293209600,          0,          2,          7,          2},          // (subframe number :9,19,29,39)
{0xc0,          -1,          1,          0,          -1,          586405642240,          8,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          551911719040,          0,          1,          7,          2},          // (subframe number :7,15,23,31,39)
{0xc0,          -1,          1,          0,          -1,          586405642240,          0,          1,          7,          2},          // (subframe number :23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          10920,                 8,          1,          3,          2},          // (subframe number :3,5,7,9,11,13)
{0xc0,          -1,          1,          0,          -1,          567489872400,          8,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          1,          0,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          1,          0,          -1,          965830828032,          8,          2,          3,          2},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xc0,          -1,          1,          0,          -1,          586406201480,          8,          1,          3,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          733007751850,          0,          1,          7,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xc0,          -1,          1,          0,          -1,          1099511627775,         8,          1,          3,          2},          // (subframe number :0,1,2,…,39)
{0xc2,          -1,          16,         1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          16,         1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          8,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          8,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          8,          1,           2,          550293209600,          0,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xc2,          -1,          4,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          4,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          4,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          2,          1,          -1,          551911719040,          2,          2,          2,          6},          // (subframe number :7,15,23,31,39)
{0xc2,          -1,          2,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          2,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          2,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          549756338176,          2,          1,          2,          6},          // (subframe number :19,39)
{0xc2,          -1,          1,          0,          -1,          168,                   0,          1,          2,          6},          // (subframe number :3,5,7)
{0xc2,          -1,          1,          0,          -1,          567489331200,          7,          1,          1,          6},          // (subframe number :24,29,34,39)
{0xc2,          -1,          1,          0,          -1,          550293209600,          7,          2,          1,          6},          // (subframe number :9,19,29,39)
{0xc2,          -1,          1,          0,          -1,          687195422720,          0,          1,          2,          6},          // (subframe number :17,19,37,39)
{0xc2,          -1,          1,          0,          -1,          550293209600,          2,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xc2,          -1,          1,          0,          -1,          551911719040,          2,          1,          2,          6},          // (subframe number :7,15,23,31,39)
{0xc2,          -1,          1,          0,          -1,          10920,                 7,          1,          1,          6},          // (subframe number :3,5,7,9,11,13)
{0xc2,          -1,          1,          0,          -1,          586405642240,          7,          2,          1,          6},          // (subframe number :23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          586405642240,          0,          1,          2,          6},          // (subframe number :23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          567489872400,          7,          2,          1,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          1,          0,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          1,          0,          -1,          965830828032,          7,          2,          1,          6},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xc2,          -1,          1,          0,          -1,          586406201480,          7,          1,          1,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          733007751850,          0,          1,          2,          6},          // (subframe number :1,3,5,7,…,37,39)
{0xc2,          -1,          1,          0,          -1,          1099511627775,         7,          1,          1,          6},          // (subframe number :0,1,2,…,39)
{0xa1,          0xb1,        16,         1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        16,         1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        8,          1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        8,          1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        4,          1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        4,          1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        2,          1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        1,          0,          -1,          549756338176,          8,          1,          3,          2},          // (subframe number :19,39)
{0xa1,          0xb1,        1,          0,          -1,          550293209600,          8,          1,          3,          2},          // (subframe number :9,19,29,39)
{0xa1,          0xb1,        1,          0,          -1,          687195422720,          2,          1,          6,          2},          // (subframe number :17,19,37,39)
{0xa1,          0xb1,        1,          0,          -1,          550293209600,          2,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xa1,          0xb1,        1,          0,          -1,          586405642240,          8,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          0xb1,        1,          0,          -1,          551911719040,          2,          1,          6,          2},          // (subframe number :7,15,23,31,39)
{0xa1,          0xb1,        1,          0,          -1,          586405642240,          2,          1,          6,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          0xb1,        1,          0,          -1,          567489872400,          8,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        1,          0,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        1,          0,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        1,          0,          -1,          733007751850,          2,          1,          6,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xa2,          0xb2,        16,         1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        16,         1,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        8,          1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        8,          1,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        4,          1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        4,          1,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        2,          1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        1,          0,          -1,          549756338176,          6,          1,          2,          4},          // (subframe number :19,39)
{0xa2,          0xb2,        1,          0,          -1,          550293209600,          6,          1,          2,          4},          // (subframe number :9,19,29,39)
{0xa2,          0xb2,        1,          0,          -1,          687195422720,          2,          1,          3,          4},          // (subframe number :17,19,37,39)
{0xa2,          0xb2,        1,          0,          -1,          550293209600,          2,          2,          3,          4},          // (subframe number :9,19,29,39)
{0xa2,          0xb2,        1,          0,          -1,          586405642240,          6,          1,          2,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          0xb2,        1,          0,          -1,          551911719040,          2,          1,          3,          4},          // (subframe number :7,15,23,31,39)
{0xa2,          0xb2,        1,          0,          -1,          586405642240,          2,          1,          3,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          0xb2,        1,          0,          -1,          567489872400,          6,          1,          2,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        1,          0,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        1,          0,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        1,          0,          -1,          733007751850,          2,          1,          3,          4},          // (subframe number :1,3,5,7,…,37,39)
{0xa3,          0xb3,        16,         1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        16,         1,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        8,          1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        8,          1,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        4,          1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        4,          1,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        2,          1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        1,          0,          -1,          549756338176,          2,          1,          2,          6},          // (subframe number :19,39)
{0xa3,          0xb3,        1,          0,          -1,          550293209600,          2,          1,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          0xb3,        1,          0,          -1,          687195422720,          2,          1,          2,          6},          // (subframe number :17,19,37,39)
{0xa3,          0xb3,        1,          0,          -1,          550293209600,          2,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          0xb3,        1,          0,          -1,          551911719040,          2,          1,          2,          6},          // (subframe number :7,15,23,31,39)
{0xa3,          0xb3,        1,          0,          -1,          586405642240,          2,          1,          2,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          0xb3,        1,          0,          -1,          586405642240,          2,          2,          2,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          0xb3,        1,          0,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        1,          0,          -1,          567489872400,          2,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        1,          0,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        1,          0,          -1,          733007751850,          2,          1,          2,          6}           // (subframe number :1,3,5,7,…,37,39)
};


int get_format0(uint8_t index,
                uint8_t unpaired,
		frequency_range_t frequency_range){

  uint16_t format=0;
  if (unpaired) {
    if (frequency_range==FR1)
      format = table_6_3_3_2_3_prachConfig_Index[index][0];
    else
      format = table_6_3_3_2_4_prachConfig_Index[index][0];
  }
  else {
    if (frequency_range==FR1)
      format = table_6_3_3_2_2_prachConfig_Index[index][0];
    else
      AssertFatal(0==1,"no paired spectrum for FR2\n");
  }
  return format;
}

int64_t *get_prach_config_info(frequency_range_t freq_range,
                               uint8_t index,
                               uint8_t unpaired) {
  int64_t *prach_config_info_p;

  if (freq_range == FR2) { //FR2
    prach_config_info_p = table_6_3_3_2_4_prachConfig_Index[index];
  }
  else { // FR1
    if (unpaired)
      prach_config_info_p = table_6_3_3_2_3_prachConfig_Index[index];
    else
      prach_config_info_p = table_6_3_3_2_2_prachConfig_Index[index];
  } // FR2 / FR1

  return prach_config_info_p;
}

void find_aggregation_candidates(uint8_t *aggregation_level,
                                 uint8_t *nr_of_candidates,
                                 NR_SearchSpace_t *ss,
                                 int L) {
  AssertFatal(L>=1 && L<=16,"L %d not ok\n",L);
  *nr_of_candidates = 0;
  switch(L) {
    case 1:
      if (ss->nrofCandidates->aggregationLevel1 != NR_SearchSpace__nrofCandidates__aggregationLevel1_n0) {
        *aggregation_level = 1;
        *nr_of_candidates = ss->nrofCandidates->aggregationLevel1;
      }
      break;
    case 2:
      if (ss->nrofCandidates->aggregationLevel2 != NR_SearchSpace__nrofCandidates__aggregationLevel2_n0) {
        *aggregation_level = 2;
        *nr_of_candidates = ss->nrofCandidates->aggregationLevel2;
      }
      break;
    case 4: 
       if (ss->nrofCandidates->aggregationLevel4 != NR_SearchSpace__nrofCandidates__aggregationLevel4_n0) {
         *aggregation_level = 4;
         *nr_of_candidates = ss->nrofCandidates->aggregationLevel4;
       }
       break;
    case 8:
       if (ss->nrofCandidates->aggregationLevel8 != NR_SearchSpace__nrofCandidates__aggregationLevel8_n0) {
         *aggregation_level = 8;
         *nr_of_candidates = ss->nrofCandidates->aggregationLevel8;
       }
       break;
    case 16:
       if (ss->nrofCandidates->aggregationLevel16 != NR_SearchSpace__nrofCandidates__aggregationLevel16_n0) {
         *aggregation_level = 16;
         *nr_of_candidates = ss->nrofCandidates->aggregationLevel16;
       }
       break;
  } 
}


void set_monitoring_periodicity_offset(NR_SearchSpace_t *ss,
                                       uint16_t period,
                                       uint16_t offset) {

  switch(period) {
    case 1:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
      break;
    case 2:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl2 = offset;
      break;
    case 4:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl4 = offset;
      break;
    case 5:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl5 = offset;
      break;
    case 8:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl8 = offset;
      break;
    case 10:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl10 = offset;
      break;
    case 16:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl16 = offset;
      break;
    case 20:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl20 = offset;
      break;
    case 40:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl40 = offset;
      break;
    case 80:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl80 = offset;
      break;
    case 160:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl160 = offset;
      break;
    case 320:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl320 = offset;
      break;
    case 640:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl640 = offset;
      break;
    case 1280:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl1280 = offset;
      break;
    case 2560:
      ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560;
      ss->monitoringSlotPeriodicityAndOffset->choice.sl2560 = offset;      break;
  default:
    AssertFatal(1==0,"Invalid monitoring slot periodicity value\n");
    break;
  }
}


void find_monitoring_periodicity_offset_common(NR_SearchSpace_t *ss,
                                               uint16_t *slot_period,
                                               uint16_t *offset) {

  switch(ss->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      *slot_period = 1;
      *offset = 0;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      *slot_period = 2;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      *slot_period = 4;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      *slot_period = 5;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      *slot_period = 8;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      *slot_period = 10;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      *slot_period = 16;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      *slot_period = 20;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      *slot_period = 40;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      *slot_period = 80;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      *slot_period = 160;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      *slot_period = 320;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      *slot_period = 640;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      *slot_period = 1280;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      *slot_period = 2560;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
  default:
    AssertFatal(1==0,"Invalid monitoring slot periodicity and offset value\n");
    break;
  }
}

int get_nr_prach_occasion_info_from_index(uint8_t index,
                                 uint32_t pointa,
                                 uint8_t mu,
                                 uint8_t unpaired,
                                 uint16_t *format,
                                 uint8_t *start_symbol,
                                 uint8_t *N_t_slot,
                                 uint8_t *N_dur,
                                 uint8_t *N_RA_slot,
                                 uint16_t *N_RA_sfn,
                                 uint8_t *max_association_period) {

  int x;
  int64_t s_map;
  uint8_t format2 = 0xff;
  if (pointa > 2016666) { //FR2
    x = table_6_3_3_2_4_prachConfig_Index[index][2];
    s_map = table_6_3_3_2_4_prachConfig_Index[index][5];
    for(int i = 0; i < 64 ;i++) {
      if ( (s_map >> i) & 0x01) {
        (*N_RA_sfn)++;
      }
    }
    *N_RA_slot = table_6_3_3_2_4_prachConfig_Index[index][7]; // Number of RACH slots within a subframe
    *max_association_period = 160/(x * 10); 
    if (start_symbol != NULL && N_t_slot != NULL && N_dur != NULL && format != NULL){
      *start_symbol = table_6_3_3_2_4_prachConfig_Index[index][6];//multiple prach occasions in diff slot
      *N_t_slot = table_6_3_3_2_4_prachConfig_Index[index][8];
      *N_dur = table_6_3_3_2_4_prachConfig_Index[index][9];
      if (table_6_3_3_2_4_prachConfig_Index[index][1] != -1)
        format2 = (uint8_t) table_6_3_3_2_4_prachConfig_Index[index][1];
        
      *format = ((uint8_t) table_6_3_3_2_4_prachConfig_Index[index][0]) | (format2<<8);
      LOG_D(MAC,"Getting Total PRACH info from index %d absoluteFrequencyPointA %u mu %u frame_type %u start_symbol %u N_t_slot %u N_dur %u N_RA_sfn = %u\n",
            index,
            pointa,
            mu,
            unpaired,
            *start_symbol,
            *N_t_slot,
            *N_dur,
	    *N_RA_sfn);
    }
    return 1;
 }
  else {

    if (unpaired) {
      x = table_6_3_3_2_3_prachConfig_Index[index][2];
      s_map = table_6_3_3_2_3_prachConfig_Index[index][4];
		  for(int i = 0; i < 64 ;i++) {
        if ( (s_map >> i) & 0x01) {
          (*N_RA_sfn)++;
				}
      }
      *N_RA_slot = table_6_3_3_2_3_prachConfig_Index[index][6]; // Number of RACH slots within a subframe
      *max_association_period = 160/(x * 10); 
      if (start_symbol != NULL && N_t_slot != NULL && N_dur != NULL && format != NULL){
        *start_symbol = table_6_3_3_2_3_prachConfig_Index[index][5];
        *N_t_slot = table_6_3_3_2_3_prachConfig_Index[index][7];
        *N_dur = table_6_3_3_2_3_prachConfig_Index[index][8];
        if (table_6_3_3_2_3_prachConfig_Index[index][1] != -1)
          format2 = (uint8_t) table_6_3_3_2_3_prachConfig_Index[index][1];
        *format = ((uint8_t) table_6_3_3_2_3_prachConfig_Index[index][0]) | (format2<<8);
        LOG_D(NR_MAC,"Getting Total PRACH info from index %d (col %lu ) absoluteFrequencyPointA %u mu %u frame_type %u start_symbol %u N_t_slot %u N_dur %u N_RA_sfn = %u\n",
              index, table_6_3_3_2_3_prachConfig_Index[index][6],
              pointa,
              mu,
              unpaired,
              *start_symbol,
              *N_t_slot,
              *N_dur,
							*N_RA_sfn);
      }
		  return 1;
	  }
    else { // FDD
      x = table_6_3_3_2_2_prachConfig_Index[index][2];
      s_map = table_6_3_3_2_2_prachConfig_Index[index][4];
      for(int i = 0; i < 64 ; i++) {
        if ( (s_map >> i) & 0x01) {
          (*N_RA_sfn)++;
        }
      }
      *N_RA_slot = table_6_3_3_2_2_prachConfig_Index[index][6];
      if (start_symbol != NULL && N_t_slot != NULL && N_dur != NULL && format != NULL){
        *start_symbol = table_6_3_3_2_2_prachConfig_Index[index][5];
        *N_t_slot = table_6_3_3_2_2_prachConfig_Index[index][7];
        *N_dur = table_6_3_3_2_2_prachConfig_Index[index][8];
        if (table_6_3_3_2_2_prachConfig_Index[index][1] != -1)
          format2 = (uint8_t) table_6_3_3_2_2_prachConfig_Index[index][1];
        *format = ((uint8_t) table_6_3_3_2_2_prachConfig_Index[index][0]) | (format2<<8);
        LOG_D(MAC,"Getting Total PRACH info from index %d absoluteFrequencyPointA %u mu %u frame_type %u start_symbol %u N_t_slot %u N_dur %u \n",
              index,
              pointa,
              mu,
              unpaired,
              *start_symbol,
              *N_t_slot,
              *N_dur);
      }
      return 1;
    }
  }
}


int get_nr_prach_info_from_index(uint8_t index,
                                 int frame,
                                 int slot,
                                 uint32_t pointa,
                                 uint8_t mu,
                                 uint8_t unpaired,
                                 uint16_t *format,
                                 uint8_t *start_symbol,
                                 uint8_t *N_t_slot,
                                 uint8_t *N_dur,
                                 uint16_t *RA_sfn_index,
                                 uint8_t *N_RA_slot,
				 uint8_t *config_period) {

  int x,y;
  int64_t s_map;
  uint8_t format2 = 0xff;

  if (pointa > 2016666) { //FR2
    int y2;
    uint8_t slot_60khz;
    x = table_6_3_3_2_4_prachConfig_Index[index][2];
    y = table_6_3_3_2_4_prachConfig_Index[index][3];
    y2 = table_6_3_3_2_4_prachConfig_Index[index][4];
    // checking n_sfn mod x = y
    if ( (frame%x)==y || (frame%x)==y2 ) {
      slot_60khz = slot >> (mu-2); // in table slots are numbered wrt 60kHz
      s_map = table_6_3_3_2_4_prachConfig_Index[index][5];
      if ((s_map >> slot_60khz) & 0x01 ) {
        for(int i = 0; i <= slot_60khz ;i++) {
          if ( (s_map >> i) & 0x01) {
            (*RA_sfn_index)++;
          }
        }
      }
      if ( ((s_map>>slot_60khz)&0x01) ) {
        *N_RA_slot = table_6_3_3_2_4_prachConfig_Index[index][7]; // Number of RACH slots within a subframe
        if (mu == 3) {
          if ( (*N_RA_slot == 1) && (slot%2 == 0) )
            return 0; // no prach in even slots @ 120kHz for 1 prach per 60khz slot
        }
        if (start_symbol != NULL && N_t_slot != NULL && N_dur != NULL && format != NULL){
          *config_period = x;
          *start_symbol = table_6_3_3_2_4_prachConfig_Index[index][6];
          *N_t_slot = table_6_3_3_2_4_prachConfig_Index[index][8];
          *N_dur = table_6_3_3_2_4_prachConfig_Index[index][9];
          if (table_6_3_3_2_4_prachConfig_Index[index][1] != -1)
            format2 = (uint8_t) table_6_3_3_2_4_prachConfig_Index[index][1];
          *format = ((uint8_t) table_6_3_3_2_4_prachConfig_Index[index][0]) | (format2<<8);
          LOG_D(MAC,"Frame %d slot %d: Getting PRACH info from index %d absoluteFrequencyPointA %u mu %u frame_type %u start_symbol %u N_t_slot %u N_dur %u N_RA_slot %u RA_sfn_index %u\n",
                frame,
                slot,
                index,
                pointa,
                mu,
                unpaired,
                *start_symbol,
                *N_t_slot,
                *N_dur,
                *N_RA_slot,
                *RA_sfn_index);
        }
        return 1;
      }
      else
        return 0; // no prach in current slot
    }
    else
      return 0; // no prach in current frame
  }
  else {
    uint8_t subframe;
    if (unpaired) {
      x = table_6_3_3_2_3_prachConfig_Index[index][2];
      y = table_6_3_3_2_3_prachConfig_Index[index][3];
      if ( (frame%x)==y ) {
        subframe = slot >> mu;
        s_map = table_6_3_3_2_3_prachConfig_Index[index][4];
        if ((s_map >> subframe) & 0x01 ) {
          for(int i = 0; i <= subframe ;i++) {
            if ( (s_map >> i) & 0x01) {
              (*RA_sfn_index)++;
            }
          }
        }
        if ( (s_map>>subframe)&0x01 ) {
         *N_RA_slot = table_6_3_3_2_3_prachConfig_Index[index][6]; // Number of RACH slots within a subframe
          if (mu == 1) {
            if ( (*N_RA_slot <= 1) && (slot%2 == 0) )
              return 0; // no prach in even slots @ 30kHz for 1 prach per subframe 
          } 
          if (start_symbol != NULL && N_t_slot != NULL && N_dur != NULL && format != NULL){
            *config_period = x;
            *start_symbol = table_6_3_3_2_3_prachConfig_Index[index][5];
            *N_t_slot = table_6_3_3_2_3_prachConfig_Index[index][7];
            *N_dur = table_6_3_3_2_3_prachConfig_Index[index][8];
            if (table_6_3_3_2_3_prachConfig_Index[index][1] != -1)
              format2 = (uint8_t) table_6_3_3_2_3_prachConfig_Index[index][1];
            *format = ((uint8_t) table_6_3_3_2_3_prachConfig_Index[index][0]) | (format2<<8);
            LOG_D(MAC,"Frame %d slot %d: Getting PRACH info from index %d (col 6 %lu) absoluteFrequencyPointA %u mu %u frame_type %u start_symbol %u N_t_slot %u N_dur %u N_RA_slot %u RA_sfn_index %u \n", frame,
              slot,
              index, table_6_3_3_2_3_prachConfig_Index[index][6],
              pointa,
              mu,
              unpaired,
              *start_symbol,
              *N_t_slot,
              *N_dur,
              *N_RA_slot,
              *RA_sfn_index);
          }
          return 1;
        }
        else
          return 0; // no prach in current slot
      }
      else
        return 0; // no prach in current frame
    }
    else { // FDD
      x = table_6_3_3_2_2_prachConfig_Index[index][2];
      y = table_6_3_3_2_2_prachConfig_Index[index][3];
      if ( (frame%x)==y ) {
        subframe = slot >> mu;
        s_map = table_6_3_3_2_2_prachConfig_Index[index][4];
        if ( (s_map>>subframe)&0x01 ) {
          *N_RA_slot = table_6_3_3_2_2_prachConfig_Index[index][6]; // Number of RACH slots within a subframe
          if (mu == 1) {
            if ((*N_RA_slot <= 1) && (slot % 2 == 0)){
              return 0; // no prach in even slots @ 30kHz for 1 prach per subframe
            }
          }
          for(int i = 0; i <= subframe ; i++) {
            if ( (s_map >> i) & 0x01) {
              (*RA_sfn_index)++;
            }
          }
          if (start_symbol != NULL && N_t_slot != NULL && N_dur != NULL && format != NULL){
            *start_symbol = table_6_3_3_2_2_prachConfig_Index[index][5];
            *config_period = x;
            *N_t_slot = table_6_3_3_2_2_prachConfig_Index[index][7];
            *N_dur = table_6_3_3_2_2_prachConfig_Index[index][8];
            if (table_6_3_3_2_2_prachConfig_Index[index][1] != -1)
              format2 = (uint8_t) table_6_3_3_2_2_prachConfig_Index[index][1];
            *format = ((uint8_t) table_6_3_3_2_2_prachConfig_Index[index][0]) | (format2<<8);
            LOG_D(MAC,"Frame %d slot %d: Getting PRACH info from index %d absoluteFrequencyPointA %u mu %u frame_type %u start_symbol %u N_t_slot %u N_dur %u \n",
                  frame,
                  slot,
                  index,
                  pointa,
                  mu,
                  unpaired,
                  *start_symbol,
                  *N_t_slot,
                  *N_dur);
          }
          return 1;
        }
        else
          return 0; // no prach in current slot
      }
      else
        return 0; // no prach in current frame
    }
  }
}

//Table 6.3.3.1-3: Mapping from logical index i to sequence number u for preamble formats with L_RA = 839
uint16_t table_63313[838] = {
129, 710, 140, 699, 120, 719, 210, 629, 168, 671, 84 , 755, 105, 734, 93 , 746, 70 , 769, 60 , 779,
2  , 837, 1  , 838, 56 , 783, 112, 727, 148, 691, 80 , 759, 42 , 797, 40 , 799, 35 , 804, 73 , 766,
146, 693, 31 , 808, 28 , 811, 30 , 809, 27 , 812, 29 , 810, 24 , 815, 48 , 791, 68 , 771, 74 , 765,
178, 661, 136, 703, 86 , 753, 78 , 761, 43 , 796, 39 , 800, 20 , 819, 21 , 818, 95 , 744, 202, 637,
190, 649, 181, 658, 137, 702, 125, 714, 151, 688, 217, 622, 128, 711, 142, 697, 122, 717, 203, 636,
118, 721, 110, 729, 89 , 750, 103, 736, 61 , 778, 55 , 784, 15 , 824, 14 , 825, 12 , 827, 23 , 816,
34 , 805, 37 , 802, 46 , 793, 207, 632, 179, 660, 145, 694, 130, 709, 223, 616, 228, 611, 227, 612,
132, 707, 133, 706, 143, 696, 135, 704, 161, 678, 201, 638, 173, 666, 106, 733, 83 , 756, 91 , 748,
66 , 773, 53 , 786, 10 , 829, 9  , 830, 7  , 832, 8  , 831, 16 , 823, 47 , 792, 64 , 775, 57 , 782,
104, 735, 101, 738, 108, 731, 208, 631, 184, 655, 197, 642, 191, 648, 121, 718, 141, 698, 149, 690,
216, 623, 218, 621, 152, 687, 144, 695, 134, 705, 138, 701, 199, 640, 162, 677, 176, 663, 119, 720,
158, 681, 164, 675, 174, 665, 171, 668, 170, 669, 87 , 752, 169, 670, 88 , 751, 107, 732, 81 , 758,
82 , 757, 100, 739, 98 , 741, 71 , 768, 59 , 780, 65 , 774, 50 , 789, 49 , 790, 26 , 813, 17 , 822,
13 , 826, 6  , 833, 5  , 834, 33 , 806, 51 , 788, 75 , 764, 99 , 740, 96 , 743, 97 , 742, 166, 673,
172, 667, 175, 664, 187, 652, 163, 676, 185, 654, 200, 639, 114, 725, 189, 650, 115, 724, 194, 645,
195, 644, 192, 647, 182, 657, 157, 682, 156, 683, 211, 628, 154, 685, 123, 716, 139, 700, 212, 627,
153, 686, 213, 626, 215, 624, 150, 689, 225, 614, 224, 615, 221, 618, 220, 619, 127, 712, 147, 692,
124, 715, 193, 646, 205, 634, 206, 633, 116, 723, 160, 679, 186, 653, 167, 672, 79 , 760, 85 , 754,
77 , 762, 92 , 747, 58 , 781, 62 , 777, 69 , 770, 54 , 785, 36 , 803, 32 , 807, 25 , 814, 18 , 821,
11 , 828, 4  , 835, 3  , 836, 19 , 820, 22 , 817, 41 , 798, 38 , 801, 44 , 795, 52 , 787, 45 , 794,
63 , 776, 67 , 772, 72 , 767, 76 , 763, 94 , 745, 102, 737, 90 , 749, 109, 730, 165, 674, 111, 728,
209, 630, 204, 635, 117, 722, 188, 651, 159, 680, 198, 641, 113, 726, 183, 656, 180, 659, 177, 662,
196, 643, 155, 684, 214, 625, 126, 713, 131, 708, 219, 620, 222, 617, 226, 613, 230, 609, 232, 607,
262, 577, 252, 587, 418, 421, 416, 423, 413, 426, 411, 428, 376, 463, 395, 444, 283, 556, 285, 554,
379, 460, 390, 449, 363, 476, 384, 455, 388, 451, 386, 453, 361, 478, 387, 452, 360, 479, 310, 529,
354, 485, 328, 511, 315, 524, 337, 502, 349, 490, 335, 504, 324, 515, 323, 516, 320, 519, 334, 505,
359, 480, 295, 544, 385, 454, 292, 547, 291, 548, 381, 458, 399, 440, 380, 459, 397, 442, 369, 470,
377, 462, 410, 429, 407, 432, 281, 558, 414, 425, 247, 592, 277, 562, 271, 568, 272, 567, 264, 575,
259, 580, 237, 602, 239, 600, 244, 595, 243, 596, 275, 564, 278, 561, 250, 589, 246, 593, 417, 422,
248, 591, 394, 445, 393, 446, 370, 469, 365, 474, 300, 539, 299, 540, 364, 475, 362, 477, 298, 541,
312, 527, 313, 526, 314, 525, 353, 486, 352, 487, 343, 496, 327, 512, 350, 489, 326, 513, 319, 520,
332, 507, 333, 506, 348, 491, 347, 492, 322, 517, 330, 509, 338, 501, 341, 498, 340, 499, 342, 497,
301, 538, 366, 473, 401, 438, 371, 468, 408, 431, 375, 464, 249, 590, 269, 570, 238, 601, 234, 605,
257, 582, 273, 566, 255, 584, 254, 585, 245, 594, 251, 588, 412, 427, 372, 467, 282, 557, 403, 436,
396, 443, 392, 447, 391, 448, 382, 457, 389, 450, 294, 545, 297, 542, 311, 528, 344, 495, 345, 494,
318, 521, 331, 508, 325, 514, 321, 518, 346, 493, 339, 500, 351, 488, 306, 533, 289, 550, 400, 439,
378, 461, 374, 465, 415, 424, 270, 569, 241, 598, 231, 608, 260, 579, 268, 571, 276, 563, 409, 430,
398, 441, 290, 549, 304, 535, 308, 531, 358, 481, 316, 523, 293, 546, 288, 551, 284, 555, 368, 471,
253, 586, 256, 583, 263, 576, 242, 597, 274, 565, 402, 437, 383, 456, 357, 482, 329, 510, 317, 522,
307, 532, 286, 553, 287, 552, 266, 573, 261, 578, 236, 603, 303, 536, 356, 483, 355, 484, 405, 434,
404, 435, 406, 433, 235, 604, 267, 572, 302, 537, 309, 530, 265, 574, 233, 606, 367, 472, 296, 543,
336, 503, 305, 534, 373, 466, 280, 559, 279, 560, 419, 420, 240, 599, 258, 581, 229, 610
};

uint8_t compute_nr_root_seq(NR_RACH_ConfigCommon_t *rach_config,
                            uint8_t nb_preambles,
                            uint8_t unpaired,
			    frequency_range_t frequency_range) {

  uint8_t config_index = rach_config->rach_ConfigGeneric.prach_ConfigurationIndex;
  uint8_t ncs_index = rach_config->rach_ConfigGeneric.zeroCorrelationZoneConfig;
  uint16_t format0 = get_format0(config_index, unpaired, frequency_range);
  uint16_t NCS = get_NCS(ncs_index, format0, rach_config->restrictedSetConfig);
  uint16_t L_ra = (rach_config->prach_RootSequenceIndex.present==NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l139) ? 139 : 839;
  uint16_t r,u,index,q,d_u,n_shift_ra,n_shift_ra_bar,d_start;
  uint32_t w;
  uint8_t found_preambles = 0;
  uint8_t found_sequences = 0;

  if (rach_config->restrictedSetConfig == 0) {
    if (NCS == 0) return nb_preambles;
    else {
      r = L_ra/NCS;
      found_sequences = (nb_preambles/r) + (nb_preambles%r!=0); //ceil(nb_preambles/r)
      LOG_D(MAC, "Computing NR root sequences: found %u sequences\n", found_sequences);
      return (found_sequences);
    }
  }
  else{
    index = rach_config->prach_RootSequenceIndex.choice.l839;
    while (found_preambles < nb_preambles) {
      u = table_63313[index%(L_ra-1)];

      q = 0;
      while (((q*u)%L_ra) != 1) q++;
      if (q < 420) d_u = q;
      else d_u = L_ra - q;

      uint16_t n_group_ra = 0;
      if (rach_config->restrictedSetConfig == 1) {
        if ( (d_u<280) && (d_u>=NCS) ) {
          n_shift_ra     = d_u/NCS;
          d_start        = (d_u<<1) + (n_shift_ra * NCS);
          n_group_ra     = L_ra/d_start;
          n_shift_ra_bar = max(0,(L_ra-(d_u<<1)-(n_group_ra*d_start))/L_ra);
        } else if  ( (d_u>=280) && (d_u<=((L_ra - NCS)>>1)) ) {
          n_shift_ra     = (L_ra - (d_u<<1))/NCS;
          d_start        = L_ra - (d_u<<1) + (n_shift_ra * NCS);
          n_group_ra     = d_u/d_start;
          n_shift_ra_bar = min(n_shift_ra,max(0,(d_u- (n_group_ra*d_start))/NCS));
        } else {
          n_shift_ra     = 0;
          n_shift_ra_bar = 0;
        }
        w = n_shift_ra*n_group_ra + n_shift_ra_bar;
        found_preambles += w;
        found_sequences++;
      }
      else {
        AssertFatal(1==0,"Procedure to find nb of sequences for restricted type B not implemented yet");
      }
    }
    LOG_D(MAC, "Computing NR root sequences: found %u sequences\n", found_sequences);
    return found_sequences;
  }
}

// TS 38.211 Table 7.4.1.1.2-3: PDSCH DMRS positions l' within a slot for single-symbol DMRS and intra-slot frequency hopping disabled.
// The first 4 colomns are PDSCH mapping type A and the last 4 colomns are PDSCH mapping type B.
// When l' = l0, it is represented by 1
// E.g. when symbol duration is 12 in colomn 7, value 1057 ('10000100001') which means l' =  l0, 5, 10.

int32_t table_7_4_1_1_2_3_pdsch_dmrs_positions_l [13][8] = {                             // Duration in symbols
{-1,          -1,          -1,         -1,          1,          1,         1,         1},       //2              // (DMRS l' position)
{0,            0,           0,          0,          1,          1,         1,         1},       //3              // (DMRS l' position)
{0,            0,           0,          0,          1,          1,         1,         1},       //4               // (DMRS l' position)
{0,            0,           0,          0,          1,          17,        17,       17},       //5               // (DMRS l' position)
{0,            0,           0,          0,          1,          17,        17,       17},       //6               // (DMRS l' position)
{0,            0,           0,          0,          1,          17,        17,       17},       //7               // (DMRS l' position)
{0,          128,         128,        128,          1,          65,        73,       73},       //8               // (DMRS l' position)
{0,          128,         128,        128,          1,         129,       145,      145},       //9               // (DMRS l' position)
{0,          512,         576,        576,          1,         129,       145,      145},       //10              // (DMRS l' position)
{0,          512,         576,        576,          1,         257,       273,      585},       //11              // (DMRS l' position)
{0,          512,         576,       2336,          1,         513,       545,      585},       //12              // (DMRS l' position)
{0,         2048,        2176,       2336,          1,         513,       545,      585},       //13              // (DMRS l' position)
{0,         2048,        2176,       2336,         -1,          -1,       -1,        -1},       //14              // (DMRS l' position)
};


// TS 38.211 Table 7.4.1.1.2-4: PDSCH DMRS positions l' within a slot for double-symbol DMRS and intra-slot frequency hopping disabled.
// The first 4 colomns are PDSCH mapping type A and the last 4 colomns are PDSCH mapping type B.
// When l' = l0, it is represented by 1

int32_t table_7_4_1_1_2_4_pdsch_dmrs_positions_l [12][8] = {                             // Duration in symbols
{-1,          -1,          -1,         -1,         -1,         -1,        -1,         -1},       //<4              // (DMRS l' position)
{0,            0,          -1,         -1,         -1,         -1,        -1,         -1},       //4               // (DMRS l' position)
{0,            0,          -1,         -1,          3,          3,        -1,         -1},       //5               // (DMRS l' position)
{0,            0,          -1,         -1,          3,          3,        -1,         -1},       //6               // (DMRS l' position)
{0,            0,          -1,         -1,          3,          3,        -1,         -1},       //7               // (DMRS l' position)
{0,            0,          -1,         -1,          3,         99,        -1,         -1},       //8               // (DMRS l' position)
{0,            0,          -1,         -1,          3,         99,        -1,         -1},       //9               // (DMRS l' position)
{0,          768,          -1,         -1,          3,        387,        -1,         -1},       //10              // (DMRS l' position)
{0,          768,          -1,         -1,          3,        387,        -1,         -1},       //11              // (DMRS l' position)
{0,          768,          -1,         -1,          3,        771,        -1,         -1},       //12              // (DMRS l' position)
{0,         3072,          -1,         -1,          3,        771,        -1,         -1},       //13              // (DMRS l' position)
{0,         3072,          -1,         -1,          -1,        -1,        -1,         -1},       //14              // (DMRS l' position)
};

// TS 38.211 Table 6.4.1.1.3-3: PUSCH DMRS positions l' within a slot for single-symbol DMRS and intra-slot frequency hopping disabled.
// The first 4 colomns are PUSCH mapping type A and the last 4 colomns are PUSCH mapping type B.
// When l' = l0, it is represented by 1
// E.g. when symbol duration is 12 in colomn 7, value 1057 ('10000100001') which means l' =  l0, 5, 10.

int32_t table_6_4_1_1_3_3_pusch_dmrs_positions_l [12][8] = {                             // Duration in symbols
{-1,          -1,          -1,         -1,          1,          1,         1,         1},       //<4              // (DMRS l' position)
{0,            0,           0,          0,          1,          1,         1,         1},       //4               // (DMRS l' position)
{0,            0,           0,          0,          1,         17,        17,        17},       //5               // (DMRS l' position)
{0,            0,           0,          0,          1,         17,        17,        17},       //6               // (DMRS l' position)
{0,            0,           0,          0,          1,         17,        17,        17},       //7               // (DMRS l' position)
{0,          128,         128,        128,          1,         65,        73,        73},       //8               // (DMRS l' position)
{0,          128,         128,        128,          1,         65,        73,        73},       //9               // (DMRS l' position)
{0,          512,         576,        576,          1,        257,       273,       585},       //10              // (DMRS l' position)
{0,          512,         576,        576,          1,        257,       273,       585},       //11              // (DMRS l' position)
{0,          512,         576,       2336,          1,       1025,      1057,       585},       //12              // (DMRS l' position)
{0,         2048,        2176,       2336,          1,       1025,      1057,       585},       //13              // (DMRS l' position)
{0,         2048,        2176,       2336,          1,       1025,      1057,       585},       //14              // (DMRS l' position)
};


// TS 38.211 Table 6.4.1.1.3-4: PUSCH DMRS positions l' within a slot for double-symbol DMRS and intra-slot frequency hopping disabled.
// The first 4 colomns are PUSCH mapping type A and the last 4 colomns are PUSCH mapping type B.
// When l' = l0, it is represented by 1

int32_t table_6_4_1_1_3_4_pusch_dmrs_positions_l [12][8] = {                             // Duration in symbols
{-1,          -1,          -1,         -1,         -1,         -1,        -1,         -1},       //<4              // (DMRS l' position)
{0,            0,          -1,         -1,         -1,         -1,        -1,         -1},       //4               // (DMRS l' position)
{0,            0,          -1,         -1,          3,          3,        -1,         -1},       //5               // (DMRS l' position)
{0,            0,          -1,         -1,          3,          3,        -1,         -1},       //6               // (DMRS l' position)
{0,            0,          -1,         -1,          3,          3,        -1,         -1},       //7               // (DMRS l' position)
{0,            0,          -1,         -1,          3,         99,        -1,         -1},       //8               // (DMRS l' position)
{0,            0,          -1,         -1,          3,         99,        -1,         -1},       //9               // (DMRS l' position)
{0,          768,          -1,         -1,          3,        387,        -1,         -1},       //10              // (DMRS l' position)
{0,          768,          -1,         -1,          3,        387,        -1,         -1},       //11              // (DMRS l' position)
{0,          768,          -1,         -1,          3,       1539,        -1,         -1},       //12              // (DMRS l' position)
{0,         3072,          -1,         -1,          3,       1539,        -1,         -1},       //13              // (DMRS l' position)
{0,         3072,          -1,         -1,          3,       1539,        -1,         -1},       //14              // (DMRS l' position)
};


void get_delta_arfcn(int i, uint32_t nrarfcn, uint64_t N_OFFs){

  uint32_t delta_arfcn = nrarfcn - N_OFFs;

  if(delta_arfcn%(nr_bandtable[i].step_size)!=0)
    AssertFatal(1 == 0, "nrarfcn %u is not on the channel raster for step size %lu", nrarfcn, nr_bandtable[i].step_size);

}

uint32_t to_nrarfcn(int nr_bandP,
                    uint64_t dl_CarrierFreq,
                    uint8_t scs_index,
                    uint32_t bw)
{
  uint64_t dl_CarrierFreq_by_1k = dl_CarrierFreq / 1000;
  int bw_kHz = bw / 1000;
  uint32_t nrarfcn;
  int i = get_nr_table_idx(nr_bandP, scs_index);

  LOG_I(NR_MAC,"Searching for nr band %d DL Carrier frequency %llu bw %u\n",nr_bandP,(long long unsigned int)dl_CarrierFreq,bw);

  AssertFatal(dl_CarrierFreq_by_1k >= nr_bandtable[i].dl_min,
        "Band %d, bw %u : DL carrier frequency %llu kHz < %llu\n",
	      nr_bandP, bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)nr_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_1k <= (nr_bandtable[i].dl_max - bw_kHz),
        "Band %d, dl_CarrierFreq %llu bw %u: DL carrier frequency %llu kHz > %llu\n",
	      nr_bandP, (long long unsigned int)dl_CarrierFreq,bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)(nr_bandtable[i].dl_max - bw_kHz));
 
  int deltaFglobal = 60;
  uint32_t N_REF_Offs = 2016667;
  uint64_t F_REF_Offs_khz = 24250080;

  if (dl_CarrierFreq < 24.25e9) {
    deltaFglobal = 15;
    N_REF_Offs = 600000;
    F_REF_Offs_khz = 3000000;
  }
  if (dl_CarrierFreq < 3e9) {
    deltaFglobal = 5;
    N_REF_Offs = 0;
    F_REF_Offs_khz = 0;
  }   

  // This is equation before Table 5.4.2.1-1 in 38101-1-f30
  // F_REF=F_REF_Offs + deltaF_Global(N_REF-NREF_REF_Offs)
  nrarfcn =  (((dl_CarrierFreq_by_1k - F_REF_Offs_khz)/deltaFglobal)+N_REF_Offs);
  get_delta_arfcn(i, nrarfcn, nr_bandtable[i].N_OFFs_DL);

  return nrarfcn;
}

// This function computes the RF reference frequency from the NR-ARFCN according to 5.4.2.1 of 3GPP TS 38.104
// this function applies to both DL and UL
uint64_t from_nrarfcn(int nr_bandP,
                      uint8_t scs_index,
                      uint32_t nrarfcn)
{
  int deltaFglobal = 5;
  uint32_t N_REF_Offs = 0;
  uint64_t F_REF_Offs_khz = 0;
  uint64_t N_OFFs, frequency, freq_min;
  int i = get_nr_table_idx(nr_bandP, scs_index);

  if (nrarfcn > 599999 && nrarfcn < 2016667) {
    deltaFglobal = 15;
    N_REF_Offs = 600000;
    F_REF_Offs_khz = 3000000;
  }
  if (nrarfcn > 2016666 && nrarfcn < 3279166) {
    deltaFglobal = 60; 
    N_REF_Offs = 2016667;
    F_REF_Offs_khz = 24250080;
  }

  int32_t delta_duplex = get_delta_duplex(nr_bandP, scs_index);

  if (delta_duplex <= 0){ // DL band >= UL band
    if (nrarfcn >= nr_bandtable[i].N_OFFs_DL){ // is TDD of FDD DL
      N_OFFs = nr_bandtable[i].N_OFFs_DL;
      freq_min = nr_bandtable[i].dl_min;
    } else {// is FDD UL
      N_OFFs = nr_bandtable[i].N_OFFs_DL + delta_duplex/deltaFglobal;
      freq_min = nr_bandtable[i].ul_min;
    }
  } else { // UL band > DL band
    if (nrarfcn >= nr_bandtable[i].N_OFFs_DL + delta_duplex/deltaFglobal){ // is FDD UL
      N_OFFs = nr_bandtable[i].N_OFFs_DL + delta_duplex/deltaFglobal;
      freq_min = nr_bandtable[i].ul_min;
    } else { // is FDD DL
      N_OFFs = nr_bandtable[i].N_OFFs_DL;
      freq_min = nr_bandtable[i].dl_min;
    }
  }

  LOG_D(NR_MAC, "Frequency from NR-ARFCN for N_OFFs %lu, duplex spacing %d KHz, deltaFglobal %d KHz\n", N_OFFs, delta_duplex, deltaFglobal);

  AssertFatal(nrarfcn >= N_OFFs,"nrarfcn %u < N_OFFs[%d] %llu\n", nrarfcn, nr_bandtable[i].band, (long long unsigned int)N_OFFs);
  get_delta_arfcn(i, nrarfcn, N_OFFs);

  frequency = 1000*(F_REF_Offs_khz + (nrarfcn - N_REF_Offs) * deltaFglobal);

  LOG_I(NR_MAC, "Computing frequency (pointA %llu => %llu KHz (freq_min %llu KHz, NR band %d N_OFFs %llu))\n",
    (unsigned long long)nrarfcn,
    (unsigned long long)frequency/1000,
    (unsigned long long)freq_min,
    nr_bandP,
    (unsigned long long)N_OFFs);

  return frequency;

}

void nr_get_tbs_dl(nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
		   int x_overhead,
                   uint8_t numdmrscdmgroupnodata,
                   uint8_t tb_scaling) {

  LOG_D(MAC, "TBS calculation\n");

  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_rel15 = &pdsch_pdu->pdsch_pdu_rel15;
  uint16_t N_PRB_oh = x_overhead;
  uint8_t N_PRB_DMRS;
  if (pdsch_rel15->dmrsConfigType == NFAPI_NR_DMRS_TYPE1) {
    // if no data in dmrs cdm group is 1 only even REs have no data
    // if no data in dmrs cdm group is 2 both odd and even REs have no data
    N_PRB_DMRS = numdmrscdmgroupnodata*6;
  }
  else {
    N_PRB_DMRS = numdmrscdmgroupnodata*4;
  }
  uint8_t N_sh_symb = pdsch_rel15->NrOfSymbols;
  uint8_t Imcs = pdsch_rel15->mcsIndex[0];
  uint16_t dmrs_length = get_num_dmrs(pdsch_rel15->dlDmrsSymbPos);
  uint16_t N_RE_prime = NR_NB_SC_PER_RB*N_sh_symb - N_PRB_DMRS*dmrs_length - N_PRB_oh;
  LOG_D(MAC, "N_RE_prime %d for %d symbols %d DMRS per PRB and %d overhead\n", N_RE_prime, N_sh_symb, N_PRB_DMRS, N_PRB_oh);

  uint16_t R;
  uint32_t TBS=0;
  uint8_t table_idx, Qm;

  /*uint8_t mcs_table = config.pdsch_config.mcs_table.value;
  uint8_t ss_type = params_rel15.search_space_type;
  uint8_t dci_format = params_rel15.dci_format;
  get_table_idx(mcs_table, dci_format, rnti_type, ss_type);*/
  table_idx = 0;
  R = nr_get_code_rate_dl(Imcs, table_idx);
  Qm = nr_get_Qm_dl(Imcs, table_idx);

  TBS = nr_compute_tbs(Qm,
                       R,
                       pdsch_rel15->rbSize,
                       N_sh_symb,
                       N_PRB_DMRS*dmrs_length,
                       N_PRB_oh,
                       tb_scaling,
		       pdsch_rel15->nrOfLayers)>>3;

  pdsch_rel15->targetCodeRate[0] = R;
  pdsch_rel15->qamModOrder[0] = Qm;
  pdsch_rel15->TBSize[0] = TBS;
  //  pdsch_rel15->nb_mod_symbols = N_RE_prime*pdsch_rel15->n_prb*pdsch_rel15->nb_codewords;
  pdsch_rel15->mcsTable[0] = table_idx;

  LOG_D(MAC, "TBS %d bytes: N_PRB_DMRS %d N_sh_symb %d N_PRB_oh %d R %d Qm %d table %d nb_symbols %d\n",
  TBS, N_PRB_DMRS, N_sh_symb, N_PRB_oh, R, Qm, table_idx,N_RE_prime*pdsch_rel15->rbSize*pdsch_rel15->NrOfCodewords );
}

//Table 5.1.3.1-1 of 38.214
uint16_t Table_51311[29][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};

//Table 5.1.3.1-2 of 38.214
// Imcs values 20 and 26 have been multiplied by 2 to avoid the floating point
uint16_t Table_51312[28][2] = {{2,120},{2,193},{2,308},{2,449},{2,602},{4,378},{4,434},{4,490},{4,553},{4,616},{4,658},{6,466},{6,517},{6,567},{6,616},{6,666},
		{6,719},{6,772},{6,822},{6,873},{8,1365},{8,711},{8,754},{8,797},{8,841},{8,885},{8,1833},{8,948}};

//Table 5.1.3.1-3 of 38.214
uint16_t Table_51313[29][2] = {{2,30},{2,40},{2,50},{2,64},{2,78},{2,99},{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{4,340},
		{4,378},{4,434},{4,490},{4,553},{4,616},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666}, {6,719}, {6,772}};

uint16_t Table_61411[28][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};

uint16_t Table_61412[28][2] = {{2,30},{2,40},{2,50},{2,64},{2,78},{2,99},{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},
		{4,378},{4,434},{4,490},{4,553},{4,616},{4,658},{4,699},{4,772},{6,567},{6,616},{6,666}, {6,772}};



uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 0 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51311[Imcs][0]);
    break;

    case 1:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 1 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_51312[Imcs][0]);
    break;

    case 2:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 2 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51313[Imcs][0]);
    break;

    default:
      LOG_E(MAC, "Invalid MCS table index %d (expected in range [0,2])\n", table_idx);
      return 0;
  }
}

uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 0 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51311[Imcs][1]);
    break;

    case 1:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 1 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_51312[Imcs][1]);
    break;

    case 2:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 2 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51313[Imcs][1]);
    break;

    default:
      LOG_E(MAC, "Invalid MCS table index %d (expected in range [0,2])\n", table_idx);
      return 0;
  }
}

uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 0 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51311[Imcs][0]);
    break;

    case 1:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 1 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_51312[Imcs][0]);
    break;

    case 2:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 2 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51313[Imcs][0]);
    break;

    case 3:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 3 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_61411[Imcs][0]);
    break;

    case 4:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 4 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_61412[Imcs][0]);
    break;

    default:
      LOG_E(MAC, "Invalid MCS table index %d (expected in range [0,4])\n", table_idx);
      return 0;
  }
}

uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 0 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51311[Imcs][1]);
    break;

    case 1:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 1 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_51312[Imcs][1]);
    break;

    case 2:
      if (Imcs > 28) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 2 (expected range [0,28])\n", Imcs);
        return 0;
      }
      return (Table_51313[Imcs][1]);
    break;

    case 3:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 3 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_61411[Imcs][1]);
    break;

    case 4:
      if (Imcs > 27) {
        LOG_E(MAC, "Invalid MCS index %d for MCS table 4 (expected range [0,27])\n", Imcs);
        return 0;
      }
      return (Table_61412[Imcs][1]);
    break;

    default:
      LOG_E(MAC, "Invalid MCS table index %d (expected in range [0,4])\n", table_idx);
      return 0;
  }
}

static inline uint8_t is_codeword_disabled(uint8_t format, uint8_t Imcs, uint8_t rv) {
  return ((format==NFAPI_NR_DL_DCI_FORMAT_1_1)&&(Imcs==26)&&(rv==1));
}

static inline uint8_t get_table_idx(uint8_t mcs_table, uint8_t dci_format, uint8_t rnti_type, uint8_t ss_type) {
  if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM256) && (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1) && ((rnti_type==NFAPI_NR_RNTI_C)||(rnti_type==NFAPI_NR_RNTI_CS)))
    return 2;
  else if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM64_LOW_SE) && (rnti_type!=NFAPI_NR_RNTI_new) && (rnti_type==NFAPI_NR_RNTI_C) && (ss_type==NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC))
    return 3;
  else if (rnti_type==NFAPI_NR_RNTI_new)
    return 3;
  else if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM256) && (rnti_type==NFAPI_NR_RNTI_CS) && (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1))
    return 2; // Condition mcs_table not configured in sps_config necessary here but not yet implemented
  /*else if((mcs_table == NFAPI_NR_MCS_TABLE_QAM64_LOW_SE) &&  (rnti_type==NFAPI_NR_RNTI_CS))
   *  table_idx = 3;
   * Note: the commented block refers to the case where the mcs_table is from sps_config*/
  else
    return 1;
}


/* returns the total DMRS symbols in a slot*/
uint8_t get_num_dmrs_symbols(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols, int startSymbol, int mappingtype){
  return get_num_dmrs(fill_dmrs_mask(pdsch_Config,dmrs_TypeA_Position,NrOfSymbols, startSymbol, mappingtype));
}

// Table 5.1.2.2.1-1 38.214
uint8_t getRBGSize(uint16_t bwp_size, long rbg_size_config) {
  
  AssertFatal(bwp_size<276,"BWP Size > 275\n");
  
  if (bwp_size < 37)  return (rbg_size_config ? 4 : 2);
  if (bwp_size < 73)  return (rbg_size_config ? 8 : 4);
  if (bwp_size < 145) return (rbg_size_config ? 16 : 8);
  else return 16;
}

uint8_t getNRBG(uint16_t bwp_size, uint16_t bwp_start, long rbg_size_config) {

  uint8_t rbg_size = getRBGSize(bwp_size,rbg_size_config);

  return (uint8_t)ceil((bwp_size+(bwp_start % rbg_size))/rbg_size);
}

uint8_t getAntPortBitWidth(NR_SetupRelease_DMRS_DownlinkConfig_t *typeA, NR_SetupRelease_DMRS_DownlinkConfig_t *typeB) {

  uint8_t nbitsA = 0;
  uint8_t nbitsB = 0;
  uint8_t type,length,nbits;

  if (typeA != NULL) {
    type = (typeA->choice.setup->dmrs_Type==NULL) ? 1:2;
    length = (typeA->choice.setup->maxLength==NULL) ? 1:2;
    nbitsA = type + length + 2;
    if (typeB == NULL) return nbitsA;
  }
  if (typeB != NULL) {
    type = (typeB->choice.setup->dmrs_Type==NULL) ? 1:2;
    length = (typeB->choice.setup->maxLength==NULL) ? 1:2;
    nbitsB = type + length + 2;
    if (typeA == NULL) return nbitsB;
  }

  nbits = (nbitsA > nbitsB) ? nbitsA : nbitsB;
  return nbits;
}


/*******************************************************************
*
* NAME :         get_l0_ul
*
* PARAMETERS :   mapping_type : PUSCH mapping type
*                dmrs_typeA_position  : higher layer parameter
*
* RETURN :       demodulation reference signal for PUSCH
*
* DESCRIPTION :  see TS 38.211 V15.4.0 Demodulation reference signals for PUSCH
*
*********************************************************************/

uint8_t get_l0_ul(uint8_t mapping_type, uint8_t dmrs_typeA_position) {

  return ((mapping_type==typeA)?dmrs_typeA_position:0);

}

int32_t get_l_prime(uint8_t duration_in_symbols, uint8_t mapping_type, pusch_dmrs_AdditionalPosition_t additional_pos, pusch_maxLength_t pusch_maxLength, uint8_t start_symbol, uint8_t dmrs_typeA_position) {

  uint8_t row, colomn;
  int32_t l_prime;

  LOG_D(NR_MAC, "In %s: PUSCH NrofSymbols:%d, startSymbol:%d, mappingtype:%d, dmrs_TypeA_Position:%d additional_pos:%d, pusch_maxLength:%d\n",
    __FUNCTION__,
    duration_in_symbols,
    start_symbol,
    mapping_type,
    dmrs_typeA_position,
    additional_pos,
    pusch_maxLength);

  // Section 6.4.1.1.3 in Spec 38.211
  // For PDSCH Mapping TypeA, ld is duration between first OFDM of the slot and last OFDM symbol of the scheduled PUSCH resources
  // For TypeB, ld is the duration of the scheduled PUSCH resources
  uint8_t ld = (mapping_type == typeA) ? (duration_in_symbols + start_symbol) : duration_in_symbols;
  uint8_t l0 = (dmrs_typeA_position == NR_MIB__dmrs_TypeA_Position_pos2) ? 2 : 3 ;

  colomn = additional_pos;

  if (mapping_type == typeB)
    colomn += 4;

  if (ld < 4)
    row = 0;
  else
    row = ld - 3;

  if (pusch_maxLength == pusch_len1) {
    l_prime = table_6_4_1_1_3_3_pusch_dmrs_positions_l[row][colomn];
    l0 = 1 << l0;
  }
  else {
    l_prime = table_6_4_1_1_3_4_pusch_dmrs_positions_l[row][colomn];
    l0 = 1<<l0 | 1<<(l0+1);
  }

  LOG_D(NR_MAC, "PUSCH - l0:%d, ld:%d,row:%d, column:%d, addpos:%d, maxlen:%d\n", l0, ld, row, colomn, additional_pos, pusch_maxLength);
  AssertFatal(l_prime>=0,"invalid l_prime < 0\n");

  l_prime = (mapping_type == typeA) ? (l_prime | l0) : (l_prime << start_symbol);
  LOG_D(MAC, " PUSCH DMRS MASK in HEX:%x\n", l_prime);

  return l_prime;

}

/*******************************************************************
*
* NAME :         get_L_ptrs
*
* PARAMETERS :   mcs(i)                 higher layer parameter in PTRS-UplinkConfig
*                I_mcs                  MCS index used for PUSCH
*                mcs_table              0 for table 5.1.3.1-1, 1 for table 5.1.3.1-1
*
* RETURN :       the parameter L_ptrs
*
* DESCRIPTION :  3GPP TS 38.214 section 6.2.3.1
*
*********************************************************************/

uint8_t get_L_ptrs(uint8_t mcs1, uint8_t mcs2, uint8_t mcs3, uint8_t I_mcs, uint8_t mcs_table) {

  uint8_t mcs4;

  if(mcs_table == 0)
    mcs4 = 29;
  else
    mcs4 = 28;

  if (I_mcs < mcs1) {
    LOG_D(PHY, "PUSH PT-RS is not present.\n");
    return -1;
  } else if (I_mcs >= mcs1 && I_mcs < mcs2)
    return 2;
  else if (I_mcs >= mcs2 && I_mcs < mcs3)
    return 1;
  else if (I_mcs >= mcs3 && I_mcs < mcs4)
    return 0;
  else {
    LOG_D(NR_MAC, "PT-RS time-density determination is obtained from the DCI for the same transport block in the initial transmission\n");
    return -1;
  }
}

/*******************************************************************
*
* NAME :         get_K_ptrs
*
* PARAMETERS :   nrb0, nrb1             PTRS uplink configuration
*                N_RB                   number of RBs scheduled for PUSCH
*
* RETURN :       the parameter K_ptrs
*
* DESCRIPTION :  3GPP TS 38.214 6.2.3 Table 6.2.3.1-2
*
*********************************************************************/

uint8_t get_K_ptrs(uint16_t nrb0, uint16_t nrb1, uint16_t N_RB) {

  if (N_RB < nrb0) {
    LOG_D(PHY,"PUSH PT-RS is not present.\n");
    return -1;
  } else if (N_RB >= nrb0 && N_RB < nrb1)
    return 2;
  else
    return 4;
}

// Set the transform precoding status according to 6.1.3 of 3GPP TS 38.214 version 16.3.0 Release 16:
// - "UE procedure for applying transform precoding on PUSCH"
uint8_t get_transformPrecoding(const NR_BWP_UplinkCommon_t *initialUplinkBWP,
                               const NR_PUSCH_Config_t *pusch_config,
                               const NR_BWP_UplinkDedicated_t *ubwp,
                               uint8_t *dci_format,
                               int rnti_type,
                               uint8_t configuredGrant){

  if (configuredGrant) {
    if (ubwp->configuredGrantConfig) {
      if (ubwp->configuredGrantConfig->choice.setup->transformPrecoder) {
        return *ubwp->configuredGrantConfig->choice.setup->transformPrecoder;
      }
    }
  }

  if (rnti_type != NR_RNTI_RA && rnti_type != NR_RNTI_TC) {
    if (*dci_format != NR_UL_DCI_FORMAT_0_0) {
      if (pusch_config && pusch_config->transformPrecoder != NULL) {
        return *pusch_config->transformPrecoder;
      }
    }
  }

  if (initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL) {
    return 1; // Transformprecoding disabled
  } else {
    LOG_D(PHY, "MAC_COMMON: Transform Precodig enabled through msg3_transformPrecoder\n");
    return 0; // Enabled
  }

  LOG_E(MAC, "In %s: could not fetch transform precoder status...\n", __FUNCTION__);
  return -1;
}

uint16_t nr_dci_size(const NR_BWP_DownlinkCommon_t *initialDownlinkBWP,
                     const NR_BWP_UplinkCommon_t *initialUplinkBWP,
                     const NR_CellGroupConfig_t *cg,
                     dci_pdu_rel15_t *dci_pdu,
                     nr_dci_format_t format,
		     nr_rnti_type_t rnti_type,
		     uint16_t N_RB,
                     int bwp_id) {

  uint16_t size = 0;
  uint16_t numRBG = 0;
  long rbg_size_config;
  int num_entries = 0;
  int pusch_antenna_ports = 1; // TODO hardcoded number of antenna ports for pusch

  const NR_BWP_DownlinkDedicated_t *bwpd = NULL;
  const NR_BWP_UplinkDedicated_t *ubwpd = NULL;
  const NR_BWP_DownlinkCommon_t *bwpc = NULL;
  const NR_BWP_UplinkCommon_t *ubwpc = NULL;
  const NR_PDSCH_Config_t *pdsch_Config = NULL;
  const NR_PUSCH_Config_t *pusch_Config = NULL;
  const NR_PUCCH_Config_t *pucch_Config = NULL;
  const NR_PDCCH_Config_t *pdcch_Config = NULL;
  const NR_SRS_Config_t *srs_config = NULL;
  if(bwp_id > 0) {
    AssertFatal(cg!=NULL,"Cellgroup is null and bwp_id!=0");
    bwpd=cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Dedicated;
    bwpc=cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Common;
    ubwpd=cg->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Dedicated;
    ubwpc=cg->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Common;
    pdsch_Config = (bwpd->pdsch_Config) ? bwpd->pdsch_Config->choice.setup : NULL;
    pdcch_Config = (bwpd->pdcch_Config) ? bwpd->pdcch_Config->choice.setup : NULL;
    pucch_Config = (ubwpd->pucch_Config) ? ubwpd->pucch_Config->choice.setup : NULL;
    pusch_Config = (ubwpd->pusch_Config) ? ubwpd->pusch_Config->choice.setup : NULL;
    srs_config = (ubwpd->srs_Config) ? ubwpd->srs_Config->choice.setup : NULL;
  }
  else if (cg){
    bwpd=cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP;
    bwpc=initialDownlinkBWP;
    ubwpd=cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP; 
    ubwpc=initialUplinkBWP;
    pdsch_Config = (bwpd->pdsch_Config) ? bwpd->pdsch_Config->choice.setup : NULL;
    pdcch_Config = (bwpd->pdcch_Config) ? bwpd->pdcch_Config->choice.setup : NULL;
    pucch_Config = (ubwpd->pucch_Config) ? ubwpd->pucch_Config->choice.setup : NULL;
    pusch_Config = (ubwpd->pusch_Config) ? ubwpd->pusch_Config->choice.setup :  NULL;
    srs_config = (ubwpd->srs_Config) ? ubwpd->srs_Config->choice.setup: NULL;
  }

  int n_ul_bwp=1,n_dl_bwp=1;
  switch(format) {
    /*Only sizes for 0_0 and 1_0 are correct at the moment*/
    case NR_UL_DCI_FORMAT_0_0:
      /// fixed: Format identifier 1, Hop flag 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2 Time Domain assgnmt 4 --20
      size += 20;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment -- hopping scenario to be updated
      size += nr_dci_size(initialDownlinkBWP,initialUplinkBWP,cg,dci_pdu,NR_DL_DCI_FORMAT_1_0, rnti_type, N_RB, bwp_id) - size; // Padding to match 1_0 size
      // UL/SUL indicator assumed to be 0
      break;

    case NR_UL_DCI_FORMAT_0_1:
      /// fixed: Format identifier 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2, ULSCH indicator 1 --16
      size += 16;
      // Carrier indicator
      if (cg->spCellConfig->spCellConfigDedicated->crossCarrierSchedulingConfig != NULL) {
        dci_pdu->carrier_indicator.nbits=3;
        size += dci_pdu->carrier_indicator.nbits;
      }
      // UL/SUL indicator
      if (cg->spCellConfig->spCellConfigDedicated->supplementaryUplink != NULL) {
        dci_pdu->carrier_indicator.nbits=1;
        size += dci_pdu->ul_sul_indicator.nbits;
      }
      // BWP Indicator
      n_ul_bwp = 0;
      if (cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
          cg->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList)
         n_ul_bwp = cg->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count;
      if (n_ul_bwp < 2)
        dci_pdu->bwp_indicator.nbits = n_ul_bwp;
      else
        dci_pdu->bwp_indicator.nbits = 2;
      size += dci_pdu->bwp_indicator.nbits;
      // Freq domain assignment
      if (pusch_Config) {
        if (pusch_Config->rbg_Size != NULL)
          rbg_size_config = 1;
        else
          rbg_size_config = 0;
        numRBG = getNRBG(NRRIV2BW(ubwpc->genericParameters.locationAndBandwidth, MAX_BWP_SIZE),
                         NRRIV2PRBOFFSET(ubwpc->genericParameters.locationAndBandwidth, MAX_BWP_SIZE),
                         rbg_size_config);
        if (pusch_Config->resourceAllocation == 0)
          dci_pdu->frequency_domain_assignment.nbits = numRBG;
        else if (pusch_Config->resourceAllocation == 1)
          dci_pdu->frequency_domain_assignment.nbits = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
        else
          dci_pdu->frequency_domain_assignment.nbits = ((int)ceil( log2( (N_RB*(N_RB+1))>>1 ) )>numRBG) ? (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) )+1 : numRBG+1;
      }
      else dci_pdu->frequency_domain_assignment.nbits = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      LOG_D(NR_MAC,"PUSCH Frequency Domain Assignment nbits %d, N_RB %d\n",dci_pdu->frequency_domain_assignment.nbits,N_RB);
      size += dci_pdu->frequency_domain_assignment.nbits;
      // Time domain assignment
      if (pusch_Config==NULL || pusch_Config->pusch_TimeDomainAllocationList==NULL) {
        if (ubwpc->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList==NULL)
          num_entries = 16; // num of entries in default table
        else
          num_entries = ubwpc->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.count;
      }
      else
        num_entries = pusch_Config->pusch_TimeDomainAllocationList->choice.setup->list.count;
      dci_pdu->time_domain_assignment.nbits = (int)ceil(log2(num_entries));
      LOG_D(NR_MAC,"PUSCH Time Domain Allocation nbits %d, pusch_Config %p\n",dci_pdu->time_domain_assignment.nbits,pusch_Config);
      size += dci_pdu->time_domain_assignment.nbits;
      // Frequency Hopping flag
      if (pusch_Config && 
          pusch_Config->frequencyHopping!=NULL && 
          pusch_Config->resourceAllocation != NR_PUSCH_Config__resourceAllocation_resourceAllocationType0) {
        dci_pdu->frequency_hopping_flag.nbits = 1;
        size += 1;
      }
      // 1st DAI
      if (cg->physicalCellGroupConfig &&
          cg->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook==NR_PhysicalCellGroupConfig__pdsch_HARQ_ACK_Codebook_dynamic)
        dci_pdu->dai[0].nbits = 2;
      else
        dci_pdu->dai[0].nbits = 1;
      size += dci_pdu->dai[0].nbits;
      LOG_D(NR_MAC,"DAI1 nbits %d\n",dci_pdu->dai[0].nbits);
      // 2nd DAI
      if (cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig && 
          cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission != NULL) { //TODO not sure about that
        dci_pdu->dai[1].nbits = 2;
        size += dci_pdu->dai[1].nbits;
      }
      // SRS resource indicator
      if (srs_config &&
          pusch_Config && 
          pusch_Config->txConfig != NULL){
        int count=0;
        if (*pusch_Config->txConfig == NR_PUSCH_Config__txConfig_codebook){
          for (int i=0; i<srs_config->srs_ResourceSetToAddModList->list.count; i++) {
            if (srs_config->srs_ResourceSetToAddModList->list.array[i]->usage == NR_SRS_ResourceSet__usage_codebook)
              count++;
          }
          if (count>1) {
            dci_pdu->srs_resource_indicator.nbits = 1;
            size += dci_pdu->srs_resource_indicator.nbits;
          }
        }
        else {
          int lmin,Lmax = 0;
          int lsum = 0;
          if ( cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
               cg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig != NULL) {
            if ( cg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->ext1->maxMIMO_Layers != NULL)
              Lmax = *cg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->ext1->maxMIMO_Layers;
            else
              AssertFatal(1==0,"MIMO on PUSCH not supported, maxMIMO_Layers needs to be set to 1\n");
          }
          else
            AssertFatal(1==0,"MIMO on PUSCH not supported, maxMIMO_Layers needs to be set to 1\n");
          for (int i=0; i<srs_config->srs_ResourceSetToAddModList->list.count; i++) {
            if (srs_config->srs_ResourceSetToAddModList->list.array[i]->usage == NR_SRS_ResourceSet__usage_nonCodebook)
              count++;
            if (count < Lmax) lmin = count;
            else lmin = Lmax;
            for (int k=1;k<=lmin;k++) {
              lsum += binomial(count,k);
            }
          }
          dci_pdu->srs_resource_indicator.nbits = (int)ceil(log2(lsum));
          size += dci_pdu->srs_resource_indicator.nbits;
        }
      } else dci_pdu->srs_resource_indicator.nbits = 0;
      LOG_D(NR_MAC,"dci_pdu->srs_resource_indicator.nbits %d\n",dci_pdu->srs_resource_indicator.nbits);
      // Precoding info and number of layers
      long transformPrecoder = get_transformPrecoding(initialUplinkBWP, pusch_Config, ubwpd, (uint8_t*)&format, rnti_type, 0);
      dci_pdu->precoding_information.nbits=0;
      if (pusch_Config && 
          pusch_Config->txConfig != NULL){
        if (*pusch_Config->txConfig == NR_PUSCH_Config__txConfig_codebook){
          if (pusch_antenna_ports > 1) {
            if (pusch_antenna_ports == 4) {
              if ((transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled) && (*pusch_Config->maxRank>1))
                dci_pdu->precoding_information.nbits = 6-(*pusch_Config->codebookSubset);
              else {
                if(*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_nonCoherent)
                  dci_pdu->precoding_information.nbits = 2;
                else
                  dci_pdu->precoding_information.nbits = 5-(*pusch_Config->codebookSubset);
              }
            }
            else {
              AssertFatal(pusch_antenna_ports==2,"Not valid number of antenna ports");
              if ((transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled) && (*pusch_Config->maxRank==2))
                dci_pdu->precoding_information.nbits = 4-(*pusch_Config->codebookSubset);
              else
                dci_pdu->precoding_information.nbits = 3-(*pusch_Config->codebookSubset);
            }
          }
        }
      }
      size += dci_pdu->precoding_information.nbits;
      LOG_D(NR_MAC,"dci_pdu->precoding_informaiton.nbits=%d\n",dci_pdu->precoding_information.nbits);
      // Antenna ports
      NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig = NULL;
      int xa=0;
      int xb=0;
      if(pusch_Config &&
         pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA != NULL){
        NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup;
        xa = ul_ant_bits(NR_DMRS_UplinkConfig,transformPrecoder);
      }
      if(pusch_Config &&
         pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB != NULL){
        NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup;
        xb = ul_ant_bits(NR_DMRS_UplinkConfig,transformPrecoder);
      }
      if (xa>xb)
        dci_pdu->antenna_ports.nbits = xa;
      else
        dci_pdu->antenna_ports.nbits = xb;
      size += dci_pdu->antenna_ports.nbits;
      LOG_D(NR_MAC,"dci_pdu->antenna_ports.nbits = %d\n",dci_pdu->antenna_ports.nbits);
      // SRS request
      if (cg->spCellConfig->spCellConfigDedicated->supplementaryUplink==NULL)
        dci_pdu->srs_request.nbits = 2;
      else
        dci_pdu->srs_request.nbits = 3;
      size += dci_pdu->srs_request.nbits;
      // CSI request
      if (cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig != NULL) {
        if (cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup->reportTriggerSize != NULL) {
          dci_pdu->csi_request.nbits = *cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup->reportTriggerSize;
          size += dci_pdu->csi_request.nbits;
        }
      }
      // CBGTI
      if (cg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig &&
          cg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->codeBlockGroupTransmission != NULL) {
        int num = cg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->codeBlockGroupTransmission->choice.setup->maxCodeBlockGroupsPerTransportBlock;
        dci_pdu->cbgti.nbits = 2 + (num<<1);
        size += dci_pdu->cbgti.nbits;
      }
      // PTRS - DMRS association
      if ( (NR_DMRS_UplinkConfig && 
            NR_DMRS_UplinkConfig->phaseTrackingRS == NULL && 
            transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled) ||
           transformPrecoder == NR_PUSCH_Config__transformPrecoder_enabled || 
           (pusch_Config && pusch_Config->maxRank &&
            *pusch_Config->maxRank==1) )
        dci_pdu->ptrs_dmrs_association.nbits = 0;
      else
        dci_pdu->ptrs_dmrs_association.nbits = 2;
      size += dci_pdu->ptrs_dmrs_association.nbits;
      // beta offset indicator
      if (pusch_Config &&
          pusch_Config->uci_OnPUSCH!=NULL){
        if (pusch_Config->uci_OnPUSCH->choice.setup->betaOffsets->present == NR_UCI_OnPUSCH__betaOffsets_PR_dynamic) {
          dci_pdu->beta_offset_indicator.nbits = 2;
          size += dci_pdu->beta_offset_indicator.nbits;
        }
      }
      // DMRS sequence init
      if (transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled) {
         dci_pdu->dmrs_sequence_initialization.nbits = 1;
         size += dci_pdu->dmrs_sequence_initialization.nbits;
      }
      break;

    case NR_DL_DCI_FORMAT_1_0:
      /// fixed: Format identifier 1, VRB2PRB 1, MCS 5, NDI 1, RV 2, HARQ PID 4, DAI 2, PUCCH TPC 2, PUCCH RInd 3, PDSCH to HARQ TInd 3 Time Domain assgnmt 4 -- 28
      size = 28;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment

      dci_pdu->frequency_domain_assignment.nbits = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      dci_pdu->time_domain_assignment.nbits = 4;
      dci_pdu->vrb_to_prb_mapping.nbits = 1;
      break;

    case NR_DL_DCI_FORMAT_1_1:
      LOG_D(NR_MAC,"DCI_FORMAT 1_1 : pdsch_Config %p, pucch_Config %p\n",pdsch_Config,pucch_Config);
      // General note: 0 bits condition is ignored as default nbits is 0.
      // Format identifier
      size = 1;
      // Carrier indicator
      if (cg->spCellConfig->spCellConfigDedicated->crossCarrierSchedulingConfig != NULL) {
        dci_pdu->carrier_indicator.nbits=3;
        size += dci_pdu->carrier_indicator.nbits;
      }

      // BWP Indicator
      n_dl_bwp = 0;
      if (cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList)
         n_dl_bwp = cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count;
      if (n_dl_bwp < 2)
        dci_pdu->bwp_indicator.nbits = n_dl_bwp;
      else
        dci_pdu->bwp_indicator.nbits = 2;
      size += dci_pdu->bwp_indicator.nbits;
      // Freq domain assignment
      if (pdsch_Config) rbg_size_config = pdsch_Config->rbg_Size;
      else rbg_size_config = 0;
      
      numRBG = getNRBG(NRRIV2BW(bwpc->genericParameters.locationAndBandwidth, MAX_BWP_SIZE),
                       NRRIV2PRBOFFSET(bwpc->genericParameters.locationAndBandwidth, MAX_BWP_SIZE),
                       rbg_size_config);
      if (pdsch_Config && pdsch_Config->resourceAllocation == 0)
         dci_pdu->frequency_domain_assignment.nbits = numRBG;
      else if (pdsch_Config == NULL || pdsch_Config->resourceAllocation == 1)
         dci_pdu->frequency_domain_assignment.nbits = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      else
         dci_pdu->frequency_domain_assignment.nbits = ((int)ceil( log2( (N_RB*(N_RB+1))>>1 ) )>numRBG) ? (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) )+1 : numRBG+1;
      size += dci_pdu->frequency_domain_assignment.nbits;
      LOG_D(NR_MAC,"dci_pdu->frequency_domain_assignment.nbits %d (N_RB %d)\n",dci_pdu->frequency_domain_assignment.nbits,N_RB);
      // Time domain assignment (see table 5.1.2.1.1-1 in 38.214
      if (pdsch_Config == NULL || pdsch_Config->pdsch_TimeDomainAllocationList==NULL) {
        if (bwpc->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList==NULL)
          num_entries = 16; // num of entries in default table
        else
          num_entries = bwpc->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
      }
      else
        num_entries = pdsch_Config->pdsch_TimeDomainAllocationList->choice.setup->list.count;
      dci_pdu->time_domain_assignment.nbits = (int)ceil(log2(num_entries));
      LOG_D(NR_MAC,"pdsch tda.nbits= %d\n",dci_pdu->time_domain_assignment.nbits);
      size += dci_pdu->time_domain_assignment.nbits;
      // VRB to PRB mapping 
      if (pdsch_Config && 
          pdsch_Config->resourceAllocation == 1 && 
          pdsch_Config->vrb_ToPRB_Interleaver != NULL) {
        dci_pdu->vrb_to_prb_mapping.nbits = 1;
        size += dci_pdu->vrb_to_prb_mapping.nbits;
      }
      // PRB bundling size indicator
      if (pdsch_Config && 
          pdsch_Config->prb_BundlingType.present == NR_PDSCH_Config__prb_BundlingType_PR_dynamicBundling) {
        dci_pdu->prb_bundling_size_indicator.nbits = 1;
        size += dci_pdu->prb_bundling_size_indicator.nbits;
      }
      // Rate matching indicator
      NR_RateMatchPatternGroup_t *group1 = pdsch_Config ? pdsch_Config->rateMatchPatternGroup1 : NULL;
      NR_RateMatchPatternGroup_t *group2 = pdsch_Config ? pdsch_Config->rateMatchPatternGroup2 : NULL;
      if ((group1 != NULL) && (group2 != NULL))
        dci_pdu->rate_matching_indicator.nbits = 2;
      if ((group1 != NULL) != (group2 != NULL))
        dci_pdu->rate_matching_indicator.nbits = 1;
      size += dci_pdu->rate_matching_indicator.nbits;
      // ZP CSI-RS trigger
      if (pdsch_Config && 
          pdsch_Config->aperiodic_ZP_CSI_RS_ResourceSetsToAddModList != NULL) {
        uint8_t nZP = pdsch_Config->aperiodic_ZP_CSI_RS_ResourceSetsToAddModList->list.count;
        dci_pdu->zp_csi_rs_trigger.nbits = (int)ceil(log2(nZP+1));
      }
      size += dci_pdu->zp_csi_rs_trigger.nbits;
      // TB1- MCS 5, NDI 1, RV 2
      size += 8;
      // TB2
      long *maxCWperDCI = pdsch_Config ? pdsch_Config->maxNrofCodeWordsScheduledByDCI : NULL;
      if ((maxCWperDCI != NULL) && (*maxCWperDCI == 2)) {
        size += 8;
      }
      // HARQ PID
      size += 4;
      // DAI
      if (cg->physicalCellGroupConfig &&
          cg->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook == NR_PhysicalCellGroupConfig__pdsch_HARQ_ACK_Codebook_dynamic) { // FIXME in case of more than one serving cell
        dci_pdu->dai[0].nbits = 2;
        size += dci_pdu->dai[0].nbits;
      }
      LOG_D(NR_MAC,"dci_pdu->dai[0].nbits %d\n",dci_pdu->dai[0].nbits);
      // TPC PUCCH
      size += 2;
      // PUCCH resource indicator
      size += 3;
      // PDSCH to HARQ timing indicator
      uint8_t I = pucch_Config->dl_DataToUL_ACK ? pucch_Config->dl_DataToUL_ACK->list.count : 8;
      dci_pdu->pdsch_to_harq_feedback_timing_indicator.nbits = (int)ceil(log2(I));
      size += dci_pdu->pdsch_to_harq_feedback_timing_indicator.nbits;
      LOG_D(NR_MAC,"dci_pdu->pdsch_to_harq_feedback_timing_indicator.nbits %d\n",dci_pdu->pdsch_to_harq_feedback_timing_indicator.nbits);
      // Antenna ports
      NR_SetupRelease_DMRS_DownlinkConfig_t *typeA = pdsch_Config ? pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA : NULL;
      NR_SetupRelease_DMRS_DownlinkConfig_t *typeB = pdsch_Config ? pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB : NULL;
      AssertFatal(typeA!=NULL || typeB!=NULL, "either dmrs_typeA or typeB must be configured\n");
      dci_pdu->antenna_ports.nbits = getAntPortBitWidth(typeA,typeB);
      size += dci_pdu->antenna_ports.nbits;
      LOG_D(NR_MAC,"dci_pdu->antenna_ports.nbits %d\n",dci_pdu->antenna_ports.nbits);
      // Tx Config Indication
      long *isTciEnable = pdcch_Config->controlResourceSetToAddModList->list.array[0]->tci_PresentInDCI;
      if (isTciEnable != NULL) {
        dci_pdu->transmission_configuration_indication.nbits = 3;
        size += dci_pdu->transmission_configuration_indication.nbits;
      }
      // SRS request
      if (cg->spCellConfig->spCellConfigDedicated->supplementaryUplink==NULL)
        dci_pdu->srs_request.nbits = 2;
      else
        dci_pdu->srs_request.nbits = 3;
      size += dci_pdu->srs_request.nbits;
      // CBGTI
      if (cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig&&
          cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission != NULL) {
        uint8_t maxCBGperTB = (cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission->choice.setup->maxCodeBlockGroupsPerTransportBlock + 1) * 2;
        long *maxCWperDCI_rrc = pdsch_Config->maxNrofCodeWordsScheduledByDCI;
        uint8_t maxCW = (maxCWperDCI_rrc == NULL) ? 1 : *maxCWperDCI_rrc;
        dci_pdu->cbgti.nbits = maxCBGperTB * maxCW;
        size += dci_pdu->cbgti.nbits;
        // CBGFI
        if (cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission->choice.setup->codeBlockGroupFlushIndicator) {
          dci_pdu->cbgfi.nbits = 1;
          size += dci_pdu->cbgfi.nbits;
        }
      }
      // DMRS sequence init
      size += 1;
      break;

    case NR_DL_DCI_FORMAT_2_0:
      break;

    case NR_DL_DCI_FORMAT_2_1:
      break;

    case NR_DL_DCI_FORMAT_2_2:
      break;

    case NR_DL_DCI_FORMAT_2_3:
      break;

    default:
      AssertFatal(1==0, "Invalid NR DCI format %d\n", format);
  }

  return size;
}

int ul_ant_bits(NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig, long transformPrecoder) {

  uint8_t type,maxl;
  if(NR_DMRS_UplinkConfig->dmrs_Type == NULL)
    type = 1;
  else
    type = 2;
  if(NR_DMRS_UplinkConfig->maxLength == NULL)
    maxl = 1;
  else
    maxl = 2;
  if (transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled)
    return( maxl+type+1);
  else {
    if (type==1)
      return (maxl<<1);
    else
      AssertFatal(1==0,"DMRS type not valid for this choice");
  }
}

int tdd_period_to_num[8] = {500,625,1000,1250,2000,2500,5000,10000};

int is_nr_DL_slot(NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,slot_t slot) {

  int period,period1,period2=0;

  if (tdd_UL_DL_ConfigurationCommon==NULL) return(1);

  if (tdd_UL_DL_ConfigurationCommon->pattern1.ext1 &&
      tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530)
    period1 = 3000+*tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  else
    period1 = tdd_period_to_num[tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity];
			       
  if (tdd_UL_DL_ConfigurationCommon->pattern2) {
    if (tdd_UL_DL_ConfigurationCommon->pattern2->ext1 &&
        tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530)
      period2 = 3000+*tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530;
    else
      period2 = tdd_period_to_num[tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity];
  }    
  period = period1+period2;
  int scs=tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing;
  int slots=period*(1<<scs)/1000;
  int slots1=period1*(1<<scs)/1000;
  int slot_in_period = slot % slots;
  if (slot_in_period < slots1) return(slot_in_period <= tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots ? 1 : 0);
  else return(slot_in_period <= slots1+tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSlots ? 1 : 0);    
}

int is_nr_UL_slot(NR_TDD_UL_DL_ConfigCommon_t	*tdd_UL_DL_ConfigurationCommon, slot_t slot, lte_frame_type_t frame_type) {

  int period,period1,period2=0;

  // Note: condition on frame_type
  // goal: the UL scheduler assumes mode is TDD therefore this hack is needed to make FDD work
  if (tdd_UL_DL_ConfigurationCommon == NULL || frame_type == FDD) {
    return(1);
  }

  if (tdd_UL_DL_ConfigurationCommon->pattern1.ext1 &&
      tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530)
    period1 = 3000+*tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  else
    period1 = tdd_period_to_num[tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity];
			       
  if (tdd_UL_DL_ConfigurationCommon->pattern2) {
    if (tdd_UL_DL_ConfigurationCommon->pattern2->ext1 &&
	      tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530)
      period2 = 3000+*tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530;
    else
      period2 = tdd_period_to_num[tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity];
  }    
  period = period1+period2;
  int scs=tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing;
  int slots=period*(1<<scs)/1000;
  int slots1=period1*(1<<scs)/1000;
  int slot_in_period = slot % slots;
  if (slot_in_period < slots1) return(slot_in_period >= tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots ? 1 : 0);
  else return(slot_in_period >= slots1+tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSlots ? 1 : 0);    
}

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols, int startSymbol, int mappingtype_fromDCI) {

  int l0;int dmrs_AdditionalPosition = 0;int maxLength = 0;
  NR_DMRS_DownlinkConfig_t *dmrs_config = NULL;

  LOG_D(MAC, "NrofSymbols:%d, startSymbol:%d, mappingtype:%d, dmrs_TypeA_Position:%d\n", NrOfSymbols, startSymbol, mappingtype_fromDCI, dmrs_TypeA_Position);

  if (dmrs_TypeA_Position == NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2) l0=2;
  else if (dmrs_TypeA_Position == NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos3) l0=3;
  else AssertFatal(1==0,"Illegal dmrs_TypeA_Position %d\n",(int)dmrs_TypeA_Position);

  // in case of DCI FORMAT 1_0 or dedicated pdsch config not received additionposition = pos2, len1 should be used
  // referred to section 5.1.6.2 in 38.214
  dmrs_AdditionalPosition = 2;
  maxLength = 1;

  if (pdsch_Config != NULL) {
    if (mappingtype_fromDCI == typeA) { // Type A
      if (pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA && pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->present == NR_SetupRelease_DMRS_DownlinkConfig_PR_setup)
        dmrs_config = (NR_DMRS_DownlinkConfig_t *)pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
    } else if (mappingtype_fromDCI == typeB) {
      if (pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB && pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB->present == NR_SetupRelease_DMRS_DownlinkConfig_PR_setup)
        dmrs_config = (NR_DMRS_DownlinkConfig_t *)pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup;
    } else {
      AssertFatal(1==0,"Incorrect Mappingtype\n");
    }

    AssertFatal(dmrs_config != NULL," DMRS configs not present in PDSCH DMRS Downlink config\n");

    // default values of maxlength = len2, additionalposition is pos2
    if (dmrs_config->maxLength != NULL) maxLength = 2;
    if (dmrs_config->dmrs_AdditionalPosition != NULL) dmrs_AdditionalPosition = *dmrs_config->dmrs_AdditionalPosition;
  }

  uint8_t ld, row, column;
  int32_t l_prime = -1;

  // columns 0-3 for TypeA, 4-7 for TypeB
  column = (mappingtype_fromDCI == typeA) ? dmrs_AdditionalPosition : (dmrs_AdditionalPosition + 4);

  // Section 7.4.1.1.2 in Spec 38.211
  // For PDSCH Mapping TypeA, ld is duration between first OFDM of the slot and last OFDM symbol of the scheduled PDSCH resources
  // For TypeB, ld is the duration of the scheduled PDSCH resources
  ld = (mappingtype_fromDCI == typeA) ? (NrOfSymbols + startSymbol) : NrOfSymbols;

  // Section 7.4.1.1.2 in Spec 38.211
  //For PDSCH Mapping typeB, if PDSCH duration ld <=4, only single symbol DMRS is supported
  if (mappingtype_fromDCI == typeB && ld <= 4)
    maxLength = 1;

  AssertFatal(ld > 2 && ld < 15,"Illegal NrOfSymbols according to Table 5.1.2.1-1 Spec 38.214 %d\n",ld);
  AssertFatal((NrOfSymbols + startSymbol) < 15,"Illegal S+L according to Table 5.1.2.1-1 Spec 38.214 S:%d L:%d\n",startSymbol, NrOfSymbols);

  if (mappingtype_fromDCI == typeA) {

    // Section 7.4.1.1.2 in Spec 38.211
    AssertFatal((l0 == 2) || (l0 == 3 && dmrs_AdditionalPosition != 3),"Wrong config, If dmrs_TypeA_Position POS3, ADD POS cannot be POS3 \n");

    // Table 5.1.2.1-1 in Spec 38.214
    AssertFatal(startSymbol <= l0, "Wrong config, Start symbol %d cannot be later than dmrs_TypeA_Position %d \n", startSymbol, l0);

    // Section 7.4.1.1.2 in Spec 38.211
    AssertFatal(l0 == 2 || (l0 == 3 && (ld != 3 || ld != 4)), "ld 3 or 4 symbols only possible with dmrs_TypeA_Position POS2 \n");

  }

  if (maxLength == 1) {
    row = ld - 2;
    l_prime = table_7_4_1_1_2_3_pdsch_dmrs_positions_l[row][column];
    l0 = 1 << l0;
  }
  else {
    row = (ld < 4) ? 0 : (ld - 3);
    l_prime = table_7_4_1_1_2_4_pdsch_dmrs_positions_l[row][column];
    l0 = 1<<l0 | 1<<(l0+1);
  }

  LOG_D(MAC, "l0:%d, ld:%d,row:%d, column:%d, addpos:%d, maxlen:%d\n", l0, ld, row, column, dmrs_AdditionalPosition, maxLength);
  AssertFatal(l_prime>=0,"ERROR in configuration.Check Time Domain allocation of this Grant. l_prime < 1. row:%d, column:%d\n", row, column);

  l_prime = (mappingtype_fromDCI == typeA) ? (l_prime | l0) : (l_prime << startSymbol);
  LOG_D(MAC, " PDSCH DMRS MASK in HEX:%x\n", l_prime);

  return l_prime;
}

uint8_t get_pusch_mcs_table(long *mcs_Table,
                            int is_tp,
                            int dci_format,
                            int rnti_type,
                            int target_ss,
                            bool config_grant) {

  // implementing 6.1.4.1 in 38.214
  if (mcs_Table != NULL) {
    if (config_grant || (rnti_type == NR_RNTI_CS)) {
      if (*mcs_Table == NR_PUSCH_Config__mcs_Table_qam256)
        return 1;
      else
        return (2+(is_tp<<1));
    }
    else {
      if ((*mcs_Table == NR_PUSCH_Config__mcs_Table_qam256) &&
          (dci_format == NR_UL_DCI_FORMAT_0_1) &&
          ((rnti_type == NR_RNTI_C ) || (rnti_type == NR_RNTI_SP_CSI)))
        return 1;
      // TODO take into account UE configuration
      if ((*mcs_Table == NR_PUSCH_Config__mcs_Table_qam64LowSE) &&
          (target_ss == NR_SearchSpace__searchSpaceType_PR_ue_Specific) &&
          ((rnti_type == NR_RNTI_C ) || (rnti_type == NR_RNTI_SP_CSI)))
        return (2+(is_tp<<1));
      if (rnti_type == NR_RNTI_MCS_C)
        return (2+(is_tp<<1));
      AssertFatal(1==0,"Invalid configuration to set MCS table");
    }
  }
  else
    return (0+(is_tp*3));
}


int binomial(int n, int k) {
  int c = 1, i;

  if (k > n-k) 
    k = n-k;

  for (i = 1; i <= k; i++, n--) {
    if (c/i > UINT_MAX/n) // return 0 on overflow
      return 0;

    c = c / i * n + c % i * n / i;
  }
  return c;
}

/* extract PTRS values from RC and validate it based upon 38.214 5.1.6.3 */
bool set_dl_ptrs_values(NR_PTRS_DownlinkConfig_t *ptrs_config,
                        uint16_t rbSize,uint8_t mcsIndex, uint8_t mcsTable,
                        uint8_t *K_ptrs, uint8_t *L_ptrs,
                        uint8_t *portIndex,uint8_t *nERatio, uint8_t *reOffset,
                        uint8_t NrOfSymbols)
{
  bool valid = true;

  /* as defined in T 38.214 5.1.6.3 */
  if(rbSize < 3) {
    valid = false;
    return valid;
  }
  /* Check for Frequency Density values */
  if(ptrs_config->frequencyDensity->list.count < 2) {
    /* Default value for K_PTRS = 2 as defined in T 38.214 5.1.6.3 */
    *K_ptrs = 2;
  }
  else {
    *K_ptrs = get_K_ptrs(*ptrs_config->frequencyDensity->list.array[0],
                         *ptrs_config->frequencyDensity->list.array[1],
                         rbSize);
  }
  /* Check for time Density values */
  if(ptrs_config->timeDensity->list.count < 3) {
    /* Default value for L_PTRS = 1 as defined in T 38.214 5.1.6.3 */
       *L_ptrs = 1;
  }
  else {
    *L_ptrs = get_L_ptrs(*ptrs_config->timeDensity->list.array[0],
                         *ptrs_config->timeDensity->list.array[1],
                         *ptrs_config->timeDensity->list.array[2],
                         mcsIndex,
                         mcsTable);
  }
  *portIndex =*ptrs_config->epre_Ratio;
  *nERatio = *ptrs_config->resourceElementOffset;
  *reOffset  = 0;
  /* If either or both of the parameters PT-RS time density (LPT-RS) and PT-RS frequency density (KPT-RS), shown in Table
   * 5.1.6.3-1 and Table 5.1.6.3-2, indicates that 'PT-RS not present', the UE shall assume that PT-RS is not present
   */
  if(*K_ptrs ==2  || *K_ptrs ==4 ) {
    valid = true;
  }
  else {
    valid = false;
    return valid;
  }
  if(*L_ptrs ==0 || *L_ptrs ==1 || *L_ptrs ==2  ) {
    valid = true;
  }
  else {
    valid = false;
    return valid;
  }
  /* PTRS is not present also :
   * When the UE is receiving a PDSCH with allocation duration of 4 symbols and if LPT-RS is set to 4, the UE shall assume
   * PT-RS is not transmitted
   * When the UE is receiving a PDSCH with allocation duration of 2 symbols as defined in Clause 7.4.1.1.2 of [4, TS
   * 38.211] and if LPT-RS is set to 2 or 4, the UE shall assume PT-RS is not transmitted.
   */
  if((NrOfSymbols == 4 && *L_ptrs ==2) || ((NrOfSymbols == 2 && *L_ptrs > 0))) {
    valid = false;
    return valid;
  }

  /* Moved below check from scheduler function to here */
  if (*L_ptrs >= NrOfSymbols) {
    valid = false;
    return valid;
  }
  return valid;
}

uint16_t get_ssb_start_symbol(const long band, NR_SubcarrierSpacing_t scs, int i_ssb) {

  switch (scs) {
    case NR_SubcarrierSpacing_kHz15:
      return symbol_ssb_AC[i_ssb];  //type A
    case NR_SubcarrierSpacing_kHz30:
      if (band == 5 || band == 66)
        return symbol_ssb_BD[i_ssb];  //type B
      else
        return symbol_ssb_AC[i_ssb];  //type C
    case NR_SubcarrierSpacing_kHz120:
      return symbol_ssb_BD[i_ssb];  //type D
    case NR_SubcarrierSpacing_kHz240:
      return symbol_ssb_E[i_ssb];
    default:
      AssertFatal(1 == 0, "SCS %ld not allowed for SSB \n",scs);
  }
}


void csi_period_offset(NR_CSI_ReportConfig_t *csirep,
                       NR_NZP_CSI_RS_Resource_t *nzpcsi,
                       int *period, int *offset) {

  if(nzpcsi != NULL) {

    NR_CSI_ResourcePeriodicityAndOffset_PR p_and_o = nzpcsi->periodicityAndOffset->present;

    switch(p_and_o){
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots4:
        *period = 4;
        *offset = nzpcsi->periodicityAndOffset->choice.slots4;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots5:
        *period = 5;
        *offset = nzpcsi->periodicityAndOffset->choice.slots5;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots8:
        *period = 8;
        *offset = nzpcsi->periodicityAndOffset->choice.slots8;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots10:
        *period = 10;
        *offset = nzpcsi->periodicityAndOffset->choice.slots10;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots16:
        *period = 16;
        *offset = nzpcsi->periodicityAndOffset->choice.slots16;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots20:
        *period = 20;
        *offset = nzpcsi->periodicityAndOffset->choice.slots20;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots32:
        *period = 32;
        *offset = nzpcsi->periodicityAndOffset->choice.slots32;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots40:
        *period = 40;
        *offset = nzpcsi->periodicityAndOffset->choice.slots40;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots64:
        *period = 64;
        *offset = nzpcsi->periodicityAndOffset->choice.slots64;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots80:
        *period = 80;
        *offset = nzpcsi->periodicityAndOffset->choice.slots80;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots160:
        *period = 160;
        *offset = nzpcsi->periodicityAndOffset->choice.slots160;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots320:
        *period = 320;
        *offset = nzpcsi->periodicityAndOffset->choice.slots320;
        break;
      case NR_CSI_ResourcePeriodicityAndOffset_PR_slots640:
        *period = 640;
        *offset = nzpcsi->periodicityAndOffset->choice.slots640;
        break;
    default:
      AssertFatal(1==0,"No periodicity and offset found in CSI resource");
    }

  }

  if(csirep != NULL) {

    NR_CSI_ReportPeriodicityAndOffset_PR p_and_o = csirep->reportConfigType.choice.periodic->reportSlotConfig.present;

    switch(p_and_o){
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots4:
        *period = 4;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots4;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots5:
        *period = 5;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots5;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots8:
        *period = 8;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots8;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots10:
        *period = 10;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots10;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots16:
        *period = 16;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots16;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots20:
        *period = 20;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots20;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots40:
        *period = 40;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots40;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots80:
        *period = 80;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots160:
        *period = 160;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots160;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots320:
        *period = 320;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320;
        break;
    default:
      AssertFatal(1==0,"No periodicity and offset resource found in CSI report");
    }
  }
}


void get_type0_PDCCH_CSS_config_parameters(NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config,
                                           frame_t frameP,
                                           NR_MIB_t *mib,
                                           uint8_t num_slot_per_frame,
                                           uint8_t ssb_subcarrier_offset,
                                           uint16_t ssb_start_symbol,
                                           NR_SubcarrierSpacing_t scs_ssb,
                                           frequency_range_t frequency_range,
                                           int nr_band,
                                           uint32_t ssb_index,
                                           uint32_t ssb_period,
                                           uint32_t ssb_offset_point_a) {

  NR_SubcarrierSpacing_t scs_pdcch;

  channel_bandwidth_t min_channel_bw;

  // according to Table 5.3.5-1 in 38.104
  // band 79 is the only one which minimum is 40
  // for all the other channels it is either 10 or 5
  // and there is no difference between the two for this implementation so it is set it to 10
  if (nr_band == 79)
    min_channel_bw = bw_40MHz;
  else
    min_channel_bw = bw_10MHz;

  if (frequency_range == FR2) {
    if(mib->subCarrierSpacingCommon == NR_MIB__subCarrierSpacingCommon_scs15or60)
      scs_pdcch = NR_SubcarrierSpacing_kHz60;
    else
      scs_pdcch = NR_SubcarrierSpacing_kHz120;
  }
  else {
    frequency_range = FR1;
    if(mib->subCarrierSpacingCommon == NR_MIB__subCarrierSpacingCommon_scs15or60)
      scs_pdcch = NR_SubcarrierSpacing_kHz15;
    else
      scs_pdcch = NR_SubcarrierSpacing_kHz30;
  }
  type0_PDCCH_CSS_config->scs_pdcch = scs_pdcch;
  type0_PDCCH_CSS_config->ssb_index = ssb_index;
  type0_PDCCH_CSS_config->frame = frameP;

  uint8_t ssb_slot = ssb_start_symbol/14;

  uint32_t is_condition_A = (ssb_subcarrier_offset == 0);   //  38.213 ch.13
  uint32_t index_4msb = (mib->pdcch_ConfigSIB1.controlResourceSetZero);
  uint32_t index_4lsb = (mib->pdcch_ConfigSIB1.searchSpaceZero);

  type0_PDCCH_CSS_config->num_rbs = -1;
  type0_PDCCH_CSS_config->num_symbols = -1;
  type0_PDCCH_CSS_config->rb_offset = -1;
  LOG_D(NR_MAC,"NR_SubcarrierSpacing_kHz30 %d, scs_ssb %d, scs_pdcch %d, min_chan_bw %d\n",(int)NR_SubcarrierSpacing_kHz30,(int)scs_ssb,(int)scs_pdcch,min_channel_bw);

  //  type0-pdcch coreset
  switch( ((int)scs_ssb << 3) | (int)scs_pdcch ){
    case (NR_SubcarrierSpacing_kHz15 << 5) | NR_SubcarrierSpacing_kHz15:
      AssertFatal(index_4msb < 15, "38.213 Table 13-1 4 MSB out of range\n");
      type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
      type0_PDCCH_CSS_config->num_rbs     = table_38213_13_1_c2[index_4msb];
      type0_PDCCH_CSS_config->num_symbols = table_38213_13_1_c3[index_4msb];
      type0_PDCCH_CSS_config->rb_offset   = table_38213_13_1_c4[index_4msb];
      break;

    case (NR_SubcarrierSpacing_kHz15 << 3) | NR_SubcarrierSpacing_kHz30:
      AssertFatal(index_4msb < 14, "38.213 Table 13-2 4 MSB out of range\n");
      type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
      type0_PDCCH_CSS_config->num_rbs     = table_38213_13_2_c2[index_4msb];
      type0_PDCCH_CSS_config->num_symbols = table_38213_13_2_c3[index_4msb];
      type0_PDCCH_CSS_config->rb_offset   = table_38213_13_2_c4[index_4msb];
      break;

    case (NR_SubcarrierSpacing_kHz30 << 3) | NR_SubcarrierSpacing_kHz15:
      if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
        AssertFatal(index_4msb < 9, "38.213 Table 13-3 4 MSB out of range\n");
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
        type0_PDCCH_CSS_config->num_rbs     = table_38213_13_3_c2[index_4msb];
        type0_PDCCH_CSS_config->num_symbols = table_38213_13_3_c3[index_4msb];
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_3_c4[index_4msb];
      }else if(min_channel_bw & bw_40MHz){
        AssertFatal(index_4msb < 9, "38.213 Table 13-5 4 MSB out of range\n");
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
        type0_PDCCH_CSS_config->num_rbs     = table_38213_13_5_c2[index_4msb];
        type0_PDCCH_CSS_config->num_symbols = table_38213_13_5_c3[index_4msb];
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_5_c4[index_4msb];
      }else{ ; }

      break;
 
    case (NR_SubcarrierSpacing_kHz30 << 3) | NR_SubcarrierSpacing_kHz30:
      if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
        type0_PDCCH_CSS_config->num_rbs     = table_38213_13_4_c2[index_4msb];
        type0_PDCCH_CSS_config->num_symbols = table_38213_13_4_c3[index_4msb];
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_4_c4[index_4msb];

      }else if(min_channel_bw & bw_40MHz){
        AssertFatal(index_4msb < 10, "38.213 Table 13-6 4 MSB out of range\n");
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
        type0_PDCCH_CSS_config->num_rbs     = table_38213_13_6_c2[index_4msb];
        type0_PDCCH_CSS_config->num_symbols = table_38213_13_6_c3[index_4msb];
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_6_c4[index_4msb];
      }else{ ; }
      break;

    case (NR_SubcarrierSpacing_kHz120 << 3) | NR_SubcarrierSpacing_kHz60:
      AssertFatal(index_4msb < 12, "38.213 Table 13-7 4 MSB out of range\n");
      if(index_4msb & 0x7){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
      }else if(index_4msb & 0x18){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 2;
      }else{ ; }

      type0_PDCCH_CSS_config->num_rbs     = table_38213_13_7_c2[index_4msb];
      type0_PDCCH_CSS_config->num_symbols = table_38213_13_7_c3[index_4msb];
      if(!is_condition_A && (index_4msb == 8 || index_4msb == 10)){
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_7_c4[index_4msb] - 1;
      }else{
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_7_c4[index_4msb];
      }
      break;

    case (NR_SubcarrierSpacing_kHz120 << 3) | NR_SubcarrierSpacing_kHz120:
      AssertFatal(index_4msb < 8, "38.213 Table 13-8 4 MSB out of range\n");
      if(index_4msb & 0x3){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
      }else if(index_4msb & 0x0c){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 3;
      }

      type0_PDCCH_CSS_config->num_rbs     = table_38213_13_8_c2[index_4msb];
      type0_PDCCH_CSS_config->num_symbols = table_38213_13_8_c3[index_4msb];
      if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_8_c4[index_4msb] - 1;
      }else{
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_8_c4[index_4msb];
      }
      break;

    case (NR_SubcarrierSpacing_kHz240 << 3) | NR_SubcarrierSpacing_kHz60:
      AssertFatal(index_4msb < 4, "38.213 Table 13-9 4 MSB out of range\n");
      type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
      type0_PDCCH_CSS_config->num_rbs     = table_38213_13_9_c2[index_4msb];
      type0_PDCCH_CSS_config->num_symbols = table_38213_13_9_c3[index_4msb];
      type0_PDCCH_CSS_config->rb_offset   = table_38213_13_9_c4[index_4msb];
      break;

    case (NR_SubcarrierSpacing_kHz240 << 3) | NR_SubcarrierSpacing_kHz120:
      AssertFatal(index_4msb < 8, "38.213 Table 13-10 4 MSB out of range\n");
      if(index_4msb & 0x3){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 1;
      }else if(index_4msb & 0x0c){
        type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern = 2;
      }
      type0_PDCCH_CSS_config->num_rbs     = table_38213_13_10_c2[index_4msb];
      type0_PDCCH_CSS_config->num_symbols = table_38213_13_10_c3[index_4msb];
      if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_10_c4[index_4msb]-1;
      }else{
        type0_PDCCH_CSS_config->rb_offset   = table_38213_13_10_c4[index_4msb];
      }

      break;

    default:
      LOG_E(NR_MAC,"NR_SubcarrierSpacing_kHz30 %d, scs_ssb %d, scs_pdcch %d, min_chan_bw %d\n",NR_SubcarrierSpacing_kHz30,(int)scs_ssb,(int)scs_pdcch,min_channel_bw);
      break;
  }

  LOG_D(NR_MAC,"Coreset0: index_4msb=%d, num_rbs=%d, num_symb=%d, rb_offset=%d\n",
        index_4msb,type0_PDCCH_CSS_config->num_rbs,type0_PDCCH_CSS_config->num_symbols,type0_PDCCH_CSS_config->rb_offset );

  AssertFatal(type0_PDCCH_CSS_config->num_rbs != -1, "Type0 PDCCH coreset num_rbs undefined, index_4msb=%d, min_channel_bw %d, scs_ssb %d, scs_pdcch %d\n",index_4msb,min_channel_bw,(int)scs_ssb,(int)scs_pdcch);
  AssertFatal(type0_PDCCH_CSS_config->num_symbols != -1, "Type0 PDCCH coreset num_symbols undefined");
  AssertFatal(type0_PDCCH_CSS_config->rb_offset != -1, "Type0 PDCCH coreset rb_offset undefined");


  //uint32_t cell_id = 0;   //  obtain from L1 later

  //mac->type0_pdcch_dci_config.coreset.rb_start = rb_offset;
  //mac->type0_pdcch_dci_config.coreset.rb_end = rb_offset + num_rbs - 1;

//  uint64_t mask = 0x0;
//  uint8_t i;
//  for(i=0; i<(type0_PDCCH_CSS_config->num_rbs/6); ++i){   //  38.331 Each bit corresponds a group of 6 RBs
//    mask = mask >> 1;
//    mask = mask | 0x100000000000;
//  }

  //LOG_I(MAC,">>>>>>>>mask %x num_rbs %d rb_offset %d\n", mask, num_rbs, rb_offset);

//    mac->type0_pdcch_dci_config.coreset.frequency_domain_resource = mask;
//    mac->type0_pdcch_dci_config.coreset.rb_offset = rb_offset;  //  additional parameter other than coreset
//
//    //mac->type0_pdcch_dci_config.type0_pdcch_coreset.duration = num_symbols;
//    mac->type0_pdcch_dci_config.coreset.cce_reg_mapping_type = CCE_REG_MAPPING_TYPE_INTERLEAVED;
//    mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_reg_bundle_size = 6;   //  L 38.211 7.3.2.2
//    mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_interleaver_size = 2;  //  R 38.211 7.3.2.2
//    mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_shift_index = cell_id;
//    mac->type0_pdcch_dci_config.coreset.precoder_granularity = PRECODER_GRANULARITY_SAME_AS_REG_BUNDLE;
//    mac->type0_pdcch_dci_config.coreset.pdcch_dmrs_scrambling_id = cell_id;


  // type0-pdcch search space
  float big_o = 0.0f;
  float big_m = 0.0f;
  type0_PDCCH_CSS_config->sfn_c = SFN_C_IMPOSSIBLE;   //  only valid for mux=1
  type0_PDCCH_CSS_config->n_c = UINT_MAX;
  type0_PDCCH_CSS_config->number_of_search_space_per_slot = UINT_MAX;
  type0_PDCCH_CSS_config->first_symbol_index = UINT_MAX;
  type0_PDCCH_CSS_config->search_space_duration = 0;  //  element of search space
  //  38.213 table 10.1-1

  /// MUX PATTERN 1
  if(type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR1){
    big_o = table_38213_13_11_c1[index_4lsb];
    type0_PDCCH_CSS_config->number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
    big_m = table_38213_13_11_c3[index_4lsb];

    uint32_t temp = (uint32_t)(big_o*(1<<scs_pdcch)) + (uint32_t)(type0_PDCCH_CSS_config->ssb_index*big_m);
    type0_PDCCH_CSS_config->n_c = temp / num_slot_per_frame;
    if((temp/num_slot_per_frame) & 0x1){
      type0_PDCCH_CSS_config->sfn_c = SFN_C_MOD_2_EQ_1;
    }else{
      type0_PDCCH_CSS_config->sfn_c = SFN_C_MOD_2_EQ_0;
    }

    if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 7) && (type0_PDCCH_CSS_config->ssb_index&1)){
      type0_PDCCH_CSS_config->first_symbol_index = type0_PDCCH_CSS_config->num_symbols;
    }else{
      type0_PDCCH_CSS_config->first_symbol_index = table_38213_13_11_c4[index_4lsb];
    }
    //  38.213 chapter 13: over two consecutive slots
    type0_PDCCH_CSS_config->search_space_duration = 2;
    // two frames
    type0_PDCCH_CSS_config->search_space_frame_period = nr_slots_per_frame[scs_ssb]<<1;
  }

  if(type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR2){
    big_o = table_38213_13_12_c1[index_4lsb];
    type0_PDCCH_CSS_config->number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
    big_m = table_38213_13_12_c3[index_4lsb];

    if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 10) && (type0_PDCCH_CSS_config->ssb_index&1)){
      type0_PDCCH_CSS_config->first_symbol_index = 7;
    }else if((index_4lsb == 6 || index_4lsb == 7 || index_4lsb == 8 || index_4lsb == 11) && (type0_PDCCH_CSS_config->ssb_index&1)){
      type0_PDCCH_CSS_config->first_symbol_index = type0_PDCCH_CSS_config->num_symbols;
    }else{
      type0_PDCCH_CSS_config->first_symbol_index = 0;
    }
    //  38.213 chapter 13: over two consecutive slots
    type0_PDCCH_CSS_config->search_space_duration = 2;
    // two frames
    type0_PDCCH_CSS_config->search_space_frame_period = nr_slots_per_frame[scs_ssb]<<1;
  }

  /// MUX PATTERN 2
  if(type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern == 2){

    if((scs_ssb == NR_SubcarrierSpacing_kHz120) && (scs_pdcch == NR_SubcarrierSpacing_kHz60)){
      //  38.213 Table 13-13
      AssertFatal(index_4lsb == 0, "38.213 Table 13-13 4 LSB out of range\n");
      //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
      //                sfn_c = SFN_C_EQ_SFN_SSB;
      type0_PDCCH_CSS_config->n_c = ssb_slot;
      switch(type0_PDCCH_CSS_config->ssb_index & 0x3){    //  ssb_index(i) mod 4
        case 0:
          type0_PDCCH_CSS_config->first_symbol_index = 0;
          break;
        case 1:
          type0_PDCCH_CSS_config->first_symbol_index = 1;
          break;
        case 2:
          type0_PDCCH_CSS_config->first_symbol_index = 6;
          break;
        case 3:
          type0_PDCCH_CSS_config->first_symbol_index = 7;
          break;
        default: break;
      }

    }else if((scs_ssb == NR_SubcarrierSpacing_kHz240) && (scs_pdcch == NR_SubcarrierSpacing_kHz120)){
      //  38.213 Table 13-14
      AssertFatal(index_4lsb == 0, "38.213 Table 13-14 4 LSB out of range\n");
      //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
      //                sfn_c = SFN_C_EQ_SFN_SSB;
      type0_PDCCH_CSS_config->n_c = ssb_slot;
      switch(type0_PDCCH_CSS_config->ssb_index & 0x7){    //  ssb_index(i) mod 8
        case 0:
          type0_PDCCH_CSS_config->first_symbol_index = 0;
          break;
        case 1:
          type0_PDCCH_CSS_config->first_symbol_index = 1;
          break;
        case 2:
          type0_PDCCH_CSS_config->first_symbol_index = 2;
          break;
        case 3:
          type0_PDCCH_CSS_config->first_symbol_index = 3;
          break;
        case 4:
          type0_PDCCH_CSS_config->first_symbol_index = 12;
          type0_PDCCH_CSS_config->n_c = ssb_slot - 1;
          break;
        case 5:
          type0_PDCCH_CSS_config->first_symbol_index = 13;
          type0_PDCCH_CSS_config->n_c = ssb_slot - 1;
          break;
        case 6:
          type0_PDCCH_CSS_config->first_symbol_index = 0;
          break;
        case 7:
          type0_PDCCH_CSS_config->first_symbol_index = 1;
          break;
        default: break;
      }
    }else{ ; }
    //  38.213 chapter 13: over one slot
    type0_PDCCH_CSS_config->search_space_duration = 1;
    // SSB periodicity in slots
    type0_PDCCH_CSS_config->search_space_frame_period = ssb_period*nr_slots_per_frame[scs_ssb];
  }

  /// MUX PATTERN 3
  if(type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern == 3){
    if((scs_ssb == NR_SubcarrierSpacing_kHz120) && (scs_pdcch == NR_SubcarrierSpacing_kHz120)){
      //  38.213 Table 13-15
      AssertFatal(index_4lsb == 0, "38.213 Table 13-15 4 LSB out of range\n");
      //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
      //                sfn_c = SFN_C_EQ_SFN_SSB;
      type0_PDCCH_CSS_config->n_c = ssb_slot;
      switch(type0_PDCCH_CSS_config->ssb_index & 0x3){    //  ssb_index(i) mod 4
        case 0:
          type0_PDCCH_CSS_config->first_symbol_index = 4;
          break;
        case 1:
          type0_PDCCH_CSS_config->first_symbol_index = 8;
          break;
        case 2:
          type0_PDCCH_CSS_config->first_symbol_index = 2;
          break;
        case 3:
          type0_PDCCH_CSS_config->first_symbol_index = 6;
          break;
        default: break;
      }
    }else{ ; }
    //  38.213 chapter 13: over one slot
    type0_PDCCH_CSS_config->search_space_duration = 1;
    // SSB periodicity in slots
    type0_PDCCH_CSS_config->search_space_frame_period = ssb_period*nr_slots_per_frame[scs_ssb];
  }

  AssertFatal(type0_PDCCH_CSS_config->number_of_search_space_per_slot!=UINT_MAX,"");

//  uint32_t coreset_duration = num_symbols * number_of_search_space_per_slot;
//    mac->type0_pdcch_dci_config.number_of_candidates[0] = table_38213_10_1_1_c2[0];
//    mac->type0_pdcch_dci_config.number_of_candidates[1] = table_38213_10_1_1_c2[1];
//    mac->type0_pdcch_dci_config.number_of_candidates[2] = table_38213_10_1_1_c2[2];   //  CCE aggregation level = 4
//    mac->type0_pdcch_dci_config.number_of_candidates[3] = table_38213_10_1_1_c2[3];   //  CCE aggregation level = 8
//    mac->type0_pdcch_dci_config.number_of_candidates[4] = table_38213_10_1_1_c2[4];   //  CCE aggregation level = 16
//    mac->type0_pdcch_dci_config.duration = search_space_duration;
//    mac->type0_pdcch_dci_config.coreset.duration = coreset_duration;   //  coreset
//    AssertFatal(first_symbol_index!=UINT_MAX,"");
//    mac->type0_pdcch_dci_config.monitoring_symbols_within_slot = (0x3fff << first_symbol_index) & (0x3fff >> (14-coreset_duration-first_symbol_index)) & 0x3fff;

  AssertFatal(type0_PDCCH_CSS_config->sfn_c!=SFN_C_IMPOSSIBLE,"");
  AssertFatal(type0_PDCCH_CSS_config->n_c!=UINT_MAX,"");

  type0_PDCCH_CSS_config->n_0 = ((uint32_t)(big_o*(1<<scs_pdcch)) + (uint32_t)(type0_PDCCH_CSS_config->ssb_index*big_m))%num_slot_per_frame;
  type0_PDCCH_CSS_config->cset_start_rb = ssb_offset_point_a - type0_PDCCH_CSS_config->rb_offset;

}

void fill_coresetZero(NR_ControlResourceSet_t *coreset0, NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config) {

  int32_t duration;

  if (coreset0 == NULL)
    coreset0 = calloc(1,sizeof(*coreset0));

  coreset0->controlResourceSetId = 0;

  AssertFatal(type0_PDCCH_CSS_config!=NULL,"No type0 CSS configuration\n");

  duration = type0_PDCCH_CSS_config->num_symbols;

  if(coreset0->frequencyDomainResources.buf == NULL) coreset0->frequencyDomainResources.buf = calloc(1,6);

  switch(type0_PDCCH_CSS_config->num_rbs){
    case 24:
      coreset0->frequencyDomainResources.buf[0] = 0xf0;
      coreset0->frequencyDomainResources.buf[1] = 0;
      break;
    case 48:
      coreset0->frequencyDomainResources.buf[0] = 0xff;
      coreset0->frequencyDomainResources.buf[1] = 0;
      break;
    case 96:
      coreset0->frequencyDomainResources.buf[0] = 0xff;
      coreset0->frequencyDomainResources.buf[1] = 0xff;
      break;
  default:
    AssertFatal(1==0,"Invalid number of PRBs %d for Coreset0\n",type0_PDCCH_CSS_config->num_rbs);
  }
  coreset0->frequencyDomainResources.buf[2] = 0;
  coreset0->frequencyDomainResources.buf[3] = 0;
  coreset0->frequencyDomainResources.buf[4] = 0;
  coreset0->frequencyDomainResources.buf[5] = 0;
  coreset0->frequencyDomainResources.size = 6;
  coreset0->frequencyDomainResources.bits_unused = 3;

  coreset0->duration = duration;
  coreset0->cce_REG_MappingType.present=NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved;
  coreset0->cce_REG_MappingType.choice.interleaved=calloc(1,sizeof(*coreset0->cce_REG_MappingType.choice.interleaved));
  coreset0->cce_REG_MappingType.choice.interleaved->reg_BundleSize = NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6;
  coreset0->cce_REG_MappingType.choice.interleaved->interleaverSize = NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n2;
  coreset0->cce_REG_MappingType.choice.interleaved->shiftIndex = NULL; // -> use cell_id
  coreset0->precoderGranularity = NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle;

  coreset0->tci_StatesPDCCH_ToAddList = NULL;
  coreset0->tci_StatesPDCCH_ToReleaseList = NULL;
  coreset0->tci_PresentInDCI = NULL;
  coreset0->pdcch_DMRS_ScramblingID = NULL;

}

void fill_searchSpaceZero(NR_SearchSpace_t *ss0, NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config) {

  if(ss0 == NULL) ss0=calloc(1,sizeof(*ss0));
  if(ss0->controlResourceSetId == NULL) ss0->controlResourceSetId=calloc(1,sizeof(*ss0->controlResourceSetId));
  if(ss0->monitoringSymbolsWithinSlot == NULL) ss0->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss0->monitoringSymbolsWithinSlot));
  if(ss0->monitoringSymbolsWithinSlot->buf == NULL) ss0->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  if(ss0->nrofCandidates == NULL) ss0->nrofCandidates = calloc(1,sizeof(*ss0->nrofCandidates));
  if(ss0->searchSpaceType == NULL) ss0->searchSpaceType = calloc(1,sizeof(*ss0->searchSpaceType));
  if(ss0->searchSpaceType->choice.common == NULL) ss0->searchSpaceType->choice.common=calloc(1,sizeof(*ss0->searchSpaceType->choice.common));
  if(ss0->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 == NULL)
    ss0->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(*ss0->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));

  uint32_t duration,periodicity,offset;
  uint16_t symbols,max_agg;

  AssertFatal(type0_PDCCH_CSS_config!=NULL,"No type0 CSS configuration\n");

  max_agg = (type0_PDCCH_CSS_config->num_symbols*type0_PDCCH_CSS_config->num_rbs)/6;

  symbols = (1-(1<<type0_PDCCH_CSS_config->num_symbols))<<type0_PDCCH_CSS_config->first_symbol_index;
  duration = type0_PDCCH_CSS_config->search_space_duration;
  periodicity = type0_PDCCH_CSS_config->search_space_frame_period;
  if (type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern == 1)
    offset = type0_PDCCH_CSS_config->n_0;
  else
    offset = type0_PDCCH_CSS_config->n_c;

  ss0->searchSpaceId = 0;
  *ss0->controlResourceSetId = 0;
  ss0->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(*ss0->monitoringSlotPeriodicityAndOffset));
  set_monitoring_periodicity_offset(ss0,periodicity,offset);
  if (duration==1)
    ss0->duration = NULL;
  else{
    ss0->duration = calloc(1,sizeof(*ss0->duration));
    *ss0->duration = duration;
  }

  ss0->monitoringSymbolsWithinSlot->size = 2;
  ss0->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss0->monitoringSymbolsWithinSlot->buf[1] = 0;
  ss0->monitoringSymbolsWithinSlot->buf[0] = 0;
  for (int i=0; i<8; i++) {
    ss0->monitoringSymbolsWithinSlot->buf[1] |= ((symbols>>(i+8))&0x01)<<(7-i);
    ss0->monitoringSymbolsWithinSlot->buf[0] |= ((symbols>>i)&0x01)<<(7-i);
  }

  // max values are set according to TS38.213 Section 10.1 Table 10.1-1
  ss0->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss0->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
  ss0->nrofCandidates->aggregationLevel4 = (((max_agg>>2) > 4)? 4 : max_agg>>2);
  ss0->nrofCandidates->aggregationLevel8 = (((max_agg>>3) > 2)? 2 : max_agg>>3);
  ss0->nrofCandidates->aggregationLevel16 = (((max_agg>>4) > 1)? 1 : max_agg>>4);

  ss0->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
}


void find_period_offest_SR (NR_SchedulingRequestResourceConfig_t *SchedulingReqRec, int *period, int *offset) {
  NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR P_O = SchedulingReqRec->periodicityAndOffset->present;
  switch (P_O){
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl1:
      *period = 1;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl1;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl2:
      *period = 2;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl2;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl4:
      *period = 4;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl4;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl5:
      *period = 5;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl5;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl8:
      *period = 8;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl8;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl10:
      *period = 10;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl10;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl16:
      *period = 16;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl16;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl20:
      *period = 20;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl20;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl40:
      *period = 40;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl40;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl80:
      *period = 80;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl80;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl160:
      *period = 160;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl160;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl320:
      *period = 320;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl320;
      break;
    case NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl640:
      *period = 640;
      *offset = SchedulingReqRec->periodicityAndOffset->choice.sl640;
      break;
    default:
      AssertFatal(1==0,"No periodicityAndOffset resources found in schedulingrequestresourceconfig");
  }
}

uint16_t compute_pucch_prb_size(uint8_t format,
                                uint8_t nr_prbs,
                                uint16_t O_tot,
                                uint16_t O_csi,
                                NR_PUCCH_MaxCodeRate_t *maxCodeRate,
                                uint8_t Qm,
                                uint8_t n_symb,
                                uint8_t n_re_ctrl) {

  uint16_t O_crc;

  if (O_tot<12)
    O_crc = 0;
  else{
    if (O_tot<20)
      O_crc = 6;
    else {
      if (O_tot<360)
        O_crc = 11;
      else
        AssertFatal(1==0,"Case for segmented PUCCH not yet implemented");
    }
  }

  int rtimes100;
  switch(*maxCodeRate){
    case NR_PUCCH_MaxCodeRate_zeroDot08 :
      rtimes100 = 8;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot15 :
      rtimes100 = 15;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot25 :
      rtimes100 = 25;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot35 :
      rtimes100 = 35;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot45 :
      rtimes100 = 45;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot60 :
      rtimes100 = 60;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot80 :
      rtimes100 = 80;
      break;
  default :
    AssertFatal(1==0,"Invalid MaxCodeRate");
  }

  float r = (float)rtimes100/100;

  if (O_csi == O_tot) {
    if ((O_tot+O_csi)>(nr_prbs*n_re_ctrl*n_symb*Qm*r))
      AssertFatal(1==0,"MaxCodeRate %.2f can't support %d UCI bits and %d CRC bits with %d PRBs",
                  r,O_tot,O_crc,nr_prbs);
    else
      return nr_prbs;
  }

  if (format==2){
    // TODO fix this for multiple CSI reports
    for (int i=1; i<=nr_prbs; i++){
      if((O_tot+O_crc)<=(i*n_symb*Qm*n_re_ctrl*r) &&
         (O_tot+O_crc)>((i-1)*n_symb*Qm*n_re_ctrl*r))
        return i;
    }
    AssertFatal(1==0,"MaxCodeRate %.2f can't support %d UCI bits and %d CRC bits with at most %d PRBs",
                r,O_tot,O_crc,nr_prbs);
  }
  else{
    AssertFatal(1==0,"Not yet implemented");
  }
}

/* extract UL PTRS values from RRC and validate it based upon 38.214 6.2.3 */
bool set_ul_ptrs_values(NR_PTRS_UplinkConfig_t *ul_ptrs_config,
                        uint16_t rbSize,uint8_t mcsIndex, uint8_t mcsTable,
                        uint8_t *K_ptrs, uint8_t *L_ptrs,
                        uint8_t *reOffset, uint8_t *maxNumPorts, uint8_t *ulPower,
                        uint8_t NrOfSymbols)
{
  bool valid = true;

  /* as defined in T 38.214 6.2.3 */
  if(rbSize < 3) {
    valid = false;
    return valid;
  }
  /* Check for Frequency Density values */
  if(ul_ptrs_config->transformPrecoderDisabled->frequencyDensity->list.count < 2) {
    /* Default value for K_PTRS = 2 as defined in T 38.214 6.2.3 */
    *K_ptrs = 2;
  }
  else {
    *K_ptrs = get_K_ptrs(*ul_ptrs_config->transformPrecoderDisabled->frequencyDensity->list.array[0],
                         *ul_ptrs_config->transformPrecoderDisabled->frequencyDensity->list.array[1],
                         rbSize);
  }
  /* Check for time Density values */
  if(ul_ptrs_config->transformPrecoderDisabled->timeDensity->list.count < 3) {
    *L_ptrs = 0;
  }
  else {
    *L_ptrs = get_L_ptrs(*ul_ptrs_config->transformPrecoderDisabled->timeDensity->list.array[0],
                         *ul_ptrs_config->transformPrecoderDisabled->timeDensity->list.array[1],
                         *ul_ptrs_config->transformPrecoderDisabled->timeDensity->list.array[2],
                         mcsIndex,
                         mcsTable);
  }
  
  *reOffset  = *ul_ptrs_config->transformPrecoderDisabled->resourceElementOffset;
  *maxNumPorts = ul_ptrs_config->transformPrecoderDisabled->maxNrofPorts;
  *ulPower = ul_ptrs_config->transformPrecoderDisabled->ptrs_Power;
  /* If either or both of the parameters PT-RS time density (LPT-RS) and PT-RS frequency density (KPT-RS), shown in Table
   * 6.2.3.1-1 and Table 6.2.3.1-2, indicates that 'PT-RS not present', the UE shall assume that PT-RS is not present
   */
  if(*K_ptrs ==2  || *K_ptrs ==4 ) {
    valid = true;
  }
  else {
    valid = false;
    return valid;
  }
  if(*L_ptrs ==0 || *L_ptrs ==1 || *L_ptrs ==2  ) {
    valid = true;
  }
  else {
    valid = false;
    return valid;
  }
  /* PTRS is not present also :
   * When the UE is receiving a PUSCH with allocation duration of 4 symbols and if LPT-RS is set to 4, the UE shall assume
   * PT-RS is not transmitted
   * When the UE is receiving a PUSCH with allocation duration of 2 symbols as defined in Clause 6.4.1.2.2 of [4, TS
   * 38.211] and if LPT-RS is set to 2 or 4, the UE shall assume PT-RS is not transmitted.
   */
  if((NrOfSymbols == 4 && *L_ptrs ==2) || ((NrOfSymbols == 2 && *L_ptrs > 0))) {
    valid = false;
    return valid;
  }

  /* Moved below check from nr_ue_scheduler function to here */
  if (*L_ptrs >= NrOfSymbols) {
    valid = false;
    return valid;
  }
  return valid;
}
