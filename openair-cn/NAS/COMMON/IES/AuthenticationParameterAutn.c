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
#include "AuthenticationParameterAutn.h"

int decode_authentication_parameter_autn(AuthenticationParameterAutn *authenticationparameterautn, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;
  int decode_result;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);

  if ((decode_result = decode_octet_string(&authenticationparameterautn->autn, ielen, buffer + decoded, len - decoded)) < 0)
    return decode_result;
  else
    decoded += decode_result;

#if defined (NAS_DEBUG)
  dump_authentication_parameter_autn_xml(authenticationparameterautn, iei);
#endif
  return decoded;
}
int encode_authentication_parameter_autn(AuthenticationParameterAutn *authenticationparameterautn, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  int encode_result;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, AUTHENTICATION_PARAMETER_AUTN_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_authentication_parameter_autn_xml(authenticationparameterautn, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;

  if ((encode_result = encode_octet_string(&authenticationparameterautn->autn, buffer + encoded, len - encoded)) < 0)
    return encode_result;
  else
    encoded += encode_result;

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_authentication_parameter_autn_xml(AuthenticationParameterAutn *authenticationparameterautn, uint8_t iei)
{
  printf("<Authentication Parameter Autn>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("%s", dump_octet_string_xml(&authenticationparameterautn->autn));
  printf("</Authentication Parameter Autn>\n");
}

