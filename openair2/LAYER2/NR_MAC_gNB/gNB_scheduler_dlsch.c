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

const int get_dl_tda(const gNB_MAC_INST *nrmac, const NR_ServingCellConfigCommon_t *scc, int slot) {

  const NR_TDD_UL_DL_Pattern_t *tdd = scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;
  AssertFatal(tdd || nrmac->common_channels->frame_type == FDD, "Dynamic TDD not handled yet\n");

  // Use special TDA in case of CSI-RS
  if(nrmac->UE_info.sched_csirs)
      return 1;

  if (tdd && tdd->nrofDownlinkSymbols > 1) { // if there is a mixed slot where we can transmit DL
    const int nr_slots_period = tdd->nrofDownlinkSlots + tdd->nrofUplinkSlots + 1;
    if ((slot%nr_slots_period) == tdd->nrofDownlinkSlots)
      return 2;
  }
  return 0; // if FDD or not mixed slot in TDD, for now use default TDA (TODO handle CSI-RS slots)
}

// Compute and write all MAC CEs and subheaders, and return number of written
// bytes
int nr_write_ce_dlsch_pdu(module_id_t module_idP,
                          const NR_UE_sched_ctrl_t *ue_sched_ctl,
                          unsigned char *mac_pdu,
                          unsigned char drx_cmd,
                          unsigned char *ue_cont_res_id) {
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

void nr_store_dlsch_buffer(module_id_t module_id,
                           frame_t frame,
                           sub_frame_t slot) {

  UE_iterator(RC.nrmac[module_id]->UE_info.list, UE) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    sched_ctrl->num_total_bytes = 0;
    sched_ctrl->dl_pdus_total = 0;

    /* loop over all activated logical channels */
    // Note: DL_SCH_LCID_DCCH, DL_SCH_LCID_DCCH1, DL_SCH_LCID_DTCH
    for (int i = 0; i < sched_ctrl->dl_lc_num; ++i) {
      const int lcid = sched_ctrl->dl_lc_ids[i];
      const uint16_t rnti = UE->rnti;
      LOG_D(NR_MAC, "In %s: UE %x: LCID %d\n", __FUNCTION__, rnti, lcid);
      if (lcid == DL_SCH_LCID_DTCH && sched_ctrl->rrc_processing_timer > 0) {
        continue;
      }
      start_meas(&RC.nrmac[module_id]->rlc_status_ind);
      sched_ctrl->rlc_status[lcid] = mac_rlc_status_ind(module_id,
                                                        rnti,
                                                        module_id,
                                                        frame,
                                                        slot,
                                                        ENB_FLAG_YES,
                                                        MBMS_FLAG_NO,
                                                        lcid,
                                                        0,
                                                        0);
      stop_meas(&RC.nrmac[module_id]->rlc_status_ind);

      if (sched_ctrl->rlc_status[lcid].bytes_in_buffer == 0)
        continue;

      sched_ctrl->dl_pdus_total += sched_ctrl->rlc_status[lcid].pdus_in_buffer;
      sched_ctrl->num_total_bytes += sched_ctrl->rlc_status[lcid].bytes_in_buffer;
      LOG_D(MAC,
            "[gNB %d][%4d.%2d] %s%d->DLSCH, RLC status for UE %d: %d bytes in buffer, total DL buffer size = %d bytes, %d total PDU bytes, %s TA command\n",
            module_id,
            frame,
            slot,
            lcid < 4 ? "DCCH":"DTCH",
            lcid,
            UE->rnti,
            sched_ctrl->rlc_status[lcid].bytes_in_buffer,
            sched_ctrl->num_total_bytes,
            sched_ctrl->dl_pdus_total,
            sched_ctrl->ta_apply ? "send":"do not send");
    }
  }
}

