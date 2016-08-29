/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2015 Eurecom

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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
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
  uint64_t bytes_sent;
  uint64_t packets_sent;
  uint64_t bytes_received;
  uint64_t packets_received;
} socket_link_t;

socket_link_t *new_link_server(int port);
socket_link_t *new_link_client(char *server, int port);
int link_send_packet(socket_link_t *link, void *data, int size);
int link_receive_packet(socket_link_t *link, void **data, int *size);
int close_link(socket_link_t *link);

#ifdef __cplusplus
}
#endif

#endif /* SOCKET_LINK_H */
