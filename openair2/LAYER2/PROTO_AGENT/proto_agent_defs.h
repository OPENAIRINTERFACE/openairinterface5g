/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2016 Eurecom

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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file enb_agent_defs.h
 * \brief enb agent common definitions 
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */
#ifndef PROTO_AGENT_DEFS_H_
#define PROTO_AGENT_DEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "openairinterface5g_limits.h"
#include "UTIL/ASYNC_IF/link_manager.h"

#define DEFAULT_PROTO_AGENT_IPv4_ADDRESS "127.0.0.1"
#define DEFAULT_PROTO_AGENT_PORT          2210
#define DEFAULT_PROTO_AGENT_CACHE        "/mnt/oai_agent_cache"

typedef enum {
  
  PROTO_AGENT_DEFAULT=0,
  
  ENB_AGENT_PHY=1,
  ENB_AGENT_MAC=2,
  ENB_AGENT_RLC=3,
  ENB_AGENT_PDCP=4,
  ENB_AGENT_RRC=5,
  ENB_AGENT_S1AP=6,
  ENB_AGENT_GTP=7,
  ENB_AGENT_X2AP=8,

  ENB_AGENT_MAX=9,
    
} proto_agent_id_t;

/*
typedef enum {
  ENB_AGENT_ACTION_NONE = 0x0,

  ENB_AGENT_ACTION_SEND = 0x1,

  ENB_AGENT_ACTION_APPLY = 0x2,

  ENB_AGENT_ACTION_CLEAR = 0x4,

  ENB_AGENT_ACTION_WRITE = 0x8,

  ENB_AGENT_ACTION_FILTER = 0x10,

  ENB_AGENT_ACTION_PREPROCESS = 0x20,

  ENB_AGENT_ACTION_METER = 0x40,
  
  ENB_AGENT_ACTION_MAX = 0x7f,
} agent_action_t;
*/
/*
typedef enum {
  
  RAN_LTE_OAI= 0,
  
 RAN_NAME_MAX = 0x7f,
} ran_name_t;
*/
typedef uint8_t xid_t;  
typedef uint8_t mod_id_t;  // module or enb id 
typedef uint8_t lcid_t;
typedef int32_t err_code_t;

typedef struct {
  /* general info */ 
 
  /* stats */

  uint32_t total_rx_msg;
  uint32_t total_tx_msg;
   
  uint32_t rx_msg[NUMBER_OF_eNB_MAX];
  uint32_t tx_msg[NUMBER_OF_eNB_MAX];

} proto_agent_info_t;

/* forward declaration */
struct proto_agent_channel_s;

typedef struct proto_agent_instance_s {
  mod_id_t    mod_id;
  proto_agent_info_t agent_info;
  struct proto_agent_channel_s *channel;
  pthread_t   recv_thread;
  uint8_t     exit;
} proto_agent_instance_t;

#endif 
