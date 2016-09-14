#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "TimeZone.h"

int decode_time_zone(TimeZone *timezone, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *timezone = *(buffer + decoded);
  decoded++;
#if defined (NAS_DEBUG)
  dump_time_zone_xml(timezone, iei);
#endif
  return decoded;
}

int encode_time_zone(TimeZone *timezone, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, TIME_ZONE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_time_zone_xml(timezone, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = *timezone;
  encoded++;
  return encoded;
}

void dump_time_zone_xml(TimeZone *timezone, uint8_t iei)
{
  printf("<Time Zone>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Time zone>%u</Time zone>\n", *timezone);
  printf("</Time Zone>\n");
}

