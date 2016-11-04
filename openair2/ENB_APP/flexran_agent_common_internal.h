/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file flexran_agent_common_internal.h
 * \brief internal agent functions for common message primitves and utilities
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_COMMON_INTERNAL_H_
#define FLEXRAN_AGENT_COMMON_INTERNAL_H_

#include <yaml.h>

#include "flexran_agent_defs.h"

int apply_reconfiguration_policy(mid_t mod_id, const char *policy, size_t policy_length);

int apply_parameter_modification(void *parameter, yaml_parser_t *parser);

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
