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

#ifndef _RRC2RRMMESSAGEREMOVEUSERREQUEST_H
#    define _RRC2RRMMESSAGEREMOVEUSERREQUEST_H

#    include "Message.h"
#    include "storage.h"
#    include "platform.h"

class RRC2RRMMessageRemoveUserRequest: public Message
{

public:
  RRC2RRMMessageRemoveUserRequest(std::string ip_dest_strP, int port_destP,
                                  cell_id_t ,
                                  mobile_id_t ,
                                  transaction_id_t );

  RRC2RRMMessageRemoveUserRequest (tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);

  ~RRC2RRMMessageRemoveUserRequest ();

  void                   Serialize ();
  void                   Forward();
  const unsigned int     GetENodeBId() {
    return 1u;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  }
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }

protected:
  cell_id_t          m_cell_id;
  mobile_id_t        m_mobile_id;
  transaction_id_t   m_transaction_id;
};
#endif

