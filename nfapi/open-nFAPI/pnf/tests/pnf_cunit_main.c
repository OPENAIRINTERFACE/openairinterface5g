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

#include "CUnit.h"

#include "Basic.h"
#include "Automated.h"
//#include "CUnit/Console.h"

#include "nfapi_interface.h"
#include "nfapi_pnf_interface.h"
#include "nfapi.h"
#include <stdio.h>  // for printf
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "debug.h"

/* Test Suite setup and cleanup functions: */

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

#define MAX_PACKED_MESSAGE_SIZE	8192

typedef struct phy_info
{
	uint8_t enabled;
	uint16_t phy_id;
	uint16_t sfn_sf;
	uint16_t sfn;
	uint16_t slot;
	

	pthread_t thread;

	int pnf_p7_port;
	char* pnf_p7_addr;

	int vnf_p7_port;
	char* vnf_p7_addr;

	nfapi_pnf_p7_config_t* config;

} phy_info_t;

typedef struct pnf_info
{
	uint8_t num_phys;
	phy_info_t phys[8];

} pnf_info_t;

phy_info_t* find_phy_info(pnf_info_t* pnf, uint16_t phy_id)
{
	int i = 0;
	for(i = 0; i < 8; ++i)
	{
		if(pnf->phys[i].enabled == 1 && pnf->phys[i].phy_id == phy_id)
		{
			return &(pnf->phys[i]);
		}
	}
	return 0;
}



/************* Test case functions ****************/

void pnf_test_start_no_config(void) 
{
	int result = nfapi_pnf_start(0);
	CU_ASSERT_EQUAL(result, -1);
}

void test_trace(nfapi_trace_level_t level, const char* message, ...)
{
	printf("%s", message);
}


void* pnf_test_start_thread(void* ptr)
{
	int result = nfapi_pnf_start((nfapi_pnf_config_t*)ptr);
	return (void*)(intptr_t)result;
}

void* pnf_test_start_p7_thread(void* ptr)
{
	int result = nfapi_pnf_p7_start((nfapi_pnf_p7_config_t*)ptr);
	return (void*)(intptr_t)result;
}

void pnf_test_start_no_connection(void) 
{
	char* vnf_addr = "127.127.0.1";
	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	sleep(1);

	nfapi_pnf_stop(config);

	int* result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL(result, 0);
}

void pnf_test_start_no_ip(void) 
{
	nfapi_pnf_config_t* config = nfapi_pnf_config_create();

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	sleep(2);

	nfapi_pnf_stop(config);

	intptr_t result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL((int)result, -1);
}

void pnf_test_start_invalid_ip(void) 
{
	char* vnf_addr = "not.an.ip.address";
	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	sleep(2);

	nfapi_pnf_stop(config);

	intptr_t result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL((int)result, -1);
}

