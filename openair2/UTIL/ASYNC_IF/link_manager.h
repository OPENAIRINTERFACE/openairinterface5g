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
/*! \file link_manager.h
 * \brief this is the implementation of a link manager
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */

#ifndef LINK_MANAGER_H
#define LINK_MANAGER_H

#include "message_queue.h"
#include "socket_link.h"

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  message_queue_t *send_queue;
  message_queue_t *receive_queue;
  socket_link_t   *socket_link;
  pthread_t       sender;
  pthread_t       receiver;
  volatile int    run;
} link_manager_t;

link_manager_t *create_link_manager(
        message_queue_t *send_queue,
        message_queue_t *receive_queue,
        socket_link_t   *link);
void destroy_link_manager(link_manager_t *);

#ifdef __cplusplus
}
#endif

#endif /* LINK_MANAGER_H */
