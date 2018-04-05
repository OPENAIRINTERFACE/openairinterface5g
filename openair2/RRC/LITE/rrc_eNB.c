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

/*! \file rrc_eNB.c
 * \brief rrc procedures for eNB
 * \author Navid Nikaein and  Raymond Knopp
 * \date 2011 - 2014
 * \version 1.0
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr
 */
#define RRC_ENB
#define RRC_ENB_C

#include "defs.h"
#include "extern.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "asn1_conversions.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "LAYER2/MAC/proto.h"
#include "UTIL/LOG/log.h"
#include "COMMON/mac_rrc_primitives.h"
#include "RRC/LITE/MESSAGES/asn1_msg.h"
#include "RRCConnectionRequest.h"
#include "RRCConnectionReestablishmentRequest.h"
//#include "ReestablishmentCause.h"
#include "BCCH-BCH-Message.h"
#include "UL-CCCH-Message.h"
#include "DL-CCCH-Message.h"
#include "UL-DCCH-Message.h"
#include "DL-DCCH-Message.h"
#include "TDD-Config.h"
#include "HandoverCommand.h"
#include "rlc.h"
#include "SIMULATION/ETH_TRANSPORT/extern.h"
#include "rrc_eNB_UE_context.h"
#include "platform_types.h"
#include "msc.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "T.h"

//#if defined(Rel10) || defined(Rel14)
#include "MeasResults.h"
//#endif

#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#include "OCG.h"
#include "OCG_extern.h"

#if defined(ENABLE_SECURITY)
#   include "UTIL/OSA/osa_defs.h"
#endif

#if defined(ENABLE_USE_MME)
#   include "rrc_eNB_S1AP.h"
#   include "rrc_eNB_GTPV1U.h"
#   if defined(ENABLE_ITTI)
#   else
#      include "../../S1AP/s1ap_eNB.h"
#   endif
#endif

#include "pdcp.h"
#include "gtpv1u_eNB_task.h"

#if defined(ENABLE_ITTI)
#   include "intertask_interface.h"
#endif

#if ENABLE_RAL
#   include "rrc_eNB_ral.h"
#endif

#include "SIMULATION/TOOLS/defs.h" // for taus

//#define XER_PRINT

extern RAN_CONTEXT_t RC;

#ifdef PHY_EMUL
extern EMULATION_VARS              *Emul_vars;
#endif
extern eNB_MAC_INST                *eNB_mac_inst;
extern UE_MAC_INST                 *UE_mac_inst;
#ifdef BIGPHYSAREA
extern void*                        bigphys_malloc(int);
#endif

extern uint16_t                     two_tier_hexagonal_cellIds[7];

mui_t                               rrc_eNB_mui = 0;

void
openair_rrc_on(
  const protocol_ctxt_t* const ctxt_pP
)
//-----------------------------------------------------------------------------
{
  int            CC_id;

    LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" ENB:OPENAIR RRC IN....\n",
          PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      rrc_config_buffer (&RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SI, BCCH, 1);
      RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SI.Active = 1;
      rrc_config_buffer (&RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0, CCCH, 1);
      RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Active = 1;
    }
}

//-----------------------------------------------------------------------------
static void
init_SI(
  const protocol_ctxt_t* const ctxt_pP,
  const int              CC_id
#if defined(ENABLE_ITTI)
  ,
  RrcConfigurationReq * configuration
#endif
)
//-----------------------------------------------------------------------------
{
#if defined(Rel10) || defined(Rel14)
  int                                 i;
#endif

#ifdef Rel14
  SystemInformationBlockType1_v1310_IEs_t *sib1_v13ext=(SystemInformationBlockType1_v1310_IEs_t *)NULL;
#endif

  LOG_D(RRC,"%s()\n\n\n\n",__FUNCTION__);

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].MIB = (uint8_t*) malloc16(4);
  // copy basic parameters
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].physCellId      = configuration->Nid_cell[CC_id];
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].p_eNB           = configuration->nb_antenna_ports[CC_id];
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Ncp             = configuration->prefix_type[CC_id];
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].dl_CarrierFreq  = configuration->downlink_frequency[CC_id];
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].ul_CarrierFreq  = configuration->downlink_frequency[CC_id]+ configuration->uplink_frequency_offset[CC_id];
#ifdef Rel14
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].pbch_repetition = configuration->pbch_repetition[CC_id];
#endif
  LOG_I(RRC, "Configuring MIB (N_RB_DL %d,phich_Resource %d,phich_Duration %d)\n", 
	(int)configuration->N_RB_DL[CC_id],
	(int)configuration->phich_resource[CC_id],
	(int)configuration->phich_duration[CC_id]);
  do_MIB(&RC.rrc[ctxt_pP->module_id]->carrier[CC_id],
#ifdef ENABLE_ITTI
	 configuration->N_RB_DL[CC_id],
	 configuration->phich_resource[CC_id],
	 configuration->phich_duration[CC_id]
#else
	 50,0,0
#endif
	 ,0);
  

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB1 = 0;
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 = 0;
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SIB1 = (uint8_t*) malloc16(32);

  AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SIB1!=NULL,PROTOCOL_RRC_CTXT_FMT" init_SI: FATAL, no memory for SIB1 allocated\n",
	      PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB1 = do_SIB1(&RC.rrc[ctxt_pP->module_id]->carrier[CC_id],ctxt_pP->module_id,CC_id
#if defined(ENABLE_ITTI)
								   , configuration
#endif
								   );

  AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB1 != 255,"FATAL, RC.rrc[enb_mod_idP].carrier[CC_id].sizeof_SIB1 == 255");

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SIB23 = (uint8_t*) malloc16(64);
  AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SIB23!=NULL,"cannot allocate memory for SIB");
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 = do_SIB23(
								     ctxt_pP->module_id,
								     
								     CC_id
#if defined(ENABLE_ITTI)
								     , configuration
#endif
								     );

  AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_SIB23 != 255,"FATAL, RC.rrc[mod].carrier[CC_id].sizeof_SIB23 == 255");
  

  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" SIB2/3 Contents (partial)\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.n_SB = %ld\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	pusch_ConfigBasic.n_SB);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.hoppingMode = %ld\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	pusch_ConfigBasic.hoppingMode);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.pusch_HoppingOffset = %ld\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	pusch_ConfigBasic.pusch_HoppingOffset);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.enable64QAM = %d\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	(int)RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	pusch_ConfigBasic.enable64QAM);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.groupHoppingEnabled = %d\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	(int)RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	ul_ReferenceSignalsPUSCH.groupHoppingEnabled);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.groupAssignmentPUSCH = %ld\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.sequenceHoppingEnabled = %d\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	(int)RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled);
  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT" pusch_config_common.cyclicShift  = %ld\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon.pusch_ConfigCommon.
	ul_ReferenceSignalsPUSCH.cyclicShift);
  
#if defined(Rel10) || defined(Rel14)

  if (RC.rrc[ctxt_pP->module_id]->carrier[CC_id].MBMS_flag > 0) {
    for (i = 0; i < RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->mbsfn_SubframeConfigList->list.count; i++) {
      // SIB 2
      //   LOG_D(RRC, "[eNB %d] mbsfn_SubframeConfigList.list.count = %ld\n", enb_mod_idP, RC.rrc[enb_mod_idP].sib2->mbsfn_SubframeConfigList->list.count);
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" SIB13 contents for MBSFN subframe allocation %d/%d(partial)\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    i,
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->mbsfn_SubframeConfigList->list.count);
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" mbsfn_Subframe_pattern is  = %x\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0] >> 0);
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" radioframe_allocation_period  = %ld (just index number, not the real value)\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationPeriod);   // need to display the real value, using array of char (like in dumping SIB2)
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" radioframe_allocation_offset  = %ld\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationOffset);
    }
    
    //   SIB13
    for (i = 0; i < RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13->mbsfn_AreaInfoList_r9.list.count; i++) {
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" SIB13 contents for MBSFN sync area %d/%d (partial)\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    i,
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13->mbsfn_AreaInfoList_r9.list.count);
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" MCCH Repetition Period: %ld (just index number, not real value)\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13->mbsfn_AreaInfoList_r9.list.array[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" MCCH Offset: %ld\n",
	    PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
	    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13->mbsfn_AreaInfoList_r9.list.array[i]->mcch_Config_r9.mcch_Offset_r9);
    }
  }
  else memset((void*)&RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13,0,sizeof(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13));
#endif

  LOG_D(RRC,
	PROTOCOL_RRC_CTXT_FMT" RRC_UE --- MAC_CONFIG_REQ (SIB1.tdd & SIB2 params) ---> MAC_UE\n",
	PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));

#ifdef Rel14
  if ((RC.rrc[ctxt_pP->module_id]->carrier[CC_id].mib.message.schedulingInfoSIB1_BR_r13>0) && 
      (RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR!=NULL)) {
      AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR->nonCriticalExtension!=NULL,
		  "sib2_br->nonCriticalExtension is null (v8.9)\n");
      AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR->nonCriticalExtension->nonCriticalExtension!=NULL,
		  "sib2_br->nonCriticalExtension is null (v9.2)\n");
      AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension!=NULL,
		  "sib2_br->nonCriticalExtension is null (v11.3)\n");
      AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension!=NULL,
		  "sib2_br->nonCriticalExtension is null (v12.5)\n");
      AssertFatal(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension!=NULL,
		  "sib2_br->nonCriticalExtension is null (v13.10)\n");
      sib1_v13ext = RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1_BR->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension;
  }
#endif

  LOG_D(RRC, "About to call rrc_mac_config_req_eNB\n");

  rrc_mac_config_req_eNB(ctxt_pP->module_id, CC_id,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].physCellId,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].p_eNB,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Ncp,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1->freqBandIndicator,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].dl_CarrierFreq,
#ifdef Rel14
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].pbch_repetition,
#endif
			 0, // rnti
			 (BCCH_BCH_Message_t *)
			 &RC.rrc[ctxt_pP->module_id]->carrier[CC_id].mib,
			 (RadioResourceConfigCommonSIB_t *) &
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->radioResourceConfigCommon,
#if defined(Rel14)
			 (RadioResourceConfigCommonSIB_t *) &
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2_BR->radioResourceConfigCommon,
#endif
			 (struct PhysicalConfigDedicated *)NULL,
#if defined(Rel10) || defined(Rel14)
			 (SCellToAddMod_r10_t *)NULL,
			 //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
			 (MeasObjectToAddMod_t **) NULL,
			 (MAC_MainConfig_t *) NULL, 0,
			 (struct LogicalChannelConfig *)NULL,
			 (MeasGapConfig_t *) NULL,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1->tdd_Config,
			 NULL,
			 &RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib1->schedulingInfoList,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].ul_CarrierFreq,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->freqInfo.ul_Bandwidth,
			 &RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->freqInfo.additionalSpectrumEmission,
			 (MBSFN_SubframeConfigList_t*) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib2->mbsfn_SubframeConfigList
#if defined(Rel10) || defined(Rel14)
			 ,
			 RC.rrc[ctxt_pP->module_id]->carrier[CC_id].MBMS_flag,
			 (MBSFN_AreaInfoList_r9_t*) & RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sib13->mbsfn_AreaInfoList_r9,
			 (PMCH_InfoList_r9_t *) NULL
#endif
#ifdef Rel14
			 , 
			 sib1_v13ext
#endif
			 );
}

#if defined(Rel10) || defined(Rel14)
/*------------------------------------------------------------------------------*/
static void
init_MCCH(
  module_id_t enb_mod_idP,
  int CC_id
)
//-----------------------------------------------------------------------------
{

  int                                 sync_area = 0;
  // initialize RRC_eNB_INST MCCH entry
  RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE =
    malloc(RC.rrc[enb_mod_idP]->carrier[CC_id].num_mbsfn_sync_area * sizeof(uint8_t*));

  for (sync_area = 0; sync_area < RC.rrc[enb_mod_idP]->carrier[CC_id].num_mbsfn_sync_area; sync_area++) {

    RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] = 0;
    RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area] = (uint8_t *) malloc16(32);

    AssertFatal(RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area] != NULL,
		"[eNB %d]init_MCCH: FATAL, no memory for MCCH MESSAGE allocated \n", enb_mod_idP);
    RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] = do_MBSFNAreaConfig(enb_mod_idP,
											    sync_area,
											    (uint8_t *)RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area],
											    &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch,
											    &RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message);
    
    LOG_I(RRC, "mcch message pointer %p for sync area %d \n",
	  RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESSAGE[sync_area],
	  sync_area);
    LOG_D(RRC, "[eNB %d] MCCH_MESSAGE  contents for Sync Area %d (partial)\n", enb_mod_idP, sync_area);
    LOG_D(RRC, "[eNB %d] CommonSF_AllocPeriod_r9 %ld\n", enb_mod_idP,
	  RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->commonSF_AllocPeriod_r9);
    LOG_D(RRC,
	  "[eNB %d] CommonSF_Alloc_r9.list.count (number of MBSFN Subframe Pattern) %d\n",
	  enb_mod_idP, RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->commonSF_Alloc_r9.list.count);
    LOG_D(RRC, "[eNB %d] MBSFN Subframe Pattern: %02x (in hex)\n",
	  enb_mod_idP,
	  RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->commonSF_Alloc_r9.list.array[0]->subframeAllocation.
	  choice.oneFrame.buf[0]);
    
    AssertFatal(RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] != 255,
		"RC.rrc[enb_mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[sync_area] == 255");
    RC.rrc[enb_mod_idP]->carrier[CC_id].MCCH_MESS[sync_area].Active = 1;
  }
  

  //Set the RC.rrc[enb_mod_idP]->MCCH_MESS.Active to 1 (allow to  transfer MCCH message RRC->MAC in function mac_rrc_data_req)

  // ??Configure MCCH logical channel
  // call mac_config_req with appropriate structure from ASN.1 description


  //  LOG_I(RRC, "DUY: serviceID is %d\n",RC.rrc[enb_mod_idP]->mcch_message->pmch_InfoList_r9.list.array[0]->mbms_SessionInfoList_r9.list.array[0]->tmgi_r9.serviceId_r9.buf[2]);
  //  LOG_I(RRC, "DUY: session ID is %d\n",RC.rrc[enb_mod_idP]->mcch_message->pmch_InfoList_r9.list.array[0]->mbms_SessionInfoList_r9.list.array[0]->sessionId_r9->buf[0]);
  rrc_mac_config_req_eNB(enb_mod_idP, CC_id,
			 0,0,0,0,0,
#ifdef Rel14 
			 0,
#endif
			 0,//rnti
			 (BCCH_BCH_Message_t *)NULL,
			 (RadioResourceConfigCommonSIB_t *) NULL,
#ifdef Rel14
			 (RadioResourceConfigCommonSIB_t *) NULL,
#endif
			 (struct PhysicalConfigDedicated *)NULL,
#if defined(Rel10) || defined(Rel14)
			 (SCellToAddMod_r10_t *)NULL,
			 //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
			 (MeasObjectToAddMod_t **) NULL,
			 (MAC_MainConfig_t *) NULL,
			 0,
			 (struct LogicalChannelConfig *)NULL,
			 (MeasGapConfig_t *) NULL,
			 (TDD_Config_t *) NULL,
			 (MobilityControlInfo_t *)NULL, 
			 (SchedulingInfoList_t *) NULL, 
			 0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
#if defined(Rel10) || defined(Rel14)
			 ,
			 0,
			 (MBSFN_AreaInfoList_r9_t *) NULL,
			 (PMCH_InfoList_r9_t *) & (RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->pmch_InfoList_r9)
#   endif
#   ifdef Rel14
			 ,
			 (SystemInformationBlockType1_v1310_IEs_t *)NULL
#   endif
			 );
  
  //LOG_I(RRC,"DUY: lcid after rrc_mac_config_req is %02d\n",RC.rrc[enb_mod_idP]->mcch_message->pmch_InfoList_r9.list.array[0]->mbms_SessionInfoList_r9.list.array[0]->logicalChannelIdentity_r9);

}

//-----------------------------------------------------------------------------
static void init_MBMS(
  module_id_t enb_mod_idP,
  int         CC_id,
  frame_t frameP
)
//-----------------------------------------------------------------------------
{
  // init the configuration for MTCH
  protocol_ctxt_t               ctxt;

  if (RC.rrc[enb_mod_idP]->carrier[CC_id].MBMS_flag > 0) {
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, enb_mod_idP, ENB_FLAG_YES, NOT_A_RNTI, frameP, 0,enb_mod_idP);

    LOG_D(RRC, "[eNB %d] Frame %d : Radio Bearer config request for MBMS\n", enb_mod_idP, frameP);   //check the lcid
    // Configuring PDCP and RLC for MBMS Radio Bearer

    rrc_pdcp_config_asn1_req(&ctxt,
                             (SRB_ToAddModList_t  *)NULL,  // SRB_ToAddModList
                             (DRB_ToAddModList_t  *)NULL,  // DRB_ToAddModList
                             (DRB_ToReleaseList_t *)NULL,
                             0,     // security mode
                             NULL,  // key rrc encryption
                             NULL,  // key rrc integrity
                             NULL   // key encryption
#   if defined(Rel10) || defined(Rel14)
                             , &(RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->pmch_InfoList_r9)
#   endif
                             ,NULL);

    rrc_rlc_config_asn1_req(&ctxt,
                            NULL, // SRB_ToAddModList
                            NULL,   // DRB_ToAddModList
                            NULL,   // DRB_ToReleaseList
                            &(RC.rrc[enb_mod_idP]->carrier[CC_id].mcch_message->pmch_InfoList_r9));

    //rrc_mac_config_req();
  }
}
#endif

//-----------------------------------------------------------------------------
uint8_t
rrc_eNB_get_next_transaction_identifier(
  module_id_t enb_mod_idP
)
//-----------------------------------------------------------------------------
{
  static uint8_t                      rrc_transaction_identifier[NUMBER_OF_eNB_MAX];
  rrc_transaction_identifier[enb_mod_idP] = (rrc_transaction_identifier[enb_mod_idP] + 1) % RRC_TRANSACTION_IDENTIFIER_NUMBER;
  LOG_T(RRC,"generated xid is %d\n",rrc_transaction_identifier[enb_mod_idP]);
  return rrc_transaction_identifier[enb_mod_idP];
}
/*------------------------------------------------------------------------------*/
/* Functions to handle UE index in eNB UE list */


////-----------------------------------------------------------------------------
//static module_id_t
//rrc_eNB_get_UE_index(
//                module_id_t enb_mod_idP,
//                uint64_t    UE_identity
//)
////-----------------------------------------------------------------------------
//{
//
//    boolean_t      reg = FALSE;
//    module_id_t    i;
//
//    AssertFatal(enb_mod_idP < NB_eNB_INST, "eNB index invalid (%d/%d)!", enb_mod_idP, NB_eNB_INST);
//
//    for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
//        if (RC.rrc[enb_mod_idP]->Info.UE_list[i] == UE_identity) {
//            // UE_identity already registered
//            reg = TRUE;
//            break;
//        }
//    }
//
//    if (reg == FALSE) {
//        return (UE_MODULE_INVALID);
//    } else
//        return (i);
//}


//-----------------------------------------------------------------------------
// return the ue context if there is already an UE with ue_identityP, NULL otherwise
static struct rrc_eNB_ue_context_s*
rrc_eNB_ue_context_random_exist(
  const protocol_ctxt_t* const ctxt_pP,
  const uint64_t               ue_identityP
)
//-----------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s*        ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
    if (ue_context_p->ue_context.random_ue_identity == ue_identityP)
      return ue_context_p;
  }
  return NULL;
}
//-----------------------------------------------------------------------------
// return the ue context if there is already an UE with the same S-TMSI(MMEC+M-TMSI), NULL otherwise
static struct rrc_eNB_ue_context_s*
rrc_eNB_ue_context_stmsi_exist(
  const protocol_ctxt_t* const ctxt_pP,
  const mme_code_t             mme_codeP,
  const m_tmsi_t               m_tmsiP
)
//-----------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s*        ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
    LOG_I(RRC,"checking for UE S-TMSI %x, mme %x (%p): rnti %x",
	  m_tmsiP, mme_codeP, ue_context_p, 
	  ue_context_p->ue_context.rnti);
    if (ue_context_p->ue_context.Initialue_identity_s_TMSI.presence == TRUE) {
      printf("=> S-TMSI %x, MME %x\n",
	    ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi,
	    ue_context_p->ue_context.Initialue_identity_s_TMSI.mme_code);
      if (ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi == m_tmsiP)
        if (ue_context_p->ue_context.Initialue_identity_s_TMSI.mme_code == mme_codeP)
          return ue_context_p;
    }
    else
      printf("\n");

  }
  return NULL;
}

//-----------------------------------------------------------------------------
// return a new ue context structure if ue_identityP, ctxt_pP->rnti not found in collection
static struct rrc_eNB_ue_context_s*
rrc_eNB_get_next_free_ue_context(
  const protocol_ctxt_t* const ctxt_pP,
  const uint64_t               ue_identityP
)
//-----------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s*        ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
					RC.rrc[ctxt_pP->module_id],
					ctxt_pP->rnti);

  if (ue_context_p == NULL) {
#if 0
    RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
      if (ue_context_p->ue_context.random_ue_identity == ue_identityP) {
        LOG_D(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" Cannot create new UE context, already exist rand UE id 0x%"PRIx64", uid %u\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
              ue_identityP,
              ue_context_p->local_uid);
        return NULL;
      }
    }
#endif
    ue_context_p = rrc_eNB_allocate_new_UE_context(RC.rrc[ctxt_pP->module_id]);

    if (ue_context_p == NULL) {
      LOG_E(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" Cannot create new UE context, no memory\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
      return NULL;
    }

    ue_context_p->ue_id_rnti                    = ctxt_pP->rnti; // here ue_id_rnti is just a key, may be something else
    ue_context_p->ue_context.rnti               = ctxt_pP->rnti; // yes duplicate, 1 may be removed
    ue_context_p->ue_context.random_ue_identity = ue_identityP;
    RB_INSERT(rrc_ue_tree_s, &RC.rrc[ctxt_pP->module_id]->rrc_ue_head, ue_context_p);
    LOG_D(RRC,
          PROTOCOL_RRC_CTXT_UE_FMT" Created new UE context uid %u\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          ue_context_p->local_uid);
    return ue_context_p;

  } else {
    LOG_E(RRC,
          PROTOCOL_RRC_CTXT_UE_FMT" Cannot create new UE context, already exist\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
    return NULL;
  }
}

#if 0 //!defined(ENABLE_USE_MME)
void rrc_eNB_emulation_notify_ue_module_id(
  const module_id_t ue_module_idP,
  const rnti_t      rntiP,
  const uint8_t     cell_identity_byte0P,
  const uint8_t     cell_identity_byte1P,
  const uint8_t     cell_identity_byte2P,
  const uint8_t     cell_identity_byte3P)
{
  module_id_t                         enb_module_id;
  struct rrc_eNB_ue_context_s*        ue_context_p = NULL;
  int                                 CC_id;

  // find enb_module_id
  for (enb_module_id = 0; enb_module_id < NUMBER_OF_eNB_MAX; enb_module_id++) {
    if(enb_module_id>0){ /*FIX LATER*/
      return;
    }
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      if (&RC.rrc[enb_module_id]->carrier[CC_id].sib1 != NULL) {
        if (
          (&RC.rrc[enb_module_id]->carrier[CC_id].sib1->cellAccessRelatedInfo.cellIdentity.buf[0] == cell_identity_byte0P) &&
          (&RC.rrc[enb_module_id]->carrier[CC_id].sib1->cellAccessRelatedInfo.cellIdentity.buf[1] == cell_identity_byte1P) &&
          (&RC.rrc[enb_module_id]->carrier[CC_id].sib1->cellAccessRelatedInfo.cellIdentity.buf[2] == cell_identity_byte2P) &&
          (&RC.rrc[enb_module_id]->carrier[CC_id].sib1->cellAccessRelatedInfo.cellIdentity.buf[3] == cell_identity_byte3P)
        ) {
          ue_context_p = rrc_eNB_get_ue_context(
                           RC.rrc[enb_module_id],
                           rntiP
                         );

          if (NULL != ue_context_p) {
            oai_emulation.info.eNB_ue_local_uid_to_ue_module_id[enb_module_id][ue_context_p->local_uid] = ue_module_idP;
          }

          //return;
        }
      }
    }
    oai_emulation.info.eNB_ue_module_id_to_rnti[enb_module_id][ue_module_idP] = rntiP;
  }

  AssertFatal(enb_module_id == NUMBER_OF_eNB_MAX,
              "Cell identity not found for ue module id %u rnti %x",
              ue_module_idP, rntiP);
}
#endif

//-----------------------------------------------------------------------------
void
rrc_eNB_free_mem_UE_context(
  const protocol_ctxt_t*               const ctxt_pP,
  struct rrc_eNB_ue_context_s*         const ue_context_pP
)
//-----------------------------------------------------------------------------
{
  int i;
  LOG_T(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Clearing UE context 0x%p (free internal structs)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        ue_context_pP);
#if defined(Rel10) || defined(Rel14)
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_SCellToAddMod_r10, &ue_context_pP->ue_context.sCell_config[0]);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_SCellToAddMod_r10, &ue_context_pP->ue_context.sCell_config[1]);
#endif

  if (ue_context_pP->ue_context.SRB_configList) {
    ASN_STRUCT_FREE(asn_DEF_SRB_ToAddModList, ue_context_pP->ue_context.SRB_configList);
    ue_context_pP->ue_context.SRB_configList = NULL;
  }

  for(i = 0;i < RRC_TRANSACTION_IDENTIFIER_NUMBER;i++){
      if (ue_context_pP->ue_context.SRB_configList2[i]) {
          free(ue_context_pP->ue_context.SRB_configList2[i]);
          ue_context_pP->ue_context.SRB_configList2[i] = NULL;
      }
  }

  if (ue_context_pP->ue_context.DRB_configList) {
    ASN_STRUCT_FREE(asn_DEF_DRB_ToAddModList, ue_context_pP->ue_context.DRB_configList);
    ue_context_pP->ue_context.DRB_configList = NULL;
  }

  for(i = 0;i < RRC_TRANSACTION_IDENTIFIER_NUMBER;i++){
      if (ue_context_pP->ue_context.DRB_configList2[i]) {
          free(ue_context_pP->ue_context.DRB_configList2[i]);
          ue_context_pP->ue_context.DRB_configList2[i] = NULL;
      }
      if (ue_context_pP->ue_context.DRB_Release_configList2[i]) {
          free(ue_context_pP->ue_context.DRB_Release_configList2[i]);
          ue_context_pP->ue_context.DRB_Release_configList2[i] = NULL;
      }
  }

  memset(ue_context_pP->ue_context.DRB_active, 0, sizeof(ue_context_pP->ue_context.DRB_active));

  if (ue_context_pP->ue_context.physicalConfigDedicated) {
    ASN_STRUCT_FREE(asn_DEF_PhysicalConfigDedicated, ue_context_pP->ue_context.physicalConfigDedicated);
    ue_context_pP->ue_context.physicalConfigDedicated = NULL;
  }

  if (ue_context_pP->ue_context.sps_Config) {
    ASN_STRUCT_FREE(asn_DEF_SPS_Config, ue_context_pP->ue_context.sps_Config);
    ue_context_pP->ue_context.sps_Config = NULL;
  }

  for (i=0; i < MAX_MEAS_OBJ; i++) {
    if (ue_context_pP->ue_context.MeasObj[i] != NULL) {
      ASN_STRUCT_FREE(asn_DEF_MeasObjectToAddMod, ue_context_pP->ue_context.MeasObj[i]);
      ue_context_pP->ue_context.MeasObj[i] = NULL;
    }
  }

  for (i=0; i < MAX_MEAS_CONFIG; i++) {
    if (ue_context_pP->ue_context.ReportConfig[i] != NULL) {
      ASN_STRUCT_FREE(asn_DEF_ReportConfigToAddMod, ue_context_pP->ue_context.ReportConfig[i]);
      ue_context_pP->ue_context.ReportConfig[i] = NULL;
    }
  }

  if (ue_context_pP->ue_context.QuantityConfig) {
    ASN_STRUCT_FREE(asn_DEF_QuantityConfig, ue_context_pP->ue_context.QuantityConfig);
    ue_context_pP->ue_context.QuantityConfig = NULL;
  }

  if (ue_context_pP->ue_context.mac_MainConfig) {
    ASN_STRUCT_FREE(asn_DEF_MAC_MainConfig, ue_context_pP->ue_context.mac_MainConfig);
    ue_context_pP->ue_context.mac_MainConfig = NULL;
  }

/*  if (ue_context_pP->ue_context.measGapConfig) {
    ASN_STRUCT_FREE(asn_DEF_MeasGapConfig, ue_context_pP->ue_context.measGapConfig);
    ue_context_pP->ue_context.measGapConfig = NULL;
  }*/
    if (ue_context_pP->ue_context.handover_info) {
      ASN_STRUCT_FREE(asn_DEF_Handover, ue_context_pP->ue_context.handover_info);
      ue_context_pP->ue_context.handover_info = NULL;
    }

  //SRB_INFO                           SI;
  //SRB_INFO                           Srb0;
  //SRB_INFO_TABLE_ENTRY               Srb1;
  //SRB_INFO_TABLE_ENTRY               Srb2;
  if (ue_context_pP->ue_context.measConfig) {
    ASN_STRUCT_FREE(asn_DEF_MeasConfig, ue_context_pP->ue_context.measConfig);
    ue_context_pP->ue_context.measConfig = NULL;
  }

  if (ue_context_pP->ue_context.measConfig) {
    ASN_STRUCT_FREE(asn_DEF_MeasConfig, ue_context_pP->ue_context.measConfig);
    ue_context_pP->ue_context.measConfig = NULL;
  }

  //HANDOVER_INFO                     *handover_info;
#if defined(ENABLE_SECURITY)
  //uint8_t kenb[32];
#endif
  //e_SecurityAlgorithmConfig__cipheringAlgorithm     ciphering_algorithm;
  //e_SecurityAlgorithmConfig__integrityProtAlgorithm integrity_algorithm;
  //uint8_t                            Status;
  //rnti_t                             rnti;
  //uint64_t                           random_ue_identity;
#if defined(ENABLE_ITTI)
  //UE_S_TMSI                          Initialue_identity_s_TMSI;
  //EstablishmentCause_t               establishment_cause;
  //ReestablishmentCause_t             reestablishment_cause;
  //uint16_t                           ue_initial_id;
  //uint32_t                           eNB_ue_s1ap_id :24;
  //security_capabilities_t            security_capabilities;
  //uint8_t                            nb_of_e_rabs;
  //e_rab_param_t                      e_rab[S1AP_MAX_E_RAB];
  //uint32_t                           enb_gtp_teid[S1AP_MAX_E_RAB];
  //transport_layer_addr_t             enb_gtp_addrs[S1AP_MAX_E_RAB];
  //rb_id_t                            enb_gtp_ebi[S1AP_MAX_E_RAB];
#endif
}

//-----------------------------------------------------------------------------
// should be called when UE is lost by eNB
void
rrc_eNB_free_UE(const module_id_t enb_mod_idP,const struct rrc_eNB_ue_context_s*        const ue_context_pP)
//-----------------------------------------------------------------------------
{


  protocol_ctxt_t                     ctxt;
#if !defined(ENABLE_USE_MME)
  module_id_t                         ue_module_id;
  /* avoid gcc warnings */
  (void)ue_module_id;
#endif
  rnti_t rnti = ue_context_pP->ue_context.rnti;
  int i, j , CC_id, pdu_number;
  LTE_eNB_ULSCH_t *ulsch = NULL;
  LTE_eNB_DLSCH_t *dlsch = NULL;
  nfapi_ul_config_request_body_t *ul_req_tmp = NULL;
  PHY_VARS_eNB *eNB_PHY = NULL;
  eNB_MAC_INST *eNB_MAC = RC.mac[enb_mod_idP];

  AssertFatal(enb_mod_idP < NB_eNB_INST, "eNB inst invalid (%d/%d) for UE %x!", enb_mod_idP, NB_eNB_INST, rnti);
  /*  ue_context_p = rrc_eNB_get_ue_context(
                   &RC.rrc[enb_mod_idP],
                   rntiP
                 );
  */
  if (NULL != ue_context_pP) {
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, enb_mod_idP, ENB_FLAG_YES, rnti, 0, 0,enb_mod_idP);
    LOG_W(RRC, "[eNB %d] Removing UE RNTI %x\n", enb_mod_idP, rnti);

#if defined(ENABLE_USE_MME)
   if( ue_context_pP->ue_context.ul_failure_timer >= 8 ) {
	LOG_I(RRC, "[eNB %d] S1AP_UE_CONTEXT_RELEASE_REQ RNTI %x\n", enb_mod_idP, rnti);
    rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ(enb_mod_idP, ue_context_pP, S1AP_CAUSE_RADIO_NETWORK, 21); // send cause 21: connection with ue lost
    /* From 3GPP 36300v10 p129 : 19.2.2.2.2 S1 UE Context Release Request (eNB triggered)
     * If the E-UTRAN internal reason is a radio link failure detected in the eNB, the eNB shall wait a sufficient time before
     *  triggering the S1 UE Context Release Request procedure
     *  in order to allow the UE to perform the NAS recovery
     *  procedure, see TS 23.401 [17].
     */
     return;
    }
#endif
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      eNB_PHY = RC.eNB[enb_mod_idP][CC_id];
      for (i=0; i<NUMBER_OF_UE_MAX; i++) {
        ulsch = eNB_PHY->ulsch[i];
        if((ulsch != NULL) && (ulsch->rnti == rnti)){
          LOG_I(RRC, "clean_eNb_ulsch UE %x \n", rnti);
          clean_eNb_ulsch(ulsch);
        }
      }
      for (i=0; i<NUMBER_OF_UE_MAX; i++) {
        dlsch = eNB_PHY->dlsch[i][0];
        if((dlsch != NULL) && (dlsch->rnti == rnti)){
          LOG_I(RRC, "clean_eNb_dlsch UE %x \n", rnti);
          clean_eNb_dlsch(dlsch);
        }
      }

      if (rrc_agent_registered[enb_mod_idP]) {
        agent_rrc_xface[enb_mod_idP]->flexran_agent_notify_ue_state_change(enb_mod_idP,
                              rnti, PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
      }

      for(j = 0; j < 10; j++){
        ul_req_tmp = &eNB_MAC->UL_req_tmp[CC_id][j].ul_config_request_body;
        if(ul_req_tmp){
          pdu_number = ul_req_tmp->number_of_pdus;
          for(int pdu_index = pdu_number-1; pdu_index >= 0; pdu_index--){
            if(ul_req_tmp->ul_config_pdu_list[pdu_index].ulsch_pdu.ulsch_pdu_rel8.rnti == rnti){
              LOG_I(RRC, "remove UE %x from ul_config_pdu_list %d/%d\n", rnti, pdu_index, pdu_number);
              if(pdu_index < pdu_number -1){
                memcpy(&ul_req_tmp->ul_config_pdu_list[pdu_index], &ul_req_tmp->ul_config_pdu_list[pdu_index+1], (pdu_number-1-pdu_index) * sizeof(nfapi_ul_config_request_pdu_t));
              }
              ul_req_tmp->number_of_pdus--;
            }
          }
        }
      }
    }
    rrc_mac_remove_ue(enb_mod_idP,rnti);
    rrc_rlc_remove_ue(&ctxt);
    pdcp_remove_UE(&ctxt);

    rrc_eNB_remove_ue_context(
      &ctxt,
      RC.rrc[enb_mod_idP],
      (struct rrc_eNB_ue_context_s*) ue_context_pP);
  }
}

//-----------------------------------------------------------------------------
void
rrc_eNB_process_RRCConnectionSetupComplete(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*         ue_context_pP,
  RRCConnectionSetupComplete_r8_IEs_t * rrcConnectionSetupComplete
)
//-----------------------------------------------------------------------------
{
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel UL-DCCH, " "processing RRCConnectionSetupComplete from UE (SRB1 Active)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));

  ue_context_pP->ue_context.Srb1.Active=1;  
  T(T_ENB_RRC_CONNECTION_SETUP_COMPLETE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#if defined(ENABLE_USE_MME)

  if (EPC_MODE_ENABLED == 1) {
    // Forward message to S1AP layer
    rrc_eNB_send_S1AP_NAS_FIRST_REQ(
      ctxt_pP,
      ue_context_pP,
      rrcConnectionSetupComplete);
  } else
#endif
  {
    // RRC loop back (no S1AP), send SecurityModeCommand to UE
    rrc_eNB_generate_SecurityModeCommand(
      ctxt_pP,
      ue_context_pP);
    // rrc_eNB_generate_UECapabilityEnquiry(enb_mod_idP,frameP,ue_mod_idP);
  }
}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_SecurityModeCommand(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP
)
//-----------------------------------------------------------------------------
{
  uint8_t                             buffer[100];
  uint8_t                             size;

  T(T_ENB_RRC_SECURITY_MODE_COMMAND, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  size = do_SecurityModeCommand(
           ctxt_pP,
           buffer,
           rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id),
           ue_context_pP->ue_context.ciphering_algorithm,
           ue_context_pP->ue_context.integrity_algorithm);

#ifdef RRC_MSG_PRINT
  uint16_t i=0;
  LOG_F(RRC,"[MSG] RRC Security Mode Command\n");

  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }

  LOG_F(RRC,"\n");
