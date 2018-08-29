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

/*! \file PHY/NR_TRANSPORT/defs.h
* \brief data structures for PDSCH/DLSCH/PUSCH/ULSCH physical and transport channel descriptors (TX/RX)
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
* \note
* \warning
*/
#ifndef __NR_TRANSPORT_UE__H__
#define __NR_TRANSPORT_UE__H__
#include <limits.h>
#include "PHY/impl_defs_top.h"

//#include "PHY/defs_nr_UE.h"
//#include "../LTE_TRANSPORT/dci.h"
//#include "../LTE_TRANSPORT/mdci.h"
//#include "../LTE_TRANSPORT/uci_common.h"
//#include "../LTE_TRANSPORT/transport_common.h"
#ifndef STANDALONE_COMPILE
#include "UTIL/LISTS/list.h"
#endif

//#include "../LTE_TRANSPORT/transport_common.h"

// structures below implement 36-211 and 36-212

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */

typedef enum {
 NEW_TRANSMISSION_HARQ,
 RETRANSMISSION_HARQ
} harq_result_t;

//#if defined(UPGRADE_RAT_NR)
#if 1
typedef struct {
  /// HARQ process id
  uint8_t harq_id;
  /// HARQ rx status
  harq_result_t rx_status;
  /// ACK bits (after decoding) 0:NACK / 1:ACK / 2:DTX
  uint8_t ack;
  /// send status (for PUCCH)
  uint8_t send_harq_status;
  /// nCCE (for PUCCH)
  uint8_t nCCE;
  /// DAI value detected from DCI1/1a/1b/1d/2/2a/2b/2c. 0xff indicates not touched
  uint8_t vDAI_DL;
  /// DAI value detected from DCI0/4. 0xff indicates not touched
  uint8_t vDAI_UL;
  /// allow to define pucch parameters TS 38.213 9.2.3 UE procedure for reporting HARQ-ACK
  uint8_t  pucch_resource_indicator;
  /// slot on which feedback ack should be send to network
  uint16_t slot_for_feedback_ack;
  /// index of a first CCE for the PDCCH reception
  uint8_t  n_CCE;
  /// number of CCEs in a control resource set of a PDCCH reception conveying DCI format 1_0
  uint8_t  N_CCE;
} NR_UE_HARQ_STATUS_t;
#endif

