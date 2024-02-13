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

/* \file rrc_UE.c
 * \brief RRC procedures
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#define RRC_UE
#define RRC_UE_C

#include "NR_DL-DCCH-Message.h"        //asn_DEF_NR_DL_DCCH_Message
#include "NR_DL-CCCH-Message.h"        //asn_DEF_NR_DL_CCCH_Message
#include "NR_BCCH-BCH-Message.h"       //asn_DEF_NR_BCCH_BCH_Message
#include "NR_BCCH-DL-SCH-Message.h"    //asn_DEF_NR_BCCH_DL_SCH_Message
#include "NR_CellGroupConfig.h"        //asn_DEF_NR_CellGroupConfig
#include "NR_BWP-Downlink.h"           //asn_DEF_NR_BWP_Downlink
#include "NR_RRCReconfiguration.h"
#include "NR_MeasConfig.h"
#include "NR_UL-DCCH-Message.h"
#include "uper_encoder.h"
#include "uper_decoder.h"

#include "rrc_defs.h"
#include "rrc_proto.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"

#include "intertask_interface.h"

#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "nr-uesoftmodem.h"
#include "plmn_data.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"
#include "openair3/SECU/secu_defs.h"
#include "openair3/SECU/key_nas_deriver.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#ifndef CELLULAR
  #include "RRC/NR/MESSAGES/asn1_msg.h"
#endif

#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#include "SIMULATION/TOOLS/sim.h" // for taus

#include "nr_nas_msg_sim.h"

static NR_UE_RRC_INST_t *NR_UE_rrc_inst;
/* NAS Attach request with IMSI */
static const char nr_nas_attach_req_imsi_dummy_NSA_case[] = {
    0x07,
    0x41,
    /* EPS Mobile identity = IMSI */
    0x71,
    0x08,
    0x29,
    0x80,
    0x43,
    0x21,
    0x43,
    0x65,
    0x87,
    0xF9,
    /* End of EPS Mobile Identity */
    0x02,
    0xE0,
    0xE0,
    0x00,
    0x20,
    0x02,
    0x03,
    0xD0,
    0x11,
    0x27,
    0x1A,
    0x80,
    0x80,
    0x21,
    0x10,
    0x01,
    0x00,
    0x00,
    0x10,
    0x81,
    0x06,
    0x00,
    0x00,
    0x00,
    0x00,
    0x83,
    0x06,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0D,
    0x00,
    0x00,
    0x0A,
    0x00,
    0x52,
    0x12,
    0xF2,
    0x01,
    0x27,
    0x11,
};

static void nr_rrc_manage_rlc_bearers(const NR_UE_RRC_INST_t *rrc,
                                      const NR_CellGroupConfig_t *cellGroupConfig,
                                      rrcPerNB_t *nb);

static void nr_rrc_ue_process_RadioBearerConfig(NR_UE_RRC_INST_t *ue_rrc,
                                                rrcPerNB_t *rrcNB,
                                                NR_RadioBearerConfig_t *const radioBearerConfig);
static void nr_rrc_ue_generate_rrcReestablishmentComplete(NR_RRCReestablishment_t *rrcReestablishment);
static void process_lte_nsa_msg(NR_UE_RRC_INST_t *rrc, nsa_msg_t *msg, int msg_len);
static void nr_rrc_ue_process_rrcReconfiguration(NR_UE_RRC_INST_t *rrc,
                                                 int gNB_index,
                                                 NR_RRCReconfiguration_t *rrcReconfiguration);

static void nr_rrc_ue_process_ueCapabilityEnquiry(NR_UE_RRC_INST_t *rrc, NR_UECapabilityEnquiry_t *UECapabilityEnquiry);
static void nr_rrc_ue_process_masterCellGroup(NR_UE_RRC_INST_t *rrc,
                                              rrcPerNB_t *rrcNB,
                                              OCTET_STRING_t *masterCellGroup,
                                              long *fullConfig);

void nr_rrc_ue_process_measConfig(rrcPerNB_t *rrc, NR_MeasConfig_t *const measConfig);

static void nr_rrc_ue_process_rrcReconfiguration(NR_UE_RRC_INST_t *rrc,
                                                 int gNB_index,
                                                 NR_RRCReconfiguration_t *rrcReconfiguration)
{
  rrcPerNB_t *rrcNB = rrc->perNB + gNB_index;

  switch (rrcReconfiguration->criticalExtensions.present) {
    case NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration: {
      NR_RRCReconfiguration_IEs_t *ie = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration;

      if (ie->radioBearerConfig != NULL) {
        LOG_I(NR_RRC, "radio Bearer Configuration is present\n");
        nr_rrc_ue_process_RadioBearerConfig(rrc, rrcNB, ie->radioBearerConfig);
        if (LOG_DEBUGFLAG(DEBUG_ASN1))
          xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void *)ie->radioBearerConfig);
      }

      if (ie->nonCriticalExtension) {
        NR_RRCReconfiguration_v1530_IEs_t *ext = ie->nonCriticalExtension;
        if (ext->masterCellGroup)
          nr_rrc_ue_process_masterCellGroup(rrc, rrcNB, ext->masterCellGroup, ext->fullConfig);
        /* Check if there is dedicated NAS information to forward to NAS */
        if (ie->nonCriticalExtension->dedicatedNAS_MessageList) {
          struct NR_RRCReconfiguration_v1530_IEs__dedicatedNAS_MessageList *tmp = ext->dedicatedNAS_MessageList;
          for (int i = 0; i < tmp->list.count; i++) {
            MessageDef *ittiMsg = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_CONN_ESTABLI_CNF);
            NasConnEstabCnf *msg = &NAS_CONN_ESTABLI_CNF(ittiMsg);
            msg->errCode = AS_SUCCESS;
            msg->nasMsg.length = tmp->list.array[i]->size;
            msg->nasMsg.data = tmp->list.array[i]->buf;
            itti_send_msg_to_task(TASK_NAS_NRUE, rrc->ue_id, ittiMsg);
          }
          tmp->list.count = 0; // to prevent the automatic free by ASN1_FREE
        }
      }

      if (ie->secondaryCellGroup != NULL) {
        NR_CellGroupConfig_t *cellGroupConfig = NULL;
        asn_dec_rval_t dec_rval = uper_decode(NULL,
                                              &asn_DEF_NR_CellGroupConfig, // might be added prefix later
                                              (void **)&cellGroupConfig,
                                              (uint8_t *)ie->secondaryCellGroup->buf,
                                              ie->secondaryCellGroup->size,
                                              0,
                                              0);
        if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
          uint8_t *buffer = ie->secondaryCellGroup->buf;
          LOG_E(NR_RRC, "NR_CellGroupConfig decode error\n");
          for (int i = 0; i < ie->secondaryCellGroup->size; i++)
            LOG_E(NR_RRC, "%02x ", buffer[i]);
          LOG_E(NR_RRC, "\n");
          // free the memory
          SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)cellGroupConfig, 1);
        }

        if (LOG_DEBUGFLAG(DEBUG_ASN1))
          xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);

        nr_rrc_cellgroup_configuration(rrcNB, rrc, cellGroupConfig);

        AssertFatal(!get_softmodem_params()->sa, "secondaryCellGroup only used in NSA for now\n");
        nr_rrc_mac_config_req_cg(rrc->ue_id, 0, cellGroupConfig);

        asn1cFreeStruc(asn_DEF_NR_CellGroupConfig, cellGroupConfig);
      }
      if (ie->measConfig != NULL) {
        LOG_I(NR_RRC, "Measurement Configuration is present\n");
        //  if some element need to be updated
        nr_rrc_ue_process_measConfig(rrcNB, ie->measConfig);
      }
      if (ie->lateNonCriticalExtension != NULL) {
        //  unuse now
      }
    } break;

    case NR_RRCReconfiguration__criticalExtensions_PR_NOTHING:
    case NR_RRCReconfiguration__criticalExtensions_PR_criticalExtensionsFuture:
    default:
      break;
  }
  return;
}

void process_nsa_message(NR_UE_RRC_INST_t *rrc, nsa_message_t nsa_message_type, void *message, int msg_len)
{
  switch (nsa_message_type) {
    case nr_SecondaryCellGroupConfig_r15:
      {
        NR_RRCReconfiguration_t *RRCReconfiguration=NULL;
        asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                                &asn_DEF_NR_RRCReconfiguration,
                                (void **)&RRCReconfiguration,
                                (uint8_t *)message,
                                msg_len); 
        
        if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
          LOG_E(NR_RRC, "NR_RRCReconfiguration decode error\n");
          // free the memory
          SEQUENCE_free( &asn_DEF_NR_RRCReconfiguration, RRCReconfiguration, 1 );
          return;
        }
        nr_rrc_ue_process_rrcReconfiguration(rrc, 0, RRCReconfiguration);
        ASN_STRUCT_FREE(asn_DEF_NR_RRCReconfiguration, RRCReconfiguration);
      }
      break;
    
    case nr_RadioBearerConfigX_r15:
      {
        NR_RadioBearerConfig_t *RadioBearerConfig=NULL;
        asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                                &asn_DEF_NR_RadioBearerConfig,
                                (void **)&RadioBearerConfig,
                                (uint8_t *)message,
                                msg_len); 
        
        if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
          LOG_E(NR_RRC, "NR_RadioBearerConfig decode error\n");
          // free the memory
          SEQUENCE_free( &asn_DEF_NR_RadioBearerConfig, RadioBearerConfig, 1 );
          return;
        }
        LOG_D(NR_RRC, "Calling nr_rrc_ue_process_RadioBearerConfig()with: e_rab_id = %ld, drbID = %ld, cipher_algo = %ld, key = %ld \n",
                     RadioBearerConfig->drb_ToAddModList->list.array[0]->cnAssociation->choice.eps_BearerIdentity,
                     RadioBearerConfig->drb_ToAddModList->list.array[0]->drb_Identity,
                     RadioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm,
                     *RadioBearerConfig->securityConfig->keyToUse);
        nr_rrc_ue_process_RadioBearerConfig(rrc, rrc->perNB + 0, RadioBearerConfig);
        if (LOG_DEBUGFLAG(DEBUG_ASN1))
          xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void *)RadioBearerConfig);
        ASN_STRUCT_FREE(asn_DEF_NR_RadioBearerConfig, RadioBearerConfig);
      }
      break;
    
    default:
      AssertFatal(1==0,"Unknown message %d\n",nsa_message_type);
      break;
  }
}

NR_UE_RRC_INST_t* nr_rrc_init_ue(char* uecap_file, int nb_inst)
{
  NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)calloc(nb_inst, sizeof(NR_UE_RRC_INST_t));
  AssertFatal(NR_UE_rrc_inst, "Couldn't allocate %d instances of RRC module\n", nb_inst);

  for(int nr_ue = 0; nr_ue < nb_inst; nr_ue++) {
    NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[nr_ue];
    rrc->ue_id = nr_ue;
    // fill UE-NR-Capability @ UE-CapabilityRAT-Container here.
    rrc->selected_plmn_identity = 1;

    rrc->dl_bwp_id = 0;
    rrc->ul_bwp_id = 0;
    rrc->as_security_activated = false;
    rrc->ra_trigger = RA_NOT_RUNNING;
    rrc->uecap_file = uecap_file;

    memset(&rrc->timers_and_constants, 0, sizeof(rrc->timers_and_constants));
    set_default_timers_and_constants(&rrc->timers_and_constants);

    for (int i = 0; i < NB_CNX_UE; i++) {
      rrcPerNB_t *ptr = &rrc->perNB[i];
      ptr->SInfo = (NR_UE_RRC_SI_INFO){0};
      init_SI_timers(&ptr->SInfo);
      for (int j = 0; j < NR_NUM_SRB; j++)
        ptr->Srb[j] = RB_NOT_PRESENT;
      for (int j = 0; j < MAX_DRBS_PER_UE; j++)
        ptr->status_DRBs[j] = RB_NOT_PRESENT;
      // SRB0 activated by default
      ptr->Srb[0] = RB_ESTABLISHED;
    }

    init_sidelink(rrc);
  }

  return NR_UE_rrc_inst;
}

