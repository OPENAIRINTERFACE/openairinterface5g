#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "KsiAndSequenceNumber.h"

int decode_ksi_and_sequence_number(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ksiandsequencenumber->ksi = (*(buffer + decoded) >> 5) & 0x7;
  ksiandsequencenumber->sequencenumber = *(buffer + decoded) & 0x1f;
  decoded++;
#if defined (NAS_DEBUG)
  dump_ksi_and_sequence_number_xml(ksiandsequencenumber, iei);
#endif
  return decoded;
}

int encode_ksi_and_sequence_number(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, KSI_AND_SEQUENCE_NUMBER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_ksi_and_sequence_number_xml(ksiandsequencenumber, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 | ((ksiandsequencenumber->ksi & 0x7) << 5) |
                        (ksiandsequencenumber->sequencenumber & 0x1f);
  encoded++;
  return encoded;
}

void dump_ksi_and_sequence_number_xml(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei)
{
  printf("<Ksi And Sequence Number>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <KSI>%u</KSI>\n", ksiandsequencenumber->ksi);
  printf("    <Sequence number>%u</Sequence number>\n", ksiandsequencenumber->sequencenumber);
  printf("</Ksi And Sequence Number>\n");
}

