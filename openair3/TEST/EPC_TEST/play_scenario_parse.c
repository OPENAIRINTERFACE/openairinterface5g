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
                                play_scenario_parse.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */

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
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "intertask_interface.h"
#include "platform_types.h"
#include "assertions.h"
#include "play_scenario.h"
//------------------------------------------------------------------------------
#define ENB_CONFIG_MAX_XSLT_PARAMS 32
//------------------------------------------------------------------------------
extern Enb_properties_array_t g_enb_properties;
//------------------------------------------------------------------------------
void et_parse_s1ap(xmlDocPtr doc, const xmlNode const *s1ap_node, et_s1ap_t * const s1ap)
{
  xmlNodePtr            cur_node  = NULL;
  xmlChar              *xml_char  = NULL;
  xmlChar              *xml_char2  = NULL;
  unsigned int          size = 0;
  int                   rc = 0;
  unsigned int          go_deeper_in_tree = 1;

  if ((NULL != s1ap_node) && (NULL != s1ap)) {
    // see http://www.xmlsoft.org/html/libxml-tree.html#xmlCopyNode
    if (NULL == s1ap->doc) {
      xmlUnlinkNode(s1ap_node);
      //cur_node = xmlCopyNodeList(s1ap_node);
      //  arg2: if 1 do a recursive copy (properties, namespaces and children when applicable) if 2 copy properties and namespaces (when applicable)
      //cur_node = xmlCopyNode(s1ap_node, 1);
      //AssertFatal(NULL != cur_node, "xmlCopyNode Failed");
      s1ap->doc = xmlNewDoc(BAD_CAST "1.0");
      xmlDocSetRootElement(s1ap->doc, s1ap_node);
    }

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
              fprintf(stdout, "s1ap %p field name %s  size %d value %s\n",s1ap, xml_char2, size, xml_char);
              xmlFree(xml_char2);
              // if success to get value, do not parse children
              //AssertFatal ((xmlStrlen(xml_char) == size), "ERROR %s() mismatch in size %d and strlen %d\n", __FUNCTION__, size, xmlStrlen(xml_char));
              //if (xmlStrlen(xml_char) == size) {
              AssertFatal ((s1ap->binary_stream_pos+xmlStrlen(xml_char)/2) <= s1ap->binary_stream_allocated_size,
                "ERROR in buffer size: binary_stream_pos %d xmlStrlen(xml_char)/2=%d\n", s1ap->binary_stream_pos, xmlStrlen(xml_char)/2);
              rc = et_hex2data( &s1ap->binary_stream[s1ap->binary_stream_pos], xml_char, xmlStrlen(xml_char));
              s1ap->binary_stream_pos += xmlStrlen(xml_char)/2;
              //et_display_node(cur_node, 0);
              AssertFatal (rc >= 0, "ERROR in converting hex string %s len %d size %d rc %d\n", xml_char, xmlStrlen(xml_char), size, rc);
              go_deeper_in_tree = 0;
              //}
              xmlFree(xml_char);
            }
          }
        }
      }
      if (0 < go_deeper_in_tree) {
        et_parse_s1ap(doc, cur_node->children, s1ap);
      }
    }
  }
}

