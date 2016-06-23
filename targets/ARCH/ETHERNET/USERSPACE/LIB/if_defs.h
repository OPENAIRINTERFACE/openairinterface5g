/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#include <netinet/ether.h>
#include <stdint.h>

// ETH transport preference modes
#define ETH_UDP_MODE        0
#define ETH_RAW_MODE        1
#define ETH_UDP_IF4_MODE    2
#define ETH_RAW_IF4_MODE    3

// Time domain RRH packet sizes
#define MAC_HEADER_SIZE_BYTES (sizeof(struct ether_header))
#define MAX_PACKET_SEQ_NUM(spp,spf) (spf/spp)
#define PAYLOAD_SIZE_BYTES(nsamps) (nsamps<<2)
#define UDP_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))
#define RAW_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + MAC_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))

// Packet sizes for IF4 interface format
#define DATA_BLOCK_SIZE_BYTES(scaled_nblocks) (sizeof(int16_t)*scaled_nblocks)
#define PRACH_BLOCK_SIZE_BYTES (sizeof(int16_t)*839*2)  // FIX hard coded prach size (uncompressed)
 
#define RAW_IF4_PDLFFT_SIZE_BYTES(nblocks) (MAC_HEADER_SIZE_BYTES + sizeof_IF4_dl_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define RAW_IF4_PULFFT_SIZE_BYTES(nblocks) (MAC_HEADER_SIZE_BYTES + sizeof_IF4_ul_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define RAW_IF4_PRACH_SIZE_BYTES (MAC_HEADER_SIZE_BYTES + sizeof_IF4_prach_header_t + PRACH_BLOCK_SIZE_BYTES)
