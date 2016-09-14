#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "CipheringKeySequenceNumber.h"

int decode_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, CIPHERING_KEY_SEQUENCE_NUMBER_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *cipheringkeysequencenumber = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_ciphering_key_sequence_number_xml(cipheringkeysequencenumber, iei);
#endif
  return decoded;
}

int decode_u8_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *cipheringkeysequencenumber = *buffer & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_ciphering_key_sequence_number_xml(cipheringkeysequencenumber, iei);
#endif
  return decoded;
}

int encode_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, CIPHERING_KEY_SEQUENCE_NUMBER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_ciphering_key_sequence_number_xml(cipheringkeysequencenumber, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*cipheringkeysequencenumber & 0x7);
  encoded++;
  return encoded;
}

uint8_t encode_u8_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
  dump_ciphering_key_sequence_number_xml(cipheringkeysequencenumber, 0);
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*cipheringkeysequencenumber & 0x7);
  encoded++;

  return bufferReturn;
}

void dump_ciphering_key_sequence_number_xml(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei)
{
  printf("<Ciphering Key Sequence Number>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Key sequence>%u</Key sequence>\n", *cipheringkeysequencenumber);
  printf("</Ciphering Key Sequence Number>\n");
}

