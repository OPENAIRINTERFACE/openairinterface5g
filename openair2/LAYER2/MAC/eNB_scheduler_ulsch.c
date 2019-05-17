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

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "assertions.h"
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

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

extern int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req);
extern void add_subframe(uint16_t *frameP, uint16_t *subframeP, int offset);
extern uint16_t sfnsf_add_subframe(uint16_t frameP, uint16_t subframeP, int offset);
extern int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req);


// This table holds the allowable PRB sizes for ULSCH transmissions
uint8_t rb_table[34] = {
  1, 2, 3, 4, 5,      // 0-4
  6, 8, 9, 10, 12,    // 5-9
  15, 16, 18, 20, 24, // 10-14
  25, 27, 30, 32, 36, // 15-19
  40, 45, 48, 50, 54, // 20-24
  60, 64, 72, 75, 80, // 25-29
  81, 90, 96, 100     // 30-33
};

extern mui_t rrc_eNB_mui;

//-----------------------------------------------------------------------------
/*
* When data are received on PHY and transmitted to MAC
*/
void
rx_sdu(const module_id_t enb_mod_idP,
       const int CC_idP,
       const frame_t frameP,
       const sub_frame_t subframeP,
       const rnti_t rntiP,
       uint8_t *sduP,
       const uint16_t sdu_lenP,
       const uint16_t timing_advance,
       const uint8_t ul_cqi)
