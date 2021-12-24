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

/*! \file asn1_msg.c
 * \brief added primitives to build the asn1 messages for FeMBMS 
 * \author Javier Morgade
 * \date 2019-2020
 * \version 1.0
 * \email: javier.morgade@ieee.org
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
#include "targets/RT/USER/lte-softmodem.h"
#include "assertions.h"
#include "LTE_RRCConnectionRequest.h"
#include "LTE_UL-CCCH-Message.h"
#include "LTE_UL-DCCH-Message.h"
#include "LTE_DL-CCCH-Message.h"
#include "LTE_DL-DCCH-Message.h"
#include "LTE_PCCH-Message.h"
#include "openair3/UTILS/conversions.h"
#include "LTE_EstablishmentCause.h"
#include "LTE_RRCConnectionSetup.h"
#include "LTE_SRB-ToAddModList.h"
#include "LTE_DRB-ToAddModList.h"
#include "LTE_HandoverPreparationInformation.h"
#include "LTE_HandoverCommand.h"
#include "LTE_MCCH-Message.h"


#include "RRC/LTE/rrc_defs.h"
#include "RRC/LTE/rrc_extern.h"
#include "LTE_RRCConnectionSetupComplete.h"
#include "LTE_RRCConnectionReconfigurationComplete.h"
#include "LTE_RRCConnectionReconfiguration.h"
#include "LTE_MasterInformationBlock.h"
#include "LTE_SystemInformation.h"


#include "LTE_SystemInformationBlockType1.h"
#include "LTE_SystemInformationBlockType1-BR-r13.h"


#include "LTE_SIB-Type.h"

#include "LTE_BCCH-DL-SCH-Message.h"
#include "LTE_SBCCH-SL-BCH-MessageType.h"
#include "LTE_SBCCH-SL-BCH-Message.h"

#include "LTE_BCCH-BCH-Message-MBMS.h"
#include "LTE_BCCH-DL-SCH-Message-MBMS.h"
#include "LTE_SystemInformationBlockType1-MBMS-r14.h"
#include "LTE_NonMBSFN-SubframeConfig-r14.h"


//#include "PHY/defs.h"

#include "LTE_MeasObjectToAddModList.h"
#include "LTE_ReportConfigToAddModList.h"
#include "LTE_MeasIdToAddModList.h"
#include "enb_config.h"


#include "intertask_interface.h"
#include "NR_FreqBandList.h"


#include "common/ran_context.h"
#include "secu_defs.h"

#if !defined (msg)
  #define msg printf
#endif


typedef struct xer_sprint_string_s {
  char *string;
  size_t string_size;
  size_t string_index;
} xer_sprint_string_t;

extern unsigned char NB_eNB_INST;


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
static int xer__print2s (const void *buffer, size_t size, void *app_key) {
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

int xer_sprint (char *string, size_t string_size, asn_TYPE_descriptor_t *td, void *sptr) {
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

uint16_t get_adjacent_cell_id(uint8_t Mod_id,uint8_t index) {
  return(two_tier_hexagonal_adjacent_cellIds[Mod_id][index]);
}
/* This only works for the hexagonal topology...need a more general function for other topologies */

uint8_t get_adjacent_cell_mod_id(uint16_t phyCellId) {
  uint8_t i;

  for(i=0; i<7; i++) {
    if(two_tier_hexagonal_cellIds[i] == phyCellId) {
      return i;
    }
  }

  LOG_E(RRC,"\nCannot get adjacent cell mod id! Fatal error!\n");
  return 0xFF; //error!
}


uint8_t do_MIB_FeMBMS(rrc_eNB_carrier_data_t *carrier, uint32_t N_RB_DL, uint32_t additionalNonMBSFN, uint32_t frame) {
  asn_enc_rval_t enc_rval;
  LTE_BCCH_BCH_Message_MBMS_t *mib_fembms=&carrier->mib_fembms;
  frame=198;
  uint8_t sfn = (uint8_t)((frame>>4)&0xff);
  uint16_t *spare = calloc(1,sizeof(uint16_t));

  if( spare == NULL  ) abort();

  switch (N_RB_DL) {
    case 6:
      mib_fembms->message.dl_Bandwidth_MBMS_r14 = LTE_MasterInformationBlock_MBMS_r14__dl_Bandwidth_MBMS_r14_n6;
      break;

    case 15:
      mib_fembms->message.dl_Bandwidth_MBMS_r14 = LTE_MasterInformationBlock_MBMS_r14__dl_Bandwidth_MBMS_r14_n15;
      break;

    case 25:
      mib_fembms->message.dl_Bandwidth_MBMS_r14 = LTE_MasterInformationBlock_MBMS_r14__dl_Bandwidth_MBMS_r14_n25;
      break;

    case 50:
      mib_fembms->message.dl_Bandwidth_MBMS_r14 = LTE_MasterInformationBlock_MBMS_r14__dl_Bandwidth_MBMS_r14_n50;
      break;

    case 75:
      mib_fembms->message.dl_Bandwidth_MBMS_r14 = LTE_MasterInformationBlock_MBMS_r14__dl_Bandwidth_MBMS_r14_n75;
      break;

    case 100:
      mib_fembms->message.dl_Bandwidth_MBMS_r14 = LTE_MasterInformationBlock_MBMS_r14__dl_Bandwidth_MBMS_r14_n100;
      break;

    default:
      AssertFatal(1==0,"Unknown dl_Bandwidth %u\n",N_RB_DL);
  }

  LOG_I(RRC,"[MIB] systemBandwidth %x, additional non MBMS subframes %x, sfn %x\n",
        (uint32_t)mib_fembms->message.dl_Bandwidth_MBMS_r14,
        (uint32_t)additionalNonMBSFN,
        (uint32_t)sfn);
  sfn = sfn<<2;
  mib_fembms->message.systemFrameNumber_r14.buf = &sfn;
  mib_fembms->message.systemFrameNumber_r14.size = 1;
  mib_fembms->message.systemFrameNumber_r14.bits_unused=2;
  mib_fembms->message.spare.buf = (uint8_t *)spare;
  mib_fembms->message.spare.size = 2;
  mib_fembms->message.spare.bits_unused = 3;  // This makes a spare of 10 bits
  //TODO additionalNonBMSFNSubframes-r14  INTEGER (0..3) ?
  //if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_BCH_Message_MBMS, (void *)mib_fembms);
  //}
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_BCH_Message_MBMS,
                                   NULL,
                                   (void *)mib_fembms,
                                   carrier->MIB_FeMBMS,
                                   24);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}



uint8_t do_MIB(rrc_eNB_carrier_data_t *carrier, uint32_t N_RB_DL, uint32_t phich_Resource,
               uint32_t phich_duration, uint32_t frame, uint32_t schedulingInfoSIB1
              ) {
  asn_enc_rval_t enc_rval;
  LTE_BCCH_BCH_Message_t *mib=&carrier->mib ;
  uint8_t sfn = (uint8_t)((frame>>2)&0xff);
  uint16_t *spare = calloc(1, sizeof(uint16_t));

  if (spare == NULL) abort();

  switch (N_RB_DL) {
    case 6:
      mib->message.dl_Bandwidth = LTE_MasterInformationBlock__dl_Bandwidth_n6;
      break;

    case 15:
      mib->message.dl_Bandwidth = LTE_MasterInformationBlock__dl_Bandwidth_n15;
      break;

    case 25:
      mib->message.dl_Bandwidth = LTE_MasterInformationBlock__dl_Bandwidth_n25;
      break;

    case 50:
      mib->message.dl_Bandwidth = LTE_MasterInformationBlock__dl_Bandwidth_n50;
      break;

    case 75:
      mib->message.dl_Bandwidth = LTE_MasterInformationBlock__dl_Bandwidth_n75;
      break;

    case 100:
      mib->message.dl_Bandwidth = LTE_MasterInformationBlock__dl_Bandwidth_n100;
      break;

    default:
      AssertFatal(1==0,"Unknown dl_Bandwidth %u\n",N_RB_DL);
  }

  AssertFatal(phich_Resource <= LTE_PHICH_Config__phich_Resource_two,"Illegal phich_Resource\n");
  mib->message.phich_Config.phich_Resource = phich_Resource;
  AssertFatal(phich_duration <= LTE_PHICH_Config__phich_Duration_extended,"Illegal phich_Duration\n");
  mib->message.phich_Config.phich_Duration = phich_duration;
  LOG_I(RRC,"[MIB] systemBandwidth %x, phich_duration %x, phich_resource %x, sfn %x\n",
        (uint32_t)mib->message.dl_Bandwidth,
        (uint32_t)phich_duration,
        (uint32_t)phich_Resource,
        (uint32_t)sfn);
  mib->message.systemFrameNumber.buf = &sfn;
  mib->message.systemFrameNumber.size = 1;
  mib->message.systemFrameNumber.bits_unused=0;
  mib->message.spare.buf = (uint8_t *)spare;
  mib->message.spare.size = 1;
  mib->message.spare.bits_unused = 3;  // This makes a spare of 5 bits
  mib->message.schedulingInfoSIB1_BR_r13 = schedulingInfoSIB1; // turn on/off eMTC
  LOG_I(RRC,"[MIB] schedulingInfoSIB1 %d\n",
        (uint32_t)mib->message.schedulingInfoSIB1_BR_r13);
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_BCH_Message,
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

//TTN for D2D
// 3GPP 36.331 (Section 5.10.7.4)
uint8_t do_MIB_SL(const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, uint32_t frame, uint8_t subframe, uint8_t in_coverage, uint8_t mode) {
  asn_enc_rval_t enc_rval;
  LTE_SBCCH_SL_BCH_MessageType_t *mib_sl = &UE_rrc_inst[ctxt_pP->module_id].mib_sl[eNB_index];
  uint8_t sfn = (uint8_t)((frame>>2)&0xff);
  UE_rrc_inst[ctxt_pP->module_id].MIB = (uint8_t *) malloc16(4);

  if (in_coverage > 0 ) {
    //in coverage
    mib_sl->inCoverage_r12 = TRUE;
    mib_sl->sl_Bandwidth_r12 = *UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->freqInfo.ul_Bandwidth;

    if (UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index]->tdd_Config) {
      mib_sl->tdd_ConfigSL_r12.subframeAssignmentSL_r12 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index]->tdd_Config->subframeAssignment;
    } else {
      mib_sl->tdd_ConfigSL_r12.subframeAssignmentSL_r12 = LTE_TDD_ConfigSL_r12__subframeAssignmentSL_r12_none;
    }

    //if triggered by sl communication
    if (UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index]->commConfig_r12->commSyncConfig_r12->list.array[0]->txParameters_r12->syncInfoReserved_r12) {
      mib_sl->reserved_r12 = *UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index]->commConfig_r12->commSyncConfig_r12->list.array[0]->txParameters_r12->syncInfoReserved_r12;
    }

    //if triggered by sl discovery
    if (UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index]->discConfig_r12->discSyncConfig_r12->list.array[0]->txParameters_r12->syncInfoReserved_r12) {
      mib_sl->reserved_r12 = *UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index]->discConfig_r12->discSyncConfig_r12->list.array[0]->txParameters_r12->syncInfoReserved_r12;
    }

    //Todo - if triggered by v2x
  } else {
    //Todo - out of coverage for V2X
    // Todo - UE has a selected SyncRef UE
    mib_sl->inCoverage_r12 = FALSE;
    //set sl-Bandwidth, subframeAssignmentSL and reserved from the pre-configured parameters
  }

  //set FrameNumber, subFrameNumber
  mib_sl->directFrameNumber_r12.buf =  &sfn;
  mib_sl->directFrameNumber_r12.size = 1;
  mib_sl->directFrameNumber_r12.bits_unused=0;
  mib_sl->directSubframeNumber_r12 = subframe;
  LOG_I(RRC,"[MIB-SL] sfn %x, subframe %x\n", (uint32_t)sfn, (uint8_t)subframe);
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_SBCCH_SL_BCH_Message,
                                   NULL,
                                   (void *)mib_sl,
                                   UE_rrc_inst[ctxt_pP->module_id].MIB,
                                   24);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_SIB1_MBMS(rrc_eNB_carrier_data_t *carrier,
                     int Mod_id,int CC_id,
                     RrcConfigurationReq *configuration
                    ) {
  //  SystemInformation_t systemInformation;
  int num_plmn = configuration->num_plmn;

  //LTE_PLMN_IdentityInfo_t *PLMN_identity_info;
  LTE_PLMN_IdentityInfo_t * PLMN_identity_info = &carrier->PLMN_identity_info_MBMS[0];
  //LTE_MCC_MNC_Digit_t dummy_mcc[num_plmn][3], dummy_mnc[num_plmn][3];
  //LTE_MCC_MNC_Digit_t *dummy_mcc = &carrier->dummy_mcc_MBMS[0][0];
  //LTE_MCC_MNC_Digit_t *dummy_mnc = &carrier->dummy_mnc_MBMS[0][0];
  LTE_MCC_MNC_Digit_t *dummy_mcc_0;
  LTE_MCC_MNC_Digit_t *dummy_mcc_1;
  LTE_MCC_MNC_Digit_t *dummy_mcc_2;
  LTE_MCC_MNC_Digit_t *dummy_mnc_0;
  LTE_MCC_MNC_Digit_t *dummy_mnc_1;
  LTE_MCC_MNC_Digit_t *dummy_mnc_2;
  asn_enc_rval_t enc_rval;
  //LTE_SchedulingInfo_MBMS_r14_t *schedulingInfo;
  //LTE_SchedulingInfo_MBMS_r14_t schedulingInfo;
  LTE_SchedulingInfo_MBMS_r14_t *schedulingInfo = &carrier->schedulingInfo_MBMS;
  LTE_SIB_Type_t *sib_type;
  uint8_t *buffer                      = carrier->SIB1_MBMS;
  LTE_BCCH_DL_SCH_Message_MBMS_t *bcch_message  = &carrier->siblock1_MBMS;
  LTE_SystemInformationBlockType1_MBMS_r14_t **sib1_MBMS = &carrier->sib1_MBMS;
  int i;
  struct LTE_MBSFN_AreaInfo_r9 *MBSFN_Area1/*, *MBSFN_Area2*/;
  memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_MBMS_t));
  bcch_message->message.present = LTE_BCCH_DL_SCH_MessageType_MBMS_r14_PR_c1;
  bcch_message->message.choice.c1.present = LTE_BCCH_DL_SCH_MessageType_MBMS_r14__c1_PR_systemInformationBlockType1_MBMS_r14;
  //  memcpy(&bcch_message.message.choice.c1.choice.systemInformationBlockType1,sib1,sizeof(SystemInformationBlockType1_t));
  *sib1_MBMS = &bcch_message->message.choice.c1.choice.systemInformationBlockType1_MBMS_r14;
  PLMN_identity_info = CALLOC(1, sizeof(LTE_PLMN_IdentityInfo_t) * num_plmn);
  //memset(&schedulingInfo,0,sizeof(LTE_SchedulingInfo_MBMS_r14_t));
  memset(schedulingInfo,0,sizeof(LTE_SchedulingInfo_MBMS_r14_t));

  if (PLMN_identity_info == NULL)
    exit(1);

  schedulingInfo = CALLOC(1, sizeof(LTE_SchedulingInfo_MBMS_r14_t));

  if (schedulingInfo == NULL)
    exit(1);

  sib_type = CALLOC(1, sizeof(LTE_SIB_Type_t));

  if (sib_type == NULL)
    exit(1);

  /* as per TS 36.311, up to 6 PLMN_identity_info are allowed in list -> add one by one */
  for (i = 0; i < num_plmn; ++i) {
    PLMN_identity_info[i].plmn_Identity.mcc = CALLOC(1,sizeof(*PLMN_identity_info[i].plmn_Identity.mcc));
    memset(PLMN_identity_info[i].plmn_Identity.mcc,0,sizeof(*PLMN_identity_info[i].plmn_Identity.mcc));
    asn_set_empty(&PLMN_identity_info[i].plmn_Identity.mcc->list);//.size=0;
    dummy_mcc_0 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mcc_1 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mcc_2 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));

    if (dummy_mcc_0 == NULL || dummy_mcc_1 == NULL || dummy_mcc_2 == NULL)
      exit(1);
    *dummy_mcc_0 = (configuration->mcc[i] / 100) % 10;
    *dummy_mcc_1 = (configuration->mcc[i] / 10) % 10;
    *dummy_mcc_2 = (configuration->mcc[i] / 1) % 10;

    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mcc->list, dummy_mcc_0);
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mcc->list, dummy_mcc_1);
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mcc->list, dummy_mcc_2);
    PLMN_identity_info[i].plmn_Identity.mnc.list.size=0;
    PLMN_identity_info[i].plmn_Identity.mnc.list.count=0;
    dummy_mnc_0 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mnc_1 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mnc_2 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));

    if (dummy_mnc_0 == NULL || dummy_mnc_1 == NULL || dummy_mnc_2 == NULL)
      exit(1);

    if (configuration->mnc[i] >= 100) {
      *dummy_mnc_0 = (configuration->mnc[i] / 100) % 10;
      *dummy_mnc_1 = (configuration->mnc[i] / 10) % 10;
      *dummy_mnc_2 = (configuration->mnc[i] / 1) % 10;
    } else {
      if (configuration->mnc_digit_length[i] == 2) {
        *dummy_mnc_0 = (configuration->mnc[i] / 10) % 10;
        *dummy_mnc_1 = (configuration->mnc[i] / 1) % 10;
        *dummy_mnc_2 = 0xf;
      } else {
        *dummy_mnc_0 = (configuration->mnc[i] / 100) % 100;
        *dummy_mnc_1 = (configuration->mnc[i] / 10) % 10;
        *dummy_mnc_2 = (configuration->mnc[i] / 1) % 10;
      }
    }

    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mnc.list, dummy_mnc_0);
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mnc.list, dummy_mnc_1);

    if (*dummy_mnc_2 != 0xf) {
      ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mnc.list, dummy_mnc_2);
    } else {
      free(dummy_mnc_2);
    }

    //assign_enum(&PLMN_identity_info.cellReservedForOperatorUse,PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved);
    PLMN_identity_info[i].cellReservedForOperatorUse=LTE_PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;
    ASN_SEQUENCE_ADD(&(*sib1_MBMS)->cellAccessRelatedInfo_r14.plmn_IdentityList_r14.list,&PLMN_identity_info[i]);
  }

  // 16 bits
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.trackingAreaCode_r14.buf = MALLOC(2);
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.trackingAreaCode_r14.buf[0] = (configuration->tac >> 8) & 0xff;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.trackingAreaCode_r14.buf[1] = (configuration->tac >> 0) & 0xff;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.trackingAreaCode_r14.size=2;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.trackingAreaCode_r14.bits_unused=0;
  // 28 bits
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.buf = MALLOC(8);
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[0] = (configuration->cell_identity >> 20) & 0xff;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[1] = (configuration->cell_identity >> 12) & 0xff;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[2] = (configuration->cell_identity >>  4) & 0xff;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[3] = (configuration->cell_identity <<  4) & 0xf0;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.size=4;
  (*sib1_MBMS)->cellAccessRelatedInfo_r14.cellIdentity_r14.bits_unused=4;
  (*sib1_MBMS)->freqBandIndicator_r14 = configuration->eutra_band[CC_id];

  schedulingInfo->si_Periodicity_r14=LTE_SchedulingInfo__si_Periodicity_rf16;
  *sib_type = LTE_SIB_Type_MBMS_r14_sibType13_v920;
  ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo_r14.list, sib_type);
  ASN_SEQUENCE_ADD(&(*sib1_MBMS)->schedulingInfoList_MBMS_r14.list, schedulingInfo);
  (*sib1_MBMS)->si_WindowLength_r14=LTE_SystemInformationBlockType1_MBMS_r14__si_WindowLength_r14_ms20;
  (*sib1_MBMS)->systemInfoValueTag_r14=0;
  //  (*sib1).nonCriticalExtension = calloc(1,sizeof(*(*sib1).nonCriticalExtension));

  //sib13 optional
  if (1) {
    (*sib1_MBMS)->systemInformationBlockType13_r14 =                             CALLOC(1,sizeof(struct LTE_SystemInformationBlockType13_r9));
    memset((*sib1_MBMS)->systemInformationBlockType13_r14,0,sizeof(struct LTE_SystemInformationBlockType13_r9));
    ((*sib1_MBMS)->systemInformationBlockType13_r14)->notificationConfig_r9.notificationRepetitionCoeff_r9= LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
    ((*sib1_MBMS)->systemInformationBlockType13_r14)->notificationConfig_r9.notificationOffset_r9= 0;
    ((*sib1_MBMS)->systemInformationBlockType13_r14)->notificationConfig_r9.notificationSF_Index_r9= 1;
    //((*sib1_MBMS)->systemInformationBlockType13_r14)->mbsfn_AreaInfoList_r9= CALLOC(1,sizeof(LTE_MBSFN_AreaInfoList_r9_t));
    //memset(((*sib1_MBMS)->systemInformationBlockType13_r14)->mbsfn_AreaInfoList_r9,0,sizeof(LTE_MBSFN_AreaInfoList_r9_t));
    LTE_MBSFN_AreaInfoList_r9_t *MBSFNArea_list= &((*sib1_MBMS)->systemInformationBlockType13_r14)->mbsfn_AreaInfoList_r9;
    //  MBSFN-AreaInfoList
    memset(MBSFNArea_list,0,sizeof(*MBSFNArea_list));
    // MBSFN Area 1
    MBSFN_Area1= CALLOC(1, sizeof(*MBSFN_Area1));
    MBSFN_Area1->mbsfn_AreaId_r9= 1;
    MBSFN_Area1->non_MBSFNregionLength= LTE_MBSFN_AreaInfo_r9__non_MBSFNregionLength_s2;
    MBSFN_Area1->notificationIndicator_r9= 0;
    MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
    MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9= 1; // in accordance with mbsfn subframe configuration in sib2
    MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_ModificationPeriod_r9_rf512;
    //  Subframe Allocation Info
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size= 1;
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=0x20<<2;  // FDD: SF1
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;
    MBSFN_Area1->mcch_Config_r9.signallingMCS_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__signallingMCS_r9_n2;
    (MBSFN_Area1)->ext1 = CALLOC (1, sizeof(*(MBSFN_Area1)->ext1));
    memset((MBSFN_Area1)->ext1,0,sizeof(*(MBSFN_Area1)->ext1));
    MBSFN_Area1->ext1->subcarrierSpacingMBMS_r14 = CALLOC(1,sizeof(*( MBSFN_Area1->ext1)->subcarrierSpacingMBMS_r14));
    memset(MBSFN_Area1->ext1->subcarrierSpacingMBMS_r14,0,sizeof(*((MBSFN_Area1)->ext1)->subcarrierSpacingMBMS_r14));
    *(MBSFN_Area1->ext1->subcarrierSpacingMBMS_r14) = LTE_MBSFN_AreaInfo_r9__ext1__subcarrierSpacingMBMS_r14_khz_1dot25;
    ASN_SEQUENCE_ADD(&MBSFNArea_list->list,MBSFN_Area1);
  }

  //nonMBSFN_SubframeConfig_r14 optional
  if(1) {
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14 =                             CALLOC(1,sizeof(struct LTE_NonMBSFN_SubframeConfig_r14));
    memset((*sib1_MBMS)->nonMBSFN_SubframeConfig_r14,0,sizeof(struct LTE_NonMBSFN_SubframeConfig_r14));
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->radioFrameAllocationPeriod_r14 = LTE_NonMBSFN_SubframeConfig_r14__radioFrameAllocationPeriod_r14_rf4;
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->radioFrameAllocationOffset_r14 = 0;
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->subframeAllocation_r14.buf = MALLOC(2);
    //100000001 byte(0)=10000000 byte(1)=xxxxxxx1
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->subframeAllocation_r14.buf[0] = 0x8<<0;
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->subframeAllocation_r14.buf[1] = 0;
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->subframeAllocation_r14.size = 2;
    (*sib1_MBMS)->nonMBSFN_SubframeConfig_r14->subframeAllocation_r14.bits_unused = 7;
  }

  //if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS, (void *)bcch_message);
  //}
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS,
                                   NULL,
                                   (void *)bcch_message,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"[eNB] SystemInformationBlockType1_MBMS Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

//-----------------------------------------------------------------------------
/*
 * Generate the configuration structure for CDRX feature
 */
LTE_DRX_Config_t *do_DrxConfig(int CC_id, 
                               RrcConfigurationReq *configuration, 
                               LTE_UE_EUTRA_Capability_t *UEcap)
