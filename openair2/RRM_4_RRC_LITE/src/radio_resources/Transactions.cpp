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

#include <stdio.h>
#include "Transactions.h"
#include "Exceptions.h"


Transactions* Transactions::s_instance = 0;

//----------------------------------------------------------------------------
Transaction* const  Transactions::GetTransaction(transaction_id_t idP)  {
    //----------------------------------------------------------------------------
    map<transaction_id_t ,Transaction*>::iterator it;
    it=m_transaction_map.find(idP);
    if (it != m_transaction_map.end()) {
        return it->second;
    } else {
        throw no_such_transaction_error();
    }
}
//----------------------------------------------------------------------------
bool Transactions::IsTransactionRegistered(transaction_id_t idP)  {
    //----------------------------------------------------------------------------
    map<transaction_id_t ,Transaction*>::iterator it;
    it=m_transaction_map.find(idP);
    if (it != m_transaction_map.end()) {
        return true;
    } else {
        return false;
    }
}
//----------------------------------------------------------------------------
void Transactions::RemoveTransaction(transaction_id_t idP) {
    //----------------------------------------------------------------------------
    unsigned int n = m_transaction_map.erase(idP);
    fprintf(stderr, "[Transactions] Transaction %d removed(%d)\n", idP, n);
}
//----------------------------------------------------------------------------
void Transactions::AddTransaction(Transaction* txP) {
    //----------------------------------------------------------------------------
    if (txP == NULL) {
        throw null_transaction_error();
    }
    transaction_id_t tx_id = txP->GetId();
    if (IsTransactionRegistered(tx_id)) {
        throw transaction_already_exist_error();
    } else {
        fprintf(stderr, "[Transactions] Adding new Transaction %d\n", tx_id);
        m_transaction_map.insert(std::pair<transaction_id_t,Transaction*>(tx_id, txP));
    }
}
//-----------------------------------------------------------------
Transactions* Transactions::Instance()
//-----------------------------------------------------------------
{
    if (Transactions::s_instance == 0) {
        Transactions::s_instance = new Transactions;
    }
    return s_instance;
}
//-----------------------------------------------------------------
Transactions::Transactions()
//-----------------------------------------------------------------
{

}
//-----------------------------------------------------------------
Transactions::~Transactions()
//-----------------------------------------------------------------
{
    if (!m_transaction_map.empty())  m_transaction_map.clear();
}

