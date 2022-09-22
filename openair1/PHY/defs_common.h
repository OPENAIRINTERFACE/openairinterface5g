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

/*! \file PHY/defs.h
 \brief Top-level defines and structure definitions
 \author R. Knopp, F. Kaltenberger
 \date 2011
 \version 0.1
 \company Eurecom
 \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
 \note
 \warning
*/

#ifndef __PHY_DEFS_COMMON__H__
#define __PHY_DEFS_COMMON__H__


#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "common_lib.h"
#include <common/utils/LOG/log.h>
#include "assertions.h"

//#include <complex.h>
#include "time_meas.h"
#include "platform_types.h"
#define MAX_NUM_RU_PER_eNB 64

#include <pthread.h>

#include "TOOLS/tools_defs.h"

#include "openairinterface5g_limits.h"
#include "common/utils/LOG/log.h"

#include "types.h"
#include "nfapi_interface.h"

#define RX_NB_TH_MAX 2
#define RX_NB_TH 2
#define RX_NB_TH_DL 14

#define LTE_SLOTS_PER_SUBFRAME 2

#define LTE_NUMBER_OF_SUBFRAMES_PER_FRAME 10
#define LTE_SLOTS_PER_FRAME  20
#define LTE_CE_FILTER_LENGTH 5
#define LTE_CE_OFFSET LTE_CE_FILTER_LENGTH
#define TX_RX_SWITCH_SYMBOL (NUMBER_OF_SYMBOLS_PER_FRAME>>1)
#define PBCH_PDU_SIZE 3 //bytes
#define NR_NUMBER_OF_SYMBOLS_PER_SLOT 14

#define PRACH_SYMBOL 3 //position of the UL PSS wrt 2nd slot of special subframe

#define NUMBER_OF_FREQUENCY_GROUPS (lte_frame_parms->N_RB_DL)

#define SSS_AMP 1148

#define MAX_NUM_PHICH_GROUPS 56  //110 RBs Ng=2, p.60 36-212, Sec. 6.9

#define MAX_MBSFN_AREA 8

#define NB_RX_ANTENNAS_MAX 64


typedef enum {TDD=1,FDD=0} frame_type_t;

typedef enum {EXTENDED=1,NORMAL=0} lte_prefix_type_t;

typedef enum {LOCALIZED=0,DISTRIBUTED=1} vrb_t;

/// Enumeration for parameter PHICH-Duration \ref PHICH_CONFIG_COMMON::phich_duration.
typedef enum {
  normal=0,
  extended=1
} PHICH_DURATION_t;

/// Enumeration for parameter Ng \ref PHICH_CONFIG_COMMON::phich_resource.
typedef enum {
  oneSixth=1,
  half=3,
  one=6,
  two=12
} PHICH_RESOURCE_t;
/// PHICH-Config from 36.331 RRC spec
typedef struct {
  /// Parameter: PHICH-Duration, see TS 36.211 (Table 6.9.3-1).
  PHICH_DURATION_t phich_duration;
  /// Parameter: Ng, see TS 36.211 (6.9). \details Value oneSixth corresponds to 1/6, half corresponds to 1/2 and so on.
  PHICH_RESOURCE_t phich_resource;
} PHICH_CONFIG_COMMON;

/// PRACH-ConfigInfo from 36.331 RRC spec
typedef struct {
  /// Parameter: prach-ConfigurationIndex, see TS 36.211 (5.7.1). \vr{[0..63]}
  uint8_t prach_ConfigIndex;
  /// Parameter: High-speed-flag, see TS 36.211 (5.7.2). \vr{[0..1]} 1 corresponds to Restricted set and 0 to Unrestricted set.
  uint8_t highSpeedFlag;
  /// Parameter: \f$N_\text{CS}\f$, see TS 36.211 (5.7.2). \vr{[0..15]}\n Refer to table 5.7.2-2 for preamble format 0..3 and to table 5.7.2-3 for preamble format 4.
  uint8_t zeroCorrelationZoneConfig;
  /// Parameter: prach-FrequencyOffset, see TS 36.211 (5.7.1). \vr{[0..94]}\n For TDD the value range is dependent on the value of \ref prach_ConfigIndex.
  uint8_t prach_FreqOffset;
} PRACH_CONFIG_INFO;


/// PRACH-ConfigSIB or PRACH-Config from 36.331 RRC spec
typedef struct {
  /// Parameter: RACH_ROOT_SEQUENCE, see TS 36.211 (5.7.1). \vr{[0..837]}
  uint16_t rootSequenceIndex;
  /// prach_Config_enabled=1 means enabled. \vr{[0..1]}
  uint8_t prach_Config_enabled;
  /// PRACH Configuration Information
  PRACH_CONFIG_INFO prach_ConfigInfo;
} PRACH_CONFIG_COMMON;

/// PRACH-eMTC-Config from 36.331 RRC spec
typedef struct {
  /// Parameter: High-speed-flag, see TS 36.211 (5.7.2). \vr{[0..1]} 1 corresponds to Restricted set and 0 to Unrestricted set.
  uint8_t highSpeedFlag;
  /// Parameter: \f$N_\text{CS}\f$, see TS 36.211 (5.7.2). \vr{[0..15]}\n Refer to table 5.7.2-2 for preamble format 0..3 and to table 5.7.2-3 for preamble format 4.
  uint8_t zeroCorrelationZoneConfig;
  /// Parameter: prach-FrequencyOffset, see TS 36.211 (5.7.1). \vr{[0..94]}\n For TDD the value range is dependent on the value of \ref prach_ConfigIndex.

  /// PRACH starting subframe periodicity, expressed in number of subframes available for preamble transmission (PRACH opportunities), see TS 36.211. Value 2 corresponds to 2 subframes, 4 corresponds to 4 subframes and so on. EUTRAN configures the PRACH starting subframe periodicity larger than or equal to the Number of PRACH repetitions per attempt for each CE level (numRepetitionPerPreambleAttempt).
  uint8_t prach_starting_subframe_periodicity[4];
  /// number of repetitions per preamble attempt per CE level
  uint8_t prach_numRepetitionPerPreambleAttempt[4];
  /// prach configuration index for each CE level
  uint8_t prach_ConfigIndex[4];
  /// indicator for CE level activation
  uint8_t prach_CElevel_enable[4];
  /// prach frequency offset for each CE level
  uint8_t prach_FreqOffset[4];
  /// indicator for CE level hopping activation
  uint8_t prach_hopping_enable[4];
  /// indicator for CE level hopping activation
  uint8_t prach_hopping_offset[4];
} PRACH_eMTC_CONFIG_INFO;

