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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>
#include "debug.h"
#include <map>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "pool.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

extern "C" {
#include <nfapi_vnf_interface.h>
#include <nfapi.h>
#include "mac.h"

#include <vendor_ext.h>

//int vnf_main(int iPortP5, int, int);
int start_simulated_mac(int*, int*);
int read_xml(const char *xml_file);

}

static uint32_t rand_range(uint32_t min, uint32_t max)
{
	return ((rand() % (max + 1 - min)) + min);
}


void* vnf_allocate(size_t size)
{
	return (void*)memory_pool::allocate(size);
}

void vnf_deallocate(void* ptr)
{
	memory_pool::deallocate((uint8_t*)ptr);
}



class udp_data
{
	public:
		bool enabled;
		uint32_t rx_port;
		uint32_t tx_port;
		std::string tx_addr;
};


class phy_info
{
	public:

		uint16_t index;
		uint16_t id;

		std::vector<uint16_t> rfs;
		std::vector<uint16_t> excluded_rfs;

		int remote_port;
		std::string remote_addr;

		uint16_t earfcn;

};

class rf_info
{
	public:

		uint16_t index;
		uint16_t band;
};

class pnf_info
{
	public:

		std::vector<phy_info> phys;
		std::vector<rf_info> rfs;
};

class vnf_p7_info
{
	public:

		vnf_p7_info()
			: thread_started(false), 
			  config(nfapi_vnf_p7_config_create(), 
				     [] (nfapi_vnf_p7_config_t* f) { nfapi_vnf_p7_config_destory(f); }),
			  mac(0)
		{
			local_port = 0;
			
			timing_window = 0;
			periodic_timing_enabled = 0;
			aperiodic_timing_enabled = 0;
			periodic_timing_period = 0;
			
			//config = nfapi_vnf_p7_config_create();
		}
		
		vnf_p7_info(const vnf_p7_info& other)  = default;
		
		vnf_p7_info(vnf_p7_info&& other) = default;
		
		vnf_p7_info& operator=(const vnf_p7_info&) = default;
		
		vnf_p7_info& operator=(vnf_p7_info&&) = default;
		
		
		
		virtual	~vnf_p7_info()
		{
			//NFAPI_TRACE(NFAPI_TRACE_INFO, "*** vnf_p7_info delete ***\n");
			
			//nfapi_vnf_p7_config_destory(config);
			
			// should we delete the mac?
		}
		

		int local_port;
		std::string local_addr;

		unsigned timing_window;
		unsigned periodic_timing_enabled;
		unsigned aperiodic_timing_enabled;
		unsigned periodic_timing_period;

		// This is not really the right place if we have multiple PHY, 
		// should be part of the phy struct
		udp_data udp;

		bool thread_started;

		//nfapi_vnf_p7_config_t* config;
		std::shared_ptr<nfapi_vnf_p7_config_t> config;

		mac_t* mac;


};

class vnf_info
{
	public:
	
		uint8_t wireshark_test_mode;

		std::map<uint16_t, pnf_info> pnfs;

		std::vector<vnf_p7_info> p7_vnfs;
		
};


/// maybe these should be in the mac file...
int phy_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMessage, uint8_t *end, void** ve, nfapi_p7_codec_config_t* codec)
{
	(void)tl;
	(void)ppReadPackedMessage;
	(void)ve;
	return -1;
}

int phy_pack_vendor_extension_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t* codec)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "phy_pack_vendor_extension_tlv\n");

	nfapi_tl_t* tlv = (nfapi_tl_t*)ve;
	switch(tlv->tag)
	{
		case VENDOR_EXT_TLV_1_TAG:
			{
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "Packing VENDOR_EXT_TLV_1\n");
				vendor_ext_tlv_1* ve = (vendor_ext_tlv_1*)tlv;
				if(!push32(ve->dummy, ppWritePackedMsg, end))
					return 0;
				return 1;
			}
			break;
		default:
			return -1;
			break;
	}
}

int vnf_sim_pack_vendor_extension_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* codec)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "pnf_sim_pack_vendor_extension_tlv\n");
	nfapi_tl_t* tlv = (nfapi_tl_t*)ve;
	switch(tlv->tag)
	{
		case VENDOR_EXT_TLV_2_TAG:
			{
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "Packing VENDOR_EXT_TLV_2\n");
				vendor_ext_tlv_2* ve = (vendor_ext_tlv_2*)tlv;
				if(!push32(ve->dummy, ppWritePackedMsg, end))
					return 0;
				return 1;
			}
			break;
	}
	return -1;
}

int vnf_sim_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMessage, uint8_t *end, void** ve, nfapi_p4_p5_codec_config_t* codec)
{
	return -1;
}




int phy_unpack_p7_vendor_extension(nfapi_p7_message_header_t* header, uint8_t** ppReadPackedMessage, uint8_t *end, nfapi_p7_codec_config_t* config)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P7_VENDOR_EXT_IND)
	{
		vendor_ext_p7_ind* req = (vendor_ext_p7_ind*)(header);
		if(!pull16(ppReadPackedMessage, &req->error_code, end))
			return 0;
	}
	return 1;
}

int phy_pack_p7_vendor_extension(nfapi_p7_message_header_t* header, uint8_t** ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t* config)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P7_VENDOR_EXT_REQ)
	{
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
		vendor_ext_p7_req* req = (vendor_ext_p7_req*)(header);
		if(!(push16(req->dummy1, ppWritePackedMsg, end) &&
			 push16(req->dummy2, ppWritePackedMsg, end)))
			return 0;
	}
	return 1;
}

int vnf_sim_unpack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t* header, uint8_t** ppReadPackedMessage, uint8_t *end, nfapi_p4_p5_codec_config_t* codec)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P5_VENDOR_EXT_RSP)
	{
		vendor_ext_p5_rsp* req = (vendor_ext_p5_rsp*)(header);
		return(!pull16(ppReadPackedMessage, &req->error_code, end));
	}
	return 0;
}

int vnf_sim_pack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t* header, uint8_t** ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* codec)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P5_VENDOR_EXT_REQ)
	{
		vendor_ext_p5_req* req = (vendor_ext_p5_req*)(header);
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s %d %d\n", __FUNCTION__, req->dummy1, req->dummy2);
		return (!(push16(req->dummy1, ppWritePackedMsg, end) &&
				  push16(req->dummy2, ppWritePackedMsg, end)));
	}
	return 0;
}

void vnf_sim_trace(nfapi_trace_level_t level, const char* message, ...)
{
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
}

void mac_dl_config_req(mac_t* mac, nfapi_dl_config_request_t* req)
{
	vnf_p7_info* info = (vnf_p7_info*)(mac->user_data);

	nfapi_vnf_p7_dl_config_req(info->config.get(), req);
}

void mac_ul_config_req(mac_t* mac, nfapi_ul_config_request_t* req)
{
	vnf_p7_info* info = (vnf_p7_info*)(mac->user_data);

	nfapi_vnf_p7_ul_config_req(info->config.get(), req);
}