#endif

  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel DL-DCCH, Generate SecurityModeCommand (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size);

  LOG_D(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" --- PDCP_DATA_REQ/%d Bytes (securityModeCommand to UE MUI %d) --->[PDCP][RB %02d]\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size,
        rrc_eNB_mui,
        DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" securityModeCommand UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
	       ctxt_pP,
	       DCCH,
	       rrc_eNB_mui++,
	       SDU_CONFIRM_NO,
	       size,
	       buffer,
	       PDCP_TRANSMISSION_MODE_CONTROL);

}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_UECapabilityEnquiry(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP
)
//-----------------------------------------------------------------------------
{

  uint8_t                             buffer[100];
  uint8_t                             size;

  T(T_ENB_RRC_UE_CAPABILITY_ENQUIRY, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  size = do_UECapabilityEnquiry(
           ctxt_pP,
           buffer,
           rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id));

  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel DL-DCCH, Generate UECapabilityEnquiry (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size);

  LOG_D(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" --- PDCP_DATA_REQ/%d Bytes (UECapabilityEnquiry MUI %d) --->[PDCP][RB %02d]\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size,
        rrc_eNB_mui,
        DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcUECapabilityEnquiry UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
	       ctxt_pP,
	       DCCH,
	       rrc_eNB_mui++,
	       SDU_CONFIRM_NO,
	       size,
	       buffer,
	       PDCP_TRANSMISSION_MODE_CONTROL);

}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_RRCConnectionReject(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  const int                    CC_id
)
//-----------------------------------------------------------------------------
{
#ifdef RRC_MSG_PRINT
  int                                 cnt;
#endif

  T(T_ENB_RRC_CONNECTION_REJECT, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size =
    do_RRCConnectionReject(ctxt_pP->module_id,
                          (uint8_t*) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload);

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRCConnectionReject\n");

  for (cnt = 0; cnt < RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size; cnt++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)RC.rrc[ctxt_pP->module_id]->Srb0.Tx_buffer.Payload)[cnt]);
  }

  LOG_F(RRC,"\n");
#endif

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Header,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size,
    MSC_AS_TIME_FMT" RRCConnectionReject UE %x size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP == NULL ? -1 : ue_context_pP->ue_context.rnti,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);

  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel DL-CCCH, Generating RRCConnectionReject (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);
}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_RRCConnectionReestablishment(
  const protocol_ctxt_t*         const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  const int                            CC_id
)
//-----------------------------------------------------------------------------
{
  LogicalChannelConfig_t             *SRB1_logicalChannelConfig;
  SRB_ToAddModList_t                 **SRB_configList;
  SRB_ToAddMod_t                     *SRB1_config;
  int                                 cnt;

  T(T_ENB_RRC_CONNECTION_REESTABLISHMENT, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  SRB_configList = &ue_context_pP->ue_context.SRB_configList;
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size =
    do_RRCConnectionReestablishment(ctxt_pP,
                                    ue_context_pP,
                                    CC_id,
                                    (uint8_t*) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload,
                                    (uint8_t) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].p_eNB, //at this point we do not have the UE capability information, so it can only be TM1 or TM2
                                    rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id),
                                    SRB_configList,
                                    &ue_context_pP->ue_context.physicalConfigDedicated);

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRCConnectionReestablishment\n");

  for (cnt = 0; cnt < RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size; cnt++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload)[cnt]);
  }

  LOG_F(RRC,"\n");
#endif

  // configure SRB1 for UE

  if (*SRB_configList != NULL) {
    for (cnt = 0; cnt < (*SRB_configList)->list.count; cnt++) {
      if ((*SRB_configList)->list.array[cnt]->srb_Identity == 1) {
        SRB1_config = (*SRB_configList)->list.array[cnt];

        if (SRB1_config->logicalChannelConfig) {
          if (SRB1_config->logicalChannelConfig->present ==
              SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
            SRB1_logicalChannelConfig = &SRB1_config->logicalChannelConfig->choice.explicitValue;
          } else {
            SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
          }
        } else {
          SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
        }

        LOG_D(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" RRC_eNB --- MAC_CONFIG_REQ  (SRB1) ---> MAC_eNB\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
        rrc_mac_config_req_eNB(ctxt_pP->module_id,
                           ue_context_pP->ue_context.primaryCC_id,
                           0,0,0,0,0,
#ifdef Rel14 
			 0,
#endif
                           ctxt_pP->rnti,
                           (BCCH_BCH_Message_t *) NULL, 
                           (RadioResourceConfigCommonSIB_t *) NULL,
#ifdef Rel14
                           (RadioResourceConfigCommonSIB_t *) NULL,
#endif
                           (struct PhysicalConfigDedicated* ) ue_context_pP->ue_context.physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
                           (SCellToAddMod_r10_t *)NULL,
                           //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
                           (MeasObjectToAddMod_t **) NULL,
                           ue_context_pP->ue_context.mac_MainConfig,
                           1,
                           SRB1_logicalChannelConfig,
                           ue_context_pP->ue_context.measGapConfig,
                           (TDD_Config_t *) NULL,
                           NULL,
                           (SchedulingInfoList_t *) NULL,
                           0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
#if defined(Rel10) || defined(Rel14)
                           , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#ifdef Rel14
                           ,(SystemInformationBlockType1_v1310_IEs_t *)NULL 
#endif
        );
        break;
      }
    }
  }

  MSC_LOG_TX_MESSAGE(MSC_RRC_ENB,
                     MSC_RRC_UE,
                     RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Header,
                     RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size,
                     MSC_AS_TIME_FMT" RRCConnectionReestablishment UE %x size %u",
                     MSC_AS_TIME_ARGS(ctxt_pP),
                     ue_context_pP->ue_context.rnti,
                     RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);


  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel DL-CCCH, Generating RRCConnectionReestablishment (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);

  // activate release timer, if RRCComplete not received after 10 frames, remove UE
  //ue_context_pP->ue_context.ue_release_timer = 1;
  // remove UE after 10 frames after RRCConnectionReestablishmentRelease is triggered
  //ue_context_pP->ue_context.ue_release_timer_thres = 100;
    // activate release timer, if RRCComplete not received after 100 frames, remove UE
  int UE_id = find_UE_id(ctxt_pP->module_id, ctxt_pP->rnti);
  RC.mac[ctxt_pP->module_id]->UE_list.UE_sched_ctrl[UE_id].ue_reestablishment_reject_timer = 1;
  // remove UE after 100 frames after RRCConnectionReestablishmentRelease is triggered
  RC.mac[ctxt_pP->module_id]->UE_list.UE_sched_ctrl[UE_id].ue_reestablishment_reject_timer_thres = 1000;
}

//-----------------------------------------------------------------------------
void
rrc_eNB_process_RRCConnectionReestablishmentComplete(
  const protocol_ctxt_t* const ctxt_pP,
  const rnti_t reestablish_rnti,
  rrc_eNB_ue_context_t*         ue_context_pP,
  const uint8_t xid,
  RRCConnectionReestablishmentComplete_r8_IEs_t * rrcConnectionReestablishmentComplete
)
//-----------------------------------------------------------------------------
{
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel UL-DCCH, processing RRCConnectionReestablishmentComplete from UE (SRB1 Active)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));

  T(T_ENB_RRC_CONNECTION_REESTABLISHMENT_COMPLETE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  DRB_ToAddModList_t*                 DRB_configList = ue_context_pP->ue_context.DRB_configList;
  SRB_ToAddModList_t*                 SRB_configList = ue_context_pP->ue_context.SRB_configList;
  SRB_ToAddModList_t**                SRB_configList2 = NULL;
  DRB_ToAddModList_t**                DRB_configList2 = NULL;
  struct SRB_ToAddMod                *SRB2_config = NULL;
  struct DRB_ToAddMod                *DRB_config = NULL;
  int i = 0;
# if defined(ENABLE_USE_MME)
  int j = 0;
  hashtable_rc_t                      h_rc;
#endif
  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;
  MeasObjectToAddModList_t           *MeasObj_list                     = NULL;
  MeasObjectToAddMod_t               *MeasObj                          = NULL;
  ReportConfigToAddModList_t         *ReportConfig_list                = NULL;
  ReportConfigToAddMod_t             *ReportConfig_per, *ReportConfig_A1,
                                     *ReportConfig_A2, *ReportConfig_A3, *ReportConfig_A4, *ReportConfig_A5;
  MeasIdToAddModList_t               *MeasId_list                      = NULL;
  MeasIdToAddMod_t                   *MeasId0, *MeasId1, *MeasId2, *MeasId3, *MeasId4, *MeasId5;
  RSRP_Range_t                       *rsrp                             = NULL;
  struct MeasConfig__speedStatePars  *Sparams                          = NULL;
  QuantityConfig_t                   *quantityConfig                   = NULL;
  CellsToAddMod_t                    *CellToAdd                        = NULL;
  CellsToAddModList_t                *CellsToAddModList                = NULL;
  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList = NULL;
  DedicatedInfoNAS_t                 *dedicatedInfoNas                 = NULL;
  /* for no gcc warnings */
  (void)dedicatedInfoNas;
  C_RNTI_t                           *cba_RNTI                         = NULL;
  uint8_t next_xid = rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id);

  ue_context_pP->ue_context.Status = RRC_CONNECTED;

  SRB_configList2 = &ue_context_pP->ue_context.SRB_configList2[xid];
  // get old configuration of SRB2
  if (*SRB_configList2 != NULL) {
    LOG_D(RRC, "SRB_configList2(%p) count is %d\n           SRB_configList2->list.array[0] addr is %p",
          SRB_configList2, (*SRB_configList2)->list.count,  (*SRB_configList2)->list.array[0]);
    for (i = 0; (i < (*SRB_configList2)->list.count) && (i < 3); i++) {
      if ((*SRB_configList2)->list.array[i]->srb_Identity == 2 ){
        LOG_D(RRC, "get SRB2_config from (ue_context_pP->ue_context.SRB_configList2[%d])\n", xid);
        SRB2_config = (*SRB_configList2)->list.array[i];
        break;
      }
    }
  }
  SRB_configList2 = &ue_context_pP->ue_context.SRB_configList2[next_xid];
  DRB_configList2 = &ue_context_pP->ue_context.DRB_configList2[next_xid];

  if (*SRB_configList2) {
    free(*SRB_configList2);
    LOG_D(RRC, "free(ue_context_pP->ue_context.SRB_configList2[%d])\n", next_xid);
  }
  *SRB_configList2 = CALLOC(1, sizeof(**SRB_configList2));
  if (SRB2_config != NULL) {
    // Add SRB2 to SRB configuration list

    ASN_SEQUENCE_ADD(&SRB_configList->list, SRB2_config);
    ASN_SEQUENCE_ADD(&(*SRB_configList2)->list, SRB2_config);

    LOG_D(RRC, "Add SRB2_config (srb_Identity:%ld) to ue_context_pP->ue_context.SRB_configList\n",
            SRB2_config->srb_Identity);
    LOG_D(RRC, "Add SRB2_config (srb_Identity:%ld) to ue_context_pP->ue_context.SRB_configList2[%d]\n",
                SRB2_config->srb_Identity, next_xid);
  } else {
    // SRB configuration list only contains SRB1.
    LOG_W(RRC,"SRB2 configuration does not exist in SRB configuration list\n");
  }



  if (*DRB_configList2) {
    free(*DRB_configList2);
    LOG_D(RRC, "free(ue_context_pP->ue_context.DRB_configList2[%d])\n", next_xid);
  }
  *DRB_configList2 = CALLOC(1, sizeof(**DRB_configList2));

  if (DRB_configList != NULL) {
    LOG_D(RRC, "get DRB_config from (ue_context_pP->ue_context.DRB_configList)\n");
    for (i = 0; (i < DRB_configList->list.count) && (i < 3); i++) {
      DRB_config = DRB_configList->list.array[i];

      // Add DRB to DRB configuration list, for RRCConnectionReconfigurationComplete
      ASN_SEQUENCE_ADD(&(*DRB_configList2)->list, DRB_config);
    }
  }
  ue_context_pP->ue_context.Srb1.Active = 1;
  //ue_context_pP->ue_context.Srb2.Srb_info.Srb_id = 2;

# if defined(ENABLE_USE_MME)
  rrc_ue_s1ap_ids_t* rrc_ue_s1ap_ids_p = NULL;
  uint16_t ue_initial_id = ue_context_pP->ue_context.ue_initial_id;
  uint32_t eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;
  eNB_RRC_INST *rrc_instance_p = RC.rrc[ENB_INSTANCE_TO_MODULE_ID(ctxt_pP->instance)];
  if (eNB_ue_s1ap_id > 0) {
    h_rc = hashtable_get(rrc_instance_p->s1ap_id2_s1ap_ids, (hash_key_t)eNB_ue_s1ap_id, (void**)&rrc_ue_s1ap_ids_p);
    if  (h_rc == HASH_TABLE_OK) {
      rrc_ue_s1ap_ids_p->ue_rnti = ctxt_pP->rnti;
    }
  }
  if (ue_initial_id != 0) {
    h_rc = hashtable_get(rrc_instance_p->initial_id2_s1ap_ids, (hash_key_t)ue_initial_id, (void**)&rrc_ue_s1ap_ids_p);
    if  (h_rc == HASH_TABLE_OK) {
      rrc_ue_s1ap_ids_p->ue_rnti = ctxt_pP->rnti;
    }
  }

  gtpv1u_enb_create_tunnel_req_t  create_tunnel_req;

  /* Save e RAB information for later */
  memset(&create_tunnel_req, 0 , sizeof(create_tunnel_req));

  for (j = 0, i = 0; i < NB_RB_MAX; i++) {
    if (ue_context_pP->ue_context.e_rab[i].status == E_RAB_STATUS_ESTABLISHED) {
      create_tunnel_req.eps_bearer_id[j]       = ue_context_pP->ue_context.e_rab[i].param.e_rab_id;
      create_tunnel_req.sgw_S1u_teid[j]        = ue_context_pP->ue_context.e_rab[i].param.gtp_teid;

      memcpy(&create_tunnel_req.sgw_addr[j],
             &ue_context_pP->ue_context.e_rab[i].param.sgw_addr,
             sizeof(transport_layer_addr_t));
      j++;
    }
  }

  create_tunnel_req.rnti       = ctxt_pP->rnti; // warning put zero above
  create_tunnel_req.num_tunnels    = j;

  gtpv1u_update_s1u_tunnel(
            ctxt_pP->instance,
            &create_tunnel_req,
            reestablish_rnti);
#endif
  /* Update RNTI in ue_context */
  ue_context_pP->ue_id_rnti                    = ctxt_pP->rnti; // here ue_id_rnti is just a key, may be something else
  ue_context_pP->ue_context.rnti               = ctxt_pP->rnti;
# if defined(ENABLE_USE_MME)
  uint8_t send_security_mode_command = FALSE;
  rrc_pdcp_config_security(
      ctxt_pP,
      ue_context_pP,
      send_security_mode_command);
  LOG_D(RRC, "set security successfully \n");
#endif
  // Measurement ID list
  MeasId_list = CALLOC(1, sizeof(*MeasId_list));
  memset((void *)MeasId_list, 0, sizeof(*MeasId_list));

  MeasId0 = CALLOC(1, sizeof(*MeasId0));
  MeasId0->measId = 1;
  MeasId0->measObjectId = 1;
  MeasId0->reportConfigId = 1;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId0);

  MeasId1 = CALLOC(1, sizeof(*MeasId1));
  MeasId1->measId = 2;
  MeasId1->measObjectId = 1;
  MeasId1->reportConfigId = 2;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId1);

  MeasId2 = CALLOC(1, sizeof(*MeasId2));
  MeasId2->measId = 3;
  MeasId2->measObjectId = 1;
  MeasId2->reportConfigId = 3;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId2);

  MeasId3 = CALLOC(1, sizeof(*MeasId3));
  MeasId3->measId = 4;
  MeasId3->measObjectId = 1;
  MeasId3->reportConfigId = 4;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId3);

  MeasId4 = CALLOC(1, sizeof(*MeasId4));
  MeasId4->measId = 5;
  MeasId4->measObjectId = 1;
  MeasId4->reportConfigId = 5;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId4);

  MeasId5 = CALLOC(1, sizeof(*MeasId5));
  MeasId5->measId = 6;
  MeasId5->measObjectId = 1;
  MeasId5->reportConfigId = 6;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId5);

  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measIdToAddModList = MeasId_list;

  // Add one EUTRA Measurement Object
  MeasObj_list = CALLOC(1, sizeof(*MeasObj_list));
  memset((void *)MeasObj_list, 0, sizeof(*MeasObj_list));

  // Configure MeasObject

  MeasObj = CALLOC(1, sizeof(*MeasObj));
  memset((void *)MeasObj, 0, sizeof(*MeasObj));

  MeasObj->measObjectId = 1;
  MeasObj->measObject.present = MeasObjectToAddMod__measObject_PR_measObjectEUTRA;
  MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 3350; //band 7, 2.68GHz
  //MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 36090; //band 33, 1.909GHz
  MeasObj->measObject.choice.measObjectEUTRA.allowedMeasBandwidth = AllowedMeasBandwidth_mbw25;
  MeasObj->measObject.choice.measObjectEUTRA.presenceAntennaPort1 = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf = CALLOC(1, sizeof(uint8_t));
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf[0] = 0;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.size = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.bits_unused = 6;
  MeasObj->measObject.choice.measObjectEUTRA.offsetFreq = NULL;   // Default is 15 or 0dB

  MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList =
    (CellsToAddModList_t *) CALLOC(1, sizeof(*CellsToAddModList));

  CellsToAddModList = MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList;

  // Add adjacent cell lists (6 per eNB)
  for (i = 0; i < 6; i++) {
    CellToAdd = (CellsToAddMod_t *) CALLOC(1, sizeof(*CellToAdd));
    CellToAdd->cellIndex = i + 1;
    CellToAdd->physCellId = get_adjacent_cell_id(ctxt_pP->module_id, i);
    CellToAdd->cellIndividualOffset = Q_OffsetRange_dB0;

    ASN_SEQUENCE_ADD(&CellsToAddModList->list, CellToAdd);
  }

  ASN_SEQUENCE_ADD(&MeasObj_list->list, MeasObj);
  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measObjectToAddModList = MeasObj_list;

  // Report Configurations for periodical, A1-A5 events
  ReportConfig_list = CALLOC(1, sizeof(*ReportConfig_list));

  ReportConfig_per = CALLOC(1, sizeof(*ReportConfig_per));

  ReportConfig_A1 = CALLOC(1, sizeof(*ReportConfig_A1));

  ReportConfig_A2 = CALLOC(1, sizeof(*ReportConfig_A2));

  ReportConfig_A3 = CALLOC(1, sizeof(*ReportConfig_A3));

  ReportConfig_A4 = CALLOC(1, sizeof(*ReportConfig_A4));

  ReportConfig_A5 = CALLOC(1, sizeof(*ReportConfig_A5));

  ReportConfig_per->reportConfigId = 1;
  ReportConfig_per->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_periodical;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.choice.periodical.purpose =
    ReportConfigEUTRA__triggerType__periodical__purpose_reportStrongestCells;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_per);

  ReportConfig_A1->reportConfigId = 2;
  ReportConfig_A1->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA1;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA1.
  a1_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA1.
  a1_Threshold.choice.threshold_RSRP = 10;

  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A1);

  if (RC.rrc[ctxt_pP->module_id]->HO_flag == 1 /*HO_MEASURMENT */ ) {
    LOG_I(RRC, "[eNB %d] frame %d: requesting A2, A3, A4, A5, and A6 event reporting\n",
          ctxt_pP->module_id, ctxt_pP->frame);
    ReportConfig_A2->reportConfigId = 3;
    ReportConfig_A2->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA2;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA2.a2_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA2.a2_Threshold.choice.threshold_RSRP = 10;

    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A2);

    ReportConfig_A3->reportConfigId = 4;
    ReportConfig_A3->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA3;

    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA3.a3_Offset = 1;   //10;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA3.reportOnLeave = 1;

    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.hysteresis = 0.5; // FIXME ...hysteresis is of type long!
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.timeToTrigger =
      TimeToTrigger_ms40;
    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A3);

    ReportConfig_A4->reportConfigId = 5;
    ReportConfig_A4->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA4;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA4.a4_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA4.a4_Threshold.choice.threshold_RSRP = 10;

    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A4);

    ReportConfig_A5->reportConfigId = 6;
    ReportConfig_A5->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA5;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold1.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold2.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold1.choice.threshold_RSRP = 10;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold2.choice.threshold_RSRP = 10;

    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A5);
    //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->reportConfigToAddModList = ReportConfig_list;

    rsrp = CALLOC(1, sizeof(RSRP_Range_t));
    *rsrp = 20;

    Sparams = CALLOC(1, sizeof(*Sparams));
    Sparams->present = MeasConfig__speedStatePars_PR_setup;
    Sparams->choice.setup.timeToTrigger_SF.sf_High = SpeedStateScaleFactors__sf_Medium_oDot75;
    Sparams->choice.setup.timeToTrigger_SF.sf_Medium = SpeedStateScaleFactors__sf_High_oDot5;
    Sparams->choice.setup.mobilityStateParameters.n_CellChangeHigh = 10;
    Sparams->choice.setup.mobilityStateParameters.n_CellChangeMedium = 5;
    Sparams->choice.setup.mobilityStateParameters.t_Evaluation = MobilityStateParameters__t_Evaluation_s60;
    Sparams->choice.setup.mobilityStateParameters.t_HystNormal = MobilityStateParameters__t_HystNormal_s120;

    quantityConfig = CALLOC(1, sizeof(*quantityConfig));
    memset((void *)quantityConfig, 0, sizeof(*quantityConfig));
    quantityConfig->quantityConfigEUTRA = CALLOC(1, sizeof(struct QuantityConfigEUTRA));
    memset((void *)quantityConfig->quantityConfigEUTRA, 0, sizeof(*quantityConfig->quantityConfigEUTRA));
    quantityConfig->quantityConfigCDMA2000 = NULL;
    quantityConfig->quantityConfigGERAN = NULL;
    quantityConfig->quantityConfigUTRA = NULL;
    quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP =
      CALLOC(1, sizeof(*(quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP)));
    quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ =
      CALLOC(1, sizeof(*(quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ)));
    *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP = FilterCoefficient_fc4;
    *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ = FilterCoefficient_fc4;

    LOG_I(RRC,
          "[eNB %d] Frame %d: potential handover preparation: store the information in an intermediate structure in case of failure\n",
          ctxt_pP->module_id, ctxt_pP->frame);
    // store the information in an intermediate structure for Hanodver management
    //rrc_inst->handover_info.as_config.sourceRadioResourceConfig.srb_ToAddModList = CALLOC(1,sizeof());
    ue_context_pP->ue_context.handover_info = CALLOC(1, sizeof(*(ue_context_pP->ue_context.handover_info)));
    //memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.srb_ToAddModList,(void *)SRB_list,sizeof(SRB_ToAddModList_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.srb_ToAddModList = *SRB_configList2;
    //memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.drb_ToAddModList,(void *)DRB_list,sizeof(DRB_ToAddModList_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.drb_ToAddModList = DRB_configList;
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.drb_ToReleaseList = NULL;
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig =
      CALLOC(1, sizeof(*ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig));
    memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig,
           (void *)ue_context_pP->ue_context.mac_MainConfig, sizeof(MAC_MainConfig_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.physicalConfigDedicated =
      CALLOC(1, sizeof(PhysicalConfigDedicated_t));
    memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.physicalConfigDedicated,
           (void*)ue_context_pP->ue_context.physicalConfigDedicated, sizeof(PhysicalConfigDedicated_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.sps_Config = NULL;
    //memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.sps_Config,(void *)rrc_inst->sps_Config[ue_mod_idP],sizeof(SPS_Config_t));

  }

#ifdef CBA
  //struct PUSCH_CBAConfigDedicated_vlola  *pusch_CBAConfigDedicated_vlola;
  uint8_t                            *cba_RNTI_buf;
  cba_RNTI = CALLOC(1, sizeof(C_RNTI_t));
  cba_RNTI_buf = CALLOC(1, 2 * sizeof(uint8_t));
  cba_RNTI->buf = cba_RNTI_buf;
  cba_RNTI->size = 2;
  cba_RNTI->bits_unused = 0;

  // associate UEs to the CBa groups as a function of their UE id
  if (rrc_inst->num_active_cba_groups) {
    cba_RNTI->buf[0] = rrc_inst->cba_rnti[ue_mod_idP % rrc_inst->num_active_cba_groups] & 0xff;
    cba_RNTI->buf[1] = 0xff;
    LOG_D(RRC,
          "[eNB %d] Frame %d: cba_RNTI = %x in group %d is attribued to UE %d\n",
          enb_mod_idP, frameP,
          rrc_inst->cba_rnti[ue_mod_idP % rrc_inst->num_active_cba_groups],
          ue_mod_idP % rrc_inst->num_active_cba_groups, ue_mod_idP);
  } else {
    cba_RNTI->buf[0] = 0x0;
    cba_RNTI->buf[1] = 0x0;
    LOG_D(RRC, "[eNB %d] Frame %d: no cba_RNTI is configured for UE %d\n", enb_mod_idP, frameP, ue_mod_idP);
  }

#endif

#if defined(ENABLE_ITTI)
  /* Initialize NAS list */
  dedicatedInfoNASList = CALLOC(1, sizeof(struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList));

  /* Add all NAS PDUs to the list */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      dedicatedInfoNas = CALLOC(1, sizeof(DedicatedInfoNAS_t));
      memset(dedicatedInfoNas, 0, sizeof(OCTET_STRING_t));
      OCTET_STRING_fromBuf(dedicatedInfoNas,
         (char*)ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer,
                           ue_context_pP->ue_context.e_rab[i].param.nas_pdu.length);
      LOG_D(RRC, "Add dedicatedInfoNas(%d) to dedicatedInfoNASList\n", i);
      ASN_SEQUENCE_ADD(&dedicatedInfoNASList->list, dedicatedInfoNas);
    }

    /* TODO parameters yet to process ... */
    {
      //      ue_context_pP->ue_context.e_rab[i].param.qos;
      //      ue_context_pP->ue_context.e_rab[i].param.sgw_addr;
      //      ue_context_pP->ue_context.e_rab[i].param.gtp_teid;
    }

    /* TODO should test if e RAB are Ok before! */
    ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_DONE;
    LOG_D(RRC, "setting the status for the default DRB (index %d) to (%d,%s)\n",
    i, ue_context_pP->ue_context.e_rab[i].status, "E_RAB_STATUS_DONE");
  }

  /* If list is empty free the list and reset the address */
  if (dedicatedInfoNASList->list.count == 0) {
    free(dedicatedInfoNASList);
    dedicatedInfoNASList = NULL;
  }

#endif

  // send RRCConnectionReconfiguration
  memset(buffer, 0, RRC_BUF_SIZE);

  size = do_RRCConnectionReconfiguration(ctxt_pP,
                                         buffer,
                                         next_xid,   //Transaction_id,
                                         (SRB_ToAddModList_t*)*SRB_configList2, // SRB_configList
                                         (DRB_ToAddModList_t*)DRB_configList,
                                         (DRB_ToReleaseList_t*)NULL,  // DRB2_list,
                                         (struct SPS_Config*)NULL,    // maybe ue_context_pP->ue_context.sps_Config,
                                         (struct PhysicalConfigDedicated*)ue_context_pP->ue_context.physicalConfigDedicated,
#ifdef EXMIMO_IOT
                                         NULL, NULL, NULL,NULL,
#else
                                         (MeasObjectToAddModList_t*)MeasObj_list,  // MeasObj_list,
                                         (ReportConfigToAddModList_t*)ReportConfig_list,  // ReportConfig_list,
                                         (QuantityConfig_t*)quantityConfig,  //quantityConfig,
                                         (MeasIdToAddModList_t*)NULL,
#endif
                                         (MAC_MainConfig_t*)ue_context_pP->ue_context.mac_MainConfig,
                                         (MeasGapConfig_t*)NULL,
                                         (MobilityControlInfo_t*)NULL,
                                         (struct MeasConfig__speedStatePars*)Sparams, // Sparams,
                                         (RSRP_Range_t*)rsrp, // rsrp,
                                         (C_RNTI_t*)cba_RNTI,  // cba_RNTI
                                         (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)dedicatedInfoNASList //dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
                                         , (SCellToAddMod_r10_t*)NULL
#endif
                                        );

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Reconfiguration\n");
  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }
  LOG_F(RRC,"\n");
  ////////////////////////////////////////
#endif

#if defined(ENABLE_ITTI)

  /* Free all NAS PDUs */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      /* Free the NAS PDU buffer and invalidate it */
      free(ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer);
      ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer = NULL;
    }
  }

#endif

  LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE id %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
         ctxt_pP,
         DCCH,
         rrc_eNB_mui++,
         SDU_CONFIRM_NO,
         size,
         buffer,
         PDCP_TRANSMISSION_MODE_CONTROL);

  // delete UE data of prior RNTI.  UE use current RNTI.
  protocol_ctxt_t ctxt_prior = *ctxt_pP;
  ctxt_prior.rnti = reestablish_rnti;

  LTE_eNB_ULSCH_t *ulsch = NULL;
  nfapi_ul_config_request_body_t *ul_req_tmp = NULL;
  PHY_VARS_eNB *eNB_PHY = NULL;
  eNB_MAC_INST *eNB_MAC = RC.mac[ctxt_prior.module_id];
  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    eNB_PHY = RC.eNB[ctxt_prior.module_id][CC_id];
    for (int i=0; i<NUMBER_OF_UE_MAX; i++) {
      ulsch = eNB_PHY->ulsch[i];
      if((ulsch != NULL) && (ulsch->rnti == ctxt_prior.rnti)){
        LOG_I(RRC, "clean_eNb_ulsch UE %x \n", ctxt_prior.rnti);
        clean_eNb_ulsch(ulsch);
        break;
      }
    }

    for(int j = 0; j < 10; j++){
      ul_req_tmp = &eNB_MAC->UL_req_tmp[CC_id][j].ul_config_request_body;
      if(ul_req_tmp){
        int pdu_number = ul_req_tmp->number_of_pdus;
        for(int pdu_index = pdu_number-1; pdu_index >= 0; pdu_index--){
          if(ul_req_tmp->ul_config_pdu_list[pdu_index].ulsch_pdu.ulsch_pdu_rel8.rnti == ctxt_prior.rnti){
            LOG_I(RRC, "remove UE %x from ul_config_pdu_list %d/%d\n", ctxt_prior.rnti, pdu_index, pdu_number);
            if(pdu_index < pdu_number -1){
               memcpy(&ul_req_tmp->ul_config_pdu_list[pdu_index], &ul_req_tmp->ul_config_pdu_list[pdu_index+1], (pdu_number-1-pdu_index) * sizeof(nfapi_ul_config_request_pdu_t));
            }
            ul_req_tmp->number_of_pdus--;
          }
        }
      }
    }
  }
  rrc_mac_remove_ue(ctxt_prior.module_id, ctxt_prior.rnti);
  rrc_rlc_remove_ue(&ctxt_prior);
  pdcp_remove_UE(&ctxt_prior);
}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_RRCConnectionReestablishmentReject(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  const int                    CC_id
)
//-----------------------------------------------------------------------------
{
#ifdef RRC_MSG_PRINT
  int                                 cnt;
#endif
  int UE_id = find_UE_id(ctxt_pP->module_id, ctxt_pP->rnti);
  RC.mac[ctxt_pP->module_id]->UE_list.UE_sched_ctrl[UE_id].ue_reestablishment_reject_timer = 1;
  RC.mac[ctxt_pP->module_id]->UE_list.UE_sched_ctrl[UE_id].ue_reestablishment_reject_timer_thres = 20;

  T(T_ENB_RRC_CONNECTION_REESTABLISHMENT_REJECT, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size =
    do_RRCConnectionReestablishmentReject(ctxt_pP->module_id,
                          (uint8_t*) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload);

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRCConnectionReestablishmentReject\n");

  for (cnt = 0; cnt < RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size; cnt++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload)[cnt]);
  }

  LOG_F(RRC,"\n");
#endif

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Header,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size,
    MSC_AS_TIME_FMT" RRCConnectionReestablishmentReject UE %x size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP == NULL ? -1 : ue_context_pP->ue_context.rnti,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);

  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel DL-CCCH, Generating RRCConnectionReestablishmentReject (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);
}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_RRCConnectionRelease(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP
)
//-----------------------------------------------------------------------------
{

  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;

  T(T_ENB_RRC_CONNECTION_RELEASE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  memset(buffer, 0, RRC_BUF_SIZE);

  size = do_RRCConnectionRelease(ctxt_pP->module_id, buffer,rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id));
  // set release timer
  //ue_context_pP->ue_context.ue_release_timer=1;
  // remove UE after 10 frames after RRCConnectionRelease is triggered
  //ue_context_pP->ue_context.ue_release_timer_thres=100;
    // set release timer
  ue_context_pP->ue_context.ue_release_timer_rrc = 1;
  // remove UE after 10 frames after RRCConnectionRelease is triggered
  ue_context_pP->ue_context.ue_release_timer_thres_rrc = 100;
  ue_context_pP->ue_context.ue_reestablishment_timer = 0;
  ue_context_pP->ue_context.ue_release_timer = 0;
  ue_context_pP->ue_context.ue_release_timer_s1 = 0;
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel DL-DCCH, Generate RRCConnectionRelease (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size);

  LOG_D(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" --- PDCP_DATA_REQ/%d Bytes (rrcConnectionRelease MUI %d) --->[PDCP][RB %u]\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size,
        rrc_eNB_mui,
        DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcConnectionRelease UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
	       ctxt_pP,
	       DCCH,
	       rrc_eNB_mui++,
	       SDU_CONFIRM_NO,
	       size,
	       buffer,
	       PDCP_TRANSMISSION_MODE_CONTROL);
}

uint8_t qci_to_priority[9]={2,4,3,5,1,6,7,8,9};

// TBD: this directive can be remived if we create a similar e_rab_param_t structure in RRC context
#if defined(ENABLE_ITTI) 
//-----------------------------------------------------------------------------
void
rrc_eNB_generate_dedicatedRRCConnectionReconfiguration(const protocol_ctxt_t* const ctxt_pP,
						     rrc_eNB_ue_context_t*          const ue_context_pP,
						     const uint8_t                ho_state
						     )
