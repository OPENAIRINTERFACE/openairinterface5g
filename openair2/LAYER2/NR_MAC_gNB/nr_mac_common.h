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

/*! \file mac.h
* \brief MAC data structures, constant, and function prototype
* \author Navid Nikaein and Raymond Knopp, WIE-TAI CHEN
* \date Dec. 2019
* \version 0.1
* \company Eurecom
* \email raymond.knopp@eurecom.fr

*/

#ifndef __LAYER2_NR_MAC_COMMON_H__
#define __LAYER2_NR_MAC_COMMON_H__

uint16_t config_bandwidth(int mu, int nb_rb, int nr_band);

uint64_t from_nrarfcn(int nr_bandP, uint8_t scs_index, uint32_t dl_nrarfcn);

uint32_t to_nrarfcn(int nr_bandP, uint64_t dl_CarrierFreq, uint8_t scs_index, uint32_t bw);

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols);

typedef enum {
  NR_DL_DCI_FORMAT_1_0 = 0,
  NR_DL_DCI_FORMAT_1_1,
  NR_DL_DCI_FORMAT_2_0,
  NR_DL_DCI_FORMAT_2_1,
  NR_DL_DCI_FORMAT_2_2,
  NR_DL_DCI_FORMAT_2_3,
  NR_UL_DCI_FORMAT_0_0,
  NR_UL_DCI_FORMAT_0_1
} nr_dci_format_t;

typedef enum {
  NR_RNTI_new = 0,
  NR_RNTI_C,
  NR_RNTI_RA,
  NR_RNTI_P,
  NR_RNTI_CS,
  NR_RNTI_TC,
  NR_RNTI_SP_CSI,
  NR_RNTI_SI,
  NR_RNTI_SFI,
  NR_RNTI_INT,
  NR_RNTI_TPC_PUSCH,
  NR_RNTI_TPC_PUCCH,
  NR_RNTI_TPC_SRS
} nr_rnti_type_t;

#endif
