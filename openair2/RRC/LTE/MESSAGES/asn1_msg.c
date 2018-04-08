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

#include "assertions.h"
#include "RRCConnectionRequest.h"
#include "UL-CCCH-Message.h"
#include "UL-DCCH-Message.h"
#include "DL-CCCH-Message.h"
#include "DL-DCCH-Message.h"
#include "PCCH-Message.h"
#include "openair3/UTILS/conversions.h"
#include "EstablishmentCause.h"
#include "RRCConnectionSetup.h"
#include "SRB-ToAddModList.h"
#include "DRB-ToAddModList.h"
#if defined(Rel10) || defined(Rel14)
#include "MCCH-Message.h"
//#define MRB1 1
#endif

#include "RRC/LITE/defs.h"
#include "RRC/LITE/extern.h"
#include "RRCConnectionSetupComplete.h"
#include "RRCConnectionReconfigurationComplete.h"
#include "RRCConnectionReconfiguration.h"
#include "MasterInformationBlock.h"
#include "SystemInformation.h"

#include "SystemInformationBlockType1.h"

#include "SIB-Type.h"

#include "BCCH-DL-SCH-Message.h"

#include "PHY/defs.h"

#include "MeasObjectToAddModList.h"
#include "ReportConfigToAddModList.h"
#include "MeasIdToAddModList.h"
#include "enb_config.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "common/ran_context.h"
#include "secu_defs.h"

#if !defined (msg)
#define msg printf
#endif

//#define XER_PRINT

typedef struct xer_sprint_string_s {
  char *string;
  size_t string_size;
  size_t string_index;
} xer_sprint_string_t;

extern unsigned char NB_eNB_INST;
extern uint8_t usim_test;

extern RAN_CONTEXT_t RC;

