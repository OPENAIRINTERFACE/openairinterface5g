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
 * \file static.h
 * \brief Prototypes of the functions used for the STATIC model
 * \date 22 May 2011
 *
 */

#ifndef STATIC_H_
#define STATIC_H_

#include "omg.h"

/**
 * \fn void start_static_generator(omg_global_param omg_param_list)
 * \brief Start the STATIC model by setting the initial position of each node
 * \param omg_param_list a structure that contains the main parameters needed to establish the random positions distribution
 */
void start_static_generator(omg_global_param omg_param_list);


/**
 \fn void place_static_node(NodePtr node)
 * \brief Generates a random position ((X,Y) coordinates) and assign it to the node passed as argument. This latter node is then added to the Node_Vector[STATIC]
 * \param node a pointer of type NodePtr that represents the node to which the random position is assigned
 */
void place_static_node(node_struct* node);

#endif /* STATIC_H_ */
