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

#include "NR_PDSCH-Config.h"
#include "NR_CellGroupConfig.h"
#include "nr_mac.h"

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

uint16_t config_bandwidth(int mu, int nb_rb, int nr_band);

void get_band(uint64_t downlink_frequency, uint16_t *current_band, int32_t *current_offset, lte_frame_type_t *current_type);

uint64_t from_nrarfcn(int nr_bandP, uint8_t scs_index, uint32_t dl_nrarfcn);

uint32_t to_nrarfcn(int nr_bandP, uint64_t dl_CarrierFreq, uint8_t scs_index, uint32_t bw);

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols);

int is_nr_DL_slot(NR_ServingCellConfigCommon_t *scc,slot_t slotP);

int is_nr_UL_slot(NR_ServingCellConfigCommon_t *scc,slot_t slotP);

uint16_t nr_dci_size(NR_ServingCellConfigCommon_t *scc,
                     NR_CellGroupConfig_t *secondaryCellGroup,
                     dci_pdu_rel15_t *dci_pdu,
                     nr_dci_format_t format,
		     nr_rnti_type_t rnti_type,
		     uint16_t N_RB,
                     int bwp_id);

void find_monitoring_periodicity_offset_common(NR_SearchSpace_t *ss,
                                               uint16_t *slot_period,
                                               uint16_t *offset);

int get_nr_prach_info_from_index(uint8_t index,
                                 int frame,
                                 int slot,
                                 uint32_t pointa,
                                 uint8_t mu,
                                 uint8_t unpaired,
                                 uint16_t *format,
                                 uint8_t *start_symbol,
                                 uint8_t *N_t_slot,
                                 uint8_t *N_dur);

uint8_t compute_nr_root_seq(NR_RACH_ConfigCommon_t *rach_config,
                            uint8_t nb_preambles,
                            uint8_t unpaired);

int ul_ant_bits(NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig,long transformPrecoder);

int get_format0(uint8_t index, uint8_t unpaired);

uint16_t get_NCS(uint8_t index, uint16_t format, uint8_t restricted_set_config);

int get_num_dmrs(uint16_t dmrs_mask );
uint8_t get_l0_ul(uint8_t mapping_type, uint8_t dmrs_typeA_position);
int32_t get_l_prime(uint8_t duration_in_symbols, uint8_t mapping_type, pusch_dmrs_AdditionalPosition_t additional_pos, pusch_maxLength_t pusch_maxLength);

uint8_t get_L_ptrs(uint8_t mcs1, uint8_t mcs2, uint8_t mcs3, uint8_t I_mcs, uint8_t mcs_table);
uint8_t get_K_ptrs(uint16_t nrb0, uint16_t nrb1, uint16_t N_RB);

#endif
