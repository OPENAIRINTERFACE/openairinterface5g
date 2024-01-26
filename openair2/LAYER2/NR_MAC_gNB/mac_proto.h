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

/*! \file mac_proto.h
 * \brief MAC functions prototypes for gNB
 * \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 */

#ifndef __LAYER2_NR_MAC_PROTO_H__
#define __LAYER2_NR_MAC_PROTO_H__

#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_TAG-Id.h"
#include "common/ngran_types.h"

void set_cset_offset(uint16_t);

void mac_top_init_gNB(ngran_node_t node_type,
                      NR_ServingCellConfigCommon_t *scc,
                      NR_ServingCellConfig_t *scd,
                      const nr_mac_config_t *conf);
void nr_mac_send_f1_setup_req(void);

void nr_mac_config_scc(gNB_MAC_INST *nrmac, NR_ServingCellConfigCommon_t *scc, const nr_mac_config_t *mac_config);
void nr_mac_configure_sib1(gNB_MAC_INST *nrmac, const f1ap_plmn_t *plmn, uint64_t cellID, int tac);

bool nr_mac_add_test_ue(gNB_MAC_INST *nrmac, uint32_t rnti, NR_CellGroupConfig_t *CellGroup);
bool nr_mac_prepare_ra_ue(gNB_MAC_INST *nrmac, uint32_t rnti, NR_CellGroupConfig_t *CellGroup);

bool nr_mac_prepare_cellgroup_update(gNB_MAC_INST *nrmac, NR_UE_info_t *UE, NR_CellGroupConfig_t *CellGroup);
int nr_mac_enable_ue_rrc_processing_timer(gNB_MAC_INST *mac, NR_UE_info_t *UE, bool apply_cellGroup);

void clear_nr_nfapi_information(gNB_MAC_INST *gNB,
                                int CC_idP,
                                frame_t frameP,
                                sub_frame_t slotP,
                                nfapi_nr_dl_tti_request_t *DL_req,
                                nfapi_nr_tx_data_request_t *TX_req,
                                nfapi_nr_ul_dci_request_t *UL_dci_req);

void nr_mac_update_timers(module_id_t module_id,
                          frame_t frame,
                          sub_frame_t slot);

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frame_rxP, sub_frame_t slot_rxP, NR_Sched_Rsp_t *sched_info);

/* \brief main DL scheduler function. Calls a preprocessor to decide on
 * resource allocation, then "post-processes" resource allocation (nFAPI
 * messages, statistics, HARQ handling, CEs, ... */
void nr_schedule_ue_spec(module_id_t module_id,
                         frame_t frame,
                         sub_frame_t slot,
                         nfapi_nr_dl_tti_request_t *DL_req,
                         nfapi_nr_tx_data_request_t *TX_req);

/* \brief default FR1 DL preprocessor init routine, returns preprocessor to call */
nr_pp_impl_dl nr_init_fr1_dlsch_preprocessor(int CC_id);

void schedule_nr_sib1(module_id_t module_idP,
                      frame_t frameP,
                      sub_frame_t slotP,
                      nfapi_nr_dl_tti_request_t *DL_req,
                      nfapi_nr_tx_data_request_t *TX_req);

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t slotP, nfapi_nr_dl_tti_request_t *DL_req);

/* \brief main UL scheduler function. Calls a preprocessor to decide on
 * resource allocation, then "post-processes" resource allocation (nFAPI
 * messages, statistics, HARQ handling, ... */
void nr_schedule_ulsch(module_id_t module_id, frame_t frame, sub_frame_t slot, nfapi_nr_ul_dci_request_t *ul_dci_req);

/* \brief default FR1 UL preprocessor init routine, returns preprocessor to call */
nr_pp_impl_ul nr_init_fr1_ulsch_preprocessor(int CC_id);

/////// Random Access MAC-PHY interface functions and primitives ///////

void nr_schedule_RA(module_id_t module_idP,
                    frame_t frameP,
                    sub_frame_t slotP,
                    nfapi_nr_ul_dci_request_t *ul_dci_req,
                    nfapi_nr_dl_tti_request_t *DL_req,
                    nfapi_nr_tx_data_request_t *TX_req);

