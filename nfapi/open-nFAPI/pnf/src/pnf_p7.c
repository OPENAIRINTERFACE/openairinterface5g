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


#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#include "pnf_p7.h"

#define FAPI2_IP_DSCP	0

extern uint16_t sf_ahead;
extern uint16_t slot_ahead;

//uint16_t sf_ahead=4;

void add_slot(uint16_t *frameP, uint16_t *slotP, int offset)
{
	uint16_t num_slots = 20; // set based on numerlogy (fixing for 1)

    *frameP    = (*frameP + ((*slotP + offset) / num_slots))%1024; 

    *slotP = ((*slotP + offset) % num_slots);
}

void add_sf(uint16_t *frameP, uint16_t *subframeP, int offset)
{
    *frameP    = (*frameP + ((*subframeP + offset) / 10))%1024;

    *subframeP = ((*subframeP + offset) % 10);
}

void subtract_sf(uint16_t *frameP, uint16_t *subframeP, int offset)
{
  if (*subframeP < offset)
  {
    *frameP = (*frameP+1024-1)%1024;
  }
  *subframeP = (*subframeP+10-offset)%10;
}

uint32_t sfnslot_add_slot(uint16_t sfn, uint16_t slot, int offset)
{
  uint32_t new_sfnslot;
//   uint16_t sfn = NFAPI_SFNSLOT2SFN(sfnslot);
//   uint16_t slot  = NFAPI_SFNSLOT2SLOT(sfnslot);

  //printf("%s() sfn:%u sf:%u\n", __FUNCTION__, sfn, sf);
  add_slot(&sfn, &slot, offset);

  new_sfnslot = sfn<<6|slot;

  //printf("%s() sfn:%u sf:%u offset:%d sfnsf:%d(DEC:%d) new:%d(DEC:%d)\n", __FUNCTION__, sfn, sf, offset, sfnsf, NFAPI_SFNSF2DEC(sfnsf), new_sfnsf, NFAPI_SFNSF2DEC(new_sfnsf));

  return new_sfnslot;
}

uint16_t sfnsf_add_sf(uint16_t sfnsf, int offset)
{
  uint16_t new_sfnsf;
  uint16_t sfn = NFAPI_SFNSF2SFN(sfnsf);
  uint16_t sf  = NFAPI_SFNSF2SF(sfnsf);

  //printf("%s() sfn:%u sf:%u\n", __FUNCTION__, sfn, sf);
  add_sf(&sfn, &sf, offset);

  new_sfnsf = sfn<<4|sf;

  //printf("%s() sfn:%u sf:%u offset:%d sfnsf:%d(DEC:%d) new:%d(DEC:%d)\n", __FUNCTION__, sfn, sf, offset, sfnsf, NFAPI_SFNSF2DEC(sfnsf), new_sfnsf, NFAPI_SFNSF2DEC(new_sfnsf));

  return new_sfnsf;
}

uint16_t sfnsf_subtract_sf(uint16_t sfnsf, int offset)
{
  uint16_t new_sfnsf;
  uint16_t sfn = NFAPI_SFNSF2SFN(sfnsf);
  uint16_t sf  = NFAPI_SFNSF2SF(sfnsf);

  //printf("%s() sfn:%u sf:%u\n", __FUNCTION__, sfn, sf);
  subtract_sf(&sfn, &sf, offset);

  new_sfnsf = sfn<<4|sf;

  //printf("%s() offset:%d sfnsf:%d(DEC:%d) new:%d(DEC:%d)\n", __FUNCTION__, offset, sfnsf, NFAPI_SFNSF2DEC(sfnsf), new_sfnsf, NFAPI_SFNSF2DEC(new_sfnsf));

  return new_sfnsf;
}

uint32_t pnf_get_current_time_hr(void)
{
	struct timeval now;
	(void)gettimeofday(&now, NULL);
	uint32_t time_hr = TIME2TIMEHR(now);
	return time_hr;
}

void* pnf_p7_malloc(pnf_p7_t* pnf_p7, size_t size)
{
	if(pnf_p7->_public.malloc)
	{
		return (pnf_p7->_public.malloc)(size);
	}
	else
	{
		return calloc(1, size); 
	}
}
void pnf_p7_free(pnf_p7_t* pnf_p7, void* ptr)
{
	if(pnf_p7->_public.free)
	{
		return (pnf_p7->_public.free)(ptr);
	}
	else
	{
		return free(ptr); 
	}
}

// todo : for now these just malloc/free need to move to a memory cache
nfapi_nr_dl_tti_request_t* allocate_nfapi_dl_tti_request(pnf_p7_t* pnf_p7) 
{ 
	void *ptr= pnf_p7_malloc(pnf_p7, sizeof(nfapi_nr_dl_tti_request_t));
        //printf("%s() ptr:%p\n", __FUNCTION__, ptr);
        return ptr;
}
nfapi_dl_config_request_t* allocate_nfapi_dl_config_request(pnf_p7_t* pnf_p7) 
{ 
	void *ptr= pnf_p7_malloc(pnf_p7, sizeof(nfapi_dl_config_request_t));
        //printf("%s() ptr:%p\n", __FUNCTION__, ptr);
        return ptr;
}
void deallocate_nfapi_dl_tti_request(nfapi_nr_dl_tti_request_t* req, pnf_p7_t* pnf_p7) 
{ 
  //printf("%s() SFN/SF:%d %s req:%p pdu_list:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->dl_config_request_body.dl_config_pdu_list);
	
	// if(pnf_p7->_public.codec_config.deallocate)
	// {	
	// 	(pnf_p7->_public.codec_config.deallocate)(req); 
	// }
	// else
	// {
	// 	free(req);
	// }

	pnf_p7_free(pnf_p7, req);
}

void deallocate_nfapi_dl_config_request(nfapi_dl_config_request_t* req, pnf_p7_t* pnf_p7) 
{ 
  //printf("%s() SFN/SF:%d %s req:%p pdu_list:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->dl_config_request_body.dl_config_pdu_list);
	if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->dl_config_request_body.dl_config_pdu_list);
	}
	else
	{
		free(req->dl_config_request_body.dl_config_pdu_list);
	}
        req->dl_config_request_body.dl_config_pdu_list=0;

	pnf_p7_free(pnf_p7, req);
}

nfapi_nr_ul_tti_request_t* allocate_nfapi_ul_tti_request(pnf_p7_t* pnf_p7) 
{ 
	void *ptr= pnf_p7_malloc(pnf_p7, sizeof(nfapi_nr_ul_tti_request_t));
        //printf("%s() ptr:%p\n", __FUNCTION__, ptr);
        return ptr;
}

nfapi_ul_config_request_t* allocate_nfapi_ul_config_request(pnf_p7_t* pnf_p7) 
{ 
	void *ptr= pnf_p7_malloc(pnf_p7, sizeof(nfapi_ul_config_request_t));
        //printf("%s() ptr:%p\n", __FUNCTION__, ptr);
        return ptr;
}

void deallocate_nfapi_ul_tti_request(nfapi_nr_ul_tti_request_t* req, pnf_p7_t* pnf_p7) 
{ 
  //printf("%s() SFN/SF:%d %s req:%p pdu_list:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->ul_config_request_body.ul_config_pdu_list);
	/*if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->pdus_list);
		(pnf_p7->_public.codec_config.deallocate)(req->groups_list);
	}
	else
	{
		free(req->pdus_list);
		free(req->groups_list);
	}
	*/
	pnf_p7_free(pnf_p7, req);
}

void deallocate_nfapi_ul_config_request(nfapi_ul_config_request_t* req, pnf_p7_t* pnf_p7) 
{ 
  //printf("%s() SFN/SF:%d %s req:%p pdu_list:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->ul_config_request_body.ul_config_pdu_list);
	if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->ul_config_request_body.ul_config_pdu_list);
	}
	else
	{
		free(req->ul_config_request_body.ul_config_pdu_list);
	}
        req->ul_config_request_body.ul_config_pdu_list=0;

	pnf_p7_free(pnf_p7, req);
}

nfapi_nr_ul_dci_request_t* allocate_nfapi_ul_dci_request(pnf_p7_t* pnf_p7) 
{ 
	return pnf_p7_malloc(pnf_p7, sizeof(nfapi_nr_ul_dci_request_t));
}

nfapi_hi_dci0_request_t* allocate_nfapi_hi_dci0_request(pnf_p7_t* pnf_p7) 
{ 
	return pnf_p7_malloc(pnf_p7, sizeof(nfapi_hi_dci0_request_t));
}

void deallocate_nfapi_ul_dci_request(nfapi_nr_ul_dci_request_t* req, pnf_p7_t* pnf_p7) 
{ 
  //printf("%s() SFN/SF:%d %s req:%p pdu_list:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->hi_dci0_request_body.hi_dci0_pdu_list);
	// if(pnf_p7->_public.codec_config.deallocate)
	// {
	// 	(pnf_p7->_public.codec_config.deallocate)(req->ul_dci_pdu_list);
	// }
	// else
	// {
	// 	free(req->ul_dci_pdu_list);
	// }

	pnf_p7_free(pnf_p7, req);
}

void deallocate_nfapi_hi_dci0_request(nfapi_hi_dci0_request_t* req, pnf_p7_t* pnf_p7) 
{ 
  //printf("%s() SFN/SF:%d %s req:%p pdu_list:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->hi_dci0_request_body.hi_dci0_pdu_list);
	if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->hi_dci0_request_body.hi_dci0_pdu_list);
	}
	else
	{
		free(req->hi_dci0_request_body.hi_dci0_pdu_list);
	}
        req->hi_dci0_request_body.hi_dci0_pdu_list=0;

	pnf_p7_free(pnf_p7, req);
}

nfapi_nr_tx_data_request_t* allocate_nfapi_tx_data_request(pnf_p7_t* pnf_p7) 
{ 
	return pnf_p7_malloc(pnf_p7, sizeof(nfapi_nr_tx_data_request_t));
}

nfapi_tx_request_t* allocate_nfapi_tx_request(pnf_p7_t* pnf_p7) 
{ 
	return pnf_p7_malloc(pnf_p7, sizeof(nfapi_tx_request_t));
}

//TODO: Check if deallocate_nfapi_tx_data_request defn is proper
void deallocate_nfapi_tx_data_request(nfapi_nr_tx_data_request_t* req, pnf_p7_t* pnf_p7) 
{
/*
	if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->pdu_list);
	}
	else
	{
		free(req->pdu_list);
	}
*/
	pnf_p7_free(pnf_p7, req);
}

void deallocate_nfapi_tx_request(nfapi_tx_request_t* req, pnf_p7_t* pnf_p7) 
{ 
	int i = 0;

  //printf("%s() SFN/SF:%d %s req:%p pdu[0]:data:%p\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pnf_p7->_public.codec_config.deallocate ? "DEALLOCATE" : "FREE", req, req->tx_request_body.tx_pdu_list[i].segments[0].segment_data);

	for(i = 0; i < req->tx_request_body.number_of_pdus; ++i)
	{
		void* data = req->tx_request_body.tx_pdu_list[i].segments[0].segment_data;

		if(pnf_p7->_public.codec_config.deallocate)
		{
			(pnf_p7->_public.codec_config.deallocate)(data);
		}
		else
		{
			free(data);
		}
                data=0;
	}


	if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->tx_request_body.tx_pdu_list);
	}
	else
	{
		free(req->tx_request_body.tx_pdu_list);
	}
        req->tx_request_body.tx_pdu_list=0;

	pnf_p7_free(pnf_p7, req);
}

nfapi_lbt_dl_config_request_t* allocate_nfapi_lbt_dl_config_request(pnf_p7_t* pnf_p7) 
{ 
	return pnf_p7_malloc(pnf_p7, sizeof(nfapi_lbt_dl_config_request_t));
}

void deallocate_nfapi_lbt_dl_config_request(nfapi_lbt_dl_config_request_t* req, pnf_p7_t* pnf_p7) 
{ 
	if(pnf_p7->_public.codec_config.deallocate)
	{
		(pnf_p7->_public.codec_config.deallocate)(req->lbt_dl_config_request_body.lbt_dl_config_req_pdu_list);
	}
	else
	{
		free(req->lbt_dl_config_request_body.lbt_dl_config_req_pdu_list);
	}
        req->lbt_dl_config_request_body.lbt_dl_config_req_pdu_list=0;

	pnf_p7_free(pnf_p7, req);
}

nfapi_ue_release_request_t* allocate_nfapi_ue_release_request(pnf_p7_t* pnf_p7)
{
    return pnf_p7_malloc(pnf_p7, sizeof(nfapi_ue_release_request_t));
}

void deallocate_nfapi_ue_release_request(nfapi_ue_release_request_t* req, pnf_p7_t* pnf_p7)
{
	pnf_p7_free(pnf_p7, req);
}

pnf_p7_rx_message_t* pnf_p7_rx_reassembly_queue_add_segment(pnf_p7_t* pnf_p7, pnf_p7_rx_reassembly_queue_t* queue, uint32_t rx_hr_time, uint16_t sequence_number, uint16_t segment_number, uint8_t m, uint8_t* data, uint16_t data_len)
{
	pnf_p7_rx_message_t* msg = 0;
	// attempt to find a entry for this segment
	pnf_p7_rx_message_t* iterator = queue->msg_queue;
	while(iterator != 0)
	{
		if(iterator->sequence_number == sequence_number)
		{
			msg = iterator;
			break;
		}

		iterator = iterator->next;
	}
	
	// if found then copy data to message
	if(msg != 0)
	{
	
		msg->segments[segment_number].buffer = (uint8_t*)pnf_p7_malloc(pnf_p7, data_len);
		memcpy(msg->segments[segment_number].buffer, data, data_len);
		msg->segments[segment_number].length = data_len;

		msg->num_segments_received++;

		// set the segement number if we have the last segment
		if(m == 0)
			msg->num_segments_expected = segment_number + 1;
	}
	// else add new rx message entry
	else
	{
		// create a new message
		msg = (pnf_p7_rx_message_t*)(pnf_p7_malloc(pnf_p7, sizeof(pnf_p7_rx_message_t)));
		memset(msg, 0, sizeof(pnf_p7_rx_message_t));

		msg->sequence_number = sequence_number;
		msg->num_segments_expected = m ? 255 : segment_number + 1;
		msg->num_segments_received = 1;
		msg->rx_hr_time = rx_hr_time;

		msg->segments[segment_number].buffer = (uint8_t*)pnf_p7_malloc(pnf_p7, data_len);
		memcpy(msg->segments[segment_number].buffer, data, data_len);
		msg->segments[segment_number].length = data_len;

		// place the message at the head of the queue
		msg->next = queue->msg_queue;
		queue->msg_queue = msg;
	}

	return msg;
}

