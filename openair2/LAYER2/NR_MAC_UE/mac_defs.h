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

/* \file mac_defs.h
 * \brief MAC data structures, constant, and function prototype
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __LAYER2_NR_MAC_DEFS_H__
#define __LAYER2_NR_MAC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform_types.h"
#include "NR_DRX-Config.h"
#include "NR_SchedulingRequestConfig.h"
#include "NR_BSR-Config.h"
#include "NR_TAG-Config.h"
#include "NR_PHR-Config.h"
#include "NR_RNTI-Value.h"
#include "NR_MIB.h"
#include "NR_MAC-CellGroupConfig.h"
#include "NR_PhysicalCellGroupConfig.h"
#include "NR_SpCellConfig.h"
#include "NR_ServingCellConfig.h"
#include "fapi_nr_ue_interface.h"
#include "NR_IF_Module.h"
#include "../NR_MAC_gNB/nr_mac_common.h"
#include "PHY/defs_nr_common.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac.h"

#define NB_NR_UE_MAC_INST 1
/*!\brief Maximum number of logical channl group IDs */


/*!\brief value for indicating BSR Timer is not running */
#define NR_MAC_UE_BSR_TIMER_NOT_RUNNING   (0xFFFF)

typedef enum {
    SFN_C_MOD_2_EQ_0, 
    SFN_C_MOD_2_EQ_1,
    SFN_C_IMPOSSIBLE
} SFN_C_TYPE;

// LTE structure, might need to be adapted for NR
typedef struct {
  /// buffer status for each lcgid
  uint8_t  BSR[NR_MAX_NUM_LCGID]; // should be more for mesh topology
  /// keep the number of bytes in rlc buffer for each lcgid
  int32_t  BSR_bytes[NR_MAX_NUM_LCGID];
  /// after multiplexing buffer remain for each lcid
  int32_t  LCID_buffer_remain[NR_MAX_NUM_LCID];
  /// sum of all lcid buffer size
  uint16_t  All_lcid_buffer_size_lastTTI;
  /// buffer status for each lcid
  uint8_t  LCID_status[NR_MAX_NUM_LCID];
  /// SR pending as defined in 36.321
  uint8_t  SR_pending;
  /// SR_COUNTER as defined in 36.321
  uint16_t SR_COUNTER;
  /// logical channel group ide for each LCID
  uint8_t  LCGID[NR_MAX_NUM_LCID];
  /// retxBSR-Timer, default value is sf2560
  uint16_t retxBSR_Timer;
  /// retxBSR_SF, number of subframe before triggering a regular BSR
  uint16_t retxBSR_SF;
  /// periodicBSR-Timer, default to infinity
  uint16_t periodicBSR_Timer;
  /// periodicBSR_SF, number of subframe before triggering a periodic BSR
  uint16_t periodicBSR_SF;
  /// default value is 0: not configured
  uint16_t sr_ProhibitTimer;
  /// sr ProhibitTime running
  uint8_t sr_ProhibitTimer_Running;
  ///  default value to n5
  uint16_t maxHARQ_Tx;
  /// default value is false
  uint16_t ttiBundling;
  /// default value is release
  struct DRX_Config *drx_config;
  /// default value is release
  struct MAC_MainConfig__phr_Config *phr_config;
  ///timer before triggering a periodic PHR
  uint16_t periodicPHR_Timer;
  ///timer before triggering a prohibit PHR
  uint16_t prohibitPHR_Timer;
  ///DL Pathloss change value
  uint16_t PathlossChange;
  ///number of subframe before triggering a periodic PHR
  int16_t periodicPHR_SF;
  ///number of subframe before triggering a prohibit PHR
  int16_t prohibitPHR_SF;
  ///DL Pathloss Change in db
  uint16_t PathlossChange_db;

  /// default value is false
  uint16_t extendedBSR_Sizes_r10;
  /// default value is false
  uint16_t extendedPHR_r10;

  //Bj bucket usage per  lcid
  int16_t Bj[NR_MAX_NUM_LCID];
  // Bucket size per lcid
  int16_t bucket_size[NR_MAX_NUM_LCID];
} NR_UE_SCHEDULING_INFO;


#define MAX_NUM_BWP 2

