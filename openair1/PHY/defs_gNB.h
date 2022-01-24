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

/*! \file PHY/defs_gNB.h
 \brief Top-level defines and structure definitions for gNB
 \author Guy De Souza
 \date 2018
 \version 0.1
 \company Eurecom
 \email: desouza@eurecom.fr
 \note
 \warning
*/

#ifndef __PHY_DEFS_GNB__H__
#define __PHY_DEFS_GNB__H__

#include "defs_eNB.h"
#include "defs_nr_common.h"
#include "CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/impl_defs_top.h"
#include "PHY/defs_common.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"

#include "nfapi_nr_interface_scf.h"

#define MAX_NUM_RU_PER_gNB MAX_NUM_RU_PER_eNB
#define MAX_PUCCH0_NID 8

typedef struct {
  int nb_id;
  int Nid[MAX_PUCCH0_NID];
  int lut[MAX_PUCCH0_NID][160][14];
} NR_gNB_PUCCH0_LUT_t;


typedef struct {
  uint32_t pbch_a;
  uint32_t pbch_a_interleaved;
  uint32_t pbch_a_prime;
  uint32_t pbch_e[NR_POLAR_PBCH_E_DWORD];
} NR_gNB_PBCH;

typedef struct {
  uint8_t ssb_start_symbol;
  uint8_t n_hf;
  uint8_t Lmax;
  uint8_t ssb_index;
  int32_t sfn;
} NR_PBCH_parms_t;


typedef enum {
  NR_SCH_IDLE,
  NR_ACTIVE,
  NR_CBA_ACTIVE,
  NR_DISABLED
} NR_SCH_status_t;


typedef struct {
  /// Nfapi DLSCH PDU
  nfapi_nr_dl_tti_pdsch_pdu pdsch_pdu;
  /// pointer to pdu from MAC interface (this is "a" in 36.212)
  uint8_t *pdu;
  /// The payload + CRC size in bits, "B" from 36-212
  uint32_t B;
  /// Pointer to the payload
  uint8_t *b;
  /// Pointers to transport block segments
  uint8_t *c[MAX_NUM_NR_DLSCH_SEGMENTS];
  /// Frame where current HARQ round was sent
  uint32_t frame;
  /// Subframe where current HARQ round was sent
  uint32_t subframe;
  /// MIMO mode for this DLSCH
  MIMO_mode_t mimo_mode;
  /// LDPC lifting size
  uint32_t Z;
} NR_DL_gNB_HARQ_t;

typedef struct {
  int frame;
  int slot;
  nfapi_nr_dl_tti_pdcch_pdu pdcch_pdu;
} NR_gNB_PDCCH_t;

typedef struct {
  uint8_t active;
  nfapi_nr_dl_tti_csi_rs_pdu csirs_pdu;
} NR_gNB_CSIRS_t;

typedef struct {
  int frame;
  int slot;
  nfapi_nr_ul_dci_request_pdus_t pdcch_pdu;
} NR_gNB_UL_PDCCH_t;

typedef struct {
  int frame;
  int dump_frame;
  uint16_t rnti;
  int round_trials[8];
  int total_bytes_tx;
  int total_bytes_rx;
  int current_Qm;
  int current_RI;
  int power[NB_ANTENNAS_RX];
  int noise_power[NB_ANTENNAS_RX];
  int DTX;
  int sync_pos;
} NR_gNB_SCH_STATS_t;

typedef struct {
  int frame;
  uint16_t rnti;
  int pucch0_sr_trials;
  int pucch0_sr_thres;
  int current_pucch0_sr_stat0;
  int current_pucch0_sr_stat1;
  int pucch0_positive_SR;
  int pucch01_trials;
  int pucch0_n00;
  int pucch0_n01;
  int pucch0_thres;
  int current_pucch0_stat0;
  int current_pucch0_stat1;
  int pucch01_DTX;
  int pucch02_trials;
  int pucch02_DTX;
  int pucch2_trials;
  int pucch2_DTX;
} NR_gNB_UCI_STATS_t;

