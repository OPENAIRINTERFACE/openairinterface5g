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

/*
                                play_scenario.h
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
#  include <libxml/xpath.h>
#  include <netinet/in.h>

#include "S1AP-PDU.h"
#include "s1ap_ies_defs.h"
#include "play_scenario_s1ap_eNB_defs.h"
#include "hashtable.h"
#include "asn_compare.h"

#define ET_XPATH_EXPRESSION_MAX_LENGTH 400


// powers of 2
#define ET_BIT_MASK_MATCH_SCTP_STREAM 1
#define ET_BIT_MASK_MATCH_SCTP_SSN    2
//#define ET_BIT_MASK_MATCH_S1AP_    2

#define MAX_ENB 16

#define ENB_CONFIG_STRING_ACTIVE_ENBS                   "Active_eNBs"

#define ENB_CONFIG_STRING_ENB_LIST                      "eNBs"
#define ENB_CONFIG_STRING_ENB_ID                        "eNB_ID"
#define ENB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define ENB_CONFIG_STRING_ENB_NAME                      "eNB_name"

#define ENB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mobile_country_code"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mobile_network_code"


#define ENB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"
#define ENB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"

#define ENB_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define ENB_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define ENB_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"

#define ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME "ENB_INTERFACE_NAME_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME   "ENB_IPV4_ADDRESS_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U    "ENB_INTERFACE_NAME_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U         "ENB_IPV4_ADDRESS_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_S1U              "ENB_PORT_FOR_S1U"


typedef struct shift_packet_s {
  unsigned int           frame_number;      // original frame number
  int                    shift_seconds;
  int                    shift_microseconds;
  int                    single;            // shift timing only for this packet (not also following packets)
  struct shift_packet_s *next;
} shift_packet_t;


typedef struct mme_ip_address_s {
  unsigned  ipv4:1;
  unsigned  ipv6:1;
  unsigned  active:1;
  char     *ipv4_address;
  char     *ipv6_address;
} mme_ip_address_t;

#define IPV4_STR_ADDR_TO_INT_NWBO(AdDr_StR,NwBo,MeSsAgE ) do {\
            struct in_addr inp;\
            if ( inet_aton(AdDr_StR, &inp ) < 0 ) {\
                AssertFatal (0, MeSsAgE);\
            } else {\
                NwBo = inp.s_addr;\
            }\
        } while (0);


typedef struct Enb_properties_s {
  /* Unique eNB_id to identify the eNB within EPC.
   * For macro eNB ids this field should be 20 bits long.
   * For home eNB ids this field should be 28 bits long.
   */
  uint32_t            eNB_id;

  /* The type of the cell */
  enum cell_type_e    cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char               *eNB_name;

  /* Tracking area code */
  uint16_t            tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t            mcc;
  uint16_t            mnc;
  uint8_t             mnc_digit_length;



  /* Physical parameters */

  int16_t                 Nid_cell[1+MAX_NUM_CCs];// for testing, change later
  /* Nb of MME to connect to */
  uint8_t             nb_mme;
  /* List of MME to connect to */
  mme_ip_address_t    mme_ip_address[S1AP_MAX_NB_MME_IP_ADDRESS];

  int                 sctp_in_streams;
  int                 sctp_out_streams;

  char               *enb_interface_name_for_S1U;
  in_addr_t           enb_ipv4_address_for_S1U;
  tcp_udp_port_t      enb_port_for_S1U;

  char               *enb_interface_name_for_S1_MME;
  in_addr_t           enb_ipv4_address_for_S1_MME;

} Enb_properties_t;

typedef struct Enb_properties_array_s {
  int                  number;
  Enb_properties_t    *properties[MAX_ENB];
} Enb_properties_array_t;

#  define ET_ENB_REGISTER_RETRY_DELAY 3
#  define ET_FSM_STATE_WAITING_RX_EVENT_DELAY_SEC 15

typedef enum {
  ET_PACKET_STATUS_START = 0,
  ET_PACKET_STATUS_NONE = ET_PACKET_STATUS_START,
  ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT,
  ET_PACKET_STATUS_SCHEDULED_FOR_SENDING,
  ET_PACKET_STATUS_SENT,
  ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING,
  ET_PACKET_STATUS_RECEIVED,
  ET_PACKET_STATUS_END
} et_packet_status_t;

typedef enum {
  ET_FSM_STATE_START = 0,
  ET_FSM_STATE_NULL = ET_FSM_STATE_START,
  ET_FSM_STATE_CONNECTING_S1C,
  ET_FSM_STATE_WAITING_RX_EVENT,
  ET_FSM_STATE_WAITING_TX_EVENT,
  ET_FSM_STATE_RUNNING,
  ET_FSM_STATE_END
} et_fsm_state_t;

enum COMPARE_ERR_CODE_e;

