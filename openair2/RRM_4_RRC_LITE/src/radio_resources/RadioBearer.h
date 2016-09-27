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

#ifndef _RADIOBEARER_H
#    define _RADIOBEARER_H

#    include <boost/ptr_container/ptr_map.hpp>
#    include <map>
//#        include <boost/shared_ptr.hpp>

#    include "Message.h"
#    include "LogicalChannel.h"
#    include "platform.h"
#    include "Command.h"
#    include "RLC-Config.h"
#    include "PDCP-Config.h"
#    include "LogicalChannelConfig.h"
using namespace std;

class RadioBearer
{
public:


  RadioBearer ():m_id(0u) {};
  RadioBearer (rb_id_t,
               T_PollRetransmit_t,
               PollPDU_t,
               PollByte_t,
               long,
               T_Reordering_t,
               T_StatusProhibit_t,
               long ,
               long ,
               long ,
               long,
               Command*);

  // RLC_UM BIDIRECTIONAL OR UNIDIRECTIONAL DL
  RadioBearer (rb_id_t,
               rlc_mode_t,
               SN_FieldLength_t,
               T_Reordering_t,
               long ,
               long ,
               long ,
               long,
               Command*);

  // RLC_UM BIDIRECTIONAL OR UNIDIRECTIONAL UL
  RadioBearer (rb_id_t,
               SN_FieldLength_t,
               long ,
               long ,
               long ,
               long,
               Command*);

  ~RadioBearer ();

  friend inline bool operator>( const RadioBearer& l, const RadioBearer r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const RadioBearer& l, const RadioBearer r ) {
    return l.m_id == r.m_id;
  }

  void SetLogicalChannelParameters(logical_channel_priority_t ,
                                   long ,
                                   long ,
                                   long);

  void SetRlcUmParameters(rlc_mode_t,
                          SN_FieldLength_t ,
                          T_StatusProhibit_t );

  void SetRlcAmParameters(T_PollRetransmit_t,
                          PollPDU_t,
                          PollByte_t,
                          long,
                          T_Reordering_t,
                          T_StatusProhibit_t);

  rb_id_t GetId() {
    return m_id;
  };

private:
  rb_id_t       m_id;
  //pdcp_config_t                     m_pdcp_config;
  rlc_mode_t                          m_rlc_mode;

  // RLC UM parameters
  SN_FieldLength_t                    m_sn_field_length;

  // RLC AM parameters
  T_PollRetransmit_t                  m_t_poll_retransmit;
  PollPDU_t                           m_poll_pdu;
  PollByte_t                          m_poll_byte;
  long                                m_max_retx_threshold;
  T_StatusProhibit_t                  m_t_status_prohibit;

  // COMMOM RLC parameters
  T_Reordering_t                      m_t_reordering;

  long                                m_priority;
  long                                m_prioritized_bit_rate;
  long                                m_bucket_size_duration;
  long                                m_logical_channel_group;


  // ASN1 AUTO GENERATED STRUCTS COMPLIANT WITH 3GPP
  //PDCP_Config_t                       m_pdcp_config;
  //RLC_Config_t                        m_rlc_config;

};
#    endif

