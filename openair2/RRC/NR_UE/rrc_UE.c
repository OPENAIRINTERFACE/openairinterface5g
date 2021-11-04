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

#include "asn1_conversions.h"

#include "NR_DL-DCCH-Message.h"        //asn_DEF_NR_DL_DCCH_Message
#include "NR_DL-CCCH-Message.h"        //asn_DEF_NR_DL_CCCH_Message
#include "NR_BCCH-BCH-Message.h"       //asn_DEF_NR_BCCH_BCH_Message
#include "NR_BCCH-DL-SCH-Message.h"    //asn_DEF_NR_BCCH_DL_SCH_Message
#include "NR_CellGroupConfig.h"        //asn_DEF_NR_CellGroupConfig
#include "NR_BWP-Downlink.h"           //asn_DEF_NR_BWP_Downlink
#include "NR_RRCReconfiguration.h"
#include "NR_MeasConfig.h"
#include "NR_UL-DCCH-Message.h"

#include "rrc_list.h"
#include "rrc_defs.h"
#include "rrc_proto.h"
#include "rrc_vars.h"
#include "rrc_extern.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"

#include "intertask_interface.h"

#include "nr-uesoftmodem.h"
#include "executables/softmodem-common.h"
#include "plmn_data.h"
#include "pdcp.h"
#include "UTIL/OSA/osa_defs.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#ifndef CELLULAR
  #include "RRC/NR/MESSAGES/asn1_msg.h"
#endif

#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#include "SIMULATION/TOOLS/sim.h" // for taus
#include <executables/softmodem-common.h>

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

extern void pdcp_config_set_security(
  const protocol_ctxt_t *const  ctxt_pP,
  pdcp_t          *const pdcp_pP,
  const rb_id_t         rb_idP,
  const uint16_t        lc_idP,
  const uint8_t         security_modeP,
  uint8_t         *const kRRCenc,
  uint8_t         *const kRRCint,
  uint8_t         *const  kUPenc
);

void
nr_rrc_ue_process_ueCapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  NR_UECapabilityEnquiry_t *UECapabilityEnquiry,
  uint8_t gNB_index
);

void
nr_rrc_ue_process_RadioBearerConfig(
    const protocol_ctxt_t *const       ctxt_pP,
    const uint8_t                      gNB_index,
    NR_RadioBearerConfig_t *const      radioBearerConfig
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

static Rrc_State_NR_t nr_rrc_get_state (module_id_t ue_mod_idP) {
  return NR_UE_rrc_inst[ue_mod_idP].nrRrcState;
}


static Rrc_Sub_State_NR_t nr_rrc_get_sub_state (module_id_t ue_mod_idP) {
  return NR_UE_rrc_inst[ue_mod_idP].nrRrcSubState;
}

static int nr_rrc_set_state (module_id_t ue_mod_idP, Rrc_State_NR_t state) {
  AssertFatal ((RRC_STATE_FIRST_NR <= state) && (state <= RRC_STATE_LAST_NR),
               "Invalid state %d!\n", state);

  if (NR_UE_rrc_inst[ue_mod_idP].nrRrcState != state) {
    NR_UE_rrc_inst[ue_mod_idP].nrRrcState = state;
    return (1);
  }

  return (0);
}

static int nr_rrc_set_sub_state( module_id_t ue_mod_idP, Rrc_Sub_State_NR_t subState ) {
  if (AMF_MODE_ENABLED) {
    switch (NR_UE_rrc_inst[ue_mod_idP].nrRrcState) {
      case RRC_STATE_INACTIVE_NR:
        AssertFatal ((RRC_SUB_STATE_INACTIVE_FIRST_NR <= subState) && (subState <= RRC_SUB_STATE_INACTIVE_LAST_NR),
                     "Invalid nr sub state %d for state %d!\n", subState, NR_UE_rrc_inst[ue_mod_idP].nrRrcState);
        break;

      case RRC_STATE_IDLE_NR:
        AssertFatal ((RRC_SUB_STATE_IDLE_FIRST_NR <= subState) && (subState <= RRC_SUB_STATE_IDLE_LAST_NR),
                     "Invalid nr sub state %d for state %d!\n", subState, NR_UE_rrc_inst[ue_mod_idP].nrRrcState);
        break;

      case RRC_STATE_CONNECTED_NR:
        AssertFatal ((RRC_SUB_STATE_CONNECTED_FIRST_NR <= subState) && (subState <= RRC_SUB_STATE_CONNECTED_LAST_NR),
                     "Invalid nr sub state %d for state %d!\n", subState, NR_UE_rrc_inst[ue_mod_idP].nrRrcState);
        break;
    }
  }

  if (NR_UE_rrc_inst[ue_mod_idP].nrRrcSubState != subState) {
    NR_UE_rrc_inst[ue_mod_idP].nrRrcSubState = subState;
    return (1);
  }

  return (0);
}

extern boolean_t nr_rrc_pdcp_config_asn1_req(
    const protocol_ctxt_t *const  ctxt_pP,
    NR_SRB_ToAddModList_t  *const srb2add_list,
    NR_DRB_ToAddModList_t  *const drb2add_list,
    NR_DRB_ToReleaseList_t *const drb2release_list,
    const uint8_t                   security_modeP,
    uint8_t                  *const kRRCenc,
    uint8_t                  *const kRRCint,
    uint8_t                  *const kUPenc,
    uint8_t                  *const kUPint
    ,LTE_PMCH_InfoList_r9_t  *pmch_InfoList_r9
    ,rb_id_t                 *const defaultDRB,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

extern rlc_op_status_t nr_rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const NR_SRB_ToAddModList_t   * const srb2add_listP,
    const NR_DRB_ToAddModList_t   * const drb2add_listP,
    const NR_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (encoded)
int8_t nr_rrc_ue_decode_secondary_cellgroup_config(const module_id_t module_id,
                                                   const uint8_t *buffer,
                                                   const uint32_t size){
    
  NR_CellGroupConfig_t *cell_group_config = NULL;
  uint32_t i;

  asn_dec_rval_t dec_rval = uper_decode(NULL,
                                        &asn_DEF_NR_CellGroupConfig,
                                        (void **)&cell_group_config,
                                        (uint8_t *)buffer,
                                        size, 0, 0);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    printf("NR_CellGroupConfig decode error\n");
    for (i=0; i<size; i++)
      printf("%02x ",buffer[i]);
    printf("\n");
    // free the memory
    SEQUENCE_free( &asn_DEF_NR_CellGroupConfig, (void *)cell_group_config, 1 );
    return -1;
  }

  if(NR_UE_rrc_inst[module_id].scell_group_config == NULL){
    NR_UE_rrc_inst[module_id].scell_group_config = cell_group_config;
    nr_rrc_ue_process_scg_config(module_id,cell_group_config);
  }else{
    nr_rrc_ue_process_scg_config(module_id,cell_group_config);
    SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)cell_group_config, 0);
  }

  return 0;
}

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (decoded)
// RRCReconfiguration
int8_t nr_rrc_ue_process_rrcReconfiguration(const module_id_t module_id, NR_RRCReconfiguration_t *rrcReconfiguration){

  switch(rrcReconfiguration->criticalExtensions.present){
    case NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration:
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig != NULL){
        if(NR_UE_rrc_inst[module_id].radio_bearer_config == NULL){
          NR_UE_rrc_inst[module_id].radio_bearer_config = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig;                
        }else{
          protocol_ctxt_t ctxt;
          NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
          PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_id, ENB_FLAG_YES, mac->crnti, 0, 0, 0);
          struct NR_RadioBearerConfig *RadioBearerConfig = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig;
          xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void*)RadioBearerConfig);
          LOG_D(NR_RRC, "Calling fill_default_rbconfig_ue at %d with: e_rab_id = %ld, drbID = %ld, cipher_algo = %ld, key = %ld \n",
                        __LINE__, RadioBearerConfig->drb_ToAddModList->list.array[0]->cnAssociation->choice.eps_BearerIdentity,
                        RadioBearerConfig->drb_ToAddModList->list.array[0]->drb_Identity,
                        RadioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm,
                        *RadioBearerConfig->securityConfig->keyToUse);
          nr_rrc_ue_process_RadioBearerConfig(&ctxt, 0, RadioBearerConfig);
        }
      }
      if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup != NULL){

        if(get_softmodem_params()->sa || get_softmodem_params()->nsa) {

          NR_CellGroupConfig_t *cellGroupConfig = NULL;
          uper_decode(NULL,
                      &asn_DEF_NR_CellGroupConfig,   //might be added prefix later
                      (void **)&cellGroupConfig,
                      (uint8_t *)rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->buf,
                      rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->size, 0, 0);

          if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
            xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);
          }

          if(NR_UE_rrc_inst[module_id].cell_group_config == NULL){
            //  first time receive the configuration, just use the memory allocated from uper_decoder. TODO this is not good implementation, need to maintain RRC_INST own structure every time.
            NR_UE_rrc_inst[module_id].cell_group_config = cellGroupConfig;
            nr_rrc_ue_process_scg_config(module_id,cellGroupConfig);
          }else{
            //  after first time, update it and free the memory after.
            SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)NR_UE_rrc_inst[module_id].cell_group_config, 0);
            NR_UE_rrc_inst[module_id].cell_group_config = cellGroupConfig;
            nr_rrc_ue_process_scg_config(module_id,cellGroupConfig);
          }
        }
        else
          nr_rrc_ue_decode_secondary_cellgroup_config(module_id,
                                                      rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->buf,
                                                      rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->size);
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
  //nr_rrc_mac_config_req_ue(); 

  return 0;
}



int8_t nr_rrc_ue_process_meas_config(NR_MeasConfig_t *meas_config){

    return 0;
}

int8_t nr_rrc_ue_process_scg_config(const module_id_t module_id, NR_CellGroupConfig_t *cell_group_config){
  int i;
  if(cell_group_config==NULL){
    //  initial list
    if(cell_group_config->spCellConfig != NULL){
      if(cell_group_config->spCellConfig->spCellConfigDedicated != NULL){
        if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList != NULL){
          for(i=0; i<cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count; ++i){
            RRC_LIST_MOD_ADD(NR_UE_rrc_inst[module_id].BWP_Downlink_list, cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[i], bwp_Id);
          }
        }
      }
    }
  }else{
    //  maintain list
    if(cell_group_config->spCellConfig != NULL){
        nr_rrc_mac_config_req_ue(0, 0, 0, NULL, NULL, cell_group_config, NULL);
        LOG_D(NR_RRC, "Filled scc now \n");
      if(cell_group_config->spCellConfig->spCellConfigDedicated != NULL){
        //  process element of list to be add by RRC message
        if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList != NULL){
          for(i=0; i<cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count; ++i){
            RRC_LIST_MOD_ADD(NR_UE_rrc_inst[module_id].BWP_Downlink_list, cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[i], bwp_Id);
          }
        }

        //  process element of list to be release by RRC message
        if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList != NULL){
          for(i=0; i<cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList->list.count; ++i){
            NR_BWP_Downlink_t *freeP = NULL;
            RRC_LIST_MOD_REL(NR_UE_rrc_inst[module_id].BWP_Downlink_list, bwp_Id, *cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList->list.array[i], freeP);
            if(freeP != NULL){
              SEQUENCE_free(&asn_DEF_NR_BWP_Downlink, (void *)freeP, 0);
            }
          }
        }
      }
    }
  } 

  return 0;
}


void process_nsa_message(NR_UE_RRC_INST_t *rrc, nsa_message_t nsa_message_type, void *message,int msg_len) {
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
        protocol_ctxt_t ctxt;
        NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_id, ENB_FLAG_YES, mac->crnti, 0, 0, 0);
        xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void*)RadioBearerConfig);
        LOG_D(NR_RRC, "Calling fill_default_rbconfig_ue at %d with: e_rab_id = %ld, drbID = %ld, cipher_algo = %ld, key = %ld \n",
                        __LINE__, RadioBearerConfig->drb_ToAddModList->list.array[0]->cnAssociation->choice.eps_BearerIdentity,
                        RadioBearerConfig->drb_ToAddModList->list.array[0]->drb_Identity,
                        RadioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm,
                        *RadioBearerConfig->securityConfig->keyToUse);
        nr_rrc_ue_process_RadioBearerConfig(&ctxt, 0, RadioBearerConfig);

      }
      break;
    
    default:
      AssertFatal(1==0,"Unknown message %d\n",nsa_message_type);
      break;
  }

}

