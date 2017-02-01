/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file eNB_scheduler_ulsch.c
 * \brief eNB procedures for the ULSCH transport channel
 * \author Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "T.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

// This table holds the allowable PRB sizes for ULSCH transmissions
uint8_t rb_table[33] = {1,2,3,4,5,6,8,9,10,12,15,16,18,20,24,25,27,30,32,36,40,45,48,50,54,60,72,75,80,81,90,96,100};

void rx_sdu(const module_id_t enb_mod_idP,
	    const int         CC_idP,
	    const frame_t     frameP,
	    const sub_frame_t subframeP,
	    const rnti_t      rntiP,
	    uint8_t          *sduP,
	    const uint16_t    sdu_lenP,
	    const int         harq_pidP,
	    uint8_t          *msg3_flagP)
{

  unsigned char  rx_ces[MAX_NUM_CE],num_ce,num_sdu,i,*payload_ptr;
  unsigned char  rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];
  int    UE_id = find_UE_id(enb_mod_idP,rntiP);
  int ii,j;
  eNB_MAC_INST *eNB = &eNB_mac_inst[enb_mod_idP];
  UE_list_t *UE_list= &eNB->UE_list;
  int crnti_rx=0;
  int old_buffer_info;

  start_meas(&eNB->rx_ulsch_sdu);

  if ((UE_id >  NUMBER_OF_UE_MAX) || (UE_id == -1)  )
    for(ii=0; ii<NB_RB_MAX; ii++) {
      rx_lengths[ii] = 0;
    }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_SDU,1);
  if (opt_enabled == 1) {
    trace_pdu(0, sduP,sdu_lenP, 0, 3, rntiP, frameP, subframeP, 0,0);
    LOG_D(OPT,"[eNB %d][ULSCH] Frame %d  rnti %x  with size %d\n",
    		  enb_mod_idP, frameP, rntiP, sdu_lenP);
  }

  LOG_D(MAC,"[eNB %d] CC_id %d Received ULSCH sdu from PHY (rnti %x, UE_id %d), parsing header\n",enb_mod_idP,CC_idP,rntiP,UE_id);

  if (sduP==NULL) { // we've got an error after N rounds
    UE_list->UE_sched_ctrl[UE_id].ul_scheduled       &= (~(1<<harq_pidP));
    return;
  }

  if (UE_id!=-1) {
    UE_list->UE_sched_ctrl[UE_id].ul_inactivity_timer=0;
    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer   =0;
    UE_list->UE_sched_ctrl[UE_id].ul_scheduled       &= (~(1<<harq_pidP));

    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync > 0) {
      UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync=0;
      mac_eNB_rrc_ul_in_sync(enb_mod_idP,CC_idP,frameP,subframeP,UE_RNTI(enb_mod_idP,UE_id));
    }
  }


  payload_ptr = parse_ulsch_header(sduP,&num_ce,&num_sdu,rx_ces,rx_lcids,rx_lengths,sdu_lenP);

  T(T_ENB_MAC_UE_UL_PDU, T_INT(enb_mod_idP), T_INT(CC_idP), T_INT(rntiP), T_INT(frameP), T_INT(subframeP),
    T_INT(harq_pidP), T_INT(sdu_lenP), T_INT(num_ce), T_INT(num_sdu));
  T(T_ENB_MAC_UE_UL_PDU_WITH_DATA, T_INT(enb_mod_idP), T_INT(CC_idP), T_INT(rntiP), T_INT(frameP), T_INT(subframeP),
    T_INT(harq_pidP), T_INT(sdu_lenP), T_INT(num_ce), T_INT(num_sdu), T_BUFFER(sduP, sdu_lenP));

  eNB->eNB_stats[CC_idP].ulsch_bytes_rx=sdu_lenP;
  eNB->eNB_stats[CC_idP].total_ulsch_bytes_rx+=sdu_lenP;
  eNB->eNB_stats[CC_idP].total_ulsch_pdus_rx+=1;
  // control element
  for (i=0; i<num_ce; i++) {

    T(T_ENB_MAC_UE_UL_CE, T_INT(enb_mod_idP), T_INT(CC_idP), T_INT(rntiP), T_INT(frameP), T_INT(subframeP),
      T_INT(rx_ces[i]));

    switch (rx_ces[i]) { // implement and process BSR + CRNTI +
    case POWER_HEADROOM:
      if (UE_id != -1) {
        UE_list->UE_template[CC_idP][UE_id].phr_info =  (payload_ptr[0] & 0x3f) - PHR_MAPPING_OFFSET;
        LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : Received PHR PH = %d (db)\n",
              enb_mod_idP, CC_idP, rx_ces[i], UE_list->UE_template[CC_idP][UE_id].phr_info);
        UE_list->UE_template[CC_idP][UE_id].phr_info_configured=1;
	UE_list->UE_sched_ctrl[UE_id].phr_received = 1;
      }
      payload_ptr+=sizeof(POWER_HEADROOM_CMD);
      break;

    case CRNTI:
      UE_id = find_UE_id(enb_mod_idP,(((uint16_t)payload_ptr[0])<<8) + payload_ptr[1]);
      LOG_I(MAC, "[eNB %d] Frame %d, Subframe %d CC_id %d MAC CE_LCID %d (ce %d/%d): CRNTI %x (UE_id %d) in Msg3\n",
	    frameP,subframeP,enb_mod_idP, CC_idP, rx_ces[i], i,num_ce,(((uint16_t)payload_ptr[0])<<8) + payload_ptr[1],UE_id);
      if (UE_id!=-1) {
	UE_list->UE_sched_ctrl[UE_id].ul_inactivity_timer=0;
	UE_list->UE_sched_ctrl[UE_id].ul_failure_timer=0;
	if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync > 0) {
	  UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync=0;
	  mac_eNB_rrc_ul_in_sync(enb_mod_idP,CC_idP,frameP,subframeP,(((uint16_t)payload_ptr[0])<<8) + payload_ptr[1]);
	}
      }
      crnti_rx=1;
      payload_ptr+=2;

      if (msg3_flagP != NULL) {
	*msg3_flagP = 0;
      }
      break;

    case TRUNCATED_BSR:
    case SHORT_BSR: {
      uint8_t lcgid;
      lcgid = (payload_ptr[0] >> 6);

      LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : Received short BSR LCGID = %u bsr = %d\n",
	    enb_mod_idP, CC_idP, rx_ces[i], lcgid, payload_ptr[0] & 0x3f);

      if (crnti_rx==1)
	LOG_I(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : Received short BSR LCGID = %u bsr = %d\n",
	      enb_mod_idP, CC_idP, rx_ces[i], lcgid, payload_ptr[0] & 0x3f);
      if (UE_id  != -1) {

        UE_list->UE_template[CC_idP][UE_id].bsr_info[lcgid] = (payload_ptr[0] & 0x3f);

	// update buffer info
	
	UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[lcgid]=BSR_TABLE[UE_list->UE_template[CC_idP][UE_id].bsr_info[lcgid]];

	UE_list->UE_template[CC_idP][UE_id].ul_total_buffer= UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[lcgid];

	PHY_vars_eNB_g[enb_mod_idP][CC_idP]->pusch_stats_bsr[UE_id][(frameP*10)+subframeP] = (payload_ptr[0] & 0x3f);
	if (UE_id == UE_list->head)
	  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR,PHY_vars_eNB_g[enb_mod_idP][CC_idP]->pusch_stats_bsr[UE_id][(frameP*10)+subframeP]);	
        if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[lcgid] == 0 ) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[lcgid]=frameP;
        }
	if (mac_eNB_get_rrc_status(enb_mod_idP,UE_RNTI(enb_mod_idP,UE_id)) < RRC_CONNECTED)
	  LOG_I(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : ul_total_buffer = %d (lcg increment %d)\n",
		enb_mod_idP, CC_idP, rx_ces[i], UE_list->UE_template[CC_idP][UE_id].ul_total_buffer,
		UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[lcgid]);	
      }
      else {

      }
      payload_ptr += 1;//sizeof(SHORT_BSR); // fixme
    }
    break;

    case LONG_BSR:
      if (UE_id  != -1) {
        UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID0] = ((payload_ptr[0] & 0xFC) >> 2);
        UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID1] =
          ((payload_ptr[0] & 0x03) << 4) | ((payload_ptr[1] & 0xF0) >> 4);
        UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID2] =
          ((payload_ptr[1] & 0x0F) << 2) | ((payload_ptr[2] & 0xC0) >> 6);
        UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID3] = (payload_ptr[2] & 0x3F);

	// update buffer info
	old_buffer_info = UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0];
	UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0]=BSR_TABLE[UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID0]];

	UE_list->UE_template[CC_idP][UE_id].ul_total_buffer+= UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0];
	if (UE_list->UE_template[CC_idP][UE_id].ul_total_buffer >= old_buffer_info)
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer -= old_buffer_info;
	else
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer = 0;

	old_buffer_info = UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1];
	UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1]=BSR_TABLE[UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID1]];
	UE_list->UE_template[CC_idP][UE_id].ul_total_buffer+= UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1];
	if (UE_list->UE_template[CC_idP][UE_id].ul_total_buffer >= old_buffer_info)
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer -= old_buffer_info;
	else
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer = 0;

	old_buffer_info = UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2];
	UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2]=BSR_TABLE[UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID2]];
	UE_list->UE_template[CC_idP][UE_id].ul_total_buffer+= UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2];
	if (UE_list->UE_template[CC_idP][UE_id].ul_total_buffer >= old_buffer_info)
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer -= old_buffer_info;
	else
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer = 0;

	old_buffer_info = UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3];
	UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3]=BSR_TABLE[UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID3]];
	UE_list->UE_template[CC_idP][UE_id].ul_total_buffer+= UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3];
	if (UE_list->UE_template[CC_idP][UE_id].ul_total_buffer >= old_buffer_info)
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer -= old_buffer_info;
	else
	  UE_list->UE_template[CC_idP][UE_id].ul_total_buffer = 0;

        LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d: Received long BSR LCGID0 = %u LCGID1 = "
              "%u LCGID2 = %u LCGID3 = %u\n",
              enb_mod_idP, CC_idP,
              rx_ces[i],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID0],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID1],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID2],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID3]);
      if (crnti_rx==1)
        LOG_I(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d: Received long BSR LCGID0 = %u LCGID1 = "
              "%u LCGID2 = %u LCGID3 = %u\n",
              enb_mod_idP, CC_idP,
              rx_ces[i],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID0],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID1],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID2],
              UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID3]);

        if (UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID0] == 0 ) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID0]=0;
        } else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID0] == 0) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID0]=frameP;
        }

        if (UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID1] == 0 ) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID1]=0;
        } else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID1] == 0) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID1]=frameP;
        }

        if (UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID2] == 0 ) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID2]=0;
        } else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID2] == 0) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID2]=frameP;
        }

        if (UE_list->UE_template[CC_idP][UE_id].bsr_info[LCGID3] == 0 ) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID3]= 0;
        } else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID3] == 0) {
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID3]=frameP;

        }
      }

      payload_ptr += 3;////sizeof(LONG_BSR);
      break;

    default:
      LOG_E(MAC, "[eNB %d] CC_id %d Received unknown MAC header (0x%02x)\n", enb_mod_idP, CC_idP, rx_ces[i]);
      break;
    }
  }

  for (i=0; i<num_sdu; i++) {
    LOG_D(MAC,"SDU Number %d MAC Subheader SDU_LCID %d, length %d\n",i,rx_lcids[i],rx_lengths[i]);

    T(T_ENB_MAC_UE_UL_SDU, T_INT(enb_mod_idP), T_INT(CC_idP), T_INT(rntiP), T_INT(frameP), T_INT(subframeP),
      T_INT(rx_lcids[i]), T_INT(rx_lengths[i]));
    T(T_ENB_MAC_UE_UL_SDU_WITH_DATA, T_INT(enb_mod_idP), T_INT(CC_idP), T_INT(rntiP), T_INT(frameP), T_INT(subframeP),
      T_INT(rx_lcids[i]), T_INT(rx_lengths[i]), T_BUFFER(payload_ptr, rx_lengths[i]));

    switch (rx_lcids[i]) {
    case CCCH :
      if (rx_lengths[i] > CCCH_PAYLOAD_SIZE_MAX) {
        LOG_E(MAC, "[eNB %d/%d] frame %d received CCCH of size %d (too big, maximum allowed is %d), dropping packet\n",
              enb_mod_idP, CC_idP, frameP, rx_lengths[i], CCCH_PAYLOAD_SIZE_MAX);
        break;
      }
      LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, Received CCCH:  %x.%x.%x.%x.%x.%x, Terminating RA procedure for UE rnti %x\n",
            enb_mod_idP,CC_idP,frameP,
            payload_ptr[0],payload_ptr[1],payload_ptr[2],payload_ptr[3],payload_ptr[4], payload_ptr[5], rntiP);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TERMINATE_RA_PROC,1);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TERMINATE_RA_PROC,0);
      for (ii=0; ii<NB_RA_PROC_MAX; ii++) {
        LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Checking proc %d : rnti (%x, %x), active %d\n",
              enb_mod_idP, CC_idP, ii,
              eNB->common_channels[CC_idP].RA_template[ii].rnti, rntiP,
              eNB->common_channels[CC_idP].RA_template[ii].RA_active);

        if ((eNB->common_channels[CC_idP].RA_template[ii].rnti==rntiP) &&
            (eNB->common_channels[CC_idP].RA_template[ii].RA_active==TRUE)) {

          //payload_ptr = parse_ulsch_header(msg3,&num_ce,&num_sdu,rx_ces,rx_lcids,rx_lengths,msg3_len);

          if (UE_id < 0) {
            memcpy(&eNB->common_channels[CC_idP].RA_template[ii].cont_res_id[0],payload_ptr,6);
            LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d CCCH: Received Msg3: length %d, offset %ld\n",
                  enb_mod_idP,CC_idP,frameP,rx_lengths[i],payload_ptr-sduP);

            if ((UE_id=add_new_ue(enb_mod_idP,CC_idP,eNB->common_channels[CC_idP].RA_template[ii].rnti,harq_pidP)) == -1 ) {
              mac_xface->macphy_exit("[MAC][eNB] Max user count reached\n");
	      // kill RA procedure
            } else
              LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Added user with rnti %x => UE %d\n",
                    enb_mod_idP,CC_idP,frameP,eNB->common_channels[CC_idP].RA_template[ii].rnti,UE_id);
          } else {
            LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d CCCH: Received Msg3 from already registered UE %d: length %d, offset %ld\n",
                  enb_mod_idP,CC_idP,frameP,UE_id,rx_lengths[i],payload_ptr-sduP);
	    // kill RA procedure
          }

          if (Is_rrc_registered == 1)
            mac_rrc_data_ind(
              enb_mod_idP,
              CC_idP,
              frameP,subframeP,
              rntiP,
              CCCH,
              (uint8_t*)payload_ptr,
              rx_lengths[i],
              ENB_FLAG_YES,
              enb_mod_idP,
              0);


          if (num_ce >0) {  // handle msg3 which is not RRCConnectionRequest
            //  process_ra_message(msg3,num_ce,rx_lcids,rx_ces);
          }

          eNB->common_channels[CC_idP].RA_template[ii].generate_Msg4 = 1;
          eNB->common_channels[CC_idP].RA_template[ii].wait_ack_Msg4 = 0;

        } // if process is active
      } // loop on RA processes
      
      break ;

    case DCCH :
    case DCCH1 :
      //      if(eNB_mac_inst[module_idP][CC_idP].Dcch_lchan[UE_id].Active==1){
      

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      LOG_T(MAC,"offset: %d\n",(unsigned char)((unsigned char*)payload_ptr-sduP));
      for (j=0; j<32; j++) {
        LOG_T(MAC,"%x ",payload_ptr[j]);
      }
      LOG_T(MAC,"\n");
