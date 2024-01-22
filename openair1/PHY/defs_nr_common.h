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

/*! \file PHY/defs_nr_common.h
 \brief Top-level defines and structure definitions
 \author Guy De Souza
 \date 2018
 \version 0.1
 \company Eurecom
 \email: desouza@eurecom.fr
 \note
 \warning
*/

#ifndef __PHY_DEFS_NR_COMMON__H__
#define __PHY_DEFS_NR_COMMON__H__

#include "PHY/impl_defs_top.h"
#include "defs_common.h"
#include "nfapi_nr_interface_scf.h"
#include "impl_defs_nr.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

#define nr_subframe_t lte_subframe_t
#define nr_slot_t lte_subframe_t

#define MAX_NUM_SUBCARRIER_SPACING 5
#define NR_MAX_OFDM_SYMBOL_SIZE 8192

#define NR_SYMBOLS_PER_SLOT NR_NUMBER_OF_SYMBOLS_PER_SLOT

#define ONE_OVER_SQRT2_Q15 23170
#define ONE_OVER_TWO_Q15 16384

#define NR_MOD_TABLE_SIZE_SHORT 686
#define NR_MOD_TABLE_BPSK_OFFSET 1
#define NR_MOD_TABLE_QPSK_OFFSET 3
#define NR_MOD_TABLE_QAM16_OFFSET 7
#define NR_MOD_TABLE_QAM64_OFFSET 23
#define NR_MOD_TABLE_QAM256_OFFSET 87

#define NR_PSS_LENGTH 127
#define NR_SSS_LENGTH 127

#define NR_MAX_PRS_LENGTH 3264 //272*6(max allocation per RB)*2(QPSK)
#define NR_MAX_PRS_INIT_LENGTH_DWORD 102 // ceil(NR_MAX_CSI_RS_LENGTH/32)
#define NR_MAX_NUM_PRS_SYMB 12
#define NR_MAX_PRS_COMB_SIZE 12
#define NR_MAX_PRS_RESOURCES_PER_SET 64
#define NR_MAX_PRS_MUTING_PATTERN_LENGTH 32

#define NR_PBCH_DMRS_LENGTH 144 // in mod symbols
#define NR_PBCH_DMRS_LENGTH_DWORD 10 // ceil(2(QPSK)*NR_PBCH_DMRS_LENGTH/32)

/*used for the resource mapping*/
#define NR_MAX_PDCCH_DMRS_LENGTH 576 // 16(L)*2(QPSK)*3(3 DMRS symbs per REG)*6(REG per CCE)
#define NR_MAX_PDCCH_SIZE 8192 // It seems it is the max polar coded block size
#define NR_MAX_DCI_PAYLOAD_SIZE 64
#define NR_MAX_DCI_SIZE 1728 //16(L)*2(QPSK)*9(12 RE per REG - 3(DMRS))*6(REG per CCE)
#define NR_MAX_DCI_SIZE_DWORD 54 // ceil(NR_MAX_DCI_SIZE/32)

#define NR_MAX_PDCCH_AGG_LEVEL 16 // 3GPP TS 38.211 V15.8 Section 7.3.2 Table 7.3.2.1-1: Supported PDCCH aggregation levels

#define NR_MAX_NB_PORTS 32

#define NR_MAX_PDSCH_TBS 3824

#define MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER 36

#define MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER 34

#define MAX_NUM_NR_RE (4*14*273*12)

#define MAX_NUM_NR_SRS_SYMBOLS 4
#define MAX_NUM_NR_SRS_AP 4

#define NR_NB_NSCID 2

#define MAX_DELAY_COMP 20

typedef enum {
  NR_MU_0=0,
  NR_MU_1,
  NR_MU_2,
  NR_MU_3,
  NR_MU_4,
} nr_numerology_index_e;

typedef enum{
  nr_ssb_type_A = 0,
  nr_ssb_type_B,
  nr_ssb_type_C,
  nr_ssb_type_D,
  nr_ssb_type_E
} nr_ssb_type_e;

typedef struct {
  uint8_t k_0_p[MAX_NUM_NR_SRS_AP][MAX_NUM_NR_SRS_SYMBOLS];
  uint8_t srs_generated_signal_bits;
  int32_t **srs_generated_signal;
  nfapi_nr_srs_pdu_t srs_pdu;
} nr_srs_info_t;

typedef struct {
  uint16_t csi_gold_init;
  uint32_t ***nr_gold_csi_rs;
  uint8_t csi_rs_generated_signal_bits;
  int32_t **csi_rs_generated_signal;
  bool csi_im_meas_computed;
  uint32_t interference_plus_noise_power;
} nr_csi_info_t;

