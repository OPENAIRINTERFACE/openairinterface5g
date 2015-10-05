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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PROCEDURE_TRANSACTION_IDENTITY_H_
#define PROCEDURE_TRANSACTION_IDENTITY_H_

#define PROCEDURE_TRANSACTION_IDENTITY_MINIMUM_LENGTH 1
#define PROCEDURE_TRANSACTION_IDENTITY_MAXIMUM_LENGTH 1

#define PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED 0
#define PROCEDURE_TRANSACTION_IDENTITY_FIRST    1
#define PROCEDURE_TRANSACTION_IDENTITY_LAST   254
typedef uint8_t ProcedureTransactionIdentity;

int encode_procedure_transaction_identity(ProcedureTransactionIdentity *proceduretransactionidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_procedure_transaction_identity_xml(ProcedureTransactionIdentity *proceduretransactionidentity, uint8_t iei);

int decode_procedure_transaction_identity(ProcedureTransactionIdentity *proceduretransactionidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* PROCEDURE TRANSACTION IDENTITY_H_ */

