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

/*! \file client_traci_OMG.c
* \brief The OMG TraCI to send/receive commands from/to  SUMO via socket interfaces.
* \author  S. Uppoor, J. Harri
* \date 2012
* \version 0.1
* \company INRIA, Eurecom
* \email: sandesh.uppor@inria.fr, haerri@eurecom.fr
* \note
* \warning
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include "client_traci_OMG.h"
#include "TraCIConstants.h"

#define UNUSED_VARIABLE(vARIABLE)   (void)(vARIABLE)

int handshake(char *hoststr,int portno)
{

  check_endianness(); // check endianness
  int i;

  for(i = 0; i< 10; i++) {
    if ( connection_(hoststr,portno) <0 ) {
#ifdef STANDALONE
      printf("connection error...trying again in 3 seconds\n");
#else
      LOG_E(OMG, " Could not connect to TraCIServer - sleeping before trying again...\n");
#endif
      sleep(5);
    } else {
#ifdef STANDALONE
      printf(" SUMO now connected to OMG on host address\n");
#else
      LOG_N(OMG, " SUMO now connected to OMG on host address %s and port %i .\n", hoststr, portno);
#endif
      return 0;
    }
  }

#ifdef STANDALONE
  printf(" SUMO unreachable...giving up...\n");
#else
  LOG_E(OMG, " SUMO unreachable...giving up...\n");
#endif
  return -1;
}

void init(int max_sim_time)
{

#ifdef STANDALONE
  printf(" Initializing TraCI...\n");
#else
  LOG_N(OMG, " Initializing TraCI...\n");
#endif
  char *objID = " ";
  // size_t size = strlen(objID);

  int noSubscribedVars = 2;
  writeUnsignedByte(0);
  writeInt(1 + 4 + 1 + 4 + 4 + 4 + (int) strlen(objID) + 1 + noSubscribedVars);
  writeUnsignedByte(CMD_SUBSCRIBE_SIM_VARIABLE); // command id
  writeInt(0); // begin time
  writeInt(max_sim_time*1000); // end time
  writeString(objID); // object id
  writeUnsignedByte(noSubscribedVars); // variable number
  writeUnsignedByte(VAR_DEPARTED_VEHICLES_IDS);
  writeUnsignedByte(VAR_ARRIVED_VEHICLES_IDS);

  // send request message
  sendExact(storageLength(storageStart));
  extractCommandStatus(receiveExact(), CMD_SUBSCRIBE_SIM_VARIABLE, description);

  if (departed == NULL) {
    departed = (string_list*) malloc(sizeof(string_list)); // departed MUST point to HEAD
    departed->string = NULL;
    departed->next = NULL;
  }

  if (arrived == NULL) {
    arrived = (string_list*) malloc(sizeof(string_list));  // arrived MUST point to HEAD
    arrived->string = NULL;
    arrived->next = NULL;
  }

  processSubscriptions();

  reset();
}

void processSubscriptions()
{
  int noSubscriptions = readInt();

  string_list* tmp_departed = departed;
  string_list* tmp_arrived = arrived;
  int s;

  for (s = 0; s<noSubscriptions; ++s) {

    int respStart = readInt();
    int respLength = readUnsignedByte();

    if (respLength == 0)
      respLength = readInt();


    int cmdId = readUnsignedByte();

    if (cmdId<0xe0||cmdId>0xef) {  // only responses to subscription to supported types (vehicles, TLC, polygones...) are accepted
      //LOG_W(OMG, " Invalide Subscription response: %d\n",cmdId);
      //printf(" Invalide Subscription response: %d\n",cmdId);
      return;
    }

    char *objID = readString();
    int varNo = readUnsignedByte();

    UNUSED_VARIABLE(objID);
    UNUSED_VARIABLE(respStart);

    int i;

    for (i=0; i<varNo; ++i) {
      int varID = readUnsignedByte();

      bool ok = readUnsignedByte()==RTYPE_OK;
      int valueDataType = readUnsignedByte();
      UNUSED_VARIABLE(valueDataType);

      if (ok&&cmdId==CMD_SUBSCRIBE_SIM_VARIABLE+0x10&&varID==VAR_DEPARTED_VEHICLES_IDS) {
        tmp_departed = readStringList(tmp_departed);
        //printf(" OMG Got departed cars\n");
        continue;
      }

      if (ok&&cmdId==CMD_SUBSCRIBE_SIM_VARIABLE+0x10&&varID==VAR_ARRIVED_VEHICLES_IDS) {
        tmp_arrived = readStringList(tmp_arrived);
        //printf(" OMG Got arrived cars\n");
        continue;
      }
    }

    free( objID );
  }
}

int extractCommandStatus(storage *s, unsigned char commandId, char * description)
{
  int success=0;

  if(s == NULL) {
    //LOG_E(OMG, " client_traci_OMG::extractCommandStatus():  Tracker is NULL \n");
    //printf(" client_traci_OMG::extractCommandStatus():  Tracker is NULL \n");
    return success=0;
  }

  // validate the the message response from SUMO
  int storageLength_ = storageLength(s);

  // tracker currently points to the begining of the recieved data in the linked list
  tracker = s;
  //         storage *freeTracker = tracker;   // save it for calling free

  int commandLength = readUnsignedByte();

  // CommandID needs to fit
  int rcvdCommandId = readUnsignedByte();

  if (rcvdCommandId == (int)commandId) {
    //printf("%d",rcvdCommandId);
    //LOG_E(OMG, " Server answered to command\n");
    //printf(" Server answered to command\n");
  }

  // Get result and description
  unsigned char result = readUnsignedByte();

  if (result != RTYPE_OK) {

    printf(" Server returned error\n");
    return success=0;

  }

  if (result == RTYPE_OK) {
    // printf(" Server returned success\n");
    success=1;
  }

  description = readString();
  // print description if needed
  free( description );
  description = 0;

  //free actual message content
  //depends on the message which is handled

  /*   if (commandId != CMD_GET_VEHICLE_VARIABLE)
  freeStorage(freeTracker);*/

  UNUSED_VARIABLE(commandLength);
  UNUSED_VARIABLE(storageLength_);

  return success;
}


