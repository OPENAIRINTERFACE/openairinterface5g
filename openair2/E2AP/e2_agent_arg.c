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

#include "e2_agent_arg.h"
#include "e2_agent_paramdef.h"
#include "common/config/config_userapi.h"

e2_agent_args_t RCconfig_NR_E2agent(void)
{
  paramdef_t e2agent_params[] = E2AGENT_PARAMS_DESC;
  int ret = config_get(config_get_if(), e2agent_params, sizeofArray(e2agent_params), CONFIG_STRING_E2AGENT);
  if (ret < 0) {
    printf("configuration file does not contain a \"%s\" section, applying default parameters from FlexRIC\n", CONFIG_STRING_E2AGENT);
    return (e2_agent_args_t) { 0 };
  }

  bool enabled = config_isparamset(e2agent_params, E2AGENT_CONFIG_SMDIR_IDX)
              && config_isparamset(e2agent_params, E2AGENT_CONFIG_IP_IDX);
  e2_agent_args_t dst = {.enabled = enabled};
  if (!enabled) {
    printf("E2 agent is DISABLED (for activation, define .%s.{%s,%s} parameters)\n", CONFIG_STRING_E2AGENT, E2AGENT_CONFIG_IP, E2AGENT_CONFIG_SMDIR);
    return dst;
  }

  if (e2agent_params[E2AGENT_CONFIG_SMDIR_IDX].strptr != NULL)
    dst.sm_dir = *e2agent_params[E2AGENT_CONFIG_SMDIR_IDX].strptr;

  if (e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr != NULL)
    dst.ip = *e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr;

  return dst;
}
