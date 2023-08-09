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

#ifndef TLV_DECODER_H_
#define TLV_DECODER_H_

#include <arpa/inet.h>  // ntohl, ntohs
#include "nas_log.h"

#ifndef NAS_DEBUG
//# define NAS_DEBUG 1
#endif

#define DECODE_U8(bUFFER, vALUE, sIZE)    \
    vALUE = *(uint8_t*)(bUFFER);    \
    sIZE += sizeof(uint8_t)

#define DECODE_U16(bUFFER, vALUE, sIZE)   \
    vALUE = ntohs(*(uint16_t*)(bUFFER));  \
    sIZE += sizeof(uint16_t)

#define DECODE_U24(bUFFER, vALUE, sIZE)   \
    vALUE = ntohl(*(uint32_t*)(bUFFER)) >> 8; \
    sIZE += sizeof(uint8_t) + sizeof(uint16_t)

#define DECODE_U32(bUFFER, vALUE, sIZE) \
  {                                     \
    uint32_t v;                         \
    memcpy(&v, bUFFER, sizeof(v));      \
    vALUE = ntohl(v);                   \
  }                                     \
  sIZE += sizeof(uint32_t)

#if (BYTE_ORDER == LITTLE_ENDIAN)
# define DECODE_LENGTH_U16(bUFFER, vALUE, sIZE)          \
    vALUE = ((*(bUFFER)) << 8) | (*((bUFFER) + 1));      \
    sIZE += sizeof(uint16_t)
#else
# define DECODE_LENGTH_U16(bUFFER, vALUE, sIZE)          \
    vALUE = (*(bUFFER)) | (*((bUFFER) + 1) << 8);        \
    sIZE += sizeof(uint16_t)
#endif

#define IES_DECODE_U8(bUFFER, dECODED, vALUE) \
    DECODE_U8(bUFFER + dECODED, vALUE, dECODED)

#define IES_DECODE_U16(bUFFER, dECODED, vALUE)  \
    DECODE_U16(bUFFER + dECODED, vALUE, dECODED)

#define IES_DECODE_U24(bUFFER, dECODED, vALUE)  \
    DECODE_U24(bUFFER + dECODED, vALUE, dECODED)

#define IES_DECODE_U32(bUFFER, dECODED, vALUE)  \
    DECODE_U32(bUFFER + dECODED, vALUE, dECODED)

typedef enum {
  TLV_DECODE_ERROR_OK                     =  0,
  TLV_DECODE_UNEXPECTED_IEI               = -1,
  TLV_DECODE_MANDATORY_FIELD_NOT_PRESENT  = -2,
  TLV_DECODE_VALUE_DOESNT_MATCH           = -3,

  /* Fatal errors - received message should not be processed */
  TLV_DECODE_WRONG_MESSAGE_TYPE           = -10,
  TLV_DECODE_PROTOCOL_NOT_SUPPORTED       = -11,
  TLV_DECODE_BUFFER_TOO_SHORT             = -12,
  TLV_DECODE_BUFFER_NULL                  = -13,
  TLV_DECODE_MAC_MISMATCH                 = -14,
} tlv_decoder_error_code;

/* Defines error code limit below which received message should be discarded
 * because it cannot be further processed */
#define TLV_DECODE_FATAL_ERROR  (TLV_DECODE_VALUE_DOESNT_MATCH)

extern int errorCodeDecoder;

void tlv_decode_perror(void);

#define CHECK_PDU_POINTER_AND_LENGTH_DECODER(bUFFER, mINIMUMlENGTH, lENGTH)    \
  if (bUFFER == NULL)                                                    \
        {                                                                      \
                printf("(%s:%d) Got NULL pointer for the payload\n",           \
                __FILE__, __LINE__);                                           \
                errorCodeDecoder = TLV_DECODE_BUFFER_NULL;                     \
                LOG_FUNC_RETURN(TLV_DECODE_BUFFER_NULL);                       \
        }                                                                      \
        if (lENGTH < mINIMUMlENGTH)                                            \
        {                                                                      \
                printf("(%s:%d) Expecting at least %d bytes, got %u\n",        \
                      __FILE__, __LINE__, mINIMUMlENGTH, lENGTH);              \
                errorCodeDecoder = TLV_DECODE_BUFFER_TOO_SHORT;                \
                LOG_FUNC_RETURN(TLV_DECODE_BUFFER_TOO_SHORT);                  \
        }

#define CHECK_LENGTH_DECODER(bUFFERlENGTH, lENGTH)                             \
        if (bUFFERlENGTH < lENGTH)                                             \
        {                                                                      \
                errorCodeDecoder = TLV_DECODE_BUFFER_TOO_SHORT;                \
                LOG_FUNC_RETURN(TLV_DECODE_BUFFER_TOO_SHORT);                  \
        }

#define CHECK_MESSAGE_TYPE(mESSAGE_tYPE, bUFFER)                               \
        {                                                                      \
                if (mESSAGE_tYPE != bUFFER)                                    \
                {                                                              \
                        errorCodeDecoder = TLV_DECODE_WRONG_MESSAGE_TYPE;      \
                        LOG_FUNC_RETURN(errorCodeDecoder);                     \
                }                                                              \
        }

#define CHECK_IEI_DECODER(iEI, bUFFER)                                  \
        if(iEI != bUFFER)                                               \
        {                                                               \
                printf("IEI is different than the one expected."        \
                "(Got: 0x%x, expecting: 0x%x)\n", bUFFER, iEI);          \
                errorCodeDecoder = TLV_DECODE_UNEXPECTED_IEI;           \
                LOG_FUNC_RETURN(TLV_DECODE_UNEXPECTED_IEI);             \
        }

#endif /* define (TLV_DECODER_H_) */

