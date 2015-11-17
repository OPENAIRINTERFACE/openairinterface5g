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
                                et_scenario.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
*/

#ifndef PLAY_SCENARIO_H_
#define PLAY_SCENARIO_H_
#  include <time.h>
#  include <stdint.h>
#  include <libxml/tree.h>
#  include <netinet/in.h>

#include "enb_config.h"
#include "s1ap_ies_defs.h"

#   define ET_ENB_REGISTER_RETRY_DELAY 3


typedef enum {
  ET_PACKET_STATUS_START = 0,
  ET_PACKET_STATUS_NONE = ET_PACKET_STATUS_START,
  ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT,
  ET_PACKET_STATUS_SCHEDULED_FOR_SENDING,
  ET_PACKET_STATUS_SENT,
  ET_PACKET_STATUS_SENT_WITH_ERRORS,
  ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING,
  ET_PACKET_STATUS_RECEIVED,
  ET_PACKET_STATUS_RECEIVED_WITH_ERRORS,
  ET_PACKET_STATUS_END
} et_packet_status_t;

typedef enum {
  ET_FSM_STATE_START = 0,
  ET_FSM_STATE_NULL = ET_FSM_STATE_START,
  ET_FSM_STATE_CONNECTING_SCTP,
  ET_FSM_STATE_WAITING_TX_EVENT,
  ET_FSM_STATE_WAITING_RX_EVENT,
  ET_FSM_STATE_END
} et_fsm_state_t;




typedef enum {
  ET_PACKET_ACTION_S1C_START = 0,
  ET_PACKET_ACTION_S1C_NULL  = ET_PACKET_ACTION_S1C_START,
  ET_PACKET_ACTION_S1C_SEND,
  ET_PACKET_ACTION_S1C_RECEIVE,
  ET_PACKET_ACTION_S1C_END
} et_packet_action_t;

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

typedef enum {
  TEST_S1AP_PDU_TYPE_START = 0,
  TEST_S1AP_PDU_TYPE_UNKNOWN = TEST_S1AP_PDU_TYPE_START,
  TEST_S1AP_PDU_TYPE_INITIATING,
  TEST_S1AP_PDU_TYPE_SUCCESSFUL_OUTCOME,
  TEST_S1AP_PDU_TYPE_UNSUCCESSFUL_OUTCOME,
  TEST_S1AP_PDU_TYPE_END
} et_s1ap_pdu_type_t;


typedef struct et_s1ap_s {
  //et_s1ap_pdu_type_t pdu_type;
  S1AP_PDU_t           pdu; // decoded ASN1 C type: choice of initiatingMessage, successfulOutcome, unsuccessfulOutcome
  uint16_t             binary_stream_pos;
  uint16_t             binary_stream_allocated_size;
  uint8_t             *binary_stream;
  s1ap_message         message; // decoded message: information elements
} et_s1ap_t;


