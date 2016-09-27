/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include "RRC2RRMMessageConnectionRequest.h"
#include "RRM2RRCMessageConnectionResponse.h"
#include "RRCMessageHandler.h"
#include "RadioResources.h"
#include "Exceptions.h"
#include "Utility.h"

using namespace std;

//----------------------------------------------------------------------------
RRC2RRMMessageConnectionRequest::RRC2RRMMessageConnectionRequest(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRC_RRM_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    ParseIpParameters(sa_fromP, sa_lenP);
    cout << "RRC2RRMMessageConnectionRequest(): m_ip_str_src = " << m_ip_str_src.c_str() << endl;

    m_asn1_message   = asn1_messageP;
    m_transaction_id = m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationRequest.transactionId;
}
//----------------------------------------------------------------------------
RRC2RRMMessageConnectionRequest::RRC2RRMMessageConnectionRequest(std::string ip_dest_strP, int port_destP, transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_asn1_message = static_cast<RRC_RRM_Message_t*>(CALLOC(1,sizeof(RRC_RRM_Message_t)));
    m_asn1_message->message.present                                                      = RRC_RRM_MessageType_PR_c1;
    m_asn1_message->message.choice.c1.present                                            = RRC_RRM_MessageType__c1_PR_rrcSystemConfigurationRequest;
    m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationRequest.transactionId = transaction_idP;

    m_transaction_id = transaction_idP;
    m_is_ipv6        = Utility::isipv6(ip_dest_strP);
    m_msg_length     = 0;
    Serialize();
}
//----------------------------------------------------------------------------
void RRC2RRMMessageConnectionRequest::Forward()
//----------------------------------------------------------------------------
{
    try {

        RRM2RRCMessageConnectionResponse* connection_response = RadioResources::Instance()->Request(*this);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, connection_response->GetSerializedMessageBuffer(), connection_response->GetSerializedMessageSize());
        delete connection_response;
    } catch (enodeb_already_connected_error & x ) {
        RRM2RRCMessageConnectionResponse  connection_response(OpenAir_RRM_Response_Status_failure, OpenAir_RRM_Response_Reason_protocolError);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, connection_response.GetSerializedMessageBuffer(), connection_response.GetSerializedMessageSize());
    } catch (std::exception const& e ) {
        RRM2RRCMessageConnectionResponse  connection_response(OpenAir_RRM_Response_Status_failure, OpenAir_RRM_Response_Reason_internalError);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, connection_response.GetSerializedMessageBuffer(), connection_response.GetSerializedMessageSize());
    }
}
//----------------------------------------------------------------------------
void RRC2RRMMessageConnectionRequest::Serialize()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        asn_enc_rval_t enc_rval;
        memset(&m_message_serialize_buffer[0], 0, MESSAGE_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_RRC_RRM_Message,
                    (void*)m_asn1_message,
                    &m_message_serialize_buffer[0],
                    MESSAGE_SERIALIZE_BUFFER_SIZE);
        printf("[RRM][RRC2RRMMessageConnectionRequest] asn_DEF_RRM_RRC_Message Encoded %d bits (%d bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

        m_msg_length = (enc_rval.encoded+7)/8;

        if (enc_rval.encoded==-1) {
            printf("[RRM][RRC2RRMMessageConnectionRequest] ASN1 : asn_DEF_RRC_RRM_Message encoding FAILED, EXITING\n");
            throw asn1_encoding_error();
        }
    }
}
//----------------------------------------------------------------------------
RRC2RRMMessageConnectionRequest::~RRC2RRMMessageConnectionRequest()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        ASN_STRUCT_FREE( asn_DEF_RRC_RRM_Message, m_asn1_message);
    }
}

