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

/******************************************************************************
 \file openair2_proc
# \brief print openair2 overall statistics
# \author Navid Nikaein
# \date 2013-2015
# \version 0.2
# \email navid.nikaein@eurecom.fr
# @ingroup _openair2
*/

#include <inttypes.h>

#include "LAYER2/RLC/rlc.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "UTIL/LOG/log.h"
#include "common/ran_context.h"

static mapping rrc_status_names[] = {
  {"RRC_INACTIVE", 0},
  {"RRC_IDLE", 1},
  {"RRC_SI_RECEIVED",2},
  {"RRC_CONNECTED",3},
  {"RRC_RECONFIGURED",4},
  {"RRC_HO_EXECUTION",5},
  {NULL, -1}
};

extern RAN_CONTEXT_t RC;

int dump_eNB_l2_stats(char *buffer, int length)
{

  int eNB_id,UE_id,number_of_cards;
  int len= length;
  int CC_id=0;
  int i;
  protocol_ctxt_t      ctxt;
  rlc_op_status_t rlc_status;
  unsigned int stat_rlc_mode;
  unsigned int stat_tx_pdcp_sdu;
  unsigned int stat_tx_pdcp_bytes;
  unsigned int stat_tx_pdcp_sdu_discarded;
  unsigned int stat_tx_pdcp_bytes_discarded;
  unsigned int stat_tx_data_pdu;
  unsigned int  stat_tx_data_bytes;
  unsigned int  stat_tx_retransmit_pdu_by_status;
  unsigned int  stat_tx_retransmit_bytes_by_status;
  unsigned int stat_tx_retransmit_pdu;
  unsigned int  stat_tx_retransmit_bytes;
  unsigned int  stat_tx_control_pdu;
  unsigned int  stat_tx_control_bytes;
  unsigned int  stat_rx_pdcp_sdu;
  unsigned int  stat_rx_pdcp_bytes;
  unsigned int  stat_rx_data_pdus_duplicate;
  unsigned int  stat_rx_data_bytes_duplicate;
  unsigned int  stat_rx_data_pdu;
  unsigned int  stat_rx_data_bytes;
  unsigned int  stat_rx_data_pdu_dropped;
  unsigned int  stat_rx_data_bytes_dropped;
  unsigned int  stat_rx_data_pdu_out_of_window;
  unsigned int  stat_rx_data_bytes_out_of_window;
  unsigned int  stat_rx_control_pdu;
  unsigned int  stat_rx_control_bytes;
  unsigned int  stat_timer_reordering_timed_out;
  unsigned int  stat_timer_poll_retransmit_timed_out;
  unsigned int  stat_timer_status_prohibit_timed_out;

#ifdef EXMIMO
  number_of_cards=1;
#else
  number_of_cards=NB_eNB_INST;
#endif
  eNB_MAC_INST *eNB;
  UE_list_t *UE_list;

  for (eNB_id=0; eNB_id<number_of_cards; eNB_id++) {
    /* reset the values */
    eNB = RC.mac[eNB_id];
    UE_list = &eNB->UE_list;

    for (CC_id=0 ; CC_id < MAX_NUM_CCs; CC_id++) {
      eNB->eNB_stats[CC_id].dlsch_bitrate= 0;

      len += sprintf(&buffer[len],"eNB %d CC %d Frame %d: Active UEs %d, Available PRBs %d, nCCE %d, Scheduling decisions %d, Missed Deadlines %d \n",
                     eNB_id, CC_id, eNB->frame,
                     eNB->eNB_stats[CC_id].num_dlactive_UEs,
                     eNB->eNB_stats[CC_id].available_prbs,
                     eNB->eNB_stats[CC_id].available_ncces,
		     eNB->eNB_stats[CC_id].sched_decisions,
		     eNB->eNB_stats[CC_id].missed_deadlines);
      
      len += sprintf(&buffer[len],"BCCH , NB_TX_MAC = %d, transmitted bytes (TTI %d, total %d) MCS (TTI %d)\n",
		     eNB->eNB_stats[CC_id].total_num_bcch_pdu,
		     eNB->eNB_stats[CC_id].bcch_buffer,
		     eNB->eNB_stats[CC_id].total_bcch_buffer,
		     eNB->eNB_stats[CC_id].bcch_mcs);

      len += sprintf(&buffer[len],"PCCH , NB_TX_MAC = %d, transmitted bytes (TTI %d, total %d) MCS (TTI %d)\n",
         eNB->eNB_stats[CC_id].total_num_pcch_pdu,
         eNB->eNB_stats[CC_id].pcch_buffer,
         eNB->eNB_stats[CC_id].total_pcch_buffer,
         eNB->eNB_stats[CC_id].pcch_mcs);
      
      eNB->eNB_stats[CC_id].dlsch_bitrate=((eNB->eNB_stats[CC_id].dlsch_bytes_tx*8)/((eNB->frame + 1)*10));
      eNB->eNB_stats[CC_id].total_dlsch_pdus_tx+=eNB->eNB_stats[CC_id].dlsch_pdus_tx;
      eNB->eNB_stats[CC_id].total_dlsch_bytes_tx+=eNB->eNB_stats[CC_id].dlsch_bytes_tx;
      eNB->eNB_stats[CC_id].total_dlsch_bitrate=((eNB->eNB_stats[CC_id].total_dlsch_bytes_tx*8)/((eNB->frame + 1)*10));

      eNB->eNB_stats[CC_id].ulsch_bitrate=((eNB->eNB_stats[CC_id].ulsch_bytes_rx*8)/((eNB->frame + 1)*10));
      eNB->eNB_stats[CC_id].total_ulsch_bitrate=((eNB->eNB_stats[CC_id].total_ulsch_bytes_rx*8)/((eNB->frame + 1)*10));

      len += sprintf(&buffer[len],"DLSCH bitrate (TTI %u, avg %u) kbps, Transmitted bytes (TTI %u, total %u), Transmitted PDU (TTI %u, total %u) \n",
                     eNB->eNB_stats[CC_id].dlsch_bitrate,
                     eNB->eNB_stats[CC_id].total_dlsch_bitrate,
                     eNB->eNB_stats[CC_id].dlsch_bytes_tx,
                     eNB->eNB_stats[CC_id].total_dlsch_bytes_tx,
                     eNB->eNB_stats[CC_id].dlsch_pdus_tx,
                     eNB->eNB_stats[CC_id].total_dlsch_pdus_tx);

      len += sprintf(&buffer[len],"ULSCH bitrate (TTI %u, avg %u) kbps, Received bytes (TTI %u, total %u), Received PDU (TTI %lu, total %u) \n",
                     eNB->eNB_stats[CC_id].ulsch_bitrate,
                     eNB->eNB_stats[CC_id].total_ulsch_bitrate,
                     eNB->eNB_stats[CC_id].ulsch_bytes_rx,
                     eNB->eNB_stats[CC_id].total_ulsch_bytes_rx,
                     eNB->eNB_stats[CC_id].ulsch_pdus_rx,
                     eNB->eNB_stats[CC_id].total_ulsch_pdus_rx);
    }

    len += sprintf(&buffer[len],"\n");

    for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
      for (i=0; i<UE_list->numactiveCCs[UE_id]; i++) {
        CC_id=UE_list->ordered_CCids[i][UE_id];
	
	UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_bitrate=((UE_list->eNB_UE_stats[CC_id][UE_id].TBS*8)/((eNB->frame + 1)*10));
        UE_list->eNB_UE_stats[CC_id][UE_id].total_dlsch_bitrate= ((UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes*8)/((eNB->frame + 1)*10));
        UE_list->eNB_UE_stats[CC_id][UE_id].total_overhead_bytes+=  UE_list->eNB_UE_stats[CC_id][UE_id].overhead_bytes;
        UE_list->eNB_UE_stats[CC_id][UE_id].avg_overhead_bytes=((UE_list->eNB_UE_stats[CC_id][UE_id].total_overhead_bytes*8)/((eNB->frame + 1)*10));
	
	UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_bitrate=((UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS*8)/((eNB->frame + 1)*10));
        UE_list->eNB_UE_stats[CC_id][UE_id].total_ulsch_bitrate= ((UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes_rx*8)/((eNB->frame + 1)*10));
       
        len += sprintf(&buffer[len],"[MAC] UE %d (DLSCH),status %s, RNTI %x : CQI %d, MCS1 %d, MCS2 %d, RB (tx %d, retx %d, total %d), ncce (tx %d, retx %d) \n",
                       UE_id,
                       map_int_to_str(rrc_status_names, UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status),
                       UE_list->eNB_UE_stats[CC_id][UE_id].crnti,
                       UE_list->UE_sched_ctrl[UE_id].dl_cqi[CC_id],
                       UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1,
                       UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2,
                       UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used,
                       UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used,
                       UE_list->eNB_UE_stats[CC_id][UE_id].ncce_used,
                       UE_list->eNB_UE_stats[CC_id][UE_id].ncce_used_retx
                      );

        len += sprintf(&buffer[len],
                       "[MAC] DLSCH bitrate (TTI %d, avg %d), Transmitted bytes "
                       "(TTI %d, total %"PRIu64"), Total Transmitted PDU %d, Overhead "
                       "(TTI %"PRIu64", total %"PRIu64", avg %"PRIu64")\n",
                       UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_bitrate,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_dlsch_bitrate,
                       UE_list->eNB_UE_stats[CC_id][UE_id].TBS,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_num_pdus,
                       UE_list->eNB_UE_stats[CC_id][UE_id].overhead_bytes,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_overhead_bytes,
                       UE_list->eNB_UE_stats[CC_id][UE_id].avg_overhead_bytes
                      );
        

	len += sprintf(&buffer[len],"[MAC] UE %d (ULSCH), Status %s, Failute timer %d, RNTI %x : rx power (normalized %d,  target %d), MCS (pre %d, post %d), RB (rx %d, retx %d, total %d), Current TBS %d \n",
                       UE_id,
                       map_int_to_str(rrc_status_names, UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status),
		       UE_list->UE_sched_ctrl[UE_id].ul_failure_timer,
                       UE_list->eNB_UE_stats[CC_id][UE_id].crnti,
		       UE_list->eNB_UE_stats[CC_id][UE_id].normalized_rx_power,
		       UE_list->eNB_UE_stats[CC_id][UE_id].target_rx_power,
		       UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1,
		       UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2,
		       UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_rx,
		       UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx_rx,
		       UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx,
		       UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS
		       );

	len += sprintf(&buffer[len],
                       "[MAC] ULSCH bitrate (TTI %d, avg %d), received bytes (total %"PRIu64"),"
                       "Total received PDU %d, Total errors %d\n", 
		       UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_bitrate,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_ulsch_bitrate,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes_rx,
                       UE_list->eNB_UE_stats[CC_id][UE_id].total_num_pdus_rx,
                       UE_list->eNB_UE_stats[CC_id][UE_id].num_errors_rx);

        len+= sprintf(&buffer[len],"[MAC] Received PHR PH = %d (db)\n", UE_list->UE_template[CC_id][UE_id].phr_info);
        len+= sprintf(&buffer[len],"[MAC] Estimated size LCGID[0][1][2][3] = %u %u %u %u\n",
                      UE_list->UE_template[CC_id][UE_id].ul_buffer_info[LCGID0],
                      UE_list->UE_template[CC_id][UE_id].ul_buffer_info[LCGID1],
                      UE_list->UE_template[CC_id][UE_id].ul_buffer_info[LCGID2],
                      UE_list->UE_template[CC_id][UE_id].ul_buffer_info[LCGID3]
                     );
      }
      
      PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt,
				     eNB_id,
				     ENB_FLAG_YES,
				     UE_list->eNB_UE_stats[0][UE_id].crnti,//UE_PCCID(eNB_id,UE_id)][UE_id].crnti, 
				     eNB->frame,
				     eNB->subframe,
				     eNB_id);

      rlc_status = rlc_stat_req(&ctxt,
				SRB_FLAG_YES,
				DCCH,
				&stat_rlc_mode,
				&stat_tx_pdcp_sdu,
				&stat_tx_pdcp_bytes,
				&stat_tx_pdcp_sdu_discarded,
				&stat_tx_pdcp_bytes_discarded,
				&stat_tx_data_pdu,
				&stat_tx_data_bytes,
				&stat_tx_retransmit_pdu_by_status,
				&stat_tx_retransmit_bytes_by_status,
				&stat_tx_retransmit_pdu,
				&stat_tx_retransmit_bytes,
				&stat_tx_control_pdu,
				&stat_tx_control_bytes,
				&stat_rx_pdcp_sdu,
				&stat_rx_pdcp_bytes,
				&stat_rx_data_pdus_duplicate,
				&stat_rx_data_bytes_duplicate,
				&stat_rx_data_pdu,
				&stat_rx_data_bytes,
				&stat_rx_data_pdu_dropped,
				&stat_rx_data_bytes_dropped,
				&stat_rx_data_pdu_out_of_window,
				&stat_rx_data_bytes_out_of_window,
				&stat_rx_control_pdu,
				&stat_rx_control_bytes,
				&stat_timer_reordering_timed_out,
				&stat_timer_poll_retransmit_timed_out,
				&stat_timer_status_prohibit_timed_out);
      
      if (rlc_status == RLC_OP_STATUS_OK) {
	len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_SDU_TO_TX = %d (bytes %d)\tNB_SDU_TO_TX_DISC %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_tx_pdcp_sdu,
		     stat_tx_pdcp_bytes,
		     stat_tx_pdcp_sdu_discarded,
		     stat_tx_pdcp_bytes_discarded);
	
	len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_TX_DATA   = %d (bytes %d)\tNB_TX_CONTROL %d (bytes %d)\tNB_TX_RETX %d (bytes %d)\tNB_TX_RETX_BY_STATUS = %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_tx_data_pdu,
		     stat_tx_data_bytes,
		     stat_tx_control_pdu,
		     stat_tx_control_bytes,
		     stat_tx_retransmit_pdu,
		     stat_tx_retransmit_bytes,
		     stat_tx_retransmit_pdu_by_status,
		     stat_tx_retransmit_bytes_by_status);
		
	
	len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_RX_DATA   = %d (bytes %d)\tNB_RX_CONTROL %d (bytes %d)\tNB_RX_DUPL %d (bytes %d)\tNB_RX_DROP = %d (bytes %d)\tNB_RX_OUT_OF_WINDOW = %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_rx_data_pdu,
		     stat_rx_data_bytes,
		     stat_rx_control_pdu,
		     stat_rx_control_bytes,
		     stat_rx_data_pdus_duplicate,
		     stat_rx_data_bytes_duplicate,
		     stat_rx_data_pdu_dropped,
		     stat_rx_data_bytes_dropped,
		     stat_rx_data_pdu_out_of_window,
		     stat_rx_data_bytes_out_of_window);
	

	len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, RX_REODERING_TIMEOUT = %d\tRX_POLL_RET_TIMEOUT %d\tRX_PROHIBIT_TIME_OUT %d\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_timer_reordering_timed_out,
		     stat_timer_poll_retransmit_timed_out,
		     stat_timer_status_prohibit_timed_out);

	len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_SDU_TO_RX = %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_rx_pdcp_sdu,
		     stat_rx_pdcp_bytes);
      }
      
      rlc_status = rlc_stat_req(&ctxt,
				SRB_FLAG_NO,
				DTCH-2, // DRB_IDENTITY
				&stat_rlc_mode,
				&stat_tx_pdcp_sdu,
				&stat_tx_pdcp_bytes,
				&stat_tx_pdcp_sdu_discarded,
				&stat_tx_pdcp_bytes_discarded,
				&stat_tx_data_pdu,
				&stat_tx_data_bytes,
				&stat_tx_retransmit_pdu_by_status,
				&stat_tx_retransmit_bytes_by_status,
				&stat_tx_retransmit_pdu,
				&stat_tx_retransmit_bytes,
				&stat_tx_control_pdu,
				&stat_tx_control_bytes,
				&stat_rx_pdcp_sdu,
				&stat_rx_pdcp_bytes,
				&stat_rx_data_pdus_duplicate,
				&stat_rx_data_bytes_duplicate,
				&stat_rx_data_pdu,
				&stat_rx_data_bytes,
				&stat_rx_data_pdu_dropped,
				&stat_rx_data_bytes_dropped,
				&stat_rx_data_pdu_out_of_window,
				&stat_rx_data_bytes_out_of_window,
				&stat_rx_control_pdu,
				&stat_rx_control_bytes,
				&stat_timer_reordering_timed_out,
				&stat_timer_poll_retransmit_timed_out,
				&stat_timer_status_prohibit_timed_out);
      
      if (rlc_status == RLC_OP_STATUS_OK) {
	len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_SDU_TO_TX = %d (bytes %d)\tNB_SDU_TO_TX_DISC %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_tx_pdcp_sdu,
		     stat_tx_pdcp_bytes,
		     stat_tx_pdcp_sdu_discarded,
		     stat_tx_pdcp_bytes_discarded);
	
	len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_TX_DATA   = %d (bytes %d)\tNB_TX_CONTROL %d (bytes %d)\tNB_TX_RETX %d (bytes %d)\tNB_TX_RETX_BY_STATUS = %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_tx_data_pdu,
		     stat_tx_data_bytes,
		     stat_tx_control_pdu,
		     stat_tx_control_bytes,
		     stat_tx_retransmit_pdu,
		     stat_tx_retransmit_bytes,
		     stat_tx_retransmit_pdu_by_status,
		     stat_tx_retransmit_bytes_by_status);
		
	
	len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_RX_DATA   = %d (bytes %d)\tNB_RX_CONTROL %d (bytes %d)\tNB_RX_DUPL %d (bytes %d)\tNB_RX_DROP = %d (bytes %d)\tNB_RX_OUT_OF_WINDOW = %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_rx_data_pdu,
		     stat_rx_data_bytes,
		     stat_rx_control_pdu,
		     stat_rx_control_bytes,
		     stat_rx_data_pdus_duplicate,
		     stat_rx_data_bytes_duplicate,
		     stat_rx_data_pdu_dropped,
		     stat_rx_data_bytes_dropped,
		     stat_rx_data_pdu_out_of_window,
		     stat_rx_data_bytes_out_of_window);
	

	len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, RX_REODERING_TIMEOUT = %d\tRX_POLL_RET_TIMEOUT %d\tRX_PROHIBIT_TIME_OUT %d\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_timer_reordering_timed_out,
		     stat_timer_poll_retransmit_timed_out,
		     stat_timer_status_prohibit_timed_out);

	len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_SDU_TO_RX = %d (bytes %d)\n",
		     (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
		     stat_rx_pdcp_sdu,
		     stat_rx_pdcp_bytes);

      }  		
    }
  }
  
  return len + 1 /* SR: for trailing \0 */;
}

