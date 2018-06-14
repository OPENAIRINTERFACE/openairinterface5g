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
#define _NFAPI_INTERFACE_NR_EXTENSION_H_
#define _NFAPI_INTERFACE_H_

#include "stddef.h"

// Constants - update based on implementation
#define NR_NFAPI_MAX_PHY_RF_INSTANCES 2
#define NR_NFAPI_PNF_PARAM_GENERAL_LOCATION_LENGTH 16
#define NR_NFAPI_PNF_PARAM_GENERAL_OUI_LENGTH 3
#define NR_NFAPI_MAX_NUM_RF_BANDS 16

// The following definition control the size of arrays used in the interface.
// These may be changed if desired. They are used in the encoder to make sure 
// that the user has not specified a 'count' larger than the max array, and also
// used by the decoder when decode an array. If the 'count' received is larger
// than the array it is to be stored in the decode fails.
#define NR_NFAPI_MAX_NUM_ANTENNAS 8
#define NR_NFAPI_MAX_NUM_SUBBANDS 13
#define NR_NFAPI_MAX_BF_VECTORS 8
#define NR_NFAPI_MAX_CC 1
#define NR_NFAPI_MAX_NUM_PHYSICAL_ANTENNAS 8
#define NR_NFAPI_MAX_RSSI 8
#define NR_NFAPI_MAX_PSC_LIST 32
#define NR_NFAPI_MAX_PCI_LIST 32
#define NR_NFAPI_MAX_CARRIER_LIST 32
#define NR_NFAPI_MAX_ARFCN_LIST 128
#define NR_NFAPI_MAX_LTE_CELLS_FOUND 8
#define NR_NFAPI_MAX_UTRAN_CELLS_FOUND 8
#define NR_NFAPI_MAX_GSM_CELLS_FOUND 8
#define NR_NFAPI_MAX_NB_IOT_CELLS_FOUND 8
#define NR_NFAPI_MAX_SI_PERIODICITY 8
#define NR_NFAPI_MAX_SI_INDEX 8
#define NR_NFAPI_MAX_MIB_LENGTH 32
#define NR_NFAPI_MAX_SIB_LENGTH 256
#define NR_NFAPI_MAX_SI_LENGTH 256
#define NR_NFAPI_MAX_OPAQUE_DATA 64
#define NR_NFAPI_MAX_NUM_SCHEDULED_UES 8 // Used in the TPM structure
#define NR_NFAPI_MAX_PNF_PHY 5
#define NR_NFAPI_MAX_PNF_PHY_RF_CONFIG 5
#define NR_NFAPI_MAX_PNF_RF  5
#define NR_NFAPI_MAX_NMM_FREQUENCY_BANDS 32
#define NR_NFAPI_MAX_RECEIVED_INTERFERENCE_POWER_RESULTS 100
#define NR_NFAPI_MAX_UL_DL_CONFIGURATIONS 5
#define NR_NFAPI_MAX_CSI_RS_RESOURCE_CONFIG 4
#define NR_NFAPI_MAX_ANTENNA_PORT_COUNT 8
#define NR_NFAPI_MAX_EPDCCH_PRB 8
#define NR_NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS 8
#define NR_NFAPI_MAX_NUMBER_ACK_NACK_TDD 8
#define NR_NFAPI_MAX_RO_DL 8

#define NR_NFAPI_HEADER_LENGTH 8
#define NR_NFAPI_P7_HEADER_LENGTH 16

#define NR_NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE 0xF000
#define NR_NFAPI_VENDOR_EXTENSION_MAX_TAG_VALUE 0xFFFF

#define NR_NFAPI_VERSION_3_0_11	0x000
#define NR_NFAPI_VERSION_3_0_12    0x001

#define NR_NFAPI_HALF_FRAME_INDEX_FIRST_HALF 0
#define NR_NFAPI_HALF_FRAME_INDEX_SECOND_HALF 1

// The IANA agreed port definition of the P5 SCTP VNF enpoint 
// http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=7701
#define NR_NFAPI_P5_SCTP_PORT		7701

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
} nr_nfapi_p4_p5_message_header_t;

