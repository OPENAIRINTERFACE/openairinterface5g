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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*! \file udp_eNB_task.h
* \brief
* \author Sebastien ROUX, Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#include <pthread.h>
#include <stdint.h>

#ifndef UDP_ENB_TASK_H_
#define UDP_ENB_TASK_H_
#include "enb_config.h"
#include "intertask_interface_types.h"


/** \brief UDP recv callback prototype. Will be called every time a payload is
 *  received on socket.
 *  \param port Port on which data had been received
 *  \param address Sender Ipv4 address
 *  \param buffer Pointer to data (should be freed by user)
 *  \param length Length of message received
 *  \param arg_p User provided argument (transparent for wrappers)
 *  @returns Execution result
 */
typedef int (*udp_recv_callback)(uint16_t  port,
                                 uint32_t  address,
                                 uint8_t  *buffer,
                                 uint32_t  length,
                                 void     *arg_p);

typedef struct {
  int       sd;              /* Socket descriptor to use */
  pthread_t listener_thread; /* Thread affected to recv */
  char     *local_address;   /* Local ipv4 address to use */
  uint16_t  local_port;      /* Local port to use */
  task_id_t task_id;         /* Task who has requested the new endpoint */
} udp_data_t;

/** \brief Create new datagram connection-less socket and create new thread
 *  for data in downstream.
 *  \param ip_addr Local IPv4 address to use
 *  \param port Local port to use
 *  \param udp_data_p
 *  \param recv_callback Pointer to data (should be freed by user)
 *  \param arg_p Optionnal argument to pass to recv_callback
 *  @returns < 0 on failure or socket descriptor on success
 */
int udp_create_connection(char *ip_addr, uint16_t port,
                          udp_data_t *udp_data_p,
                          udp_recv_callback recv_callback,
                          void *arg_p);

/** \brief Send buffer to remote peer.
 *  \param sd socket descriptor to use
 *  \param port remote port
 *  \param address remote address
 *  \param buffer Data buffer to send
 *  \param length Buffer length
 *  @returns < 0 on failure or socket descriptor on success
 */
int udp_send_to(int sd, uint16_t port, uint32_t address,
                const uint8_t *buffer, uint32_t length);


/** \brief UDP ITTI task on eNB.
 *  \param args_p
 *  @returns always NULL
 */
void *udp_eNB_task(void *args_p);

/** \brief init UDP layer.
 *  \param enb_config_p configuration of eNB
 *  @returns always 0
 */
int udp_enb_init(const Enb_properties_t *enb_config_p);

#endif /* UDP_ENB_TASK_H_ */