typedef struct {
  /// Indicator of first transmission
  uint8_t first_tx;
  /// Last Ndi received for this process on DCI (used for C-RNTI only)
  uint8_t DCINdi;
  /// HARQ tx status
  harq_result_t tx_status;
  /// Flag indicating that this ULSCH has a new packet (start of new round)
  //  uint8_t Ndi;
  /// Status Flag indicating for this ULSCH (idle,active,disabled)
  SCH_status_t status;
  /// Subframe scheduling indicator (i.e. Transmission opportunity indicator)
  uint8_t subframe_scheduling_flag;
  /// Subframe cba scheduling indicator (i.e. Transmission opportunity indicator)
  uint8_t subframe_cba_scheduling_flag;
  /// First Allocated RB
  uint16_t first_rb;
  /// Current Number of RBs
  uint16_t nb_rb;
  /// Last TPC command
  uint8_t TPC;
  /// Transport block size
  uint32_t TBS;
  /// The payload + CRC size in bits, "B" from 36-212
  uint32_t B;
  /// Length of ACK information (bits)
  uint8_t O_ACK;
  /// Pointer to the payload
  uint8_t *b;
  /// Pointers to transport block segments
  uint8_t *c[MAX_NUM_ULSCH_SEGMENTS];
  /// RTC values for each segment (for definition see 36-212 V8.6 2009-03, p.15)
  uint32_t RTC[MAX_NUM_ULSCH_SEGMENTS];
  /// Index of current HARQ round for this ULSCH
  uint8_t round;
  /// MCS format of this ULSCH
  uint8_t mcs;
  /// Redundancy-version of the current sub-frame
  uint8_t rvidx;
  /// Turbo-code outputs (36-212 V8.6 2009-03, p.12
  uint8_t d[MAX_NUM_ULSCH_SEGMENTS][(96+3+(3*6144))];
  /// Sub-block interleaver outputs (36-212 V8.6 2009-03, p.16-17)
  uint8_t w[MAX_NUM_ULSCH_SEGMENTS][3*6144];
  /// Number of code segments (for definition see 36-212 V8.6 2009-03, p.9)
  uint32_t C;
  /// Number of "small" code segments (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Cminus;
  /// Number of "large" code segments (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Cplus;
  /// Number of bits in "small" code segments (<6144) (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Kminus;
  /// Number of bits in "large" code segments (<6144) (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Kplus;
  /// Total number of bits across all segments
  uint32_t sumKr;
  /// Number of "Filler" bits (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t F;
  /// Msc_initial, Initial number of subcarriers for ULSCH (36-212, v8.6 2009-03, p.26-27)
  uint16_t Msc_initial;
  /// Nsymb_initial, Initial number of symbols for ULSCH (36-212, v8.6 2009-03, p.26-27)
  uint8_t Nsymb_initial;
  /// n_DMRS  for cyclic shift of DMRS (36.213 Table 9.1.2-2)
  uint8_t n_DMRS;
  /// n_DMRS2 for cyclic shift of DMRS (36.211 Table 5.5.1.1.-1)
  uint8_t n_DMRS2;
  /// Flag to indicate that this is a control only ULSCH (i.e. no MAC SDU)
  uint8_t control_only;
  /// Flag to indicate that this is a calibration ULSCH (i.e. no MAC SDU and filled with TDD calibration information)
  //  int calibration_flag;
  /// Number of soft channel bits
  uint32_t G;
  // decode phich
  uint8_t decode_phich;
} NR_UL_UE_HARQ_t;

typedef struct {
  /// Current Number of Symbols
  uint8_t Nsymb_pusch;
  /// SRS active flag
  uint8_t srs_active; 
//#if defined(UPGRADE_RAT_NR)
#if 1
  // Pointers to HARQ processes for the ULSCH
  NR_UL_UE_HARQ_t *harq_processes[NR_MAX_ULSCH_HARQ_PROCESSES];
  int harq_process_id[NR_MAX_SLOTS_PER_FRAME];
  // UL number of harq processes
  uint8_t number_harq_processes_for_pusch;
#endif 
  /// Pointer to CQI data (+1 for 8 bits crc)
  uint8_t o[1+MAX_CQI_BYTES];
  /// Length of CQI data (bits)
  uint8_t O;
  /// Format of CQI data
  UCI_format_t uci_format;
  /// Rank information
  uint8_t o_RI[2];
  /// Length of rank information (bits)
  uint8_t O_RI;
  /// Pointer to ACK
  uint8_t o_ACK[4];
  /// Minimum number of CQI bits for PUSCH (36-212 r8.6, Sec 5.2.4.1 p. 37)
  uint8_t O_CQI_MIN;
  /// ACK/NAK Bundling flag
  uint8_t bundling;
  /// Concatenated "e"-sequences (for definition see 36-212 V8.6 2009-03, p.17-18)
  uint8_t e[MAX_NUM_CHANNEL_BITS];
  /// Interleaved "h"-sequences (for definition see 36-212 V8.6 2009-03, p.17-18)
  uint8_t h[MAX_NUM_CHANNEL_BITS];
  /// Scrambled "b"-sequences (for definition see 36-211 V8.6 2009-03, p.14)
  uint8_t b_tilde[MAX_NUM_CHANNEL_BITS];
  /// Modulated "d"-sequences (for definition see 36-211 V8.6 2009-03, p.14)
  int32_t d[MAX_NUM_RE];
  /// Transform-coded "z"-sequences (for definition see 36-211 V8.6 2009-03, p.14-15)
  int32_t z[MAX_NUM_RE];
  /// "q" sequences for CQI/PMI (for definition see 36-212 V8.6 2009-03, p.27)
  uint8_t q[MAX_CQI_PAYLOAD];
  /// coded and interleaved CQI bits
  uint8_t o_w[(MAX_CQI_BITS+8)*3];
  /// coded CQI bits
  uint8_t o_d[96+((MAX_CQI_BITS+8)*3)];
  /// coded ACK bits
  uint8_t q_ACK[MAX_ACK_PAYLOAD];
  /// coded RI bits
  uint8_t q_RI[MAX_RI_PAYLOAD];
  /// beta_offset_cqi times 8
  uint16_t beta_offset_cqi_times8;
  /// beta_offset_ri times 8
  uint16_t beta_offset_ri_times8;
  /// beta_offset_harqack times 8
  uint16_t beta_offset_harqack_times8;
  /// power_offset
  uint8_t power_offset;
  // for cooperative communication
  uint8_t cooperation_flag;
  /// RNTI attributed to this ULSCH
  uint16_t rnti;
  /// RNTI type
  uint8_t rnti_type;
  /// f_PUSCH parameter for PUSCH power control
  int16_t f_pusch;
  /// Po_PUSCH - target output power for PUSCH
  int16_t Po_PUSCH;
  /// PHR - current power headroom (based on last PUSCH transmission)
  int16_t PHR;
  /// Po_SRS - target output power for SRS
  int16_t Po_SRS;
  /// num active cba group
  uint8_t num_active_cba_groups;
  /// num dci found for cba
  uint8_t num_cba_dci[10];
  /// allocated CBA RNTI
  uint16_t cba_rnti[4];//NUM_MAX_CBA_GROUP];
  /// UL max-harq-retransmission
  uint8_t Mlimit;
} NR_UE_ULSCH_t;

