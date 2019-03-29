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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "NwTypes.h"
#include "NwLog.h"
#include "NwUtils.h"
#include "NwGtpv1uLog.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uPrivate.h"
#include "NwGtpv1uMsg.h"
#include "assertions.h"

#include "gtpv1u.h"
#if defined(ENB_MODE)
#include "common/utils/LOG/log.h"
#endif

#define NW_GTPV1U_EPC_SPECIFIC_HEADER_SIZE                             (12)   /**< Size of GTPv1u EPC specific header */
#define NW_GTPV1U_EPC_MIN_HEADER_SIZE                                  (8)

#ifdef __cplusplus
extern "C" {
#endif

static NwGtpv1uMsgT *gpGtpv1uMsgPool = NULL;

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
                NW_OUT NwGtpv1uMsgHandleT *phMsg)
{
  NwGtpv1uStackT *pStack = (NwGtpv1uStackT *) hGtpuStackHandle;
  NwGtpv1uMsgT *pMsg;

  if(gpGtpv1uMsgPool) {
    pMsg = gpGtpv1uMsgPool;
    gpGtpv1uMsgPool = gpGtpv1uMsgPool->next;
  } else {
    NW_GTPV1U_MALLOC(pStack, sizeof(NwGtpv1uMsgT), pMsg, NwGtpv1uMsgT *);
  }


  if(pMsg) {
    pMsg->version       = NW_GTPU_VERSION;
    pMsg->protocolType  = NW_GTP_PROTOCOL_TYPE_GTP;
    pMsg->seqNumFlag    = seqNumFlag;
    pMsg->npduNumFlag   = npduNumFlag;
    pMsg->extHdrFlag    = extHdrFlag;
    pMsg->msgType       = msgType;

    if(seqNumFlag) {
      pMsg->seqNum        = seqNum;
    }

    if(npduNumFlag) {
      pMsg->npduNum       = npduNum;
    }

    if(extHdrFlag) {
      pMsg->nextExtHdrType = nextExtHeader;
    }

    pMsg->msgLen        = ((pMsg->seqNumFlag || pMsg->npduNumFlag
                            || pMsg->extHdrFlag) ?
                           4 : 0);
    //NW_GTPV1U_EPC_SPECIFIC_HEADER_SIZE : (NW_GTPV1U_EPC_SPECIFIC_HEADER_SIZE - 4));

    pMsg->msgBuf       = NULL;
    pMsg->msgBufLen    = 0;
    pMsg->msgBufOffset = 0;
    *phMsg = (NwGtpv1uMsgHandleT) pMsg;
    return NW_GTPV1U_OK;
  }

  return NW_GTPV1U_FAILURE;
}

