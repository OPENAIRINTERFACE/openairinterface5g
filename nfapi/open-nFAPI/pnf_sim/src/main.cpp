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
#include <unistd.h>
#include "debug.h"
#include "nfapi_pnf_interface.h"
#include "nfapi.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>

#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 


#include <vendor_ext.h>

#include "fapi_stub.h"
#include "pool.h"


#include <mutex>
#include <list>
#include <queue>
#include <map>
#include <vector>
#include <algorithm>
#include <stdlib.h>

#define NUM_P5_PHY 2

uint16_t phy_antenna_capability_values[] = { 1, 2, 4, 8, 16 };

static uint32_t rand_range(uint32_t min, uint32_t max)
{
	return ((rand() % (max + 1 - min)) + min);
}



extern "C" nfapi_pnf_param_response_t g_pnf_param_resp;

extern "C" {


void* pnf_allocate(size_t size)
{
	return (void*)memory_pool::allocate(size);
}

void pnf_deallocate(void* ptr)
{
	memory_pool::deallocate((uint8_t*)ptr);
}


int read_xml(const char *xml_file);

};

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

		phy_info()
			: first_subframe_ind(0), fapi(0),
			  dl_ues_per_subframe(0), ul_ues_per_subframe(0), 
			  timing_window(0), timing_info_mode(0), timing_info_period(0)
		{
			index = 0;
			id = 0;

			local_port = 0;
			remote_addr = 0;
			remote_port = 0;
	
			duplex_mode = 0;
			dl_channel_bw_support = 0;
			ul_channel_bw_support = 0;
			num_dl_layers_supported = 0;
			num_ul_layers_supported = 0;
			release_supported = 0;
			nmm_modes_supported = 0;
		}

		uint16_t index;
		uint16_t id;
		std::vector<uint8_t> rfs;
		std::vector<uint8_t> excluded_rfs;

		udp_data udp;

		std::string local_addr;
		int local_port;

		char* remote_addr;
		int remote_port;

		uint8_t duplex_mode;
		uint16_t dl_channel_bw_support;
		uint16_t ul_channel_bw_support;
		uint8_t num_dl_layers_supported;
		uint8_t num_ul_layers_supported;
		uint16_t release_supported;
		uint8_t nmm_modes_supported;

		uint8_t dl_ues_per_subframe;
		uint8_t ul_ues_per_subframe;

		uint8_t first_subframe_ind;

		// timing information recevied from the vnf
		uint8_t timing_window;
		uint8_t timing_info_mode;
		uint8_t timing_info_period;

		fapi_t* fapi;

};

class rf_info
{
	public:
		uint16_t index;
		uint16_t band;
		int16_t max_transmit_power;
		int16_t min_transmit_power;
		uint8_t num_antennas_supported;
		uint32_t min_downlink_frequency;
		uint32_t max_downlink_frequency;
		uint32_t max_uplink_frequency;
		uint32_t min_uplink_frequency;
};


class pnf_info
{
	public:

		pnf_info() 
		: release(13), wireshark_test_mode(0),
		  max_total_power(0), oui(0)
					
		{
			release = 0;
	
			sync_mode = 0;
			location_mode = 0;
			dl_config_timing = 0;
			ul_config_timing = 0;
			tx_timing = 0;
			hi_dci0_timing = 0;
	
			max_phys = 0;
			max_total_bw = 0;
			max_total_dl_layers = 0;
			max_total_ul_layers = 0;
			shared_bands = 0;
			shared_pa = 0;
			
		}

		int release;
		std::vector<phy_info> phys;
		std::vector<rf_info> rfs;

		uint8_t sync_mode;
		uint8_t location_mode;
		uint8_t location_coordinates[6];
		uint32_t dl_config_timing;
		uint32_t ul_config_timing;
		uint32_t tx_timing;
		uint32_t hi_dci0_timing;

		uint16_t max_phys;
		uint16_t max_total_bw;
		uint16_t max_total_dl_layers;
		uint16_t max_total_ul_layers;
		uint8_t shared_bands;
		uint8_t shared_pa;
		int16_t max_total_power;
		uint8_t oui;
		
		uint8_t wireshark_test_mode;

};

struct pnf_phy_user_data_t
{
	uint16_t phy_id;
	nfapi_pnf_config_t* config;
	phy_info* phy;
	nfapi_pnf_p7_config_t* p7_config;
};