// from kernel source file 3.19/include/linux/sctp.h, Big Endians
typedef struct sctp_datahdr_s {
  uint32_t    tsn;
  uint16_t    stream;
  uint16_t    ssn;
  uint32_t    ppid;
  et_s1ap_t   payload;
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

typedef struct et_sctp_hdr_s {
  unsigned int src_port;
  unsigned int dst_port;
  sctp_cid_t   chunk_type;
  union {
    sctp_datahdr_t    data_hdr;
    sctp_inithdr_t    init_hdr;
    sctp_initackhdr_t init_ack_hdr;
  } u;
} et_sctp_hdr_t;

typedef struct et_ip_s {
  unsigned int  address_family; // AF_INET, AF_INET6
  union {
    struct in6_addr  ipv6;
    in_addr_t        ipv4;
  }address;
}et_ip_t;

typedef struct et_ip_hdr_s {
  et_ip_t  src;
  et_ip_t  dst;
} et_ip_hdr_t;

typedef struct et_packet_s {
  et_packet_action_t   action;
  struct timeval       time_relative_to_first_packet;
  struct timeval       time_relative_to_last_sent_packet;
  struct timeval       time_relative_to_last_received_packet;
  unsigned int         original_frame_number;
  unsigned int         packet_number;
  et_ip_hdr_t          ip_hdr;
  et_sctp_hdr_t        sctp_hdr;
  struct et_packet_s  *next;
  //scenario running vars
  et_packet_status_t   status;
}et_packet_t;


typedef struct sctp_epoll_s {
  /* Array of events monitored by the task.
   * By default only one fd is monitored (the one used to received messages
   * from other tasks).
   * More events can be suscribed later by the task itself.
   */
  struct epoll_event *events;

  int epoll_nb_events;

} thread_desc_t;

typedef struct et_scenario_s {
  xmlChar        *name;
  et_packet_t   *list_packet;

  // playing scenario
  et_packet_t   *waited_packet;
  et_packet_t   *current_packet;
} et_scenario_t;


typedef enum {
  ET_EVENT_START = 0,
  ET_EVENT_INIT = ET_EVENT_START,
  ET_EVENT_RX_SCTP_EVENT,
  ET_EVENT_RX_S1AP,
  ET_EVENT_RX_X2AP,
  ET_EVENT_RX_PACKET_TIME_OUT,
  ET_EVENT_TX_PACKET,
  ET_EVENT_STOP,
  ET_EVENT_END
} et_event_code_t;

typedef struct et_event_init_s {
  et_scenario_t *scenario;
} et_event_init_t;

typedef struct et_event_s1ap_data_ind_s {
  sctp_datahdr_t sctp_datahdr;
} et_event_s1ap_data_ind_t;

typedef struct et_event_s1ap_data_req_s {

} et_event_s1ap_data_req_t;

typedef struct et_event_s {
  et_event_code_t code;
  union {
    et_event_init_t           init;
    et_event_s1ap_data_ind_t  s1ap_data_ind;
    et_event_s1ap_data_req_t  s1ap_data_req;
  } u;
} et_event_t;

inline void et_free_pointer(void *p) {if (NULL != p) {free(p); p=NULL;}};

//-------------------------
void et_free_packet(et_packet_t* packet);
void et_free_scenario(et_scenario_t* scenario);
//-------------------------
void et_display_packet_s1ap_data(const et_s1ap_t * const s1ap);
void et_display_packet_sctp_init(const sctp_inithdr_t * const sctp);
void et_display_packet_sctp_initack(const sctp_initackhdr_t * const sctp);
void et_display_packet_sctp_data(const sctp_datahdr_t * const sctp);
void et_display_packet_sctp(const et_sctp_hdr_t * const sctp);
void et_display_packet_ip(const et_ip_hdr_t * const ip);
void et_display_packet(const et_packet_t * const packet);
void et_display_scenario(const et_scenario_t * const scenario);
//-------------------------
int et_s1ap_decode_initiating_message(s1ap_message *message, S1ap_InitiatingMessage_t *initiating_p);
int et_s1ap_decode_successful_outcome(s1ap_message *message, S1ap_SuccessfulOutcome_t *successfullOutcome_p);
int et_s1ap_decode_unsuccessful_outcome(s1ap_message *message, S1ap_UnsuccessfulOutcome_t *unSuccessfullOutcome_p);
int et_s1ap_decode_pdu(S1AP_PDU_t * const pdu, s1ap_message * const message, const uint8_t * const buffer, const uint32_t length);
void et_decode_s1ap(et_s1ap_t * const s1ap);
//-------------------------
void et_s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind);
void * et_s1ap_eNB_task(void *arg);
int et_generate_xml_scenario(
    const char const * xml_in_dir_name,
    const char const * xml_in_scenario_filename,
    const char const * enb_config_filename,
          char const * tsml_out_scenario_filename);
//-------------------------
int et_scenario_fsm_notify_event_state_null(et_event_t event);
int et_scenario_fsm_notify_event(et_event_t event);
//-------------------------
void et_parse_s1ap(xmlDocPtr doc, const xmlNode const *s1ap_node, et_s1ap_t * const s1ap);
void et_parse_sctp_data_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_datahdr_t * const sctp_hdr);
void et_parse_sctp_init_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_inithdr_t * const sctp_hdr);
void et_parse_sctp_init_ack_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_initackhdr_t * const sctp_hdr);
void et_parse_sctp(xmlDocPtr doc, const xmlNode const *sctp_node, et_sctp_hdr_t * const sctp_hdr);
et_packet_t* et_parse_xml_packet(xmlDocPtr doc, xmlNodePtr node);
et_scenario_t* et_generate_scenario(const char  * const et_scenario_filename );
//-------------------------
void et_print_hex_octets(const unsigned char * const byte_stream, const unsigned long int num);
int  et_is_file_exists ( const char const * file_nameP, const char const *file_roleP);
int  et_strip_extension( char *in_filename);
int  et_split_path     ( char * pathP, char *** resP);
void et_display_node   ( xmlNodePtr node, unsigned int indent);
void et_display_tree   ( xmlNodePtr node, unsigned int indent);
char * et_ip2ip_str(const et_ip_t * const ip);
int et_hex2data(unsigned char * const data, const unsigned char * const hexstring, const unsigned int len);
sctp_cid_t et_chunk_type_str2cid(const xmlChar * const chunk_type_str);
const char * const et_chunk_type_cid2str(const sctp_cid_t chunk_type);
et_packet_action_t et_action_str2et_action_t(const xmlChar * const action);
void et_ip_str2et_ip(const xmlChar  * const ip_str, et_ip_t * const ip);
uint32_t et_eNB_app_register(const Enb_properties_array_t *enb_properties);
void *et_eNB_app_task(void *args_p);
int et_play_scenario(et_scenario_t* const scenario);

#endif /* PLAY_SCENARIO_H_ */
