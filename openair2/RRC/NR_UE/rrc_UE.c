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

#include "oai_asn1.h"

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
#include "rrc_vars.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "COMMON/mac_rrc_primitives.h"

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

NR_UE_RRC_INST_t *NR_UE_rrc_inst;
/* NAS Attach request with IMSI */
static const char  nr_nas_attach_req_imsi[] = {
  0x07, 0x41,
  /* EPS Mobile identity = IMSI */
  0x71, 0x08, 0x29, 0x80, 0x43, 0x21, 0x43, 0x65, 0x87,
  0xF9,
  /* End of EPS Mobile Identity */
  0x02, 0xE0, 0xE0, 0x00, 0x20, 0x02, 0x03,
  0xD0, 0x11, 0x27, 0x1A, 0x80, 0x80, 0x21, 0x10, 0x01, 0x00, 0x00,
  0x10, 0x81, 0x06, 0x00, 0x00, 0x00, 0x00, 0x83, 0x06, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x0A, 0x00, 0x52, 0x12, 0xF2,
  0x01, 0x27, 0x11,
};

void
nr_rrc_ue_process_ueCapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  NR_UECapabilityEnquiry_t *UECapabilityEnquiry,
  uint8_t gNB_index
);

uint8_t do_NR_RRCReconfigurationComplete(
                        const protocol_ctxt_t *const ctxt_pP,
                        uint8_t *buffer,
                        size_t buffer_size,
                        const uint8_t Transaction_id
                      );

void
nr_rrc_ue_generate_rrcReestablishmentComplete(
  const protocol_ctxt_t *const ctxt_pP,
  NR_RRCReestablishment_t *rrcReestablishment,
  uint8_t gNB_index
);

mui_t nr_rrc_mui=0;

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (encoded)
int8_t nr_rrc_ue_decode_secondary_cellgroup_config(const module_id_t module_id, NR_CellGroupConfig_t *cell_group_config)
{
  if(NR_UE_rrc_inst[module_id].scell_group_config == NULL)
    NR_UE_rrc_inst[module_id].scell_group_config = cell_group_config;
  else
    SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)cell_group_config, 0);

  if(cell_group_config->spCellConfig != NULL)
    configure_spcell(&NR_UE_rrc_inst[module_id], cell_group_config->spCellConfig);

  return 0;
}

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (decoded)
// RRCReconfiguration
int8_t nr_rrc_ue_process_rrcReconfiguration(const module_id_t module_id, NR_RRCReconfiguration_t *rrcReconfiguration)
{
  switch(rrcReconfiguration->criticalExtensions.present){
    case NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration:
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig != NULL){
        if(NR_UE_rrc_inst[module_id].radio_bearer_config == NULL){
          NR_UE_rrc_inst[module_id].radio_bearer_config = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig;                
        }else{
          if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
            struct NR_RadioBearerConfig *RadioBearerConfig = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig;
            xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void *) RadioBearerConfig);
          }
        }
      }
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup != NULL){
        NR_CellGroupConfig_t *cellGroupConfig = NULL;
        int size = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->size;
        uint8_t *buffer = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->buf;
        asn_dec_rval_t dec_rval =
            uper_decode(NULL,
                        &asn_DEF_NR_CellGroupConfig, // might be added prefix later
                        (void **)&cellGroupConfig,
                        (uint8_t *)rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->buf,
                        size,
                        0,
                        0);
        if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
          LOG_E(NR_RRC, "NR_CellGroupConfig decode error\n");
          for (int i = 0; i < size; i++)
            LOG_E(NR_RRC, "%02x ", buffer[i]);
          LOG_E(NR_RRC, "\n");
          // free the memory
          SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)cellGroupConfig, 1);
          return -1;
        }

        if (get_softmodem_params()->sa || get_softmodem_params()->nsa)
          nr_rrc_manage_rlc_bearers(cellGroupConfig, &NR_UE_rrc_inst[module_id], 0, module_id, NR_UE_rrc_inst[module_id].rnti);

        if (get_softmodem_params()->sa || get_softmodem_params()->nsa) {
          if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
            xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);
          }

          if(NR_UE_rrc_inst[module_id].cell_group_config == NULL) {
            //  first time receive the configuration, just use the memory allocated from uper_decoder. TODO this is not good implementation, need to maintain RRC_INST own structure every time.
            NR_UE_rrc_inst[module_id].cell_group_config = cellGroupConfig;
          }else {
            //  after first time, update it and free the memory after.
            SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)NR_UE_rrc_inst[module_id].cell_group_config, 0);
            NR_UE_rrc_inst[module_id].cell_group_config = cellGroupConfig;
          }

          if(cellGroupConfig->spCellConfig != NULL)
            configure_spcell(&NR_UE_rrc_inst[module_id], cellGroupConfig->spCellConfig);

          if (get_softmodem_params()->nsa) {
            nr_rrc_mac_config_req_scg(0, 0, cellGroupConfig);
          }
        } else
          nr_rrc_ue_decode_secondary_cellgroup_config(module_id, cellGroupConfig);
      }
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->measConfig != NULL){
        if(NR_UE_rrc_inst[module_id].meas_config == NULL){
          NR_UE_rrc_inst[module_id].meas_config = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->measConfig;
        }else{
          //  if some element need to be updated
          nr_rrc_ue_process_meas_config(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->measConfig);
        }
      }
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->lateNonCriticalExtension != NULL){
        //  unuse now
      }
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->nonCriticalExtension != NULL){
        // unuse now
      }
      break;
    
    case NR_RRCReconfiguration__criticalExtensions_PR_NOTHING:
    case NR_RRCReconfiguration__criticalExtensions_PR_criticalExtensionsFuture:
    default:
      break;
  }

  return 0;
}

int8_t nr_rrc_ue_process_meas_config(NR_MeasConfig_t *meas_config){

    return 0;
}

void process_nsa_message(NR_UE_RRC_INST_t *rrc, nsa_message_t nsa_message_type, void *message, int msg_len)
{
  module_id_t module_id=0; // TODO
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
        nr_rrc_ue_process_rrcReconfiguration(module_id,RRCReconfiguration);
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
        if (get_softmodem_params()->nsa) {
          protocol_ctxt_t ctxt;
          NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
          PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_id, ENB_FLAG_YES, mac->crnti, 0, 0, 0);
          LOG_D(NR_RRC, "Calling nr_rrc_ue_process_RadioBearerConfig() at %d with: e_rab_id = %ld, drbID = %ld, cipher_algo = %ld, key = %ld \n",
                          __LINE__, RadioBearerConfig->drb_ToAddModList->list.array[0]->cnAssociation->choice.eps_BearerIdentity,
                          RadioBearerConfig->drb_ToAddModList->list.array[0]->drb_Identity,
                          RadioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm,
                          *RadioBearerConfig->securityConfig->keyToUse);
          nr_rrc_ue_process_RadioBearerConfig(&ctxt, 0, RadioBearerConfig);
        }
        else if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
          xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void *) RadioBearerConfig);
        }
        ASN_STRUCT_FREE(asn_DEF_NR_RadioBearerConfig, RadioBearerConfig);
      }
      break;
    
    default:
      AssertFatal(1==0,"Unknown message %d\n",nsa_message_type);
      break;
  }
}

NR_UE_RRC_INST_t* openair_rrc_top_init_ue_nr(char* uecap_file, char* reconfig_file, char* rbconfig_file)
{
  if(NB_NR_UE_INST > 0) {
    NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)calloc(NB_NR_UE_INST , sizeof(NR_UE_RRC_INST_t));
    for(int nr_ue = 0; nr_ue < NB_NR_UE_INST; nr_ue++) {
      NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[nr_ue];
      // fill UE-NR-Capability @ UE-CapabilityRAT-Container here.
      rrc->selected_plmn_identity = 1;

      rrc->bwpd = NULL;
      rrc->ubwpd = NULL;
      rrc->as_security_activated = false;

      for (int i = 0; i < NB_CNX_UE; i++) {
        memset((void *)&rrc->SInfo[i], 0, sizeof(rrc->SInfo[i]));
        for (int j = 0; j < NR_NUM_SRB; j++)
          memset((void *)&rrc->Srb[i][j], 0, sizeof(rrc->Srb[i][j]));
        for (int j = 0; j < MAX_DRBS_PER_UE; j++)
          rrc->active_DRBs[i][j] = false;
        // SRB0 activated by default
        rrc->Srb[i][0].status = RB_ESTABLISHED;
      }
      rrc->ra_trigger = RA_NOT_RUNNING;
    }

    NR_UE_rrc_inst->uecap_file = uecap_file;

    if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra == 1) {
      // read in files for RRCReconfiguration and RBconfig

      LOG_I(NR_RRC, "using %s for rrc init[1/2]\n", reconfig_file);
      FILE *fd = fopen(reconfig_file,"r");
      AssertFatal(fd,
                  "cannot read file %s: errno %d, %s\n",
                  reconfig_file,
                  errno,
                  strerror(errno));
      char buffer[1024];
      int msg_len=fread(buffer,1,1024,fd);
      fclose(fd);
      process_nsa_message(NR_UE_rrc_inst, nr_SecondaryCellGroupConfig_r15, buffer,msg_len);

      LOG_I(NR_RRC, "using %s for rrc init[2/2]\n", rbconfig_file);
      fd = fopen(rbconfig_file,"r");
      AssertFatal(fd,
                  "cannot read file %s: errno %d, %s\n",
                  rbconfig_file,
                  errno,
                  strerror(errno));
      msg_len=fread(buffer,1,1024,fd);
      fclose(fd);
      process_nsa_message(NR_UE_rrc_inst, nr_RadioBearerConfigX_r15, buffer,msg_len);
    } else if (get_softmodem_params()->nsa) {
      LOG_D(NR_RRC, "In NSA mode \n");
    }

    if (get_softmodem_params()->sl_mode) {
      configure_NR_SL_Preconfig(get_softmodem_params()->sync_ref);
    }
  }
  else{
    NR_UE_rrc_inst = NULL;
  }

  return NR_UE_rrc_inst;
}

int8_t nr_ue_process_secondary_cell_list(NR_CellGroupConfig_t *cell_group_config){

    return 0;
}

int8_t nr_ue_process_mac_cell_group_config(NR_MAC_CellGroupConfig_t *mac_cell_group_config){

    return 0;
}

int8_t nr_ue_process_physical_cell_group_config(NR_PhysicalCellGroupConfig_t *phy_cell_group_config){

    return 0;
}

