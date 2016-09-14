#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EpsAttachType.h"

int decode_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, EPS_ATTACH_TYPE_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *epsattachtype = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_eps_attach_type_xml(epsattachtype, iei);
#endif
  return decoded;
}

int decode_u8_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *epsattachtype = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_eps_attach_type_xml(epsattachtype, iei);
#endif
  return decoded;
}

int encode_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, EPS_ATTACH_TYPE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_eps_attach_type_xml(epsattachtype, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*epsattachtype & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_eps_attach_type(EpsAttachType *epsattachtype)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
#if defined (NAS_DEBUG)
  dump_eps_attach_type_xml(epsattachtype, 0);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*epsattachtype & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_eps_attach_type_xml(EpsAttachType *epsattachtype, uint8_t iei)
{
  printf("<Eps Attach Type>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <EPS attach type value>%u</EPS attach type value>\n", *epsattachtype);
  printf("</Eps Attach Type>\n");
}