typedef struct {
  /// Pointers to variables related to DLSCH harq process
  NR_DL_gNB_HARQ_t harq_process;
  /// TX buffers for UE-spec transmission (antenna layers 1,...,4 after to precoding)
  int32_t *txdataF[NR_MAX_NB_LAYERS];
  /// TX buffers for UE-spec transmission (antenna ports 1000 or 1001,...,1007, before precoding)
  int32_t *txdataF_precoding[NR_MAX_NB_LAYERS];
  /// Modulated symbols buffer
  int32_t *mod_symbs[NR_MAX_NB_CODEWORDS];
  /// beamforming weights for UE-spec transmission (antenna ports 5 or 7..14), for each codeword, maximum 4 layers?
  int32_t **ue_spec_bf_weights[NR_MAX_NB_LAYERS];
  /// dl channel estimates (estimated from ul channel estimates)
  int32_t **calib_dl_ch_estimates;
  /// Allocated RNTI (0 means DLSCH_t is not currently used)
  uint16_t rnti;
  /// Active flag for baseband transmitter processing
  uint8_t active;
  /// HARQ process mask, indicates which processes are currently active
  uint16_t harq_mask;
  /// Indicator of TX activation per subframe.  Used during PUCCH detection for ACK/NAK.
  uint8_t slot_tx[80];
  /// First CCE of last PDSCH scheduling per subframe.  Again used during PUCCH detection for ACK/NAK.
  uint8_t nCCE[10];
  /// Process ID's per subframe.  Used to associate received ACKs on PUSCH/PUCCH to DLSCH harq process ids
  uint8_t harq_ids[2][80];
  /// Window size (in outgoing transport blocks) for fine-grain rate adaptation
  uint8_t ra_window_size;
  /// First-round error threshold for fine-grain rate adaptation
  uint8_t error_threshold;
  /// Number of soft channel bits
  uint32_t G;
  /// Codebook index for this dlsch (0,1,2,3)
  uint8_t codebook_index;
  /// Maximum number of HARQ processes
  uint8_t Mdlharq;
  /// Maximum number of HARQ rounds
  uint8_t Mlimit;
  /// MIMO transmission mode indicator for this sub-frame
  uint8_t Kmimo;
  /// Nsoft parameter related to UE Category
  uint32_t Nsoft;
  /// amplitude of PDSCH (compared to RS) in symbols without pilots
  int16_t sqrt_rho_a;
  /// amplitude of PDSCH (compared to RS) in symbols containing pilots
  int16_t sqrt_rho_b;
} NR_gNB_DLSCH_t;

typedef struct {
  bool active;
  nfapi_nr_dl_tti_ssb_pdu ssb_pdu;
} NR_gNB_SSB_t;

typedef struct {
  int frame;
  int slot;
  nfapi_nr_prach_pdu_t pdu;  
} gNB_PRACH_list_t;

#define NUMBER_OF_NR_PRACH_MAX 8

typedef struct {
  /// \brief ?.
  /// first index: ? [0..1023] (hard coded)
  int16_t *prachF;
  /// \brief ?.
  /// second index: rx antenna [0..63] (hard coded) \note Hard coded array size indexed by \c nb_antennas_rx.
  /// third index: frequency-domain sample [0..ofdm_symbol_size*12[
  int16_t **rxsigF;
  /// \brief local buffer to compute prach_ifft
  int32_t *prach_ifft;
  gNB_PRACH_list_t list[NUMBER_OF_NR_PRACH_MAX];
} NR_gNB_PRACH;

