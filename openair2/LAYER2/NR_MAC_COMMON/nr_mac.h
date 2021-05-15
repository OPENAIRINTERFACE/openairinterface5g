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

/* \file       nr_mac.h
 * \brief      common MAC data structures and constants
 * \author     R. Knopp, K.H. HSU, G. Casati
 * \date       2019
 * \version    0.1
 * \company    Eurecom / NTUST / Fraunhofer IIS
 * \email:     knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */

#ifndef __LAYER2_NR_MAC_H__
#define __LAYER2_NR_MAC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TABLE_38213_13_1_NUM_INDEXES 15
#define TABLE_38213_13_2_NUM_INDEXES 14
#define TABLE_38213_13_3_NUM_INDEXES 9
#define TABLE_38213_13_4_NUM_INDEXES 16
#define TABLE_38213_13_5_NUM_INDEXES 9
#define TABLE_38213_13_6_NUM_INDEXES 10
#define TABLE_38213_13_7_NUM_INDEXES 12
#define TABLE_38213_13_8_NUM_INDEXES 8
#define TABLE_38213_13_9_NUM_INDEXES 4
#define TABLE_38213_13_10_NUM_INDEXES 8
#define TABLE_38213_13_11_NUM_INDEXES 16
#define TABLE_38213_13_12_NUM_INDEXES 14

// Definitions for MAC control and data
#define NR_BCCH_DL_SCH 3 // SI
#define NR_BCCH_BCH 5    // MIB
#define CCCH_PAYLOAD_SIZE_MAX 512 
#define RAR_PAYLOAD_SIZE_MAX  128
#define MAX_BWP_SIZE          275

typedef enum frequency_range_e {
  FR1 = 0,
  FR2
} frequency_range_t;

//  For both DL/UL-SCH
//  Except:
//   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
//   - UL/DL-SCH: padding
//   - UL-SCH:    MSG3 48-bits
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|F|   LCID    |
//  |       L       |
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|F|   LCID    |
//  |       L       |
//  |       L       |

//  For both DL/UL-SCH
//  For:
//   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
//   - UL/DL-SCH: padding, for single/multiple 1-oct padding CE(s)
//   - UL-SCH:    MSG3 48-bits
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|R|   LCID    |
//  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
//  L: The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
//  F: lenght of L is 0:8 or 1:16 bits wide
//  R: Reserved bit, set to zero.

