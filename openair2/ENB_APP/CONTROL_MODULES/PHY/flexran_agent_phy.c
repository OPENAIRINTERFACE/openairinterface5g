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

/*! \file flexran_agent_phy.c
 * \brief FlexRAN agent Control Module PHY
 * \author Robert Schmidt
 * \date Oct 2018
 */

#include "flexran_agent_phy.h"
#include "flexran_agent_ran_api.h"

/* Array containing the Agent-PHY interfaces */
AGENT_PHY_xface *agent_phy_xface[NUM_MAX_ENB];

void flexran_agent_fill_phy_cell_config(mid_t mod_id, uint8_t cc_id,
                                        Protocol__FlexCellConfig *conf) {
  conf->phy_cell_id = flexran_get_cell_id(mod_id, cc_id);
  conf->has_phy_cell_id = 1;

  conf->pusch_hopping_offset = flexran_get_hopping_offset(mod_id, cc_id);
  conf->has_pusch_hopping_offset = 1;

  conf->hopping_mode = flexran_get_hopping_mode(mod_id, cc_id);
  conf->has_hopping_mode = 1;

  conf->n_sb = flexran_get_n_SB(mod_id, cc_id);
  conf->has_n_sb = 1;

  conf->phich_resource = flexran_get_phich_resource(mod_id, cc_id);
  conf->has_phich_resource = 1;

  conf->phich_duration = flexran_get_phich_duration(mod_id, cc_id);
  conf->has_phich_duration = 1;

  conf->init_nr_pdcch_ofdm_sym = flexran_get_num_pdcch_symb(mod_id, cc_id);
  conf->has_init_nr_pdcch_ofdm_sym = 1;

  conf->dl_bandwidth = flexran_get_N_RB_DL(mod_id, cc_id);
  conf->has_dl_bandwidth = 1;

  conf->ul_bandwidth = flexran_get_N_RB_UL(mod_id, cc_id);
  conf->has_ul_bandwidth = 1;

  conf->ul_cyclic_prefix_length = flexran_get_ul_cyclic_prefix_length(mod_id,  cc_id);
  conf->has_ul_cyclic_prefix_length = 1;

  conf->dl_cyclic_prefix_length = flexran_get_dl_cyclic_prefix_length(mod_id, cc_id);
  conf->has_dl_cyclic_prefix_length = 1;

  conf->antenna_ports_count = flexran_get_antenna_ports(mod_id,  cc_id);
  conf->has_antenna_ports_count = 1;

  conf->duplex_mode = flexran_get_duplex_mode(mod_id, cc_id);
  conf->has_duplex_mode = 1;

  conf->subframe_assignment = flexran_get_subframe_assignment(mod_id,  cc_id);
  conf->has_subframe_assignment = 1;

  conf->special_subframe_patterns = flexran_get_special_subframe_assignment(mod_id, cc_id);
  conf->has_special_subframe_patterns = 1;

  //TODO: Fill in with actual value, The MBSFN radio frame period
  conf->n_mbsfn_subframe_config_rfperiod = 0;
  uint32_t *elem_rfperiod = malloc(sizeof(uint32_t) * conf->n_mbsfn_subframe_config_rfperiod);
  if (elem_rfperiod)
    for(int j = 0; j < conf->n_mbsfn_subframe_config_rfperiod; j++)
      elem_rfperiod[j] = 1;
  conf->mbsfn_subframe_config_rfperiod = elem_rfperiod;

  //TODO: Fill in with actual value, The MBSFN radio frame offset
  conf->n_mbsfn_subframe_config_rfoffset = 0;
  uint32_t *elem_rfoffset = malloc(sizeof(uint32_t) * conf->n_mbsfn_subframe_config_rfoffset);
  if (elem_rfoffset)
    for(int j = 0; j < conf->n_mbsfn_subframe_config_rfoffset; j++)
      elem_rfoffset[j] = 1;
  conf->mbsfn_subframe_config_rfoffset = elem_rfoffset;

  //TODO: Fill in with actual value, Bitmap indicating the MBSFN subframes
  conf->n_mbsfn_subframe_config_sfalloc = 0;
  uint32_t *elem_sfalloc = malloc(sizeof(uint32_t) * conf->n_mbsfn_subframe_config_sfalloc);
  if (elem_sfalloc)
    for(int j = 0; j < conf->n_mbsfn_subframe_config_sfalloc; j++)
      elem_sfalloc[j] = 1;
  conf->mbsfn_subframe_config_sfalloc = elem_sfalloc;

  conf->prach_config_index = flexran_get_prach_ConfigIndex(mod_id, cc_id);
  conf->has_prach_config_index = 1;

  conf->prach_freq_offset = flexran_get_prach_FreqOffset(mod_id, cc_id);
  conf->has_prach_freq_offset = 1;

  conf->max_harq_msg3tx = flexran_get_maxHARQ_Msg3Tx(mod_id, cc_id);
  conf->has_max_harq_msg3tx = 1;

  conf->n1pucch_an = flexran_get_n1pucch_an(mod_id, cc_id);
  conf->has_n1pucch_an = 1;

  conf->deltapucch_shift = flexran_get_deltaPUCCH_Shift(mod_id, cc_id);
  conf->has_deltapucch_shift = 1;

  conf->nrb_cqi = flexran_get_nRB_CQI(mod_id, cc_id);
  conf->has_nrb_cqi = 1;

  conf->srs_subframe_config = flexran_get_srs_SubframeConfig(mod_id, cc_id);
  conf->has_srs_subframe_config = 1;

  conf->srs_bw_config = flexran_get_srs_BandwidthConfig(mod_id, cc_id);
  conf->has_srs_bw_config = 1;

  conf->srs_mac_up_pts = flexran_get_srs_MaxUpPts(mod_id, cc_id);
  conf->has_srs_mac_up_pts = 1;

  conf->dl_freq = flexran_agent_get_operating_dl_freq (mod_id, cc_id);
  conf->has_dl_freq = 1;

  conf->ul_freq = flexran_agent_get_operating_ul_freq (mod_id,  cc_id);
  conf->has_ul_freq = 1;

  conf->eutra_band = flexran_agent_get_operating_eutra_band (mod_id, cc_id);
  conf->has_eutra_band = 1;

  conf->dl_pdsch_power = flexran_agent_get_operating_pdsch_refpower(mod_id,  cc_id);
  conf->has_dl_pdsch_power = 1;

  conf->enable_64qam = flexran_get_enable64QAM(mod_id, cc_id);
  conf->has_enable_64qam = 1;
}

int flexran_agent_register_phy_xface(mid_t mod_id) {
  if (agent_phy_xface[mod_id]) {
    LOG_E(PHY, "PHY agent for eNB %d is already registered\n", mod_id);
    return -1;
  }

  AGENT_PHY_xface *xface = malloc(sizeof(AGENT_PHY_xface));

  if (!xface) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for PHY agent xface %d\n", mod_id);
    return -1;
  }

  agent_phy_xface[mod_id] = xface;
  return 0;
}

int flexran_agent_unregister_phy_xface(mid_t mod_id)
{
  if (!agent_phy_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "PHY agent for eNB %d is not registered\n", mod_id);
    return -1;
  }
  free(agent_phy_xface[mod_id]);
  agent_phy_xface[mod_id] = NULL;
  return 0;
}

AGENT_PHY_xface *flexran_agent_get_phy_xface(mid_t mod_id)
{
  return agent_phy_xface[mod_id];
}