void pnf_p7_rx_reassembly_queue_remove_msg(pnf_p7_t* pnf_p7, pnf_p7_rx_reassembly_queue_t* queue, pnf_p7_rx_message_t* msg)
{
	// remove message if it has the same sequence number
	pnf_p7_rx_message_t* iterator = queue->msg_queue;
	pnf_p7_rx_message_t* previous = 0;

	while(iterator != 0)
	{
		if(iterator->sequence_number == msg->sequence_number)
		{
			if(previous == 0)
			{
				queue->msg_queue = iterator->next;
			}
			else
			{
				previous->next = iterator->next;
			}

			//NFAPI_TRACE(NFAPI_TRACE_INFO, "Deleting reassembly message\n");
			// delete the message
			uint16_t i;
			for(i = 0; i < 128; ++i)
			{
				if(iterator->segments[i].buffer)
					pnf_p7_free(pnf_p7, iterator->segments[i].buffer);
			}
			pnf_p7_free(pnf_p7, iterator);

			break;
		}

		previous = iterator;
		iterator = iterator->next;
	}
}

void pnf_p7_rx_reassembly_queue_remove_old_msgs(pnf_p7_t* pnf_p7, pnf_p7_rx_reassembly_queue_t* queue, uint32_t rx_hr_time, uint32_t delta)
{
	// remove all messages that are too old
	pnf_p7_rx_message_t* iterator = queue->msg_queue;
	pnf_p7_rx_message_t* previous = 0;

	while(iterator != 0)
	{
		if(rx_hr_time - iterator->rx_hr_time > delta)
		{
			if(previous == 0)
			{
				queue->msg_queue = iterator->next;
			}
			else
			{
				previous->next = iterator->next;
			}
			
			NFAPI_TRACE(NFAPI_TRACE_INFO, "Deleting stale reassembly message (%u %u %d)\n", iterator->rx_hr_time, rx_hr_time, delta);

			pnf_p7_rx_message_t* to_delete = iterator;
			iterator = iterator->next;

			// delete the message
			uint16_t i;
			for(i = 0; i < 128; ++i)
			{
				if(to_delete->segments[i].buffer)
					pnf_p7_free(pnf_p7, to_delete->segments[i].buffer);
			}
			pnf_p7_free(pnf_p7, to_delete);

		}
		else
		{
			previous = iterator;
			iterator = iterator->next;
		}
	}
}


static uint32_t get_slot_time(uint32_t now_hr, uint32_t slot_start_hr)
{
	if(now_hr < slot_start_hr)
	{
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "now is earlier than start of subframe now_hr:%u sf_start_hr:%u\n", now_hr, sf_start_hr);
		return 0;
	}
	else
	{
		uint32_t now_us = TIMEHR_USEC(now_hr);
		uint32_t slot_start_us = TIMEHR_USEC(slot_start_hr);

		// if the us have wrapped adjust for it
		if(now_hr < slot_start_us)
		{
			now_us += 500000; 
		}

		return now_us - slot_start_us;
	}
}

static uint32_t get_sf_time(uint32_t now_hr, uint32_t sf_start_hr)
{
	if(now_hr < sf_start_hr)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "now is earlier than start of subframe\n");
		return 0;
	}
	else
	{
		uint32_t now_us = TIMEHR_USEC(now_hr);
		uint32_t sf_start_us = TIMEHR_USEC(sf_start_hr);

		// if the us have wrapped adjust for it
		if(now_hr < sf_start_us)
		{
			now_us += 1000000;
		}

		return now_us - sf_start_us;
	}
}



int pnf_p7_send_message(pnf_p7_t* pnf_p7, uint8_t* msg, uint32_t len)
{
	// todo : consider how to do this only once
	struct sockaddr_in remote_addr;
	memset((char*)&remote_addr, 0, sizeof(struct sockaddr_in));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(pnf_p7->_public.remote_p7_port);

	
	if(inet_aton(pnf_p7->_public.remote_p7_addr, &remote_addr.sin_addr) == -1)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "inet_aton failed %d\n", errno);
		return -1;
	}
	
	socklen_t remote_addr_len = sizeof(struct sockaddr_in);
	
	int sendto_result;
	
	if ((sendto_result = sendto((int)pnf_p7->p7_sock, (const char*)msg, len, 0, (const struct sockaddr*)&remote_addr, remote_addr_len)) < 0)
//if ((sendto_result = sendto((int)pnf_p7->p7_sock,"hello", 6, 0, (const struct sockaddr*)&remote_addr, remote_addr_len)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s %s:%d sendto(%d, %p, %d) %d failed errno: %d\n", __FUNCTION__, pnf_p7->_public.remote_p7_addr, pnf_p7->_public.remote_p7_port, (int)pnf_p7->p7_sock, (const char*)msg, len, remote_addr_len,  errno);
		return -1;
	}

	if(sendto_result != len)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s sendto failed to send the entire message %d %d\n", __FUNCTION__, sendto_result, len);
	}
	return 0;
}