bool allocate_dl_retransmission(module_id_t module_id,
                                frame_t frame,
                                sub_frame_t slot,
                                uint16_t *rballoc_mask,
                                int *n_rb_sched,
                                NR_UE_info_t * UE,
                                int current_harq_pid) {

  gNB_MAC_INST *nr_mac = RC.nrmac[module_id];
  const NR_ServingCellConfigCommon_t *scc = nr_mac->common_channels->ServingCellConfigCommon;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  NR_sched_pdsch_t *retInfo = &sched_ctrl->harq_processes[current_harq_pid].sched_pdsch;
  NR_CellGroupConfig_t *cg = UE->CellGroup;

  NR_BWP_DownlinkDedicated_t *bwpd =
      cg &&
      cg->spCellConfig &&
      cg->spCellConfig->spCellConfigDedicated ?
      cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP : NULL;

  NR_BWP_UplinkDedicated_t *ubwpd =
      cg &&
      cg->spCellConfig &&
      cg->spCellConfig->spCellConfigDedicated &&
      cg->spCellConfig->spCellConfigDedicated->uplinkConfig ?
      cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP : NULL;

  const NR_SIB1_t *sib1 = RC.nrmac[module_id]->common_channels[0].sib1 ? RC.nrmac[module_id]->common_channels[0].sib1->message.choice.c1->choice.systemInformationBlockType1 : NULL;
  NR_BWP_t *genericParameters = get_dl_bwp_genericParameters(sched_ctrl->active_bwp,
                                                             RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon,
                                                             sib1);

  const int coresetid = (sched_ctrl->active_bwp||bwpd) ? sched_ctrl->coreset->controlResourceSetId : RC.nrmac[module_id]->sched_ctrlCommon->coreset->controlResourceSetId;
  const uint16_t bwpSize = coresetid == 0 ? RC.nrmac[module_id]->cset0_bwp_size : NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);

  int rbStart = 0; // start wrt BWPstart
  NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
  int rbSize = 0;
  const int tda = get_dl_tda(RC.nrmac[module_id], scc, slot);
  AssertFatal(tda>=0,"Unable to find PDSCH time domain allocation in list\n");

  if (tda == retInfo->time_domain_allocation) {
    /* Check that there are enough resources for retransmission */
    while (rbSize < retInfo->rbSize) {
      rbStart += rbSize; /* last iteration rbSize was not enough, skip it */
      rbSize = 0;

      const int slbitmap = SL_to_bitmap(ps->startSymbolIndex, ps->nrOfSymbols);
      while (rbStart < bwpSize && (rballoc_mask[rbStart] & slbitmap) != slbitmap)
        rbStart++;

      if (rbStart >= bwpSize) {
        LOG_D(NR_MAC, "cannot allocate retransmission for RNTI %04x: no resources\n", UE->rnti);
        return false;
      }

      while (rbStart + rbSize < bwpSize &&
             (rballoc_mask[rbStart + rbSize] & slbitmap) == slbitmap &&
             rbSize < retInfo->rbSize)
        rbSize++;
    }

    /* check whether we need to switch the TDA allocation since the last
     * (re-)transmission */
    if (ps->time_domain_allocation != tda) {
      nr_set_pdsch_semi_static(sib1,
                               scc,
                               cg,
                               sched_ctrl->active_bwp,
                               bwpd,
                               tda,
                               ps->nrOfLayers,
                               sched_ctrl,
                               ps);
    }
  } else {
    /* the retransmission will use a different time domain allocation, check
     * that we have enough resources */
    NR_pdsch_semi_static_t temp_ps = *ps;

    nr_set_pdsch_semi_static(sib1,
                             scc,
                             cg,
                             sched_ctrl->active_bwp,
                             bwpd,
                             tda,
                             ps->nrOfLayers,
                             sched_ctrl,
                             &temp_ps);

    const uint16_t slbitmap = SL_to_bitmap(temp_ps.startSymbolIndex, temp_ps.nrOfSymbols);
    while (rbStart < bwpSize && (rballoc_mask[rbStart] & slbitmap) != slbitmap)
      rbStart++;

    while (rbStart + rbSize < bwpSize && (rballoc_mask[rbStart + rbSize] & slbitmap) == slbitmap)
      rbSize++;

    uint32_t new_tbs;
    uint16_t new_rbSize;
    bool success = nr_find_nb_rb(retInfo->Qm,
                                 retInfo->R,
                                 temp_ps.nrOfLayers,
                                 temp_ps.nrOfSymbols,
                                 temp_ps.N_PRB_DMRS * temp_ps.N_DMRS_SLOT,
                                 retInfo->tb_size,
                                 1, /* minimum of 1RB: need to find exact TBS, don't preclude any number */
                                 rbSize,
                                 &new_tbs,
                                 &new_rbSize);

    if (!success || new_tbs != retInfo->tb_size) {
      LOG_D(MAC, "new TBsize %d of new TDA does not match old TBS %d\n", new_tbs, retInfo->tb_size);
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
  const int cid = sched_ctrl->coreset->controlResourceSetId;
  const uint16_t Y = get_Y(cid%3, slot, UE->rnti);
  uint8_t nr_of_candidates;

  for (int i=0; i<5; i++) {
    // for now taking the lowest value among the available aggregation levels
    find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                &nr_of_candidates,
                                sched_ctrl->search_space,
                                1<<i);

    if(nr_of_candidates>0) break;
  }

  int CCEIndex = find_pdcch_candidate(RC.nrmac[module_id],
                                      /* CC_id = */ 0,
                                      sched_ctrl->aggregation_level,
                                      nr_of_candidates,
                                      &sched_ctrl->sched_pdcch,
                                      sched_ctrl->coreset,
                                      Y);

  if (CCEIndex<0) {
    LOG_D(MAC, "%4d.%2d could not find CCE for DL DCI retransmission RNTI %04x\n",
          frame, slot, UE->rnti);
    return false;
  }

  /* Find PUCCH occasion: if it fails, undo CCE allocation (undoing PUCCH
   * allocation after CCE alloc fail would be more complex) */

  int r_pucch = nr_get_pucch_resource(sched_ctrl->coreset, sched_ctrl->active_ubwp, ubwpd, CCEIndex);
  const int alloc = nr_acknack_scheduling(module_id, UE, frame, slot, r_pucch, 0);
  if (alloc<0) {
    LOG_D(MAC,
          "could not find PUCCH for UE %04x@%d.%d\n",
          UE->rnti,
          frame,
          slot);
    RC.nrmac[module_id]->pdcch_cand[cid]--;
    return false;
  }

  sched_ctrl->cce_index = CCEIndex;
  fill_pdcch_vrb_map(RC.nrmac[module_id],
                     /* CC_id = */ 0,
                     &sched_ctrl->sched_pdcch,
                     CCEIndex,
                     sched_ctrl->aggregation_level);
  /* just reuse from previous scheduling opportunity, set new start RB */
  sched_ctrl->sched_pdsch = *retInfo;
  sched_ctrl->sched_pdsch.rbStart = rbStart;
  sched_ctrl->sched_pdsch.pucch_allocation = alloc;
  /* retransmissions: directly allocate */
  *n_rb_sched -= sched_ctrl->sched_pdsch.rbSize;

  for (int rb = 0; rb < sched_ctrl->sched_pdsch.rbSize; rb++)
    rballoc_mask[rb + sched_ctrl->sched_pdsch.rbStart] ^= SL_to_bitmap(ps->startSymbolIndex, ps->nrOfSymbols);

  return true;
}

