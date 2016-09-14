#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EsmMessageContainer.h"
#include "nas_log.h"

#define NAS_DEBUG 1

int decode_esm_message_container(EsmMessageContainer *esmmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  int decode_result;
  uint16_t ielen;

  LOG_FUNC_IN;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  DECODE_LENGTH_U16(buffer + decoded, ielen, decoded);

  CHECK_LENGTH_DECODER(len - decoded, ielen);


  if ((decode_result = decode_octet_string(&esmmessagecontainer->esmmessagecontainercontents, ielen, buffer + decoded, len - decoded)) < 0) {
    LOG_FUNC_RETURN(decode_result);
  } else {
    decoded += decode_result;
  }

#if defined (NAS_DEBUG)
  dump_esm_message_container_xml(esmmessagecontainer, iei);
#endif
  LOG_FUNC_RETURN(decoded);
}

int encode_esm_message_container(EsmMessageContainer *esmmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  int32_t encode_result;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_esm_message_container_xml(esmmessagecontainer, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);

  //encoded += 2;
  //if ((encode_result = encode_octet_string(&esmmessagecontainer->esmmessagecontainercontents, buffer + sizeof(uint16_t), len - sizeof(uint16_t))) < 0)
  if ((encode_result = encode_octet_string(&esmmessagecontainer->esmmessagecontainercontents, lenPtr + sizeof(uint16_t), len - sizeof(uint16_t))) < 0)
    return encode_result;
  else
    encoded += encode_result;

  ENCODE_U16(lenPtr, encode_result, encoded);
#if 0
  lenPtr[1] = (((encoded - 2 - ((iei > 0) ? 1: 0))) & 0x0000ff00) >> 8;
  lenPtr[0] =  ((encoded - 2 - ((iei > 0) ? 1: 0))) & 0x000000ff;
#endif
  return encoded;
}

void dump_esm_message_container_xml(EsmMessageContainer *esmmessagecontainer, uint8_t iei)
{
  printf("<Esm Message Container>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("%s", dump_octet_string_xml(&esmmessagecontainer->esmmessagecontainercontents));
  printf("</Esm Message Container>\n");
}

