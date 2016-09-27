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

#ifndef _ENODEB_H
#    define _ENODEB_H

#    include "RRM-RRC-Message.h"
#    include "RadioResourceConfigCommon.h"
#    include "E-NodeB-Identity.h"
#    include "PhysicalConfigDedicated.h"
#    include "RadioResourceConfigDedicated.h"
#    include "DRB-ToAddMod.h"
#    include "SRB-ToAddMod.h"
#    include "MAC-MainConfig.h"
#    include "RRCSystemConfigurationResponse.h"
#    include "RRCConnectionReconfiguration.h"
#    include "SystemInformationBlockType1.h"
#    include "SystemInformationBlockType2.h"
#    include "SystemInformationBlockType3.h"
#    include "SystemInformation.h"

//#    include <boost/ptr_container/ptr_map.hpp>
//#    include <map>
#    include "Message.h"
#    include "Mobile.h"
#    include "RadioBearer.h"
#    include "platform.h"
#    include "Transaction.h"
#    include "LogicalChannelConfig.h"
#    include "RRCUserReconfiguration.h"

using namespace std;


class ENodeB
{
public:

  ENodeB ():m_id(0u) {};
  ENodeB (cell_id_t);
  ~ENodeB ();

  friend inline bool operator>( const ENodeB& l, const ENodeB r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const ENodeB& l, const ENodeB r ) {
    return l.m_id == r.m_id;
  }

  cell_id_t GetId() {
    return m_id;
  };

  void                            CommitTransaction(transaction_id_t transaction_idP);
  RadioResourceConfigDedicated_t* GetASN1RadioResourceConfigDedicated(transaction_id_t transaction_idP);
  RRM_RRC_Message_t*              GetRRMRRCConfigurationMessage(transaction_id_t transaction_idP);

  RRM_RRC_Message_t* AddUserRequest   (Mobile* mobileP, transaction_id_t transaction_idP);
  RRM_RRC_Message_t* AddUserConfirm   (Mobile* mobileP, transaction_id_t transaction_idP);
  RRM_RRC_Message_t* RemoveUserRequest(Mobile* mobileP, transaction_id_t transaction_idP);
  void               UserReconfigurationComplete(Mobile* mobileP,  transaction_id_t transaction_idP);
  void RemoveAllDataRadioBearers      (Mobile* mobileP, transaction_id_t transaction_idP);

  /*void AddDataRadioBearer(Mobile* mobileP,
                          rb_id_t            radio_bearer_idP,
                          unsigned short     traffic_class,
                          unsigned short     delay,
                          unsigned int       guaranted_bit_rate_uplink,
                          unsigned int       max_bit_rate_uplink,
                          unsigned int       guaranted_bit_rate_downlink,
                          unsigned int       max_bit_rate_downlink,
                          Transaction*       transactionP);*/

  /*void RemoveDataRadioBearer(Mobile* mobileP,
                          rb_id_t            radio_bearer_idP,
                          Transaction*       transactionP);*/

protected:
  void AddSignallingRadioBearer1(mobile_id_t mobile_idP, transaction_id_t transaction_idP);
  void AddSignallingRadioBearer2(mobile_id_t mobile_idP, transaction_id_t transaction_idP);
  void AddDefaultDataRadioBearer(mobile_id_t mobile_idP, transaction_id_t transaction_idP);


  //boost::ptr_map<mobile_id_t ,Mobile > m_mobile_map;
  //RadioBearer*                         m_radio_bearer_array[MAX_RB_PER_MOBILE][MAX_MOBILE_PER_ENODE_B];
  cell_id_t                            m_id;


  // ASN1 AUTO GENERATED STRUCTS COMPLIANT WITH 3GPP
  MAC_MainConfig_t                      m_mac_main_config;
  DRB_ToAddMod_t*                       m_drb_to_add_mod[MAX_DRB][MAX_MOBILE_PER_ENODE_B];
  SRB_ToAddMod_t*                       m_srb_to_add_mod[MAX_SRB][MAX_MOBILE_PER_ENODE_B];
  // FOR TRANSACTIONS
  DRB_Identity_t                        m_pending_drb_to_release[MAX_DRB][MAX_MOBILE_PER_ENODE_B];
  transaction_id_t                      m_tx_id_pending_drb_to_release[MAX_DRB][MAX_MOBILE_PER_ENODE_B];

  DRB_ToAddMod_t*                       m_pending_drb_to_add_mod[MAX_DRB][MAX_MOBILE_PER_ENODE_B];
  transaction_id_t                      m_tx_id_pending_drb_to_add_mod[MAX_DRB][MAX_MOBILE_PER_ENODE_B];

  SRB_ToAddMod_t*                       m_pending_srb_to_add_mod[MAX_SRB][MAX_MOBILE_PER_ENODE_B];
  transaction_id_t                      m_tx_id_pending_srb_to_add_mod[MAX_SRB][MAX_MOBILE_PER_ENODE_B];

};
#    endif