#endif

      if (UE_id != -1) {
	// adjust buffer occupancy of the correponding logical channel group
	if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] >= rx_lengths[i])
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] -= rx_lengths[i];
	else
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] = 0;

          LOG_D(MAC,"[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DCCH, received %d bytes form UE %d on LCID %d \n",
                enb_mod_idP,CC_idP,frameP, rx_lengths[i], UE_id, rx_lcids[i]);

          mac_rlc_data_ind(
			   enb_mod_idP,
			   rntiP,
			   enb_mod_idP,
			   frameP,
			   ENB_FLAG_YES,
			   MBMS_FLAG_NO,
			   rx_lcids[i],
			   (char *)payload_ptr,
			   rx_lengths[i],
			   1,
			   NULL);//(unsigned int*)crc_status);
          UE_list->eNB_UE_stats[CC_idP][UE_id].num_pdu_rx[rx_lcids[i]]+=1;
          UE_list->eNB_UE_stats[CC_idP][UE_id].num_bytes_rx[rx_lcids[i]]+=rx_lengths[i];
      } /* UE_id != -1 */
 
      // } 
      break;

      // all the DRBS
    case DTCH:
    default :

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      LOG_T(MAC,"offset: %d\n",(unsigned char)((unsigned char*)payload_ptr-sduP));
      for (j=0; j<32; j++) {
        LOG_T(MAC,"%x ",payload_ptr[j]);
      }
      LOG_T(MAC,"\n");
