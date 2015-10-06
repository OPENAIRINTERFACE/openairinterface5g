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
#include "ApnAggregateMaximumBitRate.h"

int decode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);
  apnaggregatemaximumbitrate->apnambrfordownlink = *(buffer + decoded);
  decoded++;
  apnaggregatemaximumbitrate->apnambrforuplink = *(buffer + decoded);
  decoded++;

  if (ielen >= 4) {
    apnaggregatemaximumbitrate->apnambrfordownlink_extended = *(buffer + decoded);
    decoded++;
    apnaggregatemaximumbitrate->apnambrforuplink_extended = *(buffer + decoded);
    decoded++;

    if (ielen >= 6) {
      apnaggregatemaximumbitrate->apnambrfordownlink_extended2 = *(buffer + decoded);
      decoded++;
      apnaggregatemaximumbitrate->apnambrforuplink_extended2 = *(buffer + decoded);
      decoded++;
    }
  }

#if defined (NAS_DEBUG)
  dump_apn_aggregate_maximum_bit_rate_xml(apnaggregatemaximumbitrate, iei);
#endif
  return decoded;
}
int encode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, APN_AGGREGATE_MAXIMUM_BIT_RATE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_apn_aggregate_maximum_bit_rate_xml(apnaggregatemaximumbitrate, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrfordownlink;
  encoded++;
  *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrforuplink;
  encoded++;

  if (apnaggregatemaximumbitrate->extensions & APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT) {
    *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrfordownlink_extended;
    encoded++;
    *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrforuplink_extended;
    encoded++;

    if (apnaggregatemaximumbitrate->extensions & APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT) {
      *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrfordownlink_extended2;
      encoded++;
      *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrforuplink_extended2;
      encoded++;
    }
  }

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_apn_aggregate_maximum_bit_rate_xml(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei)
{
  printf("<Apn Aggregate Maximum Bit Rate>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <APN AMBR for downlink>%u</APN AMBR for downlink>\n", apnaggregatemaximumbitrate->apnambrfordownlink);
  printf("    <APN AMBR for uplink>%u</APN AMBR for uplink>\n", apnaggregatemaximumbitrate->apnambrforuplink);

  if (apnaggregatemaximumbitrate->extensions & APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT) {
    printf("    <APN AMBR extended for downlink>%u</APN AMBR for downlink>\n", apnaggregatemaximumbitrate->apnambrfordownlink_extended);
    printf("    <APN AMBR extended for uplink>%u</APN AMBR for uplink>\n", apnaggregatemaximumbitrate->apnambrforuplink_extended);

    if (apnaggregatemaximumbitrate->extensions & APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT) {
      printf("    <APN AMBR extended2 for downlink>%u</APN AMBR for downlink>\n", apnaggregatemaximumbitrate->apnambrfordownlink_extended);
      printf("    <APN AMBR extended2 for uplink>%u</APN AMBR for uplink>\n", apnaggregatemaximumbitrate->apnambrforuplink_extended);
    }
  }

  printf("</Apn Aggregate Maximum Bit Rate>\n");
}

