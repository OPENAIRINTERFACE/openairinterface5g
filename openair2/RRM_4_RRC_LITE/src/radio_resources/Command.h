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
