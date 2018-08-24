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

/* \file ue_procedures.c
 * \brief procedures related to UE
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "mac_proto.h"
#include "mac_extern.h"
#include "RRC/NR_UE/rrc_proto.h"
#include "assertions.h"
#include "PHY/defs_nr_UE.h"

#include <stdio.h>
#include <math.h>

uint32_t get_ssb_slot(uint32_t ssb_index){
    //  this function now only support f <= 3GHz
    return ssb_index & 0x3 ;

    //  return first_symbol(case, freq, ssb_index) / 14
}

int8_t nr_ue_decode_mib(
	module_id_t module_id,
	int 		cc_id,
	uint8_t 	gNB_index,
	uint8_t 	extra_bits,	//	8bits 38.212 c7.1.1
	uint32_t    ssb_length,
	uint32_t 	ssb_index,
	void 		*pduP,
    uint16_t    cell_id ){

    printf("[L2][MAC] decode mib\n");

	NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    nr_mac_rrc_data_ind_ue( module_id, cc_id, gNB_index, NR_BCCH_BCH, (uint8_t *) pduP, 3 );    //  fixed 3 bytes MIB PDU
    
    AssertFatal(mac->mib != NULL, "nr_ue_decode_mib() mac->mib == NULL\n");
    //if(mac->mib != NULL){
	    uint32_t frame = (mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused);
	    uint32_t frame_number_4lsb = (uint32_t)(extra_bits & 0xf);                      //	extra bits[0:3]
	    uint32_t half_frame_bit = (uint32_t)(( extra_bits >> 4 ) & 0x1 );               //	extra bits[4]
	    uint32_t ssb_subcarrier_offset_msb = (uint32_t)(( extra_bits >> 5 ) & 0x1 );    //	extra bits[5]
	    
	    uint32_t ssb_subcarrier_offset = mac->mib->ssb_SubcarrierOffset;

	    //uint32_t ssb_index = 0;    //  TODO: ssb_index should obtain from L1 in case Lssb != 64

	    frame = frame << 4;
	    frame = frame | frame_number_4lsb;

	    if(ssb_length == 64){
	    	ssb_index = ssb_index & (( extra_bits >> 2 ) & 0x1C );    //	{ extra_bits[5:7], ssb_index[2:0] }
	    }else{
			if(ssb_subcarrier_offset_msb){
			    ssb_subcarrier_offset = ssb_subcarrier_offset | 0x10;
			}
	    }

#ifdef DEBUG_MIB
		printf("system frame number(6 MSB bits): %d\n",  mac->mib->systemFrameNumber.buf[0]);
		printf("system frame number(with LSB): %d\n", (int)frame);
		printf("subcarrier spacing:            %d\n", (int)mac->mib->subCarrierSpacingCommon);
		printf("ssb carrier offset(with MSB):  %d\n", (int)ssb_subcarrier_offset);
		printf("dmrs type A position:          %d\n", (int)mac->mib->dmrs_TypeA_Position);
		printf("pdcch config sib1:             %d\n", (int)mac->mib->pdcch_ConfigSIB1);
		printf("cell barred:                   %d\n", (int)mac->mib->cellBarred);
		printf("intra frequcney reselection:   %d\n", (int)mac->mib->intraFreqReselection);
		printf("half frame bit(extra bits):    %d\n", (int)half_frame_bit);
		printf("ssb index(extra bits):         %d\n", (int)ssb_index);
#endif

	    subcarrier_spacing_t scs_ssb = scs_30kHz;      //  default for 
        //const uint32_t scs_index = 0;
        const uint32_t num_slot_per_frame = 20;
	    subcarrier_spacing_t scs_pdcch;

        //  assume carrier frequency < 6GHz
        if(mac->mib->subCarrierSpacingCommon == NR_MIB__subCarrierSpacingCommon_scs15or60){
            scs_pdcch = scs_15kHz;
        }else{  //NR_MIB__subCarrierSpacingCommon_scs30or120
            scs_pdcch = scs_30kHz;
        }


	    channel_bandwidth_t min_channel_bw = bw_40MHz;  //  deafult for testing
	    
        uint32_t is_condition_A = (ssb_subcarrier_offset == 0);   //  38.213 ch.13
        frequency_range_t frequency_range = FR1;
        uint32_t index_4msb = (mac->mib->pdcch_ConfigSIB1 >> 4) & 0xf;
        uint32_t index_4lsb = (mac->mib->pdcch_ConfigSIB1 & 0xf);
        int32_t num_rbs = -1;
        int32_t num_symbols = -1;
        int32_t rb_offset = -1;

        //  type0-pdcch coreset
	    switch( (scs_ssb << 5)|scs_pdcch ){
            case (scs_15kHz << 5) | scs_15kHz :
                AssertFatal(index_4msb < 15, "38.213 Table 13-1 4 MSB out of range\n");
                mac->type0_pdcch_ss_mux_pattern = 1;
                num_rbs     = table_38213_13_1_c2[index_4msb];
                num_symbols = table_38213_13_1_c3[index_4msb];
                rb_offset   = table_38213_13_1_c4[index_4msb];
                break;

	        case (scs_15kHz << 5) | scs_30kHz:
                AssertFatal(index_4msb < 14, "38.213 Table 13-2 4 MSB out of range\n");
                mac->type0_pdcch_ss_mux_pattern = 1;
                num_rbs     = table_38213_13_2_c2[index_4msb];
                num_symbols = table_38213_13_2_c3[index_4msb];
                rb_offset   = table_38213_13_2_c4[index_4msb];
                break;

            case (scs_30kHz << 5) | scs_15kHz:
                if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
                    AssertFatal(index_4msb < 9, "38.213 Table 13-3 4 MSB out of range\n");
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_3_c2[index_4msb];
                    num_symbols = table_38213_13_3_c3[index_4msb];
                    rb_offset   = table_38213_13_3_c4[index_4msb];
                }else if(min_channel_bw & bw_40MHz){
                    AssertFatal(index_4msb < 9, "38.213 Table 13-5 4 MSB out of range\n");
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_5_c2[index_4msb];
                    num_symbols = table_38213_13_5_c3[index_4msb];
                    rb_offset   = table_38213_13_5_c4[index_4msb];
                }else{ ; }

                break;

            case (scs_30kHz << 5) | scs_30kHz:
                if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_4_c2[index_4msb];
                    num_symbols = table_38213_13_4_c3[index_4msb];
                    rb_offset   = table_38213_13_4_c4[index_4msb];
                }else if(min_channel_bw & bw_40MHz){
                    AssertFatal(index_4msb < 10, "38.213 Table 13-6 4 MSB out of range\n");
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_6_c2[index_4msb];
                    num_symbols = table_38213_13_6_c3[index_4msb];
                    rb_offset   = table_38213_13_6_c4[index_4msb];
                }else{ ; }
                break;

            case (scs_120kHz << 5) | scs_60kHz:
                AssertFatal(index_4msb < 12, "38.213 Table 13-7 4 MSB out of range\n");
                if(index_4msb & 0x7){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                }else if(index_4msb & 0x18){
                    mac->type0_pdcch_ss_mux_pattern = 2;
                }else{ ; }

                num_rbs     = table_38213_13_7_c2[index_4msb];
                num_symbols = table_38213_13_7_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 8 || index_4msb == 10)){
                    rb_offset   = table_38213_13_7_c4[index_4msb] - 1;
                }else{
                    rb_offset   = table_38213_13_7_c4[index_4msb];
                }
                break;

            case (scs_120kHz << 5) | scs_120kHz:
                AssertFatal(index_4msb < 8, "38.213 Table 13-8 4 MSB out of range\n");
                if(index_4msb & 0x3){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                }else if(index_4msb & 0x0c){
                    mac->type0_pdcch_ss_mux_pattern = 3;
                }

                num_rbs     = table_38213_13_8_c2[index_4msb];
                num_symbols = table_38213_13_8_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
                    rb_offset   = table_38213_13_8_c4[index_4msb] - 1;
                }else{
                    rb_offset   = table_38213_13_8_c4[index_4msb];
                }
                break;

            case (scs_240kHz << 5) | scs_60kHz:
                AssertFatal(index_4msb < 4, "38.213 Table 13-9 4 MSB out of range\n");
                mac->type0_pdcch_ss_mux_pattern = 1;
                num_rbs     = table_38213_13_9_c2[index_4msb];
                num_symbols = table_38213_13_9_c3[index_4msb];
                rb_offset   = table_38213_13_9_c4[index_4msb];
                break;

            case (scs_240kHz << 5) | scs_120kHz:
                AssertFatal(index_4msb < 8, "38.213 Table 13-10 4 MSB out of range\n");
                if(index_4msb & 0x3){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                }else if(index_4msb & 0x0c){
                    mac->type0_pdcch_ss_mux_pattern = 2;
                }
                num_rbs     = table_38213_13_10_c2[index_4msb];
                num_symbols = table_38213_13_10_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
                    rb_offset   = table_38213_13_10_c4[index_4msb]-1;
                }else{
                    rb_offset   = table_38213_13_10_c4[index_4msb];
                }
                
                break;

	        default:
	            break;
	    }

        AssertFatal(num_rbs != -1, "Type0 PDCCH coreset num_rbs undefined");
        AssertFatal(num_symbols != -1, "Type0 PDCCH coreset num_symbols undefined");
        AssertFatal(rb_offset != -1, "Type0 PDCCH coreset rb_offset undefined");
        
        //uint32_t cell_id = 0;   //  obtain from L1 later

        //mac->type0_pdcch_dci_config.coreset.rb_start = rb_offset;
        //mac->type0_pdcch_dci_config.coreset.rb_end = rb_offset + num_rbs - 1;
        uint64_t mask = 0x0;
        uint8_t i;
        for(i=0; i<(num_rbs/6); ++i){   //  38.331 Each bit corresponds a group of 6 RBs
            mask = mask >> 1;
            mask = mask | 0x100000000000;
        }
        mac->type0_pdcch_dci_config.coreset.frequency_domain_resource = mask;
        mac->type0_pdcch_dci_config.coreset.rb_offset = rb_offset;  //  additional parameter other than coreset

        //mac->type0_pdcch_dci_config.type0_pdcch_coreset.duration = num_symbols;
        mac->type0_pdcch_dci_config.coreset.cce_reg_mapping_type = CCE_REG_MAPPING_TYPE_INTERLEAVED;
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_reg_bundle_size = 6;   //  L 38.211 7.3.2.2
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_interleaver_size = 2;  //  R 38.211 7.3.2.2
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_shift_index = cell_id;
        mac->type0_pdcch_dci_config.coreset.precoder_granularity = PRECODER_GRANULARITY_SAME_AS_REG_BUNDLE;
        mac->type0_pdcch_dci_config.coreset.pdcch_dmrs_scrambling_id = cell_id;



        // type0-pdcch search space
        float big_o;
        float big_m;
        uint32_t temp;
        SFN_C_TYPE sfn_c;   //  only valid for mux=1
        uint32_t n_c;
        uint32_t number_of_search_space_per_slot;
        uint32_t first_symbol_index;
        uint32_t search_space_duration;  //  element of search space
        uint32_t coreset_duration;  //  element of coreset
        
        //  38.213 table 10.1-1

        /// MUX PATTERN 1
        if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR1){
            big_o = table_38213_13_11_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_11_c3[index_4lsb];

            temp = (uint32_t)(big_o*pow(2, scs_pdcch)) + (uint32_t)(ssb_index*big_m);
            n_c = temp / num_slot_per_frame;
            if((temp/num_slot_per_frame) & 0x1){
                sfn_c = SFN_C_MOD_2_EQ_1;
            }else{
                sfn_c = SFN_C_MOD_2_EQ_0;
            }

            if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 7) && (ssb_index&1)){
                first_symbol_index = num_symbols;
            }else{
                first_symbol_index = table_38213_13_11_c4[index_4lsb];
            }
            //  38.213 chapter 13: over two consecutive slots
            search_space_duration = 2;
        }

        if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR2){
            big_o = table_38213_13_12_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_12_c3[index_4lsb];

            if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 10) && (ssb_index&1)){
                first_symbol_index = 7;
            }else if((index_4lsb == 6 || index_4lsb == 7 || index_4lsb == 8 || index_4lsb == 11) && (ssb_index&1)){
                first_symbol_index = num_symbols;
            }else{
                first_symbol_index = 0;
            }
            //  38.213 chapter 13: over two consecutive slots
            search_space_duration = 2;
        }

        /// MUX PATTERN 2
        if(mac->type0_pdcch_ss_mux_pattern == 2){
            
            if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_60kHz)){
                //  38.213 Table 13-13
                AssertFatal(index_4lsb == 0, "38.213 Table 13-13 4 LSB out of range\n");
                //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
//                sfn_c = SFN_C_EQ_SFN_SSB;
                n_c = get_ssb_slot(ssb_index);
                switch(ssb_index & 0x3){    //  ssb_index(i) mod 4
                    case 0: 
                        first_symbol_index = 0;
                        break;
                    case 1: 
                        first_symbol_index = 1;
                        break;
                    case 2: 
                        first_symbol_index = 6;
                        break;
                    case 3: 
                        first_symbol_index = 7;
                        break;
                    default: break; 
                }
                
            }else if((scs_ssb == scs_240kHz) && (scs_pdcch == scs_120kHz)){
                //  38.213 Table 13-14
                AssertFatal(index_4lsb == 0, "38.213 Table 13-14 4 LSB out of range\n");
                //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
//                sfn_c = SFN_C_EQ_SFN_SSB;
                n_c = get_ssb_slot(ssb_index);
                switch(ssb_index & 0x7){    //  ssb_index(i) mod 8
                    case 0: 
                        first_symbol_index = 0;
                        break;
                    case 1: 
                        first_symbol_index = 1;
                        break;
                    case 2: 
                        first_symbol_index = 2;
                        break;
                    case 3: 
                        first_symbol_index = 3;
                        break;
                    case 4: 
                        first_symbol_index = 12;
                        n_c = get_ssb_slot(ssb_index) - 1;
                        break;
                    case 5: 
                        first_symbol_index = 13;
                        n_c = get_ssb_slot(ssb_index) - 1;
                        break;
                    case 6: 
                        first_symbol_index = 0;
                        break;
                    case 7: 
                        first_symbol_index = 1;
                        break;
                    default: break; 
                }
            }else{ ; }
            //  38.213 chapter 13: over one slot
            search_space_duration = 1;
        }

        /// MUX PATTERN 3
        if(mac->type0_pdcch_ss_mux_pattern == 3){
            if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_120kHz)){
                //  38.213 Table 13-15
                AssertFatal(index_4lsb == 0, "38.213 Table 13-15 4 LSB out of range\n");
                //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
//                sfn_c = SFN_C_EQ_SFN_SSB;
                n_c = get_ssb_slot(ssb_index);
                switch(ssb_index & 0x3){    //  ssb_index(i) mod 4
                    case 0: 
                        first_symbol_index = 4;
                        break;
                    case 1: 
                        first_symbol_index = 8;
                        break;
                    case 2: 
                        first_symbol_index = 2;
                        break;
                    case 3: 
                        first_symbol_index = 6;
                        break;
                    default: break; 
                }
            }else{ ; }
            //  38.213 chapter 13: over one slot
            search_space_duration = 1;
        }

        coreset_duration = num_symbols * number_of_search_space_per_slot;

        mac->type0_pdcch_dci_config.number_of_candidates[0] = table_38213_10_1_1_c2[0];
        mac->type0_pdcch_dci_config.number_of_candidates[1] = table_38213_10_1_1_c2[1];
        mac->type0_pdcch_dci_config.number_of_candidates[2] = table_38213_10_1_1_c2[2];   //  CCE aggregation level = 4
        mac->type0_pdcch_dci_config.number_of_candidates[3] = table_38213_10_1_1_c2[3];   //  CCE aggregation level = 8
        mac->type0_pdcch_dci_config.number_of_candidates[4] = table_38213_10_1_1_c2[4];   //  CCE aggregation level = 16
        mac->type0_pdcch_dci_config.duration = search_space_duration;
        mac->type0_pdcch_dci_config.coreset.duration = coreset_duration;   //  coreset
        mac->type0_pdcch_dci_config.monitoring_symbols_within_slot = (0x3fff << first_symbol_index) & (0x3fff >> (14-coreset_duration-first_symbol_index)) & 0x3fff;

        mac->type0_pdcch_ss_sfn_c = sfn_c;
        mac->type0_pdcch_ss_n_c = n_c;
        
	    // fill in the elements in config request inside P5 message
	    mac->phy_config.config_req.pbch_config.system_frame_number = frame;    //  after calculation
	    mac->phy_config.config_req.pbch_config.subcarrier_spacing_common = mac->mib->subCarrierSpacingCommon;
	    mac->phy_config.config_req.pbch_config.ssb_subcarrier_offset = ssb_subcarrier_offset;  //  after calculation
	    mac->phy_config.config_req.pbch_config.dmrs_type_a_position = mac->mib->dmrs_TypeA_Position;
	    mac->phy_config.config_req.pbch_config.pdcch_config_sib1 = mac->mib->pdcch_ConfigSIB1;
	    mac->phy_config.config_req.pbch_config.cell_barred = mac->mib->cellBarred;
	    mac->phy_config.config_req.pbch_config.intra_frequency_reselection = mac->mib->intraFreqReselection;
	    mac->phy_config.config_req.pbch_config.half_frame_bit = half_frame_bit;
	    mac->phy_config.config_req.pbch_config.ssb_index = ssb_index;
	    mac->phy_config.config_req.config_mask |= FAPI_NR_CONFIG_REQUEST_MASK_PBCH;

	    if(mac->if_module != NULL && mac->if_module->phy_config_request != NULL){
		mac->if_module->phy_config_request(&mac->phy_config);
	    }
    //}
    return 0;
}



//  TODO: change to UE parameter, scs: 15KHz, slot duration: 1ms
uint32_t get_ssb_frame(uint32_t test){
	return test;
}

// Performs :
// 1. TODO: Call RRC for link status return to PHY
// 2. TODO: Perform SR/BSR procedures for scheduling feedback
// 3. TODO: Perform PHR procedures
NR_UE_L2_STATE_t nr_ue_scheduler(
    const module_id_t module_id,
    const uint8_t gNB_index,
    const int cc_id,
    const frame_t rx_frame,
    const slot_t rx_slot,
    const int32_t ssb_index,
    const frame_t tx_frame,
    const slot_t tx_slot ){

    uint32_t search_space_mask = 0;
    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    
    //  check type0 from 38.213 13
    if(ssb_index != -1){

        if(mac->type0_pdcch_ss_mux_pattern == 1){
            //	38.213 chapter 13
            if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_0) && !(rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            	search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
            if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_1) &&  (rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            	search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 2){
            //	38.213 Table 13-13, 13-14
            if((rx_frame == get_ssb_frame(rx_frame)) && (rx_slot == mac->type0_pdcch_ss_n_c)){
                search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 3){
        	//	38.213 Table 13-15
            if((rx_frame == get_ssb_frame(rx_frame)) && (rx_slot == mac->type0_pdcch_ss_n_c)){
                search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
        }
    }

    fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    //  Type0 PDCCH search space
    if((search_space_mask & type0_pdcch) || ( mac->type0_pdcch_consecutive_slots != 0 )){
        mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_consecutive_slots - 1;

        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15 = mac->type0_pdcch_dci_config;
        dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
        dl_config->number_pdus = dl_config->number_pdus + 1;
    	
    	dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.rnti = 0xaaaa;	//	to be set
    }

    if(search_space_mask & type0a_pdcch){
    }

    if(search_space_mask & type1_pdcch){
    }

    if(search_space_mask & type2_pdcch){
    }

    if(search_space_mask & type3_pdcch){
    }


    mac->scheduled_response.dl_config = dl_config;
    

	return CONNECTION_OK;
}

int8_t nr_ue_process_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_format){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request;
    const uint16_t n_RB_ULBWP = 106;
    const uint16_t n_RB_DLBWP = 106;

    uint32_t k_offset;
    uint32_t sliv_S;
    uint32_t sliv_L;

    uint32_t l_RB;
    uint32_t start_RB;
    uint32_t tmp_RIV;

    switch(dci_format){
        case format0_0:
            /* TIME_DOM_RESOURCE_ASSIGNMENT */
            // 0, 1, 2, 3, or 4 bits as defined in:
            //         Subclause 6.1.2.1 of [6, TS 38.214] for formats format0_0,format0_1
            //         Subclause 5.1.2.1 of [6, TS 38.214] for formats format1_0,format1_1
            // The bitwidth for this field is determined as log2(I) bits,
            // where I the number of entries in the higher layer parameter pusch-AllocationList
            k_offset = table_6_1_2_1_1_2_time_dom_res_alloc_A[dci->time_dom_resource_assignment][0];
            sliv_S   = table_6_1_2_1_1_2_time_dom_res_alloc_A[dci->time_dom_resource_assignment][1];
            sliv_L   = table_6_1_2_1_1_2_time_dom_res_alloc_A[dci->time_dom_resource_assignment][2];

            /* FREQ_DOM_RESOURCE_ASSIGNMENT_UL */
            // At the moment we are supporting only format 1_0 (and not format 1_1), so we only support resource allocation type 1 (and not type 0).
            // For resource allocation type 1, the resource allocation field consists of a resource indication value (RIV):
            // RIV = n_RB_ULBWP * (l_RB - 1) + start_RB                                  if (l_RB - 1) <= floor (n_RB_ULBWP/2)
            // RIV = n_RB_ULBWP * (n_RB_ULBWP - l_RB + 1) + (n_RB_ULBWP - 1 - start_RB)  if (l_RB - 1)  > floor (n_RB_ULBWP/2)
            // the following two expressions apply only if (l_RB - 1) <= floor (n_RB_ULBWP/2)
            l_RB = floor(dci->freq_dom_resource_assignment_DL/n_RB_ULBWP) + 1;
            start_RB = dci->freq_dom_resource_assignment_DL%n_RB_ULBWP;
            // if (l_RB - 1)  > floor (n_RB_ULBWP/2) we need to recalculate them using the following lines
            tmp_RIV = n_RB_ULBWP * (l_RB - 1) + start_RB;
            if (tmp_RIV != dci->freq_dom_resource_assignment_DL) { // then (l_RB - 1)  > floor (n_RB_ULBWP/2) and we need to recalculate l_RB and start_RB
                l_RB = n_RB_ULBWP - l_RB + 2;
                start_RB = n_RB_ULBWP - start_RB - 1;
            }

            //  UL_CONFIG_REQ
            ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
            ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.rnti = rnti;
            fapi_nr_ul_config_pusch_pdu_rel15_t *ulsch_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15;
            ulsch_config_pdu->number_rbs = l_RB;
            ulsch_config_pdu->start_rb = start_RB;
            ulsch_config_pdu->number_symbols = sliv_L;
            ulsch_config_pdu->start_symbol = sliv_S;
            ulsch_config_pdu->mcs = dci->mcs;
            
            //ulsch0->harq_processes[dci->harq_process_number]->first_rb = start_RB;
            //ulsch0->harq_processes[dci->harq_process_number]->nb_rb    = l_RB;
            //ulsch0->harq_processes[dci->harq_process_number]->mcs = dci->mcs;
            //ulsch0->harq_processes[nr_pdci_info_extracted->harq_process_number]->DCINdi= nr_pdci_info_extracted->ndi;
            break;

        case format0_1:
            break;

        case format1_0: 

            /* TIME_DOM_RESOURCE_ASSIGNMENT */
            // Subclause 5.1.2.1 of [6, TS 38.214]
            // the Time domain resource assignment field of the DCI provides a row index of a higher layer configured table pdsch-symbolAllocation
            // FIXME! To clarify which parameters to update after reception of row index
            k_offset = table_5_1_2_1_1_2_time_dom_res_alloc_A[dci->time_dom_resource_assignment][0];
            sliv_S   = table_5_1_2_1_1_2_time_dom_res_alloc_A[dci->time_dom_resource_assignment][1];
            sliv_L   = table_5_1_2_1_1_2_time_dom_res_alloc_A[dci->time_dom_resource_assignment][2];


            /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
            // only uplink resource allocation type 1
            // At the moment we are supporting only format 0_0 (and not format 0_1), so we only support resource allocation type 1 (and not type 0).
            // For resource allocation type 1, the resource allocation field consists of a resource indication value (RIV):
            // RIV = n_RB_DLBWP * (l_RB - 1) + start_RB                                  if (l_RB - 1) <= floor (n_RB_DLBWP/2)
            // RIV = n_RB_DLBWP * (n_RB_DLBWP - l_RB + 1) + (n_RB_DLBWP - 1 - start_RB)  if (l_RB - 1)  > floor (n_RB_DLBWP/2)
            // the following two expressions apply only if (l_RB - 1) <= floor (n_RB_DLBWP/2)
            
            l_RB = floor(dci->freq_dom_resource_assignment_DL/n_RB_DLBWP) + 1;
            start_RB = dci->freq_dom_resource_assignment_DL%n_RB_DLBWP;
            // if (l_RB - 1)  > floor (n_RB_DLBWP/2) we need to recalculate them using the following lines
            tmp_RIV = n_RB_DLBWP * (l_RB - 1) + start_RB;
            if (tmp_RIV != dci->freq_dom_resource_assignment_DL) { // then (l_RB - 1)  > floor (n_RB_DLBWP/2) and we need to recalculate l_RB and start_RB
                l_RB = n_RB_DLBWP - l_RB + 2;
                start_RB = n_RB_DLBWP - start_RB - 1;
            }
            

            //  DL_CONFIG_REQ
            dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
            dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = rnti;
            fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
            dlsch_config_pdu->number_rbs = l_RB;
            dlsch_config_pdu->start_rb = start_RB;
            dlsch_config_pdu->number_symbols = sliv_L;
            dlsch_config_pdu->start_symbol = sliv_S;
            dlsch_config_pdu->mcs = dci->mcs;
            dlsch_config_pdu->ndi = dci->ndi;
            dlsch_config_pdu->harq_pid = dci->harq_process_number;

            dl_config->number_pdus = dl_config->number_pdus + 1;
            break;

        case format1_1:        
            break;

        case format2_0:        
            break;

        case format2_1:        
            break;

        case format2_2:        
            break;

        case format2_3:
            break;

        default: 
            break;
    }



    if(rnti == SI_RNTI){

    }else if(rnti == mac->ra_rnti){

    }else if(rnti == P_RNTI){

    }else{  //  c-rnti

        ///  check if this is pdcch order 
        //dci->random_access_preamble_index;
        //dci->ss_pbch_index;
        //dci->prach_mask_index;

        ///  else normal DL-SCH grant
    }
}