typedef struct {
	uint16_t phy_id;
	uint16_t message_id;
	uint16_t message_length;
	uint16_t m_segment_sequence; /* This consists of 3 fields - namely, M, Segement & Sequence number*/
	uint32_t checksum;
	uint32_t transmit_timestamp;
} nr_nfapi_p7_message_header_t;

typedef struct {
	uint16_t tag;
	uint16_t length;
} nr_nfapi_tl_t;
#define NR_NFAPI_TAG_LENGTH_PACKED_LEN 4





typedef struct {
	nr_nfapi_tl_t tl;
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

    //  format 0_0 C-RNTI
    uint8_t ul_sul_ind;

    //  format 0_1 C-RNTI
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
    
    //  format 0_1 CS-RNTI
    
    //  format 0_1 SP-CSI-RNTI

} nr_nfapi_dci0_pdu_rel15_t;
#define NR_NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG 0x2020

typedef struct {
	nr_nfapi_tl_t tl;
    //  common C-RNTI, TC-RNTI and RA-RNTI
	uint8_t dci_format; 
    uint8_t frequency_domain_resouce_assignment;
    uint8_t time_domain_resource_assignment;
    uint8_t vrb_to_prb_mapping;
	uint8_t mcs;
	uint8_t new_data_indication;
	uint8_t redundancy_version;
    uint8_t harq_process;
    uint8_t downlink_assignment_index;
    uint8_t tpc_command;
    uint8_t pucch_resource_indicator;
    uint8_t pdsch_to_harq_feedback_timing_indicator;

    //  format 1_0 P-RNTI
    uint8_t short_messages_indicator;

    //  format 1_0 SI-RNTI
    
    //  format 1_0 CS-RNTI
    

    //  format 1_1 C-RNTI
    uint8_t carrier_indicator;
    uint8_t bwp_indndicator;
    uint8_t prb_bundling_size_indicator;    //  38.214 chapter 5.1.2.3
    uint8_t rate_matching_indicator;
    uint8_t zp_csi_rs_trigger;
    uint8_t antenna_ports;
    uint8_t transmission_configuration_indication;
    uint8_t srs_request;
    uint8_t cbg_transmission_information;
    uint8_t cbg_flushing_out_information;

    //  format 1_1 CS-RNTI

} nr_nfapi_dci1_pdu_rel15_t;
#define NR_NFAPI_DCI1_REQUEST_DCI_PDU_REL8_TAG 0x2020

typedef struct {
    nr_nfapi_tl_t tl;
    //  common
    uint8_t dci_format;

    //  format 2_0 SFI-RNTI (SFI)
    uint8_t slot_format_count;
    uint8_t *slot_format_indicators;    //  38.213 chapter 11.1.1
     
    //  format 2_1 INT-RNTI (INT)
    uint8_t preemption_indication_count;
    uint8_t *preemption_indications;    //  38.213 chapter 11.2

    //  format 2_2 TPC-PUSCH-RNTI and TPC-PUCCH-RNTI (TPC for PUSCH, PUCCH)
    uint8_t tpc_command_count;
    uint8_t *tpc_command_numbers;

    //  format 2_3 TPC-SRS-RNTI  (TPC for SRS)
    uint8_t block_number_count;
    uint8_t *block_numbers;
    //  38.212 chapter 7.3.1.3.4
    uint8_t srs_request;    //  38.212 table 7.3.1.1.2-5   
    uint8_t tpc_command;

} nr_nfapi_dci2_pdu_rel15_t;

typedef struct {
    nr_nfapi_tl_t tl;
    uint8_t uci_format;
    uint8_t uci_channel;
    uint8_t harq_ack_bits;
    uint32_t harq_ack;
    uint8_t csi_bits;
    uint32_t csi;
    uint8_t sr_bits;
    uint32_t sr;
} nr_nfapi_uci_pdu_rel15_t;




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
		union {
			nr_nfapi_dci0_pdu_rel15_t dci0;
			nr_nfapi_dci1_pdu_rel15_t dci1;
			nr_nfapi_dci2_pdu_rel15_t dci2;
		} ;
	}nr_nfapi_dci_indication_pdu_t;

	typedef struct {
		nr_nfapi_tl_t tl;
		uint16_t number_of_dcis;
		nr_nfapi_dci_indication_pdu_t* dci_list;
	} nr_nfapi_dci_indication_body_t;

