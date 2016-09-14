#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "AdditionalUpdateType.h"

int decode_additional_update_type(AdditionalUpdateType *additionalupdatetype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  *additionalupdatetype = *buffer & 0x1;
  decoded++;
#if defined (NAS_DEBUG)
  dump_additional_update_type_xml(additionalupdatetype, iei);
#endif
  return decoded;
}

int encode_additional_update_type(AdditionalUpdateType *additionalupdatetype, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  return 0;
}
void dump_additional_update_type_xml(AdditionalUpdateType *additionalupdatetype, uint8_t iei)
{
  printf("<Additional Update Type>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <AUTV>%u</AUTV>\n", *additionalupdatetype);
  printf("</Additional Update Type>\n");
}

