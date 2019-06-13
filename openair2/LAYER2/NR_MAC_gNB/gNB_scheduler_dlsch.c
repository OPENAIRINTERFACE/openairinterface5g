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
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/MAC/mac.h"

/*NFAPI*/
#include "nfapi_nr_interface.h"
/*TAG*/
#include "NR_TAG-Id.h"


int nr_generate_dlsch_pdu(unsigned char *sdus_payload, 
	                        unsigned char *mac_pdu,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned short timing_advance_cmd,
                          NR_TAG_Id_t tag_id,
                          unsigned char *ue_cont_res_id,
                          unsigned short post_padding){

  NR_MAC_SUBHEADER_FIXED *mac_pdu_ptr = (NR_MAC_SUBHEADER_FIXED *) mac_pdu;
  unsigned char * dlsch_buffer_ptr = sdus_payload;
  uint8_t last_size = 0;
  int offset = 0, mac_ce_size, i;

  // MAC CEs 
  uint8_t mac_header_control_elements[16], *ce_ptr;
  ce_ptr = &mac_header_control_elements[0];

  // 1) Compute MAC CE and related subheaders 

  // DRX command subheader (MAC CE size 0)
  if (drx_cmd != 255) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_DRX;
    //last_size = 1;
    mac_pdu_ptr++;
  }

  // Timing Advance subheader
  if (timing_advance_cmd != 31) {
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = DL_SCH_LCID_TA_COMMAND;
    //last_size = 1;
    mac_pdu_ptr++;

    // TA MAC CE (1 octet) 
    AssertFatal(timing_advance_cmd < 64,"timing_advance_cmd %d > 63\n", timing_advance_cmd);
    ((NR_MAC_CE_TA *) ce_ptr)->TA_COMMAND = timing_advance_cmd;    //(timing_advance_cmd+31)&0x3f;
    ((NR_MAC_CE_TA *) ce_ptr)->TAGID = tag_id;

    LOG_D(MAC, "NR MAC CE timing advance command =%d (%d) TAG ID =%d\n", timing_advance_cmd, ((NR_MAC_CE_TA *) ce_ptr)->TA_COMMAND, tag_id);
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


  // 2) Generation of DLSCH MAC SDU subheaders
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
    dlsch_buffer_ptr+= sdu_lengths[i]; 
    mac_pdu_ptr += sdu_lengths[i];
  }

  // 4) Compute final offset for padding
  if (post_padding > 0) {    
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->LCID = DL_SCH_LCID_PADDING;
    mac_pdu_ptr++;

 	// compute final offset
    offset = ((unsigned char *) mac_pdu_ptr - mac_pdu);
    
  } else {            
    // no MAC subPDU with padding
  }

  //printf("Offset %d \n", ((unsigned char *) mac_pdu_ptr - mac_pdu));

  return offset;
}

