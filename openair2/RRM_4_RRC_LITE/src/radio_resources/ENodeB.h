/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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

