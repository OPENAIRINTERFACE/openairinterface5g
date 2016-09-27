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

