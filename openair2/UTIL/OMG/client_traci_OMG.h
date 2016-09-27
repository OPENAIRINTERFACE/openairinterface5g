/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

