/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief rrc NSA procedures for gNB
 */

#include <assert.h>
#include <assertions.h>
#include <openair2/RRC/NR/nr_rrc_proto.h>
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MESSAGES/asn1_msg.h"
#include "OCTET_STRING.h"
#include "T.h"
#include "asn_codecs.h"
#include "asn_internal.h"
#include "assertions.h"
#include "common/ngran_types.h"
#include "common/ran_context.h"
#include "common/utils/T/T.h"
#include "constr_TYPE.h"
#include "executables/nr-softmodem.h"
#include "executables/softmodem-common.h"
#include "gtpv1_u_messages_types.h"
#include "intertask_interface.h"
#include "ngap_messages_types.h"
#include "nr_pdcp/nr_pdcp_entity.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"
#include "nr_rrc_defs.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair3/SECU/key_nas_deriver.h"
#include "openair2/SDAP/nr_sdap/nr_sdap_entity.h"
#include "rrc_gNB_du.h"
#include "rrc_cell_management.h"
#include "rlc.h"
#include "s1ap_messages_types.h"
#include "tree.h"
#include "uper_decoder.h"
#include "uper_encoder.h"
#include "x2ap_messages_types.h"
#include "xer_decoder.h"
#include "xer_encoder.h"
#include "f1ap_common.h"
#include "lib/f1ap_ue_context.h"
#include "rrc_gNB_asn1.h"

// In case of phy-test and do-ra mode, read UE capabilities directly from file
// and put it into a CG-ConfigInfo field
static int cg_config_info_from_ue_cap_file(uint32_t maxlen, uint8_t buf[maxlen])
{
  OCTET_STRING_t *list_buf = NULL;
  if (uecap_file != NULL) {
    LOG_I(NR_RRC, "creating CG-ConfigInfo from UE capability file %s\n", uecap_file);

    FILE *f = fopen(uecap_file, "r");
    if (!f) {
      LOG_E(NR_RRC, "cannot open file %s, cannot read UE capabilities\n", uecap_file);
      return 0;
    }
    char UE_NR_Capability_xer[65536];
    size_t size = fread(UE_NR_Capability_xer, 1, sizeof UE_NR_Capability_xer, f);
    fclose(f);
    if (size == 0 || size == sizeof UE_NR_Capability_xer) {
      LOG_E(NR_RRC, "UE Capabilities XER file %s could not be read (read %ld bytes)\n", uecap_file, size);
      return 0;
    }
    NR_UE_NR_Capability_t *cap = calloc_or_fail(1, sizeof(*cap));
    asn_dec_rval_t dec_rval = xer_decode(0, &asn_DEF_NR_UE_NR_Capability, (void *)&cap, UE_NR_Capability_xer, size);
    DevAssert(dec_rval.code == RC_OK);
    //xer_fprint(stdout, &asn_DEF_NR_UE_NR_Capability, cap);

    uint8_t *buf = NULL;
    int sizebuf = uper_encode_to_new_buffer(&asn_DEF_NR_UE_NR_Capability, NULL, cap, (void **)&buf);
    NR_UE_CapabilityRAT_Container_t *cont = calloc_or_fail(1, sizeof(*cont));
    cont->rat_Type = NR_RAT_Type_nr;
    cont->ue_CapabilityRAT_Container.buf = buf;
    cont->ue_CapabilityRAT_Container.size = sizebuf;
    NR_UE_CapabilityRAT_ContainerList_t *clist = calloc_or_fail(1, sizeof(*clist));
    asn1cSeqAdd(&clist->list, cont);
    list_buf = calloc_or_fail(1, sizeof(*list_buf));
    list_buf->size = uper_encode_to_new_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList, NULL, clist, (void **)&list_buf->buf);
    ASN_STRUCT_FREE(asn_DEF_NR_UE_CapabilityRAT_ContainerList, clist);
  }

  NR_CG_ConfigInfo_t *cgci = calloc_or_fail(1, sizeof(*cgci));
  cgci->criticalExtensions.present = NR_CG_ConfigInfo__criticalExtensions_PR_c1;
  cgci->criticalExtensions.choice.c1 = calloc_or_fail(1, sizeof(*cgci->criticalExtensions.choice.c1));
  cgci->criticalExtensions.choice.c1->present = NR_CG_ConfigInfo__criticalExtensions__c1_PR_cg_ConfigInfo;
  NR_CG_ConfigInfo_IEs_t *cgci_ies = calloc_or_fail(1, sizeof(*cgci_ies));
  cgci->criticalExtensions.choice.c1->choice.cg_ConfigInfo = cgci_ies;
  cgci_ies->ue_CapabilityInfo = list_buf;

  //xer_fprint(stdout, &asn_DEF_NR_CG_ConfigInfo, cgci);
  asn_enc_rval_t rval = uper_encode_to_buffer(&asn_DEF_NR_CG_ConfigInfo, NULL, cgci, buf, maxlen);
  DevAssert(rval.encoded > 0);
  ASN_STRUCT_FREE(asn_DEF_NR_CG_ConfigInfo, cgci);
  return (rval.encoded + 7) >> 3;
}

