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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include "sctp_primitives_client.h"

#include "s1ap_common.h"
#include "s1ap_eNB.h"
#include "s1ap_mme.h"

char ip_addr[] = "127.0.0.1";

int assoc[2];
uint8_t id[] = { 0x03, 0x56, 0xf0, 0xd8 };
char identity[] = { 0x02, 0x08, 0x34 };
char tac[] = { 0x00, 0x01 };
char infoNAS[] = { 0x07, 0x42, 0x01, 0xE0, 0x06, 0x00, 0x00, 0xF1, 0x10, 0x00, 0x01, 0x00, 0x2C,
                   0x52, 0x01, 0xC1, 0x01, 0x09, 0x10, 0x03, 0x77, 0x77, 0x77, 0x07, 0x61, 0x6E, 0x72, 0x69, 0x74,
                   0x73, 0x75, 0x03, 0x63, 0x6F, 0x6D, 0x05, 0x01, 0x0A, 0x01, 0x20, 0x37, 0x27, 0x0E, 0x80, 0x80,
                   0x21, 0x0A, 0x03, 0x00, 0x00, 0x0A, 0x81, 0x06, 0x0A, 0x00, 0x00, 0x01, 0x50, 0x0B, 0xF6,
                   0x00, 0xF1, 0x10, 0x80, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01
                 };

uint32_t eNB_UE_S1AP_ID = 0;

int s1ap_eNB_generate_initial_ue_message(uint8_t **buffer,
    uint32_t *length);

int s1ap_eNB_generate_initial_ue_message(uint8_t **buffer,
    uint32_t *length)
{
  InitialUEMessageIEs  initialUEmessageIEs;
  InitialUEMessageIEs *initialUEmessageIEs_p = &initialUEmessageIEs;

  memset(initialUEmessageIEs_p, 0, sizeof(InitialUEMessageIEs));

  initialUEmessageIEs.eNB_UE_S1AP_ID = eNB_UE_S1AP_ID;
  initialUEmessageIEs.nas_pdu.buf = infoNAS;
  initialUEmessageIEs.nas_pdu.size = sizeof(infoNAS);

  initialUEmessageIEs.tai.tAC.buf = tac;
  initialUEmessageIEs.tai.tAC.size = 2;
  initialUEmessageIEs.tai.pLMNidentity.buf = identity;
  initialUEmessageIEs.tai.pLMNidentity.size = 3;
  initialUEmessageIEs.eutran_cgi.pLMNidentity.buf = identity;
  initialUEmessageIEs.eutran_cgi.pLMNidentity.size = 3;
  initialUEmessageIEs.eutran_cgi.cell_ID.buf = id;
  initialUEmessageIEs.eutran_cgi.cell_ID.size = 4;
  initialUEmessageIEs.eutran_cgi.cell_ID.bits_unused = 4;

  initialUEmessageIEs.rrC_Establishment_Cause = RRC_Establishment_Cause_mo_Data;

  return s1ap_eNB_encode_initial_ue_message(initialUEmessageIEs_p, buffer, length);
}

void recv_callback(uint8_t *buffer, uint32_t length)
{
  uint8_t *bufferS;
  uint32_t len;
  s1ap_eNB_generate_initial_ue_message(&bufferS, &len);
  sctp_send_msg(assoc[0], 1, bufferS, len);
}

int main(int argc, char *argv[])
{
  asn_enc_rval_t retVal;
  uint8_t *buffer;
  uint32_t len;

  SupportedTAs_Item_t ta;
  PLMNidentity_t plmnIdentity;

  asn_debug = 0;
  asn1_xer_print = 0;

  S1SetupRequestIEs s1SetupRequest;
  memset(&s1SetupRequest, 0, sizeof(S1SetupRequestIEs));
  s1SetupRequest.global_ENB_ID.eNB_ID.present = ENB_ID_PR_macroENB_ID;
  s1SetupRequest.global_ENB_ID.eNB_ID.choice.macroENB_ID.buf = id;
  s1SetupRequest.global_ENB_ID.eNB_ID.choice.macroENB_ID.size = 3;
  s1SetupRequest.global_ENB_ID.eNB_ID.choice.macroENB_ID.bits_unused = 4;
  OCTET_STRING_fromBuf(&s1SetupRequest.global_ENB_ID.pLMNidentity, identity, 3);

  s1SetupRequest.presenceMask |= S1SETUPREQUESTIES_ENBNAME_PRESENT;
  OCTET_STRING_fromBuf(&s1SetupRequest.eNBname, "ENB 1 eurecom",
                       strlen("ENB 1 eurecom"));

  memset(&ta, 0, sizeof(SupportedTAs_Item_t));
  memset(&plmnIdentity, 0, sizeof(PLMNidentity_t));
  OCTET_STRING_fromBuf(&ta.tAC, tac, 2);
  OCTET_STRING_fromBuf(&plmnIdentity, identity, 3);
  ASN_SEQUENCE_ADD(&ta.broadcastPLMNs, &plmnIdentity);
  ASN_SEQUENCE_ADD(&s1SetupRequest.supportedTAs, &ta);

  s1SetupRequest.defaultPagingDRX = PagingDRX_v64;

  s1ap_eNB_encode_s1setuprequest(&s1SetupRequest, &buffer, &len);

  assoc[0] = sctp_connect_to_remote_host(ip_addr, 36412, &recv_callback);
  sctp_send_msg(0, 0, buffer, len);

  free(buffer);

  //     generateUplinkNASTransport(&buffer, &len);
  //     sctp_send_msg(assoc[0], 0, buffer, len);
  //     s1ap_mme_decode_pdu(buffer, len);

  pthread_join(sctp_get_receiver_thread(assoc[0]), NULL);

  return(0);
}
