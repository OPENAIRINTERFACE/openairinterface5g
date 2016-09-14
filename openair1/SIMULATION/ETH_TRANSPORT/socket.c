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