int pnf_p7_pack_and_send_p7_message(pnf_p7_t* pnf_p7, nfapi_p7_message_header_t* header, uint32_t msg_len)
{
	header->m_segment_sequence = NFAPI_P7_SET_MSS(0, 0, pnf_p7->sequence_number);

	// Need to guard against different threads calling the encode function at the same time
	if(pthread_mutex_lock(&(pnf_p7->pack_mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
		return -1;
	}

	int len = nfapi_p7_message_pack(header, pnf_p7->tx_message_buffer, sizeof(pnf_p7->tx_message_buffer), &pnf_p7->_public.codec_config);

	if (len < 0)
	{
		if(pthread_mutex_unlock(&(pnf_p7->pack_mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return -1;
		}
		
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p7_message_pack failed with return %d\n", len );
		return -1;
	}

	if(len > pnf_p7->_public.segment_size)
	{
		int msg_body_len = len - NFAPI_P7_HEADER_LENGTH ; 
		int seg_body_len = pnf_p7->_public.segment_size - NFAPI_P7_HEADER_LENGTH ; 
		int segment_count = (msg_body_len / (seg_body_len)) + ((msg_body_len % seg_body_len) ? 1 : 0); 

		int segment = 0;
		int offset = NFAPI_P7_HEADER_LENGTH;
		uint8_t buffer[pnf_p7->_public.segment_size];
		for(segment = 0; segment < segment_count; ++segment)
		{
			uint8_t last = 0;
			uint16_t size = pnf_p7->_public.segment_size - NFAPI_P7_HEADER_LENGTH;
			if(segment + 1 == segment_count)
			{
				last = 1;
				size = (msg_body_len) - (seg_body_len * segment);
			}

			uint16_t segment_size = size + NFAPI_P7_HEADER_LENGTH;

			// Update the header with the m and segement 
			memcpy(&buffer[0], pnf_p7->tx_message_buffer, NFAPI_P7_HEADER_LENGTH);

			// set the segment length
			buffer[4] = (segment_size & 0xFF00) >> 8;
			buffer[5] = (segment_size & 0xFF);

			// set the m & segment number
			buffer[6] = ((!last) << 7) + segment;

			memcpy(&buffer[NFAPI_P7_HEADER_LENGTH], pnf_p7->tx_message_buffer + offset, size);
			offset += size;

			if(pnf_p7->_public.checksum_enabled)
			{
				nfapi_p7_update_checksum(buffer, segment_size);
			}


			pnf_p7_send_message(pnf_p7, &buffer[0], segment_size);
		}
	}
	else
	{
		if(pnf_p7->_public.checksum_enabled)
		{
			nfapi_p7_update_checksum(pnf_p7->tx_message_buffer, len);
		}

		// simple case that the message fits in a single segment
		pnf_p7_send_message(pnf_p7, pnf_p7->tx_message_buffer, len);
	}

	pnf_p7->sequence_number++;
	
	if(pthread_mutex_unlock(&(pnf_p7->pack_mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
		return -1;
	}

	return 0;
}


int pnf_nr_p7_pack_and_send_p7_message(pnf_p7_t* pnf_p7, nfapi_p7_message_header_t* header, uint32_t msg_len)
{
	header->m_segment_sequence = NFAPI_P7_SET_MSS(0, 0, pnf_p7->sequence_number);

	// Need to guard against different threads calling the encode function at the same time
	if(pthread_mutex_lock(&(pnf_p7->pack_mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
		return -1;
	}

	int len = nfapi_nr_p7_message_pack(header, pnf_p7->tx_message_buffer, sizeof(pnf_p7->tx_message_buffer), &pnf_p7->_public.codec_config);

	if (len < 0)
	{
		if(pthread_mutex_unlock(&(pnf_p7->pack_mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return -1;
		}
		
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p7_message_pack failed with return %d\n", len );
		return -1;
	}

	if(len > pnf_p7->_public.segment_size)
	{
		int msg_body_len = len - NFAPI_P7_HEADER_LENGTH ; 
		int seg_body_len = pnf_p7->_public.segment_size - NFAPI_P7_HEADER_LENGTH ; 
		int segment_count = (msg_body_len / (seg_body_len)) + ((msg_body_len % seg_body_len) ? 1 : 0); 

		int segment = 0;
		int offset = NFAPI_P7_HEADER_LENGTH;
		uint8_t buffer[pnf_p7->_public.segment_size];
		for(segment = 0; segment < segment_count; ++segment)
		{
			uint8_t last = 0;
			uint16_t size = pnf_p7->_public.segment_size - NFAPI_P7_HEADER_LENGTH;
			if(segment + 1 == segment_count)
			{
				last = 1;
				size = (msg_body_len) - (seg_body_len * segment);
			}

			uint16_t segment_size = size + NFAPI_P7_HEADER_LENGTH;

			// Update the header with the m and segement 
			memcpy(&buffer[0], pnf_p7->tx_message_buffer, NFAPI_P7_HEADER_LENGTH);

			// set the segment length
			buffer[4] = (segment_size & 0xFF00) >> 8;
			buffer[5] = (segment_size & 0xFF);

			// set the m & segment number
			buffer[6] = ((!last) << 7) + segment;

			memcpy(&buffer[NFAPI_P7_HEADER_LENGTH], pnf_p7->tx_message_buffer + offset, size);
			offset += size;

			if(pnf_p7->_public.checksum_enabled)
			{
				nfapi_p7_update_checksum(buffer, segment_size);
			}


			pnf_p7_send_message(pnf_p7, &buffer[0], segment_size);
		}
	}
	else
	{
		if(pnf_p7->_public.checksum_enabled)
		{
			nfapi_p7_update_checksum(pnf_p7->tx_message_buffer, len);
		}

		// simple case that the message fits in a single segment
		pnf_p7_send_message(pnf_p7, pnf_p7->tx_message_buffer, len);
	}

	pnf_p7->sequence_number++;
	
	if(pthread_mutex_unlock(&(pnf_p7->pack_mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
		return -1;
	}

	return 0;
}

void pnf_pack_and_send_timing_info(pnf_p7_t* pnf_p7)
{
	nfapi_timing_info_t timing_info;
	memset(&timing_info, 0, sizeof(timing_info));
	timing_info.header.message_id = NFAPI_TIMING_INFO;
	timing_info.header.phy_id = pnf_p7->_public.phy_id;

	timing_info.last_sfn_sf = pnf_p7->sfn_sf;
	timing_info.time_since_last_timing_info = pnf_p7->timing_info_ms_counter;

	timing_info.dl_config_jitter = pnf_p7->dl_config_jitter;
	timing_info.tx_request_jitter = pnf_p7->tx_jitter;
	timing_info.ul_config_jitter = pnf_p7->ul_config_jitter;
	timing_info.hi_dci0_jitter = pnf_p7->hi_dci0_jitter;

	timing_info.dl_config_latest_delay = 0;
	timing_info.tx_request_latest_delay = 0;
	timing_info.ul_config_latest_delay = 0;
	timing_info.hi_dci0_latest_delay = 0;

	timing_info.dl_config_earliest_arrival = 0;
	timing_info.tx_request_earliest_arrival = 0;
	timing_info.ul_config_earliest_arrival = 0;
	timing_info.hi_dci0_earliest_arrival = 0;


	pnf_p7_pack_and_send_p7_message(pnf_p7, &(timing_info.header), sizeof(timing_info));

	pnf_p7->timing_info_ms_counter = 0;
}


void pnf_nr_pack_and_send_timing_info(pnf_p7_t* pnf_p7)
{
	nfapi_nr_timing_info_t timing_info;
	memset(&timing_info, 0, sizeof(timing_info));
	timing_info.header.message_id = NFAPI_TIMING_INFO;
	timing_info.header.phy_id = pnf_p7->_public.phy_id;

	timing_info.last_sfn = pnf_p7->sfn;
	timing_info.last_slot = pnf_p7->slot;
	timing_info.time_since_last_timing_info = pnf_p7->timing_info_ms_counter;

	timing_info.dl_tti_jitter = pnf_p7->dl_tti_jitter;
	timing_info.tx_data_request_jitter = pnf_p7->tx_data_jitter;
	timing_info.ul_tti_jitter = pnf_p7->ul_tti_jitter;
	timing_info.ul_dci_jitter = pnf_p7->ul_dci_jitter;

	timing_info.dl_tti_latest_delay = 0;
	timing_info.tx_data_request_latest_delay = 0;
	timing_info.ul_tti_latest_delay = 0;
	timing_info.ul_dci_latest_delay = 0;

	timing_info.dl_tti_earliest_arrival = 0;
	timing_info.tx_data_request_earliest_arrival = 0;
	timing_info.ul_tti_earliest_arrival = 0;
	timing_info.ul_dci_earliest_arrival = 0;


	pnf_nr_p7_pack_and_send_p7_message(pnf_p7, &(timing_info.header), sizeof(timing_info));

	pnf_p7->timing_info_ms_counter = 0;
}


void send_dummy_slot(pnf_p7_t* pnf_p7, uint16_t sfn, uint16_t slot)
{
  struct timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t);

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(sfn_sf:%d) t:%ld.%09ld\n", __FUNCTION__, NFAPI_SFNSF2DEC(sfn_sf), t.tv_sec, t.tv_nsec);

	if(pnf_p7->_public.tx_data_req_fn && pnf_p7->_public.dummy_slot.tx_data_req)
	{
		pnf_p7->_public.dummy_slot.tx_data_req->SFN = sfn;
		pnf_p7->_public.dummy_slot.tx_data_req->Slot = slot;
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy tx_req - enter\n");
		(pnf_p7->_public.tx_data_req_fn)(&pnf_p7->_public, pnf_p7->_public.dummy_slot.tx_data_req);
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy tx_req - exit\n");
	}
	if(pnf_p7->_public.dl_tti_req_fn && pnf_p7->_public.dummy_slot.dl_tti_req)
	{
		pnf_p7->_public.dummy_slot.dl_tti_req->SFN = sfn;
		pnf_p7->_public.dummy_slot.dl_tti_req->Slot = slot;
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy dl_config_req - enter\n");
		(pnf_p7->_public.dl_tti_req_fn)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_slot.dl_tti_req);
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy dl_config_req - exit\n");
	}
	if(pnf_p7->_public.ul_tti_req_fn && pnf_p7->_public.dummy_slot.ul_tti_req)
	{
		pnf_p7->_public.dummy_slot.ul_tti_req->SFN = sfn;
		pnf_p7->_public.dummy_slot.ul_tti_req->Slot = slot;
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy ul_config_req - enter\n");
		(pnf_p7->_public.ul_tti_req_fn)(NULL, &pnf_p7->_public, pnf_p7->_public.dummy_slot.ul_tti_req);
	}
	if(pnf_p7->_public.ul_dci_req_fn && pnf_p7->_public.dummy_slot.ul_dci_req)
	{
		pnf_p7->_public.dummy_slot.ul_dci_req->SFN = sfn;
		pnf_p7->_public.dummy_slot.ul_dci_req->Slot = slot;
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy hi_dci0 - enter\n");
		(pnf_p7->_public.ul_dci_req_fn)(NULL, &pnf_p7->_public, pnf_p7->_public.dummy_slot.ul_dci_req);
	}
	#if 0
	if(pnf_p7->_public.lbt_dl_config_req && pnf_p7->_public.dummy_subframe.lbt_dl_config_req) // TODO: Change later
	{
		pnf_p7->_public.dummy_slot.lbt_dl_config_req->sfn_sf = sfn;
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy lbt - enter\n");
		(pnf_p7->_public.lbt_dl_config_req)(&pnf_p7->_public, pnf_p7->_public.dummy_slot.lbt_dl_config_req);
	}
	#endif
}


void send_dummy_subframe(pnf_p7_t* pnf_p7, uint16_t sfn_sf)
{
  struct timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t);

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(sfn_sf:%d) t:%ld.%09ld\n", __FUNCTION__, NFAPI_SFNSF2DEC(sfn_sf), t.tv_sec, t.tv_nsec);

	if(pnf_p7->_public.tx_req && pnf_p7->_public.dummy_subframe.tx_req)
	{
		pnf_p7->_public.dummy_subframe.tx_req->sfn_sf = sfn_sf;
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy tx_req - enter\n");
		(pnf_p7->_public.tx_req)(&pnf_p7->_public, pnf_p7->_public.dummy_subframe.tx_req);
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy tx_req - exit\n");
	}
	if(pnf_p7->_public.dl_config_req && pnf_p7->_public.dummy_subframe.dl_config_req)
	{
		pnf_p7->_public.dummy_subframe.dl_config_req->sfn_sf = sfn_sf;
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy dl_config_req - enter\n");
		(pnf_p7->_public.dl_config_req)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_subframe.dl_config_req);
		//NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy dl_config_req - exit\n");
	}
	if(pnf_p7->_public.ul_config_req && pnf_p7->_public.dummy_subframe.ul_config_req)
	{
		pnf_p7->_public.dummy_subframe.ul_config_req->sfn_sf = sfn_sf;
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy ul_config_req - enter\n");
		(pnf_p7->_public.ul_config_req)(NULL, &pnf_p7->_public, pnf_p7->_public.dummy_subframe.ul_config_req);
	}
	if(pnf_p7->_public.hi_dci0_req && pnf_p7->_public.dummy_subframe.hi_dci0_req)
	{
		pnf_p7->_public.dummy_subframe.hi_dci0_req->sfn_sf = sfn_sf;
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy hi_dci0 - enter\n");
		(pnf_p7->_public.hi_dci0_req)(NULL, &pnf_p7->_public, pnf_p7->_public.dummy_subframe.hi_dci0_req);
	}
	if(pnf_p7->_public.lbt_dl_config_req && pnf_p7->_public.dummy_subframe.lbt_dl_config_req)
	{
		pnf_p7->_public.dummy_subframe.lbt_dl_config_req->sfn_sf = sfn_sf;
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Dummy lbt - enter\n");
		(pnf_p7->_public.lbt_dl_config_req)(&pnf_p7->_public, pnf_p7->_public.dummy_subframe.lbt_dl_config_req);
	}
}


int pnf_p7_slot_ind(pnf_p7_t* pnf_p7, uint16_t phy_id, uint16_t sfn, uint16_t slot)
{	
	//This function is aligned with rx sfn/slot

	// We could either send an event to the p7 thread have have it run the
	// subframe or we could handle it here and lock access to the subframe
	// buffers. If we do it on the p7 thread then we run the risk of blocking
	// on the udp send. 
	
	// todo : consider a more efficent lock mechasium
	//uint16_t NUM_SLOTS = 20;//10* 2^mu
	
	if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
		return -1;
	}

	// save the curren time, sfn and slot
	pnf_p7->slot_start_time_hr = pnf_get_current_time_hr();
	slot_ahead = 6;
	uint32_t sfn_slot_tx = sfnslot_add_slot(sfn, slot, slot_ahead);
	uint16_t sfn_tx = NFAPI_SFNSLOT2SFN(sfn_slot_tx);
	uint8_t slot_tx = NFAPI_SFNSLOT2SLOT(sfn_slot_tx);

	//We align the pnf_p7 sfn/slot with tx sfn/slot, and vnf is synced with pnf_p7 sfn/slot. This is so that the scheduler runs slot_ahead from rx thread.

	pnf_p7->sfn = sfn_tx;
	pnf_p7->slot = slot_tx; 

	uint32_t rx_slot_dec = NFAPI_SFNSLOT2DEC(sfn, slot);
	uint8_t buffer_index_rx = rx_slot_dec % 20; 

	uint32_t tx_slot_dec = NFAPI_SFNSLOT2DEC(sfn_tx,slot_tx);
	uint8_t buffer_index_tx = tx_slot_dec % 20;

	// If the subframe_buffer has been configured
	if(pnf_p7->_public.slot_buffer_size!= 0) // for now value is same as sf_buffer_size
	{

		// apply the shift to the incoming sfn_sf
		if(pnf_p7->slot_shift != 0) // see in vnf_build_send_dl_node_sync
		{
			uint16_t shifted_slot = slot + pnf_p7->slot_shift; 

			// adjust for wrap-around
			if(shifted_slot < 0)
				shifted_slot += NFAPI_MAX_SFNSLOTDEC;
			else if(shifted_slot > NFAPI_MAX_SFNSLOTDEC)
				shifted_slot -= NFAPI_MAX_SFNSLOTDEC;

	//		NFAPI_TRACE(NFAPI_TRACE_INFO, "Applying shift %d to sfn/slot (%d -> %d)\n", pnf_p7->sfn_slot_shift, NFAPI_SFNSF2DEC(sfn_slot), shifted_sfn_slot);
			slot = shifted_slot;

			//
			// DJP - why does the shift not apply to pnf_p7->sfn_sf???
			//

			pnf_p7->slot_shift = 0;
		}

		nfapi_pnf_p7_slot_buffer_t* rx_slot_buffer = &(pnf_p7->slot_buffer[buffer_index_rx]);

		nfapi_pnf_p7_slot_buffer_t* tx_slot_buffer = &(pnf_p7->slot_buffer[buffer_index_tx]);

                // if (0) NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() shift:%d slot_buffer->sfn_sf:%d tx_slot_buffer->sfn_slot:%d sfn_sf:%d subframe_buffer[buffer_index:%u dl_config_req:%p tx_req:%p] "
                //     "TX:sfn_sf:%d:tx_buffer_index:%d[dl_config_req:%p tx_req:%p]\n", 
                //     __FUNCTION__, 
                //     pnf_p7->slot_shift, 
                //     NFAPI_SFNSLOT2DEC(rx_slot_buffer->sfn, rx_slot_buffer->slot), 
                //     NFAPI_SFNSLOT2DEC(tx_slot_buffer->sfn, tx_slot_buffer->slot), 
                //        	slot_dec,    buffer_index_rx, rx_slot_buffer->dl_tti_req, rx_slot_buffer->tx_data_req, 
                //     	tx_slot_dec, buffer_index_tx, tx_slot_buffer->dl_tti_req, tx_slot_buffer->tx_data_req);
					//TODO: Change later if required

		// todo : how to handle the messages we don't have, send dummies for
		// now

		//printf("tx_subframe_buffer->sfn_sf:%d sfn_sf_tx:%d\n", tx_subframe_buffer->sfn_sf, sfn_sf_tx);
		//printf("subframe_buffer->sfn_sf:%d sfn_sf:%d\n", subframe_buffer->sfn_sf, sfn_sf);
		//printf("tx_slot_buff_sfn - %d, tx_slot_buf_slot - %d, sfn_tx = %d, sllot_tx - %d \n",tx_slot_buffer->sfn,tx_slot_buffer->slot,sfn_tx,slot_tx);
		// if(tx_slot_buffer->slot == slot_tx && tx_slot_buffer->sfn == sfn_tx)
		// {	
		
		//checking in the tx slot buffers to see if a p7 msg is present. todo: what if it's a mixed slot? 

		if(tx_slot_buffer->tx_data_req != 0 && tx_slot_buffer->tx_data_req->SFN == sfn_tx && tx_slot_buffer->tx_data_req->Slot == slot_tx)
		{
				
			if(pnf_p7->_public.tx_data_req_fn)
			{	
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "Calling tx_data_req_fn in SFN/slot %d.%d \n",sfn,slot);
				LOG_D(PHY, "Process tx_data SFN/slot %d.%d buffer index: %d \n",sfn_tx,slot_tx,buffer_index_tx);	
				(pnf_p7->_public.tx_data_req_fn)(&(pnf_p7->_public), tx_slot_buffer->tx_data_req);
			}
		}
		else 
		{
			// send dummy
			if(pnf_p7->_public.tx_data_req_fn && pnf_p7->_public.dummy_slot.tx_data_req)
			{
				pnf_p7->_public.dummy_slot.tx_data_req->SFN = sfn_tx;
				pnf_p7->_public.dummy_slot.tx_data_req->Slot = slot_tx; 
					
				(pnf_p7->_public.tx_data_req_fn)(&(pnf_p7->_public), pnf_p7->_public.dummy_slot.tx_data_req);
			}
		}
		 
		if(tx_slot_buffer->dl_tti_req != 0 && tx_slot_buffer->dl_tti_req->SFN == sfn_tx && tx_slot_buffer->dl_tti_req->Slot == slot_tx) 
		{
			if(pnf_p7->_public.dl_tti_req_fn)
			{
				LOG_D(PHY, "Process dl_tti SFN/slot %d.%d buffer index: %d \n",sfn_tx,slot_tx,buffer_index_tx);
				(pnf_p7->_public.dl_tti_req_fn)(NULL, &(pnf_p7->_public), tx_slot_buffer->dl_tti_req);
			}
		}
		else
		{
			// send dummy
			if(pnf_p7->_public.dl_tti_req_fn && pnf_p7->_public.dummy_slot.dl_tti_req)
			{   
				pnf_p7->_public.dummy_slot.dl_tti_req->SFN = sfn_tx;
				pnf_p7->_public.dummy_slot.dl_tti_req->Slot = slot_tx;
				(pnf_p7->_public.dl_tti_req_fn)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_slot.dl_tti_req);
			}
		}


		if(tx_slot_buffer->ul_dci_req!= 0 && tx_slot_buffer->ul_dci_req->SFN == sfn_tx && tx_slot_buffer->ul_dci_req->Slot == slot_tx)
		{
			if(pnf_p7->_public.ul_dci_req_fn)
			{   
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "Calling UL_dci_req_fn in SFN/slot %d.%d \n",sfn,slot);
				LOG_D(PHY, "Process ul_dci SFN/slot %d.%d buffer index: %d \n",sfn_tx,slot_tx,buffer_index_tx);
 				(pnf_p7->_public.ul_dci_req_fn)(NULL, &(pnf_p7->_public), tx_slot_buffer->ul_dci_req);
			}
		}
		else
		{
			//send dummy
			if(pnf_p7->_public.ul_dci_req_fn && pnf_p7->_public.dummy_slot.ul_dci_req)
			{
				pnf_p7->_public.dummy_slot.ul_dci_req->SFN = sfn_tx;
				pnf_p7->_public.dummy_slot.ul_dci_req->Slot = slot_tx;
				(pnf_p7->_public.ul_dci_req_fn)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_slot.ul_dci_req);
			}
		}

		//deallocate slot buffers after passing down the PDUs to PHY processing

		if(tx_slot_buffer->dl_tti_req != 0)
		{
			deallocate_nfapi_dl_tti_request(tx_slot_buffer->dl_tti_req, pnf_p7);
			tx_slot_buffer->dl_tti_req = 0;
			LOG_D(PHY,"SFN/slot %d.%d Buffer index : %d freed \n",sfn_tx,slot_tx,buffer_index_tx);
		}

		if(tx_slot_buffer->tx_data_req != 0)
		{
			deallocate_nfapi_tx_data_request(tx_slot_buffer->tx_data_req, pnf_p7);
			tx_slot_buffer->tx_data_req = 0;
		}

		if(tx_slot_buffer->ul_dci_req != 0)
		{
			deallocate_nfapi_ul_dci_request(tx_slot_buffer->ul_dci_req, pnf_p7);
			tx_slot_buffer->ul_dci_req = 0;
		}


		//checking in the rx slot buffers to see if a p7 msg is present.

		if(rx_slot_buffer->ul_tti_req != 0 && rx_slot_buffer->ul_tti_req->SFN == sfn && rx_slot_buffer->ul_tti_req->Slot == slot)
		{
			if(pnf_p7->_public.ul_tti_req_fn)
			{ 	
				//NFAPI_TRACE(NFAPI_TRACE_INFO, "Calling UL_tti_req_fn in SFN/slot %d.%d \n",sfn,slot);
				(pnf_p7->_public.ul_tti_req_fn)(NULL, &(pnf_p7->_public), rx_slot_buffer->ul_tti_req);
			}
		}
		else
		{
			// send dummy
			if(pnf_p7->_public.ul_tti_req_fn && pnf_p7->_public.dummy_slot.ul_tti_req)
			{
				pnf_p7->_public.dummy_slot.ul_tti_req->SFN = sfn;
				pnf_p7->_public.dummy_slot.ul_tti_req->Slot = slot;
				LOG_D(PHY, "Process ul_tti SFN/slot %d.%d buffer index: %d \n",sfn,slot,buffer_index_rx);
				(pnf_p7->_public.ul_tti_req_fn)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_slot.ul_tti_req);
			}
		}
		if(rx_slot_buffer->ul_tti_req != 0)
		{
			deallocate_nfapi_ul_tti_request(rx_slot_buffer->ul_tti_req, pnf_p7);
			rx_slot_buffer->ul_tti_req = 0;

		}

		//reset slot buffer 

		if ( rx_slot_buffer->dl_tti_req == 0 &&
			 rx_slot_buffer->tx_data_req == 0 && 
			 rx_slot_buffer->ul_tti_req == 0)
		{
			memset(&(pnf_p7->slot_buffer[buffer_index_rx]), 0, sizeof(nfapi_pnf_p7_slot_buffer_t));
			pnf_p7->slot_buffer[buffer_index_rx].sfn = -1;
			pnf_p7->slot_buffer[buffer_index_rx].slot = -1;
		}

		//printf("pnf_p7->_public.timing_info_mode_periodic:%d pnf_p7->timing_info_period_counter:%d pnf_p7->_public.timing_info_period:%d\n", pnf_p7->_public.timing_info_mode_periodic, pnf_p7->timing_info_period_counter, pnf_p7->_public.timing_info_period);
		//printf("pnf_p7->_public.timing_info_mode_aperiodic:%d pnf_p7->timing_info_aperiodic_send:%d\n", pnf_p7->_public.timing_info_mode_aperiodic, pnf_p7->timing_info_aperiodic_send);
		//printf("pnf_p7->timing_info_ms_counter:%d\n", pnf_p7->timing_info_ms_counter);

		//send the periodic timing info if configured
		if(pnf_p7->_public.timing_info_mode_periodic && (pnf_p7->timing_info_period_counter++) == pnf_p7->_public.timing_info_period)
		{
			pnf_nr_pack_and_send_timing_info(pnf_p7);

			pnf_p7->timing_info_period_counter = 0;
		}
		else if(pnf_p7->_public.timing_info_mode_aperiodic && pnf_p7->timing_info_aperiodic_send)
		{
			pnf_nr_pack_and_send_timing_info(pnf_p7);

			pnf_p7->timing_info_aperiodic_send = 0;
		}
		else
		{
			pnf_p7->timing_info_ms_counter++;
		}

	}

	if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
		return -1;
	}

	return 0;
}



int pnf_p7_subframe_ind(pnf_p7_t* pnf_p7, uint16_t phy_id, uint16_t sfn_sf)
{
	// We could either send an event to the p7 thread have have it run the
	// subframe or we could handle it here and lock access to the subframe
	// buffers. If we do it on the p7 thread then we run the risk of blocking
	// on the udp send. 
	//
	// todo : start a timer to give us more of the 1 ms tick before send back
	// the frame
	
	// todo : consider a more efficent lock mechasium
	if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
		return -1;
	}

#if 1
	// save the curren time and sfn_sf
	pnf_p7->sf_start_time_hr = pnf_get_current_time_hr();
	pnf_p7->sfn_sf = sfn_sf;

	uint32_t sfn_sf_tx = sfnsf_add_sf(sfn_sf, sf_ahead);
	uint32_t tx_sfn_sf_dec = NFAPI_SFNSF2DEC(sfn_sf_tx);

	// If the subframe_buffer has been configured
	if(pnf_p7->_public.subframe_buffer_size != 0)
	{

		// apply the shift to the incoming sfn_sf
		if(pnf_p7->sfn_sf_shift != 0)
		{
			int32_t sfn_sf_dec = NFAPI_SFNSF2DEC(sfn_sf);

			int32_t shifted_sfn_sf = sfn_sf_dec += pnf_p7->sfn_sf_shift;

			// adjust for wrap-around
			if(shifted_sfn_sf < 0)
				shifted_sfn_sf += NFAPI_MAX_SFNSFDEC;
			else if(shifted_sfn_sf > NFAPI_MAX_SFNSFDEC)
				shifted_sfn_sf -= NFAPI_MAX_SFNSFDEC;

			NFAPI_TRACE(NFAPI_TRACE_INFO, "Applying shift %d to sfn/sf (%d -> %d)\n", pnf_p7->sfn_sf_shift, NFAPI_SFNSF2DEC(sfn_sf), shifted_sfn_sf);
			sfn_sf = shifted_sfn_sf;

                        //
                        // DJP - why does the shift not apply to pnf_p7->sfn_sf???
                        //

			pnf_p7->sfn_sf_shift = 0;
		}

		uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(sfn_sf);
		uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

		nfapi_pnf_p7_subframe_buffer_t* subframe_buffer = &(pnf_p7->subframe_buffer[buffer_index]);

		uint8_t tx_buffer_index = tx_sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;
		nfapi_pnf_p7_subframe_buffer_t* tx_subframe_buffer = &(pnf_p7->subframe_buffer[tx_buffer_index]);

                //printf("sfn_sf_dec:%d tx_sfn_sf_dec:%d\n", sfn_sf_dec, tx_sfn_sf_dec);

                if (0) NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() shift:%d subframe_buffer->sfn_sf:%d tx_subframe_buffer->sfn_sf:%d sfn_sf:%d subframe_buffer[buffer_index:%u dl_config_req:%p tx_req:%p] "
                    "TX:sfn_sf:%d:tx_buffer_index:%d[dl_config_req:%p tx_req:%p]\n", 
                    __FUNCTION__, 
                    pnf_p7->sfn_sf_shift, 
                    NFAPI_SFNSF2DEC(subframe_buffer->sfn_sf), 
                    NFAPI_SFNSF2DEC(tx_subframe_buffer->sfn_sf), 
                       sfn_sf_dec,    buffer_index,    subframe_buffer->dl_config_req,    subframe_buffer->tx_req, 
                    tx_sfn_sf_dec, tx_buffer_index, tx_subframe_buffer->dl_config_req, tx_subframe_buffer->tx_req);

		// if the subframe buffer sfn sf is set then we have atlease 1 message
		// from the vnf. 
		// todo : how to handle the messages we don't have, send dummies for
		// now

                //printf("tx_subframe_buffer->sfn_sf:%d sfn_sf_tx:%d\n", tx_subframe_buffer->sfn_sf, sfn_sf_tx);
                //printf("subframe_buffer->sfn_sf:%d sfn_sf:%d\n", subframe_buffer->sfn_sf, sfn_sf);
		if(tx_subframe_buffer->sfn_sf == sfn_sf_tx)
		{
			if(tx_subframe_buffer->tx_req != 0)
			{
				if(pnf_p7->_public.tx_req)
					(pnf_p7->_public.tx_req)(&(pnf_p7->_public), tx_subframe_buffer->tx_req);

				//deallocate_nfapi_tx_request(subframe_buffer->tx_req, pnf_p7);
			}
			else
			{
				// send dummy
				if(pnf_p7->_public.tx_req && pnf_p7->_public.dummy_subframe.tx_req)
				{
					pnf_p7->_public.dummy_subframe.tx_req->sfn_sf = sfn_sf_tx;
					(pnf_p7->_public.tx_req)(&(pnf_p7->_public), pnf_p7->_public.dummy_subframe.tx_req);
				}
			}

			if(tx_subframe_buffer->dl_config_req != 0)
			{
				if(pnf_p7->_public.dl_config_req)
					(pnf_p7->_public.dl_config_req)(NULL, &(pnf_p7->_public), tx_subframe_buffer->dl_config_req);

				//deallocate_nfapi_dl_config_request(subframe_buffer->dl_config_req, pnf_p7);
			}
			else
			{
				// send dummy
				if(pnf_p7->_public.dl_config_req && pnf_p7->_public.dummy_subframe.dl_config_req)
				{
					pnf_p7->_public.dummy_subframe.dl_config_req->sfn_sf = sfn_sf_tx;
					(pnf_p7->_public.dl_config_req)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_subframe.dl_config_req);
				}
			}

			if(tx_subframe_buffer->hi_dci0_req != 0)
			{
				if(pnf_p7->_public.hi_dci0_req)
					(pnf_p7->_public.hi_dci0_req)(NULL, &(pnf_p7->_public), tx_subframe_buffer->hi_dci0_req);

				//deallocate_nfapi_hi_dci0_request(subframe_buffer->hi_dci0_req, pnf_p7);
			}
			else
			{
				//send dummy
				if(pnf_p7->_public.hi_dci0_req && pnf_p7->_public.dummy_subframe.hi_dci0_req)
				{
					pnf_p7->_public.dummy_subframe.hi_dci0_req->sfn_sf = sfn_sf_tx;
					(pnf_p7->_public.hi_dci0_req)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_subframe.hi_dci0_req);
				}
			}

			if(tx_subframe_buffer->ue_release_req != 0)
			{
				if(pnf_p7->_public.ue_release_req)
					(pnf_p7->_public.ue_release_req)(&(pnf_p7->_public), tx_subframe_buffer->ue_release_req);
			}
			else
			{
				//send dummy
				if(pnf_p7->_public.ue_release_req && pnf_p7->_public.dummy_subframe.ue_release_req)
				{
					pnf_p7->_public.dummy_subframe.ue_release_req->sfn_sf = sfn_sf_tx;
					(pnf_p7->_public.ue_release_req)(&(pnf_p7->_public), pnf_p7->_public.dummy_subframe.ue_release_req);
				}
			}

                        if(tx_subframe_buffer->dl_config_req != 0)
                        {
                          deallocate_nfapi_dl_config_request(tx_subframe_buffer->dl_config_req, pnf_p7);
                          tx_subframe_buffer->dl_config_req = 0;
                        }
			if(tx_subframe_buffer->tx_req != 0)
                        {
                          deallocate_nfapi_tx_request(tx_subframe_buffer->tx_req, pnf_p7);
                          tx_subframe_buffer->tx_req = 0;
                        }
                        if(tx_subframe_buffer->hi_dci0_req != 0)
                        {
                          deallocate_nfapi_hi_dci0_request(tx_subframe_buffer->hi_dci0_req, pnf_p7);
                          tx_subframe_buffer->hi_dci0_req = 0;
                        }
                        if(tx_subframe_buffer->ue_release_req != 0){
                            deallocate_nfapi_ue_release_request(tx_subframe_buffer->ue_release_req, pnf_p7);
                            tx_subframe_buffer->ue_release_req = 0;
                        }
                }
		else
		{
                  // If we ever need to "send" a dummy ul_config this won't work!!!
                  send_dummy_subframe(pnf_p7, sfn_sf_tx);
		}

                if(subframe_buffer->sfn_sf == sfn_sf)
		{

			if(subframe_buffer->ul_config_req != 0)
			{
				if(pnf_p7->_public.ul_config_req)
					(pnf_p7->_public.ul_config_req)(NULL, &(pnf_p7->_public), subframe_buffer->ul_config_req);

				//deallocate_nfapi_ul_config_request(subframe_buffer->ul_config_req, pnf_p7);
			}
			else
			{
				// send dummy
				if(pnf_p7->_public.ul_config_req && pnf_p7->_public.dummy_subframe.ul_config_req)
				{
					pnf_p7->_public.dummy_subframe.ul_config_req->sfn_sf = sfn_sf;
					(pnf_p7->_public.ul_config_req)(NULL, &(pnf_p7->_public), pnf_p7->_public.dummy_subframe.ul_config_req);
				}
			}

			if(subframe_buffer->lbt_dl_config_req != 0)
			{
				if(pnf_p7->_public.lbt_dl_config_req)
					(pnf_p7->_public.lbt_dl_config_req)(&(pnf_p7->_public), subframe_buffer->lbt_dl_config_req);

				//deallocate_nfapi_lbt_dl_config_request(subframe_buffer->lbt_dl_config_req, pnf_p7);
			}
			else
			{
				// send dummy
				if(pnf_p7->_public.lbt_dl_config_req && pnf_p7->_public.dummy_subframe.lbt_dl_config_req)
				{
					pnf_p7->_public.dummy_subframe.lbt_dl_config_req->sfn_sf = sfn_sf;
					(pnf_p7->_public.lbt_dl_config_req)(&(pnf_p7->_public), pnf_p7->_public.dummy_subframe.lbt_dl_config_req);
				}

			}

                        //if(subframe_buffer->dl_config_req != 0)
                          //deallocate_nfapi_dl_config_request(subframe_buffer->dl_config_req, pnf_p7);
			//if(subframe_buffer->tx_req != 0)
                          //deallocate_nfapi_tx_request(subframe_buffer->tx_req, pnf_p7);
                        if(subframe_buffer->ul_config_req != 0)
                        {
                          deallocate_nfapi_ul_config_request(subframe_buffer->ul_config_req, pnf_p7);
                          subframe_buffer->ul_config_req = 0;

                        }
                        //if(subframe_buffer->hi_dci0_req != 0)
                          //deallocate_nfapi_hi_dci0_request(subframe_buffer->hi_dci0_req, pnf_p7);
			if(subframe_buffer->lbt_dl_config_req != 0)
                        {
                          deallocate_nfapi_lbt_dl_config_request(subframe_buffer->lbt_dl_config_req, pnf_p7);
                          subframe_buffer->lbt_dl_config_req = 0;
                        }
                } // sfn_sf match

                if (subframe_buffer->dl_config_req == 0 && subframe_buffer->tx_req == 0 && subframe_buffer->ul_config_req == 0 && subframe_buffer->lbt_dl_config_req == 0 && subframe_buffer->ue_release_req == 0)
                {
                  memset(&(pnf_p7->subframe_buffer[buffer_index]), 0, sizeof(nfapi_pnf_p7_subframe_buffer_t));
                  pnf_p7->subframe_buffer[buffer_index].sfn_sf = -1;
		}

                //printf("pnf_p7->_public.timing_info_mode_periodic:%d pnf_p7->timing_info_period_counter:%d pnf_p7->_public.timing_info_period:%d\n", pnf_p7->_public.timing_info_mode_periodic, pnf_p7->timing_info_period_counter, pnf_p7->_public.timing_info_period);
                //printf("pnf_p7->_public.timing_info_mode_aperiodic:%d pnf_p7->timing_info_aperiodic_send:%d\n", pnf_p7->_public.timing_info_mode_aperiodic, pnf_p7->timing_info_aperiodic_send);
                //printf("pnf_p7->timing_info_ms_counter:%d\n", pnf_p7->timing_info_ms_counter);

		// send the periodic timing info if configured
		if(pnf_p7->_public.timing_info_mode_periodic && (pnf_p7->timing_info_period_counter++) == pnf_p7->_public.timing_info_period)
		{
			pnf_pack_and_send_timing_info(pnf_p7);

			pnf_p7->timing_info_period_counter = 0;
		}
		else if(pnf_p7->_public.timing_info_mode_aperiodic && pnf_p7->timing_info_aperiodic_send)
		{
			pnf_pack_and_send_timing_info(pnf_p7);

			pnf_p7->timing_info_aperiodic_send = 0;
		}
		else
		{
			pnf_p7->timing_info_ms_counter++;
		}
	}
	else
	{
		//send_dummy_subframe(pnf_p7, sfn_sf_tx);
	}


        //printf("pnf_p7->tick:%d\n", pnf_p7->tick);
	if(pnf_p7->tick == 1000)
	{

		NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF P7:%d] (ONTIME/LATE) DL:(%d/%d) UL:(%d/%d) HI:(%d/%d) TX:(%d/%d)\n", pnf_p7->_public.phy_id,
					pnf_p7->stats.dl_conf_ontime, pnf_p7->stats.dl_conf_late, 
					pnf_p7->stats.ul_conf_ontime, pnf_p7->stats.ul_conf_late, 
					pnf_p7->stats.hi_dci0_ontime, pnf_p7->stats.hi_dci0_late, 
					pnf_p7->stats.tx_ontime, pnf_p7->stats.tx_late);
		pnf_p7->tick = 0;
		memset(&pnf_p7->stats, 0, sizeof(pnf_p7->stats));
	}
	pnf_p7->tick++;
