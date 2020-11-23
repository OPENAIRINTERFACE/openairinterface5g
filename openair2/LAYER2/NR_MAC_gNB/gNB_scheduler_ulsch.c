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

/*! \file gNB_scheduler_ulsch.c
 * \brief gNB procedures for the ULSCH transport channel
 * \author Navid Nikaein and Raymond Knopp, Guido Casati
 * \date 2019
 * \email: guido.casati@iis.fraunhofer.de
 * \version 1.0
 * @ingroup _mac
 */


#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "executables/softmodem-common.h"
#include "common/utils/nr/nr_common.h"


void nr_process_mac_pdu(
    module_id_t module_idP,
    rnti_t rnti,
    uint8_t CC_id,
    frame_t frameP,
    uint8_t *pduP,
    uint16_t mac_pdu_len)
{

    // This function is adapting code from the old
    // parse_header(...) and ue_send_sdu(...) functions of OAI LTE

    uint8_t *pdu_ptr = pduP, rx_lcid, done = 0;
    int pdu_len = mac_pdu_len;
    uint16_t mac_ce_len, mac_subheader_len, mac_sdu_len;


    //  For both DL/UL-SCH
    //  Except:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |       L       |

    //  For both DL/UL-SCH
    //  For:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding, for single/multiple 1-oct padding CE(s)
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|R|   LCID    |
    //  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
    //  L: The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
    //  F: lenght of L is 0:8 or 1:16 bits wide
    //  R: Reserved bit, set to zero.

    while (!done && pdu_len > 0){
        mac_ce_len = 0;
        mac_subheader_len = 1; //  default to fixed-length subheader = 1-oct
        mac_sdu_len = 0;
        rx_lcid = ((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID;

        LOG_D(MAC, "LCID received at gNB side: %d \n", rx_lcid);

        switch(rx_lcid){
            //  MAC CE

            /*#ifdef DEBUG_HEADER_PARSING
              LOG_D(MAC, "[UE] LCID %d, PDU length %d\n", ((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID, pdu_len);
            #endif*/
        case UL_SCH_LCID_RECOMMENDED_BITRATE_QUERY:
              // 38.321 Ch6.1.3.20
              mac_ce_len = 2;
              break;
        case UL_SCH_LCID_CONFIGURED_GRANT_CONFIRMATION:
                // 38.321 Ch6.1.3.7
                break;
        case UL_SCH_LCID_S_BSR:
        	//38.321 section 6.1.3.1
        	//fixed length
        	mac_ce_len =1;
        	/* Extract short BSR value */
        	break;

        case UL_SCH_LCID_S_TRUNCATED_BSR:
        	//38.321 section 6.1.3.1
        	//fixed length
        	mac_ce_len =1;
        	/* Extract short truncated BSR value */
        	break;

        case UL_SCH_LCID_L_BSR:
        	//38.321 section 6.1.3.1
        	//variable length
        	mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
        	mac_subheader_len = 2;
        	if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
        		mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
        		mac_subheader_len = 3;
        	}
        	/* Extract long BSR value */
        	break;

        case UL_SCH_LCID_L_TRUNCATED_BSR:
        	//38.321 section 6.1.3.1
        	//variable length
        	mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
        	mac_subheader_len = 2;
        	if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
        		mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
        		mac_subheader_len = 3;
        	}
        	/* Extract long truncated BSR value */
        	break;


        case UL_SCH_LCID_C_RNTI:
        	//38.321 section 6.1.3.2
        	//fixed length
        	mac_ce_len = 2;
        	/* Extract CRNTI value */
        	break;

        case UL_SCH_LCID_SINGLE_ENTRY_PHR:
        	//38.321 section 6.1.3.8
        	//fixed length
        	mac_ce_len = 2;
        	/* Extract SINGLE ENTRY PHR elements for PHR calculation */
        	break;

        case UL_SCH_LCID_MULTI_ENTRY_PHR_1_OCT:
        	//38.321 section 6.1.3.9
        	//  varialbe length
        	mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
        	mac_subheader_len = 2;
        	if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
        		mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
        		mac_subheader_len = 3;
        	}
        	/* Extract MULTI ENTRY PHR elements from single octet bitmap for PHR calculation */
        	break;

        case UL_SCH_LCID_MULTI_ENTRY_PHR_4_OCT:
        	//38.321 section 6.1.3.9
        	//  varialbe length
        	mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
        	mac_subheader_len = 2;
        	if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
        		mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
        		mac_subheader_len = 3;
        	}
        	/* Extract MULTI ENTRY PHR elements from four octets bitmap for PHR calculation */
        	break;

        case UL_SCH_LCID_PADDING:
        	done = 1;
        	//  end of MAC PDU, can ignore the rest.
        	break;

        // MAC SDUs
        case UL_SCH_LCID_SRB1:
              // todo
              break;
        case UL_SCH_LCID_SRB2:
              // todo
              break;
        case UL_SCH_LCID_SRB3:
              // todo
              break;
        case UL_SCH_LCID_CCCH_MSG3:
              // todo
              break;
        case UL_SCH_LCID_CCCH:
              // todo
              mac_subheader_len = 2;
              break;

        case UL_SCH_LCID_DTCH:
                //  check if LCID is valid at current time.
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    //mac_sdu_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                    mac_sdu_len = ((uint16_t)(((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L1 & 0x7f) << 8)
                    | ((uint16_t)((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L2 & 0xff);

                } else {
                  mac_sdu_len = (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                  mac_subheader_len = 2;
                }

                LOG_D(MAC, "[UE %d] Frame %d : ULSCH -> UL-DTCH %d (gNB %d, %d bytes)\n", module_idP, frameP, rx_lcid, module_idP, mac_sdu_len);
		int UE_id = find_nr_UE_id(module_idP, rnti);
		RC.nrmac[module_idP]->UE_info.mac_stats[UE_id].lc_bytes_rx[rx_lcid] += mac_sdu_len;
                #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
		    log_dump(MAC, pdu_ptr + mac_subheader_len, 32, LOG_DUMP_CHAR, "\n");

                #endif
                if(IS_SOFTMODEM_NOS1){
                  mac_rlc_data_ind(module_idP,
                      0x1234,
                      module_idP,
                      frameP,
                      ENB_FLAG_YES,
                      MBMS_FLAG_NO,
                      rx_lcid,
                      (char *) (pdu_ptr + mac_subheader_len),
                      mac_sdu_len,
                      1,
                      NULL);
                }
                else{
                  mac_rlc_data_ind(module_idP,
                      rnti,
                      module_idP,
                      frameP,
                      ENB_FLAG_YES,
                      MBMS_FLAG_NO,
                      rx_lcid,
                      (char *) (pdu_ptr + mac_subheader_len),
                      mac_sdu_len,
                      1,
                      NULL);
                }


            break;

        default:
        	return;
        	break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );

        if (pdu_len < 0) {
          LOG_E(MAC, "%s() residual mac pdu length < 0!, pdu_len: %d\n", __func__, pdu_len);
          return;
        }
    }
}

