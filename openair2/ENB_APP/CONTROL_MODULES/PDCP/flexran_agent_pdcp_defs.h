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
#ifndef __FLEXRAN_AGENT_PDCP_PRIMITIVES_H__
#define __FLEXRAN_AGENT_PDCP_PRIMITIVES_H__

#include "flexran_agent_defs.h"
#include "flexran.pb-c.h"
#include "header.pb-c.h"

 /*PDCP aggregated Packet stats  */
/*
typedef struct  pdcp_aggr_stats_s {
  int32_t rnti; 

  int32_t pkt_tx;
  int32_t pkt_tx_bytes;
  int32_t pkt_tx_sn;
  int32_t pkt_tx_rate_s;
  int32_t pkt_tx_throughput_s;
  int32_t pkt_tx_aiat;
  int32_t pkt_tx_aiat_s;

  int32_t pkt_rx;
  int32_t pkt_rx_bytes;
  int32_t pkt_rx_sn;
  int32_t pkt_rx_rate_s;
  int32_t pkt_rx_goodput_s;
  int32_t pkt_rx_aiat;
  int32_t pkt_rx_aiat_s;
  int32_t pkt_rx_oo;

  
} pdcp_aggr_stats_t;
*/

/* FLEXRAN AGENT-PDCP Interface */
typedef struct {
  
  
  // PDCP statistics
  void (*flexran_pdcp_stats_measurement)(mid_t mod_id, uint16_t rnti, uint16_t seq_num,  uint32_t size);
  
} AGENT_PDCP_xface;

#endif
