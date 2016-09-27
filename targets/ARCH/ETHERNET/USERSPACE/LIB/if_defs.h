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

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

// ETH transport preference modes
#define ETH_UDP_MODE        0
#define ETH_RAW_MODE        1
#define ETH_UDP_IF4p5_MODE    2
#define ETH_RAW_IF4p5_MODE    3
#define ETH_RAW_IF5_MOBIPASS    4    

// Time domain RRH packet sizes
#define MAC_HEADER_SIZE_BYTES (sizeof(struct ether_header))
#define MAX_PACKET_SEQ_NUM(spp,spf) (spf/spp)
#define PAYLOAD_SIZE_BYTES(nsamps) (nsamps<<2)
#define UDP_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))
#define RAW_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + MAC_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))

// Packet sizes for IF4p5 interface format
#define DATA_BLOCK_SIZE_BYTES(scaled_nblocks) (sizeof(uint16_t)*scaled_nblocks)
#define PRACH_BLOCK_SIZE_BYTES (sizeof(int16_t)*839*2)  // FIX hard coded prach size (uncompressed)
 
#define RAW_IF4p5_PDLFFT_SIZE_BYTES(nblocks) (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define RAW_IF4p5_PULFFT_SIZE_BYTES(nblocks) (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define RAW_IF4p5_PRACH_SIZE_BYTES (MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + PRACH_BLOCK_SIZE_BYTES)
#define UDP_IF4p5_PDLFFT_SIZE_BYTES(nblocks) (sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define UDP_IF4p5_PULFFT_SIZE_BYTES(nblocks) (sizeof_IF4p5_header_t + DATA_BLOCK_SIZE_BYTES(nblocks))  
#define UDP_IF4p5_PRACH_SIZE_BYTES (sizeof_IF4p5_header_t + PRACH_BLOCK_SIZE_BYTES)

// Mobipass packet sizes
#define RAW_IF5_MOBIPASS_BLOCK_SIZE_BYTES 1280
#define RAW_IF5_MOBIPASS_SIZE_BYTES (MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + RAW_IF5_MOBIPASS_BLOCK_SIZE_BYTES)
