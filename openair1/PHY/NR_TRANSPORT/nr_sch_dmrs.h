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

/*! \file PHY/NR_TRANSPORT/nr_sch_dmrs.h
* \brief
* \author
* \date
* \version
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef NR_SCH_DMRS_H
#define NR_SCH_DMRS_H

#include "PHY/defs_nr_common.h"

#define NR_PDSCH_DMRS_ANTENNA_PORT0 1000
#define NR_PDSCH_DMRS_NB_ANTENNA_PORTS 12

void get_antenna_ports(uint8_t *ap, uint8_t n_symbs, uint8_t config);
void get_Wt(int *Wt, const int ap, const nfapi_nr_dmrs_type_e config);
void get_Wf(int *Wf, const int ap, const nfapi_nr_dmrs_type_e config);

uint8_t get_delta(uint8_t ap, uint8_t config);

uint16_t get_dmrs_freq_idx(uint16_t n, uint8_t k_prime, uint8_t delta, uint8_t dmrs_type);

uint8_t get_l0(uint16_t dlDmrsSymbPos);

#endif
