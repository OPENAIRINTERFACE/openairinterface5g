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

/*!\file nrLDPCdecoder_defs.h
 * \brief Defines all constants and buffers for the LDPC decoder
 * \author Sebastian Wagner (TCL Communications) Email: <mailto:sebastian.wagner@tcl.com>
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_DEFS__H__
#define __NR_LDPC_DEFS__H__

// ==============================================================================
// DEFINES

/** Maximum lifting size */
#define NR_LDPC_ZMAX 384

/** Number of columns in BG1 */
#define NR_LDPC_NCOL_BG1 68
/** Number of rows in BG1 */
#define NR_LDPC_NROW_BG1 46
/** Number of edges/entries in BG1 */
#define NR_LDPC_NUM_EDGE_BG1 316
/** Number of check node (CN) groups in BG1
 A CN group is defined by its number of connected bit nodes. */
#define NR_LDPC_NUM_CN_GROUPS_BG1 9
/** First column in BG1 that is connected to only a single CN */
#define NR_LDPC_START_COL_PARITY_BG1 26

/** Number of columns in BG1 for rate 1/3 = 22/(68-2) */
#define NR_LDPC_NCOL_BG1_R13 NR_LDPC_NCOL_BG1
/** Number of columns in BG1 for rate 2/3 = 22/(35-2) */
#define NR_LDPC_NCOL_BG1_R23 35
/** Number of columns in BG1 for rate 8/9 ~ 22/(27-2) */
#define NR_LDPC_NCOL_BG1_R89 27

/** Number of bit node (BN) groups in BG1 for rate 1/3
 A BN group is defined by its number of connected CNs. */
#define NR_LDPC_NUM_BN_GROUPS_BG1_R13 30
/** Number of bit node (BN) groups in BG1 for rate 2/3 */
#define NR_LDPC_NUM_BN_GROUPS_BG1_R23 8
/** Number of bit node (BN) groups in BG1 for rate 8/9 */
#define NR_LDPC_NUM_BN_GROUPS_BG1_R89 5

/** Number of columns in BG2 */
#define NR_LDPC_NCOL_BG2 52
/** Number of rows in BG2 */
#define NR_LDPC_NROW_BG2 42
/** Number of edges/entries in BG2 */
#define NR_LDPC_NUM_EDGE_BG2 197
/** Number of check node (CN) groups in BG2
 A CN group is defined by its number of connected bit nodes. */
#define NR_LDPC_NUM_CN_GROUPS_BG2 6
/** First column in BG2 that is connected to only a single CN */
#define NR_LDPC_START_COL_PARITY_BG2 14

/** Number of columns in BG2 for rate 1/5 = 10/(52-2) */
#define NR_LDPC_NCOL_BG2_R15 NR_LDPC_NCOL_BG2
/** Number of columns in BG2 for rate 1/3 = 10/(32-2) */
#define NR_LDPC_NCOL_BG2_R13 32
/** Number of columns in BG2 for rate 2/3 = 10/(17-2) */
#define NR_LDPC_NCOL_BG2_R23 17

/** Number of bit node (BN) groups in BG2 for rate 1/5
 A BN group is defined by its number of connected CNs. */
#define NR_LDPC_NUM_BN_GROUPS_BG2_R15 13
/** Number of bit node (BN) groups in BG2 for rate 1/3 */
#define NR_LDPC_NUM_BN_GROUPS_BG2_R13 10
/** Number of bit node (BN) groups in BG2 for rate 2/3 */
#define NR_LDPC_NUM_BN_GROUPS_BG2_R23  6

/** Worst case size of the CN processing buffer */
#define NR_LDPC_SIZE_CN_PROC_BUF NR_LDPC_NUM_EDGE_BG1*NR_LDPC_ZMAX
/** Worst case size of the BN processing buffer */
#define NR_LDPC_SIZE_BN_PROC_BUF NR_LDPC_NUM_EDGE_BG1*NR_LDPC_ZMAX

/** Maximum number of possible input LLR = NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX */
#define NR_LDPC_MAX_NUM_LLR 27000