void handle_nr_ul_harq(uint16_t slot, NR_UE_sched_ctrl_t *sched_ctrl, NR_mac_stats_t *stats, nfapi_nr_crc_t crc_pdu) {

  int max_harq_rounds = 4; // TODO define macro
  uint8_t hrq_id = crc_pdu.harq_id;
  NR_UE_ul_harq_t *cur_harq = &sched_ctrl->ul_harq_processes[hrq_id];
  if (cur_harq->state==ACTIVE_SCHED) {
    if (!crc_pdu.tb_crc_status) {
      cur_harq->ndi ^= 1;
      cur_harq->round = 0;
      cur_harq->state = INACTIVE; // passed -> make inactive. can be used by scheduder for next grant
#ifdef UL_HARQ_PRINT
      printf("[HARQ HANDLER] Ulharq id %d crc passed, freeing it for scheduler\n",hrq_id);
#endif
    } else {
      cur_harq->round++;
      cur_harq->state = ACTIVE_NOT_SCHED;
#ifdef UL_HARQ_PRINT
      printf("[HARQ HANDLER] Ulharq id %d crc failed, requesting retransmission\n",hrq_id);
#endif
    }

    if (!(cur_harq->round<max_harq_rounds)) {
      cur_harq->ndi ^= 1;
      cur_harq->state = INACTIVE; // failed after 4 rounds -> make inactive
      cur_harq->round = 0;
      LOG_D(MAC,"[HARQ HANDLER] RNTI %x: Ulharq id %d crc failed in all round, freeing it for scheduler\n",crc_pdu.rnti,hrq_id);
      stats->ulsch_errors++;
    }
    return;
  } else
    LOG_W(MAC,"Incorrect ULSCH HARQ process %d or invalid state %d (ignore this warning for RA)\n",hrq_id,cur_harq->state);
}

