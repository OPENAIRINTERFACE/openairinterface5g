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


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "pnf.h"

# if 1 // for hard-code (remove later)
#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "common/ran_context.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "SCHED_NR/phy_frame_config_nr.h"

#include "NR_MIB.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac_common.h"

#endif

#define MAX_SCTP_STREAMS 16

void nfapi_pnf_phy_config_delete_all(nfapi_pnf_config_t* config)
{
	nfapi_pnf_phy_config_t* curr = config->phys;
	while(curr != 0)
	{
		nfapi_pnf_phy_config_t* to_delete = curr;
		curr = curr->next;
		free(to_delete);
	}

	config->phys = 0;
}

void nfapi_pnf_phy_config_add(nfapi_pnf_config_t* config, nfapi_pnf_phy_config_t* phy)
{
	phy->next = config->phys;
	config->phys = phy;
}

nfapi_pnf_phy_config_t* nfapi_pnf_phy_config_find(nfapi_pnf_config_t* config, uint16_t phy_id)
{
	nfapi_pnf_phy_config_t* curr = config->phys;
	while(curr != 0)
	{
		if(curr->phy_id == phy_id)
			return curr;

		curr = curr->next;
	}
	return 0;
}

void pnf_nr_handle_pnf_param_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nr_pnf_param_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_PARAM.request received\n");

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(nfapi_nr_pnf_param_request_t), &pnf->_public.codec_config) >= 0)
		{
			if(pnf->_public.state == NFAPI_PNF_IDLE)
			{
				if(pnf->_public.pnf_nr_param_req)
				{
					(pnf->_public.pnf_nr_param_req)(&pnf->_public, &req);
				}
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: PNF not in IDLE state\n", __FUNCTION__);

				nfapi_nr_pnf_param_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_nr_pnf_pnf_param_resp(&pnf->_public, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_pnf_param_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_pnf_param_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_PARAM.request received\n");

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(nfapi_pnf_param_request_t), &pnf->_public.codec_config) >= 0)
		{
			if(pnf->_public.state == NFAPI_PNF_IDLE)
			{
				if(pnf->_public.pnf_param_req)
				{
					(pnf->_public.pnf_param_req)(&pnf->_public, &req);
				}
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: PNF not in IDLE state\n", __FUNCTION__);

				nfapi_pnf_param_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_pnf_param_resp(&pnf->_public, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_pnf_config_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{


	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_pnf_config_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_CONFIG.request received\n");

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &pnf->_public.codec_config) >= 0)
		{
			// ensure correct state
			if(pnf->_public.state != NFAPI_PNF_RUNNING)
			{
				// delete the phy records
				nfapi_pnf_phy_config_delete_all(&pnf->_public);

				// create the phy records
				if (req.pnf_phy_rf_config.tl.tag == NFAPI_PNF_PHY_RF_TAG)
				{
					int i = 0;
					for(i = 0; i < req.pnf_phy_rf_config.number_phy_rf_config_info; ++i)
					{
						nfapi_pnf_phy_config_t* phy = (nfapi_pnf_phy_config_t*)malloc(sizeof(nfapi_pnf_phy_config_t));
						memset(phy, 0, sizeof(nfapi_pnf_phy_config_t));

						phy->state = NFAPI_PNF_PHY_IDLE;
						phy->phy_id = req.pnf_phy_rf_config.phy_rf_config[i].phy_id;

						nfapi_pnf_phy_config_add(&(pnf->_public), phy);
					}
				}

				if(pnf->_public.pnf_config_req)
				{
					(pnf->_public.pnf_config_req)(&(pnf->_public), &req);
				}
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: PNF not in correct state: %d\n", __FUNCTION__, pnf->_public.state);

				nfapi_pnf_config_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_pnf_config_resp(&(pnf->_public), &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_nr_handle_pnf_config_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{


	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nr_pnf_config_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_CONFIG.request received\n");

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &pnf->_public.codec_config) >= 0)
		{
			// ensure correct state
			if(pnf->_public.state != NFAPI_PNF_RUNNING)
			{
				// delete the phy records
				nfapi_pnf_phy_config_delete_all(&pnf->_public);

				// create the phy records
				if (req.pnf_phy_rf_config.tl.tag == NFAPI_PNF_PHY_RF_TAG)
				{
					int i = 0;
					for(i = 0; i < req.pnf_phy_rf_config.number_phy_rf_config_info; ++i)
					{
						nfapi_pnf_phy_config_t* phy = (nfapi_pnf_phy_config_t*)malloc(sizeof(nfapi_pnf_phy_config_t));
						memset(phy, 0, sizeof(nfapi_pnf_phy_config_t));

						phy->state = NFAPI_PNF_PHY_IDLE;
						phy->phy_id = req.pnf_phy_rf_config.phy_rf_config[i].phy_id;

						nfapi_pnf_phy_config_add(&(pnf->_public), phy);
					}
				}

				if(pnf->_public.pnf_nr_config_req)
				{
					(pnf->_public.pnf_nr_config_req)(&(pnf->_public), &req);
				}
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: PNF not in correct state: %d\n", __FUNCTION__, pnf->_public.state);

				nfapi_nr_pnf_config_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_nr_pnf_pnf_config_resp(&(pnf->_public), &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_pnf_start_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_pnf_start_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_START.request Received\n");

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &pnf->_public.codec_config) >= 0)
		{
			if(pnf->_public.state == NFAPI_PNF_CONFIGURED)
			{
				if(pnf->_public.pnf_start_req)
				{
					(pnf->_public.pnf_start_req)(&(pnf->_public), &req);
				}
			}
			else
			{
				nfapi_pnf_start_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PNF_START_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_pnf_start_resp(&(pnf->_public), &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_nr_handle_pnf_start_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nr_pnf_start_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_START.request Received\n");

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &pnf->_public.codec_config) >= 0)
		{
			if(pnf->_public.state == NFAPI_PNF_CONFIGURED)
			{
				if(pnf->_public.pnf_nr_start_req)
				{
					(pnf->_public.pnf_nr_start_req)(&(pnf->_public), &req);
				}
			}
			else
			{
				nfapi_nr_pnf_start_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PNF_START_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_nr_pnf_pnf_start_resp(&(pnf->_public), &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_pnf_stop_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_pnf_stop_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF_STOP.request Received\n");

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &pnf->_public.codec_config) >= 0)
		{
			if(pnf->_public.state == NFAPI_PNF_RUNNING)
			{
				if(pnf->_public.pnf_stop_req)
				{
					(pnf->_public.pnf_stop_req)(&(pnf->_public), &req);
				}
			}
			else
			{
				nfapi_pnf_stop_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PNF_STOP_RESPONSE;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_pnf_stop_resp(&(pnf->_public), &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}


		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_param_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_param_request_t req;

		nfapi_pnf_config_t* config = &(pnf->_public);

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PARAM.request received\n");

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_IDLE)
					{
						if(config->param_req)
						{
							(config->param_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_param_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_PARAM_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_param_resp(config, &resp);
					}
				}
				else
				{
					nfapi_param_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_PARAM_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_param_resp(config, &resp);
				}

			}
			else
			{
				nfapi_param_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_PARAM_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_param_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_nr_handle_param_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nr_param_request_scf_t req;

		nfapi_pnf_config_t* config = &(pnf->_public);

		NFAPI_TRACE(NFAPI_TRACE_INFO, "PARAM.request received\n");

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_IDLE)
					{
						if(config->nr_param_req)
						{
							(config->nr_param_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_nr_param_response_scf_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_nr_pnf_param_resp(config, &resp);
					}
				}
				else
				{
					nfapi_nr_param_response_scf_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_nr_pnf_param_resp(config, &resp);
				}

			}
			else
			{
				nfapi_nr_param_response_scf_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_nr_pnf_param_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_handle_config_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{

	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_config_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "CONFIG.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->config_req)
						{
							(config->config_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_config_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_CONFIG_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_config_resp(config, &resp);
					}
				}
				else
				{
					nfapi_config_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_CONFIG_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_config_resp(config, &resp);
				}
			}
			else
			{
				nfapi_config_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_CONFIG_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_config_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}


void pnf_nr_handle_config_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{

	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nr_config_request_scf_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "CONFIG.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);

#if 0 // emulate set_config TLV reception (hard-code)
				int tdd_return = set_tdd_config_nr(&req, 1, 7, 6, 2, 4);
#endif

				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->nr_config_req)
						{
							(config->nr_config_req)(config, phy, &req);


						}
					}
					else
					{
						nfapi_nr_config_response_scf_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_CONFIG_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_nr_pnf_config_resp(config, &resp);
					}
				}
				else
				{
					nfapi_nr_config_response_scf_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_CONFIG_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_nr_pnf_config_resp(config, &resp);
				}
			}
			else
			{
				nfapi_nr_config_response_scf_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_CONFIG_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_nr_pnf_config_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_start_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_start_request_t req;

		nfapi_pnf_config_t* config = &(pnf->_public);

		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() START.request received state:%d\n", __FUNCTION__, config->state);

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->start_req)
						{
							(config->start_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_start_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_START_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_start_resp(config, &resp);
					}
				}
				else
				{
					nfapi_start_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_START_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_start_resp(config, &resp);
				}
			}
			else
			{
				nfapi_start_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_START_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_start_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_nr_handle_start_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		 nfapi_nr_start_request_scf_t req;

		nfapi_pnf_config_t* config = &(pnf->_public);

		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() START.request received state:%d\n", __FUNCTION__, config->state);

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->nr_start_req)
						{
							(config->nr_start_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_nr_start_response_scf_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_nr_pnf_start_resp(config, &resp);
					}
				}
				else
				{
					nfapi_nr_start_response_scf_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_nr_pnf_start_resp(config, &resp);
				}
			}
			else
			{
				nfapi_nr_start_response_scf_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_nr_pnf_start_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_handle_stop_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_stop_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "STOP.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->stop_req)
						{
							(config->stop_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_stop_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_STOP_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_stop_resp(config, &resp);
					}
				}
				else
				{
					nfapi_stop_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_STOP_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_stop_resp(config, &resp);
				}
			}
			else
			{
				nfapi_stop_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_STOP_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_stop_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_nr_handle_stop_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nr_stop_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "STOP.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->nr_stop_req)
						{
							(config->nr_stop_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_stop_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_stop_resp(config, &resp);
					}
				}
				else
				{
					nfapi_stop_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_stop_resp(config, &resp);
				}
			}
			else
			{
				nfapi_stop_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_stop_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_handle_measurement_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	// ensure it's valid
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_measurement_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "MEASUREMENT.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->measurement_req)
						{
							(config->measurement_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_measurement_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_MEASUREMENT_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_measurement_resp(config, &resp);
					}
				}
				else
				{
					nfapi_measurement_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_MEASUREMENT_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_measurement_resp(config, &resp);
				}
			}
			else
			{
				nfapi_measurement_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_MEASUREMENT_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_measurement_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_rssi_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{


	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_rssi_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "RSSI.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_RUNNING)
					{
						if(config->rssi_req)
						{
							(config->rssi_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_rssi_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_RSSI_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_rssi_resp(config, &resp);
					}
				}
				else
				{
					nfapi_rssi_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_RSSI_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_rssi_resp(config, &resp);
				}
			}
			else
			{
				nfapi_rssi_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_RSSI_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_rssi_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_handle_cell_search_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_cell_search_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "CELL_SEARCH.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_RUNNING)
					{
						if(config->cell_search_req)
						{
							(config->cell_search_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_cell_search_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_CELL_SEARCH_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_cell_search_resp(config, &resp);
					}
				}
				else
				{
					nfapi_cell_search_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_CELL_SEARCH_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_cell_search_resp(config, &resp);
				}
			}
			else
			{
				nfapi_cell_search_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_CELL_SEARCH_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_cell_search_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}

}

void pnf_handle_broadcast_detect_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{

	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_broadcast_detect_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "BROADCAST_DETECT.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_RUNNING)
					{
						if(config->broadcast_detect_req)
						{
							(config->broadcast_detect_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_broadcast_detect_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_BROADCAST_DETECT_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_broadcast_detect_resp(config, &resp);
					}
				}
				else
				{
					nfapi_broadcast_detect_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_BROADCAST_DETECT_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_broadcast_detect_resp(config, &resp);
				}
			}
			else
			{
				nfapi_broadcast_detect_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_BROADCAST_DETECT_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_broadcast_detect_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);

	}
}

void pnf_handle_system_information_schedule_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_system_information_schedule_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "SYSTEM_INFORMATION_SCHEDULE.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_RUNNING)
					{
						if(config->system_information_schedule_req)
						{
							(config->system_information_schedule_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_system_information_schedule_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_system_information_schedule_resp(config, &resp);
					}
				}
				else
				{
					nfapi_system_information_schedule_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_system_information_schedule_resp(config, &resp);
				}
			}
			else
			{
				nfapi_system_information_schedule_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_system_information_schedule_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}

void pnf_handle_system_information_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_system_information_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "SYSTEM_INFORMATION.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state == NFAPI_PNF_PHY_RUNNING)
					{
						if(config->system_information_req)
						{
							(config->system_information_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_system_information_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_SYSTEM_INFORMATION_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_system_information_resp(config, &resp);
					}
				}
				else
				{
					nfapi_system_information_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_SYSTEM_INFORMATION_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_system_information_resp(config, &resp);
				}
			}
			else
			{
				nfapi_system_information_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_SYSTEM_INFORMATION_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_system_information_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}

}

void pnf_handle_nmm_stop_request(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nmm_stop_request_t req;

		NFAPI_TRACE(NFAPI_TRACE_INFO, "NMM_STOP.request received\n");

		nfapi_pnf_config_t* config = &(pnf->_public);

		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &req, sizeof(req), &config->codec_config) >= 0)
		{
			if(config->state == NFAPI_PNF_RUNNING)
			{
				nfapi_pnf_phy_config_t* phy = nfapi_pnf_phy_config_find(config, req.header.phy_id);
				if(phy)
				{
					if(phy->state != NFAPI_PNF_PHY_RUNNING)
					{
						if(config->nmm_stop_req)
						{
							(config->nmm_stop_req)(config, phy, &req);
						}
					}
					else
					{
						nfapi_nmm_stop_response_t resp;
						memset(&resp, 0, sizeof(resp));
						resp.header.message_id = NFAPI_NMM_STOP_RESPONSE;
						resp.header.phy_id = req.header.phy_id;
						resp.error_code = NFAPI_MSG_INVALID_STATE;
						nfapi_pnf_nmm_stop_resp(config, &resp);
					}
				}
				else
				{
					nfapi_nmm_stop_response_t resp;
					memset(&resp, 0, sizeof(resp));
					resp.header.message_id = NFAPI_NMM_STOP_RESPONSE;
					resp.header.phy_id = req.header.phy_id;
					resp.error_code = NFAPI_MSG_INVALID_CONFIG;
					nfapi_pnf_nmm_stop_resp(config, &resp);
				}
			}
			else
			{
				nfapi_nmm_stop_response_t resp;
				memset(&resp, 0, sizeof(resp));
				resp.header.message_id = NFAPI_NMM_STOP_RESPONSE;
				resp.header.phy_id = req.header.phy_id;
				resp.error_code = NFAPI_MSG_INVALID_STATE;
				nfapi_pnf_nmm_stop_resp(config, &resp);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		if(req.vendor_extension)
			pnf->_public.codec_config.deallocate(req.vendor_extension);
	}
}


void pnf_handle_vendor_extension(void* pRecvMsg, int recvMsgLen, pnf_t* pnf, uint16_t message_id)
{
	if (pRecvMsg == NULL || pnf == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_pnf_config_t* config = &(pnf->_public);

		if(config->allocate_p4_p5_vendor_ext)
		{
			uint16_t msg_size;
			nfapi_p4_p5_message_header_t* msg = config->allocate_p4_p5_vendor_ext(message_id, &msg_size);

			if(msg == 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to allocate vendor extention structure\n");
				return;
			}


			int unpack_result = nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, msg, msg_size, &config->codec_config);

			if(unpack_result == 0)
			{
				if(config->vendor_ext)
					config->vendor_ext(config, msg);
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
			}

			if(config->deallocate_p4_p5_vendor_ext)
				config->deallocate_p4_p5_vendor_ext(msg);

		}
	}
}

void pnf_nr_handle_p5_message(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	nfapi_p4_p5_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < NFAPI_HEADER_LENGTH)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: invalid input params\n", __FUNCTION__);
		return;
	}

	// unpack the message header
	if (nfapi_p5_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p4_p5_message_header_t), &pnf->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	switch (messageHeader.message_id)
	{
		case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_REQUEST:
			pnf_nr_handle_pnf_param_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_REQUEST:
			pnf_nr_handle_pnf_config_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_PNF_START_REQUEST:
			pnf_nr_handle_pnf_start_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_PNF_STOP_REQUEST:
			pnf_handle_pnf_stop_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST:
			pnf_nr_handle_param_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST:
			pnf_nr_handle_config_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_START_REQUEST:
			pnf_nr_handle_start_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST:
			pnf_nr_handle_stop_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_MEASUREMENT_REQUEST:
			pnf_handle_measurement_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_RSSI_REQUEST:
			pnf_handle_rssi_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_CELL_SEARCH_REQUEST:
			pnf_handle_cell_search_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_BROADCAST_DETECT_REQUEST:
			pnf_handle_broadcast_detect_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST:
			pnf_handle_system_information_schedule_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_SYSTEM_INFORMATION_REQUEST:
			pnf_handle_system_information_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NMM_STOP_REQUEST:
			pnf_handle_nmm_stop_request(pnf, pRecvMsg, recvMsgLen);
			break;

		default:
			{
				if(messageHeader.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   messageHeader.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					pnf_handle_vendor_extension(pRecvMsg, recvMsgLen, pnf, messageHeader.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P5 Unknown message ID %d\n", __FUNCTION__, messageHeader.message_id);
				}
			}
			break;
	}
}

void pnf_handle_p5_message(pnf_t* pnf, void *pRecvMsg, int recvMsgLen)
{
	nfapi_p4_p5_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < NFAPI_HEADER_LENGTH)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: invalid input params\n", __FUNCTION__);
		return;
	}

	// unpack the message header
	if (nfapi_p5_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p4_p5_message_header_t), &pnf->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	switch (messageHeader.message_id)
	{
		case NFAPI_PNF_PARAM_REQUEST:
			pnf_handle_pnf_param_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_PNF_CONFIG_REQUEST:
			pnf_handle_pnf_config_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_PNF_START_REQUEST:
			pnf_handle_pnf_start_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_PNF_STOP_REQUEST:
			pnf_handle_pnf_stop_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_PARAM_REQUEST:
			pnf_handle_param_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_CONFIG_REQUEST:
			pnf_handle_config_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_START_REQUEST:
			pnf_handle_start_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_STOP_REQUEST:
			pnf_handle_stop_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_MEASUREMENT_REQUEST:
			pnf_handle_measurement_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_RSSI_REQUEST:
			pnf_handle_rssi_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_CELL_SEARCH_REQUEST:
			pnf_handle_cell_search_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_BROADCAST_DETECT_REQUEST:
			pnf_handle_broadcast_detect_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST:
			pnf_handle_system_information_schedule_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_SYSTEM_INFORMATION_REQUEST:
			pnf_handle_system_information_request(pnf, pRecvMsg, recvMsgLen);
			break;

		case NFAPI_NMM_STOP_REQUEST:
			pnf_handle_nmm_stop_request(pnf, pRecvMsg, recvMsgLen);
			break;

		default:
			{
				if(messageHeader.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   messageHeader.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					pnf_handle_vendor_extension(pRecvMsg, recvMsgLen, pnf, messageHeader.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P5 Unknown message ID %d\n", __FUNCTION__, messageHeader.message_id);
				}
			}
			break;
	}
}


int pnf_nr_pack_and_send_p5_message(pnf_t* pnf, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len)
{
	int packed_len = nfapi_nr_p5_message_pack(msg, msg_len,
										   pnf->tx_message_buffer,
										   sizeof(pnf->tx_message_buffer),
										   &pnf->_public.codec_config);

	if (packed_len < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p5_message_pack failed (%d)\n", packed_len);
		return -1;
	}

	return pnf_send_message(pnf, pnf->tx_message_buffer, packed_len, 0/*msg->stream_id*/);
}


int pnf_pack_and_send_p5_message(pnf_t* pnf, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len)
{
	int packed_len = nfapi_p5_message_pack(msg, msg_len,
										   pnf->tx_message_buffer,
										   sizeof(pnf->tx_message_buffer),
										   &pnf->_public.codec_config);

	if (packed_len < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p5_message_pack failed (%d)\n", packed_len);
		return -1;
	}

	return pnf_send_message(pnf, pnf->tx_message_buffer, packed_len, 0/*msg->stream_id*/);
}

int pnf_pack_and_send_p4_message(pnf_t* pnf, nfapi_p4_p5_message_header_t* msg, uint32_t msg_len)
{
	int packed_len = nfapi_p4_message_pack(msg, msg_len,
										   pnf->tx_message_buffer,
										   sizeof(pnf->tx_message_buffer),
										   &pnf->_public.codec_config);

	if (packed_len < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p4_message_pack failed (%d)\n", packed_len);
		return -1;
	}

	return pnf_send_message(pnf, pnf->tx_message_buffer, packed_len, 0/*msg->stream_id*/);
}



int pnf_connect(pnf_t* pnf)
{
	struct sockaddr_in servaddr;
	uint8_t socketConnected = 0;

	if(pnf->_public.vnf_ip_addr == 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnfIpAddress is null\n");
		return -1;
	}


	(void)memset(&servaddr, 0, sizeof(struct sockaddr_in));

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Starting P5 PNF connection to VNF at %s:%u\n", pnf->_public.vnf_ip_addr, pnf->_public.vnf_p5_port);

	// todo split the vnf address list. currently only supporting 1

	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM; // For SCTP we are only interested in SOCK_STREAM
	// todo : allow the client to restrict IPV4 or IPV6
		// todo : randomize which address to connect to?

	char port_str[8];
	snprintf(port_str, sizeof(port_str), "%d", pnf->_public.vnf_p5_port);
	if(getaddrinfo(pnf->_public.vnf_ip_addr, port_str,  &hints, &servinfo) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to get host (%s) addr info h_errno:%d \n", pnf->_public.vnf_ip_addr, h_errno);
		return -1;
	}

	struct addrinfo *p = servinfo;
	int connected = 0;

	while(p != NULL && connected == 0)
	{
#ifdef NFAPI_TRACE_ENABLED
		char* family = "Unknown";
		char* address = "Unknown";
		char _addr[128];

		if(p->ai_family == AF_INET6)
		{
			family = "IPV6";
			struct sockaddr_in6* addr = (struct sockaddr_in6*)p->ai_addr;
			inet_ntop(AF_INET6, &addr->sin6_addr, _addr, sizeof(_addr));
			address = &_addr[0];
		}
		else if(p->ai_family == AF_INET)
		{
			family = "IPV4";
			struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
			address = inet_ntoa(addr->sin_addr);
		}

		NFAPI_TRACE(NFAPI_TRACE_NOTE, "Host address info  %d Family:%s Address:%s\n", i++, family, address);
#endif

		if (pnf->sctp)
		{
			// open the SCTP socket
			if ((pnf->p5_sock = socket(p->ai_family, SOCK_STREAM, IPPROTO_SCTP)) < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P5 socket errno: %d\n", errno);
				freeaddrinfo(servinfo);
				return -1;
			}
			int noDelay;
			struct sctp_initmsg initMsg;

			(void)memset(&initMsg, 0, sizeof(struct sctp_initmsg));

			// configure the socket options
			NFAPI_TRACE(NFAPI_TRACE_NOTE, "PNF Setting the SCTP_INITMSG\n");
			initMsg.sinit_num_ostreams = 5; //MAX_SCTP_STREAMS;  // number of output streams can be greater
			initMsg.sinit_max_instreams = 5; //MAX_SCTP_STREAMS;  // number of output streams can be greater
			if (setsockopt(pnf->p5_sock, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg)) < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt errno: %d\n", errno);
				freeaddrinfo(servinfo);
				return -1;
			}
			noDelay = 1;
			if (setsockopt(pnf->p5_sock, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay)) < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt errno: %d\n", errno);
				freeaddrinfo(servinfo);
				return -1;

			}

			struct sctp_event_subscribe events;
			memset( (void *)&events, 0, sizeof(events) );
			events.sctp_data_io_event = 1;

			if(setsockopt(pnf->p5_sock, SOL_SCTP, SCTP_EVENTS, (const void *)&events, sizeof(events)) < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt errno: %d\n", errno);
				freeaddrinfo(servinfo);
				return -1;
			}
		}
		else
		{
			// Create an IP socket point
			if ((pnf->p5_sock = socket(p->ai_family, SOCK_STREAM, IPPROTO_IP)) < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P5 socket errno: %d\n", errno);
				freeaddrinfo(servinfo);
				return -1;
			}
		}

		NFAPI_TRACE(NFAPI_TRACE_INFO, "P5 socket created...\n");

		if (connect(pnf->p5_sock, p->ai_addr, p->ai_addrlen ) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After connect (address:%s port:%d) errno: %d\n",
					pnf->_public.vnf_ip_addr, pnf->_public.vnf_p5_port, errno);

			if(errno == EINVAL)
			{
				freeaddrinfo(servinfo);
				return -1;
			}
			else
			{
				if(pnf->terminate != 0)
				{
					freeaddrinfo(servinfo);
					return 0;
				}
				else
				{
					close(pnf->p5_sock);
					sleep(1);
				}
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "connect succeeded...\n");

			connected = 1;
		}

		p = p->ai_next;
	}

	freeaddrinfo(servinfo);

	// If we have failed to connect return 0 and it is retry
	if(connected == 0)
		return 0;


	NFAPI_TRACE(NFAPI_TRACE_NOTE, "After connect loop\n");
	if (pnf->sctp)
	{
		socklen_t optLen;
		struct sctp_status status;

		(void)memset(&status, 0, sizeof(struct sctp_status));

		// check the connection status
		optLen = (socklen_t) sizeof(struct sctp_status);
		if (getsockopt(pnf->p5_sock, IPPROTO_SCTP, SCTP_STATUS, &status, &optLen) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After getsockopt errno: %d\n", errno);
			return -1;
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "Association ID = %d\n", status.sstat_assoc_id);
			NFAPI_TRACE(NFAPI_TRACE_INFO, "Receiver window size = %d\n", status.sstat_rwnd);
			NFAPI_TRACE(NFAPI_TRACE_INFO, "In Streams = %d\n",  status.sstat_instrms);
			NFAPI_TRACE(NFAPI_TRACE_INFO, "Out Streams = %d\n", status.sstat_outstrms);

			socketConnected = 1;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_NOTE, "Socket %s\n", socketConnected ? "CONNECTED" : "NOT_CONNECTED");
	return socketConnected;
}

