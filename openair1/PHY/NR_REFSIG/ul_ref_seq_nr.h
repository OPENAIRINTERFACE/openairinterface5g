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

/***********************************************************************
*
* FILENAME    :  ul_ref_seq_nr.hh
*
* MODULE      :  generation of uplink reference sequence for nr
*
* DESCRIPTION :  variables to generate sequences with low peak to average power
*                see 3GPP TS 38.211 5.2.2 Low-PAPR sequence generation
*
************************************************************************/

#ifndef LOWPAPR_SEQUENCES_NR_H
#define LOWPAPR_SEQUENCES_NR_H

#include "PHY/defs_nr_UE.h"
#include "PHY/types.h"

#include "PHY/NR_REFSIG/ss_pbch_nr.h"

#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"


#ifdef DEFINE_VARIABLES_LOWPAPR_SEQUENCES_NR_H
#define EXTERN
#define INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
#else
#define EXTERN  extern
#endif

/************** DEFINE ********************************************/
#define U_GROUP_NUMBER               (30)    /* maximum number of sequence-group */
#define V_BASE_SEQUENCE_NUMBER       (2)     /* maximum number of base sequences */
#define SRS_SB_CONF                  (71)    /* maximum number of configuration for srs subcarrier allocation */

/************** VARIABLES *****************************************/

#define   INDEX_SB_LESS_32            (4)    /* index of dftsizes array for which subcarrier number is less than 36 */

EXTERN const uint16_t ul_allocated_re[SRS_SB_CONF]     /* number of uplink allocated resource elements */
/* this table is derivated from TS 38.211 Table 6.4.1.4.3-1: SRS bandwidth configuration which gives m_SRS_b then all possible values of sequence length is */
/* M_sc_b_SRS = m_SRS_b * N_SC_RB/K_TC with K_TC = 2 or K_TC = 4  as specified in TS 38.211 6.4.1.4.3 */
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= { /*
K_TC               3           3           3     3     3     3     3     3     3     3     3     3     3     3     3     3     3     3     3     3
m_SRS_b            4           8          12    16    20    24    28    32    36    40    44    48    52    56    60    64    68    72    76    80   */
             6,   12,   18,   24,   30,   36,   48,   60,   72,   84,   96,  108,  120,  132,  144,  156,  168,  180,  192,  204,  216,  228,  240,
/*
K_TC         3     3     3     3     3     3     3     3     3     3     3    3      3     3     3     3     3     3     3     3     6     3     3
m_SRS_b     84    88    92    96   100   104   108   112   116   120   128   132   136   144   152   160   168   176   184   192   100   208   216   */
           252,  264,  276,  288,  300,  312,  324,  336,  348,  360,  384,  396,  408,  432,  456,  480,  504,  528,  552,  576,  600,  624,  648,
/*
K_TC         3     6     3     3     3     3     6     6     6     6     6     6     6     6     6     6     6     6     6     6
m_SRS_b    224   116,  240   256   264   272   144   152   160   168   176   184   192   208   216   224   240   256   264   272                     */
           672,  696,  720,  768,  792,  816,  864,  912,  960, 1008, 1056, 1104, 1152, 1248, 1296, 1344, 1440, 1536, 1584, 1632,

/* below numbers have been added just to include all possible lengths already used for lte */
           540,  900,  972, 1080, 1200,
}
#endif
;

/* table of largest prime number lower than uplink allocated resource elements "ul_allocated_re" */
EXTERN const uint16_t ref_ul_primes[SRS_SB_CONF]
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*           6,   12,   18,   24,   30,   36,   48,   60,   72,   84,   96,  108,  120,  132,  144,  156,  168,  180,  192,  204,  216,  228,  240,  */
             5,   11,   17,   23,   29,   31,   47,   59,   71,   83,   89,  107,  113,  127,  139,  151,  167,  179,  191,  199,  211,  227,  239,

/*         252,  264,  276,  288,  300,  312,  324,  336,  348,  360,  384,  396,  408,  432,  456,  480,  504,  528,  552,  576,  600,  624,  648,  */
           251,  263,  271,  283,  293,  311,  317,  331,  347,  359,  383,  389,  401,  431,  449,  479,  503,  523,  547,  571,  599,  619,  647,

/*         672,  696,  720,  768,  792,  816,  864,  912,  960, 1008, 1056, 1104, 1152, 1248, 1296, 1344, 1440, 1536, 1584, 1632,                    */
           661,  691,  719,  761,  787,  811,  863,  911,  953,  997, 1051, 1103, 1151, 1237, 1291, 1327, 1439, 1531, 1583, 1627,

