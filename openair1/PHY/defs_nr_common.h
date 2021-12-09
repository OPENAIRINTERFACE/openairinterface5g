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

#define NR_MAX_NB_RB 275

#define NR_NB_SC_PER_RB 12
#define NR_NB_REG_PER_CCE 6

#define NR_SYMBOLS_PER_SLOT 14

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

#define NR_PBCH_DMRS_LENGTH 144 // in mod symbols
#define NR_PBCH_DMRS_LENGTH_DWORD 10 // ceil(2(QPSK)*NR_PBCH_DMRS_LENGTH/32)

/*These max values are for the gold sequences which are generated at init for the
 * full carrier bandwidth*/
#define NR_MAX_PDCCH_DMRS_INIT_LENGTH ((NR_MAX_NB_RB<<1)*3) // 3 symbols *2(QPSK)
#define NR_MAX_PDCCH_DMRS_INIT_LENGTH_DWORD 52 // ceil(NR_MAX_PDCCH_DMRS_LENGTH/32)
/*used for the resource mapping*/
#define NR_MAX_PDCCH_DMRS_LENGTH 576 // 16(L)*2(QPSK)*3(3 DMRS symbs per REG)*6(REG per CCE)

#define NR_MAX_PDSCH_DMRS_LENGTH 3300      //275*6(k)*2(QPSK real+imag) 
#define NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD 104  // ceil(NR_MAX_PDSCH_DMRS_LENGTH/32)

#define NR_MAX_CSI_RS_LENGTH 4400 //275*8(max allocation per RB)*2(QPSK)
#define NR_MAX_CSI_RS_INIT_LENGTH_DWORD 138  // ceil(NR_MAX_CSI_RS_LENGTH/32)

#define NR_MAX_PUSCH_DMRS_LENGTH NR_MAX_PDSCH_DMRS_LENGTH 
#define NR_MAX_PUSCH_DMRS_INIT_LENGTH_DWORD NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD

#define NR_MAX_DCI_PAYLOAD_SIZE 64
#define NR_MAX_DCI_SIZE 1728 //16(L)*2(QPSK)*9(12 RE per REG - 3(DMRS))*6(REG per CCE)
#define NR_MAX_DCI_SIZE_DWORD 54 // ceil(NR_MAX_DCI_SIZE/32)

#define NR_MAX_NUM_BWP 4

#define NR_MAX_PDCCH_AGG_LEVEL 16 // 3GPP TS 38.211 V15.8 Section 7.3.2 Table 7.3.2.1-1: Supported PDCCH aggregation levels
#define NR_MAX_CSET_DURATION 3

#define NR_MAX_NB_RBG 18
#define NR_MAX_NB_LAYERS 2 // 8 // SU-MIMO (3GPP TS 38.211 V15.4.0 section 7.3.1.3)
#define NR_MAX_NB_CODEWORDS 2
#define NR_MAX_NB_HARQ_PROCESSES 16
#define NR_MAX_PDSCH_ENCODED_LENGTH (NR_MAX_NB_RB*NR_SYMBOLS_PER_SLOT*NR_NB_SC_PER_RB*8*NR_MAX_NB_LAYERS) // 8 is the maximum modulation order (it was 950984 before !!)
#define NR_MAX_PUSCH_ENCODED_LENGTH NR_MAX_PDSCH_ENCODED_LENGTH
#define NR_MAX_PDSCH_TBS 3824
#define NR_MAX_SIB_LENGTH 2976 // 3GPP TS 38.331 section 5.2.1 - The physical layer imposes a limit to the maximum size a SIB can take. The maximum SIB1 or SI message size is 2976 bits.

#define MAX_NUM_NR_DLSCH_SEGMENTS (NR_MAX_NB_LAYERS*34)
#define MAX_NR_DLSCH_PAYLOAD_BYTES (MAX_NUM_NR_DLSCH_SEGMENTS*1056)

#define MAX_NUM_NR_ULSCH_SEGMENTS 34
#define MAX_NR_ULSCH_PAYLOAD_BYTES (MAX_NUM_NR_ULSCH_SEGMENTS*1056)

#define MAX_NUM_NR_CHANNEL_BITS (14*273*12*8)  // 14 symbols, 273 RB
#define MAX_NUM_NR_RE (14*273*12)
#define NR_RX_NB_TH 1
#define NR_NB_TH_SLOT 2

extern const uint8_t nr_rv_round_map[4]; 
extern const uint8_t nr_rv_round_map_ue[4]; 

typedef enum {
  NR_MU_0=0,
  NR_MU_1,
  NR_MU_2,
  NR_MU_3,
  NR_MU_4,
} nr_numerology_index_e;

typedef enum {
  kHz15=0,
  kHz30,
  kHz60,
  kHz120,
  kHz240
} nr_scs_e;

typedef enum{
  nr_ssb_type_A = 0,
  nr_ssb_type_B,
  nr_ssb_type_C,
  nr_ssb_type_D,
  nr_ssb_type_E
} nr_ssb_type_e;

typedef enum {
  nr_FR1 = 0,
  nr_FR2
} nr_frequency_range_e;

typedef enum {
  MOD_BPSK=0,
  MOD_QPSK,
  MOD_QAM16,
  MOD_QAM64,
  MOD_QAM256
}nr_mod_t;

