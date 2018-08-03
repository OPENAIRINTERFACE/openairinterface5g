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
	void 		*pduP ){

    printf("[L2][MAC] decode mib\n");

	NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    nr_mac_rrc_data_ind_ue( module_id, cc_id, gNB_index,
		     NR_BCCH_BCH, (uint8_t *) pduP, 3 );
    

    if(mac->mib != NULL){
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

#if 0
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

	    subcarrier_spacing_t scs_ssb = scs_15kHz;      //  default for testing
	    subcarrier_spacing_t scs_pdcch = scs_15kHz;    //  default for testing
	    channel_bandwidth_t min_channel_bw = bw_5MHz;  //  deafult for testing
	    
        uint32_t is_condition_A = 1;
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
        
        uint32_t cell_id = 0;   //  obtain from L1 later

        mac->type0_pdcch_dci_config.coreset.rb_start = rb_offset;
        mac->type0_pdcch_dci_config.coreset.rb_end = rb_offset + num_rbs - 1;
        //mac->type0_pdcch_dci_config.type0_pdcch_coreset.duration = num_symbols;
        mac->type0_pdcch_dci_config.coreset.cce_reg_mapping_type = CCE_REG_MAPPING_TYPE_INTERLEAVED;
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_reg_bundle_size = 6;   //  L
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_interleaver_size = 2;  //  R
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_shift_index = cell_id;
        mac->type0_pdcch_dci_config.coreset.precoder_granularity = PRECODER_GRANULARITY_SAME_AS_REG_BUNDLE;
        mac->type0_pdcch_dci_config.coreset.pdcch_dmrs_scrambling_id = cell_id;



        // type0-pdcch search space
        float big_o;
        float big_m;
        uint32_t temp;
        SFN_C_TYPE sfn_c;
        uint32_t n_c;
        uint32_t number_of_search_space_per_slot;
        uint32_t first_symbol_index;
        uint32_t search_space_duration;  //  element of search space
        uint32_t coreset_duration;  //  element of coreset
const uint32_t scs_index = 0;
const uint32_t num_slot_per_frame = 10;
        
        //  38.213 table 10.1-1
        


        /// MUX PATTERN 1
        if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR1){
            big_o = table_38213_13_11_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_11_c3[index_4lsb];

            temp = (uint32_t)(big_o*pow(2, scs_index)) + (uint32_t)(ssb_index*big_m);
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
                sfn_c = SFN_C_EQ_SFN_SSB;
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
                sfn_c = SFN_C_EQ_SFN_SSB;
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
                sfn_c = SFN_C_EQ_SFN_SSB;
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

        mac->type0_pdcch_dci_config.number_of_candidates[2] = table_38213_10_1_1_c2[0];   //  CCE aggregation level = 4
        mac->type0_pdcch_dci_config.number_of_candidates[3] = table_38213_10_1_1_c2[1];   //  CCE aggregation level = 8
        mac->type0_pdcch_dci_config.number_of_candidates[4] = table_38213_10_1_1_c2[2];   //  CCE aggregation level = 16
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
    }
    return 0;
}



//  TODO: change to UE parameter, scs: 15KHz, slot duration: 1ms


uint32_t get_ssb_frame(){
	return 0;
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
            }
            if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_1) &&  (rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            	search_space_mask = search_space_mask | type0_pdcch;
            }
            if((mac->type0_pdcch_ss_sfn_c == SFN_C_EQ_SFN_SSB) && ( get_ssb_frame() )){
            	search_space_mask = search_space_mask | type0_pdcch;
            }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 2){
            //	38.213 Table 13-13, 13-14
            if((rx_frame == get_ssb_frame()) && (rx_slot == mac->type0_pdcch_ss_n_c)){
                search_space_mask = search_space_mask | type0_pdcch;
            }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 3){
        	//	38.213 Table 13-15
            if((rx_frame == get_ssb_frame()) && (rx_slot == mac->type0_pdcch_ss_n_c)){
                search_space_mask = search_space_mask | type0_pdcch;
            }
        }
    }

#if 0
		uint16_t rnti;

        fapi_nr_coreset_t coreset;
        uint32_t duration;
        uint8_t aggregation_level;
        uint8_t number_of_candidates;
        uint16_t monitoring_symbols_within_slot;
        //  DCI foramt-specific
        uint8_t format_2_0_number_of_candidates[5];    //  aggregation level 1, 2, 4, 8, 16
        uint8_t format_2_3_monitorying_periodicity;
        uint8_t format_2_3_number_of_candidates;
#endif
    fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    if(search_space_mask & type0_pdcch){

        dl_config->dl_config_request_body[dl_config->number_pdus].dci_pdu.dci_config_rel15 = mac->type0_pdcch_dci_config;
        dl_config->dl_config_request_body[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
        dl_config->number_pdus = dl_config->number_pdus + 1;
    	
    	dl_config->dl_config_request_body[dl_config->number_pdus].dci_pdu.dci_config_rel15.rnti = 0xaaaa;	//	to be set
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

int8_t nr_ue_decode_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_type){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    if(rnti == SI_RNTI){

    }else if(rnti == mac->ra_rnti){

    }
}

int8_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe)
{

    return 0;
}