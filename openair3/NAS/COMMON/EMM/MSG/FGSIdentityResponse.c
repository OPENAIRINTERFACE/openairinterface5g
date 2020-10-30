/*! \file FGSIdentityResponse.c

\brief identity response procedures for gNB
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

#include "FGSIdentityResponse.h"


int encode_identiy_response(fgs_identiy_response_msg *fgs_identity_reps, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  if ((encode_result =
         encode_5gs_mobile_identity(&fgs_identity_reps->fgsmobileidentity, 0, buffer +
                                    encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}