//------------------------------------------------------------------------------
void et_parse_sctp_data_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_datahdr_t * const sctp_hdr)
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
      et_parse_sctp_data_chunk(doc, cur_node, sctp_hdr);
    }
  }

}
//------------------------------------------------------------------------------
void et_parse_sctp_init_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_inithdr_t * const sctp_hdr)
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
      et_parse_sctp_init_chunk(doc, cur_node, sctp_hdr);
    }
  }
}
//------------------------------------------------------------------------------
void et_parse_sctp_init_ack_chunk(xmlDocPtr doc, const xmlNode const *sctp_node, sctp_initackhdr_t * const sctp_hdr)
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
      et_parse_sctp_init_ack_chunk(doc, cur_node, sctp_hdr);
    }
  }
}
//------------------------------------------------------------------------------
void et_parse_sctp(xmlDocPtr doc, const xmlNode const *sctp_node, et_sctp_hdr_t * const sctp_hdr)
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
          xml_char = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"pos");
          if (NULL != xml_char) {
            sctp_hdr->u.data_hdr.payload.xml_stream_pos_offset = strtoul((const char *)xml_char, NULL, 0);
            xmlFree(xml_char);

            xml_char2 = xmlGetProp((xmlNode *)sctp_node, (const xmlChar *)"size");
            if (NULL != xml_char2) {
              sctp_hdr->u.data_hdr.payload.binary_stream_allocated_size = strtoul((const char *)xml_char2, NULL, 0);
              sctp_hdr->u.data_hdr.payload.binary_stream     = calloc(1, sctp_hdr->u.data_hdr.payload.binary_stream_allocated_size);
              sctp_hdr->u.data_hdr.payload.binary_stream_pos = 0;
              fprintf(stdout, "Allocating payload of sctp_hdr %p %u bytes\n", sctp_hdr, sctp_hdr->u.data_hdr.payload.binary_stream_allocated_size);
              xmlFree(xml_char2);
            }
            et_parse_s1ap(doc, sctp_node, &sctp_hdr->u.data_hdr.payload);
            et_decode_s1ap(&sctp_hdr->u.data_hdr.payload);
            return;
          }
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
              et_parse_sctp_data_chunk(doc, sctp_node->parent, &sctp_hdr->u.data_hdr);
              break;
            case SCTP_CID_INIT:
              et_parse_sctp_init_chunk(doc, sctp_node->parent, &sctp_hdr->u.init_hdr);
              break;
            case SCTP_CID_INIT_ACK:
              et_parse_sctp_init_ack_chunk(doc, sctp_node->parent, &sctp_hdr->u.init_ack_hdr);
              break;
            default:
              ;
          }
        }
      }
    }
    for (cur_node = sctp_node->children; cur_node; cur_node = cur_node->next) {
      et_parse_sctp(doc, cur_node, sctp_hdr);
    }
  }
}
//------------------------------------------------------------------------------
void et_packet_shift_timing(et_packet_t * const packet, const struct timeval * const shift)
{
  timeval_add(&packet->time_relative_to_first_packet, &packet->time_relative_to_first_packet, shift);
  AssertFatal((packet->time_relative_to_first_packet.tv_sec >= 0) && (packet->time_relative_to_first_packet.tv_usec >= 0),
      "Bad timing result time_relative_to_first_packet=%d.%d packet num %u, original frame number %u",
      packet->time_relative_to_first_packet.tv_sec,
      packet->time_relative_to_first_packet.tv_usec,
      packet->packet_number,
      packet->original_frame_number);

  timeval_add(&packet->time_relative_to_last_received_packet, &packet->time_relative_to_last_received_packet, shift);
  AssertFatal((packet->time_relative_to_last_received_packet.tv_sec >= 0) && (packet->time_relative_to_last_received_packet.tv_usec >= 0),
      "Bad timing result time_relative_to_last_received_packet=%d.%d packet num %u, original frame number %u",
      packet->time_relative_to_last_received_packet.tv_sec,
      packet->time_relative_to_last_received_packet.tv_usec,
      packet->packet_number,
      packet->original_frame_number);

  timeval_add(&packet->time_relative_to_last_sent_packet, &packet->time_relative_to_last_sent_packet, shift);
  AssertFatal((packet->time_relative_to_last_sent_packet.tv_sec >= 0) && (packet->time_relative_to_last_sent_packet.tv_usec >= 0),
      "Bad timing result time_relative_to_last_sent_packet=%d.%d packet num %u, original frame number %u",
      packet->time_relative_to_last_sent_packet.tv_sec,
      packet->time_relative_to_last_sent_packet.tv_usec,
      packet->packet_number,
      packet->original_frame_number);
}

