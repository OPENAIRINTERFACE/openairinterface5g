/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

