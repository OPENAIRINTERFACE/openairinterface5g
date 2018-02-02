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

/*! \file steadystaterwp.h
* \brief random waypoint mobility generator
* \date 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/
#ifndef STEADYSTATERWP_H_
#define STEADYSTATERWP_H_


int start_steadystaterwp_generator (omg_global_param omg_param_list);

void place_steadystaterwp_node (node_struct* node);

void sleep_steadystaterwp_node (pair_struct* pair, double cur_time);

void move_steadystaterwp_node (pair_struct* pair, double cur_time);

double pause_probability(omg_global_param omg_param);


double initial_pause(omg_global_param omg_param);

double initial_speed(omg_global_param omg_param);

void update_steadystaterwp_nodes (double cur_time);

void get_steadystaterwp_positions_updated (double cur_time);

#endif











