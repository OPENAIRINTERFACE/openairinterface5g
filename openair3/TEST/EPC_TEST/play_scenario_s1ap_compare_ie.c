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
                                play_scenario_s1ap_compare_ie.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <crypt.h>
#include <errno.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "tree.h"
#include "queue.h"


#include "intertask_interface.h"
#include "platform_types.h"
#include "assertions.h"
#include "conversions.h"
#include "s1ap_common.h"
#include "play_scenario_s1ap_eNB_defs.h"
#include "play_scenario.h"
#include "msc.h"
//------------------------------------------------------------------------------
extern et_scenario_t  *g_scenario;
extern uint32_t        g_constraints;
//------------------------------------------------------------------------------

//asn_comp_rval_t * et_s1ap_ies_is_matching(const S1AP_PDU_PR present, s1ap_message * const m1, s1ap_message * const m2, const uint32_t constraints)
//{
//}

void update_xpath_node_mme_ue_s1ap_id(et_s1ap_t * const s1ap, xmlNode *node, const S1ap_MME_UE_S1AP_ID_t new_id)
{
  xmlNode       *cur_node = NULL;
  xmlAttrPtr     attr     = NULL;
  xmlChar       *xml_char = NULL;
  int            size     = 0;
  int            pos      = 0;
  int            go_deeper_in_tree = 1;
  //S1AP_INFO("%s() mme_ue_s1ap_id %u\n", __FUNCTION__, new_id);

  // modify
  for (cur_node = (xmlNode *)node; cur_node; cur_node = cur_node->next) {
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
        xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"pos");
        if (NULL != xml_char) {
          pos = strtoul((const char *)xml_char, NULL, 0);
          pos -= s1ap->xml_stream_pos_offset;
          AssertFatal(pos >= 0, "Bad pos %d xml_stream_pos_offset %d", pos, s1ap->xml_stream_pos_offset);
          xmlFree(xml_char);
          xml_char = xmlGetProp((xmlNode *)cur_node, (const xmlChar *)"size");
          if (NULL != xml_char) {
            const xmlChar value_d[32];
            const xmlChar value_h[20];
            const xmlChar showname[64];
            int           ret     = 0;
            int           pos2    = 0;
            unsigned long int uli = 0;
            char          hex[3]  = {0,0,0};
            char         *end_ptr = NULL;

            size = strtoul((const char *)xml_char, NULL, 0);
            xmlFree(xml_char);
            // second: try to set value (always hex)
            ret = snprintf((char *)value_d, 32, "%ld", new_id);
            AssertFatal((ret > 0) && (ret < 32), "Could not convert int to dec str");
            ret = snprintf((char *)value_h, 20, "C0%08X", new_id);
            AssertFatal((ret > 0) && (ret < 20), "Could not convert int to hex str");
            ret = snprintf((char *)showname, 64, "MME-UE-S1AP-ID: %d", new_id);
            AssertFatal((ret > 0) && (ret < 64), "Could not convert int to dec str");

            attr = xmlSetProp((xmlNode *)cur_node, (const xmlChar *)"value", value_h);
            attr = xmlSetProp((xmlNode *)cur_node, (const xmlChar *)"show", value_d);
            attr = xmlSetProp((xmlNode *)cur_node, (const xmlChar *)"showname", showname);
            //TODO update s1ap->binary_stream @pos with new_id_hex, size
            AssertFatal((pos+size) < s1ap->binary_stream_allocated_size, "Rewrite of mme_ue_s1ap_id out of bounds of binary_stream");
            //avoid endianess issues
            do {
              hex[0] = value_h[pos2++];
              hex[1] = value_h[pos2++];
              hex[2] = '\0';
              end_ptr = hex;
              uli = strtoul(hex, &end_ptr, 16);
              AssertFatal((uli != ULONG_MAX) && (end_ptr != NULL) && (*end_ptr == '\0'), "Conversion of hexstring %s failed returned %lu errno %d", hex, uli, errno);
              s1ap->binary_stream[pos++] = (unsigned char)uli;
            } while (pos2 < (2*5));
            // update ASN1
            et_decode_s1ap(s1ap);
            //S1AP_INFO("Updated ASN1 for %s\n", showname);
          }
        }
      }
    }
    if (0 < go_deeper_in_tree) {
      update_xpath_node_mme_ue_s1ap_id(s1ap, cur_node->children, new_id);
    }
  }
}

/**
 * update_xpath_nodes:
 * @nodes:    the nodes set.
 * @value:    the new value for the node(s)
 *
 * Prints the @nodes content to @output.
 * From http://www.xmlsoft.org/examples/#xpath2.c
 */
