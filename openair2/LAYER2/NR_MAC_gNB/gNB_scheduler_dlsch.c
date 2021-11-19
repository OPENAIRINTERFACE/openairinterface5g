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

/*! \file       gNB_scheduler_dlsch.c
 * \brief       procedures related to gNB for the DLSCH transport channel
 * \author      Guido Casati
 * \date        2019
 * \email:      guido.casati@iis.fraunhofe.de
 * \version     1.0
 * @ingroup     _mac

 */

#include "common/utils/nr/nr_common.h"
/*MAC*/
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"

/*NFAPI*/
#include "nfapi_nr_interface.h"
/*TAG*/
#include "NR_TAG-Id.h"

/*Softmodem params*/
#include "executables/softmodem-common.h"
#include "../../../nfapi/oai_integration/vendor_ext.h"

////////////////////////////////////////////////////////
/////* DLSCH MAC PDU generation (6.1.2 TS 38.321) */////
////////////////////////////////////////////////////////
#define OCTET 8
#define HALFWORD 16
#define WORD 32
//#define SIZE_OF_POINTER sizeof (void *)

void calculate_preferred_dl_tda(module_id_t module_id, const NR_BWP_Downlink_t *bwp)
{
  gNB_MAC_INST *nrmac = RC.nrmac[module_id];
  const int bwp_id = bwp ? bwp->bwp_Id : 0;
  if (nrmac->preferred_dl_tda[bwp_id])
    return;

  /* there is a mixed slot only when in TDD */
  NR_ServingCellConfigCommon_t *scc = nrmac->common_channels->ServingCellConfigCommon;
  const NR_TDD_UL_DL_Pattern_t *tdd =
      scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;
  const int symb_dlMixed = tdd ? (1 << tdd->nrofDownlinkSymbols) - 1 : 0;

  int target_ss;
  if (bwp) {
    target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  }
  else {
    target_ss = NR_SearchSpace__searchSpaceType_PR_common;
  }
  NR_SearchSpace_t *search_space = get_searchspace(scc, bwp ? bwp->bwp_Dedicated : NULL, target_ss);
  NR_ControlResourceSet_t *coreset = get_coreset(module_id, scc, bwp ? bwp->bwp_Dedicated : NULL, search_space, target_ss);
  // get coreset symbol "map"
  const uint16_t symb_coreset = (1 << coreset->duration) - 1;

  /* check that TDA index 0 fits into DL and does not overlap CORESET */
  const struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = bwp ?
      bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList :
      scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
  AssertFatal(tdaList->list.count >= 1, "need to have at least one TDA for DL slots\n");
  const NR_PDSCH_TimeDomainResourceAllocation_t *tdaP_DL = tdaList->list.array[0];
  AssertFatal(!tdaP_DL->k0 || *tdaP_DL->k0 == 0,
              "TimeDomainAllocation at index 1: non-null k0 (%ld) is not supported by the scheduler\n",
              *tdaP_DL->k0);
  int start, len;
  SLIV2SL(tdaP_DL->startSymbolAndLength, &start, &len);
  const uint16_t symb_tda = ((1 << len) - 1) << start;
  // check whether coreset and TDA overlap: then we cannot use it. Note that
  // here we assume that the coreset is scheduled every slot (which it
  // currently is) and starting at symbol 0
  AssertFatal((symb_coreset & symb_tda) == 0, "TDA index 0 for DL overlaps with CORESET\n");

  /* check that TDA index 1 fits into DL part of mixed slot, if it exists */
  int tdaMi = -1;
  if (tdaList->list.count > 1) {
    const NR_PDSCH_TimeDomainResourceAllocation_t *tdaP_Mi = tdaList->list.array[1];
    AssertFatal(!tdaP_Mi->k0 || *tdaP_Mi->k0 == 0,
                "TimeDomainAllocation at index 1: non-null k0 (%ld) is not supported by the scheduler\n",
                *tdaP_Mi->k0);
    int start, len;
    SLIV2SL(tdaP_Mi->startSymbolAndLength, &start, &len);
    const uint16_t symb_tda = ((1 << len) - 1) << start;
    // check whether coreset and TDA overlap: then, we cannot use it. Also,
    // check whether TDA is entirely within mixed slot DL. Note that
    // here we assume that the coreset is scheduled every slot (which it
    // currently is)
    if ((symb_coreset & symb_tda) == 0 && (symb_dlMixed & symb_tda) == symb_tda) {
      tdaMi = 1;
    } else {
      LOG_E(MAC,
            "TDA index 1 DL overlaps with CORESET or is not entirely in mixed slot (symb_coreset %x symb_dlMixed %x symb_tda %x), won't schedule DL mixed slot\n",
            symb_coreset,
            symb_dlMixed,
            symb_tda);
    }
  }

  const uint8_t slots_per_frame[5] = {10, 20, 40, 80, 160};
  const int n = slots_per_frame[*scc->ssbSubcarrierSpacing];
  nrmac->preferred_dl_tda[bwp_id] = malloc(n * sizeof(*nrmac->preferred_dl_tda[bwp_id]));

  const int nr_mix_slots = tdd ? tdd->nrofDownlinkSymbols != 0 || tdd->nrofUplinkSymbols != 0 : 0;
  const int nr_slots_period = tdd ? tdd->nrofDownlinkSlots + tdd->nrofUplinkSlots + nr_mix_slots : n;
  for (int i = 0; i < n; ++i) {
    nrmac->preferred_dl_tda[bwp_id][i] = -1;
    if (!tdd || i % nr_slots_period < tdd->nrofDownlinkSlots)
      nrmac->preferred_dl_tda[bwp_id][i] = 0;
    else if (tdd && nr_mix_slots && i % nr_slots_period == tdd->nrofDownlinkSlots)
      nrmac->preferred_dl_tda[bwp_id][i] = tdaMi;
    LOG_D(MAC, "slot %d preferred_dl_tda %d\n", i, nrmac->preferred_dl_tda[bwp_id][i]);
  }
}