/*         540,  900,  972, 1080, 1200,                                                                                                              */
           523,  887,  971, 1069, 1193,
}
#endif
;

/* Low-PAPR base sequence; see TS 38.211 clause 5.2.2 */
EXTERN int16_t  *rv_ul_ref_sig[U_GROUP_NUMBER][V_BASE_SEQUENCE_NUMBER][SRS_SB_CONF];

/* 38.211 table Table 5.2.2.2-1: Definition of phi(n) for M_ZC = 6 */
EXTERN const char phi_M_ZC_6[6*U_GROUP_NUMBER]
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*        0   1   2   3   4   5  */
/*  0 */ -3, -1,  3,  3, -1, -3,
/*  1 */ -3,  3, -1, -1,  3, -3,
/*  2 */ -3, -3, -3,  3,  1, -3,
/*  3 */  1,  1,  1,  3, -1, -3,
/*  4 */  1,  1,  1, -3, -1,  3,
/*  5 */ -3,  1, -1, -3, -3, -3,
/*  6 */ -3,  1,  3, -3, -3, -3,
/*  7 */ -3, -1,  1, -3,  1, -1,
/*  8 */ -3, -1, -3,  1, -3, -3,
/*  9 */ -3, -3,  1, -3,  3, -3,
/* 10 */ -3,  1,  3,  1, -3, -3,
/* 11 */ -3, -1, -3,  1,  1, -3,
/* 12 */  1,  1,  3, -1, -3,  3,
/* 13 */  1,  1,  3,  3, -1,  3,
/* 14 */  1,  1,  1, -3,  3, -1,
/* 15 */ -1,  1,  1, -1,  3, -3,
/* 16 */ -3, -1, -1, -1,  3, -1,
/* 17 */ -3, -3, -1,  1, -1, -3,
/* 18 */ -3, -3, -3,  1, -3, -1,
/* 19 */ -3,  1,  1, -3, -1, -3,
/* 20 */ -3,  3, -3,  1,  1, -3,
/* 21 */ -3,  1, -3, -3, -3, -1,
/* 22 */  1,  1, -3,  3,  1,  3,
/* 23 */  1,  1, -3, -3,  1, -3,
/* 24 */  1,  1,  3, -1,  3,  3,
/* 25 */  1,  1, -3,  1,  3,  3,
/* 26 */  1,  1, -1, -1,  3, -1,
/* 27 */  1,  1, -1,  3, -1, -1,
/* 28 */  1,  1, -1,  3, -3, -1,
/* 29 */  1,  1, -3,  1, -1, -1,
}
#endif
;
/* Table 5.2.2.2-2: Definition of phi ( n ) for M ZC = 12  */
EXTERN const char phi_M_ZC_12[12*U_GROUP_NUMBER]
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*        0   1   2   3   4   5   6   7   8   9  10  11  */
/* 0  */  1, -1,  3,  1,  1, -1, -1, -1,  1,  3, -3,  1,
/* 1  */ -1, -1, -1, -1,  1, -3, -1,  3,  3, -1, -3,  1,
/* 2  */ -3,  1, -3, -3, -3,  3, -3, -1,  1,  1,  1, -3,
/* 3  */ -3,  3,  1,  3, -3,  1,  1,  1,  1,  3, -3,  3,
/* 4  */ -3,  1,  3, -1, -1, -3, -3, -1, -1,  3,  1, -3,
/* 5  */ -1,  1,  1, -1,  1,  3,  3, -1, -1, -3,  1, -3,
/* 6  */ -3, -3, -1,  3,  3,  3, -3,  3, -3,  1, -1, -3,
/* 7  */ -3,  3, -3,  3,  3, -3, -1, -1,  3,  3,  1, -3,
/* 8  */ -3, -1, -3, -1, -1, -3,  3,  3, -1, -1,  1, -3,
/* 9  */ -3,  3,  3,  3, -1, -3, -3, -1, -3,  1,  3, -3,
/* 10 */  1,  3, -3,  1,  3,  3,  3,  1, -1,  1, -1,  3,
/* 11 */ -1, -3,  3, -1, -3, -3, -3, -1,  1, -1,  1, -3,
/* 12 */  3,  1,  3,  1,  3, -3, -1,  1,  3,  1, -1, -3,
/* 13 */ -3, -3,  3,  3,  3, -3, -1,  1, -3,  3,  1, -3,
/* 14 */ -3, -1,  1, -3,  1,  3,  3,  3, -1, -3,  3,  3,
/* 15 */ -3, -3,  3,  1, -3, -3, -3, -1,  3, -1,  1,  3,
/* 16 */ -1,  1,  3, -3,  1, -1,  1, -1, -1, -3,  1, -1,
/* 17 */ -3, -1, -1,  1,  3,  1,  1, -1,  1, -1, -3,  1,
/* 18 */ -3, -1,  3, -3, -3, -1, -3,  1, -1, -3,  3,  3,
/* 19 */ -3, -3,  3, -3, -1,  3,  3,  3, -1, -3,  1, -3,
/* 20 */ -3,  1, -1, -1,  3,  3, -3, -1, -1, -3, -1, -3,
/* 21 */ -3,  1,  3,  3, -1, -1, -3,  3,  3, -3,  3, -3,
/* 22 */ -3, -1, -1, -3, -3, -1, -3,  3,  1,  3, -1, -3,
/* 23 */ -3, -1,  3,  1, -3, -1, -3,  3,  1,  3,  3,  1,
/* 24 */ -3,  3,  3,  1, -3,  3, -1,  1,  3, -3,  3, -3,
/* 25 */  3, -1, -3,  3, -3, -1,  3,  3,  3, -3, -1, -3,
/* 26 */  1, -1,  3, -1, -1, -1, -3, -1,  1,  1,  1, -3,
/* 27 */ -3,  3,  1, -3,  1,  3, -1, -1,  1,  3,  3,  3,
/* 28 */ -3,  3, -3,  3, -3, -3,  3, -1, -1,  1,  3, -3,
/* 29 */ -3,  3,  1, -1,  3,  3, -3,  1, -1,  1, -1,  1,
}
#endif
;

