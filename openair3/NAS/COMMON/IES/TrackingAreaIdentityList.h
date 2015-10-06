/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef TRACKING_AREA_IDENTITY_LIST_H_
#define TRACKING_AREA_IDENTITY_LIST_H_

#define TRACKING_AREA_IDENTITY_LIST_MINIMUM_LENGTH 8
#define TRACKING_AREA_IDENTITY_LIST_MAXIMUM_LENGTH 98

typedef struct TrackingAreaIdentityList_tag {
  /* XXX - The only supported type of list is a list of TACs
   * belonging to one PLMN, with consecutive TAC values */
  //#define TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_NON_CONSECUTIVE_TACS 0b00
#define TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS   0b01
  //#define TRACKING_AREA_IDENTITY_LIST_MANY_PLMNS      0b10
  uint8_t  typeoflist:2;
  uint8_t  numberofelements:5;
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint16_t tac;
} TrackingAreaIdentityList;

int encode_tracking_area_identity_list(TrackingAreaIdentityList *trackingareaidentitylist, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_tracking_area_identity_list(TrackingAreaIdentityList *trackingareaidentitylist, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_tracking_area_identity_list_xml(TrackingAreaIdentityList *trackingareaidentitylist, uint8_t iei);

#endif /* TRACKING AREA IDENTITY LIST_H_ */