#ifdef PROC
int openair2_stats_read(char *buffer, char **my_buffer, off_t off, int length)
{

  int len = 0,fg,Overhead, Sign;
  unsigned int i,j,k,kk;
  unsigned int Mod_id = 0,CH_index;
  unsigned int tx_pdcp_sdu;
  unsigned int tx_pdcp_sdu_discarded;
  unsigned int tx_retransmit_pdu_unblock;
  unsigned int tx_retransmit_pdu_by_status;
  unsigned int tx_retransmit_pdu;
  unsigned int tx_data_pdu;
  unsigned int tx_control_pdu;
  unsigned int rx_sdu;
  unsigned int rx_error_pdu;
  unsigned int rx_data_pdu;
  unsigned int rx_data_pdu_out_of_window;
  unsigned int rx_control_pdu;

  //    if (mac_xface->is_cluster_head == 0) {
  for (k=0; k<NB_INST; k++) {



    if (Mac_rlc_xface->Is_cluster_head[k] == 0) {
#ifndef PHY_EMUL_ONE_MACHINE
      Mod_id=k-NB_CH_INST;

      len+=sprintf(&buffer[len],"UE TTI: %d\n",Mac_rlc_xface->frame);

      for (CH_index = 0; CH_index<NB_CNX_UE; CH_index++) {


        if (UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Active==1) {
          len+=sprintf(&buffer[len],"CH %d: Wideband SINR %d dB---\n",
                       CH_index,UE_mac_inst[Mod_id].Def_meas[CH_index].Wideband_sinr);
          len+=sprintf(&buffer[len],"CH %d: Subband SINR (dB) :",
                       CH_index);

          for (fg=0; fg<NUMBER_OF_MEASUREMENT_SUBBANDS; fg++) {
            len+=sprintf(&buffer[len],"%d ",UE_mac_inst[Mod_id].Def_meas[CH_index].Sinr_meas[0][fg]);
          }

          len+=sprintf(&buffer[len],"\n");


          len+=sprintf(&buffer[len],"BCCH %d, NB_RX_MAC = %d (%d errors)\n",
                       UE_mac_inst[Mod_id].Bcch_lchan[CH_index].Lchan_info.Lchan_id.Index,
                       UE_mac_inst[Mod_id].Bcch_lchan[CH_index].Lchan_info.NB_RX,
                       UE_mac_inst[Mod_id].Bcch_lchan[CH_index].Lchan_info.NB_RX_ERRORS);



          len+=sprintf(&buffer[len],"CCCH %d, NB_RX_MAC = %d (%d errors)\n",
                       UE_mac_inst[Mod_id].Ccch_lchan[CH_index].Lchan_info.Lchan_id.Index,
                       UE_mac_inst[Mod_id].Ccch_lchan[CH_index].Lchan_info.NB_RX,
                       UE_mac_inst[Mod_id].Ccch_lchan[CH_index].Lchan_info.NB_RX_ERRORS);


          len+=sprintf(&buffer[len],"LCHAN %d (DCCH), NB_TX_MAC = %d (%d bits/TTI, %d kbits/sec), NB_RX_MAC = %d (%d errors)\n",
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.Lchan_id.Index,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.NB_TX,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.output_rate,
                       (10*UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.output_rate)>>5,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.NB_RX,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.NB_RX_ERRORS);

          for(i=1; i<NB_RAB_MAX; i++) {
            if (UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Active==1) {
              Overhead=UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.output_rate - Pdcp_stats_tx_rate[k][CH_index][i];

              if(Overhead<0) {
                Overhead=-Overhead;
                Sign=-1;
              } else {
                Sign=1;
              }

              len+=sprintf(&buffer[len],
                           "[PDCP]LCHAN %d: NB_TX = %d ,Tx_rate =(%d bits/TTI ,%d Kbits/s), NB_RX = %d ,Rx_rate =(%d bits/TTI ,%d Kbits/s) , LAYER2 TX OVERHEAD: %d Kbits/s\n",
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                           Pdcp_stats_tx[k][CH_index][i],
                           Pdcp_stats_tx_rate[k][CH_index][i],
                           (10*Pdcp_stats_tx_rate[k][CH_index][i])>>5,
                           Pdcp_stats_rx[k][CH_index][i],
                           Pdcp_stats_rx_rate[k][CH_index][i],
                           (10*Pdcp_stats_rx_rate[k][CH_index][i])>>5,
                           Sign*(10*Overhead)>>5);


              int status =  rlc_stat_req     (k,
                                              UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                                              &tx_pdcp_sdu,
                                              &tx_pdcp_sdu_discarded,
                                              &tx_retransmit_pdu_unblock,
                                              &tx_retransmit_pdu_by_status,
                                              &tx_retransmit_pdu,
                                              &tx_data_pdu,
                                              &tx_control_pdu,
                                              &rx_sdu,
                                              &rx_error_pdu,
                                              &rx_data_pdu,
                                              &rx_data_pdu_out_of_window,
                                              &rx_control_pdu) ;

              if (status == RLC_OP_STATUS_OK) {
                len+=sprintf(&buffer[len],"RLC LCHAN %d, NB_SDU_TO_TX = %d\tNB_SDU_DISC %d\tNB_RX_SDU %d\n",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             tx_pdcp_sdu,
                             tx_pdcp_sdu_discarded,
                             rx_sdu);
                len+=sprintf(&buffer[len],"RLC LCHAN %d, NB_TB_TX_DATA = %d\tNB_TB_TX_CONTROL %d\tNB_TX_TB_RETRANS %d",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             tx_data_pdu,
                             tx_control_pdu,
                             tx_retransmit_pdu);
                len+=sprintf(&buffer[len],"\tRLC LCHAN %d, NB_TX_TB_RETRANS_BY_STATUS = %d\tNB_TX_TB_RETRANS_PADD %d\n",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             tx_retransmit_pdu_by_status,
                             tx_retransmit_pdu_unblock);
                len+=sprintf(&buffer[len],"RLC LCHAN %d, NB_RX_DATA = %d\tNB_RX_TB_OUT_WIN %d\tNB_RX_TB_CORRUPT %d\n",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             rx_data_pdu,
                             rx_data_pdu_out_of_window,
                             rx_error_pdu);
              }

              len+=sprintf(&buffer[len],"[MAC]: LCHAN %d, NB_TX_MAC = %d (%d bits/TTI, %d kbits/s), NB_RX_MAC = %d (%d errors)\n",
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_TX,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.output_rate,
                           (10*UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.output_rate)>>5,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX_ERRORS);
              len+=sprintf(&buffer[len],"        TX per TB: ");

              for(kk=0; kk<MAX_NUMBER_TB_PER_LCHAN/2; kk++) {
                len+=sprintf(&buffer[len],"%d . ",UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_TX_TB[kk]);
              }

              len+=sprintf(&buffer[len],"\n");
              len+=sprintf(&buffer[len],"        RXerr per TB: ");

              for(kk=0; kk<MAX_NUMBER_TB_PER_LCHAN/2; kk++)
                len+=sprintf(&buffer[len],"%d/%d . ",UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX_ERRORS_TB[kk],
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX_TB[kk]);

              len+=sprintf(&buffer[len],"\n");



            }

          }
        }
      }

#endif //PHY_EMUL_ONE_MACHINE
    } else if(Mac_rlc_xface->Is_cluster_head[k] ==1) {

      Mod_id=k;
      len+=sprintf(&buffer[len],
                   "-------------------------------------------------------------------CH %d: TTI: %d------------------------------------------------------------------\n",
                   NODE_ID[Mod_id],Mac_rlc_xface->frame);

      for(i=1; i<=NB_CNX_CH; i++) {
        if (CH_mac_inst[Mod_id].Dcch_lchan[i].Active==1) {
          len+=sprintf(&buffer[len],"\nMR index %d: DL SINR (feedback) %d dB, CQI: %s\n\n",
                       i,//CH_rrc_inst[Mod_id].Info.UE_list[i].L2_id[0],
                       CH_mac_inst[Mod_id].Def_meas[i].Wideband_sinr,
                       print_cqi(CH_mac_inst[Mod_id].Def_meas[i].cqi));

          len+=sprintf(&buffer[len],
                       "[MAC] LCHAN %d (DCCH), NB_TX_MAC= %d (%d bits/TTI, %d kbits/s), NB_RX_MAC= %d (errors %d, sacch errors %d, sach errors %d, sach_missing %d)\n\n",
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.Lchan_id.Index,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_TX,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.output_rate,
                       (10*CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.output_rate)>>5,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_ERRORS,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_SACCH_ERRORS,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_SACH_ERRORS,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_SACH_MISSING);

          for(j=0; j<NB_RAB_MAX; j++) {
            if (CH_mac_inst[Mod_id].Dtch_lchan[j][i].Active==1) {
              Overhead=CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.output_rate - Pdcp_stats_tx_rate[k][i][j];

              if(Overhead<0) {
                Overhead=-Overhead;
                Sign=-1;
              } else {
                Sign=1;
              }

              len+=sprintf(&buffer[len],
                           "[PDCP]LCHAN %d: NB_TX = %d ,Tx_rate =(%d bits/TTI ,%d Kbits/s), NB_RX = %d ,Rx_rate =(%d bits/TTI ,%d Kbits/s), LAYER2 TX OVERHEAD= %d Kbits/s\n",
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                           Pdcp_stats_tx[k][i][j],
                           Pdcp_stats_tx_rate[k][i][j],
                           (10*Pdcp_stats_tx_rate[k][i][j])>>5,
                           Pdcp_stats_rx[k][i][j],
                           Pdcp_stats_rx_rate[k][i][j],
                           (10*Pdcp_stats_rx_rate[k][i][j])>>5,
                           Sign*(10*Overhead)>>5);
              int status =  rlc_stat_req     (k,
                                              CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                                              &tx_pdcp_sdu,
                                              &tx_pdcp_sdu_discarded,
                                              &tx_retransmit_pdu_unblock,
                                              &tx_retransmit_pdu_by_status,
                                              &tx_retransmit_pdu,
                                              &tx_data_pdu,
                                              &tx_control_pdu,
                                              &rx_sdu,
                                              &rx_error_pdu,
                                              &rx_data_pdu,
                                              &rx_data_pdu_out_of_window,
                                              &rx_control_pdu) ;
              /*
              if (status == RLC_OP_STATUS_OK) {
              len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_SDU_TO_TX = %d\tNB_SDU_DISC %d\tNB_RX_SDU %d\n",
                  CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
               tx_pdcp_sdu,
               tx_pdcp_sdu_discarded,
               rx_sdu);
              len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_TB_TX_DATA = %d\tNB_TB_TX_CONTROL %d\tNB_TX_TB_RETRANS %\n",
                  CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
               tx_data_pdu,
               tx_control_pdu,
               tx_retransmit_pdu);
              len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_TX_TB_RETRANS_BY_STATUS = %d\tNB_TX_TB_RETRANS_PADD %d\n",
                  CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
               tx_retransmit_pdu_by_status,
               tx_retransmit_pdu_unblock);
              len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_RX_DATA = %d\tNB_RX_TB_OUT_WIN %d\tNB_RX_TB_CORRUPT %d\n",
                  CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
               rx_data_pdu,
               rx_data_pdu_out_of_window,
               rx_error_pdu);
              }
              */
              len+=sprintf(&buffer[len],
                           "[MAC]LCHAN %d (CNX %d,RAB %d), NB_TX_MAC= %d (%d bits/TTI, %d kbit/s), NB_RX_MAC= %d (errors %d, sacch_errors %d, sach_errors %d, sach_missing %d)\n",
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                           i,j,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_TX,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.output_rate,
                           (10*CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.output_rate)>>5,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_ERRORS,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_SACCH_ERRORS,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_SACH_ERRORS,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_SACH_MISSING);
              len+=sprintf(&buffer[len],"[MAC][SCHEDULER] TX Arrival Rate %d, TX Service Rate %d, RX Arrival rate %d, RX Service rate %d, NB_BW_REQ_RX %d\n\n",
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Arrival_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Tx_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Req_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Rx_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_BW_REQ_RX);

              /*
                      len+=sprintf(&buffer[len],"        TX per TB: ");
                      for(kk=0;kk<MAX_NUMBER_TB_PER_LCHAN/2;kk++)
                  len+=sprintf(&buffer[len],"%d.",CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_TX_TB[kk]);
                      len+=sprintf(&buffer[len],"\n");
                      len+=sprintf(&buffer[len],"        RXerr per TB: ");
                      for(kk=0;kk<MAX_NUMBER_TB_PER_LCHAN/2;kk++)
                  len+=sprintf(&buffer[len],"%d/%d . ",CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_ERRORS_TB[kk],
                         CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_TB[kk]);
                      len+=sprintf(&buffer[len],"\n");
              */
            }
          }
        }
      }

    }
  }

  return len;
}

#endif