NwGtpv1uRcT
nwGtpv1uGpduMsgNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                    NW_IN uint32_t    teid,
                    NW_IN uint8_t     seqNumFlag,
                    NW_IN uint16_t    seqNum,
                    NW_IN uint8_t    *tpdu,
                    NW_IN uint16_t    tpduLength,
                    NW_IN uint32_t    tpduOffset,
                    NW_OUT NwGtpv1uMsgHandleT *phMsg)
{
  NwGtpv1uStackT *pStack = (NwGtpv1uStackT *) hGtpuStackHandle;
  NwGtpv1uMsgT   *pMsg;
  //uint32_t          header_len  = 0;
  uint32_t          msgExtraLen = 0;

  if(gpGtpv1uMsgPool) {
    pMsg = gpGtpv1uMsgPool;
    gpGtpv1uMsgPool = gpGtpv1uMsgPool->next;
  } else {
    NW_GTPV1U_MALLOC(pStack, sizeof(NwGtpv1uMsgT), pMsg, NwGtpv1uMsgT *);
  }

  if(pMsg) {
    msgExtraLen        = (((seqNumFlag) || (NW_FALSE) || (NW_FALSE) ) ?
                          (NW_GTPV1U_EPC_SPECIFIC_HEADER_SIZE - NW_GTPV1U_EPC_MIN_HEADER_SIZE)  : 0);
    AssertFatal((msgExtraLen + NW_GTPV1U_EPC_MIN_HEADER_SIZE) <= tpduOffset,
                "Mismatch GTPU len, msgExtraLen %u tpduOffset %u",
                msgExtraLen,
                (uint32_t) tpduOffset);
    pMsg->msgBuf       = tpdu;
    pMsg->msgBufLen    = tpduLength + msgExtraLen + NW_GTPV1U_EPC_MIN_HEADER_SIZE;
    pMsg->msgBufOffset = tpduOffset - (msgExtraLen + NW_GTPV1U_EPC_MIN_HEADER_SIZE);
#if defined(LOG_GTPU) && LOG_GTPU > 0
    GTPU_DEBUG("MSG length %u, offset %u -> length %u offset %u\n",
               tpduLength, tpduOffset,
               pMsg->msgBufLen, pMsg->msgBufOffset);
#endif

    // Version field: This field is used to determine the version of the GTP-U protocol.
    // The version number shall be set to '1'.
    pMsg->version       = NW_GTPU_VERSION;
    // Protocol Type (PT): This bit is used as a protocol discriminator between GTP (when PT is '1')
    // and GTP' (when PT is '0'). GTP is described in this document and the GTP' protocol in 3GPP TS 32.295 [8].
    // Note that the interpretation of the header fields may be different in GTP' than in GTP.
    pMsg->protocolType  = NW_GTP_PROTOCOL_TYPE_GTP;
    // Extension Header flag (E): This flag indicates the presence of a meaningful value of the Next Extension
    // Header field. When it is set to '0', the Next Extension Header field either is not present or, if present,
    // shall not be interpreted.
    pMsg->extHdrFlag    = NW_FALSE;
    // Sequence number flag (S): This flag indicates the presence of a meaningful value of the Sequence Number field.
    // When it is set to '0', the Sequence Number field either is not present or, if present, shall not be interpreted.
    // When it is set to '1', the Sequence Number field is present, and shall be interpreted, as described below in
    // this section.
    // For the Echo Request, Echo Response, Error Indication and Supported Extension Headers Notification
    // messages, the S flag shall be set to '1'. Since the use of Sequence Numbers is optional for G-PDUs, the PGW,
    // SGW, ePDG and eNodeB should set the flag to '0'. However, when a G-PDU (T-PDU+header) is being relayed
    // by the Indirect Data Forwarding for Inter RAT HO procedure, then if the received G-PDU has the S flag set to
    // '1', then the relaying entity shall set S flag to '1' and forward the G-PDU (T-PDU+header). In an End marker
    // message the S flag shall be set to '0'.
    pMsg->seqNumFlag    = (seqNumFlag? NW_TRUE : NW_FALSE);
    pMsg->seqNum        = seqNum;
    // N-PDU Number flag (PN): This flag indicates the presence of a meaningful value of the N-PDU Number field.
    // When it is set to '0', the N-PDU Number field either is not present, or, if present, shall not be interpreted.
    // When it is set to '1', the N-PDU Number field is present, and shall be interpreted.
    pMsg->npduNumFlag   = NW_FALSE;
    pMsg->npduNum       = 0x00;
    // Message Type: This field indicates the type of GTP-U message.
    pMsg->msgType       = NW_GTP_GPDU;
    // Length: This field indicates the length in octets of the payload, i.e. the rest of the packet following the
    // mandatory part of the GTP header (that is the first 8 octets).
    // The Sequence Number, the N-PDU Number or any Extension headers shall be considered to be part of the payload,
    // i.e. included in the length count.
    //header_len          = ((pMsg->seqNumFlag || pMsg->npduNumFlag || pMsg->extHdrFlag ) ?
    //                        NW_GTPV1U_EPC_SPECIFIC_HEADER_SIZE : NW_GTPV1U_EPC_MIN_HEADER_SIZE);
    pMsg->msgLen        = msgExtraLen + tpduLength;

    // Tunnel Endpoint Identifier (TEID): This field unambiguously identifies a tunnel endpoint in the receiving
    // GTP-U protocol entity. The receiving end side of a GTP tunnel locally assigns the TEID value the transmitting
    // side has to use. The TEID shall be used by the receiving entity to find the PDP context, except for the
    // following cases:
    // - The Echo Request/Response and Supported Extension Headers notification messages, where the Tunnel
    //      Endpoint Identifier shall be set to all zeroes.
    // - The Error Indication message where the Tunnel Endpoint Identifier shall be set to all zeros.
    pMsg->teid          = teid;
    pMsg->nextExtHdrType= 0x00;

    //memcpy(pMsg->msgBuf + header_len, tpdu, tpduLength);
    *phMsg = (NwGtpv1uMsgHandleT) pMsg;
    return NW_GTPV1U_OK;
  }

  return NW_GTPV1U_FAILURE;
}