int pnf_send_message(pnf_t* pnf, uint8_t *msg, uint32_t len, uint16_t stream)
{

	if (pnf->sctp)
	{
#if 0
		printf("\nPNF SENDS: \n");
		for(int i=0; i<len; i++){
			printf("%d", msg[i]);
		}
		printf("\n");
#endif

		if (sctp_sendmsg(pnf->p5_sock, msg, len, NULL, 0, 42/*config->sctp_stream_number*/, 0, stream/*P5_STREAM_ID*/, 0, 0) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "sctp_sendmsg failed errno: %d\n", errno);
			return -1;
		}
	}
	else
	{
		if (write(pnf->p5_sock, msg, len) != len)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "read failed errno: %d\n", errno);
			return -1;
		}
	}
	return 0;
}

int pnf_read_dispatch_message(pnf_t* pnf)
{
	int socket_connected = 1;

	// 1. Peek the message header
	// 2. If the message is larger than the stack buffer then create a dynamic buffer
	// 3. Read the buffer
	// 4. Handle the p5 message

	uint32_t header_buffer_size = NFAPI_HEADER_LENGTH;
	uint8_t header_buffer[header_buffer_size];

	uint32_t stack_buffer_size = 32; //should it be the size of then sctp_notificatoin structure
	uint8_t stack_buffer[stack_buffer_size];

	uint8_t* dynamic_buffer = 0;

	uint8_t* read_buffer = &stack_buffer[0];
	uint32_t message_size = 0;

	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	struct sctp_sndrcvinfo sndrcvinfo;
	(void)memset(&sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));

	{
		int flags = MSG_PEEK;
		message_size = sctp_recvmsg(pnf->p5_sock, header_buffer, header_buffer_size, /*(struct sockaddr*)&addr, &addr_len*/ 0, 0, &sndrcvinfo,  &flags);

		if(message_size == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF Failed to peek sctp message size errno:%d\n", errno);
			return 0;
		}

		nfapi_p4_p5_message_header_t header;
		int unpack_result = nfapi_p5_message_header_unpack(header_buffer, header_buffer_size, &header, sizeof(header), 0);
		if(unpack_result < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF Failed to unpack p5 message header\n");
			return 0;
		}
		message_size = header.message_length;

		// now have the size of the mesage
	}

	if(message_size > stack_buffer_size)
	{
		dynamic_buffer = (uint8_t*)malloc(message_size);

		if(dynamic_buffer == NULL)
		{
			// todo : add error mesage
			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF Failed to allocate dynamic buffer for sctp_recvmsg size:%d\n", message_size);
			return -1;
		}

		read_buffer = dynamic_buffer;
	}

	{
		int flags = 0;
		(void)memset(&sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));

		int recvmsg_result = sctp_recvmsg(pnf->p5_sock, read_buffer, message_size, (struct sockaddr*)&addr, &addr_len, &sndrcvinfo, &flags);
		if(recvmsg_result == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "Failed to read sctp message size errno:%d\n", errno);
		}
		else
		{
#if 0
			// print the received message
			printf("\n MESSAGE RECEIVED: \n");
			for(int i=0; i<message_size; i++){
				printf("%d", read_buffer[i]);
			}
			printf("\n");
#endif

			if (flags & MSG_NOTIFICATION)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Notification received from %s:%u\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

				// todo - handle the events
			}
			else
			{
				/*
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Received message fd:%d from %s:%u assoc:%d on stream %d, PPID %d, length %d, flags 0x%x\n",
						pnf->p5_sock,
						inet_ntoa(addr.sin_addr),
						ntohs(addr.sin_port),
						sndrcvinfo.sinfo_assoc_id,
						sndrcvinfo.sinfo_stream,
						ntohl(sndrcvinfo.sinfo_ppid),
						message_size,
						flags);
				*/

				// handle now if complete message in one or more segments
				if ((flags & 0x80) == 0x80)
				{
					pnf_handle_p5_message(pnf, read_buffer, message_size);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_WARN, "sctp_recvmsg: unhandled mode with flags 0x%x\n", flags);

					// assume socket disconnected
					NFAPI_TRACE(NFAPI_TRACE_WARN, "Disconnected socket\n");
					socket_connected =  0;
				}


			}
		}
	}

	if(dynamic_buffer)
	{
		free(dynamic_buffer);
	}

	return socket_connected;


}

