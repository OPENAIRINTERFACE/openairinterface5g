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

/*! \file PHY/LTE_TRANSPORT/defs.h
* \brief data structures for PDSCH/DLSCH/PUSCH/ULSCH physical and transport channel descriptors (TX/RX)
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
* \note
* \warning
*/
#ifndef __LTE_TRANSPORT_DEFS__H__
#define __LTE_TRANSPORT_DEFS__H__
#include "PHY/defs.h"
#include "PHY/impl_defs_lte.h"
#include "dci.h"
#include "mdci.h"
#include "uci.h"
#ifndef STANDALONE_COMPILE
#include "UTIL/LISTS/list.h"
#endif

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

#define MAX_NUM_DLSCH_SEGMENTS 16
#define MAX_NUM_ULSCH_SEGMENTS MAX_NUM_DLSCH_SEGMENTS
#define MAX_DLSCH_PAYLOAD_BYTES (MAX_NUM_DLSCH_SEGMENTS*768)
#define MAX_ULSCH_PAYLOAD_BYTES (MAX_NUM_ULSCH_SEGMENTS*768)

#define MAX_NUM_CHANNEL_BITS (14*1200*6)  // 14 symbols, 1200 REs, 12 bits/RE
#define MAX_NUM_RE (14*1200)

#if !defined(SI_RNTI)
#define SI_RNTI  (rnti_t)0xffff
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
	       SEARCH_EXIST_OR_FREE} find_type_t;

typedef enum {
  SCH_IDLE=0,
  ACTIVE,
  CBA_ACTIVE,
  DISABLED
} SCH_status_t;


typedef struct {
  /// Status Flag indicating for this DLSCH (idle,active,disabled)
  SCH_status_t status;
  /// Transport block size
  uint32_t TBS;
  /// pointer to pdu from MAC interface (this is "a" in 36.212)
  uint8_t *pdu;
  /// The payload + CRC size in bits, "B" from 36-212
  uint32_t B;
  /// Pointer to the payload
  uint8_t *b;
  /// Pointers to transport block segments
  uint8_t *c[MAX_NUM_DLSCH_SEGMENTS];
  /// RTC values for each segment (for definition see 36-212 V8.6 2009-03, p.15)
  uint32_t RTC[MAX_NUM_DLSCH_SEGMENTS];
  /// Frame where current HARQ round was sent
  uint32_t frame;
  /// Subframe where current HARQ round was sent
  uint32_t subframe;
  /// Index of current HARQ round for this DLSCH
  uint8_t round;
  /// Modulation order
  uint8_t Qm;
  /// MCS
  uint8_t mcs;
  /// Redundancy-version of the current sub-frame
  uint8_t rvidx;
  /// MIMO mode for this DLSCH
  MIMO_mode_t mimo_mode;
  /// Current RB allocation
  uint32_t rb_alloc[4];
  /// distributed/localized flag
  vrb_t vrb_type;
  /// Current subband PMI allocation
  uint16_t pmi_alloc;
  /// Current subband RI allocation
  uint32_t ri_alloc;
  /// Current subband CQI1 allocation
  uint32_t cqi_alloc1;
  /// Current subband CQI2 allocation
  uint32_t cqi_alloc2;
  /// Current Number of RBs
  uint16_t nb_rb;
  /// Current NDI
  uint8_t ndi;
  /// downlink power offset field
  uint8_t dl_power_off;
  /// start symbold of pdsch
  uint8_t pdsch_start;
  /// Concatenated "e"-sequences (for definition see 36-212 V8.6 2009-03, p.17-18)
  uint8_t e[MAX_NUM_CHANNEL_BITS] __attribute__((aligned(32)));
  /// Turbo-code outputs (36-212 V8.6 2009-03, p.12
  uint8_t *d[MAX_NUM_DLSCH_SEGMENTS];//[(96+3+(3*6144))];
  /// Sub-block interleaver outputs (36-212 V8.6 2009-03, p.16-17)
  uint8_t w[MAX_NUM_DLSCH_SEGMENTS][3*6144];
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
  /// Number of MIMO layers (streams) (for definition see 36-212 V8.6 2009-03, p.17, TM3-4)
  uint8_t Nl;
  /// Number of layers for this PDSCH transmission (TM8-10)
  uint8_t Nlayers;
  /// First layer for this PSCH transmission
  uint8_t first_layer;
  /// codeword this transport block is mapped to
  uint8_t codeword;
} LTE_DL_eNB_HARQ_t;