///
typedef struct {
  	nr_nfapi_p7_message_header_t header;
  	uint16_t sfn_sf_slot;
  	nr_nfapi_dci_indication_body_t dci_indication_body;
} nr_nfapi_dci_indication_t;

	#define NR_NFAPI_TX_MAX_PDU 100
	typedef struct {
		nr_nfapi_tl_t tl;
		uint8_t* data;
	} nr_nfapi_rx_request_body_t;
	#define NR_NFAPI_TX_REQUEST_BODY_TAG 0x2022

///
typedef struct {
	nr_nfapi_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nr_nfapi_rx_request_body_t rx_request_body;
} nr_nfapi_rx_indication_t;

	typedef struct {
		nr_nfapi_tl_t tl;
		uint8_t ul_cqi;
		uint16_t timing_advance;
	} nr_nfapi_tx_indication_t;


    #define NR_NFAPI_TX_MAX_SEGMENTS 32
	typedef struct {
		uint16_t pdu_length;
		uint16_t pdu_index;
		uint8_t num_segments;
		struct {
			uint32_t segment_length;
			uint8_t* segment_data;
		} segments[NR_NFAPI_TX_MAX_SEGMENTS];

	} nr_nfapi_tx_indication_pdu_t;

	#define NR_NFAPI_RX_IND_MAX_PDU 100
	typedef struct {
		nr_nfapi_tl_t tl;
		nr_nfapi_tx_indication_t tx_indication;
		uint16_t number_of_pdus;
		nr_nfapi_tx_indication_pdu_t* tx_pdu_list;
	} nr_nfapi_tx_indication_body_t;
	#define NR_NFAPI_RX_INDICATION_BODY_TAG 0x2023

///
typedef struct {
	nr_nfapi_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nr_nfapi_tx_indication_body_t tx_indication_body;
} nr_nfapi_tx_request_t;



	typedef struct {
		uint8_t pdu_type;
		uint8_t pdu_size;
		union {
			/*nr_nfapi_ul_config_ulsch_pdu				ulsch_pdu;
			nr_nfapi_ul_config_ulsch_cqi_ri_pdu		ulsch_cqi_ri_pdu;
			nr_nfapi_ul_config_ulsch_harq_pdu			ulsch_harq_pdu;
			nr_nfapi_ul_config_ulsch_cqi_harq_ri_pdu	ulsch_cqi_harq_ri_pdu;
			nr_nfapi_ul_config_uci_cqi_pdu				uci_cqi_pdu;
			nr_nfapi_ul_config_uci_sr_pdu				uci_sr_pdu;
			nr_nfapi_ul_config_uci_harq_pdu			uci_harq_pdu;
			nr_nfapi_ul_config_uci_sr_harq_pdu			uci_sr_harq_pdu;
			nr_nfapi_ul_config_uci_cqi_harq_pdu		uci_cqi_harq_pdu;
			nr_nfapi_ul_config_uci_cqi_sr_pdu			uci_cqi_sr_pdu;
			nr_nfapi_ul_config_uci_cqi_sr_harq_pdu		uci_cqi_sr_harq_pdu;
			nr_nfapi_ul_config_srs_pdu					srs_pdu;
			nr_nfapi_ul_config_harq_buffer_pdu			harq_buffer_pdu;
			nr_nfapi_ul_config_ulsch_uci_csi_pdu		ulsch_uci_csi_pdu;
			nr_nfapi_ul_config_ulsch_uci_harq_pdu		ulsch_uci_harq_pdu;
			nr_nfapi_ul_config_ulsch_csi_uci_harq_pdu	ulsch_csi_uci_harq_pdu;*/
		};
	} nr_nfapi_ul_config_request_pdu_t;

	typedef struct {
		nr_nfapi_tl_t tl;
		nr_nfapi_ul_config_request_pdu_t ul_config_pdu_list;
	} nr_nfapi_ul_config_request_body_t;
