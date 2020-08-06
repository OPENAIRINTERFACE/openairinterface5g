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

/*! \file flexran_agent_s1ap.c
 * \brief FlexRAN agent Control Module S1AP
 * \author Navid Nikaein
 * \date 2019
 * \version 0.1
 */

#include "flexran_agent_s1ap.h"


/*Array containing the Agent-S1AP interfaces*/
AGENT_S1AP_xface *agent_s1ap_xface[NUM_MAX_ENB];

void flexran_agent_fill_s1ap_cell_config(mid_t mod_id,
                                         Protocol__FlexS1apConfig **s1ap_config) {
  *s1ap_config = malloc(sizeof(Protocol__FlexS1apConfig));
  if (!*s1ap_config) return;
  protocol__flex_s1ap_config__init(*s1ap_config);
  LOG_D(FLEXRAN_AGENT, "flexran_agent_fill_s1ap_cell_config %d\n", mod_id);

  // S1AP status
  (*s1ap_config)->has_pending = 1;
  (*s1ap_config)->pending = flexran_get_s1ap_mme_pending(mod_id);

  (*s1ap_config)->has_connected = 1;
  (*s1ap_config)->connected = flexran_get_s1ap_mme_connected(mod_id);

  (*s1ap_config)->enb_s1_ip = flexran_get_s1ap_enb_s1_ip(mod_id);
  if (!(*s1ap_config)->enb_s1_ip)
    (*s1ap_config)->enb_s1_ip = "";

  (*s1ap_config)->enb_name = flexran_get_s1ap_enb_name(mod_id);

  (*s1ap_config)->mme = NULL;
  (*s1ap_config)->n_mme = flexran_get_s1ap_nb_mme(mod_id);
  if ((*s1ap_config)->n_mme > 0) {
    Protocol__FlexS1apMme **mme_conf = calloc((*s1ap_config)->n_mme,
                                              sizeof(Protocol__FlexS1apMme *));
    AssertFatal(mme_conf, "%s(): MME malloc failed\n", __func__);
    for(int i = 0; i < (*s1ap_config)->n_mme; i++){
      mme_conf[i] = malloc(sizeof(Protocol__FlexS1apMme));
      AssertFatal(mme_conf[i], "%s(): MME malloc failed\n", __func__);
      protocol__flex_s1ap_mme__init(mme_conf[i]);
      if (flexran_get_s1ap_mme_conf(mod_id, i, mme_conf[i]) < 0) {
        LOG_E(FLEXRAN_AGENT,
              "error in flexran_get_s1ap_mme_conf(): cannot retrieve MME state\n");
        (*s1ap_config)->n_mme = 0;
        break;
      }
    }
    (*s1ap_config)->mme = mme_conf;
  }
}

int flexran_agent_s1ap_stats_reply(mid_t mod_id,
                                   Protocol__FlexUeStatsReport **ue_report,
                                   int n_ue,
                                   uint32_t ue_flags) {
  if (n_ue <= 0)
    return 0;
  if (!(ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_S1AP_STATS))
    return 0;

  for (int i = 0; i < n_ue; i++) {
    const rnti_t rnti = ue_report[i]->rnti;
    Protocol__FlexS1apUe *ue = malloc(sizeof(Protocol__FlexS1apUe));
    AssertFatal(ue, "%s(): MME malloc failed\n", __func__);
    protocol__flex_s1ap_ue__init(ue);
    if (flexran_get_s1ap_ue(mod_id, rnti, ue) < 0) {
      LOG_E(FLEXRAN_AGENT, "error in %s(): cannot retrieve UE conf\n", __func__);
      break;
    }
    ue_report[i]->s1ap_stats = ue;
    ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_S1AP_STATS;
  }
  return 0;
}

void flexran_agent_free_s1ap_cell_config(Protocol__FlexS1apConfig **s1ap) {
  for (int i = 0; i < (*s1ap)->n_mme; i++) {
    /* following structures allocated in the RAN API */
    for(int j = 0; j < (*s1ap)->mme[i]->n_served_gummeis; j++) {
      free((*s1ap)->mme[i]->served_gummeis[j]->plmn);
      free((*s1ap)->mme[i]->served_gummeis[j]);
    }
    free((*s1ap)->mme[i]->served_gummeis);
    for (int j = 0; j < (*s1ap)->mme[i]->n_requested_plmns; j++)
      free((*s1ap)->mme[i]->requested_plmns[j]);
    free((*s1ap)->mme[i]->requested_plmns);
    free((*s1ap)->mme[i]);
  }
  free((*s1ap)->mme);
  free(*s1ap);
  *s1ap = NULL;
}

