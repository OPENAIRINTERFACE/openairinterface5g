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

/*! \file rrc_gNB.c
 * \brief rrc procedures for gNB
 * \author Navid Nikaein and  Raymond Knopp , WEI-TAI CHEN
 * \date 2011 - 2014 , 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr, kroempa@gmail.com
 */
#define RRC_GNB_C
#define RRC_GNB_C

#include "nr_rrc_config.h"
#include "nr_rrc_defs.h"
#include "nr_rrc_extern.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "asn1_conversions.h"

#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "COMMON/mac_rrc_primitives.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_UL-DCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_CellGroupConfig.h"
#include "NR_MeasResults.h"
#include "LTE_UECapabilityInformation.h"
#include "LTE_UL-DCCH-Message.h"
#include "NR_UL-CCCH-Message.h"
#include "NR_RRCSetupRequest-IEs.h"
#include "NR_RRCSetupComplete-IEs.h"

#include "rlc.h"
#include "rrc_eNB_UE_context.h"
#include "platform_types.h"
#include "msc.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "UTIL/OSA/osa_defs.h"

#include "rrc_eNB_S1AP.h"
#include "rrc_gNB_NGAP.h"

#include "rrc_eNB_GTPV1U.h"

#include "nr_pdcp/nr_pdcp_entity.h"
#include "pdcp.h"
#include "gtpv1u_eNB_task.h"


#include "intertask_interface.h"
#include "SIMULATION/TOOLS/sim.h" // for taus

#include "executables/softmodem-common.h"
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include <openair2/X2AP/x2ap_eNB.h>

#include "BIT_STRING.h"
#include "assertions.h"

//#define XER_PRINT

extern RAN_CONTEXT_t RC;

extern boolean_t nr_rrc_pdcp_config_asn1_req(
    const protocol_ctxt_t *const  ctxt_pP,
    NR_SRB_ToAddModList_t  *const srb2add_list,
    NR_DRB_ToAddModList_t  *const drb2add_list,
    NR_DRB_ToReleaseList_t *const drb2release_list,
    const uint8_t                   security_modeP,
    uint8_t                  *const kRRCenc,
    uint8_t                  *const kRRCint,
    uint8_t                  *const kUPenc
  #if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
    ,LTE_PMCH_InfoList_r9_t  *pmch_InfoList_r9
  #endif
    ,rb_id_t                 *const defaultDRB,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

extern rlc_op_status_t nr_rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const NR_SRB_ToAddModList_t   * const srb2add_listP,
    const NR_DRB_ToAddModList_t   * const drb2add_listP,
    const NR_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

static inline uint64_t bitStr_to_uint64(BIT_STRING_t *asn);

mui_t                               rrc_gNB_mui = 0;

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///

void openair_nr_rrc_on(const protocol_ctxt_t *const ctxt_pP) {
  LOG_I(NR_RRC, PROTOCOL_NR_RRC_CTXT_FMT" gNB:OPENAIR NR RRC IN....\n",PROTOCOL_NR_RRC_CTXT_ARGS(ctxt_pP));
  rrc_config_nr_buffer (&RC.nrrrc[ctxt_pP->module_id]->carrier.SI, BCCH, 1);
  RC.nrrrc[ctxt_pP->module_id]->carrier.SI.Active = 1;
  rrc_config_nr_buffer (&RC.nrrrc[ctxt_pP->module_id]->carrier.Srb0, CCCH, 1);
  RC.nrrrc[ctxt_pP->module_id]->carrier.Srb0.Active = 1;
}

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///

void rrc_gNB_process_SgNBAdditionRequest(
  const protocol_ctxt_t  *const ctxt_pP,
  rrc_gNB_ue_context_t   *ue_context_pP
) {
  rrc_gNB_generate_SgNBAdditionRequestAcknowledge(ctxt_pP,ue_context_pP);
}

void rrc_gNB_generate_SgNBAdditionRequestAcknowledge(
  const protocol_ctxt_t  *const ctxt_pP,
  rrc_gNB_ue_context_t   *const ue_context_pP) {
  //uint8_t size;
  //uint8_t buffer[100];
  //int     CC_id = ue_context_pP->ue_context.primaryCC_id;
  //OCTET_STRING_t                                      *secondaryCellGroup;
  NR_CellGroupConfig_t                                *cellGroupconfig;
  struct NR_CellGroupConfig__rlc_BearerToAddModList   *rlc_BearerToAddModList;
  struct NR_MAC_CellGroupConfig                       *mac_CellGroupConfig;
  struct NR_PhysicalCellGroupConfig                   *physicalCellGroupConfig;
  struct NR_SpCellConfig                              *spCellConfig;
  //struct NR_CellGroupConfig__sCellToAddModList        *sCellToAddModList;
  cellGroupconfig                           = CALLOC(1,sizeof(NR_CellGroupConfig_t));
  cellGroupconfig->rlc_BearerToAddModList   = CALLOC(1,sizeof(struct NR_CellGroupConfig__rlc_BearerToAddModList));
  cellGroupconfig->mac_CellGroupConfig      = CALLOC(1,sizeof(struct NR_MAC_CellGroupConfig));
  cellGroupconfig->physicalCellGroupConfig  = CALLOC(1,sizeof(struct NR_PhysicalCellGroupConfig));
  cellGroupconfig->spCellConfig             = CALLOC(1,sizeof(struct NR_SpCellConfig));
  //cellGroupconfig->sCellToAddModList        = CALLOC(1,sizeof(struct NR_CellGroupConfig__sCellToAddModList));
  rlc_BearerToAddModList   = cellGroupconfig->rlc_BearerToAddModList;
  mac_CellGroupConfig      = cellGroupconfig->mac_CellGroupConfig;
  physicalCellGroupConfig  = cellGroupconfig->physicalCellGroupConfig;
  spCellConfig             = cellGroupconfig->spCellConfig;
  //sCellToAddModList        = cellGroupconfig->sCellToAddModList;
  rlc_bearer_config_t *rlc_config;
  rlc_config = CALLOC(1,sizeof(rlc_bearer_config_t));
  //Fill rlc_bearer config value
  rrc_config_rlc_bearer(ctxt_pP->module_id,
                        ue_context_pP->ue_context.primaryCC_id,
                        rlc_config
                       );
  //Fill rlc_bearer config to structure
  do_RLC_BEARER(ctxt_pP->module_id,
                ue_context_pP->ue_context.primaryCC_id,
                rlc_BearerToAddModList,
                rlc_config);
  mac_cellgroup_t *mac_cellgroup_config;
  mac_cellgroup_config = CALLOC(1,sizeof(mac_cellgroup_t));
  //Fill mac_cellgroup_config config value
  rrc_config_mac_cellgroup(ctxt_pP->module_id,
                           ue_context_pP->ue_context.primaryCC_id,
                           mac_cellgroup_config
                          );
  //Fill mac_cellgroup config to structure
  do_MAC_CELLGROUP(ctxt_pP->module_id,
                   ue_context_pP->ue_context.primaryCC_id,
                   mac_CellGroupConfig,
                   mac_cellgroup_config);
  physicalcellgroup_t *physicalcellgroup_config;
  physicalcellgroup_config = CALLOC(1,sizeof(physicalcellgroup_t));
  //Fill physicalcellgroup_config config value
  rrc_config_physicalcellgroup(ctxt_pP->module_id,
                               ue_context_pP->ue_context.primaryCC_id,
                               physicalcellgroup_config
                              );
  //Fill physicalcellgroup config to structure
  do_PHYSICALCELLGROUP(ctxt_pP->module_id,
                       ue_context_pP->ue_context.primaryCC_id,
                       physicalCellGroupConfig,
                       physicalcellgroup_config);
  do_SpCellConfig(RC.nrrrc[ctxt_pP->module_id],
                  spCellConfig);
}

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///

static void init_NR_SI(gNB_RRC_INST *rrc, gNB_RrcConfigurationReq *configuration) {
  LOG_D(RRC,"%s()\n\n\n\n",__FUNCTION__);
  rrc->carrier.MIB             = (uint8_t *) malloc16(4);
  rrc->carrier.sizeof_MIB      = do_MIB_NR(rrc,0);
  rrc->carrier.sizeof_SIB1      = do_SIB1_NR(&rrc->carrier,configuration);
  LOG_I(NR_RRC,"Done init_NR_SI\n");
  rrc_mac_config_req_gNB(rrc->module_id,
                         rrc->carrier.ssb_SubcarrierOffset,
                         rrc->carrier.pdsch_AntennaPorts,
                         rrc->carrier.pusch_TargetSNRx10,
                         rrc->carrier.pucch_TargetSNRx10,
                         (NR_ServingCellConfigCommon_t *)rrc->carrier.servingcellconfigcommon,
                         0,
                         0, // WIP hardcoded rnti
                         (NR_CellGroupConfig_t *)NULL
                        );

  if (get_softmodem_params()->phy_test > 0 || get_softmodem_params()->do_ra > 0) {
    // This is for phytest only, emulate first X2 message if uecap.raw file is present
    FILE *fd;
    fd = fopen("uecap.raw","r");

    if (fd != NULL) {
      char buffer[4096];
      int msg_len=fread(buffer,1,4096,fd);
      LOG_I(RRC,"Read in %d bytes for uecap\n",msg_len);
      LTE_UL_DCCH_Message_t *LTE_UL_DCCH_Message;
      asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                                &asn_DEF_LTE_UL_DCCH_Message,
                                (void **)&LTE_UL_DCCH_Message,
                                (uint8_t *)buffer,
                                msg_len);

      if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
        AssertFatal(1==0,"NR_UL_DCCH_MESSAGE decode error\n");
        // free the memory
        SEQUENCE_free( &asn_DEF_LTE_UL_DCCH_Message, LTE_UL_DCCH_Message, 1 );
        return;
      }

      fclose(fd);
      xer_fprint(stdout,&asn_DEF_LTE_UL_DCCH_Message, LTE_UL_DCCH_Message);
      // recreate enough of X2 EN-DC Container
      AssertFatal(LTE_UL_DCCH_Message->message.choice.c1.present == LTE_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation,
                  "ueCapabilityInformation not present\n");
      NR_CG_ConfigInfo_t *CG_ConfigInfo = calloc(1,sizeof(*CG_ConfigInfo));
      CG_ConfigInfo->criticalExtensions.present = NR_CG_ConfigInfo__criticalExtensions_PR_c1;
      CG_ConfigInfo->criticalExtensions.choice.c1 = calloc(1,sizeof(*CG_ConfigInfo->criticalExtensions.choice.c1));
      CG_ConfigInfo->criticalExtensions.choice.c1->present = NR_CG_ConfigInfo__criticalExtensions__c1_PR_cg_ConfigInfo;
      CG_ConfigInfo->criticalExtensions.choice.c1->choice.cg_ConfigInfo = calloc(1,sizeof(*CG_ConfigInfo->criticalExtensions.choice.c1->choice.cg_ConfigInfo));
      NR_CG_ConfigInfo_IEs_t *cg_ConfigInfo = CG_ConfigInfo->criticalExtensions.choice.c1->choice.cg_ConfigInfo;
      cg_ConfigInfo->ue_CapabilityInfo = calloc(1,sizeof(*cg_ConfigInfo->ue_CapabilityInfo));
      asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UE_CapabilityRAT_ContainerList,NULL,
                                (void *)&LTE_UL_DCCH_Message->message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList,buffer,4096);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                   enc_rval.failed_type->name, enc_rval.encoded);
      OCTET_STRING_fromBuf(cg_ConfigInfo->ue_CapabilityInfo,
                           (const char *)buffer,
                           (enc_rval.encoded+7)>>3);
      parse_CG_ConfigInfo(rrc,CG_ConfigInfo,NULL);
    } else {
      struct rrc_gNB_ue_context_s *ue_context_p = rrc_gNB_allocate_new_UE_context(rrc);
      LOG_I(NR_RRC,"Adding new user (%p)\n",ue_context_p);
      rrc_add_nsa_user(rrc,ue_context_p,NULL);
    }
  }
}


