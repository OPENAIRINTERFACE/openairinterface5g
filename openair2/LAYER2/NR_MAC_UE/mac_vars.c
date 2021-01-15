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

/* \file vars.h
 * \brief MAC Layer variables
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include <stdint.h>

#define reserved 0xffff

const char *rnti_types[]={"RNTI_new", "RNTI_C", "RNTI_RA", "NR_RNTI_P", "NR_RNTI_CS", "NR_RNTI_TC"};
const char *dci_formats[]={"1_0", "1_1", "2_0", "2_1", "2_2", "2_3", "0_0", "0_1"};

//	specification mapping talbe, table_38$x_$y_$z_c$a
//	- $x: specification
//	- $y: subclause-major
//	- $z: subclause-minor
//	- $a: ($a)th of column in table, start from zero
const int32_t table_38213_13_1_c2[16] = {24, 24, 24, 24, 24, 24, 48, 48, 48, 48, 48, 48, 96, 96, 96, reserved}; // index 15 reserved
const int32_t table_38213_13_1_c3[16] = { 2,  2,  2,  3,  3,  3,  1,  1,  2,  2,  3,  3,  1,  2,  3, reserved}; // index 15 reserved
const int32_t table_38213_13_1_c4[16] = { 0,  2,  4,  0,  2,  4, 12, 16, 12, 16, 12, 16, 38, 38, 38, reserved}; // index 15 reserved

const int32_t table_38213_13_2_c2[16] = {24, 24, 24, 24, 24, 24, 24, 24, 48, 48, 48, 48, 48, 48, reserved, reserved}; // index 14-15 reserved
const int32_t table_38213_13_2_c3[16] = { 2,  2,  2,  2,  3,  3,  3,  3,  1,  1,  2,  2,  3,  3, reserved, reserved}; // index 14-15 reserved
const int32_t table_38213_13_2_c4[16] = { 5,  6,  7,  8,  5,  6,  7,  8, 18, 20, 18, 20, 18, 20, reserved, reserved}; // index 14-15 reserved

const int32_t table_38213_13_3_c2[16] = {48, 48, 48, 48, 48, 48, 96, 96, 96, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_3_c3[16] = { 1,  1,  2,  2,  3,  3,  1,  2,  3, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_3_c4[16] = { 2,  6,  2,  6,  2,  6, 28, 28, 28, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved

const int32_t table_38213_13_4_c2[16] = {24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 48, 48, 48, 48, 48, 48};
const int32_t table_38213_13_4_c3[16] = { 2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  1,  1,  1,  2,  2,  2};
const int32_t table_38213_13_4_c4[16] = { 0,  1,  2,  3,  4,  0,  1,  2,  3,  4, 12, 14, 16, 12, 14, 16};

const int32_t table_38213_13_5_c2[16] = {48, 48, 48, 96, 96, 96, 96, 96, 96, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_5_c3[16] = { 1,  2,  3,  1,  1,  2,  2,  3,  3, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved
const int32_t table_38213_13_5_c4[16] = { 4,  4,  4,  0, 56,  0, 56,  0, 56, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 09-15 reserved

const int32_t table_38213_13_6_c2[16] = {24, 24, 24, 24, 48, 48, 48, 48, 48, 48, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved
const int32_t table_38213_13_6_c3[16] = { 2,  2,  3,  3,  1,  1,  2,  2,  3,  3, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved
const int32_t table_38213_13_6_c4[16] = { 0,  4,  0,  4,  0, 28,  0, 28,  0, 28, reserved, reserved, reserved, reserved, reserved, reserved}; // index 10-15 reserved

const int32_t table_38213_13_7_c2[16] = {48, 48, 48, 48, 48, 48, 96, 96, 48, 48, 96, 96, reserved, reserved, reserved, reserved}; // index 12-15 reserved
const int32_t table_38213_13_7_c3[16] = { 1,  1,  2,  2,  3,  3,  1,  2,  1,  1,  1,  1, reserved, reserved, reserved, reserved}; // index 12-15 reserved
const int32_t table_38213_13_7_c4[16] = { 0,  8,  0,  8,  0,  8, 28, 28,-41, 49,-41, 97, reserved, reserved, reserved, reserved}; // index 12-15 reserved, condition A as default

const int32_t table_38213_13_8_c2[16] = { 1,  1,  1,  1,  3,  3,  3,  3, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved
const int32_t table_38213_13_8_c3[16] = {24, 24, 48, 48, 24, 24, 48, 48, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved
const int32_t table_38213_13_8_c4[16] = { 0,  4, 14, 14,-20, 24,-20, 48, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 15 reserved, condition A as default

const int32_t table_38213_13_9_c2[16] = {96, 96, 96, 96, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved
const int32_t table_38213_13_9_c3[16] = { 1,  1,  2,  2, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved
const int32_t table_38213_13_9_c4[16] = { 0, 16,  0, 16, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved}; // index 04-15 reserved

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

const uint8_t table_9_2_2_1[16][8]={{0,12,2, 0, 0,3,0,0},
                                    {0,12,2, 0, 0,4,8,0},
                                    {0,12,2, 3, 0,4,8,0},
                                    {1,10,4, 0, 0,6,0,0},
                                    {1,10,4, 0, 0,3,6,9},
                                    {1,10,4, 2, 0,3,6,9},
                                    {1,10,4, 4, 0,3,6,9},
                                    {1,4, 10,0, 0,6,0,0},
                                    {1,4, 10,0, 0,3,6,9},
                                    {1,4, 10,2, 0,3,6,9},
                                    {1,4, 10,4, 0,3,6,9},
                                    {1,0, 14,0, 0,6,0,0},
                                    {1,0, 14,0, 0,3,6,9},
                                    {1,0, 14,2, 0,3,6,9},
                                    {1,0, 14,4, 0,3,6,9},
                                    {1,0, 14,26,0,3,0,0}};

// table_7_3_1_1_2_2_3_4_5 contains values for number of layers and precoding information for tables 7.3.1.1.2-2/3/4/5 from TS 38.212 subclause 7.3.1.1.2
// the first 6 columns contain table 7.3.1.1.2-2: Precoding information and number of layers, for 4 antenna ports, if transformPrecoder=disabled and maxRank = 2 or 3 or 4
// next six columns contain table 7.3.1.1.2-3: Precoding information and number of layers for 4 antenna ports, if transformPrecoder= enabled, or if transformPrecoder=disabled and maxRank = 1
// next four columns contain table 7.3.1.1.2-4: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder=disabled and maxRank = 2
// next four columns contain table 7.3.1.1.2-5: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder= enabled, or if transformPrecoder= disabled and maxRank = 1
const uint8_t table_7_3_1_1_2_2_3_4_5[64][20] = {
  {1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0},
  {1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
  {1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  2,  0,  2,  0,  1,  2,  0,  0},
  {1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  2,  0,  0,  1,  3,  0,  0},
  {2,  0,  2,  0,  2,  0,  1,  4,  1,  4,  0,  0,  1,  3,  0,  0,  1,  4,  0,  0},
  {2,  1,  2,  1,  2,  1,  1,  5,  1,  5,  0,  0,  1,  4,  0,  0,  1,  5,  0,  0},
  {2,  2,  2,  2,  2,  2,  1,  6,  1,  6,  0,  0,  1,  5,  0,  0,  0,  0,  0,  0},
  {2,  3,  2,  3,  2,  3,  1,  7,  1,  7,  0,  0,  2,  1,  0,  0,  0,  0,  0,  0},
  {2,  4,  2,  4,  2,  4,  1,  8,  1,  8,  0,  0,  2,  2,  0,  0,  0,  0,  0,  0},
  {2,  5,  2,  5,  2,  5,  1,  9,  1,  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  0,  3,  0,  3,  0,  1,  10, 1,  10, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  0,  4,  0,  4,  0,  1,  11, 1,  11, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  4,  1,  4,  0,  0,  1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  5,  1,  5,  0,  0,  1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  6,  1,  6,  0,  0,  1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  7,  1,  7,  0,  0,  1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  8,  1,  8,  0,  0,  1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  9,  1,  9,  0,  0,  1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  10, 1,  10, 0,  0,  1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  11, 1,  11, 0,  0,  1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  6,  2,  6,  0,  0,  1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  7,  2,  7,  0,  0,  1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  8,  2,  8,  0,  0,  1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  9,  2,  9,  0,  0,  1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  10, 2,  10, 0,  0,  1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  11, 2,  11, 0,  0,  1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  12, 2,  12, 0,  0,  1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  13, 2,  13, 0,  0,  1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  1,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  2,  3,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  1,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  2,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};

const uint8_t table_7_3_1_1_2_12[14][3] = {
  {1,0,1},
  {1,1,1},
  {2,0,1},
  {2,1,1},
  {2,2,1},
  {2,3,1},
  {2,0,2},
  {2,1,2},
  {2,2,2},
  {2,3,2},
  {2,4,2},
  {2,5,2},
  {2,6,2},
  {2,7,2}
};

const uint8_t table_7_3_1_1_2_13[10][4] = {
  {1,0,1,1},
  {2,0,1,1},
  {2,2,3,1},
  {2,0,2,1},
  {2,0,1,2},
  {2,2,3,2},
  {2,4,5,2},
  {2,6,7,2},
  {2,0,4,2},
  {2,2,6,2}
};

const uint8_t table_7_3_1_1_2_14[3][5] = {
  {2,0,1,2,1},
  {2,0,1,4,2},
  {2,2,3,6,2}
};

const uint8_t table_7_3_1_1_2_15[4][6] = {
  {2,0,1,2,3,1},
  {2,0,1,4,5,2},
  {2,2,3,6,7,2},
  {2,0,2,4,6,2}
};

const uint8_t table_7_3_1_1_2_16[12][2] = {
  {1,0},
  {1,1},
  {2,0},
  {2,1},
  {2,2},
  {2,3},
  {3,0},
  {3,1},
  {3,2},
  {3,3},
  {3,4},
  {3,5}
};

const uint8_t table_7_3_1_1_2_17[7][3] = {
  {1,0,1},
  {2,0,1},
  {2,2,3},
  {3,0,1},
  {3,2,3},
  {3,4,5},
  {2,0,2}
};

const uint8_t table_7_3_1_1_2_18[3][4] = {
  {2,0,1,2},
  {3,0,1,2},
  {3,3,4,5}
};

const uint8_t table_7_3_1_1_2_19[2][5] = {
  {2,0,1,2,3},
  {3,0,1,2,3}
};

const uint8_t table_7_3_1_1_2_20[28][3] = {
  {1,0,1},
  {1,1,1},
  {2,0,1},
  {2,1,1},
  {2,2,1},
  {2,3,1},
  {3,0,1},
  {3,1,1},
  {3,2,1},
  {3,3,1},
  {3,4,1},
  {3,5,1},
  {3,0,2},
  {3,1,2},
  {3,2,2},
  {3,3,2},
  {3,4,2},
  {3,5,2},
  {3,6,2},
  {3,7,2},
  {3,8,2},
  {3,9,2},
  {3,10,2},
  {3,11,2},
  {1,0,2},
  {1,1,2},
  {1,6,2},
  {1,7,2}
};

const uint8_t table_7_3_1_1_2_21[19][4] = {
  {1,0,1,1},
  {2,0,1,1},
  {2,2,3,1},
  {3,0,1,1},
  {3,2,3,1},
  {3,4,5,1},
  {2,0,2,1},
  {3,0,1,2},
  {3,2,3,2},
  {3,4,5,2},
  {3,6,7,2},
  {3,8,9,2},
  {3,10,11,2},
  {1,0,1,2},
  {1,6,7,2},
  {2,0,1,2},
  {2,2,3,2},
  {2,6,7,2},
  {2,8,9,2}
};

const uint8_t table_7_3_1_1_2_22[6][5] = {
  {2,0,1,2,1},
  {3,0,1,2,1},
  {3,3,4,5,1},
  {3,0,1,6,2},
  {3,2,3,8,2},
  {3,4,5,10,2}
};

const uint8_t table_7_3_1_1_2_23[5][6] = {
  {2,0,1,2,3,1},
  {3,0,1,2,3,1},
  {3,0,1,6,7,2},
  {3,2,3,8,9,2},
  {3,4,5,10,11,2}
};

const uint8_t table_7_3_2_3_3_1[12][5] = {
  {1,0,0,0,0},
  {1,1,0,0,0},
  {1,0,1,0,0},
  {2,0,0,0,0},
  {2,1,0,0,0},
  {2,2,0,0,0},
  {2,3,0,0,0},
  {2,0,1,0,0},
  {2,2,3,0,0},
  {2,0,1,2,0},
  {2,0,1,2,3},
  {2,0,2,0,0}
};

const uint8_t table_7_3_2_3_3_2_oneCodeword[31][6] = {
  {1,0,0,0,0,1},
  {1,1,0,0,0,1},
  {1,0,1,0,0,1},
  {2,0,0,0,0,1},
  {2,1,0,0,0,1},
  {2,2,0,0,0,1},
  {2,3,0,0,0,1},
  {2,0,1,0,0,1},
  {2,2,3,0,0,1},
  {2,0,1,2,0,1},
  {2,0,1,2,3,1},
  {2,0,2,0,0,1},
  {2,0,0,0,0,2},
  {2,1,0,0,0,2},
  {2,2,0,0,0,2},
  {2,3,0,0,0,2},
  {2,4,0,0,0,2},
  {2,5,0,0,0,2},
  {2,6,0,0,0,2},
  {2,7,0,0,0,2},
  {2,0,1,0,0,2},
  {2,2,3,0,0,2},
  {2,4,5,0,0,2},
  {2,6,7,0,0,2},
  {2,0,4,0,0,2},
  {2,2,6,0,0,2},
  {2,0,1,4,0,2},
  {2,2,3,6,0,2},
  {2,0,1,4,5,2},
  {2,2,3,6,7,2},
  {2,0,2,4,6,2}
};

const uint8_t table_7_3_2_3_3_2_twoCodeword[4][10] = {
  {2,0,1,2,3,4,0,0,0,2},
  {2,0,1,2,3,4,6,0,0,2},
  {2,0,1,2,3,4,5,6,0,2},
  {2,0,1,2,3,4,5,6,7,2}
};

const uint8_t table_7_3_2_3_3_3_oneCodeword[24][5] = {
  {1,0,0,0,0},
  {1,1,0,0,0},
  {1,0,1,0,0},
  {2,0,0,0,0},
  {2,1,0,0,0},
  {2,2,0,0,0},
  {2,3,0,0,0},
  {2,0,1,0,0},
  {2,2,3,0,0},
  {2,0,1,2,0},
  {2,0,1,2,3},
  {3,0,0,0,0},
  {3,1,0,0,0},
  {3,2,0,0,0},
  {3,3,0,0,0},
  {3,4,0,0,0},
  {3,5,0,0,0},
  {3,0,1,0,0},
  {3,2,3,0,0},
  {3,4,5,0,0},
  {3,0,1,2,0},
  {3,3,4,5,0},
  {3,0,1,2,3},
  {2,0,2,0,0}
};

const uint8_t table_7_3_2_3_3_3_twoCodeword[2][7] = {
  {3,0,1,2,3,4,0},
  {3,0,1,2,3,4,5}
};

const uint8_t table_7_3_2_3_3_4_oneCodeword[58][6] = {
  {1,0,0,0,0,1},
  {1,1,0,0,0,1},
  {1,0,1,0,0,1},
  {2,0,0,0,0,1},
  {2,1,0,0,0,1},
  {2,2,0,0,0,1},
  {2,3,0,0,0,1},
  {2,0,1,0,0,1},
  {2,2,3,0,0,1},
  {2,0,1,2,0,1},
  {2,0,1,2,3,1},
  {3,0,0,0,0,1},
  {3,1,0,0,0,1},
  {3,2,0,0,0,1},
  {3,3,0,0,0,1},
  {3,4,0,0,0,1},
  {3,5,0,0,0,1},
  {3,0,1,0,0,1},
  {3,2,3,0,0,1},
  {3,4,5,0,0,1},
  {3,0,1,2,0,1},
  {3,3,4,5,0,1},
  {3,0,1,2,3,1},
  {2,0,2,0,0,1},
  {3,0,0,0,0,2},
  {3,1,0,0,0,2},
  {3,2,0,0,0,2},
  {3,3,0,0,0,2},
  {3,4,0,0,0,2},
  {3,5,0,0,0,2},
  {3,6,0,0,0,2},
  {3,7,0,0,0,2},
  {3,8,0,0,0,2},
  {3,9,0,0,0,2},
  {3,10,0,0,0,2},
  {3,11,0,0,0,2},
  {3,0,1,0,0,2},
  {3,2,3,0,0,2},
  {3,4,5,0,0,2},
  {3,6,7,0,0,2},
  {3,8,9,0,0,2},
  {3,10,11,0,0,2},
  {3,0,1,6,0,2},
  {3,2,3,8,0,2},
  {3,4,5,10,0,2},
  {3,0,1,6,7,2},
  {3,2,3,8,9,2},
  {3,4,5,10,11,2},
  {1,0,0,0,0,2},
  {1,1,0,0,0,2},
  {1,6,0,0,0,2},
  {1,7,0,0,0,2},
  {1,0,1,0,0,2},
  {1,6,7,0,0,2},
  {2,0,1,0,0,2},
  {2,2,3,0,0,2},
  {2,6,7,0,0,2},
  {2,8,9,0,0,2}
};

const uint8_t table_7_3_2_3_3_4_twoCodeword[6][10] = {
  {3,0,1,2,3,4,0,0,0,1},
  {3,0,1,2,3,4,5,0,0,1},
  {2,0,1,2,3,6,0,0,0,2},
  {2,0,1,2,3,6,8,0,0,2},
  {2,0,1,2,3,6,7,8,0,2},
  {2,0,1,2,3,6,7,8,9,2}
};

// table 7.2-1 TS 38.321
const uint16_t table_7_2_1[16] = {
  5,    // row index 0
  10,   // row index 1
  20,   // row index 2
  30,   // row index 3
  40,   // row index 4
  60,   // row index 5
  80,   // row index 6
  120,  // row index 7
  160,  // row index 8
  240,  // row index 9
  320,  // row index 10
  480,  // row index 11
  960,  // row index 12
  1920, // row index 13
};