int create_p5_listen_socket(char* ip, int port)
{
	int p5ListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	// bind to the configured address and port
	bind(p5ListenSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	return p5ListenSock;
}
int create_p5_ipv6_listen_socket(char* ip, int port)
{
	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM; // For SCTP we are only interested in SOCK_STREAM
	hints.ai_family= AF_INET6;

	char port_str[8];
	snprintf(port_str, sizeof(port_str), "%d", port);
	getaddrinfo(ip, port_str,  &hints, &servinfo);


	int p5ListenSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
	//printf("Socket result %d\n", p5ListenSock);
	
	// bind to the configured address and port
	/*int bind_result = */
	bind(p5ListenSock, servinfo->ai_addr, servinfo->ai_addrlen);
	//printf("Bind result %d (errno:%d)\n", bind_result, errno);

	freeaddrinfo(servinfo);

	return p5ListenSock;
}

/*
int wait_for_pnf_connection(int p5ListenSock)
{
	listen(p5ListenSock, 2);
	socklen_t addrSize;
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);

	return p5Sock;
}
*/

int test_param_request_called = 0;
int test_pnf_param_request(nfapi_pnf_config_t* config, nfapi_pnf_param_request_t* req)
{
	printf("test_pnf_param_request called.... ;-)\n");
	
	nfapi_pnf_param_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;

	resp.pnf_param_general.tl.tag = NFAPI_PNF_PARAM_GENERAL_TAG;
	resp.pnf_param_general.nfapi_sync_mode = 2;
	resp.pnf_param_general.location_mode = 1;
	resp.pnf_param_general.location_coordinates[0] = 123;
	resp.pnf_param_general.dl_config_timing = 12;
	resp.pnf_param_general.tx_timing = 12;
	resp.pnf_param_general.ul_config_timing = 12;
	resp.pnf_param_general.hi_dci0_timing = 12;
	resp.pnf_param_general.maximum_number_phys = 2;
	resp.pnf_param_general.maximum_total_bandwidth = 100;
	resp.pnf_param_general.maximum_total_number_dl_layers = 2;
	resp.pnf_param_general.maximum_total_number_ul_layers = 2;
	resp.pnf_param_general.shared_bands = 0;
	resp.pnf_param_general.shared_pa = 0;
	resp.pnf_param_general.maximum_total_power = -190;
	resp.pnf_param_general.oui[0] = 88;
/*
	in.pnf_phy.tl.tag = NFAPI_PNF_PHY_TAG;
	in.pnf_phy.number_of_phys = 2;
	in.pnf_phy.phy[0].phy_config_index = 0;
	in.pnf_phy.phy[0].number_of_rfs = 2;
	in.pnf_phy.phy[0].rf_config[0].rf_config_index = 0;
	in.pnf_phy.phy[0].number_of_rf_exclusions = 1;
	in.pnf_phy.phy[0].rf_config[0].rf_config_index = 1;
	in.pnf_phy.phy[0].downlink_channel_bandwidth_supported = 20;
	in.pnf_phy.phy[0].uplink_channel_bandwidth_supported = 20;
	in.pnf_phy.phy[0].number_of_dl_layers_supported = 2;
	in.pnf_phy.phy[0].number_of_ul_layers_supported = 1;
	in.pnf_phy.phy[0].maximum_3gpp_release_supported = 3;
	in.pnf_phy.phy[0].nmm_modes_supported = 2;

	in.pnf_rf.tl.tag = NFAPI_PNF_RF_TAG;
	in.pnf_rf.number_of_rfs = 2;
	in.pnf_rf.rf[0].rf_config_index = 0;
	in.pnf_rf.rf[0].band = 1;
	in.pnf_rf.rf[0].maximum_transmit_power = -100; 
	in.pnf_rf.rf[0].minimum_transmit_power = -90; 
	in.pnf_rf.rf[0].number_of_antennas_suppported = 4;
	in.pnf_rf.rf[0].minimum_downlink_frequency = 100;
	in.pnf_rf.rf[0].maximum_downlink_frequency = 2132;
	in.pnf_rf.rf[0].minimum_uplink_frequency = 1231;
	in.pnf_rf.rf[0].maximum_uplink_frequency = 123;


	in.pnf_phy_rel10.tl.tag = NFAPI_PNF_PHY_REL10_TAG;
	in.pnf_phy_rel10.number_of_phys = 2;
	in.pnf_phy_rel10.phy[0].phy_config_index = 0;
	in.pnf_phy_rel10.phy[0].transmission_mode_7_supported = 1;
	in.pnf_phy_rel10.phy[0].transmission_mode_8_supported = 2;
	in.pnf_phy_rel10.phy[0].two_antenna_ports_for_pucch = 3;
	in.pnf_phy_rel10.phy[0].transmission_mode_9_supported = 4;
	in.pnf_phy_rel10.phy[0].simultaneous_pucch_pusch = 5;
	in.pnf_phy_rel10.phy[0].four_layer_tx_with_tm3_and_tm4 = 6;

	in.pnf_phy_rel11.tl.tag = NFAPI_PNF_PHY_REL11_TAG;
	in.pnf_phy_rel11.number_of_phys = 2;
	in.pnf_phy_rel11.phy[0].phy_config_index = 0;
	in.pnf_phy_rel11.phy[0].edpcch_supported = 1;
	in.pnf_phy_rel11.phy[0].multi_ack_csi_reporting = 2;
	in.pnf_phy_rel11.phy[0].pucch_tx_diversity = 3;
	in.pnf_phy_rel11.phy[0].ul_comp_supported = 4;
	in.pnf_phy_rel11.phy[0].transmission_mode_5_supported = 5;

	in.pnf_phy_rel12.tl.tag = NFAPI_PNF_PHY_REL12_TAG;
	in.pnf_phy_rel12.number_of_phys = 2;
	in.pnf_phy_rel12.phy[0].phy_config_index = 0;
	in.pnf_phy_rel12.phy[0].csi_subframe_set = 1;
	in.pnf_phy_rel12.phy[0].enhanced_4tx_codebook = 2;
	in.pnf_phy_rel12.phy[0].drs_supported = 3;
	in.pnf_phy_rel12.phy[0].ul_64qam_supported = 4;
	in.pnf_phy_rel12.phy[0].transmission_mode_10_supported = 5;
	in.pnf_phy_rel12.phy[0].alternative_bts_indices = 6;

	in.pnf_phy_rel13.tl.tag = NFAPI_PNF_PHY_REL13_TAG;
	in.pnf_phy_rel13.number_of_phys = 2;
	in.pnf_phy_rel13.phy[0].phy_config_index = 0;
	in.pnf_phy_rel13.phy[0].pucch_format4_supported = 1;
	in.pnf_phy_rel13.phy[0].pucch_format5_supported = 2;
	in.pnf_phy_rel13.phy[0].more_than_5_ca_support = 3;
	in.pnf_phy_rel13.phy[0].laa_supported = 4;
	in.pnf_phy_rel13.phy[0].laa_ending_in_dwpts_supported = 5;
	in.pnf_phy_rel13.phy[0].laa_starting_in_second_slot_supported = 6;
	in.pnf_phy_rel13.phy[0].beamforming_supported = 7;
	in.pnf_phy_rel13.phy[0].csi_rs_enhancement_supported = 8;
	in.pnf_phy_rel13.phy[0].drms_enhancement_supported = 9;
	in.pnf_phy_rel13.phy[0].srs_enhancement_supported = 10;
*/
	test_param_request_called++;

	nfapi_pnf_pnf_param_resp(config, &resp);
	return 0;
}

int test_config_request_called = 0;
int test_pnf_config_request(nfapi_pnf_config_t* config, nfapi_pnf_config_request_t* req)
{
	printf("test_pnf_config_request called.... ;-)\n");

	CU_ASSERT_EQUAL(req->pnf_phy_rf_config.tl.tag, NFAPI_PNF_PHY_RF_TAG);

	if(config->user_data != 0)
	{
		pnf_info_t* pnf = (pnf_info_t*)(config->user_data);	
		int i = 0;
		for(i = 0; i < req->pnf_phy_rf_config.number_phy_rf_config_info; ++i)
		{
			pnf->phys[i].enabled = 1;
			pnf->phys[i].phy_id = req->pnf_phy_rf_config.phy_rf_config[i].phy_id;
			printf("test_pnf_config_request creating phy %d\n", pnf->phys[i].phy_id);

		}
	}
	
	nfapi_pnf_config_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_pnf_config_resp(config, &resp);

	test_config_request_called++;
	return 0;
}
int test_pnf_start_request(nfapi_pnf_config_t* config, nfapi_pnf_start_request_t* req)
{
	printf("test_pnf_start_request called.... ;-)\n");

	nfapi_pnf_start_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_START_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_pnf_start_resp(config, &resp);
	return 0;
}
int test_pnf_stop_request(nfapi_pnf_config_t* config, nfapi_pnf_stop_request_t* req)
{
	printf("test_pnf_stop_request called.... ;-)\n");

	nfapi_pnf_stop_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_STOP_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_pnf_stop_resp(config, &resp);
	return 0;
}
int test_param_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_param_request_t* req)
{
	printf("test_param_request called.... ;-)\n");

	nfapi_param_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PARAM_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_MSG_OK;

	if(config->user_data != 0)
	{
		phy_info_t* phy_info = find_phy_info((pnf_info_t*)(config->user_data), req->header.phy_id);	

		resp.nfapi_config.p7_pnf_port.tl.tag = NFAPI_NFAPI_P7_PNF_PORT_TAG;
		resp.nfapi_config.p7_pnf_port.value = phy_info->pnf_p7_port;
		resp.num_tlv++;

		resp.nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_PNF_PORT_TAG;
		struct sockaddr_in pnf_p7_sockaddr;
		pnf_p7_sockaddr.sin_addr.s_addr = inet_addr(phy_info->pnf_p7_addr);
		memcpy(&(resp.nfapi_config.p7_pnf_address_ipv4.address[0]), &pnf_p7_sockaddr.sin_addr.s_addr, 4);
		resp.num_tlv++;
	}




	nfapi_pnf_param_resp(config, &resp);
	return 0;
}
int test_config_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_config_request_t* req)
{
	printf("test_config_request called.... ;-)\n");

	if(config->user_data != 0)
	{
		phy_info_t* phy_info = find_phy_info((pnf_info_t*)(config->user_data), req->header.phy_id);

		if(req->nfapi_config.p7_vnf_port.tl.tag == NFAPI_NFAPI_P7_VNF_PORT_TAG)
		{
			phy_info->vnf_p7_port = req->nfapi_config.p7_vnf_port.value;
			printf("vnf_p7_port %d\n", phy_info->vnf_p7_port);
		}

		if(req->nfapi_config.p7_vnf_address_ipv4.tl.tag == NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG)
		{
			struct sockaddr_in addr;
			memcpy(&addr.sin_addr.s_addr, req->nfapi_config.p7_vnf_address_ipv4.address, 4);
			char* ip = inet_ntoa(addr.sin_addr);
			phy_info->vnf_p7_addr = ip;
			printf("vnf_p7_addr %s\n", phy_info->vnf_p7_addr);
		}

			
	}

	nfapi_config_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_CONFIG_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_config_resp(config, &resp);
	return 0;
}
int test_dl_config_req(nfapi_pnf_p7_config_t* config, nfapi_dl_config_request_t* req)
{
	printf("test_dl_config_req called.... ;-)\n");
	return 0;
}

int test_ul_config_req(nfapi_pnf_p7_config_t* config, nfapi_ul_config_request_t* req)
{
	printf("test_ul_config_req called.... ;-)\n");
	return 0;
}
int test_hi_dci0_req(nfapi_pnf_p7_config_t* config, nfapi_hi_dci0_request_t* req)
{
	printf("test_hi_dci0_req called.... ;-)\n");
	return 0;
}
int test_tx_req(nfapi_pnf_p7_config_t* config, nfapi_tx_request_t* req)
{
	printf("test_tx_req called.... ;-)\n");
	return 0;
}
int test_lbt_dl_config_req(nfapi_pnf_p7_config_t* config, nfapi_lbt_dl_config_request_t* req)
{
	printf("test_lbt_dl_req called.... ;-)\n");
	return 0;
}
int test_start_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_start_request_t* req)
{
	printf("test_start_request called.... ;-)\n");


	if(config->user_data != 0)
	{
		phy_info_t* phy_info = find_phy_info((pnf_info_t*)(config->user_data), req->header.phy_id);

		phy_info->config = nfapi_pnf_p7_config_create();
		phy_info->config->local_p7_port = phy_info->pnf_p7_port;

		phy_info->config->remote_p7_port = phy_info->vnf_p7_port;
		phy_info->config->remote_p7_addr = phy_info->vnf_p7_addr;

		phy_info->config->dl_config_req = &test_dl_config_req;
		phy_info->config->ul_config_req = &test_ul_config_req;
		phy_info->config->hi_dci0_req = &test_hi_dci0_req;
		phy_info->config->tx_req = &test_tx_req;
		phy_info->config->lbt_dl_config_req = &test_lbt_dl_config_req;

		//phy_info->config->subframe_buffer_size = 8;
		//phy_info->config->timing_info_mode_periodic = 1;
		//phy_info->config->timing_info_period = 32;
		//phy_info->config->timing_info_mode_aperiodic = 1;

		phy_info->config->segment_size = 1400;
		phy_info->config->checksum_enabled = 0;


		pthread_create(&phy_info->thread, NULL, &pnf_test_start_p7_thread, phy_info->config);
	}

	nfapi_start_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_START_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_start_resp(config, &resp);
	return 0;
}
int test_stop_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_stop_request_t* req)
{
	printf("test_stop_request called.... ;-)\n");

	nfapi_stop_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_STOP_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_stop_resp(config, &resp);
	return 0;
}

int send_p5_message(int p5Sock, nfapi_p4_p5_message_header_t* msg, unsigned msg_size, struct sockaddr* addr, socklen_t addr_size)
{
	char buffer[256];
	int encoded_size = nfapi_p5_message_pack(msg, msg_size, buffer, sizeof(buffer), 0);
	int result  = sendto(p5Sock, buffer, encoded_size, 0, addr, addr_size);
	return result;
}