///
typedef struct {
	nr_nfapi_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nr_nfapi_ul_config_request_body_t ul_config_request_body;
} nr_nfapi_ul_config_request_t;

    typedef struct {
        uint32_t frequency_domain_resource;
        uint8_t duration;
        uint8_t cce_reg_mapping_type;
        uint8_t cce_reg_interleaved_reg_bundle_size;    //  valid if CCE to REG mapping type is interleaved type
        uint8_t cce_reg_interleaved_interleaver_size;   //  valid if CCE to REG mapping type is interleaved type
        uint8_t cce_reg_interleaved_shift_index;        //  valid if CCE to REG mapping type is interleaved type
        uint8_t precoder_granularity;
        uint8_t tci_state_pdcch;
        uint8_t tci_present_in_dci;
        uint16_t pdcch_dmrs_scrambling_id;
    }nr_nfapi_coreset_t;

    typedef struct {
        uint8_t monitoring_slot_peridicity;
        uint8_t monitoring_slot_offset;
        uint16_t monitoring_symbols_within_slot;
        uint8_t aggregation_level;
        uint8_t number_of_candidates;
        uint8_t search_space_type;

        union {
            struct {
                uint8_t dci_0_0_and_1_0;
                uint8_t dci_2_0_aggregation_level;
                uint8_t dci_2_0_number_of_candidates;
                uint8_t dci_2_1;    //  empty now
                uint8_t dci_2_2;    //  empty now
                uint8_t dci_2_3_monitorying_periodicity;
                uint8_t dci_2_3_number_of_candidates;
            } css;
            struct {
                uint8_t dci_formats;
            } uss;
        };
        
    }nr_nfapi_search_space_t;

    typedef struct {
        nr_nfapi_search_space_t search_space_sib1;
        nr_nfapi_search_space_t search_space_others_sib;
        nr_nfapi_search_space_t search_space_paging;
        nr_nfapi_coreset_t      coreset_ra;
        nr_nfapi_search_space_t search_space_ra;
    } nr_nfapi_pdcch_config_common_t;

    typedef struct {
        uint8_t k0;
        uint8_t mapping_type;
        uint8_t start_symbol;
        uint8_t length_symbol;
    } nr_nfapi_pdsch_config_common_t;

    typedef struct {

    } nr_nfapi_rach_config_common_t;

    typedef struct {

    } nr_nfapi_pusch_config_common_t;

    typedef struct {
        uint8_t scs_common;
        uint8_t ssb_subcarrier_offset;
        uint8_t dmrs_type_a_position;
        uint8_t pdcch_config_sib1;
        uint8_t cell_barred;
        uint8_t intra_frquency_reselection;
    } nr_nfapi_pbch_config_t;

	typedef struct {
		nr_nfapi_tl_t tl;
		
        nr_nfapi_pbch_config_t pbch_config_common;  //MIB

        nr_nfapi_pdcch_config_common_t pdcch_config_common;
        nr_nfapi_pdsch_config_common_t pdsch_config_common;
        
        nr_nfapi_rach_config_common_t  rach_config_common;
        nr_nfapi_pusch_config_common_t pusch_config_common;


	} nr_nfapi_dl_config_dci_dl_pdu;


	typedef struct {
		uint8_t pdu_type;
		uint8_t pdu_size;
		union {
			/*nr_nfapi_dl_config_dlsch_pdu	dlsch_pdu;
			nr_nfapi_dl_config_prs_pdu		prs_pdu;
			nr_nfapi_dl_config_csi_rs_pdu	csi_rs_pdu;*/
		};
	} nr_nfapi_dl_config_request_pdu_t;

///
typedef struct {
	nr_nfapi_p7_message_header_t header;
	uint16_t sfn_sf_slot;
	nr_nfapi_dl_config_request_pdu_t dl_config_request_body;
} nr_nfapi_dl_config_request_t;


//
// P5
//

typedef struct {
    nr_nfapi_p4_p5_message_header_t header;
    uint8_t num_tlv;


    nr_nfapi_dl_config_dci_dl_pdu dci_dl_pdu;

} nr_nfapi_config_request_t;

#endif /* _NFAPI_INTERFACE_H_ */
