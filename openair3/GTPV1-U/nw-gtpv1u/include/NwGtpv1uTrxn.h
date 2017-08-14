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

/**
 * @file NwGtpv1uTrxn.h
 * @author Amit Chawre
 * @brief
 *
 * This header file contains required definitions and functions
 * prototypes used by gtpv1u transactions.
 *
 **/


#ifndef __NW_GTPV1U_TRXN_H__
#define __NW_GTPV1U_TRXN_H__

#ifdef __cplusplus
extern "C" {
#endif

NwGtpv1uRcT
nwGtpv1uTrxnNew( NW_IN  NwGtpv1uStackT* thiz,
                 NW_OUT NwGtpv1uTrxnT **ppTrxn);


NwGtpv1uRcT
nwGtpv1uTrxnWithSeqNew( NW_IN  NwGtpv1uStackT* thiz,
                        NW_IN  uint32_t seqNum,
                        NW_OUT NwGtpv1uTrxnT **ppTrxn);

NwGtpv1uRcT
nwGtpv1uTrxnDelete( NW_INOUT NwGtpv1uTrxnT **ppTrxn);

NwGtpv1uRcT
nwGtpv1uTrxnNew( NW_IN  NwGtpv1uStackT* thiz,
                 NW_OUT NwGtpv1uTrxnT **ppTrxn);

NwGtpv1uRcT
nwGtpv1uTrxnDelete( NW_INOUT NwGtpv1uTrxnT **pthiz);

NwGtpv1uRcT
nwGtpv1uTrxnCreateAndSendMsg( NW_IN  NwGtpv1uStackT* thiz,
                              NW_IN  NwGtpv1uTrxnT *pTrxn,
                              NW_IN  uint32_t peerIp,
                              NW_IN  uint32_t peerPort,
                              NW_IN  NwGtpv1uMsgT *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* __NW_GTPV1U_TRXN_H__ */

/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