NwGtpv1uRcT
nwGtpv1uMsgFromMsgNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                       NW_IN NwGtpv1uMsgHandleT hMsg,
                       NW_OUT NwGtpv1uMsgHandleT *phMsg)
{
  NwGtpv1uStackT *pStack = (NwGtpv1uStackT *) hGtpuStackHandle;
  NwGtpv1uMsgT *pMsg;

  if(gpGtpv1uMsgPool) {
    pMsg = gpGtpv1uMsgPool;
    gpGtpv1uMsgPool = gpGtpv1uMsgPool->next;
  } else {
    NW_GTPV1U_MALLOC(pStack, sizeof(NwGtpv1uMsgT), pMsg, NwGtpv1uMsgT *);
  }


  if(pMsg) {
    memcpy(pMsg, (NwGtpv1uMsgT *)hMsg, sizeof(NwGtpv1uMsgT));
    *phMsg = (NwGtpv1uMsgHandleT) pMsg;
    return NW_GTPV1U_OK;
  }

  return NW_GTPV1U_FAILURE;
}

NwGtpv1uRcT
nwGtpv1uMsgFromBufferNew( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                          NW_IN uint8_t *pBuf,
                          NW_IN uint32_t bufLen,
                          NW_OUT NwGtpv1uMsgHandleT *phMsg)
{
  NwGtpv1uStackT *pStack = (NwGtpv1uStackT *) hGtpuStackHandle;
  NwGtpv1uMsgT *pMsg;

  if(gpGtpv1uMsgPool) {
    pMsg = gpGtpv1uMsgPool;
    gpGtpv1uMsgPool = gpGtpv1uMsgPool->next;
  } else {
    NW_GTPV1U_MALLOC(pStack, sizeof(NwGtpv1uMsgT), pMsg, NwGtpv1uMsgT *);
  }


  if(pMsg) {
    pMsg->msgBuf    =  pBuf;

    pMsg->version       = ((*pBuf) & 0xE0) >> 5;
    pMsg->protocolType  = ((*pBuf) & 0x10) >> 4;
    pMsg->extHdrFlag    = ((*pBuf) & 0x04) >> 2;
    pMsg->seqNumFlag    = ((*pBuf) & 0x02) >> 1;
    pMsg->npduNumFlag   = ((*pBuf) & 0x01);
    pBuf++;

    pMsg->msgType       = *(pBuf);
    pBuf++;

    pBuf += 2;

    pMsg->teid          = ntohl(*((uint32_t *)pBuf));
    pBuf += 4;

    if(pMsg->extHdrFlag || pMsg->seqNumFlag || pMsg->npduNumFlag) {
      pMsg->seqNum              = ntohs(*(((uint16_t *)pBuf)));
      pBuf += 2;
      pMsg->npduNum             = *(pBuf++);
      pMsg->nextExtHdrType      = *(pBuf++);
    }

    pMsg->msgBufOffset = (uint32_t)(pBuf - pMsg->msgBuf);
    pMsg->msgBufLen = bufLen - pMsg->msgBufOffset;
    pMsg->msgLen    = bufLen;
#if defined(LOG_GTPU) && LOG_GTPU > 0
    GTPU_DEBUG("nwGtpv1uMsgFromBufferNew() msgLen %u msgBufLen %u msgBufOffset %u ",
               pMsg->msgLen, pMsg->msgBufLen, pMsg->msgBufOffset);
#endif
    *phMsg = (NwGtpv1uMsgHandleT) pMsg;
    return NW_GTPV1U_OK;
  }

  return NW_GTPV1U_FAILURE;
}