static instance_t get_f1_gtp_instance(void)
{
  const f1ap_cudu_inst_t *inst = getCxt(0);
  if (!inst)
    return -1; // means no F1
  return inst->gtpInst;
}

/* returns a default radio bearer config suitable for NSA etc */
static NR_RadioBearerConfig_t *get_default_rbconfig(int eps_bearer_id,
                                                    int rb_id,
                                                    e_NR_CipheringAlgorithm ciphering_algorithm,
                                                    e_NR_SecurityConfig__keyToUse key_to_use,
                                                    const nr_pdcp_configuration_t *pdcp_config)
{
  NR_RadioBearerConfig_t *rbconfig = calloc_or_fail(1, sizeof(*rbconfig));
  rbconfig->drb_ToAddModList = calloc_or_fail(1, sizeof(*rbconfig->drb_ToAddModList));
  NR_DRB_ToAddMod_t *drb_ToAddMod = calloc_or_fail(1, sizeof(*drb_ToAddMod));
  drb_ToAddMod->drb_Identity = rb_id;
  drb_ToAddMod->pdcp_Config = nr_rrc_build_pdcp_config_ie(false, false, pdcp_config);
  asn1cCalloc(drb_ToAddMod->cnAssociation, cn_association);
  cn_association->present = NR_DRB_ToAddMod__cnAssociation_PR_eps_BearerIdentity;
  cn_association->choice.eps_BearerIdentity = eps_bearer_id;
  asn1cSeqAdd(&rbconfig->drb_ToAddModList->list, drb_ToAddMod);
  asn1cCalloc(rbconfig->securityConfig, secConf);
  asn1cCalloc(secConf->securityAlgorithmConfig, secConfAlgo);
  secConfAlgo->cipheringAlgorithm = ciphering_algorithm;
  secConfAlgo->integrityProtAlgorithm = NULL;
  asn1cCallocOne(secConf->keyToUse, key_to_use);
  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_DRB_ToAddMod, drb_ToAddMod);
  return rbconfig;
}

static void rrc_nsa_add_drb(const int ue_id,
                            const NR_DRB_ToAddModList_t *addMod,
                            const nr_pdcp_entity_security_keys_and_algos_t *sp)
{
  for (int i = 0; i < addMod->list.count; i++) {
    NR_DRB_ToAddMod_t *drb = addMod->list.array[i];
    DevAssert(drb->cnAssociation);
    DevAssert(drb->cnAssociation->present != NR_DRB_ToAddMod__cnAssociation_PR_NOTHING);
    DevAssert(drb->cnAssociation->present == NR_DRB_ToAddMod__cnAssociation_PR_eps_BearerIdentity);
    // get SDAP config
    sdap_config_t sdap = {0};
    // EPC association
    sdap.pdusession_id = drb->cnAssociation->choice.eps_BearerIdentity;
    sdap.drb_id = drb->drb_Identity;
    sdap.defaultDRB = true;
    // add SDAP entity (terminated at gNB, since it's EPC)
    nr_sdap_addmod_entity(GNB_FLAG_YES, ue_id, &sdap);
    // add PDCP entity
    nr_pdcp_add_drb(GNB_FLAG_YES, ue_id, drb->pdcp_Config, &sdap, sp);
  }
}

/* generate prototypes for the tree management functions (RB_INSERT used in rrc_add_nsa_user) */
RB_PROTOTYPE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries,
             rrc_gNB_compare_ue_rnti_id);

