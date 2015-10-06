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


/*! \file cli_server.c
* \brief oai cli server side implementation
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \warning This component can be runned only in user-space
* @ingroup util

*/
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <signal.h>
#include <stdlib.h>

#include "cli.h"
#include "UTIL/LOG/log.h"

#define MAX_NUM_CLIENTS 15
#define MAX_LINE 100

struct sockaddr_in saddr;
unsigned short server_port = 1352;

pthread_t cli_server_listener;
static void * cli_server_listen(void *arg);

cli_handler_t cli_server_handler= NULL;

cli_config *cli_cfg;

int cli_server_init(cli_handler_t handler)
{

  int one = 1;
  cli_init();

  cli_server_handler = handler;

  if ((cli_cfg->sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  if (setsockopt(cli_cfg->sfd, SOL_SOCKET, SO_REUSEADDR, (char *) &one,
                 sizeof(one)) < 0) {
    perror("setsockopt");
    return -1;
  }

  LOG_I(CLI,"socket created %d\n", cli_cfg->sfd);
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(server_port);

  if (bind(cli_cfg->sfd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
    perror("bind");
    return -1;
  }

  /* create telnet listener thread */
  if (pthread_create(&cli_server_listener, NULL, cli_server_listen, NULL)) {
    perror("thread");
    return -1;
  }

  return 0;
}

static void * cli_server_listen(void *arg)
{

  struct sockaddr caddr;
  socklen_t clen;

  clen = sizeof(caddr);

  printf("[CLI] server thread started\n");

  // to be implemented
  while (1) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    if (listen(cli_cfg->sfd, MAX_NUM_CLIENTS) < 0) {
      perror("listen");
      exit(1);
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    printf("[CLI] server Started listening on %d\n", cli_cfg->sfd);

    if ((cli_cfg->cfd = accept(cli_cfg->sfd, &caddr, &clen)) <0) {
      if (cli_cfg->cfd == EINTR )
        continue;
      else {
        perror("error on accept\n");
        exit(1);
      }
    }

    printf("[CLI] server accepted the connection %d from a new client\n", cli_cfg->cfd);
    cli_cfg->exit = 0;
    cli_start(); // init

    do {
      clen = recv(cli_cfg->cfd, buffer, MAX_SOCK_BUFFER_SIZE, 0);

      if (cli_server_handler)
        cli_server_handler(buffer, clen);

    } while (cli_cfg->exit == 0);

    cli_finish();
  }

  pthread_exit(NULL);
}

void cli_server_recv(const void * data, socklen_t len)
{

  char * msg = (char*) data;
  //reomve the newlines
  msg[len]=0;
  msg[len-1]=0;
  msg[len-2]=0;
  //Multiplex to the right handler if needed
  cli_loop(msg);
  //sprintf(buffer, "%s_ACK", msg);
  //len = send(fd, buffer, strlen(buffer), 0);
}


void cli_server_cleanup(void)
{
  /*  if (unlink(telnet_server_socket) < 0) {
  perror("");
  }
  */
  printf("[CLI] server pthread exit ! \n");

  if (cli_cfg->exit) close(cli_cfg->cfd);// close the client first

  pthread_exit(NULL);
  pthread_cancel(cli_server_listener);
  close(cli_cfg->sfd);
}

void  cli_init (void)
{

  cli_cfg = calloc(1, sizeof(cli_config));
  memset(cli_cfg, 0, sizeof(cli_config));
  cli_cfg->exit = 0;
  cli_cfg->port = 1352;
  cli_cfg->promptchar='\0';
  cli_cfg->prompt[0]='\0';
  snprintf(cli_cfg->prompt,200,"%s","oaicli");

}
#ifdef STANDALONE
int main()
{

  printf("start \n");
  cli_init();

  //Main process of telnet  server
  if (cli_server_init(cli_server_recv) < 0) {
    printf("server init failed \n");
    exit(-1);
  }

  while (cli_cfg->exit ==0) {
  }

  printf("server init done 1 \n");
  cli_server_cleanup();
  //pthread_join(telnet_server_listener, NULL);
  printf("server init done \n");


  return 0;
}

#endif

