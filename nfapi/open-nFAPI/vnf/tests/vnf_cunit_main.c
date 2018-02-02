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
#include "nfapi_vnf_interface.h"
#include "nfapi.h"
#include <stdio.h>  // for printf
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include "debug.h"
#include <unistd.h>
#include <stdlib.h>

/* Test Suite setup and cleanup functions: */

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

#define MAX_PACKED_MESSAGE_SIZE	8192

#define SFNSF2SFN(_sfnsf) ((_sfnsf) >> 4)
#define SFNSF2SF(_sfnsf) ((_sfnsf) & 0xF)

typedef struct
{
	uint8_t enabled;
	uint8_t started;
	uint16_t phy_id;

	int p7_rx_port;
	char* p7_rx_addr;

	struct sockaddr_in p7_rx_sockaddr;
	int p7_rx_sock;

	struct sockaddr_in p7_tx_sockaddr;
	int p7_tx_sock;
} pnf_test_config_phy_t;

typedef struct
{
	uint8_t enabled;
	
	char* vnf_p5_addr;
	int vnf_p5_port;

	struct sockaddr_in p5_tx_sockaddr;
	int p5_sock;

	int p7_rx_port_base;

	pnf_test_config_phy_t phys[4];

} pnf_test_config_t;

pnf_test_config_t pnf_test_config[5];

typedef struct
{
	uint8_t enabled;
	uint8_t state;
	uint16_t p5_idx;
	uint16_t phy_id;

	uint8_t vnf_idx;
	//struct sockaddr_in p7_rx_sockaddr;
	struct sockaddr_in p7_tx_sockaddr;

} vnf_test_config_phy_t;

typedef struct 
{
	uint8_t enabled;
	pthread_t thread;

	char* vnf_p7_addr;
	int vnf_p7_port;

	struct sockaddr_in p7_rx_sockaddr;

	nfapi_vnf_p7_config_t* config;

	uint8_t max_phys;
	uint8_t phy_count;
	//uint8_t phy_ids[4];
	
} vnf_test_config_vnf_t;

typedef struct  
{
	char* p5_addr;
	int p5_port;

	pthread_t thread;

	vnf_test_config_vnf_t vnfs[2];

	uint8_t phy_count;
	vnf_test_config_phy_t phys[5];

	nfapi_vnf_config_t* p5_vnf_config;

} vnf_test_config_t;

vnf_test_config_t vnf_test_config[5];

void reset_test_configs()
{
	memset(&pnf_test_config, 0, sizeof(pnf_test_config));
	memset(&vnf_test_config, 0, sizeof(vnf_test_config));
}

void pnf_create_p5_sock(pnf_test_config_t* config)
{
	config->p5_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
	config->p5_tx_sockaddr.sin_family = AF_INET;
	config->p5_tx_sockaddr.sin_port = htons(config->vnf_p5_port);
	config->p5_tx_sockaddr.sin_addr.s_addr = inet_addr(config->vnf_p5_addr);
}

void pnf_p5_connect(pnf_test_config_t* config)
{
	int connect_result = connect(config->p5_sock, (struct sockaddr *)&config->p5_tx_sockaddr, sizeof(config->p5_tx_sockaddr) );

	printf("connect_result %d %d\n", connect_result, errno);
}

pnf_test_config_phy_t* find_pnf_phy_config(pnf_test_config_t* config, uint16_t phy_id)
{
	int i = 0;
	for(i = 0; i < 4; ++i)
	{
		if(config->phys[i].phy_id == phy_id)
			return &(config->phys[i]);
	}

	return 0;
}

vnf_test_config_phy_t* find_vnf_phy_config(vnf_test_config_t* config, uint16_t phy_id)
{
	int i = 0;
	for(i = 0; i < 4; ++i)
	{
		if(config->phys[i].phy_id == phy_id)
			return &(config->phys[i]);
	}

	return 0;
}




/************* Test case functions ****************/


void vnf_test_start_no_config(void) 
{
	int result = nfapi_vnf_start(0);
	CU_ASSERT_EQUAL(result, -1);
}

void* vnf_test_start_thread(void* ptr)
{
	int result = nfapi_vnf_start((nfapi_vnf_config_t*)ptr);
	return (void*)(intptr_t)result;
}

int pnf_connection_indication_called = 0;
int pnf_connection_indication(nfapi_vnf_config_t* config, int p5_idx)
{
	printf("[VNF] pnf_connection_indication p5_idx:%d\n", p5_idx);
	pnf_connection_indication_called++;

	nfapi_pnf_param_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PNF_PARAM_REQUEST;
	req.header.message_length = 0;

	nfapi_vnf_pnf_param_req(config, p5_idx, &req);
	
	return 1;
}

int pnf_disconnect_indication(nfapi_vnf_config_t* config, int p5_idx)
{
	printf("[VNF] pnf_disconnect_indication p5_idx:%d\n", p5_idx);
	pnf_connection_indication_called++;

	vnf_test_config_t* test_config = (vnf_test_config_t*)(config->user_data);
	printf("[VNF] pnf_disconnect_indication user_data %p\n", config->user_data);

	int i = 0;
	for(i = 0; i < test_config->phy_count; ++i)
	{
		vnf_test_config_phy_t* phy = &test_config->phys[i];
		vnf_test_config_vnf_t* vnf = &test_config->vnfs[phy->p5_idx];

		// need to send stop request/response

		nfapi_vnf_p7_del_pnf((vnf->config), phy->phy_id);
		vnf->phy_count--;
	}


	for(i = 0; i < 2; ++i)
	{
		vnf_test_config_vnf_t* vnf = &test_config->vnfs[i];
	printf("[VNF] pnf_disconnect_indication phy_count:%d\n", vnf->phy_count);
		if(vnf->enabled == 1)
		{
			nfapi_vnf_p7_stop(vnf->config);
			//vnf->config->terminate = 1;

			int* result;
	printf("[VNF] waiting for vnf p7 thread to exit\n");
			pthread_join((vnf->thread), (void**)&result);
			CU_ASSERT_EQUAL(result, 0);

			vnf->enabled = 0;
		}
	}
	
	return 1;
}