//-----------------------------------------------------------------------------
{
  
  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;
  int i;
  
  struct DRB_ToAddMod                *DRB_config                       = NULL;
  struct RLC_Config                  *DRB_rlc_config                   = NULL;
  struct PDCP_Config                 *DRB_pdcp_config                  = NULL;
  struct PDCP_Config__rlc_AM         *PDCP_rlc_AM                      = NULL;
  struct PDCP_Config__rlc_UM         *PDCP_rlc_UM                      = NULL;
  struct LogicalChannelConfig        *DRB_lchan_config                 = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters
    *DRB_ul_SpecificParameters        = NULL;
  //  DRB_ToAddModList_t**                DRB_configList=&ue_context_pP->ue_context.DRB_configList; 
  DRB_ToAddModList_t*                DRB_configList=ue_context_pP->ue_context.DRB_configList; 
  DRB_ToAddModList_t**                DRB_configList2=NULL;
  //DRB_ToAddModList_t**                RRC_DRB_configList=&ue_context_pP->ue_context.DRB_configList;

  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList = NULL;
  DedicatedInfoNAS_t                 *dedicatedInfoNas                 = NULL;
  /* for no gcc warnings */
  (void)dedicatedInfoNas;

  long  *logicalchannelgroup_drb;
//  int drb_identity_index=0;

  uint8_t xid = rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id);   //Transaction_id,
  DRB_configList2=&ue_context_pP->ue_context.DRB_configList2[xid];
  if (*DRB_configList2) {
    free(*DRB_configList2);
  }
  //*DRB_configList = CALLOC(1, sizeof(*DRB_configList));
  *DRB_configList2 = CALLOC(1, sizeof(**DRB_configList2)); 
  /* Initialize NAS list */
  dedicatedInfoNASList = CALLOC(1, sizeof(struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList));

  int e_rab_done=0;
  
  for ( i = 0  ;
	i < ue_context_pP->ue_context.setup_e_rabs ;
	i++){

    if (e_rab_done >= ue_context_pP->ue_context.nb_of_e_rabs){
        break;
    }
    
    // bypass the new and already configured erabs
    if (ue_context_pP->ue_context.e_rab[i].status >= E_RAB_STATUS_DONE) {
//      drb_identity_index++;
      continue;
    }
        
    DRB_config = CALLOC(1, sizeof(*DRB_config));

    DRB_config->eps_BearerIdentity = CALLOC(1, sizeof(long));
    // allowed value 5..15, value : x+4
    *(DRB_config->eps_BearerIdentity) = ue_context_pP->ue_context.e_rab[i].param.e_rab_id;//+ 4; // especial case generation  

 //   DRB_config->drb_Identity =  1 + drb_identity_index + e_rab_done;// + i ;// (DRB_Identity_t) ue_context_pP->ue_context.e_rab[i].param.e_rab_id;
    // 1 + drb_identiy_index;  
    DRB_config->drb_Identity = i+1;

    DRB_config->logicalChannelIdentity = CALLOC(1, sizeof(long));
    *(DRB_config->logicalChannelIdentity) = DRB_config->drb_Identity + 2; //(long) (ue_context_pP->ue_context.e_rab[i].param.e_rab_id + 2); // value : x+2
    
    DRB_rlc_config = CALLOC(1, sizeof(*DRB_rlc_config));
    DRB_config->rlc_Config = DRB_rlc_config;

    DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
    DRB_config->pdcp_Config = DRB_pdcp_config;
    DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
    *DRB_pdcp_config->discardTimer = PDCP_Config__discardTimer_infinity;
    DRB_pdcp_config->rlc_AM = NULL;
    DRB_pdcp_config->rlc_UM = NULL;


    switch (ue_context_pP->ue_context.e_rab[i].param.qos.qci){
      /*
       * type: realtime data with medium packer error rate
       * action: swtich to RLC UM
       */
    case 1: // 100ms, 10^-2, p2, GBR
    case 2: // 150ms, 10^-3, p4, GBR
    case 3: // 50ms, 10^-3, p3, GBR
    case 4:  // 300ms, 10^-6, p5 
    case 7: // 100ms, 10^-3, p7, GBR
    case 9: // 300ms, 10^-6, p9
    case 65: // 75ms, 10^-2, p0.7, mission critical voice, GBR
    case 66: // 100ms, 10^-2, p2, non-mission critical  voice , GBR
      // RLC 
      DRB_rlc_config->present = RLC_Config_PR_um_Bi_Directional;
      DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
      DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
      DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = T_Reordering_ms35;
      // PDCP
      PDCP_rlc_UM = CALLOC(1, sizeof(*PDCP_rlc_UM));
      DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
      PDCP_rlc_UM->pdcp_SN_Size = PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
      break;
      
      /*
       * type: non-realtime data with low packer error rate
       * action: swtich to RLC AM
       */
    case 5:  // 100ms, 10^-6, p1 , IMS signaling 
    case 6:  // 300ms, 10^-6, p6 
    case 8: // 300ms, 10^-6, p8 
    case 69: // 60ms, 10^-6, p0.5, mission critical delay sensitive data, Lowest Priority 
    case 70: // 200ms, 10^-6, p5.5, mision critical data 
      // RLC
       DRB_rlc_config->present = RLC_Config_PR_am;
       DRB_rlc_config->choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
       DRB_rlc_config->choice.am.ul_AM_RLC.pollPDU = PollPDU_p16;
       DRB_rlc_config->choice.am.ul_AM_RLC.pollByte = PollByte_kBinfinity;
       DRB_rlc_config->choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t8;
       DRB_rlc_config->choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
       DRB_rlc_config->choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms25;

       // PDCP
       PDCP_rlc_AM = CALLOC(1, sizeof(*PDCP_rlc_AM));
       DRB_pdcp_config->rlc_AM = PDCP_rlc_AM;
       PDCP_rlc_AM->statusReportRequired = FALSE;
       
       break;
    default :
      LOG_E(RRC,"not supported qci %d\n", ue_context_pP->ue_context.e_rab[i].param.qos.qci);
      ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_FAILED; 
      ue_context_pP->ue_context.e_rab[i].xid = xid;
      e_rab_done++;
      continue;
    }

    DRB_pdcp_config->headerCompression.present = PDCP_Config__headerCompression_PR_notUsed;
    
    DRB_lchan_config = CALLOC(1, sizeof(*DRB_lchan_config));
    DRB_config->logicalChannelConfig = DRB_lchan_config;
    DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
    DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;

    if (ue_context_pP->ue_context.e_rab[i].param.qos.qci < 9 )
      DRB_ul_SpecificParameters->priority = qci_to_priority[ue_context_pP->ue_context.e_rab[i].param.qos.qci-1] + 3; 
    // ue_context_pP->ue_context.e_rab[i].param.qos.allocation_retention_priority.priority_level;
    else 
      DRB_ul_SpecificParameters->priority= 4;

    DRB_ul_SpecificParameters->prioritisedBitRate = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8;
      //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    DRB_ul_SpecificParameters->bucketSizeDuration =
      LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
    
    logicalchannelgroup_drb = CALLOC(1, sizeof(long));
    *logicalchannelgroup_drb = 1;//(i+1) % 3;
    DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;

    ASN_SEQUENCE_ADD(&DRB_configList->list, DRB_config);
    ASN_SEQUENCE_ADD(&(*DRB_configList2)->list, DRB_config);
    //ue_context_pP->ue_context.DRB_configList2[drb_identity_index] = &(*DRB_configList);
    
    LOG_I(RRC,"EPS ID %ld, DRB ID %ld (index %d), QCI %d, priority %ld, LCID %ld LCGID %ld \n",
	  *DRB_config->eps_BearerIdentity,
	  DRB_config->drb_Identity, i,
	  ue_context_pP->ue_context.e_rab[i].param.qos.qci,
	  DRB_ul_SpecificParameters->priority,
	  *(DRB_config->logicalChannelIdentity),
	  *DRB_ul_SpecificParameters->logicalChannelGroup	  
	  );

    e_rab_done++;
    ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_DONE; 
    ue_context_pP->ue_context.e_rab[i].xid = xid;
    
    {
      if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
	dedicatedInfoNas = CALLOC(1, sizeof(DedicatedInfoNAS_t));
	memset(dedicatedInfoNas, 0, sizeof(OCTET_STRING_t));
	OCTET_STRING_fromBuf(dedicatedInfoNas, 
			     (char*)ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer,
			     ue_context_pP->ue_context.e_rab[i].param.nas_pdu.length);
	ASN_SEQUENCE_ADD(&dedicatedInfoNASList->list, dedicatedInfoNas);
	LOG_I(RRC,"add NAS info with size %d (rab id %d)\n",ue_context_pP->ue_context.e_rab[i].param.nas_pdu.length, i);
      } 
      else {
	LOG_W(RRC,"Not received activate dedicated EPS bearer context request\n");
      }
      /* TODO parameters yet to process ... */
      {
	//      ue_context_pP->ue_context.e_rab[i].param.qos;
	//      ue_context_pP->ue_context.e_rab[i].param.sgw_addr;
	//      ue_context_pP->ue_context.e_rab[i].param.gtp_teid;
      }
    }
    
  }

  /* If list is empty free the list and reset the address */
  if (dedicatedInfoNASList != NULL) {
    if (dedicatedInfoNASList->list.count == 0) {
      free(dedicatedInfoNASList);
      dedicatedInfoNASList = NULL;
      LOG_W(RRC,"dedlicated NAS list is empty, free the list and reset the address\n");
    }				
  } else {
    LOG_W(RRC,"dedlicated NAS list is empty\n");
  }

  memset(buffer, 0, RRC_BUF_SIZE);

   size = do_RRCConnectionReconfiguration(ctxt_pP,
					  buffer,
					  xid,
					  (SRB_ToAddModList_t*)NULL, 
					  (DRB_ToAddModList_t*)*DRB_configList2,
					  (DRB_ToReleaseList_t*)NULL,  // DRB2_list,
                                         (struct SPS_Config*)NULL,    // *sps_Config,
					  NULL, NULL, NULL, NULL,NULL,
					  NULL, NULL,  NULL, NULL, NULL, NULL, 
					  (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
                                         , (SCellToAddMod_r10_t*)NULL
#endif
                                        );
 

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Reconfiguration\n");
  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }
  LOG_F(RRC,"\n");
  ////////////////////////////////////////
#endif

#if defined(ENABLE_ITTI)

  /* Free all NAS PDUs */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      /* Free the NAS PDU buffer and invalidate it */
      free(ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer);
      ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer = NULL;
    }
  }
#endif

 LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE RNTI %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" dedicated rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
    ctxt_pP,
    DCCH,
    rrc_eNB_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);


}
int
rrc_eNB_modify_dedicatedRRCConnectionReconfiguration(const protocol_ctxt_t* const ctxt_pP,
                             rrc_eNB_ue_context_t*          const ue_context_pP,
                             const uint8_t                ho_state
                             )
//-----------------------------------------------------------------------------
{
  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;
  int i, j;

  struct DRB_ToAddMod                *DRB_config                       = NULL;
  struct RLC_Config                  *DRB_rlc_config                   = NULL;
  struct PDCP_Config                 *DRB_pdcp_config                  = NULL;
  struct PDCP_Config__rlc_AM         *PDCP_rlc_AM                      = NULL;
  struct PDCP_Config__rlc_UM         *PDCP_rlc_UM                      = NULL;
  struct LogicalChannelConfig        *DRB_lchan_config                 = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters
  *DRB_ul_SpecificParameters        = NULL;
  DRB_ToAddModList_t*                 DRB_configList = ue_context_pP->ue_context.DRB_configList;
  DRB_ToAddModList_t*                DRB_configList2 = NULL;

  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList = NULL;
  DedicatedInfoNAS_t                 *dedicatedInfoNas                 = NULL;
  /* for no gcc warnings */
  (void)dedicatedInfoNas;

  uint8_t xid = rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id);   // Transaction_id,
  DRB_configList2 = CALLOC(1, sizeof(*DRB_configList2));
  /* Initialize NAS list */
  dedicatedInfoNASList = CALLOC(1, sizeof(struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList));

  for (i = 0; i < ue_context_pP->ue_context.nb_of_modify_e_rabs; i++) {
    // bypass the new and already configured erabs
    if (ue_context_pP->ue_context.modify_e_rab[i].status >= E_RAB_STATUS_DONE) {
      ue_context_pP->ue_context.modify_e_rab[i].xid = xid;
      continue;
    }

    if (ue_context_pP->ue_context.modify_e_rab[i].cause != S1AP_CAUSE_NOTHING) {
      // set xid of failure RAB
      ue_context_pP->ue_context.modify_e_rab[i].xid = xid;
      ue_context_pP->ue_context.modify_e_rab[i].status = E_RAB_STATUS_FAILED;
      continue;
    }

    DRB_config = NULL;
    // search exist DRB_config
    for (j = 0; j < DRB_configList->list.count; j++) {
      if((uint8_t)*(DRB_configList->list.array[j]->eps_BearerIdentity) == ue_context_pP->ue_context.modify_e_rab[i].param.e_rab_id) {
        DRB_config = DRB_configList->list.array[j];
        break;
      }
    }
    if (NULL == DRB_config) {
      ue_context_pP->ue_context.modify_e_rab[i].xid = xid;
      ue_context_pP->ue_context.modify_e_rab[i].status = E_RAB_STATUS_FAILED;
      // TODO use which cause
      ue_context_pP->ue_context.modify_e_rab[i].cause = S1AP_CAUSE_RADIO_NETWORK;
      ue_context_pP->ue_context.modify_e_rab[i].cause_value = 0;//S1ap_CauseRadioNetwork_unspecified;
      ue_context_pP->ue_context.nb_of_failed_e_rabs++;
      continue;
    }

    DRB_rlc_config = DRB_config->rlc_Config;

    DRB_pdcp_config = DRB_config->pdcp_Config;
    *DRB_pdcp_config->discardTimer = PDCP_Config__discardTimer_infinity;
    switch (ue_context_pP->ue_context.modify_e_rab[i].param.qos.qci) {
    /*
     * type: realtime data with medium packer error rate
     * action: swtich to RLC UM
     */
    case 1: // 100ms, 10^-2, p2, GBR
    case 2: // 150ms, 10^-3, p4, GBR
    case 3: // 50ms, 10^-3, p3, GBR
    case 4:  // 300ms, 10^-6, p5
    case 7: // 100ms, 10^-3, p7, GBR
    case 9: // 300ms, 10^-6, p9
    case 65: // 75ms, 10^-2, p0.7, mission critical voice, GBR
    case 66: // 100ms, 10^-2, p2, non-mission critical  voice , GBR
      // RLC
      DRB_rlc_config->present = RLC_Config_PR_um_Bi_Directional;
      DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
      DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
      DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = T_Reordering_ms35;
      // PDCP
      if (DRB_pdcp_config->rlc_AM) {
        free(DRB_pdcp_config->rlc_AM);
        DRB_pdcp_config->rlc_AM = NULL;
      }
      if (DRB_pdcp_config->rlc_UM) {
        free(DRB_pdcp_config->rlc_UM);
        DRB_pdcp_config->rlc_UM = NULL;
      }
      PDCP_rlc_UM = CALLOC(1, sizeof(*PDCP_rlc_UM));
      DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
      PDCP_rlc_UM->pdcp_SN_Size = PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
      break;

    /*
     * type: non-realtime data with low packer error rate
     * action: swtich to RLC AM
     */
    case 5:  // 100ms, 10^-6, p1 , IMS signaling
    case 6:  // 300ms, 10^-6, p6
    case 8: // 300ms, 10^-6, p8
    case 69: // 60ms, 10^-6, p0.5, mission critical delay sensitive data, Lowest Priority
    case 70: // 200ms, 10^-6, p5.5, mision critical data
       // RLC
       DRB_rlc_config->present = RLC_Config_PR_am;
       DRB_rlc_config->choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
       DRB_rlc_config->choice.am.ul_AM_RLC.pollPDU = PollPDU_p16;
       DRB_rlc_config->choice.am.ul_AM_RLC.pollByte = PollByte_kBinfinity;
       DRB_rlc_config->choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t8;
       DRB_rlc_config->choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
       DRB_rlc_config->choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms25;

       // PDCP
       if (DRB_pdcp_config->rlc_AM) {
         free(DRB_pdcp_config->rlc_AM);
         DRB_pdcp_config->rlc_AM = NULL;
       }
       if (DRB_pdcp_config->rlc_UM) {
         free(DRB_pdcp_config->rlc_UM);
         DRB_pdcp_config->rlc_UM = NULL;
       }
       PDCP_rlc_AM = CALLOC(1, sizeof(*PDCP_rlc_AM));
       DRB_pdcp_config->rlc_AM = PDCP_rlc_AM;
       PDCP_rlc_AM->statusReportRequired = FALSE;

       break;
    default :
      LOG_E(RRC, "not supported qci %d\n", ue_context_pP->ue_context.modify_e_rab[i].param.qos.qci);
      ue_context_pP->ue_context.modify_e_rab[i].status = E_RAB_STATUS_FAILED;
      ue_context_pP->ue_context.modify_e_rab[i].xid = xid;
      ue_context_pP->ue_context.modify_e_rab[i].cause = S1AP_CAUSE_RADIO_NETWORK;
      ue_context_pP->ue_context.modify_e_rab[i].cause_value = 37;//S1ap_CauseRadioNetwork_not_supported_QCI_value;
      ue_context_pP->ue_context.nb_of_failed_e_rabs++;
      continue;
    }

    DRB_pdcp_config->headerCompression.present = PDCP_Config__headerCompression_PR_notUsed;

    DRB_lchan_config = DRB_config->logicalChannelConfig;
    DRB_ul_SpecificParameters = DRB_lchan_config->ul_SpecificParameters;

    if (ue_context_pP->ue_context.modify_e_rab[i].param.qos.qci < 9 )
      DRB_ul_SpecificParameters->priority = qci_to_priority[ue_context_pP->ue_context.modify_e_rab[i].param.qos.qci-1] + 3;
    else
      DRB_ul_SpecificParameters->priority= 4;

    DRB_ul_SpecificParameters->prioritisedBitRate = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8;

    DRB_ul_SpecificParameters->bucketSizeDuration =
      LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

    ASN_SEQUENCE_ADD(&(DRB_configList2)->list, DRB_config);

    LOG_I(RRC, "EPS ID %ld, DRB ID %ld (index %d), QCI %d, priority %ld, LCID %ld LCGID %ld \n",
      *DRB_config->eps_BearerIdentity,
      DRB_config->drb_Identity, i,
      ue_context_pP->ue_context.modify_e_rab[i].param.qos.qci,
      DRB_ul_SpecificParameters->priority,
      *(DRB_config->logicalChannelIdentity),
      *DRB_ul_SpecificParameters->logicalChannelGroup
      );

    //e_rab_done++;
    ue_context_pP->ue_context.modify_e_rab[i].status = E_RAB_STATUS_DONE;
    ue_context_pP->ue_context.modify_e_rab[i].xid = xid;

    {
      if (ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.buffer != NULL) {
        dedicatedInfoNas = CALLOC(1, sizeof(DedicatedInfoNAS_t));
        memset(dedicatedInfoNas, 0, sizeof(OCTET_STRING_t));
        OCTET_STRING_fromBuf(dedicatedInfoNas,
                 (char*)ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.buffer,
                 ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.length);
        ASN_SEQUENCE_ADD(&dedicatedInfoNASList->list, dedicatedInfoNas);
        LOG_I(RRC, "add NAS info with size %d (rab id %d)\n",ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.length, i);
      }
      else {
        LOG_W(RRC, "Not received activate dedicated EPS bearer context request\n");
      }
    }
  }

  /* If list is empty free the list and reset the address */
  if (dedicatedInfoNASList != NULL) {
    if (dedicatedInfoNASList->list.count == 0) {
      free(dedicatedInfoNASList);
      dedicatedInfoNASList = NULL;
      LOG_W(RRC,"dedlicated NAS list is empty, free the list and reset the address\n");
    }
  } else {
    LOG_W(RRC,"dedlicated NAS list is empty\n");
  }

  memset(buffer, 0, RRC_BUF_SIZE);

  size = do_RRCConnectionReconfiguration(ctxt_pP,
                                          buffer,
                                          xid,
                                          (SRB_ToAddModList_t*)NULL,
                                          (DRB_ToAddModList_t*)DRB_configList2,
                                          (DRB_ToReleaseList_t*)NULL,  // DRB2_list,
                                          (struct SPS_Config*)NULL,    // *sps_Config,
                                          NULL, NULL, NULL, NULL,NULL,
                                          NULL, NULL,  NULL, NULL, NULL, NULL,
                                          (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
                                          , (SCellToAddMod_r10_t*)NULL
#endif
                                          );


#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Reconfiguration\n");
  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }
  LOG_F(RRC,"\n");
  ////////////////////////////////////////
#endif

#if defined(ENABLE_ITTI)

  /* Free all NAS PDUs */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_modify_e_rabs; i++) {
    if (ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.buffer != NULL) {
      /* Free the NAS PDU buffer and invalidate it */
      free(ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.buffer);
      ue_context_pP->ue_context.modify_e_rab[i].param.nas_pdu.buffer = NULL;
    }
  }
#endif

 LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE RNTI %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" dedicated rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
    ctxt_pP,
    DCCH,
    rrc_eNB_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);
  return 0;
}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_dedicatedRRCConnectionReconfiguration_release(  const protocol_ctxt_t*   const ctxt_pP,
        rrc_eNB_ue_context_t*    const ue_context_pP,
        uint8_t                  xid,
        uint32_t                 nas_length,
        uint8_t*                 nas_buffer)
//-----------------------------------------------------------------------------
{
    uint8_t                             buffer[RRC_BUF_SIZE];
    int                                 i;
    uint16_t                            size  = 0;
    DRB_ToReleaseList_t**                DRB_Release_configList2=NULL;
    DRB_Identity_t* DRB_release;
    struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList = NULL;

    DRB_Release_configList2=&ue_context_pP->ue_context.DRB_Release_configList2[xid];
    if (*DRB_Release_configList2) {
      free(*DRB_Release_configList2);
    }
    *DRB_Release_configList2 = CALLOC(1, sizeof(**DRB_Release_configList2));

    for(i = 0; i < NB_RB_MAX; i++){
        if((ue_context_pP->ue_context.e_rab[i].status == E_RAB_STATUS_TORELEASE) && ue_context_pP->ue_context.e_rab[i].xid == xid){
            DRB_release = CALLOC(1, sizeof(DRB_Identity_t));
            *DRB_release = i+1;
            ASN_SEQUENCE_ADD(&(*DRB_Release_configList2)->list, DRB_release);
            //free(DRB_release);
        }
    }

    /* If list is empty free the list and reset the address */
    if (nas_length > 0) {
        DedicatedInfoNAS_t                 *dedicatedInfoNas                 = NULL;
        dedicatedInfoNASList = CALLOC(1, sizeof(struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList));
        dedicatedInfoNas = CALLOC(1, sizeof(DedicatedInfoNAS_t));
        memset(dedicatedInfoNas, 0, sizeof(OCTET_STRING_t));
                       OCTET_STRING_fromBuf(dedicatedInfoNas,
                              (char*)nas_buffer,
                              nas_length);
        ASN_SEQUENCE_ADD(&dedicatedInfoNASList->list, dedicatedInfoNas);
        LOG_I(RRC,"add NAS info with size %d\n",nas_length);
    } else {
      LOG_W(RRC,"dedlicated NAS list is empty\n");
    }

    memset(buffer, 0, RRC_BUF_SIZE);
    size = do_RRCConnectionReconfiguration(ctxt_pP,
                                    buffer,
                                    xid,
                                    NULL,
                                    NULL,
                                    (DRB_ToReleaseList_t*)*DRB_Release_configList2,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
                                    , (SCellToAddMod_r10_t*)NULL
#endif
                                   );
    ue_context_pP->ue_context.e_rab_release_command_flag = 1;

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Reconfiguration\n");
  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }
  LOG_F(RRC,"\n");
  ////////////////////////////////////////
#endif

#if defined(ENABLE_ITTI)
  /* Free all NAS PDUs */
  if (nas_length > 0) {
      /* Free the NAS PDU buffer and invalidate it */
      free(nas_buffer);
  }
#endif

  LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE RNTI %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" dedicated rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
    ctxt_pP,
    DCCH,
    rrc_eNB_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);

}
#endif 
//-----------------------------------------------------------------------------
void
rrc_eNB_generate_defaultRRCConnectionReconfiguration(const protocol_ctxt_t* const ctxt_pP,
						     rrc_eNB_ue_context_t*          const ue_context_pP,
						     const uint8_t                ho_state
						     )
//-----------------------------------------------------------------------------
{
  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;
  int                                 i;

  // configure SRB1/SRB2, PhysicalConfigDedicated, MAC_MainConfig for UE
  eNB_RRC_INST*                       rrc_inst = RC.rrc[ctxt_pP->module_id];
  struct PhysicalConfigDedicated**    physicalConfigDedicated = &ue_context_pP->ue_context.physicalConfigDedicated;

  struct SRB_ToAddMod                *SRB2_config                      = NULL;
  struct SRB_ToAddMod__rlc_Config    *SRB2_rlc_config                  = NULL;
  struct SRB_ToAddMod__logicalChannelConfig *SRB2_lchan_config         = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters
      *SRB2_ul_SpecificParameters       = NULL;
  SRB_ToAddModList_t*                 SRB_configList = ue_context_pP->ue_context.SRB_configList;
  SRB_ToAddModList_t                 **SRB_configList2                  = NULL;

  struct DRB_ToAddMod                *DRB_config                       = NULL;
  struct RLC_Config                  *DRB_rlc_config                   = NULL;
  struct PDCP_Config                 *DRB_pdcp_config                  = NULL;
  struct PDCP_Config__rlc_AM         *PDCP_rlc_AM                      = NULL;
  struct PDCP_Config__rlc_UM         *PDCP_rlc_UM                      = NULL;
  struct LogicalChannelConfig        *DRB_lchan_config                 = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters
      *DRB_ul_SpecificParameters        = NULL;
  DRB_ToAddModList_t**                DRB_configList = &ue_context_pP->ue_context.DRB_configList;
  DRB_ToAddModList_t**                DRB_configList2 = NULL;
   MAC_MainConfig_t                   *mac_MainConfig                   = NULL;
  MeasObjectToAddModList_t           *MeasObj_list                     = NULL;
  MeasObjectToAddMod_t               *MeasObj                          = NULL;
  ReportConfigToAddModList_t         *ReportConfig_list                = NULL;
  ReportConfigToAddMod_t             *ReportConfig_per, *ReportConfig_A1,
                                     *ReportConfig_A2, *ReportConfig_A3, *ReportConfig_A4, *ReportConfig_A5;
  MeasIdToAddModList_t               *MeasId_list                      = NULL;
  MeasIdToAddMod_t                   *MeasId0, *MeasId1, *MeasId2, *MeasId3, *MeasId4, *MeasId5;
#if defined(Rel10) || defined(Rel14)
  long                               *sr_ProhibitTimer_r9              = NULL;
  //     uint8_t sCellIndexToAdd = rrc_find_free_SCell_index(enb_mod_idP, ue_mod_idP, 1);
  //uint8_t                            sCellIndexToAdd = 0;
#endif

  long                               *logicalchannelgroup, *logicalchannelgroup_drb;
  long                               *maxHARQ_Tx, *periodicBSR_Timer;

  RSRP_Range_t                       *rsrp                             = NULL;
  struct MeasConfig__speedStatePars  *Sparams                          = NULL;
  QuantityConfig_t                   *quantityConfig                   = NULL;
  CellsToAddMod_t                    *CellToAdd                        = NULL;
  CellsToAddModList_t                *CellsToAddModList                = NULL;
  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList = NULL;
  DedicatedInfoNAS_t                 *dedicatedInfoNas                 = NULL;
  /* for no gcc warnings */
  (void)dedicatedInfoNas;

  C_RNTI_t                           *cba_RNTI                         = NULL;

  uint8_t xid = rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id);   //Transaction_id,

#ifdef CBA
  //struct PUSCH_CBAConfigDedicated_vlola  *pusch_CBAConfigDedicated_vlola;
  uint8_t                            *cba_RNTI_buf;
  cba_RNTI = CALLOC(1, sizeof(C_RNTI_t));
  cba_RNTI_buf = CALLOC(1, 2 * sizeof(uint8_t));
  cba_RNTI->buf = cba_RNTI_buf;
  cba_RNTI->size = 2;
  cba_RNTI->bits_unused = 0;

  // associate UEs to the CBa groups as a function of their UE id
  if (rrc_inst->num_active_cba_groups) {
    cba_RNTI->buf[0] = rrc_inst->cba_rnti[ue_mod_idP % rrc_inst->num_active_cba_groups] & 0xff;
    cba_RNTI->buf[1] = 0xff;
    LOG_D(RRC,
          "[eNB %d] Frame %d: cba_RNTI = %x in group %d is attribued to UE %d\n",
          enb_mod_idP, frameP,
          rrc_inst->cba_rnti[ue_mod_idP % rrc_inst->num_active_cba_groups],
          ue_mod_idP % rrc_inst->num_active_cba_groups, ue_mod_idP);
  } else {
    cba_RNTI->buf[0] = 0x0;
    cba_RNTI->buf[1] = 0x0;
    LOG_D(RRC, "[eNB %d] Frame %d: no cba_RNTI is configured for UE %d\n", enb_mod_idP, frameP, ue_mod_idP);
  }

