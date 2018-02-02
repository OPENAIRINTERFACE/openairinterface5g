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
#include "EpsUpdateResult.h"
#include "GprsTimer.h"
#include "EpsMobileIdentity.h"
#include "TrackingAreaIdentityList.h"
#include "EpsBearerContextStatus.h"
#include "LocationAreaIdentification.h"
#include "MobileIdentity.h"
#include "EmmCause.h"
#include "PlmnList.h"
#include "EmergencyNumberList.h"
#include "EpsNetworkFeatureSupport.h"
#include "AdditionalUpdateResult.h"

#ifndef TRACKING_AREA_UPDATE_ACCEPT_H_
#define TRACKING_AREA_UPDATE_ACCEPT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define TRACKING_AREA_UPDATE_ACCEPT_MINIMUM_LENGTH ( \
    EPS_UPDATE_RESULT_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define TRACKING_AREA_UPDATE_ACCEPT_MAXIMUM_LENGTH ( \
    EPS_UPDATE_RESULT_MAXIMUM_LENGTH + \
    GPRS_TIMER_MAXIMUM_LENGTH + \
    EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH + \
    TRACKING_AREA_IDENTITY_LIST_MAXIMUM_LENGTH + \
    EPS_BEARER_CONTEXT_STATUS_MAXIMUM_LENGTH + \
    LOCATION_AREA_IDENTIFICATION_MAXIMUM_LENGTH + \
    MOBILE_IDENTITY_MAXIMUM_LENGTH + \
    EMM_CAUSE_MAXIMUM_LENGTH + \
    GPRS_TIMER_MAXIMUM_LENGTH + \
    GPRS_TIMER_MAXIMUM_LENGTH + \
    PLMN_LIST_MAXIMUM_LENGTH + \
    EMERGENCY_NUMBER_LIST_MAXIMUM_LENGTH + \
    EPS_NETWORK_FEATURE_SUPPORT_MAXIMUM_LENGTH + \
    ADDITIONAL_UPDATE_RESULT_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define TRACKING_AREA_UPDATE_ACCEPT_T3412_VALUE_PRESENT                  (1<<0)
# define TRACKING_AREA_UPDATE_ACCEPT_GUTI_PRESENT                         (1<<1)
# define TRACKING_AREA_UPDATE_ACCEPT_TAI_LIST_PRESENT                     (1<<2)
# define TRACKING_AREA_UPDATE_ACCEPT_EPS_BEARER_CONTEXT_STATUS_PRESENT    (1<<3)
# define TRACKING_AREA_UPDATE_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT (1<<4)
# define TRACKING_AREA_UPDATE_ACCEPT_MS_IDENTITY_PRESENT                  (1<<5)
# define TRACKING_AREA_UPDATE_ACCEPT_EMM_CAUSE_PRESENT                    (1<<6)
# define TRACKING_AREA_UPDATE_ACCEPT_T3402_VALUE_PRESENT                  (1<<7)
# define TRACKING_AREA_UPDATE_ACCEPT_T3423_VALUE_PRESENT                  (1<<8)
# define TRACKING_AREA_UPDATE_ACCEPT_EQUIVALENT_PLMNS_PRESENT             (1<<9)
# define TRACKING_AREA_UPDATE_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT        (1<<10)
# define TRACKING_AREA_UPDATE_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT  (1<<11)
# define TRACKING_AREA_UPDATE_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT     (1<<12)

typedef enum tracking_area_update_accept_iei_tag {
  TRACKING_AREA_UPDATE_ACCEPT_T3412_VALUE_IEI                   = 0x5A, /* 0x5A = 90 */
  TRACKING_AREA_UPDATE_ACCEPT_GUTI_IEI                          = 0x50, /* 0x50 = 80 */
  TRACKING_AREA_UPDATE_ACCEPT_TAI_LIST_IEI                      = 0x54, /* 0x54 = 84 */
  TRACKING_AREA_UPDATE_ACCEPT_EPS_BEARER_CONTEXT_STATUS_IEI     = 0x57, /* 0x57 = 87 */
  TRACKING_AREA_UPDATE_ACCEPT_LOCATION_AREA_IDENTIFICATION_IEI  = 0x13, /* 0x13 = 19 */
  TRACKING_AREA_UPDATE_ACCEPT_MS_IDENTITY_IEI                   = 0x23, /* 0x23 = 35 */
  TRACKING_AREA_UPDATE_ACCEPT_EMM_CAUSE_IEI                     = 0x53, /* 0x53 = 83 */
  TRACKING_AREA_UPDATE_ACCEPT_T3402_VALUE_IEI                   = 0x17, /* 0x17 = 23 */
  TRACKING_AREA_UPDATE_ACCEPT_T3423_VALUE_IEI                   = 0x59, /* 0x59 = 89 */
  TRACKING_AREA_UPDATE_ACCEPT_EQUIVALENT_PLMNS_IEI              = 0x4A, /* 0x4A = 74 */
  TRACKING_AREA_UPDATE_ACCEPT_EMERGENCY_NUMBER_LIST_IEI         = 0x34, /* 0x34 = 52 */
  TRACKING_AREA_UPDATE_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_IEI   = 0x64, /* 0x64 = 100 */
  TRACKING_AREA_UPDATE_ACCEPT_ADDITIONAL_UPDATE_RESULT_IEI      = 0xF0, /* 0xF0 = 240 */
} tracking_area_update_accept_iei;

/*
 * Message name: Tracking area update accept
 * Description: This message is sent by the network to the UE to provide the UE with EPS mobility management related data in response to a tracking area update request message. See table 8.2.26.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct tracking_area_update_accept_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                   protocoldiscriminator:4;
  SecurityHeaderType                      securityheadertype:4;
  MessageType                             messagetype;
  EpsUpdateResult                         epsupdateresult;
  /* Optional fields */
  uint32_t                                presencemask;
  GprsTimer                               t3412value;
  EpsMobileIdentity                       guti;
  TrackingAreaIdentityList                tailist;
  EpsBearerContextStatus                  epsbearercontextstatus;
  LocationAreaIdentification              locationareaidentification;
  MobileIdentity                          msidentity;
  EmmCause                                emmcause;
  GprsTimer                               t3402value;
  GprsTimer                               t3423value;
  PlmnList                                equivalentplmns;
  EmergencyNumberList                     emergencynumberlist;
  EpsNetworkFeatureSupport                epsnetworkfeaturesupport;
  AdditionalUpdateResult                  additionalupdateresult;
} tracking_area_update_accept_msg;

int decode_tracking_area_update_accept(tracking_area_update_accept_msg *trackingareaupdateaccept, uint8_t *buffer, uint32_t len);

int encode_tracking_area_update_accept(tracking_area_update_accept_msg *trackingareaupdateaccept, uint8_t *buffer, uint32_t len);

#endif /* ! defined(TRACKING_AREA_UPDATE_ACCEPT_H_) */

