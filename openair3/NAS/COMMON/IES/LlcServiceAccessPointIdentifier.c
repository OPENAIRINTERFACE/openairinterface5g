#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "LlcServiceAccessPointIdentifier.h"

int decode_llc_service_access_point_identifier(LlcServiceAccessPointIdentifier *llcserviceaccesspointidentifier, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *llcserviceaccesspointidentifier = *buffer & 0xf;
  decoded++;
#if defined (NAS_DEBUG)
  dump_llc_service_access_point_identifier_xml(llcserviceaccesspointidentifier, iei);
#endif
  return decoded;
}

int encode_llc_service_access_point_identifier(LlcServiceAccessPointIdentifier *llcserviceaccesspointidentifier, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, LLC_SERVICE_ACCESS_POINT_IDENTIFIER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_llc_service_access_point_identifier_xml(llcserviceaccesspointidentifier, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 |
                        (*llcserviceaccesspointidentifier & 0xf);
  encoded++;
  return encoded;
}

void dump_llc_service_access_point_identifier_xml(LlcServiceAccessPointIdentifier *llcserviceaccesspointidentifier, uint8_t iei)
{
  printf("<Llc Service Access Point Identifier>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <LLC SAPI value>%u</LLC SAPI value>\n", *llcserviceaccesspointidentifier);
  printf("</Llc Service Access Point Identifier>\n");
}

