/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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
* \author Raymond Knopp, Navid Nikaein and Michele Paffetti
* \date 2011, 2017
* \version 1.0
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr, michele.paffetti@studio.unibo.it
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



//#include for NB-IoT-------------------
#include "LTE_RRCConnectionRequest-NB.h"
#include "LTE_BCCH-DL-SCH-Message-NB.h"
#include "LTE_UL-CCCH-Message-NB.h"
#include "LTE_UL-DCCH-Message-NB.h"
#include "LTE_DL-CCCH-Message-NB.h"
#include "LTE_DL-DCCH-Message-NB.h"
#include "LTE_EstablishmentCause-NB-r13.h"
#include "LTE_RRCConnectionSetup-NB.h"
#include "LTE_SRB-ToAddModList-NB-r13.h"
#include "LTE_DRB-ToAddModList-NB-r13.h"
#include "RRC/LTE/defs_NB_IoT.h"
#include "LTE_RRCConnectionSetupComplete-NB.h"
#include "LTE_RRCConnectionReconfigurationComplete-NB.h"
#include "LTE_RRCConnectionReconfiguration-NB.h"
#include "LTE_MasterInformationBlock-NB.h"
#include "LTE_SystemInformation-NB.h"
#include "LTE_SystemInformationBlockType1.h"
#include "LTE_SIB-Type-NB-r13.h"
#include "LTE_RRCConnectionResume-NB.h"
#include "LTE_RRCConnectionReestablishment-NB.h"
#include "../defs_NB_IoT.h"
//----------------------------------------

//#include "PHY/defs.h"
#include "enb_config.h"

#if defined(ENABLE_ITTI)
  #include "intertask_interface.h"
#endif




/*do_MIB_NB_NB_IoT*/
uint8_t do_MIB_NB_IoT(
  rrc_eNB_carrier_data_NB_IoT_t *carrier,
  uint16_t N_RB_DL,//may not needed--> for NB_IoT only 1 PRB is used
  uint32_t frame,
  uint32_t hyper_frame) {
  asn_enc_rval_t enc_rval;
  LTE_BCCH_BCH_Message_NB_t *mib_NB_IoT = &(carrier->mib_NB_IoT);
  /*
   * systemFrameNumber-MSB: (TS 36.331 pag 576)
   * define the 4 MSB of the SFN (10 bits). The last significant 6 bits will be acquired implicitly by decoding the NPBCH
   * NOTE: 6 LSB will be used for counting the 64 radio frames in the TTI period (640 ms) that is exactly the MIB period
   *
   * hyperSFN-LSB:
   * indicates the 2 least significant bits of the HSFN. The remaining 8 bits are present in SIB1-NB
   * NOTE: with the 2 bits we count the 4 HSFN (is 1 SIB1-Nb modification period) while the other 6 count the number of modification periods
   *
   *
   * NOTE: in OAI never modify the SIB messages!!??
   */
  //XXX check if correct the bit assignment
  uint8_t sfn_MSB = (uint8_t)((frame>>6) & 0x0f); // all the 4 bits are set to 1
  uint8_t hsfn_LSB = (uint8_t)(hyper_frame & 0x03); //2 bits set to 1 (0x3 = 0011)
  uint16_t spare=0; //11 bits --> use uint16
  mib_NB_IoT->message.systemFrameNumber_MSB_r13.buf = &sfn_MSB;
  mib_NB_IoT->message.systemFrameNumber_MSB_r13.size = 1; //if expressed in byte
  mib_NB_IoT->message.systemFrameNumber_MSB_r13.bits_unused = 4; //is byte based (so how many bits you don't use of the 8 bits of a bite
  mib_NB_IoT->message.hyperSFN_LSB_r13.buf= &hsfn_LSB;
  mib_NB_IoT->message.hyperSFN_LSB_r13.size= 1;
  mib_NB_IoT->message.hyperSFN_LSB_r13.bits_unused = 6;
  //XXX to be set??
  mib_NB_IoT->message.spare.buf = (uint8_t *)&spare;
  mib_NB_IoT->message.spare.size = 2;
  mib_NB_IoT->message.spare.bits_unused = 5;
  //decide how to set it
  mib_NB_IoT->message.schedulingInfoSIB1_r13 =11; //see TS 36.213-->tables 16.4.1.3-3 ecc...
  mib_NB_IoT->message.systemInfoValueTag_r13= 0;
  mib_NB_IoT->message.ab_Enabled_r13 = 0;
  //to be decided
  mib_NB_IoT->message.operationModeInfo_r13.present = LTE_MasterInformationBlock_NB__operationModeInfo_r13_PR_inband_SamePCI_r13;
  mib_NB_IoT->message.operationModeInfo_r13.choice.inband_SamePCI_r13.eutra_CRS_SequenceInfo_r13 = 0;
  printf("[MIB] Initialization of frame information,sfn_MSB %x, hsfn_LSB %x\n",
         (uint32_t)sfn_MSB,
         (uint32_t)hsfn_LSB);
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_BCH_Message_NB,
                                   NULL,
                                   (void *)mib_NB_IoT,
                                   carrier->MIB_NB_IoT,
                                   100);

  if(enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

/*do_SIB1_NB*/
uint8_t do_SIB1_NB_IoT(uint8_t Mod_id, int CC_id,
                       rrc_eNB_carrier_data_NB_IoT_t *carrier,
                       NbIoTRrcConfigurationReq *configuration,
                       uint32_t frame
                      ) {
  LTE_BCCH_DL_SCH_Message_NB_t *bcch_message= &(carrier->siblock1_NB_IoT);
  LTE_SystemInformationBlockType1_NB_t **sib1_NB_IoT= &(carrier->sib1_NB_IoT);
  asn_enc_rval_t enc_rval;
  LTE_PLMN_IdentityInfo_NB_r13_t PLMN_identity_info_NB_IoT;
  LTE_MCC_MNC_Digit_t dummy_mcc[3],dummy_mnc[3];
  LTE_SchedulingInfo_NB_r13_t *schedulingInfo_NB_IoT;
  LTE_SIB_Type_NB_r13_t *sib_type_NB_IoT;
  long *attachWithoutPDN_Connectivity = NULL;
  attachWithoutPDN_Connectivity = CALLOC(1,sizeof(long));
  long *nrs_CRS_PowerOffset=NULL;
  nrs_CRS_PowerOffset = CALLOC(1, sizeof(long));
  long *eutraControlRegionSize=NULL; //this parameter should be set only if we are considering in-band operating mode (samePCI or differentPCI)
  eutraControlRegionSize = CALLOC(1,sizeof(long));
  long systemInfoValueTagSI = 0;
  memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_NB_t));
  bcch_message->message.present = LTE_BCCH_DL_SCH_MessageType_NB_PR_c1;
  bcch_message->message.choice.c1.present = LTE_BCCH_DL_SCH_MessageType_NB__c1_PR_systemInformationBlockType1_r13;
  //allocation
  *sib1_NB_IoT = &bcch_message->message.choice.c1.choice.systemInformationBlockType1_r13;
  /*TS 36.331 v14.2.0 pag 589
   * hyperSFN-MSB
   * Indicates the 8 most significant bits of the hyper-SFN. Together with the hyper-LSB in MIB-NB the complete HSFN is build up
   */
  //FIXME see if correct
  uint8_t hyperSFN_MSB = (uint8_t) ((frame>>2)& 0xff);
  //XXX to be checked
  (*sib1_NB_IoT)->hyperSFN_MSB_r13.buf = &hyperSFN_MSB;
  (*sib1_NB_IoT)->hyperSFN_MSB_r13.size = 1;
  (*sib1_NB_IoT)->hyperSFN_MSB_r13.bits_unused = 0;
  memset(&PLMN_identity_info_NB_IoT,0,sizeof(LTE_PLMN_IdentityInfo_NB_r13_t));
  PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc = CALLOC(1,sizeof(*PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc));
  memset(PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc,0,sizeof(*PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc));
  asn_set_empty(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc->list);//.size=0;
  //left as it is???
#if defined(ENABLE_ITTI)
  dummy_mcc[0] = (configuration->mcc / 100) % 10;
  dummy_mcc[1] = (configuration->mcc / 10) % 10;
  dummy_mcc[2] = (configuration->mcc / 1) % 10;
#else
  dummy_mcc[0] = 0;
  dummy_mcc[1] = 0;
  dummy_mcc[2] = 1;
#endif
  ASN_SEQUENCE_ADD(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc->list,&dummy_mcc[0]);
  ASN_SEQUENCE_ADD(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc->list,&dummy_mcc[1]);
  ASN_SEQUENCE_ADD(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mcc->list,&dummy_mcc[2]);
  PLMN_identity_info_NB_IoT.plmn_Identity_r13.mnc.list.size=0;
  PLMN_identity_info_NB_IoT.plmn_Identity_r13.mnc.list.count=0;
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
  ASN_SEQUENCE_ADD(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mnc.list,&dummy_mnc[0]);
  ASN_SEQUENCE_ADD(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mnc.list,&dummy_mnc[1]);

  if (dummy_mnc[2] != 0xf) {
    ASN_SEQUENCE_ADD(&PLMN_identity_info_NB_IoT.plmn_Identity_r13.mnc.list,&dummy_mnc[2]);
  }

  //still set to "notReserved" as in the previous case
  PLMN_identity_info_NB_IoT.cellReservedForOperatorUse_r13=LTE_PLMN_IdentityInfo_NB_r13__cellReservedForOperatorUse_r13_notReserved;
  *attachWithoutPDN_Connectivity = 0;
  PLMN_identity_info_NB_IoT.attachWithoutPDN_Connectivity_r13 = attachWithoutPDN_Connectivity;
  ASN_SEQUENCE_ADD(&(*sib1_NB_IoT)->cellAccessRelatedInfo_r13.plmn_IdentityList_r13.list,&PLMN_identity_info_NB_IoT);
  // 16 bits = 2 byte
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.buf = MALLOC(2); //MALLOC works in byte
  //lefts as it is?