int send_p5_message(int p5Sock, nfapi_p4_p5_message_header_t* msg, unsigned msg_size, struct sockaddr_in* addr, socklen_t addr_size)
{
	char buffer[256];
	int encoded_size = nfapi_p5_message_pack(msg, msg_size, buffer, sizeof(buffer), 0);
	int result = sendto(p5Sock, buffer, encoded_size, 0, (struct sockaddr*)addr, addr_size);
	(void)result;
	return 0;
}

int recv_p5_message(int p5Sock, nfapi_p4_p5_message_header_t* msg, unsigned msg_size)
{
	struct sockaddr_in addr2;
	socklen_t addr2len;
	char buffer2[256];
	int result2 = recvfrom(p5Sock, buffer2, sizeof(buffer2), 0, (struct sockaddr*)&addr2, &addr2len); 
		
	int unpack_result = nfapi_p5_message_unpack(buffer2, result2, msg, msg_size, 0); //, sizeof(resp));
	(void)unpack_result;
	return 0;
}

int send_pnf_param_response(pnf_test_config_t* config) //int p5Sock, struct sockaddr_in* addr, socklen_t addr_size)
{
	nfapi_pnf_param_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;

	resp.header.message_length = 0;
	resp.error_code = NFAPI_MSG_OK;

	//send_p5_message(p5Sock, &resp, sizeof(resp), addr, addr_size);
	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}

int create_p7_rx_socket(const char* addr, int port)
{
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);

	if(addr == 0)
		sock_addr.sin_addr.s_addr = INADDR_ANY;
	else
		sock_addr.sin_addr.s_addr = inet_addr(addr);

	int bind_result = bind(fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	if(bind_result != 0)
	{
		printf("Failed to bind p7 rx socket %d(%d) to %s:%d\n", bind_result, errno, addr, port);
	}

	
	return fd;
}

int create_p7_tx_socket()
{
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	return fd;
}

int send_param_response(pnf_test_config_t* config, uint16_t phy_id)
{
	nfapi_param_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PARAM_RESPONSE;
	resp.header.phy_id = phy_id;

	pnf_test_config_phy_t* phy = find_pnf_phy_config(config, phy_id);

	resp.error_code = NFAPI_MSG_OK;

	resp.nfapi_config.p7_pnf_port.tl.tag = NFAPI_NFAPI_P7_PNF_PORT_TAG;
	resp.nfapi_config.p7_pnf_port.value = phy->p7_rx_port;

	struct sockaddr_in pnf_p7_sockaddr;
	pnf_p7_sockaddr.sin_addr.s_addr = inet_addr(phy->p7_rx_addr);

	resp.nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
	memcpy(&resp.nfapi_config.p7_pnf_address_ipv4.address[0], &pnf_p7_sockaddr.sin_addr.s_addr, 4);

	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}

int receive_pnf_param_request(pnf_test_config_t* config)
{
	nfapi_pnf_param_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_PNF_PARAM_REQUEST);
	if(req.header.message_id == NFAPI_PNF_PARAM_REQUEST)
	{
		printf("[PNF] nfapi_pnf_param_request\n");
	}
	return 0;
}

int receive_param_request(pnf_test_config_t* config)
{
	nfapi_param_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_PARAM_REQUEST);
	if(req.header.message_id == NFAPI_PARAM_REQUEST)
	{
		printf("[PNF] nfapi_param_request phy_id:%d\n", req.header.phy_id);
	}
	else
	{
		printf("[PNF] ERROR, unexpected message %d phy_id:%d\n", req.header.message_id, req.header.phy_id);
	}
	return req.header.phy_id;
}

int send_pnf_config_response(pnf_test_config_t* config)
{
	nfapi_pnf_config_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
	resp.header.message_length = 0;
	resp.error_code = NFAPI_MSG_OK;

	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}

int send_config_response(pnf_test_config_t* config, uint16_t phy_id)
{
	nfapi_config_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_CONFIG_RESPONSE;
	resp.header.message_length = 0;
	resp.header.phy_id = phy_id;
	resp.error_code = NFAPI_MSG_OK;

	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}


int receive_pnf_config_request(pnf_test_config_t* config)
{
	nfapi_pnf_config_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_PNF_CONFIG_REQUEST);
	if(req.header.message_id == NFAPI_PNF_CONFIG_REQUEST)
	{
		printf("decoded nfapi_pnf_config_request\n");

		int i = 0;
		for(i = 0; i < req.pnf_phy_rf_config.number_phy_rf_config_info; ++i)
		{
			nfapi_phy_rf_config_info_t* info = &(req.pnf_phy_rf_config.phy_rf_config[i]);
			printf("adding pnf phy %d id %d\n",  i, info->phy_id);

			config->phys[i].enabled = 1;
			config->phys[i].phy_id = info->phy_id;
		}
	}
	return 0;
}

int receive_config_request(pnf_test_config_t* config)
{
	nfapi_config_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_CONFIG_REQUEST);
	if(req.header.message_id == NFAPI_CONFIG_REQUEST)
	{
		printf("[PNF] nfapi_config_request phy_id:%d\n", req.header.phy_id);

		pnf_test_config_phy_t* phy = find_pnf_phy_config(config, req.header.phy_id);

		// todo verify that this is the same as a vnf config?
		phy->p7_tx_sockaddr.sin_family = AF_INET;
		phy->p7_tx_sockaddr.sin_port = htons(req.nfapi_config.p7_vnf_port.value);
		memcpy(&phy->p7_tx_sockaddr.sin_addr.s_addr, &req.nfapi_config.p7_vnf_address_ipv4.address, 4);
		phy->p7_tx_sock = create_p7_tx_socket();


		printf("[PNF] vnf p7 address %s:%d\n", inet_ntoa(phy->p7_tx_sockaddr.sin_addr), ntohs(phy->p7_tx_sockaddr.sin_port));

		return req.header.phy_id;
	}

	return NFAPI_PHY_ID_NA;
}

