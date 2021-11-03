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
* \brief common MAC function prototypes
* \author Navid Nikaein and Raymond Knopp, WIE-TAI CHEN
* \date Dec. 2019
* \version 0.1
* \company Eurecom
* \email raymond.knopp@eurecom.fr
*/

#ifndef __LAYER2_NR_MAC_COMMON_H__
#define __LAYER2_NR_MAC_COMMON_H__

#include "NR_MIB.h"
#include "NR_PDSCH-Config.h"
#include "NR_CellGroupConfig.h"
#include "nr_mac.h"
#include "openair1/PHY/impl_defs_nr.h"

uint16_t config_bandwidth(int mu, int nb_rb, int nr_band);

lte_frame_type_t get_frame_type(uint16_t nr_bandP, uint8_t scs_index);

int32_t get_delta_duplex(int nr_bandP, uint8_t scs_index);

uint64_t from_nrarfcn(int nr_bandP, uint8_t scs_index, uint32_t dl_nrarfcn);

uint32_t to_nrarfcn(int nr_bandP, uint64_t dl_CarrierFreq, uint8_t scs_index, uint32_t bw);

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols,int startSymbol,int mappingtype_fromDCI);

int is_nr_DL_slot(NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,slot_t slotP);

int is_nr_UL_slot(NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon, slot_t slotP, lte_frame_type_t frame_type);

uint16_t nr_dci_size(const NR_BWP_DownlinkCommon_t *initialDLBWP,
                     const NR_BWP_UplinkCommon_t *initialULBWP,
                     const NR_CellGroupConfig_t *cg,
                     dci_pdu_rel15_t *dci_pdu,
                     nr_dci_format_t format,
                     nr_rnti_type_t rnti_type,
                     uint16_t N_RB,
                     int bwp_id);


void get_num_re_dmrs(int start_symbol,
                     uint8_t number_of_symbols,
                     uint16_t ul_dmrs_symb_pos,
                     uint8_t dmrs_type,
                     uint8_t cdm_grps_no_data,
                     uint8_t *nb_dmrs_re_per_rb,
                     uint16_t *number_dmrs_symbols);

void find_aggregation_candidates(uint8_t *aggregation_level,
                                 uint8_t *nr_of_candidates,
                                 NR_SearchSpace_t *ss,
                                 int maxL);

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
                                 uint8_t *N_dur,
                                 uint16_t *RA_sfn_index,
                                 uint8_t *N_RA_slot,
                                 uint8_t *config_period);

int get_nr_prach_occasion_info_from_index(uint8_t index,
                                 uint32_t pointa,
                                 uint8_t mu,
                                 uint8_t unpaired,
                                 uint16_t *format,
                                 uint8_t *start_symbol,
                                 uint8_t *N_t_slot,
                                 uint8_t *N_dur,
                                 uint8_t *N_RA_slot,
                                 uint16_t *N_RA_sfn,
                                 uint8_t *max_association_period);

uint8_t get_nr_prach_duration(uint8_t prach_format);

uint8_t get_pusch_mcs_table(long *mcs_Table,
                            int is_tp,
                            int dci_format,
                            int rnti_type,
                            int target_ss,
                            bool config_grant);

uint8_t compute_nr_root_seq(NR_RACH_ConfigCommon_t *rach_config,
                            uint8_t nb_preambles,
                            uint8_t unpaired,
			    frequency_range_t);

int ul_ant_bits(NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig,long transformPrecoder);

int get_format0(uint8_t index, uint8_t unpaired,frequency_range_t);

int64_t *get_prach_config_info(frequency_range_t freq_range,
                               uint8_t index,
                               uint8_t unpaired);

uint16_t get_NCS(uint8_t index, uint16_t format, uint8_t restricted_set_config);

int get_num_dmrs(uint16_t dmrs_mask );
uint8_t get_l0_ul(uint8_t mapping_type, uint8_t dmrs_typeA_position);
int32_t get_l_prime(uint8_t duration_in_symbols, uint8_t mapping_type, pusch_dmrs_AdditionalPosition_t additional_pos, pusch_maxLength_t pusch_maxLength, uint8_t start_symbolt, uint8_t dmrs_typeA_position);

