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

/*! \file asn1_msg.c
* \brief primitives to build the asn1 messages
* \author Raymond Knopp and Navid Nikaein, WEI-TAI CHEN
* \date 2011, 2018
* \version 1.0
* \company Eurecom, NTUST
* \email: raymond.knopp@eurecom.fr and  navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h> /* for atoi(3) */
#include <unistd.h> /* for getopt(3) */
#include <string.h> /* for strerror(3) */
#include <sysexits.h> /* for EX_* exit codes */
#include <errno.h>  /* for errno */
#include "common/utils/LOG/log.h"
#include <asn_application.h>
#include <asn_internal.h> /* for _ASN_DEFAULT_STACK_MAX */
#include <per_encoder.h>

#include "asn1_msg.h"
#include "RRC/NR/nr_rrc_extern.h"

#if defined(NR_Rel15)
#include "NR_SCS-SpecificCarrier.h"
#include "NR_TDD-UL-DL-ConfigCommon.h"
#include "NR_FrequencyInfoUL.h"
#include "NR_FrequencyInfoDL.h"
#include "NR_RACH-ConfigGeneric.h"
#include "NR_RACH-ConfigCommon.h"
#include "NR_PUSCH-TimeDomainResourceAllocation.h"
#include "NR_PUSCH-ConfigCommon.h"
#include "NR_PUCCH-ConfigCommon.h"
#include "NR_PDSCH-TimeDomainResourceAllocation.h"
#include "NR_PDSCH-ConfigCommon.h"
#include "NR_RateMatchPattern.h"
#include "NR_RateMatchPatternLTE-CRS.h"
#include "NR_SearchSpace.h"
#include "NR_ControlResourceSet.h"
#include "NR_EUTRA-MBSFN-SubframeConfig.h"
#include "NR_BWP-DownlinkCommon.h"
#include "NR_BWP-DownlinkDedicated.h"
#include "NR_UplinkConfigCommon.h"
#include "NR_SetupRelease.h"
#include "NR_PDCCH-ConfigCommon.h"
#include "NR_BWP-UplinkCommon.h"

#include "assertions.h"
//#include "RRCConnectionRequest.h"
//#include "UL-CCCH-Message.h"
#include "NR_UL-DCCH-Message.h"
//#include "DL-CCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
//#include "EstablishmentCause.h"
//#include "RRCConnectionSetup.h"
#include "NR_SRB-ToAddModList.h"
#include "NR_DRB-ToAddModList.h"
//#include "MCCH-Message.h"
//#define MRB1 1

//#include "RRCConnectionSetupComplete.h"
//#include "RRCConnectionReconfigurationComplete.h"
//#include "RRCConnectionReconfiguration.h"
#include "NR_MIB.h"
//#include "SystemInformation.h"

#include "NR_SIB1.h"
#include "NR_ServingCellConfigCommon.h"
//#include "SIB-Type.h"

//#include "BCCH-DL-SCH-Message.h"

//#include "PHY/defs.h"

#include "NR_MeasObjectToAddModList.h"
#include "NR_ReportConfigToAddModList.h"
#include "NR_MeasIdToAddModList.h"
#include "gnb_config.h"
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "common/ran_context.h"

//#include "PHY/defs.h"
/*#ifndef USER_MODE
#define msg printk
#ifndef errno
int errno;
#endif
#else
# if !defined (msg)
#   define msg printf
# endif
#endif*/

//#define XER_PRINT

typedef struct xer_sprint_string_s {
  char *string;
  size_t string_size;
  size_t string_index;
} xer_sprint_string_t;

//repplace LTE
//extern unsigned char NB_eNB_INST;
extern unsigned char NB_gNB_INST;
extern uint8_t usim_test;

extern RAN_CONTEXT_t RC;

uint16_t two_tier_hexagonal_nr_cellIds[7] = {0,1,2,4,5,7,8};
uint16_t two_tier_hexagonal_adjacent_nr_cellIds[7][6] = {{1,2,4,5,7,8},    // CellId 0
  {11,18,2,0,8,15}, // CellId 1
  {18,13,3,4,0,1},  // CellId 2
  {2,3,14,6,5,0},   // CellId 4
  {0,4,6,16,9,7},   // CellId 5
  {8,0,5,9,17,12},  // CellId 7
  {15,1,0,7,12,10}
};// CellId 8

/*
 * This is a helper function for xer_sprint, which directs all incoming data
 * into the provided string.
 */
static int xer__nr_print2s (const void *buffer, size_t size, void *app_key)
{
  xer_sprint_string_t *string_buffer = (xer_sprint_string_t *) app_key;
  size_t string_remaining = string_buffer->string_size - string_buffer->string_index;

  if (string_remaining > 0) {
    if (size > string_remaining) {
      size = string_remaining;
    }

    memcpy(&string_buffer->string[string_buffer->string_index], buffer, size);
    string_buffer->string_index += size;
  }

  return 0;
}

int xer_nr_sprint (char *string, size_t string_size, asn_TYPE_descriptor_t *td, void *sptr)
{
  asn_enc_rval_t er;
  xer_sprint_string_t string_buffer;

  string_buffer.string = string;
  string_buffer.string_size = string_size;
  string_buffer.string_index = 0;

  er = xer_encode(td, sptr, XER_F_BASIC, xer__nr_print2s, &string_buffer);

  if (er.encoded < 0) {
    LOG_E(RRC, "xer_sprint encoding error (%zd)!", er.encoded);
    er.encoded = string_buffer.string_size;
  } else {
    if (er.encoded > string_buffer.string_size) {
      LOG_E(RRC, "xer_sprint string buffer too small, got %zd need %zd!", string_buffer.string_size, er.encoded);
      er.encoded = string_buffer.string_size;
    }
  }

  return er.encoded;
}

uint16_t get_adjacent_cell_id_NR(uint8_t Mod_id,uint8_t index)
{
  return(two_tier_hexagonal_adjacent_nr_cellIds[Mod_id][index]);
}
/* This only works for the hexagonal topology...need a more general function for other topologies */

uint8_t get_adjacent_cell_mod_id_NR(uint16_t phyCellId)
{
  uint8_t i;

  for(i=0; i<7; i++) {
    if(two_tier_hexagonal_nr_cellIds[i] == phyCellId) {
      return i;
    }
  }

  LOG_E(RRC,"\nCannot get adjacent cell mod id! Fatal error!\n");
  return 0xFF; //error!
}

//------------------------------------------------------------------------------

