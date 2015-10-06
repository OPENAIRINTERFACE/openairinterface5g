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
#include "TimeZoneAndTime.h"

int decode_time_zone_and_time(TimeZoneAndTime *timezoneandtime, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  timezoneandtime->year = *(buffer + decoded);
  decoded++;
  timezoneandtime->month = *(buffer + decoded);
  decoded++;
  timezoneandtime->day = *(buffer + decoded);
  decoded++;
  timezoneandtime->hour = *(buffer + decoded);
  decoded++;
  timezoneandtime->minute = *(buffer + decoded);
  decoded++;
  timezoneandtime->second = *(buffer + decoded);
  decoded++;
  timezoneandtime->timezone = *(buffer + decoded);
  decoded++;
#if defined (NAS_DEBUG)
  dump_time_zone_and_time_xml(timezoneandtime, iei);
#endif
  return decoded;
}

int encode_time_zone_and_time(TimeZoneAndTime *timezoneandtime, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, TIME_ZONE_AND_TIME_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_time_zone_and_time_xml(timezoneandtime, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = timezoneandtime->year;
  encoded++;
  *(buffer + encoded) = timezoneandtime->month;
  encoded++;
  *(buffer + encoded) = timezoneandtime->day;
  encoded++;
  *(buffer + encoded) = timezoneandtime->hour;
  encoded++;
  *(buffer + encoded) = timezoneandtime->minute;
  encoded++;
  *(buffer + encoded) = timezoneandtime->second;
  encoded++;
  *(buffer + encoded) = timezoneandtime->timezone;
  encoded++;
  return encoded;
}

void dump_time_zone_and_time_xml(TimeZoneAndTime *timezoneandtime, uint8_t iei)
{
  printf("<Time Zone And Time>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Year>%u</Year>\n", timezoneandtime->year);
  printf("    <Month>%u</Month>\n", timezoneandtime->month);
  printf("    <Day>%u</Day>\n", timezoneandtime->day);
  printf("    <Hour>%u</Hour>\n", timezoneandtime->hour);
  printf("    <Minute>%u</Minute>\n", timezoneandtime->minute);
  printf("    <Second>%u</Second>\n", timezoneandtime->second);
  printf("    <Time Zone>%u</Time Zone>\n", timezoneandtime->timezone);
  printf("</Time Zone And Time>\n");
}

