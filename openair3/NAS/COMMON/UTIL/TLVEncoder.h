/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
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