typedef enum {
  RA_IDLE=0,
  WAIT_RAR=1,
  WAIT_CONTENTION_RESOLUTION=2
} RA_state_t;
/*!\brief Top level UE MAC structure */
typedef struct {

  NR_ServingCellConfigCommon_t    *scc;
  NR_ServingCellConfig_t          *scd;
  int                             servCellIndex;
  ////  MAC config
  NR_DRX_Config_t    	          *drx_Config;
  NR_SchedulingRequestConfig_t    *schedulingRequestConfig;
  NR_BSR_Config_t    	          *bsr_Config;
  NR_TAG_Config_t	          *tag_Config;
  NR_PHR_Config_t	          *phr_Config;
  NR_RNTI_Value_t 	          *cs_RNTI;
  NR_MIB_t 	                  *mib;

  NR_BWP_Downlink_t               *DLbwp[MAX_NUM_BWP];
  NR_BWP_Uplink_t                 *ULbwp[MAX_NUM_BWP];
  NR_ControlResourceSet_t         *coreset[MAX_NUM_BWP][FAPI_NR_MAX_CORESET_PER_BWP];
  NR_SearchSpace_t                *SSpace[MAX_NUM_BWP][FAPI_NR_MAX_CORESET_PER_BWP][FAPI_NR_MAX_SS_PER_CORESET];

  ///     Type0-PDCCH seach space
  fapi_nr_dl_config_dci_dl_pdu_rel15_t type0_pdcch_dci_config;
  uint32_t type0_pdcch_ss_mux_pattern;
  SFN_C_TYPE type0_pdcch_ss_sfn_c;
  uint32_t type0_pdcch_ss_n_c;
  uint32_t type0_pdcch_consecutive_slots;
  /// state of RA procedure
  RA_state_t ra_state;
  ///     RA-rnti
  uint16_t ra_rnti;
  ///     Temporary CRNTI
  uint16_t t_crnti;
  ///     CRNTI
  uint16_t crnti;

  ////	FAPI-like interface message
  fapi_nr_tx_request_t tx_request;
  fapi_nr_ul_config_request_t ul_config_request;
  fapi_nr_dl_config_request_t dl_config_request;

  ///     Interface module instances
  nr_ue_if_module_t       *if_module;
  nr_scheduled_response_t scheduled_response;
  nr_phy_config_t         phy_config;

  /// BSR report flag management
  uint8_t BSR_reporting_active;
  NR_UE_SCHEDULING_INFO   scheduling_info;

  /// PHR
  uint8_t PHR_reporting_active;
} NR_UE_MAC_INST_t;

typedef enum seach_space_mask_e {
    type0_pdcch  = 0x1, 
    type0a_pdcch = 0x2,
    type1_pdcch  = 0x4, 
    type2_pdcch  = 0x8,
    type3_pdcch  = 0x10
} search_space_mask_t;

typedef enum subcarrier_spacing_e {
    scs_15kHz  = 0x1,
    scs_30kHz  = 0x2,
    scs_60kHz  = 0x4,
    scs_120kHz = 0x8,
    scs_240kHz = 0x16
} subcarrier_spacing_t;

typedef enum channel_bandwidth_e {
    bw_5MHz   = 0x1,
    bw_10MHz  = 0x2,
    bw_20MHz  = 0x4,
    bw_40MHz  = 0x8,
    bw_80MHz  = 0x16,
    bw_100MHz = 0x32
} channel_bandwidth_t;

typedef enum frequency_range_e {
    FR1 = 0, 
    FR2
} frequency_range_t;

