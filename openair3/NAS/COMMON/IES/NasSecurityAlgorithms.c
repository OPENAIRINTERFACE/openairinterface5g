#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "NasSecurityAlgorithms.h"

int decode_nas_security_algorithms(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  nassecurityalgorithms->typeofcipheringalgorithm = (*(buffer + decoded) >> 4) & 0x7;
  nassecurityalgorithms->typeofintegrityalgorithm = *(buffer + decoded) & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_nas_security_algorithms_xml(nassecurityalgorithms, iei);
#endif
  return decoded;
}

int encode_nas_security_algorithms(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, NAS_SECURITY_ALGORITHMS_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_nas_security_algorithms_xml(nassecurityalgorithms, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 |
                        ((nassecurityalgorithms->typeofcipheringalgorithm & 0x7) << 4) |
                        (nassecurityalgorithms->typeofintegrityalgorithm & 0x7);
  encoded++;
  return encoded;
}

void dump_nas_security_algorithms_xml(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei)
{
  printf("<Nas Security Algorithms>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Type of ciphering algorithm>%u</Type of ciphering algorithm>\n", nassecurityalgorithms->typeofcipheringalgorithm);
  printf("    <Type of integrity algorithm>%u</Type of integrity algorithm>\n", nassecurityalgorithms->typeofintegrityalgorithm);
  printf("</Nas Security Algorithms>\n");
}

