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

/*! \file eNB_scheduler_ulsch.c
 * \brief eNB procedures for the ULSCH transport channel
 * \author Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

/* indented with: indent -kr eNB_scheduler_RA.c */

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
#include "intertask_interface.h"
#endif

#include "ENB_APP/flexran_agent_defs.h"
#include "flexran_agent_ran_api.h"
#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "flexran_agent_mac.h"
#include <dlfcn.h>

#include "T.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

extern int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req);
extern void add_subframe(uint16_t *frameP, uint16_t *subframeP, int offset);
extern uint16_t sfnsf_add_subframe(uint16_t frameP, uint16_t subframeP, int offset);
extern int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req);
extern uint8_t nfapi_mode;

extern uint8_t nfapi_mode;

// This table holds the allowable PRB sizes for ULSCH transmissions
uint8_t rb_table[34] =
  { 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 25, 27, 30, 32,
    36, 40, 45, 48, 50, 54, 60, 64, 72, 75, 80, 81, 90, 96, 100
  };

/* number of active slices for  past and current time*/
int n_active_slices_uplink = 1;
int n_active_slices_current_uplink = 1;

/* RB share for each slice for past and current time*/
float avg_slice_percentage_uplink=0.25;
float slice_percentage_uplink[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float slice_percentage_current_uplink[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float total_slice_percentage_uplink = 0;
float total_slice_percentage_current_uplink = 0;

// MAX MCS for each slice for past and current time
int slice_maxmcs_uplink[MAX_NUM_SLICES] = {20, 20, 20, 20};
int slice_maxmcs_current_uplink[MAX_NUM_SLICES] = {20,20,20,20};

/*resource blocks allowed*/
uint16_t         nb_rbs_allowed_slice_uplink[MAX_NUM_CCs][MAX_NUM_SLICES];
/*Slice Update */
int update_ul_scheduler[MAX_NUM_SLICES] = {1, 1, 1, 1};
int update_ul_scheduler_current[MAX_NUM_SLICES] = {1, 1, 1, 1};

/* name of available scheduler*/
char *ul_scheduler_type[MAX_NUM_SLICES] = {"schedule_ulsch_rnti",
					   "schedule_ulsch_rnti",
					   "schedule_ulsch_rnti",
					   "schedule_ulsch_rnti"
};

/* Slice Function Pointer */
slice_scheduler_ul slice_sched_ul[MAX_NUM_SLICES] = {0};

void
rx_sdu(const module_id_t enb_mod_idP,
       const int CC_idP,
       const frame_t frameP,
       const sub_frame_t subframeP,
       const rnti_t rntiP,
       uint8_t * sduP,
       const uint16_t sdu_lenP,
       const uint16_t timing_advance, const uint8_t ul_cqi)
{
  int current_rnti = rntiP;
  unsigned char rx_ces[MAX_NUM_CE], num_ce, num_sdu, i, *payload_ptr;
  unsigned char rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];
  int UE_id = find_UE_id(enb_mod_idP, current_rnti);
  int RA_id;
  int ii, j;
  eNB_MAC_INST *mac = RC.mac[enb_mod_idP];
  int harq_pid =
    subframe2harqpid(&mac->common_channels[CC_idP], frameP, subframeP);
  int lcgid_updated[4] = {0, 0, 0, 0};

  UE_list_t *UE_list = &mac->UE_list;
  int crnti_rx = 0;
  RA_t *ra =
    (RA_t *) & RC.mac[enb_mod_idP]->common_channels[CC_idP].ra[0];
  int first_rb = 0;

  start_meas(&mac->rx_ulsch_sdu);

  if ((UE_id > NUMBER_OF_UE_MAX) || (UE_id == -1))
    for (ii = 0; ii < NB_RB_MAX; ii++) {
      rx_lengths[ii] = 0;
    }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
    (VCD_SIGNAL_DUMPER_FUNCTIONS_RX_SDU, 1);
  if (opt_enabled == 1) {
    trace_pdu(0, sduP, sdu_lenP, 0, 3, current_rnti, frameP, subframeP,
	      0, 0);
    LOG_D(OPT, "[eNB %d][ULSCH] Frame %d  rnti %x  with size %d\n",
	  enb_mod_idP, frameP, current_rnti, sdu_lenP);
  }

  if (UE_id != -1) {
    LOG_D(MAC,
	  "[eNB %d][PUSCH %d] CC_id %d Received ULSCH sdu round %d from PHY (rnti %x, UE_id %d) ul_cqi %d\n",
	  enb_mod_idP, harq_pid, CC_idP,
	  UE_list->UE_sched_ctrl[UE_id].round_UL[CC_idP][harq_pid],
	  current_rnti, UE_id, ul_cqi);

    AssertFatal(UE_list->UE_sched_ctrl[UE_id].
		round_UL[CC_idP][harq_pid] < 8, "round >= 8\n");
    if (sduP != NULL) {
      UE_list->UE_sched_ctrl[UE_id].ul_inactivity_timer = 0;
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
      UE_list->UE_sched_ctrl[UE_id].ul_scheduled &= (~(1 << harq_pid));
      /* Update with smoothing: 3/4 of old value and 1/4 of new.
       * This is the logic that was done in the function
       * lte_est_timing_advance_pusch, maybe it's not necessary?
       * maybe it's even not correct at all?
       */
      UE_list->UE_sched_ctrl[UE_id].ta_update =	(UE_list->UE_sched_ctrl[UE_id].ta_update * 3 + timing_advance) / 4;
      UE_list->UE_sched_ctrl[UE_id].pusch_snr[CC_idP] = ul_cqi;
      UE_list->UE_sched_ctrl[UE_id].ul_consecutive_errors = 0;
      first_rb = UE_list->UE_template[CC_idP][UE_id].first_rb_ul[harq_pid];

      if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync > 0) {
	UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync = 0;
	mac_eNB_rrc_ul_in_sync(enb_mod_idP, CC_idP, frameP,
			       subframeP, UE_RNTI(enb_mod_idP,
						  UE_id));
      }

      /* update scheduled bytes */
      UE_list->UE_template[CC_idP][UE_id].scheduled_ul_bytes -= UE_list->UE_template[CC_idP][UE_id].TBS_UL[harq_pid];
      if (UE_list->UE_template[CC_idP][UE_id].scheduled_ul_bytes < 0)
        UE_list->UE_template[CC_idP][UE_id].scheduled_ul_bytes = 0;
    } else {		// we've got an error
      LOG_D(MAC,
	    "[eNB %d][PUSCH %d] CC_id %d ULSCH in error in round %d, ul_cqi %d\n",
	    enb_mod_idP, harq_pid, CC_idP,
	    UE_list->UE_sched_ctrl[UE_id].round_UL[CC_idP][harq_pid],
	    ul_cqi);

      //      AssertFatal(1==0,"ulsch in error\n");
      if (UE_list->UE_sched_ctrl[UE_id].round_UL[CC_idP][harq_pid] == 3) {
	UE_list->UE_sched_ctrl[UE_id].ul_scheduled &= (~(1 << harq_pid));
	UE_list->UE_sched_ctrl[UE_id].round_UL[CC_idP][harq_pid] = 0;
	if (UE_list->UE_sched_ctrl[UE_id].ul_consecutive_errors++ == 10)
	  UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 1;

        /* update scheduled bytes */
        UE_list->UE_template[CC_idP][UE_id].scheduled_ul_bytes -= UE_list->UE_template[CC_idP][UE_id].TBS_UL[harq_pid];
        if (UE_list->UE_template[CC_idP][UE_id].scheduled_ul_bytes < 0)
          UE_list->UE_template[CC_idP][UE_id].scheduled_ul_bytes = 0;
      } else
	UE_list->UE_sched_ctrl[UE_id].round_UL[CC_idP][harq_pid]++;

      first_rb = UE_list->UE_template[CC_idP][UE_id].first_rb_ul[harq_pid];

      // Program NACK for PHICH
      LOG_D(MAC,
	"Programming PHICH NACK for rnti %x harq_pid %d (first_rb %d)\n",
	current_rnti, harq_pid, first_rb);
      nfapi_hi_dci0_request_t *hi_dci0_req = &mac->HI_DCI0_req[CC_idP];
      nfapi_hi_dci0_request_body_t *hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
      nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu =
        &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
      memset((void *) hi_dci0_pdu, 0, sizeof(nfapi_hi_dci0_request_pdu_t));
      hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_HI_PDU_TYPE;
      hi_dci0_pdu->pdu_size = 2 + sizeof(nfapi_hi_dci0_hi_pdu);
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start = first_rb;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = 0;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value = 0;
      hi_dci0_req_body->number_of_hi++;
      hi_dci0_req_body->sfnsf = sfnsf_add_subframe(frameP,subframeP, 0);
      hi_dci0_req_body->tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
      hi_dci0_req->sfn_sf = sfnsf_add_subframe(frameP,subframeP, 4);
      hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;

      return;

    }
  } else if ((RA_id = find_RA_id(enb_mod_idP, CC_idP, current_rnti)) != -1) {	// Check if this is an RA process for the rnti
    AssertFatal(mac->common_channels[CC_idP].
		radioResourceConfigCommon->rach_ConfigCommon.
		maxHARQ_Msg3Tx > 1,
		"maxHARQ %d should be greater than 1\n",
		(int) mac->common_channels[CC_idP].
		radioResourceConfigCommon->rach_ConfigCommon.
		maxHARQ_Msg3Tx);

    LOG_D(MAC,
	  "[eNB %d][PUSCH %d] CC_id %d [RAPROC Msg3] Received ULSCH sdu round %d from PHY (rnti %x, RA_id %d) ul_cqi %d\n",
	  enb_mod_idP, harq_pid, CC_idP, ra[RA_id].msg3_round,
	  current_rnti, RA_id, ul_cqi);

    first_rb = ra->msg3_first_rb;

    if (sduP == NULL) {	// we've got an error on Msg3
      LOG_D(MAC,
	    "[eNB %d] CC_id %d, RA %d ULSCH in error in round %d/%d\n",
	    enb_mod_idP, CC_idP, RA_id,
	    ra[RA_id].msg3_round,
	    (int) mac->common_channels[CC_idP].
	    radioResourceConfigCommon->rach_ConfigCommon.
	    maxHARQ_Msg3Tx);
      if (ra[RA_id].msg3_round == mac->common_channels[CC_idP].radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx - 1) {
	cancel_ra_proc(enb_mod_idP, CC_idP, frameP, current_rnti);
      }

      else {
	first_rb = UE_list->UE_template[CC_idP][UE_id].first_rb_ul[harq_pid];
	ra[RA_id].msg3_round++;
	// prepare handling of retransmission
	ra[RA_id].Msg3_frame    = (ra[RA_id].Msg3_frame + ((ra[RA_id].Msg3_subframe > 1) ? 1 : 0)) % 1024;
	ra[RA_id].Msg3_subframe = (ra[RA_id].Msg3_subframe + 8) % 10;
	add_msg3(enb_mod_idP, CC_idP, &ra[RA_id], frameP, subframeP);
      }

      /* TODO: program NACK for PHICH? */

      return;
    }
  } else {
    LOG_W(MAC,
	  "Cannot find UE or RA corresponding to ULSCH rnti %x, dropping it\n",
	  current_rnti);
    return;
  }
  payload_ptr = parse_ulsch_header(sduP, &num_ce, &num_sdu, rx_ces, rx_lcids, rx_lengths, sdu_lenP);

  T(T_ENB_MAC_UE_UL_PDU, T_INT(enb_mod_idP), T_INT(CC_idP),
    T_INT(current_rnti), T_INT(frameP), T_INT(subframeP),
    T_INT(harq_pid), T_INT(sdu_lenP), T_INT(num_ce), T_INT(num_sdu));
  T(T_ENB_MAC_UE_UL_PDU_WITH_DATA, T_INT(enb_mod_idP), T_INT(CC_idP),
    T_INT(current_rnti), T_INT(frameP), T_INT(subframeP),
    T_INT(harq_pid), T_INT(sdu_lenP), T_INT(num_ce), T_INT(num_sdu),
    T_BUFFER(sduP, sdu_lenP));

  mac->eNB_stats[CC_idP].ulsch_bytes_rx = sdu_lenP;
  mac->eNB_stats[CC_idP].total_ulsch_bytes_rx += sdu_lenP;
  mac->eNB_stats[CC_idP].total_ulsch_pdus_rx += 1;

  UE_list->UE_sched_ctrl[UE_id].round_UL[CC_idP][harq_pid] = 0;

  // control element
  for (i = 0; i < num_ce; i++) {

    T(T_ENB_MAC_UE_UL_CE, T_INT(enb_mod_idP), T_INT(CC_idP),
      T_INT(current_rnti), T_INT(frameP), T_INT(subframeP),
      T_INT(rx_ces[i]));

    switch (rx_ces[i]) {	// implement and process BSR + CRNTI +
    case POWER_HEADROOM:
      if (UE_id != -1) {
	UE_list->UE_template[CC_idP][UE_id].phr_info =
	  (payload_ptr[0] & 0x3f) - PHR_MAPPING_OFFSET;
	LOG_D(MAC,
	      "[eNB %d] CC_id %d MAC CE_LCID %d : Received PHR PH = %d (db)\n",
	      enb_mod_idP, CC_idP, rx_ces[i],
	      UE_list->UE_template[CC_idP][UE_id].phr_info);
	UE_list->UE_template[CC_idP][UE_id].phr_info_configured =
	  1;
	UE_list->UE_sched_ctrl[UE_id].phr_received = 1;
      }
      payload_ptr += sizeof(POWER_HEADROOM_CMD);
      break;

    case CRNTI:
      {
	int old_rnti =
	  (((uint16_t) payload_ptr[0]) << 8) + payload_ptr[1];
	int old_UE_id = find_UE_id(enb_mod_idP, old_rnti);
	LOG_D(MAC,
	      "[eNB %d] Frame %d, Subframe %d CC_id %d MAC CE_LCID %d (ce %d/%d): CRNTI %x (UE_id %d) in Msg3\n",
	      enb_mod_idP, frameP, subframeP, CC_idP, rx_ces[i], i,
	      num_ce, old_rnti, old_UE_id);
	/* receiving CRNTI means that the current rnti has to go away */
	cancel_ra_proc(enb_mod_idP, CC_idP, frameP,
		       current_rnti);
	if (old_UE_id != -1) {
	  /* TODO: if the UE did random access (followed by a MAC uplink with
	   * CRNTI) because none of its scheduling request was granted, then
	   * according to 36.321 5.4.4 the UE's MAC will notify RRC to release
	   * PUCCH/SRS. According to 36.331 5.3.13 the UE will then apply
	   * default configuration for CQI reporting and scheduling requests,
	   * which basically means that the CQI requests won't work anymore and
	   * that the UE won't do any scheduling request anymore as long as the
	   * eNB doesn't reconfigure the UE.
	   * We have to take care of this. As the code is, nothing is done and
	   * the UE state in the eNB is wrong.
	   */
	  UE_id = old_UE_id;
	  UE_list->UE_sched_ctrl[UE_id].ul_inactivity_timer = 0;
	  UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
	  if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync > 0) {
	    UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync = 0;
	    mac_eNB_rrc_ul_in_sync(enb_mod_idP, CC_idP, frameP,
				   subframeP, old_rnti);
	  }
	  current_rnti = old_rnti;
	}
	crnti_rx = 1;
	payload_ptr += 2;
	break;
      }

    case TRUNCATED_BSR:
    case SHORT_BSR:
      {
	uint8_t lcgid;
	lcgid = (payload_ptr[0] >> 6);

	LOG_D(MAC,
	      "[eNB %d] CC_id %d MAC CE_LCID %d : Received short BSR LCGID = %u bsr = %d\n",
	      enb_mod_idP, CC_idP, rx_ces[i], lcgid,
	      payload_ptr[0] & 0x3f);

	if (crnti_rx == 1)
	  LOG_D(MAC,
		"[eNB %d] CC_id %d MAC CE_LCID %d : Received short BSR LCGID = %u bsr = %d\n",
		enb_mod_idP, CC_idP, rx_ces[i], lcgid,
		payload_ptr[0] & 0x3f);
	if (UE_id != -1) {
          int bsr = payload_ptr[0] & 0x3f;

          lcgid_updated[lcgid] = 1;

          // update buffer info
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[lcgid] = BSR_TABLE[bsr];

          UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer =
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[0] +
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[1] +
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[2] +
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[3];
          //UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer += UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer / 4;

	  RC.eNB[enb_mod_idP][CC_idP]->pusch_stats_bsr[UE_id][(frameP * 10) + subframeP] = (payload_ptr[0] & 0x3f);
	  if (UE_id == UE_list->head)
	    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR,
						    RC.eNB[enb_mod_idP][CC_idP]->pusch_stats_bsr
						    [UE_id][(frameP * 10) + subframeP]);
	  if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[lcgid] == 0) {
	    UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[lcgid] = frameP;
	  }
	  if (mac_eNB_get_rrc_status(enb_mod_idP,UE_RNTI(enb_mod_idP, UE_id)) < RRC_CONNECTED)
	    LOG_D(MAC,
		  "[eNB %d] CC_id %d MAC CE_LCID %d : estimated_ul_buffer = %d (lcg increment %d)\n",
		  enb_mod_idP, CC_idP, rx_ces[i],
		  UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer,
		  UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[lcgid]);
	} else {

	}
	payload_ptr += 1;	//sizeof(SHORT_BSR); // fixme
      }
      break;

    case LONG_BSR:
      if (UE_id != -1) {
        int bsr0 = (payload_ptr[0] & 0xFC) >> 2;
        int bsr1 = ((payload_ptr[0] & 0x03) << 4) | ((payload_ptr[1] & 0xF0) >> 4);
        int bsr2 = ((payload_ptr[1] & 0x0F) << 2) | ((payload_ptr[2] & 0xC0) >> 6);
        int bsr3 = payload_ptr[2] & 0x3F;

        lcgid_updated[0] = 1;
        lcgid_updated[1] = 1;
        lcgid_updated[2] = 1;
        lcgid_updated[3] = 1;

        // update buffer info
        UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0] = BSR_TABLE[bsr0];
        UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1] = BSR_TABLE[bsr1];
        UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2] = BSR_TABLE[bsr2];
        UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3] = BSR_TABLE[bsr3];

        UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer =
            UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[0] +
            UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[1] +
            UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[2] +
            UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[3];
        //UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer += UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer / 4;

        LOG_D(MAC,
              "[eNB %d] CC_id %d MAC CE_LCID %d: Received long BSR. Size is LCGID0 = %u LCGID1 = "
              "%u LCGID2 = %u LCGID3 = %u\n", enb_mod_idP, CC_idP,
              rx_ces[i],
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0],
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1],
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2],
              UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3]);
        if (crnti_rx == 1)
          LOG_D(MAC,
                "[eNB %d] CC_id %d MAC CE_LCID %d: Received long BSR. Size is LCGID0 = %u LCGID1 = "
                "%u LCGID2 = %u LCGID3 = %u\n", enb_mod_idP,
                CC_idP, rx_ces[i],
                UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0],
                UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1],
                UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2],
                UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3]);

	if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID0] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID0] = 0;
	} else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID0] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID0] = frameP;
	}

	if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID1] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID1] = 0;
	} else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID1] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID1] = frameP;
	}

	if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID2] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID2] = 0;
	} else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID2] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID2] = frameP;
	}

	if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[LCGID3] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID3] = 0;
	} else if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID3] == 0) {
	  UE_list->UE_template[CC_idP][UE_id].ul_buffer_creation_time[LCGID3] = frameP;

	}
      }

      payload_ptr += 3;	////sizeof(LONG_BSR);
      break;

    default:
      LOG_E(MAC,
	    "[eNB %d] CC_id %d Received unknown MAC header (0x%02x)\n",
	    enb_mod_idP, CC_idP, rx_ces[i]);
      break;
    }
  }

  for (i = 0; i < num_sdu; i++) {
    LOG_D(MAC, "SDU Number %d MAC Subheader SDU_LCID %d, length %d\n",
	  i, rx_lcids[i], rx_lengths[i]);

    T(T_ENB_MAC_UE_UL_SDU, T_INT(enb_mod_idP), T_INT(CC_idP),
      T_INT(current_rnti), T_INT(frameP), T_INT(subframeP),
      T_INT(rx_lcids[i]), T_INT(rx_lengths[i]));
    T(T_ENB_MAC_UE_UL_SDU_WITH_DATA, T_INT(enb_mod_idP), T_INT(CC_idP),
      T_INT(current_rnti), T_INT(frameP), T_INT(subframeP),
      T_INT(rx_lcids[i]), T_INT(rx_lengths[i]), T_BUFFER(payload_ptr,
							 rx_lengths
							 [i]));

    switch (rx_lcids[i]) {
    case CCCH:
      if (rx_lengths[i] > CCCH_PAYLOAD_SIZE_MAX) {
	LOG_E(MAC,
	      "[eNB %d/%d] frame %d received CCCH of size %d (too big, maximum allowed is %d, sdu_len %d), dropping packet\n",
	      enb_mod_idP, CC_idP, frameP, rx_lengths[i],
	      CCCH_PAYLOAD_SIZE_MAX, sdu_lenP);
	break;
      }
      LOG_D(MAC,
	    "[eNB %d][RAPROC] CC_id %d Frame %d, Received CCCH:  %x.%x.%x.%x.%x.%x, Terminating RA procedure for UE rnti %x\n",
	    enb_mod_idP, CC_idP, frameP, payload_ptr[0],
	    payload_ptr[1], payload_ptr[2], payload_ptr[3],
	    payload_ptr[4], payload_ptr[5], current_rnti);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TERMINATE_RA_PROC, 1);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TERMINATE_RA_PROC, 0);
      for (ii = 0; ii < NB_RA_PROC_MAX; ii++) {
	RA_t *ra = &mac->common_channels[CC_idP].ra[ii];

	LOG_D(MAC,
	      "[mac %d][RAPROC] CC_id %d Checking proc %d : rnti (%x, %x), state %d\n",
	      enb_mod_idP, CC_idP, ii, ra->rnti,
	      current_rnti, ra->state);

	if ((ra->rnti == current_rnti) && (ra->state != IDLE)) {

	  //payload_ptr = parse_ulsch_header(msg3,&num_ce,&num_sdu,rx_ces,rx_lcids,rx_lengths,msg3_len);

	  if (UE_id < 0) {
	    memcpy(&ra->cont_res_id[0], payload_ptr, 6);
	    LOG_D(MAC,
		  "[eNB %d][RAPROC] CC_id %d Frame %d CCCH: Received Msg3: length %d, offset %ld\n",
		  enb_mod_idP, CC_idP, frameP, rx_lengths[i],
		  payload_ptr - sduP);

	    if ((UE_id = add_new_ue(enb_mod_idP, CC_idP,
				    mac->common_channels[CC_idP].
				    ra[ii].rnti, harq_pid
#ifdef Rel14
				    ,
				    mac->common_channels[CC_idP].
				    ra[ii].rach_resource_type
#endif
				    )) == -1) {
	      AssertFatal(1 == 0,
			  "[MAC][eNB] Max user count reached\n");
	      // kill RA procedure
	    } else
	      LOG_D(MAC,
		    "[eNB %d][RAPROC] CC_id %d Frame %d Added user with rnti %x => UE %d\n",
		    enb_mod_idP, CC_idP, frameP, ra->rnti,
		    UE_id);
	  } else {
	    LOG_D(MAC,
		  "[eNB %d][RAPROC] CC_id %d Frame %d CCCH: Received Msg3 from already registered UE %d: length %d, offset %ld\n",
		  enb_mod_idP, CC_idP, frameP, UE_id,
		  rx_lengths[i], payload_ptr - sduP);
	    // kill RA procedure
	  }

	  mac_rrc_data_ind(enb_mod_idP,
			   CC_idP,
			   frameP, subframeP,
			   current_rnti,
			   CCCH,
			   (uint8_t *) payload_ptr,
			   rx_lengths[i],
			   0);


	  if (num_ce > 0) {	// handle msg3 which is not RRCConnectionRequest
	    //  process_ra_message(msg3,num_ce,rx_lcids,rx_ces);
	  }
	  // prepare transmission of Msg4
	  ra->state = MSG4;



	  // Program Msg4 PDCCH+DLSCH/MPDCCH transmission 4 subframes from now, // Check if this is ok for BL/CE, or if the rule is different
	  ra->Msg4_frame = frameP + ((subframeP > 5) ? 1 : 0);
	  ra->Msg4_subframe = (subframeP + 4) % 10;

	}		// if process is active
      }			// loop on RA processes

      break;

    case DCCH:
    case DCCH1:
      //      if(eNB_mac_inst[module_idP][CC_idP].Dcch_lchan[UE_id].Active==1){


