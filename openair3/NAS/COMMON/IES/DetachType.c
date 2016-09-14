#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "DetachType.h"

int decode_detach_type(DetachType *detachtype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, DETACH_TYPE_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  detachtype->switchoff = (*(buffer + decoded) >> 3) & 0x1;
  detachtype->typeofdetach = *(buffer + decoded) & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_detach_type_xml(detachtype, iei);
#endif
  return decoded;
}

int decode_u8_detach_type(DetachType *detachtype, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  detachtype->switchoff = (*(buffer + decoded) >> 3) & 0x1;
  detachtype->typeofdetach = *(buffer + decoded) & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_detach_type_xml(detachtype, iei);
#endif
  return decoded;
}

int encode_detach_type(DetachType *detachtype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, DETACH_TYPE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_detach_type_xml(detachtype, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        ((detachtype->switchoff & 0x1) << 3) |
                        (detachtype->typeofdetach & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_detach_type(DetachType *detachtype)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
#if defined (NAS_DEBUG)
  dump_detach_type_xml(detachtype, 0);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        ((detachtype->switchoff & 0x1) << 3) |
                        (detachtype->typeofdetach & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_detach_type_xml(DetachType *detachtype, uint8_t iei)
{
  printf("<Detach Type>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Switch off>%u</Switch off>\n", detachtype->switchoff);
  printf("    <Type of detach>%u</Type of detach>\n", detachtype->typeofdetach);
  printf("</Detach Type>\n");
}

