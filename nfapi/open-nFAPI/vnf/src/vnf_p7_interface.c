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


#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "vnf_p7.h"

#define FAPI2_IP_DSCP	0

nfapi_vnf_p7_config_t* nfapi_vnf_p7_config_create()
{
	vnf_p7_t* _this = (vnf_p7_t*)calloc(1, sizeof(vnf_p7_t));

	if(_this == 0)
		return 0;

	// todo : initialize
	_this->_public.segment_size = 1400;
	_this->_public.max_num_segments = 8;
	_this->_public.checksum_enabled = 1;
	
	_this->_public.malloc = &malloc;
	_this->_public.free = &free;	

	_this->_public.codec_config.allocate = &malloc;
	_this->_public.codec_config.deallocate = &free;
	

	return &(_this->_public);
}

void nfapi_vnf_p7_config_destory(nfapi_vnf_p7_config_t* config)
{
	free(config);
}


struct timespec timespec_add(struct timespec lhs, struct timespec rhs)
{
	struct timespec result;

	result.tv_sec = lhs.tv_sec + rhs.tv_sec;
	result.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;

	if(result.tv_nsec > 1e9)
	{
		result.tv_sec++;
		result.tv_nsec-= 1e9;
	}

	return result;
}

struct timespec timespec_sub(struct timespec lhs, struct timespec rhs)
{
	struct timespec result;
	if ((lhs.tv_nsec-rhs.tv_nsec)<0) 
	{
		result.tv_sec = lhs.tv_sec-rhs.tv_sec-1;
		result.tv_nsec = 1000000000+lhs.tv_nsec-rhs.tv_nsec;
	} 
	else 
	{
		result.tv_sec = lhs.tv_sec-rhs.tv_sec;
		result.tv_nsec = lhs.tv_nsec-rhs.tv_nsec;
	}
	return result;
}