#if defined(ENABLE_ITTI)
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.buf[0] = (configuration->tac >> 8) & 0xff;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.buf[1] = (configuration->tac >> 0) & 0xff;
#else
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.buf[0] = 0x00;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.buf[1] = 0x01;
#endif
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.size=2;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.trackingAreaCode_r13.bits_unused=0;
  // 28 bits --> i have to use 32 bits = 4 byte
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf = MALLOC(8); // why allocate 8 byte?
#if defined(ENABLE_ITTI)
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[0] = (configuration->cell_identity >> 20) & 0xff;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[1] = (configuration->cell_identity >> 12) & 0xff;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[2] = (configuration->cell_identity >>  4) & 0xff;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[3] = (configuration->cell_identity <<  4) & 0xf0;
#else
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[0] = 0x00;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[1] = 0x00;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[2] = 0x00;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.buf[3] = 0x10;
#endif
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.size=4;
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellIdentity_r13.bits_unused=4;
  //Still set to "notBarred" as in the previous case
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.cellBarred_r13=LTE_SystemInformationBlockType1_NB__cellAccessRelatedInfo_r13__cellBarred_r13_notBarred;
  //Still Set to "notAllowed" like in the previous case
  (*sib1_NB_IoT)->cellAccessRelatedInfo_r13.intraFreqReselection_r13=LTE_SystemInformationBlockType1_NB__cellAccessRelatedInfo_r13__intraFreqReselection_r13_notAllowed;
  (*sib1_NB_IoT)->cellSelectionInfo_r13.q_RxLevMin_r13=-65; //which value?? TS 36.331 V14.2.1 pag. 589
  (*sib1_NB_IoT)->cellSelectionInfo_r13.q_QualMin_r13 = 0; //FIXME new parameter for SIB1-NB, not present in SIB1 (for cell reselection but if not used the UE should apply the default value)
  (*sib1_NB_IoT)->p_Max_r13 = CALLOC(1, sizeof(LTE_P_Max_t));
  *((*sib1_NB_IoT)->p_Max_r13) = 23;
  //FIXME
  (*sib1_NB_IoT)->freqBandIndicator_r13 =
#if defined(ENABLE_ITTI)
    configuration->eutra_band;
#else
    5; //if not configured we use band 5 (UL: 824 MHz - 849MHz / DL: 869 MHz - 894 MHz  FDD mode)
#endif
  //OPTIONAL new parameters, to be used?
  /*
   * freqBandInfo_r13
   * multiBandInfoList_r13
   * nrs_CRS_PowerOffset_r13
   * sib1_NB_IoT->downlinkBitmap_r13.choice.subframePattern10_r13 =(is a BIT_STRING)
   */
  (*sib1_NB_IoT)->downlinkBitmap_r13 = CALLOC(1, sizeof(struct LTE_DL_Bitmap_NB_r13));
  ((*sib1_NB_IoT)->downlinkBitmap_r13)->present= LTE_DL_Bitmap_NB_r13_PR_NOTHING;
  *eutraControlRegionSize = 1;
  (*sib1_NB_IoT)->eutraControlRegionSize_r13 = eutraControlRegionSize;
  *nrs_CRS_PowerOffset= 0;
  (*sib1_NB_IoT)->nrs_CRS_PowerOffset_r13 = nrs_CRS_PowerOffset;
  schedulingInfo_NB_IoT = (LTE_SchedulingInfo_NB_r13_t *) malloc (3*sizeof(LTE_SchedulingInfo_NB_r13_t));
  sib_type_NB_IoT = (LTE_SIB_Type_NB_r13_t *) malloc (3*sizeof(LTE_SIB_Type_NB_r13_t));
  memset(&schedulingInfo_NB_IoT[0],0,sizeof(LTE_SchedulingInfo_NB_r13_t));
  memset(&schedulingInfo_NB_IoT[1],0,sizeof(LTE_SchedulingInfo_NB_r13_t));
  memset(&schedulingInfo_NB_IoT[2],0,sizeof(LTE_SchedulingInfo_NB_r13_t));
  memset(&sib_type_NB_IoT[0],0,sizeof(LTE_SIB_Type_NB_r13_t));
  memset(&sib_type_NB_IoT[1],0,sizeof(LTE_SIB_Type_NB_r13_t));
  memset(&sib_type_NB_IoT[2],0,sizeof(LTE_SIB_Type_NB_r13_t));
  // Now, follow the scheduler SIB configuration
  // There is only one sib2+sib3 common setting
  schedulingInfo_NB_IoT[0].si_Periodicity_r13=LTE_SchedulingInfo_NB_r13__si_Periodicity_r13_rf4096;
  schedulingInfo_NB_IoT[0].si_RepetitionPattern_r13=
    LTE_SchedulingInfo_NB_r13__si_RepetitionPattern_r13_every2ndRF; //This Indicates the starting radio frames within the SI window used for SI message transmission.
  schedulingInfo_NB_IoT[0].si_TB_r13= LTE_SchedulingInfo_NB_r13__si_TB_r13_b680;//208 bits
  // This is for SIB2/3
  /*SIB3 --> There is no mapping information of SIB2 since it is always present
    *  in the first SystemInformation message
    * listed in the schedulingInfoList list.
    * */
  sib_type_NB_IoT[0]=LTE_SIB_Type_NB_r13_sibType3_NB_r13;
  ASN_SEQUENCE_ADD(&schedulingInfo_NB_IoT[0].sib_MappingInfo_r13.list,&sib_type_NB_IoT[0]);
  ASN_SEQUENCE_ADD(&(*sib1_NB_IoT)->schedulingInfoList_r13.list,&schedulingInfo_NB_IoT[0]);
  //printf("[ASN Debug] SI P: %ld\n",(*sib1_NB_IoT)->schedulingInfoList_r13.list.array[0]->si_Periodicity_r13);
#if defined(ENABLE_ITTI)

  if (configuration->frame_type == TDD)
