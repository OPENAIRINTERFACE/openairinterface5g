/*! \file FGSNASSecurityModeComplete.c

\brief security mode complete procedures for gNB
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

#include "FGSNASSecurityModeComplete.h"
#include "FGSMobileIdentity.h"
#include "FGCNasMessageContainer.h"

int encode_fgs_security_mode_complete(fgs_security_mode_complete_msg *fgs_security_mode_comp, uint8_t *buffer, uint32_t len)
{
    int encoded = 0;
    int encode_result = 0;

    if ((encode_result =
          encode_5gs_mobile_identity(&fgs_security_mode_comp->fgsmobileidentity, 0x77, buffer +
                                     encoded, len - encoded)) < 0) { //Return in case of error
      return encode_result;
    } else {
      encoded += encode_result;
      if ((encode_result =
            encode_fgc_nas_message_container(&fgs_security_mode_comp->fgsnasmessagecontainer, 0x71, buffer +
                                      encoded, len - encoded)) < 0) {
          return encode_result;
      } else {
        encoded += encode_result;
      }
    }

    return encoded;
}



