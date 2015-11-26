/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "AccessPointName.h"

int decode_access_point_name(AccessPointName *accesspointname, uint8_t iei, uint8_t *buffer, uint32_t len)
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

  if ((decode_result = decode_octet_string(&accesspointname->accesspointnamevalue, ielen, buffer + decoded, len - decoded)) < 0)
    return decode_result;
  else
    decoded += decode_result;

#if defined (NAS_DEBUG)
  dump_access_point_name_xml(accesspointname, iei);
#endif
  return decoded;
}
int encode_access_point_name(AccessPointName *accesspointname, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  int encode_result;
  OctetString  apn_encoded;
  uint32_t     length_index;
  uint32_t     index;
  uint32_t     index_copy;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ACCESS_POINT_NAME_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_access_point_name_xml(accesspointname, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;

  apn_encoded.length = 0;
  apn_encoded.value  = calloc(1, ACCESS_POINT_NAME_MAXIMUM_LENGTH);
  index              = 0; // index on original APN string
  length_index       = 0; // marker where to write partial length
  index_copy         = 1;

  while ((accesspointname->accesspointnamevalue.value[index] != 0) && (index < accesspointname->accesspointnamevalue.length)) {
    if (accesspointname->accesspointnamevalue.value[index] == '.') {
      apn_encoded.value[length_index] = index_copy - length_index - 1;
      length_index = index_copy;
      index_copy   = length_index + 1;
    } else {
      apn_encoded.value[index_copy] = accesspointname->accesspointnamevalue.value[index];
      index_copy++;
    }

    index++;
  }

  apn_encoded.value[length_index] = index_copy - length_index - 1;
  apn_encoded.length = index_copy;

  if ((encode_result = encode_octet_string(&apn_encoded, buffer + encoded, len - encoded)) < 0) {
    free(apn_encoded.value);
    return encode_result;
  } else
    encoded += encode_result;

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);

  free(apn_encoded.value);
  return encoded;
}

void dump_access_point_name_xml(AccessPointName *accesspointname, uint8_t iei)
{
  printf("<Access Point Name>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("%s</Access Point Name>\n",
         dump_octet_string_xml(&accesspointname->accesspointnamevalue));
}