uint32_t pf_tbs[3][29]; // pre-computed, approximate TBS values for PF coefficient
typedef struct UEsched_s {
  float coef;
  NR_UE_info_t * UE;
} UEsched_t;

static int comparator(const void *p, const void *q) {
  return ((UEsched_t*)p)->coef < ((UEsched_t*)q)->coef;
}

void pf_dl(module_id_t module_id,
           frame_t frame,
           sub_frame_t slot,
           NR_UE_info_t **UE_list,
           int max_num_ue,
           int n_rb_sched,
           uint16_t *rballoc_mask) {
  gNB_MAC_INST *mac = RC.nrmac[module_id];
  NR_ServingCellConfigCommon_t *scc=mac->common_channels[0].ServingCellConfigCommon;
  // UEs that could be scheduled
  UEsched_t UE_sched[MAX_MOBILES_PER_GNB] = {0};
  int remainUEs = max_num_ue;
  int curUE = 0;

  /* Loop UE_info->list to check retransmission */
  UE_iterator(UE_list, UE) {
    if (UE->Msg4_ACKed != true)
      continue;

    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;

    if (sched_ctrl->ul_failure==1 && get_softmodem_params()->phy_test==0) continue;

    const NR_mac_dir_stats_t *stats = &UE->mac_stats.dl;
    NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
    NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
    /* get the PID of a HARQ process awaiting retrnasmission, or -1 otherwise */
    sched_pdsch->dl_harq_pid = sched_ctrl->retrans_dl_harq.head;
    UE->layers = ps->nrOfLayers; // initialization of layers to the previous value in the strcuture
    /* Calculate Throughput */
    const float a = 0.0005f; // corresponds to 200ms window
    const uint32_t b = UE->mac_stats.dl.current_bytes;
    UE->dl_thr_ue = (1 - a) * UE->dl_thr_ue + a * b;

    /* retransmission */
    if (sched_pdsch->dl_harq_pid >= 0) {
      /* Allocate retransmission */
      bool r = allocate_dl_retransmission(module_id, frame, slot, rballoc_mask, &n_rb_sched, UE, sched_pdsch->dl_harq_pid);

      if (!r) {
        LOG_D(NR_MAC, "%4d.%2d retransmission can NOT be allocated\n", frame, slot);
        continue;
      }

      /* reduce max_num_ue once we are sure UE can be allocated, i.e., has CCE */
      remainUEs--;

      if (remainUEs == 0)
	// we have filled all with mandatory retransmissions
	// no need to schedule new transmissions
	return;
    } else {
      /* Check DL buffer and skip this UE if no bytes and no TA necessary */
      if (sched_ctrl->num_total_bytes == 0 && frame != (sched_ctrl->ta_frame + 10) % 1024)
        continue;

      /* Calculate coeff */
      const NR_bler_options_t *bo = &mac->dl_bler;
      const int max_mcs_table = ps->mcsTableIdx == 1 ? 27 : 28;
      const int max_mcs = min(sched_ctrl->dl_max_mcs, max_mcs_table);
      sched_pdsch->mcs = get_mcs_from_bler(bo, stats, &sched_ctrl->dl_bler_stats, max_mcs, frame);
      UE->layers = set_dl_nrOfLayers(sched_ctrl);
      const uint8_t Qm = nr_get_Qm_dl(sched_pdsch->mcs, ps->mcsTableIdx);
      const uint16_t R = nr_get_code_rate_dl(sched_pdsch->mcs, ps->mcsTableIdx);
      uint32_t tbs = nr_compute_tbs(Qm,
                                    R,
                                    1, /* rbSize */
                                    10, /* hypothetical number of slots */
                                    0, /* N_PRB_DMRS * N_DMRS_SLOT */
                                    0 /* N_PRB_oh, 0 for initialBWP */,
                                    0 /* tb_scaling */,
                                    UE->layers) >> 3;
      float coeff_ue = (float) tbs / UE->dl_thr_ue;
      LOG_D(NR_MAC,"UE %04x b %d, thr_ue %f, tbs %d, coeff_ue %f\n",
            UE->rnti, b, UE->dl_thr_ue, tbs, coeff_ue);
      /* Create UE_sched list for UEs eligible for new transmission*/
      UE_sched[curUE].coef=coeff_ue;
      UE_sched[curUE].UE=UE;
      curUE++;
    }
  }

  qsort(UE_sched, sizeof(*UE_sched), sizeofArray(UE_sched), comparator);
  UEsched_t *iterator = UE_sched;

  const int min_rbSize = 5;

  /* Loop UE_sched to find max coeff and allocate transmission */
  while (remainUEs> 0 && n_rb_sched >= min_rbSize && iterator->UE != NULL) {

    NR_CellGroupConfig_t *cg = iterator->UE->CellGroup;

    NR_BWP_DownlinkDedicated_t *bwpd =
        cg &&
        cg->spCellConfig &&
        cg->spCellConfig->spCellConfigDedicated ?
        cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP : NULL;

    NR_BWP_UplinkDedicated_t *ubwpd =
        cg &&
        cg->spCellConfig &&
        cg->spCellConfig->spCellConfigDedicated &&
        cg->spCellConfig->spCellConfigDedicated->uplinkConfig ?
        cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP : NULL;

    NR_UE_sched_ctrl_t *sched_ctrl = &iterator->UE->UE_sched_ctrl;
    const uint16_t rnti = iterator->UE->rnti;
    const NR_SIB1_t *sib1 = RC.nrmac[module_id]->common_channels[0].sib1 ?
      RC.nrmac[module_id]->common_channels[0].sib1->message.choice.c1->choice.systemInformationBlockType1 :
      NULL;

    NR_BWP_t *genericParameters = get_dl_bwp_genericParameters(sched_ctrl->active_bwp,
                                                               RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon,
                                                               sib1);

    const int coresetid = (sched_ctrl->active_bwp||bwpd) ?
      sched_ctrl->coreset->controlResourceSetId :
      RC.nrmac[module_id]->sched_ctrlCommon->coreset->controlResourceSetId;
    const uint16_t bwpSize = coresetid == 0 ?
      RC.nrmac[module_id]->cset0_bwp_size :
      NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
    int rbStart = 0; // start wrt BWPstart

    if (sched_ctrl->available_dl_harq.head < 0) {
      LOG_D(MAC, "RNTI %04x has no free HARQ process, skipping\n", iterator->UE->rnti);
      iterator++;
      continue;
    }

    /* Find a free CCE */
    const int cid = sched_ctrl->coreset->controlResourceSetId;
    const uint16_t Y = get_Y(cid%3, slot, iterator->UE->rnti);
    uint8_t nr_of_candidates;

    for (int i=0; i<5; i++) {
      // for now taking the lowest value among the available aggregation levels
      find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                  &nr_of_candidates,
                                  sched_ctrl->search_space,
                                  1<<i);

      if(nr_of_candidates>0) break;
    }

    int CCEIndex = find_pdcch_candidate(mac,
                                        /* CC_id = */ 0,
                                        sched_ctrl->aggregation_level,
                                        nr_of_candidates,
                                        &sched_ctrl->sched_pdcch,
                                        sched_ctrl->coreset,
                                        Y);

    if (CCEIndex<0) {
      LOG_D(NR_MAC, "%4d.%2d could not find CCE for DL DCI RNTI %04x\n", frame, slot, rnti);
      iterator++;
      continue;
    }

    /* Find PUCCH occasion: if it fails, undo CCE allocation (undoing PUCCH
    * allocation after CCE alloc fail would be more complex) */

    int r_pucch = nr_get_pucch_resource(sched_ctrl->coreset, sched_ctrl->active_ubwp, ubwpd, CCEIndex);
    const int alloc = nr_acknack_scheduling(module_id, iterator->UE, frame, slot, r_pucch, 0);

    if (alloc<0) {
      LOG_D(NR_MAC,
            "could not find PUCCH for %04x@%d.%d\n",
            rnti,
            frame,
            slot);
      mac->pdcch_cand[cid]--;
      iterator++;
      continue;
    }

    sched_ctrl->cce_index = CCEIndex;
    fill_pdcch_vrb_map(mac,
                       /* CC_id = */ 0,
                       &sched_ctrl->sched_pdcch,
                       CCEIndex,
                       sched_ctrl->aggregation_level);

    /* MCS has been set above */
    const int tda = get_dl_tda(RC.nrmac[module_id], scc, slot);
    AssertFatal(tda>=0,"Unable to find PDSCH time domain allocation in list\n");
    NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
    NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;

    if (ps->nrOfLayers != iterator->UE->layers || ps->time_domain_allocation != tda ) {
      nr_set_pdsch_semi_static(sib1,
                               scc,
                               iterator->UE->CellGroup,
                               sched_ctrl->active_bwp,
                               bwpd,
                               tda,
                               iterator->UE->layers,
                               sched_ctrl,
                               ps);
    }

    const uint16_t slbitmap = SL_to_bitmap(ps->startSymbolIndex, ps->nrOfSymbols);

    // Freq-demain allocation
    while (rbStart < bwpSize && (rballoc_mask[rbStart] & slbitmap) != slbitmap)
      rbStart++;

    uint16_t max_rbSize = 1;

    while (rbStart + max_rbSize < bwpSize && (rballoc_mask[rbStart + max_rbSize] & slbitmap) == slbitmap)
      max_rbSize++;

    sched_pdsch->Qm = nr_get_Qm_dl(sched_pdsch->mcs, ps->mcsTableIdx);
    sched_pdsch->R = nr_get_code_rate_dl(sched_pdsch->mcs, ps->mcsTableIdx);
    sched_pdsch->pucch_allocation = alloc;
    uint32_t TBS = 0;
    uint16_t rbSize;
    // Fix me: currently, the RLC does not give us the total number of PDUs
    // awaiting. Therefore, for the time being, we put a fixed overhead of 12
    // (for 4 PDUs) and optionally + 2 for TA. Once RLC gives the number of
    // PDUs, we replace with 3 * numPDUs
    const int oh = 3 * 4 + 2 * (frame == (sched_ctrl->ta_frame + 10) % 1024);
    //const int oh = 3 * sched_ctrl->dl_pdus_total + 2 * (frame == (sched_ctrl->ta_frame + 10) % 1024);
    nr_find_nb_rb(sched_pdsch->Qm,
                  sched_pdsch->R,
                  ps->nrOfLayers,
                  ps->nrOfSymbols,
                  ps->N_PRB_DMRS * ps->N_DMRS_SLOT,
                  sched_ctrl->num_total_bytes + oh,
                  min_rbSize,
                  max_rbSize,
                  &TBS,
                  &rbSize);
    sched_pdsch->rbSize = rbSize;
    sched_pdsch->rbStart = rbStart;
    sched_pdsch->tb_size = TBS;
    /* transmissions: directly allocate */
    n_rb_sched -= sched_pdsch->rbSize;

    for (int rb = 0; rb < sched_pdsch->rbSize; rb++)
      rballoc_mask[rb + sched_pdsch->rbStart] ^= slbitmap;

    remainUEs--;
    iterator++;
  }
}

