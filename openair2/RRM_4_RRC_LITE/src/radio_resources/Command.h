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

#    ifndef _COMMAND_H
#        define _COMMAND_H

#    include "platform.h"
#    include "storage.h"

using namespace std;
using namespace tcpip;

class Command : public Storage
{
public:
  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  Command (): m_equipment_type(0u),m_equipment_id(0u),m_action(0u), m_object_type(0u), m_object_id(0u) {
    Storage();
  };
  Command (unsigned char equipment_typeP, unsigned char equipment_idP, unsigned char actionP, unsigned char object_typeP, unsigned char object_idP);
  Command(tcpip::Storage& storageP, size_t* sizeP);

  void AddData(tcpip::Storage&);
  void Serialize(unsigned char* packetP);
  unsigned int size() const;

  ~Command () {
    reset();
  };

  unsigned char GetEquipmentType() {
    return m_equipment_type;
  };
  unsigned char GetEquipmentId()   {
    return m_equipment_id;
  };
  unsigned char GetAction()        {
    return m_action;
  };
  unsigned char GetEntityType()    {
    return m_object_type;
  };
  unsigned char GetEntityId()      {
    return m_object_id;
  };

  std::string ToString();
  std::string EquipmentType2String();
  std::string Action2String();
  std::string ObjectType2String();
protected:
  unsigned char  m_equipment_type;
  unsigned char  m_equipment_id;
  unsigned char  m_action;
  unsigned char  m_object_type;
  unsigned char  m_object_id;

  bool           m_is_data_written;
};
#    endif