#endif

  T(T_ENB_RRC_CONNECTION_RECONFIGURATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  // Configure SRB2
  /// SRB2
  SRB_configList2=&ue_context_pP->ue_context.SRB_configList2[xid];
  if (*SRB_configList2) {
    free(*SRB_configList2);
  }
  *SRB_configList2 = CALLOC(1, sizeof(**SRB_configList2));
  memset(*SRB_configList2, 0, sizeof(**SRB_configList2));
  SRB2_config = CALLOC(1, sizeof(*SRB2_config));

  SRB2_config->srb_Identity = 2;
  SRB2_rlc_config = CALLOC(1, sizeof(*SRB2_rlc_config));
  SRB2_config->rlc_Config = SRB2_rlc_config;

  SRB2_rlc_config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB2_rlc_config->choice.explicitValue.present = RLC_Config_PR_am;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms15;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU = PollPDU_p8;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte = PollByte_kB1000;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;
  SRB2_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
  SRB2_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms10;

  SRB2_lchan_config = CALLOC(1, sizeof(*SRB2_lchan_config));
  SRB2_config->logicalChannelConfig = SRB2_lchan_config;

  SRB2_lchan_config->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;

  SRB2_ul_SpecificParameters = CALLOC(1, sizeof(*SRB2_ul_SpecificParameters));

  SRB2_ul_SpecificParameters->priority = 3; // let some priority for SRB1 and dedicated DRBs
  SRB2_ul_SpecificParameters->prioritisedBitRate =
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  SRB2_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  // LCG for CCCH and DCCH is 0 as defined in 36331
  logicalchannelgroup = CALLOC(1, sizeof(long));
  *logicalchannelgroup = 0;

  SRB2_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;

  SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB2_ul_SpecificParameters;
  // this list has the configuration for SRB1 and SRB2
  ASN_SEQUENCE_ADD(&SRB_configList->list, SRB2_config);
  // this list has only the configuration for SRB2
  ASN_SEQUENCE_ADD(&(*SRB_configList2)->list, SRB2_config);

  // Configure DRB
  //*DRB_configList = CALLOC(1, sizeof(*DRB_configList));
  // list for all the configured DRB
  if (*DRB_configList) {
    free(*DRB_configList);
  }
  *DRB_configList = CALLOC(1, sizeof(**DRB_configList));
  memset(*DRB_configList, 0, sizeof(**DRB_configList));

  // list for the configured DRB for a this xid
  DRB_configList2=&ue_context_pP->ue_context.DRB_configList2[xid];
  if (*DRB_configList2) {
    free(*DRB_configList2);
  }
  *DRB_configList2 = CALLOC(1, sizeof(**DRB_configList2));
  memset(*DRB_configList2, 0, sizeof(**DRB_configList2));


  /// DRB
  DRB_config = CALLOC(1, sizeof(*DRB_config));

  DRB_config->eps_BearerIdentity = CALLOC(1, sizeof(long));
  *(DRB_config->eps_BearerIdentity) = 5L; // LW set to first value, allowed value 5..15, value : x+4
  // DRB_config->drb_Identity = (DRB_Identity_t) 1; //allowed values 1..32
  // NN: this is the 1st DRB for this ue, so set it to 1
  DRB_config->drb_Identity = (DRB_Identity_t) 1;  // (ue_mod_idP+1); //allowed values 1..32, value: x
  DRB_config->logicalChannelIdentity = CALLOC(1, sizeof(long));
  *(DRB_config->logicalChannelIdentity) = (long)3; // value : x+2
  DRB_rlc_config = CALLOC(1, sizeof(*DRB_rlc_config));
  DRB_config->rlc_Config = DRB_rlc_config;

#ifdef RRC_DEFAULT_RAB_IS_AM
  DRB_rlc_config->present = RLC_Config_PR_am;
  DRB_rlc_config->choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
  DRB_rlc_config->choice.am.ul_AM_RLC.pollPDU = PollPDU_p16;
  DRB_rlc_config->choice.am.ul_AM_RLC.pollByte = PollByte_kBinfinity;
  DRB_rlc_config->choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t8;
  DRB_rlc_config->choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
  DRB_rlc_config->choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms25;
#else
  DRB_rlc_config->present = RLC_Config_PR_um_Bi_Directional;
  DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
#ifdef CBA
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering   = T_Reordering_ms5;//T_Reordering_ms25;
#else
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = T_Reordering_ms35;
#endif
#endif

  DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
  DRB_config->pdcp_Config = DRB_pdcp_config;
  DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
  *DRB_pdcp_config->discardTimer = PDCP_Config__discardTimer_infinity;
  DRB_pdcp_config->rlc_AM = NULL;
  DRB_pdcp_config->rlc_UM = NULL;

  /* avoid gcc warnings */
  (void)PDCP_rlc_AM;
  (void)PDCP_rlc_UM;

#ifdef RRC_DEFAULT_RAB_IS_AM // EXMIMO_IOT
  PDCP_rlc_AM = CALLOC(1, sizeof(*PDCP_rlc_AM));
  DRB_pdcp_config->rlc_AM = PDCP_rlc_AM;
  PDCP_rlc_AM->statusReportRequired = FALSE;
#else
  PDCP_rlc_UM = CALLOC(1, sizeof(*PDCP_rlc_UM));
  DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
  PDCP_rlc_UM->pdcp_SN_Size = PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
#endif
  DRB_pdcp_config->headerCompression.present = PDCP_Config__headerCompression_PR_notUsed;

  DRB_lchan_config = CALLOC(1, sizeof(*DRB_lchan_config));
  DRB_config->logicalChannelConfig = DRB_lchan_config;
  DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
  DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;

  DRB_ul_SpecificParameters->priority = 12;    // lower priority than srb1, srb2 and other dedicated bearer
  DRB_ul_SpecificParameters->prioritisedBitRate =LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8 ;
    //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  DRB_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  // LCG for DTCH can take the value from 1 to 3 as defined in 36331: normally controlled by upper layers (like RRM)
  logicalchannelgroup_drb = CALLOC(1, sizeof(long));
  *logicalchannelgroup_drb = 1;
  DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;

  ASN_SEQUENCE_ADD(&(*DRB_configList)->list, DRB_config);
  ASN_SEQUENCE_ADD(&(*DRB_configList2)->list, DRB_config);

  //ue_context_pP->ue_context.DRB_configList2[0] = &(*DRB_configList);

  mac_MainConfig = CALLOC(1, sizeof(*mac_MainConfig));
  ue_context_pP->ue_context.mac_MainConfig = mac_MainConfig;

  mac_MainConfig->ul_SCH_Config = CALLOC(1, sizeof(*mac_MainConfig->ul_SCH_Config));

  maxHARQ_Tx = CALLOC(1, sizeof(long));
  *maxHARQ_Tx = MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  mac_MainConfig->ul_SCH_Config->maxHARQ_Tx = maxHARQ_Tx;
  periodicBSR_Timer = CALLOC(1, sizeof(long));
  *periodicBSR_Timer = PeriodicBSR_Timer_r12_sf64;
  mac_MainConfig->ul_SCH_Config->periodicBSR_Timer = periodicBSR_Timer;
  mac_MainConfig->ul_SCH_Config->retxBSR_Timer = RetxBSR_Timer_r12_sf320;
  mac_MainConfig->ul_SCH_Config->ttiBundling = 0; // FALSE

  mac_MainConfig->timeAlignmentTimerDedicated = TimeAlignmentTimer_infinity;

  mac_MainConfig->drx_Config = NULL;

  mac_MainConfig->phr_Config = CALLOC(1, sizeof(*mac_MainConfig->phr_Config));

  mac_MainConfig->phr_Config->present = MAC_MainConfig__phr_Config_PR_setup;
  mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer = MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20; // sf20 = 20 subframes

  mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer = MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20; // sf20 = 20 subframes

  mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange = MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;  // Value dB1 =1 dB, dB3 = 3 dB

#if defined(Rel10) || defined(Rel14)
  sr_ProhibitTimer_r9 = CALLOC(1, sizeof(long));
  *sr_ProhibitTimer_r9 = 0;   // SR tx on PUCCH, Value in number of SR period(s). Value 0 = no timer for SR, Value 2= 2*SR
  mac_MainConfig->ext1 = CALLOC(1, sizeof(struct MAC_MainConfig__ext1));
  mac_MainConfig->ext1->sr_ProhibitTimer_r9 = sr_ProhibitTimer_r9;
  //sps_RA_ConfigList_rlola = NULL;
#endif

  //change the transmission mode for the primary component carrier
  //TODO: add codebook subset restriction here
  //TODO: change TM for secondary CC in SCelltoaddmodlist
  if (*physicalConfigDedicated) {
    if ((*physicalConfigDedicated)->antennaInfo) {
      (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.transmissionMode = rrc_inst->configuration.ue_TransmissionMode[0];
      LOG_D(RRC,"Setting transmission mode to %ld+1\n",rrc_inst->configuration.ue_TransmissionMode[0]);
      if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm3) {
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
	  CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
	  AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm3;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.buf= MALLOC(1);
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.buf[0] = 0xc0;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.size=1;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.bits_unused=6;
      }
      else if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm4) {
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
	  CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
	  AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm4;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.buf= MALLOC(1);
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.buf[0] = 0xfc;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.size=1;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.bits_unused=2;

      }
      else if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm5) {
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
	  CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
	  AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm5;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.buf= MALLOC(1);
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.buf[0] = 0xf0;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.size=1;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.bits_unused=4;
      }
      else if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm6) {
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
	  CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
	  AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm6;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.buf= MALLOC(1);
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.buf[0] = 0xf0;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.size=1;
	(*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.bits_unused=4;
      }
    }
    else {
      LOG_E(RRC,"antenna_info not present in physical_config_dedicated. Not reconfiguring!\n");
    }
    if ((*physicalConfigDedicated)->cqi_ReportConfig) {
      if ((rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm4) ||
	  (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm5) ||
	  (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm6)) {
	//feedback mode needs to be set as well
	//TODO: I think this is taken into account in the PHY automatically based on the transmission mode variable
	printf("setting cqi reporting mode to rm31\n");
#if defined(Rel10) || defined(Rel14)
	*((*physicalConfigDedicated)->cqi_ReportConfig->cqi_ReportModeAperiodic)=CQI_ReportModeAperiodic_rm31;
#else
	*((*physicalConfigDedicated)->cqi_ReportConfig->cqi_ReportModeAperiodic)=CQI_ReportConfig__cqi_ReportModeAperiodic_rm31; // HLC CQI, no PMI
#endif
      }
    }
    else {
      LOG_E(RRC,"cqi_ReportConfig not present in physical_config_dedicated. Not reconfiguring!\n");
    }
  }
  else {
    LOG_E(RRC,"physical_config_dedicated not present in RRCConnectionReconfiguration. Not reconfiguring!\n");
  }

  // Measurement ID list
  MeasId_list = CALLOC(1, sizeof(*MeasId_list));
  memset((void *)MeasId_list, 0, sizeof(*MeasId_list));

  MeasId0 = CALLOC(1, sizeof(*MeasId0));
  MeasId0->measId = 1;
  MeasId0->measObjectId = 1;
  MeasId0->reportConfigId = 1;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId0);

  MeasId1 = CALLOC(1, sizeof(*MeasId1));
  MeasId1->measId = 2;
  MeasId1->measObjectId = 1;
  MeasId1->reportConfigId = 2;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId1);

  MeasId2 = CALLOC(1, sizeof(*MeasId2));
  MeasId2->measId = 3;
  MeasId2->measObjectId = 1;
  MeasId2->reportConfigId = 3;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId2);

  MeasId3 = CALLOC(1, sizeof(*MeasId3));
  MeasId3->measId = 4;
  MeasId3->measObjectId = 1;
  MeasId3->reportConfigId = 4;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId3);

  MeasId4 = CALLOC(1, sizeof(*MeasId4));
  MeasId4->measId = 5;
  MeasId4->measObjectId = 1;
  MeasId4->reportConfigId = 5;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId4);

  MeasId5 = CALLOC(1, sizeof(*MeasId5));
  MeasId5->measId = 6;
  MeasId5->measObjectId = 1;
  MeasId5->reportConfigId = 6;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId5);

  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measIdToAddModList = MeasId_list;

  // Add one EUTRA Measurement Object
  MeasObj_list = CALLOC(1, sizeof(*MeasObj_list));
  memset((void *)MeasObj_list, 0, sizeof(*MeasObj_list));

  // Configure MeasObject

  MeasObj = CALLOC(1, sizeof(*MeasObj));
  memset((void *)MeasObj, 0, sizeof(*MeasObj));

  MeasObj->measObjectId = 1;
  MeasObj->measObject.present = MeasObjectToAddMod__measObject_PR_measObjectEUTRA;
  MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 3350; //band 7, 2.68GHz
  //MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 36090; //band 33, 1.909GHz
  MeasObj->measObject.choice.measObjectEUTRA.allowedMeasBandwidth = AllowedMeasBandwidth_mbw25;
  MeasObj->measObject.choice.measObjectEUTRA.presenceAntennaPort1 = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf = CALLOC(1, sizeof(uint8_t));
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf[0] = 0;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.size = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.bits_unused = 6;
  MeasObj->measObject.choice.measObjectEUTRA.offsetFreq = NULL;   // Default is 15 or 0dB

  MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList =
    (CellsToAddModList_t *) CALLOC(1, sizeof(*CellsToAddModList));

  CellsToAddModList = MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList;

  // Add adjacent cell lists (6 per eNB)
  for (i = 0; i < 6; i++) {
    CellToAdd = (CellsToAddMod_t *) CALLOC(1, sizeof(*CellToAdd));
    CellToAdd->cellIndex = i + 1;
    CellToAdd->physCellId = get_adjacent_cell_id(ctxt_pP->module_id, i);
    CellToAdd->cellIndividualOffset = Q_OffsetRange_dB0;

    ASN_SEQUENCE_ADD(&CellsToAddModList->list, CellToAdd);
  }

  ASN_SEQUENCE_ADD(&MeasObj_list->list, MeasObj);
  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measObjectToAddModList = MeasObj_list;

  // Report Configurations for periodical, A1-A5 events
  ReportConfig_list = CALLOC(1, sizeof(*ReportConfig_list));

  ReportConfig_per = CALLOC(1, sizeof(*ReportConfig_per));

  ReportConfig_A1 = CALLOC(1, sizeof(*ReportConfig_A1));

  ReportConfig_A2 = CALLOC(1, sizeof(*ReportConfig_A2));

  ReportConfig_A3 = CALLOC(1, sizeof(*ReportConfig_A3));

  ReportConfig_A4 = CALLOC(1, sizeof(*ReportConfig_A4));

  ReportConfig_A5 = CALLOC(1, sizeof(*ReportConfig_A5));

  ReportConfig_per->reportConfigId = 1;
  ReportConfig_per->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_periodical;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.choice.periodical.purpose =
    ReportConfigEUTRA__triggerType__periodical__purpose_reportStrongestCells;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_per);

  ReportConfig_A1->reportConfigId = 2;
  ReportConfig_A1->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA1;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA1.
  a1_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA1.
  a1_Threshold.choice.threshold_RSRP = 10;

  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A1);
  
  if (ho_state == 1 /*HO_MEASURMENT */ ) {
    LOG_I(RRC, "[eNB %d] frame %d: requesting A2, A3, A4, A5, and A6 event reporting\n",
          ctxt_pP->module_id, ctxt_pP->frame);
    ReportConfig_A2->reportConfigId = 3;
    ReportConfig_A2->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA2;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA2.a2_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA2.a2_Threshold.choice.threshold_RSRP = 10;

    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A2);

    ReportConfig_A3->reportConfigId = 4;
    ReportConfig_A3->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA3;

    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA3.a3_Offset = 1;   //10;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA3.reportOnLeave = 1;

    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.hysteresis = 0.5; // FIXME ...hysteresis is of type long!
    ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.timeToTrigger =
      TimeToTrigger_ms40;
    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A3);

    ReportConfig_A4->reportConfigId = 5;
    ReportConfig_A4->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA4;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA4.a4_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA4.a4_Threshold.choice.threshold_RSRP = 10;

    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A4);

    ReportConfig_A5->reportConfigId = 6;
    ReportConfig_A5->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_event;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
      ReportConfigEUTRA__triggerType__event__eventId_PR_eventA5;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold1.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold2.present = ThresholdEUTRA_PR_threshold_RSRP;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold1.choice.threshold_RSRP = 10;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
    eventA5.a5_Threshold2.choice.threshold_RSRP = 10;

    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerQuantity =
      ReportConfigEUTRA__triggerQuantity_rsrp;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
    ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

    ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A5);
    //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->reportConfigToAddModList = ReportConfig_list;

    rsrp = CALLOC(1, sizeof(RSRP_Range_t));
    *rsrp = 20;

    Sparams = CALLOC(1, sizeof(*Sparams));
    Sparams->present = MeasConfig__speedStatePars_PR_setup;
    Sparams->choice.setup.timeToTrigger_SF.sf_High = SpeedStateScaleFactors__sf_Medium_oDot75;
    Sparams->choice.setup.timeToTrigger_SF.sf_Medium = SpeedStateScaleFactors__sf_High_oDot5;
    Sparams->choice.setup.mobilityStateParameters.n_CellChangeHigh = 10;
    Sparams->choice.setup.mobilityStateParameters.n_CellChangeMedium = 5;
    Sparams->choice.setup.mobilityStateParameters.t_Evaluation = MobilityStateParameters__t_Evaluation_s60;
    Sparams->choice.setup.mobilityStateParameters.t_HystNormal = MobilityStateParameters__t_HystNormal_s120;

    quantityConfig = CALLOC(1, sizeof(*quantityConfig));
    memset((void *)quantityConfig, 0, sizeof(*quantityConfig));
    quantityConfig->quantityConfigEUTRA = CALLOC(1, sizeof(struct QuantityConfigEUTRA));
    memset((void *)quantityConfig->quantityConfigEUTRA, 0, sizeof(*quantityConfig->quantityConfigEUTRA));
    quantityConfig->quantityConfigCDMA2000 = NULL;
    quantityConfig->quantityConfigGERAN = NULL;
    quantityConfig->quantityConfigUTRA = NULL;
    quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP =
      CALLOC(1, sizeof(*(quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP)));
    quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ =
      CALLOC(1, sizeof(*(quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ)));
    *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP = FilterCoefficient_fc4;
    *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ = FilterCoefficient_fc4;

    LOG_I(RRC,
          "[eNB %d] Frame %d: potential handover preparation: store the information in an intermediate structure in case of failure\n",
          ctxt_pP->module_id, ctxt_pP->frame);
    // store the information in an intermediate structure for Hanodver management
    //rrc_inst->handover_info.as_config.sourceRadioResourceConfig.srb_ToAddModList = CALLOC(1,sizeof());
    ue_context_pP->ue_context.handover_info = CALLOC(1, sizeof(*(ue_context_pP->ue_context.handover_info)));
    //memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.srb_ToAddModList,(void *)SRB_list,sizeof(SRB_ToAddModList_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.srb_ToAddModList = *SRB_configList2;
    //memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.drb_ToAddModList,(void *)DRB_list,sizeof(DRB_ToAddModList_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.drb_ToAddModList = *DRB_configList;
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.drb_ToReleaseList = NULL;
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig =
      CALLOC(1, sizeof(*ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig));
    memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig,
           (void *)mac_MainConfig, sizeof(MAC_MainConfig_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.physicalConfigDedicated =
      CALLOC(1, sizeof(PhysicalConfigDedicated_t));
    memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.physicalConfigDedicated,
           (void*)ue_context_pP->ue_context.physicalConfigDedicated, sizeof(PhysicalConfigDedicated_t));
    ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.sps_Config = NULL;
    //memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.sps_Config,(void *)rrc_inst->sps_Config[ue_mod_idP],sizeof(SPS_Config_t));

  }

#if defined(ENABLE_ITTI)
  /* Initialize NAS list */
  dedicatedInfoNASList = CALLOC(1, sizeof(struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList));

  /* Add all NAS PDUs to the list */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      dedicatedInfoNas = CALLOC(1, sizeof(DedicatedInfoNAS_t));
      memset(dedicatedInfoNas, 0, sizeof(OCTET_STRING_t));
      OCTET_STRING_fromBuf(dedicatedInfoNas, 
			   (char*)ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer,
                           ue_context_pP->ue_context.e_rab[i].param.nas_pdu.length);
      ASN_SEQUENCE_ADD(&dedicatedInfoNASList->list, dedicatedInfoNas);
    }

    /* TODO parameters yet to process ... */
    {
      //      ue_context_pP->ue_context.e_rab[i].param.qos;
      //      ue_context_pP->ue_context.e_rab[i].param.sgw_addr;
      //      ue_context_pP->ue_context.e_rab[i].param.gtp_teid;
    }

    /* TODO should test if e RAB are Ok before! */
    ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_DONE;
    LOG_D(RRC, "setting the status for the default DRB (index %d) to (%d,%s)\n", 
	  i, ue_context_pP->ue_context.e_rab[i].status, "E_RAB_STATUS_DONE");
  }

  /* If list is empty free the list and reset the address */
  if (dedicatedInfoNASList->list.count == 0) {
    free(dedicatedInfoNASList);
    dedicatedInfoNASList = NULL;
  }

#endif

  memset(buffer, 0, RRC_BUF_SIZE);

  size = do_RRCConnectionReconfiguration(ctxt_pP,
                                         buffer,
                                         xid,   //Transaction_id,
                                         (SRB_ToAddModList_t*)*SRB_configList2, // SRB_configList
                                         (DRB_ToAddModList_t*)*DRB_configList,
                                         (DRB_ToReleaseList_t*)NULL,  // DRB2_list,
                                         (struct SPS_Config*)NULL,    // *sps_Config,
                                         (struct PhysicalConfigDedicated*)*physicalConfigDedicated,
#ifdef EXMIMO_IOT
                                         NULL, NULL, NULL,NULL,
#else
                                         (MeasObjectToAddModList_t*)MeasObj_list,
                                         (ReportConfigToAddModList_t*)ReportConfig_list,
                                         (QuantityConfig_t*)quantityConfig,
                                         (MeasIdToAddModList_t*)MeasId_list,
#endif
                                         (MAC_MainConfig_t*)mac_MainConfig,
                                         (MeasGapConfig_t*)NULL,
                                         (MobilityControlInfo_t*)NULL,
                                         (struct MeasConfig__speedStatePars*)Sparams,
                                         (RSRP_Range_t*)rsrp,
                                         (C_RNTI_t*)cba_RNTI,
                                         (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
                                         , (SCellToAddMod_r10_t*)NULL
#endif
                                        );

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Reconfiguration\n");
  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }
  LOG_F(RRC,"\n");
  ////////////////////////////////////////
#endif

#if defined(ENABLE_ITTI)

  /* Free all NAS PDUs */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      /* Free the NAS PDU buffer and invalidate it */
      free(ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer);
      ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer = NULL;
    }
  }

#endif

  LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE id %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
	       ctxt_pP,
	       DCCH,
	       rrc_eNB_mui++,
	       SDU_CONFIRM_NO,
	       size,
	       buffer,
	       PDCP_TRANSMISSION_MODE_CONTROL);
}

//-----------------------------------------------------------------------------
void
flexran_rrc_eNB_generate_defaultRRCConnectionReconfiguration(const protocol_ctxt_t* const ctxt_pP,
                 rrc_eNB_ue_context_t*          const ue_context_pP,
                 const uint8_t                ho_state,
                 agent_reconf_rrc * trig_param
                 )
//-----------------------------------------------------------------------------
{
  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;
  int                                 i;
 
  // configure SRB1/SRB2, PhysicalConfigDedicated, MAC_MainConfig for UE
  eNB_RRC_INST*                       rrc_inst = RC.rrc[ctxt_pP->module_id];
  struct PhysicalConfigDedicated**    physicalConfigDedicated = &ue_context_pP->ue_context.physicalConfigDedicated;

  struct SRB_ToAddMod                *SRB2_config                      = NULL;
  struct SRB_ToAddMod__rlc_Config    *SRB2_rlc_config                  = NULL;
  struct SRB_ToAddMod__logicalChannelConfig *SRB2_lchan_config         = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters
      *SRB2_ul_SpecificParameters       = NULL;
  SRB_ToAddModList_t*                 SRB_configList = ue_context_pP->ue_context.SRB_configList;
  SRB_ToAddModList_t                 **SRB_configList2                  = NULL;

  struct DRB_ToAddMod                *DRB_config                       = NULL;
  struct RLC_Config                  *DRB_rlc_config                   = NULL;
  struct PDCP_Config                 *DRB_pdcp_config                  = NULL;
  struct PDCP_Config__rlc_AM         *PDCP_rlc_AM                      = NULL;
  struct PDCP_Config__rlc_UM         *PDCP_rlc_UM                      = NULL;
  struct LogicalChannelConfig        *DRB_lchan_config                 = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters
      *DRB_ul_SpecificParameters        = NULL;
  DRB_ToAddModList_t**                DRB_configList = &ue_context_pP->ue_context.DRB_configList;
  DRB_ToAddModList_t**                DRB_configList2 = NULL;
   MAC_MainConfig_t                   *mac_MainConfig                   = NULL;
  MeasObjectToAddModList_t           *MeasObj_list                     = NULL;
  MeasObjectToAddMod_t               *MeasObj                          = NULL;
  ReportConfigToAddModList_t         *ReportConfig_list                = NULL;
  ReportConfigToAddMod_t             *ReportConfig_per;//, *ReportConfig_A1,
                                     // *ReportConfig_A2, *ReportConfig_A3, *ReportConfig_A4, *ReportConfig_A5;
  MeasIdToAddModList_t               *MeasId_list                      = NULL;
  MeasIdToAddMod_t                   *MeasId0; //, *MeasId1, *MeasId2, *MeasId3, *MeasId4, *MeasId5;
#if Rel10
  long                               *sr_ProhibitTimer_r9              = NULL;
  //     uint8_t sCellIndexToAdd = rrc_find_free_SCell_index(enb_mod_idP, ue_mod_idP, 1);
  //uint8_t                            sCellIndexToAdd = 0;
#endif

  long                               *logicalchannelgroup, *logicalchannelgroup_drb;
  long                               *maxHARQ_Tx, *periodicBSR_Timer;

  RSRP_Range_t                       *rsrp                             = NULL;
  struct MeasConfig__speedStatePars  *Sparams                          = NULL;
  QuantityConfig_t                   *quantityConfig                   = NULL;
  CellsToAddMod_t                    *CellToAdd                        = NULL;
  CellsToAddModList_t                *CellsToAddModList                = NULL;
  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList = NULL;
  DedicatedInfoNAS_t                 *dedicatedInfoNas                 = NULL;
  /* for no gcc warnings */
  (void)dedicatedInfoNas;

  C_RNTI_t                           *cba_RNTI                         = NULL;

  uint8_t xid = rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id);   //Transaction_id,

#ifdef CBA
  //struct PUSCH_CBAConfigDedicated_vlola  *pusch_CBAConfigDedicated_vlola;
  uint8_t                            *cba_RNTI_buf;
  cba_RNTI = CALLOC(1, sizeof(C_RNTI_t));
  cba_RNTI_buf = CALLOC(1, 2 * sizeof(uint8_t));
  cba_RNTI->buf = cba_RNTI_buf;
  cba_RNTI->size = 2;
  cba_RNTI->bits_unused = 0;

  // associate UEs to the CBa groups as a function of their UE id
  if (rrc_inst->num_active_cba_groups) {
    cba_RNTI->buf[0] = rrc_inst->cba_rnti[ue_mod_idP % rrc_inst->num_active_cba_groups] & 0xff;
    cba_RNTI->buf[1] = 0xff;
    LOG_D(RRC,
          "[eNB %d] Frame %d: cba_RNTI = %x in group %d is attribued to UE %d\n",
          enb_mod_idP, frameP,
          rrc_inst->cba_rnti[ue_mod_idP % rrc_inst->num_active_cba_groups],
          ue_mod_idP % rrc_inst->num_active_cba_groups, ue_mod_idP);
  } else {
    cba_RNTI->buf[0] = 0x0;
    cba_RNTI->buf[1] = 0x0;
    LOG_D(RRC, "[eNB %d] Frame %d: no cba_RNTI is configured for UE %d\n", enb_mod_idP, frameP, ue_mod_idP);
  }

#endif

  T(T_ENB_RRC_CONNECTION_RECONFIGURATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  // Configure SRB2
  /// SRB2
  SRB_configList2=&ue_context_pP->ue_context.SRB_configList2[xid];
  if (*SRB_configList2) {
    free(*SRB_configList2);
  }
  *SRB_configList2 = CALLOC(1, sizeof(**SRB_configList2));
  memset(*SRB_configList2, 0, sizeof(**SRB_configList2));
  SRB2_config = CALLOC(1, sizeof(*SRB2_config));

  SRB2_config->srb_Identity = 2;
  SRB2_rlc_config = CALLOC(1, sizeof(*SRB2_rlc_config));
  SRB2_config->rlc_Config = SRB2_rlc_config;

  SRB2_rlc_config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB2_rlc_config->choice.explicitValue.present = RLC_Config_PR_am;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms15;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU = PollPDU_p8;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte = PollByte_kB1000;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;
  SRB2_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
  SRB2_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms10;

  SRB2_lchan_config = CALLOC(1, sizeof(*SRB2_lchan_config));
  SRB2_config->logicalChannelConfig = SRB2_lchan_config;

  SRB2_lchan_config->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;

  SRB2_ul_SpecificParameters = CALLOC(1, sizeof(*SRB2_ul_SpecificParameters));

  SRB2_ul_SpecificParameters->priority = 3; // let some priority for SRB1 and dedicated DRBs
  SRB2_ul_SpecificParameters->prioritisedBitRate =
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  SRB2_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  // LCG for CCCH and DCCH is 0 as defined in 36331
  logicalchannelgroup = CALLOC(1, sizeof(long));
  *logicalchannelgroup = 0;

  SRB2_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;

  SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB2_ul_SpecificParameters;
  // this list has the configuration for SRB1 and SRB2
  ASN_SEQUENCE_ADD(&SRB_configList->list, SRB2_config);
  // this list has only the configuration for SRB2
  ASN_SEQUENCE_ADD(&(*SRB_configList2)->list, SRB2_config);

  // Configure DRB
  //*DRB_configList = CALLOC(1, sizeof(*DRB_configList));
  // list for all the configured DRB
  if (*DRB_configList) {
    free(*DRB_configList);
  }
  *DRB_configList = CALLOC(1, sizeof(**DRB_configList));
  memset(*DRB_configList, 0, sizeof(**DRB_configList));

  // list for the configured DRB for a this xid
  DRB_configList2=&ue_context_pP->ue_context.DRB_configList2[xid];
  if (*DRB_configList2) {
    free(*DRB_configList2);
  }
  *DRB_configList2 = CALLOC(1, sizeof(**DRB_configList2));
  memset(*DRB_configList2, 0, sizeof(**DRB_configList2));


  /// DRB
  DRB_config = CALLOC(1, sizeof(*DRB_config));

  DRB_config->eps_BearerIdentity = CALLOC(1, sizeof(long));
  *(DRB_config->eps_BearerIdentity) = 5L; // LW set to first value, allowed value 5..15, value : x+4
  // DRB_config->drb_Identity = (DRB_Identity_t) 1; //allowed values 1..32
  // NN: this is the 1st DRB for this ue, so set it to 1
  DRB_config->drb_Identity = (DRB_Identity_t) 1;  // (ue_mod_idP+1); //allowed values 1..32, value: x
  DRB_config->logicalChannelIdentity = CALLOC(1, sizeof(long));
  *(DRB_config->logicalChannelIdentity) = (long)3; // value : x+2
  DRB_rlc_config = CALLOC(1, sizeof(*DRB_rlc_config));
  DRB_config->rlc_Config = DRB_rlc_config;

#ifdef RRC_DEFAULT_RAB_IS_AM
  DRB_rlc_config->present = RLC_Config_PR_am;
  DRB_rlc_config->choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
  DRB_rlc_config->choice.am.ul_AM_RLC.pollPDU = PollPDU_p16;
  DRB_rlc_config->choice.am.ul_AM_RLC.pollByte = PollByte_kBinfinity;
  DRB_rlc_config->choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t8;
  DRB_rlc_config->choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
  DRB_rlc_config->choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms25;
#else
  DRB_rlc_config->present = RLC_Config_PR_um_Bi_Directional;
  DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
#ifdef CBA
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering   = T_Reordering_ms5;//T_Reordering_ms25;
#else
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = T_Reordering_ms35;
#endif
#endif

  DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
  DRB_config->pdcp_Config = DRB_pdcp_config;
  DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
  *DRB_pdcp_config->discardTimer = PDCP_Config__discardTimer_infinity;
  DRB_pdcp_config->rlc_AM = NULL;
  DRB_pdcp_config->rlc_UM = NULL;

  /* avoid gcc warnings */
  (void)PDCP_rlc_AM;
  (void)PDCP_rlc_UM;

#ifdef RRC_DEFAULT_RAB_IS_AM // EXMIMO_IOT
  PDCP_rlc_AM = CALLOC(1, sizeof(*PDCP_rlc_AM));
  DRB_pdcp_config->rlc_AM = PDCP_rlc_AM;
  PDCP_rlc_AM->statusReportRequired = FALSE;
#else
  PDCP_rlc_UM = CALLOC(1, sizeof(*PDCP_rlc_UM));
  DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
  PDCP_rlc_UM->pdcp_SN_Size = PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
#endif
  DRB_pdcp_config->headerCompression.present = PDCP_Config__headerCompression_PR_notUsed;

  DRB_lchan_config = CALLOC(1, sizeof(*DRB_lchan_config));
  DRB_config->logicalChannelConfig = DRB_lchan_config;
  DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
  DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;

  DRB_ul_SpecificParameters->priority = 12;    // lower priority than srb1, srb2 and other dedicated bearer
  DRB_ul_SpecificParameters->prioritisedBitRate =LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8 ;
    //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  DRB_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  // LCG for DTCH can take the value from 1 to 3 as defined in 36331: normally controlled by upper layers (like RRM)
  logicalchannelgroup_drb = CALLOC(1, sizeof(long));
  *logicalchannelgroup_drb = 1;
  DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;

  ASN_SEQUENCE_ADD(&(*DRB_configList)->list, DRB_config);
  ASN_SEQUENCE_ADD(&(*DRB_configList2)->list, DRB_config);

  //ue_context_pP->ue_context.DRB_configList2[0] = &(*DRB_configList);

  mac_MainConfig = CALLOC(1, sizeof(*mac_MainConfig));
  // ue_context_pP->ue_context.mac_MainConfig = mac_MainConfig;

  mac_MainConfig->ul_SCH_Config = CALLOC(1, sizeof(*mac_MainConfig->ul_SCH_Config));

  maxHARQ_Tx = CALLOC(1, sizeof(long));
  *maxHARQ_Tx = MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  mac_MainConfig->ul_SCH_Config->maxHARQ_Tx = maxHARQ_Tx;
  periodicBSR_Timer = CALLOC(1, sizeof(long));
  *periodicBSR_Timer = PeriodicBSR_Timer_r12_sf64;
  mac_MainConfig->ul_SCH_Config->periodicBSR_Timer = periodicBSR_Timer;
  mac_MainConfig->ul_SCH_Config->retxBSR_Timer = RetxBSR_Timer_r12_sf320;
  mac_MainConfig->ul_SCH_Config->ttiBundling = 0; // FALSE

  mac_MainConfig->timeAlignmentTimerDedicated = TimeAlignmentTimer_infinity;

  mac_MainConfig->drx_Config = NULL;

  mac_MainConfig->phr_Config = CALLOC(1, sizeof(*mac_MainConfig->phr_Config));

  mac_MainConfig->phr_Config->present = MAC_MainConfig__phr_Config_PR_setup;
  mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer = MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20; // sf20 = 20 subframes

  mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer = MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20; // sf20 = 20 subframes

  mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange = MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;  // Value dB1 =1 dB, dB3 = 3 dB

#ifdef Rel10
  sr_ProhibitTimer_r9 = CALLOC(1, sizeof(long));
  *sr_ProhibitTimer_r9 = 0;   // SR tx on PUCCH, Value in number of SR period(s). Value 0 = no timer for SR, Value 2= 2*SR
  mac_MainConfig->ext1 = CALLOC(1, sizeof(struct MAC_MainConfig__ext1));
  mac_MainConfig->ext1->sr_ProhibitTimer_r9 = sr_ProhibitTimer_r9;
  //sps_RA_ConfigList_rlola = NULL;
#endif

  //change the transmission mode for the primary component carrier
  //TODO: add codebook subset restriction here
  //TODO: change TM for secondary CC in SCelltoaddmodlist
  if (*physicalConfigDedicated) {
    if ((*physicalConfigDedicated)->antennaInfo) {
      (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.transmissionMode = rrc_inst->configuration.ue_TransmissionMode[0];
      LOG_D(RRC,"Setting transmission mode to %ld+1\n",rrc_inst->configuration.ue_TransmissionMode[0]);
      if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm3) {
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
    CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
    AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm3;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.buf= MALLOC(1);
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.buf[0] = 0xc0;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.size=1;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.bits_unused=6;
      }
      else if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm4) {
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
    CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
    AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm4;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.buf= MALLOC(1);
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.buf[0] = 0xfc;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.size=1;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm4.bits_unused=2;

      }
      else if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm5) {
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
    CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
    AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm5;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.buf= MALLOC(1);
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.buf[0] = 0xf0;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.size=1;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm5.bits_unused=4;
      }
      else if (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm6) {
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     
    CALLOC(1,sizeof(AntennaInfoDedicated__codebookSubsetRestriction_PR));
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
    AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm6;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.buf= MALLOC(1);
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.buf[0] = 0xf0;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.size=1;
  (*physicalConfigDedicated)->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm6.bits_unused=4;
      }
    }
    else {
      LOG_E(RRC,"antenna_info not present in physical_config_dedicated. Not reconfiguring!\n");
    }
    if ((*physicalConfigDedicated)->cqi_ReportConfig) {
      if ((rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm4) ||
    (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm5) ||
    (rrc_inst->configuration.ue_TransmissionMode[0]==AntennaInfoDedicated__transmissionMode_tm6)) {
  //feedback mode needs to be set as well
  //TODO: I think this is taken into account in the PHY automatically based on the transmission mode variable
  printf("setting cqi reporting mode to rm31\n");
#if defined(Rel10) || defined(Rel14)
  *((*physicalConfigDedicated)->cqi_ReportConfig->cqi_ReportModeAperiodic)=CQI_ReportModeAperiodic_rm31;
#else
  *((*physicalConfigDedicated)->cqi_ReportConfig->cqi_ReportModeAperiodic)=CQI_ReportConfig__cqi_ReportModeAperiodic_rm31; // HLC CQI, no PMI
#endif
      }
    }
    else {
      LOG_E(RRC,"cqi_ReportConfig not present in physical_config_dedicated. Not reconfiguring!\n");
    }
  }
  else {
    LOG_E(RRC,"physical_config_dedicated not present in RRCConnectionReconfiguration. Not reconfiguring!\n");
  }

  // Measurement ID list
  MeasId_list = CALLOC(1, sizeof(*MeasId_list));
  memset((void *)MeasId_list, 0, sizeof(*MeasId_list));

  MeasId0 = CALLOC(1, sizeof(*MeasId0));
  MeasId0->measId = 1;
  MeasId0->measObjectId = 1;
  MeasId0->reportConfigId = 1;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId0);

  /*
   * Add one EUTRA Measurement Object
  */

  MeasObj_list = CALLOC(1, sizeof(*MeasObj_list));
  memset((void *)MeasObj_list, 0, sizeof(*MeasObj_list));

  // Configure MeasObject 

  MeasObj = CALLOC(1, sizeof(*MeasObj));
  memset((void *)MeasObj, 0, sizeof(*MeasObj));

  MeasObj->measObjectId = 1;
  MeasObj->measObject.present = MeasObjectToAddMod__measObject_PR_measObjectEUTRA;
  MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 3350; //band 7, 2.68GHz
  //MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 36090; //band 33, 1.909GHz
  MeasObj->measObject.choice.measObjectEUTRA.allowedMeasBandwidth = AllowedMeasBandwidth_mbw25;
  MeasObj->measObject.choice.measObjectEUTRA.presenceAntennaPort1 = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf = CALLOC(1, sizeof(uint8_t));
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf[0] = 0;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.size = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.bits_unused = 6;
  MeasObj->measObject.choice.measObjectEUTRA.offsetFreq = NULL;   // Default is 15 or 0dB

  MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList =
    (CellsToAddModList_t *) CALLOC(1, sizeof(*CellsToAddModList));

  CellsToAddModList = MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList;

  // Add adjacent cell lists (6 per eNB)
  for (i = 0; i < 6; i++) {
    CellToAdd = (CellsToAddMod_t *) CALLOC(1, sizeof(*CellToAdd));
    CellToAdd->cellIndex = i + 1;
    CellToAdd->physCellId = get_adjacent_cell_id(ctxt_pP->module_id, i);
    CellToAdd->cellIndividualOffset = Q_OffsetRange_dB0;

    ASN_SEQUENCE_ADD(&CellsToAddModList->list, CellToAdd);
  }

  ASN_SEQUENCE_ADD(&MeasObj_list->list, MeasObj);
  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measObjectToAddModList = MeasObj_list;

  // Report Configurations for periodical, A1-A5 events

  /* RRC Strategy Measurement */


  if (strcmp("one_shot", trig_param->trigger_policy) == 0){

      trig_param->report_interval = 0;
      trig_param->report_amount = 0;

  }

  else if (strcmp("event_driven", trig_param->trigger_policy) == 0){

      trig_param->report_interval = 6;
      trig_param->report_amount = 2;

  }

  else if (strcmp("periodical", trig_param->trigger_policy) == 0){

      trig_param->report_interval = 1;
      trig_param->report_amount = 7;

  }

  else {

     LOG_E(FLEXRAN_AGENT, "There is something wrong on RRC agent!");
  }



  ReportConfig_list = CALLOC(1, sizeof(*ReportConfig_list));

  ReportConfig_per = CALLOC(1, sizeof(*ReportConfig_per));

    // Periodical Measurement Report

  ReportConfig_per->reportConfigId = 1;
  ReportConfig_per->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;

    ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.present =
      ReportConfigEUTRA__triggerType_PR_periodical;

    ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.choice.periodical.purpose =
      ReportConfigEUTRA__triggerType__periodical__purpose_reportStrongestCells;

    // ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.timeToTrigger = TimeToTrigger_ms40;  
    ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
   ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
   ReportConfig_per->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
   ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportInterval = trig_param->report_interval ;//ReportInterval_ms2048; // RRC counter frame- ms1024 is 1ms   

   ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportAmount = trig_param->report_amount; //ReportConfigEUTRA__reportAmount_r2; // put r1 to see once, r2 for 2 times and ...


  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_per);



    quantityConfig = CALLOC(1, sizeof(*quantityConfig));
    memset((void *)quantityConfig, 0, sizeof(*quantityConfig));
    quantityConfig->quantityConfigEUTRA = CALLOC(1, sizeof(struct QuantityConfigEUTRA));
    memset((void *)quantityConfig->quantityConfigEUTRA, 0, sizeof(*quantityConfig->quantityConfigEUTRA));
    quantityConfig->quantityConfigCDMA2000 = NULL;
    quantityConfig->quantityConfigGERAN = NULL;
    quantityConfig->quantityConfigUTRA = NULL;
    quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP =
      CALLOC(1, sizeof(*(quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP)));
    quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ =
      CALLOC(1, sizeof(*(quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ)));
    *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP = FilterCoefficient_fc4;
    *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ = FilterCoefficient_fc4;

  
#if defined(ENABLE_ITTI)
  /* Initialize NAS list */
  dedicatedInfoNASList = CALLOC(1, sizeof(struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList));

  /* Add all NAS PDUs to the list */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      dedicatedInfoNas = CALLOC(1, sizeof(DedicatedInfoNAS_t));
      memset(dedicatedInfoNas, 0, sizeof(OCTET_STRING_t));
      OCTET_STRING_fromBuf(dedicatedInfoNas, 
         (char*)ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer,
                           ue_context_pP->ue_context.e_rab[i].param.nas_pdu.length);
      ASN_SEQUENCE_ADD(&dedicatedInfoNASList->list, dedicatedInfoNas);
    }

    /* TODO parameters yet to process ... */
    // {
      //      ue_context_pP->ue_context.e_rab[i].param.qos;
      //      ue_context_pP->ue_context.e_rab[i].param.sgw_addr;
      //      ue_context_pP->ue_context.e_rab[i].param.gtp_teid;
    // }

    /* TODO should test if e RAB are Ok before! */
    ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_DONE;
    LOG_D(RRC, "setting the status for the default DRB (index %d) to (%d,%s)\n", 
    i, ue_context_pP->ue_context.e_rab[i].status, "E_RAB_STATUS_DONE");
  }

  /* If list is empty free the list and reset the address */
  if (dedicatedInfoNASList->list.count == 0) {
    free(dedicatedInfoNASList);
    dedicatedInfoNASList = NULL;
  }

