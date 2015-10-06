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

/*! \file sumo.h
* \brief The OMG interface to SUMO (Simulation of Urban Mobility)
* \author  S. Uppoor, J. Harri
* \date 2012
* \version 0.1
* \company INRIA, Eurecom
* \email: sandesh.uppor@inria.fr, haerri@eurecom.fr
* \note
* \warning
*/

#ifndef SUMO_H_
#define SUMO_H_

#include "omg.h"
#include "id_manager.h"
#include <stdio.h>

#define MIN_SUMO_STEP 100  // 100ms is the smallest allowed SUMO update step

/*! A global variable used to store the SUMO process ID to later kill it smoothly when OAI askes OMG to stop SUMO*/
pid_t pid;

/*! A sumo global variable representing the OMG-SUMO ID manager. It is created at the start_sumo_generator(void) and keep a mapping between SUMO and OAI IDs.  */
IDManagerPtr id_manager;

/*! A Node_list intended to contain the list of OAI 'active' nodes in SUMO. replaces the return of the full Node_Vector when OAI requests the new positions of its nodes. The list is reset at each use, but the pointer to the respective nodes is just set to NULL*/
node_list* active_nodes;

/*! A global variable keeping track of the last update time of SUMO, to be used to get the update interval when update_sumo_nodes(cur_time) is called */
double last_update_time;

/**
 * \fn void start_sumo_generator(omg_global_param omg_param_list)
 * \brief Start SUMO by setting the initial positions of each node then letting it sleep till it departs to its destination and add this job to the Job_Vector
 * \param omg_param_list a structure that contains the main parameters needed to establish the random positions distribution
 */
int start_sumo_generator(omg_global_param omg_param_list) ;

/**
 * \fn void update_sumo(double cur_time)
 * \brief Update SUMO simulation time by x timesteps. After advancing the SUMO for the next timestep, SUMO will automatically update all nodes positions.
 * Note that the SUMO positions are kept by SUMO. One need to specifically query SUMO to get them.
 * \param cur_time a variable of type double that represents the current time
 */
void update_sumo_nodes(double cur_time) ;

/**
 * \fn void get_sumo_positions(NodePtr node)
 * \brief Get the current position and speed of a node from SUMO. Invokes TraCI
 * \param node the pointer to a node we want to synch with SUMO.
 */
void update_sumo_positions(node_struct* node);

/**
 * \fn Node_list get_sumo_positions_updated(double cur_time);
 * \brief Get the current position and speed of a list of nodes. First, OMG retrieves the list of ACTIVE nodes, and second it updates their position from SUMO. Invokes TraCI
 * \param cur_time a variable of type double that represents the current time
 * \return the list of ACTIVE OAI nodes, which positions have been updated by SUMO
 */
node_list* get_sumo_positions_updated(double cur_time);

/**
 * \fn NodePtr get_first_inactive_OAI_node(Node_list list, int node_type)
 * \brief Get the first inactive OAI node to map it to a new SUMO node. An inactive OAI node has its mobility flag set to '0'.
 * \param node the pointer to the Node_Vector containing all OAI nodes
 * \param node_type the type of node we would like to locate (here: SUMO)
 * \return the reference to the first inactive OAI node; returns NULL is none could be found (all OAI nodes are currently active)
 */
node_struct* get_first_inactive_OAI_node(node_list* list, int node_type);

/**
 * \fn void update_IDs(String_list *departed, String_list *arrived)
 * \brief Updates the mapping between SUMO and OAI nodes; Once a node departs in SUMO, it is mapped to a inactive OAI node. If none are found, the SUMO node will not have any mapping. Once a node arrives, the mapping is removed and the OAI node becomes inactive again. When an OAI node is inactive, it mobility parameters are invalid and MUST NOT be used/requested by OAI.
 */
void update_IDs(void);

/**
 * \fn bool desactive_and_unmap(char *sumo_id)
 * \brief desactivates an OAI node as it is no longer in SUMO; will be recycled by a new SUMO node in the future;
 * \param sumo_id the string representing the SUMO ID of the OAI node;
 * \return true in case of success; false otherwise;
 */
bool desactivate_and_unmap(char *sumo_id);

/**
 * \fn bool activate_and_map(void)
 * \brief activated an OAI node as a SUMO node entered in SUMO; Conceptually, this represents adding a communication device (OAI) to a SUMO node;
 * It will find the first inactive OAI node, activates it and add the sumo mapping;
 * \param sumo_id the string representing the SUMO ID of the OAI node;
 * \return true in case of success; false if the maximum of OAI node has been reached
 */
bool activate_and_map(char *sumo_id);

/**
 * \fn int stop_sumo_generator(void)
 * \brief stops SUMO, stop the socket and kills SUMO process in the child domain.
 * \return true in case of success; false otherwise
 */
bool stop_sumo_generator(void);

#endif /* SUMO_H_ */