/* \brief Function to indicate a received preamble on PRACH.  It initiates the RA procedure.
@param module_idP Instance ID of gNB
@param preamble_index index of the received RA request
@param slotP Slot number on which to act
@param timing_offset Offset in samples of the received PRACH w.r.t. eNB timing. This is used to
@param rnti RA rnti corresponding to this PRACH preamble
@param rach_resource type (0=non BL/CE,1 CE level 0,2 CE level 1, 3 CE level 2,4 CE level 3)
*/
void nr_initiate_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP,
                         uint16_t preamble_index, uint8_t freq_index, uint8_t symbol, int16_t timing_offset);

void nr_clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, NR_RA_t *ra);

int nr_allocate_CCEs(int module_idP, int CC_idP, frame_t frameP, sub_frame_t slotP, int test_only);

void schedule_nr_prach(module_id_t module_idP, frame_t frameP, sub_frame_t slotP);

uint16_t nr_mac_compute_RIV(uint16_t N_RB_DL, uint16_t RBstart, uint16_t Lcrbs);

/////// Phy test scheduler ///////

/* \brief preprocessor for phytest: schedules UE_id 0 with fixed MCS on all
 * freq resources */
void nr_preprocessor_phytest(module_id_t module_id,
                             frame_t frame,
                             sub_frame_t slot);
/* \brief UL preprocessor for phytest: schedules UE_id 0 with fixed MCS on a
 * fixed set of resources */
bool nr_ul_preprocessor_phytest(module_id_t module_id, frame_t frame, sub_frame_t slot);

void handle_nr_uci_pucch_0_1(module_id_t mod_id,
                             frame_t frame,
                             sub_frame_t slot,
                             const nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_01);
void handle_nr_uci_pucch_2_3_4(module_id_t mod_id,
                               frame_t frame,
                               sub_frame_t slot,
                               const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_234);

void config_uldci(const NR_UE_ServingCell_Info_t *sc_info,
                  const nfapi_nr_pusch_pdu_t *pusch_pdu,
                  dci_pdu_rel15_t *dci_pdu_rel15,
                  nr_srs_feedback_t *srs_feedback,
                  int time_domain_assignment,
                  uint8_t tpc,
                  uint8_t ndi,
                  NR_UE_UL_BWP_t *ul_bwp);

void nr_schedule_pucch(gNB_MAC_INST *nrmac,
                       frame_t frameP,
                       sub_frame_t slotP);

void nr_srs_ri_computation(const nfapi_nr_srs_normalized_channel_iq_matrix_t *nr_srs_normalized_channel_iq_matrix,
                           const NR_UE_UL_BWP_t *current_BWP,
                           uint8_t *ul_ri);

int get_pucch_resourceid(NR_PUCCH_Config_t *pucch_Config, int O_uci, int pucch_resource);

void nr_schedule_srs(int module_id, frame_t frame, int slot);

void nr_csirs_scheduling(int Mod_idP, frame_t frame, sub_frame_t slot, int n_slots_frame, nfapi_nr_dl_tti_request_t *DL_req);

void nr_csi_meas_reporting(int Mod_idP,
                           frame_t frameP,
                           sub_frame_t slotP);

int nr_acknack_scheduling(gNB_MAC_INST *mac,
                          NR_UE_info_t *UE,
                          frame_t frameP,
                          sub_frame_t slotP,
                          int r_pucch,
                          int do_common);

int get_pdsch_to_harq_feedback(NR_PUCCH_Config_t *pucch_Config,
                               nr_dci_format_t dci_format,
                               uint8_t *pdsch_to_harq_feedback);
  
int nr_get_pucch_resource(NR_ControlResourceSet_t *coreset,
                          NR_PUCCH_Config_t *pucch_Config,
                          int CCEIndex);

void nr_configure_pucch(nfapi_nr_pucch_pdu_t* pucch_pdu,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_UE_info_t* UE,
                        uint8_t pucch_resource,
                        uint16_t O_csi,
                        uint16_t O_ack,
                        uint8_t O_sr,
                        int r_pucch);

