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