#endif
  {
    //FIXME in NB-IoT mandatory to be FDD --> so must give an error
    LOG_E(RRC,"[NB-IoT %d] Frame Type is TDD --> not supported by NB-IoT, exiting\n", Mod_id); //correct?
    exit(-1);
  }

  //FIXME which value chose for the following parameter
  (*sib1_NB_IoT)->si_WindowLength_r13=LTE_SystemInformationBlockType1_NB__si_WindowLength_r13_ms160;
  (*sib1_NB_IoT)->si_RadioFrameOffset_r13= 0;
  /*In Nb-IoT change/update of specific SI message can additionally be indicated by a SI message specific value tag
   * systemInfoValueTagSI (there is no SystemInfoValueTag in SIB1-NB but only in MIB-NB)
   *contained in systemInfoValueTagList_r13
   **/
  //FIXME correct?
  (*sib1_NB_IoT)->systemInfoValueTagList_r13 = CALLOC(1, sizeof(struct LTE_SystemInfoValueTagList_NB_r13));
  asn_set_empty(&(*sib1_NB_IoT)->systemInfoValueTagList_r13->list);
  ASN_SEQUENCE_ADD(&(*sib1_NB_IoT)->systemInfoValueTagList_r13->list,&systemInfoValueTagSI);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message_NB, (void *)bcch_message);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message_NB,
                                   NULL,
                                   (void *)bcch_message,
                                   carrier->SIB1_NB_IoT,
                                   100);

  if (enc_rval.encoded > 0) {
    LOG_E(RRC,"ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

#ifdef USER_MODE
  LOG_D(RRC,"[NB-IoT] SystemInformationBlockType1-NB Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
#endif

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

/*SIB23_NB_IoT*/
//to be clarified is it is possible to carry SIB2 and SIB3  in the same SI message for NB-IoT?
uint8_t do_SIB23_NB_IoT(uint8_t Mod_id,
                        int CC_id,
                        rrc_eNB_carrier_data_NB_IoT_t *carrier,//MP: this is already a carrier[CC_id]
                        NbIoTRrcConfigurationReq *configuration ) { //openair2/COMMON/rrc_messages_types.h
  struct LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member *sib2_NB_part;
  struct LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member *sib3_NB_part;
  LTE_BCCH_DL_SCH_Message_NB_t *bcch_message = &(carrier->systemInformation_NB_IoT); //is the systeminformation-->BCCH_DL_SCH_Message_NB
  LTE_SystemInformationBlockType2_NB_r13_t *sib2_NB_IoT;
  LTE_SystemInformationBlockType3_NB_r13_t *sib3_NB_IoT;
  asn_enc_rval_t enc_rval;
  LTE_RACH_Info_NB_r13_t rach_Info_NB_IoT;
  LTE_NPRACH_Parameters_NB_r13_t *nprach_parameters;
  //optional
  long *connEstFailOffset = NULL;
  connEstFailOffset = CALLOC(1, sizeof(long));
  //  RSRP_ThresholdsNPRACH_InfoList_NB_r13_t *rsrp_ThresholdsPrachInfoList;
  //  RSRP_Range_t rsrp_range;
  LTE_ACK_NACK_NumRepetitions_NB_r13_t ack_nack_repetition;
  struct LTE_NPUSCH_ConfigCommon_NB_r13__dmrs_Config_r13 *dmrs_config;
  struct LTE_DL_GapConfig_NB_r13  *dl_Gap;
  long *srs_SubframeConfig;
  srs_SubframeConfig= CALLOC(1, sizeof(long));

  if (bcch_message) {
    memset(bcch_message,0,sizeof(LTE_BCCH_DL_SCH_Message_NB_t));
  } else {
    LOG_E(RRC,"[NB-IoT %d] BCCH_MESSAGE_NB is null, exiting\n", Mod_id);
    exit(-1);
  }

  //before schould be allocated memory somewhere?
  //  if (!carrier->sib2_NB_IoT) {
  //    LOG_E(RRC,"[NB-IoT %d] sib2_NB_IoT is null, exiting\n", Mod_id);
  //    exit(-1);
  //  }
  //
  //  if (!carrier->sib3_NB_IoT) {
  //    LOG_E(RRC,"[NB-IoT %d] sib3_NB_IoT is null, exiting\n", Mod_id);
  //    exit(-1);
  //  }
  LOG_I(RRC,"[NB-IoT %d] Configuration SIB2/3\n", Mod_id);
  sib2_NB_part = CALLOC(1,sizeof(struct LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member));
  sib3_NB_part = CALLOC(1,sizeof(struct LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member));
  memset(sib2_NB_part,0,sizeof(struct LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member));
  memset(sib3_NB_part,0,sizeof(struct LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member));
  sib2_NB_part->present = LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member_PR_sib2_r13;
  sib3_NB_part->present = LTE_SystemInformation_NB_r13_IEs__sib_TypeAndInfo_r13__Member_PR_sib3_r13;
  //may bug if not correct allocation of memory
  carrier->sib2_NB_IoT = &sib2_NB_part->choice.sib2_r13;
  carrier->sib3_NB_IoT = &sib3_NB_part->choice.sib3_r13;
  sib2_NB_IoT = carrier->sib2_NB_IoT;
  sib3_NB_IoT = carrier->sib3_NB_IoT;
  nprach_parameters = (LTE_NPRACH_Parameters_NB_r13_t *) malloc (3*sizeof(LTE_NPRACH_Parameters_NB_r13_t));
  memset(&nprach_parameters[0],0,sizeof(LTE_NPRACH_Parameters_NB_r13_t));
  memset(&nprach_parameters[1],0,sizeof(LTE_NPRACH_Parameters_NB_r13_t));
  memset(&nprach_parameters[2],0,sizeof(LTE_NPRACH_Parameters_NB_r13_t));
  /// SIB2-NB-----------------------------------------
  //Barring is manage by ab-Enabled in MIB-NB (but is not a struct as ac-BarringInfo in LTE legacy)
  //RACH Config. Common--------------------------------------------------------------
  sib2_NB_IoT->radioResourceConfigCommon_r13.rach_ConfigCommon_r13.preambleTransMax_CE_r13 =
    configuration->rach_preambleTransMax_CE_NB;
  sib2_NB_IoT->radioResourceConfigCommon_r13.rach_ConfigCommon_r13.powerRampingParameters_r13.powerRampingStep =
    configuration->rach_powerRampingStep_NB;
  sib2_NB_IoT->radioResourceConfigCommon_r13.rach_ConfigCommon_r13.powerRampingParameters_r13.preambleInitialReceivedTargetPower =
    configuration->rach_preambleInitialReceivedTargetPower_NB;
  rach_Info_NB_IoT.ra_ResponseWindowSize_r13 = configuration->rach_raResponseWindowSize_NB;
  rach_Info_NB_IoT.mac_ContentionResolutionTimer_r13 = configuration-> rach_macContentionResolutionTimer_NB;
  //rach_infoList max size = maxNPRACH-Resources-NB-r13 = 3
  ASN_SEQUENCE_ADD(&sib2_NB_IoT->radioResourceConfigCommon_r13.rach_ConfigCommon_r13.rach_InfoList_r13.list,&rach_Info_NB_IoT);
  //TS 36.331 pag 614 --> if not present the value to infinity sould be used
  *connEstFailOffset = 0;
  sib2_NB_IoT->radioResourceConfigCommon_r13.rach_ConfigCommon_r13.connEstFailOffset_r13 = connEstFailOffset; /*OPTIONAL*/
  // BCCH-Config-NB-IoT----------------------------------------------------------------
  sib2_NB_IoT->radioResourceConfigCommon_r13.bcch_Config_r13.modificationPeriodCoeff_r13
    = configuration->bcch_modificationPeriodCoeff_NB;
  // PCCH-Config-NB-IoT-----------------------------------------------------------------
  sib2_NB_IoT->radioResourceConfigCommon_r13.pcch_Config_r13.defaultPagingCycle_r13
    = configuration->pcch_defaultPagingCycle_NB;
  sib2_NB_IoT->radioResourceConfigCommon_r13.pcch_Config_r13.nB_r13 = configuration->pcch_nB_NB;
  sib2_NB_IoT->radioResourceConfigCommon_r13.pcch_Config_r13.npdcch_NumRepetitionPaging_r13 = configuration-> pcch_npdcch_NumRepetitionPaging_NB;
  //NPRACH-Config-NB-IoT-----------------------------------------------------------------
  sib2_NB_IoT->radioResourceConfigCommon_r13.nprach_Config_r13.rsrp_ThresholdsPrachInfoList_r13 = NULL;
  sib2_NB_IoT->radioResourceConfigCommon_r13.nprach_Config_r13.nprach_CP_Length_r13 = configuration->nprach_CP_Length;
  /*OPTIONAL*/
  //   =CALLOC(1, sizeof(struct RSRP_ThresholdsNPRACH_InfoList_NB_r13)); //fatto uguale dopo
  //   rsrp_ThresholdsPrachInfoList = sib2_NB_IoT->radioResourceConfigCommon_r13.nprach_Config_r13.rsrp_ThresholdsPrachInfoList_r13;
  //   rsrp_range = configuration->nprach_rsrp_range_NB;
  //   ASN_SEQUENCE_ADD(&rsrp_ThresholdsPrachInfoList->list,rsrp_range);
  // According configuration to set the 3 CE level configuration setting
  nprach_parameters[0].nprach_Periodicity_r13               = configuration->nprach_Periodicity[0];
  nprach_parameters[0].nprach_StartTime_r13                 = configuration->nprach_StartTime[0];
  nprach_parameters[0].nprach_SubcarrierOffset_r13          = configuration->nprach_SubcarrierOffset[0];
  nprach_parameters[0].nprach_NumSubcarriers_r13            = configuration->nprach_NumSubcarriers[0];
  nprach_parameters[0].numRepetitionsPerPreambleAttempt_r13 = configuration->numRepetitionsPerPreambleAttempt_NB[0];
  nprach_parameters[0].nprach_SubcarrierMSG3_RangeStart_r13 = configuration->nprach_SubcarrierMSG3_RangeStart;
  nprach_parameters[0].maxNumPreambleAttemptCE_r13          = configuration->maxNumPreambleAttemptCE_NB;
  nprach_parameters[0].npdcch_NumRepetitions_RA_r13         = configuration->npdcch_NumRepetitions_RA[0];
  nprach_parameters[0].npdcch_StartSF_CSS_RA_r13            = configuration->npdcch_StartSF_CSS_RA[0];
  nprach_parameters[0].npdcch_Offset_RA_r13                 = configuration->npdcch_Offset_RA[0];
  nprach_parameters[1].nprach_Periodicity_r13               = configuration->nprach_Periodicity[1];
  nprach_parameters[1].nprach_StartTime_r13                 = configuration->nprach_StartTime[1];
  nprach_parameters[1].nprach_SubcarrierOffset_r13          = configuration->nprach_SubcarrierOffset[1];
  nprach_parameters[1].nprach_NumSubcarriers_r13            = configuration->nprach_NumSubcarriers[1];
  nprach_parameters[1].numRepetitionsPerPreambleAttempt_r13 = configuration->numRepetitionsPerPreambleAttempt_NB[1];
  nprach_parameters[1].nprach_SubcarrierMSG3_RangeStart_r13 = configuration->nprach_SubcarrierMSG3_RangeStart;
  nprach_parameters[1].maxNumPreambleAttemptCE_r13          = configuration->maxNumPreambleAttemptCE_NB;
  nprach_parameters[1].npdcch_NumRepetitions_RA_r13         = configuration->npdcch_NumRepetitions_RA[1];
  nprach_parameters[1].npdcch_StartSF_CSS_RA_r13            = configuration->npdcch_StartSF_CSS_RA[1];
  nprach_parameters[1].npdcch_Offset_RA_r13                 = configuration->npdcch_Offset_RA[1];
  nprach_parameters[2].nprach_Periodicity_r13               = configuration->nprach_Periodicity[2];
  nprach_parameters[2].nprach_StartTime_r13                 = configuration->nprach_StartTime[2];
  nprach_parameters[2].nprach_SubcarrierOffset_r13          = configuration->nprach_SubcarrierOffset[2];
  nprach_parameters[2].nprach_NumSubcarriers_r13            = configuration->nprach_NumSubcarriers[2];
  nprach_parameters[2].numRepetitionsPerPreambleAttempt_r13 = configuration->numRepetitionsPerPreambleAttempt_NB[2];
  nprach_parameters[2].nprach_SubcarrierMSG3_RangeStart_r13 = configuration->nprach_SubcarrierMSG3_RangeStart;
  nprach_parameters[2].maxNumPreambleAttemptCE_r13          = configuration->maxNumPreambleAttemptCE_NB;
  nprach_parameters[2].npdcch_NumRepetitions_RA_r13         = configuration->npdcch_NumRepetitions_RA[2];
  nprach_parameters[2].npdcch_StartSF_CSS_RA_r13            = configuration->npdcch_StartSF_CSS_RA[2];
  nprach_parameters[2].npdcch_Offset_RA_r13                 = configuration->npdcch_Offset_RA[2];
  //nprach_parameterList have a max size of 3 possible nprach configuration (see maxNPRACH_Resources_NB_r13)
  ASN_SEQUENCE_ADD(&sib2_NB_IoT->radioResourceConfigCommon_r13.nprach_Config_r13.nprach_ParametersList_r13.list,&nprach_parameters[0]);
  ASN_SEQUENCE_ADD(&sib2_NB_IoT->radioResourceConfigCommon_r13.nprach_Config_r13.nprach_ParametersList_r13.list,&nprach_parameters[1]);
  ASN_SEQUENCE_ADD(&sib2_NB_IoT->radioResourceConfigCommon_r13.nprach_Config_r13.nprach_ParametersList_r13.list,&nprach_parameters[2]);
  // NPDSCH-Config NB-IOT
  sib2_NB_IoT->radioResourceConfigCommon_r13.npdsch_ConfigCommon_r13.nrs_Power_r13= configuration->npdsch_nrs_Power;
  //NPUSCH-Config NB-IoT----------------------------------------------------------------
  //list of size 3 (see maxNPRACH_Resources_NB_r13)
  ack_nack_repetition = configuration-> npusch_ack_nack_numRepetitions_NB; //is an enumerative
  ASN_SEQUENCE_ADD(&(sib2_NB_IoT->radioResourceConfigCommon_r13.npusch_ConfigCommon_r13.ack_NACK_NumRepetitions_Msg4_r13.list) ,&ack_nack_repetition);
  *srs_SubframeConfig = configuration->npusch_srs_SubframeConfig_NB;
  sib2_NB_IoT->radioResourceConfigCommon_r13.npusch_ConfigCommon_r13.srs_SubframeConfig_r13= srs_SubframeConfig; /*OPTIONAL*/
  /*OPTIONAL*/
  dmrs_config = CALLOC(1,sizeof(struct LTE_NPUSCH_ConfigCommon_NB_r13__dmrs_Config_r13));
  dmrs_config->threeTone_CyclicShift_r13 = configuration->npusch_threeTone_CyclicShift_r13;
  dmrs_config->sixTone_CyclicShift_r13 = configuration->npusch_sixTone_CyclicShift_r13;
  /*OPTIONAL
   * -define the base sequence for a DMRS sequence in a cell with multi tone transmission (3,6,12) see TS 36.331 NPUSCH-Config-NB
   * -if not defined will be calculated based on the cellID once we configure the phy layer (rrc_mac_config_req) through the config_sib2 */
  dmrs_config->threeTone_BaseSequence_r13 = NULL;
  dmrs_config->sixTone_BaseSequence_r13 = NULL;
  dmrs_config->twelveTone_BaseSequence_r13 = NULL;
  sib2_NB_IoT->radioResourceConfigCommon_r13.npusch_ConfigCommon_r13.dmrs_Config_r13 = dmrs_config;
  //ulReferenceSignalsNPUSCH
  /*Reference Signal (RS) for UL in NB-IoT is called DRS (Demodulation Reference Signal)
   * sequence-group hopping can be enabled or disabled by means of the cell-specific parameter groupHoppingEnabled_r13
   * sequence-group hopping can be disabled for certain specific UE through the parameter groupHoppingDisabled (physicalConfigDedicated)
   * groupAssignmentNPUSCH--> is used for generate the sequence-shift pattern
   */
  sib2_NB_IoT->radioResourceConfigCommon_r13.npusch_ConfigCommon_r13.ul_ReferenceSignalsNPUSCH_r13.groupHoppingEnabled_r13= configuration->npusch_groupHoppingEnabled;
  sib2_NB_IoT->radioResourceConfigCommon_r13.npusch_ConfigCommon_r13.ul_ReferenceSignalsNPUSCH_r13.groupAssignmentNPUSCH_r13 =configuration->npusch_groupAssignmentNPUSCH_r13;
  //dl_GAP---------------------------------------------------------------------------------/*OPTIONAL*/
  dl_Gap = CALLOC(1,sizeof(struct LTE_DL_GapConfig_NB_r13));
  dl_Gap->dl_GapDurationCoeff_r13= configuration-> dl_GapDurationCoeff_NB;
  dl_Gap->dl_GapPeriodicity_r13= configuration->dl_GapPeriodicity_NB;
  dl_Gap->dl_GapThreshold_r13= configuration->dl_GapThreshold_NB;
  sib2_NB_IoT->radioResourceConfigCommon_r13.dl_Gap_r13 = dl_Gap;
  // uplinkPowerControlCommon - NB-IoT------------------------------------------------------
  sib2_NB_IoT->radioResourceConfigCommon_r13.uplinkPowerControlCommon_r13.p0_NominalNPUSCH_r13 = configuration->npusch_p0_NominalNPUSCH;
  sib2_NB_IoT->radioResourceConfigCommon_r13.uplinkPowerControlCommon_r13.deltaPreambleMsg3_r13 = configuration->deltaPreambleMsg3;
  sib2_NB_IoT->radioResourceConfigCommon_r13.uplinkPowerControlCommon_r13.alpha_r13 = configuration->npusch_alpha;
  //no deltaFlist_PUCCH and no UL cyclic prefix
  // UE Timers and Constants -NB-IoT--------------------------------------------------------
  sib2_NB_IoT->ue_TimersAndConstants_r13.t300_r13 = configuration-> ue_TimersAndConstants_t300_NB;
  sib2_NB_IoT->ue_TimersAndConstants_r13.t301_r13 = configuration-> ue_TimersAndConstants_t301_NB;
  sib2_NB_IoT->ue_TimersAndConstants_r13.t310_r13 = configuration-> ue_TimersAndConstants_t310_NB;
  sib2_NB_IoT->ue_TimersAndConstants_r13.t311_r13 = configuration-> ue_TimersAndConstants_t311_NB;
  sib2_NB_IoT->ue_TimersAndConstants_r13.n310_r13 = configuration-> ue_TimersAndConstants_n310_NB;
  sib2_NB_IoT->ue_TimersAndConstants_r13.n311_r13 = configuration-> ue_TimersAndConstants_n311_NB;
  //other SIB2-NB Parameters--------------------------------------------------------------------------------
  sib2_NB_IoT->freqInfo_r13.additionalSpectrumEmission_r13 = 1;
  sib2_NB_IoT->freqInfo_r13.ul_CarrierFreq_r13 = NULL; /*OPTIONAL*/
  sib2_NB_IoT->timeAlignmentTimerCommon_r13=LTE_TimeAlignmentTimer_infinity;//TimeAlignmentTimer_sf5120;
  /*OPTIONAL*/
  sib2_NB_IoT->multiBandInfoList_r13 = NULL;
  /// SIB3-NB-------------------------------------------------------
  sib3_NB_IoT->cellReselectionInfoCommon_r13.q_Hyst_r13=LTE_SystemInformationBlockType3_NB_r13__cellReselectionInfoCommon_r13__q_Hyst_r13_dB4;
  sib3_NB_IoT->cellReselectionServingFreqInfo_r13.s_NonIntraSearch_r13=0; //or define in configuration?
  sib3_NB_IoT->intraFreqCellReselectionInfo_r13.q_RxLevMin_r13 = -70;
  //new
  sib3_NB_IoT->intraFreqCellReselectionInfo_r13.q_QualMin_r13 = CALLOC(1,sizeof(*sib3_NB_IoT->intraFreqCellReselectionInfo_r13.q_QualMin_r13));
  *(sib3_NB_IoT->intraFreqCellReselectionInfo_r13.q_QualMin_r13)= 10; //a caso
  sib3_NB_IoT->intraFreqCellReselectionInfo_r13.p_Max_r13 = NULL;
  sib3_NB_IoT->intraFreqCellReselectionInfo_r13.s_IntraSearchP_r13 = 31; // s_intraSearch --> s_intraSearchP!!! (they call in a different way)
  sib3_NB_IoT->intraFreqCellReselectionInfo_r13.t_Reselection_r13=1;
  //how to manage?
  sib3_NB_IoT->freqBandInfo_r13 = NULL;
  sib3_NB_IoT->multiBandInfoList_r13 = NULL;
  ///BCCH message (generate the SI message)
  bcch_message->message.present = LTE_BCCH_DL_SCH_MessageType_NB_PR_c1;
  bcch_message->message.choice.c1.present = LTE_BCCH_DL_SCH_MessageType_NB__c1_PR_systemInformation_r13;
  bcch_message->message.choice.c1.choice.systemInformation_r13.criticalExtensions.present = LTE_SystemInformation_NB__criticalExtensions_PR_systemInformation_r13;
  bcch_message->message.choice.c1.choice.systemInformation_r13.criticalExtensions.choice.systemInformation_r13.sib_TypeAndInfo_r13.list.count=0;
  ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation_r13.criticalExtensions.choice.systemInformation_r13.sib_TypeAndInfo_r13.list,
                   sib2_NB_part);
  ASN_SEQUENCE_ADD(&bcch_message->message.choice.c1.choice.systemInformation_r13.criticalExtensions.choice.systemInformation_r13.sib_TypeAndInfo_r13.list,
                   sib3_NB_part);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message_NB, (void *)bcch_message);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_DL_SCH_Message_NB,
                                   NULL,
                                   (void *)bcch_message,
                                   carrier->SIB23_NB_IoT,
                                   900);
  //  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
  //               enc_rval.failed_type->name, enc_rval.encoded);
  //#if defined(ENABLE_ITTI).....
#ifdef USER_MODE
  LOG_D(RRC,"[NB-IoT] SystemInformation-NB Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
#endif

  if (enc_rval.encoded==-1) {
    msg("[RRC] ASN1 : SI-NB encoding failed for SIB23_NB_IoT\n");
    return(-1);
  }

  carrier->sib2_NB_IoT = sib2_NB_IoT;
  carrier->sib3_NB_IoT = sib3_NB_IoT;
  return((enc_rval.encoded+7)/8);
}

/*do_RRCConnectionSetup_NB_IoT--> the aim is to establish SRB1 and SRB1bis(implicitly)*/
uint8_t do_RRCConnectionSetup_NB_IoT(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t      *const ue_context_pP,
  int                              CC_id,
  uint8_t                   *const buffer, //Srb0.Tx_buffer.Payload
  const uint8_t                    Transaction_id,
  const NB_IoT_DL_FRAME_PARMS *const frame_parms, // maybe not used
  LTE_SRB_ToAddModList_NB_r13_t             **SRB_configList_NB_IoT, //for both SRB1bis and SRB1
  struct LTE_PhysicalConfigDedicated_NB_r13 **physicalConfigDedicated_NB_IoT
)

{
  asn_enc_rval_t enc_rval;
  //MP:logical channel group not defined for Nb-IoT
  //MP: logical channel priority pag 605 (is 1 for SRB1 and for SRB1bis should be the same)
  //long* prioritySRB1 = NULL;
  long *prioritySRB1bis = NULL;
  BOOLEAN_t *logicalChannelSR_Prohibit =NULL; //pag 605
  BOOLEAN_t *npusch_AllSymbols= NULL;
  // struct SRB_ToAddMod_NB_r13* SRB1_config_NB = NULL;
  // struct SRB_ToAddMod_NB_r13__rlc_Config_r13* SRB1_rlc_config_NB = NULL;
  // struct SRB_ToAddMod_NB_r13__logicalChannelConfig_r13* SRB1_lchan_config_NB = NULL;
  struct LTE_SRB_ToAddMod_NB_r13 *SRB1bis_config_NB_IoT = NULL;
  struct LTE_SRB_ToAddMod_NB_r13__rlc_Config_r13 *SRB1bis_rlc_config_NB_IoT = NULL;
  struct LTE_SRB_ToAddMod_NB_r13__logicalChannelConfig_r13 *SRB1bis_lchan_config_NB_IoT = NULL;
  //No UL_specific parameters for NB-IoT in LogicalChanelConfig-NB
  LTE_PhysicalConfigDedicated_NB_r13_t *physicalConfigDedicated2_NB_IoT = NULL;
  LTE_DL_CCCH_Message_NB_t dl_ccch_msg_NB_IoT;
  LTE_RRCConnectionSetup_NB_t *rrcConnectionSetup_NB_IoT = NULL;
  memset((void *)&dl_ccch_msg_NB_IoT,0,sizeof(LTE_DL_CCCH_Message_NB_t));
  dl_ccch_msg_NB_IoT.message.present = LTE_DL_CCCH_MessageType_NB_PR_c1;
  dl_ccch_msg_NB_IoT.message.choice.c1.present = LTE_DL_CCCH_MessageType_NB__c1_PR_rrcConnectionSetup_r13;
  rrcConnectionSetup_NB_IoT = &dl_ccch_msg_NB_IoT.message.choice.c1.choice.rrcConnectionSetup_r13;

  if (*SRB_configList_NB_IoT) {
    free(*SRB_configList_NB_IoT);
  }

  *SRB_configList_NB_IoT = CALLOC(1,sizeof(LTE_SRB_ToAddModList_NB_r13_t));
  /// SRB1--------------------
  {
    // SRB1_config_NB = CALLOC(1,sizeof(*SRB1_config_NB));
    //
    // //no srb_Identity in SRB_ToAddMod_NB
    //
    // SRB1_rlc_config_NB = CALLOC(1,sizeof(*SRB1_rlc_config_NB));
    // SRB1_config_NB->rlc_Config_r13   = SRB1_rlc_config_NB;
    //
    // SRB1_rlc_config_NB->present = SRB_ToAddMod_NB_r13__rlc_Config_r13_PR_explicitValue;
    // SRB1_rlc_config_NB->choice.explicitValue.present=RLC_Config_NB_r13_PR_am;//the only possible in NB_IoT
    //
    //// SRB1_rlc_config_NB->choice.explicitValue.choice.am.ul_AM_RLC_r13.t_PollRetransmit_r13 = enb_properties.properties[ctxt_pP->module_id]->srb1_timer_poll_retransmit_r13;
    //// SRB1_rlc_config_NB->choice.explicitValue.choice.am.ul_AM_RLC_r13.maxRetxThreshold_r13 = enb_properties.properties[ctxt_pP->module_id]->srb1_max_retx_threshold_r13;
    //// //(musT be disabled--> SRB1 config pag 640 specs )
    //// SRB1_rlc_config_NB->choice.explicitValue.choice.am.dl_AM_RLC_r13.enableStatusReportSN_Gap_r13 =NULL;
    //
    //
    // SRB1_rlc_config_NB->choice.explicitValue.choice.am.ul_AM_RLC_r13.t_PollRetransmit_r13 = T_PollRetransmit_NB_r13_ms25000;
    // SRB1_rlc_config_NB->choice.explicitValue.choice.am.ul_AM_RLC_r13.maxRetxThreshold_r13 = UL_AM_RLC_NB_r13__maxRetxThreshold_r13_t8;
    // //(musT be disabled--> SRB1 config pag 640 specs )
    // SRB1_rlc_config_NB->choice.explicitValue.choice.am.dl_AM_RLC_r13.enableStatusReportSN_Gap_r13 = NULL;
    //
    // SRB1_lchan_config_NB = CALLOC(1,sizeof(*SRB1_lchan_config_NB));
    // SRB1_config_NB->logicalChannelConfig_r13  = SRB1_lchan_config_NB;
    //
    // SRB1_lchan_config_NB->present = SRB_ToAddMod_NB_r13__logicalChannelConfig_r13_PR_explicitValue;
    //
    //
    // prioritySRB1 = CALLOC(1, sizeof(long));
    // *prioritySRB1 = 1;
    // SRB1_lchan_config_NB->choice.explicitValue.priority_r13 = prioritySRB1;
    //
    // logicalChannelSR_Prohibit = CALLOC(1, sizeof(BOOLEAN_t));
    // *logicalChannelSR_Prohibit = 1;
    // //schould be set to TRUE (specs pag 641)
    // SRB1_lchan_config_NB->choice.explicitValue.logicalChannelSR_Prohibit_r13 = logicalChannelSR_Prohibit;
    //
    // //ADD SRB1
    // ASN_SEQUENCE_ADD(&(*SRB_configList_NB_IoT)->list,SRB1_config_NB);
  }
  ///SRB1bis (The configuration for SRB1 and SRB1bis is the same) the only difference is the logical channel identity = 3 but not set here
  SRB1bis_config_NB_IoT = CALLOC(1,sizeof(*SRB1bis_config_NB_IoT));
  //no srb_Identity in SRB_ToAddMod_NB
  SRB1bis_rlc_config_NB_IoT = CALLOC(1,sizeof(*SRB1bis_rlc_config_NB_IoT));
  SRB1bis_config_NB_IoT->rlc_Config_r13   = SRB1bis_rlc_config_NB_IoT;
  SRB1bis_rlc_config_NB_IoT->present = LTE_SRB_ToAddMod_NB_r13__rlc_Config_r13_PR_explicitValue;
  SRB1bis_rlc_config_NB_IoT->choice.explicitValue.present=LTE_RLC_Config_NB_r13_PR_am;//MP: the only possible RLC config in NB_IoT
  SRB1bis_rlc_config_NB_IoT->choice.explicitValue.choice.am.ul_AM_RLC_r13.t_PollRetransmit_r13 = LTE_T_PollRetransmit_NB_r13_ms25000;
  SRB1bis_rlc_config_NB_IoT->choice.explicitValue.choice.am.ul_AM_RLC_r13.maxRetxThreshold_r13 = LTE_UL_AM_RLC_NB_r13__maxRetxThreshold_r13_t8;
  //(musT be disabled--> SRB1 config pag 640 specs )
  SRB1bis_rlc_config_NB_IoT->choice.explicitValue.choice.am.dl_AM_RLC_r13.enableStatusReportSN_Gap_r13 =NULL;
  SRB1bis_lchan_config_NB_IoT = CALLOC(1,sizeof(*SRB1bis_lchan_config_NB_IoT));
  SRB1bis_config_NB_IoT->logicalChannelConfig_r13  = SRB1bis_lchan_config_NB_IoT;
  SRB1bis_lchan_config_NB_IoT->present = LTE_SRB_ToAddMod_NB_r13__logicalChannelConfig_r13_PR_explicitValue;
  prioritySRB1bis = CALLOC(1, sizeof(long));
  *prioritySRB1bis = 1; //same as SRB1?
  SRB1bis_lchan_config_NB_IoT->choice.explicitValue.priority_r13 = prioritySRB1bis;
  logicalChannelSR_Prohibit = CALLOC(1, sizeof(BOOLEAN_t));
  *logicalChannelSR_Prohibit = 1; //schould be set to TRUE (specs pag 641)
  SRB1bis_lchan_config_NB_IoT->choice.explicitValue.logicalChannelSR_Prohibit_r13 = logicalChannelSR_Prohibit;
  //ADD SRB1bis
  //MP: Actually there is no way to distinguish SRB1 and SRB1bis once put in the list
  //MP: SRB_ToAddModList_NB_r13_t size = 1
  ASN_SEQUENCE_ADD(&(*SRB_configList_NB_IoT)->list,SRB1bis_config_NB_IoT);
  // PhysicalConfigDedicated (NPDCCH, NPUSCH, CarrierConfig, UplinkPowerControl)
  physicalConfigDedicated2_NB_IoT = CALLOC(1,sizeof(*physicalConfigDedicated2_NB_IoT));
  *physicalConfigDedicated_NB_IoT = physicalConfigDedicated2_NB_IoT;
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13= CALLOC(1, sizeof(*physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13));
  physicalConfigDedicated2_NB_IoT->npdcch_ConfigDedicated_r13 = CALLOC(1,sizeof(*physicalConfigDedicated2_NB_IoT->npdcch_ConfigDedicated_r13));
  physicalConfigDedicated2_NB_IoT->npusch_ConfigDedicated_r13 = CALLOC(1,sizeof(*physicalConfigDedicated2_NB_IoT->npusch_ConfigDedicated_r13));
  physicalConfigDedicated2_NB_IoT->uplinkPowerControlDedicated_r13 = CALLOC(1,sizeof(*physicalConfigDedicated2_NB_IoT->uplinkPowerControlDedicated_r13));
  //no tpc, no cqi and no pucch, no pdsch, no soundingRS, no AntennaInfo, no scheduling request config
  /*
   * NB-IoT supports the operation with either one or two antenna ports, AP0 and AP1.
   * For the latter case, Space Frequency Block Coding (SFBC) is applied.
   * Once selected, the same transmission scheme applies to NPBCH, NPDCCH, and NPDSCH.
   * */
  //FIXME: MP: CarrierConfigDedicated check the set values ----------------------------------------------
  //DL
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->dl_CarrierConfig_r13.dl_CarrierFreq_r13.carrierFreq_r13=0;//random value set
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->dl_CarrierConfig_r13.downlinkBitmapNonAnchor_r13= CALLOC(1,
      sizeof(struct LTE_DL_CarrierConfigDedicated_NB_r13__downlinkBitmapNonAnchor_r13));
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->dl_CarrierConfig_r13.downlinkBitmapNonAnchor_r13->present=
    LTE_DL_CarrierConfigDedicated_NB_r13__downlinkBitmapNonAnchor_r13_PR_useNoBitmap_r13;
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->dl_CarrierConfig_r13.dl_GapNonAnchor_r13 = CALLOC(1,sizeof(struct LTE_DL_CarrierConfigDedicated_NB_r13__dl_GapNonAnchor_r13));
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->dl_CarrierConfig_r13.dl_GapNonAnchor_r13->present =
    LTE_DL_CarrierConfigDedicated_NB_r13__dl_GapNonAnchor_r13_PR_useNoGap_r13;
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->dl_CarrierConfig_r13.inbandCarrierInfo_r13= NULL;
  //UL
  physicalConfigDedicated2_NB_IoT->carrierConfigDedicated_r13->ul_CarrierConfig_r13.ul_CarrierFreq_r13= NULL;
  // NPDCCH
  physicalConfigDedicated2_NB_IoT->npdcch_ConfigDedicated_r13->npdcch_NumRepetitions_r13 =0;
  physicalConfigDedicated2_NB_IoT->npdcch_ConfigDedicated_r13->npdcch_Offset_USS_r13 =0;
  physicalConfigDedicated2_NB_IoT->npdcch_ConfigDedicated_r13->npdcch_StartSF_USS_r13=0;
  // NPUSCH //(specs TS 36.331 v14.2.1 pag 643) /* OPTIONAL */
  physicalConfigDedicated2_NB_IoT->npusch_ConfigDedicated_r13->ack_NACK_NumRepetitions_r13= NULL;
  npusch_AllSymbols= CALLOC(1, sizeof(BOOLEAN_t));
  *npusch_AllSymbols= 1; //TRUE
  physicalConfigDedicated2_NB_IoT->npusch_ConfigDedicated_r13->npusch_AllSymbols_r13= npusch_AllSymbols; /* OPTIONAL */
  physicalConfigDedicated2_NB_IoT->npusch_ConfigDedicated_r13->groupHoppingDisabled_r13=NULL; /* OPTIONAL */
  // UplinkPowerControlDedicated
  physicalConfigDedicated2_NB_IoT->uplinkPowerControlDedicated_r13->p0_UE_NPUSCH_r13 = 0; // 0 dB (specs TS36.331 v14.2.1 pag 643)
  //Fill the rrcConnectionSetup-NB message
  rrcConnectionSetup_NB_IoT->rrc_TransactionIdentifier = Transaction_id; //input value
  rrcConnectionSetup_NB_IoT->criticalExtensions.present = LTE_RRCConnectionSetup_NB__criticalExtensions_PR_c1;
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.present =LTE_RRCConnectionSetup_NB__criticalExtensions__c1_PR_rrcConnectionSetup_r13 ;
  //MP: carry only SRB1bis at the moment and phyConfigDedicated
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r13.radioResourceConfigDedicated_r13.srb_ToAddModList_r13 = *SRB_configList_NB_IoT;
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r13.radioResourceConfigDedicated_r13.drb_ToAddModList_r13 = NULL;
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r13.radioResourceConfigDedicated_r13.drb_ToReleaseList_r13 = NULL;
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r13.radioResourceConfigDedicated_r13.rlf_TimersAndConstants_r13 = NULL;
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r13.radioResourceConfigDedicated_r13.physicalConfigDedicated_r13 = physicalConfigDedicated2_NB_IoT;
  rrcConnectionSetup_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionSetup_r13.radioResourceConfigDedicated_r13.mac_MainConfig_r13 = NULL;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message_NB, (void *)&dl_ccch_msg_NB_IoT);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_ccch_msg_NB_IoT,
                                   buffer,
                                   100);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

