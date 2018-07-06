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

/*! \file ue_procedures.c
 * \brief procedures related to UE
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 1
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */
#include "proto.h"
#include "RRC/NR_UE/rrc_proto.h"

int8_t nr_ue_decode_mib(
	module_id_t module_id,
	int 		CC_id,
	uint8_t 	gNB_index,
	uint8_t 	extra_bits,	//	8bits 38.212 c7.1.1
	uint32_t    l_ssb_equal_64,
	uint32_t 	*ssb_index,	//	from decoded MIB
	uint32_t 	*frameP,	//	10 bits = 6(in decoded MIB)+4(in extra bits from L1)
	void 		*pduP,		//	encoded MIB
	uint16_t 	pdu_len){

	NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    nr_mac_rrc_data_ind_ue( module_id, CC_id, gNB_index, frameP,
		     NR_BCCH_BCH, (uint8_t *) pduP, pdu_len );
    

    uint32_t frame = mac->mib->systemFrameNumber.buf[0];
    uint32_t frame_number_4lsb = (uint32_t)(extra_bits & 0xf);
    uint32_t ssb_subcarrier_offset_msb = (uint32_t)(( extra_bits >> 4 ) & 0x1 );

    frame = frame << 4;
    frame = frame | frame_number_4lsb;

    *frameP = frame;

    if(l_ssb_equal_64){
    	*ssb_index = (( extra_bits >> 4 ) & 0x7 );
    }



    return 0;
}
