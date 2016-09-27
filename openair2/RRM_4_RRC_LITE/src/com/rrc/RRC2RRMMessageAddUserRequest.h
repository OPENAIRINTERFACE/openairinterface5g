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

#ifndef _RRC2RRMMESSAGEADDUSERREQUEST_H
#    define _RRC2RRMMESSAGEADDUSERREQUEST_H

#    include "RRC-RRM-Message.h"
#    include "Message.h"
#    include "platform.h"

class RRC2RRMMessageAddUserRequest: public Message
{

public:
  RRC2RRMMessageAddUserRequest(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRC_RRM_Message_t* asn1_messageP);
  RRC2RRMMessageAddUserRequest(std::string ip_dest_strP, int port_destP, cell_id_t cell_idP, mobile_id_t mobile_idP, transaction_id_t transaction_idP);
  ~RRC2RRMMessageAddUserRequest ();

  void   Serialize ();
  void   Forward();
  const cell_id_t        GetENodeBId() {
    return m_cell_id;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  }
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }
private:
  cell_id_t             m_cell_id;
  mobile_id_t           m_mobile_id;
  transaction_id_t      m_transaction_id;
  RRC_RRM_Message_t    *m_asn1_message;
};
#endif