int pnf_nr_read_dispatch_message(pnf_t* pnf)
{
	int socket_connected = 1;

	// 1. Peek the message header
	// 2. If the message is larger than the stack buffer then create a dynamic buffer
	// 3. Read the buffer
	// 4. Handle the p5 message

	uint32_t header_buffer_size = NFAPI_HEADER_LENGTH;
	uint8_t header_buffer[header_buffer_size];

	uint32_t stack_buffer_size = 32; //should it be the size of then sctp_notificatoin structure
	uint8_t stack_buffer[stack_buffer_size];

	uint8_t* dynamic_buffer = 0;

	uint8_t* read_buffer = &stack_buffer[0];
	uint32_t message_size = 0;

	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	struct sctp_sndrcvinfo sndrcvinfo;
	(void)memset(&sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));

	{
		int flags = MSG_PEEK;
		message_size = sctp_recvmsg(pnf->p5_sock, header_buffer, header_buffer_size, /*(struct sockaddr*)&addr, &addr_len*/ 0, 0, &sndrcvinfo,  &flags);

		if(message_size == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF Failed to peek sctp message size errno:%d\n", errno);
			return 0;
		}

		nfapi_p4_p5_message_header_t header;
		int unpack_result = nfapi_p5_message_header_unpack(header_buffer, header_buffer_size, &header, sizeof(header), 0);
		if(unpack_result < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF Failed to unpack p5 message header\n");
			return 0;
		}
		message_size = header.message_length;

		// now have the size of the mesage
	}

	if(message_size > stack_buffer_size)
	{
		dynamic_buffer = (uint8_t*)malloc(message_size);

		if(dynamic_buffer == NULL)
		{
			// todo : add error mesage
			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF Failed to allocate dynamic buffer for sctp_recvmsg size:%d\n", message_size);
			return -1;
		}

		read_buffer = dynamic_buffer;
	}

	{
		int flags = 0;
		(void)memset(&sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));

		int recvmsg_result = sctp_recvmsg(pnf->p5_sock, read_buffer, message_size, (struct sockaddr*)&addr, &addr_len, &sndrcvinfo, &flags);
		if(recvmsg_result == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "Failed to read sctp message size errno:%d\n", errno);
		}
		else
		{
#if 0
			// print the received message
			printf("\n MESSAGE RECEIVED: \n");
			for(int i=0; i<message_size; i++){
				printf("%d", read_buffer[i]);
			}
			printf("\n");
#endif

			if (flags & MSG_NOTIFICATION)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Notification received from %s:%u\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

				// todo - handle the events
			}
			else
			{
				/*
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Received message fd:%d from %s:%u assoc:%d on stream %d, PPID %d, length %d, flags 0x%x\n",
						pnf->p5_sock,
						inet_ntoa(addr.sin_addr),
						ntohs(addr.sin_port),
						sndrcvinfo.sinfo_assoc_id,
						sndrcvinfo.sinfo_stream,
						ntohl(sndrcvinfo.sinfo_ppid),
						message_size,
						flags);
				*/

				// handle now if complete message in one or more segments
				if ((flags & 0x80) == 0x80)
				{
					pnf_nr_handle_p5_message(pnf, read_buffer, message_size);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_WARN, "sctp_recvmsg: unhandled mode with flags 0x%x\n", flags);

					// assume socket disconnected
					NFAPI_TRACE(NFAPI_TRACE_WARN, "Disconnected socket\n");
					socket_connected =  0;
				}


			}
		}
	}

	if(dynamic_buffer)
	{
		free(dynamic_buffer);
	}

	return socket_connected;


}


