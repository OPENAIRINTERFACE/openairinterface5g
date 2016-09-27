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

#ifndef _MOBILE_H
#    define _MOBILE_H

#    include <boost/ptr_container/ptr_map.hpp>
#    include <map>
//#        include <boost/shared_ptr.hpp>

#    include "PhysicalConfigDedicated.h"
#    include "RadioResourceConfigDedicated.h"
#    include "DRB-ToAddMod.h"
#    include "SRB-ToAddMod.h"
#    include "MAC-MainConfig.h"

#    include "Message.h"
#    include "Transaction.h"
#    include "platform.h"
#    include "RadioBearer.h"


class ENodeB;

using namespace std;

class Mobile
{
public:
  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  Mobile (mobile_id_t);
  Mobile () {
    Mobile(0u);
  };
  Mobile(cell_id_t , mobile_id_t , transaction_id_t );
  ~Mobile ();

  friend inline bool operator>( const Mobile& l, const Mobile r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const Mobile& l, const Mobile r ) {
    return l.m_id == r.m_id;
  }

  mobile_id_t                     GetId() {
    return m_id;
  };
  void                            CommitTransaction(transaction_id_t transaction_idP);
  RadioResourceConfigDedicated_t* GetASN1RadioResourceConfigDedicated(transaction_id_t transaction_idP);
  void                            AddSignallingRadioBearer2(signed int cell_indexP, transaction_id_t transaction_idP);
  void                            AddDefaultDataRadioBearer(signed int cell_indexP, transaction_id_t transaction_idP);

  /*void        AddDataRadioBearer(ENodeB* e_node_bP,
                          rb_id_t            radio_bearer_idP,
                          unsigned short     m_traffic_class,
                          unsigned short     m_delay,
                          unsigned int       m_guaranted_bit_rate_uplink,
                          unsigned int       m_max_bit_rate_uplink,
                          unsigned int       m_guaranted_bit_rate_downlink,
                          unsigned int       m_max_bit_rate_downlink,
                          Transaction*       transactionP);*/

  void        RemoveDataRadioBearer(ENodeB* e_node_bP,
                                    rb_id_t            radio_bearer_idP,
                                    Transaction*       transactionP);
protected:
  signed int  FindCellIndex(cell_id_t cell_idP);

  void        AddSignallingRadioBearer1(signed int cell_indexP, transaction_id_t transaction_idP);
  //void        AddSignallingRadioBearer1(signed int cell_indexP, Transaction* transactionP);
  //void        AddSignallingRadioBearer2(signed int cell_indexP, Transaction* transactionP);
  //void        AddDefaultDataRadioBearer(signed int cell_indexP, Transaction* transactionP);


  signed int                            m_cell_id2_cell_index[MAX_ENODE_B];
  cell_id_t                             m_cell_index2_cell_id[MAX_ENODE_B_PER_MOBILE];
  //boost::ptr_map<mobile_id_t ,Mobile >  m_mobile_map;
  //RadioBearer*                          m_radio_bearer_array[MAX_RB_PER_MOBILE][MAX_ENODE_B_PER_MOBILE];
  mobile_id_t                           m_id;

  // ASN1 AUTO GENERATED STRUCTS COMPLIANT WITH 3GPP
  MAC_MainConfig_t                      m_mac_main_config;
  DRB_ToAddMod_t*                       m_drb_to_add_mod[MAX_DRB][MAX_ENODE_B_PER_MOBILE];
  SRB_ToAddMod_t*                       m_srb_to_add_mod[MAX_SRB][MAX_ENODE_B_PER_MOBILE];

  // FOR TRANSACTIONS
  DRB_Identity_t                        m_pending_drb_to_release[MAX_DRB][MAX_ENODE_B_PER_MOBILE];
  transaction_id_t                      m_tx_id_pending_drb_to_release[MAX_DRB][MAX_ENODE_B_PER_MOBILE];

  DRB_ToAddMod_t*                       m_pending_drb_to_add_mod[MAX_DRB][MAX_ENODE_B_PER_MOBILE];
  transaction_id_t                      m_tx_id_pending_drb_to_add_mod[MAX_DRB][MAX_ENODE_B_PER_MOBILE];

  SRB_ToAddMod_t*                       m_pending_srb_to_add_mod[MAX_SRB][MAX_ENODE_B_PER_MOBILE];
  transaction_id_t                      m_tx_id_pending_srb_to_add_mod[MAX_SRB][MAX_ENODE_B_PER_MOBILE];

};
#    include "ENodeB.h"
#    endif

