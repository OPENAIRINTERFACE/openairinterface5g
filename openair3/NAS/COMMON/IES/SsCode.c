#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "SsCode.h"

int decode_ss_code(SsCode *sscode, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *sscode = *(buffer + decoded);
  decoded++;
#if defined (NAS_DEBUG)
  dump_ss_code_xml(sscode, iei);
#endif
  return decoded;
}

int encode_ss_code(SsCode *sscode, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, SS_CODE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_ss_code_xml(sscode, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = *sscode;
  encoded++;
  return encoded;
}

void dump_ss_code_xml(SsCode *sscode, uint8_t iei)
{
  printf("<Ss Code>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <SS code value>%u</SS code value>\n", *sscode);
  printf("</Ss Code>\n");
}