typedef struct {
  /// Nfapi ULSCH PDU
  nfapi_nr_pusch_pdu_t ulsch_pdu;
  /// Frame where current HARQ round was sent
  uint32_t frame;
  /// Slot where current HARQ round was sent
  uint32_t slot;
  /// Index of current HARQ round for this DLSCH
  uint8_t round;
  uint8_t ndi;
  bool new_rx;
  /// Last TPC command
  uint8_t TPC;
  /// MIMO mode for this DLSCH
  MIMO_mode_t mimo_mode;
  /// Flag indicating that this ULSCH has been allocated by a DCI (otherwise it is a retransmission based on PHICH NAK)
  uint8_t dci_alloc;
  /// Flag indicating that this ULSCH has been allocated by a RAR (otherwise it is a retransmission based on PHICH NAK or DCI)
  uint8_t rar_alloc;
  /// Status Flag indicating for this ULSCH (idle,active,disabled)
  NR_SCH_status_t status;
  /// Subframe scheduling indicator (i.e. Transmission opportunity indicator)
  uint8_t subframe_scheduling_flag;
  /// Subframe cba scheduling indicator (i.e. CBA Transmission opportunity indicator)
  uint8_t subframe_cba_scheduling_flag;
  /// PHICH active flag
  uint8_t phich_active;
  /// PHICH ACK
  uint8_t phich_ACK;
  /// First Allocated RB - previous scheduling. This is needed for PHICH generation which is done after a new scheduling
  uint16_t previous_first_rb;
  /// Flag to indicate that the UL configuration has been handled. Used to remove a stale ULSCH when frame wraps around
  uint8_t handled;
  /// Flag to indicate that this ULSCH is for calibration information sent from UE (i.e. no MAC SDU to pass up)
  //  int calibration_flag;
  /// delta_TF for power control
  int32_t delta_TF;

  
  /////////////////////// ulsch decoding ///////////////////////
  /// Transport block size (This is A from 38.212 V15.4.0 section 5.1)
  uint32_t TBS;
  /// Pointer to the payload (38.212 V15.4.0 section 5.1)
  uint8_t *b;
  /// The payload + CRC (24 bits) in bits (38.212 V15.4.0 section 5.1)
  uint32_t B;
  /// Pointers to code blocks after code block segmentation and CRC attachment (38.212 V15.4.0 section 5.2.2)
  uint8_t *c[MAX_NUM_NR_ULSCH_SEGMENTS];
  /// Number of bits in each code block (38.212 V15.4.0 section 5.2.2)
  uint32_t K;
  /// Number of "Filler" bits added in the code block segmentation (38.212 V15.4.0 section 5.2.2)
  uint32_t F;
  /// Number of code blocks after code block segmentation (38.212 V15.4.0 section 5.2.2)
  uint32_t C;
  /// Pointers to code blocks after LDPC coding (38.212 V15.4.0 section 5.3.2)
  int16_t *d[MAX_NUM_NR_ULSCH_SEGMENTS];
  /// LDPC processing buffer
  t_nrLDPC_procBuf* p_nrLDPC_procBuf[MAX_NUM_NR_ULSCH_SEGMENTS];
  /// LDPC lifting size (38.212 V15.4.0 table 5.3.2-1)
  uint32_t Z;
  /// code blocks after bit selection in rate matching for LDPC code (38.212 V15.4.0 section 5.4.2.1)
  int16_t e[MAX_NUM_NR_DLSCH_SEGMENTS][3*8448];
  /// Number of bits in each code block after rate matching for LDPC code (38.212 V15.4.0 section 5.4.2.1)
  uint32_t E;
  /// Number of segments processed so far
  uint32_t processedSegments;
  //////////////////////////////////////////////////////////////


  /////////////////////////// DMRS /////////////////////////////
  /// n_DMRS  for cyclic shift of DMRS (36.213 Table 9.1.2-2)
  uint8_t n_DMRS;
  /// n_DMRS 2 for cyclic shift of DMRS (36.211 Table 5.5.1.1.-1)
  uint8_t n_DMRS2;
  /// n_DMRS  for cyclic shift of DMRS (36.213 Table 9.1.2-2) - previous scheduling
  /// This is needed for PHICH generation which
  /// is done after a new scheduling
  uint8_t previous_n_DMRS;
  //////////////////////////////////////////////////////////////


  ///////////////////// UCI multiplexing ///////////////////////
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
  /// Temporary h sequence to flag PUSCH_x/PUSCH_y symbols which are not scrambled
  uint8_t h[MAX_NUM_CHANNEL_BITS];
  /// soft bits for each received segment ("w"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  int16_t *w[MAX_NUM_NR_ULSCH_SEGMENTS];
  //////////////////////////////////////////////////////////////
} NR_UL_gNB_HARQ_t;


typedef struct {
  /// Pointers to 16 HARQ processes for the ULSCH
  NR_UL_gNB_HARQ_t *harq_processes[NR_MAX_ULSCH_HARQ_PROCESSES];
  /// Current HARQ process id
  int harq_process_id[NR_MAX_SLOTS_PER_FRAME];
  /// HARQ process mask, indicates which processes are currently active
  uint16_t harq_mask;
  /// ACK/NAK Bundling flag
  uint8_t bundling;
  /// beta_offset_cqi times 8
  uint16_t beta_offset_cqi_times8;
  /// beta_offset_ri times 8
  uint16_t beta_offset_ri_times8;
  /// beta_offset_harqack times 8
  uint16_t beta_offset_harqack_times8;
  /// Flag to indicate that gNB awaits UE Msg3
  uint8_t Msg3_active;
  /// Flag to indicate that gNB should decode UE Msg3
  uint8_t Msg3_flag;
  /// Subframe for Msg3
  uint8_t Msg3_subframe;
  /// Frame for Msg3
  uint32_t Msg3_frame;
  /// Allocated RNTI for this ULSCH
  uint16_t rnti;
  /// RNTI type
  uint8_t rnti_type;
  /// cyclic shift for DM RS
  uint8_t cyclicShift;
  /// for cooperative communication
  uint8_t cooperation_flag;
  /// Maximum number of HARQ rounds
  uint8_t Mlimit;
  /// Maximum number of LDPC iterations
  uint8_t max_ldpc_iterations;
  /// number of iterations used in last LDPC decoding
  uint8_t last_iteration_cnt;  
} NR_gNB_ULSCH_t;

