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

/*! \file socket_traci_OMG.h
* \brief The socket interface of TraCI to connect OAI to SUMO. A 'C' reimplementation of the TraCI version of simITS (F. Hrizi, fatma.hrizi@eurecom.fr)
* \author  S. Uppoor
* \date 2012
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/

#ifndef SOCKET_TRACI_OMG_H
#define SOCKET_TRACI_OMG_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "omg.h"
#include "storage_traci_OMG.h"

int sock, portno, msgLength;
struct hostent *host;
struct sockaddr_in server_addr;

/**
 * Global parameters defined in storage_traci_OMG.h
 */
extern storage *tracker;
extern storage *head;
extern storage *storageStart;


/**
 * \fn connection_(char *,int)
 * \brief Talks to SUMO by establishing connection
 * \param Accepts host name and port number
 */
int connection_(char *, int);

/**
 * \fn sendExact(int);
 * \brief Pack the data from storage to buf and write to socket
 * \param Accepts command length
 */
void sendExact(int);

/**
 * \fn  recieveExact(void);
 * \brief Pack the data to storage from buf after reading from socket
 * Returns storage pointer
 */
storage* receiveExact(void);


/**
 * \fn  close_connection(void);
 * \brief close socket connection
 */
void close_connection(void);

#endif