typedef struct {
  /// Indicator of first transmission
  uint8_t first_tx;
  /// Last Ndi received for this process on DCI (used for C-RNTI only)
  uint8_t DCINdi;
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
} LTE_UL_UE_HARQ_t; 

#ifdef Rel14
typedef enum {
  CEmodeA = 0,
  CEmodeB = 1
} CEmode_t;
#endif

typedef struct {
  /// TX buffers for UE-spec transmission (antenna ports 5 or 7..14, prior to precoding)
  int32_t *txdataF[8];
  /// beamforming weights for UE-spec transmission (antenna ports 5 or 7..14), for each codeword, maximum 4 layers?
  int32_t **ue_spec_bf_weights[4]; 
  /// dl channel estimates (estimated from ul channel estimates)
  int32_t **calib_dl_ch_estimates;
  /// Allocated RNTI (0 means DLSCH_t is not currently used)
  uint16_t rnti;
  /// Active flag for baseband transmitter processing
  uint8_t active;
  /// HARQ process mask, indicates which processes are currently active
  uint16_t harq_mask;
  /// Indicator of TX activation per subframe.  Used during PUCCH detection for ACK/NAK.
  uint8_t subframe_tx[10];
  /// First CCE of last PDSCH scheduling per subframe.  Again used during PUCCH detection for ACK/NAK.
  uint8_t nCCE[10];
  /// Process ID's per subframe.  Used to associate received ACKs on PUSCH/PUCCH to DLSCH harq process ids
  uint8_t harq_ids[10];
  /// Window size (in outgoing transport blocks) for fine-grain rate adaptation
  uint8_t ra_window_size;
  /// First-round error threshold for fine-grain rate adaptation
  uint8_t error_threshold;
  /// Pointers to 8 HARQ processes for the DLSCH
  LTE_DL_eNB_HARQ_t *harq_processes[8];
  /// Number of soft channel bits
  uint32_t G;
  /// Codebook index for this dlsch (0,1,2,3)
  uint8_t codebook_index;
  /// Maximum number of HARQ processes (for definition see 36-212 V8.6 2009-03, p.17)
  uint8_t Mdlharq;
  /// Maximum number of HARQ rounds
  uint8_t Mlimit;
  /// MIMO transmission mode indicator for this sub-frame (for definition see 36-212 V8.6 2009-03, p.17)
  uint8_t Kmimo;
  /// Nsoft parameter related to UE Category
  uint32_t Nsoft;
  /// amplitude of PDSCH (compared to RS) in symbols without pilots
  int16_t sqrt_rho_a;
  /// amplitude of PDSCH (compared to RS) in symbols containing pilots
  int16_t sqrt_rho_b;
#ifdef Rel14
  /// indicator that this DLSCH corresponds to SIB1-BR, needed for c_init for scrambling
  uint8_t sib1_br_flag;
  /// initial absolute subframe (see 36.211 Section 6.3.1), needed for c_init for scrambling
  uint16_t i0;
  CEmode_t CEmode;
#endif
} LTE_eNB_DLSCH_t;

#define PUSCH_x 2
#define PUSCH_y 3

typedef struct {
  /// Current Number of Symbols
  uint8_t Nsymb_pusch;
  /// SRS active flag
  uint8_t srs_active;
  /// Pointers to 8 HARQ processes for the ULSCH
  LTE_UL_UE_HARQ_t *harq_processes[8];
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
} LTE_UE_ULSCH_t;

