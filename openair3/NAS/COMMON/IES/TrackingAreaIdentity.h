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

#include "OctetString.h"

#ifndef TRACKING_AREA_IDENTITY_H_
#define TRACKING_AREA_IDENTITY_H_

#define TRACKING_AREA_IDENTITY_MINIMUM_LENGTH 6
#define TRACKING_AREA_IDENTITY_MAXIMUM_LENGTH 6

typedef struct TrackingAreaIdentity_tag {
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint16_t tac;
} TrackingAreaIdentity;

int encode_tracking_area_identity(TrackingAreaIdentity *trackingareaidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_tracking_area_identity_xml(TrackingAreaIdentity *trackingareaidentity, uint8_t iei);

int decode_tracking_area_identity(TrackingAreaIdentity *trackingareaidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* TRACKING AREA IDENTITY_H_ */