typedef struct {
  uint8_t LCID: 6;    // octet 1 [5:0]
  uint8_t F: 1;       // octet 1 [6]
  uint8_t R: 1;       // octet 1 [7]
  uint8_t L: 8;       // octet 2 [7:0]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_SHORT;

typedef struct {
  uint8_t LCID: 6;    // octet 1 [5:0]
  uint8_t F: 1;       // octet 1 [6]
  uint8_t R: 1;       // octet 1 [7]
  uint8_t L1: 8;      // octet 2 [7:0]
  uint8_t L2: 8;      // octet 3 [7:0]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_LONG;

typedef struct {
  uint8_t LCID: 6;    // octet 1 [5:0]
  uint8_t R: 2;       // octet 1 [7:6]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_FIXED;

// BSR MAC CEs
// TS 38.321 ch. 6.1.3.1
// Short BSR for a specific logical channel group ID
typedef struct {
  uint8_t Buffer_size: 5;  // octet 1 LSB
  uint8_t LcgID: 3;        // octet 1 MSB
} __attribute__ ((__packed__)) NR_BSR_SHORT;

typedef NR_BSR_SHORT NR_BSR_SHORT_TRUNCATED;

// Long BSR for all logical channel group ID
typedef struct {
  uint8_t LcgID0: 1;        // octet 1 [0]
  uint8_t LcgID1: 1;        // octet 1 [1]
  uint8_t LcgID2: 1;        // octet 1 [2]
  uint8_t LcgID3: 1;        // octet 1 [3]
  uint8_t LcgID4: 1;        // octet 1 [4]
  uint8_t LcgID5: 1;        // octet 1 [5]
  uint8_t LcgID6: 1;        // octet 1 [6]
  uint8_t LcgID7: 1;        // octet 1 [7]
  uint8_t Buffer_size0: 8;  // octet 2 [7:0]
  uint8_t Buffer_size1: 8;  // octet 3 [7:0]
  uint8_t Buffer_size2: 8;  // octet 4 [7:0]
  uint8_t Buffer_size3: 8;  // octet 5 [7:0]
  uint8_t Buffer_size4: 8;  // octet 6 [7:0]
  uint8_t Buffer_size5: 8;  // octet 7 [7:0]
  uint8_t Buffer_size6: 8;  // octet 8 [7:0]
  uint8_t Buffer_size7: 8;  // octet 9 [7:0]
} __attribute__ ((__packed__)) NR_BSR_LONG;

typedef NR_BSR_LONG NR_BSR_LONG_TRUNCATED;

// 38.321 ch. 6.1.3.4
typedef struct {
  uint8_t TA_COMMAND: 6;  // octet 1 [5:0]
  uint8_t TAGID: 2;       // octet 1 [7:6]
} __attribute__ ((__packed__)) NR_MAC_CE_TA;

// single Entry PHR MAC CE
// TS 38.321 ch. 6.1.3.8
typedef struct {
  uint8_t PH: 6;    // octet 1 [5:0]
  uint8_t R1: 2;    // octet 1 [7:6]
  uint8_t PCMAX: 6; // octet 2 [5:0]
  uint8_t R2: 2;    // octet 2 [7:6]
} __attribute__ ((__packed__)) NR_SINGLE_ENTRY_PHR_MAC_CE;


// SP ZP CSI-RS Resource Set Activation/Deactivation MAC CE
// 38.321 ch. 6.1.3.19
typedef struct {
  uint8_t BWPID: 2;       // octet 1 [1:0]
  uint8_t CELLID: 5;      // octet 1 [6:2]
  uint8_t A_D: 1;         // octet 1 [7]
  uint8_t CSIRS_RSC_ID: 4; // octet 2 [3:0]
  uint8_t R: 4;            // octet 2 [7:4]
} __attribute__ ((__packed__)) NR_MAC_CE_SP_ZP_CSI_RS_RES_SET;

//TS 38.321 Sec 6.1.3.15, TCI State indicaton for UE-Specific PDCCH MAC CE
typedef struct {
  uint8_t CoresetId1: 3;   //Octect 1 [2:0]
  uint8_t ServingCellId: 5; //Octect 1 [7:3]
  uint8_t TciStateId: 7;   //Octect 2 [6:0]
  uint8_t CoresetId2: 1;   //Octect 2 [7]
} __attribute__ ((__packed__)) NR_TCI_PDCCH;

//TS 38.321 Sec 6.1.3.14, TCI State activation/deactivation for UE Specific PDSCH MAC CE
typedef struct {
  uint8_t BWP_Id: 2;       //Octect 1 [1:0]
  uint8_t ServingCellId: 5; //Octect 1 [6:2]
  uint8_t R: 1;            //Octect 1 [7]
  uint8_t T[];             //Octects 2 to MAX TCI States/8
} __attribute__ ((__packed__)) NR_TCI_PDSCH_APERIODIC_CSI;

//TS 6.1.3.16, SP CSI reporting on PUCCH Activation/Deactivation MAC CE
typedef struct {
  uint8_t BWP_Id: 2;      //Octect 1 [1:0]
  uint8_t ServingCellId: 5; //Octect 1 [6:2]
  uint8_t R1: 1;        //Octect 1 [7]
  uint8_t S0: 1;        //Octect 2 [0]
  uint8_t S1: 1;        //Octect 2 [1]
  uint8_t S2: 1;        //Octect 2 [2]
  uint8_t S3: 1;        //Octect 2 [3]
  uint8_t R2: 4;        //Octect 2 [7:4]
} __attribute__ ((__packed__)) NR_PUCCH_CSI_REPORTING;


//TS 38.321 sec 6.1.3.12
//SP CSI-RS / CSI-IM Resource Set Activation/Deactivation MAC CE
typedef struct {
  uint8_t BWP_ID: 2;
  uint8_t SCID: 5;
  uint8_t A_D: 1;
  uint8_t SP_CSI_RSID: 6;
  uint8_t IM: 1;
  uint8_t R1: 1;
  uint8_t SP_CSI_IMID: 6;
  uint8_t R2: 2;
  struct TCI_S {
    uint8_t TCI_STATE_ID: 6;
    uint8_t R: 2;
  } __attribute__ ((__packed__)) TCI_STATE;
} __attribute__ ((__packed__)) CSI_RS_CSI_IM_ACT_DEACT_MAC_CE;


//* RAR MAC subheader // TS 38.321 ch. 6.1.5, 6.2.2 *//
// - E: The Extension field is a flag indicating if the MAC subPDU including this MAC subheader is the last MAC subPDU or not in the MAC PDU
// - T: The Type field is a flag indicating whether the MAC subheader contains a Random Access Preamble ID or a Backoff Indicator (0, BI) (1, RAPID)
// - R: Reserved bit, set to "0"
// - BI: The Backoff Indicator field identifies the overload condition in the cell.
// - RAPID: The Random Access Preamble IDentifier field identifies the transmitted Random Access Preamble

/*!\brief RAR MAC subheader with RAPID */
typedef struct {
  uint8_t RAPID: 6;
  uint8_t T: 1;
  uint8_t E: 1;
} __attribute__ ((__packed__)) NR_RA_HEADER_RAPID;

/*!\brief RAR MAC subheader with Backoff Indicator */
typedef struct {
  uint8_t BI: 4;
  uint8_t R: 2;
  uint8_t T: 1;
  uint8_t E: 1;
} __attribute__ ((__packed__)) NR_RA_HEADER_BI;

// TS 38.321 ch. 6.2.3
typedef struct {
  uint8_t TA1: 7;         // octet 1 [6:0]
  uint8_t R: 1;           // octet 1 [7]
  uint8_t UL_GRANT_1: 3;  // octet 2 [2:0]
  uint8_t TA2: 5;         // octet 2 [7:3]
  uint8_t UL_GRANT_2: 8;  // octet 3 [7:0]
  uint8_t UL_GRANT_3: 8;  // octet 4 [7:0]
  uint8_t UL_GRANT_4: 8;  // octet 5 [7:0]
  uint8_t TCRNTI_1: 8;    // octet 6 [7:0]
  uint8_t TCRNTI_2: 8;    // octet 7 [7:0]
} __attribute__ ((__packed__)) NR_MAC_RAR;

// DCI pdu structures. Used by both gNB and UE.
typedef struct {
  uint16_t val;
  uint8_t nbits;
} dci_field_t;

typedef struct {

  uint8_t     format_indicator; //1 bit
  uint8_t     ra_preamble_index; //6 bits
  uint8_t     ss_pbch_index; //6 bits
  uint8_t     prach_mask_index; //4 bits
  uint8_t     mcs; //5 bits
  uint8_t     ndi; //1 bit
  uint8_t     rv; //2 bits
  uint8_t     harq_pid; //4 bits
  uint8_t     tpc; //2 bits
  uint8_t     short_messages_indicator; //2 bits
  uint8_t     short_messages; //8 bits
  uint8_t     tb_scaling; //2 bits
  uint8_t     pucch_resource_indicator; //3 bits
  uint8_t     system_info_indicator; //1 bit
  uint8_t     ulsch_indicator;
  uint8_t     slot_format_indicator_count;
  uint8_t     *slot_format_indicators;
 
  uint8_t     pre_emption_indication_count;
  uint16_t    *pre_emption_indications; //14 bit each
 
  uint8_t     block_number_count;
  uint8_t     *block_numbers;
  uint8_t     padding;

  dci_field_t mcs2; //variable
  dci_field_t ndi2; //variable
  dci_field_t rv2; //variable
  dci_field_t frequency_domain_assignment; //variable
  dci_field_t time_domain_assignment; //variable
  dci_field_t frequency_hopping_flag; //variable
  dci_field_t vrb_to_prb_mapping; //variable
  dci_field_t dai[2]; //variable
  dci_field_t pdsch_to_harq_feedback_timing_indicator; //variable
  dci_field_t carrier_indicator; //variable
  dci_field_t bwp_indicator; //variable
  dci_field_t prb_bundling_size_indicator; //variable
  dci_field_t rate_matching_indicator; //variable
  dci_field_t zp_csi_rs_trigger; //variable
  dci_field_t transmission_configuration_indication; //variable
  dci_field_t srs_request; //variable
  dci_field_t cbgti; //variable
  dci_field_t cbgfi; //variable
  dci_field_t srs_resource_indicator; //variable
  dci_field_t precoding_information; //variable
  dci_field_t csi_request; //variable
  dci_field_t ptrs_dmrs_association; //variable
  dci_field_t beta_offset_indicator; //variable
  dci_field_t cloded_loop_indicator; //variable
  dci_field_t ul_sul_indicator; //variable
  dci_field_t antenna_ports; //variable
  dci_field_t dmrs_sequence_initialization;
  dci_field_t reserved; //1_0/C-RNTI:10 bits, 1_0/P-RNTI: 6 bits, 1_0/SI-&RA-RNTI: 16 bits

} dci_pdu_rel15_t;

//  38.321 ch6.2.1, 38.331
#define DL_SCH_LCID_CCCH                           0x00
#define DL_SCH_LCID_DCCH                           0x01
#define DL_SCH_LCID_DCCH1                          0x02
#define DL_SCH_LCID_DTCH                           0x04
#define DL_SCH_LCID_RECOMMENDED_BITRATE            0x2F
#define DL_SCH_LCID_SP_ZP_CSI_RS_RES_SET_ACT       0x30
#define DL_SCH_LCID_PUCCH_SPATIAL_RELATION_ACT     0x31
#define DL_SCH_LCID_SP_SRS_ACTIVATION              0x32
#define DL_SCH_LCID_SP_CSI_REP_PUCCH_ACT           0x33
#define DL_SCH_LCID_TCI_STATE_IND_UE_SPEC_PDCCH    0x34
#define DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH    0x35
#define DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL 0x36
#define DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT   0X37
#define DL_SCH_LCID_DUPLICATION_ACT                0X38
#define DL_SCH_LCID_SCell_ACT_4_OCT                0X39
#define DL_SCH_LCID_SCell_ACT_1_OCT                0X3A
#define DL_SCH_LCID_L_DRX                          0x3B
#define DL_SCH_LCID_DRX                            0x3C
#define DL_SCH_LCID_TA_COMMAND                     0x3D
#define DL_SCH_LCID_CON_RES_ID                     0x3E
#define DL_SCH_LCID_PADDING                        0x3F

#define UL_SCH_LCID_CCCH1                          0x00
#define UL_SCH_LCID_SRB1                           0x01
#define UL_SCH_LCID_SRB2                           0x02
#define UL_SCH_LCID_SRB3                           0x03
#define UL_SCH_LCID_DTCH                           0x04
#define UL_SCH_LCID_CCCH                           0x34
#define UL_SCH_LCID_RECOMMENDED_BITRATE_QUERY      0x35
#define UL_SCH_LCID_MULTI_ENTRY_PHR_4_OCT          0x36
#define UL_SCH_LCID_CONFIGURED_GRANT_CONFIRMATION  0x37
#define UL_SCH_LCID_MULTI_ENTRY_PHR_1_OCT          0x38
#define UL_SCH_LCID_SINGLE_ENTRY_PHR               0x39
#define UL_SCH_LCID_C_RNTI                         0x3A
#define UL_SCH_LCID_S_TRUNCATED_BSR                0x3B
#define UL_SCH_LCID_L_TRUNCATED_BSR                0x3C
#define UL_SCH_LCID_S_BSR                          0x3D
#define UL_SCH_LCID_L_BSR                          0x3E
#define UL_SCH_LCID_PADDING                        0x3F

#define NR_MAX_NUM_LCID                32
#define NR_MAX_NUM_LCGID              8
#define MAX_RLC_SDU_SUBHEADER_SIZE          3

//===========
// PRACH defs
//===========

// ===============================================
// SSB to RO mapping public defines and structures
// ===============================================
#define MAX_SSB_PER_RO (16) // Maximum number of SSBs that can be mapped to a single RO
#define MAX_TDM (7) // Maximum nb of PRACH occasions TDMed in a slot
#define MAX_FDM (8) // Maximum nb of PRACH occasions FDMed in a slot

// PRACH occasion details
typedef struct prach_occasion_info {
  uint8_t start_symbol; // 0 - 13 (14 symbols in a slot)
  uint8_t fdm; // 0-7 (possible values of msg1-FDM: 1, 2, 4 or 8)
  uint8_t slot; // 0 - 159 (maximum number of slots in a 10ms frame - @ 240kHz)
  uint8_t frame; // 0 - 15 (maximum number of frames in a 160ms association pattern)
  uint8_t mapped_ssb_idx[MAX_SSB_PER_RO]; // List of mapped SSBs
  uint8_t nb_mapped_ssb;
  uint16_t format; // RO preamble format
} prach_occasion_info_t;

// PRACH occasion slot details
// A PRACH occasion slot is a series of PRACH occasions in time (symbols) and frequency
typedef struct prach_occasion_slot {
  prach_occasion_info_t prach_occasion[MAX_TDM][MAX_FDM]; // Starting symbol of each PRACH occasions in a slot
  uint8_t nb_of_prach_occasion_in_time;
  uint8_t nb_of_prach_occasion_in_freq;
} prach_occasion_slot_t;

//=========
// DCI defs
//=========

typedef enum {
  NR_DL_DCI_FORMAT_1_0 = 0,
  NR_DL_DCI_FORMAT_1_1,
  NR_DL_DCI_FORMAT_2_0,
  NR_DL_DCI_FORMAT_2_1,
  NR_DL_DCI_FORMAT_2_2,
  NR_DL_DCI_FORMAT_2_3,
  NR_UL_DCI_FORMAT_0_0,
  NR_UL_DCI_FORMAT_0_1
} nr_dci_format_t;

typedef enum {
  NR_RNTI_new = 0,
  NR_RNTI_C,
  NR_RNTI_RA,
  NR_RNTI_P,
  NR_RNTI_CS,
  NR_RNTI_TC,
  NR_RNTI_SP_CSI,
  NR_RNTI_SI,
  NR_RNTI_SFI,
  NR_RNTI_INT,
  NR_RNTI_TPC_PUSCH,
  NR_RNTI_TPC_PUCCH,
  NR_RNTI_TPC_SRS,
  NR_RNTI_MCS_C,
} nr_rnti_type_t;

typedef enum channel_bandwidth_e {
  bw_5MHz   = 0x1,
  bw_10MHz  = 0x2,
  bw_20MHz  = 0x4,
  bw_40MHz  = 0x8,
  bw_80MHz  = 0x16,
  bw_100MHz = 0x32
} channel_bandwidth_t;

typedef enum nr_ssb_and_cset_mux_pattern_type_e {
  NR_SSB_AND_CSET_MUX_PATTERN_TYPE1=1,
  NR_SSB_AND_CSET_MUX_PATTERN_TYPE2,
  NR_SSB_AND_CSET_MUX_PATTERN_TYPE3
} nr_ssb_and_cset_mux_pattern_type_t;

typedef enum {
  SFN_C_MOD_2_EQ_0,
  SFN_C_MOD_2_EQ_1,
  SFN_C_IMPOSSIBLE
} SFN_C_TYPE;

typedef struct Type0_PDCCH_CSS_config_s {
  int32_t num_rbs;
  int32_t num_symbols;
  int32_t rb_offset; // Offset from SSB RB0
  uint32_t type0_pdcch_ss_mux_pattern;
  uint16_t frame;
  SFN_C_TYPE sfn_c;
  uint32_t n_c;
  uint32_t n_0;
  uint32_t number_of_search_space_per_slot;
  uint32_t first_symbol_index;
  uint32_t search_space_duration;
  uint32_t ssb_length;
  uint32_t ssb_index;
  uint32_t cset_start_rb;
  bool active;
} NR_Type0_PDCCH_CSS_config_t;

#endif /*__LAYER2_MAC_H__ */

