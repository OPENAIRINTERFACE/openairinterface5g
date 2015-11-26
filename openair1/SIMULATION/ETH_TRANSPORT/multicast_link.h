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
/*! \file multicast.h
 *  \brief
 *  \author Lionel Gauthier and Navid Nikaein
 *  \date 2011
 *  \version 1.1
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */

#ifndef __MULTICAST_LINK_H__
#    define __MULTICAST_LINK_H__
#    ifdef MULTICAST_LINK_C
#        define private_multicast_link(x) x
#        define public_multicast_link(x) x
#    else
#        define private_multicast_link(x)
#        define public_multicast_link(x) extern x
#    endif
#    include "stdint.h"

private_multicast_link (typedef struct multicast_group_t {
  int      socket;
  struct sockaddr_in sock_remote_addr;
  char     host_addr[16];
  uint16_t port;
  char     rx_buffer[40000];
} multicast_group_t;)

#define MULTICAST_LINK_NUM_GROUPS 4

extern const char *multicast_group_list[MULTICAST_LINK_NUM_GROUPS];

private_multicast_link(void  multicast_link_init (void));
private_multicast_link(void  multicast_link_read_data (int groupP));
private_multicast_link(void  multicast_link_read (void));
private_multicast_link(void *multicast_link_main_loop (void *param));

public_multicast_link(int   multicast_link_write_sock (int groupP, char *dataP, uint32_t sizeP));
public_multicast_link(void  multicast_link_start (  void (*rx_handlerP) (unsigned int, char*), unsigned char multicast_group, char * multicast_ifname));
# ifdef BYPASS_PHY
public_multicast_link(pthread_mutex_t Bypass_phy_wr_mutex);
public_multicast_link(pthread_cond_t Bypass_phy_wr_cond);
public_multicast_link(char Bypass_phy_wr);
# endif //BYPASS_PHY
#endif
