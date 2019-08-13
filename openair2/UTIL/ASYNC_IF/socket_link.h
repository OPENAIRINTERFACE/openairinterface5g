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

/*! \file socket_link.h
 * \brief this is the implementation of a TCP socket ASYNC IF
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */

#ifndef SOCKET_LINK_H
#define SOCKET_LINK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  int      socket_fd;
  int      type;
  uint64_t bytes_sent;
  uint64_t packets_sent;
  uint64_t bytes_received;
  uint64_t packets_received;
} socket_link_t;

socket_link_t *new_link_server(int port);
socket_link_t *new_link_client(const char *server, int port);
/* setting bind_addr to NULL binds server to INADDR_ANY */
socket_link_t *new_link_udp_server(const char *bind_addr, int bind_port);
socket_link_t *new_link_udp_client(const char *server, int port);
socket_link_t *new_link_sctp_server(int port);
socket_link_t *new_link_sctp_client(const char *server, int port);
int link_send_packet(socket_link_t *link, void *data, int size, const char *peer_addr, int port);
int link_receive_packet(socket_link_t *link, void **data, int *size);
int close_link(socket_link_t *link);


#ifdef __cplusplus
}
#endif

#endif /* SOCKET_LINK_H */
