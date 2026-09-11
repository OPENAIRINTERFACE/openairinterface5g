/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "vnf_p7_common.h"

void* vnf_p7_malloc(vnf_p7_t* vnf_p7, size_t size)
{
	if(vnf_p7->_public.malloc)
	{
		return (vnf_p7->_public.malloc)(size);
	}
	else
	{
		return calloc(1, size);
	}
}
void vnf_p7_free(vnf_p7_t* vnf_p7, void* ptr)
{
	if(ptr == 0)
		return;

	if(vnf_p7->_public.free)
	{
		(vnf_p7->_public.free)(ptr);
	}
	else
	{
		free(ptr);
	}
}

void vnf_p7_codec_free(vnf_p7_t* vnf_p7, void* ptr)
{
	if(ptr == 0)
		return;

	if(vnf_p7->_public.codec_config.deallocate)
	{
		(vnf_p7->_public.codec_config.deallocate)(ptr);
	}
	else
	{
		free(ptr);
	}
}

void vnf_p7_connection_info_list_add(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* node)
{
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);
	// todo : add mutex
	node->next = vnf_p7->p7_connections;
	vnf_p7->p7_connections = node;
}

nfapi_vnf_p7_connection_info_t* vnf_p7_connection_info_list_find(vnf_p7_t* vnf_p7, uint16_t phy_id)
{
	nfapi_vnf_p7_connection_info_t* curr = vnf_p7->p7_connections;
  while (curr != 0) {
    if (curr->phy_id == phy_id)
      return curr;
    curr = curr->next;
  }
  NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): could not find P7 connection for phy_id %d\n", __func__, phy_id);

  return 0;
}

nfapi_vnf_p7_connection_info_t* vnf_p7_connection_info_list_delete(vnf_p7_t* vnf_p7, uint16_t phy_id)
{
	nfapi_vnf_p7_connection_info_t* curr = vnf_p7->p7_connections;
	nfapi_vnf_p7_connection_info_t* prev = 0;

	while(curr != 0)
	{
		if(curr->phy_id == phy_id)
		{
			if(prev == 0)
			{
				vnf_p7->p7_connections = curr->next;
			}
			else
			{
				prev->next = curr->next;
			}

			return curr;
		}
		else
		{
			prev = curr;
			curr = curr->next;
		}
	}

	return 0;
}

vnf_p7_rx_message_t* vnf_p7_rx_reassembly_queue_add_segment(vnf_p7_t* vnf_p7, vnf_p7_rx_reassembly_queue_t* queue, uint16_t sequence_number, uint16_t segment_number, uint8_t m, uint8_t* data, uint16_t data_len)
{
	vnf_p7_rx_message_t* msg = 0;
	// attempt to find a entry for this segment
	vnf_p7_rx_message_t* iterator = queue->msg_queue;
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

		msg->segments[segment_number].buffer = (uint8_t*)vnf_p7_malloc(vnf_p7, data_len);
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
		msg = (vnf_p7_rx_message_t*)(vnf_p7_malloc(vnf_p7, sizeof(vnf_p7_rx_message_t)));
		memset(msg, 0, sizeof(vnf_p7_rx_message_t));

		msg->sequence_number = sequence_number;
		msg->num_segments_expected = m ? 255 : segment_number + 1;
		msg->num_segments_received = 1;
		msg->rx_hr_time = vnf_get_current_time_hr();

		msg->segments[segment_number].buffer = (uint8_t*)vnf_p7_malloc(vnf_p7, data_len);
		memcpy(msg->segments[segment_number].buffer, data, data_len);
		msg->segments[segment_number].length = data_len;

		// place the message at the head of the queue
		msg->next = queue->msg_queue;
		queue->msg_queue = msg;
	}

	return msg;
}

void vnf_p7_rx_reassembly_queue_remove_msg(vnf_p7_t* vnf_p7, vnf_p7_rx_reassembly_queue_t* queue, vnf_p7_rx_message_t* msg)
{
	// remove message if it has the same sequence number
	vnf_p7_rx_message_t* iterator = queue->msg_queue;
	vnf_p7_rx_message_t* previous = 0;

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

			// delete the message
			uint16_t i;
			for(i = 0; i < 128; ++i)
			{
				if(iterator->segments[i].buffer)
					vnf_p7_free(vnf_p7, iterator->segments[i].buffer);
			}
			vnf_p7_free(vnf_p7, iterator);

			break;
		}

		previous = iterator;
		iterator = iterator->next;
	}
}

void vnf_p7_rx_reassembly_queue_remove_old_msgs(vnf_p7_t* vnf_p7, vnf_p7_rx_reassembly_queue_t* queue, uint32_t delta)
{
	// remove all messages that are too old
	vnf_p7_rx_message_t* iterator = queue->msg_queue;
	vnf_p7_rx_message_t* previous = 0;

	uint32_t rx_hr_time = vnf_get_current_time_hr();

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

			NFAPI_TRACE(NFAPI_TRACE_WARN, "Deleting stale reassembly message (packet rx_hr_time %u current rx_hr_time %u delta %d us)\n", iterator->rx_hr_time, rx_hr_time, delta);

			vnf_p7_rx_message_t* to_delete = iterator;
			iterator = iterator->next;

			// delete the message
			uint16_t i;
			for(i = 0; i < 128; ++i)
			{
				if(to_delete->segments[i].buffer)
					vnf_p7_free(vnf_p7, to_delete->segments[i].buffer);
			}
			vnf_p7_free(vnf_p7, to_delete);

		}
		else
		{
			previous = iterator;
			iterator = iterator->next;
		}
	}
}