void find_search_space(int ss_type,
                       NR_BWP_Downlink_t *bwp,
                       NR_SearchSpace_t *ss);

void nr_configure_pdcch(nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu,
                        NR_ControlResourceSet_t *coreset,
                        NR_sched_pdcch_t *pdcch);

NR_sched_pdcch_t set_pdcch_structure(gNB_MAC_INST *gNB_mac,
                                     NR_SearchSpace_t *ss,
                                     NR_ControlResourceSet_t *coreset,
                                     NR_ServingCellConfigCommon_t *scc,
                                     NR_BWP_t *bwp,
                                     NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config);

int find_pdcch_candidate(const gNB_MAC_INST *mac,
                         int cc_id,
                         int aggregation,
                         int nr_of_candidates,
                         const NR_sched_pdcch_t *pdcch,
                         const NR_ControlResourceSet_t *coreset,
                         uint32_t Y);

void fill_pdcch_vrb_map(gNB_MAC_INST *mac,
                        int CC_id,
                        NR_sched_pdcch_t *pdcch,
                        int first_cce,
                        int aggregation);

void fill_dci_pdu_rel15(const NR_UE_ServingCell_Info_t *servingCellInfo,
                        const NR_UE_DL_BWP_t *current_DL_BWP,
                        const NR_UE_UL_BWP_t *current_UL_BWP,
                        nfapi_nr_dl_dci_pdu_t *pdcch_dci_pdu,
                        dci_pdu_rel15_t *dci_pdu_rel15,
                        int dci_format,
                        int rnti_type,
                        int bwp_id,
                        NR_SearchSpace_t *ss,
                        NR_ControlResourceSet_t *coreset,
                        long pdsch_HARQ_ACK_Codebook,
                        uint16_t cset0_bwp_size);

void prepare_dci(const NR_UE_ServingCell_Info_t *servingCellInfo,
                 const NR_UE_DL_BWP_t *current_BWP,
                 const NR_ControlResourceSet_t *coreset,
                 dci_pdu_rel15_t *dci_pdu_rel15,
                 nr_dci_format_t format);

void set_r_pucch_parms(int rsetindex,
                       int r_pucch,
                       int bwp_size,
                       int *prb_start,
                       int *second_hop_prb,
                       int *nr_of_symbols,
                       int *start_symbol_index);

/* find coreset within the search space */
NR_ControlResourceSet_t *get_coreset(gNB_MAC_INST *nrmac,
                                     NR_ServingCellConfigCommon_t *scc,
                                     void *bwp,
                                     NR_SearchSpace_t *ss,
                                     NR_SearchSpace__searchSpaceType_PR ss_type);

/* find a search space within a BWP */
NR_SearchSpace_t *get_searchspace(NR_ServingCellConfigCommon_t *scc,
                                  NR_BWP_DownlinkDedicated_t *bwp_Dedicated,
                                  NR_SearchSpace__searchSpaceType_PR target_ss);

long get_K2(NR_PUSCH_TimeDomainResourceAllocationList_t *tdaList,
            int time_domain_assignment,
            int mu);

const NR_DMRS_UplinkConfig_t *get_DMRS_UplinkConfig(const NR_PUSCH_Config_t *pusch_Config, const NR_tda_info_t *tda_info);

NR_pusch_dmrs_t get_ul_dmrs_params(const NR_ServingCellConfigCommon_t *scc,
                                   const NR_UE_UL_BWP_t *ul_bwp,
                                   const NR_tda_info_t *tda_info,
                                   const int Layers);

uint8_t nr_get_tpc(int target, uint8_t cqi, int incr);

int get_spf(nfapi_nr_config_request_scf_t *cfg);

int to_absslot(nfapi_nr_config_request_scf_t *cfg,int frame,int slot);

int get_nrofHARQ_ProcessesForPDSCH(e_NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH n);

