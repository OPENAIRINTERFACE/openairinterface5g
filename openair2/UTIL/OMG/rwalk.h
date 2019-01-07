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

/**
 * \file rwalk.h
 * \brief Functions used for the RWALK Model
 * \date 22 May 2011
 *
 */
#ifndef RWALK_H_
#define RWALK_H_

#include "omg.h"

/**
 * \fn void start_rwalk_generator(omg_global_param omg_param_list)
 * \brief Start the RWALK model by setting the initial positions of each node then letting it sleep for a random duration. This new job is then added to the Job_Vector
 * \param omg_param_list a structure that contains the main parameters needed to establish the random positions distribution
 */
int start_rwalk_generator(omg_global_param omg_param_list) ;

/**
 * \fn void place_rwalk_node(NodePtr node)
 * \brief Called by the function start_rwalk_generator(), it generates a random position ((X,Y) coordinates)  for a node and add it to the corresponding Node_Vector_Rwp
 * \param node a pointer of type NodePtr that represents the node to which the random position is assigned
 */
void place_rwalk_node(node_struct* node) ;

/**
 * \fn Pair sleep_rwalk_node(NodePtr node, double cur_time)
 * \brief Called by the function start_rwalk_generator(omg_global_param omg_param_list), it allows the node to sleep for a random duration starting from the current time
 * \param node a pointer of type NodePtr that represents the node to which the random sleep duration is assigned
 * \param cur_time a variable of type double that represents the current time
 * \return A Pair structure containing the node and the time when it is reaching the destination
 */
void sleep_rwalk_node(pair_struct* pair, double cur_time) ;


/**
 * \fn Pair move_rwalk_node(NodePtr node, double cur_time)
 * \brief Called by the function update_rwalk_nodes(double cur_time), it computes the next destination of a node ((X,Y) coordinates and the arrival time at the destination)
 * \param node a variable of type NodePtr that represents the node that has to move
 * \param cur_time a variable of type double that represents the current time
 * \return A Pair structure containing the node structure and the arrival time at the destination
 */
void move_rwalk_node(pair_struct* pair, double cur_time) ;

/**
 * \fn void update_rwalk_nodes(double cur_time)
 * \brief Update the positions of the nodes. After comparing the current time to the first job_time, it is decided wether to start
 * \  a new job or to keep the current job
 * \param cur_time a variable of type double that represents the current time
 */
void update_rwalk_nodes(double cur_time) ; // need to implement an out-of-area check as well as a rebound function to stay in the area


/**
 * \fn void get_rwalk_positions_updated(double cur_time)
 * \brief Compute the positions of the nodes at a given time in case they are moving (intermediate positions). Otherwise, generate a message saying that
 *  the nodes are still sleeping
 * \param cur_time a variable of type double that represents the current time
 */
void get_rwalk_positions_updated(double cur_time) ;

/*for perfect simulation of random walk*/
double residualtime(omg_global_param omg_param);

#endif /* RWALK_H_ */
