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
                                play_scenario.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */

#include <string.h>
#include <limits.h>
#include <libconfig.h>
#include <inttypes.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/DOCBparser.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxml/xmlreader.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "intertask_interface_init.h"
#include "assertions.h"
#include "play_scenario.h"
//#include "s1ap_eNB.h"
#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_decoder.h"
#include "intertask_interface.h"
#include "enb_config.h"
#include "log.h"
//------------------------------------------------------------------------------
#define ENB_CONFIG_MAX_XSLT_PARAMS 32
#define PLAY_SCENARIO              1
#define GS_IS_FILE                 1
#define GS_IS_DIR                  2
//------------------------------------------------------------------------------
char                  *g_openair_dir        = NULL;
//------------------------------------------------------------------------------
extern Enb_properties_array_t enb_properties;
extern int                    xmlLoadExtDtdDefaultValue;
extern int                    asn_debug;
extern int                    asn1_xer_print;
//------------------------------------------------------------------------------
void test_print_hex_octets(const unsigned char * const byte_stream, const unsigned long int num);
int  is_file_exists ( const char const * file_nameP, const char const *file_roleP);
int  strip_extension( char *in_filename);
int  split_path     ( char * pathP, char *** resP);
void display_node   ( xmlNodePtr node, unsigned int indent);
void display_tree   ( xmlNodePtr node, unsigned int indent);
//-------------------------
void free_packet(test_packet_t* packet);
void free_scenario(test_scenario_t* scenario);
//-------------------------
void display_packet_sctp_init(const sctp_inithdr_t * const sctp);
void display_packet_sctp_initack(const sctp_initackhdr_t * const sctp);
void display_packet_sctp_data(const sctp_datahdr_t * const sctp);
void display_packet_sctp(const test_sctp_hdr_t * const sctp);
void display_packet_ip(const test_ip_hdr_t * const ip);
void display_packet(const test_packet_t * const packet);
void display_scenario(const test_scenario_t * const scenario);
//-------------------------
char * test_ip2ip_str(const test_ip_t * const ip);
int hex2data(unsigned char * const data, const unsigned char * const hexstring, const unsigned int len);
sctp_cid_t chunk_type_str2cid(const xmlChar * const chunk_type_str);
const char * const chunk_type_cid2str(const sctp_cid_t chunk_type);
test_action_t action_str2test_action_t(const xmlChar * const action);
void ip_str2test_ip(const xmlChar  * const ip_str, test_ip_t * const ip);
//-------------------------
int test_s1ap_decode_initiating_message(s1ap_message *message, S1ap_InitiatingMessage_t *initiating_p);
int test_s1ap_decode_successful_outcome(s1ap_message *message, S1ap_SuccessfulOutcome_t *successfullOutcome_p);
int test_s1ap_decode_unsuccessful_outcome(s1ap_message *message, S1ap_UnsuccessfulOutcome_t *unSuccessfullOutcome_p);
int test_s1ap_decode_pdu(s1ap_message *message, const uint8_t * const buffer,const uint32_t length);
void test_decode_s1ap(test_s1ap_t * const s1ap);
//-------------------------
void parse_s1ap(xmlDocPtr doc, const xmlNode const *s1ap_node, test_s1ap_t * const s1ap);
void parse_sctp_data_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_datahdr_t * const sctp_hdr);
void parse_sctp_init_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_inithdr_t * const sctp_hdr);
void parse_sctp_init_ack_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_initackhdr_t * const sctp_hdr);
void parse_sctp(xmlDocPtr doc, const xmlNode const *sctp_node, test_sctp_hdr_t * const sctp_hdr);
test_packet_t* parse_xml_packet(xmlDocPtr doc, xmlNodePtr node);
//-------------------------
int play_scenario(test_scenario_t* scenario);
int generate_xml_scenario(
    const char const * test_dir_name,
    const char const * test_scenario_filename,
    const char const * enb_config_filename,
          char const * play_scenario_filename /* OUT PARAM*/);

//-----------------------------------------------------------------------------
void test_print_hex_octets(const unsigned char * const byte_stream, const unsigned long int num)
//-----------------------------------------------------------------------------
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
// test if file exist in current directory
int is_file_exists( const char const * file_nameP, const char const *file_roleP)
{
  struct stat s;
  int err = stat(file_nameP, &s);
  if(-1 == err) {
    if(ENOENT == errno) {
      fprintf(stderr, "Please provide a valid %s, %s does not exist\n", file_roleP, file_nameP);
    } else {
      perror("stat");
      exit(1);
    }
  } else {
    if(S_ISREG(s.st_mode)) {
      return GS_IS_FILE;
    } else if(S_ISDIR(s.st_mode)) {
      return GS_IS_DIR;
    } else {
      fprintf(stderr, "Please provide a valid test %s, %s exists but is not found valid\n", file_roleP, file_nameP);
    }
  }
  return 0;
}


//------------------------------------------------------------------------------
int strip_extension(char *in_filename)
{
  static const uint8_t name_min_len = 1;
  static const uint8_t max_ext_len = 5; // .pdml !
  fprintf(stdout, "strip_extension %s\n", in_filename);

  if (NULL != in_filename) {
    /* Check chars starting at end of string to find last '.' */
    for (ssize_t i = strlen(in_filename); i > (name_min_len + max_ext_len); i--) {
      if (in_filename[i] == '.') {
        in_filename[i] = '\0';
        return i;
      }
    }
  }
  return -1;
}
//------------------------------------------------------------------------------
// return number of splitted items
int split_path( char * pathP, char *** resP)
{
  char *  saveptr1;
  char *  p    = strtok_r (pathP, "/", &saveptr1);
  int     n_spaces = 0;

  /// split string and append tokens to 'res'
  while (p) {
    *resP = realloc (*resP, sizeof (char*) * ++n_spaces);
    AssertFatal (*resP, "realloc failed");
    (*resP)[n_spaces-1] = p;
    p = strtok_r (NULL, "/", &saveptr1);
  }
  return n_spaces;
}
//------------------------------------------------------------------------------
void display_node(xmlNodePtr node, unsigned int indent)
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
/**
 * print_element_names:
 * @node: the initial xml node to consider.
 * @indent: indentation level.
 *
 * Prints the names of the all the xml elements
 * that are siblings or children of a given xml node.
 */