void rrc_add_nsa_user(gNB_RRC_INST *rrc, x2ap_ENDC_sgnb_addition_req_t *m, sctp_assoc_t assoc_id)
{
  AssertFatal(!IS_SA_MODE(get_softmodem_params()), "%s() cannot be called in SA mode, it is intrinsically for NSA\n", __func__);

  /* all the 0 are DU-related info that we will fill later, see rrc_add_nsa_user_resp() below */
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_create_ue_context(assoc_id, 0, rrc, 0, 0);
  if (!ue_context_p)
    return;
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  nr_pdcp_entity_security_keys_and_algos_t security_parameters = {0};

  uint8_t tmp[1024];
  byte_array_t cgci = {0};
  if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra == 1) {
    DevAssert(m == NULL);
    UE->rb_config = get_default_rbconfig(DEFAULT_NOS1_PDU_ID, 1 /* drb ID */, NR_CipheringAlgorithm_nea0, NR_SecurityConfig__keyToUse_master, &rrc->pdcp_config);
    int len = cg_config_info_from_ue_cap_file(sizeof tmp, tmp);
    DevAssert(len > 0);
    cgci = create_byte_array(len, tmp);
  } else {
    DevAssert(m != NULL);
    cgci = create_byte_array(m->rrc_buffer_size, m->rrc_buffer);

    /* TODO: handle more than one bearer */
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
    for (int i = 0; i < rrc->security.ciphering_algorithms_count; i++) {
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
    security_parameters.ciphering_algorithm = UE->ciphering_algorithm;
    security_parameters.integrity_algorithm = UE->integrity_algorithm;
    nr_derive_key(UP_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, security_parameters.ciphering_key);
    nr_derive_key(UP_INT_ALG, UE->integrity_algorithm, UE->kgnb, security_parameters.integrity_key);

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

    UE->rb_config = get_default_rbconfig(m->e_rabs_tobeadded[0].e_rab_id, m->e_rabs_tobeadded[0].drb_ID, cipher_algo, NR_SecurityConfig__keyToUse_secondary, &rrc->pdcp_config);
  }

  if(m!=NULL) {

    UE->x2_target_assoc = m->target_assoc_id;
    UE->MeNB_ue_x2_id = m->ue_x2_id;
    gtpv1u_enb_create_tunnel_req_t  create_tunnel_req = {0};
    gtpv1u_enb_create_tunnel_resp_t create_tunnel_resp = {0};
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
      if (!IS_SOFTMODEM_NOS1) {
        gtpv1u_create_s1u_tunnel(rrc->module_id, &create_tunnel_req, &create_tunnel_resp, nr_pdcp_data_req_drb);
        DevAssert(create_tunnel_resp.num_tunnels == 1);
        UE->nsa_gtp_teid[0] = create_tunnel_resp.enb_S1u_teid[0];
        UE->nsa_gtp_addrs[0] = create_tunnel_resp.enb_addr;
        UE->nsa_gtp_ebi[0] = create_tunnel_resp.eps_bearer_id[0];
      }
    }
  }

  DevAssert(UE->rb_config != NULL);
  rrc_nsa_add_drb(UE->rrc_ue_id, UE->rb_config->drb_ToAddModList, &security_parameters);

  /* assumption: only a single bearer, see above */
  NR_DRB_ToAddModList_t *rb_list = UE->rb_config->drb_ToAddModList;
  AssertFatal(rb_list->list.count == 1, "can only handle one bearer for NSA/phy-test/do-ra, but has %d\n", rb_list->list.count);
  int drb_id = rb_list->list.array[0]->drb_Identity;
  f1ap_drb_to_setup_t *drb = calloc_or_fail(1, sizeof(*drb));
  drb->id = drb_id;
  // hardcoded keep it backwards compatible for now
  // rrc->configuration.um_on_default_drb ? F1AP_RLC_MODE_UM_BIDIR : F1AP_RLC_MODE_AM,
  drb->rlc_mode = F1AP_RLC_MODE_UM_BIDIR;
  drb->up_ul_tnl_len = 1;
  drb->qos_choice = F1AP_QOS_CHOICE_NR; // we don't have EUTRAN yet, "approximate" it
  drb->nr.flows_len = 1;
  f1ap_drb_flows_mapped_t *flow = drb->nr.flows = calloc_or_fail(drb->nr.flows_len, sizeof(*flow));
  flow->qfi = 9;
  flow->param.qos_type = NON_DYNAMIC;
  flow->param.nondyn.fiveQI = 9;
  flow->param.arp.prio = 5;

  // Note: E1 support for NSA/phy-test/do-ra not implemented yet
  instance_t f1inst = get_f1_gtp_instance();
  if (f1inst >= 0) {
      gtpv1u_gnb_create_tunnel_req_t req = {
        .ue_id = UE->rrc_ue_id,
        .incoming_rb_id = drb_id,
        .pdusession_id = drb_id,
        .outgoing_teid = 0xffff, // will be updated later
        .dst_addr.length = 32,
      };
      gtpv1u_gnb_create_tunnel_resp_t resp = {0};
      int ret = gtpv1u_create_ngu_tunnel(f1inst, &req, &resp, NULL, NULL, NULL);
      AssertFatal(ret == 0, "gtpv1u_create_ngu_tunnel failed: ret %d\n", ret);
      AssertFatal(resp.gnb_addr.length == sizeof(in_addr_t),
                  "GTP tunnel response address length %d does not match IPv4 size %zu\n",
                  resp.gnb_addr.length,
                  sizeof(in_addr_t));
      memcpy(&drb->up_ul_tnl[0].tl_address, &resp.gnb_addr.buffer, resp.gnb_addr.length);
      drb->up_ul_tnl[0].teid = resp.gnb_NGu_teid;
      drb->up_ul_tnl_len = 1;
  }
  uint64_t *ue_agg_mbr_ul = malloc_or_fail(sizeof(*ue_agg_mbr_ul));
  *ue_agg_mbr_ul = 1000000000;
  byte_array_t *cg_configinfo = malloc_or_fail(sizeof(*cg_configinfo));
  *cg_configinfo = cgci;

  f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  nr_rrc_du_container_t *du = get_du_by_assoc_id(rrc, ue_data.du_assoc_id);
  if (!du) {
    LOG_E(NR_RRC, "UE %d: no valid cell for UE context setup (DU assoc_id %d not found)\n", UE->rrc_ue_id, ue_data.du_assoc_id);
    return;
  }
  // Get the first cell from the DU (NSA assumption: 1 cell per DU)
  if (seq_arr_size(&du->cells) == 0) {
    LOG_E(NR_RRC, "UE %d: no cells available in DU (assoc_id %d)\n", UE->rrc_ue_id, ue_data.du_assoc_id);
    return;
  }
  nr_rrc_cell_container_t *ue_cell = *(nr_rrc_cell_container_t **)seq_arr_at(&du->cells, 0);
  // Add the cell as PCell
  if (rrc_add_ue_serving_cell(UE, ue_cell, RRC_PCELL_INDEX) == NULL) {
    LOG_E(NR_RRC, "UE %d: failed to add PCell (cell %ld) for UE context setup\n", UE->rrc_ue_id, ue_cell->info.cell_id);
    return;
  }
  f1ap_ue_context_setup_req_t req = {
      .gNB_CU_ue_id = UE->rrc_ue_id,
      .plmn.mcc = rrc->configuration.plmn[0].mcc,
      .plmn.mnc = rrc->configuration.plmn[0].mnc,
      .plmn.mnc_digit_length = rrc->configuration.plmn[0].mnc_digit_length,
      .nr_cellid = ue_cell->info.cell_id,
      .servCellIndex = RRC_PCELL_INDEX,
      .drbs_len = 1,
      .drbs = drb,
      .cu_to_du_rrc_info.cg_configinfo = cg_configinfo,
      .gnb_du_ue_agg_mbr_ul = ue_agg_mbr_ul,
  };
  rrc->mac_rrc.ue_context_setup_request(ue_data.du_assoc_id, &req);
  free_ue_context_setup_req(&req);
}