void mac_hi_dci0_req(mac_t* mac, nfapi_hi_dci0_request_t* req)
{
	vnf_p7_info* info = (vnf_p7_info*)(mac->user_data);

	nfapi_vnf_p7_hi_dci0_req(info->config.get(), req);
}

void mac_tx_req(mac_t* mac, nfapi_tx_request_t* req)
{
	vnf_p7_info* info = (vnf_p7_info*)(mac->user_data);

	nfapi_vnf_p7_tx_req(info->config.get(), req);
}

int phy_subframe_indication(struct nfapi_vnf_p7_config* config, uint16_t phy_id, uint16_t sfn_sf)
{
	//printf("[VNF_SIM] subframe indication %d\n", sfn_sf);
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_subframe_ind(p7_vnf->mac, phy_id, sfn_sf);
	return 0;
}
int phy_harq_indication(struct nfapi_vnf_p7_config* config, nfapi_harq_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_harq_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_crc_indication(struct nfapi_vnf_p7_config* config, nfapi_crc_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_crc_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_rx_indication(struct nfapi_vnf_p7_config* config, nfapi_rx_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_rx_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_rach_indication(struct nfapi_vnf_p7_config* config, nfapi_rach_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_rach_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_srs_indication(struct nfapi_vnf_p7_config* config, nfapi_srs_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_srs_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_sr_indication(struct nfapi_vnf_p7_config* config, nfapi_sr_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_sr_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_cqi_indication(struct nfapi_vnf_p7_config* config, nfapi_cqi_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_cqi_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_lbt_dl_indication(struct nfapi_vnf_p7_config* config, nfapi_lbt_dl_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_lbt_dl_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_nb_harq_indication(struct nfapi_vnf_p7_config* config, nfapi_nb_harq_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_nb_harq_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_nrach_indication(struct nfapi_vnf_p7_config* config, nfapi_nrach_indication_t* ind)
{
	vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
	mac_nrach_ind(p7_vnf->mac, ind);
	return 1;
}
int phy_vendor_ext(struct nfapi_vnf_p7_config* config, nfapi_p7_message_header_t* msg)
{
	if(msg->message_id == P7_VENDOR_EXT_IND)
	{
		//vendor_ext_p7_ind* ind = (vendor_ext_p7_ind*)msg;
		//printf("[VNF_SIM] vendor_ext (error_code:%d)\n", ind->error_code);
	}
	else
	{
		printf("[VNF_SIM] unknown %d\n", msg->message_id);
	}
	return 0;
}
nfapi_p7_message_header_t* phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t* msg_size)
{
	if(message_id == P7_VENDOR_EXT_IND)
	{
		*msg_size = sizeof(vendor_ext_p7_ind);
		return (nfapi_p7_message_header_t*)malloc(sizeof(vendor_ext_p7_ind));
	}
	return 0;
}
void phy_deallocate_p7_vendor_ext(nfapi_p7_message_header_t* header)
{
	free(header);
}

//static pthread_t vnf_start_pthread;
static pthread_t vnf_p7_start_pthread;
void* vnf_p7_start_thread(void *ptr)
{
  printf("%s()\n", __FUNCTION__);

  //std::shared_ptr<nfapi_vnf_p7_config> config = std::shared_ptr<nfapi_vnf_p7_config>(ptr);
  nfapi_vnf_p7_config_t *config = (nfapi_vnf_p7_config_t *)ptr;

  nfapi_vnf_p7_start(config);

  return 0;
}

void set_thread_priority(int priority)
{
	//printf("%s(priority:%d)\n", __FUNCTION__, priority);
	
	pthread_attr_t ptAttr;
	
	struct sched_param schedParam;
	schedParam.__sched_priority = priority; //79;
	if(sched_setscheduler(0, SCHED_RR, &schedParam) != 0)
	{
		printf("Failed to set scheduler to SCHED_RR\n");
	}

	if(pthread_attr_setschedpolicy(&ptAttr, SCHED_RR) != 0)
	{
		printf("Failed to set pthread sched policy SCHED_RR\n");
	}

	pthread_attr_setinheritsched(&ptAttr, PTHREAD_EXPLICIT_SCHED);

	struct sched_param thread_params;
	thread_params.sched_priority = 20;
	if(pthread_attr_setschedparam(&ptAttr, &thread_params) != 0)
	{
		printf("failed to set sched param\n");
	}
}

void* vnf_p7_thread_start(void* ptr)
{
  printf("%s()\n", __FUNCTION__);

	set_thread_priority(79);

	vnf_p7_info* p7_vnf = (vnf_p7_info*)ptr;

	p7_vnf->config->port = p7_vnf->local_port;
	p7_vnf->config->subframe_indication = &phy_subframe_indication;
	p7_vnf->config->harq_indication = &phy_harq_indication;
	p7_vnf->config->crc_indication = &phy_crc_indication;
	p7_vnf->config->rx_indication = &phy_rx_indication;
	p7_vnf->config->rach_indication = &phy_rach_indication;
	p7_vnf->config->srs_indication = &phy_srs_indication;
	p7_vnf->config->sr_indication = &phy_sr_indication;
	p7_vnf->config->cqi_indication = &phy_cqi_indication;
	p7_vnf->config->lbt_dl_indication = &phy_lbt_dl_indication;
	p7_vnf->config->nb_harq_indication = &phy_nb_harq_indication;
	p7_vnf->config->nrach_indication = &phy_nrach_indication;
	p7_vnf->config->malloc = &vnf_allocate;
	p7_vnf->config->free = &vnf_deallocate;

	p7_vnf->config->trace = &vnf_sim_trace;

	p7_vnf->config->vendor_ext = &phy_vendor_ext;
	p7_vnf->config->user_data = p7_vnf;

	p7_vnf->mac->user_data = p7_vnf;

	p7_vnf->config->codec_config.unpack_p7_vendor_extension = &phy_unpack_p7_vendor_extension;
	p7_vnf->config->codec_config.pack_p7_vendor_extension = &phy_pack_p7_vendor_extension;
	p7_vnf->config->codec_config.unpack_vendor_extension_tlv = &phy_unpack_vendor_extension_tlv;
	p7_vnf->config->codec_config.pack_vendor_extension_tlv = &phy_pack_vendor_extension_tlv;
	p7_vnf->config->codec_config.allocate = &vnf_allocate;
	p7_vnf->config->codec_config.deallocate = &vnf_deallocate;

	p7_vnf->config->allocate_p7_vendor_ext = &phy_allocate_p7_vendor_ext;
	p7_vnf->config->deallocate_p7_vendor_ext = &phy_deallocate_p7_vendor_ext;

        printf("[VNF] Creating VNF NFAPI start thread %s\n", __FUNCTION__);
        pthread_create(&vnf_p7_start_pthread, NULL, &vnf_p7_start_thread, p7_vnf->config.get());

	return 0;
}

int pnf_connection_indication_cb(nfapi_vnf_config_t* config, int p5_idx)
{
	printf("[VNF_SIM] pnf connection indication idx:%d\n", p5_idx);

	pnf_info pnf;
	vnf_info* vnf = (vnf_info*)(config->user_data);
	vnf->pnfs.insert(std::pair<uint16_t, pnf_info>(p5_idx, pnf));

	nfapi_pnf_param_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_PNF_PARAM_REQUEST;
	nfapi_vnf_pnf_param_req(config, p5_idx, &req);
	return 0;
}
int pnf_disconnection_indication_cb(nfapi_vnf_config_t* config, int p5_idx)
{
	printf("[VNF_SIM] pnf disconnection indication idx:%d\n", p5_idx);

	
	vnf_info* vnf = (vnf_info*)(config->user_data);
	auto find_result = vnf->pnfs.find(p5_idx);
	
	if(find_result != vnf->pnfs.end())
	{
		pnf_info& pnf = find_result->second;

		for(phy_info& phy : pnf.phys)
		{
			vnf_p7_info& p7_vnf = vnf->p7_vnfs[0];
			nfapi_vnf_p7_del_pnf((p7_vnf.config.get()), phy.id);
		}
	}

	return 0;
}

int pnf_param_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_response_t* resp)
{
	printf("[VNF_SIM] pnf param response idx:%d error:%d\n", p5_idx, resp->error_code);

	vnf_info* vnf = (vnf_info*)(config->user_data);

	auto find_result = vnf->pnfs.find(p5_idx);
	if(find_result != vnf->pnfs.end())
	{
		pnf_info& pnf = find_result->second;

		for(int i = 0; i < resp->pnf_phy.number_of_phys; ++i)
		{
			phy_info phy;
			phy.index = resp->pnf_phy.phy[i].phy_config_index;

			printf("[VNF_SIM] (PHY:%d) phy_config_idx:%d\n", i, resp->pnf_phy.phy[i].phy_config_index);

			nfapi_vnf_allocate_phy(config, p5_idx, &(phy.id));



			for(int j = 0; j < resp->pnf_phy.phy[i].number_of_rfs; ++j)
			{
				printf("[VNF_SIM] (PHY:%d) (RF%d) %d\n", i, j, resp->pnf_phy.phy[i].rf_config[j].rf_config_index);
				phy.rfs.push_back(resp->pnf_phy.phy[i].rf_config[j].rf_config_index);
			}


			pnf.phys.push_back(phy);
		}
		for(int i = 0; i < resp->pnf_rf.number_of_rfs; ++i)
		{
			rf_info rf;
			rf.index = resp->pnf_rf.rf[i].rf_config_index;

			printf("[VNF_SIM] (RF:%d) rf_config_idx:%d\n", i, resp->pnf_rf.rf[i].rf_config_index);

			pnf.rfs.push_back(rf);
		}

		nfapi_pnf_config_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_PNF_CONFIG_REQUEST;

		req.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
		req.pnf_phy_rf_config.number_phy_rf_config_info = pnf.phys.size();

		for(unsigned i = 0; i < pnf.phys.size(); ++i)
		{
			req.pnf_phy_rf_config.phy_rf_config[i].phy_id = pnf.phys[i].id;
			req.pnf_phy_rf_config.phy_rf_config[i].phy_config_index = pnf.phys[i].index;
			req.pnf_phy_rf_config.phy_rf_config[i].rf_config_index = pnf.phys[i].rfs[0];
		}


		nfapi_vnf_pnf_config_req(config, p5_idx, &req);
	}
	return 0;
}

int pnf_config_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_response_t* resp)
{
	printf("[VNF_SIM] pnf config response idx:%d\n", p5_idx);
	
	if(1)
	{
		vnf_info* vnf = (vnf_info*)(config->user_data);
		auto find_result = vnf->pnfs.find(p5_idx);
		if(find_result != vnf->pnfs.end())
		{
			//pnf_info& pnf = find_result->second;

			nfapi_pnf_start_request_t req;
			memset(&req, 0, sizeof(req));
			req.header.message_id = NFAPI_PNF_START_REQUEST;
			nfapi_vnf_pnf_start_req(config, p5_idx, &req);
		}
	}
	else
	{
		// Rather than send the pnf_start_request we will demonstrate
		// sending a vendor extention message. The start request will be
		// send when the vendor extension response is received 

		//vnf_info* vnf = (vnf_info*)(config->user_data);
		vendor_ext_p5_req req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = P5_VENDOR_EXT_REQ;
		req.dummy1 = 45;
		req.dummy2 = 1977;
		nfapi_vnf_vendor_extension(config, p5_idx, &req.header);
	}
	return 0;
}

int pnf_start_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_response_t* resp)
{
	printf("[VNF_SIM] pnf start response idx:%d\n", p5_idx);

	vnf_info* vnf = (vnf_info*)(config->user_data);
	vnf_p7_info& p7_vnf = vnf->p7_vnfs[0];

	if(p7_vnf.thread_started == false)
	{
		pthread_t vnf_p7_thread;
		pthread_create(&vnf_p7_thread, NULL, &vnf_p7_thread_start, &p7_vnf);
		p7_vnf.thread_started = true;
	}
	else
	{
		// P7 thread already running. 
	}

	// start all the phys in the pnf.
	
	auto find_result = vnf->pnfs.find(p5_idx);
	if(find_result != vnf->pnfs.end())
	{
		pnf_info& pnf = find_result->second;

		for(unsigned i = 0; i < pnf.phys.size(); ++i)
		{
			pnf_info& pnf = find_result->second;

			nfapi_param_request_t req;
			memset(&req, 0, sizeof(req));
			req.header.message_id = NFAPI_PARAM_REQUEST;
			req.header.phy_id = pnf.phys[i].id;
			nfapi_vnf_param_req(config, p5_idx, &req);
		}
	}

	return 0;
}
int param_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_response_t* resp)
{
	printf("[VNF_SIM] param response idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);

	vnf_info* vnf = (vnf_info*)(config->user_data);

	auto find_result = vnf->pnfs.find(p5_idx);
	if(find_result != vnf->pnfs.end())
	{
		pnf_info& pnf = find_result->second;
		
		
		auto found = std::find_if(pnf.phys.begin(), pnf.phys.end(), [&](phy_info& item)
								  { return item.id == resp->header.phy_id; });

		if(found != pnf.phys.end())
		{
			phy_info& phy = (*found);

			phy.remote_port = resp->nfapi_config.p7_pnf_port.value;
			
			struct sockaddr_in pnf_p7_sockaddr;
			memcpy(&pnf_p7_sockaddr.sin_addr.s_addr, &(resp->nfapi_config.p7_pnf_address_ipv4.address[0]), 4);

			phy.remote_addr = inet_ntoa(pnf_p7_sockaddr.sin_addr);

			// for now just 1
			vnf_p7_info& p7_vnf = vnf->p7_vnfs[0];

			printf("[VNF_SIM] %d.%d pnf p7 %s:%d timing %d %d %d %d\n", p5_idx, phy.id, phy.remote_addr.c_str(), phy.remote_port, p7_vnf.timing_window, p7_vnf.periodic_timing_period, p7_vnf.aperiodic_timing_enabled, p7_vnf.periodic_timing_period);


			nfapi_config_request_t req;
			memset(&req, 0, sizeof(req));
			req.header.message_id = NFAPI_CONFIG_REQUEST;
			req.header.phy_id = phy.id;

			
			req.nfapi_config.p7_vnf_port.tl.tag = NFAPI_NFAPI_P7_VNF_PORT_TAG;
			req.nfapi_config.p7_vnf_port.value = p7_vnf.local_port;
			req.num_tlv++;

			req.nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
			struct sockaddr_in vnf_p7_sockaddr;
			vnf_p7_sockaddr.sin_addr.s_addr = inet_addr(p7_vnf.local_addr.c_str());
			memcpy(&(req.nfapi_config.p7_vnf_address_ipv4.address[0]), &vnf_p7_sockaddr.sin_addr.s_addr, 4);
			req.num_tlv++;

			req.nfapi_config.timing_window.tl.tag = NFAPI_NFAPI_TIMING_WINDOW_TAG;
			req.nfapi_config.timing_window.value = p7_vnf.timing_window;
			req.num_tlv++;

			if(p7_vnf.periodic_timing_enabled || p7_vnf.aperiodic_timing_enabled)
			{
				req.nfapi_config.timing_info_mode.tl.tag = NFAPI_NFAPI_TIMING_INFO_MODE_TAG;
				req.nfapi_config.timing_info_mode.value = (p7_vnf.aperiodic_timing_enabled << 1) | (p7_vnf.periodic_timing_enabled);
				req.num_tlv++;

				if(p7_vnf.periodic_timing_enabled)
				{
					req.nfapi_config.timing_info_period.tl.tag = NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG;
					req.nfapi_config.timing_info_period.value = p7_vnf.periodic_timing_period;
					req.num_tlv++;
				}
			}

			req.nfapi_config.earfcn.tl.tag = NFAPI_NFAPI_EARFCN_TAG;
			req.nfapi_config.earfcn.value = phy.earfcn;
			req.num_tlv++;
			
			
			if(1)
			{
				// Poplate all tlv for wireshark testing
				req.subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
				req.num_tlv++;
				
				req.subframe_config.pcfich_power_offset.tl.tag = NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG;
				req.num_tlv++;
				req.subframe_config.pb.tl.tag = NFAPI_SUBFRAME_CONFIG_PB_TAG;
				req.num_tlv++;
				req.subframe_config.dl_cyclic_prefix_type.tl.tag = NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG;
				req.num_tlv++;
				req.subframe_config.ul_cyclic_prefix_type.tl.tag = NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG;
				req.num_tlv++;
	
				req.rf_config.dl_channel_bandwidth.tl.tag = NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG;
				req.num_tlv++;
				req.rf_config.ul_channel_bandwidth.tl.tag = NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG;
				req.num_tlv++;
				req.rf_config.reference_signal_power.tl.tag = NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG;
				req.num_tlv++;
				req.rf_config.tx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG;
				req.num_tlv++;
				req.rf_config.rx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG;		
				req.num_tlv++;
	
				req.phich_config.phich_resource.tl.tag = NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG;
				req.num_tlv++;
				req.phich_config.phich_duration.tl.tag = NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG;
				req.num_tlv++;
				req.phich_config.phich_power_offset.tl.tag = NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG;	
				req.num_tlv++;
	
				req.sch_config.primary_synchronization_signal_epre_eprers.tl.tag = NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG;
				req.num_tlv++;
				req.sch_config.secondary_synchronization_signal_epre_eprers.tl.tag = NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG;
				req.num_tlv++;
				req.sch_config.physical_cell_id.tl.tag = NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG;				
				req.num_tlv++;
	
				req.prach_config.configuration_index.tl.tag = NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG;
				req.num_tlv++;
				req.prach_config.root_sequence_index.tl.tag = NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG;
				req.num_tlv++;
				req.prach_config.zero_correlation_zone_configuration.tl.tag = NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG;
				req.num_tlv++;
				req.prach_config.high_speed_flag.tl.tag = NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG;
				req.num_tlv++;
				req.prach_config.frequency_offset.tl.tag = NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG;				
				req.num_tlv++;
	
				req.pusch_config.hopping_mode.tl.tag = NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG;
				req.num_tlv++;
				req.pusch_config.hopping_offset.tl.tag = NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG;
				req.num_tlv++;
				req.pusch_config.number_of_subbands.tl.tag = NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG;
				req.num_tlv++;
	
				req.pucch_config.delta_pucch_shift.tl.tag = NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG;
				req.num_tlv++;
				req.pucch_config.n_cqi_rb.tl.tag = NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG;
				req.num_tlv++;
				req.pucch_config.n_an_cs.tl.tag = NFAPI_PUCCH_CONFIG_N_AN_CS_TAG;
				req.num_tlv++;
				req.pucch_config.n1_pucch_an.tl.tag = NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG;
				req.num_tlv++;
	
				req.srs_config.bandwidth_configuration.tl.tag = NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG;
				req.num_tlv++;
				req.srs_config.max_up_pts.tl.tag = NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG;
				req.num_tlv++;
				req.srs_config.srs_subframe_configuration.tl.tag = NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG;
				req.num_tlv++;
				req.srs_config.srs_acknack_srs_simultaneous_transmission.tl.tag = NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG;				
				req.num_tlv++;
	
				req.uplink_reference_signal_config.uplink_rs_hopping.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG;
				req.num_tlv++;
				req.uplink_reference_signal_config.group_assignment.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG;
				req.num_tlv++;
				req.uplink_reference_signal_config.cyclic_shift_1_for_drms.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG;				
				req.num_tlv++;
	
				req.laa_config.ed_threshold_lbt_pdsch.tl.tag = NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_PDSCH_TAG;
				req.num_tlv++;
				req.laa_config.ed_threshold_lbt_drs.tl.tag = NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_DRS_TAG;
				req.num_tlv++;
				req.laa_config.pd_threshold.tl.tag = NFAPI_LAA_CONFIG_PD_THRESHOLD_TAG;
				req.num_tlv++;
				req.laa_config.multi_carrier_type.tl.tag = NFAPI_LAA_CONFIG_MULTI_CARRIER_TYPE_TAG;
				req.num_tlv++;
				req.laa_config.multi_carrier_tx.tl.tag = NFAPI_LAA_CONFIG_MULTI_CARRIER_TX_TAG;
				req.num_tlv++;
				req.laa_config.multi_carrier_freeze.tl.tag = NFAPI_LAA_CONFIG_MULTI_CARRIER_FREEZE_TAG;
				req.num_tlv++;
				req.laa_config.tx_antenna_ports_drs.tl.tag = NFAPI_LAA_CONFIG_TX_ANTENNA_PORTS_FOR_DRS_TAG;
				req.num_tlv++;
				req.laa_config.tx_power_drs.tl.tag = NFAPI_LAA_CONFIG_TRANSMISSION_POWER_FOR_DRS_TAG;				
				req.num_tlv++;
	
				req.emtc_config.pbch_repetitions_enable_r13.tl.tag = NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG;
				req.num_tlv++;
				req.emtc_config.prach_catm_root_sequence_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CATM_ROOT_SEQUENCE_INDEX_TAG;
				req.num_tlv++;
				req.emtc_config.prach_catm_zero_correlation_zone_configuration.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CATM_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG;
				req.num_tlv++;
				req.emtc_config.prach_catm_high_speed_flag.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CATM_HIGH_SPEED_FLAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_starting_subframe_periodicity.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_0_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_starting_subframe_periodicity.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_1_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_starting_subframe_periodicity.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_2_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_starting_subframe_periodicity.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG;
				req.num_tlv++;
				req.emtc_config.prach_ce_level_3_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG;
				req.num_tlv++;
				req.emtc_config.pucch_interval_ulhoppingconfigcommonmodea.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG;
				req.num_tlv++;
				req.emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG;			
				req.num_tlv++;
				
				req.nb_iot_config.operating_mode.tl.tag = NFAPI_NB_IOT_CONFIG_OPERATING_MODE_TAG;
				req.nb_iot_config.operating_mode.value = rand_range(0, 3);
				req.num_tlv++;				
				req.nb_iot_config.anchor.tl.tag = NFAPI_NB_IOT_CONFIG_ANCHOR_TAG;
				req.nb_iot_config.anchor.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.prb_index.tl.tag = NFAPI_NB_IOT_CONFIG_PRB_INDEX_TAG;
				req.nb_iot_config.prb_index.value = rand_range(0, 0x1F);
				req.num_tlv++;
				req.nb_iot_config.control_region_size.tl.tag = NFAPI_NB_IOT_CONFIG_CONTROL_REGION_SIZE_TAG;
				req.nb_iot_config.control_region_size.value = rand_range(1, 4);
				req.num_tlv++;
				req.nb_iot_config.assumed_crs_aps.tl.tag = NFAPI_NB_IOT_CONFIG_ASSUMED_CRS_APS_TAG;
				req.nb_iot_config.assumed_crs_aps.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_enabled.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_ENABLED_TAG;
				req.nb_iot_config.nprach_config_0_enabled.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_sf_periodicity.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_SF_PERIODICITY_TAG;
				req.nb_iot_config.nprach_config_0_sf_periodicity.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_start_time.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_START_TIME_TAG;
				req.nb_iot_config.nprach_config_0_start_time.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_subcarrier_offset.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_SUBCARRIER_OFFSET_TAG;
				req.nb_iot_config.nprach_config_0_subcarrier_offset.value = rand_range(0, 6);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_number_of_subcarriers.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_NUMBER_OF_SUBCARRIERS_TAG;
				req.nb_iot_config.nprach_config_0_number_of_subcarriers.value = rand_range(0, 3);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_cp_length.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_CP_LENGTH_TAG;
				req.nb_iot_config.nprach_config_0_cp_length.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_0_number_of_repetitions_per_attempt.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.nb_iot_config.nprach_config_0_number_of_repetitions_per_attempt.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_enabled.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_ENABLED_TAG;
				req.nb_iot_config.nprach_config_1_enabled.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_sf_periodicity.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_SF_PERIODICITY_TAG;
				req.nb_iot_config.nprach_config_1_sf_periodicity.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_start_time.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_START_TIME_TAG;
				req.nb_iot_config.nprach_config_1_start_time.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_subcarrier_offset.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_SUBCARRIER_OFFSET_TAG;
				req.nb_iot_config.nprach_config_1_subcarrier_offset.value = rand_range(0, 6);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_number_of_subcarriers.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_NUMBER_OF_SUBCARRIERS_TAG;
				req.nb_iot_config.nprach_config_1_number_of_subcarriers.value = rand_range(0, 3);				
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_cp_length.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_CP_LENGTH_TAG;
				req.nb_iot_config.nprach_config_1_cp_length.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_1_number_of_repetitions_per_attempt.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.nb_iot_config.nprach_config_1_number_of_repetitions_per_attempt.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_enabled.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_ENABLED_TAG;
				req.nb_iot_config.nprach_config_2_enabled.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_sf_periodicity.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_SF_PERIODICITY_TAG;
				req.nb_iot_config.nprach_config_2_sf_periodicity.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_start_time.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_START_TIME_TAG;
				req.nb_iot_config.nprach_config_2_start_time.value = rand_range(0, 7);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_subcarrier_offset.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_SUBCARRIER_OFFSET_TAG;
				req.nb_iot_config.nprach_config_2_subcarrier_offset.value = rand_range(0, 6);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_number_of_subcarriers.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_NUMBER_OF_SUBCARRIERS_TAG;
				req.nb_iot_config.nprach_config_2_number_of_subcarriers.value = rand_range(0, 3);				
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_cp_length.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_CP_LENGTH_TAG;
				req.nb_iot_config.nprach_config_2_cp_length.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.nprach_config_2_number_of_repetitions_per_attempt.tl.tag = NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
				req.nb_iot_config.nprach_config_2_number_of_repetitions_per_attempt.value = rand_range(0, 7);				
				req.num_tlv++;
				req.nb_iot_config.three_tone_base_sequence.tl.tag = NFAPI_NB_IOT_CONFIG_THREE_TONE_BASE_SEQUENCE_TAG;
				req.nb_iot_config.three_tone_base_sequence.value = rand_range(0, 0x0F);				
				req.num_tlv++;
				req.nb_iot_config.six_tone_base_sequence.tl.tag = NFAPI_NB_IOT_CONFIG_SIX_TONE_BASE_SEQUENCE_TAG;
				req.nb_iot_config.six_tone_base_sequence.value = rand_range(0, 0x03);				
				req.num_tlv++;
				req.nb_iot_config.twelve_tone_base_sequence.tl.tag = NFAPI_NB_IOT_CONFIG_TWELVE_TONE_BASE_SEQUENCE_TAG;
				req.nb_iot_config.twelve_tone_base_sequence.value = rand_range(0, 0x1F);				
				req.num_tlv++;
				req.nb_iot_config.three_tone_cyclic_shift.tl.tag = NFAPI_NB_IOT_CONFIG_THREE_TONE_CYCLIC_SHIFT_TAG;
				req.nb_iot_config.three_tone_cyclic_shift.value = rand_range(0, 5); // what is the max
				req.num_tlv++;
				req.nb_iot_config.six_tone_cyclic_shift.tl.tag = NFAPI_NB_IOT_CONFIG_SIX_TONE_CYCLIC_SHIFT_TAG;
				req.nb_iot_config.six_tone_cyclic_shift.value = rand_range(0, 5); // what is the max
				req.num_tlv++;
				req.nb_iot_config.dl_gap_config_enable.tl.tag = NFAPI_NB_IOT_CONFIG_DL_GAP_CONFIG_ENABLE_TAG;
				req.nb_iot_config.dl_gap_config_enable.value = rand_range(0, 1);
				req.num_tlv++;
				req.nb_iot_config.dl_gap_threshold.tl.tag = NFAPI_NB_IOT_CONFIG_DL_GAP_THRESHOLD_TAG;
				req.nb_iot_config.dl_gap_threshold.value = rand_range(0, 3);
				req.num_tlv++;
				req.nb_iot_config.dl_gap_periodicity.tl.tag = NFAPI_NB_IOT_CONFIG_DL_GAP_PERIODICITY_TAG;
				req.nb_iot_config.dl_gap_periodicity.value = rand_range(0, 3);
				req.num_tlv++;
				req.nb_iot_config.dl_gap_duration_coefficient.tl.tag = NFAPI_NB_IOT_CONFIG_DL_GAP_DURATION_COEFFICIENT_TAG;
				req.nb_iot_config.dl_gap_duration_coefficient.value = rand_range(0, 3);
				req.num_tlv++;
				
				req.tdd_frame_structure_config.subframe_assignment.tl.tag = NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG;
				req.num_tlv++;
				req.tdd_frame_structure_config.special_subframe_patterns.tl.tag = NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG;		
				req.num_tlv++;
				
				req.l23_config.data_report_mode.tl.tag = NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG;
				req.num_tlv++;
				req.l23_config.sfnsf.tl.tag = NFAPI_L23_CONFIG_SFNSF_TAG;				
				req.num_tlv++;
			}

			vendor_ext_tlv_2 ve2;
			memset(&ve2, 0, sizeof(ve2));
			ve2.tl.tag = VENDOR_EXT_TLV_2_TAG;
			ve2.dummy = 2016;
			req.vendor_extension = &ve2.tl;

			nfapi_vnf_config_req(config, p5_idx, &req);
		}
		else
		{
			printf("[VNF_SIM] param response failed to find pnf %d phy %d\n", p5_idx, resp->header.phy_id);
		}
	}
	else
	{
		printf("[VNF_SIM] param response failed to find pnf %d\n", p5_idx);
	}

	return 0;
}

int config_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_response_t* resp)
{
	printf("[VNF_SIM] config response idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);


	nfapi_start_request_t req;
	memset(&req, 0, sizeof(req));
	req.header.message_id = NFAPI_START_REQUEST;
	req.header.phy_id = resp->header.phy_id;
	nfapi_vnf_start_req(config, p5_idx, &req);

	return 0;
}

void test_p4_requests(nfapi_vnf_config_t* config, int p5_idx, int phy_id)
{
	{
		nfapi_measurement_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_MEASUREMENT_REQUEST;
		req.header.phy_id = phy_id;
		
		req.dl_rs_tx_power.tl.tag = NFAPI_MEASUREMENT_REQUEST_DL_RS_XTX_POWER_TAG;
		req.dl_rs_tx_power.value = 42;
		req.received_interference_power.tl.tag = NFAPI_MEASUREMENT_REQUEST_RECEIVED_INTERFERENCE_POWER_TAG;
		req.received_interference_power.value = 42;
		req.thermal_noise_power.tl.tag = NFAPI_MEASUREMENT_REQUEST_THERMAL_NOISE_POWER_TAG;
		req.thermal_noise_power.value = 42;
		
		nfapi_vnf_measurement_req(config, p5_idx, &req);
	}
	{
		nfapi_rssi_request_t lte_req;
		memset(&lte_req, 0, sizeof(lte_req));
		lte_req.header.message_id = NFAPI_RSSI_REQUEST;
		lte_req.header.phy_id = phy_id;
		
		lte_req.rat_type = NFAPI_RAT_TYPE_LTE;
		lte_req.lte_rssi_request.tl.tag = NFAPI_LTE_RSSI_REQUEST_TAG;
		lte_req.lte_rssi_request.frequency_band_indicator = 2;
		lte_req.lte_rssi_request.measurement_period = 1000;
		lte_req.lte_rssi_request.bandwidth = 50;
		lte_req.lte_rssi_request.timeout = 0;
		lte_req.lte_rssi_request.number_of_earfcns = 2;
		lte_req.lte_rssi_request.earfcn[0] = 389;
		lte_req.lte_rssi_request.earfcn[1] = 123;
		
		nfapi_vnf_rssi_request(config, p5_idx, &lte_req);
		
		nfapi_rssi_request_t utran_req;
		memset(&utran_req, 0, sizeof(utran_req));
		utran_req.header.message_id = NFAPI_RSSI_REQUEST;
		utran_req.header.phy_id = phy_id;
		
		utran_req.rat_type = NFAPI_RAT_TYPE_UTRAN;
		utran_req.utran_rssi_request.tl.tag = NFAPI_UTRAN_RSSI_REQUEST_TAG;
		utran_req.utran_rssi_request.frequency_band_indicator = 2;
		utran_req.utran_rssi_request.measurement_period = 1000;
		utran_req.utran_rssi_request.timeout = 0;
		utran_req.utran_rssi_request.number_of_uarfcns = 2;
		utran_req.utran_rssi_request.uarfcn[0] = 2348;
		utran_req.utran_rssi_request.uarfcn[1] = 52;
		
		nfapi_vnf_rssi_request(config, p5_idx, &utran_req);		
		
		
		nfapi_rssi_request_t geran_req;
		memset(&geran_req, 0, sizeof(geran_req));
		geran_req.header.message_id = NFAPI_RSSI_REQUEST;
		geran_req.header.phy_id = phy_id;
		
		geran_req.rat_type = NFAPI_RAT_TYPE_GERAN;
		geran_req.geran_rssi_request.tl.tag = NFAPI_GERAN_RSSI_REQUEST_TAG;
		geran_req.geran_rssi_request.frequency_band_indicator = 2;
		geran_req.geran_rssi_request.measurement_period = 1000;
		geran_req.geran_rssi_request.timeout = 0;
		geran_req.geran_rssi_request.number_of_arfcns = 1;
		geran_req.geran_rssi_request.arfcn[0].arfcn = 34;
		geran_req.geran_rssi_request.arfcn[0].direction = 0;
		
		nfapi_vnf_rssi_request(config, p5_idx, &geran_req);		
	}
	{
		nfapi_cell_search_request_t lte_req;
		memset(&lte_req, 0, sizeof(lte_req));
		lte_req.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
		lte_req.header.phy_id = phy_id;		
		
		lte_req.rat_type = NFAPI_RAT_TYPE_LTE;
		lte_req.lte_cell_search_request.tl.tag = NFAPI_LTE_CELL_SEARCH_REQUEST_TAG;
		lte_req.lte_cell_search_request.earfcn = 1234;
		lte_req.lte_cell_search_request.measurement_bandwidth = 50;
		lte_req.lte_cell_search_request.exhaustive_search = 1;
		lte_req.lte_cell_search_request.timeout = 1000;
		lte_req.lte_cell_search_request.number_of_pci = 1;
		lte_req.lte_cell_search_request.pci[0] = 234;
		
		nfapi_vnf_cell_search_request(config, p5_idx, &lte_req);
		
		nfapi_cell_search_request_t utran_req;
		memset(&utran_req, 0, sizeof(utran_req));
		utran_req.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
		utran_req.header.phy_id = phy_id;		
		
		utran_req.rat_type = NFAPI_RAT_TYPE_UTRAN;
		utran_req.utran_cell_search_request.tl.tag = NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG;
		utran_req.utran_cell_search_request.uarfcn = 1234;
		utran_req.utran_cell_search_request.exhaustive_search = 0;
		utran_req.utran_cell_search_request.timeout = 1000;
		utran_req.utran_cell_search_request.number_of_psc = 1;
		utran_req.utran_cell_search_request.psc[0] = 234;
		
		nfapi_vnf_cell_search_request(config, p5_idx, &utran_req);		
		
		nfapi_cell_search_request_t geran_req;
		memset(&geran_req, 0, sizeof(geran_req));
		geran_req.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
		geran_req.header.phy_id = phy_id;		
		
		geran_req.rat_type = NFAPI_RAT_TYPE_GERAN;
		geran_req.geran_cell_search_request.tl.tag = NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG;
		geran_req.geran_cell_search_request.timeout = 1000;
		geran_req.geran_cell_search_request.number_of_arfcn = 1;
		geran_req.geran_cell_search_request.arfcn[0] = 8765;
		
		nfapi_vnf_cell_search_request(config, p5_idx, &geran_req);				
	}
	{
		nfapi_broadcast_detect_request_t lte_req;
		memset(&lte_req, 0, sizeof(lte_req));
		lte_req.header.message_id = NFAPI_BROADCAST_DETECT_REQUEST;
		lte_req.header.phy_id = phy_id;		
		
		lte_req.rat_type = NFAPI_RAT_TYPE_LTE;
		lte_req.lte_broadcast_detect_request.tl.tag = NFAPI_LTE_BROADCAST_DETECT_REQUEST_TAG;
		lte_req.lte_broadcast_detect_request.earfcn = 1234;
		lte_req.lte_broadcast_detect_request.pci = 50;
		lte_req.lte_broadcast_detect_request.timeout = 1000;
		
		lte_req.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
		lte_req.pnf_cell_search_state.length = 3;
		
		nfapi_vnf_broadcast_detect_request(config, p5_idx, &lte_req);
		
		nfapi_broadcast_detect_request_t utran_req;
		memset(&utran_req, 0, sizeof(utran_req));
		utran_req.header.message_id = NFAPI_BROADCAST_DETECT_REQUEST;
		utran_req.header.phy_id = phy_id;		
		
		utran_req.rat_type = NFAPI_RAT_TYPE_LTE;
		utran_req.utran_broadcast_detect_request.tl.tag = NFAPI_UTRAN_BROADCAST_DETECT_REQUEST_TAG;
		utran_req.utran_broadcast_detect_request.uarfcn = 1234;
		utran_req.utran_broadcast_detect_request.psc = 50;
		utran_req.utran_broadcast_detect_request.timeout = 1000;
		
		utran_req.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
		utran_req.pnf_cell_search_state.length = 3;
		
		nfapi_vnf_broadcast_detect_request(config, p5_idx, &utran_req);		
	}
	{
		nfapi_system_information_schedule_request_t lte_req;
		memset(&lte_req, 0, sizeof(lte_req));
		lte_req.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST;
		lte_req.header.phy_id = phy_id;		
		
		lte_req.rat_type = NFAPI_RAT_TYPE_LTE;
		lte_req.lte_system_information_schedule_request.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG;
		lte_req.lte_system_information_schedule_request.earfcn = 1234;
		lte_req.lte_system_information_schedule_request.pci = 50;
		lte_req.lte_system_information_schedule_request.downlink_channel_bandwidth = 100;
		lte_req.lte_system_information_schedule_request.phich_configuration = 3;
		lte_req.lte_system_information_schedule_request.number_of_tx_antenna = 2;
		lte_req.lte_system_information_schedule_request.retry_count = 4;
		lte_req.lte_system_information_schedule_request.timeout = 1000;
		
		lte_req.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
		lte_req.pnf_cell_broadcast_state.length = 3;
		
		nfapi_vnf_system_information_schedule_request(config, p5_idx, &lte_req);
	}
	{
		nfapi_system_information_request_t lte_req;
		memset(&lte_req, 0, sizeof(lte_req));
		lte_req.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
		lte_req.header.phy_id = phy_id;		
		
		lte_req.rat_type = NFAPI_RAT_TYPE_LTE;		
		lte_req.lte_system_information_request.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG;
		lte_req.lte_system_information_request.earfcn = 1234;
		lte_req.lte_system_information_request.pci= 456;
		lte_req.lte_system_information_request.downlink_channel_bandwidth = 5;
		lte_req.lte_system_information_request.phich_configuration = 2;
		lte_req.lte_system_information_request.number_of_tx_antenna = 2;
		lte_req.lte_system_information_request.number_of_si_periodicity = 1;
		lte_req.lte_system_information_request.si_periodicity[0].si_periodicity = 3;
		lte_req.lte_system_information_request.si_periodicity[0].si_index = 3;
		lte_req.lte_system_information_request.si_window_length = 15;
		lte_req.lte_system_information_request.timeout = 1000;
		
		nfapi_vnf_system_information_request(config, p5_idx, &lte_req);
		
		nfapi_system_information_request_t utran_req;
		memset(&utran_req, 0, sizeof(utran_req));
		utran_req.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
		utran_req.header.phy_id = phy_id;		
		
		utran_req.rat_type = NFAPI_RAT_TYPE_UTRAN;
		utran_req.utran_system_information_request.tl.tag = NFAPI_UTRAN_SYSTEM_INFORMATION_REQUEST_TAG;
		utran_req.utran_system_information_request.uarfcn = 1234;
		utran_req.utran_system_information_request.psc = 456;
		utran_req.utran_system_information_request.timeout = 1000;
		
		nfapi_vnf_system_information_request(config, p5_idx, &utran_req);		
		
		nfapi_system_information_request_t geran_req;
		memset(&geran_req, 0, sizeof(geran_req));
		geran_req.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
		geran_req.header.phy_id = phy_id;		
		
		geran_req.rat_type = NFAPI_RAT_TYPE_GERAN;
		geran_req.geran_system_information_request.tl.tag = NFAPI_GERAN_SYSTEM_INFORMATION_REQUEST_TAG;
		geran_req.geran_system_information_request.arfcn = 1234;
		geran_req.geran_system_information_request.bsic = 21;
		geran_req.geran_system_information_request.timeout = 1000;
		
		nfapi_vnf_system_information_request(config, p5_idx, &geran_req);	
	}
	{
		nfapi_nmm_stop_request_t req;
		memset(&req, 0, sizeof(req));
		req.header.message_id = NFAPI_NMM_STOP_REQUEST;
		req.header.phy_id = phy_id;		
		nfapi_vnf_nmm_stop_request(config, p5_idx, &req);	
	}
}


int start_resp_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_response_t* resp)
{
	printf("[VNF_SIM] start response idx:%d phy_id:%d\n", p5_idx, resp->header.phy_id);

	vnf_info* vnf = (vnf_info*)(config->user_data);
	
	if(vnf->wireshark_test_mode)
		test_p4_requests(config, p5_idx,  resp->header.phy_id);

	auto find_result = vnf->pnfs.find(p5_idx);
	if(find_result != vnf->pnfs.end())
	{
		pnf_info& pnf = find_result->second;
		
		
		auto found = std::find_if(pnf.phys.begin(), pnf.phys.end(), [&](phy_info& item)
								  { return item.id == resp->header.phy_id; });

		if(found != pnf.phys.end())
		{
			phy_info& phy = (*found);

			vnf_p7_info& p7_vnf = vnf->p7_vnfs[0];

			nfapi_vnf_p7_add_pnf((p7_vnf.config.get()), phy.remote_addr.c_str(), phy.remote_port, phy.id);
			

		}


	}
	return 0;
}


nfapi_p4_p5_message_header_t* vnf_sim_allocate_p4_p5_vendor_ext(uint16_t message_id, uint16_t* msg_size)
{
	if(message_id == P5_VENDOR_EXT_RSP)
	{
		*msg_size = sizeof(vendor_ext_p5_rsp);
		return (nfapi_p4_p5_message_header_t*)malloc(sizeof(vendor_ext_p5_rsp));
	}
	return 0;
}
void vnf_sim_deallocate_p4_p5_vendor_ext(nfapi_p4_p5_message_header_t* header)
{
	free(header);
}
int vendor_ext_cb(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg)
{
	printf("[VNF_SIM] %s\n", __FUNCTION__);

	switch(msg->message_id)
	{
		case P5_VENDOR_EXT_RSP:
			{
				vendor_ext_p5_rsp* rsp = (vendor_ext_p5_rsp*)msg;
				printf("[VNF_SIM] P5_VENDOR_EXT_RSP error_code:%d\n", rsp->error_code);

				// send the start request
	
				nfapi_pnf_start_request_t req;
				memset(&req, 0, sizeof(req));
				req.header.message_id = NFAPI_PNF_START_REQUEST;
				nfapi_vnf_pnf_start_req(config, p5_idx, &req);
			}
			break;

	}

	return 0;
}

int read_vnf_xml(vnf_info& vnf, const char* xml_file)
{
	try
	{
		
		std::ifstream input(xml_file);
	
		using boost::property_tree::ptree;
		ptree pt;
	
		read_xml(input, pt);
		
		
	
		for(const auto& v : pt.get_child("vnf.vnf_p7_list"))
		{
			if(v.first == "vnf_p7")
			{
				vnf_p7_info vnf_p7;
				vnf_p7.local_port = v.second.get<unsigned>("port");
				vnf_p7.local_addr = v.second.get<std::string>("address");
	
				vnf_p7.timing_window = v.second.get<unsigned>("timing_window");
				vnf_p7.periodic_timing_enabled = v.second.get<unsigned>("periodic_timing_enabled");
				vnf_p7.aperiodic_timing_enabled = v.second.get<unsigned>("aperiodic_timing_enabled");
				vnf_p7.periodic_timing_period = v.second.get<unsigned>("periodic_timing_window");
	
				
				boost::optional<const boost::property_tree::ptree&> d = v.second.get_child_optional("data.udp");
				if(d.is_initialized())
				{
					vnf_p7.udp.enabled = true;
					vnf_p7.udp.rx_port = d.get().get<unsigned>("rx_port");
					vnf_p7.udp.tx_port = d.get().get<unsigned>("tx_port");
					vnf_p7.udp.tx_addr = d.get().get<std::string>("tx_addr");
				}
				else
				{
					vnf_p7.udp.enabled = false;
				}
				
				vnf.wireshark_test_mode = v.second.get<unsigned>("wireshark_test_mode", 0);
	
				vnf_p7.mac = mac_create(vnf.wireshark_test_mode);
				vnf_p7.mac->dl_config_req = &mac_dl_config_req;
				vnf_p7.mac->ul_config_req = &mac_ul_config_req;
				vnf_p7.mac->hi_dci0_req = &mac_hi_dci0_req;
				vnf_p7.mac->tx_req = &mac_tx_req;
	
				if(vnf_p7.udp.enabled)
				{
					mac_start_data(vnf_p7.mac, 
								   vnf_p7.udp.rx_port, 
								   vnf_p7.udp.tx_addr.c_str(), 
								   vnf_p7.udp.tx_port);
				}
	
				vnf.p7_vnfs.push_back(vnf_p7);
			}
		}
	}
	catch(std::exception& e)
	{
		printf("%s", e.what());
		return -1;
	}
	catch(boost::exception& e)
	{
		printf("%s", boost::diagnostic_information(e).c_str());
	}

	struct ifaddrs *ifaddr;
	getifaddrs(&ifaddr);


	while(ifaddr)
	{
		int family = ifaddr->ifa_addr->sa_family;
		if(family == AF_INET)
		{
			char host[128];
			getnameinfo(ifaddr->ifa_addr, sizeof(sockaddr_in), host, sizeof(host),	NULL, 0, 0);
			printf("%s\n", host);
		}
		ifaddr = ifaddr->ifa_next;
	}
	
	return 0;

}


int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Use parameters: <P5 Port> <xml config file>\n");
		return 0;
	}

	set_thread_priority(50);

	vnf_info vnf;

	if(read_vnf_xml(vnf, argv[2]) < 0)
	{
		printf("Failed to read xml file>\n");
		return 0;
	}

	nfapi_vnf_config_t* config = nfapi_vnf_config_create();

	config->vnf_ipv4 = 1;
	config->vnf_p5_port = atoi(argv[1]);
	config->pnf_connection_indication = &pnf_connection_indication_cb;
	config->pnf_disconnect_indication = &pnf_disconnection_indication_cb;
	config->pnf_param_resp = &pnf_param_resp_cb;
	config->pnf_config_resp = &pnf_config_resp_cb;
	config->pnf_start_resp = &pnf_start_resp_cb;
	config->param_resp = &param_resp_cb;
	config->config_resp = &config_resp_cb;
	config->start_resp = &start_resp_cb;
	config->vendor_ext = &vendor_ext_cb;
	
	
	
	config->trace = &vnf_sim_trace;
	
	config->malloc = &vnf_allocate;
	config->free = &vnf_deallocate;

	config->user_data = &vnf;

	config->codec_config.unpack_vendor_extension_tlv = &vnf_sim_unpack_vendor_extension_tlv;
	config->codec_config.pack_vendor_extension_tlv = &vnf_sim_pack_vendor_extension_tlv;

	config->codec_config.unpack_p4_p5_vendor_extension = &vnf_sim_unpack_p4_p5_vendor_extension;
	config->codec_config.pack_p4_p5_vendor_extension = &vnf_sim_pack_p4_p5_vendor_extension;
	config->allocate_p4_p5_vendor_ext = &vnf_sim_allocate_p4_p5_vendor_ext;
	config->deallocate_p4_p5_vendor_ext = &vnf_sim_deallocate_p4_p5_vendor_ext;
	config->codec_config.allocate = &vnf_allocate;
	config->codec_config.deallocate = &vnf_deallocate;


	printf("Calling nfapi_vnf_start\n");
	return nfapi_vnf_start(config);
}
