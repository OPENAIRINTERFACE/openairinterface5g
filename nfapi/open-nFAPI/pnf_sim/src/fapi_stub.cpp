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

#include "fapi_stub.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include <mutex>
#include <queue>
#include <list>

struct phy_pdu
{
	phy_pdu() : buffer_len(1500), buffer(0), len(0)
	{
		buffer = (char*) malloc(buffer_len);
	}
	
	virtual ~phy_pdu()
	{
		free(buffer);
	}


	unsigned buffer_len;
	char* buffer;
	unsigned len;
};

class fapi_private 
{
		std::mutex mutex;
		std::queue<phy_pdu*> rx_buffer;

		std::queue<phy_pdu*> free_store;
	public:

		fapi_private()
			: byte_count(0), tick(0), first_dl_config(false)
		{
		}

		phy_pdu* allocate_phy_pdu()
		{
			phy_pdu* pdu = 0;
			mutex.lock();
			if(free_store.empty())
			{
				pdu = new phy_pdu();
			}
			else
			{
				pdu = free_store.front();
				free_store.pop();
			}
			mutex.unlock();
			return pdu;
		}

		void release_phy_pdu(phy_pdu* pdu)
		{
			mutex.lock();
			free_store.push(pdu);
			mutex.unlock();
		}

		bool rx_buffer_empty()
		{
			bool empty;
			mutex.lock();
			empty = rx_buffer.empty();
			mutex.unlock();

			return empty;
		}
		

		void push_rx_buffer(phy_pdu* buff)
		{
			mutex.lock();
			rx_buffer.push(buff);
			mutex.unlock();
		}

		phy_pdu* pop_rx_buffer()
		{
			phy_pdu* buff = 0;
			mutex.lock();
			if(!rx_buffer.empty())
			{
				buff = rx_buffer.front();
				rx_buffer.pop();
			}
			mutex.unlock();
			return buff;
		}

		uint32_t byte_count;
		uint32_t tick;
		bool first_dl_config;

};

extern "C"
{
	typedef struct fapi_internal
	{
		fapi_t _public;

		fapi_cb_t callbacks;

		uint8_t state;
		fapi_config_t config;

		int rx_sock;
		int tx_sock;
		struct sockaddr_in tx_addr;

		uint32_t tx_byte_count;
		uint32_t tick;
		
		fapi_private* fapi;

	} fapi_internal_t;
}

extern void set_thread_priority(int);
/*
{
	pthread_attr_t ptAttr;
	
	struct sched_param schedParam;
	schedParam.__sched_priority = 79;
	sched_setscheduler(0, SCHED_RR, &schedParam);

	pthread_attr_setschedpolicy(&ptAttr, SCHED_RR);

	pthread_attr_setinheritsched(&ptAttr, PTHREAD_EXPLICIT_SCHED);

	struct sched_param thread_params;
	thread_params.sched_priority = 20;
	pthread_attr_setschedparam(&ptAttr, &thread_params);
}
*/