typedef struct {
  uint8_t active;
  /// Frame where current PUCCH pdu was sent
  uint32_t frame;
  /// Slot where current PUCCH pdu was sent
  uint32_t slot;
  /// ULSCH PDU
  nfapi_nr_pucch_pdu_t pucch_pdu;
} NR_gNB_PUCCH_t;

typedef struct {
  /// \brief Pointers (dynamic) to the received data in the time domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdata;
  /// \brief Pointers (dynamic) to the received data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdataF;
  /// \brief holds the transmit data in the frequency domain.
  /// For IFFT_FPGA this points to the same memory as PHY_vars->rx_vars[a].RX_DMA_BUFFER. //?
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: tx antenna [0..14[ where 14 is the total supported antenna ports.
  /// - third index: sample [0..samples_per_frame_woCP]
  int32_t **txdataF;
  /// \brief Anaglogue beam ID for each OFDM symbol (used when beamforming not done in RU)
  /// - first index: antenna port
  /// - second index: beam_id [0.. symbols_per_frame[
  uint8_t **beam_id;  
  int32_t *debugBuff;
  int32_t debugBuff_sample_offset;
} NR_gNB_COMMON;


typedef struct {
  /// \brief Holds the received data in the frequency domain for the allocated RBs in repeated format.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size[
  int32_t **rxdataF_ext;
  /// \brief Holds the received data in the frequency domain for the allocated RBs in normal format.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index (definition from phy_init_lte_eNB()): ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_ext2;
  /// \brief Hold the channel estimates in time domain based on DRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..4*ofdm_symbol_size[
  int32_t **ul_ch_estimates_time;
  /// \brief Hold the channel estimates in frequency domain based on DRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_estimates;
  /// \brief Uplink channel estimates extracted in PRBS.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_estimates_ext;
  /// \brief Hold the PTRS phase estimates in frequency domain.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_ptrs_estimates;
  /// \brief Uplink phase estimates extracted in PRBS.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_ptrs_estimates_ext;
  /// \brief Holds the compensated signal.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_comp;
  /// \brief Magnitude of the UL channel estimates. Used for 2nd-bit level thresholds in LLR computation
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_mag;
  /// \brief Magnitude of the UL channel estimates scaled for 3rd bit level thresholds in LLR computation
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_magb;
  /// \brief Cross-correlation of two UE signals.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: symbol [0..]
  int32_t **rho;
  /// \f$\log_2(\max|H_i|^2)\f$
  int16_t log2_maxh;
  /// \brief Magnitude of Uplink Channel first layer (16QAM level/First 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_UL[
  int32_t **ul_ch_mag0;
  /// \brief Magnitude of Uplink Channel second layer (16QAM level/First 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_UL[
  int32_t **ul_ch_mag1[8][8];
  /// \brief Magnitude of Uplink Channel, first layer (2nd 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_UL[
  int32_t **ul_ch_magb0;
  /// \brief Magnitude of Uplink Channel second layer (2nd 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_UL[
  int32_t **ul_ch_magb1[8][8];
  /// measured RX power based on DRS
  int ulsch_power[8];
  /// total signal over antennas
  int ulsch_power_tot;
  /// measured RX noise power
  int ulsch_noise_power[8];
  /// total noise over antennas
  int ulsch_noise_power_tot;
  /// \brief llr values.
  /// - first index: ? [0..1179743] (hard coded)
  int16_t *llr;
  /// DMRS symbol index, to be updated every DMRS symbol within a slot.
  uint8_t dmrs_symbol;
  // PTRS symbol index, to be updated every PTRS symbol within a slot.
  uint8_t ptrs_symbol_index;
  /// bit mask of PT-RS ofdm symbol indicies
  uint16_t ptrs_symbols;
  // PTRS subcarriers per OFDM symbol
  int32_t ptrs_re_per_slot;
  /// \brief Estimated phase error based upon PTRS on each symbol .
  /// - first index: ? [0..7] Number of Antenna
  /// - second index: ? [0...14] smybol per slot
  int32_t **ptrs_phase_per_slot;
  /// \brief Total RE count after DMRS/PTRS RE's are extracted from respective symbol.
  /// - first index: ? [0...14] smybol per slot
  int16_t *ul_valid_re_per_slot;
  /// flag to verify if channel level computation is done
  uint8_t cl_done;
  /// flag to indicate DTX on reception
  int DTX;
} NR_gNB_PUSCH;

/// Context data structure for RX/TX portion of slot processing
typedef struct {
  /// Component Carrier index
  uint8_t              CC_id;
  /// timestamp transmitted to HW
  openair0_timestamp timestamp_tx;
  /// slot to act upon for transmission
  int slot_tx;
  /// slot to act upon for reception
  int slot_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for reception
  int frame_rx;
  /// \brief Instance count for RXn-TXnp4 processing thread.
  /// \internal This variable is protected by \ref mutex_rxtx.
  int instance_cnt;
  /// pthread structure for RXn-TXnp4 processing thread
  pthread_t pthread;
  /// pthread attributes for RXn-TXnp4 processing thread
  pthread_attr_t attr;
  /// condition variable for tx processing thread
  pthread_cond_t cond;
  /// mutex for RXn-TXnp4 processing thread
  pthread_mutex_t mutex;
  /// scheduling parameters for RXn-TXnp4 thread
  struct sched_param sched_param_rxtx;

  /// \internal This variable is protected by \ref mutex_RUs_tx.
  int instance_cnt_RUs;
  /// condition variable for tx processing thread
  pthread_cond_t cond_RUs;
  /// mutex for L1 RXTX processing thread
  pthread_mutex_t mutex_RUs;
  /// mutex for L1 TX FH synchronization
  pthread_mutex_t mutex_RUs_tx;
} gNB_L1_rxtx_proc_t;


/// Context data structure for eNB slot processing
typedef struct gNB_L1_proc_t_s {
  /// Component Carrier index
  uint8_t              CC_id;
  /// thread index
  int thread_index;
  /// timestamp received from HW
  openair0_timestamp timestamp_rx;
  /// timestamp to send to "slave rru"
  openair0_timestamp timestamp_tx;
  /// slot to act upon for reception
  int slot_rx;
  /// slot to act upon for PRACH
  int slot_prach;
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for PRACH
  int frame_prach;
  /// \internal This variable is protected by \ref mutex_td.
  int instance_cnt_td;
  /// \internal This variable is protected by \ref mutex_te.
  int instance_cnt_te;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;

  // instance count for over-the-air gNB synchronization
  int instance_cnt_synch;
  /// \internal This variable is protected by \ref mutex_asynch_rxtx.
  int instance_cnt_asynch_rxtx;
  /// pthread structure for eNB single processing thread
  pthread_t pthread_single;
  /// pthread structure for asychronous RX/TX processing thread
  pthread_t pthread_asynch_rxtx;
  /// pthread structure for dumping L1 stats
  pthread_t L1_stats_thread;
  /// pthread structure for printing time meas
  pthread_t process_stats_thread;
  /// pthread structure for reordering L1 tx thread messages
  pthread_t pthread_tx_reorder;
  /// flag to indicate first RX acquisition
  int first_rx;
  /// flag to indicate first TX transmission
  int first_tx;
  /// pthread attributes for single gNB processing thread
  pthread_attr_t attr_single;
  /// pthread attributes for prach processing thread
  pthread_attr_t attr_prach;
  /// pthread attributes for asynchronous RX thread
  pthread_attr_t attr_asynch_rxtx;
  /// scheduling parameters for parallel turbo-decoder thread
  struct sched_param sched_param_td;
  /// scheduling parameters for parallel turbo-encoder thread
  struct sched_param sched_param_te;
  /// scheduling parameters for single eNB thread
  struct sched_param sched_param_single;
  /// scheduling parameters for prach thread
  struct sched_param sched_param_prach;
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  pthread_cond_t cond_prach;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// mutex for parallel turbo-decoder thread
  pthread_mutex_t mutex_td;
  /// mutex for parallel turbo-encoder thread
  pthread_mutex_t mutex_te;
  /// mutex for PRACH thread
  pthread_mutex_t mutex_prach;
  /// mutex for asynch RX/TX thread
  pthread_mutex_t mutex_asynch_rxtx;
  /// mutex for RU access to eNB processing (PDSCH/PUSCH)
  pthread_mutex_t mutex_RU;
  /// mutex for RU_tx access to eNB_tx processing (PDSCH/PUSCH)
  pthread_mutex_t mutex_RU_tx;
  /// mutex for RU access to eNB processing (PRACH)
  pthread_mutex_t mutex_RU_PRACH;
  /// mutex for RU access to eNB processing (PRACH BR)
  pthread_mutex_t mutex_RU_PRACH_br;
  /// mask for RUs serving eNB (PDSCH/PUSCH)
  int RU_mask, RU_mask_tx;
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach;
  /// set of scheduling variables RXn-TXnp4 threads
  gNB_L1_rxtx_proc_t L1_proc, L1_proc_tx;
} gNB_L1_proc_t;



typedef struct {
  // common measurements
  //! estimated noise power (linear)
  unsigned int   n0_power[MAX_NUM_RU_PER_gNB];
  //! estimated noise power (dB)
  unsigned int n0_power_dB[MAX_NUM_RU_PER_gNB];
  //! total estimated noise power (linear)
  unsigned int   n0_power_tot;
  //! estimated avg noise power (dB)
  unsigned int n0_power_tot_dB;
  //! estimated avg noise power (dB)
  int n0_power_tot_dBm;
  //! estimated avg noise power per RB per RX ant (lin)
  unsigned int n0_subband_power[MAX_NUM_RU_PER_gNB][275];
  //! estimated avg noise power per RB per RX ant (dB)
  unsigned int n0_subband_power_dB[MAX_NUM_RU_PER_gNB][275];
  //! estimated avg subband noise power (dB)
  unsigned int n0_subband_power_avg_dB;
  //! estimated avg subband noise power per antenna (dB)
  unsigned int n0_subband_power_avg_perANT_dB[NB_ANTENNAS_RX];
  //! estimated avg noise power per RB (dB)
  int n0_subband_power_tot_dB[275];
  //! estimated avg noise power per RB (dBm)
  int n0_subband_power_tot_dBm[275];

  // gNB measurements (per user)
  //! estimated received spatial signal power (linear)
  unsigned int   rx_spatial_power[NUMBER_OF_NR_ULSCH_MAX][NB_ANTENNAS_TX][NB_ANTENNAS_RX];
  //! estimated received spatial signal power (dB)
  unsigned int rx_spatial_power_dB[NUMBER_OF_NR_ULSCH_MAX][NB_ANTENNAS_TX][NB_ANTENNAS_RX];
  //! estimated rssi (dBm)
  int            rx_rssi_dBm[NUMBER_OF_NR_ULSCH_MAX];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_NR_ULSCH_MAX][2];
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_NR_ULSCH_MAX][2];

  /// Wideband CQI (= SINR)
  int            wideband_cqi[NUMBER_OF_NR_ULSCH_MAX][MAX_NUM_RU_PER_gNB];
  /// Wideband CQI in dB (= SINR dB)
  int            wideband_cqi_dB[NUMBER_OF_NR_ULSCH_MAX][MAX_NUM_RU_PER_gNB];
  /// Wideband CQI (sum of all RX antennas, in dB)
  char           wideband_cqi_tot[NUMBER_OF_NR_ULSCH_MAX];
  /// Subband CQI per RX antenna and RB (= SINR)
  int            subband_cqi[NUMBER_OF_NR_ULSCH_MAX][MAX_NUM_RU_PER_gNB][275];
  /// Total Subband CQI and RB (= SINR)
  int            subband_cqi_tot[NUMBER_OF_NR_ULSCH_MAX][275];
  /// Subband CQI in dB and RB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_NR_ULSCH_MAX][MAX_NUM_RU_PER_gNB][275];
  /// Total Subband CQI and RB
  int            subband_cqi_tot_dB[NUMBER_OF_NR_ULSCH_MAX][275];
  /// PRACH background noise level
  int            prach_I0;
} PHY_MEASUREMENTS_gNB;

