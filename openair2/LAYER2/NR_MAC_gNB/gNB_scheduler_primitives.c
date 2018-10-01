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

/*! \file gNB_scheduler_primitives.c
 * \brief primitives used by gNB for BCH, RACH, ULSCH, DLSCH scheduling
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 * @ingroup _mac

 */

#include "assertions.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/MAC/mac_extern.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#include "T.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_gNB_SCHEDULER 1

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

extern int n_active_slices;

int is_nr_UL_sf(NR_COMMON_channels_t * ccP, sub_frame_t subframeP){
  // if FDD return dummy value
  if (ccP->tdd_Config == NULL)
    return (0);

  switch (ccP->tdd_Config->subframeAssignment) {
  case 1:
    switch (subframeP) {
    case 0:
    case 4:
    case 5:
    case 9:
      return (0);
      break;
    case 2:
    case 3:
    case 7:
    case 8:
      return (1);
      break;
    default:
      return (0);
      break;
    }
    break;
  case 3:
    if ((subframeP <= 1) || (subframeP >= 5))
      return (0);
    else if ((subframeP > 1) && (subframeP < 5))
      return (1);
    else
      AssertFatal(1 == 0, "Unknown subframe number\n");
    break;
  case 4:
    if ((subframeP <= 1) || (subframeP >= 4))
      return (0);
    else if ((subframeP > 1) && (subframeP < 4))
      return (1);
    else
      AssertFatal(1 == 0, "Unknown subframe number\n");
    break;
  case 5:
    if ((subframeP <= 1) || (subframeP >= 3))
      return (0);
    else if ((subframeP > 1) && (subframeP < 3))
      return (1);
    else
      AssertFatal(1 == 0, "Unknown subframe number\n");
    break;
  default:
    AssertFatal(1 == 0,
    "subframe %d Unsupported TDD configuration %d\n",
    subframeP, (int) ccP->tdd_Config->subframeAssignment);
    break;
  }
}
