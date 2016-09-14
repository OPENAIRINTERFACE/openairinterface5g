#    ifndef _TRANSACTIONS_H
#        define _TRANSACTIONS_H

#        include <map>
#        include "Message.h"
#        include "Transaction.h"
#        include "platform.h"
using namespace std;

class Transactions
{
public:

  static Transactions *Instance ();
  ~Transactions ();

  bool    IsTransactionRegistered(transaction_id_t idP);
  void    AddTransaction(Transaction* txP);
  void    RemoveTransaction(transaction_id_t idP);
  Transaction* const GetTransaction(transaction_id_t idP);

private:
  Transactions ();

  static Transactions                *s_instance;
  map<transaction_id_t,Transaction*>  m_transaction_map;
};
#    endif

