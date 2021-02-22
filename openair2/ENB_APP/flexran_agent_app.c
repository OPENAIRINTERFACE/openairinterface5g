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

/*! \file flexran_agent_app.c
 * \brief CM for handling of applications
 * \author Robert Schmidt
 * \date 2020
 * \version 0.1
 */

#include "flexran_agent_app.h"
#include "queue.h"
#include <dlfcn.h>

/* app definition */
typedef struct flexran_agent_app_internal_s {
  char *name;
  void *dl_handle;
  flexran_agent_app_t *app;

  SLIST_ENTRY (flexran_agent_app_internal_s) entries;
} flexran_agent_app_internal_t;

/* a list of all apps */
SLIST_HEAD(flexran_apps_s, flexran_agent_app_internal_s) flexran_apps[NUM_MAX_ENB];

void flexran_agent_app_init(mid_t mod_id) {
  //flexran_apps[mod_id] = SLIST_HEAD_INITIALIZER(flexran_apps[mod_id]);
  SLIST_INIT(&flexran_apps[mod_id]);
}

int flexran_agent_start_app_direct(mid_t mod_id, flexran_agent_app_t *app, char *name) {
  app->start(mod_id, NULL, 0);

  flexran_agent_app_internal_t *so = malloc(sizeof(flexran_agent_app_internal_t));
  DevAssert(so);
  so->name = strdup(name);
  DevAssert(so->name);
  so->dl_handle = NULL;
  so->app = app;
  SLIST_INSERT_HEAD(&flexran_apps[mod_id], so, entries);
  return 0;
}

int flexran_agent_start_app(mid_t mod_id, Protocol__FlexAgentReconfigurationSubsystem *sub) {
  char s[512];
  int rc = flexran_agent_map_name_to_delegated_object(mod_id, sub->name, s, 512);
  if (rc < 0) {
    LOG_E(FLEXRAN_AGENT, "cannot map name %s\n", sub->name);
    return -1;
  }
  void *h = dlopen(s, RTLD_NOW);
  if (!h) {
    LOG_E(FLEXRAN_AGENT, "cannot open shared object %s\n", s);
    return -1;
  }
  flexran_agent_app_t *app = dlsym(h, "app");
  if (!app) {
    LOG_E(FLEXRAN_AGENT, "cannot locate app inside of shared object %s\n", s);
    return -1;
  }
  app->start(mod_id, sub->params, sub->n_params);

  flexran_agent_app_internal_t *so = malloc(sizeof(flexran_agent_app_internal_t));
  DevAssert(so);
  so->name = strdup(sub->name);
  DevAssert(so->name);
  so->dl_handle = h;
  so->app = app;
  SLIST_INSERT_HEAD(&flexran_apps[mod_id], so, entries);

  return 0;
}

int flexran_agent_stop_app(mid_t mod_id, char *name) {
  flexran_agent_app_internal_t *so = NULL;
  SLIST_FOREACH(so, &flexran_apps[mod_id], entries) {
    if (strcmp(so->name, name) == 0) {
      LOG_I(FLEXRAN_AGENT, "application %s seems to be running\n", name);
      break;
    }
  }
  if (!so) {
    LOG_E(FLEXRAN_AGENT, "no such application %s found\n", name);
    return -1;
  }
  flexran_agent_app_t *app = so->app;
  app->stop(mod_id);

  SLIST_REMOVE(&flexran_apps[mod_id], so, flexran_agent_app_internal_s, entries);
  free(so->name);
  if (so->dl_handle)
    dlclose(so->dl_handle);
  free(so);
  so = NULL;
  return 0;
}

int flexran_agent_reconfig_app(
    mid_t mod_id,
    Protocol__FlexAgentReconfigurationSubsystem *sub)
{
  flexran_agent_app_internal_t *so = NULL;
  SLIST_FOREACH(so, &flexran_apps[mod_id], entries) {
    if (strcmp(so->name, sub->name) == 0) {
      LOG_I(FLEXRAN_AGENT, "application %s is running\n", sub->name);
      break;
    }
  }
  if (!so) {
    LOG_E(FLEXRAN_AGENT, "no such application %s found\n", sub->name);
    return -1;
  }
  flexran_agent_app_t *app = so->app;
  LOG_I(FLEXRAN_AGENT, "reconfiguring app %s\n", so->name);
  app->reconfig(mod_id, sub->params, sub->n_params);
  return 0;
}

int flexran_agent_custom_command_app(
    mid_t mod_id,
    Protocol__FlexAgentReconfigurationSubsystem *sub)
{
  flexran_agent_app_internal_t *so = NULL;
  SLIST_FOREACH(so, &flexran_apps[mod_id], entries) {
    if (strcmp(so->name, sub->name) == 0) {
      LOG_I(FLEXRAN_AGENT, "application %s is running\n", sub->name);
      break;
    }
  }
  if (!so) {
    LOG_E(FLEXRAN_AGENT, "no such application %s found\n", sub->name);
    return -1;
  }
  void (*cmd)(mid_t, Protocol__FlexAgentReconfigurationSubsystem *) = dlsym(so->dl_handle, sub->behavior);
  if (!cmd) {
    LOG_E(FLEXRAN_AGENT,
          "cannot locate command %s inside of app %s\n",
          sub->behavior,
          so->name);
    return -1;
  }

  LOG_I(FLEXRAN_AGENT,
        "calling command %s inside of app %s\n",
        sub->behavior,
        so->name);
  (*cmd)(mod_id, sub);
  return 0;
}

void flexran_agent_handle_apps(
    mid_t mod_id,
    Protocol__FlexAgentReconfigurationSubsystem **subs,
    int n_subs) {
  for (int i = 0; i < n_subs; ++i) {
    Protocol__FlexAgentReconfigurationSubsystem *s = subs[i];
    if (strcmp(s->behavior, "start") == 0) {
      int rc = flexran_agent_start_app(mod_id, s);
      if (rc == 0)
        LOG_I(FLEXRAN_AGENT, "application \"%s\" started\n", s->name);
    } else if (strcmp(s->behavior, "stop") == 0) {
      int rc = flexran_agent_stop_app(mod_id, s->name);
      if (rc == 0)
        LOG_I(FLEXRAN_AGENT, "application \"%s\" stopped\n", s->name);
    } else if (strcmp(s->behavior, "reconfig") == 0) {
      flexran_agent_reconfig_app(mod_id, s);
    } else {
      flexran_agent_custom_command_app(mod_id, s);
    }
  }
}

void flexran_agent_fill_loaded_apps(mid_t mod_id,
                                    Protocol__FlexEnbConfigReply *reply) {
  int n = 0;
  flexran_agent_app_internal_t *so = NULL;
  SLIST_FOREACH(so, &flexran_apps[mod_id], entries)
    ++n;
  reply->n_loadedapps = n;
  reply->loadedapps = calloc(n, sizeof(char *));
  if (!reply->loadedapps) {
    reply->n_loadedapps = 0;
    return;
  }
  n = 0;
  SLIST_FOREACH(so, &flexran_apps[mod_id], entries)
    reply->loadedapps[n++] = so->name;
}