typedef struct {
  uint8_t identifier_dci_formats          ; // 0  IDENTIFIER_DCI_FORMATS:
  uint8_t carrier_ind                     ; // 1  CARRIER_IND: 0 or 3 bits, as defined in Subclause x.x of [5, TS38.213]
  uint8_t sul_ind_0_1                     ; // 2  SUL_IND_0_1:
  uint8_t slot_format_ind                 ; // 3  SLOT_FORMAT_IND: size of DCI format 2_0 is configurable by higher layers up to 128 bits, according to Subclause 11.1.1 of [5, TS 38.213]
  uint8_t pre_emption_ind                 ; // 4  PRE_EMPTION_IND: size of DCI format 2_1 is configurable by higher layers up to 126 bits, according to Subclause 11.2 of [5, TS 38.213]. Each pre-emption indication is 14 bits
  uint8_t block_number                    ; // 5  BLOCK_NUMBER: starting position of a block is determined by the parameter startingBitOfFormat2_3
  uint8_t close_loop_ind                  ; // 6  CLOSE_LOOP_IND:
  uint8_t bandwidth_part_ind              ; // 7  BANDWIDTH_PART_IND:
  uint8_t short_message_ind               ; // 8  SHORT_MESSAGE_IND:
  uint8_t short_messages                  ; // 9  SHORT_MESSAGES:
  uint16_t freq_dom_resource_assignment_UL; // 10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
  //    (NOTE 1) If DCI format 0_0 is monitored in common search space
  //    and if the number of information bits in the DCI format 0_0 prior to padding
  //    is larger than the payload size of the DCI format 1_0 monitored in common search space
  //    the bitwidth of the frequency domain resource allocation field in the DCI format 0_0
  //    is reduced such that the size of DCI format 0_0 equals to the size of the DCI format 1_0
  uint16_t freq_dom_resource_assignment_DL; // 11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
  uint8_t time_dom_resource_assignment    ; // 12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
  //    where I the number of entries in the higher layer parameter pusch-AllocationList
  uint8_t vrb_to_prb_mapping              ; // 13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
  uint8_t prb_bundling_size_ind           ; // 14 PRB_BUNDLING_SIZE_IND:0 bit if the higher layer parameter PRB_bundling is not configured or is set to 'static', or 1 bit if the higher layer parameter PRB_bundling is set to 'dynamic' according to Subclause 5.1.2.3 of [6, TS 38.214]
  uint8_t rate_matching_ind               ; // 15 RATE_MATCHING_IND: 0, 1, or 2 bits according to higher layer parameter rate-match-PDSCH-resource-set
  uint8_t zp_csi_rs_trigger               ; // 16 ZP_CSI_RS_TRIGGER:
  uint8_t freq_hopping_flag               ; // 17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
  uint8_t tb1_mcs                         ; // 18 TB1_MCS:
  uint8_t tb1_ndi                         ; // 19 TB1_NDI:
  uint8_t tb1_rv                          ; // 20 TB1_RV:
  uint8_t tb2_mcs                         ; // 21 TB2_MCS:
  uint8_t tb2_ndi                         ; // 22 TB2_NDI:
  uint8_t tb2_rv                          ; // 23 TB2_RV:
  uint8_t mcs                             ; // 24 MCS:
  uint8_t ndi                             ; // 25 NDI:
  uint8_t rv                              ; // 26 RV:
  uint8_t harq_process_number             ; // 27 HARQ_PROCESS_NUMBER:
  uint8_t dai                             ; // 28 DAI: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
  //    2 if one serving cell is configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 bits are the counter DAI
  //    0 otherwise
  uint8_t first_dai                       ; // 29 FIRST_DAI: (1 or 2 bits) 1 bit for semi-static HARQ-ACK
  uint8_t second_dai                      ; // 30 SECOND_DAI: (0 or 2 bits) 2 bits for dynamic HARQ-ACK codebook with two HARQ-ACK sub-codebooks
  uint8_t tb_scaling                      ; // 31 TB_SCALING:
  uint8_t tpc_pusch                       ; // 32 TPC_PUSCH:
  uint8_t tpc_pucch                       ; // 33 TPC_PUCCH:
  uint8_t pucch_resource_ind              ; // 34 PUCCH_RESOURCE_IND:
  uint8_t pdsch_to_harq_feedback_time_ind ; // 35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
  uint8_t srs_resource_ind                ; // 36 SRS_RESOURCE_IND:
  uint8_t precod_nbr_layers               ; // 37 PRECOD_NBR_LAYERS:
  uint8_t antenna_ports                   ; // 38 ANTENNA_PORTS:
  uint8_t tci                             ; // 39 TCI: 0 bit if higher layer parameter tci-PresentInDCI is not enabled; otherwise 3 bits
  uint8_t srs_request                     ; // 40 SRS_REQUEST:
  uint8_t tpc_cmd                         ; // 41 TPC_CMD:
  uint8_t csi_request                     ; // 42 CSI_REQUEST:
  uint8_t cbgti                           ; // 43 CBGTI: 0, 2, 4, 6, or 8 bits determined by higher layer parameter maxCodeBlockGroupsPerTransportBlock for the PDSCH
  uint8_t cbgfi                           ; // 44 CBGFI: 0 or 1 bit determined by higher layer parameter codeBlockGroupFlushIndicator
  uint8_t ptrs_dmrs                       ; // 45 PTRS_DMRS:
  uint8_t beta_offset_ind                 ; // 46 BETA_OFFSET_IND:
  uint8_t dmrs_seq_ini                    ; // 47 DMRS_SEQ_INI: 1 bit if the cell has two ULs and the number of bits for DCI format 1_0 before padding
  //    is larger than the number of bits for DCI format 0_0 before padding; 0 bit otherwise
  uint8_t ul_sch_ind                      ; // 48 UL_SCH_IND:  value of "1" indicates UL-SCH shall be transmitted on the PUSCH and a value of "0" indicates UL-SCH shall not be transmitted on the PUSCH
  uint16_t padding_nr_dci                 ; // 49 PADDING_NR_DCI: (Note 2) If DCI format 0_0 is monitored in common search space
  //    and if the number of information bits in the DCI format 0_0 prior to padding
  //    is less than the payload size of the DCI format 1_0 monitored in common search space
  //    zeros shall be appended to the DCI format 0_0
  //    until the payload size equals that of the DCI format 1_0
  uint8_t sul_ind_0_0                     ; // 50 SUL_IND_0_0:
  uint8_t ra_preamble_index               ; // 51 RA_PREAMBLE_INDEX:
  uint8_t sul_ind_1_0                     ; // 52 SUL_IND_1_0:
  uint8_t ss_pbch_index                   ; // 53 SS_PBCH_INDEX
  uint8_t prach_mask_index                ; // 54 PRACH_MASK_INDEX
  uint8_t reserved_nr_dci                 ; // 55 RESERVED_NR_DCI
} nr_dci_pdu_rel15_t;

