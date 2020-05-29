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

                LOG_D(MAC, "[UE %d] Frame %d : DLSCH -> DL-DTCH %d (gNB %d, %d bytes)\n", module_idP, frameP, rx_lcid, module_idP, mac_sdu_len);

                #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
                    LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);

                    for (i = 0; i < 32; i++)
                      LOG_T(MAC, "%x.", (pdu_ptr + mac_subheader_len)[i]);

                    LOG_T(MAC, "\n");
                #endif

                if (IS_SOFTMODEM_NOS1){
                  if (rx_lcid < NB_RB_MAX && rx_lcid >= UL_SCH_LCID_DTCH) {

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
                  } else {
                    LOG_E(MAC, "[UE %d] Frame %d : unknown LCID %d (gNB %d)\n", module_idP, frameP, rx_lcid, module_idP);
                  }
                }

            break;

        default:
        	return;
        	break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );

        if (pdu_len < 0) {
          LOG_E(MAC, "%s() residual mac pdu length < 0!\n", __func__);
          return;
        }
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
               const uint8_t ul_cqi){
  int current_rnti = 0, UE_id = -1, harq_pid = 0;
  gNB_MAC_INST *gNB_mac = NULL;
  NR_UE_list_t *UE_list = NULL;
  NR_UE_sched_ctrl_t *UE_scheduling_control = NULL;

  current_rnti = rntiP;
  UE_id = find_nr_UE_id(gnb_mod_idP, current_rnti);
  gNB_mac = RC.nrmac[gnb_mod_idP];
  UE_list = &gNB_mac->UE_list;

  if (UE_id != -1) {
    UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);

    LOG_D(MAC, "[gNB %d][PUSCH %d] CC_id %d %d.%d Received ULSCH sdu from PHY (rnti %x, UE_id %d) ul_cqi %d\n",
          gnb_mod_idP,
          harq_pid,
          CC_idP,
          frameP,
          slotP,
          current_rnti,
          UE_id,
          ul_cqi);

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
      //UE_scheduling_control->ta_update = timing_advance;
      LOG_D(MAC, "Received PDU at MAC gNB \n");
      nr_process_mac_pdu(gnb_mod_idP, CC_idP, frameP, sduP, sdu_lenP);
    }
  }
}
