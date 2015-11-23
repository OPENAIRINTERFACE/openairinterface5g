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

/*! \file client_traci_OMG.h
* \brief The OMG TraCI to send/receive commands from/to  SUMO via socket interfaces.
* \author  S. Uppoor, J. Harri
* \date 2012
* \version 0.1
* \company INRIA, Eurecom
* \email: sandesh.uppor@inria.fr, haerri@eurecom.fr
* \note
* \warning
*/

#ifndef TRACICLIENT_OMG_H
#define TRACICLIENT_OMG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omg.h"

//#include "TraCIConstants.h"
#include "storage_traci_OMG.h"
#include "socket_traci_OMG.h"

#define MAX_ATTEMPTS 5;  // number of connection attemps to SUMO

int targetTime;

char *description;

string_list* departed;  // string list of all vehicles appearing in SUMO at the current time step
string_list* arrived;  // string list of all vehicles leaving SUMO at the current time step

/**
 * Global parameters defined in storage_traci_OMG.h
 */
extern storage *head;
extern storage *storageStart;
extern storage *tracker;
extern int descLen;

/**
 * \fn handshake(char *,int)
 * \brief Talks to SUMO by establishing connection
 * \param Accepts host name and port number
 */
int handshake(char *,int);

/**
 * \fn extractCommandStatus(storage *, unsigned char , char *)
 * \brief Validates the RESPONSE recieved by SUMO
 * \param Accepts storage pointer where data is read and stored, command ID for which response is recieved, description
 */
int extractCommandStatus(storage *, unsigned char , char *);

/**
 * \fn commandSimulationStep(double)
 * \brief Advances the SUMO Simulation
 * \param Accepts number of steps or seconds SUMO should Simulate
 */
void commandSimulationStep(double);

/**
 * \fn commandClose(void);
 * \brief Send termination command to SUMO
 */
void commandClose(void);

/**
 * \fn commandGetVehicleVariable(char *vehID, int varID)
 * \brief Gets the position position and speed info
 * \param Accepts vehicle ID and variable ID, here variable ID is the comaand type
 */
void commandGetVehicleVariable(char *vehID, int varID);


/**
 * \fn get_num_sumo_nodes(void)
 * \brief Return the total number of nodes to be simulated in SUMO
 */
int commandGetMaxSUMONodesVariable(void);

/**
 * \fn init(int max_sim_time)
 * \brief Initialize SUMO and set the periodic subscriptions
 */
void init(int max_sim_time);

/**
 * \fn void processSubscriptions(String_list departed, String_list arrived)
 * \brief process the subscription values sent by SUMO at each SUMO time step
 */
void processSubscriptions(void);

/**
 * \fn void GetSpeed(NodePtr node, char * sumo_id);
 * \brief ask SUMO to return the speed for the indicated vehicle ID
 * \param NodePtr node the pointer to the OAISim node
 * \param char *sumo_id the SUMO ID of the target node
 */
void GetSpeed(node_struct* node, char * sumo_id);

/**
 * \fn void Position(NodePtr node, char * sumo_id);
 * \brief ask SUMO to return the position (X,Y) for the indicated vehicle ID
 * \param NodePtr node the pointer to the OAISim node
 * \param char *sumo_id the SUMO ID of the target node
 */
void GetPosition(node_struct* node, char * sumo_id);


#endif

