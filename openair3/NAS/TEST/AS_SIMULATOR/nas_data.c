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

/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    nas_data.c

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Non-Access-Stratum data

Author    Frederic Maurel

Description Defines constants and functions used by the AS simulator
    process.

*****************************************************************************/

#include "nas_data.h"

#include "emm_msgDef.h"
#include "esm_msgDef.h"

#include "emm_cause.h"
#include "esm_cause.h"

#include <stdio.h>  // snprintf

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static ssize_t imsiIdentity(char* buffer, size_t len,
                            const ImsiMobileIdentity_t* ident);
static ssize_t imeiIdentity(char* buffer, size_t len,
                            const ImeiMobileIdentity_t* ident);
static ssize_t tmsiIdentity(char* buffer, size_t len,
                            const TmsiMobileIdentity_t* ident);
static ssize_t tmgiIdentity(char* buffer, size_t len,
                            const TmgiMobileIdentity_t* ident);

static const char* timerUnit(int unit);

static ssize_t ipv4Addr(char* buffer, size_t len, const OctetString* addr);
static ssize_t ipv6Addr(char* buffer, size_t len, const OctetString* addr);
static ssize_t ipv4v6Addr(char* buffer, size_t len, const OctetString* addr);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *        Return EMM message type
 * -----------------------------------------------------------------------------
 */
