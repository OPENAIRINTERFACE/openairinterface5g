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


#ifndef _FAPI_NR_UE_INTERFACE_H_
#define _FAPI_NR_UE_INTERFACE_H_

#include "stddef.h"
#include "fapi_nr_ue_constants.h"

typedef unsigned int	uint32_t;
typedef unsigned short	uint16_t;
typedef unsigned char	uint8_t;
typedef signed int		int32_t;
typedef signed short	int16_t;
typedef signed char		int8_t;

typedef struct {
    //  dci pdu
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
    uint8_t padding_bits;
    uint8_t ul_sul_ind;
    uint8_t ul_sch_indicator;

    uint8_t random_access_preamble_index;
    uint8_t ss_pbch_index;
    uint8_t prach_mask_index;

    uint8_t carrier_indicator;
    uint8_t bwp_indndicator;
    uint8_t vrb_to_prb_mapping;
    uint8_t downlink_assignment_index1;
    uint8_t downlink_assignment_index2;
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
    uint8_t short_messages;
    uint8_t tb_scaling;

    uint8_t prb_bundling_size_indicator;    //  38.214 chapter 5.1.2.3
    uint8_t rate_matching_indicator;
    uint8_t zp_csi_rs_trigger;
    uint8_t transmission_configuration_indication;
    uint8_t cbg_flushing_out_information;
    
    uint8_t slot_format_count;
    uint8_t *slot_format_indicators;    //  38.213 chapter 11.1.1
     
    uint8_t preemption_indication_count;
    uint8_t *preemption_indications;    //  38.213 chapter 11.2

    uint8_t block_number_count;         //  for F22 and F23
    uint8_t *block_numbers;             //  for F22 and F23
    uint8_t closed_loop_indicator;

    uint8_t tpc_command_count;
    uint8_t *tpc_command_numbers;
    uint8_t dci2_3_srs_request;    //  38.212 table 7.3.1.1.2-5   
    uint8_t dci2_3_tpc_command;

} fapi_nr_dci_pdu_rel15_t;

typedef struct {
    uint8_t uci_format;
    uint8_t uci_channel;
    uint8_t harq_ack_bits;
    uint32_t harq_ack;
    uint8_t csi_bits;
    uint32_t csi;
    uint8_t sr_bits;
    uint32_t sr;
} fapi_nr_uci_pdu_rel15_t;

    

    typedef struct {
        /// frequency_domain_resource;
        uint32_t rb_start;
        uint32_t rb_end;

        uint8_t duration;
        uint8_t cce_reg_mapping_type;                   //  interleaved or noninterleaved
        uint8_t cce_reg_interleaved_reg_bundle_size;    //  valid if CCE to REG mapping type is interleaved type
        uint8_t cce_reg_interleaved_interleaver_size;   //  valid if CCE to REG mapping type is interleaved type
        uint8_t cce_reg_interleaved_shift_index;        //  valid if CCE to REG mapping type is interleaved type
        uint8_t precoder_granularity;
        uint16_t pdcch_dmrs_scrambling_id;

        uint8_t tci_state_pdcch;
        uint8_t tci_present_in_dci;
    } fapi_nr_coreset_t;

//
// Top level FAPI messages
//



//
// P7
//

	typedef struct {
		uint16_t rnti;
		uint8_t dci_type;
        fapi_nr_dci_pdu_rel15_t dci;
	} fapi_nr_dci_indication_pdu_t;


///
typedef struct {
  	uint32_t sfn_slot;
    uint16_t number_of_dcis;
  	fapi_nr_dci_indication_pdu_t *dci_list;
} fapi_nr_dci_indication_t;


    typedef struct {
        uint32_t pdu_length;
        uint8_t* pdu;
    } fapi_nr_pdsch_pdu_t;

    typedef struct {
        uint8_t* pdu;   //  3bytes
        uint8_t additional_bits;
        uint8_t ssb_index;
        uint8_t ssb_length;
        uint16_t cell_id;

    } fapi_nr_mib_pdu_t;

    typedef struct {
        uint32_t pdu_length;
        uint8_t* pdu;
        uint32_t sibs_mask;
    } fapi_nr_sib_pdu_t;

	typedef struct {
        uint8_t pdu_type;
        union {
            fapi_nr_pdsch_pdu_t pdsch_pdu;
            fapi_nr_mib_pdu_t mib_pdu;
            fapi_nr_sib_pdu_t sib_pdu;
        };
	} fapi_nr_rx_request_body_t;