int read_pnf_xml(pnf_info& pnf, const char* xml_file)
{
	try
	{
		std::ifstream input(xml_file);
	
		using boost::property_tree::ptree;
		ptree pt;
	
		read_xml(input, pt);
		
		pnf.wireshark_test_mode = pt.get<unsigned>("pnf.wireshark_test_mode", 0);
	
		
		pnf.sync_mode = pt.get<unsigned>("pnf.sync_mode");
		pnf.location_mode= pt.get<unsigned>("pnf.location_mode");
		//pnf.sync_mode = pt.get<unsigned>("pnf.location_coordinates");
	
		pnf.dl_config_timing= pt.get<unsigned>("pnf.dl_config_timing");
		pnf.ul_config_timing = pt.get<unsigned>("pnf.ul_config_timing");
		pnf.tx_timing = pt.get<unsigned>("pnf.tx_timing");
		pnf.hi_dci0_timing = pt.get<unsigned>("pnf.hi_dci0_timing");
	
		pnf.max_phys = pt.get<unsigned>("pnf.max_phys");
		pnf.max_total_bw = pt.get<unsigned>("pnf.max_total_bandwidth");
		pnf.max_total_dl_layers = pt.get<unsigned>("pnf.max_total_num_dl_layers");
		pnf.max_total_ul_layers = pt.get<unsigned>("pnf.max_total_num_ul_layers");
	
		pnf.shared_bands = pt.get<unsigned>("pnf.shared_bands");
		pnf.shared_pa = pt.get<unsigned>("pnf.shared_pas");
	
		pnf.max_total_power = pt.get<signed>("pnf.maximum_total_power");
	
		//"oui");
	
		for(const auto& v : pt.get_child("pnf.phys"))
		{
			if(v.first == "phy")
			{
				phy_info phy;
				
				
					
				phy.index = v.second.get<unsigned>("index");
				phy.local_port = v.second.get<unsigned>("port");
				phy.local_addr = v.second.get<std::string>("address");
				phy.duplex_mode = v.second.get<unsigned>("duplex_mode");
	
				phy.dl_channel_bw_support = v.second.get<unsigned>("downlink_channel_bandwidth_support");
				phy.ul_channel_bw_support = v.second.get<unsigned>("uplink_channel_bandwidth_support");
				phy.num_dl_layers_supported = v.second.get<unsigned>("number_of_dl_layers");
				phy.num_ul_layers_supported = v.second.get<unsigned>("number_of_ul_layers");
				phy.release_supported = v.second.get<unsigned>("3gpp_release_supported");
				phy.nmm_modes_supported = v.second.get<unsigned>("nmm_modes_supported");
	
				for(const auto& v2 : v.second.get_child("rfs"))
				{
					if(v2.first == "index")
						phy.rfs.push_back(v2.second.get_value<unsigned>());
				}
				for(const auto& v2 : v.second.get_child("excluded_rfs"))
				{
					if(v2.first == "index")
						phy.excluded_rfs.push_back(v2.second.get_value<unsigned>());
				}
	
				boost::optional<const boost::property_tree::ptree&> d = v.second.get_child_optional("data.udp");
				if(d.is_initialized())
				{
					phy.udp.enabled = true;
					phy.udp.rx_port = d.get().get<unsigned>("rx_port");
					phy.udp.tx_port = d.get().get<unsigned>("tx_port");
					phy.udp.tx_addr = d.get().get<std::string>("tx_addr");
				}
				else
				{
					phy.udp.enabled = false;
				}
	
				phy.dl_ues_per_subframe = v.second.get<unsigned>("dl_ues_per_subframe");
				phy.ul_ues_per_subframe = v.second.get<unsigned>("ul_ues_per_subframe");
	
				pnf.phys.push_back(phy);
			}
		}	
		for(const auto& v : pt.get_child("pnf.rfs"))
		{
			if(v.first == "rf")
			{
				rf_info rf;
	
				rf.index = v.second.get<unsigned>("index");
				rf.band = v.second.get<unsigned>("band");
				rf.max_transmit_power = v.second.get<signed>("max_transmit_power");
				rf.min_transmit_power = v.second.get<signed>("min_transmit_power");
				rf.num_antennas_supported = v.second.get<unsigned>("num_antennas_supported");
				rf.min_downlink_frequency = v.second.get<unsigned>("min_downlink_frequency");
				rf.max_downlink_frequency = v.second.get<unsigned>("max_downlink_frequency");
				rf.min_uplink_frequency = v.second.get<unsigned>("max_uplink_frequency");
				rf.max_uplink_frequency = v.second.get<unsigned>("min_uplink_frequency");
	
				pnf.rfs.push_back(rf);
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
		return -1;
	}
	
	return 0;
}



void pnf_sim_trace(nfapi_trace_level_t level, const char* message, ...)
{
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
}

void set_thread_priority(int priority)
{
	//printf("%s(priority:%d)\n", __FUNCTION__, priority);

	pthread_attr_t ptAttr;
	
	struct sched_param schedParam;
	schedParam.__sched_priority = priority; //79;
	if(sched_setscheduler(0, SCHED_RR, &schedParam) != 0)
	{
		printf("failed to set SCHED_RR\n");
	}

	if(pthread_attr_setschedpolicy(&ptAttr, SCHED_RR) != 0)
	{
		printf("failed to set pthread SCHED_RR %d\n", errno);
	}

	pthread_attr_setinheritsched(&ptAttr, PTHREAD_EXPLICIT_SCHED);

	struct sched_param thread_params;
	thread_params.sched_priority = 20;
	if(pthread_attr_setschedparam(&ptAttr, &thread_params) != 0)
	{
		printf("failed to set sched param\n");
	}
}


void* pnf_p7_thread_start(void* ptr)
{
	set_thread_priority(79);

	nfapi_pnf_p7_config_t* config = (nfapi_pnf_p7_config_t*)ptr;
	nfapi_pnf_p7_start(config);
	
	return 0;
}



int pnf_param_request(nfapi_pnf_config_t* config, nfapi_pnf_param_request_t* req)
{
	printf("[PNF_SIM] pnf param request\n");

	nfapi_pnf_param_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;

	pnf_info* pnf = (pnf_info*)(config->user_data);
	
	resp.pnf_param_general.tl.tag = NFAPI_PNF_PARAM_GENERAL_TAG;
	resp.pnf_param_general.nfapi_sync_mode = pnf->sync_mode;
	resp.pnf_param_general.location_mode = pnf->location_mode;
	//uint8_t location_coordinates[NFAPI_PNF_PARAM_GENERAL_LOCATION_LENGTH];
	resp.pnf_param_general.dl_config_timing = pnf->dl_config_timing;
	resp.pnf_param_general.tx_timing = pnf->tx_timing;
	resp.pnf_param_general.ul_config_timing = pnf->ul_config_timing;
	resp.pnf_param_general.hi_dci0_timing = pnf->hi_dci0_timing;
	resp.pnf_param_general.maximum_number_phys = pnf->max_phys;
	resp.pnf_param_general.maximum_total_bandwidth = pnf->max_total_bw;
	resp.pnf_param_general.maximum_total_number_dl_layers = pnf->max_total_dl_layers;
	resp.pnf_param_general.maximum_total_number_ul_layers = pnf->max_total_ul_layers;
	resp.pnf_param_general.shared_bands = pnf->shared_bands;
	resp.pnf_param_general.shared_pa = pnf->shared_pa;
	resp.pnf_param_general.maximum_total_power = pnf->max_total_power;
	//uint8_t oui[NFAPI_PNF_PARAM_GENERAL_OUI_LENGTH];

	resp.pnf_phy.tl.tag = NFAPI_PNF_PHY_TAG;
	resp.pnf_phy.number_of_phys = pnf->phys.size();
	
	for(int i = 0; i < pnf->phys.size(); ++i)
	{
		resp.pnf_phy.phy[i].phy_config_index = pnf->phys[i].index; 
		resp.pnf_phy.phy[i].downlink_channel_bandwidth_supported = pnf->phys[i].dl_channel_bw_support;
		resp.pnf_phy.phy[i].uplink_channel_bandwidth_supported = pnf->phys[i].ul_channel_bw_support;
		resp.pnf_phy.phy[i].number_of_dl_layers_supported = pnf->phys[i].num_dl_layers_supported;
		resp.pnf_phy.phy[i].number_of_ul_layers_supported = pnf->phys[i].num_ul_layers_supported;
		resp.pnf_phy.phy[i].maximum_3gpp_release_supported = pnf->phys[i].release_supported;
		resp.pnf_phy.phy[i].nmm_modes_supported = pnf->phys[i].nmm_modes_supported;

		resp.pnf_phy.phy[i].number_of_rfs = pnf->phys[i].rfs.size();
		for(int j = 0; j < pnf->phys[i].rfs.size(); ++j)
		{
			resp.pnf_phy.phy[i].rf_config[j].rf_config_index = pnf->phys[i].rfs[j];
		}

		resp.pnf_phy.phy[i].number_of_rf_exclusions = pnf->phys[i].excluded_rfs.size();
		for(int j = 0; j < pnf->phys[i].excluded_rfs.size(); ++j)
		{
			resp.pnf_phy.phy[i].excluded_rf_config[j].rf_config_index = pnf->phys[i].excluded_rfs[j];
		}
	}


	resp.pnf_rf.tl.tag = NFAPI_PNF_RF_TAG;
	resp.pnf_rf.number_of_rfs = pnf->rfs.size();

	for(int i = 0; i < pnf->rfs.size(); ++i)
	{
		resp.pnf_rf.rf[i].rf_config_index = pnf->rfs[i].index; 
		resp.pnf_rf.rf[i].band = pnf->rfs[i].band;
		resp.pnf_rf.rf[i].maximum_transmit_power = pnf->rfs[i].max_transmit_power; 
		resp.pnf_rf.rf[i].minimum_transmit_power = pnf->rfs[i].min_transmit_power;
		resp.pnf_rf.rf[i].number_of_antennas_suppported = pnf->rfs[i].num_antennas_supported;
		resp.pnf_rf.rf[i].minimum_downlink_frequency = pnf->rfs[i].min_downlink_frequency;
		resp.pnf_rf.rf[i].maximum_downlink_frequency = pnf->rfs[i].max_downlink_frequency;
		resp.pnf_rf.rf[i].minimum_uplink_frequency = pnf->rfs[i].min_uplink_frequency;
		resp.pnf_rf.rf[i].maximum_uplink_frequency = pnf->rfs[i].max_uplink_frequency;
	}

	if(pnf->release >= 10)
	{
		resp.pnf_phy_rel10.tl.tag = NFAPI_PNF_PHY_REL10_TAG;
		resp.pnf_phy_rel10.number_of_phys = pnf->phys.size();
		
		for(int i = 0; i < pnf->phys.size(); ++i)
		{
			resp.pnf_phy_rel10.phy[i].phy_config_index = pnf->phys[i].index; 
			resp.pnf_phy_rel10.phy[i].transmission_mode_7_supported = 0;
			resp.pnf_phy_rel10.phy[i].transmission_mode_8_supported = 1;
			resp.pnf_phy_rel10.phy[i].two_antenna_ports_for_pucch = 0;
			resp.pnf_phy_rel10.phy[i].transmission_mode_9_supported = 1;
			resp.pnf_phy_rel10.phy[i].simultaneous_pucch_pusch = 0;
			resp.pnf_phy_rel10.phy[i].four_layer_tx_with_tm3_and_tm4 = 1;

		}
	}

	if(pnf->release >= 11)
	{
		resp.pnf_phy_rel11.tl.tag = NFAPI_PNF_PHY_REL11_TAG;
		resp.pnf_phy_rel11.number_of_phys = pnf->phys.size();
		
		for(int i = 0; i < pnf->phys.size(); ++i)
		{
			resp.pnf_phy_rel11.phy[i].phy_config_index = pnf->phys[i].index; 
			resp.pnf_phy_rel11.phy[i].edpcch_supported = 0;
			resp.pnf_phy_rel11.phy[i].multi_ack_csi_reporting = 1;
			resp.pnf_phy_rel11.phy[i].pucch_tx_diversity = 0;
			resp.pnf_phy_rel11.phy[i].ul_comp_supported = 1;
			resp.pnf_phy_rel11.phy[i].transmission_mode_5_supported = 0;
		}
	}

	if(pnf->release >= 12)
	{
		resp.pnf_phy_rel12.tl.tag = NFAPI_PNF_PHY_REL12_TAG;
		resp.pnf_phy_rel12.number_of_phys = pnf->phys.size();
		
		for(int i = 0; i < pnf->phys.size(); ++i)
		{
			resp.pnf_phy_rel12.phy[i].phy_config_index = pnf->phys[i].index; 
			resp.pnf_phy_rel12.phy[i].csi_subframe_set = 0;
			resp.pnf_phy_rel12.phy[i].enhanced_4tx_codebook = 2; // yes this is invalid
			resp.pnf_phy_rel12.phy[i].drs_supported = 0;
			resp.pnf_phy_rel12.phy[i].ul_64qam_supported = 1;
			resp.pnf_phy_rel12.phy[i].transmission_mode_10_supported = 0;
			resp.pnf_phy_rel12.phy[i].alternative_bts_indices = 1;
		}
	}

	if(pnf->release >= 13)
	{
		resp.pnf_phy_rel13.tl.tag = NFAPI_PNF_PHY_REL13_TAG;
		resp.pnf_phy_rel13.number_of_phys = pnf->phys.size();
		
		for(int i = 0; i < pnf->phys.size(); ++i)
		{
			resp.pnf_phy_rel13.phy[i].phy_config_index = pnf->phys[i].index; 
			resp.pnf_phy_rel13.phy[i].pucch_format4_supported = 0;
			resp.pnf_phy_rel13.phy[i].pucch_format5_supported = 1;
			resp.pnf_phy_rel13.phy[i].more_than_5_ca_support = 0;
			resp.pnf_phy_rel13.phy[i].laa_supported = 1;
			resp.pnf_phy_rel13.phy[i].laa_ending_in_dwpts_supported = 0;
			resp.pnf_phy_rel13.phy[i].laa_starting_in_second_slot_supported = 1;
			resp.pnf_phy_rel13.phy[i].beamforming_supported = 0;
			resp.pnf_phy_rel13.phy[i].csi_rs_enhancement_supported = 1;
			resp.pnf_phy_rel13.phy[i].drms_enhancement_supported = 0;
			resp.pnf_phy_rel13.phy[i].srs_enhancement_supported = 1;
		}
		
		resp.pnf_phy_rel13_nb_iot.tl.tag = NFAPI_PNF_PHY_REL13_NB_IOT_TAG;
		resp.pnf_phy_rel13_nb_iot.number_of_phys = pnf->phys.size();		
		
		for(int i = 0; i < pnf->phys.size(); ++i)
		{
			resp.pnf_phy_rel13_nb_iot.phy[i].phy_config_index = pnf->phys[i].index; 
			
			resp.pnf_phy_rel13_nb_iot.phy[i].number_of_rfs = pnf->phys[i].rfs.size();
			for(int j = 0; j < pnf->phys[i].rfs.size(); ++j)
			{
				resp.pnf_phy_rel13_nb_iot.phy[i].rf_config[j].rf_config_index = pnf->phys[i].rfs[j];
			}
	
			resp.pnf_phy_rel13_nb_iot.phy[i].number_of_rf_exclusions = pnf->phys[i].excluded_rfs.size();
			for(int j = 0; j < pnf->phys[i].excluded_rfs.size(); ++j)
			{
				resp.pnf_phy_rel13_nb_iot.phy[i].excluded_rf_config[j].rf_config_index = pnf->phys[i].excluded_rfs[j];
			}
			
			resp.pnf_phy_rel13_nb_iot.phy[i].number_of_dl_layers_supported = pnf->phys[i].num_dl_layers_supported;
			resp.pnf_phy_rel13_nb_iot.phy[i].number_of_ul_layers_supported = pnf->phys[i].num_ul_layers_supported;
			resp.pnf_phy_rel13_nb_iot.phy[i].maximum_3gpp_release_supported = pnf->phys[i].release_supported;
			resp.pnf_phy_rel13_nb_iot.phy[i].nmm_modes_supported = pnf->phys[i].nmm_modes_supported;

		}
	}


	nfapi_pnf_pnf_param_resp(config, &resp);
	
	return 0;
}

int pnf_config_request(nfapi_pnf_config_t* config, nfapi_pnf_config_request_t* req)
{
	printf("[PNF_SIM] pnf config request\n");

	pnf_info* pnf = (pnf_info*)(config->user_data);

	for(int i = 0; i < req->pnf_phy_rf_config.number_phy_rf_config_info; ++i)
	{
		auto found = std::find_if(pnf->phys.begin(), pnf->phys.end(), [&](phy_info& item)
								  { return item.index == req->pnf_phy_rf_config.phy_rf_config[i].phy_config_index; });

		if(found != pnf->phys.end())
		{
			phy_info& phy = (*found);
			phy.id = req->pnf_phy_rf_config.phy_rf_config[i].phy_id;
			printf("[PNF_SIM] pnf config request assigned phy_id %d to phy_config_index %d\n", phy.id, req->pnf_phy_rf_config.phy_rf_config[i].phy_config_index);
		}
		else
		{
			// did not find the phy
			printf("[PNF_SIM] pnf config request did not find phy_config_index %d\n", req->pnf_phy_rf_config.phy_rf_config[i].phy_config_index);
		}

	}

	nfapi_pnf_config_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_pnf_config_resp(config, &resp);

	return 0;
}

int fapi_param_response(fapi_t* fapi, fapi_param_resp_t* resp)
{
	printf("[PNF_SIM] fapi param response\n");
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_param_response_t nfapi_resp;

	memset(&nfapi_resp, 0, sizeof(nfapi_resp));
	nfapi_resp.header.message_id = NFAPI_PARAM_RESPONSE;
	nfapi_resp.header.phy_id = data->phy_id;
	nfapi_resp.error_code = resp->error_code;

	for(int i = 0; i < resp->number_of_tlvs; ++i)
	{
		switch(resp->tlvs[i].tag)
		{
			case FAPI_PHY_STATE_TAG:
				nfapi_resp.l1_status.phy_state.tl.tag = NFAPI_L1_STATUS_PHY_STATE_TAG;
				nfapi_resp.l1_status.phy_state.value = resp->tlvs[i].value;
				nfapi_resp.num_tlv++;
				break;

			case FAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG:
				nfapi_resp.phy_capabilities.dl_bandwidth_support.tl.tag = NFAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG;
				nfapi_resp.phy_capabilities.dl_bandwidth_support.value = resp->tlvs[i].value;
				nfapi_resp.num_tlv++;
				break;

			case FAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG:
				nfapi_resp.phy_capabilities.ul_bandwidth_support.tl.tag = NFAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG;
				nfapi_resp.phy_capabilities.ul_bandwidth_support.value = resp->tlvs[i].value;
				nfapi_resp.num_tlv++;
				break;
		}
	}
	
	if(1)
	{
		// just code to populate all the tlv for testing with wireshark
		// todo : these should be move up so that they are populated by fapi
		
		nfapi_resp.phy_capabilities.dl_modulation_support.tl.tag = NFAPI_PHY_CAPABILITIES_DL_MODULATION_SUPPORT_TAG;
		nfapi_resp.phy_capabilities.dl_modulation_support.value = rand_range(0, 0x0F);
		nfapi_resp.num_tlv++;
		nfapi_resp.phy_capabilities.ul_modulation_support.tl.tag = NFAPI_PHY_CAPABILITIES_UL_MODULATION_SUPPORT_TAG;
		nfapi_resp.phy_capabilities.ul_modulation_support.value = rand_range(0, 0x07);
		nfapi_resp.num_tlv++;
		nfapi_resp.phy_capabilities.phy_antenna_capability.tl.tag = NFAPI_PHY_CAPABILITIES_PHY_ANTENNA_CAPABILITY_TAG;
		nfapi_resp.phy_capabilities.phy_antenna_capability.value = phy_antenna_capability_values[rand_range(0, 4)];
		nfapi_resp.num_tlv++;
		nfapi_resp.phy_capabilities.release_capability.tl.tag = NFAPI_PHY_CAPABILITIES_RELEASE_CAPABILITY_TAG;
		nfapi_resp.phy_capabilities.release_capability.value = rand_range(0, 0x3F);
		nfapi_resp.num_tlv++;
		nfapi_resp.phy_capabilities.mbsfn_capability.tl.tag = NFAPI_PHY_CAPABILITIES_MBSFN_CAPABILITY_TAG;
		nfapi_resp.phy_capabilities.mbsfn_capability.value = rand_range(0, 1);
		nfapi_resp.num_tlv++;
		
		
		nfapi_resp.laa_capability.laa_support.tl.tag = NFAPI_LAA_CAPABILITY_LAA_SUPPORT_TAG;
		nfapi_resp.laa_capability.laa_support.value = rand_range(0, 1);
		nfapi_resp.num_tlv++;
		nfapi_resp.laa_capability.pd_sensing_lbt_support.tl.tag = NFAPI_LAA_CAPABILITY_PD_SENSING_LBT_SUPPORT_TAG;
		nfapi_resp.laa_capability.pd_sensing_lbt_support.value = rand_range(0, 1);		
		nfapi_resp.num_tlv++;
		nfapi_resp.laa_capability.multi_carrier_lbt_support.tl.tag = NFAPI_LAA_CAPABILITY_MULTI_CARRIER_LBT_SUPPORT_TAG;
		nfapi_resp.laa_capability.multi_carrier_lbt_support.value = rand_range(0, 0x0F);		
		nfapi_resp.num_tlv++;
		nfapi_resp.laa_capability.partial_sf_support.tl.tag = NFAPI_LAA_CAPABILITY_PARTIAL_SF_SUPPORT_TAG;
		nfapi_resp.laa_capability.partial_sf_support.value = rand_range(0, 1);		
		nfapi_resp.num_tlv++;
		
		nfapi_resp.nb_iot_capability.nb_iot_support.tl.tag = NFAPI_LAA_CAPABILITY_NB_IOT_SUPPORT_TAG;
		nfapi_resp.nb_iot_capability.nb_iot_support.value = rand_range(0, 2);		
		nfapi_resp.num_tlv++;
		nfapi_resp.nb_iot_capability.nb_iot_operating_mode_capability.tl.tag = NFAPI_LAA_CAPABILITY_NB_IOT_OPERATING_MODE_CAPABILITY_TAG;
		nfapi_resp.nb_iot_capability.nb_iot_operating_mode_capability.value = rand_range(0, 1);		
		nfapi_resp.num_tlv++;
		
		nfapi_resp.subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.subframe_config.pcfich_power_offset.tl.tag = NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.subframe_config.pb.tl.tag = NFAPI_SUBFRAME_CONFIG_PB_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.subframe_config.dl_cyclic_prefix_type.tl.tag = NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.subframe_config.ul_cyclic_prefix_type.tl.tag = NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG;
		nfapi_resp.num_tlv++;		
		
		nfapi_resp.rf_config.dl_channel_bandwidth.tl.tag = NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.rf_config.ul_channel_bandwidth.tl.tag = NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.rf_config.reference_signal_power.tl.tag = NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.rf_config.tx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.rf_config.rx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.phich_config.phich_resource.tl.tag = NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.phich_config.phich_duration.tl.tag = NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.phich_config.phich_power_offset.tl.tag = NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.sch_config.primary_synchronization_signal_epre_eprers.tl.tag = NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.sch_config.secondary_synchronization_signal_epre_eprers.tl.tag = NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.sch_config.physical_cell_id.tl.tag = NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.prach_config.configuration_index.tl.tag = NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.prach_config.root_sequence_index.tl.tag = NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.prach_config.zero_correlation_zone_configuration.tl.tag = NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.prach_config.high_speed_flag.tl.tag = NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.prach_config.frequency_offset.tl.tag = NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.pusch_config.hopping_mode.tl.tag = NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.pusch_config.hopping_offset.tl.tag = NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.pusch_config.number_of_subbands.tl.tag = NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.pucch_config.delta_pucch_shift.tl.tag = NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.pucch_config.n_cqi_rb.tl.tag = NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.pucch_config.n_an_cs.tl.tag = NFAPI_PUCCH_CONFIG_N_AN_CS_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.pucch_config.n1_pucch_an.tl.tag = NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.srs_config.bandwidth_configuration.tl.tag = NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.srs_config.max_up_pts.tl.tag = NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.srs_config.srs_subframe_configuration.tl.tag = NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.srs_config.srs_acknack_srs_simultaneous_transmission.tl.tag = NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.uplink_reference_signal_config.uplink_rs_hopping.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.uplink_reference_signal_config.group_assignment.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.uplink_reference_signal_config.cyclic_shift_1_for_drms.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.tdd_frame_structure_config.subframe_assignment.tl.tag = NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.tdd_frame_structure_config.special_subframe_patterns.tl.tag = NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG;
		nfapi_resp.num_tlv++;
		
		nfapi_resp.l23_config.data_report_mode.tl.tag = NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG;
		nfapi_resp.num_tlv++;
		nfapi_resp.l23_config.sfnsf.tl.tag = NFAPI_L23_CONFIG_SFNSF_TAG;
		nfapi_resp.num_tlv++;
	}
		

	{
		//if(phy->state == NFAPI_PNF_PHY_IDLE)
		//if(nfapi_resp.l1_status.phy_state.value == NFAPI_PNF_PHY_IDLE)
		{
			// -- NFAPI
			// Downlink UEs per Subframe
			nfapi_resp.nfapi_config.dl_ue_per_sf.tl.tag = NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG;
			nfapi_resp.nfapi_config.dl_ue_per_sf.value = data->phy->dl_ues_per_subframe;
			nfapi_resp.num_tlv++;
			// Uplink UEs per Subframe
			nfapi_resp.nfapi_config.ul_ue_per_sf.tl.tag = NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG;
			nfapi_resp.nfapi_config.ul_ue_per_sf.value = data->phy->ul_ues_per_subframe;
			nfapi_resp.num_tlv++;
			// nFAPI RF Bands
			nfapi_resp.nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
			nfapi_resp.nfapi_config.rf_bands.number_rf_bands = 2;
			nfapi_resp.nfapi_config.rf_bands.rf_band[0] = 23;
			nfapi_resp.nfapi_config.rf_bands.rf_band[1] = 7;
			
			// P7 PNF Address IPv4
			nfapi_resp.nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
			struct sockaddr_in pnf_p7_sockaddr;
			pnf_p7_sockaddr.sin_addr.s_addr = inet_addr(data->phy->local_addr.c_str());
			memcpy(&(nfapi_resp.nfapi_config.p7_pnf_address_ipv4.address[0]), &pnf_p7_sockaddr.sin_addr.s_addr, 4);
			nfapi_resp.num_tlv++;
			// P7 PNF Address IPv6
			// P7 PNF Port
			nfapi_resp.nfapi_config.p7_pnf_port.tl.tag = NFAPI_NFAPI_P7_PNF_PORT_TAG;
			nfapi_resp.nfapi_config.p7_pnf_port.value = data->phy->local_port;
			nfapi_resp.num_tlv++;
			// NMM GSM Frequency Bands
			nfapi_resp.nfapi_config.nmm_gsm_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG;
			nfapi_resp.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands = 1;
			nfapi_resp.nfapi_config.nmm_gsm_frequency_bands.bands[0] = 23;
			nfapi_resp.num_tlv++;
			// NMM UMTS Frequency Bands
			nfapi_resp.nfapi_config.nmm_umts_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG;
			nfapi_resp.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands = 1;
			nfapi_resp.nfapi_config.nmm_umts_frequency_bands.bands[0] = 23;
			nfapi_resp.num_tlv++;
			// NMM LTE Frequency Bands
			nfapi_resp.nfapi_config.nmm_lte_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG;
			nfapi_resp.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands = 1;
			nfapi_resp.nfapi_config.nmm_lte_frequency_bands.bands[0] = 23;
			nfapi_resp.num_tlv++;
			// NMM Uplink RSSI supported
			nfapi_resp.nfapi_config.nmm_uplink_rssi_supported.tl.tag = NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG;
			nfapi_resp.nfapi_config.nmm_uplink_rssi_supported.value = 1;
			nfapi_resp.num_tlv++;

		}
	}


		
	nfapi_pnf_param_resp(data->config, &nfapi_resp);

	return 0;
}

int fapi_config_response(fapi_t* fapi, fapi_config_resp_t* resp)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_config_response_t nfapi_resp;
	memset(&nfapi_resp, 0, sizeof(nfapi_resp));
	nfapi_resp.header.message_id = NFAPI_CONFIG_RESPONSE;
	nfapi_resp.header.phy_id = data->phy_id;
	nfapi_resp.error_code = resp->error_code;
	nfapi_pnf_config_resp(data->config, &nfapi_resp);

	return 0;
}

int fapi_subframe_ind(fapi_t* fapi, fapi_subframe_ind_t* resp)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	if(data->phy->first_subframe_ind == 0)
	{
		printf("Sending nfapi_pnf_start_resp phy_id:%d\n", data->phy_id);
		nfapi_start_response_t start_resp;
		memset(&start_resp, 0, sizeof(start_resp));
		start_resp.header.message_id = NFAPI_START_RESPONSE;
		start_resp.header.phy_id = data->phy_id;
		start_resp.error_code = NFAPI_MSG_OK;
		nfapi_pnf_start_resp(data->config, &start_resp);
		
		data->phy->first_subframe_ind = 1;

		if(data->phy->udp.enabled)
		{
			fapi_start_data(fapi, 
							data->phy->udp.rx_port, 
							data->phy->udp.tx_addr.c_str(), 
							data->phy->udp.tx_port);
		}
	
	}


	nfapi_pnf_p7_subframe_ind(data->p7_config, data->phy_id, resp->sfn_sf);
	
	return 0;

}