#define NUM_SLOT_FRAME 10

#define NBR_NR_FORMATS              8     // The number of formats is 8 (0_0, 0_1, 1_0, 1_1, 2_0, 2_1, 2_2, 2_3)
#define NBR_NR_DCI_FIELDS           56    // The number of different dci fields defined in TS 38.212 subclause 7.3.1

#define IDENTIFIER_DCI_FORMATS           0
#define CARRIER_IND                      1
#define SUL_IND_0_1                      2
#define SLOT_FORMAT_IND                  3
#define PRE_EMPTION_IND                  4
#define BLOCK_NUMBER                     5
#define CLOSE_LOOP_IND                   6
#define BANDWIDTH_PART_IND               7
#define SHORT_MESSAGE_IND                8
#define SHORT_MESSAGES                   9
#define FREQ_DOM_RESOURCE_ASSIGNMENT_UL 10
#define FREQ_DOM_RESOURCE_ASSIGNMENT_DL 11
#define TIME_DOM_RESOURCE_ASSIGNMENT    12
#define VRB_TO_PRB_MAPPING              13
#define PRB_BUNDLING_SIZE_IND           14
#define RATE_MATCHING_IND               15
#define ZP_CSI_RS_TRIGGER               16
#define FREQ_HOPPING_FLAG               17
#define TB1_MCS                         18
#define TB1_NDI                         19
#define TB1_RV                          20
#define TB2_MCS                         21
#define TB2_NDI                         22
#define TB2_RV                          23
#define MCS                             24
#define NDI                             25
#define RV                              26
#define HARQ_PROCESS_NUMBER             27
#define DAI_                            28
#define FIRST_DAI                       29
#define SECOND_DAI                      30
#define TB_SCALING                      31
#define TPC_PUSCH                       32
#define TPC_PUCCH                       33
#define PUCCH_RESOURCE_IND              34
#define PDSCH_TO_HARQ_FEEDBACK_TIME_IND 35
#define SRS_RESOURCE_IND                36
#define PRECOD_NBR_LAYERS               37
#define ANTENNA_PORTS                   38
#define TCI                             39
#define SRS_REQUEST                     40
#define TPC_CMD                         41
#define CSI_REQUEST                     42
#define CBGTI                           43
#define CBGFI                           44
#define PTRS_DMRS                       45
#define BETA_OFFSET_IND                 46
#define DMRS_SEQ_INI                    47
#define UL_SCH_IND                      48
#define PADDING_NR_DCI                  49
#define SUL_IND_0_0                     50
#define RA_PREAMBLE_INDEX               51
#define SUL_IND_1_0                     52
#define SS_PBCH_INDEX                   53
#define PRACH_MASK_INDEX                54
#define RESERVED_NR_DCI                 55

/*@}*/
#endif /*__LAYER2_MAC_DEFS_H__ */