NwGtpv1uRcT
nwGtpv1uMsgDelete( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                   NW_IN NwGtpv1uMsgHandleT hMsg)
{
  ((NwGtpv1uMsgT *)hMsg)->next = gpGtpv1uMsgPool;
  gpGtpv1uMsgPool = (NwGtpv1uMsgT *) hMsg;
  return NW_GTPV1U_OK;
}

/**
 * Set TEID for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 * @param[in] teid: TEID value.
 */

NwGtpv1uRcT
nwGtpv1uMsgSetTeid(NW_IN NwGtpv1uMsgHandleT hMsg, uint32_t teid)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  thiz->teid = teid;
#if defined(LOG_GTPU) && LOG_GTPU > 0
  GTPU_DEBUG("nwGtpv1uMsgSetTeid() teid %u", teid);
#endif
  return NW_GTPV1U_OK;
}

/**
 * Set sequence for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 * @param[in] seqNum: Flag boolean value.
 */

NwGtpv1uRcT
nwGtpv1uMsgSetSeqNumber(NW_IN NwGtpv1uMsgHandleT hMsg, uint32_t seqNum)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  thiz->seqNum = seqNum;
  return NW_GTPV1U_OK;
}

/**
 * Get TEID present for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

uint32_t
nwGtpv1uMsgGetTeid(NW_IN NwGtpv1uMsgHandleT hMsg)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  return (thiz->teid);
}


/**
 * Get sequence number for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

uint32_t
nwGtpv1uMsgGetSeqNumber(NW_IN NwGtpv1uMsgHandleT hMsg)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  return (thiz->seqNum);
}

/**
 * Get msg type for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

uint32_t
nwGtpv1uMsgGetMsgType(NW_IN NwGtpv1uMsgHandleT hMsg)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  return (thiz->msgType);
}

/**
 * Get tpdu for gtpv1u message.
 *
 * @param[in] hMsg : Message handle.
 */

NwGtpv1uRcT
nwGtpv1uMsgGetTpdu(NW_IN NwGtpv1uMsgHandleT hMsg, uint8_t *pTpduBuf,
                   uint32_t *pTpduLength)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  uint8_t headerLength = ((thiz->seqNumFlag || thiz->extHdrFlag
                         || thiz->npduNumFlag) ? 12 : 8);

  *pTpduLength = thiz->msgLen - headerLength;
  memcpy(pTpduBuf, thiz->msgBuf + headerLength, *pTpduLength);
  return NW_GTPV1U_OK;
}

uint8_t *
nwGtpv1uMsgGetTpduHandle(NW_IN NwGtpv1uMsgHandleT hMsg)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  return (thiz->msgBuf + ((thiz->seqNumFlag || thiz->extHdrFlag
                           || thiz->npduNumFlag) ? 12 : 8));
}

uint32_t
nwGtpv1uMsgGetTpduLength(NW_IN NwGtpv1uMsgHandleT hMsg)
{
  NwGtpv1uMsgT *thiz = (NwGtpv1uMsgT *) hMsg;
  return (thiz->msgLen - ((thiz->seqNumFlag || thiz->extHdrFlag
                           || thiz->npduNumFlag) ? 12 : 8));
}

