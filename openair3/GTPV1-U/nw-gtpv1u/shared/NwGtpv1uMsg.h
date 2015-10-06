/*----------------------------------------------------------------------------*
 *                                                                            *
 *                             n w - g t p v 2 u                              *
 *    G P R S   T u n n e l i n g    P r o t o c o l   v 2 u    S t a c k     *
 *                                                                            *
 *                                                                            *
 * Copyright (c) 2010-2011 Amit Chawre                                        *
 * All rights reserved.                                                       *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions         *
 * are met:                                                                   *
 *                                                                            *
 * 1. Redistributions of source code must retain the above copyright          *
 *    notice, this list of conditions and the following disclaimer.           *
 * 2. Redistributions in binary form must reproduce the above copyright       *
 *    notice, this list of conditions and the following disclaimer in the     *
 *    documentation and/or other materials provided with the distribution.    *
 * 3. The name of the author may not be used to endorse or promote products   *
 *    derived from this software without specific prior written permission.   *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR       *
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES  *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.    *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,           *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT   *
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY      *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT        *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF   *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          *
 *----------------------------------------------------------------------------*/

#ifndef __NW_GTPV1U_MSG_H__
#define __NW_GTPV1U_MSG_H__

#include "NwTypes.h"
#include "NwGtpv1u.h"

/**
 * @file NwGtpv1uMsg.h
 * @brief This file defines APIs for to build new outgoing gtpv1u messages and to parse incoming messages.
*/

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*
 *   G T P V 2 C     I E    D A T A - T Y P E      D E F I N I T I O N S    *
 *--------------------------------------------------------------------------*/

#pragma pack(1)

typedef struct NwGtpv1uIeTv1 {
  uint8_t  t;
  uint8_t  v;
} NwGtpv1uIeTv1T;

typedef struct NwGtpv1uIeTv2 {
  uint8_t  t;
  uint8_t  v;
} NwGtpv1uIeTv2T;

typedef struct NwGtpv1uIeTv4 {
  uint8_t  t;
  uint32_t  v;
} NwGtpv1uIeTv4T;

typedef struct NwGtpv1uIeTlv {
  uint8_t  t;
  uint16_t l;
} NwGtpv1uIeTlvT;

#pragma pack()


/**
 * Allocate a GPDU gtpv1u message.
 *
 * @param[in] hGtpuStackHandle : gtpv1u stack handle.
 * @param[in] teidPresent : TEID is present flag.
 * @param[in] teid : TEID for this message.
 * @param[in] seqNum : Sequence number for this message.
 * @param[out] phMsg : Pointer to message handle.
 */

NwGtpv1uRcT
nwGtpv1uMsgNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                NW_IN uint8_t     seqNumFlag,
                NW_IN uint8_t     npduNumFlag,
                NW_IN uint8_t     extHdrFlag,
                NW_IN uint8_t     msgType,
                NW_IN uint8_t     teid,
                NW_IN uint16_t    seqNum,
                NW_IN uint8_t     npduNum,
                NW_IN uint8_t     nextExtHeader,
                NW_OUT NwGtpv1uMsgHandleT *phMsg);


/**
 * Allocate a gtpv1u message.
 *
 * @param[in] hGtpuStackHandle : gtpv1u stack handle.
 * @param[in] teid : TEID for this message.
 * @param[in] seqNumFlag : Sequence number flag for this message.
 * @param[in] seqNum : Sequence number for this message.
 * @param[in] pTpdu: T-PDU for this message.
 * @param[in] tpduLength: T-PDU length for this message.
 * @param[out] phMsg : Pointer to message handle.
 */

NwGtpv1uRcT
nwGtpv1uGpduMsgNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                    NW_IN uint32_t    teid,
                    NW_IN uint8_t     seqNumFlag,
                    NW_IN uint16_t    seqNum,
                    NW_IN uint8_t*    tpdu,
                    NW_IN uint16_t    tpduLength,
                    NW_IN uint32_t    tpduOffset,
                    NW_OUT NwGtpv1uMsgHandleT *phMsg);

/**
 * Allocate a gtpv1u message from another gtpv1u message.
 *
 * @param[in] hGtpuStackHandle : gtpv1u stack handle.
 * @param[out] hMsg : Handle to input message.
 * @param[out] phMsg : Pointer to message handle.
 */

NwGtpv1uRcT
nwGtpv1uMsgFromMsgNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                       NW_IN NwGtpv1uMsgHandleT hMsg,
                       NW_OUT NwGtpv1uMsgHandleT *phMsg);
/**
 * Allocate a gtpv1u message from data buffer.
 *
 * @param[in] hGtpuStackHandle : gtpv1u stack handle.
 * @param[in] pBuf: Buffer to be copied in this message.
 * @param[in] bufLen: Buffer length to be copied in this message.
 * @param[out] phMsg : Pointer to message handle.
 */