//-----------------------------------------------------------------------------
{
  int current_rnti = 0;
  int UE_id = -1;
  int RA_id = 0;
  int old_rnti = -1;
  int old_UE_id = -1;
  int crnti_rx = 0;
  int harq_pid = 0;
  int first_rb = 0;
  unsigned char num_ce = 0;
  unsigned char num_sdu = 0;
  unsigned char *payload_ptr = NULL;
  unsigned char rx_ces[MAX_NUM_CE];
  unsigned char rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];
  uint8_t lcgid = 0;
  int lcgid_updated[4] = {0, 0, 0, 0};
  eNB_MAC_INST *mac = NULL;
  UE_list_t *UE_list = NULL;
  rrc_eNB_ue_context_t *ue_contextP = NULL;
  UE_sched_ctrl *UE_scheduling_control = NULL;
  UE_TEMPLATE *UE_template_ptr = NULL;

  /* Init */
  current_rnti = rntiP;
  UE_id = find_UE_id(enb_mod_idP, current_rnti);
  mac = RC.mac[enb_mod_idP];
  harq_pid = subframe2harqpid(&mac->common_channels[CC_idP], frameP, subframeP);
  UE_list = &mac->UE_list;
  memset(rx_ces, 0, MAX_NUM_CE * sizeof(unsigned char));
  memset(rx_lcids, 0, NB_RB_MAX * sizeof(unsigned char));
  memset(rx_lengths, 0, NB_RB_MAX * sizeof(unsigned short));
  start_meas(&mac->rx_ulsch_sdu);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_SDU, 1);

  if (opt_enabled == 1) {
    trace_pdu(DIRECTION_UPLINK, sduP, sdu_lenP, 0, WS_C_RNTI, current_rnti, frameP, subframeP, 0, 0);
    LOG_D(OPT, "[eNB %d][ULSCH] Frame %d rnti %x with size %d\n",
          enb_mod_idP,
          frameP,
          current_rnti,
          sdu_lenP);
  }

  if (UE_id != -1) {
    UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);
    UE_template_ptr = &(UE_list->UE_template[CC_idP][UE_id]);

    LOG_D(MAC, "[eNB %d][PUSCH %d] CC_id %d %d.%d Received ULSCH sdu round %d from PHY (rnti %x, UE_id %d) ul_cqi %d\n",
          enb_mod_idP,
          harq_pid,
          CC_idP,
          frameP,
          subframeP,
          UE_scheduling_control->round_UL[CC_idP][harq_pid],
          current_rnti,
          UE_id,
          ul_cqi);

    AssertFatal(UE_scheduling_control->round_UL[CC_idP][harq_pid] < 8, "round >= 8\n");

    if (sduP != NULL) {
      UE_scheduling_control->ul_inactivity_timer = 0;
      UE_scheduling_control->ul_failure_timer = 0;
      UE_scheduling_control->ul_scheduled &= (~(1 << harq_pid));
      /* Update with smoothing: 3/4 of old value and 1/4 of new.
       * This is the logic that was done in the function
       * lte_est_timing_advance_pusch, maybe it's not necessary?
       * maybe it's even not correct at all?
       */
      UE_scheduling_control->ta_update = (UE_scheduling_control->ta_update * 3 + timing_advance) / 4;
      UE_scheduling_control->pusch_snr[CC_idP] = ul_cqi;
      UE_scheduling_control->ul_consecutive_errors = 0;

      first_rb = UE_template_ptr->first_rb_ul[harq_pid];

      if (UE_scheduling_control->ul_out_of_sync > 0) {
        UE_scheduling_control->ul_out_of_sync = 0;
        mac_eNB_rrc_ul_in_sync(enb_mod_idP, CC_idP, frameP, subframeP, current_rnti);
      }

      /* Update bytes to schedule */
      UE_template_ptr->scheduled_ul_bytes -= UE_template_ptr->TBS_UL[harq_pid];

      if (UE_template_ptr->scheduled_ul_bytes < 0) {
        UE_template_ptr->scheduled_ul_bytes = 0;
      }
    } else {  // sduP == NULL => error
      LOG_W(MAC, "[eNB %d][PUSCH %d] CC_id %d %d.%d ULSCH in error in round %d, ul_cqi %d, UE_id %d, RNTI %x\n",
            enb_mod_idP,
            harq_pid,
            CC_idP,
            frameP,
            subframeP,
            UE_scheduling_control->round_UL[CC_idP][harq_pid],
            ul_cqi,
            UE_id,
            current_rnti);

      if (ul_cqi > 200) { // too high energy pattern
        UE_scheduling_control->pusch_snr[CC_idP] = ul_cqi;
        LOG_W(MAC, "[MAC] Too high energy pattern\n");
      }

      if (UE_scheduling_control->round_UL[CC_idP][harq_pid] == 3) {
        UE_scheduling_control->ul_scheduled &= (~(1 << harq_pid));
        UE_scheduling_control->round_UL[CC_idP][harq_pid] = 0;

        if (UE_scheduling_control->ul_consecutive_errors++ == 10) {
          UE_scheduling_control->ul_failure_timer = 1;
        }

        /* Update scheduled bytes */
        UE_template_ptr->scheduled_ul_bytes -= UE_template_ptr->TBS_UL[harq_pid];

        if (UE_template_ptr->scheduled_ul_bytes < 0) {
          UE_template_ptr->scheduled_ul_bytes = 0;
        }

        if (find_RA_id(enb_mod_idP, CC_idP, current_rnti) != -1) {
          cancel_ra_proc(enb_mod_idP, CC_idP, frameP, current_rnti);
        }
      } else {
        UE_scheduling_control->round_UL[CC_idP][harq_pid]++;
      }

      /* CDRX UL HARQ timers */
      if (UE_scheduling_control->cdrx_configured == TRUE) {
        /* Synchronous UL HARQ */
        UE_scheduling_control->ul_synchronous_harq_timer[CC_idP][harq_pid] = 5;
        /* 
         * The NACK is programmed in n+4 subframes, so UE will have drxRetransmission running.
         * Setting ul_synchronous_harq_timer = 5 will trigger drxRetransmission timer.
         * Note: in case of asynchronous UL HARQ process restart here relevant RTT timer.
         * Start corresponding CDRX ULRetransmission timer.
         */
      }

      first_rb = UE_template_ptr->first_rb_ul[harq_pid];

      /* Program NACK for PHICH */
      LOG_D(MAC, "Programming PHICH NACK for rnti %x harq_pid %d (first_rb %d)\n",
            current_rnti,
            harq_pid,
            first_rb);

      nfapi_hi_dci0_request_t *hi_dci0_req = NULL;
      uint8_t sf_ahead_dl = ul_subframe2_k_phich(&mac->common_channels[CC_idP], subframeP);
      hi_dci0_req = &mac->HI_DCI0_req[CC_idP][(subframeP + sf_ahead_dl) % 10];
      nfapi_hi_dci0_request_body_t *hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
      nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
      memset((void *) hi_dci0_pdu, 0, sizeof(nfapi_hi_dci0_request_pdu_t));
      hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_HI_PDU_TYPE;
      hi_dci0_pdu->pdu_size = 2 + sizeof(nfapi_hi_dci0_hi_pdu);
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start = first_rb;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = 0;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value = 0;
      hi_dci0_req_body->number_of_hi++;
      hi_dci0_req_body->sfnsf = sfnsf_add_subframe(frameP, subframeP, 0);
      hi_dci0_req_body->tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
      hi_dci0_req->sfn_sf = sfnsf_add_subframe(frameP, subframeP, sf_ahead_dl);
      hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;
      return;
    }

    // if UE_id == -1
  } else if ((RA_id = find_RA_id(enb_mod_idP, CC_idP, current_rnti)) != -1) { // Check if this is an RA process for the rnti
    RA_t *ra = (RA_t *) &(mac->common_channels[CC_idP].ra[RA_id]);
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

    if (ra->rach_resource_type > 0) {
      harq_pid = 0;
    }

#endif
    AssertFatal(mac->common_channels[CC_idP].radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx > 1,
                "maxHARQ %d should be greater than 1\n",
                (int) mac->common_channels[CC_idP].radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx);
    LOG_D(MAC, "[eNB %d][PUSCH %d] CC_id %d [RAPROC Msg3] Received ULSCH sdu round %d from PHY (rnti %x, RA_id %d) ul_cqi %d\n",
          enb_mod_idP,
          harq_pid,
          CC_idP,
          ra->msg3_round,
          current_rnti,
          RA_id,
          ul_cqi);
    first_rb = ra->msg3_first_rb;

    if (sduP == NULL) { // we've got an error on Msg3
      LOG_D(MAC, "[eNB %d] CC_id %d, RA %d ULSCH in error in round %d/%d\n",
            enb_mod_idP,
            CC_idP,
            RA_id,
            ra->msg3_round,
            (int) mac->common_channels[CC_idP].radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx);

      if (ra->msg3_round >= mac->common_channels[CC_idP].radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx - 1) {
        cancel_ra_proc(enb_mod_idP, CC_idP, frameP, current_rnti);
      } else {
        // first_rb = UE_template_ptr->first_rb_ul[harq_pid]; // UE_id = -1 !!!!
        ra->msg3_round++;
        /* Prepare handling of retransmission */
        get_Msg3allocret(&mac->common_channels[CC_idP],
                         ra->Msg3_subframe,
                         ra->Msg3_frame,
                         &ra->Msg3_frame,
                         &ra->Msg3_subframe);
        // prepare handling of retransmission
        add_msg3(enb_mod_idP, CC_idP, ra, frameP, subframeP);
      }

      /* TODO: program NACK for PHICH? */
      return;
    }
  } else {
    LOG_W(MAC, "Cannot find UE or RA corresponding to ULSCH rnti %x, dropping it\n", current_rnti);
    return;
  }

  payload_ptr = parse_ulsch_header(sduP, &num_ce, &num_sdu, rx_ces, rx_lcids, rx_lengths, sdu_lenP);

  if (payload_ptr == NULL) {
    LOG_E(MAC,"[eNB %d][PUSCH %d] CC_id %d ulsch header unknown lcid(rnti %x, UE_id %d)\n",
          enb_mod_idP,
          harq_pid,
          CC_idP,
          current_rnti,
          UE_id);
    return;
  }

  T(T_ENB_MAC_UE_UL_PDU,
    T_INT(enb_mod_idP),
    T_INT(CC_idP),
    T_INT(current_rnti),
    T_INT(frameP),
    T_INT(subframeP),
    T_INT(harq_pid),
    T_INT(sdu_lenP),
    T_INT(num_ce),
    T_INT(num_sdu));
  T(T_ENB_MAC_UE_UL_PDU_WITH_DATA,
    T_INT(enb_mod_idP),
    T_INT(CC_idP),
    T_INT(current_rnti),
    T_INT(frameP),
    T_INT(subframeP),
    T_INT(harq_pid),
    T_INT(sdu_lenP),
    T_INT(num_ce),
    T_INT(num_sdu),
    T_BUFFER(sduP, sdu_lenP));
  mac->eNB_stats[CC_idP].ulsch_bytes_rx = sdu_lenP;
  mac->eNB_stats[CC_idP].total_ulsch_bytes_rx += sdu_lenP;
  mac->eNB_stats[CC_idP].total_ulsch_pdus_rx += 1;

  if (UE_id != -1) {
    UE_scheduling_control->round_UL[CC_idP][harq_pid] = 0;
  }

  /* Control element */
  for (int i = 0; i < num_ce; i++) {
    T(T_ENB_MAC_UE_UL_CE,
      T_INT(enb_mod_idP),
      T_INT(CC_idP),
      T_INT(current_rnti),
      T_INT(frameP),
      T_INT(subframeP),
      T_INT(rx_ces[i]));

    switch (rx_ces[i]) {  // implement and process PHR + CRNTI + BSR
      case POWER_HEADROOM:
        if (UE_id != -1) {
          UE_template_ptr->phr_info = (payload_ptr[0] & 0x3f) - PHR_MAPPING_OFFSET + (int8_t)(hundred_times_log10_NPRB[UE_template_ptr->nb_rb_ul[harq_pid] - 1] / 100);

          if (UE_template_ptr->phr_info > 40) {
            UE_template_ptr->phr_info = 40;
          }

          LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : Received PHR PH = %d (db)\n",
                enb_mod_idP,
                CC_idP,
                rx_ces[i],
                UE_template_ptr->phr_info);

          UE_template_ptr->phr_info_configured = 1;
          UE_scheduling_control->phr_received = 1;
        }

        payload_ptr += sizeof(POWER_HEADROOM_CMD);
        break;

      case CRNTI:
        old_rnti = (((uint16_t) payload_ptr[0]) << 8) + payload_ptr[1];
        old_UE_id = find_UE_id(enb_mod_idP, old_rnti);
        LOG_D(MAC, "[eNB %d] Frame %d, Subframe %d CC_id %d MAC CE_LCID %d (ce %d/%d): CRNTI %x (UE_id %d) in Msg3\n",
              enb_mod_idP,
              frameP,
              subframeP,
              CC_idP,
              rx_ces[i],
              i,
              num_ce,
              old_rnti,
              old_UE_id);

        /* Receiving CRNTI means that the current rnti has to go away */
        if (old_UE_id != -1) {
          if (mac_eNB_get_rrc_status(enb_mod_idP,old_rnti) ==  RRC_HO_EXECUTION) {
            LOG_I(MAC, "[eNB %d] Frame %d, Subframe %d CC_id %d : (rnti %x UE_id %d) Handover case\n",
                  enb_mod_idP,
                  frameP,
                  subframeP,
                  CC_idP,
                  old_rnti,
                  old_UE_id);

            UE_id = old_UE_id;
            current_rnti = old_rnti;
            /* Clear timer */
            UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);
            UE_template_ptr = &(UE_list->UE_template[CC_idP][UE_id]);

            UE_scheduling_control->uplane_inactivity_timer = 0;
            UE_scheduling_control->ul_inactivity_timer = 0;
            UE_scheduling_control->ul_failure_timer = 0;

            if (UE_scheduling_control->ul_out_of_sync > 0) {
              UE_scheduling_control->ul_out_of_sync = 0;
              mac_eNB_rrc_ul_in_sync(enb_mod_idP,
                                     CC_idP,
                                     frameP,
                                     subframeP,
                                     old_rnti);
            }
            UE_template_ptr->ul_SR = 1;
            UE_scheduling_control->crnti_reconfigurationcomplete_flag = 1;
            UE_list->UE_template[UE_PCCID(enb_mod_idP, UE_id)][UE_id].configured = 1;
            cancel_ra_proc(enb_mod_idP,
                           CC_idP,
                           frameP,
                           current_rnti);
          } else {
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
            RA_id = find_RA_id(enb_mod_idP, CC_idP, current_rnti);

            if (RA_id != -1) {
              RA_t *ra = &(mac->common_channels[CC_idP].ra[RA_id]);
              mac_rrc_data_ind(enb_mod_idP,
                               CC_idP,
                               frameP, subframeP,
                               UE_id,
                               old_rnti,
                               DCCH,
                               (uint8_t *) payload_ptr,
                               rx_lengths[i],
                               0
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                               ,ra->rach_resource_type > 0
#endif
                              );
              /* Received a new rnti */
              ra->state = MSGCRNTI;
              LOG_I(MAC, "[eNB %d] Frame %d, Subframe %d CC_id %d : (rnti %x UE_id %d) Received rnti(Msg4)\n",
                    enb_mod_idP,
                    frameP,
                    subframeP,
                    CC_idP,
                    old_rnti,
                    old_UE_id);
              UE_id = old_UE_id;
              current_rnti = old_rnti;
              ra->rnti = old_rnti;
              ra->crnti_rrc_mui = rrc_eNB_mui-1;
              ra->crnti_harq_pid = -1;

              /* Clear timer */
              UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);
              UE_template_ptr = &(UE_list->UE_template[CC_idP][UE_id]);

              UE_scheduling_control->uplane_inactivity_timer = 0;
              UE_scheduling_control->ul_inactivity_timer = 0;
              UE_scheduling_control->ul_failure_timer = 0;

              if (UE_scheduling_control->ul_out_of_sync > 0) {
                UE_scheduling_control->ul_out_of_sync = 0;
                mac_eNB_rrc_ul_in_sync(enb_mod_idP, CC_idP, frameP, subframeP, old_rnti);
              }
              UE_template_ptr->ul_SR = 1;
              UE_scheduling_control->crnti_reconfigurationcomplete_flag = 1;
              // break;
            }
          }
        } else {
          cancel_ra_proc(enb_mod_idP, CC_idP, frameP, current_rnti);
          LOG_W(MAC, "[MAC] Can't find old UE_id\n");
        }

        crnti_rx = 1;
        payload_ptr += 2; // sizeof(CRNTI)
        break;

      case TRUNCATED_BSR:
      case SHORT_BSR:
        lcgid = (payload_ptr[0] >> 6);
        LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : Received short BSR LCGID = %u bsr = %d\n",
              enb_mod_idP,
              CC_idP,
              rx_ces[i],
              lcgid,
              payload_ptr[0] & 0x3f);

        if (UE_id != -1) {
          int bsr = 0;
          bsr = payload_ptr[0] & 0x3f;
          lcgid_updated[lcgid] = 1;
          /* Update buffer info */
          UE_template_ptr->ul_buffer_info[lcgid] = BSR_TABLE[bsr];
          UE_template_ptr->estimated_ul_buffer =
            UE_template_ptr->ul_buffer_info[LCGID0] +
            UE_template_ptr->ul_buffer_info[LCGID1] +
            UE_template_ptr->ul_buffer_info[LCGID2] +
            UE_template_ptr->ul_buffer_info[LCGID3];

          RC.eNB[enb_mod_idP][CC_idP]->pusch_stats_bsr[UE_id][(frameP * 10) + subframeP] = (payload_ptr[0] & 0x3f);

          if (UE_id == UE_list->head) {
            VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR, (payload_ptr[0] & 0x3f));
          }

          if (UE_template_ptr->ul_buffer_creation_time[lcgid] == 0) {
            UE_template_ptr->ul_buffer_creation_time[lcgid] = frameP;
          }

          if (mac_eNB_get_rrc_status(enb_mod_idP,UE_RNTI(enb_mod_idP, UE_id)) < RRC_CONNECTED) {
            LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d : estimated_ul_buffer = %d (lcg increment %d)\n",
                  enb_mod_idP,
                  CC_idP,
                  rx_ces[i],
                  UE_template_ptr->estimated_ul_buffer,
                  UE_template_ptr->ul_buffer_info[lcgid]);
          }
        } else {
          /* Need error message */
        }

        payload_ptr += 1;  // sizeof(SHORT_BSR)
        break;

      case LONG_BSR:
        if (UE_id != -1) {
          int bsr0 = (payload_ptr[0] & 0xFC) >> 2;
          int bsr1 = ((payload_ptr[0] & 0x03) << 4) | ((payload_ptr[1] & 0xF0) >> 4);
          int bsr2 = ((payload_ptr[1] & 0x0F) << 2) | ((payload_ptr[2] & 0xC0) >> 6);
          int bsr3 = payload_ptr[2] & 0x3F;
          lcgid_updated[LCGID0] = 1;
          lcgid_updated[LCGID1] = 1;
          lcgid_updated[LCGID2] = 1;
          lcgid_updated[LCGID3] = 1;
          /* Update buffer info */
          UE_template_ptr->ul_buffer_info[LCGID0] = BSR_TABLE[bsr0];
          UE_template_ptr->ul_buffer_info[LCGID1] = BSR_TABLE[bsr1];
          UE_template_ptr->ul_buffer_info[LCGID2] = BSR_TABLE[bsr2];
          UE_template_ptr->ul_buffer_info[LCGID3] = BSR_TABLE[bsr3];

          UE_template_ptr->estimated_ul_buffer =
            UE_template_ptr->ul_buffer_info[LCGID0] +
            UE_template_ptr->ul_buffer_info[LCGID1] +
            UE_template_ptr->ul_buffer_info[LCGID2] +
            UE_template_ptr->ul_buffer_info[LCGID3];

          LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d: Received long BSR. Size is LCGID0 = %u LCGID1 = %u LCGID2 = %u LCGID3 = %u\n",
            enb_mod_idP,
            CC_idP,
            rx_ces[i],
            UE_template_ptr->ul_buffer_info[LCGID0],
            UE_template_ptr->ul_buffer_info[LCGID1],
            UE_template_ptr->ul_buffer_info[LCGID2],
            UE_template_ptr->ul_buffer_info[LCGID3]);

          if (crnti_rx == 1) {
            LOG_D(MAC, "[eNB %d] CC_id %d MAC CE_LCID %d: Received CRNTI.\n",
                  enb_mod_idP,
                  CC_idP,
                  rx_ces[i]);
          }

          for(int lcgid = 0; lcgid <= LCGID3; lcgid++) {
            if (UE_template_ptr->ul_buffer_info[lcgid] == 0) {
              UE_template_ptr->ul_buffer_creation_time[lcgid] = 0;
            } else if (UE_template_ptr->ul_buffer_creation_time[lcgid] == 0) {
              UE_template_ptr->ul_buffer_creation_time[lcgid] = frameP;
            }
          }
        }

        payload_ptr += 3; // sizeof(LONG_BSR)
        break;

      default:
        LOG_E(MAC, "[eNB %d] CC_id %d Received unknown MAC header (0x%02x)\n",
              enb_mod_idP,
              CC_idP,
              rx_ces[i]);
        break;
    } // end switch on control element
  } // end for loop on control element

  for (int i = 0; i < num_sdu; i++) {
    LOG_D(MAC, "SDU Number %d MAC Subheader SDU_LCID %d, length %d\n",
          i,
          rx_lcids[i],
          rx_lengths[i]);
    T(T_ENB_MAC_UE_UL_SDU,
      T_INT(enb_mod_idP),
      T_INT(CC_idP),
      T_INT(current_rnti),
      T_INT(frameP),
      T_INT(subframeP),
      T_INT(rx_lcids[i]),
      T_INT(rx_lengths[i]));
    T(T_ENB_MAC_UE_UL_SDU_WITH_DATA,
      T_INT(enb_mod_idP),
      T_INT(CC_idP),
      T_INT(current_rnti),
      T_INT(frameP),
      T_INT(subframeP),
      T_INT(rx_lcids[i]),
      T_INT(rx_lengths[i]),
      T_BUFFER(payload_ptr, rx_lengths[i]));

    switch (rx_lcids[i]) {
      case CCCH:
        if (rx_lengths[i] > CCCH_PAYLOAD_SIZE_MAX) {
          LOG_E(MAC, "[eNB %d/%d] frame %d received CCCH of size %d (too big, maximum allowed is %d, sdu_len %d), dropping packet\n",
                enb_mod_idP,
                CC_idP,
                frameP,
                rx_lengths[i],
                CCCH_PAYLOAD_SIZE_MAX,
                sdu_lenP);
          break;
        }

        LOG_D(MAC, "[eNB %d][RAPROC] CC_id %d Frame %d, Received CCCH:  %x.%x.%x.%x.%x.%x, Terminating RA procedure for UE rnti %x\n",
              enb_mod_idP,
              CC_idP,
              frameP,
              payload_ptr[0], payload_ptr[1], payload_ptr[2], payload_ptr[3], payload_ptr[4], payload_ptr[5],
              current_rnti);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TERMINATE_RA_PROC, 1);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TERMINATE_RA_PROC, 0);
        RA_id = find_RA_id(enb_mod_idP, CC_idP, current_rnti);

        if (RA_id != -1) {
          RA_t *ra = &(mac->common_channels[CC_idP].ra[RA_id]);
          LOG_D(MAC, "[mac %d][RAPROC] CC_id %d Checking proc %d : rnti (%x, %x), state %d\n",
                enb_mod_idP,
                CC_idP,
                RA_id,
                ra->rnti,
                current_rnti,
                ra->state);

          if (UE_id < 0) {
            memcpy(&(ra->cont_res_id[0]), payload_ptr, 6);
            LOG_D(MAC, "[eNB %d][RAPROC] CC_id %d Frame %d CCCH: Received Msg3: length %d, offset %ld\n",
                  enb_mod_idP,
                  CC_idP,
                  frameP,
                  rx_lengths[i],
                  payload_ptr - sduP);

            if ((UE_id = add_new_ue(enb_mod_idP, CC_idP, ra->rnti, harq_pid
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                    , ra->rach_resource_type
#endif
                                   )) == -1) {
              LOG_E(MAC,"[MAC][eNB] Max user count reached\n");
              cancel_ra_proc(enb_mod_idP, CC_idP, frameP, current_rnti); // send Connection Reject ???
              break;
              // kill RA proc
            } else {
              LOG_D(MAC, "[eNB %d][RAPROC] CC_id %d Frame %d Added user with rnti %x => UE %d\n",
                    enb_mod_idP,
                    CC_idP,
                    frameP,
                    ra->rnti,
                    UE_id);

              UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);
              UE_template_ptr = &(UE_list->UE_template[CC_idP][UE_id]);
            }
          } else {
            LOG_D(MAC, "[eNB %d][RAPROC] CC_id %d Frame %d CCCH: Received Msg3 from already registered UE %d: length %d, offset %ld\n",
                  enb_mod_idP,
                  CC_idP,
                  frameP,
                  UE_id,
                  rx_lengths[i],
                  payload_ptr - sduP);
            // kill RA proc
          }

          mac_rrc_data_ind(enb_mod_idP,
                           CC_idP,
                           frameP, subframeP,
                           UE_id,
                           current_rnti,
                           CCCH,
                           (uint8_t *) payload_ptr,
                           rx_lengths[i],
                           0
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                           ,ra->rach_resource_type > 0
#endif
                          );

          if (num_ce > 0) { // handle msg3 which is not RRCConnectionRequest
            //  process_ra_message(msg3,num_ce,rx_lcids,rx_ces);
          }

          // prepare transmission of Msg4
          ra->state = MSG4;

          if(mac->common_channels[CC_idP].tdd_Config != NULL) {
            switch(mac->common_channels[CC_idP].tdd_Config->subframeAssignment) {
              case 1:
                ra->Msg4_frame = frameP + ((subframeP > 2) ? 1 : 0);
                ra->Msg4_subframe = (subframeP + 7) % 10;
                break;

              default:
                printf("%s:%d: TODO\n", __FILE__, __LINE__);
                abort();
                // TODO need to be complete for other tdd configs.
            }
          } else {
            /* Program Msg4 PDCCH+DLSCH/MPDCCH transmission 4 subframes from now,
              * Check if this is ok for BL/CE, or if the rule is different
              */
            ra->Msg4_frame = frameP + ((subframeP > 5) ? 1 : 0);
            ra->Msg4_subframe = (subframeP + 4) % 10;
          }

          UE_scheduling_control->crnti_reconfigurationcomplete_flag = 0;
        } // if RA process is active

        break;

      case DCCH:
      case DCCH1:
#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
        LOG_T(MAC, "offset: %d\n", (unsigned char) ((unsigned char *) payload_ptr - sduP));

        for (int j = 0; j < 32; j++) {
          LOG_T(MAC, "%x ", payload_ptr[j]);
        }

        LOG_T(MAC, "\n");
#endif

        if (UE_id != -1) {
          if (lcgid_updated[UE_template_ptr->lcgidmap[rx_lcids[i]]] == 0) {
            /* Adjust buffer occupancy of the correponding logical channel group */
            if (UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]] >= rx_lengths[i])
              UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]] -= rx_lengths[i];
            else
              UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]] = 0;

            UE_template_ptr->estimated_ul_buffer =
              UE_template_ptr->ul_buffer_info[0] +
              UE_template_ptr->ul_buffer_info[1] +
              UE_template_ptr->ul_buffer_info[2] +
              UE_template_ptr->ul_buffer_info[3];
            //UE_template_ptr->estimated_ul_buffer += UE_template_ptr->estimated_ul_buffer / 4;
          }

          LOG_D(MAC,
                "[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DCCH, received %d bytes form UE %d on LCID %d \n",
                enb_mod_idP, CC_idP, frameP, rx_lengths[i], UE_id,
                rx_lcids[i]);
          mac_rlc_data_ind(enb_mod_idP, current_rnti, enb_mod_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, rx_lcids[i], (char *) payload_ptr, rx_lengths[i], 1, NULL);  //(unsigned int*)crc_status);
          UE_list->eNB_UE_stats[CC_idP][UE_id].num_pdu_rx[rx_lcids[i]] += 1;
          UE_list->eNB_UE_stats[CC_idP][UE_id].num_bytes_rx[rx_lcids[i]] += rx_lengths[i];
        }

        break;

      // all the DRBS
      case DTCH:
      default:
#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
        LOG_T(MAC, "offset: %d\n",
              (unsigned char) ((unsigned char *) payload_ptr - sduP));

        for (int j = 0; j < 32; j++) {
          LOG_T(MAC, "%x ", payload_ptr[j]);
        }

        LOG_T(MAC, "\n");
