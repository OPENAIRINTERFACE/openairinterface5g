/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __NR_MAC_RRC_CONFIG_H__
#define __NR_MAC_RRC_CONFIG_H__

#include <stdbool.h>
#include <stdint.h>
#include "NR_BCCH-BCH-Message.h"
#include "NR_BCCH-DL-SCH-Message.h"
#include "NR_CellGroupConfig.h"
#include "NR_UE-NR-Capability.h"
#include "NR_MeasConfig.h"
#include "NR_MeasurementTimingConfiguration.h"
#include "NR_UL-CCCH-Message.h"
#include "f1ap_messages_types.h"
#include "common/platform_types.h"
#include "openair2/LAYER2/NR_MAC_COMMON/ue_capability_handler.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_configuration.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac.h"
struct NR_MeasurementTimingConfiguration;
struct NR_PDSCH_TimeDomainResourceAllocationList;

// forward declaration of MAC configuration parameters, definition is included in C file
typedef struct nr_mac_config_s nr_mac_config_t;
typedef enum nr_srs_type_e nr_srs_type_t;
typedef struct nr_mac_timers nr_mac_timers_t;
typedef struct measgap_config measgap_config_t;
typedef struct gNB_MAC_INST_s gNB_MAC_INST;
typedef struct nr_cell_sched_s nr_cell_sched_t;

void nr_rrc_config_dl_tda(NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList,
                          frame_type_t frame_type,
                          const NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,
                          int csi_symbols,
                          int len_coreset);
void nr_rrc_config_ul_tda(NR_ServingCellConfigCommon_t *scc, int min_fb_delay, nr_srs_type_t do_SRS);
NR_SearchSpace_t *rrc_searchspace_config(bool is_common,
                                         int searchspaceid,
                                         int coresetid,
                                         const int num_agg_level_candidates[NUM_PDCCH_AGG_LEVELS]);
int csi_symbols_in_slot(const NR_ServingCellConfigCommon_t *scc);
void prepare_sim_uecap(NR_UE_NR_Capability_t *cap,
                       NR_ServingCellConfigCommon_t *scc,
                       int numerology,
                       int rbsize,
                       int mcs_table_dl,
                       int mcs_table_ul);

NR_BCCH_BCH_Message_t *get_new_MIB_NR(const NR_ServingCellConfigCommon_t *scc);
void free_MIB_NR(NR_BCCH_BCH_Message_t *mib);
int encode_MIB_NR(NR_BCCH_BCH_Message_t *mib, int frame, uint8_t *buf, int buf_size);
int encode_MIB_NR_setup(NR_MIB_t *mib, int frame, uint8_t *buf, int buf_size);
int configure_coreset_for_mux23(const NR_ServingCellConfigCommon_t *scc,
                                int offset,
                                int limit,
                                int bwp_start,
                                int bwp_size,
                                bool do_TCI);
struct NR_MeasurementTimingConfiguration;
struct NR_MeasurementTimingConfiguration *get_new_MeasurementTimingConfiguration(const NR_ServingCellConfigCommon_t *scc);
int encode_MeasurementTimingConfiguration(const struct NR_MeasurementTimingConfiguration *mtc, uint8_t *buf, int buf_len);
void free_MeasurementTimingConfiguration(struct NR_MeasurementTimingConfiguration *mtc);

NR_BCCH_DL_SCH_Message_t *get_SIB1_NR(const NR_ServingCellConfigCommon_t *scc,
                                      const plmn_id_t *plmn,
                                      uint64_t cellID,
                                      int tac,
                                      const nr_mac_config_t *mac_config);
void update_SIB1_NR_SI(NR_BCCH_DL_SCH_Message_t *sib1, int num_sibs, int sibs[num_sibs]);
int encode_sysinfo_ie(NR_SystemInformation_IEs_t *sysInfo, uint8_t *buf, int len);
void free_SIB1_NR(NR_BCCH_DL_SCH_Message_t *sib1);
int encode_SIB_NR(NR_BCCH_DL_SCH_Message_t *sib, uint8_t *buffer, int max_buffer_size);
void add_sib_to_systeminformation(NR_SystemInformation_IEs_t *si, struct NR_SystemInformation_IEs__sib_TypeAndInfo__Member *type);
NR_SIB19_r17_t *get_SIB19_NR(const NR_ServingCellConfigCommon_t *scc);

