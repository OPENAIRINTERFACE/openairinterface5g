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

/*PHY*/
#include "PHY/CODING/coding_defs.h"
#include "PHY/defs_nr_common.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
/*MAC*/
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"

/*NFAPI*/
#include "nfapi_nr_interface.h"
/*TAG*/
#include "NR_TAG-Id.h"

////////////////////////////////////////////////////////
/////* DLSCH MAC PDU generation (6.1.2 TS 38.321) */////
////////////////////////////////////////////////////////
#define OCTET 8
#define HALFWORD 16
#define WORD 32
//#define SIZE_OF_POINTER sizeof (void *)

int nr_generate_dlsch_pdu(module_id_t module_idP,
                          unsigned char *sdus_payload,
                          unsigned char *mac_pdu,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned char *ue_cont_res_id,
                          unsigned short post_padding) {
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_MAC_SUBHEADER_FIXED *mac_pdu_ptr = (NR_MAC_SUBHEADER_FIXED *) mac_pdu;
  unsigned char *dlsch_buffer_ptr = sdus_payload;
  uint8_t last_size = 0;
  int offset = 0, mac_ce_size, i, timing_advance_cmd, tag_id = 0;
  // MAC CEs
  uint8_t mac_header_control_elements[16], *ce_ptr;
  ce_ptr = &mac_header_control_elements[0];
  uint16_t UE_id = 0; //TODO need to get as a function parameter or need to invoke api to UE_id using module Id and RNTI
  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];
  NR_UE_list_t *UE_list = &gNB_mac->UE_list;
  NR_UE_sched_ctrl_t *ue_sched_ctl = NULL;
  //NR_CellGroupConfig_t *config = UE_list->secondaryCellGroup[UE_id];
  ue_sched_ctl = &(UE_list->UE_sched_ctrl[UE_id]);

  NR_mac_stats_t *mac_stats = &(UE_list->mac_stats[UE_id]);

  // 1) Compute MAC CE and related subheaders

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
  if (gNB->ta_len) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_TA_COMMAND;
    //last_size = 1;
    mac_pdu_ptr++;
    // TA MAC CE (1 octet)
    timing_advance_cmd = gNB->ta_command;
    AssertFatal(timing_advance_cmd < 64, "timing_advance_cmd %d > 63\n", timing_advance_cmd);
    ((NR_MAC_CE_TA *) ce_ptr)->TA_COMMAND = timing_advance_cmd;    //(timing_advance_cmd+31)&0x3f;

    if (gNB->tag->tag_Id != 0) {
      tag_id = gNB->tag->tag_Id;
      ((NR_MAC_CE_TA *) ce_ptr)->TAGID = tag_id;
    }

    LOG_D(MAC, "NR MAC CE timing advance command = %d (%d) TAG ID = %d\n", timing_advance_cmd, ((NR_MAC_CE_TA *) ce_ptr)->TA_COMMAND, tag_id);
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
    LOG_T(MAC, "[gNB ][RAPROC] Generate contention resolution msg: %x.%x.%x.%x.%x.%x\n",
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
    LOG_D(MAC, "NR MAC CE TCI state indication for UE Specific PDCCH = %d \n", nr_UESpec_TCI_StateInd_PDCCH.TciStateId);
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
    LOG_D(MAC, "NR MAC CE of ZP CSIRS Serv cell ID = %d BWPID= %d Rsc set ID = %d\n", ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.serv_cell_id, ue_sched_ctl->UE_mac_ce_ctrl.sp_zp_csi_rs.bwpid,
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

  // 2) Generation of DLSCH MAC subPDUs including subheaders and MAC SDUs
  for (i = 0; i < num_sdus; i++) {
    LOG_D(MAC, "[gNB] Generate DLSCH header num sdu %d len sdu %d\n", num_sdus, sdu_lengths[i]);

    if (sdu_lengths[i] < 128) {
      ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
      ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
      ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = sdu_lcids[i];
      ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = (unsigned char) sdu_lengths[i];
      last_size = 2;
    } else {
      ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->R = 0;
      ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->F = 1;
      ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->LCID = sdu_lcids[i];
      ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->L1 = ((unsigned short) sdu_lengths[i] >> 8) & 0x7f;
      ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->L2 = (unsigned short) sdu_lengths[i] & 0xff;
      last_size = 3;
    }

    mac_pdu_ptr += last_size;
    // 3) cycle through SDUs, compute each relevant and place dlsch_buffer in
    memcpy((void *) mac_pdu_ptr, (void *) dlsch_buffer_ptr, sdu_lengths[i]);
    dlsch_buffer_ptr += sdu_lengths[i];
    mac_pdu_ptr += sdu_lengths[i];

    mac_stats->lc_bytes_tx[sdu_lcids[i]] += sdu_lengths[i];
  }

  // 4) Compute final offset for padding
  if (post_padding > 0) {
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->LCID = DL_SCH_LCID_PADDING;
    mac_pdu_ptr++;
  } else {
    // no MAC subPDU with padding
  }

  // compute final offset
  offset = ((unsigned char *) mac_pdu_ptr - mac_pdu);
  //printf("Offset %d \n", ((unsigned char *) mac_pdu_ptr - mac_pdu));
  return offset;
}

void handle_nr_uci(NR_UL_IND_t *UL_info, NR_UE_sched_ctrl_t *sched_ctrl, NR_mac_stats_t *stats, int target_snrx10) {
  // TODO
  int max_harq_rounds = 4; // TODO define macro
  int num_ucis = UL_info->uci_ind.num_ucis;
  nfapi_nr_uci_t *uci_list = UL_info->uci_ind.uci_list;

  for (int i = 0; i < num_ucis; i++) {
    switch (uci_list[i].pdu_type) {
      case NFAPI_NR_UCI_PDCCH_PDU_TYPE: break;

      case NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE: {
        //if (get_softmodem_params()->phy_test == 0) {
          nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_pdu = &uci_list[i].pucch_pdu_format_0_1;
          // handle harq
          int harq_idx_s = 0;
          // tpc (power control)
          sched_ctrl->tpc1 = nr_get_tpc(target_snrx10,uci_pdu->ul_cqi,30);
          // iterate over received harq bits
          for (int harq_bit = 0; harq_bit < uci_pdu->harq->num_harq; harq_bit++) {
            // search for the right harq process
            for (int harq_idx = harq_idx_s; harq_idx < NR_MAX_NB_HARQ_PROCESSES; harq_idx++) {
              // if the gNB received ack with a good confidence
              if ((UL_info->slot-1) == sched_ctrl->harq_processes[harq_idx].feedback_slot) {
                if ((uci_pdu->harq->harq_list[harq_bit].harq_value == 1) &&
                    (uci_pdu->harq->harq_confidence_level == 0)) {
                  // toggle NDI and reset round
                  sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
                  sched_ctrl->harq_processes[harq_idx].round = 0;
                }
                else
                  sched_ctrl->harq_processes[harq_idx].round++;
                sched_ctrl->harq_processes[harq_idx].is_waiting = 0;
                harq_idx_s = harq_idx + 1;
                // if the max harq rounds was reached
                if (sched_ctrl->harq_processes[harq_idx].round == max_harq_rounds) {
                  sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
                  sched_ctrl->harq_processes[harq_idx].round = 0;
                  stats->dlsch_errors++;
                }
                break;
              }
              // if feedback slot processing is aborted
              else if (((UL_info->slot-1) > sched_ctrl->harq_processes[harq_idx].feedback_slot) &&
                      (sched_ctrl->harq_processes[harq_idx].is_waiting)) {
                sched_ctrl->harq_processes[harq_idx].round++;
                if (sched_ctrl->harq_processes[harq_idx].round == max_harq_rounds) {
                  sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
                  sched_ctrl->harq_processes[harq_idx].round = 0;
                }
                sched_ctrl->harq_processes[harq_idx].is_waiting = 0;
              }
            }
          }
        //}
        break;
      }

      case NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE: break;
    }
  }

  UL_info->uci_ind.num_ucis = 0;
}

void nr_schedule_ue_spec(module_id_t module_id,
                         frame_t frame,
                         sub_frame_t slot,
                         int num_slots_per_tdd) {
  gNB_MAC_INST *gNB_mac = RC.nrmac[module_id];
  const int ta_len = gNB_mac->ta_len;
  const int UE_id = 0;
  const int CC_id = 0;
  const int bwp_id = 1;

  nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;
  nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[gNB_mac->TX_req[CC_id].Number_of_PDUs];

  NR_UE_list_t *UE_list = &gNB_mac->UE_list;

  if (UE_list->num_UEs ==0) return;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
              "downlinkBWP_ToAddModList has %d BWP!\n",
              secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
  NR_BWP_Downlink_t *bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id - 1];
  const uint16_t bwpSize = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);

  unsigned char sdu_lcids[NB_RB_MAX] = {0};
  uint16_t sdu_lengths[NB_RB_MAX] = {0};
  const uint16_t rnti = UE_list->rnti[UE_id];

  uint8_t mac_sdus[MAX_NR_DLSCH_PAYLOAD_BYTES];

  LOG_D(MAC, "Scheduling UE specific search space DCI type 1\n");

  int pucch_sched;
  nr_update_pucch_scheduling(module_id, UE_id, frame, slot, num_slots_per_tdd, &pucch_sched);
  NR_sched_pucch *pucch = &UE_list->UE_sched_ctrl[UE_id].sched_pucch[pucch_sched];
  const int TBS_bytes = nr_fill_nfapi_dl_pdu(module_id,
                                             dl_req,
                                             pucch,
                                             9 /* mcs */,
                                             bwpSize,
                                             0 /* bwpStart */);

  if (TBS_bytes == 0)
   return;

  const int lcid = DL_SCH_LCID_DTCH;
  int header_length_total = 0;
  int header_length_last = 0;
  int sdu_length_total = 0;
  int num_sdus = 0;
  LOG_D(MAC, "[gNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (TBS %d bytes, len %d)\n",
      module_id, frame, lcid, TBS_bytes, TBS_bytes - ta_len - header_length_total - sdu_length_total - 3);

  const mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(module_id,
                                                              rnti,
                                                              module_id,
                                                              frame,
                                                              slot,
                                                              ENB_FLAG_YES,
                                                              MBMS_FLAG_NO,
                                                              lcid,
                                                              0,
                                                              0);

  if (rlc_status.bytes_in_buffer > 0) {

    LOG_I(MAC, "configure fapi due to data availability \n");

    LOG_I(MAC, "[gNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d), TBS_bytes: %d \n \n",
        module_id, frame, TBS_bytes - ta_len - header_length_total - sdu_length_total - 3,
        lcid, header_length_total, TBS_bytes);

    sdu_lengths[num_sdus] = mac_rlc_data_req(module_id,
        rnti,
        module_id,
        frame,
        ENB_FLAG_YES,
        MBMS_FLAG_NO,
        lcid,
        TBS_bytes - ta_len - header_length_total - sdu_length_total - 3,
        (char *)&mac_sdus[sdu_length_total],
        0,
        0);

    LOG_W(MAC, "[gNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n", module_id, sdu_lengths[num_sdus], lcid);

    sdu_lcids[num_sdus] = lcid;
    sdu_length_total += sdu_lengths[num_sdus];
    header_length_last = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
    header_length_total += header_length_last;

    num_sdus++;

    //ue_sched_ctl->uplane_inactivity_timer = 0;
  }
  else {
    LOG_I(MAC, "Configuring DL_TX in %d.%d: random data\n", frame, slot);
    // fill dlsch_buffer with random data
    for (int i = 0; i < TBS_bytes; i++)
      mac_sdus[i] = (unsigned char) (lrand48()&0xff);
    sdu_lcids[0] = 0x3f; // DRB
    sdu_lengths[0] = TBS_bytes - ta_len - 3;
    header_length_total += 2 + (sdu_lengths[0] >= 128);
    sdu_length_total += sdu_lengths[0];
    num_sdus +=1;
  }

  // there is at least one SDU or TA command
  // if (num_sdus > 0 ){
  if (ta_len + sdu_length_total + header_length_total == 0) {
    // There is no data from RLC or MAC header, so don't schedule
    return;
  }

  // Check if there is data from RLC or CE
  const int post_padding = TBS_bytes >= 2 + header_length_total + sdu_length_total + ta_len;
  // padding param currently not in use
  //padding = TBS_bytes - header_length_total - sdu_length_total - ta_len - 1;

  const int offset = nr_generate_dlsch_pdu(
      module_id,
      (unsigned char *)mac_sdus,
      (unsigned char *)gNB_mac->UE_list.DLSCH_pdu[0][0].payload[0],
      num_sdus, // num_sdus
      sdu_lengths,
      sdu_lcids,
      255, // no drx
      NULL, // contention res id
      post_padding);

  // Padding: fill remainder of DLSCH with 0
  if (post_padding > 0) {
    for (int j = 0; j < TBS_bytes - offset; j++)
      gNB_mac->UE_list.DLSCH_pdu[0][0].payload[0][offset + j] = 0;
  }

  configure_fapi_dl_Tx(module_id,
                       frame,
                       slot,
                       dl_req,
                       tx_req,
                       TBS_bytes,
                       gNB_mac->pdu_index[CC_id]);

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
  if (frame%100 == 0){
    LOG_I(MAC,
          "%d.%d, first 10 payload bytes, TBS size: %d \n",
          frame,
          slot,
          TBS_bytes);
    for(int i = 0; i < 10; i++) {
      LOG_I(MAC, "byte %d : %x\n", i,((uint8_t *)gNB_mac->UE_list.DLSCH_pdu[0][0].payload[0])[i]);
    }
  }
#endif
}