// ==============================================================================
// GLOBAL CONSTANT VARIABLES

/** Start addresses for the cnProcBuf for each CN group in BG1*/
static const uint32_t lut_startAddrCnGroups_BG1[NR_LDPC_NUM_CN_GROUPS_BG1] = {0, 1152, 8832, 43392, 61824, 75264, 81408, 88320, 92160};
/** Start addresses for the cnProcBuf for each CN group in BG2*/
static const uint32_t lut_startAddrCnGroups_BG2[NR_LDPC_NUM_CN_GROUPS_BG2] = {0, 6912, 37632, 54912, 61824, 67968};

/** Number of BNs of CN group for BG1.
  E.g. 10 means that there is a CN group where every CN is connected to 10 BNs */
static const uint8_t lut_numBnInCnGroups_BG1_R13[NR_LDPC_NUM_CN_GROUPS_BG1] = {3, 4,  5, 6, 7, 8, 9, 10, 19};
/** Number of rows/CNs in every CN group for rate = 1/3 BG1, e.g. 5 rows of CNs connected to 4 BNs */
static const uint8_t lut_numCnInCnGroups_BG1_R13[NR_LDPC_NUM_CN_GROUPS_BG1] = {1, 5, 18, 8, 5, 2, 2,  1,  4};
/** Number of rows/CNs in every CN group for rate = 2/3 BG1, e.g. 3 rows of CNs connected to 7 BNs */
static const uint8_t lut_numCnInCnGroups_BG1_R23[NR_LDPC_NUM_CN_GROUPS_BG1] = {1, 0,  0, 0, 3, 2, 2,  1,  4};
/** Number of rows/CNs in every CN group for rate = 8/9 BG1, e.g. 4 rows of CNs connected to 19 BNs */
static const uint8_t lut_numCnInCnGroups_BG1_R89[NR_LDPC_NUM_CN_GROUPS_BG1] = {1, 0,  0, 0, 0, 0, 0,  0,  4};

/** Number of connected BNs for every column in BG1 rate = 1/3, e.g. in first column all BNs are connected to 30 CNs */
static const uint8_t lut_numEdgesPerBn_BG1_R13[NR_LDPC_START_COL_PARITY_BG1] = {30, 28, 7, 11, 9, 4, 8, 12, 8, 7, 12, 10, 12, 11, 10, 7, 10, 10, 13, 7, 8, 11, 12, 5, 6, 6};
/** Number of connected BNs for every column in BG1 rate = 2/3, e.g. in first column all BNs are connected to 12 CNs */
static const uint8_t lut_numEdgesPerBn_BG1_R23[NR_LDPC_START_COL_PARITY_BG1] = {12, 11, 4,  5, 5, 3, 4,  5, 5, 3,  6,  6,  6,  6,  5, 3,  6,  5,  6, 4, 5,  6,  6, 3, 3, 2};
/** Number of connected BNs for every column in BG1 rate = 8/9, e.g. in first column all BNs are connected to 5 CNs */
static const uint8_t lut_numEdgesPerBn_BG1_R89[NR_LDPC_START_COL_PARITY_BG1] = {5,   4, 3,  3, 3, 3, 3,  3, 3, 3,  3,  3,  3,  3,  3, 3,  3,  3,  3, 3, 3,  3,  3, 2, 2, 2};

/** Number of BNs of CN group for BG2.
  E.g. 3 means that there is a CN group where every CN is connected to 3 BNs */
