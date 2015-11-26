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
#ifndef SCTP_ITTI_MESSAGING_H_
#define SCTP_ITTI_MESSAGING_H_

int sctp_itti_send_new_message_ind(task_id_t task_id, uint32_t assoc_id, uint8_t *buffer,
                                   uint32_t buffer_length, uint16_t stream);

int sctp_itti_send_association_resp(task_id_t task_id, instance_t instance,
                                    int32_t assoc_id,
                                    uint16_t cnx_id, enum sctp_state_e state,
                                    uint16_t out_streams, uint16_t in_streams);

int sctp_itti_send_association_ind(task_id_t task_id, instance_t instance,
                                   int32_t assoc_id, uint16_t port,
                                   uint16_t out_streams, uint16_t in_streams);


#endif /* SCTP_ITTI_MESSAGING_H_ */