#endif
      if (rx_lcids[i]  < NB_RB_MAX ) {
	LOG_D(MAC,"[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DTCH, received %d bytes from UE %d for lcid %d\n",
	      enb_mod_idP,CC_idP,frameP, rx_lengths[i], UE_id, rx_lcids[i]);
	
	if (UE_id != -1) {
	  // adjust buffer occupancy of the correponding logical channel group
	  LOG_D(MAC,"[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DTCH, received %d bytes from UE %d for lcid %d, removing from LCGID %ld, %d\n",
		enb_mod_idP,CC_idP,frameP, rx_lengths[i], UE_id,rx_lcids[i],
		UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]],
		UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]]);
	  
	  if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] >= rx_lengths[i])
	    UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] -= rx_lengths[i];
	  else
	    UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] = 0;
	  if ((rx_lengths[i] <SCH_PAYLOAD_SIZE_MAX) &&  (rx_lengths[i] > 0) ) {   // MAX SIZE OF transport block
	    mac_rlc_data_ind(
			     enb_mod_idP,
			     rntiP,
			     enb_mod_idP,
			     frameP,
			     ENB_FLAG_YES,
			     MBMS_FLAG_NO,
			     rx_lcids[i],
			     (char *)payload_ptr,
			     rx_lengths[i],
			     1,
			     NULL);//(unsigned int*)crc_status);
	    
	    UE_list->eNB_UE_stats[CC_idP][UE_id].num_pdu_rx[rx_lcids[i]]+=1;
	    UE_list->eNB_UE_stats[CC_idP][UE_id].num_bytes_rx[rx_lcids[i]]+=rx_lengths[i];
	  }
	  else { /* rx_length[i] */
	    UE_list->eNB_UE_stats[CC_idP][UE_id].num_errors_rx+=1;
	    LOG_E(MAC,"[eNB %d] CC_id %d Frame %d : Max size of transport block reached LCID %d from UE %d ",
		  enb_mod_idP, CC_idP, frameP, rx_lcids[i], UE_id);
	  }
	}    
	else {/*(UE_id != -1*/ 
	  LOG_E(MAC,"[eNB %d] CC_id %d Frame %d : received unsupported or unknown LCID %d from UE %d ",
		enb_mod_idP, CC_idP, frameP, rx_lcids[i], UE_id);
	}
      }

      break;
    }
  
    payload_ptr+=rx_lengths[i];
  }

  /* NN--> FK: we could either check the payload, or use a phy helper to detect a false msg3 */
  if ((num_sdu == 0) && (num_ce==0)) {
    if (UE_id != -1)
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_num_errors_rx+=1;

    if (msg3_flagP != NULL) {
      if( *msg3_flagP == 1 ) {
        LOG_N(MAC,"[eNB %d] CC_id %d frame %d : false msg3 detection: signal phy to canceling RA and remove the UE\n", enb_mod_idP, CC_idP, frameP);
        *msg3_flagP=0;
      }
    }
  } else {
    if (UE_id != -1) {
      UE_list->eNB_UE_stats[CC_idP][UE_id].pdu_bytes_rx=sdu_lenP;
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_pdu_bytes_rx+=sdu_lenP;
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_num_pdus_rx+=1;
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_SDU,0);
  stop_meas(&eNB->rx_ulsch_sdu);
}


uint32_t bytes_to_bsr_index(int32_t nbytes)
{

  uint32_t i=0;

  if (nbytes<0) {
    return(0);
  }

  while ((i<BSR_TABLE_SIZE)&&
         (BSR_TABLE[i]<=nbytes)) {
    i++;
  }

  return(i-1);
}