/*
* When data are received on PHY and transmitted to MAC
*/
void nr_rx_sdu(const module_id_t gnb_mod_idP,
               const int CC_idP,
               const frame_t frameP,
               const sub_frame_t slotP,
               const rnti_t rntiP,
               uint8_t *sduP,
               const uint16_t sdu_lenP,
               const uint16_t timing_advance,
               const uint8_t ul_cqi,
               const uint16_t rssi){
  int current_rnti = 0, UE_id = -1, harq_pid = 0;
  gNB_MAC_INST *gNB_mac = NULL;
  NR_UE_info_t *UE_info = NULL;
  NR_UE_sched_ctrl_t *UE_scheduling_control = NULL;

  current_rnti = rntiP;
  UE_id = find_nr_UE_id(gnb_mod_idP, current_rnti);
  gNB_mac = RC.nrmac[gnb_mod_idP];
  UE_info = &gNB_mac->UE_info;
  int target_snrx10 = gNB_mac->pusch_target_snrx10;

  if (sduP != NULL) {
    T(T_GNB_MAC_UL_PDU_WITH_DATA, T_INT(gnb_mod_idP), T_INT(CC_idP),
      T_INT(rntiP), T_INT(frameP), T_INT(slotP), T_INT(-1) /* harq_pid */,
      T_BUFFER(sduP, sdu_lenP));
  }

  if (UE_id != -1) {
    UE_scheduling_control = &(UE_info->UE_sched_ctrl[UE_id]);

    UE_info->mac_stats[UE_id].ulsch_total_bytes_rx += sdu_lenP;
    LOG_D(MAC, "[gNB %d][PUSCH %d] CC_id %d %d.%d Received ULSCH sdu from PHY (rnti %x, UE_id %d) ul_cqi %d\n",
          gnb_mod_idP,
          harq_pid,
          CC_idP,
          frameP,
          slotP,
          current_rnti,
          UE_id,
          ul_cqi);

    // if not missed detection (10dB threshold for now)
    if (UE_scheduling_control->ul_rssi < (100+rssi)) {
      UE_scheduling_control->tpc0 = nr_get_tpc(target_snrx10,ul_cqi,30);
      if (timing_advance != 0xffff)
        UE_scheduling_control->ta_update = timing_advance;
      UE_scheduling_control->ul_rssi = rssi;
      LOG_D(MAC, "[UE %d] PUSCH TPC %d and TA %d\n",UE_id,UE_scheduling_control->tpc0,UE_scheduling_control->ta_update);
    }
    else{
      UE_scheduling_control->tpc0 = 1;
    }

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)

    LOG_I(MAC, "Printing received UL MAC payload at gNB side: %d \n");
    for (int i = 0; i < sdu_lenP ; i++) {
	  //harq_process_ul_ue->a[i] = (unsigned char) rand();
	  //printf("a[%d]=0x%02x\n",i,harq_process_ul_ue->a[i]);
	  printf("%02x ",(unsigned char)sduP[i]);
    }
    printf("\n");

#endif

    if (sduP != NULL){
      LOG_D(MAC, "Received PDU at MAC gNB \n");
      nr_process_mac_pdu(gnb_mod_idP, current_rnti, CC_idP, frameP, sduP, sdu_lenP);
    }
    else {

    }
  } else {
    if (!sduP) // check that CRC passed
      return;

    /* we don't know this UE (yet). Check whether there is a ongoing RA (Msg 3)
     * and check the corresponding UE's RNTI match, in which case we activate
     * it. */
    for (int i = 0; i < NR_NB_RA_PROC_MAX; ++i) {
      NR_RA_t *ra = &gNB_mac->common_channels[CC_idP].ra[i];
      if (ra->state != WAIT_Msg3)
        continue;

      // random access pusch with TC-RNTI
      if (ra->rnti != current_rnti) {
        LOG_W(MAC,
              "expected TC-RNTI %04x to match current RNTI %04x\n",
              ra->rnti,
              current_rnti);
        continue;
      }
      const int UE_id = add_new_nr_ue(gnb_mod_idP, ra->rnti);
      UE_info->secondaryCellGroup[UE_id] = ra->secondaryCellGroup;
      compute_csi_bitlen(ra->secondaryCellGroup, UE_info, UE_id);
      UE_info->UE_beam_index[UE_id] = ra->beam_id;
      struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = ra->secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList;
      AssertFatal(bwpList->list.count == 1,
                  "downlinkBWP_ToAddModList has %d BWP!\n",
                  bwpList->list.count);
      const int bwp_id = 1;
      UE_info->UE_sched_ctrl[UE_id].active_bwp = bwpList->list.array[bwp_id - 1];
      struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = ra->secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList;
      AssertFatal(ubwpList->list.count == 1,
                  "uplinkBWP_ToAddModList has %d BWP!\n",
                  ubwpList->list.count);
      UE_info->UE_sched_ctrl[UE_id].active_ubwp = ubwpList->list.array[bwp_id - 1];
      LOG_I(MAC,
            "[gNB %d][RAPROC] PUSCH with TC_RNTI %x received correctly, "
            "adding UE MAC Context UE_id %d/RNTI %04x\n",
            gnb_mod_idP,
            current_rnti,
            UE_id,
            ra->rnti);
      // re-initialize ta update variables afrer RA procedure completion
      UE_info->UE_sched_ctrl[UE_id].ta_frame = frameP;

      free(ra->preambles.preamble_list);
      ra->state = RA_IDLE;
      LOG_I(MAC,
            "reset RA state information for RA-RNTI %04x/index %d\n",
            ra->rnti,
            i);
      return;
    }
  }
}

