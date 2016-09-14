#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "AuthenticationRequest.h"

int decode_authentication_request(authentication_request_msg *authentication_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, AUTHENTICATION_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_u8_nas_key_set_identifier(&authentication_request->naskeysetidentifierasme, 0, *(buffer + decoded) >> 4, len - decoded)) < 0)
    return decoded_result;

  decoded++;

  if ((decoded_result = decode_authentication_parameter_rand(&authentication_request->authenticationparameterrand, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  if ((decoded_result = decode_authentication_parameter_autn(&authentication_request->authenticationparameterautn, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  return decoded;
}

int encode_authentication_request(authentication_request_msg *authentication_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, AUTHENTICATION_REQUEST_MINIMUM_LENGTH, len);

  *(buffer + encoded) = ((encode_u8_nas_key_set_identifier(&authentication_request->naskeysetidentifierasme) & 0x0f) << 4) | 0x00;
  encoded++;

  if ((encode_result =
         encode_authentication_parameter_rand(&authentication_request->authenticationparameterrand,
             0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  if ((encode_result =
         encode_authentication_parameter_autn(&authentication_request->authenticationparameterautn,
             0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}

