#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "ImeisvRequest.h"

int decode_imeisv_request(ImeisvRequest *imeisvrequest, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, IMEISV_REQUEST_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *imeisvrequest = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_imeisv_request_xml(imeisvrequest, iei);
#endif
  return decoded;
}

int decode_u8_imeisv_request(ImeisvRequest *imeisvrequest, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *imeisvrequest = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_imeisv_request_xml(imeisvrequest, iei);
#endif
  return decoded;
}

int encode_imeisv_request(ImeisvRequest *imeisvrequest, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, IMEISV_REQUEST_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_imeisv_request_xml(imeisvrequest, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*imeisvrequest & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_imeisv_request(ImeisvRequest *imeisvrequest)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
  dump_imeisv_request_xml(imeisvrequest, 0);
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*imeisvrequest & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_imeisv_request_xml(ImeisvRequest *imeisvrequest, uint8_t iei)
{
  printf("<Imeisv Request>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <IMEISV request value>%u</IMEISV request value>\n", *imeisvrequest);
  printf("</Imeisv Request>\n");
}

