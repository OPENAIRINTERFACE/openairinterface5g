#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EsmInformationRequest.h"

int decode_esm_information_request(esm_information_request_msg *esm_information_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, ESM_INFORMATION_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  return decoded;
}

int encode_esm_information_request(esm_information_request_msg *esm_information_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ESM_INFORMATION_REQUEST_MINIMUM_LENGTH, len);

  return encoded;
}

