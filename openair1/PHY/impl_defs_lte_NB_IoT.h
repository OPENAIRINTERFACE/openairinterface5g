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

/*! \file PHY/impl_defs_lte_NB_IoT.h
* \brief LTE Physical channel configuration and variable structure definitions
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

#ifndef __PHY_IMPL_DEFS_NB_IOT__H__
#define __PHY_IMPL_DEFS_NB_IOT__H__

#include <asn_SEQUENCE_OF.h>
#include "types_NB_IoT.h"
//#include "defs.h"

typedef enum {TDD_NB_IoT=1,FDD_NB_IoT=0} NB_IoT_frame_type_t;
typedef enum {EXTENDED_NB_IoT=1,NORMAL_NB_IoT=0} NB_IoT_prefix_type_t;
typedef enum {SF_DL_NB_IoT, SF_UL_NB_IoT, SF_S_NB_IoT} NB_IoT_subframe_t;

/////////////////////////
  /// Union for \ref TPC_PDCCH_CONFIG::tpc_Index.
typedef union {
  /// Index of N when DCI format 3 is used. See TS 36.212 (5.3.3.1.6). {[1..15]}
  uint8_t indexOfFormat3;
  /// Index of M when DCI format 3A is used. See TS 36.212 (5.3.3.1.7). {[1..31]}
  uint8_t indexOfFormat3A;
} TPC_INDEX_NB_IoT_t;

/// TPC-PDCCH-Config Information Element from 36.331 RRC spec
typedef struct {
  /// RNTI for power control using DCI format 3/3A, see TS 36.212. {[0..65535]}
  uint16_t rnti;
  /// Index of N or M, see TS 36.212 (5.3.3.1.6 and 5.3.3.1.7), where N or M is dependent on the used DCI format (i.e. format 3 or 3a).
  TPC_INDEX_NB_IoT_t tpc_Index;
} TPC_PDCCH_CONFIG_NB_IoT;
  /// Enumeration for parameter \f$N_\text{ANRep}\f$ \ref PUCCH_CONFIG_DEDICATED::repetitionFactor.
typedef enum {
  //n2=0,
  n4_n,
  n6_n
} ACKNAKREP_NB_IoT_t;

/// Enumeration for \ref PUCCH_CONFIG_DEDICATED::tdd_AckNackFeedbackMode.
typedef enum {
  bundling_N=0,
  multiplexing_N
} ANFBmode_NB_IoT_t;

/// PUCCH-ConfigDedicated from 36.331 RRC spec
typedef struct {
  /// Flag to indicate ACK NAK repetition activation, see TS 36.213 (10.1). {[0..1]}
  uint8_t ackNackRepetition;
  /// Parameter: \f$N_\text{ANRep}\f$, see TS 36.213 (10.1).
  ACKNAKREP_NB_IoT_t repetitionFactor;
  /// Parameter: \f$n^{(1)}_\text{PUCCH,ANRep}\f$, see TS 36.213 (10.1). {[0..2047]}
  uint16_t n1PUCCH_AN_Rep;
  /// Feedback mode, see TS 36.213 (7.3). \details Applied to both PUCCH and PUSCH feedback. For TDD, should always be set to bundling.
  ANFBmode_NB_IoT_t tdd_AckNackFeedbackMode;
} PUCCH_CONFIG_DEDICATED_NB_IoT;
  // UE specific PUSCH configuration.
typedef struct {
  /// Parameter: \f$I^\text{HARQ-ACK}_\text{offset}\f$, see TS 36.213 (Table 8.6.3-1). {[0..15]}
  uint16_t betaOffset_ACK_Index;
  /// Parameter: \f$I^{RI}_\text{offset}\f$, see TS 36.213 (Table 8.6.3-2). {[0..15]}
  uint16_t betaOffset_RI_Index;
  /// Parameter: \f$I^{CQI}_\text{offset}\f$, see TS 36.213 (Table 8.6.3-3). {[0..15]}
  uint16_t betaOffset_CQI_Index;
} PUSCH_CONFIG_DEDICATED_NB_IoT;
/// Enumeration for Parameter \f$P_A\f$ \ref PDSCH_CONFIG_DEDICATED::p_a.
typedef enum {
  //dBm6=0, ///< (dB-6) corresponds to -6 dB
 // dBm477, ///< (dB-4dot77) corresponds to -4.77 dB
 // dBm3,   ///< (dB-3) corresponds to -3 dB
  //dBm177, ///< (dB-1dot77) corresponds to -1.77 dB
  //dB0,    ///< corresponds to 0 dB
 // dB1,    ///< corresponds to 1 dB
  dB2_NB,    ///< corresponds to 2 dB
  dB3_NB     ///< corresponds to 3 dB
} PA_NB_IoT_t;

/// PDSCH-ConfigDedicated from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$P_A\f$, see TS 36.213 (5.2).
  PA_NB_IoT_t p_a;
} PDSCH_CONFIG_DEDICATED_NB_IoT;

/// UplinkPowerControlDedicated Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$P_\text{0\_UE\_PUSCH}(1)\f$, see TS 36.213 (5.1.1.1), unit dB. {[-8..7]}\n This field is applicable for
  /// non-persistent scheduling, only.
  int8_t p0_UE_PUSCH;
  /// Parameter: Ks, see TS 36.213 (5.1.1.1). {[0..1]}\n en0 corresponds to value 0 corresponding to state “disabled”. en1
  /// corresponds to value 1.25 corresponding to “enabled”. \note the specification sais it is an enumerated value. \warning the
  /// enumeration values do not correspond to the given values in the specification (en1 should be 1.25).
  uint8_t deltaMCS_Enabled;
  /// Parameter: Accumulation-enabled, see TS 36.213 (5.1.1.1). {[0..1]} 1 corresponds to "enabled" whereas 0 corresponds to
  /// "disabled".
  uint8_t accumulationEnabled;
  /// Parameter: \f$P_\text{0\_UE\_PUCCH}(1)\f$, see TS 36.213 (5.1.2.1), unit dB. {[-8..7]}
  int8_t p0_UE_PUCCH;
  /// Parameter: \f$P_\text{SRS\_OFFSET}\f$, see TS 36.213 (5.1.3.1). {[0..15]}\n For Ks=1.25 (\ref deltaMCS_Enabled), the actual
  /// parameter value is pSRS_Offset value - 3. For Ks=0, the actual parameter value is -10.5 + 1.5*pSRS_Offset value.
  int8_t pSRS_Offset;
  /// Specifies the filtering coefficient for RSRP measurements used to calculate path loss, as specified in TS 36.213 (5.1.1.1).\details The same filtering mechanism applies as for quantityConfig described in 5.5.3.2. \note the specification sais it is an enumerated value.
  uint8_t filterCoefficient;
} UL_POWER_CONTROL_DEDICATED_NB_IoT;

/// Union for \ref TPC_PDCCH_CONFIG::tpc_Index.
// typedef union {
/// Index of N when DCI format 3 is used. See TS 36.212 (5.3.3.1.6). {[1..15]}
// uint8_t indexOfFormat3;
/// Index of M when DCI format 3A is used. See TS 36.212 (5.3.3.1.7). {[1..31]}
// uint8_t indexOfFormat3A;
//} TPC_INDEX_NB_IoT_t;

/// CQI-ReportPeriodic
typedef struct {
  /// Parameter: \f$n^{(2)}_\text{PUCCH}\f$, see TS 36.213 (7.2). {[0..1185]}, -1 indicates inactivity
  int16_t cqi_PUCCH_ResourceIndex;
  /// Parameter: CQI/PMI Periodicity and Offset Configuration Index \f$I_\text{CQI/PMI}\f$, see TS 36.213 (tables 7.2.2-1A
  /// and 7.2.2-1C). {[0..1023]}
  int16_t cqi_PMI_ConfigIndex;
  /// Parameter: K, see 36.213 (4.2.2). {[1..4]}
  uint8_t K;
  /// Parameter: RI Config Index \f$I_\text{RI}\f$, see TS 36.213 (7.2.2-1B). {[0..1023]}, -1 indicates inactivity
  int16_t ri_ConfigIndex;
  /// Parameter: Simultaneous-AN-and-CQI, see TS 36.213 (10.1). {[0..1]} 1 indicates that simultaneous transmission of ACK/NACK and
  /// CQI is allowed.
  uint8_t simultaneousAckNackAndCQI;
  /// parameter computed from Tables 7.2.2-1A and 7.2.2-1C
  uint16_t Npd;
  /// parameter computed from Tables 7.2.2-1A and 7.2.2-1C
  uint16_t N_OFFSET_CQI;
} CQI_REPORTPERIODIC_NB_IoT;

/// Enumeration for parameter reporting mode \ref CQI_REPORT_CONFIG::cqi_ReportModeAperiodic.
typedef enum {
  //rm12=0,
  //rm20=1,
  //rm22=2,
  rm30_N=3,
  rm31_N=4
} CQI_REPORTMODEAPERIODIC_NB_IoT;

/// CQI-ReportConfig Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: reporting mode. Value rm12 corresponds to Mode 1-2, rm20 corresponds to Mode 2-0, rm22 corresponds to Mode 2-2 etc. PUSCH reporting modes are described in TS 36.213 [23, 7.2.1].
  CQI_REPORTMODEAPERIODIC_NB_IoT cqi_ReportModeAperiodic;
  /// Parameter: \f$\Delta_\text{offset}\f$, see TS 36.213 (7.2.3). {[-1..6]}\n Actual value = IE value * 2 [dB].
  int8_t nomPDSCH_RS_EPRE_Offset;
  CQI_REPORTPERIODIC_NB_IoT CQI_ReportPeriodic;
} CQI_REPORT_CONFIG_NB_IoT;

/// SoundingRS-UL-ConfigDedicated Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$B_\text{SRS}\f$, see TS 36.211 (table 5.5.3.2-1, 5.5.3.2-2, 5.5.3.2-3 and 5.5.3.2-4). {[0..3]} \note the
  /// specification sais it is an enumerated value.
  uint8_t srs_Bandwidth;
  /// Parameter: SRS hopping bandwidth \f$b_\text{hop}\in\{0,1,2,3\}\f$, see TS 36.211 (5.5.3.2) {[0..3]} \note the specification
  /// sais it is an enumerated value.
  uint8_t srs_HoppingBandwidth;
  /// Parameter: \f$n_\text{RRC}\f$, see TS 36.211 (5.5.3.2). {[0..23]}
  uint8_t freqDomainPosition;
  /// Parameter: Duration, see TS 36.213 (8.2). {[0..1]} 0 corresponds to "single" and 1 to "indefinite".
  uint8_t duration;
  /// Parameter: \f$k_\text{TC}\in\{0,1\}\f$, see TS 36.211 (5.5.3.2). {[0..1]}
  uint8_t transmissionComb;
  /// Parameter: \f$I_\text{SRS}\f$, see TS 36.213 (table 8.2-1). {[0..1023]}
  uint16_t srs_ConfigIndex;
  /// Parameter: \f$n^\text{CS}_\text{SRS}\f$. See TS 36.211 (5.5.3.1). {[0..7]} \note the specification sais it is an enumerated
  /// value.
  uint8_t cyclicShift;
  // Parameter: internal implementation: UE SRS configured
  uint8_t srsConfigDedicatedSetup;
  // Parameter: cell srs subframe for internal implementation
  uint8_t srsCellSubframe;
  // Parameter: ue srs subframe for internal implementation
  uint8_t srsUeSubframe;
} SOUNDINGRS_UL_CONFIG_DEDICATED_NB_IoT;


/// Enumeration for parameter SR transmission \ref SCHEDULING_REQUEST_CONFIG::dsr_TransMax.
typedef enum {
  //sr_n4=0,
 // sr_n8=1,
 // sr_n16=2,
  sr_n32_N=3,
  sr_n64_N=4
} DSR_TRANSMAX_NB_IoT_t;

/// SchedulingRequestConfig Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$n^{(1)}_\text{PUCCH,SRI}\f$, see TS 36.213 (10.1). {[0..2047]}
  uint16_t sr_PUCCH_ResourceIndex;
  /// Parameter: \f$I_\text{SR}\f$, see TS 36.213 (10.1). {[0..155]}
  uint8_t sr_ConfigIndex;
  /// Parameter for SR transmission in TS 36.321 (5.4.4). \details The value n4 corresponds to 4 transmissions, n8 corresponds to 8 transmissions and so on.
  DSR_TRANSMAX_NB_IoT_t dsr_TransMax;
} SCHEDULING_REQUEST_CONFIG_NB_IoT;

typedef struct {
  /// Downlink Power offset field
  uint8_t dl_pow_off;
  ///Subband resource allocation field
  uint8_t rballoc_sub[50];
  ///Total number of PRBs indicator
  uint8_t pre_nb_available_rbs;
} MU_MIMO_mode_NB_IoT;
////////////////////////

typedef struct {

    /// \brief Holds the received data in the frequency domain.
    /// - first index: rx antenna [0..nb_antennas_rx[
    /// - second index: symbol [0..28*ofdm_symbol_size[
    int32_t **rxdataF;

    /// \brief Hold the channel estimates in frequency domain.
    /// - first index: eNB id [0..6] (hard coded)
    /// - second index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
    /// - third index: samples? [0..symbols_per_tti*(ofdm_symbol_size+LTE_CE_FILTER_LENGTH)[
    int32_t **dl_ch_estimates[7];

    /// \brief Hold the channel estimates in time domain (used for tracking).
    /// - first index: eNB id [0..6] (hard coded)
    /// - second index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
    /// - third index: samples? [0..2*ofdm_symbol_size[
    int32_t **dl_ch_estimates_time[7];
}NB_IoT_UE_COMMON_PER_THREAD;

typedef struct {
  /// \brief Holds the transmit data in time domain.
  /// For IFFT_FPGA this points to the same memory as PHY_vars->tx_vars[a].TX_DMA_BUFFER.
  /// - first index: tx antenna [0..nb_antennas_tx[
  /// - second index: sample [0..FRAME_LENGTH_COMPLEX_SAMPLES[
  int32_t **txdata;
  /// \brief Holds the transmit data in the frequency domain.
  /// For IFFT_FPGA this points to the same memory as PHY_vars->rx_vars[a].RX_DMA_BUFFER.
  /// - first index: tx antenna [0..nb_antennas_tx[
  /// - second index: sample [0..FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX[
  int32_t **txdataF;

  /// \brief Holds the received data in time domain.
  /// Should point to the same memory as PHY_vars->rx_vars[a].RX_DMA_BUFFER.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: sample [0..FRAME_LENGTH_COMPLEX_SAMPLES+2048[
  int32_t **rxdata;

  NB_IoT_UE_COMMON_PER_THREAD common_vars_rx_data_per_thread[2];

  /// holds output of the sync correlator
  int32_t *sync_corr;
  /// estimated frequency offset (in radians) for all subcarriers
  int32_t freq_offset;
  /// eNb_id user is synched to
  int32_t eNb_id;
} NB_IoT_UE_COMMON;

typedef struct {
  /// \brief Received frequency-domain signal after extraction.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **rxdataF_ext;
  /// \brief Received frequency-domain ue specific pilots.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..12*N_RB_DL[
  int32_t **rxdataF_uespec_pilots;
  /// \brief Received frequency-domain signal after extraction and channel compensation.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **rxdataF_comp0;
  /// \brief Received frequency-domain signal after extraction and channel compensation for the second stream. For the SIC receiver we need to store the history of this for each harq process and round
  /// - first index: ? [0..7] (hard coded) accessed via \c harq_pid
  /// - second index: ? [0..7] (hard coded) accessed via \c round
  /// - third index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - fourth index: ? [0..168*N_RB_DL[
  int32_t **rxdataF_comp1[8][8];
  /// \brief Downlink channel estimates extracted in PRBS.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_estimates_ext;
  /// \brief Downlink cross-correlation of MIMO channel estimates (unquantized PMI) extracted in PRBS. For the SIC receiver we need to store the history of this for each harq process and round
  /// - first index: ? [0..7] (hard coded) accessed via \c harq_pid
  /// - second index: ? [0..7] (hard coded) accessed via \c round
  /// - third index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - fourth index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_rho_ext[8][8];
  /// \brief Downlink beamforming channel estimates in frequency domain.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: samples? [0..symbols_per_tti*(ofdm_symbol_size+LTE_CE_FILTER_LENGTH)[
  int32_t **dl_bf_ch_estimates;
  /// \brief Downlink beamforming channel estimates.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_bf_ch_estimates_ext;
  /// \brief Downlink cross-correlation of MIMO channel estimates (unquantized PMI) extracted in PRBS.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_rho2_ext;
  /// \brief Downlink PMIs extracted in PRBS and grouped in subbands.
  /// - first index: ressource block [0..N_RB_DL[
  uint8_t *pmi_ext;
  /// \brief Magnitude of Downlink Channel first layer (16QAM level/First 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_mag0;
  /// \brief Magnitude of Downlink Channel second layer (16QAM level/First 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_mag1[8][8];
  /// \brief Magnitude of Downlink Channel, first layer (2nd 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_magb0;
  /// \brief Magnitude of Downlink Channel second layer (2nd 64QAM level).
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **dl_ch_magb1[8][8];
  /// \brief Cross-correlation of two eNB signals.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: symbol [0..]
  int32_t **rho;
  /// never used... always send dl_ch_rho_ext instead...
  int32_t **rho_i;
  /// \brief Pointers to llr vectors (2 TBs).
  /// - first index: ? [0..1] (hard coded)
  /// - second index: ? [0..1179743] (hard coded)
  int16_t *llr[2];
  /// \f$\log_2(\max|H_i|^2)\f$
  int16_t log2_maxh;
    /// \f$\log_2(\max|H_i|^2)\f$ //this is for TM3-4 layer1 channel compensation
  int16_t log2_maxh0;
    /// \f$\log_2(\max|H_i|^2)\f$ //this is for TM3-4 layer2 channel commpensation
  int16_t log2_maxh1;
  /// \brief LLR shifts for subband scaling.
  /// - first index: ? [0..168*N_RB_DL[
  uint8_t *llr_shifts;
  /// \brief Pointer to LLR shifts.
  /// - first index: ? [0..168*N_RB_DL[
  uint8_t *llr_shifts_p;
  /// \brief Pointers to llr vectors (128-bit alignment).
  /// - first index: ? [0..0] (hard coded)
  /// - second index: ? [0..]
  int16_t **llr128;
  /// \brief Pointers to llr vectors (128-bit alignment).
  /// - first index: ? [0..0] (hard coded)
  /// - second index: ? [0..]
  int16_t **llr128_2ndstream;
  //uint32_t *rb_alloc;
  //uint8_t Qm[2];
  //MIMO_mode_t mimo_mode;
} NB_IoT_UE_PDSCH;

/// NPRACH-ParametersList-NB-r13 from 36.331 RRC spec
typedef struct NPRACH_Parameters_NB_IoT{
  /// the period time for nprach
  uint16_t nprach_Periodicity;
  /// for the start time for the NPRACH resource from 40ms-2560ms
  uint16_t nprach_StartTime;
  /// for the subcarrier of set to the NPRACH preamble from n0 - n34
  uint16_t nprach_SubcarrierOffset;
  ///number of subcarriers in a NPRACH resource allowed values (n12,n24,n36,n48)
  uint16_t nprach_NumSubcarriers;
  /// where is the region that in NPRACH resource to indicate if this UE support MSG3 for multi-tone or not. from 0 - 1
  uint16_t nprach_SubcarrierMSG3_RangeStart;
  /// The max preamble transmission attempt for the CE level from 1 - 128
  uint16_t maxNumPreambleAttemptCE;
  /// Number of NPRACH repetitions per attempt for each NPRACH resource
  uint16_t numRepetitionsPerPreambleAttempt;
  /// The number of the repetition for DCI use in RAR/MSG3/MSG4 from 1 - 2048 (Rmax)
  uint16_t npdcch_NumRepetitions_RA;
  /// Starting subframe for NPDCCH Common searching space for (RAR/MSG3/MSG4)
  uint16_t npdcch_StartSF_CSS_RA;
  /// Fractional period offset of starting subframe for NPDCCH common search space
  uint16_t npdcch_Offset_RA;
} nprach_parameters_NB_IoT_t;

typedef struct{
  nprach_parameters_NB_IoT_t list[3];
}NPRACH_List_NB_IoT_t;

typedef long RSRP_Range_t;

typedef struct {
  A_SEQUENCE_OF(RSRP_Range_t) list;
}rsrp_ThresholdsNPrachInfoList;


/// NPRACH_ConfigSIB-NB from 36.331 RRC spec
typedef struct {
  /// nprach_CP_Length_r13, for the CP length(unit us) only 66.7 and 266.7 is implemented
  uint16_t nprach_CP_Length;
  /// The criterion for UEs to select a NPRACH resource. Up to 2 RSRP threshold values can be signalled.  {[1..2]}
  rsrp_ThresholdsNPrachInfoList *rsrp_ThresholdsPrachInfoList;
  /// NPRACH Parameters List
  NPRACH_List_NB_IoT_t nprach_ParametersList;

} NPRACH_CONFIG_COMMON;

/// NPDSCH-ConfigCommon from 36.331 RRC spec
typedef struct {
  /// see TS 36.213 (16.2). {[-60..50]}\n Provides the downlink reference-signal EPRE. The actual value in dBm.
  uint16_t nrs_Power;
} NPDSCH_CONFIG_COMMON;

typedef struct{
  /// The base sequence of DMRS sequence in a cell for 3 tones transmission; see TS 36.211 [21, 10.1.4.1.2]. If absent, it is given by NB-IoT CellID mod 12. Value 12 is not used.
  uint16_t threeTone_BaseSequence;
  /// Define 3 cyclic shifts for the 3-tone case, see TS 36.211 [21, 10.1.4.1.2].
  uint16_t threeTone_CyclicShift;
  /// The base sequence of DMRS sequence in a cell for 6 tones transmission; see TS 36.211 [21, 10.1.4.1.2]. If absent, it is given by NB-IoT CellID mod 14. Value 14 is not used.
  uint16_t sixTone_BaseSequence;
  /// Define 4 cyclic shifts for the 6-tone case, see TS 36.211 [21, 10.1.4.1.2].
  uint16_t sixTone_CyclicShift;
  /// The base sequence of DMRS sequence in a cell for 12 tones transmission; see TS 36.211 [21, 10.1.4.1.2]. If absent, it is given by NB-IoT CellID mod 30. Value 30 is not used.
  uint16_t twelveTone_BaseSequence;

}DMRS_CONFIG_t;

/// UL-ReferenceSignalsNPUSCH from 36.331 RRC spec
typedef struct {
  /// Parameter: Group-hopping-enabled, see TS 36.211 (5.5.1.3). {[0..1]}
  uint8_t groupHoppingEnabled;
  /// , see TS 36.211 (5.5.1.3). {[0..29]}
  uint8_t groupAssignmentNPUSCH;
  /// Parameter: cyclicShift, see TS 36.211 (Table 5.5.2.1.1-2). {[0..7]}
  uint8_t cyclicShift;
  /// nPRS for cyclic shift of DRS \note not part of offical UL-ReferenceSignalsPUSCH ASN1 specification.
  uint8_t nPRS[20];
  /// group hopping sequence for DMRS, 36.211, Section 10.1.4.1.3. Second index corresponds to the four possible subcarrier configurations
  uint8_t grouphop[20][4];
  /// sequence hopping sequence for DRS \note not part of offical UL-ReferenceSignalsPUSCH ASN1 specification.
  uint8_t seqhop[20];
} UL_REFERENCE_SIGNALS_NPUSCH_t;


/// PUSCH-ConfigCommon from 36.331 RRC spec.
typedef struct {
  /// Number of repetitions for ACK/NACK HARQ response to NPDSCH containing Msg4 per NPRACH resource, see TS 36.213 [23, 16.4.2].
  uint8_t ack_NACK_NumRepetitions_Msg4[3];
  /// SRS SubframeConfiguration. See TS 36.211 [21, table 5.5.3.3-1]. Value sc0 corresponds to value 0, sc1 to value 1 and so on.
  uint8_t srs_SubframeConfig;
  /// Parameter: \f$N^{HO}_{RB}\f$, see TS 36.211 (5.3.4). {[0..98]}
  DMRS_CONFIG_t dmrs_Config;
  /// Ref signals configuration
  UL_REFERENCE_SIGNALS_NPUSCH_t ul_ReferenceSignalsNPUSCH;

} NPUSCH_CONFIG_COMMON;


typedef struct{
  /// See TS 36.213 [23, 16.2.1.1], unit dBm.
  uint8_t p0_NominalNPUSCH;
  /// See TS 36.213 [23, 16.2.1.1] where al0 corresponds to 0, al04 corresponds to value 0.4, al05 to 0.5, al06 to 0.6, al07 to 0.7, al08 to 0.8, al09 to 0.9 and al1 corresponds to 1. 
  uint8_t alpha;
  /// See TS 36.213 [23, 16.2.1.1]. Actual value = IE value * 2 [dB].
  uint8_t deltaPreambleMsg3;
}UplinkPowerControlCommon_NB_IoT;


/* DL-GapConfig-NB-r13 */
typedef struct {
	uint16_t	 dl_GapThreshold;
	uint16_t	 dl_GapPeriodicity;
	uint16_t	 dl_GapDurationCoeff;
} DL_GapConfig_NB_IoT;