long get_K2(NR_BWP_Uplink_t *ubwp, int time_domain_assignment, int mu) {
  DevAssert(ubwp);
  const NR_PUSCH_TimeDomainResourceAllocation_t *tda_list = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[time_domain_assignment];
  if (tda_list->k2)
    return *tda_list->k2;
  else if (mu < 2)
    return 1;
  else if (mu == 2)
    return 2;
  else
    return 3;
}

int8_t select_ul_harq_pid(NR_UE_sched_ctrl_t *sched_ctrl) {
  const uint8_t max_ul_harq_pids = 3; // temp: for testing
  // schedule active harq processes
  for (uint8_t hrq_id = 0; hrq_id < max_ul_harq_pids; hrq_id++) {
    NR_UE_ul_harq_t *cur_harq = &sched_ctrl->ul_harq_processes[hrq_id];
    if (cur_harq->state == ACTIVE_NOT_SCHED) {
      LOG_D(MAC, "Found ulharq id %d, scheduling it for retransmission\n", hrq_id);
      return hrq_id;
    }
  }

  // schedule new harq processes
  for (uint8_t hrq_id=0; hrq_id < max_ul_harq_pids; hrq_id++) {
    NR_UE_ul_harq_t *cur_harq = &sched_ctrl->ul_harq_processes[hrq_id];
    if (cur_harq->state == INACTIVE) {
      LOG_D(MAC, "Found new ulharq id %d, scheduling it\n", hrq_id);
      return hrq_id;
    }
  }
  LOG_E(MAC, "All UL HARQ processes are busy. Cannot schedule ULSCH\n");
  return -1;
}

