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
                                play_scenario_display.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */
#include <string.h>
#include <stdio.h>

#include "intertask_interface.h"
#include "platform_types.h"
#include "assertions.h"
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_decoder.h"
#include "play_scenario.h"
//-----------------------------------------------------------------------------
void et_print_hex_octets(const unsigned char * const byte_stream, const unsigned long int num)
{
  unsigned long octet_index = 0;

  if (byte_stream == NULL) {
    return;
  }

  fprintf(stdout, "+-----+-------------------------------------------------+\n");
  fprintf(stdout, "|     |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
  fprintf(stdout, "+-----+-------------------------------------------------+\n");

  for (octet_index = 0; octet_index < num; octet_index++) {
    if ((octet_index % 16) == 0) {
      if (octet_index != 0) {
        fprintf(stdout, " |\n");
      }

      fprintf(stdout, " %04ld |", octet_index);
    }

    /*
     * Print every single octet in hexadecimal form
     */
    fprintf(stdout, " %02x", byte_stream[octet_index]);
    /*
     * Align newline and pipes according to the octets in groups of 2
     */
  }

  /*
   * Append enough spaces and put final pipe
   */
  unsigned char index;

  for (index = octet_index; index < 16; ++index) {
    fprintf(stdout, "   ");
  }

  fprintf(stdout, " |\n");
}

//------------------------------------------------------------------------------
void et_display_node(xmlNodePtr node, unsigned int indent)
{
  int i = 0;
  if (node->type == XML_ELEMENT_NODE) {
    xmlChar *path = xmlGetNodePath(node);
    for (i=0; i<indent; i++) {
      printf("  ");
    }
    if (node->children != NULL && node->children->type == XML_TEXT_NODE) {
      xmlChar *content = xmlNodeGetContent(node);
      printf("%s -> %s\n", path, content);
      xmlFree(content);
    } else {
      printf("%s\n", path);
    }
    xmlFree(path);
  }
}
//------------------------------------------------------------------------------
void et_display_tree(xmlNodePtr node, unsigned int indent)
{
  xmlNode *cur_node = NULL;

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      et_display_node(cur_node, indent);
    }
    et_display_tree(cur_node->children, indent++);
  }
}
//------------------------------------------------------------------------------
void et_display_packet_s1ap_data(const et_s1ap_t * const s1ap)
{
  char       *message_string = NULL;

  if (s1ap) {
    message_string = calloc(20000, sizeof(char));
    AssertFatal (NULL != message_string, "ERROR malloc()failed!\n");
    s1ap_string_total_size = 0;
    switch(s1ap->pdu.present) {
    case S1AP_PDU_PR_initiatingMessage:
      switch(s1ap->pdu.choice.initiatingMessage.procedureCode) {
      case S1ap_ProcedureCode_id_downlinkNASTransport:      s1ap_xer_print_s1ap_downlinknastransport(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_InitialContextSetup:       s1ap_xer_print_s1ap_initialcontextsetuprequest(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_UEContextRelease:          s1ap_xer_print_s1ap_uecontextreleasecommand(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_Paging:                    s1ap_xer_print_s1ap_paging(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_uplinkNASTransport:        s1ap_xer_print_s1ap_uplinknastransport(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_S1Setup:                   s1ap_xer_print_s1ap_s1setuprequest(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_initialUEMessage:          s1ap_xer_print_s1ap_initialuemessage(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_UEContextReleaseRequest:   s1ap_xer_print_s1ap_uecontextreleaserequest(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_UECapabilityInfoIndication:s1ap_xer_print_s1ap_uecapabilityinfoindication(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_NASNonDeliveryIndication:  s1ap_xer_print_s1ap_nasnondeliveryindication_(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      default:
        AssertFatal( 0 , "Unknown procedure ID (%d) for initiating message\n",
                     (int)s1ap->pdu.choice.initiatingMessage.procedureCode);
      }
      break;
    case S1AP_PDU_PR_successfulOutcome:
      switch(s1ap->pdu.choice.successfulOutcome.procedureCode) {
      case S1ap_ProcedureCode_id_S1Setup:                   s1ap_xer_print_s1ap_s1setupresponse(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_InitialContextSetup:       s1ap_xer_print_s1ap_initialcontextsetupresponse(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_UEContextRelease:          s1ap_xer_print_s1ap_uecontextreleasecomplete(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      default:
        AssertFatal(0, "Unknown procedure ID (%d) for successfull outcome message\n",
                   (int)s1ap->pdu.choice.successfulOutcome.procedureCode);
      }
      break;

    case S1AP_PDU_PR_unsuccessfulOutcome:
      switch(s1ap->pdu.choice.unsuccessfulOutcome.procedureCode) {
      case S1ap_ProcedureCode_id_S1Setup:                   s1ap_xer_print_s1ap_s1setupfailure(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      case S1ap_ProcedureCode_id_InitialContextSetup:       s1ap_xer_print_s1ap_initialcontextsetupfailure(s1ap_xer__print2sp, message_string, (s1ap_message *)&s1ap->message);break;
      default:
        et_free_pointer(message_string);
        AssertFatal(0,"Unknown procedure ID (%d) for unsuccessfull outcome message\n",
                   (int)s1ap->pdu.choice.unsuccessfulOutcome.procedureCode);
        break;
      }
      break;
    default:
      AssertFatal(0, "Unknown presence (%d) or not implemented\n", (int)s1ap->pdu.present);
      break;
    }
    fprintf(stdout, "\t\tSCTP.data XML dump:\n%s\n", message_string);
    et_free_pointer(message_string);
  }
}
//------------------------------------------------------------------------------
void et_display_packet_sctp_init(const sctp_inithdr_t * const sctp)
{

  if (sctp) {
    fprintf(stdout, "\t\tSCTP.init.init_tag               : %u\n", sctp->init_tag);
    fprintf(stdout, "\t\tSCTP.init.a_rwnd                 : %u\n", sctp->a_rwnd);
    fprintf(stdout, "\t\tSCTP.init.num_inbound_streams    : %u\n", sctp->num_inbound_streams);
    fprintf(stdout, "\t\tSCTP.init.num_outbound_streams   : %u\n", sctp->num_outbound_streams);
    fprintf(stdout, "\t\tSCTP.init.initial_tsn            : %u\n", sctp->initial_tsn);
  }
}
//------------------------------------------------------------------------------
void et_display_packet_sctp_initack(const sctp_initackhdr_t * const sctp)
{

  if (sctp) {
    fprintf(stdout, "\t\tSCTP.initack.init_tag               : %u\n", sctp->init_tag);
    fprintf(stdout, "\t\tSCTP.initack.a_rwnd                 : %u\n", sctp->a_rwnd);
    fprintf(stdout, "\t\tSCTP.initack.num_inbound_streams    : %u\n", sctp->num_inbound_streams);
    fprintf(stdout, "\t\tSCTP.initack.num_outbound_streams   : %u\n", sctp->num_outbound_streams);
    fprintf(stdout, "\t\tSCTP.initack.initial_tsn            : %u\n", sctp->initial_tsn);
  }
}
//------------------------------------------------------------------------------
void et_display_packet_sctp_data(const sctp_datahdr_t * const sctp)
{
  if (sctp) {
    fprintf(stdout, "\t\tSCTP.data.tsn                 : %u\n", sctp->tsn);
    fprintf(stdout, "\t\tSCTP.data.stream              : %u\n", sctp->stream);
    fprintf(stdout, "\t\tSCTP.data.ssn                 : %u\n", sctp->ssn);
    fprintf(stdout, "\t\tSCTP.data.ppid                : %u\n", sctp->ppid);
    if (sctp->ppid == 18) {
      et_display_packet_s1ap_data(&sctp->payload);
    }
    fprintf(stdout, "\t\tSCTP.data.binary_stream_allocated_size : %u\n", sctp->payload.binary_stream_allocated_size);
    if (NULL != sctp->payload.binary_stream) {
      fprintf(stdout, "\t\tSCTP.data.binary_stream       :\n");
      et_print_hex_octets(sctp->payload.binary_stream, sctp->payload.binary_stream_allocated_size);
    } else {
      fprintf(stdout, "\t\tSCTP.data.binary_stream       : NULL\n");
    }
  }
}

//------------------------------------------------------------------------------
void et_display_packet_sctp(const et_sctp_hdr_t * const sctp)
{
  if (sctp) {
    fprintf(stdout, "\t\tSCTP.src_port      : %u\n", sctp->src_port);
    fprintf(stdout, "\t\tSCTP.dst_port      : %u\n", sctp->dst_port);
    fprintf(stdout, "\t\tSCTP.chunk_type    : %s\n", et_chunk_type_cid2str(sctp->chunk_type));
    switch (sctp->chunk_type) {
      case SCTP_CID_DATA:
        et_display_packet_sctp_data(&sctp->u.data_hdr);
        break;
      case SCTP_CID_INIT:
        et_display_packet_sctp_initack(&sctp->u.init_hdr);
        break;
      case SCTP_CID_INIT_ACK:
        et_display_packet_sctp_initack(&sctp->u.init_ack_hdr);
        break;
      default:
        ;
    }
  }
}
//------------------------------------------------------------------------------
void et_display_packet_ip(const et_ip_hdr_t * const ip)
{
  if (ip) {
    fprintf(stdout, "\t\tSource address      : %s\n", et_ip2ip_str(&ip->src));
    fprintf(stdout, "\t\tDestination address : %s\n", et_ip2ip_str(&ip->dst));
  }
}

//------------------------------------------------------------------------------
void et_display_packet(const et_packet_t * const packet)
{
  if (packet) {
    fprintf(stdout, "-------------------------------------------------------------------------------\n");
    fprintf(stdout, "\tPacket:\tnum %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
    fprintf(stdout, "\tPacket:\ttime relative to 1st packet           %ld.%06lu\n",
        packet->time_relative_to_first_packet.tv_sec, packet->time_relative_to_first_packet.tv_usec);
    fprintf(stdout, "\tPacket:\ttime relative to last tx packet       %ld.%06lu\n",
        packet->time_relative_to_last_sent_packet.tv_sec, packet->time_relative_to_last_sent_packet.tv_usec);
    fprintf(stdout, "\tPacket:\ttime relative to last_received packet %ld.%06lu\n",
        packet->time_relative_to_last_received_packet.tv_sec, packet->time_relative_to_last_received_packet.tv_usec);

    switch(packet->action) {
    case   ET_PACKET_ACTION_S1C_SEND:
      fprintf(stdout, "\tPacket:\tAction SEND\n");
      break;
    case   ET_PACKET_ACTION_S1C_RECEIVE:
      fprintf(stdout, "\tPacket:\tAction RECEIVE\n");
      break;
    default:
      fprintf(stdout, "\tPacket:\tAction UNKNOWN\n");
    }
    switch(packet->status) {
    case   ET_PACKET_STATUS_NONE:
      fprintf(stdout, "\tPacket:\tStatus NONE\n");
      break;
    case   ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT:
      fprintf(stdout, "\tPacket:\tStatus NOT_TAKEN_IN_ACCOUNT\n");
      break;
    case   ET_PACKET_STATUS_SCHEDULED_FOR_SENDING:
      fprintf(stdout, "\tPacket:\tStatus SCHEDULED_FOR_SENDING\n");
      break;
    case   ET_PACKET_STATUS_SENT:
      fprintf(stdout, "\tPacket:\tStatus SENT\n");
      break;
    case   ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING:
      fprintf(stdout, "\tPacket:\tStatus SCHEDULED_FOR_RECEIVING\n");
      break;
    case   ET_PACKET_STATUS_RECEIVED:
      fprintf(stdout, "\tPacket:\tStatus RECEIVED\n");
      break;
    default:
      fprintf(stdout, "\tPacket:\tStatus UNKNOWN\n");
    }
    et_display_packet_ip(&packet->ip_hdr);
    et_display_packet_sctp(&packet->sctp_hdr);
  }
}
//------------------------------------------------------------------------------
void et_display_scenario(const et_scenario_t * const scenario)
{
  et_packet_t *packet = NULL;
  if (scenario) {
    fprintf(stdout, "Scenario: %s\n", (scenario->name != NULL) ? (char*)scenario->name:"UNKNOWN NAME");
    packet = scenario->list_packet;
    while (packet) {
      et_display_packet(packet);
      packet = packet->next;
    }
  }
}