NwGtpv1uRcT
nwGtpv1uMsgAddIeTV1(NW_IN NwGtpv1uMsgHandleT hMsg,
                    NW_IN uint8_t       type,
                    NW_IN uint8_t       value)
{
  NwGtpv1uMsgT *pMsg = (NwGtpv1uMsgT *) hMsg;
  NwGtpv1uIeTv1T *pIe;

  pIe = (NwGtpv1uIeTv1T *) (pMsg->msgBuf + pMsg->msgLen);

  pIe->t        = type;
  pIe->v        = value;

  pMsg->msgLen += sizeof(NwGtpv1uIeTv1T);

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwGtpv1uMsgAddIeTV2(NW_IN NwGtpv1uMsgHandleT hMsg,
                    NW_IN uint8_t       type,
                    NW_IN uint16_t      length,
                    NW_IN uint16_t      value)
{
  NwGtpv1uMsgT *pMsg = (NwGtpv1uMsgT *) hMsg;
  NwGtpv1uIeTv2T *pIe;

  pIe = (NwGtpv1uIeTv2T *) (pMsg->msgBuf + pMsg->msgLen);

  pIe->t        = type;
  pIe->v        = htons(value);

  pMsg->msgLen += sizeof(NwGtpv1uIeTv2T);

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwGtpv1uMsgAddIeTV4(NW_IN NwGtpv1uMsgHandleT hMsg,
                    NW_IN uint8_t       type,
                    NW_IN uint16_t      length,
                    NW_IN uint32_t      value)
{
  NwGtpv1uMsgT *pMsg = (NwGtpv1uMsgT *) hMsg;
  NwGtpv1uIeTv4T *pIe;

  pIe = (NwGtpv1uIeTv4T *) (pMsg->msgBuf + pMsg->msgLen);

  pIe->t        = type;
  pIe->v        = htonl(value);

  pMsg->msgLen += sizeof(NwGtpv1uIeTv4T);

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwGtpv1uMsgAddIe(NW_IN NwGtpv1uMsgHandleT hMsg,
                 NW_IN uint8_t       type,
                 NW_IN uint16_t      length,
                 NW_IN uint8_t      *pVal)
{
  NwGtpv1uMsgT *pMsg = (NwGtpv1uMsgT *) hMsg;
  NwGtpv1uIeTlvT *pIe;

  pIe = (NwGtpv1uIeTlvT *) (pMsg->msgBuf + pMsg->msgLen);

  pIe->t        = type;
  pIe->l        = htons(length);

  memcpy(pIe + 4, pVal, length);
  pMsg->msgLen += (4 + length);

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwGtpv1uMsgHexDump(NwGtpv1uMsgHandleT hMsg, FILE *fp)
{

  NwGtpv1uMsgT *pMsg = (NwGtpv1uMsgT *) hMsg;
  uint8_t *data = pMsg->msgBuf;
  uint32_t size = pMsg->msgLen;

  unsigned char *p = (unsigned char *)data;
  unsigned char c;
  int n;
  char bytestr[4] = {0};
  char addrstr[10] = {0};
  char hexstr[ 16*3 + 5] = {0};
  char charstr[16*1 + 5] = {0};
  fprintf((FILE *)fp, "\n");

  for(n=1; n<=size; n++) {
    if (n%16 == 1) {
      /* store address for this line */
      snprintf(addrstr, sizeof(addrstr), "%.4lx",
               (p-data) );
    }

    c = *p;

    if (isalnum(c) == 0) {
      c = '.';
    }

    /* store hex str (for left side) */
    snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
    strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

    /* store char str (for right side) */
    snprintf(bytestr, sizeof(bytestr), "%c", c);
    strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

    if(n%16 == 0) {
      /* line completed */
      fprintf((FILE *)fp, "[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
      hexstr[0] = 0;
      charstr[0] = 0;
    } else if(n%8 == 0) {
      /* half line: add whitespaces */
      strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
      strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
    }

    p++; /* next byte */
  }

  if (strlen(hexstr) > 0) {
    /* print rest of buffer if not empty */
    fprintf((FILE *)fp, "[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);

  }

  fprintf((FILE *)fp, "\n");

  return NW_GTPV1U_OK;
}

#ifdef __cplusplus
}
#endif



/*--------------------------------------------------------------------------*
 *                          E N D   O F   F I L E                           *
 *--------------------------------------------------------------------------*/
