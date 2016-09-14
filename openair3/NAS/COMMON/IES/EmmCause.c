#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EmmCause.h"

int decode_emm_cause(EmmCause *emmcause, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *emmcause = *(buffer + decoded);
  decoded++;
#if defined (NAS_DEBUG)
  dump_emm_cause_xml(emmcause, iei);
#endif
  return decoded;
}

int encode_emm_cause(EmmCause *emmcause, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, EMM_CAUSE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_emm_cause_xml(emmcause, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = *emmcause;
  encoded++;
  return encoded;
}

void dump_emm_cause_xml(EmmCause *emmcause, uint8_t iei)
{
  printf("<Emm Cause>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Cause value>%u</Cause value>\n", *emmcause);
  printf("</Emm Cause>\n");
}

