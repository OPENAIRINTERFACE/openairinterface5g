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

/*! \file rrc_gNB_nsa.c
 * \brief rrc NSA procedures for gNB
 * \author Raymond Knopp
 * \date 2019
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr
 */

#include "nr_rrc_defs.h"
#include "NR_RRCReconfiguration.h"
#include "NR_UE-NR-Capability.h"
//#include "NR_UE-CapabilityRAT-ContainerList.h"
#include "LTE_UE-CapabilityRAT-ContainerList.h"
#include "NR_CellGroupConfig.h"
#include "NR_CG-Config.h"
//#include "NR_SRB-ToAddModList.h"
#include "uper_encoder.h"
#include "uper_decoder.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/RRC/LTE/rrc_eNB_GTPV1U.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "executables/softmodem-common.h"
#include "executables/nr-softmodem.h"
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "openair3/SECU/secu_defs.h"
#include "openair3/SECU/key_nas_deriver.h"

#include <openair2/RRC/NR/nr_rrc_proto.h>
#include "nr_pdcp/nr_pdcp_oai_api.h"
#include "MESSAGES/asn1_msg.h"

void rrc_parse_ue_capabilities(gNB_RRC_INST *rrc, NR_UE_CapabilityRAT_ContainerList_t *UE_CapabilityRAT_ContainerList, x2ap_ENDC_sgnb_addition_req_t *m, NR_CG_ConfigInfo_IEs_t *cg_config_info)
{
  OCTET_STRING_t *ueCapabilityRAT_Container_nr=NULL;
  OCTET_STRING_t *ueCapabilityRAT_Container_MRDC=NULL;
  asn_dec_rval_t dec_rval;
  int list_size=0;

  AssertFatal(UE_CapabilityRAT_ContainerList!=NULL,"UE_CapabilityRAT_ContainerList is null\n");
  AssertFatal((list_size=UE_CapabilityRAT_ContainerList->list.count) >= 2, "UE_CapabilityRAT_ContainerList->list.size %d < 2\n",UE_CapabilityRAT_ContainerList->list.count);

  for (int i=0; i<list_size; i++) {
    if (UE_CapabilityRAT_ContainerList->list.array[i]->rat_Type == NR_RAT_Type_nr) ueCapabilityRAT_Container_nr = &UE_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container;
    else if (UE_CapabilityRAT_ContainerList->list.array[i]->rat_Type == NR_RAT_Type_eutra_nr) ueCapabilityRAT_Container_MRDC = &UE_CapabilityRAT_ContainerList->list.array[i]->ue_CapabilityRAT_Container;
  }

  // decode and store capabilities
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_allocate_new_ue_context(rrc);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  if (ueCapabilityRAT_Container_nr != NULL) {
    dec_rval = uper_decode(NULL, &asn_DEF_NR_UE_NR_Capability, (void **)&UE->UE_Capability_nr, ueCapabilityRAT_Container_nr->buf, ueCapabilityRAT_Container_nr->size, 0, 0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
      LOG_E(RRC, "Failed to decode UE NR capabilities (%zu bytes) container size %lu\n", dec_rval.consumed,ueCapabilityRAT_Container_nr->size);
      ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, UE->UE_Capability_nr);
      UE->UE_Capability_nr = 0;
      AssertFatal(1==0,"exiting\n");
    }
  }

  if (ueCapabilityRAT_Container_MRDC != NULL) {
    dec_rval = uper_decode(NULL, &asn_DEF_NR_UE_MRDC_Capability, (void **)&UE->UE_Capability_MRDC, ueCapabilityRAT_Container_MRDC->buf, ueCapabilityRAT_Container_MRDC->size, 0, 0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
      LOG_E(RRC, "Failed to decode UE MRDC capabilities (%zu bytes)\n", dec_rval.consumed);
      ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability, UE->UE_Capability_MRDC);
      UE->UE_Capability_MRDC = 0;
      AssertFatal(1==0,"exiting\n");
    }
  }

  // dump ue_capabilities

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) && ueCapabilityRAT_Container_nr != NULL ) {
    xer_fprint(stdout, &asn_DEF_NR_UE_NR_Capability, UE->UE_Capability_nr);
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) && ueCapabilityRAT_Container_MRDC != NULL ) {
    xer_fprint(stdout, &asn_DEF_NR_UE_MRDC_Capability, UE->UE_Capability_MRDC);
  }
  LOG_A(NR_RRC, "Successfully decoded UE NR capabilities (NR and MRDC)\n");

  AssertFatal(NODE_IS_MONOLITHIC(rrc->node_type), "phy_test and do_ra only work in monolithic\n");
  UE->spCellConfig = calloc(1, sizeof(struct NR_SpCellConfig));
  UE->spCellConfig->spCellConfigDedicated = RC.nrmac[0]->common_channels[0].pre_ServingCellConfig;
  LOG_I(NR_RRC,"Adding new NSA user (%p)\n",ue_context_p);
  rrc_add_nsa_user(rrc,ue_context_p, m);
}

