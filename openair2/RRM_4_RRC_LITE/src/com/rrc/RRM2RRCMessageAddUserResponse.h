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

#ifndef _RRM2RRCMESSAGEADDUSERRESPONSE_H
#    define _RRM2RRCMESSAGEADDUSERRESPONSE_H

#    include "RRM-RRC-Message.h"
#    include "Message.h"
#    include "platform.h"

class RRM2RRCMessageAddUserResponse: public Message
{

public:
  RRM2RRCMessageAddUserResponse(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRM_RRC_Message_t* asn1_messageP);
  RRM2RRCMessageAddUserResponse(transaction_id_t transaction_idP, msg_response_status_t statusP, msg_response_reason_t reasonP, RRM_RRC_Message_t* asn1_messageP);
  RRM2RRCMessageAddUserResponse(msg_response_status_t statusP, msg_response_reason_t reasonP, cell_id_t cell_idP, mobile_id_t mobile_idP, transaction_id_t transaction_idP);
  ~RRM2RRCMessageAddUserResponse ();

  void   Serialize ();
  void   Forward();

  msg_response_status_t  GetStatus() {
    return m_status;
  };
  msg_response_reason_t  GetReason() {
    return m_reason;
  };
  const cell_id_t        GetENodeBId() {
    return m_cell_id;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  };
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  };

protected:
  msg_response_status_t m_status;
  msg_response_reason_t m_reason;
  cell_id_t             m_cell_id;
  mobile_id_t           m_mobile_id;
  transaction_id_t      m_transaction_id;

  RRM_RRC_Message_t*    m_asn1_message;
};
#endif