#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      LOG_T(MAC, "offset: %d\n",
	    (unsigned char) ((unsigned char *) payload_ptr - sduP));
      for (j = 0; j < 32; j++) {
	LOG_T(MAC, "%x ", payload_ptr[j]);
      }
      LOG_T(MAC, "\n");
#endif

      if (UE_id != -1) {
        if (lcgid_updated[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] == 0) {
	  // adjust buffer occupancy of the correponding logical channel group
	  if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] >= rx_lengths[i])
	    UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] -= rx_lengths[i];
	  else
	    UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] = 0;

          UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer =
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[0] +
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[1] +
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[2] +
          UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[3];
          //UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer += UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer / 4;
        }

	LOG_D(MAC,
	      "[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DCCH, received %d bytes form UE %d on LCID %d \n",
	      enb_mod_idP, CC_idP, frameP, rx_lengths[i], UE_id,
	      rx_lcids[i]);

	mac_rlc_data_ind(enb_mod_idP, current_rnti, enb_mod_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, rx_lcids[i], (char *) payload_ptr, rx_lengths[i], 1, NULL);	//(unsigned int*)crc_status);
	UE_list->eNB_UE_stats[CC_idP][UE_id].num_pdu_rx[rx_lcids[i]] += 1;
	UE_list->eNB_UE_stats[CC_idP][UE_id].num_bytes_rx[rx_lcids[i]] += rx_lengths[i];


      }

      /* UE_id != -1 */
      // }
      break;

      // all the DRBS
    case DTCH:
    default:

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      LOG_T(MAC, "offset: %d\n",
	    (unsigned char) ((unsigned char *) payload_ptr - sduP));
      for (j = 0; j < 32; j++) {
	LOG_T(MAC, "%x ", payload_ptr[j]);
      }
      LOG_T(MAC, "\n");