void update_xpath_nodes_mme_ue_s1ap_id(et_s1ap_t * const s1ap_payload, xmlNodeSetPtr nodes, const S1ap_MME_UE_S1AP_ID_t new_id)
{
  int           size = 0;
  int           i    = 0;
  xmlNode      *s1ap_node = NULL;

  size = (nodes) ? nodes->nodeNr : 0;
  //S1AP_DEBUG("%s() num nodes %u\n", __FUNCTION__, size);

  /*
   * NOTE: the nodes are processed in reverse order, i.e. reverse document
   *       order because xmlNodeSetContent can actually free up descendant
   *       of the node and such nodes may have been selected too ! Handling
   *       in reverse order ensure that descendant are accessed first, before
   *       they get removed. Mixing XPath and modifications on a tree must be
   *       done carefully !
   */
  for(i = size - 1; i >= 0; i--) {
    s1ap_node = nodes->nodeTab[i];
    AssertFatal(NULL != s1ap_node, "One element of resultset of XPATH expression is NULL\n");
    update_xpath_node_mme_ue_s1ap_id(s1ap_payload, s1ap_node, new_id);
    /*
     * All the elements returned by an XPath query are pointers to
     * elements from the tree *except* namespace nodes where the XPath
     * semantic is different from the implementation in libxml2 tree.
     * As a result when a returned node set is freed when
     * xmlXPathFreeObject() is called, that routine must check the
     * element type. But node from the returned set may have been removed
     * by xmlNodeSetContent() resulting in access to freed data.
     * This can be exercised by running valgrind
     * There is 2 ways around it:
     *   - make a copy of the pointers to the nodes from the result set
     *     then call xmlXPathFreeObject() and then modify the nodes
     * or
     *   - remove the reference to the modified nodes from the node set
     *     as they are processed, if they are not namespace nodes.
     */
    if (nodes->nodeTab[i]->type != XML_NAMESPACE_DECL) {
      nodes->nodeTab[i] = NULL;
    }
  }
}
//------------------------------------------------------------------------------
int et_s1ap_update_mme_ue_s1ap_id(et_packet_t * const packet, const S1ap_MME_UE_S1AP_ID_t old_id, const S1ap_MME_UE_S1AP_ID_t new_id)
{
  xmlChar              xpath_expression[ET_XPATH_EXPRESSION_MAX_LENGTH];
  int                  ret       = 0;
  xmlDocPtr            doc       = NULL;
  xmlXPathContextPtr   xpath_ctx = NULL;
  xmlXPathObjectPtr    xpath_obj = NULL;

  //S1AP_DEBUG("%s() packet num %u original frame number %u, mme_ue_s1ap_id %u -> %u\n", __FUNCTION__, packet->packet_number, packet->original_frame_number, old_id, new_id);

  ret = snprintf(xpath_expression, ET_XPATH_EXPRESSION_MAX_LENGTH, "//field[@name=\"s1ap.MME_UE_S1AP_ID\"][@show=\"%u\"]", old_id);
  AssertFatal((ret > 0) && (ret < ET_XPATH_EXPRESSION_MAX_LENGTH), "Could not build XPATH expression err=%d", ret);

  doc = packet->sctp_hdr.u.data_hdr.payload.doc;
  // Create xpath evaluation context
  xpath_ctx = xmlXPathNewContext(doc);
  if(xpath_ctx == NULL) {
      fprintf(stderr,"Error: unable to create new XPath context\n");
      xmlFreeDoc(doc);
      return(-1);
  }

  // Evaluate xpath expression
  xpath_obj = xmlXPathEvalExpression(xpath_expression, xpath_ctx);
  xmlXPathFreeContext(xpath_ctx);
  AssertFatal(xpath_obj != NULL, "Unable to evaluate XPATH expression \"%s\"\n", xpath_expression);

  if(xmlXPathNodeSetIsEmpty(xpath_obj->nodesetval)){
    xmlXPathFreeObject(xpath_obj);
    S1AP_DEBUG("%s() No match \"%s\"packet num %u original frame number %u, mme_ue_s1ap_id %u -> %u\n",
        __FUNCTION__, xpath_expression, packet->packet_number, packet->original_frame_number, old_id, new_id);
    return -1;
  }
  // update selected nodes
  update_xpath_nodes_mme_ue_s1ap_id(&packet->sctp_hdr.u.data_hdr.payload, xpath_obj->nodesetval, new_id);

  // Cleanup of XPath data
  xmlXPathFreeObject(xpath_obj);

  return 0;
}
