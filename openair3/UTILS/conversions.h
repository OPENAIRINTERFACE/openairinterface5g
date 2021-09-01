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

#include "assertions.h"

#ifndef CONVERSIONS_H_
#define CONVERSIONS_H_

/* Endianness conversions for 16 and 32 bits integers from host to network order */
#if (BYTE_ORDER == LITTLE_ENDIAN)
# define hton_int32(x)   \
    (((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) |  \
    ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24))

# define hton_int16(x)   \
    (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8)

# define ntoh_int32_buf(bUF)        \
    ((*(bUF)) << 24) | ((*((bUF) + 1)) << 16) | ((*((bUF) + 2)) << 8)   \
  | (*((bUF) + 3))
#else
# define hton_int32(x) (x)
# define hton_int16(x) (x)
#endif

#define IN_ADDR_TO_BUFFER(X,bUFF) INT32_TO_BUFFER((X).s_addr,(char*)bUFF)

#define IN6_ADDR_TO_BUFFER(X,bUFF)                     \
    do {                                               \
        ((uint8_t*)(bUFF))[0]  = (X).s6_addr[0];  \
        ((uint8_t*)(bUFF))[1]  = (X).s6_addr[1];  \
        ((uint8_t*)(bUFF))[2]  = (X).s6_addr[2];  \
        ((uint8_t*)(bUFF))[3]  = (X).s6_addr[3];  \
        ((uint8_t*)(bUFF))[4]  = (X).s6_addr[4];  \
        ((uint8_t*)(bUFF))[5]  = (X).s6_addr[5];  \
        ((uint8_t*)(bUFF))[6]  = (X).s6_addr[6];  \
        ((uint8_t*)(bUFF))[7]  = (X).s6_addr[7];  \
        ((uint8_t*)(bUFF))[8]  = (X).s6_addr[8];  \
        ((uint8_t*)(bUFF))[9]  = (X).s6_addr[9];  \
        ((uint8_t*)(bUFF))[10] = (X).s6_addr[10]; \
        ((uint8_t*)(bUFF))[11] = (X).s6_addr[11]; \
        ((uint8_t*)(bUFF))[12] = (X).s6_addr[12]; \
        ((uint8_t*)(bUFF))[13] = (X).s6_addr[13]; \
        ((uint8_t*)(bUFF))[14] = (X).s6_addr[14]; \
        ((uint8_t*)(bUFF))[15] = (X).s6_addr[15]; \
    } while(0)

#define BUFFER_TO_INT8(buf, x) (x = ((buf)[0]))

#define INT8_TO_BUFFER(x, buf) ((buf)[0] = (x))

/* Convert an integer on 16 bits to the given bUFFER */
#define INT16_TO_BUFFER(x, buf) \
do {                            \
    (buf)[0] = (x) >> 8;        \
    (buf)[1] = (x);             \
} while(0)

/* Convert an array of char containing vALUE to x */
#define BUFFER_TO_INT16(buf, x) \
do {                            \
    x = ((buf)[0] << 8)  |      \
        ((buf)[1]);             \
} while(0)

/* Convert an integer on 24 bits to the given bUFFER */
#define INT24_TO_BUFFER(x, buf) \
do {                            \
    (buf)[0] = (x) >> 16;       \
    (buf)[1] = (x) >> 8;        \
    (buf)[2] = (x);             \
} while(0)

/* Convert an array of char containing vALUE to x */
#define BUFFER_TO_INT24(buf, x) \
do {                            \
    x = ((buf)[0] << 16)  |     \
        ((buf)[1] << 8  )   |   \
        ((buf)[2]);             \
} while(0)


/* Convert an integer on 32 bits to the given bUFFER */
#define INT32_TO_BUFFER(x, buf) \
do {                            \
    (buf)[0] = (x) >> 24;       \
    (buf)[1] = (x) >> 16;       \
    (buf)[2] = (x) >> 8;        \
    (buf)[3] = (x);             \
} while(0)