/// PRACH-ConfigSIB or PRACH-Config from 36.331 RRC spec
typedef struct {
  /// Parameter: RACH_ROOT_SEQUENCE, see TS 36.211 (5.7.1). \vr{[0..837]}
  uint16_t rootSequenceIndex;
  /// prach_Config_enabled=1 means enabled. \vr{[0..1]}
  uint8_t prach_Config_enabled;
  /// PRACH Configuration Information
  PRACH_eMTC_CONFIG_INFO prach_ConfigInfo;
} PRACH_eMTC_CONFIG_COMMON;


/// Enumeration for parameter \f$N_\text{ANRep}\f$ \ref PUCCH_CONFIG_DEDICATED::repetitionFactor.
typedef enum {
  n2=0,
  n4,
  n6
} ACKNAKREP_t;

/// Enumeration for \ref PUCCH_CONFIG_DEDICATED::tdd_AckNackFeedbackMode.
typedef enum {
  bundling=0,
  multiplexing
} ANFBmode_t;

/// PUCCH-ConfigDedicated from 36.331 RRC spec
typedef struct {
  /// Flag to indicate ACK NAK repetition activation, see TS 36.213 (10.1). \vr{[0..1]}
  uint8_t ackNackRepetition;
  /// Parameter: \f$N_\text{ANRep}\f$, see TS 36.213 (10.1).
  ACKNAKREP_t repetitionFactor;
  /// Parameter: \f$n^{(1)}_\text{PUCCH,ANRep}\f$, see TS 36.213 (10.1). \vr{[0..2047]}
  uint16_t n1PUCCH_AN_Rep;
  /// Feedback mode, see TS 36.213 (7.3). \details Applied to both PUCCH and PUSCH feedback. For TDD, should always be set to bundling.
  ANFBmode_t tdd_AckNackFeedbackMode;
} PUCCH_CONFIG_DEDICATED;

/// PUCCH-ConfigCommon from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$\Delta^\text{PUCCH}_\text{shift}\f$, see TS 36.211 (5.4.1). \vr{[1..3]} \note the specification sais it is an enumerated value.
  uint8_t deltaPUCCH_Shift;
  /// Parameter: \f$N^{(2)}_\text{RB}\f$, see TS 36.211 (5.4). \vr{[0..98]}
  uint8_t nRB_CQI;
  /// Parameter: \f$N^{(1)}_\text{CS}\f$, see TS 36.211 (5.4). \vr{[0..7]}
  uint8_t nCS_AN;
  /// Parameter: \f$N^{(1)}_\text{PUCCH}\f$ see TS 36.213 (10.1). \vr{[0..2047]}
  uint16_t n1PUCCH_AN;

  /// group hopping sequence for DRS \note not part of offical UL-PUCCH_CONFIG_COMMON ASN1 specification.
  uint8_t grouphop[20];
  /// sequence hopping sequence for DRS \note not part of offical UL-PUCCH_CONFIG_COMMON ASN1 specification.
  uint8_t seqhop[20];
} PUCCH_CONFIG_COMMON;

/// UL-ReferenceSignalsPUSCH from 36.331 RRC spec
typedef struct {
  /// Parameter: Group-hopping-enabled, see TS 36.211 (5.5.1.3). \vr{[0..1]}
  uint8_t groupHoppingEnabled;
  /// Parameter: \f$\Delta SS\f$, see TS 36.211 (5.5.1.3). \vr{[0..29]}
  uint8_t groupAssignmentPUSCH;
  /// Parameter: Sequence-hopping-enabled, see TS 36.211 (5.5.1.4). \vr{[0..1]}
  uint8_t sequenceHoppingEnabled;
  /// Parameter: cyclicShift, see TS 36.211 (Table 5.5.2.1.1-2). \vr{[0..7]}
  uint8_t cyclicShift;
  /// nPRS for cyclic shift of DRS \note not part of offical UL-ReferenceSignalsPUSCH ASN1 specification.
  uint8_t nPRS[20];
  /// group hopping sequence for DRS \note not part of offical UL-ReferenceSignalsPUSCH ASN1 specification.
  uint8_t grouphop[20];
  /// sequence hopping sequence for DRS \note not part of offical UL-ReferenceSignalsPUSCH ASN1 specification.
  uint8_t seqhop[20];
} UL_REFERENCE_SIGNALS_PUSCH_t;

/// Enumeration for parameter Hopping-mode \ref PUSCH_CONFIG_COMMON::hoppingMode.
typedef enum {
  interSubFrame=0,
  intraAndInterSubFrame=1
} PUSCH_HOPPING_t;

/// PUSCH-ConfigCommon from 36.331 RRC spec.
typedef struct {
  /// Parameter: \f$N_{sb}\f$, see TS 36.211 (5.3.4). \vr{[1..4]}
  uint8_t n_SB;
  /// Parameter: Hopping-mode, see TS 36.211 (5.3.4).
  PUSCH_HOPPING_t hoppingMode;
  /// Parameter: \f$N^{HO}_{RB}\f$, see TS 36.211 (5.3.4). \vr{[0..98]}
  uint8_t pusch_HoppingOffset;
  /// See TS 36.213 (8.6.1). \vr{[0..1]} 1 indicates 64QAM is allowed, 0 not allowed.
  uint8_t enable64QAM;
  /// Ref signals configuration
  UL_REFERENCE_SIGNALS_PUSCH_t ul_ReferenceSignalsPUSCH;
} PUSCH_CONFIG_COMMON;

/// UE specific PUSCH configuration.
typedef struct {
  /// Parameter: \f$I^\text{HARQ-ACK}_\text{offset}\f$, see TS 36.213 (Table 8.6.3-1). \vr{[0..15]}
  uint16_t betaOffset_ACK_Index;
  /// Parameter: \f$I^{RI}_\text{offset}\f$, see TS 36.213 (Table 8.6.3-2). \vr{[0..15]}
  uint16_t betaOffset_RI_Index;
  /// Parameter: \f$I^{CQI}_\text{offset}\f$, see TS 36.213 (Table 8.6.3-3). \vr{[0..15]}
  uint16_t betaOffset_CQI_Index;
} PUSCH_CONFIG_DEDICATED;

/// lola CBA information
typedef struct {
  ///
  uint16_t betaOffset_CA_Index;
  ///
  uint16_t cShift;
} PUSCH_CA_CONFIG_DEDICATED;

/// PDSCH-ConfigCommon from 36.331 RRC spec
typedef struct {
  /// Parameter: Reference-signal power, see TS 36.213 (5.2). \vr{[-60..50]}\n Provides the downlink reference-signal EPRE. The actual value in dBm.
  int8_t referenceSignalPower;
  /// Parameter: \f$P_B\f$, see TS 36.213 (Table 5.2-1). \vr{[0..3]}
  uint8_t p_b;
} PDSCH_CONFIG_COMMON;

