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

/*! \file lte-softmodem-common.c
 * \brief Top-level threads for eNodeB
 * \author Nokia BellLabs France, francois Taburet
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#include "lte-softmodem.h"
#include "UTIL/OPT/opt.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
static softmodem_params_t softmodem_params;
char *parallel_config=NULL;
char *worker_config=NULL;

uint64_t get_softmodem_optmask(void) {
  return softmodem_params.optmask;
}

uint64_t set_softmodem_optmask(uint64_t bitmask) {
  softmodem_params.optmask = softmodem_params.optmask | bitmask;
  return softmodem_params.optmask;
}

softmodem_params_t *get_softmodem_params(void) {
  return &softmodem_params;
}

void get_common_options(void) {
  uint32_t online_log_messages;
  uint32_t glog_level ;
  uint32_t start_telnetsrv;
  uint32_t noS1;
  uint32_t nokrnmod;
  uint32_t nonbiot;
  uint32_t rfsim;
  uint32_t basicsim;
  paramdef_t cmdline_params[] =CMDLINE_PARAMS_DESC ;
  paramdef_t cmdline_logparams[] =CMDLINE_LOGPARAMS_DESC ;
  checkedparam_t cmdline_log_CheckParams[] = CMDLINE_LOGPARAMS_CHECK_DESC;
  config_get( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL);
  config_set_checkfunctions(cmdline_logparams, cmdline_log_CheckParams,
                            sizeof(cmdline_logparams)/sizeof(paramdef_t));
  config_get( cmdline_logparams,sizeof(cmdline_logparams)/sizeof(paramdef_t),NULL);

  if(config_isparamset(cmdline_logparams,CMDLINE_ONLINELOG_IDX)) {
    set_glog_onlinelog(online_log_messages);
  }

  if(config_isparamset(cmdline_logparams,CMDLINE_GLOGLEVEL_IDX)) {
    set_glog(glog_level);
  }

  if (start_telnetsrv) {
    load_module_shlib("telnetsrv",NULL,0,NULL);
  }

  if (noS1) {
    set_softmodem_optmask(SOFTMODEM_NOS1_BIT);
  }

  if (nokrnmod) {
    set_softmodem_optmask(SOFTMODEM_NOKRNMOD_BIT);
  }

  if (nonbiot) {
    set_softmodem_optmask(SOFTMODEM_NONBIOT_BIT);
  }

  if (rfsim) {
    set_softmodem_optmask(SOFTMODEM_RFSIM_BIT);
  }

  if (basicsim) {
    set_softmodem_optmask(SOFTMODEM_BASICSIM_BIT);
  }

#if BASIC_SIMULATOR
  set_softmodem_optmask(SOFTMODEM_BASICSIM_BIT);
#endif

  if(parallel_config != NULL) set_parallel_conf(parallel_config);

  if(worker_config != NULL)   set_worker_conf(worker_config);
}

unsigned int is_nos1exec(char *exepath) {
  if ( strcmp( basename(exepath), "lte-softmodem-nos1") == 0)
    return 1;

  if ( strcmp( basename(exepath), "lte-uesoftmodem-nos1") == 0)
    return 1;

  return 0;
}