int send_pnf_start_response(pnf_test_config_t* config)
{
	nfapi_pnf_start_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_START_RESPONSE;
	resp.header.message_length = 0;
	resp.error_code = NFAPI_MSG_OK;

	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}

int send_start_response(pnf_test_config_t* config, uint16_t phy_id)
{
	nfapi_start_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_START_RESPONSE;
	resp.header.message_length = 0;
	resp.header.phy_id  = phy_id;
	resp.error_code = NFAPI_MSG_OK;

	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}

int receive_pnf_start_request(pnf_test_config_t* config)
{
	nfapi_pnf_start_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_PNF_START_REQUEST);
	if(req.header.message_id == NFAPI_PNF_START_REQUEST)
	{
		printf("decoded nfapi_pnf_start_request\n");

		int phy_idx = 0;
		for(phy_idx = 0; phy_idx < 4; ++phy_idx)
		{
			if(config->phys[phy_idx].enabled)
			{
				pnf_test_config_phy_t* phy = &(config->phys[phy_idx]);
				phy->p7_rx_port = config->p7_rx_port_base + phy_idx;
				phy->p7_rx_addr = "127.0.0.1"; 
			}
		}
	}
	return 0;
}

int receive_start_request(pnf_test_config_t* config)
{
	nfapi_start_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_START_REQUEST);
	if(req.header.message_id == NFAPI_START_REQUEST)
	{
		printf("[PNF] nfapi_start_request\n");

		pnf_test_config_phy_t* phy = find_pnf_phy_config(config, req.header.phy_id);
		printf("[PNF] creating p7 rx socket %s:%d\n", phy->p7_rx_addr, phy->p7_rx_port);
		phy->p7_rx_sock = create_p7_rx_socket(phy->p7_rx_addr, phy->p7_rx_port);
		phy->started = 1;

	}

	return req.header.phy_id;
}

int send_pnf_stop_response(pnf_test_config_t* config, uint16_t phy_id)
{
	printf("pnf_stop_response\n");
	nfapi_pnf_stop_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_STOP_RESPONSE;
	resp.header.message_length = 0;
	resp.error_code = NFAPI_MSG_OK;

	send_p5_message(config->p5_sock, &resp.header, sizeof(resp), &(config->p5_tx_sockaddr), sizeof(config->p5_tx_sockaddr));
	return 0;
}

int receive_pnf_stop_request(pnf_test_config_t* config)
{
	printf("pnf_stop_request\n");
	nfapi_pnf_stop_request_t req;
	recv_p5_message(config->p5_sock, &req.header, sizeof(req));

	CU_ASSERT_EQUAL(req.header.message_id, NFAPI_PNF_STOP_REQUEST);
	if(req.header.message_id == NFAPI_PNF_STOP_REQUEST)
	{
		printf("decoded nfapi_pnf_stop_request\n");
	}
	return 0;
}
int pnf_param_response(nfapi_vnf_config_t* config, int p5_idx,nfapi_pnf_param_response_t* response)
{
	printf("[VNF] pnf_param_response p5_idx:%d\n", p5_idx);
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

	uint16_t phy_id;
	nfapi_vnf_allocate_phy(config, p5_idx, &phy_id);

	nfapi_pnf_config_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PNF_CONFIG_REQUEST;
	req.header.message_length = 0;
	req.header.phy_id = NFAPI_PHY_ID_NA;

	req.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
	req.pnf_phy_rf_config.number_phy_rf_config_info = 2;
	nfapi_vnf_allocate_phy(config, p5_idx, &req.pnf_phy_rf_config.phy_rf_config[0].phy_id);
	nfapi_vnf_allocate_phy(config, p5_idx, &req.pnf_phy_rf_config.phy_rf_config[1].phy_id);
	
	uint16_t i = vnf_test_config[0].phy_count;

	vnf_test_config[0].phys[i].enabled = 1;
	vnf_test_config[0].phys[i].p5_idx = p5_idx;
	vnf_test_config[0].phys[i].phy_id = req.pnf_phy_rf_config.phy_rf_config[0].phy_id;

	i++;

	vnf_test_config[0].phys[i].enabled = 1;
	vnf_test_config[0].phys[i].p5_idx = p5_idx;
	vnf_test_config[0].phys[i].phy_id = req.pnf_phy_rf_config.phy_rf_config[1].phy_id;

	i++;
	
	vnf_test_config[0].phy_count = i;

	nfapi_vnf_pnf_config_req(config, p5_idx, &req);

	return 0;
}

int pnf_config_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_response_t* response)
{
	printf("pnf_config_response\n");
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

	nfapi_pnf_start_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PNF_START_REQUEST;
	req.header.message_length = 0;
	req.header.phy_id = response->header.phy_id;

	nfapi_vnf_pnf_start_req(config, p5_idx, &req);

	return 0;
}


void* vnf_test_start_p7_thread(void* ptr)
{
	int result = nfapi_vnf_p7_start((nfapi_vnf_p7_config_t*)ptr);
	(void)result;
	return 0;
}

int test_subframe_indication(nfapi_vnf_p7_config_t* config, uint16_t phy_id, uint16_t sfn_sf)
{
	//printf("[VNF:%d] (%d:%d) SUBFRAME_IND\n", phy_id, SFNSF2SFN(sfn_sf), SFNSF2SF(sfn_sf));

	nfapi_dl_config_request_t dl_config_req;
	memset(&dl_config_req, 0, sizeof(dl_config_req));
	dl_config_req.header.message_id = NFAPI_DL_CONFIG_REQUEST;
	dl_config_req.header.phy_id = phy_id;
	dl_config_req.sfn_sf = sfn_sf;
	nfapi_vnf_p7_dl_config_req(config, &dl_config_req);
	
	nfapi_ul_config_request_t ul_config_req;
	memset(&ul_config_req, 0, sizeof(ul_config_req));
	ul_config_req.header.message_id = NFAPI_UL_CONFIG_REQUEST;
	ul_config_req.header.phy_id = phy_id;
	ul_config_req.sfn_sf = sfn_sf;
	nfapi_vnf_p7_ul_config_req(config, &ul_config_req);

	nfapi_hi_dci0_request_t hi_dci0_req;
	memset(&hi_dci0_req, 0, sizeof(hi_dci0_req));
	hi_dci0_req.header.message_id = NFAPI_HI_DCI0_REQUEST;
	hi_dci0_req.header.phy_id = phy_id;
	hi_dci0_req.sfn_sf = sfn_sf;
	nfapi_vnf_p7_hi_dci0_req(config, &hi_dci0_req);


	nfapi_tx_request_t tx_req;
	memset(&tx_req, 0, sizeof(tx_req));
	tx_req.header.message_id = NFAPI_TX_REQUEST;
	tx_req.header.phy_id = phy_id;
	tx_req.sfn_sf = sfn_sf;
	nfapi_vnf_p7_tx_req(config, &tx_req);
	return 0;
}