int pnf_message_pump(pnf_t* pnf)
{
	uint8_t socketConnected = 1;

	while(socketConnected && pnf->terminate == 0)
	{
		fd_set rfds;
		int selectRetval = 0;

		// select on a timeout and then get the message
		FD_ZERO(&rfds);
		FD_SET(pnf->p5_sock, &rfds);

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		selectRetval = select(pnf->p5_sock+1, &rfds, NULL, NULL, &timeout);

		if(selectRetval == 0)
		{
			// timeout
			continue;
		}
		else if (selectRetval == -1 && (errno == EINTR))
		{
			// interrupted by signal
			NFAPI_TRACE(NFAPI_TRACE_WARN, "P5 Signal Interrupt %d\n", errno);
			continue;
		}
		else if (selectRetval == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "P5 select() failed\n");
			sleep(1);
			continue;
		}

		if(FD_ISSET(pnf->p5_sock, &rfds))
		{
			socketConnected = pnf_read_dispatch_message(pnf);
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "Why are we here\n");
		}
	}

	// Drop back to idle if we have lost connection
	pnf->_public.state = NFAPI_PNF_PHY_IDLE;


	// close the connection and socket
	if (close(pnf->p5_sock) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "close(sctpSock) failed errno: %d\n", errno);
	}

	return 0;

}

