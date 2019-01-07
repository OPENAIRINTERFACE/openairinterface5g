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