static const uint8_t lut_numBnInCnGroups_BG2_R15[NR_LDPC_NUM_CN_GROUPS_BG2] = {3,  4, 5, 6, 8, 10};
/** Number of rows/CNs in every CN group for rate = 1/5 BG2, e.g. 6 rows of CNs connected to 3 BNs */
static const uint8_t lut_numCnInCnGroups_BG2_R15[NR_LDPC_NUM_CN_GROUPS_BG2] = {6, 20, 9, 3, 2, 2};
/** Number of rows/CNs in every CN group for rate = 1/3 BG2, e.g. 8 rows of CNs connected to 4 BNs */
static const uint8_t lut_numCnInCnGroups_BG2_R13[NR_LDPC_NUM_CN_GROUPS_BG2] = {0,  8, 7, 3, 2, 2};
/** Number of rows/CNs in every CN group for rate = 2/3 BG2, e.g. 1 row of CNs connected to 4 BNs */
static const uint8_t lut_numCnInCnGroups_BG2_R23[NR_LDPC_NUM_CN_GROUPS_BG2] = {0,  1, 0, 2, 2, 2};

/** Number of connected BNs for every column in BG2 rate = 1/5, e.g. in first column all BNs are connected to 22 CNs */
static const uint8_t lut_numEdgesPerBn_BG2_R15[NR_LDPC_START_COL_PARITY_BG2] = {22, 23, 10, 5, 5, 14, 7, 13, 6, 8, 9, 16, 9, 12};
/** Number of connected BNs for every column in BG2 rate = 1/3, e.g. in first column all BNs are connected to 14 CNs */
static const uint8_t lut_numEdgesPerBn_BG2_R13[NR_LDPC_START_COL_PARITY_BG2] = {14, 16,  2, 4, 4,  6, 6,  8, 6, 6, 6, 13, 5,  7};
/** Number of connected BNs for every column in BG2 rate = 2/3, e.g. in first column all BNs are connected to 6 CNs */
static const uint8_t lut_numEdgesPerBn_BG2_R23[NR_LDPC_START_COL_PARITY_BG2] = { 6,  5,  2, 3, 3,  4, 3,  4, 3, 4, 3,  5, 2,  2};

// Number of groups for bit node processing
/** Number of connected CNs for every column/BN in BG1 for rate = 1/3, Worst case is BG1 with up to 30 CNs connected to one BN
  E.g. 42 parity BNs connected to single CN */
                                                                            // BG1: 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
