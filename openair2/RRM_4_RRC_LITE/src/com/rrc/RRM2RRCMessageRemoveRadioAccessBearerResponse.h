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

#ifndef _RRM2RRCMESSAGEREMOVERADIOACCESSBEARERRESPONSE_H
#    define _RRM2RRCMESSAGEREMOVERADIOACCESSBEARERRESPONSE_H

#    include "Message.h"
#    include "platform.h"
#    include "Transaction.h"

class RRM2RRCMessageRemoveRadioAccessBearerResponse: public Message
{

public:
  RRM2RRCMessageRemoveRadioAccessBearerResponse(tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);
  RRM2RRCMessageRemoveRadioAccessBearerResponse(msg_response_status_t ,
      msg_response_reason_t ,
      cell_id_t ,
      mobile_id_t ,
      rb_id_t            ,
      transaction_id_t );
  RRM2RRCMessageRemoveRadioAccessBearerResponse(msg_response_status_t ,
      msg_response_reason_t ,
      cell_id_t ,
      mobile_id_t ,
      rb_id_t            ,
      transaction_id_t ,
      Transaction* );


  ~RRM2RRCMessageRemoveRadioAccessBearerResponse ();

  void   Serialize ();
  void   Forward();

  msg_response_status_t GetStatus() {
    return m_status;
  };
  msg_response_reason_t GetReason() {
    return m_reason;
  };
  cell_id_t             GetENodeBId() {
    return m_cell_id;
  };
  mobile_id_t           GetMobileId() {
    return m_mobile_id;
  };
  rb_id_t               GetRadioBearerId() {
    return m_radio_bearer_id;
  };
  transaction_id_t      GetTransactionId() {
    return m_transaction_id;
  };

protected:
  msg_response_status_t m_status;
  msg_response_reason_t m_reason;

  cell_id_t          m_cell_id;
  mobile_id_t        m_mobile_id;
  rb_id_t            m_radio_bearer_id;
  transaction_id_t   m_transaction_id;
  Transaction*       m_transaction_result;
};
#endif

