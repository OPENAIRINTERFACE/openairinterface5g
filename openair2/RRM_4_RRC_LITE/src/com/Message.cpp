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
#include <cassert>
#include <string.h>
#include <arpa/inet.h>
//-----------------------------------------------------------------
#include "Message.h"
#include "RRC-RRM-Message.h"
#include "RRM-RRC-Message.h"
#include "Utility.h"
#include "RRC2RRMMessageConnectionRequest.h"
#include "RRC2RRMMessageAddUserRequest.h"
#include "RRC2RRMMessageAddUserConfirm.h"
#include "RRC2RRMMessageAddRadioAccessBearerRequest.h"
#include "RRC2RRMMessageRemoveRadioAccessBearerRequest.h"
#include "RRC2RRMMessageRemoveUserRequest.h"
#include "RRC2RRMMessageAddRadioAccessBearerConfirm.h"
#include "RRM2RRCMessageConnectionResponse.h"
#include "RRM2RRCMessageAddUserResponse.h"
#include "RRM2RRCMessageAddRadioAccessBearerResponse.h"
#include "RRM2RRCMessageRemoveRadioAccessBearerResponse.h"
#include "RRM2RRCMessageRemoveUserResponse.h"
#include "RRM2RRCMessageUserReconfiguration.h"
#include "RRC2RRMMessageUserReconfigurationComplete.h"
#include "storage.h"
uint8_t Message::s_buffer[MESSAGE_DESERIALIZE_BUFFER_SIZE];

