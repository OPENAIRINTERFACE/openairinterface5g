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

#ifndef NAS_REQUEST_TYPE_H_
#define NAS_REQUEST_TYPE_H_

#define REQUEST_TYPE_MINIMUM_LENGTH 1
#define REQUEST_TYPE_MAXIMUM_LENGTH 1

#define REQUEST_TYPE_INITIAL_REQUEST  0b001
#define REQUEST_TYPE_HANDOVER   0b010
#define REQUEST_TYPE_UNUSED   0b011
#define REQUEST_TYPE_EMERGENCY    0b100
typedef uint8_t RequestType;

int encode_request_type(RequestType *requesttype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_request_type_xml(RequestType *requesttype, uint8_t iei);

uint8_t encode_u8_request_type(RequestType *requesttype);

int decode_request_type(RequestType *requesttype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_request_type(RequestType *requesttype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* NAS_REQUEST_TYPE_H_ */

