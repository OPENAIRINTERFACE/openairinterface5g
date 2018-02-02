/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