#endif

	if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
		return -1;
	}

	return 0;
}



// return 1 if in window
// return 0 if out of window

uint8_t is_nr_p7_request_in_window(uint16_t sfn,uint16_t slot, const char* name, pnf_p7_t* phy)
{
	uint32_t recv_sfn_slot_dec = NFAPI_SFNSLOT2DEC(sfn,slot);
	uint32_t current_sfn_slot_dec = NFAPI_SFNSLOT2DEC(phy->sfn,phy->slot);
	//printf("p7_msg_sfn: %d, p7_msg_slot: %d, phy_sfn:%d , phy_slot:%d \n",sfn,slot,phy->sfn,phy->slot);
	uint8_t in_window = 0;
	uint8_t timing_window = phy->_public.slot_buffer_size;

	// if(recv_sfn_slot_dec <= current_sfn_slot_dec)
	// {
	// 	// Need to check for wrap in window
	// 	if(((recv_sfn_slot_dec + timing_window) % NFAPI_MAX_SFNSLOTDEC) < recv_sfn_slot_dec)
	// 	{
	// 		if(current_sfn_slot_dec > ((recv_sfn_slot_dec + timing_window) % NFAPI_MAX_SFNSLOTDEC))
	// 		{
	// 			// out of window
	// 			NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is late %d (with wrap)\n", current_sfn_slot_dec, name, recv_sfn_slot_dec);
	// 		}
	// 		else
	// 		{
	// 			// ok
	// 			//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d (with wrap)\n", current_sfn_sf_dec, name, recv_sfn_sf_dec);
	// 			in_window = 1;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		if((current_sfn_slot_dec - recv_sfn_slot_dec) <= timing_window)
	// 			{
	// 				// in window
	// 				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d\n", current_sfn_slot_dec, name, recv_sfn_slot_dec);
	// 				in_window = 1;
	// 			}
	// 		//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in late %d (delta:%d)\n", current_sfn_slot_dec, name, recv_sfn_slot_dec, (current_sfn_slot_dec - recv_sfn_slot_dec));
	// 	}

	// }
	// else
	// {
	// 	// Need to check it is in window
	// 	if((recv_sfn_slot_dec - current_sfn_slot_dec) <= timing_window)
	// 	{
	// 		// in window
	// 		//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d\n", current_sfn_sf_dec, name, recv_sfn_sf_dec);
	// 		in_window = 1;
	// 	}
	// 	else
	// 	{
	// 		// too far in the future
	// 		NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is out of window %d (delta:%d) [max:%d]\n", current_sfn_slot_dec, name, recv_sfn_slot_dec,  (recv_sfn_slot_dec - current_sfn_slot_dec), timing_window);
	// 	}

	// }
	if(current_sfn_slot_dec <= recv_sfn_slot_dec + timing_window){
		in_window = 1;
		//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d\n", current_sfn_slot_dec, name, recv_sfn_slot_dec);
	}
	else if(current_sfn_slot_dec + NFAPI_MAX_SFNSLOTDEC <= recv_sfn_slot_dec + timing_window){ //checking for wrap
		in_window = 1;
		//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d\n", current_sfn_slot_dec, name, recv_sfn_slot_dec);
	}
  
	else
	{ 	
		
		NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is out of window %d (delta:%d) [max:%d]\n", current_sfn_slot_dec, name, recv_sfn_slot_dec,  (current_sfn_slot_dec - recv_sfn_slot_dec), timing_window);
		
	}//Need to add more cases
	

	return in_window;
}