// Compute and write all MAC CEs and subheaders, and return number of written
// bytes
int nr_write_ce_dlsch_pdu(module_id_t module_idP,
                          const NR_UE_sched_ctrl_t *ue_sched_ctl,
                          unsigned char *mac_pdu,
                          unsigned char drx_cmd,
                          unsigned char *ue_cont_res_id)
{
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_MAC_SUBHEADER_FIXED *mac_pdu_ptr = (NR_MAC_SUBHEADER_FIXED *) mac_pdu;
  uint8_t last_size = 0;
  int offset = 0, mac_ce_size, i, timing_advance_cmd, tag_id = 0;
  // MAC CEs
  uint8_t mac_header_control_elements[16], *ce_ptr;
  ce_ptr = &mac_header_control_elements[0];

  // DRX command subheader (MAC CE size 0)
  if (drx_cmd != 255) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_DRX;
    //last_size = 1;
    mac_pdu_ptr++;
  }

  // Timing Advance subheader
  /* This was done only when timing_advance_cmd != 31
  // now TA is always send when ta_timer resets regardless of its value
  // this is done to avoid issues with the timeAlignmentTimer which is
  // supposed to monitor if the UE received TA or not */
  if (ue_sched_ctl->ta_apply) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_TA_COMMAND;
    //last_size = 1;
    mac_pdu_ptr++;
    // TA MAC CE (1 octet)
    timing_advance_cmd = ue_sched_ctl->ta_update;
    AssertFatal(timing_advance_cmd < 64, "timing_advance_cmd %d > 63\n", timing_advance_cmd);
    ((NR_MAC_CE_TA *) ce_ptr)->TA_COMMAND = timing_advance_cmd;    //(timing_advance_cmd+31)&0x3f;

    if (gNB->tag->tag_Id != 0) {
      tag_id = gNB->tag->tag_Id;
      ((NR_MAC_CE_TA *) ce_ptr)->TAGID = tag_id;
    }

    LOG_D(NR_MAC, "NR MAC CE timing advance command = %d (%d) TAG ID = %d\n", timing_advance_cmd, ((NR_MAC_CE_TA *) ce_ptr)->TA_COMMAND, tag_id);
    mac_ce_size = sizeof(NR_MAC_CE_TA);
    // Copying  bytes for MAC CEs to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *) ce_ptr, mac_ce_size);
    ce_ptr += mac_ce_size;
    mac_pdu_ptr += (unsigned char) mac_ce_size;


  }

  // Contention resolution fixed subheader and MAC CE
  if (ue_cont_res_id) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_CON_RES_ID;
    mac_pdu_ptr++;
    //last_size = 1;
    // contention resolution identity MAC ce has a fixed 48 bit size
    // this contains the UL CCCH SDU. If UL CCCH SDU is longer than 48 bits,
    // it contains the first 48 bits of the UL CCCH SDU
    LOG_T(NR_MAC, "[gNB ][RAPROC] Generate contention resolution msg: %x.%x.%x.%x.%x.%x\n",
          ue_cont_res_id[0], ue_cont_res_id[1], ue_cont_res_id[2],
          ue_cont_res_id[3], ue_cont_res_id[4], ue_cont_res_id[5]);
    // Copying bytes (6 octects) to CEs pointer
    mac_ce_size = 6;
    memcpy(ce_ptr, ue_cont_res_id, mac_ce_size);
    // Copying bytes for MAC CEs to mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *) ce_ptr, mac_ce_size);
    ce_ptr += mac_ce_size;
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  //TS 38.321 Sec 6.1.3.15 TCI State indication for UE Specific PDCCH MAC CE SubPDU generation
  if (ue_sched_ctl->UE_mac_ce_ctrl.pdcch_state_ind.is_scheduled) {
    //filling subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_TCI_STATE_IND_UE_SPEC_PDCCH;
    mac_pdu_ptr++;
    //Creating the instance of CE structure
    NR_TCI_PDCCH  nr_UESpec_TCI_StateInd_PDCCH;
    //filling the CE structre
    nr_UESpec_TCI_StateInd_PDCCH.CoresetId1 = ((ue_sched_ctl->UE_mac_ce_ctrl.pdcch_state_ind.coresetId) & 0xF) >> 1; //extracting MSB 3 bits from LS nibble
    nr_UESpec_TCI_StateInd_PDCCH.ServingCellId = (ue_sched_ctl->UE_mac_ce_ctrl.pdcch_state_ind.servingCellId) & 0x1F; //extracting LSB 5 Bits
    nr_UESpec_TCI_StateInd_PDCCH.TciStateId = (ue_sched_ctl->UE_mac_ce_ctrl.pdcch_state_ind.tciStateId) & 0x7F; //extracting LSB 7 bits
    nr_UESpec_TCI_StateInd_PDCCH.CoresetId2 = (ue_sched_ctl->UE_mac_ce_ctrl.pdcch_state_ind.coresetId) & 0x1; //extracting LSB 1 bit
    LOG_D(NR_MAC, "NR MAC CE TCI state indication for UE Specific PDCCH = %d \n", nr_UESpec_TCI_StateInd_PDCCH.TciStateId);
    mac_ce_size = sizeof(NR_TCI_PDCCH);
    // Copying  bytes for MAC CEs to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *)&nr_UESpec_TCI_StateInd_PDCCH, mac_ce_size);
    //incrementing the PDU pointer
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  //TS 38.321 Sec 6.1.3.16, SP CSI reporting on PUCCH Activation/Deactivation MAC CE
  if (ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.is_scheduled) {
    //filling the subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_SP_CSI_REP_PUCCH_ACT;
    mac_pdu_ptr++;
    //creating the instance of CE structure
    NR_PUCCH_CSI_REPORTING nr_PUCCH_CSI_reportingActDeact;
    //filling the CE structure
    nr_PUCCH_CSI_reportingActDeact.BWP_Id = (ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.bwpId) & 0x3; //extracting LSB 2 bibs
    nr_PUCCH_CSI_reportingActDeact.ServingCellId = (ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.servingCellId) & 0x1F; //extracting LSB 5 bits
    nr_PUCCH_CSI_reportingActDeact.S0 = ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.s0tos3_actDeact[0];
    nr_PUCCH_CSI_reportingActDeact.S1 = ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.s0tos3_actDeact[1];
    nr_PUCCH_CSI_reportingActDeact.S2 = ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.s0tos3_actDeact[2];
    nr_PUCCH_CSI_reportingActDeact.S3 = ue_sched_ctl->UE_mac_ce_ctrl.SP_CSI_reporting_pucch.s0tos3_actDeact[3];
    nr_PUCCH_CSI_reportingActDeact.R2 = 0;
    mac_ce_size = sizeof(NR_PUCCH_CSI_REPORTING);
    // Copying MAC CE data to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *)&nr_PUCCH_CSI_reportingActDeact, mac_ce_size);
    //incrementing the PDU pointer
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  //TS 38.321 Sec 6.1.3.14, TCI State activation/deactivation for UE Specific PDSCH MAC CE
  if (ue_sched_ctl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.is_scheduled) {
    //Computing the number of octects to be allocated for Flexible array member
    //of MAC CE structure
    uint8_t num_octects = (ue_sched_ctl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.highestTciStateActivated) / 8 + 1; //Calculating the number of octects for allocating the memory
    //filling the subheader
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH;
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t);
    last_size = 2;
    //Incrementing the PDU pointer
    mac_pdu_ptr += last_size;
    //allocating memory for CE Structure
    NR_TCI_PDSCH_APERIODIC_CSI *nr_UESpec_TCI_StateInd_PDSCH = (NR_TCI_PDSCH_APERIODIC_CSI *)malloc(sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t));
    //initializing to zero
    memset((void *)nr_UESpec_TCI_StateInd_PDSCH, 0, sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t));
    //filling the CE Structure
    nr_UESpec_TCI_StateInd_PDSCH->BWP_Id = (ue_sched_ctl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.bwpId) & 0x3; //extracting LSB 2 Bits
    nr_UESpec_TCI_StateInd_PDSCH->ServingCellId = (ue_sched_ctl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.servingCellId) & 0x1F; //extracting LSB 5 bits

    for(i = 0; i < (num_octects * 8); i++) {
      if(ue_sched_ctl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.tciStateActDeact[i])
        nr_UESpec_TCI_StateInd_PDSCH->T[i / 8] = nr_UESpec_TCI_StateInd_PDSCH->T[i / 8] | (1 << (i % 8));
    }

    mac_ce_size = sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t);
    //Copying  bytes for MAC CEs to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *)nr_UESpec_TCI_StateInd_PDSCH, mac_ce_size);
    //incrementing the mac pdu pointer
    mac_pdu_ptr += (unsigned char) mac_ce_size;
    //freeing the allocated memory
    free(nr_UESpec_TCI_StateInd_PDSCH);
  }

  //TS38.321 Sec 6.1.3.13 Aperiodic CSI Trigger State Subselection MAC CE
  if (ue_sched_ctl->UE_mac_ce_ctrl.aperi_CSI_trigger.is_scheduled) {
    //Computing the number of octects to be allocated for Flexible array member
    //of MAC CE structure
    uint8_t num_octects = (ue_sched_ctl->UE_mac_ce_ctrl.aperi_CSI_trigger.highestTriggerStateSelected) / 8 + 1; //Calculating the number of octects for allocating the memory
    //filling the subheader
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL;
    ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t);
    last_size = 2;
    //Incrementing the PDU pointer
    mac_pdu_ptr += last_size;
    //allocating memory for CE structure
    NR_TCI_PDSCH_APERIODIC_CSI *nr_Aperiodic_CSI_Trigger = (NR_TCI_PDSCH_APERIODIC_CSI *)malloc(sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t));
    //initializing to zero
    memset((void *)nr_Aperiodic_CSI_Trigger, 0, sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t));
    //filling the CE Structure
    nr_Aperiodic_CSI_Trigger->BWP_Id = (ue_sched_ctl->UE_mac_ce_ctrl.aperi_CSI_trigger.bwpId) & 0x3; //extracting LSB 2 bits
    nr_Aperiodic_CSI_Trigger->ServingCellId = (ue_sched_ctl->UE_mac_ce_ctrl.aperi_CSI_trigger.servingCellId) & 0x1F; //extracting LSB 5 bits
    nr_Aperiodic_CSI_Trigger->R = 0;

    for(i = 0; i < (num_octects * 8); i++) {
      if(ue_sched_ctl->UE_mac_ce_ctrl.aperi_CSI_trigger.triggerStateSelection[i])
        nr_Aperiodic_CSI_Trigger->T[i / 8] = nr_Aperiodic_CSI_Trigger->T[i / 8] | (1 << (i % 8));
    }

    mac_ce_size = sizeof(NR_TCI_PDSCH_APERIODIC_CSI) + num_octects * sizeof(uint8_t);
    // Copying  bytes for MAC CEs to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *)nr_Aperiodic_CSI_Trigger, mac_ce_size);
    //incrementing the mac pdu pointer
    mac_pdu_ptr += (unsigned char) mac_ce_size;
    //freeing the allocated memory
    free(nr_Aperiodic_CSI_Trigger);
  }

  if (ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.is_scheduled) {
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->LCID = DL_SCH_LCID_SP_ZP_CSI_RS_RES_SET_ACT;
    mac_pdu_ptr++;
    ((NR_MAC_CE_SP_ZP_CSI_RS_RES_SET *) mac_pdu_ptr)->A_D = ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.act_deact;
    ((NR_MAC_CE_SP_ZP_CSI_RS_RES_SET *) mac_pdu_ptr)->CELLID = ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.serv_cell_id & 0x1F; //5 bits
    ((NR_MAC_CE_SP_ZP_CSI_RS_RES_SET *) mac_pdu_ptr)->BWPID = ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.bwpid & 0x3; //2 bits
    ((NR_MAC_CE_SP_ZP_CSI_RS_RES_SET *) mac_pdu_ptr)->CSIRS_RSC_ID = ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.rsc_id & 0xF; //4 bits
    ((NR_MAC_CE_SP_ZP_CSI_RS_RES_SET *) mac_pdu_ptr)->R = 0;
    LOG_D(NR_MAC, "NR MAC CE of ZP CSIRS Serv cell ID = %d BWPID= %d Rsc set ID = %d\n", ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.serv_cell_id, ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.bwpid,
          ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.rsc_id);
    mac_ce_size = sizeof(NR_MAC_CE_SP_ZP_CSI_RS_RES_SET);
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  if (ue_sched_ctl->UE_mac_ce_ctrl.csi_im.is_scheduled) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT;
    mac_pdu_ptr++;
    CSI_RS_CSI_IM_ACT_DEACT_MAC_CE csi_rs_im_act_deact_ce;
    csi_rs_im_act_deact_ce.A_D = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.act_deact;
    csi_rs_im_act_deact_ce.SCID = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.serv_cellid & 0x3F;//gNB_PHY -> ssb_pdu.ssb_pdu_rel15.PhysCellId;
    csi_rs_im_act_deact_ce.BWP_ID = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.bwp_id;
    csi_rs_im_act_deact_ce.R1 = 0;
    csi_rs_im_act_deact_ce.IM = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.im;// IF set CSI IM Rsc id will presesent else CSI IM RSC ID is abscent
    csi_rs_im_act_deact_ce.SP_CSI_RSID = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.nzp_csi_rsc_id;

    if ( csi_rs_im_act_deact_ce.IM ) { //is_scheduled if IM is 1 else this field will not present
      csi_rs_im_act_deact_ce.R2 = 0;
      csi_rs_im_act_deact_ce.SP_CSI_IMID = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.csi_im_rsc_id;
      mac_ce_size = sizeof ( csi_rs_im_act_deact_ce ) - sizeof ( csi_rs_im_act_deact_ce.TCI_STATE );
    } else {
      mac_ce_size = sizeof ( csi_rs_im_act_deact_ce ) - sizeof ( csi_rs_im_act_deact_ce.TCI_STATE ) - 1;
    }

    memcpy ((void *) mac_pdu_ptr, (void *) & ( csi_rs_im_act_deact_ce), mac_ce_size);
    mac_pdu_ptr += (unsigned char) mac_ce_size;

    if (csi_rs_im_act_deact_ce.A_D ) { //Following IE is_scheduled only if A/D is 1
      mac_ce_size = sizeof ( struct TCI_S);

      for ( i = 0; i < ue_sched_ctl->UE_mac_ce_ctrl.csi_im.nb_tci_resource_set_id; i++) {
        csi_rs_im_act_deact_ce.TCI_STATE.R = 0;
        csi_rs_im_act_deact_ce.TCI_STATE.TCI_STATE_ID = ue_sched_ctl->UE_mac_ce_ctrl.csi_im.tci_state_id [i] & 0x7F;
        memcpy ((void *) mac_pdu_ptr, (void *) & (csi_rs_im_act_deact_ce.TCI_STATE), mac_ce_size);
        mac_pdu_ptr += (unsigned char) mac_ce_size;
      }
    }
  }

  // compute final offset
  offset = ((unsigned char *) mac_pdu_ptr - mac_pdu);
  //printf("Offset %d \n", ((unsigned char *) mac_pdu_ptr - mac_pdu));
  return offset;
}