#endif
      if (rx_lcids[i] < NB_RB_MAX) {
	LOG_D(MAC,
	      "[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DTCH, received %d bytes from UE %d for lcid %d\n",
	      enb_mod_idP, CC_idP, frameP, rx_lengths[i], UE_id,
	      rx_lcids[i]);

	if (UE_id != -1) {
	  // adjust buffer occupancy of the correponding logical channel group
	  LOG_D(MAC,
		"[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DTCH, received %d bytes from UE %d for lcid %d, removing from LCGID %ld, %d\n",
		enb_mod_idP, CC_idP, frameP, rx_lengths[i],
		UE_id, rx_lcids[i],
		UE_list->UE_template[CC_idP][UE_id].
		lcgidmap[rx_lcids[i]],
		UE_list->UE_template[CC_idP][UE_id].
		ul_buffer_info[UE_list->UE_template[CC_idP]
			       [UE_id].lcgidmap[rx_lcids[i]]]);

          if (lcgid_updated[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] == 0) {
	    if (UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] >= rx_lengths[i])
	      UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] -= rx_lengths[i];
	    else
	      UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[UE_list->UE_template[CC_idP][UE_id].lcgidmap[rx_lcids[i]]] = 0;

            UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer =
               UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[0] +
               UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[1] +
               UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[2] +
               UE_list->UE_template[CC_idP][UE_id].ul_buffer_info[3];
            //UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer += UE_list->UE_template[CC_idP][UE_id].estimated_ul_buffer / 4;
          }

	  if ((rx_lengths[i] < SCH_PAYLOAD_SIZE_MAX) && (rx_lengths[i] > 0)) {	// MAX SIZE OF transport block
	    mac_rlc_data_ind(enb_mod_idP, current_rnti, enb_mod_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, rx_lcids[i], (char *) payload_ptr, rx_lengths[i], 1, NULL);	//(unsigned int*)crc_status);

	    UE_list->eNB_UE_stats[CC_idP][UE_id].num_pdu_rx[rx_lcids[i]] += 1;
	    UE_list->eNB_UE_stats[CC_idP][UE_id].num_bytes_rx[rx_lcids[i]] += rx_lengths[i];
	    //clear uplane_inactivity_timer
	    UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer = 0;
	  } else {	/* rx_length[i] */
	    UE_list->eNB_UE_stats[CC_idP][UE_id].num_errors_rx += 1;
	    LOG_E(MAC,
		  "[eNB %d] CC_id %d Frame %d : Max size of transport block reached LCID %d from UE %d ",
		  enb_mod_idP, CC_idP, frameP, rx_lcids[i],
		  UE_id);
	  }
	} else {	/*(UE_id != -1 */
	  LOG_E(MAC,
		"[eNB %d] CC_id %d Frame %d : received unsupported or unknown LCID %d from UE %d ",
		enb_mod_idP, CC_idP, frameP, rx_lcids[i], UE_id);
	}
      }

      break;
    }

    payload_ptr += rx_lengths[i];
  }

  // Program ACK for PHICH
  LOG_D(MAC,
	"Programming PHICH ACK for rnti %x harq_pid %d (first_rb %d)\n",
	current_rnti, harq_pid, first_rb);
  nfapi_hi_dci0_request_t *hi_dci0_req = &mac->HI_DCI0_req[CC_idP];
  nfapi_hi_dci0_request_body_t *hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
  nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu =
    &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
  memset((void *) hi_dci0_pdu, 0, sizeof(nfapi_hi_dci0_request_pdu_t));
  hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_HI_PDU_TYPE;
  hi_dci0_pdu->pdu_size = 2 + sizeof(nfapi_hi_dci0_hi_pdu);
  hi_dci0_pdu->hi_pdu.hi_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
  hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start = first_rb;
  hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = 0;
  hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value = 1;
  hi_dci0_req_body->number_of_hi++;
  hi_dci0_req_body->sfnsf = sfnsf_add_subframe(frameP,subframeP, 0);
  hi_dci0_req_body->tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
  hi_dci0_req->sfn_sf = sfnsf_add_subframe(frameP,subframeP, 4);
  hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;

  /* NN--> FK: we could either check the payload, or use a phy helper to detect a false msg3 */
  if ((num_sdu == 0) && (num_ce == 0)) {
    if (UE_id != -1)
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_num_errors_rx += 1;
    /*
      if (msg3_flagP != NULL) {
      if( *msg3_flagP == 1 ) {
      LOG_N(MAC,"[eNB %d] CC_id %d frame %d : false msg3 detection: signal phy to canceling RA and remove the UE\n", enb_mod_idP, CC_idP, frameP);
      *msg3_flagP=0;
      }
      } */
  } else {
    if (UE_id != -1) {
      UE_list->eNB_UE_stats[CC_idP][UE_id].pdu_bytes_rx        = sdu_lenP;
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_pdu_bytes_rx += sdu_lenP;
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_num_pdus_rx  += 1;
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_SDU, 0);
  stop_meas(&mac->rx_ulsch_sdu);
}

