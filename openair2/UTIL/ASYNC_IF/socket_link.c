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

/*! \file socket_link.c
 * \brief this is the implementation of a TCP socket ASYNC IF
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */

#include "socket_link.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdint.h>

socket_link_t *new_link_server(int port)
{
  socket_link_t      *ret = NULL;
  int                reuse;
  struct sockaddr_in addr;
  socklen_t          addrlen;
  int                socket_server = -1;
  int no_delay;

  
  ret = calloc(1, sizeof(socket_link_t));
  if (ret == NULL) {
    LOG_E(MAC, "%s:%d: out of memory\n", __FILE__, __LINE__);
    goto error;
  }
  ret->socket_fd = -1;


  //printf("MAC create a new link server socket at port %d\n", port);

  socket_server = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_server == -1) {
    LOG_E(MAC, "%s:%d: socket: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  reuse = 1;
  if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {

    LOG_E(MAC, "%s:%d: setsockopt: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  no_delay = 1;
  if (setsockopt(socket_server, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay)) == -1) {

    LOG_E(MAC, "%s:%d: setsockopt: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(socket_server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    LOG_E(MAC, "%s:%d: bind: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  if (listen(socket_server, 5)) {
    LOG_E(MAC, "%s:%d: listen: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  addrlen = sizeof(addr);
  ret->socket_fd = accept(socket_server, (struct sockaddr *)&addr, &addrlen);
  if (ret->socket_fd == -1) {
    LOG_E(MAC, "%s:%d: accept: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  //printf("MAC connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  return ret;

error:
  close(socket_server);
  if (ret != NULL) close(ret->socket_fd);
  free(ret);

  LOG_E(MAC, "ERROR in new_link_server (see above), returning NULL\n");
  return NULL;
}

socket_link_t *new_link_client(char *server, int port)
{
  socket_link_t      *ret = NULL;
  struct sockaddr_in addr;
  int no_delay;

  ret = calloc(1, sizeof(socket_link_t));
  if (ret == NULL) {
    LOG_E(MAC, "%s:%d: out of memory\n", __FILE__, __LINE__);
    goto error;
  }
  ret->socket_fd = -1;

  LOG_D(MAC, "create a new link client socket connecting to %s:%d\n", server, port);

  ret->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (ret->socket_fd == -1) {
    LOG_E(MAC, "%s:%d: socket: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  no_delay = 1;
  if (setsockopt(ret->socket_fd, SOL_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay)) == -1) {
    LOG_E(MAC, "%s:%d: setsockopt: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_aton(server, &addr.sin_addr) == 0) {
    LOG_E(MAC, "invalid IP address '%s', use a.b.c.d notation\n", server);
    goto error;
  }
  if (connect(ret->socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    LOG_E(MAC, "%s:%d: connect: %s\n", __FILE__, __LINE__, strerror(errno));
    goto error;
  }

  LOG_D(MAC, "connection to %s:%d established\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  return ret;

error:
  if (ret != NULL) close(ret->socket_fd);
  free(ret);
  LOG_E(MAC, "ERROR in new_link_client (see above), returning NULL\n");
  return NULL;
}

socket_link_t *new_link_udp_server(int port){

  socket_link_t  *ret = NULL;

  struct sockaddr_in si_me, si_other;
  int socket_server, i, slen = sizeof(si_other) , recv_bytes;
  char buf[1500];

  ret = calloc(1, sizeof(socket_link_t));
  if (ret == NULL) {
    LOG_D(PROTO_AGENT, "%s:%d: out of memory\n", __FILE__, __LINE__);
    goto error;
  }
  ret->socket_fd = -1;

  //printf("PROTO_AGENT: create a new udp link server socket at port %d\n", port);

  //create a UDP socket
  if ((socket_server=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        goto error;
  }

  // zero out the structure
  memset((char *) &si_me, 0, sizeof(si_me));

  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(port);
  si_me.sin_addr.s_addr = INADDR_ANY;

  //bind socket to port
  if( bind(socket_server , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        goto error;
  }
  ret->socket_fd = socket_server;
  ret->peer_port = 0;
  return ret;
  
error:
  close(socket_server);
  if (ret != NULL) close(ret->socket_fd);
  free(ret);
  //printf("\n\n\nERROR PROTO_AGENT: ERROR in new_link_udp_server (see above), returning NULL\n");
  return NULL;
}


socket_link_t *new_link_udp_client(char *server, int port){

  socket_link_t      *ret = NULL;
  ret = calloc(1, sizeof(socket_link_t));
  if (ret == NULL) {
    LOG_E(MAC, "%s:%d: out of memory\n", __FILE__, __LINE__);
    //goto error;
  }
  ret->socket_fd = -1;

  struct sockaddr_in si_other, server_info;
  int s, i, slen=sizeof(si_other);
  char buf[1500];
  char message[1500];
 
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        goto error;
  }
 
  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = 0; //htons(port);
  

  
  if (inet_aton(server, &si_other.sin_addr) == 0){
        fprintf(stderr, "inet_aton() failed\n");
        goto error;
  }
  connect(s, (struct sockaddr *)&si_other, sizeof(si_other)); 
  
  getsockname(s, (struct sockaddr *)&si_other, &slen);

  ret->socket_fd = s;
  ret->peer_port = port; //ntohs(si_other.sin_port);
 
  return ret;
error:
  if (ret != NULL) close(ret->socket_fd);
  free(ret);
  LOG_E(MAC, "ERROR in new_link_udp_client (see above), returning NULL\n");
  return NULL;
}


socket_link_t *new_link_sctp_server(int port)
{

  socket_link_t  *ret = NULL;
  ret = calloc(1, sizeof(socket_link_t));
  if (ret == NULL) {
    LOG_D(PROTO_AGENT, "%s:%d: out of memory\n", __FILE__, __LINE__);
    goto error;
  }
  ret->socket_fd = -1;

  int listenSock, temp;
  struct sockaddr_in servaddr;
 
  listenSock = socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP);
  if(listenSock == -1)
  {
      perror("socket()");
      exit(1);
  }
 
  bzero ((void *) &servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);
 
  temp = bind (listenSock, (struct sockaddr *) &servaddr, sizeof (servaddr));
 
  if(temp == -1 )
  {
      perror("bind()");
      close(listenSock);
      exit(1);
  }
 
  temp = listen (listenSock, 5);
    if(temp == -1 )
  {
      perror("listen()");
      close(listenSock);
      exit(1);
  }

  ret->socket_fd = accept (listenSock, (struct sockaddr *) NULL, (int *) NULL);
  
  return ret;

error:
  close(listenSock);
  if (ret != NULL) close(ret->socket_fd);
  free(ret);
  LOG_E(MAC,"ERROR in new_link_sctp_server (see above), returning NULL\n");
  return NULL;
}

socket_link_t *new_link_sctp_client(char *server, int port)
{

  socket_link_t      *ret = NULL;
  ret = calloc(1, sizeof(socket_link_t));
  if (ret == NULL) {
    LOG_D(PROTO_AGENT, "%s:%d: out of memory\n", __FILE__, __LINE__);
    goto error;
  }
  ret->socket_fd = -1;

  int temp;
  struct sockaddr_in servaddr;
    
  ret->socket_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP);
 
  if (ret->socket_fd == -1)
  {
      perror("socket()");
      exit(1);
  }
 
  bzero ((void *) &servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons (port);

  if (inet_aton(server, &servaddr.sin_addr) == 0) {
    LOG_E(MAC,"invalid IP address '%s', use a.b.c.d notation\n", server);
    goto error;
  }

  temp = connect (ret->socket_fd, (struct sockaddr *) &servaddr, sizeof (servaddr));
 
  if (temp == -1)
  {
      perror("connect()");
      close(ret->socket_fd);
      exit(1);
  }

  return ret;

error:
  if (ret != NULL) close(ret->socket_fd);
  free(ret);
  LOG_E(MAC, "ERROR in new_link_sctp_client (see above), returning NULL\n");
  return NULL;
}

static int socket_udp_send(int socket_fd, void *buf, int size, char *peer_addr, int port)
{
  
  struct sockaddr_in si_other;
  int slen = sizeof(si_other);
  char *s = buf;
  int   l;
  int my_socket;
  
  LOG_D(PROTO_AGENT,"UDP send\n");

  my_socket = socket_fd;
  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(port);
     
  if (inet_aton(peer_addr , &si_other.sin_addr) == 0) 
  {
      fprintf(stderr, "inet_aton() failed\n");
      exit(1);
  }

  while (size) {
    l = sendto(my_socket, s, size, 0, (struct sockaddr *) &si_other, slen);
    if (l == -1) goto error;
    if (l == 0) { printf("\n\n\nERROR PROTO_AGENT: %s:%d: this cannot happen, normally...\n", __FILE__, __LINE__); abort(); }
    size -= l;
    s += l;
  }
  
  return 0;
error:
  LOG_E(MAC,"socket_udp_send: ERROR: %s\n", strerror(errno));
  return -1;
}

static int socket_udp_receive(int socket_fd, void *buf, int size)
{
  LOG_D(PROTO_AGENT,"UDP RECEIVE\n");

  struct sockaddr_in client;
  int slen = sizeof(client);
  char *s = buf;
  int   l;

  while (size) {
    l = recvfrom(socket_fd, s, size, 0, (struct sockaddr *) &client, &slen);
    getsockname(s, (struct sockaddr *)&client, &slen);
    LOG_D(PROTO_AGENT, "Got message from src port: %u\n", ntohs(client.sin_port));
    if (l == -1) goto error;
    if (l == 0) goto socket_closed;
    size -= l;
    s += l;
  }

  return ntohs(client.sin_port);

error:
  LOG_E(MAC, "socket_udp_receive: ERROR: %s\n", strerror(errno));
  return -1;

socket_closed:
  LOG_E(MAC, "socket_udp_receive: socket closed\n");
  return -1;
}


/*
 * return -1 on error and 0 if the sending was fine
 */
static int socket_send(int socket_fd, void *buf, int size)
{
  char *s = buf;
  int   l;

  while (size) {
    l = send(socket_fd, s, size, MSG_NOSIGNAL);
    if (l == -1) goto error;
    if (l == 0) { LOG_E(MAC, "%s:%d: this cannot happen, normally...\n", __FILE__, __LINE__); abort(); }
    size -= l;
    s += l;
  }

  return 0;

error:
  LOG_E(MAC, "socket_send: ERROR: %s\n", strerror(errno));
  return -1;
}

/*
 * return -1 on error and 0 if the receiving was fine
 */
static int socket_receive(int socket_fd, void *buf, int size)
{
  char *s = buf;
  int   l;

  while (size) {
    l = read(socket_fd, s, size);
    if (l == -1) goto error;
    if (l == 0) goto socket_closed;
    size -= l;
    s += l;
  }

  return 0;

error:
  LOG_E(MAC, "socket_receive: ERROR: %s\n", strerror(errno));
  return -1;

socket_closed:
  LOG_E(MAC, "socket_receive: socket closed\n");
  return -1;
}

/*
 * return -1 on error and 0 if the sending was fine
 */
int link_send_packet(socket_link_t *link, void *data, int size, uint16_t proto_type, char *peer_addr, int port)
{
  char sizebuf[4];
  int32_t s = size;

  /* send the size first, maximum is 2^31 bytes */
  sizebuf[0] = (s >> 24) & 255;
  sizebuf[1] = (s >> 16) & 255;
  sizebuf[2] = (s >> 8) & 255;
  sizebuf[3] = s & 255;
  if ((proto_type == 0) || (proto_type == 2))
  {
    if (socket_send(link->socket_fd, sizebuf, 4) == -1)
      goto error;

    link->bytes_sent += 4;

    if (socket_send(link->socket_fd, data, size) == -1)
      goto error;

    link->bytes_sent += size;
    link->packets_sent++;
  }
  else if (proto_type == 1 )
  {
    while (link->peer_port == 0)
    {
      sleep(0.1);
    }
    LOG_D(PROTO_AGENT, "peer port is %d", link->peer_port);
    if (socket_udp_send(link->socket_fd, sizebuf, 4, peer_addr, link->peer_port) == -1)
      goto error;
    
    LOG_I(PROTO_AGENT,"sent %d bytes over the channel\n", (int32_t *)sizebuf);
    link->bytes_sent += 4;

    if (socket_udp_send(link->socket_fd, data, size, peer_addr, link->peer_port) == -1)
      goto error;

    link->bytes_sent += size;
    link->packets_sent++;
  }

  return 0;

error:
  return -1;
}

/*
 * return -1 on error and 0 if the sending was fine
 */

int link_receive_packet(socket_link_t *link, void **ret_data, int *ret_size, uint16_t proto_type, char *peer_addr, int port)
{
  unsigned char sizebuf[4];
  int32_t       size;
  void          *data = NULL;
  
  int peer_port = 0;
  /* received the size first, maximum is 2^31 bytes */
  if ((proto_type == 0) || (proto_type == 2))
  {
    if (socket_receive(link->socket_fd, sizebuf, 4) == -1)
      goto error;
  }
  else if (proto_type == 1)
  {
      /* received the size first, maximum is 2^31 bytes */
    peer_port = socket_udp_receive(link->socket_fd, sizebuf, 4);
      if ( peer_port == -1)
        goto error;
    if (peer_port == 0) link->peer_port = peer_port;
  }
  
  size = (sizebuf[0] << 24) |
         (sizebuf[1] << 16) |
         (sizebuf[2] << 8)  |
          sizebuf[3];

  link->bytes_received += 4;


  data = malloc(size);
  if (data == NULL) {
    LOG_E(MAC, "%s:%d: out of memory\n", __FILE__, __LINE__);
    goto error;
  }
  if ((proto_type == 0) || (proto_type == 2))
  {
  
    if (socket_receive(link->socket_fd, data, size) == -1)
      goto error;
  }
  else if (proto_type == 1)
  { 
    if (socket_udp_receive(link->socket_fd, data, size) == -1)
      goto error;
  }

  link->bytes_received += size;
  link->packets_received++;

  *ret_data = data;
  *ret_size = size;
  return 0;

error:
  free(data);
  *ret_data = NULL;
  *ret_size = 0;
  return -1;
}

/*
 * return -1 on error, 0 if all is fine
 */
int close_link(socket_link_t *link)
{
  close(link->socket_fd);
  memset(link, 0, sizeof(socket_link_t));
  free(link);
  return 0;
}

#ifdef SERVER_TEST

#include <inttypes.h>

int main(void)
{
  void *data;
  int size;
  socket_link_t *l = new_link_server(2210);
  if (l == NULL) { printf("no link created\n"); return 1; }
  printf("link is up\n");
  printf("server starts sleeping...\n");
  /* this sleep is here to test for broken pipe. You can run "nc localhost 2210"
   * and interrupt it quickly so that the server gets a 'broken' pipe on the
   * following link_send_packet.
   */
  sleep(1);
  printf("... done\n");
  if (link_send_packet(l, "hello\n", 6+1) ||
      link_send_packet(l, "world\n", 6+1)) return 1;
  if (link_receive_packet(l, &data, &size)) return 1; printf("%s", (char *)data); free(data);
  if (link_receive_packet(l, &data, &size)) return 1; printf("%s", (char *)data); free(data);
  printf("stats:\n");
  printf("    sent packets %"PRIu64"\n", l->packets_sent);
  printf("    sent bytes %"PRIu64"\n", l->bytes_sent);
  printf("    received packets %"PRIu64"\n", l->packets_received);
  printf("    received bytes %"PRIu64"\n", l->bytes_received);
  if (close_link(l)) return 1;
  printf("link is down\n");
  return 0;
}

#endif

#ifdef CLIENT_TEST

#include <inttypes.h>

int main(void)
{
  void *data;
  int size;
  socket_link_t *l = new_link_client("127.0.0.1", 2210);
  if (l == NULL) { printf("no link created\n"); return 1; }
  printf("link is up\n");
  if (link_receive_packet(l, &data, &size)) return 1; printf("%s", (char *)data); free(data);
  if (link_receive_packet(l, &data, &size)) return 1; printf("%s", (char *)data); free(data);
  if (link_send_packet(l, "bye\n", 4+1) ||
      link_send_packet(l, "server\n", 7+1)) return 1;
  printf("stats:\n");
  printf("    sent packets %"PRIu64"\n", l->packets_sent);
  printf("    sent bytes %"PRIu64"\n", l->bytes_sent);
  printf("    received packets %"PRIu64"\n", l->packets_received);
  printf("    received bytes %"PRIu64"\n", l->bytes_received);
  if (close_link(l)) return 1;
  printf("link is down\n");
  return 0;
}

#endif