//------------------------------------------------------------------------------
void display_tree(xmlNodePtr node, unsigned int indent)
{
  xmlNode *cur_node = NULL;

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      display_node(cur_node, indent);
    }
    display_tree(cur_node->children, indent++);
  }
}
//------------------------------------------------------------------------------
void free_packet(test_packet_t* packet)
{
  if (packet) {
    switch (packet->sctp_hdr.chunk_type) {
      case SCTP_CID_DATA:
        free_pointer(packet->sctp_hdr.u.data_hdr.payload.binary_stream);
        break;
      default:
        ;
    }
    free_pointer(packet);
  }
}

//------------------------------------------------------------------------------
void free_scenario(test_scenario_t* scenario)
{
  test_packet_t *packet = NULL;
  test_packet_t *next_packet = NULL;
  if (scenario) {
    packet = scenario->list_packet;
    while (packet) {
      next_packet = packet->next;
      free_packet(packet);
      packet = next_packet->next;
    }
    free_pointer(scenario);
  }
}


//------------------------------------------------------------------------------
void display_packet_sctp_init(const sctp_inithdr_t * const sctp)
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
void display_packet_sctp_initack(const sctp_initackhdr_t * const sctp)
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
void display_packet_sctp_data(const sctp_datahdr_t * const sctp)
{
  if (sctp) {
    fprintf(stdout, "\t\tSCTP.data.tsn                 : %u\n", sctp->tsn);
    fprintf(stdout, "\t\tSCTP.data.stream              : %u\n", sctp->stream);
    fprintf(stdout, "\t\tSCTP.data.ssn                 : %u\n", sctp->ssn);
    fprintf(stdout, "\t\tSCTP.data.ppid                : %u\n", sctp->ppid);
    //fprintf(stdout, "\t\tSCTP.data.pdu_type            : %u\n", sctp->payload.pdu_type);
    //fprintf(stdout, "\t\tSCTP.data.procedure_code      : %u\n", sctp->payload.procedure_code);
    fprintf(stdout, "\t\tSCTP.data.binary_stream_allocated_size : %u\n", sctp->payload.binary_stream_allocated_size);
    if (NULL != sctp->payload.binary_stream) {
      fprintf(stdout, "\t\tSCTP.data.binary_stream       :\n");
      test_print_hex_octets(sctp->payload.binary_stream, sctp->payload.binary_stream_allocated_size);
    } else {
      fprintf(stdout, "\t\tSCTP.data.binary_stream       : NULL\n");
    }
  }
}

//------------------------------------------------------------------------------
void display_packet_sctp(const test_sctp_hdr_t * const sctp)
{
  if (sctp) {
    fprintf(stdout, "\t\tSCTP.src_port      : %u\n", sctp->src_port);
    fprintf(stdout, "\t\tSCTP.dst_port      : %u\n", sctp->dst_port);
    fprintf(stdout, "\t\tSCTP.chunk_type    : %s\n", chunk_type_cid2str(sctp->chunk_type));
    switch (sctp->chunk_type) {
      case SCTP_CID_DATA:
        display_packet_sctp_data(&sctp->u.data_hdr);
        break;
      case SCTP_CID_INIT:
        display_packet_sctp_initack(&sctp->u.init_hdr);
        break;
      case SCTP_CID_INIT_ACK:
        display_packet_sctp_initack(&sctp->u.init_ack_hdr);
        break;
      default:
        ;
    }
  }
}
//------------------------------------------------------------------------------
void display_packet_ip(const test_ip_hdr_t * const ip)
{
  if (ip) {
    fprintf(stdout, "\t\tSource address      : %s\n", test_ip2ip_str(&ip->src));
    fprintf(stdout, "\t\tDestination address : %s\n", test_ip2ip_str(&ip->dst));
  }
}

