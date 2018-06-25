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


#ifndef _NFAPI_NR_INTERFACE_NR_EXTENSION_H_
#define _NFAPI_NR_INTERFACE_NR_EXTENSION_H_
#define _NFAPI_NR_INTERFACE_H_

#include "stddef.h"

// Constants - update based on implementation
#define NFAPI_NR_MAX_PHY_RF_INSTANCES 2
#define NFAPI_NR_PNF_PARAM_GENERAL_LOCATION_LENGTH 16
#define NFAPI_NR_PNF_PARAM_GENERAL_OUI_LENGTH 3
#define NFAPI_NR_MAX_NUM_RF_BANDS 16

// The following definition control the size of arrays used in the interface.
// These may be changed if desired. They are used in the encoder to make sure 
// that the user has not specified a 'count' larger than the max array, and also
// used by the decoder when decode an array. If the 'count' received is larger
// than the array it is to be stored in the decode fails.
#define NFAPI_NR_MAX_NUM_ANTENNAS 8
#define NFAPI_NR_MAX_NUM_SUBBANDS 13
#define NFAPI_NR_MAX_BF_VECTORS 8
#define NFAPI_NR_MAX_CC 1
#define NFAPI_NR_MAX_NUM_PHYSICAL_ANTENNAS 8
#define NFAPI_NR_MAX_RSSI 8
#define NFAPI_NR_MAX_PSC_LIST 32
#define NFAPI_NR_MAX_PCI_LIST 32
#define NFAPI_NR_MAX_CARRIER_LIST 32
#define NFAPI_NR_MAX_ARFCN_LIST 128
#define NFAPI_NR_MAX_LTE_CELLS_FOUND 8
#define NFAPI_NR_MAX_UTRAN_CELLS_FOUND 8
#define NFAPI_NR_MAX_GSM_CELLS_FOUND 8
#define NFAPI_NR_MAX_NB_IOT_CELLS_FOUND 8
#define NFAPI_NR_MAX_SI_PERIODICITY 8
#define NFAPI_NR_MAX_SI_INDEX 8
#define NFAPI_NR_MAX_MIB_LENGTH 32
#define NFAPI_NR_MAX_SIB_LENGTH 256
#define NFAPI_NR_MAX_SI_LENGTH 256
#define NFAPI_NR_MAX_OPAQUE_DATA 64
#define NFAPI_NR_MAX_NUM_SCHEDULED_UES 8 // Used in the TPM structure
#define NFAPI_NR_MAX_PNF_PHY 5
#define NFAPI_NR_MAX_PNF_PHY_RF_CONFIG 5
#define NFAPI_NR_MAX_PNF_RF  5
#define NFAPI_NR_MAX_NMM_FREQUENCY_BANDS 32
#define NFAPI_NR_MAX_RECEIVED_INTERFERENCE_POWER_RESULTS 100
#define NFAPI_NR_MAX_UL_DL_CONFIGURATIONS 5
#define NFAPI_NR_MAX_CSI_RS_RESOURCE_CONFIG 4
#define NFAPI_NR_MAX_ANTENNA_PORT_COUNT 8
#define NFAPI_NR_MAX_EPDCCH_PRB 8
#define NFAPI_NR_MAX_TX_PHYSICAL_ANTENNA_PORTS 8
#define NFAPI_NR_MAX_NUMBER_ACK_NACK_TDD 8
#define NFAPI_NR_MAX_RO_DL 8

#define NFAPI_NR_HEADER_LENGTH 8
#define NFAPI_NR_P7_HEADER_LENGTH 16

#define NFAPI_NR_VENDOR_EXTENSION_MIN_TAG_VALUE 0xF000
#define NFAPI_NR_VENDOR_EXTENSION_MAX_TAG_VALUE 0xFFFF

#define NFAPI_NR_VERSION_3_0_11	0x000
#define NFAPI_NR_VERSION_3_0_12    0x001

#define NFAPI_NR_HALF_FRAME_INDEX_FIRST_HALF 0
#define NFAPI_NR_HALF_FRAME_INDEX_SECOND_HALF 1

// The IANA agreed port definition of the P5 SCTP VNF enpoint 
// http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=7701
#define NFAPI_NR_P5_SCTP_PORT		7701


