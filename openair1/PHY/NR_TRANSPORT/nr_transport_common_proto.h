
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

/*! \file PHY/NR_TRANSPORT/nr_mcs.c
* \brief Some support routines for NR MCS computations
* \author
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef __NR_TRANSPORT_COMMON_PROTO__H__
#define __NR_TRANSPORT_COMMON_PROTO__H__

#define MAX_NUM_NR_DLSCH_SEGMENTS 16
#define MAX_NUM_NR_ULSCH_SEGMENTS MAX_NUM_NR_DLSCH_SEGMENTS

#define MAX_NR_DLSCH_PAYLOAD_BYTES (MAX_NUM_NR_DLSCH_SEGMENTS*1056)
#define MAX_NR_ULSCH_PAYLOAD_BYTES (MAX_NUM_NR_DLSCH_SEGMENTS*1056)

#define MAX_NUM_NR_CHANNEL_BITS (14*273*12*6)  // 14 symbols, 273 RB
#define MAX_NUM_NR_RE (14*273*12)

// Functions below implement minor procedures from 38-214

/** \brief Computes Q based on I_MCS PDSCH and when 'MCS-Table-PDSCH' is set to "256QAM". Implements Table 5.1.3.1-2 from 38.214.
    @param I_MCS */
uint8_t get_nr_Qm(uint8_t I_MCS);

/** \brief Computes Q based on I_MCS PUSCH. Implements Table 6.1.4.1-1 from 38.214.
    @param I_MCS */
uint8_t get_nr_Qm_ul(uint8_t I_MCS);

#endif
