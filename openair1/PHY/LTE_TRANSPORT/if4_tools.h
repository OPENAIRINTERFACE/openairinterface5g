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

/// IF4 PRACH Configuration (32 bits)
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

/// IF4 Antenna Gain
struct IF4_gain {
  /// Reserved 
  uint32_t rsvd:10;
  /// FFT Exponent Output
  uint32_t exponent:6;  
};  
  
struct IF4_dl_packet {
  /// Destination Address
  
  /// Source Address
  
  /// Type
  
  /// Sub-Type
  
  /// Reserved
  
  /// Frame Status
  
  /// Data Blocks
  
  /// Frame Check Sequence
   
};

struct IF4_ul_packet {
  /// Destination Address
  
  /// Source Address
  
  /// Type
  
  /// Sub-Type
  
  /// Reserved
  
  /// Frame Status
  
  /// Gain 0
  /// Gain 1
  /// Gain 2
  /// Gain 3
  /// Gain 4
  /// Gain 5
  /// Gain 6
  /// Gain 7

  /// Data Blocks
  
  /// Frame Check Sequence
  uint32_t fcs;
};

struct IF4_prach_packet {
  
  
};

void send_IF4(PHY_VARS_eNB *eNB, int subframe);

void recv_IF4(PHY_VARS_eNB *eNB, int subframe);