bool check_si_validity(NR_UE_RRC_SI_INFO *SI_info, int si_type)
{
  switch (si_type) {
    case NR_SIB_TypeInfo__type_sibType2:
      if (!SI_info->sib2)
        return false;
    case NR_SIB_TypeInfo__type_sibType3:
      if (!SI_info->sib3)
        return false;
    case NR_SIB_TypeInfo__type_sibType4:
      if (!SI_info->sib4)
        return false;
    case NR_SIB_TypeInfo__type_sibType5:
      if (!SI_info->sib5)
        return false;
    case NR_SIB_TypeInfo__type_sibType6:
      if (!SI_info->sib6)
        return false;
    case NR_SIB_TypeInfo__type_sibType7:
      if (!SI_info->sib7)
        return false;
    case NR_SIB_TypeInfo__type_sibType8:
      if (!SI_info->sib8)
        return false;
    case NR_SIB_TypeInfo__type_sibType9:
      if (!SI_info->sib9)
        return false;
    case NR_SIB_TypeInfo__type_sibType10_v1610:
      if (!SI_info->sib10)
        return false;
    case NR_SIB_TypeInfo__type_sibType11_v1610:
      if (!SI_info->sib11)
        return false;
    case NR_SIB_TypeInfo__type_sibType12_v1610:
      if (!SI_info->sib12)
        return false;
    case NR_SIB_TypeInfo__type_sibType13_v1610:
      if (!SI_info->sib13)
        return false;
    case NR_SIB_TypeInfo__type_sibType14_v1610:
      if (!SI_info->sib14)
        return false;
    default :
      AssertFatal(false, "Invalid SIB type %d\n", si_type);
  }
  return true;
}

int check_si_status(NR_UE_RRC_SI_INFO *SI_info)
{
  // schedule reception of SIB1 if RRC doesn't have it
  if (!SI_info->sib1)
    return 1;
  else {
    if (SI_info->sib1->si_SchedulingInfo) {
      // Check if RRC has configured default SI
      // from SIB2 to SIB14 as current ASN1 version
      // TODO can be used for on demand SI when (if) implemented
      for (int i = 2; i < 15; i++) {
        int si_index = i - 2;
        if ((SI_info->default_otherSI_map >> si_index) & 0x01) {
          // if RRC has no valid version of one of the default configured SI
          // Then schedule reception of otherSI
          if (!check_si_validity(SI_info, si_index))
            return 2;
        }
      }
    }
  }
  return 0;
}

/*brief decode BCCH-BCH (MIB) message*/
static void nr_rrc_ue_decode_NR_BCCH_BCH_Message(NR_UE_RRC_INST_t *rrc,
                                                 const uint8_t gNB_index,
                                                 uint8_t *const bufferP,
                                                 const uint8_t buffer_len)
{
  NR_BCCH_BCH_Message_t *bcch_message = NULL;

  asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                 &asn_DEF_NR_BCCH_BCH_Message,
                                                 (void **)&bcch_message,
                                                 (const void *)bufferP,
                                                 buffer_len);

  if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, "NR_BCCH_BCH decode error\n");
    return;
  }

  int get_sib = 0;
  if (get_softmodem_params()->sa &&
      bcch_message->message.choice.mib->cellBarred == NR_MIB__cellBarred_notBarred &&
      rrc->nrRrcState != RRC_STATE_DETACH_NR) {
    NR_UE_RRC_SI_INFO *SI_info = &rrc->perNB[gNB_index].SInfo;
    // to schedule MAC to get SI if required
    get_sib = check_si_status(SI_info);
  }
  nr_rrc_mac_config_req_mib(rrc->ue_id, 0, bcch_message->message.choice.mib, get_sib);
  ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_Message, bcch_message);
  return;
}

