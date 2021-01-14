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
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "vnf.h"
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"


void* vnf_malloc(nfapi_vnf_config_t* config, size_t size)
{
	if(config->malloc)
	{
		return (config->malloc)(size);
	}
	else
	{
		return calloc(1, size); 
	}
}
void vnf_free(nfapi_vnf_config_t* config, void* ptr)
{
	if(config->free)
	{
		return (config->free)(ptr);
	}
	else
	{
		return free(ptr); 
	}
}

void nfapi_vnf_phy_info_list_add(nfapi_vnf_config_t* config, nfapi_vnf_phy_info_t* info)
{
	NFAPI_TRACE(NFAPI_TRACE_INFO, "Adding phy p5_idx:%d phy_id:%d\n", info->p5_idx, info->phy_id);
	info->next = config->phy_list;
	config->phy_list = info;
}

nfapi_vnf_phy_info_t* nfapi_vnf_phy_info_list_find(nfapi_vnf_config_t* config, uint16_t phy_id)
{
	nfapi_vnf_phy_info_t* curr = config->phy_list;
	while(curr != 0)
	{
		if(curr->phy_id == phy_id)
			return curr;

		curr = curr->next;
	}

	return 0;
}




void nfapi_vnf_pnf_list_add(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* node)
{
	node->next = config->pnf_list;
	config->pnf_list = node;
}


nfapi_vnf_pnf_info_t* nfapi_vnf_pnf_list_find(nfapi_vnf_config_t* config, int p5_idx)
{
	NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : config->pnf_list:%p\n", __FUNCTION__, config->pnf_list);

	nfapi_vnf_pnf_info_t* curr = config->pnf_list;
	while(curr != 0)
	{
		if(curr->p5_idx == p5_idx)
                {
                  NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : curr->p5_idx:%d p5_idx:%d\n", __FUNCTION__, curr->p5_idx, p5_idx);
			return curr;
                        }

                NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : curr->next:%p\n", __FUNCTION__, curr->next);

		curr = curr->next;
	}

	return 0;
}

void vnf_nr_handle_pnf_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_PARAM.reponse\n");

		nfapi_nr_pnf_param_response_t msg;
			
		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			// Invoke the call back
			if(config->pnf_nr_param_resp)
			{
				(config->pnf_nr_param_resp)(config, p5_idx, &msg);
			}
		}	
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_pnf_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_PARAM.reponse\n");

		nfapi_pnf_param_response_t msg;
			
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			// Invoke the call back
			if(config->pnf_param_resp)
			{
				(config->pnf_param_resp)(config, p5_idx, &msg);
			}
		}	
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}