/// Enumeration for Parameter \f$P_A\f$ \ref PDSCH_CONFIG_DEDICATED::p_a.
typedef enum {
  dBm6=0, ///< (dB-6) corresponds to -6 dB
  dBm477, ///< (dB-4dot77) corresponds to -4.77 dB
  dBm3,   ///< (dB-3) corresponds to -3 dB
  dBm177, ///< (dB-1dot77) corresponds to -1.77 dB
  dB0,    ///< corresponds to 0 dB
  dB1,    ///< corresponds to 1 dB
  dB2,    ///< corresponds to 2 dB
  dB3     ///< corresponds to 3 dB
} PA_t;

/// PDSCH-ConfigDedicated from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$P_A\f$, see TS 36.213 (5.2).
  PA_t p_a;
} PDSCH_CONFIG_DEDICATED;

/// SoundingRS-UL-ConfigCommon Information Element from 36.331 RRC spec
typedef struct {
  /// enabled flag=1 means SRS is enabled. \vr{[0..1]}
  uint8_t enabled_flag;
  /// Parameter: SRS Bandwidth Configuration, see TS 36.211 (table 5.5.3.2-1, 5.5.3.2-2, 5.5.3.2-3 and 5.5.3.2-4). \vr{[0..7]}\n Actual configuration depends on UL bandwidth. \note the specification sais it is an enumerated value.
  uint8_t srs_BandwidthConfig;
  /// Parameter: SRS SubframeConfiguration, see TS 36.211 (table 5.5.3.3-1 for FDD, table 5.5.3.3-2 for TDD). \vr{[0..15]} \note the specification sais it is an enumerated value.
  uint8_t srs_SubframeConfig;
  /// Parameter: Simultaneous-AN-and-SRS, see TS 36.213 (8.2). \vr{[0..1]}
  uint8_t ackNackSRS_SimultaneousTransmission;
  /// Parameter: srsMaxUpPts, see TS 36.211 (5.5.3.2). \details If this field is present, reconfiguration of \f$m^\text{max}_\text{SRS,0}\f$ applies for UpPts, otherwise reconfiguration does not apply.
  uint8_t srs_MaxUpPts;
} SOUNDINGRS_UL_CONFIG_COMMON;

/// \note UNUSED
typedef enum {
  ulpc_al0=0,
  ulpc_al04=1,
  ulpc_al05=2,
  ulpc_al06=3,
  ulpc_al07=4,
  ulpc_al08=5,
  ulpc_al09=6,
  ulpc_al11=7
} UL_POWER_CONTROL_COMMON_alpha_t;

/// Enumeration for \ref deltaFList_PUCCH_t::deltaF_PUCCH_Format1.
typedef enum {
  deltaF_PUCCH_Format1_deltaF_2  = 0,
  deltaF_PUCCH_Format1_deltaF0   = 1,
  deltaF_PUCCH_Format1_deltaF2   = 2
} deltaF_PUCCH_Format1_t;

/// Enumeration for \ref deltaFList_PUCCH_t::deltaF_PUCCH_Format1b.
typedef enum {
  deltaF_PUCCH_Format1b_deltaF1  = 0,
  deltaF_PUCCH_Format1b_deltaF3  = 1,
  deltaF_PUCCH_Format1b_deltaF5  = 2
} deltaF_PUCCH_Format1b_t;

/// Enumeration for \ref deltaFList_PUCCH_t::deltaF_PUCCH_Format2.
typedef enum {
  deltaF_PUCCH_Format2_deltaF_2  = 0,
  deltaF_PUCCH_Format2_deltaF0   = 1,
  deltaF_PUCCH_Format2_deltaF1   = 2,
  deltaF_PUCCH_Format2_deltaF2   = 3
} deltaF_PUCCH_Format2_t;

/// Enumeration for \ref deltaFList_PUCCH_t::deltaF_PUCCH_Format2a.
typedef enum {
  deltaF_PUCCH_Format2a_deltaF_2 = 0,
  deltaF_PUCCH_Format2a_deltaF0  = 1,
  deltaF_PUCCH_Format2a_deltaF2  = 2
} deltaF_PUCCH_Format2a_t;

/// Enumeration for \ref deltaFList_PUCCH_t::deltaF_PUCCH_Format2b.
typedef enum {
  deltaF_PUCCH_Format2b_deltaF_2 = 0,
  deltaF_PUCCH_Format2b_deltaF0  = 1,
  deltaF_PUCCH_Format2b_deltaF2  = 2
} deltaF_PUCCH_Format2b_t;

/// DeltaFList-PUCCH from 36.331 RRC spec
typedef struct {
  deltaF_PUCCH_Format1_t   deltaF_PUCCH_Format1;
  deltaF_PUCCH_Format1b_t  deltaF_PUCCH_Format1b;
  deltaF_PUCCH_Format2_t   deltaF_PUCCH_Format2;
  deltaF_PUCCH_Format2a_t  deltaF_PUCCH_Format2a;
  deltaF_PUCCH_Format2b_t  deltaF_PUCCH_Format2b;
} deltaFList_PUCCH_t;

/// SoundingRS-UL-ConfigDedicated Information Element from 36.331 RRC spec
typedef struct {
  /// This descriptor is active
  uint8_t active;
  /// This descriptor's frame
  uint16_t frame;
  /// This descriptor's subframe
  uint8_t  subframe;
  /// rnti
  uint16_t rnti;
  /// Parameter: \f$B_\text{SRS}\f$, see TS 36.211 (table 5.5.3.2-1, 5.5.3.2-2, 5.5.3.2-3 and 5.5.3.2-4). \vr{[0..3]} \note the specification sais it is an enumerated value.
  uint8_t srs_Bandwidth;
  /// Parameter: SRS hopping bandwidth \f$b_\text{hop}\in\{0,1,2,3\}\f$, see TS 36.211 (5.5.3.2) \vr{[0..3]} \note the specification sais it is an enumerated value.
  uint8_t srs_HoppingBandwidth;
  /// Parameter: \f$n_\text{RRC}\f$, see TS 36.211 (5.5.3.2). \vr{[0..23]}
  uint8_t freqDomainPosition;
  /// Parameter: Duration, see TS 36.213 (8.2). \vr{[0..1]} 0 corresponds to "single" and 1 to "indefinite".
  uint8_t duration;
  /// Parameter: \f$k_\text{TC}\in\{0,1\}\f$, see TS 36.211 (5.5.3.2). \vr{[0..1]}
  uint8_t transmissionComb;
  /// Parameter: \f$I_\text{SRS}\f$, see TS 36.213 (table 8.2-1). \vr{[0..1023]}
  uint16_t srs_ConfigIndex;
  /// Parameter: \f$n^\text{CS}_\text{SRS}\f$. See TS 36.211 (5.5.3.1). \vr{[0..7]} \note the specification sais it is an enumerated value.
  uint8_t cyclicShift;
  // Parameter: internal implementation: UE SRS configured
  uint8_t srsConfigDedicatedSetup;
  // Parameter: cell srs subframe for internal implementation
  uint8_t srsCellSubframe;
  // Parameter: ue srs subframe for internal implementation
  uint8_t srsUeSubframe;
} SOUNDINGRS_UL_CONFIG_DEDICATED;