///
typedef struct {
	uint32_t sfn_slot;
    uint16_t number_pdus;
	fapi_nr_rx_request_body_t *rx_request_body;
} fapi_nr_rx_indication_t;

	typedef struct {
		uint8_t ul_cqi;
		uint16_t timing_advance;
        uint16_t rnti;
	} fapi_nr_tx_config_t;

	typedef struct {
		uint16_t pdu_length;
		uint16_t pdu_index;
        uint8_t* pdu;
	} fapi_nr_tx_request_pdu_t;

	typedef struct {
		fapi_nr_tx_config_t tx_config;
		uint16_t number_of_pdus;
		fapi_nr_tx_request_pdu_t* tx_pdu_list;
	} fapi_nr_tx_request_body_t;

///
typedef struct {
	uint32_t sfn_slot;
	fapi_nr_tx_request_body_t tx_request_body;
} fapi_nr_tx_request_t;


	typedef struct {
		uint8_t pdu_type;
		uint8_t pdu_size;
		union {

		};
	} fapi_nr_ul_config_request_pdu_t;

	typedef struct {
		fapi_nr_ul_config_request_pdu_t ul_config_pdu_list;
	} fapi_nr_ul_config_request_body_t;
///
typedef struct {
	uint32_t sfn_slot;
	fapi_nr_ul_config_request_body_t ul_config_request_body;
} fapi_nr_ul_config_request_t;


    typedef struct {
        uint16_t rnti;

        fapi_nr_coreset_t coreset;
        uint32_t duration;
        uint8_t number_of_candidates[5];    //  aggregation level 1, 2, 4, 8, 16 
        uint16_t monitoring_symbols_within_slot;
        //  DCI foramt-specific
        uint8_t format_2_0_number_of_candidates[5];    //  aggregation level 1, 2, 4, 8, 16
        uint8_t format_2_3_monitorying_periodicity;
        uint8_t format_2_3_number_of_candidates;
    } fapi_nr_dl_config_dci_dl_pdu_rel15_t;

    typedef struct {
        fapi_nr_dl_config_dci_dl_pdu_rel15_t dci_config_rel15;
    } fapi_nr_dl_config_dci_pdu;

    typedef struct {
        uint16_t rnti;
        fapi_nr_dci_pdu_rel15_t dci_config;
    } fapi_nr_dl_config_dlsch_pdu_rel15_t;

    typedef struct {
        fapi_nr_dl_config_dlsch_pdu_rel15_t dlsch_config_rel15;
    } fapi_nr_dl_config_dlsch_pdu;

	typedef struct {
		uint8_t pdu_type;
		//uint8_t pdu_size;
		union {
            fapi_nr_dl_config_dci_pdu dci_pdu;
            fapi_nr_dl_config_dlsch_pdu dlsch_pdu;
		};
	} fapi_nr_dl_config_request_pdu_t;

typedef struct {
	uint32_t sfn_slot;
    uint8_t number_pdus;
	fapi_nr_dl_config_request_pdu_t dl_config_request_body[10];        //  TODO MARCO
} fapi_nr_dl_config_request_t;