/* Table 5.2.2.2-3: Definition of phi (n ) for M_ZC = 18 */
EXTERN const char phi_M_ZC_18[18*U_GROUP_NUMBER]
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17 */
/* 0   */  -1,  3, -1, -3,  3,  1, -3, -1,  3, -3, -1, -1,  1,  1,  1, -1, -1, -1,
/* 1   */   3, -3,  3, -1,  1,  3, -3, -1, -3, -3, -1, -3,  3,  1, -1,  3, -3,  3,
/* 2   */  -3,  3,  1, -1, -1,  3, -3, -1,  1,  1,  1,  1,  1, -1,  3, -1, -3, -1,
/* 3   */  -3, -3,  3,  3,  3,  1, -3,  1,  3,  3,  1, -3, -3,  3, -1, -3, -1,  1,
/* 4   */   1,  1, -1, -1, -3, -1,  1, -3, -3, -3,  1, -3, -1, -1,  1, -1,  3,  1,
/* 5   */   3, -3,  1,  1,  3, -1,  1, -1, -1, -3,  1,  1, -1,  3,  3, -3,  3, -1,
/* 6   */  -3,  3, -1,  1,  3,  1, -3, -1,  1,  1, -3,  1,  3,  3, -1, -3, -3, -3,
/* 7   */   1,  1, -3,  3,  3,  1,  3, -3,  3, -1,  1,  1, -1,  1, -3, -3, -1,  3,
/* 8   */  -3,  1, -3, -3,  1, -3, -3,  3,  1, -3, -1, -3, -3, -3, -1,  1,  1,  3,
/* 9   */   3, -1,  3,  1, -3, -3, -1,  1, -3, -3,  3,  3,  3,  1,  3, -3,  3, -3,
/* 10  */  -3, -3, -3,  1, -3,  3,  1,  1,  3, -3, -3,  1,  3, -1,  3, -3, -3,  3,
/* 11  */  -3, -3,  3,  3,  3, -1, -1, -3, -1, -1, -1,  3,  1, -3, -3, -1,  3, -1,
/* 12  */  -3, -1, -3, -3,  1,  1, -1, -3, -1, -3, -1, -1,  3,  3, -1,  3,  1,  3,
/* 13  */   1,  1, -3, -3, -3, -3,  1,  3, -3,  3,  3,  1, -3, -1,  3, -1, -3,  1,
/* 14  */  -3,  3, -1, -3, -1, -3,  1,  1, -3, -3, -1, -1,  3, -3,  1,  3,  1,  1,
/* 15  */   3,  1, -3,  1, -3,  3,  3, -1, -3, -3, -1, -3, -3,  3, -3, -1,  1,  3,
/* 16  */  -3, -1, -3, -1, -3,  1,  3, -3, -1,  3,  3,  3,  1, -1, -3,  3, -1, -3,
/* 17  */  -3, -1,  3,  3, -1,  3, -1, -3, -1,  1, -1, -3, -1, -1, -1,  3,  3,  1,
/* 18  */  -3,  1, -3, -1, -1,  3,  1, -3, -3, -3, -1, -3, -3,  1,  1,  1, -1, -1,
/* 19  */   3,  3,  3, -3, -1, -3, -1,  3, -1,  1, -1, -3,  1, -3, -3, -1,  3,  3,
/* 20  */  -3,  1,  1, -3,  1,  1,  3, -3, -1, -3, -1,  3, -3,  3, -1, -1, -1, -3,
/* 21  */   1, -3, -1, -3,  3,  3, -1, -3,  1, -3, -3, -1, -3, -1,  1,  3,  3,  3,
/* 22  */  -3, -3,  1, -1, -1,  1,  1, -3, -1,  3,  3,  3,  3, -1,  3,  1,  3,  1,
/* 23  */   3, -1, -3,  1, -3, -3, -3,  3,  3, -1,  1, -3, -1,  3,  1,  1,  3,  3,
/* 24  */   3, -1, -1,  1, -3, -1, -3, -1, -3, -3, -1, -3,  1,  1,  1, -3, -3,  3,
/* 25  */  -3, -3,  1, -3,  3,  3,  3, -1,  3,  1,  1, -3, -3, -3,  3, -3, -1, -1,
/* 26  */  -3, -1, -1, -3,  1, -3,  3, -1, -1, -3,  3,  3, -3, -1,  3, -1, -1, -1,
/* 27  */  -3, -3,  3,  3, -3,  1,  3, -1, -3,  1, -1, -3,  3, -3, -1, -1, -1,  3,
/* 28  */  -1, -3,  1, -3, -3, -3,  1,  1,  3,  3, -3,  3,  3, -3, -1,  3, -3,  1,
/* 29  */  -3,  3,  1, -1, -1, -1, -1,  1, -1,  3,  3, -3, -1,  1,  3, -1,  3, -1,
}
#endif
;