#define NFAPI_NR_MAX_NUM_DL_ALLOCATIONS 16
#define NFAPI_NR_MAX_NUM_UL_ALLOCATIONS 16
#define NFAPI_NR_MAX_NUM_SERVING_CELLS 32
#define NFAPI_NR_MAX_NUM_ZP_CSI_RS_RESOURCE_PER_SET 16

typedef unsigned int	uint32_t;
typedef unsigned short	uint16_t;
typedef unsigned char	uint8_t;
typedef signed int		int32_t;
typedef signed short	int16_t;
typedef signed char		int8_t;

typedef struct {
	uint16_t phy_id;
	uint16_t message_id;
	uint16_t message_length;
	uint16_t spare;
} nfapi_nr_p4_p5_message_header_t;

typedef struct {
	uint16_t phy_id;
	uint16_t message_id;
	uint16_t message_length;
	uint16_t m_segment_sequence; /* This consists of 3 fields - namely, M, Segement & Sequence number*/
	uint32_t checksum;
	uint32_t transmit_timestamp;
} nfapi_nr_p7_message_header_t;

typedef struct {
	uint16_t tag;
	uint16_t length;
} nfapi_nr_tl_t;
#define NFAPI_NR_TAG_LENGTH_PACKED_LEN 4





typedef struct {
	nfapi_nr_tl_t tl;
    //  common C-RNTI
	uint8_t dci_format; 
    uint8_t frequency_domain_resouce_assignment;    //  38.214 chapter 5.1.2.2
    uint8_t time_domain_resource_assignment;        //  38.214 chapter 5.1.2.1
	uint8_t frequency_hopping_enabled_flag;
	uint8_t frequency_hopping_bits;
	uint8_t mcs;
	uint8_t new_data_indication;
	uint8_t redundancy_version;
    uint8_t harq_process;
    uint8_t tpc_command;

    uint8_t ul_sul_ind;

    uint8_t carrier_indicator;
    uint8_t bwp_indndicator;
    uint8_t vrb_to_prb_mapping;
    uint8_t downlink_assignment_index_1;
    uint8_t downlink_assignment_index_2;
    uint8_t srs_resource_indicator;
    uint8_t precoding_information;
    uint8_t antenna_ports;
    uint8_t srs_request;
    uint8_t cqi_csi_request;
    uint8_t cbg_transmission_information;
    uint8_t ptrs_dmrs_association;
    
    uint8_t downlink_assignment_index;
    uint8_t pucch_resource_indicator;
    uint8_t pdsch_to_harq_feedback_timing_indicator;

    uint8_t short_messages_indicator;

    uint8_t prb_bundling_size_indicator;    //  38.214 chapter 5.1.2.3
    uint8_t rate_matching_indicator;
    uint8_t zp_csi_rs_trigger;
    uint8_t transmission_configuration_indication;
    uint8_t cbg_flushing_out_information;
    
    uint8_t slot_format_count;
    uint8_t *slot_format_indicators;    //  38.213 chapter 11.1.1
     
    uint8_t preemption_indication_count;
    uint8_t *preemption_indications;    //  38.213 chapter 11.2

    uint8_t tpc_command_count;
    uint8_t *tpc_command_numbers;

    uint8_t block_number_count;
    uint8_t *block_numbers;
    uint8_t dci2_3_srs_request;    //  38.212 table 7.3.1.1.2-5   
    uint8_t dci2_3_tpc_command;

} nfapi_nr_dci_pdu_rel15_t;
#define NFAPI_NR_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG 0x2020

typedef struct {
    nfapi_nr_tl_t tl;
    uint8_t uci_format;
    uint8_t uci_channel;
    uint8_t harq_ack_bits;
    uint32_t harq_ack;
    uint8_t csi_bits;
    uint32_t csi;
    uint8_t sr_bits;
    uint32_t sr;
} nfapi_nr_uci_pdu_rel15_t;




//
// Top level NFAPI messages
//



//
// P7
//

	typedef struct {
		uint16_t rnti;
		uint8_t dci_type;
		uint8_t dci_size;
        nfapi_nr_dci_pdu_rel15_t dci;
	}nfapi_nr_dci_indication_pdu_t;

	typedef struct {
		nfapi_nr_tl_t tl;
		uint16_t number_of_dcis;
		nfapi_nr_dci_indication_pdu_t* dci_list;
	} nfapi_nr_dci_indication_body_t;