//
// P5
//

    

    typedef struct {
        fapi_nr_coreset_t coreset;

        uint8_t monitoring_slot_peridicity;
        uint8_t monitoring_slot_offset;
        uint16_t duration;
        uint16_t monitoring_symbols_within_slot;
        uint8_t number_of_candidates[5];            //  aggregation level 1, 2, 4, 8, 16

        uint8_t dci_2_0_number_of_candidates[5];    //  aggregation level 1, 2, 4, 8, 16
        uint8_t dci_2_3_monitorying_periodicity;
        uint8_t dci_2_3_number_of_candidates;
        
    } fapi_nr_search_space_t;

    typedef struct {
        fapi_nr_search_space_t search_space_sib1;
        fapi_nr_search_space_t search_space_others_sib;
        fapi_nr_search_space_t search_space_paging;
        //fapi_nr_coreset_t      coreset_ra;         //  common coreset
        fapi_nr_search_space_t search_space_ra;    
    } fapi_nr_pdcch_config_common_t;

    typedef struct {
        uint8_t k0;
        uint8_t mapping_type;
        uint8_t symbol_starting;
        uint8_t symbol_length;
    } fapi_nr_pdsch_time_domain_resource_allocation_t;

    typedef struct {
        fapi_nr_pdsch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_DL_ALLOCATIONS];
    } fapi_nr_pdsch_config_common_t;

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
    } fapi_nr_rach_config_common_t;

    typedef struct {
        uint8_t k2;
        uint8_t mapping_type;
        uint8_t symbol_starting;
        uint8_t symbol_length;
    } fapi_nr_pusch_time_domain_resource_allocation_t;
      
    typedef struct {
        uint8_t group_hopping_enabled_transform_precoding;
        fapi_nr_pusch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_UL_ALLOCATIONS];
        uint8_t msg3_delta_preamble;
        uint8_t p0_nominal_with_grant;
    } fapi_nr_pusch_config_common_t;

    typedef struct {
        uint8_t pucch_resource_common;
        uint8_t pucch_group_hopping;
        uint8_t hopping_id;
        uint8_t p0_nominal;
    } fapi_nr_pucch_config_common_t;

    typedef struct {

        uint8_t subcarrier_spacing_common;
        uint8_t ssb_subcarrier_offset;
        uint8_t dmrs_type_a_position;
        uint8_t pdcch_config_sib1;
        uint8_t cell_barred;
        uint8_t intra_frequency_reselection;

        uint16_t system_frame_number;
        uint8_t ssb_index;
        uint8_t half_frame_bit;
    } fapi_nr_pbch_config_t;

    typedef struct {
        
        fapi_nr_pdcch_config_common_t pdcch_config_common;
        fapi_nr_pdsch_config_common_t pdsch_config_common;
        
    } fapi_nr_dl_bwp_common_config_t;



    typedef struct {
        uint16_t int_rnti;
        uint8_t time_frequency_set;
        uint8_t dci_payload_size;
        uint8_t serving_cell_id[FAPI_NR_MAX_NUM_SERVING_CELLS];    //  interrupt configuration per serving cell
        uint8_t position_in_dci[FAPI_NR_MAX_NUM_SERVING_CELLS];    //  interrupt configuration per serving cell
    } fapi_nr_downlink_preemption_t;

    typedef struct {
        uint8_t tpc_index;
        uint8_t tpc_index_sul;
        uint8_t target_cell;
    } fapi_nr_pusch_tpc_command_config_t;

    typedef struct {
        uint8_t tpc_index_pcell;
        uint8_t tpc_index_pucch_scell;
    } fapi_nr_pucch_tpc_command_config_t;

    typedef struct {
        uint8_t starting_bit_of_format_2_3;
        uint8_t feild_type_format_2_3;
    } fapi_nr_srs_tpc_command_config_t;

    typedef struct {
        fapi_nr_downlink_preemption_t downlink_preemption;
        fapi_nr_pusch_tpc_command_config_t tpc_pusch;
        fapi_nr_pucch_tpc_command_config_t tpc_pucch;
        fapi_nr_srs_tpc_command_config_t tpc_srs;
    } fapi_nr_pdcch_config_dedicated_t;

    typedef struct {
        uint8_t dmrs_type;
        uint8_t dmrs_addition_position;
        uint8_t max_length;
        uint16_t scrambling_id0;
        uint16_t scrambling_id1;
        uint8_t ptrs_frequency_density[2];      //  phase tracking rs
        uint8_t ptrs_time_density[3];           //  phase tracking rs
        uint8_t ptrs_epre_ratio;                //  phase tracking rs
        uint8_t ptrs_resource_element_offset;   //  phase tracking rs
    } fapi_nr_dmrs_downlink_config_t;

    typedef struct {
        uint8_t bwp_or_cell_level;
        uint8_t pattern_type;
        uint32_t resource_blocks[9];        //  bitmaps type 275 bits
        uint8_t slot_type;                  //  bitmaps type one/two slot(s)
        uint32_t symbols_in_resouece_block; //  bitmaps type 14/28 bits
        uint8_t periodic;                   //  bitmaps type 
        uint32_t pattern[2];                //  bitmaps type 2/4/5/8/10/20/40 bits

        fapi_nr_coreset_t coreset;         //  coreset

        uint8_t subcarrier_spacing;
        uint8_t mode;
    } fapi_nr_rate_matching_pattern_group_t;

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
    } fapi_nr_zp_csi_rs_resource_t;

    typedef struct {
        uint16_t data_scrambling_id_pdsch;
        fapi_nr_dmrs_downlink_config_t dmrs_dl_for_pdsch_mapping_type_a;
        fapi_nr_dmrs_downlink_config_t dmrs_dl_for_pdsch_mapping_type_b; 
        uint8_t vrb_to_prb_interleaver;
        uint8_t resource_allocation;
        fapi_nr_pdsch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_DL_ALLOCATIONS];
        uint8_t pdsch_aggregation_factor;
        fapi_nr_rate_matching_pattern_group_t rate_matching_pattern_group1;
        fapi_nr_rate_matching_pattern_group_t rate_matching_pattern_group2;
        uint8_t rbg_size;
        uint8_t mcs_table;
        uint8_t max_num_of_code_word_scheduled_by_dci;
        uint8_t bundle_size;        //  prb_bundling static
        uint8_t bundle_size_set1;   //  prb_bundling dynamic 
        uint8_t bundle_size_set2;   //  prb_bundling dynamic
        fapi_nr_zp_csi_rs_resource_t periodically_zp_csi_rs_resource_set[FAPI_NR_MAX_NUM_ZP_CSI_RS_RESOURCE_PER_SET];
    } fapi_nr_pdsch_config_dedicated_t;

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
    } fapi_nr_pucch_resource_t;

    typedef struct {
        uint8_t periodicity;
        uint8_t number_of_harq_process;
        fapi_nr_pucch_resource_t n1_pucch_an;
    } fapi_nr_sps_config_t;

    typedef struct {
        uint8_t beam_failure_instance_max_count;
        uint8_t beam_failure_detection_timer;
    } fapi_nr_radio_link_monitoring_config_t;

    typedef struct {
        fapi_nr_pdcch_config_dedicated_t pdcch_config_dedicated;
        fapi_nr_pdsch_config_dedicated_t pdsch_config_dedicated;
        fapi_nr_sps_config_t sps_config;
        fapi_nr_radio_link_monitoring_config_t radio_link_monitoring_config;

    } fapi_nr_dl_bwp_dedicated_config_t;

    typedef struct {
        fapi_nr_rach_config_common_t  rach_config_common;
        fapi_nr_pusch_config_common_t pusch_config_common;
        fapi_nr_pucch_config_common_t pucch_config_common;

    } fapi_nr_ul_bwp_common_config_t;
        
    typedef struct {
        uint8_t inter_slot_frequency_hopping;
        uint8_t additional_dmrs;
        uint8_t max_code_rate;
        uint8_t number_of_slots;
        uint8_t pi2bpsk;
        uint8_t simultaneous_harq_ack_csi;
    } fapi_nr_pucch_format_config_t;

    typedef struct {
        fapi_nr_pucch_format_config_t format1;
        fapi_nr_pucch_format_config_t format2;
        fapi_nr_pucch_format_config_t format3;
        fapi_nr_pucch_format_config_t format4;
        fapi_nr_pucch_resource_t multi_csi_pucch_resources[2];
        uint8_t dl_data_to_ul_ack[8];
        //  pucch power control
        uint8_t deltaF_pucch_f0;
        uint8_t deltaF_pucch_f1;
        uint8_t deltaF_pucch_f2;
        uint8_t deltaF_pucch_f3;
        uint8_t deltaF_pucch_f4;
        uint8_t two_pucch_pc_adjusment_states;
    } fapi_nr_pucch_config_dedicated_t;

    typedef struct {
        uint8_t dmrs_type;
        uint8_t dmrs_addition_position;
        
        uint8_t ptrs_type;  //cp-OFDM, dft-S-OFDM
        uint16_t ptrs_frequency_density[2];
        uint8_t ptrs_time_density[3];
        uint8_t ptrs_max_number_of_ports;
        uint8_t ptrs_resource_element_offset;
        uint8_t ptrs_power;
        uint16_t ptrs_sample_density[5];
        uint8_t ptrs_time_density_transform_precoding;

        uint8_t max_length;
        uint16_t scrambling_id0;
        uint16_t scrambling_id1;
        uint8_t npusch_identity;
        uint8_t disable_sequence_group_hopping;
        uint8_t sequence_hopping_enable;
    } fapi_nr_dmrs_uplink_config_t;

    typedef struct {
        uint8_t tpc_accmulation;
        uint8_t msg3_alpha;
        uint8_t p0_nominal_with_grant;
        uint8_t two_pusch_pc_adjustments_states;
        uint8_t delta_mcs;
    } fapi_nr_pusch_power_control_t;

    typedef struct {
        uint16_t data_scrambling_identity;
        uint8_t tx_config;
        fapi_nr_dmrs_uplink_config_t dmrs_ul_for_pusch_mapping_type_a;
        fapi_nr_dmrs_uplink_config_t dmrs_ul_for_pusch_mapping_type_b;
        fapi_nr_pusch_power_control_t pusch_power_control;
        uint8_t frequency_hopping;
        uint16_t frequency_hopping_offset_lists[4];
        uint8_t resource_allocation;
        fapi_nr_pusch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_UL_ALLOCATIONS];
        uint8_t pusch_aggregation_factor;
        uint8_t mcs_table;
        uint8_t mcs_table_transform_precoder;
        uint8_t transform_precoder;
        uint8_t codebook_subset;
        uint8_t max_rank;
        uint8_t rbg_size;

        //uci-OnPUSCH
        uint8_t uci_on_pusch_type;  //dynamic, semi-static
        uint8_t beta_offset_ack_index1[4];
        uint8_t beta_offset_ack_index2[4];
        uint8_t beta_offset_ack_index3[4];
        uint8_t beta_offset_csi_part1_index1[4];
        uint8_t beta_offset_csi_part1_index2[4];
        uint8_t beta_offset_csi_part2_index1[4];
        uint8_t beta_offset_csi_part2_index2[4];

        uint8_t tp_pi2BPSK;
    } fapi_nr_pusch_config_dedicated_t;

    typedef struct {
        uint8_t frequency_hopping;
        fapi_nr_dmrs_uplink_config_t cg_dmrs_configuration;
        uint8_t mcs_table;
        uint8_t mcs_table_transform_precoder;

        //uci-OnPUSCH
        uint8_t uci_on_pusch_type;  //dynamic, semi-static
        uint8_t beta_offset_ack_index1[4];
        uint8_t beta_offset_ack_index2[4];
        uint8_t beta_offset_ack_index3[4];
        uint8_t beta_offset_csi_part1_index1[4];
        uint8_t beta_offset_csi_part1_index2[4];
        uint8_t beta_offset_csi_part2_index1[4];
        uint8_t beta_offset_csi_part2_index2[4];

        uint8_t resource_allocation;
        //  rgb-Size structure missing in spec.
        uint8_t power_control_loop_to_use;
        //  p0-PUSCH-Alpha
        uint8_t p0;
        uint8_t alpha;

        uint8_t transform_precoder;
        uint8_t number_of_harq_process;
        uint8_t rep_k;
        uint8_t rep_k_rv;
        uint8_t periodicity;
        uint8_t configured_grant_timer;
        //  rrc-ConfiguredUplinkGrant
        uint16_t time_domain_offset;
        uint8_t time_domain_allocation;
        uint32_t frequency_domain_allocation;
        uint8_t antenna_ports;
        uint8_t dmrs_seq_initialization;
        uint8_t precoding_and_number_of_layers;
        uint8_t srs_resource_indicator;
        uint8_t mcs_and_tbs;
        uint8_t frequency_hopping_offset;
        uint8_t path_loss_reference_index;

    } fapi_nr_configured_grant_config_t;

    typedef struct {
        uint8_t qcl_type1_serving_cell_index;
        uint8_t qcl_type1_bwp_id;
        uint8_t qcl_type1_rs_type;  //  csi-rs or ssb
        uint8_t qcl_type1_nzp_csi_rs_resource_id;
        uint8_t qcl_type1_ssb_index;
        uint8_t qcl_type1_type;
        
        uint8_t qcl_type2_serving_cell_index;
        uint8_t qcl_type2_bwp_id;
        uint8_t qcl_type2_rs_type;  //  csi-rs or ssb
        uint8_t qcl_type2_nzp_csi_rs_resource_id;
        uint8_t qcl_type2_ssb_index;
        uint8_t qcl_type2_type;

    } fapi_nr_tci_state_t;

    typedef struct {
        uint8_t root_sequence_index;
        //  rach genertic
        uint8_t prach_configuration_index;
        uint8_t msg1_fdm;
        uint8_t msg1_frequency_start;
        uint8_t zero_correlation_zone_config;
        uint8_t preamble_received_target_power;
        uint8_t preamble_transmission_max;
        uint8_t power_ramping_step;
        uint8_t ra_window_size;

        uint8_t rsrp_threshold_ssb;
        //  PRACH-ResourceDedicatedBFR
        uint8_t bfr_ssb_index[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint8_t bfr_ssb_ra_preamble_index[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        // NZP-CSI-RS-Resource
        uint8_t bfr_csi_rs_nzp_resource_mapping[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint8_t bfr_csi_rs_power_control_offset[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint8_t bfr_csi_rs_power_control_offset_ss[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint16_t bfr_csi_rs_scrambling_id[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint8_t bfr_csi_rs_resource_periodicity[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint16_t bfr_csi_rs_resource_offset[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        fapi_nr_tci_state_t qcl_infomation_periodic_csi_rs[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];

        uint8_t bfr_csirs_ra_occasions[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
        uint8_t bfr_csirs_ra_preamble_index[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS][FAPI_NR_MAX_RA_OCCASION_PER_CSIRS];

        uint8_t ssb_per_rach_occasion;
        uint8_t ra_ssb_occasion_mask_index;
        fapi_nr_search_space_t recovery_search_space;
        //  RA-Prioritization
        uint8_t power_ramping_step_high_priority;
        uint8_t scaling_factor_bi;
        uint8_t beam_failure_recovery_timer;
    } fapi_nr_beam_failure_recovery_config_t;

    typedef struct {
        fapi_nr_pucch_config_dedicated_t pucch_config_dedicated;
        fapi_nr_pusch_config_dedicated_t pusch_config_dedicated;
        fapi_nr_configured_grant_config_t configured_grant_config;
        //  SRS-Config
        uint8_t srs_tpc_accumulation;
        fapi_nr_beam_failure_recovery_config_t beam_failure_recovery_config;
        
    } fapi_nr_ul_bwp_dedicated_config_t;

#define FAPI_NR_CONFIG_REQUEST_MASK_PBCH                0x01
#define FAPI_NR_CONFIG_REQUEST_MASK_DL_BWP_COMMON       0x02
#define FAPI_NR_CONFIG_REQUEST_MASK_UL_BWP_COMMON       0x04
#define FAPI_NR_CONFIG_REQUEST_MASK_DL_BWP_DEDICATED    0x08
#define FAPI_NR_CONFIG_REQUEST_MASK_UL_BWP_DEDICATED    0x10

typedef struct {
    uint32_t config_mask;

    fapi_nr_pbch_config_t pbch_config;  //  MIB

    fapi_nr_dl_bwp_common_config_t     dl_bwp_common;
    fapi_nr_dl_bwp_dedicated_config_t  dl_bwp_dedicated;

    fapi_nr_ul_bwp_common_config_t     ul_bwp_common;
    fapi_nr_ul_bwp_dedicated_config_t  ul_bwp_dedicated;

} fapi_nr_config_request_t;

#endif /* _FAPI_INTERFACE_H_ */