#ifdef USER_MODE
  LOG_D(RRC,"RRCConnectionSetup-NB Encoded %zd bits (%zd bytes), ecause %d\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8,ecause);
#endif
  return((enc_rval.encoded+7)/8);
}

/*do_SecurityModeCommand - exactly the same as previous implementation*/
uint8_t do_SecurityModeCommand_NB_IoT(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *const buffer,
  const uint8_t Transaction_id,
  const uint8_t cipheringAlgorithm,
  const uint8_t integrityProtAlgorithm) {
  LTE_DL_DCCH_Message_NB_t dl_dcch_msg_NB_IoT;
  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg_NB_IoT,0,sizeof(LTE_DL_DCCH_Message_NB_t));
  dl_dcch_msg_NB_IoT.message.present = LTE_DL_DCCH_MessageType_NB_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.present = LTE_DL_DCCH_MessageType_NB__c1_PR_securityModeCommand_r13;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.securityModeCommand_r13.rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.securityModeCommand_r13.criticalExtensions.present = LTE_SecurityModeCommand__criticalExtensions_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.securityModeCommand_r13.criticalExtensions.choice.c1.present =
    LTE_SecurityModeCommand__criticalExtensions__c1_PR_securityModeCommand_r8;
  // the two following information could be based on the mod_id
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.securityModeCommand_r13.criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm
    = (LTE_CipheringAlgorithm_r12_t)cipheringAlgorithm; //bug solved
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.securityModeCommand_r13.criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm
    = (e_LTE_SecurityAlgorithmConfig__integrityProtAlgorithm)integrityProtAlgorithm;

  //only changed "asn_DEF_DL_DCCH_Message_NB"
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_DCCH_Message_NB, (void *)&dl_dcch_msg_NB_IoT);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_dcch_msg_NB_IoT,
                                   buffer,
                                   100);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

  //#if defined(ENABLE_ITTI)
  //# if !defined(DISABLE_XER_SPRINT)....