uint8_t is_p7_request_in_window(uint16_t sfnsf, const char* name, pnf_p7_t* phy)
{
	uint32_t recv_sfn_sf_dec = NFAPI_SFNSF2DEC(sfnsf);
	uint32_t current_sfn_sf_dec = NFAPI_SFNSF2DEC(phy->sfn_sf);

	uint8_t in_window = 0;
	uint8_t timing_window = phy->_public.subframe_buffer_size;

	if(recv_sfn_sf_dec <= current_sfn_sf_dec)
	{
		// Need to check for wrap in window
		if(((current_sfn_sf_dec + timing_window) % NFAPI_MAX_SFNSFDEC) < current_sfn_sf_dec)
		{
			if(recv_sfn_sf_dec > ((current_sfn_sf_dec + timing_window) % NFAPI_MAX_SFNSFDEC))
			{
				// out of window
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is late %d (with wrap)\n", current_sfn_sf_dec, name, recv_sfn_sf_dec);
			}
			else
			{
				// ok
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d (with wrap)\n", current_sfn_sf_dec, name, recv_sfn_sf_dec);
				in_window = 1;
			}
		}
		else
		{
			// too late
			NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in late %d (delta:%d)\n", current_sfn_sf_dec, name, recv_sfn_sf_dec, (current_sfn_sf_dec - recv_sfn_sf_dec));
		}

	}
	else
	{
		// Need to check it is in window
		if((recv_sfn_sf_dec - current_sfn_sf_dec) <= timing_window)
		{
			// in window
			//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is in window %d\n", current_sfn_sf_dec, name, recv_sfn_sf_dec);
			in_window = 1;
		}
		else
		{
			// too far in the future
			NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] %s is out of window %d (delta:%d) [max:%d]\n", current_sfn_sf_dec, name, recv_sfn_sf_dec,  (recv_sfn_sf_dec - current_sfn_sf_dec), timing_window);
		}

	}

	return in_window;
}


// P7 messages 
void pnf_handle_dl_tti_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "DL_CONFIG.req Received\n");
	nfapi_nr_dl_tti_request_t* req  = allocate_nfapi_dl_tti_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_dl_tti_request structure\n");
		return;
	}
	int unpack_result = nfapi_nr_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_nr_dl_tti_request_t), &(pnf_p7->_public.codec_config));

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

        if(is_nr_p7_request_in_window(req->SFN,req->Slot, "dl_tti_request", pnf_p7))
            {
                uint32_t sfn_slot_dec = NFAPI_SFNSLOT2DEC(req->SFN,req->Slot);
                uint8_t buffer_index = sfn_slot_dec % 20;

                struct timespec t;
                clock_gettime(CLOCK_MONOTONIC, &t);

                NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE DL_TTI_REQ current tx sfn/slot:%d.%d p7 msg sfn/slot: %d.%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, pnf_p7->sfn,pnf_p7->slot, req->SFN, req->Slot, buffer_index);

			// if there is already an dl_tti_req make sure we free it.
			if(pnf_p7->slot_buffer[buffer_index].dl_tti_req != 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "%s() is_nr_p7_request_in_window()=TRUE buffer_index occupied - free it first sfn_slot:%d buffer_index:%d\n", __FUNCTION__, NFAPI_SFNSLOT2DEC(req->SFN,req->Slot), buffer_index);
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing dl_config_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));
				deallocate_nfapi_dl_tti_request(pnf_p7->slot_buffer[buffer_index].dl_tti_req, pnf_p7);
			}

			// filling dl_tti_request in slot buffer
			pnf_p7->slot_buffer[buffer_index].sfn = req->SFN;
			pnf_p7->slot_buffer[buffer_index].slot = req->Slot;
			pnf_p7->slot_buffer[buffer_index].dl_tti_req = req;

			pnf_p7->stats.dl_tti_ontime++;
		}
		else
		{
			//NFAPI_TRACE(NFAPI_TRACE_NOTE, "NOT storing dl_config_req SFN/SF %d\n", req->sfn_sf);
			deallocate_nfapi_dl_tti_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.dl_tti_late++;
		} 
		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack dl_tti_req");
		deallocate_nfapi_dl_tti_request(req, pnf_p7);
	}
}


void pnf_handle_dl_config_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "DL_CONFIG.req Received\n");

	nfapi_dl_config_request_t* req  = allocate_nfapi_dl_config_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_dl_config_request structure\n");
		return;
	}

	int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_dl_config_request_t), &(pnf_p7->_public.codec_config));

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

                if (
                    0 && 
                    (NFAPI_SFNSF2DEC(req->sfn_sf) % 100 ==0 ||
                     NFAPI_SFNSF2DEC(req->sfn_sf) % 105 ==0 
                    )
                )
                  NFAPI_TRACE(NFAPI_TRACE_INFO, "DL_CONFIG.req sfn_sf:%d pdcch:%u dci:%u pdu:%u pdsch_rnti:%u pcfich:%u\n", 
                      NFAPI_SFNSF2DEC(req->sfn_sf),
                      req->dl_config_request_body.number_pdcch_ofdm_symbols,
                      req->dl_config_request_body.number_dci,
                      req->dl_config_request_body.number_pdu,
                      req->dl_config_request_body.number_pdsch_rnti,
                      req->dl_config_request_body.transmission_power_pcfich
                      );

                if(is_p7_request_in_window(req->sfn_sf, "dl_config_request", pnf_p7))
                {
                  uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(req->sfn_sf);
                  uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

                        struct timespec t;
                        clock_gettime(CLOCK_MONOTONIC, &t);

                  NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE DL_CONFIG_REQ sfn_sf:%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, sfn_sf_dec, buffer_index);

			// if there is already an dl_config_req make sure we free it.
			if(pnf_p7->subframe_buffer[buffer_index].dl_config_req != 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "%s() is_p7_request_in_window()=TRUE buffer_index occupied - free it first sfn_sf:%d buffer_index:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), buffer_index);
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing dl_config_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));
				deallocate_nfapi_dl_config_request(pnf_p7->subframe_buffer[buffer_index].dl_config_req, pnf_p7);
			}

			// saving dl_config_request in subframe buffer
			pnf_p7->subframe_buffer[buffer_index].sfn_sf = req->sfn_sf;
			pnf_p7->subframe_buffer[buffer_index].dl_config_req = req;

			pnf_p7->stats.dl_conf_ontime++;
			
		}
		else
		{
			//NFAPI_TRACE(NFAPI_TRACE_NOTE, "NOT storing dl_config_req SFN/SF %d\n", req->sfn_sf);
			deallocate_nfapi_dl_config_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.dl_conf_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack dl_config_req");
		deallocate_nfapi_dl_config_request(req, pnf_p7);
	}
}


void pnf_handle_ul_tti_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "UL_CONFIG.req Received\n");

	nfapi_nr_ul_tti_request_t* req  = allocate_nfapi_ul_tti_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_ul_tti_request structure\n");
		return;
	}

	int unpack_result = nfapi_nr_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_nr_ul_tti_request_t), &(pnf_p7->_public.codec_config));

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_nr_p7_request_in_window(req->SFN,req->Slot, "ul_tti_request", pnf_p7))
		{
			uint32_t sfn_slot_dec = NFAPI_SFNSLOT2DEC(req->SFN,req->Slot);
			uint8_t buffer_index = (sfn_slot_dec % 20);

                        struct timespec t;
                        clock_gettime(CLOCK_MONOTONIC, &t);

                        NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE UL_TTI_REQ current tx sfn/slot:%d.%d p7 msg sfn/slot: %d.%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, pnf_p7->sfn,pnf_p7->slot, req->SFN, req->Slot, buffer_index);

			if(pnf_p7->slot_buffer[buffer_index].ul_tti_req != 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing ul_config_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_ul_tti_request(pnf_p7->slot_buffer[buffer_index].ul_tti_req, pnf_p7);
			}
			
			//filling slot buffer

			pnf_p7->slot_buffer[buffer_index].sfn = req->SFN;
			pnf_p7->slot_buffer[buffer_index].slot = req->Slot;
			pnf_p7->slot_buffer[buffer_index].ul_tti_req = req;
			
			pnf_p7->stats.ul_tti_ontime++;
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] NOT storing ul_tti_req OUTSIDE OF TRANSMIT BUFFER WINDOW SFN/SLOT %d\n", NFAPI_SFNSLOT2DEC(pnf_p7->sfn,pnf_p7->slot), NFAPI_SFNSLOT2DEC(req->SFN,req->Slot));
			deallocate_nfapi_ul_tti_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.ul_tti_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack ul_tti_req\n");
		deallocate_nfapi_ul_tti_request(req, pnf_p7);
	}
}