char openair_rrc_gNB_configuration(const module_id_t gnb_mod_idP, gNB_RrcConfigurationReq *configuration) {
  protocol_ctxt_t      ctxt;
  gNB_RRC_INST         *rrc=RC.nrrrc[gnb_mod_idP];
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, gnb_mod_idP, GNB_FLAG_YES, NOT_A_RNTI, 0, 0,gnb_mod_idP);
  LOG_I(NR_RRC,
        PROTOCOL_NR_RRC_CTXT_FMT" Init...\n",
        PROTOCOL_NR_RRC_CTXT_ARGS(&ctxt));

  AssertFatal(rrc != NULL, "RC.nrrrc not initialized!");
  AssertFatal(NUMBER_OF_UE_MAX < (module_id_t)0xFFFFFFFFFFFFFFFF, " variable overflow");
  AssertFatal(configuration!=NULL,"configuration input is null\n");
  rrc->module_id = gnb_mod_idP;
  rrc->Nb_ue = 0;
  rrc->carrier.Srb0.Active = 0;
  nr_uid_linear_allocator_init(&rrc->uid_allocator);
  RB_INIT(&rrc->rrc_ue_head);
  rrc->initial_id2_s1ap_ids = hashtable_create (NUMBER_OF_UE_MAX * 2, NULL, NULL);
  rrc->s1ap_id2_s1ap_ids    = hashtable_create (NUMBER_OF_UE_MAX * 2, NULL, NULL);
  rrc->carrier.servingcellconfigcommon = configuration->scc;
  rrc->carrier.ssb_SubcarrierOffset = configuration->ssb_SubcarrierOffset;
  rrc->carrier.pdsch_AntennaPorts = configuration->pdsch_AntennaPorts;
  rrc->carrier.pusch_TargetSNRx10 = configuration->pusch_TargetSNRx10;
  rrc->carrier.pucch_TargetSNRx10 = configuration->pucch_TargetSNRx10;
  /// System Information INIT
  LOG_I(NR_RRC, PROTOCOL_NR_RRC_CTXT_FMT" Checking release \n",PROTOCOL_NR_RRC_CTXT_ARGS(&ctxt));
  init_NR_SI(rrc, configuration);
  rrc_init_nr_global_param();
  openair_nr_rrc_on(&ctxt);
  return 0;
}//END openair_rrc_gNB_configuration


void rrc_gNB_process_AdditionRequestInformation(const module_id_t gnb_mod_idP, x2ap_ENDC_sgnb_addition_req_t *m) {
  struct NR_CG_ConfigInfo *cg_configinfo = NULL;
  asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                            &asn_DEF_NR_CG_ConfigInfo,
                            (void **)&cg_configinfo,
                            (uint8_t *)m->rrc_buffer,
                            (int) m->rrc_buffer_size);//m->rrc_buffer_size);
  gNB_RRC_INST         *rrc=RC.nrrrc[gnb_mod_idP];

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    AssertFatal(1==0,"NR_UL_DCCH_MESSAGE decode error\n");
    // free the memory
    SEQUENCE_free(&asn_DEF_NR_CG_ConfigInfo, cg_configinfo, 1);
    return;
  }

  xer_fprint(stdout,&asn_DEF_NR_CG_ConfigInfo, cg_configinfo);
  // recreate enough of X2 EN-DC Container
  AssertFatal(cg_configinfo->criticalExtensions.choice.c1->present == NR_CG_ConfigInfo__criticalExtensions__c1_PR_cg_ConfigInfo,
              "ueCapabilityInformation not present\n");
  parse_CG_ConfigInfo(rrc,cg_configinfo,m);
}


//-----------------------------------------------------------------------------
uint8_t
rrc_gNB_get_next_transaction_identifier(
    module_id_t gnb_mod_idP
)
//-----------------------------------------------------------------------------
{
  static uint8_t                      nr_rrc_transaction_identifier[NUMBER_OF_gNB_MAX];
  nr_rrc_transaction_identifier[gnb_mod_idP] = (nr_rrc_transaction_identifier[gnb_mod_idP] + 1) % NR_RRC_TRANSACTION_IDENTIFIER_NUMBER;
  LOG_T(NR_RRC, "generated xid is %d\n", nr_rrc_transaction_identifier[gnb_mod_idP]);
  return nr_rrc_transaction_identifier[gnb_mod_idP];
}

//-----------------------------------------------------------------------------
void
rrc_gNB_generate_RRCSetup(
    const protocol_ctxt_t    *const ctxt_pP,
    rrc_gNB_ue_context_t     *const ue_context_pP,
    const int                CC_id
)
//-----------------------------------------------------------------------------
{
  LOG_I(NR_RRC, "rrc_gNB_generate_RRCSetup \n");
  NR_SRB_ToAddModList_t        *SRB_configList = NULL;

  // T(T_GNB_RRC_SETUP,
  //   T_INT(ctxt_pP->module_id),
  //   T_INT(ctxt_pP->frame),
  //   T_INT(ctxt_pP->subframe),
  //   T_INT(ctxt_pP->rnti));
  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  SRB_configList = ue_p->SRB_configList;
  ue_p->Srb0.Tx_buffer.payload_size = do_RRCSetup(ctxt_pP,
              ue_context_pP,
              CC_id,
              (uint8_t *) ue_p->Srb0.Tx_buffer.Payload,
              rrc_gNB_get_next_transaction_identifier(ctxt_pP->module_id),
              SRB_configList);

  LOG_DUMPMSG(NR_RRC, DEBUG_RRC,
              (char *)(ue_p->Srb0.Tx_buffer.Payload),
              ue_p->Srb0.Tx_buffer.payload_size,
              "[MSG] RRC Setup\n");

  LOG_D(NR_RRC,
      PROTOCOL_NR_RRC_CTXT_UE_FMT" RRC_gNB --- MAC_CONFIG_REQ  (SRB1) ---> MAC_gNB\n",
      PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));

  // rrc_mac_config_req_eNB

  MSC_LOG_TX_MESSAGE(
      MSC_RRC_GNB,
      MSC_RRC_UE,
      ue_p->Srb0.Tx_buffer.Header, // LG WARNING
      ue_p->Srb0.Tx_buffer.payload_size,
      MSC_AS_TIME_FMT" RRCSetup UE %x size %u",
      MSC_AS_TIME_ARGS(ctxt_pP),
      ue_context_pP->ue_context.rnti,
      ue_p->Srb0.Tx_buffer.payload_size);
  LOG_I(NR_RRC,
      PROTOCOL_NR_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel DL-CCCH, Generating RRCSetup (bytes %d)\n",
      PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
      ue_p->Srb0.Tx_buffer.payload_size);
  // activate release timer, if RRCSetupComplete not received after 100 frames, remove UE
  ue_context_pP->ue_context.ue_release_timer = 1;
  // remove UE after 10 frames after RRCConnectionRelease is triggered
  ue_context_pP->ue_context.ue_release_timer_thres = 1000;
  /* init timers */
  //   ue_context_pP->ue_context.ue_rrc_inactivity_timer = 0;
