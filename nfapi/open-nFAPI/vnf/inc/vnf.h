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


#ifndef _VNF_H_
#define _VNF_H_

#include "nfapi_vnf_interface.h"

typedef struct 
{

	nfapi_vnf_config_t _public;

	uint8_t terminate;
	uint8_t sctp;

	uint8_t tx_message_buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
	uint16_t next_phy_id;
	
} vnf_t;


int vnf_pack_and_send_p5_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len);
int vnf_nr_pack_and_send_p5_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len);

int vnf_pack_and_send_p4_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len);

int vnf_read_dispatch_message(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* pnf);
int vnf_nr_read_dispatch_message(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* pnf);


void nfapi_vnf_phy_info_list_add(nfapi_vnf_config_t* config, nfapi_vnf_phy_info_t* info);
nfapi_vnf_phy_info_t* nfapi_vnf_phy_info_list_find(nfapi_vnf_config_t* config, uint16_t phy_id);
void nfapi_vnf_pnf_list_add(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* node);
nfapi_vnf_pnf_info_t* nfapi_vnf_pnf_list_find(nfapi_vnf_config_t* config, int p5_idx);
#endif // _VNF_H_