static NR_RRCReconfiguration_IEs_t *get_default_reconfig(const byte_array_t cgc)
{
  NR_RRCReconfiguration_IEs_t *reconfig = calloc(1, sizeof(NR_RRCReconfiguration_IEs_t));
  AssertFatal(reconfig != NULL, "out of memory\n");
  AssertFatal(cgc.buf != NULL, "cgc is null\n");
  reconfig->radioBearerConfig = NULL;
  // Copy the stored CellGroupConfig to the RRCReconfiguration message
  reconfig->secondaryCellGroup = calloc_or_fail(1, sizeof(*reconfig->secondaryCellGroup));
  OCTET_STRING_fromBuf(reconfig->secondaryCellGroup, (const char *)cgc.buf, cgc.len);
  reconfig->measConfig = NULL;
  reconfig->lateNonCriticalExtension = NULL;
  reconfig->nonCriticalExtension = NULL;
  return reconfig;
}

static NR_CG_Config_t *generate_CG_Config(const NR_RRCReconfiguration_t *reconfig, const NR_RadioBearerConfig_t *rbconfig)
{
  NR_CG_Config_t *cg_Config = calloc_or_fail(1, sizeof(*cg_Config));
  cg_Config->criticalExtensions.present = NR_CG_Config__criticalExtensions_PR_c1;
  cg_Config->criticalExtensions.choice.c1 = calloc_or_fail(1, sizeof(*cg_Config->criticalExtensions.choice.c1));
  cg_Config->criticalExtensions.choice.c1->present = NR_CG_Config__criticalExtensions__c1_PR_cg_Config;
  NR_CG_Config_IEs_t *cgc_ie = calloc_or_fail(1, sizeof(*cgc_ie));
  cg_Config->criticalExtensions.choice.c1->choice.cg_Config = cgc_ie;
  cgc_ie->scg_CellGroupConfig = calloc_or_fail(1, sizeof(*cgc_ie->scg_CellGroupConfig));
  cgc_ie->scg_CellGroupConfig->size =
      uper_encode_to_new_buffer(&asn_DEF_NR_RRCReconfiguration, NULL, reconfig, (void **)&cgc_ie->scg_CellGroupConfig->buf);
  AssertFatal(cgc_ie->scg_CellGroupConfig->size > 0,
              "ASN1 message encoding of RRCReconfiguration failed (%ld)!\n",
              cgc_ie->scg_CellGroupConfig->size);

  cgc_ie->scg_RB_Config = calloc_or_fail(1, sizeof(*cgc_ie->scg_RB_Config));
  cgc_ie->scg_RB_Config->size =
      uper_encode_to_new_buffer(&asn_DEF_NR_RadioBearerConfig, NULL, rbconfig, (void **)&cgc_ie->scg_RB_Config->buf);
  AssertFatal(cgc_ie->scg_RB_Config->size > 0, "ASN1 message encoding failed (%ld)!\n", cgc_ie->scg_RB_Config->size);

  if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra > 0) {
    // This is for phytest only, emulate first X2 message if uecap.raw file is present
    LOG_I(RRC, "Dumping NR_RRCReconfiguration message (%jd bytes) to reconfig.raw\n", cgc_ie->scg_CellGroupConfig->size);
    FILE *fd = fopen("reconfig.raw", "w");
    AssertFatal(fd != NULL, "could not open reconig.raw for writing: %d, %s\n", errno, strerror(errno));
    fwrite(cgc_ie->scg_CellGroupConfig->buf, cgc_ie->scg_CellGroupConfig->size, 1, fd);
    fclose(fd);

    LOG_I(RRC, "Dumping scg_RB_Config message (%jd bytes) to reconfig.raw\n", cgc_ie->scg_RB_Config->size);
    fd = fopen("rbconfig.raw", "w");
    AssertFatal(fd != NULL, "could not open rbconig.raw for writing: %d, %s\n", errno, strerror(errno));
    fwrite(cgc_ie->scg_RB_Config->buf, cgc_ie->scg_RB_Config->size, 1, fd);
    fclose(fd);
  }

  return cg_Config;
}