/// UplinkPowerControlDedicated Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$P_\text{0\_UE\_PUSCH}(1)\f$, see TS 36.213 (5.1.1.1), unit dB. \vr{[-8..7]}\n This field is applicable for non-persistent scheduling, only.
  int8_t p0_UE_PUSCH;
  /// Parameter: Ks, see TS 36.213 (5.1.1.1). \vr{[0..1]}\n en0 corresponds to value 0 corresponding to state “disabled”. en1 corresponds to value 1.25 corresponding to “enabled”. \note the specification sais it is an enumerated value. \warning the enumeration values do not correspond to the given values in the specification (en1 should be 1.25).
  uint8_t deltaMCS_Enabled;
  /// Parameter: Accumulation-enabled, see TS 36.213 (5.1.1.1). \vr{[0..1]} 1 corresponds to "enabled" whereas 0 corresponds to "disabled".
  uint8_t accumulationEnabled;
  /// Parameter: \f$P_\text{0\_UE\_PUCCH}(1)\f$, see TS 36.213 (5.1.2.1), unit dB. \vr{[-8..7]}
  int8_t p0_UE_PUCCH;
  /// Parameter: \f$P_\text{SRS\_OFFSET}\f$, see TS 36.213 (5.1.3.1). \vr{[0..15]}\n For Ks=1.25 (\ref deltaMCS_Enabled), the actual parameter value is pSRS_Offset value - 3. For Ks=0, the actual parameter value is -10.5 + 1.5*pSRS_Offset value.
  int8_t pSRS_Offset;
  /// Specifies the filtering coefficient for RSRP measurements used to calculate path loss, as specified in TS 36.213 (5.1.1.1).\details The same filtering mechanism applies as for quantityConfig described in 5.5.3.2. \note the specification sais it is an enumerated value.
  uint8_t filterCoefficient;
} UL_POWER_CONTROL_DEDICATED;

/// Enumeration for parameter \f$\alpha\f$ \ref UL_POWER_CONTROL_CONFIG_COMMON::alpha.
typedef enum {
  al0=0,
  al04=1,
  al05=2,
  al06=3,
  al07=4,
  al08=5,
  al09=6,
  al1=7
} PUSCH_alpha_t;

/// \note UNUSED
typedef enum {
  deltaFm2=0,
  deltaF0,
  deltaF1,
  deltaF2,
  deltaF3,
  deltaF5
} deltaF_PUCCH_t;

/// UplinkPowerControlCommon Information Element from 36.331 RRC spec \note this structure does not currently make use of \ref deltaFList_PUCCH_t.
typedef struct {
  /// Parameter: \f$P_\text{0\_NOMINAL\_PUSCH}(1)\f$, see TS 36.213 (5.1.1.1), unit dBm. \vr{[-126..24]}\n This field is applicable for non-persistent scheduling, only.
  int8_t p0_NominalPUSCH;
  /// Parameter: \f$\alpha\f$, see TS 36.213 (5.1.1.1) \warning the enumeration values do not correspond to the given values in the specification (al04 should be 0.4, ...)!
  PUSCH_alpha_t alpha;
  /// Parameter: \f$P_\text{0\_NOMINAL\_PUCCH}\f$ See TS 36.213 (5.1.2.1), unit dBm. \vr{[-127..-96]}
  int8_t p0_NominalPUCCH;
  /// Parameter: \f$\Delta_\text{PREAMBLE\_Msg3}\f$ see TS 36.213 (5.1.1.1). \vr{[-1..6]}\n Actual value = IE value * 2 [dB].
  int8_t deltaPreambleMsg3;
  /// Parameter: \f$\Delta_\text{F\_PUCCH}(F)\f$ for the PUCCH format 1, see TS 36.213 (5.1.2). \vr{[0..2]} \warning check value range, why is this a long? \note the specification sais it is an enumerated value.
  long deltaF_PUCCH_Format1;
  /// Parameter: \f$\Delta_\text{F\_PUCCH}(F)\f$ for the PUCCH format 1a, see TS 36.213 (5.1.2). \vr{[0..2]} \warning check value range, why is this a long? \note the specification sais it is an enumerated value.
  long deltaF_PUCCH_Format1a;
  /// Parameter: \f$\Delta_\text{F\_PUCCH}(F)\f$ for the PUCCH format 1b, see TS 36.213 (5.1.2). \vr{[0..2]} \warning check value range, why is this a long? \note the specification sais it is an enumerated value.
  long deltaF_PUCCH_Format1b;
  /// Parameter: \f$\Delta_\text{F\_PUCCH}(F)\f$ for the PUCCH format 2, see TS 36.213 (5.1.2). \vr{[0..3]} \warning check value range, why is this a long? \note the specification sais it is an enumerated value.
  long deltaF_PUCCH_Format2;
  /// Parameter: \f$\Delta_\text{F\_PUCCH}(F)\f$ for the PUCCH format 2a, see TS 36.213 (5.1.2). \vr{[0..2]} \warning check value range, why is this a long? \note the specification sais it is an enumerated value.
  long deltaF_PUCCH_Format2a;
  /// Parameter: \f$\Delta_\text{F\_PUCCH}(F)\f$ for the PUCCH format 2b, see TS 36.213 (5.1.2). \vr{[0..2]} \warning check value range, why is this a long? \note the specification sais it is an enumerated value.
  long deltaF_PUCCH_Format2b;
} UL_POWER_CONTROL_CONFIG_COMMON;

/// Union for \ref TPC_PDCCH_CONFIG::tpc_Index.
typedef union {
  /// Index of N when DCI format 3 is used. See TS 36.212 (5.3.3.1.6). \vr{[1..15]}
  uint8_t indexOfFormat3;
  /// Index of M when DCI format 3A is used. See TS 36.212 (5.3.3.1.7). \vr{[1..31]}
  uint8_t indexOfFormat3A;
} TPC_INDEX_t;

/// TPC-PDCCH-Config Information Element from 36.331 RRC spec
typedef struct {
  /// RNTI for power control using DCI format 3/3A, see TS 36.212. \vr{[0..65535]}
  uint16_t rnti;
  /// Index of N or M, see TS 36.212 (5.3.3.1.6 and 5.3.3.1.7), where N or M is dependent on the used DCI format (i.e. format 3 or 3a).
  TPC_INDEX_t tpc_Index;
} TPC_PDCCH_CONFIG;

