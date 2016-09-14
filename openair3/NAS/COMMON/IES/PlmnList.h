#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PLMN_LIST_H_
#define PLMN_LIST_H_

#define PLMN_LIST_MINIMUM_LENGTH 5
#define PLMN_LIST_MAXIMUM_LENGTH 47

typedef struct PlmnList_tag {
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
} PlmnList;

int encode_plmn_list(PlmnList *plmnlist, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_plmn_list(PlmnList *plmnlist, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_plmn_list_xml(PlmnList *plmnlist, uint8_t iei);

#endif /* PLMN LIST_H_ */

