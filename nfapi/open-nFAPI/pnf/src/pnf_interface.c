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


#include "pnf.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"

nfapi_pnf_config_t* nfapi_pnf_config_create()
{
	pnf_t* _this = (pnf_t*)malloc(sizeof(pnf_t));

	if(_this == 0)
		return 0;

	memset(_this, 0, sizeof(pnf_t));

	_this->sctp = 1;	// enable sctp
	
	_this->_public.vnf_p5_port = NFAPI_P5_SCTP_PORT;
	
	_this->_public.malloc = &malloc;
	_this->_public.free = &free;	

	_this->_public.codec_config.allocate = &malloc;
	_this->_public.codec_config.deallocate = &free;

	return (nfapi_pnf_config_t* )_this;
}

void nfapi_pnf_config_destory(nfapi_pnf_config_t* config)
{
	free(config);
}

int nfapi_pnf_start(nfapi_pnf_config_t* config)
{
	// Verify that config is not null
	if(config == 0)
		return -1;

	// Make sure to set the defined trace function before using NFAPI_TRACE
	if(config->trace)
		nfapi_trace_g = config->trace;

	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);

	pnf_t* _this = (pnf_t*)(config);

	while (_this->terminate == 0)
	{
		int connect_result = pnf_connect(_this);

		if(connect_result > 0)
		{
			pnf_message_pump(_this);
		}
		else if(connect_result < 0)
		{
			return connect_result;
		}

		sleep(1);
	}
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() terminate=1 - EXITTING............\n", __FUNCTION__);

	return 0;
}

int nfapi_nr_pnf_start(nfapi_pnf_config_t* config)
{
	// Verify that config is not null
	if(config == 0)
		return -1;

	// Make sure to set the defined trace function before using NFAPI_TRACE
	if(config->trace)
		nfapi_trace_g = config->trace;

	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);

	pnf_t* _this = (pnf_t*)(config);

	while (_this->terminate == 0)
	{
		int connect_result = pnf_connect(_this);

		if(connect_result > 0)
		{
			pnf_nr_message_pump(_this);
		}
		else if(connect_result < 0)
		{
			return connect_result;
		}

		sleep(1);
	}
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() terminate=1 - EXITTING............\n", __FUNCTION__);

	return 0;
}

int nfapi_pnf_stop(nfapi_pnf_config_t* config)
{
	// Verify that config is not null
	if(config == 0)
		return -1;


	pnf_t* _this = (pnf_t*)(config);
	_this->terminate = 1;

	// todo wait for the pnf to stop before returning

	return 0;
}


int nfapi_nr_pnf_pnf_param_resp(nfapi_pnf_config_t* config, nfapi_nr_pnf_param_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_nr_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_nr_pnf_param_response_t));
}

int nfapi_pnf_pnf_param_resp(nfapi_pnf_config_t* config, nfapi_pnf_param_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_pnf_param_response_t));
}

int nfapi_pnf_pnf_config_resp(nfapi_pnf_config_t* config, nfapi_pnf_config_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	if(resp->error_code == NFAPI_MSG_OK)
	{
		config->state = NFAPI_PNF_CONFIGURED;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_pnf_config_response_t));
}


int nfapi_nr_pnf_pnf_config_resp(nfapi_pnf_config_t* config, nfapi_nr_pnf_config_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	if(resp->error_code == NFAPI_MSG_OK)
	{
		config->state = NFAPI_PNF_CONFIGURED;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_nr_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_nr_pnf_config_response_t));
}


int nfapi_pnf_pnf_start_resp(nfapi_pnf_config_t* config, nfapi_pnf_start_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	if(resp->error_code == NFAPI_MSG_OK)
	{
		config->state = NFAPI_PNF_RUNNING;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_pnf_start_response_t));
}

int nfapi_nr_pnf_pnf_start_resp(nfapi_pnf_config_t* config, nfapi_nr_pnf_start_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	if(resp->error_code == NFAPI_MSG_OK)
	{
		config->state = NFAPI_PNF_RUNNING;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_nr_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_nr_pnf_start_response_t));
}


int nfapi_pnf_pnf_stop_resp(nfapi_pnf_config_t* config, nfapi_pnf_stop_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	if(resp->error_code == NFAPI_MSG_OK)
	{
		config->state = NFAPI_PNF_CONFIGURED;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_pnf_stop_response_t));
}
int nfapi_pnf_param_resp(nfapi_pnf_config_t* config, nfapi_param_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}
	
	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_param_response_t));
}


int nfapi_nr_pnf_param_resp(nfapi_pnf_config_t* config, nfapi_nr_param_response_scf_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}
	
	pnf_t* _this = (pnf_t*)(config);

	return pnf_nr_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_nr_param_response_scf_t));
}

int nfapi_pnf_config_resp(nfapi_pnf_config_t* config, nfapi_config_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, resp->header.phy_id);

	if(phy)
	{
		if(resp->error_code == NFAPI_MSG_OK)
		{
			phy->state = NFAPI_PNF_PHY_CONFIGURED;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: unknow phy id %d\n", __FUNCTION__, resp->header.phy_id);
		return -1;
	}

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_config_response_t));
}