/// Enumeration for parameter SR transmission \ref SCHEDULING_REQUEST_CONFIG::dsr_TransMax.
typedef enum {
  sr_n4=0,
  sr_n8=1,
  sr_n16=2,
  sr_n32=3,
  sr_n64=4
} DSR_TRANSMAX_t;

/// SchedulingRequestConfig Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: \f$n^{(1)}_\text{PUCCH,SRI}\f$, see TS 36.213 (10.1). \vr{[0..2047]}
  uint16_t sr_PUCCH_ResourceIndex;
  /// Parameter: \f$I_\text{SR}\f$, see TS 36.213 (10.1). \vr{[0..155]}
  uint8_t sr_ConfigIndex;
  /// Parameter for SR transmission in TS 36.321 (5.4.4). \details The value n4 corresponds to 4 transmissions, n8 corresponds to 8 transmissions and so on.
  DSR_TRANSMAX_t dsr_TransMax;
} SCHEDULING_REQUEST_CONFIG;

/// CQI-ReportPeriodic
typedef struct {
  /// Parameter: \f$n^{(2)}_\text{PUCCH}\f$, see TS 36.213 (7.2). \vr{[0..1185]}, -1 indicates inactivity
  int16_t cqi_PUCCH_ResourceIndex;
  /// Parameter: CQI/PMI Periodicity and Offset Configuration Index \f$I_\text{CQI/PMI}\f$, see TS 36.213 (tables 7.2.2-1A and 7.2.2-1C). \vr{[0..1023]}
  int16_t cqi_PMI_ConfigIndex;
  /// Parameter: K, see 36.213 (4.2.2). \vr{[1..4]}
  uint8_t K;
  /// Parameter: RI Config Index \f$I_\text{RI}\f$, see TS 36.213 (7.2.2-1B). \vr{[0..1023]}, -1 indicates inactivity
  int16_t ri_ConfigIndex;
  /// Parameter: Simultaneous-AN-and-CQI, see TS 36.213 (10.1). \vr{[0..1]} 1 indicates that simultaneous transmission of ACK/NACK and CQI is allowed.
  uint8_t simultaneousAckNackAndCQI;
  /// parameter computed from Tables 7.2.2-1A and 7.2.2-1C
  uint16_t Npd;
  /// parameter computed from Tables 7.2.2-1A and 7.2.2-1C
  uint16_t N_OFFSET_CQI;
} CQI_REPORTPERIODIC;

/// Enumeration for parameter reporting mode \ref CQI_REPORT_CONFIG::cqi_ReportModeAperiodic.
typedef enum {
  rm12=0,
  rm20=1,
  rm22=2,
  rm30=3,
  rm31=4
} CQI_REPORTMODEAPERIODIC;

/// CQI-ReportConfig Information Element from 36.331 RRC spec
typedef struct {
  /// Parameter: reporting mode. Value rm12 corresponds to Mode 1-2, rm20 corresponds to Mode 2-0, rm22 corresponds to Mode 2-2 etc. PUSCH reporting modes are described in TS 36.213 [23, 7.2.1].
  CQI_REPORTMODEAPERIODIC cqi_ReportModeAperiodic;
  /// Parameter: \f$\Delta_\text{offset}\f$, see TS 36.213 (7.2.3). \vr{[-1..6]}\n Actual value = IE value * 2 [dB].
  int8_t nomPDSCH_RS_EPRE_Offset;
  CQI_REPORTPERIODIC CQI_ReportPeriodic;
} CQI_REPORT_CONFIG;

/// MBSFN-SubframeConfig Information Element from 36.331 RRC spec \note deviates from specification.
typedef struct {
  /// MBSFN subframe occurance. \details Radio-frames that contain MBSFN subframes occur when equation SFN mod radioFrameAllocationPeriod = radioFrameAllocationOffset is satisfied. When fourFrames is used for subframeAllocation, the equation defines the first radio frame referred to in the description below. Values n1 and n2 are not applicable when fourFrames is used. \note the specification sais it is an enumerated value {n1, n2, n4, n8, n16, n32}.
  int radioframeAllocationPeriod;
  /// MBSFN subframe occurance. \vr{[0..7]}\n Radio-frames that contain MBSFN subframes occur when equation SFN mod radioFrameAllocationPeriod = radioFrameAllocationOffset is satisfied. When fourFrames is used for subframeAllocation, the equation defines the first radio frame referred to in the description below. Values n1 and n2 are not applicable when fourFrames is used.
  int radioframeAllocationOffset;
  /// oneFrame or fourFrames. \vr{[0..1]}
  int fourFrames_flag;
  /// Subframe configuration. \vr{[0..63]} (\ref fourFrames_flag == 0) or \vr{[0..16777215]} (\ref fourFrames_flag == 1)
  /// \par fourFrames_flag == 0
  /// "1" denotes that the corresponding subframe is allocated for MBSFN. The following mapping applies:\n FDD: The first/leftmost bit defines the MBSFN allocation for subframe #1, the second bit for #2, third bit for #3 , fourth bit for #6, fifth bit for #7, sixth bit for #8.\n TDD: The first/leftmost bit defines the allocation for subframe #3, the second bit for #4, third bit for #7, fourth bit for #8, fifth bit for #9. Uplink subframes are not allocated. The last bit is not used.
  /// \par fourFrames_flag == 1
  /// A bit-map indicating MBSFN subframe allocation in four consecutive radio frames, "1" denotes that the corresponding subframe is allocated for MBSFN. The bitmap is interpreted as follows:\n FDD: Starting from the first radioframe and from the first/leftmost bit in the bitmap, the allocation applies to subframes #1, #2, #3 , #6, #7, and #8 in the sequence of the four radio-frames.\n TDD: Starting from the first radioframe and from the first/leftmost bit in the bitmap, the allocation applies to subframes #3, #4, #7, #8, and #9 in the sequence of the four radio-frames. The last four bits are not used. Uplink subframes are not allocated.
  int mbsfn_SubframeConfig;
} MBSFN_config_t;

typedef struct {
  int radioframeAllocationPeriod;
  int radioframeAllocationOffset;
  int non_mbsfn_SubframeConfig;
} NonMBSFN_config_t;


