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

/*! \file nr_rrc_config.c
 * \brief rrc config for gNB
 * \author Raymond Knopp, WEI-TAI CHEN
 * \date 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#ifndef __NR_RRC_CONFIG_H__
#define __NR_RRC_CONFIG_H__

#include "nr_rrc_defs.h"
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"

// forward declaration of MAC configuration parameters, definition is included in C file
struct nr_mac_config_t;
typedef struct nr_mac_config_t nr_mac_config_t;

void nr_rrc_config_dl_tda(struct NR_PDSCH_TimeDomainResourceAllocationList *pdsch_TimeDomainAllocationList,
                          frame_type_t frame_type,
                          NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,
                          int curr_bwp);
void nr_rrc_config_ul_tda(NR_ServingCellConfigCommon_t *scc, int min_fb_delay);
NR_SearchSpace_t *rrc_searchspace_config(bool is_common, int searchspaceid, int coresetid);

void prepare_sim_uecap(NR_UE_NR_Capability_t *cap,
                       NR_ServingCellConfigCommon_t *scc,
                       int numerology,
                       int rbsize,
                       int mcs_table_dl,
                       int mcs_table_ul);

NR_BCCH_BCH_Message_t *get_new_MIB_NR(const NR_ServingCellConfigCommon_t *scc);
void free_MIB_NR(NR_BCCH_BCH_Message_t *mib);
int encode_MIB_NR(NR_BCCH_BCH_Message_t *mib, int frame, uint8_t *buf, int buf_size);


#define NR_MAX_SIB_LENGTH 2976 // 3GPP TS 38.331 section 5.2.1
NR_BCCH_DL_SCH_Message_t *get_SIB1_NR(const NR_ServingCellConfigCommon_t *scc, const f1ap_plmn_t *plmn, uint64_t cellID, int tac);
void free_SIB1_NR(NR_BCCH_DL_SCH_Message_t *sib1);
int encode_SIB1_NR(NR_BCCH_DL_SCH_Message_t *sib1, uint8_t *buffer, int max_buffer_size);

NR_CellGroupConfig_t *get_initial_cellGroupConfig(int uid,
                                                  const NR_ServingCellConfigCommon_t *scc,
                                                  const NR_ServingCellConfig_t *servingcellconfigdedicated,
                                                  const nr_mac_config_t *configuration);
void update_cellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig,
                            const int uid,
                            NR_UE_NR_Capability_t *uecap,
                            const nr_mac_config_t *configuration,
                            const NR_ServingCellConfigCommon_t *scc);
void free_cellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig);
int encode_cellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig, uint8_t *buffer, int max_buffer_size);
NR_CellGroupConfig_t *decode_cellGroupConfig(const uint8_t *buffer, int max_buffer_size);

/* Note: this function returns a new CellGroupConfig for a user with given
 * configuration, but it will also overwrite the ServingCellConfig passed in
 * parameter servingcellconfigdedicated! */
NR_CellGroupConfig_t *get_default_secondaryCellGroup(const NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                                                     NR_ServingCellConfig_t *servingcellconfigdedicated,
                                                     const NR_UE_NR_Capability_t *uecap,
                                                     int scg_id,
                                                     int servCellIndex,
                                                     const nr_mac_config_t *configuration,
                                                     int uid);

#endif
