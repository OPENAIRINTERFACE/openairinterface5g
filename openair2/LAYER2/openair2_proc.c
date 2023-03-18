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
#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "common/utils/LOG/log.h"
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

int dump_eNB_l2_stats(char *buffer, int length) {
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
    number_of_cards=NB_eNB_INST;
    eNB_MAC_INST *eNB;
    UE_info_t *UE_info;

    for (eNB_id=0; eNB_id<number_of_cards; eNB_id++) {
        /* reset the values */
        eNB = RC.mac[eNB_id];
        UE_info = &eNB->UE_info;

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

        for (UE_id=UE_info->list.head; UE_id>=0; UE_id=UE_info->list.next[UE_id]) {
            for (i=0; i<UE_info->numactiveCCs[UE_id]; i++) {
                CC_id=UE_info->ordered_CCids[i][UE_id];
                UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_bitrate=((UE_info->eNB_UE_stats[CC_id][UE_id].TBS*8)/((eNB->frame + 1)*10));
                UE_info->eNB_UE_stats[CC_id][UE_id].total_dlsch_bitrate= ((UE_info->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes*8)/((eNB->frame + 1)*10));
                UE_info->eNB_UE_stats[CC_id][UE_id].total_overhead_bytes+=  UE_info->eNB_UE_stats[CC_id][UE_id].overhead_bytes;
                UE_info->eNB_UE_stats[CC_id][UE_id].avg_overhead_bytes=((UE_info->eNB_UE_stats[CC_id][UE_id].total_overhead_bytes*8)/((eNB->frame + 1)*10));
                UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_bitrate=((UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_TBS*8)/((eNB->frame + 1)*10));
                UE_info->eNB_UE_stats[CC_id][UE_id].total_ulsch_bitrate= ((UE_info->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes_rx*8)/((eNB->frame + 1)*10));
                len += sprintf(&buffer[len],"[MAC] UE %d (DLSCH),status %s, RNTI %x : CQI %d, MCS1 %d, MCS2 %d, RB (tx %d, retx %d, total %d), ncce (tx %d, retx %d) \n",
                               UE_id,
                               map_int_to_str(rrc_status_names, UE_info->eNB_UE_stats[CC_id][UE_id].rrc_status),
                               UE_info->eNB_UE_stats[CC_id][UE_id].crnti,
                               UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],
                               UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1,
                               UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2,
                               UE_info->eNB_UE_stats[CC_id][UE_id].rbs_used,
                               UE_info->eNB_UE_stats[CC_id][UE_id].rbs_used_retx,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_rbs_used,
                               UE_info->eNB_UE_stats[CC_id][UE_id].ncce_used,
                               UE_info->eNB_UE_stats[CC_id][UE_id].ncce_used_retx
                              );
                len += sprintf(&buffer[len],
                               "[MAC] DLSCH bitrate (TTI %d, avg %d), Transmitted bytes "
                               "(TTI %d, total %"PRIu64"), Total Transmitted PDU %d, Overhead "
                               "(TTI %"PRIu64", total %"PRIu64", avg %"PRIu64")\n",
                               UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_bitrate,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_dlsch_bitrate,
                               UE_info->eNB_UE_stats[CC_id][UE_id].TBS,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_num_pdus,
                               UE_info->eNB_UE_stats[CC_id][UE_id].overhead_bytes,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_overhead_bytes,
                               UE_info->eNB_UE_stats[CC_id][UE_id].avg_overhead_bytes
                              );
                len += sprintf(&buffer[len],"[MAC] UE %d (ULSCH), Status %s, Failute timer %d, RNTI %x : snr (%d,  target %d), MCS (pre %d, post %d), RB (rx %d, retx %d, total %d), Current TBS %d \n",
                               UE_id,
                               map_int_to_str(rrc_status_names, UE_info->eNB_UE_stats[CC_id][UE_id].rrc_status),
                               UE_info->UE_sched_ctrl[UE_id].ul_failure_timer,
                               UE_info->eNB_UE_stats[CC_id][UE_id].crnti,
                               UE_info->eNB_UE_stats[CC_id][UE_id].snr,
                               UE_info->eNB_UE_stats[CC_id][UE_id].target_snr,
                               UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1,
                               UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2,
                               UE_info->eNB_UE_stats[CC_id][UE_id].rbs_used_rx,
                               UE_info->eNB_UE_stats[CC_id][UE_id].rbs_used_retx_rx,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx,
                               UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_TBS
                              );
                len += sprintf(&buffer[len],
                               "[MAC] ULSCH bitrate (TTI %d, avg %d), received bytes (total %"PRIu64"),"
                               "Total received PDU %d, Total errors %d\n",
                               UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_bitrate,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_ulsch_bitrate,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes_rx,
                               UE_info->eNB_UE_stats[CC_id][UE_id].total_num_pdus_rx,
                               UE_info->eNB_UE_stats[CC_id][UE_id].num_errors_rx);
                len+= sprintf(&buffer[len],"[MAC] Received PHR PH = %d (db)\n", UE_info->UE_template[CC_id][UE_id].phr_info);
                len+= sprintf(&buffer[len],"[MAC] Estimated size LCGID[0][1][2][3] = %u %u %u %u\n",
                              UE_info->UE_template[CC_id][UE_id].ul_buffer_info[LCGID0],
                              UE_info->UE_template[CC_id][UE_id].ul_buffer_info[LCGID1],
                              UE_info->UE_template[CC_id][UE_id].ul_buffer_info[LCGID2],
                              UE_info->UE_template[CC_id][UE_id].ul_buffer_info[LCGID3]
                             );
            }

            PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt,
                                           eNB_id,
                                           ENB_FLAG_YES,
                                           UE_info->eNB_UE_stats[0][UE_id].crnti,//UE_PCCID(eNB_id,UE_id)][UE_id].crnti,
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
                len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_SDU_TO_TX = %u (bytes %u)\tNB_SDU_TO_TX_DISC %u (bytes %u)\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_tx_pdcp_sdu,
                             stat_tx_pdcp_bytes,
                             stat_tx_pdcp_sdu_discarded,
                             stat_tx_pdcp_bytes_discarded);
                len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_TX_DATA   = %u (bytes %u)\tNB_TX_CONTROL %u (bytes %u)\tNB_TX_RETX %u (bytes %u)\tNB_TX_RETX_BY_STATUS = %u (bytes %u)\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_tx_data_pdu,
                             stat_tx_data_bytes,
                             stat_tx_control_pdu,
                             stat_tx_control_bytes,
                             stat_tx_retransmit_pdu,
                             stat_tx_retransmit_bytes,
                             stat_tx_retransmit_pdu_by_status,
                             stat_tx_retransmit_bytes_by_status);
                len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_RX_DATA   = %u (bytes %u)\tNB_RX_CONTROL %u (bytes %u)\tNB_RX_DUPL %u (bytes %u)\tNB_RX_DROP = %u (bytes %u)\tNB_RX_OUT_OF_WINDOW = %u (bytes %u)\n",
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
                len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, RX_REODERING_TIMEOUT = %u\tRX_POLL_RET_TIMEOUT %u\tRX_PROHIBIT_TIME_OUT %u\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_timer_reordering_timed_out,
                             stat_timer_poll_retransmit_timed_out,
                             stat_timer_status_prohibit_timed_out);
                len+=sprintf(&buffer[len],"[RLC] DCCH Mode %s, NB_SDU_TO_RX = %u (bytes %u)\n",
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
                len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_SDU_TO_TX = %u (bytes %u)\tNB_SDU_TO_TX_DISC %u (bytes %u)\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_tx_pdcp_sdu,
                             stat_tx_pdcp_bytes,
                             stat_tx_pdcp_sdu_discarded,
                             stat_tx_pdcp_bytes_discarded);
                len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_TX_DATA   = %u (bytes %u)\tNB_TX_CONTROL %u (bytes %u)\tNB_TX_RETX %u (bytes %u)\tNB_TX_RETX_BY_STATUS = %u (bytes %u)\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_tx_data_pdu,
                             stat_tx_data_bytes,
                             stat_tx_control_pdu,
                             stat_tx_control_bytes,
                             stat_tx_retransmit_pdu,
                             stat_tx_retransmit_bytes,
                             stat_tx_retransmit_pdu_by_status,
                             stat_tx_retransmit_bytes_by_status);
                len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_RX_DATA   = %u (bytes %u)\tNB_RX_CONTROL %u (bytes %u)\tNB_RX_DUPL %u (bytes %u)\tNB_RX_DROP = %u (bytes %u)\tNB_RX_OUT_OF_WINDOW = %u (bytes %u)\n",
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
                len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, RX_REODERING_TIMEOUT = %u\tRX_POLL_RET_TIMEOUT %u\tRX_PROHIBIT_TIME_OUT %u\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_timer_reordering_timed_out,
                             stat_timer_poll_retransmit_timed_out,
                             stat_timer_status_prohibit_timed_out);
                len+=sprintf(&buffer[len],"[RLC] DTCH Mode %s, NB_SDU_TO_RX = %u (bytes %u)\n",
                             (stat_rlc_mode==RLC_MODE_AM)? "AM": (stat_rlc_mode==RLC_MODE_UM)?"UM":"NONE",
                             stat_rx_pdcp_sdu,
                             stat_rx_pdcp_bytes);
            }
        }
    }

    return len + 1 /* SR: for trailing \0 */;
}