static int nr_decode_SI(NR_UE_RRC_SI_INFO *SI_info, NR_SystemInformation_t *si)
{
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_IN );

  // Dump contents
  if (si->criticalExtensions.present == NR_SystemInformation__criticalExtensions_PR_systemInformation ||
      si->criticalExtensions.present == NR_SystemInformation__criticalExtensions_PR_criticalExtensionsFuture_r16) {
    LOG_D( RRC, "[UE] si->criticalExtensions.choice.NR_SystemInformation_t->sib_TypeAndInfo.list.count %d\n",
           si->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.count );
  } else {
    LOG_D( RRC, "[UE] Unknown criticalExtension version (not Rel16)\n" );
    return -1;
  }

  for (int i = 0; i < si->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.count; i++) {
    SystemInformation_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = si->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.array[i];
    LOG_I(RRC, "Found SIB%d\n", typeandinfo->present + 1);
    switch(typeandinfo->present) {
      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2:
        if(!SI_info->sib2)
          SI_info->sib2 = calloc(1, sizeof(*SI_info->sib2));
        memcpy(SI_info->sib2, typeandinfo->choice.sib2, sizeof(NR_SIB2_t));
        nr_timer_start(&SI_info->sib2_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib3:
        if(!SI_info->sib3)
          SI_info->sib3 = calloc(1, sizeof(*SI_info->sib3));
        memcpy(SI_info->sib3, typeandinfo->choice.sib3, sizeof(NR_SIB3_t));
        nr_timer_start(&SI_info->sib3_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib4:
        if(!SI_info->sib4)
          SI_info->sib4 = calloc(1, sizeof(*SI_info->sib4));
        memcpy(SI_info->sib4, typeandinfo->choice.sib4, sizeof(NR_SIB4_t));
        nr_timer_start(&SI_info->sib4_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib5:
        if(!SI_info->sib5)
          SI_info->sib5 = calloc(1, sizeof(*SI_info->sib5));
        memcpy(SI_info->sib5, typeandinfo->choice.sib5, sizeof(NR_SIB5_t));
        nr_timer_start(&SI_info->sib5_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib6:
        if(!SI_info->sib6)
          SI_info->sib6 = calloc(1, sizeof(*SI_info->sib6));
        memcpy(SI_info->sib6, typeandinfo->choice.sib6, sizeof(NR_SIB6_t));
        nr_timer_start(&SI_info->sib6_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib7:
        if(!SI_info->sib7)
          SI_info->sib7 = calloc(1, sizeof(*SI_info->sib7));
        memcpy(SI_info->sib7, typeandinfo->choice.sib7, sizeof(NR_SIB7_t));
        nr_timer_start(&SI_info->sib7_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib8:
        if(!SI_info->sib8)
          SI_info->sib8 = calloc(1, sizeof(*SI_info->sib8));
        memcpy(SI_info->sib8, typeandinfo->choice.sib8, sizeof(NR_SIB8_t));
        nr_timer_start(&SI_info->sib8_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib9:
        if(!SI_info->sib9)
          SI_info->sib9 = calloc(1, sizeof(*SI_info->sib9));
        memcpy(SI_info->sib9, typeandinfo->choice.sib9, sizeof(NR_SIB9_t));
        nr_timer_start(&SI_info->sib9_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib10_v1610:
        if(!SI_info->sib10)
          SI_info->sib10 = calloc(1, sizeof(*SI_info->sib10));
        memcpy(SI_info->sib10, typeandinfo->choice.sib10_v1610, sizeof(NR_SIB10_r16_t));
        nr_timer_start(&SI_info->sib10_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib11_v1610:
        if(!SI_info->sib11)
          SI_info->sib11 = calloc(1, sizeof(*SI_info->sib11));
        memcpy(SI_info->sib11, typeandinfo->choice.sib11_v1610, sizeof(NR_SIB11_r16_t));
        nr_timer_start(&SI_info->sib11_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib12_v1610:
        if(!SI_info->sib12)
          SI_info->sib12 = calloc(1, sizeof(*SI_info->sib12));
        memcpy(SI_info->sib12, typeandinfo->choice.sib12_v1610, sizeof(NR_SIB12_r16_t));
        nr_timer_start(&SI_info->sib12_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib13_v1610:
        if(!SI_info->sib13)
          SI_info->sib13 = calloc(1, sizeof(*SI_info->sib13));
        memcpy(SI_info->sib13, typeandinfo->choice.sib13_v1610, sizeof(NR_SIB13_r16_t));
        nr_timer_start(&SI_info->sib13_timer);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib14_v1610:
        if(!SI_info->sib14)
          SI_info->sib14 = calloc(1, sizeof(*SI_info->sib14));
        memcpy(SI_info->sib12, typeandinfo->choice.sib14_v1610, sizeof(NR_SIB14_r16_t));
        nr_timer_start(&SI_info->sib14_timer);
        break;
      default:
        break;
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_OUT);
  return 0;
}

static void nr_rrc_handle_msg3_indication(NR_UE_RRC_INST_t *rrc, rnti_t rnti)
{
  switch (rrc->ra_trigger) {
    case INITIAL_ACCESS_FROM_RRC_IDLE:
      // After SIB1 is received, prepare RRCConnectionRequest
      rrc->rnti = rnti;
      // start timer T300
      NR_UE_Timers_Constants_t *tac = &rrc->timers_and_constants;
      nr_timer_start(&tac->T300);
      break;
    case RRC_CONNECTION_REESTABLISHMENT:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    case DURING_HANDOVER:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    case NON_SYNCHRONISED:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    case TRANSITION_FROM_RRC_INACTIVE:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    case TO_ESTABLISH_TA:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    case REQUEST_FOR_OTHER_SI:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    case BEAM_FAILURE_RECOVERY:
      AssertFatal(1==0, "ra_trigger not implemented yet!\n");
      break;
    default:
      AssertFatal(1==0, "Invalid ra_trigger value!\n");
      break;
  }
}

static void nr_rrc_ue_prepare_RRCSetupRequest(NR_UE_RRC_INST_t *rrc)
{
  LOG_D(NR_RRC, "Generation of RRCSetupRequest\n");
  uint8_t rv[6];
  // Get RRCConnectionRequest, fill random for now
  // Generate random byte stream for contention resolution
  for (int i = 0; i < 6; i++) {
#ifdef SMBV
    // if SMBV is configured the contention resolution needs to be fix for the connection procedure to succeed
    rv[i] = i;
#else
    rv[i] = taus() & 0xff;
#endif
  }

  uint8_t buf[1024];
  int len = do_RRCSetupRequest(buf, sizeof(buf), rv);

  nr_rlc_srb_recv_sdu(rrc->ue_id, 0, buf, len);
}

void nr_rrc_configure_default_SI(NR_UE_RRC_SI_INFO *SI_info,
                                 NR_SIB1_t *sib1)
{
  struct NR_SI_SchedulingInfo *si_SchedulingInfo = sib1->si_SchedulingInfo;
  if (!si_SchedulingInfo)
    return;
  SI_info->default_otherSI_map = 0;
  for (int i = 0; i < si_SchedulingInfo->schedulingInfoList.list.count; i++) {
    struct NR_SchedulingInfo *schedulingInfo = si_SchedulingInfo->schedulingInfoList.list.array[i];
    for (int j = 0; j < schedulingInfo->sib_MappingInfo.list.count; j++) {
      struct NR_SIB_TypeInfo *sib_Type = schedulingInfo->sib_MappingInfo.list.array[j];
      SI_info->default_otherSI_map |= 1 << sib_Type->type;
    }
  }
}

static int8_t nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message(NR_UE_RRC_INST_t *rrc,
                                                      const uint8_t gNB_index,
                                                      uint8_t *const Sdu,
                                                      const uint8_t Sdu_len,
                                                      const uint8_t rsrq,
                                                      const uint8_t rsrp)
{
  NR_BCCH_DL_SCH_Message_t *bcch_message = NULL;

  NR_UE_RRC_SI_INFO *SI_info = &rrc->perNB[gNB_index].SInfo;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_IN);

  asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                 &asn_DEF_NR_BCCH_DL_SCH_Message,
                                                 (void **)&bcch_message,
                                                 (const void *)Sdu,
                                                 Sdu_len);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NR_BCCH_DL_SCH_Message,(void *)bcch_message);
  }

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, "[UE %ld] Failed to decode BCCH_DLSCH_MESSAGE (%zu bits)\n", rrc->ue_id, dec_rval.consumed);
    log_dump(NR_RRC, Sdu, Sdu_len, LOG_DUMP_CHAR,"   Received bytes:\n");
    // free the memory
    SEQUENCE_free(&asn_DEF_NR_BCCH_DL_SCH_Message, (void *)bcch_message, 1);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return -1;
  }

  if (bcch_message->message.present == NR_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1->present) {
      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        LOG_D(NR_RRC, "[UE %ld] Decoding SIB1\n", rrc->ue_id);
        asn1cFreeStruc(asn_DEF_NR_SIB1, SI_info->sib1);
        NR_SIB1_t *sib1 = bcch_message->message.choice.c1->choice.systemInformationBlockType1;
        if(!SI_info->sib1)
          SI_info->sib1 = calloc(1, sizeof(*SI_info->sib1));
        memcpy(SI_info->sib1, sib1, sizeof(NR_SIB1_t));
        if(g_log->log_component[NR_RRC].level >= OAILOG_DEBUG)
          xer_fprint(stdout, &asn_DEF_NR_SIB1, (const void *) SI_info->sib1);
        LOG_A(NR_RRC, "SIB1 decoded\n");
        nr_timer_start(&SI_info->sib1_timer);
        if (rrc->nrRrcState == RRC_STATE_IDLE_NR) {
          rrc->ra_trigger = INITIAL_ACCESS_FROM_RRC_IDLE;
          // preparing RRC setup request payload in advance
          nr_rrc_ue_prepare_RRCSetupRequest(rrc);
        }
        // configure default SI
        nr_rrc_configure_default_SI(SI_info, sib1);
        // configure timers and constant
        nr_rrc_set_sib1_timers_and_constants(&rrc->timers_and_constants, sib1);
        nr_rrc_mac_config_req_sib1(rrc->ue_id, 0, sib1->si_SchedulingInfo, sib1->servingCellConfigCommon);
        break;
      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformation:
        LOG_I(NR_RRC, "[UE %ld] Decoding SI\n", rrc->ue_id);
        NR_SystemInformation_t *si = bcch_message->message.choice.c1->choice.systemInformation;
        nr_decode_SI(SI_info, si);
        SEQUENCE_free(&asn_DEF_NR_BCCH_DL_SCH_Message, (void *)bcch_message, 1);
        break;
      case NR_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
      default:
        break;
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
  return 0;
}

static void nr_rrc_manage_rlc_bearers(const NR_UE_RRC_INST_t *rrc,
                                      const NR_CellGroupConfig_t *cellGroupConfig,
                                      rrcPerNB_t *nb)
{
  if (cellGroupConfig->rlc_BearerToReleaseList != NULL) {
    for (int i = 0; i < cellGroupConfig->rlc_BearerToReleaseList->list.count; i++) {
      NR_LogicalChannelIdentity_t *lcid = cellGroupConfig->rlc_BearerToReleaseList->list.array[i];
      AssertFatal(lcid, "LogicalChannelIdentity shouldn't be null here\n");
      nr_rlc_release_entity(rrc->ue_id, *lcid);
    }
  }

  if (cellGroupConfig->rlc_BearerToAddModList != NULL) {
    for (int i = 0; i < cellGroupConfig->rlc_BearerToAddModList->list.count; i++) {
      NR_RLC_BearerConfig_t *rlc_bearer = cellGroupConfig->rlc_BearerToAddModList->list.array[i];
      NR_LogicalChannelIdentity_t lcid = rlc_bearer->logicalChannelIdentity;
      if (nb->active_RLC_entity[lcid]) {
        if (rlc_bearer->reestablishRLC)
          nr_rlc_reestablish_entity(rrc->ue_id, lcid);
        nr_rlc_reconfigure_entity(rrc->ue_id, lcid, rlc_bearer->rlc_Config);
      } else {
        nb->active_RLC_entity[lcid] = true;
        AssertFatal(rlc_bearer->servedRadioBearer, "servedRadioBearer mandatory in case of setup\n");
        AssertFatal(rlc_bearer->servedRadioBearer->present != NR_RLC_BearerConfig__servedRadioBearer_PR_NOTHING,
                    "Invalid RB for RLC configuration\n");
        if (rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity) {
          NR_SRB_Identity_t srb_id = rlc_bearer->servedRadioBearer->choice.srb_Identity;
          nr_rlc_add_srb(rrc->ue_id, srb_id, rlc_bearer);
        } else { // DRB
          NR_DRB_Identity_t drb_id = rlc_bearer->servedRadioBearer->choice.drb_Identity;
          nr_rlc_add_drb(rrc->ue_id, drb_id, rlc_bearer);
        }
      }
    }
  }
}

void nr_rrc_cellgroup_configuration(rrcPerNB_t *rrcNB, NR_UE_RRC_INST_t *rrc, NR_CellGroupConfig_t *cellGroupConfig)
{
  NR_UE_Timers_Constants_t *tac = &rrc->timers_and_constants;

  NR_SpCellConfig_t *spCellConfig = cellGroupConfig->spCellConfig;
  if(spCellConfig != NULL) {
    if (spCellConfig->reconfigurationWithSync != NULL) {
      NR_ReconfigurationWithSync_t *reconfigurationWithSync = spCellConfig->reconfigurationWithSync;
      // perform Reconfiguration with sync according to 5.3.5.5.2
      if (!rrc->as_security_activated && rrc->nrRrcState != RRC_STATE_IDLE_NR) {
        // perform the actions upon going to RRC_IDLE as specified in 5.3.11
        // with the release cause 'other' upon which the procedure ends
        // TODO
      }
      nr_timer_stop(&tac->T310);
      int t304_value = nr_rrc_get_T304(reconfigurationWithSync->t304);
      nr_timer_setup(&tac->T304, t304_value, 10); // 10ms step
      nr_timer_start(&tac->T304);
      rrc->rnti = reconfigurationWithSync->newUE_Identity;
      // resume suspended radio bearers
      for (int i = 0; i < NR_NUM_SRB; i++) {
        if (rrcNB->Srb[i] == RB_SUSPENDED)
          rrcNB->Srb[i] = RB_ESTABLISHED;
      }
      for (int i = 0; i < MAX_DRBS_PER_UE; i++) {
        if (rrcNB->status_DRBs[i] == RB_SUSPENDED)
          rrcNB->status_DRBs[i] = RB_ESTABLISHED;
      }
      // TODO reset MAC
    }
    nr_rrc_handle_SetupRelease_RLF_TimersAndConstants(rrc, spCellConfig->rlf_TimersAndConstants);
    if (spCellConfig->spCellConfigDedicated) {
      if (spCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id)
        rrc->dl_bwp_id = *spCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id;
      if (spCellConfig->spCellConfigDedicated->uplinkConfig &&
          spCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id)
        rrc->dl_bwp_id = *spCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id;
    }
  }

  nr_rrc_manage_rlc_bearers(rrc, cellGroupConfig, rrcNB);

  AssertFatal(cellGroupConfig->sCellToReleaseList == NULL,
              "Secondary serving cell release not implemented\n");

  AssertFatal(cellGroupConfig->sCellToAddModList == NULL,
              "Secondary serving cell addition not implemented\n");
}


static void nr_rrc_ue_process_masterCellGroup(NR_UE_RRC_INST_t *rrc,
                                              rrcPerNB_t *rrcNB,
                                              OCTET_STRING_t *masterCellGroup,
                                              long *fullConfig)
{
  AssertFatal(!fullConfig, "fullConfig not supported yet\n");
  NR_CellGroupConfig_t *cellGroupConfig = NULL;
  uper_decode(NULL,
              &asn_DEF_NR_CellGroupConfig,   //might be added prefix later
              (void **)&cellGroupConfig,
              (uint8_t *)masterCellGroup->buf,
              masterCellGroup->size, 0, 0);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);
  }

  nr_rrc_cellgroup_configuration(rrcNB, rrc, cellGroupConfig);

  LOG_D(RRC,"Sending CellGroupConfig to MAC\n");
  nr_rrc_mac_config_req_cg(rrc->ue_id, 0, cellGroupConfig);

  asn1cFreeStruc(asn_DEF_NR_CellGroupConfig, cellGroupConfig);
}

static void rrc_ue_generate_RRCSetupComplete(const NR_UE_RRC_INST_t *rrc, const uint8_t Transaction_id)
{
  uint8_t buffer[100];
  uint8_t size;
  const char *nas_msg;
  int   nas_msg_length;

  if (get_softmodem_params()->sa) {
    as_nas_info_t initialNasMsg;
    nr_ue_nas_t *nas = get_ue_nas_info(rrc->ue_id);
    generateRegistrationRequest(&initialNasMsg, nas);
    nas_msg = (char*)initialNasMsg.data;
    nas_msg_length = initialNasMsg.length;
  } else {
    nas_msg = nr_nas_attach_req_imsi_dummy_NSA_case;
    nas_msg_length = sizeof(nr_nas_attach_req_imsi_dummy_NSA_case);
  }

  size = do_RRCSetupComplete(buffer, sizeof(buffer), Transaction_id, rrc->selected_plmn_identity, nas_msg_length, nas_msg);
  LOG_I(NR_RRC, "[UE %ld][RAPROC] Logical Channel UL-DCCH (SRB1), Generating RRCSetupComplete (bytes%d)\n", rrc->ue_id, size);
  int srb_id = 1; // RRC setup complete on SRB1
  LOG_D(NR_RRC, "[RRC_UE %ld] PDCP_DATA_REQ/%d Bytes RRCSetupComplete ---> %d\n", rrc->ue_id, size, srb_id);

  nr_pdcp_data_req_srb(rrc->ue_id, srb_id, 0, size, buffer, deliver_pdu_srb_rlc, NULL);
}

static void nr_rrc_process_rrcsetup(NR_UE_RRC_INST_t *rrc,
                                    const uint8_t gNB_index,
                                    const NR_RRCSetup_t *rrcSetup)
{
  // if the RRCSetup is received in response to an RRCReestablishmentRequest
  // or RRCResumeRequest or RRCResumeRequest1
  // TODO none of the procedures implemented yet

  // perform the cell group configuration procedure in accordance with the received masterCellGroup
  nr_rrc_ue_process_masterCellGroup(rrc,
                                    rrc->perNB + gNB_index,
                                    &rrcSetup->criticalExtensions.choice.rrcSetup->masterCellGroup,
                                    NULL);
  // perform the radio bearer configuration procedure in accordance with the received radioBearerConfig
  nr_rrc_ue_process_RadioBearerConfig(rrc,
                                      rrc->perNB + gNB_index,
                                      &rrcSetup->criticalExtensions.choice.rrcSetup->radioBearerConfig);

  // TODO (not handled) if stored, discard the cell reselection priority information provided by
  // the cellReselectionPriorities or inherited from another RAT

  // stop timer T300, T301, T319, T320 if running;
  NR_UE_Timers_Constants_t *timers = &rrc->timers_and_constants;
  nr_timer_stop(&timers->T300);
  nr_timer_stop(&timers->T301);
  nr_timer_stop(&timers->T319);
  nr_timer_stop(&timers->T320);

  // TODO if T390 and T302 are running (not implemented)

  // if the RRCSetup is received in response to an RRCResumeRequest, RRCResumeRequest1 or RRCSetupRequest
  // enter RRC_CONNECTED
  rrc->nrRrcState = RRC_STATE_CONNECTED_NR;

  // set the content of RRCSetupComplete message
  // TODO procedues described in 5.3.3.4 seems more complex than what we actualy do
  rrc_ue_generate_RRCSetupComplete(rrc, rrcSetup->rrc_TransactionIdentifier);
}

static int8_t nr_rrc_ue_decode_ccch(NR_UE_RRC_INST_t *rrc,
                                    const NRRrcMacCcchDataInd *ind,
                                    const uint8_t gNB_index)
{
  NR_DL_CCCH_Message_t *dl_ccch_msg = NULL;
  asn_dec_rval_t dec_rval;
  int rval=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_IN);
  LOG_D(RRC, "[NR UE%ld] Decoding DL-CCCH message (%d bytes), State %d\n", rrc->ue_id, ind->sdu_size, rrc->nrRrcState);

  dec_rval = uper_decode(NULL, &asn_DEF_NR_DL_CCCH_Message, (void **)&dl_ccch_msg, ind->sdu, ind->sdu_size, 0, 0);

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_DL_CCCH_Message, (void *)dl_ccch_msg);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E(RRC, "[UE %ld] Failed to decode DL-CCCH-Message (%zu bytes)\n", rrc->ue_id, dec_rval.consumed);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
    return -1;
   }

   if (dl_ccch_msg->message.present == NR_DL_CCCH_MessageType_PR_c1) {
     switch (dl_ccch_msg->message.choice.c1->present) {
       case NR_DL_CCCH_MessageType__c1_PR_NOTHING:
         LOG_I(NR_RRC, "[UE%ld] Received PR_NOTHING on DL-CCCH-Message\n", rrc->ue_id);
         rval = 0;
         break;

       case NR_DL_CCCH_MessageType__c1_PR_rrcReject:
         LOG_I(NR_RRC, "[UE%ld] Logical Channel DL-CCCH (SRB0), Received RRCReject \n", rrc->ue_id);
         rval = 0;
         break;

       case NR_DL_CCCH_MessageType__c1_PR_rrcSetup:
         LOG_I(NR_RRC, "[UE%ld][RAPROC] Logical Channel DL-CCCH (SRB0), Received NR_RRCSetup\n", rrc->ue_id);
         nr_rrc_process_rrcsetup(rrc, gNB_index, dl_ccch_msg->message.choice.c1->choice.rrcSetup);
         rval = 0;
         break;

       default:
         LOG_E(NR_RRC, "[UE%ld] Unknown message\n", rrc->ue_id);
         rval = -1;
         break;
     }
   }

   ASN_STRUCT_FREE(asn_DEF_NR_DL_CCCH_Message, dl_ccch_msg);
   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
   return rval;
}

static void nr_rrc_ue_process_securityModeCommand(NR_UE_RRC_INST_t *ue_rrc,
                                                  NR_SecurityModeCommand_t *const securityModeCommand,
                                                  const uint8_t gNB_index)
{
  int securityMode = 0;
  LOG_I(NR_RRC, "Receiving from SRB1 (DL-DCCH), Processing securityModeCommand (eNB %d)\n", gNB_index);

  NR_SecurityConfigSMC_t *securityConfigSMC =
      &securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC;

  switch (securityConfigSMC->securityAlgorithmConfig.cipheringAlgorithm) {
    case NR_CipheringAlgorithm_nea0:
    case NR_CipheringAlgorithm_nea1:
    case NR_CipheringAlgorithm_nea2:
      LOG_I(NR_RRC, "Security algorithm is set to nea%d\n", securityMode);
      securityMode = securityConfigSMC->securityAlgorithmConfig.cipheringAlgorithm;
      break;
    default:
      LOG_W(NR_RRC, "Security algorithm is set to none\n");
      securityMode = NR_CipheringAlgorithm_spare1;
      break;
  }
  ue_rrc->cipheringAlgorithm = securityConfigSMC->securityAlgorithmConfig.cipheringAlgorithm;

  if (securityConfigSMC->securityAlgorithmConfig.integrityProtAlgorithm != NULL) {
    switch (*securityConfigSMC->securityAlgorithmConfig.integrityProtAlgorithm) {
      case NR_IntegrityProtAlgorithm_nia1:
        LOG_I(NR_RRC, "Integrity protection algorithm is set to nia1\n");
        securityMode |= 1 << 5;
        break;

      case NR_IntegrityProtAlgorithm_nia2:
        LOG_I(NR_RRC, "Integrity protection algorithm is set to nia2\n");
        securityMode |= 1 << 6;
        break;

      default:
        LOG_I(NR_RRC, "Integrity protection algorithm is set to none\n");
        securityMode |= 0x70;
        break;
    }

    ue_rrc->integrityProtAlgorithm = *securityConfigSMC->securityAlgorithmConfig.integrityProtAlgorithm;
  }

  LOG_D(NR_RRC, "security mode is %x \n", securityMode);
  NR_UL_DCCH_Message_t ul_dcch_msg = {0};

  // memset((void *)&SecurityModeCommand,0,sizeof(SecurityModeCommand_t));
  ul_dcch_msg.message.present = NR_UL_DCCH_MessageType_PR_c1;
  asn1cCalloc(ul_dcch_msg.message.choice.c1, c1);

  if (securityMode >= NO_SECURITY_MODE) {
    LOG_I(NR_RRC, "rrc_ue_process_securityModeCommand, security mode complete case \n");
    c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
  } else {
    LOG_I(NR_RRC, "rrc_ue_process_securityModeCommand, security mode failure case \n");
    c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeFailure;
    c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
  }

  uint8_t kRRCenc[16] = {0};
  uint8_t kUPenc[16] = {0};
  uint8_t kRRCint[16] = {0};
  nr_derive_key(UP_ENC_ALG, ue_rrc->cipheringAlgorithm, ue_rrc->kgnb, kUPenc);
  nr_derive_key(RRC_ENC_ALG, ue_rrc->cipheringAlgorithm, ue_rrc->kgnb, kRRCenc);
  nr_derive_key(RRC_INT_ALG, ue_rrc->integrityProtAlgorithm, ue_rrc->kgnb, kRRCint);

  log_dump(NR_RRC, ue_rrc->kgnb, 32, LOG_DUMP_CHAR, "driving kRRCenc, kRRCint and kUPenc from KgNB=");

  if (securityMode != 0xff) {
    uint8_t security_mode = ue_rrc->cipheringAlgorithm | (ue_rrc->integrityProtAlgorithm << 4);
    // configure lower layers to apply SRB integrity protection and ciphering
    for (int i = 1; i < NR_NUM_SRB; i++) {
      if (ue_rrc->perNB[gNB_index].Srb[i] == RB_ESTABLISHED)
        nr_pdcp_config_set_security(ue_rrc->ue_id, i, security_mode, kRRCenc, kRRCint, kUPenc);
    }
  } else {
    LOG_I(NR_RRC, "skipped pdcp_config_set_security() as securityMode == 0x%02x", securityMode);
  }

  if (securityModeCommand->criticalExtensions.present == NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand) {
    asn1cCalloc(c1->choice.securityModeComplete, modeComplete);
    modeComplete->rrc_TransactionIdentifier = securityModeCommand->rrc_TransactionIdentifier;
    modeComplete->criticalExtensions.present = NR_SecurityModeComplete__criticalExtensions_PR_securityModeComplete;
    asn1cCalloc(modeComplete->criticalExtensions.choice.securityModeComplete, ext);
    ext->nonCriticalExtension = NULL;
    LOG_I(NR_RRC,
          "Receiving from SRB1 (DL-DCCH), encoding securityModeComplete (gNB %d), rrc_TransactionIdentifier: %ld\n",
          gNB_index,
          securityModeCommand->rrc_TransactionIdentifier);
    uint8_t buffer[200];
    asn_enc_rval_t enc_rval =
        uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message, NULL, (void *)&ul_dcch_msg, buffer, sizeof(buffer));
    AssertFatal(enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n", enc_rval.failed_type->name, enc_rval.encoded);

    if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
      xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
    }
    log_dump(NR_RRC, buffer, 16, LOG_DUMP_CHAR, "securityModeComplete payload: ");
    LOG_D(NR_RRC, "securityModeComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded + 7) / 8);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NR_UL_DCCH_Message, &ul_dcch_msg);

    for (int i = 0; i < (enc_rval.encoded + 7) / 8; i++) {
      LOG_T(NR_RRC, "%02x.", buffer[i]);
    }
    LOG_T(NR_RRC, "\n");

    // TODO the SecurityModeCommand message needs to pass the integrity protection check
    //  for the UE to declare AS security to be activated
    ue_rrc->as_security_activated = true;
    int srb_id = 1; // SecurityModeComplete in SRB1
    nr_pdcp_data_req_srb(ue_rrc->ue_id, srb_id, 0, (enc_rval.encoded + 7) / 8, buffer, deliver_pdu_srb_rlc, NULL);
  } else
    LOG_W(NR_RRC,
          "securityModeCommand->criticalExtensions.present (%d) != "
          "NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand\n",
          securityModeCommand->criticalExtensions.present);
}

 //-----------------------------------------------------------------------------
void nr_rrc_ue_process_measConfig(rrcPerNB_t *rrc, NR_MeasConfig_t *const measConfig)
//-----------------------------------------------------------------------------
{
  int i;
  long ind;
  NR_MeasObjectToAddMod_t *measObj = NULL;
  NR_ReportConfigToAddMod_t *reportConfig = NULL;

  if (measConfig->measObjectToRemoveList != NULL) {
    for (i = 0; i < measConfig->measObjectToRemoveList->list.count; i++) {
      ind = *measConfig->measObjectToRemoveList->list.array[i];
      free(rrc->MeasObj[ind - 1]);
    }
  }

  if (measConfig->measObjectToAddModList != NULL) {
    LOG_I(NR_RRC, "Measurement Object List is present\n");
    for (i = 0; i < measConfig->measObjectToAddModList->list.count; i++) {
      measObj = measConfig->measObjectToAddModList->list.array[i];
      ind = measConfig->measObjectToAddModList->list.array[i]->measObjectId;

      if (rrc->MeasObj[ind - 1]) {
        LOG_D(NR_RRC, "Modifying measurement object %ld\n", ind);
        memcpy(rrc->MeasObj[ind - 1], (char *)measObj, sizeof(NR_MeasObjectToAddMod_t));
      } else {
        LOG_I(NR_RRC, "Adding measurement object %ld\n", ind);

        if (measObj->measObject.present == NR_MeasObjectToAddMod__measObject_PR_measObjectNR) {
          rrc->MeasObj[ind - 1] = measObj;
        }
      }
    }

    LOG_I(NR_RRC, "call rrc_mac_config_req \n");
    // rrc_mac_config_req_ue
  }

  if (measConfig->reportConfigToRemoveList != NULL) {
    for (i = 0; i < measConfig->reportConfigToRemoveList->list.count; i++) {
      ind = *measConfig->reportConfigToRemoveList->list.array[i];
      free(rrc->ReportConfig[ind - 1]);
    }
  }

  if (measConfig->reportConfigToAddModList != NULL) {
    LOG_I(NR_RRC, "Report Configuration List is present\n");
    for (i = 0; i < measConfig->reportConfigToAddModList->list.count; i++) {
      ind = measConfig->reportConfigToAddModList->list.array[i]->reportConfigId;
      reportConfig = measConfig->reportConfigToAddModList->list.array[i];

      if (rrc->ReportConfig[ind - 1]) {
        LOG_I(NR_RRC, "Modifying Report Configuration %ld\n", ind - 1);
        memcpy(rrc->ReportConfig[ind - 1],
               (char *)measConfig->reportConfigToAddModList->list.array[i],
               sizeof(NR_ReportConfigToAddMod_t));
      } else {
        LOG_D(NR_RRC, "Adding Report Configuration %ld %p \n", ind - 1, measConfig->reportConfigToAddModList->list.array[i]);
        if (reportConfig->reportConfig.present == NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR) {
          rrc->ReportConfig[ind - 1] = measConfig->reportConfigToAddModList->list.array[i];
        }
      }
    }
  }

  if (measConfig->measIdToRemoveList != NULL) {
    for (i = 0; i < measConfig->measIdToRemoveList->list.count; i++) {
      ind = *measConfig->measIdToRemoveList->list.array[i];
      free(rrc->MeasId[ind - 1]);
    }
  }

  if (measConfig->measIdToAddModList != NULL) {
    for (i = 0; i < measConfig->measIdToAddModList->list.count; i++) {
      ind = measConfig->measIdToAddModList->list.array[i]->measId;

      if (rrc->MeasId[ind - 1]) {
        LOG_D(NR_RRC, "Modifying Measurement ID %ld\n", ind - 1);
        memcpy(rrc->MeasId[ind - 1], (char *)measConfig->measIdToAddModList->list.array[i], sizeof(NR_MeasIdToAddMod_t));
      } else {
        LOG_D(NR_RRC, "Adding Measurement ID %ld %p\n", ind - 1, measConfig->measIdToAddModList->list.array[i]);
        rrc->MeasId[ind - 1] = measConfig->measIdToAddModList->list.array[i];
      }
    }
  }

  if (measConfig->quantityConfig != NULL) {
    if (rrc->QuantityConfig) {
      LOG_D(NR_RRC, "Modifying Quantity Configuration \n");
      memcpy(rrc->QuantityConfig, (char *)measConfig->quantityConfig, sizeof(NR_QuantityConfig_t));
    } else {
      LOG_D(NR_RRC, "Adding Quantity configuration\n");
      rrc->QuantityConfig = measConfig->quantityConfig;
    }
  }

  if (measConfig->measGapConfig != NULL) {
    if (rrc->measGapConfig) {
      memcpy(rrc->measGapConfig, (char *)measConfig->measGapConfig, sizeof(NR_MeasGapConfig_t));
    } else {
      rrc->measGapConfig = measConfig->measGapConfig;
    }
  }

  if (measConfig->s_MeasureConfig) {
    if (measConfig->s_MeasureConfig->present == NR_MeasConfig__s_MeasureConfig_PR_ssb_RSRP) {
      rrc->s_measure = measConfig->s_MeasureConfig->choice.ssb_RSRP;
    } else if (measConfig->s_MeasureConfig->present == NR_MeasConfig__s_MeasureConfig_PR_csi_RSRP) {
      rrc->s_measure = measConfig->s_MeasureConfig->choice.csi_RSRP;
    }
  }
}

static void nr_rrc_ue_process_RadioBearerConfig(NR_UE_RRC_INST_t *ue_rrc,
                                                rrcPerNB_t *rrcNB,
                                                NR_RadioBearerConfig_t *const radioBearerConfig)
{
  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void *)radioBearerConfig);

  if (radioBearerConfig->srb3_ToRelease)
    nr_pdcp_release_srb(ue_rrc->ue_id, 3);

  uint8_t kRRCenc[16] = {0};
  uint8_t kRRCint[16] = {0};
  if (ue_rrc->as_security_activated) {
    if (radioBearerConfig->securityConfig != NULL) {
      // When the field is not included, continue to use the currently configured keyToUse
      if (radioBearerConfig->securityConfig->keyToUse) {
        AssertFatal(*radioBearerConfig->securityConfig->keyToUse == NR_SecurityConfig__keyToUse_master,
                    "Secondary key usage seems not to be implemented\n");
        ue_rrc->keyToUse = *radioBearerConfig->securityConfig->keyToUse;
      }
      // When the field is not included, continue to use the currently configured security algorithm
      if (radioBearerConfig->securityConfig->securityAlgorithmConfig) {
        ue_rrc->cipheringAlgorithm = radioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm;
        ue_rrc->integrityProtAlgorithm = *radioBearerConfig->securityConfig->securityAlgorithmConfig->integrityProtAlgorithm;
      }
    }
    nr_derive_key(RRC_ENC_ALG, ue_rrc->cipheringAlgorithm, ue_rrc->kgnb, kRRCenc);
    nr_derive_key(RRC_INT_ALG, ue_rrc->integrityProtAlgorithm, ue_rrc->kgnb, kRRCint);
  }

  if (radioBearerConfig->srb_ToAddModList != NULL) {
    for (int cnt = 0; cnt < radioBearerConfig->srb_ToAddModList->list.count; cnt++) {
      struct NR_SRB_ToAddMod *srb = radioBearerConfig->srb_ToAddModList->list.array[cnt];
      if (rrcNB->Srb[srb->srb_Identity] == RB_NOT_PRESENT)
        add_srb(false,
                ue_rrc->ue_id,
                radioBearerConfig->srb_ToAddModList->list.array[cnt],
                ue_rrc->cipheringAlgorithm,
                ue_rrc->integrityProtAlgorithm,
                kRRCenc,
                kRRCint);
      else {
        AssertFatal(srb->discardOnPDCP == NULL, "discardOnPDCP not yet implemented\n");
        AssertFatal(srb->reestablishPDCP == NULL, "reestablishPDCP not yet implemented\n");
        if (srb->pdcp_Config && srb->pdcp_Config->t_Reordering)
          nr_pdcp_reconfigure_srb(ue_rrc->ue_id, srb->srb_Identity, *srb->pdcp_Config->t_Reordering);
      }
      rrcNB->Srb[srb->srb_Identity] = RB_ESTABLISHED;
    }
  }

  if (radioBearerConfig->drb_ToReleaseList) {
    for (int cnt = 0; cnt < radioBearerConfig->drb_ToReleaseList->list.count; cnt++) {
      NR_DRB_Identity_t *DRB_id = radioBearerConfig->drb_ToReleaseList->list.array[cnt];
      if (DRB_id)
        nr_pdcp_release_drb(ue_rrc->ue_id, *DRB_id);
    }
  }

  // Establish DRBs if present
  if (radioBearerConfig->drb_ToAddModList != NULL) {
    for (int cnt = 0; cnt < radioBearerConfig->drb_ToAddModList->list.count; cnt++) {
      struct NR_DRB_ToAddMod *drb = radioBearerConfig->drb_ToAddModList->list.array[cnt];
      int DRB_id = drb->drb_Identity;
      if (rrcNB->status_DRBs[DRB_id] == RB_ESTABLISHED) {
        AssertFatal(drb->reestablishPDCP == NULL, "reestablishPDCP not yet implemented\n");
        AssertFatal(drb->recoverPDCP == NULL, "recoverPDCP not yet implemented\n");
        NR_SDAP_Config_t *sdap_Config = drb->cnAssociation ? drb->cnAssociation->choice.sdap_Config : NULL;
        if (drb->pdcp_Config || sdap_Config)
          nr_pdcp_reconfigure_drb(ue_rrc->ue_id, DRB_id, drb->pdcp_Config, sdap_Config);
        if (drb->cnAssociation)
          AssertFatal(drb->cnAssociation->choice.sdap_Config == NULL, "SDAP reconfiguration not yet implemented\n");
      } else {
        rrcNB->status_DRBs[DRB_id] = RB_ESTABLISHED;
        add_drb(false,
                ue_rrc->ue_id,
                radioBearerConfig->drb_ToAddModList->list.array[cnt],
                ue_rrc->cipheringAlgorithm,
                ue_rrc->integrityProtAlgorithm,
                kRRCenc,
                kRRCint);
      }
    }
  } // drb_ToAddModList //

  ue_rrc->nrRrcState = RRC_STATE_CONNECTED_NR;
  LOG_I(NR_RRC, "State = NR_RRC_CONNECTED\n");
}