int send_p7_message(int p7Sock, nfapi_p7_message_header_t* msg, unsigned msg_size, struct sockaddr_in* addr, socklen_t addr_size)
{
	char buffer[256];
	int encoded_size = nfapi_p7_message_pack(msg, buffer, sizeof(buffer), 0);
	int result = sendto(p7Sock, buffer, encoded_size, 0, (struct sockaddr*)addr, addr_size);

	printf("send_p7_message result %d\n", result);
	return result;
}
int send_p7_segmented_message(int p7Sock, nfapi_p7_message_header_t* msg, unsigned msg_size, struct sockaddr_in* addr, socklen_t addr_size)
{
	char buffer[1024 * 10];
	int encoded_size = nfapi_p7_message_pack(msg, buffer, sizeof(buffer), 0);

	uint16_t segment_size = 256;

	if(encoded_size < segment_size)
	{
		int result = sendto(p7Sock, buffer, encoded_size, 0, (struct sockaddr*)addr, addr_size);
		return result;
	}
	else
	{
	//	printf("Segmenting p7 message %d\n", encoded_size);

		uint8_t segment_count = 0;
		uint8_t buffer2[segment_size];
		uint32_t offset = 0;
		uint16_t msg_size;
		//uint8_t m = 1;

		do{

			if(segment_count == 0)
			{
				msg_size = segment_size;
				memcpy(&buffer2[0], &buffer[0], msg_size);
				// M = 1
				buffer2[6] = (1 << 7) | (segment_count & 0x7F);

				offset = msg_size;
			}
			else
			{
				memcpy(&buffer2[0], &buffer[0], NFAPI_P7_HEADER_LENGTH);

				if(encoded_size - offset > segment_size)
				{
					// M = 1
					buffer2[6] = (1 << 7) | (segment_count & 0x7F);
					msg_size = segment_size;
				}
				else
				{
					// M = 0
					buffer2[6] = (0 << 7) | (segment_count & 0x7F);
					msg_size = encoded_size - offset + NFAPI_P7_HEADER_LENGTH;
				}


				memcpy(&buffer2[NFAPI_P7_HEADER_LENGTH], &buffer[offset], msg_size - NFAPI_P7_HEADER_LENGTH);
			
				offset += msg_size - NFAPI_P7_HEADER_LENGTH;
			}

			uint8_t* p = &buffer2[4];
			push16(msg_size, &p, &buffer2[segment_size]);

		//printf("Segmenting p7 message %d %di %d\n", segment_count, msg_size, offset);
			/*int result = */sendto(p7Sock, buffer2, msg_size, 0, (struct sockaddr*)addr, addr_size);

			segment_count++;

		}while(offset < encoded_size);

	}

	return 0;
}
int send_p7_segmented_message_outoforder(int p7Sock, nfapi_p7_message_header_t* msg, unsigned msg_size, struct sockaddr_in* addr, socklen_t addr_size)
{
	char buffer[1024 * 10];
	int encoded_size = nfapi_p7_message_pack(msg, buffer, sizeof(buffer), 0);

	uint16_t segment_size = 256;

	if(encoded_size < segment_size)
	{
		int result = sendto(p7Sock, buffer, encoded_size, 0, (struct sockaddr*)addr, addr_size);
		return result;
	}
	else
	{
	//	printf("Segmenting p7 message %d\n", encoded_size);

		uint8_t segment_count = 0;
		uint8_t buffer2[128][segment_size];
		uint32_t offset = 0;
		uint16_t msg_size[128];
		uint8_t i = 0;

		do{

			if(segment_count == 0)
			{
				msg_size[i] = segment_size;
				memcpy(&buffer2[i][0], &buffer[0], msg_size[i]);
				// M = 1
				buffer2[i][6] = (1 << 7) | (segment_count & 0x7F);

				offset = msg_size[i];
			}
			else
			{
				memcpy(&buffer2[i][0], &buffer[0], NFAPI_P7_HEADER_LENGTH);

				if(encoded_size - offset > segment_size)
				{
					// M = 1
					buffer2[i][6] = (1 << 7) | (segment_count & 0x7F);
					msg_size[i] = segment_size;
				}
				else
				{
					// M = 0
					buffer2[i][6] = (0 << 7) | (segment_count & 0x7F);
					msg_size[i] = encoded_size - offset + NFAPI_P7_HEADER_LENGTH;
				}


				memcpy(&buffer2[i][NFAPI_P7_HEADER_LENGTH], &buffer[offset], msg_size[i] - NFAPI_P7_HEADER_LENGTH);
			
				offset += msg_size[i] - NFAPI_P7_HEADER_LENGTH;
			}

			uint8_t* p = &buffer2[i][4];
			push16(msg_size[i], &p, &buffer2[i][segment_size]);


			segment_count++;
			i++;

		}while(offset < encoded_size);

		// send all odd then send event
		uint8_t e = 0;
		uint8_t o = 1;
		while(o <= i)
		{
			sendto(p7Sock, &buffer2[o][0], msg_size[o], 0, (struct sockaddr*)addr, addr_size);
			o += 2;
		}
		while(e <= i)
		{
			sendto(p7Sock, &buffer2[e][0], msg_size[e], 0, (struct sockaddr*)addr, addr_size);
			e += 2;
		}

	}

	return 0;
}

int get_p7_segmented_message(int p7Sock, nfapi_p7_message_header_t* msg, unsigned _msg_size, struct sockaddr_in* addr, socklen_t addr_size, uint8_t (buffer2)[][256], uint16_t msg_size[256], uint8_t i)
{
	char buffer[1024 * 10];
	int encoded_size = nfapi_p7_message_pack(msg, buffer, sizeof(buffer), 0);

	uint16_t segment_size = 256;

	printf("get_p7_segmented_message size %d\n", encoded_size);

	if(encoded_size < segment_size)
	{
		msg_size[i] = encoded_size;
		memcpy(&buffer2[i][0], &buffer[0], msg_size[i]);
		i++;
	}
	else
	{
		uint8_t segment_count = 0;
		uint32_t offset = 0;

		do{

			if(segment_count == 0)
			{
				msg_size[i] = segment_size;
				memcpy(&buffer2[i][0], &buffer[0], msg_size[i]);
				// M = 1
				(buffer2[i][6]) = (1 << 7) | (segment_count & 0x7F);

				offset = msg_size[i];
			}
			else
			{
				memcpy(&buffer2[i][0], &buffer[0], NFAPI_P7_HEADER_LENGTH);

				if(encoded_size - offset > segment_size)
				{
					// M = 1
					(buffer2[i][6]) = (1 << 7) | (segment_count & 0x7F);
					msg_size[i] = segment_size;
				}
				else
				{
					// M = 0
					(buffer2[i][6]) = (0 << 7) | (segment_count & 0x7F);
					msg_size[i] = encoded_size - offset + NFAPI_P7_HEADER_LENGTH;
				}


				memcpy(&buffer2[i][NFAPI_P7_HEADER_LENGTH], &buffer[offset], msg_size[i] - NFAPI_P7_HEADER_LENGTH);
			
				offset += msg_size[i] - NFAPI_P7_HEADER_LENGTH;
			}

			uint8_t* p = &buffer2[i][4];
			push16(msg_size[i], &p, &buffer2[i][segment_size]);


			segment_count++;
			i++;

		}while(offset < encoded_size);

	}

	return i;
}


int recv_p5_message(int p5Sock, nfapi_p4_p5_message_header_t* msg, unsigned msg_size)
{
	//struct sockaddr_in addr2;
	//socklen_t addr2len;
	char buffer2[1024];
	int result2 = recv(p5Sock, buffer2, sizeof(buffer2), 0); //, (struct sockaddr*)&addr2, &addr2len); 

	if(result2 == -1)
	{
		printf("%s recvfrom failed errno:%d\n", __FUNCTION__, errno);
	}
	else
	{
		/*int unpack_result =*/ nfapi_p5_message_unpack(buffer2, result2, msg, msg_size, 0); //, sizeof(resp));
	}
	return 0;
}