///
typedef struct {
  	nfapi_nr_p7_message_header_t header;
  	uint16_t sfn_sf_slot;
  	nfapi_nr_dci_indication_body_t dci_indication_body;
} nfapi_nr_dci_indication_t;

	#define NFAPI_NR_TX_MAX_PDU 100
	typedef struct {
		nfapi_nr_tl_t tl;
		uint8_t* data;
	} nfapi_nr_rx_request_body_t;
	#define NFAPI_NR_TX_REQUEST_BODY_TAG 0x2022

///
typedef struct {
	nfapi_nr_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nfapi_nr_rx_request_body_t rx_request_body;
} nfapi_nr_rx_indication_t;

	typedef struct {
		nfapi_nr_tl_t tl;
		uint8_t ul_cqi;
		uint16_t timing_advance;
	} nfapi_nr_tx_indication_t;


    #define NFAPI_NR_TX_MAX_SEGMENTS 32
	typedef struct {
		uint16_t pdu_length;
		uint16_t pdu_index;
		uint8_t num_segments;
		struct {
			uint32_t segment_length;
			uint8_t* segment_data;
		} segments[NFAPI_NR_TX_MAX_SEGMENTS];

	} nfapi_nr_tx_indication_pdu_t;

	#define NFAPI_NR_RX_IND_MAX_PDU 100
	typedef struct {
		nfapi_nr_tl_t tl;
		nfapi_nr_tx_indication_t tx_indication;
		uint16_t number_of_pdus;
		nfapi_nr_tx_indication_pdu_t* tx_pdu_list;
	} nfapi_nr_tx_indication_body_t;
	#define NFAPI_NR_RX_INDICATION_BODY_TAG 0x2023

///
typedef struct {
	nfapi_nr_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nfapi_nr_tx_indication_body_t tx_indication_body;
} nfapi_nr_tx_request_t;


	typedef struct {
		uint8_t pdu_type;
		uint8_t pdu_size;
		union {
			/*nfapi_nr_ul_config_ulsch_pdu				ulsch_pdu;
			nfapi_nr_ul_config_ulsch_cqi_ri_pdu		ulsch_cqi_ri_pdu;
			nfapi_nr_ul_config_ulsch_harq_pdu			ulsch_harq_pdu;
			nfapi_nr_ul_config_ulsch_cqi_harq_ri_pdu	ulsch_cqi_harq_ri_pdu;
			nfapi_nr_ul_config_uci_cqi_pdu				uci_cqi_pdu;
			nfapi_nr_ul_config_uci_sr_pdu				uci_sr_pdu;
			nfapi_nr_ul_config_uci_harq_pdu			uci_harq_pdu;
			nfapi_nr_ul_config_uci_sr_harq_pdu			uci_sr_harq_pdu;
			nfapi_nr_ul_config_uci_cqi_harq_pdu		uci_cqi_harq_pdu;
			nfapi_nr_ul_config_uci_cqi_sr_pdu			uci_cqi_sr_pdu;
			nfapi_nr_ul_config_uci_cqi_sr_harq_pdu		uci_cqi_sr_harq_pdu;
			nfapi_nr_ul_config_srs_pdu					srs_pdu;
			nfapi_nr_ul_config_harq_buffer_pdu			harq_buffer_pdu;
			nfapi_nr_ul_config_ulsch_uci_csi_pdu		ulsch_uci_csi_pdu;
			nfapi_nr_ul_config_ulsch_uci_harq_pdu		ulsch_uci_harq_pdu;
			nfapi_nr_ul_config_ulsch_csi_uci_harq_pdu	ulsch_csi_uci_harq_pdu;*/
		};
	} nfapi_nr_ul_config_request_pdu_t;

	typedef struct {
		nfapi_nr_tl_t tl;
		nfapi_nr_ul_config_request_pdu_t ul_config_pdu_list;
	} nfapi_nr_ul_config_request_body_t;