int fapi_harq_ind(fapi_t* fapi, fapi_harq_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_harq_indication_t harq_ind;
	memset(&harq_ind, 0, sizeof(harq_ind));
	harq_ind.header.message_id = NFAPI_HARQ_INDICATION;
	harq_ind.header.phy_id = data->p7_config->phy_id;
	harq_ind.sfn_sf = ind->sfn_sf;

	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		harq_ind.harq_indication_body.tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
		harq_ind.harq_indication_body.number_of_harqs = 1;
	
		nfapi_harq_indication_pdu_t pdus[harq_ind.harq_indication_body.number_of_harqs];
		memset(&pdus, 0, sizeof(pdus));
		
		pdus[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
		pdus[0].rx_ue_information.handle = rand_range(0, 9999);
		pdus[0].rx_ue_information.rnti = rand_range(1, 65535);
		
		
		
		pdus[0].harq_indication_tdd_rel8.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL8_TAG;
		pdus[0].harq_indication_tdd_rel8.mode = rand_range(0, 4);
		pdus[0].harq_indication_tdd_rel8.number_of_ack_nack = rand_range(1, 4);
		
		switch(pdus[0].harq_indication_tdd_rel8.mode)
		{
			case 0:
			{
				pdus[0].harq_indication_tdd_rel8.harq_data.bundling.value_0 = rand_range(1, 7);
				pdus[0].harq_indication_tdd_rel8.harq_data.bundling.value_1 = rand_range(1, 7);
			}
			break;
			case 1:
			{
				pdus[0].harq_indication_tdd_rel8.harq_data.multiplex.value_0 = rand_range(1, 7);
				pdus[0].harq_indication_tdd_rel8.harq_data.multiplex.value_1 = rand_range(1, 7);
				pdus[0].harq_indication_tdd_rel8.harq_data.multiplex.value_2 = rand_range(1, 7);
				pdus[0].harq_indication_tdd_rel8.harq_data.multiplex.value_3 = rand_range(1, 7);
			}
			break;
			case 2:
			{
				pdus[0].harq_indication_tdd_rel8.harq_data.special_bundling.value_0 = rand_range(1, 7);
			}
			break;
		};
		
		
		pdus[0].harq_indication_tdd_rel9.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL9_TAG;
		pdus[0].harq_indication_tdd_rel9.mode = rand_range(0, 4);
		pdus[0].harq_indication_tdd_rel9.number_of_ack_nack = 1;
		
		switch(pdus[0].harq_indication_tdd_rel9.mode)
		{
			case 0:
			{		
				pdus[0].harq_indication_tdd_rel9.harq_data[0].bundling.value_0 = rand_range(1, 7);
			}
			break;
			case 1:
			{
				pdus[0].harq_indication_tdd_rel9.harq_data[0].multiplex.value_0 = rand_range(1, 7);
			}
			break;
			case 2:
			{
				pdus[0].harq_indication_tdd_rel9.harq_data[0].special_bundling.value_0 = rand_range(1, 7);
			}
			break;
			case 3:
			{
				pdus[0].harq_indication_tdd_rel9.harq_data[0].channel_selection.value_0 = rand_range(1, 7);
			}
			break;
			case 4:
			{
				pdus[0].harq_indication_tdd_rel9.harq_data[0].format_3.value_0 = rand_range(1, 7);
			}
			break;
		};

	
	
		pdus[0].harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
		pdus[0].harq_indication_tdd_rel13.mode = rand_range(0, 6);
		pdus[0].harq_indication_tdd_rel13.number_of_ack_nack = 1;

		switch(pdus[0].harq_indication_tdd_rel13.mode)
		{
			case 0:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = rand_range(1, 7);		
			}
			break;
			case 1:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].multiplex.value_0 = rand_range(1, 7);
			}
			break;			
			case 2:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].special_bundling.value_0 = rand_range(1, 7);
			}
			break;			
			case 3:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].channel_selection.value_0 = rand_range(1, 7);
			}
			break;			
			case 4:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].format_3.value_0 = rand_range(1, 7);
			}
			break;			
			case 5:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].format_4.value_0 = rand_range(1, 7);
			}
			break;			
			case 6:
			{
				pdus[0].harq_indication_tdd_rel13.harq_data[0].format_5.value_0 = rand_range(1, 7);
			}
			break;			
		};
	
		pdus[0].harq_indication_fdd_rel8.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL8_TAG;
		pdus[0].harq_indication_fdd_rel8.harq_tb1 = rand_range(1, 7);
		pdus[0].harq_indication_fdd_rel8.harq_tb2 = rand_range(1, 7);
		
		pdus[0].harq_indication_fdd_rel9.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL9_TAG;
		pdus[0].harq_indication_fdd_rel9.mode = rand_range(0, 2);
		pdus[0].harq_indication_fdd_rel9.number_of_ack_nack = 2;
		pdus[0].harq_indication_fdd_rel9.harq_tb_n[0] = rand_range(1, 7);
		pdus[0].harq_indication_fdd_rel9.harq_tb_n[1] = rand_range(1, 7);
	
		pdus[0].harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
		pdus[0].harq_indication_fdd_rel13.mode = rand_range(0, 2);
		pdus[0].harq_indication_fdd_rel13.number_of_ack_nack = 2;
		pdus[0].harq_indication_fdd_rel13.harq_tb_n[0] = rand_range(1, 7);
		pdus[0].harq_indication_fdd_rel13.harq_tb_n[1] = rand_range(1, 7);
	
		pdus[0].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
		pdus[0].ul_cqi_information.ul_cqi = rand_range(0,255);
		pdus[0].ul_cqi_information.channel	 = rand_range(0, 1);

		harq_ind.harq_indication_body.harq_pdu_list = pdus;
		
		nfapi_pnf_p7_harq_ind(data->p7_config, &harq_ind);	
	}
	else
	{

		harq_ind.harq_indication_body.tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
		harq_ind.harq_indication_body.number_of_harqs = 8;
	
		nfapi_harq_indication_pdu_t pdus[8];
		memset(&pdus, 0, sizeof(pdus));
	
		for(int i = 0; i < 8; ++i)
		{
			pdus[i].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
			pdus[i].rx_ue_information.handle = 0xFF;
			pdus[i].rx_ue_information.rnti = i;
	
			pdus[i].harq_indication_fdd_rel8.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL8_TAG;
			pdus[i].harq_indication_fdd_rel8.harq_tb1 = 1;
			pdus[i].harq_indication_fdd_rel8.harq_tb2 = 2;
		}
	
		harq_ind.harq_indication_body.harq_pdu_list = pdus;
		
		nfapi_pnf_p7_harq_ind(data->p7_config, &harq_ind);	
	}
	
	return 0;	
}