typedef struct NR_DL_FRAME_PARMS NR_DL_FRAME_PARMS;

typedef uint32_t (*get_samples_per_slot_t)(int slot, const NR_DL_FRAME_PARMS *fp);
typedef uint32_t (*get_slot_from_timestamp_t)(openair0_timestamp timestamp_rx, const NR_DL_FRAME_PARMS *fp);

typedef uint32_t (*get_samples_slot_timestamp_t)(int slot, const NR_DL_FRAME_PARMS *fp, uint8_t sl_ahead);

struct NR_DL_FRAME_PARMS {
  /// frequency range
  nr_frequency_range_e freq_range;
  //  /// Placeholder to replace overlapping fields below
  //  nfapi_nr_rf_config_t rf_config;
  /// Placeholder to replace SSB overlapping fields below
  //  nfapi_nr_sch_config_t sch_config;
  /// Number of resource blocks (RB) in DL
  int N_RB_DL;
  /// Number of resource blocks (RB) in UL
  int N_RB_UL;
  /// Number of resource blocks (RB) in SL
  int N_RB_SL;
  ///  total Number of Resource Block Groups: this is ceil(N_PRB/P)
  uint8_t N_RBG;
  /// Total Number of Resource Block Groups SubSets: this is P
  uint8_t N_RBGS;
  /// NR Band
  uint16_t nr_band;
  /// DL carrier frequency
  uint64_t dl_CarrierFreq;
  /// UL carrier frequency
  uint64_t ul_CarrierFreq;
  /// SL carrier frequency
  uint64_t sl_CarrierFreq;
  /// TX attenuation
  uint32_t att_tx;
  /// RX attenuation
  uint32_t att_rx;
  ///  total Number of Resource Block Groups: this is ceil(N_PRB/P)
  /// Frame type (0 FDD, 1 TDD)
  frame_type_t frame_type;
  uint8_t tdd_config;
  /// Sidelink Cell ID
  uint16_t Nid_SL;
  /// Cell ID
  uint16_t Nid_cell;
  /// subcarrier spacing (15,30,60,120)
  uint32_t subcarrier_spacing;
  /// 3/4 sampling
  uint8_t threequarter_fs;
  /// Size of FFT
  uint16_t ofdm_symbol_size;
  /// Number of prefix samples in all but first symbol of slot
  uint16_t nb_prefix_samples;
  /// Number of prefix samples in first symbol of slot
  uint16_t nb_prefix_samples0;
  /// Carrier offset in FFT buffer for first RE in PRB0
  uint16_t first_carrier_offset;
  /// Number of OFDM/SC-FDMA symbols in one slot
  uint16_t symbols_per_slot;
  /// Number of slots per subframe
  uint16_t slots_per_subframe;
  /// Number of slots per frame
  uint16_t slots_per_frame;
  /// Number of samples in a subframe
  uint32_t samples_per_subframe;
  /// Number of samples in current slot
  get_samples_per_slot_t get_samples_per_slot;
  /// slot calculation from timestamp
  get_slot_from_timestamp_t get_slot_from_timestamp;
  /// Number of samples before slot
  get_samples_slot_timestamp_t get_samples_slot_timestamp;
  /// Number of samples in 0th and center slot of a subframe
  uint32_t samples_per_slot0;
  /// Number of samples in other slots of the subframe
  uint32_t samples_per_slotN0;
  /// Number of samples in a radio frame
  uint32_t samples_per_frame;
  /// Number of samples in a subframe without CP
  uint32_t samples_per_subframe_wCP;
  /// Number of samples in a slot without CP
  uint32_t samples_per_slot_wCP;
  /// Number of samples in a radio frame without CP
  uint32_t samples_per_frame_wCP;
  /// NR numerology index [0..5] as specified in 38.211 Section 4 (mu). 0=15khZ SCS, 1=30khZ, 2=60kHz, etc
  uint8_t numerology_index;
  /// Number of Physical transmit antennas in node (corresponds to nrOfAntennaPorts)
  uint8_t nb_antennas_tx;
  /// Number of Receive antennas in node
  uint8_t nb_antennas_rx;
  /// Number of common transmit antenna ports in eNodeB (1 or 2)
  uint8_t nb_antenna_ports_gNB;
  /// Cyclic Prefix for DL (0=Normal CP, 1=Extended CP)
  lte_prefix_type_t Ncp;
  /// sequence which is computed based on carrier frequency and numerology to rotate/derotate each OFDM symbol according to Section 5.3 in 38.211
  /// First dimension is for the direction of the link (0 DL, 1 UL, 2 SL)
  c16_t symbol_rotation[3][224];
  /// sequence used to compensate the phase rotation due to timeshifted OFDM symbols
  /// First dimenstion is for different CP lengths
  c16_t timeshift_symbol_rotation[4096*2] __attribute__ ((aligned (16)));
  /// Table used to apply the delay compensation in DL/UL
  c16_t delay_table[2 * MAX_DELAY_COMP + 1][NR_MAX_OFDM_SYMBOL_SIZE];
  /// SRS configuration from TS 38.331 RRC
  SRS_NR srs_nr;
  /// Power used by SSB in order to estimate signal strength and path loss
  int ss_PBCH_BlockPower;
  /// for NR TDD management
  TDD_UL_DL_configCommon_t  *p_tdd_UL_DL_Configuration;