#ifdef USER_MODE
  LOG_D(RRC,"[NB-IoT %d] securityModeCommand-NB for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);
#endif

  if (enc_rval.encoded==-1) {
    LOG_E(RRC,"[NB-IoT %d] ASN1 : securityModeCommand-NB encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

/*do_UECapabilityEnquiry_NB_IoT - very similar to legacy lte*/
uint8_t do_UECapabilityEnquiry_NB_IoT(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t               *const buffer,
  const uint8_t                Transaction_id
)

{
  LTE_DL_DCCH_Message_NB_t dl_dcch_msg_NB_IoT;
  //no RAT type in NB-IoT
  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg_NB_IoT,0,sizeof(LTE_DL_DCCH_Message_NB_t));
  dl_dcch_msg_NB_IoT.message.present           = LTE_DL_DCCH_MessageType_NB_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.present = LTE_DL_DCCH_MessageType_NB__c1_PR_ueCapabilityEnquiry_r13;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.ueCapabilityEnquiry_r13.rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.ueCapabilityEnquiry_r13.criticalExtensions.present = LTE_UECapabilityEnquiry_NB__criticalExtensions_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.ueCapabilityEnquiry_r13.criticalExtensions.choice.c1.present =
    LTE_UECapabilityEnquiry_NB__criticalExtensions__c1_PR_ueCapabilityEnquiry_r13;

  //no ue_CapabilityRequest (list of RAT_Type)

  //only changed "asn_DEF_DL_DCCH_Message_NB"
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_DCCH_Message_NB, (void *)&dl_dcch_msg_NB_IoT);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_dcch_msg_NB_IoT,
                                   buffer,
                                   100);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

  //#if defined(ENABLE_ITTI)
  //# if !defined(DISABLE_XER_SPRINT)....
#ifdef USER_MODE
  LOG_D(RRC,"[NB-IoT %d] UECapabilityEnquiry-NB for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);
#endif

  if (enc_rval.encoded==-1) {
    LOG_E(RRC,"[NB-IoT %d] ASN1 : UECapabilityEnquiry-NB encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

/*do_RRCConnectionReconfiguration_NB_IoT-->may convey information for resource configuration
 * (including RBs, MAC main configuration and physical channel configuration)
 * including any associated dedicated NAS information.*/
uint16_t do_RRCConnectionReconfiguration_NB_IoT(
  const protocol_ctxt_t        *const ctxt_pP,
  uint8_t                            *buffer,
  uint8_t                             Transaction_id,
  LTE_SRB_ToAddModList_NB_r13_t          *SRB1_list_NB, //SRB_ConfigList2 (default)--> only SRB1
  LTE_DRB_ToAddModList_NB_r13_t          *DRB_list_NB_IoT, //DRB_ConfigList (default)
  LTE_DRB_ToReleaseList_NB_r13_t         *DRB_list2_NB_IoT, //is NULL when passed
  struct LTE_PhysicalConfigDedicated_NB_r13     *physicalConfigDedicated_NB_IoT,
  LTE_MAC_MainConfig_NB_r13_t                   *mac_MainConfig_NB_IoT,
  struct LTE_RRCConnectionReconfiguration_NB_r13_IEs__dedicatedInfoNASList_r13 *dedicatedInfoNASList_NB_IoT)

{
  //check on DRB_list if contains more than 2 DRB?
  asn_enc_rval_t enc_rval;
  LTE_DL_DCCH_Message_NB_t dl_dcch_msg_NB_IoT;
  LTE_RRCConnectionReconfiguration_NB_t *rrcConnectionReconfiguration_NB;
  memset(&dl_dcch_msg_NB_IoT,0,sizeof(LTE_DL_DCCH_Message_NB_t));
  dl_dcch_msg_NB_IoT.message.present           = LTE_DL_DCCH_MessageType_NB_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.present = LTE_DL_DCCH_MessageType_NB__c1_PR_rrcConnectionReconfiguration_r13;
  rrcConnectionReconfiguration_NB          = &dl_dcch_msg_NB_IoT.message.choice.c1.choice.rrcConnectionReconfiguration_r13;
  // RRCConnectionReconfiguration
  rrcConnectionReconfiguration_NB->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReconfiguration_NB->criticalExtensions.present = LTE_RRCConnectionReconfiguration_NB__criticalExtensions_PR_c1;
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.present =LTE_RRCConnectionReconfiguration_NB__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r13 ;
  //RAdioResourceconfigDedicated
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13 =
    CALLOC(1,sizeof(*rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13));
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->srb_ToAddModList_r13 = SRB1_list_NB; //only SRB1
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->drb_ToAddModList_r13 = DRB_list_NB_IoT;
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->drb_ToReleaseList_r13 = DRB_list2_NB_IoT; //NULL
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->physicalConfigDedicated_r13 = physicalConfigDedicated_NB_IoT;
  //FIXME may not used now
  //rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->rlf_TimersAndConstants_r13

  if (mac_MainConfig_NB_IoT!=NULL) {
    rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->mac_MainConfig_r13 =
      CALLOC(1, sizeof(*rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->mac_MainConfig_r13));
    rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->mac_MainConfig_r13->present
      =LTE_RadioResourceConfigDedicated_NB_r13__mac_MainConfig_r13_PR_explicitValue_r13;
    //why memcopy only this one?
    memcpy(&rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->mac_MainConfig_r13->choice.explicitValue_r13,
           mac_MainConfig_NB_IoT, sizeof(*mac_MainConfig_NB_IoT));
  } else {
    rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.radioResourceConfigDedicated_r13->mac_MainConfig_r13=NULL;
  }

  //no measConfig, measIDlist
  //no mobilityControlInfo
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.dedicatedInfoNASList_r13 = dedicatedInfoNASList_NB_IoT;
  //mainly used for cell-reselection/handover purposes??
  rrcConnectionReconfiguration_NB->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r13.fullConfig_r13 = NULL;
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_dcch_msg_NB_IoT,
                                   buffer,
                                   RRC_BUF_SIZE);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed %s, %li\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

  //changed only asn_DEF_DL_DCCH_Message_NB
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_DL_DCCH_Message_NB,(void *)&dl_dcch_msg_NB_IoT);
  }

  //#if defined(ENABLE_ITTI)
  //# if !defined(DISABLE_XER_SPRINT)...
  LOG_I(RRC,"RRCConnectionReconfiguration-NB Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

/*do_RRCConnectionReestablishmentReject - exactly the same as legacy LTE*/
uint8_t do_RRCConnectionReestablishmentReject_NB_IoT(
  uint8_t                    Mod_id,
  uint8_t                   *const buffer) {
  asn_enc_rval_t enc_rval;
  LTE_DL_CCCH_Message_NB_t dl_ccch_msg_NB_IoT;
  LTE_RRCConnectionReestablishmentReject_t *rrcConnectionReestablishmentReject;
  memset((void *)&dl_ccch_msg_NB_IoT,0,sizeof(LTE_DL_CCCH_Message_NB_t));
  dl_ccch_msg_NB_IoT.message.present = LTE_DL_CCCH_MessageType_NB_PR_c1;
  dl_ccch_msg_NB_IoT.message.choice.c1.present = LTE_DL_CCCH_MessageType_NB__c1_PR_rrcConnectionReestablishmentReject_r13;
  rrcConnectionReestablishmentReject    = &dl_ccch_msg_NB_IoT.message.choice.c1.choice.rrcConnectionReestablishmentReject_r13;
  // RRCConnectionReestablishmentReject //exactly the same as LTE
  rrcConnectionReestablishmentReject->criticalExtensions.present = LTE_RRCConnectionReestablishmentReject__criticalExtensions_PR_rrcConnectionReestablishmentReject_r8;

  //Only change in "asn_DEF_DL_CCCH_Message_NB"
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message_NB, (void *)&dl_ccch_msg_NB_IoT);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_ccch_msg_NB_IoT,
                                   buffer,
                                   100);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC,"ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

  //Only change in "asn_DEF_DL_CCCH_Message_NB"
#ifdef USER_MODE
  LOG_D(RRC,"RRCConnectionReestablishmentReject Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);
#endif
  return((enc_rval.encoded+7)/8);
}

/*do_RRCConnectionReject_NB_IoT*/
uint8_t do_RRCConnectionReject_NB_IoT(
  uint8_t                    Mod_id,
  uint8_t                   *const buffer)

{
  asn_enc_rval_t enc_rval;
  LTE_DL_CCCH_Message_NB_t          dl_ccch_msg_NB_IoT;
  LTE_RRCConnectionReject_NB_t      *rrcConnectionReject_NB_IoT;
  memset((void *)&dl_ccch_msg_NB_IoT,0,sizeof(LTE_DL_CCCH_Message_NB_t));
  dl_ccch_msg_NB_IoT.message.present           = LTE_DL_CCCH_MessageType_NB_PR_c1;
  dl_ccch_msg_NB_IoT.message.choice.c1.present = LTE_DL_CCCH_MessageType_NB__c1_PR_rrcConnectionReject_r13;
  rrcConnectionReject_NB_IoT = &dl_ccch_msg_NB_IoT.message.choice.c1.choice.rrcConnectionReject_r13;
  // RRCConnectionReject-NB_IoT
  rrcConnectionReject_NB_IoT->criticalExtensions.present = LTE_RRCConnectionReject_NB__criticalExtensions_PR_c1;
  rrcConnectionReject_NB_IoT->criticalExtensions.choice.c1.present = LTE_RRCConnectionReject_NB__criticalExtensions__c1_PR_rrcConnectionReject_r13;
  /* let's put an extended wait time of 1s for the moment */
  rrcConnectionReject_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReject_r13.extendedWaitTime_r13 = 1;
  //new-use of suspend indication
  //If present, this field indicates that the UE should remain suspended and not release its stored context.
  rrcConnectionReject_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReject_r13.rrc_SuspendIndication_r13=
    CALLOC(1, sizeof(long));
  *(rrcConnectionReject_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReject_r13.rrc_SuspendIndication_r13)=
    LTE_RRCConnectionReject_NB_r13_IEs__rrc_SuspendIndication_r13_true;

  //Only Modified "asn_DEF_DL_CCCH_Message_NB"
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_DL_CCCH_Message_NB, (void *)&dl_ccch_msg_NB_IoT);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_ccch_msg_NB_IoT,
                                   buffer,
                                   100);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed (%s, %ld)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

#ifdef USER_MODE
  LOG_D(RRC,"RRCConnectionReject-NB Encoded %zd bits (%zd bytes)\n",
        enc_rval.encoded,(enc_rval.encoded+7)/8);
#endif
  return((enc_rval.encoded+7)/8);
}


