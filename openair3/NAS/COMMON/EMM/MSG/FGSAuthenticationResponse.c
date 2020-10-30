/*! \file FGSAuthenticationResponse.c

\brief authentication response procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "nas_log.h"

#include "FGSAuthenticationResponse.h"


int encode_fgs_authentication_response(fgs_authentication_response_msg *authentication_response, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;


  if ((encode_result =
         encode_authentication_response_parameter(&authentication_response->authenticationresponseparameter,
           AUTHENTICATION_RESPONSE_PARAMETER_IEI, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}