bool check_si_validity(NR_UE_RRC_SI_INFO *SI_info, int si_type)
{
  switch (si_type) {
    case NR_SIB_TypeInfo__type_sibType2:
      if (!SI_info->sib2 || SI_info->sib2_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType3:
      if (!SI_info->sib3 || SI_info->sib3_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType4:
      if (!SI_info->sib4 || SI_info->sib4_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType5:
      if (!SI_info->sib5 || SI_info->sib5_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType6:
      if (!SI_info->sib6 || SI_info->sib6_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType7:
      if (!SI_info->sib7 || SI_info->sib7_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType8:
      if (!SI_info->sib8 || SI_info->sib8_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType9:
      if (!SI_info->sib9 || SI_info->sib9_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType10_v1610:
      if (!SI_info->sib10 || SI_info->sib10_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType11_v1610:
      if (!SI_info->sib11 || SI_info->sib11_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType12_v1610:
      if (!SI_info->sib12 || SI_info->sib12_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType13_v1610:
      if (!SI_info->sib13 || SI_info->sib13_timer == -1)
        return false;
      break;
    case NR_SIB_TypeInfo__type_sibType14_v1610:
      if (!SI_info->sib14 || SI_info->sib14_timer == -1)
        return false;
      break;
    default :
      AssertFatal(false, "Invalid SIB type %d\n", si_type);
  }
  return true;
}

int check_si_status(NR_UE_RRC_SI_INFO *SI_info)
{
  // schedule reception of SIB1 if RRC doesn't have it
  // or if the timer expired
  if (!SI_info->sib1 || SI_info->sib1_timer == -1)
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
int8_t nr_rrc_ue_decode_NR_BCCH_BCH_Message(const module_id_t module_id, const uint8_t gNB_index, uint8_t *const bufferP, const uint8_t buffer_len)
{
  NR_BCCH_BCH_Message_t *bcch_message = NULL;

  if (NR_UE_rrc_inst[module_id].mib == NULL)
    LOG_A(NR_RRC, "Configuring MAC for first MIB reception\n");

  asn_dec_rval_t dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_BCCH_BCH_Message, (void **)&bcch_message, (const void *)bufferP, buffer_len);

  int ret;
  if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, "NR_BCCH_BCH decode error\n");
    ret = -1;
  } else {
    //  link to rrc instance
    ASN_STRUCT_FREE(asn_DEF_NR_MIB, NR_UE_rrc_inst[module_id].mib);
    NR_UE_rrc_inst[module_id].mib = bcch_message->message.choice.mib;
    bcch_message->message.choice.mib = NULL;

    int get_sib = 0;
    if (get_softmodem_params()->sa && NR_UE_rrc_inst[module_id].mib->cellBarred == NR_MIB__cellBarred_notBarred) {
      NR_UE_RRC_SI_INFO *SI_info = &NR_UE_rrc_inst[module_id].SInfo[gNB_index];
      // to schedule MAC to get SI if required
      get_sib = check_si_status(SI_info);
    }
    nr_rrc_mac_config_req_mib(module_id, 0, NR_UE_rrc_inst[module_id].mib, get_sib);
    ret = 0;
  }
  ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_Message, bcch_message);
  return ret;
}

int nr_decode_SI(const module_id_t module_id, const uint8_t gNB_index, NR_SystemInformation_t *si)
{
  NR_UE_RRC_SI_INFO *SI_info = &NR_UE_rrc_inst[module_id].SInfo[gNB_index];
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

    switch(typeandinfo->present) {
      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2:
        if(!SI_info->sib2)
          SI_info->sib2 = calloc(1, sizeof(*SI_info->sib2));
        memcpy(SI_info->sib2, typeandinfo->choice.sib2, sizeof(NR_SIB2_t));
        SI_info->sib2_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB2 from gNB %"PRIu8"\n", module_id, gNB_index);
        break; // case SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib3:
        if(!SI_info->sib3)
          SI_info->sib3 = calloc(1, sizeof(*SI_info->sib3));
        memcpy(SI_info->sib3, typeandinfo->choice.sib3, sizeof(NR_SIB3_t));
        SI_info->sib3_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB3 from gNB %"PRIu8"\n", module_id, gNB_index );
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib4:
        if(!SI_info->sib4)
          SI_info->sib4 = calloc(1, sizeof(*SI_info->sib4));
        memcpy(SI_info->sib4, typeandinfo->choice.sib4, sizeof(NR_SIB4_t));
        SI_info->sib4_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB4 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib5:
        if(!SI_info->sib5)
          SI_info->sib5 = calloc(1, sizeof(*SI_info->sib5));
        memcpy(SI_info->sib5, typeandinfo->choice.sib5, sizeof(NR_SIB5_t));
        SI_info->sib5_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB5 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib6:
        if(!SI_info->sib6)
          SI_info->sib6 = calloc(1, sizeof(*SI_info->sib6));
        memcpy(SI_info->sib6, typeandinfo->choice.sib6, sizeof(NR_SIB6_t));
        SI_info->sib6_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB6 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib7:
        if(!SI_info->sib7)
          SI_info->sib7 = calloc(1, sizeof(*SI_info->sib7));
        memcpy(SI_info->sib7, typeandinfo->choice.sib7, sizeof(NR_SIB7_t));
        SI_info->sib7_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB7 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib8:
        if(!SI_info->sib8)
          SI_info->sib8 = calloc(1, sizeof(*SI_info->sib8));
        memcpy(SI_info->sib8, typeandinfo->choice.sib8, sizeof(NR_SIB8_t));
        SI_info->sib8_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB8 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib9:
        if(!SI_info->sib9)
          SI_info->sib9 = calloc(1, sizeof(*SI_info->sib9));
        memcpy(SI_info->sib9, typeandinfo->choice.sib9, sizeof(NR_SIB9_t));
        SI_info->sib9_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB9 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib10_v1610:
        if(!SI_info->sib10)
          SI_info->sib10 = calloc(1, sizeof(*SI_info->sib10));
        memcpy(SI_info->sib10, typeandinfo->choice.sib10_v1610, sizeof(NR_SIB10_r16_t));
        SI_info->sib10_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB10 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib11_v1610:
        if(!SI_info->sib11)
          SI_info->sib11 = calloc(1, sizeof(*SI_info->sib11));
        memcpy(SI_info->sib11, typeandinfo->choice.sib11_v1610, sizeof(NR_SIB11_r16_t));
        SI_info->sib11_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB11 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib12_v1610:
        if(!SI_info->sib12)
          SI_info->sib12 = calloc(1, sizeof(*SI_info->sib12));
        memcpy(SI_info->sib12, typeandinfo->choice.sib12_v1610, sizeof(NR_SIB12_r16_t));
        SI_info->sib12_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB12 from gNB %"PRIu8"\n", module_id, gNB_index);
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib13_v1610:
        if(!SI_info->sib13)
          SI_info->sib13 = calloc(1, sizeof(*SI_info->sib13));
        memcpy(SI_info->sib13, typeandinfo->choice.sib13_v1610, sizeof(NR_SIB13_r16_t));
        SI_info->sib13_timer = 0;
        LOG_I( RRC, "[UE %"PRIu8"] Found SIB13 from gNB %"PRIu8"\n", module_id, gNB_index );
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib14_v1610:
        if(!SI_info->sib14)
          SI_info->sib14 = calloc(1, sizeof(*SI_info->sib14));
        memcpy(SI_info->sib12, typeandinfo->choice.sib14_v1610, sizeof(NR_SIB14_r16_t));
        SI_info->sib14_timer = 0;
        LOG_I(RRC, "[UE %"PRIu8"] Found SIB14 from gNB %"PRIu8"\n", module_id, gNB_index);
      break;
      default:
        break;
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_OUT);
  return 0;
}

void nr_rrc_ue_generate_ra_msg(module_id_t module_id, int rnti)
{
  switch(NR_UE_rrc_inst[module_id].ra_trigger) {
    case INITIAL_ACCESS_FROM_RRC_IDLE:
      // After SIB1 is received, prepare RRCConnectionRequest
      nr_rrc_ue_generate_RRCSetupRequest(module_id, rnti);
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

void nr_rrc_ue_generate_RRCSetupRequest(module_id_t module_id, int rnti)
{
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
  int len = do_RRCSetupRequest(module_id, buf, sizeof(buf), rv);

  /* convention: RNTI for SRB0 is zero, as it changes all the time */
  nr_rlc_srb_recv_sdu(rnti, 0, buf, len);
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

/**\brief decode NR BCCH-DLSCH (SI) messages
   \param module_idP    module id
   \param gNB_index     gNB index
   \param sduP          pointer to buffer of ASN message BCCH-DLSCH
   \param sdu_len       length of buffer
   \param rsrq          RSRQ
   \param rsrp          RSRP*/
static int8_t nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message(module_id_t module_id,
                                                      const uint8_t gNB_index,
                                                      uint8_t *const Sdu,
                                                      const uint8_t Sdu_len,
                                                      const uint8_t rsrq,
                                                      const uint8_t rsrp)
{
  NR_BCCH_DL_SCH_Message_t *bcch_message = NULL;
  NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[module_id];
  NR_UE_RRC_SI_INFO *SI_info = &rrc->SInfo[gNB_index];
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
    LOG_E(NR_RRC, "[UE %"PRIu8"] Failed to decode BCCH_DLSCH_MESSAGE (%zu bits)\n",
           module_id,
           dec_rval.consumed);
    log_dump(NR_RRC, Sdu, Sdu_len, LOG_DUMP_CHAR,"   Received bytes:\n");
    // free the memory
    SEQUENCE_free(&asn_DEF_NR_BCCH_DL_SCH_Message, (void *)bcch_message, 1);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return -1;
  }

  if (bcch_message->message.present == NR_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1->present) {
      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        LOG_D(NR_RRC, "[UE %"PRIu8"] Decoding SIB1\n", module_id);
        if(SI_info->sib1)
          ASN_STRUCT_FREE(asn_DEF_NR_SIB1, SI_info->sib1);
        NR_SIB1_t *sib1 = bcch_message->message.choice.c1->choice.systemInformationBlockType1;
        SI_info->sib1 = sib1;
        if(g_log->log_component[NR_RRC].level >= OAILOG_DEBUG)
          xer_fprint(stdout, &asn_DEF_NR_SIB1, (const void *) SI_info->sib1);
        LOG_A(NR_RRC, "SIB1 decoded\n");
        SI_info->sib1_timer = 0;
        // FIXME: improve condition for the RA trigger
        if(rrc->nrRrcState == RRC_STATE_IDLE_NR) {
          rrc->ra_trigger = INITIAL_ACCESS_FROM_RRC_IDLE;
          LOG_D(PHY,"Setting state to RRC_STATE_IDLE_NR\n");
        }
        // configure default SI
        nr_rrc_configure_default_SI(SI_info, sib1);
        // configure timers and constant
        nr_rrc_set_sib1_timers_and_constants(&rrc->timers_and_constants, sib1);
        // take ServingCellConfigCommon and configure L1/L2
        rrc->servingCellConfigCommonSIB = sib1->servingCellConfigCommon;
        nr_rrc_mac_config_req_sib1(module_id, 0, sib1->si_SchedulingInfo, sib1->servingCellConfigCommon);
        break;
      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformation:
        LOG_I(NR_RRC, "[UE %"PRIu8"] Decoding SI\n", module_id);
        NR_SystemInformation_t *si = bcch_message->message.choice.c1->choice.systemInformation;
        nr_decode_SI(module_id, gNB_index, si);
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

void nr_rrc_manage_rlc_bearers(const NR_CellGroupConfig_t *cellGroupConfig,
                               NR_UE_RRC_INST_t *rrc,
                               int gNB_index,
                               module_id_t module_id,
                               int rnti)
{
  if (cellGroupConfig->rlc_BearerToReleaseList != NULL) {
    for (int i = 0; i < cellGroupConfig->rlc_BearerToReleaseList->list.count; i++) {
      NR_LogicalChannelIdentity_t *lcid = cellGroupConfig->rlc_BearerToReleaseList->list.array[i];
      AssertFatal(lcid, "LogicalChannelIdentity shouldn't be null here\n");
      nr_rlc_release_entity(rnti, *lcid);
    }
  }

  if (cellGroupConfig->rlc_BearerToAddModList != NULL) {
    for (int i = 0; i < cellGroupConfig->rlc_BearerToAddModList->list.count; i++) {
      NR_RLC_BearerConfig_t *rlc_bearer = cellGroupConfig->rlc_BearerToAddModList->list.array[i];
      NR_LogicalChannelIdentity_t lcid = rlc_bearer->logicalChannelIdentity;
      if (rrc->active_RLC_entity[gNB_index][lcid]) {
        if (rlc_bearer->reestablishRLC)
          nr_rlc_reestablish_entity(rnti, lcid);
        nr_rlc_reconfigure_entity(rnti, lcid, rlc_bearer->rlc_Config, rlc_bearer->mac_LogicalChannelConfig);
      } else {
        rrc->active_RLC_entity[gNB_index][lcid] = true;
        AssertFatal(rlc_bearer->servedRadioBearer, "servedRadioBearer mandatory in case of setup\n");
        AssertFatal(rlc_bearer->servedRadioBearer->present != NR_RLC_BearerConfig__servedRadioBearer_PR_NOTHING,
                    "Invalid RB for RLC configuration\n");
        if (rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity) {
          NR_SRB_Identity_t srb_id = rlc_bearer->servedRadioBearer->choice.srb_Identity;
          nr_rlc_add_srb(rnti, srb_id, rlc_bearer);
        } else { // DRB
          NR_DRB_Identity_t drb_id = rlc_bearer->servedRadioBearer->choice.drb_Identity;
          nr_rlc_add_drb(rnti, drb_id, rlc_bearer);
        }
      }
    }
  }
  nr_rrc_mac_config_req_ue_logicalChannelBearer(module_id,
                                                cellGroupConfig->rlc_BearerToAddModList,
                                                cellGroupConfig->rlc_BearerToReleaseList);
}

void nr_rrc_ue_process_masterCellGroup(const protocol_ctxt_t *const ctxt_pP,
                                       uint8_t gNB_index,
                                       OCTET_STRING_t *masterCellGroup,
                                       long *fullConfig)
{
  NR_CellGroupConfig_t *cellGroupConfig = NULL;
  uper_decode(NULL,
              &asn_DEF_NR_CellGroupConfig,   //might be added prefix later
              (void **)&cellGroupConfig,
              (uint8_t *)masterCellGroup->buf,
              masterCellGroup->size, 0, 0);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);
  }

  NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[ctxt_pP->module_id];

  if(rrc->cell_group_config == NULL){
    rrc->cell_group_config = calloc(1,sizeof(NR_CellGroupConfig_t));
  }

  nr_rrc_manage_rlc_bearers(cellGroupConfig, rrc, gNB_index, ctxt_pP->module_id, ctxt_pP->rntiMaybeUEid);

  if(cellGroupConfig->mac_CellGroupConfig != NULL){
    //TODO (configure the MAC entity of this cell group as specified in 5.3.5.5.5)
    LOG_I(RRC, "Received mac_CellGroupConfig from gNB\n");
    if(rrc->cell_group_config->mac_CellGroupConfig != NULL){
      LOG_E(RRC, "UE RRC instance already contains mac CellGroupConfig which will be overwritten\n");
      // Laurent: there are cases where the not NULL value is also not coming from a previous malloc
      // so it is better to let the potential memory leak than corrupting the heap
      //free(rrc->cell_group_config->mac_CellGroupConfig);
    }
    rrc->cell_group_config->mac_CellGroupConfig = malloc(sizeof(struct NR_MAC_CellGroupConfig));
    memcpy(rrc->cell_group_config->mac_CellGroupConfig,cellGroupConfig->mac_CellGroupConfig,
                     sizeof(struct NR_MAC_CellGroupConfig));
  }

  if(cellGroupConfig->sCellToReleaseList != NULL) {
    //TODO (perform SCell release as specified in 5.3.5.5.8)
  }

  if(cellGroupConfig->spCellConfig != NULL) {
    configure_spcell(rrc, cellGroupConfig->spCellConfig);
    // TS 38.331 - Section 5.3.5.5.2 Reconfiguration with sync
    if (cellGroupConfig->spCellConfig->reconfigurationWithSync != NULL) {
      if(fullConfig)
        set_default_timers_and_constants(&rrc->timers_and_constants);
      LOG_A(NR_RRC, "Received the reconfigurationWithSync in %s\n", __FUNCTION__);
      NR_ReconfigurationWithSync_t *reconfigurationWithSync = cellGroupConfig->spCellConfig->reconfigurationWithSync;
      rrc->timers_and_constants.T304_active = true;
      nr_rrc_set_T304(&rrc->timers_and_constants, reconfigurationWithSync);
      // TODO: Implementation of the remaining procedures regarding the reception of the reconfigurationWithSync, TS 38.331 - Section 5.3.5.5.2
    }

    if (rrc->cell_group_config &&
        rrc->cell_group_config->spCellConfig) {
      memcpy(rrc->cell_group_config->spCellConfig,cellGroupConfig->spCellConfig,
             sizeof(struct NR_SpCellConfig));
    } else {
      if (rrc->cell_group_config)
        rrc->cell_group_config->spCellConfig = cellGroupConfig->spCellConfig;
      else
        rrc->cell_group_config = cellGroupConfig;
    }
    LOG_D(RRC,"Sending CellGroupConfig to MAC\n");
    nr_rrc_mac_config_req_mcg(ctxt_pP->module_id, 0, cellGroupConfig);
  }

  if(fullConfig)
   // full configuration after re-establishment or during RRC resume
   nr_rrc_set_sib1_timers_and_constants(&rrc->timers_and_constants, rrc->SInfo[gNB_index].sib1);

  if( cellGroupConfig->sCellToAddModList != NULL){
    //TODO (perform SCell addition/modification as specified in 5.3.5.5.9)
  }

  if(cellGroupConfig->ext2 != NULL && cellGroupConfig->ext2->bh_RLC_ChannelToReleaseList_r16 != NULL){
    //TODO (perform the BH RLC channel addition/modification as specified in 5.3.5.5.11)
  }

  if(cellGroupConfig->ext2 != NULL && cellGroupConfig->ext2->bh_RLC_ChannelToAddModList_r16 != NULL){
    //TODO (perform the BH RLC channel addition/modification as specified in 5.3.5.5.11)
  }
}

void configure_spcell(NR_UE_RRC_INST_t *rrc, NR_SpCellConfig_t *spcell_config)
{
  nr_rrc_handle_SetupRelease_RLF_TimersAndConstants(rrc, spcell_config->rlf_TimersAndConstants);

  if(spcell_config->spCellConfigDedicated) {
    NR_ServingCellConfig_t *scd = spcell_config->spCellConfigDedicated;
    if(scd->firstActiveDownlinkBWP_Id) {
      if(*scd->firstActiveDownlinkBWP_Id == 0)
        rrc->bwpd = scd->initialDownlinkBWP;
      else {
        AssertFatal(scd->downlinkBWP_ToAddModList, "No DL BWP list configured\n");
        const struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = scd->downlinkBWP_ToAddModList;
        NR_BWP_Downlink_t *dl_bwp = NULL;
        for (int i = 0; i < bwpList->list.count; i++) {
          dl_bwp = bwpList->list.array[i];
          if(dl_bwp->bwp_Id == *scd->firstActiveDownlinkBWP_Id)
            break;
        }
        AssertFatal(dl_bwp != NULL,"Couldn't find DLBWP corresponding to BWP ID %ld\n", *scd->firstActiveDownlinkBWP_Id);
        rrc->bwpd = dl_bwp->bwp_Dedicated;
      }
      // if any of the reference signal(s) that are used for radio link monitoring are reconfigured by the received spCellConfigDedicated
      // reset RLF timers and constants
      if (rrc->bwpd->radioLinkMonitoringConfig)
        reset_rlf_timers_and_constants(&rrc->timers_and_constants);
    }
    if(scd->uplinkConfig && scd->uplinkConfig->firstActiveUplinkBWP_Id) {
      if(*scd->uplinkConfig->firstActiveUplinkBWP_Id == 0)
        rrc->ubwpd = scd->uplinkConfig->initialUplinkBWP;
      else {
        AssertFatal(scd->uplinkConfig->uplinkBWP_ToAddModList, "No UL BWP list configured\n");
        const struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = scd->uplinkConfig->uplinkBWP_ToAddModList;
        NR_BWP_Uplink_t *ul_bwp = NULL;
        for (int i = 0; i < ubwpList->list.count; i++) {
          ul_bwp = ubwpList->list.array[i];
          if(ul_bwp->bwp_Id == *scd->uplinkConfig->firstActiveUplinkBWP_Id)
            break;
        }
        AssertFatal(ul_bwp != NULL,"Couldn't find DLBWP corresponding to BWP ID %ld\n", *scd->uplinkConfig->firstActiveUplinkBWP_Id);
        rrc->ubwpd = ul_bwp->bwp_Dedicated;
      }
    }
  }
}

static void rrc_ue_generate_RRCSetupComplete(const protocol_ctxt_t *const ctxt_pP,
                                             const uint8_t gNB_index,
                                             const uint8_t Transaction_id,
                                             uint8_t sel_plmn_id)
{
  uint8_t buffer[100];
  uint8_t size;
  const char *nas_msg;
  int   nas_msg_length;

  if (get_softmodem_params()->sa) {
    as_nas_info_t initialNasMsg;
    nr_ue_nas_t *nas = get_ue_nas_info(ctxt_pP->module_id);
    generateRegistrationRequest(&initialNasMsg, nas);
    nas_msg = (char*)initialNasMsg.data;
    nas_msg_length = initialNasMsg.length;
  } else {
    nas_msg         = nr_nas_attach_req_imsi;
    nas_msg_length  = sizeof(nr_nas_attach_req_imsi);
  }

  size = do_RRCSetupComplete(ctxt_pP->module_id, buffer, sizeof(buffer),
                             Transaction_id, sel_plmn_id, nas_msg_length, nas_msg);
  LOG_I(NR_RRC,
        "[UE %d][RAPROC] Frame %d : Logical Channel UL-DCCH (SRB1), Generating RRCSetupComplete (bytes%d, gNB %d)\n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        size,
        gNB_index);
  int srb_id = 1; // RRC setup complete on SRB1
  LOG_D(NR_RRC,
        "[FRAME %05d][RRC_UE][MOD %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCSetupComplete to gNB %d MUI %d) --->][PDCP][MOD %02d][RB "
        "%02d]\n",
        ctxt_pP->frame,
        ctxt_pP->module_id,
        size,
        gNB_index,
        nr_rrc_mui,
        ctxt_pP->module_id + NB_eNB_INST,
        srb_id);

  nr_pdcp_data_req_srb(ctxt_pP->rntiMaybeUEid, srb_id, nr_rrc_mui++, size, buffer, deliver_pdu_srb_rlc, NULL);
}

static int8_t nr_rrc_ue_decode_ccch(const protocol_ctxt_t *const ctxt_pP, const uint8_t *buf, int len, const uint8_t gNB_index)
{
  NR_UE_RRC_INST_t *rrc = &NR_UE_rrc_inst[ctxt_pP->module_id];
  NR_DL_CCCH_Message_t *dl_ccch_msg=NULL;
  asn_dec_rval_t dec_rval;
  int rval=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_IN);
  LOG_D(RRC,
        "[NR UE%d] Decoding DL-CCCH message (%d bytes), State %d\n",
        ctxt_pP->module_id,
        len,
        rrc->nrRrcState);

  dec_rval = uper_decode(NULL, &asn_DEF_NR_DL_CCCH_Message, (void **)&dl_ccch_msg, buf, len, 0, 0);

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_DL_CCCH_Message, (void *)dl_ccch_msg);

  if (dec_rval.code != RC_OK && dec_rval.consumed == 0) {
    LOG_E(RRC,
          "[UE %d] Frame %d : Failed to decode DL-CCCH-Message (%zu bytes)\n",
          ctxt_pP->module_id,
          ctxt_pP->frame,
          dec_rval.consumed);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
    return -1;
   }

   if (dl_ccch_msg->message.present == NR_DL_CCCH_MessageType_PR_c1) {
     switch (dl_ccch_msg->message.choice.c1->present) {
       case NR_DL_CCCH_MessageType__c1_PR_NOTHING:
	 LOG_I(NR_RRC, "[UE%d] Frame %d : Received PR_NOTHING on DL-CCCH-Message\n",
	       ctxt_pP->module_id,
	       ctxt_pP->frame);
	 rval = 0;
	 break;

       case NR_DL_CCCH_MessageType__c1_PR_rrcReject:
	 LOG_I(NR_RRC,
	       "[UE%d] Frame %d : Logical Channel DL-CCCH (SRB0), Received RRCReject \n",
	       ctxt_pP->module_id,
	       ctxt_pP->frame);
	 rval = 0;
	 break;

       case NR_DL_CCCH_MessageType__c1_PR_rrcSetup:
         LOG_I(NR_RRC, "[UE%d][RAPROC] Frame %d : Logical Channel DL-CCCH (SRB0), Received NR_RRCSetup RNTI %lx\n", ctxt_pP->module_id, ctxt_pP->frame, ctxt_pP->rntiMaybeUEid);

         // Get configuration
         // Release T300 timer
         rrc->timers_and_constants.T300_active = 0;

         nr_rrc_ue_process_masterCellGroup(ctxt_pP, gNB_index, &dl_ccch_msg->message.choice.c1->choice.rrcSetup->criticalExtensions.choice.rrcSetup->masterCellGroup, NULL);
         nr_rrc_ue_process_RadioBearerConfig(ctxt_pP, gNB_index, &dl_ccch_msg->message.choice.c1->choice.rrcSetup->criticalExtensions.choice.rrcSetup->radioBearerConfig);
         rrc->nrRrcState = RRC_STATE_CONNECTED_NR;
         rrc->rnti = ctxt_pP->rntiMaybeUEid;
         rrc_ue_generate_RRCSetupComplete(ctxt_pP, gNB_index, dl_ccch_msg->message.choice.c1->choice.rrcSetup->rrc_TransactionIdentifier, rrc->selected_plmn_identity);
         rval = 0;
         break;

       default:
	 LOG_E(NR_RRC, "[UE%d] Frame %d : Unknown message\n",
	       ctxt_pP->module_id,
	       ctxt_pP->frame);
	 rval = -1;
	 break;
     }
   }

   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
   return rval;
 }

 // from NR SRB3
 int8_t nr_rrc_ue_decode_NR_DL_DCCH_Message(
   const module_id_t module_id,
   const uint8_t     gNB_index,
   const uint8_t    *bufferP,
   const uint32_t    buffer_len ){
   //  uper_decode by nr R15 rrc_connection_reconfiguration

   int32_t i;
   NR_DL_DCCH_Message_t *nr_dl_dcch_msg = NULL;
   MessageDef *msg_p;

   asn_dec_rval_t dec_rval = uper_decode(  NULL,
					   &asn_DEF_NR_DL_DCCH_Message,
					   (void**)&nr_dl_dcch_msg,
					   (uint8_t *)bufferP,
					   buffer_len, 0, 0);

   if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {
     for (i=0; i<buffer_len; i++)
       LOG_D(NR_RRC, "%02x ",bufferP[i]);
     LOG_D(NR_RRC, "\n");
     // free the memory
     SEQUENCE_free( &asn_DEF_NR_DL_DCCH_Message, (void *)nr_dl_dcch_msg, 1 );
     return -1;
   }

   if(nr_dl_dcch_msg != NULL){
     switch(nr_dl_dcch_msg->message.present){
       case NR_DL_DCCH_MessageType_PR_c1:
	 switch(nr_dl_dcch_msg->message.choice.c1->present){
	   case NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration:
	     nr_rrc_ue_process_rrcReconfiguration(module_id,nr_dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration);
	     break;

	   case NR_DL_DCCH_MessageType__c1_PR_NOTHING:
	   case NR_DL_DCCH_MessageType__c1_PR_rrcResume:
	   case NR_DL_DCCH_MessageType__c1_PR_rrcRelease:
	     msg_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_CONN_RELEASE_IND);
	     if((nr_dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.present == NR_RRCRelease__criticalExtensions_PR_rrcRelease) &&
		(nr_dl_dcch_msg->message.choice.c1->present == NR_DL_DCCH_MessageType__c1_PR_rrcRelease)){
		 nr_dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq->deprioritisationTimer =
		 NR_RRCRelease_IEs__deprioritisationReq__deprioritisationTimer_min5;
		 nr_dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq->deprioritisationType =
		 NR_RRCRelease_IEs__deprioritisationReq__deprioritisationType_frequency;
	       }
	     itti_send_msg_to_task(TASK_RRC_NRUE,module_id,msg_p);
	     break;

	   case NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment:
	   case NR_DL_DCCH_MessageType__c1_PR_securityModeCommand:
	   case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
	   case NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
	   case NR_DL_DCCH_MessageType__c1_PR_counterCheck:
	   case NR_DL_DCCH_MessageType__c1_PR_mobilityFromNRCommand:
	   case NR_DL_DCCH_MessageType__c1_PR_dlDedicatedMessageSegment_r16:
	   case NR_DL_DCCH_MessageType__c1_PR_ueInformationRequest_r16:
	   case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransferMRDC_r16:
	   case NR_DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r16:
	   case NR_DL_DCCH_MessageType__c1_PR_spare3:
	   case NR_DL_DCCH_MessageType__c1_PR_spare2:
	   case NR_DL_DCCH_MessageType__c1_PR_spare1:
	   default:
	     //  not supported or unused
	     break;
	 }
	 break;
       case NR_DL_DCCH_MessageType_PR_NOTHING:
       case NR_DL_DCCH_MessageType_PR_messageClassExtension:
       default:
	 //  not supported or unused
	 break;
     }

     //  release memory allocation
     SEQUENCE_free( &asn_DEF_NR_DL_DCCH_Message, (void *)nr_dl_dcch_msg, 1 );
   }else{
     //  log..
   }

   return 0;
 }


void nr_rrc_ue_process_securityModeCommand(const protocol_ctxt_t *const ctxt_pP,
                                           NR_SecurityModeCommand_t *const securityModeCommand,
                                           const uint8_t gNB_index)
{
  asn_enc_rval_t enc_rval;
  NR_UL_DCCH_Message_t ul_dcch_msg;
  uint8_t buffer[200];
  int securityMode;
  LOG_I(NR_RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), Processing securityModeCommand (eNB %d)\n",
        ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, gNB_index);

  NR_SecurityConfigSMC_t *securityConfigSMC = &securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC;
  NR_UE_RRC_INST_t *ue_rrc = &NR_UE_rrc_inst[ctxt_pP->module_id];

  switch (securityConfigSMC->securityAlgorithmConfig.cipheringAlgorithm) {
    case NR_CipheringAlgorithm_nea0:
      LOG_I(NR_RRC,"[UE %d] Security algorithm is set to nea0\n",
            ctxt_pP->module_id);
      securityMode= NR_CipheringAlgorithm_nea0;
      break;

    case NR_CipheringAlgorithm_nea1:
      LOG_I(NR_RRC,"[UE %d] Security algorithm is set to nea1\n",ctxt_pP->module_id);
      securityMode= NR_CipheringAlgorithm_nea1;
      break;

    case NR_CipheringAlgorithm_nea2:
      LOG_I(NR_RRC,"[UE %d] Security algorithm is set to nea2\n",
            ctxt_pP->module_id);
      securityMode = NR_CipheringAlgorithm_nea2;
      break;

    default:
      LOG_I(NR_RRC,"[UE %d] Security algorithm is set to none\n",ctxt_pP->module_id);
      securityMode = NR_CipheringAlgorithm_spare1;
      break;
  }
  ue_rrc->cipheringAlgorithm = securityConfigSMC->securityAlgorithmConfig.cipheringAlgorithm;

  if (securityConfigSMC->securityAlgorithmConfig.integrityProtAlgorithm != NULL) {
    switch (*securityConfigSMC->securityAlgorithmConfig.integrityProtAlgorithm) {
      case NR_IntegrityProtAlgorithm_nia1:
        LOG_I(NR_RRC,"[UE %d] Integrity protection algorithm is set to nia1\n",ctxt_pP->module_id);
        securityMode |= 1 << 5;
        break;

      case NR_IntegrityProtAlgorithm_nia2:
        LOG_I(NR_RRC,"[UE %d] Integrity protection algorithm is set to nia2\n",ctxt_pP->module_id);
        securityMode |= 1 << 6;
        break;

      default:
        LOG_I(NR_RRC,"[UE %d] Integrity protection algorithm is set to none\n",ctxt_pP->module_id);
        securityMode |= 0x70 ;
        break;
    }

    ue_rrc->integrityProtAlgorithm = *securityConfigSMC->securityAlgorithmConfig.integrityProtAlgorithm;
  }

  LOG_D(NR_RRC,"[UE %d] security mode is %x \n",ctxt_pP->module_id, securityMode);
  memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
  //memset((void *)&SecurityModeCommand,0,sizeof(SecurityModeCommand_t));
  ul_dcch_msg.message.present = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1 = calloc(1, sizeof(*ul_dcch_msg.message.choice.c1));

  if (securityMode >= NO_SECURITY_MODE) {
    LOG_I(NR_RRC, "rrc_ue_process_securityModeCommand, security mode complete case \n");
    ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
  } else {
    LOG_I(NR_RRC, "rrc_ue_process_securityModeCommand, security mode failure case \n");
    ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeFailure;
    ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
  }

  uint8_t kRRCenc[16] = {0};
  uint8_t kUPenc[16] = {0};
  uint8_t kRRCint[16] = {0};
  nr_derive_key(UP_ENC_ALG,
                ue_rrc->cipheringAlgorithm,
                ue_rrc->kgnb,
                kUPenc);
  nr_derive_key(RRC_ENC_ALG,
                ue_rrc->cipheringAlgorithm,
                ue_rrc->kgnb,
                kRRCenc);
  nr_derive_key(RRC_INT_ALG,
                ue_rrc->integrityProtAlgorithm,
                ue_rrc->kgnb,
                kRRCint);

  LOG_I(NR_RRC, "driving kRRCenc, kRRCint and kUPenc from KgNB="
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x"
        "%02x%02x%02x%02x\n",
        ue_rrc->kgnb[0],  ue_rrc->kgnb[1],  ue_rrc->kgnb[2],  ue_rrc->kgnb[3],
        ue_rrc->kgnb[4],  ue_rrc->kgnb[5],  ue_rrc->kgnb[6],  ue_rrc->kgnb[7],
        ue_rrc->kgnb[8],  ue_rrc->kgnb[9],  ue_rrc->kgnb[10], ue_rrc->kgnb[11],
        ue_rrc->kgnb[12], ue_rrc->kgnb[13], ue_rrc->kgnb[14], ue_rrc->kgnb[15],
        ue_rrc->kgnb[16], ue_rrc->kgnb[17], ue_rrc->kgnb[18], ue_rrc->kgnb[19],
        ue_rrc->kgnb[20], ue_rrc->kgnb[21], ue_rrc->kgnb[22], ue_rrc->kgnb[23],
        ue_rrc->kgnb[24], ue_rrc->kgnb[25], ue_rrc->kgnb[26], ue_rrc->kgnb[27],
        ue_rrc->kgnb[28], ue_rrc->kgnb[29], ue_rrc->kgnb[30], ue_rrc->kgnb[31]);

  if (securityMode != 0xff) {
    uint8_t security_mode = ue_rrc->cipheringAlgorithm | (ue_rrc->integrityProtAlgorithm << 4);
    // configure lower layers to apply SRB integrity protection and ciphering
    for (int i = 1; i < NR_NUM_SRB; i++) {
      if (ue_rrc->Srb[gNB_index][i].status == RB_ESTABLISHED)
        nr_pdcp_config_set_security(ctxt_pP->rntiMaybeUEid, i, security_mode, kRRCenc, kRRCint, kUPenc);
    }
  } else {
    LOG_I(NR_RRC, "skipped pdcp_config_set_security() as securityMode == 0x%02x", securityMode);
  }

  if (securityModeCommand->criticalExtensions.present == NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand) {
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete = CALLOC(1, sizeof(NR_SecurityModeComplete_t));
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->rrc_TransactionIdentifier = securityModeCommand->rrc_TransactionIdentifier;
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->criticalExtensions.present = NR_SecurityModeComplete__criticalExtensions_PR_securityModeComplete;
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->criticalExtensions.choice.securityModeComplete = CALLOC(1, sizeof(NR_SecurityModeComplete_IEs_t));
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->criticalExtensions.choice.securityModeComplete->nonCriticalExtension =NULL;
    LOG_I(NR_RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), encoding securityModeComplete (gNB %d), rrc_TransactionIdentifier: %ld\n",
          ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, gNB_index, securityModeCommand->rrc_TransactionIdentifier);
    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message,
                                     NULL,
                                     (void *)&ul_dcch_msg,
                                     buffer,
                                     100);
    AssertFatal(enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);

    if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
      xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
    }
    log_dump(NR_RRC, buffer, 16, LOG_DUMP_CHAR, "securityModeComplete payload: ");
    LOG_D(NR_RRC, "securityModeComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);

    for (int i = 0; i < (enc_rval.encoded + 7) / 8; i++) {
      LOG_T(NR_RRC, "%02x.", buffer[i]);
    }
    LOG_T(NR_RRC, "\n");

    //TODO the SecurityModeCommand message needs to pass the integrity protection check
    // for the UE to declare AS security to be activated
    ue_rrc->as_security_activated = true;
    int srb_id = 1; // SecurityModeComplete in SRB1
    nr_pdcp_data_req_srb(ctxt_pP->rntiMaybeUEid,
                         srb_id,
                         nr_rrc_mui++,
                         (enc_rval.encoded + 7) / 8,
                         buffer,
                         deliver_pdu_srb_rlc,
                         NULL);
  } else
    LOG_W(NR_RRC,"securityModeCommand->criticalExtensions.present (%d) != NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand\n",
          securityModeCommand->criticalExtensions.present);
}

 //-----------------------------------------------------------------------------
 void
 nr_rrc_ue_process_measConfig(
     const protocol_ctxt_t *const       ctxt_pP,
     const uint8_t                      gNB_index,
     NR_MeasConfig_t *const             measConfig
 )
 //-----------------------------------------------------------------------------
 {
   int i;
   long ind;
   NR_MeasObjectToAddMod_t   *measObj        = NULL;
   NR_ReportConfigToAddMod_t *reportConfig   = NULL;

   if (measConfig->measObjectToRemoveList != NULL) {
     for (i = 0; i < measConfig->measObjectToRemoveList->list.count; i++) {
       ind = *measConfig->measObjectToRemoveList->list.array[i];
       free(NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1]);
     }
   }

   if (measConfig->measObjectToAddModList != NULL) {
     LOG_I(NR_RRC, "Measurement Object List is present\n");
     for (i = 0; i < measConfig->measObjectToAddModList->list.count; i++) {
       measObj = measConfig->measObjectToAddModList->list.array[i];
       ind     = measConfig->measObjectToAddModList->list.array[i]->measObjectId;

       if (NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1]) {
         LOG_D(NR_RRC, "Modifying measurement object %ld\n",ind);
         memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1],
           (char *)measObj,
           sizeof(NR_MeasObjectToAddMod_t));
       } else {
	 LOG_I(NR_RRC, "Adding measurement object %ld\n", ind);

	 if (measObj->measObject.present == NR_MeasObjectToAddMod__measObject_PR_measObjectNR) {
	     NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1]=measObj;
	 }
       }
     }

     LOG_I(NR_RRC, "call rrc_mac_config_req \n");
     // rrc_mac_config_req_ue
   }

   if (measConfig->reportConfigToRemoveList != NULL) {
     for (i = 0; i < measConfig->reportConfigToRemoveList->list.count; i++) {
       ind = *measConfig->reportConfigToRemoveList->list.array[i];
       free(NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1]);
     }
   }

   if (measConfig->reportConfigToAddModList != NULL) {
     LOG_I(NR_RRC,"Report Configuration List is present\n");
     for (i = 0; i < measConfig->reportConfigToAddModList->list.count; i++) {
       ind          = measConfig->reportConfigToAddModList->list.array[i]->reportConfigId;
       reportConfig = measConfig->reportConfigToAddModList->list.array[i];

       if (NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1]) {
         LOG_I(NR_RRC, "Modifying Report Configuration %ld\n", ind-1);
         memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1],
                 (char *)measConfig->reportConfigToAddModList->list.array[i],
                 sizeof(NR_ReportConfigToAddMod_t));
       } else {
         LOG_D(NR_RRC,"Adding Report Configuration %ld %p \n", ind-1, measConfig->reportConfigToAddModList->list.array[i]);
         if (reportConfig->reportConfig.present == NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR) {
             NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1] = measConfig->reportConfigToAddModList->list.array[i];
         }
       }
     }
   }

   if (measConfig->measIdToRemoveList != NULL) {
     for (i = 0; i < measConfig->measIdToRemoveList->list.count; i++) {
       ind = *measConfig->measIdToRemoveList->list.array[i];
       free(NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1]);
     }
   }

   if (measConfig->measIdToAddModList != NULL) {
     for (i = 0; i < measConfig->measIdToAddModList->list.count; i++) {
       ind = measConfig->measIdToAddModList->list.array[i]->measId;

       if (NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1]) {
         LOG_D(NR_RRC, "Modifying Measurement ID %ld\n",ind-1);
         memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1],
                 (char *)measConfig->measIdToAddModList->list.array[i],
                 sizeof(NR_MeasIdToAddMod_t));
       } else {
         LOG_D(NR_RRC, "Adding Measurement ID %ld %p\n", ind-1, measConfig->measIdToAddModList->list.array[i]);
         NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1] = measConfig->measIdToAddModList->list.array[i];
       }
     }
   }

   if (measConfig->quantityConfig != NULL) {
     if (NR_UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[gNB_index]) {
       LOG_D(NR_RRC,"Modifying Quantity Configuration \n");
       memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[gNB_index],
	       (char *)measConfig->quantityConfig,
	       sizeof(NR_QuantityConfig_t));
     } else {
       LOG_D(NR_RRC, "Adding Quantity configuration\n");
       NR_UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[gNB_index] = measConfig->quantityConfig;
     }
   }

   if (measConfig->measGapConfig != NULL) {
     if (NR_UE_rrc_inst[ctxt_pP->module_id].measGapConfig[gNB_index]) {
       memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].measGapConfig[gNB_index],
	       (char *)measConfig->measGapConfig,
	       sizeof(NR_MeasGapConfig_t));
     } else {
       NR_UE_rrc_inst[ctxt_pP->module_id].measGapConfig[gNB_index] = measConfig->measGapConfig;
     }
   }

   if (measConfig->s_MeasureConfig->present == NR_MeasConfig__s_MeasureConfig_PR_ssb_RSRP) {
     NR_UE_rrc_inst[ctxt_pP->module_id].s_measure = measConfig->s_MeasureConfig->choice.ssb_RSRP;
   } else if (measConfig->s_MeasureConfig->present == NR_MeasConfig__s_MeasureConfig_PR_csi_RSRP) {
     NR_UE_rrc_inst[ctxt_pP->module_id].s_measure = measConfig->s_MeasureConfig->choice.csi_RSRP;
   }
 }

 void nr_rrc_ue_process_RadioBearerConfig(const protocol_ctxt_t *const ctxt_pP,
                                          const uint8_t gNB_index,
                                          NR_RadioBearerConfig_t *const radioBearerConfig)
 {
   if (LOG_DEBUGFLAG(DEBUG_ASN1))
     xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void *)radioBearerConfig);

   NR_UE_RRC_INST_t *ue_rrc = &NR_UE_rrc_inst[ctxt_pP->module_id];
   if (radioBearerConfig->srb3_ToRelease)
     nr_pdcp_release_srb(ctxt_pP->rntiMaybeUEid, 3);

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
     nr_derive_key(RRC_ENC_ALG,
                   NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb,
                   kRRCenc);
     nr_derive_key(RRC_INT_ALG,
                   NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm,
                   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb,
                   kRRCint);
   }

   if (radioBearerConfig->srb_ToAddModList != NULL) {
     for (int cnt = 0; cnt < radioBearerConfig->srb_ToAddModList->list.count; cnt++) {
       struct NR_SRB_ToAddMod *srb = radioBearerConfig->srb_ToAddModList->list.array[cnt];
       NR_UE_RRC_SRB_INFO_t *Srb_info = &ue_rrc->Srb[gNB_index][srb->srb_Identity];
       if (Srb_info->status == RB_NOT_PRESENT)
         add_srb(ctxt_pP->enb_flag,
                 ctxt_pP->rntiMaybeUEid,
                 radioBearerConfig->srb_ToAddModList->list.array[cnt],
                 ue_rrc->cipheringAlgorithm,
                 ue_rrc->integrityProtAlgorithm,
                 kRRCenc,
                 kRRCint);
       else {
         AssertFatal(srb->discardOnPDCP == NULL, "discardOnPDCP not yet implemented\n");
         AssertFatal(srb->reestablishPDCP == NULL, "reestablishPDCP not yet implemented\n");
         if (srb->pdcp_Config && srb->pdcp_Config->t_Reordering)
           nr_pdcp_reconfigure_srb(ctxt_pP->rntiMaybeUEid, srb->srb_Identity, *srb->pdcp_Config->t_Reordering);
       }
       Srb_info->status = RB_ESTABLISHED;
     }
   }

   if (radioBearerConfig->drb_ToReleaseList) {
     for (int cnt = 0; cnt < radioBearerConfig->drb_ToReleaseList->list.count; cnt++) {
       NR_DRB_Identity_t *DRB_id = radioBearerConfig->drb_ToReleaseList->list.array[cnt];
       if (DRB_id)
         nr_pdcp_release_drb(ctxt_pP->rntiMaybeUEid, *DRB_id);
     }
   }

   // Establish DRBs if present
   if (radioBearerConfig->drb_ToAddModList != NULL) {
     for (int cnt = 0; cnt < radioBearerConfig->drb_ToAddModList->list.count; cnt++) {
       struct NR_DRB_ToAddMod *drb = radioBearerConfig->drb_ToAddModList->list.array[cnt];
       int DRB_id = drb->drb_Identity;
       if (ue_rrc->active_DRBs[gNB_index][DRB_id]) {
         AssertFatal(drb->reestablishPDCP == NULL, "reestablishPDCP not yet implemented\n");
         AssertFatal(drb->recoverPDCP == NULL, "recoverPDCP not yet implemented\n");
         if (drb->pdcp_Config && drb->pdcp_Config->t_Reordering)
           nr_pdcp_reconfigure_drb(ctxt_pP->rntiMaybeUEid, DRB_id, *drb->pdcp_Config->t_Reordering);
         if (drb->cnAssociation)
           AssertFatal(drb->cnAssociation->choice.sdap_Config == NULL, "SDAP reconfiguration not yet implemented\n");
       } else {
         ue_rrc->active_DRBs[gNB_index][DRB_id] = true;
         add_drb(ctxt_pP->enb_flag,
                 ctxt_pP->rntiMaybeUEid,
                 radioBearerConfig->drb_ToAddModList->list.array[cnt],
                 ue_rrc->cipheringAlgorithm,
                 ue_rrc->integrityProtAlgorithm,
                 kRRCenc,
                 kRRCint);
       }
     }
   } // drb_ToAddModList //

   NR_UE_rrc_inst[ctxt_pP->module_id].nrRrcState = RRC_STATE_CONNECTED_NR;
   LOG_I(NR_RRC, "[UE %d] State = NR_RRC_CONNECTED (gNB %d)\n", ctxt_pP->module_id, gNB_index);
 }

 //-----------------------------------------------------------------------------
 static void rrc_ue_process_rrcReconfiguration(const protocol_ctxt_t *const  ctxt_pP,
                                               NR_RRCReconfiguration_t *rrcReconfiguration,
                                               uint8_t gNB_index)
 //-----------------------------------------------------------------------------
 {
   LOG_I(NR_RRC, "[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing RRCReconfiguration (gNB %d)\n",
       ctxt_pP->module_id, ctxt_pP->frame, gNB_index);

   NR_RRCReconfiguration_IEs_t *ie = NULL;

   if (rrcReconfiguration->criticalExtensions.present
		     == NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration) {
     ie = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration;
     if (ie->measConfig != NULL) {
       LOG_I(NR_RRC, "Measurement Configuration is present\n");
 //      nr_rrc_ue_process_measConfig(ctxt_pP, gNB_index, ie->measConfig);
     }

     if((ie->nonCriticalExtension) && (ie->nonCriticalExtension->masterCellGroup != NULL)) {
       nr_rrc_ue_process_masterCellGroup(ctxt_pP,
                                         gNB_index,
                                         ie->nonCriticalExtension->masterCellGroup,
                                         ie->nonCriticalExtension->fullConfig);
     }

     if (ie->radioBearerConfig != NULL) {
       LOG_I(NR_RRC, "radio Bearer Configuration is present\n");
       nr_rrc_ue_process_RadioBearerConfig(ctxt_pP, gNB_index, ie->radioBearerConfig);
     }

     /* Check if there is dedicated NAS information to forward to NAS */
     if ((ie->nonCriticalExtension) && (ie->nonCriticalExtension->dedicatedNAS_MessageList != NULL)) {
       int list_count;
       uint32_t pdu_length;
       uint8_t *pdu_buffer;
       MessageDef *msg_p;

       for (list_count = 0; list_count < ie->nonCriticalExtension->dedicatedNAS_MessageList->list.count; list_count++) {
	 pdu_length = ie->nonCriticalExtension->dedicatedNAS_MessageList->list.array[list_count]->size;
	 pdu_buffer = ie->nonCriticalExtension->dedicatedNAS_MessageList->list.array[list_count]->buf;
	 msg_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_CONN_ESTABLI_CNF);
	 NAS_CONN_ESTABLI_CNF(msg_p).errCode = AS_SUCCESS;
	 NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.length = pdu_length;
	 NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.data = pdu_buffer;
	 itti_send_msg_to_task(TASK_NAS_NRUE, ctxt_pP->instance, msg_p);
       }

       free (ie->nonCriticalExtension->dedicatedNAS_MessageList);
     }
   }
 }

 void nr_rrc_ue_generate_RRCReconfigurationComplete(const protocol_ctxt_t *const ctxt_pP,
                                                    const uint8_t gNB_index,
                                                    const int srb_id,
                                                    const uint8_t Transaction_id)
 {
   uint8_t buffer[32], size;
   size = do_NR_RRCReconfigurationComplete(ctxt_pP, buffer, sizeof(buffer), Transaction_id);
   LOG_I(NR_RRC,
         PROTOCOL_RRC_CTXT_UE_FMT
         " Logical Channel UL-DCCH (SRB1), Generating RRCReconfigurationComplete (bytes %d, gNB_index %d)\n",
         PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
         size,
         gNB_index);
   AssertFatal(srb_id == 1 || srb_id == 3, "Invalid SRB ID %d\n", srb_id);
   LOG_D(RLC,
         "[FRAME %05d][RRC_UE][INST %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCReconfigurationComplete to gNB %d MUI %d) "
         "--->][PDCP][INST %02d][RB %02d]\n",
         ctxt_pP->frame,
         UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
         size,
         gNB_index,
         nr_rrc_mui,
         UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
         srb_id);
   nr_pdcp_data_req_srb(ctxt_pP->rntiMaybeUEid, srb_id, nr_rrc_mui++, size, buffer, deliver_pdu_srb_rlc, NULL);
 }

 int nr_rrc_ue_decode_dcch(const protocol_ctxt_t *const ctxt_pP,
                           const srb_id_t Srb_id,
                           const uint8_t *const Buffer,
                           size_t Buffer_size,
                           const uint8_t gNB_indexP)

 {
   asn_dec_rval_t dec_rval;
   NR_DL_DCCH_Message_t *dl_dcch_msg = NULL;
   MessageDef *msg_p;

   if (Srb_id != 1 && Srb_id != 2) {
     LOG_E(NR_RRC,
           "[UE %d] Frame %d: Received message on DL-DCCH (SRB%ld), should not have ...\n",
           ctxt_pP->module_id,
           ctxt_pP->frame,
           Srb_id);
   }

   LOG_D(NR_RRC, "Decoding DL-DCCH Message\n");
   dec_rval = uper_decode(NULL, &asn_DEF_NR_DL_DCCH_Message, (void **)&dl_dcch_msg, Buffer, Buffer_size, 0, 0);

   if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
     LOG_E(NR_RRC, "Failed to decode DL-DCCH (%zu bytes)\n", dec_rval.consumed);
     ASN_STRUCT_FREE(asn_DEF_NR_DL_DCCH_Message, dl_dcch_msg);
     return -1;
   }

   if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
     xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message, (void *)dl_dcch_msg);
   }

   if (dl_dcch_msg->message.present == NR_DL_DCCH_MessageType_PR_c1) {
     switch (dl_dcch_msg->message.choice.c1->present) {
       case NR_DL_DCCH_MessageType__c1_PR_NOTHING:
         LOG_I(NR_RRC, "Received PR_NOTHING on DL-DCCH-Message\n");
         break;

       case NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration: {
         rrc_ue_process_rrcReconfiguration(ctxt_pP, dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration, gNB_indexP);
         nr_rrc_ue_generate_RRCReconfigurationComplete(
             ctxt_pP,
             gNB_indexP,
             Srb_id,
             dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration->rrc_TransactionIdentifier);
         break;
       }

       case NR_DL_DCCH_MessageType__c1_PR_rrcResume:
       case NR_DL_DCCH_MessageType__c1_PR_rrcRelease:
         LOG_I(NR_RRC, "[UE %d] Received RRC Release (gNB %d)\n", ctxt_pP->module_id, gNB_indexP);

         msg_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_CONN_RELEASE_IND);

         if ((dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.present
              == NR_RRCRelease__criticalExtensions_PR_rrcRelease)
             && (dl_dcch_msg->message.choice.c1->present == NR_DL_DCCH_MessageType__c1_PR_rrcRelease)) {
           dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq
               ->deprioritisationTimer = NR_RRCRelease_IEs__deprioritisationReq__deprioritisationTimer_min5;
           dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq
               ->deprioritisationType = NR_RRCRelease_IEs__deprioritisationReq__deprioritisationType_frequency;
         }

         itti_send_msg_to_task(TASK_NAS_NRUE, ctxt_pP->instance, msg_p);
         break;
       case NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
         LOG_I(NR_RRC, "[UE %d] Received Capability Enquiry (gNB %d)\n", ctxt_pP->module_id, gNB_indexP);
         nr_rrc_ue_process_ueCapabilityEnquiry(ctxt_pP, dl_dcch_msg->message.choice.c1->choice.ueCapabilityEnquiry, gNB_indexP);
         break;
       case NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment:
         LOG_I(NR_RRC,
               "[UE%d] Frame %d : Logical Channel DL-DCCH (SRB1), Received RRCReestablishment\n",
               ctxt_pP->module_id,
               ctxt_pP->frame);
         nr_rrc_ue_generate_rrcReestablishmentComplete(ctxt_pP,
                                                       dl_dcch_msg->message.choice.c1->choice.rrcReestablishment,
                                                       gNB_indexP);
         break;
       case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer: {
         NR_DLInformationTransfer_t *dlInformationTransfer = dl_dcch_msg->message.choice.c1->choice.dlInformationTransfer;

         if (dlInformationTransfer->criticalExtensions.present
             == NR_DLInformationTransfer__criticalExtensions_PR_dlInformationTransfer) {
           /* This message hold a dedicated info NAS payload, forward it to NAS */
           NR_DedicatedNAS_Message_t *dedicatedNAS_Message =
               dlInformationTransfer->criticalExtensions.choice.dlInformationTransfer->dedicatedNAS_Message;

           MessageDef *msg_p;
           msg_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_DOWNLINK_DATA_IND);
           NAS_DOWNLINK_DATA_IND(msg_p).UEid = ctxt_pP->module_id; // TODO set the UEid to something else ?
           NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.length = dedicatedNAS_Message->size;
           NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.data = dedicatedNAS_Message->buf;
           itti_send_msg_to_task(TASK_NAS_NRUE, ctxt_pP->instance, msg_p);
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
         LOG_I(NR_RRC, "[UE %d] Received securityModeCommand (gNB %d)\n", ctxt_pP->module_id, gNB_indexP);
         nr_rrc_ue_process_securityModeCommand(ctxt_pP, dl_dcch_msg->message.choice.c1->choice.securityModeCommand, gNB_indexP);
         break;
     }
   }
   return 0;
 }