//------------------------------------------------------------------------------
et_packet_t* et_parse_xml_packet(xmlDocPtr doc, xmlNodePtr node)
{

  et_packet_t          *packet   = NULL;
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
    packet->action = et_action_str2et_action_t(xml_char);
    packet->status = ET_PACKET_STATUS_NONE;
    xmlFree(xml_char);
    packet->packet_number = packet_number++;

    for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
      //if (cur_node->type == XML_ELEMENT_NODE) {
        if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"frame.time_relative"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"value");
          afloat = atof((const char*)xml_char);
          xmlFree(xml_char);
          fprintf(stdout, "Parsing packet frame.time_relative: %f\n", afloat);
          packet->time_relative_to_first_packet.tv_sec   = (int)afloat;
          packet->time_relative_to_first_packet.tv_usec  = (int)((afloat - packet->time_relative_to_first_packet.tv_sec)*1000000.0);

          if (first_packet > 0) {
            initial_time = packet->time_relative_to_first_packet;
            packet->time_relative_to_first_packet.tv_sec  = 0;
            packet->time_relative_to_first_packet.tv_usec = 0;
            first_packet = 0;
          } else {
            timersub(&packet->time_relative_to_first_packet, &initial_time,
                &packet->time_relative_to_first_packet);
          }
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"frame.number"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"value");
          packet->original_frame_number = strtoul((const char *)xml_char, NULL, 0);
          fprintf(stdout, "Parsing packet frame.number: %u\n", packet->original_frame_number);
          xmlFree(xml_char);
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ip.src"))) {
          xml_char = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
          et_ip_str2et_ip(xml_char, &packet->ip_hdr.src);
          xmlFree(xml_char);
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ip.dst"))) {
          xml_char = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
          et_ip_str2et_ip(xml_char, &packet->ip_hdr.dst);
          xmlFree(xml_char);
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"proto"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"name");
          if (NULL != xml_char) {
            if ((!xmlStrcmp(xml_char, (const xmlChar *)"sctp"))) {
              et_parse_sctp(doc, cur_node, &packet->sctp_hdr);
            }
            xmlFree(xml_char);
          }
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"eNB.instance"))) {
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"value");
          packet->enb_instance = strtoul((const char *)xml_char, NULL, 0);
          xmlFree(xml_char);
        }
      //}
    }
  }
  return packet;
}
//------------------------------------------------------------------------------
et_scenario_t* et_generate_scenario(
    const char  * const tsml_out_scenario_filename)
{
  xmlDocPtr            doc           = NULL;
  xmlNodePtr           root          = NULL;
  xmlNodePtr           node          = NULL;
  xmlChar             *xml_char      = NULL;
  et_scenario_t       *scenario      = NULL;
  et_packet_t          *last_packet  = NULL;
  et_packet_t          *packet       = NULL;
  et_packet_t         **next_packet  = NULL;

  doc = xmlParseFile(tsml_out_scenario_filename);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse scenario xml file %s!\n", tsml_out_scenario_filename);
  } else {
    fprintf(stdout, "Test scenario file to play: %s\n", tsml_out_scenario_filename);
    //xmlDebugDumpDocument(NULL, doc);
  }

  // Get root
  root = xmlDocGetRootElement(doc);
  if (NULL != root) {
    if ((!xmlStrcmp(root->name, (const xmlChar *)"scenario"))) {
      xml_char = xmlGetProp(root, (const xmlChar *)"name");
      printf("scenario name: %s\n", xml_char);
      scenario            = calloc(1, sizeof(*scenario));
      scenario->name      = xml_char; // nodup nofree

      next_packet = &scenario->list_packet;
      for (node = root->children; node != NULL; node = node->next) {
        if ((!xmlStrcmp(node->name, (const xmlChar *)"packet"))) {
          packet = et_parse_xml_packet(doc, node);
          if (NULL != packet) {
            // special case: S1AP same frame for 2 packets
            if (NULL != last_packet) {
              if (last_packet->original_frame_number == packet->original_frame_number) {
                // updating because these informations are not in 2nd sctp header (same IP packet)
                packet->sctp_hdr.dst_port = last_packet->sctp_hdr.dst_port;
                packet->sctp_hdr.src_port = last_packet->sctp_hdr.src_port;
              }
            }
            *next_packet = packet;
            next_packet = &packet->next;
          } else {
            fprintf(stdout, "WARNING omitted packet\n");
          }
        }
        last_packet = packet;
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
int et_generate_xml_scenario(
    const char const * xml_in_dir_name,
    const char const * xml_in_scenario_filename,
    const char const * enb_config_filename,
          char const * tsml_out_scenario_filename)
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


  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;
  cur = xsltParseStylesheetFile((const xmlChar *)"/usr/share/oai/xsl/play_scenario.xsl");
  if (NULL == cur) {
    AssertFatal (0, "Could not parse stylesheet file /usr/share/oai/xsl/play_scenario.xsl!\n");
  } else {
    fprintf(stdout, "XSLT style sheet: /usr/share/oai/xsl/play_scenario.xsl\n");
  }

  doc = xmlParseFile(xml_in_scenario_filename);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse scenario xml file %s!\n", xml_in_scenario_filename);
  } else {
    fprintf(stdout, "Test scenario file: %s\n", xml_in_scenario_filename);
  }

  for (i = 0; i < g_enb_properties.number; i++) {
    // eNB S1-C IPv4 address
    sprintf(astring, "enb%d_s1c", i);
    params[nb_params++] = strdup(astring);
    addr.s_addr = g_enb_properties.properties[i]->enb_ipv4_address_for_S1_MME;
    sprintf(astring, "\"%s\"", inet_ntoa(addr));
    params[nb_params++] = strdup(astring);

    // MME S1-C IPv4 address
    for (j = 0; j < g_enb_properties.properties[i]->nb_mme; j++) {
      sprintf(astring, "mme%d_s1c_%d", i, j);
      params[nb_params++] = strdup(astring);
      AssertFatal (g_enb_properties.properties[i]->mme_ip_address[j].ipv4_address,
          "Only support MME IPv4 address\n");
      sprintf(astring, "\"%s\"", g_enb_properties.properties[i]->mme_ip_address[j].ipv4_address);
      params[nb_params++] = strdup(astring);
    }
  }
  params[nb_params] = NULL;
  res = xsltApplyStylesheet(cur, doc, params);
  if (NULL != res) {
    sprintf((char *)tsml_out_scenario_filename,"%s",xml_in_scenario_filename);
    if (et_strip_extension((char *)tsml_out_scenario_filename) > 0) {
      strcat((char *)tsml_out_scenario_filename, ".tsml");
      play_scenario_file = fopen( tsml_out_scenario_filename, "w+");
      if (NULL != play_scenario_file) {
        xsltSaveResultToFile(play_scenario_file, res, cur);
        fclose(play_scenario_file);
        fprintf(stdout, "Wrote test scenario to %s\n", tsml_out_scenario_filename);
      } else {
        fprintf(stderr, "ERROR in fopen(%s)\n", tsml_out_scenario_filename);
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

