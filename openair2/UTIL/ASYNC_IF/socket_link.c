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

  LOG_D(MAC, "create a new link server socket at port %d\n", port);

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

  close(socket_server);

  LOG_D(MAC, "connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
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
int link_send_packet(socket_link_t *link, void *data, int size)
{
  char sizebuf[4];
  int32_t s = size;

  /* send the size first, maximum is 2^31 bytes */
  sizebuf[0] = (s >> 24) & 255;
  sizebuf[1] = (s >> 16) & 255;
  sizebuf[2] = (s >> 8) & 255;
  sizebuf[3] = s & 255;

  if (socket_send(link->socket_fd, sizebuf, 4) == -1)
    goto error;

  link->bytes_sent += 4;

  if (socket_send(link->socket_fd, data, size) == -1)
    goto error;

  link->bytes_sent += size;
  link->packets_sent++;

  return 0;

error:
  return -1;
}

/*
 * return -1 on error and 0 if the sending was fine
 */
int link_receive_packet(socket_link_t *link, void **ret_data, int *ret_size)
{
  unsigned char sizebuf[4];
  int32_t       size;
  void          *data = NULL;

  /* received the size first, maximum is 2^31 bytes */
  if (socket_receive(link->socket_fd, sizebuf, 4) == -1)
    goto error;

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

  if (socket_receive(link->socket_fd, data, size) == -1)
    goto error;

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
