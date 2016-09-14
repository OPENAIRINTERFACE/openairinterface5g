#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PAGING_IDENTITY_H_
#define PAGING_IDENTITY_H_

#define PAGING_IDENTITY_MINIMUM_LENGTH 2
#define PAGING_IDENTITY_MAXIMUM_LENGTH 2

typedef uint8_t PagingIdentity;

int encode_paging_identity(PagingIdentity *pagingidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_paging_identity_xml(PagingIdentity *pagingidentity, uint8_t iei);

int decode_paging_identity(PagingIdentity *pagingidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* PAGING IDENTITY_H_ */

