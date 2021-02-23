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


#ifndef _PNF_H_
#define _PNF_H_

#include "nfapi_pnf_interface.h"

#define NFAPI_MAX_PACKED_MESSAGE_SIZE 8192

typedef struct {

	nfapi_pnf_config_t _public;

	int p5_sock;
	uint8_t tx_message_buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];

	uint8_t sctp;

	uint8_t terminate;

} pnf_t;


int pnf_connect(pnf_t *pnf);
int pnf_message_pump(pnf_t *pnf);
int pnf_nr_message_pump(pnf_t *pnf);

int pnf_nr_pack_and_send_p5_message(pnf_t* pnf, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len);
int pnf_pack_and_send_p5_message(pnf_t* pnf, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len);
int pnf_pack_and_send_p4_message(pnf_t* pnf, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len);
int pnf_send_message(pnf_t* pnf, uint8_t* msg, uint32_t msg_len, uint16_t stream_id);

nfapi_pnf_phy_config_t* nfapi_pnf_phy_config_find(nfapi_pnf_config_t* config, uint16_t phy_id);

#endif // _PNF_H_

