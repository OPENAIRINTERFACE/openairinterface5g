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

#include "NR_MAC_gNB/mac_proto.h"

/*
* When data are received on PHY and transmitted to MAC
*/
void nr_rx_sdu(const module_id_t gnb_mod_idP,
               const int CC_idP,
               const frame_t frameP,
               const sub_frame_t subframeP,
               const rnti_t rntiP,
               uint8_t *sduP,
               const uint16_t sdu_lenP,
               const uint16_t timing_advance,
               const uint8_t ul_cqi){
  int current_rnti = 0, UE_id = -1, harq_pid = 0;
  gNB_MAC_INST *gNB_mac = NULL;
  NR_UE_list_t *UE_list = NULL;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;

  current_rnti = rntiP;
  UE_id = find_nr_UE_id(gnb_mod_idP, current_rnti);
  gNB_mac = RC.nrmac[gnb_mod_idP];
  UE_list = &gNB_mac->UE_list;

  if (UE_id != -1) {
    UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);

    LOG_D(MAC, "[gNB %d][PUSCH %d] CC_id %d %d.%d Received ULSCH sdu round %d from PHY (rnti %x, UE_id %d) ul_cqi %d\n",
          gnb_mod_idP,
          harq_pid,
          CC_idP,
          frameP,
          subframeP,
          UE_scheduling_control->round_UL[CC_idP][harq_pid],
          current_rnti,
          UE_id,
          ul_cqi);

    if (sduP != NULL)
      UE_scheduling_control->ta_update = timing_advance;
  }
}

// WIP
// todo: complete
// TS 38.321 ch 6.1 Protocol Data Units - UL-SCH
void nr_process_mac_pdu(module_id_t module_idP,
                        uint8_t CC_id,
                        frame_t frameP,
                        uint8_t *pduP,
                        uint16_t mac_pdu_len,
                        uint8_t UE_id){

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
        mac_ce_len = 0x0000;
        mac_subheader_len = 0x0001; //  default to fixed-length subheader = 1-oct
        mac_sdu_len = 0x0000;
        rx_lcid = ((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID;

        switch(rx_lcid){
            //  MAC CEs
            case UL_SCH_LCID_RECOMMENDED_BITRATE_QUERY:
              // 38.321 Ch6.1.3.20
              mac_ce_len = 2;
              break;
            case UL_SCH_LCID_MULTI_ENTRY_PHR_4_OCT:
              // 38.321 Ch6.1.3.9
              // variable length
              // todo
              break;
            case UL_SCH_LCID_CONFIGURED_GRANT_CONFIRMATION:
                // 38.321 Ch6.1.3.7
                break;
            case UL_SCH_LCID_MULTI_ENTRY_PHR_1_OCT:
              // 38.321 Ch6.1.3.9
              // variable length
              // todo
              break;
            case UL_SCH_LCID_SINGLE_ENTRY_PHR:
              // 38.321 Ch6.1.3.8
              mac_ce_len = 2;
              break;
            case UL_SCH_LCID_C_RNTI:
              // 38.321 Ch6.1.3.2
              mac_ce_len = 2;
              break;
            case UL_SCH_LCID_S_TRUNCATED_BSR:
              // 38.321 Ch6.1.3.1
              // fixed length
              mac_ce_len = 1;
              break;
            case UL_SCH_LCID_L_TRUNCATED_BSR:
              // 38.321 Ch6.1.3.1
              // variable length
              // todo
              break;
            case UL_SCH_LCID_S_BSR:
              // 38.321 Ch6.1.3.1
              // fixed length
              mac_ce_len = 1;
              break;
            case UL_SCH_LCID_L_BSR:
              // 38.321 Ch6.1.3.1
              // variable length
              // todo
              break;
            case UL_SCH_LCID_PADDING:
              done = 1;
              // end of MAC PDU, can ignore the rest.
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
            case UL_SCH_LCID_CCCH:
              // todo
              mac_subheader_len = 2;
              break;
            case UL_SCH_LCID_DTCH:
              // todo
            default:
              if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                  //mac_sdu_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                  mac_subheader_len = 3;
                  mac_sdu_len = ((uint16_t)(((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L1 & 0x7f) << 8) | ((uint16_t)((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L2 & 0xff);

              } else {
                mac_sdu_len = (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
              }
              // todo
              break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );

        AssertFatal(pdu_len >= 0, "[MAC] nr_process_mac_pdu, residual mac pdu length < 0!\n");
    }
}