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
