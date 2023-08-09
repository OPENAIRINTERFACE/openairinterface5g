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

/*! \file RegistrationRequest.h
 * \brief registration request procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ExtendedProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "SpareHalfOctet.h"
#include "FGSMobileIdentity.h"
#include "NasKeySetIdentifier.h"
#include "FGSRegistrationType.h"
#include "MessageType.h"
#include "FGMMCapability.h"
#include "NrUESecurityCapability.h"

#ifndef REGISTRATION_REQUEST_H_
#define REGISTRATION_REQUEST_H_


# define REGISTRATION_REQUEST_NON_CURRENT_NATIVE_NAS_KEYSET_PRESENT           (1<<0)
# define REGISTRATION_REQUEST_5GMM_CAPABILITY_PRESENT                         (1<<1)
# define REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_PRESENT                  (1<<2)
# define REGISTRATION_REQUEST_REQUESTED_NSSAI_PRESENT                         (1<<3)
# define REGISTRATION_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT             (1<<4)
# define REGISTRATION_REQUEST_S1_UE_NETWORK_CAPABILITY_PRESENT                (1<<5)
# define REGISTRATION_REQUEST_UPLINK_DATA_STATUS_PRESENT                      (1<<6)
# define REGISTRATION_REQUEST_PDU_SESSION_STATUS_PRESENT                      (1<<7)
# define REGISTRATION_REQUEST_MICO_INDICATION_PRESENT                         (1<<8)
# define REGISTRATION_REQUEST_UE_STATUS_PRESENT                               (1<<9)
# define REGISTRATION_REQUEST_ADDITIONAL_GUTI_PRESENT                         (1<<10)
# define REGISTRATION_REQUEST_ALLOWED_PDU_SESSION_STATUS_PRESENT              (1<<11)
# define REGISTRATION_REQUEST_UE_USAGE_SETTING_PRESENT                        (1<<12)
# define REGISTRATION_REQUEST_REQUESTED_DRX_PARAMETERS_PRESENT                (1<<13)
# define REGISTRATION_REQUEST_EPS_NAS_MESSAGE_CONTAINER_PRESENT               (1<<14)
# define REGISTRATION_REQUEST_LADN_INDICATION_PRESENT                         (1<<15)
# define REGISTRATION_REQUEST_PAYLOAD_CONTAINER_TYPE_PRESENT                  (1<<16)
# define REGISTRATION_REQUEST_PAYLOAD_CONTAINER_PRESENT                       (1<<17)
# define REGISTRATION_REQUEST_NETWORK_SLICING_INDICATION_PRESENT              (1<<18)
# define REGISTRATION_REQUEST_5GS_UPDATE_TYPE_PRESENT                         (1<<19)
# define REGISTRATION_REQUEST_NAS_MESSAGE_CONTAINER_PRESENT                   (1<<20)
# define REGISTRATION_REQUEST_EPS_BEARER_CONTEXT_STATUS_PRESENT               (1<<21)

typedef enum registration_request_iei_tag {
  REGISTRATION_REQUEST_NON_CURRENT_NATIVE_NAS_KEYSET_IEI                          = 0xC0, /* 0xC- = 192- */
  REGISTRATION_REQUEST_5GMM_CAPABILITY_IEI                                        = 0x10, /* 0x10 = 16  */
  REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_IEI                                 = 0x2E, /* 0x2E = 46  */
  REGISTRATION_REQUEST_REQUESTED_NSSAI_IEI                                        = 0x2F, /* 0x2F = 47  */
  REGISTRATION_REQUEST_LAST_VISITED_REGISTERED_TAI_IEI                            = 0x52, /* 0x52 = 82  */
  REGISTRATION_REQUEST_S1_UE_NETWORK_CAPABILITY_IEI                               = 0x17, /* 0x17 = 23  */
  REGISTRATION_REQUEST_UPLINK_DATA_STATUS_IEI                                     = 0x40, /* 0x40 = 64  */
  REGISTRATION_REQUEST_PDU_SESSION_STATUS_IEI                                     = 0x50, /* 0x50 = 80  */
  REGISTRATION_REQUEST_MICO_INDICATION_IEI                                        = 0xB0, /* 0xB- = 176- */
  REGISTRATION_REQUEST_UE_STATUS_IEI                                              = 0x2B, /* 0x2B = 43  */
  REGISTRATION_REQUEST_ADDITIONAL_GUTI_IEI                                        = 0x77, /* 0x77 = 119  */
  REGISTRATION_REQUEST_ALLOWED_PDU_SESSION_STATUS_IEI                             = 0x25, /* 0x25 = 37  */
  REGISTRATION_REQUEST_UE_USAGE_SETTING_IEI                                       = 0x18, /* 0x18 = 24  */
  REGISTRATION_REQUEST_REQUESTED_DRX_PARAMETERS_IEI                               = 0x51, /* 0x51 = 81  */
  REGISTRATION_REQUEST_EPS_NAS_MESSAGE_CONTAINER_IEI                              = 0x70, /* 0x70 = 112  */
  REGISTRATION_REQUEST_LADN_INDICATION_IEI                                        = 0x74, /* 0x74 = 116  */
  REGISTRATION_REQUEST_PAYLOAD_CONTAINER_TYPE_IEI                                 = 0x80, /* 0x80 = 128  */
  REGISTRATION_REQUEST_PAYLOAD_CONTAINER_IEI                                      = 0x7B, /* 0x7B = 123  */
  REGISTRATION_REQUEST_NETWORK_SLICING_INDICATION_IEI                             = 0x90, /* 0x90 = 144  */
  REGISTRATION_REQUEST_5GS_UPDATE_TYPE_IEI                                        = 0x53, /* 0x53 = 83  */
  REGISTRATION_REQUEST_NAS_MESSAGE_CONTAINER_IEI                                  = 0x71, /* 0x71 = 113  */
  REGISTRATION_REQUEST_EPS_BEARER_CONTEXT_STATUS_IEI                              = 0x60  /* 0x60 = 96  */
} registration_request_iei;

/*
 * Message name: Registration request
 * Description: This message is sent by the UE to the AMF. See TS24.501 table 8.2.6.1.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct registration_request_msg_tag {
  /* Mandatory fields */
  ExtendedProtocolDiscriminator           protocoldiscriminator;
  SecurityHeaderType                      securityheadertype:4;
  SpareHalfOctet                          sparehalfoctet:4;
  MessageType                             messagetype;
  FGSRegistrationType                     fgsregistrationtype;
  NasKeySetIdentifier                     naskeysetidentifier;
  FGSMobileIdentity                       fgsmobileidentity;

  /* Optional fields */
  uint32_t                                presencemask;
  FGMMCapability                          fgmmcapability;
  NrUESecurityCapability                  nruesecuritycapability;
} registration_request_msg;

int decode_registration_request(registration_request_msg *registrationrequest, uint8_t *buffer, uint32_t len);

int encode_registration_request(registration_request_msg *registrationrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(REGISTRATION_REQUEST_H_) */