int test_harq_indication(nfapi_vnf_p7_config_t* config, nfapi_harq_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) HARQ_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}

int test_nb_harq_indication(nfapi_vnf_p7_config_t* config, nfapi_nb_harq_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) HARQ_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}


int test_crc_indication(nfapi_vnf_p7_config_t* config, nfapi_crc_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) CRC_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}
int test_rx_indication(nfapi_vnf_p7_config_t* config, nfapi_rx_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) RX_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}
int test_rach_indication(nfapi_vnf_p7_config_t* config, nfapi_rach_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) RACH_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}
int test_nrach_indication(nfapi_vnf_p7_config_t* config, nfapi_nrach_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) RACH_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}
int test_srs_indication(nfapi_vnf_p7_config_t* config, nfapi_srs_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) SRS_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}
int test_sr_indication(nfapi_vnf_p7_config_t* config, nfapi_sr_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) SR_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}
int test_cqi_indication(nfapi_vnf_p7_config_t* config, nfapi_cqi_indication_t* ind)
{
	//printf("[VNF:%d] (%d:%d) CQI_IND\n", ind->header.phy_id, SFNSF2SFN(ind->sfn_sf), SFNSF2SF(ind->sfn_sf));
	return 0;
}

int pnf_start_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_response_t* response)
{
	printf("[VNF] pnf_start_response p5_idx:%d\n", p5_idx);
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

	return 0;
}

int pnf_stop_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_stop_response_t* response)
{
	printf("pnf_stop_response\n");
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

	nfapi_vnf_p7_stop(vnf_test_config[0].vnfs[0].config);
	return 0;
}

int param_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_response_t* response)
{
	printf("[VNF] param_response p5_idx:%d phy_id:%d\n", p5_idx, response->header.phy_id);
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

	struct sockaddr_in addr;
	memcpy(&addr.sin_addr.s_addr, &response->nfapi_config.p7_pnf_address_ipv4.address, 4);
	printf("[VNF] param_response pnf p7 %s:%d\n", inet_ntoa(addr.sin_addr), response->nfapi_config.p7_pnf_port.value);

	// find the vnf phy configuration
	vnf_test_config_phy_t* phy = find_vnf_phy_config(&(vnf_test_config[0]), response->header.phy_id);
	
	// save the pnf p7 connection information
	phy->p7_tx_sockaddr.sin_port = response->nfapi_config.p7_pnf_port.value;
	memcpy(&phy->p7_tx_sockaddr.sin_addr.s_addr, &response->nfapi_config.p7_pnf_address_ipv4.address, 4);

	nfapi_config_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_CONFIG_REQUEST;
	req.header.phy_id = response->header.phy_id;

	vnf_test_config_vnf_t* vnf = &(vnf_test_config[0].vnfs[phy->vnf_idx]);
	req.nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
	memcpy(&req.nfapi_config.p7_vnf_address_ipv4.address, &vnf->p7_rx_sockaddr.sin_addr.s_addr, 4);

	req.nfapi_config.p7_vnf_port.tl.tag = NFAPI_NFAPI_P7_VNF_PORT_TAG;
	req.nfapi_config.p7_vnf_port.value = vnf->p7_rx_sockaddr.sin_port;

	nfapi_vnf_config_req(config, p5_idx, &req);
	return 0;
}

int config_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_response_t* response)
{
	printf("[VNF] config_response p5_idx:%d phy_id:%d\n", p5_idx, response->header.phy_id);
	return 0;
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

}

int send_vnf_start_req(vnf_test_config_t* config, vnf_test_config_phy_t* phy)
{
	nfapi_start_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_START_REQUEST;
	req.header.phy_id = phy->phy_id;

	nfapi_vnf_start_req(config->p5_vnf_config, phy->p5_idx, &req);
	return 0;
}

int start_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_response_t* response)
{
	printf("[VNF] start_response p5_idx:%d phy_id:%d\n", p5_idx, response->header.phy_id);
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);

	sleep(1);
	
	vnf_test_config_phy_t* phy = find_vnf_phy_config(&(vnf_test_config[0]), response->header.phy_id);

	char* addr = inet_ntoa(phy->p7_tx_sockaddr.sin_addr);
	int port = phy->p7_tx_sockaddr.sin_port;

	vnf_test_config_vnf_t* vnf = &(vnf_test_config[0].vnfs[phy->vnf_idx]);

	nfapi_vnf_p7_add_pnf(vnf->config, addr, port, response->header.phy_id);
	return 0;
}

int stop_response(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_response_t* response)
{
	printf("stop_response p5_idx:%d phy_id:%d\n", p5_idx, response->header.phy_id);
	CU_ASSERT_EQUAL(response->error_code, NFAPI_MSG_OK);
	return 0;
}

