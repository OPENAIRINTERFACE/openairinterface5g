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

/*! \file phy_procedures_lte_eNB.c
* \brief Implementation of common utilities for eNB/UE procedures from 36.213 LTE specifications
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"


nr_subframe_t nr_subframe_select(nfapi_config_request_t *cfg,unsigned char subframe)
{
  if (cfg->subframe_config.duplex_mode.value == FDD)
    return(SF_DL);
}

// First possible symbol is used with n=0
int nr_get_ssb_start_symbol(nfapi_config_request_t *cfg, NR_DL_FRAME_PARMS *fp)
{
  int mu = cfg->subframe_config.numerology_index_mu.value;
  int symbol = 0;

  switch(mu) {

  case NR_MU_0:
    symbol = 2;
    break;

  case NR_MU_1: // case B
    symbol = 4;
    break;

  case NR_MU_3:
    symbol = 4;
    break;

  case NR_MU_4:
    symbol = 8;
    break;

  default:
    AssertFatal(0==1, "Invalid numerology index %d for the synchronization block\n", mu);
  }

  if (cfg->sch_config.half_frame_index)
    symbol += (5 * fp->symbols_per_slot * fp->slots_per_subframe);

  return symbol;
}

void nr_set_ssb_first_subcarrier(nfapi_config_request_t *cfg)
{
  cfg->sch_config.ssb_subcarrier_offset.value = 0;
}