void pnf_handle_ul_config_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "UL_CONFIG.req Received\n");

	nfapi_ul_config_request_t* req  = allocate_nfapi_ul_config_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_ul_config_request structure\n");
		return;
	}

	int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_ul_config_request_t), &(pnf_p7->_public.codec_config));

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_p7_request_in_window(req->sfn_sf, "ul_config_request", pnf_p7))
		{
			uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(req->sfn_sf);
			uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

                        struct timespec t;
                        clock_gettime(CLOCK_MONOTONIC, &t);

                        NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE UL_CONFIG_REQ sfn_sf:%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, sfn_sf_dec, buffer_index);

			if(pnf_p7->subframe_buffer[buffer_index].ul_config_req != 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing ul_config_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_ul_config_request(pnf_p7->subframe_buffer[buffer_index].ul_config_req, pnf_p7);
			}

			pnf_p7->subframe_buffer[buffer_index].sfn_sf = req->sfn_sf;
			pnf_p7->subframe_buffer[buffer_index].ul_config_req = req;
			
			pnf_p7->stats.ul_conf_ontime++;
		}
		else
		{
			NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] NOT storing ul_config_req OUTSIDE OF TRANSMIT BUFFER WINDOW SFN/SF %d\n", NFAPI_SFNSF2DEC(pnf_p7->sfn_sf), NFAPI_SFNSF2DEC(req->sfn_sf));
			deallocate_nfapi_ul_config_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.ul_conf_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack ul_config_req\n");
		deallocate_nfapi_ul_config_request(req, pnf_p7);
	}
}


void pnf_handle_ul_dci_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "HI_DCI0.req Received\n");

	nfapi_nr_ul_dci_request_t* req  = allocate_nfapi_ul_dci_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to allocate nfapi_ul_dci_request structure\n");
		return;
	}

	int unpack_result = nfapi_nr_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_nr_ul_dci_request_t), &pnf_p7->_public.codec_config);

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_nr_p7_request_in_window(req->SFN,req->Slot,"ul_dci_request", pnf_p7))
		{
			uint32_t sfn_slot_dec = NFAPI_SFNSLOT2DEC(req->SFN,req->Slot);
			uint8_t buffer_index = sfn_slot_dec % 20;

			if(pnf_p7->slot_buffer[buffer_index].ul_dci_req!= 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing hi_dci0_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_ul_dci_request(pnf_p7->slot_buffer[buffer_index].ul_dci_req, pnf_p7);
			}

			pnf_p7->slot_buffer[buffer_index].sfn = req->SFN;
			pnf_p7->slot_buffer[buffer_index].ul_dci_req = req;

			pnf_p7->stats.ul_dci_ontime++;
			
		}
		else
		{
			//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] NOT storing hi_dci0_req SFN/SF %d/%d\n", pMyPhyInfo->sfnSf, SFNSF2SFN(req->sfn_sf), SFNSF2SF(req->sfn_sf));
			deallocate_nfapi_ul_dci_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.ul_dci_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack UL DCI req\n");
		deallocate_nfapi_ul_dci_request(req, pnf_p7);
	}
}



void pnf_handle_hi_dci0_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)

{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "HI_DCI0.req Received\n");

	nfapi_hi_dci0_request_t* req  = allocate_nfapi_hi_dci0_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_hi_dci0_request structure\n");
		return;
	}

	int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_hi_dci0_request_t), &pnf_p7->_public.codec_config);

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_p7_request_in_window(req->sfn_sf, "hi_dci0_request", pnf_p7))
		{
			uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(req->sfn_sf);
			uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

			if(pnf_p7->subframe_buffer[buffer_index].hi_dci0_req!= 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing hi_dci0_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_hi_dci0_request(pnf_p7->subframe_buffer[buffer_index].hi_dci0_req, pnf_p7);
			}

			pnf_p7->subframe_buffer[buffer_index].sfn_sf = req->sfn_sf;
			pnf_p7->subframe_buffer[buffer_index].hi_dci0_req = req;

			pnf_p7->stats.hi_dci0_ontime++;
			
		}
		else
		{
			//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] NOT storing hi_dci0_req SFN/SF %d/%d\n", pMyPhyInfo->sfnSf, SFNSF2SFN(req->sfn_sf), SFNSF2SF(req->sfn_sf));
			deallocate_nfapi_hi_dci0_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.hi_dci0_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack hi_dci0_req\n");
		deallocate_nfapi_hi_dci0_request(req, pnf_p7);
	}
}


void pnf_handle_tx_data_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "TX.req Received\n");
	
	nfapi_nr_tx_data_request_t* req = allocate_nfapi_tx_data_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_tx_request structure\n");
		return;
	}

	int unpack_result = nfapi_nr_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_nr_tx_data_request_t), &pnf_p7->_public.codec_config);
	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_nr_p7_request_in_window(req->SFN, req->Slot,"tx_request", pnf_p7))
		{
			uint32_t sfn_slot_dec = NFAPI_SFNSLOT2DEC(req->SFN,req->Slot);
			uint8_t buffer_index = sfn_slot_dec % 20;

                        struct timespec t;
                        clock_gettime(CLOCK_MONOTONIC, &t);

                        //NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE TX_DATA_REQ sfn_sf:%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, sfn_slot_dec, buffer_index);
#if 0
                        if (0 && NFAPI_SFNSF2DEC(req->sfn_sf)%100==0) NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() TX_REQ.req sfn_sf:%d pdus:%d - TX_REQ is within window\n",
                            __FUNCTION__,
                            NFAPI_SFNSF2DEC(req->sfn_sf),
                            req->tx_request_body.number_of_pdus);
#endif

			if(pnf_p7->slot_buffer[buffer_index].tx_data_req != 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing tx_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_tx_data_request(pnf_p7->slot_buffer[buffer_index].tx_data_req, pnf_p7);
			}

			pnf_p7->slot_buffer[buffer_index].sfn = req->SFN;
			pnf_p7->slot_buffer[buffer_index].slot = req->Slot;
			pnf_p7->slot_buffer[buffer_index].tx_data_req = req;

			pnf_p7->stats.tx_data_ontime++;
		}
		else
		{
                  NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() TX_DATA_REQUEST Request is outside of window REQ:SFN_SLOT:%d CURR:SFN_SLOT:%d\n", __FUNCTION__, NFAPI_SFNSLOT2DEC(req->SFN,req->Slot), NFAPI_SFNSLOT2DEC(pnf_p7->sfn,pnf_p7->slot));

			deallocate_nfapi_tx_data_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.tx_data_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		deallocate_nfapi_tx_data_request(req, pnf_p7);
	}
}



void pnf_handle_tx_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	//NFAPI_TRACE(NFAPI_TRACE_INFO, "TX.req Received\n");
	
	nfapi_tx_request_t* req = allocate_nfapi_tx_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_tx_request structure\n");
		return;
	}

	int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_tx_request_t), &pnf_p7->_public.codec_config);
	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_p7_request_in_window(req->sfn_sf, "tx_request", pnf_p7))
		{
			uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(req->sfn_sf);
			uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

                        struct timespec t;
                        clock_gettime(CLOCK_MONOTONIC, &t);

                        NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE TX_REQ sfn_sf:%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, sfn_sf_dec, buffer_index);

                        if (0 && NFAPI_SFNSF2DEC(req->sfn_sf)%100==0) NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() TX_REQ.req sfn_sf:%d pdus:%d - TX_REQ is within window\n",
                            __FUNCTION__,
                            NFAPI_SFNSF2DEC(req->sfn_sf),
                            req->tx_request_body.number_of_pdus);

			if(pnf_p7->subframe_buffer[buffer_index].tx_req != 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing tx_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_tx_request(pnf_p7->subframe_buffer[buffer_index].tx_req, pnf_p7);
			}

			pnf_p7->subframe_buffer[buffer_index].sfn_sf = req->sfn_sf;
			pnf_p7->subframe_buffer[buffer_index].tx_req = req;

			pnf_p7->stats.tx_ontime++;
		}
		else
		{
                  NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() TX_REQUEST Request is outside of window REQ:SFN_SF:%d CURR:SFN_SF:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), NFAPI_SFNSF2DEC(pnf_p7->sfn_sf));

			deallocate_nfapi_tx_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}

			pnf_p7->stats.tx_late++;
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		deallocate_nfapi_tx_request(req, pnf_p7);
	}
}

void pnf_handle_lbt_dl_config_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
	nfapi_lbt_dl_config_request_t* req = allocate_nfapi_lbt_dl_config_request(pnf_p7);

	if(req == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to alloced nfapi_lbt_dl_config_request structure\n");
		return;
	}

	int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_lbt_dl_config_request_t), &pnf_p7->_public.codec_config);

	if(unpack_result == 0)
	{
		if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
			return;
		}

		if(is_p7_request_in_window(req->sfn_sf, "lbt_dl_request", pnf_p7))
		{
			uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(req->sfn_sf);
			uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

			if(pnf_p7->subframe_buffer[buffer_index].lbt_dl_config_req != 0)
			{
				//NFAPI_TRACE(NFAPI_TRACE_NOTE, "[%d] Freeing tx_req at index %d (%d/%d)", 
				//			pMyPhyInfo->sfnSf, bufferIdx,
				//			SFNSF2SFN(dreq->sfn_sf), SFNSF2SF(dreq->sfn_sf));

				deallocate_nfapi_lbt_dl_config_request(pnf_p7->subframe_buffer[buffer_index].lbt_dl_config_req, pnf_p7);
			}

			pnf_p7->subframe_buffer[buffer_index].sfn_sf = req->sfn_sf;
			pnf_p7->subframe_buffer[buffer_index].lbt_dl_config_req = req;
		}
		else
		{
			deallocate_nfapi_lbt_dl_config_request(req, pnf_p7);

			if(pnf_p7->_public.timing_info_mode_aperiodic)
			{
				pnf_p7->timing_info_aperiodic_send = 1;
			}
		}

		if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
			return;
		}
	}
	else
	{
		deallocate_nfapi_lbt_dl_config_request(req, pnf_p7);
	}

}

void pnf_handle_p7_vendor_extension(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7, uint16_t message_id)
{
	if(pnf_p7->_public.allocate_p7_vendor_ext)
	{
		uint16_t msg_size;
		nfapi_p7_message_header_t* msg = pnf_p7->_public.allocate_p7_vendor_ext(message_id, &msg_size);

		if(msg == 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to allocate vendor extention structure\n");
			return;
		}

		int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, msg, msg_size, &pnf_p7->_public.codec_config);

		if(unpack_result == 0)
		{
			if(pnf_p7->_public.vendor_ext)
				pnf_p7->_public.vendor_ext(&(pnf_p7->_public), msg);
		}
		
		if(pnf_p7->_public.deallocate_p7_vendor_ext)
			pnf_p7->_public.deallocate_p7_vendor_ext(msg);
		
	}
	
}

void pnf_handle_ue_release_request(void* pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7)
{
    nfapi_ue_release_request_t* req = allocate_nfapi_ue_release_request(pnf_p7);
    if(req == NULL)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to alloced nfapi_ue_release_request structure\n");
        return;
    }

    int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, req, sizeof(nfapi_ue_release_request_t), &pnf_p7->_public.codec_config);
    if(unpack_result == 0)
    {
        if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "failed to lock mutex\n");
            return;
        }

        if(is_p7_request_in_window(req->sfn_sf, "ue_release_request", pnf_p7))
        {
            uint32_t sfn_sf_dec = NFAPI_SFNSF2DEC(req->sfn_sf);
            uint8_t buffer_index = sfn_sf_dec % pnf_p7->_public.subframe_buffer_size;

            struct timespec t;
            clock_gettime(CLOCK_MONOTONIC, &t);

            NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() %ld.%09ld POPULATE UE_RELEASE_REQ sfn_sf:%d buffer_index:%d\n", __FUNCTION__, t.tv_sec, t.tv_nsec, sfn_sf_dec, buffer_index);

            if (0 && NFAPI_SFNSF2DEC(req->sfn_sf)%100==0) NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() UE_RELEASE_REQ.req sfn_sf:%d rntis:%d - UE_RELEASE_REQ is within window\n",
                            __FUNCTION__,
                            NFAPI_SFNSF2DEC(req->sfn_sf),
                            req->ue_release_request_body.number_of_TLVs);

            if(pnf_p7->subframe_buffer[buffer_index].ue_release_req != 0)
            {
                deallocate_nfapi_ue_release_request(pnf_p7->subframe_buffer[buffer_index].ue_release_req, pnf_p7);
            }

            pnf_p7->subframe_buffer[buffer_index].sfn_sf = req->sfn_sf;
            pnf_p7->subframe_buffer[buffer_index].ue_release_req = req;

            pnf_p7->stats.tx_ontime++;
        }
        else
        {
            NFAPI_TRACE(NFAPI_TRACE_INFO,"%s() UE_RELEASE_REQUEST Request is outside of window REQ:SFN_SF:%d CURR:SFN_SF:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), NFAPI_SFNSF2DEC(pnf_p7->sfn_sf));

            deallocate_nfapi_ue_release_request(req, pnf_p7);
            if(pnf_p7->_public.timing_info_mode_aperiodic)
            {
                pnf_p7->timing_info_aperiodic_send = 1;
            }

            pnf_p7->stats.tx_late++;
        }
        nfapi_ue_release_response_t resp;
        memset(&resp, 0, sizeof(resp));
        resp.header.message_id = NFAPI_UE_RELEASE_RESPONSE;
        resp.header.phy_id = req->header.phy_id;
        resp.error_code = NFAPI_MSG_OK;
        nfapi_pnf_ue_release_resp(&(pnf_p7->_public), &resp);
        NFAPI_TRACE(NFAPI_TRACE_INFO, "do ue_release_response\n");

        if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "failed to unlock mutex\n");
            return;
        }
    }
    else
    {
        deallocate_nfapi_ue_release_request(req, pnf_p7);
    }
}

uint32_t calculate_t2(uint32_t now_time_hr, uint16_t sfn_sf, uint32_t sf_start_time_hr)
{
	uint32_t sf_time_us = get_sf_time(now_time_hr, sf_start_time_hr);
	uint32_t t2 = (NFAPI_SFNSF2DEC(sfn_sf) * 1000) + sf_time_us;

        if (0)
        {
          static uint32_t prev_t2 = 0;

          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(now_time_hr:%u sfn_sf:%d sf_start_time_Hr:%u) sf_time_us:%u t2:%u prev_t2:%u diff:%u\n",
              __FUNCTION__,
              now_time_hr, NFAPI_SFNSF2DEC(sfn_sf), sf_start_time_hr,
              sf_time_us,
              t2,
              prev_t2,
              t2-prev_t2);

          prev_t2 = t2;
        }

	return t2;
}

