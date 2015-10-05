/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

