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

/*! \file PHY/LTE_TRANSPORT/transport_commont.h
* \brief data structures for PDSCH/DLSCH/PUSCH/ULSCH physical and transport channel descriptors (TX/RX) common to both eNB/UE
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
* \note
* \warning
*/
#ifndef __TRANSPORT_COMMON__H__
#define __TRANSPORT_COMMON__H__
#include "PHY/defs_common.h"
#include "dci.h"
#include "mdci.h"
//#include "uci.h"
//#ifndef STANDALONE_COMPILE
//  #include "UTIL/LISTS/list.h"
//#endif

#define MOD_TABLE_QPSK_OFFSET 1
#define MOD_TABLE_16QAM_OFFSET 5
#define MOD_TABLE_64QAM_OFFSET 21
#define MOD_TABLE_PSS_OFFSET 85

// structures below implement 36-211 and 36-212

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */



#define NSOFT 1827072
#define LTE_NULL 2

// maximum of 3 segments before each coding block if data length exceeds 6144 bits.

#define MAX_NUM_DLSCH_SEGMENTS 13
#define MAX_NUM_ULSCH_SEGMENTS MAX_NUM_DLSCH_SEGMENTS
#define MAX_DLSCH_PAYLOAD_BYTES (MAX_NUM_DLSCH_SEGMENTS*768)
#define MAX_ULSCH_PAYLOAD_BYTES (MAX_NUM_ULSCH_SEGMENTS*768)

#define MAX_NUM_CHANNEL_BITS (14*1200*6)  // 14 symbols, 1200 REs, 12 bits/RE
#define MAX_NUM_RE (14*1200)

#if !defined(SI_RNTI)
  #define SI_RNTI  (rnti_t)0xffff
  #define SI_RNTI_MBMS  (rnti_t)0xfff9
#endif
#if !defined(M_RNTI)
  #define M_RNTI   (rnti_t)0xfffd
#endif
#if !defined(P_RNTI)
  #define P_RNTI   (rnti_t)0xfffe
#endif
#if !defined(CBA_RNTI)
  #define CBA_RNTI (rnti_t)0xfff4
#endif
#if !defined(C_RNTI)
  #define C_RNTI   (rnti_t)0x1234
#endif
// These are the codebook indexes according to Table 6.3.4.2.3-1 of 36.211
//1 layer
#define PMI_2A_11  0
#define PMI_2A_1m1 1
#define PMI_2A_1j  2
#define PMI_2A_1mj 3
//2 layers
#define PMI_2A_R1_10 0
#define PMI_2A_R1_11 1
#define PMI_2A_R1_1j 2

typedef enum { SEARCH_EXIST=0,
               SEARCH_EXIST_OR_FREE,
               SEARCH_EXIST_RA
             } find_type_t;

typedef enum {
  SCH_IDLE=0,
  ACTIVE,
  CBA_ACTIVE,
  DISABLED
} SCH_status_t;


typedef enum {
  CEmodeA = 0,
  CEmodeB = 1
} CEmode_t;

#define PUSCH_x 2
#define PUSCH_y 3

typedef enum {
  pucch_format1=0,
  pucch_format1a,
  pucch_format1b,
  pucch_format1b_csA2,
  pucch_format1b_csA3,
  pucch_format1b_csA4,
  pucch_format2,
  pucch_format2a,
  pucch_format2b,
  pucch_format3    // PUCCH format3
} PUCCH_FMT_t;

typedef enum {
  SR,
  HARQ,
  CQI,
  HARQ_SR,
  HARQ_CQI,
  SR_CQI,
  HARQ_SR_CQI
} UCI_type_t;

typedef enum {
  NOCE,
  CEMODEA,
  CEMODEB
} UE_type_t;

typedef enum {
  SI_PDSCH=0,
  RA_PDSCH,
  P_PDSCH,
  PDSCH,
  PDSCH1,
  PMCH
} PDSCH_t;

typedef enum {
  rx_standard=0,
  rx_IC_single_stream,
  rx_IC_dual_stream,
  rx_SIC_dual_stream
} RX_type_t;


typedef enum {
  DCI_COMMON_SPACE,
  DCI_UE_SPACE
} dci_space_t;

typedef struct {
  uint8_t f_ra;
  uint8_t t0_ra;
  uint8_t t1_ra;
  uint8_t t2_ra;
} PRACH_TDD_PREAMBLE_MAP_elem;
typedef struct {
  uint8_t num_prach;
  PRACH_TDD_PREAMBLE_MAP_elem map[6];
} PRACH_TDD_PREAMBLE_MAP;

typedef struct {
  uint16_t slss_id;
  uint8_t *slmib;
} SLSS_t;

typedef struct {
  // SL Configuration
  /// Number of SL resource blocks (1-100)
  uint32_t N_SL_RB;
  /// prb-start (0-99)
  uint32_t prb_Start;
  /// prb-End (0-99)
  uint32_t prb_End;
  /// SL-OffsetIndicator (0-10239)
  uint32_t SL_OffsetIndicator;
  /// PSCCH subframe bitmap, first 64-bits (up to 40 bits for Rel 12)
  uint64_t bitmap1;
  /// PSCCH subframe bitmap, 2nd 64-bits (up to 100 bits for Rel 14)
  uint64_t bitmap2;

  // SCI parameters
  /// npscch resource index
  uint32_t n_pscch;
  /// format of SCI (0,1)
  uint32_t format;
  /// SCI0 frequency hopping flag
  uint32_t freq_hopping_flag;
  /// SCI0 Resource Block Coding
  uint32_t resource_block_coding;
  /// SCI0 Time Resource Pattern for SLSCH
  uint32_t time_resource_pattern;
  /// SCI0 MCS for SLSCH
  uint32_t mcs;
  /// SCI0 Timing advance indication for SLSCH
  uint32_t timing_advance_indication;
  /// SCI0 Group Destination ID for SLSCH
  uint32_t group_destination_id;

  // SLSCH Parameters
  /// Number of Subbands (36.213 14.1.1.2)
  uint32_t Nsb;
  /// N_RB_HO (36.213 14.1.1.2)
  uint32_t N_RB_HO;
  /// n_ss_PSSCH (36.211 9.2.4)
  uint32_t n_ss_PSSCH;
  /// n_ssf_PSSCH
  uint32_t n_ssf_PSSCH;
  /// cinit (36.331 hoppingParameter-r12)
  uint32_t cinit;
  /// redundancy version
  uint32_t rvidx;
  /// n_prime_VRB (36.213 14.1.1.2.1)
  uint32_t n_prime_VRB;
  /// M_RB_PSSCH_RP (36.213 14.1.3
  uint32_t M_RB_PSSCH_RP;
  /// n_prime_PRB (36.213 14.1.1.4
  uint32_t n_prime_PRB;
  /// m_nprime_PRB_PSSCH (36.213 14.1.3)
  uint32_t m_nprime_PRB_PSCCH;
  /// payload length
  int payload_length;
  /// pointer to payload
  uint8_t *payload;
} SLSCH_t;

typedef struct {
  /// payload length
  int payload_length;
  uint8_t payload[100];
} SLDCH_t;

#define TTI_SYNC 0
#define SLSS 1
#define SLDCH 2
#define SLSCH 3

typedef struct UE_tport_header_s {
  int packet_type;
  uint16_t absSF;
} UE_tport_header_t;

typedef struct UE_tport_s {
  UE_tport_header_t header;
  union {
    SLSS_t slss;
    SLDCH_t sldch;
    SLSCH_t slsch;
  };
  uint8_t payload[1500];
} UE_tport_t;


/**@}*/
#endif