typedef struct {
  /// Indicator of first transmission
  uint8_t first_tx;
  /// Last Ndi received for this process on DCI (used for C-RNTI only)
  uint8_t DCINdi;
  /// DLSCH status flag indicating
  SCH_status_t status;
  /// Transport block size
  uint32_t TBS;
  /// The payload + CRC size in bits
  uint32_t B;
  /// Pointer to the payload
  uint8_t *b;
  /// Pointers to transport block segments
  uint8_t *c[MAX_NUM_DLSCH_SEGMENTS];
  /// RTC values for each segment (for definition see 36-212 V8.6 2009-03, p.15)
  uint32_t RTC[MAX_NUM_DLSCH_SEGMENTS];
  /// Index of current HARQ round for this DLSCH
  uint8_t round;
  /// MCS format for this DLSCH
  uint8_t mcs;
  /// Qm (modulation order) for this DLSCH
  uint8_t Qm;
  /// Redundancy-version of the current sub-frame
  uint8_t rvidx;
  /// MIMO mode for this DLSCH
  MIMO_mode_t mimo_mode;
  /// soft bits for each received segment ("w"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  int16_t w[MAX_NUM_DLSCH_SEGMENTS][3*(6144+64)];
  /// for abstraction soft bits for each received segment ("w"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  double w_abs[MAX_NUM_DLSCH_SEGMENTS][3*(6144+64)];
  /// soft bits for each received segment ("d"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  int16_t *d[MAX_NUM_DLSCH_SEGMENTS];
  /// Number of code segments (for definition see 36-212 V8.6 2009-03, p.9)
  uint32_t C;
  /// Number of "small" code segments (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Cminus;
  /// Number of "large" code segments (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Cplus;
  /// Number of bits in "small" code segments (<6144) (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Kminus;
  /// Number of bits in "large" code segments (<6144) (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t Kplus;
  /// Number of "Filler" bits (for definition see 36-212 V8.6 2009-03, p.10)
  uint32_t F;
  /// Number of MIMO layers (streams) (for definition see 36-212 V8.6 2009-03, p.17)
  uint8_t Nl;
  /// current delta_pucch
  int8_t delta_PUCCH;
  /// Number of soft channel bits
  uint32_t G;
  /// Current Number of RBs
  uint16_t nb_rb;
  /// Starting RB number
  uint16_t start_rb;
  /// Number of Symbols
  uint16_t nb_symbols;
  /// Starting Symbol number
  uint16_t start_symbol;
  /// Current subband PMI allocation
  uint16_t pmi_alloc;
  /// Current RB allocation (even slots)
  uint32_t rb_alloc_even[4];
  /// Current RB allocation (odd slots)
  uint32_t rb_alloc_odd[4];
  /// distributed/localized flag
  vrb_t vrb_type;
  /// downlink power offset field
  uint8_t dl_power_off;
  /// trials per round statistics
  uint32_t trials[8];
  /// error statistics per round
  uint32_t errors[8];
  /// codeword this transport block is mapped to
  uint8_t codeword;
  /// HARQ-ACKs
  NR_UE_HARQ_STATUS_t harq_ack;
} NR_DL_UE_HARQ_t;