typedef enum {
  ET_ERROR_MATCH_START = COMPARE_ERR_CODE_END,
  ET_ERROR_MATCH_PACKET_SCTP_CHUNK_TYPE = ET_ERROR_MATCH_START,
  ET_ERROR_MATCH_PACKET_SCTP_PPID,
  ET_ERROR_MATCH_PACKET_SCTP_ASSOC_ID,
  ET_ERROR_MATCH_PACKET_SCTP_STREAM_ID,
  ET_ERROR_MATCH_PACKET_SCTP_SSN,
  ET_ERROR_MATCH_PACKET_S1AP_PRESENT,
  ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE,
  ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY,
  ET_ERROR_MATCH_END
} et_error_match_code_t;




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
  uint16_t             xml_stream_pos_offset;
  uint16_t             binary_stream_pos;
  uint16_t             binary_stream_allocated_size;
  uint8_t             *binary_stream;
  s1ap_message         message; // decoded message: information elements
  xmlDocPtr            doc; // XML representation of the S1AP PDU
} et_s1ap_t;


// from kernel source file 3.19/include/linux/sctp.h, Big Endians
typedef struct sctp_datahdr_s {
  uint32_t    tsn;
  uint16_t    stream;
  uint16_t    ssn;
  uint32_t    ppid;
  uint32_t    assoc_id; // filled when running scenario
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
  char str[INET6_ADDRSTRLEN+1];
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
  instance_t           enb_instance;
  et_ip_hdr_t          ip_hdr;
  et_sctp_hdr_t        sctp_hdr;
  struct et_packet_s  *next;

  //scenario running vars
  et_packet_status_t   status;
  long                 timer_id;         // ITTI timer id for waiting rx packets
  struct timeval       timestamp_packet; // timestamp when rx or tx packet
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
  xmlChar                *name;
  et_packet_t            *list_packet;
  //--------------------------
  // playing scenario
  //--------------------------
  Enb_properties_array_t *enb_properties;
  uint32_t                register_enb_pending;
  uint32_t                registered_enb;
  long                    enb_register_retry_timer_id;


  hash_table_t           *hash_mme2association_id;
  hash_table_t           *hash_old_ue_mme_id2ue_mme_id;
  struct timeval          time_last_tx_packet;    // Time last sent packet
  struct timeval          time_last_rx_packet;    // Time last packet received with all previous scenario RX packet received.
  et_packet_t            *last_rx_packet;         // Last packet received with all previous scenario RX packet received.
  et_packet_t            *last_tx_packet;         // Last sent packet
  et_packet_t            *next_packet;            // Next packet to be handled in the scenario (RX or TX packet)

  int                     timer_count;
} et_scenario_t;


typedef enum {
  ET_EVENT_START = 0,
  ET_EVENT_INIT = ET_EVENT_START,
  ET_EVENT_S1C_CONNECTED,
  ET_EVENT_RX_SCTP_EVENT,
  ET_EVENT_RX_S1AP,
  ET_EVENT_RX_PACKET_TIME_OUT,
  ET_EVENT_TX_TIMED_PACKET,
  ET_EVENT_TICK,
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
    et_packet_t               *tx_timed_packet;
    et_packet_t               *rx_packet_time_out;
  } u;
} et_event_t;

inline void et_free_pointer(void *p) {if (NULL != p) {free(p);}};

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
int et_s1ap_eNB_compare_assoc_id( struct s1ap_eNB_mme_data_s *p1, struct s1ap_eNB_mme_data_s *p2);
uint32_t et_s1ap_generate_eNB_id(void);
uint16_t et_s1ap_eNB_fetch_add_global_cnx_id(void);
void et_s1ap_eNB_prepare_internal_data(void);
void et_s1ap_eNB_insert_new_instance(s1ap_eNB_instance_t *new_instance_p);
struct s1ap_eNB_mme_data_s *et_s1ap_eNB_get_MME(s1ap_eNB_instance_t *instance_p,int32_t assoc_id, uint16_t cnx_id);
s1ap_eNB_instance_t *et_s1ap_eNB_get_instance(instance_t instance);
void et_s1ap_eNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,uint32_t buffer_length, uint16_t stream);
int et_handle_s1ap_mismatch_mme_ue_s1ap_id(et_packet_t * const spacket, et_packet_t * const rx_packet);
asn_comp_rval_t* et_s1ap_is_matching(et_s1ap_t * const s1ap1, et_s1ap_t * const s1ap2, const uint32_t constraints);
et_packet_t* et_build_packet_from_s1ap_data_ind(et_event_s1ap_data_ind_t * const s1ap_data_ind);
int et_scenario_set_packet_received(et_packet_t * const packet);
int  et_s1ap_process_rx_packet(et_event_s1ap_data_ind_t * const sctp_data_ind);
void et_s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t * const sctp_data_ind);
void et_s1ap_eNB_register_mme(s1ap_eNB_instance_t *instance_p,
                                  net_ip_address_t    *mme_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams);
