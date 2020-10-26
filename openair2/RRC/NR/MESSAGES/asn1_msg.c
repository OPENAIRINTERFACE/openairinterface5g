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
* \email: {raymond.knopp, navid.nikaein}@eurecom.fr and kroempa@gmail.com
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

#include "intertask_interface.h"

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

//replace LTE
//extern unsigned char NB_eNB_INST;
extern unsigned char NB_gNB_INST;

extern RAN_CONTEXT_t RC;

/*
 * This is a helper function for xer_sprint, which directs all incoming data
 * into the provided string.
 */
static int xer__nr_print2s (const void *buffer, size_t size, void *app_key) {
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

int xer_nr_sprint (char *string, size_t string_size, asn_TYPE_descriptor_t *td, void *sptr) {
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

//------------------------------------------------------------------------------

uint8_t do_MIB_NR(gNB_RRC_INST *rrc,uint32_t frame) { 

  asn_enc_rval_t enc_rval;
  rrc_gNB_carrier_data_t *carrier = &rrc->carrier;  

  NR_BCCH_BCH_Message_t *mib = &carrier->mib;
  NR_ServingCellConfigCommon_t *scc = carrier->servingcellconfigcommon;

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
  uint16_t *spare= CALLOC(1, sizeof(uint16_t));

  if (spare == NULL) abort();

  mib->message.choice.mib->spare.buf = (uint8_t *)spare;
  mib->message.choice.mib->spare.size = 1;
  mib->message.choice.mib->spare.bits_unused = 7;  // This makes a spare of 1 bits

  mib->message.choice.mib->ssb_SubcarrierOffset = (carrier->ssb_SubcarrierOffset)&15;
  mib->message.choice.mib->pdcch_ConfigSIB1.controlResourceSetZero = *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero;
  mib->message.choice.mib->pdcch_ConfigSIB1.searchSpaceZero = *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero;
  AssertFatal(scc->ssbSubcarrierSpacing != NULL, "scc->ssbSubcarrierSpacing is null\n");
  switch (*scc->ssbSubcarrierSpacing) {
  case NR_SubcarrierSpacing_kHz15:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60;
    break;
    
  case NR_SubcarrierSpacing_kHz30:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs30or120;
    break;
    
  case NR_SubcarrierSpacing_kHz60:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60;
    break;
    
  case NR_SubcarrierSpacing_kHz120:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs30or120;
    break;
    
  case NR_SubcarrierSpacing_kHz240:
    AssertFatal(1==0,"Unknown subCarrierSpacingCommon %d\n",(int)*scc->ssbSubcarrierSpacing);
    break;
    
  default:
      AssertFatal(1==0,"Unknown subCarrierSpacingCommon %d\n",(int)*scc->ssbSubcarrierSpacing);
  }

  switch (scc->dmrs_TypeA_Position) {
  case 	NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2:
    mib->message.choice.mib->dmrs_TypeA_Position = NR_MIB__dmrs_TypeA_Position_pos2;
    break;
    
  case 	NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos3:
    mib->message.choice.mib->dmrs_TypeA_Position = NR_MIB__dmrs_TypeA_Position_pos3;
    break;
    
  default:
    AssertFatal(1==0,"Unknown dmrs_TypeA_Position %d\n",(int)scc->dmrs_TypeA_Position);
  }

  //  assign_enum
  mib->message.choice.mib->cellBarred = NR_MIB__cellBarred_notBarred;
  //  assign_enum
  mib->message.choice.mib->intraFreqReselection = NR_MIB__intraFreqReselection_notAllowed;
  //encode MIB to data
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                   NULL,
                                   (void *)mib,
                                   carrier->MIB,
                                   24);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_SIB1_NR(rrc_gNB_carrier_data_t *carrier, 
	               gNB_RrcConfigurationReq *configuration
                  ) {
  asn_enc_rval_t enc_rval;
  NR_BCCH_DL_SCH_Message_t *sib1_message ;
  struct NR_SIB1 *sib1 ;
  int i;
  struct NR_PLMN_IdentityInfo nr_plmn_info;

  // TODO : Add support for more than one PLMN
  //int num_plmn = configuration->num_plmn;
  int num_plmn = 1;
  struct NR_PLMN_Identity nr_plmn[num_plmn];
  NR_MCC_MNC_Digit_t nr_mcc_digit[num_plmn][3];
  NR_MCC_MNC_Digit_t nr_mnc_digit[num_plmn][3];
  //  struct NR_UAC_BarringInfoSet nr_uac_BarringInfoSet;
  sib1_message = CALLOC(1,sizeof(NR_BCCH_DL_SCH_Message_t));
  memset(sib1_message,0,sizeof(NR_BCCH_DL_SCH_Message_t));
  carrier->siblock1 = sib1_message;
  sib1_message->message.present = NR_BCCH_DL_SCH_MessageType_PR_c1;
  sib1_message->message.choice.c1 = CALLOC(1,sizeof(struct NR_BCCH_DL_SCH_MessageType__c1));
  memset(sib1_message->message.choice.c1,0,sizeof(struct NR_BCCH_DL_SCH_MessageType__c1));
  sib1_message->message.choice.c1->present = NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1;
  sib1_message->message.choice.c1->choice.systemInformationBlockType1 = CALLOC(1,sizeof(struct NR_SIB1));
  sib1 = sib1_message->message.choice.c1->choice.systemInformationBlockType1;
  memset(sib1,0,sizeof(struct NR_SIB1));
  sib1->cellSelectionInfo = CALLOC(1,sizeof(struct NR_SIB1__cellSelectionInfo));
  memset(sib1->cellSelectionInfo,0,sizeof(struct NR_SIB1__cellSelectionInfo));
  sib1->cellSelectionInfo->q_RxLevMin = -50;
  memset(&nr_plmn_info.plmn_IdentityList,0,sizeof(struct NR_PLMN_IdentityInfo__plmn_IdentityList));
  asn_set_empty(&nr_plmn_info.plmn_IdentityList.list);
  memset(&nr_plmn_info,0,sizeof(struct NR_PLMN_IdentityInfo));
  memset(nr_plmn,0,num_plmn*sizeof(struct NR_PLMN_Identity));

  for (i = 0; i < num_plmn; ++i) {
    nr_mcc_digit[i][0] = (configuration->mcc[i]/100)%10;
    nr_mcc_digit[i][1] = (configuration->mcc[i]/10)%10;
    nr_mcc_digit[i][2] = (configuration->mcc[i])%10;
    nr_plmn[i].mcc = CALLOC(1,sizeof(struct NR_MCC));
    memset(nr_plmn[i].mcc,0,sizeof(struct NR_MCC));
    asn_set_empty(&nr_plmn[i].mcc->list);
    ASN_SEQUENCE_ADD(&nr_plmn[i].mcc->list, &nr_mcc_digit[i][0]);
    ASN_SEQUENCE_ADD(&nr_plmn[i].mcc->list, &nr_mcc_digit[i][1]);
    ASN_SEQUENCE_ADD(&nr_plmn[i].mcc->list, &nr_mcc_digit[i][2]);
    nr_mnc_digit[i][0] = (configuration->mnc[i]/100)%10;
    nr_mnc_digit[i][1] = (configuration->mnc[i]/10)%10;
    nr_mnc_digit[i][2] = (configuration->mnc[i])%10;
    memset(&nr_plmn[i].mnc,0,sizeof(NR_MNC_t));
    nr_plmn[i].mnc.list.size=0;
    nr_plmn[i].mnc.list.count=0;
    ASN_SEQUENCE_ADD(&nr_plmn[i].mnc.list, &nr_mnc_digit[i][0]);
    ASN_SEQUENCE_ADD(&nr_plmn[i].mnc.list, &nr_mnc_digit[i][1]);
    ASN_SEQUENCE_ADD(&nr_plmn[i].mnc.list, &nr_mnc_digit[i][2]);
    ASN_SEQUENCE_ADD(&nr_plmn_info.plmn_IdentityList.list, &nr_plmn[i]);
  }//end plmn loop

  nr_plmn_info.cellIdentity.buf = MALLOC(8);
  memset(nr_plmn_info.cellIdentity.buf,0,8);
  nr_plmn_info.cellIdentity.buf[0]= (configuration->cell_identity >> 20) & 0xff;
  nr_plmn_info.cellIdentity.buf[1]= (configuration->cell_identity >> 12) & 0xff;
  nr_plmn_info.cellIdentity.buf[2]= (configuration->cell_identity >> 4) & 0xff;
  nr_plmn_info.cellIdentity.buf[3]= (configuration->cell_identity << 4) & 0xff;
  nr_plmn_info.cellIdentity.size= 4;
  nr_plmn_info.cellIdentity.bits_unused= 4;
  nr_plmn_info.cellReservedForOperatorUse = 0;
  memset(&sib1->cellAccessRelatedInfo,0,sizeof(NR_CellAccessRelatedInfo_t));
  ASN_SEQUENCE_ADD(&sib1->cellAccessRelatedInfo.plmn_IdentityList.list, &nr_plmn_info);
#if 0
  sib1->uac_BarringInfo = CALLOC(1, sizeof(struct NR_SIB1__uac_BarringInfo));
  memset(sib1->uac_BarringInfo, 0, sizeof(struct NR_SIB1__uac_BarringInfo));
  nr_uac_BarringInfoSet.uac_BarringFactor = NR_UAC_BarringInfoSet__uac_BarringFactor_p95;
  nr_uac_BarringInfoSet.uac_BarringTime = NR_UAC_BarringInfoSet__uac_BarringTime_s4;
  nr_uac_BarringInfoSet.uac_BarringForAccessIdentity.buf = MALLOC(1);
  memset(nr_uac_BarringInfoSet.uac_BarringForAccessIdentity.buf,0,1);
  nr_uac_BarringInfoSet.uac_BarringForAccessIdentity.size = 1;
  nr_uac_BarringInfoSet.uac_BarringForAccessIdentity.bits_unused = 1;
  ASN_SEQUENCE_ADD(&sib1->uac_BarringInfo->uac_BarringInfoSetList, &nr_uac_BarringInfoSet);
#endif
  //encode SIB1 to data
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)sib1_message,
                                   carrier->SIB1,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}


void  do_RLC_BEARER(uint8_t Mod_id,
                    int CC_id,
                    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_BearerToAddModList,
                    rlc_bearer_config_t  *rlc_config) {
  struct NR_RLC_BearerConfig *rlc_bearer;
  rlc_bearer = CALLOC(1,sizeof(struct NR_RLC_BearerConfig));
  rlc_bearer->logicalChannelIdentity = rlc_config->LogicalChannelIdentity[CC_id];
  rlc_bearer->servedRadioBearer = CALLOC(1,sizeof(struct NR_RLC_BearerConfig__servedRadioBearer));
  rlc_bearer->servedRadioBearer->present = rlc_config->servedRadioBearer_present[CC_id];

  if(rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity) {
    rlc_bearer->servedRadioBearer->choice.srb_Identity = rlc_config->srb_Identity[CC_id];
  } else if(rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity) {
    rlc_bearer->servedRadioBearer->choice.drb_Identity = rlc_config->drb_Identity[CC_id];
  }

  rlc_bearer->reestablishRLC = CALLOC(1,sizeof(long));
  *(rlc_bearer->reestablishRLC) = rlc_config->reestablishRLC[CC_id];
  rlc_bearer->rlc_Config = CALLOC(1,sizeof(struct NR_RLC_Config));
  rlc_bearer->rlc_Config->present = rlc_config->rlc_Config_present[CC_id];

  if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_am) {
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
  } else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Bi_Directional) {
    rlc_bearer->rlc_Config->choice.um_Bi_Directional = CALLOC(1,sizeof(struct NR_RLC_Config__um_Bi_Directional));
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength) = rlc_config->ul_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength) = rlc_config->dl_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.t_Reassembly   = rlc_config->dl_UM_t_Reassembly[CC_id];
  } else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Uni_Directional_UL) {
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL = CALLOC(1,sizeof(struct NR_RLC_Config__um_Uni_Directional_UL));
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL->ul_UM_RLC.sn_FieldLength    = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL->ul_UM_RLC.sn_FieldLength) = rlc_config->ul_UM_sn_FieldLength[CC_id];
  } else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Uni_Directional_DL) {
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


void do_MAC_CELLGROUP(uint8_t Mod_id,
                      int CC_id,
                      NR_MAC_CellGroupConfig_t *mac_CellGroupConfig,
                      mac_cellgroup_t  *mac_cellgroup_config) {
  mac_CellGroupConfig->drx_Config               = CALLOC(1,sizeof(struct NR_SetupRelease_DRX_Config));
  mac_CellGroupConfig->schedulingRequestConfig  = CALLOC(1,sizeof(struct NR_SchedulingRequestConfig));
  mac_CellGroupConfig->bsr_Config               = CALLOC(1,sizeof(struct NR_BSR_Config));
  mac_CellGroupConfig->tag_Config               = CALLOC(1,sizeof(struct NR_TAG_Config));
  mac_CellGroupConfig->phr_Config               = CALLOC(1,sizeof(struct NR_SetupRelease_PHR_Config));
  mac_CellGroupConfig->drx_Config->present      = mac_cellgroup_config->DRX_Config_PR[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup = CALLOC(1,sizeof(struct NR_DRX_Config));
  mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present = mac_cellgroup_config->drx_onDurationTimer_PR[CC_id];

  if(mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present == NR_DRX_Config__drx_onDurationTimer_PR_subMilliSeconds) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.choice.subMilliSeconds = mac_cellgroup_config->subMilliSeconds[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present == NR_DRX_Config__drx_onDurationTimer_PR_milliSeconds) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.choice.milliSeconds    = mac_cellgroup_config->milliSeconds[CC_id];
  }

  mac_CellGroupConfig->drx_Config->choice.setup->drx_InactivityTimer        = mac_cellgroup_config->drx_InactivityTimer[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_HARQ_RTT_TimerDL       = mac_cellgroup_config->drx_HARQ_RTT_TimerDL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_HARQ_RTT_TimerUL       = mac_cellgroup_config->drx_HARQ_RTT_TimerUL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_RetransmissionTimerDL  = mac_cellgroup_config->drx_RetransmissionTimerDL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_RetransmissionTimerUL  = mac_cellgroup_config->drx_RetransmissionTimerUL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present = mac_cellgroup_config->drx_LongCycleStartOffset_PR[CC_id];

  if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms10) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms10 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms20) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms20 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms32) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms32 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms40) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms40 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms60) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms60 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms64) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms64 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms70) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms70 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms80) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms80 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms128) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms128 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms160) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms160 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms256) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms256 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms320) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms320 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms512) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms512 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms640) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms640 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms1024) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms1024 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms1280) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms1280 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms2048) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms2048 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms2560) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms2560 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms5120) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms5120 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms10240) {
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
  mac_CellGroupConfig->phr_Config->choice.setup->dummy                     = mac_cellgroup_config->phr_Type2SpCell[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Type2OtherCell        = mac_cellgroup_config->phr_Type2OtherCell[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_ModeOtherCG           = mac_cellgroup_config->phr_ModeOtherCG[CC_id];
  mac_CellGroupConfig->skipUplinkTxDynamic      = mac_cellgroup_config->skipUplinkTxDynamic[CC_id];
}


void  do_PHYSICALCELLGROUP(uint8_t Mod_id,
                           int CC_id,
                           NR_PhysicalCellGroupConfig_t *physicalCellGroupConfig,
                           physicalcellgroup_t *physicalcellgroup_config) {
  physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH = CALLOC(1,sizeof(long));
  physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH = CALLOC(1,sizeof(long));
  physicalCellGroupConfig->p_NR_FR1                      = CALLOC(1,sizeof(NR_P_Max_t));
  physicalCellGroupConfig->tpc_SRS_RNTI                  = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->tpc_PUCCH_RNTI                = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->tpc_PUSCH_RNTI                = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->sp_CSI_RNTI                   = CALLOC(1,sizeof(NR_RNTI_Value_t));
  *(physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH) = physicalcellgroup_config->harq_ACK_SpatialBundlingPUCCH[CC_id];
  *(physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH) = physicalcellgroup_config->harq_ACK_SpatialBundlingPUSCH[CC_id];
  *(physicalCellGroupConfig->p_NR_FR1)                      = physicalcellgroup_config->p_NR[CC_id];
  physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook          = physicalcellgroup_config->pdsch_HARQ_ACK_Codebook[CC_id];
  *(physicalCellGroupConfig->tpc_SRS_RNTI)                  = physicalcellgroup_config->tpc_SRS_RNTI[CC_id];
  *(physicalCellGroupConfig->tpc_PUCCH_RNTI)                = physicalcellgroup_config->tpc_PUCCH_RNTI[CC_id];
  *(physicalCellGroupConfig->tpc_PUSCH_RNTI)                = physicalcellgroup_config->tpc_PUSCH_RNTI[CC_id];
  *(physicalCellGroupConfig->sp_CSI_RNTI)                   = physicalcellgroup_config->sp_CSI_RNTI[CC_id];
  physicalCellGroupConfig->cs_RNTI                       = CALLOC(1,sizeof(struct NR_SetupRelease_RNTI_Value));
  physicalCellGroupConfig->cs_RNTI->present              = physicalcellgroup_config->RNTI_Value_PR[CC_id];

  if(physicalCellGroupConfig->cs_RNTI->present == NR_SetupRelease_RNTI_Value_PR_setup) {
    physicalCellGroupConfig->cs_RNTI->choice.setup = physicalcellgroup_config->RNTI_Value[CC_id];
  }
}



void do_SpCellConfig(gNB_RRC_INST *rrc,
                      struct NR_SpCellConfig  *spconfig){
  //gNB_RrcConfigurationReq  *common_configuration;
  //common_configuration = CALLOC(1,sizeof(gNB_RrcConfigurationReq));
  //Fill servingcellconfigcommon config value
  //Fill common config to structure
  //  rrc->configuration = common_configuration;
  spconfig->reconfigurationWithSync = CALLOC(1,sizeof(struct NR_ReconfigurationWithSync));
}