  TDD_UL_DL_configCommon_t  *p_tdd_UL_DL_ConfigurationCommon2;

  TDD_UL_DL_SlotConfig_t *p_TDD_UL_DL_ConfigDedicated;

  /// TDD configuration
  uint16_t tdd_uplink_nr[2*NR_MAX_SLOTS_PER_FRAME]; /* this is a bitmap of symbol of each slot given for 2 frames */

  uint8_t half_frame_bit;

  //SSB related params
  /// Start in Subcarrier index of the SSB block
  uint16_t ssb_start_subcarrier;
  /// SSB type
  nr_ssb_type_e ssb_type;
  /// Max number of SSB in frame
  uint8_t Lmax;
  /// SS block pattern (max 64 ssb, each bit is on/off ssb)
  uint64_t L_ssb;
  /// Total number of SSB transmitted
  uint8_t N_ssb;
  /// SSB index
  uint8_t ssb_index;
  /// OFDM symbol offset divisor for UL
  uint32_t ofdm_offset_divisor;
  uint16_t tdd_slot_config;
  uint8_t tdd_period;
};

// PRS config structures
typedef struct {
    uint16_t PRSResourceSetPeriod[2];   // [slot period, slot offset] of a PRS resource set
    uint16_t PRSResourceOffset;         // Slot offset of each PRS resource defined relative to the slot offset of the PRS resource set (0...511)
    uint8_t  PRSResourceRepetition;     // Repetition factor for all PRS resources in resource set (1 /*default*/, 2, 4, 6, 8, 16, 32)
    uint8_t  PRSResourceTimeGap;        // Slot offset between two consecutive repetition indices of all PRS resources in a PRS resource set (1 /*default*/, 2, 4, 6, 8, 16, 32)
    uint16_t NumRB;                     // Number of PRBs allocated to all PRS resources in a PRS resource set (<= 272 and multiples of 4)
    uint8_t  NumPRSSymbols;             // Number of OFDM symbols in a slot allocated to each PRS resource in a PRS resource set
    uint8_t  SymbolStart;               // Starting OFDM symbol of each PRS resource in a PRS resource set
    uint16_t RBOffset;                  // Starting PRB index of all PRS resources in a PRS resource set
    uint8_t  CombSize;                  // RE density of all PRS resources in a PRS resource set (2, 4, 6, 12)
    uint8_t  REOffset;                  // Starting RE offset in the first OFDM symbol of each PRS resource in a PRS resource set
    uint32_t MutingPattern1[32];        // Muting bit pattern option-1, specified as [] or a binary-valued vector of length 2, 4, 6, 8, 16, or 32
    uint32_t MutingPattern2[32];        // Muting bit pattern option-2, specified as [] or a binary-valued vector of length 2, 4, 6, 8, 16, or 32
    uint8_t  MutingBitRepetition;       // Muting bit repetition factor, specified as 1, 2, 4, or 8
    uint16_t NPRSID;                    // Sequence identity of each PRS resource in a PRS resource set, specified in the range [0, 4095]
} prs_config_t;

typedef struct {
    int8_t  gNB_id;
    int32_t sfn;
    int8_t  slot;
    int8_t  rxAnt_idx;
    float dl_toa;
    int32_t dl_aoa;
    float snr;
    float rsrp;
    float rsrp_dBm;
    int32_t reserved;
} prs_meas_t;

// rel16 prs k_prime table as per ts138.211 sec.7.4.1.7.2
#define K_PRIME_TABLE_ROW_SIZE 4
#define K_PRIME_TABLE_COL_SIZE 12
#define PRS_K_PRIME_TABLE { {0,1,0,1,0,1,0,1,0,1,0,1}, \
                            {0,2,1,3,0,2,1,3,0,2,1,3}, \
                            {0,3,1,4,2,5,0,3,1,4,2,5}, \
                            {0,6,3,9,1,7,4,10,2,8,5,11} };

#define KHz (1000UL)
#define MHz (1000*KHz)

#endif
