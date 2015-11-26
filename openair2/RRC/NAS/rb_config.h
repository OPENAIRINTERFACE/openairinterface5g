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

/** \file rb_config.h
 *  \brief Openair radio bearer configuration header file
 *  \author Raymond Knopp and Navid Nikaein
 */

#ifndef __RB_CONFIG_H__
#define __RB_CONFIG_H__

#include <netinet/in.h>
/*
typedef struct {

  int fd; // socket file descriptor

  int stats;

  int action;  // add or delete

  int rb;
  int cx;
  int inst;

  int saddr_ipv4set;
  int daddr_ipv4set;
  in_addr_t saddr_ipv4;
  in_addr_t daddr_ipv4;

  int saddr_ipv6set;
  int daddr_ipv6set;
  struct in6_addr saddr_ipv6;
  struct in6_addr daddr_ipv6;

  int dscp;



} rb_config;
*/
int rb_validate_config_ipv4(int cx, int inst, int rb);
int rb_conf_ipv4(int action,int cx, int inst, int rb, int dscp, in_addr_t saddr_ipv4, in_addr_t daddr_ipv4);
void rb_ioctl_init(int inst);
int rb_stats_req(int inst);
void init_socket(void);
in_addr_t ipv4_address (int thirdOctet, int fourthOctet);



#endif
