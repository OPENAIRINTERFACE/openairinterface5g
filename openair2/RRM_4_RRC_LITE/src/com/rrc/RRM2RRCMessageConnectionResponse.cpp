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
//----------------------------------------------------------------------------
#include "RRM2RRCMessageConnectionResponse.h"
#include "RadioResources.h"
#include "Exceptions.h"
#include "TestScenario.h"
//----------------------------------------------------------------------------
RRM2RRCMessageConnectionResponse::RRM2RRCMessageConnectionResponse(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRM_RRC_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    ParseIpParameters(sa_fromP, sa_lenP);
    cout << "RRM2RRCMessageConnectionResponse(): m_ip_str_src = " << m_ip_str_src.c_str() << endl;

    m_asn1_message   = asn1_messageP;
    m_status         = m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.rrm_Response_Status;
    m_reason         = m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.rrm_Response_Reason;
    m_transaction_id = m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.transactionId;
    m_cell_id        = m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.e_NodeB_Identity.physCellId;
}

//----------------------------------------------------------------------------
RRM2RRCMessageConnectionResponse::RRM2RRCMessageConnectionResponse(transaction_id_t transaction_idP, msg_response_status_t statusP, msg_response_reason_t reasonP, RRM_RRC_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    m_status         = statusP;
    m_reason         = reasonP;
    m_transaction_id = transaction_idP;

    m_asn1_message   = asn1_messageP;
    m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.transactionId       = transaction_idP;
    m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.rrm_Response_Status = statusP;
    m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.rrm_Response_Reason = reasonP;

    m_cell_id        = m_asn1_message->message.choice.c1.choice.rrcSystemConfigurationResponse.e_NodeB_Identity.physCellId;
    Serialize();
}
//----------------------------------------------------------------------------
RRM2RRCMessageConnectionResponse::RRM2RRCMessageConnectionResponse(msg_response_status_t statusP, msg_response_reason_t reasonP, cell_id_t cell_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_status  = statusP;
    m_reason  = reasonP;
    m_cell_id = cell_idP;
}
//----------------------------------------------------------------------------
void RRM2RRCMessageConnectionResponse::Forward()
//----------------------------------------------------------------------------
{
    cout << "[RRM][RRM2RRCMessageConnectionResponse] status = " << m_status << " reason = " << m_reason << " cell id = " <<  m_cell_id << endl;
#ifdef TEST_SCENARIO
    TestScenario::notifyRxMessage(this);
#endif
}
//----------------------------------------------------------------------------
void RRM2RRCMessageConnectionResponse::Serialize()
//----------------------------------------------------------------------------
{

    if (m_asn1_message != NULL) {
        //xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)m_asn1_message);
        char errbuf[128];
        size_t errlen = sizeof(errbuf);
        int ret = asn_check_constraints(&asn_DEF_RRM_RRC_Message, m_asn1_message, errbuf, &errlen);
        if (ret) {
            cerr << "[RRM][RRM2RRCMessageConnectionResponse] CONSTRAINT VALIDATION FAILED FOR RRM-RRC Message: " <<  errbuf << endl;
            exit (-1);
        }



        asn_enc_rval_t enc_rval;
        memset(&m_message_serialize_buffer[0], 0, MESSAGE_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_RRM_RRC_Message,
                    (void*)m_asn1_message,
                    &m_message_serialize_buffer[0],
                    MESSAGE_SERIALIZE_BUFFER_SIZE);
        cout << "[RRM][RRM2RRCMessageConnectionResponse] asn_DEF_RRM_RRC_Message Encoded " << enc_rval.encoded << " bits (" << (enc_rval.encoded+7)/8 << " bytes)" << endl;

        m_msg_length = (enc_rval.encoded+7)/8;

        if (enc_rval.encoded==-1) {
            cout << "[RRM][RRM2RRCMessageConnectionResponse] ASN1 : asn_DEF_RRM_RRC_Message encoding FAILED, EXITING" << endl;
            throw asn1_encoding_error();
        }
        //xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)m_asn1_message);
    }
}
//----------------------------------------------------------------------------
RRM2RRCMessageConnectionResponse::~RRM2RRCMessageConnectionResponse()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        ASN_STRUCT_FREE( asn_DEF_RRM_RRC_Message, m_asn1_message);
    }
}