void  start_vnf_p5(vnf_test_config_t* test_config)
{
	test_config->p5_vnf_config = nfapi_vnf_config_create();

	test_config->p5_vnf_config->vnf_p5_port = test_config->p5_port;
	test_config->p5_vnf_config->vnf_ipv4 = 1;
	test_config->p5_vnf_config->pnf_connection_indication = &pnf_connection_indication;
	test_config->p5_vnf_config->pnf_disconnect_indication = &pnf_disconnect_indication;
	test_config->p5_vnf_config->pnf_param_resp = &pnf_param_response;
	test_config->p5_vnf_config->pnf_config_resp = &pnf_config_response;
	test_config->p5_vnf_config->pnf_start_resp = &pnf_start_response;
	test_config->p5_vnf_config->pnf_stop_resp = &pnf_stop_response;
	test_config->p5_vnf_config->param_resp = &param_response;
	test_config->p5_vnf_config->config_resp = &config_response;
	test_config->p5_vnf_config->start_resp = &start_response;
	test_config->p5_vnf_config->stop_resp = &stop_response;

	test_config->p5_vnf_config->user_data = test_config;

	pthread_create(&test_config->thread, NULL, &vnf_test_start_thread, test_config->p5_vnf_config);

	sleep(1);

}

void start_vnf_p7(vnf_test_config_vnf_t* vnf)
{
	
	// todo : select which vnf to use for these phy's
	vnf->enabled = 1;

	vnf->p7_rx_sockaddr.sin_addr.s_addr = inet_addr(vnf->vnf_p7_addr);
	vnf->p7_rx_sockaddr.sin_port = vnf->vnf_p7_port;


	vnf->config = nfapi_vnf_p7_config_create();
	vnf->config->checksum_enabled = 0;
	vnf->config->port = vnf->vnf_p7_port;
	vnf->config->subframe_indication = &test_subframe_indication;
	vnf->config->harq_indication = &test_harq_indication;
	vnf->config->crc_indication = &test_crc_indication;
	vnf->config->rx_indication = &test_rx_indication;
	vnf->config->rach_indication = &test_rach_indication;
	vnf->config->srs_indication = &test_srs_indication;
	vnf->config->sr_indication = &test_sr_indication;
	vnf->config->cqi_indication = &test_cqi_indication;
	
	vnf->config->nb_harq_indication = &test_nb_harq_indication;
	vnf->config->nrach_indication = &test_nrach_indication;
	

	vnf->config->segment_size = 1400;
	vnf->config->max_num_segments = 6;

	pthread_create(&(vnf->thread), NULL, &vnf_test_start_p7_thread, vnf->config);

	vnf->enabled = 1;

	//vnf->phy_count++;

	sleep(1);
}

void send_p7_segmented_msg(int sock, char* msg, int len, int segment_size, struct sockaddr* addr,  socklen_t addr_len)
{
	static uint8_t sequence_num = 0;
	if(len < segment_size)
	{
		msg[7] = sequence_num;
		sendto(sock, msg, len, 0, addr, addr_len);
	}
	else
	{
		int msg_body_len = len - 12 ; 
		int seg_body_len = segment_size - 12 ; 
		int segments = (msg_body_len / (seg_body_len)) + ((msg_body_len % seg_body_len) ? 1 : 0); 

		//printf("sending segmented message len:%d seg_size:%d count:%d\n", len, segment_size, segments);

		int segment = 0;
		int offset = 12;
		for(segment = 0; segment < segments; ++segment)
		{
			uint8_t last = 0;
			uint16_t size = segment_size - 12;
			if(segment + 1 == segments)
			{
				last = 1;
				size = (msg_body_len) - (seg_body_len * segment);
			}

			char buffer[segment_size];

			memcpy(&buffer[0], msg, 12);
			buffer[6] = ((!last) << 7) + segment;
			buffer[7] = sequence_num;

			// msg length
			uint8_t* p = (uint8_t*)&buffer[4];
			push16(size + 12, &p, (uint8_t*)&buffer[size + 12]);

			memcpy(&buffer[12], msg + offset, size);
			offset += size;
		
			sendto(sock, &buffer[0], size + 12, 0, addr, addr_len);

		}


	}

	sequence_num++;
}

void start_phy(vnf_test_config_phy_t* phy)
{

	nfapi_param_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PARAM_REQUEST;
	req.header.phy_id = phy->phy_id;

	nfapi_vnf_config_t* p5_vnf_config = (vnf_test_config[0].p5_vnf_config);
	

	nfapi_vnf_param_req(p5_vnf_config, phy->p5_idx, &req);

}