void add_ue_ulsch_info(module_id_t module_idP, int CC_id, int UE_id, sub_frame_t subframeP, UE_ULSCH_STATUS status)
{

  eNB_ulsch_info[module_idP][CC_id][UE_id].rnti             = UE_RNTI(module_idP,UE_id);
  eNB_ulsch_info[module_idP][CC_id][UE_id].subframe         = subframeP;
  eNB_ulsch_info[module_idP][CC_id][UE_id].status           = status;

  eNB_ulsch_info[module_idP][CC_id][UE_id].serving_num++;

}

unsigned char *parse_ulsch_header(unsigned char *mac_header,
                                  unsigned char *num_ce,
                                  unsigned char *num_sdu,
                                  unsigned char *rx_ces,
                                  unsigned char *rx_lcids,
                                  unsigned short *rx_lengths,
                                  unsigned short tb_length)
{

  unsigned char not_done=1,num_ces=0,num_sdus=0,lcid,num_sdu_cnt;
  unsigned char *mac_header_ptr = mac_header;
  unsigned short length, ce_len=0;

  while (not_done==1) {

    if (((SCH_SUBHEADER_FIXED*)mac_header_ptr)->E == 0) {
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID;

    if (lcid < EXTENDED_POWER_HEADROOM) {
      if (not_done==0) { // last MAC SDU, length is implicit
        mac_header_ptr++;
        length = tb_length-(mac_header_ptr-mac_header)-ce_len;

        for (num_sdu_cnt=0; num_sdu_cnt < num_sdus ; num_sdu_cnt++) {
          length -= rx_lengths[num_sdu_cnt];
        }
      } else {
        if (((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F == 0) {
          length = ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L;
          mac_header_ptr += 2;//sizeof(SCH_SUBHEADER_SHORT);
        } else { // F = 1
          length = ((((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_MSB & 0x7f ) << 8 ) | (((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_LSB & 0xff);
          mac_header_ptr += 3;//sizeof(SCH_SUBHEADER_LONG);
        }
      }

      LOG_D(MAC,"[eNB] sdu %d lcid %d tb_length %d length %d (offset now %ld)\n",
            num_sdus,lcid,tb_length, length,mac_header_ptr-mac_header);
      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else { // This is a control element subheader POWER_HEADROOM, BSR and CRNTI
      if (lcid == SHORT_PADDING) {
        mac_header_ptr++;
      } else {
        rx_ces[num_ces] = lcid;
        num_ces++;
        mac_header_ptr++;

        if (lcid==LONG_BSR) {
          ce_len+=3;
        } else if (lcid==CRNTI) {
          ce_len+=2;
        } else if ((lcid==POWER_HEADROOM) || (lcid==TRUNCATED_BSR)|| (lcid== SHORT_BSR)) {
          ce_len++;
        } else {
          LOG_E(MAC,"unknown CE %d \n", lcid);
          mac_xface->macphy_exit("unknown CE");
        }
      }
    }
  }

  *num_ce = num_ces;
  *num_sdu = num_sdus;

  return(mac_header_ptr);
}

/* This function is called by PHY layer when it schedules some
 * uplink for a random access message 3.
 * The MAC scheduler has to skip the RBs used by this message 3
 * (done below in schedule_ulsch).
 */
void set_msg3_subframe(module_id_t Mod_id,
                       int CC_id,
                       int frame,
                       int subframe,
                       int rnti,
                       int Msg3_frame,
                       int Msg3_subframe)
{
  eNB_MAC_INST *eNB=&eNB_mac_inst[Mod_id];
  int i;
  for (i=0; i<NB_RA_PROC_MAX; i++) {
    if (eNB->common_channels[CC_id].RA_template[i].RA_active == TRUE &&
        eNB->common_channels[CC_id].RA_template[i].rnti == rnti) {
      eNB->common_channels[CC_id].RA_template[i].Msg3_subframe = Msg3_subframe;
      break;
    }
  }
}

void schedule_ulsch(module_id_t module_idP, 
		    frame_t frameP,
		    unsigned char cooperation_flag,
		    sub_frame_t subframeP, 
		    unsigned char sched_subframe) {



  uint16_t first_rb[MAX_NUM_CCs],i;
  int CC_id;
  eNB_MAC_INST *eNB=&eNB_mac_inst[module_idP];

  start_meas(&eNB->schedule_ulsch);


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    //leave out first RB for PUCCH
    first_rb[CC_id] = 1;

    // UE data info;
    // check which UE has data to transmit
    // function to decide the scheduling
    // e.g. scheduling_rslt = Greedy(granted_UEs, nb_RB)

    // default function for default scheduling
    //

    // output of scheduling, the UE numbers in RBs, where it is in the code???
    // check if RA (Msg3) is active in this subframeP, if so skip the PRBs used for Msg3
    // Msg3 is using 1 PRB so we need to increase first_rb accordingly
    // not sure about the break (can there be more than 1 active RA procedure?)

    for (i=0; i<NB_RA_PROC_MAX; i++) {
      if ((eNB->common_channels[CC_id].RA_template[i].RA_active == TRUE) &&
          (eNB->common_channels[CC_id].RA_template[i].generate_rar == 0) &&
          (eNB->common_channels[CC_id].RA_template[i].generate_Msg4 == 0) &&
          (eNB->common_channels[CC_id].RA_template[i].wait_ack_Msg4 == 0) &&
          (eNB->common_channels[CC_id].RA_template[i].Msg3_subframe == sched_subframe)) {
        first_rb[CC_id]++;
        eNB->common_channels[CC_id].RA_template[i].Msg3_subframe = -1;
        break;
      }
    }

    /*
    if (mac_xface->is_prach_subframe(&(mac_xface->lte_frame_parms),frameP,subframeP)) {
      first_rb[CC_id] = (mac_xface->get_prach_prb_offset(&(mac_xface->lte_frame_parms),
    */

  }

  schedule_ulsch_rnti(module_idP, cooperation_flag, frameP, subframeP, sched_subframe,first_rb);

#ifdef CBA
  schedule_ulsch_cba_rnti(module_idP, cooperation_flag, frameP, subframeP, sched_subframe, first_rb);
#endif


  stop_meas(&eNB->schedule_ulsch);

}



void schedule_ulsch_rnti(module_id_t   module_idP,
                         unsigned char cooperation_flag,
                         frame_t       frameP,
                         sub_frame_t   subframeP,
                         unsigned char sched_subframe,
                         uint16_t     *first_rb)
{

  int                UE_id;
  uint8_t            aggregation    = 2;
  rnti_t             rnti           = -1;
  uint8_t            round          = 0;
  uint8_t            harq_pid       = 0;
  void              *ULSCH_dci      = NULL;
  LTE_eNB_UE_stats  *eNB_UE_stats   = NULL;
  DCI_PDU           *DCI_pdu;
  uint8_t                 status         = 0;
  uint8_t                 rb_table_index = -1;
  uint16_t                TBS = 0;
  //  int32_t                buffer_occupancy=0;
  uint32_t                cqi_req,cshift,ndi,mcs=0,rballoc,tpc;
  int32_t                 normalized_rx_power, target_rx_power=-90;
  static int32_t          tpc_accumulated=0;

  int n,CC_id = 0;
  eNB_MAC_INST      *eNB=&eNB_mac_inst[module_idP];
  UE_list_t         *UE_list=&eNB->UE_list;
  UE_TEMPLATE       *UE_template;
  UE_sched_ctrl     *UE_sched_ctrl;

  //  int                rvidx_tab[4] = {0,2,3,1};
  LTE_DL_FRAME_PARMS   *frame_parms;
  int drop_ue=0;

  //  LOG_I(MAC,"entering ulsch preprocesor\n");

  ulsch_scheduler_pre_processor(module_idP,
                                frameP,
                                subframeP,
                                first_rb);

  //  LOG_I(MAC,"exiting ulsch preprocesor\n");

  // loop over all active UEs
  for (UE_id=UE_list->head_ul; UE_id>=0; UE_id=UE_list->next_ul[UE_id]) {

    // don't schedule if Msg4 is not received yet
    if (UE_list->UE_template[UE_PCCID(module_idP,UE_id)][UE_id].configured==FALSE) {
      LOG_I(MAC,"[eNB %d] frame %d subfarme %d, UE %d: not configured, skipping UE scheduling \n", 
	    module_idP,frameP,subframeP,UE_id);
      continue;
    }

    rnti = UE_RNTI(module_idP,UE_id);

    if (rnti==NOT_A_RNTI) {
      LOG_W(MAC,"[eNB %d] frame %d subfarme %d, UE %d: no RNTI \n", module_idP,frameP,subframeP,UE_id);
      continue;
    }

    /* let's drop the UE if get_eNB_UE_stats returns NULL when calling it with any of the UE's active UL CCs */
    /* TODO: refine? */
    drop_ue = 0;
    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      if (mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti) == NULL) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: no PHY context\n", module_idP,frameP,subframeP,UE_id,rnti,CC_id);
        drop_ue = 1;
        break;
      }
    }
    if (drop_ue == 1) {
/* we can't come here, ulsch_scheduler_pre_processor won't put in the list a UE with no PHY context */
abort();
      /* TODO: this is a hack. Sometimes the UE has no PHY context but
       * is still present in the MAC with 'ul_failure_timer' = 0 and
       * 'ul_out_of_sync' = 0. It seems wrong and the UE stays there forever. Let's
       * start an UL out of sync procedure in this case.
       * The root cause of this problem has to be found and corrected.
       * In the meantime, this hack...
       */
      if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer == 0 &&
          UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 0) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: UE in weird state, let's put it 'out of sync'\n",
              module_idP,frameP,subframeP,UE_id,rnti,CC_id);
        // inform RRC of failure and clear timer
        mac_eNB_rrc_ul_failure(module_idP,CC_id,frameP,subframeP,rnti);
        UE_list->UE_sched_ctrl[UE_id].ul_failure_timer=0;
        UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync=1;
      }
      continue;
    }

    // loop over all active UL CC_ids for this UE
    for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      frame_parms = mac_xface->get_lte_frame_parms(module_idP,CC_id);
      eNB_UE_stats = mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti);

      aggregation=get_aggregation(get_bw_index(module_idP,CC_id), 
				  eNB_UE_stats->DL_cqi[0],
				  format0);
      
      if (CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,aggregation,rnti)) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: not enough nCCE\n", module_idP,frameP,subframeP,UE_id,rnti,CC_id);
        continue; // break;
      } else{
	LOG_D(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d mode %s: aggregation level %d\n", 
	      module_idP,frameP,subframeP,UE_id,rnti,CC_id, mode_string[eNB_UE_stats->mode], 1<<aggregation);
      }


      if (eNB_UE_stats->mode == PUSCH) { // ue has a ulsch channel

        DCI_pdu = &eNB->common_channels[CC_id].DCI_pdu;
        UE_template   = &UE_list->UE_template[CC_id][UE_id];
        UE_sched_ctrl = &UE_list->UE_sched_ctrl[UE_id];

        if (mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,openair_harq_UL) == -1 ) {
          LOG_W(MAC,"[eNB %d] Scheduler Frame %d, subframeP %d: candidate harq_pid from PHY for UE %d CC %d RNTI %x\n",
                module_idP,frameP,subframeP, UE_id, CC_id, rnti);
          continue;
        } else
          LOG_T(MAC,"[eNB %d] Frame %d, subframeP %d, UE %d CC %d : got harq pid %d  round %d (rnti %x,mode %s)\n",
                module_idP,frameP,subframeP,UE_id,CC_id, harq_pid, round,rnti,mode_string[eNB_UE_stats->mode]);

	PHY_vars_eNB_g[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP] = UE_template->ul_total_buffer;
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BO,PHY_vars_eNB_g[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP]);	
        if (((UE_is_to_be_scheduled(module_idP,CC_id,UE_id)>0)) || (round>0))// || ((frameP%10)==0))
          // if there is information on bsr of DCCH, DTCH or if there is UL_SR, or if there is a packet to retransmit, or we want to schedule a periodic feedback every 10 frames
        {
	  LOG_D(MAC,"[eNB %d][PUSCH] Frame %d subframe %d Scheduling UE %d/%x in round %d(SR %d,UL_inactivity timer %d,UL_failure timer %d)\n",
		module_idP,frameP,subframeP,UE_id,rnti,round,UE_template->ul_SR,
		UE_sched_ctrl->ul_inactivity_timer,
		UE_sched_ctrl->ul_failure_timer);
          // reset the scheduling request
          UE_template->ul_SR = 0;
          status = mac_eNB_get_rrc_status(module_idP,rnti);
	  if (status < RRC_CONNECTED)
	    cqi_req = 0;
	  else if (UE_sched_ctrl->cqi_req_timer>30) {
	    cqi_req = 1;
	    UE_sched_ctrl->cqi_req_timer=0;
	  }
	  else
	    cqi_req = 0;

          //power control
          //compute the expected ULSCH RX power (for the stats)

          // this is the normalized RX power and this should be constant (regardless of mcs
          normalized_rx_power = eNB_UE_stats->UL_rssi[0];
          target_rx_power = mac_xface->get_target_pusch_rx_power(module_idP,CC_id);

          // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_template->pusch_tpc_tx_frame*10+UE_template->pusch_tpc_tx_subframe;
          if (((framex10psubframe+10)<=(frameP*10+subframeP)) || //normal case
	      ((framex10psubframe>(frameP*10+subframeP)) && (((10240-framex10psubframe+frameP*10+subframeP)>=10)))) //frame wrap-around
	    {
	    UE_template->pusch_tpc_tx_frame=frameP;
	    UE_template->pusch_tpc_tx_subframe=subframeP;
            if (normalized_rx_power>(target_rx_power+1)) {
              tpc = 0; //-1
              tpc_accumulated--;
            } else if (normalized_rx_power<(target_rx_power-1)) {
              tpc = 2; //+1
              tpc_accumulated++;
            } else {
              tpc = 1; //0
            }
          } else {
            tpc = 1; //0
          }

	  if (tpc!=1) {
	    LOG_D(MAC,"[eNB %d] ULSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
		  module_idP,frameP,subframeP,harq_pid,tpc,
		  tpc_accumulated,normalized_rx_power,target_rx_power);
	  }

          // new transmission
          if (round==0) {

            ndi = 1-UE_template->oldNDI_UL[harq_pid];
            UE_template->oldNDI_UL[harq_pid]=ndi;
	    UE_list->eNB_UE_stats[CC_id][UE_id].normalized_rx_power=normalized_rx_power;
	    UE_list->eNB_UE_stats[CC_id][UE_id].target_rx_power=target_rx_power;
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1=UE_template->pre_assigned_mcs_ul;
            mcs = UE_template->pre_assigned_mcs_ul;//cmin (UE_template->pre_assigned_mcs_ul, openair_daq_vars.target_ue_ul_mcs); // adjust, based on user-defined MCS
            if (UE_template->pre_allocated_rb_table_index_ul >=0) {
              rb_table_index=UE_template->pre_allocated_rb_table_index_ul;
            } else {
	      mcs=10;//cmin (10, openair_daq_vars.target_ue_ul_mcs);
              rb_table_index=5; // for PHR
	    }

            UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2=mcs;
	    //            buffer_occupancy = UE_template->ul_total_buffer;

            while (((rb_table[rb_table_index]>(frame_parms->N_RB_UL-1-first_rb[CC_id])) ||
		    (rb_table[rb_table_index]>45)) &&
                   (rb_table_index>0)) {
              rb_table_index--;
            }

            TBS = mac_xface->get_TBS_UL(mcs,rb_table[rb_table_index]);
	    UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx+=rb_table[rb_table_index];
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS=TBS;
	    //            buffer_occupancy -= TBS;
            rballoc = mac_xface->computeRIV(frame_parms->N_RB_UL,
                                            first_rb[CC_id],
                                            rb_table[rb_table_index]);

            T(T_ENB_MAC_UE_UL_SCHEDULE, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP),
              T_INT(subframeP), T_INT(harq_pid), T_INT(mcs), T_INT(first_rb[CC_id]), T_INT(rb_table[rb_table_index]),
              T_INT(TBS), T_INT(ndi));

	    if (mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED)
	      LOG_I(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE %d (mcs %d, first rb %d, nb_rb %d, rb_table_index %d, TBS %d, harq_pid %d)\n",
		    module_idP,harq_pid,rnti,CC_id,frameP,subframeP,UE_id,mcs,
		    first_rb[CC_id],rb_table[rb_table_index],
		    rb_table_index,TBS,harq_pid);

	    // bad indices : 20 (40 PRB), 21 (45 PRB), 22 (48 PRB)
            // increment for next UE allocation
            first_rb[CC_id]+=rb_table[rb_table_index];
            //store for possible retransmission
            UE_template->nb_rb_ul[harq_pid] = rb_table[rb_table_index];
	    UE_sched_ctrl->ul_scheduled |= (1<<harq_pid);
	    if (UE_id == UE_list->head)
	      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SCHEDULED,UE_sched_ctrl->ul_scheduled);

	    // adjust total UL buffer status by TBS, wait for UL sdus to do final update
	    LOG_D(MAC,"[eNB %d] CC_id %d UE %d/%x : adjusting ul_total_buffer, old %d, TBS %d\n", module_idP,CC_id,UE_id,rnti,UE_template->ul_total_buffer,TBS);
	    if (UE_template->ul_total_buffer > TBS)
	      UE_template->ul_total_buffer -= TBS;
	    else
	      UE_template->ul_total_buffer = 0;
	    LOG_D(MAC,"ul_total_buffer, new %d\n", UE_template->ul_total_buffer);
	    // Cyclic shift for DM RS
	    cshift = 0;// values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
	    	    
	    if (frame_parms->frame_type == TDD) {
	      switch (frame_parms->N_RB_UL) {
	      case 6:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_1_5MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_1_5MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_1_5MHz_TDD_1_6_t,
				format0,
				0);
		break;
		
	      default:
	      case 25:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_5MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_5MHz_TDD_1_6_t,
				format0,
				0);
		break;
		
	      case 50:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_10MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_10MHz_TDD_1_6_t,
				format0,
				0);
		break;
		
	      case 100:
		ULSCH_dci = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->padding  = 0;
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_template->DAI_ul[sched_subframe];
		((DCI0_20MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_20MHz_TDD_1_6_t),
				aggregation,
				sizeof_DCI0_20MHz_TDD_1_6_t,
				format0,
				0);
		break;
	      }
	    } // TDD
	    else { //FDD
	      switch (frame_parms->N_RB_UL) {
	      case 25:
	      default:
		
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_5MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_5MHz_FDD_t),
				aggregation,
				sizeof_DCI0_5MHz_FDD_t,
				format0,
				0);
		break;
		
	      case 6:
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_1_5MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_1_5MHz_FDD_t),
				aggregation,
				sizeof_DCI0_1_5MHz_FDD_t,
				format0,
				0);
		break;
		
	      case 50:
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_10MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_10MHz_FDD_t),
				aggregation,
				sizeof_DCI0_10MHz_FDD_t,
				format0,
				0);
		break;
		
	      case 100:
		ULSCH_dci          = UE_template->ULSCH_DCI[harq_pid];
		
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->type     = 0;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->mcs      = mcs;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->ndi      = ndi;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->TPC      = tpc;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->padding  = 0;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->cshift   = cshift;
		((DCI0_20MHz_FDD_t *)ULSCH_dci)->cqi_req  = cqi_req;
		
		add_ue_spec_dci(DCI_pdu,
				ULSCH_dci,
				rnti,
				sizeof(DCI0_20MHz_FDD_t),
				aggregation,
				sizeof_DCI0_20MHz_FDD_t,
				format0,
				0);
		break;
		
	      }
	    }


	    add_ue_ulsch_info(module_idP,
			      CC_id,
			      UE_id,
			      subframeP,
			      S_UL_SCHEDULED);
	    
	    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0\n", module_idP,CC_id,frameP,subframeP,UE_id);
