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