#endif

        if (rx_lcids[i] < NB_RB_MAX) {
          LOG_D(MAC, "[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DTCH, received %d bytes from UE %d for lcid %d\n",
                enb_mod_idP,
                CC_idP,
                frameP,
                rx_lengths[i],
                UE_id,
                rx_lcids[i]);

          if (UE_id != -1) {
            /* Adjust buffer occupancy of the correponding logical channel group */
            LOG_D(MAC, "[eNB %d] CC_id %d Frame %d : ULSCH -> UL-DTCH, received %d bytes from UE %d for lcid %d, removing from LCGID %ld, %d\n",
                  enb_mod_idP,
                  CC_idP,
                  frameP,
                  rx_lengths[i],
                  UE_id,
                  rx_lcids[i],
                  UE_template_ptr->lcgidmap[rx_lcids[i]],
                  UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]]);

            if (lcgid_updated[UE_template_ptr->lcgidmap[rx_lcids[i]]] == 0) {
              if (UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]] >= rx_lengths[i]) {
                UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]] -= rx_lengths[i];
              } else {
                UE_template_ptr->ul_buffer_info[UE_template_ptr->lcgidmap[rx_lcids[i]]] = 0;
              }

              UE_template_ptr->estimated_ul_buffer =
                UE_template_ptr->ul_buffer_info[0] +
                UE_template_ptr->ul_buffer_info[1] +
                UE_template_ptr->ul_buffer_info[2] +
                UE_template_ptr->ul_buffer_info[3];
            }

            if ((rx_lengths[i] < SCH_PAYLOAD_SIZE_MAX) && (rx_lengths[i] > 0)) {  // MAX SIZE OF transport block
              mac_rlc_data_ind(enb_mod_idP, current_rnti, enb_mod_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, rx_lcids[i], (char *) payload_ptr, rx_lengths[i], 1, NULL);
              UE_list->eNB_UE_stats[CC_idP][UE_id].num_pdu_rx[rx_lcids[i]] += 1;
              UE_list->eNB_UE_stats[CC_idP][UE_id].num_bytes_rx[rx_lcids[i]] += rx_lengths[i];
              /* Clear uplane_inactivity_timer */
              UE_scheduling_control->uplane_inactivity_timer = 0;

              /* Reset RRC inactivity timer after uplane activity */
              ue_contextP = rrc_eNB_get_ue_context(RC.rrc[enb_mod_idP], current_rnti);

              if (ue_contextP != NULL) {
                ue_contextP->ue_context.ue_rrc_inactivity_timer = 1;
              } else {
                LOG_E(MAC, "[eNB %d] CC_id %d Couldn't find the context associated to UE (RNTI %d) and reset RRC inactivity timer\n",
                      enb_mod_idP,
                      CC_idP,
                      current_rnti);
              }
            } else {  /* rx_length[i] Max size */
              UE_list->eNB_UE_stats[CC_idP][UE_id].num_errors_rx += 1;
              LOG_E(MAC, "[eNB %d] CC_id %d Frame %d : Max size of transport block reached LCID %d from UE %d ",
                    enb_mod_idP,
                    CC_idP,
                    frameP,
                    rx_lcids[i],
                    UE_id);
            }
          } else {  // end if (UE_id != -1)
            LOG_E(MAC,"[eNB %d] CC_id %d Frame %d : received unsupported or unknown LCID %d from UE %d ",
                  enb_mod_idP,
                  CC_idP,
                  frameP,
                  rx_lcids[i],
                  UE_id);
          }
        }

        break;
    }

    payload_ptr += rx_lengths[i];
  }

  /* CDRX UL HARQ timers */
  if (UE_id != -1) {
    if (UE_scheduling_control->cdrx_configured == TRUE) {
      /* Synchronous UL HARQ */
      UE_scheduling_control->ul_synchronous_harq_timer[CC_idP][harq_pid] = 5;
      /* 
       * The ACK is programmed in n+4 subframes, so UE will have drxRetransmission running.
       * Setting ul_synchronous_harq_timer = 5 will trigger drxRetransmission timer.
       * Note: in case of asynchronous UL HARQ process restart here relevant RTT timer
       * Stop corresponding CDRX ULRetransmission timer
       */
    }
  }

  /* Program ACK for PHICH */
  LOG_D(MAC, "Programming PHICH ACK for rnti %x harq_pid %d (first_rb %d)\n",
        current_rnti,
        harq_pid,
        first_rb);

  nfapi_hi_dci0_request_t *hi_dci0_req;
  uint8_t sf_ahead_dl = ul_subframe2_k_phich(&mac->common_channels[CC_idP], subframeP);
  hi_dci0_req = &mac->HI_DCI0_req[CC_idP][(subframeP+sf_ahead_dl)%10];
  nfapi_hi_dci0_request_body_t *hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
  nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci +
                                      hi_dci0_req_body->number_of_hi];
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
  hi_dci0_req->sfn_sf = sfnsf_add_subframe(frameP,subframeP, sf_ahead_dl);
  hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;

  /* NN--> FK: we could either check the payload, or use a phy helper to detect a false msg3 */
  if ((num_sdu == 0) && (num_ce == 0)) {
    if (UE_id != -1)
      UE_list->eNB_UE_stats[CC_idP][UE_id].total_num_errors_rx += 1;
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

//-----------------------------------------------------------------------------
/*
 * Return the BSR table index corresponding to the number of bytes in input
 */
uint32_t
bytes_to_bsr_index(int32_t nbytes)
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
/*
 * Add ue info in eNB_ulsch_info[module_idP][CC_id][UE_id] struct
 */
void
add_ue_ulsch_info(module_id_t module_idP,
                  int CC_id,
                  int UE_id,
                  sub_frame_t subframeP,
                  UE_ULSCH_STATUS status)
//-----------------------------------------------------------------------------
{
  eNB_ulsch_info[module_idP][CC_id][UE_id].rnti     = UE_RNTI(module_idP, UE_id);
  eNB_ulsch_info[module_idP][CC_id][UE_id].subframe = subframeP;
  eNB_ulsch_info[module_idP][CC_id][UE_id].status   = status;
  eNB_ulsch_info[module_idP][CC_id][UE_id].serving_num++;
}

//-----------------------------------------------------------------------------
/*
 * Parse MAC header from ULSCH
 */
unsigned char *
parse_ulsch_header(unsigned char *mac_header,
                   unsigned char *num_ce,
                   unsigned char *num_sdu,
                   unsigned char *rx_ces,
                   unsigned char *rx_lcids,
                   unsigned short *rx_lengths,
                   unsigned short tb_length)
//-----------------------------------------------------------------------------
{
  unsigned char not_done = 1;
  unsigned char num_ces = 0;
  unsigned char num_sdus = 0;
  unsigned char lcid = 0;
  unsigned char num_sdu_cnt = 0;
  unsigned char *mac_header_ptr = NULL;
  unsigned short length, ce_len = 0;
  /* Init */
  mac_header_ptr = mac_header;

  while (not_done == 1) {
    if (((SCH_SUBHEADER_FIXED *) mac_header_ptr)->E == 0) {
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->LCID;

    if (lcid < EXTENDED_POWER_HEADROOM) {
      if (not_done == 0) {  // last MAC SDU, length is implicit
        mac_header_ptr++;
        length = tb_length - (mac_header_ptr - mac_header) - ce_len;

        for (num_sdu_cnt = 0; num_sdu_cnt < num_sdus; num_sdu_cnt++) {
          length -= rx_lengths[num_sdu_cnt];
        }
      } else {
        if (((SCH_SUBHEADER_SHORT *) mac_header_ptr)->F == 0) {
          length = ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L;
          mac_header_ptr += 2;  //sizeof(SCH_SUBHEADER_SHORT);
        } else {  // F = 1
          length = ((((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_MSB & 0x7f) << 8) |
                   (((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_LSB & 0xff);
          mac_header_ptr += 3;  //sizeof(SCH_SUBHEADER_LONG);
        }
      }

      LOG_D(MAC, "[eNB] sdu %d lcid %d tb_length %d length %d (offset now %ld)\n",
            num_sdus,
            lcid,
            tb_length,
            length,
            mac_header_ptr - mac_header);
      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else {  // This is a control element subheader POWER_HEADROOM, BSR and CRNTI
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
        } else if ((lcid == POWER_HEADROOM) || (lcid == TRUNCATED_BSR) || (lcid == SHORT_BSR)) {
          ce_len++;
        } else {
          LOG_E(MAC, "unknown CE %d \n", lcid);
          return NULL;
        }
      }
    }
  }

  *num_ce = num_ces;
  *num_sdu = num_sdus;
  return (mac_header_ptr);
}

//-----------------------------------------------------------------------------
/* This function is called by PHY layer when it schedules some
 * uplink for a random access message 3.
 * The MAC scheduler has to skip the RBs used by this message 3
 * (done below in schedule_ulsch).
 * This function seems to be unused, the Msg3_subframe is set somewhere else...
 * In NFAPI??
 */
void
set_msg3_subframe(module_id_t mod_id,
                  int CC_id,
                  int frame, // Not used, remove?
                  int subframe, // Not used, remove?
                  int rnti,
                  int Msg3_frame, // Not used, remove?
                  int Msg3_subframe)
//-----------------------------------------------------------------------------
{
  int RA_id = 0;
  /* Init */
  RA_id = find_RA_id(mod_id, CC_id, rnti); // state == WAITMSG3 instead of state != IDLE (?)

  if (RA_id != -1) {
    RC.mac[mod_id]->common_channels[CC_id].ra[RA_id].Msg3_subframe = Msg3_subframe;
  } else {
    LOG_E(MAC, "[MAC] Unknown RAPROC associated to RNTI %x\n", rnti);
  }

  return;
}

//-----------------------------------------------------------------------------
/*
 * Main function called for uplink scheduling (DCI0).
 */
void
schedule_ulsch(module_id_t module_idP,
               frame_t frameP,
               sub_frame_t subframeP)
//-----------------------------------------------------------------------------
{
  uint16_t first_rb[NFAPI_CC_MAX];
  eNB_MAC_INST *mac = NULL;
  slice_info_t *sli = NULL;
  COMMON_channels_t *cc = NULL;
  int sched_subframe;
  int sched_frame;

  /* Init */
  mac = RC.mac[module_idP];
  sli = &(mac->slice_info);
  memset(first_rb, 0, NFAPI_CC_MAX * sizeof(uint16_t));
  start_meas(&(mac->schedule_ulsch));

  sched_subframe = (subframeP + 4) % 10;
  sched_frame = frameP;
  cc = mac->common_channels;

  /* For TDD: check subframes where we have to act and return if nothing should be done now */
  if (cc->tdd_Config) {  // Done only for CC_id = 0, assume tdd_Config for all CC_id
    int tdd_sfa = cc->tdd_Config->subframeAssignment;

    switch (subframeP) {
      case 0:
        if ((tdd_sfa == 0) || (tdd_sfa == 3))
          sched_subframe = 4;
        else if (tdd_sfa == 6)
          sched_subframe = 7;
        else
          return;

        break;

      case 1:
        if ((tdd_sfa == 0) || (tdd_sfa == 1))
          sched_subframe = 7;
        else if (tdd_sfa == 6)
          sched_subframe = 8;
        else
          return;

        break;

      case 2:  // Don't schedule UL in subframe 2 for TDD
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
          sched_subframe = 2;
        else
          return;

        break;

      case 6:
        if (tdd_sfa == 0 || tdd_sfa == 1)
          sched_subframe = 2;
        else if (tdd_sfa == 6)
          sched_subframe = 3;
        else
          return;

        break;

      case 7:
        return;

      case 8:
        if ((tdd_sfa >= 2) && (tdd_sfa <= 5))
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

      default:
        return;
    }
  }

  if (sched_subframe < subframeP) {
    sched_frame++;
    sched_frame %= 1024;
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  int emtc_active[5];
  memset(emtc_active, 0, 5 * sizeof(int));
  schedule_ulsch_rnti_emtc(module_idP, frameP, subframeP, sched_subframe, emtc_active);
#endif

  /* Note: RC.nb_mac_CC[module_idP] should be lower than or equal to NFAPI_CC_MAX */
  for (int CC_id = 0; CC_id < RC.nb_mac_CC[module_idP]; CC_id++, cc++) {
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
    first_rb[CC_id] = (emtc_active[CC_id] == 1) ? 7 : 1;
#else
    /* Note: the size of PUCCH is arbitrary, to be done properly. */
    switch (RC.eNB[module_idP][CC_id]->frame_parms.N_RB_DL) {
    case 25:  first_rb[CC_id] = 1; break; // leave out first RB for PUCCH
    case 50:  first_rb[CC_id] = 2; break; // leave out first RB for PUCCH
    case 100: first_rb[CC_id] = 3; break; // leave out first RB for PUCCH
    default: LOG_E(MAC, "nb RBs not handled, todo.\n"); exit(1);
    }
#endif
    RA_t *ra_ptr = cc->ra;

    /* From Louis-Adrien to François:
     * The comment bloc below is to configure with a command line.
     * I took it from the equivalent part in the fairRR scheduler (around line 2578 in eNB_scheduler_fairRR.c).
     * As said in the meeting, it seems to work only for small TBS.
     * The cause of false RA still present with this fix is to investigate.
     *
     * Note: in the get_prach_prb_offset() function below, the last argument is frameP in eNB_scheduler_fairRR.c
     * I think it should be sched_frame instead. This parameter has only impacts in case TDD and preamble format 4.
     * To confirm.
     */
    /*
    int start_rb = 0;
    int nb_rb = 6;
    LTE_DL_FRAME_PARMS *frame_parms = &(RC.eNB[module_idP][CC_id]->frame_parms);

    if (is_prach_subframe(frame_parms, sched_frame, sched_subframe) == 1) {
      start_rb = get_prach_prb_offset(frame_parms,
                                      frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
                                      frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset,
                                      0, // tdd_mapindex
                                      sched_frame); // Nf

      first_rb[CC_id] = start_rb + nb_rb;
    }
    */

    /*
     * Check if RA (Msg3) is active in this subframeP, if so skip the PRB used for Msg3
     * Msg3 is using 1 PRB so we need to increase first_rb accordingly
     * Not sure about the break (can there be more than 1 active RA procedure per CC_id and per subframe?)
     */
    for (int ra_index = 0; ra_index < NB_RA_PROC_MAX; ra_index++, ra_ptr++) {
      if ((ra_ptr->state == WAITMSG3) && (ra_ptr->Msg3_subframe == sched_subframe)) {
        if (first_rb[CC_id] < ra_ptr->msg3_first_rb + ra_ptr->msg3_nb_rb) {
          first_rb[CC_id] = ra_ptr->msg3_first_rb + ra_ptr->msg3_nb_rb;
        }

        /* Louis-Adrien: I couldn't find an interdiction of multiple Msg3 scheduling
         * on the same time resources. Also the performance improvement of breaking is low,
         * since we will loop until the end, most of the time.
         * I'm letting the break as a reminder, in case of misunderstanding the spec.
         */
        // break;
      }
    }
  }

  /* Run each enabled slice-specific schedulers one by one */
  for (int i = 0; i < sli->n_ul; i++) {
    /* By default the scheduler is schedule_ulsch_rnti (see below) */
    sli->ul[i].sched_cb(module_idP, i, frameP, subframeP, sched_subframe, first_rb);
  }

  stop_meas(&mac->schedule_ulsch);
}

//-----------------------------------------------------------------------------
/*
* Schedule the DCI0 for ULSCH
*/
void
schedule_ulsch_rnti(module_id_t   module_idP,
                    int           slice_idx,
                    frame_t       frameP,
                    sub_frame_t   subframeP,
                    unsigned char sched_subframeP,
                    uint16_t      *first_rb)
//-----------------------------------------------------------------------------
{
  rnti_t rnti = -1;
  uint8_t aggregation = 2;
  uint8_t round_index = 0;
  uint8_t harq_pid = 0;
  uint8_t status = 0;
  uint8_t rb_table_index = -1;
  uint8_t dlsch_flag = 0;
  uint16_t ul_req_index = 0;
  uint32_t cqi_req = 0;
  uint32_t cshift = 0;
  uint32_t ndi = 0;
  uint32_t tpc = 0;
  int32_t normalized_rx_power = 0;
  int32_t target_rx_power = 0;
  int32_t framex10psubframe = 0;
  static int32_t tpc_accumulated = 0;
  int sched_frame = 0;
  int CC_id = 0;
  eNB_MAC_INST *mac = NULL;
  COMMON_channels_t *cc = NULL;
  UE_list_t *UE_list = NULL;
  slice_info_t *sli = NULL;
  UE_TEMPLATE *UE_template_ptr = NULL;
  UE_sched_ctrl *UE_sched_ctrl_ptr = NULL;
  int rvidx_tab[4] = {0, 2, 3, 1};
  int first_rb_slice[NFAPI_CC_MAX];
  int n_rb_ul_tab[NFAPI_CC_MAX];
  /* Init */
  mac = RC.mac[module_idP];
  cc = mac->common_channels;
  UE_list = &(mac->UE_list);
  sli = &(mac->slice_info);
  memset(first_rb_slice, 0, NFAPI_CC_MAX * sizeof(int));
  memset(n_rb_ul_tab, 0, NFAPI_CC_MAX * sizeof(int));
  sched_frame = frameP;

  if (sched_subframeP < subframeP) {
    sched_frame++;
    sched_frame %= 1024;
  }

  /* NFAPI struct init */
  nfapi_hi_dci0_request_t        *hi_dci0_req      = &(mac->HI_DCI0_req[CC_id][subframeP]);
  nfapi_hi_dci0_request_body_t   *hi_dci0_req_body = &(hi_dci0_req->hi_dci0_request_body);
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;
  nfapi_ul_config_request_t      *ul_req_tmp       = &(mac->UL_req_tmp[CC_id][sched_subframeP]);
  nfapi_ul_config_request_body_t *ul_req_tmp_body  = &(ul_req_tmp->ul_config_request_body);
  nfapi_ul_config_ulsch_harq_information *ulsch_harq_information;
  hi_dci0_req->sfn_sf = (frameP << 4) + subframeP;

  /* Note: RC.nb_mac_CC[module_idP] should be lower than or equal to NFAPI_CC_MAX */
  for (CC_id = 0; CC_id < RC.nb_mac_CC[module_idP]; CC_id++) {
    n_rb_ul_tab[CC_id] = to_prb(cc[CC_id].ul_Bandwidth); // return total number of PRB
    /* HACK: let's remove the PUCCH from available RBs
     * we suppose PUCCH size is:
     * - for 25 RBs: 1 RB (top and bottom of ressource grid)
     * - for 50:     2 RBs
     * - for 100:    3 RBs
     * This is totally arbitrary and might even be wrong.
     * We suppose 'first_rb[]' has been correctly populated by the caller,
     * so we only remove the top part of the resource grid.
     */
    switch (n_rb_ul_tab[CC_id]) {
    case 25:  n_rb_ul_tab[CC_id] -= 1; break;
    case 50:  n_rb_ul_tab[CC_id] -= 2; break;
    case 100: n_rb_ul_tab[CC_id] -= 3; break;
    default: LOG_E(MAC, "RBs setting not handled. Todo.\n"); exit(1);
    }
    UE_list->first_rb_offset[CC_id][slice_idx] = cmin(n_rb_ul_tab[CC_id], sli->ul[slice_idx].first_rb);
  }

  /* 
   * ULSCH preprocessor: set UE_template->
   * pre_allocated_nb_rb_ul[slice_idx]
   * pre_assigned_mcs_ul
   * pre_allocated_rb_table_index_ul
   */
  ulsch_scheduler_pre_processor(module_idP, slice_idx, frameP, subframeP, sched_frame, sched_subframeP, first_rb);

  for (CC_id = 0; CC_id < RC.nb_mac_CC[module_idP]; CC_id++) {
    first_rb_slice[CC_id] = first_rb[CC_id] + UE_list->first_rb_offset[CC_id][slice_idx];
  }

  // loop over all active UEs until end of function
  for (int UE_id = UE_list->head_ul; UE_id >= 0; UE_id = UE_list->next_ul[UE_id]) {
    if (!ue_ul_slice_membership(module_idP, UE_id, slice_idx)) {
      continue;
    }
    
    if (UE_list->UE_template[UE_PCCID(module_idP, UE_id)][UE_id].rach_resource_type > 0)  continue;

    // don't schedule if Msg5 is not received yet
    if (UE_list->UE_template[UE_PCCID(module_idP, UE_id)][UE_id].configured == FALSE) {
      LOG_D(MAC, "[eNB %d] frame %d, subframe %d, UE %d: not configured, skipping UE scheduling \n",
            module_idP,
            frameP,
            subframeP,
            UE_id);
      continue;
    }

    rnti = UE_RNTI(module_idP, UE_id);

    if (rnti == NOT_A_RNTI) {
      LOG_W(MAC, "[eNB %d] frame %d, subframe %d, UE %d: no RNTI \n",
            module_idP,
            frameP,
            subframeP,
            UE_id);
      continue;
    }

    // loop over all active UL CC_ids for this UE until end of function
    for (int n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      /* This is the actual CC_id in the list */
      CC_id = UE_list->ordered_ULCCids[n][UE_id];

      UE_template_ptr = &(UE_list->UE_template[CC_id][UE_id]);
      UE_sched_ctrl_ptr = &(UE_list->UE_sched_ctrl[UE_id]);
      harq_pid = subframe2harqpid(&cc[CC_id], sched_frame, sched_subframeP);
      round_index = UE_sched_ctrl_ptr->round_UL[CC_id][harq_pid];
      AssertFatal(round_index < 8, "round %d > 7 for UE %d/%x\n",
                  round_index,
                  UE_id,
                  rnti);
      LOG_D(MAC, "[eNB %d] frame %d subframe %d (sched_frame %d, sched_subframe %d), Checking PUSCH %d for UE %d/%x CC %d : aggregation level %d, N_RB_UL %d\n",
            module_idP,
            frameP,
            subframeP,
            sched_frame,
            sched_subframeP,
            harq_pid,
            UE_id,
            rnti,
            CC_id,
            aggregation,
            n_rb_ul_tab[CC_id]);
      /* Seems unused, only for debug */
      RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP * 10) + subframeP] = UE_template_ptr->estimated_ul_buffer;
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BO, UE_template_ptr->estimated_ul_buffer);

      /*
       * If there is information on BSR of DCCH, DTCH or if there is UL_SR,
       * or if there is a packet to retransmit, or we want to schedule a periodic feedback
       */
      if (UE_is_to_be_scheduled(module_idP, CC_id, UE_id) > 0 || round_index > 0) {
        LOG_D(MAC, "[eNB %d][PUSCH %d] Frame %d subframe %d Scheduling UE %d/%x in round %d(SR %d,UL_inactivity timer %d,UL_failure timer %d,cqi_req_timer %d)\n",
              module_idP,
              harq_pid,
              frameP,
              subframeP,
              UE_id,
              rnti,
              round_index,
              UE_template_ptr->ul_SR,
              UE_sched_ctrl_ptr->ul_inactivity_timer,
              UE_sched_ctrl_ptr->ul_failure_timer,
              UE_sched_ctrl_ptr->cqi_req_timer);

        /* Reset the scheduling request */
        UE_template_ptr->ul_SR = 0;
        status = mac_eNB_get_rrc_status(module_idP, rnti);

        /* New transmission */
        if (round_index == 0) {
          /* Be sure that there are some free RBs */
          if (first_rb_slice[CC_id] >= n_rb_ul_tab[CC_id]) {
            LOG_W(MAC, "[eNB %d] frame %d, subframe %d, UE %d/%x CC %d: dropping, not enough RBs\n",
              module_idP,
              frameP,
              subframeP,
              UE_id,
              rnti,
              CC_id);

            continue;
          }

          /* Should format_flag be 2 in CCE_allocation_infeasible??? */
          /* This test seems to be way too long, can we provide an optimization? */
          if (CCE_allocation_infeasible(module_idP, CC_id, 1, subframeP, aggregation, rnti)) {
            LOG_W(MAC, "[eNB %d] frame %d, subframe %d, UE %d/%x CC %d: not enough CCE\n",
              module_idP,
              frameP,
              subframeP,
              UE_id,
              rnti,
              CC_id);

            continue;
          }
          
          /* Handle the aperiodic CQI report */
          cqi_req = 0;

          if (status >= RRC_CONNECTED && UE_sched_ctrl_ptr->cqi_req_timer > 30) {
            if (UE_sched_ctrl_ptr->cqi_received == 0) {
              if (NFAPI_MODE != NFAPI_MONOLITHIC) {
                cqi_req = 0;
              } else {
                cqi_req = 1;

                /* TDD: to be safe, do not ask CQI in special Subframes:36.213/7.2.3 CQI definition */
                if (cc[CC_id].tdd_Config) {
                  switch (cc[CC_id].tdd_Config->subframeAssignment) {
                    case 1:
                      if(subframeP == 1 || subframeP == 6) {
                        cqi_req=0;
                      }
                      break;

                    case 3:
                      if(subframeP == 1) {
                        cqi_req=0;
                      }
                      break;

                    default:
                      LOG_E(MAC," TDD config not supported\n");
                      break;
                  }
                }

                if(cqi_req == 1) {
                  UE_sched_ctrl_ptr->cqi_req_flag |= 1 << sched_subframeP;
                }
              }
            } else {
              UE_sched_ctrl_ptr->cqi_req_flag = 0;
              UE_sched_ctrl_ptr->cqi_received = 0;
              UE_sched_ctrl_ptr->cqi_req_timer = 0;
            }
          }

          /* Power control */
          /*
           * Compute the expected ULSCH RX power (for the stats)
           * This is the normalized RX power and this should be constant (regardless of mcs)
           * Is not in dBm, unit from nfapi, converting to dBm
           * ToDo: Noise power hard coded to 30
           */
          normalized_rx_power = ((5 * UE_sched_ctrl_ptr->pusch_snr[CC_id] - 640) / 10) + 30;
          target_rx_power = (mac->puSch10xSnr / 10) + 30;

          /*
           * This assumes accumulated tpc
           * Make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
           */
          framex10psubframe = (UE_template_ptr->pusch_tpc_tx_frame * 10) + UE_template_ptr->pusch_tpc_tx_subframe;

          if (((framex10psubframe + 10) <= (frameP * 10 + subframeP)) || // normal case
              ((framex10psubframe > (frameP * 10 + subframeP)) && (((10240 - framex10psubframe + frameP * 10 + subframeP) >= 10)))) { //frame wrap-around

            UE_template_ptr->pusch_tpc_tx_frame = frameP;
            UE_template_ptr->pusch_tpc_tx_subframe = subframeP;

            if (normalized_rx_power > (target_rx_power + 4)) {
              tpc = 0; // -1
              tpc_accumulated--;
            } else if (normalized_rx_power < (target_rx_power - 4)) {
              tpc = 2; // +1
              tpc_accumulated++;
            } else {
              tpc = 1; // 0
            }
          } else {
            tpc = 1; // 0
          }

          if (tpc != 1) {
            LOG_D(MAC, "[eNB %d] ULSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
              module_idP,
              frameP,
              subframeP,
              harq_pid,
              tpc,
              tpc_accumulated,
              normalized_rx_power,
              target_rx_power);
          }

          ndi = 1 - UE_template_ptr->oldNDI_UL[harq_pid]; // NDI: new data indicator
          UE_template_ptr->oldNDI_UL[harq_pid] = ndi;
          UE_list->eNB_UE_stats[CC_id][UE_id].normalized_rx_power = normalized_rx_power;
          UE_list->eNB_UE_stats[CC_id][UE_id].target_rx_power = target_rx_power;
          UE_template_ptr->mcs_UL[harq_pid] = cmin(UE_template_ptr->pre_assigned_mcs_ul, sli->ul[slice_idx].maxmcs);
          UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1= UE_template_ptr->mcs_UL[harq_pid];

          /* CDRX */
          if (UE_sched_ctrl_ptr->cdrx_configured) {
            UE_sched_ctrl_ptr->drx_inactivity_timer = 1; // reset drx inactivity timer when new transmission
            VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_INACTIVITY, (unsigned long) UE_sched_ctrl_ptr->drx_inactivity_timer);
            UE_sched_ctrl_ptr->dci0_ongoing_timer = 1; // when set the UE_template_ptr->ul_SR cannot be set to 1, 
                                                       // see definition for more information
          }

          if (UE_template_ptr->pre_allocated_rb_table_index_ul >= 0) {
            rb_table_index = UE_template_ptr->pre_allocated_rb_table_index_ul;
          } else {
            UE_template_ptr->mcs_UL[harq_pid] = 10;
            rb_table_index = 5; // for PHR
          }

          UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2 = UE_template_ptr->mcs_UL[harq_pid];

          while (((rb_table[rb_table_index] > (n_rb_ul_tab[CC_id] - first_rb_slice[CC_id])) ||
                  (rb_table[rb_table_index] > 45)) && (rb_table_index > 0)) {
            rb_table_index--;
          }

          UE_template_ptr->TBS_UL[harq_pid] = get_TBS_UL(UE_template_ptr->mcs_UL[harq_pid], rb_table[rb_table_index]);
          UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx += rb_table[rb_table_index];
          UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS = UE_template_ptr->TBS_UL[harq_pid];
          UE_list->eNB_UE_stats[CC_id][UE_id].total_ulsch_TBS += UE_template_ptr->TBS_UL[harq_pid];
          T(T_ENB_MAC_UE_UL_SCHEDULE,
            T_INT(module_idP),
            T_INT(CC_id),
            T_INT(rnti),
            T_INT(frameP),
            T_INT(subframeP),
            T_INT(harq_pid),
            T_INT(UE_template_ptr->mcs_UL[harq_pid]),
            T_INT(first_rb_slice[CC_id]),
            T_INT(rb_table[rb_table_index]),
            T_INT(UE_template_ptr->TBS_UL[harq_pid]),
            T_INT(ndi));
          /* Store information for possible retransmission */
          UE_template_ptr->nb_rb_ul[harq_pid] = rb_table[rb_table_index];
          UE_template_ptr->first_rb_ul[harq_pid] = first_rb_slice[CC_id];
          UE_template_ptr->cqi_req[harq_pid] = cqi_req;
          UE_sched_ctrl_ptr->ul_scheduled |= (1 << harq_pid);

          if (UE_id == UE_list->head) {
            VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SCHEDULED, UE_sched_ctrl_ptr->ul_scheduled);
          }

          /* Adjust scheduled UL bytes by TBS, wait for UL sdus to do final update */
          LOG_D(MAC, "[eNB %d] CC_id %d UE %d/%x : adjusting scheduled_ul_bytes, old %d, TBS %d\n",
                module_idP,
                CC_id,
                UE_id,
                rnti,
                UE_template_ptr->scheduled_ul_bytes,
                UE_template_ptr->TBS_UL[harq_pid]);
          UE_template_ptr->scheduled_ul_bytes += UE_template_ptr->TBS_UL[harq_pid];
          LOG_D(MAC, "scheduled_ul_bytes, new %d\n",
                UE_template_ptr->scheduled_ul_bytes);
          /* Cyclic shift for DM-RS */
          cshift = 0; // values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
          /* Save it for a potential retransmission */
          UE_template_ptr->cshift[harq_pid] = cshift;
          /* Setting DCI0 NFAPI struct */
          hi_dci0_pdu = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
          memset((void *) hi_dci0_pdu, 0,sizeof(nfapi_hi_dci0_request_pdu_t));
          hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_DCI_PDU_TYPE;
          hi_dci0_pdu->pdu_size = 2 + sizeof(nfapi_hi_dci0_dci_pdu);
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dci_format = NFAPI_UL_DCI_FORMAT_0;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti = rnti;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.transmission_power = 6000;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.resource_block_start = first_rb_slice[CC_id];
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.number_of_resource_block = rb_table[rb_table_index];
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.mcs_1 = UE_template_ptr->mcs_UL[harq_pid];
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms = cshift;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag = 0;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.new_data_indication_1 = ndi;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tpc = tpc;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cqi_csi_request = cqi_req;
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dl_assignment_index = UE_template_ptr->DAI_ul[sched_subframeP];
          hi_dci0_pdu->dci_pdu.dci_pdu_rel8.harq_pid = harq_pid;
          hi_dci0_req_body->number_of_dci++;
          hi_dci0_req_body->sfnsf = sfnsf_add_subframe(sched_frame, sched_subframeP, 0);
          hi_dci0_req_body->tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
          hi_dci0_req->sfn_sf = frameP << 4 | subframeP;
          hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;
          LOG_D(MAC, "[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d\n",
                harq_pid,
                frameP,
                subframeP,
                UE_id,
                rnti,
                sched_frame,
                sched_subframeP);
          ul_req_index = 0;
          dlsch_flag = 0;

          for(ul_req_index = 0; ul_req_index < ul_req_tmp_body->number_of_pdus; ul_req_index++) {
            if(ul_req_tmp_body->ul_config_pdu_list[ul_req_index].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE &&
                ul_req_tmp_body->ul_config_pdu_list[ul_req_index].uci_harq_pdu.ue_information.ue_information_rel8.rnti == rnti) {
              dlsch_flag = 1;
              LOG_D(MAC, "Frame %d, Subframe %d:rnti %x ul_req_index %d Switched UCI HARQ to ULSCH HARQ(first)\n",
                    frameP,
                    subframeP,
                    rnti,
                    ul_req_index);
              break;
            }
          }

          /* Add UL_config PDUs */
          fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp_body->ul_config_pdu_list[ul_req_index],
                                               cqi_req,
                                               cc,
                                               UE_template_ptr->physicalConfigDedicated,
                                               get_tmode(module_idP, CC_id, UE_id),
                                               mac->ul_handle,
                                               rnti,
                                               first_rb_slice[CC_id],     // resource_block_start
                                               rb_table[rb_table_index],  // number_of_resource_blocks
                                               UE_template_ptr->mcs_UL[harq_pid],
                                               cshift,    // cyclic_shift_2_for_drms
                                               0,         // frequency_hopping_enabled_flag
                                               0,         // frequency_hopping_bits
                                               ndi,       // new_data_indication
                                               0,         // redundancy_version
                                               harq_pid,  // harq_process_number
                                               0,         // ul_tx_mode
                                               0,         // current_tx_nb
                                               0,         // n_srs
                                               get_TBS_UL(UE_template_ptr->mcs_UL[harq_pid], rb_table[rb_table_index]));
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

          /* This is a BL/CE UE allocation */
          if (UE_template_ptr->rach_resource_type > 0) {
            fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp_body->ul_config_pdu_list[ul_req_index],
                                                 UE_template_ptr->rach_resource_type > 2 ? 2 : 1,
                                                 1,  // total_number_of_repetitions
                                                 1,  // repetition_number
                                                 (frameP * 10) + subframeP);
          }
#endif

          if (dlsch_flag == 1) {
            if (cqi_req == 1) {
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE;
              ulsch_harq_information = &ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.harq_information;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag =
                NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;  // last symbol not punctured
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks =
                rb_table[rb_table_index];
            } else {
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE;
              ulsch_harq_information = &ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.harq_information;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag =
                NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;  // last symbol not punctured
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks = rb_table[rb_table_index];
            }

            fill_nfapi_ulsch_harq_information(module_idP, CC_id,rnti, ulsch_harq_information,subframeP);
          } else {
            ul_req_tmp_body->number_of_pdus++;
          }

          ul_req_tmp->header.message_id = NFAPI_UL_CONFIG_REQUEST;
          ul_req_tmp_body->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
          mac->ul_handle++;
          ul_req_tmp->sfn_sf = sched_frame << 4 | sched_subframeP;
          add_ue_ulsch_info(module_idP, CC_id, UE_id, subframeP, S_UL_SCHEDULED);
          LOG_D(MAC, "[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0\n",
                module_idP,
                CC_id,
                frameP,
                subframeP,
                UE_id);
          LOG_D(MAC, "[PUSCH %d] SFN/SF:%04d%d UL_CFG:SFN/SF:%04d%d CQI:%d for UE %d/%x\n",
                harq_pid,
                frameP,
                subframeP,
                sched_frame,
                sched_subframeP,
                cqi_req,
                UE_id,
                rnti);
          /* Increment first rb for next UE allocation */
          first_rb_slice[CC_id] += rb_table[rb_table_index];
        } else { // round_index > 0 => retransmission
          T(T_ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION,
            T_INT(module_idP),
            T_INT(CC_id),
            T_INT(rnti),
            T_INT(frameP),
            T_INT(subframeP),
            T_INT(harq_pid),
            T_INT(UE_template_ptr->mcs_UL[harq_pid]),
            T_INT(UE_template_ptr->first_rb_ul[harq_pid]),
            T_INT(UE_template_ptr->nb_rb_ul[harq_pid]),
            T_INT(round_index));
          /* Add UL_config PDUs */
          LOG_D(MAC, "[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d\n",
                harq_pid,
                frameP,
                subframeP,
                UE_id,
                rnti,
                sched_frame,
                sched_subframeP);
          ul_req_index = 0;
          dlsch_flag = 0;
          cqi_req = UE_template_ptr->cqi_req[harq_pid];

          for(ul_req_index = 0; ul_req_index < ul_req_tmp_body->number_of_pdus; ul_req_index++) {
            if(ul_req_tmp_body->ul_config_pdu_list[ul_req_index].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE &&
                ul_req_tmp_body->ul_config_pdu_list[ul_req_index].uci_harq_pdu.ue_information.ue_information_rel8.rnti == rnti) {
              dlsch_flag = 1;
              LOG_D(MAC, "Frame %d, Subframe %d:rnti %x ul_req_index %d Switched UCI HARQ to ULSCH HARQ(first)\n",
                    frameP,
                    subframeP,
                    rnti,
                    ul_req_index);
              break;
            }
          }

          fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp_body->ul_config_pdu_list[ul_req_index],
                                               cqi_req,
                                               cc,
                                               UE_template_ptr->physicalConfigDedicated,
                                               get_tmode(module_idP, CC_id, UE_id),
                                               mac->ul_handle,
                                               rnti,
                                               UE_template_ptr->first_rb_ul[harq_pid], // resource_block_start
                                               UE_template_ptr->nb_rb_ul[harq_pid],    // number_of_resource_blocks
                                               UE_template_ptr->mcs_UL[harq_pid],
                                               cshift, // cyclic_shift_2_for_drms
                                               0,      // frequency_hopping_enabled_flag
                                               0,      // frequency_hopping_bits
                                               UE_template_ptr->oldNDI_UL[harq_pid], // new_data_indication
                                               rvidx_tab[round_index & 3],           // redundancy_version
                                               harq_pid,  // harq_process_number
                                               0, // ul_tx_mode
                                               0, // current_tx_nb
                                               0, // n_srs
                                               UE_template_ptr->TBS_UL[harq_pid]);
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
          /* This is a BL/CE UE allocation */
          if (UE_template_ptr->rach_resource_type > 0) {
            fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp_body->ul_config_pdu_list[ul_req_index],
                                                 UE_template_ptr->rach_resource_type > 2 ? 2 : 1,
                                                 1, // total_number_of_repetitions
                                                 1, // repetition_number
                                                 (frameP * 10) + subframeP);
          }
#endif

          if(dlsch_flag == 1) {
            if(cqi_req == 1) {
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE;
              ulsch_harq_information = &ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.harq_information;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag=
                NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;    // last symbol not punctured
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks =
                UE_template_ptr->nb_rb_ul[harq_pid];
            } else {
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE;
              ulsch_harq_information = &ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.harq_information;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag =
                NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;  // last symbol not punctured
              ul_req_tmp_body->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks =
                UE_template_ptr->nb_rb_ul[harq_pid];
            }

            fill_nfapi_ulsch_harq_information(module_idP, CC_id,rnti, ulsch_harq_information, subframeP);
          } else {
            ul_req_tmp_body->number_of_pdus++;
          }

          mac->ul_handle++;
          ul_req_tmp_body->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
          ul_req_tmp->sfn_sf = sched_frame<<4|sched_subframeP;
          ul_req_tmp->header.message_id = NFAPI_UL_CONFIG_REQUEST;
          LOG_D(MAC, "[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d cqi_req %d\n",
            harq_pid,
            frameP,
            subframeP,
            UE_id,
            rnti,
            sched_frame,
            sched_subframeP,
            cqi_req);

          /* HACK: RBs used by retransmission have to be reserved.
           * The current mechanism uses the notion of 'first_rb', so
           * we skip all RBs below the ones retransmitted. This is
           * not correct. Imagine only RB 23 is retransmitted, then all
           * RBs < 23 will be marked unusable for new transmissions (case where round == 0).
           * Note also that this code works only if the preprocessor orders
           * UEs with retransmission with higher priority than UEs with new
           * transmission.
           * All this should be cleaned up properly.
           */
          if (first_rb_slice[CC_id] < UE_template_ptr->first_rb_ul[harq_pid] + UE_template_ptr->nb_rb_ul[harq_pid])
            first_rb_slice[CC_id] = UE_template_ptr->first_rb_ul[harq_pid] + UE_template_ptr->nb_rb_ul[harq_pid];
        }  // end of round > 0 
      }  // UE_is_to_be_scheduled
    }  // loop over all active CC_ids
  }  // loop over UE_ids
}

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
//-----------------------------------------------------------------------------
/*
 * default ULSCH scheduler for LTE-M
 */