#endif

  memset(buffer, 0, RRC_BUF_SIZE);
  
  size = do_RRCConnectionReconfiguration(ctxt_pP,
                                         buffer,
                                         xid,   //Transaction_id,
                                         (SRB_ToAddModList_t*)NULL, // SRB_configList
                                         (DRB_ToAddModList_t*)NULL,
                                         (DRB_ToReleaseList_t*)NULL,  // DRB2_list,
                                         (struct SPS_Config*)NULL,    // *sps_Config,
                                         (struct PhysicalConfigDedicated*)*physicalConfigDedicated,
// #ifdef EXMIMO_IOT
//                                          NULL, NULL, NULL,NULL,
// #else
                                         (MeasObjectToAddModList_t*)MeasObj_list,
                                         (ReportConfigToAddModList_t*)ReportConfig_list,
                                         (QuantityConfig_t*)quantityConfig,
                                         (MeasIdToAddModList_t*)MeasId_list,
// #endif
                                         (MAC_MainConfig_t*)mac_MainConfig,
                                         (MeasGapConfig_t*)NULL,
                                         (MobilityControlInfo_t*)NULL,
                                         (struct MeasConfig__speedStatePars*)Sparams,
                                         (RSRP_Range_t*)rsrp,
                                         (C_RNTI_t*)cba_RNTI,
                                         (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
                                         , (SCellToAddMod_r10_t*)NULL
#endif
                                        );

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Reconfiguration\n");
  for (i = 0; i < size; i++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
  }
  LOG_F(RRC,"\n");
  ////////////////////////////////////////
#endif

#if defined(ENABLE_ITTI)

  /* Free all NAS PDUs */
  for (i = 0; i < ue_context_pP->ue_context.nb_of_e_rabs; i++) {
    if (ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer != NULL) {
      /* Free the NAS PDU buffer and invalidate it */
      free(ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer);
      ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer = NULL;
    }
  }

#endif

  LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE id %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
         ctxt_pP,
         DCCH,
         rrc_eNB_mui++,
         SDU_CONFIRM_NO,
         size,
         buffer,
         PDCP_TRANSMISSION_MODE_CONTROL);
}


//-----------------------------------------------------------------------------
int
rrc_eNB_generate_RRCConnectionReconfiguration_SCell(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t* const ue_context_pP,
  uint32_t dl_CarrierFreq_r10
)
//-----------------------------------------------------------------------------
{

  uint8_t size;
  uint8_t buffer[100];

#if defined(Rel10) || defined(Rel14)
  uint8_t sCellIndexToAdd = 0; //one SCell so far

  //   uint8_t sCellIndexToAdd;
  //   sCellIndexToAdd = rrc_find_free_SCell_index(enb_mod_idP, ue_mod_idP, 1);
  //  if (RC.rrc[enb_mod_idP]->sCell_config[ue_mod_idP][sCellIndexToAdd] ) {
  if (ue_context_pP->ue_context.sCell_config != NULL) {
    ue_context_pP->ue_context.sCell_config[sCellIndexToAdd].cellIdentification_r10->dl_CarrierFreq_r10 = dl_CarrierFreq_r10;
  } else {
    LOG_E(RRC,"Scell not configured!\n");
    return(-1);
  }

#endif
  size = do_RRCConnectionReconfiguration(ctxt_pP,
                                         buffer,
                                         rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id),//Transaction_id,
                                         (SRB_ToAddModList_t*)NULL,
                                         (DRB_ToAddModList_t*)NULL,
                                         (DRB_ToReleaseList_t*)NULL,
                                         (struct SPS_Config*)NULL,
                                         (struct PhysicalConfigDedicated*)NULL,
                                         (MeasObjectToAddModList_t*)NULL,
                                         (ReportConfigToAddModList_t*)NULL,
                                         (QuantityConfig_t*)NULL,
                                         (MeasIdToAddModList_t*)NULL,
                                         (MAC_MainConfig_t*)NULL,
                                         (MeasGapConfig_t*)NULL,
                                         (MobilityControlInfo_t*)NULL,
                                         (struct MeasConfig__speedStatePars*)NULL,
                                         (RSRP_Range_t*)NULL,
                                         (C_RNTI_t*)NULL,
                                         (struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList*)NULL

#if defined(Rel10) || defined(Rel14)
                                         , ue_context_pP->ue_context.sCell_config
#endif
                                        );
  LOG_I(RRC,"[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration (bytes %d, UE id %x)\n",
        ctxt_pP->module_id,ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcConnectionReconfiguration UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_eNB_mui,
    size);

  rrc_data_req(
	       ctxt_pP,
	       DCCH,
	       rrc_eNB_mui++,
	       SDU_CONFIRM_NO,
	       size,
	       buffer,
	       PDCP_TRANSMISSION_MODE_CONTROL);
  return(0);
}


//-----------------------------------------------------------------------------
void
rrc_eNB_process_MeasurementReport(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*         ue_context_pP,
  const MeasResults_t*   const measResults2
)
//-----------------------------------------------------------------------------
{
  int i=0;
  int neighboring_cells=-1;
  
  T(T_ENB_RRC_MEASUREMENT_REPORT, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  if (measResults2 == NULL )
    return;
  
  if (measResults2->measId > 0 ){
     if (ue_context_pP->ue_context.measResults == NULL) {
       ue_context_pP->ue_context.measResults = CALLOC(1, sizeof(MeasResults_t));
     }
     ue_context_pP->ue_context.measResults->measId=measResults2->measId; 
     ue_context_pP->ue_context.measResults->measResultPCell.rsrpResult=measResults2->measResultPCell.rsrpResult;
     ue_context_pP->ue_context.measResults->measResultPCell.rsrqResult=measResults2->measResultPCell.rsrqResult;
     LOG_D(RRC, "[eNB %d]Frame %d: UE %x (Measurement Id %d): RSRP of Source %ld\n", ctxt_pP->module_id, ctxt_pP->frame, ctxt_pP->rnti, (int)measResults2->measId, ue_context_pP->ue_context.measResults->measResultPCell.rsrpResult-140);
     LOG_D(RRC, "[eNB %d]Frame %d: UE %x (Measurement Id %d): RSRQ of Source %ld\n", ctxt_pP->module_id, ctxt_pP->frame, ctxt_pP->rnti, (int)measResults2->measId, ue_context_pP->ue_context.measResults->measResultPCell.rsrqResult/2 - 20);
   }
   if (measResults2->measResultNeighCells == NULL)
     return;

   if (measResults2->measResultNeighCells->choice.measResultListEUTRA.list.count > 0) {
     neighboring_cells = measResults2->measResultNeighCells->choice.measResultListEUTRA.list.count;
     
     if (ue_context_pP->ue_context.measResults->measResultNeighCells == NULL) {
       
       ue_context_pP->ue_context.measResults->measResultNeighCells = CALLOC(1, sizeof(*measResults2->measResultNeighCells)*neighboring_cells);
     }
     ue_context_pP->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.count = neighboring_cells;
     for (i=0; i < neighboring_cells; i++){
       memcpy (ue_context_pP->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[i],
	       measResults2->measResultNeighCells->choice.measResultListEUTRA.list.array[i],
	       sizeof(MeasResultListEUTRA_t));
       
       LOG_D(RRC, "Physical Cell Id %d\n",
	     (int)ue_context_pP->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[i]->physCellId);
       LOG_D(RRC, "RSRP of Target %d\n",
	     (int)*(ue_context_pP->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[i]->measResult.rsrpResult));
       LOG_D(RRC, "RSRQ of Target %d\n",
	     (int)*(ue_context_pP->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[i]->measResult.rsrqResult));
     }
   }

// #if defined(Rel10) || defined(Rel14)

  
// #else
  // LOG_I(RRC, "RSRP of Source %d\n", measResults2->measResultServCell.rsrpResult);
  // LOG_I(RRC, "RSRQ of Source %d\n", measResults2->measResultServCell.rsrqResult);
// #endif

  // if (ue_context_pP->ue_context.handover_info->ho_prepare != 0xF0) {
  //   rrc_eNB_generate_HandoverPreparationInformation(ctxt_pP,
  //       ue_context_pP,
  //       measResults2->measResultNeighCells->choice.
  //       measResultListEUTRA.list.array[0]->physCellId);
  // } else {
  //   LOG_D(RRC, "[eNB %d] Frame %d: Ignoring MeasReport from UE %x as Handover is in progress... \n", ctxt_pP->module_id, ctxt_pP->frame,
  //         ctxt_pP->rnti);
  // }

  //Look for IP address of the target eNB
  //Send Handover Request -> target eNB
  //Wait for Handover Acknowledgement <- target eNB
  //Send Handover Command

  //x2delay();
  //    handover_request_x2(ue_mod_idP,enb_mod_idP,measResults2->measResultNeighCells->choice.measResultListEUTRA.list.array[0]->physCellId);

  //    uint8_t buffer[100];
  //    int size=rrc_eNB_generate_Handover_Command_TeNB(0,0,buffer);
  //
  //      send_check_message((char*)buffer,size);
  //send_handover_command();

}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_HandoverPreparationInformation(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t* const ue_context_pP,
  PhysCellId_t                 targetPhyId
)
//-----------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s*        ue_context_target_p = NULL;
  //uint8_t                             UE_id_target        = -1;
  uint8_t                             mod_id_target = get_adjacent_cell_mod_id(targetPhyId);
  HANDOVER_INFO                      *handoverInfo = CALLOC(1, sizeof(*handoverInfo));
  /*
     uint8_t buffer[100];
     uint8_t size;
     struct PhysicalConfigDedicated  **physicalConfigDedicated = &RC.rrc[enb_mod_idP]->physicalConfigDedicated[ue_mod_idP];
     RadioResourceConfigDedicated_t *radioResourceConfigDedicated = CALLOC(1,sizeof(RadioResourceConfigDedicated_t));
   */

  T(T_ENB_RRC_HANDOVER_PREPARATION_INFORMATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  handoverInfo->as_config.antennaInfoCommon.antennaPortsCount = 0;    //Not used 0- but check value
  handoverInfo->as_config.sourceDl_CarrierFreq = 36090;   //Verify!

  memcpy((void *)&handoverInfo->as_config.sourceMasterInformationBlock,
         (void*)&RC.rrc[ctxt_pP->module_id]->carrier[0] /* CROUX TBC */.mib, sizeof(MasterInformationBlock_t));
  memcpy((void *)&handoverInfo->as_config.sourceMeasConfig,
         (void*)ue_context_pP->ue_context.measConfig, sizeof(MeasConfig_t));

  // FIXME handoverInfo not used...
  free( handoverInfo );
  handoverInfo = 0;

  //to be configured
  memset((void*)&ue_context_pP->ue_context.handover_info->as_config.sourceSecurityAlgorithmConfig,
         0, sizeof(SecurityAlgorithmConfig_t));

  memcpy((void*)&ue_context_pP->ue_context.handover_info->as_config.sourceSystemInformationBlockType1,
         (void*)&RC.rrc[ctxt_pP->module_id]->carrier[0] /* CROUX TBC */.SIB1, sizeof(SystemInformationBlockType1_t));
  memcpy((void*)&ue_context_pP->ue_context.handover_info->as_config.sourceSystemInformationBlockType2,
         (void*)&RC.rrc[ctxt_pP->module_id]->carrier[0] /* CROUX TBC */.SIB23, sizeof(SystemInformationBlockType2_t));
  ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo =
    CALLOC(1, sizeof(ReestablishmentInfo_t));
  ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo->sourcePhysCellId =
    RC.rrc[ctxt_pP->module_id]->carrier[0] /* CROUX TBC */.physCellId;
  ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo->targetCellShortMAC_I.buf = NULL;  // Check values later
  ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo->targetCellShortMAC_I.size = 0;
  ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo->targetCellShortMAC_I.bits_unused = 0;
  ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo->additionalReestabInfoList = NULL;
  ue_context_pP->ue_context.handover_info->ho_prepare = 0xFF;    //0xF0;
  ue_context_pP->ue_context.handover_info->ho_complete = 0;

  if (mod_id_target != 0xFF) {
    //UE_id_target = rrc_find_free_ue_index(modid_target);
    ue_context_target_p =
      rrc_eNB_get_ue_context(
        RC.rrc[mod_id_target],
        ue_context_pP->ue_context.rnti);

    /*UE_id_target = rrc_eNB_get_next_free_UE_index(
                    mod_id_target,
                    RC.rrc[ctxt_pP->module_id]->Info.UE_list[ue_mod_idP]);  //this should return a new index*/

    if (ue_context_target_p == NULL) { // if not already in target cell
      ue_context_target_p = rrc_eNB_allocate_new_UE_context(RC.rrc[ctxt_pP->module_id]);
      ue_context_target_p->ue_id_rnti      = ue_context_pP->ue_context.rnti;             // LG: should not be the same
      ue_context_target_p->ue_context.rnti = ue_context_target_p->ue_id_rnti; // idem
      LOG_N(RRC,
            "[eNB %d] Frame %d : Emulate sending HandoverPreparationInformation msg from eNB source %d to eNB target %ld: source UE_id %x target UE_id %x source_modId: %d target_modId: %d\n",
            ctxt_pP->module_id,
            ctxt_pP->frame,
            RC.rrc[ctxt_pP->module_id]->carrier[0] /* CROUX TBC */.physCellId,
            targetPhyId,
            ue_context_pP->ue_context.rnti,
            ue_context_target_p->ue_id_rnti,
            ctxt_pP->module_id,
            mod_id_target);
      ue_context_target_p->ue_context.handover_info =
        CALLOC(1, sizeof(*(ue_context_target_p->ue_context.handover_info)));
      memcpy((void*)&ue_context_target_p->ue_context.handover_info->as_context,
             (void*)&ue_context_pP->ue_context.handover_info->as_context,
             sizeof(AS_Context_t));
      memcpy((void*)&ue_context_target_p->ue_context.handover_info->as_config,
             (void*)&ue_context_pP->ue_context.handover_info->as_config,
             sizeof(AS_Config_t));
      ue_context_target_p->ue_context.handover_info->ho_prepare = 0x00;// 0xFF;
      ue_context_target_p->ue_context.handover_info->ho_complete = 0;
      ue_context_pP->ue_context.handover_info->modid_t = mod_id_target;
      ue_context_pP->ue_context.handover_info->ueid_s  = ue_context_pP->ue_context.rnti;
      ue_context_pP->ue_context.handover_info->modid_s = ctxt_pP->module_id;
      ue_context_target_p->ue_context.handover_info->modid_t = mod_id_target;
      ue_context_target_p->ue_context.handover_info->modid_s = ctxt_pP->module_id;
      ue_context_target_p->ue_context.handover_info->ueid_t  = ue_context_target_p->ue_context.rnti;

    } else {
      LOG_E(RRC, "\nError in obtaining free UE id in target eNB %ld for handover \n", targetPhyId);
    }

  } else {
    LOG_E(RRC, "\nError in obtaining Module ID of target eNB for handover \n");
  }
}

//-----------------------------------------------------------------------------
void
rrc_eNB_process_handoverPreparationInformation(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*           const ue_context_pP
)
//-----------------------------------------------------------------------------
{
  T(T_ENB_RRC_HANDOVER_PREPARATION_INFORMATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));


  LOG_I(RRC,
        "[eNB %d] Frame %d : Logical Channel UL-DCCH, processing RRCHandoverPreparationInformation, sending RRCConnectionReconfiguration to UE %d \n",
        ctxt_pP->module_id, ctxt_pP->frame, ue_context_pP->ue_context.rnti);
  rrc_eNB_generate_RRCConnectionReconfiguration_handover(
    ctxt_pP,
    ue_context_pP,
    NULL,
    0);
}


//-----------------------------------------------------------------------------
void
check_handovers(
  protocol_ctxt_t* const ctxt_pP
)
//-----------------------------------------------------------------------------
{
  int                                 result;
  struct rrc_eNB_ue_context_s*        ue_context_p;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &RC.rrc[ctxt_pP->module_id]->rrc_ue_head) {
    ctxt_pP->rnti  = ue_context_p->ue_id_rnti;

    if (ue_context_p->ue_context.handover_info != NULL) {
      if (ue_context_p->ue_context.handover_info->ho_prepare == 0xFF) {
        LOG_D(RRC,
              "[eNB %d] Frame %d: Incoming handover detected for new UE_idx %d (source eNB %d->target eNB %d) \n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              ctxt_pP->rnti,
              ctxt_pP->module_id,
              ue_context_p->ue_context.handover_info->modid_t);
        // source eNB generates rrcconnectionreconfiguration to prepare the HO
        rrc_eNB_process_handoverPreparationInformation(
          ctxt_pP,
          ue_context_p);
        ue_context_p->ue_context.handover_info->ho_prepare = 0xF1;
      }

      if (ue_context_p->ue_context.handover_info->ho_complete == 0xF1) {
        LOG_D(RRC,
              "[eNB %d] Frame %d: handover Command received for new UE_id  %x current eNB %d target eNB: %d \n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              ctxt_pP->rnti,
              ctxt_pP->module_id,
              ue_context_p->ue_context.handover_info->modid_t);
        //rrc_eNB_process_handoverPreparationInformation(enb_mod_idP,frameP,i);
        result = pdcp_data_req(ctxt_pP,
                               SRB_FLAG_YES,
                               DCCH,
                               rrc_eNB_mui++,
                               SDU_CONFIRM_NO,
                               ue_context_p->ue_context.handover_info->size,
                               ue_context_p->ue_context.handover_info->buf,
                               PDCP_TRANSMISSION_MODE_CONTROL);
        AssertFatal(result == TRUE, "PDCP data request failed!\n");
        ue_context_p->ue_context.handover_info->ho_complete = 0xF2;
      }
    }
  }
}

// 5.3.5.4 RRCConnectionReconfiguration including the mobilityControlInfo to prepare the UE handover
//-----------------------------------------------------------------------------
void
rrc_eNB_generate_RRCConnectionReconfiguration_handover(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*           const ue_context_pP,
  uint8_t*                const nas_pdu,
  const uint32_t                nas_length
)
//-----------------------------------------------------------------------------
{
  T(T_ENB_RRC_CONNECTION_RECONFIGURATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));


  uint8_t                             buffer[RRC_BUF_SIZE];
  uint16_t                            size;
  int                                 i;
  uint8_t                             rv[2];
  uint16_t                            Idx;
  // configure SRB1/SRB2, PhysicalConfigDedicated, MAC_MainConfig for UE
  eNB_RRC_INST*                       rrc_inst = RC.rrc[ctxt_pP->module_id];
  struct PhysicalConfigDedicated**    physicalConfigDedicated = &ue_context_pP->ue_context.physicalConfigDedicated;

  struct SRB_ToAddMod                *SRB2_config;
  struct SRB_ToAddMod__rlc_Config    *SRB2_rlc_config;
  struct SRB_ToAddMod__logicalChannelConfig *SRB2_lchan_config;
  struct LogicalChannelConfig__ul_SpecificParameters *SRB2_ul_SpecificParameters;
  LogicalChannelConfig_t             *SRB1_logicalChannelConfig = NULL;
  SRB_ToAddModList_t*                 SRB_configList = ue_context_pP->ue_context.SRB_configList;    // not used in this context: may be removed
  SRB_ToAddModList_t                 *SRB_configList2;

  struct DRB_ToAddMod                *DRB_config;
  struct RLC_Config                  *DRB_rlc_config;
  struct PDCP_Config                 *DRB_pdcp_config;
  struct PDCP_Config__rlc_UM         *PDCP_rlc_UM;
  struct LogicalChannelConfig        *DRB_lchan_config;
  struct LogicalChannelConfig__ul_SpecificParameters *DRB_ul_SpecificParameters;
  DRB_ToAddModList_t                 *DRB_configList2;

  MAC_MainConfig_t                   *mac_MainConfig;
  MeasObjectToAddModList_t           *MeasObj_list;
  MeasObjectToAddMod_t               *MeasObj;
  ReportConfigToAddModList_t         *ReportConfig_list;
  ReportConfigToAddMod_t             *ReportConfig_per, *ReportConfig_A1,
                                     *ReportConfig_A2, *ReportConfig_A3, *ReportConfig_A4, *ReportConfig_A5;
  MeasIdToAddModList_t               *MeasId_list;
  MeasIdToAddMod_t                   *MeasId0, *MeasId1, *MeasId2, *MeasId3, *MeasId4, *MeasId5;
  QuantityConfig_t                   *quantityConfig;
  MobilityControlInfo_t              *mobilityInfo;
  // HandoverCommand_t handoverCommand;
  //uint8_t                             sourceModId =
  //  get_adjacent_cell_mod_id(ue_context_pP->ue_context.handover_info->as_context.reestablishmentInfo->sourcePhysCellId);
#if defined(Rel10) || defined(Rel14)
  long                               *sr_ProhibitTimer_r9;