NR_CellGroupConfig_t *get_initial_cellGroupConfig(int uid,
                                                  bool redcap,
                                                  const NR_ServingCellConfigCommon_t *scc,
                                                  const nr_cell_sched_t *cell,
                                                  const nr_rlc_configuration_t *default_rlc_config,
                                                  int ssb_index);
void update_cellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig,
                            const int uid,
                            const NR_UE_NR_Capability_t *uecap,
                            const NR_feature_set_ids_t *fs_ids,
                            const nr_cell_sched_t *cell,
                            const NR_ServingCellConfigCommon_t *scc);
int encode_cellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig, uint8_t *buffer, int max_buffer_size);

/* Note: this function returns a new CellGroupConfig for a user with given
 * configuration, but it will also overwrite the ServingCellConfig passed in
 * parameter servingcellconfigdedicated! */
NR_CellGroupConfig_t *get_default_secondaryCellGroup(const NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                                                     const NR_UE_NR_Capability_t *uecap,
                                                     const NR_feature_set_ids_t *fs_ids,
                                                     int scg_id,
                                                     int servCellIndex,
                                                     const nr_mac_config_t *configuration,
                                                     const nr_cell_sched_t *cell,
                                                     int uid,
                                                     int ssb_index);

NR_ReconfigurationWithSync_t *get_reconfiguration_with_sync(rnti_t rnti, uid_t uid, const NR_ServingCellConfigCommon_t *scc, int frame);

struct NR_RLC_Config *nr_srb_config(const nr_rlc_configuration_t *default_rlc_config);
struct NR_RLC_Config *nr_drb_config(NR_RLC_Config_PR rlc_config_pr, const nr_rlc_configuration_t *default_rlc_config);

NR_RLC_BearerConfig_t *get_SRB_RLC_BearerConfig(long channelId,
                                                long priority,
                                                long bucketSizeDuration,
                                                const nr_rlc_configuration_t *default_rlc_config);
NR_RLC_BearerConfig_t *get_DRB_RLC_BearerConfig(long lcChannelId,
                                                long drbId,
                                                NR_RLC_Config_PR rlc_conf,
                                                long priority,
                                                const nr_rlc_configuration_t *default_rlc_config);
NR_CellGroupConfig_t *update_cellGroupConfig_for_reconfig(NR_CellGroupConfig_t *cellGroupConfig,
                                                          const nr_cell_sched_t *cell,
                                                          const NR_UE_NR_Capability_t *uecap,
                                                          const NR_feature_set_ids_t *fs_ids,
                                                          const NR_ServingCellConfigCommon_t *scc,
                                                          int uid,
                                                          int old_bwp,
                                                          int new_bwp,
                                                          int ssb_index);
NR_MeasurementTimingConfiguration_t *get_nr_mtc(uint8_t *buf, uint32_t len);
measgap_config_t create_measgap_config(const NR_MeasurementTimingConfiguration_t *mtc, int scs, int min_rxtxtime);
int encode_measgap_config(const measgap_config_t *c, uint8_t *buf);
long ue_supported_ul_layers(const NR_UE_NR_Capability_t *uecap, int ul_id);
long ue_supported_dl_layers(const NR_ServingCellConfigCommon_t *scc, const NR_UE_NR_Capability_t *uecap, int dl_id);
void create_trp_info_item(const f1ap_trp_information_req_t *req,
                          f1ap_trp_information_t *trp_info_item,
                          positioning_config_t *positioning_config,
                          int trp_idx);
f1ap_srs_configuration_t cp_rrc_to_f1ap_srs_configuration(NR_UE_UL_BWP_t *current_UL_BWP, NR_ServingCellConfigCommon_t *scc);
#endif
