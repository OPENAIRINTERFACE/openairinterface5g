
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

/*! \file PHY/NR_TRANSPORT/nr_transport_common_proto.h
* \brief Some support routines
* \author
* \date 2019
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef __NR_TRANSPORT_COMMON_PROTO__H__
#define __NR_TRANSPORT_COMMON_PROTO__H__

#include "PHY/defs_nr_common.h"

#define MAX_NUM_NR_DLSCH_SEGMENTS 16
#define MAX_NUM_NR_ULSCH_SEGMENTS MAX_NUM_NR_DLSCH_SEGMENTS


#define NR_PUSCH_x 2 // UCI placeholder bit TS 38.212 V15.4.0 subclause 5.3.3.1
#define NR_PUSCH_y 3 // UCI placeholder bit 

/** \brief Computes Q based on I_MCS PDSCH and table_idx for downlink. Implements MCS Tables from 38.214. */
uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx);
uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx);

/** \brief Computes Q based on I_MCS PDSCH and table_idx for uplink. Implements MCS Tables from 38.214. */
uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx);
uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx);

void nr_get_tbs_dl(nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu,
                   nfapi_nr_dl_config_dci_dl_pdu dci_pdu,
                   nfapi_nr_config_request_t config);

/** \brief Computes available bits G. */
uint32_t nr_get_G(uint16_t nb_rb, uint16_t nb_symb_sch, uint8_t nb_re_dmrs, uint16_t length_dmrs, uint8_t Qm, uint8_t Nl);

uint32_t nr_get_E(uint32_t G, uint8_t C, uint8_t Qm, uint8_t Nl, uint8_t r);

#endif