int fapi_crc_ind(fapi_t* fapi, fapi_crc_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_crc_indication_t crc_ind;
	memset(&crc_ind, 0, sizeof(crc_ind));
	crc_ind.header.message_id = NFAPI_CRC_INDICATION;
	crc_ind.header.phy_id = data->p7_config->phy_id;
	crc_ind.sfn_sf = ind->sfn_sf;
	
	
	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		crc_ind.crc_indication_body.tl.tag = NFAPI_CRC_INDICATION_BODY_TAG;
		crc_ind.crc_indication_body.number_of_crcs = 1;
		
		nfapi_crc_indication_pdu_t pdus[crc_ind.crc_indication_body.number_of_crcs];
		memset(&pdus, 0, sizeof(pdus));
		
		pdus[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
		pdus[0].rx_ue_information.handle = rand_range(0, 9999);
		pdus[0].rx_ue_information.rnti = rand_range(1, 65535);
		
		pdus[0].crc_indication_rel8.tl.tag = NFAPI_CRC_INDICATION_REL8_TAG;
		pdus[0].crc_indication_rel8.crc_flag = rand_range(0, 1);
		
		crc_ind.crc_indication_body.crc_pdu_list = pdus;
		nfapi_pnf_p7_crc_ind(data->p7_config, &crc_ind);
	}
	else
	{
		crc_ind.crc_indication_body.tl.tag = NFAPI_CRC_INDICATION_BODY_TAG;
		crc_ind.crc_indication_body.number_of_crcs = ind->body.number_of_crcs;
	
		crc_ind.crc_indication_body.crc_pdu_list = (nfapi_crc_indication_pdu_t*)malloc(sizeof(nfapi_crc_indication_pdu_t) * ind->body.number_of_crcs);
	
		for(int i = 0; i < ind->body.number_of_crcs; ++i)
		{
			crc_ind.crc_indication_body.crc_pdu_list[i].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
			crc_ind.crc_indication_body.crc_pdu_list[i].rx_ue_information.handle = ind->body.pdus[i].rx_ue_info.handle;
			crc_ind.crc_indication_body.crc_pdu_list[i].rx_ue_information.rnti = ind->body.pdus[i].rx_ue_info.rnti;
			crc_ind.crc_indication_body.crc_pdu_list[i].crc_indication_rel8.tl.tag = NFAPI_CRC_INDICATION_REL8_TAG;
			crc_ind.crc_indication_body.crc_pdu_list[i].crc_indication_rel8.crc_flag = ind->body.pdus[i].rel8_pdu.crc_flag;
		}
	
		nfapi_pnf_p7_crc_ind(data->p7_config, &crc_ind);
	
		free(crc_ind.crc_indication_body.crc_pdu_list);
	}
	
	return 0;
}
int fapi_rx_ulsch_ind(fapi_t* fapi, fapi_rx_ulsch_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_rx_indication_t rx_ind;
	memset(&rx_ind, 0, sizeof(rx_ind));
	rx_ind.header.message_id = NFAPI_RX_ULSCH_INDICATION;
	rx_ind.header.phy_id = data->p7_config->phy_id;
	rx_ind.sfn_sf = ind->sfn_sf;
	
	if(1)//((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
		rx_ind.rx_indication_body.number_of_pdus = 1;
		
		uint8_t rx_data[1024];
		
		nfapi_rx_indication_pdu_t pdus[rx_ind.rx_indication_body.number_of_pdus];
		memset(&pdus, 0, sizeof(pdus));
		
                strcpy((char*)rx_data, (char*)"123456789");

		for(int i = 0; i < rx_ind.rx_indication_body.number_of_pdus;++i)
		{
		
			pdus[i].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
			pdus[i].rx_ue_information.handle = rand_range(0, 9999);
			pdus[i].rx_ue_information.rnti = rand_range(1, 65535);
			
			pdus[i].rx_indication_rel8.tl.tag = NFAPI_RX_INDICATION_REL8_TAG;
			pdus[i].rx_indication_rel8.length = 10;//rand_range(0, 1024);
			pdus[i].rx_indication_rel8.offset = 0;//djp - 1;
			pdus[i].rx_indication_rel8.ul_cqi = rand_range(0, 255);
			pdus[i].rx_indication_rel8.timing_advance = rand_range(0, 63);
			
			//pdus[i].rx_indication_rel9.tl.tag = NFAPI_RX_INDICATION_REL9_TAG;
			//pdus[i].rx_indication_rel9.timing_advance_r9 = rand_range(0, 7690);
			
			pdus[i].data = &rx_data[0];
		}
		
		rx_ind.rx_indication_body.rx_pdu_list = pdus;
		
		nfapi_pnf_p7_rx_ind(data->p7_config, &rx_ind);
	}
	else
	{

		rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
		rx_ind.rx_indication_body.number_of_pdus = ind->body.number_of_pdus;
	
		rx_ind.rx_indication_body.rx_pdu_list = (nfapi_rx_indication_pdu_t*)malloc(sizeof(nfapi_rx_indication_pdu_t) * ind->body.number_of_pdus);
	
		for(int i = 0; i < ind->body.number_of_pdus; ++i)
		{
			rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
			rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.handle = ind->body.pdus[i].rx_ue_info.handle;
			rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.tl.tag = NFAPI_RX_INDICATION_REL8_TAG;
			rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length = ind->body.pdus[i].rel8_pdu.length;
			rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.offset = 1;
			rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel9.tl.tag = 0;
			rx_ind.rx_indication_body.rx_pdu_list[i].data = (uint8_t*)ind->body.data[i];
	
		}
		
		nfapi_pnf_p7_rx_ind(data->p7_config, &rx_ind);
	
		free(rx_ind.rx_indication_body.rx_pdu_list);
	}
	
	return 0;

}
int fapi_rx_cqi_ind(fapi_t* fapi, fapi_rx_cqi_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_cqi_indication_t cqi_ind;
	memset(&cqi_ind, 0, sizeof(cqi_ind));
	cqi_ind.header.message_id = NFAPI_RX_CQI_INDICATION;
	cqi_ind.header.phy_id = data->p7_config->phy_id;
	cqi_ind.sfn_sf = ind->sfn_sf;
	
	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		cqi_ind.cqi_indication_body.tl.tag = NFAPI_CQI_INDICATION_BODY_TAG;
		cqi_ind.cqi_indication_body.number_of_cqis = 3;
		
		nfapi_cqi_indication_pdu_t cqi_pdu_list[cqi_ind.cqi_indication_body.number_of_cqis];
		memset(&cqi_pdu_list, 0, sizeof(cqi_pdu_list));
		nfapi_cqi_indication_raw_pdu_t cqi_raw_pdu_list[cqi_ind.cqi_indication_body.number_of_cqis];
		//memset(&cqi_raw_pdu_list, 0, sizeof(cqi_raw_pdu_list));
		
		
		for(int i = 0; i < cqi_ind.cqi_indication_body.number_of_cqis; ++i)
		{
			cqi_pdu_list[i].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
			cqi_pdu_list[i].rx_ue_information.handle = rand_range(0, 9999);
			cqi_pdu_list[i].rx_ue_information.rnti = rand_range(1, 65535);
			
			uint8_t rel8_or_9 = rand_range(0, 1);
			
			if(rel8_or_9)
			{
				cqi_pdu_list[i].cqi_indication_rel8.tl.tag = NFAPI_CQI_INDICATION_REL8_TAG;
				cqi_pdu_list[i].cqi_indication_rel8.length = 8; //rand_range(1, 12);		
				cqi_pdu_list[i].cqi_indication_rel8.data_offset = 1; //rand_range(0, 1);		
				cqi_pdu_list[i].cqi_indication_rel8.ul_cqi = 0;
				cqi_pdu_list[i].cqi_indication_rel8.ri = rand_range(0, 4);		
				cqi_pdu_list[i].cqi_indication_rel8.timing_advance = rand_range(0, 63);		
			}
			else
			{
				cqi_pdu_list[i].cqi_indication_rel9.tl.tag = NFAPI_CQI_INDICATION_REL9_TAG;
				cqi_pdu_list[i].cqi_indication_rel9.length = 8; //rand_range(1, 12);		
				cqi_pdu_list[i].cqi_indication_rel9.data_offset = 1; //rand_range(0, 1);		
				cqi_pdu_list[i].cqi_indication_rel9.ul_cqi = 0; //rand_range(0, 1);		
				cqi_pdu_list[i].cqi_indication_rel9.number_of_cc_reported = 1;
				cqi_pdu_list[i].cqi_indication_rel9.ri[0] = rand_range(0, 8);		
				cqi_pdu_list[i].cqi_indication_rel9.timing_advance = rand_range(0, 63);		
				cqi_pdu_list[i].cqi_indication_rel9.timing_advance_r9 = rand_range(0, 7690);
			}
			
			cqi_pdu_list[i].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
			cqi_pdu_list[i].ul_cqi_information.ul_cqi = rand_range(0,255);
			cqi_pdu_list[i].ul_cqi_information.channel = rand_range(0, 1);	
		}
		
		cqi_ind.cqi_indication_body.cqi_pdu_list = cqi_pdu_list;
		cqi_ind.cqi_indication_body.cqi_raw_pdu_list = cqi_raw_pdu_list;
		
		nfapi_pnf_p7_cqi_ind(data->p7_config, &cqi_ind);
	}
	else
	{
		nfapi_pnf_p7_cqi_ind(data->p7_config, &cqi_ind);
	}
	
	return 0;
}
int fapi_rx_sr_ind(fapi_t* fapi, fapi_rx_sr_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_sr_indication_t sr_ind;
	memset(&sr_ind, 0, sizeof(sr_ind));
	sr_ind.header.message_id = NFAPI_RX_SR_INDICATION;
	sr_ind.header.phy_id = data->p7_config->phy_id;
	sr_ind.sfn_sf = ind->sfn_sf;
	
	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		sr_ind.sr_indication_body.tl.tag = NFAPI_SR_INDICATION_BODY_TAG;
		sr_ind.sr_indication_body.number_of_srs = 1;
		
		nfapi_sr_indication_pdu_t pdus[sr_ind.sr_indication_body.number_of_srs];
		memset(&pdus, 0, sizeof(pdus));
		
		pdus[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
		pdus[0].rx_ue_information.handle = rand_range(0, 9999);
		pdus[0].rx_ue_information.rnti = rand_range(1, 65535);
		
		pdus[0].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
		pdus[0].ul_cqi_information.ul_cqi = rand_range(0,255);
		pdus[0].ul_cqi_information.channel = rand_range(0, 1);			
		
		sr_ind.sr_indication_body.sr_pdu_list = pdus;
		
		nfapi_pnf_p7_sr_ind(data->p7_config, &sr_ind);
	}
	else
	{
		nfapi_pnf_p7_sr_ind(data->p7_config, &sr_ind);	
	}
	
	return 0;
}