uint8_t do_MIB_NR(rrc_gNB_carrier_data_t *carrier, 
                  uint32_t frame, 
                  uint32_t ssb_SubcarrierOffset, 
                  uint32_t pdcch_ConfigSIB1, 
                  uint32_t subCarrierSpacingCommon, 
                  uint32_t dmrs_TypeA_Position)
{

  asn_enc_rval_t enc_rval;

  NR_BCCH_BCH_Message_t *mib = &carrier->mib;
  memset(mib,0,sizeof(NR_BCCH_BCH_Message_t));
  mib->message.present = NR_BCCH_BCH_MessageType_PR_mib;
  
  mib->message.choice.mib = CALLOC(1,sizeof(struct NR_MIB));
  memset(mib->message.choice.mib,0,sizeof(struct NR_MIB));
  //36.331 SFN BIT STRING (SIZE (8)  , 38.331 SFN BIT STRING (SIZE (6))
  uint8_t sfn_msb = (uint8_t)((frame>>4)&0x3f);
  mib->message.choice.mib->systemFrameNumber.buf = CALLOC(1,sizeof(uint8_t));
  mib->message.choice.mib->systemFrameNumber.buf[0] = sfn_msb << 2;
  mib->message.choice.mib->systemFrameNumber.size = 1;
  mib->message.choice.mib->systemFrameNumber.bits_unused=2;

  //38.331 spare BIT STRING (SIZE (1))
  uint16_t *spare= calloc(1, sizeof(uint16_t));
  if (spare == NULL) abort();
  mib->message.choice.mib->spare.buf = (uint8_t *)spare;
  mib->message.choice.mib->spare.size = 1;
  mib->message.choice.mib->spare.bits_unused = 7;  // This makes a spare of 1 bits

  mib->message.choice.mib->ssb_SubcarrierOffset = ssb_SubcarrierOffset;
  mib->message.choice.mib->pdcch_ConfigSIB1 = pdcch_ConfigSIB1;
  
  switch (subCarrierSpacingCommon) {
    case 15:
      mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60;
      break;

    case 30:
      mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs30or120;
      break;

    case 60:
      mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60;
      break;

    case 120:
      mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs30or120;
      break;

    default:
    AssertFatal(1==0,"Unknown subCarrierSpacingCommon %d\n",subCarrierSpacingCommon);
  }

    switch (dmrs_TypeA_Position) {
    case 2:
      mib->message.choice.mib->dmrs_TypeA_Position = NR_MIB__dmrs_TypeA_Position_pos2;
      break;

    case 3:
      mib->message.choice.mib->dmrs_TypeA_Position = NR_MIB__dmrs_TypeA_Position_pos3;
      break;

    default:
    AssertFatal(1==0,"Unknown dmrs_TypeA_Position %d\n",dmrs_TypeA_Position);

  }

  //  assign_enum
  mib->message.choice.mib->cellBarred = NR_MIB__cellBarred_notBarred;
  //  assign_enum
  mib->message.choice.mib->intraFreqReselection = NR_MIB__intraFreqReselection_notAllowed;

  

  //encode MIB to data
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                   NULL,
                                   (void*)mib,
                                   carrier->MIB,
                                   24);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);


  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

