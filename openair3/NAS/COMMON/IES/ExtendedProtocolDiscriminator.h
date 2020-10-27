/*! \file ExtendedProtocolDiscriminator.h

\brief Extended protocol discriminator for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EXTENDED_PROTOCOL_DISCRIMINATOR_H_
#define EXTENDED_PROTOCOL_DISCRIMINATOR_H_

typedef uint8_t ExtendedProtocolDiscriminator;

int encode_ex_protocol_discriminator(ExtendedProtocolDiscriminator *exprotocoldiscriminator, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_ex_protocol_discriminator(ExtendedProtocolDiscriminator *exprotocoldiscriminator, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* EXTENDED PROTOCOL DISCRIMINATOR_H_ */