void vnf_test_start_connect(void)
{
	reset_test_configs();

	int segment_size = 256;

	vnf_test_config[0].p5_addr = "127.0.0.1";
	vnf_test_config[0].p5_port = 4242;

	vnf_test_config[0].vnfs[0].enabled = 0;
	vnf_test_config[0].vnfs[0].vnf_p7_addr = "127.0.0.1";
	vnf_test_config[0].vnfs[0].vnf_p7_port = 7878;
	vnf_test_config[0].vnfs[0].max_phys = 1;

	vnf_test_config[0].vnfs[1].enabled = 0;
	vnf_test_config[0].vnfs[1].vnf_p7_addr = "127.0.0.1";
	vnf_test_config[0].vnfs[1].vnf_p7_port = 7879;
	vnf_test_config[0].vnfs[1].max_phys = 1;


	// this is the action that p9 would take to set the vnf p5 address/port
	pnf_test_config[0].enabled = 1;
	pnf_test_config[0].vnf_p5_port = vnf_test_config[0].p5_port;
	pnf_test_config[0].vnf_p5_addr = vnf_test_config[0].p5_addr;
	pnf_test_config[0].p7_rx_port_base = 8000;

	pnf_test_config[1].enabled = 1;
	pnf_test_config[1].vnf_p5_port = vnf_test_config[0].p5_port;
	pnf_test_config[1].vnf_p5_addr = vnf_test_config[0].p5_addr;
	pnf_test_config[1].p7_rx_port_base = 8100;

	pnf_connection_indication_called = 0;

	start_vnf_p5(&vnf_test_config[0]);


	pnf_create_p5_sock(&pnf_test_config[0]);
	pnf_p5_connect(&pnf_test_config[0]);

	receive_pnf_param_request(&pnf_test_config[0]);
	send_pnf_param_response(&pnf_test_config[0]);

	receive_pnf_config_request(&pnf_test_config[0]);
	send_pnf_config_response(&pnf_test_config[0]);

	receive_pnf_start_request(&pnf_test_config[0]);
	send_pnf_start_response(&pnf_test_config[0]);

	pnf_create_p5_sock(&pnf_test_config[1]);

	pnf_p5_connect(&pnf_test_config[1]);

	receive_pnf_param_request(&pnf_test_config[1]);
	send_pnf_param_response(&pnf_test_config[1]);

	receive_pnf_config_request(&pnf_test_config[1]);
	send_pnf_config_response(&pnf_test_config[1]);

	receive_pnf_start_request(&pnf_test_config[1]);
	send_pnf_start_response(&pnf_test_config[1]);

	// start the vnf_p7 thread and assign by phys to that instance

	start_vnf_p7(&vnf_test_config[0].vnfs[0]);
	//start_vnf_p7(&vnf_test_config[0].vnfs[1]);
	
	printf("---- configuring phy %d -----\n", vnf_test_config[0].phys[0].phy_id);
	vnf_test_config[0].phys[0].vnf_idx = 0;
	start_phy(&vnf_test_config[0].phys[0]);

	int phy_id = receive_param_request(&pnf_test_config[0]);
	send_param_response(&pnf_test_config[0], phy_id); 

	phy_id = receive_config_request(&pnf_test_config[0]);
	send_config_response(&pnf_test_config[0], phy_id);

	printf("---- configuring phy %d -----\n", vnf_test_config[0].phys[2].phy_id);
	vnf_test_config[0].phys[2].vnf_idx = 0; 
	start_phy(&vnf_test_config[0].phys[2]);

	phy_id = receive_param_request(&pnf_test_config[1]);
	send_param_response(&pnf_test_config[1], phy_id); 

	phy_id = receive_config_request(&pnf_test_config[1]);
	send_config_response(&pnf_test_config[1], phy_id);

	printf("---- configuring phy %d -----\n", vnf_test_config[0].phys[1].phy_id);

	vnf_test_config[0].phys[1].vnf_idx = 0;
	start_phy(&vnf_test_config[0].phys[1]);

	phy_id = receive_param_request(&pnf_test_config[0]);
	send_param_response(&pnf_test_config[0], phy_id); 

	phy_id = receive_config_request(&pnf_test_config[0]);
	send_config_response(&pnf_test_config[0], phy_id);

	//-------------
	printf("---- starting phy %d -----\n", vnf_test_config[0].phys[0].phy_id);

	send_vnf_start_req(&vnf_test_config[0], &vnf_test_config[0].phys[0]);
	phy_id = receive_start_request(&pnf_test_config[0]);
	send_start_response(&pnf_test_config[0], phy_id);
/*
	printf("---- starting phy %d -----\n", vnf_test_config[0].phys[2].phy_id);

	send_vnf_start_req(&vnf_test_config[0], &vnf_test_config[0].phys[2]);
	phy_id = receive_start_request(&pnf_test_config[1]);
	send_start_response(&pnf_test_config[1], phy_id);

	printf("---- starting phy %d -----\n", vnf_test_config[0].phys[1].phy_id);

	send_vnf_start_req(&vnf_test_config[0], &vnf_test_config[0].phys[1]);
	phy_id = receive_start_request(&pnf_test_config[0]);
	send_start_response(&pnf_test_config[0], phy_id);
*/
	//close(pnf_test_config[0].p5_sock);

	char buffer[2048];
	int buffer_size = sizeof(buffer);

	
	fd_set rdfs;
	int exit = 0;
	while(exit != 1)
	{
		int pnf_idx, phy_idx; 
		int max_fd = 0;
		FD_ZERO(&rdfs);

		for(pnf_idx = 0; pnf_idx < 4; ++pnf_idx)
		{
			if(pnf_test_config[pnf_idx].enabled)
			{
				FD_SET(pnf_test_config[pnf_idx].p5_sock, &rdfs);
				if(pnf_test_config[pnf_idx].p5_sock > max_fd)
					max_fd = pnf_test_config[pnf_idx].p5_sock;
			}

			for(phy_idx = 0; phy_idx < 4; ++phy_idx)
			{
				pnf_test_config_phy_t* phy_info = &(pnf_test_config[pnf_idx].phys[phy_idx]);

				if(phy_info->started == 1 && phy_info->enabled == 1)
				{
					//////////////////printf("adding %d/%d %d %d\n", pnf_idx, phy_idx, phy_info->phy_id, phy_info->p7_rx_sock);
					FD_SET(phy_info->p7_rx_sock, &rdfs);
					if(phy_info->p7_rx_sock > max_fd)
						max_fd = phy_info->p7_rx_sock;
				}
			}
		}

		
		// changed to select,
		int select_result = select(max_fd + 1, &rdfs, NULL, NULL, NULL);
		(void)select_result;

		for(pnf_idx = 0; pnf_idx < 4; ++pnf_idx)
		{
			if(pnf_test_config[pnf_idx].enabled)
			{
				//pnf_test_config_t* pnf_config = &(pnf_test_config[pnf_idx]);
			}

			for(phy_idx = 0; phy_idx < 4; ++phy_idx)
			{
				pnf_test_config_phy_t* phy_info = &(pnf_test_config[pnf_idx].phys[phy_idx]);

				if(phy_info->enabled)
				{
					if(FD_ISSET(phy_info->p7_rx_sock, &rdfs))
					{
						//struct sockaddr_in recv_addr;
						//memset(&recv_addr, 0, sizeof(recv_addr));
						//socklen_t recv_addr_size;
						int len = recvfrom(phy_info->p7_rx_sock, &buffer[0], buffer_size, 0, 0, 0);// (struct sockaddr*)&recv_addr, &recv_addr_size);

						if(len == -1)
						{
							printf("recvfrom %d failed %d\n", phy_info->p7_rx_sock, errno);
							continue;
						}

						nfapi_p7_message_header_t header;
						nfapi_p7_message_header_unpack(buffer, len, &header, sizeof(header), 0);

						switch(header.message_id)
						{
							case NFAPI_DL_NODE_SYNC:
								{
									nfapi_dl_node_sync_t msg;
									nfapi_p7_message_unpack(buffer, len, &msg, sizeof(msg), 0);
									printf("[PNF:%d] NFAPI_DL_NODE_SYNC t1:%d\n", msg.header.phy_id, msg.t1);

									nfapi_ul_node_sync_t resp;
									memset(&resp, 0, sizeof(resp));
									resp.header.message_id = NFAPI_UL_NODE_SYNC;
									resp.header.phy_id = msg.header.phy_id;
									resp.t1 = msg.t1;
									usleep(50);
									resp.t2 = msg.t1 + 50; // 50 us rx latency
									resp.t3 = msg.t1 + 10; // 10 us pnf proc

									usleep(50);

									len = nfapi_p7_message_pack(&resp, buffer, buffer_size, 0);
									// send ul node sycn
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));
								}
								break;
							case NFAPI_DL_CONFIG_REQUEST:
								{
									nfapi_dl_config_request_t msg;
									nfapi_p7_message_unpack(buffer, len, &msg, sizeof(msg), 0);
									//printf("[PNF:%d] (%d/%d) NFAPI_DL_CONFIG_REQUEST\n", header.phy_id, SFNSF2SFN(msg.sfn_sf), SFNSF2SF(msg.sfn_sf));

									if(SFNSF2SFN(msg.sfn_sf) == 500)
										exit = 1;
									// simulate the uplink messages
									
									nfapi_harq_indication_t harq_ind;
									memset(&harq_ind, 0, sizeof(harq_ind));
									harq_ind.header.message_id = NFAPI_HARQ_INDICATION;
									harq_ind.header.phy_id = msg.header.phy_id;
									harq_ind.sfn_sf = msg.sfn_sf;
									harq_ind.harq_indication_body.tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
									harq_ind.harq_indication_body.number_of_harqs = 2;

									harq_ind.harq_indication_body.harq_pdu_list = (nfapi_harq_indication_pdu_t*)(malloc(sizeof(nfapi_harq_indication_pdu_t) * harq_ind.harq_indication_body.number_of_harqs));

									int i = 0;
									for(i = 0; i < harq_ind.harq_indication_body.number_of_harqs; ++i)
									{
										harq_ind.harq_indication_body.harq_pdu_list[i].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
									}
									len = nfapi_p7_message_pack(&harq_ind, buffer, buffer_size, 0);
									//sendto(phy_info->p7_tx_sock, &buffer[0], len, 0, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));
									


									//sendto(phy_info->p7_tx_sock, &buffer[0], len, 0, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									free(harq_ind.harq_indication_body.harq_pdu_list);

									nfapi_crc_indication_t crc_ind;
									memset(&crc_ind, 0, sizeof(crc_ind));
									crc_ind.header.message_id = NFAPI_CRC_INDICATION;
									crc_ind.header.phy_id = msg.header.phy_id;
									crc_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&crc_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									nfapi_sr_indication_t sr_ind;
									memset(&sr_ind, 0, sizeof(sr_ind));
									sr_ind.header.message_id = NFAPI_RX_SR_INDICATION;
									sr_ind.header.phy_id = msg.header.phy_id;
									sr_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&sr_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									nfapi_cqi_indication_t cqi_ind;
									memset(&cqi_ind, 0, sizeof(cqi_ind));
									cqi_ind.header.message_id = NFAPI_RX_CQI_INDICATION;
									cqi_ind.header.phy_id = msg.header.phy_id;
									cqi_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&cqi_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									nfapi_rach_indication_t rach_ind;
									memset(&rach_ind, 0, sizeof(rach_ind));
									rach_ind.header.message_id = NFAPI_RACH_INDICATION;
									rach_ind.header.phy_id = msg.header.phy_id;
									rach_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&rach_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									nfapi_srs_indication_t srs_ind;
									memset(&srs_ind, 0, sizeof(srs_ind));
									srs_ind.header.message_id = NFAPI_SRS_INDICATION;
									srs_ind.header.phy_id = msg.header.phy_id;
									srs_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&srs_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									nfapi_rx_indication_t rx_ind;
									memset(&rx_ind, 0, sizeof(rx_ind));
									rx_ind.header.message_id = NFAPI_RX_ULSCH_INDICATION;
									rx_ind.header.phy_id = msg.header.phy_id;
									rx_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&rx_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									if(msg.sfn_sf % 5 == 0)
									{
										// periodically send the timing info
										nfapi_timing_info_t timing_info;
										memset(&timing_info, 0, sizeof(timing_info));
										timing_info.header.message_id = NFAPI_TIMING_INFO;
										timing_info.header.phy_id = msg.header.phy_id;
										len = nfapi_p7_message_pack(&timing_info, buffer, buffer_size, 0);
										send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));
									}
									
									nfapi_nb_harq_indication_t nb_harq_ind;
									memset(&nb_harq_ind, 0, sizeof(nb_harq_ind));
									nb_harq_ind.header.message_id = NFAPI_NB_HARQ_INDICATION;
									nb_harq_ind.header.phy_id = msg.header.phy_id;
									nb_harq_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&nb_harq_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

									nfapi_nrach_indication_t nrach_ind;
									memset(&nrach_ind, 0, sizeof(nrach_ind));
									nrach_ind.header.message_id = NFAPI_NRACH_INDICATION;
									nrach_ind.header.phy_id = msg.header.phy_id;
									nrach_ind.sfn_sf = msg.sfn_sf;
									len = nfapi_p7_message_pack(&nrach_ind, buffer, buffer_size, 0);
									send_p7_segmented_msg(phy_info->p7_tx_sock, &buffer[0], len, segment_size, (struct sockaddr*)&phy_info->p7_tx_sockaddr, sizeof(phy_info->p7_tx_sockaddr));

								}
								break;
							case NFAPI_UL_CONFIG_REQUEST:
								{
									nfapi_ul_config_request_t msg;
									nfapi_p7_message_unpack(buffer, len, &msg, sizeof(msg), 0);
									//printf("[PNF:%d] (%d/%d) NFAPI_UL_CONFIG_REQUEST\n", header.phy_id, SFNSF2SFN(msg.sfn_sf), SFNSF2SF(msg.sfn_sf));
								}
								break;
							case NFAPI_HI_DCI0_REQUEST:
								{
									nfapi_hi_dci0_request_t msg;
									nfapi_p7_message_unpack(buffer, len, &msg, sizeof(msg), 0);
									//printf("[PNF:%d] (%d/%d) NFAPI_HI_DCI0_REQUEST\n", header.phy_id, SFNSF2SFN(msg.sfn_sf), SFNSF2SF(msg.sfn_sf));
								}
								break;
							case NFAPI_TX_REQUEST:
								{
									nfapi_tx_request_t msg;
									nfapi_p7_message_unpack(buffer, len, &msg, sizeof(msg), 0);
									//printf("[PNF:%d] (%d/%d) NFAPI_TX_REQUEST\n", header.phy_id, SFNSF2SFN(msg.sfn_sf), SFNSF2SF(msg.sfn_sf));
								}
								break;
						}
					
					}
				}
			}
		}
	}

	printf("Triggering p5 shutdown\n");

	// vnf p5 trigger shutdown
	nfapi_pnf_stop_request_t stop_req;
	memset(&stop_req, 0, sizeof(stop_req));
	stop_req.header.message_id = NFAPI_PNF_STOP_REQUEST;
	nfapi_vnf_pnf_stop_req(vnf_test_config[0].p5_vnf_config, 0, &stop_req);

	phy_id = receive_pnf_stop_request(&pnf_test_config[0]);
	send_pnf_stop_response(&pnf_test_config[0], phy_id);


	nfapi_vnf_stop(vnf_test_config[0].p5_vnf_config);


	
	int* result;
	pthread_join((vnf_test_config[0].thread), (void**)&result);
	CU_ASSERT_EQUAL(result, 0);
	//CU_ASSERT_EQUAL(pnf_connection_indication_called, 1);


	close(pnf_test_config[0].p5_sock);
}