NR_UE_RRC_INST_t* openair_rrc_top_init_ue_nr(char* rrc_config_path){
  int nr_ue;
  if(NB_NR_UE_INST > 0){
    NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)malloc(NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));
    memset(NR_UE_rrc_inst, 0, NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));
    for(nr_ue=0;nr_ue<NB_NR_UE_INST;nr_ue++){
      // fill UE-NR-Capability @ UE-CapabilityRAT-Container here.
      NR_UE_rrc_inst[nr_ue].selected_plmn_identity = 1;

      // TODO: Put the appropriate list of SIBs
      NR_UE_rrc_inst[nr_ue].requested_SI_List.buf = CALLOC(1,4);
      NR_UE_rrc_inst[nr_ue].requested_SI_List.buf[0] = SIB2 | SIB3 | SIB5;  // SIB2 - SIB9
      NR_UE_rrc_inst[nr_ue].requested_SI_List.buf[1] = 0;                   // SIB10 - SIB17
      NR_UE_rrc_inst[nr_ue].requested_SI_List.buf[2] = 0;                   // SIB18 - SIB25
      NR_UE_rrc_inst[nr_ue].requested_SI_List.buf[3] = 0;                   // SIB26 - SIB32
      NR_UE_rrc_inst[nr_ue].requested_SI_List.size= 4;
      NR_UE_rrc_inst[nr_ue].requested_SI_List.bits_unused= 0;

      NR_UE_rrc_inst[nr_ue].ra_trigger = RA_NOT_RUNNING;

      //  init RRC lists
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].RLC_Bearer_Config_list, NR_maxLC_ID);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SchedulingRequest_list, NR_maxNrofSR_ConfigPerCellGroup);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TAG_list, NR_maxNrofTAGs);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TDD_UL_DL_SlotConfig_list, NR_maxNrofSlots);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].BWP_Downlink_list, NR_maxNrofBWPs);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ControlResourceSet_list[0], 3);   //  for init-dl-bwp
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ControlResourceSet_list[1], 3);   //  for dl-bwp id=0
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ControlResourceSet_list[2], 3);   //  for dl-bwp id=1
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ControlResourceSet_list[3], 3);   //  for dl-bwp id=2
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ControlResourceSet_list[4], 3);   //  for dl-bwp id=3
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SearchSpace_list[0], 10);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SearchSpace_list[1], 10);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SearchSpace_list[2], 10);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SearchSpace_list[3], 10);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SearchSpace_list[4], 10);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SlotFormatCombinationsPerCell_list[0], NR_maxNrofAggregatedCellsPerCellGroup);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SlotFormatCombinationsPerCell_list[1], NR_maxNrofAggregatedCellsPerCellGroup);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SlotFormatCombinationsPerCell_list[2], NR_maxNrofAggregatedCellsPerCellGroup);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SlotFormatCombinationsPerCell_list[3], NR_maxNrofAggregatedCellsPerCellGroup);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SlotFormatCombinationsPerCell_list[4], NR_maxNrofAggregatedCellsPerCellGroup);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TCI_State_list[0], NR_maxNrofTCI_States);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TCI_State_list[1], NR_maxNrofTCI_States);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TCI_State_list[2], NR_maxNrofTCI_States);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TCI_State_list[3], NR_maxNrofTCI_States);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].TCI_State_list[4], NR_maxNrofTCI_States);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].RateMatchPattern_list[0], NR_maxNrofRateMatchPatterns);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].RateMatchPattern_list[1], NR_maxNrofRateMatchPatterns);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].RateMatchPattern_list[2], NR_maxNrofRateMatchPatterns);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].RateMatchPattern_list[3], NR_maxNrofRateMatchPatterns);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].RateMatchPattern_list[4], NR_maxNrofRateMatchPatterns);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ZP_CSI_RS_Resource_list[0], NR_maxNrofZP_CSI_RS_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ZP_CSI_RS_Resource_list[1], NR_maxNrofZP_CSI_RS_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ZP_CSI_RS_Resource_list[2], NR_maxNrofZP_CSI_RS_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ZP_CSI_RS_Resource_list[3], NR_maxNrofZP_CSI_RS_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].ZP_CSI_RS_Resource_list[4], NR_maxNrofZP_CSI_RS_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].Aperidic_ZP_CSI_RS_ResourceSet_list[0], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].Aperidic_ZP_CSI_RS_ResourceSet_list[1], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].Aperidic_ZP_CSI_RS_ResourceSet_list[2], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].Aperidic_ZP_CSI_RS_ResourceSet_list[3], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].Aperidic_ZP_CSI_RS_ResourceSet_list[4], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SP_ZP_CSI_RS_ResourceSet_list[0], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SP_ZP_CSI_RS_ResourceSet_list[1], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SP_ZP_CSI_RS_ResourceSet_list[2], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SP_ZP_CSI_RS_ResourceSet_list[3], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].SP_ZP_CSI_RS_ResourceSet_list[4], NR_maxNrofZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].NZP_CSI_RS_Resource_list, NR_maxNrofNZP_CSI_RS_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].NZP_CSI_RS_ResourceSet_list, NR_maxNrofNZP_CSI_RS_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].CSI_IM_Resource_list, NR_maxNrofCSI_IM_Resources);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].CSI_IM_ResourceSet_list, NR_maxNrofCSI_IM_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].CSI_SSB_ResourceSet_list, NR_maxNrofCSI_SSB_ResourceSets);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].CSI_ResourceConfig_list, NR_maxNrofCSI_ResourceConfigurations);
      RRC_LIST_INIT(NR_UE_rrc_inst[nr_ue].CSI_ReportConfig_list, NR_maxNrofCSI_ReportConfigurations);
    }

    if (get_softmodem_params()->phy_test==1 || get_softmodem_params()->do_ra==1) {
      // read in files for RRCReconfiguration and RBconfig
      FILE *fd;
      char filename[1024];
      if (rrc_config_path)
        sprintf(filename,"%s/reconfig.raw",rrc_config_path);
      else
        sprintf(filename,"reconfig.raw");
      fd = fopen(filename,"r");
          char buffer[1024];
      AssertFatal(fd,
                  "cannot read file %s: errno %d, %s\n",
                  filename,
                  errno,
                  strerror(errno));
      int msg_len=fread(buffer,1,1024,fd);
      process_nsa_message(NR_UE_rrc_inst, nr_SecondaryCellGroupConfig_r15, buffer,msg_len);
      fclose(fd);
      if (rrc_config_path)
        sprintf(filename,"%s/rbconfig.raw",rrc_config_path);
      else
        sprintf(filename,"rbconfig.raw");
      fd = fopen(filename,"r");
      AssertFatal(fd,
                  "cannot read file %s: errno %d, %s\n",
                  filename,
                  errno,
                  strerror(errno));
      msg_len=fread(buffer,1,1024,fd);
      process_nsa_message(NR_UE_rrc_inst, nr_SecondaryCellGroupConfig_r15, buffer,msg_len);
      fclose(fd);

    }
    else if (get_softmodem_params()->nsa)
    {
      LOG_D(NR_RRC, "In NSA mode \n");
    }
  }
  else{
    NR_UE_rrc_inst = NULL;
  }

  return NR_UE_rrc_inst;
}


int8_t nr_ue_process_rlc_bearer_list(NR_CellGroupConfig_t *cell_group_config){

    return 0;
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

int8_t nr_ue_process_spcell_config(NR_SpCellConfig_t *spcell_config){

    return 0;
}

/*brief decode BCCH-BCH (MIB) message*/
int8_t nr_rrc_ue_decode_NR_BCCH_BCH_Message(
    const module_id_t module_id,
    const uint8_t     gNB_index,
    uint8_t           *const bufferP,
    const uint8_t     buffer_len ){

    NR_BCCH_BCH_Message_t *bcch_message = NULL;

    if (NR_UE_rrc_inst[module_id].mib != NULL)
      SEQUENCE_free( &asn_DEF_NR_BCCH_BCH_Message, (void *)bcch_message, 1 );
    else LOG_I(NR_RRC,"Configuring MAC for first MIB reception\n");

    asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                   &asn_DEF_NR_BCCH_BCH_Message,
                                                   (void **)&bcch_message,
                                                   (const void *)bufferP,
                                                   buffer_len );

    if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {
      LOG_E(NR_RRC,"NR_BCCH_BCH decode error\n");
      // free the memory
      SEQUENCE_free( &asn_DEF_NR_BCCH_BCH_Message, (void *)bcch_message, 1 );
      return -1;
    }
    else {
      //  link to rrc instance
      SEQUENCE_free( &asn_DEF_NR_MIB, (void *)NR_UE_rrc_inst[module_id].mib, 1 );
      NR_UE_rrc_inst[module_id].mib = bcch_message->message.choice.mib;
      //memcpy( (void *)mib,
      //    (void *)&bcch_message->message.choice.mib,
      //    sizeof(NR_MIB_t) );

      nr_rrc_mac_config_req_ue( 0, 0, 0, NR_UE_rrc_inst[module_id].mib, NULL, NULL, NULL);
    }

    return 0;
}

const char *nr_SIBreserved( long value ) {
  if (value < 0 || value > 1)
    return "ERR";

  if (value)
    return "notReserved";

  return "reserved";
}

void nr_dump_sib2( NR_SIB2_t *sib2 ){
//cellReselectionInfoCommon
  //nrofSS_BlocksToAverage
  if( sib2->cellReselectionInfoCommon.nrofSS_BlocksToAverage)
    LOG_I( RRC, "cellReselectionInfoCommon.nrofSS_BlocksToAverage : %ld\n",
           *sib2->cellReselectionInfoCommon.nrofSS_BlocksToAverage );
  else
    LOG_I( RRC, "cellReselectionInfoCommon->nrofSS_BlocksToAverage : not defined\n" );

  //absThreshSS_BlocksConsolidation
  if( sib2->cellReselectionInfoCommon.absThreshSS_BlocksConsolidation){
    LOG_I( RRC, "absThreshSS_BlocksConsolidation.thresholdRSRP : %ld\n",
           *sib2->cellReselectionInfoCommon.absThreshSS_BlocksConsolidation->thresholdRSRP );
    LOG_I( RRC, "absThreshSS_BlocksConsolidation.thresholdRSRQ : %ld\n",
           *sib2->cellReselectionInfoCommon.absThreshSS_BlocksConsolidation->thresholdRSRQ );
    LOG_I( RRC, "absThreshSS_BlocksConsolidation.thresholdSINR : %ld\n",
           *sib2->cellReselectionInfoCommon.absThreshSS_BlocksConsolidation->thresholdSINR );
  } else
    LOG_I( RRC, "cellReselectionInfoCommon->absThreshSS_BlocksConsolidation : not defined\n" );

  //q_Hyst
  LOG_I( RRC, "cellReselectionInfoCommon.q_Hyst : %ld\n",
           sib2->cellReselectionInfoCommon.q_Hyst );

  //speedStateReselectionPars
  if( sib2->cellReselectionInfoCommon.speedStateReselectionPars){
    LOG_I( RRC, "speedStateReselectionPars->mobilityStateParameters.t_Evaluation : %ld\n",
           sib2->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.t_Evaluation);
    LOG_I( RRC, "speedStateReselectionPars->mobilityStateParameters.t_HystNormal  : %ld\n",
           sib2->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.t_HystNormal);
    LOG_I( RRC, "speedStateReselectionPars->mobilityStateParameters.n_CellChangeMedium : %ld\n",
           sib2->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.n_CellChangeMedium);
    LOG_I( RRC, "speedStateReselectionPars->mobilityStateParameters.n_CellChangeHigh : %ld\n",
           sib2->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.n_CellChangeHigh);
    LOG_I( RRC, "speedStateReselectionPars->q_HystSF.sf_Medium : %ld\n",
           sib2->cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_Medium);
    LOG_I( RRC, "speedStateReselectionPars->q_HystSF.sf_High : %ld\n",
           sib2->cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_High);
  } else
    LOG_I( RRC, "cellReselectionInfoCommon->speedStateReselectionPars : not defined\n" );

//cellReselectionServingFreqInfo
  if( sib2->cellReselectionServingFreqInfo.s_NonIntraSearchP)
    LOG_I( RRC, "cellReselectionServingFreqInfo.s_NonIntraSearchP : %ld\n",
           *sib2->cellReselectionServingFreqInfo.s_NonIntraSearchP );
  else
    LOG_I( RRC, "cellReselectionServingFreqInfo->s_NonIntraSearchP : not defined\n" );
 
  if( sib2->cellReselectionServingFreqInfo.s_NonIntraSearchQ)
    LOG_I( RRC, "cellReselectionServingFreqInfo.s_NonIntraSearchQ : %ld\n",
           *sib2->cellReselectionServingFreqInfo.s_NonIntraSearchQ );
  else
    LOG_I( RRC, "cellReselectionServingFreqInfo->s_NonIntraSearchQ : not defined\n" );
  
  LOG_I( RRC, "cellReselectionServingFreqInfo.threshServingLowP : %ld\n",
         sib2->cellReselectionServingFreqInfo.threshServingLowP );
  
  if( sib2->cellReselectionServingFreqInfo.threshServingLowQ)
    LOG_I( RRC, "cellReselectionServingFreqInfo.threshServingLowQ : %ld\n",
           *sib2->cellReselectionServingFreqInfo.threshServingLowQ );
  else
    LOG_I( RRC, "cellReselectionServingFreqInfo->threshServingLowQ : not defined\n" );
  
  LOG_I( RRC, "cellReselectionServingFreqInfo.cellReselectionPriority : %ld\n",
         sib2->cellReselectionServingFreqInfo.cellReselectionPriority );
  if( sib2->cellReselectionServingFreqInfo.cellReselectionSubPriority)
    LOG_I( RRC, "cellReselectionServingFreqInfo.cellReselectionSubPriority : %ld\n",
           *sib2->cellReselectionServingFreqInfo.cellReselectionSubPriority );
  else
    LOG_I( RRC, "cellReselectionServingFreqInfo->cellReselectionSubPriority : not defined\n" );

//intraFreqCellReselectionInfo
  LOG_I( RRC, "intraFreqCellReselectionInfo.q_RxLevMin : %ld\n",
         sib2->intraFreqCellReselectionInfo.q_RxLevMin );
  
  if( sib2->intraFreqCellReselectionInfo.q_RxLevMinSUL)
    LOG_I( RRC, "intraFreqCellReselectionInfo.q_RxLevMinSUL : %ld\n",
           *sib2->intraFreqCellReselectionInfo.q_RxLevMinSUL );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->q_RxLevMinSUL : not defined\n" );
  
  if( sib2->intraFreqCellReselectionInfo.q_QualMin)
    LOG_I( RRC, "intraFreqCellReselectionInfo.q_QualMin : %ld\n",
           *sib2->intraFreqCellReselectionInfo.q_QualMin );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->q_QualMin : not defined\n" );
  
  LOG_I( RRC, "intraFreqCellReselectionInfo.s_IntraSearchP : %ld\n",
         sib2->intraFreqCellReselectionInfo.s_IntraSearchP );
  
  if( sib2->intraFreqCellReselectionInfo.s_IntraSearchQ)
    LOG_I( RRC, "intraFreqCellReselectionInfo.s_IntraSearchQ : %ld\n",
           *sib2->intraFreqCellReselectionInfo.s_IntraSearchQ );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->s_IntraSearchQ : not defined\n" );
  
  LOG_I( RRC, "intraFreqCellReselectionInfo.t_ReselectionNR : %ld\n",
         sib2->intraFreqCellReselectionInfo.t_ReselectionNR );

  if( sib2->intraFreqCellReselectionInfo.frequencyBandList)
    LOG_I( RRC, "intraFreqCellReselectionInfo.frequencyBandList : %p\n",
           sib2->intraFreqCellReselectionInfo.frequencyBandList );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->frequencyBandList : not defined\n" );
 
  if( sib2->intraFreqCellReselectionInfo.frequencyBandListSUL)
    LOG_I( RRC, "intraFreqCellReselectionInfo.frequencyBandListSUL : %p\n",
           sib2->intraFreqCellReselectionInfo.frequencyBandListSUL );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->frequencyBandListSUL : not defined\n" );
  
  if( sib2->intraFreqCellReselectionInfo.p_Max)
    LOG_I( RRC, "intraFreqCellReselectionInfo.p_Max : %ld\n",
           *sib2->intraFreqCellReselectionInfo.p_Max );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->p_Max : not defined\n" );
 
  if( sib2->intraFreqCellReselectionInfo.smtc)
    LOG_I( RRC, "intraFreqCellReselectionInfo.smtc : %p\n",
           sib2->intraFreqCellReselectionInfo.smtc );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->smtc : not defined\n" );
 
  if( sib2->intraFreqCellReselectionInfo.ss_RSSI_Measurement)
    LOG_I( RRC, "intraFreqCellReselectionInfo.ss_RSSI_Measurement : %p\n",
           sib2->intraFreqCellReselectionInfo.ss_RSSI_Measurement );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->ss_RSSI_Measurement : not defined\n" );

  if( sib2->intraFreqCellReselectionInfo.ssb_ToMeasure)
    LOG_I( RRC, "intraFreqCellReselectionInfo.ssb_ToMeasure : %p\n",
           sib2->intraFreqCellReselectionInfo.ssb_ToMeasure );
  else
    LOG_I( RRC, "intraFreqCellReselectionInfo->ssb_ToMeasure : not defined\n" );
  
  LOG_I( RRC, "intraFreqCellReselectionInfo.deriveSSB_IndexFromCell : %d\n",
         sib2->intraFreqCellReselectionInfo.deriveSSB_IndexFromCell );

}