static void nr_rrc_ue_generate_RRCReconfigurationComplete(NR_UE_RRC_INST_t *rrc, const int srb_id, const uint8_t Transaction_id)
{
  uint8_t buffer[32], size;
  size = do_NR_RRCReconfigurationComplete(buffer, sizeof(buffer), Transaction_id);
  LOG_I(NR_RRC, " Logical Channel UL-DCCH (SRB1), Generating RRCReconfigurationComplete (bytes %d)\n", size);
  AssertFatal(srb_id == 1 || srb_id == 3, "Invalid SRB ID %d\n", srb_id);
  LOG_D(RLC,
        "PDCP_DATA_REQ/%d Bytes (RRCReconfigurationComplete) "
        "--->][PDCP][RB %02d]\n",
        size,
        srb_id);
  nr_pdcp_data_req_srb(rrc->ue_id, srb_id, 0, size, buffer, deliver_pdu_srb_rlc, NULL);
}

static int nr_rrc_ue_decode_dcch(NR_UE_RRC_INST_t *rrc,
                                 const srb_id_t Srb_id,
                                 const uint8_t *const Buffer,
                                 size_t Buffer_size,
                                 const uint8_t gNB_indexP)

{
  NR_DL_DCCH_Message_t *dl_dcch_msg = NULL;
  if (Srb_id != 1 && Srb_id != 2) {
    LOG_E(NR_RRC, "Received message on DL-DCCH (SRB%ld), should not have ...\n", Srb_id);
  }

  LOG_D(NR_RRC, "Decoding DL-DCCH Message\n");
  asn_dec_rval_t dec_rval = uper_decode(NULL, &asn_DEF_NR_DL_DCCH_Message, (void **)&dl_dcch_msg, Buffer, Buffer_size, 0, 0);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, "Failed to decode DL-DCCH (%zu bytes)\n", dec_rval.consumed);
    ASN_STRUCT_FREE(asn_DEF_NR_DL_DCCH_Message, dl_dcch_msg);
    return -1;
  }

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message, (void *)dl_dcch_msg);
  }

  switch (dl_dcch_msg->message.present) {
    case NR_DL_DCCH_MessageType_PR_c1: {
      struct NR_DL_DCCH_MessageType__c1 *c1 = dl_dcch_msg->message.choice.c1;
      switch (c1->present) {
        case NR_DL_DCCH_MessageType__c1_PR_NOTHING:
          LOG_I(NR_RRC, "Received PR_NOTHING on DL-DCCH-Message\n");
          break;

        case NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration: {
          nr_rrc_ue_process_rrcReconfiguration(rrc, gNB_indexP, c1->choice.rrcReconfiguration);
          nr_rrc_ue_generate_RRCReconfigurationComplete(rrc, Srb_id, c1->choice.rrcReconfiguration->rrc_TransactionIdentifier);
        } break;

        case NR_DL_DCCH_MessageType__c1_PR_rrcResume:
          LOG_I(NR_RRC, "Received rrcResume on DL-DCCH-Message\n");
          break;
        case NR_DL_DCCH_MessageType__c1_PR_rrcRelease:
          LOG_I(NR_RRC, "[UE %ld] Received RRC Release (gNB %d)\n", rrc->ue_id, gNB_indexP);
          // TODO properly implement procedures in 5.3.8.3 of 38.331
          NR_Release_Cause_t cause = OTHER;
          nr_rrc_going_to_IDLE(rrc, cause, dl_dcch_msg->message.choice.c1->choice.rrcRelease);
          break;

        case NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
          LOG_I(NR_RRC, "Received Capability Enquiry (gNB %d)\n", gNB_indexP);
          nr_rrc_ue_process_ueCapabilityEnquiry(rrc, c1->choice.ueCapabilityEnquiry);
          break;

        case NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment:
          LOG_I(NR_RRC, "Logical Channel DL-DCCH (SRB1), Received RRCReestablishment\n");
          nr_rrc_ue_generate_rrcReestablishmentComplete(c1->choice.rrcReestablishment);
          break;

        case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer: {
          NR_DLInformationTransfer_t *dlInformationTransfer = c1->choice.dlInformationTransfer;

          if (dlInformationTransfer->criticalExtensions.present
              == NR_DLInformationTransfer__criticalExtensions_PR_dlInformationTransfer) {
            /* This message hold a dedicated info NAS payload, forward it to NAS */
            NR_DedicatedNAS_Message_t *dedicatedNAS_Message =
                dlInformationTransfer->criticalExtensions.choice.dlInformationTransfer->dedicatedNAS_Message;

            MessageDef *ittiMsg = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_DOWNLINK_DATA_IND);
            NasDlDataInd *msg = &NAS_DOWNLINK_DATA_IND(ittiMsg);
            msg->UEid = rrc->ue_id;
            msg->nasMsg.length = dedicatedNAS_Message->size;
            msg->nasMsg.data = dedicatedNAS_Message->buf;
            itti_send_msg_to_task(TASK_NAS_NRUE, rrc->ue_id, ittiMsg);
            dedicatedNAS_Message->buf = NULL; // to keep the buffer, up to NAS to free it
          }
        } break;
        case NR_DL_DCCH_MessageType__c1_PR_mobilityFromNRCommand:
        case NR_DL_DCCH_MessageType__c1_PR_dlDedicatedMessageSegment_r16:
        case NR_DL_DCCH_MessageType__c1_PR_ueInformationRequest_r16:
        case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransferMRDC_r16:
        case NR_DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r16:
        case NR_DL_DCCH_MessageType__c1_PR_spare3:
        case NR_DL_DCCH_MessageType__c1_PR_spare2:
        case NR_DL_DCCH_MessageType__c1_PR_spare1:
        case NR_DL_DCCH_MessageType__c1_PR_counterCheck:
          break;
        case NR_DL_DCCH_MessageType__c1_PR_securityModeCommand:
          LOG_I(NR_RRC, "Received securityModeCommand (gNB %d)\n", gNB_indexP);
          nr_rrc_ue_process_securityModeCommand(rrc, c1->choice.securityModeCommand, gNB_indexP);
          break;
      }
    } break;
    default:
      break;
  }
  //  release memory allocation
  SEQUENCE_free(&asn_DEF_NR_DL_DCCH_Message, (void *)dl_dcch_msg, 1);
  return 0;
}