const char* emmMsgType(int type)
{
  if (type == ATTACH_REQUEST) {
    return "ATTACH_REQUEST";
  } else if (type == ATTACH_ACCEPT) {
    return "ATTACH_ACCEPT";
  } else if (type == ATTACH_COMPLETE) {
    return "ATTACH_COMPLETE";
  } else if (type == ATTACH_REJECT) {
    return "ATTACH_REJECT";
  } else if (type == DETACH_REQUEST) {
    return "DETACH_REQUEST";
  } else if (type == DETACH_ACCEPT) {
    return "DETACH_ACCEPT";
  } else if (type == TRACKING_AREA_UPDATE_REQUEST) {
    return "TRACKING_AREA_UPDATE_REQUEST";
  } else if (type == TRACKING_AREA_UPDATE_ACCEPT) {
    return "TRACKING_AREA_UPDATE_ACCEPT";
  } else if (type == TRACKING_AREA_UPDATE_COMPLETE) {
    return "TRACKING_AREA_UPDATE_COMPLETE";
  } else if (type == TRACKING_AREA_UPDATE_REJECT) {
    return "TRACKING_AREA_UPDATE_REJECT";
  } else if (type == EXTENDED_SERVICE_REQUEST) {
    return "EXTENDED_SERVICE_REQUEST";
  } else if (type == SERVICE_REJECT) {
    return "SERVICE_REJECT";
  } else if (type == GUTI_REALLOCATION_COMMAND) {
    return "GUTI_REALLOCATION_COMMAND";
  } else if (type == GUTI_REALLOCATION_COMPLETE) {
    return "GUTI_REALLOCATION_COMPLETE";
  } else if (type == AUTHENTICATION_REQUEST) {
    return "AUTHENTICATION_REQUEST";
  } else if (type == AUTHENTICATION_RESPONSE) {
    return "AUTHENTICATION_RESPONSE";
  } else if (type == AUTHENTICATION_REJECT) {
    return "AUTHENTICATION_REJECT";
  } else if (type == AUTHENTICATION_FAILURE) {
    return "AUTHENTICATION_FAILURE";
  } else if (type == IDENTITY_REQUEST) {
    return "IDENTITY_REQUEST";
  } else if (type == IDENTITY_RESPONSE) {
    return "IDENTITY_RESPONSE";
  } else if (type == SECURITY_MODE_COMMAND) {
    return "SECURITY_MODE_COMMAND";
  } else if (type == SECURITY_MODE_COMPLETE) {
    return "SECURITY_MODE_COMPLETE";
  } else if (type == SECURITY_MODE_REJECT) {
    return "SECURITY_MODE_REJECT";
  } else if (type == EMM_STATUS) {
    return "EMM_STATUS";
  } else if (type == EMM_INFORMATION) {
    return "EMM_INFORMATION";
  } else if (type == DOWNLINK_NAS_TRANSPORT) {
    return "DOWNLINK_NAS_TRANSPORT";
  } else if (type == UPLINK_NAS_TRANSPORT) {
    return "UPLINK_NAS_TRANSPORT";
  } else if (type == CS_SERVICE_NOTIFICATION) {
    return "CS_SERVICE_NOTIFICATION";
  } else if (type == SERVICE_REQUEST) {
    return "SERVICE_REQUEST";
  } else {
    return "Unknown EMM message type";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return ESM message type
 * -----------------------------------------------------------------------------
 */
const char* esmMsgType(int type)
{
  if (type == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST) {
    return "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST";
  } else if (type == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT) {
    return "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT";
  } else if (type == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT) {
    return "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT";
  } else if (type == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST) {
    return "ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST";
  } else if (type == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT) {
    return "ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT";
  } else if (type == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT) {
    return "ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT";
  } else if (type == MODIFY_EPS_BEARER_CONTEXT_REQUEST) {
    return "MODIFY_EPS_BEARER_CONTEXT_REQUEST";
  } else if (type == MODIFY_EPS_BEARER_CONTEXT_ACCEPT) {
    return "MODIFY_EPS_BEARER_CONTEXT_ACCEPT";
  } else if (type == MODIFY_EPS_BEARER_CONTEXT_REJECT) {
    return "MODIFY_EPS_BEARER_CONTEXT_REJECT";
  } else if (type == DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST) {
    return "DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST";
  } else if (type == DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT) {
    return "DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT";
  } else if (type == PDN_CONNECTIVITY_REQUEST) {
    return "PDN_CONNECTIVITY_REQUEST";
  } else if (type == PDN_CONNECTIVITY_REJECT) {
    return "PDN_CONNECTIVITY_REJECT";
  } else if (type == PDN_DISCONNECT_REQUEST) {
    return "PDN_DISCONNECT_REQUEST";
  } else if (type == PDN_DISCONNECT_REJECT) {
    return "PDN_DISCONNECT_REJECT";
  } else if (type == BEARER_RESOURCE_ALLOCATION_REQUEST) {
    return "BEARER_RESOURCE_ALLOCATION_REQUEST";
  } else if (type == BEARER_RESOURCE_ALLOCATION_REJECT) {
    return "BEARER_RESOURCE_ALLOCATION_REJECT";
  } else if (type == BEARER_RESOURCE_MODIFICATION_REQUEST) {
    return "BEARER_RESOURCE_MODIFICATION_REQUEST";
  } else if (type == BEARER_RESOURCE_MODIFICATION_REJECT) {
    return "BEARER_RESOURCE_MODIFICATION_REJECT";
  } else if (type == ESM_INFORMATION_REQUEST) {
    return "ESM_INFORMATION_REQUEST";
  } else if (type == ESM_INFORMATION_RESPONSE) {
    return "ESM_INFORMATION_RESPONSE";
  } else if (type == ESM_STATUS) {
    return "ESM_STATUS";
  } else {
    return "Unknown ESM message type";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return EMM cause code
 * -----------------------------------------------------------------------------
 */
const char* emmCauseCode(EmmCause code)
{
  if (code == EMM_CAUSE_SUCCESS) {
    return "SUCCESS";
  } else if (code == EMM_CAUSE_IMSI_UNKNOWN_IN_HSS) {
    return "IMSI_UNKNOWN_IN_HSS";
  } else if (code == EMM_CAUSE_ILLEGAL_UE) {
    return "ILLEGAL_UE";
  } else if (code == EMM_CAUSE_ILLEGAL_ME) {
    return "ILLEGAL_ME";
  } else if (code == EMM_CAUSE_INVALID_UE) {
    return "INVALID_UE";
  } else if (code == EMM_CAUSE_IMPLICITLY_DETACHED) {
    return "IMPLICITLY_DETACHED";
  } else if (code == EMM_CAUSE_IMEI_NOT_ACCEPTED) {
    return "IMEI_NOT_ACCEPTED";
  } else if (code == EMM_CAUSE_EPS_NOT_ALLOWED) {
    return "EPS_NOT_ALLOWED";
  } else if (code == EMM_CAUSE_BOTH_NOT_ALLOWED) {
    return "BOTH_NOT_ALLOWED";
  } else if (code == EMM_CAUSE_PLMN_NOT_ALLOWED) {
    return "PLMN_NOT_ALLOWED";
  } else if (code == EMM_CAUSE_TA_NOT_ALLOWED) {
    return "TA_NOT_ALLOWED";
  } else if (code == EMM_CAUSE_ROAMING_NOT_ALLOWED) {
    return "ROAMING_NOT_ALLOWED";
  } else if (code == EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN) {
    return "EPS_NOT_ALLOWED_IN_PLMN";
  } else if (code == EMM_CAUSE_NO_SUITABLE_CELLS) {
    return "NO_SUITABLE_CELLS";
  } else if (code == EMM_CAUSE_CSG_NOT_AUTHORIZED) {
    return "CSG_NOT_AUTHORIZED";
  } else if (code == EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN) {
    return "NOT_AUTHORIZED_IN_PLMN";
  } else if (code == EMM_CAUSE_NO_EPS_BEARER_CTX_ACTIVE) {
    return "NO_EPS_BEARER_CTX_ACTIVE";
  } else if (code == EMM_CAUSE_MSC_NOT_REACHABLE) {
    return "MSC_NOT_REACHABLE";
  } else if (code == EMM_CAUSE_NETWORK_FAILURE) {
    return "NETWORK_FAILURE";
  } else if (code == EMM_CAUSE_CS_DOMAIN_NOT_AVAILABLE) {
    return "CS_DOMAIN_NOT_AVAILABLE";
  } else if (code == EMM_CAUSE_ESM_FAILURE) {
    return "ESM_FAILURE";
  } else if (code == EMM_CAUSE_MAC_FAILURE) {
    return "MAC_FAILURE";
  } else if (code == EMM_CAUSE_SYNCH_FAILURE) {
    return "SYNCH_FAILURE";
  } else if (code == EMM_CAUSE_CONGESTION) {
    return "CONGESTION";
  } else if (code == EMM_CAUSE_UE_SECURITY_MISMATCH) {
    return "UE_SECURITY_MISMATCH";
  } else if (code == EMM_CAUSE_SECURITY_MODE_REJECTED) {
    return "SECURITY_MODE_REJECTED";
  } else if (code == EMM_CAUSE_NON_EPS_AUTH_UNACCEPTABLE) {
    return "NON_EPS_AUTH_UNACCEPTABLE";
  } else if (code == EMM_CAUSE_CS_SERVICE_NOT_AVAILABLE) {
    return "CS_SERVICE_NOT_AVAILABLE";
  } else if (code == EMM_CAUSE_SEMANTICALLY_INCORRECT) {
    return "SEMANTICALLY_INCORRECT";
  } else if (code == EMM_CAUSE_INVALID_MANDATORY_INFO) {
    return "INVALID_MANDATORY_INFO";
  } else if (code == EMM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED) {
    return "MESSAGE_TYPE_NOT_IMPLEMENTED";
  } else if (code == EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE) {
    return "MESSAGE_TYPE_NOT_COMPATIBLE";
  } else if (code == EMM_CAUSE_IE_NOT_IMPLEMENTED) {
    return "IE_NOT_IMPLEMENTED";
  } else if (code == EMM_CAUSE_CONDITIONAL_IE_ERROR) {
    return "CONDITIONAL_IE_ERROR";
  } else if (code == EMM_CAUSE_MESSAGE_NOT_COMPATIBLE) {
    return "MESSAGE_NOT_COMPATIBLE";
  } else if (code == EMM_CAUSE_PROTOCOL_ERROR) {
    return "PROTOCOL_ERROR";
  } else {
    return "Unknown EMM cause code";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return EMM cause code
 * -----------------------------------------------------------------------------
 */
const char* esmCauseCode(EsmCause code)
{
  if (code == ESM_CAUSE_SUCCESS) {
    return "SUCCESS";
  } else if (code == ESM_CAUSE_OPERATOR_DETERMINED_BARRING) {
    return "OPERATOR_DETERMINED_BARRING";
  } else if (code == ESM_CAUSE_INSUFFICIENT_RESOURCES) {
    return "INSUFFICIENT_RESOURCES";
  } else if (code == ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME) {
    return "UNKNOWN_ACCESS_POINT_NAME";
  } else if (code == ESM_CAUSE_UNKNOWN_PDN_TYPE) {
    return "UNKNOWN_PDN_TYPE";
  } else if (code == ESM_CAUSE_USER_AUTHENTICATION_FAILED) {
    return "USER_AUTHENTICATION_FAILED";
  } else if (code == ESM_CAUSE_REQUEST_REJECTED_BY_GW) {
    return "REQUEST_REJECTED_BY_GW";
  } else if (code == ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED) {
    return "REQUEST_REJECTED_UNSPECIFIED";
  } else if (code == ESM_CAUSE_SERVICE_OPTION_NOT_SUPPORTED) {
    return "SERVICE_OPTION_NOT_SUPPORTED";
  } else if (code == ESM_CAUSE_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED) {
    return "REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED";
  } else if (code == ESM_CAUSE_SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER) {
    return "SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER";
  } else if (code == ESM_CAUSE_PTI_ALREADY_IN_USE) {
    return "PTI_ALREADY_IN_USE";
  } else if (code == ESM_CAUSE_REGULAR_DEACTIVATION) {
    return "REGULAR_DEACTIVATION";
  } else if (code == ESM_CAUSE_EPS_QOS_NOT_ACCEPTED) {
    return "EPS_QOS_NOT_ACCEPTED";
  } else if (code == ESM_CAUSE_NETWORK_FAILURE) {
    return "NETWORK_FAILURE";
  } else if (code == ESM_CAUSE_REACTIVATION_REQUESTED) {
    return "REACTIVATION_REQUESTED";
  } else if (code == ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION) {
    return "SEMANTIC_ERROR_IN_THE_TFT_OPERATION";
  } else if (code == ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION) {
    return "SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION";
  } else if (code == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) {
    return "INVALID_EPS_BEARER_IDENTITY";
  } else if (code == ESM_CAUSE_SEMANTIC_ERRORS_IN_PACKET_FILTER) {
    return "SEMANTIC_ERRORS_IN_PACKET_FILTER";
  } else if (code == ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER) {
    return "SYNTACTICAL_ERROR_IN_PACKET_FILTER";
  } else if (code == ESM_CAUSE_PTI_MISMATCH) {
    return "PTI_MISMATCH";
  } else if (code == ESM_CAUSE_LAST_PDN_DISCONNECTION_NOT_ALLOWED) {
    return "LAST_PDN_DISCONNECTION_NOT_ALLOWED";
  } else if (code == ESM_CAUSE_PDN_TYPE_IPV4_ONLY_ALLOWED) {
    return "PDN_TYPE_IPV4_ONLY_ALLOWED";
  } else if (code == ESM_CAUSE_PDN_TYPE_IPV6_ONLY_ALLOWED) {
    return "PDN_TYPE_IPV6_ONLY_ALLOWED";
  } else if (code == ESM_CAUSE_SINGLE_ADDRESS_BEARERS_ONLY_ALLOWED) {
    return "SINGLE_ADDRESS_BEARERS_ONLY_ALLOWED";
  } else if (code == ESM_CAUSE_ESM_INFORMATION_NOT_RECEIVED) {
    return "ESM_INFORMATION_NOT_RECEIVED";
  } else if (code == ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST) {
    return "PDN_CONNECTION_DOES_NOT_EXIST";
  } else if (code == ESM_CAUSE_MULTIPLE_PDN_CONNECTIONS_NOT_ALLOWED) {
    return "MULTIPLE_PDN_CONNECTIONS_NOT_ALLOWED";
  } else if (code == ESM_CAUSE_COLLISION_WITH_NETWORK_INITIATED_REQUEST) {
    return "COLLISION_WITH_NETWORK_INITIATED_REQUEST";
  } else if (code == ESM_CAUSE_UNSUPPORTED_QCI_VALUE) {
    return "UNSUPPORTED_QCI_VALUE";
  } else if (code == ESM_CAUSE_BEARER_HANDLING_NOT_SUPPORTED) {
    return "BEARER_HANDLING_NOT_SUPPORTED";
  } else if (code == ESM_CAUSE_INVALID_PTI_VALUE) {
    return "INVALID_PTI_VALUE";
  } else if (code == ESM_CAUSE_APN_RESTRICTION_VALUE_NOT_COMPATIBLE) {
    return "APN_RESTRICTION_VALUE_NOT_COMPATIBLE";
  } else if (code == ESM_CAUSE_SEMANTICALLY_INCORRECT) {
    return "SEMANTICALLY_INCORRECT";
  } else if (code == ESM_CAUSE_INVALID_MANDATORY_INFO) {
    return "INVALID_MANDATORY_INFO";
  } else if (code == ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED) {
    return "MESSAGE_TYPE_NOT_IMPLEMENTED";
  } else if (code == ESM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE) {
    return "MESSAGE_TYPE_NOT_COMPATIBLE";
  } else if (code == ESM_CAUSE_IE_NOT_IMPLEMENTED) {
    return "IE_NOT_IMPLEMENTED";
  } else if (code == ESM_CAUSE_CONDITIONAL_IE_ERROR) {
    return "CONDITIONAL_IE_ERROR";
  } else if (code == ESM_CAUSE_MESSAGE_NOT_COMPATIBLE) {
    return "MESSAGE_NOT_COMPATIBLE";
  } else if (code == ESM_CAUSE_PROTOCOL_ERROR) {
    return "PROTOCOL_ERROR";
  } else {
    return "Unknown ESM cause code";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return attach type
 * -----------------------------------------------------------------------------
 */
const char* attachType(const EpsAttachType* type)
{
  if (*type == EPS_ATTACH_TYPE_EPS) {
    return "EPS";
  } else if (*type == EPS_ATTACH_TYPE_IMSI) {
    return "IMSI";
  } else if (*type == EPS_ATTACH_TYPE_EMERGENCY) {
    return "EMERGENCY";
  } else if (*type == EPS_ATTACH_TYPE_RESERVED) {
    return "RESERVED";
  } else {
    return "Unknown attach type";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return detach type
 * -----------------------------------------------------------------------------
 */
const char* detachType(const DetachType* type)
{
  if (type->switchoff == DETACH_TYPE_NORMAL_DETACH) {
    if (type->typeofdetach == DETACH_TYPE_EPS) {
      return "Normal EPS";
    } else if (type->typeofdetach == DETACH_TYPE_IMSI) {
      return "Normal IMSI";
    } else if (type->typeofdetach == DETACH_TYPE_EPS_IMSI) {
      return "Normal EMERGENCY";
    } else if (type->typeofdetach == DETACH_TYPE_RESERVED_1) {
      return "Normal RESERVED";
    } else if (type->typeofdetach == DETACH_TYPE_RESERVED_2) {
      return "Normal RESERVED";
    } else {
      return "Unknown attach type";
    }
  } else if (type->switchoff == DETACH_TYPE_SWITCH_OFF) {
    if (type->typeofdetach == DETACH_TYPE_EPS) {
      return "Switch-off EPS";
    } else if (type->typeofdetach == DETACH_TYPE_IMSI) {
      return "Switch-off IMSI";
    } else if (type->typeofdetach == DETACH_TYPE_EPS_IMSI) {
      return "Switch-off EMERGENCY";
    } else if (type->typeofdetach == DETACH_TYPE_RESERVED_1) {
      return "Switch-off RESERVED";
    } else if (type->typeofdetach == DETACH_TYPE_RESERVED_2) {
      return "Switch-off RESERVED";
    } else {
      return "Unknown attach type";
    }
  }

  return "Unknon switch-off parameter";
}

/*
 * -----------------------------------------------------------------------------
 *        Display EPS mobile identity
 * -----------------------------------------------------------------------------
 */
ssize_t epsIdentity(char* buffer, size_t len, const EpsMobileIdentity* ident)
{
  int index = 0;

  if (ident->imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    index += imsiIdentity(buffer + index, len - index,
                          (ImsiMobileIdentity_t*)(&ident->imsi));
  } else if (ident->imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    index += imeiIdentity(buffer + index, len - index,
                          (ImeiMobileIdentity_t*)(&ident->imei));
  } else if (ident->guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    index += snprintf(buffer + index, len - index, "GUTI = {");
    index += snprintf(buffer + index, len - index,
                      "plmn = %u%u%u%u%u",
                      ident->guti.mccdigit1, ident->guti.mccdigit2,
                      ident->guti.mccdigit3, ident->guti.mncdigit1,
                      ident->guti.mncdigit2);

    if (ident->guti.mncdigit3 != 0xf) {
      index += snprintf(buffer + index, len - index, "%u",
                        ident->guti.mncdigit3);
    }

    index += snprintf(buffer + index, len - index,
                      ", MMEgid = %u, MMEcode = %u, m_tmsi = %u}",
                      ident->guti.mmegroupid, ident->guti.mmecode,
                      ident->guti.mtmsi);
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *        Display mobile identity
 * -----------------------------------------------------------------------------
 */
ssize_t mobileIdentity(char* buffer, size_t len, const MobileIdentity* ident)
{
  int index = 0;

  if (ident->imsi.typeofidentity == MOBILE_IDENTITY_IMSI) {
    index += imsiIdentity(buffer + index, len - index, &ident->imsi);
  } else if (ident->imei.typeofidentity == MOBILE_IDENTITY_IMEI) {
    index += imeiIdentity(buffer + index, len - index, &ident->imei);
  } else if (ident->tmsi.typeofidentity == MOBILE_IDENTITY_TMSI) {
    index += tmsiIdentity(buffer + index, len - index, &ident->tmsi);
  } else if (ident->tmgi.typeofidentity == MOBILE_IDENTITY_TMGI) {
    index += tmgiIdentity(buffer + index, len - index, &ident->tmgi);
  } else if (ident->no_id.typeofidentity == MOBILE_IDENTITY_NOT_AVAILABLE) {
    index += snprintf(buffer + index, len - index, "NOT AVAILABLE");
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *        Return mobile identity type
 * -----------------------------------------------------------------------------
 */
const char* identityType(const IdentityType2* type)
{
  if (*type == IDENTITY_TYPE_2_IMSI) {
    return "IMSI";
  } else if (*type == IDENTITY_TYPE_2_IMEI) {
    return "IMEI";
  } else if (*type == IDENTITY_TYPE_2_IMEISV) {
    return "IMEISV";
  } else if (*type == IDENTITY_TYPE_2_TMSI) {
    return "TMSI";
  } else {
    return "Unknown identity type";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return PDN request type
 * -----------------------------------------------------------------------------
 */
const char* requestType(const RequestType* type)
{
  if (*type == REQUEST_TYPE_INITIAL_REQUEST) {
    return "INITIAL";
  } else if (*type == REQUEST_TYPE_HANDOVER) {
    return "HANDOVER";
  } else if (*type == REQUEST_TYPE_UNUSED) {
    return "UNUSED";
  } else if (*type == REQUEST_TYPE_EMERGENCY) {
    return "EMERGENCY";
  } else {
    return "Unknown PDN request type";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return PDN type
 * -----------------------------------------------------------------------------
 */
const char* pdnType(const PdnType* type)
{
  if (*type == PDN_TYPE_IPV4) {
    return "IPV4";
  } else if (*type == PDN_TYPE_IPV6) {
    return "IPV6";
  } else if (*type == PDN_TYPE_IPV4V6) {
    return "IPV4V6";
  } else if (*type == PDN_TYPE_UNUSED) {
    return "UNUSED";
  } else {
    return "Unknown PDN type";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Display PDN address
 * -----------------------------------------------------------------------------
 */
ssize_t pdnAddress(char* buffer, size_t len, const PdnAddress* addr)
{
  int index = 0;

  switch (addr->pdntypevalue) {
  case PDN_VALUE_TYPE_IPV4:
    /* Display IPv4 PDN address */
    index += ipv4Addr(buffer + index, len - index,
                      &addr->pdnaddressinformation);
    break;

  case PDN_VALUE_TYPE_IPV6:
    /* Display IPv6 suffix */
    index += ipv6Addr(buffer + index, len - index,
                      &addr->pdnaddressinformation);
    break;

  case PDN_VALUE_TYPE_IPV4V6:
    /* Display IPv4 PDN address and IPv6 suffix */
    index += ipv4v6Addr(buffer + index, len - index,
                        &addr->pdnaddressinformation);
    break;

  default:
    index += snprintf(buffer + index, len - index, "Unknown");
    break;
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *      Dispay the NAS key set identifier
 * -----------------------------------------------------------------------------
 */
ssize_t nasKeySetIdentifier(char* buffer, size_t len,
                            const NasKeySetIdentifier* ksi)
{
  int index = 0;

  index += snprintf(buffer + index, len - index, "{%s: %d}",
                    (ksi->tsc == NAS_KEY_SET_IDENTIFIER_NATIVE)? "NATIVE" :
                    (ksi->tsc == NAS_KEY_SET_IDENTIFIER_MAPPED)? "MAPPED" :
                    "Unknown", ksi->naskeysetidentifier);

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *    Dispay the authentication parameter (RAND, AUTN, RES)
 * -----------------------------------------------------------------------------
 */
ssize_t authenticationParameter(char* buffer, size_t len, const OctetString* param)
{
  int index = 0;

  for (int i = 0; i < param->length; i++) {
    index += snprintf(buffer + index, len - index, "%.2hx",
                      (unsigned char)(param->value[i]));
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *        Return the NAS ciphering algorithm identifier
 * -----------------------------------------------------------------------------
 */
const char* nasCipheringAlgorithm(const NasSecurityAlgorithms* algo)
{
  if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA0) {
    return "EEA0";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA1) {
    return "128-EEA1";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA2) {
    return "128-EEA2";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA3) {
    return "EEA3";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA4) {
    return "EEA4";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA5) {
    return "EEA5";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA6) {
    return "EEA6";
  } else if (algo->typeofcipheringalgorithm == NAS_SECURITY_ALGORITHMS_EEA7) {
    return "EEA7";
  } else {
    return "Unknown";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return the NAS integrity algorithm identifier
 * -----------------------------------------------------------------------------
 */
const char* nasIntegrityAlgorithm(const NasSecurityAlgorithms* algo)
{
  if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA0) {
    return "EIA0";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA1) {
    return "128-EIA1";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA2) {
    return "128-EIA2";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA3) {
    return "EIA3";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA4) {
    return "EIA4";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA5) {
    return "EIA5";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA6) {
    return "EIA6";
  } else if (algo->typeofintegrityalgorithm == NAS_SECURITY_ALGORITHMS_EIA7) {
    return "EIA7";
  } else {
    return "Unknown";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Display GPRS timer
 * -----------------------------------------------------------------------------
 */
ssize_t gprsTimer(char* buffer, size_t len, const GprsTimer* timer)
{
  int index = 0;

  if (timer->unit != GPRS_TIMER_UNIT_0S) {
    index += snprintf(buffer + index, len - index,
                      "{Unit = %s, Value = %u}",
                      timerUnit(timer->unit), timer->timervalue);
  } else {
    index += snprintf(buffer + index, len - index, "{Deactivated}");
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *      Display the content of TAI list
 * -----------------------------------------------------------------------------
 */
ssize_t taiList(char* buffer, size_t len, const TrackingAreaIdentityList* tai)
{
  int index = 0;

  index += snprintf(buffer + index, len - index, "TAI = {%s",
                    (tai->typeoflist == TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS)?
                    "One PLMN consecutive TACs" : "Unknown");

  index += snprintf(buffer + index, len - index,
                    ", plmn = %u%u%u%u%u",
                    tai->mccdigit1, tai->mccdigit2,
                    tai->mccdigit3, tai->mncdigit1,
                    tai->mncdigit2);

  if (tai->mncdigit3 != 0xf) {
    index += snprintf(buffer + index, len - index, "%u",
                      tai->mncdigit3);
  }

  index += snprintf(buffer + index, len - index,
                    ", n_tacs = %u, tac = 0x%.4x",
                    /* LW: number of elements is coded as N-1 (0 -> 1 element, 1 -> 2 elements...),
                     *  see 3GPP TS 24.301, section 9.9.3.33.1 */
                    tai->numberofelements + 1,
                    tai->tac);

  index += snprintf(buffer + index, len - index, "}");

  return (index);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *    Display the International Mobile Subscriber Identity
 * -----------------------------------------------------------------------------
 */
ssize_t imsiIdentity(char* buffer, size_t len, const ImsiMobileIdentity_t* imsi)
{
  int index = 0;

  if (imsi->oddeven != MOBILE_IDENTITY_EVEN) {
    index += snprintf(buffer + index, len - index,
                      "IMSI = %u%u%u%u%u%u%u%u%u%u%u%u%u%u",
                      imsi->digit1, imsi->digit2, imsi->digit3, imsi->digit4,
                      imsi->digit5, imsi->digit6, imsi->digit7, imsi->digit8,
                      imsi->digit9, imsi->digit10, imsi->digit11, imsi->digit12,
                      imsi->digit13, imsi->digit14);
  } else {
    index += snprintf(buffer + index, len - index,
                      "IMSI = %u%u%u%u%u%u%u%u%u%u%u%u%u",
                      imsi->digit1, imsi->digit2, imsi->digit3, imsi->digit4,
                      imsi->digit5, imsi->digit7, imsi->digit8, imsi->digit9,
                      imsi->digit10, imsi->digit11, imsi->digit12,
                      imsi->digit13, imsi->digit14);
  }

  if (imsi->digit15 != 0xf) {
    index += snprintf(buffer + index, len - index, "%u", imsi->digit15);
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *    Display the International Mobile Equipment Identity
 * -----------------------------------------------------------------------------
 */
ssize_t imeiIdentity(char* buffer, size_t len, const ImeiMobileIdentity_t* imei)
{
  int index = 0;

  if (imei->oddeven != MOBILE_IDENTITY_EVEN) {
    index += snprintf(buffer + index, len - index,
                      "IMEI = %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u",
                      imei->digit1, imei->digit2, imei->digit3, imei->digit4,
                      imei->digit5, imei->digit6, imei->digit7, imei->digit8,
                      imei->digit9, imei->digit10, imei->digit11, imei->digit12,
                      imei->digit13, imei->digit14, imei->digit15);
  } else {
    index += snprintf(buffer + index, len - index,
                      "IMEI = %u%u%u%u%u%u%u%u%u%u%u%u%u%u",
                      imei->digit1, imei->digit2, imei->digit3, imei->digit4,
                      imei->digit5, imei->digit6, imei->digit7, imei->digit8,
                      imei->digit9, imei->digit10, imei->digit11, imei->digit12,
                      imei->digit13, imei->digit14);
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *    Display the Temporary Mobile Subscriber Identity
 * -----------------------------------------------------------------------------
 */
ssize_t tmsiIdentity(char* buffer, size_t len, const TmsiMobileIdentity_t* tmsi)
{
  int index = 0;

  index += snprintf(buffer + index, len - index,
                    "TMSI = %u%u%u%u%u%u%u%u%u%u%u%u%u%u",
                    tmsi->digit2, tmsi->digit3, tmsi->digit4,
                    tmsi->digit5, tmsi->digit6, tmsi->digit7, tmsi->digit8,
                    tmsi->digit9, tmsi->digit10, tmsi->digit11, tmsi->digit12,
                    tmsi->digit13, tmsi->digit14, tmsi->digit15);

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *    Display the Temporary Mobile Group Identity
 * -----------------------------------------------------------------------------
 */
ssize_t tmgiIdentity(char* buffer, size_t len, const TmgiMobileIdentity_t* tmgi)
{
  int index = 0;

  index += snprintf(buffer + index, len - index, "TMGI = {");
  index += snprintf(buffer + index, len - index, "MBMS service ID = %u",
                    tmgi->mbmsserviceid);

  if (tmgi->mccmncindication) {
    index += snprintf(buffer + index, len - index, ", MCC = %u%u%u",
                      tmgi->mccdigit1, tmgi->mccdigit2, tmgi->mccdigit3);
    index += snprintf(buffer + index, len - index, ", MNC = %u%u",
                      tmgi->mncdigit1, tmgi->mncdigit2);

    if (tmgi->mncdigit3 != 0xf) {
      index += snprintf(buffer + index, len - index, "%u",
                        tmgi->mncdigit3);
    }
  }

  if (tmgi->mbmssessionidindication) {
    index += snprintf(buffer + index, len - index, ", MBMS session ID = %u",
                      tmgi->mbmssessionid);
  }

  index += snprintf(buffer + index, len - index, "}");

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *        Return GPRS timer unit
 * -----------------------------------------------------------------------------
 */
const char* timerUnit(int unit)
{
  if (unit == GPRS_TIMER_UNIT_2S) {
    return "2 seconds";
  } else if (unit == GPRS_TIMER_UNIT_60S) {
    return "Minutes";
  } else if (unit == GPRS_TIMER_UNIT_360S) {
    return "Decihours";
  } else if (unit == GPRS_TIMER_UNIT_0S) {
    return "Deactivated";
  } else {
    return "Unknown timer unit";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Display IPv4 address
 * -----------------------------------------------------------------------------
 */
ssize_t ipv4Addr(char* buffer, size_t len, const OctetString* addr)
{
  return snprintf(buffer, len, "%u.%u.%u.%u",
                  addr->value[0], addr->value[1],
                  addr->value[2], addr->value[3]);
}

/*
 * -----------------------------------------------------------------------------
 *        Display IPv6 address
 * -----------------------------------------------------------------------------
 */
ssize_t ipv6Addr(char* buffer, size_t len, const OctetString* addr)
{
  return snprintf(buffer, len, "%x%.2x:%x%.2x:%x%.2x:%x%.2x",
                  addr->value[0], addr->value[1],
                  addr->value[2], addr->value[3],
                  addr->value[4], addr->value[5],
                  addr->value[6], addr->value[7]);
}

/*
 * -----------------------------------------------------------------------------
 *        Display IPv4v6 address
 * -----------------------------------------------------------------------------
 */
ssize_t ipv4v6Addr(char* buffer, size_t len, const OctetString* addr)
{
  return snprintf(buffer, len, "%u.%u.%u.%u / %x%.2x:%x%.2x:%x%.2x:%x%.2x",
                  addr->value[0], addr->value[1],
                  addr->value[2], addr->value[3],
                  addr->value[4], addr->value[5],
                  addr->value[6], addr->value[7],
                  addr->value[8], addr->value[9],
                  addr->value[10], addr->value[11]);
}

