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

/*
                                generate_scenario.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
*/

#ifndef GENERATE_SCENARIO_H_
#define GENERATE_SCENARIO_H_
# include <time.h>
# include <stdint.h>
#include <libxml/tree.h>

/** @defgroup _enb_app ENB APP 
 * @ingroup _oai2
 * @{
 */

typedef enum {
  ACTION_S1C_START = 0,
  ACTION_S1C_NULL  = ACTION_S1C_START,
  ACTION_S1C_SEND,
  ACTION_S1C_RECEIVE,
  ACTION_S1C_END
} test_action_t;

// from kernel source file 3.19/include/linux/sctp.h
typedef enum {
  SCTP_CID_DATA                   = 0,
  SCTP_CID_INIT                   = 1,
  SCTP_CID_INIT_ACK               = 2,
  SCTP_CID_SACK                   = 3,
  SCTP_CID_HEARTBEAT              = 4,
  SCTP_CID_HEARTBEAT_ACK          = 5,
  SCTP_CID_ABORT                  = 6,
  SCTP_CID_SHUTDOWN               = 7,
  SCTP_CID_SHUTDOWN_ACK           = 8,
  SCTP_CID_ERROR                  = 9,
  SCTP_CID_COOKIE_ECHO            = 10,
  SCTP_CID_COOKIE_ACK             = 11,
  SCTP_CID_ECN_ECNE               = 12,
  SCTP_CID_ECN_CWR                = 13,
  SCTP_CID_SHUTDOWN_COMPLETE      = 14,

  /* AUTH Extension Section 4.1 */
  SCTP_CID_AUTH                   = 0x0F,

  /* PR-SCTP Sec 3.2 */
  SCTP_CID_FWD_TSN                = 0xC0,

  /* Use hex, as defined in ADDIP sec. 3.1 */
  SCTP_CID_ASCONF                 = 0xC1,
  SCTP_CID_ASCONF_ACK             = 0x80,
} sctp_cid_t; /* enum */

// from kernel source file 3.19/include/linux/sctp.h, Big Endians
typedef struct sctp_datahdr_s {
  uint32_t tsn;
  uint16_t stream;
  uint16_t ssn;
  uint32_t ppid;
  uint8_t  payload[0];
} sctp_datahdr_t;

// from kernel source file 3.19/include/linux/sctp.h, Big Endians
typedef struct sctp_inithdr {
  uint32_t init_tag;
  uint32_t a_rwnd;
  uint16_t num_outbound_streams;
  uint16_t num_inbound_streams;
  uint32_t initial_tsn;
  uint8_t  params[0];
}  sctp_inithdr_t;

typedef sctp_inithdr_t sctp_initackhdr_t;

typedef struct test_sctp_hdr_s {
  unsigned int src_port;
  unsigned int dst_port;
  sctp_cid_t   chunk_type;
  union {
    sctp_datahdr_t    data_hdr;
    sctp_inithdr_t    init_hdr;
    sctp_initackhdr_t init_ack_hdr;
  } u;
} test_sctp_hdr_t;

typedef struct test_packet_s {
  test_action_t   action;
  struct timeval  time_relative_to_first_packet;
  struct timeval  time_relative_to_last_packet;
  test_sctp_hdr_t sctp_hdr;
  uint16_t        s1ap_byte_stream_count;
  uint8_t        *s1ap_byte_stream;
  xmlNodePtr     *s1ap_node;

  struct test_packet_s *next;
}test_packet_t;

typedef struct test_scenario_s {
  xmlChar        *name;
  test_packet_t  *list_packet;
}test_scenario_t;

inline void free_pointer(void *p) {if (NULL != p) {free(p); p=NULL;}};
#endif /* ENB_CONFIG_H_ */
/** @} */