void vnf_nr_handle_pnf_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_CONFIG_RESPONSE\n");
		
		nfapi_nr_pnf_config_response_t msg;
		
		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			// Invoke the call back
			if(config->pnf_nr_config_resp)
			{
				(config->pnf_nr_config_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_pnf_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_CONFIG_RESPONSE\n");
		
		nfapi_pnf_config_response_t msg;
		
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			// Invoke the call back
			if(config->pnf_config_resp)
			{
				(config->pnf_config_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_nr_handle_pnf_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_START_RESPONSE\n");
	
		nfapi_nr_pnf_start_response_t msg;
	
		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->pnf_nr_start_resp)
			{
				(config->pnf_nr_start_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_pnf_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_START_RESPONSE\n");
	
		nfapi_pnf_start_response_t msg;
	
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->pnf_start_resp)
			{
				(config->pnf_start_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_pnf_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_STOP_RESPONSE\n");
	
		nfapi_pnf_stop_response_t msg;

		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->pnf_stop_resp)
			{
				(config->pnf_stop_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);

	}
}

void vnf_handle_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{

	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PARAM_RESPONSE\n");
		
		nfapi_param_response_t msg;
		
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			
			if (msg.error_code == NFAPI_MSG_OK)
			{
				nfapi_vnf_phy_info_t* phy_info = nfapi_vnf_phy_info_list_find(config, msg.header.phy_id);
		
				if(msg.nfapi_config.p7_pnf_address_ipv4.tl.tag)
				{
					struct sockaddr_in sockAddr;
		
					(void)memcpy(&sockAddr.sin_addr.s_addr, msg.nfapi_config.p7_pnf_address_ipv4.address, NFAPI_IPV4_ADDRESS_LENGTH);
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 IPv4 address: %s\n", inet_ntoa(sockAddr.sin_addr));
		
					// store address
					phy_info->p7_pnf_address.sin_addr = sockAddr.sin_addr;
				}
		
				if(msg.nfapi_config.p7_pnf_address_ipv6.tl.tag)
				{
					struct sockaddr_in6 sockAddr6;
					char addr6[64];
					(void)memcpy(&sockAddr6.sin6_addr, msg.nfapi_config.p7_pnf_address_ipv6.address, NFAPI_IPV6_ADDRESS_LENGTH);
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 IPv6 address: %s\n", inet_ntop(AF_INET6, &sockAddr6.sin6_addr, addr6, sizeof(addr6)));
				}
				
				if (msg.nfapi_config.p7_pnf_port.tl.tag)
				{
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 Port: %d\n", msg.nfapi_config.p7_pnf_port.value);
		
					// store port
					phy_info->p7_pnf_address.sin_port = htons(msg.nfapi_config.p7_pnf_port.value);
				}
			}
			
			if(config->param_resp)
			{
				(config->param_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_nr_handle_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{

	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PARAM_RESPONSE\n");
		
		nfapi_nr_param_response_scf_t msg;
		
		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			
			if (msg.error_code == NFAPI_NR_PARAM_MSG_OK)
			{
				nfapi_vnf_phy_info_t* phy_info = nfapi_vnf_phy_info_list_find(config, msg.header.phy_id);
		
				if(msg.nfapi_config.p7_pnf_address_ipv4.tl.tag)
				{
					struct sockaddr_in sockAddr;
		
					(void)memcpy(&sockAddr.sin_addr.s_addr, msg.nfapi_config.p7_pnf_address_ipv4.address, NFAPI_IPV4_ADDRESS_LENGTH);
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 IPv4 address: %s\n", inet_ntoa(sockAddr.sin_addr));
		
					// store address
					phy_info->p7_pnf_address.sin_addr = sockAddr.sin_addr;
				}
		
				if(msg.nfapi_config.p7_pnf_address_ipv6.tl.tag)
				{
					struct sockaddr_in6 sockAddr6;
					char addr6[64];
					(void)memcpy(&sockAddr6.sin6_addr, msg.nfapi_config.p7_pnf_address_ipv6.address, NFAPI_IPV6_ADDRESS_LENGTH);
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 IPv6 address: %s\n", inet_ntop(AF_INET6, &sockAddr6.sin6_addr, addr6, sizeof(addr6)));
				}
				
				if (msg.nfapi_config.p7_pnf_port.tl.tag)
				{
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 Port: %d\n", msg.nfapi_config.p7_pnf_port.value);
		
					// store port
					phy_info->p7_pnf_address.sin_port = htons(msg.nfapi_config.p7_pnf_port.value);
				}
			}
			
			if(config->nr_param_resp)
			{
				(config->nr_param_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}


void vnf_nr_handle_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{

	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CONFIG_RESPONSE\n");
			
		nfapi_nr_config_response_scf_t msg;
		
		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >=0 )
		{
			// check the error code:

			if (msg.error_code == NFAPI_NR_CONFIG_MSG_OK){
				if(config->nr_config_resp)
				{
					(config->nr_config_resp)(config, p5_idx, &msg);
				}
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
	
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{

	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CONFIG_RESPONSE\n");
			
		nfapi_config_response_t msg;
		
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >=0 )
		{
			// check the error code:

			if (msg.error_code == NFAPI_MSG_OK){
				if(config->config_resp)
				{
					(config->config_resp)(config, p5_idx, &msg);
				}
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
	
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{	
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received START_RESPONSE\n");

		nfapi_start_response_t msg;
		
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->start_resp)
			{
				(config->start_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_nr_handle_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{	
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received START_RESPONSE\n");

		nfapi_nr_start_response_scf_t msg;
		
		// unpack the message
		if (nfapi_nr_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{	// check the error code
			if (msg.error_code == NFAPI_NR_START_MSG_OK){
				if(config->nr_start_resp)
				{
					(config->nr_start_resp)(config, p5_idx, &msg);
				}
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}

		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{

	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received STOP.response\n");
	
		nfapi_stop_response_t msg;
			
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->stop_resp)
			{
				(config->stop_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
	
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_measurement_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received MEASUREMENT.response\n");
		
		nfapi_measurement_response_t msg;
		
		// unpack the message
		if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->measurement_resp)
			{
				(config->measurement_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_rssi_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received RSSI.response\n");		

		nfapi_rssi_response_t msg;
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->rssi_resp)
			{
				(config->rssi_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_rssi_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received RSSI.indication\n");	

		nfapi_rssi_indication_t msg;
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->rssi_ind)
			{
				(config->rssi_ind)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_cell_search_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CELL_SEARCH.response\n");
		
		nfapi_cell_search_response_t msg;
	
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->cell_search_resp)
			{
				(config->cell_search_resp)(config, p5_idx, &msg);
			}	
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_cell_search_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_cell_search_indication: NULL parameters\n");
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CELL_SEARCH.indication\n");
	
		nfapi_cell_search_indication_t msg;
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->cell_search_ind)
			{
				(config->cell_search_ind)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_cell_search_response: Unpack message failed, ignoring\n");
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_broadcast_detect_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received BROADCAST_DETECT.response\n");

		nfapi_broadcast_detect_response_t msg;

		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->broadcast_detect_resp)
			{
				(config->broadcast_detect_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_broadcast_detect_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received BROADCAST_DETECT.indication\n");

		nfapi_broadcast_detect_indication_t msg;

		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->broadcast_detect_ind)
			{
				(config->broadcast_detect_ind)(config, p5_idx, &msg);
			}	
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
			return;
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_system_information_schedule_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION_SCHEDULE.response\n");
	
		nfapi_system_information_schedule_response_t msg;
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->system_information_schedule_resp)
			{
				(config->system_information_schedule_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
			
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_system_information_schedule_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION_SCHEDULE.indication\n");

		nfapi_system_information_schedule_indication_t msg;

		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->system_information_schedule_ind)
			{
				(config->system_information_schedule_ind)(config, p5_idx, &msg);
			}	
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
	
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_system_information_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION.response\n");

		nfapi_system_information_response_t msg;
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0)
		{
			if(config->system_information_resp)
			{
				(config->system_information_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
	
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_system_information_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION.indication\n");
		
		nfapi_system_information_indication_t msg;
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
			return;
		}
	
		if(config->system_information_ind)
		{
			(config->system_information_ind)(config, p5_idx, &msg);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
}

void vnf_handle_nmm_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx)
{
	// ensure it's valid
	if (pRecvMsg == NULL || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Received NMM_STOP.response\n");	
		
		nfapi_nmm_stop_response_t msg;	
		
		// unpack the message
		if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(nfapi_nmm_stop_response_t), &config->codec_config) >= 0)
		{
			if(config->nmm_stop_resp)
			{
				(config->nmm_stop_resp)(config, p5_idx, &msg);
			}
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		}
		
		// make sure to release any dyanmic part of the message
		if(msg.vendor_extension)
			config->codec_config.deallocate(msg.vendor_extension);
	}
	
}

void vnf_handle_vendor_extension(void* pRecvMsg, int recvMsgLen, nfapi_vnf_config_t* config, int p5_idx, uint16_t message_id)
{
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
	
	if(config->allocate_p4_p5_vendor_ext && config->deallocate_p4_p5_vendor_ext)
	{
		uint16_t msg_size;
		
		nfapi_p4_p5_message_header_t* msg = config->allocate_p4_p5_vendor_ext(message_id, &msg_size);

		if(msg)
		{
			if(nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, msg, msg_size, &config->codec_config) >= 0)
			{
				if(config->vendor_ext)
					config->vendor_ext(config, p5_idx, msg);
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
			}
			
			config->deallocate_p4_p5_vendor_ext(msg);
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to allocate vendor extention structure\n");
		}
	}
}

void vnf_nr_handle_p4_p5_message(void *pRecvMsg, int recvMsgLen, int p5_idx, nfapi_vnf_config_t* config)
{
	nfapi_p4_p5_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < NFAPI_HEADER_LENGTH || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_p4_p5_message: invalid input params\n");
		return;
	}

	// unpack the message header
	if (nfapi_p5_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p4_p5_message_header_t), &config->codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	switch (messageHeader.message_id)
	{
		case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_RESPONSE:
			vnf_nr_handle_pnf_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE:
			vnf_nr_handle_pnf_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE:
			vnf_nr_handle_pnf_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_PNF_STOP_RESPONSE:
			vnf_handle_pnf_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE:
			vnf_nr_handle_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE:
			vnf_nr_handle_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE:
			vnf_nr_handle_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE:
			vnf_handle_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		default:
			{
				if(messageHeader.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   messageHeader.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					vnf_handle_vendor_extension(pRecvMsg, recvMsgLen, config, p5_idx, messageHeader.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P5 Unknown message ID %d\n", __FUNCTION__, messageHeader.message_id);
				}
			}
			break;
	}
}

void vnf_handle_p4_p5_message(void *pRecvMsg, int recvMsgLen, int p5_idx, nfapi_vnf_config_t* config)
{
	nfapi_p4_p5_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < NFAPI_HEADER_LENGTH || config == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_p4_p5_message: invalid input params\n");
		return;
	}

	// unpack the message header
	if (nfapi_p5_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p4_p5_message_header_t), &config->codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	switch (messageHeader.message_id)
	{
		case NFAPI_PNF_PARAM_RESPONSE:
			vnf_handle_pnf_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_PNF_CONFIG_RESPONSE:
			vnf_handle_pnf_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_PNF_START_RESPONSE:
			vnf_handle_pnf_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_PNF_STOP_RESPONSE:
			vnf_handle_pnf_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_PARAM_RESPONSE:
			vnf_handle_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_CONFIG_RESPONSE:
			vnf_handle_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_START_RESPONSE:
			vnf_handle_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_STOP_RESPONSE:
			vnf_handle_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_MEASUREMENT_RESPONSE:
			vnf_handle_measurement_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_RSSI_RESPONSE:
			vnf_handle_rssi_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_RSSI_INDICATION:
			vnf_handle_rssi_indication(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_CELL_SEARCH_RESPONSE:
			vnf_handle_cell_search_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_CELL_SEARCH_INDICATION:
			vnf_handle_cell_search_indication(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_BROADCAST_DETECT_RESPONSE:
			vnf_handle_broadcast_detect_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_BROADCAST_DETECT_INDICATION:
			vnf_handle_broadcast_detect_indication(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE:
			vnf_handle_system_information_schedule_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION:
			vnf_handle_system_information_schedule_indication(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_SYSTEM_INFORMATION_RESPONSE:
			vnf_handle_system_information_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_SYSTEM_INFORMATION_INDICATION:
			vnf_handle_system_information_indication(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		case NFAPI_NMM_STOP_RESPONSE:
			vnf_handle_nmm_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
			break;

		default:
			{
				if(messageHeader.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   messageHeader.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					vnf_handle_vendor_extension(pRecvMsg, recvMsgLen, config, p5_idx, messageHeader.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P5 Unknown message ID %d\n", __FUNCTION__, messageHeader.message_id);
				}
			}
			break;
	}
}


int vnf_nr_read_dispatch_message(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* pnf)
{
	if(1)
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
			message_size = sctp_recvmsg(pnf->p5_sock, header_buffer, header_buffer_size, (struct sockaddr*)&addr, &addr_len, &sndrcvinfo,  &flags);

			if(message_size == -1)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to peek sctp message size errno:%d\n", errno);
				return 0;
			}

			nfapi_p4_p5_message_header_t header;
			int unpack_result = nfapi_p5_message_header_unpack(header_buffer, header_buffer_size, &header, sizeof(header), 0);
			if(unpack_result < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to decode message header %d\n", unpack_result);
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
				NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to allocate dynamic buffer for sctp_recvmsg size:%d\n", message_size);
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
						// printf("\nVNF RECEIVES:\n");
						// for(int i=0; i<message_size; i++){
						// 	printf("%d", read_buffer[i]);
						// }
						// printf("\n");

						vnf_nr_handle_p4_p5_message(read_buffer, message_size, pnf->p5_idx, config);
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
}

int vnf_read_dispatch_message(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* pnf)
{
	if(1)
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
			message_size = sctp_recvmsg(pnf->p5_sock, header_buffer, header_buffer_size, (struct sockaddr*)&addr, &addr_len, &sndrcvinfo,  &flags);

			if(message_size == -1)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to peek sctp message size errno:%d\n", errno);
				return 0;
			}

			nfapi_p4_p5_message_header_t header;
			int unpack_result = nfapi_p5_message_header_unpack(header_buffer, header_buffer_size, &header, sizeof(header), 0);
			if(unpack_result < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to decode message header %d\n", unpack_result);
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
				NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to allocate dynamic buffer for sctp_recvmsg size:%d\n", message_size);
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
						// printf("\nVNF RECEIVES:\n");
						// for(int i=0; i<message_size; i++){
						// 	printf("%d", read_buffer[i]);
						// }
						// printf("\n");

						vnf_handle_p4_p5_message(read_buffer, message_size, pnf->p5_idx, config);
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
}

static int vnf_send_p5_msg(nfapi_vnf_pnf_info_t* pnf, const void *msg, int len, uint8_t stream)
{
	// printf("\n MESSAGE SENT: \n");
	// for(int i=0; i<len; i++){
	// 	printf("%d", *(uint8_t *)(msg + i));
	// }
	// printf("\n");

	//NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s len:%d stream:%d\n", __FUNCTION__, len, stream);

	int result = sctp_sendmsg(pnf->p5_sock, msg, len, (struct sockaddr*)&pnf->p5_pnf_sockaddr, sizeof(pnf->p5_pnf_sockaddr),1, 0, stream, 0, 4);

	if(result != len)
	{
		if(result <  0)
		{
			// error
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "sctp sendto failed errno: %d\n", errno);
		}
		else
		{
			// did not send all the message
		}
	}

	return 0;
}

int vnf_nr_pack_and_send_p5_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len)
{
	nfapi_vnf_pnf_info_t* pnf = nfapi_vnf_pnf_list_find(&(vnf->_public), p5_idx);
	
	if(pnf)
	{
		// pack the message for transmission
		int packedMessageLength = nfapi_nr_p5_message_pack(msg, msg_len, vnf->tx_message_buffer, sizeof(vnf->tx_message_buffer), &vnf->_public.codec_config);

		if (packedMessageLength < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p5_message_pack failed with return %d\n", packedMessageLength);
			return -1;
		}
		return vnf_send_p5_msg(pnf, vnf->tx_message_buffer, packedMessageLength, 0);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find pnf info for p5_idx:%d\n", __FUNCTION__, p5_idx);
		return -1;
	}
}


int vnf_pack_and_send_p5_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len)
{
	nfapi_vnf_pnf_info_t* pnf = nfapi_vnf_pnf_list_find(&(vnf->_public), p5_idx);
	
	if(pnf)
	{
		// pack the message for transmission
		int packedMessageLength = nfapi_p5_message_pack(msg, msg_len, vnf->tx_message_buffer, sizeof(vnf->tx_message_buffer), &vnf->_public.codec_config);

		if (packedMessageLength < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p5_message_pack failed with return %d\n", packedMessageLength);
			return -1;
		}

		return vnf_send_p5_msg(pnf, vnf->tx_message_buffer, packedMessageLength, 0/*msg->phy_id*/);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find pnf info for p5_idx:%d\n", __FUNCTION__, p5_idx);
		return -1;
	}
}


int vnf_pack_and_send_p4_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len)
{
	nfapi_vnf_pnf_info_t* pnf = nfapi_vnf_pnf_list_find(&(vnf->_public), p5_idx);
	
	if(pnf)
	{
		// pack the message for transmission
		int packedMessageLength = nfapi_p4_message_pack(msg, msg_len, vnf->tx_message_buffer, sizeof(vnf->tx_message_buffer), &vnf->_public.codec_config);

		if (packedMessageLength < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p4_message_pack failed with return %d\n", packedMessageLength);
			return -1;
		}

		return vnf_send_p5_msg(pnf, vnf->tx_message_buffer, packedMessageLength, 0/*msg->phy_id*/);
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find pnf info for p5_idx:%d\n", __FUNCTION__, p5_idx);
		return -1;
	}
}

