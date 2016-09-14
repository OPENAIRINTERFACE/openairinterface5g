#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "PdnType.h"

int decode_pdn_type(PdnType *pdntype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, PDN_TYPE_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *pdntype = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_pdn_type_xml(pdntype, iei);
#endif
  return decoded;
}

int decode_u8_pdn_type(PdnType *pdntype, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *pdntype = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_pdn_type_xml(pdntype, iei);
#endif
  return decoded;
}

int encode_pdn_type(PdnType *pdntype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, PDN_TYPE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_pdn_type_xml(pdntype, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*pdntype & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_pdn_type(PdnType *pdntype)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
#if defined (NAS_DEBUG)
  dump_pdn_type_xml(pdntype, 0);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*pdntype & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_pdn_type_xml(PdnType *pdntype, uint8_t iei)
{
  printf("<Pdn Type>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <PDN type value>%u</PDN type value>\n", *pdntype);
  printf("</Pdn Type>\n");
}

