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
#include "LocationAreaIdentification.h"

int decode_location_area_identification(LocationAreaIdentification *locationareaidentification, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  locationareaidentification->mccdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  locationareaidentification->mccdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  locationareaidentification->mncdigit3 = (*(buffer + decoded) >> 4) & 0xf;
  locationareaidentification->mccdigit3 = *(buffer + decoded) & 0xf;
  decoded++;
  locationareaidentification->mncdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  locationareaidentification->mncdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  //IES_DECODE_U16(locationareaidentification->lac, *(buffer + decoded));
  IES_DECODE_U16(buffer, decoded, locationareaidentification->lac);
#if defined (NAS_DEBUG)
  dump_location_area_identification_xml(locationareaidentification, iei);
#endif
  return decoded;
}

int encode_location_area_identification(LocationAreaIdentification *locationareaidentification, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, LOCATION_AREA_IDENTIFICATION_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_location_area_identification_xml(locationareaidentification, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 | ((locationareaidentification->mccdigit2 & 0xf) << 4) |
                        (locationareaidentification->mccdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((locationareaidentification->mncdigit3 & 0xf) << 4) |
                        (locationareaidentification->mccdigit3 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((locationareaidentification->mncdigit2 & 0xf) << 4) |
                        (locationareaidentification->mncdigit1 & 0xf);
  encoded++;
  IES_ENCODE_U16(buffer, encoded, locationareaidentification->lac);
  return encoded;
}

void dump_location_area_identification_xml(LocationAreaIdentification *locationareaidentification, uint8_t iei)
{
  printf("<Location Area Identification>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <MCC digit 2>%u</MCC digit 2>\n", locationareaidentification->mccdigit2);
  printf("    <MCC digit 1>%u</MCC digit 1>\n", locationareaidentification->mccdigit1);
  printf("    <MNC digit 3>%u</MNC digit 3>\n", locationareaidentification->mncdigit3);
  printf("    <MCC digit 3>%u</MCC digit 3>\n", locationareaidentification->mccdigit3);
  printf("    <MNC digit 2>%u</MNC digit 2>\n", locationareaidentification->mncdigit2);
  printf("    <MNC digit 1>%u</MNC digit 1>\n", locationareaidentification->mncdigit1);
  printf("    <LAC>%u</LAC>\n", locationareaidentification->lac);
  printf("</Location Area Identification>\n");
}