uint32_t calculate_nr_t2(uint32_t now_time_hr, uint16_t sfn,uint16_t slot, uint32_t slot_start_time_hr)
{
	uint32_t slot_time_us = get_slot_time(now_time_hr, slot_start_time_hr);
	uint32_t t2 = (NFAPI_SFNSLOT2DEC(sfn, slot) * 500) + slot_time_us;
	
        if (0)
        {
          static uint32_t prev_t2 = 0;

          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(now_time_hr:%u sfn:%d slot:%d slot_start_time_Hr:%u) slot_time_us:%u t2:%u prev_t2:%u diff:%u\n",
              __FUNCTION__,
              now_time_hr, NFAPI_SFNSLOT2DEC(sfn, slot), slot_start_time_hr,
              slot_time_us,
              t2,
              prev_t2,
              t2-prev_t2);

          prev_t2 = t2;
        }

	return t2;
}

uint32_t calculate_t3(uint16_t sfn_sf, uint32_t sf_start_time_hr)
{
	uint32_t now_time_hr = pnf_get_current_time_hr();

	uint32_t sf_time_us = get_sf_time(now_time_hr, sf_start_time_hr);

	uint32_t t3 = (NFAPI_SFNSF2DEC(sfn_sf) * 1000) + sf_time_us;

	return t3;
}

uint32_t calculate_nr_t3(uint16_t sfn, uint16_t slot, uint32_t slot_start_time_hr)
{
	uint32_t now_time_hr = pnf_get_current_time_hr();

	uint32_t slot_time_us = get_slot_time(now_time_hr, slot_start_time_hr);

	uint32_t t3 = (NFAPI_SFNSLOT2DEC(sfn, slot) * 500) + slot_time_us;
	
	return t3;
}

void pnf_handle_dl_node_sync(void *pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7, uint32_t rx_hr_time)
{
	nfapi_dl_node_sync_t dl_node_sync;

	//NFAPI_TRACE(NFAPI_TRACE_INFO, "DL_NODE_SYNC Received\n");

	if (pRecvMsg == NULL || pnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return;
	}

	// unpack the message
	if (nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &dl_node_sync, sizeof(dl_node_sync), &pnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		return;
	}

	if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
		return;
	}


	if (dl_node_sync.delta_sfn_sf != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Will shift SF timing by %d on next subframe\n", dl_node_sync.delta_sfn_sf);

		pnf_p7->sfn_sf_shift = dl_node_sync.delta_sfn_sf;
	}

	nfapi_ul_node_sync_t ul_node_sync;
	memset(&ul_node_sync, 0, sizeof(ul_node_sync));
	ul_node_sync.header.message_id = NFAPI_UL_NODE_SYNC;
	ul_node_sync.header.phy_id = dl_node_sync.header.phy_id;
	ul_node_sync.t1 = dl_node_sync.t1;
	ul_node_sync.t2 = calculate_t2(rx_hr_time, pnf_p7->sfn_sf, pnf_p7->sf_start_time_hr);
	ul_node_sync.t3 = calculate_t3(pnf_p7->sfn_sf, pnf_p7->sf_start_time_hr);

	if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
		return;
	}

	pnf_p7_pack_and_send_p7_message(pnf_p7, &(ul_node_sync.header), sizeof(ul_node_sync));
}


void pnf_nr_handle_dl_node_sync(void *pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7, uint32_t rx_hr_time)
{	
	//printf("Received DL node sync");

	nfapi_nr_dl_node_sync_t dl_node_sync;

	//NFAPI_TRACE(NFAPI_TRACE_INFO, "DL_NODE_SYNC Received\n");

	if (pRecvMsg == NULL || pnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
		return;
	}

	// unpack the message
	if (nfapi_nr_p7_message_unpack(pRecvMsg, recvMsgLen, &dl_node_sync, sizeof(dl_node_sync), &pnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
		return;
	}

	if(pthread_mutex_lock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to lock mutex\n");
		return;
	}


	if (dl_node_sync.delta_sfn_slot != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "Will shift Slot timing by %d on next slot\n", dl_node_sync.delta_sfn_slot);

		pnf_p7->slot_shift = dl_node_sync.delta_sfn_slot;
	}

	nfapi_nr_ul_node_sync_t ul_node_sync;
	memset(&ul_node_sync, 0, sizeof(ul_node_sync));
	ul_node_sync.header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC;
	ul_node_sync.header.phy_id = dl_node_sync.header.phy_id;
	ul_node_sync.t1 = dl_node_sync.t1;
	ul_node_sync.t2 = calculate_nr_t2(rx_hr_time, pnf_p7->sfn,pnf_p7->slot, pnf_p7->slot_start_time_hr);
	ul_node_sync.t3 = calculate_nr_t3(pnf_p7->sfn,pnf_p7->slot, pnf_p7->slot_start_time_hr);

	if(pthread_mutex_unlock(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to unlock mutex\n");
		return;
	}

	pnf_nr_p7_pack_and_send_p7_message(pnf_p7, &(ul_node_sync.header), sizeof(ul_node_sync));
	//printf("\nSSent UL Node Sync sfn:%d,slot:%d\n",pnf_p7->sfn,pnf_p7->slot);
}

void pnf_dispatch_p7_message(void *pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7,  uint32_t rx_hr_time)
{
	nfapi_p7_message_header_t header;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < 4 || pnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: invalid input params\n", __FUNCTION__);
		return;
	}

	// unpack the message header
	if (nfapi_p7_message_header_unpack(pRecvMsg, recvMsgLen, &header, sizeof(header), &pnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	// ensure the message is sensible
	if (recvMsgLen < 8 || pRecvMsg == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_WARN, "Invalid message size: %d, ignoring\n", recvMsgLen);
		return;
	}

	switch (header.message_id)
	{
		case NFAPI_DL_NODE_SYNC:
			pnf_handle_dl_node_sync(pRecvMsg, recvMsgLen, pnf_p7, rx_hr_time);
			break;
		case NFAPI_DL_CONFIG_REQUEST:
			pnf_handle_dl_config_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;

		case NFAPI_UL_CONFIG_REQUEST:
			pnf_handle_ul_config_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;

		case NFAPI_HI_DCI0_REQUEST:
			pnf_handle_hi_dci0_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;

		case NFAPI_TX_REQUEST:
			pnf_handle_tx_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;
		
		case NFAPI_LBT_DL_CONFIG_REQUEST:
			pnf_handle_lbt_dl_config_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;

		case NFAPI_UE_RELEASE_REQUEST:
			pnf_handle_ue_release_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;

		default:
			{
				if(header.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   header.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					pnf_handle_p7_vendor_extension(pRecvMsg, recvMsgLen, pnf_p7, header.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P7 Unknown message ID %d\n", __FUNCTION__, header.message_id);
				}
			}
			break;
	}
}

void pnf_nr_dispatch_p7_message(void *pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7,  uint32_t rx_hr_time)
{
	nfapi_p7_message_header_t header;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < 4 || pnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: invalid input params\n", __FUNCTION__);
		return;
	}

	// unpack the message header
	if (nfapi_p7_message_header_unpack(pRecvMsg, recvMsgLen, &header, sizeof(header), &pnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	// ensure the message is sensible
	if (recvMsgLen < 8 || pRecvMsg == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_WARN, "Invalid message size: %d, ignoring\n", recvMsgLen);
		return;
	}

	switch (header.message_id)
	{
		case NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC:
			pnf_nr_handle_dl_node_sync(pRecvMsg, recvMsgLen, pnf_p7, rx_hr_time);
			break;
		case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
			pnf_handle_dl_tti_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;
		case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
			pnf_handle_ul_tti_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;
		case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
			pnf_handle_ul_dci_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;
		case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
			pnf_handle_tx_data_request(pRecvMsg, recvMsgLen, pnf_p7);
			break;
		default:
			{
				if(header.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   header.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					pnf_handle_p7_vendor_extension(pRecvMsg, recvMsgLen, pnf_p7, header.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P7 Unknown message ID %d\n", __FUNCTION__, header.message_id);
				}
			}
			break;
	}
}

void pnf_handle_p7_message(void *pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7,  uint32_t rx_hr_time)
{
	nfapi_p7_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < 4 || pnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "pnf_handle_p7_message: invalid input params (%d %d %d)\n", pRecvMsg, recvMsgLen, pnf_p7);
		return;
	}

	// unpack the message header
	if (nfapi_p7_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p7_message_header_t), &pnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	uint8_t m = NFAPI_P7_GET_MORE(messageHeader.m_segment_sequence);
	uint8_t sequence_num = NFAPI_P7_GET_SEQUENCE(messageHeader.m_segment_sequence);
	uint8_t segment_num = NFAPI_P7_GET_SEGMENT(messageHeader.m_segment_sequence);

	if(pnf_p7->_public.checksum_enabled)
	{
		uint32_t checksum = nfapi_p7_calculate_checksum(pRecvMsg, recvMsgLen);
		if(checksum != messageHeader.checksum)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "Checksum verification failed %d %d\n", checksum, messageHeader.checksum);
			return;
		}
	}

	if(m == 0 && segment_num == 0)
	{
		// we have a complete message
		// ensure the message is sensible
		if (recvMsgLen < 8 || pRecvMsg == NULL)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "Invalid message size: %d, ignoring\n", recvMsgLen);
			return;
		}

		pnf_dispatch_p7_message(pRecvMsg, recvMsgLen, pnf_p7, rx_hr_time);
	}
	else
	{
		pnf_p7_rx_message_t* rx_msg = pnf_p7_rx_reassembly_queue_add_segment(pnf_p7, &(pnf_p7->reassembly_queue), rx_hr_time, sequence_num, segment_num, m, pRecvMsg, recvMsgLen);

		if(rx_msg->num_segments_received == rx_msg->num_segments_expected)
		{
			// send the buffer on
			uint16_t i = 0;
			uint16_t length = 0;
			for(i = 0; i < rx_msg->num_segments_expected; ++i)
			{
				length += rx_msg->segments[i].length - (i > 0 ? NFAPI_P7_HEADER_LENGTH : 0);
			}
			
			if(pnf_p7->reassemby_buffer_size < length)
			{
				pnf_p7_free(pnf_p7, pnf_p7->reassemby_buffer);
				pnf_p7->reassemby_buffer = 0;
			}

			if(pnf_p7->reassemby_buffer == 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "Resizing PNF_P7 Reassembly buffer %d->%d\n", pnf_p7->reassemby_buffer_size, length);
				pnf_p7->reassemby_buffer = (uint8_t*)pnf_p7_malloc(pnf_p7, length);

				if(pnf_p7->reassemby_buffer == 0)
				{
					NFAPI_TRACE(NFAPI_TRACE_NOTE, "Failed to allocate PNF_P7 reassemby buffer len:%d\n", length);
					return;
				}
                                memset(pnf_p7->reassemby_buffer, 0, length);
				pnf_p7->reassemby_buffer_size = length;
			}
			
			uint16_t offset = 0;
			for(i = 0; i < rx_msg->num_segments_expected; ++i)
			{
				if(i == 0)
				{
					memcpy(pnf_p7->reassemby_buffer, rx_msg->segments[i].buffer, rx_msg->segments[i].length);
					offset += rx_msg->segments[i].length;
				}
				else
				{
					memcpy(pnf_p7->reassemby_buffer + offset, rx_msg->segments[i].buffer + NFAPI_P7_HEADER_LENGTH, rx_msg->segments[i].length - NFAPI_P7_HEADER_LENGTH);
					offset += rx_msg->segments[i].length - NFAPI_P7_HEADER_LENGTH;
				}
			}

			
			pnf_dispatch_p7_message(pnf_p7->reassemby_buffer, length, pnf_p7, rx_msg->rx_hr_time);


			// delete the structure
			pnf_p7_rx_reassembly_queue_remove_msg(pnf_p7, &(pnf_p7->reassembly_queue), rx_msg);
		}
	}

	pnf_p7_rx_reassembly_queue_remove_old_msgs(pnf_p7, &(pnf_p7->reassembly_queue), rx_hr_time, 1000);
	
}