#define NBIOT_INBAND_LTEPCI 0
#define NBIOT_INBAND_IOTPCI 1
#define NBIOT_INGUARD       2
#define NBIOT_STANDALONE    3


typedef struct {
  /// for inband, lte bandwidth
  uint8_t LTE_N_RB_DL;
  uint8_t LTE_N_RB_UL;
  /// Cell ID
  uint16_t Nid_cell;
  /// shift of pilot position in one RB
  uint8_t nushift;
  /// indicates if node is a UE (NODE=2) or eNB (PRIMARY_CH=0).
  uint8_t node_id;
  /// Frequency index of CBMIMO1 card
  uint8_t freq_idx;
  /// RX Frequency for ExpressMIMO/LIME
  uint32_t carrier_freq[4];
  /// TX Frequency for ExpressMIMO/LIME
  uint32_t carrier_freqtx[4];
  /// RX gain for ExpressMIMO/LIME
  uint32_t rxgain[4];
  /// TX gain for ExpressMIMO/LIME
  uint32_t txgain[4];
  /// RF mode for ExpressMIMO/LIME
  uint32_t rfmode[4];
  /// RF RX DC Calibration for ExpressMIMO/LIME
  uint32_t rxdc[4];
  /// RF TX DC Calibration for ExpressMIMO/LIME
  uint32_t rflocal[4];
  /// RF VCO calibration for ExpressMIMO/LIME
  uint32_t rfvcolocal[4];
  /// Turns on second TX of CBMIMO1 card
  uint8_t dual_tx;
  /// flag to indicate SISO transmission
  uint8_t mode1_flag;
  /// Indicator that 20 MHz channel uses 3/4 sampling frequency
  //uint8_t threequarter_fs;
  /// Size of FFT
  uint16_t ofdm_symbol_size;
  /// Number of prefix samples in all but first symbol of slot
  uint16_t nb_prefix_samples;
  /// Number of prefix samples in first symbol of slot
  uint16_t nb_prefix_samples0;
  /// Carrier offset in FFT buffer for first RE in PRB0
  uint16_t first_carrier_offset;
  /// Number of samples in a subframe
  uint32_t samples_per_tti;
  /// Number of OFDM/SC-FDMA symbols in one subframe (to be modified to account for potential different in UL/DL)
  uint16_t symbols_per_tti;
  /// Number of Physical transmit antennas in node
  uint8_t nb_antennas_tx;
  /// Number of Receive antennas in node
  uint8_t nb_antennas_rx;
  /// Number of common transmit antenna ports in eNodeB (1 or 2)
  uint8_t nb_antenna_ports_eNB;
  /// Number of common receiving antenna ports in eNodeB (1 or 2)
  uint8_t nb_antenna_ports_rx_eNB;
  /// NPRACH Config Common (from 36-331 RRC spec)
  NPRACH_CONFIG_COMMON nprach_config_common;
  /// NPDSCH Config Common (from 36-331 RRC spec)
  NPDSCH_CONFIG_COMMON npdsch_config_common;
  /// PUSCH Config Common (from 36-331 RRC spec)
  NPUSCH_CONFIG_COMMON npusch_config_common;
  /// UL Power Control (from 36-331 RRC spec)
  UplinkPowerControlCommon_NB_IoT ul_power_control_config_common;
  /// DL Gap
  DL_GapConfig_NB_IoT DL_gap_config;
  /// Size of SI windows used for repetition of one SI message (in frames)
  uint8_t SIwindowsize;
  /// Period of SI windows used for repetition of one SI message (in frames)
  uint16_t SIPeriod;
  int                 eutra_band;
  uint32_t            dl_CarrierFreq;
  uint32_t            ul_CarrierFreq;
  // CE level to determine the NPRACH Configuration (one CE for each NPRACH config.)
  uint8_t             CE;

  /*
   * index of the PRB assigned to NB-IoT carrier in in-band/guard-band operating mode
   */
  unsigned short NB_IoT_RB_ID;


  /*Following FAPI approach:
   * 0 = in-band with same PCI
   * 1 = in-band with diff PCI
   * 2 = guard band
   * 3 =stand alone
   */
  uint16_t operating_mode;
  /*
   * Only for In-band operating mode with same PCI
   * its measured in number of OFDM symbols
   * allowed values:
   * 1, 2, 3, 4(this value is written in FAPI specs but not exist in TS 36.331 v14.2.1 pag 587)
   * -1 (we put this value when is not defined - other operating mode)
   */
  uint16_t control_region_size;

  /*Number of EUTRA CRS antenna ports (AP)
   * valid only for in-band different PCI mode
   * value 0 = indicates the same number of AP as NRS APs
   * value 1 = four CRS APs
   */
  uint16_t eutra_NumCRS_ports;

  /* Subcarrier bandwidth
  0 -> 3.75 kHz
  1 -> 15 kHz
  */
  uint8_t subcarrier_spacing; 

} NB_IoT_DL_FRAME_PARMS;

