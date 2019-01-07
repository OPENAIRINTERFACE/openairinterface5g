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

/*! \file PHY/LTE_TRANSPORT/if5_tools.h
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#ifndef __IF5_TOOLS_H__
#define __IF5_TOOLS_H__

#include <stdint.h>
#include "PHY/defs_eNB.h"

#define IF5_RRH_GW_DL 0x0022
#define IF5_RRH_GW_UL 0x0023
#define IF5_MOBIPASS 0xbffe

struct IF5_mobipass_header {  
  /// 
  uint16_t flags; 
  /// 
  uint16_t fifo_status;
  /// 
  uint8_t seqno;
  ///
  uint8_t ack;
  ///
  uint32_t word0;
  /// 
  uint32_t time_stamp;
  
} __attribute__ ((__packed__));

typedef struct IF5_mobipass_header IF5_mobipass_header_t;
#define sizeof_IF5_mobipass_header_t 14

void send_IF5(RU_t *, openair0_timestamp, int, uint8_t*, uint16_t);

void recv_IF5(RU_t *, openair0_timestamp*, int, uint16_t);

void malloc_IF5_buffer(RU_t *ru);

#endif