int recv_p7_message(int p7Sock, nfapi_p7_message_header_t* msg, unsigned msg_size)
{
	char buffer2[2560];
	int result2 = recvfrom(p7Sock, buffer2, sizeof(buffer2), 0, 0, 0);

	if(result2 == -1)
	{
		printf("%s recvfrom failed errno:%d\n", __FUNCTION__, errno);
	}
	else
	{
		/*int unpack_result =*/ nfapi_p7_message_unpack(buffer2, result2, msg, msg_size, 0); //, sizeof(resp));
	}
	return 0;
}

int send_pnf_param_request(int p5Sock, struct sockaddr* addr, socklen_t addr_size)
{
	printf("%s\n", __FUNCTION__);
	nfapi_pnf_param_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PNF_PARAM_REQUEST;
	req.header.message_length = 0;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
	return 0;
}

int receive_pnf_param_response(int p5Sock, uint8_t error_code)
{
	nfapi_pnf_param_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));

		printf("0x%x\n", resp.header.message_id);
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_PNF_PARAM_RESPONSE);
	if(resp.header.message_id == NFAPI_PNF_PARAM_RESPONSE)
	{
		printf("decoded nfapi_pnf_param_response\n");
		CU_ASSERT_EQUAL(resp.error_code, error_code);
		if(error_code == NFAPI_MSG_OK)
		{
			CU_ASSERT_EQUAL(resp.pnf_param_general.tl.tag, NFAPI_PNF_PARAM_GENERAL_TAG);
			CU_ASSERT_EQUAL(resp.pnf_param_general.nfapi_sync_mode,  2);
			CU_ASSERT_EQUAL(resp.pnf_param_general.location_mode, 1);
		}
	}
	return 0;
}

int send_pnf_config_request(int p5Sock, struct sockaddr* addr, socklen_t addr_size)
{
	nfapi_pnf_config_request_t req;
	req.header.message_id = NFAPI_PNF_CONFIG_REQUEST;
	req.header.message_length = 0;

	req.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
	req.pnf_phy_rf_config.number_phy_rf_config_info = 1;
	req.pnf_phy_rf_config.phy_rf_config[0].phy_id = 0;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
	return 0;
}

int receive_pnf_config_response(int p5Sock)
{
	nfapi_pnf_config_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_PNF_CONFIG_RESPONSE);
	if(resp.header.message_id == NFAPI_PNF_CONFIG_RESPONSE)
	{
		printf("decoded nfapi_pnf_config_response\n");
	}
	return 0;
}

void send_pnf_start_request(int p5Sock,  struct sockaddr* addr, socklen_t addr_size)
{
	nfapi_pnf_start_request_t req;
	req.header.message_id = NFAPI_PNF_START_REQUEST;
	req.header.message_length = 0;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
}

void receive_pnf_start_response(int p5Sock)
{
	nfapi_pnf_start_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_PNF_START_RESPONSE);
	if(resp.header.message_id == NFAPI_PNF_START_RESPONSE)
	{
		printf("decoded nfapi_pnf_start_response\n");
	}
}

void send_pnf_stop_request(int p5Sock,  struct sockaddr* addr, socklen_t addr_size)
{
	nfapi_pnf_stop_request_t req;
	req.header.message_id = NFAPI_PNF_STOP_REQUEST;
	req.header.message_length = 0;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
}

void receive_pnf_stop_response(int p5Sock)
{
	nfapi_pnf_stop_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_PNF_STOP_RESPONSE);
	if(resp.header.message_id == NFAPI_PNF_STOP_RESPONSE)
	{
		printf("decoded nfapi_pnf_stop_response\n");
	}
}

void send_param_request(int p5Sock, int phy_id, struct sockaddr* addr, socklen_t addr_size)
{
	nfapi_param_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PARAM_REQUEST;
	req.header.phy_id = phy_id;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
}
void receive_param_response(int p5Sock)
{
	nfapi_pnf_stop_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_PARAM_RESPONSE);
	if(resp.header.message_id == NFAPI_PARAM_RESPONSE)
	{
		printf("decoded nfapi_param_response\n");
	}
}
void send_config_request(int p5Sock, int phy_id, struct sockaddr* addr, socklen_t addr_size, pnf_info_t* pnf_info)
{
	nfapi_config_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_CONFIG_REQUEST;
	req.header.phy_id = phy_id;

	phy_info_t* phy_info = find_phy_info(pnf_info, phy_id);

	req.nfapi_config.p7_vnf_port.tl.tag = NFAPI_NFAPI_P7_VNF_PORT_TAG;
	req.nfapi_config.p7_vnf_port.value = phy_info->vnf_p7_port;
	req.num_tlv++;

	//req.nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
	//req.num_tlv++;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
}
void receive_config_response(int p5Sock)
{
	nfapi_pnf_stop_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_CONFIG_RESPONSE);
	if(resp.header.message_id == NFAPI_CONFIG_RESPONSE)
	{
		printf("decoded nfapi_config_response\n");
	}
}
void send_start_request(int p5Sock, int phy_id, struct sockaddr* addr, socklen_t addr_size)
{
	nfapi_start_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_START_REQUEST;
	req.header.phy_id = phy_id;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
}
void receive_start_response(int p5Sock)
{
	nfapi_pnf_start_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_START_RESPONSE);
	if(resp.header.message_id == NFAPI_START_RESPONSE)
	{
		printf("decoded nfapi_start_response\n");
	}
}
void send_stop_request(int p5Sock, int phy_id, struct sockaddr* addr, socklen_t addr_size)
{
	nfapi_start_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_STOP_REQUEST;
	req.header.phy_id = phy_id;

	send_p5_message(p5Sock, &(req.header), sizeof(req), addr, addr_size);
}

void receive_stop_response(int p5Sock)
{
	nfapi_pnf_stop_response_t resp;
	recv_p5_message(p5Sock, &(resp.header), sizeof(resp));
	
	CU_ASSERT_EQUAL(resp.header.message_id, NFAPI_STOP_RESPONSE);
	if(resp.header.message_id == NFAPI_STOP_RESPONSE)
	{
		printf("decoded nfapi_stop_response\n");
	}
}

void send_dl_node_sync(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_dl_node_sync_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_DL_NODE_SYNC;
	req.header.phy_id = phy_id;
	req.t1 = 1977;
	req.delta_sfn_sf = 5000;

	send_p7_message(p7Sock, &(req.header), sizeof(req), addr, addr_size);
}
void receive_ul_node_sync(int p7Sock)
{
	nfapi_ul_node_sync_t sync;
	recv_p7_message(p7Sock, &(sync.header), sizeof(sync));
	
	CU_ASSERT_EQUAL(sync.header.message_id, NFAPI_UL_NODE_SYNC);
	if(sync.header.message_id == NFAPI_UL_NODE_SYNC)
	{
		printf("decoded nfapi_ul_node_sync t1:%d t2:%d t3:%d\n",
				sync.t1, sync.t2, sync.t3);
	}
}