/* generate prototypes for the tree management functions (RB_INSERT used in rrc_add_nsa_user) */
RB_PROTOTYPE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries,
             rrc_gNB_compare_ue_rnti_id);

void rrc_add_nsa_user(gNB_RRC_INST *rrc, rrc_gNB_ue_context_t *ue_context_p, x2ap_ENDC_sgnb_addition_req_t *m)
{
  AssertFatal(!get_softmodem_params()->sa, "%s() cannot be called in SA mode, it is intrinsically for NSA\n", __func__);
  // generate nr-Config-r15 containers for LTE RRC : inside message for X2 EN-DC (CG-Config Message from 38.331)
  const nr_mac_config_t *configuration = &RC.nrmac[0]->radio_config;
  MessageDef *msg;
  msg = itti_alloc_new_message(TASK_RRC_ENB, 0, X2AP_ENDC_SGNB_ADDITION_REQ_ACK);
  gtpv1u_enb_create_tunnel_req_t  create_tunnel_req;
  gtpv1u_enb_create_tunnel_resp_t create_tunnel_resp;
  protocol_ctxt_t ctxt={0};
  uint8_t kUPenc[16] = {0};
  uint8_t kUPint[16] = {0};
  int i;
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  // In case of phy-test and do-ra mode, read UE capabilities directly from file
  if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra == 1) {
    NR_UE_NR_Capability_t* UE_Capability_nr = NULL;
    char UE_NR_Capability_xer[65536];
    FILE *f = NULL;
    if (uecap_file)
      f = fopen(uecap_file, "r");
    if(f){
      size_t size = fread(UE_NR_Capability_xer, 1, sizeof UE_NR_Capability_xer, f);
      if (size == 0 || size == sizeof UE_NR_Capability_xer)
        LOG_E(NR_RRC,"UE Capabilities XER file %s is too large (%ld)\n", uecap_file, size);
      else {
        UE_Capability_nr = CALLOC(1,sizeof(NR_UE_NR_Capability_t));
        asn_dec_rval_t dec_rval = xer_decode(0, &asn_DEF_NR_UE_NR_Capability, (void *)&UE_Capability_nr, UE_NR_Capability_xer, size);
        assert(dec_rval.code == RC_OK);
        xer_fprint(stdout,&asn_DEF_NR_UE_NR_Capability,(void *)UE_Capability_nr);
      }
    }
    else
      LOG_E(NR_RRC,"Could not open UE Capabilities input file. Not handling OAI UE Capabilities.\n");
    UE->UE_Capability_nr = UE_Capability_nr;
  }

  // NR RRCReconfiguration
  UE->reconfig = calloc(1, sizeof(NR_RRCReconfiguration_t));
  memset((void *)UE->reconfig, 0, sizeof(NR_RRCReconfiguration_t));
  UE->reconfig->rrc_TransactionIdentifier = 0;
  UE->reconfig->criticalExtensions.present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
  NR_RRCReconfiguration_IEs_t *reconfig_ies=calloc(1,sizeof(NR_RRCReconfiguration_IEs_t));
  UE->reconfig->criticalExtensions.choice.rrcReconfiguration = reconfig_ies;
  if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra == 1 || get_softmodem_params()->sa == 1){
    UE->rb_config = get_default_rbconfig(10 /* EPS bearer ID */, 1 /* drb ID */, NR_CipheringAlgorithm_nea0, NR_SecurityConfig__keyToUse_master);
  } else {
    /* TODO: handle more than one bearer */
    if (m == NULL) {
      LOG_E(RRC, "fatal: m==NULL\n");
      exit(1);
    }
    if (m->nb_e_rabs_tobeadded != 1) {
      LOG_E(RRC, "fatal: m->nb_e_rabs_tobeadded = %d, should be 1\n", m->nb_e_rabs_tobeadded);
      exit(1);
    }

    /* store security key and security capabilities */
    memcpy(UE->kgnb, m->kgnb, 32);
    UE->security_capabilities.nRencryption_algorithms = m->security_capabilities.encryption_algorithms;
    UE->security_capabilities.nRintegrity_algorithms = m->security_capabilities.integrity_algorithms;

    /* Select ciphering algorithm based on gNB configuration file and
     * UE's supported algorithms.
     * We take the first from the list that is supported by the UE.
     * The ordering of the list comes from the configuration file.
     */
    /* preset nea0 as fallback */
    UE->ciphering_algorithm = 0;
    for (i = 0; i < rrc->security.ciphering_algorithms_count; i++) {
      int nea_mask[4] = {
        0,
        0x8000,  /* nea1 */
        0x4000,  /* nea2 */
        0x2000   /* nea3 */
      };
      if (rrc->security.ciphering_algorithms[i] == 0) {
        /* nea0 already preselected */
        break;
      }
      if (UE->security_capabilities.nRencryption_algorithms & nea_mask[rrc->security.ciphering_algorithms[i]]) {
        UE->ciphering_algorithm = rrc->security.ciphering_algorithms[i];
        break;
      }
    }

    LOG_I(RRC, "selecting ciphering algorithm %d\n", (int)UE->ciphering_algorithm);

    /* integrity: no integrity protection for DRB in ENDC mode
     * as written in 38.331: "If UE is connected to E-UTRA/EPC, this field
     * indicates the integrity protection algorithm to be used for SRBs
     * configured with NR PDCP, as specified in TS 33.501"
     * So nothing for DRB. Plus a test with a COTS UE revealed that it
     * did not apply integrity on the DRB.
     */
    UE->integrity_algorithm = 0;

    LOG_I(RRC, "selecting integrity algorithm %d\n", UE->integrity_algorithm);

    /* derive UP security key */
    nr_derive_key(UP_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, kUPenc);
    nr_derive_key(UP_INT_ALG, UE->integrity_algorithm, UE->kgnb, kUPint);

    e_NR_CipheringAlgorithm cipher_algo;
    switch (UE->ciphering_algorithm) {
      case 0:
        cipher_algo = NR_CipheringAlgorithm_nea0;
        break;
      case 1:
        cipher_algo = NR_CipheringAlgorithm_nea1;
        break;
      case 2:
        cipher_algo = NR_CipheringAlgorithm_nea2;
        break;
      case 3:
        cipher_algo = NR_CipheringAlgorithm_nea3;
        break;
      default:
        LOG_E(RRC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
        exit(1);
    }

    UE->rb_config = get_default_rbconfig(m->e_rabs_tobeadded[0].e_rab_id, m->e_rabs_tobeadded[0].drb_ID, cipher_algo, NR_SecurityConfig__keyToUse_secondary);
  }

  NR_ServingCellConfig_t *scc = UE->spCellConfig ? UE->spCellConfig->spCellConfigDedicated : NULL;
  // The MAC has the ServingCellConfigCommon; the below code is incorrect: the
  // CU should send a UE Context Setup Request to request the creating of the
  // MAC Context
  NR_ServingCellConfigCommon_t *sccc = RC.nrmac[0]->common_channels[0].ServingCellConfigCommon;
  UE->secondaryCellGroup = get_default_secondaryCellGroup(sccc,
                                                          scc,
                                                          UE->UE_Capability_nr,
                                                          1,
                                                          1,
                                                          configuration,
                                                          ue_context_p->ue_context.rrc_ue_id);
  AssertFatal(UE->secondaryCellGroup != NULL, "out of memory\n");
  xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, UE->secondaryCellGroup);

  fill_default_reconfig(sccc, scc, reconfig_ies, UE->secondaryCellGroup, UE->UE_Capability_nr, ue_context_p->ue_context.rrc_ue_id);
  UE->rnti = UE->secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity;
  NR_CG_Config_t *CG_Config = calloc(1,sizeof(*CG_Config));
  memset((void *)CG_Config,0,sizeof(*CG_Config));
  // int CG_Config_size = generate_CG_Config(rrc,CG_Config,UE->reconfig,UE->rb_config);
  generate_CG_Config(rrc, CG_Config, UE->reconfig, UE->rb_config);

  if(m!=NULL) {
    uint8_t inde_list[m->nb_e_rabs_tobeadded];
    memset(inde_list, 0, m->nb_e_rabs_tobeadded*sizeof(uint8_t));

    if (m->nb_e_rabs_tobeadded>0) {
      for (int i=0; i<m->nb_e_rabs_tobeadded; i++) {
        // Add the new E-RABs at the corresponding rrc ue context of the gNB
        UE->e_rab[i].param.e_rab_id = m->e_rabs_tobeadded[i].e_rab_id;
        UE->e_rab[i].param.gtp_teid = m->e_rabs_tobeadded[i].gtp_teid;
        memcpy(&UE->e_rab[i].param.sgw_addr, &m->e_rabs_tobeadded[i].sgw_addr, sizeof(transport_layer_addr_t));
        UE->nb_of_e_rabs++;
        //Fill the required E-RAB specific information for the creation of the S1-U tunnel between the gNB and the SGW
        create_tunnel_req.eps_bearer_id[i] = UE->e_rab[i].param.e_rab_id;
        create_tunnel_req.sgw_S1u_teid[i] = UE->e_rab[i].param.gtp_teid;
        memcpy(&create_tunnel_req.sgw_addr[i], &UE->e_rab[i].param.sgw_addr, sizeof(transport_layer_addr_t));
        inde_list[i] = i;
        LOG_I(RRC,"S1-U tunnel: index %d target sgw ip %d.%d.%d.%d length %d gtp teid %u\n",
              i,
              create_tunnel_req.sgw_addr[i].buffer[0],
              create_tunnel_req.sgw_addr[i].buffer[1],
              create_tunnel_req.sgw_addr[i].buffer[2],
              create_tunnel_req.sgw_addr[i].buffer[3],
              create_tunnel_req.sgw_addr[i].length,
              create_tunnel_req.sgw_S1u_teid[i]);
      }

      create_tunnel_req.rnti = ue_context_p->ue_context.rrc_ue_id;
      create_tunnel_req.num_tunnels    = m->nb_e_rabs_tobeadded;
      RB_INSERT(rrc_nr_ue_tree_s, &RC.nrrrc[rrc->module_id]->rrc_ue_head, ue_context_p);
      PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, rrc->module_id, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0, rrc->module_id);
      memset(&create_tunnel_resp, 0, sizeof(create_tunnel_resp));
      if (!IS_SOFTMODEM_NOS1) {
        LOG_D(RRC, "Calling gtpv1u_create_s1u_tunnel()\n");
        gtpv1u_create_s1u_tunnel(ctxt.instance, &create_tunnel_req, &create_tunnel_resp, nr_pdcp_data_req_drb);
        rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
          &ctxt,
          &create_tunnel_resp,
          &inde_list[0]);
      }
      X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).nb_e_rabs_admitted_tobeadded = m->nb_e_rabs_tobeadded;
      X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).target_assoc_id = m->target_assoc_id;

      for (int i = 0; i < UE->nb_of_e_rabs; i++) {
        X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].e_rab_id = UE->e_rab[i].param.e_rab_id;
        X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gtp_teid = create_tunnel_resp.enb_S1u_teid[i];
        memcpy(&X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr, &create_tunnel_resp.enb_addr, sizeof(transport_layer_addr_t));
        //The length field in the X2AP targetting structure is expected in bits but the create_tunnel_resp returns the address length in bytes
        X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.length = create_tunnel_resp.enb_addr.length*8;
        LOG_I(RRC,"S1-U create_tunnel_resp tunnel: index %d target gNB ip %d.%d.%d.%d length %d gtp teid %u\n",
              i,
              create_tunnel_resp.enb_addr.buffer[0],
              create_tunnel_resp.enb_addr.buffer[1],
              create_tunnel_resp.enb_addr.buffer[2],
              create_tunnel_resp.enb_addr.buffer[3],
              create_tunnel_resp.enb_addr.length,
              create_tunnel_resp.enb_S1u_teid[i]);
        LOG_I(RRC,"X2AP sGNB Addition Request: index %d target gNB ip %d.%d.%d.%d length %d gtp teid %u\n",
              i,
              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.buffer[0],
              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.buffer[1],
              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.buffer[2],
              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.buffer[3],
              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gnb_addr.length,
              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].gtp_teid);
      }
    } else
      LOG_W(RRC, "No E-RAB to be added received from SgNB Addition Request message \n");

    X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).MeNB_ue_x2_id = m->ue_x2_id;
    X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).SgNB_ue_x2_id = UE->secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity;
    //X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer_size = CG_Config_size; //Need to verify correct value for the buffer_size
    // Send to X2 entity to transport to MeNB
    asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CG_Config,
                              NULL,
                              (void *)CG_Config,
                              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer,
                              sizeof(X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer));
    X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer_size = (enc_rval.encoded+7)>>3;
    itti_send_msg_to_task(TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(0), msg); //Check right id instead of hardcoding
  }

  // below we configure the stack. In SA, we fully adopted F1, meaning that
  // layers use different IDs (MAC/RLC use RNTI as DU UE ID, above use NGAP ID
  // as CU UE ID.
  uint32_t du_ue_id = ue_context_p->ue_context.rnti;
  static uint32_t rrc_ue_id = 0;
  rrc_ue_id++;
  f1_ue_data_t du_ue_data = {.secondary_ue = rrc_ue_id};
  du_add_f1_ue_data(du_ue_id, &du_ue_data);
  f1_ue_data_t cu_ue_data = {.secondary_ue = du_ue_id};
  cu_add_f1_ue_data(rrc_ue_id, &cu_ue_data);
  LOG_I(RRC, "Assign CU UE ID %d and DU UE ID %d to UE RNTI %04x\n", rrc_ue_id, du_ue_id, ue_context_p->ue_context.rnti);

  // configure MAC and RLC
  bool ret = false;
  if (get_softmodem_params()->phy_test) {
    // phytest mode: we don't set up RA, etc
    ret = nr_mac_add_test_ue(RC.nrmac[rrc->module_id], du_ue_id, ue_context_p->ue_context.secondaryCellGroup);
  } else {
    NR_CellGroupConfig_t *secondaryCellGroup = ue_context_p->ue_context.secondaryCellGroup;
    DevAssert(secondaryCellGroup->spCellConfig
              && secondaryCellGroup->spCellConfig->reconfigurationWithSync
              && secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated
              && secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra);
    NR_SCHED_LOCK(&RC.nrmac[rrc->module_id]->sched_lock);
    ret = nr_mac_prepare_ra_ue(RC.nrmac[rrc->module_id], du_ue_id, ue_context_p->ue_context.secondaryCellGroup);
    NR_SCHED_UNLOCK(&RC.nrmac[rrc->module_id]->sched_lock);
  }
  AssertFatal(ret, "cannot add NSA UE in MAC, aborting\n");

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, rrc->module_id, GNB_FLAG_YES, rrc_ue_id, 0, 0, rrc->module_id);
  LOG_W(RRC,
        "Calling RRC PDCP/RLC ASN1 request functions for protocol context %p with module_id %d, rnti %lx, frame %d, subframe %d eNB_index %d \n",
        &ctxt,
        ctxt.module_id,
        ctxt.rntiMaybeUEid,
        ctxt.frame,
        ctxt.subframe,
        ctxt.eNB_index);

  nr_pdcp_add_drbs(ctxt.enb_flag,
                   rrc_ue_id,
                   ue_context_p->ue_context.rb_config->drb_ToAddModList,
                   (ue_context_p->ue_context.integrity_algorithm << 4) | ue_context_p->ue_context.ciphering_algorithm,
                   kUPenc,
                   kUPint);

  ctxt.rntiMaybeUEid = du_ue_id;
  // assume only a single bearer
  const NR_DRB_ToAddModList_t *drb_list = ue_context_p->ue_context.rb_config->drb_ToAddModList;
  DevAssert(drb_list->list.count == 1);
  const NR_DRB_ToAddMod_t *drb = drb_list->list.array[0];
  const struct NR_CellGroupConfig__rlc_BearerToAddModList *bearer_list =
      ue_context_p->ue_context.secondaryCellGroup->rlc_BearerToAddModList;
  const NR_RLC_BearerConfig_t *bearer = bearer_list->list.array[0];
  DevAssert(bearer_list->list.count == 1);
  DevAssert(drb->drb_Identity == bearer->servedRadioBearer->choice.drb_Identity);
  nr_rlc_add_drb(ctxt.rntiMaybeUEid, drb->drb_Identity, bearer);

  LOG_D(RRC, "%s:%d: done RRC PDCP/RLC ASN1 request for UE rnti %lx\n", __FUNCTION__, __LINE__, ctxt.rntiMaybeUEid);
}