//------------------------------------------------------------------------------
void display_packet(const test_packet_t * const packet)
{
  if (packet) {
    fprintf(stdout, "\tPacket:\tnum %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
    fprintf(stdout, "\tPacket:\ttime relative to 1st packet           %ld.%06lu\n",
        packet->time_relative_to_first_packet.tv_sec, packet->time_relative_to_first_packet.tv_usec);
    fprintf(stdout, "\tPacket:\ttime relative to last tx packet       %ld.%06lu\n",
        packet->time_relative_to_last_sent_packet.tv_sec, packet->time_relative_to_last_sent_packet.tv_usec);
    fprintf(stdout, "\tPacket:\ttime relative to last_received packet %ld.%06lu\n",
        packet->time_relative_to_last_received_packet.tv_sec, packet->time_relative_to_last_received_packet.tv_usec);

    switch(packet->action) {
    case   ACTION_S1C_SEND:
      fprintf(stdout, "\tPacket:\tAction SEND\n");
      break;
    case   ACTION_S1C_RECEIVE:
      fprintf(stdout, "\tPacket:\tAction RECEIVE\n");
      break;
    default:
      fprintf(stdout, "\tPacket:\tAction UNKNOWN\n");
    }
    display_packet_ip(&packet->ip_hdr);
    display_packet_sctp(&packet->sctp_hdr);
  }
}
//------------------------------------------------------------------------------
void display_scenario(const test_scenario_t * const scenario)
{
  test_packet_t *packet = NULL;
  if (scenario) {
    fprintf(stdout, "Scenario: %s\n", (scenario->name != NULL) ? (char*)scenario->name:"UNKNOWN NAME");
    packet = scenario->list_packet;
    while (packet) {
      display_packet(packet);
      packet = packet->next;
    }
  }
}

//------------------------------------------------------------------------------
char * test_ip2ip_str(const test_ip_t * const ip)
{
  static char str[INET6_ADDRSTRLEN];

  sprintf(str, "ERROR");
  switch (ip->address_family) {
    case AF_INET6:
      inet_ntop(AF_INET6, &(ip->address.ipv6), str, INET6_ADDRSTRLEN);
      break;
    case AF_INET:
      inet_ntop(AF_INET, &(ip->address.ipv4), str, INET_ADDRSTRLEN);
      break;
    default:
      ;
  }
  return str;
}
//------------------------------------------------------------------------------
//convert hexstring to len bytes of data
//returns 0 on success, negative on error
//data is a buffer of at least len bytes
//hexstring is upper or lower case hexadecimal, NOT prepended with "0x"
int hex2data(unsigned char * const data, const unsigned char * const hexstring, const unsigned int len)
{
  unsigned const char *pos = hexstring;
  char *endptr = NULL;
  size_t count = 0;

  fprintf(stdout, "%s(%s,%d)\n", __FUNCTION__, hexstring, len);

  if ((len > 1) && (strlen((const char*)hexstring) % 2)) {
    //or hexstring has an odd length
    return -3;
  }

  if (len == 1)  {
    char buf[5] = {'0', 'x', 0, pos[0], '\0'};
    data[0] = strtol(buf, &endptr, 16);
    /* Check for various possible errors */
    AssertFatal ((errno == 0) || (data[0] != 0), "ERROR %s() strtol: %s\n", __FUNCTION__, strerror(errno));
    AssertFatal (endptr != buf, "ERROR %s() No digits were found\n", __FUNCTION__);
    return 0;
  }

  for(count = 0; count < len/2; count++) {
    char buf[5] = {'0', 'x', pos[0], pos[1], 0};
    data[count] = strtol(buf, &endptr, 16);
    pos += 2 * sizeof(char);
    AssertFatal (endptr[0] == '\0', "ERROR %s() non-hexadecimal character encountered buf %p endptr %p buf %s count %zu pos %p\n", __FUNCTION__, buf, endptr, buf, count, pos);
    AssertFatal (endptr != buf, "ERROR %s() No digits were found\n", __FUNCTION__);
  }
  return 0;
}
//------------------------------------------------------------------------------
sctp_cid_t chunk_type_str2cid(const xmlChar * const chunk_type_str)
{
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"DATA")))              { return SCTP_CID_DATA;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"INIT")))              { return SCTP_CID_INIT;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"INIT_ACK")))          { return SCTP_CID_INIT_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SACK")))              { return SCTP_CID_SACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"HEARTBEAT")))         { return SCTP_CID_HEARTBEAT;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"HEARTBEAT_ACK")))     { return SCTP_CID_HEARTBEAT_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ABORT")))             { return SCTP_CID_ABORT;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SHUTDOWN")))          { return SCTP_CID_SHUTDOWN;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SHUTDOWN_ACK")))      { return SCTP_CID_SHUTDOWN_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ERROR")))             { return SCTP_CID_ERROR;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"COOKIE_ECHO")))       { return SCTP_CID_COOKIE_ECHO;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"COOKIE_ACK")))        { return SCTP_CID_COOKIE_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ECN_ECNE")))          { return SCTP_CID_ECN_ECNE;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ECN_CWR")))           { return SCTP_CID_ECN_CWR;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SHUTDOWN_COMPLETE"))) { return SCTP_CID_SHUTDOWN_COMPLETE;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"AUTH")))              { return SCTP_CID_AUTH;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"FWD_TSN")))           { return SCTP_CID_FWD_TSN;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ASCONF")))            { return SCTP_CID_ASCONF;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ASCONF_ACK")))        { return SCTP_CID_ASCONF_ACK;}
  AssertFatal (0, "ERROR: %s() cannot convert: %s\n", __FUNCTION__, chunk_type_str);
}
//------------------------------------------------------------------------------
const char * const chunk_type_cid2str(const sctp_cid_t chunk_type)
{
  switch (chunk_type) {
    case  SCTP_CID_DATA:              return "DATA"; break;
    case  SCTP_CID_INIT:              return "INIT"; break;
    case  SCTP_CID_INIT_ACK:          return "INIT_ACK"; break;
    case  SCTP_CID_SACK:              return "SACK"; break;
    case  SCTP_CID_HEARTBEAT:         return "HEARTBEAT"; break;
    case  SCTP_CID_HEARTBEAT_ACK:     return "HEARTBEAT_ACK"; break;
    case  SCTP_CID_ABORT:             return "ABORT"; break;
    case  SCTP_CID_SHUTDOWN:          return "SHUTDOWN"; break;
    case  SCTP_CID_SHUTDOWN_ACK:      return "SHUTDOWN_ACK"; break;
    case  SCTP_CID_ERROR:             return "ERROR"; break;
    case  SCTP_CID_COOKIE_ECHO:       return "COOKIE_ECHO"; break;
    case  SCTP_CID_COOKIE_ACK:        return "COOKIE_ACK"; break;
    case  SCTP_CID_ECN_ECNE:          return "ECN_ECNE"; break;
    case  SCTP_CID_ECN_CWR:           return "ECN_CWR"; break;
    case  SCTP_CID_SHUTDOWN_COMPLETE: return "SHUTDOWN_COMPLETE"; break;
    case  SCTP_CID_AUTH:              return "AUTH"; break;
    case  SCTP_CID_FWD_TSN:           return "FWD_TSN"; break;
    case  SCTP_CID_ASCONF:            return "ASCONF"; break;
    case  SCTP_CID_ASCONF_ACK:        return "ASCONF_ACK"; break;
    default:
      AssertFatal (0, "ERROR %s(): Unknown chunk_type %d!\n", __FUNCTION__, chunk_type);
  }
}
//------------------------------------------------------------------------------
test_action_t action_str2test_action_t(const xmlChar * const action)
{
  if ((!xmlStrcmp(action, (const xmlChar *)"SEND")))              { return ACTION_S1C_SEND;}
  if ((!xmlStrcmp(action, (const xmlChar *)"RECEIVE")))              { return ACTION_S1C_RECEIVE;}
  AssertFatal (0, "ERROR: %s cannot convert: %s\n", __FUNCTION__, action);
  //if (NULL == action) {return ACTION_S1C_NULL;}
}
//------------------------------------------------------------------------------
void ip_str2test_ip(const xmlChar  * const ip_str, test_ip_t * const ip)
{
  AssertFatal (NULL != ip_str, "ERROR %s() Cannot convert null string to ip address!\n", __FUNCTION__);
  AssertFatal (NULL != ip,     "ERROR %s() out parameter pointer is NULL!\n", __FUNCTION__);
  // store this IP address in sa:
  if (inet_pton(AF_INET, (const char*)ip_str, (void*)&(ip->address.ipv4)) > 0) {
    ip->address_family = AF_INET;
  } else if (inet_pton(AF_INET6, (const char*)ip_str, (void*)&(ip->address.ipv6)) > 0) {
    ip->address_family = AF_INET6;
  } else {
    ip->address_family = AF_UNSPEC;
    AssertFatal (0, "ERROR %s() Could not parse ip address %s!\n", __FUNCTION__, ip_str);
  }
}
//------------------------------------------------------------------------------
int test_s1ap_decode_initiating_message(s1ap_message *message,
    S1ap_InitiatingMessage_t *initiating_p)
{
  char       *message_string = NULL;
  int         ret = -1;

  DevAssert(initiating_p != NULL);

  message_string = calloc(20000, sizeof(char));
  AssertFatal (NULL != message_string, "ERROR malloc()failed!\n");
  message->procedureCode = initiating_p->procedureCode;
  message->criticality   = initiating_p->criticality;

  switch(initiating_p->procedureCode) {
  case S1ap_ProcedureCode_id_downlinkNASTransport:
    ret = s1ap_decode_s1ap_downlinknastransporties(
            &message->msg.s1ap_DownlinkNASTransportIEs,
            &initiating_p->value);
    s1ap_xer_print_s1ap_downlinknastransport(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetuprequesties(
            &message->msg.s1ap_InitialContextSetupRequestIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_initialcontextsetuprequest(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_UEContextRelease:
    ret = s1ap_decode_s1ap_uecontextreleasecommandies(
            &message->msg.s1ap_UEContextReleaseCommandIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_uecontextreleasecommand(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_Paging:
    ret = s1ap_decode_s1ap_pagingies(
            &message->msg.s1ap_PagingIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_paging(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_uplinkNASTransport:
    ret = s1ap_decode_s1ap_uplinknastransporties (&message->msg.s1ap_UplinkNASTransportIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_uplinknastransport(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setuprequesties (&message->msg.s1ap_S1SetupRequestIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_s1setuprequest(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_initialUEMessage:
    ret = s1ap_decode_s1ap_initialuemessageies (&message->msg.s1ap_InitialUEMessageIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_initialuemessage(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_UEContextReleaseRequest:
    ret = s1ap_decode_s1ap_uecontextreleaserequesties (&message->msg.s1ap_UEContextReleaseRequestIEs, &initiating_p->value);
    s1ap_xer_print_s1ap_uecontextreleaserequest(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_UECapabilityInfoIndication:
    ret = s1ap_decode_s1ap_uecapabilityinfoindicationies (&message->msg.s1ap_UECapabilityInfoIndicationIEs, &initiating_p->value);
    //s1ap_xer_print_s1ap_uecapabilityinfoindication(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_NASNonDeliveryIndication:
    ret = s1ap_decode_s1ap_nasnondeliveryindication_ies (&message->msg.s1ap_NASNonDeliveryIndication_IEs, &initiating_p->value);
    s1ap_xer_print_s1ap_nasnondeliveryindication_(s1ap_xer__print2sp, message_string, message);
    break;

  default:
    free(message_string);
    AssertFatal( 0 , "Unknown procedure ID (%d) for initiating message\n",
                 (int)initiating_p->procedureCode);
    return -1;
  }
  fprintf(stdout, "s1ap_xer_print:\n%s\n", message_string);
  free(message_string);
  return ret;
}

//------------------------------------------------------------------------------
int test_s1ap_decode_successful_outcome(s1ap_message *message,
    S1ap_SuccessfulOutcome_t *successfullOutcome_p)
{
  char       *message_string = NULL;
  int         ret = -1;

  DevAssert(successfullOutcome_p != NULL);
  message_string = calloc(20000, sizeof(char));
  AssertFatal (NULL != message_string, "ERROR malloc()failed!\n");

  message->procedureCode = successfullOutcome_p->procedureCode;
  message->criticality   = successfullOutcome_p->criticality;

  switch(successfullOutcome_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setupresponseies(
            &message->msg.s1ap_S1SetupResponseIEs, &successfullOutcome_p->value);
    s1ap_xer_print_s1ap_s1setupresponse(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetupresponseies (&message->msg.s1ap_InitialContextSetupResponseIEs, &successfullOutcome_p->value);
    s1ap_xer_print_s1ap_initialcontextsetupresponse(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_UEContextRelease:
      ret = s1ap_decode_s1ap_uecontextreleasecompleteies (&message->msg.s1ap_UEContextReleaseCompleteIEs, &successfullOutcome_p->value);
      s1ap_xer_print_s1ap_uecontextreleasecomplete(s1ap_xer__print2sp, message_string, message);
    break;

  default:
    free(message_string);
    AssertFatal(0, "Unknown procedure ID (%d) for successfull outcome message\n",
               (int)successfullOutcome_p->procedureCode);
    return -1;
  }
  fprintf(stdout, "s1ap_xer_print:\n%s\n", message_string);
  free(message_string);
  return ret;
}

//------------------------------------------------------------------------------
int test_s1ap_decode_unsuccessful_outcome(s1ap_message *message,
    S1ap_UnsuccessfulOutcome_t *unSuccessfullOutcome_p)
{
  char       *message_string = NULL;
  int ret = -1;

  DevAssert(unSuccessfullOutcome_p != NULL);
  message_string = calloc(20000, sizeof(char));
  AssertFatal (NULL != message_string, "ERROR malloc()failed!\n");

  message->procedureCode = unSuccessfullOutcome_p->procedureCode;
  message->criticality   = unSuccessfullOutcome_p->criticality;

  switch(unSuccessfullOutcome_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    ret = s1ap_decode_s1ap_s1setupfailureies(
             &message->msg.s1ap_S1SetupFailureIEs, &unSuccessfullOutcome_p->value);
    s1ap_xer_print_s1ap_s1setupfailure(s1ap_xer__print2sp, message_string, message);
    break;

  case S1ap_ProcedureCode_id_InitialContextSetup:
    ret = s1ap_decode_s1ap_initialcontextsetupfailureies (&message->msg.s1ap_InitialContextSetupFailureIEs, &unSuccessfullOutcome_p->value);
    s1ap_xer_print_s1ap_initialcontextsetupfailure(s1ap_xer__print2sp, message_string, message);
    break;


  default:
    free(message_string);
    AssertFatal(0,"Unknown procedure ID (%d) for unsuccessfull outcome message\n",
               (int)unSuccessfullOutcome_p->procedureCode);
    break;
  }
  fprintf(stdout, "s1ap_xer_print:\n%s\n", message_string);
  free(message_string);
  return ret;
}

//------------------------------------------------------------------------------
int test_s1ap_decode_pdu(s1ap_message *message, const uint8_t * const buffer,
                        const uint32_t length)
{
  S1AP_PDU_t  pdu;
  S1AP_PDU_t *pdu_p = &pdu;
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  memset((void *)pdu_p, 0, sizeof(S1AP_PDU_t));

  dec_ret = aper_decode(NULL,
                        &asn_DEF_S1AP_PDU,
                        (void **)&pdu_p,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    S1AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  message->direction = pdu_p->present;

  switch(pdu_p->present) {
  case S1AP_PDU_PR_initiatingMessage:
    return test_s1ap_decode_initiating_message(message,
           &pdu_p->choice.initiatingMessage);

  case S1AP_PDU_PR_successfulOutcome:
    return test_s1ap_decode_successful_outcome(message,
           &pdu_p->choice.successfulOutcome);

  case S1AP_PDU_PR_unsuccessfulOutcome:
    return test_s1ap_decode_unsuccessful_outcome(message,
           &pdu_p->choice.unsuccessfulOutcome);

  default:
    AssertFatal(0, "Unknown presence (%d) or not implemented\n", (int)pdu_p->present);
    break;
  }
  return -1;
}
//------------------------------------------------------------------------------
void test_decode_s1ap(test_s1ap_t * const s1ap)
{
  if (NULL != s1ap) {
    if (test_s1ap_decode_pdu(&s1ap->message, s1ap->binary_stream, s1ap->binary_stream_allocated_size) < 0) {
      AssertFatal (0, "ERROR %s() Cannot decode S1AP message!\n", __FUNCTION__);
    }
  }
}
//------------------------------------------------------------------------------
void parse_s1ap(xmlDocPtr doc, const xmlNode const *s1ap_node, test_s1ap_t * const s1ap)
{
  xmlNode              *cur_node  = NULL;
  xmlChar              *xml_char  = NULL;
  xmlChar              *xml_char2  = NULL;
  unsigned int          size = 0;
  int                   rc = 0;
  unsigned int          go_deeper_in_tree = 1;

  if ((NULL != s1ap_node) && (NULL != s1ap)) {
    for (cur_node = (xmlNode *)s1ap_node; cur_node; cur_node = cur_node->next) {
      go_deeper_in_tree = 1;
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"field"))) {
        // do not get hidden fields
        xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"hide");
        if (NULL != xml_char) {
          if ((!xmlStrcmp(xml_char, (const xmlChar *)"yes"))) {
            go_deeper_in_tree = 0;
          }
          xmlFree(xml_char);
        }
        if (0 < go_deeper_in_tree) {
          // first get size
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"size");
          if (NULL != xml_char) {
            size = strtoul((const char *)xml_char, NULL, 0);
            xmlFree(xml_char);
            // second: try to get value (always hex)
            xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"value");
            if (NULL != xml_char) {
              xml_char2 = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"name");
              fprintf(stdout, "s1ap %p field %s  size %d value %s\n",s1ap, xml_char2, size, xml_char);
              xmlFree(xml_char2);
              // if success to get value, do not parse children
              //AssertFatal ((xmlStrlen(xml_char) == size), "ERROR %s() mismatch in size %d and strlen %d\n", __FUNCTION__, size, xmlStrlen(xml_char));
              //if (xmlStrlen(xml_char) == size) {
              AssertFatal ((s1ap->binary_stream_pos+xmlStrlen(xml_char)/2) <= s1ap->binary_stream_allocated_size,
                "ERROR %s() in buffer size: binary_stream_pos %d xmlStrlen(xml_char)/2=%d\n", __FUNCTION__, s1ap->binary_stream_pos, xmlStrlen(xml_char)/2);
              rc = hex2data( &s1ap->binary_stream[s1ap->binary_stream_pos], xml_char, xmlStrlen(xml_char));
              s1ap->binary_stream_pos += xmlStrlen(xml_char)/2;
              display_node(cur_node, 0);
              AssertFatal (rc >= 0, "ERROR %s() in converting hex string %s len %d size %d rc %d\n", __FUNCTION__, xml_char, xmlStrlen(xml_char), size, rc);
              go_deeper_in_tree = 0;
              //}
              xmlFree(xml_char);
            }
          }
        }
      }
      if (0 < go_deeper_in_tree) {
        parse_s1ap(doc, cur_node->children, s1ap);
      }
    }
  }
}

//------------------------------------------------------------------------------
void parse_sctp_data_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_datahdr_t * const sctp_hdr)
{
  xmlNode              *cur_node  = NULL;
  xmlChar              *xml_char  = NULL;
  xmlChar              *xml_char2 = NULL;

  if ((NULL != sctp_node) && (NULL != sctp_hdr)) {
    xml_char = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"name");
    if (NULL != xml_char) {
      if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.data_payload_proto_id"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->ppid = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.data_sid"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->stream = strtoul((const char *)xml_char2, NULL, 16);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.data_tsn"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->tsn = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.data_ssn"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->ssn = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      }
      xmlFree(xml_char);
    }
    for (cur_node = sctp_node->children; cur_node; cur_node = cur_node->next) {
      parse_sctp_data_chunk(doc, cur_node, sctp_hdr);
    }
  }

}
//------------------------------------------------------------------------------
void parse_sctp_init_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_inithdr_t * const sctp_hdr)
{
  xmlNode              *cur_node  = NULL;
  xmlChar              *xml_char  = NULL;
  xmlChar              *xml_char2 = NULL;

  if ((NULL != sctp_node) && (NULL != sctp_hdr)) {
    xml_char = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"name");
    if (NULL != xml_char) {
      if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.init_nr_out_streams"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->num_outbound_streams = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.init_nr_in_streams"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->num_inbound_streams = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.init_credit"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->a_rwnd = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.init_initial_tsn"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->initial_tsn = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.init_initiate_tag"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->init_tag = strtoul((const char *)xml_char2, NULL, 16);
          xmlFree(xml_char2);
        }
      }
      xmlFree(xml_char);
    }
    for (cur_node = sctp_node->children; cur_node; cur_node = cur_node->next) {
      parse_sctp_init_chunk(doc, cur_node, sctp_hdr);
    }
  }
}
//------------------------------------------------------------------------------
void parse_sctp_init_ack_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_initackhdr_t * const sctp_hdr)
{
  xmlNode              *cur_node  = NULL;
  xmlChar              *xml_char  = NULL;
  xmlChar              *xml_char2 = NULL;

  if ((NULL != sctp_node) && (NULL != sctp_hdr)) {
    xml_char = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"name");
    if (NULL != xml_char) {
      if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.initack_nr_out_streams"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->num_outbound_streams = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.initack_nr_in_streams"))) {
        xml_char2 = xmlGetProp((xmlNode *)(xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->num_inbound_streams = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.initack_credit"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->a_rwnd = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.initack_initial_tsn"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->initial_tsn = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.initack_initiate_tag"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"show");
        if (NULL != xml_char2) {
          sctp_hdr->init_tag = strtoul((const char *)xml_char2, NULL, 16);
          xmlFree(xml_char2);
        }
      }
      xmlFree(xml_char);
    }
    for (cur_node = sctp_node->children; cur_node; cur_node = cur_node->next) {
      parse_sctp_init_ack_chunk(doc, cur_node, sctp_hdr);
    }
  }
}
//------------------------------------------------------------------------------
void parse_sctp(xmlDocPtr doc, const xmlNode const *sctp_node, test_sctp_hdr_t * const sctp_hdr)
{
  xmlNode              *cur_node  = NULL;
  xmlChar              *xml_char  = NULL;
  xmlChar              *xml_char2 = NULL;

  if ((NULL != sctp_node) && (NULL != sctp_hdr)) {
    if ((!xmlStrcmp(sctp_node->name, (const xmlChar *)"proto"))) {
      xml_char = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"name");
      if (NULL != xml_char) {
        if ((!xmlStrcmp(xml_char, (const xmlChar *)"s1ap"))) {
          xmlFree(xml_char);
          xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"size");
          if (NULL != xml_char2) {
            sctp_hdr->u.data_hdr.payload.binary_stream_allocated_size = strtoul((const char *)xml_char2, NULL, 0);
            sctp_hdr->u.data_hdr.payload.binary_stream = calloc(1, sctp_hdr->u.data_hdr.payload.binary_stream_allocated_size);
            xmlFree(xml_char2);
          }
          parse_s1ap(doc, sctp_node, &sctp_hdr->u.data_hdr.payload);
          test_decode_s1ap(&sctp_hdr->u.data_hdr.payload);
          return;
        }
        xmlFree(xml_char);
      }
    }
    //if ((cur_node->type == XML_ATTRIBUTE_NODE) || (cur_node->type == XML_ELEMENT_NODE)) {
    xml_char = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"name");
    if (NULL != xml_char) {
      if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.srcport"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->src_port = strtoul((const char *)xml_char2, NULL, 16);
          xmlFree(xml_char2);
        }
      } else if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.dstport"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->dst_port = strtoul((const char *)xml_char2, NULL, 16);
          xmlFree(xml_char2);
        }
      } else  if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp.chunk_type"))) {
        xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"value");
        if (NULL != xml_char2) {
          sctp_hdr->chunk_type = strtoul((const char *)xml_char2, NULL, 0);
          xmlFree(xml_char2);
          switch (sctp_hdr->chunk_type) {
            case SCTP_CID_DATA:
              parse_sctp_data_chunk(doc, sctp_node->parent, &sctp_hdr->u.data_hdr);
              break;
            case SCTP_CID_INIT:
              parse_sctp_init_chunk(doc, sctp_node->parent, &sctp_hdr->u.init_hdr);
              break;
            case SCTP_CID_INIT_ACK:
              parse_sctp_init_ack_chunk(doc, sctp_node->parent, &sctp_hdr->u.init_ack_hdr);
              break;
            default:
              ;
          }
        }
      }
    }
    for (cur_node = sctp_node->children; cur_node; cur_node = cur_node->next) {
      parse_sctp(doc, cur_node, sctp_hdr);
    }
  }
}
//------------------------------------------------------------------------------
test_packet_t* parse_xml_packet(xmlDocPtr doc, xmlNodePtr node) {

  test_packet_t        *packet   = NULL;
  xmlNode              *cur_node = NULL;
  xmlChar              *xml_char = NULL;
  float                 afloat    = (float)0.0;
  static struct timeval initial_time         = { .tv_sec = 0, .tv_usec = 0 };
  static struct timeval relative_last_sent_packet     = { .tv_sec = 0, .tv_usec = 0 };
  static struct timeval relative_last_received_packet = { .tv_sec = 0, .tv_usec = 0 };
  static char           first_packet          = 1;
  static char           first_sent_packet     = 1;
  static char           first_received_packet = 1;
  static unsigned int   packet_number = 1;

  if (NULL != node) {
    packet = calloc(1, sizeof(*packet));

    xml_char = xmlGetProp(node, (const xmlChar *)"action");
    packet->action = action_str2test_action_t(xml_char);
    xmlFree(xml_char);
    packet->packet_number = packet_number++;

    for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
      //if (cur_node->type == XML_ELEMENT_NODE) {
        if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"frame.time_relative"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"value");
          afloat = atof((const char*)xml_char);
          xmlFree(xml_char);
          packet->time_relative_to_first_packet.tv_sec   = (int)afloat;
          packet->time_relative_to_first_packet.tv_usec  = (int)((afloat - packet->time_relative_to_first_packet.tv_sec)*1000000);

          if (first_packet > 0) {
            initial_time = packet->time_relative_to_first_packet;
            packet->time_relative_to_first_packet.tv_sec  = 0;
            packet->time_relative_to_first_packet.tv_usec = 0;
            first_packet = 0;
          } else {
            timersub(&packet->time_relative_to_first_packet, &initial_time,
                &packet->time_relative_to_first_packet);
          }
          if (packet->action == ACTION_S1C_SEND) {
            if (first_sent_packet > 0) {
              relative_last_sent_packet = packet->time_relative_to_first_packet;
              packet->time_relative_to_last_sent_packet.tv_sec  = 0;
              packet->time_relative_to_last_sent_packet.tv_usec = 0;
              first_sent_packet = 0;
            } else {
              timersub(&packet->time_relative_to_first_packet, &relative_last_sent_packet,
                  &packet->time_relative_to_last_sent_packet);
              relative_last_sent_packet = packet->time_relative_to_first_packet;
            }
          } else if (packet->action == ACTION_S1C_RECEIVE) {
            if (first_received_packet > 0) {
              relative_last_received_packet.tv_sec = packet->time_relative_to_first_packet.tv_sec;
              relative_last_received_packet.tv_usec = packet->time_relative_to_first_packet.tv_usec;
              packet->time_relative_to_last_received_packet.tv_sec  = 0;
              packet->time_relative_to_last_received_packet.tv_usec = 0;
              first_received_packet = 0;
            } else {
              timersub(&packet->time_relative_to_first_packet, &relative_last_received_packet,
                  &packet->time_relative_to_last_received_packet);
              relative_last_received_packet = packet->time_relative_to_first_packet;
            }
          }

        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"frame.number"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"value");
          packet->original_frame_number = strtoul((const char *)xml_char, NULL, 0);
          xmlFree(xml_char);
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ip.src"))) {
          xml_char = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
          ip_str2test_ip(xml_char, &packet->ip_hdr.src);
          xmlFree(xml_char);
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ip.dst"))) {
          xml_char = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
          ip_str2test_ip(xml_char, &packet->ip_hdr.dst);
          xmlFree(xml_char);
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"proto"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"name");
          if (NULL != xml_char) {
            if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp"))) {
              parse_sctp(doc, cur_node, &packet->sctp_hdr);
            }
            xmlFree(xml_char);
          }
        }
      //}
    }
  }
  return packet;
}
//------------------------------------------------------------------------------
int play_scenario(test_scenario_t* scenario) {
  //TODO
  display_scenario(scenario);
  return 0;
}
//------------------------------------------------------------------------------
test_scenario_t* generate_scenario(
    const char  * const play_scenario_filename )
{
  xmlDocPtr         doc      = NULL;
  xmlNodePtr        root     = NULL;
  xmlNodePtr        node     = NULL;
  xmlChar          *xml_char = NULL;
  test_scenario_t  *scenario = NULL;
  test_packet_t    *packet   = NULL;
  test_packet_t   **next_packet   = NULL;

  doc = xmlParseFile(play_scenario_filename);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse scenario xml file %s!\n", play_scenario_filename);
  } else {
    fprintf(stdout, "Test scenario file to play: %s\n", play_scenario_filename);
    //xmlDebugDumpDocument(NULL, doc);
  }

  // Get root
  root = xmlDocGetRootElement(doc);
  if (NULL != root) {
    if ((!xmlStrcmp(root->name, (const xmlChar *)"scenario"))) {
      xml_char = xmlGetProp(root, (const xmlChar *)"name");
      printf("scenario name: %s\n", xml_char);
      scenario = calloc(1, sizeof(*scenario));
      scenario->name = xml_char; // nodup nofree
      next_packet = &scenario->list_packet;
      for (node = root->children; node != NULL; node = node->next) {
        if ((!xmlStrcmp(node->name, (const xmlChar *)"packet"))) {
          packet = parse_xml_packet(doc, node);
          if (NULL != packet) {
            *next_packet = packet;
            next_packet = &packet->next;
          } else {
            fprintf(stdout, "WARNING omitted packet:\n");
            display_node(node, 0);
          }
        }
      }
    }
  } else {
    fprintf(stderr, "Empty xml document\n");
  }
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return scenario;
}

