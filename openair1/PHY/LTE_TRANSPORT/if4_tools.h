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

/*! \file PHY/LTE_TRANSPORT/if4_tools.h
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#include "PHY/defs.h"

/// Macro for IF4 packet type
#define IF4p5_PACKET_TYPE 0x080A 
#define IF4p5_PULFFT 0x0019 
#define IF4p5_PDLFFT 0x0020
#define IF4p5_PRACH 0x0021

struct IF4p5_header {  
  /// Type
  uint16_t type; 
  /// Sub-Type
  uint16_t sub_type;
  /// Reserved
  uint32_t rsvd;
  /// Frame Status
  uint32_t frame_status;

} __attribute__ ((__packed__));

typedef struct IF4p5_header IF4p5_header_t;
#define sizeof_IF4p5_header_t 12 

void gen_IF4p5_dl_header(IF4p5_header_t*, int, int);

void gen_IF4p5_ul_header(IF4p5_header_t*, int, int);

void gen_IF4p5_prach_header(IF4p5_header_t*, int, int);

void send_IF4p5(PHY_VARS_eNB*, int, int, uint16_t, int);

void recv_IF4p5(PHY_VARS_eNB*, int*, int*, uint16_t*, uint32_t*);

void malloc_IF4p5_buffer(PHY_VARS_eNB*);
