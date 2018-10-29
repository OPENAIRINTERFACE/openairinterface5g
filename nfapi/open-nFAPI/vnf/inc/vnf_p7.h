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


#ifndef _VNF_P7_H_
#define _VNF_P7_H_

#include "nfapi_vnf_interface.h"

#define TIMEHR_SEC(_time_hr) ((uint32_t)(_time_hr) >> 20)
#define TIMEHR_USEC(_time_hr) ((uint32_t)(_time_hr) & 0xFFFFF)
#define TIME2TIMEHR(_time) (((uint32_t)(_time.tv_sec) & 0xFFF) << 20 | ((uint32_t)(_time.tv_usec) & 0xFFFFF))



typedef struct {
	uint8_t* buffer;
	uint16_t length;
} vnf_p7_rx_message_segment_t;

typedef struct vnf_p7_rx_message vnf_p7_rx_message_t;

typedef struct vnf_p7_rx_message {
	uint8_t sequence_number;
	uint8_t num_segments_received;
	uint8_t num_segments_expected;

	// the spec allows of upto 128 segments, this does seem excessive
	vnf_p7_rx_message_segment_t segments[128];

	uint32_t rx_hr_time;

	vnf_p7_rx_message_t* next;
} vnf_p7_rx_message_t;

typedef struct {

	vnf_p7_rx_message_t* msg_queue;

} vnf_p7_rx_reassembly_queue_t;

typedef struct nfapi_vnf_p7_connection_info {

	/*! The PHY id */
	int phy_id;


	// this does not belong here...
	uint8_t stream_id;

	/*! Flag indicating the sync state of the P7 conenction */
	uint8_t in_sync;

	int dl_out_sync_offset; 
	int dl_out_sync_period; // ms (as a pow2)

	int dl_in_sync_offset; 
	int dl_in_sync_period; // ms (as a pow2)

	uint8_t filtered_adjust;
	uint16_t min_sync_cycle_count;
	uint32_t latency[8];
	uint32_t average_latency;
	int32_t sf_offset_filtered;
	int32_t sf_offset_trend;
	int32_t sf_offset;
	uint16_t zero_count;
	int32_t adjustment;
	int32_t insync_minor_adjustment;
	int32_t insync_minor_adjustment_duration;

	uint32_t previous_t1;
	uint32_t previous_t2;
	int32_t previous_sf_offset_filtered;

	int sfn_sf;

	int socket;
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
	
	vnf_p7_rx_reassembly_queue_t reassembly_queue;
	uint8_t* reassembly_buffer;
	uint32_t reassembly_buffer_size;

	uint32_t sequence_number;

	struct nfapi_vnf_p7_connection_info* next;

} nfapi_vnf_p7_connection_info_t;

typedef struct {

	nfapi_vnf_p7_config_t _public;
	
	// private data
	uint8_t terminate;
	nfapi_vnf_p7_connection_info_t* p7_connections;
	int socket;
	uint32_t sf_start_time_hr;
	uint8_t* rx_message_buffer; // would this be better put in the p7 conenction info?
	uint16_t rx_message_buffer_size;
	
} vnf_p7_t;

uint32_t vnf_get_current_time_hr(void);

uint16_t increment_sfn_sf(uint16_t sfn_sf);
int vnf_sync(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* p7_info);
int send_mac_subframe_indications(vnf_p7_t* config);
int vnf_p7_read_dispatch_message(vnf_p7_t* vnf_p7 );

void vnf_p7_connection_info_list_add(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* node);
nfapi_vnf_p7_connection_info_t* vnf_p7_connection_info_list_find(vnf_p7_t* vnf_p7, uint16_t phy_id);
nfapi_vnf_p7_connection_info_t* vnf_p7_connection_info_list_delete(vnf_p7_t* vnf_p7, uint16_t phy_id);

int vnf_p7_pack_and_send_p7_msg(vnf_p7_t* vnf_p7, nfapi_p7_message_header_t* header);

void vnf_p7_release_msg(vnf_p7_t* vnf_p7, nfapi_p7_message_header_t* header);
void vnf_p7_release_pdu(vnf_p7_t* vnf_p7, void* pdu);


#endif // _VNF_P7_H_