void rrc_remove_nsa_user(gNB_RRC_INST *rrc, int rnti) {
  protocol_ctxt_t      ctxt;
  rrc_gNB_ue_context_t *ue_context;
  int                  e_rab;

  LOG_D(RRC, "calling rrc_remove_nsa_user rnti %d\n", rnti);
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, rrc->module_id, GNB_FLAG_YES, rnti, 0, 0, rrc->module_id);

  ue_context = rrc_gNB_get_ue_context_by_rnti_any_du(rrc, rnti);
  if (ue_context == NULL) {
    LOG_W(RRC, "rrc_remove_nsa_user: rnti %d not found\n", rnti);
    return;
  }

  nr_pdcp_remove_UE(ctxt.rntiMaybeUEid);

  rrc_rlc_remove_ue(&ctxt);

  // lock the scheduler before removing the UE. Note: mac_remove_nr_ue() checks
  // that the scheduler is actually locked!
  NR_SCHED_LOCK(&RC.nrmac[rrc->module_id]->sched_lock);
  mac_remove_nr_ue(RC.nrmac[rrc->module_id], rnti);
  NR_SCHED_UNLOCK(&RC.nrmac[rrc->module_id]->sched_lock);
  gtpv1u_enb_delete_tunnel_req_t tmp={0};
  tmp.rnti=rnti;
  tmp.from_gnb=1;
  LOG_D(RRC, "ue_context->ue_context.nb_of_e_rabs %d will be deleted for rnti %d\n", ue_context->ue_context.nb_of_e_rabs, rnti);
  for (e_rab = 0; e_rab < ue_context->ue_context.nb_of_e_rabs; e_rab++) {
    tmp.eps_bearer_id[tmp.num_erab++]= ue_context->ue_context.nsa_gtp_ebi[e_rab];
    // erase data
    ue_context->ue_context.nsa_gtp_teid[e_rab] = 0;
    memset(&ue_context->ue_context.nsa_gtp_addrs[e_rab], 0, sizeof(ue_context->ue_context.nsa_gtp_addrs[e_rab]));
    ue_context->ue_context.nsa_gtp_ebi[e_rab] = 0;
  }
  gtpv1u_delete_all_s1u_tunnel(rrc->module_id, rnti);
  /* remove context */
  rrc_gNB_remove_ue_context(rrc, ue_context);
}