void rrc_add_nsa_user_resp(gNB_RRC_UE_t *UE, const f1ap_ue_context_setup_resp_t *resp)
{
  DevAssert(resp->crnti != NULL);
  /* we did not fill any DU-related ID info in rrc_add_nsa_user() */
  UE->rnti = *resp->crnti;
  DevAssert(cu_exists_f1_ue_data(UE->rrc_ue_id));
  f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
  ue_data.secondary_ue = resp->gNB_DU_ue_id;
  bool success = cu_update_f1_ue_data(UE->rrc_ue_id, &ue_data);
  DevAssert(success);

  instance_t f1inst = get_f1_gtp_instance();
  if (f1inst >= 0) {
    // Note: E1 support for NSA/phy-test/do-ra not implemented yet
    // so set up GTP from here
    for (int i = 0; i < resp->drbs_len; ++i) {
      f1ap_drb_setup_t *drb = &resp->drbs[i];
      DevAssert(drb->up_dl_tnl_len == 1);
      in_addr_t addr = drb->up_dl_tnl[0].tl_address;
      uint32_t teid = drb->up_dl_tnl[0].teid;
      GtpuUpdateTunnelOutgoingAddressAndTeid(f1inst, UE->rrc_ue_id, drb->id, addr, teid);
    }
  }

  NR_RRCReconfiguration_t *reconfig = calloc(1, sizeof(NR_RRCReconfiguration_t));
  reconfig->rrc_TransactionIdentifier = 0;
  reconfig->criticalExtensions.present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
  reconfig->criticalExtensions.choice.rrcReconfiguration = get_default_reconfig(UE->mcg);

  NR_CG_Config_t *CG_Config = generate_CG_Config(reconfig, UE->rb_config);
  ASN_STRUCT_FREE(asn_DEF_NR_RRCReconfiguration, reconfig);

  if (get_softmodem_params()->phy_test > 0 || get_softmodem_params()->do_ra > 0) {
    /* we are done, no X2 answer necessary */
    ASN_STRUCT_FREE(asn_DEF_NR_CG_Config, CG_Config);
    return;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_RRC_ENB, 0, X2AP_ENDC_SGNB_ADDITION_REQ_ACK);
  x2ap_ENDC_sgnb_addition_req_ACK_t *ack = &X2AP_ENDC_SGNB_ADDITION_REQ_ACK(msg);

  ack->nb_e_rabs_admitted_tobeadded = UE->nb_of_e_rabs;
  DevAssert(UE->x2_target_assoc > 0);
  ack->target_assoc_id = UE->x2_target_assoc;

  for (int i = 0; i < UE->nb_of_e_rabs; i++) {
    ack->e_rabs_admitted_tobeadded[i].e_rab_id = UE->e_rab[i].param.e_rab_id;
    ack->e_rabs_admitted_tobeadded[i].gtp_teid = UE->nsa_gtp_teid[0];
    memcpy(&ack->e_rabs_admitted_tobeadded[i].gnb_addr, &UE->nsa_gtp_addrs[0], sizeof(transport_layer_addr_t));
    ack->e_rabs_admitted_tobeadded[i].gnb_addr.length = 32; // bits, IPv4 only
  }

  ack->MeNB_ue_x2_id = UE->MeNB_ue_x2_id;
  ack->SgNB_ue_x2_id = UE->rrc_ue_id;

  // Send to X2 entity to transport to MeNB
  asn_enc_rval_t enc_rval =
      uper_encode_to_buffer(&asn_DEF_NR_CG_Config, NULL, (void *)CG_Config, ack->rrc_buffer, sizeof(ack->rrc_buffer));
  ack->rrc_buffer_size = (enc_rval.encoded + 7) >> 3;
  itti_send_msg_to_task(TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(0), msg);
  ASN_STRUCT_FREE(asn_DEF_NR_CG_Config, CG_Config);
}