#ifdef DEBUG
	    dump_dci(frame_parms, &DCI_pdu->dci_alloc[DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci-1]);
#endif
	    
          }
	  else {
            T(T_ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP),
              T_INT(subframeP), T_INT(harq_pid), T_INT(mcs), T_INT(first_rb[CC_id]), T_INT(rb_table[rb_table_index]),
              T_INT(round));

            LOG_D(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) UE %d (mcs %d, first rb %d, nb_rb %d, rb_table_index %d, TBS %d, harq_pid %d,round %d)\n",
                  module_idP,harq_pid,rnti,CC_id,frameP,subframeP,UE_id,mcs,
                  first_rb[CC_id],rb_table[rb_table_index],
                  rb_table_index,TBS,harq_pid,round);
	  }/* 
	  else if (round > 0) { //we schedule a retransmission

            ndi = UE_template->oldNDI_UL[harq_pid];

            if ((round&3)==0) {
              mcs = openair_daq_vars.target_ue_ul_mcs;
            } else {
              mcs = rvidx_tab[round&3] + 28; //not correct for round==4!

            }

            LOG_I(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE retransmission (mcs %d, first rb %d, nb_rb %d, harq_pid %d, round %d)\n",
                  module_idP,UE_id,rnti,CC_id,frameP,subframeP,mcs,
                  first_rb[CC_id],UE_template->nb_rb_ul[harq_pid],
		  harq_pid, round);

            rballoc = mac_xface->computeRIV(frame_parms->N_RB_UL,
                                            first_rb[CC_id],
                                            UE_template->nb_rb_ul[harq_pid]);
            first_rb[CC_id]+=UE_template->nb_rb_ul[harq_pid];  // increment for next UE allocation
         
	    UE_list->eNB_UE_stats[CC_id][UE_id].num_retransmission_rx+=1;
	    UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx_rx=UE_template->nb_rb_ul[harq_pid];
	    UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx+=UE_template->nb_rb_ul[harq_pid];
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1=mcs;
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2=mcs;
	  }
	   */

        } // UE_is_to_be_scheduled
      } // UE is in PUSCH
    } // loop over UE_id
  } // loop of CC_id
}

