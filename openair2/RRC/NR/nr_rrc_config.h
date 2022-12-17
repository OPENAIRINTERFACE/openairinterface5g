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

#define asn1cCallocOne(VaR, VaLue) \
  VaR = calloc(1,sizeof(*VaR)); *VaR=VaLue;
#define asn1cCalloc(VaR, lOcPtr) \
  typeof(VaR) lOcPtr = VaR = calloc(1,sizeof(*VaR));
#define asn1cSequenceAdd(VaR, TyPe, lOcPtr) \
  TyPe *lOcPtr= calloc(1,sizeof(TyPe)); \
  ASN_SEQUENCE_ADD(&VaR,lOcPtr);

void set_phr_config(NR_MAC_CellGroupConfig_t *mac_CellGroupConfig);
uint64_t get_ssb_bitmap(const NR_ServingCellConfigCommon_t *scc);
void rrc_coreset_config(NR_ControlResourceSet_t *coreset,
                        int bwp_id,
                        int curr_bwp,
                        uint64_t ssb_bitmap);
NR_SearchSpace_t *rrc_searchspace_config(bool is_common,
                                         int searchspaceid,
                                         int coresetid);
void nr_rrc_config_dl_tda(struct NR_PDSCH_TimeDomainResourceAllocationList *pdsch_TimeDomainAllocationList,
                          frame_type_t frame_type,
                          NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,
                          int curr_bwp);
void nr_rrc_config_ul_tda(NR_ServingCellConfigCommon_t *scc, int min_fb_delay);
void config_pucch_resset0(NR_PUCCH_Config_t *pucch_Config, int uid, int curr_bwp, NR_UE_NR_Capability_t *uecap);
void config_pucch_resset1(NR_PUCCH_Config_t *pucch_Config, NR_UE_NR_Capability_t *uecap);
void set_dl_DataToUL_ACK(NR_PUCCH_Config_t *pucch_Config, int min_feedback_time);
void set_pucch_power_config(NR_PUCCH_Config_t *pucch_Config, int do_csirs);
void scheduling_request_config(const NR_ServingCellConfigCommon_t *scc,
                               NR_PUCCH_Config_t *pucch_Config);
void config_rsrp_meas_report(NR_CSI_MeasConfig_t *csi_MeasConfig, const NR_ServingCellConfigCommon_t *servingcellconfigcommon, NR_PUCCH_CSI_Resource_t *pucchcsires, int do_csi, int rep_id, int uid);
void config_csi_meas_report(NR_CSI_MeasConfig_t *csi_MeasConfig,
                            const NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                            NR_PUCCH_CSI_Resource_t *pucchcsires,
                            struct NR_SetupRelease_PDSCH_Config *pdsch_Config,
                            const rrc_pdsch_AntennaPorts_t *antennaports,
                            const int max_layers,
                            int rep_id,
                            int uid);
void config_csirs(const NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                  NR_CSI_MeasConfig_t *csi_MeasConfig,
                  int uid,
                  int num_dl_antenna_ports,
                  int curr_bwp,
                  int do_csirs,
                  int id);
void config_csiim(int do_csirs, int dl_antenna_ports, int curr_bwp,
                  NR_CSI_MeasConfig_t *csi_MeasConfig, int id);
void config_srs(const NR_ServingCellConfigCommon_t *scc,
                NR_SetupRelease_SRS_Config_t *setup_release_srs_Config,
                const NR_UE_NR_Capability_t *uecap,
                const int curr_bwp,
                const int uid,
                const int res_id,
                const long maxMIMO_Layers,
                const int do_srs);
struct NR_SetupRelease_PDSCH_Config *config_pdsch(uint64_t ssb_bitmap, int bwp_Id, int dl_antenna_ports);
void set_dl_mcs_table(int scs,
                      NR_UE_NR_Capability_t *cap,
                      NR_BWP_DownlinkDedicated_t *bwp_Dedicated,
                      const NR_ServingCellConfigCommon_t *scc);
void prepare_sim_uecap(NR_UE_NR_Capability_t *cap,
                       NR_ServingCellConfigCommon_t *scc,
                       int numerology,
                       int rbsize,
                       int mcs_table);
struct NR_SetupRelease_PUSCH_Config *config_pusch(NR_PUSCH_Config_t *pusch_Config, const NR_ServingCellConfigCommon_t *scc);
void config_downlinkBWP(NR_BWP_Downlink_t *bwp,
                        const NR_ServingCellConfigCommon_t *scc,
                        const NR_ServingCellConfig_t *servingcellconfigdedicated,
                        NR_UE_NR_Capability_t *uecap,
                        int dl_antenna_ports,
                        bool force_256qam_off,
                        int bwp_loop, bool is_SA);
void config_uplinkBWP(NR_BWP_Uplink_t *ubwp,
                      long bwp_loop, bool is_SA, int uid,
                      const gNB_RrcConfigurationReq *configuration,
                      const NR_ServingCellConfig_t *servingcellconfigdedicated,
                      const NR_ServingCellConfigCommon_t *scc,
                      NR_UE_NR_Capability_t *uecap);

#endif
