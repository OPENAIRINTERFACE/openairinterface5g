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
#include "RRC2RRMMessageRemoveRadioAccessBearerRequest.h"
#include "RRM2RRCMessageRemoveRadioAccessBearerResponse.h"
#include "RRCMessageHandler.h"
#include "RadioResources.h"
#include "Exceptions.h"

//----------------------------------------------------------------------------
RRC2RRMMessageRemoveRadioAccessBearerRequest::RRC2RRMMessageRemoveRadioAccessBearerRequest(std::string ip_dest_strP, int port_destP,
                                                                                    cell_id_t cell_idP,
                                                                                     mobile_id_t mobile_idP,
                                                                                     rb_id_t            radio_bearer_idP,
                                                                                     transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_is_ipv6 = Utility::isipv6(ip_dest_strP);
    m_cell_id                     = cell_idP;
    m_mobile_id                   = mobile_idP;
    m_radio_bearer_id             = radio_bearer_idP;
    m_transaction_id              = transaction_idP;
}
//----------------------------------------------------------------------------
RRC2RRMMessageRemoveRadioAccessBearerRequest::RRC2RRMMessageRemoveRadioAccessBearerRequest(tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP):
Message(in_messageP)
//----------------------------------------------------------------------------
{
    ParseIpParameters(sa_fromP, sa_lenP);
    m_cell_id                     = m_message_storage.readChar();
    m_mobile_id                   = m_message_storage.readChar();
    m_radio_bearer_id             = m_message_storage.readChar();
    m_transaction_id              = m_message_storage.readChar();
    printf("----------------------------------------------------------------------------------------------------------\n");
    printf("RRC\t-------REMOVE DATA RADIO BEARER REQUEST------->\tRRM\n");
    printf("----------------------------------------------------------------------------------------------------------\n");
    printf("cell id = %d mobile id = %d radio bearer id = %d transaction id = %d \n", m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id);
}
//----------------------------------------------------------------------------
void RRC2RRMMessageRemoveRadioAccessBearerRequest::Forward()
//----------------------------------------------------------------------------
{
    try {
        Transaction* tx = RadioResources::Instance()->Request(*this);
        RRM2RRCMessageRemoveRadioAccessBearerResponse  response(Message::STATUS_REMOVE_RADIO_ACCESS_BEARER_SUCCESSFULL, Message::MSG_RESP_OK, m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id, tx);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    } catch (mobile_already_connected_error & x ) {
        RRM2RRCMessageRemoveRadioAccessBearerResponse  response(Message::STATUS_REMOVE_RADIO_ACCESS_BEARER_FAILED, Message::MSG_RESP_PROTOCOL_ERROR, m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    } catch (std::exception const& e ) {
        RRM2RRCMessageRemoveRadioAccessBearerResponse  response(Message::STATUS_REMOVE_RADIO_ACCESS_BEARER_FAILED, Message::MSG_RESP_INTERNAL_ERROR, m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    }
}
//----------------------------------------------------------------------------
void RRC2RRMMessageRemoveRadioAccessBearerRequest::Serialize()
//----------------------------------------------------------------------------
{
    m_message_storage.reset();
    m_message_storage.writeChar(Message::MESSAGE_REMOVE_RADIO_ACCESS_BEARER_REQUEST);
    m_message_storage.writeShort(7u);
    m_message_storage.writeChar(m_cell_id);
    m_message_storage.writeChar(m_mobile_id);
    m_message_storage.writeChar(m_radio_bearer_id);
    m_message_storage.writeChar(m_transaction_id);
}
//----------------------------------------------------------------------------
RRC2RRMMessageRemoveRadioAccessBearerRequest::~RRC2RRMMessageRemoveRadioAccessBearerRequest()
//----------------------------------------------------------------------------
{
}

