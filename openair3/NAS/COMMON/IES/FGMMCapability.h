/*! \file FGSMobileIdentity.h

\brief 5GS Mobile Identity for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef FGMM_CAPABILITY_H_
#define FGMM_CAPABILITY_H_


typedef struct {
  uint8_t iei;
  uint8_t length;
  uint8_t value;
} FGMMCapability;


int encode_5gmm_capability(FGMMCapability *fgmmcapability, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* FGMM_CAPABILITY_H_ */