#define BLER_UPDATE_FRAME 10
#define BLER_FILTER 0.9f
int get_mcs_from_bler(module_id_t mod_id, int CC_id, frame_t frame, sub_frame_t slot, int UE_id)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  const NR_ServingCellConfigCommon_t *scc = nrmac->common_channels[CC_id].ServingCellConfigCommon;
  const int n = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];

  NR_DL_bler_stats_t *bler_stats = &nrmac->UE_info.UE_sched_ctrl[UE_id].dl_bler_stats;
  /* first call: everything is zero. Initialize to sensible default */
  if (bler_stats->last_frame_slot == 0 && bler_stats->mcs == 0) {
    bler_stats->last_frame_slot = frame * n + slot;
    bler_stats->mcs = 9;
    bler_stats->bler = (nrmac->dl_bler_target_lower + nrmac->dl_bler_target_upper) / 2;
    bler_stats->rd2_bler = nrmac->dl_rd2_bler_threshold;
  }
  const int now = frame * n + slot;
  int diff = now - bler_stats->last_frame_slot;
  if (diff < 0) // wrap around
    diff += 1024 * n;

  const uint8_t old_mcs = bler_stats->mcs;
  const NR_mac_stats_t *stats = &nrmac->UE_info.mac_stats[UE_id];
  // TODO put back this condition when relevant
  /*const int dret3x = stats->dlsch_rounds[3] - bler_stats->dlsch_rounds[3];
  if (dret3x > 0) {
     if there is a third retransmission, decrease MCS for stabilization and
     restart averaging window to stabilize transmission 
    bler_stats->last_frame_slot = now;
    bler_stats->mcs = max(9, bler_stats->mcs - 1);
    memcpy(bler_stats->dlsch_rounds, stats->dlsch_rounds, sizeof(stats->dlsch_rounds));
    LOG_D(MAC, "%4d.%2d: %d retx in 3rd round, setting MCS to %d and restarting window\n", frame, slot, dret3x, bler_stats->mcs);
    return bler_stats->mcs;
  }*/
  if (diff < BLER_UPDATE_FRAME * n)
    return old_mcs; // no update

  // last update is longer than x frames ago
  const int dtx = stats->dlsch_rounds[0] - bler_stats->dlsch_rounds[0];
  const int dretx = stats->dlsch_rounds[1] - bler_stats->dlsch_rounds[1];
  const int dretx2 = stats->dlsch_rounds[2] - bler_stats->dlsch_rounds[2];
  const float bler_window = dtx > 0 ? (float) dretx / dtx : bler_stats->bler;
  const float rd2_bler_wnd = dtx > 0 ? (float) dretx2 / dtx : bler_stats->rd2_bler;
  bler_stats->bler = BLER_FILTER * bler_stats->bler + (1 - BLER_FILTER) * bler_window;
  bler_stats->rd2_bler = BLER_FILTER / 4 * bler_stats->rd2_bler + (1 - BLER_FILTER / 4) * rd2_bler_wnd;

  int new_mcs = old_mcs;
  // TODO put back this condition when relevant
  /* first ensure that number of 2nd retx is below threshold. If this is the
   * case, use 1st retx to adjust faster 
  if (bler_stats->rd2_bler > nrmac->dl_rd2_bler_threshold && old_mcs > 6) {
    new_mcs -= 2;
  } else if (bler_stats->rd2_bler < nrmac->dl_rd2_bler_threshold) {*/
  if (bler_stats->bler < nrmac->dl_bler_target_lower && old_mcs < nrmac->dl_max_mcs && dtx > 9)
    new_mcs += 1;
  else if (bler_stats->bler > nrmac->dl_bler_target_upper && old_mcs > 6)
    new_mcs -= 1;
  // else we are within threshold boundaries

  bler_stats->last_frame_slot = now;
  bler_stats->mcs = new_mcs;
  memcpy(bler_stats->dlsch_rounds, stats->dlsch_rounds, sizeof(stats->dlsch_rounds));
  LOG_D(MAC, "%4d.%2d MCS %d -> %d (dtx %d, dretx %d, BLER wnd %.3f avg %.6f, dretx2 %d, RD2 BLER wnd %.3f avg %.6f)\n",
        frame, slot, old_mcs, new_mcs, dtx, dretx, bler_window, bler_stats->bler, dretx2, rd2_bler_wnd, bler_stats->rd2_bler);
  return new_mcs;
}