typedef struct {
  /// \brief Pointers (dynamic) to the received data in the time domain.
  /// - first index: rx antenna [0..nb_antennas_rx]
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti]
  int32_t **rxdata;
  /// \brief Pointers (dynamic) to the received data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti]
  int32_t **rxdataF;
  /// \brief holds the transmit data in the frequency domain.
  /// For IFFT_FPGA this points to the same memory as PHY_vars->rx_vars[a].RX_DMA_BUFFER. //?
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: tx antenna [0..14[ where 14 is the total supported antenna ports.
  /// - third index: sample [0..]
  int32_t **txdataF; 
} NB_IoT_eNB_COMMON;

typedef struct {
  /// \brief Holds the transmit data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **txdata;
  /// \brief Holds the receive data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdata[3];
  /// \brief Holds the last subframe of received data in time domain after removal of 7.5kHz frequency offset.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: sample [0..samples_per_tti[
  int32_t **rxdata_7_5kHz[3];
  /// \brief Holds the received data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdataF[3];
  /// \brief Holds output of the sync correlator.
  /// - first index: sample [0..samples_per_tti*10[
  uint32_t *sync_corr[3];
} NB_IoT_RU_COMMON;

typedef struct {
  /// \brief Hold the channel estimates in frequency domain based on SRS.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..ofdm_symbol_size[
  int32_t **srs_ch_estimates[3];
  /// \brief Hold the channel estimates in time domain based on SRS.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..2*ofdm_symbol_size[
  int32_t **srs_ch_estimates_time[3];
  /// \brief Holds the SRS for channel estimation at the RX.
  /// - first index: ? [0..ofdm_symbol_size[
  int32_t *srs;
} NB_IoT_eNB_SRS;

typedef struct {
  /// \brief Holds the received data in the frequency domain for the allocated RBs in repeated format.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..2*ofdm_symbol_size[
  /// - third index (definition from phy_init_lte_eNB()): ? [0..24*N_RB_UL*frame_parms->symbols_per_tti[
  /// \warning inconsistent third index definition
  int32_t **rxdataF_ext[3];
  /// \brief Holds the received data in the frequency domain for the allocated RBs in normal format.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index (definition from phy_init_lte_eNB()): ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_ext2[3];
  /// \brief Hold the channel estimates in time domain based on DRS.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..4*ofdm_symbol_size[
  int32_t **drs_ch_estimates_time[3];
  /// \brief Hold the channel estimates in frequency domain based on DRS.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **drs_ch_estimates[3];
  /// \brief Hold the channel estimates for UE0 in case of Distributed Alamouti Scheme.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **drs_ch_estimates_0[3];
  /// \brief Hold the channel estimates for UE1 in case of Distributed Almouti Scheme.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **drs_ch_estimates_1[3];
  /// \brief Holds the compensated signal.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_comp[3];
  /// \brief Hold the compensated data (y)*(h0*) in case of Distributed Alamouti Scheme.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_comp_0[3];
  /// \brief Hold the compensated data (y*)*(h1) in case of Distributed Alamouti Scheme.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_comp_1[3];
  /// \brief ?.
  /// - first index: sector id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_mag[3];
  /// \brief ?.
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_magb[3];
  /// \brief Hold the channel mag for UE0 in case of Distributed Alamouti Scheme.
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_mag_0[3];
  /// \brief Hold the channel magb for UE0 in case of Distributed Alamouti Scheme.
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_magb_0[3];
  /// \brief Hold the channel mag for UE1 in case of Distributed Alamouti Scheme.
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_mag_1[3];
  /// \brief Hold the channel magb for UE1 in case of Distributed Alamouti Scheme.
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: rx antenna id [0..nb_antennas_rx[
  /// - third index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_magb_1[3];
  /// measured RX power based on DRS
  int ulsch_power[2];
  /// measured RX power based on DRS for UE0 in case of Distributed Alamouti Scheme
  int ulsch_power_0[2];
  /// measured RX power based on DRS for UE0 in case of Distributed Alamouti Scheme
  int ulsch_power_1[2];
  /// \brief llr values.
  /// - first index: ? [0..1179743] (hard coded)
  int16_t *llr;
#ifdef LOCALIZATION
  /// number of active subcarrier for a specific UE
  int32_t active_subcarrier;
  /// subcarrier power in dBm
  int32_t *subcarrier_power;
#endif
} NB_IoT_eNB_PUSCH;

#define PBCH_A_NB_IoT 24
typedef struct {
  uint8_t pbch_d[96+(3*(16+PBCH_A_NB_IoT))];
  uint8_t pbch_w[3*3*(16+PBCH_A_NB_IoT)];
  uint8_t pbch_e[1920];
} NB_IoT_eNB_PBCH;


typedef enum {
  /// TM1
  SISO_NB_IoT=0,
  /// TM2
  ALAMOUTI_NB_IoT=1,
  /// TM3
  LARGE_CDD_NB_IoT=2,
  /// the next 6 entries are for TM5
  UNIFORM_PRECODING11_NB_IoT=3,
  UNIFORM_PRECODING1m1_NB_IoT=4,
  UNIFORM_PRECODING1j_NB_IoT=5,
  UNIFORM_PRECODING1mj_NB_IoT=6,
  PUSCH_PRECODING0_NB_IoT=7,
  PUSCH_PRECODING1_NB_IoT=8,
  /// the next 3 entries are for TM4
  DUALSTREAM_UNIFORM_PRECODING1_NB_IoT=9,
  DUALSTREAM_UNIFORM_PRECODINGj_NB_IoT=10,
  DUALSTREAM_PUSCH_PRECODING_NB_IoT=11,
  TM7_NB_IoT=12,
  TM8_NB_IoT=13,
  TM9_10_NB_IoT=14
} MIMO_mode_NB_IoT_t;

typedef struct {
  /// \brief ?.
  /// first index: ? [0..1023] (hard coded)
  int16_t *prachF;
  /// \brief ?.
  /// first index: rx antenna [0..63] (hard coded) \note Hard coded array size indexed by \c nb_antennas_rx.
  /// second index: ? [0..ofdm_symbol_size*12[
  int16_t *rxsigF[64];
  /// \brief local buffer to compute prach_ifft (necessary in case of multiple CCs)
  /// first index: rx antenna [0..63] (hard coded) \note Hard coded array size indexed by \c nb_antennas_rx.
  /// second index: ? [0..2047] (hard coded)
  int16_t *prach_ifft[64];
} NB_IoT_eNB_PRACH;

typedef enum {
  NOT_SYNCHED_NB_IoT=0,
  PRACH_NB_IoT=1,
  RA_RESPONSE_NB_IoT=2,
  PUSCH_NB_IoT=3,
  RESYNCH_NB_IoT=4
} UE_MODE_NB_IoT_t;


#endif