void nr_dump_sib3( NR_SIB3_t *sib3 ) {
//intraFreqNeighCellList
  if( sib3->intraFreqNeighCellList){
    LOG_I( RRC, "intraFreqNeighCellList : %p\n",
           sib3->intraFreqNeighCellList );    
    const int n = sib3->intraFreqNeighCellList->list.count;
    for (int i = 0; i < n; ++i){
      LOG_I( RRC, "intraFreqNeighCellList->physCellId : %ld\n",
             sib3->intraFreqNeighCellList->list.array[i]->physCellId );
      LOG_I( RRC, "intraFreqNeighCellList->q_OffsetCell : %ld\n",
             sib3->intraFreqNeighCellList->list.array[i]->q_OffsetCell );

      if( sib3->intraFreqNeighCellList->list.array[i]->q_RxLevMinOffsetCell)
        LOG_I( RRC, "intraFreqNeighCellList->q_RxLevMinOffsetCell : %ld\n",
               *sib3->intraFreqNeighCellList->list.array[i]->q_RxLevMinOffsetCell );
      else
        LOG_I( RRC, "intraFreqNeighCellList->q_RxLevMinOffsetCell : not defined\n" );
      
      if( sib3->intraFreqNeighCellList->list.array[i]->q_RxLevMinOffsetCellSUL)
        LOG_I( RRC, "intraFreqNeighCellList->q_RxLevMinOffsetCellSUL : %ld\n",
               *sib3->intraFreqNeighCellList->list.array[i]->q_RxLevMinOffsetCellSUL );
      else
        LOG_I( RRC, "intraFreqNeighCellList->q_RxLevMinOffsetCellSUL : not defined\n" );
      
      if( sib3->intraFreqNeighCellList->list.array[i]->q_QualMinOffsetCell)
        LOG_I( RRC, "intraFreqNeighCellList->q_QualMinOffsetCell : %ld\n",
               *sib3->intraFreqNeighCellList->list.array[i]->q_QualMinOffsetCell );
      else
        LOG_I( RRC, "intraFreqNeighCellList->q_QualMinOffsetCell : not defined\n" );
    }
  } else{
    LOG_I( RRC, "intraFreqCellReselectionInfo : not defined\n" );
  }

//intraFreqBlackCellList
  if( sib3->intraFreqBlackCellList){
    LOG_I( RRC, "intraFreqBlackCellList : %p\n",
           sib3->intraFreqBlackCellList );    
    const int n = sib3->intraFreqBlackCellList->list.count;
    for (int i = 0; i < n; ++i){
      LOG_I( RRC, "intraFreqBlackCellList->start : %ld\n",
             sib3->intraFreqBlackCellList->list.array[i]->start );

      if( sib3->intraFreqBlackCellList->list.array[i]->range)
        LOG_I( RRC, "intraFreqBlackCellList->range : %ld\n",
             *sib3->intraFreqBlackCellList->list.array[i]->range );
      else
        LOG_I( RRC, "intraFreqBlackCellList->range : not defined\n" );
    }
   } else{
    LOG_I( RRC, "intraFreqBlackCellList : not defined\n" );
   }

//lateNonCriticalExtension
  if( sib3->lateNonCriticalExtension)
    LOG_I( RRC, "lateNonCriticalExtension : %p\n",
           sib3->lateNonCriticalExtension );
  else
    LOG_I( RRC, "lateNonCriticalExtension : not defined\n" );
}

int nr_decode_SI( const protocol_ctxt_t *const ctxt_pP, const uint8_t gNB_index ) {
  NR_SystemInformation_t **si = &NR_UE_rrc_inst[ctxt_pP->module_id].si[gNB_index];
  int new_sib = 0;
  NR_SIB1_t *sib1 = NR_UE_rrc_inst[ctxt_pP->module_id].sib1[gNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_IN );

  // Dump contents
  if ((*si)->criticalExtensions.present == NR_SystemInformation__criticalExtensions_PR_systemInformation ||
      (*si)->criticalExtensions.present == NR_SystemInformation__criticalExtensions_PR_criticalExtensionsFuture_r16) {
    LOG_D( RRC, "[UE] (*si)->criticalExtensions.choice.NR_SystemInformation_t->sib_TypeAndInfo.list.count %d\n",
           (*si)->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.count );
  } else {
    LOG_D( RRC, "[UE] Unknown criticalExtension version (not Rel16)\n" );
    return -1;
  }

  for (int i=0; i<(*si)->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.count; i++) {
    SystemInformation_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = (*si)->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.array[i];

    switch(typeandinfo->present) {
      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&2) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=2;
          //new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib2[gNB_index], &typeandinfo->choice.sib2, sizeof(NR_SIB2_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB2 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
          nr_dump_sib2( NR_UE_rrc_inst[ctxt_pP->module_id].sib2[gNB_index] );
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB2 params  gNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, gNB_index, ctxt_pP->module_id );
          //TODO rrc_mac_config_req_ue

          // After SI is received, prepare RRCConnectionRequest
          if (NR_UE_rrc_inst[ctxt_pP->module_id].MBMS_flag < 3) // see -Q option
            if (AMF_MODE_ENABLED) {
              nr_rrc_ue_generate_RRCSetupRequest( ctxt_pP->module_id, gNB_index );
            }

          if (NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].State == NR_RRC_IDLE) {
            LOG_I( RRC, "[UE %d] Received SIB1/SIB2/SIB3 Switching to RRC_SI_RECEIVED\n", ctxt_pP->module_id );
            NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].State = NR_RRC_SI_RECEIVED;
#if ENABLE_RAL
 /* TODO          {
              MessageDef                            *message_ral_p = NULL;
              rrc_ral_system_information_ind_t       ral_si_ind;
              message_ral_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_RAL_SYSTEM_INFORMATION_IND);
              memset(&ral_si_ind, 0, sizeof(rrc_ral_system_information_ind_t));
              ral_si_ind.plmn_id.MCCdigit2 = '0';
              ral_si_ind.plmn_id.MCCdigit1 = '2';
              ral_si_ind.plmn_id.MNCdigit3 = '0';
              ral_si_ind.plmn_id.MCCdigit3 = '8';
              ral_si_ind.plmn_id.MNCdigit2 = '9';
              ral_si_ind.plmn_id.MNCdigit1 = '9';
              ral_si_ind.cell_id        = 1;
              ral_si_ind.dbm            = 0;
              //ral_si_ind.dbm            = fifo_dump_emos_UE.PHY_measurements->rx_rssi_dBm[gNB_index];
              // TO DO
              ral_si_ind.sinr           = 0;
              //ral_si_ind.sinr           = fifo_dump_emos_UE.PHY_measurements->subband_cqi_dB[gNB_index][phy_vars_ue->lte_frame_parms.nb_antennas_rx][0];
              // TO DO
              ral_si_ind.link_data_rate = 0;
              memcpy (&message_ral_p->ittiMsg, (void *) &ral_si_ind, sizeof(rrc_ral_system_information_ind_t));
#warning "ue_mod_idP ? for instance ?"
              itti_send_msg_to_task (TASK_RAL_UE, UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id), message_ral_p);
            }*/
#endif
          }
        }

        break; // case SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib3:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&4) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=4;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib3[gNB_index], &typeandinfo->choice.sib3, sizeof(LTE_SystemInformationBlockType3_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB3 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
          nr_dump_sib3( NR_UE_rrc_inst[ctxt_pP->module_id].sib3[gNB_index] );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib4:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&8) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=8;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib4[gNB_index], typeandinfo->choice.sib4, sizeof(NR_SIB4_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB4 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib5:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&16) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=16;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib5[gNB_index], typeandinfo->choice.sib5, sizeof(NR_SIB5_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB5 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
          //dump_sib5(NR_UE_rrc_inst[ctxt_pP->module_id].sib5[gNB_index]);
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib6:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&32) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=32;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib6[gNB_index], typeandinfo->choice.sib6, sizeof(NR_SIB6_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB6 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib7:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&64) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=64;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib7[gNB_index], typeandinfo->choice.sib7, sizeof(NR_SIB7_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB7 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib8:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&128) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=128;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib8[gNB_index], typeandinfo->choice.sib8, sizeof(NR_SIB8_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB8 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib9:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&256) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=256;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib9[gNB_index], typeandinfo->choice.sib9, sizeof(NR_SIB9_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB9 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib10_v1610:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&512) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=512;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib10[gNB_index], typeandinfo->choice.sib10_v1610, sizeof(NR_SIB10_r16_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB10 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib11_v1610:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&1024) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=1024;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib11[gNB_index], typeandinfo->choice.sib11_v1610, sizeof(NR_SIB11_r16_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB11 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib12_v1610:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&2048) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=2048;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib12[gNB_index], typeandinfo->choice.sib12_v1610, sizeof(NR_SIB12_r16_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB12 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }

        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib13_v1610:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&4096) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=4096;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib13[gNB_index], typeandinfo->choice.sib13_v1610, sizeof(NR_SIB13_r16_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB13 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
          //dump_sib13( NR_UE_rrc_inst[ctxt_pP->module_id].sib13[gNB_index] );
          // adding here function to store necessary parameters for using in decode_MCCH_Message + maybe transfer to PHY layer
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB13 params gNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, gNB_index, ctxt_pP->module_id);
          // TODO rrc_mac_config_req_ue
        }
        break;

      case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib14_v1610:
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus&8192) == 0) {
          NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus|=8192;
          new_sib=1;
          memcpy( NR_UE_rrc_inst[ctxt_pP->module_id].sib12[gNB_index], typeandinfo->choice.sib14_v1610, sizeof(NR_SIB14_r16_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB14 from gNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, gNB_index );
        }
      
      break;

      default:
        break;
    }
    if (new_sib == 1) {
      NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIcnt++;
      if (NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIcnt == sib1->si_SchedulingInfo->schedulingInfoList.list.count)
        nr_rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_SIB_COMPLETE );
  
      LOG_I(NR_RRC,"SIStatus %x, SIcnt %d/%d\n",
            NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus,
            NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIcnt,
            sib1->si_SchedulingInfo->schedulingInfoList.list.count);
    }
  }

  //if (new_sib == 1) {
  //  NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIcnt++;

  //  if (NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIcnt == sib1->schedulingInfoList.list.count)
  //    rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_SIB_COMPLETE );

  //  LOG_I(NR_RRC, "SIStatus %x, SIcnt %d/%d\n",
  //        NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus,
  //        NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIcnt,
  //        sib1->schedulingInfoList.list.count);
  //}

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_OUT);
  return 0;
}

