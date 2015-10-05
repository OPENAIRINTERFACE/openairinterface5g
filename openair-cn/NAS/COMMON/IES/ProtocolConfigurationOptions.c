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
#include "ProtocolConfigurationOptions.h"

int decode_protocol_configuration_options(ProtocolConfigurationOptions *protocolconfigurationoptions, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  uint8_t ielen = 0;
  int decode_result;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);

  if (((*(buffer + decoded) >> 7) & 0x1) != 1) {
    errorCodeDecoder = TLV_DECODE_VALUE_DOESNT_MATCH;
    return TLV_DECODE_VALUE_DOESNT_MATCH;
  }

  /* Bits 7 to 4 of octet 3 are spare, read as 0 */
  if (((*(buffer + decoded) & 0x78) >> 3) != 0) {
    errorCodeDecoder = TLV_DECODE_VALUE_DOESNT_MATCH;
    return TLV_DECODE_VALUE_DOESNT_MATCH;
  }

  protocolconfigurationoptions->configurationprotol = (*(buffer + decoded) >> 1) & 0x7;
  decoded++;

  //IES_DECODE_U16(protocolconfigurationoptions->protocolid, *(buffer + decoded));
  protocolconfigurationoptions->num_protocol_id_or_container_id = 0;
  while ((len - decoded) > 0) {
    IES_DECODE_U16(buffer, decoded, protocolconfigurationoptions->protocolid[protocolconfigurationoptions->num_protocol_id_or_container_id]);
    DECODE_U8(buffer + decoded, protocolconfigurationoptions->lengthofprotocolid[protocolconfigurationoptions->num_protocol_id_or_container_id], decoded);

    if (protocolconfigurationoptions->lengthofprotocolid[protocolconfigurationoptions->num_protocol_id_or_container_id] > 0) {
      if ((decode_result = decode_octet_string(&protocolconfigurationoptions->protocolidcontents[protocolconfigurationoptions->num_protocol_id_or_container_id],
                         protocolconfigurationoptions->lengthofprotocolid[protocolconfigurationoptions->num_protocol_id_or_container_id], buffer + decoded, len - decoded)) < 0) {
        return decode_result;
      } else {
        decoded += decode_result;
      }
    } else {
      protocolconfigurationoptions->protocolidcontents[protocolconfigurationoptions->num_protocol_id_or_container_id].length = 0;
      protocolconfigurationoptions->protocolidcontents[protocolconfigurationoptions->num_protocol_id_or_container_id].value = NULL;
    }
    protocolconfigurationoptions->num_protocol_id_or_container_id += 1;
  }

#if defined (NAS_DEBUG)
  dump_protocol_configuration_options_xml(protocolconfigurationoptions, iei);
#endif
  return decoded;
}
int encode_protocol_configuration_options(ProtocolConfigurationOptions *protocolconfigurationoptions, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint8_t  num_protocol_id_or_container_id = 0;
  uint32_t encoded = 0;
  int encode_result;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, PROTOCOL_CONFIGURATION_OPTIONS_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_protocol_configuration_options_xml(protocolconfigurationoptions, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = 0x00 | (1 << 7) |
                        (protocolconfigurationoptions->configurationprotol & 0x7);
  encoded++;

  while (num_protocol_id_or_container_id < protocolconfigurationoptions->num_protocol_id_or_container_id) {

    IES_ENCODE_U16(buffer, encoded, protocolconfigurationoptions->protocolid[num_protocol_id_or_container_id]);
    *(buffer + encoded) = protocolconfigurationoptions->lengthofprotocolid[num_protocol_id_or_container_id];
    encoded++;

    if ((encode_result = encode_octet_string(&protocolconfigurationoptions->protocolidcontents[num_protocol_id_or_container_id],
    		buffer + encoded,
    		len - encoded)) < 0)
      return encode_result;
    else
      encoded += encode_result;

    num_protocol_id_or_container_id += 1;
  }
  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_protocol_configuration_options_xml(ProtocolConfigurationOptions *protocolconfigurationoptions, uint8_t iei)
{
  int   i;
  printf("<Protocol Configuration Options>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Configuration protol>%u</Configuration protol>\n", protocolconfigurationoptions->configurationprotol);
  i = 0;
  while (i < protocolconfigurationoptions->num_protocol_id_or_container_id) {
    printf("        <Protocol ID>%u</Protocol ID>\n", protocolconfigurationoptions->protocolid[i]);
    printf("        <Length of protocol ID>%u</Length of protocol ID>\n", protocolconfigurationoptions->lengthofprotocolid[i]);
    printf("        %s",dump_octet_string_xml(&protocolconfigurationoptions->protocolidcontents[i]));
    i++;
  }
  printf("</Protocol Configuration Options>\n");
}

