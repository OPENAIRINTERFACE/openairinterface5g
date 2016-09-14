#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EsmCause.h"

int decode_esm_cause(EsmCause *esmcause, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *esmcause = *(buffer + decoded);
  decoded++;
#if defined (NAS_DEBUG)
  dump_esm_cause_xml(esmcause, iei);
#endif
  return decoded;
}

int encode_esm_cause(EsmCause *esmcause, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ESM_CAUSE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_esm_cause_xml(esmcause, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = *esmcause;
  encoded++;
  return encoded;
}

void dump_esm_cause_xml(EsmCause *esmcause, uint8_t iei)
{
  printf("<Esm Cause>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Cause value>%u</Cause value>\n", *esmcause);
  printf("</Esm Cause>\n");
}