int fapi_rach_ind(fapi_t* fapi, fapi_rach_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_rach_indication_t rach_ind;
	memset(&rach_ind, 0, sizeof(rach_ind));
	rach_ind.header.message_id = NFAPI_RACH_INDICATION;
	rach_ind.header.phy_id = data->p7_config->phy_id;
	rach_ind.sfn_sf = ind->sfn_sf;
	
	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		rach_ind.rach_indication_body.tl.tag = NFAPI_RACH_INDICATION_BODY_TAG;
		rach_ind.rach_indication_body.number_of_preambles = 1;
	
		nfapi_preamble_pdu_t pdus[rach_ind.rach_indication_body.number_of_preambles];
		memset(&pdus, 0, sizeof(pdus));
		
		pdus[0].preamble_rel8.tl.tag = NFAPI_PREAMBLE_REL8_TAG;
		pdus[0].preamble_rel8.rnti = rand_range(1, 65535);
		pdus[0].preamble_rel8.preamble = rand_range(0, 63);
		pdus[0].preamble_rel8.timing_advance = rand_range(0, 1282);
		pdus[0].preamble_rel9.tl.tag = NFAPI_PREAMBLE_REL9_TAG;
		pdus[0].preamble_rel9.timing_advance_r9 = rand_range(0, 7690);
		pdus[0].preamble_rel13.tl.tag = NFAPI_PREAMBLE_REL13_TAG;
		pdus[0].preamble_rel13.rach_resource_type = rand_range(0, 4);
			
		rach_ind.rach_indication_body.preamble_list = pdus;
		nfapi_pnf_p7_rach_ind(data->p7_config, &rach_ind);
	}
	else
	{
		nfapi_pnf_p7_rach_ind(data->p7_config, &rach_ind);
	}
	
	return 0;
}

int fapi_srs_ind(fapi_t* fapi, fapi_srs_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);

	nfapi_srs_indication_t srs_ind;
	memset(&srs_ind, 0, sizeof(srs_ind));
	srs_ind.header.message_id = NFAPI_SRS_INDICATION;
	srs_ind.header.phy_id = data->p7_config->phy_id;
	srs_ind.sfn_sf = ind->sfn_sf;

	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		srs_ind.srs_indication_body.tl.tag = NFAPI_SRS_INDICATION_BODY_TAG;
		srs_ind.srs_indication_body.number_of_ues = 1;
		
		nfapi_srs_indication_pdu_t pdus[srs_ind.srs_indication_body.number_of_ues];
		memset(&pdus, 0, sizeof(pdus));		
		
		pdus[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
		pdus[0].rx_ue_information.handle = rand_range(0, 9999);
		pdus[0].rx_ue_information.rnti = rand_range(1, 65535);
				
		pdus[0].srs_indication_fdd_rel8.tl.tag = NFAPI_SRS_INDICATION_FDD_REL8_TAG;
		pdus[0].srs_indication_fdd_rel8.doppler_estimation = rand_range(0, 255);
		pdus[0].srs_indication_fdd_rel8.timing_advance = rand_range(0, 63);
		pdus[0].srs_indication_fdd_rel8.number_of_resource_blocks = 2; //rand_range(0, 255);
		pdus[0].srs_indication_fdd_rel8.rb_start = rand_range(0, 245);
		pdus[0].srs_indication_fdd_rel8.snr[0] = rand_range(0, 255);
		pdus[0].srs_indication_fdd_rel8.snr[1] = rand_range(0, 255);
		
		pdus[0].srs_indication_fdd_rel9.tl.tag = NFAPI_SRS_INDICATION_FDD_REL9_TAG;
		pdus[0].srs_indication_fdd_rel9.timing_advance_r9 = rand_range(0, 7690);
		
		pdus[0].srs_indication_tdd_rel10.tl.tag = NFAPI_SRS_INDICATION_TDD_REL10_TAG;
		pdus[0].srs_indication_tdd_rel10.uppts_symbol = rand_range(0, 1);
		
		pdus[0].srs_indication_fdd_rel11.tl.tag = NFAPI_SRS_INDICATION_FDD_REL11_TAG;
		pdus[0].srs_indication_fdd_rel11.ul_rtoa;		
		
		pdus[0].tdd_channel_measurement.tl.tag = NFAPI_TDD_CHANNEL_MEASUREMENT_TAG;
		pdus[0].tdd_channel_measurement.num_prb_per_subband = rand_range(0, 255);
		pdus[0].tdd_channel_measurement.number_of_subbands = 1;
		pdus[0].tdd_channel_measurement.num_atennas = 2;
		pdus[0].tdd_channel_measurement.subands[0].subband_index = rand_range(0, 255);
		pdus[0].tdd_channel_measurement.subands[0].channel[0] = rand_range(0, 9999);
		pdus[0].tdd_channel_measurement.subands[0].channel[1] = rand_range(0, 9999);
	
		srs_ind.srs_indication_body.srs_pdu_list = pdus;
		nfapi_pnf_p7_srs_ind(data->p7_config, &srs_ind);
	}
	else
	{
		nfapi_pnf_p7_srs_ind(data->p7_config, &srs_ind);
	}
	
	return 0;
}

