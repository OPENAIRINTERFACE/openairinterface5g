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
#include <exception>
#include <stdexcept>
#include <stdio.h>
//----------------------------------------------------------------------------
#include "RRC2RRMMessageAddUserConfirm.h"
#include "RRM2RRCMessageUserReconfiguration.h"
#include "RRCMessageHandler.h"
#include "RadioResources.h"
#include "Exceptions.h"
//----------------------------------------------------------------------------
RRC2RRMMessageAddUserConfirm::RRC2RRMMessageAddUserConfirm(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRC_RRM_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    ParseIpParameters(sa_fromP, sa_lenP);
    m_asn1_message   = asn1_messageP;
    m_transaction_id = m_asn1_message->message.choice.c1.choice.rrcAddUserConfirm.transactionId;
    m_cell_id        = m_asn1_message->message.choice.c1.choice.rrcAddUserConfirm.e_NodeB_Identity.physCellId;
    m_mobile_id      = m_asn1_message->message.choice.c1.choice.rrcAddUserConfirm.c_RNTI;
}
//----------------------------------------------------------------------------
RRC2RRMMessageAddUserConfirm::RRC2RRMMessageAddUserConfirm(std::string ip_dest_strP, int port_destP, cell_id_t cell_idP, mobile_id_t mobile_idP, transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_asn1_message = static_cast<RRC_RRM_Message_t*>(CALLOC(1,sizeof(RRC_RRM_Message_t)));
    memset(m_asn1_message, 0, sizeof(RRC_RRM_Message_t));
    m_asn1_message->message.present                                                        = RRC_RRM_MessageType_PR_c1;
    m_asn1_message->message.choice.c1.present                                              = RRC_RRM_MessageType__c1_PR_rrcAddUserConfirm;
    m_asn1_message->message.choice.c1.choice.rrcAddUserConfirm.transactionId               = transaction_idP;
    m_asn1_message->message.choice.c1.choice.rrcAddUserConfirm.e_NodeB_Identity.physCellId = cell_idP;
    m_asn1_message->message.choice.c1.choice.rrcAddUserConfirm.c_RNTI                      = mobile_idP;


    m_transaction_id = transaction_idP;
    m_cell_id        = cell_idP;
    m_mobile_id      = mobile_idP;
    m_is_ipv6        = Utility::isipv6(ip_dest_strP);
    m_msg_length     = 0;
    Serialize();
}
//----------------------------------------------------------------------------
void RRC2RRMMessageAddUserConfirm::Forward()
//----------------------------------------------------------------------------
{
    try {
        RRM2RRCMessageUserReconfiguration* user_reconfiguration = RadioResources::Instance()->Request(*this);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, user_reconfiguration->GetSerializedMessageBuffer(), user_reconfiguration->GetSerializedMessageSize());
        delete user_reconfiguration;
    } catch (std::exception const& e ) {
        RRM2RRCMessageUserReconfiguration  response(OpenAir_RRM_Response_Status_failure, OpenAir_RRM_Response_Reason_internalError, m_cell_id, m_mobile_id, m_transaction_id);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    }
}
//----------------------------------------------------------------------------
void RRC2RRMMessageAddUserConfirm::Serialize ()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        asn_enc_rval_t enc_rval;
        memset(&m_message_serialize_buffer[0], 0, MESSAGE_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_RRC_RRM_Message,
                    (void*)m_asn1_message,
                    &m_message_serialize_buffer[0],
                    MESSAGE_SERIALIZE_BUFFER_SIZE);
        cout << "[RRM][RRC2RRMMessageAddUserConfirm] asn_DEF_RRC_RRM_Message Encoded " << enc_rval.encoded << " bits (" << (enc_rval.encoded+7)/8 << " bytes)" << endl;

        m_msg_length = (enc_rval.encoded+7)/8;

        if (enc_rval.encoded==-1) {
            cerr << "[RRM][RRC2RRMMessageAddUserConfirm] ASN1 : asn_DEF_RRC_RRM_Message encoding FAILED, EXITING" << endl;
            throw asn1_encoding_error();
        }
    }
}
//----------------------------------------------------------------------------
RRC2RRMMessageAddUserConfirm::~RRC2RRMMessageAddUserConfirm()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        cout << "[RRM][RRC2RRMMessageAddUserConfirm::~RRC2RRMMessageAddUserConfirm()] free asn1_message start" << endl;
        ASN_STRUCT_FREE( asn_DEF_RRC_RRM_Message, m_asn1_message);
        cout << "[RRM][RRC2RRMMessageAddUserConfirm::~RRC2RRMMessageAddUserConfirm()] free asn1_message end" << endl;
    }
}