void rrc_remove_nsa_user_context(gNB_RRC_INST *rrc, rrc_gNB_ue_context_t *ue_context)
{
  if (!IS_SOFTMODEM_NOS1)
    gtpv1u_delete_all_s1u_tunnel(rrc->module_id, ue_context->ue_context.rrc_ue_id);
  instance_t f1inst = get_f1_gtp_instance();
  if (f1inst >= 0)
    gtpv1u_delete_all_s1u_tunnel(f1inst, ue_context->ue_context.rrc_ue_id);
  // we don't use E1 => we have to free SDAP
  nr_sdap_delete_ue_entities(ue_context->ue_context.rrc_ue_id);
  rrc_remove_ue(rrc, ue_context);
}

void rrc_release_nsa_user(gNB_RRC_INST *rrc, rrc_gNB_ue_context_t *ue_context)
{
  gNB_RRC_UE_t *UE = &ue_context->ue_context;
  f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  f1ap_ue_context_rel_cmd_t cmd = {
      .gNB_CU_ue_id = UE->rrc_ue_id,
      .gNB_DU_ue_id = ue_data.secondary_ue,
      .cause = F1AP_CAUSE_RADIO_NETWORK,
      .cause_value = 10, // 10 = F1AP_CauseRadioNetwork_normal_release
  };
  rrc->mac_rrc.ue_context_release_command(ue_data.du_assoc_id, &cmd);
  rrc_remove_nsa_user_context(rrc, ue_context);
}
