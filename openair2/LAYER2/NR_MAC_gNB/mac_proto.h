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

#include "nr_mac_gNB.h"
#include "PHY/defs_gNB.h"
#include "NR_TAG-Id.h"

#define MAX_ACK_BITS 2 //only format 0 is available for now

void set_cset_offset(uint16_t);

void mac_top_init_gNB(void);

void config_common(int Mod_idP,
                   int pdsch_AntennaPorts,
		   NR_ServingCellConfigCommon_t *scc
		   );

int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
			   int ssb_SubcarrierOffset,
                           int pdsch_AntennaPorts,
                           NR_ServingCellConfigCommon_t *scc,
			   int nsa_flag,
			   uint32_t rnti,
			   NR_CellGroupConfig_t *secondaryCellGroup
                           );

void clear_nr_nfapi_information(gNB_MAC_INST * gNB, 
                                int CC_idP,
                                frame_t frameP, 
                                sub_frame_t subframeP);

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP,
			       frame_t frame_txP, sub_frame_t slot_txP,
			       frame_t frame_rxP, sub_frame_t slot_rxP);

int nr_generate_dlsch_pdu(module_id_t Mod_idP,
                          unsigned char *sdus_payload,
                          unsigned char *mac_pdu,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned char *ue_cont_res_id,
                          unsigned short post_padding);

void nr_schedule_ue_spec(module_id_t module_idP, frame_t frameP, sub_frame_t slotP);

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);

void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   subframeP);

int configure_fapi_dl_pdu(int Mod_id,
                         int *CCEIndeces,
                         nfapi_nr_dl_tti_request_body_t *dl_req,
                         NR_sched_pucch *pucch_sched,
                         uint8_t *mcsIndex,
                         uint16_t *rbSize,
                         uint16_t *rbStart);

void config_uldci(NR_BWP_Uplink_t *ubwp,nfapi_nr_pusch_pdu_t *pusch_pdu,nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15, dci_pdu_rel15_t *dci_pdu_rel15, int *dci_formats, int *rnti_types);

void configure_fapi_dl_Tx(module_id_t Mod_idP,
                          frame_t       frameP,
                          sub_frame_t   slotP,
                          nfapi_nr_dl_tti_request_body_t *dl_req,
                          nfapi_nr_pdu_t *tx_req,
                          int tbs_bytes,
                          int16_t pdu_index);

void nr_schedule_uss_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP,
                                   NR_sched_pucch *pucch_sched,
                                   nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_config);

void nr_schedule_uss_ulsch_phytest(int Mod_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP);

void nr_update_pucch_scheduling(int Mod_idP,
                                int UE_id,
                                frame_t frameP,
                                sub_frame_t slotP,
                                int slots_per_tdd,
                                NR_sched_pucch *sched_pucch);

void get_pdsch_to_harq_feedback(int Mod_idP,
                                int UE_id,
                                NR_SearchSpace__searchSpaceType_PR ss_type,
                                uint8_t *pdsch_to_harq_feedback);
  
void nr_configure_css_dci_initial(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
                                  nr_scs_e scs_common,
                                  nr_scs_e pdcch_scs,
                                  nr_frequency_range_e freq_range,
                                  uint8_t rmsi_pdcch_config,
                                  uint8_t ssb_idx,
                                  uint8_t k_ssb,
                                  uint16_t sfn_ssb,
                                  uint8_t n_ssb,
                                  uint16_t nb_slots_per_frame,
                                  uint16_t N_RB);
/*
int nr_is_dci_opportunity(nfapi_nr_search_space_t search_space,
                          nfapi_nr_coreset_t coreset,
                          uint16_t frame,
                          uint16_t slot,
                          nfapi_nr_config_request_scf_t cfg);
*/
void nr_configure_pucch(nfapi_nr_pucch_pdu_t* pucch_pdu,
			NR_ServingCellConfigCommon_t *scc,
			NR_BWP_Uplink_t *bwp,
                        uint8_t pucch_resource,
                        uint16_t O_uci,
                        uint16_t O_ack,
                        uint8_t SR_flag);
void nr_configure_pdcch(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
                        int ss_type,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_BWP_Downlink_t *bwp);

void fill_dci_pdu_rel15(nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15,
                        dci_pdu_rel15_t *dci_pdu_rel15,
                        int *dci_formats,
                        int *rnti_types);
int get_spf(nfapi_nr_config_request_scf_t *cfg);

int to_absslot(nfapi_nr_config_request_scf_t *cfg,int frame,int slot);

void get_band(uint64_t downlink_frequency, uint16_t *current_band, int32_t *current_offset, lte_frame_type_t *current_type);

void nr_get_tbs_dl(nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
		   int x_overhead);
/** \brief Computes Q based on I_MCS PDSCH and table_idx for downlink. Implements MCS Tables from 38.214. */
uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx);
uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx);

/** \brief Computes Q based on I_MCS PDSCH and table_idx for uplink. Implements MCS Tables from 38.214. */
uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx);
uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx);

int NRRIV2BW(int locationAndBandwidth,int N_RB);

int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB);

void dump_nr_ue_list(NR_UE_list_t *listP, int ul_flag);

int find_nr_UE_id(module_id_t mod_idP, rnti_t rntiP);

int add_new_nr_ue(module_id_t mod_idP, rnti_t rntiP);

int get_num_dmrs(uint16_t dmrs_mask );
uint8_t get_l0_ul(uint8_t mapping_type, uint8_t dmrs_typeA_position);
int32_t get_l_prime(uint8_t duration_in_symbols, uint8_t mapping_type, pusch_dmrs_AdditionalPosition_t additional_pos, pusch_maxLength_t pusch_maxLength);

uint8_t get_L_ptrs(uint8_t mcs1, uint8_t mcs2, uint8_t mcs3, uint8_t I_mcs, uint8_t mcs_table);
uint8_t get_K_ptrs(uint16_t nrb0, uint16_t nrb1, uint16_t N_RB);

uint16_t nr_dci_size(nr_dci_format_t format,
                         nr_rnti_type_t rnti_type,
                         uint16_t N_RB);

int allocate_nr_CCEs(gNB_MAC_INST *nr_mac,
                     int bwp_id,
                     int coreset_id,
                     int aggregation,
                     int search_space, // 0 common, 1 ue-specific
                     int UE_id,
                     int m
                     );

int is_nr_DL_slot(NR_ServingCellConfigCommon_t *scc,slot_t slotP);
int is_nr_UL_slot(NR_ServingCellConfigCommon_t *scc,slot_t slotP);

int get_dlscs(nfapi_nr_config_request_t *cfg);

int get_ulscs(nfapi_nr_config_request_t *cfg);

int get_symbolsperslot(nfapi_nr_config_request_t *cfg);

int32_t get_nr_uldl_offset(int nr_bandP);

void config_nr_mib(int Mod_idP, 
                   int CC_idP,
                   int p_gNBP,
                   int subCarrierSpacingCommon,
                   uint32_t ssb_SubcarrierOffset,
                   int dmrs_TypeA_Position,
                   uint32_t pdcch_ConfigSIB1,
                   int cellBarred,
                   int intraFreqReselection);


void nr_process_mac_pdu(
    module_id_t module_idP,
    uint8_t CC_id,
    frame_t frameP,
    uint8_t *pduP,
    uint16_t mac_pdu_len);

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
               const uint16_t timing_advance, const uint8_t ul_cqi);

#endif /*__LAYER2_NR_MAC_PROTO_H__*/
