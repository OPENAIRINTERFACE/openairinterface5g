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
#include "RRC2RRMMessageUserReconfigurationComplete.h"
#include "RadioResources.h"
#include "Exceptions.h"
#include "Utility.h"
//----------------------------------------------------------------------------
RRC2RRMMessageUserReconfigurationComplete::RRC2RRMMessageUserReconfigurationComplete(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRC_RRM_Message_t* asn1_messageP)
//----------------------------------------------------------------------------
{
    Message();
    ParseIpParameters(sa_fromP, sa_lenP);
    cout << "RRC2RRMMessageUserReconfigurationComplete(): m_ip_str_src = " << m_ip_str_src.c_str() << endl;

    m_asn1_message   = asn1_messageP;
    m_transaction_id = m_asn1_message->message.choice.c1.choice.rrcUserReconfigurationComplete.transactionId;
    m_cell_id        = m_asn1_message->message.choice.c1.choice.rrcUserReconfigurationComplete.e_NodeB_Identity.physCellId;
    m_mobile_id      = m_asn1_message->message.choice.c1.choice.rrcUserReconfigurationComplete.c_RNTI;
}
//----------------------------------------------------------------------------
RRC2RRMMessageUserReconfigurationComplete::RRC2RRMMessageUserReconfigurationComplete(std::string ip_dest_strP, int port_destP, cell_id_t cell_idP, mobile_id_t mobile_idP, transaction_id_t transaction_idP)
//----------------------------------------------------------------------------
{
    Message();
    m_asn1_message = static_cast<RRC_RRM_Message_t*>(CALLOC(1,sizeof(RRC_RRM_Message_t)));
    memset(m_asn1_message, 0, sizeof(RRC_RRM_Message_t));
    m_asn1_message->message.present                                                        = RRC_RRM_MessageType_PR_c1;
    m_asn1_message->message.choice.c1.present                                              = RRC_RRM_MessageType__c1_PR_rrcUserReconfigurationComplete;
    m_asn1_message->message.choice.c1.choice.rrcUserReconfigurationComplete.transactionId               = transaction_idP;
    m_asn1_message->message.choice.c1.choice.rrcUserReconfigurationComplete.e_NodeB_Identity.physCellId = cell_idP;
    m_asn1_message->message.choice.c1.choice.rrcUserReconfigurationComplete.c_RNTI                      = mobile_idP;


    m_transaction_id = transaction_idP;
    m_cell_id        = cell_idP;
    m_mobile_id      = mobile_idP;
    m_is_ipv6        = Utility::isipv6(ip_dest_strP);
    m_msg_length     = 0;
    Serialize();
}
//----------------------------------------------------------------------------
void RRC2RRMMessageUserReconfigurationComplete::Forward()
//----------------------------------------------------------------------------
{
    try {
        RadioResources::Instance()->Request(*this);
    } catch (std::exception const& e ) {

    }
}
//----------------------------------------------------------------------------
void RRC2RRMMessageUserReconfigurationComplete::Serialize()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        //xer_fprint(stdout, &asn_DEF_RRC_RRM_Message, (void*)m_asn1_message);
        char errbuf[256];
        size_t errlen = sizeof(errbuf);
        int ret = asn_check_constraints(&asn_DEF_RRC_RRM_Message, m_asn1_message, errbuf, &errlen);
        if (ret) {
            fprintf(stderr, "CONSTRAINT VALIDATION FAILED FOR RRM-RRC Message: %s\n", errbuf);
            exit (-1);
        }



        asn_enc_rval_t enc_rval;
        memset(&m_message_serialize_buffer[0], 0, MESSAGE_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_RRC_RRM_Message,
                    (void*)m_asn1_message,
                    &m_message_serialize_buffer[0],
                    MESSAGE_SERIALIZE_BUFFER_SIZE);
        printf("[RRM][RRC2RRMMessageUserReconfigurationComplete] asn_DEF_RRC_RRM_Message Encoded %d bits (%d bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

        m_msg_length = (enc_rval.encoded+7)/8;

        if (enc_rval.encoded==-1) {
            cerr << "[RRM][RRC2RRMMessageUserReconfigurationComplete] ASN1 : asn_DEF_RRC_RRM_Message encoding FAILED, EXITING" << endl;
            cerr << "[RRM][RRC2RRMMessageUserReconfigurationComplete] Cannot encode "<< enc_rval.failed_type->name <<" " << strerror(errno) << endl;
            throw asn1_encoding_error();
        }
        //xer_fprint(stdout, &asn_DEF_RRC_RRM_Message, (void*)m_asn1_message);
    }
}
//----------------------------------------------------------------------------
RRC2RRMMessageUserReconfigurationComplete::~RRC2RRMMessageUserReconfigurationComplete()
//----------------------------------------------------------------------------
{
    if (m_asn1_message != NULL) {
        cout << "[RRM][RRC2RRMMessageUserReconfigurationComplete::~RRC2RRMMessageUserReconfigurationComplete()] free asn1_message start" << endl;
        ASN_STRUCT_FREE( asn_DEF_RRC_RRM_Message, m_asn1_message);
        m_asn1_message = NULL;
        cout << "[RRM][RRC2RRMMessageUserReconfigurationComplete::~RRC2RRMMessageUserReconfigurationComplete()] free asn1_message end " << endl;
    }
}
