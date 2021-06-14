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
#include "nfapi/oai_integration/vendor_ext.h"

//38.321 Table 6.1.3.1-1
const uint32_t NR_SHORT_BSR_TABLE[32] = {
    0,    10,    14,    20,    28,     38,     53,     74,
  102,   142,   198,   276,   384,    535,    745,   1038,
 1446,  2014,  2806,  3909,  5446,   7587,  10570,  14726,
20516, 28581, 39818, 55474, 77284, 107669, 150000, 300000
};

//38.321 Table 6.1.3.1-2
const uint32_t NR_LONG_BSR_TABLE[256] ={
       0,       10,       11,       12,       13,       14,       15,       16,       17,       18,       19,       20,       22,       23,        25,         26,
      28,       30,       32,       34,       36,       38,       40,       43,       46,       49,       52,       55,       59,       62,        66,         71,
      75,       80,       85,       91,       97,      103,      110,      117,      124,      132,      141,      150,      160,      170,       181,        193,
     205,      218,      233,      248,      264,      281,      299,      318,      339,      361,      384,      409,      436,      464,       494,        526,
     560,      597,      635,      677,      720,      767,      817,      870,      926,      987,     1051,     1119,     1191,     1269,      1351,       1439,
    1532,     1631,     1737,     1850,     1970,     2098,     2234,     2379,     2533,     2698,     2873,     3059,     3258,     3469,      3694,       3934,
    4189,     4461,     4751,     5059,     5387,     5737,     6109,     6506,     6928,     7378,     7857,     8367,     8910,     9488,     10104,      10760,
   11458,    12202,    12994,    13838,    14736,    15692,    16711,    17795,    18951,    20181,    21491,    22885,    24371,    25953,     27638,      29431,
   31342,    33376,    35543,    37850,    40307,    42923,    45709,    48676,    51836,    55200,    58784,    62599,    66663,    70990,     75598,      80505,
   85730,    91295,    97221,   103532,   110252,   117409,   125030,   133146,   141789,   150992,   160793,   171231,   182345,   194182,    206786,     220209,
  234503,   249725,   265935,   283197,   301579,   321155,   342002,   364202,   387842,   413018,   439827,   468377,   498780,   531156,    565634,     602350,
  641449,   683087,   727427,   774645,   824928,   878475,   935498,   996222,  1060888,  1129752,  1203085,  1281179,  1364342,  1452903,   1547213,    1647644,
 1754595,  1868488,  1989774,  2118933,  2256475,  2402946,  2558924,  2725027,  2901912,  3090279,  3290873,  3504487,  3731968,  3974215,   4232186,    4506902,
 4799451,  5110989,  5442750,  5796046,  6172275,  6572925,  6999582,  7453933,  7937777,  8453028,  9001725,  9586039, 10208280, 10870913,  11576557,   12328006,
13128233, 13980403, 14887889, 15854280, 16883401, 17979324, 19146385, 20389201, 21712690, 23122088, 24622972, 26221280, 27923336, 29735875,  31666069,   33721553,
35910462, 38241455, 40723756, 43367187, 46182206, 49179951, 52372284, 55771835, 59392055, 63247269, 67352729, 71724679, 76380419, 81338368, 162676736, 4294967295
};

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


    NR_UE_info_t *UE_info = &RC.nrmac[module_idP]->UE_info;
    int UE_id = find_nr_UE_id(module_idP, rnti);
    if (UE_id == -1) {
      LOG_E(MAC, "%s() UE_id == -1\n",__func__);
      return;
    }
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
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

        unsigned char *ce_ptr;
        int n_Lcg = 0;

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
        case UL_SCH_LCID_S_TRUNCATED_BSR:
        	//38.321 section 6.1.3.1
        	//fixed length
        	mac_ce_len =1;
        	/* Extract short BSR value */
               ce_ptr = &pdu_ptr[mac_subheader_len];
               NR_BSR_SHORT *bsr_s = (NR_BSR_SHORT *) ce_ptr;
               sched_ctrl->estimated_ul_buffer = 0;
               sched_ctrl->estimated_ul_buffer = NR_SHORT_BSR_TABLE[bsr_s->Buffer_size];
               LOG_D(MAC, "SHORT BSR, LCG ID %d, BS Index %d, BS value < %d, est buf %d\n",
                     bsr_s->LcgID,
                     bsr_s->Buffer_size,
                     NR_SHORT_BSR_TABLE[bsr_s->Buffer_size],
                     sched_ctrl->estimated_ul_buffer);
        	break;

        case UL_SCH_LCID_L_BSR:
        case UL_SCH_LCID_L_TRUNCATED_BSR:
        	//38.321 section 6.1.3.1
        	//variable length
        	mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
        	mac_subheader_len = 2;
        	if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
        		mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
        		mac_subheader_len = 3;
        	}
        	/* Extract long BSR value */
               ce_ptr = &pdu_ptr[mac_subheader_len];
               NR_BSR_LONG *bsr_l = (NR_BSR_LONG *) ce_ptr;
               sched_ctrl->estimated_ul_buffer = 0;

               n_Lcg = bsr_l->LcgID7 + bsr_l->LcgID6 + bsr_l->LcgID5 + bsr_l->LcgID4 +
                       bsr_l->LcgID3 + bsr_l->LcgID2 + bsr_l->LcgID1 + bsr_l->LcgID0;

               LOG_D(MAC, "LONG BSR, LCG ID(7-0) %d/%d/%d/%d/%d/%d/%d/%d\n",
                     bsr_l->LcgID7, bsr_l->LcgID6, bsr_l->LcgID5, bsr_l->LcgID4,
                     bsr_l->LcgID3, bsr_l->LcgID2, bsr_l->LcgID1, bsr_l->LcgID0);

               for (int n = 0; n < n_Lcg; n++){
                 LOG_D(MAC, "LONG BSR, %d/%d (n/n_Lcg), BS Index %d, BS value < %d",
                       n, n_Lcg, pdu_ptr[mac_subheader_len + 1 + n],
                       NR_LONG_BSR_TABLE[pdu_ptr[mac_subheader_len + 1 + n]]);
                 sched_ctrl->estimated_ul_buffer +=
                       NR_LONG_BSR_TABLE[pdu_ptr[mac_subheader_len + 1 + n]];
               }

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

                /* Updated estimated buffer when receiving data */
                if (sched_ctrl->estimated_ul_buffer >= mac_sdu_len)
                  sched_ctrl->estimated_ul_buffer -= mac_sdu_len;
                else
                  sched_ctrl->estimated_ul_buffer = 0;

            break;

        default:
          LOG_D(MAC, "Received unknown MAC header (LCID = 0x%02x)\n", rx_lcid);
          return;
          break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );

        if (pdu_len < 0) {
          LOG_E(MAC, "%s() residual mac pdu length < 0!, pdu_len: %d\n", __func__, pdu_len);
          LOG_E(MAC, "MAC PDU ");
          for (int i = 0; i < 20; i++) // Only printf 1st - 20nd bytes
            printf("%02x ", pdu_ptr[i]);
          printf("\n");
          return;
        }
    }
}

