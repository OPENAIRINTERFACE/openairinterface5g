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
