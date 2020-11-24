/*! \file RegistrationAccept.c

\brief 5GS registration accept procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "RegistrationAccept.h"
#include "assertions.h"

int decode_registration_accept(registration_accept_msg *registration_accept, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  /* Decoding mandatory fields */
  if ((decoded_result = decode_fgs_registration_result(&registration_accept->fgsregistrationresult, 0, *(buffer + decoded), len - decoded)) < 0)
    return decoded_result;

  decoded += decoded_result;

  // todo ,Decoding optional fields
  return decoded;
}

int encode_registration_accept(registration_accept_msg *registration_accept, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;

  LOG_FUNC_IN;


  *(buffer + encoded) = encode_fgs_registration_result(&registration_accept->fgsregistrationresult);
  encoded = encoded + 2;

  // todo ,Encoding optional fields
  LOG_FUNC_RETURN(encoded);
}