//------------------------------------------------------------------------------
int generate_xml_scenario(
    const char const * test_dir_name,
    const char const * test_scenario_filename,
    const char const * enb_config_filename,
          char const * play_scenario_filename /* OUT PARAM*/)
//------------------------------------------------------------------------------
{
  //int fd_pdml_in;
  xsltStylesheetPtr cur = NULL;
  xmlDocPtr         doc, res;
  FILE             *play_scenario_file = NULL;
  const char       *params[2*ENB_CONFIG_MAX_XSLT_PARAMS];
  int               nb_params = 0;
  int               i,j;
  char              astring[1024];
  struct in_addr    addr;
  int               ret      = 0;

  memset(astring, 0, sizeof(astring));
  if (getcwd(astring, sizeof(astring)) != NULL) {
    fprintf(stdout, "working in %s directory\n", astring);
  } else {
    perror("getcwd() ERROR");
    exit(1);
  }

  memset(astring, 0, sizeof(astring));
  strcat(astring, g_openair_dir);
  strcat(astring, "/openair3/TEST/EPC_TEST/play_scenario.xsl");

  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;
  cur = xsltParseStylesheetFile((const xmlChar *)astring);
  if (NULL == cur) {
    AssertFatal (0, "Could not parse stylesheet file %s (check OPENAIR_DIR env variable)!\n", astring);
  } else {
    fprintf(stdout, "XSLT style sheet: %s\n", astring);
  }

  doc = xmlParseFile(test_scenario_filename);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse scenario xml file %s!\n", test_scenario_filename);
  } else {
    fprintf(stdout, "Test scenario file: %s\n", test_scenario_filename);
  }

  for (i = 0; i < enb_properties.number; i++) {
    // eNB S1-C IPv4 address
    sprintf(astring, "enb_s1c%d", i);
    params[nb_params++] = strdup(astring);
    addr.s_addr = enb_properties.properties[i]->enb_ipv4_address_for_S1_MME;
    sprintf(astring, "\"%s\"", inet_ntoa(addr));
    params[nb_params++] = strdup(astring);

    // MME S1-C IPv4 address
    for (j = 0; j < enb_properties.properties[i]->nb_mme; j++) {
      sprintf(astring, "mme_s1c%d_%d", i, j);
      params[nb_params++] = strdup(astring);
      AssertFatal (enb_properties.properties[i]->mme_ip_address[j].ipv4_address,
          "Only support MME IPv4 address\n");
      sprintf(astring, "\"%s\"", enb_properties.properties[i]->mme_ip_address[j].ipv4_address);
      params[nb_params++] = strdup(astring);
    }
  }
  params[nb_params] = NULL;
  res = xsltApplyStylesheet(cur, doc, params);
  if (NULL != res) {
    sprintf((char *)play_scenario_filename,"%s",test_scenario_filename);
    if (strip_extension((char *)play_scenario_filename) > 0) {
      strcat((char *)play_scenario_filename, ".tsml");
      play_scenario_file = fopen( play_scenario_filename, "w+");
      if (NULL != play_scenario_file) {
        xsltSaveResultToFile(play_scenario_file, res, cur);
        fclose(play_scenario_file);
        fprintf(stdout, "Wrote test scenario to %s\n", play_scenario_filename);
      } else {
        fprintf(stderr, "ERROR in fopen(%s)\n", play_scenario_filename);
        ret = -1;
      }
    } else {
      fprintf(stderr, "ERROR in strip_extension()\n");
      ret = -1;
    }
  } else {
    fprintf(stderr, "ERROR in xsltApplyStylesheet()\n");
    ret = -1;
  }
  xsltFreeStylesheet(cur);
  xmlFreeDoc(doc);
  xmlFreeDoc(res);

  xsltCleanupGlobals();
  xmlCleanupParser();
  return ret;
}