static const uint8_t lut_numBnInBnGroups_BG1_R13[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = {42, 0, 0, 1, 1, 2, 4, 3, 1, 4, 3, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
/** Number of connected CNs for every column/BN in BG1 for rate = 2/3 */
static const uint8_t lut_numBnInBnGroups_BG1_R23[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = { 9, 1, 5, 3, 7, 8, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/** Number of connected CNs for every column/BN in BG1 for rate = 8/9 */
static const uint8_t lut_numBnInBnGroups_BG1_R89[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = { 1, 3,21, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/** Number of connected CNs for every column/BN in BG2 for rate = 1/5, Worst case is BG1 with up to 30 CNs connected to one BN
  E.g. 38 parity BNs connected to single CN */
                                                                            // BG2: 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
static const uint8_t lut_numBnInBnGroups_BG2_R15[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = {38, 0, 0, 0, 2, 1, 1, 1, 2, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0};
/** Number of connected CNs for every column/BN in BG2 for rate = 1/3 */
static const uint8_t lut_numBnInBnGroups_BG2_R13[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = {18, 1, 0, 2, 1, 5, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/** Number of connected CNs for every column/BN in BG2 for rate = 2/3 */
static const uint8_t lut_numBnInBnGroups_BG2_R23[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = { 3, 3, 5, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Start addresses for the bnProcBuf for each BN group
// BG1
/** Start address for every BN group within the BN processing buffer for BG1 rate = 1/3 */
static const uint32_t lut_startAddrBnGroups_BG1_R13[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = {0, 16128, 17664, 19584, 24192, 34944, 44160, 47616, 62976, 75648, 94080, 99072, 109824};
/** Start address for every BN group within the BN processing buffer for BG1 rate = 2/3 */
static const uint32_t lut_startAddrBnGroups_BG1_R23[NR_LDPC_NUM_BN_GROUPS_BG1_R23] = {0, 3456, 4224, 9984, 14592, 28032, 46464, 50688};
/** Start address for every BN group within the BN processing buffer for BG1 rate = 8/9 */
static const uint32_t lut_startAddrBnGroups_BG1_R89[NR_LDPC_NUM_BN_GROUPS_BG1_R89] = {0, 384, 2688, 26880, 28416};

/** Start address for every BN group within the LLR processing buffer for BG1 rate = 1/3 */
static const uint16_t lut_startAddrBnGroupsLlr_BG1_R13[NR_LDPC_NUM_BN_GROUPS_BG1_R13] = {0, 16128, 16512, 16896, 17664, 19200, 20352, 20736, 22272, 23424, 24960, 25344, 25728};
/** Start address for every BN group within the LLR processing buffer for BG1 rate = 2/3 */
static const uint16_t lut_startAddrBnGroupsLlr_BG1_R23[NR_LDPC_NUM_BN_GROUPS_BG1_R23] = {0, 3456, 3840, 5760, 6912, 9600, 12672, 13056};
/** Start address for every BN group within the LLR processing buffer for BG1 rate = 8/9 */
static const uint16_t lut_startAddrBnGroupsLlr_BG1_R89[NR_LDPC_NUM_BN_GROUPS_BG1_R89] = {0, 384, 1536, 9600, 9984};

// BG2
/** Start address for every BN group within the BN processing buffer for BG2 rate = 1/5 */
static const uint32_t lut_startAddrBnGroups_BG2_R15[NR_LDPC_NUM_BN_GROUPS_BG2_R15] = {0, 14592, 18432, 20736, 23424, 26496, 33408, 37248, 41856, 46848, 52224, 58368, 66816};
/** Start address for every BN group within the BN processing buffer for BG2 rate = 1/3 */
static const uint32_t lut_startAddrBnGroups_BG2_R13[NR_LDPC_NUM_BN_GROUPS_BG2_R13] = {0, 6912, 7680, 10752, 12672, 24192, 26880, 29952, 34944, 40320};
/** Start address for every BN group within the BN processing buffer for BG2 rate = 2/3 */
static const uint32_t lut_startAddrBnGroups_BG2_R23[NR_LDPC_NUM_BN_GROUPS_BG2_R23] = {0, 1152, 3456, 9216, 13824, 17664};

/** Start address for every BN group within the LLR processing buffer for BG2 rate = 1/5 */
static const uint16_t lut_startAddrBnGroupsLlr_BG2_R15[NR_LDPC_NUM_BN_GROUPS_BG2_R15] = {0, 14592, 15360, 15744, 16128, 16512, 17280, 17664, 18048, 18432, 18816, 19200, 19584};
/** Start address for every BN group within the LLR processing buffer for BG2 rate = 1/3 */
static const uint16_t lut_startAddrBnGroupsLlr_BG2_R13[NR_LDPC_NUM_BN_GROUPS_BG2_R13] = {0, 6912, 7296, 8064, 8448, 10368, 10752, 11136, 11520, 11904};
/** Start address for every BN group within the LLR processing buffer for BG2 rate = 2/3 */
static const uint16_t lut_startAddrBnGroupsLlr_BG2_R23[NR_LDPC_NUM_BN_GROUPS_BG2_R23] = {0,  1152,  2304,  4224,  5376, 6144};

/** Vector of 32 '1' in int8 for application with AVX2 */
static const int8_t ones256_epi8[32] __attribute__ ((aligned(32))) = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
/** Vector of 32 '0' in int8 for application with AVX2 */
static const int8_t zeros256_epi8[32] __attribute__ ((aligned(32))) = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/** Vector of 32 '127' in int8 for application with AVX2 */
static const int8_t maxLLR256_epi8[32] __attribute__ ((aligned(32))) = {127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};

/** Vector of 64 '1' in int8 for application with AVX512 */
static const int8_t ones512_epi8[64] __attribute__ ((aligned(64))) = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
/** Vector of 64 '0' in int8 for application with AVX512 */
static const int8_t zeros512_epi8[64] __attribute__ ((aligned(64))) = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/** Vector of 64 '127' in int8 for application with AVX512 */
static const int8_t maxLLR512_epi8[64] __attribute__ ((aligned(64))) = {127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};




#endif
