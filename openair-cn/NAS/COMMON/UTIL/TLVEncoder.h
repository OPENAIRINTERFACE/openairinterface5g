/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