static int8_t check_requested_SI_List(module_id_t module_id, BIT_STRING_t requested_SI_List, NR_SIB1_t sib1) {

  if(sib1.si_SchedulingInfo) {

    bool SIB_to_request[32] = {};

    LOG_D(RRC, "SIBs broadcasting: ");
    for(int i = 0; i < sib1.si_SchedulingInfo->schedulingInfoList.list.array[0]->sib_MappingInfo.list.count; i++) {
      printf("SIB%li  ", sib1.si_SchedulingInfo->schedulingInfoList.list.array[0]->sib_MappingInfo.list.array[i]->type + 2);
    }
    printf("\n");

    LOG_D(RRC, "SIBs needed by UE: ");
    for(int j = 0; j < 8*requested_SI_List.size; j++) {
      if( ((requested_SI_List.buf[j/8]>>(j%8))&1) == 1) {

        printf("SIB%i  ", j + 2);

        SIB_to_request[j] = true;
        for(int i = 0; i < sib1.si_SchedulingInfo->schedulingInfoList.list.array[0]->sib_MappingInfo.list.count; i++) {
          if(sib1.si_SchedulingInfo->schedulingInfoList.list.array[0]->sib_MappingInfo.list.array[i]->type == j) {
            SIB_to_request[j] = false;
            break;
          }
        }

      }
    }
    printf("\n");

    LOG_D(RRC, "SIBs to request by UE: ");
    bool do_ra = false;
    for(int j = 0; j < 8*requested_SI_List.size; j++) {
      if(SIB_to_request[j]) {
        printf("SIB%i  ", j + 2);
        do_ra = true;
      }
    }
    printf("\n");

    if(do_ra) {

      NR_UE_rrc_inst[module_id].ra_trigger = REQUEST_FOR_OTHER_SI;
      get_softmodem_params()->do_ra = 1;

      if(sib1.si_SchedulingInfo->si_RequestConfig) {
        LOG_D(RRC, "Trigger contention-free RA procedure (ra_trigger = %i)\n", NR_UE_rrc_inst[module_id].ra_trigger);
      } else {
        LOG_D(RRC, "Trigger contention-based RA procedure (ra_trigger = %i)\n", NR_UE_rrc_inst[module_id].ra_trigger);
      }

    }

  }

  return 0;
}

int8_t nr_rrc_ue_generate_ra_msg(module_id_t module_id, uint8_t gNB_index) {

  switch(NR_UE_rrc_inst[module_id].ra_trigger){
    case INITIAL_ACCESS_FROM_RRC_IDLE:
      // After SIB1 is received, prepare RRCConnectionRequest
      nr_rrc_ue_generate_RRCSetupRequest(module_id,gNB_index);
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

  return 0;
}

int8_t nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message(module_id_t module_id,
                                               const uint8_t gNB_index,
                                               uint8_t *const Sdu,
                                               const uint8_t Sdu_len,
                                               const uint8_t rsrq,
                                               const uint8_t rsrp) {

  NR_BCCH_DL_SCH_Message_t *bcch_message = NULL;
  NR_SIB1_t *sib1 = NR_UE_rrc_inst[module_id].sib1[gNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_IN );

  if (((NR_UE_rrc_inst[module_id].Info[gNB_index].SIStatus&1) == 1) && sib1->si_SchedulingInfo &&// SIB1 received
      (NR_UE_rrc_inst[module_id].Info[gNB_index].SIcnt == sib1->si_SchedulingInfo->schedulingInfoList.list.count)) {
    // to prevent memory bloating
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return 0;
  }

  nr_rrc_set_sub_state( module_id, RRC_SUB_STATE_IDLE_RECEIVING_SIB_NR );

  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                            &asn_DEF_NR_BCCH_DL_SCH_Message,
                            (void **)&bcch_message,
                            (const void *)Sdu,
                            Sdu_len );

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_BCCH_DL_SCH_Message,(void *)bcch_message );
  }

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E( NR_RRC, "[UE %"PRIu8"] Failed to decode BCCH_DLSCH_MESSAGE (%zu bits)\n",
           module_id,
           dec_rval.consumed );
    log_dump(NR_RRC, Sdu, Sdu_len, LOG_DUMP_CHAR,"   Received bytes:\n" );
    // free the memory
    SEQUENCE_free( &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return -1;
  }

  if (bcch_message->message.present == NR_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1->present) {
      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        if ((NR_UE_rrc_inst[module_id].Info[gNB_index].SIStatus&1) == 0) {
          NR_SIB1_t *sib1 = NR_UE_rrc_inst[module_id].sib1[gNB_index];
          if(sib1 != NULL){
            SEQUENCE_free(&asn_DEF_NR_SIB1, (void *)sib1, 1 );
          }
	        NR_UE_rrc_inst[module_id].Info[gNB_index].SIStatus|=1;
          sib1 = bcch_message->message.choice.c1->choice.systemInformationBlockType1;
          if (*(int64_t*)sib1 != 1) {
            NR_UE_rrc_inst[module_id].sib1[gNB_index] = sib1;
            if( g_log->log_component[NR_RRC].level >= OAILOG_DEBUG ) {
              xer_fprint(stdout, &asn_DEF_NR_SIB1, (const void *) NR_UE_rrc_inst[module_id].sib1[gNB_index]);
            }
            LOG_I(NR_RRC, "SIB1 decoded\n");

            ///	    dump_SIB1();
            // FIXME: improve condition for the RA trigger
            // Check for on-demand not broadcasted SI
            check_requested_SI_List(module_id, NR_UE_rrc_inst[module_id].requested_SI_List, *sib1);
            if( nr_rrc_get_state(module_id) <= RRC_STATE_IDLE_NR ) {
              NR_UE_rrc_inst[module_id].ra_trigger = INITIAL_ACCESS_FROM_RRC_IDLE;
              LOG_D(PHY,"Setting state to NR_RRC_SI_RECEIVED\n");
              nr_rrc_set_state (module_id, NR_RRC_SI_RECEIVED);
            }
            // take ServingCellConfigCommon and configure L1/L2
            NR_UE_rrc_inst[module_id].servingCellConfigCommonSIB = sib1->servingCellConfigCommon;
            nr_rrc_mac_config_req_ue(module_id,0,0,NULL,sib1->servingCellConfigCommon,NULL,NULL);
            nr_rrc_ue_generate_ra_msg(module_id,gNB_index);
          } else {
            LOG_E(NR_RRC, "SIB1 not decoded\n");
          }
        }
        break;

      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformation:
        if ((NR_UE_rrc_inst[module_id].Info[gNB_index].SIStatus&1) == 1) {
          LOG_W(NR_RRC, "Decoding SI not implemented yet\n");
          // TODO: Decode SI
          /*
          // SIB1 with schedulingInfoList is available
          NR_SystemInformation_t *si = NR_UE_rrc_inst[module_id].si[gNB_index];

          memcpy( si,
                  bcch_message->message.choice.c1->choice.systemInformation,
                  sizeof(NR_SystemInformation_t) );
          LOG_I(NR_RRC, "[UE %"PRIu8"] Decoding SI\n", module_id);
          nr_decode_SI( ctxt_pP, gNB_index );

          if (nfapi_mode == 3)
            UE_mac_inst[ctxt_pP->module_id].SI_Decoded = 1;
           */
        }
        break;

      case NR_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
      default:
        break;
    }
  }

  if (nr_rrc_get_sub_state(module_id) == RRC_SUB_STATE_IDLE_SIB_COMPLETE_NR) {
    //if ( (NR_UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.data != NULL) || (!AMF_MODE_ENABLED)) {
      nr_rrc_ue_generate_RRCSetupRequest(module_id, 0);
      nr_rrc_set_sub_state( module_id, RRC_SUB_STATE_IDLE_CONNECTING );
    //}
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
  return 0;
}

//-----------------------------------------------------------------------------
void
nr_rrc_ue_process_masterCellGroup(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t gNB_index,
  OCTET_STRING_t *masterCellGroup
)
//-----------------------------------------------------------------------------
{
  NR_CellGroupConfig_t *cellGroupConfig=NULL;
  uper_decode(NULL,
              &asn_DEF_NR_CellGroupConfig,   //might be added prefix later
              (void **)&cellGroupConfig,
              (uint8_t *)masterCellGroup->buf,
              masterCellGroup->size, 0, 0);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);
  }

  if( cellGroupConfig->spCellConfig != NULL &&  cellGroupConfig->spCellConfig->reconfigurationWithSync != NULL){
    //TODO (perform Reconfiguration with sync according to 5.3.5.5.2)
    //TODO (resume all suspended radio bearers and resume SCG transmission for all radio bearers, if suspended)
    // NSA procedures
  }

  if(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config == NULL){
    NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config = calloc(1,sizeof(NR_CellGroupConfig_t));
  }

  if( cellGroupConfig->rlc_BearerToReleaseList != NULL){
    //TODO (perform RLC bearer release as specified in 5.3.5.5.3)
  }

  if( cellGroupConfig->rlc_BearerToAddModList != NULL){
    //TODO (perform the RLC bearer addition/modification as specified in 5.3.5.5.4)
    if(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList != NULL){
      free(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList);
    }
    NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList = calloc(1, sizeof(struct NR_CellGroupConfig__rlc_BearerToAddModList));
    memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList,cellGroupConfig->rlc_BearerToAddModList,
                 sizeof(struct NR_CellGroupConfig__rlc_BearerToAddModList));
  }

  if( cellGroupConfig->mac_CellGroupConfig != NULL){
    //TODO (configure the MAC entity of this cell group as specified in 5.3.5.5.5)
    LOG_I(RRC, "Received mac_CellGroupConfig from gNB\n");
    if(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->mac_CellGroupConfig != NULL){
      LOG_E(RRC, "UE RRC instance already contains mac CellGroupConfig which will be overwritten\n");
      free(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->mac_CellGroupConfig);
    }
    NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->mac_CellGroupConfig = malloc(sizeof(struct NR_MAC_CellGroupConfig));
    memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->mac_CellGroupConfig,cellGroupConfig->mac_CellGroupConfig,
                     sizeof(struct NR_MAC_CellGroupConfig));
  }

  if( cellGroupConfig->sCellToReleaseList != NULL){
    //TODO (perform SCell release as specified in 5.3.5.5.8)
  }

  if( cellGroupConfig->spCellConfig != NULL){
    if (NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config &&
	      NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->spCellConfig) {
      memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->spCellConfig,cellGroupConfig->spCellConfig,
             sizeof(struct NR_SpCellConfig));
    } else {
      if (NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config)
	      NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->spCellConfig = cellGroupConfig->spCellConfig;
      else
	      NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config = cellGroupConfig;
    }
    LOG_D(RRC,"Sending CellGroupConfig to MAC\n");
    nr_rrc_mac_config_req_ue(ctxt_pP->module_id,0,0,NULL,NULL,cellGroupConfig,NULL);
    //TODO (configure the SpCell as specified in 5.3.5.5.7)
  }

  if( cellGroupConfig->sCellToAddModList != NULL){
    //TODO (perform SCell addition/modification as specified in 5.3.5.5.9)
  }

  if( cellGroupConfig->ext2->bh_RLC_ChannelToReleaseList_r16 != NULL){
    //TODO (perform the BH RLC channel addition/modification as specified in 5.3.5.5.11)
  }

  if( cellGroupConfig->ext2->bh_RLC_ChannelToAddModList_r16 != NULL){
    //TODO (perform the BH RLC channel addition/modification as specified in 5.3.5.5.11)
  }
}