void nr_rrc_handle_ra_indication(unsigned int mod_id, bool ra_succeeded)
{
  NR_UE_Timers_Constants_t *timers = &NR_UE_rrc_inst[mod_id].timers_and_constants;
  if (ra_succeeded && timers->T304_active == true) {
    // successful Random Access procedure triggered by reconfigurationWithSync
    timers->T304_active = false;
    timers->T304_cnt = 0;
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
  protocol_ctxt_t ctxt;

  MessageDef *msg_p = NULL;
  itti_receive_msg(TASK_RRC_NRUE, &msg_p);
  instance_t instance = ITTI_MSG_DESTINATION_INSTANCE(msg_p);
  unsigned int ue_mod_id = UE_INSTANCE_TO_MODULE_ID(instance);

  switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
       LOG_W(NR_RRC, " *** Exiting RRC thread\n");
       itti_exit_task();
       break;

    case MESSAGE_TEST:
       LOG_D(NR_RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME(msg_p));
       break;

    case NR_RRC_MAC_SYNC_IND:
       LOG_D(NR_RRC, "[UE %d] Received %s: frame %d\n", ue_mod_id, ITTI_MSG_NAME(msg_p), NR_RRC_MAC_SYNC_IND(msg_p).frame);
       nr_sync_msg_t sync_msg = NR_RRC_MAC_SYNC_IND(msg_p).in_sync ? IN_SYNC : OUT_OF_SYNC;
       NR_UE_Timers_Constants_t *tac = &NR_UE_rrc_inst[ue_mod_id].timers_and_constants;
       handle_rlf_sync(tac, sync_msg);
       break;

    case NRRRC_FRAME_PROCESS:
       LOG_D(NR_RRC, "[UE %d] Received %s: frame %d\n", ue_mod_id, ITTI_MSG_NAME(msg_p), NRRRC_FRAME_PROCESS(msg_p).frame);
       // increase the timers every 10ms (every new frame)
       NR_UE_Timers_Constants_t *timers = &NR_UE_rrc_inst[ue_mod_id].timers_and_constants;
       nr_rrc_handle_timers(timers);
       NR_UE_RRC_SI_INFO *SInfo = &NR_UE_rrc_inst[ue_mod_id].SInfo[NRRRC_FRAME_PROCESS(msg_p).gnb_id];
       nr_rrc_SI_timers(SInfo);
       break;

    case NR_RRC_MAC_MSG3_IND:
       PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_NO, NR_RRC_MAC_MSG3_IND(msg_p).rnti, 0, 0);
       LOG_D(NR_RRC, "[UE %d] Received %s for RNTI %d\n", ue_mod_id, ITTI_MSG_NAME(msg_p), NR_RRC_MAC_MSG3_IND(msg_p).rnti);
       nr_rrc_ue_generate_ra_msg(ue_mod_id, NR_RRC_MAC_MSG3_IND(msg_p).rnti);
       break;

    case NR_RRC_MAC_RA_IND:
       LOG_D(NR_RRC,
             "[UE %d] Received %s: frame %d RA %s\n",
             ue_mod_id,
             ITTI_MSG_NAME(msg_p),
             NR_RRC_MAC_RA_IND(msg_p).frame,
             NR_RRC_MAC_RA_IND(msg_p).RA_succeeded ? "successful" : "failed");
       nr_rrc_handle_ra_indication(ue_mod_id, NR_RRC_MAC_RA_IND(msg_p).RA_succeeded);
       break;

    case NR_RRC_MAC_BCCH_DATA_IND:
       LOG_D(NR_RRC, "[UE %d] Received %s: gNB %d\n", ue_mod_id, ITTI_MSG_NAME(msg_p), NR_RRC_MAC_BCCH_DATA_IND(msg_p).gnb_index);
       NRRrcMacBcchDataInd *bcch = &NR_RRC_MAC_BCCH_DATA_IND(msg_p);
       PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, GNB_FLAG_NO, NOT_A_RNTI, bcch->frame, 0, bcch->gnb_index);
       if (bcch->is_bch)
         nr_rrc_ue_decode_NR_BCCH_BCH_Message(ue_mod_id, bcch->gnb_index, bcch->sdu, bcch->sdu_size);
       else
         nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message(ctxt.module_id,
                                                 bcch->gnb_index,
                                                 bcch->sdu,
                                                 bcch->sdu_size,
                                                 bcch->rsrq,
                                                 bcch->rsrp);
       break;

    case NR_RRC_MAC_CCCH_DATA_IND:
       PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_NO, NR_RRC_MAC_CCCH_DATA_IND(msg_p).rnti, 0, 0);
       nr_rrc_ue_decode_ccch(&ctxt,
                             NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu,
                             NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size,
                             /* gNB_index = */ 0);
       break;

    /* PDCP messages */
    case NR_RRC_DCCH_DATA_IND:
       PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt,
                                      NR_RRC_DCCH_DATA_IND(msg_p).module_id,
                                      GNB_FLAG_NO,
                                      NR_RRC_DCCH_DATA_IND(msg_p).rnti,
                                      NR_RRC_DCCH_DATA_IND(msg_p).frame,
                                      0,
                                      NR_RRC_DCCH_DATA_IND(msg_p).gNB_index);
       LOG_D(NR_RRC,
             "[UE %d] Received %s: frameP %d, DCCH %d, gNB %d\n",
             NR_RRC_DCCH_DATA_IND(msg_p).module_id,
             ITTI_MSG_NAME(msg_p),
             NR_RRC_DCCH_DATA_IND(msg_p).frame,
             NR_RRC_DCCH_DATA_IND(msg_p).dcch_index,
             NR_RRC_DCCH_DATA_IND(msg_p).gNB_index);
       LOG_D(NR_RRC,
             PROTOCOL_RRC_CTXT_UE_FMT "Received %s DCCH %d, gNB %d\n",
             PROTOCOL_NR_RRC_CTXT_UE_ARGS(&ctxt),
             ITTI_MSG_NAME(msg_p),
             NR_RRC_DCCH_DATA_IND(msg_p).dcch_index,
             NR_RRC_DCCH_DATA_IND(msg_p).gNB_index);
       nr_rrc_ue_decode_dcch(&ctxt,
                             NR_RRC_DCCH_DATA_IND(msg_p).dcch_index,
                             NR_RRC_DCCH_DATA_IND(msg_p).sdu_p,
                             NR_RRC_DCCH_DATA_IND(msg_p).sdu_size,
                             NR_RRC_DCCH_DATA_IND(msg_p).gNB_index);
       break;

    case NAS_KENB_REFRESH_REQ:
       memcpy((void *)NR_UE_rrc_inst[ue_mod_id].kgnb,
              (void *)NAS_KENB_REFRESH_REQ(msg_p).kenb,
              sizeof(NR_UE_rrc_inst[ue_mod_id].kgnb));
       LOG_D(RRC,
             "[UE %d] Received %s: refreshed RRC::KgNB = "
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x"
             "%02x%02x%02x%02x\n",
             ue_mod_id,
             ITTI_MSG_NAME(msg_p),
             NR_UE_rrc_inst[ue_mod_id].kgnb[0],
             NR_UE_rrc_inst[ue_mod_id].kgnb[1],
             NR_UE_rrc_inst[ue_mod_id].kgnb[2],
             NR_UE_rrc_inst[ue_mod_id].kgnb[3],
             NR_UE_rrc_inst[ue_mod_id].kgnb[4],
             NR_UE_rrc_inst[ue_mod_id].kgnb[5],
             NR_UE_rrc_inst[ue_mod_id].kgnb[6],
             NR_UE_rrc_inst[ue_mod_id].kgnb[7],
             NR_UE_rrc_inst[ue_mod_id].kgnb[8],
             NR_UE_rrc_inst[ue_mod_id].kgnb[9],
             NR_UE_rrc_inst[ue_mod_id].kgnb[10],
             NR_UE_rrc_inst[ue_mod_id].kgnb[11],
             NR_UE_rrc_inst[ue_mod_id].kgnb[12],
             NR_UE_rrc_inst[ue_mod_id].kgnb[13],
             NR_UE_rrc_inst[ue_mod_id].kgnb[14],
             NR_UE_rrc_inst[ue_mod_id].kgnb[15],
             NR_UE_rrc_inst[ue_mod_id].kgnb[16],
             NR_UE_rrc_inst[ue_mod_id].kgnb[17],
             NR_UE_rrc_inst[ue_mod_id].kgnb[18],
             NR_UE_rrc_inst[ue_mod_id].kgnb[19],
             NR_UE_rrc_inst[ue_mod_id].kgnb[20],
             NR_UE_rrc_inst[ue_mod_id].kgnb[21],
             NR_UE_rrc_inst[ue_mod_id].kgnb[22],
             NR_UE_rrc_inst[ue_mod_id].kgnb[23],
             NR_UE_rrc_inst[ue_mod_id].kgnb[24],
             NR_UE_rrc_inst[ue_mod_id].kgnb[25],
             NR_UE_rrc_inst[ue_mod_id].kgnb[26],
             NR_UE_rrc_inst[ue_mod_id].kgnb[27],
             NR_UE_rrc_inst[ue_mod_id].kgnb[28],
             NR_UE_rrc_inst[ue_mod_id].kgnb[29],
             NR_UE_rrc_inst[ue_mod_id].kgnb[30],
             NR_UE_rrc_inst[ue_mod_id].kgnb[31]);
       break;

    case NAS_UPLINK_DATA_REQ: {
       uint32_t length;
       uint8_t *buffer;
       LOG_I(NR_RRC, "[UE %d] Received %s: UEid %d\n", ue_mod_id, ITTI_MSG_NAME(msg_p), NAS_UPLINK_DATA_REQ(msg_p).UEid);
       /* Create message for PDCP (ULInformationTransfer_t) */
       length =
           do_NR_ULInformationTransfer(&buffer, NAS_UPLINK_DATA_REQ(msg_p).nasMsg.length, NAS_UPLINK_DATA_REQ(msg_p).nasMsg.data);
       /* Transfer data to PDCP */
       PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, GNB_FLAG_NO, NR_UE_rrc_inst[ue_mod_id].rnti, 0, 0, 0);
       // check if SRB2 is created, if yes request data_req on SRB2
       rb_id_t srb_id = NR_UE_rrc_inst[ue_mod_id].Srb[0][2].status == RB_ESTABLISHED ? 2 : 1;
       nr_pdcp_data_req_srb(ctxt.rntiMaybeUEid, srb_id, nr_rrc_mui++, length, buffer, deliver_pdu_srb_rlc, NULL);
       break;
    }

    default:
       LOG_E(NR_RRC, "[UE %d] Received unexpected message %s\n", ue_mod_id, ITTI_MSG_NAME(msg_p));
       break;
  }
  LOG_D(NR_RRC, "[UE %d] RRC Status %d\n", ue_mod_id, NR_UE_rrc_inst[ue_mod_id].nrRrcState);
  int result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
  AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  return NULL;
}

