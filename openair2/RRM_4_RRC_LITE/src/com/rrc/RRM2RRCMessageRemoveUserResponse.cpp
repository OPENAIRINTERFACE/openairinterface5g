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
#include "RRM2RRCMessageRemoveUserResponse.h"
#include "RadioResources.h"
#include "Exceptions.h"
#include "TestScenario.h"
//----------------------------------------------------------------------------
RRM2RRCMessageRemoveUserResponse::RRM2RRCMessageRemoveUserResponse(tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP):Message(in_messageP)
//----------------------------------------------------------------------------
{
    ParseIpParameters(sa_fromP, sa_lenP);
    m_status         = m_message_storage.readChar();
    m_reason         = m_message_storage.readChar();
    m_cell_id        = m_message_storage.readChar();
    m_mobile_id      = m_message_storage.readChar();
    m_transaction_id = m_message_storage.readChar();
    m_transaction_result = NULL;
    cout << "----------------------------------------------------------------------------------------------------------" << endl;
    cout << "RRC\t<-------REMOVE USER RESPONSE-------\tRRM  STATUS: " << Message::StatusToString(m_status).c_str() << "  REASON: " << Message::ReasonToString(m_reason).c_str() << endl;
    cout << "----------------------------------------------------------------------------------------------------------"<< endl;
    cout << "cell id = "<< m_cell_id << " mobile id = " << m_mobile_id << " transaction id = " << m_transaction_id << endl;
    if ((msg_lengthP - 5) > 0 ) {
        m_transaction_result = new Transaction(m_transaction_id, m_message_storage, msg_lengthP - 5);
        printf("%s\n",m_transaction_result->ToString().c_str());
    }
}
//----------------------------------------------------------------------------
RRM2RRCMessageRemoveUserResponse::RRM2RRCMessageRemoveUserResponse(msg_response_status_t statusP,
                                                                                       msg_response_reason_t reasonP,
                                                                                       cell_id_t cell_idP,
                                                                                       mobile_id_t mobile_idP,
                                                                                       transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_status           = statusP;
    m_reason           = reasonP;
    m_cell_id          = cell_idP;
    m_mobile_id        = mobile_idP;
    m_transaction_id   = transaction_idP;
}
//----------------------------------------------------------------------------
RRM2RRCMessageRemoveUserResponse::RRM2RRCMessageRemoveUserResponse(msg_response_status_t statusP,
                                                                                       msg_response_reason_t reasonP,
                                                                                       cell_id_t cell_idP,
                                                                                       mobile_id_t mobile_idP,
                                                                                       transaction_id_t transaction_idP,
                                                                                       Transaction* transactionP)
//----------------------------------------------------------------------------
{
    Message();
    m_status           = statusP;
    m_reason           = reasonP;
    m_cell_id          = cell_idP;
    m_mobile_id        = mobile_idP;
    m_transaction_id   = transaction_idP;
    m_transaction_result = transactionP;
}

//----------------------------------------------------------------------------
void RRM2RRCMessageRemoveUserResponse::Serialize ()
//----------------------------------------------------------------------------
{
//    printf("[RRM][RRM2RRCMessageRemoveUserResponse] Serialize () status = %d reason = %d cell id = %d mobile id = %d transaction id = %d \n",  m_status, m_reason, m_cell_id, m_mobile_id, m_transaction_id);
    int tx_packet_size = 0;
    if (m_transaction_result != NULL) {
        tx_packet_size = m_transaction_result->Size();
    }
    m_message_storage.reset();
    m_message_storage.writeChar(Message::MESSAGE_REMOVE_USER_RESPONSE);
    m_message_storage.writeShort(8u+tx_packet_size);
    m_message_storage.writeChar(m_status);
    m_message_storage.writeChar(m_reason);
    m_message_storage.writeChar(m_cell_id);
    m_message_storage.writeChar(m_mobile_id);
    m_message_storage.writeChar(m_transaction_id);
    if (m_transaction_result != NULL) {
        unsigned char packet[tx_packet_size];
        m_transaction_result->Serialize(&packet[0]);
        m_message_storage.writePacket(&packet[0], tx_packet_size);
    }
}
//----------------------------------------------------------------------------
void RRM2RRCMessageRemoveUserResponse::Forward()
//----------------------------------------------------------------------------
{
    #ifdef TEST_SCENARIO
    TestScenario::notifyRxMessage(this);
    #endif
}
//----------------------------------------------------------------------------
RRM2RRCMessageRemoveUserResponse::~RRM2RRCMessageRemoveUserResponse()
//----------------------------------------------------------------------------
{
    if (m_transaction_result != NULL) {
        delete m_transaction_result;
    }
}