/*--------------------------------------------------*/
static void rrc_ue_generate_RRCSetupComplete(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t gNB_index,
  const uint8_t Transaction_id,
  uint8_t sel_plmn_id){
  
  uint8_t buffer[100];
  uint8_t size;
  const char *nas_msg;
  int   nas_msg_length;
  NR_UE_MAC_INST_t *mac = get_mac_inst(0);

  if (mac->cg &&
      mac->cg->spCellConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated &&
      mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig)
    AssertFatal(1==0,"2 > csi_MeasConfig is not null\n");

 if (AMF_MODE_ENABLED) {
#if defined(ITTI_SIM)
    as_nas_info_t initialNasMsg;
    generateRegistrationRequest(&initialNasMsg, ctxt_pP->module_id);
    nas_msg = (char*)initialNasMsg.data;
    nas_msg_length = initialNasMsg.length;
#else
    if (get_softmodem_params()->sa) {
      as_nas_info_t initialNasMsg;
      generateRegistrationRequest(&initialNasMsg, ctxt_pP->module_id);
      nas_msg = (char*)initialNasMsg.data;
      nas_msg_length = initialNasMsg.length;
    } else {
      nas_msg         = (char *) NR_UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.data;
      nas_msg_length  = NR_UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.length;
    }
#endif
  } else {
    nas_msg         = nr_nas_attach_req_imsi;
    nas_msg_length  = sizeof(nr_nas_attach_req_imsi);
  }
  size = do_RRCSetupComplete(ctxt_pP->module_id, buffer, sizeof(buffer),
                             Transaction_id, sel_plmn_id, nas_msg_length, nas_msg);
  LOG_I(NR_RRC,"[UE %d][RAPROC] Frame %d : Logical Channel UL-DCCH (SRB1), Generating RRCSetupComplete (bytes%d, gNB %d)\n",
   ctxt_pP->module_id,ctxt_pP->frame, size, gNB_index);
  LOG_D(NR_RRC,
       "[FRAME %05d][RRC_UE][MOD %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCSetupComplete to gNB %d MUI %d) --->][PDCP][MOD %02d][RB %02d]\n",
       ctxt_pP->frame, ctxt_pP->module_id+NB_RN_INST, size, gNB_index, nr_rrc_mui, ctxt_pP->module_id+NB_eNB_INST, DCCH);

  //for (int i=0;i<size;i++) printf("%02x ",buffer[i]);
  //printf("\n");

   // ctxt_pP_local.rnti = ctxt_pP->rnti;
  rrc_data_req_nr_ue(ctxt_pP,
                  DCCH,
                  nr_rrc_mui++,
                  SDU_CONFIRM_NO,
                  size,
                  buffer,
                  PDCP_TRANSMISSION_MODE_CONTROL);

#ifdef ITTI_SIM
  MessageDef *message_p;
  uint8_t *message_buffer;
  message_buffer = itti_malloc (TASK_RRC_NRUE, TASK_RRC_GNB_SIM, size);
  memcpy (message_buffer, buffer, size);
  message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_DCCH_DATA_IND);
  UE_RRC_DCCH_DATA_IND (message_p).rbid = 1;
  UE_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
  UE_RRC_DCCH_DATA_IND (message_p).size  = size;
  itti_send_msg_to_task (TASK_RRC_GNB_SIM, ctxt_pP->instance, message_p);
#endif
}

int8_t nr_rrc_ue_decode_ccch( const protocol_ctxt_t *const ctxt_pP, const NR_SRB_INFO *const Srb_info, const uint8_t gNB_index ){

  NR_DL_CCCH_Message_t *dl_ccch_msg=NULL;
  asn_dec_rval_t dec_rval;
  int rval=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_IN);
  LOG_D(RRC,"[NR UE%d] Decoding DL-CCCH message (%d bytes), State %d\n",ctxt_pP->module_id,Srb_info->Rx_buffer.payload_size,
      NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].State);
   dec_rval = uper_decode(NULL,
			  &asn_DEF_NR_DL_CCCH_Message,
			  (void **)&dl_ccch_msg,
			  (uint8_t *)Srb_info->Rx_buffer.Payload,
			  Srb_info->Rx_buffer.payload_size,0,0);

//	 if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
     xer_fprint(stdout,&asn_DEF_NR_DL_CCCH_Message,(void *)dl_ccch_msg);
//	 }

   if ((dec_rval.code != RC_OK) && (dec_rval.consumed==0)) {
     LOG_E(RRC,"[UE %d] Frame %d : Failed to decode DL-CCCH-Message (%zu bytes)\n",ctxt_pP->module_id,ctxt_pP->frame,dec_rval.consumed);
     VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
     return -1;
   }

   if (dl_ccch_msg->message.present == NR_DL_CCCH_MessageType_PR_c1) {
     if (NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].SIStatus > 0) {
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
	 LOG_I(NR_RRC,
	       "[UE%d][RAPROC] Frame %d : Logical Channel DL-CCCH (SRB0), Received NR_RRCSetup RNTI %x\n",
	       ctxt_pP->module_id,
	       ctxt_pP->frame,
	       ctxt_pP->rnti);

	 // Get configuration
	 // Release T300 timer
	 NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].T300_active = 0;

	 nr_rrc_ue_process_masterCellGroup(
					   ctxt_pP,
					   gNB_index,
					   &dl_ccch_msg->message.choice.c1->choice.rrcSetup->criticalExtensions.choice.rrcSetup->masterCellGroup);
	 nr_rrc_ue_process_RadioBearerConfig(ctxt_pP,
					     gNB_index,
					     &dl_ccch_msg->message.choice.c1->choice.rrcSetup->criticalExtensions.choice.rrcSetup->radioBearerConfig);
	 nr_rrc_set_state (ctxt_pP->module_id, RRC_STATE_CONNECTED);
	 nr_rrc_set_sub_state (ctxt_pP->module_id, RRC_SUB_STATE_CONNECTED);
	 NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].rnti = ctxt_pP->rnti;
	 rrc_ue_generate_RRCSetupComplete(
					  ctxt_pP,
					  gNB_index,
					  dl_ccch_msg->message.choice.c1->choice.rrcSetup->rrc_TransactionIdentifier,
					  NR_UE_rrc_inst[ctxt_pP->module_id].selected_plmn_identity);
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
       printf("%02x ",bufferP[i]);
     printf("\n");
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


 //-----------------------------------------------------------------------------
 void
 nr_rrc_ue_process_securityModeCommand(
   const protocol_ctxt_t *const ctxt_pP,
   NR_SecurityModeCommand_t *const securityModeCommand,
   const uint8_t                gNB_index
 )
 //-----------------------------------------------------------------------------
 {
   asn_enc_rval_t enc_rval;
   NR_UL_DCCH_Message_t ul_dcch_msg;
   uint8_t buffer[200];
   int i, securityMode;
   LOG_I(NR_RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), Processing securityModeCommand (eNB %d)\n",
	 ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, gNB_index);

   switch (securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm) {
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
   NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm =
   securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm;

   if (securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm != NULL)
   {
     switch (*securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm) {
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

     NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm =
     *securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm;

   }

   LOG_D(NR_RRC,"[UE %d] security mode is %x \n",ctxt_pP->module_id, securityMode);
   memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
   //memset((void *)&SecurityModeCommand,0,sizeof(SecurityModeCommand_t));
   ul_dcch_msg.message.present           = NR_UL_DCCH_MessageType_PR_c1;
   ul_dcch_msg.message.choice.c1         = calloc(1, sizeof(*ul_dcch_msg.message.choice.c1));

   if (securityMode >= NO_SECURITY_MODE) {
     LOG_I(NR_RRC, "rrc_ue_process_securityModeCommand, security mode complete case \n");
     ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
   } else {
     LOG_I(NR_RRC, "rrc_ue_process_securityModeCommand, security mode failure case \n");
     ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeFailure;
     ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
   }

   uint8_t *kRRCenc = NULL;
   uint8_t *kUPenc = NULL;
   uint8_t *kRRCint = NULL;
  nr_derive_key_up_enc(NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                       NR_UE_rrc_inst[ctxt_pP->module_id].kgnb,
                       &kUPenc);
  nr_derive_key_rrc_enc(NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                        NR_UE_rrc_inst[ctxt_pP->module_id].kgnb,
                       &kRRCenc);
  nr_derive_key_rrc_int(NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm,
                        NR_UE_rrc_inst[ctxt_pP->module_id].kgnb,
                       &kRRCint);
   LOG_I(NR_RRC, "driving kRRCenc, kRRCint and kUPenc from KgNB="
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x"
   "%02x%02x%02x%02x\n",
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[0],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[1],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[2],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[3],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[4],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[5],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[6],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[7],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[8],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[9],  NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[10], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[11],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[12], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[13], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[14], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[15],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[16], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[17], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[18], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[19],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[20], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[21], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[22], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[23],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[24], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[25], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[26], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[27],
   NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[28], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[29], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[30], NR_UE_rrc_inst[ctxt_pP->module_id].kgnb[31]);

   if (securityMode != 0xff) {
     pdcp_config_set_security(ctxt_pP, NULL, DCCH, DCCH+2,
                              NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm
                              | (NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm << 4),
                              kRRCenc, kRRCint, kUPenc);
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
     AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
		  enc_rval.failed_type->name, enc_rval.encoded);

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
    }
     log_dump(MAC, buffer, 16, LOG_DUMP_CHAR, "securityModeComplete payload: ");
     LOG_D(NR_RRC, "securityModeComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);

     for (i = 0; i < (enc_rval.encoded + 7) / 8; i++) {
       LOG_T(NR_RRC, "%02x.", buffer[i]);
     }

     LOG_T(NR_RRC, "\n");
 #ifdef ITTI_SIM
     MessageDef *message_p;
     uint8_t *message_buffer;
     message_buffer = itti_malloc (TASK_RRC_NRUE,TASK_RRC_GNB_SIM,
			(enc_rval.encoded + 7) / 8);
     memcpy (message_buffer, buffer, (enc_rval.encoded + 7) / 8);

     message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_DCCH_DATA_IND);
     GNB_RRC_DCCH_DATA_IND (message_p).rbid  = DCCH;
     GNB_RRC_DCCH_DATA_IND (message_p).sdu   = message_buffer;
     GNB_RRC_DCCH_DATA_IND (message_p).size    = (enc_rval.encoded + 7) / 8;
     itti_send_msg_to_task (TASK_RRC_GNB_SIM, ctxt_pP->instance, message_p);
 #else
     rrc_data_req_nr_ue (ctxt_pP,
                      DCCH,
                      nr_rrc_mui++,
                      SDU_CONFIRM_NO,
                      (enc_rval.encoded + 7) / 8,
                      buffer,
                      PDCP_TRANSMISSION_MODE_CONTROL);
 #endif
   } else
     LOG_W(NR_RRC,"securityModeCommand->criticalExtensions.present (%d) != NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand\n",
		  securityModeCommand->criticalExtensions.present);
 }

 //-----------------------------------------------------------------------------
 void nr_rrc_ue_generate_RRCSetupRequest(module_id_t module_id, const uint8_t gNB_index) {
   uint8_t i=0,rv[6];
   /* TODO: Melissa, this is not a proper fix. The NAS layer should be
      getting intialized and then the substate will not crash when AMF_MODE_ENABLED
      is equal to 1. However, as a side note, when we keep the code below,
      once the CBRA procedure is finished, the NAS layer is ran and the AMF_MODE_ENABLED
      is switched to one and the substate assertion in the nr_rrc_set_sub_state()
      does not happen. So show this to Raymond and maybe its okay? */
   if(get_softmodem_params()->sa && !get_softmodem_params()->emulate_l2) {
     AMF_MODE_ENABLED = 1;
   }
   if(NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size ==0) {
     // Get RRCConnectionRequest, fill random for now
     // Generate random byte stream for contention resolution
     for (i=0; i<6; i++) {
 #ifdef SMBV
       // if SMBV is configured the contention resolution needs to be fix for the connection procedure to succeed
       rv[i]=i;
 #else
       rv[i]=taus()&0xff;
 #endif
       LOG_T(NR_RRC,"%x.",rv[i]);
     }

     LOG_T(NR_RRC,"\n");
     NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size =
	do_RRCSetupRequest(
	  module_id,
	  (uint8_t *)NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.Payload,
          sizeof(NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.Payload),
	  rv);
     LOG_I(NR_RRC,"[UE %d] : Logical Channel UL-CCCH (SRB0), Generating RRCSetupRequest (bytes %d, gNB %d)\n",
	   module_id, NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size, gNB_index);

     for (i=0; i<NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size; i++) {
       LOG_T(NR_RRC,"%x.",NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.Payload[i]);
       //printf("%x.",NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.Payload[i]);

     }

     LOG_T(NR_RRC,"\n");
     //printf("\n");
     /*UE_rrc_inst[ue_mod_idP].Srb0[Idx].Tx_buffer.Payload[i] = taus()&0xff;
     UE_rrc_inst[ue_mod_idP].Srb0[Idx].Tx_buffer.payload_size =i; */

#ifdef ITTI_SIM
    MessageDef *message_p;
    uint8_t *message_buffer;
    message_buffer = itti_malloc (TASK_RRC_NRUE,TASK_RRC_GNB_SIM,
          NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size);
    memcpy (message_buffer, (uint8_t*)NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.Payload,
          NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size);
    message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_CCCH_DATA_IND);
    GNB_RRC_CCCH_DATA_IND (message_p).sdu = message_buffer;
    GNB_RRC_CCCH_DATA_IND (message_p).size  = NR_UE_rrc_inst[module_id].Srb0[gNB_index].Tx_buffer.payload_size;
    itti_send_msg_to_task (TASK_RRC_GNB_SIM, gNB_index, message_p);
#endif
  }
}

