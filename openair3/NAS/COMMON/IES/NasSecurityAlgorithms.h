/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef NAS_SECURITY_ALGORITHMS_H_
#define NAS_SECURITY_ALGORITHMS_H_

#define NAS_SECURITY_ALGORITHMS_MINIMUM_LENGTH 1
#define NAS_SECURITY_ALGORITHMS_MAXIMUM_LENGTH 2

typedef struct NasSecurityAlgorithms_tag {
#define NAS_SECURITY_ALGORITHMS_EEA0  0b000
#define NAS_SECURITY_ALGORITHMS_EEA1  0b001
#define NAS_SECURITY_ALGORITHMS_EEA2  0b010
#define NAS_SECURITY_ALGORITHMS_EEA3  0b011
#define NAS_SECURITY_ALGORITHMS_EEA4  0b100
#define NAS_SECURITY_ALGORITHMS_EEA5  0b101
#define NAS_SECURITY_ALGORITHMS_EEA6  0b110
#define NAS_SECURITY_ALGORITHMS_EEA7  0b111
  uint8_t  typeofcipheringalgorithm:3;
#define NAS_SECURITY_ALGORITHMS_EIA0  0b000
#define NAS_SECURITY_ALGORITHMS_EIA1  0b001
#define NAS_SECURITY_ALGORITHMS_EIA2  0b010
#define NAS_SECURITY_ALGORITHMS_EIA3  0b011
#define NAS_SECURITY_ALGORITHMS_EIA4  0b100
#define NAS_SECURITY_ALGORITHMS_EIA5  0b101
#define NAS_SECURITY_ALGORITHMS_EIA6  0b110
#define NAS_SECURITY_ALGORITHMS_EIA7  0b111
  uint8_t  typeofintegrityalgorithm:3;
} NasSecurityAlgorithms;

int encode_nas_security_algorithms(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_nas_security_algorithms_xml(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei);

int decode_nas_security_algorithms(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* NAS SECURITY ALGORITHMS_H_ */