///
typedef struct {
	nfapi_nr_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nfapi_nr_ul_config_request_body_t ul_config_request_body;
} nfapi_nr_ul_config_request_t;



	typedef struct {
		uint8_t pdu_type;
		uint8_t pdu_size;
		union {
			/*nfapi_nr_dl_config_dlsch_pdu	dlsch_pdu;
			nfapi_nr_dl_config_prs_pdu		prs_pdu;
			nfapi_nr_dl_config_csi_rs_pdu	csi_rs_pdu;*/
		};
	} nfapi_nr_dl_config_request_pdu_t;

///
typedef struct {
	nfapi_nr_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nfapi_nr_dl_config_request_pdu_t dl_config_request_body;
} nfapi_nr_dl_config_request_t;


//
// P5
//

    typedef struct {
        uint32_t frequency_domain_resource;
        uint8_t duration;
        uint8_t cce_reg_mapping_type;                   //  interleaved or noninterleaved
        uint8_t cce_reg_interleaved_reg_bundle_size;    //  valid if CCE to REG mapping type is interleaved type
        uint8_t cce_reg_interleaved_interleaver_size;   //  valid if CCE to REG mapping type is interleaved type
        uint8_t cce_reg_interleaved_shift_index;        //  valid if CCE to REG mapping type is interleaved type
        uint8_t precoder_granularity;
        uint8_t tci_state_pdcch;
        uint8_t tci_present_in_dci;
        uint16_t pdcch_dmrs_scrambling_id;
    } nfapi_nr_coreset_t;

    typedef struct {
        nfapi_nr_coreset_t coreset;

        uint8_t monitoring_slot_peridicity;
        uint8_t monitoring_slot_offset;
        uint16_t duration;
        uint16_t monitoring_symbols_within_slot;
        uint8_t number_of_candidates[5];            //  aggregation level 1, 2, 4, 8, 16

        uint8_t dci_2_0_number_of_candidates[5];    //  aggregation level 1, 2, 4, 8, 16
        uint8_t dci_2_3_monitorying_periodicity;
        uint8_t dci_2_3_number_of_candidates;
        
    } nfapi_nr_search_space_t;

    typedef struct {
        nfapi_nr_search_space_t search_space_sib1;
        nfapi_nr_search_space_t search_space_others_sib;
        nfapi_nr_search_space_t search_space_paging;
        nfapi_nr_coreset_t      coreset_ra;         //  common coreset
        nfapi_nr_search_space_t search_space_ra;    
    } nfapi_nr_pdcch_config_common_t;

    typedef struct {
        uint8_t k0;
        uint8_t mapping_type;
        uint8_t symbol_starting;
        uint8_t symbol_length;
    } nfapi_nr_pdsch_time_domain_resource_allocation_t;

    typedef struct {
        nfapi_nr_pdsch_time_domain_resource_allocation_t allocation_list[NFAPI_NR_MAX_NUM_DL_ALLOCATIONS];
    } nfapi_nr_pdsch_config_common_t;

    typedef struct {
        uint8_t prach_configuration_index;
        uint8_t msg1_fdm;
        uint8_t msg1_frequency_start;
        uint8_t zero_correlation_zone_config;
        uint8_t preamble_received_target_power;
        uint8_t preamble_transmission_max;
        uint8_t power_ramping_step;
        uint8_t ra_window_size;

        uint8_t total_number_of_preamble;
        uint8_t ssb_occasion_per_rach;
        uint8_t cb_preamble_per_ssb;

        uint8_t group_a_msg3_size;
        uint8_t group_a_number_of_preamble;
        uint8_t group_b_power_offset;
        uint8_t contention_resolution_timer;
        uint8_t rsrp_threshold_ssb;
        uint8_t rsrp_threshold_ssb_sul;
        uint8_t prach_length;   //  l839, l139
        uint8_t prach_root_sequence_index;  //  0 - 837 for l839, 0 - 137 for l139
        uint8_t msg1_subcarrier_spacing;
        uint8_t restrictedset_config;
        uint8_t msg3_transform_precoding;
    } nfapi_nr_rach_config_common_t;

    typedef struct {
        uint8_t k2;
        uint8_t mapping_type;
        uint8_t symbol_starting;
        uint8_t symbol_length;
    } nfapi_nr_pusch_time_domain_resource_allocation_t;
      
    typedef struct {
        uint8_t group_hopping_enabled_transform_precoding;
        nfapi_nr_pusch_time_domain_resource_allocation_t allocation_list[NFAPI_NR_MAX_NUM_UL_ALLOCATIONS];
        uint8_t msg3_delta_preamble;
        uint8_t p0_nominal_with_grant;
    } nfapi_nr_pusch_config_common_t;

    typedef struct {
        uint8_t pucch_resource_common;
        uint8_t pucch_group_hopping;
        uint8_t hopping_id;
        uint8_t p0_nominal;
    } nfapi_nr_pucch_config_common_t;

    typedef struct {
        uint8_t subcarrier_spacing_common;
        uint8_t ssb_subcarrier_offset;
        uint8_t dmrs_type_a_position;
        uint8_t pdcch_config_sib1;
        uint8_t cell_barred;
        uint8_t intra_frquency_reselection;
    } nfapi_nr_pbch_config_t;

    typedef struct {
        nfapi_nr_tl_t tl;
        
        nfapi_nr_pdcch_config_common_t pdcch_config_common;
        nfapi_nr_pdsch_config_common_t pdsch_config_common;
        
    } nfapi_nr_dl_bwp_common_config_t;



    typedef struct {
        uint16_t int_rnti;
        uint8_t time_frequency_set;
        uint8_t dci_payload_size;
        uint8_t serving_cell_id[NFAPI_NR_MAX_NUM_SERVING_CELLS];    //  interrupt configuration per serving cell
        uint8_t position_in_dci[NFAPI_NR_MAX_NUM_SERVING_CELLS];    //  interrupt configuration per serving cell
    } nfapi_nr_downlink_preemption_t;

    typedef struct {
        uint8_t tpc_index;
        uint8_t tpc_index_sul;
        uint8_t target_cell;
    } nfapi_nr_pusch_tpc_command_config_t;

    typedef struct {
        uint8_t tpc_index_pcell;
        uint8_t tpc_index_pucch_scell;
    } nfapi_nr_pucch_tpc_command_config_t;

    typedef struct {
        uint8_t starting_bit_of_format_2_3;
        uint8_t feild_type_format_2_3;
    } nfapi_nr_srs_tpc_command_config_t;

    typedef struct {
        nfapi_nr_downlink_preemption_t downlink_preemption;
        nfapi_nr_pusch_tpc_command_config_t tpc_pusch;
        nfapi_nr_pucch_tpc_command_config_t tpc_pucch;
        nfapi_nr_srs_tpc_command_config_t tpc_srs;
    } nfapi_nr_pdcch_config_dedicated_t;

    typedef struct {
        uint8_t dmrs_type;
        uint8_t dmrs_addition_position;
        uint8_t max_length;
        uint16_t scrambling_id_0;
        uint16_t scrambling_id_1;
        uint8_t ptrs_frequency_density[2];      //  phase tracking rs
        uint8_t ptrs_time_density[3];           //  phase tracking rs
        uint8_t ptrs_epre_ratio;                //  phase tracking rs
        uint8_t ptrs_resource_element_offset;   //  phase tracking rs
    } nfapi_nr_dmrs_downlink_config_t;

    typedef struct {
        uint8_t bwp_or_cell_level;
        uint8_t pattern_type;
        uint32_t resource_blocks[9];        //  bitmaps type 275 bits
        uint8_t slot_type;                  //  bitmaps type one/two slot(s)
        uint32_t symbols_in_resouece_block; //  bitmaps type 14/28 bits
        uint8_t periodic;                   //  bitmaps type 
        uint32_t pattern[2];                //  bitmaps type 2/4/5/8/10/20/40 bits

        nfapi_nr_coreset_t coreset;         //  coreset

        uint8_t subcarrier_spacing;
        uint8_t mode;
    } nfapi_nr_rate_matching_pattern_group_t;

    typedef struct {
        //  resource mapping
        uint8_t row;    //  row1/row2/row4/other
        uint16_t frequency_domain_allocation; //    4/12/3/6 bits
        uint8_t number_of_ports;
        uint8_t first_ofdm_symbol_in_time_domain;
        uint8_t first_ofdm_symbol_in_time_domain2;
        uint8_t cdm_type;
        uint8_t density;            //  .5/1/3
        uint8_t density_dot5_type;  //  even/odd PRBs
        
        uint8_t frequency_band_starting_rb;     //  freqBand
        uint8_t frequency_band_number_of_rb;    //  freqBand

        //  periodicityAndOffset
        uint8_t periodicity;    //  slot4/5/8/10/16/20/32/40/64/80/160/320/640
        uint32_t offset;        //  0..639 bits
    } nfapi_nr_zp_csi_rs_resource_t;

    typedef struct {
        uint16_t data_scrambling_id_pdsch;
        nfapi_nr_dmrs_downlink_config_t dmrs_dl_for_pdsch_mapping_type_a;
        nfapi_nr_dmrs_downlink_config_t dmrs_dl_for_pdsch_mapping_type_b; 
        uint8_t vrb_to_prb_interleaver;
        uint8_t resource_allocation;
        nfapi_nr_pdsch_time_domain_resource_allocation_t allocation_list[NFAPI_NR_MAX_NUM_DL_ALLOCATIONS];
        uint8_t pdsch_aggregation_factor;
        nfapi_nr_rate_matching_pattern_group_t rate_matching_pattern_group1;
        nfapi_nr_rate_matching_pattern_group_t rate_matching_pattern_group2;
        uint8_t rbg_size;
        uint8_t mcs_table;
        uint8_t max_num_of_code_word_scheduled_by_dci;
        uint8_t bundle_size;        //  prb_bundling static
        uint8_t bundle_size_set1;   //  prb_bundling dynamic 
        uint8_t bundle_size_set2;   //  prb_bundling dynamic
        nfapi_nr_zp_csi_rs_resource_t periodically_zp_csi_rs_resource_set[NFAPI_NR_MAX_NUM_ZP_CSI_RS_RESOURCE_PER_SET];
    } nfapi_nr_pdsch_config_dedicated_t;

    typedef struct {
        uint16_t starting_prb;
        uint8_t intra_slot_frequency_hopping;
        uint16_t second_hop_prb;
        uint8_t format;                 //  pucch format 0..4
        uint8_t initial_cyclic_shift;
        uint8_t number_of_symbols;
        uint8_t starting_symbol_index;
        uint8_t time_domain_occ;
        uint8_t number_of_prbs;
        uint8_t occ_length;
        uint8_t occ_index;
    } nfapi_nr_pucch_resource_t;

    typedef struct {
        uint8_t periodicity;
        uint8_t number_of_harq_process;
        nfapi_nr_pucch_resource_t n1_pucch_an;
    } nfapi_nr_sps_config_t;

    typedef struct {
        uint8_t beam_failure_instance_max_count;
        uint8_t beam_failure_detection_timer;
    } nfapi_nr_radio_link_monitoring_config_t;

    typedef struct {
        nfapi_nr_pdcch_config_dedicated_t pdcch_config_dedicated;
        nfapi_nr_pdsch_config_dedicated_t pdsch_config_dedicated;
        nfapi_nr_sps_config_t sps_config;
        nfapi_nr_radio_link_monitoring_config_t radio_link_monitoring_config;

    } nfapi_nr_dl_bwp_dedicated_config_t;

    typedef struct {
        nfapi_nr_rach_config_common_t  rach_config_common;
        nfapi_nr_pusch_config_common_t pusch_config_common;
        nfapi_nr_pucch_config_common_t pucch_config_common;

    } nfapi_nr_ul_bwp_common_config_t;
        
    typedef struct {

    } nfapi_nr_ul_bwp_dedicated_config_t;

typedef struct {
    nfapi_nr_p4_p5_message_header_t header;
    uint8_t num_tlv;

    nfapi_nr_pbch_config_t pbch_config_common;  //  MIB

    nfapi_nr_dl_bwp_common_config_t     dl_bwp_common;
    nfapi_nr_dl_bwp_dedicated_config_t  dl_bwp_dedicated;

    nfapi_nr_ul_bwp_common_config_t     ul_bwp_common;
    nfapi_nr_ul_bwp_dedicated_config_t  ul_bwp_dedicated;

} nfapi_nr_config_request_t;

#endif /* _NFAPI_INTERFACE_H_ */
