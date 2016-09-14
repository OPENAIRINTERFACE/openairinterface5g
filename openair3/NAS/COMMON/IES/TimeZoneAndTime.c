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

