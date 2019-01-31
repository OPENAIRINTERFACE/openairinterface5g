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

/*! \file flexran_agent_mac_internal.h
 * \brief Implementation specific definitions for the FlexRAN MAC agent
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_MAC_INTERNAL_H_
#define FLEXRAN_AGENT_MAC_INTERNAL_H_

#include <pthread.h>

#include <yaml.h>

#include "flexran_agent_mac.h"
#include "flexran_agent_common.h"
#include "flexran_agent_defs.h"

/*This will be used for producing continuous status updates for the MAC
 *Needs to be thread-safe
 */
typedef struct {
  /*Flag showing if continuous mac stats update is enabled*/
  uint8_t is_initialized;
  volatile uint8_t cont_update;
  xid_t xid;
  Protocol__FlexranMessage *stats_req;
  Protocol__FlexranMessage *prev_stats_reply;

  pthread_mutex_t *mutex;
} mac_stats_updates_context_t;

/*Array holding the last stats reports for each eNB. Used for continuous reporting*/
mac_stats_updates_context_t mac_stats_context[NUM_MAX_ENB];

/*Functions to initialize and destroy the struct required for the
 *continuous stats update report*/
err_code_t flexran_agent_init_cont_mac_stats_update(mid_t mod_id);

err_code_t flexran_agent_destroy_cont_mac_stats_update(mid_t mod_id);


/*Enable/Disable the continuous stats update service for the MAC*/
err_code_t flexran_agent_enable_cont_mac_stats_update(mid_t mod_id, xid_t xid,
						  stats_request_config_t *stats_req);

err_code_t flexran_agent_disable_cont_mac_stats_update(mid_t mod_id);

Protocol__FlexranMessage * flexran_agent_generate_diff_mac_stats_report(Protocol__FlexranMessage *new_report,
								    Protocol__FlexranMessage *old_report);

Protocol__FlexUeStatsReport * copy_ue_stats_report(Protocol__FlexUeStatsReport * original);

Protocol__FlexCellStatsReport * copy_cell_stats_report(Protocol__FlexCellStatsReport *original);

Protocol__FlexRlcBsr * copy_rlc_report(Protocol__FlexRlcBsr * original);

Protocol__FlexUlCqiReport * copy_ul_cqi_report(Protocol__FlexUlCqiReport * original);

Protocol__FlexDlCqiReport * copy_dl_cqi_report(Protocol__FlexDlCqiReport * original);

Protocol__FlexPagingBufferReport * copy_paging_buffer_report(Protocol__FlexPagingBufferReport *original);

Protocol__FlexDlCsi * copy_csi_report(Protocol__FlexDlCsi * original);

Protocol__FlexNoiseInterferenceReport * copy_noise_inter_report(Protocol__FlexNoiseInterferenceReport *original);

int compare_ue_stats_reports(Protocol__FlexUeStatsReport *rep1,
			    Protocol__FlexUeStatsReport *rep2);

int compare_cell_stats_reports(Protocol__FlexCellStatsReport *rep1,
			    Protocol__FlexCellStatsReport *rep2);


/* Functions for parsing the MAC agent policy reconfiguration command */

int parse_mac_config(mid_t mod_id, yaml_parser_t *parser);

int parse_dl_scheduler_config(mid_t mod_id, yaml_parser_t *parser);

int parse_dl_scheduler_parameters(mid_t mod_id, yaml_parser_t *parser);

int parse_ul_scheduler_config(mid_t mod_id, yaml_parser_t *parser);

int parse_ul_scheduler_parameters(mid_t mod_id, yaml_parser_t *parser);

int load_dl_scheduler_function(mid_t mod_id, const char *function_name);

/*** Functions for handling a slice config ***/

/* allocate memory for a Protocol__FlexSliceConfig structure with n_dl DL slice
 * configs and m_ul UL slice configs */
Protocol__FlexSliceConfig *flexran_agent_create_slice_config(int n_dl, int m_ul);

/* read the general slice parameters via RAN into the given
 * Protocol__FlexSliceConfig struct */
void flexran_agent_read_slice_config(mid_t mod_id, Protocol__FlexSliceConfig *s);

/* read the DL slice config via the RAN into a given Protocol__FlexDlSlice
 * struct */
void flexran_agent_read_slice_dl_config(mid_t mod_id, int slice_idx, Protocol__FlexDlSlice *dl_slice);

/* read the UL slice config via the RAN into a given Protocol__FlexUlSlice
 * struct */
void flexran_agent_read_slice_ul_config(mid_t mod_id, int slice_idx, Protocol__FlexUlSlice *ul_slice);

/* reads content of slice over the sc_update structure, so that it can be
 * applied later by performing a diff between slice_config and sc_update */
void prepare_update_slice_config(mid_t mod_id, Protocol__FlexSliceConfig *slice);

/* apply generic slice parameters (e.g. intra-/interslice sharing activated or
 * not) if there are changes. Returns the number of changed parameters. */
int apply_new_slice_config(mid_t mod_id, Protocol__FlexSliceConfig *olds, Protocol__FlexSliceConfig *news);

/* apply new configuration of slice in DL if there are changes between the
 * parameters. Returns the number of changed parameters. */
int apply_new_slice_dl_config(mid_t mod_id, Protocol__FlexDlSlice *oldc, Protocol__FlexDlSlice *newc);

/* apply new configuration of slice in UL if there are changes between the
 * parameters. Returns the number of changed parameters. */
int apply_new_slice_ul_config(mid_t mod_id, Protocol__FlexUlSlice *oldc, Protocol__FlexUlSlice *newc);

/* inserts a new ue_config into the structure keeping ue to slice association
 * updates and marks so it can be applied */
void prepare_ue_slice_assoc_update(mid_t mod_id, Protocol__FlexUeConfig *ue_config);

/* apply a new association between a UE and a slice (both DL and UL) */
int apply_ue_slice_assoc_update(mid_t mod_id);

#endif /*FLEXRAN_AGENT_MAC_INTERNAL_H_*/
