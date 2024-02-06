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

/*! \file nr_rrc_proto.h
 * \brief RRC functions prototypes for gNB
 * \author Navid Nikaein and Raymond Knopp, WEI-TAI-CHEN
 * \date 2010 - 2014, 2018
 * \email navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 */
/** \addtogroup _rrc
 *  @{
 */

#ifndef __NR_RRC_PROTO_H__
#define __NR_RRC_PROTO_H__

#include "RRC/NR/nr_rrc_defs.h"
#include "NR_RRCReconfiguration.h"
#include "NR_UE-NR-Capability.h"
#include "NR_UE-CapabilityRAT-ContainerList.h"
#include "LTE_UE-CapabilityRAT-ContainerList.h"
#include "NR_CG-Config.h"
#include "NR_CG-ConfigInfo.h"
#include "NR_SecurityConfig.h"
#include "NR_CellGroupConfig.h"

#define NR_MAX_SUPPORTED_DL_LAYERS 2

void rrc_gNB_process_SgNBAdditionRequest( 
     const protocol_ctxt_t  *const ctxt_pP,
     rrc_gNB_ue_context_t   *ue_context_pP 
     );

void rrc_gNB_generate_SgNBAdditionRequestAcknowledge( 
     const protocol_ctxt_t  *const ctxt_pP,
     rrc_gNB_ue_context_t   *const ue_context_pP
     );

rrc_gNB_ue_context_t *rrc_gNB_allocate_new_UE_context(gNB_RRC_INST *rrc_instance_pP);

void rrc_parse_ue_capabilities(gNB_RRC_INST *rrc,NR_UE_CapabilityRAT_ContainerList_t *UE_CapabilityRAT_ContainerList, x2ap_ENDC_sgnb_addition_req_t *m, NR_CG_ConfigInfo_IEs_t * cg_config_info);

void rrc_add_nsa_user(gNB_RRC_INST *rrc, rrc_gNB_ue_context_t *ue_context_p, x2ap_ENDC_sgnb_addition_req_t *m);

void rrc_remove_nsa_user(gNB_RRC_INST *rrc, int rnti);

void fill_default_reconfig(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                           NR_ServingCellConfig_t *servingcellconfigdedicated,
                           NR_RRCReconfiguration_IEs_t *reconfig,
                           NR_CellGroupConfig_t *secondaryCellGroup,
                           NR_UE_NR_Capability_t *uecap,
                           int uid);

int generate_CG_Config(gNB_RRC_INST *rrc, 
		       NR_CG_Config_t *cg_Config,
		       NR_RRCReconfiguration_t *reconfig,
		       NR_RadioBearerConfig_t *rbconfig);

int parse_CG_ConfigInfo(gNB_RRC_INST *rrc, NR_CG_ConfigInfo_t *CG_ConfigInfo, x2ap_ENDC_sgnb_addition_req_t *m);

void
rrc_gNB_generate_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t          *const ue_context_pP,
  int n_drbs,
  const f1ap_drb_to_be_setup_t *drbs
);

unsigned int rrc_gNB_get_next_transaction_identifier(module_id_t gnb_mod_idP);

void
rrc_gNB_generate_UECapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t  *const ue_context_pP
);

void
rrc_gNB_generate_RRCRelease(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t  *const ue_context_pP
);

/**\brief RRC eNB task.
   \param args_p Pointer on arguments to start the task. */
void *rrc_gnb_task(void *args_p);

/**\ Function to set or overwrite PTRS DL RRC parameters.
   \ *bwp Pointer to dedicated RC config structure
   \ *ptrsNrb Pointer to K_ptrs N_RB related parameters
   \ *ptrsMcs Pointer to L_ptrs MCS related parameters
   \ *epre_Ratio Pointer to ep_ratio
   \ *reOffset Pointer to RE Offset Value */
void rrc_config_dl_ptrs_params(NR_BWP_Downlink_t *bwp, long *ptrsNrb, long *ptrsMcs, long *epre_Ratio, long *reOffset);

int nr_rrc_reconfiguration_req(rrc_gNB_ue_context_t         *const ue_context_pP,
                               protocol_ctxt_t              *const ctxt_pP,
                               const int                    dl_bwp_id,
                               const int                    ul_bwp_id);

void
rrc_gNB_generate_dedicatedRRCReconfiguration_release(
    const protocol_ctxt_t   *const ctxt_pP,
    rrc_gNB_ue_context_t    *const ue_context_pP,
    uint8_t                  xid,
    uint32_t                 nas_length,
    uint8_t                 *nas_buffer);

void rrc_gNB_generate_dedicatedRRCReconfiguration(const protocol_ctxt_t *const ctxt_pP, rrc_gNB_ue_context_t *ue_context_pP);

bool ue_associated_to_cuup(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue);
sctp_assoc_t get_existing_cuup_for_ue(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue);
sctp_assoc_t get_new_cuup_for_ue(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue, int sst, int sd);
int rrc_gNB_process_e1_setup_req(sctp_assoc_t assoc_id, e1ap_setup_req_t *req);

/* Process indication of E1 connection loss on CU-CP */
void rrc_gNB_process_e1_lost_connection(gNB_RRC_INST *rrc, e1ap_lost_connection_t *lc, sctp_assoc_t assoc_id);

void bearer_context_setup_direct(e1ap_bearer_setup_req_t *req,
                                 instance_t instance);

void bearer_context_setup_e1ap(e1ap_bearer_setup_req_t *req,
                                 instance_t instance);

void ue_cxt_mod_send_e1ap(MessageDef *msg,
                          instance_t instance);

void ue_cxt_mod_direct(MessageDef *msg,
                       instance_t instance);

void prepare_and_send_ue_context_modification_f1(rrc_gNB_ue_context_t *ue_context_p,
                                                 e1ap_bearer_setup_resp_t *e1ap_resp);
void nr_pdcp_add_srbs(eNB_flag_t enb_flag,
                      ue_id_t UEid,
                      NR_SRB_ToAddModList_t *const srb2add_list,
                      const uint8_t security_modeP,
                      uint8_t *const kRRCenc,
                      uint8_t *const kUPint);

void nr_pdcp_add_drbs(eNB_flag_t enb_flag,
                      ue_id_t rntiMaybeUEid,
                      NR_DRB_ToAddModList_t *const drb2add_list,
                      const uint8_t security_modeP,
                      uint8_t *const kUPenc,
                      uint8_t *const kUPint);

int rrc_gNB_generate_pcch_msg(sctp_assoc_t assoc_id, const NR_SIB1_t *sib, uint32_t tmsi, uint8_t paging_drx);

void nr_rrc_transfer_protected_rrc_message(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue_p, uint8_t srb_id, const uint8_t* buffer, int size);
/** @}*/
#endif