void send_uplink_indications(fapi_internal_t* instance, uint16_t sfn_sf)
{
	fapi_harq_ind_t harq_ind;
	(instance->callbacks.fapi_harq_ind)(&(instance->_public), &harq_ind);

	fapi_crc_ind_t crc_ind;
	crc_ind.header.message_id = FAPI_CRC_INDICATION;
	crc_ind.header.length = 0; //??;
	crc_ind.sfn_sf = sfn_sf;
	crc_ind.body.number_of_crcs = 1;
	crc_ind.body.pdus[0].rx_ue_info.handle = 0; //??
	crc_ind.body.pdus[0].rx_ue_info.rnti = 0; //??
	crc_ind.body.pdus[0].rel8_pdu.crc_flag = 1;

	(instance->callbacks.fapi_crc_ind)(&(instance->_public), &crc_ind);

	if(!instance->fapi->rx_buffer_empty())
	{
		fapi_rx_ulsch_ind_t rx_ind;
		memset(&rx_ind, 0, sizeof(rx_ind));
		rx_ind.header.message_id = FAPI_RX_ULSCH_INDICATION;
		rx_ind.sfn_sf = sfn_sf;


		phy_pdu* buff = 0;
		int i = 0;
		std::list<phy_pdu*> free_list;
		do
		{
			buff = instance->fapi->pop_rx_buffer();
			if(buff != 0)
			{
				if(buff->len == 0)
				{
					printf("[FAPI] Buffer length = 0\n");
				}

				rx_ind.body.pdus[i].rx_ue_info.handle = 0xDEADBEEF;
				rx_ind.body.pdus[i].rx_ue_info.rnti = 0x4242;

				rx_ind.body.pdus[i].rel8_pdu.length = buff->len;
				//rx_ind.pdus[i].rel8_pdu.data_offset;
				//rx_ind.pdus[i].rel8_pdu.ul_cqi;
				//rx_ind.pdus[i].rel8_pdu.timing_advance;

				rx_ind.body.data[i] = buff->buffer;

				rx_ind.body.number_of_pdus++;
				i++;

				instance->fapi->byte_count += buff->len;

				free_list.push_back(buff);
			}
		}while(buff != 0 && i < 8);

		(instance->callbacks.fapi_rx_ulsch_ind)(&(instance->_public), &rx_ind);

		for(phy_pdu* pdu : free_list)
		{
			instance->fapi->release_phy_pdu(pdu);
			//free(tx_req.tx_request_body.tx_pdu_list[j].segments[0].segment_data);
		}
	}
	else
	{
		fapi_rx_ulsch_ind_t rx_ind;
		memset(&rx_ind, 0, sizeof(rx_ind));
		rx_ind.header.message_id = FAPI_RX_ULSCH_INDICATION;
		rx_ind.sfn_sf = sfn_sf;
		(instance->callbacks.fapi_rx_ulsch_ind)(&(instance->_public), &rx_ind);
	}

	
	fapi_rx_cqi_ind_t cqi_ind;
	cqi_ind.sfn_sf = sfn_sf;
	(instance->callbacks.fapi_rx_cqi_ind)(&(instance->_public), &cqi_ind);

	fapi_rx_sr_ind_t sr_ind;
	sr_ind.sfn_sf = sfn_sf;
	(instance->callbacks.fapi_rx_sr_ind)(&(instance->_public), &sr_ind);

	fapi_rach_ind_t rach_ind;
	rach_ind.sfn_sf = sfn_sf;
	(instance->callbacks.fapi_rach_ind)(&(instance->_public), &rach_ind);

	fapi_srs_ind_t srs_ind;
	srs_ind.sfn_sf = sfn_sf;
	(instance->callbacks.fapi_srs_ind)(&(instance->_public), &srs_ind);
	/*
	nfapi_lbt_dl_indication_t lbt_ind;
	memset(&lbt_ind, 0, sizeof(lbt_ind));
	lbt_ind.header.message_id = NFAPI_LBT_DL_INDICATION;
	lbt_ind.header.phy_id = config->phy_id;
	lbt_ind.sfn_sf = sfn_sf;
	nfapi_pnf_p7_lbt_dl_ind(config, &lbt_ind);

	vendor_ext_p7_ind ve_p7_ind;
	memset(&ve_p7_ind, 0, sizeof(ve_p7_ind));
	ve_p7_ind.header.message_id = P7_VENDOR_EXT_IND;
	ve_p7_ind.header.phy_id = config->phy_id;
	ve_p7_ind.error_code = NFAPI_MSG_OK;
	nfapi_pnf_p7_vendor_extension(config, &(ve_p7_ind.header));
	*/
	
	fapi_nb_harq_ind_t nb_harq_ind;
	nb_harq_ind.sfn_sf = sfn_sf;
	(instance->callbacks.fapi_nb_harq_ind)(&(instance->_public), &nb_harq_ind);

	fapi_nrach_ind_t nrach_ind;
	nrach_ind.sfn_sf = sfn_sf;
	(instance->callbacks.fapi_nrach_ind)(&(instance->_public), &nrach_ind);

}

