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

#include "RRC/NR/nr_rrc_defs.h"
#include "flexran_agent_extern.h"
#include "NR_RRCReconfiguration.h"
#include "NR_UE-NR-Capability.h"
#include "NR_UE-CapabilityRAT-ContainerList.h"
#include "LTE_UE-CapabilityRAT-ContainerList.h"
#include "NR_CG-Config.h"
#include "NR_CG-ConfigInfo.h"
#include "NR_SecurityConfig.h"

int rrc_init_nr_global_param(void);

void rrc_config_nr_buffer(NR_SRB_INFO* Srb_info,
                          uint8_t Lchan_type,
                          uint8_t Role);

int8_t mac_rrc_nr_data_req(const module_id_t Mod_idP,
                           const int         CC_id,
                           const frame_t     frameP,
                           const rb_id_t     Srb_id,
                           const rnti_t      rnti,
                           const uint8_t     Nb_tb,
                           uint8_t *const    buffer_pP );

void rrc_gNB_process_SgNBAdditionRequest( 
     const protocol_ctxt_t  *const ctxt_pP,
     rrc_gNB_ue_context_t   *ue_context_pP 
     );

void rrc_gNB_generate_SgNBAdditionRequestAcknowledge( 
     const protocol_ctxt_t  *const ctxt_pP,
     rrc_gNB_ue_context_t   *const ue_context_pP
     );

struct rrc_gNB_ue_context_s *rrc_gNB_allocate_new_UE_context(gNB_RRC_INST *rrc_instance_pP);

void rrc_parse_ue_capabilities(gNB_RRC_INST *rrc,NR_UE_CapabilityRAT_ContainerList_t *UE_CapabilityRAT_ContainerList, x2ap_ENDC_sgnb_addition_req_t *m, NR_CG_ConfigInfo_IEs_t * cg_config_info);

void rrc_add_nsa_user(gNB_RRC_INST *rrc,struct rrc_gNB_ue_context_s *ue_context_p, x2ap_ENDC_sgnb_addition_req_t *m);

void rrc_remove_nsa_user(gNB_RRC_INST *rrc, int rnti);

void fill_default_secondaryCellGroup(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                                     NR_ServingCellConfig_t *servingcellconfigdedicated,
                                     NR_CellGroupConfig_t *secondaryCellGroup,
                                     int scg_id,
                                     int servCellIndex,
                                     int dl_antenna_ports,
                                     int do_csirs,
                                     int uid);

void config_csirs(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                  NR_CSI_MeasConfig_t *csi_MeasConfig,
                  int dl_antenna_ports,
                  int curr_bwp,
                  int do_csirs);

void fill_default_reconfig(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                           NR_ServingCellConfig_t *servingcellconfigdedicated,
                           NR_RRCReconfiguration_IEs_t *reconfig,
                           NR_CellGroupConfig_t *secondaryCellGroup,
                           int dl_antenna_ports,
                           int do_csirs,
                           int uid);

void fill_default_rbconfig(NR_RadioBearerConfig_t *rbconfig,
                           int eps_bearer_id, int rb_id,
                           e_NR_CipheringAlgorithm ciphering_algorithm,
                           e_NR_SecurityConfig__keyToUse key_to_use);

int generate_CG_Config(gNB_RRC_INST *rrc, 
		       NR_CG_Config_t *cg_Config,
		       NR_RRCReconfiguration_t *reconfig,
		       NR_RadioBearerConfig_t *rbconfig);

void apply_macrlc_config(gNB_RRC_INST *rrc,
                         rrc_gNB_ue_context_t         *const ue_context_pP,
                         const protocol_ctxt_t        *const ctxt_pP );

void apply_pdcp_config(rrc_gNB_ue_context_t         *const ue_context_pP,
                       const protocol_ctxt_t        *const ctxt_pP );

void
rrc_gNB_generate_RRCSetup(
    const protocol_ctxt_t        *const ctxt_pP,
    rrc_gNB_ue_context_t         *const ue_context_pP,
    OCTET_STRING_t               *masterCellGroup_from_DU,
    NR_ServingCellConfigCommon_t *scc,
    const int                    CC_id);

int parse_CG_ConfigInfo(gNB_RRC_INST *rrc, NR_CG_ConfigInfo_t *CG_ConfigInfo, x2ap_ENDC_sgnb_addition_req_t *m);

void
rrc_gNB_generate_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t          *const ue_context_pP
);

uint8_t
rrc_gNB_get_next_transaction_identifier(
    module_id_t gnb_mod_idP
);

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
   \param void *args_p Pointer on arguments to start the task. */
void *rrc_gnb_task(void *args_p);

/* Trigger RRC periodic processing. To be called once per ms. */
void nr_rrc_trigger(protocol_ctxt_t *ctxt, int CC_id, int frame, int subframe);

/**\ Function to set or overwrite PTRS DL RRC parameters.
   \ *bwp Pointer to dedicated RC config structure
   \ *ptrsNrb Pointer to K_ptrs N_RB related parameters
   \ *ptrsMcs Pointer to L_ptrs MCS related parameters
   \ *epre_Ratio Pointer to ep_ratio
   \ *reOffset Pointer to RE Offset Value */
void rrc_config_dl_ptrs_params(NR_BWP_Downlink_t *bwp, int *ptrsNrb, int *ptrsMcs, int *epre_Ratio, int * reOffset);

uint8_t
nr_rrc_data_req(
  const protocol_ctxt_t   *const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_size,
  uint8_t                 *const buffer_pP,
  const pdcp_transmission_mode_t modeP
);

int
nr_rrc_mac_remove_ue(module_id_t mod_idP,
                  rnti_t rntiP);

int8_t nr_mac_rrc_data_ind(
    const module_id_t     module_idP,
    const int             CC_id,
    const frame_t         frameP,
    const sub_frame_t     sub_frameP,
    const int             UE_id,
    const rnti_t          rntiP,
    const rb_id_t         srb_idP,
    const uint8_t        *sduP,
    const sdu_size_t      sdu_lenP,
    const boolean_t   brOption
);

int nr_rrc_gNB_decode_ccch(protocol_ctxt_t    *const ctxt_pP,
                           const uint8_t      *buffer,
                           int                buffer_length,
                           OCTET_STRING_t     *du_to_cu_rrc_container,
                           const int          CC_id);

void
rrc_gNB_generate_dedicatedRRCReconfiguration_release(
    const protocol_ctxt_t   *const ctxt_pP,
    rrc_gNB_ue_context_t    *const ue_context_pP,
    uint8_t                  xid,
    uint32_t                 nas_length,
    uint8_t                 *nas_buffer);

void 
rrc_gNB_generate_dedicatedRRCReconfiguration(
    const protocol_ctxt_t     *const ctxt_pP,
    rrc_gNB_ue_context_t      *ue_context_pP);