void receive_timing_info(int p7Sock)
{
	nfapi_timing_info_t timing_info;
	recv_p7_message(p7Sock, &(timing_info.header), sizeof(timing_info));
	
	CU_ASSERT_EQUAL(timing_info.header.message_id, NFAPI_TIMING_INFO);
	if(timing_info.header.message_id == NFAPI_TIMING_INFO)
	{
		printf("decoded nfapi_timing_info\n");
	}
	else
	{
		printf("decoded ?? 0x%x\n", timing_info.header.message_id);
	}
}
void send_dl_config_req(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{
	nfapi_dl_config_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_DL_CONFIG_REQUEST;
	req.header.phy_id = phy_id;
	req.sfn_sf = sfn_sf;

	send_p7_message(p7Sock, &(req.header), sizeof(req), addr, addr_size);
}
void send_ul_config_req(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{
	nfapi_ul_config_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_UL_CONFIG_REQUEST;
	req.header.phy_id = phy_id;
	req.sfn_sf = sfn_sf;

	send_p7_message(p7Sock, &(req.header), sizeof(req), addr, addr_size);
}
void send_hi_dci0_req(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{
	nfapi_hi_dci0_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_HI_DCI0_REQUEST;
	req.header.phy_id = phy_id;
	req.sfn_sf = sfn_sf;

	send_p7_message(p7Sock, &(req.header), sizeof(req), addr, addr_size);
}
void send_tx_req(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{
	nfapi_tx_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_TX_REQUEST;
	req.header.phy_id = phy_id;
	req.sfn_sf = sfn_sf;
	send_p7_message(p7Sock, &(req.header), sizeof(req), addr, addr_size);
}

void send_tx_req_1(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{
	nfapi_tx_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_TX_REQUEST;
	req.header.phy_id = phy_id;
	req.sfn_sf = sfn_sf;
	req.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
	req.tx_request_body.number_of_pdus = 8;
	req.tx_request_body.tx_pdu_list = (nfapi_tx_request_pdu_t*)(malloc(sizeof(nfapi_tx_request_t) * 8));
	uint8_t i = 0;
	uint8_t test_pdu[1200];

	for(i = 0; i < 8; i++)
	{
		req.tx_request_body.tx_pdu_list[i].pdu_length = sizeof(test_pdu);
		req.tx_request_body.tx_pdu_list[i].pdu_index = i;
		req.tx_request_body.tx_pdu_list[i].num_segments =1;
		req.tx_request_body.tx_pdu_list[i].segments[0].segment_length = sizeof(test_pdu);
		req.tx_request_body.tx_pdu_list[i].segments[0].segment_data  = (void*)&test_pdu;
	}

	send_p7_segmented_message(p7Sock, &(req.header), sizeof(req), addr, addr_size);

	free(req.tx_request_body.tx_pdu_list);

}

void send_tx_req_2(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{
	nfapi_tx_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_TX_REQUEST;
	req.header.phy_id = phy_id;
	req.sfn_sf = sfn_sf;
	req.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
	req.tx_request_body.number_of_pdus = 8;
	req.tx_request_body.tx_pdu_list = (nfapi_tx_request_pdu_t*)(malloc(sizeof(nfapi_tx_request_t) * 8));
	uint8_t i = 0;
	uint8_t test_pdu[1200];

	for(i = 0; i < 8; i++)
	{
		req.tx_request_body.tx_pdu_list[i].pdu_length = sizeof(test_pdu);
		req.tx_request_body.tx_pdu_list[i].pdu_index = i;
		req.tx_request_body.tx_pdu_list[i].num_segments =1;
		req.tx_request_body.tx_pdu_list[i].segments[0].segment_length = sizeof(test_pdu);
		req.tx_request_body.tx_pdu_list[i].segments[0].segment_data  = (void*)&test_pdu;
	}

	send_p7_segmented_message_outoforder(p7Sock, &(req.header), sizeof(req), addr, addr_size);

	free(req.tx_request_body.tx_pdu_list);

}
void send_dl_subframe_msgs_interleaved(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size, uint16_t sfn_sf)
{

	uint8_t buffer[256][256];
	uint16_t msg_size[256];
	uint16_t count = 0;

	{
		nfapi_dl_config_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_DL_CONFIG_REQUEST;
		req.header.phy_id = phy_id;
		req.header.m_segment_sequence = 34;
		req.sfn_sf = sfn_sf;
		req.dl_config_request_body.tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
		req.dl_config_request_body.number_pdu = 8;
		req.dl_config_request_body.dl_config_pdu_list = (nfapi_dl_config_request_pdu_t*)malloc(sizeof(nfapi_dl_config_request_pdu_t) * 8);

		uint8_t i = 0;
		for(i = 0; i < 8; i++)
		{
			nfapi_dl_config_request_pdu_t* pdu = &(req.dl_config_request_body.dl_config_pdu_list[i]);
			pdu->pdu_type = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
			pdu->dlsch_pdu.dlsch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG;
		}


		count = get_p7_segmented_message(p7Sock, &(req.header), sizeof(req), addr, addr_size, buffer, msg_size, count);
	}

	{
		nfapi_ul_config_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_UL_CONFIG_REQUEST;
		req.header.phy_id = phy_id;
		req.header.m_segment_sequence = 35;
		req.sfn_sf = sfn_sf;

		count = get_p7_segmented_message(p7Sock, &(req.header), sizeof(req), addr, addr_size, buffer, msg_size, count);
	}

	{
		nfapi_hi_dci0_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_HI_DCI0_REQUEST;
		req.header.phy_id = phy_id;
		req.header.m_segment_sequence = 36;
		req.sfn_sf = sfn_sf;
		count = get_p7_segmented_message(p7Sock, &(req.header), sizeof(req), addr, addr_size, buffer, msg_size, count);
	}

	{
		nfapi_tx_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_TX_REQUEST;
		req.header.phy_id = phy_id;
		req.header.m_segment_sequence = 37;
		req.sfn_sf = sfn_sf;
		req.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
		req.tx_request_body.number_of_pdus = 8;
		req.tx_request_body.tx_pdu_list = (nfapi_tx_request_pdu_t*)(malloc(sizeof(nfapi_tx_request_t) * 8));
		uint8_t i = 0;
		uint8_t test_pdu[1200];

		for(i = 0; i < 8; i++)
		{
			req.tx_request_body.tx_pdu_list[i].pdu_length = sizeof(test_pdu);
			req.tx_request_body.tx_pdu_list[i].pdu_index = i;
			req.tx_request_body.tx_pdu_list[i].num_segments =1;
			req.tx_request_body.tx_pdu_list[i].segments[0].segment_length = sizeof(test_pdu);
			req.tx_request_body.tx_pdu_list[i].segments[0].segment_data  = (void*)&test_pdu;
		}

		count = get_p7_segmented_message(p7Sock, &(req.header), sizeof(req), addr, addr_size, buffer, msg_size, count);
	}

	// send all odd then send event
	uint8_t e = 0;
	uint8_t o = 1;
	while(o <= count)
	{
		if(msg_size[o] == 0)
			printf("**** Sending a zero length msg %d\n", o);
		sendto(p7Sock, &buffer[o][0], msg_size[o], 0, (struct sockaddr*)addr, addr_size);
		o += 2;
	}
	while(e <= count)
	{
		if(msg_size[e] == 0)
			printf("**** Sending a zero length msg %d\n", e);

		sendto(p7Sock, &buffer[e][0], msg_size[e], 0, (struct sockaddr*)addr, addr_size);
		e += 2;
	}
	//free(req.tx_request_body.tx_pdu_list);
}

void send_slot_indication(phy_info_t* phy_info)
{
	// DONE: add sfn and slot as members in the phy_info
	nfapi_pnf_p7_slot_ind(phy_info->config, phy_info->phy_id, phy_info->sfn, phy_info->slot);
}

void send_subframe_indication(phy_info_t* phy_info)
{
	nfapi_pnf_p7_subframe_ind(phy_info->config, phy_info->phy_id, phy_info->sfn_sf);
}

void send_harq_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_harq_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_HARQ_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_harq_ind((phy_info->config), &ind);
}
void recieve_harq_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_harq_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_HARQ_INDICATION);
	if(ind.header.message_id == NFAPI_HARQ_INDICATION)
	{
		printf("decoded nfapi_harq_indication\n");
	}
	else
	{
		printf("decoded ?? 0x%x\n", ind.header.message_id);
	}
}

void send_crc_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_crc_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_CRC_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_crc_ind((phy_info->config), &ind);

}
void recieve_crc_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_crc_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_CRC_INDICATION);
	if(ind.header.message_id == NFAPI_CRC_INDICATION)
	{
		printf("decoded nfapi_crc_indication\n");
	}
}

void send_rx_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_rx_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_RX_ULSCH_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_rx_ind((phy_info->config), &ind);
}
void recieve_rx_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_rx_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_RX_ULSCH_INDICATION);
	if(ind.header.message_id == NFAPI_RX_ULSCH_INDICATION)
	{
		printf("decoded nfapi_rx_indication\n");
	}
}

void send_rach_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_rach_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_RACH_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_rach_ind((phy_info->config), &ind);
}
void recieve_rach_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_rach_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_RACH_INDICATION);
	if(ind.header.message_id == NFAPI_RACH_INDICATION)
	{
		printf("decoded nfapi_rach_indication\n");
	}
}

void send_srs_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_srs_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_SRS_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_srs_ind((phy_info->config), &ind);
}
void recieve_srs_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_srs_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_SRS_INDICATION);
	if(ind.header.message_id == NFAPI_SRS_INDICATION)
	{
		printf("decoded nfapi_srs_indication\n");
	}
}

void send_sr_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_sr_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_RX_SR_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_sr_ind((phy_info->config), &ind);
}
void recieve_sr_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_sr_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_RX_SR_INDICATION);
	if(ind.header.message_id == NFAPI_RX_SR_INDICATION)
	{
		printf("decoded nfapi_sr_indication\n");
	}
}

void send_cqi_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_cqi_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_RX_CQI_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_cqi_ind((phy_info->config), &ind);
}
void recieve_cqi_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_cqi_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_RX_CQI_INDICATION);
	if(ind.header.message_id == NFAPI_RX_CQI_INDICATION)
	{
		printf("decoded nfapi_cqi_indication\n");
	}
}