//no do_MBSFNAreaConfig(..) in NB-IoT
//no do_MeasurementReport(..) in NB-IoT

/*do_DLInformationTransfer_NB*/
uint8_t do_DLInformationTransfer_NB_IoT(
  uint8_t Mod_id,
  uint8_t **buffer,
  uint8_t transaction_id,
  uint32_t pdu_length,
  uint8_t *pdu_buffer)

{
  ssize_t encoded;
  LTE_DL_DCCH_Message_NB_t dl_dcch_msg_NB_IoT;
  memset(&dl_dcch_msg_NB_IoT, 0, sizeof(LTE_DL_DCCH_Message_NB_t));
  dl_dcch_msg_NB_IoT.message.present           = LTE_DL_DCCH_MessageType_NB_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.present = LTE_DL_DCCH_MessageType_NB__c1_PR_dlInformationTransfer_r13;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.dlInformationTransfer_r13.rrc_TransactionIdentifier = transaction_id;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.dlInformationTransfer_r13.criticalExtensions.present = LTE_DLInformationTransfer_NB__criticalExtensions_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.dlInformationTransfer_r13.criticalExtensions.choice.c1.present = LTE_DLInformationTransfer_NB__criticalExtensions__c1_PR_dlInformationTransfer_r13;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.dlInformationTransfer_r13.criticalExtensions.choice.c1.choice.dlInformationTransfer_r13.dedicatedInfoNAS_r13.size = pdu_length;
  dl_dcch_msg_NB_IoT.message.choice.c1.choice.dlInformationTransfer_r13.criticalExtensions.choice.c1.choice.dlInformationTransfer_r13.dedicatedInfoNAS_r13.buf = pdu_buffer;
  encoded = uper_encode_to_new_buffer (&asn_DEF_LTE_DL_DCCH_Message_NB, NULL, (void *) &dl_dcch_msg_NB_IoT, (void **) buffer);
  //only change in "asn_DEF_DL_DCCH_Message_NB"
  return encoded;
}

