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