uint8_t get_L_ptrs(uint8_t mcs1, uint8_t mcs2, uint8_t mcs3, uint8_t I_mcs, uint8_t mcs_table);
uint8_t get_K_ptrs(uint16_t nrb0, uint16_t nrb1, uint16_t N_RB);

void get_type0_PDCCH_CSS_config_parameters(NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config,
                                           frame_t frameP,
                                           NR_MIB_t *mib,
                                           uint8_t num_slot_per_frame,
                                           uint8_t ssb_subcarrier_offset,
                                           uint16_t ssb_start_symbol,
                                           NR_SubcarrierSpacing_t scs_ssb,
                                           frequency_range_t frequency_range,
                                           uint32_t ssb_index,
                                           uint32_t ssb_period,
                                           uint32_t ssb_offset_point_a);

uint16_t get_ssb_start_symbol(const long band, NR_SubcarrierSpacing_t scs, int i_ssb);

void get_info_from_tda_tables(int default_abc,
                              int tda,
                              int dmrs_TypeA_Position,
                              int normal_CP,
                              int *startSymbolIndex,
                              int *nrOfSymbols);

void fill_coresetZero(NR_ControlResourceSet_t *coreset0, NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config);
void fill_searchSpaceZero(NR_SearchSpace_t *ss0, NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config);

uint16_t compute_pucch_prb_size(uint8_t format,
                                uint8_t nr_prbs,
                                uint16_t O_tot,
                                uint16_t O_csi,
                                NR_PUCCH_MaxCodeRate_t *maxCodeRate,
                                uint8_t Qm,
                                uint8_t n_symb,
                                uint8_t n_re_ctrl);

int16_t get_N_RA_RB (int delta_f_RA_PRACH,int delta_f_PUSCH);

void find_period_offest_SR (NR_SchedulingRequestResourceConfig_t *SchedulingReqRec, int *period, int *offset);

void csi_period_offset(NR_CSI_ReportConfig_t *csirep,
                       NR_NZP_CSI_RS_Resource_t *nzpcsi,
                       int *period, int *offset);

void reverse_n_bits(uint8_t *value, uint16_t bitlen);

bool set_dl_ptrs_values(NR_PTRS_DownlinkConfig_t *ptrs_config,
                        uint16_t rbSize, uint8_t mcsIndex, uint8_t mcsTable,
                        uint8_t *K_ptrs, uint8_t *L_ptrs,uint8_t *portIndex,
                        uint8_t *nERatio,uint8_t *reOffset,
                        uint8_t NrOfSymbols);

bool set_ul_ptrs_values(NR_PTRS_UplinkConfig_t *ul_ptrs_config,
                        uint16_t rbSize,uint8_t mcsIndex, uint8_t mcsTable,
                        uint8_t *K_ptrs, uint8_t *L_ptrs,
                        uint8_t *reOffset, uint8_t *maxNumPorts, uint8_t *ulPower,
                        uint8_t NrOfSymbols);

uint8_t get_num_dmrs_symbols(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols,int startSymbol,int mappingtype);

/* \brief Set the transform precoding according to 6.1.3 of 3GPP TS 38.214 version 16.3.0 Release 16
@param    *pusch_config,   pointer to pusch config
@param    *ubwp            pointer to uplink bwp
@param    *dci_format      pointer to dci format
@param    rnti_type        rnti type
@param    configuredGrant  indicates whether a configured grant was received or not
@returns                   transformPrecoding value */
uint8_t get_transformPrecoding(const NR_BWP_UplinkCommon_t *initialUplinkBWP,
                               const NR_PUSCH_Config_t *pusch_config,
                               const NR_BWP_UplinkDedicated_t *ubwp,
                               uint8_t *dci_format,
                               int rnti_type,
                               uint8_t configuredGrant);

void nr_mac_gNB_rrc_ul_failure(const module_id_t Mod_instP,
                            const int CC_idP,
                            const frame_t frameP,
                            const sub_frame_t subframeP,
                            const rnti_t rntiP) ;
#endif
