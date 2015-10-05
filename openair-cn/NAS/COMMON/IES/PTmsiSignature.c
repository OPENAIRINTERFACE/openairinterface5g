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
#include "PTmsiSignature.h"

int decode_p_tmsi_signature(PTmsiSignature *ptmsisignature, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  int decode_result;
  uint8_t ielen = 3;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  if ((decode_result = decode_octet_string(&ptmsisignature->ptmsisignaturevalue, ielen, buffer + decoded, len - decoded)) < 0)
    return decode_result;
  else
    decoded += decode_result;

#if defined (NAS_DEBUG)
  dump_p_tmsi_signature_xml(ptmsisignature, iei);
#endif
  return decoded;
}

int encode_p_tmsi_signature(PTmsiSignature *ptmsisignature, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encode_result;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, P_TMSI_SIGNATURE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_p_tmsi_signature_xml(ptmsisignature, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  if ((encode_result = encode_octet_string(&ptmsisignature->ptmsisignaturevalue, buffer + encoded, len - encoded)) < 0)
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}

void dump_p_tmsi_signature_xml(PTmsiSignature *ptmsisignature, uint8_t iei)
{
  printf("<P Tmsi Signature>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("%s",dump_octet_string_xml(&ptmsisignature->ptmsisignaturevalue));
  printf("</P Tmsi Signature>\n");
}

