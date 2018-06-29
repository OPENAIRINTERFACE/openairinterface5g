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

void
nr_ue_decode_mib(
	module_id_t module_id,
	int 		CC_id,
	uint8_t 	gNB_index,
	uint8_t 	extra_bits,
	uint32_t 	ssb_index,
	uint32_t 	*frameP,
	void 		*pduP,
	uint16_t 	pdu_len){

    nr_mac_rrc_data_ind_ue( module_id, CC_id, gNB_index, frameP,
		     NR_BCCH_BCH, (uint8_t *) pduP, pdu_len );
    
    //	frame calculation
}
