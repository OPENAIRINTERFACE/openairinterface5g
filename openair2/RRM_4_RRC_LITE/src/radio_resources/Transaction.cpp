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

#include <iostream>
#include <sstream>
#include <stdio.h>
//----------------------------------------------------------------------------
#include "Transaction.h"
//----------------------------------------------------------------------------
Transaction::Transaction (transaction_id_t transaction_idP): m_id(transaction_idP)
//----------------------------------------------------------------------------
{
    m_commands_vector.reserve(16);
    fprintf(stderr, "[Transaction] new Transaction %d\n", transaction_idP);
}
//----------------------------------------------------------------------------
Transaction::Transaction (transaction_id_t transaction_idP, tcpip::Storage& storageP, packet_size_t sizeP): m_id(transaction_idP)
//----------------------------------------------------------------------------
{
    fprintf(stderr, "[Transaction] new Transaction %d\n", transaction_idP);
    m_commands_vector.reserve(16);
    while (sizeP > 0) {
        Command* command = new Command(storageP, &sizeP);
        AddCommand(command);
    }
}
//----------------------------------------------------------------------------
void Transaction::AddCommand (Command* commandP)
//----------------------------------------------------------------------------
{
    fprintf(stderr, "[Transaction][%d] AddCommand() %s\n", m_id, commandP->ToString().c_str());
    m_commands_vector.push_back(commandP);
}
// Serialize Transaction in packet
//----------------------------------------------------------------------------
void Transaction::Serialize(unsigned char* packetP)
//----------------------------------------------------------------------------
{
    int  cursor = 0;
    fprintf(stderr, "[Transaction][%d] Serialize()\n", m_id);

    vector<Command*>::const_iterator ci;
    for(ci=m_commands_vector.begin(); ci!=m_commands_vector.end(); ci++)
    {
       Command* command = *ci;
       int size = command->size();
       command->Serialize(&packetP[cursor]);
       cursor += size;
    }
}
//----------------------------------------------------------------------------
std::string Transaction::ToString()
//----------------------------------------------------------------------------
{
    stringstream result;
    result << "Transaction " << (unsigned int)m_id << ":\n";
    vector<Command*>::const_iterator ci;
    for(ci=m_commands_vector.begin(); ci!=m_commands_vector.end(); ci++)
    {
       result << "\t" << (*ci)->ToString() << "\n";
    }
    return result.str();
}

//----------------------------------------------------------------------------
packet_size_t Transaction::Size() const {
//----------------------------------------------------------------------------

    if (m_commands_vector.empty()) return 0;
    packet_size_t size = 0;
    for (unsigned int i = 0; i < m_commands_vector.size(); i++) {
        if (m_commands_vector.at(i) != NULL) {
            size += m_commands_vector[i]->size();
        }
    }
    fprintf(stderr, "[Transaction][%d] Size() = %d\n", m_id, size);
    return size;
}
//----------------------------------------------------------------------------
Transaction::~Transaction()
//----------------------------------------------------------------------------
{

    fprintf(stderr, "[Transaction][%d] ~Transaction()\n", m_id);
    vector<Command*>::const_iterator ci;
    for(ci=m_commands_vector.begin(); ci!=m_commands_vector.end(); ci++)
    {
       Command* command = *ci;
       delete command;
    }
    m_commands_vector.clear();
}