/* Table 5.2.2.2-4: Definition of phi (n ) for M_ZC = 24 */
EXTERN const char phi_M_ZC_24[24*U_GROUP_NUMBER]
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23 */
/* 0   */  -1, -3,  3, -1,  3,  1,  3, -1,  1, -3, -1, -3, -1,  1,  3, -3, -1, -3,  3,  3,  3, -3, -3, -3,
/* 1   */  -1, -3,  3,  1,  1, -3,  1, -3, -3,  1, -3, -1, -1,  3, -3,  3,  3,  3, -3,  1,  3,  3, -3, -3,
/* 2   */  -1, -3, -3,  1, -1, -1, -3,  1,  3, -1, -3, -1, -1, -3,  1,  1,  3,  1, -3, -1, -1,  3, -3, -3,
/* 3   */   1, -3,  3, -1, -3, -1,  3,  3,  1, -1,  1,  1,  3, -3, -1, -3, -3, -3, -1,  3, -3, -1, -3, -3,
/* 4   */  -1,  3, -3, -3, -1,  3, -1, -1,  1,  3,  1,  3, -1, -1, -3,  1,  3,  1, -1, -3,  1, -1, -3, -3,
/* 5   */  -3, -1,  1, -3, -3,  1,  1, -3,  3, -1, -1, -3,  1,  3,  1, -1, -3, -1, -3,  1, -3, -3, -3, -3,
/* 6   */  -3,  3,  1,  3, -1,  1, -3,  1, -3,  1, -1, -3, -1, -3, -3, -3, -3, -1, -1, -1,  1,  1, -3, -3,
/* 7   */  -3,  1,  3, -1,  1, -1,  3, -3,  3, -1, -3, -1, -3,  3, -1, -1, -1, -3, -1, -1, -3,  3,  3, -3,
/* 8   */  -3,  1, -3,  3, -1, -1, -1, -3,  3,  1, -1, -3, -1,  1,  3, -1,  1, -1,  1, -3, -3, -3, -3, -3,
/* 9   */   1,  1, -1, -3, -1,  1,  1, -3,  1, -1,  1, -3,  3, -3, -3,  3, -1, -3,  1,  3, -3,  1, -3, -3,
/* 10  */  -3, -3, -3, -1,  3, -3,  3,  1,  3,  1, -3, -1, -1, -3,  1,  1,  3,  1, -1, -3,  3,  1,  3, -3,
/* 11  */  -3,  3, -1,  3,  1, -1, -1, -1,  3,  3,  1,  1,  1,  3,  3,  1, -3, -3, -1,  1, -3,  1,  3, -3,
/* 12  */   3, -3,  3, -1, -3,  1,  3,  1, -1, -1, -3, -1,  3, -3,  3, -1, -1,  3,  3, -3, -3,  3, -3, -3,
/* 13  */  -3,  3, -1,  3, -1,  3,  3,  1,  1, -3,  1,  3, -3,  3, -3, -3, -1,  1,  3, -3, -1, -1, -3, -3,
/* 14  */  -3,  1, -3, -1, -1,  3,  1,  3, -3,  1, -1,  3,  3, -1, -3,  3, -3, -1, -1, -3, -3, -3,  3, -3,
/* 15  */  -3, -1, -1, -3,  1, -3, -3, -1, -1,  3, -1,  1, -1,  3,  1, -3, -1,  3,  1,  1, -1, -1, -3, -3,
/* 16  */  -3, -3,  1, -1,  3,  3, -3, -1,  1, -1, -1,  1,  1, -1, -1,  3, -3,  1, -3,  1, -1, -1, -1, -3,
/* 17  */   3, -1,  3, -1,  1, -3,  1,  1, -3, -3,  3, -3, -1, -1, -1, -1, -1, -3, -3, -1,  1,  1, -3, -3,
/* 18  */   3,  1, -3,  1, -3, -3,  1, -3,  1, -3, -3, -3, -3, -3,  1, -3, -3,  1,  1, -3,  1,  1, -3, -3,
/* 19  */  -3, -3,  3,  3,  1, -1, -1, -1,  1, -3, -1,  1, -1,  3, -3, -1, -3, -1, -1,  1, -3,  3, -1, -3,
/* 20  */  -3, -3, -1, -1, -1, -3,  1, -1, -3, -1,  3, -3,  1, -3,  3, -3,  3,  3,  1, -1, -1,  1, -3, -3,
/* 21  */   3, -1,  1, -1,  3, -3,  1,  1,  3, -1, -3,  3,  1, -3,  3, -1, -1, -1, -1,  1, -3, -3, -3, -3,
/* 22  */  -3,  1, -3,  3, -3,  1, -3,  3,  1, -1, -3, -1, -3, -3, -3, -3,  1,  3, -1,  1,  3,  3,  3, -3,
/* 23  */  -3, -1,  1, -3, -1, -1,  1,  1,  1,  3,  3, -1,  1, -1,  1, -1, -1, -3, -3, -3,  3,  1, -1, -3,
/* 24  */  -3,  3, -1, -3, -1, -1, -1,  3, -1, -1,  3, -3, -1,  3, -3,  3, -3, -1,  3,  1,  1, -1, -3, -3,
/* 25  */  -3,  1, -1, -3, -3, -1,  1, -3, -1, -3,  1,  1, -1,  1,  1,  3,  3,  3, -1,  1, -1,  1, -1, -3,
/* 26  */  -1,  3, -1, -1,  3,  3, -1, -1, -1,  3, -1, -3,  1,  3,  1,  1, -3, -3, -3, -1, -3, -1, -3, -3,
/* 27  */   3, -3, -3, -1,  3,  3, -3, -1,  3,  1,  1,  1,  3, -1,  3, -3, -1,  3, -1,  3,  1, -1, -3, -3,
/* 28  */  -3,  1, -3,  1, -3,  1,  1,  3,  1, -3, -3, -1,  1,  3, -1, -3,  3,  1, -1, -3, -3, -3, -3, -3,
/* 29  */   3, -3, -1,  1,  3, -1, -1, -3, -1,  3, -1, -3, -1, -3,  3, -1,  3,  1,  1, -3,  3, -3, -3, -3,
}
#endif
;
/************** FUNCTION ******************************************/


