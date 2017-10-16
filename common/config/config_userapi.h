/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#ifdef __cplusplus
extern "C"
{
#endif
#define CONFIG_GETSOURCE    ( (config_get_if()==NULL) ? NULL : config_get_if()->cfgmode       )
#define CONFIG_GETNUMP      ( (config_get_if()==NULL) ? 0    : config_get_if()->num_cfgP      )
#define CONFIG_GETP(P)      ( (config_get_if()==NULL) ? NULL : config_get_if()->cfgP[P]       )
#define CONFIG_ISFLAGSET(P) ( (config_get_if()==NULL) ? 0    : !!(config_get_if()->rtflags & P))

extern configmodule_interface_t *config_get_if(void);
extern char * config_check_valptr(paramdef_t *cfgoptions, char **ptr, int length) ;
extern void config_printhelp(paramdef_t *,int numparams);
extern int config_process_cmdline(paramdef_t *params,int numparams, char *prefix);
extern int config_get(paramdef_t *params,int numparams, char *prefix);
extern int config_isparamset(paramdef_t *params,int paramidx);
extern void config_assign_int(paramdef_t *cfgoptions, char *fullname, int val);
extern int config_process_cmdline(paramdef_t *cfgoptions,int numoptions, char *prefix);
#define config_getlist config_get_if()->getlist
#define CONFIG_GETCONFFILE (config_get_if()->cfgP[0])

#ifdef __cplusplus
}
#endif

#endif