void nr_rrc_handle_ra_indication(NR_UE_RRC_INST_t *rrc, bool ra_succeeded)
{
  NR_UE_Timers_Constants_t *timers = &rrc->timers_and_constants;
  if (ra_succeeded && is_nr_timer_active(timers->T304)) {
    // successful Random Access procedure triggered by reconfigurationWithSync
    nr_timer_stop(&timers->T304);
    // TODO handle the rest of procedures as described in 5.3.5.3 for when
    // reconfigurationWithSync is included in spCellConfig
  }
}

void *rrc_nrue_task(void *args_p)
{
  itti_mark_task_ready(TASK_RRC_NRUE);
  while (1) {
    rrc_nrue(NULL);
  }
}

void *rrc_nrue(void *notUsed)
{
  MessageDef *msg_p = NULL;
  itti_receive_msg(TASK_RRC_NRUE, &msg_p);
  instance_t instance = ITTI_MSG_DESTINATION_INSTANCE(msg_p);
  LOG_D(NR_RRC, "[UE %ld] Received %s\n", instance, ITTI_MSG_NAME(msg_p));

  NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[instance];
  AssertFatal(instance == rrc->ue_id, "Instance %ld received from ITTI doesn't matach with UE-ID %ld\n", instance, rrc->ue_id);

  switch (ITTI_MSG_ID(msg_p)) {
  case TERMINATE_MESSAGE:
    LOG_W(NR_RRC, " *** Exiting RRC thread\n");
    itti_exit_task();
    break;

  case MESSAGE_TEST:
    break;

  case NR_RRC_MAC_SYNC_IND: {
    nr_sync_msg_t sync_msg = NR_RRC_MAC_SYNC_IND(msg_p).in_sync ? IN_SYNC : OUT_OF_SYNC;
    NR_UE_Timers_Constants_t *tac = &rrc->timers_and_constants;
    handle_rlf_sync(tac, sync_msg);
  } break;

  case NRRRC_FRAME_PROCESS:
    LOG_D(NR_RRC, "Received %s: frame %d\n", ITTI_MSG_NAME(msg_p), NRRRC_FRAME_PROCESS(msg_p).frame);
    // increase the timers every 10ms (every new frame)
    nr_rrc_handle_timers(rrc);
    NR_UE_RRC_SI_INFO *SInfo = &rrc->perNB[NRRRC_FRAME_PROCESS(msg_p).gnb_id].SInfo;
    nr_rrc_SI_timers(SInfo);
    break;

  case NR_RRC_MAC_MSG3_IND:
    nr_rrc_handle_msg3_indication(rrc, NR_RRC_MAC_MSG3_IND(msg_p).rnti);
    break;

  case NR_RRC_MAC_RA_IND:
    LOG_D(NR_RRC,
	  "[UE %ld] Received %s: frame %d RA %s\n",
	  rrc->ue_id,
	  ITTI_MSG_NAME(msg_p),
	  NR_RRC_MAC_RA_IND(msg_p).frame,
	  NR_RRC_MAC_RA_IND(msg_p).RA_succeeded ? "successful" : "failed");
    nr_rrc_handle_ra_indication(rrc, NR_RRC_MAC_RA_IND(msg_p).RA_succeeded);
    break;

  case NR_RRC_MAC_BCCH_DATA_IND:
    LOG_D(NR_RRC, "[UE %ld] Received %s: gNB %d\n", rrc->ue_id, ITTI_MSG_NAME(msg_p), NR_RRC_MAC_BCCH_DATA_IND(msg_p).gnb_index);
    NRRrcMacBcchDataInd *bcch = &NR_RRC_MAC_BCCH_DATA_IND(msg_p);
    if (bcch->is_bch)
      nr_rrc_ue_decode_NR_BCCH_BCH_Message(rrc, bcch->gnb_index, bcch->sdu, bcch->sdu_size);
    else
      nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message(rrc, bcch->gnb_index, bcch->sdu, bcch->sdu_size, bcch->rsrq, bcch->rsrp);
    break;

  case NR_RRC_MAC_SBCCH_DATA_IND:
    LOG_D(NR_RRC, "[UE %ld] Received %s: gNB %d\n", instance, ITTI_MSG_NAME(msg_p), NR_RRC_MAC_SBCCH_DATA_IND(msg_p).gnb_index);
    NRRrcMacSBcchDataInd *sbcch = &NR_RRC_MAC_SBCCH_DATA_IND(msg_p);
    
    nr_rrc_ue_decode_NR_SBCCH_SL_BCH_Message(rrc, sbcch->gnb_index,sbcch->frame, sbcch->slot, sbcch->sdu,
                                             sbcch->sdu_size, sbcch->rx_slss_id);
    break;

  case NR_RRC_MAC_CCCH_DATA_IND: {
    NRRrcMacCcchDataInd *ind = &NR_RRC_MAC_CCCH_DATA_IND(msg_p);
    nr_rrc_ue_decode_ccch(rrc, ind, 0);
  } break;

  case NR_RRC_DCCH_DATA_IND:
    nr_rrc_ue_decode_dcch(rrc,
			  NR_RRC_DCCH_DATA_IND(msg_p).dcch_index,
			  NR_RRC_DCCH_DATA_IND(msg_p).sdu_p,
			  NR_RRC_DCCH_DATA_IND(msg_p).sdu_size,
			  NR_RRC_DCCH_DATA_IND(msg_p).gNB_index);
    break;

  case NAS_KENB_REFRESH_REQ:
    memcpy(rrc->kgnb, NAS_KENB_REFRESH_REQ(msg_p).kenb, sizeof(rrc->kgnb));
    break;

  case NAS_DETACH_REQ:
    if (!NAS_DETACH_REQ(msg_p).wait_release) {
      rrc->nrRrcState = RRC_STATE_DETACH_NR;
      NR_Release_Cause_t release_cause = OTHER;
      nr_rrc_going_to_IDLE(rrc, release_cause, NULL);
    }
    break;

  case NAS_UPLINK_DATA_REQ: {
    uint32_t length;
    uint8_t *buffer;
    NasUlDataReq *req = &NAS_UPLINK_DATA_REQ(msg_p);
    /* Create message for PDCP (ULInformationTransfer_t) */
    length = do_NR_ULInformationTransfer(&buffer, req->nasMsg.length, req->nasMsg.data);
    /* Transfer data to PDCP */
    // check if SRB2 is created, if yes request data_req on SRB2
    // error: the remote gNB is hardcoded here
    rb_id_t srb_id = rrc->perNB[0].Srb[2] == RB_ESTABLISHED ? 2 : 1;
    nr_pdcp_data_req_srb(rrc->ue_id, srb_id, 0, length, buffer, deliver_pdu_srb_rlc, NULL);
    break;
  }

  default:
    LOG_E(NR_RRC, "[UE %ld] Received unexpected message %s\n", rrc->ue_id, ITTI_MSG_NAME(msg_p));
    break;
  }
  LOG_D(NR_RRC, "[UE %ld] RRC Status %d\n", rrc->ue_id, rrc->nrRrcState);
  int result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
  AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  return NULL;
}