//------------------------------------------------------------------------------
static void usage (
    int argc,
    char *argv[])
//------------------------------------------------------------------------------
{
  fprintf (stdout, "Please report any bug to: %s\n",PACKAGE_BUGREPORT);
  fprintf (stdout, "Usage: %s [options]\n\n", argv[0]);
  fprintf (stdout, "\n");
  fprintf (stdout, "Client options:\n");
  fprintf (stdout, "\t-S | --server         <server network @>  File name (with no path) of a test scenario that has to be replayed (TODO in future?)\n");
  fprintf (stdout, "Server options:\n");
  fprintf (stdout, "\t-d | --test-dir       <dir>               Directory where a set of files related to a particular test are located\n");
  fprintf (stdout, "\t-c | --enb-conf-file  <file>              Provide an eNB config file, valid for the testbed\n");
  fprintf (stdout, "\t-s | --scenario       <file>              File name (with no path) of a test scenario that has to be replayed ()\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Other options:\n");
  fprintf (stdout, "\t-h | --help                               Print this help and return\n");
  fprintf (stdout, "\t-v | --version                            Print informations about the version of this executable\n");
  fprintf (stdout, "\n");
}

//------------------------------------------------------------------------------
int
config_parse_opt_line (
  int argc,
  char *argv[],
  char **test_dir_name,
  char **scenario_file_name,
  char **enb_config_file_name)
