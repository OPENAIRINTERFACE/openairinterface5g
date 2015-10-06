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

#ifndef _TRANSACTION_H
#    define _TRANSACTION_H

#    include <vector>
#    include "Message.h"
#    include "Command.h"
#    include "platform.h"
#    include "storage.h"

using namespace std;

class Transaction
{
public:
  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  Transaction ():m_id(0u) {};
  Transaction (transaction_id_t);
  Transaction (transaction_id_t , tcpip::Storage& , packet_size_t );
  ~Transaction ();

  void AddCommand (Command*);
  friend inline bool operator>( const Transaction& l, const Transaction r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const Transaction& l, const Transaction r ) {
    return l.m_id == r.m_id;
  }


  void Commit() {};
  void Rollback() {};
  transaction_id_t GetId() {
    return m_id;
  };


  std::string ToString();

  void Serialize(unsigned char* packetP);
  packet_size_t Size() const;

protected:
  transaction_id_t   m_id;
  vector<Command*> m_commands_vector;
};
#    endif

