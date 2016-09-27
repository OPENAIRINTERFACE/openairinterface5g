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

#ifndef _DATARADIOBEARER_H
#    define _DATARADIOBEARER_H

#    include <boost/ptr_container/ptr_map.hpp>
#    include <map>
//#        include <boost/shared_ptr.hpp>

#    include "Message.h"
#    include "LogicalChannel.h"
#    include "RadioBearer.h"
#    include "DRB-ToAddMod.h"

using namespace std;

class DataRadioBearer: public RadioBearer
{
public:
  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  DataRadioBearer ():m_id(0u) {};
  DataRadioBearer (unsigned int);
  ~DataRadioBearer ();

  friend inline bool operator>( const DataRadioBearer& l, const DataRadioBearer r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const DataRadioBearer& l, const DataRadioBearer r ) {
    return l.m_id == r.m_id;
  }


private:
  unsigned int       m_id;


  DRB_ToAddMod_t     m_drb_to_add_mod;
};
#    endif