//-----------------------------------------------------------------------------
int32_t
nr_rrc_ue_establish_srb1(
    module_id_t       ue_mod_idP,
    frame_t           frameP,
    uint8_t           gNB_index,
    NR_SRB_ToAddMod_t *SRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  NR_UE_rrc_inst[ue_mod_idP].Srb1[gNB_index].Active = 1;
  NR_UE_rrc_inst[ue_mod_idP].Srb1[gNB_index].status = RADIO_CONFIG_OK;//RADIO CFG
  NR_UE_rrc_inst[ue_mod_idP].Srb1[gNB_index].Srb_info.Srb_id = 1;
  LOG_I(NR_RRC, "[UE %d], CONFIG_SRB1 %d corresponding to gNB_index %d\n", ue_mod_idP, DCCH, gNB_index);
  return(0);
}

//-----------------------------------------------------------------------------
int32_t
nr_rrc_ue_establish_srb2(
    module_id_t       ue_mod_idP,
    frame_t           frameP,
    uint8_t           gNB_index,
    NR_SRB_ToAddMod_t *SRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  NR_UE_rrc_inst[ue_mod_idP].Srb2[gNB_index].Active = 1;
  NR_UE_rrc_inst[ue_mod_idP].Srb2[gNB_index].status = RADIO_CONFIG_OK;//RADIO CFG
  NR_UE_rrc_inst[ue_mod_idP].Srb2[gNB_index].Srb_info.Srb_id = 2;
  LOG_I(NR_RRC, "[UE %d], CONFIG_SRB2 %d corresponding to gNB_index %d\n", ue_mod_idP, DCCH1, gNB_index);
  return(0);
}

 //-----------------------------------------------------------------------------
 int32_t
 nr_rrc_ue_establish_drb(
     module_id_t       ue_mod_idP,
     frame_t           frameP,
     uint8_t           gNB_index,
     NR_DRB_ToAddMod_t *DRB_config
 )
 //-----------------------------------------------------------------------------
 {
   // add descriptor from RRC PDU
   int oip_ifup = 0, ip_addr_offset3 = 0, ip_addr_offset4 = 0;
   /* avoid gcc warnings */
   (void)oip_ifup;
   (void)ip_addr_offset3;
   (void)ip_addr_offset4;
   LOG_I(NR_RRC,"[UE %d] Frame %d: processing RRCReconfiguration: reconfiguring DRB %ld\n",
	 ue_mod_idP, frameP, DRB_config->drb_Identity);

  if(!AMF_MODE_ENABLED) {
    ip_addr_offset3 = 0;
    ip_addr_offset4 = 1;
    LOG_I(OIP, "[UE %d] trying to bring up the OAI interface %d, IP X.Y.%d.%d\n", ue_mod_idP, ip_addr_offset3+ue_mod_idP,
	  ip_addr_offset3+ue_mod_idP+1, ip_addr_offset4+ue_mod_idP+1);
    oip_ifup = nas_config(ip_addr_offset3+ue_mod_idP+1,   // interface_id
			UE_NAS_USE_TUN?1:(ip_addr_offset3+ue_mod_idP+1), // third_octet
			ip_addr_offset4+ue_mod_idP+1, // fourth_octet
			"oip");                        // interface suffix (when using kernel module)

    if (oip_ifup == 0 && (!UE_NAS_USE_TUN)) { // interface is up --> send a config the DRB
      LOG_I(OIP, "[UE %d] Config the ue net interface %d to send/receive pkt on DRB %ld to/from the protocol stack\n",
	    ue_mod_idP,
	    ip_addr_offset3+ue_mod_idP,
	    (long int)((gNB_index * NR_maxDRB) + DRB_config->drb_Identity));
      rb_conf_ipv4(0,//add
		   ue_mod_idP,//cx align with the UE index
		   ip_addr_offset3+ue_mod_idP,//inst num_enb+ue_index
		   (gNB_index * NR_maxDRB) + DRB_config->drb_Identity,//rb
		   0,//dscp
		   ipv4_address(ip_addr_offset3+ue_mod_idP+1, ip_addr_offset4+ue_mod_idP+1),//saddr
		   ipv4_address(ip_addr_offset3+ue_mod_idP+1, gNB_index+1));//daddr
      LOG_D(NR_RRC,"[UE %d] State = Attached (gNB %d)\n",ue_mod_idP,gNB_index);
    }
  }

   return(0);
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

 //-----------------------------------------------------------------------------
 void
 nr_rrc_ue_process_RadioBearerConfig(
     const protocol_ctxt_t *const       ctxt_pP,
     const uint8_t                      gNB_index,
     NR_RadioBearerConfig_t *const      radioBearerConfig
 )
 //-----------------------------------------------------------------------------
 {
   long SRB_id, DRB_id;
   int i, cnt;

   if( radioBearerConfig->srb3_ToRelease != NULL){
     if( *radioBearerConfig->srb3_ToRelease == TRUE){
       //TODO (release the PDCP entity and the srb-Identity of the SRB3.)
     }
   }

   if (radioBearerConfig->srb_ToAddModList != NULL) {
     if (radioBearerConfig->securityConfig != NULL) {
       if (*radioBearerConfig->securityConfig->keyToUse == NR_SecurityConfig__keyToUse_master) {
	      NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm = radioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm;
	      NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm = *radioBearerConfig->securityConfig->securityAlgorithmConfig->integrityProtAlgorithm;
       }
     }

     uint8_t *kRRCenc = NULL;
     uint8_t *kRRCint = NULL;
     nr_derive_key_rrc_enc(NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                           NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kRRCenc);
     nr_derive_key_rrc_int(NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm,
                           NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kRRCint);

     // Refresh SRBs
      nr_rrc_pdcp_config_asn1_req(ctxt_pP,
                                  radioBearerConfig->srb_ToAddModList,
                                  NULL,
                                  NULL,
                                  NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm |
                                  (NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm << 4),
                                  kRRCenc,
                                  kRRCint,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList);
     // Refresh SRBs
      nr_rrc_rlc_config_asn1_req(ctxt_pP,
                                  radioBearerConfig->srb_ToAddModList,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList
                                  );

     for (cnt = 0; cnt < radioBearerConfig->srb_ToAddModList->list.count; cnt++) {
       SRB_id = radioBearerConfig->srb_ToAddModList->list.array[cnt]->srb_Identity;
       LOG_D(NR_RRC,"[UE %d]: Frame %d SRB config cnt %d (SRB%ld)\n", ctxt_pP->module_id, ctxt_pP->frame, cnt, SRB_id);
       if (SRB_id == 1) {
	 if (NR_UE_rrc_inst[ctxt_pP->module_id].SRB1_config[gNB_index]) {
	   memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].SRB1_config[gNB_index],
		  radioBearerConfig->srb_ToAddModList->list.array[cnt],
		  sizeof(NR_SRB_ToAddMod_t));
	 } else {
	   NR_UE_rrc_inst[ctxt_pP->module_id].SRB1_config[gNB_index] = radioBearerConfig->srb_ToAddModList->list.array[cnt];
	   nr_rrc_ue_establish_srb1(ctxt_pP->module_id,
				   ctxt_pP->frame,
				   gNB_index,
				   radioBearerConfig->srb_ToAddModList->list.array[cnt]);

	   LOG_I(NR_RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB1 gNB %d) --->][MAC_UE][MOD %02d][]\n",
	       ctxt_pP->frame, ctxt_pP->module_id, gNB_index, ctxt_pP->module_id);
	   nr_rrc_mac_config_req_ue_logicalChannelBearer(ctxt_pP->module_id,0,gNB_index,1,true); //todo handle mac_LogicalChannelConfig
	   // rrc_mac_config_req_ue
	 }
       } else {
	 if (NR_UE_rrc_inst[ctxt_pP->module_id].SRB2_config[gNB_index]) {
	   memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].SRB2_config[gNB_index],
	       radioBearerConfig->srb_ToAddModList->list.array[cnt], sizeof(NR_SRB_ToAddMod_t));
	 } else {
	   NR_UE_rrc_inst[ctxt_pP->module_id].SRB2_config[gNB_index] = radioBearerConfig->srb_ToAddModList->list.array[cnt];
	   nr_rrc_ue_establish_srb2(ctxt_pP->module_id,
				   ctxt_pP->frame,
				   gNB_index,
				   radioBearerConfig->srb_ToAddModList->list.array[cnt]);

	   LOG_I(NR_RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB2 gNB %d) --->][MAC_UE][MOD %02d][]\n",
	       ctxt_pP->frame, ctxt_pP->module_id, gNB_index, ctxt_pP->module_id);
	   nr_rrc_mac_config_req_ue_logicalChannelBearer(ctxt_pP->module_id,0,gNB_index,2,true); //todo handle mac_LogicalChannelConfig
	   // rrc_mac_config_req_ue
	 }
       } // srb2
     }
   } // srb_ToAddModList

   // Establish DRBs if present
   if (radioBearerConfig->drb_ToAddModList != NULL) {
     if ((NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB == NULL) &&
       (radioBearerConfig->drb_ToAddModList->list.count >= 1)) {
       NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB = malloc(sizeof(rb_id_t));
       *NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB = radioBearerConfig->drb_ToAddModList->list.array[0]->drb_Identity;
     }

     for (cnt = 0; cnt < radioBearerConfig->drb_ToAddModList->list.count; cnt++) {
       DRB_id = radioBearerConfig->drb_ToAddModList->list.array[cnt]->drb_Identity;
       if (NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1]) {
	 memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1],
		 radioBearerConfig->drb_ToAddModList->list.array[cnt], sizeof(NR_DRB_ToAddMod_t));
       } else {
	 //LOG_D(NR_RRC, "Adding DRB %ld %p\n", DRB_id-1, radioBearerConfig->drb_ToAddModList->list.array[cnt]);
	 NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1] = radioBearerConfig->drb_ToAddModList->list.array[cnt];
	 int j;
	 struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list = NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList;
	 if (rlc_bearer2add_list != NULL) {
	   for(j = 0; j < rlc_bearer2add_list->list.count; j++){
	     if(rlc_bearer2add_list->list.array[j]->servedRadioBearer != NULL){
	       if(rlc_bearer2add_list->list.array[j]->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity){
	         if(DRB_id == rlc_bearer2add_list->list.array[j]->servedRadioBearer->choice.drb_Identity){
	           LOG_I(NR_RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ (DRB lcid %ld gNB %d) --->][MAC_UE][MOD %02d][]\n",
	               ctxt_pP->frame, ctxt_pP->module_id, rlc_bearer2add_list->list.array[j]->logicalChannelIdentity, 0, ctxt_pP->module_id);
	           nr_rrc_mac_config_req_ue_logicalChannelBearer(ctxt_pP->module_id,0,0,rlc_bearer2add_list->list.array[j]->logicalChannelIdentity,true); //todo handle mac_LogicalChannelConfig
	         }
	       }
	     }
	   }
	 }
       }
     }

     uint8_t *kUPenc = NULL;
     uint8_t *kUPint = NULL;

     nr_derive_key_up_enc(NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                          NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kUPenc);
     nr_derive_key_up_int(NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm,
                          NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kUPint);

     MSC_LOG_TX_MESSAGE(
	 MSC_RRC_UE,
	 MSC_PDCP_UE,
	 NULL,
	 0,
	 MSC_AS_TIME_FMT" CONFIG_REQ UE %x DRB (security %X)",
	 MSC_AS_TIME_ARGS(ctxt_pP),
	 ctxt_pP->rnti,
	 NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm |
	 (NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm << 4));

       // Refresh DRBs
        nr_rrc_pdcp_config_asn1_req(ctxt_pP,
                                    NULL,
                                    radioBearerConfig->drb_ToAddModList,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL,
                                    kUPenc,
                                    kUPint,
                                    NULL,
                                    NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB,
                                    NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList);
       // Refresh DRBs
        nr_rrc_rlc_config_asn1_req(ctxt_pP,
                                    NULL,
                                    radioBearerConfig->drb_ToAddModList,
                                    NULL,
                                    NULL,
                                    NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToAddModList
                                    );
   } // drb_ToAddModList //

   if (radioBearerConfig->drb_ToReleaseList != NULL) {
     for (i = 0; i < radioBearerConfig->drb_ToReleaseList->list.count; i++) {
       DRB_id = *radioBearerConfig->drb_ToReleaseList->list.array[i];
       free(NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1]);
     }
   }

   if (NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToReleaseList != NULL) {
     for (i = 0; i < NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToReleaseList->list.count; i++) {
       NR_LogicalChannelIdentity_t lcid = *NR_UE_rrc_inst[ctxt_pP->module_id].cell_group_config->rlc_BearerToReleaseList->list.array[i];
       LOG_I(NR_RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ (RB lcid %ld gNB %d release) --->][MAC_UE][MOD %02d][]\n",
           ctxt_pP->frame, ctxt_pP->module_id, lcid, 0, ctxt_pP->module_id);
       nr_rrc_mac_config_req_ue_logicalChannelBearer(ctxt_pP->module_id,0,0,lcid,false); //todo handle mac_LogicalChannelConfig
     }
   }

   NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].State = NR_RRC_CONNECTED;
   LOG_I(NR_RRC,"[UE %d] State = NR_RRC_CONNECTED (gNB %d)\n", ctxt_pP->module_id, gNB_index);
 }

 //-----------------------------------------------------------------------------
 static void
 rrc_ue_process_rrcReconfiguration(
   const protocol_ctxt_t *const  ctxt_pP,
   NR_RRCReconfiguration_t       *rrcReconfiguration,
   uint8_t                       gNB_index
 )
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

     if(ie->nonCriticalExtension->masterCellGroup!=NULL) {
       nr_rrc_ue_process_masterCellGroup(
           ctxt_pP,
           gNB_index,
           ie->nonCriticalExtension->masterCellGroup);
     }

     if (ie->radioBearerConfig != NULL) {
       LOG_I(NR_RRC, "radio Bearer Configuration is present\n");
       nr_rrc_ue_process_RadioBearerConfig(ctxt_pP, gNB_index, ie->radioBearerConfig);
     }

     /* Check if there is dedicated NAS information to forward to NAS */
     if (ie->nonCriticalExtension->dedicatedNAS_MessageList != NULL) {
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

 //-----------------------------------------------------------------------------
 void nr_rrc_ue_generate_RRCReconfigurationComplete( const protocol_ctxt_t *const ctxt_pP, const uint8_t gNB_index, const uint8_t Transaction_id ) {
   uint8_t buffer[32], size;
   size = do_NR_RRCReconfigurationComplete(ctxt_pP, buffer, sizeof(buffer), Transaction_id);
   LOG_I(NR_RRC,PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel UL-DCCH (SRB1), Generating RRCReconfigurationComplete (bytes %d, gNB_index %d)\n",
	 PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP), size, gNB_index);
   LOG_D(RLC,
	 "[FRAME %05d][RRC_UE][INST %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCReconfigurationComplete to gNB %d MUI %d) --->][PDCP][INST %02d][RB %02d]\n",
	 ctxt_pP->frame,
	 UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
	 size,
	 gNB_index,
	 nr_rrc_mui,
	 UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
	 DCCH);
 #ifdef ITTI_SIM
   MessageDef *message_p;
   uint8_t *message_buffer;
   message_buffer = itti_malloc (TASK_RRC_NRUE,TASK_RRC_GNB_SIM,size);
   memcpy (message_buffer, buffer, size);

   message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_DCCH_DATA_IND);
   UE_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
   UE_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
   UE_RRC_DCCH_DATA_IND (message_p).size  = size;
   itti_send_msg_to_task (TASK_RRC_GNB_SIM, ctxt_pP->instance, message_p);

 #else
   rrc_data_req_nr_ue (
     ctxt_pP,
     DCCH,
     nr_rrc_mui++,
     SDU_CONFIRM_NO,
     size,
     buffer,
     PDCP_TRANSMISSION_MODE_CONTROL);
 #endif

 }

 // from NR SRB1
 //-----------------------------------------------------------------------------
 int
 nr_rrc_ue_decode_dcch(
   const protocol_ctxt_t *const ctxt_pP,
   const srb_id_t               Srb_id,
   const uint8_t         *const Buffer,
   size_t                       Buffer_size,
   const uint8_t                gNB_indexP
 )
 //-----------------------------------------------------------------------------
 {
   asn_dec_rval_t                      dec_rval;
   NR_DL_DCCH_Message_t                *dl_dcch_msg  = NULL;
   MessageDef *msg_p;

   if (Srb_id != 1) {
     LOG_E(NR_RRC,"[UE %d] Frame %d: Received message on DL-DCCH (SRB%ld), should not have ...\n",
           ctxt_pP->module_id, ctxt_pP->frame, Srb_id);
   } else {
     LOG_D(NR_RRC, "Received message on SRB%ld\n", Srb_id);
   }

   LOG_D(NR_RRC, "Decoding DL-DCCH Message\n");
   dec_rval = uper_decode( NULL,
			   &asn_DEF_NR_DL_DCCH_Message,
			   (void **)&dl_dcch_msg,
			   Buffer,
			   Buffer_size,
			   0,
			   0);

   if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
     LOG_E(NR_RRC, "Failed to decode DL-DCCH (%zu bytes)\n", dec_rval.consumed);
     return -1;
   }

   if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
     xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message,(void *)dl_dcch_msg);
   }

     if (dl_dcch_msg->message.present == NR_DL_DCCH_MessageType_PR_c1) {
	 switch (dl_dcch_msg->message.choice.c1->present) {
	     case NR_DL_DCCH_MessageType__c1_PR_NOTHING:
		 LOG_I(NR_RRC, "Received PR_NOTHING on DL-DCCH-Message\n");
		 break;

	     case NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration:
	     {
	       rrc_ue_process_rrcReconfiguration(ctxt_pP,
						   dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration,
						   gNB_indexP);
	       nr_rrc_ue_generate_RRCReconfigurationComplete(ctxt_pP,
					   gNB_indexP,
					   dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration->rrc_TransactionIdentifier);
	       break;
	     }

	     case NR_DL_DCCH_MessageType__c1_PR_rrcResume:
	     case NR_DL_DCCH_MessageType__c1_PR_rrcRelease:
	       LOG_I(NR_RRC, "[UE %d] Received RRC Release (gNB %d)\n",
		       ctxt_pP->module_id, gNB_indexP);

	       msg_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NAS_CONN_RELEASE_IND);

	       if((dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.present == NR_RRCRelease__criticalExtensions_PR_rrcRelease) &&
		    (dl_dcch_msg->message.choice.c1->present == NR_DL_DCCH_MessageType__c1_PR_rrcRelease)){
		     dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq->deprioritisationTimer =
		     NR_RRCRelease_IEs__deprioritisationReq__deprioritisationTimer_min5;
		     dl_dcch_msg->message.choice.c1->choice.rrcRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq->deprioritisationType =
		     NR_RRCRelease_IEs__deprioritisationReq__deprioritisationType_frequency;
		 }

		  itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
		  break;
	     case NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
         LOG_I(NR_RRC, "[UE %d] Received Capability Enquiry (gNB %d)\n", ctxt_pP->module_id,gNB_indexP);
         nr_rrc_ue_process_ueCapabilityEnquiry(
           ctxt_pP,
           dl_dcch_msg->message.choice.c1->choice.ueCapabilityEnquiry,
           gNB_indexP);
         break;
	     case NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment:
         LOG_I(NR_RRC,
             "[UE%d] Frame %d : Logical Channel DL-DCCH (SRB1), Received RRCReestablishment\n",
             ctxt_pP->module_id,
             ctxt_pP->frame);
         nr_rrc_ue_generate_rrcReestablishmentComplete(
           ctxt_pP,
           dl_dcch_msg->message.choice.c1->choice.rrcReestablishment,
           gNB_indexP);
		     break;
	     case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
	     {
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
	     }

	       break;
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
         LOG_I(NR_RRC, "[UE %d] Received securityModeCommand (gNB %d)\n",
               ctxt_pP->module_id, gNB_indexP);
         nr_rrc_ue_process_securityModeCommand(
             ctxt_pP,
             dl_dcch_msg->message.choice.c1->choice.securityModeCommand,
             gNB_indexP);

         break;
	    }
     }
   return 0;
 }

 //-----------------------------------------------------------------------------
 void *rrc_nrue_task( void *args_p ) {
   MessageDef   *msg_p;
   instance_t    instance;
   unsigned int  ue_mod_id;
   int           result;
   NR_SRB_INFO   *srb_info_p;
   protocol_ctxt_t  ctxt;
   itti_mark_task_ready (TASK_RRC_NRUE);

   while(1) {
     // Wait for a message
     itti_receive_msg (TASK_RRC_NRUE, &msg_p);
     instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);
     ue_mod_id = UE_INSTANCE_TO_MODULE_ID(instance);

     switch (ITTI_MSG_ID(msg_p)) {
       case TERMINATE_MESSAGE:
         LOG_W(NR_RRC, " *** Exiting RRC thread\n");
         itti_exit_task ();
         break;

       case MESSAGE_TEST:
         LOG_D(NR_RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
         break;

       case NR_RRC_MAC_BCCH_DATA_IND:
         LOG_D(NR_RRC, "[UE %d] Received %s: frameP %d, gNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
               NR_RRC_MAC_BCCH_DATA_IND (msg_p).frame, NR_RRC_MAC_BCCH_DATA_IND (msg_p).gnb_index);
         PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, GNB_FLAG_NO, NOT_A_RNTI, NR_RRC_MAC_BCCH_DATA_IND (msg_p).frame, 0,NR_RRC_MAC_BCCH_DATA_IND (msg_p).gnb_index);
         nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message (ctxt.module_id,
                  NR_RRC_MAC_BCCH_DATA_IND (msg_p).gnb_index,
                  NR_RRC_MAC_BCCH_DATA_IND (msg_p).sdu,
                  NR_RRC_MAC_BCCH_DATA_IND (msg_p).sdu_size,
                  NR_RRC_MAC_BCCH_DATA_IND (msg_p).rsrq,
                  NR_RRC_MAC_BCCH_DATA_IND (msg_p).rsrp);
         break;

       case NR_RRC_MAC_CCCH_DATA_IND:
         LOG_D(NR_RRC, "[UE %d] RNTI %x Received %s: frameP %d, gNB %d\n",
               ue_mod_id,
               NR_RRC_MAC_CCCH_DATA_IND (msg_p).rnti,
               ITTI_MSG_NAME (msg_p),
               NR_RRC_MAC_CCCH_DATA_IND (msg_p).frame,
               NR_RRC_MAC_CCCH_DATA_IND (msg_p).gnb_index);
         srb_info_p = &NR_UE_rrc_inst[ue_mod_id].Srb0[NR_RRC_MAC_CCCH_DATA_IND (msg_p).gnb_index];
         memcpy (srb_info_p->Rx_buffer.Payload, NR_RRC_MAC_CCCH_DATA_IND (msg_p).sdu,
           NR_RRC_MAC_CCCH_DATA_IND (msg_p).sdu_size);
         srb_info_p->Rx_buffer.payload_size = NR_RRC_MAC_CCCH_DATA_IND (msg_p).sdu_size;
         PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_NO, NR_RRC_MAC_CCCH_DATA_IND (msg_p).rnti, NR_RRC_MAC_CCCH_DATA_IND (msg_p).frame, 0);
              // PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, GNB_FLAG_NO, NR_RRC_MAC_CCCH_DATA_IND (msg_p).rnti, NR_RRC_MAC_CCCH_DATA_IND (msg_p).frame, 0, NR_RRC_MAC_CCCH_DATA_IND (msg_p).gnb_index);
              nr_rrc_ue_decode_ccch (&ctxt,
                                  srb_info_p,
                                  NR_RRC_MAC_CCCH_DATA_IND (msg_p).gnb_index);
         break;

      /* PDCP messages */
      case NR_RRC_DCCH_DATA_IND:
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, NR_RRC_DCCH_DATA_IND (msg_p).module_id, GNB_FLAG_NO, NR_RRC_DCCH_DATA_IND (msg_p).rnti, NR_RRC_DCCH_DATA_IND (msg_p).frame, 0,NR_RRC_DCCH_DATA_IND (msg_p).gNB_index);
        LOG_D(NR_RRC, "[UE %d] Received %s: frameP %d, DCCH %d, gNB %d\n",
              NR_RRC_DCCH_DATA_IND (msg_p).module_id,
              ITTI_MSG_NAME (msg_p),
              NR_RRC_DCCH_DATA_IND (msg_p).frame,
              NR_RRC_DCCH_DATA_IND (msg_p).dcch_index,
              NR_RRC_DCCH_DATA_IND (msg_p).gNB_index);
        LOG_D(NR_RRC, PROTOCOL_RRC_CTXT_UE_FMT"Received %s DCCH %d, gNB %d\n",
              PROTOCOL_NR_RRC_CTXT_UE_ARGS(&ctxt),
              ITTI_MSG_NAME (msg_p),
              NR_RRC_DCCH_DATA_IND (msg_p).dcch_index,
              NR_RRC_DCCH_DATA_IND (msg_p).gNB_index);
        nr_rrc_ue_decode_dcch (
          &ctxt,
          NR_RRC_DCCH_DATA_IND (msg_p).dcch_index,
          NR_RRC_DCCH_DATA_IND (msg_p).sdu_p,
          NR_RRC_DCCH_DATA_IND (msg_p).sdu_size,
          NR_RRC_DCCH_DATA_IND (msg_p).gNB_index);
        break;

      case NAS_KENB_REFRESH_REQ:
        memcpy((void *)NR_UE_rrc_inst[ue_mod_id].kgnb, (void *)NAS_KENB_REFRESH_REQ(msg_p).kenb, sizeof(NR_UE_rrc_inst[ue_mod_id].kgnb));
        LOG_D(RRC, "[UE %d] Received %s: refreshed RRC::KgNB = "
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x\n",
              ue_mod_id, ITTI_MSG_NAME (msg_p),
              NR_UE_rrc_inst[ue_mod_id].kgnb[0],  NR_UE_rrc_inst[ue_mod_id].kgnb[1],  NR_UE_rrc_inst[ue_mod_id].kgnb[2],  NR_UE_rrc_inst[ue_mod_id].kgnb[3],
              NR_UE_rrc_inst[ue_mod_id].kgnb[4],  NR_UE_rrc_inst[ue_mod_id].kgnb[5],  NR_UE_rrc_inst[ue_mod_id].kgnb[6],  NR_UE_rrc_inst[ue_mod_id].kgnb[7],
              NR_UE_rrc_inst[ue_mod_id].kgnb[8],  NR_UE_rrc_inst[ue_mod_id].kgnb[9],  NR_UE_rrc_inst[ue_mod_id].kgnb[10], NR_UE_rrc_inst[ue_mod_id].kgnb[11],
              NR_UE_rrc_inst[ue_mod_id].kgnb[12], NR_UE_rrc_inst[ue_mod_id].kgnb[13], NR_UE_rrc_inst[ue_mod_id].kgnb[14], NR_UE_rrc_inst[ue_mod_id].kgnb[15],
              NR_UE_rrc_inst[ue_mod_id].kgnb[16], NR_UE_rrc_inst[ue_mod_id].kgnb[17], NR_UE_rrc_inst[ue_mod_id].kgnb[18], NR_UE_rrc_inst[ue_mod_id].kgnb[19],
              NR_UE_rrc_inst[ue_mod_id].kgnb[20], NR_UE_rrc_inst[ue_mod_id].kgnb[21], NR_UE_rrc_inst[ue_mod_id].kgnb[22], NR_UE_rrc_inst[ue_mod_id].kgnb[23],
              NR_UE_rrc_inst[ue_mod_id].kgnb[24], NR_UE_rrc_inst[ue_mod_id].kgnb[25], NR_UE_rrc_inst[ue_mod_id].kgnb[26], NR_UE_rrc_inst[ue_mod_id].kgnb[27],
              NR_UE_rrc_inst[ue_mod_id].kgnb[28], NR_UE_rrc_inst[ue_mod_id].kgnb[29], NR_UE_rrc_inst[ue_mod_id].kgnb[30], NR_UE_rrc_inst[ue_mod_id].kgnb[31]);
        break;

      case NAS_UPLINK_DATA_REQ: {
        uint32_t length;
        uint8_t *buffer;
        LOG_I(NR_RRC, "[UE %d] Received %s: UEid %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), NAS_UPLINK_DATA_REQ (msg_p).UEid);
        /* Create message for PDCP (ULInformationTransfer_t) */
        length = do_NR_ULInformationTransfer(&buffer, NAS_UPLINK_DATA_REQ (msg_p).nasMsg.length, NAS_UPLINK_DATA_REQ (msg_p).nasMsg.data);
        /* Transfer data to PDCP */
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, GNB_FLAG_NO, NR_UE_rrc_inst[ue_mod_id].Info[0].rnti, 0, 0,0);
#ifdef ITTI_SIM
        MessageDef *message_p;
        uint8_t *message_buffer;
        message_buffer = itti_malloc (TASK_RRC_NRUE,TASK_RRC_GNB_SIM,length);
        memcpy (message_buffer, buffer, length);
        
        message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_DCCH_DATA_IND);
        if(NR_UE_rrc_inst[ue_mod_id].SRB2_config[0] == NULL) 
          UE_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
        else
          UE_RRC_DCCH_DATA_IND (message_p).rbid = DCCH1;
        UE_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
        UE_RRC_DCCH_DATA_IND (message_p).size  = length;
        itti_send_msg_to_task (TASK_RRC_GNB_SIM, ctxt.instance, message_p);