void send_lbt_dl_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_lbt_dl_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_LBT_DL_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_lbt_dl_ind((phy_info->config), &ind);
}
void recieve_lbt_dl_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_lbt_dl_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_LBT_DL_INDICATION);
	if(ind.header.message_id == NFAPI_LBT_DL_INDICATION)
	{
		printf("decoded nfapi_lbt_dl_indication\n");
	}
}

void send_nb_harq_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_nb_harq_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_NB_HARQ_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_nb_harq_ind((phy_info->config), &ind);
}
void recieve_nb_harq_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_nb_harq_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_NB_HARQ_INDICATION);
	if(ind.header.message_id == NFAPI_NB_HARQ_INDICATION)
	{
		printf("decoded nfapi_nb_harq_indication\n");
	}
}

void send_nrach_ind(phy_info_t* phy_info, uint16_t sfn_sf)
{
	nfapi_nrach_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_NRACH_INDICATION;
	ind.header.phy_id = phy_info->phy_id;
	ind.sfn_sf = sfn_sf;

	nfapi_pnf_p7_nrach_ind((phy_info->config), &ind);
}
void recieve_nrach_ind(int p7Sock, int phy_id, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_nrach_indication_t ind;
	recv_p7_message(p7Sock, &(ind.header), sizeof(ind));
	
	CU_ASSERT_EQUAL(ind.header.message_id, NFAPI_NRACH_INDICATION);
	if(ind.header.message_id == NFAPI_NRACH_INDICATION)
	{
		printf("decoded nfapi_nrach_indication\n");
	}
}

void pnf_test_start_connect(void) 
{
	pnf_info_t pnf_info;
	pnf_info.num_phys = 1;
	pnf_info.phys[0].pnf_p7_port = 9345;
	pnf_info.phys[0].pnf_p7_addr = "127.0.0.1";
	pnf_info.phys[0].vnf_p7_port = 8875;
	pnf_info.phys[0].vnf_p7_addr = "127.0.0.1";

	char* vnf_addr = "127.0.0.1";
	int vnf_port = 6699;
	
	int p5ListenSock = create_p5_listen_socket(vnf_addr, vnf_port);
	printf("p5ListenSock %d %d\n", p5ListenSock, errno);
	listen(p5ListenSock, 2);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;
	config->pnf_start_req = &test_pnf_start_request;
	config->pnf_stop_req = &test_pnf_stop_request;
	config->param_req = &test_param_request;
	config->config_req = &test_config_request;
	config->start_req = &test_start_request;
	config->stop_req = &test_stop_request;

	config->user_data = &pnf_info;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);;
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);
	printf("p5 connection accepted %d %d\n", p5Sock, errno);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);
	
	send_pnf_start_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_start_response(p5Sock);

	send_param_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_param_response(p5Sock);
	

	send_config_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize, &pnf_info);
	receive_config_response(p5Sock);

	send_start_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_start_response(p5Sock);

	sleep(2);

	int p7Sock = socket(AF_INET, SOCK_DGRAM, 0);

	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(pnf_info.phys[0].vnf_p7_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		bind(p7Sock, (struct sockaddr*)&addr, sizeof(addr));
	}

	printf("Sending p7 messages too %s:%d\n", pnf_info.phys[0].pnf_p7_addr, pnf_info.phys[0].pnf_p7_port);

	struct sockaddr_in p7_addr;
	p7_addr.sin_family = AF_INET;
	p7_addr.sin_port = htons(pnf_info.phys[0].pnf_p7_port);
	p7_addr.sin_addr.s_addr = inet_addr(pnf_info.phys[0].pnf_p7_addr);
	socklen_t p7_addrSize = sizeof(p7_addr);

	send_dl_node_sync(p7Sock, 0, &p7_addr, p7_addrSize);
	receive_ul_node_sync(p7Sock);


#define SFNSF(_sfn, _sf) ((_sfn << 4) + _sf)

	uint16_t sfn_sf = SFNSF(500, 3); // sfn:500 sf:3
	pnf_info.phys[0].sfn_sf = sfn_sf;
	send_subframe_indication(&(pnf_info.phys[0]));

	// should generate dummy messages

	usleep(500);

	sfn_sf = SFNSF(500, 4); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;

	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_ul_config_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_hi_dci0_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_tx_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);


	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	// should send the cached messages

	send_harq_ind(&pnf_info.phys[0], sfn_sf);
	recieve_harq_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_crc_ind(&pnf_info.phys[0], sfn_sf);
	recieve_crc_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rx_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rx_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rach_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rach_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_srs_ind(&pnf_info.phys[0], sfn_sf);
	recieve_srs_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_sr_ind(&pnf_info.phys[0], sfn_sf);
	recieve_sr_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_cqi_ind(&pnf_info.phys[0], sfn_sf);
	recieve_cqi_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_lbt_dl_ind(&pnf_info.phys[0], sfn_sf);
	recieve_lbt_dl_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_nb_harq_ind(&pnf_info.phys[0], sfn_sf);
	recieve_nb_harq_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_nrach_ind(&pnf_info.phys[0], sfn_sf);
	recieve_nrach_ind(p7Sock, 0, &p7_addr, p7_addrSize);


	// send out of window dl_config, should expect the timing info
	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, SFNSF(600, 0));

	sfn_sf = SFNSF(500, 5); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;
	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	receive_timing_info(p7Sock);

	send_stop_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_stop_response(p5Sock);



	send_pnf_stop_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_stop_response(p5Sock);

	nfapi_pnf_stop(config);

	int* result;
	pthread_join(thread, (void**)&result);
	//CU_ASSERT_NOT_EQUAL(*result, -1);
	CU_ASSERT_EQUAL(test_param_request_called, 1);
	CU_ASSERT_EQUAL(test_config_request_called, 1);

	close(p5Sock);
	close(p5ListenSock);

}
void pnf_test_start_connect_ipv6(void) 
{
	pnf_info_t pnf_info;
	pnf_info.num_phys = 1;
	pnf_info.phys[0].pnf_p7_port = 9345;
	pnf_info.phys[0].pnf_p7_addr = "::1";
	pnf_info.phys[0].vnf_p7_port = 8875;
	pnf_info.phys[0].vnf_p7_addr = "::1";

	char* vnf_addr = "::1";
	int vnf_port = 6698;
	
	int p5ListenSock = create_p5_ipv6_listen_socket(vnf_addr, vnf_port);
	printf("p5ListenSock ipv6 %d %d\n", p5ListenSock, errno);
	int listen_result = listen(p5ListenSock, 2);
	printf("Listen result %d\n", listen_result);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;
	config->pnf_start_req = &test_pnf_start_request;
	config->pnf_stop_req = &test_pnf_stop_request;
	config->param_req = &test_param_request;
	config->config_req = &test_config_request;
	config->start_req = &test_start_request;
	config->stop_req = &test_stop_request;

	config->user_data = &pnf_info;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	struct sockaddr_in6 addr;
	socklen_t addrSize = sizeof(addr);;
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);
	printf("p5 connection accepted %d %d\n", p5Sock, errno);
	
	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);
	
	send_pnf_start_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_start_response(p5Sock);

	send_param_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_param_response(p5Sock);
}
        
void pnf_test_state_pnf_config_invalid_state()
{
	char* vnf_addr = "127.0.0.1";
	int vnf_port = 4546;
	
	int p5ListenSock = create_p5_listen_socket(vnf_addr, vnf_port);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	listen(p5ListenSock, 2);
	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);
	printf("%s p5Sock %d errno %d\n", __FUNCTION__, p5Sock, errno);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_INVALID_STATE);
	
	nfapi_pnf_stop(config);

	int* result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(test_param_request_called, 1);
	CU_ASSERT_EQUAL(test_config_request_called, 1);

	close(p5Sock);
	close(p5ListenSock);

}