#endif

  long                               *logicalchannelgroup, *logicalchannelgroup_drb;
  long                               *maxHARQ_Tx, *periodicBSR_Timer;

  // RSRP_Range_t *rsrp;
  struct MeasConfig__speedStatePars  *Sparams;
  CellsToAddMod_t                    *CellToAdd;
  CellsToAddModList_t                *CellsToAddModList;
  // srb 1: for HO
  struct SRB_ToAddMod                *SRB1_config;
  struct SRB_ToAddMod__rlc_Config    *SRB1_rlc_config;
  struct SRB_ToAddMod__logicalChannelConfig *SRB1_lchan_config;
  struct LogicalChannelConfig__ul_SpecificParameters *SRB1_ul_SpecificParameters;
  // phy config dedicated
  PhysicalConfigDedicated_t          *physicalConfigDedicated2;
  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList;
  protocol_ctxt_t                     ctxt;

  LOG_D(RRC, "[eNB %d] Frame %d: handover preparation: get the newSourceUEIdentity (C-RNTI): ",
        ctxt_pP->module_id, ctxt_pP->frame);

  for (i = 0; i < 2; i++) {
    rv[i] = taus() & 0xff;
    LOG_D(RRC, " %x.", rv[i]);
  }

  LOG_D(RRC, "[eNB %d] Frame %d : handover reparation: add target eNB SRB1 and PHYConfigDedicated reconfiguration\n",
        ctxt_pP->module_id, ctxt_pP->frame);
  // 1st: reconfigure SRB
  SRB_configList2 = CALLOC(1, sizeof(*SRB_configList));
  SRB1_config = CALLOC(1, sizeof(*SRB1_config));
  SRB1_config->srb_Identity = 1;
  SRB1_rlc_config = CALLOC(1, sizeof(*SRB1_rlc_config));
  SRB1_config->rlc_Config = SRB1_rlc_config;

  SRB1_rlc_config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB1_rlc_config->choice.explicitValue.present = RLC_Config_PR_am;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms15;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU = PollPDU_p8;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte = PollByte_kB1000;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t16;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms10;

  SRB1_lchan_config = CALLOC(1, sizeof(*SRB1_lchan_config));
  SRB1_config->logicalChannelConfig = SRB1_lchan_config;

  SRB1_lchan_config->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
  SRB1_ul_SpecificParameters = CALLOC(1, sizeof(*SRB1_ul_SpecificParameters));

  SRB1_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB1_ul_SpecificParameters;

  SRB1_ul_SpecificParameters->priority = 1;

  //assign_enum(&SRB1_ul_SpecificParameters->prioritisedBitRate,LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity);
  SRB1_ul_SpecificParameters->prioritisedBitRate =
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;

  //assign_enum(&SRB1_ul_SpecificParameters->bucketSizeDuration,LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50);
  SRB1_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  logicalchannelgroup = CALLOC(1, sizeof(long));
  *logicalchannelgroup = 0;
  SRB1_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;

  ASN_SEQUENCE_ADD(&SRB_configList2->list, SRB1_config);

  //2nd: now reconfigure phy config dedicated
  physicalConfigDedicated2 = CALLOC(1, sizeof(*physicalConfigDedicated2));
  *physicalConfigDedicated = physicalConfigDedicated2;

  physicalConfigDedicated2->pdsch_ConfigDedicated =
    CALLOC(1, sizeof(*physicalConfigDedicated2->pdsch_ConfigDedicated));
  physicalConfigDedicated2->pucch_ConfigDedicated =
    CALLOC(1, sizeof(*physicalConfigDedicated2->pucch_ConfigDedicated));
  physicalConfigDedicated2->pusch_ConfigDedicated =
    CALLOC(1, sizeof(*physicalConfigDedicated2->pusch_ConfigDedicated));
  physicalConfigDedicated2->uplinkPowerControlDedicated =
    CALLOC(1, sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated));
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH =
    CALLOC(1, sizeof(*physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH));
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH =
    CALLOC(1, sizeof(*physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH));
  physicalConfigDedicated2->cqi_ReportConfig = NULL;  //CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig));
  physicalConfigDedicated2->soundingRS_UL_ConfigDedicated = CALLOC(1,sizeof(*physicalConfigDedicated2->soundingRS_UL_ConfigDedicated));
  physicalConfigDedicated2->antennaInfo = CALLOC(1, sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->schedulingRequestConfig =
    CALLOC(1, sizeof(*physicalConfigDedicated2->schedulingRequestConfig));
  // PDSCH
  //assign_enum(&physicalConfigDedicated2->pdsch_ConfigDedicated->p_a,
  //          PDSCH_ConfigDedicated__p_a_dB0);
  physicalConfigDedicated2->pdsch_ConfigDedicated->p_a = PDSCH_ConfigDedicated__p_a_dB0;

  // PUCCH
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.present =
    PUCCH_ConfigDedicated__ackNackRepetition_PR_release;
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.choice.release = 0;
  physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode = NULL;    //PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;

  // Pusch_config_dedicated
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_ACK_Index = 0;  // 2.00
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_RI_Index = 0;   // 1.25
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_CQI_Index = 8;  // 2.25

  // UplinkPowerControlDedicated
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUSCH = 0; // 0 dB
  //assign_enum(&physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled,
  // UplinkPowerControlDedicated__deltaMCS_Enabled_en1);
  physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled =
    UplinkPowerControlDedicated__deltaMCS_Enabled_en1;
  physicalConfigDedicated2->uplinkPowerControlDedicated->accumulationEnabled = 1; // should be TRUE in order to have 0dB power offset
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUCCH = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->pSRS_Offset = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient =
    CALLOC(1, sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient));
  //  assign_enum(physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient,FilterCoefficient_fc4); // fc4 dB
  *physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient = FilterCoefficient_fc4;  // fc4 dB

  // TPC-PDCCH-Config
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->present = TPC_PDCCH_Config_PR_setup;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_Index.present = TPC_Index_PR_indexOfFormat3;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_Index.choice.indexOfFormat3 = 1;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf = CALLOC(1, 2);
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.size = 2;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf[0] = 0x12;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf[1] = 0x34 + ue_context_pP->local_uid;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.bits_unused = 0;

  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->present = TPC_PDCCH_Config_PR_setup;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_Index.present = TPC_Index_PR_indexOfFormat3;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_Index.choice.indexOfFormat3 = 1;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf = CALLOC(1, 2);
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.size = 2;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf[0] = 0x22;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf[1] = 0x34 + ue_context_pP->local_uid;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.bits_unused = 0;

  // CQI ReportConfig
  /*
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic=CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic));
     assign_enum(physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic,
     CQI_ReportConfig__cqi_ReportModeAperiodic_rm30); // HLC CQI, no PMI
     physicalConfigDedicated2->cqi_ReportConfig->nomPDSCH_RS_EPRE_Offset = 0; // 0 dB
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic=CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic));
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->present =  CQI_ReportPeriodic_PR_setup;
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex = 0;  // n2_pucch
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_pmi_ConfigIndex = 0;  // Icqi/pmi
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_FormatIndicatorPeriodic.present = CQI_ReportPeriodic__setup__cqi_FormatIndicatorPeriodic_PR_subbandCQI;  // subband CQI
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_FormatIndicatorPeriodic.choice.subbandCQI.k=4;

     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex=NULL;
     physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.simultaneousAckNackAndCQI=0;
   */

  //soundingRS-UL-ConfigDedicated
  /*
     physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->present = SoundingRS_UL_ConfigDedicated_PR_setup;
     assign_enum(&physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth,
     SoundingRS_UL_ConfigDedicated__setup__srs_Bandwidth_bw0);
     assign_enum(&physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth,
     SoundingRS_UL_ConfigDedicated__setup__srs_HoppingBandwidth_hbw0);
     physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition=0;
     physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.duration=1;
     physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex=1;
     physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb=0;
     assign_enum(&physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift,
     SoundingRS_UL_ConfigDedicated__setup__cyclicShift_cs0);
   */

  //AntennaInfoDedicated
  physicalConfigDedicated2->antennaInfo = CALLOC(1, sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->antennaInfo->present = PhysicalConfigDedicated__antennaInfo_PR_explicitValue;
  //assign_enum(&physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode,
  //     AntennaInfoDedicated__transmissionMode_tm2);
  /*
     switch (transmission_mode){
     case 1:
     physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm1;
     break;
     case 2:
     physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm2;
     break;
     case 4:
     physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm4;
     break;
     case 5:
     physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm5;
     break;
     case 6:
     physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm6;
     break;
     }
   */
  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.present =
    AntennaInfoDedicated__ue_TransmitAntennaSelection_PR_release;
  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.release = 0;

  // SchedulingRequestConfig
  physicalConfigDedicated2->schedulingRequestConfig->present = SchedulingRequestConfig_PR_setup;
  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = ue_context_pP->local_uid;

  if (rrc_inst->carrier[0].sib1->tdd_Config==NULL) {  // FD
    physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 5 + (ue_context_pP->local_uid %
        10);   // Isr = 5 (every 10 subframes, offset=2+UE_id mod3)
  } else {
    switch (rrc_inst->carrier[0].sib1->tdd_Config->subframeAssignment) {
    case 1:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7 + (ue_context_pP->local_uid & 1) + ((
            ue_context_pP->local_uid & 3) >> 1) * 5;    // Isr = 5 (every 10 subframes, offset=2 for UE0, 3 for UE1, 7 for UE2, 8 for UE3 , 2 for UE4 etc..)
      break;

    case 3:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7 + (ue_context_pP->local_uid %
          3);    // Isr = 5 (every 10 subframes, offset=2 for UE0, 3 for UE1, 3 for UE2, 2 for UE3 , etc..)
      break;

    case 4:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7 + (ue_context_pP->local_uid &
          1);    // Isr = 5 (every 10 subframes, offset=2 for UE0, 3 for UE1, 3 for UE2, 2 for UE3 , etc..)
      break;

    default:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7; // Isr = 5 (every 10 subframes, offset=2 for all UE0 etc..)
      break;
    }
  }

  //  assign_enum(&physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax,
  //SchedulingRequestConfig__setup__dsr_TransMax_n4);
  //  assign_enum(&physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax = SchedulingRequestConfig__setup__dsr_TransMax_n4;
  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax =
    SchedulingRequestConfig__setup__dsr_TransMax_n4;

  LOG_D(RRC,
        "handover_config [FRAME %05d][RRC_eNB][MOD %02d][][--- MAC_CONFIG_REQ  (SRB1 UE %x) --->][MAC_eNB][MOD %02d][]\n",
        ctxt_pP->frame, ctxt_pP->module_id, ue_context_pP->ue_context.rnti, ctxt_pP->module_id);
  rrc_mac_config_req_eNB(
			 ctxt_pP->module_id,
			 ue_context_pP->ue_context.primaryCC_id,
			 0,0,0,0,0,
#ifdef Rel14 
0,
#endif 
			 ue_context_pP->ue_context.rnti,
			 (BCCH_BCH_Message_t *) NULL,
			 (RadioResourceConfigCommonSIB_t*) NULL,
#ifdef Rel14
			 (RadioResourceConfigCommonSIB_t*) NULL,
#endif
			 ue_context_pP->ue_context.physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
			 (SCellToAddMod_r10_t *)NULL,
			 //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
			 (MeasObjectToAddMod_t **) NULL,
			 ue_context_pP->ue_context.mac_MainConfig,
			 1,
			 SRB1_logicalChannelConfig,
			 ue_context_pP->ue_context.measGapConfig,
			 (TDD_Config_t*) NULL,
			 (MobilityControlInfo_t*) NULL,
			 (SchedulingInfoList_t*) NULL,
			 0,
			 NULL,
			 NULL,
			 (MBSFN_SubframeConfigList_t *) NULL
#if defined(Rel10) || defined(Rel14)
			 , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#   ifdef Rel14
			 ,
			 (SystemInformationBlockType1_v1310_IEs_t *)NULL
#   endif
			 );
  
  // Configure target eNB SRB2
  /// SRB2
  SRB2_config = CALLOC(1, sizeof(*SRB2_config));
  SRB_configList2 = CALLOC(1, sizeof(*SRB_configList2));
  memset(SRB_configList2, 0, sizeof(*SRB_configList2));

  SRB2_config->srb_Identity = 2;
  SRB2_rlc_config = CALLOC(1, sizeof(*SRB2_rlc_config));
  SRB2_config->rlc_Config = SRB2_rlc_config;

  SRB2_rlc_config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB2_rlc_config->choice.explicitValue.present = RLC_Config_PR_am;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms15;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU = PollPDU_p8;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte = PollByte_kB1000;
  SRB2_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;
  SRB2_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering = T_Reordering_ms35;
  SRB2_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms10;

  SRB2_lchan_config = CALLOC(1, sizeof(*SRB2_lchan_config));
  SRB2_config->logicalChannelConfig = SRB2_lchan_config;

  SRB2_lchan_config->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;

  SRB2_ul_SpecificParameters = CALLOC(1, sizeof(*SRB2_ul_SpecificParameters));

  SRB2_ul_SpecificParameters->priority = 1;
  SRB2_ul_SpecificParameters->prioritisedBitRate =
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  SRB2_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  // LCG for CCCH and DCCH is 0 as defined in 36331
  logicalchannelgroup = CALLOC(1, sizeof(long));
  *logicalchannelgroup = 0;

  SRB2_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;
  SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB2_ul_SpecificParameters;
  ASN_SEQUENCE_ADD(&SRB_configList->list, SRB2_config);
  ASN_SEQUENCE_ADD(&SRB_configList2->list, SRB2_config);

  // Configure target eNB DRB
  DRB_configList2 = CALLOC(1, sizeof(*DRB_configList2));
  /// DRB
  DRB_config = CALLOC(1, sizeof(*DRB_config));

  //DRB_config->drb_Identity = (DRB_Identity_t) 1; //allowed values 1..32
  // NN: this is the 1st DRB for this ue, so set it to 1
  DRB_config->drb_Identity = (DRB_Identity_t) 1;  // (ue_mod_idP+1); //allowed values 1..32
  DRB_config->logicalChannelIdentity = CALLOC(1, sizeof(long));
  *(DRB_config->logicalChannelIdentity) = (long)3;
  DRB_rlc_config = CALLOC(1, sizeof(*DRB_rlc_config));
  DRB_config->rlc_Config = DRB_rlc_config;
  DRB_rlc_config->present = RLC_Config_PR_um_Bi_Directional;
  DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = SN_FieldLength_size10;
  DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = T_Reordering_ms35;

  DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
  DRB_config->pdcp_Config = DRB_pdcp_config;
  DRB_pdcp_config->discardTimer = NULL;
  DRB_pdcp_config->rlc_AM = NULL;
  PDCP_rlc_UM = CALLOC(1, sizeof(*PDCP_rlc_UM));
  DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
  PDCP_rlc_UM->pdcp_SN_Size = PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
  DRB_pdcp_config->headerCompression.present = PDCP_Config__headerCompression_PR_notUsed;

  DRB_lchan_config = CALLOC(1, sizeof(*DRB_lchan_config));
  DRB_config->logicalChannelConfig = DRB_lchan_config;
  DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
  DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;

  DRB_ul_SpecificParameters->priority = 2;    // lower priority than srb1, srb2
  DRB_ul_SpecificParameters->prioritisedBitRate =
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  DRB_ul_SpecificParameters->bucketSizeDuration =
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  // LCG for DTCH can take the value from 1 to 3 as defined in 36331: normally controlled by upper layers (like RRM)
  logicalchannelgroup_drb = CALLOC(1, sizeof(long));
  *logicalchannelgroup_drb = 1;
  DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;

  ASN_SEQUENCE_ADD(&DRB_configList2->list, DRB_config);

  mac_MainConfig = CALLOC(1, sizeof(*mac_MainConfig));
  ue_context_pP->ue_context.mac_MainConfig = mac_MainConfig;

  mac_MainConfig->ul_SCH_Config = CALLOC(1, sizeof(*mac_MainConfig->ul_SCH_Config));

  maxHARQ_Tx = CALLOC(1, sizeof(long));
  *maxHARQ_Tx = MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  mac_MainConfig->ul_SCH_Config->maxHARQ_Tx = maxHARQ_Tx;

  periodicBSR_Timer = CALLOC(1, sizeof(long));
  *periodicBSR_Timer = PeriodicBSR_Timer_r12_sf64;
  mac_MainConfig->ul_SCH_Config->periodicBSR_Timer = periodicBSR_Timer;

  mac_MainConfig->ul_SCH_Config->retxBSR_Timer = RetxBSR_Timer_r12_sf320;

  mac_MainConfig->ul_SCH_Config->ttiBundling = 0; // FALSE

  mac_MainConfig->drx_Config = NULL;

  mac_MainConfig->phr_Config = CALLOC(1, sizeof(*mac_MainConfig->phr_Config));

  mac_MainConfig->phr_Config->present = MAC_MainConfig__phr_Config_PR_setup;
  mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer = MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20; // sf20 = 20 subframes

  mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer = MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20; // sf20 = 20 subframes

  mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange = MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;  // Value dB1 =1 dB, dB3 = 3 dB

#if defined(Rel10) || defined(Rel14)
  sr_ProhibitTimer_r9 = CALLOC(1, sizeof(long));
  *sr_ProhibitTimer_r9 = 0;   // SR tx on PUCCH, Value in number of SR period(s). Value 0 = no timer for SR, Value 2= 2*SR
  mac_MainConfig->ext1 = CALLOC(1, sizeof(struct MAC_MainConfig__ext1));
  mac_MainConfig->ext1->sr_ProhibitTimer_r9 = sr_ProhibitTimer_r9;
  //sps_RA_ConfigList_rlola = NULL;
#endif
  // Measurement ID list
  MeasId_list = CALLOC(1, sizeof(*MeasId_list));
  memset((void *)MeasId_list, 0, sizeof(*MeasId_list));

  MeasId0 = CALLOC(1, sizeof(*MeasId0));
  MeasId0->measId = 1;
  MeasId0->measObjectId = 1;
  MeasId0->reportConfigId = 1;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId0);

  MeasId1 = CALLOC(1, sizeof(*MeasId1));
  MeasId1->measId = 2;
  MeasId1->measObjectId = 1;
  MeasId1->reportConfigId = 2;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId1);

  MeasId2 = CALLOC(1, sizeof(*MeasId2));
  MeasId2->measId = 3;
  MeasId2->measObjectId = 1;
  MeasId2->reportConfigId = 3;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId2);

  MeasId3 = CALLOC(1, sizeof(*MeasId3));
  MeasId3->measId = 4;
  MeasId3->measObjectId = 1;
  MeasId3->reportConfigId = 4;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId3);

  MeasId4 = CALLOC(1, sizeof(*MeasId4));
  MeasId4->measId = 5;
  MeasId4->measObjectId = 1;
  MeasId4->reportConfigId = 5;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId4);

  MeasId5 = CALLOC(1, sizeof(*MeasId5));
  MeasId5->measId = 6;
  MeasId5->measObjectId = 1;
  MeasId5->reportConfigId = 6;
  ASN_SEQUENCE_ADD(&MeasId_list->list, MeasId5);

  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measIdToAddModList = MeasId_list;

  // Add one EUTRA Measurement Object
  MeasObj_list = CALLOC(1, sizeof(*MeasObj_list));
  memset((void *)MeasObj_list, 0, sizeof(*MeasObj_list));

  // Configure MeasObject

  MeasObj = CALLOC(1, sizeof(*MeasObj));
  memset((void *)MeasObj, 0, sizeof(*MeasObj));

  MeasObj->measObjectId = 1;
  MeasObj->measObject.present = MeasObjectToAddMod__measObject_PR_measObjectEUTRA;
  MeasObj->measObject.choice.measObjectEUTRA.carrierFreq = 36090;
  MeasObj->measObject.choice.measObjectEUTRA.allowedMeasBandwidth = AllowedMeasBandwidth_mbw25;
  MeasObj->measObject.choice.measObjectEUTRA.presenceAntennaPort1 = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf = CALLOC(1, sizeof(uint8_t));
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf[0] = 0;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.size = 1;
  MeasObj->measObject.choice.measObjectEUTRA.neighCellConfig.bits_unused = 6;
  MeasObj->measObject.choice.measObjectEUTRA.offsetFreq = NULL;   // Default is 15 or 0dB

  MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList =
    (CellsToAddModList_t *) CALLOC(1, sizeof(*CellsToAddModList));
  CellsToAddModList = MeasObj->measObject.choice.measObjectEUTRA.cellsToAddModList;

  // Add adjacent cell lists (6 per eNB)
  for (i = 0; i < 6; i++) {
    CellToAdd = (CellsToAddMod_t *) CALLOC(1, sizeof(*CellToAdd));
    CellToAdd->cellIndex = i + 1;
    CellToAdd->physCellId = get_adjacent_cell_id(ctxt_pP->module_id, i);
    CellToAdd->cellIndividualOffset = Q_OffsetRange_dB0;

    ASN_SEQUENCE_ADD(&CellsToAddModList->list, CellToAdd);
  }

  ASN_SEQUENCE_ADD(&MeasObj_list->list, MeasObj);
  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measObjectToAddModList = MeasObj_list;

  // Report Configurations for periodical, A1-A5 events
  ReportConfig_list = CALLOC(1, sizeof(*ReportConfig_list));

  ReportConfig_per = CALLOC(1, sizeof(*ReportConfig_per));

  ReportConfig_A1 = CALLOC(1, sizeof(*ReportConfig_A1));

  ReportConfig_A2 = CALLOC(1, sizeof(*ReportConfig_A2));

  ReportConfig_A3 = CALLOC(1, sizeof(*ReportConfig_A3));

  ReportConfig_A4 = CALLOC(1, sizeof(*ReportConfig_A4));

  ReportConfig_A5 = CALLOC(1, sizeof(*ReportConfig_A5));

  ReportConfig_per->reportConfigId = 1;
  ReportConfig_per->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_periodical;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerType.choice.periodical.purpose =
    ReportConfigEUTRA__triggerType__periodical__purpose_reportStrongestCells;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_per->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_per);

  ReportConfig_A1->reportConfigId = 2;
  ReportConfig_A1->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA1;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA1.
  a1_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA1.
  a1_Threshold.choice.threshold_RSRP = 10;

  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A1->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A1);

  ReportConfig_A2->reportConfigId = 3;
  ReportConfig_A2->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA2;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA2.
  a2_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA2.
  a2_Threshold.choice.threshold_RSRP = 10;

  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A2->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A2);

  ReportConfig_A3->reportConfigId = 4;
  ReportConfig_A3->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA3;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA3.a3_Offset =
    10;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
  eventA3.reportOnLeave = 1;

  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A3->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A3);

  ReportConfig_A4->reportConfigId = 5;
  ReportConfig_A4->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA4;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA4.
  a4_Threshold.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA4.
  a4_Threshold.choice.threshold_RSRP = 10;

  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A4->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A4);

  ReportConfig_A5->reportConfigId = 6;
  ReportConfig_A5->reportConfig.present = ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.present =
    ReportConfigEUTRA__triggerType_PR_event;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present =
    ReportConfigEUTRA__triggerType__event__eventId_PR_eventA5;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
  eventA5.a5_Threshold1.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
  eventA5.a5_Threshold2.present = ThresholdEUTRA_PR_threshold_RSRP;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
  eventA5.a5_Threshold1.choice.threshold_RSRP = 10;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.
  eventA5.a5_Threshold2.choice.threshold_RSRP = 10;

  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.triggerQuantity = ReportConfigEUTRA__triggerQuantity_rsrp;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportQuantity = ReportConfigEUTRA__reportQuantity_both;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.maxReportCells = 2;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportInterval = ReportInterval_ms120;
  ReportConfig_A5->reportConfig.choice.reportConfigEUTRA.reportAmount = ReportConfigEUTRA__reportAmount_infinity;

  ASN_SEQUENCE_ADD(&ReportConfig_list->list, ReportConfig_A5);

  Sparams = CALLOC(1, sizeof(*Sparams));
  Sparams->present = MeasConfig__speedStatePars_PR_setup;
  Sparams->choice.setup.timeToTrigger_SF.sf_High = SpeedStateScaleFactors__sf_Medium_oDot75;
  Sparams->choice.setup.timeToTrigger_SF.sf_Medium = SpeedStateScaleFactors__sf_High_oDot5;
  Sparams->choice.setup.mobilityStateParameters.n_CellChangeHigh = 10;
  Sparams->choice.setup.mobilityStateParameters.n_CellChangeMedium = 5;
  Sparams->choice.setup.mobilityStateParameters.t_Evaluation = MobilityStateParameters__t_Evaluation_s60;
  Sparams->choice.setup.mobilityStateParameters.t_HystNormal = MobilityStateParameters__t_HystNormal_s120;

  quantityConfig = CALLOC(1, sizeof(*quantityConfig));
  memset((void *)quantityConfig, 0, sizeof(*quantityConfig));
  quantityConfig->quantityConfigEUTRA = CALLOC(1, sizeof(*quantityConfig->quantityConfigEUTRA));
  memset((void *)quantityConfig->quantityConfigEUTRA, 0, sizeof(*quantityConfig->quantityConfigEUTRA));
  quantityConfig->quantityConfigCDMA2000 = NULL;
  quantityConfig->quantityConfigGERAN = NULL;
  quantityConfig->quantityConfigUTRA = NULL;
  quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP =
    CALLOC(1, sizeof(*quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP));
  quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ =
    CALLOC(1, sizeof(*quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ));
  *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRP = FilterCoefficient_fc4;
  *quantityConfig->quantityConfigEUTRA->filterCoefficientRSRQ = FilterCoefficient_fc4;

  /* mobilityinfo  */

  mobilityInfo = CALLOC(1, sizeof(*mobilityInfo));
  memset((void *)mobilityInfo, 0, sizeof(*mobilityInfo));
  mobilityInfo->targetPhysCellId =
    (PhysCellId_t) two_tier_hexagonal_cellIds[ue_context_pP->ue_context.handover_info->modid_t];
  LOG_D(RRC, "[eNB %d] Frame %d: handover preparation: targetPhysCellId: %ld mod_id: %d ue: %x \n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        mobilityInfo->targetPhysCellId,
        ctxt_pP->module_id,
        ue_context_pP->ue_context.rnti);

  mobilityInfo->additionalSpectrumEmission = CALLOC(1, sizeof(*mobilityInfo->additionalSpectrumEmission));
  *mobilityInfo->additionalSpectrumEmission = 1;  //Check this value!

  mobilityInfo->t304 = MobilityControlInfo__t304_ms50;    // need to configure an appropriate value here

  // New UE Identity (C-RNTI) to identify an UE uniquely in a cell
  mobilityInfo->newUE_Identity.size = 2;
  mobilityInfo->newUE_Identity.bits_unused = 0;
  mobilityInfo->newUE_Identity.buf = rv;
  mobilityInfo->newUE_Identity.buf[0] = rv[0];
  mobilityInfo->newUE_Identity.buf[1] = rv[1];

  //memset((void *)&mobilityInfo->radioResourceConfigCommon,(void *)&rrc_inst->sib2->radioResourceConfigCommon,sizeof(RadioResourceConfigCommon_t));
  //memset((void *)&mobilityInfo->radioResourceConfigCommon,0,sizeof(RadioResourceConfigCommon_t));

  // Configuring radioResourceConfigCommon
  mobilityInfo->radioResourceConfigCommon.rach_ConfigCommon =
    CALLOC(1, sizeof(*mobilityInfo->radioResourceConfigCommon.rach_ConfigCommon));
  memcpy((void *)mobilityInfo->radioResourceConfigCommon.rach_ConfigCommon,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.rach_ConfigCommon, sizeof(RACH_ConfigCommon_t));
  mobilityInfo->radioResourceConfigCommon.prach_Config.prach_ConfigInfo =
    CALLOC(1, sizeof(*mobilityInfo->radioResourceConfigCommon.prach_Config.prach_ConfigInfo));
  memcpy((void *)mobilityInfo->radioResourceConfigCommon.prach_Config.prach_ConfigInfo,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.prach_Config.prach_ConfigInfo,
         sizeof(PRACH_ConfigInfo_t));

  mobilityInfo->radioResourceConfigCommon.prach_Config.rootSequenceIndex =
    rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.prach_Config.rootSequenceIndex;
  mobilityInfo->radioResourceConfigCommon.pdsch_ConfigCommon =
    CALLOC(1, sizeof(*mobilityInfo->radioResourceConfigCommon.pdsch_ConfigCommon));
  memcpy((void *)mobilityInfo->radioResourceConfigCommon.pdsch_ConfigCommon,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.pdsch_ConfigCommon, sizeof(PDSCH_ConfigCommon_t));
  memcpy((void *)&mobilityInfo->radioResourceConfigCommon.pusch_ConfigCommon,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.pusch_ConfigCommon, sizeof(PUSCH_ConfigCommon_t));
  mobilityInfo->radioResourceConfigCommon.phich_Config = NULL;
  mobilityInfo->radioResourceConfigCommon.pucch_ConfigCommon =
    CALLOC(1, sizeof(*mobilityInfo->radioResourceConfigCommon.pucch_ConfigCommon));
  memcpy((void *)mobilityInfo->radioResourceConfigCommon.pucch_ConfigCommon,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.pucch_ConfigCommon, sizeof(PUCCH_ConfigCommon_t));
  mobilityInfo->radioResourceConfigCommon.soundingRS_UL_ConfigCommon =
    CALLOC(1, sizeof(*mobilityInfo->radioResourceConfigCommon.soundingRS_UL_ConfigCommon));
  memcpy((void *)mobilityInfo->radioResourceConfigCommon.soundingRS_UL_ConfigCommon,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon,
         sizeof(SoundingRS_UL_ConfigCommon_t));
  mobilityInfo->radioResourceConfigCommon.uplinkPowerControlCommon =
    CALLOC(1, sizeof(*mobilityInfo->radioResourceConfigCommon.uplinkPowerControlCommon));
  memcpy((void *)mobilityInfo->radioResourceConfigCommon.uplinkPowerControlCommon,
         (void *)&rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.uplinkPowerControlCommon,
         sizeof(UplinkPowerControlCommon_t));
  mobilityInfo->radioResourceConfigCommon.antennaInfoCommon = NULL;
  mobilityInfo->radioResourceConfigCommon.p_Max = NULL;   // CALLOC(1,sizeof(*mobilityInfo->radioResourceConfigCommon.p_Max));
  //memcpy((void *)mobilityInfo->radioResourceConfigCommon.p_Max,(void *)rrc_inst->sib1->p_Max,sizeof(P_Max_t));
  mobilityInfo->radioResourceConfigCommon.tdd_Config = NULL;  //CALLOC(1,sizeof(TDD_Config_t));
  //memcpy((void *)mobilityInfo->radioResourceConfigCommon.tdd_Config,(void *)rrc_inst->sib1->tdd_Config,sizeof(TDD_Config_t));
  mobilityInfo->radioResourceConfigCommon.ul_CyclicPrefixLength =
    rrc_inst->carrier[0] /* CROUX TBC */.sib2->radioResourceConfigCommon.ul_CyclicPrefixLength;
  //End of configuration of radioResourceConfigCommon

  mobilityInfo->carrierFreq = CALLOC(1, sizeof(*mobilityInfo->carrierFreq));  //CALLOC(1,sizeof(CarrierFreqEUTRA_t)); 36090
  mobilityInfo->carrierFreq->dl_CarrierFreq = 36090;
  mobilityInfo->carrierFreq->ul_CarrierFreq = NULL;

  mobilityInfo->carrierBandwidth = CALLOC(1, sizeof(
      *mobilityInfo->carrierBandwidth));    //CALLOC(1,sizeof(struct CarrierBandwidthEUTRA));  AllowedMeasBandwidth_mbw25
  mobilityInfo->carrierBandwidth->dl_Bandwidth = CarrierBandwidthEUTRA__dl_Bandwidth_n25;
  mobilityInfo->carrierBandwidth->ul_Bandwidth = NULL;
  mobilityInfo->rach_ConfigDedicated = NULL;

  // store the srb and drb list for ho management, mainly in case of failure

  memcpy(ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.srb_ToAddModList,
         (void*)SRB_configList2,
         sizeof(SRB_ToAddModList_t));
  memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.drb_ToAddModList,
         (void*)DRB_configList2,
         sizeof(DRB_ToAddModList_t));
  ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.drb_ToReleaseList = NULL;
  memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.mac_MainConfig,
         (void*)mac_MainConfig,
         sizeof(MAC_MainConfig_t));
  memcpy((void*)ue_context_pP->ue_context.handover_info->as_config.sourceRadioResourceConfig.physicalConfigDedicated,
         (void*)ue_context_pP->ue_context.physicalConfigDedicated,
         sizeof(PhysicalConfigDedicated_t));
  /*    memcpy((void *)rrc_inst->handover_info[ue_mod_idP]->as_config.sourceRadioResourceConfig.sps_Config,
     (void *)rrc_inst->sps_Config[ue_mod_idP],
     sizeof(SPS_Config_t));
   */
  LOG_I(RRC, "[eNB %d] Frame %d: adding new UE\n",
        ctxt_pP->module_id, ctxt_pP->frame);
  //Idx = (ue_mod_idP * NB_RB_MAX) + DCCH;
  Idx = DCCH;
  // SRB1
  ue_context_pP->ue_context.Srb1.Active = 1;
  ue_context_pP->ue_context.Srb1.Srb_info.Srb_id = Idx;
  memcpy(&ue_context_pP->ue_context.Srb1.Srb_info.Lchan_desc[0], &DCCH_LCHAN_DESC, LCHAN_DESC_SIZE);
  memcpy(&ue_context_pP->ue_context.Srb1.Srb_info.Lchan_desc[1], &DCCH_LCHAN_DESC, LCHAN_DESC_SIZE);

  // SRB2 
  ue_context_pP->ue_context.Srb2.Active = 1;
  ue_context_pP->ue_context.Srb2.Srb_info.Srb_id = Idx;
  memcpy(&ue_context_pP->ue_context.Srb2.Srb_info.Lchan_desc[0], &DCCH_LCHAN_DESC, LCHAN_DESC_SIZE);
  memcpy(&ue_context_pP->ue_context.Srb2.Srb_info.Lchan_desc[1], &DCCH_LCHAN_DESC, LCHAN_DESC_SIZE);

  LOG_I(RRC, "[eNB %d] CALLING RLC CONFIG SRB1 (rbid %d) for UE %x\n",
        ctxt_pP->module_id, Idx, ue_context_pP->ue_context.rnti);

  //      rrc_pdcp_config_req (enb_mod_idP, frameP, 1, CONFIG_ACTION_ADD, idx, UNDEF_SECURITY_MODE);
  //      rrc_rlc_config_req(enb_mod_idP,frameP,1,CONFIG_ACTION_ADD,Idx,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);

  rrc_pdcp_config_asn1_req(&ctxt,
                           ue_context_pP->ue_context.SRB_configList,
                           (DRB_ToAddModList_t *) NULL, (DRB_ToReleaseList_t *) NULL, 0xff, NULL, NULL, NULL
#if defined(Rel10) || defined(Rel14)
                           , (PMCH_InfoList_r9_t *) NULL
#endif
                           ,NULL);

  rrc_rlc_config_asn1_req(&ctxt,
                          ue_context_pP->ue_context.SRB_configList,
                          (DRB_ToAddModList_t *) NULL, (DRB_ToReleaseList_t *) NULL
#if defined(Rel10) || defined(Rel14)
                          , (PMCH_InfoList_r9_t *) NULL
#endif
                         );

  /* Initialize NAS list */
  dedicatedInfoNASList = NULL;

  //  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->reportConfigToAddModList = ReportConfig_list;
  memset(buffer, 0, RRC_BUF_SIZE);

  size = do_RRCConnectionReconfiguration(
           ctxt_pP,
           buffer,
           rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id),   //Transaction_id,
           SRB_configList2,
           DRB_configList2,
           NULL,  // DRB2_list,
           NULL,    //*sps_Config,
           ue_context_pP->ue_context.physicalConfigDedicated,
           MeasObj_list,
           ReportConfig_list,
           NULL,    //quantityConfig,
           MeasId_list,
           mac_MainConfig,
           NULL,
           mobilityInfo,
           Sparams,
           NULL,
           NULL,
           dedicatedInfoNASList
#if defined(Rel10) || defined(Rel14)
           , NULL   // SCellToAddMod_r10_t
#endif
         );

  LOG_I(RRC,
        "[eNB %d] Frame %d, Logical Channel DL-DCCH, Generate RRCConnectionReconfiguration for handover (bytes %d, UE rnti %x)\n",
        ctxt_pP->module_id, ctxt_pP->frame, size, ue_context_pP->ue_context.rnti);
  // to be updated if needed
  /*if (RC.rrc[ctxt_pP->module_id]->SRB1_config[ue_mod_idP]->logicalChannelConfig) {
     if (RC.rrc[ctxt_pP->module_id]->SRB1_config[ue_mod_idP]->logicalChannelConfig->present == SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
     SRB1_logicalChannelConfig = &RC.rrc[ctxt_pP->module_id]->SRB1_config[ue_mod_idP]->logicalChannelConfig->choice.explicitValue;
     }
     else {
     SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
     }
     }
     else {
     SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
     }
   */

  LOG_D(RRC,
        "[FRAME %05d][RRC_eNB][MOD %02d][][--- PDCP_DATA_REQ/%d Bytes (rrcConnectionReconfiguration_handover to UE %x MUI %d) --->][PDCP][MOD %02d][RB %02d]\n",
        ctxt_pP->frame, ctxt_pP->module_id, size, ue_context_pP->ue_context.rnti, rrc_eNB_mui, ctxt_pP->module_id, DCCH);
  //rrc_rlc_data_req(ctxt_pP->module_id,frameP, 1,(ue_mod_idP*NB_RB_MAX)+DCCH,rrc_eNB_mui++,0,size,(char*)buffer);
  //pdcp_data_req (ctxt_pP->module_id, frameP, 1, (ue_mod_idP * NB_RB_MAX) + DCCH,rrc_eNB_mui++, 0, size, (char *) buffer, 1);

  rrc_mac_config_req_eNB(
			 ctxt_pP->module_id,
			 ue_context_pP->ue_context.primaryCC_id,
			 0,0,0,0,0,
#ifdef Rel14 
			 0,
#endif
			 ue_context_pP->ue_context.rnti,
			 (BCCH_BCH_Message_t *) NULL,
			 (RadioResourceConfigCommonSIB_t *) NULL,
#ifdef Rel14
			 (RadioResourceConfigCommonSIB_t *) NULL,
#endif
			 ue_context_pP->ue_context.physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
			 (SCellToAddMod_r10_t *)NULL,
			 //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
			 (MeasObjectToAddMod_t **) NULL,
			 ue_context_pP->ue_context.mac_MainConfig,
			 1,
			 SRB1_logicalChannelConfig,
			 ue_context_pP->ue_context.measGapConfig,
			 (TDD_Config_t *) NULL,
			 (MobilityControlInfo_t *) mobilityInfo,
			 (SchedulingInfoList_t *) NULL, 0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
#if defined(Rel10) || defined(Rel14)
			 , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#   ifdef Rel14
			 ,
			 (SystemInformationBlockType1_v1310_IEs_t *)NULL
#   endif
			 );
  
  /*
     handoverCommand.criticalExtensions.present = HandoverCommand__criticalExtensions_PR_c1;
     handoverCommand.criticalExtensions.choice.c1.present = HandoverCommand__criticalExtensions__c1_PR_handoverCommand_r8;
     handoverCommand.criticalExtensions.choice.c1.choice.handoverCommand_r8.handoverCommandMessage.buf = buffer;
     handoverCommand.criticalExtensions.choice.c1.choice.handoverCommand_r8.handoverCommandMessage.size = size;
   */
//#warning "COMPILATION PROBLEM"
#ifdef PROBLEM_COMPILATION_RESOLVED

  if (sourceModId != 0xFF) {
    memcpy(RC.rrc[sourceModId].handover_info[RC.rrc[ctxt_pP->module_id]->handover_info[ue_mod_idP]->ueid_s]->buf,
           (void *)buffer, size);
    RC.rrc[sourceModId].handover_info[RC.rrc[ctxt_pP->module_id]->handover_info[ue_mod_idP]->ueid_s]->size = size;
    RC.rrc[sourceModId].handover_info[RC.rrc[ctxt_pP->module_id]->handover_info[ue_mod_idP]->ueid_s]->ho_complete =
      0xF1;
    //RC.rrc[ctxt_pP->module_id]->handover_info[ue_mod_idP]->ho_complete = 0xFF;
    LOG_D(RRC, "[eNB %d] Frame %d: setting handover complete to 0xF1 for (%d,%d) and to 0xFF for (%d,%d)\n",
          ctxt_pP->module_id,
          ctxt_pP->frame,
          sourceModId,
          RC.rrc[ctxt_pP->module_id]->handover_info[ue_mod_idP]->ueid_s,
          ctxt_pP->module_id,
          ue_mod_idP);
  } else
    LOG_W(RRC,
          "[eNB %d] Frame %d: rrc_eNB_generate_RRCConnectionReconfiguration_handover: Could not find source eNB mod_id.\n",
          ctxt_pP->module_id, ctxt_pP->frame);

#endif
}

/*
  void ue_rrc_process_rrcConnectionReconfiguration(uint8_t enb_mod_idP,frame_t frameP,
  RRCConnectionReconfiguration_t *rrcConnectionReconfiguration,
  uint8_t CH_index) {

  if (rrcConnectionReconfiguration->criticalExtensions.present == RRCConnectionReconfiguration__criticalExtensions_PR_c1)
  if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.present == RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8) {

  if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated) {
  rrc_ue_process_radioResourceConfigDedicated(enb_mod_idP,frameP,CH_index,
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated);

  }

  // check other fields for
  }
  }
*/

//-----------------------------------------------------------------------------
void
rrc_eNB_process_RRCConnectionReconfigurationComplete(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*        ue_context_pP,
  const uint8_t xid
)
//-----------------------------------------------------------------------------
{
  int                                 i, drb_id;
#ifdef PDCP_USE_NETLINK
  int                                 oip_ifup = 0;
  int                                 dest_ip_offset = 0;
  module_id_t                         ue_module_id   = -1;
  /* avoid gcc warnings */
  (void)oip_ifup;
  (void)dest_ip_offset;
  (void)ue_module_id;
#endif

  uint8_t                            *kRRCenc = NULL;
  uint8_t                            *kRRCint = NULL;
  uint8_t                            *kUPenc = NULL;
  ue_context_pP->ue_context.ue_reestablishment_timer = 0;

  DRB_ToAddModList_t*                 DRB_configList = ue_context_pP->ue_context.DRB_configList2[xid];
  SRB_ToAddModList_t*                 SRB_configList = ue_context_pP->ue_context.SRB_configList2[xid];
  DRB_ToReleaseList_t*                DRB_Release_configList2 = ue_context_pP->ue_context.DRB_Release_configList2[xid];
  DRB_Identity_t*                     drb_id_p      = NULL;

  T(T_ENB_RRC_CONNECTION_RECONFIGURATION_COMPLETE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#if defined(ENABLE_SECURITY)

  /* Derive the keys from kenb */
  if (DRB_configList != NULL) {
    derive_key_up_enc(ue_context_pP->ue_context.ciphering_algorithm,
                      ue_context_pP->ue_context.kenb, &kUPenc);
  }

  derive_key_rrc_enc(ue_context_pP->ue_context.ciphering_algorithm,
                     ue_context_pP->ue_context.kenb, &kRRCenc);
  derive_key_rrc_int(ue_context_pP->ue_context.integrity_algorithm,
                     ue_context_pP->ue_context.kenb, &kRRCint);

#endif

  // Refresh SRBs/DRBs
  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_PDCP_ENB,
    NULL,
    0,
    MSC_AS_TIME_FMT" CONFIG_REQ UE %x DRB (security unchanged)",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti);

  rrc_pdcp_config_asn1_req(
    ctxt_pP,
    SRB_configList, //NULL,  //LG-RK 14/05/2014 SRB_configList,
    DRB_configList, 
//    (DRB_ToReleaseList_t *) NULL,
    DRB_Release_configList2,
    /*RC.rrc[ctxt_pP->module_id]->ciphering_algorithm[ue_mod_idP] |
             (RC.rrc[ctxt_pP->module_id]->integrity_algorithm[ue_mod_idP] << 4),
     */
    0xff, // already configured during the securitymodecommand
    kRRCenc,
    kRRCint,
    kUPenc
#if defined(Rel10) || defined(Rel14)
    , (PMCH_InfoList_r9_t *) NULL
#endif
    ,NULL);
  // Refresh SRBs/DRBs
  rrc_rlc_config_asn1_req(
    ctxt_pP,
    SRB_configList, // NULL,  //LG-RK 14/05/2014 SRB_configList,
    DRB_configList,
//    (DRB_ToReleaseList_t *) NULL
    DRB_Release_configList2
#if defined(Rel10) || defined(Rel14)
    , (PMCH_InfoList_r9_t *) NULL
#endif
  );
  
  // set the SRB active in Ue context
  if (SRB_configList != NULL) {
    for (i = 0; (i < SRB_configList->list.count) && (i < 3); i++) {
      if (SRB_configList->list.array[i]->srb_Identity == 1 ){
	ue_context_pP->ue_context.Srb1.Active=1;
      }
      else if (SRB_configList->list.array[i]->srb_Identity == 2 )  {
	  ue_context_pP->ue_context.Srb2.Active=1;
	  ue_context_pP->ue_context.Srb2.Srb_info.Srb_id=2;
	     LOG_I(RRC,"[eNB %d] Frame  %d CC %d : SRB2 is now active\n",
		ctxt_pP->module_id,
		ctxt_pP->frame,
		ue_context_pP->ue_context.primaryCC_id);
      } else {
	LOG_W(RRC,"[eNB %d] Frame  %d CC %d : invalide SRB identity %ld\n",
	      ctxt_pP->module_id,
	      ctxt_pP->frame,
              ue_context_pP->ue_context.primaryCC_id,
	      SRB_configList->list.array[i]->srb_Identity);
      }
    }
    free(SRB_configList);
    ue_context_pP->ue_context.SRB_configList2[xid] = NULL;
  }
  
  // Loop through DRBs and establish if necessary

  if (DRB_configList != NULL) {
    for (i = 0; i < DRB_configList->list.count; i++) {  // num max DRB (11-3-8)
      if (DRB_configList->list.array[i]) {
	drb_id = (int)DRB_configList->list.array[i]->drb_Identity;
        LOG_I(RRC,
              "[eNB %d] Frame  %d : Logical Channel UL-DCCH, Received RRCConnectionReconfigurationComplete from UE rnti %x, reconfiguring DRB %d/LCID %d\n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              ctxt_pP->rnti,
              (int)DRB_configList->list.array[i]->drb_Identity,
              (int)*DRB_configList->list.array[i]->logicalChannelIdentity);
        // for pre-ci tests
        LOG_I(RRC,
              "[eNB %d] Frame  %d : Logical Channel UL-DCCH, Received RRCConnectionReconfigurationComplete from UE %u, reconfiguring DRB %d/LCID %d\n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              oai_emulation.info.eNB_ue_local_uid_to_ue_module_id[ctxt_pP->module_id][ue_context_pP->local_uid],
              (int)DRB_configList->list.array[i]->drb_Identity,
              (int)*DRB_configList->list.array[i]->logicalChannelIdentity);

        if (ue_context_pP->ue_context.DRB_active[drb_id] == 0) {
          /*
             rrc_pdcp_config_req (ctxt_pP->module_id, frameP, 1, CONFIG_ACTION_ADD,
             (ue_mod_idP * NB_RB_MAX) + *DRB_configList->list.array[i]->logicalChannelIdentity,UNDEF_SECURITY_MODE);
             rrc_rlc_config_req(ctxt_pP->module_id,frameP,1,CONFIG_ACTION_ADD,
             (ue_mod_idP * NB_RB_MAX) + (int)*RC.rrc[ctxt_pP->module_id]->DRB_config[ue_mod_idP][i]->logicalChannelIdentity,
             RADIO_ACCESS_BEARER,Rlc_info_um);
           */
          ue_context_pP->ue_context.DRB_active[drb_id] = 1;

          LOG_D(RRC,
                "[eNB %d] Frame %d: Establish RLC UM Bidirectional, DRB %d Active\n",
                ctxt_pP->module_id, ctxt_pP->frame, (int)DRB_configList->list.array[i]->drb_Identity);
#if  defined(PDCP_USE_NETLINK) && !defined(LINK_ENB_PDCP_TO_GTPV1U)
          // can mean also IPV6 since ether -> ipv6 autoconf
#   if !defined(OAI_NW_DRIVER_TYPE_ETHERNET) && !defined(EXMIMO) && !defined(OAI_USRP) && !defined(OAI_BLADERF) && !defined(ETHERNET)
          LOG_I(OIP, "[eNB %d] trying to bring up the OAI interface oai%d\n",
                ctxt_pP->module_id,
                ctxt_pP->module_id);
          oip_ifup = nas_config(
                       ctxt_pP->module_id,   // interface index
                       ctxt_pP->module_id + 1,   // thrid octet
                       ctxt_pP->module_id + 1);  // fourth octet

          if (oip_ifup == 0) {    // interface is up --> send a config the DRB
            dest_ip_offset = 8;
            LOG_I(OIP,
                  "[eNB %d] Config the oai%d to send/receive pkt on DRB %ld to/from the protocol stack\n",
                  ctxt_pP->module_id, ctxt_pP->module_id,
                  (long int)((ue_context_pP->local_uid * maxDRB) + DRB_configList->list.array[i]->drb_Identity));
            ue_module_id = oai_emulation.info.eNB_ue_local_uid_to_ue_module_id[ctxt_pP->module_id][ue_context_pP->local_uid];
            rb_conf_ipv4(0, //add
                         ue_module_id,  //cx
                         ctxt_pP->module_id,    //inst
                         (ue_module_id * maxDRB) + DRB_configList->list.array[i]->drb_Identity, // RB
                         0,    //dscp
                         ipv4_address(ctxt_pP->module_id + 1, ctxt_pP->module_id + 1),  //saddr
                         ipv4_address(ctxt_pP->module_id + 1, dest_ip_offset + ue_module_id + 1));  //daddr
            LOG_D(RRC, "[eNB %d] State = Attached (UE rnti %x module id %u)\n",
                  ctxt_pP->module_id, ue_context_pP->ue_context.rnti, ue_module_id);
          }

#   endif
#endif

          LOG_D(RRC,
                PROTOCOL_RRC_CTXT_UE_FMT" RRC_eNB --- MAC_CONFIG_REQ  (DRB) ---> MAC_eNB\n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));

          if (DRB_configList->list.array[i]->logicalChannelIdentity) {
            DRB2LCHAN[i] = (uint8_t) * DRB_configList->list.array[i]->logicalChannelIdentity;
          }

          rrc_mac_config_req_eNB(
				 ctxt_pP->module_id,
				 ue_context_pP->ue_context.primaryCC_id,
				 0,0,0,0,0,
#ifdef Rel14 
				 0,
#endif
				 ue_context_pP->ue_context.rnti,
				 (BCCH_BCH_Message_t *) NULL,
				 (RadioResourceConfigCommonSIB_t *) NULL,
#ifdef Rel14
				 (RadioResourceConfigCommonSIB_t *) NULL,
#endif
				 ue_context_pP->ue_context.physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
				 (SCellToAddMod_r10_t *)NULL,
				 //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
				 (MeasObjectToAddMod_t **) NULL,
				 ue_context_pP->ue_context.mac_MainConfig,
				 DRB2LCHAN[i],
				 DRB_configList->list.array[i]->logicalChannelConfig,
				 ue_context_pP->ue_context.measGapConfig,
				 (TDD_Config_t *) NULL,
				 NULL,
				 (SchedulingInfoList_t *) NULL,
				 0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
#if defined(Rel10) || defined(Rel14)
				 , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#   ifdef Rel14
				 ,
				 (SystemInformationBlockType1_v1310_IEs_t *)NULL
#   endif
				 );
	  
        } else {        // remove LCHAN from MAC/PHY

          if (ue_context_pP->ue_context.DRB_active[drb_id] == 1) {
            // DRB has just been removed so remove RLC + PDCP for DRB
            /*      rrc_pdcp_config_req (ctxt_pP->module_id, frameP, 1, CONFIG_ACTION_REMOVE,
               (ue_mod_idP * NB_RB_MAX) + DRB2LCHAN[i],UNDEF_SECURITY_MODE);
             */
            rrc_rlc_config_req(
              ctxt_pP,
              SRB_FLAG_NO,
              MBMS_FLAG_NO,
              CONFIG_ACTION_REMOVE,
              DRB2LCHAN[i],
              Rlc_info_um);
          }

          ue_context_pP->ue_context.DRB_active[drb_id] = 0;
          LOG_D(RRC,
                PROTOCOL_RRC_CTXT_UE_FMT" RRC_eNB --- MAC_CONFIG_REQ  (DRB) ---> MAC_eNB\n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
          rrc_mac_config_req_eNB(ctxt_pP->module_id,
				 ue_context_pP->ue_context.primaryCC_id,
				 0,0,0,0,0,
#ifdef Rel14 
				 0,
#endif
				 ue_context_pP->ue_context.rnti,
				 (BCCH_BCH_Message_t *) NULL,
				 (RadioResourceConfigCommonSIB_t *) NULL,
#ifdef Rel14
				 (RadioResourceConfigCommonSIB_t *) NULL,
#endif
				 ue_context_pP->ue_context.physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
				 (SCellToAddMod_r10_t *)NULL,
				 //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
				 (MeasObjectToAddMod_t **) NULL,
				 ue_context_pP->ue_context.mac_MainConfig,
				 DRB2LCHAN[i],
				 (LogicalChannelConfig_t *) NULL,
				 (MeasGapConfig_t *) NULL,
				 (TDD_Config_t *) NULL,
				 NULL, 
				 (SchedulingInfoList_t *) NULL,
				 0, NULL, NULL, NULL
#if defined(Rel10) || defined(Rel14)
				 , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#   ifdef Rel14
				 ,
				 (SystemInformationBlockType1_v1310_IEs_t *)NULL
#   endif
				 );
        }
      }
    }
   free(DRB_configList);
    ue_context_pP->ue_context.DRB_configList2[xid] = NULL;
  }

  if(DRB_Release_configList2 != NULL){
      for (i = 0; i < DRB_Release_configList2->list.count; i++) {
          if (DRB_Release_configList2->list.array[i]) {
              drb_id_p = DRB_Release_configList2->list.array[i];
              drb_id = *drb_id_p;
              if (ue_context_pP->ue_context.DRB_active[drb_id] == 1) {
                  ue_context_pP->ue_context.DRB_active[drb_id] = 0;
              }
          }
      }
      free(DRB_Release_configList2);
      ue_context_pP->ue_context.DRB_Release_configList2[xid] = NULL;
  }
}

//-----------------------------------------------------------------------------
void
rrc_eNB_generate_RRCConnectionSetup(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  const int                    CC_id
)
//-----------------------------------------------------------------------------
{

  LogicalChannelConfig_t             *SRB1_logicalChannelConfig;  //,*SRB2_logicalChannelConfig;
  SRB_ToAddModList_t                **SRB_configList;
  SRB_ToAddMod_t                     *SRB1_config;
  int                                 cnt;

  T(T_ENB_RRC_CONNECTION_SETUP, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  SRB_configList = &ue_context_pP->ue_context.SRB_configList;
  RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size =
    do_RRCConnectionSetup(ctxt_pP,
                          ue_context_pP,
                          CC_id,
                          (uint8_t*) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload,
			  (uint8_t) RC.rrc[ctxt_pP->module_id]->carrier[CC_id].p_eNB, //at this point we do not have the UE capability information, so it can only be TM1 or TM2
                          rrc_eNB_get_next_transaction_identifier(ctxt_pP->module_id),
                          SRB_configList,
                          &ue_context_pP->ue_context.physicalConfigDedicated);

#ifdef RRC_MSG_PRINT
  LOG_F(RRC,"[MSG] RRC Connection Setup\n");

  for (cnt = 0; cnt < RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size; cnt++) {
    LOG_F(RRC,"%02x ", ((uint8_t*)RC.rrc[ctxt_pP->module_id]->Srb0.Tx_buffer.Payload)[cnt]);
  }

  LOG_F(RRC,"\n");
  //////////////////////////////////
#endif

  // configure SRB1/SRB2, PhysicalConfigDedicated, MAC_MainConfig for UE

  if (*SRB_configList != NULL) {
    for (cnt = 0; cnt < (*SRB_configList)->list.count; cnt++) {
      if ((*SRB_configList)->list.array[cnt]->srb_Identity == 1) {
        SRB1_config = (*SRB_configList)->list.array[cnt];

        if (SRB1_config->logicalChannelConfig) {
          if (SRB1_config->logicalChannelConfig->present ==
              SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
            SRB1_logicalChannelConfig = &SRB1_config->logicalChannelConfig->choice.explicitValue;
          } else {
            SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
          }
        } else {
          SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
        }

        LOG_D(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" RRC_eNB --- MAC_CONFIG_REQ  (SRB1) ---> MAC_eNB\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
        rrc_mac_config_req_eNB(
			       ctxt_pP->module_id,
			       ue_context_pP->ue_context.primaryCC_id,
			       0,0,0,0,0,
#ifdef Rel14 
			       0,
#endif
			       ue_context_pP->ue_context.rnti,
			       (BCCH_BCH_Message_t *) NULL,
			       (RadioResourceConfigCommonSIB_t *) NULL,
#ifdef Rel14
			       (RadioResourceConfigCommonSIB_t *) NULL,
#endif
			       ue_context_pP->ue_context.physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
			       (SCellToAddMod_r10_t *)NULL,
			       //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
#endif
			       (MeasObjectToAddMod_t **) NULL,
			       ue_context_pP->ue_context.mac_MainConfig,
			       1,
			       SRB1_logicalChannelConfig,
			       ue_context_pP->ue_context.measGapConfig,
			       (TDD_Config_t *) NULL,
			       NULL,
			       (SchedulingInfoList_t *) NULL,
			       0, NULL, NULL, (MBSFN_SubframeConfigList_t *) NULL
#if defined(Rel10) || defined(Rel14)
			       , 0, (MBSFN_AreaInfoList_r9_t *) NULL, (PMCH_InfoList_r9_t *) NULL
#endif
#   ifdef Rel14
			       ,
			       (SystemInformationBlockType1_v1310_IEs_t *)NULL
#   endif
			       );
        break;
      }
    }
  }

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_RRC_UE,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Header, // LG WARNING
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size,
    MSC_AS_TIME_FMT" RRCConnectionSetup UE %x size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);


  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel DL-CCCH, Generating RRCConnectionSetup (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size);

  // activate release timer, if RRCSetupComplete not received after 10 frames, remove UE
  //ue_context_pP->ue_context.ue_release_timer=1;
  // remove UE after 10 frames after RRCConnectionRelease is triggered
  //ue_context_pP->ue_context.ue_release_timer_thres=100;
     // activate release timer, if RRCSetupComplete not received after 100 frames, remove UE
   ue_context_pP->ue_context.ue_release_timer=1;
   // remove UE after 10 frames after RRCConnectionRelease is triggered
   ue_context_pP->ue_context.ue_release_timer_thres=1000;
}


#if defined(ENABLE_ITTI)
//-----------------------------------------------------------------------------
char
openair_rrc_eNB_configuration(
  const module_id_t enb_mod_idP,
  RrcConfigurationReq* configuration
)
#else
char
openair_rrc_eNB_init(
  const module_id_t enb_mod_idP
)
#endif
//-----------------------------------------------------------------------------
{
  protocol_ctxt_t ctxt;
  int             CC_id;

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, enb_mod_idP, ENB_FLAG_YES, NOT_A_RNTI, 0, 0,enb_mod_idP);
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_FMT" Init...\n",
        PROTOCOL_RRC_CTXT_ARGS(&ctxt));

#if OCP_FRAMEWORK
  while ( RC.rrc[enb_mod_idP] == NULL ) {
    LOG_E(RRC, "RC.rrc not yet initialized, waiting 1 second\n");
    sleep(1);
  }
#endif 
  AssertFatal(RC.rrc[enb_mod_idP] != NULL, "RC.rrc not initialized!");
  AssertFatal(NUMBER_OF_UE_MAX < (module_id_t)0xFFFFFFFFFFFFFFFF, " variable overflow");
#ifdef ENABLE_ITTI
  AssertFatal(configuration!=NULL,"configuration input is null\n");
#endif
  //    for (j = 0; j < NUMBER_OF_UE_MAX; j++)
  //        RC.rrc[ctxt.module_id].Info.UE[j].Status = RRC_IDLE;  //CH_READY;
  //
  //#if defined(ENABLE_USE_MME)
  //    // Connect eNB to MME
  //    if (EPC_MODE_ENABLED <= 0)
  //#endif
  //    {
  //        /* Init security parameters */
  //        for (j = 0; j < NUMBER_OF_UE_MAX; j++) {
  //            RC.rrc[ctxt.module_id].ciphering_algorithm[j] = SecurityAlgorithmConfig__cipheringAlgorithm_eea0;
  //            RC.rrc[ctxt.module_id].integrity_algorithm[j] = SecurityAlgorithmConfig__integrityProtAlgorithm_eia2;
  //            rrc_eNB_init_security(enb_mod_idP, j);
  //        }
  //    }
  RC.rrc[ctxt.module_id]->Nb_ue = 0;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    RC.rrc[ctxt.module_id]->carrier[CC_id].Srb0.Active = 0;
  }

  uid_linear_allocator_init(&RC.rrc[ctxt.module_id]->uid_allocator);
  RB_INIT(&RC.rrc[ctxt.module_id]->rrc_ue_head);
  //    for (j = 0; j < (NUMBER_OF_UE_MAX + 1); j++) {
  //        RC.rrc[enb_mod_idP]->Srb2[j].Active = 0;
  //    }


  RC.rrc[ctxt.module_id]->initial_id2_s1ap_ids = hashtable_create (NUMBER_OF_UE_MAX * 2, NULL, NULL);
  RC.rrc[ctxt.module_id]->s1ap_id2_s1ap_ids    = hashtable_create (NUMBER_OF_UE_MAX * 2, NULL, NULL);

  memcpy(&RC.rrc[ctxt.module_id]->configuration,configuration,sizeof(RrcConfigurationReq));

  /// System Information INIT

  LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" Checking release \n",
        PROTOCOL_RRC_CTXT_ARGS(&ctxt));
#if defined(Rel10) || defined(Rel14)

  // can clear it at runtime
  RC.rrc[ctxt.module_id]->carrier[0].MBMS_flag = 0;

  // This has to come from some top-level configuration
  // only CC_id 0 is logged
#if defined(Rel10)
  LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" Rel10 RRC detected, MBMS flag %d\n",
#else
  LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" Rel14 RRC detected, MBMS flag %d\n",
#endif
        PROTOCOL_RRC_CTXT_ARGS(&ctxt),
        RC.rrc[ctxt.module_id]->carrier[0].MBMS_flag);