EXTERN int16_t *base_sequence_36_or_larger(unsigned int M_ZC, unsigned int u, unsigned int v, unsigned int scaling, unsigned int if_dmrs_seq);


EXTERN int16_t *base_sequence_less_than_36(unsigned int M_ZC, unsigned int u, unsigned int scaling);
/*!
\brief This function generate the sounding reference symbol (SRS) for the uplink.
@param tables of srs
@param scaling amplitude of the reference signal
*/
void generate_ul_reference_signal_sequences(unsigned int scaling);
void free_ul_reference_signal_sequences(void);



// Supported for 100Mhz configuration - which has max 273 RBs
#define MAX_INDEX_DMRS_UL_ALLOCATED_REs 53


EXTERN const uint16_t dmrs_ul_allocated_res[MAX_INDEX_DMRS_UL_ALLOCATED_REs]     
/* Number of possible DMRS REs based on PRBs allocated for PUSCH. Array has values until 273 RBs (100Mhz BW)
   Number of PUSCH RBs allocated should be able to be expressed as 2topowerofn*3topowerofn*5tothepowerofn
   According to 3GPP spec 38.211 section 6.3.1.4
   Table used in calculating DMRS low papr type1 sequence for transform precoding                                    */
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*RBs      1,    2,     3,    4,    5,     6,    8,    9,    10,   12,   15,   16,                 */ 
           6,    12,    18,   24,   30,    36,   48,   54,   60,   72,   90,   96,    

