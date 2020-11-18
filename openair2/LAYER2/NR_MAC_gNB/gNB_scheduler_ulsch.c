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
//#define ENABLE_MAC_PAYLOAD_DEBUG 1


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
    LOG_E(MAC,"Incorrect ULSCH HARQ process %d or invalid state %d\n",hrq_id,cur_harq->state);
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
