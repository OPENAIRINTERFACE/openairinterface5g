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