uint16_t two_tier_hexagonal_cellIds[7] = {0,1,2,4,5,7,8};
uint16_t two_tier_hexagonal_adjacent_cellIds[7][6] = {{1,2,4,5,7,8},    // CellId 0
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
static int xer__print2s (const void *buffer, size_t size, void *app_key)
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

int xer_sprint (char *string, size_t string_size, asn_TYPE_descriptor_t *td, void *sptr)
{
  asn_enc_rval_t er;
  xer_sprint_string_t string_buffer;

  string_buffer.string = string;
  string_buffer.string_size = string_size;
  string_buffer.string_index = 0;

  er = xer_encode(td, sptr, XER_F_BASIC, xer__print2s, &string_buffer);

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

uint16_t get_adjacent_cell_id(uint8_t Mod_id,uint8_t index)
{
  return(two_tier_hexagonal_adjacent_cellIds[Mod_id][index]);
}
/* This only works for the hexagonal topology...need a more general function for other topologies */

uint8_t get_adjacent_cell_mod_id(uint16_t phyCellId)
{
  uint8_t i;

  for(i=0; i<7; i++) {
    if(two_tier_hexagonal_cellIds[i] == phyCellId) {
      return i;
    }
  }

  LOG_E(RRC,"\nCannot get adjacent cell mod id! Fatal error!\n");
  return 0xFF; //error!
}

uint8_t do_MIB(rrc_eNB_carrier_data_t *carrier, uint32_t N_RB_DL, uint32_t phich_Resource, uint32_t phich_duration, uint32_t frame)
{

  asn_enc_rval_t enc_rval;
  BCCH_BCH_Message_t *mib=&carrier->mib ;
  uint8_t sfn = (uint8_t)((frame>>2)&0xff);
  uint16_t *spare= calloc(1, sizeof(uint16_t));
  if (spare == NULL) abort();

  switch (N_RB_DL) {

  case 6:
    mib->message.dl_Bandwidth = MasterInformationBlock__dl_Bandwidth_n6;
    break;

  case 15:
    mib->message.dl_Bandwidth = MasterInformationBlock__dl_Bandwidth_n15;
    break;

  case 25:
    mib->message.dl_Bandwidth = MasterInformationBlock__dl_Bandwidth_n25;
    break;

  case 50:
    mib->message.dl_Bandwidth = MasterInformationBlock__dl_Bandwidth_n50;
    break;

  case 75:
    mib->message.dl_Bandwidth = MasterInformationBlock__dl_Bandwidth_n75;
    break;

  case 100:
    mib->message.dl_Bandwidth = MasterInformationBlock__dl_Bandwidth_n100;
    break;
  default:
    AssertFatal(1==0,"Unknown dl_Bandwidth %d\n",N_RB_DL);
  }

  AssertFatal(phich_Resource <= PHICH_Config__phich_Resource_two,"Illegal phich_Resource\n");
  mib->message.phich_Config.phich_Resource = phich_Resource;
  AssertFatal(phich_duration <= PHICH_Config__phich_Duration_extended,"Illegal phich_Duration\n");
  mib->message.phich_Config.phich_Duration = phich_duration;
  LOG_I(RRC,"[MIB] systemBandwidth %x, phich_duration %x, phich_resource %x,sfn %x\n",
         (uint32_t)mib->message.dl_Bandwidth,
         (uint32_t)phich_duration,
         (uint32_t)phich_Resource,
         (uint32_t)sfn);

  mib->message.systemFrameNumber.buf = &sfn;
  mib->message.systemFrameNumber.size = 1;
  mib->message.systemFrameNumber.bits_unused=0;
  mib->message.spare.buf = (uint8_t *)spare;
#ifndef Rel14
  mib->message.spare.size = 2;
  mib->message.spare.bits_unused = 6;  // This makes a spare of 10 bits
#else
  mib->message.spare.size = 1;
  mib->message.spare.bits_unused = 3;  // This makes a spare of 5 bits
  mib->message.schedulingInfoSIB1_BR_r13 = 0; // turn off eMTC
#endif

  enc_rval = uper_encode_to_buffer(&asn_DEF_BCCH_BCH_Message,
                                   (void*)mib,
                                   carrier->MIB,
                                   24);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  /*
#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_BCCH_BCH_Message, (void *) &mib)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_BCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_bcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_bcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, enb_module_idP, msg_p);
    }
  }
# endif
#endif
  */
  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_SIB1(rrc_eNB_carrier_data_t *carrier,
		int Mod_id,int CC_id
#if defined(ENABLE_ITTI)
                , RrcConfigurationReq *configuration
#endif
               )
{

  //  SystemInformation_t systemInformation;
  PLMN_IdentityInfo_t PLMN_identity_info;
  MCC_MNC_Digit_t dummy_mcc[3],dummy_mnc[3];
  asn_enc_rval_t enc_rval;
  SchedulingInfo_t schedulingInfo;
  SIB_Type_t sib_type;

  uint8_t *buffer                      = carrier->SIB1;
  BCCH_DL_SCH_Message_t *bcch_message  = &carrier->siblock1;
  SystemInformationBlockType1_t **sib1 = &carrier->sib1;

  
  memset(bcch_message,0,sizeof(BCCH_DL_SCH_Message_t));
  bcch_message->message.present = BCCH_DL_SCH_MessageType_PR_c1;
  bcch_message->message.choice.c1.present = BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1;
  //  memcpy(&bcch_message.message.choice.c1.choice.systemInformationBlockType1,sib1,sizeof(SystemInformationBlockType1_t));

  *sib1 = &bcch_message->message.choice.c1.choice.systemInformationBlockType1;

  memset(&PLMN_identity_info,0,sizeof(PLMN_IdentityInfo_t));
  memset(&schedulingInfo,0,sizeof(SchedulingInfo_t));
  memset(&sib_type,0,sizeof(SIB_Type_t));



  PLMN_identity_info.plmn_Identity.mcc = CALLOC(1,sizeof(*PLMN_identity_info.plmn_Identity.mcc));
  memset(PLMN_identity_info.plmn_Identity.mcc,0,sizeof(*PLMN_identity_info.plmn_Identity.mcc));

  asn_set_empty(&PLMN_identity_info.plmn_Identity.mcc->list);//.size=0;

#if defined(ENABLE_ITTI)
  dummy_mcc[0] = (configuration->mcc / 100) % 10;
  dummy_mcc[1] = (configuration->mcc / 10) % 10;
  dummy_mcc[2] = (configuration->mcc / 1) % 10;
#else
  dummy_mcc[0] = 0;
  dummy_mcc[1] = 0;
  dummy_mcc[2] = 1;
#endif
  ASN_SEQUENCE_ADD(&PLMN_identity_info.plmn_Identity.mcc->list,&dummy_mcc[0]);
  ASN_SEQUENCE_ADD(&PLMN_identity_info.plmn_Identity.mcc->list,&dummy_mcc[1]);
  ASN_SEQUENCE_ADD(&PLMN_identity_info.plmn_Identity.mcc->list,&dummy_mcc[2]);

  PLMN_identity_info.plmn_Identity.mnc.list.size=0;
  PLMN_identity_info.plmn_Identity.mnc.list.count=0;
#if defined(ENABLE_ITTI)

  if (configuration->mnc >= 100) {
    dummy_mnc[0] = (configuration->mnc / 100) % 10;
    dummy_mnc[1] = (configuration->mnc / 10) % 10;
    dummy_mnc[2] = (configuration->mnc / 1) % 10;
  } else {
    if (configuration->mnc_digit_length == 2) {
      dummy_mnc[0] = (configuration->mnc / 10) % 10;
      dummy_mnc[1] = (configuration->mnc / 1) % 10;
      dummy_mnc[2] = 0xf;
    } else {
      dummy_mnc[0] = (configuration->mnc / 100) % 100;
      dummy_mnc[1] = (configuration->mnc / 10) % 10;
      dummy_mnc[2] = (configuration->mnc / 1) % 10;
    }
  }

#else
  dummy_mnc[0] = 0;
  dummy_mnc[1] = 1;
  dummy_mnc[2] = 0xf;
#endif
  ASN_SEQUENCE_ADD(&PLMN_identity_info.plmn_Identity.mnc.list,&dummy_mnc[0]);
  ASN_SEQUENCE_ADD(&PLMN_identity_info.plmn_Identity.mnc.list,&dummy_mnc[1]);

  if (dummy_mnc[2] != 0xf) {
    ASN_SEQUENCE_ADD(&PLMN_identity_info.plmn_Identity.mnc.list,&dummy_mnc[2]);
  }

  //assign_enum(&PLMN_identity_info.cellReservedForOperatorUse,PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved);
  PLMN_identity_info.cellReservedForOperatorUse=PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;

  ASN_SEQUENCE_ADD(&(*sib1)->cellAccessRelatedInfo.plmn_IdentityList.list,&PLMN_identity_info);


  // 16 bits
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf = MALLOC(2);
#if defined(ENABLE_ITTI)
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf[0] = (configuration->tac >> 8) & 0xff;
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf[1] = (configuration->tac >> 0) & 0xff;
#else
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf[0] = 0x00;
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf[1] = 0x01;
#endif
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.size=2;
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.bits_unused=0;

  // 28 bits
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf = MALLOC(8);
#if defined(ENABLE_ITTI)
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[0] = (configuration->cell_identity >> 20) & 0xff;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[1] = (configuration->cell_identity >> 12) & 0xff;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[2] = (configuration->cell_identity >>  4) & 0xff;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[3] = (configuration->cell_identity <<  4) & 0xf0;
#else
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[0] = 0x00;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[1] = 0x00;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[2] = 0x00;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[3] = 0x10;
#endif
  (*sib1)->cellAccessRelatedInfo.cellIdentity.size=4;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.bits_unused=4;

  //  assign_enum(&(*sib1)->cellAccessRelatedInfo.cellBarred,SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred);
  (*sib1)->cellAccessRelatedInfo.cellBarred=SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred;

  //  assign_enum(&(*sib1)->cellAccessRelatedInfo.intraFreqReselection,SystemInformationBlockType1__cellAccessRelatedInfo__intraFreqReselection_allowed);
  (*sib1)->cellAccessRelatedInfo.intraFreqReselection=SystemInformationBlockType1__cellAccessRelatedInfo__intraFreqReselection_notAllowed;
  (*sib1)->cellAccessRelatedInfo.csg_Indication=0;

  (*sib1)->cellSelectionInfo.q_RxLevMin=-65;
  (*sib1)->cellSelectionInfo.q_RxLevMinOffset=NULL;
  //(*sib1)->p_Max = CALLOC(1, sizeof(P_Max_t));
  //*((*sib1)->p_Max) = 23;
  (*sib1)->freqBandIndicator =
#if defined(ENABLE_ITTI)
    configuration->eutra_band[CC_id];
#else
    7;
#endif

  schedulingInfo.si_Periodicity=SchedulingInfo__si_Periodicity_rf8;

  // This is for SIB2/3
  sib_type=SIB_Type_sibType3;
  ASN_SEQUENCE_ADD(&schedulingInfo.sib_MappingInfo.list,&sib_type);
  ASN_SEQUENCE_ADD(&(*sib1)->schedulingInfoList.list,&schedulingInfo);

  //  ASN_SEQUENCE_ADD(&schedulingInfo.sib_MappingInfo.list,NULL);

#if defined(ENABLE_ITTI)

  if (configuration->frame_type[CC_id] == TDD)
#endif
  {
    (*sib1)->tdd_Config =                             CALLOC(1,sizeof(struct TDD_Config));

    (*sib1)->tdd_Config->subframeAssignment =
#if defined(ENABLE_ITTI)
      configuration->tdd_config[CC_id];
#else
      3;
#endif

    (*sib1)->tdd_Config->specialSubframePatterns =
#if defined(ENABLE_ITTI)
      configuration->tdd_config_s[CC_id];
#else
    0;
#endif
  }

  (*sib1)->si_WindowLength=SystemInformationBlockType1__si_WindowLength_ms20;
  (*sib1)->systemInfoValueTag=0;
  //  (*sib1).nonCriticalExtension = calloc(1,sizeof(*(*sib1).nonCriticalExtension));

#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_BCCH_DL_SCH_Message, (void*)bcch_message);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_BCCH_DL_SCH_Message,
                                   (void*)bcch_message,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[10000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_BCCH_DL_SCH_Message, (void *)bcch_message)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_BCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_bcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_bcch.text, message_string, message_string_size);
      itti_send_msg_to_task(TASK_UNKNOWN, Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"[eNB] SystemInformationBlockType1 Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_SIB23(uint8_t Mod_id,

                 int CC_id
#if defined(ENABLE_ITTI)
                 , RrcConfigurationReq *configuration
#endif
                )
{
  struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib2_part,*sib3_part;
#if defined(Rel10) || defined(Rel14)
  struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib13_part;
  MBSFN_SubframeConfigList_t *MBSFNSubframeConfigList;
  MBSFN_AreaInfoList_r9_t *MBSFNArea_list;
  struct MBSFN_AreaInfo_r9 *MBSFN_Area1, *MBSFN_Area2;
#endif
  asn_enc_rval_t enc_rval;

  uint8_t                           *buffer       = RC.rrc[Mod_id]->carrier[CC_id].SIB23;
  BCCH_DL_SCH_Message_t             *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
  SystemInformationBlockType2_t     **sib2        = &RC.rrc[Mod_id]->carrier[CC_id].sib2;
  SystemInformationBlockType3_t     **sib3        = &RC.rrc[Mod_id]->carrier[CC_id].sib3;
#if defined(Rel10) || defined(Rel14)
  SystemInformationBlockType13_r9_t **sib13       = &RC.rrc[Mod_id]->carrier[CC_id].sib13;
  uint8_t                           MBMS_flag     = RC.rrc[Mod_id]->carrier[CC_id].MBMS_flag;
#endif

  if (bcch_message) {
    memset(bcch_message,0,sizeof(BCCH_DL_SCH_Message_t));
  } else {
    LOG_E(RRC,"[eNB %d] BCCH_MESSAGE is null, exiting\n", Mod_id);
    exit(-1);
  }

  if (!sib2) {
    LOG_E(RRC,"[eNB %d] sib2 is null, exiting\n", Mod_id);
    exit(-1);
  }

  if (!sib3) {
    LOG_E(RRC,"[eNB %d] sib3 is null, exiting\n", Mod_id);
    exit(-1);
  }

#if defined(Rel10) || defined(Rel14)
  LOG_I(RRC,"[eNB %d] Configuration SIB2/3, MBMS = %d\n", Mod_id, MBMS_flag);
#else
  LOG_I(RRC,"[eNB %d] Configuration SIB2/3\n", Mod_id);
#endif
  sib2_part = CALLOC(1,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  sib3_part = CALLOC(1,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  memset(sib2_part,0,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  memset(sib3_part,0,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));

  sib2_part->present = SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2;
  sib3_part->present = SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3;

  *sib2 = &sib2_part->choice.sib2;
  *sib3 = &sib3_part->choice.sib3;

#if defined(Rel10) || defined(Rel14)

  if (MBMS_flag > 0) {
    sib13_part = CALLOC(1,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    memset(sib13_part,0,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib13_part->present = SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920;
    *sib13 = &sib13_part->choice.sib13_v920;
  }

#endif

  // sib2

  (*sib2)->ac_BarringInfo = NULL;
#if defined(Rel10) || defined(Rel14)
#if 0
  (*sib2)->ssac_BarringForMMTEL_Voice_r9 = NULL;
  (*sib2)->ssac_BarringForMMTEL_Video_r9 = NULL;
  (*sib2)->ac_BarringForCSFB_r10 = NULL;
#endif
  (*sib2)->ext1 = NULL;
  (*sib2)->ext2 = NULL;
#endif

#if defined(ENABLE_ITTI)

  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles                         = configuration->rach_numberOfRA_Preambles[CC_id];
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig                        = NULL;

  if (configuration->rach_preamblesGroupAConfig[CC_id]) {
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig
      = CALLOC(1,sizeof(struct RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig));
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->sizeOfRA_PreamblesGroupA
      = configuration->rach_sizeOfRA_PreamblesGroupA[CC_id];
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messageSizeGroupA
      = configuration->rach_messageSizeGroupA[CC_id];
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messagePowerOffsetGroupB
      = configuration->rach_messagePowerOffsetGroupB[CC_id];
  }

  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep                   = configuration->rach_powerRampingStep[CC_id];
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower =
    configuration->rach_preambleInitialReceivedTargetPower[CC_id];
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax                       = configuration->rach_preambleTransMax[CC_id];
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize                  = configuration->rach_raResponseWindowSize[CC_id];
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer          =
    configuration->rach_macContentionResolutionTimer[CC_id];
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.maxHARQ_Msg3Tx                                            = configuration->rach_maxHARQ_Msg3Tx[CC_id];

  // BCCH-Config
  (*sib2)->radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff
    = configuration->bcch_modificationPeriodCoeff[CC_id];

  // PCCH-Config
  (*sib2)->radioResourceConfigCommon.pcch_Config.defaultPagingCycle
    = configuration->pcch_defaultPagingCycle[CC_id];
  (*sib2)->radioResourceConfigCommon.pcch_Config.nB
    = configuration->pcch_nB[CC_id];

  // PRACH-Config
  (*sib2)->radioResourceConfigCommon.prach_Config.rootSequenceIndex
    = configuration->prach_root[CC_id];
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_ConfigIndex
    = configuration->prach_config_index[CC_id];
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.highSpeedFlag
    = configuration->prach_high_speed[CC_id];
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig
    = configuration->prach_zero_correlation[CC_id];
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_FreqOffset
    = configuration->prach_freq_offset[CC_id];

  // PDSCH-Config
  (*sib2)->radioResourceConfigCommon.pdsch_ConfigCommon.referenceSignalPower
    = configuration->pdsch_referenceSignalPower[CC_id];
  (*sib2)->radioResourceConfigCommon.pdsch_ConfigCommon.p_b
    = configuration->pdsch_p_b[CC_id];

  // PUSCH-Config
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.n_SB
    = configuration->pusch_n_SB[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode
    = configuration->pusch_hoppingMode[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset
    = configuration->pusch_hoppingOffset[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM
    = configuration->pusch_enable64QAM[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled
    = configuration->pusch_groupHoppingEnabled[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH
    = configuration->pusch_groupAssignment[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled
    = configuration->pusch_sequenceHoppingEnabled[CC_id];
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift
    = configuration->pusch_nDMRS1[CC_id];

  // PUCCH-Config

  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift
    = configuration->pucch_delta_shift[CC_id];
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.nRB_CQI
    = configuration->pucch_nRB_CQI[CC_id];
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.nCS_AN
    = configuration->pucch_nCS_AN[CC_id];
#if !defined(Rel10) && !defined(Rel14)
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.n1PUCCH_AN
    = configuration->pucch_n1_AN[CC_id];
#endif

  // SRS Config
  if (configuration->srs_enable[CC_id]==1) {
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
      = SoundingRS_UL_ConfigCommon_PR_setup;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig
      = configuration->srs_BandwidthConfig[CC_id];
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig
      = configuration->srs_SubframeConfig[CC_id];
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission
      = configuration->srs_ackNackST[CC_id];

    if (configuration->srs_MaxUpPts[CC_id]) {
      (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts
        = CALLOC(1,sizeof(long));
      *(*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts=1;
    } else {
      (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts = NULL;
    }
    RC.rrc[Mod_id]->srs_enable[CC_id] = 1;
  } else {
    RC.rrc[Mod_id]->srs_enable[CC_id] = 0;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present=SoundingRS_UL_ConfigCommon_PR_release;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.release=0;
  }

  // uplinkPowerControlCommon

  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUSCH
    = configuration->pusch_p0_Nominal[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUCCH
    = configuration->pucch_p0_Nominal[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.alpha
    = configuration->pusch_alpha[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1
    = configuration->pucch_deltaF_Format1[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b
    = configuration->pucch_deltaF_Format1b[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2
    = configuration->pucch_deltaF_Format2[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a
    = configuration->pucch_deltaF_Format2a[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b
    = configuration->pucch_deltaF_Format2b[CC_id];
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaPreambleMsg3
    = configuration->msg3_delta_Preamble[CC_id];
  (*sib2)->radioResourceConfigCommon.ul_CyclicPrefixLength
    = configuration->ul_CyclicPrefixLength[CC_id];

  // UE Timers and Constants

  (*sib2)->ue_TimersAndConstants.t300
    = configuration->ue_TimersAndConstants_t300[CC_id];
  (*sib2)->ue_TimersAndConstants.t301
    = configuration->ue_TimersAndConstants_t301[CC_id];
  (*sib2)->ue_TimersAndConstants.t310
    = configuration->ue_TimersAndConstants_t310[CC_id];
  (*sib2)->ue_TimersAndConstants.n310
    = configuration->ue_TimersAndConstants_n310[CC_id];
  (*sib2)->ue_TimersAndConstants.t311
    = configuration->ue_TimersAndConstants_t311[CC_id];
  (*sib2)->ue_TimersAndConstants.n311
    = configuration->ue_TimersAndConstants_n311[CC_id];

#else
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles=RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n64;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig = NULL;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep=RACH_ConfigCommon__powerRampingParameters__powerRampingStep_dB2;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower=
    RACH_ConfigCommon__powerRampingParameters__preambleInitialReceivedTargetPower_dBm_100;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax=RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize=RACH_ConfigCommon__ra_SupervisionInfo__ra_ResponseWindowSize_sf10;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer=
    RACH_ConfigCommon__ra_SupervisionInfo__mac_ContentionResolutionTimer_sf48;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.maxHARQ_Msg3Tx = 4;

  // BCCH-Config
  (*sib2)->radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff=BCCH_Config__modificationPeriodCoeff_n2;

  // PCCH-Config
  (*sib2)->radioResourceConfigCommon.pcch_Config.defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf128;
  (*sib2)->radioResourceConfigCommon.pcch_Config.nB=PCCH_Config__nB_oneT;

  // PRACH-Config
  (*sib2)->radioResourceConfigCommon.prach_Config.rootSequenceIndex=Mod_id;//0;//384;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_ConfigIndex = 0;//3;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.highSpeedFlag = 0;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig = 1;//12;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_FreqOffset = 2;

  // PDSCH-Config
  (*sib2)->radioResourceConfigCommon.pdsch_ConfigCommon.referenceSignalPower=0;  // corresponds to 24.7 dBm 5 MHz/ 27.7 10 MHz/ 30.7 20 MHz


  (*sib2)->radioResourceConfigCommon.pdsch_ConfigCommon.p_b=0;

  // PUSCH-Config
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.n_SB=1;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode=PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset=0;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM=0;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled=1;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH=0;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled=0;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift=0;

  // PUCCH-Config

  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift=PUCCH_ConfigCommon__deltaPUCCH_Shift_ds1;
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.nRB_CQI = 1;
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.nCS_AN = 0;
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.n1PUCCH_AN = 32;


  (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present=SoundingRS_UL_ConfigCommon_PR_release;
  (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.release=0;

  // uplinkPowerControlCommon

  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUSCH = -108;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUCCH = -108;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.alpha=UplinkPowerControlCommon__alpha_al1;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1=DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b=DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;

  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2=DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;

  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a=DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;

  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b=DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;

  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaPreambleMsg3 = 6;

  (*sib2)->radioResourceConfigCommon.ul_CyclicPrefixLength=UL_CyclicPrefixLength_len1;

  (*sib2)->ue_TimersAndConstants.t300=UE_TimersAndConstants__t300_ms1000;

  (*sib2)->ue_TimersAndConstants.t301=UE_TimersAndConstants__t301_ms1000;

  (*sib2)->ue_TimersAndConstants.t310=UE_TimersAndConstants__t310_ms1000;

  (*sib2)->ue_TimersAndConstants.n310=UE_TimersAndConstants__n310_n20;

  (*sib2)->ue_TimersAndConstants.t311=UE_TimersAndConstants__t311_ms10000;

  (*sib2)->ue_TimersAndConstants.n311=UE_TimersAndConstants__n311_n1;

#endif

  (*sib2)->freqInfo.additionalSpectrumEmission = 1;
  (*sib2)->freqInfo.ul_CarrierFreq = NULL;
  (*sib2)->freqInfo.ul_Bandwidth = NULL;
  //  (*sib2)->mbsfn_SubframeConfigList = NULL;

#if defined(Rel10) || defined(Rel14)

  if (MBMS_flag > 0) {
    LOG_I(RRC,"Adding MBSFN subframe Configuration 1 to SIB2\n");
    MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig1;
    (*sib2)->mbsfn_SubframeConfigList = CALLOC(1,sizeof(struct MBSFN_SubframeConfigList));
    MBSFNSubframeConfigList = (*sib2)->mbsfn_SubframeConfigList;

    sib2_mbsfn_SubframeConfig1= CALLOC(1,sizeof(*sib2_mbsfn_SubframeConfig1));
    memset((void*)sib2_mbsfn_SubframeConfig1,0,sizeof(*sib2_mbsfn_SubframeConfig1));

    sib2_mbsfn_SubframeConfig1->radioframeAllocationPeriod= MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
    sib2_mbsfn_SubframeConfig1->radioframeAllocationOffset= 1;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.present= MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;

    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x38<<2;

    ASN_SEQUENCE_ADD(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig1);

    if (MBMS_flag == 4 ) {
      LOG_I(RRC,"Adding MBSFN subframe Configuration 2 to SIB2\n");
      MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig2;
      sib2_mbsfn_SubframeConfig2= CALLOC(1,sizeof(*sib2_mbsfn_SubframeConfig2));
      memset((void*)sib2_mbsfn_SubframeConfig2,0,sizeof(*sib2_mbsfn_SubframeConfig2));

      sib2_mbsfn_SubframeConfig2->radioframeAllocationPeriod= MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
      sib2_mbsfn_SubframeConfig2->radioframeAllocationOffset= 1;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.present= MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.size= 1;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.bits_unused= 2;

      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.buf[0]=0x07<<2;
     

      ASN_SEQUENCE_ADD(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig2);
    }
  }

#else // no MBMS transmission
  (*sib2)->mbsfn_SubframeConfigList = NULL;
#endif

  (*sib2)->timeAlignmentTimerCommon=TimeAlignmentTimer_infinity;//TimeAlignmentTimer_sf5120;

  /// (*SIB3)
#if defined(Rel10) || defined(Rel14)
  (*sib3)->ext1 = NULL;
#if 0
  (*sib3)->s_IntraSearch_v920=NULL;
  (*sib3)->s_NonIntraSearch_v920=NULL;
  (*sib3)->q_QualMin_r9=NULL;
  (*sib3)->threshServingLowQ_r9=NULL;
#endif
#endif
  (*sib3)->cellReselectionInfoCommon.q_Hyst=SystemInformationBlockType3__cellReselectionInfoCommon__q_Hyst_dB4;

  (*sib3)->cellReselectionInfoCommon.speedStateReselectionPars=NULL;

  (*sib3)->cellReselectionServingFreqInfo.s_NonIntraSearch=NULL;
  (*sib3)->cellReselectionServingFreqInfo.threshServingLow=31;
  (*sib3)->cellReselectionServingFreqInfo.cellReselectionPriority=7;

  (*sib3)->intraFreqCellReselectionInfo.q_RxLevMin = -70;
  (*sib3)->intraFreqCellReselectionInfo.p_Max = NULL;
  (*sib3)->intraFreqCellReselectionInfo.s_IntraSearch = CALLOC(1,sizeof(*(*sib3)->intraFreqCellReselectionInfo.s_IntraSearch));
  *(*sib3)->intraFreqCellReselectionInfo.s_IntraSearch = 31;
  (*sib3)->intraFreqCellReselectionInfo.allowedMeasBandwidth=CALLOC(1,sizeof(*(*sib3)->intraFreqCellReselectionInfo.allowedMeasBandwidth));

  *(*sib3)->intraFreqCellReselectionInfo.allowedMeasBandwidth = AllowedMeasBandwidth_mbw6;

  (*sib3)->intraFreqCellReselectionInfo.presenceAntennaPort1 = 0;
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.buf = CALLOC(8,1);
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.size = 1;
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.buf[0] = 1 << 6;
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.bits_unused = 6;
  (*sib3)->intraFreqCellReselectionInfo.t_ReselectionEUTRA = 1;
  (*sib3)->intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF = (struct SpeedStateScaleFactors *)NULL;

  // SIB13
  // fill in all elements of SIB13 if present
#if defined(Rel10) || defined(Rel14)

  if (MBMS_flag > 0 ) {
    //  Notification for mcch change
    (*sib13)->notificationConfig_r9.notificationRepetitionCoeff_r9= MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
    (*sib13)->notificationConfig_r9.notificationOffset_r9= 0;
    (*sib13)->notificationConfig_r9.notificationSF_Index_r9= 1;

    //  MBSFN-AreaInfoList
    MBSFNArea_list= &(*sib13)->mbsfn_AreaInfoList_r9;//CALLOC(1,sizeof(*MBSFNArea_list));
    memset(MBSFNArea_list,0,sizeof(*MBSFNArea_list));
    // MBSFN Area 1
    MBSFN_Area1= CALLOC(1, sizeof(*MBSFN_Area1));
    MBSFN_Area1->mbsfn_AreaId_r9= 1;
    MBSFN_Area1->non_MBSFNregionLength= MBSFN_AreaInfo_r9__non_MBSFNregionLength_s2;
    MBSFN_Area1->notificationIndicator_r9= 0;
    MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9= MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
    MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9= 1; // in accordance with mbsfn subframe configuration in sib2
    MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9= MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_ModificationPeriod_r9_rf512;
    //  Subframe Allocation Info
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size= 1;

    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=0x20<<2;  // FDD: SF1
    

    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;

    MBSFN_Area1->mcch_Config_r9.signallingMCS_r9= MBSFN_AreaInfo_r9__mcch_Config_r9__signallingMCS_r9_n7;

    ASN_SEQUENCE_ADD(&MBSFNArea_list->list,MBSFN_Area1);

    //MBSFN Area 2: currently only activated for eMBMS relaying
    if (MBMS_flag == 4 ) {
      MBSFN_Area2= CALLOC(1, sizeof(*MBSFN_Area2));
      MBSFN_Area2->mbsfn_AreaId_r9= 2;
      MBSFN_Area2->non_MBSFNregionLength= MBSFN_AreaInfo_r9__non_MBSFNregionLength_s2;
      MBSFN_Area2->notificationIndicator_r9= 1;
      MBSFN_Area2->mcch_Config_r9.mcch_RepetitionPeriod_r9= MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
      MBSFN_Area2->mcch_Config_r9.mcch_Offset_r9= 1;
      MBSFN_Area2->mcch_Config_r9.mcch_ModificationPeriod_r9= MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_ModificationPeriod_r9_rf512;
      // Subframe Allocation Info
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.size= 1;
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;

      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=0x04<<2;  // FDD: SF6
     

      MBSFN_Area2->mcch_Config_r9.signallingMCS_r9= MBSFN_AreaInfo_r9__mcch_Config_r9__signallingMCS_r9_n7;

      ASN_SEQUENCE_ADD(&MBSFNArea_list->list,MBSFN_Area2);
    }

    //  end of adding for MBMS SIB13
  }

#endif

  bcch_message->message.present = BCCH_DL_SCH_MessageType_PR_c1;
  bcch_message->message.choice.c1.present = BCCH_DL_SCH_MessageType__c1_PR_systemInformation;

  /*  memcpy((void*)&bcch_message.message.choice.c1.choice.systemInformation,
   (void*)systemInformation,
   sizeof(SystemInformation_t));*/

  bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.present = SystemInformation__criticalExtensions_PR_systemInformation_r8;

  bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count=0;

  //  asn_set_empty(&systemInformation->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list);//.size=0;
  //  systemInformation->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count=0;
  ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,
                   sib2_part);
  ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,
                   sib3_part);
#if defined(Rel10) || defined(Rel14)

  if (MBMS_flag > 0) {
    ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,sib13_part);
  }

#endif


#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_BCCH_DL_SCH_Message, (void*)bcch_message);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_BCCH_DL_SCH_Message,
                                   (void*)bcch_message,
                                   buffer,
                                   900);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);


#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[15000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_BCCH_DL_SCH_Message, (void *)bcch_message)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_BCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_bcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_bcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"[eNB] SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCConnectionRequest(uint8_t Mod_id, uint8_t *buffer,uint8_t *rv)
{

  asn_enc_rval_t enc_rval;
  uint8_t buf[5],buf2=0;
  uint8_t ecause=0;

  UL_CCCH_Message_t ul_ccch_msg;

  RRCConnectionRequest_t *rrcConnectionRequest;

  memset((void *)&ul_ccch_msg,0,sizeof(UL_CCCH_Message_t));

  ul_ccch_msg.message.present           = UL_CCCH_MessageType_PR_c1;
  ul_ccch_msg.message.choice.c1.present = UL_CCCH_MessageType__c1_PR_rrcConnectionRequest;
  rrcConnectionRequest          = &ul_ccch_msg.message.choice.c1.choice.rrcConnectionRequest;

  rrcConnectionRequest->criticalExtensions.present = RRCConnectionRequest__criticalExtensions_PR_rrcConnectionRequest_r8;

  if (1) {
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.present = InitialUE_Identity_PR_randomValue;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.size = 5;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.bits_unused = 0;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf = buf;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[0] = rv[0];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[1] = rv[1];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[2] = rv[2];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[3] = rv[3];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[4] = rv[4];
  } else {
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.present = InitialUE_Identity_PR_s_TMSI;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.mmec.size = 1;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.mmec.bits_unused = 0;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.mmec.buf = buf;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.mmec.buf[0] = 0x12;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.size = 4;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.bits_unused = 0;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.buf = &buf[1];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.buf[0] = 0x34;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.buf[1] = 0x56;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.buf[2] = 0x78;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.s_TMSI.m_TMSI.buf[3] = 0x9a;
  }

  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.establishmentCause = EstablishmentCause_mo_Signalling; //EstablishmentCause_mo_Data;

  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.spare.buf = &buf2;
  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.spare.size=1;
  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.spare.bits_unused = 7;


  enc_rval = uper_encode_to_buffer(&asn_DEF_UL_CCCH_Message,
                                   (void*)&ul_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_UL_CCCH_Message, (void *) &ul_ccch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_UE, RRC_UL_CCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_ul_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_ul_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, NB_eNB_INST + Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"[UE] RRCConnectionRequest Encoded %zd bits (%zd bytes), ecause %d\n",enc_rval.encoded,(enc_rval.encoded+7)/8,ecause);

  return((enc_rval.encoded+7)/8);

}

uint8_t do_RRCConnectionSetupComplete(uint8_t Mod_id, uint8_t *buffer, const uint8_t Transaction_id, const int dedicatedInfoNASLength, const char *dedicatedInfoNAS)
{


  asn_enc_rval_t enc_rval;

  UL_DCCH_Message_t ul_dcch_msg;

  RRCConnectionSetupComplete_t *rrcConnectionSetupComplete;

  memset((void *)&ul_dcch_msg,0,sizeof(UL_DCCH_Message_t));

  ul_dcch_msg.message.present           = UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete;
  rrcConnectionSetupComplete            = &ul_dcch_msg.message.choice.c1.choice.rrcConnectionSetupComplete;

  rrcConnectionSetupComplete->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionSetupComplete->criticalExtensions.present = RRCConnectionSetupComplete__criticalExtensions_PR_c1;
  rrcConnectionSetupComplete->criticalExtensions.choice.c1.present = RRCConnectionSetupComplete__criticalExtensions__c1_PR_rrcConnectionSetupComplete_r8;

  rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.nonCriticalExtension=CALLOC(1,
      sizeof(*rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.nonCriticalExtension));

  if(usim_test == 0)
      rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.selectedPLMN_Identity= 2;
  else
      rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.selectedPLMN_Identity= 1;

  rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME =
    NULL;//calloc(1,sizeof(*rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME));
  /*
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->plmn_Identity=NULL;
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmegi.buf = calloc(2,1);
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmegi.buf[0] = 0x0;
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmegi.buf[1] = 0x1;
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmegi.size=2;
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmegi.bits_unused=0;
  */
  memset(&rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.dedicatedInfoNAS,0,sizeof(OCTET_STRING_t));
  OCTET_STRING_fromBuf(&rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.dedicatedInfoNAS,
                       dedicatedInfoNAS, dedicatedInfoNASLength);

  /*
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmec.buf = calloc(1,1);
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmec.buf[0] = 0x98;
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmec.size=1;
    rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.registeredMME->mmec.bits_unused=0;
  */

  enc_rval = uper_encode_to_buffer(&asn_DEF_UL_DCCH_Message,
                                   (void*)&ul_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_UL_DCCH_Message, (void *) &ul_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_UE, RRC_UL_DCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_ul_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_ul_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, NB_eNB_INST + Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"RRCConnectionSetupComplete Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  return((enc_rval.encoded+7)/8);

}

//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionReconfigurationComplete(
  const protocol_ctxt_t* const ctxt_pP,
  uint8_t* buffer,
  const uint8_t Transaction_id
)
//------------------------------------------------------------------------------
{


  asn_enc_rval_t enc_rval;

  UL_DCCH_Message_t ul_dcch_msg;

  RRCConnectionReconfigurationComplete_t *rrcConnectionReconfigurationComplete;

  memset((void *)&ul_dcch_msg,0,sizeof(UL_DCCH_Message_t));

  ul_dcch_msg.message.present                     = UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present           = UL_DCCH_MessageType__c1_PR_rrcConnectionReconfigurationComplete;
  rrcConnectionReconfigurationComplete            = &ul_dcch_msg.message.choice.c1.choice.rrcConnectionReconfigurationComplete;

  rrcConnectionReconfigurationComplete->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReconfigurationComplete->criticalExtensions.present =
    RRCConnectionReconfigurationComplete__criticalExtensions_PR_rrcConnectionReconfigurationComplete_r8;
  rrcConnectionReconfigurationComplete->criticalExtensions.choice.rrcConnectionReconfigurationComplete_r8.nonCriticalExtension=NULL;

  enc_rval = uper_encode_to_buffer(&asn_DEF_UL_DCCH_Message,
                                   (void*)&ul_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_UL_DCCH_Message, (void *) &ul_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_UE, RRC_UL_DCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_ul_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_ul_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"RRCConnectionReconfigurationComplete Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  return((enc_rval.encoded+7)/8);
}


//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionSetup(
  const protocol_ctxt_t*     const ctxt_pP,
  rrc_eNB_ue_context_t*      const ue_context_pP,
  int                        CC_id,
  uint8_t*                   const buffer,
  const uint8_t              transmission_mode,
  const uint8_t              Transaction_id,
  SRB_ToAddModList_t  **SRB_configList,
  struct PhysicalConfigDedicated  **physicalConfigDedicated)
{

  asn_enc_rval_t enc_rval;
  uint8_t ecause=0;
  eNB_RRC_INST *rrc               = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[CC_id];
 
  long* logicalchannelgroup = NULL;
  struct SRB_ToAddMod* SRB1_config = NULL;
  struct SRB_ToAddMod__rlc_Config* SRB1_rlc_config = NULL;
  struct SRB_ToAddMod__logicalChannelConfig* SRB1_lchan_config = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters* SRB1_ul_SpecificParameters = NULL;

#ifdef CBA
  struct PUSCH_CBAConfigDedicated_vlola*  pusch_CBAConfigDedicated_vlola = NULL;
  long* betaOffset_CBA_Index = NULL;
  long* cShift_CBA = NULL;
#endif
  PhysicalConfigDedicated_t* physicalConfigDedicated2 = NULL;

  DL_CCCH_Message_t dl_ccch_msg;

  RRCConnectionSetup_t* rrcConnectionSetup = NULL;

  memset((void *)&dl_ccch_msg,0,sizeof(DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = DL_CCCH_MessageType__c1_PR_rrcConnectionSetup;
  rrcConnectionSetup          = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionSetup;

  // RRCConnectionSetup
  // Configure SRB1

  //  *SRB_configList = CALLOC(1,sizeof(*SRB_configList));
  if (*SRB_configList) {
    free(*SRB_configList);
  }

  *SRB_configList = CALLOC(1,sizeof(SRB_ToAddModList_t));

  /// SRB1
  SRB1_config = CALLOC(1,sizeof(*SRB1_config));

  SRB1_config->srb_Identity = 1;
  SRB1_rlc_config = CALLOC(1,sizeof(*SRB1_rlc_config));
  SRB1_config->rlc_Config   = SRB1_rlc_config;

  SRB1_rlc_config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB1_rlc_config->choice.explicitValue.present=RLC_Config_PR_am;
#if defined(ENABLE_ITTI)
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = rrc->srb1_timer_poll_retransmit;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = rrc->srb1_poll_pdu;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = rrc->srb1_poll_byte;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = rrc->srb1_max_retx_threshold;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = rrc->srb1_timer_reordering;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = rrc->srb1_timer_status_prohibit;
#else 
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms20;;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = PollByte_kBinfinity;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t8;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms35;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms0;
#endif 

  SRB1_lchan_config = CALLOC(1,sizeof(*SRB1_lchan_config));
  SRB1_config->logicalChannelConfig   = SRB1_lchan_config;

  SRB1_lchan_config->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
  SRB1_ul_SpecificParameters = CALLOC(1,sizeof(*SRB1_ul_SpecificParameters));

  SRB1_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB1_ul_SpecificParameters;


  SRB1_ul_SpecificParameters->priority = 1;

  //assign_enum(&SRB1_ul_SpecificParameters->prioritisedBitRate,LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity);
  SRB1_ul_SpecificParameters->prioritisedBitRate=LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;

  //assign_enum(&SRB1_ul_SpecificParameters->bucketSizeDuration,LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50);
  SRB1_ul_SpecificParameters->bucketSizeDuration=LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

  logicalchannelgroup = CALLOC(1,sizeof(long));
  *logicalchannelgroup=0;
  SRB1_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;


  ASN_SEQUENCE_ADD(&(*SRB_configList)->list,SRB1_config);

  // PhysicalConfigDedicated

  physicalConfigDedicated2 = CALLOC(1,sizeof(*physicalConfigDedicated2));
  *physicalConfigDedicated = physicalConfigDedicated2;

  physicalConfigDedicated2->pdsch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pdsch_ConfigDedicated));
  physicalConfigDedicated2->pucch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pucch_ConfigDedicated));
  physicalConfigDedicated2->pusch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pusch_ConfigDedicated));
  physicalConfigDedicated2->uplinkPowerControlDedicated   = CALLOC(1,sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated));
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH         = CALLOC(1,sizeof(*physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH));
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH         = CALLOC(1,sizeof(*physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH));
  physicalConfigDedicated2->cqi_ReportConfig              = CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig));
  if (rrc->srs_enable[CC_id]==1)
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated = CALLOC(1,sizeof(*physicalConfigDedicated2->soundingRS_UL_ConfigDedicated));
  else
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated = NULL;
  physicalConfigDedicated2->antennaInfo                   = CALLOC(1,sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->schedulingRequestConfig       = CALLOC(1,sizeof(*physicalConfigDedicated2->schedulingRequestConfig));

  // PDSCH
  //assign_enum(&physicalConfigDedicated2->pdsch_ConfigDedicated->p_a,
  //        PDSCH_ConfigDedicated__p_a_dB0);
  if (carrier->p_eNB==2)
    physicalConfigDedicated2->pdsch_ConfigDedicated->p_a=   PDSCH_ConfigDedicated__p_a_dB_3;
  else
    physicalConfigDedicated2->pdsch_ConfigDedicated->p_a=   PDSCH_ConfigDedicated__p_a_dB0;

  // PUCCH
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.present=PUCCH_ConfigDedicated__ackNackRepetition_PR_release;
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.choice.release=0;

  if (carrier->sib1->tdd_Config == NULL) {
    physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode=NULL;//PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;
  } else { //TDD
    physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode= CALLOC(1,sizeof(long));
    *(physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode) =
      PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_bundling;//PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;
  }

  // Pusch_config_dedicated
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_ACK_Index = 0; // 2.00
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_RI_Index  = 0; // 1.25
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_CQI_Index = 8; // 2.25

  // UplinkPowerControlDedicated
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUSCH = 0; // 0 dB
  //assign_enum(&physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled,
  // UplinkPowerControlDedicated__deltaMCS_Enabled_en1);
  physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled= UplinkPowerControlDedicated__deltaMCS_Enabled_en1;
  physicalConfigDedicated2->uplinkPowerControlDedicated->accumulationEnabled = 1;  // TRUE
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUCCH = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->pSRS_Offset = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient = CALLOC(1,
      sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient));
  //  assign_enum(physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient,FilterCoefficient_fc4); // fc4 dB
  *physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient=FilterCoefficient_fc4; // fc4 dB

  // TPC-PDCCH-Config

  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->present=TPC_PDCCH_Config_PR_setup;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_Index.present = TPC_Index_PR_indexOfFormat3;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_Index.choice.indexOfFormat3 = 1;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf=CALLOC(1,2);
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.size=2;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf[0]=0x12;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf[1]=0x34+ue_context_pP->local_uid;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.bits_unused=0;

  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->present=TPC_PDCCH_Config_PR_setup;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_Index.present = TPC_Index_PR_indexOfFormat3;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_Index.choice.indexOfFormat3 = 1;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf=CALLOC(1,2);
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.size=2;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf[0]=0x22;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf[1]=0x34+ue_context_pP->local_uid;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.bits_unused=0;

  // CQI ReportConfig

  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic=CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic));
#if defined(Rel10) || defined(Rel14)
  *physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic= CQI_ReportModeAperiodic_rm30;
#else
  *physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic=CQI_ReportConfig__cqi_ReportModeAperiodic_rm30; // HLC CQI, no PMI
#endif
  physicalConfigDedicated2->cqi_ReportConfig->nomPDSCH_RS_EPRE_Offset = 0; // 0 dB
  //physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic=NULL;
  
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic=CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic));
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->present =  CQI_ReportPeriodic_PR_release;
    /*
    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->present =  CQI_ReportPeriodic_PR_setup;
    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex = 0;  // n2_pucch
    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_pmi_ConfigIndex = 0;  // Icqi/pmi
    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_FormatIndicatorPeriodic.present = CQI_ReportPeriodic__setup__cqi_FormatIndicatorPeriodic_PR_subbandCQI;  // subband CQI
    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_FormatIndicatorPeriodic.choice.subbandCQI.k=4;

    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex=NULL;
    physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.simultaneousAckNackAndCQI=0;
    */

  //soundingRS-UL-ConfigDedicated
  if (rrc->srs_enable[CC_id]==1) {
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->present = SoundingRS_UL_ConfigDedicated_PR_setup;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth =
                                                             SoundingRS_UL_ConfigDedicated__setup__srs_Bandwidth_bw0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth =
          SoundingRS_UL_ConfigDedicated__setup__srs_HoppingBandwidth_hbw0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition=0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.duration=1;
    if (carrier->sib1->tdd_Config==NULL) { // FDD
      if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
	  == SoundingRS_UL_ConfigCommon_PR_setup)
	if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig!=0) 
	  LOG_W(RRC,"This code has been optimized for SRS Subframe Config 0, but current config is %zd. Expect undefined behaviour!\n",
		carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig);
      if (ue_context_pP->local_uid >=20) 
	LOG_W(RRC,"This code has been optimized for up to 10 UEs, but current UE_id is %d. Expect undefined behaviour!\n",
	      ue_context_pP->local_uid);
      //the current code will allow for 20 UEs - to be revised for more
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex=7+ue_context_pP->local_uid/2;
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb= ue_context_pP->local_uid%2;
    }
    else {
      if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
	  == SoundingRS_UL_ConfigCommon_PR_setup)
	if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig!=7) {
	  LOG_W(RRC,"This code has been optimized for SRS Subframe Config 7 and TDD config 3, but current configs are %zd and %zd. Expect undefined behaviour!\n",
		carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig,
		carrier->sib1->tdd_Config->subframeAssignment);
	}
      if (ue_context_pP->local_uid >=6) 
	LOG_W(RRC,"This code has been optimized for up to 6 UEs, but current UE_id is %d. Expect undefined behaviour!\n",
	      ue_context_pP->local_uid);
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex=17+ue_context_pP->local_uid/2;
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb= ue_context_pP->local_uid%2;
    }
    LOG_W(RRC,"local UID %d, srs ConfigIndex %zd, TransmissionComb %zd\n",ue_context_pP->local_uid,
	  physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex,
	  physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb);

    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift=
          SoundingRS_UL_ConfigDedicated__setup__cyclicShift_cs0;
  }


  //AntennaInfoDedicated
  physicalConfigDedicated2->antennaInfo = CALLOC(1,sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->antennaInfo->present = PhysicalConfigDedicated__antennaInfo_PR_explicitValue;
  //assign_enum(&physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode,
  //     AntennaInfoDedicated__transmissionMode_tm2);

  switch (transmission_mode) {
  default:
    LOG_W(RRC,"At RRCConnectionSetup Transmission mode can only take values 1 or 2! Defaulting to 1!\n");
  case 1:
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm1;
    break;

  case 2:
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm2;
    break;
    /*
  case 3:
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm3;
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction=     CALLOC(1,
        sizeof(*physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction));
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction->present =
      AntennaInfoDedicated__codebookSubsetRestriction_PR_n2TxAntenna_tm3;
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.buf= MALLOC(1);
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.buf[0] = 0xc0;
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.size=1;
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.codebookSubsetRestriction->choice.n2TxAntenna_tm3.bits_unused=6;

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

  case 7:
    physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     AntennaInfoDedicated__transmissionMode_tm7;
    break;
    */
  }


  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.present = AntennaInfoDedicated__ue_TransmitAntennaSelection_PR_release;
  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.release = 0;

  // SchedulingRequestConfig

  physicalConfigDedicated2->schedulingRequestConfig->present = SchedulingRequestConfig_PR_setup;
  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 71 - ue_context_pP->local_uid/10;//ue_context_pP->local_uid;

  if (carrier->sib1->tdd_Config == NULL) { // FDD
    physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 5+(ue_context_pP->local_uid%10);  // Isr = 5 (every 10 subframes, offset=2+UE_id mod3)
  } else {
    switch (carrier->sib1->tdd_Config->subframeAssignment) {
    case 1:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7+(ue_context_pP->local_uid&1)+((
            ue_context_pP->local_uid&3)>>1)*5;  // Isr = 5 (every 10 subframes, offset=2 for UE0, 3 for UE1, 7 for UE2, 8 for UE3 , 2 for UE4 etc..)
      break;

    case 3:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7+
          (ue_context_pP->local_uid%3);  // Isr = 5 (every 10 subframes, offset=2 for UE0, 3 for UE1, 3 for UE2, 2 for UE3 , etc..)
      break;

    case 4:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7+
          (ue_context_pP->local_uid&1);  // Isr = 5 (every 10 subframes, offset=2 for UE0, 3 for UE1, 3 for UE2, 2 for UE3 , etc..)
      break;

    default:
      physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 7;  // Isr = 5 (every 10 subframes, offset=2 for all UE0 etc..)
      break;
    }
  }

  //  assign_enum(&physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax,
  //SchedulingRequestConfig__setup__dsr_TransMax_n4);
  //  assign_enum(&physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax = SchedulingRequestConfig__setup__dsr_TransMax_n4;
  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax = SchedulingRequestConfig__setup__dsr_TransMax_n4;

  rrcConnectionSetup->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionSetup->criticalExtensions.present = RRCConnectionSetup__criticalExtensions_PR_c1;
  rrcConnectionSetup->criticalExtensions.choice.c1.present =RRCConnectionSetup__criticalExtensions__c1_PR_rrcConnectionSetup_r8 ;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.srb_ToAddModList = *SRB_configList;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.drb_ToAddModList = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.drb_ToReleaseList = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.sps_Config = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.physicalConfigDedicated = physicalConfigDedicated2;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig = NULL;
#ifdef CBA
  betaOffset_CBA_Index = CALLOC(1,sizeof(long));
  cShift_CBA = CALLOC(1,sizeof(long));
  *betaOffset_CBA_Index=10; // need to be changed by Kaijie
  *cShift_CBA=4;
  physicalConfigDedicated2->pusch_CBAConfigDedicated_vlola->betaOffset_CBA_Index=betaOffset_CBA_Index;
  physicalConfigDedicated2->pusch_CBAConfigDedicated_vlola->cShift_CBA=cShift_CBA;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.sps_CBA_ConfigList_vlola = NULL;
#endif


#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_DL_CCCH_Message, (void*)&dl_ccch_msg);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_CCCH_Message,
                                   (void*)&dl_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_CCCH_Message, (void *) &dl_ccch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_CCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"RRCConnectionSetup Encoded %zd bits (%zd bytes), ecause %d\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8,ecause);

  //  FREEMEM(SRB_list);
  //  free(SRB1_config);
  //  free(SRB1_rlc_config);
  //  free(SRB1_lchan_config);
  //  free(SRB1_ul_SpecificParameters);

  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_SecurityModeCommand(
  const protocol_ctxt_t* const ctxt_pP,
  uint8_t* const buffer,
  const uint8_t Transaction_id,
  const uint8_t cipheringAlgorithm,
  const uint8_t integrityProtAlgorithm
)
//------------------------------------------------------------------------------
{
  DL_DCCH_Message_t dl_dcch_msg;
  asn_enc_rval_t enc_rval;

  memset(&dl_dcch_msg,0,sizeof(DL_DCCH_Message_t));

  dl_dcch_msg.message.present           = DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = DL_DCCH_MessageType__c1_PR_securityModeCommand;

  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.present = SecurityModeCommand__criticalExtensions_PR_c1;

  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.choice.c1.present =
    SecurityModeCommand__criticalExtensions__c1_PR_securityModeCommand_r8;
  // the two following information could be based on the mod_id
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm
    = (CipheringAlgorithm_r12_t)cipheringAlgorithm;
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm
    = (e_SecurityAlgorithmConfig__integrityProtAlgorithm)integrityProtAlgorithm;

#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_DL_DCCH_Message, (void*)&dl_dcch_msg);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_DCCH_Message,
                                   (void*)&dl_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_DCCH_Message, (void *) &dl_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_DCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"[eNB %d] securityModeCommand for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    LOG_E(RRC,"[eNB %d] ASN1 : securityModeCommand encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  //  rrc_ue_process_ueCapabilityEnquiry(0,1000,&dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry,0);
  //  exit(-1);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_UECapabilityEnquiry(
  const protocol_ctxt_t* const ctxt_pP,
  uint8_t*               const buffer,
  const uint8_t                Transaction_id
)
//------------------------------------------------------------------------------
{

  DL_DCCH_Message_t dl_dcch_msg;

  RAT_Type_t rat=RAT_Type_eutra;
  asn_enc_rval_t enc_rval;

  memset(&dl_dcch_msg,0,sizeof(DL_DCCH_Message_t));

  dl_dcch_msg.message.present           = DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry;

  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.rrc_TransactionIdentifier = Transaction_id;

  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.present = UECapabilityEnquiry__criticalExtensions_PR_c1;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.present =
    UECapabilityEnquiry__criticalExtensions__c1_PR_ueCapabilityEnquiry_r8;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.count=0;
  ASN_SEQUENCE_ADD(&dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list,
                   &rat);

#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_DL_DCCH_Message, (void*)&dl_dcch_msg);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_DCCH_Message,
                                   (void*)&dl_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_DCCH_Message, (void *) &dl_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_CCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"[eNB %d] UECapabilityRequest for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    LOG_E(RRC,"[eNB %d] ASN1 : UECapabilityRequest encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint16_t
do_RRCConnectionReconfiguration(
  const protocol_ctxt_t*        const ctxt_pP,
  uint8_t                            *buffer,
  uint8_t                             Transaction_id,
  SRB_ToAddModList_t                 *SRB_list,
  DRB_ToAddModList_t                 *DRB_list,
  DRB_ToReleaseList_t                *DRB_list2,
  struct SPS_Config                  *sps_Config,
  struct PhysicalConfigDedicated     *physicalConfigDedicated,
  MeasObjectToAddModList_t           *MeasObj_list,
  ReportConfigToAddModList_t         *ReportConfig_list,
  QuantityConfig_t                   *quantityConfig,
  MeasIdToAddModList_t               *MeasId_list,
  MAC_MainConfig_t                   *mac_MainConfig,
  MeasGapConfig_t                    *measGapConfig,
  MobilityControlInfo_t              *mobilityInfo,
  struct MeasConfig__speedStatePars  *speedStatePars,
  RSRP_Range_t                       *rsrp,
  C_RNTI_t                           *cba_rnti,
  struct RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList
  *dedicatedInfoNASList

#if defined(Rel10) || defined(Rel14)
  , SCellToAddMod_r10_t  *SCell_config
#endif
)
//------------------------------------------------------------------------------
{

  asn_enc_rval_t enc_rval;

  DL_DCCH_Message_t dl_dcch_msg;
  RRCConnectionReconfiguration_t *rrcConnectionReconfiguration;


  memset(&dl_dcch_msg,0,sizeof(DL_DCCH_Message_t));

  dl_dcch_msg.message.present           = DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration;
  rrcConnectionReconfiguration          = &dl_dcch_msg.message.choice.c1.choice.rrcConnectionReconfiguration;

  // RRCConnectionReconfiguration
  rrcConnectionReconfiguration->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReconfiguration->criticalExtensions.present = RRCConnectionReconfiguration__criticalExtensions_PR_c1;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.present =RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8 ;

  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated = CALLOC(1,
      sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated));
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList = SRB_list;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList = DRB_list;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToReleaseList = DRB_list2;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->sps_Config = sps_Config;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->physicalConfigDedicated = physicalConfigDedicated;
#ifdef CBA
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->cba_RNTI_vlola= cba_rnti;
#endif

  if (mac_MainConfig!=NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig->present
      =RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue;
    memcpy(&rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue,
           mac_MainConfig,
           sizeof(*mac_MainConfig));
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig=NULL;
  }

  if (MeasId_list != NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig           = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig));
    memset((void*)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig,
           0, sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig));

    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->reportConfigToAddModList = ReportConfig_list;
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measIdToAddModList       = MeasId_list;
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->measObjectToAddModList   = MeasObj_list;

    if (quantityConfig!=NULL) {
      rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->quantityConfig = CALLOC(1,
          sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->quantityConfig));
      memcpy((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->quantityConfig,
             (void *)quantityConfig,
             sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->quantityConfig));
    } else {
      rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->quantityConfig = NULL;
    }

    if(speedStatePars != NULL) {
      rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->speedStatePars = CALLOC(1,
          sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->speedStatePars));
      memcpy((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->speedStatePars,
             (void *)speedStatePars,sizeof(*speedStatePars));
    } else {
      rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->speedStatePars = NULL;
    }

    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig->s_Measure= rsrp;
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig = NULL;
  }

  if (mobilityInfo !=NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo));
    memcpy((void*)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo, (void*)mobilityInfo,
           sizeof(MobilityControlInfo_t));

  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo  = NULL;
  }

  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.dedicatedInfoNASList = dedicatedInfoNASList;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.securityConfigHO     = NULL;

  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_DCCH_Message,
                                   (void*)&dl_dcch_msg,
                                   buffer,
                                   RRC_BUF_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %zd)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#ifdef XER_PRINT
  xer_fprint(stdout,&asn_DEF_DL_DCCH_Message,(void*)&dl_dcch_msg);
#endif

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[30000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_DCCH_Message, (void *) &dl_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_DCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
# endif
#endif

  LOG_I(RRC,"RRCConnectionReconfiguration Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  // for (i=0;i<30;i++)
  //    msg("%x.",buffer[i]);
  // msg("\n");


  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionReestablishment(
  const protocol_ctxt_t*     const ctxt_pP,
  rrc_eNB_ue_context_t*      const ue_context_pP,
  int                              CC_id,
  uint8_t*                   const buffer,
  const uint8_t                    transmission_mode,
  const uint8_t                    Transaction_id,
  SRB_ToAddModList_t               **SRB_configList,
  struct PhysicalConfigDedicated   **physicalConfigDedicated)
{
  asn_enc_rval_t enc_rval;

  long* logicalchannelgroup = NULL;
  struct SRB_ToAddMod* SRB1_config = NULL;
  struct SRB_ToAddMod* SRB2_config = NULL;
  struct SRB_ToAddMod__rlc_Config* SRB1_rlc_config = NULL;
  struct SRB_ToAddMod__logicalChannelConfig* SRB1_lchan_config = NULL;
  struct LogicalChannelConfig__ul_SpecificParameters* SRB1_ul_SpecificParameters = NULL;
  eNB_RRC_INST *rrc               = RC.rrc[ctxt_pP->module_id];

#ifdef CBA
  struct PUSCH_CBAConfigDedicated_vlola* pusch_CBAConfigDedicated_vlola = NULL;
  long* betaOffset_CBA_Index = NULL;
  long* cShift_CBA = NULL;
#endif
  PhysicalConfigDedicated_t* physicalConfigDedicated2 = NULL;

  DL_CCCH_Message_t dl_ccch_msg;

  RRCConnectionReestablishment_t* rrcConnectionReestablishment = NULL;

  int i = 0;
  SRB_ToAddModList_t **SRB_configList2 = NULL;
  SRB_configList2 = &ue_context_pP->ue_context.SRB_configList2[Transaction_id];
  if (*SRB_configList2) {
    free(*SRB_configList2);
  }
  *SRB_configList2 = CALLOC(1, sizeof(SRB_ToAddModList_t));

  memset((void *)&dl_ccch_msg, 0, sizeof(DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishment;
  rrcConnectionReestablishment          = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionReestablishment;

  // RRCConnectionReestablishment
  // Configure SRB1


  // get old configuration of SRB2
  if (*SRB_configList != NULL) {
    for (i = 0; (i < (*SRB_configList)->list.count) && (i < 3); i++) {
      LOG_D(RRC, "(*SRB_configList)->list.array[%d]->srb_Identity=%ld\n",
          i, (*SRB_configList)->list.array[i]->srb_Identity);
      if ((*SRB_configList)->list.array[i]->srb_Identity == 2 ){
        SRB2_config = (*SRB_configList)->list.array[i];
      } else if ((*SRB_configList)->list.array[i]->srb_Identity == 1 ){
        SRB1_config = (*SRB_configList)->list.array[i];
      }
    }
  }

  if (SRB1_config == NULL) {
    // default SRB1 configuration
    LOG_W(RRC,"SRB1 configuration does not exist in SRB configuration list, use default\n");
    /// SRB1
    SRB1_config = CALLOC(1, sizeof(*SRB1_config));

    SRB1_config->srb_Identity = 1;
    SRB1_rlc_config = CALLOC(1, sizeof(*SRB1_rlc_config));
    SRB1_config->rlc_Config   = SRB1_rlc_config;

    SRB1_rlc_config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
    SRB1_rlc_config->choice.explicitValue.present=RLC_Config_PR_am;
  #if defined(ENABLE_ITTI)
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = rrc->srb1_timer_poll_retransmit;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = rrc->srb1_poll_pdu;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = rrc->srb1_poll_byte;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = rrc->srb1_max_retx_threshold;
    SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = rrc->srb1_timer_reordering;
    SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = rrc->srb1_timer_status_prohibit;
  #else
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms20;;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = PollByte_kBinfinity;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t8;
    SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms35;
    SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms0;
  #endif

    SRB1_lchan_config = CALLOC(1, sizeof(*SRB1_lchan_config));
    SRB1_config->logicalChannelConfig = SRB1_lchan_config;

    SRB1_lchan_config->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
    SRB1_ul_SpecificParameters = CALLOC(1, sizeof(*SRB1_ul_SpecificParameters));

    SRB1_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB1_ul_SpecificParameters;
    SRB1_ul_SpecificParameters->priority = 1;

    //assign_enum(&SRB1_ul_SpecificParameters->prioritisedBitRate,LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity);
    SRB1_ul_SpecificParameters->prioritisedBitRate=LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;

    //assign_enum(&SRB1_ul_SpecificParameters->bucketSizeDuration,LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50);
    SRB1_ul_SpecificParameters->bucketSizeDuration=LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

    logicalchannelgroup = CALLOC(1, sizeof(long));
    *logicalchannelgroup = 0;
    SRB1_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;
  }

  if (SRB2_config == NULL) {
    LOG_W(RRC,"SRB2 configuration does not exist in SRB configuration list\n");
  } else {
    ASN_SEQUENCE_ADD(&(*SRB_configList2)->list, SRB2_config);
  }

  if (*SRB_configList) {
    free(*SRB_configList);
  }

  *SRB_configList = CALLOC(1, sizeof(SRB_ToAddModList_t));

  ASN_SEQUENCE_ADD(&(*SRB_configList)->list,SRB1_config);

  physicalConfigDedicated2 = *physicalConfigDedicated;

  rrcConnectionReestablishment->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReestablishment->criticalExtensions.present = RRCConnectionReestablishment__criticalExtensions_PR_c1;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.present = RRCConnectionReestablishment__criticalExtensions__c1_PR_rrcConnectionReestablishment_r8;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.radioResourceConfigDedicated.srb_ToAddModList = *SRB_configList;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.radioResourceConfigDedicated.drb_ToAddModList = NULL;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.radioResourceConfigDedicated.drb_ToReleaseList = NULL;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.radioResourceConfigDedicated.sps_Config = NULL;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.radioResourceConfigDedicated.physicalConfigDedicated = physicalConfigDedicated2;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.radioResourceConfigDedicated.mac_MainConfig = NULL;

  uint8_t KeNB_star[32] = { 0 };
  uint16_t pci = rrc->carrier[CC_id].physCellId;
  uint32_t earfcn_dl = (uint32_t)freq_to_arfcn10(RC.mac[ctxt_pP->module_id]->common_channels[CC_id].eutra_band,
                  rrc->carrier[CC_id].dl_CarrierFreq);
  bool     is_rel8_only = true;
  if (earfcn_dl > 65535) {
    is_rel8_only = false;
  }

  LOG_D(RRC, "pci=%d, eutra_band=%d, downlink_frequency=%d, earfcn_dl=%u, is_rel8_only=%s\n",
      pci,
      RC.mac[ctxt_pP->module_id]->common_channels[CC_id].eutra_band,
      rrc->carrier[CC_id].dl_CarrierFreq,
      earfcn_dl,
      is_rel8_only == true ? "true": "false");
#if defined(ENABLE_SECURITY)
  if (ue_context_pP->ue_context.nh_ncc >= 0) {
    derive_keNB_star(ue_context_pP->ue_context.nh, pci, earfcn_dl, is_rel8_only, KeNB_star);
    rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.nextHopChainingCount = ue_context_pP->ue_context.nh_ncc;
  } else { // first HO 
    derive_keNB_star (ue_context_pP->ue_context.kenb, pci, earfcn_dl, is_rel8_only, KeNB_star);
    // LG: really 1
    rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.nextHopChainingCount = 0;
  }

  // copy KeNB_star to ue_context_pP->ue_context.kenb
  memcpy (ue_context_pP->ue_context.kenb, KeNB_star, 32);
  ue_context_pP->ue_context.kenb_ncc = 0;
#else
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.nextHopChainingCount = 0;
#endif

  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.nonCriticalExtension = NULL;

#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_DL_CCCH_Message, (void*)&dl_ccch_msg);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_CCCH_Message,
                                   (void*)&dl_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_CCCH_Message, (void *) &dl_ccch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_CCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
    }
  }
# endif
#endif

#ifdef USER_MODE
  LOG_D(RRC,"RRCConnectionReestablishment Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);
#endif

  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionReestablishmentReject(
  uint8_t                    Mod_id,
  uint8_t*                   const buffer)
//------------------------------------------------------------------------------
{

  asn_enc_rval_t enc_rval;

  DL_CCCH_Message_t dl_ccch_msg;
  RRCConnectionReestablishmentReject_t *rrcConnectionReestablishmentReject;

  memset((void *)&dl_ccch_msg,0,sizeof(DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentReject;
  rrcConnectionReestablishmentReject    = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionReestablishmentReject;

  // RRCConnectionReestablishmentReject
  rrcConnectionReestablishmentReject->criticalExtensions.present = RRCConnectionReestablishmentReject__criticalExtensions_PR_rrcConnectionReestablishmentReject_r8;

#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_DL_CCCH_Message, (void*)&dl_ccch_msg);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_CCCH_Message,
                                   (void*)&dl_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_CCCH_Message, (void *) &dl_ccch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_CCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"RRCConnectionReestablishmentReject Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);

  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionReject(
  uint8_t                    Mod_id,
  uint8_t*                   const buffer)
//------------------------------------------------------------------------------
{

  asn_enc_rval_t enc_rval;

  DL_CCCH_Message_t dl_ccch_msg;
  RRCConnectionReject_t *rrcConnectionReject;

  memset((void *)&dl_ccch_msg,0,sizeof(DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = DL_CCCH_MessageType__c1_PR_rrcConnectionReject;
  rrcConnectionReject                   = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionReject;

  // RRCConnectionReject
  rrcConnectionReject->criticalExtensions.present = RRCConnectionReject__criticalExtensions_PR_c1;
  rrcConnectionReject->criticalExtensions.choice.c1.present = RRCConnectionReject__criticalExtensions__c1_PR_rrcConnectionReject_r8;
  /* let's put a wait time of 1s for the moment */
  rrcConnectionReject->criticalExtensions.choice.c1.choice.rrcConnectionReject_r8.waitTime = 1;

#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_DL_CCCH_Message, (void*)&dl_ccch_msg);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_CCCH_Message,
                                   (void*)&dl_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %ld)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_CCCH_Message, (void *) &dl_ccch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_CCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_ccch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_ccch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"RRCConnectionReject Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);

  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCConnectionRelease(
  uint8_t                             Mod_id,
  uint8_t                            *buffer,
  uint8_t                             Transaction_id)
{

  asn_enc_rval_t enc_rval;

  DL_DCCH_Message_t dl_dcch_msg;
  RRCConnectionRelease_t *rrcConnectionRelease;


  memset(&dl_dcch_msg,0,sizeof(DL_DCCH_Message_t));

  dl_dcch_msg.message.present           = DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = DL_DCCH_MessageType__c1_PR_rrcConnectionRelease;
  rrcConnectionRelease                  = &dl_dcch_msg.message.choice.c1.choice.rrcConnectionRelease;

  // RRCConnectionRelease
  rrcConnectionRelease->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionRelease->criticalExtensions.present = RRCConnectionRelease__criticalExtensions_PR_c1;
  rrcConnectionRelease->criticalExtensions.choice.c1.present =RRCConnectionRelease__criticalExtensions__c1_PR_rrcConnectionRelease_r8 ;

  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.releaseCause = ReleaseCause_other;
  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.redirectedCarrierInfo = NULL;
  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.idleModeMobilityControlInfo = NULL;

  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.nonCriticalExtension=CALLOC(1,
      sizeof(*rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.nonCriticalExtension));

  enc_rval = uper_encode_to_buffer(&asn_DEF_DL_DCCH_Message,
                                   (void*)&dl_dcch_msg,
                                   buffer,
                                   RRC_BUF_SIZE);

  return((enc_rval.encoded+7)/8);
}

uint8_t TMGI[5] = {4,3,2,1,0};//TMGI is a string of octet, ref. TS 24.008 fig. 10.5.4a


#if defined(Rel10) || defined(Rel14)
uint8_t do_MBSFNAreaConfig(uint8_t Mod_id,
                           uint8_t sync_area,
                           uint8_t *buffer,
                           MCCH_Message_t *mcch_message,
                           MBSFNAreaConfiguration_r9_t **mbsfnAreaConfiguration)
{

  asn_enc_rval_t enc_rval;
  MBSFN_SubframeConfig_t *mbsfn_SubframeConfig1;
  PMCH_Info_r9_t *pmch_Info_1;
  MBMS_SessionInfo_r9_t *mbms_Session_1;
  // MBMS_SessionInfo_r9_t *mbms_Session_2;
  eNB_RRC_INST *rrc               = RC.rrc[Mod_id];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[0];
 

  memset(mcch_message,0,sizeof(MCCH_Message_t));
  mcch_message->message.present = MCCH_MessageType_PR_c1;
  mcch_message->message.choice.c1.present = MCCH_MessageType__c1_PR_mbsfnAreaConfiguration_r9;
  *mbsfnAreaConfiguration = &mcch_message->message.choice.c1.choice.mbsfnAreaConfiguration_r9;

  // Common Subframe Allocation (CommonSF-Alloc-r9)

  mbsfn_SubframeConfig1= CALLOC(1,sizeof(*mbsfn_SubframeConfig1));
  memset((void*)mbsfn_SubframeConfig1,0,sizeof(*mbsfn_SubframeConfig1));
  //
  mbsfn_SubframeConfig1->radioframeAllocationPeriod= MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
  mbsfn_SubframeConfig1->radioframeAllocationOffset= 1;
  mbsfn_SubframeConfig1->subframeAllocation.present= MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
  mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;

  // CURRENTLY WE ARE SUPPORITNG ONLY ONE sf ALLOCATION
  switch (sync_area) {
  case 0:
    if (carrier->sib1->tdd_Config != NULL) {// pattern 001110 for TDD
      mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x08<<2;// shift 2bits cuz 2last bits are unused.
    } else { //111000
      mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x38<<2;
    }

    break;

  case 1:
    if (carrier->sib1->tdd_Config != NULL) {
      mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x08<<2;// shift 2bits cuz 2last bits are unused.
    } else { // 000111
      mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x07<<2;
    }

  default :
    break;
  }

  ASN_SEQUENCE_ADD(&(*mbsfnAreaConfiguration)->commonSF_Alloc_r9.list,mbsfn_SubframeConfig1);

  //  commonSF-AllocPeriod-r9
  (*mbsfnAreaConfiguration)->commonSF_AllocPeriod_r9= MBSFNAreaConfiguration_r9__commonSF_AllocPeriod_r9_rf16;

  // PMCHs Information List (PMCH-InfoList-r9)
  // PMCH_1  Config
  pmch_Info_1 = CALLOC(1,sizeof(PMCH_Info_r9_t));
  memset((void*)pmch_Info_1,0,sizeof(PMCH_Info_r9_t));

  /*
   * take the value of last mbsfn subframe in this CSA period because there is only one PMCH in this mbsfn area
   * Note: this has to be set based on the subframeAllocation and CSA
   */
  pmch_Info_1->pmch_Config_r9.sf_AllocEnd_r9= 3;
  pmch_Info_1->pmch_Config_r9.dataMCS_r9= 7;
  pmch_Info_1->pmch_Config_r9.mch_SchedulingPeriod_r9= PMCH_Config_r9__mch_SchedulingPeriod_r9_rf16;

  // MBMSs-SessionInfoList-r9
  //  pmch_Info_1->mbms_SessionInfoList_r9 = CALLOC(1,sizeof(struct MBMS_SessionInfoList_r9));
  //  Session 1
  mbms_Session_1 = CALLOC(1,sizeof(MBMS_SessionInfo_r9_t));
  memset(mbms_Session_1,0,sizeof(MBMS_SessionInfo_r9_t));
  // TMGI value
  mbms_Session_1->tmgi_r9.plmn_Id_r9.present= TMGI_r9__plmn_Id_r9_PR_plmn_Index_r9;
  mbms_Session_1->tmgi_r9.plmn_Id_r9.choice.plmn_Index_r9= 1;
  // Service ID
  memset(&mbms_Session_1->tmgi_r9.serviceId_r9,0,sizeof(OCTET_STRING_t));// need to check
  OCTET_STRING_fromBuf(&mbms_Session_1->tmgi_r9.serviceId_r9,(const char*)&TMGI[2],3);
  // Session ID is still missing here, it can be used as an rab id or mrb id
  mbms_Session_1->sessionId_r9 = CALLOC(1,sizeof(OCTET_STRING_t));
  mbms_Session_1->sessionId_r9->buf= MALLOC(1);
  mbms_Session_1->sessionId_r9->size= 1;
  mbms_Session_1->sessionId_r9->buf[0]= MTCH;
  // Logical Channel ID
  mbms_Session_1->logicalChannelIdentity_r9= MTCH;
  ASN_SEQUENCE_ADD(&pmch_Info_1->mbms_SessionInfoList_r9.list,mbms_Session_1);

  /*    //  Session 2
  //mbms_Session_2 = CALLOC(1,sizeof(MBMS_SessionInfo_r9_t));
  memset(mbms_Session_2,0,sizeof(MBMS_SessionInfo_r9_t));
  // TMGI value
  mbms_Session_2->tmgi_r9.plmn_Id_r9.present= TMGI_r9__plmn_Id_r9_PR_plmn_Index_r9;
  mbms_Session_2->tmgi_r9.plmn_Id_r9.choice.plmn_Index_r9= 1;
  // Service ID
  memset(&mbms_Session_2->tmgi_r9.serviceId_r9,0,sizeof(OCTET_STRING_t));// need to check
  OCTET_STRING_fromBuf(&mbms_Session_2->tmgi_r9.serviceId_r9,(const char*)&TMGI[3],3);
  // Session ID is still missing here
  mbms_Session_2->sessionID_r9->buf= MALLOC(1);
  mbms_Session_2->sessionID_r9->size= 1;
  mbms_Session_2->sessionID_r9->buf[0]= 0x11;
  // Logical Channel ID
  mbms_Session_2->logicalChannelIdentity_r9= 2;
  ASN_SEQUENCE_ADD(&pmch_Info_1->mbms_SessionInfoList_r9.list,mbms_Session_2);
  */
  ASN_SEQUENCE_ADD(&(*mbsfnAreaConfiguration)->pmch_InfoList_r9.list,pmch_Info_1);

#ifdef XER_PRINT
  xer_fprint(stdout,&asn_DEF_MCCH_Message,(void*)mcch_message);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_MCCH_Message,
                                   (void*)mcch_message,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_MCCH_Message, (void *) &mcch_message)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_MCCH, message_string_size);
      msg_p->ittiMsg.rrc_dl_mcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_mcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, Mod_id, msg_p);
    }
  }
# endif
#endif

  LOG_D(RRC,"[eNB] MCCH Message Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : MCCH  encoding failed for MBSFNAreaConfiguration\n");
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}
#endif

uint8_t do_MeasurementReport(uint8_t Mod_id, uint8_t *buffer,int measid,int phy_id,long rsrp_s,long rsrq_s,long rsrp_t,long rsrq_t)
{

  asn_enc_rval_t enc_rval;

  UL_DCCH_Message_t ul_dcch_msg;

  MeasurementReport_t  *measurementReport;

  ul_dcch_msg.message.present                     = UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present           = UL_DCCH_MessageType__c1_PR_measurementReport;
  measurementReport            = &ul_dcch_msg.message.choice.c1.choice.measurementReport;

  measurementReport->criticalExtensions.present=MeasurementReport__criticalExtensions_PR_c1;
  measurementReport->criticalExtensions.choice.c1.present=MeasurementReport__criticalExtensions__c1_PR_measurementReport_r8;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.nonCriticalExtension=CALLOC(1,
      sizeof(*measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.nonCriticalExtension));

  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measId=measid;
#if defined(Rel10) || defined(Rel14)
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultPCell.rsrpResult=rsrp_s;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultPCell.rsrqResult=rsrq_s;
#else
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultServCell.rsrpResult=rsrp_s;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultServCell.rsrqResult=rsrq_s;
#endif
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells=CALLOC(1,
      sizeof(*measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells));
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells->present=MeasResults__measResultNeighCells_PR_measResultListEUTRA;

  MeasResultListEUTRA_t  *measResultListEUTRA2;
  measResultListEUTRA2 = CALLOC(1,sizeof(*measResultListEUTRA2));

  struct MeasResultEUTRA *measresulteutra2;
  measresulteutra2 = CALLOC(1,sizeof(*measresulteutra2));
  measresulteutra2->physCellId=phy_id;//1;

  struct MeasResultEUTRA__cgi_Info *measresult_cgi2;
  measresult_cgi2 = CALLOC(1,sizeof(*measresult_cgi2));

  memset(&measresult_cgi2->cellGlobalId.plmn_Identity,0,sizeof(measresult_cgi2->cellGlobalId.plmn_Identity));

  // measresult_cgi2->cellGlobalId.plmn_Identity.mcc=CALLOC(1,sizeof(measresult_cgi2->cellGlobalId.plmn_Identity.mcc));
  measresult_cgi2->cellGlobalId.plmn_Identity.mcc = CALLOC(1, sizeof(*measresult_cgi2->cellGlobalId.plmn_Identity.mcc));

  asn_set_empty(&measresult_cgi2->cellGlobalId.plmn_Identity.mcc->list);//.size=0;

  MCC_MNC_Digit_t dummy;
  dummy=2;
  ASN_SEQUENCE_ADD(&measresult_cgi2->cellGlobalId.plmn_Identity.mcc->list,&dummy);
  dummy=6;
  ASN_SEQUENCE_ADD(&measresult_cgi2->cellGlobalId.plmn_Identity.mcc->list,&dummy);
  dummy=2;
  ASN_SEQUENCE_ADD(&measresult_cgi2->cellGlobalId.plmn_Identity.mcc->list,&dummy);

  measresult_cgi2->cellGlobalId.plmn_Identity.mnc.list.size=0;
  measresult_cgi2->cellGlobalId.plmn_Identity.mnc.list.count=0;
  dummy=8;
  ASN_SEQUENCE_ADD(&measresult_cgi2->cellGlobalId.plmn_Identity.mnc.list,&dummy);
  dummy=0;
  ASN_SEQUENCE_ADD(&measresult_cgi2->cellGlobalId.plmn_Identity.mnc.list,&dummy);

  measresult_cgi2->cellGlobalId.cellIdentity.buf=MALLOC(8);
  measresult_cgi2->cellGlobalId.cellIdentity.buf[0]=0x01;
  measresult_cgi2->cellGlobalId.cellIdentity.buf[1]=0x48;
  measresult_cgi2->cellGlobalId.cellIdentity.buf[2]=0x0f;
  measresult_cgi2->cellGlobalId.cellIdentity.buf[3]=0x03;
  measresult_cgi2->cellGlobalId.cellIdentity.size=4;
  measresult_cgi2->cellGlobalId.cellIdentity.bits_unused=4;

  measresult_cgi2->trackingAreaCode.buf = MALLOC(2);
  measresult_cgi2->trackingAreaCode.buf[0]=0x00;
  measresult_cgi2->trackingAreaCode.buf[1]=0x10;
  measresult_cgi2->trackingAreaCode.size=2;
  measresult_cgi2->trackingAreaCode.bits_unused=0;


  measresulteutra2->cgi_Info=measresult_cgi2;

  struct MeasResultEUTRA__measResult meas2;
  //    int rsrp_va=10;

  meas2.rsrpResult=&(rsrp_t);
  //&rsrp_va;
  meas2.rsrqResult=&(rsrq_t);

  measresulteutra2->measResult=meas2;

  ASN_SEQUENCE_ADD(&measResultListEUTRA2->list,measresulteutra2);

  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells->choice.measResultListEUTRA=*(measResultListEUTRA2);

  enc_rval = uper_encode_to_buffer(&asn_DEF_UL_DCCH_Message,
                                   (void*)&ul_dcch_msg,
                                   buffer,
                                   100);



  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[20000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_UL_DCCH_Message, (void *) &ul_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_UE, RRC_DL_DCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, NB_eNB_INST + Mod_id, msg_p);
    }
  }
# endif
#endif

  printf("Measurement Report Encoded %zu bits (%zu bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  return((enc_rval.encoded+7)/8);
}

uint8_t do_DLInformationTransfer(uint8_t Mod_id, uint8_t **buffer, uint8_t transaction_id, uint32_t pdu_length, uint8_t *pdu_buffer)
{
  ssize_t encoded;

  DL_DCCH_Message_t dl_dcch_msg;

  memset(&dl_dcch_msg, 0, sizeof(DL_DCCH_Message_t));

  dl_dcch_msg.message.present           = DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = DL_DCCH_MessageType__c1_PR_dlInformationTransfer;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.rrc_TransactionIdentifier = transaction_id;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.present = DLInformationTransfer__criticalExtensions_PR_c1;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.present = DLInformationTransfer__criticalExtensions__c1_PR_dlInformationTransfer_r8;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.present =
    DLInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.size = pdu_length;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.buf = pdu_buffer;

  encoded = uper_encode_to_new_buffer (&asn_DEF_DL_DCCH_Message, NULL, (void*) &dl_dcch_msg, (void **) buffer);

  /*
#if defined(ENABLE_ITTI)
# if !defined(DISABLE_XER_SPRINT)
  {
    char        message_string[10000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_DL_DCCH_Message, (void *)&dl_dcch_msg)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_ENB, RRC_DL_DCCH, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_dl_dcch.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_dl_dcch.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, Mod_id, msg_p);
    }
  }
# endif
#endif
  */

  return encoded;
}

uint8_t do_Paging(uint8_t Mod_id, uint8_t *buffer, ue_paging_identity_t ue_paging_identity, cn_domain_t cn_domain)
{
  LOG_D(RRC, "[eNB %d] do_Paging start\n", Mod_id);
  asn_enc_rval_t enc_rval;

  PCCH_Message_t pcch_msg;
  PagingRecord_t *paging_record_p;
  int j;

  pcch_msg.message.present           = PCCH_MessageType_PR_c1;
  pcch_msg.message.choice.c1.present = PCCH_MessageType__c1_PR_paging;

  pcch_msg.message.choice.c1.choice.paging.pagingRecordList = CALLOC(1,sizeof(*pcch_msg.message.choice.c1.choice.paging.pagingRecordList));

  pcch_msg.message.choice.c1.choice.paging.systemInfoModification = NULL;
  pcch_msg.message.choice.c1.choice.paging.etws_Indication = NULL;
  pcch_msg.message.choice.c1.choice.paging.nonCriticalExtension = NULL;

  asn_set_empty(&pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list);
  pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list.count = 0;

  if ((paging_record_p = calloc(1, sizeof(PagingRecord_t))) == NULL) {
    /* Possible error on calloc */
    return (-1);
  }

  memset(paging_record_p, 0, sizeof(PagingRecord_t));

  /* convert ue_paging_identity_t to PagingUE_Identity_t */
  if (ue_paging_identity.presenceMask == UE_PAGING_IDENTITY_s_tmsi) {
    paging_record_p->ue_Identity.present = PagingUE_Identity_PR_s_TMSI;
    MME_CODE_TO_OCTET_STRING(ue_paging_identity.choice.s_tmsi.mme_code,
                             &paging_record_p->ue_Identity.choice.s_TMSI.mmec);
    paging_record_p->ue_Identity.choice.s_TMSI.mmec.bits_unused = 0;
    M_TMSI_TO_OCTET_STRING(ue_paging_identity.choice.s_tmsi.m_tmsi,
                             &paging_record_p->ue_Identity.choice.s_TMSI.m_TMSI);
    paging_record_p->ue_Identity.choice.s_TMSI.m_TMSI.bits_unused = 0;
  } else if (ue_paging_identity.presenceMask == UE_PAGING_IDENTITY_imsi) {
    IMSI_Digit_t imsi_digit[21];
    for (j = 0; j< ue_paging_identity.choice.imsi.length; j++) {  /* IMSI size */
      imsi_digit[j] = (IMSI_Digit_t)ue_paging_identity.choice.imsi.buffer[j];
      ASN_SEQUENCE_ADD(&paging_record_p->ue_Identity.choice.imsi.list, &imsi_digit[j]);
    }
  }

  /* set cn_domain */
  if (cn_domain == CN_DOMAIN_PS) {
    paging_record_p->cn_Domain = PagingRecord__cn_Domain_ps;
  } else {
    paging_record_p->cn_Domain = PagingRecord__cn_Domain_cs;
  }
  /* add to list */
  ASN_SEQUENCE_ADD(&pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list, paging_record_p);
  LOG_D(RRC, "[eNB %d] do_Paging paging_record: cn_Domain %ld, ue_paging_identity.presenceMask %d, PagingRecordList.count %d\n",
          Mod_id, paging_record_p->cn_Domain, ue_paging_identity.presenceMask, pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list.count);

  enc_rval = uper_encode_to_buffer(&asn_DEF_PCCH_Message, (void*)&pcch_msg, buffer, RRC_BUF_SIZE);

  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
#ifdef XER_PRINT
  xer_fprint(stdout, &asn_DEF_PCCH_Message, (void*)&pcch_msg);
#endif

  return((enc_rval.encoded+7)/8);
}

uint8_t do_ULInformationTransfer(uint8_t **buffer, uint32_t pdu_length, uint8_t *pdu_buffer)
{
  ssize_t encoded;

  UL_DCCH_Message_t ul_dcch_msg;

  memset(&ul_dcch_msg, 0, sizeof(UL_DCCH_Message_t));

  ul_dcch_msg.message.present           = UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = UL_DCCH_MessageType__c1_PR_ulInformationTransfer;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.present = ULInformationTransfer__criticalExtensions_PR_c1;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.present = DLInformationTransfer__criticalExtensions__c1_PR_dlInformationTransfer_r8;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.present =
    ULInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.size = pdu_length;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.buf = pdu_buffer;

  encoded = uper_encode_to_new_buffer (&asn_DEF_UL_DCCH_Message, NULL, (void*) &ul_dcch_msg, (void **) buffer);

  return encoded;
}

OAI_UECapability_t *fill_ue_capability(char *UE_EUTRA_Capability_xer_fname)
{
  static OAI_UECapability_t UECapability; /* TODO declared static to allow returning this has an address should be allocated in a cleaner way. */
  static SupportedBandEUTRA_t Bandlist[4]; // the macro ASN_SEQUENCE_ADD() does not copy the source, but only stores a reference to it
  static InterFreqBandInfo_t InterFreqBandInfo[4][4]; // the macro ASN_SEQUENCE_ADD() does not copy the source, but only stores a reference to it
  static BandInfoEUTRA_t BandInfoEUTRA[4]; // the macro ASN_SEQUENCE_ADD() does not copy the source, but only stores a reference to it

  asn_enc_rval_t enc_rval;
  asn_dec_rval_t dec_rval;

  long maxNumberROHC_ContextSessions = PDCP_Parameters__maxNumberROHC_ContextSessions_cs16;
  int i;

  UE_EUTRA_Capability_t *UE_EUTRA_Capability;
  char UE_EUTRA_Capability_xer[8192];
  size_t size;

  LOG_I(RRC,"Allocating %zu bytes for UE_EUTRA_Capability\n",sizeof(*UE_EUTRA_Capability));

  UE_EUTRA_Capability = CALLOC(1, sizeof(*UE_EUTRA_Capability));

  assert(UE_EUTRA_Capability);

  if (!UE_EUTRA_Capability_xer_fname)  {
    Bandlist[0].bandEUTRA  = 3;  // UL 1710-1785, DL 1805-1880 FDD
    Bandlist[0].halfDuplex = 0;
    Bandlist[1].bandEUTRA  = 20;  // UL 824-849 , DL 869-894 FDD
    Bandlist[1].halfDuplex = 0;
    Bandlist[2].bandEUTRA  = 7;   // UL 2500-2570, DL 2620-2690 FDD
    Bandlist[2].halfDuplex = 0;
    Bandlist[3].bandEUTRA  = 38;  // UL/DL 2570-2620, TDD
    Bandlist[3].halfDuplex = 0;

    memset((void*)InterFreqBandInfo, 0, sizeof(InterFreqBandInfo));
    memset((void*)BandInfoEUTRA, 0, sizeof(BandInfoEUTRA));

    InterFreqBandInfo[0][0].interFreqNeedForGaps = 0;
    InterFreqBandInfo[0][1].interFreqNeedForGaps = 1;
    InterFreqBandInfo[0][2].interFreqNeedForGaps = 1;
    InterFreqBandInfo[0][3].interFreqNeedForGaps = 1;
    InterFreqBandInfo[1][0].interFreqNeedForGaps = 1;
    InterFreqBandInfo[1][1].interFreqNeedForGaps = 0;
    InterFreqBandInfo[1][2].interFreqNeedForGaps = 1;
    InterFreqBandInfo[1][3].interFreqNeedForGaps = 1;
    InterFreqBandInfo[2][0].interFreqNeedForGaps = 1;
    InterFreqBandInfo[2][1].interFreqNeedForGaps = 1;
    InterFreqBandInfo[2][2].interFreqNeedForGaps = 0;
    InterFreqBandInfo[2][3].interFreqNeedForGaps = 1;
    InterFreqBandInfo[3][0].interFreqNeedForGaps = 1;
    InterFreqBandInfo[3][1].interFreqNeedForGaps = 1;
    InterFreqBandInfo[3][2].interFreqNeedForGaps = 1;
    InterFreqBandInfo[3][3].interFreqNeedForGaps = 0;


    UE_EUTRA_Capability->accessStratumRelease = 0;//AccessStratumRelease_rel8;
    UE_EUTRA_Capability->ue_Category          = 4;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0001=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0002=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0003=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0004=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0006=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0101=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0102=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0103=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0104=0;

    UE_EUTRA_Capability->pdcp_Parameters.maxNumberROHC_ContextSessions = &maxNumberROHC_ContextSessions;

    UE_EUTRA_Capability->phyLayerParameters.ue_TxAntennaSelectionSupported = 0;
    UE_EUTRA_Capability->phyLayerParameters.ue_SpecificRefSigsSupported    = 0;
    UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.count                          = 0;
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void*)&Bandlist[0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void*)&Bandlist[1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void*)&Bandlist[2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void*)&Bandlist[3]);

    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void*)&BandInfoEUTRA[0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void*)&BandInfoEUTRA[1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void*)&BandInfoEUTRA[2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void*)&BandInfoEUTRA[3]);

    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void*)&InterFreqBandInfo[0][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void*)&InterFreqBandInfo[0][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void*)&InterFreqBandInfo[0][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void*)&InterFreqBandInfo[0][3]);

    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void*)&InterFreqBandInfo[1][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void*)&InterFreqBandInfo[1][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void*)&InterFreqBandInfo[1][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void*)&InterFreqBandInfo[1][3]);

    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void*)&InterFreqBandInfo[2][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void*)&InterFreqBandInfo[2][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void*)&InterFreqBandInfo[2][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void*)&InterFreqBandInfo[2][3]);

    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void*)&InterFreqBandInfo[3][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void*)&InterFreqBandInfo[3][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void*)&InterFreqBandInfo[3][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void*)&InterFreqBandInfo[3][3]);

    // UE_EUTRA_Capability->measParameters.bandListEUTRA.list.count                         = 0;  // no measurements on other bands
    // UE_EUTRA_Capability->featureGroupIndicators  // null

    if(usim_test == 1)
    {
      // featureGroup is mandatory for CMW tests
      // featureGroup is filled only for usim-test mode
      BIT_STRING_t *bit_string = CALLOC(1, sizeof(*bit_string));
      char featrG[4]           = { 0x00, 0x08, 0x00, 0x04 };
      bit_string->buf          = CALLOC(1, 4);
      memcpy(bit_string->buf, featrG, 4);
      bit_string->size         = 4;
      bit_string->bits_unused  = 0;
      UE_EUTRA_Capability->featureGroupIndicators = bit_string;
    }

    // UE_EUTRA_Capability->interRAT_Parameters     // null
  } else {

    FILE* f = fopen(UE_EUTRA_Capability_xer_fname, "r");
    assert(f);
    size = fread(UE_EUTRA_Capability_xer, 1, sizeof UE_EUTRA_Capability_xer, f);
    fclose(f);

    if (size == 0 || size == sizeof UE_EUTRA_Capability_xer) {
      LOG_E(RRC,"UE Capabilities XER file %s is too large\n", UE_EUTRA_Capability_xer_fname);
      free( UE_EUTRA_Capability);
      return(NULL);
    }

    dec_rval = xer_decode(0, &asn_DEF_UE_EUTRA_Capability, (void*)UE_EUTRA_Capability, UE_EUTRA_Capability_xer, size);
    assert(dec_rval.code == RC_OK);
  }

  UECapability.UE_EUTRA_Capability = UE_EUTRA_Capability;