void nr_get_tbs_dl(nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
		   int x_overhead,
                   uint8_t numdmrscdmgroupnodata,
                   uint8_t tb_scaling);

int NRRIV2BW(int locationAndBandwidth,int N_RB);

int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB);

/* Functions to manage an NR_list_t */
void create_nr_list(NR_list_t *listP, int len);
void resize_nr_list(NR_list_t *list, int new_len);
void destroy_nr_list(NR_list_t *list);
void add_nr_list(NR_list_t *listP, int id);
void remove_nr_list(NR_list_t *listP, int id);
void add_tail_nr_list(NR_list_t *listP, int id);
void add_front_nr_list(NR_list_t *listP, int id);
void remove_front_nr_list(NR_list_t *listP);

NR_UE_info_t * find_nr_UE(NR_UEs_t* UEs, rnti_t rntiP);

int find_nr_RA_id(module_id_t mod_idP, int CC_idP, rnti_t rntiP);

void configure_UE_BWP(gNB_MAC_INST *nr_mac,
                      NR_ServingCellConfigCommon_t *scc,
                      NR_UE_sched_ctrl_t *sched_ctrl,
                      NR_RA_t *ra,
                      NR_UE_info_t *UE,
                      int dl_bwp_switch,
                      int ul_bwp_switch);

NR_UE_info_t* add_new_nr_ue(gNB_MAC_INST *nr_mac, rnti_t rntiP, NR_CellGroupConfig_t *CellGroup);

void mac_remove_nr_ue(gNB_MAC_INST *nr_mac, rnti_t rnti);

int nr_get_default_pucch_res(int pucch_ResourceCommon);

int get_dlscs(nfapi_nr_config_request_t *cfg);

int get_ulscs(nfapi_nr_config_request_t *cfg);

int get_symbolsperslot(nfapi_nr_config_request_t *cfg);

int nr_write_ce_dlsch_pdu(module_id_t module_idP,
                          const NR_UE_sched_ctrl_t *ue_sched_ctl,
                          unsigned char *mac_pdu,
                          unsigned char drx_cmd,
                          unsigned char *ue_cont_res_id);

int binomial(int n, int k);

bool is_xlsch_in_slot(uint64_t bitmap, sub_frame_t slot);

/* \brief Function to indicate a received SDU on ULSCH.
@param Mod_id Instance ID of gNB
@param CC_id Component carrier index
@param rnti RNTI of UE transmitting the SDU
@param sdu Pointer to received SDU
@param sdu_len Length of SDU
@param timing_advance timing advance adjustment after this pdu
@param ul_cqi Uplink CQI estimate after this pdu (SNR quantized to 8 bits, -64 ... 63.5 dB in .5dB steps)
*/
void nr_rx_sdu(const module_id_t gnb_mod_idP,
               const int CC_idP,
               const frame_t frameP,
               const sub_frame_t subframeP,
               const rnti_t rntiP,
               uint8_t * sduP,
               const uint16_t sdu_lenP,
               const uint16_t timing_advance,
               const uint8_t ul_cqi,
               const uint16_t rssi);

void create_dl_harq_list(NR_UE_sched_ctrl_t *sched_ctrl, const NR_UE_ServingCell_Info_t *sc_info);

void reset_dl_harq_list(NR_UE_sched_ctrl_t *sched_ctrl);

void reset_ul_harq_list(NR_UE_sched_ctrl_t *sched_ctrl);

void handle_nr_ul_harq(const int CC_idP,
                       module_id_t mod_id,
                       frame_t frame,
                       sub_frame_t slot,
                       const nfapi_nr_crc_t *crc_pdu);

void handle_nr_srs_measurements(const module_id_t module_id,
                                const frame_t frame,
                                const sub_frame_t slot,
                                nfapi_nr_srs_indication_pdu_t *srs_ind);

void find_SSB_and_RO_available(gNB_MAC_INST *nrmac);

NR_pdsch_dmrs_t get_dl_dmrs_params(const NR_ServingCellConfigCommon_t *scc,
                                   const NR_UE_DL_BWP_t *BWP,
                                   const NR_tda_info_t *tda_info,
                                   const int Layers);