#else
        // check if SRB2 is created, if yes request data_req on DCCH1 (SRB2)
        if(NR_UE_rrc_inst[ue_mod_id].SRB2_config[0] == NULL) {
          rrc_data_req_nr_ue (&ctxt,
                           DCCH,
                           nr_rrc_mui++,
                           SDU_CONFIRM_NO,
                           length, buffer,
                           PDCP_TRANSMISSION_MODE_CONTROL);
        } else {
          rrc_data_req_nr_ue (&ctxt,
                           DCCH1,
                           nr_rrc_mui++,
                           SDU_CONFIRM_NO,
                           length, buffer,
                           PDCP_TRANSMISSION_MODE_CONTROL);
        }
#endif
        break;
      }

      default:
        LOG_E(NR_RRC, "[UE %d] Received unexpected message %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
    }
    LOG_D(NR_RRC, "[UE %d] RRC Status %d\n", ue_mod_id, nr_rrc_get_state(ue_mod_id));
    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
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

//-----------------------------------------------------------------------------
void
nr_rrc_ue_process_ueCapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  NR_UECapabilityEnquiry_t *UECapabilityEnquiry,
  uint8_t gNB_index
)
//-----------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  NR_UL_DCCH_Message_t ul_dcch_msg;
  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container;
  uint8_t buffer[200];
  int i;
  LOG_I(NR_RRC,"[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing UECapabilityEnquiry (gNB %d)\n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        gNB_index);
  memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
  memset((void *)&ue_CapabilityRAT_Container,0,sizeof(NR_UE_CapabilityRAT_Container_t));
  ul_dcch_msg.message.present            = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_UL_DCCH_MessageType__c1));
  ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation;
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation                            = CALLOC(1, sizeof(struct NR_UECapabilityInformation));
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->rrc_TransactionIdentifier = UECapabilityEnquiry->rrc_TransactionIdentifier;
  ue_CapabilityRAT_Container.rat_Type = NR_RAT_Type_nr;
  NR_UE_NR_Capability_t*             UE_Capability_nr;
  UE_Capability_nr = CALLOC(1,sizeof(NR_UE_NR_Capability_t));
  NR_BandNR_t *nr_bandnr;
  nr_bandnr  = CALLOC(1,sizeof(NR_BandNR_t));
  nr_bandnr->bandNR = 1;
  ASN_SEQUENCE_ADD(
    &UE_Capability_nr->rf_Parameters.supportedBandListNR.list,
    nr_bandnr);
  OAI_NR_UECapability_t *UECap;
  UECap = CALLOC(1,sizeof(OAI_NR_UECapability_t));
  UECap->UE_NR_Capability = UE_Capability_nr;
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_NR_UE_NR_Capability,(void *)UE_Capability_nr);
  }

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
  OCTET_STRING_t * requestedFreqBandsNR = UECapabilityEnquiry->criticalExtensions.choice.ueCapabilityEnquiry->ue_CapabilityEnquiryExt;
  nsa_sendmsg_to_lte_ue(requestedFreqBandsNR->buf, requestedFreqBandsNR->size, UE_CAPABILITY_INFO);
  //  ue_CapabilityRAT_Container.ueCapabilityRAT_Container.buf  = UE_rrc_inst[ue_mod_idP].UECapability;
  // ue_CapabilityRAT_Container.ueCapabilityRAT_Container.size = UE_rrc_inst[ue_mod_idP].UECapability_size;
  AssertFatal(UECapabilityEnquiry->criticalExtensions.present == NR_UECapabilityEnquiry__criticalExtensions_PR_ueCapabilityEnquiry,
              "UECapabilityEnquiry->criticalExtensions.present (%d) != UECapabilityEnquiry__criticalExtensions_PR_c1 (%d)\n",
              UECapabilityEnquiry->criticalExtensions.present,NR_UECapabilityEnquiry__criticalExtensions_PR_ueCapabilityEnquiry);

  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.present           = NR_UECapabilityInformation__criticalExtensions_PR_ueCapabilityInformation;
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation   = CALLOC(1, sizeof(struct NR_UECapabilityInformation_IEs));
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList             = CALLOC(1, sizeof(struct NR_UE_CapabilityRAT_ContainerList));
  ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.count = 0;

  for (i=0; i<UECapabilityEnquiry->criticalExtensions.choice.ueCapabilityEnquiry->ue_CapabilityRAT_RequestList.list.count; i++) {
    if (UECapabilityEnquiry->criticalExtensions.choice.ueCapabilityEnquiry->ue_CapabilityRAT_RequestList.list.array[i]->rat_Type
        == NR_RAT_Type_nr) {
      ASN_SEQUENCE_ADD(
        &ul_dcch_msg.message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list,
        &ue_CapabilityRAT_Container);
      enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, 100);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                   enc_rval.failed_type->name, enc_rval.encoded);

      if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
        xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
      }

      LOG_I(NR_RRC, "UECapabilityInformation Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
#ifdef ITTI_SIM
      MessageDef *message_p;
      uint8_t *message_buffer;
      message_buffer = itti_malloc (TASK_RRC_NRUE,TASK_RRC_GNB_SIM,
               (enc_rval.encoded + 7) / 8);
      memcpy (message_buffer, buffer, (enc_rval.encoded + 7) / 8);

      message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_DCCH_DATA_IND);
      GNB_RRC_DCCH_DATA_IND (message_p).rbid  = DCCH;
      GNB_RRC_DCCH_DATA_IND (message_p).sdu   = message_buffer;
      GNB_RRC_DCCH_DATA_IND (message_p).size  = (enc_rval.encoded + 7) / 8;
      itti_send_msg_to_task (TASK_RRC_GNB_SIM, ctxt_pP->instance, message_p);