void nr_fr1_dlsch_preprocessor(module_id_t module_id, frame_t frame, sub_frame_t slot) {
  NR_UEs_t *UE_info = &RC.nrmac[module_id]->UE_info;

  if (UE_info->list[0] == NULL)
    return;

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;
  const int CC_id = 0;
  /* Get bwpSize and TDAfrom the first UE */
  /* This is temporary and it assumes all UEs have the same BWP and TDA*/
  NR_UE_info_t *UE=UE_info->list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  const int tda = get_dl_tda(RC.nrmac[module_id], scc, slot);
  int startSymbolIndex, nrOfSymbols;
  const struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = sched_ctrl->active_bwp ?
        sched_ctrl->active_bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList :
        scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
  AssertFatal(tda < tdaList->list.count, "time_domain_allocation %d>=%d\n", tda, tdaList->list.count);
  const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);

  const NR_SIB1_t *sib1 = RC.nrmac[module_id]->common_channels[0].sib1 ? RC.nrmac[module_id]->common_channels[0].sib1->message.choice.c1->choice.systemInformationBlockType1 : NULL;
  NR_BWP_t *genericParameters = get_dl_bwp_genericParameters(sched_ctrl->active_bwp,
                                                             RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon,
                                                             sib1);

  NR_BWP_DownlinkDedicated_t *bwpd =
      UE->CellGroup &&
      UE->CellGroup->spCellConfig &&
      UE->CellGroup->spCellConfig->spCellConfigDedicated ?
      UE->CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP : NULL;

  const int coresetid = (sched_ctrl->active_bwp||bwpd) ? sched_ctrl->coreset->controlResourceSetId : RC.nrmac[module_id]->sched_ctrlCommon->coreset->controlResourceSetId;

  const uint16_t bwpSize = coresetid == 0 ? RC.nrmac[module_id]->cset0_bwp_size : NRRIV2BW(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);
  const uint16_t BWPStart = coresetid == 0 ? RC.nrmac[module_id]->cset0_bwp_start : NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);

  const uint16_t slbitmap = SL_to_bitmap(startSymbolIndex, nrOfSymbols);
  uint16_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;
  uint16_t rballoc_mask[bwpSize];
  int n_rb_sched = 0;

  for (int i = 0; i < bwpSize; i++) {
    // calculate mask: init with "NOT" vrb_map:
    // if any RB in vrb_map is blocked (1), the current RBG will be 0
    rballoc_mask[i] = (~vrb_map[i+BWPStart])&0x3fff; //bitwise not and 14 symbols

    // if all the pdsch symbols are free
    if ((rballoc_mask[i]&slbitmap) == slbitmap) {
      n_rb_sched++;
    }
  }

  /* Retrieve amount of data to send for this UE */
  nr_store_dlsch_buffer(module_id, frame, slot);
  /* proportional fair scheduling algorithm */
  pf_dl(module_id,
        frame,
        slot,
        UE_info->list,
        MAX_MOBILES_PER_GNB,
        n_rb_sched,
        rballoc_mask);
}