int8_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe){

    return 0;
}



void nr_ue_process_mac_pdu(
    module_id_t module_idP,
    uint8_t CC_id,
    uint8_t *pduP, 
    uint16_t mac_pdu_len, 
    uint8_t eNB_index){

    uint8_t *pdu_ptr = pduP;
    uint16_t pdu_len = mac_pdu_len;

    uint16_t mac_ce_len;
    uint16_t mac_subheader_len;
    uint16_t mac_sdu_len;

    //  For both DL/UL-SCH
    //  Except:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |       L       |

    //  For both DL/UL-SCH
    //  For:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding, for single/multiple 1-oct padding CE(s)
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|R|   LCID    |
    //  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
    //  L: The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
    //  F: lenght of L is 0:8 or 1:16 bits wide
    //  R: Reserved bit, set to zero.

    uint8_t done = 0;
    
    while (!done || pdu_len <= 0){
        mac_ce_len = 0x0000;
        mac_subheader_len = 0x0001; //  default to fixed-length subheader = 1-oct
        mac_sdu_len = 0x0000;
        switch(((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID){
            //  MAC CE
            case DL_SCH_LCID_CCCH:
                //  MSG4 RRC Connection Setup 38.331
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH:
                //  38.321 Ch6.1.3.14
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL:
                //  38.321 Ch6.1.3.13
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT:
                //  38.321 Ch6.1.3.12
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_SRS_ACTIVATION:
                //  38.321 Ch6.1.3.17
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            
            case DL_SCH_LCID_RECOMMENDED_BITRATE:
                //  38.321 Ch6.1.3.20
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_SP_ZP_CSI_RS_RES_SET_ACT:
                //  38.321 Ch6.1.3.19
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_PUCCH_SPATIAL_RELATION_ACT:
                //  38.321 Ch6.1.3.18
                mac_ce_len = 3;
                break;
            case DL_SCH_LCID_SP_CSI_REP_PUCCH_ACT:
                //  38.321 Ch6.1.3.16
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_TCI_STATE_IND_UE_SPEC_PDCCH:
                //  38.321 Ch6.1.3.15
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_DUPLICATION_ACT:
                //  38.321 Ch6.1.3.11
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_SCell_ACT_4_OCT:
                //  38.321 Ch6.1.3.10
                mac_ce_len = 4;
                break;
            case DL_SCH_LCID_SCell_ACT_1_OCT:
                //  38.321 Ch6.1.3.10
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_L_DRX:
                //  38.321 Ch6.1.3.6
                //  fixed length but not yet specify.
                mac_ce_len = 0;
                break;
            case DL_SCH_LCID_DRX:
                //  38.321 Ch6.1.3.5
                //  fixed length but not yet specify.
                mac_ce_len = 0;
                break;
            case DL_SCH_LCID_TA_COMMAND:
                //  38.321 Ch6.1.3.4
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_CON_RES_ID:
                //  38.321 Ch6.1.3.3
                mac_ce_len = 6;
                break;
            case DL_SCH_LCID_PADDING:
                done = 1;
                //  end of MAC PDU, can ignore the rest.
                break;

            //  MAC SDU
            case DL_SCH_LCID_SRB1:
                //  check if LCID is valid at current time.
            case UL_SCH_LCID_SRB2:
                //  check if LCID is valid at current time.
            case UL_SCH_LCID_SRB3:
                //  check if LCID is valid at current time.
            default:
                //  check if LCID is valid at current time.
                mac_sdu_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_sdu_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                //  DRB LCID by RRC
                break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        AssertFatal(pdu_len >= 0, "[MAC] nr_ue_process_mac_pdu, residual mac pdu length < 0!\n");
    }
}
