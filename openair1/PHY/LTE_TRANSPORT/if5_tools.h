/*! \file PHY/LTE_TRANSPORT/if5_tools.h
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#include <stdint.h>
#include "PHY/defs.h"

#define IF5_RRH_GW_DL 0x0022
#define IF5_RRH_GW_UL 0x0023
#define IF5_MOBIPASS 0xbffe

struct IF5_mobipass_header {  
  /// 
  uint16_t flags; 
  /// 
  uint16_t fifo_status;
  /// 
  uint8_t seqno;
  ///
  uint8_t ack;
  ///
  uint32_t word0;
  /// 
  uint32_t time_stamp;
  
} __attribute__ ((__packed__));

typedef struct IF5_mobipass_header IF5_mobipass_header_t;
#define sizeof_IF5_mobipass_header_t 14

void send_IF5(PHY_VARS_eNB*, openair0_timestamp, int, uint8_t*, uint16_t);

void recv_IF5(PHY_VARS_eNB*, openair0_timestamp*, int, uint16_t);