int nfapi_nr_pnf_config_resp(nfapi_pnf_config_t* config, nfapi_nr_config_response_scf_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, resp->header.phy_id);

	if(phy)
	{
		if(resp->error_code == NFAPI_NR_CONFIG_MSG_OK)
		{
			phy->state = NFAPI_PNF_PHY_CONFIGURED;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: unknow phy id %d\n", __FUNCTION__, resp->header.phy_id);
		return -1;
	}

	return pnf_nr_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_nr_config_response_scf_t));
}


int nfapi_pnf_start_resp(nfapi_pnf_config_t* config, nfapi_start_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, resp->header.phy_id);
	if(phy)
	{
		if(resp->error_code == NFAPI_MSG_OK)
		{
			phy->state = NFAPI_PNF_PHY_RUNNING;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: unknown phy id %d\n", __FUNCTION__, resp->header.phy_id);
		return -1;
	}

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_start_response_t));
}

int nfapi_nr_pnf_start_resp(nfapi_pnf_config_t* config, nfapi_nr_start_response_scf_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, resp->header.phy_id);
	if(phy)
	{
		if(resp->error_code == NFAPI_NR_START_MSG_OK)
		{
			phy->state = NFAPI_PNF_PHY_RUNNING;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: unknown phy id %d\n", __FUNCTION__, resp->header.phy_id);
		return -1;
	}

	return pnf_nr_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_nr_start_response_scf_t));
}


int nfapi_pnf_stop_resp(nfapi_pnf_config_t* config, nfapi_stop_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, resp->header.phy_id);
	if(phy)
	{
		if(resp->error_code == NFAPI_MSG_OK)
		{
			phy->state = NFAPI_PNF_PHY_CONFIGURED;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: unknow phy id %d\n", __FUNCTION__, resp->header.phy_id);
		return -1;
	}


	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_stop_response_t));
}

int nfapi_pnf_measurement_resp(nfapi_pnf_config_t* config, nfapi_measurement_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, &(resp->header), sizeof(nfapi_measurement_response_t));
}


int nfapi_pnf_rssi_resp(nfapi_pnf_config_t* config, nfapi_rssi_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(resp->header), sizeof(nfapi_rssi_response_t));
}

int nfapi_pnf_rssi_ind(nfapi_pnf_config_t* config, nfapi_rssi_indication_t* ind)
{
	if (config == NULL || ind == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(ind->header), sizeof(nfapi_rssi_indication_t));
}
int nfapi_pnf_cell_search_resp(nfapi_pnf_config_t* config, nfapi_cell_search_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Send CELL_SEARCH.response\n");

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(resp->header), sizeof(nfapi_cell_search_response_t));
}
int nfapi_pnf_cell_search_ind(nfapi_pnf_config_t* config, nfapi_cell_search_indication_t* ind)
{
	if (config == NULL || ind == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(ind->header), sizeof(nfapi_cell_search_indication_t));
}
int nfapi_pnf_broadcast_detect_resp(nfapi_pnf_config_t* config, nfapi_broadcast_detect_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(resp->header), sizeof(nfapi_broadcast_detect_response_t));
}
int nfapi_pnf_broadcast_detect_ind(nfapi_pnf_config_t* config, nfapi_broadcast_detect_indication_t* ind)
{
	if (config == NULL || ind == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(ind->header), sizeof(nfapi_broadcast_detect_indication_t));
}
int nfapi_pnf_system_information_schedule_resp(nfapi_pnf_config_t* config, nfapi_system_information_schedule_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(resp->header), sizeof(nfapi_system_information_schedule_response_t));
}

int nfapi_pnf_system_information_schedule_ind(nfapi_pnf_config_t* config, nfapi_system_information_schedule_indication_t* ind)
{
	if (config == NULL || ind == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(ind->header), sizeof(nfapi_system_information_schedule_indication_t));
}
int nfapi_pnf_system_information_resp(nfapi_pnf_config_t* config, nfapi_system_information_response_t* resp)
{
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(resp->header), sizeof(nfapi_system_information_response_t));
}

int nfapi_pnf_system_information_ind(nfapi_pnf_config_t* config, nfapi_system_information_indication_t* ind)
{
	if (config == NULL || ind == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(ind->header), sizeof(nfapi_system_information_indication_t));
}
int nfapi_pnf_nmm_stop_resp(nfapi_pnf_config_t* config, nfapi_nmm_stop_response_t* resp)
{
	// ensure it's valid
	if (config == NULL || resp == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p4_message(_this, &(resp->header), sizeof(nfapi_nmm_stop_request_t));
}

int nfapi_pnf_vendor_extension(nfapi_pnf_config_t* config, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len)
{
	// ensure it's valid
	if (config == NULL || msg == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return -1;
	}

	pnf_t* _this = (pnf_t*)(config);

	return pnf_pack_and_send_p5_message(_this, msg, msg_len);
}