int fapi_lbt_dl_ind(fapi_t* fapi, fapi_lbt_dl_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);
	
	nfapi_lbt_dl_indication_t lbt_dl_ind;
	memset(&lbt_dl_ind, 0, sizeof(lbt_dl_ind));
	lbt_dl_ind.header.message_id = NFAPI_LBT_DL_INDICATION;
	lbt_dl_ind.header.phy_id = data->p7_config->phy_id;
	lbt_dl_ind.sfn_sf = ind->sfn_sf;

	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		lbt_dl_ind.lbt_dl_indication_body.tl.tag = NFAPI_LBT_DL_INDICATION_BODY_TAG;
		lbt_dl_ind.lbt_dl_indication_body.number_of_pdus = 2;
		
		nfapi_lbt_dl_indication_pdu_t pdus[lbt_dl_ind.lbt_dl_indication_body.number_of_pdus];
		memset(&pdus, 0, sizeof(pdus));	
		
		pdus[0].pdu_type = 0; // LBT_PDSCH_RSP PDU
		pdus[0].pdu_size = 0;
		pdus[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.tl.tag = NFAPI_LBT_PDSCH_RSP_PDU_REL13_TAG;
		pdus[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.handle = 0xABCD;
		pdus[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.result = rand_range(0, 1);
		pdus[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.lte_txop_symbols = rand_range(0, 0xFFFF);
		pdus[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.initial_partial_sf = rand_range(0, 1);
		
		pdus[1].pdu_type = 1; // LBT_DRS_RSP PDU
		pdus[1].pdu_size = 0;
		pdus[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.tl.tag = NFAPI_LBT_DRS_RSP_PDU_REL13_TAG;
		pdus[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.handle = 0xABCD;
		pdus[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.result = rand_range(0, 1);

		lbt_dl_ind.lbt_dl_indication_body.lbt_indication_pdu_list = pdus;
		nfapi_pnf_p7_lbt_dl_ind(data->p7_config, &lbt_dl_ind);
	}
	else
	{
		nfapi_pnf_p7_lbt_dl_ind(data->p7_config, &lbt_dl_ind);
	}
	
	return 0;
}

int fapi_nb_harq_ind(fapi_t* fapi, fapi_nb_harq_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);
	
	nfapi_nb_harq_indication_t nb_harq_ind;
	memset(&nb_harq_ind, 0, sizeof(nb_harq_ind));
	nb_harq_ind.header.message_id = NFAPI_NB_HARQ_INDICATION;
	nb_harq_ind.header.phy_id = data->p7_config->phy_id;
	nb_harq_ind.sfn_sf = ind->sfn_sf;

	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		
		nb_harq_ind.nb_harq_indication_body.tl.tag = NFAPI_NB_HARQ_INDICATION_BODY_TAG;
		nb_harq_ind.nb_harq_indication_body.number_of_harqs = 1;
		
		nfapi_nb_harq_indication_pdu_t pdus[nb_harq_ind.nb_harq_indication_body.number_of_harqs];
		memset(&pdus, 0, sizeof(pdus));	
		
		pdus[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
		pdus[0].rx_ue_information.handle = rand_range(0, 0xFFFF);
		pdus[0].rx_ue_information.rnti = rand_range(0, 65535);
		
		pdus[0].nb_harq_indication_fdd_rel13.tl.tag = NFAPI_NB_HARQ_INDICATION_FDD_REL13_TAG;
		pdus[0].nb_harq_indication_fdd_rel13.harq_tb1 = rand_range(1, 7);
		
		pdus[0].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
		pdus[0].ul_cqi_information.ul_cqi = rand_range(0, 255);
		pdus[0].ul_cqi_information.channel = rand_range(0, 1);
		
		nb_harq_ind.nb_harq_indication_body.nb_harq_pdu_list = pdus;
		nfapi_pnf_p7_nb_harq_ind(data->p7_config, &nb_harq_ind);
	}
	else
	{
		nfapi_pnf_p7_nb_harq_ind(data->p7_config, &nb_harq_ind);
	}
	
	return 0;
}

int fapi_nrach_ind(fapi_t* fapi, fapi_nrach_ind_t* ind)
{
	pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)(fapi->user_data);
	
	nfapi_nrach_indication_t nrach_ind;
	memset(&nrach_ind, 0, sizeof(nrach_ind));
	nrach_ind.header.message_id = NFAPI_NRACH_INDICATION;
	nrach_ind.header.phy_id = data->p7_config->phy_id;
	nrach_ind.sfn_sf = ind->sfn_sf;

	if(((pnf_info*)(data->config->user_data))->wireshark_test_mode)
	{
		nrach_ind.nrach_indication_body.tl.tag = NFAPI_NRACH_INDICATION_BODY_TAG;
		nrach_ind.nrach_indication_body.number_of_initial_scs_detected = 1;
		
		nfapi_nrach_indication_pdu_t pdus[nrach_ind.nrach_indication_body.number_of_initial_scs_detected];
		memset(&pdus, 0, sizeof(pdus));	
		
		pdus[0].nrach_indication_rel13.tl.tag = NFAPI_NRACH_INDICATION_REL13_TAG;
		pdus[0].nrach_indication_rel13.rnti = rand_range(1, 65535);
		pdus[0].nrach_indication_rel13.initial_sc = rand_range(0, 47);
		pdus[0].nrach_indication_rel13.timing_advance = rand_range(0, 3840);
		pdus[0].nrach_indication_rel13.nrach_ce_level = rand_range(0, 2);
		
		nrach_ind.nrach_indication_body.nrach_pdu_list = pdus;
		
		nfapi_pnf_p7_nrach_ind(data->p7_config, &nrach_ind);
	}
	else
	{
		nfapi_pnf_p7_nrach_ind(data->p7_config, &nrach_ind);
	}	
	
	return 0;
}

int pnf_start_request(nfapi_pnf_config_t* config, nfapi_pnf_start_request_t* req)
{

	pnf_info* pnf = (pnf_info*)(config->user_data);

	// start all phys that have been configured
	for(phy_info& phy : pnf->phys)
	{
		if(phy.id != 0)
		{
	//auto found = std::find_if(pnf->phys.begin(), pnf->phys.end(), [&](phy_info& item)
	//		{ return item.id == req->header.phy_id; });
//
//	if(found != pnf->phys.end())
//	{
//		phy_info& phy = (*found);

			fapi_cb_t cb;
			cb.fapi_param_response = &fapi_param_response;
			cb.fapi_config_response = &fapi_config_response;
			cb.fapi_subframe_ind = &fapi_subframe_ind;
			cb.fapi_harq_ind = fapi_harq_ind;
			cb.fapi_crc_ind = fapi_crc_ind;
			cb.fapi_rx_ulsch_ind = fapi_rx_ulsch_ind;
			cb.fapi_rx_cqi_ind = fapi_rx_cqi_ind;
			cb.fapi_rx_sr_ind = fapi_rx_sr_ind;
			cb.fapi_rach_ind = fapi_rach_ind;
			cb.fapi_srs_ind = fapi_srs_ind;
			
			cb.fapi_lbt_dl_ind = fapi_lbt_dl_ind;
			cb.fapi_nb_harq_ind = fapi_nb_harq_ind;
			cb.fapi_nrach_ind = fapi_nrach_ind;
			
			

			fapi_config_t c;
			c.duplex_mode = phy.duplex_mode;
			c.dl_channel_bw_support = phy.dl_channel_bw_support;
			c.ul_channel_bw_support = phy.ul_channel_bw_support;

			phy.fapi = fapi_create(&cb, &c);
			printf("[PNF_SIM] staring fapi %p phy_id:%d\n", phy.fapi, phy.id);

			pnf_phy_user_data_t* data = (pnf_phy_user_data_t*)malloc(sizeof(pnf_phy_user_data_t));
			data->phy_id = phy.id;
			data->config = config;
			data->phy = &phy;

			phy.fapi->user_data = data; 
		}

	}

	nfapi_pnf_start_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_START_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_pnf_start_resp(config, &resp);

	return 0;
}

int pnf_stop_request(nfapi_pnf_config_t* config, nfapi_pnf_stop_request_t* req)
{
	printf("[PNF_SIM] pnf stop request\n");

	nfapi_pnf_stop_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_PNF_STOP_RESPONSE;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_pnf_stop_resp(config, &resp);

	return 0;
}

int param_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_param_request_t* req)
{
	printf("[PNF_SIM] param request phy_id:%d\n", req->header.phy_id);

	pnf_info* pnf = (pnf_info*)(config->user_data);

	auto found = std::find_if(pnf->phys.begin(), pnf->phys.end(), [&](phy_info& item)
								  { return item.id == req->header.phy_id; });

	if(found != pnf->phys.end())
	{
		phy_info& phy_info = (*found);

		fapi_param_req_t fapi_req;
		fapi_req.header.message_id = req->header.message_id;
		fapi_req.header.length = 0;

		// convert nfapi to fapi

		fapi_param_request(phy_info.fapi, &fapi_req);

	}
	else
	{
		// did not find the phy
	}


	printf("[PNF_SIM] param request .. exit\n");


	return 0;
}

int config_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_config_request_t* req)
{
	printf("[PNF_SIM] config request phy_id:%d\n", req->header.phy_id);

	pnf_info* pnf = (pnf_info*)(config->user_data);

	auto found = std::find_if(pnf->phys.begin(), pnf->phys.end(), [&](phy_info& item)
								  { return item.id == req->header.phy_id; });

	if(found != pnf->phys.end())
	{
		phy_info& phy_info = (*found);


		if(req->nfapi_config.timing_window.tl.tag == NFAPI_NFAPI_TIMING_WINDOW_TAG)
		{
			phy_info.timing_window = req->nfapi_config.timing_window.value;
		}

		if(req->nfapi_config.timing_info_mode.tl.tag == NFAPI_NFAPI_TIMING_INFO_MODE_TAG)
		{
			printf("timing info mode provided\n");
			phy_info.timing_info_mode = req->nfapi_config.timing_info_mode.value;
		}
		else 
		{
			phy_info.timing_info_mode = 0;
			printf("NO timing info mode provided\n");
		}

		if(req->nfapi_config.timing_info_period.tl.tag == NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG)
		{
			printf("timing info period provided\n");
			phy_info.timing_info_period = req->nfapi_config.timing_info_period.value;
		}
		else 
		{
			phy_info.timing_info_period = 0;
		}

		phy_info.remote_port = req->nfapi_config.p7_vnf_port.value;

		struct sockaddr_in vnf_p7_sockaddr;
		memcpy(&vnf_p7_sockaddr.sin_addr.s_addr, &(req->nfapi_config.p7_vnf_address_ipv4.address[0]), 4);
		phy_info.remote_addr = inet_ntoa(vnf_p7_sockaddr.sin_addr);

		printf("[PNF_SIM] %d vnf p7 %s:%d timing %d %d %d\n", phy_info.id, phy_info.remote_addr, phy_info.remote_port, 
				phy_info.timing_window, phy_info.timing_info_mode, phy_info.timing_info_period);

		fapi_config_req_t fapi_req;
		fapi_config_request(phy_info.fapi, &fapi_req);
	}

	return 0;
}

nfapi_p7_message_header_t* phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t* msg_size)
{
	if(message_id == P7_VENDOR_EXT_REQ)
	{
		(*msg_size) = sizeof(vendor_ext_p7_req);
		return (nfapi_p7_message_header_t*)malloc(sizeof(vendor_ext_p7_req));
	}
	
	return 0;
}

void phy_deallocate_p7_vendor_ext(nfapi_p7_message_header_t* header)
{
	free(header);
}

int phy_dl_config_req(nfapi_pnf_p7_config_t* pnf_p7, nfapi_dl_config_request_t* req)
{
	//printf("[PNF_SIM] dl config request\n");

	if(req->vendor_extension)
		free(req->vendor_extension);

	phy_info* phy = (phy_info*)(pnf_p7->user_data);

	fapi_dl_config_req_t fapi_req;
	// convert
	fapi_dl_config_request(phy->fapi, &fapi_req);

	return 0;
}
int phy_ul_config_req(nfapi_pnf_p7_config_t* pnf_p7, nfapi_ul_config_request_t* req)
{
	//printf("[PNF_SIM] ul config request\n");
	phy_info* phy = (phy_info*)(pnf_p7->user_data);

	fapi_ul_config_req_t fapi_req;
	// convert
	fapi_ul_config_request(phy->fapi, &fapi_req);

	return 0;
}
int phy_hi_dci0_req(nfapi_pnf_p7_config_t* pnf_p7, nfapi_hi_dci0_request_t* req)
{
	//printf("[PNF_SIM] hi dci0 request\n");
	phy_info* phy = (phy_info*)(pnf_p7->user_data);

	fapi_hi_dci0_req_t fapi_req;
	// convert
	fapi_hi_dci0_request(phy->fapi, &fapi_req);
	return 0;
}
int phy_tx_req(nfapi_pnf_p7_config_t* pnf_p7, nfapi_tx_request_t* req)
{
	//printf("[PNF_SIM] tx request\n");
	phy_info* phy = (phy_info*)(pnf_p7->user_data);

	fapi_tx_req_t fapi_req;
	fapi_req.header.message_id = FAPI_TX_REQUEST;
	fapi_req.sfn_sf = req->sfn_sf;
	fapi_req.body.number_of_pdus = req->tx_request_body.number_of_pdus;

	fapi_tx_req_pdu_t pdus[8];
	fapi_req.body.pdus = &pdus[0];

	for(int i = 0; i < fapi_req.body.number_of_pdus; ++i)
	{
		fapi_req.body.pdus[i].pdu_length = req->tx_request_body.tx_pdu_list[i].pdu_length;
		fapi_req.body.pdus[i].pdu_index = req->tx_request_body.tx_pdu_list[i].pdu_index;
		fapi_req.body.pdus[i].num_tlv = 1;
		fapi_req.body.pdus[i].tlvs[0].value = (uint32_t*)req->tx_request_body.tx_pdu_list[i].segments[0].segment_data;

		//if the pnf wants to retain the pointer then req->tx_request_body.tx_pdu_list[i].segments[0].segment_data should be set to 0


	}

	fapi_tx_request(phy->fapi, &fapi_req);
/*
	if(fapi_req.body.number_of_pdus > 0)
	{
		for(int i = 0; i < fapi_req.body.number_of_pdus; ++i)
		{
			//printf("freeing tx pdu %p\n", fapi_req.body.pdus[i].tlvs[0].value);
			if(0)
			{
				free(fapi_req.body.pdus[i].tlvs[0].value);
			}
			else
			{
				pnf_deallocate(fapi_req.body.pdus[i].tlvs[0].value);
			}
		}
	}
*/	

	return 0;
}
int phy_lbt_dl_config_req(nfapi_pnf_p7_config_t*, nfapi_lbt_dl_config_request_t* req)
{
	//printf("[PNF_SIM] lbt dl config request\n");
	return 0;
}

int phy_vendor_ext(nfapi_pnf_p7_config_t* config, nfapi_p7_message_header_t* msg)
{
	if(msg->message_id == P7_VENDOR_EXT_REQ)
	{
		vendor_ext_p7_req* req = (vendor_ext_p7_req*)msg;
		//printf("[PNF_SIM] vendor request (1:%d 2:%d)\n", req->dummy1, req->dummy2);
	}
	else
	{
		printf("[PNF_SIM] unknown vendor ext\n");
	}
	return 0;
}



int phy_pack_p7_vendor_extension(nfapi_p7_message_header_t* header, uint8_t** ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t* codex)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P7_VENDOR_EXT_IND)
	{
		vendor_ext_p7_ind* ind = (vendor_ext_p7_ind*)(header);
		if(!push16(ind->error_code, ppWritePackedMsg, end))
			return 0;
		
		return 1;
	}
	return -1;
}