/* Convert an array of char containing vALUE to x */
#define BUFFER_TO_INT32(buf, x) \
do {                            \
    x = ((buf)[0] << 24) |      \
        ((buf)[1] << 16) |      \
        ((buf)[2] << 8)  |      \
        ((buf)[3]);             \
} while(0)

/* Convert an integer on 32 bits to an octet string from aSN1c tool */
#define INT32_TO_OCTET_STRING(x, aSN)           \
do {                                            \
    (aSN)->buf = calloc(4, sizeof(uint8_t));    \
    INT32_TO_BUFFER(x, ((aSN)->buf));           \
    (aSN)->size = 4;                            \
} while(0)

#define INT32_TO_BIT_STRING(x, aSN) \
do {                                \
    INT32_TO_OCTET_STRING(x, aSN);  \
    (aSN)->bits_unused = 0;         \
} while(0)

#define INT16_TO_OCTET_STRING(x, aSN)           \
do {                                            \
    (aSN)->buf = calloc(2, sizeof(uint8_t));    \
    INT16_TO_BUFFER(x, ((aSN)->buf));           \
    (aSN)->size = 2;                            \
} while(0)

#define INT16_TO_BIT_STRING(x, aSN) \
do {                                \
    INT16_TO_OCTET_STRING(x, aSN);  \
    (aSN)->bits_unused = 0;         \
} while(0)


#define INT24_TO_OCTET_STRING(x, aSN)           \
do {                                            \
    (aSN)->buf = calloc(3, sizeof(uint8_t));    \
    INT24_TO_BUFFER(x, ((aSN)->buf));           \
    (aSN)->size = 3;                            \
} while(0)

#define INT24_TO_BIT_STRING(x, aSN) \
do {                                \
    INT24_TO_OCTET_STRING(x, aSN);  \
    (aSN)->bits_unused = 0;         \
} while(0)


#define INT8_TO_OCTET_STRING(x, aSN)            \
do {                                            \
    (aSN)->buf = calloc(1, sizeof(uint8_t));    \
    (aSN)->size = 1;                            \
    INT8_TO_BUFFER(x, (aSN)->buf);              \
} while(0)

#define MME_CODE_TO_OCTET_STRING INT8_TO_OCTET_STRING
#define M_TMSI_TO_OCTET_STRING   INT32_TO_OCTET_STRING
#define MME_GID_TO_OCTET_STRING  INT16_TO_OCTET_STRING

#define AMF_REGION_TO_BIT_STRING(x, aSN)      \
  do {                                        \
    INT8_TO_OCTET_STRING(x, aSN);             \
    (aSN)->bits_unused = 0;                   \
} while(0)

#define AMF_SETID_TO_BIT_STRING(x, aSN)       \
  do {                                        \
    INT16_TO_OCTET_STRING(x, aSN);            \
    (aSN)->bits_unused = 6;                   \
} while(0)

#define AMF_POINTER_TO_BIT_STRING(x, aSN)     \
  do {                                        \
    INT8_TO_OCTET_STRING(x, aSN);             \
    (aSN)->bits_unused = 2;                   \
} while(0)


#define ENCRALG_TO_BIT_STRING(encralg, bitstring)    \
    do {                        \
    (bitstring)->size=2;                \
    (bitstring)->bits_unused=0;            \
    (bitstring)->buf=calloc (2, sizeof (uint8_t));    \
    (bitstring)->buf[0] = (encralg) >> 8;         \
    (bitstring)->buf[1] = (encralg);        \
    }while(0)

#define INTPROTALG_TO_BIT_STRING(intprotalg, bitstring)    \
do {                                \
    (bitstring)->size=2;                    \
    (bitstring)->bits_unused=0;                \
    (bitstring)->buf=calloc (2, sizeof (uint8_t));        \
    (bitstring)->buf[0] = (intprotalg) >> 8;         \
    (bitstring)->buf[1] = (intprotalg);            \
}while(0)