typedef struct LTE_DL_FRAME_PARMS {
  /// Number of resource blocks (RB) in DL
  uint8_t N_RB_DL;
  /// Number of resource blocks (RB) in UL
  uint8_t N_RB_UL;
  /// EUTRA Band
  uint8_t eutra_band;
  /// DL carrier frequency
  uint32_t dl_CarrierFreq;
  /// UL carrier frequency
  uint32_t ul_CarrierFreq;
  /// TX attenuation
  uint32_t att_tx;
  /// RX attenuation
  uint32_t att_rx;
  ///  total Number of Resource Block Groups: this is ceil(N_PRB/P)
  uint8_t N_RBG;
  /// Total Number of Resource Block Groups SubSets: this is P
  uint8_t N_RBGS;
  /// Cell ID
  uint16_t Nid_cell;
  /// MBSFN Area ID
  uint16_t Nid_cell_mbsfn;
  /// Cyclic Prefix for DL (0=Normal CP, 1=Extended CP)
  lte_prefix_type_t Ncp;
  /// Cyclic Prefix for UL (0=Normal CP, 1=Extended CP)
  lte_prefix_type_t Ncp_UL;
  /// shift of pilot position in one RB
  uint8_t nushift;
  /// Frame type (0 FDD, 1 TDD)
  frame_type_t frame_type;
  /// TDD subframe assignment (0-7) (default = 3) (254=RX only, 255=TX only)
  uint8_t tdd_config;
  /// TDD S-subframe configuration (0-9)
  uint8_t tdd_config_S;
  /// srs extra symbol flag for TDD
  uint8_t srsX;
  /// indicates if node is a UE (NODE=2) or eNB (PRIMARY_CH=0).
  uint8_t node_id;
  /// Indicator that 20 MHz channel uses 3/4 sampling frequency
  uint8_t threequarter_fs;
  /// Size of FFT
  uint16_t ofdm_symbol_size;
  uint8_t FeMBMS_active;
  /// Size of FFT
  uint16_t ofdm_symbol_size_khz_1dot25;
  /// Number of prefix samples in all but first symbol of slot
  uint16_t nb_prefix_samples;
  /// Number of prefix samples in first symbol of slot
  uint16_t nb_prefix_samples0;
  /// Number of prefix samples in all but first symbol of slot
  uint16_t nb_prefix_samples_khz_1dot25;
  /// Number of prefix samples in first symbol of slot
  uint16_t nb_prefix_samples0_khz_1dot25;
  /// Carrier offset in FFT buffer for first RE in PRB0
  uint16_t first_carrier_offset;
  /// Carrier offset in FFT buffer for first RE in PRB0 (FeMBMS
  uint16_t first_carrier_offset_khz_1dot25;
  /// Number of samples in a subframe
  uint32_t samples_per_tti;
  /// Number of samples in a subframe
  uint32_t samples_per_subframe;
  /// Number of samples in a slot
  uint32_t samples_per_slot;
  /// Number of OFDM/SC-FDMA symbols in one subframe (to be modified to account for potential different in UL/DL)
  uint16_t symbols_per_tti;
  /// Number of OFDM symbols in DL portion of S-subframe
  uint16_t dl_symbols_in_S_subframe;
  /// Number of SC-FDMA symbols in UL portion of S-subframe
  uint16_t ul_symbols_in_S_subframe;
  /// Number of Physical transmit antennas in node
  uint8_t nb_antennas_tx;
  /// Number of Receive antennas in node
  uint8_t nb_antennas_rx;
  /// Number of common transmit antenna ports in eNodeB (1 or 2)
  uint8_t nb_antenna_ports_eNB;
  /// PRACH_CONFIG
  PRACH_CONFIG_COMMON prach_config_common;
  /// PRACH_eMTC_CONFIG
  PRACH_eMTC_CONFIG_COMMON prach_emtc_config_common;
  /// PUCCH Config Common (from 36-331 RRC spec)
  PUCCH_CONFIG_COMMON pucch_config_common;
  /// PDSCH Config Common (from 36-331 RRC spec)
  PDSCH_CONFIG_COMMON pdsch_config_common;
  /// PUSCH Config Common (from 36-331 RRC spec)
  PUSCH_CONFIG_COMMON pusch_config_common;
  /// PHICH Config (from 36-331 RRC spec)
  PHICH_CONFIG_COMMON phich_config_common;
  /// SRS Config (from 36-331 RRC spec)
  SOUNDINGRS_UL_CONFIG_COMMON soundingrs_ul_config_common;
  /// UL Power Control (from 36-331 RRC spec)
  UL_POWER_CONTROL_CONFIG_COMMON ul_power_control_config_common;
  /// Number of MBSFN Configurations
  int num_MBSFN_config;
  /// Array of MBSFN Configurations (max 8 (maxMBSFN-Allocations) elements as per 36.331)
  MBSFN_config_t MBSFN_config[8];
  uint8_t NonMBSFN_config_flag;
  NonMBSFN_config_t NonMBSFN_config;
  /// Maximum Number of Retransmissions of RRCConnectionRequest (from 36-331 RRC Spec)
  uint8_t maxHARQ_Msg3Tx;
  /// Size of SI windows used for repetition of one SI message (in frames)
  uint8_t SIwindowsize;
  /// Period of SI windows used for repetition of one SI message (in frames)
  uint16_t SIPeriod;
  /// REGs assigned to PCFICH
  uint16_t pcfich_reg[4];
  /// Index of first REG assigned to PCFICH
  uint8_t pcfich_first_reg_idx;
  /// REGs assigned to PHICH
  uint16_t phich_reg[MAX_NUM_PHICH_GROUPS][3];

  struct MBSFN_SubframeConfig *mbsfn_SubframeConfig[MAX_MBSFN_AREA];
  struct NonMBSFN_SubframeConfig *non_mbsfn_SubframeConfig;
  /// for fair RR scheduler
  uint32_t ue_multiple_max;
} LTE_DL_FRAME_PARMS;

typedef enum {
  /// TM1
  SISO=0,
  /// TM2
  ALAMOUTI=1,
  /// TM3
  LARGE_CDD=2,
  /// the next 6 entries are for TM5
  UNIFORM_PRECODING11=3,
  UNIFORM_PRECODING1m1=4,
  UNIFORM_PRECODING1j=5,
  UNIFORM_PRECODING1mj=6,
  PUSCH_PRECODING0=7,
  PUSCH_PRECODING1=8,
  /// the next 3 entries are for TM4
  DUALSTREAM_UNIFORM_PRECODING1=9,
  DUALSTREAM_UNIFORM_PRECODINGj=10,
  DUALSTREAM_PUSCH_PRECODING=11,
  TM7=12,
  TM8=13,
  TM9_10=14
} MIMO_mode_t;

typedef enum {
  NR_SISO=0,
  NR_DUALSTREAM=1
} MIMO_nrmode_t;


typedef enum {
  /// MRT
  MRT=0,
  /// ZF
  ZF=1,
  /// MMSE
  MMSE=2
} PRECODE_TYPE_t;

