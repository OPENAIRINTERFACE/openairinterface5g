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
* \author Raymond Knopp and Navid Nikaein
* \date 2011
* \version 1.0
* \company Eurecom
* \email: raymond.knopp@eurecom.fr and  navid.nikaein@eurecom.fr
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h> /* for atoi(3) */
#include <unistd.h> /* for getopt(3) */
#include <string.h> /* for strerror(3) */
#include <sysexits.h> /* for EX_* exit codes */
#include <errno.h>  /* for errno */
#include "UTIL/LOG/log.h"
#include <asn_application.h>
#include <asn_internal.h> /* for _ASN_DEFAULT_STACK_MAX */
#include <per_encoder.h>

#include "asn1_msg.h"
#include "RRC/NR/nr_rrc_defs.h"
#include "RRC/NR/nr_rrc_extern.h"

#if defined(NR_Rel15)
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
  uint8_t sfn = (uint8_t)((frame>>2)&0x3f);
  mib->message.choice.mib->systemFrameNumber.buf = &sfn;
  mib->message.choice.mib->systemFrameNumber.size = 1;
  mib->message.choice.mib->systemFrameNumber.bits_unused=0;

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
                                   int CC_id
                                   #if defined(ENABLE_ITTI)
                                   ,gNB_RrcConfigurationReq *configuration
                                   #endif
                                  )
{ 
  NR_ServingCellConfigCommon_t  **servingcellconfigcommon  =  &RC.nrrrc[Mod_id]->carrier[CC_id].servingcellconfigcommon;

  if (!servingcellconfigcommon) {
    LOG_E(NR_RRC,"[gNB %d] servingcellconfigcommon is null, exiting\n", Mod_id);
    exit(-1);
  }

  (*servingcellconfigcommon)->physCellId                      = CALLOC(1,sizeof(NR_PhysCellId_t));
  (*servingcellconfigcommon)->frequencyInfoDL                 = CALLOC(1,sizeof(struct NR_FrequencyInfoDL));
  (*servingcellconfigcommon)->initialDownlinkBWP              = CALLOC(1,sizeof(struct NR_BWP_DownlinkCommon));
  (*servingcellconfigcommon)->uplinkConfigCommon              = CALLOC(1,sizeof(struct NR_UplinkConfigCommon));
  //(*servingcellconfigcommon)->supplementaryUplinkConfig              = CALLOC(1,sizeof(struct NR_UplinkConfigCommon));  
  (*servingcellconfigcommon)->ssb_PositionsInBurst            = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__ssb_PositionsInBurst));
  (*servingcellconfigcommon)->ssb_periodicityServingCell      = CALLOC(1,sizeof(long));
  //(*servingcellconfigcommon)->lte_CRS_ToMatchAround           = CALLOC(1,sizeof(struct NR_SetupRelease_RateMatchPatternLTE_CRS));
  (*servingcellconfigcommon)->rateMatchPatternToAddModList    = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__rateMatchPatternToAddModList));
  (*servingcellconfigcommon)->rateMatchPatternToReleaseList   = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__rateMatchPatternToReleaseList));
  (*servingcellconfigcommon)->subcarrierSpacing               = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon   = CALLOC(1,sizeof(struct NR_TDD_UL_DL_ConfigCommon));
  //(*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon2  = CALLOC(1,sizeof(struct NR_TDD_UL_DL_ConfigCommon);

  //------------------------------------Start Fill ServingCellConfigCommon------------------------------------//
  //physCellId
  *((*servingcellconfigcommon)->physCellId)  = configuration->Nid_cell[CC_id];

    //frequencyInfoDL
  (*servingcellconfigcommon)->frequencyInfoDL->absoluteFrequencySSB     = configuration->absoluteFrequencySSB[CC_id];
  (*servingcellconfigcommon)->frequencyInfoDL->ssb_SubcarrierOffset     = CALLOC(1,sizeof(long));
  *((*servingcellconfigcommon)->frequencyInfoDL->ssb_SubcarrierOffset)  = configuration->ssb_SubcarrierOffset[CC_id];  
  
  NR_FreqBandIndicatorNR_t  *dl_frequencyBandList;
  dl_frequencyBandList      = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t));
  *(dl_frequencyBandList)   = configuration->DL_FreqBandIndicatorNR[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->frequencyInfoDL->frequencyBandList.list,&dl_frequencyBandList);
  
  (*servingcellconfigcommon)->frequencyInfoDL->absoluteFrequencyPointA = configuration->DL_absoluteFrequencyPointA[CC_id];
  
  struct NR_SCS_SpecificCarrier                    *dl_scs_SpecificCarrierList;
  dl_scs_SpecificCarrierList                     = CALLOC(1,sizeof(struct NR_SCS_SpecificCarrier));
  dl_scs_SpecificCarrierList->offsetToCarrier    = configuration->DL_offsetToCarrier[CC_id];
  dl_scs_SpecificCarrierList->subcarrierSpacing  = configuration->DL_SCS_SubcarrierSpacing[CC_id];
  dl_scs_SpecificCarrierList->k0                 = configuration->DL_SCS_SpecificCarrier_k0[CC_id];
  dl_scs_SpecificCarrierList->carrierBandwidth   = configuration->DL_carrierBandwidth[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->frequencyInfoDL->scs_SpecificCarrierList.list,&dl_scs_SpecificCarrierList);

  //initialDownlinkBWP
  //initialDownlinkBWP  -----  genericParameters
  (*servingcellconfigcommon)->initialDownlinkBWP->genericParameters.locationAndBandwidth = configuration->DL_locationAndBandwidth[CC_id];
  (*servingcellconfigcommon)->initialDownlinkBWP->genericParameters.subcarrierSpacing    = configuration->DL_BWP_SubcarrierSpacing[CC_id];

  if(configuration->DL_BWP_prefix_type[CC_id]){
    (*servingcellconfigcommon)->initialDownlinkBWP->genericParameters.cyclicPrefix  = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->initialDownlinkBWP->genericParameters.cyclicPrefix  = NR_BWP__cyclicPrefix_extended;
  }

  //initialDownlinkBWP  -----  pdcch_ConfigCommon
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon                = CALLOC(1,sizeof(struct NR_SetupRelease_PDCCH_ConfigCommon));
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->present       = NR_SetupRelease_PDCCH_ConfigCommon_PR_setup;
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup  = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon));
  
  //Fill  initialDownlinkBWP  ->  pdcch_ConfigCommon  ->  ControlResourceSet list //
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourcesSets = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon__commonControlResourcesSets));
  
  struct NR_ControlResourceSet  *bwp_dl_controlresourceset;
  bwp_dl_controlresourceset  = CALLOC(1,sizeof(struct NR_ControlResourceSet));
  bwp_dl_controlresourceset->controlResourceSetId      = configuration->PDCCH_common_controlResourceSetId[CC_id];
  //BIT STRING (SIZE (45))
  bwp_dl_controlresourceset->frequencyDomainResources.buf     = MALLOC(6);
  bwp_dl_controlresourceset->frequencyDomainResources.size    = 6;
  bwp_dl_controlresourceset->frequencyDomainResources.bits_unused = 3;
  bwp_dl_controlresourceset->frequencyDomainResources.buf[0]  = 0x1f;
  bwp_dl_controlresourceset->frequencyDomainResources.buf[1]  = 0xff;   
  bwp_dl_controlresourceset->frequencyDomainResources.buf[2]  = 0xff; 
  bwp_dl_controlresourceset->frequencyDomainResources.buf[3]  = 0xff; 
  bwp_dl_controlresourceset->frequencyDomainResources.buf[4]  = 0xff; 
  bwp_dl_controlresourceset->frequencyDomainResources.buf[5]  = 0xff; 
  bwp_dl_controlresourceset->frequencyDomainResources.buf[6]  = 0xff; 

  bwp_dl_controlresourceset->duration = configuration->PDCCH_common_ControlResourceSet_duration[CC_id];

  bwp_dl_controlresourceset->cce_REG_MappingType.present = configuration->PDCCH_cce_REG_MappingType[CC_id];

  if(bwp_dl_controlresourceset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved ){
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved = CALLOC(1,sizeof(struct NR_ControlResourceSet__cce_REG_MappingType__interleaved));
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->reg_BundleSize    = configuration->PDCCH_reg_BundleSize[CC_id];
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->interleaverSize   = configuration->PDCCH_interleaverSize[CC_id];
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.interleaved->shiftIndex        = configuration->PDCCH_shiftIndex[CC_id];
  }else if(bwp_dl_controlresourceset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved){
    bwp_dl_controlresourceset->cce_REG_MappingType.choice.nonInterleaved = 0;
  }

  bwp_dl_controlresourceset->precoderGranularity =  configuration->PDCCH_precoderGranularity[CC_id];

  bwp_dl_controlresourceset->tci_StatesPDCCH = CALLOC(1,sizeof(struct NR_ControlResourceSet__tci_StatesPDCCH));
  NR_TCI_StateId_t  *TCI_StateId;
  TCI_StateId = CALLOC(1,sizeof(NR_TCI_StateId_t));
  memset(&TCI_StateId,0,sizeof(NR_TCI_StateId_t));
  *(TCI_StateId) = configuration->PDCCH_TCI_StateId[CC_id];
  ASN_SEQUENCE_ADD(&bwp_dl_controlresourceset->tci_StatesPDCCH->list,&TCI_StateId);

  if(configuration->tci_PresentInDCI[CC_id]){
    bwp_dl_controlresourceset->tci_PresentInDCI  = CALLOC(1,sizeof(long));
    bwp_dl_controlresourceset->tci_PresentInDCI  = NR_ControlResourceSet__tci_PresentInDCI_enabled;
  }

  bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID = CALLOC(1,sizeof(BIT_STRING_t));
  bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID->buf  = MALLOC(2);
  bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID->size = 2;
  bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID->bits_unused = 0;
  bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID->buf[0] = 0xff;
  bwp_dl_controlresourceset->pdcch_DMRS_ScramblingID->buf[1] = 0xff;

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourcesSets->list,&bwp_dl_controlresourceset);

  //Fill  initialDownlinkBWP  ->  pdcch_ConfigCommon  ->  SearchSpace list //
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaces = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon__commonSearchSpaces));
  
  NR_SearchSpace_t  *bwp_dl_searchspace;
  bwp_dl_searchspace = CALLOC(1,sizeof(NR_SearchSpace_t));
  bwp_dl_searchspace->searchSpaceId         = configuration->SearchSpaceId[CC_id];
  bwp_dl_searchspace->controlResourceSetId  = CALLOC(1,sizeof(NR_ControlResourceSetId_t));
  *(bwp_dl_searchspace->controlResourceSetId)  = configuration->commonSearchSpaces_controlResourceSetId[CC_id];

  bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset = CALLOC(1,sizeof(struct NR_SearchSpace__monitoringSlotPeriodicityAndOffset));
  bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_choice[CC_id];
  
  if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl1 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl1[CC_id];
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl2 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl2[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl4 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl4[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl5 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl5[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl8 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl8[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl10 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl10[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl16 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl16[CC_id];    
  }else if(bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->present == NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20){
    bwp_dl_searchspace->monitoringSlotPeriodicityAndOffset->choice.sl20 = configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_sl20[CC_id];    
  }

  bwp_dl_searchspace->monitoringSymbolsWithinSlot = CALLOC(1,sizeof(BIT_STRING_t));
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->buf=MALLOC(2);
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->size=2;
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->bits_unused=2;  
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->buf[0]=0x3f;
  bwp_dl_searchspace->monitoringSymbolsWithinSlot->buf[1]=0xff;

  bwp_dl_searchspace->nrofCandidates = CALLOC(1,sizeof(struct NR_SearchSpace__nrofCandidates)); 
  bwp_dl_searchspace->nrofCandidates->aggregationLevel1 = configuration->SearchSpace_nrofCandidates_aggregationLevel1[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel2 = configuration->SearchSpace_nrofCandidates_aggregationLevel2[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel4 = configuration->SearchSpace_nrofCandidates_aggregationLevel4[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel8 = configuration->SearchSpace_nrofCandidates_aggregationLevel8[CC_id];
  bwp_dl_searchspace->nrofCandidates->aggregationLevel16 = configuration->SearchSpace_nrofCandidates_aggregationLevel16[CC_id];

  bwp_dl_searchspace->searchSpaceType = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType));
  bwp_dl_searchspace->searchSpaceType->present = configuration->SearchSpace_searchSpaceType[CC_id];

  if(bwp_dl_searchspace->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_common){
    bwp_dl_searchspace->searchSpaceType->choice.common = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__common));
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0 = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__common__dci_Format2_0));
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel1   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel2   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel4   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel8   = CALLOC(1,sizeof(long)); 
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel16  = CALLOC(1,sizeof(long)); 

    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel1)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel2)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel4)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel8)  = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8[CC_id];
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_0->nrofCandidates_SFI.aggregationLevel16) = configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16[CC_id];
    
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3 = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__common__dci_Format2_3));
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3->monitoringPeriodicity = CALLOC(1,sizeof(long));
    *(bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3->monitoringPeriodicity) = configuration->Common_dci_Format2_3_monitoringPeriodicity[CC_id];
    bwp_dl_searchspace->searchSpaceType->choice.common->dci_Format2_3->nrofPDCCH_Candidates  = configuration->Common_dci_Format2_3_nrofPDCCH_Candidates[CC_id];

  }else if (bwp_dl_searchspace->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific){
    bwp_dl_searchspace->searchSpaceType->choice.ue_Specific = CALLOC(1,sizeof(struct NR_SearchSpace__searchSpaceType__ue_Specific));
    bwp_dl_searchspace->searchSpaceType->choice.ue_Specific->dci_Formats = configuration->ue_Specific__dci_Formats[CC_id];
  }

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaces->list,&bwp_dl_searchspace);

  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1                    = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation  = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->pagingSearchSpace                  = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace                     = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  (*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_ControlResourceSet              = CALLOC(1,sizeof(NR_ControlResourceSetId_t));

  *((*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1)                    = configuration->searchSpaceSIB1[CC_id];
  *((*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation)  = configuration->searchSpaceOtherSystemInformation[CC_id];
  *((*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->pagingSearchSpace)                  = configuration->pagingSearchSpace[CC_id];
  *((*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace)                     = configuration->ra_SearchSpace[CC_id];
  *((*servingcellconfigcommon)->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_ControlResourceSet)              = configuration->rach_ra_ControlResourceSet[CC_id];

  //initialDownlinkBWP  -----  pdsch_ConfigCommon
 
  (*servingcellconfigcommon)->initialDownlinkBWP->pdsch_ConfigCommon                 = CALLOC(1,sizeof(struct NR_SetupRelease_PDSCH_ConfigCommon));
  (*servingcellconfigcommon)->initialDownlinkBWP->pdsch_ConfigCommon->present        = NR_SetupRelease_PDSCH_ConfigCommon_PR_setup;
  (*servingcellconfigcommon)->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_PDSCH_ConfigCommon));

  (*servingcellconfigcommon)->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_AllocationList = CALLOC(1,sizeof(struct NR_PDSCH_ConfigCommon__pdsch_AllocationList));
  
  struct NR_PDSCH_TimeDomainResourceAllocation      *bwp_dl_timedomainresourceallocation;
  bwp_dl_timedomainresourceallocation               = CALLOC(1,sizeof(struct NR_PDSCH_TimeDomainResourceAllocation));
  bwp_dl_timedomainresourceallocation->k0           = CALLOC(1,sizeof(long));
  *(bwp_dl_timedomainresourceallocation->k0)                            = configuration->PDSCH_TimeDomainResourceAllocation_k0[CC_id];
  bwp_dl_timedomainresourceallocation->mappingType                      = configuration->PDSCH_TimeDomainResourceAllocation_mappingType[CC_id];
  bwp_dl_timedomainresourceallocation->startSymbolAndLength.buf         =MALLOC(1);
  bwp_dl_timedomainresourceallocation->startSymbolAndLength.size        =1;
  bwp_dl_timedomainresourceallocation->startSymbolAndLength.bits_unused =1;
  bwp_dl_timedomainresourceallocation->startSymbolAndLength.buf[0]      =0x7f;

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_AllocationList->list,&bwp_dl_timedomainresourceallocation);

  //uplinkConfigCommon 
  //uplinkConfigCommon  frequencyInfoUL //
  (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL                     = CALLOC(1,sizeof(struct NR_FrequencyInfoUL));
  (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyBandList  = CALLOC(1,sizeof(struct NR_MultiFrequencyBandListNR));
  
  NR_FreqBandIndicatorNR_t  *ul_frequencyBandList;
  ul_frequencyBandList      = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t)); 
  *(ul_frequencyBandList)   = configuration->UL_FreqBandIndicatorNR[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list,&ul_frequencyBandList);

  (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA = CALLOC(1,sizeof(NR_ARFCN_ValueNR_t));
  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA) = configuration->UL_absoluteFrequencyPointA[CC_id];
  
  struct NR_SCS_SpecificCarrier   *ul_scs_SpecificCarrierList;
  ul_scs_SpecificCarrierList      = CALLOC(1,sizeof(struct NR_SCS_SpecificCarrier));
  ul_scs_SpecificCarrierList->offsetToCarrier    = configuration->UL_offsetToCarrier[CC_id];
  ul_scs_SpecificCarrierList->subcarrierSpacing  = configuration->UL_SCS_SubcarrierSpacing[CC_id];
  ul_scs_SpecificCarrierList->k0                 = configuration->UL_SCS_SpecificCarrier_k0[CC_id];
  ul_scs_SpecificCarrierList->carrierBandwidth   = configuration->UL_carrierBandwidth[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarriers.list,&ul_scs_SpecificCarrierList);

  (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->additionalSpectrumEmission = CALLOC(1,sizeof(NR_AdditionalSpectrumEmission_t));
  (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->p_Max                      = CALLOC(1,sizeof(NR_P_Max_t));
  (*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyShift7p5khz       = CALLOC(1,sizeof(long));

  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->additionalSpectrumEmission) = configuration->UL_additionalSpectrumEmission[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->p_Max)                      = configuration->UL_p_Max[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->frequencyInfoUL->frequencyShift7p5khz)       = configuration->UL_frequencyShift7p5khz[CC_id];
  
  //uplinkConfigCommon  initialUplinkBWP //
  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> genericParameters//
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth = configuration->UL_locationAndBandwidth[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing    = configuration->UL_BWP_SubcarrierSpacing[CC_id];

  if(configuration->UL_BWP_prefix_type[CC_id]){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.cyclicPrefix = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->genericParameters.cyclicPrefix = NR_BWP__cyclicPrefix_extended;
  } 

  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> rach_ConfigCommon//
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon                = CALLOC(1,sizeof(NR_SetupRelease_RACH_ConfigCommon_t));
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->present       = NR_SetupRelease_RACH_ConfigCommon_PR_setup;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup  = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon));

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles = CALLOC(1,sizeof(long));
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles) = configuration->rach_totalNumberOfRA_Preambles[CC_id];
  
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB));
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
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon__groupBconfigured));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured->ra_Msg3SizeGroupA            = configuration->rach_ra_Msg3SizeGroupA[CC_id];
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured->messagePowerOffsetGroupB     = configuration->rach_messagePowerOffsetGroupB[CC_id];
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured->numberOfRA_PreamblesGroupA   = configuration->rach_numberOfRA_PreamblesGroupA[CC_id];
  }

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer = configuration->rach_ra_ContentionResolutionTimer[CC_id];
  
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB            = CALLOC(1,sizeof(NR_RSRP_Range_t));
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB_SUL        = CALLOC(1,sizeof(NR_RSRP_Range_t));
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB)            = configuration->rsrp_ThresholdSSB[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB_SUL)        = configuration->rsrp_ThresholdSSB_SUL[CC_id];

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present   = configuration->prach_RootSequenceIndex_choice[CC_id];  
  if((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present == NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l839){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l839 = configuration->prach_RootSequenceIndex_l839[CC_id];
  }else if ((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present == NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l139){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139 = configuration->prach_RootSequenceIndex_l139[CC_id];
  }

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing       = configuration->prach_msg1_SubcarrierSpacing[CC_id]; 
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig          = configuration->restrictedSetConfig[CC_id];

  if(configuration->msg3_transformPrecoding[CC_id]){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoding      = CALLOC(1,sizeof(long));    
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoding      = NR_RACH_ConfigCommon__msg3_transformPrecoding_enabled;
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
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon                 = CALLOC(1,sizeof(NR_SetupRelease_PUSCH_ConfigCommon_t));  
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->present        = NR_SetupRelease_PUSCH_ConfigCommon_PR_setup;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_PUSCH_ConfigCommon));


  if(configuration->groupHoppingEnabledTransformPrecoding[CC_id]){
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = CALLOC(1,sizeof(long));
    (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = NR_PUSCH_ConfigCommon__groupHoppingEnabledTransformPrecoding_enabled;
  }

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_AllocationList = CALLOC(1,sizeof(struct NR_PUSCH_ConfigCommon__pusch_AllocationList));
  
  struct NR_PUSCH_TimeDomainResourceAllocation    *pusch_configcommontimedomainresourceallocation;
  pusch_configcommontimedomainresourceallocation  = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));

  pusch_configcommontimedomainresourceallocation->k2      = CALLOC(1,sizeof(long));
  *(pusch_configcommontimedomainresourceallocation->k2)   = configuration->PUSCH_TimeDomainResourceAllocation_k2[CC_id];
  
  pusch_configcommontimedomainresourceallocation->mappingType  = configuration->PUSCH_TimeDomainResourceAllocation_mappingType[CC_id];
  pusch_configcommontimedomainresourceallocation->startSymbolAndLength.buf          = MALLOC(1);
  pusch_configcommontimedomainresourceallocation->startSymbolAndLength.size         = 1;
  pusch_configcommontimedomainresourceallocation->startSymbolAndLength.bits_unused  = 1;
  pusch_configcommontimedomainresourceallocation->startSymbolAndLength.buf[0]       = 0x7f;
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_AllocationList->list,&pusch_configcommontimedomainresourceallocation); 

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble      = CALLOC(1,sizeof(long));
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble)   = configuration->msg3_DeltaPreamble[CC_id];
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant     = CALLOC(1,sizeof(long));
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant)  = configuration->p0_NominalWithGrant[CC_id];

  //Fill  initialUplinkBWP -> BWP-UplinkCommon -> pucch_ConfigCommon//

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon               = CALLOC(1,sizeof(struct NR_SetupRelease_PUCCH_ConfigCommon)); 
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->present      = NR_SetupRelease_PUCCH_ConfigCommon_PR_setup;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup = CALLOC(1,sizeof(struct NR_PUCCH_ConfigCommon));

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon = CALLOC(1,sizeof(BIT_STRING_t));
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId = CALLOC(1,sizeof(BIT_STRING_t));
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal = CALLOC(1,sizeof(long));

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_GroupHopping   = configuration->pucch_GroupHopping[CC_id];
  *((*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal)           = configuration->p0_nominal[CC_id];

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon->buf = MALLOC(1);
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon->size = 1;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon->bits_unused = 4;  
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon->buf[0] = 0x0f;

  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId->buf = MALLOC(2);
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId->size = 2;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId->bits_unused = 6;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId->buf[0] = 0x03;
  (*servingcellconfigcommon)->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId->buf[1] = 0xff;

  //ssb_PositionsInBurst
  (*servingcellconfigcommon)->ssb_PositionsInBurst->present = configuration->ServingCellConfigCommon_ssb_PositionsInBurst_PR[CC_id];

  if((*servingcellconfigcommon)->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap){
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.buf = MALLOC(1);
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.size = 1;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.bits_unused = 4;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.shortBitmap.buf[0] = 0x0f;
  }else if((*servingcellconfigcommon)->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap){
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.buf = MALLOC(1);
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.size = 1;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.bits_unused = 0;
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.mediumBitmap.buf[0] = 0xff;
  }else if((*servingcellconfigcommon)->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_longBitmap){
    (*servingcellconfigcommon)->ssb_PositionsInBurst->choice.longBitmap.buf = MALLOC(8);
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
  struct NR_RateMatchPattern  *ratematchpattern; 
  ratematchpattern            = CALLOC(1,sizeof(struct NR_RateMatchPattern));
  ratematchpattern->rateMatchPatternId    = configuration->rateMatchPatternId[CC_id];
  ratematchpattern->patternType.present   = configuration->RateMatchPattern_patternType[CC_id];

  if(ratematchpattern->patternType.present == NR_RateMatchPattern__patternType_PR_bitmaps){
    ratematchpattern->patternType.choice.bitmaps = CALLOC(1,sizeof(struct NR_RateMatchPattern__patternType__bitmaps));
    memset(&ratematchpattern->patternType.choice.bitmaps,0,sizeof(struct NR_RateMatchPattern__patternType__bitmaps));

    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf = MALLOC(35);
    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.size = 35;
    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.bits_unused = 5;
    ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf[0] = 0x07;
    for (int i =1;i<=34;i++ ){
      ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf[i] =0xff;
    }

    ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.present = configuration->symbolsInResourceBlock[CC_id];
    if(ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.present == NR_RateMatchPattern__patternType__bitmaps__symbolsInResourceBlock_PR_oneSlot){
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf=MALLOC(2);
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.size=2;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.bits_unused=2;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf[0]=0x3f;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf[1]=0xff;      
    }else if(ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.present == NR_RateMatchPattern__patternType__bitmaps__symbolsInResourceBlock_PR_twoSlots){
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf=MALLOC(4);
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.size=4;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.bits_unused=4;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[0]=0x0f;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[1]=0xff;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[2]=0xff;
      ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf[3]=0xff;      
    }

    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern = CALLOC(1,sizeof(struct NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern));
    ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present = configuration->periodicityAndPattern[CC_id];
    if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n2){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.buf = MALLOC(1);
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.bits_unused = 6;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.buf[0] =0x03;
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n4){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.buf = MALLOC(1);
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.bits_unused = 4;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.buf[0] = 0x0f;
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n5){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.buf = MALLOC(1);
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.bits_unused = 3;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.buf[0] = 0x1f;   
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n8){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.buf = MALLOC(1);
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.size = 1;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.bits_unused = 0;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.buf[0] = 0xff;    
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n10){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf = MALLOC(2);
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.size = 2;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.bits_unused = 6;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf[0] = 0x03;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf[1] = 0xff;    
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n20){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf = MALLOC(3);
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.size = 3;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.bits_unused = 4;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf[0] = 0x0f;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf[1] = 0xff;
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf[2] = 0xff;   
    }else if(ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->present == NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern_PR_n40){
      ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf = MALLOC(5);
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

  ratematchpattern->subcarrierSpacing = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  *(ratematchpattern->subcarrierSpacing) = configuration->RateMatchPattern_subcarrierSpacing[CC_id];
  ratematchpattern->mode = configuration->RateMatchPattern_mode[CC_id];

  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->rateMatchPatternToAddModList->list,&ratematchpattern);
  
  //rateMatchPatternToReleaseList
  NR_RateMatchPatternId_t   *ratematchpatternid;
  ratematchpatternid        = CALLOC(1,sizeof(NR_RateMatchPatternId_t));
  *(ratematchpatternid) = configuration->rateMatchPatternId[CC_id];
  ASN_SEQUENCE_ADD(&(*servingcellconfigcommon)->rateMatchPatternToReleaseList->list,&ratematchpatternid);

  //subcarrierSpacing
  *(*servingcellconfigcommon)->subcarrierSpacing            = configuration->NIA_SubcarrierSpacing[CC_id];

  //tdd_UL_DL_ConfigurationCommon
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing       = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->dl_UL_TransmissionPeriodicity    = CALLOC(1,sizeof(long));
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofDownlinkSlots                = CALLOC(1,sizeof(long));
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofDownlinkSymbols              = CALLOC(1,sizeof(long));
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofUplinkSlots                  = CALLOC(1,sizeof(long));
  (*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofUplinkSymbols                = CALLOC(1,sizeof(long));

  *((*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing)    = configuration->referenceSubcarrierSpacing[CC_id];
  *((*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->dl_UL_TransmissionPeriodicity) = configuration->dl_UL_TransmissionPeriodicity[CC_id];
  *((*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofDownlinkSlots)             = configuration->nrofDownlinkSlots[CC_id];
  *((*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofDownlinkSymbols)           = configuration->nrofDownlinkSymbols[CC_id];
  *((*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofUplinkSlots)               = configuration->nrofUplinkSlots[CC_id];
  *((*servingcellconfigcommon)->tdd_UL_DL_ConfigurationCommon->nrofUplinkSymbols)             = configuration->nrofUplinkSymbols[CC_id];

  //ss_PBCH_BlockPower
  (*servingcellconfigcommon)->ss_PBCH_BlockPower           = configuration->ServingCellConfigCommon_ss_PBCH_BlockPower[CC_id];

}
