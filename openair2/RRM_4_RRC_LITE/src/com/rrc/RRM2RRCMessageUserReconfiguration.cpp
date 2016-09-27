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
#include "RRM2RRCMessageUserReconfiguration.h"
#include "RadioResources.h"
#include "Exceptions.h"
#include "TestScenario.h"
//----------------------------------------------------------------------------
RRM2RRCMessageUserReconfiguration::RRM2RRCMessageUserReconfiguration(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRM_RRC_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    ParseIpParameters(sa_fromP, sa_lenP);
    cout << "RRM2RRCMessageUserReconfiguration(): m_ip_str_src = " << m_ip_str_src.c_str() << endl;

    m_asn1_message   = asn1_messageP;
    m_status         = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.rrm_Response_Status;
    m_reason         = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.rrm_Response_Reason;
    m_transaction_id = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.transactionId;
    m_cell_id        = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.e_NodeB_Identity.physCellId;
    m_mobile_id      = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.c_RNTI;
}

//----------------------------------------------------------------------------
RRM2RRCMessageUserReconfiguration::RRM2RRCMessageUserReconfiguration(transaction_id_t transaction_idP, msg_response_status_t statusP, msg_response_reason_t reasonP, RRM_RRC_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    m_status         = statusP;
    m_reason         = reasonP;
    m_transaction_id = transaction_idP;

    m_asn1_message   = asn1_messageP;
    m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.transactionId       = transaction_idP;
    m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.rrm_Response_Status = statusP;
    m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.rrm_Response_Reason = reasonP;

    m_cell_id        = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.e_NodeB_Identity.physCellId;
    m_mobile_id      = m_asn1_message->message.choice.c1.choice.rrcUserReconfiguration.c_RNTI;
    Serialize();
}
//----------------------------------------------------------------------------
RRM2RRCMessageUserReconfiguration::RRM2RRCMessageUserReconfiguration(msg_response_status_t statusP, msg_response_reason_t reasonP, cell_id_t cell_idP, mobile_id_t mobile_idP, transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_status         = statusP;
    m_reason         = reasonP;
    m_cell_id        = cell_idP;
    m_mobile_id      = mobile_idP;
    m_transaction_id = transaction_idP;
}
//----------------------------------------------------------------------------
void RRM2RRCMessageUserReconfiguration::Forward()
//----------------------------------------------------------------------------
{
    #ifdef TEST_SCENARIO
    TestScenario::notifyRxMessage(this);
    #endif
}
//----------------------------------------------------------------------------
void RRM2RRCMessageUserReconfiguration::Serialize()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        //xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)m_asn1_message);
        char errbuf[256];
        size_t errlen = sizeof(errbuf);
        int ret = asn_check_constraints(&asn_DEF_RRM_RRC_Message, m_asn1_message, errbuf, &errlen);
        if (ret) {
            fprintf(stderr, "CONSTRAINT VALIDATION FAILED FOR RRM-RRC Message: %s\n", errbuf);
            exit (-1);
        }



        asn_enc_rval_t enc_rval;
        memset(&m_message_serialize_buffer[0], 0, MESSAGE_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_RRM_RRC_Message,
                    (void*)m_asn1_message,
                    &m_message_serialize_buffer[0],
                    MESSAGE_SERIALIZE_BUFFER_SIZE);
        printf("[RRM][RRM2RRCMessageUserReconfiguration] asn_DEF_RRM_RRC_Message Encoded %d bits (%d bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

        m_msg_length = (enc_rval.encoded+7)/8;

        if (enc_rval.encoded==-1) {
            cerr << "[RRM][RRM2RRCMessageUserReconfiguration] ASN1 : asn_DEF_RRM_RRC_Message encoding FAILED, EXITING" << endl;
            cerr << "[RRM][RRM2RRCMessageUserReconfiguration] Cannot encode "<< enc_rval.failed_type->name <<" " << strerror(errno) << endl;
            throw asn1_encoding_error();
        }
        //xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)m_asn1_message);
    }
}
//----------------------------------------------------------------------------
RRM2RRCMessageUserReconfiguration::~RRM2RRCMessageUserReconfiguration()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        cout << "[RRM][RRM2RRCMessageUserReconfiguration::~RRM2RRCMessageUserReconfiguration()] free asn1_message start" << endl;
        ASN_STRUCT_FREE( asn_DEF_RRM_RRC_Message, m_asn1_message);
        m_asn1_message = NULL;
        cout << "[RRM][RRM2RRCMessageUserReconfiguration::~RRM2RRCMessageUserReconfiguration()] free asn1_message end " << endl;
    }
}
