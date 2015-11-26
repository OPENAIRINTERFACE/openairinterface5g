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

#ifndef UDP_MESSAGES_TYPES_H_
#define UDP_MESSAGES_TYPES_H_

#define UDP_INIT(mSGpTR)    (mSGpTR)->ittiMsg.udp_init

typedef struct {
  uint32_t  port;
  char     *address;
} udp_init_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  buffer_offset;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_data_req_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_data_ind_t;

#endif /* UDP_MESSAGES_TYPES_H_ */