void nr_store_dlsch_buffer(module_id_t module_id,
                           frame_t frame,
                           sub_frame_t slot) {

  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    sched_ctrl->num_total_bytes = 0;

    int lcid; 
    const uint16_t rnti = UE_info->rnti[UE_id];
    LOG_D(NR_MAC,"UE %d/%x : lcid_mask %x\n",UE_id,rnti,sched_ctrl->lcid_mask);
    if ((sched_ctrl->lcid_mask&(1<<2)) > 0)
      sched_ctrl->rlc_status[DL_SCH_LCID_DCCH1] = mac_rlc_status_ind(module_id,
                                                        rnti,
                                                        module_id,
                                                          frame,
                                                        slot,
                                                        ENB_FLAG_YES,
                                                        MBMS_FLAG_NO,
                                                        DL_SCH_LCID_DCCH1,
                                                        0,
                                                        0);
    if ((sched_ctrl->lcid_mask&(1<<1)) > 0)
       sched_ctrl->rlc_status[DL_SCH_LCID_DCCH] = mac_rlc_status_ind(module_id,
                                                        rnti,
                                                        module_id,
                                                        frame,
                                                        slot,
                                                        ENB_FLAG_YES,
                                                        MBMS_FLAG_NO,
                                                        DL_SCH_LCID_DCCH,
                                                        0,
                                                        0);
    if ((sched_ctrl->lcid_mask&(1<<4)) > 0)  
       sched_ctrl->rlc_status[DL_SCH_LCID_DTCH] = mac_rlc_status_ind(module_id,
                                                                    rnti,
                                                                    module_id,
                                                                    frame,
                                                                    slot,
                                                                    ENB_FLAG_YES,
                                                                    MBMS_FLAG_NO,
                                                                    DL_SCH_LCID_DTCH,
                                                                    0,
                                                                    0);

     if(sched_ctrl->rlc_status[DL_SCH_LCID_DCCH].bytes_in_buffer > 0){
       lcid = DL_SCH_LCID_DCCH;       
     } 
     else if (sched_ctrl->rlc_status[DL_SCH_LCID_DCCH1].bytes_in_buffer > 0)
     {
       lcid = DL_SCH_LCID_DCCH1;       
     }else{
       lcid = DL_SCH_LCID_DTCH;       
     }
                                                      
    sched_ctrl->num_total_bytes += sched_ctrl->rlc_status[lcid].bytes_in_buffer;
    //later multiplex here. Just select DCCH/SRB before DTCH/DRB
    sched_ctrl->lcid_to_schedule = lcid;

    if (sched_ctrl->num_total_bytes == 0
        && !sched_ctrl->ta_apply) /* If TA should be applied, give at least one RB */
      continue;

    LOG_D(NR_MAC,
          "[%s][%d.%d], %s%d->DLSCH, RLC status %d bytes TA %d\n",
          __func__,
          frame,
          slot,
          lcid<4?"DCCH":"DTCH",
          lcid,
          sched_ctrl->rlc_status[lcid].bytes_in_buffer,
          sched_ctrl->ta_apply);
  }
}

bool allocate_dl_retransmission(module_id_t module_id,
                                frame_t frame,
                                sub_frame_t slot,
                                uint8_t *rballoc_mask,
                                int *n_rb_sched,
                                int UE_id,
                                int current_harq_pid) {

  const NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels->ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  NR_sched_pdsch_t *retInfo = &sched_ctrl->harq_processes[current_harq_pid].sched_pdsch;
  NR_CellGroupConfig_t *cg = UE_info->CellGroup[UE_id];
  NR_BWP_DownlinkDedicated_t *bwpd= cg ? cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP:NULL;


  NR_BWP_t *genericParameters = sched_ctrl->active_bwp ?
                                &sched_ctrl->active_bwp->bwp_Common->genericParameters :
                                &RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon->downlinkConfigCommon->initialDownlinkBWP->genericParameters;

  const uint16_t bwpSize = NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
  int rbStart = 0; // start wrt BWPstart

  NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
  const long f = (sched_ctrl->active_bwp ||bwpd) ? sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats : 0;
  int rbSize = 0;
  const int tda = RC.nrmac[module_id]->preferred_dl_tda[sched_ctrl->active_bwp ? sched_ctrl->active_bwp->bwp_Id : 0][slot];
  AssertFatal(tda>=0,"Unable to find PDSCH time domain allocation in list\n");
  if (tda == retInfo->time_domain_allocation) {
    /* Check that there are enough resources for retransmission */
    while (rbSize < retInfo->rbSize) {
      rbStart += rbSize; /* last iteration rbSize was not enough, skip it */
      rbSize = 0;
      while (rbStart < bwpSize && !rballoc_mask[rbStart])
        rbStart++;
      if (rbStart >= bwpSize) {
        LOG_D(NR_MAC, "cannot allocate retransmission for UE %d/RNTI %04x: no resources\n", UE_id, UE_info->rnti[UE_id]);
        return false;
      }
      while (rbStart + rbSize < bwpSize && rballoc_mask[rbStart + rbSize] && rbSize < retInfo->rbSize)
        rbSize++;
    }
    /* check whether we need to switch the TDA allocation since the last
     * (re-)transmission */
    if (ps->time_domain_allocation != tda)
      nr_set_pdsch_semi_static(scc, cg, sched_ctrl->active_bwp, bwpd, tda, f, ps);
  } else {
    /* the retransmission will use a different time domain allocation, check
     * that we have enough resources */
    while (rbStart < bwpSize && !rballoc_mask[rbStart])
      rbStart++;
    while (rbStart + rbSize < bwpSize && rballoc_mask[rbStart + rbSize])
      rbSize++;
    NR_pdsch_semi_static_t temp_ps;
    temp_ps.nrOfLayers = 1;
    nr_set_pdsch_semi_static(scc, cg, sched_ctrl->active_bwp, bwpd, tda, f, &temp_ps);
    uint32_t new_tbs;
    uint16_t new_rbSize;
    bool success = nr_find_nb_rb(retInfo->Qm,
                                 retInfo->R,
                                 temp_ps.nrOfSymbols,
                                 temp_ps.N_PRB_DMRS * temp_ps.N_DMRS_SLOT,
                                 retInfo->tb_size,
                                 rbSize,
                                 &new_tbs,
                                 &new_rbSize);
    if (!success || new_tbs != retInfo->tb_size) {
      LOG_D(MAC, "%s(): new TBsize %d of new TDA does not match old TBS %d\n", __func__, new_tbs, retInfo->tb_size);
      return false; /* the maximum TBsize we might have is smaller than what we need */
    }
    /* we can allocate it. Overwrite the time_domain_allocation, the number
     * of RBs, and the new TB size. The rest is done below */
    retInfo->tb_size = new_tbs;
    retInfo->rbSize = new_rbSize;
    retInfo->time_domain_allocation = tda;
    sched_ctrl->pdsch_semi_static = temp_ps;
  }

  /* Find a free CCE */
  bool freeCCE = find_free_CCE(module_id, slot, UE_id);
  if (!freeCCE) {
    LOG_D(MAC, "%4d.%2d could not find CCE for DL DCI retransmission UE %d/RNTI %04x\n",
          frame, slot, UE_id, UE_info->rnti[UE_id]);
    return false;
  }

  /* Find PUCCH occasion: if it fails, undo CCE allocation (undoing PUCCH
   * allocation after CCE alloc fail would be more complex) */
  const int alloc = nr_acknack_scheduling(module_id, UE_id, frame, slot, -1, 0);
  if (alloc<0) {
    LOG_D(MAC,
          "%s(): could not find PUCCH for UE %d/%04x@%d.%d\n",
          __func__,
          UE_id,
          UE_info->rnti[UE_id],
          frame,
          slot);
    int cid = sched_ctrl->coreset->controlResourceSetId;
    UE_info->num_pdcch_cand[UE_id][cid]--;
    int *cce_list = RC.nrmac[module_id]->cce_list[sched_ctrl->active_bwp?sched_ctrl->active_bwp->bwp_Id:0][cid];
    for (int i = 0; i < sched_ctrl->aggregation_level; i++)
      cce_list[sched_ctrl->cce_index + i] = 0;
    return false;
  }

  /* just reuse from previous scheduling opportunity, set new start RB */
  sched_ctrl->sched_pdsch = *retInfo;
  sched_ctrl->sched_pdsch.rbStart = rbStart;

  sched_ctrl->sched_pdsch.pucch_allocation = alloc;

  /* retransmissions: directly allocate */
  *n_rb_sched -= sched_ctrl->sched_pdsch.rbSize;
  for (int rb = 0; rb < sched_ctrl->sched_pdsch.rbSize; rb++)
    rballoc_mask[rb + sched_ctrl->sched_pdsch.rbStart] = 0;
  return true;
}

