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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "EpsAttachType.h"
#include "NasKeySetIdentifier.h"
#include "EpsMobileIdentity.h"
#include "UeNetworkCapability.h"
#include "EsmMessageContainer.h"
#include "PTmsiSignature.h"
#include "TrackingAreaIdentity.h"
#include "DrxParameter.h"
#include "MsNetworkCapability.h"
#include "MsNetworkFeatureSupport.h"
#include "LocationAreaIdentification.h"
#include "TmsiStatus.h"
#include "MobileStationClassmark2.h"
#include "MobileStationClassmark3.h"
#include "SupportedCodecList.h"
#include "AdditionalUpdateType.h"
#include "GutiType.h"
#include "VoiceDomainPreferenceAndUeUsageSetting.h"

#ifndef ATTACH_REQUEST_H_
#define ATTACH_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ATTACH_REQUEST_MINIMUM_LENGTH ( \
    EPS_ATTACH_TYPE_MINIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    EPS_MOBILE_IDENTITY_MINIMUM_LENGTH + \
    UE_NETWORK_CAPABILITY_MINIMUM_LENGTH + \
    ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ATTACH_REQUEST_MAXIMUM_LENGTH ( \
    EPS_ATTACH_TYPE_MAXIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH + \
    UE_NETWORK_CAPABILITY_MAXIMUM_LENGTH + \
    ESM_MESSAGE_CONTAINER_MAXIMUM_LENGTH + \
    P_TMSI_SIGNATURE_MAXIMUM_LENGTH + \
    EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH + \
    TRACKING_AREA_IDENTITY_MAXIMUM_LENGTH + \
    DRX_PARAMETER_MAXIMUM_LENGTH + \
    MS_NETWORK_CAPABILITY_MAXIMUM_LENGTH + \
    MS_NETWORK_FEATURE_SUPPORT_MAXIMUM_LENGTH +	  \
    LOCATION_AREA_IDENTIFICATION_MAXIMUM_LENGTH + \
    TMSI_STATUS_MAXIMUM_LENGTH + \
    MOBILE_STATION_CLASSMARK_2_MAXIMUM_LENGTH + \
    MOBILE_STATION_CLASSMARK_3_MAXIMUM_LENGTH + \
    SUPPORTED_CODEC_LIST_MAXIMUM_LENGTH + \
    ADDITIONAL_UPDATE_TYPE_MAXIMUM_LENGTH + \
    GUTI_TYPE_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT                           (1<<0)
# define ATTACH_REQUEST_ADDITIONAL_GUTI_PRESENT                               (1<<1)
# define ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT                   (1<<2)
# define ATTACH_REQUEST_DRX_PARAMETER_PRESENT                                 (1<<3)
# define ATTACH_REQUEST_MS_NETWORK_CAPABILITY_PRESENT                         (1<<4)
# define ATTACH_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT              (1<<5)
# define ATTACH_REQUEST_TMSI_STATUS_PRESENT                                   (1<<6)
# define ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT                    (1<<7)
# define ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT                    (1<<8)
# define ATTACH_REQUEST_SUPPORTED_CODECS_PRESENT                              (1<<9)
# define ATTACH_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT                        (1<<10)
# define ATTACH_REQUEST_OLD_GUTI_TYPE_PRESENT                                 (1<<11)
# define ATTACH_REQUEST_VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_PRESENT  (1<<12)
# define ATTACH_REQUEST_MS_NETWORK_FEATURE_SUPPORT_PRESENT                    (1<<13)

typedef enum attach_request_iei_tag {
  ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_IEI                          = 0x19, /* 0x19 = 25  */
  ATTACH_REQUEST_ADDITIONAL_GUTI_IEI                              = 0x50, /* 0x50 = 80  */
  ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_IEI                  = 0x52, /* 0x52 = 82  */
  ATTACH_REQUEST_DRX_PARAMETER_IEI                                = 0x5C, /* 0x5C = 92  */
  ATTACH_REQUEST_MS_NETWORK_FEATURE_SUPPORT_IEI                   = 0xC0, /* 0xC- = 192-  */
  ATTACH_REQUEST_MS_NETWORK_CAPABILITY_IEI                        = 0x31, /* 0x31 = 49  */
  ATTACH_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_IEI             = 0x13, /* 0x13 = 19  */
  ATTACH_REQUEST_TMSI_STATUS_IEI                                  = 0x90, /* 0x90 = 144 */
  ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_2_IEI                   = 0x11, /* 0x11 = 17  */
  ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_3_IEI                   = 0x20, /* 0x20 = 32  */
  ATTACH_REQUEST_SUPPORTED_CODECS_IEI                             = 0x40, /* 0x40 = 64  */
  ATTACH_REQUEST_ADDITIONAL_UPDATE_TYPE_IEI                       = 0xF0, /* 0xF0 = 240 */
  ATTACH_REQUEST_OLD_GUTI_TYPE_IEI                                = 0xE0, /* 0xE0 = 224 */
  ATTACH_REQUEST_VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_IEI = 0x5D, /* 0x5D = 93  */
} attach_request_iei;

/*
 * Message name: Attach request
 * Description: This message is sent by the UE to the network in order to perform an attach procedure. See table 8.2.4.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct attach_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                   protocoldiscriminator:4;
  SecurityHeaderType                      securityheadertype:4;
  MessageType                             messagetype;
  EpsAttachType                           epsattachtype;
  NasKeySetIdentifier                     naskeysetidentifier;
  EpsMobileIdentity                       oldgutiorimsi;
  UeNetworkCapability                     uenetworkcapability;
  EsmMessageContainer                     esmmessagecontainer;
  /* Optional fields */
  uint32_t                                presencemask;
  PTmsiSignature                          oldptmsisignature;
  EpsMobileIdentity                       additionalguti;
  TrackingAreaIdentity                    lastvisitedregisteredtai;
  DrxParameter                            drxparameter;
  MsNetworkCapability                     msnetworkcapability;
  LocationAreaIdentification              oldlocationareaidentification;
  TmsiStatus                              tmsistatus;
  MobileStationClassmark2                 mobilestationclassmark2;
  MobileStationClassmark3                 mobilestationclassmark3;
  SupportedCodecList                      supportedcodecs;
  AdditionalUpdateType                    additionalupdatetype;
  GutiType                                oldgutitype;
  VoiceDomainPreferenceAndUeUsageSetting  voicedomainpreferenceandueusagesetting;
  MsNetworkFeatureSupport                 msnetworkfeaturesupport;
} attach_request_msg;

int decode_attach_request(attach_request_msg *attachrequest, uint8_t *buffer, uint32_t len);

int encode_attach_request(attach_request_msg *attachrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ATTACH_REQUEST_H_) */

