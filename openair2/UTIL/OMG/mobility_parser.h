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