void nr_simple_ulsch_preprocessor(module_id_t module_id,
                                  frame_t frame,
                                  sub_frame_t slot,
                                  int num_slots_per_tdd,
                                  uint64_t ulsch_in_slot_bitmap) {
  gNB_MAC_INST *nr_mac = RC.nrmac[module_id];
  NR_COMMON_channels_t *cc = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  const int mu = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  NR_UE_info_t *UE_info = &nr_mac->UE_info;

  AssertFatal(UE_info->num_UEs <= 1,
              "%s() cannot handle more than one UE, but found %d\n",
              __func__,
              UE_info->num_UEs);
  if (UE_info->num_UEs == 0)
    return;

  const int UE_id = 0;
  const int CC_id = 0;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  const int tda = 1;
  const struct NR_PUSCH_TimeDomainResourceAllocationList *tdaList =
    sched_ctrl->active_ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  AssertFatal(tda < tdaList->list.count,
              "time domain assignment %d >= %d\n",
              tda,
              tdaList->list.count);
  int K2 = get_K2(sched_ctrl->active_ubwp, tda, mu);
  const int sched_frame = frame + (slot + K2 >= num_slots_per_tdd);
  const int sched_slot = (slot + K2) % num_slots_per_tdd;
  if (!is_xlsch_in_slot(ulsch_in_slot_bitmap, sched_slot))
    return;

  /* get first, largest unallocated region */
  uint16_t *vrb_map_UL =
      &RC.nrmac[module_id]->common_channels[CC_id].vrb_map_UL[sched_slot * 275];
  uint16_t rbStart = 0;
  while (vrb_map_UL[rbStart]) rbStart++;
  const uint16_t bwpSize = NRRIV2BW(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  uint16_t rbSize = 1;
  while (rbStart + rbSize < bwpSize && !vrb_map_UL[rbStart+rbSize])
    rbSize++;

  sched_ctrl->sched_pusch.time_domain_allocation = tda;
  sched_ctrl->sched_pusch.slot = sched_slot;
  sched_ctrl->sched_pusch.frame = sched_frame;

  const int target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  sched_ctrl->search_space = get_searchspace(sched_ctrl->active_bwp, target_ss);
  uint8_t nr_of_candidates;
  find_aggregation_candidates(&sched_ctrl->aggregation_level,
                              &nr_of_candidates,
                              sched_ctrl->search_space);
  sched_ctrl->coreset = get_coreset(
      sched_ctrl->active_bwp, sched_ctrl->search_space, 1 /* dedicated */);
  const int cid = sched_ctrl->coreset->controlResourceSetId;
  const uint16_t Y = UE_info->Y[UE_id][cid][slot];
  const int m = UE_info->num_pdcch_cand[UE_id][cid];
  sched_ctrl->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                           sched_ctrl->active_bwp,
                                           sched_ctrl->coreset,
                                           sched_ctrl->aggregation_level,
                                           Y,
                                           m,
                                           nr_of_candidates);
  if (sched_ctrl->cce_index < 0) {
    LOG_E(MAC, "%s(): CCE list not empty, couldn't schedule PUSCH\n", __func__);
    return;
  }
  UE_info->num_pdcch_cand[UE_id][cid]++;

  sched_ctrl->sched_pusch.mcs = 9;
  sched_ctrl->sched_pusch.rbStart = rbStart;
  sched_ctrl->sched_pusch.rbSize = rbSize;

  /* mark the corresponding RBs as used */
  for (int rb = 0; rb < sched_ctrl->sched_pusch.rbSize; rb++)
    vrb_map_UL[rb + sched_ctrl->sched_pusch.rbStart] = 1;
}

