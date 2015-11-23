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

#ifndef SCTP_MESSAGES_TYPES_H_
#define SCTP_MESSAGES_TYPES_H_

#define SCTP_NEW_ASSOCIATION_REQ(mSGpTR) (mSGpTR)->ittiMsg.sctp_new_association_req
#define SCTP_NEW_ASSOCIATION_RESP(mSGpTR)(mSGpTR)->ittiMsg.sctp_new_association_resp
#define SCTP_NEW_ASSOCIATION_IND(mSGpTR) (mSGpTR)->ittiMsg.sctp_new_association_ind
#define SCTP_DATA_IND(mSGpTR)            (mSGpTR)->ittiMsg.sctp_data_ind
#define SCTP_DATA_REQ(mSGpTR)            (mSGpTR)->ittiMsg.sctp_data_req
#define SCTP_INIT_MSG(mSGpTR)            (mSGpTR)->ittiMsg.sctp_init
#define SCTP_CLOSE_ASSOCIATION(mSGpTR)   (mSGpTR)->ittiMsg.sctp_close_association

enum sctp_state_e {
  SCTP_STATE_CLOSED,
  SCTP_STATE_SHUTDOWN,
  SCTP_STATE_ESTABLISHED,
  SCTP_STATE_UNREACHABLE
};

typedef struct sctp_new_association_req_s {
  /* Upper layer connexion identifier */
  uint16_t         ulp_cnx_id;

  /* The port to connect to */
  uint16_t         port;
  /* Payload Protocol Identifier to use */
  uint32_t         ppid;

  /* Number of streams used for this association */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Local address to bind to */
  net_ip_address_t local_address;
  /* Remote address to connect to */
  net_ip_address_t remote_address;
} sctp_new_association_req_t;

typedef struct sctp_new_association_ind_s {
  /* Assoc id of the new association */
  int32_t  assoc_id;

  /* The port used by remote host */
  uint16_t port;

  /* Number of streams used for this association */
  uint16_t in_streams;
  uint16_t out_streams;
} sctp_new_association_ind_t;

typedef struct sctp_new_association_resp_s {
  /* Upper layer connexion identifier */
  uint16_t ulp_cnx_id;

  /* SCTP Association ID */
  int32_t  assoc_id;

  /* Input/output streams */
  uint16_t out_streams;
  uint16_t in_streams;

  /* State of the association at SCTP level */
  enum sctp_state_e sctp_state;
} sctp_new_association_resp_t;

typedef struct sctp_data_ind_s {
  /* SCTP Association ID */
  int32_t   assoc_id;

  /* Buffer to send over SCTP */
  uint32_t  buffer_length;
  uint8_t  *buffer;

  /* Streams on which data will be sent/received */
  uint16_t  stream;
} sctp_data_ind_t;

typedef struct sctp_init_s {
  /* Request usage of ipv4 */
  unsigned  ipv4:1;
  /* Request usage of ipv6 */
  unsigned  ipv6:1;
  uint8_t   nb_ipv4_addr;
  uint32_t  ipv4_address[10];
  uint8_t   nb_ipv6_addr;
  char     *ipv6_address[10];
  uint16_t  port;
  uint32_t  ppid;
} sctp_init_t;


typedef struct sctp_close_association_s {
  uint32_t  assoc_id;
} sctp_close_association_t;


typedef sctp_data_ind_t sctp_data_req_t;

typedef struct sctp_listener_register_upper_layer_s {
  /* Port to listen to */
  uint16_t port;
  /* Payload protocol identifier
   * Any data receveid on PPID != will be discarded
   */
  uint32_t ppid;
} sctp_listener_register_upper_layer_t;

#endif /* SCTP_MESSAGES_TYPES_H_ */
