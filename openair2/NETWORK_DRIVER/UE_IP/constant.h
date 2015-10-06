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


#ifndef _UE_IP_CST
#define _UE_IP_CST

#define UE_IP_MAX_LENGTH 180

// General Constants
#define UE_IP_MTU                    1500
#define UE_IP_TX_QUEUE_LEN           100
#define UE_IP_ADDR_LEN               8
#define UE_IP_INET6_ADDRSTRLEN       46
#define UE_IP_INET_ADDRSTRLEN        16
#define UE_IP_DEFAULT_RAB_ID         1

#define UE_IP_RESET_RX_FLAGS         0


#define UE_IP_RETRY_LIMIT_DEFAULT    (int)5

#define UE_IP_MESSAGE_MAXLEN         (int)5004

#define UE_IP_TIMER_ESTABLISHMENT_DEFAULT (int)12
#define UE_IP_TIMER_RELEASE_DEFAULT       (int)2
#define UE_IP_TIMER_IDLE                  UINT_MAX
#define UE_IP_TIMER_TICK                  HZ

#define UE_IP_PDCPH_SIZE                  (int)sizeof(struct pdcp_data_req_header_s)
#define UE_IP_IPV4_SIZE                   (int)20
#define UE_IP_IPV6_SIZE                   (int)40




#define UE_IP_NB_INSTANCES_MAX       8


#endif