int phy_unpack_p7_vendor_extension(nfapi_p7_message_header_t* header, uint8_t** ppReadPackedMessage, uint8_t *end, nfapi_p7_codec_config_t* codec)
{
	if(header->message_id == P7_VENDOR_EXT_REQ)
	{
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
		vendor_ext_p7_req* req = (vendor_ext_p7_req*)(header);
		if(!(pull16(ppReadPackedMessage, &req->dummy1, end) &&
			 pull16(ppReadPackedMessage, &req->dummy2, end)))
			return 0;
		return 1;
	}
	return -1;
}

int phy_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMessage, uint8_t* end, void** ve, nfapi_p7_codec_config_t* config)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "phy_unpack_vendor_extension_tlv\n");

	switch(tl->tag)
	{
		case VENDOR_EXT_TLV_1_TAG:
			*ve = malloc(sizeof(vendor_ext_tlv_1));
			if(!pull32(ppReadPackedMessage, &((vendor_ext_tlv_1*)(*ve))->dummy, end))
				return 0;

			return 1;
			break;
	}

	return -1;
}

int phy_pack_vendor_extention_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t* end, nfapi_p7_codec_config_t* config)
{
	//printf("%s\n", __FUNCTION__);
	(void)ve;
	(void)ppWritePackedMsg;
	return -1;
}

int pnf_sim_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMessage, uint8_t *end, void** ve, nfapi_p4_p5_codec_config_t* config)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "pnf_sim_unpack_vendor_extension_tlv\n");

	switch(tl->tag)
	{
		case VENDOR_EXT_TLV_2_TAG:
			*ve = malloc(sizeof(vendor_ext_tlv_2));
			if(!pull32(ppReadPackedMessage, &((vendor_ext_tlv_2*)(*ve))->dummy, end))
				return 0;

			return 1;
			break;
	}
	
	return -1;
}

int pnf_sim_pack_vendor_extention_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* config)
{
	//printf("%s\n", __FUNCTION__);
	(void)ve;
	(void)ppWritePackedMsg;
	return -1;
}
int start_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_start_request_t* req)
{
	printf("[PNF_SIM] start request phy_id:%d\n", req->header.phy_id);

	pnf_info* pnf = (pnf_info*)(config->user_data);

	auto found = std::find_if(pnf->phys.begin(), pnf->phys.end(), [&](phy_info& item)
								  { return item.id == req->header.phy_id; });

	if(found != pnf->phys.end())
	{
		phy_info& phy_info = (*found);


		nfapi_pnf_p7_config_t* p7_config = nfapi_pnf_p7_config_create();

		p7_config->phy_id = phy->phy_id;

		p7_config->remote_p7_port = phy_info.remote_port;
		p7_config->remote_p7_addr = phy_info.remote_addr;
		p7_config->local_p7_port = phy_info.local_port;
		p7_config->local_p7_addr = (char*)phy_info.local_addr.c_str();
		p7_config->user_data = &phy_info;

		p7_config->malloc = &pnf_allocate;
		p7_config->free = &pnf_deallocate;
		p7_config->codec_config.allocate = &pnf_allocate;
		p7_config->codec_config.deallocate = &pnf_deallocate;
		
		p7_config->trace = &pnf_sim_trace;

		phy->user_data = p7_config;

		p7_config->subframe_buffer_size = phy_info.timing_window;
		if(phy_info.timing_info_mode & 0x1)
		{
			p7_config->timing_info_mode_periodic = 1;
			p7_config->timing_info_period = phy_info.timing_info_period;
		}
		
		if(phy_info.timing_info_mode & 0x2)
		{
			p7_config->timing_info_mode_aperiodic = 1;
		}

		p7_config->dl_config_req = &phy_dl_config_req;
		p7_config->ul_config_req = &phy_ul_config_req;
		p7_config->hi_dci0_req = &phy_hi_dci0_req;
		p7_config->tx_req = &phy_tx_req;
		p7_config->lbt_dl_config_req = &phy_lbt_dl_config_req;

		p7_config->vendor_ext = &phy_vendor_ext;

		p7_config->allocate_p7_vendor_ext = &phy_allocate_p7_vendor_ext;
		p7_config->deallocate_p7_vendor_ext = &phy_deallocate_p7_vendor_ext;

		p7_config->codec_config.unpack_p7_vendor_extension = &phy_unpack_p7_vendor_extension;
		p7_config->codec_config.pack_p7_vendor_extension = &phy_pack_p7_vendor_extension;
		p7_config->codec_config.unpack_vendor_extension_tlv = &phy_unpack_vendor_extension_tlv;
		p7_config->codec_config.pack_vendor_extension_tlv = &phy_pack_vendor_extention_tlv;

		pthread_t p7_thread;
		pthread_create(&p7_thread, NULL, &pnf_p7_thread_start, p7_config);


		((pnf_phy_user_data_t*)(phy_info.fapi->user_data))->p7_config = p7_config;

		fapi_start_req_t fapi_req;
		fapi_start_request(phy_info.fapi, &fapi_req);

	}

	return 0;
}

