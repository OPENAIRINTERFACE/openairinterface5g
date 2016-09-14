#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "LinkedEpsBearerIdentity.h"

int decode_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, LINKED_EPS_BEARER_IDENTITY_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *linkedepsbeareridentity = *buffer & 0xf;
  decoded++;
#if defined (NAS_DEBUG)
  dump_linked_eps_bearer_identity_xml(linkedepsbeareridentity, iei);
#endif
  return decoded;
}

int decode_u8_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *linkedepsbeareridentity = *buffer & 0xf;
  decoded++;
#if defined (NAS_DEBUG)
  dump_linked_eps_bearer_identity_xml(linkedepsbeareridentity, iei);
#endif
  return decoded;
}

int encode_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, LINKED_EPS_BEARER_IDENTITY_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_linked_eps_bearer_identity_xml(linkedepsbeareridentity, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*linkedepsbeareridentity & 0xf);
  encoded++;
  return encoded;
}

uint8_t encode_u8_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
#if defined (NAS_DEBUG)
  dump_linked_eps_bearer_identity_xml(linkedepsbeareridentity, 0);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*linkedepsbeareridentity & 0xf);
  encoded++;

  return bufferReturn;
}

void dump_linked_eps_bearer_identity_xml(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei)
{
  printf("<Linked Eps Bearer Identity>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Linked EPS bearer identity value>%u</Linked EPS bearer identity value>\n", *linkedepsbeareridentity);
  printf("</Linked Eps Bearer Identity>\n");
}