typedef struct {
  /// Flag indicating that this ULSCH has been allocated by a DCI (otherwise it is a retransmission based on PHICH NAK)
  uint8_t dci_alloc;
  /// Flag indicating that this ULSCH has been allocated by a RAR (otherwise it is a retransmission based on PHICH NAK or DCI)
  uint8_t rar_alloc;
  /// Status Flag indicating for this ULSCH (idle,active,disabled)
  SCH_status_t status;
  /// Flag to indicate that eNB should decode UE Msg3
  uint8_t Msg3_flag;
  /// Subframe for reception
  uint8_t subframe;
  /// Frame for reception
  uint32_t frame;
  /// Flag to indicate that the UL configuration has been handled. Used to remove a stale ULSCH when frame wraps around
  uint8_t handled;
  /// PHICH active flag
  uint8_t phich_active;
  /// PHICH ACK
  uint8_t phich_ACK;
  /// First Allocated RB
  uint16_t first_rb;
  /// First Allocated RB - previous scheduling
  /// This is needed for PHICH generation which
  /// is done after a new scheduling
  uint16_t previous_first_rb;
  /// Current Number of RBs
  uint16_t nb_rb; 
  /// Current Modulation order
  uint8_t Qm;
  /// Transport block size
  uint32_t TBS;
  /// The payload + CRC size in bits
  uint32_t B;
  /// Number of soft channel bits
  uint32_t G;
  /// CQI CRC status
  uint8_t cqi_crc_status;
  /// Pointer to CQI data
  uint8_t o[MAX_CQI_BYTES];
  /// Format of CQI data
  UCI_format_t uci_format;
  /// Length of CQI data under RI=1 assumption(bits)
  uint8_t Or1;
  /// Length of CQI data under RI=2 assumption(bits)
  uint8_t Or2;
  /// Rank information
  uint8_t o_RI[2];
  /// Length of rank information (bits)
  uint8_t O_RI;
  /// Pointer to ACK
  uint8_t o_ACK[4];
  /// Length of ACK information (bits)
  uint8_t O_ACK;
  /// The value of DAI in DCI format 0
  uint8_t V_UL_DAI;
  /// "q" sequences for CQI/PMI (for definition see 36-212 V8.6 2009-03, p.27)
  int8_t q[MAX_CQI_PAYLOAD];
  /// number of coded CQI bits after interleaving
  uint8_t o_RCC;
  /// coded and interleaved CQI bits
  int8_t o_w[(MAX_CQI_BITS+8)*3];
  /// coded CQI bits
  int8_t o_d[96+((MAX_CQI_BITS+8)*3)];
  /// coded ACK bits
  int16_t q_ACK[MAX_ACK_PAYLOAD];
  /// coded RI bits
  int16_t q_RI[MAX_RI_PAYLOAD];
  /// Concatenated "e"-sequences (for definition see 36-212 V8.6 2009-03, p.17-18)
  int16_t e[MAX_NUM_CHANNEL_BITS] __attribute__((aligned(32)));
  /// Temporary h sequence to flag PUSCH_x/PUSCH_y symbols which are not scrambled
  uint8_t h[MAX_NUM_CHANNEL_BITS];
  /// Pointer to the payload
  uint8_t *b;
  /// Pointers to transport block segments
  uint8_t *c[MAX_NUM_ULSCH_SEGMENTS];
  /// RTC values for each segment (for definition see 36-212 V8.6 2009-03, p.15)
  uint32_t RTC[MAX_NUM_ULSCH_SEGMENTS];
  /// Current Number of Symbols
  uint8_t Nsymb_pusch;
  /// SRS active flag
  uint8_t srs_active;
  /// NDI
  uint8_t ndi;
  /// Index of current HARQ round for this ULSCH
  uint8_t round;
  /// Redundancy-version of the current sub-frame
  uint8_t rvidx;
  /// soft bits for each received segment ("w"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  int16_t w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  /// soft bits for each received segment ("d"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  int16_t *d[MAX_NUM_ULSCH_SEGMENTS];
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
  /// Msc_initial, Initial number of subcarriers for ULSCH (36-212, v8.6 2009-03, p.26-27)
  uint16_t Msc_initial;
  /// Nsymb_initial, Initial number of symbols for ULSCH (36-212, v8.6 2009-03, p.26-27)
  uint8_t Nsymb_initial;
  /// n_DMRS  for cyclic shift of DMRS (36.213 Table 9.1.2-2)
  uint8_t n_DMRS;
  /// n_DMRS  for cyclic shift of DMRS (36.213 Table 9.1.2-2) - previous scheduling
  /// This is needed for PHICH generation which
  /// is done after a new scheduling
  uint8_t previous_n_DMRS;
  /// n_DMRS 2 for cyclic shift of DMRS (36.211 Table 5.5.1.1.-1)
  uint8_t n_DMRS2;
  /// Flag to indicate that this ULSCH is for calibration information sent from UE (i.e. no MAC SDU to pass up)
  //  int calibration_flag;
  /// delta_TF for power control
  int32_t delta_TF;
} LTE_UL_eNB_HARQ_t;


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

