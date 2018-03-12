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

/*! \file flexran_agent_common_internal.h
 * \brief internal agent functions for common message primitves and utilities
 * \author Xenofon Foukas and N. Nikaein
 * \date 2017
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_COMMON_INTERNAL_H_
#define FLEXRAN_AGENT_COMMON_INTERNAL_H_

#include <yaml.h>

#include "flexran_agent_defs.h"

int apply_reconfiguration_policy(mid_t mod_id, const char *policy, size_t policy_length);

int apply_parameter_modification(void *parameter, yaml_parser_t *parser);

int parse_enb_id(mid_t mod_id, yaml_parser_t *parser);
int parse_enb_config_parameters(mid_t mod_id, yaml_parser_t *parser) ;


// This can be used when parsing for a specific system that is not yet implmeneted
// in order to skip its configuration, without affecting the rest
int skip_system_section(yaml_parser_t *parser);

// This can be used when parsing for a specific subsystem that is not yet implmeneted
// in order to skip its configuration, without affecting the rest
int skip_subsystem_section(yaml_parser_t *parser);

// This can be used when parsing for the parameters of a specific subsystem 
//that is not yet implmeneted in order to skip its configuration, without affecting the rest
int skip_subsystem_parameters_config(yaml_parser_t *parser);

// This can be used when configuring the parameters of a specific subsystem 
//that is not yet implmeneted in order to skip its configuration, without affecting the rest
int skip_parameter_modification(yaml_parser_t *parser);

#endif
