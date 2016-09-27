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

#ifndef _RADIORESOURCES_H
#    define _RADIORESOURCES_H

#    include "RRM-RRC-Message.h"
#    include <map>
//#    include <boost/ptr_container/ptr_map.hpp>
#    include "platform_defines.h"
#    include "Message.h"
#    include "ENodeB.h"
#    include "RRC2RRMMessageConnectionRequest.h"
#    include "RRM2RRCMessageConnectionResponse.h"
#    include "RRC2RRMMessageAddUserRequest.h"
#    include "RRM2RRCMessageAddUserResponse.h"
#    include "RRC2RRMMessageAddUserConfirm.h"
#    include "RRM2RRCMessageUserReconfiguration.h"
#    include "RRC2RRMMessageUserReconfigurationComplete.h"
#    include "RRC2RRMMessageRemoveUserRequest.h"
#    include "RRC2RRMMessageAddRadioAccessBearerRequest.h"
#    include "RRC2RRMMessageAddRadioAccessBearerConfirm.h"
#    include "RRC2RRMMessageRemoveRadioAccessBearerRequest.h"
#    include "platform.h"
#    include "Transaction.h"

using namespace std;

class RadioResources
{
public:

  //static const int MAX_MOBILES_PER_ENODE_B        = 16;
  //static const int MAX_RADIO_BEARERS_PER_MOBILE   = 16;
  //static const int MAX_RADIO_BEARERS_PER_ENODE_B  = 256;

  // mobile_id_t is local to the cell here
  typedef std::pair<cell_id_t,mobile_id_t> mobile_node_local_key_t;


  static RadioResources *Instance ();
  ~RadioResources ();

  bool    IsENodeBRegistered(cell_id_t idP);
  ENodeB* const GetENodeB(cell_id_t idP);
  bool    IsMobileRegistered(cell_id_t e_node_b_idP, mobile_id_t mobile_idP);
  Mobile* const GetMobile(cell_id_t e_node_b_idP, mobile_id_t mobile_idP);

  void    AddENodeB(cell_id_t idP);
  RRM2RRCMessageConnectionResponse*  Request(RRC2RRMMessageConnectionRequest &requestP);
  RRM2RRCMessageAddUserResponse*     Request(RRC2RRMMessageAddUserRequest    &requestP);
  RRM2RRCMessageUserReconfiguration* Request(RRC2RRMMessageAddUserConfirm    &requestP);
  void                               Request(RRC2RRMMessageUserReconfigurationComplete &confirmP);
  Transaction*                       Request(RRC2RRMMessageRemoveUserRequest    &requestP);
  Transaction*                       Request(RRC2RRMMessageAddRadioAccessBearerRequest    &requestP);
  Transaction*                       Request(RRC2RRMMessageAddRadioAccessBearerConfirm    &confirmP);
  Transaction*                       Request(RRC2RRMMessageRemoveRadioAccessBearerRequest &requestP);
private:
  RadioResources ();

  static RadioResources                   *s_instance;
  static cell_id_t                         s_cell_id_generator;
  map<cell_id_t,ENodeB*>                   m_e_node_b_map;
  map<mobile_node_local_key_t,Mobile*>     m_mobile_local_map;
};
#    endif