#define MAX_NUM_NR_PRACH_PREAMBLES 64
#define MAX_NUM_NR_RX_RACH_PDUS 4
#define MAX_NUM_NR_RX_PRACH_PREAMBLES 4
#define MAX_UL_PDUS_PER_SLOT 8
#define MAX_NUM_NR_SRS_PDUS 8
#define MAX_NUM_NR_UCI_PDUS 8

/// Top-level PHY Data Structure for gNB
typedef struct PHY_VARS_gNB_s {
  /// Module ID indicator for this instance
  module_id_t          Mod_id;
  uint8_t              CC_id;
  uint8_t              configured;
  gNB_L1_proc_t        proc;
  int                  single_thread_flag;
  int                  abstraction_flag;
  int                  num_RU;
  RU_t                 *RU_list[MAX_NUM_RU_PER_gNB];
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t         eth_params_n;
  /// Ethernet parameters for fronthaul interface
  eth_params_t         eth_params;
  int                  rx_total_gain_dB;
  int                  (*nr_start_if)(struct RU_t_s *ru, struct PHY_VARS_gNB_s *gNB);
  uint8_t              local_flag;
  nfapi_nr_config_request_scf_t  gNB_config;
  NR_DL_FRAME_PARMS    frame_parms;
  PHY_MEASUREMENTS_gNB measurements;
  NR_IF_Module_t       *if_inst;
  NR_UL_IND_t          UL_INFO;
  pthread_mutex_t      UL_INFO_mutex;

  /// NFAPI RX ULSCH information
  nfapi_nr_rx_data_pdu_t  rx_pdu_list[MAX_UL_PDUS_PER_SLOT];
  /// NFAPI RX ULSCH CRC information
  nfapi_nr_crc_t crc_pdu_list[MAX_UL_PDUS_PER_SLOT];
  /// NFAPI SRS information
  nfapi_nr_srs_indication_pdu_t srs_pdu_list[MAX_NUM_NR_SRS_PDUS];
  /// NFAPI UCI information
  nfapi_nr_uci_t uci_pdu_list[MAX_NUM_NR_UCI_PDUS];
  /// NFAPI PRACH information
  nfapi_nr_prach_indication_pdu_t prach_pdu_indication_list[MAX_NUM_NR_RX_RACH_PDUS];
  /// NFAPI PRACH information
  nfapi_nr_prach_indication_preamble_t preamble_list[MAX_NUM_NR_RX_PRACH_PREAMBLES];

  //Sched_Rsp_t         Sched_INFO;
  nfapi_nr_ul_tti_request_t     UL_tti_req;
  nfapi_nr_uci_indication_t uci_indication;
  
  //  nfapi_nr_dl_tti_pdcch_pdu    *pdcch_pdu;
  //  nfapi_nr_ul_dci_request_pdus_t  *ul_dci_pdu;
  uint16_t num_pdsch_rnti[80];
  NR_gNB_PBCH        pbch;
  nr_cce_t           cce_list[MAX_DCI_CORESET][NR_MAX_PDCCH_AGG_LEVEL];
  NR_gNB_COMMON      common_vars;
  NR_gNB_PRACH       prach_vars;
  NR_gNB_PUSCH       *pusch_vars[NUMBER_OF_NR_ULSCH_MAX];
  NR_gNB_PUCCH_t     *pucch[NUMBER_OF_NR_PUCCH_MAX];
  NR_gNB_PDCCH_t     pdcch_pdu[NUMBER_OF_NR_PDCCH_MAX];
  NR_gNB_UL_PDCCH_t  ul_pdcch_pdu[NUMBER_OF_NR_PDCCH_MAX];
  NR_gNB_DLSCH_t     *dlsch[NUMBER_OF_NR_DLSCH_MAX][2];    // Nusers times two spatial streams
  NR_gNB_ULSCH_t     *ulsch[NUMBER_OF_NR_ULSCH_MAX][2];  // [Nusers times][2 codewords] 
  NR_gNB_DLSCH_t     *dlsch_SI,*dlsch_ra,*dlsch_p;
  NR_gNB_DLSCH_t     *dlsch_PCH;
  /// statistics for DLSCH measurement collection
  NR_gNB_SCH_STATS_t dlsch_stats[NUMBER_OF_NR_SCH_STATS_MAX];
  /// statistics for ULSCH measurement collection
  NR_gNB_SCH_STATS_t ulsch_stats[NUMBER_OF_NR_SCH_STATS_MAX];
  NR_gNB_UCI_STATS_t uci_stats[NUMBER_OF_NR_UCI_STATS_MAX];
  t_nrPolar_params    *uci_polarParams;

  uint8_t pbch_configured;
  char gNB_generate_rar;

  // PUCCH0 Look-up table for cyclic-shifts
  NR_gNB_PUCCH0_LUT_t pucch0_lut;
  /// NR synchronization sequences
  int16_t d_pss[NR_PSS_LENGTH];
  int16_t d_sss[NR_SSS_LENGTH];

  /// PBCH DMRS sequence
  uint32_t nr_gold_pbch_dmrs[2][64][NR_PBCH_DMRS_LENGTH_DWORD];

  /// PBCH interleaver
  uint8_t nr_pbch_interleaver[NR_POLAR_PBCH_PAYLOAD_BITS];

  /// PDCCH DMRS sequence
  uint32_t ***nr_gold_pdcch_dmrs;

  /// PDSCH DMRS sequence
  uint32_t ****nr_gold_pdsch_dmrs;

  /// PUSCH DMRS
  uint32_t ****nr_gold_pusch_dmrs;

  // Mask of occupied RBs, per symbol and PRB
  uint32_t rb_mask_ul[14][9];

  /// CSI  RS sequence
  uint32_t ***nr_gold_csi_rs;

  /// Indicator set to 0 after first SR
  uint8_t first_sr[NUMBER_OF_NR_SR_MAX];

  /// PRACH root sequence
  uint32_t X_u[64][839];

  uint32_t max_peak_val;

  /// OFDM symbol offset divisor for UL
  uint32_t ofdm_offset_divisor;
  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// first index: ? [0..N_RB_DL*12[
  double *sinr_dB;

  /// N0 (used for abstraction)
  double N0;

  unsigned char first_run_I0_measurements;


  unsigned char    is_secondary_gNB; // primary by default
  unsigned char    is_init_sync;     /// Flag to tell if initial synchronization is performed. This affects how often the secondary eNB will listen to the PSS from the primary system.
  unsigned char    has_valid_precoder; /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from, and this B/F vector is created.
  unsigned char    PgNB_id;          /// id of Primary eNB

  /// hold the precoder for NULL beam to the primary user
  int              **dl_precoder_SgNB[3];
  char             log2_maxp; /// holds the maximum channel/precoder coefficient

  int  prb_interpolation;

  /// if ==0 enables phy only test mode
  int mac_enabled;
  /// counter to average prach energh over first 100 prach opportunities
  int prach_energy_counter;

  int pucch0_thres;
  int pusch_thres;
  int prach_thres;
  uint64_t bad_pucch;
  int num_ulprbbl;
  int ulprbbl[275];
  /*
  time_stats_t phy_proc;
  */
  time_stats_t *phy_proc_tx[2];
  time_stats_t phy_proc_rx;
  time_stats_t rx_prach;
  /*
  time_stats_t ofdm_mod_stats;
  */
  time_stats_t dlsch_encoding_stats;
  time_stats_t dlsch_modulation_stats;
  time_stats_t dlsch_scrambling_stats;
  time_stats_t dlsch_resource_mapping_stats;
  time_stats_t dlsch_layer_mapping_stats;
  time_stats_t dlsch_precoding_stats;
  time_stats_t tinput;
  time_stats_t tprep;
  time_stats_t tparity;
  time_stats_t toutput;
  
  time_stats_t dlsch_rate_matching_stats;
  time_stats_t dlsch_interleaving_stats;
  time_stats_t dlsch_segmentation_stats;

  time_stats_t rx_pusch_stats;
  time_stats_t ul_indication_stats;
  time_stats_t ulsch_decoding_stats;
  time_stats_t ulsch_rate_unmatching_stats;
  time_stats_t ulsch_ldpc_decoding_stats;
  time_stats_t ulsch_deinterleaving_stats;
  time_stats_t ulsch_unscrambling_stats;
  time_stats_t ulsch_channel_estimation_stats;
  time_stats_t ulsch_ptrs_processing_stats;
  time_stats_t ulsch_channel_compensation_stats;
  time_stats_t ulsch_rbs_extraction_stats;
  time_stats_t ulsch_mrc_stats;
  time_stats_t ulsch_llr_stats;

  /*
  time_stats_t rx_dft_stats;
  time_stats_t ulsch_freq_offset_estimation_stats;
  */
  notifiedFIFO_t *respDecode;
  notifiedFIFO_t *resp_L1;
  notifiedFIFO_t *L1_tx_free;
  notifiedFIFO_t *L1_tx_filled;
  notifiedFIFO_t *L1_tx_out;
  notifiedFIFO_t *resp_RU_tx;
  tpool_t *threadPool;
  int nbDecode;
  uint8_t thread_pool_size;
  int number_of_nr_dlsch_max;
  int number_of_nr_ulsch_max;
  void * scopeData;
} PHY_VARS_gNB;

