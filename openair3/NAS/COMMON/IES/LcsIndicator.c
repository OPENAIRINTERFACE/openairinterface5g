#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "LcsIndicator.h"

int decode_lcs_indicator(LcsIndicator *lcsindicator, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  *lcsindicator = *(buffer + decoded);
  decoded++;
#if defined (NAS_DEBUG)
  dump_lcs_indicator_xml(lcsindicator, iei);
#endif
  return decoded;
}

int encode_lcs_indicator(LcsIndicator *lcsindicator, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, LCS_INDICATOR_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_lcs_indicator_xml(lcsindicator, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = *lcsindicator;
  encoded++;
  return encoded;
}

void dump_lcs_indicator_xml(LcsIndicator *lcsindicator, uint8_t iei)
{
  printf("<Lcs Indicator>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <LCS indicator value>%u</LCS indicator value>\n", *lcsindicator);
  printf("</Lcs Indicator>\n");
}

