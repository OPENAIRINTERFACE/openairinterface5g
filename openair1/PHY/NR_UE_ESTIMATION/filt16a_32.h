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
#ifndef __FILT16A_H__
#define __FILT16A_H__
static const short filt16a_l0[16] = {16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_mm0[16] = {0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_r0[16] = {0, 0, 0, 0, 0, 4096, 8192, 12288, 16384, 20480, 24576, 28672, 0, 0, 0, 0};

static const short filt16a_m0[16] = {0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, -4096, -8192, -12288, 0, 0, 0, 0};

static const short filt16a_l1[16] = {20480, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_mm1[16] = {0, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_ml1[16] = {-4096, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_mr1[16] = {0, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, -4096, -8192, 0, 0, 0, 0};

static const short filt16a_r1[16] = {0, 0, 0, 0, 0, 0, 4096, 8192, 12288, 16384, 20480, 24576, 0, 0, 0, 0};

static const short filt16a_m1[16] = {-4096, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, -4096, -8192, 0, 0, 0, 0};

static const short filt16a_l2[16] = {24576, 20480, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_mm2[16] = {0, 0, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0};

static const short filt16a_ml2[16] = {-8192, -4096, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0};

static const short filt16a_mr2[16] = {0, 0, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, -4096, 0, 0, 0, 0};

static const short filt16a_r2[16] = {0, 0, 0, 0, 0, 0, 0, 4096, 8192, 12288, 16384, 20480, 0, 0, 0, 0};

static const short filt16a_m2[16] = {-8192, -4096, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, -4096, 0, 0, 0, 0};

static const short filt16a_l3[16] = {28672, 24576, 20480, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_mm3[16] = {0, 0, 0, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0};

static const short filt16a_ml3[16] = {-12288, -8192, -4096, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0};

static const short filt16a_r3[16] = {0, 0, 0, 0, 0, 0, 0, 0, 4096, 8192, 12288, 16384, 0, 0, 0, 0};

static const short filt16a_m3[16] = {-12288, -8192, -4096, 0, 4096, 8192, 12288, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0};

static const short filt16a_l0_dc[16] = {16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_r0_dc[16] = {0, 0, 0, 0, 0, 3276, 9830, 13107, 16384, 19660, 22937, 26214, 0, 0, 0, 0};

static const short filt16a_m0_dc[16] = {0, 4096, 8192, 12288, 16384, 13107, 6553, 3276, 0, -3277, -6554, -9831, 0, 0, 0, 0};

static const short filt16a_l1_dc[16] = {16384, 12288, 8192, 4096, 0, -4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_r1_dc[16] = {0, 0, 0, 0, 0, 0, 6553, 9830, 13107, 16384, 19660, 22937, 0, 0, 0, 0};

static const short filt16a_m1_dc[16] = {-4096, 0, 4096, 8192, 12288, 16384, 9830, 6553, 3276, 0, -3277, -6554, 0, 0, 0, 0};

static const short filt16a_l2_dc[16] = {26214, 22937, 19660, 16384, 13107, 9830, 6553, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_r2_dc[16] = {0, 0, 0, 0, 0, 0, 0, 4096, 8192, 12288, 16384, 20480, 0, 0, 0, 0};

static const short filt16a_m2_dc[16] = {-6554, -3277, 0, 3276, 6553, 6553, 16384, 12288, 8192, 4096, 0, -4096, 0, 0, 0, 0};

static const short filt16a_l3_dc[16] = {26214, 22937, 19660, 16384, 13107, 9830, 3276, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_r3_dc[16] = {0, 0, 0, 0, 0, 0, 0, 0, 4096, 8192, 12288, 16384, 0, 0, 0, 0};

static const short filt16a_m3_dc[16] = {-9831, -6554, -3277, 0, 3276, 6553, 9830, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0};

static const short filt16a_1[16] = {16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 0, 0, 0, 0};

static const short filt16a_2l0[16] = {16384, 12288, 8192, 4096, -4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_2r0[16] = {0, 4096, 8192, 12288, 16384, 20480, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_2l1[16] = {20480, 16384, 12288, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16a_2r1[16] = {-4096, 0, 4096, 8192, 12288, 16384, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*filter8*/
static const short filt8_l0[8] = {16384, 8192, 0, 0, 0, 0, 0, 0};

static const short filt8_mr0[8] = {0, 0, 0, 8192, 16384, 8192, 0, -8192};

static const short filt8_r0[8] = {0, 8192, 16384, 24576, 0, 0, 0, 0};

static const short filt8_m0[8] = {0, 8192, 16384, 8192, 0, 0, 0, 0};

static const short filt8_mm0[8] = {0, 0, 0, 8192, 16384, 8192, 0, 0};

static const short filt8_dcma[8] = {16384, 12288, 8192, 4096, 4096, 0, 0, 0};

static const short filt8_dcmb[8] = {0, 4096, 8192, 4096, 4096, 0, 0, 0};

static const short filt8_dcmc[8] = {0, 0, 0, 4096, 4096, 8192, 4096, 0};

static const short filt8_dcmd[8] = {0, 0, 0, 4096, 4096, 8192, 12288, 16384};

static const short filt8_dcl0[8] = {0, 0, 16384, 12288, 8192, 4096, 0, 0};

static const short filt8_dcr0[8] = {0, 0, 0, 4096, 8192, 12288, 16384, 0};

static const short filt8_dcl0_h[8] = {16384, 12288, 8192, 4096, 0, 0, 0, 0};

static const short filt8_dcr0_h[8] = {0, 4096, 8192, 12288, 16384, 0, 0, 0};

static const short filt8_l1[8] = {24576, 16384, 8192, 0, 0, 0, 0, 0};

static const short filt8_ml1[8] = {-8192, 0, 8192, 16384, 8192, 0, 0, 0};

static const short filt8_r1[8] = {0, 0, 8192, 16384, 0, 0, 0, 0};

static const short filt8_m1[8] = {0, 0, 8192, 16384, 8192, 0, 0, 0};

static const short filt8_mm1[8] = {0, 0, 0, 0, 8192, 16384, 8192, 0};

static const short filt8_dcl1[8] = {0, 0, 16384, 12288, 8192, 4096, 0, 0};

static const short filt8_dcr1[8] = {0, 0, 0, 4096, 8192, 12288, 16384, 0};

static const short filt8_dcl1_h[8] = {0, 16384, 12288, 8192, 4096, 0, 0, 0};

static const short filt8_dcr1_h[8] = {0, 0, 4096, 8192, 12288, 16384, 0, 0};

static const short filt8_ml2[8] = {13107, 9830, 6554, 3277, 0, 0, 0, 0};

static const short filt8_mr2[8] = {3277, 6554, 9830, 13107, 0, 0, 0, 0};

static const short filt8_rr1[8] = {8192, 12288, 16384, 20480, 0, 0, 0, 0};

static const short filt8_rr2[8] = {-4096, -8192, -12288, -16384, 0, 0, 0, 0};

static const short filt8_l2[8] = {0, 0, 13107, 9830, 6554, 3277, 0, 0};

static const short filt8_r2[8] = {0, 0, 3277, 6554, 9830, 13107, 0, 0};

static const short filt8_m2[8] = {0, 0, 0, 0, 13107, 9830, 6554, 3277};

static const short filt8_mm2[8] = {0, 0, 0, 0, 3277, 6554, 9830, 13107};

static const short filt8_rl2[8] = {19661, 22938, 26214, 29491, 0, 0, 0, 0};

static const short filt8_rm2[8] = {-3277, -6554, -9830, -13107, 0, 0, 0, 0}; //-3277,-6554,-9830,-13107

static const short filt8_l3[8] = {22938, 19661, 0, 0, 13107, 9830, 6554, 3277};

static const short filt8_r3[8] = {-7537, -4260, 0, 0, 3277, 6554, 9830, 13107}; //-6554,-3277

static const short filt8_rl3[8] = {0, 0, 19661, 22938, 0, 0, 0, 0};

static const short filt8_rr3[8] = {0, 0, -4260, -7537, 0, 0, 0, 0}; //-3277,-6554

static const short filt8_dcrl1[8] = {14895, 13405, 11916, 10426, 8937, 7447, 5958, 4468};

static const short filt8_dcrh1[8] = {2979, 1489, 0, 0, 0, 0, 0, 0};

static const short filt8_dcll1[8] = {13405, 14895, 0, 0, 0, 0, 0, 0};

static const short filt8_dclh1[8] = {1489, 2979, 4468, 5958, 7447, 8937, 10426, 11916};

static const short filt8_dcrl2[8] = {0, 0, 0, 0, 14895, 13405, 11916, 10426};

static const short filt8_dcrh2[8] = {
    8937,
    7447,
    5958,
    4468,
    2979,
    1489,
    0,
    0,
};

static const short filt8_dcll2[8] = {7447, 8937, 10426, 11916, 13405, 14895, 0, 0};

static const short filt8_dclh2[8] = {0, 0, 0, 0, 1489, 2979, 4468, 5958};

static const short filt8_avlip0[8] = {16384, 16384, 16384, 16384, 16384, 16384, 16384, 15019};

static const short filt8_avlip1[8] = {13653, 12288, 10923, 9557, 8192, 6827, 5461, 4096};

static const short filt8_avlip2[8] = {2731, 1365, 0, 0, 0, 0, 0, 0};

static const short filt8_avlip3[8] = {2731, 4096, 5461, 6827, 8192, 9557, 10923, 12288};

static const short filt8_avlip4[8] = {13653, 15019, 16384, 15019, 13653, 12288, 10923, 9557};

static const short filt8_avlip5[8] = {8192, 6827, 5461, 4096, 2731, 1365, 0, 0};

static const short filt8_avlip6[8] = {13653, 15019, 16384, 16384, 16384, 16384, 16384, 16384};

// Comb size 2
static const short filt8_start[8] = {12288, 8192, 4096, 0, 0, 0, 0, 0};

static const short filt8_start_shift2[8] = {0, 0, 12288, 8192, 4096, 0, 0, 0};

static const short filt8_middle2[8] = {4096, 8192, 8192, 8192, 4096, 0, 0, 0};

static const short filt8_middle4[8] = {0, 0, 4096, 8192, 8192, 8192, 4096, 0};

static const short filt8_end[8] = {4096, 8192, 12288, 16384, 0, 0, 0, 0};

static const short filt8_end_shift2[8] = {0, 0, 4096, 8192, 12288, 16384, 0, 0};

// Comb size 4
static const short filt16_start[16] = {12288, 8192, 8192, 8192, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16_middle4[16] = {4096, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 4096, 0, 0, 0, 0, 0, 0, 0};

static const short filt16_end[16] = {4096, 8192, 8192, 8192, 12288, 16384, 16384, 16384, 0, 0, 0, 0, 0, 0, 0, 0};

// CSI-RS
static const short filt24_start[24] = {12288, 11605, 10923, 10240, 9557, 8875, 8192, 7509, 6827, 6144, 5461, 4779,
                                       0,     0,     0,     0,     0,    0,    0,    0,    0,    0,    0,    0};

static const short filt24_end[24] = {4096,  4779,  5461,  6144,  6827,  7509,  8192,  8875,  9557,  10240, 10923, 11605,
                                     16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384};

static const short filt24_middle[24] = {4096,  4779,  5461,  6144,  6827, 7509, 8192, 8875, 9557, 10240, 10923, 11605,
                                        12288, 11605, 10923, 10240, 9557, 8875, 8192, 7509, 6827, 6144,  5461,  4779};

// UL
static const short filt16_ul_p0[16] = {4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt16_ul_p1p2[16] = {4096, 4096, 4096, 4096, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 0, 0, 0, 0};

static const short filt16_ul_middle[16] =
    {2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048};

static const short filt16_ul_last[16] = {4096, 4096, 4096, 4096, 8192, 8192, 8192, 8192, 0, 0, 0, 0, 0, 0, 0, 0};

static const short filt8_rep4[8] = {16384, 16384, 16384, 16384, 0, 0, 0, 0};

// DL
// DMRS_Type1
// using UL filter

// DMRS_Type2
static const short filt16_dl_first_type2[16] = {16384, 16384, 16384, 8192, 8192, 8192, 8192, 8192, 8192, 0, 0, 0, 0, 0, 0};

static const short filt16_dl_middle_type2[16] =
    {8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 0, 0, 0, 0};

static const short filt16_dl_last_type2[16] = {8192, 8192, 8192, 8192, 8192, 8192, 16384, 16384, 16384, 0, 0, 0, 0, 0, 0, 0};
#endif