void pnf_test_state_pnf_param_invalid_state()
{
	pnf_info_t pnf_info;

	char* vnf_addr = "127.0.0.1";
	int vnf_port = 4547;
	
	int p5ListenSock = create_p5_listen_socket(vnf_addr, vnf_port);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;

	config->user_data = &pnf_info;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	listen(p5ListenSock, 2);
	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);
	
	nfapi_pnf_stop(config);

	int* result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(test_param_request_called, 1);
	CU_ASSERT_EQUAL(test_config_request_called, 2);

	close(p5Sock);
	close(p5ListenSock);

}
void pnf_test_p7_segmentation_test_1(void) 
{
	pnf_info_t pnf_info;
	pnf_info.num_phys = 1;
	pnf_info.phys[0].pnf_p7_port = 9348;
	pnf_info.phys[0].pnf_p7_addr = "127.0.0.1";
	pnf_info.phys[0].vnf_p7_port = 8878;
	pnf_info.phys[0].vnf_p7_addr = "127.0.0.1";

	char* vnf_addr = "127.0.0.1";
	int vnf_port = 6699;
	
	int p5ListenSock = create_p5_listen_socket(vnf_addr, vnf_port);
	printf("p5ListenSock %d %d\n", p5ListenSock, errno);
	listen(p5ListenSock, 2);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;
	config->pnf_start_req = &test_pnf_start_request;
	config->pnf_stop_req = &test_pnf_stop_request;
	config->param_req = &test_param_request;
	config->config_req = &test_config_request;
	config->start_req = &test_start_request;
	config->stop_req = &test_stop_request;

	config->user_data = &pnf_info;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);;
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);
	printf("p5 connection accepted %d %d\n", p5Sock, errno);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);
	
	send_pnf_start_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_start_response(p5Sock);

	send_param_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_param_response(p5Sock);
	

	send_config_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize, &pnf_info);
	receive_config_response(p5Sock);

	send_start_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_start_response(p5Sock);

	sleep(2);

	int p7Sock = socket(AF_INET, SOCK_DGRAM, 0);

	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(pnf_info.phys[0].vnf_p7_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		bind(p7Sock, (struct sockaddr*)&addr, sizeof(addr));
	}

	printf("Sending p7 messages too %s:%d\n", pnf_info.phys[0].pnf_p7_addr, pnf_info.phys[0].pnf_p7_port);

	struct sockaddr_in p7_addr;
	p7_addr.sin_family = AF_INET;
	p7_addr.sin_port = htons(pnf_info.phys[0].pnf_p7_port);
	p7_addr.sin_addr.s_addr = inet_addr(pnf_info.phys[0].pnf_p7_addr);
	socklen_t p7_addrSize = sizeof(p7_addr);

	send_dl_node_sync(p7Sock, 0, &p7_addr, p7_addrSize);
	receive_ul_node_sync(p7Sock);


#define SFNSF(_sfn, _sf) ((_sfn << 4) + _sf)

	uint16_t sfn_sf = SFNSF(500, 3); // sfn:500 sf:3
	pnf_info.phys[0].sfn_sf = sfn_sf;
	send_subframe_indication(&(pnf_info.phys[0]));

	// should generate dummy messages

	usleep(500);

	sfn_sf = SFNSF(500, 4); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;

	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_ul_config_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_hi_dci0_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_tx_req_1(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);


	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	// should send the cached messages

	send_harq_ind(&pnf_info.phys[0], sfn_sf);
	recieve_harq_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_crc_ind(&pnf_info.phys[0], sfn_sf);
	recieve_crc_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rx_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rx_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rach_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rach_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_srs_ind(&pnf_info.phys[0], sfn_sf);
	recieve_srs_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_sr_ind(&pnf_info.phys[0], sfn_sf);
	recieve_sr_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_cqi_ind(&pnf_info.phys[0], sfn_sf);
	recieve_cqi_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_lbt_dl_ind(&pnf_info.phys[0], sfn_sf);
	recieve_lbt_dl_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_nb_harq_ind(&pnf_info.phys[0], sfn_sf);
	recieve_nb_harq_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_nrach_ind(&pnf_info.phys[0], sfn_sf);
	recieve_nrach_ind(p7Sock, 0, &p7_addr, p7_addrSize);


	// send out of window dl_config, should expect the timing info
	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, SFNSF(600, 0));

	sfn_sf = SFNSF(500, 5); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;
	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	receive_timing_info(p7Sock);

	nfapi_pnf_p7_stop(pnf_info.phys[0].config);
	int* result;
	pthread_join(pnf_info.phys[0].thread, (void**)&result);
	close(p7Sock);

	send_stop_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_stop_response(p5Sock);

	send_pnf_stop_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_stop_response(p5Sock);

	nfapi_pnf_stop(config);

	pthread_join(thread, (void**)&result);
	//CU_ASSERT_NOT_EQUAL(*result, -1);
	CU_ASSERT_EQUAL(test_param_request_called, 1);
	CU_ASSERT_EQUAL(test_config_request_called, 1);

	close(p5Sock);
	close(p5ListenSock);

}
void pnf_test_p7_segmentation_test_2(void) 
{
	pnf_info_t pnf_info;
	pnf_info.num_phys = 1;
	pnf_info.phys[0].pnf_p7_port = 9348;
	pnf_info.phys[0].pnf_p7_addr = "127.0.0.1";
	pnf_info.phys[0].vnf_p7_port = 8878;
	pnf_info.phys[0].vnf_p7_addr = "127.0.0.1";

	char* vnf_addr = "127.0.0.1";
	int vnf_port = 6699;
	
	int p5ListenSock = create_p5_listen_socket(vnf_addr, vnf_port);
	printf("p5ListenSock %d %d\n", p5ListenSock, errno);
	listen(p5ListenSock, 2);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;
	config->pnf_start_req = &test_pnf_start_request;
	config->pnf_stop_req = &test_pnf_stop_request;
	config->param_req = &test_param_request;
	config->config_req = &test_config_request;
	config->start_req = &test_start_request;
	config->stop_req = &test_stop_request;

	config->user_data = &pnf_info;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);;
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);
	printf("p5 connection accepted %d %d\n", p5Sock, errno);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);
	
	send_pnf_start_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_start_response(p5Sock);

	send_param_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_param_response(p5Sock);
	

	send_config_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize, &pnf_info);
	receive_config_response(p5Sock);

	send_start_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_start_response(p5Sock);

	sleep(2);

	int p7Sock = socket(AF_INET, SOCK_DGRAM, 0);

	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(pnf_info.phys[0].vnf_p7_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		bind(p7Sock, (struct sockaddr*)&addr, sizeof(addr));
	}

	printf("Sending p7 messages too %s:%d\n", pnf_info.phys[0].pnf_p7_addr, pnf_info.phys[0].pnf_p7_port);

	struct sockaddr_in p7_addr;
	p7_addr.sin_family = AF_INET;
	p7_addr.sin_port = htons(pnf_info.phys[0].pnf_p7_port);
	p7_addr.sin_addr.s_addr = inet_addr(pnf_info.phys[0].pnf_p7_addr);
	socklen_t p7_addrSize = sizeof(p7_addr);

	send_dl_node_sync(p7Sock, 0, &p7_addr, p7_addrSize);
	receive_ul_node_sync(p7Sock);


