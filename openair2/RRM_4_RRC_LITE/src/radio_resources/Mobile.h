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