//------------------------------------------------------------------------------
{
  int                           option;
  int                           rv                   = 0;
  const Enb_properties_array_t *enb_properties_p     = NULL;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */
    LONG_OPTION_ENB_CONF_FILE,
    LONG_OPTION_SCENARIO_FILE,
    LONG_OPTION_TEST_DIR,
    LONG_OPTION_HELP,
    LONG_OPTION_VERSION
  };

  static struct option long_options[] = {
    {"enb-conf-file",  required_argument, 0, LONG_OPTION_ENB_CONF_FILE},
    {"scenario ",      required_argument, 0, LONG_OPTION_SCENARIO_FILE},
    {"test-dir",       required_argument, 0, LONG_OPTION_TEST_DIR},
    {"help",           no_argument,       0, LONG_OPTION_HELP},
    {"version",        no_argument,       0, LONG_OPTION_VERSION},
     {NULL, 0, NULL, 0}
  };

  /*
   * Parsing command line
   */
  while ((option = getopt_long (argc, argv, "vhc:s:d:", long_options, NULL)) != -1) {
    switch (option) {
      case LONG_OPTION_ENB_CONF_FILE:
      case 'c':
        if (optarg) {
          *enb_config_file_name = strdup(optarg);
          printf("eNB config file name is %s\n", *enb_config_file_name);
          rv |= PLAY_SCENARIO;
        }
        break;

      case LONG_OPTION_SCENARIO_FILE:
      case 's':
        if (optarg) {
          *scenario_file_name = strdup(optarg);
          printf("Scenario file name is %s\n", *scenario_file_name);
          rv |= PLAY_SCENARIO;
        }
        break;

      case LONG_OPTION_TEST_DIR:
      case 'd':
        if (optarg) {
          *test_dir_name = strdup(optarg);
          if (is_file_exists(*test_dir_name, "test dirname") != GS_IS_DIR) {
            fprintf(stderr, "Please provide a valid test dirname, %s is not a valid directory name\n", *test_dir_name);
            exit(1);
          }
          printf("Test dir name is %s\n", *test_dir_name);
        }
        break;

      case LONG_OPTION_VERSION:
      case 'v':
        printf("Version %s\n", PACKAGE_VERSION);
        exit (0);
        break;

      case LONG_OPTION_HELP:
      case 'h':
      default:
        usage (argc, argv);
        exit (0);
    }
  }
  if (NULL == *test_dir_name) {
    fprintf(stderr, "Please provide a valid test dirname\n");
    exit(1);
  }
  if (chdir(*test_dir_name) != 0) {
    fprintf(stderr, "ERROR: chdir %s returned %s\n", *test_dir_name, strerror(errno));
    exit(1);
  }
  if (rv & PLAY_SCENARIO) {
    if (NULL == *enb_config_file_name) {
      fprintf(stderr, "ERROR: please provide the original eNB config file name that should be in %s\n", *test_dir_name);
    }
    if (is_file_exists(*enb_config_file_name, "eNB config file") != GS_IS_FILE) {
      fprintf(stderr, "ERROR: original eNB config file name %s is not found in dir %s\n", *enb_config_file_name, *test_dir_name);
    }
    enb_properties_p = enb_config_init(*enb_config_file_name);

    if (NULL == *scenario_file_name) {
      fprintf(stderr, "ERROR: please provide the scenario file name that should be in %s\n", *test_dir_name);
    }
    if (is_file_exists(*scenario_file_name, "Scenario file") != GS_IS_FILE) {
      fprintf(stderr, "ERROR: Scenario file name %s is not found in dir %s\n", *scenario_file_name, *test_dir_name);
    }
  }
  return rv;
}

