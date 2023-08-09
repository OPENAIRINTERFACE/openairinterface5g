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
#ifndef INCLUDE_CONFIG_COMMON_H
#define INCLUDE_CONFIG_COMMON_H
#include "config_load_configmodule.h"
#include "common/utils/utils.h"

#ifdef __cplusplus
extern "C"
{
#endif
void config_check_valptr(configmodule_interface_t *cfg, paramdef_t *cfgoptions, int elt_sz, int nb_elt);
/* functions to set a parameter to its default value */
int config_setdefault_int(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
int config_setdefault_int64(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
int config_setdefault_intlist(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
int config_setdefault_string(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
int config_setdefault_stringlist(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
int config_setdefault_double(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
int config_setdefault_ipv4addr(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix);
void *config_allocate_new(configmodule_interface_t *cfg, int sz, bool autoFree);
void config_assign_int(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *fullname, int val);

#ifdef __cplusplus
}
#endif

#endif