typedef struct {
  /// RNTI
  uint16_t rnti;
  /// RNTI type
  uint8_t rnti_type;
  /// Active flag for DLSCH demodulation
  uint8_t active;
  /// Transmission mode
  uint8_t mode1_flag;
  /// amplitude of PDSCH (compared to RS) in symbols without pilots
  int16_t sqrt_rho_a;
  /// amplitude of PDSCH (compared to RS) in symbols containing pilots
  int16_t sqrt_rho_b;
  /// Current HARQ process id threadRx Odd and threadRx Even
  uint8_t current_harq_pid;
  /// Current subband antenna selection
  uint32_t antenna_alloc;
  /// Current subband RI allocation
  uint32_t ri_alloc;
  /// Current subband CQI1 allocation
  uint32_t cqi_alloc1;
  /// Current subband CQI2 allocation
  uint32_t cqi_alloc2;
  /// saved subband PMI allocation from last PUSCH/PUCCH report
  uint16_t pmi_alloc;
//#if defined(UPGRADE_RAT_NR)
#if 1
  /// Pointers to up to HARQ processes
  NR_DL_UE_HARQ_t *harq_processes[NR_MAX_DLSCH_HARQ_PROCESSES];
  // DL number of harq processes
  uint8_t number_harq_processes_for_pdsch;
  /* higher layer parameter for reception of two transport blocks TS 38.213 9.1.3.1 Type-2 HARQ-ACK codebook dtermination */
  uint8_t Number_MCS_HARQ_DL_DCI;
  /* spatial bundling of PUCCH */
  uint8_t HARQ_ACK_spatial_bundling_PUCCH;
#endif
  /// Maximum number of HARQ processes(for definition see 36-212 V8.6 2009-03, p.17
  uint8_t Mdlharq;
  /// MIMO transmission mode indicator for this sub-frame (for definition see 36-212 V8.6 2009-03, p.17)
  uint8_t Kmimo;
  /// Nsoft parameter related to UE Category
  uint32_t Nsoft;
  /// Maximum number of Turbo iterations
  uint8_t max_turbo_iterations;
  /// number of iterations used in last turbo decoding
  uint8_t last_iteration_cnt;
  /// accumulated tx power adjustment for PUCCH
  int8_t               g_pucch;
} NR_UE_DLSCH_t;

typedef enum {format0_0,
              format0_1,
              format1_0,
              format1_1,
              format2_0,
              format2_1,
              format2_2,
              format2_3
             } NR_DCI_format_t;


typedef enum {nr_pucch_format0=0,
              nr_pucch_format1,
              nr_pucch_format2,
              nr_pucch_format3,
              nr_pucch_format4
             } NR_PUCCH_FMT_t;

typedef struct {
  /// Length of DCI in bits
  uint8_t dci_length;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  boolean_t ra_flag;
  /// rnti
  rnti_t rnti;
  /// rnti type
  //crc_scrambled_t rnti_type;
  /// Format
  NR_DCI_format_t format;
  /// search space
  dci_space_t search_space;
  /// DCI pdu
  uint32_t dci_pdu[4];
//#if defined(UPGRADE_RAT_NR)
#if 1
  /// harq information
  uint8_t harq_pid_pusch;
  /// delay between current slot and slot to transmit
  uint8_t number_slots_rx_to_tx;
#endif
} NR_DCI_ALLOC_t;


/**@}*/
#endif