void vnf_test_start_connect_ipv6(void)
{
	char* vnf_addr = "::1";
	int vnf_port = 4242;
	pnf_connection_indication_called = 0;

	nfapi_vnf_config_t* config = nfapi_vnf_config_create();
	config->vnf_p5_port = vnf_port;
	config->vnf_ipv4 = 0;
	config->vnf_ipv6 = 1;
	config->pnf_connection_indication = &pnf_connection_indication;
	config->pnf_disconnect_indication = &pnf_disconnect_indication;
	config->pnf_param_resp = &pnf_param_response;
	config->pnf_config_resp = &pnf_config_response;
	config->pnf_start_resp = &pnf_start_response;

	pthread_t thread;
	pthread_create(&thread, NULL, &vnf_test_start_thread, config);

	sleep(1);

	
	int p5Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
	struct sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(vnf_port);
	//addr.sin6_addr = inet_addr(vnf_addr);
	inet_pton(AF_INET6, vnf_addr, &addr.sin6_addr);


	/*
	int connect_result = connect(p5Sock, (struct sockaddr *)&addr, sizeof(addr) );

	printf("connect_result %d %d\n", connect_result, errno);

	receive_pnf_param_request(p5Sock);
	send_pnf_param_response(p5Sock, &addr, sizeof(addr));

	receive_pnf_config_request(p5Sock);
	send_pnf_config_response(p5Sock, &addr, sizeof(addr));

	receive_pnf_start_request(p5Sock);
	send_pnf_start_response(p5Sock, &addr, sizeof(addr));
	*/
	sleep(4);
	nfapi_vnf_stop(config);
	
	int* result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(pnf_connection_indication_called, 1);


	close(p5Sock);
}
void vnf_test_start_connect_2(void)
{
	char* vnf_addr = "127.0.0.1";
	int vnf_port = 4242;
	pnf_connection_indication_called = 0;

	nfapi_vnf_config_t * config = nfapi_vnf_config_create();
	config->vnf_p5_port = vnf_port;
	config->pnf_connection_indication = &pnf_connection_indication;

	pthread_t thread;
	pthread_create(&thread, NULL, &vnf_test_start_thread, config);

	sleep(1);

	
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(vnf_port);
	addr.sin_addr.s_addr = inet_addr(vnf_addr);

	
	int p5Sock1 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	int connect_result_1 = connect(p5Sock1, (struct sockaddr *)&addr, sizeof(addr) );

	printf("connect_result_1 %d %d\n", connect_result_1, errno);

	int p5Sock2 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	int connect_result_2 = connect(p5Sock2, (struct sockaddr *)&addr, sizeof(addr) );

	printf("connect_result_2 %d %d\n", connect_result_2, errno);

	sleep(1);

	close(p5Sock1);

	sleep(1);

	close(p5Sock2);

	sleep(1);

	nfapi_vnf_stop(config);
	
	int* result;
	pthread_join(thread, (void**)&result);
	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(pnf_connection_indication_called, 2);


	close(p5Sock1);
	close(p5Sock2);
}

void vnf_test_p7_segmentation_test1(void)
{

}


/************* Test Runner Code goes here **************/

int main ( void )
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if ( CUE_SUCCESS != CU_initialize_registry() )
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite( "vnf_test_suite", init_suite, clean_suite );
   if ( NULL == pSuite ) 
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

        //(NULL == CU_add_test(pSuite, "vnf_test_start_connect_2", vnf_test_start_connect_2)) 
   /* add the tests to the suite */
   if ( (NULL == CU_add_test(pSuite, "vnf_test_start_no_config", vnf_test_start_no_config)) ||
        (NULL == CU_add_test(pSuite, "vnf_test_start_connect", vnf_test_start_connect)) ||
        (NULL == CU_add_test(pSuite, "vnf_test_p7_segmentation_test1", vnf_test_p7_segmentation_test1))
      )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   // Run all tests using the basic interface
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_set_output_filename("vnf_unit_test_results.xml");
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

   CU_cleanup_registry();
   return CU_get_error();

}