void nr_rrc_ue_process_sidelink_radioResourceConfig(
  module_id_t                                Mod_idP,
  uint8_t                                    gNB_index,
  NR_SetupRelease_SL_ConfigDedicatedNR_r16_t  *sl_ConfigDedicatedNR
)
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

void nr_rrc_ue_process_ueCapabilityEnquiry(const protocol_ctxt_t *const ctxt_pP,
                                           NR_UECapabilityEnquiry_t *UECapabilityEnquiry,
                                           uint8_t gNB_index)
{
  asn_enc_rval_t enc_rval;
  asn_dec_rval_t dec_rval;
  NR_UL_DCCH_Message_t ul_dcch_msg;
  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container;
  char UE_NR_Capability_xer[65536];
  size_t size;
  uint8_t buffer[500];
  LOG_I(NR_RRC,"[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing UECapabilityEnquiry (gNB %d)\n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        gNB_index);

  memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
  memset((void *)&ue_CapabilityRAT_Container,0,sizeof(NR_UE_CapabilityRAT_Container_t));
  ul_dcch_msg.message.present = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1 = CALLOC(1, sizeof(struct NR_UL_DCCH_MessageType__c1));
  ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation;
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation = CALLOC(1, sizeof(struct NR_UECapabilityInformation));
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->rrc_TransactionIdentifier = UECapabilityEnquiry->rrc_TransactionIdentifier;
  ue_CapabilityRAT_Container.rat_Type = NR_RAT_Type_nr;
  NR_UE_NR_Capability_t* UE_Capability_nr = NULL;

  char *file_path = NR_UE_rrc_inst[ctxt_pP->module_id].uecap_file;

  FILE *f = NULL;
  if (file_path)
    f = fopen(file_path, "r");
  if(f){
    size = fread(UE_NR_Capability_xer, 1, sizeof UE_NR_Capability_xer, f);
    if (size == 0 || size == sizeof UE_NR_Capability_xer) {
      LOG_E(NR_RRC,"UE Capabilities XER file %s is too large (%ld)\n", file_path,size);
      free(UE_Capability_nr);
      return;
    }
    dec_rval = xer_decode(0, &asn_DEF_NR_UE_NR_Capability, (void *)&UE_Capability_nr, UE_NR_Capability_xer, size);
    assert(dec_rval.code == RC_OK);
  }
  else {
    UE_Capability_nr = CALLOC(1,sizeof(NR_UE_NR_Capability_t));
    NR_BandNR_t *nr_bandnr;
    nr_bandnr  = CALLOC(1,sizeof(NR_BandNR_t));
    nr_bandnr->bandNR = 1;
    asn1cSeqAdd(&UE_Capability_nr->rf_Parameters.supportedBandListNR.list,
                     nr_bandnr);
  }
  OAI_NR_UECapability_t *UECap;
  UECap = CALLOC(1,sizeof(OAI_NR_UECapability_t));
  UECap->UE_NR_Capability = UE_Capability_nr;
  xer_fprint(stdout,&asn_DEF_NR_UE_NR_Capability,(void *)UE_Capability_nr);

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UE_NR_Capability,
                                   NULL,
                                   (void *)UE_Capability_nr,
                                   &UECap->sdu[0],
                                   MAX_UE_NR_CAPABILITY_SIZE);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  UECap->sdu_size = (enc_rval.encoded + 7) / 8;
  LOG_I(PHY, "[RRC]UE NR Capability encoded, %d bytes (%zd bits)\n",
        UECap->sdu_size, enc_rval.encoded + 7);

  NR_UE_rrc_inst[ctxt_pP->module_id].UECap = UECap;
  NR_UE_rrc_inst[ctxt_pP->module_id].UECapability = UECap->sdu;
  NR_UE_rrc_inst[ctxt_pP->module_id].UECapability_size = UECap->sdu_size; 
  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container.ue_CapabilityRAT_Container,
                       (const char *)NR_UE_rrc_inst[ctxt_pP->module_id].UECapability,
                       NR_UE_rrc_inst[ctxt_pP->module_id].UECapability_size);

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
  ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation =
      CALLOC(1, sizeof(struct NR_UECapabilityInformation_IEs));
  ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList =
      CALLOC(1, sizeof(struct NR_UE_CapabilityRAT_ContainerList));
  ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.count = 0;

  for (int i = 0; i < ueCapabilityEnquiry_ie->ue_CapabilityRAT_RequestList.list.count; i++) {
    if (ueCapabilityEnquiry_ie->ue_CapabilityRAT_RequestList.list.array[i]->rat_Type == NR_RAT_Type_nr) {
      asn1cSeqAdd(&ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list,
                  &ue_CapabilityRAT_Container);
      enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, 500);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                   enc_rval.failed_type->name, enc_rval.encoded);

      if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
        xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
      }
      LOG_I(NR_RRC, "UECapabilityInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
      int srb_id = 1; // UECapabilityInformation on SRB1
      nr_pdcp_data_req_srb(ctxt_pP->rntiMaybeUEid,
                           srb_id,
                           nr_rrc_mui++,
                           (enc_rval.encoded + 7) / 8,
                           buffer,
                           deliver_pdu_srb_rlc,
                           NULL);
    }
  }
}