float thr_ue[MAX_MOBILES_PER_GNB];
uint32_t pf_tbs[3][29]; // pre-computed, approximate TBS values for PF coefficient

void pf_dl(module_id_t module_id,
           frame_t frame,
           sub_frame_t slot,
           NR_list_t *UE_list,
           int max_num_ue,
           int n_rb_sched,
           uint8_t *rballoc_mask) {

  gNB_MAC_INST *mac = RC.nrmac[module_id];
  NR_UE_info_t *UE_info = &mac->UE_info;
  NR_ServingCellConfigCommon_t *scc=mac->common_channels[0].ServingCellConfigCommon;
  float coeff_ue[MAX_MOBILES_PER_GNB];
  // UEs that could be scheduled
  int ue_array[MAX_MOBILES_PER_GNB];
  NR_list_t UE_sched = { .head = -1, .next = ue_array, .tail = -1, .len = MAX_MOBILES_PER_GNB };

  /* Loop UE_info->list to check retransmission */
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    if (UE_info->Msg4_ACKed[UE_id] != true) continue;
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    if (sched_ctrl->ul_failure==1 && get_softmodem_params()->phy_test==0) continue;
    NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
    NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
    /* get the PID of a HARQ process awaiting retrnasmission, or -1 otherwise */
    sched_pdsch->dl_harq_pid = sched_ctrl->retrans_dl_harq.head;

    /* Calculate Throughput */
    const float a = 0.0005f; // corresponds to 200ms window
    const uint32_t b = UE_info->mac_stats[UE_id].dlsch_current_bytes;
    thr_ue[UE_id] = (1 - a) * thr_ue[UE_id] + a * b;

    /* retransmission */
    if (sched_pdsch->dl_harq_pid >= 0) {
      /* Allocate retransmission */
      bool r = allocate_dl_retransmission(
          module_id, frame, slot, rballoc_mask, &n_rb_sched, UE_id, sched_pdsch->dl_harq_pid);
      if (!r) {
        LOG_D(NR_MAC, "%4d.%2d retransmission can NOT be allocated\n", frame, slot);
        continue;
      }
      /* reduce max_num_ue once we are sure UE can be allocated, i.e., has CCE */
      max_num_ue--;
      if (max_num_ue < 0) return;
    } else {
      /* Check DL buffer and skip this UE if no bytes and no TA necessary */
      if (sched_ctrl->num_total_bytes == 0 && frame != (sched_ctrl->ta_frame + 10) % 1024)
        continue;

      /* Calculate coeff */
      ps->nrOfLayers = 1;
      sched_pdsch->mcs = get_mcs_from_bler(module_id, /* CC_id = */ 0, frame, slot, UE_id);
      uint32_t tbs = pf_tbs[ps->mcsTableIdx][sched_pdsch->mcs];
      coeff_ue[UE_id] = (float) tbs / thr_ue[UE_id];
      LOG_D(NR_MAC,"b %d, thr_ue[%d] %f, tbs %d, coeff_ue[%d] %f\n",
            b, UE_id, thr_ue[UE_id], tbs, UE_id, coeff_ue[UE_id]);
      /* Create UE_sched list for UEs eligible for new transmission*/
      add_tail_nr_list(&UE_sched, UE_id);
    }
  }

  /* Loop UE_sched to find max coeff and allocate transmission */
  while (max_num_ue > 0 && n_rb_sched > 0 && UE_sched.head >= 0) {

    /* Find max coeff from UE_sched*/
    int *max = &UE_sched.head; /* assume head is max */
    int *p = &UE_sched.next[*max];
    while (*p >= 0) {
      /* if the current one has larger coeff, save for later */
      if (coeff_ue[*p] > coeff_ue[*max])
        max = p;
      p = &UE_sched.next[*p];
    }
    /* remove the max one: do not use remove_nr_list() it goes through the
     * whole list every time. Note that UE_sched.tail might not be set
     * correctly anymore */
    const int UE_id = *max;
    p = &UE_sched.next[*max];
    *max = UE_sched.next[*max];
    *p = -1;
    NR_CellGroupConfig_t *cg = UE_info->CellGroup[UE_id];
    NR_BWP_DownlinkDedicated_t *bwpd= cg ? cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP:NULL;

    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    int bwp_Id = sched_ctrl->active_bwp ? sched_ctrl->active_bwp->bwp_Id : 0;
    const uint16_t rnti = UE_info->rnti[UE_id];
    NR_BWP_t *genericParameters = sched_ctrl->active_bwp ?
      &sched_ctrl->active_bwp->bwp_Common->genericParameters:
      &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;

    const uint16_t bwpSize = NRRIV2BW(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);
    int rbStart = 0; // start wrt BWPstart

    /* Find a free CCE */
    bool freeCCE = find_free_CCE(module_id, slot, UE_id);
    if (!freeCCE) {
      LOG_D(NR_MAC, "%4d.%2d could not find CCE for DL DCI UE %d/RNTI %04x\n", frame, slot, UE_id, rnti);
      continue;
    }
    /* reduce max_num_ue once we are sure UE can be allocated, i.e., has CCE */
    max_num_ue--;
    if (max_num_ue < 0) return;

    /* Find PUCCH occasion: if it fails, undo CCE allocation (undoing PUCCH
    * allocation after CCE alloc fail would be more complex) */
    const int alloc = nr_acknack_scheduling(module_id, UE_id, frame, slot, -1,0);
    if (alloc<0) {
      LOG_D(NR_MAC,
            "%s(): could not find PUCCH for UE %d/%04x@%d.%d\n",
            __func__,
            UE_id,
            rnti,
            frame,
            slot);
      int cid = sched_ctrl->coreset->controlResourceSetId;
      UE_info->num_pdcch_cand[UE_id][cid]--;
      int *cce_list = mac->cce_list[bwp_Id][cid];
      for (int i = 0; i < sched_ctrl->aggregation_level; i++)
        cce_list[sched_ctrl->cce_index + i] = 0;
      return;
    }

    // Freq-demain allocation
    while (rbStart < bwpSize && !rballoc_mask[rbStart]) rbStart++;

    uint16_t max_rbSize = 1;
    while (rbStart + max_rbSize < bwpSize && rballoc_mask[rbStart + max_rbSize])
      max_rbSize++;

    /* MCS has been set above */

    const int tda = RC.nrmac[module_id]->preferred_dl_tda[sched_ctrl->active_bwp ? sched_ctrl->active_bwp->bwp_Id : 0][slot];
    AssertFatal(tda>=0,"Unable to find PDSCH time domain allocation in list\n");
    NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
    NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
    const long f = (sched_ctrl->active_bwp || bwpd) ? sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats : 0;
    if (ps->time_domain_allocation != tda)
      nr_set_pdsch_semi_static(scc, UE_info->CellGroup[UE_id], sched_ctrl->active_bwp, bwpd, tda, f, ps);
    sched_pdsch->Qm = nr_get_Qm_dl(sched_pdsch->mcs, ps->mcsTableIdx);
    sched_pdsch->R = nr_get_code_rate_dl(sched_pdsch->mcs, ps->mcsTableIdx);
    sched_pdsch->pucch_allocation = alloc;
    uint32_t TBS = 0;
    uint16_t rbSize;
    const int oh = 3 + 2 * (frame == (sched_ctrl->ta_frame + 10) % 1024);
    nr_find_nb_rb(sched_pdsch->Qm,
                  sched_pdsch->R,
                  ps->nrOfSymbols,
                  ps->N_PRB_DMRS * ps->N_DMRS_SLOT,
                  sched_ctrl->num_total_bytes + oh,
                  max_rbSize,
                  &TBS,
                  &rbSize);
    sched_pdsch->rbSize = rbSize;
    sched_pdsch->rbStart = rbStart;
    sched_pdsch->tb_size = TBS;

    /* transmissions: directly allocate */
    n_rb_sched -= sched_pdsch->rbSize;
    for (int rb = 0; rb < sched_pdsch->rbSize; rb++)
      rballoc_mask[rb + sched_pdsch->rbStart] = 0;
  }
}

