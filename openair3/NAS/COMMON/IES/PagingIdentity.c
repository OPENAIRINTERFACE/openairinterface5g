#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "PagingIdentity.h"

int decode_paging_identity(PagingIdentity *pagingidentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *pagingidentity = *buffer & 0x1;
  decoded++;
#if defined (NAS_DEBUG)
  dump_paging_identity_xml(pagingidentity, iei);
#endif
  return decoded;
}

int encode_paging_identity(PagingIdentity *pagingidentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, PAGING_IDENTITY_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_paging_identity_xml(pagingidentity, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 |
                        (*pagingidentity & 0x1);
  encoded++;
  return encoded;
}

void dump_paging_identity_xml(PagingIdentity *pagingidentity, uint8_t iei)
{
  printf("<Paging Identity>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Paging identity value>%u</Paging identity value>\n", *pagingidentity);
  printf("</Paging Identity>\n");
}

