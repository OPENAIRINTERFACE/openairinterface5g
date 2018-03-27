/*
 * Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _NFAPI_INTERFACE_NR_EXTENSION_H_
#define _NFAPI_INTERFACE_H_
#define _NFAPI_INTERFACE_NR_EXTENSION_H_

#include "stddef.h"

#define NFAPI_MAX_PNF_PHY 5

typedef struct {
	uint16_t tag;
	uint16_t length;
} nfapi_tl_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t value;
} nfapi_uint16_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	int16_t value;
} nfapi_int16_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t value;
} nfapi_uint8_tlv_t;

typedef enum {
  NFAPI_3GPP_REL_SUPPORTED_9 = 1,
 	NFAPI_3GPP_REL_SUPPORTED_10 = 2,
 	NFAPI_3GPP_REL_SUPPORTED_11 = 4,
	NFAPI_3GPP_REL_SUPPORTED_12 = 8,
  NFAPI_3GPP_REL_SUPPORTED_15 = 64
} nfapi_3gpp_release_supported_e;
 
 
typedef enum {
 	NFAPI_RAT_TYPE_LTE = 0,
 	NFAPI_RAT_TYPE_UTRAN = 1,
 	NFAPI_RAT_TYPE_GERAN = 2,
	NFAPI_RAT_TYPE_NB_IOT = 3,
  NFAPI_RAT_TYPE_NR = 4
} nfapi_rat_type_e;

typedef struct {
	nfapi_uint16_tlv_t duplex_mode;
	nfapi_uint16_tlv_t pcfich_power_offset;
	nfapi_uint16_tlv_t pb;
	nfapi_uint16_tlv_t dl_cyclic_prefix_type;
	nfapi_uint16_tlv_t ul_cyclic_prefix_type;
} nfapi_subframe_config_t;

#define NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG 0x0001
#define NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG 0x0002
#define NFAPI_SUBFRAME_CONFIG_PB_TAG 0x0003
#define NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG 0x0004
#define NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG 0x0005

typedef struct {
	nfapi_uint16_tlv_t dl_channel_bandwidth;
	nfapi_uint16_tlv_t ul_channel_bandwidth;
	nfapi_uint16_tlv_t reference_signal_power;
	nfapi_uint16_tlv_t tx_antenna_ports;
	nfapi_uint16_tlv_t rx_antenna_ports;
} nfapi_rf_config_t;

#define NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG 0x000A
#define NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG 0x000B
#define NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG 0x000C
#define NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG 0x000D
#define NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG 0x000E

typedef struct {
  uint16_t phy_config_index;
  uint8_t mu;
} nfapi_pnf_phy_rel15_info_t;

typedef struct {
  nfapi_tl_t tl;
  uint16_t number_of_phys;
  nfapi_pnf_phy_rel15_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel15_t;
#define NFAPI_PNF_PHY_REL15_TAG 0x100H

typedef enum {
  NFAPI_HALF_FRAME_INDEX_FIRST_HALF = 0,
  NFAPI_HALF_FRAME_INDEX_SECOND_HALF = 1
} nfapi_half_frame_index_e;

typedef struct {
  nfapi_uint16_tlv_t primary_synchronization_signal_epre_eprers;
	nfapi_uint16_tlv_t secondary_synchronization_signal_epre_eprers;
	nfapi_uint16_tlv_t physical_cell_id;
  nfapi_half_frame_index_e half_frame_index;
  nfapi_uint16_tlv_t ssb_subcarrier_offset;
  nfapi_uint16_tlv_t ssb_position_in_burst;
  nfapi_uint16_tlv_t ssb_periodicity;
  nfapi_uint16_tlv_t ss_pbch_block_power;
} nfapi_sch_config_t;

#define NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG 0x001E
#define NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG 0x001F
#define NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG 0x0020
#define NFAPI_SCH_CONFIG_HALF_FRAME_INDEX_TAG 0x0021
#define NFAPI_SCH_CONFIG_SSB_SUBCARRIER_OFFSET_TAG 0x0022
#define NFAPI_SCH_CONFIG_SSB_POSITION_IN_BURST 0x0023
#define NFAPI_SCH_CONFIG_SSB_PERIODICITY 0x0024
#define NFAPI_SCH_CONFIG_SS_PBCH_BLOCK_POWER 0x0025

//temporary

typedef struct {
  nfapi_subframe_config_t subframe_config;
  nfapi_rf_config_t rf_config;
  nfapi_sch_config_t sch_config;
  nfapi_pnf_phy_rel15_t pnf_phy_rel15;
} nfapi_param_t;

#endif