#else
      rrc_data_req_nr_ue (
        ctxt_pP,
        DCCH,
        nr_rrc_mui++,
        SDU_CONFIRM_NO,
        (enc_rval.encoded + 7) / 8,
        buffer,
        PDCP_TRANSMISSION_MODE_CONTROL);
#endif
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
    uint32_t length;
    uint8_t buffer[100];
    length = do_RRCReestablishmentComplete(buffer, sizeof(buffer),
                                           rrcReestablishment->rrc_TransactionIdentifier);
    LOG_I(NR_RRC,"[UE %d][RAPROC] Frame %d : Logical Channel UL-DCCH (SRB1), Generating RRCReestablishmentComplete (bytes%d, gNB %d)\n",
          ctxt_pP->module_id,ctxt_pP->frame, length, gNB_index);
#ifdef ITTI_SIM
    MessageDef *message_p;
    uint8_t *message_buffer;
    message_buffer = itti_malloc (TASK_RRC_NRUE,TASK_RRC_GNB_SIM,length);
    memcpy (message_buffer, buffer, length);

    message_p = itti_alloc_new_message (TASK_RRC_NRUE, 0, UE_RRC_DCCH_DATA_IND);
    UE_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
    UE_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
    UE_RRC_DCCH_DATA_IND (message_p).size  = length;
    itti_send_msg_to_task (TASK_RRC_GNB_SIM, ctxt_pP->instance, message_p);

#endif
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
    init_queue(&nr_wait_ul_tti_req_queue);

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
  ASN_SEQUENCE_ADD(&UE_Capability_nr->rf_Parameters.supportedBandListNR.list, nr_bandnr);
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
            MessageDef *dummy_msg = itti_alloc_new_message(TASK_RRC_NSA_UE, 0, UE_CAPABILITY_DUMMY);
            LOG_D(NR_RRC, "We are calling nsa_sendmsg_to_lte_ue to send a UE_CAPABILITY_DUMMY\n");
            nsa_sendmsg_to_lte_ue(dummy_msg, sizeof(dummy_msg), UE_CAPABILITY_DUMMY);
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
                            &asn_DEF_LTE_MeasObjectToAddMod,
                            (void **)&nr_meas_obj,
                            msg_buffer,
                            msg_len);
            if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
            {
              SEQUENCE_free(&asn_DEF_LTE_MeasObjectToAddMod, nr_meas_obj, ASFM_FREE_EVERYTHING);
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
                      "nr_RadioBearerConfig1_r15 size %d nr_SecondaryCellGroupConfig_r15 size %d sizeof(hdr) %zu, msg_len = %d\n",
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