void nr_fr1_dlsch_preprocessor(module_id_t module_id, frame_t frame, sub_frame_t slot)
{
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  if (UE_info->num_UEs == 0)
    return;

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;
  const int CC_id = 0;

  /* Get bwpSize from the first UE */
  int UE_id = UE_info->list.head;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  const uint16_t bwpSize = NRRIV2BW(sched_ctrl->active_bwp ?
				    sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth:
				    scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,
				    MAX_BWP_SIZE);
  const uint16_t BWPStart = NRRIV2PRBOFFSET(sched_ctrl->active_bwp ?
				            sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth:
				            scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,
				            MAX_BWP_SIZE);

  uint16_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;
  uint8_t rballoc_mask[bwpSize];
  int n_rb_sched = 0;
  for (int i = 0; i < bwpSize; i++) {
    // calculate mask: init with "NOT" vrb_map:
    // if any RB in vrb_map is blocked (1), the current RBG will be 0
    rballoc_mask[i] = !vrb_map[i+BWPStart];
    n_rb_sched += rballoc_mask[i];
  }

  /* Retrieve amount of data to send for this UE */
  nr_store_dlsch_buffer(module_id, frame, slot);

  /* proportional fair scheduling algorithm */
  pf_dl(module_id,
        frame,
        slot,
        &UE_info->list,
        2,
        n_rb_sched,
        rballoc_mask);
}

nr_pp_impl_dl nr_init_fr1_dlsch_preprocessor(module_id_t module_id, int CC_id)
{
  /* in the PF algorithm, we have to use the TBsize to compute the coefficient.
   * This would include the number of DMRS symbols, which in turn depends on
   * the time domain allocation. In case we are in a mixed slot, we do not want
   * to recalculate all these values just, and therefore we provide a look-up
   * table which should approximately give us the TBsize */
  for (int mcsTableIdx = 0; mcsTableIdx < 3; ++mcsTableIdx) {
    for (int mcs = 0; mcs < 29; ++mcs) {
      if (mcs > 27 && mcsTableIdx == 1)
        continue;
      const uint8_t Qm = nr_get_Qm_dl(mcs, mcsTableIdx);
      const uint16_t R = nr_get_code_rate_dl(mcs, mcsTableIdx);
      pf_tbs[mcsTableIdx][mcs] = nr_compute_tbs(Qm,
                                                R,
                                                1, /* rbSize */
                                                10, /* hypothetical number of slots */
                                                0, /* N_PRB_DMRS * N_DMRS_SLOT */
                                                0 /* N_PRB_oh, 0 for initialBWP */,
                                                0 /* tb_scaling */,
                                                1 /* nrOfLayers */)
                                 >> 3;
    }
  }

  return nr_fr1_dlsch_preprocessor;
}

