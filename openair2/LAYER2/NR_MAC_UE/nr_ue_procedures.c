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

int8_t nr_ue_decode_mib(
	module_id_t module_id,
	int 		cc_id,
	uint8_t 	gNB_index,
	uint8_t 	extra_bits,	//	8bits 38.212 c7.1.1
	uint32_t    l_ssb_equal_64,
	//uint32_t 	*ssb_index,	//	from decoded MIB
	//uint32_t 	*frameP,	//	10 bits = 6(in decoded MIB)+4(in extra bits from L1)
	void 		*pduP,		//	encoded MIB
	uint16_t 	pdu_len){

    printf("[L2][MAC] decode mib\n");

	NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    nr_mac_rrc_data_ind_ue( module_id, cc_id, gNB_index,
		     NR_BCCH_BCH, (uint8_t *) pduP, pdu_len );
    

    if(mac->mib != NULL){
	    uint32_t frame = (mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused);
	    uint32_t frame_number_4lsb = (uint32_t)(extra_bits & 0xf);                      //	extra bits[0:3]
	    uint32_t half_frame_bit = (uint32_t)(( extra_bits >> 4 ) & 0x1 );               //	extra bits[4]
	    uint32_t ssb_subcarrier_offset_msb = (uint32_t)(( extra_bits >> 5 ) & 0x1 );    //	extra bits[5]
	    
	    uint32_t ssb_subcarrier_offset = mac->mib->ssb_SubcarrierOffset;

	    uint32_t ssb_index = 0;

	    frame = frame << 4;
	    frame = frame | frame_number_4lsb;

	    if(l_ssb_equal_64){
	    	ssb_index = (( extra_bits >> 5 ) & 0x7 );                                   //	extra bits[5:7]
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
	    uint32_t ssb_coreset_multiplexing_pattern;
        uint32_t is_condition_A = 1;
        frequency_range_t frequency_range = FR1;
        uint32_t index_4msb = (mac->mib->pdcch_ConfigSIB1 >> 4) & 0xf;
        uint32_t index_4lsb = (mac->mib->pdcch_ConfigSIB1 & 0xf);

        //  type0-pdcch coreset
	    switch( (scs_ssb << 5)|scs_pdcch ){
            case (scs_15kHz << 5) | scs_15kHz :
                AssertFatal(index_4msb < 15, "38.213 Table 13-1 4 MSB out of range\n");
                ssb_coreset_multiplexing_pattern = 1;
                mac->num_rbs     = table_38213_13_1_c2[index_4msb];
                mac->num_symbols = table_38213_13_1_c3[index_4msb];
                mac->rb_offset   = table_38213_13_1_c4[index_4msb];
                break;

	        case (scs_15kHz << 5) | scs_30kHz:
                AssertFatal(index_4msb < 14, "38.213 Table 13-2 4 MSB out of range\n");
                ssb_coreset_multiplexing_pattern = 1;
                mac->num_rbs     = table_38213_13_2_c2[index_4msb];
                mac->num_symbols = table_38213_13_2_c3[index_4msb];
                mac->rb_offset   = table_38213_13_2_c4[index_4msb];
                break;

            case (scs_30kHz << 5) | scs_15kHz:
                if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
                    AssertFatal(index_4msb < 9, "38.213 Table 13-3 4 MSB out of range\n");
                    ssb_coreset_multiplexing_pattern = 1;
                    mac->num_rbs     = table_38213_13_3_c2[index_4msb];
                    mac->num_symbols = table_38213_13_3_c3[index_4msb];
                    mac->rb_offset   = table_38213_13_3_c4[index_4msb];
                }else if(min_channel_bw & bw_40MHz){
                    AssertFatal(index_4msb < 9, "38.213 Table 13-5 4 MSB out of range\n");
                    ssb_coreset_multiplexing_pattern = 1;
                    mac->num_rbs     = table_38213_13_5_c2[index_4msb];
                    mac->num_symbols = table_38213_13_5_c3[index_4msb];
                    mac->rb_offset   = table_38213_13_5_c4[index_4msb];
                }else{ ; }

                break;

            case (scs_30kHz << 5) | scs_30kHz:
                if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
                    ssb_coreset_multiplexing_pattern = 1;
                    mac->num_rbs     = table_38213_13_4_c2[index_4msb];
                    mac->num_symbols = table_38213_13_4_c3[index_4msb];
                    mac->rb_offset   = table_38213_13_4_c4[index_4msb];
                }else if(min_channel_bw & bw_40MHz){
                    AssertFatal(index_4msb < 10, "38.213 Table 13-6 4 MSB out of range\n");
                    ssb_coreset_multiplexing_pattern = 1;
                    mac->num_rbs     = table_38213_13_6_c2[index_4msb];
                    mac->num_symbols = table_38213_13_6_c3[index_4msb];
                    mac->rb_offset   = table_38213_13_6_c4[index_4msb];
                }else{ ; }
                break;

            case (scs_120kHz << 5) | scs_60kHz:
                AssertFatal(index_4msb < 12, "38.213 Table 13-7 4 MSB out of range\n");
                if(index_4msb & 0x7){
                    ssb_coreset_multiplexing_pattern = 1;
                }else if(index_4msb & 0x18){
                    ssb_coreset_multiplexing_pattern = 2;
                }else{ ; }

                mac->num_rbs     = table_38213_13_7_c2[index_4msb];
                mac->num_symbols = table_38213_13_7_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 8 || index_4msb == 10)){
                    mac->rb_offset   = table_38213_13_7_c4[index_4msb] - 1;
                }else{
                    mac->rb_offset   = table_38213_13_7_c4[index_4msb];
                }
                break;

            case (scs_120kHz << 5) | scs_120kHz:
                AssertFatal(index_4msb < 8, "38.213 Table 13-8 4 MSB out of range\n");
                if(index_4msb & 0x3){
                    ssb_coreset_multiplexing_pattern = 1;
                }else if(index_4msb & 0x0c){
                    ssb_coreset_multiplexing_pattern = 3;
                }

                mac->num_rbs     = table_38213_13_8_c2[index_4msb];
                mac->num_symbols = table_38213_13_8_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
                    mac->rb_offset   = table_38213_13_8_c4[index_4msb] - 1;
                }else{
                    mac->rb_offset   = table_38213_13_8_c4[index_4msb];
                }
                break;

            case (scs_240kHz << 5) | scs_60kHz:
                AssertFatal(index_4msb < 4, "38.213 Table 13-9 4 MSB out of range\n");
                ssb_coreset_multiplexing_pattern = 1;
                mac->num_rbs     = table_38213_13_9_c2[index_4msb];
                mac->num_symbols = table_38213_13_9_c3[index_4msb];
                mac->rb_offset   = table_38213_13_9_c4[index_4msb];
                break;

            case (scs_240kHz << 5) | scs_120kHz:
                AssertFatal(index_4msb < 8, "38.213 Table 13-10 4 MSB out of range\n");
                if(index_4msb & 0x3){
                    ssb_coreset_multiplexing_pattern = 1;
                }else if(index_4msb & 0x0c){
                    ssb_coreset_multiplexing_pattern = 2;
                }
                mac->num_rbs     = table_38213_13_10_c2[index_4msb];
                mac->num_symbols = table_38213_13_10_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
                    mac->rb_offset   = table_38213_13_10_c4[index_4msb]-1;
                }else{
                    mac->rb_offset   = table_38213_13_10_c4[index_4msb];
                }
                
                break;

	        default:
	            break;
	    }

        // type0-pdcch search space
        float big_o;
        uint32_t number_of_search_space_per_slot;
        float big_m;
        uint32_t first_symbol_index;

        const uint32_t num_symbols_coreset = 0; //  check after
        uint32_t ii = 0;    //  check later
        uint32_t kk = 0;
        if(ssb_coreset_multiplexing_pattern == 1 && frequency_range == FR1){
            big_o = table_38213_13_11_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_11_c3[index_4lsb];

            if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 7) && (ii&1)){
                first_symbol_index = num_symbols_coreset;
            }else{
                first_symbol_index = table_38213_13_11_c4[index_4lsb];
            }
        }

        if(ssb_coreset_multiplexing_pattern == 1 && frequency_range == FR2){
            big_o = table_38213_13_12_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_12_c3[index_4lsb];

            if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 10) && (ii&1)){
                first_symbol_index = 7;
            }else if((index_4lsb == 6 || index_4lsb == 7 || index_4lsb == 8 || index_4lsb == 11) && (ii&1)){
                first_symbol_index = num_symbols_coreset;
            }else{
                first_symbol_index = 0;
            }
        }

        if(ssb_coreset_multiplexing_pattern == 2){
            if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_60kHz)){
                //  38.213 Table 13-13
                AssertFatal(index_4lsb == 0, "38.213 Table 13-13 4 LSB out of range\n");
                //  TODO check PDCCH monitoring occasions (SFN and slot number)
                //  TODO check first symbol index
                //first_symbol_index = 
            }else if((scs_ssb == scs_240kHz) && (scs_pdcch == scs_120kHz)){
                //  38.213 Table 13-14
                AssertFatal(index_4lsb == 0, "38.213 Table 13-14 4 LSB out of range\n");
                //  TODO check PDCCH monitoring occasions (SFN and slot number)
                //  TODO check first symbol index
                //first_symbol_index = 
            }else{ ; }
        }

        if(ssb_coreset_multiplexing_pattern == 3){
            if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_120kHz)){
                //  38.213 Table 13-15
                AssertFatal(index_4lsb == 0, "38.213 Table 13-15 4 LSB out of range\n");
                //  TODO check PDCCH monitoring occasions (SFN and slot number)
                //  TODO check first symbol index
                //first_symbol_index = 
            }else{ ; }
        }

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
    const frame_t tx_frame,
    const slot_t tx_slot){


	return CONNECTION_OK;
}