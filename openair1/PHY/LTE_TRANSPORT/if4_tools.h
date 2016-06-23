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

#include <stdint.h>
#include "PHY/defs.h"

/// Macro for IF4 packet type
#define IF4_PACKET_TYPE 0x080A 
#define IF4_PULFFT 0x0019 
#define IF4_PDLFFT 0x0020
#define IF4_PRACH 0x0021

/* 

Bit-field reference

/// IF4 Frame Status (32 bits)
struct IF4_frame_status {
  /// Antenna Numbers
  uint32_t ant_num:3;
  /// Antenna Start
  uint32_t ant_start:3;
  /// Radio Frame Number
  uint32_t rf_num:16;
  /// Sub-frame Number
  uint32_t sf_num:4;
  /// Symbol Number
  uint32_t sym_num:4;
  /// Reserved
  uint32_t rsvd:2;    
};

/// IF4 Antenna Gain (16 bits)
struct IF4_gain {
  /// Reserved 
  uint16_t rsvd:10;
  /// FFT Exponent Output
  uint16_t exponent:6;  
};  

/// IF4 LTE PRACH Configuration (32 bits)
struct IF4_lte_prach_conf {
  /// Reserved
  uint32_t rsvd:3;
  /// Antenna Indication
  uint32_t ant:3;
  /// Radio Frame Number
  uint32_t rf_num:16;
  /// Sub-frame Number
  uint32_t sf_num:4;
  /// FFT Exponent Output
  uint32_t exponent:6;  
};

*/

struct IF4_dl_header {
  /// Destination Address
  
  /// Source Address
  
  /// Type
  uint16_t type; 
  /// Sub-Type
  uint16_t sub_type;
  /// Reserved
  uint32_t rsvd;
  /// Frame Status
  uint32_t frame_status;
  /// Data Blocks

  /// Frame Check Sequence

};

typedef struct IF4_dl_header IF4_dl_header_t;
#define sizeof_IF4_dl_header_t 12 

struct IF4_ul_header {
  /// Destination Address
  
  /// Source Address
  
  /// Type
  uint16_t type;
  /// Sub-Type
  uint16_t sub_type;
  /// Reserved
  uint32_t rsvd;
  /// Frame Status
  uint32_t frame_status;
  /// Gain 0
  uint16_t gain0;
  /// Gain 1
  uint16_t gain1;
  /// Gain 2
  uint16_t gain2;
  /// Gain 3
  uint16_t gain3;
  /// Gain 4
  uint16_t gain4;
  /// Gain 5
  uint16_t gain5;
  /// Gain 6
  uint16_t gain6;
  /// Gain 7
  uint16_t gain7;
  /// Data Blocks

  /// Frame Check Sequence

};

typedef struct IF4_ul_header IF4_ul_header_t;
#define sizeof_IF4_ul_header_t 28 

struct IF4_prach_header {
  /// Destination Address 
  
  /// Source Address
  
  /// Type
  uint16_t type;
  /// Sub-Type
  uint16_t sub_type;
  /// Reserved
  uint32_t rsvd;
  /// LTE Prach Configuration
  uint32_t prach_conf;
  /// Prach Data Block (one antenna)

  /// Frame Check Sequence

};

typedef struct IF4_prach_header IF4_prach_header_t;
#define sizeof_IF4_prach_header_t 12

void gen_IF4_dl_header(IF4_dl_header_t*, int, int);

void gen_IF4_ul_header(IF4_ul_header_t*, int, int);

void gen_IF4_prach_header(IF4_prach_header_t*, int, int);

void send_IF4(PHY_VARS_eNB*, int, int, uint16_t, int);

void recv_IF4(PHY_VARS_eNB*, int, int, uint16_t*, uint32_t*);