uint16_t get_pm_index(const gNB_MAC_INST *nrmac,
                      const NR_UE_info_t *UE,
                      nr_dci_format_t dci_format,
                      int layers,
                      int xp_pdsch_antenna_ports);

uint8_t get_mcs_from_cqi(int mcs_table, int cqi_table, int cqi_idx);

uint8_t get_dl_nrOfLayers(const NR_UE_sched_ctrl_t *sched_ctrl, const nr_dci_format_t dci_format);

void set_sched_pucch_list(NR_UE_sched_ctrl_t *sched_ctrl,
                          const NR_UE_UL_BWP_t *ul_bwp,
                          const NR_ServingCellConfigCommon_t *scc);
void free_sched_pucch_list(NR_UE_sched_ctrl_t *sched_ctrl);

int get_dl_tda(const gNB_MAC_INST *nrmac, const NR_ServingCellConfigCommon_t *scc, int slot);
int get_ul_tda(gNB_MAC_INST *nrmac, const NR_ServingCellConfigCommon_t *scc, int frame, int slot);

int get_cce_index(const gNB_MAC_INST *nrmac,
                  const int CC_id,
                  const int slot,
                  const rnti_t rnti,
                  uint8_t *aggregation_level,
                  const NR_SearchSpace_t *ss,
                  const NR_ControlResourceSet_t *coreset,
                  NR_sched_pdcch_t *sched_pdcch,
                  bool is_common);

bool nr_find_nb_rb(uint16_t Qm,
                   uint16_t R,
                   long transform_precoding,
                   uint8_t nrOfLayers,
                   uint16_t nb_symb_sch,
                   uint16_t nb_dmrs_prb,
                   uint32_t bytes,
                   uint16_t nb_rb_min,
                   uint16_t nb_rb_max,
                   uint32_t *tbs,
                   uint16_t *nb_rb);

int get_mcs_from_bler(const NR_bler_options_t *bler_options,
                      const NR_mac_dir_stats_t *stats,
                      NR_bler_stats_t *bler_stats,
                      int max_mcs,
                      frame_t frame);

int ul_buffer_index(int frame, int slot, int scs, int size);

void UL_tti_req_ahead_initialization(gNB_MAC_INST * gNB, NR_ServingCellConfigCommon_t *scc, int n, int CCid, frame_t frameP, int slotP, int scs);

void nr_sr_reporting(gNB_MAC_INST *nrmac, frame_t frameP, sub_frame_t slotP);

size_t dump_mac_stats(gNB_MAC_INST *gNB, char *output, size_t strlen, bool reset_rsrp);

void process_CellGroup(NR_CellGroupConfig_t *CellGroup, NR_UE_info_t *UE);

void prepare_initial_ul_rrc_message(gNB_MAC_INST *mac, NR_UE_info_t *UE);
void send_initial_ul_rrc_message(int rnti, const uint8_t *sdu, sdu_size_t sdu_len, void *data);

void abort_nr_dl_harq(NR_UE_info_t* UE, int8_t harq_pid);

void nr_mac_trigger_release_timer(NR_UE_sched_ctrl_t *sched_ctrl, NR_SubcarrierSpacing_t subcarrier_spacing);
bool nr_mac_check_release(NR_UE_sched_ctrl_t *sched_ctrl, int rnti);
void nr_mac_release_ue(gNB_MAC_INST *mac, int rnti);

void nr_mac_trigger_ul_failure(NR_UE_sched_ctrl_t *sched_ctrl, NR_SubcarrierSpacing_t subcarrier_spacing);
void nr_mac_reset_ul_failure(NR_UE_sched_ctrl_t *sched_ctrl);
void nr_mac_check_ul_failure(const gNB_MAC_INST *nrmac, int rnti, NR_UE_sched_ctrl_t *sched_ctrl);

void nr_mac_trigger_reconfiguration(const gNB_MAC_INST *nrmac, const NR_UE_info_t *UE);

#endif /*__LAYER2_NR_MAC_PROTO_H__*/