void* fapi_thread_start(void* ptr)
{
	set_thread_priority(81);

	fapi_internal_t* instance = (fapi_internal_t*)ptr;
	uint16_t sfn_sf_dec = 0;
	uint32_t last_tv_usec = 0;
	uint32_t last_tv_sec = 0;

	uint32_t millisec;
	uint32_t last_millisec = -1;
	uint16_t catchup = 0;

	while(1)
	{
		// get the time
		struct timeval sf_start;
		(void)gettimeofday(&sf_start, NULL);
		
		uint16_t sfn_sf = ((((sfn_sf_dec) / 10) << 4) | (((sfn_sf_dec) - (((sfn_sf_dec) / 10) * 10)) & 0xF));
		// increment the sfn/sf - for the next subframe
		sfn_sf_dec++;
		if(sfn_sf_dec > 10239)
			sfn_sf_dec = 0;

	
		fapi_subframe_ind_t ind;
		ind.sfn_sf = sfn_sf;

		if(instance->fapi->first_dl_config)
			send_uplink_indications(instance, sfn_sf);

		if(instance->tick == 1000)
		{
			if(instance->tx_byte_count > 0)
			{
				printf("[FAPI] Tx rate %d bytes/sec\n", instance->tx_byte_count);
				instance->tx_byte_count = 0;
			}
			
			instance->tick = 0;
		}

		instance->tick++;

		(instance->callbacks.fapi_subframe_ind)(&(instance->_public), &ind);

		{
			phy_pdu* pdu = instance->fapi->allocate_phy_pdu();
			int len = recvfrom(instance->rx_sock, pdu->buffer, pdu->buffer_len, MSG_DONTWAIT, 0, 0);
			if(len > 0)
			{
				pdu->len = len;
				instance->fapi->push_rx_buffer(pdu);
			}
			else
			{
				instance->fapi->release_phy_pdu(pdu);
			}
		}
		

		if(catchup)
		{
			catchup--;
		}
		else
		{
			struct timespec now_ts;
			struct timespec sleep_ts;
			struct timespec sleep_rem_ts;

			// get the current time
			clock_gettime(CLOCK_MONOTONIC, &now_ts);


			// determine how long to sleep before the start of the next 1ms
			sleep_ts.tv_sec = 0;
			sleep_ts.tv_nsec = 1e6 - (now_ts.tv_nsec % 1000000);

			int nanosleep_result = nanosleep(&sleep_ts, &sleep_rem_ts);

			if(nanosleep_result != 0)
				printf("*** nanosleep failed or was interrupted\n");


			clock_gettime(CLOCK_MONOTONIC, &now_ts);
			millisec = now_ts.tv_nsec / 1e6;

			if(last_millisec != -1 && ((last_millisec + 1 ) % 1000) != millisec)
			{
				printf("*** missing millisec %d %d\n", last_millisec, millisec);
				catchup = millisec - last_millisec - 1;
			}

			last_millisec = millisec;
		}
	}
}