void pnf_nr_handle_p7_message(void *pRecvMsg, int recvMsgLen, pnf_p7_t* pnf_p7,  uint32_t rx_hr_time)
{
	nfapi_p7_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < 4 || pnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "pnf_handle_p7_message: invalid input params (%d %d %d)\n", pRecvMsg, recvMsgLen, pnf_p7);
		return;
	}

	// unpack the message header
	if (nfapi_p7_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p7_message_header_t), &pnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	uint8_t m = NFAPI_P7_GET_MORE(messageHeader.m_segment_sequence);
	uint8_t sequence_num = NFAPI_P7_GET_SEQUENCE(messageHeader.m_segment_sequence);
	uint8_t segment_num = NFAPI_P7_GET_SEGMENT(messageHeader.m_segment_sequence);

	if(pnf_p7->_public.checksum_enabled)
	{
		uint32_t checksum = nfapi_p7_calculate_checksum(pRecvMsg, recvMsgLen);
		if(checksum != messageHeader.checksum)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "Checksum verification failed %d %d\n", checksum, messageHeader.checksum);
			return;
		}
	}

	if(m == 0 && segment_num == 0)
	{
		// we have a complete message
		// ensure the message is sensible
		if (recvMsgLen < 8 || pRecvMsg == NULL)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "Invalid message size: %d, ignoring\n", recvMsgLen);
			return;
		}

		pnf_nr_dispatch_p7_message(pRecvMsg, recvMsgLen, pnf_p7, rx_hr_time);
	}
	else
	{
		pnf_p7_rx_message_t* rx_msg = pnf_p7_rx_reassembly_queue_add_segment(pnf_p7, &(pnf_p7->reassembly_queue), rx_hr_time, sequence_num, segment_num, m, pRecvMsg, recvMsgLen);

		if(rx_msg->num_segments_received == rx_msg->num_segments_expected)
		{
			// send the buffer on
			uint16_t i = 0;
			uint16_t length = 0;
			for(i = 0; i < rx_msg->num_segments_expected; ++i)
			{
				length += rx_msg->segments[i].length - (i > 0 ? NFAPI_P7_HEADER_LENGTH : 0);
			}
			
			if(pnf_p7->reassemby_buffer_size < length)
			{
				pnf_p7_free(pnf_p7, pnf_p7->reassemby_buffer);
				pnf_p7->reassemby_buffer = 0;
			}

			if(pnf_p7->reassemby_buffer == 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "Resizing PNF_P7 Reassembly buffer %d->%d\n", pnf_p7->reassemby_buffer_size, length);
				pnf_p7->reassemby_buffer = (uint8_t*)pnf_p7_malloc(pnf_p7, length);

				if(pnf_p7->reassemby_buffer == 0)
				{
					NFAPI_TRACE(NFAPI_TRACE_NOTE, "Failed to allocate PNF_P7 reassemby buffer len:%d\n", length);
					return;
				}
                                memset(pnf_p7->reassemby_buffer, 0, length);
				pnf_p7->reassemby_buffer_size = length;
			}
			
			uint16_t offset = 0;
			for(i = 0; i < rx_msg->num_segments_expected; ++i)
			{
				if(i == 0)
				{
					memcpy(pnf_p7->reassemby_buffer, rx_msg->segments[i].buffer, rx_msg->segments[i].length);
					offset += rx_msg->segments[i].length;
				}
				else
				{
					memcpy(pnf_p7->reassemby_buffer + offset, rx_msg->segments[i].buffer + NFAPI_P7_HEADER_LENGTH, rx_msg->segments[i].length - NFAPI_P7_HEADER_LENGTH);
					offset += rx_msg->segments[i].length - NFAPI_P7_HEADER_LENGTH;
				}
			}

			
			pnf_nr_dispatch_p7_message(pnf_p7->reassemby_buffer, length, pnf_p7, rx_msg->rx_hr_time);


			// delete the structure
			pnf_p7_rx_reassembly_queue_remove_msg(pnf_p7, &(pnf_p7->reassembly_queue), rx_msg);
		}
	}

	pnf_p7_rx_reassembly_queue_remove_old_msgs(pnf_p7, &(pnf_p7->reassembly_queue), rx_hr_time, 1000);
	
}


void pnf_nfapi_p7_read_dispatch_message(pnf_p7_t* pnf_p7, uint32_t now_hr_time)
{
	int recvfrom_result = 0;
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_size = sizeof(remote_addr);

	do
	{
		// peek the header
		uint8_t header_buffer[NFAPI_P7_HEADER_LENGTH];
		recvfrom_result = recvfrom(pnf_p7->p7_sock, header_buffer, NFAPI_P7_HEADER_LENGTH, MSG_DONTWAIT | MSG_PEEK, (struct sockaddr*)&remote_addr, &remote_addr_size);

		if(recvfrom_result > 0)
		{
			// get the segment size
			nfapi_p7_message_header_t header;
			nfapi_p7_message_header_unpack(header_buffer, NFAPI_P7_HEADER_LENGTH, &header, 34, 0);

			// resize the buffer if we have a large segment
			if(header.message_length > pnf_p7->rx_message_buffer_size)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "reallocing rx buffer %d\n", header.message_length); 
				pnf_p7->rx_message_buffer = realloc(pnf_p7->rx_message_buffer, header.message_length);
				pnf_p7->rx_message_buffer_size = header.message_length;
			}

			// read the segment
			recvfrom_result = recvfrom(pnf_p7->p7_sock, pnf_p7->rx_message_buffer, header.message_length, MSG_DONTWAIT, (struct sockaddr*)&remote_addr, &remote_addr_size);

		now_hr_time = pnf_get_current_time_hr(); //DJP - moved to here - get closer timestamp???

			if(recvfrom_result > 0)
			{
				pnf_handle_p7_message(pnf_p7->rx_message_buffer, recvfrom_result, pnf_p7, now_hr_time);
				//printf("\npnf_handle_p7_message sfn=%d,slot=%d\n",pnf_p7->sfn,pnf_p7->slot);
			}
		}
		else if(recvfrom_result == 0)
		{
			// recv zero length message
			recvfrom_result = recvfrom(pnf_p7->p7_sock, header_buffer, 0, MSG_DONTWAIT, (struct sockaddr*)&remote_addr, &remote_addr_size);
		}

		if(recvfrom_result == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// return to the select
				//NFAPI_TRACE(NFAPI_TRACE_WARN, "%s recvfrom would block :%d\n", __FUNCTION__, errno);
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_WARN, "%s recvfrom failed errno:%d\n", __FUNCTION__, errno);
			}
		}

		// need to update the time as we would only use the value from the
		// select
	}
	while(recvfrom_result > 0);
}

void pnf_nr_nfapi_p7_read_dispatch_message(pnf_p7_t* pnf_p7, uint32_t now_hr_time)
{
	int recvfrom_result = 0;
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_size = sizeof(remote_addr);
	remote_addr.sin_family = 2; //hardcoded
	do
	{
		// peek the header
		uint8_t header_buffer[NFAPI_P7_HEADER_LENGTH];
		recvfrom_result = recvfrom(pnf_p7->p7_sock, header_buffer, NFAPI_P7_HEADER_LENGTH, MSG_DONTWAIT | MSG_PEEK, (struct sockaddr*)&remote_addr, &remote_addr_size);
		if(recvfrom_result > 0)
		{
			// get the segment size
			nfapi_p7_message_header_t header;
			nfapi_p7_message_header_unpack(header_buffer, NFAPI_P7_HEADER_LENGTH, &header, 34, 0);

			// resize the buffer if we have a large segment
			if(header.message_length > pnf_p7->rx_message_buffer_size)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "reallocing rx buffer %d\n", header.message_length); 
				pnf_p7->rx_message_buffer = realloc(pnf_p7->rx_message_buffer, header.message_length);
				pnf_p7->rx_message_buffer_size = header.message_length;
			}

			// read the segment
			recvfrom_result = recvfrom(pnf_p7->p7_sock, pnf_p7->rx_message_buffer, header.message_length, MSG_DONTWAIT, (struct sockaddr*)&remote_addr, &remote_addr_size);

		now_hr_time = pnf_get_current_time_hr(); //DJP - moved to here - get closer timestamp???

			if(recvfrom_result > 0)
			{
				pnf_nr_handle_p7_message(pnf_p7->rx_message_buffer, recvfrom_result, pnf_p7, now_hr_time);
				//printf("\npnf_handle_p7_message sfn=%d,slot=%d\n",pnf_p7->sfn,pnf_p7->slot);
			}
		}
		else if(recvfrom_result == 0)
		{
			// recv zero length message
			recvfrom_result = recvfrom(pnf_p7->p7_sock, header_buffer, 0, MSG_DONTWAIT, (struct sockaddr*)&remote_addr, &remote_addr_size);
		}

		if(recvfrom_result == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// return to the select
				//NFAPI_TRACE(NFAPI_TRACE_WARN, "%s recvfrom would block :%d\n", __FUNCTION__, errno);
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_WARN, "%s recvfrom failed errno:%d\n", __FUNCTION__, errno);
			}
		}

		// need to update the time as we would only use the value from the
		// select
	}
	while(recvfrom_result > 0);
}


int pnf_p7_message_pump(pnf_p7_t* pnf_p7)
{

	// initialize the mutex lock
	if(pthread_mutex_init(&(pnf_p7->mutex), NULL) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 mutex init: %d\n", errno);
		return -1;
	}
	
	if(pthread_mutex_init(&(pnf_p7->pack_mutex), NULL) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 mutex init: %d\n", errno);
		return -1;
	}	

	// create the pnf p7 socket
	if ((pnf_p7->p7_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 socket errno: %d\n", errno);
		return -1;
	}
	NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 socket created (%d)...\n", pnf_p7->p7_sock);

	// configure the UDP socket options
	int reuseaddr_enable = 1;
	if (setsockopt(pnf_p7->p7_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_enable, sizeof(int)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF P7 setsockopt (SOL_SOCKET, SO_REUSEADDR) failed  errno: %d\n", errno);
		return -1;
	}

/*
	int reuseport_enable = 1;
	if (setsockopt(pnf_p7->p7_sock, SOL_SOCKET, SO_REUSEPORT, &reuseport_enable, sizeof(int)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF P7 setsockopt (SOL_SOCKET, SO_REUSEPORT) failed  errno: %d\n", errno);
		return -1;
	}
*/
		
	int iptos_value = FAPI2_IP_DSCP << 2;
	if (setsockopt(pnf_p7->p7_sock, IPPROTO_IP, IP_TOS, &iptos_value, sizeof(iptos_value)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF P7 setsockopt (IPPROTO_IP, IP_TOS) failed errno: %d\n", errno);
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = pnf_p7->_public.local_p7_port;

	if(pnf_p7->_public.local_p7_addr == 0)
	{
		addr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		//addr.sin_addr.s_addr = inet_addr(pnf_p7->_public.local_p7_addr);
		if(inet_aton(pnf_p7->_public.local_p7_addr, &addr.sin_addr) == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "inet_aton failed\n");
		}
	}


	NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 binding %d too %s:%d\n", pnf_p7->p7_sock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	if (bind(pnf_p7->p7_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF_P7 bind error fd:%d errno: %d\n", pnf_p7->p7_sock, errno);
		return -1;
	}
	NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 bind succeeded...\n");

	while(pnf_p7->terminate == 0)
	{
		fd_set rfds;
		int selectRetval = 0;

		// select on a timeout and then get the message
		FD_ZERO(&rfds);
		FD_SET(pnf_p7->p7_sock, &rfds);

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		selectRetval = select(pnf_p7->p7_sock+1, &rfds, NULL, NULL, &timeout);

		uint32_t now_hr_time = pnf_get_current_time_hr();

		
		

		if(selectRetval == 0)
		{	
			// timeout
			continue;
		}
		else if (selectRetval == -1 && (errno == EINTR))
		{
			// interrupted by signal
			NFAPI_TRACE(NFAPI_TRACE_WARN, "PNF P7 Signal Interrupt %d\n", errno);
			continue;
		}
		else if (selectRetval == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "PNF P7 select() failed\n");
			sleep(1);
			continue;
		}

		if(FD_ISSET(pnf_p7->p7_sock, &rfds)) 

		{
			pnf_nfapi_p7_read_dispatch_message(pnf_p7, now_hr_time);
		}
	}
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF_P7 Terminating..\n");

	// close the connection and socket
	if (close(pnf_p7->p7_sock) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "close failed errno: %d\n", errno);
	}

	if(pthread_mutex_destroy(&(pnf_p7->pack_mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "mutex destroy failed errno: %d\n", errno);
	}

	if(pthread_mutex_destroy(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "mutex destroy failed errno: %d\n", errno);
	}

	return 0;
}

int pnf_nr_p7_message_pump(pnf_p7_t* pnf_p7)
{

	// initialize the mutex lock
	if(pthread_mutex_init(&(pnf_p7->mutex), NULL) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 mutex init: %d\n", errno);
		return -1;
	}
	
	if(pthread_mutex_init(&(pnf_p7->pack_mutex), NULL) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 mutex init: %d\n", errno);
		return -1;
	}	

	// create the pnf p7 socket
	if ((pnf_p7->p7_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 socket errno: %d\n", errno);
		return -1;
	}
	NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 socket created (%d)...\n", pnf_p7->p7_sock);

	// configure the UDP socket options
	int reuseaddr_enable = 1;
	if (setsockopt(pnf_p7->p7_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_enable, sizeof(int)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF P7 setsockopt (SOL_SOCKET, SO_REUSEADDR) failed  errno: %d\n", errno);
		return -1;
	}

/*
	int reuseport_enable = 1;
	if (setsockopt(pnf_p7->p7_sock, SOL_SOCKET, SO_REUSEPORT, &reuseport_enable, sizeof(int)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF P7 setsockopt (SOL_SOCKET, SO_REUSEPORT) failed  errno: %d\n", errno);
		return -1;
	}
*/
		
	int iptos_value = FAPI2_IP_DSCP << 2;
	if (setsockopt(pnf_p7->p7_sock, IPPROTO_IP, IP_TOS, &iptos_value, sizeof(iptos_value)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF P7 setsockopt (IPPROTO_IP, IP_TOS) failed errno: %d\n", errno);
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = pnf_p7->_public.local_p7_port;

	if(pnf_p7->_public.local_p7_addr == 0)
	{
		addr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		//addr.sin_addr.s_addr = inet_addr(pnf_p7->_public.local_p7_addr);
		if(inet_aton(pnf_p7->_public.local_p7_addr, &addr.sin_addr) == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "inet_aton failed\n");
		}
	}


	NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 binding %d too %s:%d\n", pnf_p7->p7_sock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	if (bind(pnf_p7->p7_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF_P7 bind error fd:%d errno: %d\n", pnf_p7->p7_sock, errno);
		return -1;
	}
	NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 bind succeeded...\n");

	while(pnf_p7->terminate == 0)
	{
		fd_set rfds;
		int selectRetval = 0;

		// select on a timeout and then get the message
		FD_ZERO(&rfds);
		FD_SET(pnf_p7->p7_sock, &rfds);

		struct timeval timeout;
		timeout.tv_sec = 100;
		timeout.tv_usec = 0;

		selectRetval = select(pnf_p7->p7_sock+1, &rfds, NULL, NULL, &timeout);

		uint32_t now_hr_time = pnf_get_current_time_hr();

		
		

		if(selectRetval == 0)
		{	
			// timeout
			continue;
		}
		else if (selectRetval == -1 && (errno == EINTR))
		{
			// interrupted by signal
			NFAPI_TRACE(NFAPI_TRACE_WARN, "PNF P7 Signal Interrupt %d\n", errno);
			continue;
		}
		else if (selectRetval == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "PNF P7 select() failed\n");
			sleep(1);
			continue;
		}

		if(FD_ISSET(pnf_p7->p7_sock, &rfds)) 

		{
			pnf_nr_nfapi_p7_read_dispatch_message(pnf_p7, now_hr_time);
		}
	}
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "PNF_P7 Terminating..\n");

	// close the connection and socket
	if (close(pnf_p7->p7_sock) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "close failed errno: %d\n", errno);
	}

	if(pthread_mutex_destroy(&(pnf_p7->pack_mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "mutex destroy failed errno: %d\n", errno);
	}

	if(pthread_mutex_destroy(&(pnf_p7->mutex)) != 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "mutex destroy failed errno: %d\n", errno);
	}

	return 0;
}