#else
  LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" Rel8 RRC\n", PROTOCOL_RRC_CTXT_ARGS(&ctxt));
#endif
#ifdef CBA

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    for (j = 0; j < NUM_MAX_CBA_GROUP; j++) {
      RC.rrc[ctxt.module_id]->carrier[CC_id].cba_rnti[j] = CBA_OFFSET + j;
    }

    if (RC.rrc[ctxt.module_id]->carrier[CC_id].num_active_cba_groups > NUM_MAX_CBA_GROUP) {
      RC.rrc[ctxt.module_id]->carrier[CC_id].num_active_cba_groups = NUM_MAX_CBA_GROUP;
    }

    LOG_D(RRC,
          PROTOCOL_RRC_CTXT_FMT" Initialization of 4 cba_RNTI values (%x %x %x %x) num active groups %d\n",
          PROTOCOL_RRC_CTXT_ARGS(&ctxt),
          enb_mod_idP, RC.rrc[ctxt.module_id]->carrier[CC_id].cba_rnti[0],
          RC.rrc[ctxt.module_id]->carrier[CC_id].cba_rnti[1],
          RC.rrc[ctxt.module_id]->carrier[CC_id].cba_rnti[2],
          RC.rrc[ctxt.module_id]->carrier[CC_id].cba_rnti[3],
          RC.rrc[ctxt.module_id]->carrier[CC_id].num_active_cba_groups);
  }

#endif

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    init_SI(&ctxt,
            CC_id
#if defined(ENABLE_ITTI)
            , configuration
#endif
           );
    for (int ue_id = 0; ue_id < NUMBER_OF_UE_MAX; ue_id++) {
        RC.rrc[ctxt.module_id]->carrier[CC_id].sizeof_paging[ue_id] = 0;
        RC.rrc[ctxt.module_id]->carrier[CC_id].paging[ue_id] = (uint8_t*) malloc16(256);
    }
  }

  rrc_init_global_param();
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

#if defined(Rel10) || defined(Rel14)
    switch (RC.rrc[ctxt.module_id]->carrier[CC_id].MBMS_flag) {
    case 1:
    case 2:
    case 3:
      LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" Configuring 1 MBSFN sync area\n", PROTOCOL_RRC_CTXT_ARGS(&ctxt));
      RC.rrc[ctxt.module_id]->carrier[CC_id].num_mbsfn_sync_area = 1;
      break;

    case 4:
      LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" Configuring 2 MBSFN sync area\n", PROTOCOL_RRC_CTXT_ARGS(&ctxt));
      RC.rrc[ctxt.module_id]->carrier[CC_id].num_mbsfn_sync_area = 2;
      break;

    default:
      RC.rrc[ctxt.module_id]->carrier[CC_id].num_mbsfn_sync_area = 0;
      break;
    }

    // if we are here the RC.rrc[enb_mod_idP]->MBMS_flag > 0,
    /// MCCH INIT
    if (RC.rrc[ctxt.module_id]->carrier[CC_id].MBMS_flag > 0) {
      init_MCCH(ctxt.module_id, CC_id);
      /// MTCH data bearer init
      init_MBMS(ctxt.module_id, CC_id, 0);
    }
#endif

    openair_rrc_top_init_eNB(RC.rrc[ctxt.module_id]->carrier[CC_id].MBMS_flag,0);
  }
  openair_rrc_on(&ctxt);

  return 0;

}

/*------------------------------------------------------------------------------*/
int
rrc_eNB_decode_ccch(
  protocol_ctxt_t* const ctxt_pP,
  const SRB_INFO*        const Srb_info,
  const int              CC_id
)
//-----------------------------------------------------------------------------
{
  module_id_t                                   Idx;
  asn_dec_rval_t                      dec_rval;
  UL_CCCH_Message_t                  *ul_ccch_msg = NULL;
  RRCConnectionRequest_r8_IEs_t*                rrcConnectionRequest = NULL;
  RRCConnectionReestablishmentRequest_r8_IEs_t* rrcConnectionReestablishmentRequest = NULL;
  int                                 i, rval;
  struct rrc_eNB_ue_context_s*                  ue_context_p = NULL;
  uint64_t                                      random_value = 0;
  int                                           stmsi_received = 0;

  T(T_ENB_RRC_UL_CCCH_DATA_IN, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  //memset(ul_ccch_msg,0,sizeof(UL_CCCH_Message_t));

  LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Decoding UL CCCH %x.%x.%x.%x.%x.%x (%p)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        ((uint8_t*) Srb_info->Rx_buffer.Payload)[0],
        ((uint8_t *) Srb_info->Rx_buffer.Payload)[1],
        ((uint8_t *) Srb_info->Rx_buffer.Payload)[2],
        ((uint8_t *) Srb_info->Rx_buffer.Payload)[3],
        ((uint8_t *) Srb_info->Rx_buffer.Payload)[4],
        ((uint8_t *) Srb_info->Rx_buffer.Payload)[5], (uint8_t *) Srb_info->Rx_buffer.Payload);
  dec_rval = uper_decode(
               NULL,
               &asn_DEF_UL_CCCH_Message,
               (void**)&ul_ccch_msg,
               (uint8_t*) Srb_info->Rx_buffer.Payload,
               100,
               0,
               0);

  /*
#if defined(ENABLE_ITTI)
#   if defined(DISABLE_ITTI_XER_PRINT)
  {
    MessageDef                         *message_p;

    message_p = itti_alloc_new_message(TASK_RRC_ENB, RRC_UL_CCCH_MESSAGE);
    memcpy(&message_p->ittiMsg, (void *)ul_ccch_msg, sizeof(RrcUlCcchMessage));

    itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, message_p);
  }
#   else
  {
    char                                message_string[10000];
    size_t                              message_string_size;

    if ((message_string_size =
           xer_sprint(message_string, sizeof(message_string), &asn_DEF_UL_CCCH_Message, (void *)ul_ccch_msg)) > 0) {
      MessageDef                         *msg_p;

      msg_p = itti_alloc_new_message_sized(TASK_RRC_ENB, RRC_UL_CCCH, message_string_size + sizeof(IttiMsgText));
      msg_p->ittiMsg.rrc_ul_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_ul_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
#   endif
#endif
  */

  for (i = 0; i < 8; i++) {
    LOG_T(RRC, "%x.", ((uint8_t *) & ul_ccch_msg)[i]);
  }

  if (dec_rval.consumed == 0) {
    LOG_E(RRC,
          PROTOCOL_RRC_CTXT_UE_FMT" FATAL Error in receiving CCCH\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
    return -1;
  }

  if (ul_ccch_msg->message.present == UL_CCCH_MessageType_PR_c1) {

    switch (ul_ccch_msg->message.choice.c1.present) {

    case UL_CCCH_MessageType__c1_PR_NOTHING:
      LOG_I(RRC,
            PROTOCOL_RRC_CTXT_FMT" Received PR_NOTHING on UL-CCCH-Message\n",
            PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));
      break;

    case UL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentRequest:
      T(T_ENB_RRC_CONNECTION_REESTABLISHMENT_REQUEST, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Connection Reestablishment Request\n");

      for (i = 0; i < Srb_info->Rx_buffer.payload_size; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Srb_info->Rx_buffer.Payload)[i]);
      }

      LOG_F(RRC,"\n");
#endif
      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT"MAC_eNB--- MAC_DATA_IND (rrcConnectionReestablishmentRequest on SRB0) --> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
      rrcConnectionReestablishmentRequest =
        &ul_ccch_msg->message.choice.c1.choice.rrcConnectionReestablishmentRequest.criticalExtensions.choice.rrcConnectionReestablishmentRequest_r8;
      LOG_I(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionReestablishmentRequest cause %s\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            ((rrcConnectionReestablishmentRequest->reestablishmentCause == ReestablishmentCause_otherFailure) ?    "Other Failure" :
             (rrcConnectionReestablishmentRequest->reestablishmentCause == ReestablishmentCause_handoverFailure) ? "Handover Failure" :
             "reconfigurationFailure"));
      /*{
      uint64_t                            c_rnti = 0;

      memcpy(((uint8_t *) & c_rnti) + 3, rrcConnectionReestablishmentRequest.UE_identity.c_RNTI.buf,
      rrcConnectionReestablishmentRequest.UE_identity.c_RNTI.size);
      ue_mod_id = rrc_eNB_get_UE_index(enb_mod_idP, c_rnti);
      }

      if ((RC.rrc[enb_mod_idP]->phyCellId == rrcConnectionReestablishmentRequest.UE_identity.physCellId) &&
      (ue_mod_id != UE_INDEX_INVALID)){
      rrc_eNB_generate_RRCConnectionReestablishment(enb_mod_idP, frameP, ue_mod_id);
      }else {
      rrc_eNB_generate_RRCConnectionReestablishmentReject(enb_mod_idP, frameP, ue_mod_id);
      }
      */
      /* reject all reestablishment attempts for the moment */
//      rrc_eNB_generate_RRCConnectionReestablishmentReject(ctxt_pP,
//                       rrc_eNB_get_ue_context(RC.rrc[ctxt_pP->module_id], ctxt_pP->rnti),
//                       CC_id);
{
      uint16_t                          c_rnti = 0;

      if (rrcConnectionReestablishmentRequest->ue_Identity.physCellId != RC.rrc[ctxt_pP->module_id]->carrier[CC_id].physCellId) {
        LOG_E(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionReestablishmentRequest ue_Identity.physCellId(%ld) is not equal to current physCellId(%d), let's reject the UE\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
              rrcConnectionReestablishmentRequest->ue_Identity.physCellId,
              RC.rrc[ctxt_pP->module_id]->carrier[CC_id].physCellId);
        rrc_eNB_generate_RRCConnectionReestablishmentReject(ctxt_pP, ue_context_p, CC_id);
        break;
      }
      LOG_D(RRC, "physCellId is %ld\n", rrcConnectionReestablishmentRequest->ue_Identity.physCellId);

      for (i = 0; i < rrcConnectionReestablishmentRequest->ue_Identity.shortMAC_I.size; i++) {
        LOG_D(RRC, "rrcConnectionReestablishmentRequest->ue_Identity.shortMAC_I.buf[%d] = %x\n",
            i, rrcConnectionReestablishmentRequest->ue_Identity.shortMAC_I.buf[i]);
      }

      if (rrcConnectionReestablishmentRequest->ue_Identity.c_RNTI.size == 0 ||
          rrcConnectionReestablishmentRequest->ue_Identity.c_RNTI.size > 2) {
        LOG_E(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionReestablishmentRequest c_RNTI range error, let's reject the UE\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
        rrc_eNB_generate_RRCConnectionReestablishmentReject(ctxt_pP, ue_context_p, CC_id);
        break;

      }

      c_rnti = BIT_STRING_to_uint16(&rrcConnectionReestablishmentRequest->ue_Identity.c_RNTI);
      LOG_D(RRC, "c_rnti is %x\n", c_rnti);

      ue_context_p = rrc_eNB_get_ue_context(RC.rrc[ctxt_pP->module_id], c_rnti);
      if (ue_context_p == NULL) {
        LOG_E(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionReestablishmentRequest without UE context, let's reject the UE\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
        rrc_eNB_generate_RRCConnectionReestablishmentReject(ctxt_pP, ue_context_p, CC_id);
        break;
      }
      int UE_id = find_UE_id(ctxt_pP->module_id, c_rnti);
      if(ue_context_p->ue_context.ue_reestablishment_timer > 0 || RC.mac[ctxt_pP->module_id]->UE_list.UE_sched_ctrl[UE_id].ue_reestablishment_reject_timer > 0){
          LOG_I(RRC,
                PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionReestablishment(Previous) don't complete, let's reject the UE\n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
          rrc_eNB_generate_RRCConnectionReestablishmentReject(ctxt_pP, ue_context_p, CC_id);
          break;
      }
      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" UE context: %p\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            ue_context_p);
      ue_context_p->ue_context.ul_failure_timer = 0;
      ue_context_p->ue_context.ue_release_timer = 0;
      ue_context_p->ue_context.ue_reestablishment_timer = 0;
      ue_context_p->ue_context.ue_release_timer_s1 = 0;
      ue_context_p->ue_context.ue_release_timer_rrc = 0;

      /* reset timers */
      ue_context_p->ue_context.ul_failure_timer = 0;
      ue_context_p->ue_context.ue_release_timer = 0;

      // insert C-RNTI to map
      for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
        if (reestablish_rnti_map[i][0] == 0) {
          reestablish_rnti_map[i][0] = ctxt_pP->rnti;
          reestablish_rnti_map[i][1] = c_rnti;
          break;
        }
      }
      LOG_D(RRC, "reestablish_rnti_map[%d] [0] %x, [1] %x\n",
            i, reestablish_rnti_map[i][0], reestablish_rnti_map[i][1]);

#if defined(ENABLE_ITTI)
      ue_context_p->ue_context.reestablishment_cause = rrcConnectionReestablishmentRequest->reestablishmentCause;
      LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Accept connection reestablishment request from UE physCellId %ld cause %ld\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            rrcConnectionReestablishmentRequest->ue_Identity.physCellId,
            ue_context_p->ue_context.reestablishment_cause);
#else
        LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Accept connection restablishment request for UE\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
#endif

#ifndef NO_RRM
      send_msg(&S_rrc, msg_rrc_MR_attach_ind(ctxt_pP->module_id, Mac_id));
#else

      ue_context_p->ue_context.primaryCC_id = CC_id;

      //LG COMMENT Idx = (ue_mod_idP * NB_RB_MAX) + DCCH;
      Idx = DCCH;
      // SRB1
      ue_context_p->ue_context.Srb1.Active = 1;
      ue_context_p->ue_context.Srb1.Srb_info.Srb_id = Idx;
      memcpy(&ue_context_p->ue_context.Srb1.Srb_info.Lchan_desc[0],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);
      memcpy(&ue_context_p->ue_context.Srb1.Srb_info.Lchan_desc[1],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);

      // SRB2: set  it to go through SRB1 with id 1 (DCCH)
      ue_context_p->ue_context.Srb2.Active = 1;
      ue_context_p->ue_context.Srb2.Srb_info.Srb_id = Idx;
      memcpy(&ue_context_p->ue_context.Srb2.Srb_info.Lchan_desc[0],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);
      memcpy(&ue_context_p->ue_context.Srb2.Srb_info.Lchan_desc[1],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);

      rrc_eNB_generate_RRCConnectionReestablishment(ctxt_pP, ue_context_p, CC_id);
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT"CALLING RLC CONFIG SRB1 (rbid %d)\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            Idx);

      MSC_LOG_TX_MESSAGE(MSC_RRC_ENB,
                         MSC_PDCP_ENB,
                         NULL,
                         0,
                         MSC_AS_TIME_FMT" CONFIG_REQ UE %x SRB",
                         MSC_AS_TIME_ARGS(ctxt_pP),
                         ue_context_p->ue_context.rnti);

      rrc_pdcp_config_asn1_req(ctxt_pP,
                               ue_context_p->ue_context.SRB_configList,
                               (DRB_ToAddModList_t *) NULL,
                               (DRB_ToReleaseList_t*) NULL,
                               0xff,
                               NULL,
                               NULL,
                               NULL
#   if defined(Rel10) || defined(Rel14)
                               , (PMCH_InfoList_r9_t *) NULL
#   endif
                               ,NULL);

      rrc_rlc_config_asn1_req(ctxt_pP,
                              ue_context_p->ue_context.SRB_configList,
                              (DRB_ToAddModList_t*) NULL,
                              (DRB_ToReleaseList_t*) NULL
#   if defined(Rel10) || defined(Rel14)
                              , (PMCH_InfoList_r9_t *) NULL
#   endif
                             );
#endif //NO_RRM
      }
      break;

    case UL_CCCH_MessageType__c1_PR_rrcConnectionRequest:
      T(T_ENB_RRC_CONNECTION_REQUEST, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Connection Request\n");

      for (i = 0; i < Srb_info->Rx_buffer.payload_size; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Srb_info->Rx_buffer.Payload)[i]);
      }

      LOG_F(RRC,"\n");
#endif
      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT"MAC_eNB --- MAC_DATA_IND  (rrcConnectionRequest on SRB0) --> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
      ue_context_p = rrc_eNB_get_ue_context(
                       RC.rrc[ctxt_pP->module_id],
                       ctxt_pP->rnti);

      if (ue_context_p != NULL) {
        // erase content
        rrc_eNB_free_mem_UE_context(ctxt_pP, ue_context_p);

        MSC_LOG_RX_DISCARDED_MESSAGE(
          MSC_RRC_ENB,
          MSC_RRC_UE,
          Srb_info->Rx_buffer.Payload,
          dec_rval.consumed,
          MSC_AS_TIME_FMT" RRCConnectionRequest UE %x size %u (UE already in context)",
          MSC_AS_TIME_ARGS(ctxt_pP),
          ue_context_p->ue_context.rnti,
          dec_rval.consumed);
      } else {
        rrcConnectionRequest = &ul_ccch_msg->message.choice.c1.choice.rrcConnectionRequest.criticalExtensions.choice.rrcConnectionRequest_r8;
        {
          if (InitialUE_Identity_PR_randomValue == rrcConnectionRequest->ue_Identity.present) {
            AssertFatal(rrcConnectionRequest->ue_Identity.choice.randomValue.size == 5,
                        "wrong InitialUE-Identity randomValue size, expected 5, provided %d",
                        rrcConnectionRequest->ue_Identity.choice.randomValue.size);
            memcpy(((uint8_t*) & random_value) + 3,
                   rrcConnectionRequest->ue_Identity.choice.randomValue.buf,
                   rrcConnectionRequest->ue_Identity.choice.randomValue.size);
            /* if there is already a registered UE (with another RNTI) with this random_value,
             * the current one must be removed from MAC/PHY (zombie UE)
             */
#if 0
            if ((ue_context_p = rrc_eNB_ue_context_random_exist(ctxt_pP, random_value))) {
              LOG_W(RRC, "new UE rnti %x (coming with random value) is already there as UE %x, removing %x from MAC/PHY\n",
                    ctxt_pP->rnti, ue_context_p->ue_context.rnti, ctxt_pP->rnti);
	      rrc_mac_remove_ue(ctxt_pP->module_id, ctxt_pP->rnti);
              ue_context_p = NULL;
              return 0;
            } else {
              ue_context_p = rrc_eNB_get_next_free_ue_context(ctxt_pP, random_value);
            }
#endif
            if ((ue_context_p = rrc_eNB_ue_context_random_exist(ctxt_pP, random_value))) {
              LOG_W(RRC, "new UE rnti %x (coming with random value) is already there as UE %x, removing %x from MAC/PHY\n",
                    ctxt_pP->rnti, ue_context_p->ue_context.rnti, ue_context_p->ue_context.rnti);
              ue_context_p->ue_context.ul_failure_timer = 20000;
            }
            ue_context_p = rrc_eNB_get_next_free_ue_context(ctxt_pP, random_value);
          } else if (InitialUE_Identity_PR_s_TMSI == rrcConnectionRequest->ue_Identity.present) {
            /* Save s-TMSI */
            S_TMSI_t   s_TMSI = rrcConnectionRequest->ue_Identity.choice.s_TMSI;
            mme_code_t mme_code = BIT_STRING_to_uint8(&s_TMSI.mmec);
            m_tmsi_t   m_tmsi   = BIT_STRING_to_uint32(&s_TMSI.m_TMSI);
            random_value = (((uint64_t)mme_code) << 32) | m_tmsi;
            if ((ue_context_p = rrc_eNB_ue_context_stmsi_exist(ctxt_pP, mme_code, m_tmsi))) {
	      LOG_I(RRC," S-TMSI exists, ue_context_p %p, old rnti %x => %x\n",ue_context_p,ue_context_p->ue_context.rnti,ctxt_pP->rnti);
	      rrc_mac_remove_ue(ctxt_pP->module_id, ue_context_p->ue_context.rnti);
	      stmsi_received=1;
              /* replace rnti in the context */
              /* for that, remove the context from the RB tree */
              RB_REMOVE(rrc_ue_tree_s, &RC.rrc[ctxt_pP->module_id]->rrc_ue_head, ue_context_p);
              /* and insert again, after changing rnti everywhere it has to be changed */
              ue_context_p->ue_id_rnti = ctxt_pP->rnti;
	      ue_context_p->ue_context.rnti = ctxt_pP->rnti;
              RB_INSERT(rrc_ue_tree_s, &RC.rrc[ctxt_pP->module_id]->rrc_ue_head, ue_context_p);
              /* reset timers */
              ue_context_p->ue_context.ul_failure_timer = 0;
              ue_context_p->ue_context.ue_release_timer = 0;
              ue_context_p->ue_context.ue_reestablishment_timer = 0;
              ue_context_p->ue_context.ue_release_timer_s1 = 0;
              ue_context_p->ue_context.ue_release_timer_rrc = 0;
            } else {
	      LOG_I(RRC," S-TMSI doesn't exist, setting Initialue_identity_s_TMSI.m_tmsi to %p => %x\n",ue_context_p,m_tmsi);
//              ue_context_p = rrc_eNB_get_next_free_ue_context(ctxt_pP, NOT_A_RANDOM_UE_IDENTITY);
              ue_context_p = rrc_eNB_get_next_free_ue_context(ctxt_pP,random_value);
              if (ue_context_p == NULL)
                LOG_E(RRC, "%s:%d:%s: rrc_eNB_get_next_free_ue_context returned NULL\n", __FILE__, __LINE__, __FUNCTION__);
              if (ue_context_p != NULL) {
	        ue_context_p->ue_context.Initialue_identity_s_TMSI.presence = TRUE;
	        ue_context_p->ue_context.Initialue_identity_s_TMSI.mme_code = mme_code;
	        ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi = m_tmsi;
              } else {
                /* TODO: do we have to break here? */
                //break;
              }
            }

            MSC_LOG_RX_MESSAGE(
              MSC_RRC_ENB,
              MSC_RRC_UE,
              Srb_info->Rx_buffer.Payload,
              dec_rval.consumed,
              MSC_AS_TIME_FMT" RRCConnectionRequest UE %x size %u (s-TMSI mmec %u m_TMSI %u random UE id (0x%" PRIx64 ")",
              MSC_AS_TIME_ARGS(ctxt_pP),
              ue_context_p->ue_context.rnti,
              dec_rval.consumed,
              ue_context_p->ue_context.Initialue_identity_s_TMSI.mme_code,
              ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi,
              ue_context_p->ue_context.random_ue_identity);
          } else {
            LOG_E(RRC,
                  PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionRequest without random UE identity or S-TMSI not supported, let's reject the UE\n",
                  PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
            rrc_eNB_generate_RRCConnectionReject(ctxt_pP,
                             rrc_eNB_get_ue_context(RC.rrc[ctxt_pP->module_id], ctxt_pP->rnti),
                             CC_id);
            break;
          }

        }
        LOG_D(RRC,
              PROTOCOL_RRC_CTXT_UE_FMT" UE context: %p\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
              ue_context_p);

        if (ue_context_p != NULL) {


#if defined(ENABLE_ITTI)
          ue_context_p->ue_context.establishment_cause = rrcConnectionRequest->establishmentCause;
          ue_context_p->ue_context.reestablishment_cause = ReestablishmentCause_spare1;
	  if (stmsi_received==0)
	    LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Accept new connection from UE random UE identity (0x%" PRIx64 ") MME code %u TMSI %u cause %ld\n",
		  PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
		  ue_context_p->ue_context.random_ue_identity,
		  ue_context_p->ue_context.Initialue_identity_s_TMSI.mme_code,
		  ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi,
		  ue_context_p->ue_context.establishment_cause);
	  else
	    LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Accept new connection from UE  MME code %u TMSI %u cause %ld\n",
		  PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
		  ue_context_p->ue_context.Initialue_identity_s_TMSI.mme_code,
		  ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi,
		  ue_context_p->ue_context.establishment_cause);
#else
          LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Accept new connection for UE random UE identity (0x%" PRIx64 ")\n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
                ue_context_p->ue_context.random_ue_identity);
#endif
          if (stmsi_received == 0)
	    RC.rrc[ctxt_pP->module_id]->Nb_ue++;

        } else {
          // no context available
	  if (rrc_agent_registered[ctxt_pP->module_id]) {
	    agent_rrc_xface[ctxt_pP->module_id]->flexran_agent_notify_ue_state_change(ctxt_pP->module_id,
										      ctxt_pP->rnti,
										      PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
	  }
          LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Can't create new context for UE random UE identity (0x%" PRIx64 ")\n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
                random_value);
	  rrc_mac_remove_ue(ctxt_pP->module_id,ctxt_pP->rnti);
          return -1;
        }
      }

#ifndef NO_RRM
      send_msg(&S_rrc, msg_rrc_MR_attach_ind(ctxt_pP->module_id, Mac_id));
#else

      ue_context_p->ue_context.primaryCC_id = CC_id;

      //LG COMMENT Idx = (ue_mod_idP * NB_RB_MAX) + DCCH;
      Idx = DCCH;
      // SRB1
      ue_context_p->ue_context.Srb1.Active = 1;
      ue_context_p->ue_context.Srb1.Srb_info.Srb_id = Idx;
      memcpy(&ue_context_p->ue_context.Srb1.Srb_info.Lchan_desc[0],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);
      memcpy(&ue_context_p->ue_context.Srb1.Srb_info.Lchan_desc[1],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);

      // SRB2: set  it to go through SRB1 with id 1 (DCCH)
      ue_context_p->ue_context.Srb2.Active = 1;
      ue_context_p->ue_context.Srb2.Srb_info.Srb_id = Idx;
      memcpy(&ue_context_p->ue_context.Srb2.Srb_info.Lchan_desc[0],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);
      memcpy(&ue_context_p->ue_context.Srb2.Srb_info.Lchan_desc[1],
             &DCCH_LCHAN_DESC,
             LCHAN_DESC_SIZE);
      
      rrc_eNB_generate_RRCConnectionSetup(ctxt_pP, ue_context_p, CC_id);
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT"CALLING RLC CONFIG SRB1 (rbid %d)\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            Idx);

      MSC_LOG_TX_MESSAGE(
        MSC_RRC_ENB,
        MSC_PDCP_ENB,
        NULL,
        0,
        MSC_AS_TIME_FMT" CONFIG_REQ UE %x SRB",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti);

      rrc_pdcp_config_asn1_req(ctxt_pP,
                               ue_context_p->ue_context.SRB_configList,
                               (DRB_ToAddModList_t *) NULL,
                               (DRB_ToReleaseList_t*) NULL,
                               0xff,
                               NULL,
                               NULL,
                               NULL
#   if defined(Rel10) || defined(Rel14)
                               , (PMCH_InfoList_r9_t *) NULL
#   endif
                               ,NULL);

      rrc_rlc_config_asn1_req(ctxt_pP,
                              ue_context_p->ue_context.SRB_configList,
                              (DRB_ToAddModList_t*) NULL,
                              (DRB_ToReleaseList_t*) NULL
#   if defined(Rel10) || defined(Rel14)
                              , (PMCH_InfoList_r9_t *) NULL
#   endif
                             );
#endif //NO_RRM

      break;

    default:
      LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Unknown message\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
      rval = -1;
      break;
    }

    rval = 0;
  } else {
    LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT"  Unknown error \n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
    rval = -1;
  }

  return rval;
}