uint32_t bytes_to_bsr_index(int32_t nbytes)
{
  uint32_t i = 0;

  if (nbytes < 0) {
    return (0);
  }

  while ((i < BSR_TABLE_SIZE) && (BSR_TABLE[i] <= nbytes)) {
    i++;
  }

  return (i - 1);
}

void
add_ue_ulsch_info(module_id_t module_idP, int CC_id, int UE_id,
		  sub_frame_t subframeP, UE_ULSCH_STATUS status)
{
  eNB_ulsch_info[module_idP][CC_id][UE_id].rnti     = UE_RNTI(module_idP, UE_id);
  eNB_ulsch_info[module_idP][CC_id][UE_id].subframe = subframeP;
  eNB_ulsch_info[module_idP][CC_id][UE_id].status   = status;

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
  unsigned char not_done = 1, num_ces = 0, num_sdus =
    0, lcid, num_sdu_cnt;
  unsigned char *mac_header_ptr = mac_header;
  unsigned short length, ce_len = 0;

  while (not_done == 1) {

    if (((SCH_SUBHEADER_FIXED *) mac_header_ptr)->E == 0) {
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->LCID;

    if (lcid < EXTENDED_POWER_HEADROOM) {
      if (not_done == 0) {	// last MAC SDU, length is implicit
	mac_header_ptr++;
	length = tb_length - (mac_header_ptr - mac_header) - ce_len;

	for (num_sdu_cnt = 0; num_sdu_cnt < num_sdus;
	     num_sdu_cnt++) {
	  length -= rx_lengths[num_sdu_cnt];
	}
      } else {
	if (((SCH_SUBHEADER_SHORT *) mac_header_ptr)->F == 0) {
	  length = ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L;
	  mac_header_ptr += 2;	//sizeof(SCH_SUBHEADER_SHORT);
	} else {	// F = 1
	  length =
	    ((((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_MSB &
	      0x7f) << 8) | (((SCH_SUBHEADER_LONG *)
			      mac_header_ptr)->L_LSB & 0xff);
	  mac_header_ptr += 3;	//sizeof(SCH_SUBHEADER_LONG);
	}
      }

      LOG_D(MAC,
	    "[eNB] sdu %d lcid %d tb_length %d length %d (offset now %ld)\n",
	    num_sdus, lcid, tb_length, length,
	    mac_header_ptr - mac_header);
      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else {		// This is a control element subheader POWER_HEADROOM, BSR and CRNTI
      if (lcid == SHORT_PADDING) {
	mac_header_ptr++;
      } else {
	rx_ces[num_ces] = lcid;
	num_ces++;
	mac_header_ptr++;

	if (lcid == LONG_BSR) {
	  ce_len += 3;
	} else if (lcid == CRNTI) {
	  ce_len += 2;
	} else if ((lcid == POWER_HEADROOM)
		   || (lcid == TRUNCATED_BSR)
		   || (lcid == SHORT_BSR)) {
	  ce_len++;
	} else {
	  LOG_E(MAC, "unknown CE %d \n", lcid);
	  AssertFatal(1 == 0, "unknown CE");
	}
      }
    }
  }

  *num_ce = num_ces;
  *num_sdu = num_sdus;

  return (mac_header_ptr);
}

/* This function is called by PHY layer when it schedules some
 * uplink for a random access message 3.
 * The MAC scheduler has to skip the RBs used by this message 3
 * (done below in schedule_ulsch).
 */
void
set_msg3_subframe(module_id_t mod_id,
		  int CC_id,
		  int frame,
		  int subframe, int rnti, int Msg3_frame,
		  int Msg3_subframe)
{
  eNB_MAC_INST *mac = RC.mac[mod_id];
  int i;
  for (i = 0; i < NB_RA_PROC_MAX; i++) {
    if (mac->common_channels[CC_id].ra[i].state != IDLE &&
	mac->common_channels[CC_id].ra[i].rnti == rnti) {
      mac->common_channels[CC_id].ra[i].Msg3_subframe =
	Msg3_subframe;
      break;
    }
  }
}

void
schedule_ulsch(module_id_t module_idP, frame_t frameP,
	       sub_frame_t subframeP)
{
  uint16_t first_rb[MAX_NUM_CCs], i;
  int CC_id;
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc;

  start_meas(&mac->schedule_ulsch);

  int sched_subframe = (subframeP + 4) % 10;

  cc = &mac->common_channels[0];
  int tdd_sfa;
  // for TDD: check subframes where we have to act and return if nothing should be done now
  if (cc->tdd_Config) {
    tdd_sfa = cc->tdd_Config->subframeAssignment;
    switch (subframeP) {
    case 0:
      if ((tdd_sfa == 0) || (tdd_sfa == 3) || (tdd_sfa == 6))
	sched_subframe = 4;
      else
	return;
      break;
    case 1:
      if ((tdd_sfa == 0) || (tdd_sfa == 1))
	sched_subframe = 7;
      else if (tdd_sfa == 6)
	sched_subframe = 8;
      break;
    default:
      return;

    case 2:		// Don't schedule UL in subframe 2 for TDD
      return;
    case 3:
      if (tdd_sfa == 2)
	sched_subframe = 7;
      else
	return;
      break;
    case 4:
      if (tdd_sfa == 1)
	sched_subframe = 8;
      else
	return;
      break;
    case 5:
      if (tdd_sfa == 0)
	sched_subframe = 9;
      else if (tdd_sfa == 6)
	sched_subframe = 3;
      else
	return;
      break;
    case 6:
      if (tdd_sfa == 1)
	sched_subframe = 2;
      else if (tdd_sfa == 6)
	sched_subframe = 3;
      else
	return;
      break;
    case 7:
      return;
    case 8:
      if ((tdd_sfa >= 2) || (tdd_sfa <= 5))
	sched_subframe = 2;
      else
	return;
      break;
    case 9:
      if ((tdd_sfa == 1) || (tdd_sfa == 3) || (tdd_sfa == 4))
	sched_subframe = 3;
      else if (tdd_sfa == 6)
	sched_subframe = 4;
      else
	return;
      break;
    }
  }
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {



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

    for (i = 0; i < NB_RA_PROC_MAX; i++) {
      if ((cc->ra[i].state == WAITMSG3) &&
	  (cc->ra[i].Msg3_subframe == sched_subframe)) {
	first_rb[CC_id]++;
	//    cc->ray[i].Msg3_subframe = -1;
	break;
      }
    }
  }

  // perform slice-specifc operations

  total_slice_percentage_uplink=0;
  avg_slice_percentage_uplink=1.0/n_active_slices_uplink;

  // reset the slice percentage for inactive slices
  for (i = n_active_slices_uplink; i< MAX_NUM_SLICES; i++) {
    slice_percentage_uplink[i]=0;
  }
  for (i = 0; i < n_active_slices_uplink; i++) {
    if (slice_percentage_uplink[i] < 0 ){
      LOG_W(MAC, "[eNB %d] frame %d subframe %d:invalid slice %d percentage %f. resetting to zero",
            module_idP, frameP, subframeP, i, slice_percentage_uplink[i]);
      slice_percentage_uplink[i]=0;
    }
    total_slice_percentage_uplink+=slice_percentage_uplink[i];
  }

  for (i = 0; i < n_active_slices_uplink; i++) {

    // Load any updated functions
    if (update_ul_scheduler[i] > 0 ) {
      slice_sched_ul[i] = dlsym(NULL, ul_scheduler_type[i]);
      update_ul_scheduler[i] = 0;
      update_ul_scheduler_current[i] = 0;
      //slice_percentage_current_uplink[i]= slice_percentage_uplink[i];
      //total_slice_percentage_current_uplink+=slice_percentage_uplink[i];
      //if (total_slice_percentage_current_uplink> 1)
      //total_slice_percentage_current_uplink=1;
      LOG_N(MAC,"update ul scheduler slice %d\n", i);
    }
    // the new total RB share is within the range
    if (total_slice_percentage_uplink <= 1.0){

      // check if the number of slices has changed, and log
      if (n_active_slices_current_uplink != n_active_slices_uplink ){
        if ((n_active_slices_uplink > 0) && (n_active_slices_uplink <= MAX_NUM_SLICES)) {
          LOG_N(MAC,"[eNB %d]frame %d subframe %d: number of active UL slices has changed: %d-->%d\n",
                module_idP, frameP, subframeP, n_active_slices_current_uplink, n_active_slices_uplink);
          n_active_slices_current_uplink = n_active_slices_uplink;
        } else {
          LOG_W(MAC,"invalid number of UL slices %d, revert to the previous value %d\n",
                n_active_slices_uplink, n_active_slices_current_uplink);
          n_active_slices_uplink = n_active_slices_current_uplink;
        }
      }

      // check if the slice rb share has changed, and log the console
      if (slice_percentage_current_uplink[i] != slice_percentage_uplink[i]){
        LOG_N(MAC,"[eNB %d][SLICE %d][UL] frame %d subframe %d: total percentage %f-->%f, slice RB percentage has changed: %f-->%f\n",
              module_idP, i, frameP, subframeP, total_slice_percentage_current_uplink,
              total_slice_percentage_uplink, slice_percentage_current_uplink[i], slice_percentage_uplink[i]);
        total_slice_percentage_current_uplink = total_slice_percentage_uplink;
        slice_percentage_current_uplink[i] = slice_percentage_uplink[i];
      }

      // check if the slice max MCS, and log the console
      if (slice_maxmcs_current_uplink[i] != slice_maxmcs_uplink[i]){
        if ((slice_maxmcs_uplink[i] >= 0) && (slice_maxmcs_uplink[i] <= 16)){
          LOG_N(MAC,"[eNB %d][SLICE %d][UL] frame %d subframe %d: slice MAX MCS has changed: %d-->%d\n",
                module_idP, i, frameP, subframeP, slice_maxmcs_current_uplink[i], slice_maxmcs_uplink[i]);
          slice_maxmcs_current_uplink[i] = slice_maxmcs_uplink[i];
        } else {
          LOG_W(MAC,"[eNB %d][SLICE %d][UL] invalid slice max mcs %d, revert the previous value %d\n",
                module_idP, i, slice_maxmcs_uplink[i],slice_maxmcs_current_uplink[i]);
          slice_maxmcs_uplink[i] = slice_maxmcs_current_uplink[i];
        }
      }

      // check if a new scheduler, and log the console
      if (update_ul_scheduler_current[i] != update_ul_scheduler[i]){
        LOG_N(MAC,"[eNB %d][SLICE %d][UL] frame %d subframe %d: UL scheduler for this slice is updated: %s \n",
              module_idP, i, frameP, subframeP, ul_scheduler_type[i]);
        update_ul_scheduler_current[i] = update_ul_scheduler[i];
      }
    } else {
      if (n_active_slices_uplink == n_active_slices_current_uplink) {
        LOG_W(MAC,"[eNB %d][SLICE %d][UL] invalid total RB share (%f->%f), reduce proportionally the RB share by 0.1\n",
              module_idP, i, total_slice_percentage_current_uplink, total_slice_percentage_uplink);
        if (slice_percentage_uplink[i] > avg_slice_percentage_uplink) {
          slice_percentage_uplink[i] -= 0.1;
          total_slice_percentage_uplink -= 0.1;
        }
      } else {
        // here we can correct the values, e.g. reduce proportionally
        LOG_W(MAC,"[eNB %d][SLICE %d][UL] invalid total RB share (%f->%f), revert the  number of slice to its previous value (%d->%d)\n",
              module_idP, i, total_slice_percentage_current_uplink,
              total_slice_percentage_uplink, n_active_slices_uplink,
              n_active_slices_current_uplink);
        n_active_slices_uplink = n_active_slices_current_uplink;
        slice_percentage_uplink[i] = slice_percentage_current_uplink[i];
      }
    }

    // Run each enabled slice-specific schedulers one by one
    slice_sched_ul[i](module_idP, i, frameP, subframeP, sched_subframe, first_rb);
  }

  stop_meas(&mac->schedule_ulsch);
}

void
schedule_ulsch_rnti(module_id_t module_idP,
					slice_id_t slice_id,
		    frame_t frameP,
		    sub_frame_t subframeP,
		    unsigned char sched_subframeP, uint16_t * first_rb)
{
  int UE_id;
  uint8_t aggregation = 2;
  rnti_t rnti = -1;
  uint8_t round = 0;
  uint8_t harq_pid = 0;
  uint8_t status = 0;
  uint8_t rb_table_index = -1;
  uint32_t cqi_req, cshift, ndi, tpc;
  int32_t normalized_rx_power;
  int32_t target_rx_power = -90;
  static int32_t tpc_accumulated = 0;
  int n;
  int CC_id = 0;
  int drop_ue = 0;
  int N_RB_UL;
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  UE_list_t *UE_list = &mac->UE_list;
  UE_TEMPLATE *UE_template;
  UE_sched_ctrl *UE_sched_ctrl;
  int sched_frame = frameP;
  int rvidx_tab[4] = { 0, 2, 3, 1 };

  if (sched_subframeP < subframeP)
      sched_frame++;

  nfapi_hi_dci0_request_t        *hi_dci0_req = &mac->HI_DCI0_req[CC_id];
  nfapi_hi_dci0_request_body_t   *hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;

  nfapi_ul_config_request_t *ul_req_tmp            = &mac->UL_req_tmp[CC_id][sched_subframeP];
  nfapi_ul_config_request_body_t *ul_req_tmp_body  = &ul_req_tmp->ul_config_request_body;

  //LOG_D(MAC, "entering ulsch preprocesor\n");
  ulsch_scheduler_pre_processor(module_idP, slice_id, frameP, subframeP, first_rb);

  //LOG_D(MAC, "exiting ulsch preprocesor\n");

  hi_dci0_req->sfn_sf = (frameP << 4) + subframeP;

  // loop over all active UEs
  for (UE_id = UE_list->head_ul; UE_id >= 0;
       UE_id = UE_list->next_ul[UE_id]) {

    if (!ue_slice_membership(UE_id, slice_id))
        continue;

    // don't schedule if Msg4 is not received yet
    if (UE_list->UE_template[UE_PCCID(module_idP, UE_id)][UE_id].
        configured == FALSE) {
        LOG_D(MAC,
              "[eNB %d] frame %d subfarme %d, UE %d: not configured, skipping UE scheduling \n",
              module_idP, frameP, subframeP, UE_id);
        continue;
    }

    rnti = UE_RNTI(module_idP, UE_id);

    if (rnti == NOT_A_RNTI) {
      LOG_W(MAC, "[eNB %d] frame %d subfarme %d, UE %d: no RNTI \n",
	    module_idP, frameP, subframeP, UE_id);
      continue;
    }

    drop_ue = 0;
    /* let's drop the UE if get_eNB_UE_stats returns NULL when calling it with any of the UE's active UL CCs */
    /* TODO: refine?

       for (n=0; n<UE_list->numactiveULCCs[UE_id]; n++) {
       CC_id = UE_list->ordered_ULCCids[n][UE_id];

       if (mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti) == NULL) {
       LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: no PHY context\n", module_idP,frameP,subframeP,UE_id,rnti,CC_id);
       drop_ue = 1;
       break;
       }
       } */
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
	LOG_W(MAC,
	      "[eNB %d] frame %d subframe %d, UE %d/%x CC %d: UE in weird state, let's put it 'out of sync'\n",
	      module_idP, frameP, subframeP, UE_id, rnti, CC_id);
	// inform RRC of failure and clear timer
	mac_eNB_rrc_ul_failure(module_idP, CC_id, frameP,
			       subframeP, rnti);
	UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
	UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync = 1;
      }
      continue;
    }
    // loop over all active UL CC_ids for this UE
    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      N_RB_UL = to_prb(cc[CC_id].ul_Bandwidth);

      /*
	aggregation=get_aggregation(get_bw_index(module_idP,CC_id),
	eNB_UE_stats->dl_cqi,
	format0);
      */

      if (CCE_allocation_infeasible
	  (module_idP, CC_id, 1, subframeP, aggregation, rnti)) {
	LOG_W(MAC,
	      "[eNB %d] frame %d subframe %d, UE %d/%x CC %d: not enough nCCE\n",
	      module_idP, frameP, subframeP, UE_id, rnti, CC_id);
	continue;	// break;
      }

      /* be sure that there are some free RBs */
      if (first_rb[CC_id] >= N_RB_UL - 1) {
	LOG_W(MAC,
	      "[eNB %d] frame %d subframe %d, UE %d/%x CC %d: dropping, not enough RBs\n",
	      module_idP, frameP, subframeP, UE_id, rnti, CC_id);
	continue;
      }
      //      if (eNB_UE_stats->mode == PUSCH) { // ue has a ulsch channel

      UE_template = &UE_list->UE_template[CC_id][UE_id];
      UE_sched_ctrl = &UE_list->UE_sched_ctrl[UE_id];
      harq_pid = subframe2harqpid(&cc[CC_id], sched_frame, sched_subframeP);
      round = UE_sched_ctrl->round_UL[CC_id][harq_pid];
      AssertFatal(round < 8, "round %d > 7 for UE %d/%x\n", round,
		  UE_id, rnti);
      LOG_D(MAC,
	    "[eNB %d] frame %d subframe %d (sched_frame %d, sched_subframe %d), Checking PUSCH %d for UE %d/%x CC %d : aggregation level %d, N_RB_UL %d\n",
	    module_idP, frameP, subframeP, sched_frame, sched_subframeP, harq_pid, UE_id, rnti,
	    CC_id, aggregation, N_RB_UL);

      RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP * 10) + subframeP] = UE_template->estimated_ul_buffer;
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BO,RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP *
							   10) +
							  subframeP]);
      if (UE_is_to_be_scheduled(module_idP, CC_id, UE_id) > 0 || round > 0)	// || ((frameP%10)==0))
	// if there is information on bsr of DCCH, DTCH or if there is UL_SR, or if there is a packet to retransmit, or we want to schedule a periodic feedback every 10 frames
	{
	  LOG_D(MAC,
		"[eNB %d][PUSCH %d] Frame %d subframe %d Scheduling UE %d/%x in round %d(SR %d,UL_inactivity timer %d,UL_failure timer %d,cqi_req_timer %d)\n",
		module_idP, harq_pid, frameP, subframeP, UE_id, rnti,
		round, UE_template->ul_SR,
		UE_sched_ctrl->ul_inactivity_timer,
		UE_sched_ctrl->ul_failure_timer,
		UE_sched_ctrl->cqi_req_timer);
	  // reset the scheduling request
	  UE_template->ul_SR = 0;
	  status = mac_eNB_get_rrc_status(module_idP, rnti);
	  if (status < RRC_CONNECTED)
	    cqi_req = 0;
	  else if (UE_sched_ctrl->cqi_req_timer > 30) {
	    if (nfapi_mode) {
	      cqi_req = 0;
	    } else {
	      cqi_req = 1;
	    }
	    UE_sched_ctrl->cqi_req_timer = 0;
	  } else
	    cqi_req = 0;

	  //power control
	  //compute the expected ULSCH RX power (for the stats)

	  // this is the normalized RX power and this should be constant (regardless of mcs
	  normalized_rx_power = UE_sched_ctrl->pusch_snr[CC_id];
	  target_rx_power = 178;

	  // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_template->pusch_tpc_tx_frame * 10 + UE_template->pusch_tpc_tx_subframe;
	  if (((framex10psubframe + 10) <= (frameP * 10 + subframeP)) ||	//normal case
	      ((framex10psubframe > (frameP * 10 + subframeP)) && (((10240 - framex10psubframe + frameP * 10 + subframeP) >= 10))))	//frame wrap-around
	    {
	      UE_template->pusch_tpc_tx_frame = frameP;
	      UE_template->pusch_tpc_tx_subframe = subframeP;
	      if (normalized_rx_power > (target_rx_power + 4)) {
		tpc = 0;	//-1
		tpc_accumulated--;
	      } else if (normalized_rx_power < (target_rx_power - 4)) {
		tpc = 2;	//+1
		tpc_accumulated++;
	      } else {
		tpc = 1;	//0
	      }
	    } else {
	    tpc = 1;	//0
	  }
	  //tpc = 1;
	  if (tpc != 1) {
	    LOG_D(MAC,
		  "[eNB %d] ULSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
		  module_idP, frameP, subframeP, harq_pid, tpc,
		  tpc_accumulated, normalized_rx_power,
		  target_rx_power);
	  }
	  // new transmission
	  if (round == 0) {

	    ndi = 1 - UE_template->oldNDI_UL[harq_pid];
	    UE_template->oldNDI_UL[harq_pid] = ndi;
	    UE_list->eNB_UE_stats[CC_id][UE_id].normalized_rx_power = normalized_rx_power;
	    UE_list->eNB_UE_stats[CC_id][UE_id].target_rx_power = target_rx_power;
		UE_template->mcs_UL[harq_pid] = cmin(UE_template->pre_assigned_mcs_ul, slice_maxmcs_uplink[slice_id]);
		UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1= UE_template->mcs_UL[harq_pid];
		//cmin (UE_template->pre_assigned_mcs_ul, openair_daq_vars.target_ue_ul_mcs); // adjust, based on user-defined MCS
	    if (UE_template->pre_allocated_rb_table_index_ul >= 0) {
	      rb_table_index = UE_template->pre_allocated_rb_table_index_ul;
	    } else {
	      UE_template->mcs_UL[harq_pid] = 10;	//cmin (10, openair_daq_vars.target_ue_ul_mcs);
	      rb_table_index = 5;	// for PHR
	    }

	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2 = UE_template->mcs_UL[harq_pid];
	    //            buffer_occupancy = UE_template->ul_total_buffer;


	    while (((rb_table[rb_table_index] > (N_RB_UL - 1 - first_rb[CC_id]))
                    || (rb_table[rb_table_index] > 45))
                    && (rb_table_index > 0)) {
	      rb_table_index--;
	    }

	    UE_template->TBS_UL[harq_pid] = get_TBS_UL(UE_template->mcs_UL[harq_pid],
						       rb_table[rb_table_index]);
	    UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx += rb_table[rb_table_index];
	    UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS = UE_template->TBS_UL[harq_pid];
            UE_list->eNB_UE_stats[CC_id][UE_id].total_ulsch_TBS += UE_template->TBS_UL[harq_pid];
	    //            buffer_occupancy -= TBS;

	    T(T_ENB_MAC_UE_UL_SCHEDULE, T_INT(module_idP),
	      T_INT(CC_id), T_INT(rnti), T_INT(frameP),
	      T_INT(subframeP), T_INT(harq_pid),
	      T_INT(UE_template->mcs_UL[harq_pid]),
	      T_INT(first_rb[CC_id]),
	      T_INT(rb_table[rb_table_index]),
	      T_INT(UE_template->TBS_UL[harq_pid]), T_INT(ndi));

	    if (mac_eNB_get_rrc_status(module_idP, rnti) < RRC_CONNECTED)
	      LOG_D(MAC,
		    "[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE %d (mcs %d, first rb %d, nb_rb %d, rb_table_index %d, TBS %d, harq_pid %d)\n",
		    module_idP, harq_pid, rnti, CC_id, frameP,
		    subframeP, UE_id,
		    UE_template->mcs_UL[harq_pid],
		    first_rb[CC_id], rb_table[rb_table_index],
		    rb_table_index,
		    UE_template->TBS_UL[harq_pid], harq_pid);

	    // bad indices : 20 (40 PRB), 21 (45 PRB), 22 (48 PRB)
	    //store for possible retransmission
	    UE_template->nb_rb_ul[harq_pid] = rb_table[rb_table_index];
	    UE_template->first_rb_ul[harq_pid] = first_rb[CC_id];

	    UE_sched_ctrl->ul_scheduled |= (1 << harq_pid);
	    if (UE_id == UE_list->head)
	      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SCHEDULED,
						      UE_sched_ctrl->ul_scheduled);

	    // adjust scheduled UL bytes by TBS, wait for UL sdus to do final update
	    LOG_D(MAC,
		  "[eNB %d] CC_id %d UE %d/%x : adjusting scheduled_ul_bytes, old %d, TBS %d\n",
		  module_idP, CC_id, UE_id, rnti,
		  UE_template->scheduled_ul_bytes,
		  UE_template->TBS_UL[harq_pid]);

            UE_template->scheduled_ul_bytes += UE_template->TBS_UL[harq_pid];

            LOG_D(MAC, "scheduled_ul_bytes, new %d\n", UE_template->scheduled_ul_bytes);

	    // Cyclic shift for DM RS
	    cshift = 0;	// values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
	    // save it for a potential retransmission
	    UE_template->cshift[harq_pid] = cshift;

	    hi_dci0_pdu = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
	    memset((void *) hi_dci0_pdu, 0,sizeof(nfapi_hi_dci0_request_pdu_t));
	    hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_DCI_PDU_TYPE;
	    hi_dci0_pdu->pdu_size = 2 + sizeof(nfapi_hi_dci0_dci_pdu);
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dci_format = NFAPI_UL_DCI_FORMAT_0;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti = rnti;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.transmission_power = 6000;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.resource_block_start = first_rb[CC_id];
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.number_of_resource_block = rb_table[rb_table_index];
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.mcs_1 = UE_template->mcs_UL[harq_pid];
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms = cshift;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag = 0;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.new_data_indication_1 = ndi;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tpc = tpc;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cqi_csi_request = cqi_req;
	    hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dl_assignment_index = UE_template->DAI_ul[sched_subframeP];

	    hi_dci0_req_body->number_of_dci++;
	    hi_dci0_req_body->sfnsf = sfnsf_add_subframe(frameP, subframeP, 4);
	    hi_dci0_req_body->tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;

	    hi_dci0_req->sfn_sf = frameP<<4|subframeP; // sfnsf_add_subframe(sched_frame, sched_subframeP, 0); // sunday!
	    hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;


	    LOG_D(MAC,
		  "[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d\n",
		  harq_pid, frameP, subframeP, UE_id, rnti,
		  sched_frame, sched_subframeP);

	    // Add UL_config PDUs
	    fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp_body->ul_config_pdu_list[ul_req_tmp_body->number_of_pdus], cqi_req, cc, UE_template->physicalConfigDedicated, get_tmode(module_idP, CC_id, UE_id), mac->ul_handle, rnti, first_rb[CC_id],	// resource_block_start
						 rb_table[rb_table_index],	// number_of_resource_blocks
						 UE_template->mcs_UL[harq_pid], cshift,	// cyclic_shift_2_for_drms
						 0,	// frequency_hopping_enabled_flag
						 0,	// frequency_hopping_bits
						 ndi,	// new_data_indication
						 0,	// redundancy_version
						 harq_pid,	// harq_process_number
						 0,	// ul_tx_mode
						 0,	// current_tx_nb
						 0,	// n_srs
						 get_TBS_UL
						 (UE_template->
						  mcs_UL[harq_pid],
						  rb_table
						  [rb_table_index]));
