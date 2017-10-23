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

#ifndef _IOCTL_H
#define _IOCTL_H

#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <linux/in6.h>
//#include <linux/netdevice.h>

//#include <graal_constant.h>
//#define GRAAL_STATE_IDLE      0
//#define GRAAL_STATE_CONNECTED     1
//#define GRAAL_STATE_ESTABLISHMENT_REQUEST 2
//#define GRAAL_STATE_ESTABLISHMENT_FAILURE 3
//#define GRAAL_STATE_RELEASE_FAILURE   4

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

extern int inet_pton(int af, const char *src, void *dst);
extern char *inet_ntop(int af, const void *src, char *dst, size_t sise);

#endif //_IOCTL_H