//-----------------------------------------------------------------
#define TRACE_RRM_MESSAGES
#define LOOP_TEST
//-----------------------------------------------------------------
using namespace std;
using namespace tcpip;
//-----------------------------------------------------------------
Message::Message()
//-----------------------------------------------------------------
{
    m_message_buffer = NULL;
    m_ip_char_src = new char(INET6_ADDRSTRLEN);
}
//-----------------------------------------------------------------
Message::Message(tcpip::Storage& in_messageP): m_message_storage(in_messageP)
//-----------------------------------------------------------------
{
    m_message_buffer = NULL;
    m_ip_char_src = new char(INET6_ADDRSTRLEN);
}
//-----------------------------------------------------------------
Message* Message::DeserializeRRCMessage(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP)
//-----------------------------------------------------------------
{
    Message*           message = NULL;
    asn_dec_rval_t     rval;
    asn_codec_ctx_t   *opt_codec_ctx = 0;
    RRC_RRM_Message_t  *asn1_message = 0;
#ifdef LOOP_TEST
    for (int i=0; i<10000; i++) {
    message = NULL;
    opt_codec_ctx = 0;
    asn1_message = 0;

    rval = uper_decode(opt_codec_ctx,
                       &asn_DEF_RRC_RRM_Message,/* Type to decode */
                       (void **)&asn1_message,     /* Pointer to a target structure's pointer */
                       in_bufferP,              /* Data to be decoded */
                       size_dataP,              /* Size of data buffer */
                       0,                       /* Number of unused leading bits, 0..7 */
                       0);   /* Number of unused tailing bits, 0..7 */
    ASN_STRUCT_FREE(asn_DEF_RRC_RRM_Message, asn1_message);
    }
    cout << "Message::DeserializeRRCMessage(): LOOP TEST END" << endl;
    opt_codec_ctx = 0;
    asn1_message = 0;
#endif

    rval = uper_decode(opt_codec_ctx,
                       &asn_DEF_RRC_RRM_Message,/* Type to decode */
                       (void **)&asn1_message,     /* Pointer to a target structure's pointer */
                       in_bufferP,              /* Data to be decoded */
                       size_dataP,              /* Size of data buffer */
                       0,                       /* Number of unused leading bits, 0..7 */
                       0);   /* Number of unused tailing bits, 0..7 */

    xer_fprint(stdout, &asn_DEF_RRC_RRM_Message, (void*)asn1_message);
    if (rval.code != RC_OK) {
        cerr << "    DeserializeRRCMessage   ERROR IN ASN1 DECODING" << endl;
        return NULL;
    }
    if (asn1_message->message.present == RRC_RRM_MessageType_PR_c1) {
        switch (asn1_message->message.choice.c1.present) {
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_NOTHING:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_NOTHING" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcSystemConfigurationRequest:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_SYSTEM_CONFIGURATION_REQUEST" << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageConnectionRequest(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcAddUserRequest:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_ADD_USER_REQUEST" << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageAddUserRequest(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcAddUserConfirm:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_ADD_USER_CONFIRM" << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageAddUserConfirm(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcUserReconfigurationComplete:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_USER_RECONFIGURATION_COMPLETE" << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageUserReconfigurationComplete(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcAddDataRadioBearerRequest:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_ADD_DATA_RADIO_BEARER_REQUEST" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcAddDataRadioBearerConfirm:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_ADD_DATA_RADIO_BEARER_CONFIRM" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcRemoveUserRequest:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_REMOVE_USER_REQUEST" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcRemoveDataRadioBearerRequest:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_REMOVE_DATA_RADIO_BEARER_REQUEST" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
            //-------------------------------
            case RRC_RRM_MessageType__c1_PR_rrcRemoveDataRadioBearerConfirm:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_REMOVE_DATA_RADIO_BEARER_CONFIRM" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
        }
    } else {
        cerr << "    DeserializeRRMMessage   ERROR IN message.present field" << endl;
        return NULL;
    }
    return message;
}
//-----------------------------------------------------------------
Message* Message::DeserializeRRMMessage(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP)
//-----------------------------------------------------------------
{
    Message*           message = NULL;
    asn_dec_rval_t     rval;
    asn_codec_ctx_t   *opt_codec_ctx = 0;
    RRM_RRC_Message_t  *asn1_message = 0;
#ifdef LOOP_TEST
    for (int i=0; i<10000; i++) {
    message = NULL;
    opt_codec_ctx = 0;
    asn1_message = 0;

    rval = uper_decode(opt_codec_ctx,
                       &asn_DEF_RRM_RRC_Message,/* Type to decode */
                       (void **)&asn1_message,     /* Pointer to a target structure's pointer */
                       in_bufferP,              /* Data to be decoded */
                       size_dataP,              /* Size of data buffer */
                       0,                       /* Number of unused leading bits, 0..7 */
                       0);   /* Number of unused tailing bits, 0..7 */
    ASN_STRUCT_FREE(asn_DEF_RRM_RRC_Message, asn1_message);
    }
    cout << "Message::DeserializeRRMMessage(): LOOP TEST END" << endl;
    opt_codec_ctx = 0;
    asn1_message = 0;
#endif
    rval = uper_decode(opt_codec_ctx,
                       &asn_DEF_RRM_RRC_Message,/* Type to decode */
                       (void **)&asn1_message,     /* Pointer to a target structure's pointer */
                       in_bufferP,              /* Data to be decoded */
                       size_dataP,              /* Size of data buffer */
                       0,                       /* Number of unused leading bits, 0..7 */
                       0);   /* Number of unused tailing bits, 0..7 */

    xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)asn1_message);
    if (rval.code != RC_OK) {
        cerr << "    DeserializeRRMMessage   ERROR IN ASN1 DECODING" << endl;
        return NULL;
    }
    if (asn1_message->message.present == RRM_RRC_MessageType_PR_c1) {
        switch (asn1_message->message.choice.c1.present) {
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_NOTHING:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_NOTHING" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(asn1_message);
            break;
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_rrcSystemConfigurationResponse:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_SYSTEM_CONFIGURATION_RESPONSE" << endl;
            cout << dec;
            #endif
            message = new RRM2RRCMessageConnectionResponse(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_rrcAddUserResponse:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_ADD_USER_RESPONSE" << endl;
            cout << dec;
            #endif
            message = new RRM2RRCMessageAddUserResponse(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_rrcUserReconfiguration:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_USER_RECONFIGURATION" << endl;
            cout << dec;
            #endif
            message = new RRM2RRCMessageUserReconfiguration(sa_fromP, sa_lenP, asn1_message);
            break;
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_rrcAddDataRadioBearerResponse:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_ADD_DATA_RADIO_BEARER_RESPONSE" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionResponse(asn1_message);
            break;
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_rrcRemoveUserResponse:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_REMOVE_USER_RESPONSE" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionResponse(asn1_message);
            break;
            //-------------------------------
            case RRM_RRC_MessageType__c1_PR_rrcRemoveDataRadioBearerResponse:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize(): MESSAGE_REMOVE_DATA_RADIO_BEARER_RESPONSE" << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionResponse(asn1_message);
            break;

        }
    } else {
        cerr << "    DeserializeRRMMessage   ERROR IN message.present field" << endl;
        return NULL;
    }
    return message;
}
//-----------------------------------------------------------------
Message* Message::Deserialize(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP)
//-----------------------------------------------------------------
{

    msg_type_t     msg_type;
    msg_length_t   msg_length;
    frame_t        msg_frame;

    Storage        in_message(const_cast<unsigned char*>(reinterpret_cast<const unsigned char *>(in_bufferP)), size_dataP);
    msg_type     = in_message.readChar();
    msg_length   = in_message.readShort();
    //msg_frame    = in_message.readInt();
    assert(msg_length == size_dataP);

    msg_length   = msg_length - 1 - 2;

    Message* message = NULL;
    switch (msg_type) {
        //-------------------------------
        case MESSAGE_CONNECTION_REQUEST:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_CONNECTION_REQUEST"  << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageConnectionRequest(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_CONNECTION_RESPONSE:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_CONNECTION_RESPONSE"  << endl;
            cout << dec;
            #endif
            //message = new RRM2RRCMessageConnectionResponse(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_ADD_USER_REQUEST:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_ADD_USER_REQUEST"  << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageAddUserRequest(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_ADD_USER_CONFIRM:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_ADD_USER_CONFIRM"  << endl;
            cout << dec;
            #endif
            //message = new RRC2RRMMessageAddUserConfirm(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_ADD_USER_RESPONSE:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_ADD_USER_RESPONSE"  << endl;
            cout << dec;
            #endif
            //message = new RRM2RRCMessageAddUserResponse(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_ADD_RADIO_ACCESS_BEARER_REQUEST:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_ADD_RADIO_ACCESS_BEARER_REQUEST"  << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageAddRadioAccessBearerRequest(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_ADD_RADIO_ACCESS_BEARER_CONFIRM:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_ADD_RADIO_ACCESS_BEARER_CONFIRM"  << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageAddRadioAccessBearerConfirm(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_ADD_RADIO_ACCESS_BEARER_RESPONSE:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_ADD_RADIO_ACCESS_BEARER_RESPONSE"  << endl;
            cout << dec;
            #endif
            message = new RRM2RRCMessageAddRadioAccessBearerResponse(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_REMOVE_RADIO_ACCESS_BEARER_REQUEST:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_REMOVE_RADIO_ACCESS_BEARER_REQUEST"  << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageRemoveRadioAccessBearerRequest(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_REMOVE_RADIO_ACCESS_BEARER_RESPONSE:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_REMOVE_RADIO_ACCESS_BEARER_RESPONSE"  << endl;
            cout << dec;
            #endif
            message = new RRM2RRCMessageRemoveRadioAccessBearerResponse(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_REMOVE_USER_REQUEST:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_REMOVE_USER_REQUEST"  << endl;
            cout << dec;
            #endif
            message = new RRC2RRMMessageRemoveUserRequest(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        case MESSAGE_REMOVE_USER_RESPONSE:
            //-------------------------------
            #ifdef TRACE_RRM_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type               : MESSAGE_REMOVE_USER_RESPONSE"  << endl;
            cout << dec;
            #endif
            message = new RRM2RRCMessageRemoveUserResponse(in_message, msg_length, msg_frame, sa_fromP, sa_lenP);
            break;

            //-------------------------------
        default:
            //-------------------------------
            #ifdef TRACE_RRM_UMTS_MESSAGES
            cout << hex;
            cout << "Message::deserialize():" << endl;
            cout << "    Type NOT HANDLED     : 0x" << static_cast<unsigned int>(msg_type) << endl;
            cout << dec;
            #endif
            ;
    }
    return message;
}
//-----------------------------------------------------------------
void Message::ParseIpParameters(struct sockaddr *sa_fromP,socklen_t sa_lenP)
//-----------------------------------------------------------------
{

    if (sa_lenP == sizeof(struct sockaddr_in6)) {
        m_is_ipv6 = true;
        memcpy(&m_sockaddr_in6_src,sa_fromP,sa_lenP);
        Utility::l2ip(m_sockaddr_in6_src.sin6_addr, m_ip_str_src,0);
        //const char* result = inet_ntop(AF_INET6, &m_sockaddr_in6_src,  m_ip_char_src, sa_lenP);
        //cout << "inet_ntop returned" <<  result << endl;
        m_port_src = ntohs(m_sockaddr_in6_src.sin6_port);
        cout << "Message::ParseIpParameters: m_ip_str_src6 = " << m_ip_str_src << endl;
        cout << "Message::ParseIpParameters: m_port_src6 = " << m_port_src << endl;
    }  else if (sa_lenP == sizeof(struct sockaddr_in)) {
        m_is_ipv6 = false;
        memcpy(&m_sockaddr_in4_src,sa_fromP,sa_lenP);
        ipaddr_t a;
        memcpy(&a,&m_sockaddr_in4_src.sin_addr,4);
        Utility::l2ip(a,m_ip_str_src);
        m_port_src = ntohs(m_sockaddr_in4_src.sin_port);
        cout << "Message::ParseIpParameters: m_ip_str_src = " << m_ip_str_src << endl;
        cout << "Message::ParseIpParameters: m_port_src = " << m_port_src << endl;
    }
}
//----------------------------------------------------------------------------
msg_length_t Message::GetSerializedMessageSize()
//----------------------------------------------------------------------------
{
    return m_msg_length;
}
//----------------------------------------------------------------------------
const char* Message::GetSerializedMessageBuffer()
//----------------------------------------------------------------------------
{
    return const_cast<const char*>(reinterpret_cast<char*>(m_message_serialize_buffer));
}
//----------------------------------------------------------------------------
std::string Message::StatusToString(msg_response_status_t statusP)
//----------------------------------------------------------------------------
{
    stringstream result;
    switch (statusP) {
        //-------------------------------
        case STATUS_CONNECTION_ACCEPTED:
            //-------------------------------
            result << "CONNECTION_ACCEPTED"; return result.str();
            break;
        //-------------------------------
        case STATUS_CONNECTION_FAILED:
            //-------------------------------
            result << "CONNECTION_FAILED"; return result.str();
            break;
        //-------------------------------
        case STATUS_ADD_USER_SUCCESSFULL:
            //-------------------------------
            result << "ADD_USER_SUCCESSFULL"; return result.str();
            break;
        //-------------------------------
        case STATUS_ADD_USER_FAILED:
            //-------------------------------
            result << "ADD_USER_FAILED"; return result.str();
            break;
        //-------------------------------
        case STATUS_REMOVE_USER_SUCCESSFULL:
            //-------------------------------
            result << "REMOVE_USER_SUCCESSFULL"; return result.str();
            break;
        //-------------------------------
        case STATUS_REMOVE_USER_FAILED:
            //-------------------------------
            result << "REMOVE_USER_FAILED"; return result.str();
            break;
        //-------------------------------
        case STATUS_ADD_RADIO_ACCESS_BEARER_SUCCESSFULL:
            //-------------------------------
            result << "ADD_RADIO_ACCESS_BEARER_SUCCESSFULL"; return result.str();
            break;
        //-------------------------------
        case STATUS_ADD_RADIO_ACCESS_BEARER_FAILED:
            //-------------------------------
            result << "ADD_RADIO_ACCESS_BEARER_FAILED"; return result.str();
            break;
        //-------------------------------
        case STATUS_REMOVE_RADIO_ACCESS_BEARER_SUCCESSFULL:
            //-------------------------------
            result << "REMOVE_RADIO_ACCESS_BEARER_SUCCESSFULL"; return result.str();
            break;
        //-------------------------------
        case STATUS_REMOVE_RADIO_ACCESS_BEARER_FAILED:
            //-------------------------------
            result << "REMOVE_RADIO_ACCESS_BEARER_FAILED"; return result.str();
            break;
        default:
            result << "UNKNOWN STATUS " << statusP; return result.str();
    }
    return result.str();
}
//----------------------------------------------------------------------------
std::string Message::ReasonToString(msg_response_reason_t reasonP)
//----------------------------------------------------------------------------
{
    stringstream result;
    switch (reasonP) {
        //-------------------------------
        case MSG_RESP_OK:
            //-------------------------------
            result << "RESP_OK"; return result.str();
            break;
        //-------------------------------
        case MSG_RESP_PROTOCOL_ERROR:
            //-------------------------------
            result << "RESP_PROTOCOL_ERROR"; return result.str();
            break;
        //-------------------------------
        case MSG_RESP_RESSOURCES_UNAVAILABLE:
            //-------------------------------
            result << "RESP_RESSOURCES_UNAVAILABLE"; return result.str();
            break;
        //-------------------------------
        case MSG_RESP_UNKNOWN_MESSAGE:
            //-------------------------------
            result << "RESP_UNKNOWN_MESSAGE"; return result.str();
            break;
        //-------------------------------
        case MSG_RESP_INTERNAL_ERROR:
            //-------------------------------
            result << "RESP_INTERNAL_ERROR"; return result.str();
            break;
        default:
            result << "UNKNOWN REASON " << reasonP; return result.str();
    }
    return result.str();
}
//-----------------------------------------------------------------
Message::~Message()
//-----------------------------------------------------------------
{
    if (m_message_buffer != NULL) {
        delete [] m_message_buffer;
    }
    if (m_ip_char_src != NULL) {
        delete [] m_ip_char_src;
    }
    m_message_storage.reset();
}
