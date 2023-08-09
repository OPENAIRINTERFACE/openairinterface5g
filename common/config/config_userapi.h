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

/*! \file common/config/config_userapi.h
 * \brief: configuration module, include file to be used by the source code calling the
 *  configuration module to access configuration parameters
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#ifndef INCLUDE_CONFIG_USERAPI_H
#define INCLUDE_CONFIG_USERAPI_H
#include "config_load_configmodule.h"
#include "common/utils/utils.h"

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef NOT_uniqCfg
extern configmodule_interface_t *uniqCfg;
#define config_get_if() uniqCfg // Existing code implicit uniq configuration context
#endif
/* utility functions to ease usage of config module structures */
#define CONFIG_GETSOURCE ((config_get_if() == NULL) ? NULL : config_get_if()->cfgmode)
#define CONFIG_ISFLAGSET(P) ( (config_get_if()==NULL) ? 0    : !!(config_get_if()->rtflags & P))
#define CONFIG_SETRTFLAG(P)   if (config_get_if()) { config_get_if()->rtflags |= P; }
#define CONFIG_CLEARRTFLAG(P) if (config_get_if()) { config_get_if()->rtflags &= (~P); }
#define CONFIG_ISPARAMFLAGSET(P,F) ( !!(P.paramflags & F))
int config_paramidx_fromname(paramdef_t *params, int numparams, char *name);

/* utility functions, to be used by configuration module and/or configuration libraries */
void config_printhelp(paramdef_t *, int numparams, char *prefix);
int config_process_cmdline(configmodule_interface_t *cfg, paramdef_t *params, int numparams, char *prefix);
int config_assign_ipv4addr(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *ipv4addr);

/* apis to get/check parameters, to be used by oai modules, at configuration time */
#define CONFIG_CHECKALLSECTIONS "ALLSECTIONS"
int config_check_unknown_cmdlineopt(configmodule_interface_t *cfg, char *prefix);
int config_get(configmodule_interface_t *cfg, paramdef_t *params, int numparams, char *prefix);
int config_getlist(configmodule_interface_t *cfg, paramlist_def_t *ParamList, paramdef_t *params, int numparams, char *prefix);

/* apis to set some of the paramdef_t fields before using the get/getlist api's */
void config_set_checkfunctions(paramdef_t *params, checkedparam_t *checkfunctions, int numparams);

/* apis to retrieve parameters info after calling get or getlist functions */
int config_isparamset(paramdef_t *params, int paramidx);
int config_get_processedint(configmodule_interface_t *cfg, paramdef_t *cfgoption);

/* functions to be used in parameters definition, to check parameters values */
int config_check_intval(configmodule_interface_t *cfg, paramdef_t *param);
int config_check_modify_integer(configmodule_interface_t *cfg, paramdef_t *param);
int config_check_intrange(configmodule_interface_t *cfg, paramdef_t *param);
int config_check_strval(configmodule_interface_t *cfg, paramdef_t *param);
int config_checkstr_assign_integer(configmodule_interface_t *cfg, paramdef_t *param);

#define CONFIG_GETCONFFILE (config_get_if()->cfgP[0])

#ifdef __cplusplus
}
#endif

#endif