#ifdef Rel14
typedef enum {
  NOCE,
  CEMODEA,
  CEMODEB
} UE_type_t;
#endif

typedef struct {
  uint8_t     active;
  /// Absolute frame for this UCI
  uint16_t    frame;
  /// Absolute subframe for this UCI
  uint8_t     subframe;
  /// corresponding UE RNTI
  uint16_t    rnti;
  /// Type (SR,HARQ,CQI,HARQ_SR,HARQ_CQI,SR_CQI,HARQ_SR_CQI)
  UCI_type_t  type;
  /// SRS active flag
  uint8_t     srs_active;
  /// PUCCH format to use
  PUCCH_FMT_t pucch_fmt;
  /// number of PUCCH antenna ports 
  uint8_t     num_antenna_ports;
  /// number of PUCCH resources
  uint8_t     num_pucch_resources;
  /// two antenna n1_pucch 1_0
  uint16_t    n_pucch_1[4][2];
  /// two antenna n1_pucch 1_0 for SR
  uint16_t    n_pucch_1_0_sr[2];
   /// two antenna n2_pucch
  uint16_t    n_pucch_2[2];
  /// two antenna n3_pucch
  uint16_t    n_pucch_3[2];
  /// TDD Bundling/multiplexing flag
  uint8_t     tdd_bundling;
  /// Received Energy
  uint32_t stat;
#ifdef Rel14
  /// non BL/CE, CEmodeA, CEmodeB
  UE_type_t ue_type;
  /// Indicates the symbols that are left empty due to eMTC retuning.
  uint8_t empty_symbols;
  /// number of repetitions for BL/CE
  uint16_t total_repetitions;
  /// The size of the DL CQI/PMI in bits.
  uint16_t dl_cqi_pmi_size2;
  /// The starting PRB for the PUCCH
  uint8_t starting_prb;
  /// The number of PRB in PUCCH
  uint8_t n_PRB;
  /// Selected CDM option
  uint8_t cdm_Index;
  // Indicates if the resource blocks allocated for this grant overlap with the SRS configuration.
  uint8_t Nsrs;
#endif
} LTE_eNB_UCI;

typedef struct {
  /// HARQ process mask, indicates which processes are currently active
  uint16_t harq_mask;
  /// Pointers to 8 HARQ processes for the ULSCH
  LTE_UL_eNB_HARQ_t *harq_processes[8];
  /// Maximum number of HARQ rounds
  uint8_t Mlimit;
  /// Maximum number of iterations used in eNB turbo decoder
  uint8_t max_turbo_iterations;
  /// ACK/NAK Bundling flag
  uint8_t bundling;
  /// beta_offset_cqi times 8
  uint16_t beta_offset_cqi_times8;
  /// beta_offset_ri times 8
  uint16_t beta_offset_ri_times8;
  /// beta_offset_harqack times 8
  uint16_t beta_offset_harqack_times8;
  /// Flag to indicate that eNB awaits UE Msg3
  uint8_t Msg3_active;
  /// RNTI attributed to this ULSCH
  uint16_t rnti;
  /// cyclic shift for DM RS
  uint8_t cyclicShift;
  /// cooperation flag
  uint8_t cooperation_flag;
  /// num active cba group
  uint8_t num_active_cba_groups;
  /// allocated CBA RNTI for this ulsch
  uint16_t cba_rnti[4];//NUM_MAX_CBA_GROUP];
#ifdef LOCALIZATION
  /// epoch timestamp in millisecond
  int32_t reference_timestamp_ms;
  /// aggregate physical states every n millisecond
  int32_t aggregation_period_ms;
  /// a set of lists used for localization
  struct list loc_rss_list[10], loc_rssi_list[10], loc_subcarrier_rss_list[10], loc_timing_advance_list[10], loc_timing_update_list[10];
  struct list tot_loc_rss_list, tot_loc_rssi_list, tot_loc_subcarrier_rss_list, tot_loc_timing_advance_list, tot_loc_timing_update_list;
#endif
} LTE_eNB_ULSCH_t;

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
} LTE_DL_UE_HARQ_t;

