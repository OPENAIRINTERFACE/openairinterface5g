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

#ifdef USER_MODE
#define msg printf
#endif
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

