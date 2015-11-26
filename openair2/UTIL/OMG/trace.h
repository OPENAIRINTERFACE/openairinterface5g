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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file trace.h
* \brief The trace-based mobility model for OMG/OAI (mobility is statically imported from a file)
* \author  S. Uppoor
* \date 2011
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/

#ifndef TRACE_H_
#define TRACE_H_
//#include "defs.h"
#include "omg.h"
#include "trace_hashtable.h"
#include "mobility_parser.h"

int start_trace_generator (omg_global_param omg_param_list);

void place_trace_node (node_struct* node, node_data* n);

void move_trace_node (pair_struct* pair, node_data* n_data, double cur_time);

void schedule_trace_node ( pair_struct* pair, node_data* n_data, double cur_time);

void sleep_trace_node ( pair_struct* pair, node_data* n_data, double cur_time);

void update_trace_nodes (double cur_time);

void get_trace_positions_updated (double cur_time);
void clear_list (void);

#endif /* TRACE_H_ */
