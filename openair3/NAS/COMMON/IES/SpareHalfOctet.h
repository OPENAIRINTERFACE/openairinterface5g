/*! \file SpareHalfOctet.h

\brief registration request procedures for gNB
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef SPARE_HALF_OCTET_H_
#define SPARE_HALF_OCTET_H_

#define SPARE_HALF_OCTET_MINIMUM_LENGTH 1
#define SPARE_HALF_OCTET_MAXIMUM_LENGTH 1

typedef uint8_t SpareHalfOctet;

int encode_spare_half_octet(SpareHalfOctet *sparehalfoctet, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_spare_half_octet(SpareHalfOctet *sparehalfoctet, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* SPARE HALF OCTET_H_ */

