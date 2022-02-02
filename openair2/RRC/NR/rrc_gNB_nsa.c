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
#include "NR_CG-Config.h"
//#include "NR_SRB-ToAddModList.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/RRC/LTE/rrc_eNB_GTPV1U.h"
#include "executables/softmodem-common.h"
#include "executables/nr-softmodem.h"
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "UTIL/OSA/osa_defs.h"

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

void rrc_parse_ue_capabilities(gNB_RRC_INST *rrc, NR_UE_CapabilityRAT_ContainerList_t *UE_CapabilityRAT_ContainerList, x2ap_ENDC_sgnb_addition_req_t *m, NR_CG_ConfigInfo_IEs_t  *cg_config_info) {
  struct rrc_gNB_ue_context_s        *ue_context_p = NULL;

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
  ue_context_p = rrc_gNB_allocate_new_UE_context(rrc);

  if (ueCapabilityRAT_Container_nr != NULL) {
    dec_rval = uper_decode(NULL,
                           &asn_DEF_NR_UE_NR_Capability,
                           (void **)&ue_context_p->ue_context.UE_Capability_nr,
                           ueCapabilityRAT_Container_nr->buf,
                           ueCapabilityRAT_Container_nr->size, 0, 0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
      LOG_E(RRC, "Failed to decode UE NR capabilities (%zu bytes) container size %lu\n", dec_rval.consumed,ueCapabilityRAT_Container_nr->size);
      ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability,
                      ue_context_p->ue_context.UE_Capability_nr);
      ue_context_p->ue_context.UE_Capability_nr = 0;
      AssertFatal(1==0,"exiting\n");
    }
  }

  if (ueCapabilityRAT_Container_MRDC != NULL) {
    dec_rval = uper_decode(NULL,
                           &asn_DEF_NR_UE_MRDC_Capability,
                           (void **)&ue_context_p->ue_context.UE_Capability_MRDC,
                           ueCapabilityRAT_Container_MRDC->buf,
                           ueCapabilityRAT_Container_MRDC->size, 0, 0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
      LOG_E(RRC, "Failed to decode UE MRDC capabilities (%zu bytes)\n", dec_rval.consumed);
      ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability,
                      ue_context_p->ue_context.UE_Capability_MRDC);
      ue_context_p->ue_context.UE_Capability_MRDC = 0;
      AssertFatal(1==0,"exiting\n");
    }
  }

  // dump ue_capabilities

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) && ueCapabilityRAT_Container_nr != NULL ) {
    xer_fprint(stdout, &asn_DEF_NR_UE_NR_Capability, ue_context_p->ue_context.UE_Capability_nr);
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) && ueCapabilityRAT_Container_MRDC != NULL ) {
    xer_fprint(stdout, &asn_DEF_NR_UE_MRDC_Capability, ue_context_p->ue_context.UE_Capability_MRDC);
  }
  LOG_A(NR_RRC, "Successfully decoded UE NR capabilities (NR and MRDC)\n");

  rrc_add_nsa_user(rrc,ue_context_p, m);
}

/* generate prototypes for the tree management functions (RB_INSERT used in rrc_add_nsa_user) */
RB_PROTOTYPE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries,
             rrc_gNB_compare_ue_rnti_id);