// monitor the p7 endpoints and the timing loop and 
// send indications to mac
int nfapi_vnf_p7_start(nfapi_vnf_p7_config_t* config)
{
	if(config == 0)
		return -1;

	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;

	// Create p7 receive udp port 
	// todo : this needs updating for Ipv6
	
	NFAPI_TRACE(NFAPI_TRACE_INFO, "Initialising VNF P7 port:%u\n", config->port);

	// open the UDP socket
	if ((vnf_p7->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 socket errno: %d\n", errno);
		return -1;
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 socket created...\n");

	// configure the UDP socket options
	int iptos_value = FAPI2_IP_DSCP << 2;
	if (setsockopt(vnf_p7->socket, IPPROTO_IP, IP_TOS, &iptos_value, sizeof(iptos_value)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (IP_TOS) errno: %d\n", errno);
		return -1;
	}
	
	NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 setsockopt succeeded...\n");

	// Create the address structure
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config->port);
	addr.sin_addr.s_addr = INADDR_ANY;

	// bind to the configured port
	NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 binding too %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	if (bind(vnf_p7->socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
	//if (sctp_bindx(config->socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
		return -1;
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 bind succeeded...\n");


	//struct timespec original_pselect_timeout;
	struct timespec pselect_timeout;
	pselect_timeout.tv_sec = 0;
	pselect_timeout.tv_nsec = 1000000; // ns in a 1 us


	struct timespec pselect_start;
	struct timespec pselect_stop;

	//struct timespec sf_end;

	long last_millisecond = -1;


	struct timespec sf_duration;
	sf_duration.tv_sec = 0;
	sf_duration.tv_nsec = 1e6; // We want 1ms pause

	struct timespec sf_start;
	clock_gettime(CLOCK_MONOTONIC, &sf_start);
	long millisecond = sf_start.tv_nsec / 1e6;
	sf_start = timespec_add(sf_start, sf_duration);
	NFAPI_TRACE(NFAPI_TRACE_INFO, "next subframe will start at %d.%d\n", sf_start.tv_sec, sf_start.tv_nsec);

	while(vnf_p7->terminate == 0)
	{
		fd_set rfds;
		int maxSock = 0;
		FD_ZERO(&rfds);
		int selectRetval = 0;

		// Add the p7 socket
		FD_SET(vnf_p7->socket, &rfds);
		maxSock = vnf_p7->socket;
		
		clock_gettime(CLOCK_MONOTONIC, &pselect_start);
		//long millisecond = pselect_start.tv_nsec / 1e6;

		if((last_millisecond == -1) || (millisecond == last_millisecond) || (millisecond == (last_millisecond + 1) % 1000) )
		{
                  //NFAPI_TRACE(NFAPI_TRACE_INFO, "pselect_start:%d.%d sf_start:%d.%d\n", pselect_start.tv_sec, pselect_start.tv_nsec, sf_start.tv_sec, sf_start.tv_nsec);


			if((pselect_start.tv_sec > sf_start.tv_sec) || 
			   ((pselect_start.tv_sec == sf_start.tv_sec) && (pselect_start.tv_nsec > sf_start.tv_nsec)))
			{
				// overran the end of the subframe we do not want to wait
				pselect_timeout.tv_sec = 0;
				pselect_timeout.tv_nsec = 0;

				//struct timespec overrun = timespec_sub(pselect_start, sf_start);
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "Subframe overrun detected of %d.%d running to catchup\n", overrun.tv_sec, overrun.tv_nsec);
			}
			else
			{
				// still time before the end of the subframe wait
				pselect_timeout = timespec_sub(sf_start, pselect_start);

			}

//original_pselect_timeout = pselect_timeout;

			// detemine how long to sleep in ns before the start of the next 1ms
			//pselect_timeout.tv_nsec = 1e6 - (pselect_start.tv_nsec % 1000000);

			//uint8_t underrun_possible =0;
			
			// if we are not sleeping until the next milisecond due to the
			// insycn minor adjment flag it so we don't consider it an error
			//uint8_t underrun_possible =0;
			/*
			{
				nfapi_vnf_p7_connection_info_t* phy = vnf_p7->p7_connections;
				if(phy && phy->in_sync && phy->insync_minor_adjustment != 0 && phy->insync_minor_adjustment_duration > 0 && pselect_start.tv_nsec != 0)
				{
					NFAPI_TRACE(NFAPI_TRACE_NOTE, "[VNF] Subframe minor adjustment %d (%d->%d)\n", phy->insync_minor_adjustment,
							pselect_timeout.tv_nsec, pselect_timeout.tv_nsec - (phy->insync_minor_adjustment * 1000)) 
					if(phy->insync_minor_adjustment > 0)
					{
						// todo check we don't go below 0
						if((phy->insync_minor_adjustment * 1000) > pselect_timeout.tv_nsec)
							pselect_timeout.tv_nsec = 0;
						else
							pselect_timeout.tv_nsec = pselect_timeout.tv_nsec - (phy->insync_minor_adjustment * 1000);


						//underrun_possible = 1;
					}
					else if(phy->insync_minor_adjustment < 0)
					{
						// todo check we don't go below 0
						pselect_timeout.tv_nsec = pselect_timeout.tv_nsec - (phy->insync_minor_adjustment * 1000);
					}

					//phy->insync_minor_adjustment = 0;
					phy->insync_minor_adjustment_duration--;
				}
			}
			*/
			

//long wraps = pselect_timeout.tv_nsec % 1e9;


			selectRetval = pselect(maxSock+1, &rfds, NULL, NULL, &pselect_timeout, NULL);

			clock_gettime(CLOCK_MONOTONIC, &pselect_stop);

                        nfapi_vnf_p7_connection_info_t* phy = vnf_p7->p7_connections;

if (selectRetval==-1 && errno == 22)
{
  NFAPI_TRACE(NFAPI_TRACE_ERROR, "INVAL: pselect_timeout:%d.%ld adj[dur:%d adj:%d], sf_dur:%d.%ld\n", 
  pselect_timeout.tv_sec, pselect_timeout.tv_nsec, 
  phy->insync_minor_adjustment_duration, phy->insync_minor_adjustment, 
  sf_duration.tv_sec, sf_duration.tv_nsec);
}
			if(selectRetval == 0)
			{
				// calculate the start of the next subframe
				sf_start = timespec_add(sf_start, sf_duration);
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "next subframe will start at %d.%d\n", sf_start.tv_sec, sf_start.tv_nsec);

				if(phy && phy->in_sync && phy->insync_minor_adjustment != 0 && phy->insync_minor_adjustment_duration > 0)
				{
                                        long insync_minor_adjustment_ns = (phy->insync_minor_adjustment * 1000);

                                        sf_start.tv_nsec -= insync_minor_adjustment_ns;

#if 1
                                        if (sf_start.tv_nsec > 1e9)
                                        {
                                          sf_start.tv_sec++;
                                          sf_start.tv_nsec-=1e9;
                                        }
                                        else if (sf_start.tv_nsec < 0)
                                        {
                                          sf_start.tv_sec--;
                                          sf_start.tv_nsec+=1e9;
                                        }
#else
                                        //NFAPI_TRACE(NFAPI_TRACE_NOTE, "[VNF] BEFORE adjustment - Subframe minor adjustment %dus sf_start.tv_nsec:%d\n", phy->insync_minor_adjustment, sf_start.tv_nsec);
					if(phy->insync_minor_adjustment > 0)
					{
						// decrease the subframe duration a little
                                                if (sf_start.tv_nsec > insync_minor_adjustment_ns)
                                                  sf_start.tv_nsec -= insync_minor_adjustment_ns;
                                                else
                                                {
                                                  NFAPI_TRACE(NFAPI_TRACE_ERROR, "[VNF] Adjustment would make it negative sf:%d.%ld adjust:%ld\n\n\n", sf_start.tv_sec, sf_start.tv_nsec, insync_minor_adjustment_ns);
                                                  sf_start.tv_sec--;
                                                  sf_start.tv_nsec += 1e9 - insync_minor_adjustment_ns;
                                                }
					}
					else if(phy->insync_minor_adjustment < 0)
					{
						// todo check we don't go below 0
						// increase the subframe duration a little
						sf_start.tv_nsec += insync_minor_adjustment_ns;

                                                if (sf_start.tv_nsec < 0)
                                                {
                                                  NFAPI_TRACE(NFAPI_TRACE_ERROR, "[VNF] OVERFLOW %d.%ld\n\n\n\n", sf_start.tv_sec, sf_start.tv_nsec);
                                                  sf_start.tv_sec++;
                                                  sf_start.tv_nsec += 1e9;
                                                }
					}
#endif

					//phy->insync_minor_adjustment = 0;
                                        phy->insync_minor_adjustment_duration--;

                                        NFAPI_TRACE(NFAPI_TRACE_NOTE, "[VNF] AFTER adjustment - Subframe minor adjustment %dus sf_start.tv_nsec:%d duration:%u\n", 
                                            phy->insync_minor_adjustment, sf_start.tv_nsec, phy->insync_minor_adjustment_duration);

                                        if (phy->insync_minor_adjustment_duration==0)
                                        {
                                          phy->insync_minor_adjustment = 0;
                                        }
				}
				/*
				long pselect_stop_millisecond = pselect_stop.tv_nsec / 1e6;
				if(millisecond == pselect_stop_millisecond)
				{
					// we have woke up in the same subframe
					if(underrun_possible == 0)
						NFAPI_TRACE(NFAPI_TRACE_WARN, "subframe pselect underrun %ld (%d.%d)\n", millisecond, pselect_stop.tv_sec, pselect_stop.tv_nsec);
				}
				else if(((millisecond + 1) % 1000) != pselect_stop_millisecond)
				{
					// we have overrun the subframe
					NFAPI_TRACE(NFAPI_TRACE_WARN, "subframe pselect overrun %ld %ld\n", millisecond, pselect_stop_millisecond);
					NFAPI_TRACE(NFAPI_TRACE_WARN, "subframe underrun %ld\n", millisecond);
				}
				last_millisecond = millisecond;
				*/
				
				millisecond ++;
			}
		}
		else
		{
			// we have overrun the subframe advance to go and collect $200
			if((millisecond - last_millisecond) > 3)
				NFAPI_TRACE(NFAPI_TRACE_WARN, "subframe overrun %ld %ld (%ld)\n", millisecond, last_millisecond, millisecond - last_millisecond + 1);

			last_millisecond = ( last_millisecond + 1 ) % 1000;
			selectRetval = 0;
		}

		if(selectRetval == 0)
		{
			vnf_p7->sf_start_time_hr = vnf_get_current_time_hr();

			// pselect timed out
			nfapi_vnf_p7_connection_info_t* curr = vnf_p7->p7_connections;

			while(curr != 0)
			{
				curr->sfn_sf = increment_sfn_sf(curr->sfn_sf);
				
				vnf_sync(vnf_p7, curr);

				curr = curr->next;
			}

			send_mac_subframe_indications(vnf_p7);

		}
		else if(selectRetval > 0)
		{
			// have a p7 message
			if(FD_ISSET(vnf_p7->socket, &rfds))
			{
				vnf_p7_read_dispatch_message(vnf_p7);
			}
		}
		else
		{
			// pselect error
			if(selectRetval == -1 && errno == EINTR)
			{
				// a sigal was received.
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_INFO, "P7 select failed result %d errno %d timeout:%d.%d orginal:%d.%d last_ms:%ld ms:%ld\n", selectRetval, errno, pselect_timeout.tv_sec, pselect_timeout.tv_nsec, pselect_timeout.tv_sec, pselect_timeout.tv_nsec, last_millisecond, millisecond);
				// should we exit now?
                                if (selectRetval == -1 && errno == 22) // invalid argument??? not sure about timeout duration
                                {
                                  usleep(100000);
                                }
			}
		}

	}

	
	NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p7 socket\n");
	close(vnf_p7->socket);

	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() returning\n", __FUNCTION__);

	return 0;
}

int nfapi_vnf_p7_stop(nfapi_vnf_p7_config_t* config)
{
	if(config == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	vnf_p7->terminate =1;
	return 0;
}

int nfapi_vnf_p7_add_pnf(nfapi_vnf_p7_config_t* config, const char* pnf_p7_addr, int pnf_p7_port, int phy_id)
{
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(config:%p phy_id:%d pnf_addr:%s pnf_p7_port:%d)\n", __FUNCTION__, config, phy_id,  pnf_p7_addr, pnf_p7_port);

	if(config == 0)
        {
          return -1;
        }

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;

	nfapi_vnf_p7_connection_info_t* node = (nfapi_vnf_p7_connection_info_t*)malloc(sizeof(nfapi_vnf_p7_connection_info_t));

	memset(node, 0, sizeof(nfapi_vnf_p7_connection_info_t));
	node->phy_id = phy_id;
	node->in_sync = 0;
	node->dl_out_sync_offset = 30;
	node->dl_out_sync_period = 10;
	node->dl_in_sync_offset = 30;
	node->dl_in_sync_period = 512;
	node->sfn_sf = 0;

	node->min_sync_cycle_count = 8;

	// save the remote endpoint information
	node->remote_addr.sin_family = AF_INET;
	node->remote_addr.sin_port = htons(pnf_p7_port);
	node->remote_addr.sin_addr.s_addr = inet_addr(pnf_p7_addr);

	vnf_p7_connection_info_list_add(vnf_p7, node);

	return 0;
}

int nfapi_vnf_p7_del_pnf(nfapi_vnf_p7_config_t* config, int phy_id)
{
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(phy_id:%d)\n", __FUNCTION__, phy_id);

	if(config == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;

	nfapi_vnf_p7_connection_info_t* to_delete = vnf_p7_connection_info_list_delete(vnf_p7, phy_id);

	if(to_delete)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(phy_id:%d) deleting connection info\n", __FUNCTION__, phy_id);
		free(to_delete);
	}

	return 0;
}
int nfapi_vnf_p7_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_dl_config_request_t* req)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(config:%p req:%p)\n", __FUNCTION__, config, req);

	if(config == 0 || req == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_ul_config_req(nfapi_vnf_p7_config_t* config, nfapi_ul_config_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}
int nfapi_vnf_p7_hi_dci0_req(nfapi_vnf_p7_config_t* config, nfapi_hi_dci0_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}
int nfapi_vnf_p7_tx_req(nfapi_vnf_p7_config_t* config, nfapi_tx_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}
int nfapi_vnf_p7_lbt_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_lbt_dl_config_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}
int nfapi_vnf_p7_vendor_extension(nfapi_vnf_p7_config_t* config, nfapi_p7_message_header_t* header)
{
	if(config == 0 || header == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	return vnf_p7_pack_and_send_p7_msg(vnf_p7, header);
}

int nfapi_vnf_p7_ue_release_req(nfapi_vnf_p7_config_t* config, nfapi_ue_release_request_t* req)
{
    if(config == 0 || req == 0)
        return -1;

    vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
    return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_release_msg(nfapi_vnf_p7_config_t* config, nfapi_p7_message_header_t* header)
{
	if(config == 0 || header == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	vnf_p7_release_msg(vnf_p7, header);

	return 0;

}

int nfapi_vnf_p7_release_pdu(nfapi_vnf_p7_config_t* config, void* pdu)
{
	if(config == 0 || pdu == 0)
		return -1;

	vnf_p7_t* vnf_p7 = (vnf_p7_t*)config;
	vnf_p7_release_pdu(vnf_p7, pdu);

	return 0;
}
