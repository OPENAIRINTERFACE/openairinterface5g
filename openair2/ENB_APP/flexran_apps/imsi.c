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

/*! \file imsi.c
 * \brief flexran IMSI UE-slice association app
 * \author Robert Schmidt, Firas Abdeljelil
 * \date 2020
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include "flexran_agent_common.h"
#include "flexran_agent_async.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_timer.h"
#include "flexran_agent_defs.h"
#include "flexran_agent_net_comm.h"
#include "flexran_agent_ran_api.h"
#include "flexran_agent_phy.h"
#include "flexran_agent_mac.h"
#include "flexran_agent_rrc.h"
#include "flexran_agent_pdcp.h"
#include "flexran_agent_s1ap.h"
#include "flexran_agent_app.h"
#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "queue.h"
typedef struct Map_regex_slice slist_data_t;
struct Map_regex_slice {
  char regex_[30] ;
  int slice_dl;
  int slice_ul;
  regex_t regex;
  SLIST_ENTRY (Map_regex_slice) entries;
};
bool reconfig = false;
int ue[MAX_MOBILES_PER_ENB]={ [0 ... MAX_MOBILES_PER_ENB - 1] = -1 };

SLIST_HEAD(slisthead, Map_regex_slice) head;

int dump_list(void) {
  int len = 1;
  slist_data_t *datap = NULL;
  SLIST_FOREACH(datap, &head, entries) {
    LOG_W(FLEXRAN_AGENT, "%d: regex %s DL %d UL %d\n", len++, datap->regex_,
          datap->slice_dl, datap->slice_ul);
  }
  return len;
}

void add(char *regex, int dl, int ul) {
  slist_data_t *datap = malloc(sizeof(slist_data_t));
  if (!datap) {
    LOG_E(FLEXRAN_AGENT, "cannot allocate memory for slist_data_t\n");
    return;
  }
  strcpy(datap->regex_, regex);
  datap->slice_dl = dl;
  datap->slice_ul = ul;
  regcomp(&datap->regex, datap->regex_, 0);
  SLIST_INSERT_HEAD(&head, datap, entries);
    if (ul==-1)
      LOG_I(FLEXRAN_AGENT, "added new element to list: regex %s slice DL %d\n" ,regex, dl);
    else if (dl==-1)
      LOG_I(FLEXRAN_AGENT, "added new element to list: regex %s slice UL %d\n" ,regex, ul);
    else 
      LOG_I(FLEXRAN_AGENT, "added new element to list: regex %s slice DL %d UL %d\n",
            regex, dl, ul);
}
void remove_(char *regex) {
  slist_data_t *datap = NULL;
  SLIST_FOREACH(datap, &head, entries) {
    if (strcmp(datap->regex_, regex) == 0) {
      SLIST_REMOVE(&head, datap, Map_regex_slice, entries);
      if (datap->slice_dl==-1)
        LOG_I(FLEXRAN_AGENT, "removed element from list: regex %s slice UL %d\n",datap->regex_, datap->slice_ul);
      else if (datap->slice_ul==-1)
        LOG_I(FLEXRAN_AGENT, "removed element from list: regex %s slice DL %d\n",datap->regex_, datap->slice_dl);
      else
      LOG_I(FLEXRAN_AGENT, "removed element from list: regex %s slice DL %d UL %d\n",
            datap->regex_, datap->slice_dl, datap->slice_ul);
      regfree(&datap->regex);
      free(datap);
      return;
    }
  }
  LOG_E(FLEXRAN_AGENT, "no regex %s found in list\n", regex);
}

Protocol__FlexranMessage *matching_tick(
    mid_t mod_id,
    const Protocol__FlexranMessage *msg) {
  const int num = flexran_get_mac_num_ues(mod_id);
  for (int i = 0; i < num; i++) {
    int ue_id = flexran_get_mac_ue_id(mod_id, i);
    if (ue[ue_id] >= 0 && !reconfig)
      continue;
    rnti_t rnti = flexran_get_mac_ue_crnti(mod_id, ue_id);
    uint64_t imsi = flexran_get_ue_imsi(mod_id, rnti);
    char snum[20];
    sprintf(snum, "%" PRIu64, imsi);
    slist_data_t *datap = NULL;
    SLIST_FOREACH(datap, &head, entries) {
      if (regexec(&datap->regex, snum, 0, NULL, 0) == 0) {

        int dl = datap->slice_dl;
        if (dl >= 0 && dl != flexran_get_ue_dl_slice_id(mod_id, ue_id)) {
          int ra = flexran_set_ue_dl_slice_id(mod_id, ue_id, dl);
          if (ra)
            LOG_I(FLEXRAN_AGENT,
                  "RNTI %04x/IMSI %s in the slice_dl %d\n",
                  rnti,
                  snum,
                  dl);
          else
            LOG_W(FLEXRAN_AGENT, "No such DL slice %d\n", dl);
        }

        int ul = datap->slice_ul;
        if (ul >= 0 && ul != flexran_get_ue_ul_slice_id(mod_id, ue_id)) {
          int rb = flexran_set_ue_ul_slice_id(mod_id, ue_id, ul);
          if (rb)
            LOG_I(FLEXRAN_AGENT,
                  "RNTI %04x/IMSI %s in the slice_ul %d\n",
                  rnti,
                  snum,
                  ul);
          else
            LOG_W(FLEXRAN_AGENT, "No such DL slice %d\n", dl);
        }
      }
    }
    ue[ue_id] = ue_id;
  }
  reconfig = false;
  return NULL;
}
void add_param(Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p, int n_p) {
  char s[30];
  reconfig = true;
  bool supp=false;
  bool sl_E=false; 
  int dl=-1;
  int ul=-1;
  slist_data_t *datap = NULL;
  for (int i = 0; i < n_p; ++i) {
    switch (p[i]->value->param_case) {
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM__NOT_SET:
        LOG_I(FLEXRAN_AGENT, "    param %s is not set\n", p[i]->key);
        break;
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM_INTEGER: 
        if( strcmp(p[i]->key, "slice_dl")==0)
           dl= p[i]->value->integer;    
        else
          ul= p[i]->value->integer;    
        sl_E=true; 
        break;
      case PROTOCOL__FLEX_AGENT_RECONFIGURATION_PARAM__PARAM_STR:
        strcpy(s,p[i]->value->str);  
        break;
      default:
        LOG_W(FLEXRAN_AGENT, "    type of param %s is unknown\n", p[i]->key);
        break;
    };
  }
  if (!SLIST_EMPTY(&head)){
    SLIST_FOREACH(datap, &head, entries) {
      if (strcmp(datap->regex_, s) == 0) {
        if (datap->slice_dl!=dl || datap->slice_ul!=ul)
          supp=true;
      }
    }
  }
  if (sl_E==false)
    remove_(s);
  else {
    if (supp==true)
      remove_(s);
    add(s,dl,ul);
  }
  /* re-verify all UEs again */
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    ue[i] = -1;
}
/* imsi_start: entry point for imsi app. Installs a callback using the timer
 * API to have function imsi_tick() called every 5s */
#define TIMER 32
int imsi_start(mid_t mod_id,
               Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p,
               int n_p)
{
  LOG_W(FLEXRAN_AGENT, "%s(): enable IMSI app timer\n", __func__);
  flexran_agent_create_timer(mod_id,
                             500,
                             FLEXRAN_AGENT_TIMER_TYPE_PERIODIC,
                             TIMER,
                             matching_tick,
                             NULL);
  SLIST_INIT(&head);
  return 0;
}

int imsi_reconfig(mid_t mod_id,
                  Protocol__FlexAgentReconfigurationSubsystem__ParamsEntry **p,
                  int n_p)
{ 
  add_param(p, n_p);
  return 0;
}

/* stop function: cleans up before app can be unloaded. Here: stop the timer */
int imsi_stop(mid_t mod_id) {
  LOG_W(FLEXRAN_AGENT, "%s(): disable IMSI app timer\n", __func__);
  flexran_agent_destroy_timer(mod_id, TIMER);
  
  return 0;
}

/* app definition. visibility should be set to default so this symbol is
 * reachable from outside */
flexran_agent_app_t __attribute__ ((visibility ("default"))) app = {
  .start = imsi_start,
  .reconfig = imsi_reconfig,
  .stop =  imsi_stop
};