int pnf_nr_message_pump(pnf_t* pnf)
{
	uint8_t socketConnected = 1;

	while(socketConnected && pnf->terminate == 0)
	{
		fd_set rfds;
		int selectRetval = 0;

		// select on a timeout and then get the message
		FD_ZERO(&rfds);
		FD_SET(pnf->p5_sock, &rfds);

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		selectRetval = select(pnf->p5_sock+1, &rfds, NULL, NULL, &timeout);

		if(selectRetval == 0)
		{
			// timeout
			continue;
		}
		else if (selectRetval == -1 && (errno == EINTR))
		{
			// interrupted by signal
			NFAPI_TRACE(NFAPI_TRACE_WARN, "P5 Signal Interrupt %d\n", errno);
			continue;
		}
		else if (selectRetval == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "P5 select() failed\n");
			sleep(1);
			continue;
		}

		if(FD_ISSET(pnf->p5_sock, &rfds))
		{
			socketConnected = pnf_nr_read_dispatch_message(pnf);
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "Why are we here\n");
		}
	}

	// Drop back to idle if we have lost connection
	pnf->_public.state = NFAPI_PNF_PHY_IDLE;


	// close the connection and socket
	if (close(pnf->p5_sock) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "close(sctpSock) failed errno: %d\n", errno);
	}

	return 0;

}
