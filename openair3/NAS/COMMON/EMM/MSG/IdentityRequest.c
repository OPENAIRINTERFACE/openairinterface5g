#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "IdentityRequest.h"

int decode_identity_request(identity_request_msg *identity_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, IDENTITY_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_u8_identity_type_2(&identity_request->identitytype, 0, *(buffer + decoded) >> 4, len - decoded)) < 0)
    return decoded_result;

  decoded++;
  return decoded;
}

int encode_identity_request(identity_request_msg *identity_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, IDENTITY_REQUEST_MINIMUM_LENGTH, len);

  *(buffer + encoded) = encode_u8_identity_type_2(&identity_request->identitytype) & 0x0f;
  encoded++;
  return encoded;
}