void flexran_agent_s1ap_destroy_stats_reply(Protocol__FlexStatsReply *reply) {
  for (int i = 0; i < reply->n_ue_report; ++i) {
    if (!reply->ue_report[i]->s1ap_stats)
      continue;
    free(reply->ue_report[i]->s1ap_stats->selected_plmn);
    free(reply->ue_report[i]->s1ap_stats);
    reply->ue_report[i]->s1ap_stats = NULL;
  }
}

void flexran_agent_handle_mme_update(mid_t mod_id,
                                     size_t n_mme,
                                     Protocol__FlexS1apMme **mme) {
  if (n_mme == 0 || n_mme > 1) {
    LOG_E(FLEXRAN_AGENT, "cannot handle %lu MMEs yet\n", n_mme);
    return;
  }

  if (!mme[0]->s1_ip) {
    LOG_E(FLEXRAN_AGENT, "no S1 IP present, cannot handle request\n");
    return;
  }

  if (mme[0]->has_state
      && mme[0]->state == PROTOCOL__FLEX_MME_STATE__FLMMES_DISCONNECTED) {
    int rc = flexran_remove_s1ap_mme(mod_id, 1, &mme[0]->s1_ip);
    if (rc == 0)
      LOG_I(FLEXRAN_AGENT, "remove MME at IP %s\n", mme[0]->s1_ip);
    else
      LOG_W(FLEXRAN_AGENT,
            "could not remove MME: flexran_remove_s1ap_mme() returned %d\n",
            rc);
  } else {
    int rc = flexran_add_s1ap_mme(mod_id, 1, &mme[0]->s1_ip);
    if (rc == 0)
      LOG_I(FLEXRAN_AGENT, "add MME at IP %s\n", mme[0]->s1_ip);
    else
      LOG_W(FLEXRAN_AGENT,
            "could not add MME: flexran_add_s1ap_mme() returned %d\n",
            rc);
  }
}

void flexran_agent_handle_plmn_update(mid_t mod_id,
                                      int CC_id,
                                      size_t n_plmn,
                                      Protocol__FlexPlmn **plmn_id) {
  if (n_plmn < 1 || n_plmn > 6) {
    LOG_E(FLEXRAN_AGENT, "cannot handle %lu PLMNs\n", n_plmn);
    return;
  }

  /* We assume the controller has checked all the parameters within each
   * plmn_id */
  int rc = flexran_set_new_plmn_id(mod_id, CC_id, n_plmn, plmn_id);
  if (rc == 0) {
    LOG_I(FLEXRAN_AGENT, "set %lu new PLMNs:\n", n_plmn);
    for (int i = 0; i < (int)n_plmn; ++i)
      LOG_I(FLEXRAN_AGENT, "    MCC %d MNC %d MNC length %d\n",
            plmn_id[i]->mcc, plmn_id[i]->mnc, plmn_id[0]->mnc_length);
  } else {
    LOG_W(FLEXRAN_AGENT,
          "could not set new PLMN configuration: flexran_set_new_plmn_id() returned %d\n",
          rc);
  }
}

int flexran_agent_register_s1ap_xface(mid_t mod_id) {
  if (agent_s1ap_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "S1AP agent CM for eNB %d is already registered\n", mod_id);
    return -1;
  }

  AGENT_S1AP_xface *xface = malloc(sizeof(AGENT_S1AP_xface));
  if (!xface) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for S1AP agent xface %d\n", mod_id);
    return -1;
  }

  // not implemented yet
  xface->flexran_s1ap_notify_release_request=NULL;

  agent_s1ap_xface[mod_id] = xface;

  return 0;
}

int flexran_agent_unregister_s1ap_xface(mid_t mod_id) {
  if (!agent_s1ap_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "S1AP agent for eNB %d is not registered\n", mod_id);
    return -1;
  }
  agent_s1ap_xface[mod_id]->flexran_s1ap_notify_release_request=NULL;
  free(agent_s1ap_xface[mod_id]);
  agent_s1ap_xface[mod_id] = NULL;
  return 0;
}

AGENT_S1AP_xface *flexran_agent_get_s1ap_xface(mid_t mod_id) {
  return agent_s1ap_xface[mod_id];
}