#ifdef CBA
void schedule_ulsch_cba_rnti(module_id_t module_idP, unsigned char cooperation_flag, frame_t frameP, sub_frame_t subframeP, unsigned char sched_subframe, uint16_t *first_rb)
{

  eNB_MAC_INST *eNB = &eNB_mac_inst[module_idP];
  UE_list_t         *UE_list=&eNB->UE_list;
  //UE_TEMPLATE       *UE_template;
  void              *ULSCH_dci      = NULL;
  DCI_PDU *DCI_pdu;
  uint8_t CC_id=0;
  uint8_t rb_table_index=0, aggregation=2;
  uint32_t rballoc;
  uint8_t cba_group, cba_resources;
  uint8_t required_rbs[NUM_MAX_CBA_GROUP];
  int8_t num_cba_resources[NUM_MAX_CBA_GROUP];// , weight[NUM_MAX_CBA_GROUP]
  // the following vars should become a vector [MAX_NUM_CCs]
  LTE_DL_FRAME_PARMS   *frame_parms;
  int8_t available_rbs=0;
  uint8_t remaining_rbs=0;
  uint8_t allocated_rbs=0;
  uint8_t total_UEs=UE_list->num_UEs;
  uint8_t active_UEs[NUM_MAX_CBA_GROUP];
  uint8_t total_groups=eNB_mac_inst[module_idP].common_channels[CC_id].num_active_cba_groups;
  int     min_rb_unit=2;
  uint8_t cba_policy=CBA_ES;
  int     UE_id;
  uint8_t mcs[NUM_MAX_CBA_GROUP];
  uint32_t total_cba_resources=0;
  uint32_t total_rbs=0;
  // We compute the weight of each group and initialize some variables

  // loop over all active UEs
  //  for (UE_id=UE_list->head_ul;UE_id>=0;UE_id=UE_list->next_ul[UE_id]) {

  for (cba_group=0; cba_group<total_groups; cba_group++) {
    // UEs in PUSCH with traffic
    //    weight[cba_group] = 0;
    required_rbs[cba_group] = 0;
    num_cba_resources[cba_group]=0;
    active_UEs[cba_group]=0;
    mcs[cba_group]=10;//openair_daq_vars.target_ue_ul_mcs;
  }

  //LOG_D(MAC, "[eNB ] CBA granted ues are %d\n",granted_UEs );

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    frame_parms=mac_xface->get_lte_frame_parms(module_idP,CC_id);
    available_rbs=frame_parms->N_RB_DL-1-first_rb[CC_id];
    remaining_rbs=available_rbs;
    total_groups=eNB_mac_inst[module_idP].common_channels[CC_id].num_active_cba_groups;
    min_rb_unit=get_min_rb_unit(module_idP,CC_id);

    if (available_rbs  < min_rb_unit )
      continue;

    // remove this condition later
    // cba group template uses the exisitng UE template, and thus if a UE
    // is scheduled, the correspodning group can't be used for CBA
    // this can be fixed later
    if (total_groups > 0)  {
      DCI_pdu = &eNB_mac_inst[module_idP].common_channels[CC_id].DCI_pdu;

      for (cba_group=0;
           (cba_group<total_groups)   > (1<<aggregation));
           cba_group++) {
        // equal weight
        //weight[cba_group] = floor(total_UEs/active_groups);//find_num_active_UEs_in_cbagroup(module_idP, cba_group);

        for (UE_id=UE_list->head_ul; UE_id>=0; UE_id=UE_list->next_ul[UE_id]) {
          if (UE_RNTI(module_idP,UE_id)==NOT_A_RNTI)
            continue;

          // simple UE identity based grouping
          if ((UE_id % total_groups) == cba_group) { // this could be simplifed to  active_UEs[UE_id % total_groups]++;
            if ((mac_eNB_get_rrc_status(module_idP,rnti) > RRC_CONNECTED) &&
                (UE_is_to_be_scheduled(module_idP,CC_id,UE_id) == 0)) {
              active_UEs[cba_group]++;
            }
          }

          if (UE_list->UE_template[CC_id][UE_id].pre_assigned_mcs_ul <= 2) {
            mcs[cba_group]= 8; // apply fixed scheduling
          } else if ((UE_id % total_groups) == cba_group) {
            mcs[cba_group]= cmin(mcs[cba_group],UE_list->UE_template[CC_id][UE_id].pre_assigned_mcs_ul);
          }
        }

        mcs[cba_group]= mcs[cba_group];//cmin(mcs[cba_group],openair_daq_vars.target_ue_ul_mcs);

        if (available_rbs < min_rb_unit )
          break;

        // If the group needs some resource
        // determine the total number of allocations (one or multiple DCIs): to be refined
        if ((active_UEs[cba_group] > 0) && (eNB_mac_inst[module_idP].common_channels[CC_id].cba_rnti[cba_group] != 0)) {
          // to be refined in case of : total_UEs >> weight[cba_group]*available_rbs

          switch(cba_policy) {
          case CBA_ES:
            required_rbs[cba_group] = (uint8_t)floor(available_rbs/total_groups); // allocate equally among groups
            num_cba_resources[cba_group]=1;
            break;

            // can't have more than one allocation for the same group/UE
            /*  case CBA_ES_S:
            required_rbs[cba_group] = (uint8_t)floor(available_rbs/total_groups); // allocate equally among groups
            if (required_rbs[cba_group] > min_rb_unit)
            num_cba_resources[cba_group]=(uint8_t)(required_rbs[cba_group]/ min_rb_unit);
            break;*/
          case CBA_PF:
            required_rbs[cba_group] = (uint8_t)floor((active_UEs[cba_group]*available_rbs)/total_UEs);
            num_cba_resources[cba_group]=1;
            break;
            /* case CBA_PF_S:
            required_rbs[cba_group] = (uint8_t)ceil((active_UEs[cba_group]*available_rbs)/total_UEs);
            if (required_rbs[cba_group] > min_rb_unit)
            num_cba_resources[cba_group]=(uint8_t) floor(required_rbs[cba_group] / min_rb_unit);
            break;*/

          default:
            LOG_W(MAC,"unknown CBA policy\n");
            break;
          }

          total_cba_resources+=num_cba_resources[cba_group];
          total_rbs+=required_rbs[cba_group];

          /*  while ((remaining_rbs < required_rbs[cba_group]) &&
          (required_rbs[cba_group] > 0) &&
          (required_rbs[cba_group] < min_rb_unit ))
          required_rbs[cba_group]--;
           */

          /*
          while (rb_table[rb_table_index] < required_rbs[cba_group])
          rb_table_index++;

          while (rb_table[rb_table_index] > remaining_rbs )
          rb_table_index--;

          remaining_rbs-=rb_table[rb_table_index];
          required_rbs[cba_group]=rb_table[rb_table_index];
           */
          //    num_cba_resources[cba_group]=1;

        }
      }

      // phase 2 reduce the number of cba allocations among the groups
      cba_group=0;

      if (total_cba_resources <= 0) {
        return;
      }

      // increase rb if any left: to be done
      cba_group=0;

      while (total_rbs  < available_rbs - 1 ) {
        required_rbs[cba_group%total_groups]++;
        total_rbs++;
        cba_group++;
      }

      // phase 3:
      for (cba_group=0; cba_group<total_groups; cba_group++) {

        LOG_N(MAC,
              "[eNB %d] CC_id %d Frame %d, subframe %d: cba group %d active_ues %d total groups %d mcs %d, available/required rb (%d/%d), num resources %d, ncce required %d \n",
              module_idP, CC_id, frameP, subframeP, cba_group,active_UEs[cba_group],total_groups,
              mcs[cba_group], available_rbs,required_rbs[cba_group],
              num_cba_resources[cba_group],
              (1<<aggregation) * num_cba_resources[cba_group]);

        for (cba_resources=0; cba_resources < num_cba_resources[cba_group]; cba_resources++) {
          rb_table_index =0;

          // check if there was an allocation for this group in the 1st phase
          if (required_rbs[cba_group] == 0 )
            continue;

          while (rb_table[rb_table_index] < (uint8_t) ceil(required_rbs[cba_group] / num_cba_resources[cba_group]) ) {
            rb_table_index++;
          }

          while (rb_table[rb_table_index] > remaining_rbs ) {
            rb_table_index--;
          }

          remaining_rbs-=rb_table[rb_table_index];
          allocated_rbs=rb_table[rb_table_index];

          rballoc = mac_xface->computeRIV(mac_xface->lte_frame_parms->N_RB_UL,
                                          first_rb[CC_id],
                                          rb_table[rb_table_index]);

          first_rb[CC_id]+=rb_table[rb_table_index];
          LOG_N(MAC,
                "[eNB %d] CC_id %d Frame %d, subframeP %d: schedule CBA access %d rnti %x, total/required/allocated/remaining rbs (%d/%d/%d/%d), mcs %d, rballoc %d\n",
                module_idP, CC_id, frameP, subframeP, cba_group,eNB_mac_inst[module_idP].common_channels[CC_id].cba_rnti[cba_group],
                available_rbs, required_rbs[cba_group], allocated_rbs, remaining_rbs,
                mcs[cba_group],rballoc);

          switch (frame_parms->N_RB_UL) {
          case 6:
            mac_xface->macphy_exit("[MAC][eNB] CBA RB=6 not supported \n");
            break;

          case 25:
            if (frame_parms->frame_type == TDD) {
              ULSCH_dci = UE_list->UE_template[CC_id][cba_group].ULSCH_DCI[0];

              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs[cba_group];
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = 1;
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = 1;//tpc;
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cba_group;
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_list->UE_template[CC_id][cba_group].DAI_ul[sched_subframe];
              ((DCI0_5MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = 1;

              //add_ue_spec_dci
              add_common_dci(DCI_pdu,
                             ULSCH_dci,
                             eNB_mac_inst[module_idP].common_channels[CC_id].cba_rnti[cba_group],
                             sizeof(DCI0_5MHz_TDD_1_6_t),
                             aggregation,
                             sizeof_DCI0_5MHz_TDD_1_6_t,
                             format0,
                             0);
            } else {
              ULSCH_dci = UE_list->UE_template[CC_id][cba_group].ULSCH_DCI[0];

              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->type     = 0;
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->mcs      = mcs[cba_group];
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->ndi      = 1;
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->TPC      = 1;//tpc;
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->cshift   = cba_group;
              ((DCI0_5MHz_FDD_t *)ULSCH_dci)->cqi_req  = 1;

              //add_ue_spec_dci
              add_common_dci(DCI_pdu,
                             ULSCH_dci,
                             eNB_mac_inst[module_idP].common_channels[CC_id].cba_rnti[cba_group],
                             sizeof(DCI0_5MHz_FDD_t),
                             aggregation,
                             sizeof_DCI0_5MHz_FDD_t,
                             format0,
                             0);
            }

            LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for CBA group %d, RB 25 format 0\n", module_idP,CC_id,frameP,subframeP,cba_group);
            break;

          case 50:
            if (frame_parms->frame_type == TDD) {
              ULSCH_dci = UE_list->UE_template[CC_id][cba_group].ULSCH_DCI[0];

              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->type     = 0;
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->hopping  = 0;
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->rballoc  = rballoc;
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->mcs      = mcs[cba_group];
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->ndi      = 1;
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->TPC      = 1;//tpc;
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->cshift   = cba_group;
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->dai      = UE_list->UE_template[CC_id][cba_group].DAI_ul[sched_subframe];
              ((DCI0_10MHz_TDD_1_6_t *)ULSCH_dci)->cqi_req  = 1;

              //add_ue_spec_dci
              add_common_dci(DCI_pdu,
                             ULSCH_dci,
                             eNB_mac_inst[module_idP].common_channels[CC_id].cba_rnti[cba_group],
                             sizeof(DCI0_10MHz_TDD_1_6_t),
                             aggregation,
                             sizeof_DCI0_10MHz_TDD_1_6_t,
                             format0,
                             0);
            } else {
              ULSCH_dci = UE_list->UE_template[CC_id][cba_group].ULSCH_DCI[0];

              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->type     = 0;
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->hopping  = 0;
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->rballoc  = rballoc;
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->mcs      = mcs[cba_group];
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->ndi      = 1;
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->TPC      = 1;//tpc;
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->cshift   = cba_group;
              ((DCI0_10MHz_FDD_t *)ULSCH_dci)->cqi_req  = 1;

              //add_ue_spec_dci
              add_common_dci(DCI_pdu,
                             ULSCH_dci,
                             eNB_mac_inst[module_idP].common_channels[CC_id].cba_rnti[cba_group],
                             sizeof(DCI0_10MHz_FDD_t),
                             aggregation,
                             sizeof_DCI0_10MHz_FDD_t,
                             format0,
                             0);
            }

            LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for CBA group %d, RB 50 format 0\n", module_idP,CC_id,frameP,subframeP,cba_group);
            break;

          case 100:
            mac_xface->macphy_exit("[MAC][eNB] CBA RB=100 not supported \n");
            break;

          default:
            break;
          }

          //      break;// for the moment only schedule one
        }
      }
    }
  }
}
#endif
