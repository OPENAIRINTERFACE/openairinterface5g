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

void set_cset_offset(uint16_t);

void mac_top_init_gNB(void);

void config_common(int Mod_idP,
		   NR_ServingCellConfigCommon_t *scc
		   );
int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
			   int ssb_SubcarrierOffset,
                           NR_ServingCellConfigCommon_t *scc,
			   int nsa_flag,
			   uint32_t rnti,
			   NR_CellGroupConfig_t *secondaryCellGroup
                           );
int  is_nr_UL_slot(NR_COMMON_channels_t * ccP, int slotP);

void clear_nr_nfapi_information(gNB_MAC_INST * gNB, 
                                int CC_idP,
                                frame_t frameP, 
                                sub_frame_t subframeP);

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);

void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   subframeP);

int configure_fapi_dl_Tx(int Mod_id,
			 nfapi_nr_dl_config_request_body_t *dl_req,
			 nfapi_tx_request_pdu_t *TX_req,
			 nfapi_nr_config_request_t *cfg,
			 nfapi_nr_coreset_t* coreset,
			 nfapi_nr_search_space_t* search_space,
			 int16_t pdu_index);


void nr_schedule_uss_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP,
                                   nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config);

void nr_configure_css_dci_initial(nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params,
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

int nr_is_dci_opportunity(nfapi_nr_search_space_t search_space,
                          nfapi_nr_coreset_t coreset,
                          uint16_t frame,
                          uint16_t slot,
                          nfapi_nr_config_request_t cfg);

void nr_configure_dci_from_pdcch_config(nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params,
                                        nfapi_nr_coreset_t* coreset,
                                        nfapi_nr_search_space_t* search_space,
					NR_BWP_Downlink_t *bwp);

int get_dlscs(nfapi_nr_config_request_t *cfg);

int get_ulscs(nfapi_nr_config_request_t *cfg);

int get_spf(nfapi_nr_config_request_t *cfg);

int to_absslot(nfapi_nr_config_request_t *cfg,int frame,int slot);

int get_symbolsperslot(nfapi_nr_config_request_t *cfg);

void get_band(uint32_t downlink_frequency, uint16_t *current_band, int32_t *current_offset, lte_frame_type_t *current_type);

uint64_t from_nrarfcn(int nr_bandP, uint32_t dl_nrarfcn);

uint32_t to_nrarfcn(int nr_bandP, uint64_t dl_CarrierFreq, uint32_t bw);


void nr_get_tbs_dl(nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu,
                   nfapi_nr_dl_config_dci_dl_pdu dci_pdu,
		   int x_overhead);
/** \brief Computes Q based on I_MCS PDSCH and table_idx for downlink. Implements MCS Tables from 38.214. */
uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx);
uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx);

/** \brief Computes Q based on I_MCS PDSCH and table_idx for uplink. Implements MCS Tables from 38.214. */
uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx);
uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx);

int NRRIV2BW(int locationAndBandwidth,int N_RB);

int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB);

void
dump_nr_ue_list(NR_UE_list_t *listP,
		int ul_flag);

int
find_nr_UE_id(module_id_t mod_idP,
	      rnti_t rntiP);

int add_new_nr_ue(module_id_t mod_idP,
		  rnti_t rntiP);

int get_num_dmrs(uint16_t dmrs_mask );

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols);

#endif /*__LAYER2_NR_MAC_PROTO_H__*/