void nr_rrc_ue_process_sidelink_radioResourceConfig(NR_SetupRelease_SL_ConfigDedicatedNR_r16_t *sl_ConfigDedicatedNR)
{
  //process sl_CommConfig, configure MAC/PHY for transmitting SL communication (RRC_CONNECTED)
  if (sl_ConfigDedicatedNR != NULL) {
    switch (sl_ConfigDedicatedNR->present){
      case NR_SetupRelease_SL_ConfigDedicatedNR_r16_PR_setup:
        //TODO
        break;
      case NR_SetupRelease_SL_ConfigDedicatedNR_r16_PR_release:
        break;
      case NR_SetupRelease_SL_ConfigDedicatedNR_r16_PR_NOTHING:
        break;
      default:
        break;
    }
  }
}

static void nr_rrc_ue_process_ueCapabilityEnquiry(NR_UE_RRC_INST_t *rrc, NR_UECapabilityEnquiry_t *UECapabilityEnquiry)
{
  NR_UL_DCCH_Message_t ul_dcch_msg = {0};
  //
  LOG_I(NR_RRC, "Receiving from SRB1 (DL-DCCH), Processing UECapabilityEnquiry\n");

  ul_dcch_msg.message.present = NR_UL_DCCH_MessageType_PR_c1;
  asn1cCalloc(ul_dcch_msg.message.choice.c1, c1);
  c1->present = NR_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation;
  asn1cCalloc(c1->choice.ueCapabilityInformation, info);
  info->rrc_TransactionIdentifier = UECapabilityEnquiry->rrc_TransactionIdentifier;
  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container = {.rat_Type = NR_RAT_Type_nr};

  char *file_path = rrc->uecap_file;

  FILE *f = NULL;
  if (file_path)
    f = fopen(file_path, "r");
  if(f){
    char UE_NR_Capability_xer[65536];
    size_t size = fread(UE_NR_Capability_xer, 1, sizeof UE_NR_Capability_xer, f);
    if (size == 0 || size == sizeof UE_NR_Capability_xer) {
      LOG_E(NR_RRC, "UE Capabilities XER file %s is too large (%ld)\n", file_path, size);
      return;
    }
    asn_dec_rval_t dec_rval =
        xer_decode(0, &asn_DEF_NR_UE_NR_Capability, (void *)&rrc->UECap.UE_NR_Capability, UE_NR_Capability_xer, size);
    assert(dec_rval.code == RC_OK);
  }
  else {
    rrc->UECap.UE_NR_Capability = CALLOC(1, sizeof(NR_UE_NR_Capability_t));
    asn1cSequenceAdd(rrc->UECap.UE_NR_Capability->rf_Parameters.supportedBandListNR.list, NR_BandNR_t, nr_bandnr);
    nr_bandnr->bandNR = 1;
  }
  xer_fprint(stdout, &asn_DEF_NR_UE_NR_Capability, (void *)rrc->UECap.UE_NR_Capability);

  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UE_NR_Capability,
                                                  NULL,
                                                  (void *)rrc->UECap.UE_NR_Capability,
                                                  &rrc->UECap.sdu[0],
                                                  MAX_UE_NR_CAPABILITY_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  rrc->UECap.sdu_size = (enc_rval.encoded + 7) / 8;
  LOG_I(PHY, "[RRC]UE NR Capability encoded, %d bytes (%zd bits)\n", rrc->UECap.sdu_size, enc_rval.encoded + 7);

  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container.ue_CapabilityRAT_Container, (const char *)rrc->UECap.sdu, rrc->UECap.sdu_size);

  NR_UECapabilityEnquiry_IEs_t *ueCapabilityEnquiry_ie = UECapabilityEnquiry->criticalExtensions.choice.ueCapabilityEnquiry;
  if (get_softmodem_params()->nsa == 1) {
    OCTET_STRING_t *requestedFreqBandsNR = ueCapabilityEnquiry_ie->ue_CapabilityEnquiryExt;
    nsa_sendmsg_to_lte_ue(requestedFreqBandsNR->buf, requestedFreqBandsNR->size, UE_CAPABILITY_INFO);
  }
  //  ue_CapabilityRAT_Container.ueCapabilityRAT_Container.buf  = UE_rrc_inst[ue_mod_idP].UECapability;
  // ue_CapabilityRAT_Container.ueCapabilityRAT_Container.size = UE_rrc_inst[ue_mod_idP].UECapability_size;
  AssertFatal(UECapabilityEnquiry->criticalExtensions.present == NR_UECapabilityEnquiry__criticalExtensions_PR_ueCapabilityEnquiry,
              "UECapabilityEnquiry->criticalExtensions.present (%d) != UECapabilityEnquiry__criticalExtensions_PR_c1 (%d)\n",
              UECapabilityEnquiry->criticalExtensions.present,NR_UECapabilityEnquiry__criticalExtensions_PR_ueCapabilityEnquiry);

  NR_UECapabilityInformation_t *ueCapabilityInformation = ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation;
  ueCapabilityInformation->criticalExtensions.present = NR_UECapabilityInformation__criticalExtensions_PR_ueCapabilityInformation;
  asn1cCalloc(ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation, infoIE);
  asn1cCalloc(infoIE->ue_CapabilityRAT_ContainerList, UEcapList);
  UEcapList->list.count = 0;

  for (int i = 0; i < ueCapabilityEnquiry_ie->ue_CapabilityRAT_RequestList.list.count; i++) {
    if (ueCapabilityEnquiry_ie->ue_CapabilityRAT_RequestList.list.array[i]->rat_Type == NR_RAT_Type_nr) {
      asn1cSeqAdd(&UEcapList->list, &ue_CapabilityRAT_Container);
      uint8_t buffer[500];
      asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message, NULL, (void *)&ul_dcch_msg, buffer, 500);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                   enc_rval.failed_type->name, enc_rval.encoded);

      if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
        xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
      }
      LOG_I(NR_RRC, "UECapabilityInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
      int srb_id = 1; // UECapabilityInformation on SRB1
      nr_pdcp_data_req_srb(rrc->ue_id, srb_id, 0, (enc_rval.encoded + 7) / 8, buffer, deliver_pdu_srb_rlc, NULL);
    }
  }
}

