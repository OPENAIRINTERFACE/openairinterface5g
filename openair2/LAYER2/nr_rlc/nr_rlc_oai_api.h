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

/*! \file       nr_rlc_oai_api.h
 * \brief       Header file for nr_rlc_oai_api
 * \author      Guido Casati
 * \date        2020
 * \email:      guido.casati@iis.fraunhofe.de
 * \version     1.0
 * @ingroup     _rlc

 */

#include "NR_RLC-BearerConfig.h"
#include "NR_RLC-Config.h"
#include "NR_LogicalChannelIdentity.h"
#include "NR_RadioBearerConfig.h"
#include "NR_CellGroupConfig.h"
#include "openair2/RRC/NR/nr_rrc_proto.h"
#include "nr_rlc_ue_manager.h"


struct NR_RLC_Config;
struct NR_LogicalChannelConfig;

void nr_rlc_add_srb(int rnti, int srb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig);
void nr_rlc_add_drb(int rnti, int drb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig);

void nr_rlc_reestablish_entity(int rnti, int lc_id);
void nr_rlc_remove_ue(int rnti);
bool nr_rlc_update_rnti(int from_rnti, int to_rnti);

/* test function for CI to trigger reestablishments */
void nr_rlc_test_trigger_reestablishment(int rnti);

void nr_rlc_release_entity(int rnti, logical_chan_id_t channel_id);

void nr_rlc_reconfigure_entity(int rnti, int lc_id, NR_RLC_Config_t *rlc_Config);

int nr_rlc_get_available_tx_space(
  const rnti_t            rntiP,
  const logical_chan_id_t channel_idP);

void nr_rlc_activate_avg_time_to_tx(
  const rnti_t            rnti,
  const logical_chan_id_t channel_id,
  const bool              is_on);

void nr_rlc_srb_recv_sdu(const int rnti, const logical_chan_id_t channel_id, unsigned char *buf, int size);

void nr_rlc_activate_srb0(int rnti, void *data,
                          void (*send_initial_ul_rrc_message)(
                                     int                    rnti,
                                     const uint8_t         *sdu,
                                     sdu_size_t             sdu_len,
                                     void                  *data));

bool nr_rlc_get_statistics(int rnti, int srb_flag, int rb_id, nr_rlc_statistics_t *out);