#ifdef Rel14
	    if (UE_template->rach_resource_type > 0) {	// This is a BL/CE UE allocation
	      fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp_body->ul_config_pdu_list[ul_req_tmp_body->number_of_pdus], UE_template->rach_resource_type > 2 ? 2 : 1, 1,	//total_number_of_repetitions
						   1,	//repetition_number
						   (frameP *
						    10) +
						   subframeP);
	    }
#endif
	    ul_req_tmp->header.message_id = NFAPI_UL_CONFIG_REQUEST;
	    ul_req_tmp_body->number_of_pdus++;
	    ul_req_tmp_body->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
	    mac->ul_handle++;

	    uint16_t ul_sched_frame = sched_frame;
	    uint16_t ul_sched_subframeP = sched_subframeP;

	    add_subframe(&ul_sched_frame, &ul_sched_subframeP, 2);
	    ul_req_tmp->sfn_sf = ul_sched_frame<<4|ul_sched_subframeP;

	    add_ue_ulsch_info(module_idP,
			      CC_id, UE_id, subframeP,
			      S_UL_SCHEDULED);

	    //LOG_D(MAC, "[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0\n", module_idP, CC_id, frameP, subframeP, UE_id);
	    LOG_D(MAC,"[PUSCH %d] SFN/SF:%04d%d UL_CFG:SFN/SF:%04d%d CQI:%d for UE %d/%x\n", harq_pid,frameP,subframeP,ul_sched_frame,ul_sched_subframeP,cqi_req,UE_id,rnti);

	    // increment first rb for next UE allocation
	    first_rb[CC_id] += rb_table[rb_table_index];
	  } else {	// round > 0 => retransmission
	    T(T_ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION,
	      T_INT(module_idP), T_INT(CC_id), T_INT(rnti),
	      T_INT(frameP), T_INT(subframeP), T_INT(harq_pid),
	      T_INT(UE_template->mcs_UL[harq_pid]),
	      T_INT(first_rb[CC_id]),
	      T_INT(rb_table[rb_table_index]), T_INT(round));

#if 0
            /* This is done in rx_sdu, as it has to.
             * Since the code is a bit different, let's keep this version here for review, in case of problem.
             */
	    // fill in NAK information

	    hi_dci0_pdu = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
	    memset((void *) hi_dci0_pdu, 0,
		   sizeof(nfapi_hi_dci0_request_pdu_t));
	    hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_HI_PDU_TYPE;
	    hi_dci0_pdu->pdu_size = 2 + sizeof(nfapi_hi_dci0_hi_pdu);
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start = UE_template->first_rb_ul[harq_pid];
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = UE_template->cshift[harq_pid];
	    hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value = 0;
	    hi_dci0_req_body->number_of_hi++;
	    hi_dci0_req_body->sfnsf = sfnsf_add_subframe(sched_frame, sched_subframeP, 0);
	    hi_dci0_req->sfn_sf = frameP<<4|subframeP;
	    hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;

	    LOG_D(MAC,
		  "[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) UE %d (mcs %d, first rb %d, nb_rb %d, TBS %d, round %d)\n",
		  module_idP, harq_pid, rnti, CC_id, frameP,
		  subframeP, UE_id, UE_template->mcs_UL[harq_pid],
		  UE_template->first_rb_ul[harq_pid],
		  UE_template->nb_rb_ul[harq_pid],
		  UE_template->TBS_UL[harq_pid], round);
#endif
	    // Add UL_config PDUs
	    LOG_D(MAC,
		  "[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d\n",
		  harq_pid, frameP, subframeP, UE_id, rnti,
		  sched_frame, sched_subframeP);
	    fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp_body->ul_config_pdu_list[ul_req_tmp_body->number_of_pdus], cqi_req, cc, UE_template->physicalConfigDedicated, get_tmode(module_idP, CC_id, UE_id), mac->ul_handle, rnti, UE_template->first_rb_ul[harq_pid],	// resource_block_start
						 UE_template->nb_rb_ul[harq_pid],	// number_of_resource_blocks
						 UE_template->mcs_UL[harq_pid], cshift,	// cyclic_shift_2_for_drms
						 0,	// frequency_hopping_enabled_flag
						 0,	// frequency_hopping_bits
						 UE_template->oldNDI_UL[harq_pid],	// new_data_indication
						 rvidx_tab[round & 3],	// redundancy_version
						 harq_pid,	// harq_process_number
						 0,	// ul_tx_mode
						 0,	// current_tx_nb
						 0,	// n_srs
						 UE_template->
						 TBS_UL[harq_pid]);
#ifdef Rel14
	    if (UE_template->rach_resource_type > 0) {	// This is a BL/CE UE allocation
	      fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp_body->ul_config_pdu_list[ul_req_tmp_body->number_of_pdus], UE_template->rach_resource_type > 2 ? 2 : 1, 1,	//total_number_of_repetitions
						   1,	//repetition_number
						   (frameP *
						    10) +
						   subframeP);
	    }
#endif
	    ul_req_tmp_body->number_of_pdus++;
	    mac->ul_handle++;

	    ul_req_tmp_body->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;

	    ul_req_tmp->sfn_sf = sched_frame<<4|sched_subframeP;
	    ul_req_tmp->header.message_id = NFAPI_UL_CONFIG_REQUEST;

	    LOG_D(MAC,"[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d cqi_req %d\n",
		  harq_pid,frameP,subframeP,UE_id,rnti,sched_frame,sched_subframeP,cqi_req);
	  }		/*
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

	}			// UE_is_to_be_scheduled
    }			// loop over UE_id
  }				// loop of CC_id
}
