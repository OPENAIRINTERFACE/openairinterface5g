/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file radio/ETHERNET/if_defs.h
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#ifndef __ETHERNET_USERSPACE_LIB_IF_DEFS__H__
#define __ETHERNET_USERSPACE_LIB_IF_DEFS__H__

#include <netinet/ether.h>
#include <stdint.h>

#ifndef LITE_COMPILATION
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#endif

// ETH transport preference modes
#define ETH_UDP_MODE          0
#define ETH_RAW_MODE          1
#define ETH_UDP_IF4p5_MODE    2
#define ETH_RAW_IF4p5_MODE    3
#define ETH_UDP_IF5_ECPRI_MODE  4    

// COMMOM HEADER LENGTHS

#define NO_COMPRESS      0       
#define ALAW_COMPRESS    1

#define UDP_HEADER_SIZE_BYTES	8
#define IPV4_HEADER_SIZE_BYTES	60	// This is the maximum IPv4 header length

// Time domain RRH packet sizes
#define MAC_HEADER_SIZE_BYTES (sizeof(struct ether_header))
#define MAX_PACKET_SEQ_NUM(spp,spf) (spf/spp)
#define PAYLOAD_SIZE_BYTES(nsamps) (nsamps<<2)
#define UDP_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))
#define RAW_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + MAC_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))

#define PAYLOAD_SIZE_BYTES_ALAW(nsamps) (nsamps<<1)
#define UDP_PACKET_SIZE_BYTES_ALAW(nsamps) (APP_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES_ALAW(nsamps))
#define RAW_PACKET_SIZE_BYTES_ALAW(nsamps) (APP_HEADER_SIZE_BYTES + MAC_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES_ALAW(nsamps))

// Packet sizes for IF4p5 interface format
#define DATA_BLOCK_SIZE_BYTES(scaled_nblocks) (sizeof(uint16_t)*scaled_nblocks)
#define PRACH_NUM_SAMPLES (839*2)
#define PRACH_BLOCK_SIZE_BYTES (sizeof(int16_t)*PRACH_NUM_SAMPLES)  
 
#define RAW_IF4p5_PDLFFT_SIZE_BYTES(nblocks) (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define RAW_IF4p5_PULFFT_SIZE_BYTES(nblocks) (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define RAW_IF4p5_PULTICK_SIZE_BYTES (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t)  
#define RAW_IF4p5_PRACH_SIZE_BYTES (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + PRACH_BLOCK_SIZE_BYTES)
#define UDP_IF4p5_PDLFFT_SIZE_BYTES(nblocks) (UDP_HEADER_SIZE_BYTES + IPV4_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define UDP_IF4p5_PULFFT_SIZE_BYTES(nblocks) (UDP_HEADER_SIZE_BYTES + IPV4_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define UDP_IF4p5_PULTICK_SIZE_BYTES (UDP_HEADER_SIZE_BYTES + IPV4_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t)  
#define UDP_IF4p5_PRACH_SIZE_BYTES (UDP_HEADER_SIZE_BYTES + IPV4_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + PRACH_BLOCK_SIZE_BYTES)

// Mobipass packet sizes
#define RAW_IF5_MOBIPASS_BLOCK_SIZE_BYTES 1280
#define RAW_IF5_MOBIPASS_SIZE_BYTES (MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + RAW_IF5_MOBIPASS_BLOCK_SIZE_BYTES)
#define PAYLOAD_MOBIPASS_NUM_SAMPLES  640

#endif