uint32_t get_slot_time(uint32_t now_hr, uint32_t slot_start_hr)
{
	if(now_hr < slot_start_hr)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "now is earlier than start of slot\n");
		return 0;
	}
	else
	{
		uint32_t now_us = TIMEHR_USEC(now_hr);
		uint32_t slot_start_us = TIMEHR_USEC(slot_start_hr);

		// if the us have wrapped adjust for it
		if(now_hr < slot_start_us)
		{
			now_us += 1000000;
		}

		return now_us - slot_start_us;
	}
}

uint32_t calculate_transmit_timestamp(int mu, uint16_t sfn, uint16_t slot, uint32_t slot_start_time_hr)
{
	uint32_t now_time_hr = vnf_get_current_time_hr();

	uint32_t slot_time_us = get_slot_time(now_time_hr, slot_start_time_hr);

	uint32_t tt = NFAPI_SFNSLOT2DEC(mu, sfn, slot) * NFAPI_SLOTLEN(mu) + slot_time_us;

	return tt;
}

uint32_t vnf_get_current_time_hr()
{
	struct timeval now;
	(void)gettimeofday(&now, NULL);
	uint32_t time_hr = TIME2TIMEHR(now);
	return time_hr;
}

struct timespec timespec_delta(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0)
	{
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

int vnf_send_p7_msg(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* p7_info, uint8_t* msg, const uint32_t len)
{
	int sendto_result = sendto(vnf_p7->socket, msg, len, 0, (struct sockaddr*)&(p7_info->remote_addr), sizeof(p7_info->remote_addr));
	if(sendto_result != len)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() sendto_result %d %d\n", __FUNCTION__, sendto_result, errno);
	}

	return 0;
}

int vnf_p7_pack_and_send_p7_msg(vnf_p7_t* vnf_p7, nfapi_p7_message_header_t* header)
{

	nfapi_vnf_p7_connection_info_t* p7_connection = vnf_p7_connection_info_list_find(vnf_p7, header->phy_id);
	if(p7_connection)
	{
		int send_result = 0;
		uint8_t  buffer[1024 * 32];

		header->m_segment_sequence = NFAPI_P7_SET_MSS(0, 0, p7_connection->sequence_number);

		int len = nfapi_p7_message_pack(header, buffer, sizeof(buffer), &vnf_p7->_public.codec_config);

                //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() phy_id:%d nfapi_p7_message_pack()=len=%d vnf_p7->_public.segment_size:%u\n", __FUNCTION__, header->phy_id, len, vnf_p7->_public.segment_size);

		if(len < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() failed to pack p7 message phy_id:%d\n", __FUNCTION__, header->phy_id);
			return -1;
		}

		if(len > vnf_p7->_public.segment_size)
		{
			// todo : consider replacing with the sendmmsg call
			// todo : worry about blocking writes?

			// segmenting the transmit
			int msg_body_len = len - NFAPI_P7_HEADER_LENGTH ;
			int seg_body_len = vnf_p7->_public.segment_size - NFAPI_P7_HEADER_LENGTH ;
			int segment_count = (msg_body_len / (seg_body_len)) + ((msg_body_len % seg_body_len) ? 1 : 0);

			int segment = 0;
			int offset = NFAPI_P7_HEADER_LENGTH;
			uint8_t tx_buffer[vnf_p7->_public.segment_size];
                        NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() MORE THAN ONE SEGMENT phy_id:%d nfapi_p7_message_pack()=len=%d vnf_p7->_public.segment_size:%u\n", __FUNCTION__, header->phy_id, len, vnf_p7->_public.segment_size);
			for(segment = 0; segment < segment_count; ++segment)
			{
				uint8_t last = 0;
				uint16_t size = vnf_p7->_public.segment_size - NFAPI_P7_HEADER_LENGTH;
				if(segment + 1 == segment_count)
				{
					last = 1;
					size = (msg_body_len) - (seg_body_len * segment);
				}

				uint16_t segment_size = size + NFAPI_P7_HEADER_LENGTH;

				// Update the header with the m and segement
				memcpy(&tx_buffer[0], buffer, NFAPI_P7_HEADER_LENGTH);

				// set the segment length
				tx_buffer[4] = (segment_size & 0xFF00) >> 8;
				tx_buffer[5] = (segment_size & 0xFF);

				// set the m & segment number
				tx_buffer[6] = ((!last) << 7) + segment;

				memcpy(&tx_buffer[NFAPI_P7_HEADER_LENGTH], &buffer[0] + offset, size);
				offset += size;

				if(vnf_p7->_public.checksum_enabled)
				{
					nfapi_p7_update_checksum(tx_buffer, segment_size);
				}

				nfapi_p7_update_transmit_timestamp(buffer, calculate_transmit_timestamp(p7_connection->mu, p7_connection->sfn, p7_connection->slot, vnf_p7->slot_start_time_hr));

				send_result = vnf_send_p7_msg(vnf_p7, p7_connection,  &tx_buffer[0], segment_size);
			}
		}
		else
		{
			if(vnf_p7->_public.checksum_enabled)
			{
				nfapi_p7_update_checksum(buffer, len);
			}

			nfapi_p7_update_transmit_timestamp(buffer, calculate_transmit_timestamp(p7_connection->mu, p7_connection->sfn, p7_connection->slot, vnf_p7->slot_start_time_hr));

			// simple case that the message fits in a single segement
			send_result = vnf_send_p7_msg(vnf_p7, p7_connection, &buffer[0], len);
		}

		p7_connection->sequence_number++;

		return send_result;
	}
	else
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find p7 connection info for phy_id:%d\n", __FUNCTION__, header->phy_id);
		return -1;
	}
}

void vnf_p7_release_pdu(vnf_p7_t* vnf_p7, void* pdu)
{
	vnf_p7_free(vnf_p7, pdu);
}