void et_s1ap_handle_s1_setup_message(s1ap_eNB_mme_data_t *mme_desc_p, int sctp_shutdown);
void et_s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB);
void et_s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);
void * et_s1ap_eNB_task(void *arg);
//-------------------------
int et_generate_xml_scenario(
    const char const * xml_in_dir_name,
    const char const * xml_in_scenario_filename,
    const char const * enb_config_filename,
          char const * tsml_out_scenario_filename);
//-------------------------
void timeval_add (struct timeval * const result, const struct timeval * const a, const struct timeval * const b);
int timeval_subtract (struct timeval * const result, struct timeval * const a, struct timeval * const b);
void et_scenario_wait_rx_packet(et_packet_t * const packet);
void et_scenario_schedule_tx_packet(et_packet_t * packet);
et_fsm_state_t et_scenario_fsm_notify_event_state_running(et_event_t event);
et_fsm_state_t et_scenario_fsm_notify_event_state_waiting_tx(et_event_t event);
et_fsm_state_t et_scenario_fsm_notify_event_state_waiting_rx(et_event_t event);
et_fsm_state_t et_scenario_fsm_notify_event_state_connecting_s1c(et_event_t event);
et_fsm_state_t et_scenario_fsm_notify_event_state_null(et_event_t event);
et_fsm_state_t et_scenario_fsm_notify_event(et_event_t event);
//-------------------------
void et_parse_s1ap(xmlDocPtr doc, const xmlNode const *s1ap_node, et_s1ap_t * const s1ap);
void et_parse_sctp_data_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_datahdr_t * const sctp_hdr);
void et_parse_sctp_init_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_inithdr_t * const sctp_hdr);
void et_parse_sctp_init_ack_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_initackhdr_t * const sctp_hdr);
void et_parse_sctp(xmlDocPtr doc, const xmlNode const *sctp_node, et_sctp_hdr_t * const sctp_hdr);
et_packet_t* et_parse_xml_packet(xmlDocPtr doc, xmlNodePtr node);
et_scenario_t* et_generate_scenario(const char  * const et_scenario_filename);
//-------------------------
asn_comp_rval_t * et_s1ap_ies_is_matching(const S1AP_PDU_PR present, s1ap_message * const m1, s1ap_message * const m2, const uint32_t constraints);
void update_xpath_node_mme_ue_s1ap_id(et_s1ap_t * const s1ap, xmlNode *node, const S1ap_MME_UE_S1AP_ID_t new_id);
void update_xpath_nodes_mme_ue_s1ap_id(et_s1ap_t * const s1ap_payload, xmlNodeSetPtr nodes, const S1ap_MME_UE_S1AP_ID_t new_id);
int et_s1ap_update_mme_ue_s1ap_id(et_packet_t * const packet, const S1ap_MME_UE_S1AP_ID_t old_id, const S1ap_MME_UE_S1AP_ID_t new_id);
//-------------------------
asn_comp_rval_t * et_sctp_data_is_matching(sctp_datahdr_t * const sctp1, sctp_datahdr_t * const sctp2, const uint32_t constraints);
asn_comp_rval_t * et_sctp_is_matching(et_sctp_hdr_t * const sctp1, et_sctp_hdr_t * const sctp2, const uint32_t constraints);
//------------------------------------------------------------------------------
void et_print_hex_octets(const unsigned char * const byte_stream, const unsigned long int num);
int  et_is_file_exists ( const char const * file_nameP, const char const *file_roleP);
int  et_strip_extension( char *in_filename);
void et_get_shift_arg( char * line_argument, shift_packet_t * const shift);
int  et_split_path     ( char * pathP, char *** resP);
void et_display_node   ( xmlNodePtr node, unsigned int indent);
void et_display_tree   ( xmlNodePtr node, unsigned int indent);
char * et_ip2ip_str(const et_ip_t * const ip);
int et_compare_et_ip_to_net_ip_address(const et_ip_t * const ip, const net_ip_address_t * const net_ip);
int et_hex2data(unsigned char * const data, const unsigned char * const hexstring, const unsigned int len);
sctp_cid_t et_chunk_type_str2cid(const xmlChar * const chunk_type_str);
const char * const et_chunk_type_cid2str(const sctp_cid_t chunk_type);
const char * const et_error_match2str(const int err);
et_packet_action_t et_action_str2et_action_t(const xmlChar * const action);
void et_ip_str2et_ip(const xmlChar  * const ip_str, et_ip_t * const ip);
void et_enb_config_init(const  char const * lib_config_file_name_pP);
const Enb_properties_array_t *et_enb_config_get(void);
void et_eNB_app_register(const Enb_properties_array_t *enb_properties);
void *et_eNB_app_task(void *args_p);
int et_play_scenario(et_scenario_t* const scenario, const struct shift_packet_s *shifts);

#endif /* PLAY_SCENARIO_H_ */
