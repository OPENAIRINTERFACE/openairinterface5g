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

/*! \file LAYER2/MAC/defs.h
* \brief MAC data structures, constant, and function prototype
* \author Navid Nikaein and Raymond Knopp
* \date 2011
* \version 0.5
* \email navid.nikaein@eurecom.fr

*/
/** @defgroup _oai2  openair2 Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/*@}*/

#ifndef __LAYER2_MAC_DEFS_H__
#define __LAYER2_MAC_DEFS_H__



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "COMMON/platform_constants.h"
#include "BCCH-BCH-Message.h"
#include "RadioResourceConfigCommon.h"
#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "MeasGapConfig.h"
#include "SchedulingInfoList.h"
#include "TDD-Config.h"
#include "RACH-ConfigCommon.h"
#include "MeasObjectToAddModList.h"
#include "MobilityControlInfo.h"
#if defined(Rel10) || defined(Rel14)
#include "MBSFN-AreaInfoList-r9.h"
#include "MBSFN-SubframeConfigList.h"
#include "PMCH-InfoList-r9.h"
#include "SCellToAddMod-r10.h"
#endif
#ifdef Rel14
#include "SystemInformationBlockType1-v1310-IEs.h"
#endif

#include "nfapi_interface.h"
#include "PHY_INTERFACE/IF_Module.h"

#include "PHY/TOOLS/time_meas.h"

#include "PHY/defs_common.h" // for PRACH_RESOURCES_t

#include "targets/ARCH/COMMON/common_lib.h"

//solve implicit declaration
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

/** @defgroup _mac  MAC
 * @ingroup _oai2
 * @{
 */

/*!\brief Values of BCCH logical channel (fake)*/
#define NR_BCCH_DL_SCH 3			// SI

/*!\brief Values of PCCH logical channel (fake) */
#define NR_BCCH_BCH 5			// MIB
/*@}*/
#endif /*__LAYER2_MAC_DEFS_H__ */
