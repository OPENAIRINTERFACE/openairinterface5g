/*! \file SpareHalfOctet.c

\brief registration request procedures for gNB
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
#include "SpareHalfOctet.h"

int decode_spare_half_octet(SpareHalfOctet *sparehalfoctet, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  return 0;
}

int encode_spare_half_octet(SpareHalfOctet *sparehalfoctet, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  return 0;
}

