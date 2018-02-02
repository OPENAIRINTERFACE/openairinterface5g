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

/*! \file rwalk.c
* \brief static  mobility generator
* \author  M. Mahersi, N. Nikaein, J. Harri
* \date 2011
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "static.h"


void
start_static_generator (omg_global_param omg_param_list)
{

  int id;
  static int n_id = 0;
  node_struct *node = NULL;
  mobility_struct *mobility = NULL;

  srand (omg_param_list.seed + STATIC);

  LOG_I (OMG, "Static mobility model for %d %d nodes\n", omg_param_list.nodes,
         omg_param_list.nodes_type);

  for (id = n_id; id < (omg_param_list.nodes + n_id); id++) {

    node = create_node ();
    mobility = create_mobility ();

    node->id = id;
    node->type = omg_param_list.nodes_type;
    node->mob = mobility;
    node->generator = STATIC;
    place_static_node (node); //initial positions
  }

  n_id += omg_param_list.nodes;


}



void
place_static_node (node_struct * node)
{
  if (omg_param_list[node->type].user_fixed && node->type == eNB) {
    if (omg_param_list[node->type].fixed_x <=
        omg_param_list[node->type].max_x
        && omg_param_list[node->type].fixed_x >=
        omg_param_list[node->type].min_x)
      node->x_pos = omg_param_list[node->type].fixed_x;
    else
      node->x_pos =
        (double) ((int)
                  (randomgen
                   (omg_param_list[node->type].min_x,
                    omg_param_list[node->type].max_x) * 100)) / 100;

    node->mob->x_from = node->x_pos;
    node->mob->x_to = node->x_pos;

    if (omg_param_list[node->type].fixed_y <=
        omg_param_list[node->type].max_y
        && omg_param_list[node->type].fixed_y >=
        omg_param_list[node->type].min_y)
      node->y_pos = omg_param_list[node->type].fixed_y;
    else
      node->y_pos =
        (double) ((int)
                  (randomgen
                   (omg_param_list[node->type].min_y,
                    omg_param_list[node->type].max_y) * 100)) / 100;

    node->mob->y_from = node->y_pos;
    node->mob->y_to = node->y_pos;
  } else {

    node->x_pos =
      (double) ((int)
                (randomgen
                 (omg_param_list[node->type].min_x,
                  omg_param_list[node->type].max_x) * 100)) / 100;
    node->mob->x_from = node->x_pos;
    node->mob->x_to = node->x_pos;
    node->y_pos =
      (double) ((int)
                (randomgen
                 (omg_param_list[node->type].min_y,
                  omg_param_list[node->type].max_y) * 100)) / 100;
    node->mob->y_from = node->y_pos;
    node->mob->y_to = node->y_pos;
  }

  node->mob->speed = 0.0;
  node->mob->journey_time = 0.0;

  LOG_I (OMG,
         "[STATIC] Initial position of node ID: %d type(%d: %s):  (X = %.2f, Y = %.2f) speed = 0.0\n",
         node->id, node->type, (node->type==eNB)?"eNB":(node->type==UE)?"UE":"Relay", node->x_pos, node->y_pos);

  node_vector_end[node->type] =
    (node_list *) add_entry (node, node_vector_end[node->type]);

  if (node_vector[node->type] == NULL)
    node_vector[node->type] = node_vector_end[node->type];

  node_vector_len[node->type]++;
}
