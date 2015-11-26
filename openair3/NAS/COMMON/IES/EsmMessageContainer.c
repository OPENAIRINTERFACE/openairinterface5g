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