static void nr_rrc_ue_generate_rrcReestablishmentComplete(NR_RRCReestablishment_t *rrcReestablishment)
//-----------------------------------------------------------------------------
{
    uint8_t buffer[RRC_BUFFER_SIZE] = {0};
    int size = do_RRCReestablishmentComplete(buffer, RRC_BUFFER_SIZE,
                                           rrcReestablishment->rrc_TransactionIdentifier);
    LOG_I(NR_RRC, "[RAPROC] Logical Channel UL-DCCH (SRB1), Generating RRCReestablishmentComplete (bytes %d)\n", size);
}

void *recv_msgs_from_lte_ue(void *args_p)
{
    itti_mark_task_ready (TASK_RRC_NSA_NRUE);
    int from_lte_ue_fd = get_from_lte_ue_fd();
    for (;;)
    {
        nsa_msg_t msg;
        int recvLen = recvfrom(from_lte_ue_fd, &msg, sizeof(msg),
                               MSG_WAITALL | MSG_TRUNC, NULL, NULL);
        if (recvLen == -1)
        {
            LOG_E(NR_RRC, "%s: recvfrom: %s\n", __func__, strerror(errno));
            continue;
        }
        if (recvLen > sizeof(msg))
        {
            LOG_E(NR_RRC, "%s: Received truncated message %d\n", __func__, recvLen);
            continue;
        }
        process_lte_nsa_msg(NR_UE_rrc_inst, &msg, recvLen);
    }
    return NULL;
}

static void nsa_rrc_ue_process_ueCapabilityEnquiry(void)
{
  NR_UE_NR_Capability_t *UE_Capability_nr = NR_UE_rrc_inst[0].UECap.UE_NR_Capability = CALLOC(1, sizeof(NR_UE_NR_Capability_t));
  NR_BandNR_t *nr_bandnr = CALLOC(1, sizeof(NR_BandNR_t));
  nr_bandnr->bandNR = 78;
  asn1cSeqAdd(&UE_Capability_nr->rf_Parameters.supportedBandListNR.list, nr_bandnr);
  OAI_NR_UECapability_t *UECap = CALLOC(1, sizeof(OAI_NR_UECapability_t));
  UECap->UE_NR_Capability = UE_Capability_nr;

  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UE_NR_Capability,
                                   NULL,
                                   (void *)UE_Capability_nr,
                                   &UECap->sdu[0],
                                   MAX_UE_NR_CAPABILITY_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  UECap->sdu_size = (enc_rval.encoded + 7) / 8;
  LOG_A(NR_RRC, "[NR_RRC] NRUE Capability encoded, %d bytes (%zd bits)\n",
        UECap->sdu_size, enc_rval.encoded + 7);

  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container;
  memset(&ue_CapabilityRAT_Container, 0, sizeof(NR_UE_CapabilityRAT_Container_t));
  ue_CapabilityRAT_Container.rat_Type = NR_RAT_Type_nr;
  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container.ue_CapabilityRAT_Container,
                       (const char *)NR_UE_rrc_inst[0].UECap.sdu,
                       NR_UE_rrc_inst[0].UECap.sdu_size);

  nsa_sendmsg_to_lte_ue(ue_CapabilityRAT_Container.ue_CapabilityRAT_Container.buf,
                        ue_CapabilityRAT_Container.ue_CapabilityRAT_Container.size,
                        NRUE_CAPABILITY_INFO);
}

