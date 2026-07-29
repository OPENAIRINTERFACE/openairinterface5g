/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __LAYER2_UE_CAPABILITY_HANDLER_H__
#define __LAYER2_UE_CAPABILITY_HANDLER_H__

#include "NR_UE-NR-Capability.h"

typedef struct {
  int dl_feature_set_percc_id;
  int ul_feature_set_percc_id;
  int dlset_id;
  int ulset_id;
} NR_feature_set_ids_t;

typedef enum {
  NR_SA, /* 1 NR CC */
  EN_DC, /* 1 EUTRA CC + 1 NR CC */
  NR_DC, /* 2 NR CCs */
} nr_rat_type_t;

NR_feature_set_ids_t get_feature_set_ids(const NR_UE_NR_Capability_t *cap, int band, nr_rat_type_t type);
#endif