int measurement_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_measurement_request_t* req)
{
	nfapi_measurement_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_MEASUREMENT_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_MSG_OK;
	nfapi_pnf_measurement_resp(config, &resp);
	return 0;
}
int rssi_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_rssi_request_t* req)
{
	nfapi_rssi_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_RSSI_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_P4_MSG_OK;
	nfapi_pnf_rssi_resp(config, &resp);
	
	nfapi_rssi_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_RSSI_INDICATION;
	ind.header.phy_id = req->header.phy_id;
	ind.error_code = NFAPI_P4_MSG_OK;
	ind.rssi_indication_body.tl.tag = NFAPI_RSSI_INDICATION_TAG;
	ind.rssi_indication_body.number_of_rssi = 1;
	ind.rssi_indication_body.rssi[0] = -42;
	nfapi_pnf_rssi_ind(config, &ind);
	return 0;
}
int cell_search_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_cell_search_request_t* req)
{
	nfapi_cell_search_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_CELL_SEARCH_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_P4_MSG_OK;
	nfapi_pnf_cell_search_resp(config, &resp);
	
	nfapi_cell_search_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_CELL_SEARCH_INDICATION;
	ind.header.phy_id = req->header.phy_id;
	ind.error_code = NFAPI_P4_MSG_OK;
	
	switch(req->rat_type)
	{
		case NFAPI_RAT_TYPE_LTE:
		{
			ind.lte_cell_search_indication.tl.tag = NFAPI_LTE_CELL_SEARCH_INDICATION_TAG;
			ind.lte_cell_search_indication.number_of_lte_cells_found = 1;
			ind.lte_cell_search_indication.lte_found_cells[0].pci = 123;
			ind.lte_cell_search_indication.lte_found_cells[0].rsrp = 123;
			ind.lte_cell_search_indication.lte_found_cells[0].rsrq = 123;
			ind.lte_cell_search_indication.lte_found_cells[0].frequency_offset = 123;
		}
		break;
		case NFAPI_RAT_TYPE_UTRAN:
		{
			ind.utran_cell_search_indication.tl.tag = NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG;
			ind.utran_cell_search_indication.number_of_utran_cells_found = 1;
			ind.utran_cell_search_indication.utran_found_cells[0].psc = 89;
			ind.utran_cell_search_indication.utran_found_cells[0].rscp = 89;
			ind.utran_cell_search_indication.utran_found_cells[0].ecno = 89;
			ind.utran_cell_search_indication.utran_found_cells[0].frequency_offset = -89;
			
		}
		break;
		case NFAPI_RAT_TYPE_GERAN:
		{
			ind.geran_cell_search_indication.tl.tag = NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG;
			ind.geran_cell_search_indication.number_of_gsm_cells_found = 1;
			ind.geran_cell_search_indication.gsm_found_cells[0].bsic = 23;
			ind.geran_cell_search_indication.gsm_found_cells[0].rxlev = 23;
			ind.geran_cell_search_indication.gsm_found_cells[0].rxqual = 23;
			ind.geran_cell_search_indication.gsm_found_cells[0].frequency_offset = -23;
			ind.geran_cell_search_indication.gsm_found_cells[0].sfn_offset = 230;
			
		}
		break;
	}
	
	ind.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
	ind.pnf_cell_search_state.length = 3;
	
	nfapi_pnf_cell_search_ind(config, &ind);	
	return 0;
}
int broadcast_detect_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_broadcast_detect_request_t* req)
{
	nfapi_broadcast_detect_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_BROADCAST_DETECT_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_P4_MSG_OK;
	nfapi_pnf_broadcast_detect_resp(config, &resp);
	
	nfapi_broadcast_detect_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_BROADCAST_DETECT_INDICATION;
	ind.header.phy_id = req->header.phy_id;
	ind.error_code = NFAPI_P4_MSG_OK;
	
	switch(req->rat_type)
	{
		case NFAPI_RAT_TYPE_LTE:
		{
			ind.lte_broadcast_detect_indication.tl.tag = NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG;
			ind.lte_broadcast_detect_indication.number_of_tx_antenna = 1;
			ind.lte_broadcast_detect_indication.mib_length = 4;
			//ind.lte_broadcast_detect_indication.mib...
			ind.lte_broadcast_detect_indication.sfn_offset = 77;
		
		}
		break;
		case NFAPI_RAT_TYPE_UTRAN:
		{
			ind.utran_broadcast_detect_indication.tl.tag = NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG;
			ind.utran_broadcast_detect_indication.mib_length = 4;
			//ind.utran_broadcast_detect_indication.mib...
			ind.utran_broadcast_detect_indication.sfn_offset;
			
		}
		break;
	}
	
	ind.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
	ind.pnf_cell_broadcast_state.length = 3;
	
	nfapi_pnf_broadcast_detect_ind(config, &ind);	
	return 0;
}
int system_information_schedule_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_system_information_schedule_request_t* req)
{
	nfapi_system_information_schedule_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_P4_MSG_OK;
	nfapi_pnf_system_information_schedule_resp(config, &resp);
	
	nfapi_system_information_schedule_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION;
	ind.header.phy_id = req->header.phy_id;
	ind.error_code = NFAPI_P4_MSG_OK;
	
	ind.lte_system_information_indication.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG;
	ind.lte_system_information_indication.sib_type = 3;
	ind.lte_system_information_indication.sib_length = 5;
	//ind.lte_system_information_indication.sib...
	
	nfapi_pnf_system_information_schedule_ind(config, &ind);		
	return 0;
}
int system_information_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_system_information_request_t* req)
{
	nfapi_system_information_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_SYSTEM_INFORMATION_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_P4_MSG_OK;
	nfapi_pnf_system_information_resp(config, &resp);
	
	nfapi_system_information_indication_t ind;
	memset(&ind, 0, sizeof(ind));
	ind.header.message_id = NFAPI_SYSTEM_INFORMATION_INDICATION;
	ind.header.phy_id = req->header.phy_id;
	ind.error_code = NFAPI_P4_MSG_OK;
	
	switch(req->rat_type)
	{
		case NFAPI_RAT_TYPE_LTE:
		{
			ind.lte_system_information_indication.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG;
			ind.lte_system_information_indication.sib_type = 1;
			ind.lte_system_information_indication.sib_length = 3;
			//ind.lte_system_information_indication.sib...
		}
		break;
		case NFAPI_RAT_TYPE_UTRAN:
		{
			ind.utran_system_information_indication.tl.tag = NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG;
			ind.utran_system_information_indication.sib_length = 3;
			//ind.utran_system_information_indication.sib...
			
		}
		break;
		case NFAPI_RAT_TYPE_GERAN:
		{
			ind.geran_system_information_indication.tl.tag = NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG;
			ind.geran_system_information_indication.si_length = 3;
			//ind.geran_system_information_indication.si...
			
		}
		break;
	}
		
	
	nfapi_pnf_system_information_ind(config, &ind);		

	return 0;
}
int nmm_stop_request(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy, nfapi_nmm_stop_request_t* req)
{
	nfapi_nmm_stop_response_t resp;
	memset(&resp, 0, sizeof(resp));
	resp.header.message_id = NFAPI_NMM_STOP_RESPONSE;
	resp.header.phy_id = req->header.phy_id;
	resp.error_code = NFAPI_P4_MSG_OK;
	nfapi_pnf_nmm_stop_resp(config, &resp);
	return 0;
}
	
int vendor_ext(nfapi_pnf_config_t* config, nfapi_p4_p5_message_header_t* msg)
{
	NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF_SIM] P5 %s %p\n", __FUNCTION__, msg);

	switch(msg->message_id)
	{
		case P5_VENDOR_EXT_REQ:
			{
				vendor_ext_p5_req* req = (vendor_ext_p5_req*)msg;
				NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF_SIM] P5 Vendor Ext Req (%d %d)\n", req->dummy1, req->dummy2);
				// send back the P5_VENDOR_EXT_RSP
				vendor_ext_p5_rsp rsp;
				memset(&rsp, 0, sizeof(rsp));
				rsp.header.message_id = P5_VENDOR_EXT_RSP;
				rsp.error_code = NFAPI_MSG_OK;
				nfapi_pnf_vendor_extension(config, &rsp.header, sizeof(vendor_ext_p5_rsp));
			}
			break;
	}
	
	return 0;
}

nfapi_p4_p5_message_header_t* pnf_sim_allocate_p4_p5_vendor_ext(uint16_t message_id, uint16_t* msg_size)
{
	if(message_id == P5_VENDOR_EXT_REQ)
	{
		(*msg_size) = sizeof(vendor_ext_p5_req);
		return (nfapi_p4_p5_message_header_t*)malloc(sizeof(vendor_ext_p5_req));
	}
	
	return 0;
}

void pnf_sim_deallocate_p4_p5_vendor_ext(nfapi_p4_p5_message_header_t* header)
{
	free(header);
}


int pnf_sim_pack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t* header, uint8_t** ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* config)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P5_VENDOR_EXT_RSP)
	{
		vendor_ext_p5_rsp* rsp = (vendor_ext_p5_rsp*)(header);
		return (!push16(rsp->error_code, ppWritePackedMsg, end));
	}
	return 0;
}

int pnf_sim_unpack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t* header, uint8_t** ppReadPackedMessage, uint8_t *end, nfapi_p4_p5_codec_config_t* codec)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	if(header->message_id == P5_VENDOR_EXT_REQ)
	{
		vendor_ext_p5_req* req = (vendor_ext_p5_req*)(header);
		return (!(pull16(ppReadPackedMessage, &req->dummy1, end) &&
	  			  pull16(ppReadPackedMessage, &req->dummy2, end)));
		 
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s (%d %d)\n", __FUNCTION__, req->dummy1, req->dummy2);
	}
	return 0;
}
int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Use parameters: <IP Address of VNF P5> <P5 Port> <Config File>\n");
		return 0;
	}

	set_thread_priority(50);

	pnf_info pnf;
	if(read_pnf_xml(pnf, argv[3]) < 0)
	{
		printf("Failed to read xml file>\n");
		return 0;
	}


	nfapi_pnf_config_t* config = nfapi_pnf_config_create();

	config->vnf_ip_addr = argv[1];
	config->vnf_p5_port = atoi(argv[2]);

	config->pnf_param_req = &pnf_param_request;
	config->pnf_config_req = &pnf_config_request;
	config->pnf_start_req = &pnf_start_request;
	config->pnf_stop_req = &pnf_stop_request;
	config->param_req = &param_request;
	config->config_req = &config_request;
	config->start_req = &start_request;
	
	config->measurement_req = &measurement_request;
	config->rssi_req = &rssi_request;
	config->cell_search_req = &cell_search_request;
	config->broadcast_detect_req = &broadcast_detect_request;
	config->system_information_schedule_req = &system_information_schedule_request;
	config->system_information_req = &system_information_request;
	config->nmm_stop_req = &nmm_stop_request;
	
	config->vendor_ext = &vendor_ext;

	config->trace = &pnf_sim_trace;

	config->user_data = &pnf;

	// To allow custom vendor extentions to be added to nfapi
	config->codec_config.unpack_vendor_extension_tlv = &pnf_sim_unpack_vendor_extension_tlv;
	config->codec_config.pack_vendor_extension_tlv = &pnf_sim_pack_vendor_extention_tlv;
	
	config->allocate_p4_p5_vendor_ext = &pnf_sim_allocate_p4_p5_vendor_ext;
	config->deallocate_p4_p5_vendor_ext = &pnf_sim_deallocate_p4_p5_vendor_ext;

	config->codec_config.unpack_p4_p5_vendor_extension = &pnf_sim_unpack_p4_p5_vendor_extension;
	config->codec_config.pack_p4_p5_vendor_extension = &pnf_sim_pack_p4_p5_vendor_extension;

	return nfapi_pnf_start(config);
	
}
