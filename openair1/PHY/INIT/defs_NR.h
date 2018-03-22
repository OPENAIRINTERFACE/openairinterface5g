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

#ifndef __INIT_DEFS_NR__H__
#define __INIT_DEFS_NR__H__

#include "PHY/defs.h"

typedef enum {
  NR_MU_0=0,
  NR_MU_1,
  NR_MU_2,
  NR_MU_3,
  NR_MU_4,
} nr_numerology_index_e;

/// Subcarrier spacings in Hz indexed by numerology index
uint32_t nr_subcarrier_spacing[MAX_NUM_SUBCARRIER_SPACING] = {15e3, 30e3, 60e3, 120e3, 240e3};

#endif