#define KENB_STAR_TO_BIT_STRING(kenbstar, bitstring)    \
do {                            \
    (bitstring)->size=32;                \
    (bitstring)->bits_unused=0;            \
    (bitstring)->buf= calloc (32, sizeof (uint8_t));\
    memcpy((bitstring)->buf, kenbstar, 32*sizeof(uint8_t));            \
}while(0)

#define UEAGMAXBITRTD_TO_ASN_PRIMITIVES(uegmaxbitrtd, asnprimitives)        \
do {                                         \
    (asnprimitives)->size=5;                        \
    (asnprimitives)->buf=calloc (5, sizeof (uint8_t));            \
    (asnprimitives)->buf[0] = (uegmaxbitrtd) >> 32;                \
    (asnprimitives)->buf[1] = (uegmaxbitrtd) >> 24;                \
    (asnprimitives)->buf[2] = (uegmaxbitrtd) >> 16;                \
    (asnprimitives)->buf[3] = (uegmaxbitrtd) >> 8;                \
    (asnprimitives)->buf[4] = (uegmaxbitrtd);                \
 }while(0)

#define UEAGMAXBITRTU_TO_ASN_PRIMITIVES(uegmaxbitrtu, asnprimitives)        \
do {                                         \
    (asnprimitives)->size=5;                        \
    (asnprimitives)->buf=calloc (5, sizeof (uint8_t));            \
    (asnprimitives)->buf[0] = (uegmaxbitrtu) >> 32;                \
    (asnprimitives)->buf[1] = (uegmaxbitrtu) >> 24;                \
    (asnprimitives)->buf[2] = (uegmaxbitrtu) >> 16;                \
    (asnprimitives)->buf[3] = (uegmaxbitrtu) >> 8;                \
    (asnprimitives)->buf[4] = (uegmaxbitrtu);                \
 }while(0)