void schedule_ulsch_rnti_emtc(module_id_t   module_idP,
                              frame_t       frameP,
                              sub_frame_t   subframeP,
                              unsigned char sched_subframeP,
                              int          *emtc_active)
//-----------------------------------------------------------------------------
{
  int               UE_id          = -1;
  rnti_t            rnti           = -1;
  uint8_t           round_UL       = 0;
  uint8_t           harq_pid       = 0;
  uint8_t           status         = 0;
  uint32_t          cshift         = 0;
  uint32_t          ndi            = 0;
  int32_t           normalized_rx_power = 0;
  int32_t           target_rx_power = -90;
  int               n       = 0;
  int               CC_id = 0;
  int               N_RB_UL = 0;
  int               sched_frame = frameP;
  int               rvidx_tab[4] = {0,2,3,1};
  int               tpc = 0;
  int               cqi_req = 0;
  eNB_MAC_INST      *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc  = eNB->common_channels;
  UE_list_t         *UE_list = &(eNB->UE_list);
  UE_TEMPLATE       *UE_template = NULL;
  UE_sched_ctrl     *UE_sched_ctrl = NULL;

  if (sched_subframeP < subframeP) {
    sched_frame++;
  }

  nfapi_hi_dci0_request_body_t   *hi_dci0_req = &(eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body);
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu = NULL;
  nfapi_ul_config_request_body_t *ul_req_tmp = &(eNB->UL_req_tmp[CC_id][sched_subframeP].ul_config_request_body);

  /* If frameP odd don't schedule */
  if ((frameP & 1) == 1) {
    return;
  }

  /* Loop over all active UEs */
  for (UE_id = UE_list->head_ul; UE_id >= 0; UE_id = UE_list->next_ul[UE_id]) {
    UE_template = &(UE_list->UE_template[UE_PCCID(module_idP, UE_id)][UE_id]);

    /* LTE-M device */
    if (UE_template->rach_resource_type == 0) {
      continue;
    }

    /* Don't schedule if Msg4 is not received yet */
    if (UE_template->configured == FALSE) {
      LOG_D(MAC,"[eNB %d] frame %d subframe %d, UE %d: not configured, skipping UE scheduling \n",
            module_idP,
            frameP,
            subframeP,
            UE_id);
      continue;
    }

    rnti = UE_RNTI(module_idP, UE_id);

    if (rnti == NOT_A_RNTI) {
      LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d: no RNTI \n",
            module_idP,
            frameP,
            subframeP,
            UE_id);
      continue;
    }

    /* Loop over all active UL CC_ids for this UE */
    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      /* This is the actual CC_id in the list */
      CC_id        = UE_list->ordered_ULCCids[n][UE_id];
      N_RB_UL      = to_prb(cc[CC_id].ul_Bandwidth);
      UE_template   = &(UE_list->UE_template[CC_id][UE_id]);
      UE_sched_ctrl = &UE_list->UE_sched_ctrl[UE_id];
      harq_pid      = 0;
      round_UL      = UE_sched_ctrl->round_UL[CC_id][harq_pid];
      AssertFatal(round_UL < 8,"round_UL %d > 7 for UE %d/%x\n",
                  round_UL,
                  UE_id,
                  rnti);
      LOG_D(MAC,"[eNB %d] frame %d subframe %d,Checking PUSCH %d for BL/CE UE %d/%x CC %d : aggregation level %d, N_RB_UL %d\n",
            module_idP,
            frameP,
            subframeP,
            harq_pid,
            UE_id,
            rnti,
            CC_id,
            24, // agregation level
            N_RB_UL);
      RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP] = UE_template->estimated_ul_buffer;
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BO, UE_template->estimated_ul_buffer);

      //if ((UE_is_to_be_scheduled(module_idP, CC_id, UE_id) > 0) && (subframeP == 5)) {
      if ((UE_template->ul_SR > 0 || round_UL > 0 || status < RRC_CONNECTED) && (subframeP == 5)) {
        /*
         * if there is information on bsr of DCCH, DTCH,
         * or if there is UL_SR,
         * or if there is a packet to retransmit,
         * or we want to schedule a periodic feedback every frame
         */
        LOG_D(MAC,"[eNB %d][PUSCH %d] Frame %d subframe %d Scheduling UE %d/%x in round_UL %d(SR %d,UL_inactivity timer %d,UL_failure timer %d,cqi_req_timer %d)\n",
              module_idP,
              harq_pid,
              frameP,
              subframeP,
              UE_id,
              rnti,
              round_UL,
              UE_template->ul_SR,
              UE_sched_ctrl->ul_inactivity_timer,
              UE_sched_ctrl->ul_failure_timer,
              UE_sched_ctrl->cqi_req_timer);
        /* Reset the scheduling request */
        emtc_active[CC_id] = 1;
        UE_template->ul_SR = 0;
        status = mac_eNB_get_rrc_status(module_idP,rnti);
        cqi_req = 0;
        /* Power control: compute the expected ULSCH RX power (for the stats) */
        /* This is the normalized RX power and this should be constant (regardless of mcs) */
        normalized_rx_power = UE_sched_ctrl->pusch_snr[CC_id];
        target_rx_power = 178;
        /* This assumes accumulated tpc */
        /* Make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out */
        int32_t framex10psubframe = UE_template->pusch_tpc_tx_frame * 10 + UE_template->pusch_tpc_tx_subframe;

        if (((framex10psubframe + 10) <= (frameP * 10 + subframeP)) || // normal case
            ((framex10psubframe > (frameP * 10 + subframeP)) && (((10240 - framex10psubframe + frameP * 10 + subframeP) >= 10)))) { // frame wrap-around
          UE_template->pusch_tpc_tx_frame = frameP;
          UE_template->pusch_tpc_tx_subframe = subframeP;

          if (normalized_rx_power > (target_rx_power + 4)) {
            tpc = 0; //-1
            UE_sched_ctrl->tpc_accumulated[CC_id]--;
          } else if (normalized_rx_power < (target_rx_power - 4)) {
            tpc = 2; //+1
            UE_sched_ctrl->tpc_accumulated[CC_id]++;
          } else {
            tpc = 1; //0
          }
        } else {
          tpc = 1; //0
        }

        if (tpc != 1) {
          LOG_D(MAC,"[eNB %d] ULSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, normalized/target rx power %d/%d\n",
                module_idP,
                frameP,
                subframeP,
                harq_pid,
                tpc,
                UE_sched_ctrl->tpc_accumulated[CC_id],
                normalized_rx_power,
                target_rx_power);
        }

        /* New transmission */
        if (round_UL == 0) {
          ndi = 1 - UE_template->oldNDI_UL[harq_pid];
          UE_template->oldNDI_UL[harq_pid] = ndi;
          UE_template->mcs_UL[harq_pid] = 4;
          UE_template->TBS_UL[harq_pid] = get_TBS_UL(UE_template->mcs_UL[harq_pid], 6);
          UE_list->eNB_UE_stats[CC_id][UE_id].normalized_rx_power = normalized_rx_power;
          UE_list->eNB_UE_stats[CC_id][UE_id].target_rx_power = target_rx_power;
          UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1 = 4;
          UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2 = UE_template->mcs_UL[harq_pid];
          UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx += 6;
          UE_list->eNB_UE_stats[CC_id][UE_id].ulsch_TBS = UE_template->TBS_UL[harq_pid];
          T(T_ENB_MAC_UE_UL_SCHEDULE,
            T_INT(module_idP),
            T_INT(CC_id),
            T_INT(rnti),
            T_INT(frameP),
            T_INT(subframeP),
            T_INT(harq_pid),
            T_INT(UE_template->mcs_UL[harq_pid]),
            T_INT(0),
            T_INT(6),
            T_INT(UE_template->TBS_UL[harq_pid]),
            T_INT(ndi));
          /* Store for possible retransmission */
          UE_template->nb_rb_ul[harq_pid]    = 6;
          UE_sched_ctrl->ul_scheduled |= (1 << harq_pid);

          if (UE_id == UE_list->head) {
            VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SCHEDULED, UE_sched_ctrl->ul_scheduled);
          }

          /* Adjust total UL buffer status by TBS, wait for UL sdus to do final update */
          UE_template->scheduled_ul_bytes += UE_template->TBS_UL[harq_pid];
          LOG_D(MAC, "scheduled_ul_bytes, new %d\n", UE_template->scheduled_ul_bytes);
          /* Cyclic shift for DMRS */
          cshift = 0; // values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
          /* save it for a potential retransmission */
          UE_template->cshift[harq_pid] = cshift;
          AssertFatal (UE_template->physicalConfigDedicated != NULL, "UE_template->physicalConfigDedicated is null\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4 != NULL, "UE_template->physicalConfigDedicated->ext4 is null\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11 != NULL, "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11 is null\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.present == LTE_EPDCCH_Config_r11__config_r11_PR_setup,
                       "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.present != setup\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 != NULL,
                       "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 = NULL\n");
          LTE_EPDCCH_SetConfig_r11_t *epdcch_setconfig_r11 = UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11->list.array[0];
          AssertFatal(epdcch_setconfig_r11 != NULL, "epdcch_setconfig_r11 is null\n");
          AssertFatal(epdcch_setconfig_r11->ext2 != NULL, "epdcch_setconfig_r11->ext2 is null\n");
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13 != NULL, "epdcch_setconfig_r11->ext2->mpdcch_config_r13 is null");
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13->present == LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13_PR_setup,
                      "epdcch_setconfig_r11->ext2->mpdcch_config_r13->present is not setup\n");
          AssertFatal(epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 != NULL, "epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 is null");
          AssertFatal(epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present == LTE_EPDCCH_SetConfig_r11__ext2__numberPRB_Pairs_v1310_PR_setup,
                      "epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present is not setup\n");
          LOG_D(MAC,"[PUSCH %d] Frame %d, Subframe %d: Adding UL 6-0A MPDCCH for BL/CE UE %d/%x, ulsch_frame %d, ulsch_subframe %d, UESS MPDCCH Narrowband %d\n",
                harq_pid,
                frameP,
                subframeP,
                UE_id,
                rnti,
                sched_frame,
                sched_subframeP,
                (int)epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1);
          UE_template->first_rb_ul[harq_pid] = narrowband_to_first_rb (cc, epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1);
          hi_dci0_pdu = &(hi_dci0_req->hi_dci0_pdu_list[hi_dci0_req->number_of_dci + hi_dci0_req->number_of_hi]);
          memset((void *) hi_dci0_pdu, 0, sizeof(nfapi_hi_dci0_request_pdu_t));
          hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE;
          hi_dci0_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_mpdcch_pdu));
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dci_format = (UE_template->rach_resource_type > 1) ? 5 : 4;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ce_mode = (UE_template->rach_resource_type > 1) ? 2 : 1;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mpdcch_narrowband = epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.number_of_prb_pairs = 6;       // checked above that it has to be this
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.resource_block_assignment = 0; // Note: this can be dynamic
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mpdcch_transmission_type = epdcch_setconfig_r11->transmissionType_r11;  // distibuted
          AssertFatal(UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 != NULL,
                      "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 is null\n");
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.start_symbol = *UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ecce_index = 0;        // Note: this should be dynamic
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.aggreagation_level = 24;        // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.rnti_type = 4; // other
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.rnti = rnti;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ce_mode = (UE_template->rach_resource_type < 3) ? 1 : 2; // already set above...
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.drms_scrambling_init = epdcch_setconfig_r11->dmrs_ScramblingSequenceInt_r11;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.initial_transmission_sf_io = (frameP * 10) + subframeP;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.transmission_power = 6000;     // 0dB
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.resource_block_start = UE_template->first_rb_ul[harq_pid];
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.number_of_resource_blocks = 6;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mcs = 4;       // adjust according to size of RAR, 208 bits with N1A_PRB = 3
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.pusch_repetition_levels = 0;
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_pdsch_HoppingConfig_r13 == LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__mpdcch_pdsch_HoppingConfig_r13_off,
                      "epdcch_setconfig_r11->ext2->mpdcch_config_r13->mpdcch_pdsch_HoppingConfig_r13 is not off\n");
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.frequency_hopping_flag = 1 - epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_pdsch_HoppingConfig_r13;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.redudency_version = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.new_data_indication = UE_template->oldNDI_UL[harq_pid];
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.harq_process = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.tpc = tpc;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.csi_request = cqi_req;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ul_inex = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dai_presence_flag = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dl_assignment_index = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.srs_request = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dci_subframe_repetition_number = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.tcp_bitmap = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.total_dci_length_include_padding = 29; // hard-coded for 10 MHz
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.number_of_tx_antenna_ports = 1;
          hi_dci0_req->number_of_dci++;
          LOG_D(MAC,"[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG. Request for BL/CE UE %d/%x, ulsch_frame %d, ulsch_subframe %d, UESS mpdcch narrowband %d\n",
                harq_pid,
                frameP,
                subframeP,
                UE_id,
                rnti,
                sched_frame,
                sched_subframeP,
                (int)epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1);
          fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp->ul_config_pdu_list[ul_req_tmp->number_of_pdus],
                                               cqi_req,
                                               cc,
                                               UE_template->physicalConfigDedicated,
                                               get_tmode(module_idP,CC_id,UE_id),
                                               eNB->ul_handle,
                                               rnti,
                                               UE_template->first_rb_ul[harq_pid], // resource_block_start
                                               UE_template->nb_rb_ul[harq_pid], // number_of_resource_blocks
                                               UE_template->mcs_UL[harq_pid],
                                               cshift, // cyclic_shift_2_for_drms
                                               0, // frequency_hopping_enabled_flag
                                               0, // frequency_hopping_bits
                                               UE_template->oldNDI_UL[harq_pid], // new_data_indication
                                               rvidx_tab[round_UL&3], // redundancy_version
                                               harq_pid, // harq_process_number
                                               0, // ul_tx_mode
                                               0, // current_tx_nb
                                               0, // n_srs
                                               UE_template->TBS_UL[harq_pid]
                                              );
          fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp->ul_config_pdu_list[ul_req_tmp->number_of_pdus],
                                               UE_template->rach_resource_type > 2 ? 2 : 1,
                                               1, // total_number_of_repetitions
                                               1, // repetition_number
                                               (frameP * 10) + subframeP);
          ul_req_tmp->number_of_pdus++;
          eNB->ul_handle++;
          add_ue_ulsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_UL_SCHEDULED);
          LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0\n",
                module_idP,
                CC_id,
                frameP,
                subframeP,
                UE_id);
        } else { // round_UL > 0 => retransmission
          /* In LTE-M the UL HARQ process is asynchronous */
          T(T_ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION,
            T_INT(module_idP),
            T_INT(CC_id),
            T_INT(rnti),
            T_INT(frameP),
            T_INT(subframeP),
            T_INT(harq_pid),
            T_INT(UE_template->mcs_UL[harq_pid]),
            T_INT(0),
            T_INT(6),
            T_INT(round_UL));
          AssertFatal (UE_template->physicalConfigDedicated != NULL, "UE_template->physicalConfigDedicated is null\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4 != NULL, "UE_template->physicalConfigDedicated->ext4 is null\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11 != NULL, "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11 is null\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.present == LTE_EPDCCH_Config_r11__config_r11_PR_setup,
                       "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.present != setup\n");
          AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 != NULL,
                       "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 = NULL\n");
          LTE_EPDCCH_SetConfig_r11_t *epdcch_setconfig_r11 = UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11->list.array[0];
          AssertFatal(epdcch_setconfig_r11 != NULL, "epdcch_setconfig_r11 is null\n");
          AssertFatal(epdcch_setconfig_r11->ext2 != NULL, "epdcch_setconfig_r11->ext2 is null\n");
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13 != NULL, "epdcch_setconfig_r11->ext2->mpdcch_config_r13 is null");
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13 != NULL, "epdcch_setconfig_r11->ext2->mpdcch_config_r13 is null");
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13->present == LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13_PR_setup,
                      "epdcch_setconfig_r11->ext2->mpdcch_config_r13->present is not setup\n");
          AssertFatal(epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 != NULL, "epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 is null");
          AssertFatal(epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present == LTE_EPDCCH_SetConfig_r11__ext2__numberPRB_Pairs_v1310_PR_setup,
                      "epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present is not setup\n");
          LOG_D(MAC,"[PUSCH %d] Frame %d, Subframe %d: Adding UL 6-0A MPDCCH for BL/CE UE %d/%x, ulsch_frame %d, ulsch_subframe %d,UESS MPDCCH Narrowband %d\n",
                harq_pid,
                frameP,
                subframeP,
                UE_id,
                rnti,
                sched_frame,
                sched_subframeP,
                (int)epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1);
          UE_template->first_rb_ul[harq_pid] = narrowband_to_first_rb(cc, epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13-1);
          hi_dci0_pdu = &(hi_dci0_req->hi_dci0_pdu_list[hi_dci0_req->number_of_dci+hi_dci0_req->number_of_hi]);
          memset((void *) hi_dci0_pdu, 0, sizeof(nfapi_hi_dci0_request_pdu_t));
          hi_dci0_pdu->pdu_type = NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE;
          hi_dci0_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_mpdcch_pdu));
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dci_format = (UE_template->rach_resource_type > 1) ? 5 : 4;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ce_mode = (UE_template->rach_resource_type > 1) ? 2 : 1;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mpdcch_narrowband = epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.number_of_prb_pairs = 6;       // checked above that it has to be this
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.resource_block_assignment = 0; // Note: this can be dynamic
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mpdcch_transmission_type = epdcch_setconfig_r11->transmissionType_r11;  // distibuted
          AssertFatal(UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 != NULL,
                      "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 is null\n");
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.start_symbol = *UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ecce_index = 0;        // Note: this should be dynamic
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.aggreagation_level = 24;        // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.rnti_type = 4; // other
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.rnti = rnti;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ce_mode = (UE_template->rach_resource_type < 3) ? 1 : 2;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.drms_scrambling_init = epdcch_setconfig_r11->dmrs_ScramblingSequenceInt_r11;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.initial_transmission_sf_io = (frameP * 10) + subframeP;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.transmission_power = 6000;     // 0dB
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.resource_block_start = UE_template->first_rb_ul[harq_pid];
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.number_of_resource_blocks = 6;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mcs = 4;       // adjust according to size of RAR, 208 bits with N1A_PRB=3
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.pusch_repetition_levels = 0;
          AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_pdsch_HoppingConfig_r13 == LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13__setup__mpdcch_pdsch_HoppingConfig_r13_off,
                      "epdcch_setconfig_r11->ext2->mpdcch_config_r13->mpdcch_pdsch_HoppingConfig_r13 is not off\n");
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.frequency_hopping_flag  = 1 - epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_pdsch_HoppingConfig_r13;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.redudency_version = rvidx_tab[round_UL&3];
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.new_data_indication = UE_template->oldNDI_UL[harq_pid];
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.harq_process = harq_pid;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.tpc = tpc;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.csi_request = cqi_req;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.ul_inex = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dai_presence_flag = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dl_assignment_index = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.srs_request = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.dci_subframe_repetition_number = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.tcp_bitmap = 0;
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.total_dci_length_include_padding = 29; // hard-coded for 10 MHz
          hi_dci0_pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.number_of_tx_antenna_ports = 1;
          hi_dci0_req->number_of_dci++;
          fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp->ul_config_pdu_list[ul_req_tmp->number_of_pdus],
                                               cqi_req,
                                               cc,
                                               UE_template->physicalConfigDedicated,
                                               get_tmode(module_idP,CC_id,UE_id),
                                               eNB->ul_handle,
                                               rnti,
                                               UE_template->first_rb_ul[harq_pid], // resource_block_start
                                               UE_template->nb_rb_ul[harq_pid], // number_of_resource_blocks
                                               UE_template->mcs_UL[harq_pid],
                                               cshift, // cyclic_shift_2_for_drms
                                               0, // frequency_hopping_enabled_flag
                                               0, // frequency_hopping_bits
                                               UE_template->oldNDI_UL[harq_pid], // new_data_indication
                                               rvidx_tab[round_UL&3], // redundancy_version
                                               harq_pid, // harq_process_number
                                               0, // ul_tx_mode
                                               0, // current_tx_nb
                                               0, // n_srs
                                               UE_template->TBS_UL[harq_pid]
                                              );
          fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp->ul_config_pdu_list[ul_req_tmp->number_of_pdus],
                                               UE_template->rach_resource_type>2 ? 2 : 1,
                                               1, //total_number_of_repetitions
                                               1, //repetition_number
                                               (frameP * 10) + subframeP);
          ul_req_tmp->number_of_pdus++;
          eNB->ul_handle++;
        }
      } // UE_is_to_be_scheduled
    } // ULCCs
  } // loop over UE_id
}
#endif