#ifdef ITTI_SIM
  MessageDef *message_p;
  uint8_t *message_buffer;
  message_buffer = itti_malloc (TASK_RRC_GNB_SIM, TASK_RRC_UE_SIM,
            ue_p->Srb0.Tx_buffer.payload_size);
  memcpy (message_buffer, (uint8_t*)ue_p->Srb0.Tx_buffer.Payload, ue_p->Srb0.Tx_buffer.payload_size);
  message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, GNB_RRC_CCCH_DATA_IND);
  GNB_RRC_CCCH_DATA_IND (message_p).sdu = message_buffer;
  GNB_RRC_CCCH_DATA_IND (message_p).size  = ue_p->Srb0.Tx_buffer.payload_size;
  itti_send_msg_to_task (TASK_RRC_UE_SIM, ctxt_pP->instance, message_p);
#endif
}

void
rrc_gNB_generate_RRCReject(
    const protocol_ctxt_t    *const ctxt_pP,
    rrc_gNB_ue_context_t     *const ue_context_pP,
    const int                CC_id
)
//-----------------------------------------------------------------------------
{
  LOG_I(NR_RRC, "rrc_gNB_generate_RRCReject \n");
  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  ue_p->Srb0.Tx_buffer.payload_size = do_RRCReject(ctxt_pP->module_id,
                                                  (uint8_t *)ue_p->Srb0.Tx_buffer.Payload);
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC,
              (char *)(ue_p->Srb0.Tx_buffer.Payload),
              ue_p->Srb0.Tx_buffer.payload_size,
              "[MSG] RRCReject \n");
  MSC_LOG_TX_MESSAGE(MSC_RRC_GNB,
                    MSC_RRC_UE,
                    ue_p->Srb0.Tx_buffer.Header,
                    ue_p->Srb0.Tx_buffer.payload_size,
                    MSC_AS_TIME_FMT" NR_RRCReject UE %x size %u",
                    MSC_AS_TIME_ARGS(ctxt_pP),
                    ue_context_pP == NULL ? -1 : ue_context_pP->ue_context.rnti,
                    ue_p->Srb0.Tx_buffer.payload_size);
  LOG_I(NR_RRC,
      PROTOCOL_NR_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel DL-CCCH, Generating NR_RRCReject (bytes %d)\n",
      PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
      ue_p->Srb0.Tx_buffer.payload_size);

#ifdef ITTI_SIM
  MessageDef *message_p;
  uint8_t *message_buffer;
  message_buffer = itti_malloc (TASK_RRC_GNB_SIM, TASK_RRC_UE_SIM,
            ue_p->Srb0.Tx_buffer.payload_size);
  memcpy (message_buffer, (uint8_t*)ue_p->Srb0.Tx_buffer.Payload, ue_p->Srb0.Tx_buffer.payload_size);
  message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, GNB_RRC_CCCH_DATA_IND);
  GNB_RRC_CCCH_DATA_IND (message_p).sdu = message_buffer;
  GNB_RRC_CCCH_DATA_IND (message_p).size  = ue_p->Srb0.Tx_buffer.payload_size;
  itti_send_msg_to_task (TASK_RRC_UE_SIM, ctxt_pP->instance, message_p);
#endif
}

//-----------------------------------------------------------------------------
/*
* Process the rrc setup complete message from UE (SRB1 Active)
*/
void
rrc_gNB_process_RRCSetupComplete(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_gNB_ue_context_t      *ue_context_pP,
  NR_RRCSetupComplete_IEs_t *rrcSetupComplete
)
//-----------------------------------------------------------------------------
{
  LOG_I(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" [RAPROC] Logical Channel UL-DCCH, " "processing NR_RRCSetupComplete from UE (SRB1 Active)\n",
      PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
  ue_context_pP->ue_context.Srb1.Active = 1;
  ue_context_pP->ue_context.Status = NR_RRC_CONNECTED;

  if (AMF_MODE_ENABLED) {
    rrc_gNB_send_NGAP_NAS_FIRST_REQ(ctxt_pP, ue_context_pP, rrcSetupComplete);
  } else {
    rrc_gNB_generate_SecurityModeCommand(ctxt_pP, ue_context_pP);
  }
}

//-----------------------------------------------------------------------------
void 
rrc_gNB_generate_defaultRRCReconfiguration(
    const protocol_ctxt_t     *const ctxt_pP,
    rrc_gNB_ue_context_t      *ue_context_pP
)
//-----------------------------------------------------------------------------
{
  // gNB_RRC_UE_t           *ue_p = &ue_context_pP->ue_context;
  uint8_t                buffer[RRC_BUF_SIZE];
  uint16_t               size;
  gNB_RRC_INST           *gnb_rrc_inst = RC.nrrrc[ctxt_pP->module_id];

  size = do_RRCReconfiguration(ctxt_pP, ue_context_pP, buffer,
                               rrc_gNB_get_next_transaction_identifier(ctxt_pP->module_id),
                               gnb_rrc_inst);
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC,(char *)buffer, size, "[MSG] RRC Reconfiguration\n");

  free(ue_context_pP->ue_context.nas_pdu.buffer);

  LOG_I(NR_RRC, "[gNB %d] Frame %d, Logical Channel DL-DCCH, Generate NR_RRCReconfiguration (bytes %d, UE id %x)\n",
          ctxt_pP->module_id,
          ctxt_pP->frame,
          size,
          ue_context_pP->ue_context.rnti);
  LOG_D(NR_RRC, "[FRAME %05d][RRC_gNB][MOD %u][][--- PDCP_DATA_REQ/%d Bytes (rrcReconfiguration to UE %x MUI %d) --->][PDCP][MOD %u][RB %u]\n",
          ctxt_pP->frame,
          ctxt_pP->module_id,
          size,
          ue_context_pP->ue_context.rnti,
          rrc_gNB_mui,
          ctxt_pP->module_id,
          DCCH);
  MSC_LOG_TX_MESSAGE(MSC_RRC_GNB,
          MSC_RRC_UE,
          buffer,
          size,
          MSC_AS_TIME_FMT" NR_RRCReconfiguration UE %x MUI %d size %u",
          MSC_AS_TIME_ARGS(ctxt_pP),
          ue_context_pP->ue_context.rnti,
          rrc_gNB_mui,
          size);
#ifdef ITTI_SIM
  MessageDef *message_p;
  uint8_t *message_buffer;
  message_buffer = itti_malloc (TASK_RRC_GNB_SIM, TASK_RRC_UE_SIM, size);
  memcpy (message_buffer, buffer, size);
  message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, GNB_RRC_DCCH_DATA_IND);
  GNB_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
  GNB_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
  GNB_RRC_DCCH_DATA_IND (message_p).size	= size;
  itti_send_msg_to_task (TASK_RRC_UE_SIM, ctxt_pP->instance, message_p);
#else
  nr_rrc_data_req(ctxt_pP,
              DCCH,
              rrc_gNB_mui++,
              SDU_CONFIRM_NO,
              size,
              buffer,
              PDCP_TRANSMISSION_MODE_CONTROL);
#endif
  // rrc_pdcp_config_asn1_req
  // rrc_rlc_config_asn1_req
}

