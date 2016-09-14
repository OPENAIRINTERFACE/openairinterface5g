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

