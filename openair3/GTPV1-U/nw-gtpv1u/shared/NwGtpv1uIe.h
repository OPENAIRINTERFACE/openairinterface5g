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
 * @file NwGtpv1uIe.h
 * @brief This header file contains Information Element definitions for GTPv1u
 * as per 3GPP TS 29281-930.
*/

#ifndef __NW_GTPV1U_IE_H__
#define __NW_GTPV1U_IE_H__

/*--------------------------------------------------------------------------*
 *   G T P V 2 U    I E  T Y P E     M A C R O     D E F I N I T I O N S    *
 *--------------------------------------------------------------------------*/

#define NW_GTPV1U_IE_RECOVERY                                           (14)
#define NW_GTPV1U_IE_TEID_DATA_I                                        (15)
#define NW_GTPV1U_IE_GSN_ADDRESS                                        (133)
#define NW_GTPV1U_IE_EXT_HDR_TYPE_LIST                                  (141)
#define NW_GTPV1U_IE_PRIVATE_EXTENSION                                  (255)


/*--------------------------------------------------------------------------*
 *   G T P V 2 U      C A U S E      V A L U E     D E F I N I T I O N S    *
 *--------------------------------------------------------------------------*/

#define NW_GTPV1U_CAUSE_REQUEST_ACCEPTED                                (16)
#define NW_GTPV1U_CAUSE_MANDATORY_IE_MISSING                            (70)

#endif  /* __NW_GTPV1U_IE_H__ */

/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

