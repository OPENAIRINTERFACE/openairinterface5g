/*! \file ExtendedProtocolDiscriminator.c

\brief Extended protocol discriminator for registration request procedures
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
#include "ExtendedProtocolDiscriminator.h"

int encode_ex_protocol_discriminator(ExtendedProtocolDiscriminator *exprotocoldiscriminator, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  return 0;
}

int decode_ex_protocol_discriminator(ExtendedProtocolDiscriminator *exprotocoldiscriminator, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  return 0;
}