typedef struct {
  /// time-based localization, relying on TA and TOA
  double time_based;
  /// power-based localization, relying on RSS and RSSI
  double power_based;
} eNB_UE_estimated_distances;

typedef struct {
  /// UL RSSI per receive antenna
  int32_t UL_rssi[NB_ANTENNAS_RX];
  /// PUCCH1a/b power (digital linear)
  uint32_t Po_PUCCH;
  /// PUCCH1a/b power (dBm)
  int32_t Po_PUCCH_dBm;
  /// PUCCH1 power (digital linear), conditioned on below threshold
  uint32_t Po_PUCCH1_below;
  /// PUCCH1 power (digital linear), conditioned on above threshold
  uint32_t Po_PUCCH1_above;
  /// Indicator that Po_PUCCH has been updated by PHY
  int32_t Po_PUCCH_update;
  /// DL Wideband CQI index (2 TBs)
  uint8_t DL_cqi[2];
  /// DL Subband CQI index (from HLC feedback)
  uint8_t DL_subband_cqi[2][13];
  /// DL PMI Single Stream
  uint16_t DL_pmi_single;
  /// DL PMI Dual Stream
  uint16_t DL_pmi_dual;
  /// Current RI
  uint8_t rank;
  /// CRNTI of UE
  uint16_t crnti; ///user id (rnti) of connected UEs
  /// Initial timing offset estimate from PRACH for RAR
  int32_t UE_timing_offset;
  /// Timing advance estimate from PUSCH for MAC timing advance signalling
  int32_t timing_advance_update;
  /// Current mode of UE (NOT SYCHED, RAR, PUSCH)
  UE_MODE_t mode;
  /// Current sector where UE is attached
  uint8_t sector;

  /// dlsch l2 errors
  uint32_t dlsch_l2_errors[8];
  /// dlsch trials per harq and round
  uint32_t dlsch_trials[8][8];
  /// dlsch ACK/NACK per hard_pid and round
  uint32_t dlsch_ACK[8][8];
  uint32_t dlsch_NAK[8][8];

  /// ulsch l2 errors per harq_pid
  uint32_t ulsch_errors[8];
  /// ulsch l2 consecutive errors per harq_pid
  uint32_t ulsch_consecutive_errors; //[8];
  /// ulsch trials/errors/fer per harq and round
  uint32_t ulsch_decoding_attempts[8][8];
  uint32_t ulsch_round_errors[8][8];
  uint32_t ulsch_decoding_attempts_last[8][8];
  uint32_t ulsch_round_errors_last[8][8];
  uint32_t ulsch_round_fer[8][8];
  uint32_t sr_received;
  uint32_t sr_total;

  /// dlsch sliding count and total errors in round 0 are used to compute the dlsch_mcs_offset
  uint32_t dlsch_sliding_cnt;
  uint32_t dlsch_NAK_round0;
  int8_t dlsch_mcs_offset;

  /// Target mcs1 after rate-adaptation (used by MAC layer scheduler)
  uint8_t dlsch_mcs1;
  /// Target mcs2 after rate-adaptation (used by MAC layer scheduler)
  uint8_t dlsch_mcs2;
  /// Total bits received from MAC on PDSCH
  int total_TBS_MAC;
  /// Total bits acknowledged on PDSCH
  int total_TBS;
  /// Total bits acknowledged on PDSCH (last interval)
  int total_TBS_last;
  /// Bitrate on the PDSCH [bps]
  unsigned int dlsch_bitrate;
  //  unsigned int total_transmitted_bits;
#ifdef LOCALIZATION
  eNB_UE_estimated_distances distance;
  int32_t *subcarrier_rssi;
#endif
} LTE_eNB_UE_stats;

typedef struct {
  /// HARQ process id
  uint8_t harq_id;
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
} harq_status_t;

typedef struct {
  /// RNTI
  uint16_t rnti;
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
  /// HARQ-ACKs
  harq_status_t harq_ack[10];
  /// Pointers to up to 8 HARQ processes
  LTE_DL_UE_HARQ_t *harq_processes[8];
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
} LTE_UE_DLSCH_t;



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


/**@}*/
#endif