typedef enum {
  format0,
  format1,
  format1A,
  format1B,
  format1C,
  format1D,
  format1E_2A_M10PRB,
  format2,
  format2A,
  format2B,
  format2C,
  format2D,
  format3,
  format3A,
  format4,
  format5,
  format6_0A,
  format6_0B,
  format6_1A,
  format6_1B,
  format6_2
} DCI_format_t;

typedef struct {
  /// Length of DCI in bits
  uint8_t dci_length;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  bool ra_flag;
  /// rnti
  rnti_t rnti;
  /// harq_pid
  rnti_t harq_pid;
  /// Format
  DCI_format_t format;
  /// DCI pdu
  uint8_t dci_pdu[8];
} DCI_ALLOC_t;

#define MAX_EPDCCH_PRB 8

typedef struct {
  /// Length of DCI in bits
  uint8_t dci_length;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  bool ra_flag;
  /// rnti
  rnti_t rnti;
  /// Format
  DCI_format_t format;
  /// epdcch resource assignment (0=localized,1=distributed)
  uint8_t epdcch_resource_assignment_flag;
  /// epdcch index
  uint16_t epdcch_id;
  /// epdcch start symbol
  uint8_t epdcch_start_symbol;
  /// epdcch number of PRBs in set
  uint8_t epdcch_num_prb;
  /// vector of prb ids for set
  uint8_t epdcch_prb_index[MAX_EPDCCH_PRB];
  /// LBT parameter for frame configuration
  uint8_t dwpts_symbols;
  /// LBT parameter for frame configuration
  uint8_t initial_lbt_sf;
  /// DCI pdu
  uint8_t dci_pdu[8];
} eDCI_ALLOC_t;

typedef struct {
  /// Length of DCI in bits
  uint8_t dci_length;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  bool ra_flag;
  /// rnti
  rnti_t rnti;
  /// Format
  DCI_format_t format;
  /// harq process index
  uint8_t harq_pid;
  /// Narrowband index
  uint8_t narrowband;
  /// number of mdpcch repetitions
  uint16_t reps;
  /// number of PRB pairs for MPDCCH
  uint8_t number_of_prb_pairs;
  /// mpdcch resource assignment (combinatorial index r)
  uint8_t resource_block_assignment;
  /// transmission type (0=localized,1=distributed)
  uint8_t transmission_type;
  /// mpdcch start symbol
  uint8_t start_symbol;
  /// CE mode (1=ModeA,2=ModeB)
  uint8_t ce_mode;
  /// 0-503 n_EPDCCHid_i
  uint16_t dmrs_scrambling_init;
  /// Absolute subframe of the initial transmission (0-10239)
  uint16_t i0;
  /// current absolute subframe number
  uint16_t absSF;
  /// DCI pdu
  uint8_t dci_pdu[8];
} mDCI_ALLOC_t;

typedef struct {
  /// Preamble index for PRACH (0-63)
  uint8_t ra_PreambleIndex;
  /// RACH MaskIndex
  uint8_t ra_RACH_MaskIndex;
  /// Target received power at eNB (-120 ... -82 dBm)
  int8_t ra_PREAMBLE_RECEIVED_TARGET_POWER;
  /// PRACH index for TDD (0 ... 6) depending on TDD configuration and prachConfigIndex
  uint8_t ra_TDD_map_index;
  /// Corresponding RA-RNTI for UL-grant
  uint16_t ra_RNTI;
  /// Pointer to Msg3 payload for UL-grant
  uint8_t *Msg3;
} PRACH_RESOURCES_t;


typedef struct {
  /// Downlink Power offset field
  uint8_t dl_pow_off;
  ///Subband resource allocation field
  uint8_t rballoc_sub[50];
  ///Total number of PRBs indicator
  uint8_t pre_nb_available_rbs;
} MU_MIMO_mode;

typedef enum {
  NOT_SYNCHED=0,
  PRACH=1,
  RA_RESPONSE=2,
  RA_WAIT_CR=3,
  PUSCH=4,
  RESYNCH=5,
  NUM_UE_MODE=6
} UE_MODE_t;

#define FOREACH_PARALLEL(GEN)   \
  GEN(PARALLEL_SINGLE_THREAD)   \
  GEN(PARALLEL_RU_L1_SPLIT)     \
  GEN(PARALLEL_RU_L1_TRX_SPLIT)

#define GENERATE_ENUM(N) N,
#define GENERATE_ENUMTXT(N) {(char*)#N, N},

typedef enum {
  FOREACH_PARALLEL(GENERATE_ENUM)
} PARALLEL_CONF_t;

#define FOREACH_WORKER(GEN) GEN(WORKER_DISABLE) GEN(WORKER_ENABLE)
typedef enum {
  FOREACH_WORKER(GENERATE_ENUM)
} WORKER_CONF_t;

typedef struct THREAD_STRUCT_s {
  PARALLEL_CONF_t  parallel_conf;
  WORKER_CONF_t    worker_conf;
} THREAD_STRUCT;

typedef enum {SF_DL, SF_UL, SF_S} lte_subframe_t;

#define NUMBER_OF_SUBBANDS_MAX 13
#define NUMBER_OF_HARQ_PID_MAX 8

#define MAX_FRAME_NUMBER 0x400


#define NUMBER_OF_RN_MAX 3
typedef enum {no_relay=1,unicast_relay_type1,unicast_relay_type2, multicast_relay} relaying_type_t;



#define MCS_COUNT 29
#define MCS_TABLE_LENGTH_MAX 64


#define NUM_DCI_MAX 32

#define NUMBER_OF_eNB_SECTORS_MAX 3

#define NB_BANDS_MAX 8

#include "common/utils/LOG/log_extern.h"
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;


#define MODE_DECODE_NONE         0
#define MODE_DECODE_SSE          1
#define MODE_DECODE_C            2
#define MODE_DECODE_AVX2         3

#define DECODE_INITTD8_SSE_FPTRIDX   0
#define DECODE_INITTD16_SSE_FPTRIDX  1
#define DECODE_INITTD_AVX2_FPTRIDX   2
#define DECODE_TD8_SSE_FPTRIDX       3
#define DECODE_TD16_SSE_FPTRIDX      4
#define DECODE_TD_C_FPTRIDX          5
#define DECODE_TD16_AVX2_FPTRIDX     6
#define DECODE_FREETD8_FPTRIDX       7
#define DECODE_FREETD16_FPTRIDX      8
#define DECODE_FREETD_AVX2_FPTRIDX   9
#define ENCODE_SSE_FPTRIDX           10
#define ENCODE_C_FPTRIDX             11
#define ENCODE_INIT_SSE_FPTRIDX      12
#define DECODE_NUM_FPTR              13


// Mask for identifying subframe for MBMS
#define MBSFN_TDD_SF3 0x80// for TDD
#define MBSFN_TDD_SF4 0x40
#define MBSFN_TDD_SF7 0x20
#define MBSFN_TDD_SF8 0x10
#define MBSFN_TDD_SF9 0x08