#ifdef XER_PRINT
  xer_fprint(stdout,&asn_DEF_UE_EUTRA_Capability,(void *)UE_EUTRA_Capability);
#endif
  enc_rval = uper_encode_to_buffer(&asn_DEF_UE_EUTRA_Capability,
                                   (void*)UE_EUTRA_Capability,
                                   &UECapability.sdu[0],
                                   MAX_UE_CAPABILITY_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

#if defined(ENABLE_ITTI)
# if defined(DISABLE_XER_SPRINT)
  {
    MessageDef *msg_p;

    msg_p = itti_alloc_new_message (TASK_RRC_UE, RRC_UE_EUTRA_CAPABILITY);
    memcpy (&msg_p->ittiMsg, (void *) UE_EUTRA_Capability, sizeof(RrcUeEutraCapability));

    itti_send_msg_to_task (TASK_UNKNOWN, NB_eNB_INST, msg_p);
  }
# else
  {
    char        message_string[10000];
    size_t      message_string_size;

    if ((message_string_size = xer_sprint(message_string, sizeof(message_string), &asn_DEF_UE_EUTRA_Capability, (void *)UE_EUTRA_Capability)) > 0) {
      MessageDef *msg_p;

      msg_p = itti_alloc_new_message_sized (TASK_RRC_UE, RRC_UE_EUTRA_CAPABILITY, message_string_size + sizeof (IttiMsgText));
      msg_p->ittiMsg.rrc_ue_eutra_capability.size = message_string_size;
      memcpy(&msg_p->ittiMsg.rrc_ue_eutra_capability.text, message_string, message_string_size);

      itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, msg_p);
    }
  }
# endif
#endif

  UECapability.sdu_size = (enc_rval.encoded + 7) / 8;
  LOG_I(PHY, "[RRC]UE Capability encoded, %d bytes (%zd bits)\n",
        UECapability.sdu_size, enc_rval.encoded + 7);
  {
    char *sdu;
    sdu = malloc (3 * UECapability.sdu_size + 1 /* For '\0' */);

    for (i = 0; i < UECapability.sdu_size; i++) {
      sprintf (&sdu[3 * i], "%02x.", UECapability.sdu[i]);
    }

    LOG_D(PHY, "[RRC]UE Capability encoded, %s\n", sdu);
    free(sdu);
  }

  return(&UECapability);
}