NwGtpv1uRcT
nwGtpv1uMsgFromBufferNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                          NW_IN uint8_t* pBuf,
                          NW_IN uint32_t bufLen,
                          NW_OUT NwGtpv1uMsgHandleT *phMsg);

/**
 * Free a gtpv1u message.
 *
 * @param[in] hGtpuStackHandle : gtpv1u stack handle.
 * @param[in] hMsg : Message handle.
 */

NwGtpv1uRcT
nwGtpv1uMsgDelete( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                   NW_IN NwGtpv1uMsgHandleT hMsg);

/**
 * Set TEID for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 * @param[in] teid: TEID value.
 */

NwGtpv1uRcT
nwGtpv1uMsgSetTeid(NW_IN NwGtpv1uMsgHandleT hMsg, uint32_t teid);

/**
 * Set TEID present flag for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 * @param[in] teidPesent: Flag boolean value.
 */

NwGtpv1uRcT
nwGtpv1uMsgSetTeidPresent(NW_IN NwGtpv1uMsgHandleT hMsg, NwBoolT teidPresent);

/**
 * Set sequence for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 * @param[in] seqNum: Flag boolean value.
 */

NwGtpv1uRcT
nwGtpv1uMsgSetSeqNumber(NW_IN NwGtpv1uMsgHandleT hMsg, uint32_t seqNum);

/**
 * Get TEID present for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

uint32_t
nwGtpv1uMsgGetTeid(NW_IN NwGtpv1uMsgHandleT hMsg);

/**
 * Get TEID present for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

NwBoolT
nwGtpv1uMsgGetTeidPresent(NW_IN NwGtpv1uMsgHandleT hMsg);

/**
 * Get sequence number for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

uint32_t
nwGtpv1uMsgGetSeqNumber(NW_IN NwGtpv1uMsgHandleT hMsg);

/**
 * Get tpdu for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 * @param[inout] pTpduBuf : Buffer to copy the T-PDU.
 * @param[out] hMsg : T-PDU length.
 */

uint32_t
nwGtpv1uMsgGetTpdu(NW_IN NwGtpv1uMsgHandleT hMsg, uint8_t* pTpduBuf, uint32_t* pTpduLength);

uint8_t*
nwGtpv1uMsgGetTpduHandle(NW_IN NwGtpv1uMsgHandleT hMsg);

uint32_t
nwGtpv1uMsgGetTpduLength(NW_IN NwGtpv1uMsgHandleT hMsg);

/**
 * Add a gtpv1u information element of length 1 to gtpv1u mesasge.
 *
 * @param[in] hMsg : Handle to gtpv1u message.
 * @param[in] type : IE type.
 * @param[in] value : IE value.
 */

NwGtpv1uRcT
nwGtpv1uMsgAddIeTV1(NW_IN NwGtpv1uMsgHandleT hMsg,
                    NW_IN uint8_t       type,
                    NW_IN uint8_t       value);


/**
 * Add a gtpv1u information element of length 2 to gtpv1u mesasge.
 *
 * @param[in] hMsg : Handle to gtpv1u message.
 * @param[in] type : IE type.
 * @param[in] value : IE value.
 */

NwGtpv1uRcT
nwGtpv1uMsgAddIeTV2(NW_IN NwGtpv1uMsgHandleT hMsg,
                    NW_IN uint8_t       type,
                    NW_IN uint16_t      length,
                    NW_IN uint16_t      value);


/**
 * Add a gtpv1u information element of length 4 to gtpv1u mesasge.
 *
 * @param[in] hMsg : Handle to gtpv1u message.
 * @param[in] type : IE type.
 * @param[in] value : IE value.
 */

NwGtpv1uRcT
nwGtpv1uMsgAddIeTV4(NW_IN NwGtpv1uMsgHandleT hMsg,
                    NW_IN uint8_t       type,
                    NW_IN uint16_t      length,
                    NW_IN uint32_t      value);


/**
 * Add a gtpv1u information element of variable length to gtpv1u mesasge.
 *
 * @param[in] hMsg : Handle to gtpv1u message.
 * @param[in] type : IE type.
 * @param[in] length : IE length.
 * @param[in] value : IE value.
 */

NwGtpv1uRcT
nwGtpv1uMsgAddIe(NW_IN NwGtpv1uMsgHandleT hMsg,
                 NW_IN uint8_t       type,
                 NW_IN uint16_t      length,
                 NW_IN uint8_t*      pVal);

/**
 * Dump the contents of gtpv1u mesasge.
 *
 * @param[in] hMsg : Handle to gtpv1u message.
 * @param[in] fp: Pointer to output file.
 */

NwGtpv1uRcT
nwGtpv1uMsgHexDump(NwGtpv1uMsgHandleT hMsg, FILE* fp);

uint32_t
nwGtpv1uMsgGetMsgType(NW_IN NwGtpv1uMsgHandleT hMsg);

#ifdef __cplusplus
}
#endif

#endif /* __NW_TYPES_H__ */


/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