#define SFNSF(_sfn, _sf) ((_sfn << 4) + _sf)

	uint16_t sfn_sf = SFNSF(500, 3); // sfn:500 sf:3
	pnf_info.phys[0].sfn_sf = sfn_sf;
	send_subframe_indication(&(pnf_info.phys[0]));

	// should generate dummy messages

	usleep(500);

	sfn_sf = SFNSF(500, 4); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;

	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_ul_config_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_hi_dci0_req(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);
	send_tx_req_2(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);


	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	// should send the cached messages

	send_harq_ind(&pnf_info.phys[0], sfn_sf);
	recieve_harq_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_crc_ind(&pnf_info.phys[0], sfn_sf);
	recieve_crc_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rx_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rx_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rach_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rach_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_srs_ind(&pnf_info.phys[0], sfn_sf);
	recieve_srs_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_sr_ind(&pnf_info.phys[0], sfn_sf);
	recieve_sr_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_cqi_ind(&pnf_info.phys[0], sfn_sf);
	recieve_cqi_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_lbt_dl_ind(&pnf_info.phys[0], sfn_sf);
	recieve_lbt_dl_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	// send out of window dl_config, should expect the timing info
	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, SFNSF(600, 0));

	sfn_sf = SFNSF(500, 5); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;
	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	receive_timing_info(p7Sock);

	send_stop_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_stop_response(p5Sock);

	nfapi_pnf_p7_stop(pnf_info.phys[0].config);
	int* result;
	pthread_join(pnf_info.phys[0].thread, (void**)&result);
	close(p7Sock);


	send_pnf_stop_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_stop_response(p5Sock);

	nfapi_pnf_stop(config);

	pthread_join(thread, (void**)&result);
	//CU_ASSERT_NOT_EQUAL(*result, -1);
	CU_ASSERT_EQUAL(test_param_request_called, 1);
	CU_ASSERT_EQUAL(test_config_request_called, 1);

	close(p5Sock);
	close(p5ListenSock);

}
void pnf_test_p7_segmentation_test_3(void) 
{
	pnf_info_t pnf_info;
	pnf_info.num_phys = 1;
	pnf_info.phys[0].pnf_p7_port = 9348;
	pnf_info.phys[0].pnf_p7_addr = "127.0.0.1";
	pnf_info.phys[0].vnf_p7_port = 8878;
	pnf_info.phys[0].vnf_p7_addr = "127.0.0.1";

	char* vnf_addr = "127.0.0.1";
	int vnf_port = 6699;
	
	int p5ListenSock = create_p5_listen_socket(vnf_addr, vnf_port);
	printf("p5ListenSock %d %d\n", p5ListenSock, errno);
	listen(p5ListenSock, 2);

	test_param_request_called = 0;
	test_config_request_called = 0;

	nfapi_pnf_config_t* config = nfapi_pnf_config_create();
	config->vnf_ip_addr = vnf_addr;
	config->vnf_p5_port = vnf_port;
	config->pnf_param_req = &test_pnf_param_request;
	config->pnf_config_req = &test_pnf_config_request;
	config->pnf_start_req = &test_pnf_start_request;
	config->pnf_stop_req = &test_pnf_stop_request;
	config->param_req = &test_param_request;
	config->config_req = &test_config_request;
	config->start_req = &test_start_request;
	config->stop_req = &test_stop_request;

	config->user_data = &pnf_info;

	pthread_t thread;
	pthread_create(&thread, NULL, &pnf_test_start_thread, config);

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);;
	int	p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);
	printf("p5 connection accepted %d %d\n", p5Sock, errno);

	send_pnf_param_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_param_response(p5Sock, NFAPI_MSG_OK);

	send_pnf_config_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_config_response(p5Sock);
	
	send_pnf_start_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_start_response(p5Sock);

	send_param_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_param_response(p5Sock);
	

	send_config_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize, &pnf_info);
	receive_config_response(p5Sock);

	send_start_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_start_response(p5Sock);

	sleep(2);

	int p7Sock = socket(AF_INET, SOCK_DGRAM, 0);

	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(pnf_info.phys[0].vnf_p7_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		bind(p7Sock, (struct sockaddr*)&addr, sizeof(addr));
	}

	printf("Sending p7 messages too %s:%d\n", pnf_info.phys[0].pnf_p7_addr, pnf_info.phys[0].pnf_p7_port);

	struct sockaddr_in p7_addr;
	p7_addr.sin_family = AF_INET;
	p7_addr.sin_port = htons(pnf_info.phys[0].pnf_p7_port);
	p7_addr.sin_addr.s_addr = inet_addr(pnf_info.phys[0].pnf_p7_addr);
	socklen_t p7_addrSize = sizeof(p7_addr);

	send_dl_node_sync(p7Sock, 0, &p7_addr, p7_addrSize);
	receive_ul_node_sync(p7Sock);


#define SFNSF(_sfn, _sf) ((_sfn << 4) + _sf)

	uint16_t sfn_sf = SFNSF(500, 3); // sfn:500 sf:3
	pnf_info.phys[0].sfn_sf = sfn_sf;
	send_subframe_indication(&(pnf_info.phys[0]));

	// should generate dummy messages

	usleep(500);

	sfn_sf = SFNSF(500, 4); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;


	send_dl_subframe_msgs_interleaved(p7Sock, 0, &p7_addr, p7_addrSize, sfn_sf);


	usleep(500);

	send_subframe_indication(&(pnf_info.phys[0]));
	// should send the cached messages

	send_harq_ind(&pnf_info.phys[0], sfn_sf);
	recieve_harq_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_crc_ind(&pnf_info.phys[0], sfn_sf);
	recieve_crc_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rx_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rx_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_rach_ind(&pnf_info.phys[0], sfn_sf);
	recieve_rach_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_srs_ind(&pnf_info.phys[0], sfn_sf);
	recieve_srs_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_sr_ind(&pnf_info.phys[0], sfn_sf);
	recieve_sr_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	send_cqi_ind(&pnf_info.phys[0], sfn_sf);
	recieve_cqi_ind(p7Sock, 0, &p7_addr, p7_addrSize);
	
	send_lbt_dl_ind(&pnf_info.phys[0], sfn_sf);
	recieve_lbt_dl_ind(p7Sock, 0, &p7_addr, p7_addrSize);

	// send out of window dl_config, should expect the timing info
	printf("Sending late dl config req\n");
	send_dl_config_req(p7Sock, 0, &p7_addr, p7_addrSize, SFNSF(600, 0));

	sfn_sf = SFNSF(500, 5); // sfn:500 sf:4
	pnf_info.phys[0].sfn_sf = sfn_sf;
	sleep(2);

	printf("Sending subframe indication\n");
	send_subframe_indication(&(pnf_info.phys[0]));
	printf("Waitning for timing info\n");
	receive_timing_info(p7Sock);

	printf("Sending stop request\n");
	send_stop_request(p5Sock, 0, (struct sockaddr*)&addr, addrSize);
	receive_stop_response(p5Sock);

	printf("Waiting for P7 thread to stop\n");
	nfapi_pnf_p7_stop(pnf_info.phys[0].config);
	int* result;
	pthread_join(pnf_info.phys[0].thread, (void**)&result);
	close(p7Sock);


	send_pnf_stop_request(p5Sock, (struct sockaddr*)&addr, addrSize);
	receive_pnf_stop_response(p5Sock);

	nfapi_pnf_stop(config);

	pthread_join(thread, (void**)&result);
	//CU_ASSERT_NOT_EQUAL(*result, -1);
	CU_ASSERT_EQUAL(test_param_request_called, 1);
	CU_ASSERT_EQUAL(test_config_request_called, 1);

	close(p5Sock);
	close(p5ListenSock);

}

/************* Test Runner Code goes here **************/

int main ( void )
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if ( CUE_SUCCESS != CU_initialize_registry() )
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite( "pnf_test_suite", init_suite, clean_suite );
   if ( NULL == pSuite ) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the suite */
   if ( (NULL == CU_add_test(pSuite, "pnf_test_start_no_config", pnf_test_start_no_config)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_start_no_ip", pnf_test_start_no_ip)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_start_invalid_ip", pnf_test_start_invalid_ip)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_start_no_connection", pnf_test_start_no_connection)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_start_connect", pnf_test_start_connect)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_start_pnf_config_invalid_state", pnf_test_state_pnf_config_invalid_state)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_start_pnf_param_invalid_state", pnf_test_state_pnf_param_invalid_state)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_p7_segmentation_test_1", pnf_test_p7_segmentation_test_1)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_p7_segmentation_test_2", pnf_test_p7_segmentation_test_2)) ||
        (NULL == CU_add_test(pSuite, "pnf_test_p7_segmentation_test_3", pnf_test_p7_segmentation_test_3))
		)
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   //(NULL == CU_add_test(pSuite, "pnf_test_start_connect_ipv6", pnf_test_start_connect_ipv6)) ||

   // Run all tests using the basic interface
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_set_output_filename("pnf_unit_test_results.xml");
   CU_basic_run_tests();

	CU_pSuite s = CU_get_registry()->pSuite;
	int count = 0;
	while(s)
	{
		CU_pTest t = s->pTest;
		while(t)
		{
			count++;
			t = t->pNext;
		}
		s = s->pNext;
	}

	printf("%d..%d\n", 1, count);



	s = CU_get_registry()->pSuite;
	count = 1;
	while(s)
	{
		CU_pTest t = s->pTest;
		while(t)
		{
			int pass = 1;
			CU_FailureRecord* failures = CU_get_failure_list();
			while(failures)
			{
				if(strcmp(failures->pSuite->pName, s->pName) == 0 &&
				   strcmp(failures->pTest->pName, t->pName) == 0)
				{
					pass = 0;
					failures = 0;
				}
				else
				{
					failures = failures->pNext;
				}
			}

			if(pass)
				printf("ok %d - %s:%s\n", count, s->pName, t->pName);
			else 
				printf("not ok %d - %s:%s\n", count, s->pName, t->pName);

			count++;
			t = t->pNext;
		}
		s = s->pNext;
	}

   // Clean up registry and return 
   CU_cleanup_registry();
   return CU_get_error();

}