void do_SERVINGCELLCONFIGCOMMON(uint8_t Mod_id,
                                int     CC_id,
                                #if defined(ENABLE_ITTI)
                                gNB_RrcConfigurationReq *configuration,
                                #endif
                                int initial_flag
                                )
{ 
  NR_ServingCellConfigCommon_t    **servingcellconfigcommon  =  &RC.nrrrc[Mod_id]->carrier[CC_id].servingcellconfigcommon;
  NR_FreqBandIndicatorNR_t                        *dl_frequencyBandList;
  struct NR_SCS_SpecificCarrier                   *dl_scs_SpecificCarrierList;
  NR_TCI_StateId_t                                *TCI_StateId;
  struct NR_ControlResourceSet                    *bwp_dl_controlresourceset;
  NR_SearchSpace_t                                *bwp_dl_searchspace;
  struct NR_PDSCH_TimeDomainResourceAllocation    *bwp_dl_timedomainresourceallocation;
  NR_FreqBandIndicatorNR_t                        *ul_frequencyBandList;
  struct NR_SCS_SpecificCarrier                   *ul_scs_SpecificCarrierList;
  struct NR_PUSCH_TimeDomainResourceAllocation    *pusch_configcommontimedomainresourceallocation;
  struct NR_RateMatchPattern                      *ratematchpattern;
  NR_RateMatchPatternId_t                         *ratematchpatternid;

  if(initial_flag == 1){
    (*servingcellconfigcommon)                                            = CALLOC(1,sizeof(NR_ServingCellConfigCommon_t));
    (*servingcellconfigcommon)->physCellId                                = CALLOC(1,sizeof(NR_PhysCellId_t));
    (*servingcellconfigcommon)->downlinkConfigCommon                      = CALLOC(1,sizeof(struct NR_DownlinkConfigCommon));
    (*servingcellconfigcommon)->downlinkConfigCommon->frequencyInfoDL     = CALLOC(1,sizeof(struct NR_FrequencyInfoDL));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP  = CALLOC(1,sizeof(struct NR_BWP_DownlinkCommon));
    (*servingcellconfigcommon)->uplinkConfigCommon                        = CALLOC(1,sizeof(struct NR_UplinkConfigCommon));
    (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL       = CALLOC(1,sizeof(struct NR_FrequencyInfoUL));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP      = CALLOC(1,sizeof(struct NR_BWP_UplinkCommon));
    //(*servingcellconfigcommon)->supplementaryUplinkConfig       = CALLOC(1,sizeof(struct NR_UplinkConfigCommon));  
    (*servingcellconfigcommon)->ssb_PositionsInBurst                      = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__ssb_PositionsInBurst));
    (*servingcellconfigcommon)->ssb_periodicityServingCell                = CALLOC(1,sizeof(long));
    //(*servingcellconfigcommon)->lte_CRS_ToMatchAround           = CALLOC(1,sizeof(struct NR_SetupRelease_RateMatchPatternLTE_CRS));
    (*servingcellconfigcommon)->rateMatchPatternToAddModList              = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__rateMatchPatternToAddModList));
    (*servingcellconfigcommon)->rateMatchPatternToReleaseList             = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__rateMatchPatternToReleaseList));
    (*servingcellconfigcommon)->subcarrierSpacing                         = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
    (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon             = CALLOC(1,sizeof(struct NR_TDD_UL_DL_ConfigCommon));


    (*servingcellconfigcommon)->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB     = CALLOC(1,sizeof(NR_ARFCN_ValueNR_t));
    
    dl_frequencyBandList              = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t));
    dl_scs_SpecificCarrierList        = CALLOC(1,sizeof(struct NR_SCS_SpecificCarrier));

    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->genericParameters.cyclicPrefix    = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon                = CALLOC(1,sizeof(struct NR_SetupRelease_PDCCH_ConfigCommon));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup  = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero    = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero           = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourceSet  = CALLOC(1,sizeof(struct NR_ControlResourceSet));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpace         = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon__commonSearchSpace));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1                    = CALLOC(1,sizeof(NR_SearchSpaceId_t));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation  = CALLOC(1,sizeof(NR_SearchSpaceId_t));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->pagingSearchSpace                  = CALLOC(1,sizeof(NR_SearchSpaceId_t));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace                     = CALLOC(1,sizeof(NR_SearchSpaceId_t));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon                 = CALLOC(1,sizeof(struct NR_SetupRelease_PDSCH_ConfigCommon));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_PDSCH_ConfigCommon));
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList = CALLOC(1,sizeof(struct NR_PDSCH_TimeDomainResourceAllocationList));

    (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyBandList          = CALLOC(1,sizeof(struct NR_MultiFrequencyBandListNR));
    (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA    = CALLOC(1,sizeof(NR_ARFCN_ValueNR_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->additionalSpectrumEmission = CALLOC(1,sizeof(NR_AdditionalSpectrumEmission_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->p_Max                      = CALLOC(1,sizeof(NR_P_Max_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyShift7p5khz       = CALLOC(1,sizeof(long));

    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.cyclicPrefix    = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon                 = CALLOC(1,sizeof(NR_SetupRelease_RACH_ConfigCommon_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles                  = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB  = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured                           = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon__groupBconfigured));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB            = CALLOC(1,sizeof(NR_RSRP_Range_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB_SUL        = CALLOC(1,sizeof(NR_RSRP_Range_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing       = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoding      = CALLOC(1,sizeof(long));
    
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon                 = CALLOC(1,sizeof(NR_SetupRelease_PUSCH_ConfigCommon_t)); 
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_PUSCH_ConfigCommon));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = CALLOC(1,sizeof(long));

    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList  = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocationList));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble              = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant             = CALLOC(1,sizeof(long));

    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon                                                = CALLOC(1,sizeof(struct NR_SetupRelease_PUCCH_ConfigCommon)); 
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup                                  = CALLOC(1,sizeof(struct NR_PUCCH_ConfigCommon));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon            = CALLOC(1,sizeof(BIT_STRING_t));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal                      = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon            = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId                       = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.buf  = MALLOC(1);
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.buf = MALLOC(1);
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf   = MALLOC(8);

    bwp_dl_controlresourceset =(*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourceSet;
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved               = CALLOC(1,sizeof(struct NR_ControlResourceSet__cce_REG_MappingType__interleaved));
    bwp_dl_controlresourceset->frequencyDomainResources.buf                         = MALLOC(6);
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->shiftIndex   = CALLOC(1,sizeof(long));
    bwp_dl_controlresourceset->tci_StatesPDCCH_ToAddList                            = CALLOC(1,sizeof(struct NR_ControlResourceSet__tci_StatesPDCCH_ToAddList));
    bwp_dl_controlresourceset->tci_PresentInDCI                                     = CALLOC(1,sizeof(long));
    bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID                              = CALLOC(1,sizeof(long));
    TCI_StateId                                                                     = CALLOC(1,sizeof(NR_TCI_StateId_t));
    bwp_dl_searchspace                                                              = CALLOC(1,sizeof(NR_SearchSpace_t));
    bwp_dl_searchspace->controlResourceSetId                                        = CALLOC(1,sizeof(NR_ControlResourceSetId_t));
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset                          = CALLOC(1,sizeof(struct NR_SearchSpace__monitoringSlotPeriodicityAndOffset));
    bwp_dl_searchspace->duration                                                    = CALLOC(1,sizeof(long));
    bwp_dl_searchspace->monitoringSymbolsWithinSlot                                 = CALLOC(1,sizeof(BIT_STRING_t));
    bwp_dl_searchspace->monitoringSymbolsWithinSlot->buf                            = MALLOC(2);
    bwp_dl_searchspace->nrofCandidates                                              = CALLOC(1,sizeof(struct NR_SearchSpace__nrofCandidates)); 
    bwp_dl_searchspace->searchSpaceType                                             = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType));
    bwp_dl_searchspace->searchSpaceType->choice.common                              = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__common));
    bwp_dl_searchspace->searchSpaceType->choice.ue_Specific = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__ue_Specific));

    bwp_dl_timedomainresourceallocation               = CALLOC(1,sizeof(struct NR_PDSCH_TimeDomainResourceAllocation));
    bwp_dl_timedomainresourceallocation->k0           = CALLOC(1,sizeof(long));
    
    ul_frequencyBandList        = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t));
    ul_scs_SpecificCarrierList  = CALLOC(1,sizeof(struct NR_SCS_SpecificCarrier));

    pusch_configcommontimedomainresourceallocation      = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
    pusch_configcommontimedomainresourceallocation->k2  = CALLOC(1,sizeof(long));

    ratematchpattern                              = CALLOC(1,sizeof(struct NR_RateMatchPattern));
    ratematchpattern->patternType.choice.bitmaps  = CALLOC(1,sizeof(struct NR_RateMatchPattern__patternType__bitmaps));
    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf = MALLOC(35);
    ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf   = MALLOC(2);
    ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf  = MALLOC(4);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern                       = CALLOC(1,sizeof(struct NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern));
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.buf        = MALLOC(1);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.buf        = MALLOC(1);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.buf        = MALLOC(1);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.buf        = MALLOC(1);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf       = MALLOC(2);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf       = MALLOC(3);
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf       = MALLOC(5);
    ratematchpattern->subcarrierSpacing           = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
    ratematchpatternid                            = CALLOC(1,sizeof(NR_RateMatchPatternId_t));
  }else{
    bwp_dl_controlresourceset =(*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourceSet;
  }


  //------------------------------------Start Fill ServingCellConfigCommon------------------------------------//
  //physCellId
  *((*servingcellconfigcommon)->physCellId)  = configuration->Nid_cell[CC_id];

    //NR_DownlinkConfigCommon
  *((*servingcellconfigcommon)->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB)  = configuration->absoluteFrequencySSB[CC_id]; 

  *(dl_frequencyBandList)   = configuration->DL_FreqBandIndicatorNR[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list,&dl_frequencyBandList);

  (*servingcellconfigcommon)->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA = configuration->DL_absoluteFrequencyPointA[CC_id];

  dl_scs_SpecificCarrierList->offsetToCarrier    = configuration->DL_offsetToCarrier[CC_id];
  dl_scs_SpecificCarrierList->subcarrierSpacing  = configuration->DL_SCS_SubcarrierSpacing[CC_id];
  dl_scs_SpecificCarrierList->carrierBandwidth   = configuration->DL_carrierBandwidth[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list,&dl_scs_SpecificCarrierList);

  //initialDownlinkBWP
  //initialDownlinkBWP  -----  genericParameters
  (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth = configuration->DL_locationAndBandwidth[CC_id];
  (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing    = configuration->DL_BWP_SubcarrierSpacing[CC_id];

  if(configuration->DL_BWP_prefix_type[CC_id]){
    (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->genericParameters.cyclicPrefix  = NR_BWP__cyclicPrefix_extended;
  }

  //initialDownlinkBWP  -----  pdcch_ConfigCommon
  
  (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->present       = NR_SetupRelease_PDCCH_ConfigCommon_PR_setup;

  //Fill  initialDownlinkBWP  ->  pdcch_ConfigCommon  ->  ControlResourceSet list //

  *((*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero) = configuration->controlResourceSetZero[CC_id];
  *((*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero)        = configuration->searchSpaceZero[CC_id];

  bwp_dl_controlresourceset->controlResourceSetId         = configuration->PDCCH_common_controlResourceSetId[CC_id];
  //BIT STRING (SIZE (45))
  bwp_dl_controlresourceset->frequencyDomainResources.size          = 6;
  bwp_dl_controlresourceset->frequencyDomainResources.bits_unused   = 3;
  bwp_dl_controlresourceset->frequencyDomainResources.buf[0]        = 0x1f;
  bwp_dl_controlresourceset->frequencyDomainResources.buf[1]        = 0xff;
  bwp_dl_controlresourceset->frequencyDomainResources.buf[2]        = 0xff;
  bwp_dl_controlresourceset->frequencyDomainResources.buf[3]        = 0xff; 
  bwp_dl_controlresourceset->frequencyDomainResources.buf[4]        = 0xff; 
  bwp_dl_controlresourceset->frequencyDomainResources.buf[5]        = 0xff; 

  bwp_dl_controlresourceset->duration                     = configuration->PDCCH_common_ControlResourceSet_duration[CC_id];
  bwp_dl_controlresourceset->cce_REG_MappingType.present  = configuration->PDCCH_cce_REG_MappingType[CC_id];

  if(bwp_dl_controlresourceset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved ){
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->reg_BundleSize    = configuration->PDCCH_reg_BundleSize[CC_id];
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->interleaverSize   = configuration->PDCCH_interleaverSize[CC_id];
    *(bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->shiftIndex)        = configuration->PDCCH_shiftIndex[CC_id];
  }else if(bwp_dl_controlresourceset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved){
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.nonInterleaved = 0;
  }

  bwp_dl_controlresourceset->precoderGranularity =  configuration->PDCCH_precoderGranularity[CC_id];

  *(TCI_StateId) = configuration->PDCCH_TCI_StateId[CC_id];
  ASN_SEQUENCE_ADD(&bwp_dl_controlresourceset->tci_StatesPDCCH_ToAddList->list,&TCI_StateId);

  if(configuration->tci_PresentInDCI[CC_id]){
    bwp_dl_controlresourceset->tci_PresentInDCI  = NR_ControlResourceSet__tci_PresentInDCI_enabled;
  }

  *(bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID)   = configuration->PDCCH_DMRS_ScramblingID[CC_id];;

  //Fill  downlinkConfigCommon  ->  initialDownlinkBWP  ->  pdcch_ConfigCommon  ->  SearchSpace list //
  bwp_dl_searchspace->searchSpaceId             = configuration->SearchSpaceId[CC_id];
  *(bwp_dl_searchspace->controlResourceSetId)   = configuration->commonSearchSpaces_controlResourceSetId[CC_id];

  bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_choice[CC_id];
  
  if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl1 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl2 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl4 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl5 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl8 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl10 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl16 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl20 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl40 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl80 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl160 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl320 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl640 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl1280 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl2560 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id];    
  }

  *(bwp_dl_searchspace->duration)                 = configuration->SearchSpace_duration[CC_id];

  bwp_dl_searchspace->monitoringSymbolsWithinSlot->size=2;
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->bits_unused=2;  
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->buf[0]=0x3f;
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->buf[1]=0xff;

  bwp_dl_searchspace->nrofCandidates->aggregationLevel1 = configuration->SearchSpace_nrofCandidates_aggregationLevel1[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel2 = configuration->SearchSpace_nrofCandidates_aggregationLevel2[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel4 = configuration->SearchSpace_nrofCandidates_aggregationLevel4[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel8 = configuration->SearchSpace_nrofCandidates_aggregationLevel8[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel16 = configuration->SearchSpace_nrofCandidates_aggregationLevel16[CC_id];

  bwp_dl_searchspace->searchSpaceType->present = configuration->SearchSpace_searchSpaceType[CC_id];

  if(bwp_dl_searchspace->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_common){
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0               = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__common__dci_Format2_0));
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel1   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel2   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel4   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel8   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel16  = CALLOC(1,sizeof(long));
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3                         = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__common__dci_Format2_3));
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3->monitoringPeriodicity  = CALLOC(1,sizeof(long));
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel1)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel2)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel4)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel8)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel16) = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3->monitoringPeriodicity) = configuration->Common_dci_Format2_3_monitoringPeriodicity[CC_id];
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3->nrofPDCCH_Candidates  = configuration->Common_dci_Format2_3_nrofPDCCH_Candidates[CC_id];

  }else if (bwp_dl_searchspace->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific){
    bwp_dl_searchspace->searchSpaceType->choice.ue_Specific->dci_Formats = configuration->ue_Specific__dci_Formats[CC_id];
  }

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpace->list,&bwp_dl_searchspace);

  *((*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1)                    = configuration->searchSpaceSIB1[CC_id];
  *((*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation)  = configuration->searchSpaceOtherSystemInformation[CC_id];
  *((*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->pagingSearchSpace)                  = configuration->pagingSearchSpace[CC_id];
  *((*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace)                     = configuration->ra_SearchSpace[CC_id];

  //initialDownlinkBWP  -----  pdsch_ConfigCommon
  (*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->present        = NR_SetupRelease_PDSCH_ConfigCommon_PR_setup;
  
  *(bwp_dl_timedomainresourceallocation->k0)                            = configuration->PDSCH_TimeDomainResourceAllocation_k0[CC_id];
  bwp_dl_timedomainresourceallocation->mappingType                      = configuration->PDSCH_TimeDomainResourceAllocation_mappingType[CC_id];
  bwp_dl_timedomainresourceallocation->startSymbolAndLength             = configuration->PDSCH_TimeDomainResourceAllocation_startSymbolAndLength[CC_id];

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,&bwp_dl_timedomainresourceallocation);

  //uplinkConfigCommon 
  //uplinkConfigCommon  frequencyInfoUL //
  *(ul_frequencyBandList)   = configuration->UL_FreqBandIndicatorNR[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list,&ul_frequencyBandList);

  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA) = configuration->UL_absoluteFrequencyPointA[CC_id];
  
  ul_scs_SpecificCarrierList->offsetToCarrier    = configuration->UL_offsetToCarrier[CC_id];
  ul_scs_SpecificCarrierList->subcarrierSpacing  = configuration->UL_SCS_SubcarrierSpacing[CC_id];
  ul_scs_SpecificCarrierList->carrierBandwidth   = configuration->UL_carrierBandwidth[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list,&ul_scs_SpecificCarrierList);

  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->additionalSpectrumEmission) = configuration->UL_additionalSpectrumEmission[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->p_Max)                      = configuration->UL_p_Max[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyShift7p5khz)       = configuration->UL_frequencyShift7p5khz[CC_id];
  
  //uplinkConfigCommon  initialUplinkBWP //
  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> genericParameters//
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth = configuration->UL_locationAndBandwidth[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing    = configuration->UL_BWP_SubcarrierSpacing[CC_id];

  if(configuration->UL_BWP_prefix_type[CC_id]){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.cyclicPrefix = NR_BWP__cyclicPrefix_extended;
  } 

  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> rach_ConfigCommon//
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->present       = NR_SetupRelease_RACH_ConfigCommon_PR_setup;
  
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles) = configuration->rach_totalNumberOfRA_Preambles[CC_id];

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice[CC_id];
  
  if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present       == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneEighth){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneEighth = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneFourth){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneFourth = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneHalf){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneHalf   = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one       = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_two){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.two       = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_four){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.four      = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_eight){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.eight     = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight[CC_id];
  }else if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present == NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_sixteen){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.sixteen   = configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen[CC_id];
  }      

  if(configuration->rach_groupBconfigured[CC_id]){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured->ra_Msg3SizeGroupA            = configuration->rach_ra_Msg3SizeGroupA[CC_id];
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured->messagePowerOffsetGroupB     = configuration->rach_messagePowerOffsetGroupB[CC_id];
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured->numberOfRA_PreamblesGroupA   = configuration->rach_numberOfRA_PreamblesGroupA[CC_id];
  }

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer = configuration->rach_ra_ContentionResolutionTimer[CC_id];

  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB)            = configuration->rsrp_ThresholdSSB[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB_SUL)        = configuration->rsrp_ThresholdSSB_SUL[CC_id];

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present   = configuration->prach_RootSequenceIndex_choice[CC_id];  
  if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present == NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l839){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l839 = configuration->prach_RootSequenceIndex_l839[CC_id];
  }else if ((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present == NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l139){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139 = configuration->prach_RootSequenceIndex_l139[CC_id];
  }

  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)    = configuration->prach_msg1_SubcarrierSpacing[CC_id]; 
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig          = configuration->restrictedSetConfig[CC_id];

  if(configuration->msg3_transformPrecoding[CC_id]){
    *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoding)   = NR_RACH_ConfigCommon__msg3_transformPrecoding_enabled;
  }

  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> rach_ConfigCommon -> rach_ConfigGeneric//  
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex       = configuration->prach_ConfigurationIndex[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM                       = configuration->prach_msg1_FDM[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart            = configuration->prach_msg1_FrequencyStart[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig      = configuration->zeroCorrelationZoneConfig[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower    = configuration->preambleReceivedTargetPower[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleTransMax               = configuration->preambleTransMax[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.powerRampingStep               = configuration->powerRampingStep[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.ra_ResponseWindow              = configuration->ra_ResponseWindow[CC_id];

  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> pusch_ConfigCommon//
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->present        = NR_SetupRelease_PUSCH_ConfigCommon_PR_setup;

  if(configuration->groupHoppingEnabledTransformPrecoding[CC_id]){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = NR_PUSCH_ConfigCommon__groupHoppingEnabledTransformPrecoding_enabled;
  }

  *(pusch_configcommontimedomainresourceallocation->k2)         = configuration->PUSCH_TimeDomainResourceAllocation_k2[CC_id];
  pusch_configcommontimedomainresourceallocation->mappingType   = configuration->PUSCH_TimeDomainResourceAllocation_mappingType[CC_id];
  pusch_configcommontimedomainresourceallocation->startSymbolAndLength  = configuration->PUSCH_TimeDomainResourceAllocation_startSymbolAndLength[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,&pusch_configcommontimedomainresourceallocation); 

  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble)   = configuration->msg3_DeltaPreamble[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant)  = configuration->p0_NominalWithGrant[CC_id];

  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> pucch_ConfigCommon//
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->present      = NR_SetupRelease_PUCCH_ConfigCommon_PR_setup;
  
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_GroupHopping      = configuration->pucch_GroupHopping[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal)           = configuration->p0_nominal[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon) = configuration->pucch_ResourceCommon[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId)            = configuration->hoppingId[CC_id];

  (*servingcellconfigcommon)->uplinkConfigCommon->timeAlignmentTimerCommon = configuration->UL_timeAlignmentTimerCommon[CC_id];

  (*servingcellconfigcommon)->n_TimingAdvanceOffset = CALLOC(1,sizeof(long));
  *((*servingcellconfigcommon)->n_TimingAdvanceOffset)=configuration->ServingCellConfigCommon_n_TimingAdvanceOffset[CC_id];
  //ssb_PositionsInBurst
  (*servingcellconfigcommon)->ssb_PositionsInBurst->present = configuration->ServingCellConfigCommon_ssb_PositionsInBurst_PR[CC_id];

  if((*servingcellconfigcommon)->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap){
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.size = 1;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.bits_unused = 4;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.buf[0] = 0x0f;
  }else if((*servingcellconfigcommon)->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap){
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.size = 1;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.bits_unused = 0;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.buf[0] = 0xff;
  }else if((*servingcellconfigcommon)->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_longBitmap){
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.size = 8;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.bits_unused = 0;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[0] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[1] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[2] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[3] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[4] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[5] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[6] = 0xff;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf[7] = 0xff;
  }

  //ssb_periodicityServingCell
  *(*servingcellconfigcommon)->ssb_periodicityServingCell  = configuration->ServingCellConfigCommon_ssb_periodicityServingCell[CC_id];
  //dmrs_TypeA_Position
  (*servingcellconfigcommon)->dmrs_TypeA_Position          = configuration->ServingCellConfigCommon_dmrs_TypeA_Position[CC_id];

  //lte_CRS_ToMatchAround

  //rateMatchPatternToAddModList
   
  ratematchpattern->rateMatchPatternId    = configuration->rateMatchPatternId[CC_id];
  ratematchpattern->patternType.present   = configuration->RateMatchPattern_patternType[CC_id];

  if(ratematchpattern->patternType.present == NR_RateMatchPattern__patternType_PR_bitmaps){

    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.size = 35;
    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.bits_unused = 5;
    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf[0] = 0x07;

    for (int i =1;i<=34;i++ ){
      ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf[i] =0xff;
    }

    ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.present = configuration->symbolsInResourceBlock[CC_id];
    if(ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.present == NR_RateMatchPattern__patternType__bitmaps__symbolsInResourceBlock_PR_oneSlot){
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.size=2;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.bits_unused=2;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf[0]=0x3f;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf[1]=0xff;      
    }else if(ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.present == NR_RateMatchPattern__patternType__bitmaps__symbolsInResourceBlock_PR_twoSlots){
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.size=4;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.bits_unused=4;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[0]=0x0f;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[1]=0xff;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[2]=0xff;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[3]=0xff;      
    }

    
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present = configuration->periodicityAndPattern[CC_id];
    if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n2){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.bits_unused = 6;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.buf[0] =0x03;
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n4){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.bits_unused = 4;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.buf[0] = 0x0f;
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n5){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.bits_unused = 3;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.buf[0] = 0x1f;   
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n8){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.bits_unused = 0;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.buf[0] = 0xff;    
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n10){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.size = 2;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.bits_unused = 6;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf[0] = 0x03;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf[1] = 0xff;    
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n20){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.size = 3;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.bits_unused = 4;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf[0] = 0x0f;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf[1] = 0xff;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf[2] = 0xff;   
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n40){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.size = 5;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.bits_unused = 0;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf[0] = 0xff;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf[1] = 0xff; 
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf[2] = 0xff;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf[3] = 0xff;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf[4] = 0xff;     
    }

  }else if(ratematchpattern->patternType.present == NR_RateMatchPattern__patternType_PR_controlResourceSet){
    ratematchpattern->patternType.choice.controlResourceSet = configuration->RateMatchPattern_controlResourceSet[CC_id];
  }

  *(ratematchpattern->subcarrierSpacing) = configuration->RateMatchPattern_subcarrierSpacing[CC_id];
  ratematchpattern->mode = configuration->RateMatchPattern_mode[CC_id];

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->rateMatchPatternToAddModList->list,&ratematchpattern);
  
  //rateMatchPatternToReleaseList
  *(ratematchpatternid) = configuration->rateMatchPatternId[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->rateMatchPatternToReleaseList->list,&ratematchpatternid);

  //subcarrierSpacing
  *(*servingcellconfigcommon)->subcarrierSpacing            = configuration->NIA_SubcarrierSpacing[CC_id];

  //tdd_UL_DL_ConfigurationCommon
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing             = configuration->referenceSubcarrierSpacing[CC_id];
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity = configuration->dl_UL_TransmissionPeriodicity[CC_id];
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots             = configuration->nrofDownlinkSlots[CC_id];
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols           = configuration->nrofDownlinkSymbols[CC_id];
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots               = configuration->nrofUplinkSlots[CC_id];
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols             = configuration->nrofUplinkSymbols[CC_id];

  //ss_PBCH_BlockPower
  (*servingcellconfigcommon)->ss_PBCH_BlockPower           = configuration->ServingCellConfigCommon_ss_PBCH_BlockPower[CC_id];

}

void  do_RLC_BEARER(uint8_t Mod_id,
                    int CC_id,
                    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_BearerToAddModList,
                    rlc_bearer_config_t  *rlc_config){

  struct NR_RLC_BearerConfig *rlc_bearer;
  rlc_bearer = CALLOC(1,sizeof(struct NR_RLC_BearerConfig));

  rlc_bearer->logicalChannelIdentity = rlc_config->LogicalChannelIdentity[CC_id];

  rlc_bearer->servedRadioBearer = CALLOC(1,sizeof(struct NR_RLC_BearerConfig__servedRadioBearer));
  rlc_bearer->servedRadioBearer->present = rlc_config->servedRadioBearer_present[CC_id];

  if(rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity){
    rlc_bearer->servedRadioBearer->choice.srb_Identity = rlc_config->srb_Identity[CC_id];
  }else if(rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity){
    rlc_bearer->servedRadioBearer->choice.drb_Identity = rlc_config->drb_Identity[CC_id];
  }
  
  rlc_bearer->reestablishRLC = CALLOC(1,sizeof(long));
  *(rlc_bearer->reestablishRLC) = rlc_config->reestablishRLC[CC_id];

  rlc_bearer->rlc_Config = CALLOC(1,sizeof(struct NR_RLC_Config));
  rlc_bearer->rlc_Config->present = rlc_config->rlc_Config_present[CC_id];

  if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_am){

    rlc_bearer->rlc_Config->choice.am = CALLOC(1,sizeof(struct NR_RLC_Config__am));
    
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength     = CALLOC(1,sizeof(NR_SN_FieldLengthAM_t));
    *(rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength)  = rlc_config->ul_AM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit   = rlc_config->t_PollRetransmit[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.pollPDU            = rlc_config->pollPDU[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.pollByte           = rlc_config->pollByte[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold   = rlc_config->maxRetxThreshold[CC_id];

    rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength     = CALLOC(1,sizeof(NR_SN_FieldLengthAM_t));
    *(rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength)  = rlc_config->dl_AM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.t_Reassembly       = rlc_config->dl_AM_t_Reassembly[CC_id];
    rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit   = rlc_config->t_StatusProhibit[CC_id];

  }else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Bi_Directional){
    rlc_bearer->rlc_Config->choice.um_Bi_Directional = CALLOC(1,sizeof(struct NR_RLC_Config__um_Bi_Directional));

    rlc_bearer->rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength) = rlc_config->ul_UM_sn_FieldLength[CC_id];

    rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength) = rlc_config->dl_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.t_Reassembly   = rlc_config->dl_UM_t_Reassembly[CC_id];

  }else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Uni_Directional_UL){
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL = CALLOC(1,sizeof(struct NR_RLC_Config__um_Uni_Directional_UL));

    rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL->ul_UM_RLC.sn_FieldLength    = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL->ul_UM_RLC.sn_FieldLength) = rlc_config->ul_UM_sn_FieldLength[CC_id];

  }else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Uni_Directional_DL){
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL = CALLOC(1,sizeof(struct NR_RLC_Config__um_Uni_Directional_DL));

    rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL->dl_UM_RLC.sn_FieldLength    = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL->dl_UM_RLC.sn_FieldLength) = rlc_config->dl_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL->dl_UM_RLC.t_Reassembly      = rlc_config->dl_UM_t_Reassembly[CC_id];
  }

  rlc_bearer->mac_LogicalChannelConfig = CALLOC(1,sizeof(struct NR_LogicalChannelConfig));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters = CALLOC(1,sizeof(struct NR_LogicalChannelConfig__ul_SpecificParameters));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->priority            = rlc_config->priority[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->prioritisedBitRate  = rlc_config->prioritisedBitRate[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->bucketSizeDuration  = rlc_config->bucketSizeDuration[CC_id];

  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedServingCells = CALLOC(1,sizeof(struct NR_LogicalChannelConfig__ul_SpecificParameters__allowedServingCells));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedSCS_List     = CALLOC(1,sizeof(struct NR_LogicalChannelConfig__ul_SpecificParameters__allowedSCS_List));

  NR_ServCellIndex_t *servingcellindex;
  servingcellindex = CALLOC(1,sizeof(NR_ServCellIndex_t));
  *servingcellindex = rlc_config->allowedServingCells[CC_id];
  ASN_SEQUENCE_ADD(&(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedServingCells->list),&servingcellindex);

  NR_SubcarrierSpacing_t *subcarrierspacing;
  subcarrierspacing = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  *subcarrierspacing = rlc_config->subcarrierspacing[CC_id];
  ASN_SEQUENCE_ADD(&(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedSCS_List->list),&subcarrierspacing);

  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->maxPUSCH_Duration           = CALLOC(1,sizeof(long));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->configuredGrantType1Allowed = CALLOC(1,sizeof(long));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup         = CALLOC(1,sizeof(long));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID         = CALLOC(1,sizeof(NR_SchedulingRequestId_t));

  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->maxPUSCH_Duration)           = rlc_config->maxPUSCH_Duration[CC_id];
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->configuredGrantType1Allowed) = rlc_config->configuredGrantType1Allowed[CC_id];
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup)         = rlc_config->logicalChannelGroup[CC_id];
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID)         = rlc_config->schedulingRequestID[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_Mask               = rlc_config->logicalChannelSR_Mask[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_DelayTimerApplied  = rlc_config->logicalChannelSR_DelayTimerApplied[CC_id];

  ASN_SEQUENCE_ADD(&(rlc_BearerToAddModList->list),&rlc_bearer);
}

void  do_MAC_CELLGROUP(uint8_t Mod_id,
                       int CC_id,
                       struct NR_MAC_CellGroupConfig *mac_CellGroupConfig,
                       mac_cellgroup_t  *mac_cellgroup_config){

  mac_CellGroupConfig->drx_Config               = CALLOC(1,sizeof(struct NR_SetupRelease_DRX_Config));
  mac_CellGroupConfig->schedulingRequestConfig  = CALLOC(1,sizeof(struct NR_SchedulingRequestConfig));
  mac_CellGroupConfig->bsr_Config               = CALLOC(1,sizeof(struct NR_BSR_Config));
  mac_CellGroupConfig->tag_Config               = CALLOC(1,sizeof(struct NR_TAG_Config));
  mac_CellGroupConfig->phr_Config               = CALLOC(1,sizeof(struct NR_SetupRelease_PHR_Config));

  mac_CellGroupConfig->drx_Config->present      = mac_cellgroup_config->DRX_Config_PR[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup = CALLOC(1,sizeof(struct NR_DRX_Config));
  mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present = mac_cellgroup_config->drx_onDurationTimer_PR[CC_id];
  
  if(mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present == NR_DRX_Config__drx_onDurationTimer_PR_subMilliSeconds){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.choice.subMilliSeconds = mac_cellgroup_config->subMilliSeconds[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present == NR_DRX_Config__drx_onDurationTimer_PR_milliSeconds){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.choice.milliSeconds    = mac_cellgroup_config->milliSeconds[CC_id];
  }

  mac_CellGroupConfig->drx_Config->choice.setup->drx_InactivityTimer        = mac_cellgroup_config->drx_InactivityTimer[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_HARQ_RTT_TimerDL       = mac_cellgroup_config->drx_HARQ_RTT_TimerDL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_HARQ_RTT_TimerUL       = mac_cellgroup_config->drx_HARQ_RTT_TimerUL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_RetransmissionTimerDL  = mac_cellgroup_config->drx_RetransmissionTimerDL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_RetransmissionTimerUL  = mac_cellgroup_config->drx_RetransmissionTimerUL[CC_id];

  mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present = mac_cellgroup_config->drx_LongCycleStartOffset_PR[CC_id];

  if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms10){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms10 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms20){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms20 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms32){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms32 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms40){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms40 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms60){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms60 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms64){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms64 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms70){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms70 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms80){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms80 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms128){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms128 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms160){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms160 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms256){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms256 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms320){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms320 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms512){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms512 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms640){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms640 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms1024){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms1024 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms1280){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms1280 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms2048){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms2048 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms2560){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms2560 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms5120){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms5120 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms10240){
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms10240 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }

  mac_CellGroupConfig->drx_Config->choice.setup->shortDRX = CALLOC(1,sizeof(struct NR_DRX_Config__shortDRX));
  mac_CellGroupConfig->drx_Config->choice.setup->shortDRX->drx_ShortCycle       = mac_cellgroup_config->drx_ShortCycle[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->shortDRX->drx_ShortCycleTimer  = mac_cellgroup_config->drx_ShortCycleTimer[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_SlotOffset                 = mac_cellgroup_config->drx_SlotOffset[CC_id];

  mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList = CALLOC(1,sizeof(struct NR_SchedulingRequestConfig__schedulingRequestToAddModList));

  struct NR_SchedulingRequestToAddMod *schedulingrequestlist;
  schedulingrequestlist = CALLOC(1,sizeof(struct NR_SchedulingRequestToAddMod));
  schedulingrequestlist->schedulingRequestId  = mac_cellgroup_config->schedulingRequestId[CC_id];
  schedulingrequestlist->sr_ProhibitTimer = CALLOC(1,sizeof(long));
  *(schedulingrequestlist->sr_ProhibitTimer) = mac_cellgroup_config->sr_ProhibitTimer[CC_id];
  schedulingrequestlist->sr_TransMax      = mac_cellgroup_config->sr_TransMax[CC_id];

  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList->list),&schedulingrequestlist);


  mac_CellGroupConfig->bsr_Config->periodicBSR_Timer              = mac_cellgroup_config->periodicBSR_Timer[CC_id];
  mac_CellGroupConfig->bsr_Config->retxBSR_Timer                  = mac_cellgroup_config->retxBSR_Timer[CC_id];
  mac_CellGroupConfig->bsr_Config->logicalChannelSR_DelayTimer    = CALLOC(1,sizeof(long));
  *(mac_CellGroupConfig->bsr_Config->logicalChannelSR_DelayTimer)    = mac_cellgroup_config->logicalChannelSR_DelayTimer[CC_id];

  mac_CellGroupConfig->tag_Config->tag_ToAddModList = CALLOC(1,sizeof(struct NR_TAG_Config__tag_ToAddModList));
  struct NR_TAG *tag;
  tag = CALLOC(1,sizeof(struct NR_TAG));
  tag->tag_Id             = mac_cellgroup_config->tag_Id[CC_id];
  tag->timeAlignmentTimer = mac_cellgroup_config->timeAlignmentTimer[CC_id];
  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->tag_Config->tag_ToAddModList->list),&tag);

  mac_CellGroupConfig->phr_Config->present = mac_cellgroup_config->PHR_Config_PR[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup   = CALLOC(1,sizeof(struct NR_PHR_Config));
  mac_CellGroupConfig->phr_Config->choice.setup->phr_PeriodicTimer         = mac_cellgroup_config->phr_PeriodicTimer[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_ProhibitTimer         = mac_cellgroup_config->phr_ProhibitTimer[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Tx_PowerFactorChange  = mac_cellgroup_config->phr_Tx_PowerFactorChange[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->multiplePHR               = mac_cellgroup_config->multiplePHR[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Type2SpCell           = mac_cellgroup_config->phr_Type2SpCell[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Type2OtherCell        = mac_cellgroup_config->phr_Type2OtherCell[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_ModeOtherCG           = mac_cellgroup_config->phr_ModeOtherCG[CC_id];

  mac_CellGroupConfig->skipUplinkTxDynamic      = mac_cellgroup_config->skipUplinkTxDynamic[CC_id];

}

void  do_PHYSICALCELLGROUP(uint8_t Mod_id,
                           int CC_id,
                           struct NR_PhysicalCellGroupConfig *physicalCellGroupConfig,
                           physicalcellgroup_t *physicalcellgroup_config){

  physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH = CALLOC(1,sizeof(long));
  physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH = CALLOC(1,sizeof(long));
  physicalCellGroupConfig->p_NR                          = CALLOC(1,sizeof(NR_P_Max_t));
  physicalCellGroupConfig->tpc_SRS_RNTI                  = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->tpc_PUCCH_RNTI                = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->tpc_PUSCH_RNTI                = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->sp_CSI_RNTI                   = CALLOC(1,sizeof(NR_RNTI_Value_t));

  *(physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH) = physicalcellgroup_config->harq_ACK_SpatialBundlingPUCCH[CC_id];
  *(physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH) = physicalcellgroup_config->harq_ACK_SpatialBundlingPUSCH[CC_id];
  *(physicalCellGroupConfig->p_NR)                          = physicalcellgroup_config->p_NR[CC_id];
  physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook          = physicalcellgroup_config->pdsch_HARQ_ACK_Codebook[CC_id];
  *(physicalCellGroupConfig->tpc_SRS_RNTI)                  = physicalcellgroup_config->tpc_SRS_RNTI[CC_id];
  *(physicalCellGroupConfig->tpc_PUCCH_RNTI)                = physicalcellgroup_config->tpc_PUCCH_RNTI[CC_id];
  *(physicalCellGroupConfig->tpc_PUSCH_RNTI)                = physicalcellgroup_config->tpc_PUSCH_RNTI[CC_id];
  *(physicalCellGroupConfig->sp_CSI_RNTI)                   = physicalcellgroup_config->sp_CSI_RNTI[CC_id];

  physicalCellGroupConfig->cs_RNTI                       = CALLOC(1,sizeof(struct NR_SetupRelease_RNTI_Value));
  physicalCellGroupConfig->cs_RNTI->present              = physicalcellgroup_config->RNTI_Value_PR[CC_id];

  if(physicalCellGroupConfig->cs_RNTI->present == NR_SetupRelease_RNTI_Value_PR_setup){
    physicalCellGroupConfig->cs_RNTI->choice.setup = physicalcellgroup_config->RNTI_Value[CC_id];
  }

}

void  do_SpCellConfig(uint8_t Mod_id,
                      int CC_id,
                      struct NR_SpCellConfig  *spconfig){

  //spconfig->servCellIndex = CALLOC(1,sizeof(NR_ServCellIndex_t));
  //*(spconfig->servCellIndex)=

  gNB_RrcConfigurationReq  *common_configuration;
  common_configuration = CALLOC(1,sizeof(gNB_RrcConfigurationReq));
  //Fill servingcellconfigcommon config value
  rrc_config_servingcellconfigcommon(Mod_id,
                                     CC_id
                                     #if defined(ENABLE_ITTI)
                                     ,common_configuration
                                     #endif
                                     );
  //Fill common config to structure
  do_SERVINGCELLCONFIGCOMMON(Mod_id,
                             CC_id,
                             #if defined(ENABLE_ITTI)
                             common_configuration,
                             #endif
                             0
                             );

  spconfig->reconfigurationWithSync = CALLOC(1,sizeof(struct NR_ReconfigurationWithSync));
/*
  memcpy( spconfig->reconfigurationWithSync, 
          RC.nrrrc[Mod_id]->carrier[0].servingcellconfigcommon,
          sizeof(struct NR_ServingCellConfigCommon));
*/
}