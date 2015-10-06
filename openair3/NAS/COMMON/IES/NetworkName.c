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
#include "NetworkName.h"

int decode_network_name(NetworkName *networkname, uint8_t iei, uint8_t *buffer, uint32_t len)
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

  if (((*buffer >> 7) & 0x1) != 1) {
    errorCodeDecoder = TLV_DECODE_VALUE_DOESNT_MATCH;
    return TLV_DECODE_VALUE_DOESNT_MATCH;
  }

  networkname->codingscheme = (*(buffer + decoded) >> 5) & 0x7;
  networkname->addci = (*(buffer + decoded) >> 4) & 0x1;
  networkname->numberofsparebitsinlastoctet = (*(buffer + decoded) >> 1) & 0x7;

  if ((decode_result = decode_octet_string(&networkname->textstring, ielen, buffer + decoded, len - decoded)) < 0)
    return decode_result;
  else
    decoded += decode_result;

#if defined (NAS_DEBUG)
  dump_network_name_xml(networkname, iei);
#endif
  return decoded;
}
int encode_network_name(NetworkName *networkname, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  int encode_result;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, NETWORK_NAME_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_network_name_xml(networkname, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = 0x00 | (1 << 7) |
                        ((networkname->codingscheme & 0x7) << 4) |
                        ((networkname->addci & 0x1) << 3) |
                        (networkname->numberofsparebitsinlastoctet & 0x7);
  encoded++;

  if ((encode_result = encode_octet_string(&networkname->textstring, buffer + encoded, len - encoded)) < 0)
    return encode_result;
  else
    encoded += encode_result;

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_network_name_xml(NetworkName *networkname, uint8_t iei)
{
  printf("<Network Name>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Coding scheme>%u</Coding scheme>\n", networkname->codingscheme);
  printf("    <Add CI>%u</Add CI>\n", networkname->addci);
  printf("    <Number of spare bits in last octet>%u</Number of spare bits in last octet>\n", networkname->numberofsparebitsinlastoctet);
  printf("%s", dump_octet_string_xml(&networkname->textstring));
  printf("</Network Name>\n");
}