//-----------------------------------------------------------------------------
{
  /* Check CC id */
  if (CC_id >= MAX_NUM_CCs) {
    LOG_E(RRC, "[do_DrxConfig] Invalid CC_id for DRX configuration\n");
    return NULL;
  }

  /* No CDRX configuration */
  if (configuration->radioresourceconfig[CC_id].drx_Config_present == LTE_DRX_Config_PR_NOTHING) {
    return NULL;
  }

  /* CDRX not implemented for TDD */
  if (configuration->frame_type[CC_id] == 1) {
    LOG_E(RRC, "[do_DrxConfig] CDRX not implemented for TDD\n");
    return NULL;
  }

  /* Need UE capabilities */  
  if (!UEcap) {
    LOG_E(RRC,"[do_DrxConfig] No UEcap pointer\n");
    return NULL;
  }

  /* Check the UE capabilities, CDRX not implemented for Coverage Extension */
  LTE_UE_EUTRA_Capability_v920_IEs_t	*cap_920 = NULL;
  LTE_UE_EUTRA_Capability_v940_IEs_t	*cap_940 = NULL;
  LTE_UE_EUTRA_Capability_v1020_IEs_t	*cap_1020 = NULL;
  LTE_UE_EUTRA_Capability_v1060_IEs_t	*cap_1060 = NULL;
  LTE_UE_EUTRA_Capability_v1090_IEs_t	*cap_1090 = NULL;
  LTE_UE_EUTRA_Capability_v1130_IEs_t	*cap_1130 = NULL;
  LTE_UE_EUTRA_Capability_v1170_IEs_t	*cap_1170 = NULL;
  LTE_UE_EUTRA_Capability_v1180_IEs_t	*cap_1180 = NULL;
  LTE_UE_EUTRA_Capability_v11a0_IEs_t	*cap_11a0 = NULL;
  LTE_UE_EUTRA_Capability_v1250_IEs_t	*cap_1250 = NULL;
  LTE_UE_EUTRA_Capability_v1260_IEs_t	*cap_1260 = NULL;
  LTE_UE_EUTRA_Capability_v1270_IEs_t	*cap_1270 = NULL;
  LTE_UE_EUTRA_Capability_v1280_IEs_t	*cap_1280 = NULL;
  LTE_UE_EUTRA_Capability_v1310_IEs_t	*cap_1310 = NULL;
  LTE_CE_Parameters_r13_t	*CE_param = NULL;
  long *ce_a_param = NULL;

  cap_920 = UEcap->nonCriticalExtension;
  if (cap_920) {
    cap_940 = cap_920->nonCriticalExtension;
    if (cap_940) {
      cap_1020 = cap_940->nonCriticalExtension;
      if (cap_1020) {
        cap_1060 = cap_1020->nonCriticalExtension;
        if (cap_1060) {
          cap_1090 = cap_1060->nonCriticalExtension;
          if (cap_1090) {
            cap_1130 = cap_1090->nonCriticalExtension;
            if (cap_1130) {
              cap_1170 = cap_1130->nonCriticalExtension;
              if (cap_1170) {
                cap_1180 = cap_1170->nonCriticalExtension;
                if (cap_1180) {
                  cap_11a0 = cap_1180->nonCriticalExtension;
                  if (cap_11a0) {
                    cap_1250 = cap_11a0->nonCriticalExtension;
                    if (cap_1250) {
                      cap_1260 = cap_1250->nonCriticalExtension;
                      if (cap_1260) {
                        cap_1270 = cap_1260->nonCriticalExtension;
                        if (cap_1270) {
                          cap_1280 = cap_1270->nonCriticalExtension;
                          if (cap_1280) {
                            cap_1310 = cap_1280->nonCriticalExtension;
                            if (cap_1310) {
                              CE_param = cap_1310->ce_Parameters_r13;
                              if (CE_param) {
                                ce_a_param = CE_param->ce_ModeA_r13;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (ce_a_param) {
    LOG_E(RRC,"[do_DrxConfig] Coverage Extension not supported by CDRX\n");
    return NULL;
  }

  LTE_DRX_Config_t *drxConfig = NULL;
  BIT_STRING_t *featureGroupIndicators = NULL;
  bool ueSupportCdrxShortFlag = false;
  bool ueSupportCdrxLongFlag = false;

  /* Check the UE capabilities for short and long CDRX cycles support */
  featureGroupIndicators = UEcap->featureGroupIndicators;
  if (featureGroupIndicators) {
    if (featureGroupIndicators->size > 1 || (featureGroupIndicators->size == 1 && featureGroupIndicators->bits_unused < 4)) {
      ueSupportCdrxShortFlag = ((featureGroupIndicators->buf[0] & (uint8_t) 0x10) > 0);
      ueSupportCdrxLongFlag = ((featureGroupIndicators->buf[0] & (uint8_t) 0x08) > 0);
      LOG_D(RRC,"[do_DrxConfig] featureGroupIndicators->buf[0]: %x\n", featureGroupIndicators->buf[0]);
    } else LOG_W(RRC,"[do_DrxConfig] Not enough featureGroupIndicators bits\n");
  } else LOG_W(RRC,"[do_DrxConfig] No featureGroupIndicators pointer\n");

  drxConfig = (LTE_DRX_Config_t *) malloc(sizeof(LTE_DRX_Config_t));

  if (drxConfig == NULL) return NULL;

  memset(drxConfig, 0, sizeof(LTE_DRX_Config_t));

  /* Long DRX cycle support is mandatory for CDRX activation */
  if (!ueSupportCdrxLongFlag) {
    drxConfig->present = LTE_DRX_Config_PR_release;
  } else {
    drxConfig->present = configuration->radioresourceconfig[CC_id].drx_Config_present;
  }

  if (drxConfig->present == LTE_DRX_Config_PR_release) {
    drxConfig->choice.release = (NULL_t) 0;
  } else {
    struct LTE_DRX_Config__setup *choiceSetup = &drxConfig->choice.setup;
    choiceSetup->onDurationTimer = configuration->radioresourceconfig[CC_id].drx_onDurationTimer;
    choiceSetup->drx_InactivityTimer = configuration->radioresourceconfig[CC_id].drx_InactivityTimer;
    choiceSetup->drx_RetransmissionTimer = configuration->radioresourceconfig[CC_id].drx_RetransmissionTimer;
    choiceSetup->longDRX_CycleStartOffset.present = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset_present;

    switch (choiceSetup->longDRX_CycleStartOffset.present) {
      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf10:
        choiceSetup->longDRX_CycleStartOffset.choice.sf10 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf20:
        choiceSetup->longDRX_CycleStartOffset.choice.sf20 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf32:
        choiceSetup->longDRX_CycleStartOffset.choice.sf32 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf40:
        choiceSetup->longDRX_CycleStartOffset.choice.sf40 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf64:
        choiceSetup->longDRX_CycleStartOffset.choice.sf64 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf80:
        choiceSetup->longDRX_CycleStartOffset.choice.sf80 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf128:
        choiceSetup->longDRX_CycleStartOffset.choice.sf128 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf160:
        choiceSetup->longDRX_CycleStartOffset.choice.sf160 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf256:
        choiceSetup->longDRX_CycleStartOffset.choice.sf256 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf320:
        choiceSetup->longDRX_CycleStartOffset.choice.sf320 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf512:
        choiceSetup->longDRX_CycleStartOffset.choice.sf512 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf640:
        choiceSetup->longDRX_CycleStartOffset.choice.sf640 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf1024:
        choiceSetup->longDRX_CycleStartOffset.choice.sf1024 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf1280:
        choiceSetup->longDRX_CycleStartOffset.choice.sf1280 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf2048:
        choiceSetup->longDRX_CycleStartOffset.choice.sf2048 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      case LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf2560:
        choiceSetup->longDRX_CycleStartOffset.choice.sf2560 = configuration->radioresourceconfig[CC_id].drx_longDrx_CycleStartOffset;
        break;

      default:
        break;
    }

    /* Short DRX cycle configuration */
    if (!ueSupportCdrxShortFlag || configuration->radioresourceconfig[CC_id].drx_shortDrx_ShortCycleTimer == 0) {
      choiceSetup->shortDRX = NULL;
    } else {
      choiceSetup->shortDRX = MALLOC(sizeof(struct LTE_DRX_Config__setup__shortDRX));
      memset(choiceSetup->shortDRX, 0, sizeof(struct LTE_DRX_Config__setup__shortDRX));
      choiceSetup->shortDRX->shortDRX_Cycle = configuration->radioresourceconfig[CC_id].drx_shortDrx_Cycle;
      choiceSetup->shortDRX->drxShortCycleTimer = configuration->radioresourceconfig[CC_id].drx_shortDrx_ShortCycleTimer;
    }
  }

  return drxConfig;
}

uint8_t do_SIB1(rrc_eNB_carrier_data_t *carrier,
                int Mod_id,int CC_id, BOOLEAN_t brOption,
                RrcConfigurationReq *configuration
               ) {
  //  SystemInformation_t systemInformation;
  int num_plmn = configuration->num_plmn;
  LTE_PLMN_IdentityInfo_t *PLMN_identity_info;
  LTE_MCC_MNC_Digit_t *dummy_mcc_0;
  LTE_MCC_MNC_Digit_t *dummy_mcc_1;
  LTE_MCC_MNC_Digit_t *dummy_mcc_2;
  LTE_MCC_MNC_Digit_t *dummy_mnc_0;
  LTE_MCC_MNC_Digit_t *dummy_mnc_1;
  LTE_MCC_MNC_Digit_t *dummy_mnc_2;
  asn_enc_rval_t enc_rval;
  LTE_SchedulingInfo_t schedulingInfo,schedulingInfo2;
  LTE_SIB_Type_t sib_type,sib_type2;
  uint8_t *buffer;
  LTE_BCCH_DL_SCH_Message_t *bcch_message;
  LTE_SystemInformationBlockType1_t **sib1;

  if (brOption) {
    buffer       = carrier->SIB1_BR;
    bcch_message = &carrier->siblock1_BR;
    sib1         = &carrier->sib1_BR;
  } else {
    buffer       = carrier->SIB1;
    bcch_message = &carrier->siblock1;
    sib1         = &carrier->sib1;
  }

  memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_t));
  bcch_message->message.present = LTE_BCCH_DL_SCH_MessageType_PR_c1;
  bcch_message->message.choice.c1.present = LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1;
  //  memcpy(&bcch_message.message.choice.c1.choice.systemInformationBlockType1,sib1,sizeof(SystemInformationBlockType1_t));
  *sib1 = &bcch_message->message.choice.c1.choice.systemInformationBlockType1;
  PLMN_identity_info = CALLOC(1, sizeof(LTE_PLMN_IdentityInfo_t) * num_plmn);

  if (PLMN_identity_info == NULL)
    exit(1);

  memset(PLMN_identity_info,0,num_plmn * sizeof(LTE_PLMN_IdentityInfo_t));
  memset(&schedulingInfo,0,sizeof(LTE_SchedulingInfo_t));
  memset(&sib_type,0,sizeof(LTE_SIB_Type_t));
  if(configuration->eMBMS_M2_configured){
       memset(&schedulingInfo2,0,sizeof(LTE_SchedulingInfo_t));
       memset(&sib_type2,0,sizeof(LTE_SIB_Type_t));
  }

  /* as per TS 36.311, up to 6 PLMN_identity_info are allowed in list -> add one by one */
  for (int i = 0; i < num_plmn; ++i) {
    PLMN_identity_info[i].plmn_Identity.mcc = CALLOC(1,sizeof(*PLMN_identity_info[i].plmn_Identity.mcc));
    memset(PLMN_identity_info[i].plmn_Identity.mcc,0,sizeof(*PLMN_identity_info[i].plmn_Identity.mcc));
    asn_set_empty(&PLMN_identity_info[i].plmn_Identity.mcc->list);//.size=0;
    dummy_mcc_0 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mcc_1 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mcc_2 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));

    if (dummy_mcc_0 == NULL || dummy_mcc_1 == NULL || dummy_mcc_2 == NULL)
      exit(1);


    *dummy_mcc_0 = (configuration->mcc[i] / 100) % 10;
    *dummy_mcc_1 = (configuration->mcc[i] / 10) % 10;
    *dummy_mcc_2 = (configuration->mcc[i] / 1) % 10;
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mcc->list, dummy_mcc_0);
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mcc->list, dummy_mcc_1);
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mcc->list, dummy_mcc_2);
    PLMN_identity_info[i].plmn_Identity.mnc.list.size=0;
    PLMN_identity_info[i].plmn_Identity.mnc.list.count=0;
    dummy_mnc_0 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mnc_1 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));
    dummy_mnc_2 = CALLOC(1, sizeof(LTE_MCC_MNC_Digit_t));

    if (dummy_mnc_0 == NULL || dummy_mnc_1 == NULL || dummy_mnc_2 == NULL)
      exit(1);


    if (configuration->mnc[i] >= 100) {
      *dummy_mnc_0 = (configuration->mnc[i] / 100) % 10;
      *dummy_mnc_1 = (configuration->mnc[i] / 10) % 10;
      *dummy_mnc_2 = (configuration->mnc[i] / 1) % 10;
    } else {
      if (configuration->mnc_digit_length[i] == 2) {
        *dummy_mnc_0 = (configuration->mnc[i] / 10) % 10;
        *dummy_mnc_1 = (configuration->mnc[i] / 1) % 10;
        *dummy_mnc_2 = 0xf;
      } else {
        *dummy_mnc_0 = (configuration->mnc[i] / 100) % 100;
        *dummy_mnc_1 = (configuration->mnc[i] / 10) % 10;
        *dummy_mnc_2 = (configuration->mnc[i] / 1) % 10;
      }
    }
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mnc.list, dummy_mnc_0);
    ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mnc.list, dummy_mnc_1);

    if (*dummy_mnc_2 != 0xf) {
      ASN_SEQUENCE_ADD(&PLMN_identity_info[i].plmn_Identity.mnc.list, dummy_mnc_2);
    } else {
      free(dummy_mnc_2);
    }

    //assign_enum(&PLMN_identity_info.cellReservedForOperatorUse,PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved);
    PLMN_identity_info[i].cellReservedForOperatorUse=LTE_PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;
    ASN_SEQUENCE_ADD(&(*sib1)->cellAccessRelatedInfo.plmn_IdentityList.list,&PLMN_identity_info[i]);
  }

  // 16 bits
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf = MALLOC(2);
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf[0] = (configuration->tac >> 8) & 0xff;
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.buf[1] = (configuration->tac >> 0) & 0xff;
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.size = 2;
  (*sib1)->cellAccessRelatedInfo.trackingAreaCode.bits_unused = 0;
  // 28 bits
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf = MALLOC(8);
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[0] = (configuration->cell_identity >> 20) & 0xff;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[1] = (configuration->cell_identity >> 12) & 0xff;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[2] = (configuration->cell_identity >> 4) & 0xff;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.buf[3] = (configuration->cell_identity << 4) & 0xf0;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.size=4;
  (*sib1)->cellAccessRelatedInfo.cellIdentity.bits_unused=4;
  //  assign_enum(&(*sib1)->cellAccessRelatedInfo.cellBarred,SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred);
  (*sib1)->cellAccessRelatedInfo.cellBarred=LTE_SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred;
  //  assign_enum(&(*sib1)->cellAccessRelatedInfo.intraFreqReselection,SystemInformationBlockType1__cellAccessRelatedInfo__intraFreqReselection_allowed);
  (*sib1)->cellAccessRelatedInfo.intraFreqReselection=LTE_SystemInformationBlockType1__cellAccessRelatedInfo__intraFreqReselection_notAllowed;
  (*sib1)->cellAccessRelatedInfo.csg_Indication=0;
  (*sib1)->cellSelectionInfo.q_RxLevMin=-65;
  (*sib1)->cellSelectionInfo.q_RxLevMinOffset=NULL;
  //(*sib1)->p_Max = CALLOC(1, sizeof(P_Max_t));
  // *((*sib1)->p_Max) = 23;
  (*sib1)->freqBandIndicator = configuration->eutra_band[CC_id];
  schedulingInfo.si_Periodicity=LTE_SchedulingInfo__si_Periodicity_rf8;
  if(configuration->eMBMS_M2_configured){
       schedulingInfo2.si_Periodicity=LTE_SchedulingInfo__si_Periodicity_rf8;
  }
  // This is for SIB2/3
  sib_type=LTE_SIB_Type_sibType3;
  ASN_SEQUENCE_ADD(&schedulingInfo.sib_MappingInfo.list,&sib_type);
  ASN_SEQUENCE_ADD(&(*sib1)->schedulingInfoList.list,&schedulingInfo);
  if(configuration->eMBMS_M2_configured){
         sib_type2=LTE_SIB_Type_sibType13_v920;
         ASN_SEQUENCE_ADD(&schedulingInfo2.sib_MappingInfo.list,&sib_type2);
         ASN_SEQUENCE_ADD(&(*sib1)->schedulingInfoList.list,&schedulingInfo2);
  }
  //  ASN_SEQUENCE_ADD(&schedulingInfo.sib_MappingInfo.list,NULL);

  if (configuration->frame_type[CC_id] == TDD)
  {
    (*sib1)->tdd_Config =                             CALLOC(1,sizeof(struct LTE_TDD_Config));
    (*sib1)->tdd_Config->subframeAssignment = configuration->tdd_config[CC_id];
    (*sib1)->tdd_Config->specialSubframePatterns =  configuration->tdd_config_s[CC_id];
  }

  (*sib1)->si_WindowLength = LTE_SystemInformationBlockType1__si_WindowLength_ms20;
  (*sib1)->systemInfoValueTag = 0;
  (*sib1)->nonCriticalExtension = calloc(1, sizeof(LTE_SystemInformationBlockType1_v890_IEs_t));
  LTE_SystemInformationBlockType1_v890_IEs_t *sib1_890 = (*sib1)->nonCriticalExtension;
  sib1_890->lateNonCriticalExtension = NULL;
  sib1_890->nonCriticalExtension = calloc(1, sizeof(LTE_SystemInformationBlockType1_v920_IEs_t));
  memset(sib1_890->nonCriticalExtension, 0, sizeof(LTE_SystemInformationBlockType1_v920_IEs_t));
  LTE_SystemInformationBlockType1_v920_IEs_t *sib1_920 = (*sib1_890).nonCriticalExtension;
  sib1_920->ims_EmergencySupport_r9 = NULL; // ptr
  sib1_920->cellSelectionInfo_v920 = NULL;
  sib1_920->nonCriticalExtension = calloc(1, sizeof(LTE_SystemInformationBlockType1_v1130_IEs_t));
  memset(sib1_920->nonCriticalExtension, 0, sizeof(LTE_SystemInformationBlockType1_v1130_IEs_t));
  //////Rel11
  LTE_SystemInformationBlockType1_v1130_IEs_t *sib1_1130 = sib1_920->nonCriticalExtension;
  sib1_1130->tdd_Config_v1130 = NULL; // ptr
  sib1_1130->cellSelectionInfo_v1130 = NULL; // ptr
  sib1_1130->nonCriticalExtension = calloc(1, sizeof(LTE_SystemInformationBlockType1_v1250_IEs_t));
  memset(sib1_1130->nonCriticalExtension, 0, sizeof(LTE_SystemInformationBlockType1_v1250_IEs_t));
  ///Rel12
  LTE_SystemInformationBlockType1_v1250_IEs_t *sib1_1250 = sib1_1130->nonCriticalExtension;
  sib1_1250->cellAccessRelatedInfo_v1250.category0Allowed_r12 = NULL; // long*
  sib1_1250->cellSelectionInfo_v1250 = NULL;
  sib1_1250->freqBandIndicatorPriority_r12 = 0; // long* // FIXME
  sib1_1250->nonCriticalExtension = NULL;
  ////Rel1310
  if ((configuration->schedulingInfoSIB1_BR_r13[CC_id] != 0) &&
      (brOption==TRUE)) {
    sib1_1250->nonCriticalExtension = calloc(1, sizeof(LTE_SystemInformationBlockType1_v1310_IEs_t));
    memset(sib1_1250->nonCriticalExtension, 0, sizeof(LTE_SystemInformationBlockType1_v1310_IEs_t));
    LTE_SystemInformationBlockType1_v1310_IEs_t *sib1_1310 = sib1_1250->nonCriticalExtension;

    if (configuration->hyperSFN_r13[CC_id]) {
      sib1_1310->hyperSFN_r13 = calloc(1, sizeof(BIT_STRING_t)); // type
      memset(sib1_1310->hyperSFN_r13, 0, sizeof(BIT_STRING_t));
      sib1_1310->hyperSFN_r13->buf = calloc(2, sizeof(uint8_t));
      memmove(sib1_1310->hyperSFN_r13->buf, configuration->hyperSFN_r13[CC_id], 2 * sizeof(uint8_t));
      sib1_1310->hyperSFN_r13->size = 2;
      sib1_1310->hyperSFN_r13->bits_unused = 6;
    } else
      sib1_1310->hyperSFN_r13 = NULL;

    if (configuration->eDRX_Allowed_r13[CC_id]) {
      sib1_1310->eDRX_Allowed_r13 = calloc(1, sizeof(long));
      *sib1_1310->eDRX_Allowed_r13 = *configuration->eDRX_Allowed_r13[CC_id];
    } else
      sib1_1310->eDRX_Allowed_r13 = NULL; // long*

    if (configuration->cellSelectionInfoCE_r13[CC_id]) {
      sib1_1310->cellSelectionInfoCE_r13 = calloc(1, sizeof(LTE_CellSelectionInfoCE_r13_t));
      memset(sib1_1310->cellSelectionInfoCE_r13, 0, sizeof(LTE_CellSelectionInfoCE_r13_t));
      sib1_1310->cellSelectionInfoCE_r13->q_RxLevMinCE_r13 = configuration->q_RxLevMinCE_r13[CC_id]; // (Q_RxLevMin_t) long

      if (configuration->q_QualMinRSRQ_CE_r13[CC_id]) {
        sib1_1310->cellSelectionInfoCE_r13->q_QualMinRSRQ_CE_r13 = calloc(1, sizeof(long));
        *sib1_1310->cellSelectionInfoCE_r13->q_QualMinRSRQ_CE_r13 = *configuration->q_QualMinRSRQ_CE_r13[CC_id];
      } else
        sib1_1310->cellSelectionInfoCE_r13->q_QualMinRSRQ_CE_r13 = NULL;
    } else
      sib1_1310->cellSelectionInfoCE_r13 = NULL;

    if (configuration->bandwidthReducedAccessRelatedInfo_r13[CC_id]) {
      sib1_1310->bandwidthReducedAccessRelatedInfo_r13
        = calloc(1, sizeof(struct LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13));
      LOG_I(RRC,"Allocating memory for BR access of SI (%p)\n",
            sib1_1310->bandwidthReducedAccessRelatedInfo_r13);
      sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_WindowLength_BR_r13
        = configuration->si_WindowLength_BR_r13[CC_id]; // 0
      sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_RepetitionPattern_r13
        = configuration->si_RepetitionPattern_r13[CC_id]; // 0
      sib1_1310->bandwidthReducedAccessRelatedInfo_r13->schedulingInfoList_BR_r13 = calloc(1, sizeof(LTE_SchedulingInfoList_BR_r13_t));
      LTE_SchedulingInfo_BR_r13_t *schedulinginfo_br_13 = calloc(1, sizeof(LTE_SchedulingInfo_BR_r13_t));
      memset(schedulinginfo_br_13, 0, sizeof(LTE_SchedulingInfo_BR_r13_t));
      int num_sched_info_br = configuration->scheduling_info_br_size[CC_id];
      int index;

      for (index = 0; index < num_sched_info_br; ++index) {
        schedulinginfo_br_13->si_Narrowband_r13 = configuration->si_Narrowband_r13[CC_id][index];
        schedulinginfo_br_13->si_TBS_r13 = configuration->si_TBS_r13[CC_id][index];
        LOG_I(RRC,"Adding (%d,%d) to scheduling_info_br_13\n",(int)schedulinginfo_br_13->si_Narrowband_r13,(int)schedulinginfo_br_13->si_TBS_r13);
        ASN_SEQUENCE_ADD(&sib1_1310->bandwidthReducedAccessRelatedInfo_r13->schedulingInfoList_BR_r13->list, schedulinginfo_br_13);
      }

      if (configuration->fdd_DownlinkOrTddSubframeBitmapBR_r13[CC_id]) {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13
          = calloc(1, sizeof(struct LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__fdd_DownlinkOrTddSubframeBitmapBR_r13));
        memset(sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13, 0,
               sizeof(struct LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__fdd_DownlinkOrTddSubframeBitmapBR_r13));

        if (*configuration->fdd_DownlinkOrTddSubframeBitmapBR_r13[CC_id]) {
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->present
            = LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__fdd_DownlinkOrTddSubframeBitmapBR_r13_PR_subframePattern10_r13;
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern10_r13.buf = calloc(2, sizeof(uint8_t));
          memmove(sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern10_r13.buf, &configuration->fdd_DownlinkOrTddSubframeBitmapBR_val_r13[CC_id],
                  2 * sizeof(uint8_t));
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern10_r13.size = 2;
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern10_r13.bits_unused = 6;
        } else {
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->present
            = LTE_SystemInformationBlockType1_v1310_IEs__bandwidthReducedAccessRelatedInfo_r13__fdd_DownlinkOrTddSubframeBitmapBR_r13_PR_subframePattern40_r13;
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern40_r13.buf = calloc(5, sizeof(uint8_t));
          //                  memmove(sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern40_r13.buf, &configuration->fdd_DownlinkOrTddSubframeBitmapBR_val_r13[CC_id], 5 * sizeof(uint8_t));
          int bm_index;

          for (bm_index = 0; bm_index < 5; bm_index++) {
            sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern40_r13.buf[bm_index] = 0xFF;
          }

          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern40_r13.size = 5;
          sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13->choice.subframePattern40_r13.bits_unused = 0;
        }
      } else {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_DownlinkOrTddSubframeBitmapBR_r13 = NULL;
      }

      if (configuration->fdd_UplinkSubframeBitmapBR_r13[CC_id]) {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13 = calloc(1, sizeof(BIT_STRING_t));
        memset(sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13, 0, sizeof(BIT_STRING_t));
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13->buf = calloc(2, sizeof(uint8_t));
        memmove(sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13->buf, configuration->fdd_UplinkSubframeBitmapBR_r13[CC_id],
                2 * sizeof(uint8_t));
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13->size = 2;
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13->bits_unused = 6;
      } else {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->fdd_UplinkSubframeBitmapBR_r13 = NULL;
      }

      sib1_1310->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13 = configuration->startSymbolBR_r13[CC_id];
      sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_HoppingConfigCommon_r13
        = configuration->si_HoppingConfigCommon_r13[CC_id];

      if (configuration->si_ValidityTime_r13[CC_id]) {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_ValidityTime_r13 = calloc(1, sizeof(long));
        memset(sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_ValidityTime_r13, 0, sizeof(long));
        *sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_ValidityTime_r13 = *configuration->si_ValidityTime_r13[CC_id];
      } else {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->si_ValidityTime_r13 = NULL;
      }

      LTE_SystemInfoValueTagSI_r13_t *systemInfoValueTagSi_r13;
      int num_system_info_value_tag = configuration->system_info_value_tag_SI_size[CC_id];

      if (num_system_info_value_tag > 0) {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->systemInfoValueTagList_r13 = calloc(1, sizeof(LTE_SystemInfoValueTagList_r13_t));

        for (index = 0; index < num_system_info_value_tag; ++index) {
          systemInfoValueTagSi_r13 = CALLOC(1, sizeof(LTE_SystemInfoValueTagSI_r13_t));

          if (configuration->systemInfoValueTagSi_r13[CC_id][index]) {
            *systemInfoValueTagSi_r13 = configuration->systemInfoValueTagSi_r13[CC_id][index];
          } else {
            *systemInfoValueTagSi_r13 = 0;
          }

          ASN_SEQUENCE_ADD(&sib1_1310->bandwidthReducedAccessRelatedInfo_r13->systemInfoValueTagList_r13->list, systemInfoValueTagSi_r13);
        }
      } else {
        sib1_1310->bandwidthReducedAccessRelatedInfo_r13->systemInfoValueTagList_r13 = NULL;
      }
    } else {
      sib1_1310->bandwidthReducedAccessRelatedInfo_r13 = NULL;
    }

    sib1_1310->nonCriticalExtension = calloc(1, sizeof(LTE_SystemInformationBlockType1_v1320_IEs_t));
    memset(sib1_1310->nonCriticalExtension, 0, sizeof(LTE_SystemInformationBlockType1_v1320_IEs_t));
    /////Rel1320
    LTE_SystemInformationBlockType1_v1320_IEs_t *sib1_1320 = sib1_1310->nonCriticalExtension;

    if (configuration->freqHoppingParametersDL_r13[CC_id]) {
      sib1_1320->freqHoppingParametersDL_r13 = calloc(1, sizeof(struct LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13));
      memset(sib1_1320->freqHoppingParametersDL_r13, 0, sizeof(struct LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13));

      if (configuration->mpdcch_pdsch_HoppingNB_r13[CC_id]) {
        sib1_1320->freqHoppingParametersDL_r13->mpdcch_pdsch_HoppingNB_r13 = calloc(1, sizeof(long));
        *sib1_1320->freqHoppingParametersDL_r13->mpdcch_pdsch_HoppingNB_r13 = *configuration->mpdcch_pdsch_HoppingNB_r13[CC_id];
      } else
        sib1_1320->freqHoppingParametersDL_r13->mpdcch_pdsch_HoppingNB_r13 = NULL;

      sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeA_r13 = calloc(1,
          sizeof(struct LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeA_r13));
      memset(sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeA_r13, 0,
             sizeof(struct LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeA_r13));

      if (configuration->interval_DLHoppingConfigCommonModeA_r13[CC_id]) {
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeA_r13->present =
          LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeA_r13_PR_interval_FDD_r13;
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeA_r13->choice.interval_FDD_r13 = configuration->interval_DLHoppingConfigCommonModeA_r13_val[CC_id];
      } else {
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeA_r13->present =
          LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeA_r13_PR_interval_TDD_r13;
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeA_r13->choice.interval_TDD_r13 = configuration->interval_DLHoppingConfigCommonModeA_r13_val[CC_id];
      }

      sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeB_r13 = calloc(1,
          sizeof(struct LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeB_r13));
      memset(sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeB_r13, 0,
             sizeof(struct LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeB_r13));

      if (configuration->interval_DLHoppingConfigCommonModeB_r13[CC_id]) {
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeB_r13->present =
          LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeB_r13_PR_interval_FDD_r13;
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeB_r13->choice.interval_FDD_r13 = configuration->interval_DLHoppingConfigCommonModeB_r13_val[CC_id];
      } else {
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeB_r13->present =
          LTE_SystemInformationBlockType1_v1320_IEs__freqHoppingParametersDL_r13__interval_DLHoppingConfigCommonModeB_r13_PR_interval_TDD_r13;
        sib1_1320->freqHoppingParametersDL_r13->interval_DLHoppingConfigCommonModeB_r13->choice.interval_TDD_r13 = configuration->interval_DLHoppingConfigCommonModeB_r13_val[CC_id];
      }

      if (configuration->mpdcch_pdsch_HoppingOffset_r13[CC_id]) {
        sib1_1320->freqHoppingParametersDL_r13->mpdcch_pdsch_HoppingOffset_r13 = calloc(1, sizeof(long));
        *sib1_1320->freqHoppingParametersDL_r13->mpdcch_pdsch_HoppingOffset_r13 = *configuration->mpdcch_pdsch_HoppingOffset_r13[CC_id];
      } else
        sib1_1320->freqHoppingParametersDL_r13->mpdcch_pdsch_HoppingOffset_r13 = NULL;
    } else
      sib1_1320->freqHoppingParametersDL_r13 = NULL;

    sib1_1320->nonCriticalExtension = NULL;
  }

  (*sib1)->si_WindowLength=LTE_SystemInformationBlockType1__si_WindowLength_ms20;
  (*sib1)->systemInfoValueTag=0;
  //  (*sib1).nonCriticalExtension = calloc(1,sizeof(*(*sib1).nonCriticalExtension));

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)bcch_message,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"[eNB] SystemInformationBlockType1 Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}





uint8_t do_SIB23(uint8_t Mod_id,

                 int CC_id, BOOLEAN_t brOption, 
                 RrcConfigurationReq *configuration
                ) {
  struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib2_part,*sib3_part;
  int eMTC_configured = configuration->eMTC_configured;
  struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib18_part, *sib19_part, *sib21_part;
  LTE_SL_CommRxPoolList_r12_t *SL_CommRxPoolList; //for SIB18
  struct LTE_SL_CommResourcePool_r12 *SL_CommResourcePool; //for SIB18
  LTE_SL_DiscRxPoolList_r12_t *SL_DiscRxPoolList; //for SIB19 (discRxPool)
  struct LTE_SL_DiscResourcePool_r12 *SL_DiscResourcePool; //for SIB19 (discRxPool)
  //SL_DiscRxPoolList_r12_t *SL_DiscRxPoolPSList; //for SIB19 (discRxPoolPS)
  //struct SL_DiscResourcePool_r12 *SL_DiscResourcePoolPS; //for SIB19 (discRxPoolPS)
  //struct SL_V2X_ConfigCommon_r14 *SL_V2X_ConfigCommon;
  struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib13_part=NULL;
  LTE_MBSFN_SubframeConfigList_t *MBSFNSubframeConfigList;
  LTE_MBSFN_AreaInfoList_r9_t *MBSFNArea_list;
  struct LTE_MBSFN_AreaInfo_r9 *MBSFN_Area1, *MBSFN_Area2;
  asn_enc_rval_t enc_rval;
  LTE_BCCH_DL_SCH_Message_t         *bcch_message = &RC.rrc[Mod_id]->carrier[CC_id].systemInformation;
  uint8_t                       *buffer;
  LTE_SystemInformationBlockType2_t **sib2;
  RadioResourceConfig           *rrconfig;

  if (brOption) {
    buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23_BR;
    sib2     = &RC.rrc[Mod_id]->carrier[CC_id].sib2_BR;
    rrconfig = &configuration->radioresourceconfig_BR[CC_id];
    LOG_I(RRC,"Running SIB2/3 Encoding for eMTC\n");
  } else {
    buffer   = RC.rrc[Mod_id]->carrier[CC_id].SIB23;
    sib2     = &RC.rrc[Mod_id]->carrier[CC_id].sib2;
    rrconfig = &configuration->radioresourceconfig[CC_id];
  }

  LTE_SystemInformationBlockType3_t       **sib3        = &RC.rrc[Mod_id]->carrier[CC_id].sib3;
  LTE_SystemInformationBlockType13_r9_t   **sib13       = &RC.rrc[Mod_id]->carrier[CC_id].sib13;
  //TTN - for D2D
  LTE_SystemInformationBlockType18_r12_t     **sib18        = &RC.rrc[Mod_id]->carrier[CC_id].sib18;
  LTE_SystemInformationBlockType19_r12_t     **sib19        = &RC.rrc[Mod_id]->carrier[CC_id].sib19;
  LTE_SystemInformationBlockType21_r14_t     **sib21        = &RC.rrc[Mod_id]->carrier[CC_id].sib21;

  if (bcch_message) {
    memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_t));
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

  LOG_I(RRC,"[eNB %d] Configuration SIB2/3, eMBMS = %d\n", Mod_id, configuration->eMBMS_configured);
  sib2_part = CALLOC(1,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  sib3_part = CALLOC(1,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  memset(sib2_part,0,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  memset(sib3_part,0,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
  sib2_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2;
  sib3_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3;
  *sib2 = &sib2_part->choice.sib2;
  *sib3 = &sib3_part->choice.sib3;

  if ((configuration->eMBMS_configured > 0) && (brOption==FALSE)) {
    sib13_part = CALLOC(1,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    memset(sib13_part,0,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib13_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920;
    *sib13 = &sib13_part->choice.sib13_v920;
  }

  if (configuration->SL_configured > 0) {
    //TTN - for D2D
    sib18_part = CALLOC(1,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib19_part = CALLOC(1,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib21_part = CALLOC(1,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    memset(sib18_part,0,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    memset(sib19_part,0,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    memset(sib21_part,0,sizeof(struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib18_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib18_v1250;
    sib19_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib19_v1250;
    sib21_part->present = LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib21_v1430;
    *sib18 = &sib18_part->choice.sib18_v1250;
    *sib19 = &sib19_part->choice.sib19_v1250;
    *sib21 = &sib21_part->choice.sib21_v1430;
  }

  // sib2
  (*sib2)->ac_BarringInfo = NULL;
  (*sib2)->ext1 = NULL;
  (*sib2)->ext2 = NULL;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles                         = rrconfig->rach_numberOfRA_Preambles;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig                        = NULL;

  if (rrconfig->rach_preamblesGroupAConfig) {
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig
      = CALLOC(1,sizeof(struct LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig));
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->sizeOfRA_PreamblesGroupA
      = rrconfig->rach_sizeOfRA_PreamblesGroupA;
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messageSizeGroupA
      = rrconfig->rach_messageSizeGroupA;
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messagePowerOffsetGroupB
      = rrconfig->rach_messagePowerOffsetGroupB;
  }

  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep                   = rrconfig->rach_powerRampingStep;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower = rrconfig->rach_preambleInitialReceivedTargetPower;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax                       = rrconfig->rach_preambleTransMax;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize                  = rrconfig->rach_raResponseWindowSize;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer          = rrconfig->rach_macContentionResolutionTimer;
  (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.maxHARQ_Msg3Tx                                            = rrconfig->rach_maxHARQ_Msg3Tx;

  if (eMTC_configured>0) {
    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1 = calloc(1, sizeof(struct LTE_RACH_ConfigCommon__ext1));
    memset((*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1, 0, sizeof(struct LTE_RACH_ConfigCommon__ext1));

    if (rrconfig->preambleTransMax_CE_r13) {
      (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1->preambleTransMax_CE_r13 = calloc(1, sizeof(LTE_PreambleTransMax_t));
      *(*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1->preambleTransMax_CE_r13 = *rrconfig->preambleTransMax_CE_r13; // to be re-initialized when we find the enum
    } else (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1->preambleTransMax_CE_r13 = NULL;

    (*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1->rach_CE_LevelInfoList_r13 = calloc(1, sizeof(LTE_RACH_CE_LevelInfoList_r13_t));
    memset((*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1->rach_CE_LevelInfoList_r13, 0, sizeof(LTE_RACH_CE_LevelInfoList_r13_t));
    LTE_RACH_CE_LevelInfo_r13_t *rach_ce_levelinfo_r13;
    int num_rach_ce_level_info = configuration->rach_CE_LevelInfoList_r13_size[CC_id];
    int index;

    for (index = 0; index < num_rach_ce_level_info; ++index) {
      rach_ce_levelinfo_r13 = calloc(1, sizeof(LTE_RACH_CE_LevelInfo_r13_t));

      if (configuration->rach_CE_LevelInfoList_r13_size[CC_id]) {
        rach_ce_levelinfo_r13->preambleMappingInfo_r13.firstPreamble_r13 = configuration->firstPreamble_r13[CC_id][index];
        rach_ce_levelinfo_r13->preambleMappingInfo_r13.lastPreamble_r13  = configuration->lastPreamble_r13[CC_id][index];
        rach_ce_levelinfo_r13->ra_ResponseWindowSize_r13                 = configuration->ra_ResponseWindowSize_r13[CC_id][index];
        rach_ce_levelinfo_r13->mac_ContentionResolutionTimer_r13         = configuration->mac_ContentionResolutionTimer_r13[CC_id][index];
        rach_ce_levelinfo_r13->rar_HoppingConfig_r13                     = configuration->rar_HoppingConfig_r13[CC_id][index];
      } else {
        rach_ce_levelinfo_r13->preambleMappingInfo_r13.firstPreamble_r13 = 0;
        rach_ce_levelinfo_r13->preambleMappingInfo_r13.lastPreamble_r13 = 63;
        rach_ce_levelinfo_r13->ra_ResponseWindowSize_r13 = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf80;
        rach_ce_levelinfo_r13->mac_ContentionResolutionTimer_r13 = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf200;
        rach_ce_levelinfo_r13->rar_HoppingConfig_r13 = LTE_RACH_CE_LevelInfo_r13__rar_HoppingConfig_r13_off;
      }

      ASN_SEQUENCE_ADD(&(*sib2)->radioResourceConfigCommon.rach_ConfigCommon.ext1->rach_CE_LevelInfoList_r13->list, rach_ce_levelinfo_r13);
    }
  }

  // BCCH-Config
  (*sib2)->radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff
    = rrconfig->bcch_modificationPeriodCoeff;
  // PCCH-Config
  (*sib2)->radioResourceConfigCommon.pcch_Config.defaultPagingCycle
    = rrconfig->pcch_defaultPagingCycle;
  (*sib2)->radioResourceConfigCommon.pcch_Config.nB
    = rrconfig->pcch_nB;
  // PRACH-Config
  (*sib2)->radioResourceConfigCommon.prach_Config.rootSequenceIndex
    = rrconfig->prach_root;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_ConfigIndex
    = rrconfig->prach_config_index;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.highSpeedFlag
    = rrconfig->prach_high_speed;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig
    = rrconfig->prach_zero_correlation;
  (*sib2)->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_FreqOffset
    = rrconfig->prach_freq_offset;
  // PDSCH-Config
  (*sib2)->radioResourceConfigCommon.pdsch_ConfigCommon.referenceSignalPower
    = rrconfig->pdsch_referenceSignalPower;
  (*sib2)->radioResourceConfigCommon.pdsch_ConfigCommon.p_b
    = rrconfig->pdsch_p_b;
  // PUSCH-Config
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.n_SB
    = rrconfig->pusch_n_SB;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode
    = rrconfig->pusch_hoppingMode;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset
    = rrconfig->pusch_hoppingOffset;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM
    = rrconfig->pusch_enable64QAM;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled
    = rrconfig->pusch_groupHoppingEnabled;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH
    = rrconfig->pusch_groupAssignment;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled
    = rrconfig->pusch_sequenceHoppingEnabled;
  (*sib2)->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift
    = rrconfig->pusch_nDMRS1;
  // PUCCH-Config
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift
    = rrconfig->pucch_delta_shift;
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.nRB_CQI
    = rrconfig->pucch_nRB_CQI;
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.nCS_AN
    = rrconfig->pucch_nCS_AN;
  (*sib2)->radioResourceConfigCommon.pucch_ConfigCommon.n1PUCCH_AN
    = rrconfig->pucch_n1_AN;

  // SRS Config
  if (rrconfig->srs_enable == 1) {
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
      = LTE_SoundingRS_UL_ConfigCommon_PR_setup;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig
      = rrconfig->srs_BandwidthConfig;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig
      = rrconfig->srs_SubframeConfig;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission
      = rrconfig->srs_ackNackST;

    if (rrconfig->srs_MaxUpPts) {
      (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts
        = CALLOC(1,sizeof(long));
      *(*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts=1;
    } else {
      (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts = NULL;
    }

    RC.rrc[Mod_id]->srs_enable[CC_id] = 1;
  } else {
    RC.rrc[Mod_id]->srs_enable[CC_id] = 0;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present=LTE_SoundingRS_UL_ConfigCommon_PR_release;
    (*sib2)->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.release=0;
  }

  // uplinkPowerControlCommon
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUSCH
    = rrconfig->pusch_p0_Nominal;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUCCH
    = rrconfig->pucch_p0_Nominal;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.alpha
    = rrconfig->pusch_alpha;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1
    = rrconfig->pucch_deltaF_Format1;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b
    = rrconfig->pucch_deltaF_Format1b;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2
    = rrconfig->pucch_deltaF_Format2;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a
    = rrconfig->pucch_deltaF_Format2a;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b
    = rrconfig->pucch_deltaF_Format2b;
  (*sib2)->radioResourceConfigCommon.uplinkPowerControlCommon.deltaPreambleMsg3
    = rrconfig->msg3_delta_Preamble;
  (*sib2)->radioResourceConfigCommon.ul_CyclicPrefixLength
    = rrconfig->ul_CyclicPrefixLength;

  if (eMTC_configured>0) {
    (*sib2)->radioResourceConfigCommon.ext4 = calloc(1, sizeof(struct LTE_RadioResourceConfigCommonSIB__ext4));
    memset((*sib2)->radioResourceConfigCommon.ext4, 0, sizeof(struct LTE_RadioResourceConfigCommonSIB__ext4));
    (*sib2)->radioResourceConfigCommon.ext4->bcch_Config_v1310 = NULL; //calloc(1, sizeof(BCCH_Config_v1310_t));
    //memset((*sib2)->radioResourceConfigCommon.ext4->bcch_Config_v1310, 0, sizeof(BCCH_Config_v1310_t));
    //(*sib2)->radioResourceConfigCommon.ext4->bcch_Config_v1310->modificationPeriodCoeff_v1310 = BCCH_Config_v1310__modificationPeriodCoeff_v1310_n64;

    if (configuration->pcch_config_v1310) {
      (*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310 = CALLOC(1, sizeof(LTE_PCCH_Config_v1310_t));
      (*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310->paging_narrowBands_r13 = configuration->paging_narrowbands_r13[CC_id];
      (*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310->mpdcch_NumRepetition_Paging_r13 = configuration->mpdcch_numrepetition_paging_r13[CC_id];

      if (configuration->nb_v1310[CC_id]) {
        (*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310->nB_v1310 = CALLOC(1, sizeof(long));
        *(*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310->nB_v1310 = *configuration->nb_v1310[CC_id];
      } else {
        (*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310->nB_v1310 = NULL;
      }
    } else (*sib2)->radioResourceConfigCommon.ext4->pcch_Config_v1310 = NULL;

    if (configuration->sib2_freq_hoppingParameters_r13_exists[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13 = CALLOC(1, sizeof(LTE_FreqHoppingParameters_r13_t));

      if (configuration->sib2_interval_ULHoppingConfigCommonModeA_r13[CC_id]) {
        (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeA_r13
          = CALLOC(1, sizeof(struct LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13));

        if (*configuration->sib2_interval_ULHoppingConfigCommonModeA_r13[CC_id] == 0) {
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeA_r13->present
            = LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_interval_FDD_r13;
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeA_r13->choice.interval_FDD_r13
            = configuration->sib2_interval_ULHoppingConfigCommonModeA_r13_val[CC_id];
        } else {
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeA_r13->present
            = LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_interval_TDD_r13;
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeA_r13->choice.interval_TDD_r13
            = configuration->sib2_interval_ULHoppingConfigCommonModeA_r13_val[CC_id];
        }
      } else (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeA_r13 = NULL;

      if (configuration->sib2_interval_ULHoppingConfigCommonModeB_r13[CC_id]) {
        (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeB_r13
          = CALLOC(1, sizeof(struct LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13));

        if (*configuration->sib2_interval_ULHoppingConfigCommonModeB_r13[CC_id] == 0)  {
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeB_r13->present
            = LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_interval_FDD_r13;
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeB_r13->choice.interval_FDD_r13
            = configuration->sib2_interval_ULHoppingConfigCommonModeB_r13_val[CC_id];
        } else {
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeB_r13->present
            = LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_interval_TDD_r13;
          (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeB_r13->choice.interval_TDD_r13
            = configuration->sib2_interval_ULHoppingConfigCommonModeB_r13_val[CC_id];
        }
      } else (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13->interval_ULHoppingConfigCommonModeB_r13 = NULL;
    } else (*sib2)->radioResourceConfigCommon.ext4->freqHoppingParameters_r13 = NULL;

    // pdsch_ConfigCommon_v1310
    (*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310 = CALLOC(1,sizeof(LTE_PDSCH_ConfigCommon_v1310_t));

    if (configuration->pdsch_maxNumRepetitionCEmodeA_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeA_r13 = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeA_r13 = *configuration->pdsch_maxNumRepetitionCEmodeA_r13[CC_id];
    } else {
      (*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeA_r13 = NULL;
    }

    if (configuration->pdsch_maxNumRepetitionCEmodeB_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeB_r13 = CALLOC(1, sizeof(long)); // check if they're really long
      *(*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeB_r13 = *configuration->pdsch_maxNumRepetitionCEmodeB_r13[CC_id];
    } else {
      (*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeB_r13 = NULL;
    }

    //  *(*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeA_r13 = 0;
    //  (*sib2)->radioResourceConfigCommon.ext4->pdsch_ConfigCommon_v1310->pdsch_maxNumRepetitionCEmodeB_r13 = NULL;
    //  pusch_ConfigCommon_v1310
    (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310 = calloc(1,sizeof(LTE_PUSCH_ConfigCommon_v1310_t));

    if (configuration->pusch_maxNumRepetitionCEmodeA_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeA_r13 = calloc(1,sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeA_r13 = *configuration->pusch_maxNumRepetitionCEmodeA_r13[CC_id];
    } else {
      (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeA_r13 = NULL;
    }

    if (configuration->pusch_maxNumRepetitionCEmodeB_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeB_r13 = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeB_r13 = *configuration->pusch_maxNumRepetitionCEmodeB_r13[CC_id];
    } else {
      (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeB_r13 = NULL;
    }

    if (configuration->pusch_HoppingOffset_v1310[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_HoppingOffset_v1310 = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_HoppingOffset_v1310 = *configuration->pusch_HoppingOffset_v1310[CC_id];
    } else {
      (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_HoppingOffset_v1310 = NULL;
    }

    //  *(*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeA_r13 = 0;
    //  (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_maxNumRepetitionCEmodeB_r13 = NULL;
    //  (*sib2)->radioResourceConfigCommon.ext4->pusch_ConfigCommon_v1310->pusch_HoppingOffset_v1310 = NULL;

    if (rrconfig->prach_ConfigCommon_v1310) {
      (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310 = calloc(1, sizeof(LTE_PRACH_ConfigSIB_v1310_t));
      memset((*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310, 0, sizeof(LTE_PRACH_ConfigSIB_v1310_t));
      LTE_RSRP_Range_t *rsrp_range;
      int num_rsrp_range = configuration->rsrp_range_list_size[CC_id];
      int rsrp_index;

      for (rsrp_index = 0; rsrp_index < num_rsrp_range; ++rsrp_index) {
        rsrp_range = CALLOC(1, sizeof(LTE_RSRP_Range_t));

        if (configuration->rsrp_range_list_size[CC_id]) *rsrp_range = configuration->rsrp_range[CC_id][rsrp_index];
        else                                            *rsrp_range = 60;

        ASN_SEQUENCE_ADD(&(*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->rsrp_ThresholdsPrachInfoList_r13.list, rsrp_range);
      }

      (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13 = NULL;

      if (rrconfig->mpdcch_startSF_CSS_RA_r13) {
        (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13 = calloc(1, sizeof(struct LTE_PRACH_ConfigSIB_v1310__mpdcch_startSF_CSS_RA_r13));
        memset((*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13, 0, sizeof(struct LTE_PRACH_ConfigSIB_v1310__mpdcch_startSF_CSS_RA_r13));

        if (*rrconfig->mpdcch_startSF_CSS_RA_r13) {
          (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13->present = LTE_PRACH_ConfigSIB_v1310__mpdcch_startSF_CSS_RA_r13_PR_fdd_r13;
          (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13->choice.fdd_r13 = rrconfig->mpdcch_startSF_CSS_RA_r13_val;
        } else {
          (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13->present = LTE_PRACH_ConfigSIB_v1310__mpdcch_startSF_CSS_RA_r13_PR_tdd_r13;
          (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->mpdcch_startSF_CSS_RA_r13->choice.tdd_r13 = rrconfig->mpdcch_startSF_CSS_RA_r13_val;
        }
      }

      if (rrconfig->prach_HoppingOffset_r13) {
        (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->prach_HoppingOffset_r13 = calloc(1, sizeof(long));
        *(*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->prach_HoppingOffset_r13 = *rrconfig->prach_HoppingOffset_r13;
      } else (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->prach_HoppingOffset_r13 = NULL;

      LTE_PRACH_ParametersCE_r13_t *prach_parametersce_r13;
      int num_prach_parameters_ce = configuration->prach_parameters_list_size[CC_id];
      int prach_parameters_index;
      AssertFatal(num_prach_parameters_ce > 0, "PRACH CE parameter list is empty\n");

      for (prach_parameters_index = 0; prach_parameters_index < num_prach_parameters_ce; ++prach_parameters_index) {
        prach_parametersce_r13 = CALLOC(1, sizeof(LTE_PRACH_ParametersCE_r13_t));
        prach_parametersce_r13->prach_ConfigIndex_r13 = configuration->prach_config_index[CC_id][prach_parameters_index];
        prach_parametersce_r13->prach_FreqOffset_r13 = configuration->prach_freq_offset[CC_id][prach_parameters_index];
        AssertFatal(configuration->prach_StartingSubframe_r13[CC_id][prach_parameters_index]!=NULL,
                    "configuration->prach_StartingSubframe_r13[%d][%d] is null",
                    (int)CC_id,(int)prach_parameters_index);

        if (configuration->prach_StartingSubframe_r13[CC_id][prach_parameters_index]) {
          prach_parametersce_r13->prach_StartingSubframe_r13 = CALLOC(1, sizeof(long));
          *prach_parametersce_r13->prach_StartingSubframe_r13 = *configuration->prach_StartingSubframe_r13[CC_id][prach_parameters_index];
        } else prach_parametersce_r13->prach_StartingSubframe_r13 = NULL;

        if (configuration->maxNumPreambleAttemptCE_r13[CC_id][prach_parameters_index]) {
          prach_parametersce_r13->maxNumPreambleAttemptCE_r13 = CALLOC(1, sizeof(long));
          *prach_parametersce_r13->maxNumPreambleAttemptCE_r13 = *configuration->maxNumPreambleAttemptCE_r13[CC_id][prach_parameters_index];
        } else prach_parametersce_r13->maxNumPreambleAttemptCE_r13 = NULL;

        prach_parametersce_r13->numRepetitionPerPreambleAttempt_r13 = configuration->numRepetitionPerPreambleAttempt_r13[CC_id][prach_parameters_index];
        prach_parametersce_r13->mpdcch_NumRepetition_RA_r13 = configuration->mpdcch_NumRepetition_RA_r13[CC_id][prach_parameters_index];
        prach_parametersce_r13->prach_HoppingConfig_r13 = configuration->prach_HoppingConfig_r13[CC_id][prach_parameters_index];
        long *maxavailablenarrowband;
        int num_narrow_bands = configuration->max_available_narrow_band_size[CC_id][prach_parameters_index];
        int narrow_band_index;

        for (narrow_band_index = 0; narrow_band_index < num_narrow_bands; narrow_band_index++) {
          maxavailablenarrowband = CALLOC(1, sizeof(long));
          *maxavailablenarrowband = configuration->max_available_narrow_band[CC_id][prach_parameters_index][narrow_band_index];
          ASN_SEQUENCE_ADD(&prach_parametersce_r13->mpdcch_NarrowbandsToMonitor_r13.list, maxavailablenarrowband);
        }

        prach_parametersce_r13->mpdcch_NumRepetition_RA_r13 = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r1;
        prach_parametersce_r13->prach_HoppingConfig_r13 = LTE_PRACH_ParametersCE_r13__prach_HoppingConfig_r13_off;
        ASN_SEQUENCE_ADD(&(*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310->prach_ParametersListCE_r13.list, prach_parametersce_r13);
      }
    } else (*sib2)->radioResourceConfigCommon.ext4->prach_ConfigCommon_v1310 = NULL;

    (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310 = calloc(1, sizeof(LTE_PUCCH_ConfigCommon_v1310_t));
    memset((*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310, 0, sizeof(LTE_PUCCH_ConfigCommon_v1310_t));
    (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->n1PUCCH_AN_InfoList_r13 = calloc(1, sizeof(LTE_N1PUCCH_AN_InfoList_r13_t));
    int num_pucch_info_list = configuration->pucch_info_value_size[CC_id];
    int pucch_index;
    long *pucch_info_value;

    for (pucch_index = 0; pucch_index <  num_pucch_info_list; ++pucch_index) {
      pucch_info_value = CALLOC(1, sizeof(long));

      if (configuration->pucch_info_value_size[CC_id]) *pucch_info_value = configuration->pucch_info_value[CC_id][pucch_index];
      else                                             *pucch_info_value = 0;

      ASN_SEQUENCE_ADD(&(*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->n1PUCCH_AN_InfoList_r13->list, pucch_info_value);
    }

    if (configuration->pucch_NumRepetitionCE_Msg4_Level0_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level0_r13  = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level0_r13 =  *configuration->pucch_NumRepetitionCE_Msg4_Level0_r13[CC_id];
    } else (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level0_r13 = NULL;

    if (configuration->pucch_NumRepetitionCE_Msg4_Level1_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level1_r13  = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level1_r13 =  *configuration->pucch_NumRepetitionCE_Msg4_Level1_r13[CC_id];
    } else (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level1_r13 = NULL;

    if (configuration->pucch_NumRepetitionCE_Msg4_Level2_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level2_r13  = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level2_r13 =  *configuration->pucch_NumRepetitionCE_Msg4_Level2_r13[CC_id];
    } else (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level2_r13 = NULL;

    if (configuration->pucch_NumRepetitionCE_Msg4_Level3_r13[CC_id]) {
      (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level3_r13  = CALLOC(1, sizeof(long));
      *(*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level3_r13 =  *configuration->pucch_NumRepetitionCE_Msg4_Level3_r13[CC_id];
    } else (*sib2)->radioResourceConfigCommon.ext4->pucch_ConfigCommon_v1310->pucch_NumRepetitionCE_Msg4_Level3_r13 = NULL;
  } // eMTC_configured>0

  //-----------------------------------------------------------------------------------------------------------------------------------------
  // UE Timers and Constants
  (*sib2)->ue_TimersAndConstants.t300
    = rrconfig->ue_TimersAndConstants_t300;
  (*sib2)->ue_TimersAndConstants.t301
    = rrconfig->ue_TimersAndConstants_t301;
  (*sib2)->ue_TimersAndConstants.t310
    = rrconfig->ue_TimersAndConstants_t310;
  (*sib2)->ue_TimersAndConstants.n310
    = rrconfig->ue_TimersAndConstants_n310;
  (*sib2)->ue_TimersAndConstants.t311
    = rrconfig->ue_TimersAndConstants_t311;
  (*sib2)->ue_TimersAndConstants.n311
    = rrconfig->ue_TimersAndConstants_n311;

  (*sib2)->freqInfo.additionalSpectrumEmission = 1;
  (*sib2)->freqInfo.ul_CarrierFreq = NULL;
  (*sib2)->freqInfo.ul_Bandwidth = NULL;
  //  (*sib2)->mbsfn_SubframeConfigList = NULL;

  if (configuration->eMBMS_configured > 0) {
    LOG_I(RRC,"Adding MBSFN subframe Configuration 1 to SIB2\n");
    LTE_MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig1;
    (*sib2)->mbsfn_SubframeConfigList = CALLOC(1,sizeof(struct LTE_MBSFN_SubframeConfigList));
    MBSFNSubframeConfigList = (*sib2)->mbsfn_SubframeConfigList;
    sib2_mbsfn_SubframeConfig1= CALLOC(1,sizeof(*sib2_mbsfn_SubframeConfig1));
    memset((void *)sib2_mbsfn_SubframeConfig1,0,sizeof(*sib2_mbsfn_SubframeConfig1));
    sib2_mbsfn_SubframeConfig1->radioframeAllocationPeriod= LTE_MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
    sib2_mbsfn_SubframeConfig1->radioframeAllocationOffset= 1;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.size= 1;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.bits_unused= 2;
    sib2_mbsfn_SubframeConfig1->subframeAllocation.choice.oneFrame.buf[0]=0x38<<2;
    ASN_SEQUENCE_ADD(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig1);

    if (configuration->eMBMS_configured == 4 ) {
      LOG_I(RRC,"Adding MBSFN subframe Configuration 2 to SIB2\n");
      LTE_MBSFN_SubframeConfig_t *sib2_mbsfn_SubframeConfig2;
      sib2_mbsfn_SubframeConfig2= CALLOC(1,sizeof(*sib2_mbsfn_SubframeConfig2));
      memset((void *)sib2_mbsfn_SubframeConfig2,0,sizeof(*sib2_mbsfn_SubframeConfig2));
      sib2_mbsfn_SubframeConfig2->radioframeAllocationPeriod= LTE_MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
      sib2_mbsfn_SubframeConfig2->radioframeAllocationOffset= 1;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.buf= MALLOC(1);
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.size= 1;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.bits_unused= 2;
      sib2_mbsfn_SubframeConfig2->subframeAllocation.choice.oneFrame.buf[0]=0x07<<2;
      ASN_SEQUENCE_ADD(&MBSFNSubframeConfigList->list,sib2_mbsfn_SubframeConfig2);
    }
  }

  (*sib2)->timeAlignmentTimerCommon=LTE_TimeAlignmentTimer_infinity;//TimeAlignmentTimer_sf5120;
  /// (*SIB3)
  (*sib3)->ext1 = NULL;
  (*sib3)->cellReselectionInfoCommon.q_Hyst=LTE_SystemInformationBlockType3__cellReselectionInfoCommon__q_Hyst_dB4;
  (*sib3)->cellReselectionInfoCommon.speedStateReselectionPars=NULL;
  (*sib3)->cellReselectionServingFreqInfo.s_NonIntraSearch=NULL;
  (*sib3)->cellReselectionServingFreqInfo.threshServingLow=31;
  (*sib3)->cellReselectionServingFreqInfo.cellReselectionPriority=7;
  (*sib3)->intraFreqCellReselectionInfo.q_RxLevMin = -70;
  (*sib3)->intraFreqCellReselectionInfo.p_Max = NULL;
  (*sib3)->intraFreqCellReselectionInfo.s_IntraSearch = CALLOC(1,sizeof(*(*sib3)->intraFreqCellReselectionInfo.s_IntraSearch));
  *(*sib3)->intraFreqCellReselectionInfo.s_IntraSearch = 31;
  (*sib3)->intraFreqCellReselectionInfo.allowedMeasBandwidth=CALLOC(1,sizeof(*(*sib3)->intraFreqCellReselectionInfo.allowedMeasBandwidth));
  *(*sib3)->intraFreqCellReselectionInfo.allowedMeasBandwidth = LTE_AllowedMeasBandwidth_mbw6;
  (*sib3)->intraFreqCellReselectionInfo.presenceAntennaPort1 = 0;
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.buf = CALLOC(8,1);
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.size = 1;
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.buf[0] = 1 << 6;
  (*sib3)->intraFreqCellReselectionInfo.neighCellConfig.bits_unused = 6;
  (*sib3)->intraFreqCellReselectionInfo.t_ReselectionEUTRA = 1;
  (*sib3)->intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF = (struct LTE_SpeedStateScaleFactors *)NULL;
  (*sib3)->ext1 = CALLOC(1, sizeof(struct LTE_SystemInformationBlockType3__ext1));
  (*sib3)->ext1->s_IntraSearch_v920 = CALLOC(1, sizeof(struct LTE_SystemInformationBlockType3__ext1__s_IntraSearch_v920));
  (*sib3)->ext1->s_IntraSearch_v920->s_IntraSearchP_r9 = 31; // FIXME
  (*sib3)->ext1->s_IntraSearch_v920->s_IntraSearchQ_r9 = 4;
  (*sib3)->ext4 = CALLOC(1, sizeof(struct LTE_SystemInformationBlockType3__ext4));
  (*sib3)->ext4->cellSelectionInfoCE_r13 = CALLOC(1, sizeof(LTE_CellSelectionInfoCE_r13_t));
  (*sib3)->ext4->cellSelectionInfoCE_r13->q_RxLevMinCE_r13 = -70;
  (*sib3)->ext4->cellSelectionInfoCE_r13->q_QualMinRSRQ_CE_r13 = NULL;
  (*sib3)->ext4->t_ReselectionEUTRA_CE_r13 = CALLOC(1, sizeof(LTE_T_ReselectionEUTRA_CE_r13_t));
  *(*sib3)->ext4->t_ReselectionEUTRA_CE_r13 = 2;

  // SIB13
  // fill in all elements of SIB13 if present
  if (configuration->eMBMS_configured > 0 ) {
    //  Notification for mcch change
    (*sib13)->notificationConfig_r9.notificationRepetitionCoeff_r9= LTE_MBMS_NotificationConfig_r9__notificationRepetitionCoeff_r9_n2;
    (*sib13)->notificationConfig_r9.notificationOffset_r9= 0;
    (*sib13)->notificationConfig_r9.notificationSF_Index_r9= 1;
    //  MBSFN-AreaInfoList
    MBSFNArea_list= &(*sib13)->mbsfn_AreaInfoList_r9;//CALLOC(1,sizeof(*MBSFNArea_list));
    memset(MBSFNArea_list,0,sizeof(*MBSFNArea_list));
    // MBSFN Area 1
    MBSFN_Area1= CALLOC(1, sizeof(*MBSFN_Area1));
    MBSFN_Area1->mbsfn_AreaId_r9= 1;
    MBSFN_Area1->non_MBSFNregionLength= LTE_MBSFN_AreaInfo_r9__non_MBSFNregionLength_s2;
    MBSFN_Area1->notificationIndicator_r9= 0;
    MBSFN_Area1->mcch_Config_r9.mcch_RepetitionPeriod_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
    MBSFN_Area1->mcch_Config_r9.mcch_Offset_r9= 1; // in accordance with mbsfn subframe configuration in sib2
    MBSFN_Area1->mcch_Config_r9.mcch_ModificationPeriod_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_ModificationPeriod_r9_rf512;
    //  Subframe Allocation Info
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.size= 1;
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=0x20<<2;  // FDD: SF1
    MBSFN_Area1->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;
    MBSFN_Area1->mcch_Config_r9.signallingMCS_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__signallingMCS_r9_n7;
    ASN_SEQUENCE_ADD(&MBSFNArea_list->list,MBSFN_Area1);

    //MBSFN Area 2: currently only activated for eMBMS relaying
    if (configuration->eMBMS_configured == 4 ) {
      MBSFN_Area2= CALLOC(1, sizeof(*MBSFN_Area2));
      MBSFN_Area2->mbsfn_AreaId_r9= 2;
      MBSFN_Area2->non_MBSFNregionLength= LTE_MBSFN_AreaInfo_r9__non_MBSFNregionLength_s2;
      MBSFN_Area2->notificationIndicator_r9= 1;
      MBSFN_Area2->mcch_Config_r9.mcch_RepetitionPeriod_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_RepetitionPeriod_r9_rf32;
      MBSFN_Area2->mcch_Config_r9.mcch_Offset_r9= 1;
      MBSFN_Area2->mcch_Config_r9.mcch_ModificationPeriod_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__mcch_ModificationPeriod_r9_rf512;
      // Subframe Allocation Info
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.buf= MALLOC(1);
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.size= 1;
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.bits_unused= 2;
      MBSFN_Area2->mcch_Config_r9.sf_AllocInfo_r9.buf[0]=0x04<<2;  // FDD: SF6
      MBSFN_Area2->mcch_Config_r9.signallingMCS_r9= LTE_MBSFN_AreaInfo_r9__mcch_Config_r9__signallingMCS_r9_n7;
      ASN_SEQUENCE_ADD(&MBSFNArea_list->list,MBSFN_Area2);
    }

    //  end of adding for MBMS SIB13
  }

  if (configuration->SL_configured>0) {
    //TTN - for D2D
    // SIB18
    //commRxPool_r12
    (*sib18)->commConfig_r12 = CALLOC (1, sizeof(*(*sib18)->commConfig_r12));
    SL_CommRxPoolList= &(*sib18)->commConfig_r12->commRxPool_r12;
    memset(SL_CommRxPoolList,0,sizeof(*SL_CommRxPoolList));
    SL_CommResourcePool = CALLOC(1, sizeof(*SL_CommResourcePool));
    memset(SL_CommResourcePool,0,sizeof(*SL_CommResourcePool));
    SL_CommResourcePool->sc_CP_Len_r12 = configuration->rxPool_sc_CP_Len[CC_id];
    SL_CommResourcePool->sc_Period_r12 = configuration->rxPool_sc_Period[CC_id];
    SL_CommResourcePool->data_CP_Len_r12  = configuration->rxPool_data_CP_Len[CC_id];
    //sc_TF_ResourceConfig_r12
    SL_CommResourcePool->sc_TF_ResourceConfig_r12.prb_Num_r12 = configuration->rxPool_ResourceConfig_prb_Num[CC_id];
    SL_CommResourcePool->sc_TF_ResourceConfig_r12.prb_Start_r12 = configuration->rxPool_ResourceConfig_prb_Start[CC_id];
    SL_CommResourcePool->sc_TF_ResourceConfig_r12.prb_End_r12 = configuration->rxPool_ResourceConfig_prb_End[CC_id];
    SL_CommResourcePool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.present = configuration->rxPool_ResourceConfig_offsetIndicator_present[CC_id];

    if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.present == LTE_SL_OffsetIndicator_r12_PR_small_r12 ) {
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.choice.small_r12 = configuration->rxPool_ResourceConfig_offsetIndicator_choice[CC_id] ;
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.present == LTE_SL_OffsetIndicator_r12_PR_large_r12 ) {
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.choice.large_r12 = configuration->rxPool_ResourceConfig_offsetIndicator_choice[CC_id] ;
    }

    SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present = configuration->rxPool_ResourceConfig_subframeBitmap_present[CC_id];

    if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs4_r12) {
      //for BS4
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs8_r12) {
      //for BS8
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs12_r12) {
      //for BS12
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs16_r12) {
      //for BS16
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs30_r12) {
      //for BS30
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs40_r12) {
      //for BS40
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs42_r12) {
      //for BS42
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.size = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.buf  = (uint8_t *)configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_CommResourcePool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.bits_unused = configuration->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }

    //dataHoppingConfig_r12
    SL_CommResourcePool->dataHoppingConfig_r12.hoppingParameter_r12 = 0;
    SL_CommResourcePool->dataHoppingConfig_r12.numSubbands_r12  =  LTE_SL_HoppingConfigComm_r12__numSubbands_r12_ns1;
    SL_CommResourcePool->dataHoppingConfig_r12.rb_Offset_r12 = 0;
    //ue_SelectedResourceConfig_r12
    SL_CommResourcePool->ue_SelectedResourceConfig_r12 = CALLOC (1, sizeof (*SL_CommResourcePool->ue_SelectedResourceConfig_r12));
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.prb_Num_r12 = 20;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.prb_Start_r12 = 5;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.prb_End_r12 = 44;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.offsetIndicator_r12.present = LTE_SL_OffsetIndicator_r12_PR_small_r12;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.offsetIndicator_r12.choice.small_r12 = 0 ;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.present = LTE_SubframeBitmapSL_r12_PR_bs40_r12;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.size = 5;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf  = CALLOC(1,5);
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.bits_unused = 0;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf[0] = 0xF0;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf[1] = 0xFF;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf[2] = 0xFF;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf[3] = 0xFF;
    SL_CommResourcePool->ue_SelectedResourceConfig_r12->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf[4] = 0xFF;
    //SL_CommResourcePool->ue_SelectedResourceConfig_r12->trpt_Subset_r12 = CALLOC (1, sizeof(*SL_CommResourcePool->ue_SelectedResourceConfig_r12->trpt_Subset_r12));
    //rxParametersNCell_r12
    SL_CommResourcePool->rxParametersNCell_r12 = CALLOC (1, sizeof (*SL_CommResourcePool->rxParametersNCell_r12));
    SL_CommResourcePool->rxParametersNCell_r12->tdd_Config_r12 = CALLOC (1, sizeof (*SL_CommResourcePool->rxParametersNCell_r12->tdd_Config_r12));
    SL_CommResourcePool->rxParametersNCell_r12->tdd_Config_r12->subframeAssignment = 0 ;
    SL_CommResourcePool->rxParametersNCell_r12->tdd_Config_r12->specialSubframePatterns = 0;
    SL_CommResourcePool->rxParametersNCell_r12->syncConfigIndex_r12 = 0;
    //txParameters_r12
    SL_CommResourcePool->txParameters_r12 = CALLOC (1, sizeof (*SL_CommResourcePool->txParameters_r12));
    SL_CommResourcePool->txParameters_r12->sc_TxParameters_r12.alpha_r12 = LTE_Alpha_r12_al0;
    SL_CommResourcePool->txParameters_r12->sc_TxParameters_r12.p0_r12 = 0;
    SL_CommResourcePool->ext1 = NULL ;
    //end SL_CommResourcePool
    //add SL_CommResourcePool to SL_CommRxPoolList
    ASN_SEQUENCE_ADD(&SL_CommRxPoolList->list,SL_CommResourcePool);
    //end commRxPool_r12
    //TODO:  commTxPoolNormalCommon_r12, similar to commRxPool_r12
    //TODO: commTxPoolExceptional_r12
    //TODO: commSyncConfig_r12
    // may add commTxResourceUC-ReqAllowed with Ext1
    (*sib18)->ext1 = NULL;
    (*sib18)->lateNonCriticalExtension = NULL;
    // end SIB18
    // SIB19
    // fill in all elements of SIB19 if present
    //discConfig_r12
    (*sib19)->discConfig_r12 = CALLOC (1, sizeof(*(*sib19)->discConfig_r12));
    SL_DiscRxPoolList = &(*sib19)->discConfig_r12->discRxPool_r12;
    memset(SL_DiscRxPoolList,0,sizeof(*SL_DiscRxPoolList));
    //fill SL_DiscResourcePool
    SL_DiscResourcePool = CALLOC(1, sizeof(*SL_DiscResourcePool));
    SL_DiscResourcePool->cp_Len_r12 = configuration->discRxPool_cp_Len[CC_id];
    SL_DiscResourcePool->discPeriod_r12 = configuration->discRxPool_discPeriod[CC_id];
    //sc_TF_ResourceConfig_r12
    SL_DiscResourcePool->numRetx_r12 = configuration->discRxPool_numRetx[CC_id];
    SL_DiscResourcePool->numRepetition_r12 = configuration->discRxPool_numRepetition[CC_id];
    SL_DiscResourcePool->tf_ResourceConfig_r12.prb_Num_r12 = configuration->discRxPool_ResourceConfig_prb_Num[CC_id];
    SL_DiscResourcePool->tf_ResourceConfig_r12.prb_Start_r12 = configuration->discRxPool_ResourceConfig_prb_Start[CC_id];
    SL_DiscResourcePool->tf_ResourceConfig_r12.prb_End_r12 = configuration->discRxPool_ResourceConfig_prb_End[CC_id];
    SL_DiscResourcePool->tf_ResourceConfig_r12.offsetIndicator_r12.present = configuration->discRxPool_ResourceConfig_offsetIndicator_present[CC_id];

    if (SL_DiscResourcePool->tf_ResourceConfig_r12.offsetIndicator_r12.present == LTE_SL_OffsetIndicator_r12_PR_small_r12 ) {
      SL_DiscResourcePool->tf_ResourceConfig_r12.offsetIndicator_r12.choice.small_r12 = configuration->discRxPool_ResourceConfig_offsetIndicator_choice[CC_id] ;
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.offsetIndicator_r12.present == LTE_SL_OffsetIndicator_r12_PR_large_r12 ) {
      SL_DiscResourcePool->tf_ResourceConfig_r12.offsetIndicator_r12.choice.large_r12 = configuration->discRxPool_ResourceConfig_offsetIndicator_choice[CC_id] ;
    }

    SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present = configuration->discRxPool_ResourceConfig_subframeBitmap_present[CC_id];

    if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs4_r12) {
      //for BS4
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf  =  (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs8_r12) {
      //for BS8
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.buf  = (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs12_r12) {
      //for BS12
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.buf  = (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs16_r12) {
      //for BS16
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.buf  = (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs30_r12) {
      //for BS30
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.buf  = (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs40_r12) {
      //for BS40
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf  = (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.present == LTE_SubframeBitmapSL_r12_PR_bs42_r12) {
      //for BS42
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.size = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.buf  = (uint8_t *)configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
      SL_DiscResourcePool->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.bits_unused = configuration->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }

    //add SL_DiscResourcePool to SL_DiscRxPoolList
    ASN_SEQUENCE_ADD(&SL_DiscRxPoolList->list,SL_DiscResourcePool);
    /*
    //for DiscRxPoolPS
    (*sib19)->ext1 = CALLOC (1, sizeof(*(*sib19)->ext1));
    (*sib19)->ext1->discConfigPS_13 = CALLOC (1, sizeof(*((*sib19)->ext1->discConfigPS_13)));

    SL_DiscRxPoolPSList = &(*sib19)->ext1->discConfigPS_13->discRxPoolPS_r13;
    memset(SL_DiscRxPoolPSList,0,sizeof(*SL_DiscRxPoolPSList));
    //fill SL_DiscResourcePool
    SL_DiscResourcePoolPS = CALLOC(1, sizeof(*SL_DiscResourcePoolPS));

    SL_DiscResourcePoolPS->cp_Len_r12 = configuration->discRxPoolPS_cp_Len[CC_id];
    SL_DiscResourcePoolPS->discPeriod_r12 = configuration->discRxPoolPS_discPeriod[CC_id];
    //sc_TF_ResourceConfig_r12
    SL_DiscResourcePoolPS->numRetx_r12 = configuration->discRxPoolPS_numRetx[CC_id];
    SL_DiscResourcePoolPS->numRepetition_r12 =  configuration->discRxPoolPS_numRepetition[CC_id];

    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.prb_Num_r12 = configuration->discRxPoolPS_ResourceConfig_prb_Num[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.prb_Start_r12 = configuration->discRxPoolPS_ResourceConfig_prb_Start[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.prb_End_r12 = configuration->discRxPoolPS_ResourceConfig_prb_End[CC_id];

    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.offsetIndicator_r12.present = configuration->discRxPoolPS_ResourceConfig_offsetIndicator_present[CC_id];
    if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.offsetIndicator_r12.present == SL_OffsetIndicator_r12_PR_small_r12 ) {
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.offsetIndicator_r12.choice.small_r12 = configuration->discRxPoolPS_ResourceConfig_offsetIndicator_choice[CC_id] ;
    } else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.offsetIndicator_r12.present == SL_OffsetIndicator_r12_PR_large_r12 ){
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.offsetIndicator_r12.choice.large_r12 = configuration->discRxPoolPS_ResourceConfig_offsetIndicator_choice[CC_id] ;
    }

    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_present[CC_id];
    if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs4_r12){
    //for BS4
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs4_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs8_r12){
    //for BS8
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs8_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    } else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs12_r12){
    //for BS12
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs12_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs16_r12){
    //for BS16
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs16_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs30_r12){
    //for BS30
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs30_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs40_r12){
    //for BS40
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }else if (SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.present == SubframeBitmapSL_r12_PR_bs42_r12){
    //for BS42
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.size = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[CC_id];
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.buf  = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[CC_id];;
    SL_DiscResourcePoolPS->tf_ResourceConfig_r12.subframeBitmap_r12.choice.bs42_r12.bits_unused = configuration->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[CC_id];
    }

    //add SL_DiscResourcePool to SL_DiscRxPoolList
    ASN_SEQUENCE_ADD(&SL_DiscRxPoolPSList->list,SL_DiscResourcePoolPS);
    */
    (*sib19)->lateNonCriticalExtension = NULL;
    //end SIB19
    //SIB21
    (*sib21)->sl_V2X_ConfigCommon_r14 = CALLOC (1, sizeof(*(*sib21)->sl_V2X_ConfigCommon_r14));
    //SL_V2X_ConfigCommon= (*sib21)->sl_V2X_ConfigCommon_r14;
    memset((*sib21)->sl_V2X_ConfigCommon_r14,0,sizeof(*(*sib21)->sl_V2X_ConfigCommon_r14));
    struct LTE_SL_CommRxPoolListV2X_r14 *SL_CommRxPoolListV2X;
    struct LTE_SL_CommResourcePoolV2X_r14 *SL_CommResourcePoolV2X;
    (*sib21)->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14 = CALLOC(1, sizeof(*(*sib21)->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14));
    SL_CommRxPoolListV2X = (*sib21)->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14;
    SL_CommResourcePoolV2X = CALLOC(1, sizeof(*SL_CommResourcePoolV2X));
    memset(SL_CommResourcePoolV2X,0,sizeof(*SL_CommResourcePoolV2X));
    SL_CommResourcePoolV2X->sl_OffsetIndicator_r14 = CALLOC(1, sizeof(*SL_CommResourcePoolV2X->sl_OffsetIndicator_r14));
    SL_CommResourcePoolV2X->sl_OffsetIndicator_r14->present  = LTE_SL_OffsetIndicator_r12_PR_small_r12;
    SL_CommResourcePoolV2X->sl_OffsetIndicator_r14->choice.small_r12 = 0;
    SL_CommResourcePoolV2X->sl_Subframe_r14.present = LTE_SubframeBitmapSL_r14_PR_bs40_r14;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.size =  5;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.buf =  CALLOC(1,5);
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.bits_unused = 0;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.buf[0] = 0xF0;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.buf[1] = 0xFF;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.buf[2] = 0xFF;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.buf[3] = 0xFF;
    SL_CommResourcePoolV2X->sl_Subframe_r14.choice.bs40_r14.buf[4] = 0xFF;
    SL_CommResourcePoolV2X->adjacencyPSCCH_PSSCH_r14 = 1;
    SL_CommResourcePoolV2X->sizeSubchannel_r14 = 10;
    SL_CommResourcePoolV2X->numSubchannel_r14 =  5;
    SL_CommResourcePoolV2X->startRB_Subchannel_r14 = 10;
    //rxParametersNCell_r12
    SL_CommResourcePoolV2X->rxParametersNCell_r14 = CALLOC (1, sizeof (*SL_CommResourcePoolV2X->rxParametersNCell_r14));
    SL_CommResourcePoolV2X->rxParametersNCell_r14->tdd_Config_r14 = CALLOC (1, sizeof (*SL_CommResourcePoolV2X->rxParametersNCell_r14->tdd_Config_r14));
    SL_CommResourcePoolV2X->rxParametersNCell_r14->tdd_Config_r14->subframeAssignment = 0 ;
    SL_CommResourcePoolV2X->rxParametersNCell_r14->tdd_Config_r14->specialSubframePatterns = 0;
    SL_CommResourcePoolV2X->rxParametersNCell_r14->syncConfigIndex_r14 = 0;
    ASN_SEQUENCE_ADD(&SL_CommRxPoolListV2X->list,SL_CommResourcePoolV2X);
    //end SIB21
  }

  bcch_message->message.present = LTE_BCCH_DL_SCH_MessageType_PR_c1;
  bcch_message->message.choice.c1.present = LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformation;
  /*  memcpy((void*)&bcch_message.message.choice.c1.choice.systemInformation,
   (void*)systemInformation,
   sizeof(SystemInformation_t));*/
  bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.present = LTE_SystemInformation__criticalExtensions_PR_systemInformation_r8;
  bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count=0;
  //  asn_set_empty(&systemInformation->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list);//.size=0;
  //  systemInformation->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count=0;
  ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,
                   sib2_part);
  ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,
                   sib3_part);

  if (configuration->eMBMS_configured > 0) {
    ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation.criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list, sib13_part);
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)bcch_message,
                                   buffer,
                                   900);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"[eNB] SystemInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI encoding failed for SIB23\n");
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCConnectionRequest(uint8_t Mod_id, uint8_t *buffer,uint8_t *rv) {
  asn_enc_rval_t enc_rval;
  uint8_t buf[5],buf2=0;
  LTE_UL_CCCH_Message_t ul_ccch_msg;
  LTE_RRCConnectionRequest_t *rrcConnectionRequest;
  memset((void *)&ul_ccch_msg,0,sizeof(LTE_UL_CCCH_Message_t));
  ul_ccch_msg.message.present           = LTE_UL_CCCH_MessageType_PR_c1;
  ul_ccch_msg.message.choice.c1.present = LTE_UL_CCCH_MessageType__c1_PR_rrcConnectionRequest;
  rrcConnectionRequest          = &ul_ccch_msg.message.choice.c1.choice.rrcConnectionRequest;
  rrcConnectionRequest->criticalExtensions.present = LTE_RRCConnectionRequest__criticalExtensions_PR_rrcConnectionRequest_r8;

  if (1) {
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.present = LTE_InitialUE_Identity_PR_randomValue;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.size = 5;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.bits_unused = 0;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf = buf;
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[0] = rv[0];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[1] = rv[1];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[2] = rv[2];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[3] = rv[3];
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.choice.randomValue.buf[4] = rv[4];
  } else {
    rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.ue_Identity.present = LTE_InitialUE_Identity_PR_s_TMSI;
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

  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.establishmentCause = LTE_EstablishmentCause_mo_Signalling; //EstablishmentCause_mo_Data;
  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.spare.buf = &buf2;
  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.spare.size=1;
  rrcConnectionRequest->criticalExtensions.choice.rrcConnectionRequest_r8.spare.bits_unused = 7;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_UL_CCCH_Message, (void *)&ul_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_CCCH_Message,
                                   NULL,
                                   (void *)&ul_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"[UE] RRCConnectionRequest Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}


//TTN for D2D - 3GPP TS 36.331 (Section 5.10.2.3)
uint8_t do_SidelinkUEInformation(uint8_t Mod_id, uint8_t *buffer,  LTE_SL_DestinationInfoList_r12_t  *destinationInfoList, long *discTxResourceReq, SL_TRIGGER_t mode) {
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  LTE_SidelinkUEInformation_r12_t *sidelinkUEInformation;
  LTE_ARFCN_ValueEUTRA_r9_t carrierFreq = 25655;//sidelink communication frequency (hardcoded - should come from SIB2)
  memset((void *)&ul_dcch_msg,0,sizeof(LTE_UL_DCCH_Message_t));
  ul_dcch_msg.message.present = LTE_UL_DCCH_MessageType_PR_messageClassExtension;
  ul_dcch_msg.message.choice.messageClassExtension.present  = LTE_UL_DCCH_MessageType__messageClassExtension_PR_c2;
  ul_dcch_msg.message.choice.messageClassExtension.choice.c2.present =  LTE_UL_DCCH_MessageType__messageClassExtension__c2_PR_sidelinkUEInformation_r12;
  sidelinkUEInformation            = &ul_dcch_msg.message.choice.messageClassExtension.choice.c2.choice.sidelinkUEInformation_r12;
  //3GPP TS 36.331 (Section 5.10.2.3)
  sidelinkUEInformation->criticalExtensions.present = LTE_SidelinkUEInformation_r12__criticalExtensions_PR_c1;
  sidelinkUEInformation->criticalExtensions.choice.c1.present = LTE_SidelinkUEInformation_r12__criticalExtensions__c1_PR_sidelinkUEInformation_r12;

  switch(mode) {
    //if SIB18 is available
    case SL_RECEIVE_COMMUNICATION: // to receive sidelink communication
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commRxInterestedFreq_r12 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commRxInterestedFreq_r12));
      memcpy((void *)sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commRxInterestedFreq_r12, (void *)&carrierFreq,
             sizeof(LTE_ARFCN_ValueEUTRA_r9_t));
      break;

    case SL_TRANSMIT_NON_RELAY_ONE_TO_MANY: //to transmit non-relay related one-to-many sidelink communication
      //commTxResourceReq
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commTxResourceReq_r12 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commTxResourceReq_r12));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commTxResourceReq_r12->carrierFreq_r12 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commTxResourceReq_r12->carrierFreq_r12));
      memcpy((void *)sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commTxResourceReq_r12->carrierFreq_r12, (void *)&carrierFreq,
             sizeof(LTE_ARFCN_ValueEUTRA_r9_t));
      memcpy(&sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.commTxResourceReq_r12->destinationInfoList_r12,
             destinationInfoList,
             sizeof(LTE_SL_DestinationInfoList_r12_t));
      break;

    case SL_TRANSMIT_NON_RELAY_ONE_TO_ONE://transmit non-relay related one-to-one sidelink communication
      //if commTxResourceUC-ReqAllowed is included in SIB18
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceReqUC_r13 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceReqUC_r13));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceReqUC_r13->carrierFreq_r12 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceReqUC_r13->carrierFreq_r12));
      memcpy((void *)sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceReqUC_r13->carrierFreq_r12, (void *)&carrierFreq,
             sizeof (LTE_ARFCN_ValueEUTRA_r9_t));
      memcpy(&sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceReqUC_r13->destinationInfoList_r12,
             destinationInfoList,
             sizeof(LTE_SL_DestinationInfoList_r12_t));
      break;

    case SL_TRANSMIT_RELAY_ONE_TO_ONE: //transmit relay related one-to-one sidelink communication
      //if SIB19 includes discConfigRelay and UE acts a relay or UE has a selected relay
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13= CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->commTxResourceReqRelayUC_r13 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->commTxResourceReqRelayUC_r13));
      memcpy(&sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->commTxResourceReqRelayUC_r13->destinationInfoList_r12,
             destinationInfoList,
             sizeof(*destinationInfoList));
      //set ue-type to relayUE or remoteUE
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->ue_Type_r13
        =LTE_SidelinkUEInformation_v1310_IEs__commTxResourceInfoReqRelay_r13__ue_Type_r13_relayUE;
      //sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12->nonCriticalExtension->commTxResourceInfoReqRelay_r13->ue_Type_r13 =SidelinkUEInformation_v1310_IEs__commTxResourceInfoReqRelay_r13__ue_Type_r13_remoteUE;
      break;

    case SL_TRANSMIT_RELAY_ONE_TO_MANY: //transmit relay related one-to-many sidelink communication
      //if SIB19 includes discConfigRelay and UE acts a relay
      //set ue-type to relayUE
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13= CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->commTxResourceReqRelay_r13 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->commTxResourceReqRelay_r13));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->ue_Type_r13 =
        LTE_SidelinkUEInformation_v1310_IEs__commTxResourceInfoReqRelay_r13__ue_Type_r13_relayUE;
      memcpy(&sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->commTxResourceInfoReqRelay_r13->commTxResourceReqRelay_r13->destinationInfoList_r12,
             destinationInfoList,
             sizeof(*destinationInfoList));
      break;

    //if SIB19 is available
    //we consider only one frequency  - a serving frequency
    case SL_RECEIVE_DISCOVERY: //receive sidelink discovery announcements
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.discRxInterest_r12 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.discRxInterest_r12));
      *sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.discRxInterest_r12 = LTE_SidelinkUEInformation_r12_IEs__discRxInterest_r12_true;
      break;

    case SL_TRANSMIT_NON_PS_DISCOVERY://to transmit non-PS related sidelink discovery announcements
      //for the first frequency
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.discTxResourceReq_r12 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.discTxResourceReq_r12));
      memcpy((void *)sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.discTxResourceReq_r12,
             (void *)discTxResourceReq,
             sizeof(long));
      //for additional frequency
      break;

    case SL_TRANSMIT_PS_DISCOVERY://to transmit PS related sidelink discovery announcements
      //if to transmit non-relay PS related discovery announcements and SIB19 includes discConfigPS
      //if UE is acting as relay UE and SIB includes discConfigRelay (relay threshold condition)
      //if relay UE/has a selected relay UE and if SIB19 includes discConfigRelay
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->discTxResourceReqPS_r13 = CALLOC(1,
          sizeof(*sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->discTxResourceReqPS_r13));
      sidelinkUEInformation->criticalExtensions.choice.c1.choice.sidelinkUEInformation_r12.nonCriticalExtension->discTxResourceReqPS_r13->discTxResourceReq_r13 = *discTxResourceReq;
      break;

    //SIB21
    case SL_RECEIVE_V2X:
      //TODO
      break;

    case SL_TRANSMIT_V2X:
      //TODO
      break;

    //TODO: request sidelink discovery transmission/reception gaps
    //TODO: report the system information parameters related to sidelink discovery of carriers other than the primary
    default:
      break;
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message,
                                   NULL,
                                   (void *)&ul_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"SidelinkUEInformation Encoded %d bits (%d bytes)\n",(uint32_t)enc_rval.encoded,(uint32_t)((enc_rval.encoded+7)/8));
  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCConnectionSetupComplete(uint8_t Mod_id, uint8_t *buffer, const uint8_t Transaction_id, uint8_t sel_plmn_id, const int dedicatedInfoNASLength, const char *dedicatedInfoNAS) {
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  LTE_RRCConnectionSetupComplete_t *rrcConnectionSetupComplete;
  memset((void *)&ul_dcch_msg,0,sizeof(LTE_UL_DCCH_Message_t));
  ul_dcch_msg.message.present           = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete;
  rrcConnectionSetupComplete            = &ul_dcch_msg.message.choice.c1.choice.rrcConnectionSetupComplete;
  rrcConnectionSetupComplete->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionSetupComplete->criticalExtensions.present = LTE_RRCConnectionSetupComplete__criticalExtensions_PR_c1;
  rrcConnectionSetupComplete->criticalExtensions.choice.c1.present = LTE_RRCConnectionSetupComplete__criticalExtensions__c1_PR_rrcConnectionSetupComplete_r8;
  rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.nonCriticalExtension=CALLOC(1,
      sizeof(*rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.nonCriticalExtension));
  rrcConnectionSetupComplete->criticalExtensions.choice.c1.choice.rrcConnectionSetupComplete_r8.selectedPLMN_Identity= sel_plmn_id;
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
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message,
                                   NULL,
                                   (void *)&ul_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"RRCConnectionSetupComplete Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionReconfigurationComplete(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *buffer,
  const uint8_t Transaction_id
)
//------------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  LTE_RRCConnectionReconfigurationComplete_t *rrcConnectionReconfigurationComplete;
  memset((void *)&ul_dcch_msg,0,sizeof(LTE_UL_DCCH_Message_t));
  ul_dcch_msg.message.present                     = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present           = LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionReconfigurationComplete;
  rrcConnectionReconfigurationComplete            = &ul_dcch_msg.message.choice.c1.choice.rrcConnectionReconfigurationComplete;
  rrcConnectionReconfigurationComplete->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReconfigurationComplete->criticalExtensions.present =
    LTE_RRCConnectionReconfigurationComplete__criticalExtensions_PR_rrcConnectionReconfigurationComplete_r8;
  rrcConnectionReconfigurationComplete->criticalExtensions.choice.rrcConnectionReconfigurationComplete_r8.nonCriticalExtension=NULL;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message,
                                   NULL,
                                   (void *)&ul_dcch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(RRC,"RRCConnectionReconfigurationComplete Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}


//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionSetup(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_eNB_ue_context_t      *const ue_context_pP,
  int                        CC_id,
  uint8_t                   *const buffer,
  const uint8_t              transmission_mode,
  const uint8_t              Transaction_id,
  LTE_SRB_ToAddModList_t  **SRB_configList,
  struct LTE_PhysicalConfigDedicated  **physicalConfigDedicated) {
  asn_enc_rval_t enc_rval;
  eNB_RRC_INST *rrc               = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[CC_id];
  long *logicalchannelgroup = NULL;
  struct LTE_SRB_ToAddMod *SRB1_config = NULL;
  struct LTE_SRB_ToAddMod__rlc_Config *SRB1_rlc_config = NULL;
  struct LTE_SRB_ToAddMod__logicalChannelConfig *SRB1_lchan_config = NULL;
  struct LTE_LogicalChannelConfig__ul_SpecificParameters *SRB1_ul_SpecificParameters = NULL;
  LTE_PhysicalConfigDedicated_t *physicalConfigDedicated2 = NULL;
  LTE_DL_CCCH_Message_t dl_ccch_msg;
  LTE_RRCConnectionSetup_t *rrcConnectionSetup = NULL;
  LTE_DL_FRAME_PARMS *frame_parms = &RC.eNB[ctxt_pP->module_id][CC_id]->frame_parms;
  memset((void *)&dl_ccch_msg,0,sizeof(LTE_DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = LTE_DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionSetup;
  rrcConnectionSetup          = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionSetup;
  LTE_MAC_MainConfig_t *mac_MainConfig = NULL;

  // RRCConnectionSetup
  // Configure SRB1

  //  *SRB_configList = CALLOC(1,sizeof(*SRB_configList));
  if (*SRB_configList) {
    free(*SRB_configList);
  }

  *SRB_configList = CALLOC(1,sizeof(LTE_SRB_ToAddModList_t));
  /// SRB1
  SRB1_config = CALLOC(1,sizeof(*SRB1_config));
  SRB1_config->srb_Identity = 1;
  SRB1_rlc_config = CALLOC(1,sizeof(*SRB1_rlc_config));
  SRB1_config->rlc_Config   = SRB1_rlc_config;
  SRB1_rlc_config->present = LTE_SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB1_rlc_config->choice.explicitValue.present=LTE_RLC_Config_PR_am;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = rrc->srb1_timer_poll_retransmit;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = rrc->srb1_poll_pdu;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = rrc->srb1_poll_byte;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = rrc->srb1_max_retx_threshold;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = rrc->srb1_timer_reordering;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = rrc->srb1_timer_status_prohibit;
  SRB1_lchan_config = CALLOC(1,sizeof(*SRB1_lchan_config));
  SRB1_config->logicalChannelConfig   = SRB1_lchan_config;
  SRB1_lchan_config->present = LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
  SRB1_ul_SpecificParameters = CALLOC(1,sizeof(*SRB1_ul_SpecificParameters));
  SRB1_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB1_ul_SpecificParameters;
  SRB1_ul_SpecificParameters->priority = 1;
  //assign_enum(&SRB1_ul_SpecificParameters->prioritisedBitRate,LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity);
  SRB1_ul_SpecificParameters->prioritisedBitRate=LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  //assign_enum(&SRB1_ul_SpecificParameters->bucketSizeDuration,LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50);
  SRB1_ul_SpecificParameters->bucketSizeDuration=LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
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
    physicalConfigDedicated2->pdsch_ConfigDedicated->p_a=   LTE_PDSCH_ConfigDedicated__p_a_dB_3;
  else
    physicalConfigDedicated2->pdsch_ConfigDedicated->p_a=   LTE_PDSCH_ConfigDedicated__p_a_dB0;

  // PUCCH
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.present=LTE_PUCCH_ConfigDedicated__ackNackRepetition_PR_release;
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.choice.release=0;

  if (carrier->sib1->tdd_Config == NULL) {
    physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode=NULL;//PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;
  } else { //TDD
    physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode= CALLOC(1,sizeof(long));
    *(physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode) =
      LTE_PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_bundling;//PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;
  }

  // Pusch_config_dedicated
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_ACK_Index = 0; // 2.00
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_RI_Index  = 0; // 1.25
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_CQI_Index = 8; // 2.25
  // UplinkPowerControlDedicated
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUSCH = 0; // 0 dB
  //assign_enum(&physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled,
  // UplinkPowerControlDedicated__deltaMCS_Enabled_en1);
  physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled= LTE_UplinkPowerControlDedicated__deltaMCS_Enabled_en1;
  physicalConfigDedicated2->uplinkPowerControlDedicated->accumulationEnabled = 1;  // TRUE
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUCCH = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->pSRS_Offset = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient = CALLOC(1,
      sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient));
  //  assign_enum(physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient,FilterCoefficient_fc4); // fc4 dB
  *physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient=LTE_FilterCoefficient_fc4; // fc4 dB
  // TPC-PDCCH-Config
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->present=LTE_TPC_PDCCH_Config_PR_setup;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_Index.present = LTE_TPC_Index_PR_indexOfFormat3;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_Index.choice.indexOfFormat3 = 1;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf=CALLOC(1,2);
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.size=2;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf[0]=0x12;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.buf[1]=0x34+ue_context_pP->local_uid;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH->choice.setup.tpc_RNTI.bits_unused=0;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->present=LTE_TPC_PDCCH_Config_PR_setup;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_Index.present = LTE_TPC_Index_PR_indexOfFormat3;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_Index.choice.indexOfFormat3 = 1;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf=CALLOC(1,2);
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.size=2;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf[0]=0x22;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.buf[1]=0x34+ue_context_pP->local_uid;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH->choice.setup.tpc_RNTI.bits_unused=0;
  /* CQI ReportConfig */
  // Aperiodic configuration
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic = CALLOC(1, sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic));
  *physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic = LTE_CQI_ReportModeAperiodic_rm30; // HLC CQI, no PMI
  physicalConfigDedicated2->cqi_ReportConfig->nomPDSCH_RS_EPRE_Offset = 0; // 0 dB (int -1...6)
  // Periodic configuration
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic = CALLOC(1, sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic));
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->present = LTE_CQI_ReportPeriodic_PR_release;
  /*
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->present = CQI_ReportPeriodic_PR_setup;
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex = 0; // n2_pucch
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_pmi_ConfigIndex = 0; // Icqi/pmi
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_FormatIndicatorPeriodic.present = CQI_ReportPeriodic__setup__cqi_FormatIndicatorPeriodic_PR_subbandCQI;  // subband CQI
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_FormatIndicatorPeriodic.choice.subbandCQI.k = 4;
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex = NULL;
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.simultaneousAckNackAndCQI = 0;
  */

  //soundingRS-UL-ConfigDedicated
  if (rrc->srs_enable[CC_id] == 1) {
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->present = LTE_SoundingRS_UL_ConfigDedicated_PR_setup;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth =
      LTE_SoundingRS_UL_ConfigDedicated__setup__srs_Bandwidth_bw0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth =
      LTE_SoundingRS_UL_ConfigDedicated__setup__srs_HoppingBandwidth_hbw0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition=0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.duration=1;

    if (carrier->sib1->tdd_Config==NULL) { // FDD
      if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
          == LTE_SoundingRS_UL_ConfigCommon_PR_setup)
        if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig!=0)
          LOG_W(RRC,"This code has been optimized for SRS Subframe Config 0, but current config is %zd. Expect undefined behaviour!\n",
                carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig);

      if (ue_context_pP->local_uid >=20)
        LOG_W(RRC,"This code has been optimized for up to 10 UEs, but current UE_id is %d. Expect undefined behaviour!\n",
              ue_context_pP->local_uid);

      //the current code will allow for 20 UEs - to be revised for more
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex=7+ue_context_pP->local_uid/2;
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb= ue_context_pP->local_uid%2;
    } else {
      if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
          == LTE_SoundingRS_UL_ConfigCommon_PR_setup)
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
      LTE_SoundingRS_UL_ConfigDedicated__setup__cyclicShift_cs0;
  }

  //AntennaInfoDedicated
  physicalConfigDedicated2->antennaInfo = CALLOC(1,sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->antennaInfo->present = LTE_PhysicalConfigDedicated__antennaInfo_PR_explicitValue;
  //assign_enum(&physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode,
  //     AntennaInfoDedicated__transmissionMode_tm2);
  LOG_D(RRC,"physicalConfigDedicated2 %p, physicalConfigDedicated2->antennaInfo %p => %d\n",physicalConfigDedicated2,physicalConfigDedicated2->antennaInfo,transmission_mode);

  switch (transmission_mode) {
    default:
      LOG_W(RRC,"At RRCConnectionSetup Transmission mode can only take values 1 or 2! Defaulting to 1!\n");

    case 1:
      physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     LTE_AntennaInfoDedicated__transmissionMode_tm1;
      break;

    case 2:
      physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     LTE_AntennaInfoDedicated__transmissionMode_tm2;
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

  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.present = LTE_AntennaInfoDedicated__ue_TransmitAntennaSelection_PR_release;
  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.release = 0;
  // SchedulingRequestConfig
  physicalConfigDedicated2->schedulingRequestConfig->present = LTE_SchedulingRequestConfig_PR_setup;

  if (carrier->sib1->tdd_Config == NULL) {
    physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 71 - ue_context_pP->local_uid/10;//ue_context_pP->local_uid;
  } else {
    switch (carrier->sib1->tdd_Config->subframeAssignment) {
      case 1:
        switch(frame_parms->N_RB_UL) {
          case 25:
            physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 15 - ue_context_pP->local_uid/4;
            break;

          case 50:
            physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 31 - ue_context_pP->local_uid/4;
            break;

          case 100:
            physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 63 - ue_context_pP->local_uid/4;
            break;
        }

        break;

      default:
        physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 71 - ue_context_pP->local_uid/10;//ue_context_pP->local_uid;
        break;
    }
  }

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
  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax = LTE_SchedulingRequestConfig__setup__dsr_TransMax_n64;
  rrcConnectionSetup->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionSetup->criticalExtensions.present = LTE_RRCConnectionSetup__criticalExtensions_PR_c1;
  rrcConnectionSetup->criticalExtensions.choice.c1.present = LTE_RRCConnectionSetup__criticalExtensions__c1_PR_rrcConnectionSetup_r8 ;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.srb_ToAddModList = *SRB_configList;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.drb_ToAddModList = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.drb_ToReleaseList = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.sps_Config = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.physicalConfigDedicated = physicalConfigDedicated2;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig = CALLOC(1,sizeof(struct LTE_RadioResourceConfigDedicated__mac_MainConfig));
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig->present = LTE_RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue;
  /* MAC MainConfig */
  mac_MainConfig = &rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig->choice.explicitValue;
  //* ul_SCH_Config *//
  mac_MainConfig->ul_SCH_Config = CALLOC(1, sizeof(*mac_MainConfig->ul_SCH_Config));
  long *maxHARQ_Tx = CALLOC(1, sizeof(long));
  *maxHARQ_Tx = LTE_MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  long *periodicBSR_Timer = CALLOC(1, sizeof(long));
  *periodicBSR_Timer = LTE_PeriodicBSR_Timer_r12_sf64; // LTE_PeriodicBSR_Timer_r12_infinity
  mac_MainConfig->ul_SCH_Config->maxHARQ_Tx = maxHARQ_Tx; // max number of UL HARQ transmission
  mac_MainConfig->ul_SCH_Config->periodicBSR_Timer = periodicBSR_Timer;
  mac_MainConfig->ul_SCH_Config->retxBSR_Timer = LTE_RetxBSR_Timer_r12_sf320; // LTE_RetxBSR_Timer_r12_sf5120  // regular BSR timer
  mac_MainConfig->ul_SCH_Config->ttiBundling = 0; // FALSE
  //* timeAlignmentTimerDedicated *//
  mac_MainConfig->timeAlignmentTimerDedicated = LTE_TimeAlignmentTimer_infinity;
  //* DRX Config *//
  mac_MainConfig->drx_Config = NULL;
  //* PHR Config *//
  mac_MainConfig->phr_Config = CALLOC(1, sizeof(*mac_MainConfig->phr_Config));
  mac_MainConfig->phr_Config->present = LTE_MAC_MainConfig__phr_Config_PR_setup;
  mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer =
    LTE_MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20; // sf20 = 20 subframes // LTE_MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_infinity
  mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer =
    LTE_MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20; // sf20 = 20 subframes // LTE_MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf1000
  mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange = LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;  // Value dB1 =1 dB, dB3 = 3 dB

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message, (void *)&dl_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message,
                                   NULL,
                                   (void *)&dl_ccch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(RRC,"RRCConnectionSetup Encoded %zd bits (%zd bytes) \n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCConnectionSetup_BR(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_eNB_ue_context_t      *const ue_context_pP,
  int                              CC_id,
  uint8_t                   *const buffer,
  const uint8_t                    transmission_mode,
  const uint8_t                    Transaction_id,
  LTE_SRB_ToAddModList_t             **SRB_configList,
  struct LTE_PhysicalConfigDedicated **physicalConfigDedicated) {
  asn_enc_rval_t enc_rval;
  eNB_RRC_INST *rrc               = RC.rrc[ctxt_pP->module_id];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[CC_id];
  long *logicalchannelgroup = NULL;
  struct LTE_SRB_ToAddMod *SRB1_config = NULL;
  struct LTE_SRB_ToAddMod__rlc_Config *SRB1_rlc_config = NULL;
  struct LTE_SRB_ToAddMod__logicalChannelConfig *SRB1_lchan_config = NULL;
  struct LTE_LogicalChannelConfig__ul_SpecificParameters *SRB1_ul_SpecificParameters = NULL;
  LTE_PhysicalConfigDedicated_t *physicalConfigDedicated2 = NULL;
  LTE_DL_CCCH_Message_t dl_ccch_msg;
  LTE_RRCConnectionSetup_t *rrcConnectionSetup = NULL;
  memset((void *)&dl_ccch_msg,0,sizeof(LTE_DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = LTE_DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionSetup;
  rrcConnectionSetup          = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionSetup;

  // RRCConnectionSetup
  // Configure SRB1

  //  *SRB_configList = CALLOC(1,sizeof(*SRB_configList));
  if (*SRB_configList) {
    free(*SRB_configList);
  }

  *SRB_configList = CALLOC(1,sizeof(LTE_SRB_ToAddModList_t));
  /// SRB1
  SRB1_config = CALLOC(1,sizeof(*SRB1_config));
  SRB1_config->srb_Identity = 1;
  SRB1_rlc_config = CALLOC(1, sizeof(*SRB1_rlc_config));
  SRB1_config->rlc_Config  = SRB1_rlc_config; // check this
  SRB1_rlc_config->present = LTE_SRB_ToAddMod__rlc_Config_PR_explicitValue;
  SRB1_rlc_config->choice.explicitValue.present = LTE_RLC_Config_PR_am;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = rrc->srb1_timer_poll_retransmit;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = rrc->srb1_poll_pdu;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = rrc->srb1_poll_byte;
  SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = rrc->srb1_max_retx_threshold;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = rrc->srb1_timer_reordering;
  SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = rrc->srb1_timer_status_prohibit;
  SRB1_lchan_config = CALLOC(1, sizeof(*SRB1_lchan_config));
  SRB1_config->logicalChannelConfig = SRB1_lchan_config;
  SRB1_lchan_config->present = LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
  SRB1_ul_SpecificParameters = CALLOC(1, sizeof(*SRB1_ul_SpecificParameters));
  SRB1_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB1_ul_SpecificParameters;
  SRB1_ul_SpecificParameters->priority = 1;
  //assign_enum(&SRB1_ul_SpecificParameters->prioritisedBitRate,LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity);
  SRB1_ul_SpecificParameters->prioritisedBitRate=LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  //assign_enum(&SRB1_ul_SpecificParameters->bucketSizeDuration,LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50);
  SRB1_ul_SpecificParameters->bucketSizeDuration=LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
  logicalchannelgroup = CALLOC(1,sizeof(long));
  *logicalchannelgroup=0;
  SRB1_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup;
  ASN_SEQUENCE_ADD(&(*SRB_configList)->list,SRB1_config);
  // PhysicalConfigDedicated
  physicalConfigDedicated2 = CALLOC(1,sizeof(*physicalConfigDedicated2));
  *physicalConfigDedicated = physicalConfigDedicated2;
  physicalConfigDedicated2->pdsch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pdsch_ConfigDedicated));
  physicalConfigDedicated2->pucch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pucch_ConfigDedicated));
  physicalConfigDedicated2->pusch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pusch_ConfigDedicated));;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH         = NULL;
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH         = NULL;
  physicalConfigDedicated2->uplinkPowerControlDedicated   = CALLOC(1,sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated));
  physicalConfigDedicated2->cqi_ReportConfig              = CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig));

  if (rrc->srs_enable[CC_id])
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated = CALLOC(1,sizeof(*physicalConfigDedicated2->soundingRS_UL_ConfigDedicated));
  else
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated = NULL;

  physicalConfigDedicated2->antennaInfo                   = CALLOC(1,sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->schedulingRequestConfig       = CALLOC(1,sizeof(*physicalConfigDedicated2->schedulingRequestConfig));

  if (carrier->p_eNB==2)
    physicalConfigDedicated2->pdsch_ConfigDedicated->p_a = LTE_PDSCH_ConfigDedicated__p_a_dB_3;
  else
    physicalConfigDedicated2->pdsch_ConfigDedicated->p_a = LTE_PDSCH_ConfigDedicated__p_a_dB0;

  // PUCCH
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.present=LTE_PUCCH_ConfigDedicated__ackNackRepetition_PR_release;
  physicalConfigDedicated2->pucch_ConfigDedicated->ackNackRepetition.choice.release=0;
  // PUSCH
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_ACK_Index = 10;
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_RI_Index = 9;
  physicalConfigDedicated2->pusch_ConfigDedicated->betaOffset_CQI_Index = 10;

  //

  if (carrier->sib1->tdd_Config == NULL) {
    physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode=NULL;//PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;
  } else { //TDD
    physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode = CALLOC(1,sizeof(long));
    *(physicalConfigDedicated2->pucch_ConfigDedicated->tdd_AckNackFeedbackMode) =
      LTE_PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_bundling;//PUCCH_ConfigDedicated__tdd_AckNackFeedbackMode_multiplexing;
  }

  /// TODO to be reviewed
  // UplinkPowerControlDedicated
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUSCH = 0; // 0 dB
  //assign_enum(&physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled,
  // UplinkPowerControlDedicated__deltaMCS_Enabled_en1);
  physicalConfigDedicated2->uplinkPowerControlDedicated->deltaMCS_Enabled= LTE_UplinkPowerControlDedicated__deltaMCS_Enabled_en1;
  physicalConfigDedicated2->uplinkPowerControlDedicated->accumulationEnabled = 1;  // TRUE
  physicalConfigDedicated2->uplinkPowerControlDedicated->p0_UE_PUCCH = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->pSRS_Offset = 0; // 0 dB
  physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient = CALLOC(1,
      sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient));
  //  assign_enum(physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient,FilterCoefficient_fc4); // fc4 dB
  *physicalConfigDedicated2->uplinkPowerControlDedicated->filterCoefficient=LTE_FilterCoefficient_fc4; // fc4 dB
  // TPC-PDCCH-Config
  // CQI ReportConfig
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic = CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic));
  *physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportModeAperiodic = LTE_CQI_ReportModeAperiodic_rm20;
  physicalConfigDedicated2->cqi_ReportConfig->nomPDSCH_RS_EPRE_Offset = 0; // 0 dB
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic = CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic));
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->present = LTE_CQI_ReportPeriodic_PR_release;
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic->choice.release = (NULL_t)0;

  /// TODO to be reviewed
  if (rrc->srs_enable[CC_id]) {
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->present = LTE_SoundingRS_UL_ConfigDedicated_PR_setup;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth =
      LTE_SoundingRS_UL_ConfigDedicated__setup__srs_Bandwidth_bw0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth =
      LTE_SoundingRS_UL_ConfigDedicated__setup__srs_HoppingBandwidth_hbw0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition=0;
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.duration=1;

    if (carrier->sib1->tdd_Config==NULL) { // FDD
      if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
          == LTE_SoundingRS_UL_ConfigCommon_PR_setup)
        if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig!=0)
          LOG_W(RRC,"This code has been optimized for SRS Subframe Config 0, but current config is %d. Expect undefined behaviour!\n",
                (int)carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig);

      if (ue_context_pP->local_uid >=20)
        LOG_W(RRC,"This code has been optimized for up to 10 UEs, but current UE_id is %d. Expect undefined behaviour!\n",
              ue_context_pP->local_uid);

      //the current code will allow for 20 UEs - to be revised for more
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex=7+ue_context_pP->local_uid/2;
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb= ue_context_pP->local_uid%2;
    } else {
      if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present
          == LTE_SoundingRS_UL_ConfigCommon_PR_setup)
        if (carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig!=7) {
          LOG_W(RRC,"This code has been optimized for SRS Subframe Config 7 and TDD config 3, but current configs are %d and %d. Expect undefined behaviour!\n",
                (int)carrier->sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig,
                (int)carrier->sib1->tdd_Config->subframeAssignment);
        }

      if (ue_context_pP->local_uid >=6)
        LOG_W(RRC,"This code has been optimized for up to 6 UEs, but current UE_id is %d. Expect undefined behaviour!\n",
              ue_context_pP->local_uid);

      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex=17+ue_context_pP->local_uid/2;
      physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb= ue_context_pP->local_uid%2;
    }

    LOG_W(RRC,"local UID %d, srs ConfigIndex %d, TransmissionComb %d\n",ue_context_pP->local_uid,
          (int)physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex,
          (int)physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb);
    physicalConfigDedicated2->soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift=
      LTE_SoundingRS_UL_ConfigDedicated__setup__cyclicShift_cs0;
  }

  //AntennaInfoDedicated
  physicalConfigDedicated2->antennaInfo = CALLOC(1,sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->antennaInfo->present = LTE_PhysicalConfigDedicated__antennaInfo_PR_explicitValue;

  switch (transmission_mode) {
    default:
      LOG_W(RRC,"At RRCConnectionSetup Transmission mode can only take values 1 or 2! Defaulting to 1!\n");

    case 1:
      physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     LTE_AntennaInfoDedicated__transmissionMode_tm1;
      break;

    case 2:
      physicalConfigDedicated2->antennaInfo->choice.explicitValue.transmissionMode=     LTE_AntennaInfoDedicated__transmissionMode_tm2;
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

  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.present = LTE_AntennaInfoDedicated__ue_TransmitAntennaSelection_PR_release;
  physicalConfigDedicated2->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.release = 0;
  // SchedulingRequestConfig
  physicalConfigDedicated2->schedulingRequestConfig->present = LTE_SchedulingRequestConfig_PR_setup;
  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex = 18;//ue_context_pP->local_uid;

  if (carrier->sib1->tdd_Config == NULL) { // FDD
    physicalConfigDedicated2->schedulingRequestConfig->choice.setup.sr_ConfigIndex = 76+(ue_context_pP->local_uid%10);  // Isr = 76 (every 80 subframes, offset=2+UE_id mod3)
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

  physicalConfigDedicated2->schedulingRequestConfig->choice.setup.dsr_TransMax = LTE_SchedulingRequestConfig__setup__dsr_TransMax_n16;
  physicalConfigDedicated2->ext4 =  calloc(1, sizeof(struct LTE_PhysicalConfigDedicated__ext4) );
  physicalConfigDedicated2->ext4->csi_RS_ConfigNZPToReleaseList_r11 = NULL;
  physicalConfigDedicated2->ext4->csi_RS_ConfigNZPToAddModList_r11 = NULL;
  physicalConfigDedicated2->ext4->csi_RS_ConfigZPToAddModList_r11 = NULL;
  physicalConfigDedicated2->ext4->csi_RS_ConfigZPToReleaseList_r11 = NULL;
  physicalConfigDedicated2->ext4->epdcch_Config_r11 = calloc(1, sizeof(struct LTE_EPDCCH_Config_r11 ));
  physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.present = LTE_EPDCCH_Config_r11__config_r11_PR_setup;
  physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.subframePatternConfig_r11 = NULL;
  physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 = calloc(1,sizeof(long));
  *physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 = 2;
  physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToReleaseList_r11 = NULL;
  physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 = calloc(1, sizeof(LTE_EPDCCH_SetConfigToAddModList_r11_t));
  //  memset(physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11, 0, sizeof())
  LTE_EPDCCH_SetConfig_r11_t *epdcch_setconfig_r11 = calloc(1, sizeof(LTE_EPDCCH_SetConfig_r11_t));
  epdcch_setconfig_r11->setConfigId_r11 = 0;
  epdcch_setconfig_r11->transmissionType_r11 = LTE_EPDCCH_SetConfig_r11__transmissionType_r11_distributed;
  epdcch_setconfig_r11->resourceBlockAssignment_r11.numberPRB_Pairs_r11 = LTE_EPDCCH_SetConfig_r11__resourceBlockAssignment_r11__numberPRB_Pairs_r11_n2;
  //epdcch_setconfig_r11->resourceBlockAssignment_r11.resourceBlockAssignment_r11 = calloc(0, sizeof(BIT_STRING_t));
  epdcch_setconfig_r11->resourceBlockAssignment_r11.resourceBlockAssignment_r11.buf = calloc(1, 5 * sizeof(uint8_t));
  epdcch_setconfig_r11->resourceBlockAssignment_r11.resourceBlockAssignment_r11.size = 5;
  epdcch_setconfig_r11->resourceBlockAssignment_r11.resourceBlockAssignment_r11.bits_unused = 2;
  memset(epdcch_setconfig_r11->resourceBlockAssignment_r11.resourceBlockAssignment_r11.buf, 0, 5 * sizeof(uint8_t));
  epdcch_setconfig_r11->dmrs_ScramblingSequenceInt_r11 = 54;
  epdcch_setconfig_r11->pucch_ResourceStartOffset_r11 = 0;
  epdcch_setconfig_r11->re_MappingQCL_ConfigId_r11 = NULL;
  epdcch_setconfig_r11->ext2 = calloc(1, sizeof(struct LTE_EPDCCH_SetConfig_r11__ext2));
  epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 = calloc(1,sizeof(struct LTE_EPDCCH_SetConfig_r11__ext2__numberPRB_Pairs_v1310));
  epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present =  LTE_EPDCCH_SetConfig_r11__ext2__numberPRB_Pairs_v1310_PR_setup;
  epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->choice.setup = LTE_EPDCCH_SetConfig_r11__ext2__numberPRB_Pairs_v1310__setup_n6;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13 = calloc(1, sizeof(struct LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13));
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->present = LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13_PR_setup;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.csi_NumRepetitionCE_r13                = LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__csi_NumRepetitionCE_r13_sf1;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_pdsch_HoppingConfig_r13         = LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__mpdcch_pdsch_HoppingConfig_r13_off;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_StartSF_UESS_r13.present        = LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__mpdcch_StartSF_UESS_r13_PR_fdd_r13;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_StartSF_UESS_r13.choice.fdd_r13 = LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__mpdcch_StartSF_UESS_r13__fdd_r13_v1;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_NumRepetition_r13               = LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__mpdcch_NumRepetition_r13_r1;
  epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13                  = 3; // note: this is narrowband index 2
  ASN_SEQUENCE_ADD(physicalConfigDedicated2->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11, epdcch_setconfig_r11);
  // FIXME allocate physicalConfigDedicated2->ext7
  physicalConfigDedicated2->ext7 = CALLOC(1, sizeof(struct LTE_PhysicalConfigDedicated__ext7));
  physicalConfigDedicated2->ext7->pdsch_ConfigDedicated_v1310 = NULL; // has some parameters to be filled
  physicalConfigDedicated2->ext7->pusch_ConfigDedicated_r13 = NULL;
  physicalConfigDedicated2->ext7->pucch_ConfigDedicated_r13 = NULL;
  physicalConfigDedicated2->ext7->pdcch_CandidateReductions_r13 = NULL;
  physicalConfigDedicated2->ext7->cqi_ReportConfig_v1310 = NULL;
  physicalConfigDedicated2->ext7->soundingRS_UL_ConfigDedicated_v1310 = NULL;
  physicalConfigDedicated2->ext7->soundingRS_UL_ConfigDedicatedUpPTsExt_r13 = NULL;
  physicalConfigDedicated2->ext7->soundingRS_UL_ConfigDedicatedAperiodic_v1310 = NULL;
  physicalConfigDedicated2->ext7->soundingRS_UL_ConfigDedicatedAperiodicUpPTsExt_r13 = NULL;
  physicalConfigDedicated2->ext7->csi_RS_Config_v1310 = NULL;
  // FIXME ce_Mode_r13 allocation
  physicalConfigDedicated2->ext7->ce_Mode_r13 = CALLOC(1, sizeof(struct LTE_PhysicalConfigDedicated__ext7__ce_Mode_r13));
  physicalConfigDedicated2->ext7->ce_Mode_r13->present      = LTE_PhysicalConfigDedicated__ext7__ce_Mode_r13_PR_setup;
  physicalConfigDedicated2->ext7->ce_Mode_r13->choice.setup = LTE_PhysicalConfigDedicated__ext7__ce_Mode_r13__setup_ce_ModeA;
  physicalConfigDedicated2->ext7->csi_RS_ConfigNZPToAddModListExt_r13 = NULL;
  physicalConfigDedicated2->ext7->csi_RS_ConfigNZPToReleaseListExt_r13 = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig = CALLOC(1,sizeof(struct LTE_RadioResourceConfigDedicated__mac_MainConfig));
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig->present = LTE_RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue;
  LTE_MAC_MainConfig_t *mac_MainConfig = &rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig->choice.explicitValue;
  mac_MainConfig->ul_SCH_Config = CALLOC(1, sizeof(*mac_MainConfig->ul_SCH_Config));
  //long *maxHARQ_Tx = CALLOC(1, sizeof(long));
  //*maxHARQ_Tx = LTE_MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  long *periodicBSR_Timer = CALLOC(1, sizeof(long));
  *periodicBSR_Timer = LTE_PeriodicBSR_Timer_r12_sf64;
  //mac_MainConfig->ul_SCH_Config->maxHARQ_Tx = maxHARQ_Tx;
  mac_MainConfig->ul_SCH_Config->periodicBSR_Timer = periodicBSR_Timer;
  mac_MainConfig->ul_SCH_Config->retxBSR_Timer = LTE_RetxBSR_Timer_r12_sf320;
  mac_MainConfig->ul_SCH_Config->ttiBundling = 0; // FALSE
  mac_MainConfig->timeAlignmentTimerDedicated = LTE_TimeAlignmentTimer_infinity;
  mac_MainConfig->drx_Config = NULL;
  mac_MainConfig->phr_Config = CALLOC(1, sizeof(*mac_MainConfig->phr_Config));
  mac_MainConfig->phr_Config->present = LTE_MAC_MainConfig__phr_Config_PR_setup;
  mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer = LTE_MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20; // sf20 = 20 subframes
  mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer = LTE_MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20; // sf20 = 20 subframes
  mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange = LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB3;  // Value dB1 =1 dB, dB3 = 3 dB
  rrcConnectionSetup->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionSetup->criticalExtensions.present = LTE_RRCConnectionSetup__criticalExtensions_PR_c1;
  rrcConnectionSetup->criticalExtensions.choice.c1.present =LTE_RRCConnectionSetup__criticalExtensions__c1_PR_rrcConnectionSetup_r8 ;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.srb_ToAddModList = *SRB_configList;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.drb_ToAddModList = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.drb_ToReleaseList = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.sps_Config = NULL;
  rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.physicalConfigDedicated = physicalConfigDedicated2;
  // rrcConnectionSetup->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated.mac_MainConfig = NULL;
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message, (void *)&dl_ccch_msg);
  }
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message,
                                   NULL,
                                   (void *)&dl_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);


  return((enc_rval.encoded+7)/8);
}




uint8_t do_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *const buffer,
  const uint8_t Transaction_id,
  const uint8_t cipheringAlgorithm,
  const uint8_t integrityProtAlgorithm
)
//------------------------------------------------------------------------------
{
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_securityModeCommand;
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.present = LTE_SecurityModeCommand__criticalExtensions_PR_c1;
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.choice.c1.present =
    LTE_SecurityModeCommand__criticalExtensions__c1_PR_securityModeCommand_r8;
  // the two following information could be based on the mod_id
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm
    = (LTE_CipheringAlgorithm_r12_t)cipheringAlgorithm;
  dl_dcch_msg.message.choice.c1.choice.securityModeCommand.criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm
    = (e_LTE_SecurityAlgorithmConfig__integrityProtAlgorithm)integrityProtAlgorithm;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_DCCH_Message, (void *)&dl_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

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
uint8_t do_UECapabilityEnquiry( const protocol_ctxt_t *const ctxt_pP,
                                uint8_t               *const buffer,
                                const uint8_t                Transaction_id,
                                int16_t              eutra_band,
                                uint32_t              nr_band)
//------------------------------------------------------------------------------
{
  NR_FreqBandList_t *nsa_band_list;
  NR_FreqBandInformation_t *nsa_band;
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  LTE_RAT_Type_t rat=LTE_RAT_Type_eutra;
  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.present = LTE_UECapabilityEnquiry__criticalExtensions_PR_c1;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.present =
    LTE_UECapabilityEnquiry__criticalExtensions__c1_PR_ueCapabilityEnquiry_r8;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.count=0;
  ASN_SEQUENCE_ADD(&dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list,
                   &rat);
  LTE_UECapabilityEnquiry_r8_IEs_t *r8 = &dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8;
  LTE_UECapabilityEnquiry_v8a0_IEs_t r8_a0;
  LTE_UECapabilityEnquiry_v1180_IEs_t r11_80;
  LTE_UECapabilityEnquiry_v1310_IEs_t r13_10;
  LTE_UECapabilityEnquiry_v1430_IEs_t r14_30;
  LTE_UECapabilityEnquiry_v1510_IEs_t r15_10;
  OCTET_STRING_t req_freq;

  if (nr_band>0) {

    /* request NR configuration */

    memset(&r8_a0, 0, sizeof(r8_a0));
    memset(&r11_80, 0, sizeof(r11_80));
    memset(&r13_10, 0, sizeof(r13_10));
    memset(&r14_30, 0, sizeof(r14_30));
    memset(&r15_10, 0, sizeof(r15_10));

    r8->nonCriticalExtension = &r8_a0;
    r8_a0.nonCriticalExtension = &r11_80;
    r11_80.nonCriticalExtension = &r13_10;
    r13_10.nonCriticalExtension = &r14_30;
    r14_30.nonCriticalExtension = &r15_10;

    /* TODO: no hardcoded values here */

    nsa_band_list = (NR_FreqBandList_t *)calloc(1, sizeof(NR_FreqBandList_t));

    nsa_band = (NR_FreqBandInformation_t *) calloc(1,sizeof(NR_FreqBandInformation_t));
    nsa_band->present = NR_FreqBandInformation_PR_bandInformationEUTRA;
    nsa_band->choice.bandInformationEUTRA = (NR_FreqBandInformationEUTRA_t *) calloc(1, sizeof(NR_FreqBandInformationEUTRA_t));
    nsa_band->choice.bandInformationEUTRA->bandEUTRA = eutra_band;
    ASN_SEQUENCE_ADD(&nsa_band_list->list, nsa_band);

    nsa_band = (NR_FreqBandInformation_t *) calloc(1,sizeof(NR_FreqBandInformation_t));
    nsa_band->present = NR_FreqBandInformation_PR_bandInformationNR;
    nsa_band->choice.bandInformationNR = (NR_FreqBandInformationNR_t *) calloc(1, sizeof(NR_FreqBandInformationNR_t));
  //if(nr_band > 0)
    nsa_band->choice.bandInformationNR->bandNR = nr_band;
  //else
  //  nsa_band->choice.bandInformationNR->bandNR = 78;
    ASN_SEQUENCE_ADD(&nsa_band_list->list, nsa_band);

  //unsigned char req_freq_buf[5] = { 0x00, 0x20, 0x1a, 0x02, 0x68 };  // bands 7 & nr78
    unsigned char req_freq_buf[1024];
    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_FreqBandList,
                                NULL,
                                (void *)nsa_band_list,
                                req_freq_buf,
                                1024);

    xer_fprint(stdout, &asn_DEF_NR_FreqBandList, (void *)nsa_band_list);



  //unsigned char req_freq_buf[5] = { 0x00, 0x20, 0x1a, 0x08, 0x18 };  // bands 7 & nr260

  //unsigned char req_freq_buf[13] = { 0x00, 0xc0, 0x18, 0x01, 0x01, 0x30, 0x4b, 0x04, 0x0e, 0x08, 0x24, 0x04, 0xd0 };
//  unsigned char req_freq_buf[21] = {
//0x01, 0x60, 0x18, 0x05, 0x80, 0xc0, 0x04, 0x04, 0xc1, 0x2c, 0x10, 0x08, 0x20, 0x30, 0x40, 0xe0, 0x82, 0x40, 0x28, 0x80, 0x9a
//  };

    req_freq.buf = req_freq_buf;
    req_freq.size = (enc_rval.encoded+7)/8;
//  req_freq.size = 21;

    r15_10.requestedFreqBandsNR_MRDC_r15 = &req_freq;
    // Add request for eutra-nr 
  }
//  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_DCCH_Message, (void *)&dl_dcch_msg);
//  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

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
uint8_t do_NR_UECapabilityEnquiry( const protocol_ctxt_t *const ctxt_pP,
                                   uint8_t               *const buffer,
                                   const uint8_t                Transaction_id,
                                   int16_t              eutra_band,
                                   uint32_t             nr_band)
//------------------------------------------------------------------------------
{
  NR_FreqBandList_t *nsa_band_list;
  NR_FreqBandInformation_t *nsa_band;
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  LTE_RAT_Type_t rat_nr=LTE_RAT_Type_nr;
  LTE_RAT_Type_t rat_eutra_nr=LTE_RAT_Type_eutra_nr;
  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.present = LTE_UECapabilityEnquiry__criticalExtensions_PR_c1;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.present =
    LTE_UECapabilityEnquiry__criticalExtensions__c1_PR_ueCapabilityEnquiry_r8;
  dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.count=0;
  ASN_SEQUENCE_ADD(&dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list,
                   &rat_nr);
  ASN_SEQUENCE_ADD(&dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list,
                   &rat_eutra_nr);

  /* request NR configuration */
  LTE_UECapabilityEnquiry_r8_IEs_t *r8 = &dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8;
  LTE_UECapabilityEnquiry_v8a0_IEs_t r8_a0;
  LTE_UECapabilityEnquiry_v1180_IEs_t r11_80;
  LTE_UECapabilityEnquiry_v1310_IEs_t r13_10;
  LTE_UECapabilityEnquiry_v1430_IEs_t r14_30;
  LTE_UECapabilityEnquiry_v1510_IEs_t r15_10;

  memset(&r8_a0, 0, sizeof(r8_a0));
  memset(&r11_80, 0, sizeof(r11_80));
  memset(&r13_10, 0, sizeof(r13_10));
  memset(&r14_30, 0, sizeof(r14_30));
  memset(&r15_10, 0, sizeof(r15_10));

  r8->nonCriticalExtension = &r8_a0;
  r8_a0.nonCriticalExtension = &r11_80;
  r11_80.nonCriticalExtension = &r13_10;
  r13_10.nonCriticalExtension = &r14_30;
  r14_30.nonCriticalExtension = &r15_10;

  /* TODO: no hardcoded values here */

  nsa_band_list = (NR_FreqBandList_t *)calloc(1, sizeof(NR_FreqBandList_t));

  nsa_band = (NR_FreqBandInformation_t *) calloc(1,sizeof(NR_FreqBandInformation_t));
  nsa_band->present = NR_FreqBandInformation_PR_bandInformationEUTRA;
  nsa_band->choice.bandInformationEUTRA = (NR_FreqBandInformationEUTRA_t *) calloc(1, sizeof(NR_FreqBandInformationEUTRA_t));
  nsa_band->choice.bandInformationEUTRA->bandEUTRA = eutra_band;
  ASN_SEQUENCE_ADD(&nsa_band_list->list, nsa_band);

  nsa_band = (NR_FreqBandInformation_t *) calloc(1,sizeof(NR_FreqBandInformation_t));
  nsa_band->present = NR_FreqBandInformation_PR_bandInformationNR;
  nsa_band->choice.bandInformationNR = (NR_FreqBandInformationNR_t *) calloc(1, sizeof(NR_FreqBandInformationNR_t));
  if(nr_band > 0)
    nsa_band->choice.bandInformationNR->bandNR = nr_band;
  else
    nsa_band->choice.bandInformationNR->bandNR = 78;
  ASN_SEQUENCE_ADD(&nsa_band_list->list, nsa_band);

  OCTET_STRING_t req_freq;
  //unsigned char req_freq_buf[5] = { 0x00, 0x20, 0x1a, 0x02, 0x68 };  // bands 7 & nr78
  unsigned char req_freq_buf[100];
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_FreqBandList,
      NULL,
      (void *)nsa_band_list,
      req_freq_buf,
      1024);

  //unsigned char req_freq_buf[5] = { 0x00, 0x20, 0x1a, 0x08, 0x18 };  // bands 7 & nr260

  //unsigned char req_freq_buf[13] = { 0x00, 0xc0, 0x18, 0x01, 0x01, 0x30, 0x4b, 0x04, 0x0e, 0x08, 0x24, 0x04, 0xd0 };
//  unsigned char req_freq_buf[21] = {
//0x01, 0x60, 0x18, 0x05, 0x80, 0xc0, 0x04, 0x04, 0xc1, 0x2c, 0x10, 0x08, 0x20, 0x30, 0x40, 0xe0, 0x82, 0x40, 0x28, 0x80, 0x9a
//  };

  req_freq.buf = req_freq_buf;
  req_freq.size = (enc_rval.encoded+7)/8;
//  req_freq.size = 21;

  r15_10.requestedFreqBandsNR_MRDC_r15 = &req_freq;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_DCCH_Message, (void *)&dl_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(RRC,"[eNB %d] NR UECapabilityRequest for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    LOG_E(RRC,"[eNB %d] ASN1 : NR UECapabilityRequest encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}


uint16_t do_RRCConnectionReconfiguration_BR(const protocol_ctxt_t        *const ctxt_pP,
    uint8_t                            *buffer,
    uint8_t                             Transaction_id,
    LTE_SRB_ToAddModList_t                 *SRB_list,
    LTE_DRB_ToAddModList_t                 *DRB_list,
    LTE_DRB_ToReleaseList_t                *DRB_list2,
    struct LTE_SPS_Config                  *sps_Config,
    struct LTE_PhysicalConfigDedicated     *physicalConfigDedicated,
    LTE_MeasObjectToAddModList_t           *MeasObj_list,
    LTE_ReportConfigToAddModList_t         *ReportConfig_list,
    LTE_QuantityConfig_t                   *quantityConfig,
    LTE_MeasIdToAddModList_t               *MeasId_list,
    LTE_MAC_MainConfig_t                   *mac_MainConfig,
    LTE_MeasGapConfig_t                    *measGapConfig,
    LTE_MobilityControlInfo_t              *mobilityInfo,
    struct LTE_MeasConfig__speedStatePars  *speedStatePars,
    LTE_RSRP_Range_t                       *rsrp,
    LTE_C_RNTI_t                           *cba_rnti,
    struct LTE_RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList,
    LTE_SCellToAddMod_r10_t  *SCell_config) {
  asn_enc_rval_t enc_rval;
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  LTE_RRCConnectionReconfiguration_t *rrcConnectionReconfiguration;
  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration;
  rrcConnectionReconfiguration          = &dl_dcch_msg.message.choice.c1.choice.rrcConnectionReconfiguration;
  // RRCConnectionReconfiguration
  rrcConnectionReconfiguration->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReconfiguration->criticalExtensions.present = LTE_RRCConnectionReconfiguration__criticalExtensions_PR_c1;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.present =LTE_RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8 ;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated = CALLOC(1,
      sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated));
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList = SRB_list;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList = DRB_list;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToReleaseList = DRB_list2;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->sps_Config = sps_Config;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->physicalConfigDedicated = physicalConfigDedicated;
  physicalConfigDedicated->cqi_ReportConfig = CALLOC(1, sizeof(struct LTE_CQI_ReportConfig));
  physicalConfigDedicated->cqi_ReportConfig->cqi_ReportModeAperiodic = CALLOC(1, sizeof(LTE_CQI_ReportModeAperiodic_t));
  *physicalConfigDedicated->cqi_ReportConfig->cqi_ReportModeAperiodic = LTE_CQI_ReportModeAperiodic_rm20;
  physicalConfigDedicated->cqi_ReportConfig->nomPDSCH_RS_EPRE_Offset = 0;

  if (mac_MainConfig!=NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig->present
      =LTE_RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue;
    memcpy(&rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue,
           mac_MainConfig,
           sizeof(*mac_MainConfig));
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig=NULL;
  }

  if (MeasId_list != NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig           = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig));
    memset((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig,
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
    memcpy((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo, (void *)mobilityInfo,
           sizeof(LTE_MobilityControlInfo_t));
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo  = NULL;
  }

  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.dedicatedInfoNASList = dedicatedInfoNASList;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.securityConfigHO     = NULL;
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   RRC_BUF_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed %s, %lu!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_DL_DCCH_Message,(void *)&dl_dcch_msg);
  }

  LOG_I(RRC,"RRCConnectionReconfiguration Encoded %d bits (%d bytes)\n",(int)enc_rval.encoded,(int)(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
/*
 * Copy the different Information Elements in the RRC structure
 */
uint16_t do_RRCConnectionReconfiguration(const protocol_ctxt_t *const ctxt_pP,
    uint8_t                                *buffer,
    uint8_t                                 Transaction_id,
    LTE_SRB_ToAddModList_t                 *SRB_list,
    LTE_DRB_ToAddModList_t                 *DRB_list,
    LTE_DRB_ToReleaseList_t                *DRB_list2,
    struct LTE_SPS_Config                  *sps_Config,
    struct LTE_PhysicalConfigDedicated     *physicalConfigDedicated,
    LTE_MeasObjectToAddModList_t           *MeasObj_list,
    LTE_ReportConfigToAddModList_t         *ReportConfig_list,
    LTE_QuantityConfig_t                   *quantityConfig,
    LTE_MeasIdToAddModList_t               *MeasId_list,
    LTE_MAC_MainConfig_t                   *mac_MainConfig,
    LTE_MeasGapConfig_t                    *measGapConfig,
    LTE_MobilityControlInfo_t              *mobilityInfo,
    LTE_SecurityConfigHO_t                 *securityConfigHO,
    struct LTE_MeasConfig__speedStatePars  *speedStatePars,
    LTE_RSRP_Range_t                       *rsrp,
    LTE_C_RNTI_t                           *cba_rnti,
    struct LTE_RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList
    *dedicatedInfoNASList,
    LTE_SL_CommConfig_r12_t                *sl_CommConfig,
    LTE_SL_DiscConfig_r12_t                *sl_DiscConfig,
    LTE_SCellToAddMod_r10_t              *SCell_config
                                        )
//------------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  LTE_RRCConnectionReconfiguration_t *rrcConnectionReconfiguration;
  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration;
  rrcConnectionReconfiguration          = &dl_dcch_msg.message.choice.c1.choice.rrcConnectionReconfiguration;
  /* RRC Connection Reconfiguration */
  rrcConnectionReconfiguration->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReconfiguration->criticalExtensions.present = LTE_RRCConnectionReconfiguration__criticalExtensions_PR_c1;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.present = LTE_RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8 ;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated = CALLOC(1,
      sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated));
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList = SRB_list;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList = DRB_list;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToReleaseList = DRB_list2;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->sps_Config = sps_Config;
  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->physicalConfigDedicated = physicalConfigDedicated;

  if (mac_MainConfig!=NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig->present
      =LTE_RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue;
    memcpy(&rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue,
           mac_MainConfig,
           sizeof(*mac_MainConfig));
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->mac_MainConfig=NULL;
  }

  if (MeasId_list != NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig           = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig));
    memset((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.measConfig,
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
    memcpy((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo, (void *)mobilityInfo,
           sizeof(LTE_MobilityControlInfo_t));
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo  = NULL;
  }

  if (securityConfigHO != NULL) {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.securityConfigHO     = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.securityConfigHO));
    memcpy((void *)rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.securityConfigHO, (void *)securityConfigHO,
           sizeof(LTE_SecurityConfigHO_t));
  } else {
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.securityConfigHO     = NULL;
  }

  rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.dedicatedInfoNASList = dedicatedInfoNASList;

  //TTN for D2D
  //allocate dedicated resource pools for SL communication (sl_CommConfig_r12)
  if (sl_CommConfig != NULL) {
    LOG_I(RRC,"[RRCConnectionReconfiguration] allocating a dedicated resource pool for SL communication \n");
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      = CALLOC(1,
               sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_CommConfig_r12
      = CALLOC(1,
               sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_CommConfig_r12));
    memcpy((void *)
           rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_CommConfig_r12,
           (void *)sl_CommConfig,
           sizeof(LTE_SL_CommConfig_r12_t));
  }

  //allocate dedicated resource pools for SL discovery (sl_DiscConfig)
  if (sl_DiscConfig != NULL) {
    LOG_I(RRC,"[RRCConnectionReconfiguration] allocating a dedicated resource pool for SL discovery \n");
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension = CALLOC(1,
        sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      = CALLOC(1,
               sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension));
    rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_DiscConfig_r12
      = CALLOC(1,
               sizeof(*rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_DiscConfig_r12));
    memcpy((void *)
           rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_DiscConfig_r12,
           (void *)sl_DiscConfig,
           sizeof(LTE_SL_DiscConfig_r12_t));
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   RRC_BUF_SIZE);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_DL_DCCH_Message,(void *)&dl_dcch_msg);
  }

  LOG_I(RRC,"RRCConnectionReconfiguration Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  // for (i=0;i<30;i++)
  //    msg("%x.",buffer[i]);
  // msg("\n");
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_RRCConnectionReestablishment(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_eNB_ue_context_t      *const ue_context_pP,
  int                              CC_id,
  uint8_t                   *const buffer,
  const uint8_t                    transmission_mode,
  const uint8_t                    Transaction_id,
  LTE_SRB_ToAddModList_t               **SRB_configList,
  struct LTE_PhysicalConfigDedicated   **physicalConfigDedicated) {
  asn_enc_rval_t enc_rval;
  long *logicalchannelgroup = NULL;
  struct LTE_SRB_ToAddMod *SRB1_config = NULL;
  struct LTE_SRB_ToAddMod *SRB2_config = NULL;
  struct LTE_SRB_ToAddMod__rlc_Config *SRB1_rlc_config = NULL;
  struct LTE_SRB_ToAddMod__logicalChannelConfig *SRB1_lchan_config = NULL;
  struct LTE_LogicalChannelConfig__ul_SpecificParameters *SRB1_ul_SpecificParameters = NULL;
  eNB_RRC_INST *rrc               = RC.rrc[ctxt_pP->module_id];
  LTE_PhysicalConfigDedicated_t *physicalConfigDedicated2 = NULL;
  LTE_DL_CCCH_Message_t dl_ccch_msg;
  LTE_RRCConnectionReestablishment_t *rrcConnectionReestablishment = NULL;
  int i = 0;
  ue_context_pP->ue_context.reestablishment_xid = Transaction_id;
  LTE_SRB_ToAddModList_t **SRB_configList2 = NULL;
  SRB_configList2 = &ue_context_pP->ue_context.SRB_configList2[Transaction_id];

  if (*SRB_configList2) {
    free(*SRB_configList2);
  }

  *SRB_configList2 = CALLOC(1, sizeof(LTE_SRB_ToAddModList_t));
  memset((void *)&dl_ccch_msg, 0, sizeof(LTE_DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = LTE_DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishment;
  rrcConnectionReestablishment          = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionReestablishment;

  // RRCConnectionReestablishment
  // Configure SRB1

  // get old configuration of SRB2
  if (*SRB_configList != NULL) {
    for (i = 0; (i < (*SRB_configList)->list.count) && (i < 3); i++) {
      LOG_D(RRC, "(*SRB_configList)->list.array[%d]->srb_Identity=%ld\n",
            i, (*SRB_configList)->list.array[i]->srb_Identity);

      if ((*SRB_configList)->list.array[i]->srb_Identity == 2 ) {
        SRB2_config = (*SRB_configList)->list.array[i];
      } else if ((*SRB_configList)->list.array[i]->srb_Identity == 1 ) {
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
    SRB1_rlc_config->present = LTE_SRB_ToAddMod__rlc_Config_PR_explicitValue;
    SRB1_rlc_config->choice.explicitValue.present=LTE_RLC_Config_PR_am;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = rrc->srb1_timer_poll_retransmit;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = rrc->srb1_poll_pdu;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = rrc->srb1_poll_byte;
    SRB1_rlc_config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = rrc->srb1_max_retx_threshold;
    SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = rrc->srb1_timer_reordering;
    SRB1_rlc_config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = rrc->srb1_timer_status_prohibit;
    SRB1_lchan_config = CALLOC(1, sizeof(*SRB1_lchan_config));
    SRB1_config->logicalChannelConfig = SRB1_lchan_config;
    SRB1_lchan_config->present = LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
    SRB1_ul_SpecificParameters = CALLOC(1, sizeof(*SRB1_ul_SpecificParameters));
    SRB1_lchan_config->choice.explicitValue.ul_SpecificParameters = SRB1_ul_SpecificParameters;
    SRB1_ul_SpecificParameters->priority = 1;
    //assign_enum(&SRB1_ul_SpecificParameters->prioritisedBitRate,LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity);
    SRB1_ul_SpecificParameters->prioritisedBitRate=LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    //assign_enum(&SRB1_ul_SpecificParameters->bucketSizeDuration,LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50);
    SRB1_ul_SpecificParameters->bucketSizeDuration=LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
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

  *SRB_configList = CALLOC(1, sizeof(LTE_SRB_ToAddModList_t));
  ASN_SEQUENCE_ADD(&(*SRB_configList)->list,SRB1_config);
  physicalConfigDedicated2 = *physicalConfigDedicated;
  rrcConnectionReestablishment->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReestablishment->criticalExtensions.present = LTE_RRCConnectionReestablishment__criticalExtensions_PR_c1;
  rrcConnectionReestablishment->criticalExtensions.choice.c1.present = LTE_RRCConnectionReestablishment__criticalExtensions__c1_PR_rrcConnectionReestablishment_r8;
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
  rrcConnectionReestablishment->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r8.nonCriticalExtension = NULL;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message, (void *)&dl_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message,
                                   NULL,
                                   (void *)&dl_ccch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_E(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(RRC,"RRCConnectionReestablishment Encoded %u bits (%u bytes)\n",
        (uint32_t)enc_rval.encoded, (uint32_t)(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t do_RRCConnectionReestablishmentReject(uint8_t                    Mod_id,
    uint8_t                   *const buffer)
//------------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  LTE_DL_CCCH_Message_t dl_ccch_msg;
  LTE_RRCConnectionReestablishmentReject_t *rrcConnectionReestablishmentReject;
  memset((void *)&dl_ccch_msg,0,sizeof(LTE_DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = LTE_DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentReject;
  rrcConnectionReestablishmentReject    = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionReestablishmentReject;
  // RRCConnectionReestablishmentReject
  rrcConnectionReestablishmentReject->criticalExtensions.present = LTE_RRCConnectionReestablishmentReject__criticalExtensions_PR_rrcConnectionReestablishmentReject_r8;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message, (void *)&dl_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message,
                                   NULL,
                                   (void *)&dl_ccch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_E(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(RRC,"RRCConnectionReestablishmentReject Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t do_RRCConnectionReject(uint8_t                    Mod_id,
                               uint8_t                   *const buffer)
//------------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  LTE_DL_CCCH_Message_t dl_ccch_msg;
  LTE_RRCConnectionReject_t *rrcConnectionReject;
  memset((void *)&dl_ccch_msg,0,sizeof(LTE_DL_CCCH_Message_t));
  dl_ccch_msg.message.present           = LTE_DL_CCCH_MessageType_PR_c1;
  dl_ccch_msg.message.choice.c1.present = LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReject;
  rrcConnectionReject                   = &dl_ccch_msg.message.choice.c1.choice.rrcConnectionReject;
  // RRCConnectionReject
  rrcConnectionReject->criticalExtensions.present = LTE_RRCConnectionReject__criticalExtensions_PR_c1;
  rrcConnectionReject->criticalExtensions.choice.c1.present = LTE_RRCConnectionReject__criticalExtensions__c1_PR_rrcConnectionReject_r8;
  /* let's put a wait time of 1s for the moment */
  rrcConnectionReject->criticalExtensions.choice.c1.choice.rrcConnectionReject_r8.waitTime = 1;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message, (void *)&dl_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message,
                                   NULL,
                                   (void *)&dl_ccch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_E(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(RRC,"RRCConnectionReject Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCConnectionRelease(uint8_t                             Mod_id,
                                uint8_t                            *buffer,
                                uint8_t                             Transaction_id) {
  asn_enc_rval_t enc_rval;
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  LTE_RRCConnectionRelease_t *rrcConnectionRelease;
  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionRelease;
  rrcConnectionRelease                  = &dl_dcch_msg.message.choice.c1.choice.rrcConnectionRelease;
  // RRCConnectionRelease
  rrcConnectionRelease->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionRelease->criticalExtensions.present = LTE_RRCConnectionRelease__criticalExtensions_PR_c1;
  rrcConnectionRelease->criticalExtensions.choice.c1.present =LTE_RRCConnectionRelease__criticalExtensions__c1_PR_rrcConnectionRelease_r8 ;
  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.releaseCause = LTE_ReleaseCause_other;
  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.redirectedCarrierInfo = NULL;
  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.idleModeMobilityControlInfo = NULL;
  rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.nonCriticalExtension=CALLOC(1,
      sizeof(*rrcConnectionRelease->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.nonCriticalExtension));
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   RRC_BUF_SIZE);
  return((enc_rval.encoded+7)/8);
}

uint8_t TMGI[5] = {4,3,2,1,0};//TMGI is a string of octet, ref. TS 24.008 fig. 10.5.4a


uint8_t do_MBSFNAreaConfig(uint8_t Mod_id,
                           uint8_t sync_area,
                           uint8_t *buffer,
                           LTE_MCCH_Message_t *mcch_message,
                           LTE_MBSFNAreaConfiguration_r9_t **mbsfnAreaConfiguration) {
  asn_enc_rval_t enc_rval;
  LTE_MBSFN_SubframeConfig_t *mbsfn_SubframeConfig1;
  LTE_PMCH_Info_r9_t *pmch_Info_1;
  LTE_MBMS_SessionInfo_r9_t *mbms_Session_1;
  // MBMS_SessionInfo_r9_t *mbms_Session_2;
  eNB_RRC_INST *rrc               = RC.rrc[Mod_id];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[0];
  memset(mcch_message,0,sizeof(LTE_MCCH_Message_t));
  mcch_message->message.present = LTE_MCCH_MessageType_PR_c1;
  mcch_message->message.choice.c1.present = LTE_MCCH_MessageType__c1_PR_mbsfnAreaConfiguration_r9;
  *mbsfnAreaConfiguration = &mcch_message->message.choice.c1.choice.mbsfnAreaConfiguration_r9;
  // Common Subframe Allocation (CommonSF-Alloc-r9)
  mbsfn_SubframeConfig1= CALLOC(1,sizeof(*mbsfn_SubframeConfig1));
  memset((void *)mbsfn_SubframeConfig1,0,sizeof(*mbsfn_SubframeConfig1));
  //
  mbsfn_SubframeConfig1->radioframeAllocationPeriod= LTE_MBSFN_SubframeConfig__radioframeAllocationPeriod_n4;
  mbsfn_SubframeConfig1->radioframeAllocationOffset= 1;
  mbsfn_SubframeConfig1->subframeAllocation.present= LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame;
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
  (*mbsfnAreaConfiguration)->commonSF_AllocPeriod_r9= LTE_MBSFNAreaConfiguration_r9__commonSF_AllocPeriod_r9_rf16;
  // PMCHs Information List (PMCH-InfoList-r9)
  // PMCH_1  Config
  pmch_Info_1 = CALLOC(1,sizeof(LTE_PMCH_Info_r9_t));
  memset((void *)pmch_Info_1,0,sizeof(LTE_PMCH_Info_r9_t));
  /*
   * take the value of last mbsfn subframe in this CSA period because there is only one PMCH in this mbsfn area
   * Note: this has to be set based on the subframeAllocation and CSA
   */
  pmch_Info_1->pmch_Config_r9.sf_AllocEnd_r9= 3;
  pmch_Info_1->pmch_Config_r9.dataMCS_r9= 7;
  pmch_Info_1->pmch_Config_r9.mch_SchedulingPeriod_r9= LTE_PMCH_Config_r9__mch_SchedulingPeriod_r9_rf16;
  // MBMSs-SessionInfoList-r9
  //  pmch_Info_1->mbms_SessionInfoList_r9 = CALLOC(1,sizeof(struct MBMS_SessionInfoList_r9));
  //  Session 1
  mbms_Session_1 = CALLOC(1,sizeof(LTE_MBMS_SessionInfo_r9_t));
  memset(mbms_Session_1,0,sizeof(LTE_MBMS_SessionInfo_r9_t));
  // TMGI value
  mbms_Session_1->tmgi_r9.plmn_Id_r9.present= LTE_TMGI_r9__plmn_Id_r9_PR_plmn_Index_r9;
  mbms_Session_1->tmgi_r9.plmn_Id_r9.choice.plmn_Index_r9= 1;
  // Service ID
  memset(&mbms_Session_1->tmgi_r9.serviceId_r9,0,sizeof(OCTET_STRING_t));// need to check
  OCTET_STRING_fromBuf(&mbms_Session_1->tmgi_r9.serviceId_r9,(const char *)&TMGI[2],3);
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

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_MCCH_Message,(void *)mcch_message);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_MCCH_Message,
                                   NULL,
                                   (void *)mcch_message,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(RRC,"[eNB] MCCH Message Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : MCCH  encoding failed for MBSFNAreaConfiguration\n");
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}


uint8_t do_MeasurementReport(uint8_t Mod_id, uint8_t *buffer,int measid,int phy_id,long rsrp_s,long rsrq_s,long rsrp_t,long rsrq_t) {
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  LTE_MeasurementReport_t  *measurementReport;
  ul_dcch_msg.message.present                     = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present           = LTE_UL_DCCH_MessageType__c1_PR_measurementReport;
  measurementReport            = &ul_dcch_msg.message.choice.c1.choice.measurementReport;
  measurementReport->criticalExtensions.present=LTE_MeasurementReport__criticalExtensions_PR_c1;
  measurementReport->criticalExtensions.choice.c1.present=LTE_MeasurementReport__criticalExtensions__c1_PR_measurementReport_r8;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.nonCriticalExtension=CALLOC(1,
      sizeof(*measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.nonCriticalExtension));
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measId=measid;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultPCell.rsrpResult=rsrp_s;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultPCell.rsrqResult=rsrq_s;
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells=CALLOC(1,
      sizeof(*measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells));
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells->present=LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA;
  LTE_MeasResultListEUTRA_t  *measResultListEUTRA2;
  measResultListEUTRA2 = CALLOC(1,sizeof(*measResultListEUTRA2));
  struct LTE_MeasResultEUTRA *measresulteutra2;
  measresulteutra2 = CALLOC(1,sizeof(*measresulteutra2));
  measresulteutra2->physCellId=phy_id;//1;
  struct LTE_MeasResultEUTRA__cgi_Info *measresult_cgi2;
  measresult_cgi2 = CALLOC(1,sizeof(*measresult_cgi2));
  memset(&measresult_cgi2->cellGlobalId.plmn_Identity,0,sizeof(measresult_cgi2->cellGlobalId.plmn_Identity));
  // measresult_cgi2->cellGlobalId.plmn_Identity.mcc=CALLOC(1,sizeof(measresult_cgi2->cellGlobalId.plmn_Identity.mcc));
  measresult_cgi2->cellGlobalId.plmn_Identity.mcc = CALLOC(1, sizeof(*measresult_cgi2->cellGlobalId.plmn_Identity.mcc));
  asn_set_empty(&measresult_cgi2->cellGlobalId.plmn_Identity.mcc->list);//.size=0;
  LTE_MCC_MNC_Digit_t dummy;
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
  struct LTE_MeasResultEUTRA__measResult meas2;
  //    int rsrp_va=10;
  meas2.rsrpResult=&(rsrp_t);
  //&rsrp_va;
  meas2.rsrqResult=&(rsrq_t);
  meas2.ext1 = NULL;
  measresulteutra2->measResult=meas2;
  ASN_SEQUENCE_ADD(&measResultListEUTRA2->list,measresulteutra2);
  measurementReport->criticalExtensions.choice.c1.choice.measurementReport_r8.measResults.measResultNeighCells->choice.measResultListEUTRA=*(measResultListEUTRA2);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message,
                                   NULL,
                                   (void *)&ul_dcch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    free(measResultListEUTRA2);
    measResultListEUTRA2 = NULL;
    return -1;
  }

  free(measResultListEUTRA2);
  measResultListEUTRA2 = NULL;
  return((enc_rval.encoded+7)/8);
}

uint8_t do_DLInformationTransfer(uint8_t Mod_id, uint8_t **buffer, uint8_t transaction_id, uint32_t pdu_length, uint8_t *pdu_buffer) {
  ssize_t encoded;
  LTE_DL_DCCH_Message_t dl_dcch_msg;
  memset(&dl_dcch_msg, 0, sizeof(LTE_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_dlInformationTransfer;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.rrc_TransactionIdentifier = transaction_id;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.present = LTE_DLInformationTransfer__criticalExtensions_PR_c1;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.present = LTE_DLInformationTransfer__criticalExtensions__c1_PR_dlInformationTransfer_r8;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.present =
    LTE_DLInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.size = pdu_length;
  dl_dcch_msg.message.choice.c1.choice.dlInformationTransfer.criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.buf = pdu_buffer;
  encoded = uper_encode_to_new_buffer (&asn_DEF_LTE_DL_DCCH_Message, NULL, (void *) &dl_dcch_msg, (void **) buffer);
  return encoded;
}

uint8_t do_Paging(uint8_t Mod_id, uint8_t *buffer, ue_paging_identity_t ue_paging_identity, cn_domain_t cn_domain) {
  LOG_D(RRC, "[eNB %d] do_Paging start\n", Mod_id);
  asn_enc_rval_t enc_rval;
  LTE_PCCH_Message_t pcch_msg;
  LTE_PagingRecord_t *paging_record_p;
  int j;
  pcch_msg.message.present           = LTE_PCCH_MessageType_PR_c1;
  pcch_msg.message.choice.c1.present = LTE_PCCH_MessageType__c1_PR_paging;
  pcch_msg.message.choice.c1.choice.paging.pagingRecordList = CALLOC(1,sizeof(*pcch_msg.message.choice.c1.choice.paging.pagingRecordList));
  pcch_msg.message.choice.c1.choice.paging.systemInfoModification = NULL;
  pcch_msg.message.choice.c1.choice.paging.etws_Indication = NULL;
  pcch_msg.message.choice.c1.choice.paging.nonCriticalExtension = NULL;
  asn_set_empty(&pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list);
  pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list.count = 0;

  if ((paging_record_p = calloc(1, sizeof(LTE_PagingRecord_t))) == NULL) {
    /* Possible error on calloc */
    return (-1);
  }

  memset(paging_record_p, 0, sizeof(LTE_PagingRecord_t));

  /* convert ue_paging_identity_t to PagingUE_Identity_t */
  if (ue_paging_identity.presenceMask == UE_PAGING_IDENTITY_s_tmsi) {
    paging_record_p->ue_Identity.present = LTE_PagingUE_Identity_PR_s_TMSI;
    MME_CODE_TO_OCTET_STRING(ue_paging_identity.choice.s_tmsi.mme_code,
                             &paging_record_p->ue_Identity.choice.s_TMSI.mmec);
    paging_record_p->ue_Identity.choice.s_TMSI.mmec.bits_unused = 0;
    M_TMSI_TO_OCTET_STRING(ue_paging_identity.choice.s_tmsi.m_tmsi,
                           &paging_record_p->ue_Identity.choice.s_TMSI.m_TMSI);
    paging_record_p->ue_Identity.choice.s_TMSI.m_TMSI.bits_unused = 0;
  } else if (ue_paging_identity.presenceMask == UE_PAGING_IDENTITY_imsi) {
    paging_record_p->ue_Identity.present = LTE_PagingUE_Identity_PR_imsi;
    LTE_IMSI_Digit_t imsi_digit[21];

    for (j = 0; j< ue_paging_identity.choice.imsi.length; j++) {  /* IMSI size */
      imsi_digit[j] = (LTE_IMSI_Digit_t)ue_paging_identity.choice.imsi.buffer[j];
      ASN_SEQUENCE_ADD(&paging_record_p->ue_Identity.choice.imsi.list, &imsi_digit[j]);
    }
  }

  /* set cn_domain */
  if (cn_domain == CN_DOMAIN_PS) {
    paging_record_p->cn_Domain = LTE_PagingRecord__cn_Domain_ps;
  } else {
    paging_record_p->cn_Domain = LTE_PagingRecord__cn_Domain_cs;
  }

  /* add to list */
  ASN_SEQUENCE_ADD(&pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list, paging_record_p);
  LOG_D(RRC, "[eNB %d] do_Paging paging_record: cn_Domain %ld, ue_paging_identity.presenceMask %d, PagingRecordList.count %d\n",
        Mod_id, paging_record_p->cn_Domain, ue_paging_identity.presenceMask, pcch_msg.message.choice.c1.choice.paging.pagingRecordList->list.count);
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_PCCH_Message, NULL, (void *)&pcch_msg, buffer, RRC_BUF_SIZE);

  if(enc_rval.encoded == -1) {
    LOG_I(RRC, "[eNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_PCCH_Message, (void *)&pcch_msg);
  }

  return((enc_rval.encoded+7)/8);
}

uint8_t do_ULInformationTransfer(uint8_t **buffer, uint32_t pdu_length, uint8_t *pdu_buffer) {
  ssize_t encoded;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  memset(&ul_dcch_msg, 0, sizeof(LTE_UL_DCCH_Message_t));
  ul_dcch_msg.message.present           = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_ulInformationTransfer;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.present = LTE_ULInformationTransfer__criticalExtensions_PR_c1;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.present = LTE_DLInformationTransfer__criticalExtensions__c1_PR_dlInformationTransfer_r8;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.present =
    LTE_ULInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.size = pdu_length;
  ul_dcch_msg.message.choice.c1.choice.ulInformationTransfer.criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.choice.dedicatedInfoNAS.buf = pdu_buffer;
  encoded = uper_encode_to_new_buffer (&asn_DEF_LTE_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, (void **) buffer);
  return encoded;
}

int do_HandoverPreparation(char *ho_buf, int ho_size, LTE_UE_EUTRA_Capability_t *ue_eutra_cap, int rrc_size) {
  asn_enc_rval_t enc_rval;
  LTE_HandoverPreparationInformation_t ho;
  LTE_HandoverPreparationInformation_r8_IEs_t *ho_info;
  LTE_UE_CapabilityRAT_Container_t *ue_cap_rat_container;
  char rrc_buf[rrc_size];
  memset(rrc_buf, 0, rrc_size);
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UE_EUTRA_Capability,
                                   NULL,
                                   ue_eutra_cap,
                                   rrc_buf,
                                   rrc_size);
  /* TODO: free the OCTET_STRING */
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  memset(&ho, 0, sizeof(ho));
  ho.criticalExtensions.present = LTE_HandoverPreparationInformation__criticalExtensions_PR_c1;
  ho.criticalExtensions.choice.c1.present = LTE_HandoverPreparationInformation__criticalExtensions__c1_PR_handoverPreparationInformation_r8;
  ho_info = &ho.criticalExtensions.choice.c1.choice.handoverPreparationInformation_r8;
  {
    ue_cap_rat_container = (LTE_UE_CapabilityRAT_Container_t *)calloc(1,sizeof(LTE_UE_CapabilityRAT_Container_t));
    ue_cap_rat_container->rat_Type = LTE_RAT_Type_eutra;
    AssertFatal (OCTET_STRING_fromBuf(
                   &ue_cap_rat_container->ueCapabilityRAT_Container,
                   rrc_buf, rrc_size) != -1, "fatal: OCTET_STRING_fromBuf failed\n");
    ASN_SEQUENCE_ADD(&ho_info->ue_RadioAccessCapabilityInfo.list, ue_cap_rat_container);
  }
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_HandoverPreparationInformation,
                                   NULL,
                                   &ho,
                                   ho_buf,
                                   ho_size);
  /* TODO: free the OCTET_STRING */
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  return((enc_rval.encoded+7)/8);
}

int do_HandoverCommand(char *ho_buf, int ho_size, char *rrc_buf, int rrc_size) {
  asn_enc_rval_t enc_rval;
  LTE_HandoverCommand_t ho;
  memset(&ho, 0, sizeof(ho));
  ho.criticalExtensions.present = LTE_HandoverCommand__criticalExtensions_PR_c1;
  ho.criticalExtensions.choice.c1.present = LTE_HandoverCommand__criticalExtensions__c1_PR_handoverCommand_r8;
  AssertFatal (OCTET_STRING_fromBuf(
                 &ho.criticalExtensions.choice.c1.choice.handoverCommand_r8.handoverCommandMessage,
                 rrc_buf, rrc_size) != -1, "fatal: OCTET_STRING_fromBuf failed\n");
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_HandoverCommand,
                                   NULL,
                                   &ho,
                                   ho_buf,
                                   ho_size);
  /* TODO: free the OCTET_STRING */
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  return((enc_rval.encoded+7)/8);
}

OAI_UECapability_t *fill_ue_capability(char *UE_EUTRA_Capability_xer_fname) {
  static OAI_UECapability_t UECapability; /* TODO declared static to allow returning this has an address should be allocated in a cleaner way. */
  static LTE_SupportedBandEUTRA_t Bandlist[4]; // the macro ASN_SEQUENCE_ADD() does not copy the source, but only stores a reference to it
  static LTE_InterFreqBandInfo_t InterFreqBandInfo[4][4]; // the macro ASN_SEQUENCE_ADD() does not copy the source, but only stores a reference to it
  static LTE_BandInfoEUTRA_t BandInfoEUTRA[4]; // the macro ASN_SEQUENCE_ADD() does not copy the source, but only stores a reference to it
  asn_enc_rval_t enc_rval;
  asn_dec_rval_t dec_rval;
  long maxNumberROHC_ContextSessions = LTE_PDCP_Parameters__maxNumberROHC_ContextSessions_cs16;
  int i;
  LTE_UE_EUTRA_Capability_t *UE_EUTRA_Capability;
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
    memset((void *)InterFreqBandInfo, 0, sizeof(InterFreqBandInfo));
    memset((void *)BandInfoEUTRA, 0, sizeof(BandInfoEUTRA));
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
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0001_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0002_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0003_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0004_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0006_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0101_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0102_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0103_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.supportedROHC_Profiles.profile0x0104_r15=0;
    UE_EUTRA_Capability->pdcp_Parameters.maxNumberROHC_ContextSessions = &maxNumberROHC_ContextSessions;
    UE_EUTRA_Capability->phyLayerParameters.ue_TxAntennaSelectionSupported = 0;
    UE_EUTRA_Capability->phyLayerParameters.ue_SpecificRefSigsSupported    = 0;
    UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.count                          = 0;
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void *)&Bandlist[0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void *)&Bandlist[1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void *)&Bandlist[2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list,(void *)&Bandlist[3]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void *)&BandInfoEUTRA[0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void *)&BandInfoEUTRA[1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void *)&BandInfoEUTRA[2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list,(void *)&BandInfoEUTRA[3]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void *)&InterFreqBandInfo[0][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void *)&InterFreqBandInfo[0][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void *)&InterFreqBandInfo[0][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[0]->interFreqBandList.list,(void *)&InterFreqBandInfo[0][3]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void *)&InterFreqBandInfo[1][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void *)&InterFreqBandInfo[1][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void *)&InterFreqBandInfo[1][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[1]->interFreqBandList.list,(void *)&InterFreqBandInfo[1][3]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void *)&InterFreqBandInfo[2][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void *)&InterFreqBandInfo[2][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void *)&InterFreqBandInfo[2][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[2]->interFreqBandList.list,(void *)&InterFreqBandInfo[2][3]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void *)&InterFreqBandInfo[3][0]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void *)&InterFreqBandInfo[3][1]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void *)&InterFreqBandInfo[3][2]);
    ASN_SEQUENCE_ADD(&UE_EUTRA_Capability->measParameters.bandListEUTRA.list.array[3]->interFreqBandList.list,(void *)&InterFreqBandInfo[3][3]);

    // UE_EUTRA_Capability->measParameters.bandListEUTRA.list.count                         = 0;  // no measurements on other bands
    // UE_EUTRA_Capability->featureGroupIndicators  // null

    if (get_softmodem_params()->usim_test == 1) {
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
    FILE *f = fopen(UE_EUTRA_Capability_xer_fname, "r");
    assert(f);
    size = fread(UE_EUTRA_Capability_xer, 1, sizeof UE_EUTRA_Capability_xer, f);
    fclose(f);

    if (size == 0 || size == sizeof UE_EUTRA_Capability_xer) {
      LOG_E(RRC,"UE Capabilities XER file %s is too large\n", UE_EUTRA_Capability_xer_fname);
      free( UE_EUTRA_Capability);
      return(NULL);
    }

    dec_rval = xer_decode(0, &asn_DEF_LTE_UE_EUTRA_Capability, (void *)UE_EUTRA_Capability, UE_EUTRA_Capability_xer, size);
    assert(dec_rval.code == RC_OK);
  }

  UECapability.UE_EUTRA_Capability = UE_EUTRA_Capability;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_UE_EUTRA_Capability,(void *)UE_EUTRA_Capability);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UE_EUTRA_Capability,
                                   NULL,
                                   (void *)UE_EUTRA_Capability,
                                   &UECapability.sdu[0],
                                   MAX_UE_CAPABILITY_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
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