void nr_schedule_ulsch(module_id_t module_id,
                       frame_t frame,
                       sub_frame_t slot,
                       int num_slots_per_tdd,
                       int ul_slots,
                       uint64_t ulsch_in_slot_bitmap) {
  RC.nrmac[module_id]->pre_processor_ul(
      module_id, frame, slot, num_slots_per_tdd, ulsch_in_slot_bitmap);

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  const NR_UE_list_t *UE_list = &UE_info->list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    if (sched_ctrl->sched_pusch.rbSize <= 0)
      continue;

    uint16_t rnti = UE_info->rnti[UE_id];

    NR_PUSCH_Config_t *pusch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pusch_Config->choice.setup;
    uint8_t transform_precoding = 0;
    if (!pusch_Config->transformPrecoder)
      transform_precoding = !scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder;
    else
      transform_precoding = *pusch_Config->transformPrecoder;

    /* PUSCH in a later slot, but corresponding DCI now! */
    nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_id]->UL_tti_req_ahead[0][sched_ctrl->sched_pusch.slot];
    AssertFatal(future_ul_tti_req->SFN == sched_ctrl->sched_pusch.frame
                && future_ul_tti_req->Slot == sched_ctrl->sched_pusch.slot,
                "%d.%d future UL_tti_req's frame.slot %d.%d does not match PUSCH %d.%d\n",
                frame, slot,
                future_ul_tti_req->SFN,
                future_ul_tti_req->Slot,
                sched_ctrl->sched_pusch.frame,
                sched_ctrl->sched_pusch.slot);
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
    nfapi_nr_pusch_pdu_t *pusch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pusch_pdu;
    memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));
    future_ul_tti_req->n_pdus += 1;

    LOG_D(MAC, "%4d.%2d Scheduling UE specific PUSCH\n", frame, slot);

    int dci_formats[2];
    if (sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats)
      dci_formats[0]  = NR_UL_DCI_FORMAT_0_1;
    else
      dci_formats[0]  = NR_UL_DCI_FORMAT_0_0;
    int rnti_types[2] = { NR_RNTI_C, 0 };

    //Resource Allocation in time domain
    const int tda = sched_ctrl->sched_pusch.time_domain_allocation;
    const struct NR_PUSCH_TimeDomainResourceAllocationList *tdaList =
      sched_ctrl->active_ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
    const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
    int StartSymbolIndex, NrOfSymbols;
    SLIV2SL(startSymbolAndLength,&StartSymbolIndex,&NrOfSymbols);

    pusch_pdu->start_symbol_index = StartSymbolIndex;
    pusch_pdu->nr_of_symbols = NrOfSymbols;

    pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
    pusch_pdu->rnti = rnti;
    pusch_pdu->handle = 0; //not yet used

    pusch_pdu->bwp_size  = NRRIV2BW(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
    pusch_pdu->bwp_start = NRRIV2PRBOFFSET(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
    pusch_pdu->subcarrier_spacing = sched_ctrl->active_ubwp->bwp_Common->genericParameters.subcarrierSpacing;
    pusch_pdu->cyclic_prefix = 0;

    if (pusch_Config->dataScramblingIdentityPUSCH)
      pusch_pdu->data_scrambling_id = *pusch_Config->dataScramblingIdentityPUSCH;
    else
      pusch_pdu->data_scrambling_id = *scc->physCellId;

    pusch_pdu->transform_precoding = transform_precoding;
    pusch_pdu->mcs_index = sched_ctrl->sched_pusch.mcs;
    const int target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
    if (pusch_pdu->transform_precoding)
      pusch_pdu->mcs_table = get_pusch_mcs_table(pusch_Config->mcs_Table,
                                                 0,
                                                 dci_formats[0],
                                                 rnti_types[0],
                                                 target_ss,
                                                 false);
    else
      pusch_pdu->mcs_table =
          get_pusch_mcs_table(pusch_Config->mcs_TableTransformPrecoder,
                              1,
                              dci_formats[0],
                              rnti_types[0],
                              target_ss,
                              false);

    pusch_pdu->target_code_rate = nr_get_code_rate_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
    pusch_pdu->qam_mod_order = nr_get_Qm_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
    if (pusch_Config->tp_pi2BPSK) {
      if (((pusch_pdu->mcs_table == 3) && (pusch_pdu->mcs_index < 2))
          || ((pusch_pdu->mcs_table == 4) && (pusch_pdu->mcs_index < 6))) {
        pusch_pdu->target_code_rate = pusch_pdu->target_code_rate>>1;
        pusch_pdu->qam_mod_order = pusch_pdu->qam_mod_order<<1;
      }
    }
    pusch_pdu->nrOfLayers = 1;

    //Pusch Allocation in frequency domain [TS38.214, sec 6.1.2.2]
    AssertFatal(pusch_Config->resourceAllocation == NR_PUSCH_Config__resourceAllocation_resourceAllocationType1,
                "Only frequency resource allocation type 1 is currently supported\n");
    pusch_pdu->resource_alloc = 1; //type 1
    pusch_pdu->rb_start = sched_ctrl->sched_pusch.rbStart;
    pusch_pdu->rb_size = sched_ctrl->sched_pusch.rbSize;
    pusch_pdu->vrb_to_prb_mapping = 0;

    if (pusch_Config->frequencyHopping==NULL)
      pusch_pdu->frequency_hopping = 0;
    else
      pusch_pdu->frequency_hopping = 1;


    // --------------------
    // ------- DMRS -------
    // --------------------
    const int mapping_type = tdaList->list.array[tda]->mappingType;
    NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig;
    if (mapping_type == NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeA)
      NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup;
    else
      NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup;
    if (NR_DMRS_UplinkConfig->dmrs_Type == NULL)
      pusch_pdu->dmrs_config_type = 0;
    else
      pusch_pdu->dmrs_config_type = 1;
    pusch_pdu->scid = 0;      // DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]
    if (pusch_pdu->transform_precoding) { // transform precoding disabled
      long *scramblingid;
      if (pusch_pdu->scid == 0)
        scramblingid = NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID0;
      else
        scramblingid = NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID1;
      if (scramblingid == NULL)
        pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
      else
        pusch_pdu->ul_dmrs_scrambling_id = *scramblingid;
    }
    else {
      pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
      if (NR_DMRS_UplinkConfig->transformPrecodingEnabled->nPUSCH_Identity != NULL)
        pusch_pdu->pusch_identity = *NR_DMRS_UplinkConfig->transformPrecodingEnabled->nPUSCH_Identity;
      else
        pusch_pdu->pusch_identity = *scc->physCellId;
    }
    pusch_dmrs_AdditionalPosition_t additional_pos;
    if (NR_DMRS_UplinkConfig->dmrs_AdditionalPosition == NULL)
      additional_pos = 2;
    else {
      if (*NR_DMRS_UplinkConfig->dmrs_AdditionalPosition == NR_DMRS_UplinkConfig__dmrs_AdditionalPosition_pos3)
        additional_pos = 3;
      else
        additional_pos = *NR_DMRS_UplinkConfig->dmrs_AdditionalPosition;
    }
    pusch_maxLength_t pusch_maxLength;
    if (NR_DMRS_UplinkConfig->maxLength == NULL)
      pusch_maxLength = 1;
    else
      pusch_maxLength = 2;
    uint16_t l_prime_mask = get_l_prime(pusch_pdu->nr_of_symbols, mapping_type, additional_pos, pusch_maxLength);
    pusch_pdu->ul_dmrs_symb_pos = l_prime_mask << pusch_pdu->start_symbol_index;

    pusch_pdu->num_dmrs_cdm_grps_no_data = 1;
    pusch_pdu->dmrs_ports = 1;

    // --------------------
    // ------- PTRS -------
    // --------------------
    if (NR_DMRS_UplinkConfig->phaseTrackingRS != NULL) {
      // TODO to be fixed from RRC config
      uint8_t ptrs_mcs1 = 2;  // higher layer parameter in PTRS-UplinkConfig
      uint8_t ptrs_mcs2 = 4;  // higher layer parameter in PTRS-UplinkConfig
      uint8_t ptrs_mcs3 = 10; // higher layer parameter in PTRS-UplinkConfig
      uint16_t n_rb0 = 25;    // higher layer parameter in PTRS-UplinkConfig
      uint16_t n_rb1 = 75;    // higher layer parameter in PTRS-UplinkConfig
      pusch_pdu->pusch_ptrs.ptrs_time_density = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, pusch_pdu->mcs_index, pusch_pdu->mcs_table);
      pusch_pdu->pusch_ptrs.ptrs_freq_density = get_K_ptrs(n_rb0, n_rb1, pusch_pdu->rb_size);
      pusch_pdu->pusch_ptrs.ptrs_ports_list   = (nfapi_nr_ptrs_ports_t *) malloc(2*sizeof(nfapi_nr_ptrs_ports_t));
      pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;

      pusch_pdu->pdu_bit_map |= PUSCH_PDU_BITMAP_PUSCH_PTRS; // enable PUSCH PTRS
    }
    else{
      pusch_pdu->pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS
    }

    int8_t harq_id = select_ul_harq_pid(&UE_info->UE_sched_ctrl[UE_id]);
    if (harq_id < 0) return;
    NR_UE_ul_harq_t *cur_harq = &UE_info->UE_sched_ctrl[UE_id].ul_harq_processes[harq_id];
    pusch_pdu->pusch_data.harq_process_id = harq_id;
    pusch_pdu->pusch_data.new_data_indicator = cur_harq->ndi;
    pusch_pdu->pusch_data.rv_index = nr_rv_round_map[cur_harq->round];

    cur_harq->state = ACTIVE_SCHED;
    cur_harq->last_tx_slot = sched_ctrl->sched_pusch.slot;

    uint8_t num_dmrs_symb = 0;
    for(int i = pusch_pdu->start_symbol_index; i < pusch_pdu->start_symbol_index + pusch_pdu->nr_of_symbols; i++)
      num_dmrs_symb += (pusch_pdu->ul_dmrs_symb_pos >> i) & 1;

    uint8_t N_PRB_DMRS;
    if (pusch_pdu->dmrs_config_type == 0)
      N_PRB_DMRS = pusch_pdu->num_dmrs_cdm_grps_no_data*6;
    else
      N_PRB_DMRS = pusch_pdu->num_dmrs_cdm_grps_no_data*4;

    pusch_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_pdu->qam_mod_order,
                                                   pusch_pdu->target_code_rate,
                                                   pusch_pdu->rb_size,
                                                   pusch_pdu->nr_of_symbols,
                                                   N_PRB_DMRS * num_dmrs_symb,
                                                   0, //nb_rb_oh
                                                   0,
                                                   pusch_pdu->nrOfLayers)>>3;

    UE_info->mac_stats[UE_id].ulsch_rounds[cur_harq->round]++;
    if (cur_harq->round == 0)
      UE_info->mac_stats[UE_id].ulsch_total_bytes_scheduled += pusch_pdu->pusch_data.tb_size;

    pusch_pdu->pusch_data.num_cb = 0; //CBG not supported

    nfapi_nr_ul_dci_request_t *ul_dci_req = &RC.nrmac[module_id]->UL_dci_req[0];
    ul_dci_req->SFN = frame;
    ul_dci_req->Slot = slot;
    nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu = &ul_dci_req->ul_dci_pdu_list[ul_dci_req->numPdus];
    memset(ul_dci_request_pdu, 0, sizeof(nfapi_nr_ul_dci_request_pdus_t));
    ul_dci_request_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    ul_dci_request_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &ul_dci_request_pdu->pdcch_pdu.pdcch_pdu_rel15;
    ul_dci_req->numPdus += 1;

    LOG_D(MAC,"Configuring ULDCI/PDCCH in %d.%d\n", frame,slot);

    nr_configure_pdcch(RC.nrmac[0],
                       pdcch_pdu_rel15,
                       rnti,
                       sched_ctrl->search_space,
                       sched_ctrl->coreset,
                       scc,
                       sched_ctrl->active_bwp,
                       sched_ctrl->aggregation_level,
                       sched_ctrl->cce_index);

    dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
    memset(dci_pdu_rel15, 0, sizeof(dci_pdu_rel15));
    NR_CellGroupConfig_t *secondaryCellGroup = UE_info->secondaryCellGroup[UE_id];
    const int n_ubwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count;
    config_uldci(sched_ctrl->active_ubwp,
                 pusch_pdu,
                 pdcch_pdu_rel15,
                 &dci_pdu_rel15[0],
                 dci_formats,
                 tda,
                 UE_info->UE_sched_ctrl[UE_id].tpc0,
                 n_ubwp,
                 sched_ctrl->active_bwp->bwp_Id);
    fill_dci_pdu_rel15(scc,
                       secondaryCellGroup,
                       pdcch_pdu_rel15,
                       dci_pdu_rel15,
                       dci_formats,
                       rnti_types,
                       pusch_pdu->bwp_size,
                       sched_ctrl->active_bwp->bwp_Id);

    memset(&sched_ctrl->sched_pusch, 0, sizeof(sched_ctrl->sched_pusch));
  }
}