void
nr_rrc_ue_generate_rrcReestablishmentComplete(
  const protocol_ctxt_t *const ctxt_pP,
  NR_RRCReestablishment_t *rrcReestablishment,
  uint8_t gNB_index
)
//-----------------------------------------------------------------------------
{
    uint8_t buffer[RRC_BUFFER_SIZE] = {0};
    int size = do_RRCReestablishmentComplete(buffer, RRC_BUFFER_SIZE,
                                           rrcReestablishment->rrc_TransactionIdentifier);
    LOG_I(NR_RRC,"[UE %d][RAPROC] Frame %d : Logical Channel UL-DCCH (SRB1), Generating RRCReestablishmentComplete (bytes%d, gNB %d)\n",
          ctxt_pP->module_id,ctxt_pP->frame, size, gNB_index);
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
        process_lte_nsa_msg(&msg, recvLen);
    }
    return NULL;
}

void start_oai_nrue_threads()
{
    init_queue(&nr_rach_ind_queue);
    init_queue(&nr_rx_ind_queue);
    init_queue(&nr_crc_ind_queue);
    init_queue(&nr_uci_ind_queue);
    init_queue(&nr_sfn_slot_queue);
    init_queue(&nr_chan_param_queue);
    init_queue(&nr_dl_tti_req_queue);
    init_queue(&nr_tx_req_queue);
    init_queue(&nr_ul_dci_req_queue);
    init_queue(&nr_ul_tti_req_queue);

    if (sem_init(&sfn_slot_semaphore, 0, 0) != 0)
    {
      LOG_E(MAC, "sem_init() error\n");
      abort();
    }

    init_nrUE_standalone_thread(ue_id_g);

}

