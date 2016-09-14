#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "Nonce.h"

int decode_nonce(Nonce *nonce, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  //IES_DECODE_U32(*nonce, *(buffer + decoded));
  IES_DECODE_U32(buffer, decoded, *nonce);
#if defined (NAS_DEBUG)
  dump_nonce_xml(nonce, iei);
#endif
  return decoded;
}

int encode_nonce(Nonce *nonce, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, NONCE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_nonce_xml(nonce, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  IES_ENCODE_U32(buffer, encoded, *nonce);
  return encoded;
}

void dump_nonce_xml(Nonce *nonce, uint8_t iei)
{
  printf("<Nonce>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Nonce value>%u</Nonce value>\n", *nonce);
  printf("</Nonce>\n");
}

