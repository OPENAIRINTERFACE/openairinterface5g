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