static void nsa_rrc_ue_process_ueCapabilityEnquiry(void)
{
  NR_UE_NR_Capability_t *UE_Capability_nr = CALLOC(1, sizeof(NR_UE_NR_Capability_t));
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

  NR_UE_rrc_inst[0].UECap = UECap;
  NR_UE_rrc_inst[0].UECapability = UECap->sdu;
  NR_UE_rrc_inst[0].UECapability_size = UECap->sdu_size;

  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container;
  memset(&ue_CapabilityRAT_Container, 0, sizeof(NR_UE_CapabilityRAT_Container_t));
  ue_CapabilityRAT_Container.rat_Type = NR_RAT_Type_nr;
  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container.ue_CapabilityRAT_Container,
                       (const char *)NR_UE_rrc_inst[0].UECapability,
                       NR_UE_rrc_inst[0].UECapability_size);

  nsa_sendmsg_to_lte_ue(ue_CapabilityRAT_Container.ue_CapabilityRAT_Container.buf,
                        ue_CapabilityRAT_Container.ue_CapabilityRAT_Container.size,
                        NRUE_CAPABILITY_INFO);
}

void process_lte_nsa_msg(nsa_msg_t *msg, int msg_len)
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

void nr_ue_rrc_timer_trigger(int module_id, int frame, int gnb_id)
{
  MessageDef *message_p;
  message_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NRRRC_FRAME_PROCESS);
  NRRRC_FRAME_PROCESS(message_p).frame = frame;
  NRRRC_FRAME_PROCESS(message_p).gnb_id = gnb_id;
  LOG_D(NR_RRC, "RRC timer trigger: frame %d\n", frame);
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
}