//-----------------------------------------------------------------------------
int
rrc_eNB_decode_dcch(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                Srb_id,
  const uint8_t*    const      Rx_sdu,
  const sdu_size_t             sdu_sizeP
)
//-----------------------------------------------------------------------------
{

  asn_dec_rval_t                      dec_rval;
  //UL_DCCH_Message_t uldcchmsg;
  UL_DCCH_Message_t                  *ul_dcch_msg = NULL; //&uldcchmsg;
  int i;
  struct rrc_eNB_ue_context_s*        ue_context_p = NULL;
#if defined(ENABLE_ITTI)
#   if defined(ENABLE_USE_MME)
  MessageDef *                        msg_delete_tunnels_p = NULL;
  uint8_t                             xid;
#endif
#endif

  int dedicated_DRB=0; 

  T(T_ENB_RRC_UL_DCCH_DATA_IN, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

  if ((Srb_id != 1) && (Srb_id != 2)) {
    LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Received message on SRB%d, should not have ...\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          Srb_id);
  } else {
    LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Received message on SRB%d\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          Srb_id);
  }
  //memset(ul_dcch_msg,0,sizeof(UL_DCCH_Message_t));

  LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Decoding UL-DCCH Message\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
  dec_rval = uper_decode(
               NULL,
               &asn_DEF_UL_DCCH_Message,
               (void**)&ul_dcch_msg,
               Rx_sdu,
               sdu_sizeP,
               0,
               0);
  /*
#if defined(ENABLE_ITTI)
#   if defined(DISABLE_ITTI_XER_PRINT)
  {
    MessageDef                         *message_p;

    message_p = itti_alloc_new_message(TASK_RRC_ENB, RRC_UL_DCCH_MESSAGE);
    memcpy(&message_p->ittiMsg, (void *)ul_dcch_msg, sizeof(RrcUlDcchMessage));

    itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, message_p);
  }
#   else
  {
    char                                message_string[10000];
    size_t                              message_string_size;

    if ((message_string_size =
           xer_sprint(message_string, sizeof(message_string), &asn_DEF_UL_DCCH_Message, (void *)ul_dcch_msg)) >= 0) {
      MessageDef                         *msg_p;

      msg_p = itti_alloc_new_message_sized(TASK_RRC_ENB, RRC_UL_DCCH, message_string_size + sizeof(IttiMsgText));
      msg_p->ittiMsg.rrc_ul_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_ul_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
#   endif
#endif
  */
  {
    for (i = 0; i < sdu_sizeP; i++) {
      LOG_T(RRC, "%x.", Rx_sdu[i]);
    }

    LOG_T(RRC, "\n");
  }

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Failed to decode UL-DCCH (%zu bytes)\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          dec_rval.consumed);
    return -1;
  }

  ue_context_p = rrc_eNB_get_ue_context(
                   RC.rrc[ctxt_pP->module_id],
                   ctxt_pP->rnti);

  if (ul_dcch_msg->message.present == UL_DCCH_MessageType_PR_c1) {

    switch (ul_dcch_msg->message.choice.c1.present) {
    case UL_DCCH_MessageType__c1_PR_NOTHING:   /* No components present */
      break;

    case UL_DCCH_MessageType__c1_PR_csfbParametersRequestCDMA2000:
      break;

    case UL_DCCH_MessageType__c1_PR_measurementReport:
      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND "
            "%d bytes (measurementReport) ---> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);
      rrc_eNB_process_MeasurementReport(
        ctxt_pP,
        ue_context_p,
        &ul_dcch_msg->message.choice.c1.choice.measurementReport.
        criticalExtensions.choice.c1.choice.measurementReport_r8.measResults);
      break;

    case UL_DCCH_MessageType__c1_PR_rrcConnectionReconfigurationComplete:
#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Connection Reconfiguration Complete\n");

      for (i = 0; i < sdu_sizeP; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif
      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" RRCConnectionReconfigurationComplete UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(RRCConnectionReconfigurationComplete) ---> RRC_eNB]\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);

      if (ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.criticalExtensions.
          present ==
          RRCConnectionReconfigurationComplete__criticalExtensions_PR_rrcConnectionReconfigurationComplete_r8) {
	/*NN: revise the condition */
        if (ue_context_p->ue_context.Status == RRC_RECONFIGURED){
	  dedicated_DRB = 1;
	  LOG_I(RRC,
		PROTOCOL_RRC_CTXT_UE_FMT" UE State = RRC_RECONFIGURED (dedicated DRB, xid %ld)\n",
		PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier);
	}else {
	  dedicated_DRB = 0;
	  ue_context_p->ue_context.Status = RRC_RECONFIGURED;
	  LOG_I(RRC,
		PROTOCOL_RRC_CTXT_UE_FMT" UE State = RRC_RECONFIGURED (default DRB, xid %ld)\n",
		PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier);
	}
	rrc_eNB_process_RRCConnectionReconfigurationComplete(
          ctxt_pP,
          ue_context_p,
	  ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier);

	//WARNING:Inform the controller about the UE activation. Should be moved to RRC agent in the future
	if (rrc_agent_registered[ctxt_pP->module_id]) {
	  agent_rrc_xface[ctxt_pP->eNB_index]->flexran_agent_notify_ue_state_change(ctxt_pP->module_id,
										ue_context_p->ue_id_rnti,
										PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_UPDATED);
	}
      }
#if defined(ENABLE_ITTI)
#   if defined(ENABLE_USE_MME)
      if (EPC_MODE_ENABLED == 1) {
	if (dedicated_DRB == 1){
//	  rrc_eNB_send_S1AP_E_RAB_SETUP_RESP(ctxt_pP,
//					     ue_context_p,
//					     ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier);
if (ue_context_p->ue_context.nb_of_modify_e_rabs > 0) {
            rrc_eNB_send_S1AP_E_RAB_MODIFY_RESP(ctxt_pP,
                             ue_context_p,
                             ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier);

            ue_context_p->ue_context.nb_of_modify_e_rabs = 0;
            ue_context_p->ue_context.nb_of_failed_e_rabs = 0;
            memset(ue_context_p->ue_context.modify_e_rab, 0, sizeof(ue_context_p->ue_context.modify_e_rab));
            for(int i = 0; i < NB_RB_MAX; i++) {
              ue_context_p->ue_context.modify_e_rab[i].xid = -1;
            }

          } else if(ue_context_p->ue_context.e_rab_release_command_flag == 1){
            xid = ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier;
            ue_context_p->ue_context.e_rab_release_command_flag = 0;
            //gtp tunnel delete
            msg_delete_tunnels_p = itti_alloc_new_message(TASK_RRC_ENB, GTPV1U_ENB_DELETE_TUNNEL_REQ);
            memset(&GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p), 0, sizeof(GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p)));
            GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).rnti = ue_context_p->ue_context.rnti;
            for(i = 0; i < NB_RB_MAX; i++){
               if(xid == ue_context_p->ue_context.e_rab[i].xid){
                 GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).eps_bearer_id[GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).num_erab++] = ue_context_p->ue_context.enb_gtp_ebi[i];
                 ue_context_p->ue_context.enb_gtp_teid[i] = 0;
                 memset(&ue_context_p->ue_context.enb_gtp_addrs[i], 0, sizeof(ue_context_p->ue_context.enb_gtp_addrs[i]));
                 ue_context_p->ue_context.enb_gtp_ebi[i]  = 0;
               }
            }
            itti_send_msg_to_task(TASK_GTPV1_U, ctxt_pP->instance, msg_delete_tunnels_p);
            //S1AP_E_RAB_RELEASE_RESPONSE
            rrc_eNB_send_S1AP_E_RAB_RELEASE_RESPONSE(ctxt_pP,
                    ue_context_p,
                    xid);
          } else {
              rrc_eNB_send_S1AP_E_RAB_SETUP_RESP(ctxt_pP,
                             ue_context_p,
                             ul_dcch_msg->message.choice.c1.choice.rrcConnectionReconfigurationComplete.rrc_TransactionIdentifier);
          }
	}else {
          if(ue_context_p->ue_context.reestablishment_cause == ReestablishmentCause_spare1){
	    rrc_eNB_send_S1AP_INITIAL_CONTEXT_SETUP_RESP(ctxt_pP,
						       ue_context_p);
          } else {
            ue_context_p->ue_context.reestablishment_cause = ReestablishmentCause_spare1;
            for (uint8_t e_rab = 0; e_rab < ue_context_p->ue_context.nb_of_e_rabs; e_rab++) {
              if (ue_context_p->ue_context.e_rab[e_rab].status == E_RAB_STATUS_DONE) {
                ue_context_p->ue_context.e_rab[e_rab].status = E_RAB_STATUS_ESTABLISHED;
              } else {
                ue_context_p->ue_context.e_rab[e_rab].status = E_RAB_STATUS_FAILED;
              }
            }
          }
	}
      }    
#else  // establish a dedicated bearer 
      if (dedicated_DRB == 0 ) {
	//	ue_context_p->ue_context.e_rab[0].status = E_RAB_STATUS_ESTABLISHED;
	rrc_eNB_reconfigure_DRBs(ctxt_pP,ue_context_p);
      }
      
#endif
#endif 
      break;

    case UL_DCCH_MessageType__c1_PR_rrcConnectionReestablishmentComplete:
      T(T_ENB_RRC_CONNECTION_REESTABLISHMENT_COMPLETE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Connection Reestablishment Complete\n");

      for (i = 0; i < sdu_sizeP; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif

      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" rrcConnectionReestablishmentComplete UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

      LOG_I(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(rrcConnectionReestablishmentComplete) ---> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);
       {
        int UE_id = find_UE_id(ctxt_pP->module_id, ctxt_pP->rnti);
        RC.mac[ctxt_pP->module_id]->UE_list.UE_sched_ctrl[UE_id].ue_reestablishment_reject_timer = 0;
        rnti_t reestablish_rnti = 0;
        // select C-RNTI from map
        for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
          if (reestablish_rnti_map[i][0] == ctxt_pP->rnti) {
            reestablish_rnti = reestablish_rnti_map[i][1];
            ue_context_p = rrc_eNB_get_ue_context(
                              RC.rrc[ctxt_pP->module_id],
                              reestablish_rnti);
            // clear currentC-RNTI from map
            reestablish_rnti_map[i][0] = 0;
            reestablish_rnti_map[i][1] = 0;
            break;
          }
        }
        LOG_D(RRC, "reestablish_rnti_map[%d] [0] %x, [1] %x\n",
              i, reestablish_rnti_map[i][0], reestablish_rnti_map[i][1]);

        if (!ue_context_p) {
          LOG_E(RRC,
                PROTOCOL_RRC_CTXT_UE_FMT" RRCConnectionReestablishmentComplete without UE context, falt\n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
          break;
        }

        if (ul_dcch_msg->message.choice.c1.choice.rrcConnectionReestablishmentComplete.criticalExtensions.present ==
            RRCConnectionReestablishmentComplete__criticalExtensions_PR_rrcConnectionReestablishmentComplete_r8) {
          rrc_eNB_process_RRCConnectionReestablishmentComplete(ctxt_pP, reestablish_rnti, ue_context_p,
              ul_dcch_msg->message.choice.c1.choice.rrcConnectionReestablishmentComplete.rrc_TransactionIdentifier,
              &ul_dcch_msg->message.choice.c1.choice.rrcConnectionReestablishmentComplete.criticalExtensions.choice.rrcConnectionReestablishmentComplete_r8);

          //WARNING:Inform the controller about the UE activation. Should be moved to RRC agent in the future
          if (mac_agent_registered[ctxt_pP->module_id]) {
            agent_rrc_xface[ctxt_pP->eNB_index]->flexran_agent_notify_ue_state_change(ctxt_pP->module_id,
                                                                                      ue_context_p->ue_id_rnti,
                                                                                      PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_ACTIVATED);
          }
        }
        //ue_context_p->ue_context.ue_release_timer = 0;
        ue_context_p->ue_context.ue_reestablishment_timer = 1;
        // remove UE after 100 frames after RRCConnectionReestablishmentRelease is triggered
        ue_context_p->ue_context.ue_reestablishment_timer_thres = 1000;
      }
      break;

    case UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete:
#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Connection SetupComplete\n");

      for (i = 0; i < sdu_sizeP; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif

      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" RRCConnectionSetupComplete UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(RRCConnectionSetupComplete) ---> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);

      if (ul_dcch_msg->message.choice.c1.choice.rrcConnectionSetupComplete.criticalExtensions.present ==
          RRCConnectionSetupComplete__criticalExtensions_PR_c1) {
        if (ul_dcch_msg->message.choice.c1.choice.rrcConnectionSetupComplete.criticalExtensions.choice.c1.
            present ==
            RRCConnectionSetupComplete__criticalExtensions__c1_PR_rrcConnectionSetupComplete_r8) {
          rrc_eNB_process_RRCConnectionSetupComplete(
            ctxt_pP,
            ue_context_p,
            &ul_dcch_msg->message.choice.c1.choice.rrcConnectionSetupComplete.criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8);
          ue_context_p->ue_context.Status = RRC_CONNECTED;
          LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" UE State = RRC_CONNECTED \n",
                PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
	  
	  //WARNING:Inform the controller about the UE activation. Should be moved to RRC agent in the future
	  if (rrc_agent_registered[ctxt_pP->module_id]) {
	    agent_rrc_xface[ctxt_pP->eNB_index]->flexran_agent_notify_ue_state_change(ctxt_pP->module_id,
										  ue_context_p->ue_id_rnti,
										  PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_ACTIVATED);
	  }
        }
      }

      ue_context_p->ue_context.ue_release_timer=0;
      break;

    case UL_DCCH_MessageType__c1_PR_securityModeComplete:
      T(T_ENB_RRC_SECURITY_MODE_COMPLETE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Security Mode Complete\n");

      for (i = 0; i < sdu_sizeP; i++) eNB->pusch_vars[UE_id]{
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif

      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" securityModeComplete UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

      LOG_I(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" received securityModeComplete on UL-DCCH %d from UE\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH);
      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(securityModeComplete) ---> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);
#ifdef XER_PRINT
      xer_fprint(stdout, &asn_DEF_UL_DCCH_Message, (void *)ul_dcch_msg);
#endif
      // confirm with PDCP about the security mode for DCCH
      //rrc_pdcp_config_req (enb_mod_idP, frameP, 1,CONFIG_ACTION_SET_SECURITY_MODE, (ue_mod_idP * NB_RB_MAX) + DCCH, 0x77);
      // continue the procedure
      rrc_eNB_generate_UECapabilityEnquiry(
        ctxt_pP,
        ue_context_p);
      break;

    case UL_DCCH_MessageType__c1_PR_securityModeFailure:
      T(T_ENB_RRC_SECURITY_MODE_FAILURE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC Security Mode Failure\n");

      for (i = 0; i < sdu_sizeP; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif

      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" securityModeFailure UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

      LOG_W(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(securityModeFailure) ---> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);
#ifdef XER_PRINT
      xer_fprint(stdout, &asn_DEF_UL_DCCH_Message, (void *)ul_dcch_msg);
#endif
      // cancel the security mode in PDCP

      // followup with the remaining procedure
//#warning "LG Removed rrc_eNB_generate_UECapabilityEnquiry after receiving securityModeFailure"
      rrc_eNB_generate_UECapabilityEnquiry(ctxt_pP, ue_context_p);
      break;

    case UL_DCCH_MessageType__c1_PR_ueCapabilityInformation:
      T(T_ENB_RRC_UE_CAPABILITY_INFORMATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC UECapablility Information \n");

      for (i = 0; i < sdu_sizeP; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif

      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" ueCapabilityInformation UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

      LOG_I(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" received ueCapabilityInformation on UL-DCCH %d from UE\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH);
      LOG_D(RRC,
            PROTOCOL_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(UECapabilityInformation) ---> RRC_eNB\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);
#ifdef XER_PRINT
      xer_fprint(stdout, &asn_DEF_UL_DCCH_Message, (void *)ul_dcch_msg);
#endif
      LOG_I(RRC, "got UE capabilities for UE %x\n", ctxt_pP->rnti);
      if (ue_context_p->ue_context.UE_Capability) {
        LOG_I(RRC, "freeing old UE capabilities for UE %x\n", ctxt_pP->rnti);
        asn_DEF_UE_EUTRA_Capability.free_struct(&asn_DEF_UE_EUTRA_Capability,
              ue_context_p->ue_context.UE_Capability, 0);
        ue_context_p->ue_context.UE_Capability = 0;
      }
      dec_rval = uper_decode(NULL,
                             &asn_DEF_UE_EUTRA_Capability,
                             (void **)&ue_context_p->ue_context.UE_Capability,
                             ul_dcch_msg->message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.
                             choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list.
                             array[0]->ueCapabilityRAT_Container.buf,
                             ul_dcch_msg->message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.
                             choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list.
                             array[0]->ueCapabilityRAT_Container.size, 0, 0);
#ifdef XER_PRINT
      xer_fprint(stdout, &asn_DEF_UE_EUTRA_Capability, ue_context_p->ue_context.UE_Capability);
#endif

      if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
        LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Failed to decode UE capabilities (%zu bytes)\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
              dec_rval.consumed);
        asn_DEF_UE_EUTRA_Capability.free_struct(&asn_DEF_UE_EUTRA_Capability,
              ue_context_p->ue_context.UE_Capability, 0);
        ue_context_p->ue_context.UE_Capability = 0;
      }

#if defined(ENABLE_USE_MME)

      if (EPC_MODE_ENABLED == 1) {
        rrc_eNB_send_S1AP_UE_CAPABILITIES_IND(ctxt_pP,
                                              ue_context_p,
                                              ul_dcch_msg);
      }
#else 
      ue_context_p->ue_context.nb_of_e_rabs = 1;
      for (i = 0; i < ue_context_p->ue_context.nb_of_e_rabs; i++){
	ue_context_p->ue_context.e_rab[i].status = E_RAB_STATUS_NEW;
	ue_context_p->ue_context.e_rab[i].param.e_rab_id = 1+i;
	ue_context_p->ue_context.e_rab[i].param.qos.qci=9;
      }
      ue_context_p->ue_context.setup_e_rabs =ue_context_p->ue_context.nb_of_e_rabs;
#endif

      rrc_eNB_generate_defaultRRCConnectionReconfiguration(ctxt_pP,
          ue_context_p,
          RC.rrc[ctxt_pP->module_id]->HO_flag);
      break;

    case UL_DCCH_MessageType__c1_PR_ulHandoverPreparationTransfer:
      T(T_ENB_RRC_UL_HANDOVER_PREPARATION_TRANSFER, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;

    case UL_DCCH_MessageType__c1_PR_ulInformationTransfer:
      T(T_ENB_RRC_UL_INFORMATION_TRANSFER, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      LOG_D(RRC,"[MSG] RRC UL Information Transfer \n");
#ifdef RRC_MSG_PRINT
      LOG_F(RRC,"[MSG] RRC UL Information Transfer \n");

      for (i = 0; i < sdu_sizeP; i++) {
        LOG_F(RRC,"%02x ", ((uint8_t*)Rx_sdu)[i]);
      }

      LOG_F(RRC,"\n");
#endif


      MSC_LOG_RX_MESSAGE(
        MSC_RRC_ENB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" ulInformationTransfer UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);

#if defined(ENABLE_USE_MME)

      if (EPC_MODE_ENABLED == 1) {
        rrc_eNB_send_S1AP_UPLINK_NAS(ctxt_pP,
                                     ue_context_p,
                                     ul_dcch_msg);
      }

#endif
      break;

    case UL_DCCH_MessageType__c1_PR_counterCheckResponse:
      T(T_ENB_RRC_COUNTER_CHECK_RESPONSE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;

#if defined(Rel10) || defined(Rel14)

    case UL_DCCH_MessageType__c1_PR_ueInformationResponse_r9:
      T(T_ENB_RRC_UE_INFORMATION_RESPONSE_R9, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;

    case UL_DCCH_MessageType__c1_PR_proximityIndication_r9:
      T(T_ENB_RRC_PROXIMITY_INDICATION_R9, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;

    case UL_DCCH_MessageType__c1_PR_rnReconfigurationComplete_r10:
      T(T_ENB_RRC_RECONFIGURATION_COMPLETE_R10, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;

    case UL_DCCH_MessageType__c1_PR_mbmsCountingResponse_r10:
      T(T_ENB_RRC_MBMS_COUNTING_RESPONSE_R10, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;

    case UL_DCCH_MessageType__c1_PR_interFreqRSTDMeasurementIndication_r10:
      T(T_ENB_RRC_INTER_FREQ_RSTD_MEASUREMENT_INDICATION, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      break;
#endif

    default:
      T(T_ENB_RRC_UNKNOW_MESSAGE, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
        T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));

      LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Unknown message %s:%u\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            __FILE__, __LINE__);
      return -1;
    }

    return 0;
  } else {
    LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Unknown error %s:%u\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          __FILE__, __LINE__);
    return -1;
  }

}

#if defined(ENABLE_ITTI)
void rrc_eNB_reconfigure_DRBs (const protocol_ctxt_t* const ctxt_pP,
			       rrc_eNB_ue_context_t*  ue_context_pP){

  int i;
  int e_rab_done=0;
  for (i = 0; 
       i < 3;//NB_RB_MAX - 3;  // S1AP_MAX_E_RAB
       i++) {
    
    if ( ue_context_pP->ue_context.e_rab[i].status < E_RAB_STATUS_DONE){ 
      ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_NEW;
      ue_context_pP->ue_context.e_rab[i].param.e_rab_id = i + 1;
      ue_context_pP->ue_context.e_rab[i].param.qos.qci = i % 9;
      ue_context_pP->ue_context.e_rab[i].param.qos.allocation_retention_priority.priority_level= i % PRIORITY_LEVEL_LOWEST;
      ue_context_pP->ue_context.e_rab[i].param.qos.allocation_retention_priority.pre_emp_capability= PRE_EMPTION_CAPABILITY_DISABLED;
      ue_context_pP->ue_context.e_rab[i].param.qos.allocation_retention_priority.pre_emp_vulnerability= PRE_EMPTION_VULNERABILITY_DISABLED;
      ue_context_pP->ue_context.e_rab[i].param.nas_pdu.buffer = NULL;
      ue_context_pP->ue_context.e_rab[i].param.nas_pdu.length = 0;
      //	memset (ue_context_pP->ue_context.e_rab[i].param.sgw_addr.buffer,0,20);
      ue_context_pP->ue_context.e_rab[i].param.sgw_addr.length = 0;
      ue_context_pP->ue_context.e_rab[i].param.gtp_teid=0;
      
      ue_context_pP->ue_context.nb_of_e_rabs++;
      e_rab_done++;
      LOG_I(RRC,"setting up the dedicated DRBs %d (index %d) status %d \n", 
	    ue_context_pP->ue_context.e_rab[i].param.e_rab_id, i, ue_context_pP->ue_context.e_rab[i].status);
    }
  }
  ue_context_pP->ue_context.setup_e_rabs+=e_rab_done;
 
  rrc_eNB_generate_dedicatedRRCConnectionReconfiguration(ctxt_pP, ue_context_pP, 0);
}


//-----------------------------------------------------------------------------
void*
rrc_enb_task(
  void* args_p
)
//-----------------------------------------------------------------------------
{
  MessageDef                         *msg_p;
  const char                         *msg_name_p;
  instance_t                          instance;
  int                                 result;
  SRB_INFO                           *srb_info_p;
  int                                 CC_id;

  protocol_ctxt_t                     ctxt;
  itti_mark_task_ready(TASK_RRC_ENB);
  LOG_I(RRC,"Entering main loop of RRC message task\n");
  while (1) {
    // Wait for a message
    itti_receive_msg(TASK_RRC_ENB, &msg_p);

    msg_name_p = ITTI_MSG_NAME(msg_p);
    instance = ITTI_MSG_INSTANCE(msg_p);
    LOG_I(RRC,"Received message %s\n",msg_name_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(RRC, " *** Exiting RRC thread\n");
      itti_exit_task();
      break;

    case MESSAGE_TEST:
      LOG_I(RRC, "[eNB %d] Received %s\n", instance, msg_name_p);
      break;

      /* Messages from MAC */
    case RRC_MAC_CCCH_DATA_IND:
      PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt,
                                    instance,
                                    ENB_FLAG_YES,
                                    RRC_MAC_CCCH_DATA_IND(msg_p).rnti,
                                    msg_p->ittiMsgHeader.lte_time.frame,
                                    msg_p->ittiMsgHeader.lte_time.slot);
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Received %s\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(&ctxt),
            msg_name_p);

      CC_id = RRC_MAC_CCCH_DATA_IND(msg_p).CC_id;
      srb_info_p = &RC.rrc[instance]->carrier[CC_id].Srb0;

      LOG_I(RRC,"Decoding CCCH : inst %d, CC_id %d, ctxt %p, sib_info_p->Rx_buffer.payload_size %d\n",
	    instance,CC_id,&ctxt, RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size);
      AssertFatal(RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size < RRC_BUFFER_SIZE_MAX,
		  "CCCH message has size %d > %d\n",
		  RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size,RRC_BUFFER_SIZE_MAX);
      memcpy(srb_info_p->Rx_buffer.Payload,
             RRC_MAC_CCCH_DATA_IND(msg_p).sdu,
             RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size);
      srb_info_p->Rx_buffer.payload_size = RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size;

      rrc_eNB_decode_ccch(&ctxt, srb_info_p, CC_id);
      break;
	    
      /* Messages from PDCP */
    case RRC_DCCH_DATA_IND:
      PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt,
                                    instance,
                                    ENB_FLAG_YES,
                                    RRC_DCCH_DATA_IND(msg_p).rnti,
                                    msg_p->ittiMsgHeader.lte_time.frame,
                                    msg_p->ittiMsgHeader.lte_time.slot);
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Received on DCCH %d %s\n",
            PROTOCOL_RRC_CTXT_UE_ARGS(&ctxt),
            RRC_DCCH_DATA_IND(msg_p).dcch_index,
            msg_name_p);
      rrc_eNB_decode_dcch(&ctxt,
                          RRC_DCCH_DATA_IND(msg_p).dcch_index,
                          RRC_DCCH_DATA_IND(msg_p).sdu_p,
                          RRC_DCCH_DATA_IND(msg_p).sdu_size);

      // Message buffer has been processed, free it now.
      result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), RRC_DCCH_DATA_IND(msg_p).sdu_p);
      AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      break;

#   if defined(ENABLE_USE_MME)

      /* Messages from S1AP */
    case S1AP_DOWNLINK_NAS:
      rrc_eNB_process_S1AP_DOWNLINK_NAS(msg_p, msg_name_p, instance, &rrc_eNB_mui);
      break;

    case S1AP_INITIAL_CONTEXT_SETUP_REQ:
      rrc_eNB_process_S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_UE_CTXT_MODIFICATION_REQ:
      rrc_eNB_process_S1AP_UE_CTXT_MODIFICATION_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_PAGING_IND:
      LOG_D(RRC, "[eNB %d] Received Paging message from S1AP: %s\n", instance, msg_name_p);
      rrc_eNB_process_PAGING_IND(msg_p, msg_name_p, instance);
      break;
  
    case S1AP_E_RAB_SETUP_REQ: 
      rrc_eNB_process_S1AP_E_RAB_SETUP_REQ(msg_p, msg_name_p, instance);
      LOG_D(RRC, "[eNB %d] Received the message %s\n", instance, msg_name_p);
      break;

    case S1AP_E_RAB_MODIFY_REQ:
      rrc_eNB_process_S1AP_E_RAB_MODIFY_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_E_RAB_RELEASE_COMMAND:
      rrc_eNB_process_S1AP_E_RAB_RELEASE_COMMAND(msg_p, msg_name_p, instance);
      break;
    
    case S1AP_UE_CONTEXT_RELEASE_REQ:
      rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_UE_CONTEXT_RELEASE_COMMAND:
      rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_COMMAND(msg_p, msg_name_p, instance);
      break;

    case GTPV1U_ENB_DELETE_TUNNEL_RESP:
      /* Nothing to do. Apparently everything is done in S1AP processing */
      //LOG_I(RRC, "[eNB %d] Received message %s, not processed because procedure not synched\n",
      //instance, msg_name_p);
      if (rrc_eNB_get_ue_context(RC.rrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)
          && rrc_eNB_get_ue_context(RC.rrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_rrc > 0) {
        rrc_eNB_get_ue_context(RC.rrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_rrc =
        rrc_eNB_get_ue_context(RC.rrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_thres_rrc;
      }
      break;

#   endif

      /* Messages from eNB app */
    case RRC_CONFIGURATION_REQ:
      LOG_I(RRC, "[eNB %d] Received %s : %p\n", instance, msg_name_p,&RRC_CONFIGURATION_REQ(msg_p));
      openair_rrc_eNB_configuration(ENB_INSTANCE_TO_MODULE_ID(instance), &RRC_CONFIGURATION_REQ(msg_p));
      break;

    default:
      LOG_E(RRC, "[eNB %d] Received unexpected message %s\n", instance, msg_name_p);
      break;
    }

    result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}
#endif

/*------------------------------------------------------------------------------*/
void
openair_rrc_top_init_eNB(int eMBMS_active,uint8_t HO_active)
//-----------------------------------------------------------------------------
{

  module_id_t         module_id;
  int                 CC_id;

  /* for no gcc warnings */
  (void)CC_id;

  LOG_D(RRC, "[OPENAIR][INIT] Init function start: NB_eNB_INST=%d\n", RC.nb_inst);

  if (RC.nb_inst > 0) {
    LOG_I(RRC,"[eNB] handover active state is %d \n", HO_active);

    for (module_id=0; module_id<NB_eNB_INST; module_id++) {
      RC.rrc[module_id]->HO_flag   = (uint8_t)HO_active;
    }

#if defined(Rel10) || defined(Rel14)
    LOG_I(RRC,"[eNB] eMBMS active state is %d \n", eMBMS_active);

    for (module_id=0; module_id<NB_eNB_INST; module_id++) {
      for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
        RC.rrc[module_id]->carrier[CC_id].MBMS_flag = (uint8_t)eMBMS_active;
      }
    }

#endif
#ifdef CBA

    for (module_id=0; module_id<RC.nb_inst; module_id++) {
      for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
        RC.rrc[module_id]->carrier[CC_id].num_active_cba_groups = cba_group_active;
      }
    }

#endif
  } 


}

//-----------------------------------------------------------------------------
void
rrc_top_cleanup_eNB(
  void
)
//-----------------------------------------------------------------------------
{

  for (int i=0;i<RC.nb_inst;i++) free (RC.rrc[i]);
  free(RC.rrc);
}


//-----------------------------------------------------------------------------
RRC_status_t
rrc_rx_tx(
  protocol_ctxt_t* const ctxt_pP,
  const int          CC_id
)
//-----------------------------------------------------------------------------
{
  //uint8_t        UE_id;
  int32_t        current_timestamp_ms, ref_timestamp_ms;
  struct timeval ts;
  struct rrc_eNB_ue_context_s   *ue_context_p = NULL,*ue_to_be_removed = NULL;

#ifdef LOCALIZATION
  double                         estimated_distance;
  protocol_ctxt_t                ctxt;
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_IN);

    check_handovers(ctxt_pP);
    // counetr, and get the value and aggregate

    // check for UL failure
    RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
      if ((ctxt_pP->frame == 0) && (ctxt_pP->subframe==0)) {
	if (ue_context_p->ue_context.Initialue_identity_s_TMSI.presence == TRUE) {
	  LOG_I(RRC,"UE rnti %x:S-TMSI %x failure timer %d/8\n",
		ue_context_p->ue_context.rnti,
		ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi,
		ue_context_p->ue_context.ul_failure_timer);
	}
	else {
	  LOG_I(RRC,"UE rnti %x failure timer %d/8\n",
		ue_context_p->ue_context.rnti,
		ue_context_p->ue_context.ul_failure_timer);
	}
      }
      if (ue_context_p->ue_context.ul_failure_timer>0) {
	ue_context_p->ue_context.ul_failure_timer++;
	if (ue_context_p->ue_context.ul_failure_timer >= 8) {
	  // remove UE after 20 seconds after MAC has indicated UL failure
	  LOG_I(RRC,"Removing UE %x instance\n",ue_context_p->ue_context.rnti);
	  ue_to_be_removed = ue_context_p;
	  break;
	}
      }
      if (ue_context_p->ue_context.ue_release_timer_s1>0) {
        ue_context_p->ue_context.ue_release_timer_s1++;
        if (ue_context_p->ue_context.ue_release_timer_s1 >=
            ue_context_p->ue_context.ue_release_timer_thres_s1) {
          LOG_I(RRC,"Removing UE %x instance Because of UE_CONTEXT_RELEASE_COMMAND not received after %d ms from sending request\n",
                         ue_context_p->ue_context.rnti, ue_context_p->ue_context.ue_release_timer_thres_s1);
          ue_to_be_removed = ue_context_p;
          break;
        }
      }

      if (ue_context_p->ue_context.ue_release_timer_rrc>0) {
        ue_context_p->ue_context.ue_release_timer_rrc++;
        if (ue_context_p->ue_context.ue_release_timer_rrc >=
          ue_context_p->ue_context.ue_release_timer_thres_rrc) {
          LOG_I(RRC,"Removing UE %x instance After UE_CONTEXT_RELEASE_Complete\n", ue_context_p->ue_context.rnti);
          ue_to_be_removed = ue_context_p;
          break;
        }
      }

      if (ue_context_p->ue_context.ue_reestablishment_timer>0) {
        ue_context_p->ue_context.ue_reestablishment_timer++;
        if (ue_context_p->ue_context.ue_reestablishment_timer >=
            ue_context_p->ue_context.ue_reestablishment_timer_thres) {
          LOG_I(RRC,"UE %d reestablishment_timer max\n",ue_context_p->ue_context.rnti);
          ue_context_p->ue_context.ul_failure_timer = 20000;
          ue_to_be_removed = ue_context_p;
          ue_context_p->ue_context.ue_reestablishment_timer = 0;
          break;
        }
      }
      if (ue_context_p->ue_context.ue_release_timer>0) {
	ue_context_p->ue_context.ue_release_timer++;
	if (ue_context_p->ue_context.ue_release_timer >= 
	    ue_context_p->ue_context.ue_release_timer_thres) {
	  LOG_I(RRC,"Removing UE %x instance\n",ue_context_p->ue_context.rnti);
	  ue_to_be_removed = ue_context_p;
	  break;
	}
      }
    }
    if (ue_to_be_removed) {
      if(ue_to_be_removed->ue_context.ul_failure_timer >= 8) {
          ue_to_be_removed->ue_context.ue_release_timer_s1 = 1;
          ue_to_be_removed->ue_context.ue_release_timer_thres_s1 = 100;
          ue_to_be_removed->ue_context.ue_release_timer = 0;
          ue_to_be_removed->ue_context.ue_reestablishment_timer = 0;
      }
      rrc_eNB_free_UE(ctxt_pP->module_id,ue_to_be_removed);
      if(ue_to_be_removed->ue_context.ul_failure_timer >= 8){
        ue_to_be_removed->ue_context.ul_failure_timer = 0;
      }
    }

#ifdef RRC_LOCALIZATION

    /* for the localization, only primary CC_id might be relevant*/
    gettimeofday(&ts, NULL);
    current_timestamp_ms = ts.tv_sec * 1000 + ts.tv_usec / 1000;
    ref_timestamp_ms = RC.rrc[ctxt_pP->module_id]->reference_timestamp_ms;
    RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
      ctxt = *ctxt_pP;
      ctxt.rnti = ue_context_p->ue_context.rnti;
      estimated_distance = rrc_get_estimated_ue_distance(
                             &ctxt,
                             CC_id,
                             RC.rrc[ctxt_pP->module_id]->loc_type);

      if ((current_timestamp_ms - ref_timestamp_ms > RC.rrc[ctxt_pP->module_id]->aggregation_period_ms) &&
          estimated_distance != -1) {
        LOG_D(LOCALIZE, " RRC [UE/id %d -> eNB/id %d] timestamp %d frame %d estimated r = %f\n",
              ctxt.rnti,
              ctxt_pP->module_id,
              current_timestamp_ms,
              ctxt_pP->frame,
              estimated_distance);
        LOG_D(LOCALIZE, " RRC status %d\n", ue_context_p->ue_context.Status);
        push_front(&RC.rrc[ctxt_pP->module_id]->loc_list,
                   estimated_distance);
        RC.rrc[ctxt_pP->module_id]->reference_timestamp_ms = current_timestamp_ms;
      }
    }

#endif
    (void)ts; /* remove gcc warning "unused variable" */
    (void)ref_timestamp_ms; /* remove gcc warning "unused variable" */
    (void)current_timestamp_ms; /* remove gcc warning "unused variable" */

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
  return (RRC_OK);
}

