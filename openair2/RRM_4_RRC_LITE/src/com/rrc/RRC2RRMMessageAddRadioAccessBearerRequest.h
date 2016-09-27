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

#ifndef _RRC2RRMMESSAGEADDRADIOACCESSBEARERREQUEST_H
#    define _RRC2RRMMESSAGEADDRADIOACCESSBEARERREQUEST_H

#    include "Message.h"
#    include "storage.h"
#    include "platform.h"

class RRC2RRMMessageAddRadioAccessBearerRequest: public Message
{

public:
  RRC2RRMMessageAddRadioAccessBearerRequest(std::string ip_dest_strP, int port_destP,
      cell_id_t cell_idP,
      mobile_id_t mobile_idP,
      rb_id_t          radio_bearer_idP,
      unsigned short   traffic_classP,
      unsigned short   delayP,
      unsigned int     guaranted_bit_rate_uplinkP,
      unsigned int     max_bit_rate_uplinkP,
      unsigned int     guaranted_bit_rate_downlinkP,
      unsigned int     max_bit_rate_downlinkP,
      transaction_id_t transaction_idP);
  RRC2RRMMessageAddRadioAccessBearerRequest (tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);
  ~RRC2RRMMessageAddRadioAccessBearerRequest ();

  void   Serialize ();
  void   Forward();
  const unsigned int     GetENodeBId() {
    return m_cell_id;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  }
  const rb_id_t          GetRadioBearerId() {
    return m_radio_bearer_id;
  }
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }

  const unsigned short   GetTrafficClass()             {
    return m_traffic_class;
  }
  const unsigned short   GetDelay()                    {
    return m_delay;
  }
  const unsigned int     GetGuarantedBitRateUplink()   {
    return m_guaranted_bit_rate_uplink;
  }
  const unsigned int     GetMaxBitRateUplink()         {
    return m_max_bit_rate_uplink;
  }
  const unsigned int     GetGuarantedBitRateDownlink() {
    return m_guaranted_bit_rate_downlink;
  }
  const unsigned int     GetMaxBitRateDownlink()       {
    return m_max_bit_rate_downlink;
  }

private:
  cell_id_t          m_cell_id;
  mobile_id_t        m_mobile_id;
  rb_id_t            m_radio_bearer_id;
  unsigned short     m_traffic_class;
  unsigned short     m_delay;
  unsigned int       m_guaranted_bit_rate_uplink;
  unsigned int       m_max_bit_rate_uplink;
  unsigned int       m_guaranted_bit_rate_downlink;
  unsigned int       m_max_bit_rate_downlink;
  transaction_id_t   m_transaction_id;
};
#endif

