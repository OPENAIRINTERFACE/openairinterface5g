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

/*! \file FGSUplinkNasTransport.h

\brief uplink nas transport procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ExtendedProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "SpareHalfOctet.h"
#include "MessageType.h"

#ifndef FGS_UPLINK_NAS_TRANSPORT_H_
#define FGS_UPLINK_NAS_TRANSPORT_H_

/*
 * Message name: uplink nas transpaort
 * Description: The UL NAS TRANSPORT message transports message payload and associated information to the AMF. See table 8.2.10.1.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct PayloadContainerType_tag{
    uint8_t  iei:4;
    uint8_t  type:4;
}PayloadContainerType;
typedef struct FGSPayloadContainer_tag {
  OctetString payloadcontainercontents;
} FGSPayloadContainer;

typedef struct fgs_uplink_nas_transport_msg_tag {
    /* Mandatory fields */
    ExtendedProtocolDiscriminator           protocoldiscriminator;
    SecurityHeaderType                      securityheadertype:4;
    SpareHalfOctet                          sparehalfoctet:4;
    MessageType                             messagetype;
    PayloadContainerType                    payloadcontainertype;
    FGSPayloadContainer                     fgspayloadcontainer;
    /* Optional fields */
    uint16_t                                pdusessionid;
    uint8_t                                 requesttype;
    OctetString                             snssai;
    OctetString                             dnn;
} fgs_uplink_nas_transport_msg;

int encode_fgs_uplink_nas_transport(fgs_uplink_nas_transport_msg *fgs_security_mode_comp, uint8_t *buffer, uint32_t len);

#endif /* ! defined(FGS_UPLINK_NAS_TRANSPORT_H_) */


