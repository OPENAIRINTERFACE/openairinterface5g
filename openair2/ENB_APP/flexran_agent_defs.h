/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file flexran_agent_defs.h
 * \brief FlexRAN agent common definitions 
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */
#ifndef FLEXRAN_AGENT_DEFS_H_
#define FLEXRAN_AGENT_DEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "link_manager.h"

#define NUM_MAX_ENB 2
#define NUM_MAX_UE 2048
#define DEFAULT_FLEXRAN_AGENT_IPv4_ADDRESS "127.0.0.1"
#define DEFAULT_FLEXRAN_AGENT_PORT          2210
#define DEFAULT_FLEXRAN_AGENT_CACHE        "/mnt/oai_agent_cache"

typedef enum {
  
  FLEXRAN_AGENT_DEFAULT=0,
  
  FLEXRAN_AGENT_PHY=1,
  FLEXRAN_AGENT_MAC=2,
  FLEXRAN_AGENT_RLC=3,
  FLEXRAN_AGENT_PDCP=4,
  FLEXRAN_AGENT_RRC=5,
  FLEXRAN_AGENT_S1AP=6,
  FLEXRAN_AGENT_GTP=7,
  FLEXRAN_AGENT_X2AP=8,

  FLEXRAN_AGENT_MAX=9,
    
} agent_id_t;

typedef enum {
  /* no action  */
  FLEXRAN_AGENT_ACTION_NONE = 0x0,

  /* send action  */
  FLEXRAN_AGENT_ACTION_SEND = 0x1,

 /* apply action  */
  FLEXRAN_AGENT_ACTION_APPLY = 0x2,

  /* clear action  */
  FLEXRAN_AGENT_ACTION_CLEAR = 0x4,

  /* write action  */
  FLEXRAN_AGENT_ACTION_WRITE = 0x8,

  /* filter action  */
  FLEXRAN_AGENT_ACTION_FILTER = 0x10,

  /* preprocess action  */
  FLEXRAN_AGENT_ACTION_PREPROCESS = 0x20,

  /* meter action  */
  FLEXRAN_AGENT_ACTION_METER = 0x40,
  
  /* Max number of states available */
  FLEXRAN_AGENT_ACTION_MAX = 0x7f,
} agent_action_t;


typedef enum {
  
  RAN_LTE_OAI= 0,
  
 /* Max number of states available */
 RAN_NAME_MAX = 0x7f,
} ran_name_t;

typedef uint8_t xid_t;  
typedef uint8_t mid_t;  // module or enb id 
typedef uint8_t lcid_t;
typedef int32_t  err_code_t; 



typedef struct {
  /* general info */ 
 
  /* stats */

  uint32_t total_rx_msg;
  uint32_t total_tx_msg;
   
  uint32_t rx_msg[NUM_MAX_ENB];
  uint32_t tx_msg[NUM_MAX_ENB];

} flexran_agent_info_t;

typedef struct {
  mid_t enb_id;
  flexran_agent_info_t agent_info;
  
} flexran_agent_instance_t;

#endif 