//-----------------------------------------------------------------------------
/*
* Process the RRC Reconfiguration Complete from the UE
*/
void
rrc_gNB_process_RRCReconfigurationComplete(
    const protocol_ctxt_t *const ctxt_pP,
    rrc_gNB_ue_context_t  *ue_context_pP,
    const uint8_t xid
)
{
  int                                 drb_id;
  uint8_t                            *kRRCenc = NULL;
  uint8_t                            *kRRCint = NULL;
  uint8_t                            *kUPenc = NULL;
  NR_DRB_ToAddModList_t              *DRB_configList = ue_context_pP->ue_context.DRB_configList2[xid];
  NR_SRB_ToAddModList_t              *SRB_configList = ue_context_pP->ue_context.SRB_configList2[xid];
  NR_DRB_ToReleaseList_t             *DRB_Release_configList2 = ue_context_pP->ue_context.DRB_Release_configList2[xid];
  NR_DRB_Identity_t                  *drb_id_p      = NULL;
  uint8_t                             nr_DRB2LCHAN[8];

  ue_context_pP->ue_context.ue_reestablishment_timer = 0;

#ifndef PHYSIM
  /* Derive the keys from kgnb */
  if (DRB_configList != NULL) {
      derive_key_up_enc(ue_context_pP->ue_context.ciphering_algorithm,
                      ue_context_pP->ue_context.kgnb,
                      &kUPenc);
  }

  derive_key_rrc_enc(ue_context_pP->ue_context.ciphering_algorithm,
                      ue_context_pP->ue_context.kgnb,
                      &kRRCenc);
  derive_key_rrc_int(ue_context_pP->ue_context.integrity_algorithm,
                      ue_context_pP->ue_context.kgnb,
                      &kRRCint);
#endif
  /* Refresh SRBs/DRBs */
  MSC_LOG_TX_MESSAGE(MSC_RRC_GNB, MSC_PDCP_ENB, NULL, 0, MSC_AS_TIME_FMT" CONFIG_REQ UE %x DRB (security unchanged)",
                   MSC_AS_TIME_ARGS(ctxt_pP),
                   ue_context_pP->ue_context.rnti);

#ifndef ITTI_SIM
  nr_rrc_pdcp_config_asn1_req(ctxt_pP,
                              SRB_configList, // NULL,
                              DRB_configList,
                              DRB_Release_configList2,
                              0xff, // already configured during the securitymodecommand
                              kRRCenc,
                              kRRCint,
                              kUPenc,
                              NULL,
                              NULL,
                              NULL);
  /* Refresh SRBs/DRBs */
  nr_rrc_rlc_config_asn1_req(ctxt_pP,
                          SRB_configList, // NULL,
                          DRB_configList,
                          DRB_Release_configList2,
                          NULL,
                          NULL);
#endif

  /* Set the SRB active in UE context */
  if (SRB_configList != NULL) {
    for (int i = 0; (i < SRB_configList->list.count) && (i < 3); i++) {
      if (SRB_configList->list.array[i]->srb_Identity == 1) {
        ue_context_pP->ue_context.Srb1.Active = 1;
      } else if (SRB_configList->list.array[i]->srb_Identity == 2) {
        ue_context_pP->ue_context.Srb2.Active = 1;
        ue_context_pP->ue_context.Srb2.Srb_info.Srb_id = 2;
        LOG_I(NR_RRC,"[gNB %d] Frame      %d CC %d : SRB2 is now active\n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              ue_context_pP->ue_context.primaryCC_id);
      } else {
        LOG_W(NR_RRC,"[gNB %d] Frame      %d CC %d : invalide SRB identity %ld\n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              ue_context_pP->ue_context.primaryCC_id,
              SRB_configList->list.array[i]->srb_Identity);
      }
    }

    free(SRB_configList);
    ue_context_pP->ue_context.SRB_configList2[xid] = NULL;
  }

  /* Loop through DRBs and establish if necessary */
  if (DRB_configList != NULL) {
    for (int i = 0; i < DRB_configList->list.count; i++) {
      if (DRB_configList->list.array[i]) {
        drb_id = (int)DRB_configList->list.array[i]->drb_Identity;
        LOG_I(NR_RRC, "[gNB %d] Frame  %d : Logical Channel UL-DCCH, Received NR_RRCReconfigurationComplete from UE rnti %x, reconfiguring DRB %d/LCID %d\n",
            ctxt_pP->module_id,
            ctxt_pP->frame,
            ctxt_pP->rnti,
            (int)DRB_configList->list.array[i]->drb_Identity,
            (int)*DRB_configList->list.array[i]->pdcp_Config->moreThanOneRLC->primaryPath.logicalChannel);

        if (ue_context_pP->ue_context.DRB_active[drb_id] == 0) {
          ue_context_pP->ue_context.DRB_active[drb_id] = 1;
          LOG_D(NR_RRC, "[gNB %d] Frame %d: Establish RLC UM Bidirectional, DRB %d Active\n",
                  ctxt_pP->module_id, ctxt_pP->frame, (int)DRB_configList->list.array[i]->drb_Identity);

          LOG_D(NR_RRC,
                  PROTOCOL_NR_RRC_CTXT_UE_FMT" RRC_gNB --- MAC_CONFIG_REQ  (DRB) ---> MAC_gNB\n",
                  PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));

          if (DRB_configList->list.array[i]->pdcp_Config->moreThanOneRLC->primaryPath.logicalChannel) {
            nr_DRB2LCHAN[i] = (uint8_t) * DRB_configList->list.array[i]->pdcp_Config->moreThanOneRLC->primaryPath.logicalChannel;
          }

            // rrc_mac_config_req_eNB
        } else {        // remove LCHAN from MAC/PHY
          if (ue_context_pP->ue_context.DRB_active[drb_id] == 1) {
            // DRB has just been removed so remove RLC + PDCP for DRB
            /*      rrc_pdcp_config_req (ctxt_pP->module_id, frameP, 1, CONFIG_ACTION_REMOVE,
            (ue_mod_idP * NB_RB_MAX) + DRB2LCHAN[i],UNDEF_SECURITY_MODE);
            */
            rrc_rlc_config_req(ctxt_pP,
                                SRB_FLAG_NO,
                                MBMS_FLAG_NO,
                                CONFIG_ACTION_REMOVE,
                                nr_DRB2LCHAN[i],
                                Rlc_info_um);
          }

          ue_context_pP->ue_context.DRB_active[drb_id] = 0;
          LOG_D(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" RRC_eNB --- MAC_CONFIG_REQ  (DRB) ---> MAC_eNB\n",
                  PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));

          // rrc_mac_config_req_eNB

        } // end else of if (ue_context_pP->ue_context.DRB_active[drb_id] == 0)
      } // end if (DRB_configList->list.array[i])
    } // end for (int i = 0; i < DRB_configList->list.count; i++)

    free(DRB_configList);
    ue_context_pP->ue_context.DRB_configList2[xid] = NULL;
  } // end if DRB_configList != NULL

  if(DRB_Release_configList2 != NULL) {
    for (int i = 0; i < DRB_Release_configList2->list.count; i++) {
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

/*------------------------------------------------------------------------------*/
int nr_rrc_gNB_decode_ccch(protocol_ctxt_t    *const ctxt_pP,
                           const uint8_t      *buffer,
                           int                buffer_length,
                           const int          CC_id)
{
  asn_dec_rval_t                                    dec_rval;
  NR_UL_CCCH_Message_t                             *ul_ccch_msg = NULL;
  struct rrc_gNB_ue_context_s                      *ue_context_p = NULL;
  gNB_RRC_INST                                     *gnb_rrc_inst = RC.nrrrc[ctxt_pP->module_id];
  NR_RRCSetupRequest_IEs_t                         *rrcSetupRequest = NULL;
  uint64_t                                         random_value = 0;

  dec_rval = uper_decode( NULL,
                          &asn_DEF_NR_UL_CCCH_Message,
                          (void **)&ul_ccch_msg,
                          (uint8_t *) buffer,
                          100,
                          0,
                          0);

  if (dec_rval.consumed == 0) {
    /* TODO */
    LOG_E(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" FATAL Error in receiving CCCH\n",
               PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
    return -1;
  }

  if (ul_ccch_msg->message.present == NR_UL_CCCH_MessageType_PR_c1) {
    switch (ul_ccch_msg->message.choice.c1->present) {
      case NR_UL_CCCH_MessageType__c1_PR_NOTHING:
        /* TODO */
        LOG_I(NR_RRC,
            PROTOCOL_NR_RRC_CTXT_FMT" Received PR_NOTHING on UL-CCCH-Message\n",
            PROTOCOL_NR_RRC_CTXT_ARGS(ctxt_pP));
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcSetupRequest:
        ue_context_p = rrc_gNB_get_ue_context(gnb_rrc_inst, ctxt_pP->rnti);
        if (ue_context_p != NULL) {
          rrc_gNB_free_mem_UE_context(ctxt_pP, ue_context_p);
          MSC_LOG_RX_DISCARDED_MESSAGE(
              MSC_RRC_GNB,
              MSC_RRC_UE,
              buffer,
              dec_rval.consumed,
              MSC_AS_TIME_FMT" NR_RRCSetupRequest UE %x size %u (UE already in context)",
              MSC_AS_TIME_ARGS(ctxt_pP),
              ue_context_p->ue_context.rnti,
              dec_rval.consumed);
        } else {
          rrcSetupRequest = &ul_ccch_msg->message.choice.c1->choice.rrcSetupRequest->rrcSetupRequest;
          if (NR_InitialUE_Identity_PR_randomValue == rrcSetupRequest->ue_Identity.present) {
            /* randomValue                         BIT STRING (SIZE (39)) */
            if (rrcSetupRequest->ue_Identity.choice.randomValue.size != 5) { // 39-bit random value
              LOG_E(NR_RRC, "wrong InitialUE-Identity randomValue size, expected 5, provided %lu",
                          (long unsigned int)rrcSetupRequest->ue_Identity.choice.randomValue.size);
              return -1;
            }

            memcpy(((uint8_t *) & random_value) + 3,
                    rrcSetupRequest->ue_Identity.choice.randomValue.buf,
                    rrcSetupRequest->ue_Identity.choice.randomValue.size);

            /* if there is already a registered UE (with another RNTI) with this random_value,
            * the current one must be removed from MAC/PHY (zombie UE)
            */
            if ((ue_context_p = rrc_gNB_ue_context_random_exist(RC.nrrrc[ctxt_pP->module_id], random_value))) {
              LOG_W(NR_RRC, "new UE rnti %x (coming with random value) is already there as UE %x, removing %x from MAC/PHY\n",
                      ctxt_pP->rnti, ue_context_p->ue_context.rnti, ue_context_p->ue_context.rnti);
              ue_context_p->ue_context.ul_failure_timer = 20000;
            }

            ue_context_p = rrc_gNB_get_next_free_ue_context(ctxt_pP, RC.nrrrc[ctxt_pP->module_id], random_value);
            ue_context_p->ue_context.Srb0.Srb_id = 0;
            ue_context_p->ue_context.Srb0.Active = 1;
            memcpy(ue_context_p->ue_context.Srb0.Rx_buffer.Payload,
                    buffer,
                    buffer_length);
            ue_context_p->ue_context.Srb0.Rx_buffer.payload_size = buffer_length;
          } else if (NR_InitialUE_Identity_PR_ng_5G_S_TMSI_Part1 == rrcSetupRequest->ue_Identity.present) {
            /* TODO */
            /* <5G-S-TMSI> = <AMF Set ID><AMF Pointer><5G-TMSI> 48-bit */
            /* ng-5G-S-TMSI-Part1                  BIT STRING (SIZE (39)) */
            if (rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1.size != 5) {
              LOG_E(NR_RRC, "wrong ng_5G_S_TMSI_Part1 size, expected 5, provided %lu \n",
                          (long unsigned int)rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1.size);
              return -1;
            }

            uint64_t s_tmsi_part1 = bitStr_to_uint64(&rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1);

            // memcpy(((uint8_t *) & random_value) + 3,
            //         rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1.buf,
            //         rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1.size);

            if ((ue_context_p = rrc_gNB_ue_context_5g_s_tmsi_exist(RC.nrrrc[ctxt_pP->module_id], s_tmsi_part1))) {
              LOG_I(NR_RRC, " 5G-S-TMSI-Part1 exists, ue_context_p %p, old rnti %x => %x\n",ue_context_p, ue_context_p->ue_context.rnti, ctxt_pP->rnti);

              nr_rrc_mac_remove_ue(ctxt_pP->module_id, ue_context_p->ue_context.rnti);

              /* replace rnti in the context */
              /* for that, remove the context from the RB tree */
              RB_REMOVE(rrc_nr_ue_tree_s, &RC.nrrrc[ctxt_pP->module_id]->rrc_ue_head, ue_context_p);
              /* and insert again, after changing rnti everywhere it has to be changed */
              ue_context_p->ue_id_rnti = ctxt_pP->rnti;
              ue_context_p->ue_context.rnti = ctxt_pP->rnti;
              RB_INSERT(rrc_nr_ue_tree_s, &RC.nrrrc[ctxt_pP->module_id]->rrc_ue_head, ue_context_p);
              /* reset timers */
              ue_context_p->ue_context.ul_failure_timer = 0;
              ue_context_p->ue_context.ue_release_timer = 0;
              ue_context_p->ue_context.ue_reestablishment_timer = 0;
              ue_context_p->ue_context.ue_release_timer_s1 = 0;
              ue_context_p->ue_context.ue_release_timer_rrc = 0;
            } else {
              LOG_I(NR_RRC, " 5G-S-TMSI-Part1 doesn't exist, setting ng_5G_S_TMSI_Part1 to %p => %ld\n",
                              ue_context_p, s_tmsi_part1);

              ue_context_p = rrc_gNB_get_next_free_ue_context(ctxt_pP, RC.nrrrc[ctxt_pP->module_id], s_tmsi_part1);

              if (ue_context_p == NULL) {
                  LOG_E(RRC, "%s:%d:%s: rrc_gNB_get_next_free_ue_context returned NULL\n", __FILE__, __LINE__, __FUNCTION__);
              }

              if (ue_context_p != NULL) {
                  ue_context_p->ue_context.Initialue_identity_5g_s_TMSI.presence = TRUE;
                  ue_context_p->ue_context.ng_5G_S_TMSI_Part1 = s_tmsi_part1;
              }
            }
          } else {
            /* TODO */
            memcpy(((uint8_t *) & random_value) + 3,
                    rrcSetupRequest->ue_Identity.choice.randomValue.buf,
                    rrcSetupRequest->ue_Identity.choice.randomValue.size);

            rrc_gNB_get_next_free_ue_context(ctxt_pP, RC.nrrrc[ctxt_pP->module_id], random_value);
            LOG_E(NR_RRC,
                    PROTOCOL_NR_RRC_CTXT_UE_FMT" RRCSetupRequest without random UE identity or S-TMSI not supported, let's reject the UE\n",
                    PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
            rrc_gNB_generate_RRCReject(ctxt_pP,
                                       rrc_gNB_get_ue_context(gnb_rrc_inst, ctxt_pP->rnti),
                                       CC_id);
            break;
          }
        }

        if (ue_context_p != NULL) {
          ue_context_p->ue_context.establishment_cause = rrcSetupRequest->establishmentCause;
        }

        rrc_gNB_generate_RRCSetup(ctxt_pP,
                                  rrc_gNB_get_ue_context(gnb_rrc_inst, ctxt_pP->rnti),
                                  CC_id);
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcResumeRequest:
        LOG_I(NR_RRC, "receive rrcResumeRequest message \n");
        /* TODO */
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcReestablishmentRequest:
        LOG_I(NR_RRC, "receive rrcReestablishmentRequest message \n");
        /* TODO */
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcSystemInfoRequest:
        LOG_I(NR_RRC, "receive rrcSystemInfoRequest message \n");
        /* TODO */
        break;

      default:
        LOG_E(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" Unknown message\n",
                   PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
        break;
    }
  }
  return 0;
}

/*! \fn uint64_t bitStr_to_uint64(BIT_STRING_t *)
 *\brief  This function extract at most a 64 bits value from a BIT_STRING_t object, the exact bits number depend on the BIT_STRING_t contents.
 *\param[in] pointer to the BIT_STRING_t object.
 *\return the extracted value.
 */
static inline uint64_t bitStr_to_uint64(BIT_STRING_t *asn) {
  uint64_t result = 0;
  int index;
  int shift;

  DevCheck ((asn->size > 0) && (asn->size <= 8), asn->size, 0, 0);

  shift = ((asn->size - 1) * 8) - asn->bits_unused;
  for (index = 0; index < (asn->size - 1); index++) {
    result |= (uint64_t)asn->buf[index] << shift;
    shift -= 8;
  }

  result |= asn->buf[index] >> asn->bits_unused;

  return result;
}

//-----------------------------------------------------------------------------
int
rrc_gNB_decode_dcch(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const uint8_t    *const      Rx_sdu,
  const sdu_size_t             sdu_sizeP
)
//-----------------------------------------------------------------------------
{
  asn_dec_rval_t                      dec_rval;
  NR_UL_DCCH_Message_t                *ul_dcch_msg  = NULL;
  struct rrc_gNB_ue_context_s         *ue_context_p = NULL;
  // NR_RRCSetupComplete_t               *rrcSetupComplete = NULL;

  int i;

  if ((Srb_id != 1) && (Srb_id != 2)) {
    LOG_E(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" Received message on SRB%ld, should not have ...\n",
          PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
          Srb_id);
  } else {
    LOG_D(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" Received message on SRB%ld\n",
              PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
              Srb_id);
  }

  LOG_D(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" Decoding UL-DCCH Message\n",
                  PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
  dec_rval = uper_decode(
                  NULL,
                  &asn_DEF_NR_UL_DCCH_Message,
                  (void **)&ul_dcch_msg,
                  Rx_sdu,
                  sdu_sizeP,
                  0,
                  0);
  // xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);

  {
    for (i = 0; i < sdu_sizeP; i++) {
      LOG_T(NR_RRC, "%x.", Rx_sdu[i]);
    }

    LOG_T(NR_RRC, "\n");
  }

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" Failed to decode UL-DCCH (%zu bytes)\n",
        PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
        dec_rval.consumed);
    return -1;
  }

  ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[ctxt_pP->module_id],
                                          ctxt_pP->rnti);

  if (ul_dcch_msg->message.present == NR_UL_DCCH_MessageType_PR_c1) {
    switch (ul_dcch_msg->message.choice.c1->present) {
      case NR_UL_DCCH_MessageType__c1_PR_NOTHING:
        LOG_I(NR_RRC,
            PROTOCOL_NR_RRC_CTXT_FMT" Received PR_NOTHING on UL-DCCH-Message\n",
            PROTOCOL_NR_RRC_CTXT_ARGS(ctxt_pP));
        break;

      case NR_UL_DCCH_MessageType__c1_PR_rrcReconfigurationComplete:
        if(!ue_context_p) {
          LOG_I(NR_RRC, "Processing NR_RRCReconfigurationComplete UE %x, ue_context_p is NULL\n", ctxt_pP->rnti);
          break;
        }

        LOG_DUMPMSG(NR_RRC, DEBUG_RRC, (char *)(Rx_sdu), sdu_sizeP,
                    "[MSG] RRC Connection Reconfiguration Complete\n");
        MSC_LOG_RX_MESSAGE(
        MSC_RRC_GNB,
        MSC_RRC_UE,
        Rx_sdu,
        sdu_sizeP,
        MSC_AS_TIME_FMT" NR_RRCReconfigurationComplete UE %x size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->ue_context.rnti,
        sdu_sizeP);
        LOG_D(NR_RRC,
            PROTOCOL_NR_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
            "(RRCReconfigurationComplete) ---> RRC_gNB]\n",
            PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH,
            sdu_sizeP);

        if (ul_dcch_msg->message.choice.c1->present == NR_UL_DCCH_MessageType__c1_PR_rrcReconfigurationComplete) {
          if (ul_dcch_msg->message.choice.c1->choice.rrcReconfigurationComplete->criticalExtensions.present ==
            NR_RRCReconfigurationComplete__criticalExtensions_PR_rrcReconfigurationComplete)
            rrc_gNB_process_RRCReconfigurationComplete(
                ctxt_pP,
                ue_context_p,
                ul_dcch_msg->message.choice.c1->choice.rrcReconfigurationComplete->rrc_TransactionIdentifier);
        }

        rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(ctxt_pP, ue_context_p);

        break;

      case NR_UL_DCCH_MessageType__c1_PR_rrcSetupComplete:
        if(!ue_context_p) {
          LOG_I(NR_RRC, "Processing NR_RRCSetupComplete UE %x, ue_context_p is NULL\n", ctxt_pP->rnti);
          break;
        }

        LOG_DUMPMSG(NR_RRC, DEBUG_RRC,(char *)Rx_sdu,sdu_sizeP,
                    "[MSG] RRC SetupComplete\n");
        MSC_LOG_RX_MESSAGE(
                MSC_RRC_GNB,
                MSC_RRC_UE,
                Rx_sdu,
                sdu_sizeP,
                MSC_AS_TIME_FMT" NR_RRCSetupComplete UE %x size %u",
                MSC_AS_TIME_ARGS(ctxt_pP),
                ue_context_p->ue_context.rnti,
                sdu_sizeP);
        LOG_D(NR_RRC,
                PROTOCOL_NR_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
                "(RRCSetupComplete) ---> RRC_gNB\n",
                PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
                DCCH,
                sdu_sizeP);

        if (ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.present ==
              NR_RRCSetupComplete__criticalExtensions_PR_rrcSetupComplete) {
          if (ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.choice.
            rrcSetupComplete->ng_5G_S_TMSI_Value != NULL) {
            if (ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.choice.
            rrcSetupComplete->ng_5G_S_TMSI_Value->present == NR_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value_PR_ng_5G_S_TMSI_Part2) {
            // ng-5G-S-TMSI-Part2                  BIT STRING (SIZE (9))
              if (ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.choice.
                rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI_Part2.size != 2) {
                LOG_E(NR_RRC, "wrong ng_5G_S_TMSI_Part2 size, expected 2, provided %lu",
                            (long unsigned int)ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->
                            criticalExtensions.choice.rrcSetupComplete->
                            ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI_Part2.size);
                return -1;
              }

              if (ue_context_p->ue_context.ng_5G_S_TMSI_Part1 != 0) {
                ue_context_p->ue_context.ng_5G_S_TMSI_Part2 =
                                BIT_STRING_to_uint16(&ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->
                                    criticalExtensions.choice.rrcSetupComplete->
                                    ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI_Part2);
              }

            /* TODO */
            } else if (ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.choice.
              rrcSetupComplete->ng_5G_S_TMSI_Value->present == NR_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value_PR_ng_5G_S_TMSI) {
              // NG-5G-S-TMSI ::=                         BIT STRING (SIZE (48))
              if (ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.choice.
                rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.size != 6) {
                LOG_E(NR_RRC, "wrong ng_5G_S_TMSI size, expected 6, provided %lu",
                            (long unsigned int)ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->
                            criticalExtensions.choice.rrcSetupComplete->
                            ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.size);
                return -1;
              }

              uint64_t fiveg_s_TMSI = bitStr_to_uint64(&ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->
                  criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI);
              LOG_I(NR_RRC, "Received rrcSetupComplete, 5g_s_TMSI: 0x%lX, amf_set_id: 0x%lX(%ld), amf_pointer: 0x%lX(%ld), 5g TMSI: 0x%X \n",
                  fiveg_s_TMSI, fiveg_s_TMSI >> 38, fiveg_s_TMSI >> 38,
                  (fiveg_s_TMSI >> 32) & 0x3F, (fiveg_s_TMSI >> 32) & 0x3F,
                  (uint32_t)fiveg_s_TMSI);
              if (ue_context_p->ue_context.Initialue_identity_5g_s_TMSI.presence == TRUE) {
                  ue_context_p->ue_context.Initialue_identity_5g_s_TMSI.amf_set_id = fiveg_s_TMSI >> 38;
                  ue_context_p->ue_context.Initialue_identity_5g_s_TMSI.amf_pointer = (fiveg_s_TMSI >> 32) & 0x3F;
                  ue_context_p->ue_context.Initialue_identity_5g_s_TMSI.fiveg_tmsi = (uint32_t)fiveg_s_TMSI;
              }
            }
          }

          rrc_gNB_process_RRCSetupComplete(
                  ctxt_pP,
                  ue_context_p,
                  ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete->criticalExtensions.choice.rrcSetupComplete);
          LOG_I(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" UE State = RRC_CONNECTED \n",
              PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
        }

        ue_context_p->ue_context.ue_release_timer = 0;
        break;
      case NR_UL_DCCH_MessageType__c1_PR_securityModeComplete:
        // to avoid segmentation fault
        if(!ue_context_p) {
          LOG_I(NR_RRC, "Processing securityModeComplete UE %x, ue_context_p is NULL\n", ctxt_pP->rnti);
          break;
        }

        LOG_I(NR_RRC,
              PROTOCOL_NR_RRC_CTXT_UE_FMT" received securityModeComplete on UL-DCCH %d from UE\n",
              PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
              DCCH);
        LOG_D(NR_RRC,
              PROTOCOL_NR_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
              "(securityModeComplete) ---> RRC_eNB\n",
              PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
              DCCH,
              sdu_sizeP);

        if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
          xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)ul_dcch_msg);
        }

        rrc_gNB_generate_UECapabilityEnquiry(ctxt_pP, ue_context_p);
        //rrc_gNB_generate_defaultRRCReconfiguration(ctxt_pP, ue_context_p);
        break;

      case NR_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation:
        if(!ue_context_p) {
          LOG_I(NR_RRC, "Processing ueCapabilityInformation UE %x, ue_context_p is NULL\n", ctxt_pP->rnti);
          break;
        }

        LOG_DUMPMSG(NR_RRC,DEBUG_RRC,(char *)Rx_sdu,sdu_sizeP,
                "[MSG] NR_RRC UECapablility Information\n");
            MSC_LOG_RX_MESSAGE(
            MSC_RRC_GNB,
            MSC_RRC_UE,
            Rx_sdu,
            sdu_sizeP,
            MSC_AS_TIME_FMT" ueCapabilityInformation UE %x size %u",
            MSC_AS_TIME_ARGS(ctxt_pP),
            ue_context_p->ue_context.rnti,
            sdu_sizeP);
        LOG_I(NR_RRC,
            PROTOCOL_NR_RRC_CTXT_UE_FMT" received ueCapabilityInformation on UL-DCCH %d from UE\n",
            PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
            DCCH);
        LOG_D(RRC,
        PROTOCOL_NR_RRC_CTXT_UE_FMT" RLC RB %02d --- RLC_DATA_IND %d bytes "
        "(UECapabilityInformation) ---> RRC_eNB\n",
        PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
        DCCH,
        sdu_sizeP);
        if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
            xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)ul_dcch_msg);
        }
        LOG_I(NR_RRC, "got UE capabilities for UE %x\n", ctxt_pP->rnti);
        int eutra_index = -1;

        if( ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.present ==
        NR_UECapabilityInformation__criticalExtensions_PR_ueCapabilityInformation ) {
          for(i = 0;i < ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.count; i++){
            if(ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->rat_Type ==
              NR_RAT_Type_nr){
              if(ue_context_p->ue_context.UE_Capability_nr){
                ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability,ue_context_p->ue_context.UE_Capability_nr);
                ue_context_p->ue_context.UE_Capability_nr = 0;
              }

              dec_rval = uper_decode(NULL,
                                      &asn_DEF_NR_UE_NR_Capability,
                                      (void**)&ue_context_p->ue_context.UE_Capability_nr,
                                      ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container.buf,
                                      ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container.size,
                                      0,0);
              if(LOG_DEBUGFLAG(DEBUG_ASN1)){
                xer_fprint(stdout,&asn_DEF_NR_UE_NR_Capability,ue_context_p->ue_context.UE_Capability_nr);
              }

              if((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)){
                LOG_E(NR_RRC,PROTOCOL_NR_RRC_CTXT_UE_FMT" Failed to decode nr UE capabilities (%zu bytes)\n",
                PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),dec_rval.consumed);
                ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability,ue_context_p->ue_context.UE_Capability_nr);
                ue_context_p->ue_context.UE_Capability_nr = 0;
              }

              ue_context_p->ue_context.UE_Capability_size =
              ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container.size;
              if(eutra_index != -1){
                LOG_E(NR_RRC,"fatal: more than 1 eutra capability\n");
                exit(1);
              }
              eutra_index = i;
            }

            if(ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->rat_Type ==
            NR_RAT_Type_eutra_nr){
            if(ue_context_p->ue_context.UE_Capability_MRDC){
              ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability,ue_context_p->ue_context.UE_Capability_MRDC);
              ue_context_p->ue_context.UE_Capability_MRDC = 0;
            }
            dec_rval = uper_decode(NULL,
                                    &asn_DEF_NR_UE_MRDC_Capability,
                                    (void**)&ue_context_p->ue_context.UE_Capability_MRDC,
                                    ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container.buf,
                                    ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container.size,
                                    0,0);

            if(LOG_DEBUGFLAG(DEBUG_ASN1)){
              xer_fprint(stdout,&asn_DEF_NR_UE_MRDC_Capability,ue_context_p->ue_context.UE_Capability_MRDC);
            }

            if((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)){
              LOG_E(NR_RRC,PROTOCOL_NR_RRC_CTXT_FMT" Failed to decode nr UE capabilities (%zu bytes)\n",
                  PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),dec_rval.consumed);
              ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability,ue_context_p->ue_context.UE_Capability_MRDC);
              ue_context_p->ue_context.UE_Capability_MRDC = 0;
            }
              ue_context_p->ue_context.UE_MRDC_Capability_size =
              ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container.size;
            }

            if(ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList->list.array[i]->rat_Type ==
            NR_RAT_Type_eutra){
              //TODO
            }
          }

          if(eutra_index == -1)
          break;
        }

        rrc_gNB_generate_defaultRRCReconfiguration(ctxt_pP, ue_context_p);
        break;

      default:
        break;
    }
  }
  return 0;
}

