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

/*! \file socket.c
* \brief
* \author Lionel Gauthier
* \date 2011
* \version 1.0
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>

#define SOCKET_C
//#include "openair_defs.h"
#include "socket.h"

#define msg printf
//------------------------------------------------------------------------------
void
socket_setnonblocking (int sockP)
{
  //------------------------------------------------------------------------------

  int             opts;

  opts = fcntl (sockP, F_GETFL);

  if (opts < 0) {
    perror ("fcntl(F_GETFL)");
    exit (EXIT_FAILURE);
  }

  opts = (opts | O_NONBLOCK);

  if (fcntl (sockP, F_SETFL, opts) < 0) {
    perror ("fcntl(F_SETFL)");
    exit (EXIT_FAILURE);
  }

  return;
}

//------------------------------------------------------------------------------
int
make_socket_inet (int typeP, uint16_t * portP, struct sockaddr_in *ptr_addressP)
{
  //------------------------------------------------------------------------------

  int             sock;
  unsigned int             length = sizeof (struct sockaddr_in);
  struct sockaddr_in name;


  assert ((typeP == SOCK_STREAM) || (typeP == SOCK_DGRAM));

  /* Create the socket. */
  sock = socket (PF_INET, typeP, 0);

  if (sock < 0) {
    fprintf (stderr, "ERROR: %s line %d socket %m", __FILE__, __LINE__);
    exit (EXIT_FAILURE);
  }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (*portP);
  name.sin_addr.s_addr = htonl (INADDR_ANY);

  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
    close (sock);
    fprintf (stderr, "ERROR: %s line %d bind port %d %m", __FILE__, __LINE__, *portP);
    exit (EXIT_FAILURE);
  }

  if (ptr_addressP != NULL) {
    getsockname (sock, (struct sockaddr *) ptr_addressP, &length);
  }

  msg("[SOCKET] bound socket port %d\n", *portP);
  return sock;
}