#define MBSFN_FDD_SF1 0x80// for FDD
#define MBSFN_FDD_SF2 0x40
#define MBSFN_FDD_SF3 0x20
#define MBSFN_FDD_SF6 0x10
#define MBSFN_FDD_SF7 0x08
#define MBSFN_FDD_SF8 0x04


typedef uint8_t(decoder_if_t)(int16_t *y,
                              int16_t *y2,
                              uint8_t *decoded_bytes,
                              uint8_t *decoded_bytes2,
                              uint16_t n,
                              uint8_t max_iterations,
                              uint8_t crc_type,
                              uint8_t F,
                              time_stats_t *init_stats,
                              time_stats_t *alpha_stats,
                              time_stats_t *beta_stats,
                              time_stats_t *gamma_stats,
                              time_stats_t *ext_stats,
                              time_stats_t *intl1_stats,
                              time_stats_t *intl2_stats);

typedef uint8_t(encoder_if_t)(uint8_t *input,
                              uint16_t input_length_bytes,
                              uint8_t *output,
                              uint8_t F);

extern int oai_exit;

static inline void wait_sync(char *thread_name) {
  int rc;
  printf( "waiting for sync (%s,%d/%p,%p,%p)\n",thread_name,sync_var,&sync_var,&sync_cond,&sync_mutex);
  AssertFatal((rc = pthread_mutex_lock( &sync_mutex ))==0,"sync mutex lock error");

  while (sync_var<0 && !oai_exit)
    pthread_cond_wait( &sync_cond, &sync_mutex );

  AssertFatal((rc = pthread_mutex_unlock( &sync_mutex ))==0,"sync mutex unlock error");
  printf( "got sync (%s)\n", thread_name);
  /*
   * Raphael Defosseux: added for CI to get faster the got sync message.
   */
  fflush(stdout);
  fflush(stderr);
}


static inline int wakeup_thread(pthread_mutex_t *mutex,
                                pthread_cond_t *cond,
                                int *instance_cnt,
                                char *name,
                                int sleeptime,
                                int sleep_cnt_max) {
  int rc;
  int sleep_cnt=0;
  AssertFatal((rc = pthread_mutex_lock(mutex))==0,"wakeup_thread(): error locking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);

  while (*instance_cnt == 0 && !oai_exit) {
    AssertFatal((rc = pthread_mutex_unlock(mutex))==0,"wakeup_thread(): error unlocking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);
    sleep_cnt++;

    if (sleep_cnt>sleep_cnt_max) return(-1);

    usleep(sleeptime);
    AssertFatal((rc = pthread_mutex_lock(mutex))==0,"wakeup_thread(): error locking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);
  }

  *instance_cnt = *instance_cnt + 1;
  AssertFatal((rc = pthread_mutex_unlock(mutex))==0,"wakeup_thread(): error unlocking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);

  // the thread can now be woken up
  if (pthread_cond_signal(cond) != 0) {
    LOG_E( PHY, "ERROR pthread_cond_signal\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }

  AssertFatal((rc = pthread_mutex_unlock(mutex))==0,"wakeup_thread(): error unlocking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);
  return(0);
}


static inline int timedwait_on_condition(pthread_mutex_t *mutex,
    pthread_cond_t *cond,
    int *instance_cnt,
    char *name,
    uint32_t time_ns) {
  int rc;
  int waitret=0;
  struct timespec now, abstime;
  AssertFatal((rc = pthread_mutex_lock(mutex))==0,"[SCHED][eNB] timedwait_on_condition(): error locking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);

  while (*instance_cnt < 0 && !oai_exit) {
    clock_gettime(CLOCK_REALTIME, &now);
    // most of the time the thread is waiting here
    // proc->instance_cnt_rxtx is -1
    abstime.tv_sec = now.tv_sec;
    abstime.tv_nsec = now.tv_nsec + time_ns;

    if (abstime.tv_nsec >= 1000*1000*1000) {
      abstime.tv_nsec -= 1000*1000*1000;
      abstime.tv_sec  += 1;
    }

    if ((waitret = pthread_cond_timedwait(cond,mutex,&abstime)) == 0) break; // this unlocks mutex_rxtx while waiting and then locks it again
  }

  AssertFatal((rc = pthread_mutex_unlock(mutex)) == 0,"[SCHED][eNB] timedwait_on_condition(): error unlocking mutex return %d for %s\n", rc, name);
  return(waitret);
}


static inline int wait_on_condition(pthread_mutex_t *mutex,
                                    pthread_cond_t *cond,
                                    int *instance_cnt,
                                    char *name) {
  int rc;
  AssertFatal((rc = pthread_mutex_lock(mutex))==0,"[SCHED][eNB] wait_on_condition(): error locking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);

  while (*instance_cnt < 0 && !oai_exit) {
    // most of the time the thread is waiting here
    // proc->instance_cnt_rxtx is -1
    pthread_cond_wait(cond,mutex); // this unlocks mutex_rxtx while waiting and then locks it again
  }

  AssertFatal((rc = pthread_mutex_unlock(mutex))==0,"[SCHED][eNB] wait_on_condition(): error unlocking mutex return %d for %s\n", rc, name);
  return(0);
}


static inline int wait_on_busy_condition(pthread_mutex_t *mutex,
    pthread_cond_t *cond,
    int *instance_cnt,
    char *name) {
  int rc;
  AssertFatal((rc = pthread_mutex_lock(mutex))==0,"[SCHED][eNB] wait_on_busy_condition(): error locking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);

  while (*instance_cnt == 0 && !oai_exit) {
    // most of the time the thread will skip this
    // waits only if proc->instance_cnt_rxtx is 0
    pthread_cond_wait(cond,mutex); // this unlocks mutex_rxtx while waiting and then locks it again
  }

  AssertFatal((rc = pthread_mutex_unlock(mutex))==0,"[SCHED][eNB] wait_on_busy_condition(): error unlocking mutex return %d for %s\n", rc, name);
  return(0);
}


static inline int release_thread(pthread_mutex_t *mutex,
                                 int *instance_cnt,
                                 char *name) {
  int rc;
  AssertFatal((rc = pthread_mutex_lock(mutex))==0,"[SCHED][eNB] release_thread(): error locking mutex for %s (%d %s, %p)\n", name, rc, strerror(rc), (void *)mutex);
  *instance_cnt=-1;
  AssertFatal((rc = pthread_mutex_unlock(mutex))==0,"[SCHED][eNB] release_thread(): error unlocking mutex return %d for %s\n", rc, name);
  return(0);
}

#endif //  __PHY_DEFS_COMMON_H__