void rrc_gNB_process_release_request(const module_id_t gnb_mod_idP, x2ap_ENDC_sgnb_release_request_t *m)
{
  gNB_RRC_INST *rrc = RC.nrrrc[gnb_mod_idP];
  rrc_remove_nsa_user(rrc, m->rnti);
}

void rrc_gNB_process_dc_overall_timeout(const module_id_t gnb_mod_idP, x2ap_ENDC_dc_overall_timeout_t *m)
{
  gNB_RRC_INST *rrc = RC.nrrrc[gnb_mod_idP];
  rrc_remove_nsa_user(rrc, m->rnti);
}

void nr_rrc_subframe_process(protocol_ctxt_t *const ctxt_pP, const int CC_id) {
  MessageDef *msg;

  /* send a tick to x2ap */
  if (is_x2ap_enabled()){
    msg = itti_alloc_new_message(TASK_RRC_ENB, X2AP_SUBFRAME_PROCESS);
    itti_send_msg_to_task(TASK_X2AP, ctxt_pP->module_id, msg);
  }
}

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///
void *rrc_gnb_task(void *args_p) {
  MessageDef                         *msg_p;
  const char                         *msg_name_p;
  instance_t                         instance;
  int                                result;
  //SRB_INFO                           *srb_info_p;
  //int                                CC_id;
  protocol_ctxt_t                    ctxt;
  itti_mark_task_ready(TASK_RRC_GNB);
  LOG_I(NR_RRC,"Entering main loop of NR_RRC message task\n");

  while (1) {
    // Wait for a message
    itti_receive_msg(TASK_RRC_GNB, &msg_p);
    msg_name_p = ITTI_MSG_NAME(msg_p);
    instance = ITTI_MSG_INSTANCE(msg_p);

    /* RRC_SUBFRAME_PROCESS is sent every subframe, do not log it */
    if (ITTI_MSG_ID(msg_p) != RRC_SUBFRAME_PROCESS)
      LOG_I(NR_RRC,"Received message %s\n",msg_name_p);

    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(NR_RRC, " *** Exiting NR_RRC thread\n");
        itti_exit_task();
        break;

      case MESSAGE_TEST:
        LOG_I(NR_RRC, "[gNB %d] Received %s\n", instance, msg_name_p);
        break;

      case RRC_SUBFRAME_PROCESS:
        nr_rrc_subframe_process(&RRC_SUBFRAME_PROCESS(msg_p).ctxt, RRC_SUBFRAME_PROCESS(msg_p).CC_id);
        break;

      /* Messages from MAC */
      case NR_RRC_MAC_CCCH_DATA_IND:
      // PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt,
      //                               NR_RRC_MAC_CCCH_DATA_IND(msg_p).gnb_index,
      //                               GNB_FLAG_YES,
      //                               NR_RRC_MAC_CCCH_DATA_IND(msg_p).rnti,
      //                               msg_p->ittiMsgHeader.lte_time.frame,
      //                               msg_p->ittiMsgHeader.lte_time.slot);
        LOG_I(NR_RRC,"Decoding CCCH : inst %d, CC_id %d, ctxt %p, sib_info_p->Rx_buffer.payload_size %d\n",
                instance,
                NR_RRC_MAC_CCCH_DATA_IND(msg_p).CC_id,
                &ctxt,
                NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size);

        if (NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size >= CCCH_SDU_SIZE) {
          LOG_I(NR_RRC, "CCCH message has size %d > %d\n",
              NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size,CCCH_SDU_SIZE);
          break;
      }

      nr_rrc_gNB_decode_ccch(&ctxt,
                          (uint8_t *)NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu,
                          NR_RRC_MAC_CCCH_DATA_IND(msg_p).sdu_size,
                          NR_RRC_MAC_CCCH_DATA_IND(msg_p).CC_id);
      break;

      /* Messages from PDCP */
      case NR_RRC_DCCH_DATA_IND:
        // PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt,
        //                               instance,
        //                               ENB_FLAG_YES,
        //                               RRC_DCCH_DATA_IND(msg_p).rnti,
        //                               msg_p->ittiMsgHeader.lte_time.frame,
        //                               msg_p->ittiMsgHeader.lte_time.slot);
        LOG_D(NR_RRC, PROTOCOL_NR_RRC_CTXT_UE_FMT" Received on DCCH %d %s\n",
                PROTOCOL_NR_RRC_CTXT_UE_ARGS(&ctxt),
                NR_RRC_DCCH_DATA_IND(msg_p).dcch_index,
                msg_name_p);
        rrc_gNB_decode_dcch(&ctxt,
                            NR_RRC_DCCH_DATA_IND(msg_p).dcch_index,
                            NR_RRC_DCCH_DATA_IND(msg_p).sdu_p,
                            NR_RRC_DCCH_DATA_IND(msg_p).sdu_size);
        break;

      /*
      #if defined(ENABLE_USE_MME)

            // Messages from S1AP
          case S1AP_DOWNLINK_NAS:
            rrc_eNB_process_S1AP_DOWNLINK_NAS(msg_p, msg_name_p, instance, &rrc_gNB_mui);
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
            ///Nothing to do. Apparently everything is done in S1AP processing
            //LOG_I(RRC, "[eNB %d] Received message %s, not processed because procedure not synched\n",
            //instance, msg_name_p);
            if (rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)
                && rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_rrc > 0) {
              rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_rrc =
              rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_thres_rrc;
            }
            break;

      #endif
      */
      /* Messages from gNB app */
      case NRRRC_CONFIGURATION_REQ:
        LOG_I(NR_RRC, "[gNB %d] Received %s : %p\n", instance, msg_name_p,&NRRRC_CONFIGURATION_REQ(msg_p));
        openair_rrc_gNB_configuration(GNB_INSTANCE_TO_MODULE_ID(instance), &NRRRC_CONFIGURATION_REQ(msg_p));
        break;

      /* Messages from X2AP */
      case X2AP_ENDC_SGNB_ADDITION_REQ:
        LOG_I(NR_RRC, "Received ENDC sgNB addition request from X2AP \n");
        rrc_gNB_process_AdditionRequestInformation(GNB_INSTANCE_TO_MODULE_ID(instance), &X2AP_ENDC_SGNB_ADDITION_REQ(msg_p));
        break;

      case X2AP_ENDC_SGNB_RECONF_COMPLETE:
        LOG_I(NR_RRC, "Handling of reconfiguration complete message at RRC gNB is pending \n");
        break;

      case NGAP_INITIAL_CONTEXT_SETUP_REQ:
        rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p, msg_name_p, instance);

      case X2AP_ENDC_SGNB_RELEASE_REQUEST:
        LOG_I(NR_RRC, "Received ENDC sgNB release request from X2AP \n");
        rrc_gNB_process_release_request(GNB_INSTANCE_TO_MODULE_ID(instance), &X2AP_ENDC_SGNB_RELEASE_REQUEST(msg_p));
        break;

      case X2AP_ENDC_DC_OVERALL_TIMEOUT:
        rrc_gNB_process_dc_overall_timeout(GNB_INSTANCE_TO_MODULE_ID(instance), &X2AP_ENDC_DC_OVERALL_TIMEOUT(msg_p));
        break;

      /* Messages from GTP */
      case GTPV1U_ENB_DELETE_TUNNEL_RESP:
        /* nothing to do? */
        break;

      default:
        LOG_E(NR_RRC, "[gNB %d] Received unexpected message %s\n", instance, msg_name_p);
        break;
    }

    result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}

