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
