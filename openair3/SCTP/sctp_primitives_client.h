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

#include <stdint.h>

#include "sctp_eNB_defs.h"

#ifndef SCTP_PRIMITIVES_CLIENT_H_
#define SCTP_PRIMITIVES_CLIENT_H_

/** @defgroup _sctp_impl_ SCTP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/** \brief SCTP recv callback prototype. Will be called every time a message is
 * received on socket.
 * \param assocId SCTP association ID
 * \param stream SCTP stream on which data had been received
 * \param buffer Pointer to data (should be freed by user)
 * \param length Length of message received
 * @return Execution result
 */
typedef int (*sctp_recv_callback)(uint32_t  assocId,
                                  uint32_t  stream,
                                  uint8_t  *buffer,
                                  uint32_t  length);

/** \brief SCTP connected callback prototype. Will be called once the
 * association is ready.
 * \param args argument provided by upper layer
 * \param assocId SCTP association ID
 * \param instreams Number of input streams negotiated with remote peer
 * \param outstreams Number of output streams negotiated with remote peer
 * @return Execution result
 */
typedef int (*sctp_connected_callback)(void     *args,
                                       uint32_t  assocId,
                                       uint32_t  instreams,
                                       uint32_t  outstreams);

/** \brief Perform association to a remote peer
 * \param ip_addr Peer IPv4 address
 * \param port Remote port to connect to
 * \param args Upper layer args that will be provided to connected callback
 * \param connected_callback Connected callback
 * \param recv_callback Data received callback
 * @return < 0 in case of failure
 */
int sctp_connect_to_remote_host(char *local_ip_addr[],
                                int nb_local_addr,
                                char *remote_ip_addr,
                                uint16_t port,
                                int socket_type,
                                sctp_data_t *sctp_data_p);

/** \brief Send message over SCTP
 * \param sctp_data_p Pointer to the SCTP desc
 * \param ppid Payload Protocol Identifier
 * \param stream SCTP stream on which data will be sent
 * \param buffer Pointer to buffer
 * \param length Buffer length
 * @return < 0 in case of failure
 */
int sctp_send_msg(sctp_data_t *sctp_data_p, uint16_t ppid, uint16_t stream,
                  const uint8_t *buffer, const uint32_t length);

/** \brief Flush the FIFO of messages from the socket
 * \param sctp_data_p
 * @return < 0 in case of failure
 */
int sctp_run(sctp_data_t *sctp_data_p);

/** \brief Properly disconnect the peer
 * \param assoc_id The SCTP association ID
 * @return < 0 in case of failure
 */
void sctp_disconnect(uint32_t assoc_id);

void sctp_terminate(void);

/* @} */
#endif /* SCTP_PRIMITIVES_CLIENT_H_ */