static void process_lte_nsa_msg(NR_UE_RRC_INST_t *rrc, nsa_msg_t *msg, int msg_len)
{
    if (msg_len < sizeof(msg->msg_type))
    {
        LOG_E(RRC, "Msg_len = %d\n", msg_len);
        return;
    }
    LOG_D(NR_RRC, "Processing an NSA message\n");
    Rrc_Msg_Type_t msg_type = msg->msg_type;
    uint8_t *const msg_buffer = msg->msg_buffer;
    msg_len -= sizeof(msg->msg_type);
    switch (msg_type)
    {
        case UE_CAPABILITY_ENQUIRY:
        {
            LOG_D(NR_RRC, "We are processing a %d message \n", msg_type);
            NR_FreqBandList_t *nr_freq_band_list = NULL;
            asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                            &asn_DEF_NR_FreqBandList,
                            (void **)&nr_freq_band_list,
                            msg_buffer,
                            msg_len);
            if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
            {
              SEQUENCE_free(&asn_DEF_NR_FreqBandList, nr_freq_band_list, ASFM_FREE_EVERYTHING);
              LOG_E(RRC, "Failed to decode UECapabilityInfo (%zu bits)\n", dec_rval.consumed);
              break;
            }
            for (int i = 0; i < nr_freq_band_list->list.count; i++)
            {
                LOG_D(NR_RRC, "Received NR band information: %ld.\n",
                     nr_freq_band_list->list.array[i]->choice.bandInformationNR->bandNR);
            }
            int dummy_msg = 0;// whatever piece of data, it will never be used by sendee
            LOG_D(NR_RRC, "We are calling nsa_sendmsg_to_lte_ue to send a UE_CAPABILITY_DUMMY\n");
            nsa_sendmsg_to_lte_ue(&dummy_msg, sizeof(dummy_msg), UE_CAPABILITY_DUMMY);
            LOG_A(NR_RRC, "Sent initial NRUE Capability response to LTE UE\n");
            break;
        }

        case NRUE_CAPABILITY_ENQUIRY:
        {
            LOG_I(NR_RRC, "We are processing a %d message \n", msg_type);
            NR_FreqBandList_t *nr_freq_band_list = NULL;
            asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                            &asn_DEF_NR_FreqBandList,
                            (void **)&nr_freq_band_list,
                            msg_buffer,
                            msg_len);
            if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
            {
              SEQUENCE_free(&asn_DEF_NR_FreqBandList, nr_freq_band_list, ASFM_FREE_EVERYTHING);
              LOG_E(NR_RRC, "Failed to decode UECapabilityInfo (%zu bits)\n", dec_rval.consumed);
              break;
            }
            LOG_I(NR_RRC, "Calling nsa_rrc_ue_process_ueCapabilityEnquiry\n");
            nsa_rrc_ue_process_ueCapabilityEnquiry();
            break;
        }

        case RRC_MEASUREMENT_PROCEDURE:
        {
            LOG_I(NR_RRC, "We are processing a %d message \n", msg_type);

            LTE_MeasObjectToAddMod_t *nr_meas_obj = NULL;
            asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                            &asn_DEF_NR_MeasObjectToAddMod,
                            (void **)&nr_meas_obj,
                            msg_buffer,
                            msg_len);
            if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
            {
              SEQUENCE_free(&asn_DEF_NR_MeasObjectToAddMod, nr_meas_obj, ASFM_FREE_EVERYTHING);
              LOG_E(RRC, "Failed to decode measurement object (%zu bits) %d\n", dec_rval.consumed, dec_rval.code);
              break;
            }
            LOG_D(NR_RRC, "NR carrierFreq_r15 (ssb): %ld and sub carrier spacing:%ld\n",
                  nr_meas_obj->measObject.choice.measObjectNR_r15.carrierFreq_r15,
                  nr_meas_obj->measObject.choice.measObjectNR_r15.rs_ConfigSSB_r15.subcarrierSpacingSSB_r15);
            start_oai_nrue_threads();
            break;
        }
        case RRC_CONFIG_COMPLETE_REQ:
        {
            struct msg {
                uint32_t RadioBearer_size;
                uint32_t SecondaryCellGroup_size;
                uint8_t trans_id;
                uint8_t padding[3];
                uint8_t buffer[];
            } hdr;
            AssertFatal(msg_len >= sizeof(hdr), "Bad received msg\n");
            memcpy(&hdr, msg_buffer, sizeof(hdr));
            LOG_I(NR_RRC, "We got an RRC_CONFIG_COMPLETE_REQ\n");
            uint32_t nr_RadioBearer_size = hdr.RadioBearer_size;
            uint32_t nr_SecondaryCellGroup_size = hdr.SecondaryCellGroup_size;
            AssertFatal(sizeof(hdr) + nr_RadioBearer_size + nr_SecondaryCellGroup_size <= msg_len,
                      "nr_RadioBearerConfig1_r15 size %u nr_SecondaryCellGroupConfig_r15 size %u sizeof(hdr) %zu, msg_len = %d\n",
                      nr_RadioBearer_size,
                      nr_SecondaryCellGroup_size,
                      sizeof(hdr), msg_len);
            NR_RRC_TransactionIdentifier_t t_id = hdr.trans_id;
            LOG_I(NR_RRC, "nr_RadioBearerConfig1_r15 size %d nr_SecondaryCellGroupConfig_r15 size %d t_id %ld\n",
                      nr_RadioBearer_size,
                      nr_SecondaryCellGroup_size,
                      t_id);

            uint8_t *nr_RadioBearer_buffer = msg_buffer + offsetof(struct msg, buffer);
            uint8_t *nr_SecondaryCellGroup_buffer = nr_RadioBearer_buffer + nr_RadioBearer_size;
            process_nsa_message(NR_UE_rrc_inst, nr_SecondaryCellGroupConfig_r15, nr_SecondaryCellGroup_buffer,
                                nr_SecondaryCellGroup_size);
            process_nsa_message(NR_UE_rrc_inst, nr_RadioBearerConfigX_r15, nr_RadioBearer_buffer, nr_RadioBearer_size);
            LOG_I(NR_RRC, "Calling do_NR_RRCReconfigurationComplete. t_id %ld \n", t_id);
            uint8_t buffer[RRC_BUF_SIZE];
            size_t size = do_NR_RRCReconfigurationComplete_for_nsa(buffer, sizeof(buffer), t_id);
            nsa_sendmsg_to_lte_ue(buffer, size, NR_RRC_CONFIG_COMPLETE_REQ);
            break;
        }

        case OAI_TUN_IFACE_NSA:
        {
          LOG_I(NR_RRC, "We got an OAI_TUN_IFACE_NSA!!\n");
          char cmd_line[RRC_BUF_SIZE];
          memcpy(cmd_line, msg_buffer, sizeof(cmd_line));
          LOG_D(NR_RRC, "Command line: %s\n", cmd_line);
          if (background_system(cmd_line) != 0)
          {
            LOG_E(NR_RRC, "ESM-PROC - failed command '%s'", cmd_line);
          }
          break;
        }

        default:
            LOG_E(NR_RRC, "No NSA Message Found\n");
    }
}

void nr_rrc_going_to_IDLE(NR_UE_RRC_INST_t *rrc,
                          NR_Release_Cause_t release_cause,
                          NR_RRCRelease_t *RRCRelease)
{
  NR_UE_Timers_Constants_t *tac = &rrc->timers_and_constants;

  // if going to RRC_IDLE was triggered by reception
  // of the RRCRelease message including a waitTime
  NR_RejectWaitTime_t *waitTime = NULL;
  if (RRCRelease) {
    struct NR_RRCRelease_IEs *rrcReleaseIEs = RRCRelease->criticalExtensions.choice.rrcRelease;
    if(rrcReleaseIEs) {
      waitTime = rrcReleaseIEs->nonCriticalExtension ?
                 rrcReleaseIEs->nonCriticalExtension->waitTime : NULL;
      if (waitTime) {
        nr_timer_stop(&tac->T302); // stop 302
        // start timer T302 with the value set to the waitTime
        int target = *waitTime * 1000; // waitTime is in seconds
        nr_timer_setup(&tac->T302, target, 10);
        nr_timer_start(&tac->T302);
        // TODO inform upper layers that access barring is applicable
        // for all access categories except categories '0' and '2'.
        LOG_E(NR_RRC,"Go to IDLE. Handling RRCRelease message including a waitTime not implemented\n");
      }
    }
  }
  if (!waitTime) {
    if (is_nr_timer_active(tac->T302)) {
      nr_timer_stop(&tac->T302);
      // TODO barring alleviation as in 5.3.14.4
      // not implemented
      LOG_E(NR_RRC,"Go to IDLE. Barring alleviation not implemented\n");
    }
  }
  if (is_nr_timer_active(tac->T390)) {
    nr_timer_stop(&tac->T390);
    // TODO barring alleviation as in 5.3.14.4
    // not implemented
    LOG_E(NR_RRC,"Go to IDLE. Barring alleviation not implemented\n");
  }
  if (!RRCRelease && rrc->nrRrcState == RRC_STATE_INACTIVE_NR) {
    // TODO discard the cell reselection priority information provided by the cellReselectionPriorities
    // cell reselection priorities not implemented yet
    nr_timer_stop(&tac->T320);
  }
  // Stop all the timers except T302, T320 and T325
  nr_timer_stop(&tac->T300);
  nr_timer_stop(&tac->T301);
  nr_timer_stop(&tac->T304);
  nr_timer_stop(&tac->T310);
  nr_timer_stop(&tac->T311);
  nr_timer_stop(&tac->T319);

  // discard the UE Inactive AS context
  // TODO there is no inactive AS context

  // release the suspendConfig
  // TODO suspendConfig not handled yet

  // discard the keys (only kgnb is stored)
  memset(rrc->kgnb, 0, sizeof(rrc->kgnb));

  // release all radio resources, including release of the RLC entity,
  // the MAC configuration and the associated PDCP entity
  // and SDAP for all established RBs
  for (int j = 0; j < NB_CNX_UE; j++) {
    rrcPerNB_t *nb = &rrc->perNB[j];
    for (int i = 0; i < MAX_DRBS_PER_UE; i++) {
      if (nb->status_DRBs[i] != RB_NOT_PRESENT) {
        nb->status_DRBs[i] = RB_NOT_PRESENT;
        nr_pdcp_release_drb(rrc->ue_id, i);
      }
    }
    for (int i = 1; i < NR_NUM_SRB; i++) {
      if (nb->Srb[i] != RB_NOT_PRESENT) {
        nb->Srb[i] = RB_NOT_PRESENT;
        nr_pdcp_release_srb(rrc->ue_id, i);
      }
    }
    for (int i = 0; i < NR_MAX_NUM_LCID; i++) {
      if (nb->active_RLC_entity[i]) {
        nb->active_RLC_entity[i] = false;
        nr_rlc_release_entity(rrc->ue_id, i);
      }
    }
  }

  for (int i = 0; i < NB_CNX_UE; i++) {
    rrcPerNB_t *nb = &rrc->perNB[i];
    NR_UE_RRC_SI_INFO *SI_info = &nb->SInfo;
    init_SI_timers(SI_info);
    asn1cFreeStruc(asn_DEF_NR_SIB1, SI_info->sib1);
    asn1cFreeStruc(asn_DEF_NR_SIB2, SI_info->sib2);
    asn1cFreeStruc(asn_DEF_NR_SIB3, SI_info->sib3);
    asn1cFreeStruc(asn_DEF_NR_SIB4, SI_info->sib4);
    asn1cFreeStruc(asn_DEF_NR_SIB5, SI_info->sib5);
    asn1cFreeStruc(asn_DEF_NR_SIB6, SI_info->sib6);
    asn1cFreeStruc(asn_DEF_NR_SIB7, SI_info->sib7);
    asn1cFreeStruc(asn_DEF_NR_SIB8, SI_info->sib8);
    asn1cFreeStruc(asn_DEF_NR_SIB9, SI_info->sib9);
    asn1cFreeStruc(asn_DEF_NR_SIB10_r16, SI_info->sib10);
    asn1cFreeStruc(asn_DEF_NR_SIB11_r16, SI_info->sib11);
    asn1cFreeStruc(asn_DEF_NR_SIB12_r16, SI_info->sib12);
    asn1cFreeStruc(asn_DEF_NR_SIB13_r16, SI_info->sib13);
    asn1cFreeStruc(asn_DEF_NR_SIB14_r16, SI_info->sib14);
  }

  // reset MAC
  NR_UE_MAC_reset_cause_t cause = (rrc->nrRrcState == RRC_STATE_DETACH_NR) ? DETACH : GO_TO_IDLE;
  nr_rrc_mac_config_req_reset(rrc->ue_id, cause);

  // enter RRC_IDLE
  LOG_I(NR_RRC, "RRC moved into IDLE state\n");
  if (rrc->nrRrcState != RRC_STATE_DETACH_NR)
    rrc->nrRrcState = RRC_STATE_IDLE_NR;

  rrc->rnti = 0;

  // Indicate the release of the RRC connection to upper layers
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NR_NAS_CONN_RELEASE_IND);
  NR_NAS_CONN_RELEASE_IND(msg_p).cause = release_cause;
  itti_send_msg_to_task(TASK_NAS_NRUE, rrc->ue_id, msg_p);
}

void handle_t300_expiry(NR_UE_RRC_INST_t *rrc)
{
  // reset MAC, release the MAC configuration
  NR_UE_MAC_reset_cause_t cause = T300_EXPIRY;
  nr_rrc_mac_config_req_reset(rrc->ue_id, cause);
  // TODO handle connEstFailureControl
  // TODO inform upper layers about the failure to establish the RRC connection
}

//This calls the sidelink preconf message after RRC, MAC instances are created.
void start_sidelink(int instance)
{

  NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[instance];

  if (get_softmodem_params()->sl_mode == 2) {

    //Process the Sidelink Preconfiguration
    rrc_ue_process_sidelink_Preconfiguration(rrc, get_softmodem_params()->sync_ref);

  }
}