#define OCTET_STRING_TO_INT8(aSN, x)    \
do {                                    \
    DevCheck((aSN)->size == 1, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT8((aSN)->buf, x);    \
} while(0)

#define OCTET_STRING_TO_INT16(aSN, x)   \
do {                                    \
    DevCheck((aSN)->size == 2 || (aSN)->size == 3, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT16((aSN)->buf, x);    \
} while(0)

#define OCTET_STRING_TO_INT24(aSN, x)   \
do {                                    \
    DevCheck((aSN)->size == 2 || (aSN)->size == 3, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT24((aSN)->buf, x);    \
} while(0)

#define OCTET_STRING_TO_INT32(aSN, x)   \
do {                                    \
    DevCheck((aSN)->size == 4, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT32((aSN)->buf, x);    \
} while(0)

#define BIT_STRING_TO_INT32(aSN, x)     \
do {                                    \
    DevCheck((aSN)->bits_unused == 0, (aSN)->bits_unused, 0, 0);    \
    OCTET_STRING_TO_INT32(aSN, x);      \
} while(0)

#define BIT_STRING_TO_CELL_IDENTITY(aSN, vALUE)                     \
do {                                                                \
    DevCheck((aSN)->bits_unused == 4, (aSN)->bits_unused, 4, 0);    \
    vALUE = ((aSN)->buf[0] << 20) | ((aSN)->buf[1] << 12) |         \
        ((aSN)->buf[2] << 4) | (aSN)->buf[3];                       \
} while(0)

#define BIT_STRING_TO_NR_CELL_IDENTITY(aSN, vALUE)                     \
do {                                                                   \
    DevCheck((aSN)->bits_unused == 4, (aSN)->bits_unused, 4, 0);       \
    vALUE = ((aSN)->buf[0] << 28) | ((aSN)->buf[1] << 20) |            \
        ((aSN)->buf[2] << 12) | ((aSN)->buf[3]<<4) | ((aSN)->buf[4]>>4);  \
} while(0)

#define MCC_HUNDREDS(vALUE) \
    ((vALUE) / 100)
/* When MNC is only composed of 2 digits, set the hundreds unit to 0xf */
#define MNC_HUNDREDS(vALUE, mNCdIGITlENGTH) \
    ( mNCdIGITlENGTH == 2 ? 15 : (vALUE) / 100)
#define MCC_MNC_DECIMAL(vALUE) \
    (((vALUE) / 10) % 10)
#define MCC_MNC_DIGIT(vALUE) \
    ((vALUE) % 10)

#define MCC_TO_BUFFER(mCC, bUFFER)      \
do {                                    \
    DevAssert(bUFFER != NULL);          \
    (bUFFER)[0] = MCC_HUNDREDS(mCC);    \
    (bUFFER)[1] = MCC_MNC_DECIMAL(mCC); \
    (bUFFER)[2] = MCC_MNC_DIGIT(mCC);   \
} while(0)

#define MCC_MNC_TO_PLMNID(mCC, mNC, mNCdIGITlENGTH, oCTETsTRING)               \
do {                                                                           \
    (oCTETsTRING)->buf = calloc(3, sizeof(uint8_t));                           \
    (oCTETsTRING)->buf[0] = (MCC_MNC_DECIMAL(mCC) << 4) | MCC_HUNDREDS(mCC);   \
    (oCTETsTRING)->buf[1] = (MNC_HUNDREDS(mNC,mNCdIGITlENGTH) << 4) | MCC_MNC_DIGIT(mCC);     \
    (oCTETsTRING)->buf[2] = (MCC_MNC_DIGIT(mNC) << 4) | MCC_MNC_DECIMAL(mNC);  \
    (oCTETsTRING)->size = 3;                                                   \
} while(0)

#define PLMNID_TO_MCC_MNC(oCTETsTRING, mCC, mNC, mNCdIGITlENGTH)                  \
do {                                                                              \
    mCC = ((oCTETsTRING)->buf[0] & 0x0F) * 100 +                                  \
          ((oCTETsTRING)->buf[0] >> 4 & 0x0F) * 10 +                              \
          ((oCTETsTRING)->buf[1] & 0x0F);                                         \
    mNCdIGITlENGTH = ((oCTETsTRING)->buf[1] >> 4 & 0x0F) == 0xF ? 2 : 3;          \
    mNC = (mNCdIGITlENGTH == 2 ? 0 : ((oCTETsTRING)->buf[1] >> 4 & 0x0F) * 100) + \
          ((oCTETsTRING)->buf[2] & 0x0F) * 10 +                                   \
          ((oCTETsTRING)->buf[2] >> 4 & 0x0F);                                    \
} while (0)

#define MCC_MNC_TO_TBCD(mCC, mNC, mNCdIGITlENGTH, tBCDsTRING)        \
do {                                                                 \
    char _buf[3];                                                    \
     DevAssert((mNCdIGITlENGTH == 3) || (mNCdIGITlENGTH == 2));      \
    _buf[0] = (MCC_MNC_DECIMAL(mCC) << 4) | MCC_HUNDREDS(mCC);       \
    _buf[1] = (MNC_HUNDREDS(mNC,mNCdIGITlENGTH) << 4) | MCC_MNC_DIGIT(mCC);\
    _buf[2] = (MCC_MNC_DIGIT(mNC) << 4) | MCC_MNC_DECIMAL(mNC);      \
    OCTET_STRING_fromBuf(tBCDsTRING, _buf, 3);                       \
} while(0)

#define TBCD_TO_MCC_MNC(tBCDsTRING, mCC, mNC, mNCdIGITlENGTH)    \
do {                                                             \
    int mNC_hundred;                                             \
    DevAssert((tBCDsTRING)->size == 3);                          \
    mNC_hundred = (((tBCDsTRING)->buf[1] & 0xf0) >> 4);          \
    if (mNC_hundred == 0xf) {                                    \
        mNC_hundred = 0;                                         \
        mNCdIGITlENGTH = 2;                                      \
    } else {                                                     \
            mNCdIGITlENGTH = 3;                                  \
    }                                                            \
    mCC = (((((tBCDsTRING)->buf[0]) & 0xf0) >> 4) * 10) +        \
        ((((tBCDsTRING)->buf[0]) & 0x0f) * 100) +                \
        (((tBCDsTRING)->buf[1]) & 0x0f);                         \
    mNC = (mNC_hundred * 100) +                                  \
        ((((tBCDsTRING)->buf[2]) & 0xf0) >> 4) +                 \
        ((((tBCDsTRING)->buf[2]) & 0x0f) * 10);                  \
} while(0)

#define TBCD_TO_PLMN_T(tBCDsTRING, pLMN)                            \
do {                                                                \
    DevAssert((tBCDsTRING)->size == 3);                             \
    (pLMN)->MCCdigit2 = (((tBCDsTRING)->buf[0] & 0xf0) >> 4);       \
    (pLMN)->MCCdigit3 = ((tBCDsTRING)->buf[0] & 0x0f);              \
    (pLMN)->MCCdigit1 = (tBCDsTRING)->buf[1] & 0x0f;                \
    (pLMN)->MNCdigit3 = (((tBCDsTRING)->buf[1] & 0xf0) >> 4) == 0xF \
    ? 0 : (((tBCDsTRING)->buf[1] & 0xf0) >> 4);       \
    (pLMN)->MNCdigit2 = (((tBCDsTRING)->buf[2] & 0xf0) >> 4);       \
    (pLMN)->MNCdigit1 = ((tBCDsTRING)->buf[2] & 0x0f);              \
} while(0)

#define PLMN_T_TO_TBCD(pLMN, tBCDsTRING, mNClENGTH)                 \
do {                                                                \
    tBCDsTRING[0] = (pLMN.MCCdigit2 << 4) | pLMN.MCCdigit1;         \
    /* ambiguous (think about len 2) */                             \
    if (mNClENGTH == 2) {                                      \
        tBCDsTRING[1] = (0x0F << 4) | pLMN.MCCdigit3;               \
        tBCDsTRING[2] = (pLMN.MNCdigit2 << 4) | pLMN.MNCdigit1;     \
    } else {                                                        \
        tBCDsTRING[1] = (pLMN.MNCdigit3 << 4) | pLMN.MCCdigit3;     \
        tBCDsTRING[2] = (pLMN.MNCdigit2 << 4) | pLMN.MNCdigit1;     \
    }                                                               \
} while(0)

#define PLMN_T_TO_MCC_MNC(pLMN, mCC, mNC, mNCdIGITlENGTH)               \
do {                                                                    \
    mCC = pLMN.MCCdigit3 * 100 + pLMN.MCCdigit2 * 10 + pLMN.MCCdigit1;  \
    mNCdIGITlENGTH = (pLMN.MNCdigit3 == 0xF ? 2 : 3);                   \
    mNC = (mNCdIGITlENGTH == 2 ? 0 : pLMN.MNCdigit3 * 100)              \
          + pLMN.MNCdigit2 * 10 + pLMN.MNCdigit1;                       \
} while(0)


/* TS 38.473 v15.2.1 section 9.3.1.32:
 * C RNTI is BIT_STRING(16)
 */
#define C_RNTI_TO_BIT_STRING(mACRO, bITsTRING)          \
do {                                                    \
    (bITsTRING)->buf = calloc(2, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = (mACRO) >> 8;                 \
    (bITsTRING)->buf[1] = ((mACRO) & 0x0ff);            \
    (bITsTRING)->size = 2;                              \
    (bITsTRING)->bits_unused = 0;                       \
} while(0)


/* TS 38.473 v15.2.1 section 9.3.2.3:
 * TRANSPORT LAYER ADDRESS for IPv4 is 32bit (TS 38.414)
 */
#define TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(mACRO, bITsTRING)    \
do {                                                    \
    (bITsTRING)->buf = calloc(4, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = (mACRO) >> 24 & 0xFF;         \
    (bITsTRING)->buf[1] = (mACRO) >> 16 & 0xFF;         \
    (bITsTRING)->buf[2] = (mACRO) >> 8 & 0xFF;          \
    (bITsTRING)->buf[3] = (mACRO) &  0xFF;              \
    (bITsTRING)->size = 4;                              \
    (bITsTRING)->bits_unused = 0;                       \
} while(0)

#define BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(bITsTRING, mACRO)    \
do {                                                                    \
    DevCheck((bITsTRING)->size == 4, (bITsTRING)->size, 4, 0);          \
    DevCheck((bITsTRING)->bits_unused == 0, (bITsTRING)->bits_unused, 0, 0); \
    mACRO = ((bITsTRING)->buf[0] << 24) +                               \
            ((bITsTRING)->buf[1] << 16) +                               \
            ((bITsTRING)->buf[2] << 8) +                                \
            ((bITsTRING)->buf[3]);                                      \
} while (0)


/* TS 38.473 v15.1.1 section 9.3.1.12:
 * NR CELL ID
 */
#define NR_CELL_ID_TO_BIT_STRING(mACRO, bITsTRING)      \
do {                                                    \
    (bITsTRING)->buf = calloc(5, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 28) & 0xff;       \
    (bITsTRING)->buf[1] = ((mACRO) >> 20) & 0xff;       \
    (bITsTRING)->buf[2] = ((mACRO) >> 12) & 0xff;       \
    (bITsTRING)->buf[3] = ((mACRO) >> 4)  & 0xff;       \
    (bITsTRING)->buf[4] = ((mACRO) & 0x0f) << 4;        \
    (bITsTRING)->size = 5;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)

/*
#define INT16_TO_3_BYTE_BUFFER(x, buf) \
do {                            \
	(buf)[0] = 0x00; \
    (buf)[1] = (x) >> 8;        \
    (buf)[2] = (x);             \
} while(0)
*/

#define NR_FIVEGS_TAC_ID_TO_BIT_STRING(x, aSN)      \
do {                                                    \
    (aSN)->buf = calloc(3, sizeof(uint8_t));    \
    (aSN)->size = 3;              \
    (aSN)->buf[0] = 0x00;		  \
    (aSN)->buf[1] = (x) >> 8;        \
    (aSN)->buf[2] = (x);             \
} while(0)



/* TS 38.473 v15.2.1 section 9.3.1.55:
 * MaskedIMEISV is BIT_STRING(64)
 */
#define MaskedIMEISV_TO_BIT_STRING(mACRO, bITsTRING)    \
do {                                                    \
    (bITsTRING)->buf = calloc(8, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = (mACRO) >> 56 & 0xFF;         \
    (bITsTRING)->buf[1] = (mACRO) >> 48 & 0xFF;         \
    (bITsTRING)->buf[2] = (mACRO) >> 40 & 0xFF;         \
    (bITsTRING)->buf[3] = (mACRO) >> 32 & 0xFF;         \
    (bITsTRING)->buf[4] = (mACRO) >> 24 & 0xFF;         \
    (bITsTRING)->buf[5] = (mACRO) >> 16 & 0xFF;         \
    (bITsTRING)->buf[6] = (mACRO) >> 8 & 0xFF;          \
    (bITsTRING)->buf[7] = (mACRO) >> 4 & 0xFF;          \
    (bITsTRING)->size = 8;                              \
    (bITsTRING)->bits_unused = 0;                       \
} while(0)

#define BIT_STRING_TO_MaskedIMEISV(bITsTRING, mACRO)    \
do {                                                                    \
    DevCheck((bITsTRING)->size == 8, (bITsTRING)->size, 8, 0);          \
    DevCheck((bITsTRING)->bits_unused == 0, (bITsTRING)->bits_unused, 0, 0); \
    mACRO = ((bITsTRING)->buf[0] << 56) +                               \
            ((bITsTRING)->buf[1] << 48) +                               \
            ((bITsTRING)->buf[2] << 40) +                               \
            ((bITsTRING)->buf[3] << 32) +                               \
            ((bITsTRING)->buf[4] << 24) +                               \
            ((bITsTRING)->buf[5] << 16) +                               \
            ((bITsTRING)->buf[6] << 8) +                                \
            ((bITsTRING)->buf[7]);                                      \
} while (0)

/* TS 36.413 v10.9.0 section 9.2.1.37:
 * Macro eNB ID:
 * Equal to the 20 leftmost bits of the Cell
 * Identity IE contained in the E-UTRAN CGI
 * IE (see subclause 9.2.1.38) of each cell
 * served by the eNB.
 */
#define MACRO_ENB_ID_TO_BIT_STRING(mACRO, bITsTRING)    \
do {                                                    \
    (bITsTRING)->buf = calloc(3, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 12);              \
    (bITsTRING)->buf[1] = (mACRO) >> 4;                 \
    (bITsTRING)->buf[2] = ((mACRO) & 0x0f) << 4;        \
    (bITsTRING)->size = 3;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)


#define MACRO_GNB_ID_TO_BIT_STRING(mACRO, bITsTRING)    \
do {                                                    \
    (bITsTRING)->buf = calloc(4, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 20);              \
    (bITsTRING)->buf[1] = (mACRO) >> 12;                \
    (bITsTRING)->buf[2] = (mACRO) >> 4;                 \
    (bITsTRING)->buf[3] = ((mACRO) & 0x0f) << 4;        \
    (bITsTRING)->size = 4;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)

/* TS 36.413 v10.9.0 section 9.2.1.38:
 * E-UTRAN CGI/Cell Identity
 * The leftmost bits of the Cell
 * Identity correspond to the eNB
 * ID (defined in subclause 9.2.1.37).
 */
#define MACRO_ENB_ID_TO_CELL_IDENTITY(mACRO, cELL_iD, bITsTRING) \
do {                                                    \
    (bITsTRING)->buf = calloc(4, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 12);              \
    (bITsTRING)->buf[1] = (mACRO) >> 4;                 \
    (bITsTRING)->buf[2] = (((mACRO) & 0x0f) << 4) | ((cELL_iD) >> 4);        \
    (bITsTRING)->buf[3] = ((cELL_iD) & 0x0f) << 4;        \
    (bITsTRING)->size = 4;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)

#define MACRO_GNB_ID_TO_CELL_IDENTITY(mACRO, cELL_iD, bITsTRING) \
do {                                                    \
    (bITsTRING)->buf = calloc(5, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 20);              \
    (bITsTRING)->buf[1] = (mACRO) >> 12;                 \
    (bITsTRING)->buf[2] = (mACRO) >> 4;        \
    (bITsTRING)->buf[3] = (((mACRO) & 0x0f) << 4) | ((cELL_iD) >> 4);        \
    (bITsTRING)->buf[4] = ((cELL_iD) & 0x0f) << 4;        \
    (bITsTRING)->size = 5;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)


/* Used to format an uint32_t containing an ipv4 address */
#define IPV4_ADDR    "%u.%u.%u.%u"
#define IPV4_ADDR_FORMAT(aDDRESS)               \
    (uint8_t)((aDDRESS)  & 0x000000ff),         \
    (uint8_t)(((aDDRESS) & 0x0000ff00) >> 8 ),  \
    (uint8_t)(((aDDRESS) & 0x00ff0000) >> 16),  \
    (uint8_t)(((aDDRESS) & 0xff000000) >> 24)

#define IPV4_ADDR_DISPLAY_8(aDDRESS)            \
    (aDDRESS)[0], (aDDRESS)[1], (aDDRESS)[2], (aDDRESS)[3]

#define TAC_TO_ASN1 INT16_TO_OCTET_STRING
#define GTP_TEID_TO_ASN1 INT32_TO_OCTET_STRING
#define OCTET_STRING_TO_TAC OCTET_STRING_TO_INT16

void hexa_to_ascii(uint8_t *from, char *to, size_t length);

int ascii_to_hex(uint8_t *dst, const char *h);

#endif /* CONVERSIONS_H_ */