//------------------------------------------------------------------------------
int main( int argc, char **argv )
//------------------------------------------------------------------------------
{
  int              actions              = 0;
  char            *test_dir_name        = NULL;
  char            *scenario_file_name   = NULL;
  char            *enb_config_file_name = NULL;
  char             play_scenario_filename[NAME_MAX];
  int              ret                  = 0;
  test_scenario_t *scenario             = NULL;

  memset(play_scenario_filename, 0, sizeof(play_scenario_filename));
  g_openair_dir = getenv("OPENAIR_DIR");
  if (NULL == g_openair_dir) {
    fprintf(stderr, "ERROR: Could not get OPENAIR_DIR environment variable\n");
    exit(1);
  }

  // logging
  logInit();
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, NULL);

  set_comp_log(S1AP, LOG_TRACE, LOG_MED, 1);
  set_comp_log(SCTP, LOG_TRACE, LOG_MED, 1);
  asn_debug      = 1;
  asn1_xer_print = 1;

  //parameters
  actions = config_parse_opt_line (argc, argv, &test_dir_name, &scenario_file_name, &enb_config_file_name); //Command-line options
  if  (actions & PLAY_SCENARIO) {
    if (generate_xml_scenario(test_dir_name, scenario_file_name,enb_config_file_name, play_scenario_filename) == 0) {
      if (NULL != (scenario = generate_scenario(play_scenario_filename))) {
        ret = play_scenario(scenario);
      } else {
        fprintf(stderr, "ERROR: Could not generate scenario from tsml file\n");
        ret = -1;
      }
    } else {
      fprintf(stderr, "ERROR: Could not generate tsml scenario from xml file\n");
      ret = -1;
    }
    free_pointer(test_dir_name);
    free_pointer(scenario_file_name);
    free_pointer(enb_config_file_name);
  }

  return ret;
}