extern "C"
{
	fapi_t* fapi_create(fapi_cb_t* callbacks, fapi_config_t* config)
	{
		fapi_internal_t* instance = (fapi_internal*)calloc(1, sizeof(fapi_internal_t));
		instance->callbacks = *callbacks;
		instance->config = *config;
		instance->state = 0;

		instance->fapi = new fapi_private();

		return (fapi_t*)instance;
	}
	
	void fapi_destroy(fapi_t* fapi)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
		
		delete instance->fapi;
		free(instance);
	}

	void* fapi_rx_thread_start(void* ptr)
	{
		set_thread_priority(60);

		fapi_internal_t* instance = (fapi_internal_t*)ptr;

		while(1)
		{
			phy_pdu* pdu = instance->fapi->allocate_phy_pdu();
			int len = recvfrom(instance->rx_sock, pdu->buffer, pdu->buffer_len, 0, 0, 0);
			if(len > 0)
			{
				pdu->len = len;
				instance->fapi->push_rx_buffer(pdu);
			}
			else
			{
				instance->fapi->release_phy_pdu(pdu);
			}

		}
	}

	void fapi_start_data(fapi_t* fapi, unsigned rx_port, const char* tx_address, unsigned tx_port)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;

		printf("[FAPI] Rx Data from %d\n", rx_port);
		printf("[FAPI] Tx Data to %s:%d\n", tx_address, tx_port);

		instance->rx_sock = socket(AF_INET, SOCK_DGRAM, 0);
		
		if(instance->rx_sock < 0)
		{
			printf("[FAPI] Failed to create socket\n");
			return;
		}

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(rx_port);
		addr.sin_addr.s_addr = INADDR_ANY;
		
		int bind_result = bind(instance->rx_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		
		if(bind_result == -1)
		{
			printf("[FAPI] Failed to bind to port %d\n", rx_port);
			close(instance->rx_sock);
			return ;
		}

		instance->tx_sock = socket(AF_INET, SOCK_DGRAM, 0);
		instance->tx_addr.sin_family = AF_INET;
		instance->tx_addr.sin_port = htons(tx_port);
		instance->tx_addr.sin_addr.s_addr = inet_addr(tx_address);

	}


	void fill_tlv(fapi_tlv_t tlvs[], uint8_t count, uint8_t tag, uint8_t len, uint16_t value)
	{
		tlvs[count].tag = tag;
		tlvs[count].value = value;
		tlvs[count].length = len;
		
	}

	int fapi_param_request(fapi_t* fapi, fapi_param_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
			
		fapi_param_resp_t resp;
		resp.header.message_id = FAPI_PARAM_RESPONSE;

		resp.error_code = FAPI_MSG_OK;

		resp.number_of_tlvs = 0;
		fill_tlv(resp.tlvs, resp.number_of_tlvs++, FAPI_PHY_STATE_TAG, 2, instance->state);


		if(instance->state == 0)
		{
			if(instance->config.duplex_mode == 0)
			{
				// -- TDD
				// Downlink Bandwidth Support
				// Uplink Bandwidth Support
				// Downlink Modulation Support
				// Uplink Modulation Support
				// PHY Antenna Capability
				// Release Capability
				// MBSFN Capability
			}
			else if(instance->config.duplex_mode == 1)
			{
				// -- FDD
				// Downlink Bandwidth Support
				fill_tlv(resp.tlvs, resp.number_of_tlvs++, FAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG, 2, instance->config.dl_channel_bw_support);
				// Uplink Bandwidth Support
				fill_tlv(resp.tlvs, resp.number_of_tlvs++, FAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG, 2, instance->config.ul_channel_bw_support);
				// Downlink Modulation Support
				// Uplink Modulation Support
				// PHY Antenna Capability
				// Release Capability
				// MBSFN Capability
				// LAA Capability
			}
		}
		else
		{
			if(instance->config.duplex_mode == 0)
			{
				// -- TDD
				// Downlink Bandwidth Support
				// Uplink Bandwidth Support
				// Downlink Modulation Support
				// Uplink Modulation Support
				// PHY Antenna Capability
				// Release Capability
				// MBSFN Capability
				// Duplexing Mode
				// PCFICH Power Offset
				// P-B
				// DL Cyclic Prefix Type
				// UL Cyclic Prefix Type
				// RF Config
				// PHICH Config
				// SCH Config
				// PRACH Config
				// PUSCH Config
				// PUCCH Config
				// SRS Config
				// Uplink Reference Signal Config
				// TDD Frame Structure Config
				// Data Report Mode
			}
			else if(instance->config.duplex_mode == 1)
			{
				// FDD
				// Downlink Bandwidth Support
				// Uplink Bandwidth Support
				// Downlink Modulation Support
				// Uplink Modulation Support
				// PHY Antenna Capability
				// Release Capability
				// MBSFN Capability
				// LAA Capability
				// Duplexing Mode
				// PCFICH Power Offset
				// P-B
				// DL Cyclic Prefix Type
				// UL Cyclic Prefix Type
				// RF Config
				// PHICH Config
				// SCH Config
				// PRACH Config
				// PUSCH Config
				// PUCCH Config
				// SRS Config
				// Uplink Reference Signal Config
				// Data Report Mode
			}
		}


		//todo fill
		(instance->callbacks.fapi_param_response)(fapi, &resp);

		return 0;
	}

	int fapi_config_request(fapi_t* fapi, fapi_config_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
			
		fapi_config_resp_t resp;
		resp.header.message_id = FAPI_CONFIG_RESPONSE;
		resp.error_code = FAPI_MSG_OK;

		(instance->callbacks.fapi_config_response)(fapi, &resp);
		return 0;
	}

	int fapi_start_request(fapi_t* fapi, fapi_start_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
			
		pthread_t fapi_thread;
		pthread_create(&fapi_thread, NULL, &fapi_thread_start, instance);
		
		return 0;
	}

	int fapi_dl_config_request(fapi_t* fapi, fapi_dl_config_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
		instance->fapi->first_dl_config = true;
		return 0;
	}
	int fapi_ul_config_request(fapi_t* fapi, fapi_ul_config_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
		return 0;
	}
	int fapi_hi_dci0_request(fapi_t* fapi, fapi_hi_dci0_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;
		return 0;
	}
	int fapi_tx_request(fapi_t* fapi, fapi_tx_req_t* req)
	{
		fapi_internal_t* instance = (fapi_internal_t*)fapi;

		for(int i = 0; i < req->body.number_of_pdus; ++i)
		{
			uint16_t len = req->body.pdus[i].pdu_length;
			uint32_t* data = req->body.pdus[i].tlvs[0].value;
			//printf("[FAPI] sfnsf:%d len:%d\n", req->sfn_sf,len);
			//
			instance->tx_byte_count += len;

			int sendto_result = sendto(instance->tx_sock, data, len, 0, (struct sockaddr*)&(instance->tx_addr), sizeof(instance->tx_addr));
			
			if(sendto_result == -1)
			{
				// error
			}
		}

		return 0;
	}
}