typedef struct LDPCDecode_s {
  PHY_VARS_gNB *gNB;
  NR_UL_gNB_HARQ_t *ulsch_harq;
  t_nrLDPC_dec_params decoderParms;
  NR_gNB_ULSCH_t *ulsch;
  short* ulsch_llr; 
  int ulsch_id;
  int harq_pid;
  int rv_index;
  int A;
  int E;
  int Kc;
  int Qm;
  int Kr_bytes;
  int nbSegments;
  int segment_r;
  int r_offset;
  int offset;
  int Tbslbrm;
  int decodeIterations;
} ldpcDecode_t;

struct ldpcReqId {
  uint16_t rnti;
  uint16_t frame;
  uint8_t  subframe;
  uint8_t  codeblock;
  uint16_t spare;
} __attribute__((packed));

union ldpcReqUnion {
  struct ldpcReqId s;
  uint64_t p;
};

typedef struct processingData_L1 {
  int frame_rx;
  int frame_tx;
  int slot_rx;
  int slot_tx;
  openair0_timestamp timestamp_tx;
  PHY_VARS_gNB *gNB;
} processingData_L1_t;

typedef enum {
  FILLED,
  FILLING,
  NOT_FILLED
} msgStatus_t;

typedef struct processingData_L1tx {
  int frame;
  int slot;
  openair0_timestamp timestamp_tx;
  PHY_VARS_gNB *gNB;
  nfapi_nr_dl_tti_pdcch_pdu pdcch_pdu;
  nfapi_nr_ul_dci_request_pdus_t ul_pdcch_pdu;
  NR_gNB_CSIRS_t csirs_pdu[NUMBER_OF_NR_CSIRS_MAX];
  NR_gNB_DLSCH_t *dlsch[NUMBER_OF_NR_DLSCH_MAX][2];
  NR_gNB_SSB_t ssb[64];
  uint16_t num_pdsch_slot;
  time_stats_t phy_proc_tx;
} processingData_L1tx_t;

#endif