void rrc_add_nsa_user(gNB_RRC_INST *rrc,struct rrc_gNB_ue_context_s *ue_context_p, x2ap_ENDC_sgnb_addition_req_t *m) {
  // generate nr-Config-r15 containers for LTE RRC : inside message for X2 EN-DC (CG-Config Message from 38.331)
  rrc_gNB_carrier_data_t *carrier=&rrc->carrier;
  MessageDef *msg;
  msg = itti_alloc_new_message(TASK_RRC_ENB, 0, X2AP_ENDC_SGNB_ADDITION_REQ_ACK);
  gtpv1u_enb_create_tunnel_req_t  create_tunnel_req;
  gtpv1u_enb_create_tunnel_resp_t create_tunnel_resp;
  protocol_ctxt_t ctxt={0};
  unsigned char *kUPenc = NULL;
  unsigned char *kUPint = NULL;
  int i;

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
    ue_context_p->ue_context.UE_Capability_nr = UE_Capability_nr;
  }

  // NR RRCReconfiguration
  AssertFatal(rrc->Nb_ue < MAX_NR_RRC_UE_CONTEXTS,"cannot add another UE\n");
  ue_context_p->ue_context.reconfig = calloc(1,sizeof(NR_RRCReconfiguration_t));
  ue_context_p->ue_context.secondaryCellGroup = calloc(1,sizeof(NR_CellGroupConfig_t));
  memset((void *)ue_context_p->ue_context.reconfig,0,sizeof(NR_RRCReconfiguration_t));
  ue_context_p->ue_context.reconfig->rrc_TransactionIdentifier=0;
  ue_context_p->ue_context.reconfig->criticalExtensions.present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
  NR_RRCReconfiguration_IEs_t *reconfig_ies=calloc(1,sizeof(NR_RRCReconfiguration_IEs_t));
  ue_context_p->ue_context.reconfig->criticalExtensions.choice.rrcReconfiguration = reconfig_ies;
  carrier->initial_csi_index[ue_context_p->local_uid + 1] = 0;
  ue_context_p->ue_context.rb_config = calloc(1,sizeof(NR_RRCReconfiguration_t));
  if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra == 1 || get_softmodem_params()->sa == 1){
    fill_default_rbconfig(ue_context_p->ue_context.rb_config,
                          5 /* EPS bearer ID */,
                          1 /* drb ID */,
                          NR_CipheringAlgorithm_nea0,
                          NR_SecurityConfig__keyToUse_master);
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
    memcpy(ue_context_p->ue_context.kgnb, m->kgnb, 32);
    ue_context_p->ue_context.security_capabilities.nRencryption_algorithms = m->security_capabilities.encryption_algorithms;
    ue_context_p->ue_context.security_capabilities.nRintegrity_algorithms = m->security_capabilities.integrity_algorithms;

    /* Select ciphering algorithm based on gNB configuration file and
     * UE's supported algorithms.
     * We take the first from the list that is supported by the UE.
     * The ordering of the list comes from the configuration file.
     */
    /* preset nea0 as fallback */
    ue_context_p->ue_context.ciphering_algorithm = 0;
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
      if (ue_context_p->ue_context.security_capabilities.nRencryption_algorithms & nea_mask[rrc->security.ciphering_algorithms[i]]) {
        ue_context_p->ue_context.ciphering_algorithm = rrc->security.ciphering_algorithms[i];
        break;
      }
    }

    LOG_I(RRC, "selecting ciphering algorithm %d\n", (int)ue_context_p->ue_context.ciphering_algorithm);

    /* integrity: no integrity protection for DRB in ENDC mode
     * as written in 38.331: "If UE is connected to E-UTRA/EPC, this field
     * indicates the integrity protection algorithm to be used for SRBs
     * configured with NR PDCP, as specified in TS 33.501"
     * So nothing for DRB. Plus a test with a COTS UE revealed that it
     * did not apply integrity on the DRB.
     */
    ue_context_p->ue_context.integrity_algorithm = 0;

    LOG_I(RRC, "selecting integrity algorithm %d\n", ue_context_p->ue_context.integrity_algorithm);

    /* derive UP security key */
    nr_derive_key_up_enc(ue_context_p->ue_context.ciphering_algorithm,
                         ue_context_p->ue_context.kgnb,
                         &kUPenc);
    nr_derive_key_up_int(ue_context_p->ue_context.integrity_algorithm,
                         ue_context_p->ue_context.kgnb,
                         &kUPint);

    e_NR_CipheringAlgorithm cipher_algo;
    switch (ue_context_p->ue_context.ciphering_algorithm) {
    case 0: cipher_algo = NR_CipheringAlgorithm_nea0; break;
    case 1: cipher_algo = NR_CipheringAlgorithm_nea1; break;
    case 2: cipher_algo = NR_CipheringAlgorithm_nea2; break;
    case 3: cipher_algo = NR_CipheringAlgorithm_nea3; break;
    default: LOG_E(RRC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__); exit(1);
    }

    fill_default_rbconfig(ue_context_p->ue_context.rb_config,
                          m->e_rabs_tobeadded[0].e_rab_id,
                          m->e_rabs_tobeadded[0].drb_ID,
                          cipher_algo,
                          NR_SecurityConfig__keyToUse_secondary);
  }
  if (ue_context_p->ue_context.spCellConfig) {
    fill_default_reconfig(carrier->servingcellconfigcommon,
                        ue_context_p->ue_context.spCellConfig->spCellConfigDedicated,
                        reconfig_ies,
                        ue_context_p->ue_context.secondaryCellGroup,
                        carrier->pdsch_AntennaPorts,
                        carrier->minRXTXTIME,
                        carrier->do_CSIRS,
                        carrier->initial_csi_index[ue_context_p->local_uid + 1],
                        ue_context_p->local_uid);
  } else {
    fill_default_reconfig(carrier->servingcellconfigcommon,
                        NULL,
                        reconfig_ies,
                        ue_context_p->ue_context.secondaryCellGroup,
                        carrier->pdsch_AntennaPorts,
                        carrier->minRXTXTIME,
                        carrier->do_CSIRS,
                        carrier->initial_csi_index[ue_context_p->local_uid + 1],
                        ue_context_p->local_uid);
  }
  ue_context_p->ue_id_rnti = ue_context_p->ue_context.secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity;
  NR_CG_Config_t *CG_Config = calloc(1,sizeof(*CG_Config));
  memset((void *)CG_Config,0,sizeof(*CG_Config));
  //int CG_Config_size = generate_CG_Config(rrc,CG_Config,ue_context_p->ue_context.reconfig,ue_context_p->ue_context.rb_config);
  generate_CG_Config(rrc,CG_Config,ue_context_p->ue_context.reconfig,ue_context_p->ue_context.rb_config);

  if(m!=NULL) {
    uint8_t inde_list[m->nb_e_rabs_tobeadded];
    memset(inde_list, 0, m->nb_e_rabs_tobeadded*sizeof(uint8_t));

    if (m->nb_e_rabs_tobeadded>0) {
      for (int i=0; i<m->nb_e_rabs_tobeadded; i++) {
        // Add the new E-RABs at the corresponding rrc ue context of the gNB 
        ue_context_p->ue_context.e_rab[i].param.e_rab_id = m->e_rabs_tobeadded[i].e_rab_id;
        ue_context_p->ue_context.e_rab[i].param.gtp_teid = m->e_rabs_tobeadded[i].gtp_teid;
        memcpy(&ue_context_p->ue_context.e_rab[i].param.sgw_addr, &m->e_rabs_tobeadded[i].sgw_addr, sizeof(transport_layer_addr_t));
        ue_context_p->ue_context.nb_of_e_rabs++;
        //Fill the required E-RAB specific information for the creation of the S1-U tunnel between the gNB and the SGW
        create_tunnel_req.eps_bearer_id[i]       = ue_context_p->ue_context.e_rab[i].param.e_rab_id;
        create_tunnel_req.sgw_S1u_teid[i]        = ue_context_p->ue_context.e_rab[i].param.gtp_teid;
        memcpy(&create_tunnel_req.sgw_addr[i], &ue_context_p->ue_context.e_rab[i].param.sgw_addr, sizeof(transport_layer_addr_t));
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

      create_tunnel_req.rnti           = ue_context_p->ue_id_rnti;
      create_tunnel_req.num_tunnels    = m->nb_e_rabs_tobeadded;
      RB_INSERT(rrc_nr_ue_tree_s, &RC.nrrrc[rrc->module_id]->rrc_ue_head, ue_context_p);
      PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, rrc->module_id, GNB_FLAG_YES, ue_context_p->ue_id_rnti, 0, 0,rrc->module_id);
      memset(&create_tunnel_resp, 0, sizeof(create_tunnel_resp));
      if (!IS_SOFTMODEM_NOS1) {
        LOG_D(RRC, "Calling gtpv1u_create_s1u_tunnel()\n");
        gtpv1u_create_s1u_tunnel(
          ctxt.instance,
          &create_tunnel_req,
          &create_tunnel_resp);
        rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
          &ctxt,
          &create_tunnel_resp,
          &inde_list[0]);
      }
      X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).nb_e_rabs_admitted_tobeadded = m->nb_e_rabs_tobeadded;
      X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).target_assoc_id = m->target_assoc_id;

      for(int i=0; i<ue_context_p->ue_context.nb_of_e_rabs; i++) {
        X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).e_rabs_admitted_tobeadded[i].e_rab_id = ue_context_p->ue_context.e_rab[i].param.e_rab_id;
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
    X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).SgNB_ue_x2_id = ue_context_p->ue_context.secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity;
    //X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer_size = CG_Config_size; //Need to verify correct value for the buffer_size
    // Send to X2 entity to transport to MeNB
    asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CG_Config,
                              NULL,
                              (void *)CG_Config,
                              X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer,
                              sizeof(X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer));
    X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg).rrc_buffer_size = (enc_rval.encoded+7)>>3;
    itti_send_msg_to_task(TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(0), msg); //Check right id instead of hardcoding
  } else if (get_softmodem_params()->do_ra || get_softmodem_params()->sa) {
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, rrc->module_id, GNB_FLAG_YES, ue_context_p->ue_id_rnti, 0, 0,rrc->module_id);
  }

  rrc->Nb_ue++;
  // configure MAC and RLC
  if (NODE_IS_DU(rrc->node_type)) {
    rrc_mac_config_req_gNB(rrc->module_id,
                           rrc->carrier.ssb_SubcarrierOffset,
                           rrc->carrier.pdsch_AntennaPorts,
			   rrc->carrier.pusch_AntennaPorts,
                           rrc->carrier.sib1_tda,
                           rrc->carrier.servingcellconfigcommon,
                           &rrc->carrier.mib,
                           NULL,
                           1, // add_ue flag
                           ue_context_p->ue_id_rnti,
                           ue_context_p->ue_context.secondaryCellGroup);
  } else {
    rrc_mac_config_req_gNB(rrc->module_id,
                           rrc->carrier.ssb_SubcarrierOffset,
                           rrc->carrier.pdsch_AntennaPorts,
                           rrc->carrier.pusch_AntennaPorts,
                           rrc->carrier.sib1_tda,
                           NULL,
                           NULL,
                           NULL,
                           1, // add_ue flag
                           ue_context_p->ue_id_rnti,
                           ue_context_p->ue_context.secondaryCellGroup);
  }

  if(m == NULL){
    LOG_W(RRC, "Calling RRC PDCP/RLC ASN1 request functions for protocol context %p with module_id %d, rnti %x, frame %d, subframe %d eNB_index %d \n", &ctxt,
                                                                                                                                                        ctxt.module_id,
                                                                                                                                                        ctxt.rnti,
                                                                                                                                                        ctxt.frame,
                                                                                                                                                        ctxt.subframe,
                                                                                                                                                        ctxt.eNB_index);
  }

  nr_rrc_pdcp_config_asn1_req(&ctxt,
                              get_softmodem_params()->sa ? ue_context_p->ue_context.rb_config->srb_ToAddModList : (NR_SRB_ToAddModList_t *) NULL,
                              ue_context_p->ue_context.rb_config->drb_ToAddModList ,
                              ue_context_p->ue_context.rb_config->drb_ToReleaseList,
                              (ue_context_p->ue_context.integrity_algorithm << 4) | ue_context_p->ue_context.ciphering_algorithm,
                              NULL,          /* kRRCenc - unused */
                              NULL,          /* kRRCint - unused */
                              kUPenc,        /* kUPenc  */
                              kUPint,        /* kUPint */
                              NULL,
                              NULL,
                              ue_context_p->ue_context.secondaryCellGroup->rlc_BearerToAddModList);

  nr_rrc_rlc_config_asn1_req (&ctxt,
                              get_softmodem_params()->sa ? ue_context_p->ue_context.rb_config->srb_ToAddModList : (NR_SRB_ToAddModList_t *) NULL,
                              ue_context_p->ue_context.rb_config->drb_ToAddModList,
                              ue_context_p->ue_context.rb_config->drb_ToReleaseList,
                              (LTE_PMCH_InfoList_r9_t *) NULL,
                              ue_context_p->ue_context.secondaryCellGroup->rlc_BearerToAddModList);

  LOG_D(RRC, "%s:%d: done RRC PDCP/RLC ASN1 request for UE rnti %x\n", __FUNCTION__, __LINE__, ctxt.rnti);

}

