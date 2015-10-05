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
#include "CsfbResponse.h"

int decode_csfb_response(CsfbResponse *csfbresponse, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, CSFB_RESPONSE_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *csfbresponse = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_csfb_response_xml(csfbresponse, iei);
#endif
  return decoded;
}

int decode_u8_csfb_response(CsfbResponse *csfbresponse, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *csfbresponse = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_csfb_response_xml(csfbresponse, iei);
#endif
  return decoded;
}

int encode_csfb_response(CsfbResponse *csfbresponse, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, CSFB_RESPONSE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_csfb_response_xml(csfbresponse, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*csfbresponse & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_csfb_response(CsfbResponse *csfbresponse)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
  dump_csfb_response_xml(csfbresponse, 0);
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*csfbresponse & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_csfb_response_xml(CsfbResponse *csfbresponse, uint8_t iei)
{
  printf("<Csfb Response>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <CSFB response value>%u</CSFB response value>\n", *csfbresponse);
  printf("</Csfb Response>\n");
}

