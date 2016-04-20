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
#include <string.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "UeNetworkCapability.h"

int decode_ue_network_capability(UeNetworkCapability *uenetworkcapability, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  DECODE_U8(buffer + decoded, ielen, decoded);

  memset(uenetworkcapability, 0, sizeof(UeNetworkCapability));

  LOG_TRACE(INFO, "decode_ue_network_capability len = %d",ielen);
  CHECK_LENGTH_DECODER(len - decoded, ielen);
  uenetworkcapability->eea = *(buffer + decoded);
  decoded++;
  uenetworkcapability->eia = *(buffer + decoded);
  decoded++;

  /* Parts below not mandatory and may not be present */
  if (ielen > 2) {
    uenetworkcapability->uea = *(buffer + decoded);
    decoded++;

    if (ielen > 3) {
      uenetworkcapability->ucs2 = (*(buffer + decoded) >> 7) & 0x1;
      uenetworkcapability->uia = *(buffer + decoded) & 0x7f;
      decoded++;
      uenetworkcapability->umts_present =1;
      LOG_TRACE(INFO, "uenetworkcapability decoded UMTS\n");

      if (ielen > 4) {
        uenetworkcapability->spare = (*(buffer + decoded) >> 5) & 0x7;
        uenetworkcapability->csfb  = (*(buffer + decoded) >> 4) & 0x1;
        uenetworkcapability->lpp   = (*(buffer + decoded) >> 3) & 0x1;
        uenetworkcapability->lcs   = (*(buffer + decoded) >> 2) & 0x1;
        uenetworkcapability->srvcc = (*(buffer + decoded) >> 1) & 0x1;
        uenetworkcapability->nf    = *(buffer + decoded) & 0x1;
        decoded++;
        uenetworkcapability->gprs_present =1;
        LOG_TRACE(INFO, "uenetworkcapability decoded GPRS\n");
      }

//#warning "Force GPRS present if UMTS present"
      uenetworkcapability->gprs_present =1;
    }
  }

  LOG_TRACE(INFO, "uenetworkcapability decoded=%u\n", decoded);

  if ((ielen + 2) != decoded) {
    decoded = ielen + 1 + (iei > 0 ? 1 : 0) /* Size of header for this IE */;
    LOG_TRACE(INFO, "uenetworkcapability then decoded=%u\n", decoded);
  }

#if defined (NAS_DEBUG)
  dump_ue_network_capability_xml(uenetworkcapability, iei);
#endif
  return decoded;
}
int encode_ue_network_capability(UeNetworkCapability *uenetworkcapability, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, UE_NETWORK_CAPABILITY_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_ue_network_capability_xml(uenetworkcapability, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = uenetworkcapability->eea;
  encoded++;
  *(buffer + encoded) = uenetworkcapability->eia;
  encoded++;
  LOG_TRACE(INFO, "uenetworkcapability encoded EPS %u\n", encoded);

  if (uenetworkcapability->umts_present) {
    *(buffer + encoded) = uenetworkcapability->uea;
    encoded++;
    *(buffer + encoded) = 0x00 | ((uenetworkcapability->ucs2 & 0x1) << 7) |
                          (uenetworkcapability->uia & 0x7f);
    encoded++;
    LOG_TRACE(INFO, "uenetworkcapability encoded UMTS %u\n", encoded);
  }

  if (uenetworkcapability->gprs_present) {
//#warning "Force gea to 0x60"
    *(buffer + encoded) = 0x00 | 0x60;
    ////((uenetworkcapability->spare & 0x7) << 5) | // spare coded as zero
    //((uenetworkcapability->csfb  & 0x1) << 4) |
    //((uenetworkcapability->lpp   & 0x1) << 3) |
    //((uenetworkcapability->lcs   & 0x1) << 2) |
    //((uenetworkcapability->srvcc & 0x1) << 1) |
    //(uenetworkcapability->nf     & 0x1);
    encoded++;
    LOG_TRACE(INFO, "uenetworkcapability encoded GPRS %u\n", encoded);
  }

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_ue_network_capability_xml(UeNetworkCapability *uenetworkcapability, uint8_t iei)
{
  printf("<Ue Network Capability>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <EEA>%02x</EEA>\n", uenetworkcapability->eea);
  printf("    <EIA>%02x</EIA>\n", uenetworkcapability->eia);
  printf("    <UEA>%02x</UEA>\n", uenetworkcapability->uea);
  printf("    <UCS2>%u</UCS2>\n", uenetworkcapability->ucs2);
  printf("    <UIA>%u</UIA>\n", uenetworkcapability->uia);
  printf("    <SPARE>%u</SPARE>\n", uenetworkcapability->spare);
  printf("    <CSFB>%u</CSFB>\n", uenetworkcapability->csfb);
  printf("    <LPP>%u</LPP>\n", uenetworkcapability->lpp);
  printf("    <LCS>%u</LCS>\n", uenetworkcapability->lcs);
  printf("    <SR VCC>%u</SR VCC>\n", uenetworkcapability->srvcc);
  printf("    <NF>%u<NF/>\n", uenetworkcapability->nf);
  printf("    <UMTS>%u<UMTS/>\n", uenetworkcapability->umts_present);
  printf("    <GPRS>%u<GPRS/>\n", uenetworkcapability->gprs_present);
  printf("</Ue Network Capability>\n");
}