/*do_ULInformationTransfer*/
//for the moment is not needed (UE-SIDE)

/*OAI_UECapability_t *fill_ue_capability*/

/*do_RRCConnectionReestablishment_NB-->used to re-establish SRB1*/ //which parameter to use?
uint8_t do_RRCConnectionReestablishment_NB_IoT(
  uint8_t Mod_id,
  uint8_t *const buffer,
  const uint8_t     Transaction_id,
  const NB_IoT_DL_FRAME_PARMS *const frame_parms, //to be changed
  LTE_SRB_ToAddModList_NB_r13_t      *SRB_list_NB_IoT) { //should contain SRB1 already configured?
  asn_enc_rval_t enc_rval;
  LTE_DL_CCCH_Message_NB_t dl_ccch_msg_NB_IoT;
  LTE_RRCConnectionReestablishment_NB_t *rrcConnectionReestablishment_NB_IoT;
  memset(&dl_ccch_msg_NB_IoT, 0, sizeof(LTE_DL_CCCH_Message_NB_t));
  dl_ccch_msg_NB_IoT.message.present = LTE_DL_CCCH_MessageType_NB_PR_c1;
  dl_ccch_msg_NB_IoT.message.choice.c1.present = LTE_DL_CCCH_MessageType_NB__c1_PR_rrcConnectionReestablishment_r13;
  rrcConnectionReestablishment_NB_IoT = &dl_ccch_msg_NB_IoT.message.choice.c1.choice.rrcConnectionReestablishment_r13;
  //rrcConnectionReestablishment_NB
  rrcConnectionReestablishment_NB_IoT->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.present = LTE_RRCConnectionReestablishment_NB__criticalExtensions_PR_c1;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.present = LTE_RRCConnectionReestablishment_NB__criticalExtensions__c1_PR_rrcConnectionReestablishment_r13;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.radioResourceConfigDedicated_r13.srb_ToAddModList_r13 = SRB_list_NB_IoT;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.radioResourceConfigDedicated_r13.drb_ToAddModList_r13 = NULL;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.radioResourceConfigDedicated_r13.drb_ToReleaseList_r13 = NULL;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.radioResourceConfigDedicated_r13.rlf_TimersAndConstants_r13= NULL;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.radioResourceConfigDedicated_r13.mac_MainConfig_r13= NULL;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.radioResourceConfigDedicated_r13.physicalConfigDedicated_r13 = NULL;
  rrcConnectionReestablishment_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionReestablishment_r13.nextHopChainingCount_r13=0;
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_CCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_ccch_msg_NB_IoT,
                                   buffer,
                                   RRC_BUF_SIZE);

  if (enc_rval.encoded <= 0) {
    LOG_E(RRC, "ASN1 message encoding failed (%s, %li)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_DL_CCCH_Message_NB,(void *)&dl_ccch_msg_NB_IoT);
  }

  LOG_I(RRC,"RRCConnectionReestablishment-NB Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  return 0;
}

/*do_RRCConnectionRelease_NB--> is used to command the release of an RRC connection*/
uint8_t do_RRCConnectionRelease_NB_IoT(
  uint8_t                             Mod_id,
  uint8_t                            *buffer,
  const uint8_t                             Transaction_id) {
  asn_enc_rval_t enc_rval;
  LTE_DL_DCCH_Message_NB_t dl_dcch_msg_NB_IoT;
  LTE_RRCConnectionRelease_NB_t *rrcConnectionRelease_NB_IoT;
  memset(&dl_dcch_msg_NB_IoT,0,sizeof(LTE_DL_DCCH_Message_NB_t));
  dl_dcch_msg_NB_IoT.message.present           = LTE_DL_DCCH_MessageType_NB_PR_c1;
  dl_dcch_msg_NB_IoT.message.choice.c1.present = LTE_DL_DCCH_MessageType_NB__c1_PR_rrcConnectionRelease_r13;
  rrcConnectionRelease_NB_IoT                  = &dl_dcch_msg_NB_IoT.message.choice.c1.choice.rrcConnectionRelease_r13;
  // RRCConnectionRelease
  rrcConnectionRelease_NB_IoT->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionRelease_NB_IoT->criticalExtensions.present = LTE_RRCConnectionRelease_NB__criticalExtensions_PR_c1;
  rrcConnectionRelease_NB_IoT->criticalExtensions.choice.c1.present = LTE_RRCConnectionRelease_NB__criticalExtensions__c1_PR_rrcConnectionRelease_r13 ;
  rrcConnectionRelease_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r13.releaseCause_r13 = LTE_ReleaseCause_NB_r13_other;
  rrcConnectionRelease_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r13.redirectedCarrierInfo_r13 = NULL;
  rrcConnectionRelease_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r13.extendedWaitTime_r13 = NULL;
  //Why allocate memory for non critical extension?
  rrcConnectionRelease_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r13.nonCriticalExtension=CALLOC(1,
      sizeof(*rrcConnectionRelease_NB_IoT->criticalExtensions.choice.c1.choice.rrcConnectionRelease_r13.nonCriticalExtension));
  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message_NB,
                                   NULL,
                                   (void *)&dl_dcch_msg_NB_IoT,
                                   buffer,
                                   RRC_BUF_SIZE);//check
  return((enc_rval.encoded+7)/8);
}




