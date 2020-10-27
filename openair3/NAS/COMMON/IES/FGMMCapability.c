/*! \file FGSMobileIdentity.c

\brief 5GS Mobile Identity for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "FGMMCapability.h"



int encode_5gmm_capability(FGMMCapability *fgmmcapability, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  if(iei){
    *buffer = fgmmcapability->iei;
     encoded++;
     *(buffer+encoded) = fgmmcapability->length;
     encoded++;
     *(buffer+encoded) = fgmmcapability->value;
     encoded++;
  }
  return encoded;
}