/*RBs      18,   20,    24,   25,   27,    30,   32,   36,   40,   45,   48,   50,                 */ 
           108,  120,   144,  150,  162,   180,  192,  216,  240,  270,  288,  300,    

/*RBs      54,   60,    64,   72    75,    80,   81,   90,   96,   100,   (Bw 40Mhz) */ 
           324,  360,   384,  432,  450,   480,  486,  540,  576,  600,  

/*RBs      108,  120    125   128   135    144   150   160   162   180    192    200               */
           648,  720,   750,  768,  810,   864,  900,  960,  972,  1080,  1152,  1200,
           
/*RBs      216    225    240   243   250   256    270                        supported until 100Mhz */
           1296,  1350,  1440, 1458, 1500, 1536,  1620  

}
#endif
;

/* Table of largest prime number N_ZC < possible DMRS REs M_ZC, this array has values until 100Mhz
   According to 3GPP spec 38.211 section 5.2.2.1
   Table used in calculating DMRS low papr type1 sequence for transform precoding                              */
EXTERN const uint16_t dmrs_ref_ul_primes[MAX_INDEX_DMRS_UL_ALLOCATED_REs]
#ifdef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
= {
/*DMRS REs              6,    12,   18,   24,    30,    36,    48,   54,   60,   72,   90,   96,    */
                        5,    11,   17,   23,    29,    31,    47,   53,   59,   71,   89,   89,    

/*DMRS REs              108,  120,  144,  150,   162,   180,   192,  216,  240,  270,  288,  300,     */             
                        107,  113,  139,  149,   157,   179,   191,  211,  239,  269,  283,  293, 

/*DMRS REs              324,  360,  384,  432,   450,   480,   486,  540,  576,  600,                 */
                        317,  359,  383,  431,   449,   479,   479,  523,  571,  599,  
		   
/*DMRS REs              648,  720,  750,  768,   810,   864,   900,  960,  972,  1080, 1152, 1200,   */
                        647,  719,  743,  761,   809,   863,   887,  953,  971,  1069, 1151, 1193, 
           
/*DMRS REs              1296, 1350,  1440, 1458, 1500,  1536   1620  supported until 100Mhz */ 
                        1291, 1327,  1439, 1453, 1499,  1531,  1619
        

}
#endif
;

/// PUSCH DMRS for transform precoding
EXTERN int16_t  *gNB_dmrs_lowpaprtype1_sequence[U_GROUP_NUMBER][V_BASE_SEQUENCE_NUMBER][MAX_INDEX_DMRS_UL_ALLOCATED_REs];
EXTERN int16_t  *dmrs_lowpaprtype1_ul_ref_sig[U_GROUP_NUMBER][V_BASE_SEQUENCE_NUMBER][MAX_INDEX_DMRS_UL_ALLOCATED_REs];
int16_t  get_index_for_dmrs_lowpapr_seq(int16_t num_dmrs_res);
void     generate_lowpapr_typ1_refsig_sequences(unsigned int scaling);
void     free_gnb_lowpapr_sequences(void);


#undef INIT_VARIABLES_LOWPAPR_SEQUENCES_NR_H
#undef EXTERN

#endif /* SSS_NR_H */