typedef enum {
  RA_2STEP = 0,
  RA_4STEP
} nr_ra_type_e;

typedef struct {
  /// Size of first RBG
  uint8_t start_size;
  /// Nominal size
  uint8_t P;
  /// Size of last RBG
  uint8_t end_size;
  /// Number of RBG
  uint8_t N_RBG;
}nr_rbg_parms_t;

typedef struct {
  /// Size of first PRG
  uint8_t start_size;
  /// Nominal size
  uint8_t P_prime;
  /// Size of last PRG
  uint8_t end_size;
  /// Number of PRG
  uint8_t N_PRG;
} nr_prg_parms_t;

typedef struct NR_BWP_PARMS {
  /// BWP ID
  uint8_t bwp_id;
  /// Subcarrier spacing
  nr_scs_e scs;
  /// Freq domain location -- 1st CRB index
  uint8_t location;
  /// Bandwidth in PRB
  uint16_t N_RB;
  /// Cyclic prefix
  uint8_t cyclic_prefix;
  /// RBG params
  nr_rbg_parms_t rbg_parms;
  /// PRG params
  nr_prg_parms_t prg_parms;
} NR_BWP_PARMS;

typedef struct {
  uint8_t reg_idx;
  uint16_t start_sc_idx;
  uint8_t symb_idx;
} nr_reg_t;

typedef struct {
  uint8_t cce_idx;
  nr_reg_t reg_list[NR_NB_REG_PER_CCE];
} nr_cce_t;

typedef struct {
  /// PRACH format retrieved from prach_ConfigIndex
  uint16_t prach_format;
  /// Preamble index for PRACH (0-63)
  uint8_t ra_PreambleIndex;
  /// Preamble Tx Counter
  uint8_t RA_PREAMBLE_TRANSMISSION_COUNTER;
  /// Preamble Power Ramping Counter
  uint8_t RA_PREAMBLE_POWER_RAMPING_COUNTER;
  /// 2-step RA power offset
  int POWER_OFFSET_2STEP_RA;
  /// Target received power at gNB. Baseline is range -202..-60 dBm. Depends on delta preamble, power ramping counter and step.
  int ra_PREAMBLE_RECEIVED_TARGET_POWER;
  /// PRACH index for TDD (0 ... 6) depending on TDD configuration and prachConfigIndex
  uint8_t ra_TDD_map_index;
  /// RA Preamble Power Ramping Step in dB
  uint32_t RA_PREAMBLE_POWER_RAMPING_STEP;
  ///
  uint8_t RA_PREAMBLE_BACKOFF;
  ///
  uint8_t RA_SCALING_FACTOR_BI;
  /// Indicating whether it is 2-step or 4-step RA
  nr_ra_type_e RA_TYPE;
  /// UE configured maximum output power
  int RA_PCMAX;
  /// Corresponding RA-RNTI for UL-grant
  uint16_t ra_RNTI;
  /// Frame of last completed synch
  uint16_t sync_frame;
  /// Flag to indicate that prach is ready to start: it is enabled with an initial delay after the sync
  uint8_t init_msg1;
} NR_PRACH_RESOURCES_t;

typedef struct NR_DL_FRAME_PARMS NR_DL_FRAME_PARMS;

typedef uint32_t (*get_samples_per_slot_t)(int slot, NR_DL_FRAME_PARMS* fp);
typedef uint32_t (*get_slot_from_timestamp_t)(openair0_timestamp timestamp_rx, NR_DL_FRAME_PARMS* fp);

typedef uint32_t (*get_samples_slot_timestamp_t)(int slot, NR_DL_FRAME_PARMS* fp, uint8_t sl_ahead);

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
  /// TX attenuation
  uint32_t att_tx;
  /// RX attenuation
  uint32_t att_rx;
  ///  total Number of Resource Block Groups: this is ceil(N_PRB/P)
  /// Frame type (0 FDD, 1 TDD)
  lte_frame_type_t frame_type;
  uint8_t tdd_config;
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
  /// UPA Panel Antenna configuration: two_one, two_two, four_one, three_two, six_one, etc
  uint8_t antennas_tx_n1_n2;
  /// UPA Panel Antenna Type: non, Single panel, or multiple
  uint8_t antennas_tx_panel_type;
  /// Number of Receive antennas in node
  uint8_t nb_antennas_rx;
  /// Number of common transmit antenna ports in eNodeB (1 or 2)
  uint8_t nb_antenna_ports_gNB;
  /// Cyclic Prefix for DL (0=Normal CP, 1=Extended CP)
  lte_prefix_type_t Ncp;
  /// sequence which is computed based on carrier frequency and numerology to rotate/derotate each OFDM symbol according to Section 5.3 in 38.211
  /// First dimension is for the direction of the link (0 DL, 1 UL)
  int16_t symbol_rotation[2][224*2];
  /// sequence used to compensate the phase rotation due to timeshifted OFDM symbols
  /// First dimenstion is for different CP lengths
  int16_t timeshift_symbol_rotation[4096*2] __attribute__ ((aligned (16)));
  /// shift of pilot position in one RB
  uint8_t nushift;
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
  /// PBCH polar encoder params
  t_nrPolar_params pbch_polar_params;
  /// OFDM symbol offset divisor for UL
  uint32_t ofdm_offset_divisor;
};



#define KHz (1000UL)
#define MHz (1000*KHz)

#endif
