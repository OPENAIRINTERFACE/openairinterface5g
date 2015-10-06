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
#include "EpsQualityOfService.h"

static int decode_eps_qos_bit_rates(EpsQoSBitRates* epsqosbitrates, const uint8_t *buffer)
{
  int decoded = 0;
  epsqosbitrates->maxBitRateForUL = *(buffer + decoded);
  decoded++;
  epsqosbitrates->maxBitRateForDL = *(buffer + decoded);
  decoded++;
  epsqosbitrates->guarBitRateForUL = *(buffer + decoded);
  decoded++;
  epsqosbitrates->guarBitRateForDL = *(buffer + decoded);
  decoded++;
  return decoded;
}

int decode_eps_quality_of_service(EpsQualityOfService *epsqualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len)
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
  epsqualityofservice->qci = *(buffer + decoded);
  decoded++;

  if ( ielen > 2 + (iei > 0) ? 1 : 0 ) {
    /* bitRates is present */
    epsqualityofservice->bitRatesPresent = 1;
    decoded += decode_eps_qos_bit_rates(&epsqualityofservice->bitRates,
                                        buffer + decoded);
  } else {
    /* bitRates is not present */
    epsqualityofservice->bitRatesPresent = 0;
  }

  if ( ielen > 6 + (iei > 0) ? 1 : 0 ) {
    /* bitRatesExt is present */
    epsqualityofservice->bitRatesExtPresent = 1;
    decoded += decode_eps_qos_bit_rates(&epsqualityofservice->bitRatesExt,
                                        buffer + decoded);
  } else {
    /* bitRatesExt is not present */
    epsqualityofservice->bitRatesExtPresent = 0;
  }

#if defined (NAS_DEBUG)
  dump_eps_quality_of_service_xml(epsqualityofservice, iei);
#endif
  return decoded;
}

static int encode_eps_qos_bit_rates(const EpsQoSBitRates* epsqosbitrates, uint8_t *buffer)
{
  int encoded = 0;
  *(buffer + encoded) = epsqosbitrates->maxBitRateForUL;
  encoded ++;
  *(buffer + encoded) = epsqosbitrates->maxBitRateForDL;
  encoded ++;
  *(buffer + encoded) = epsqosbitrates->guarBitRateForUL;
  encoded ++;
  *(buffer + encoded) = epsqosbitrates->guarBitRateForDL;
  encoded ++;
  return encoded;
}

int encode_eps_quality_of_service(EpsQualityOfService *epsqualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, EPS_QUALITY_OF_SERVICE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_eps_quality_of_service_xml(epsqualityofservice, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = epsqualityofservice->qci;
  encoded++;

  if (epsqualityofservice->bitRatesPresent) {
    encoded += encode_eps_qos_bit_rates(&epsqualityofservice->bitRates,
                                        buffer + encoded);
  }

  if (epsqualityofservice->bitRatesExtPresent) {
    encoded += encode_eps_qos_bit_rates(&epsqualityofservice->bitRatesExt,
                                        buffer + encoded);
  }

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_eps_quality_of_service_xml(EpsQualityOfService *epsqualityofservice, uint8_t iei)
{
  printf("<Eps Quality Of Service>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <QCI>%u</QCI>\n", epsqualityofservice->qci);

  if (epsqualityofservice->bitRatesPresent) {
    printf("    <bitRates>\n");
    printf("        <maxBitRateForUL>%u</maxBitRateForUL>\n",
           epsqualityofservice->bitRates.maxBitRateForUL);
    printf("        <maxBitRateForDL>%u</maxBitRateForDL>\n",
           epsqualityofservice->bitRates.maxBitRateForDL);
    printf("        <guarBitRateForUL>%u</guarBitRateForUL>\n",
           epsqualityofservice->bitRates.guarBitRateForUL);
    printf("        <guarBitRateForDL>%u</guarBitRateForDL>\n",
           epsqualityofservice->bitRates.guarBitRateForDL);
    printf("    </bitRates>\n");
  }

  if (epsqualityofservice->bitRatesExtPresent) {
    printf("    <bitRatesExt>\n");
    printf("        <maxBitRateForUL>%u</maxBitRateForUL>\n",
           epsqualityofservice->bitRatesExt.maxBitRateForUL);
    printf("        <maxBitRateForDL>%u</maxBitRateForDL>\n",
           epsqualityofservice->bitRatesExt.maxBitRateForDL);
    printf("        <guarBitRateForUL>%u</guarBitRateForUL>\n",
           epsqualityofservice->bitRatesExt.guarBitRateForUL);
    printf("        <guarBitRateForDL>%u</guarBitRateForDL>\n",
           epsqualityofservice->bitRatesExt.guarBitRateForDL);
    printf("    </bitRatesExt>\n");
  }

  printf("</Eps Quality Of Service>\n");
}

#define EPS_QOS_BIT_RATE_MAX  262144  // 256 Mbps
int eps_qos_bit_rate_value(uint8_t br)
{
  if (br < 0b00000001) {
    return (EPS_QOS_BIT_RATE_MAX);
  } else if ( (br > 0b00000000) && (br < 0b01000000) ) {
    return (br);
  } else if ( (br > 0b00111111) && (br < 0b10000000) ) {
    return (64 + (br - 0b01000000) * 8);
  } else if ( (br > 0b01111111) && (br < 0b11111111) ) {
    return (576 + (br - 0b10000000) * 64);
  } else {
    return (0);
  }
}

int eps_qos_bit_rate_ext_value(uint8_t br)
{
  if ( (br > 0b00000000) && (br < 0b01001011) ) {
    return (8600 + br * 100);
  } else if ( (br > 0b01001010) && (br < 0b10111011) ) {
    return (16384 + (br - 0b01001010) * 1024);
  } else if ( (br > 0b10111010) && (br < 0b11111011) ) {
    return (131072 + (br - 0b10111010) * 2048);
  } else {
    return (-1);
  }
}

