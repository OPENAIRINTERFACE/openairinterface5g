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

Source    nas_data.h

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Non-Access-Stratum data

Author    Frederic Maurel

Description Defines constants and functions used by the AS simulator
    process.

*****************************************************************************/

#ifndef __NAS_DATA_H__
#define __NAS_DATA_H__

#include "EpsAttachType.h"
#include "DetachType.h"
#include "NasKeySetIdentifier.h"
#include "EpsMobileIdentity.h"
#include "MobileIdentity.h"
#include "IdentityType2.h"
#include "NasRequestType.h"
#include "PdnType.h"
#include "PdnAddress.h"
#include "NasSecurityAlgorithms.h"
#include "GprsTimer.h"
#include "TrackingAreaIdentityList.h"
#include "EmmCause.h"
#include "EsmCause.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

const char* emmMsgType(int type);
const char* esmMsgType(int type);

const char* emmCauseCode(EmmCause code);
const char* esmCauseCode(EsmCause code);

const char* attachType(const EpsAttachType* type);
const char* detachType(const DetachType* type);

ssize_t epsIdentity(char* buffer, size_t len, const EpsMobileIdentity* ident);
const char* identityType(const IdentityType2* type);
ssize_t mobileIdentity(char* buffer, size_t len, const MobileIdentity* ident);

const char* requestType(const RequestType* type);
const char* pdnType(const PdnType* type);
ssize_t pdnAddress(char* buffer, size_t len, const PdnAddress* addr);

ssize_t nasKeySetIdentifier(char* buffer, size_t len, const NasKeySetIdentifier* ksi);
ssize_t authenticationParameter(char* buffer, size_t len, const OctetString* param);
const char* nasCipheringAlgorithm(const NasSecurityAlgorithms* algo);
const char* nasIntegrityAlgorithm(const NasSecurityAlgorithms* algo);

ssize_t gprsTimer(char* buffer, size_t len, const GprsTimer* timer);
ssize_t taiList(char* buffer, size_t len, const TrackingAreaIdentityList* tai);

#endif // __NAS_DATA_H__
