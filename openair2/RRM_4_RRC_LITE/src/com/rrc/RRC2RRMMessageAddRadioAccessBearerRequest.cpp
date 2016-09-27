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

#include <stdio.h>

#include "RRC2RRMMessageAddRadioAccessBearerRequest.h"
#include "RRM2RRCMessageAddRadioAccessBearerResponse.h"
#include "RRCMessageHandler.h"
#include "RadioResources.h"
#include "Exceptions.h"

//----------------------------------------------------------------------------
RRC2RRMMessageAddRadioAccessBearerRequest::RRC2RRMMessageAddRadioAccessBearerRequest(std::string ip_dest_strP, int port_destP,
                                                                                    cell_id_t cell_idP,
                                                                                     mobile_id_t mobile_idP,
                                                                                     rb_id_t            radio_bearer_idP,
                                                                                     unsigned short     traffic_classP,
                                                                                     unsigned short     delayP,
                                                                                     unsigned int       guaranted_bit_rate_uplinkP,
                                                                                     unsigned int       max_bit_rate_uplinkP,
                                                                                     unsigned int       guaranted_bit_rate_downlinkP,
                                                                                     unsigned int       max_bit_rate_downlinkP,
                                                                                     transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_is_ipv6 = Utility::isipv6(ip_dest_strP);
    m_cell_id                     = cell_idP;
    m_mobile_id                   = mobile_idP;
    m_radio_bearer_id             = radio_bearer_idP;
    m_traffic_class               = traffic_classP;
    m_delay                       = delayP;
    m_guaranted_bit_rate_uplink   = guaranted_bit_rate_uplinkP;
    m_max_bit_rate_uplink         = max_bit_rate_uplinkP;
    m_guaranted_bit_rate_downlink = guaranted_bit_rate_downlinkP;
    m_max_bit_rate_downlink       = max_bit_rate_downlinkP;
    m_transaction_id              = transaction_idP;
}
//----------------------------------------------------------------------------
RRC2RRMMessageAddRadioAccessBearerRequest::RRC2RRMMessageAddRadioAccessBearerRequest(tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP):
Message(in_messageP)
//----------------------------------------------------------------------------
{
    ParseIpParameters(sa_fromP, sa_lenP);
    m_cell_id                     = m_message_storage.readChar();
    m_mobile_id                   = m_message_storage.readChar();
    m_radio_bearer_id             = m_message_storage.readChar();
    m_traffic_class               = m_message_storage.readShort();
    m_delay                       = m_message_storage.readShort();
    m_guaranted_bit_rate_uplink   = m_message_storage.readInt();
    m_max_bit_rate_uplink         = m_message_storage.readInt();
    m_guaranted_bit_rate_downlink = m_message_storage.readInt();
    m_max_bit_rate_downlink       = m_message_storage.readInt();
    m_transaction_id              = m_message_storage.readChar();
    printf("----------------------------------------------------------------------------------------------------------\n");
    printf("RRC\t-------ADD DATA RADIO BEARER REQUEST------->\tRRM\n");
    printf("----------------------------------------------------------------------------------------------------------\n");
    printf("cell id = %d mobile id = %d radio bearer id = %d transaction id = %d \n", m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id);
}
//----------------------------------------------------------------------------
void RRC2RRMMessageAddRadioAccessBearerRequest::Forward()
//----------------------------------------------------------------------------
{
    try {
        Transaction* tx = RadioResources::Instance()->Request(*this);
        RRM2RRCMessageAddRadioAccessBearerResponse  response(Message::STATUS_ADD_RADIO_ACCESS_BEARER_SUCCESSFULL, Message::MSG_RESP_OK, m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id, tx);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    } catch (mobile_already_connected_error & x ) {
        RRM2RRCMessageAddRadioAccessBearerResponse  response(Message::STATUS_ADD_RADIO_ACCESS_BEARER_FAILED, Message::MSG_RESP_PROTOCOL_ERROR, m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    } catch (std::exception const& e ) {
        RRM2RRCMessageAddRadioAccessBearerResponse  response(Message::STATUS_ADD_RADIO_ACCESS_BEARER_FAILED, Message::MSG_RESP_INTERNAL_ERROR, m_cell_id, m_mobile_id, m_radio_bearer_id, m_transaction_id);
        RRCMessageHandler::Instance()->Send2Peer(this->m_ip_str_src, this->m_port_src, response.GetSerializedMessageBuffer(), response.GetSerializedMessageSize());
    }
}
//----------------------------------------------------------------------------
void RRC2RRMMessageAddRadioAccessBearerRequest::Serialize()
//----------------------------------------------------------------------------
{
    m_message_storage.reset();
    m_message_storage.writeChar(Message::MESSAGE_ADD_RADIO_ACCESS_BEARER_REQUEST);
    m_message_storage.writeShort(27u);
    m_message_storage.writeChar(m_cell_id);
    m_message_storage.writeChar(m_mobile_id);
    m_message_storage.writeChar(m_radio_bearer_id);
    m_message_storage.writeShort(m_traffic_class);
    m_message_storage.writeShort(m_delay);
    m_message_storage.writeInt(m_guaranted_bit_rate_uplink);
    m_message_storage.writeInt(m_max_bit_rate_uplink);
    m_message_storage.writeInt(m_guaranted_bit_rate_downlink);
    m_message_storage.writeInt( m_max_bit_rate_downlink);
    m_message_storage.writeChar(m_transaction_id);
}
//----------------------------------------------------------------------------
RRC2RRMMessageAddRadioAccessBearerRequest::~RRC2RRMMessageAddRadioAccessBearerRequest()
//----------------------------------------------------------------------------
{
}

