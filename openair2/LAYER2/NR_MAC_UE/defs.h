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

#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/defs.h"
#include "COMMON/platform_constants.h"
#include "BCCH-BCH-Message.h"
#include "RadioResourceConfigCommon.h"
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


/*!\brief Top level UE MAC structure */
typedef struct {
    
    ////  MAC config
    drx_Config_t *drx_config;
    SchedulingRequestConfig_t *SchedulingRequestConfig;
    BSR_Config_t *BSR_Config;
    TAG_Config_t *TAG_Config;
    phr_Config_t *phr_Config;
    cs_RNTI_t *cs_RNTI;
    ServCellIndex_t *servCellIndex;

    ////  Serving cell config
    TDD_UL_DL_ConfigDedicated_t *tdd_UL_DL_ConfigurationDedicated;
    //  init DL BWP
    BWP_DownlinkDedicated_t *init_DL_BWP;
    //  DL BWP list, not default one
    BWP_Downlink_t **BWP_Downlink_list;
    int BWP_Downlink_count;
    //BWP_Id_t *firstActiveDownlinkBWP_Id;
    long *bwp_InactivityTimer;
    BWP_Id_t *defaultDownlinkBWP_Id;
    //struct UplinkConfig *uplinkConfig;
    //struct UplinkConfig *supplementaryUplink;
    PDSCH_ServingCellConfig_t *pdsch_ServingCellConfig;
    CSI_MeasConfig_t *csi_MeasConfig;
    //SRS_CarrierSwitching_t *carrierSwitching;
    //long *sCellDeactivationTimer /* OPTIONAL */;
    //struct CrossCarrierSchedulingConfig *crossCarrierSchedulingConfig   /* OPTIONAL */;
    TAG_Id_t tag_Id;
    //long *ue_BeamLockFunction    /* OPTIONAL */;
    //long *pathlossReferenceLinking   /* OPTIONAL */;    

} UE_MAC_INST;
#include "proto.h"
/*@}*/
#endif /*__LAYER2_MAC_DEFS_H__ */