//-----------------------------------------------------------------------------
void
rrc_gNB_generate_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t          *const ue_context_pP
)
//-----------------------------------------------------------------------------
{
  uint8_t                             buffer[100];
  uint8_t                             size;
  T(T_ENB_RRC_SECURITY_MODE_COMMAND, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));
  NR_IntegrityProtAlgorithm_t integrity_algorithm = (NR_IntegrityProtAlgorithm_t)ue_context_pP->ue_context.integrity_algorithm;
  size = do_NR_SecurityModeCommand(
           ctxt_pP,
           buffer,
           rrc_gNB_get_next_transaction_identifier(ctxt_pP->module_id),
           ue_context_pP->ue_context.ciphering_algorithm,
           &integrity_algorithm);
  LOG_DUMPMSG(NR_RRC,DEBUG_RRC,(char *)buffer,size,"[MSG] RRC Security Mode Command\n");
  LOG_I(NR_RRC,
        PROTOCOL_NR_RRC_CTXT_UE_FMT" Logical Channel DL-DCCH, Generate SecurityModeCommand (bytes %d)\n",
        PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
        size);
  LOG_D(NR_RRC,
        PROTOCOL_NR_RRC_CTXT_UE_FMT" --- PDCP_DATA_REQ/%d Bytes (securityModeCommand to UE MUI %d) --->[PDCP][RB %02d]\n",
        PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
        size,
        rrc_gNB_mui,
        DCCH);
  MSC_LOG_TX_MESSAGE(
    MSC_RRC_GNB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" securityModeCommand UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_gNB_mui,
    size);

  LOG_I(NR_RRC,"calling rrc_data_req :securityModeCommand\n");