void rrc_remove_nsa_user(gNB_RRC_INST *rrc, int rnti) {
  protocol_ctxt_t      ctxt;
  rrc_gNB_ue_context_t *ue_context;
  int                  e_rab;

  LOG_D(RRC, "calling rrc_remove_nsa_user rnti %d\n", rnti);
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, rrc->module_id, GNB_FLAG_YES, rnti, 0, 0, rrc->module_id);

  ue_context = rrc_gNB_get_ue_context(rrc, rnti);
  if (ue_context == NULL) {
    LOG_W(RRC, "rrc_remove_nsa_user: rnti %d not found\n", rnti);
    return;
  }

  pdcp_remove_UE(&ctxt);

  rrc_rlc_remove_ue(&ctxt);

  mac_remove_nr_ue(rrc->module_id, rnti);
  gtpv1u_enb_delete_tunnel_req_t tmp={0};
  tmp.rnti=rnti;
  tmp.from_gnb=1;
  LOG_D(RRC, "ue_context->ue_context.nb_of_e_rabs %d will be deleted for rnti %d\n", ue_context->ue_context.nb_of_e_rabs, rnti);
  for (e_rab = 0; e_rab < ue_context->ue_context.nb_of_e_rabs; e_rab++) {
    tmp.eps_bearer_id[tmp.num_erab++]= ue_context->ue_context.gnb_gtp_ebi[e_rab];
    // erase data
    ue_context->ue_context.gnb_gtp_teid[e_rab] = 0;
    memset(&ue_context->ue_context.gnb_gtp_addrs[e_rab], 0, sizeof(ue_context->ue_context.gnb_gtp_addrs[e_rab]));
    ue_context->ue_context.gnb_gtp_ebi[e_rab] = 0;
  }
  gtpv1u_delete_s1u_tunnel(rrc->module_id,  &tmp);
  /* remove context */
  rrc_gNB_remove_ue_context(&ctxt, rrc, ue_context);
}