void abort_nr_ul_harq(module_id_t mod_id, int UE_id, int8_t harq_pid)
{
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  NR_UE_ul_harq_t *harq = &sched_ctrl->ul_harq_processes[harq_pid];

  harq->ndi ^= 1;
  harq->round = 0;
  UE_info->mac_stats[UE_id].ulsch_errors++;
  add_tail_nr_list(&sched_ctrl->available_ul_harq, harq_pid);

  /* the transmission failed: the UE won't send the data we expected initially,
   * so retrieve to correctly schedule after next BSR */
  sched_ctrl->sched_ul_bytes -= harq->sched_pusch.tb_size;
  if (sched_ctrl->sched_ul_bytes < 0)
    sched_ctrl->sched_ul_bytes = 0;
}

void handle_nr_ul_harq(module_id_t mod_id,
                       frame_t frame,
                       sub_frame_t slot,
                       const nfapi_nr_crc_t *crc_pdu)
{
  int UE_id = find_nr_UE_id(mod_id, crc_pdu->rnti);
  if (UE_id < 0) {
    LOG_E(MAC, "%s(): unknown RNTI %04x in PUSCH\n", __func__, crc_pdu->rnti);
    return;
  }
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  int8_t harq_pid = sched_ctrl->feedback_ul_harq.head;
  while (crc_pdu->harq_id != harq_pid || harq_pid < 0) {
    LOG_W(MAC,
          "Unexpected ULSCH HARQ PID %d (have %d) for RNTI %04x (ignore this warning for RA)\n",
          crc_pdu->harq_id,
          harq_pid,
          crc_pdu->rnti);
    if (harq_pid < 0)
      return;

    remove_front_nr_list(&sched_ctrl->feedback_ul_harq);
    sched_ctrl->ul_harq_processes[harq_pid].is_waiting = false;
    if(sched_ctrl->ul_harq_processes[harq_pid].round >= MAX_HARQ_ROUNDS - 1) {
      abort_nr_ul_harq(mod_id, UE_id, harq_pid);
    } else {
      sched_ctrl->ul_harq_processes[harq_pid].round++;
      add_tail_nr_list(&sched_ctrl->retrans_ul_harq, harq_pid);
    }
    harq_pid = sched_ctrl->feedback_ul_harq.head;
  }
  remove_front_nr_list(&sched_ctrl->feedback_ul_harq);
  NR_UE_ul_harq_t *harq = &sched_ctrl->ul_harq_processes[harq_pid];
  DevAssert(harq->is_waiting);
  harq->feedback_slot = -1;
  harq->is_waiting = false;
  if (!crc_pdu->tb_crc_status) {
    harq->ndi ^= 1;
    harq->round = 0;
    LOG_D(MAC,
          "Ulharq id %d crc passed for RNTI %04x\n",
          harq_pid,
          crc_pdu->rnti);
    add_tail_nr_list(&sched_ctrl->available_ul_harq, harq_pid);
  } else if (harq->round >= MAX_HARQ_ROUNDS - 1) {
    abort_nr_ul_harq(mod_id, UE_id, harq_pid);
    LOG_D(MAC,
          "RNTI %04x: Ulharq id %d crc failed in all rounds\n",
          crc_pdu->rnti,
          harq_pid);
  } else {
    harq->round++;
    LOG_D(MAC,
          "Ulharq id %d crc failed for RNTI %04x\n",
          harq_pid,
          crc_pdu->rnti);
    add_tail_nr_list(&sched_ctrl->retrans_ul_harq, harq_pid);
  }
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
  gNB_MAC_INST *gNB_mac = RC.nrmac[gnb_mod_idP];
  NR_UE_info_t *UE_info = &gNB_mac->UE_info;

  const int current_rnti = rntiP;
  const int UE_id = find_nr_UE_id(gnb_mod_idP, current_rnti);
  const int target_snrx10 = gNB_mac->pusch_target_snrx10;

  if (UE_id != -1) {
    NR_UE_sched_ctrl_t *UE_scheduling_control = &UE_info->UE_sched_ctrl[UE_id];
    const int8_t harq_pid = UE_scheduling_control->feedback_ul_harq.head;

    if (sduP)
      T(T_GNB_MAC_UL_PDU_WITH_DATA, T_INT(gnb_mod_idP), T_INT(CC_idP),
        T_INT(rntiP), T_INT(frameP), T_INT(slotP), T_INT(harq_pid),
        T_BUFFER(sduP, sdu_lenP));

    UE_info->mac_stats[UE_id].ulsch_total_bytes_rx += sdu_lenP;
    LOG_D(NR_MAC, "[gNB %d][PUSCH %d] CC_id %d %d.%d Received ULSCH sdu from PHY (rnti %x, UE_id %d) ul_cqi %d sduP %p\n",
          gnb_mod_idP,
          harq_pid,
          CC_idP,
          frameP,
          slotP,
          current_rnti,
          UE_id,
          ul_cqi,
          sduP);

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
      LOG_D(NR_MAC, "Received PDU at MAC gNB \n");

      const uint32_t tb_size = UE_scheduling_control->ul_harq_processes[harq_pid].sched_pusch.tb_size;
      UE_scheduling_control->sched_ul_bytes -= tb_size;
      if (UE_scheduling_control->sched_ul_bytes < 0)
        UE_scheduling_control->sched_ul_bytes = 0;

      nr_process_mac_pdu(gnb_mod_idP, current_rnti, CC_idP, frameP, sduP, sdu_lenP);
    }
  } else if(sduP) {

    bool no_sig = true;
    for (int k = 0; k < sdu_lenP; k++) {
      if(sduP[k]!=0) {
        no_sig = false;
        break;
      }
    }

    if(no_sig) {
      LOG_W(NR_MAC, "No signal\n");
    }

    T(T_GNB_MAC_UL_PDU_WITH_DATA, T_INT(gnb_mod_idP), T_INT(CC_idP),
      T_INT(rntiP), T_INT(frameP), T_INT(slotP), T_INT(-1) /* harq_pid */,
      T_BUFFER(sduP, sdu_lenP));

    /* we don't know this UE (yet). Check whether there is a ongoing RA (Msg 3)
     * and check the corresponding UE's RNTI match, in which case we activate
     * it. */
    for (int i = 0; i < NR_NB_RA_PROC_MAX; ++i) {
      NR_RA_t *ra = &gNB_mac->common_channels[CC_idP].ra[i];
      if (ra->state != WAIT_Msg3)
        continue;

      if(no_sig) {
        LOG_W(NR_MAC, "Random Access %i failed at state %i\n", i, ra->state);
        //nr_mac_remove_ra_rnti(gnb_mod_idP, ra->rnti);
        nr_clear_ra_proc(gnb_mod_idP, CC_idP, frameP);
      } else {

        // random access pusch with TC-RNTI
        if (ra->rnti != current_rnti) {
          LOG_W(NR_MAC,
                "expected TC_RNTI %04x to match current RNTI %04x\n",
                ra->rnti,
                current_rnti);

          if( (frameP==ra->Msg3_frame) && (slotP==ra->Msg3_slot) ) {
            LOG_W(NR_MAC, "Random Access %i failed at state %i\n", i, ra->state);
            //nr_mac_remove_ra_rnti(gnb_mod_idP, ra->rnti);
            nr_clear_ra_proc(gnb_mod_idP, CC_idP, frameP);
          }

          continue;
        }

        const int UE_id = add_new_nr_ue(gnb_mod_idP, ra->rnti, ra->secondaryCellGroup);
        UE_info->UE_beam_index[UE_id] = ra->beam_id;

        // re-initialize ta update variables after RA procedure completion
        UE_info->UE_sched_ctrl[UE_id].ta_frame = frameP;

        LOG_I(NR_MAC,
              "reset RA state information for RA-RNTI %04x/index %d\n",
              ra->rnti,
              i);

        LOG_I(NR_MAC,
              "[gNB %d][RAPROC] PUSCH with TC_RNTI %x received correctly, "
              "adding UE MAC Context UE_id %d/RNTI %04x\n",
              gnb_mod_idP,
              current_rnti,
              UE_id,
              ra->rnti);

        if(ra->cfra) {

          LOG_I(NR_MAC, "(ue %i, rnti 0x%04x) CFRA procedure succeeded!\n", UE_id, ra->rnti);
          //nr_mac_remove_ra_rnti(gnb_mod_idP, ra->rnti);
          nr_clear_ra_proc(gnb_mod_idP, CC_idP, frameP);
          UE_info->active[UE_id] = true;

        } else {

          LOG_I(NR_MAC,"[RAPROC] RA-Msg3 received (sdu_lenP %d)\n",sdu_lenP);
          LOG_D(NR_MAC,"[RAPROC] Received Msg3:\n");
          for (int k = 0; k < sdu_lenP; k++) {
            LOG_D(NR_MAC,"(%i): 0x%x\n",k,sduP[k]);
          }

          // UE Contention Resolution Identity
          // Store the first 48 bits belonging to the uplink CCCH SDU within Msg3 to fill in Msg4
          // First byte corresponds to R/LCID MAC sub-header
          memcpy(ra->cont_res_id, &sduP[1], sizeof(uint8_t) * 6);

          nr_process_mac_pdu(gnb_mod_idP, current_rnti, CC_idP, frameP, sduP, sdu_lenP);

          ra->state = Msg4;
          ra->Msg4_frame = ( frameP +2 ) % 1024;
          ra->Msg4_slot = 1;
          LOG_I(NR_MAC, "Scheduling RA-Msg4 for TC_RNTI %04x (state %d, frame %d, slot %d)\n", ra->rnti, ra->state, ra->Msg4_frame, ra->Msg4_slot);

        }
        return;

      }
    }
  } else {
    for (int i = 0; i < NR_NB_RA_PROC_MAX; ++i) {
      NR_RA_t *ra = &gNB_mac->common_channels[CC_idP].ra[i];
      if (ra->state != WAIT_Msg3)
        continue;

      LOG_W(NR_MAC, "Random Access %i failed at state %i\n", i, ra->state);
      //nr_mac_remove_ra_rnti(gnb_mod_idP, ra->rnti);
      nr_clear_ra_proc(gnb_mod_idP, CC_idP, frameP);
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

int next_list_entry_looped(NR_list_t *list, int UE_id)
{
  if (UE_id < 0)
    return list->head;
  return list->next[UE_id] < 0 ? list->head : list->next[UE_id];
}

float ul_thr_ue[MAX_MOBILES_PER_GNB];
int bsr0ue = -1;
void pf_ul(module_id_t module_id,
           frame_t frame,
           sub_frame_t slot,
           int num_slots_per_tdd,
           NR_list_t *UE_list,
           int n_rb_sched,
           uint8_t *rballoc_mask,
           int max_num_ue) {

  const int CC_id = 0;
  const int tda = 1;
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[CC_id].ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  const int min_rb = 5;
  float coeff_ue[MAX_MOBILES_PER_GNB];
  // UEs that could be scheduled
  int ue_array[MAX_MOBILES_PER_GNB];
  NR_list_t UE_sched = { .head = -1, .next = ue_array, .tail = -1, .len = MAX_MOBILES_PER_GNB };

  /* Hack: currently, we do not have SR, and need to schedule UEs continuously.
   * To keep the wasted resources low, we switch UEs to be scheduled in a
   * round-robin fashion below, and only schedule a UE with BSR=0 if it is the
   * selected one */
  bsr0ue = next_list_entry_looped(UE_list, bsr0ue);

  /* Loop UE_list to calculate throughput and coeff */
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    int rbStart = NRRIV2PRBOFFSET(sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    const uint16_t bwpSize = NRRIV2BW(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

    /* Calculate throughput */
    const float a = 0.0005f; // corresponds to 200ms window
    const uint32_t b = UE_info->mac_stats[UE_id].ulsch_current_bytes;
    ul_thr_ue[UE_id] = (1 - a) * ul_thr_ue[UE_id] + a * b;

    /* Save PUSCH field */
    /* we want to avoid a lengthy deduction of DMRS and other parameters in
     * every TTI if we can save it, so check whether dci_format, TDA, or
     * num_dmrs_cdm_grps_no_data has changed and only then recompute */
    sched_ctrl->sched_pusch.time_domain_allocation = tda;
    sched_ctrl->search_space = get_searchspace(sched_ctrl->active_bwp, NR_SearchSpace__searchSpaceType_PR_ue_Specific);
    sched_ctrl->coreset = get_coreset(sched_ctrl->active_bwp, sched_ctrl->search_space, 1 /* dedicated */);
    const long f = sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats;
    const int dci_format = f ? NR_UL_DCI_FORMAT_0_1 : NR_UL_DCI_FORMAT_0_0;
    const uint8_t num_dmrs_cdm_grps_no_data = 1;
    NR_sched_pusch_save_t *ps = &sched_ctrl->pusch_save;
    if (ps->time_domain_allocation != tda
        || ps->dci_format != dci_format
        || ps->num_dmrs_cdm_grps_no_data != num_dmrs_cdm_grps_no_data)
      nr_save_pusch_fields(scc, sched_ctrl->active_ubwp, dci_format, tda, num_dmrs_cdm_grps_no_data, ps);

    /* Check if retransmission is necessary */
    sched_ctrl->sched_pusch.ul_harq_pid = sched_ctrl->retrans_ul_harq.head;
    if (sched_ctrl->sched_pusch.ul_harq_pid >= 0) {
      /* RETRANSMISSION: Allocate retransmission*/
      /* Find free CCE */
      bool freeCCE = find_free_CCE(module_id, slot, UE_id);
      if (!freeCCE) {
        LOG_D(MAC, "%4d.%2d no free CCE for retransmission UL DCI UE %04x\n", frame, slot, UE_info->rnti[UE_id]);
        continue;
      }
      /* reduce max_num_ue once we are sure UE can be allocated, i.e., has CCE */
      max_num_ue--;
      if (max_num_ue < 0)
        return;

      /* Save shced_frame and sched_slot before overwrite by previous PUSCH filed */
      NR_UE_ul_harq_t *cur_harq = &sched_ctrl->ul_harq_processes[sched_ctrl->sched_pusch.ul_harq_pid];
      cur_harq->sched_pusch.frame = sched_ctrl->sched_pusch.frame;
      cur_harq->sched_pusch.slot = sched_ctrl->sched_pusch.slot;
      /* Get previous PSUCH filed info */
      sched_ctrl->sched_pusch = cur_harq->sched_pusch;
      NR_sched_pusch_t *sched_pusch = &sched_ctrl->sched_pusch;
      LOG_D(MAC, "%4d.%2d Allocate UL retransmission UE %d/RNTI %04x sched %4d.%2d (%d RBs)\n",
            frame, slot, UE_id, UE_info->rnti[UE_id],
            sched_pusch->frame, sched_pusch->slot,
            sched_pusch->rbSize);

      while (rbStart < bwpSize && !rballoc_mask[rbStart]) rbStart++;
      if (rbStart + sched_pusch->rbSize >= bwpSize) {
        LOG_W(MAC, "cannot allocate UL data for UE %d/RNTI %04x: no resources\n",
              UE_id, UE_info->rnti[UE_id]);
        return;
      }
      sched_pusch->rbStart = rbStart;
      /* no need to recompute the TBS, it will be the same */

      /* Mark the corresponding RBs as used */
      n_rb_sched -= sched_pusch->rbSize;
      for (int rb = 0; rb < sched_ctrl->sched_pusch.rbSize; rb++)
        rballoc_mask[rb + sched_ctrl->sched_pusch.rbStart] = 0;

      continue;
    }

    /* Calculate TBS from MCS */
    NR_sched_pusch_t *sched_pusch = &sched_ctrl->sched_pusch;
    const int mcs = 9;
    sched_pusch->mcs = mcs;
    sched_pusch->R = nr_get_code_rate_ul(mcs, ps->mcs_table);
    sched_pusch->Qm = nr_get_Qm_ul(mcs, ps->mcs_table);
    if (ps->pusch_Config->tp_pi2BPSK
        && ((ps->mcs_table == 3 && mcs < 2) || (ps->mcs_table == 4 && mcs < 6))) {
      sched_pusch->R >>= 1;
      sched_pusch->Qm <<= 1;
    }

    /* Check BSR and schedule UE if it is zero to avoid starvation, since we do
     * not have SR (yet) */
    if (sched_ctrl->estimated_ul_buffer - sched_ctrl->sched_ul_bytes <= 0) {
      if (UE_id != bsr0ue)
        continue;
      /* if no data, pre-allocate 5RB */
      bool freeCCE = find_free_CCE(module_id, slot, UE_id);
      if (!freeCCE) {
        LOG_D(MAC, "%4d.%2d no free CCE for UL DCI UE %04x (BSR 0)\n", frame, slot, UE_info->rnti[UE_id]);
        continue;
      }
      /* reduce max_num_ue once we are sure UE can be allocated, i.e., has CCE */
      max_num_ue--;
      if (max_num_ue < 0)
        return;

      while (rbStart < bwpSize && !rballoc_mask[rbStart]) rbStart++;
      if (rbStart + min_rb >= bwpSize) {
        LOG_W(MAC, "cannot allocate UL data for UE %d/RNTI %04x: no resources\n",
              UE_id, UE_info->rnti[UE_id]);
        return;
      }
      sched_pusch->rbStart = rbStart;
      sched_pusch->rbSize = min_rb;
      sched_pusch->tb_size = nr_compute_tbs(sched_pusch->Qm,
                                            sched_pusch->R,
                                            sched_pusch->rbSize,
                                            ps->nrOfSymbols,
                                            ps->N_PRB_DMRS * ps->num_dmrs_symb,
                                            0, // nb_rb_oh
                                            0,
                                            1 /* NrOfLayers */)
                             >> 3;

      /* Mark the corresponding RBs as used */
      n_rb_sched -= sched_pusch->rbSize;
      for (int rb = 0; rb < sched_ctrl->sched_pusch.rbSize; rb++)
        rballoc_mask[rb + sched_ctrl->sched_pusch.rbStart] = 0;

      continue;
    }

    /* Create UE_sched for UEs eligibale for new data transmission*/
    add_tail_nr_list(&UE_sched, UE_id);

    /* Calculate coefficient*/
    const uint32_t tbs = nr_compute_tbs(sched_pusch->Qm,
                                        sched_pusch->R,
                                        1, // rbSize
                                        ps->nrOfSymbols,
                                        ps->N_PRB_DMRS * ps->num_dmrs_symb,
                                        0, // nb_rb_oh
                                        0,
                                        1 /* NrOfLayers */)
                          >> 3;
    coeff_ue[UE_id] = (float) tbs / ul_thr_ue[UE_id];
    LOG_D(MAC,"b %d, ul_thr_ue[%d] %f, tbs %d, coeff_ue[%d] %f\n",
          b, UE_id, ul_thr_ue[UE_id], tbs, UE_id, coeff_ue[UE_id]);
  }


  /* Loop UE_sched to find max coeff and allocate transmission */
  while (UE_sched.head >= 0 && max_num_ue> 0 && n_rb_sched > 0) {
    /* Find max coeff */
    int *max = &UE_sched.head; /* Find max coeff: assume head is max */
    int *p = &UE_sched.next[*max];
    while (*p >= 0) {
      /* Find max coeff: if the current one has larger coeff, save for later */
      if (coeff_ue[*p] > coeff_ue[*max])
        max = p;
      p = &UE_sched.next[*p];
    }
    /* Find max coeff: remove the max one: do not use remove_nr_list() since it
     * goes through the whole list every time. Note that UE_sched.tail might
     * not be set correctly anymore */
    const int UE_id = *max;
    p = &UE_sched.next[*max];
    *max = UE_sched.next[*max];
    *p = -1;

    bool freeCCE = find_free_CCE(module_id, slot, UE_id);
    if (!freeCCE) {
      LOG_D(MAC, "%4d.%2d no free CCE for UL DCI UE %04x\n", frame, slot, UE_info->rnti[UE_id]);
      continue;
    }

    /* reduce max_num_ue once we are sure UE can be allocated, i.e., has CCE */
    max_num_ue--;
    if (max_num_ue < 0)
      return;

    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    int rbStart = NRRIV2PRBOFFSET(sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    const uint16_t bwpSize = NRRIV2BW(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    NR_sched_pusch_t *sched_pusch = &sched_ctrl->sched_pusch;


    while (rbStart < bwpSize && !rballoc_mask[rbStart]) rbStart++;
    sched_pusch->rbStart = rbStart;
    if (rbStart + min_rb >= bwpSize) {
      LOG_W(MAC, "cannot allocate UL data for UE %d/RNTI %04x: no resources\n",
            UE_id, UE_info->rnti[UE_id]);
      return;
    }

    /* Calculate the current scheduling bytes */
    const int B = cmax(sched_ctrl->estimated_ul_buffer - sched_ctrl->sched_ul_bytes, 0);
    uint16_t rbSize = min_rb - 1;
    do {
      rbSize++;
      sched_pusch->rbSize = rbSize;
      sched_pusch->tb_size = nr_compute_tbs(sched_pusch->Qm,
                                            sched_pusch->R,
                                            sched_pusch->rbSize,
                                            sched_ctrl->pusch_save.nrOfSymbols,
                                            sched_ctrl->pusch_save.N_PRB_DMRS * sched_ctrl->pusch_save.num_dmrs_symb,
                                            0, // nb_rb_oh
                                            0,
                                            1 /* NrOfLayers */)
                             >> 3;
    } while (rbStart + rbSize < bwpSize && rballoc_mask[rbStart+rbSize] &&
             sched_pusch->tb_size < B);
    LOG_D(MAC,"rbSize %d, TBS %d, est buf %d, sched_ul %d, B %d\n",
          rbSize, sched_pusch->tb_size, sched_ctrl->estimated_ul_buffer, sched_ctrl->sched_ul_bytes, B);

    /* Mark the corresponding RBs as used */
    n_rb_sched -= sched_pusch->rbSize;
    for (int rb = 0; rb < sched_ctrl->sched_pusch.rbSize; rb++)
      rballoc_mask[rb + sched_ctrl->sched_pusch.rbStart] = 0;
  }
}

bool nr_simple_ulsch_preprocessor(module_id_t module_id,
                                  frame_t frame,
                                  sub_frame_t slot,
                                  int num_slots_per_tdd,
                                  uint64_t ulsch_in_slot_bitmap) {
  gNB_MAC_INST *nr_mac = RC.nrmac[module_id];
  NR_COMMON_channels_t *cc = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  const int mu = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  NR_UE_info_t *UE_info = &nr_mac->UE_info;

  if (UE_info->num_UEs == 0)
    return false;

  const int CC_id = 0;

  /* NOT support different K2 in here, Get the K2 for first UE */
  int UE_id = UE_info->list.head;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  const int tda = 1;
  const struct NR_PUSCH_TimeDomainResourceAllocationList *tdaList =
    sched_ctrl->active_ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  AssertFatal(tda < tdaList->list.count,
              "time domain assignment %d >= %d\n",
              tda,
              tdaList->list.count);

  int K2 = get_K2(sched_ctrl->active_ubwp, tda, mu);
  const int sched_frame = frame + (slot + K2 >= nr_slots_per_frame[mu]);
  const int sched_slot = (slot + K2) % nr_slots_per_frame[mu];
  if (!is_xlsch_in_slot(ulsch_in_slot_bitmap, sched_slot))
    return false;

  sched_ctrl->sched_pusch.slot = sched_slot;
  sched_ctrl->sched_pusch.frame = sched_frame;

  /* Confirm all the UE have same K2 as the first UE */
  for (UE_id = UE_info->list.next[UE_id]; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    AssertFatal(K2 == get_K2(sched_ctrl->active_ubwp, tda, mu),
                "Different K2, %d(UE%d) != %ld(UE%d)\n", K2, 0, get_K2(sched_ctrl->active_ubwp, tda, mu), UE_id);
    sched_ctrl->sched_pusch.slot = sched_slot;
    sched_ctrl->sched_pusch.frame = sched_frame;
  }

  /* Change vrb_map_UL to rballoc_mask */
  uint16_t *vrb_map_UL =
      &RC.nrmac[module_id]->common_channels[CC_id].vrb_map_UL[sched_slot * MAX_BWP_SIZE];
  const uint16_t bwpSize = NRRIV2BW(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
  int startSymbolIndex, nrOfSymbols;
  SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);
  const uint16_t symb = ((1 << nrOfSymbols) - 1) << startSymbolIndex;
  int st = 0, e = 0, len = 0;
  for (int i = 0; i < bwpSize; i++) {
    while ((vrb_map_UL[i] & symb) != 0 && i < bwpSize)
      i++;
    st = i;
    while ((vrb_map_UL[i] & symb) == 0 && i < bwpSize)
      i++;
    if (i - st > len) {
      len = i - st;
      e = i - 1;
    }
  }
  st = e - len + 1;


  uint8_t rballoc_mask[bwpSize];

  /* Calculate mask: if any RB in vrb_map_UL is blocked (1), the current RB will be 0 */
  for (int i = 0; i < bwpSize; i++)
    rballoc_mask[i] = i >= st && i <= e;

  /* proportional fair scheduling algorithm */
  pf_ul(module_id,
        frame,
        slot,
        num_slots_per_tdd,
        &UE_info->list,
        len,
        rballoc_mask,
        2);
  return true;
}

void nr_schedule_ulsch(module_id_t module_id,
                       frame_t frame,
                       sub_frame_t slot,
                       int num_slots_per_tdd,
                       int ul_slots,
                       uint64_t ulsch_in_slot_bitmap) {
  /* Uplink data ONLY can be scheduled when the current slot is downlink slot,
   * because we have to schedule the DCI0 first before schedule uplink data */
  if (is_xlsch_in_slot(ulsch_in_slot_bitmap, slot)) {
    LOG_D(MAC, "Current slot %d is NOT DL slot, cannot schedule DCI0 for UL data\n", slot);
    return;
  }
  bool do_sched = RC.nrmac[module_id]->pre_processor_ul(
      module_id, frame, slot, num_slots_per_tdd, ulsch_in_slot_bitmap);
  if (!do_sched)
    return;

  const int CC_id = 0;
  nfapi_nr_ul_dci_request_t *ul_dci_req = &RC.nrmac[module_id]->UL_dci_req[CC_id];
  ul_dci_req->SFN = frame;
  ul_dci_req->Slot = slot;
  /* a PDCCH PDU groups DCIs per BWP and CORESET. Save a pointer to each
   * allocated PDCCH so we can easily allocate UE's DCIs independent of any
   * CORESET order */
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_bwp_coreset[MAX_NUM_BWP][MAX_NUM_CORESET] = {{0}};

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  const NR_list_t *UE_list = &UE_info->list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    UE_info->mac_stats[UE_id].ulsch_current_bytes = 0;
    /* dynamic PUSCH values (RB alloc, MCS, hence R, Qm, TBS) that change in
     * every TTI are pre-populated by the preprocessor and used below */
    NR_sched_pusch_t *sched_pusch = &sched_ctrl->sched_pusch;
    if (sched_pusch->rbSize <= 0)
      continue;

    uint16_t rnti = UE_info->rnti[UE_id];

    int8_t harq_id = sched_pusch->ul_harq_pid;
    if (NFAPI_MODE == NFAPI_MODE_VNF)
      harq_id = 1;
    else
    {
      if (harq_id < 0) {
        /* PP has not selected a specific HARQ Process, get a new one */
        harq_id = sched_ctrl->available_ul_harq.head;
        // if(NFAPI_MODE == NFAPI_MODE_VNF)
        //   harq_id = 1;
        AssertFatal(harq_id >= 0,
                    "no free HARQ process available for UE %d\n",
                    UE_id);
        remove_front_nr_list(&sched_ctrl->available_ul_harq);
        sched_pusch->ul_harq_pid = harq_id;
      } else {
        /* PP selected a specific HARQ process. Check whether it will be a new
        * transmission or a retransmission, and remove from the corresponding
        * list */
        if (sched_ctrl->ul_harq_processes[harq_id].round == 0)
          remove_nr_list(&sched_ctrl->available_ul_harq, harq_id);
        else
          remove_nr_list(&sched_ctrl->retrans_ul_harq, harq_id);
      }
    }
    NR_UE_ul_harq_t *cur_harq = &sched_ctrl->ul_harq_processes[harq_id];
    DevAssert(!cur_harq->is_waiting);
    add_tail_nr_list(&sched_ctrl->feedback_ul_harq, harq_id);
    cur_harq->feedback_slot = sched_pusch->slot;
    cur_harq->is_waiting = true;
    if (NFAPI_MODE == NFAPI_MODE_VNF)
      cur_harq->is_waiting = 0;
    

    int rnti_types[2] = { NR_RNTI_C, 0 };

    /* pre-computed PUSCH values that only change if time domain allocation,
     * DCI format, or DMRS parameters change. Updated in the preprocessor
     * through nr_save_pusch_fields() */
    NR_sched_pusch_save_t *ps = &sched_ctrl->pusch_save;

    /* Statistics */
    AssertFatal(cur_harq->round < 8, "Indexing ulsch_rounds[%d] is out of bounds\n", cur_harq->round);
    UE_info->mac_stats[UE_id].ulsch_rounds[cur_harq->round]++;
    if (cur_harq->round == 0) {
      UE_info->mac_stats[UE_id].ulsch_total_bytes_scheduled += sched_pusch->tb_size;
      /* Save information on MCS, TBS etc for the current initial transmission
       * so we have access to it when retransmitting */
      cur_harq->sched_pusch = *sched_pusch;
      sched_ctrl->sched_ul_bytes += sched_pusch->tb_size;
    } else {
      LOG_D(MAC,
            "%d.%2d UL retransmission RNTI %04x sched %d.%2d HARQ PID %d round %d NDI %d\n",
            frame,
            slot,
            rnti,
            sched_pusch->frame,
            sched_pusch->slot,
            harq_id,
            cur_harq->round,
            cur_harq->ndi);
    }
    UE_info->mac_stats[UE_id].ulsch_current_bytes = sched_pusch->tb_size;

    LOG_D(MAC,
          "%4d.%2d RNTI %04x UL sched %4d.%2d start %d RBS %d MCS %d TBS %d HARQ PID %d round %d NDI %d\n",
          frame,
          slot,
          rnti,
          sched_pusch->frame,
          sched_pusch->slot,
          sched_pusch->rbStart,
          sched_pusch->rbSize,
          sched_pusch->mcs,
          sched_pusch->tb_size,
          harq_id,
          cur_harq->round,
          cur_harq->ndi);

    /* PUSCH in a later slot, but corresponding DCI now! */
    nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_id]->UL_tti_req_ahead[0][sched_pusch->slot];
    AssertFatal(future_ul_tti_req->SFN == sched_pusch->frame
                && future_ul_tti_req->Slot == sched_pusch->slot,
                "%d.%d future UL_tti_req's frame.slot %d.%d does not match PUSCH %d.%d\n",
                frame, slot,
                future_ul_tti_req->SFN,
                future_ul_tti_req->Slot,
                sched_pusch->frame,
                sched_pusch->slot);
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
    nfapi_nr_pusch_pdu_t *pusch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pusch_pdu;
    memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));
    future_ul_tti_req->n_pdus += 1;

    LOG_D(MAC, "%4d.%2d Scheduling UE specific PUSCH\n", frame, slot);

    pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
    pusch_pdu->rnti = rnti;
    pusch_pdu->handle = 0; //not yet used

    /* FAPI: BWP */
    pusch_pdu->bwp_size  = NRRIV2BW(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    pusch_pdu->bwp_start = NRRIV2PRBOFFSET(sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    pusch_pdu->subcarrier_spacing = sched_ctrl->active_ubwp->bwp_Common->genericParameters.subcarrierSpacing;
    pusch_pdu->cyclic_prefix = 0;

    /* FAPI: PUSCH information always included */
    pusch_pdu->target_code_rate = sched_pusch->R;
    pusch_pdu->qam_mod_order = sched_pusch->Qm;
    pusch_pdu->mcs_index = sched_pusch->mcs;
    pusch_pdu->mcs_table = ps->mcs_table;
    pusch_pdu->transform_precoding = ps->transform_precoding;
    if (ps->pusch_Config->dataScramblingIdentityPUSCH)
      pusch_pdu->data_scrambling_id = *ps->pusch_Config->dataScramblingIdentityPUSCH;
    else
      pusch_pdu->data_scrambling_id = *scc->physCellId;
    pusch_pdu->nrOfLayers = 1;

    /* FAPI: DMRS */
    pusch_pdu->ul_dmrs_symb_pos = ps->ul_dmrs_symb_pos;
    pusch_pdu->dmrs_config_type = ps->dmrs_config_type;
    if (pusch_pdu->transform_precoding) { // transform precoding disabled
      long *scramblingid;
      if (pusch_pdu->scid == 0)
        scramblingid = ps->NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID0;
      else
        scramblingid = ps->NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID1;
      if (scramblingid == NULL)
        pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
      else
        pusch_pdu->ul_dmrs_scrambling_id = *scramblingid;
    }
    else {
      pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
      if (ps->NR_DMRS_UplinkConfig->transformPrecodingEnabled->nPUSCH_Identity != NULL)
        pusch_pdu->pusch_identity = *ps->NR_DMRS_UplinkConfig->transformPrecodingEnabled->nPUSCH_Identity;
      else
        pusch_pdu->pusch_identity = *scc->physCellId;
    }
    pusch_pdu->scid = 0;      // DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]
    pusch_pdu->num_dmrs_cdm_grps_no_data = ps->num_dmrs_cdm_grps_no_data;
    pusch_pdu->dmrs_ports = 1;

    /* FAPI: Pusch Allocation in frequency domain */
    AssertFatal(ps->pusch_Config->resourceAllocation == NR_PUSCH_Config__resourceAllocation_resourceAllocationType1,
                "Only frequency resource allocation type 1 is currently supported\n");
    pusch_pdu->resource_alloc = 1; //type 1
    pusch_pdu->rb_start = sched_pusch->rbStart;
    pusch_pdu->rb_size = sched_pusch->rbSize;
    pusch_pdu->vrb_to_prb_mapping = 0;
    if (ps->pusch_Config->frequencyHopping==NULL)
      pusch_pdu->frequency_hopping = 0;
    else
      pusch_pdu->frequency_hopping = 1;

    /* FAPI: Resource Allocation in time domain */
    pusch_pdu->start_symbol_index = ps->startSymbolIndex;
    pusch_pdu->nr_of_symbols = ps->nrOfSymbols;

    /* PUSCH PDU */
    AssertFatal(cur_harq->round < 4, "Indexing nr_rv_round_map[%d] is out of bounds\n", cur_harq->round);
    pusch_pdu->pusch_data.rv_index = nr_rv_round_map[cur_harq->round];
    pusch_pdu->pusch_data.harq_process_id = harq_id;
    pusch_pdu->pusch_data.new_data_indicator = cur_harq->ndi;
    pusch_pdu->pusch_data.tb_size = sched_pusch->tb_size;
    pusch_pdu->pusch_data.num_cb = 0; //CBG not supported

    /* TRANSFORM PRECODING --------------------------------------------------------*/

    if (pusch_pdu->transform_precoding == NR_PUSCH_Config__transformPrecoder_enabled){

      // U as specified in section 6.4.1.1.1.2 in 38.211, if sequence hopping and group hopping are disabled
      pusch_pdu->dfts_ofdm.low_papr_group_number = pusch_pdu->pusch_identity % 30;

      // V as specified in section 6.4.1.1.1.2 in 38.211 V = 0 if sequence hopping and group hopping are disabled
      if ((ps->NR_DMRS_UplinkConfig->transformPrecodingEnabled->sequenceGroupHopping == NULL) &&
            (ps->NR_DMRS_UplinkConfig->transformPrecodingEnabled->sequenceHopping == NULL))
        pusch_pdu->dfts_ofdm.low_papr_sequence_number = 0;
      else
        AssertFatal(1==0,"SequenceGroupHopping or sequenceHopping are NOT Supported\n");

      LOG_D(NR_MAC,"TRANSFORM PRECODING IS ENABLED. CDM groups: %d, U: %d MCS table: %d\n", pusch_pdu->num_dmrs_cdm_grps_no_data, pusch_pdu->dfts_ofdm.low_papr_group_number, ps->mcs_table);
    }

    /*-----------------------------------------------------------------------------*/

    /* PUSCH PTRS */
    if (ps->NR_DMRS_UplinkConfig->phaseTrackingRS != NULL) {
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

    /* look up the PDCCH PDU for this BWP and CORESET. If it does not exist,
     * create it */
    const int bwpid = sched_ctrl->active_bwp->bwp_Id;
    const int coresetid = sched_ctrl->coreset->controlResourceSetId;
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu = pdcch_pdu_bwp_coreset[bwpid][coresetid];
    if (!pdcch_pdu) {
      nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu = &ul_dci_req->ul_dci_pdu_list[ul_dci_req->numPdus];
      memset(ul_dci_request_pdu, 0, sizeof(nfapi_nr_ul_dci_request_pdus_t));
      ul_dci_request_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      ul_dci_request_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      pdcch_pdu = &ul_dci_request_pdu->pdcch_pdu.pdcch_pdu_rel15;
      ul_dci_req->numPdus += 1;
      nr_configure_pdcch(pdcch_pdu, sched_ctrl->search_space, sched_ctrl->coreset, scc, sched_ctrl->active_bwp);
      pdcch_pdu_bwp_coreset[bwpid][coresetid] = pdcch_pdu;
    }

    LOG_D(MAC,"Configuring ULDCI/PDCCH in %d.%d\n", frame,slot);

    /* Fill PDCCH DL DCI PDU */
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu->dci_pdu[pdcch_pdu->numDlDci];
    pdcch_pdu->numDlDci++;
    dci_pdu->RNTI = rnti;
    if (sched_ctrl->coreset->pdcch_DMRS_ScramblingID &&
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

    dci_pdu_rel15_t uldci_payload;
    memset(&uldci_payload, 0, sizeof(uldci_payload));
    NR_CellGroupConfig_t *secondaryCellGroup = UE_info->secondaryCellGroup[UE_id];
    const int n_ubwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count;
    config_uldci(sched_ctrl->active_ubwp,
                 pusch_pdu,
                 &uldci_payload,
                 ps->dci_format,
                 ps->time_domain_allocation,
                 UE_info->UE_sched_ctrl[UE_id].tpc0,
                 n_ubwp,
                 sched_ctrl->active_bwp->bwp_Id);
    fill_dci_pdu_rel15(scc,
                       secondaryCellGroup,
                       dci_pdu,
                       &uldci_payload,
                       ps->dci_format,
                       rnti_types[0],
                       pusch_pdu->bwp_size,
                       sched_ctrl->active_bwp->bwp_Id);

    memset(sched_pusch, 0, sizeof(*sched_pusch));
  }
}
