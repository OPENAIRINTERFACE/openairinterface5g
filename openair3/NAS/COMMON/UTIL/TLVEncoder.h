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
#ifndef TLV_ENCODER_H_
#define TLV_ENCODER_H_

#include <arpa/inet.h>  // htonl, htons

#define ENCODE_U8(buffer, value, size)    \
    *(uint8_t*)(buffer) = value;    \
    size += sizeof(uint8_t)

#define ENCODE_U16(buffer, value, size)   \
    *(uint16_t*)(buffer) = htons(value);  \
   size += sizeof(uint16_t)

#define ENCODE_U24(buffer, value, size)   \
    *(uint32_t*)(buffer) = htonl(value);  \
    size += sizeof(uint8_t) + sizeof(uint16_t)

#define ENCODE_U32(buffer, value, size)   \
    *(uint32_t*)(buffer) = htonl(value);  \
    size += sizeof(uint32_t)

#define IES_ENCODE_U8(buffer, encoded, value)   \
    ENCODE_U8(buffer + encoded, value, encoded)

#define IES_ENCODE_U16(buffer, encoded, value)    \
    ENCODE_U16(buffer + encoded, value, encoded)

#define IES_ENCODE_U24(buffer, encoded, value)    \
    ENCODE_U24(buffer + encoded, value, encoded)

#define IES_ENCODE_U32(buffer, encoded, value)    \
    ENCODE_U32(buffer + encoded, value, encoded)

typedef enum {
  TLV_ENCODE_ERROR_OK                      =  0,
  TLV_ENCODE_VALUE_DOESNT_MATCH            = -1,
  /* Fatal errors - message should not be sent */
  TLV_ENCODE_OCTET_STRING_TOO_LONG_FOR_IEI = -10,
  TLV_ENCODE_WRONG_MESSAGE_TYPE            = -11,
  TLV_ENCODE_PROTOCOL_NOT_SUPPORTED        = -12,
  TLV_ENCODE_BUFFER_TOO_SHORT              = -13,
  TLV_ENCODE_BUFFER_NULL                   = -14,
} tlv_encoder_error_code;

/* Defines error code limit below which message should be sent because
 * it cannot be further processed */
#define TLV_ENCODE_FATAL_ERROR  (TLV_ENCODE_VALUE_DOESNT_MATCH)

extern int errorCodeEncoder;

void tlv_encode_perror(void);

#define CHECK_PDU_POINTER_AND_LENGTH_ENCODER(bUFFER, mINIMUMlENGTH, lENGTH)    \
  if (bUFFER == NULL)                                                    \
        {                                                                      \
                printf("Got NULL pointer for the payload\n");                  \
                errorCodeEncoder = TLV_ENCODE_BUFFER_NULL;                     \
                return TLV_ENCODE_BUFFER_NULL;                                 \
        }                                                                      \
        if (lENGTH < mINIMUMlENGTH)                                            \
        {                                                                      \
                printf("(%s:%d) Expecting at least %d bytes, got %d\n",        \
           __FILE__, __LINE__, mINIMUMlENGTH, lENGTH);             \
                errorCodeEncoder = TLV_ENCODE_BUFFER_TOO_SHORT;                \
                return TLV_ENCODE_BUFFER_TOO_SHORT;                            \
        }

#define CHECK_PDU_POINTER_ENCODER(bUFFER)                                      \
  if (bUFFER == NULL)                                                    \
        {                                                                      \
                printf("Got NULL pointer for the payload\n");                  \
                errorCodeEncoder = TLV_ENCODE_BUFFER_NULL;                     \
                return TLV_ENCODE_BUFFER_NULL;                                 \
        }

#endif /* define (TLV_ENCODER_H_) */