void commandSimulationStep(double time)
{
  // progress the simulation in SUMO
  // reset is used to initalize the global parameters
  reset();
  // Send command
  writeUnsignedByte(1 + 1 + 4); // command length
  writeUnsignedByte(CMD_SIMSTEP2); // look up TraCIConstants.h
  //   writeInt((int)(time*1000)); // TraCI accepts time in milli seconds
  writeInt((int)(time)); // TraCI accepts time in milli seconds
  sendExact(storageLength(storageStart));

  extractCommandStatus(receiveExact(), CMD_SIMSTEP2, description);

  if (departed == NULL) {
    departed = (string_list*) malloc(sizeof(string_list)); // departed MUST point to HEAD
    departed->string = NULL;
    departed->next = NULL;
  }

  if (arrived == NULL) {
    arrived = (string_list*) malloc(sizeof(string_list));  // arrived MUST point to HEAD
    arrived->string = NULL;
    arrived->next = NULL;
  }

  /* if (departed == NULL)
  departed = (String_list)malloc(sizeof(String_list));  // departed MUST point to HEAD

  if (arrived == NULL)
  arrived = (String_list)malloc(sizeof(String_list));  // departed MUST point to HEAD
  */

  processSubscriptions();

}

void commandClose()
{
  reset();
  // command length
  writeUnsignedByte(0x02);
  // command id
  writeUnsignedByte(CMD_CLOSE);

  // send request message
#ifdef STANDALONE
  printf("closing the socket... \n");
#else
  LOG_N(OMG,"closing the socket... \n");
#endif
  sendExact(storageLength(storageStart));
  extractCommandStatus(receiveExact(), CMD_CLOSE, description);

}