nr_pp_impl_dl nr_init_fr1_dlsch_preprocessor(module_id_t module_id, int CC_id) {
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
                                                1 /* nrOfLayers */) >> 3;
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

  //if (slot==7 || slot == 17) return;

  /* PREPROCESSOR */
  gNB_mac->pre_processor_dl(module_id, frame, slot);
  const int CC_id = 0;
  NR_ServingCellConfigCommon_t *scc = gNB_mac->common_channels[CC_id].ServingCellConfigCommon;
  NR_UEs_t *UE_info = &gNB_mac->UE_info;
  nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;

  UE_iterator(UE_info->list, UE) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;

    if (sched_ctrl->ul_failure==1 && get_softmodem_params()->phy_test==0) continue;

    NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
    UE->mac_stats.dl.current_bytes = 0;
    NR_CellGroupConfig_t *cg = UE->CellGroup;

    NR_BWP_DownlinkDedicated_t *bwpd =
        cg &&
        cg->spCellConfig &&
        cg->spCellConfig->spCellConfigDedicated ?
        cg->spCellConfig->spCellConfigDedicated->initialDownlinkBWP : NULL;

    /* update TA and set ta_apply every 10 frames.
     * Possible improvement: take the periodicity from input file.
     * If such UE is not scheduled now, it will be by the preprocessor later.
     * If we add the CE, ta_apply will be reset */
    if (frame == (sched_ctrl->ta_frame + 10) % 1024) {
      sched_ctrl->ta_apply = true; /* the timer is reset once TA CE is scheduled */
      LOG_D(NR_MAC, "[UE %04x][%d.%d] UL timing alignment procedures: setting flag for Timing Advance command\n", UE->rnti, frame, slot);
    }

    if (sched_pdsch->rbSize <= 0)
      continue;

    const rnti_t rnti = UE->rnti;
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
                  "no free HARQ process available for UE %04x\n",
                  UE->rnti);
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
    UE->mac_stats.dl.rounds[harq->round]++;
    LOG_D(NR_MAC,
          "%4d.%2d [DLSCH/PDSCH/PUCCH] RNTI %04x DCI L %d start %3d RBs %3d startSymbol %2d nb_symbol %2d dmrspos %x MCS %2d nrOfLayers %d TBS %4d HARQ PID %2d round %d RV %d NDI %d dl_data_to_ULACK %d (%d.%d) PUCCH allocation %d TPC %d\n",
          frame,
          slot,
          rnti,
          sched_ctrl->aggregation_level,
          sched_pdsch->rbStart,
          sched_pdsch->rbSize,
          ps->startSymbolIndex,
          ps->nrOfSymbols,
          ps->dl_dmrs_symb_pos,
          sched_pdsch->mcs,
          nrOfLayers,
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

    const NR_SIB1_t *sib1 = RC.nrmac[module_id]->common_channels[0].sib1 ? RC.nrmac[module_id]->common_channels[0].sib1->message.choice.c1->choice.systemInformationBlockType1 : NULL;
    NR_BWP_t *genericParameters = get_dl_bwp_genericParameters(bwp,
                                                               RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon,
                                                               sib1);

    NR_SearchSpace_t *ss = (bwp||bwpd) ? sched_ctrl->search_space : gNB_mac->sched_ctrlCommon->search_space;

    const int bwp_id = bwp ? bwp->bwp_Id : 0;
    const int coresetid = (bwp||bwpd) ? sched_ctrl->coreset->controlResourceSetId : gNB_mac->sched_ctrlCommon->coreset->controlResourceSetId;

    /* look up the PDCCH PDU for this CC, BWP, and CORESET. If it does not exist, create it */
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu = gNB_mac->pdcch_pdu_idx[CC_id][coresetid];

    if (!pdcch_pdu) {
      LOG_D(NR_MAC, "creating pdcch pdu, pdcch_pdu = NULL. \n");
      nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
      memset(dl_tti_pdcch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
      dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      dl_req->nPDUs += 1;
      pdcch_pdu = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
      LOG_D(NR_MAC,"Trying to configure DL pdcch for UE %04x, bwp %d, cs %d\n", UE->rnti, bwp_id, coresetid);
      NR_ControlResourceSet_t *coreset = (bwp||bwpd)? sched_ctrl->coreset:gNB_mac->sched_ctrlCommon->coreset;
      nr_configure_pdcch(pdcch_pdu, coreset, genericParameters, &sched_ctrl->sched_pdcch);
      gNB_mac->pdcch_pdu_idx[CC_id][coresetid] = pdcch_pdu;
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

    if (coresetid == 0) {
      pdsch_pdu->BWPSize  = gNB_mac->cset0_bwp_size;
      pdsch_pdu->BWPStart = gNB_mac->cset0_bwp_start;
    } else {
      pdsch_pdu->BWPSize  = NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
      pdsch_pdu->BWPStart = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);
    }

    pdsch_pdu->SubcarrierSpacing = genericParameters->subcarrierSpacing;
    pdsch_pdu->CyclicPrefix = genericParameters->cyclicPrefix ? *genericParameters->cyclicPrefix : 0;
    // Codeword information
    pdsch_pdu->NrOfCodewords = 1;
    //number of information bits per 1024 coded bits expressed in 0.1 bit units
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
    // Precoding
    if (sched_ctrl->set_pmi) {
      const int report_id = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.csi_report_id;
      nr_csi_report_t *csi_report = &UE->csi_report_template[report_id];
      pdsch_pdu->precodingAndBeamforming.prg_size = pdsch_pdu->rbSize;
      pdsch_pdu->precodingAndBeamforming.prgs_list[0].pm_idx = set_pm_index(sched_ctrl,
                                                                            nrOfLayers,
                                                                            csi_report->N1,
                                                                            csi_report->N2,
                                                                            gNB_mac->xp_pdsch_antenna_ports,
                                                                            csi_report->codebook_mode);
    }
    // TBS_LBRM according to section 5.4.2.1 of 38.212
    long *maxMIMO_Layers = cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->ext1->maxMIMO_Layers;
    AssertFatal (maxMIMO_Layers != NULL,"Option with max MIMO layers not configured is not supported\n");
    int nl_tbslbrm = *maxMIMO_Layers < 4 ? *maxMIMO_Layers : 4;
    // Maximum number of PRBs across all configured DL BWPs
    int bw_tbslbrm = get_bw_tbslbrm(genericParameters, cg);
    pdsch_pdu->maintenance_parms_v3.tbSizeLbrmBytes = nr_compute_tbslbrm(ps->mcsTableIdx,
                                                                         bw_tbslbrm,
                                                                         nl_tbslbrm);

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

    LOG_D(NR_MAC,"Configuring DCI/PDCCH in %d.%d at CCE %d, rnti %x\n", frame,slot,sched_ctrl->cce_index,rnti);
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
    const int n_dl_bwp = bwp ? cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count : 0;
    AssertFatal(n_dl_bwp <= NR_MAX_NUM_BWP, "downlinkBWP_ToAddModList has %d BWP!\n", n_dl_bwp);

    // as per table 7.3.1.1.2-1 in 38.212
    dci_payload.bwp_indicator.val = bwp ? (n_dl_bwp < 4 ? bwp->bwp_Id : bwp->bwp_Id - 1) : 0;

    if (bwp) AssertFatal(bwp->bwp_Dedicated->pdsch_Config->choice.setup->resourceAllocation == NR_PDSCH_Config__resourceAllocation_resourceAllocationType1,
                           "Only frequency resource allocation type 1 is currently supported\n");

    dci_payload.frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pdsch_pdu->rbSize,
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
          "nrOfLayers %d, time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d tpc %d ti %d\n",
          frame,
          slot,
          dci_payload.frequency_domain_assignment.val,
          pdsch_pdu->rbStart,
          pdsch_pdu->rbSize,
          pdsch_pdu->BWPSize,
          pdsch_pdu->nrOfLayers,
          dci_payload.time_domain_assignment.val,
          dci_payload.vrb_to_prb_mapping.val,
          dci_payload.mcs,
          dci_payload.tb_scaling,
          dci_payload.ndi,
          dci_payload.rv,
          dci_payload.tpc,
          pucch->timing_indicator);

    int dci_format = ss && ss->searchSpaceType && ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific ?
                     NR_DL_DCI_FORMAT_1_1 : NR_DL_DCI_FORMAT_1_0;

    const int rnti_type = NR_RNTI_C;
    fill_dci_pdu_rel15(scc,
                       cg,
                       dci_pdu,
                       &dci_payload,
                       dci_format,
                       rnti_type,
                       pdsch_pdu->BWPSize,
                       bwp ? bwp->bwp_Id : 0,
                       coresetid,
                       gNB_mac->cset0_bwp_size);

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
            "%d.%2d DL retransmission RNTI %04x HARQ PID %d round %d NDI %d\n",
            frame,
            slot,
            rnti,
            current_harq_pid,
            harq->round,
            harq->ndi);
      AssertFatal(harq->sched_pdsch.tb_size == TBS,
                  "UE %04x mismatch between scheduled TBS and buffered TB for HARQ PID %d\n",
                  UE->rnti,
                  current_harq_pid);
      T(T_GNB_MAC_RETRANSMISSION_DL_PDU_WITH_DATA, T_INT(module_id), T_INT(CC_id), T_INT(rnti),
        T_INT(frame), T_INT(slot), T_INT(current_harq_pid), T_INT(harq->round), T_BUFFER(harq->transportBlock, TBS));
    } else { /* initial transmission */
      LOG_D(NR_MAC, "[%s] Initial HARQ transmission in %d.%d\n", __FUNCTION__, frame, slot);
      uint8_t *buf = (uint8_t *) harq->transportBlock;
      /* first, write all CEs that might be there */
      int written = nr_write_ce_dlsch_pdu(module_id,
                                          sched_ctrl,
                                          (unsigned char *)buf,
                                          255, // no drx
                                          NULL); // contention res id
      buf += written;
      uint8_t *bufEnd = buf + TBS - written;
      DevAssert(TBS > written);
      int dlsch_total_bytes = 0;
      /* next, get RLC data */
      start_meas(&gNB_mac->rlc_data_req);

      if (sched_ctrl->num_total_bytes > 0) {
        /* loop over all activated logical channels */
        for (int i = 0; i < sched_ctrl->dl_lc_num; ++i) {
          const int lcid = sched_ctrl->dl_lc_ids[i];

          if (sched_ctrl->rlc_status[lcid].bytes_in_buffer == 0)
            continue; // no data for this LC        tbs_size_t len = 0;

          int lcid_bytes=0;
          while (bufEnd-buf > sizeof(NR_MAC_SUBHEADER_LONG) + 1 ) {
            // we do not know how much data we will get from RLC, i.e., whether it
            // will be longer than 256B or not. Therefore, reserve space for long header, then
            // fetch data, then fill real length
            NR_MAC_SUBHEADER_LONG *header = (NR_MAC_SUBHEADER_LONG *) buf;
            /* limit requested number of bytes to what preprocessor specified, or
             * such that TBS is full */
            const rlc_buffer_occupancy_t ndata = min(sched_ctrl->rlc_status[lcid].bytes_in_buffer,
                                                 bufEnd-buf-sizeof(NR_MAC_SUBHEADER_LONG));
            tbs_size_t len = mac_rlc_data_req(module_id,
                                              rnti,
                                              module_id,
                                              frame,
                                              ENB_FLAG_YES,
                                              MBMS_FLAG_NO,
                                              lcid,
                                              ndata,
                                              (char *)buf+sizeof(NR_MAC_SUBHEADER_LONG),
                                              0,
                                              0);
            LOG_D(NR_MAC,
                  "%4d.%2d RNTI %04x: %d bytes from %s %d (ndata %d, remaining size %ld)\n",
                  frame,
                  slot,
                  rnti,
                  len,
                  lcid < 4 ? "DCCH" : "DTCH",
                  lcid,
                  ndata,
                  bufEnd-buf-+sizeof(NR_MAC_SUBHEADER_LONG));

            if (len == 0)
              break;

            header->R = 0;
            header->F = 1;
            header->LCID = lcid;
            header->L = htons(len);
            buf += len+sizeof(NR_MAC_SUBHEADER_LONG);
            dlsch_total_bytes += len;
            lcid_bytes += len;
          }

          UE->mac_stats.dl.lc_bytes[lcid] += lcid_bytes;
        }
      } else if (get_softmodem_params()->phy_test || get_softmodem_params()->do_ra) {
        /* we will need the large header, phy-test typically allocates all
         * resources and fills to the last byte below */
        LOG_D(NR_MAC, "Configuring DL_TX in %d.%d: TBS %d of random data\n", frame, slot, TBS);

        if (bufEnd-buf > sizeof(NR_MAC_SUBHEADER_LONG) ) {
          NR_MAC_SUBHEADER_LONG *header = (NR_MAC_SUBHEADER_LONG *) buf;
          // fill dlsch_buffer with random data
          header->R = 0;
          header->F = 1;
          header->LCID = DL_SCH_LCID_PADDING;
          buf += sizeof(NR_MAC_SUBHEADER_LONG);
          header->L = htons(bufEnd-buf);
          dlsch_total_bytes += bufEnd-buf;

          for (; buf < bufEnd - 3; buf += 4) {
            uint32_t *buf32 = (uint32_t *)buf;
            *buf32 = lrand48();
          }
          for (; buf < bufEnd; buf++)
            *buf = lrand48() & 0xff;
        }
      }

      stop_meas(&gNB_mac->rlc_data_req);

      // Add padding header and zero rest out if there is space left
      if (bufEnd-buf > 0) {
        NR_MAC_SUBHEADER_FIXED *padding = (NR_MAC_SUBHEADER_FIXED *) buf;
        padding->R = 0;
        padding->LCID = DL_SCH_LCID_PADDING;
        buf += 1;
        memset(buf,0,bufEnd-buf);
        buf=bufEnd;
      }

      UE->mac_stats.dl.total_bytes += TBS;
      UE->mac_stats.dl.current_bytes = TBS;
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
              "%d.%2d UE %04x TA scheduled, resetting TA frame\n",
              frame,
              slot,
              UE->rnti);
      }

      T(T_GNB_MAC_DL_PDU_WITH_DATA, T_INT(module_id), T_INT(CC_id), T_INT(rnti),
        T_INT(frame), T_INT(slot), T_INT(current_harq_pid), T_BUFFER(harq->transportBlock, TBS));
    }

    const int ntx_req = gNB_mac->TX_req[CC_id].Number_of_PDUs;
    nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[ntx_req];
    tx_req->PDU_length = TBS;
    tx_req->PDU_index  = pduindex;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = TBS + 2;
    memcpy(tx_req->TLVs[0].value.direct, harq->transportBlock, TBS);
    gNB_mac->TX_req[CC_id].Number_of_PDUs++;
    gNB_mac->TX_req[CC_id].SFN = frame;
    gNB_mac->TX_req[CC_id].Slot = slot;
    /* mark UE as scheduled */
    sched_pdsch->rbSize = 0;
  }
}
