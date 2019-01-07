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

/*! \file mobility_parser.h
* \brief A parser for trace-based mobility information (parsed from a file)
* \author  S. Uppoor
* \date 2011
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/

#ifndef MOBILITY_PARSER_H_
#define MOBILITY_PARSER_H_

#include "trace_hashtable.h"


/*
 * function reads each line and checks if vehicle has an entry in the hastable,
 * if so append append is called else a new linked list to the vehicle is created.
 * @param need mobility file to be given
 */

void parse_data (char *trace_file, int node_type);  // mobility file need to be given here, add in omg_param_list, get it from there

/**
 * function builds a linked list which holds vehicle id and its mapping pointer to
 * the head of the linked list in the hash table.
 * @param headRef is the pointer to this linked list
 * @param vid is the vehicle id
 * @param pointer to the head of the linked list to the vehicle entry in the hash table
 */
void add_node_info (int nid,int n_gid, int node_type);
int find_node_info (int vid, int node_type);

/**
 * just counts the number of nodes in the mobility file
 */
int get_number_of_nodes (int node_type);




/**
 * function returns the node containing next position (just reads the linked list from the hashtable)
 * and never repeats, each time next location ahead of the current is returned
 * @param hashtable from which the node is to be looked
 * @param node_id is the nodes whose next location need to be retrieved from the linked list
 */
node_data* get_next_data (hash_table_t* table, int vid, int flag);



#endif /* MOBILITY_PARSER_H_ */