void commandGetVehicleVariable(char *vehID, int varID)// malloc for vehID and varID depends on speed or position
{
  reset();
  int domID = CMD_GET_VEHICLE_VARIABLE;//0xa4 specific for get vehicle variable command

  // command length
  writeUnsignedByte(1 + 1 + 1 + 4 + (int)strlen(vehID));
  // command id
  writeUnsignedByte(CMD_GET_VEHICLE_VARIABLE);
  // variable id
  writeUnsignedByte(varID);
  // object id
  writeString(vehID);

  // send request message
  sendExact(storageLength(storageStart));

  // receive answer message
  //receiveExact();
  if (extractCommandStatus(receiveExact(), CMD_GET_VEHICLE_VARIABLE, description)) { //<---RESPONSE_GET_VEHICLE_VARIABLE

    // validate result state
    if(tracker == NULL) {
      // LOG_E(OMG, " client_traci_OMG::commandGetVehicleVariable():  Tracker is NULL \n");
      //printf(" client_traci_OMG::commandGetVehicleVariable():  Tracker is NULL \n");
      return;
    }

    int length = readUnsignedByte();

    if(length ==0)
      length = readInt();

    int cmdId =readUnsignedByte();

    if (cmdId != (CMD_GET_VEHICLE_VARIABLE+0x10)) {
      //LOG_E(OMG, " Wrong response recieved\n");
      //printf(" Wrong response recieved\n");
      return;
    }

    int VariableID = readUnsignedByte();
    char* rs = readString();

    int valueDataType = readUnsignedByte();
    UNUSED_VARIABLE(VariableID);
    UNUSED_VARIABLE(rs);
    UNUSED_VARIABLE(valueDataType);
    UNUSED_VARIABLE(domID);

    free( rs );
  }
}

// TODO not working for now..need to find a way to get the same info without using CMD_SCENARIO (as not implemented by SUMO)
int commandGetMaxSUMONodesVariable()
{
  reset();

  int max_car = 0;

  // command length
  writeUnsignedByte(1 + 1 + 1 + 1 + 4 + 1 + 1 + 4);
  // command id
  writeUnsignedByte(CMD_SCENARIO);
  // flag
  writeUnsignedByte(0x00); // GET command for the generic environment-related values
  // command id
  //writeUnsignedByte(CMD_SCENARIO);
  // domain id
  writeUnsignedByte(0x01); // vehicle

  writeInt(0); // first vehicular domain

  // variable id
  writeUnsignedByte(DOMVAR_MAXCOUNT); // get maximum number of vehicles

  writeUnsignedByte(TYPE_INTEGER); // get maximum number of vehicles
  writeInt(max_car); // get maximum number of vehicles

  // send request message
  sendExact(storageLength(storageStart));

  // receive answer message
  if (extractCommandStatus(receiveExact(), CMD_SCENARIO, description)) { //<---RESPONSE_GET_VEHICLE_VARIABLE

    // validate result state

    if(tracker == NULL) {
      //LOG_E(OMG, " client_traci_OMG::commandGetMaxSUMONodesVariable():  Tracker is NULL \n");
      return -1;
    }

    int res = readUnsignedByte();
    int Length = readUnsignedByte(); // to check with Int
    int cmdId =readUnsignedByte();

    if (cmdId != (CMD_SCENARIO)) {
      //LOG_E(OMG, " Wrong response recieved \n");
      return -1;
    }

    int flag = readUnsignedByte();
    int dom = readUnsignedByte(); // domain
    int domID = readInt(); // domain ID
    int VariableID = readUnsignedByte();

    int valueDataType = readUnsignedByte();

    if (valueDataType == TYPE_INTEGER) {
      max_car = readInt();
      //LOG_N(OMG, " max Number SUMO nodes is: %f \n", max_car);
    } else {
      //LOG_W(OMG, " No Matching Data Type Value \n");
    }

    UNUSED_VARIABLE(res);
    UNUSED_VARIABLE(Length);
    UNUSED_VARIABLE(flag);
    UNUSED_VARIABLE(dom);
    UNUSED_VARIABLE(domID);
    UNUSED_VARIABLE(VariableID);
  }

  return max_car;
}




void GetSpeed(node_struct* node, char * sumo_id)
{
  commandGetVehicleVariable(sumo_id, VAR_SPEED);
  double speed_double = readDouble();
  node->mob->speed = speed_double;
}

void GetPosition(node_struct* node, char * sumo_id)
{
  commandGetVehicleVariable(sumo_id, VAR_POSITION);
  double x_double = readDouble();

  if (x_double < 0.0)
    x_double = 0.0;

  double y_double = readDouble();

  if (y_double < 0.0)
    y_double = 0.0;

  node->x_pos = x_double;
  node->y_pos = y_double;
}