/*TODO expand the scheduling functionalities
this function is just generating a dummy MAC PDU and is used to transmit the TA
The structure of the algorithm is taken from the schedule_ue_spec function in LTE 
and is preserved for future reference.*/
void
nr_schedule_ue_spec(module_id_t module_idP, frame_t frameP, sub_frame_t slotP){


  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  UE_list_t *UE_list = &gNB->UE_list;
  nfapi_nr_dl_config_request_body_t *dl_req; 

  // TODO size corretly the arrays
  // at present, preserved the array length from lte code
  // however not sure why NR_MAX_NB_RB was used here
  unsigned char sdu_lcids[NR_MAX_NB_RB] = {0};
  uint16_t sdu_lengths[NR_MAX_NB_RB] = {0};

  int padding = 0, post_padding = 0, ta_len = 0, header_length_total = 0, sdu_length_total = 0, num_sdus = 0;
  int CC_id, sub_pdu_id, lcid, offset, i, j=0, k=0;

  // hardcoded parameters
  // for DMRS configuration type 1
  // sdus should come from RLC 
  static unsigned char dlsch_buffer[MAX_NR_DLSCH_PAYLOAD_BYTES];
  uint8_t Qm = 2;
  uint16_t R = 697;
  uint16_t nb_rb = 50 ;
  uint32_t TBS = nr_compute_tbs(Qm, R, nb_rb, 12, 6, 0, 1)/8; // this is in bits TODO use nr_get_tbs
  int ta_update = 17;
  NR_TAG_Id_t tag_id = 0;
  int UE_id = 0; // UE_list->head is -1 !

  UE_sched_ctrl_t *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
  
  for (CC_id = 0; CC_id < RC.nb_nr_mac_CC[module_idP] + 1; CC_id++) {
    LOG_D(MAC, "doing nr_schedule_ue_spec for CC_id %d\n", CC_id);
    dl_req = &gNB->DL_req[CC_id].dl_config_request_body;

    //for (UE_id = UE_list->head; UE_id >= -1; UE_id = UE_list->next[UE_id]) {

      /* 
      //process retransmission
      if (round != 8) {
   
      } else {	// This is a potentially new SDU opportunity */

        if (gNB->tag->timeAlignmentTimer != NULL) {
          if (gNB->tag->timeAlignmentTimer == 0) {
            ta_update = ue_sched_ctl->ta_update;
            /* if we send TA then set timer to not send it for a while */
            if (ta_update != 31)
              ue_sched_ctl->ta_timer = 20;
            /* reset ta_update */
            ue_sched_ctl->ta_update = 31;
          } else {
            ta_update = 31;
          }
        }

        ta_len = (ta_update != 31) ? 2 : 0;

        // retrieve TAG ID
        if(gNB->tag->tag_Id != NULL ){
          tag_id = gNB->tag->tag_Id;
        }

        // fill dlsch_buffer with random data
        for (i = 0; i < MAX_NR_DLSCH_PAYLOAD_BYTES; i++){
          dlsch_buffer[i] = (unsigned char) rand();
        }

        /*
        //Sending SDUs with size 1
        //Initialize elements of sdu_lcids and sdu_lengths
        //TODO this will be eventually be removed 
        for (j = 0; j < NB_RB_MAX; j++){
          sdu_lcids[j] = 0x05; // DRB
        }
        
        for (k = 0; k < NB_RB_MAX; k++){
          sdu_lengths[k] = 1;
          header_length_total += 2;
          sdu_length_total += 1;
        }*/

        //Sending SDUs with size 1
        //Initialize elements of sdu_lcids and sdu_lengths
        //TODO this will be eventually be removed
        while (TBS - header_length_total - sdu_length_total - ta_len >= 3){
          if (k < NR_MAX_NB_RB && j < NR_MAX_NB_RB){
            sdu_lcids[j] = 0x05; // DRB
            sdu_lengths[k] = 1;
            header_length_total += 2;
            sdu_length_total += 1;
            num_sdus +=1;
            k++, j++;
          }
          else {
            break;
          }
        }

        /*
        // RLC data on DCCH
        if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
        }

        // RLC data on DCCH1
        if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
        }

        // looping over lcid
        for (lcid = NB_RB_MAX - 1; lcid >= DTCH; lcid--) {
        // here sdu_lcids[num_sdus] is populated
        }*/

        // there is at least one SDU or TA command
        // if (num_sdus > 0 ){
        if (ta_len + sdu_length_total + header_length_total > 0) {

          // Check if there is data from RLC or CE 
          if (TBS - header_length_total - sdu_length_total - ta_len >= 2) {
            // we have to consider padding
	          // padding param currently not in use
            padding = TBS - header_length_total - sdu_length_total - ta_len - 1;
            post_padding = 1;
          } else {
            post_padding = 0;
          }


          offset = nr_generate_dlsch_pdu((unsigned char *) dlsch_buffer,
          	                            (unsigned char *) UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0],
          	                            num_sdus,    //num_sdus
                                        sdu_lengths,
                                        sdu_lcids,
          	                            255,          // no drx
          	                            ta_update,    // timing advance
                                        tag_id,
          	                            NULL,         // contention res id
          	                            post_padding);
          // Padding: fill remainder of DLSCH with 0 
          if (post_padding > 0){
            for (int j = 0; j < (TBS - offset); j++)
              UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + j] = 0;
          }    

          // Printing bit by bit for debugging purpose 
          /*for (int k = 0; k < TBS; k++){
            printf("MAC PDU %u\n",((( UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][k/8]) & (1 << (k & 7))) >> (k & 7)));     
            if ((k+1)%8 == 0)
              printf("\n");
          }*/
          
	        }
          else {  // There is no data from RLC or MAC header, so don't schedule
          } 
	      //}
    //} // UE_id loop
  } // CC_id loop
}