#ifdef ITTI_SIM
  MessageDef *message_p;
  uint8_t *message_buffer;
  message_buffer = itti_malloc (TASK_RRC_GNB_SIM, TASK_RRC_UE_SIM,size);
  memcpy (message_buffer, buffer, size);
  message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, GNB_RRC_DCCH_DATA_IND);
  GNB_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
  GNB_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
  GNB_RRC_DCCH_DATA_IND (message_p).size	= size;
  itti_send_msg_to_task (TASK_RRC_UE_SIM, ctxt_pP->instance, message_p);
#else
  nr_rrc_data_req(ctxt_pP,
               DCCH,
               rrc_gNB_mui++,
               SDU_CONFIRM_NO,
               size,
               buffer,
               PDCP_TRANSMISSION_MODE_CONTROL);
#endif

}

void
rrc_gNB_generate_UECapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t          *const ue_context_pP
)
//-----------------------------------------------------------------------------
{
  uint8_t                             buffer[100];
  uint8_t                             size;
  T(T_ENB_RRC_UE_CAPABILITY_ENQUIRY, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->frame),
    T_INT(ctxt_pP->subframe), T_INT(ctxt_pP->rnti));
  size = do_NR_SA_UECapabilityEnquiry(
           ctxt_pP,
           buffer,
           rrc_gNB_get_next_transaction_identifier(ctxt_pP->module_id));
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel DL-DCCH, Generate NR UECapabilityEnquiry (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size);
  LOG_D(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" --- PDCP_DATA_REQ/%d Bytes (NR UECapabilityEnquiry MUI %d) --->[PDCP][RB %02d]\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size,
        rrc_gNB_mui,
        DCCH);
  MSC_LOG_TX_MESSAGE(
    MSC_RRC_GNB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" rrcNRUECapabilityEnquiry UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_gNB_mui,
    size);