void nr_schedule_ue_spec(module_id_t module_id,
                         frame_t frame,
                         sub_frame_t slot) {
  gNB_MAC_INST *gNB_mac = RC.nrmac[module_id];
  if (!is_xlsch_in_slot(gNB_mac->dlsch_slot_bitmap[slot / 64], slot))
    return;

  /* PREPROCESSOR */
  //Following commented section condition should be removed before update with develop.
  /*if(slot!= 1 && slot!=11){
    return;
  }*/
  gNB_mac->pre_processor_dl(module_id, frame, slot);

  const int CC_id = 0;
  NR_ServingCellConfigCommon_t *scc = gNB_mac->common_channels[CC_id].ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &gNB_mac->UE_info;

  nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;

  NR_list_t *UE_list = &UE_info->list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    if (sched_ctrl->ul_failure==1 && get_softmodem_params()->phy_test==0) continue;
    NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
    UE_info->mac_stats[UE_id].dlsch_current_bytes = 0;
    NR_CellGroupConfig_t *cg = UE_info->CellGroup[UE_id];
    NR_BWP_DownlinkDedicated_t *bwpd= cg ? cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP:NULL;

    /* update TA and set ta_apply every 10 frames.
     * Possible improvement: take the periodicity from input file.
     * If such UE is not scheduled now, it will be by the preprocessor later.
     * If we add the CE, ta_apply will be reset */
    if (frame == (sched_ctrl->ta_frame + 10) % 1024){
      sched_ctrl->ta_apply = true; /* the timer is reset once TA CE is scheduled */
      LOG_D(NR_MAC, "[UE %d][%d.%d] UL timing alignment procedures: setting flag for Timing Advance command\n", UE_id, frame, slot);
    }

    if (sched_pdsch->rbSize <= 0)
      continue;

    const rnti_t rnti = UE_info->rnti[UE_id];

    /* pre-computed PDSCH values that only change if time domain
     * allocation/DMRS parameters change. Updated in the preprocessor through
     * nr_set_pdsch_semi_static() */
    NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;

    /* POST processing */
    const uint8_t nrOfLayers = ps->nrOfLayers;
    const uint16_t R = sched_pdsch->R;
    const uint8_t Qm = sched_pdsch->Qm;
    const uint32_t TBS = sched_pdsch->tb_size;

    int8_t current_harq_pid = sched_pdsch->dl_harq_pid;
    if (current_harq_pid < 0) {
      /* PP has not selected a specific HARQ Process, get a new one */
      current_harq_pid = sched_ctrl->available_dl_harq.head;
      AssertFatal(current_harq_pid >= 0,
                  "no free HARQ process available for UE %d\n",
                  UE_id);
      remove_front_nr_list(&sched_ctrl->available_dl_harq);
      sched_pdsch->dl_harq_pid = current_harq_pid;
    } else {
      /* PP selected a specific HARQ process. Check whether it will be a new
       * transmission or a retransmission, and remove from the corresponding
       * list */
      if (sched_ctrl->harq_processes[current_harq_pid].round == 0)
        remove_nr_list(&sched_ctrl->available_dl_harq, current_harq_pid);
      else
        remove_nr_list(&sched_ctrl->retrans_dl_harq, current_harq_pid);
    }
    NR_UE_harq_t *harq = &sched_ctrl->harq_processes[current_harq_pid];
    DevAssert(!harq->is_waiting);
    add_tail_nr_list(&sched_ctrl->feedback_dl_harq, current_harq_pid);
    NR_sched_pucch_t *pucch = &sched_ctrl->sched_pucch[sched_pdsch->pucch_allocation];
    harq->feedback_frame = pucch->frame;
    harq->feedback_slot = pucch->ul_slot;
    harq->is_waiting = true;
    UE_info->mac_stats[UE_id].dlsch_rounds[harq->round]++;

    LOG_D(NR_MAC,
          "%4d.%2d [DLSCH/PDSCH/PUCCH] UE %d RNTI %04x DCI L %d start %3d RBs %3d startSymbol %2d nb_symbol %2d dmrspos %x MCS %2d TBS %4d HARQ PID %2d round %d RV %d NDI %d dl_data_to_ULACK %d (%d.%d) PUCCH allocation %d TPC %d\n",
          frame,
          slot,
          UE_id,
          rnti,
          sched_ctrl->aggregation_level,
          sched_pdsch->rbStart,
          sched_pdsch->rbSize,
          ps->startSymbolIndex,
          ps->nrOfSymbols,
          ps->dl_dmrs_symb_pos,
          sched_pdsch->mcs,
          TBS,
          current_harq_pid,
          harq->round,
          nr_rv_round_map[harq->round],
          harq->ndi,
          pucch->timing_indicator,
          pucch->frame,
          pucch->ul_slot,
          sched_pdsch->pucch_allocation,
          sched_ctrl->tpc1);

    NR_BWP_Downlink_t *bwp = sched_ctrl->active_bwp;

    /* look up the PDCCH PDU for this CC, BWP, and CORESET. If it does not
     * exist, create it */

    // BWP
    NR_BWP_t *genericParameters = bwp ? &bwp->bwp_Common->genericParameters : &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;

    const int bwpid = bwp ? bwp->bwp_Id : 0;
    const int coresetid = (bwp||bwpd) ? sched_ctrl->coreset->controlResourceSetId : gNB_mac->sched_ctrlCommon->coreset->controlResourceSetId;
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu = gNB_mac->pdcch_pdu_idx[CC_id][bwpid][coresetid];
    if (!pdcch_pdu) {
      nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
      memset(dl_tti_pdcch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
      dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      dl_req->nPDUs += 1;
      pdcch_pdu = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
      LOG_D(NR_MAC,"Trying to configure DL pdcch for UE %d, bwp %d, cs %d\n",UE_id,bwpid,coresetid);
      NR_SearchSpace_t *ss = (bwp||bwpd) ? sched_ctrl->search_space:gNB_mac->sched_ctrlCommon->search_space;
      NR_ControlResourceSet_t *coreset = (bwp||bwpd)? sched_ctrl->coreset:gNB_mac->sched_ctrlCommon->coreset;
      nr_configure_pdcch(gNB_mac, pdcch_pdu, ss, coreset, scc, genericParameters, NULL);
      gNB_mac->pdcch_pdu_idx[CC_id][bwpid][coresetid] = pdcch_pdu;
    }

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset(dl_tti_pdsch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));
    dl_req->nPDUs += 1;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

    pdsch_pdu->pduBitmap = 0;
    pdsch_pdu->rnti = rnti;
    /* SCF222: PDU index incremented for each PDSCH PDU sent in TX control
     * message. This is used to associate control information to data and is
     * reset every slot. */
    const int pduindex = gNB_mac->pdu_index[CC_id]++;
    pdsch_pdu->pduIndex = pduindex;

    pdsch_pdu->BWPSize  = NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
    pdsch_pdu->BWPStart = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);
    pdsch_pdu->SubcarrierSpacing = genericParameters->subcarrierSpacing;

    pdsch_pdu->CyclicPrefix = genericParameters->cyclicPrefix ? *genericParameters->cyclicPrefix : 0;

    // Codeword information
    pdsch_pdu->NrOfCodewords = 1;
    pdsch_pdu->targetCodeRate[0] = R;
    pdsch_pdu->qamModOrder[0] = Qm;
    pdsch_pdu->mcsIndex[0] = sched_pdsch->mcs;
    pdsch_pdu->mcsTable[0] = ps->mcsTableIdx;
    AssertFatal(harq!=NULL,"harq is null\n");
    AssertFatal(harq->round<4,"%d",harq->round);
    pdsch_pdu->rvIndex[0] = nr_rv_round_map[harq->round];
    pdsch_pdu->TBSize[0] = TBS;

    pdsch_pdu->dataScramblingId = *scc->physCellId;
    pdsch_pdu->nrOfLayers = nrOfLayers;
    pdsch_pdu->transmissionScheme = 0;
    pdsch_pdu->refPoint = 0; // Point A

    // DMRS
    pdsch_pdu->dlDmrsSymbPos = ps->dl_dmrs_symb_pos;
    pdsch_pdu->dmrsConfigType = ps->dmrsConfigType;
    pdsch_pdu->dlDmrsScramblingId = *scc->physCellId;
    pdsch_pdu->SCID = 0;
    pdsch_pdu->numDmrsCdmGrpsNoData = ps->numDmrsCdmGrpsNoData;
    pdsch_pdu->dmrsPorts = (1<<nrOfLayers)-1;  // FIXME with a better implementation

    // Pdsch Allocation in frequency domain
    pdsch_pdu->resourceAlloc = 1;
    pdsch_pdu->rbStart = sched_pdsch->rbStart;
    pdsch_pdu->rbSize = sched_pdsch->rbSize;
    pdsch_pdu->VRBtoPRBMapping = 1; // non-interleaved, check if this is ok for initialBWP

    // Resource Allocation in time domain
    pdsch_pdu->StartSymbolIndex = ps->startSymbolIndex;
    pdsch_pdu->NrOfSymbols = ps->nrOfSymbols;

    NR_PDSCH_Config_t *pdsch_Config=NULL;
    if (bwp &&
        bwp->bwp_Dedicated &&
        bwp->bwp_Dedicated->pdsch_Config &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup)
      pdsch_Config =  bwp->bwp_Dedicated->pdsch_Config->choice.setup;

    /* Check and validate PTRS values */
    struct NR_SetupRelease_PTRS_DownlinkConfig *phaseTrackingRS =
      pdsch_Config ? pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS : NULL;
    if (phaseTrackingRS) {
      bool valid_ptrs_setup = set_dl_ptrs_values(phaseTrackingRS->choice.setup,
                                                 pdsch_pdu->rbSize,
                                                 pdsch_pdu->mcsIndex[0],
                                                 pdsch_pdu->mcsTable[0],
                                                 &pdsch_pdu->PTRSFreqDensity,
                                                 &pdsch_pdu->PTRSTimeDensity,
                                                 &pdsch_pdu->PTRSPortIndex,
                                                 &pdsch_pdu->nEpreRatioOfPDSCHToPTRS,
                                                 &pdsch_pdu->PTRSReOffset,
                                                 pdsch_pdu->NrOfSymbols);
      if (valid_ptrs_setup)
        pdsch_pdu->pduBitmap |= 0x1; // Bit 0: pdschPtrs - Indicates PTRS included (FR2)
    }

    /* Fill PDCCH DL DCI PDU */
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu->dci_pdu[pdcch_pdu->numDlDci];
    pdcch_pdu->numDlDci++;
    dci_pdu->RNTI = rnti;
    if (sched_ctrl->coreset &&
        sched_ctrl->search_space &&
        sched_ctrl->coreset->pdcch_DMRS_ScramblingID &&
        sched_ctrl->search_space->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) {
      dci_pdu->ScramblingId = *sched_ctrl->coreset->pdcch_DMRS_ScramblingID;
      dci_pdu->ScramblingRNTI = rnti;
    } else {
      dci_pdu->ScramblingId = *scc->physCellId;
      dci_pdu->ScramblingRNTI = 0;
    }
    dci_pdu->AggregationLevel = sched_ctrl->aggregation_level;
    dci_pdu->CceIndex = sched_ctrl->cce_index;
    dci_pdu->beta_PDCCH_1_0 = 0;
    dci_pdu->powerControlOffsetSS = 1;

    /* DCI payload */
    dci_pdu_rel15_t dci_payload;
    memset(&dci_payload, 0, sizeof(dci_pdu_rel15_t));
    // bwp indicator
    const int n_dl_bwp = bwp ? UE_info->CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count : 0;
    AssertFatal(n_dl_bwp <= 1, "downlinkBWP_ToAddModList has %d BWP!\n", n_dl_bwp);

    // as per table 7.3.1.1.2-1 in 38.212
    dci_payload.bwp_indicator.val = bwp ? (n_dl_bwp < 4 ? bwp->bwp_Id : bwp->bwp_Id - 1) : 0;
    if (bwp) AssertFatal(bwp->bwp_Dedicated->pdsch_Config->choice.setup->resourceAllocation == NR_PDSCH_Config__resourceAllocation_resourceAllocationType1,
			 "Only frequency resource allocation type 1 is currently supported\n");
    dci_payload.frequency_domain_assignment.val =
        PRBalloc_to_locationandbandwidth0(
            pdsch_pdu->rbSize,
            pdsch_pdu->rbStart,
            pdsch_pdu->BWPSize);
    dci_payload.format_indicator = 1;
    dci_payload.time_domain_assignment.val = ps->time_domain_allocation;
    dci_payload.mcs = sched_pdsch->mcs;
    dci_payload.rv = pdsch_pdu->rvIndex[0];
    dci_payload.harq_pid = current_harq_pid;
    dci_payload.ndi = harq->ndi;
    dci_payload.dai[0].val = (pucch->dai_c-1)&3;
    dci_payload.tpc = sched_ctrl->tpc1; // TPC for PUCCH: table 7.2.1-1 in 38.213
    dci_payload.pucch_resource_indicator = pucch->resource_indicator;
    dci_payload.pdsch_to_harq_feedback_timing_indicator.val = pucch->timing_indicator; // PDSCH to HARQ TI
    dci_payload.antenna_ports.val = ps->dmrs_ports_id;
    dci_payload.dmrs_sequence_initialization.val = pdsch_pdu->SCID;
    LOG_D(NR_MAC,
          "%4d.%2d DCI type 1 payload: freq_alloc %d (%d,%d,%d), "
          "time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d tpc %d ti %d\n",
          frame,
          slot,
          dci_payload.frequency_domain_assignment.val,
          pdsch_pdu->rbStart,
          pdsch_pdu->rbSize,
          pdsch_pdu->BWPSize,
          dci_payload.time_domain_assignment.val,
          dci_payload.vrb_to_prb_mapping.val,
          dci_payload.mcs,
          dci_payload.tb_scaling,
          dci_payload.ndi,
          dci_payload.rv,
          dci_payload.tpc,
          pucch->timing_indicator);

    const long f = sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats;
    int dci_format;
    if (sched_ctrl->search_space) {
       dci_format = f ? NR_DL_DCI_FORMAT_1_1 : NR_DL_DCI_FORMAT_1_0;
    }
    else {
       dci_format = NR_DL_DCI_FORMAT_1_0;
    }
    const int rnti_type = NR_RNTI_C;

    fill_dci_pdu_rel15(scc,
                       UE_info->CellGroup[UE_id],
                       dci_pdu,
                       &dci_payload,
                       dci_format,
                       rnti_type,
                       pdsch_pdu->BWPSize,
                       bwp? bwp->bwp_Id : 0);

    LOG_D(NR_MAC,
          "coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d\n",
          (unsigned long long)pdcch_pdu->FreqDomainResource,
          pdcch_pdu->StartSymbolIndex,
          pdcch_pdu->DurationSymbols);

    if (harq->round != 0) { /* retransmission */
      /* we do not have to do anything, since we do not require to get data
       * from RLC or encode MAC CEs. The TX_req structure is filled below 
       * or copy data to FAPI structures */
      LOG_D(NR_MAC,
            "%d.%2d DL retransmission UE %d/RNTI %04x HARQ PID %d round %d NDI %d\n",
            frame,
            slot,
            UE_id,
            rnti,
            current_harq_pid,
            harq->round,
            harq->ndi);

      AssertFatal(harq->sched_pdsch.tb_size == TBS,
                  "UE %d mismatch between scheduled TBS and buffered TB for HARQ PID %d\n",
                  UE_id,
                  current_harq_pid);
    } else { /* initial transmission */

      LOG_D(NR_MAC, "[%s] Initial HARQ transmission in %d.%d\n", __FUNCTION__, frame, slot);

      uint8_t *buf = (uint8_t *) harq->tb;

      /* first, write all CEs that might be there */
      int written = nr_write_ce_dlsch_pdu(module_id,
                                          sched_ctrl,
                                          (unsigned char *)buf,
                                          255, // no drx
                                          NULL); // contention res id
      buf += written;
      int size = TBS - written;
      DevAssert(size >= 0);

      /* next, get RLC data */

      // const int lcid = DL_SCH_LCID_DTCH;
      const int lcid = sched_ctrl->lcid_to_schedule;
      int dlsch_total_bytes = 0;
      start_meas(&gNB_mac->rlc_data_req);
      if (sched_ctrl->num_total_bytes > 0) {
        tbs_size_t len = 0;
        while (size > 3) {
          // we do not know how much data we will get from RLC, i.e., whether it
          // will be longer than 256B or not. Therefore, reserve space for long header, then
          // fetch data, then fill real length
          NR_MAC_SUBHEADER_LONG *header = (NR_MAC_SUBHEADER_LONG *) buf;
          buf += 3;
          size -= 3;

          /* limit requested number of bytes to what preprocessor specified, or
           * such that TBS is full */
          const rlc_buffer_occupancy_t ndata = min(sched_ctrl->rlc_status[lcid].bytes_in_buffer, size);


          len = mac_rlc_data_req(module_id,
                                 rnti,
                                 module_id,
                                 frame,
                                 ENB_FLAG_YES,
                                 MBMS_FLAG_NO,
                                 lcid,
                                 ndata,
                                 (char *)buf,
                                 0,
                                 0);


          LOG_D(NR_MAC,
                "%4d.%2d RNTI %04x: %d bytes from %s %d (ndata %d, remaining size %d)\n",
                frame,
                slot,
                rnti,
                len,
                lcid < 4 ? "DCCH" : "DTCH",
                lcid,
                ndata,
                size);
          if (len == 0)
            break;

          header->R = 0;
          header->F = 1;
          header->LCID = lcid;
          header->L1 = (len >> 8) & 0xff;
          header->L2 = len & 0xff;
          size -= len;
          buf += len;
          dlsch_total_bytes += len;
        }
        if (len == 0) {
          /* RLC did not have data anymore, mark buffer as unused */
          buf -= 3;
          size += 3;
        }
      }
      else if (get_softmodem_params()->phy_test || get_softmodem_params()->do_ra || get_softmodem_params()->sa) {
        /* we will need the large header, phy-test typically allocates all
         * resources and fills to the last byte below */
        NR_MAC_SUBHEADER_LONG *header = (NR_MAC_SUBHEADER_LONG *) buf;
        buf += 3;
        size -= 3;
        DevAssert(size > 0);
        LOG_D(NR_MAC, "Configuring DL_TX in %d.%d: TBS %d with %d B of random data\n", frame, slot, TBS, size);
        // fill dlsch_buffer with random data
        for (int i = 0; i < size; i++)
          buf[i] = lrand48() & 0xff;
        header->R = 0;
        header->F = 1;
        header->LCID = DL_SCH_LCID_PADDING;
        header->L1 = (size >> 8) & 0xff;
        header->L2 = size & 0xff;
        size -= size;
        buf += size;
        dlsch_total_bytes += size;
      }
      stop_meas(&gNB_mac->rlc_data_req);

      // Add padding header and zero rest out if there is space left
      if (size > 0) {
        NR_MAC_SUBHEADER_FIXED *padding = (NR_MAC_SUBHEADER_FIXED *) buf;
        padding->R = 0;
        padding->LCID = DL_SCH_LCID_PADDING;
        size -= 1;
        buf += 1;
        while (size > 0) {
          *buf = 0;
          buf += 1;
          size -= 1;
        }
      }

      UE_info->mac_stats[UE_id].dlsch_total_bytes += TBS;
      UE_info->mac_stats[UE_id].dlsch_current_bytes = TBS;
      UE_info->mac_stats[UE_id].lc_bytes_tx[lcid] += dlsch_total_bytes;

      /* save retransmission information */
      harq->sched_pdsch = *sched_pdsch;
      /* save which time allocation has been used, to be used on
       * retransmissions */
      harq->sched_pdsch.time_domain_allocation = ps->time_domain_allocation;

      // ta command is sent, values are reset
      if (sched_ctrl->ta_apply) {
        sched_ctrl->ta_apply = false;
        sched_ctrl->ta_frame = frame;
        LOG_D(NR_MAC,
              "%d.%2d UE %d TA scheduled, resetting TA frame\n",
              frame,
              slot,
              UE_id);
      }

      T(T_GNB_MAC_DL_PDU_WITH_DATA, T_INT(module_id), T_INT(CC_id), T_INT(rnti),
        T_INT(frame), T_INT(slot), T_INT(current_harq_pid), T_BUFFER(harq->tb, TBS));
    }

    const int ntx_req = gNB_mac->TX_req[CC_id].Number_of_PDUs;
    nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[ntx_req];
    tx_req->PDU_length = TBS;
    tx_req->PDU_index  = pduindex;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = TBS + 2;
    memcpy(tx_req->TLVs[0].value.direct, harq->tb, TBS);
    gNB_mac->TX_req[CC_id].Number_of_PDUs++;
    gNB_mac->TX_req[CC_id].SFN = frame;
    gNB_mac->TX_req[CC_id].Slot = slot;

    /* mark UE as scheduled */
    sched_pdsch->rbSize = 0;
  }
}
