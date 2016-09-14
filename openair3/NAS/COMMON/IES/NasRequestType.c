#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "NasRequestType.h"

int decode_request_type(RequestType *requesttype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, REQUEST_TYPE_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *requesttype = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_request_type_xml(requesttype, iei);
#endif
  return decoded;
}

int decode_u8_request_type(RequestType *requesttype, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *requesttype = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_request_type_xml(requesttype, iei);
#endif
  return decoded;
}

int encode_request_type(RequestType *requesttype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, REQUEST_TYPE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_request_type_xml(requesttype, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*requesttype & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_request_type(RequestType *requesttype)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
#if defined (NAS_DEBUG)
  dump_request_type_xml(requesttype, 0);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*requesttype & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_request_type_xml(RequestType *requesttype, uint8_t iei)
{
  printf("<Request Type>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Request type value>%u</Request type value>\n", *requesttype);
  printf("</Request Type>\n");
}