#ifdef ITTI_SIM
  MessageDef *message_p;
  uint8_t *message_buffer;
  message_buffer = itti_malloc (TASK_RRC_GNB_SIM, TASK_RRC_UE_SIM, size);
  memcpy (message_buffer, buffer, size);
  message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, GNB_RRC_DCCH_DATA_IND);
  GNB_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
  GNB_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
  GNB_RRC_DCCH_DATA_IND (message_p).size  = size;
  itti_send_msg_to_task (TASK_RRC_UE_SIM, ctxt_pP->instance, message_p);
#else
  nr_rrc_data_req(
    ctxt_pP,
    DCCH,
    rrc_gNB_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);
#endif
}

//-----------------------------------------------------------------------------
/*
* Generate the RRC Connection Release to UE.
* If received, UE should switch to RRC_IDLE mode.
*/
void
rrc_gNB_generate_RRCConnectionRelease(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t *const ue_context_pP
)
//-----------------------------------------------------------------------------
{
  uint8_t buffer[RRC_BUF_SIZE];
  uint16_t size = 0;

  memset(buffer, 0, RRC_BUF_SIZE);

  size = do_NR_RRCConnectionRelease(buffer,rrc_gNB_get_next_transaction_identifier(ctxt_pP->module_id));
  ue_context_pP->ue_context.ue_reestablishment_timer = 0;
  ue_context_pP->ue_context.ue_release_timer = 0;
  LOG_I(NR_RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel DL-DCCH, Generate RRCConnectionRelease (bytes %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size);
  LOG_D(NR_RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" --- PDCP_DATA_REQ/%d Bytes (rrcConnectionRelease MUI %d) --->[PDCP][RB %u]\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        size,
        rrc_gNB_mui,
        DCCH);
  MSC_LOG_TX_MESSAGE(
    MSC_RRC_GNB,
    MSC_RRC_UE,
    buffer,
    size,
    MSC_AS_TIME_FMT" LTE_RRCConnectionRelease UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_context.rnti,
    rrc_gNB_mui,
    size);

  if (NODE_IS_CU(RC.rrc[ctxt_pP->module_id]->node_type)) {
    MessageDef *m = itti_alloc_new_message(TASK_RRC_ENB, F1AP_UE_CONTEXT_RELEASE_CMD);
    F1AP_UE_CONTEXT_RELEASE_CMD(m).rnti = ctxt_pP->rnti;
    F1AP_UE_CONTEXT_RELEASE_CMD(m).cause = F1AP_CAUSE_RADIO_NETWORK;
    F1AP_UE_CONTEXT_RELEASE_CMD(m).cause_value = 10; // 10 = F1AP_CauseRadioNetwork_normal_release
    F1AP_UE_CONTEXT_RELEASE_CMD(m).rrc_container = buffer;
    F1AP_UE_CONTEXT_RELEASE_CMD(m).rrc_container_length = size;
    itti_send_msg_to_task(TASK_CU_F1, ctxt_pP->module_id, m);
  } else {
    nr_rrc_data_req(ctxt_pP,
                 DCCH,
                 rrc_gNB_mui++,
                 SDU_CONFIRM_NO,
                 size,
                 buffer,
                 PDCP_TRANSMISSION_MODE_CONTROL);
  }
}
void nr_rrc_trigger(protocol_ctxt_t *ctxt, int CC_id, int frame, int subframe)
{
  MessageDef *message_p;
  message_p = itti_alloc_new_message(TASK_RRC_GNB, RRC_SUBFRAME_PROCESS);
  RRC_SUBFRAME_PROCESS(message_p).ctxt  = *ctxt;
  RRC_SUBFRAME_PROCESS(message_p).CC_id = CC_id;
  itti_send_msg_to_task(TASK_RRC_GNB, ctxt->module_id, message_p);
}
