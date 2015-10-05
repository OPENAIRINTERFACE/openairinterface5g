/*----------------------------------------------------------------------------*
 *                                                                            *
 *                              n w - g t p v 2 c                             *
 *    G P R S   T u n n e l i n g    P r o t o c o l   v 2 c    S t a c k     *
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

#ifndef __NW_GTPV2C_TUNNEL_H__
#define __NW_GTPV2C_TUNNEL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "NwTypes.h"
#include "NwUtils.h"
#include "NwError.h"
#include "NwGtpv2c.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NwGtpv2cStack;

typedef struct NwGtpv2cTunnel {
  uint32_t                        teid;
  uint32_t                        ipv4AddrRemote;
  NwGtpv2cUlpTunnelHandleT      hUlpTunnel;
  RB_ENTRY (NwGtpv2cTunnel)     tunnelMapRbtNode;            /**< RB Tree Data Structure Node        */
  struct NwGtpv2cTunnel*        next;
} NwGtpv2cTunnelT;

NwGtpv2cTunnelT*
nwGtpv2cTunnelNew(struct NwGtpv2cStack *hStack, uint32_t teid, uint32_t peerIpv4Addr, NwGtpv2cUlpTunnelHandleT hUlpTunnel);

NwRcT
nwGtpv2cTunnelDelete(struct NwGtpv2cStack *pStack, NwGtpv2cTunnelT* thiz);

NwRcT
nwGtpv2cTunnelGetUlpTunnelHandle( NwGtpv2cTunnelT* thiz, NwGtpv2cUlpTunnelHandleT* phUlpTunnel);

#ifdef __cplusplus
}
#endif

#endif

/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

