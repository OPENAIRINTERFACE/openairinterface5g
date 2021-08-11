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


/*! \file asn1_msg.c
 * \brief primitives to build the asn1 messages / primitives to build FeMBMS asn1  messages
 * \author Raymond Knopp, Navid Nikaein and Javier Morgade
 * \date 2011 / 2019
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr javier.morgade@ieee.org
 */

#ifdef EXMIMO
  #include <pthread.h>
#endif

#include "mac_extern.h"
#include "mac.h"
#include "mac_proto.h"
#include "SCHED_UE/sched_UE.h"
#include "PHY/impl_defs_top.h"
#include "PHY_INTERFACE/phy_interface_extern.h"
#include "COMMON/mac_rrc_primitives.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"

#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "RRC/LTE/rrc_extern.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "openair2/PHY_INTERFACE/phy_stub_UE.h"

#include "pdcp.h"
#include "targets/RT/USER/lte-softmodem.h"

#include "intertask_interface.h"

#include "assertions.h"

#include "SIMULATION/TOOLS/sim.h" // for taus

#define DEBUG_HEADER_PARSING 1
#define ENABLE_MAC_PAYLOAD_DEBUG 1


extern UL_IND_t *UL_INFO;


extern int next_ra_frame;
extern module_id_t next_Mod_id;

rb_id_t mbms_rab_id=2047;//[8] = {2047,2047,2047,2047,2047,2047,2047,2047};
static int mbms_mch_i=0;
//static int num_msi_per_CSA[28];


/*
 *
#ifndef USER_MODE
#define msg debug_msg
#endif
 */

mapping BSR_names[] = {
  {"NONE", 0},
  {"SHORT BSR", 1},
  {"TRUNCATED BSR", 2},
  {"LONG BSR", 3},
  {"PADDING BSR", 4},
  {NULL, -1}
};


void ue_init_mac(module_id_t module_idP) {
  int i;
  // default values as deined in 36.331 sec 9.2.2
  LOG_I(MAC, "[UE%d] Applying default macMainConfig\n", module_idP);
  //UE_mac_inst[module_idP].scheduling_info.macConfig=NULL;
  UE_mac_inst[module_idP].scheduling_info.retxBSR_Timer =
    LTE_RetxBSR_Timer_r12_sf10240;
  UE_mac_inst[module_idP].scheduling_info.periodicBSR_Timer =
    LTE_PeriodicBSR_Timer_r12_infinity;
  UE_mac_inst[module_idP].scheduling_info.periodicPHR_Timer =
    LTE_MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20;
  UE_mac_inst[module_idP].scheduling_info.prohibitPHR_Timer =
    LTE_MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20;
  UE_mac_inst[module_idP].scheduling_info.PathlossChange_db =
    LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;
  UE_mac_inst[module_idP].PHR_state =
    LTE_MAC_MainConfig__phr_Config_PR_setup;
  UE_mac_inst[module_idP].scheduling_info.SR_COUNTER = 0;
  UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer = 0;
  UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer_Running = 0;
  UE_mac_inst[module_idP].scheduling_info.maxHARQ_Tx =
    LTE_MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  UE_mac_inst[module_idP].scheduling_info.ttiBundling = 0;
  UE_mac_inst[module_idP].scheduling_info.extendedBSR_Sizes_r10 = 0;
  UE_mac_inst[module_idP].scheduling_info.extendedPHR_r10 = 0;
  UE_mac_inst[module_idP].scheduling_info.drx_config = NULL;
  UE_mac_inst[module_idP].scheduling_info.phr_config = NULL;
  // set init value 0xFFFF, make sure periodic timer and retx time counters are NOT active, after bsr transmission set the value configured by the NW.
  UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF =
    MAC_UE_BSR_TIMER_NOT_RUNNING;
  UE_mac_inst[module_idP].scheduling_info.retxBSR_SF =
    MAC_UE_BSR_TIMER_NOT_RUNNING;
  UE_mac_inst[module_idP].BSR_reporting_active = BSR_TRIGGER_NONE;
  UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF =
    get_sf_perioidicPHR_Timer(UE_mac_inst[module_idP].
                              scheduling_info.periodicPHR_Timer);
  UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF =
    get_sf_prohibitPHR_Timer(UE_mac_inst[module_idP].
                             scheduling_info.prohibitPHR_Timer);
  UE_mac_inst[module_idP].scheduling_info.PathlossChange_db =
    get_db_dl_PathlossChange(UE_mac_inst[module_idP].
                             scheduling_info.PathlossChange);
  UE_mac_inst[module_idP].PHR_reporting_active = 0;

  for (i = 0; i < MAX_NUM_LCID; i++) {
    LOG_D(MAC,
          "[UE%d] Applying default logical channel config for LCGID %d\n",
          module_idP, i);
    UE_mac_inst[module_idP].scheduling_info.Bj[i] = -1;
    UE_mac_inst[module_idP].scheduling_info.bucket_size[i] = -1;

    if (i < DTCH) {   // initilize all control channels lcgid to 0
      UE_mac_inst[module_idP].scheduling_info.LCGID[i] = 0;
    } else {    // initialize all the data channels lcgid to 1
      UE_mac_inst[module_idP].scheduling_info.LCGID[i] = 1;
    }

    UE_mac_inst[module_idP].scheduling_info.LCID_status[i] =
      LCID_EMPTY;
    UE_mac_inst[module_idP].scheduling_info.LCID_buffer_remain[i] = 0;
  }

  if(NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) {
    pthread_mutex_init(&UE_mac_inst[module_idP].UL_INFO_mutex,NULL);
    UE_mac_inst[module_idP].UE_mode[0] = PRACH; //NOT_SYNCHED;
    UE_mac_inst[module_idP].first_ULSCH_Tx =0;
    UE_mac_inst[module_idP].SI_Decoded = 0;
    next_ra_frame = 0;
    next_Mod_id = 0;
  }
}


unsigned char *parse_header(unsigned char *mac_header,
                            unsigned char *num_ce,
                            unsigned char *num_sdu,
                            unsigned char *rx_ces,
                            unsigned char *rx_lcids,
                            unsigned short *rx_lengths,
                            unsigned short tb_length) {
  unsigned char not_done = 1, num_ces = 0, num_cont_res =
                                          0, num_padding = 0, num_sdus = 0, lcid, num_sdu_cnt;
  unsigned char *mac_header_ptr = mac_header;
  unsigned short length, ce_len = 0;

  while (not_done == 1) {
    if (((SCH_SUBHEADER_FIXED *) mac_header_ptr)->E == 0) {
      //      printf("E=0\n");
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->LCID;

    if (lcid < UE_CONT_RES) {
      //printf("[MAC][UE] header %x.%x.%x\n",mac_header_ptr[0],mac_header_ptr[1],mac_header_ptr[2]);
      if (not_done == 0) {  // last MAC SDU, length is implicit
        mac_header_ptr++;
        length =
          tb_length - (mac_header_ptr - mac_header) - ce_len;

        for (num_sdu_cnt = 0; num_sdu_cnt < num_sdus;
             num_sdu_cnt++) {
          length -= rx_lengths[num_sdu_cnt];
        }
      } else {
        if (((SCH_SUBHEADER_LONG *) mac_header_ptr)->F == 1) {
          length =
            ((((SCH_SUBHEADER_LONG *) mac_header_ptr)->
              L_MSB & 0x7f)
             << 8) | (((SCH_SUBHEADER_LONG *) mac_header_ptr)->
                      L_LSB & 0xff);
          mac_header_ptr += 3;
#ifdef DEBUG_HEADER_PARSING
          LOG_D(MAC, "[UE] parse long sdu, size %x \n", length);
#endif
        } else {  //if (((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F == 0) {
          length = ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L;
          mac_header_ptr += 2;
        }
      }

#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC, "[UE] sdu %d lcid %d length %d (offset now %ld)\n",
            num_sdus, lcid, length, mac_header_ptr - mac_header);
#endif
      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else {    // This is a control element subheader
      if (lcid == SHORT_PADDING) {
        num_padding++;
        mac_header_ptr++;
      } else {
        rx_ces[num_ces] = lcid;
        num_ces++;
        mac_header_ptr++;

        if (lcid == TIMING_ADV_CMD) {
          ce_len++;
        } else if (lcid == UE_CONT_RES) {
          // FNA: check MAC Header is one of thoses defined in Annex B of 36.321
          // Check there is only 1 Contention Resolution
          if (num_cont_res) {
            LOG_W(MAC,
                  "[UE] Msg4 Wrong received format: More than 1 Contention Resolution\n");
            // exit parsing
            return NULL;
          }

          // UE_CONT_RES shall never be the last subheader unless this is the only MAC subheader
          if ((not_done == 0)
              && ((num_sdus) || (num_ces > 1) || (num_padding))) {
            LOG_W(MAC,
                  "[UE] Msg4 Wrong received format: Contention Resolution after num_ces=%d num_sdus=%d num_padding=%d\n",
                  num_ces, num_sdus, num_padding);
            // exit parsing
            return NULL;
          }

          num_cont_res++;
          ce_len += 6;
        }
      }

#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC, "[UE] ce %d lcid %d (offset now %ld)\n", num_ces,
            lcid, mac_header_ptr - mac_header);
#endif
    }
  }

  *num_ce = num_ces;
  *num_sdu = num_sdus;
  return (mac_header_ptr);
}

uint32_t
ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP,
          uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe) {
  // no UL-SCH resources available for this tti && UE has a valid PUCCH resources for SR configuration for this tti
  //  int MGL=6;// measurement gap length in ms
  int MGRP = 0;   // measurement gap repetition period in ms
  int gapOffset = -1;
  int T = 0;
  DevCheck(module_idP < (int) NB_UE_INST, module_idP, NB_UE_INST, 0);
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");

  // determin the measurement gap
  if (UE_mac_inst[module_idP].measGapConfig != NULL) {
    if (UE_mac_inst[module_idP].measGapConfig->choice.setup.
        gapOffset.present == LTE_MeasGapConfig__setup__gapOffset_PR_gp0) {
      MGRP = 40;
      gapOffset =
        UE_mac_inst[module_idP].measGapConfig->choice.
        setup.gapOffset.choice.gp0;
    } else if (UE_mac_inst[module_idP].measGapConfig->choice.
               setup.gapOffset.present ==
               LTE_MeasGapConfig__setup__gapOffset_PR_gp1) {
      MGRP = 80;
      gapOffset =
        UE_mac_inst[module_idP].measGapConfig->choice.
        setup.gapOffset.choice.gp1;
    } else {
      LOG_W(MAC, "Measurement GAP offset is unknown\n");
    }

    T = MGRP / 10;
    DevAssert(T != 0);

    //check the measurement gap and sr prohibit timer
    if ((subframe == gapOffset % 10)
        && ((frameP % T) == (floor(gapOffset / 10)))
        && (UE_mac_inst[module_idP].
            scheduling_info.sr_ProhibitTimer_Running == 0)) {
      UE_mac_inst[module_idP].scheduling_info.SR_pending = 1;
      return (0);
    }
  }

  if ((UE_mac_inst[module_idP].physicalConfigDedicated != NULL) &&
      (UE_mac_inst[module_idP].scheduling_info.SR_pending == 1) &&
      (UE_mac_inst[module_idP].scheduling_info.SR_COUNTER <
       (1 <<
        (2 +
         UE_mac_inst[module_idP].
         physicalConfigDedicated->schedulingRequestConfig->choice.setup.
         dsr_TransMax)))) {
    LOG_D(MAC,
          "[UE %d][SR %x] Frame %d subframe %d PHY asks for SR (SR_COUNTER/dsr_TransMax %d/%d), SR_pending %d\n",
          module_idP, rnti, frameP, subframe,
          UE_mac_inst[module_idP].scheduling_info.SR_COUNTER,
          (1 <<
           (2 +
            UE_mac_inst[module_idP].
            physicalConfigDedicated->schedulingRequestConfig->choice.
            setup.dsr_TransMax)),
          UE_mac_inst[module_idP].scheduling_info.SR_pending);
    UE_mac_inst[module_idP].scheduling_info.SR_COUNTER++;

    // start the sr-prohibittimer : rel 9 and above
    if (UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer > 0) { // timer configured
      UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer--;
      UE_mac_inst[module_idP].scheduling_info.
      sr_ProhibitTimer_Running = 1;
    } else {
      UE_mac_inst[module_idP].scheduling_info.
      sr_ProhibitTimer_Running = 0;
    }

    LOG_D(MAC,
          "[UE %d][SR %x] Frame %d subframe %d send SR indication (SR_COUNTER/dsr_TransMax %d/%d), SR_pending %d\n",
          module_idP, rnti, frameP, subframe,
          UE_mac_inst[module_idP].scheduling_info.SR_COUNTER,
          (1 <<
           (2 +
            UE_mac_inst[module_idP].
            physicalConfigDedicated->schedulingRequestConfig->choice.
            setup.dsr_TransMax)),
          UE_mac_inst[module_idP].scheduling_info.SR_pending);
    //UE_mac_inst[module_idP].ul_active =1;
    return (1);   //instruct phy to signal SR
  } else {
    // notify RRC to relase PUCCH/SRS
    // clear any configured dl/ul
    // initiate RA
    if (UE_mac_inst[module_idP].scheduling_info.SR_pending) {
      // release all pucch resource
      UE_mac_inst[module_idP].physicalConfigDedicated = NULL;
      UE_mac_inst[module_idP].ul_active = 0;
      UE_mac_inst[module_idP].BSR_reporting_active =
        BSR_TRIGGER_NONE;
      LOG_I(MAC, "[UE %d] Release all SRs \n", module_idP);
    }

    UE_mac_inst[module_idP].scheduling_info.SR_pending = 0;
    UE_mac_inst[module_idP].scheduling_info.SR_COUNTER = 0;
    return (0);
  }
}

//------------------------------------------------------------------------------
void
ue_send_sdu(module_id_t module_idP,
            uint8_t CC_id,
            frame_t frameP,
            sub_frame_t subframeP,
            uint8_t *sdu, uint16_t sdu_len, uint8_t eNB_index)
//------------------------------------------------------------------------------
{
  unsigned char rx_ces[MAX_NUM_CE], num_ce, num_sdu, i, *payload_ptr;
  unsigned char rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];
  unsigned char *tx_sdu;
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].rx_dlsch_sdu);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_IN);
  //LOG_D(MAC,"sdu: %x.%x.%x\n",sdu[0],sdu[1],sdu[2]);
  trace_pdu(DIRECTION_DOWNLINK, sdu, sdu_len, module_idP, WS_C_RNTI,
            UE_mac_inst[module_idP].crnti, frameP, subframeP, 0, 0);
  payload_ptr =
    parse_header(sdu, &num_ce, &num_sdu, rx_ces, rx_lcids, rx_lengths,
                 sdu_len);
#ifdef DEBUG_HEADER_PARSING
  LOG_D(MAC,
        "[UE %d] ue_send_sdu : Frame %d eNB_index %d : num_ce %d num_sdu %d\n",
        module_idP, frameP, eNB_index, num_ce, num_sdu);
#endif
#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
  LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);

  for (i = 0; i < 32; i++) {
    LOG_T(MAC, "%x.", sdu[i]);
  }

  LOG_T(MAC, "\n");
#endif

  if (payload_ptr != NULL) {
    for (i = 0; i < num_ce; i++) {
      LOG_I(MAC, "ce %d : %d\n",i,rx_ces[i]);
      switch (rx_ces[i]) {
        case UE_CONT_RES:
          LOG_I(MAC,
                "[UE %d][RAPROC] Frame %d : received contention resolution msg: %x.%x.%x.%x.%x.%x, Terminating RA procedure\n",
                module_idP, frameP, payload_ptr[0], payload_ptr[1],
                payload_ptr[2], payload_ptr[3], payload_ptr[4],
                payload_ptr[5]);

          if (UE_mac_inst[module_idP].RA_active == 1) {
            LOG_I(MAC,
                  "[UE %d][RAPROC] Frame %d : Clearing RA_active flag\n",
                  module_idP, frameP);
            UE_mac_inst[module_idP].RA_active = 0;
            // check if RA procedure has finished completely (no contention)
            tx_sdu = &UE_mac_inst[module_idP].CCCH_pdu.payload[3];

            //Note: 3 assumes sizeof(SCH_SUBHEADER_SHORT) + PADDING CE, which is when UL-Grant has TBS >= 9 (64 bits)
            // (other possibility is 1 for TBS=7 (SCH_SUBHEADER_FIXED), or 2 for TBS=8 (SCH_SUBHEADER_FIXED+PADDING or SCH_SUBHEADER_SHORT)
            for (i = 0; i < 6; i++)
              if (tx_sdu[i] != payload_ptr[i]) {
                LOG_E(MAC,
                      "[UE %d][RAPROC] Contention detected, RA failed\n",
                      module_idP);

                if(NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) { // phy_stub mode
                  //  Modification for phy_stub mode operation here. We only need to make sure that the ue_mode is back to
                  // PRACH state.
                  LOG_I(MAC, "nfapi_mode3: Setting UE_mode BACK to PRACH 1\n");
                  UE_mac_inst[module_idP].UE_mode[eNB_index] = PRACH;
                  //ra_failed(module_idP,CC_id,eNB_index);UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
                } else {
                  ra_failed(module_idP, CC_id, eNB_index);
                }

                UE_mac_inst
                [module_idP].
                RA_contention_resolution_timer_active = 0;
                VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
                (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU,
                 VCD_FUNCTION_OUT);
                return;
              }

            LOG_I(MAC,
                  "[UE %d][RAPROC] Frame %d : Clearing contention resolution timer\n",
                  module_idP, frameP);
            UE_mac_inst
            [module_idP].
            RA_contention_resolution_timer_active = 0;

            if(NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) { // phy_stub mode
              // Modification for phy_stub mode operation here. We only need to change the ue_mode to PUSCH
              UE_mac_inst[module_idP].UE_mode[eNB_index] = PUSCH;
            } else { // Full stack mode
              ra_succeeded(module_idP,CC_id,eNB_index);
            }
          }

          payload_ptr += 6;
          break;

        case TIMING_ADV_CMD:
#ifdef DEBUG_HEADER_PARSING
          LOG_D(MAC, "[UE] CE %d : UE Timing Advance : %d\n", i,
                payload_ptr[0]);
#endif

          // Eliminate call to process_timing_advance for the phy_stub UE operation mode. Is this correct?
          if (NFAPI_MODE!=NFAPI_UE_STUB_PNF && NFAPI_MODE!=NFAPI_MODE_STANDALONE_PNF) {
            process_timing_advance(module_idP,CC_id,payload_ptr[0]);
          }

          payload_ptr++;
          break;

        case DRX_CMD:
#ifdef DEBUG_HEADER_PARSING
          LOG_D(MAC, "[UE] CE %d : UE DRX :", i);
#endif
          payload_ptr++;
          break;
      }
    }

    for (i = 0; i < num_sdu; i++) {
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC, "[UE] SDU %d : LCID %d, length %d\n", i,
            rx_lcids[i], rx_lengths[i]);
#endif

      if (rx_lcids[i] == CCCH) {
        LOG_D(MAC,
              "[UE %d] rnti %x Frame %d : DLSCH -> DL-CCCH, RRC message (eNB %d, %d bytes)\n",
              module_idP, UE_mac_inst[module_idP].crnti, frameP,
              eNB_index, rx_lengths[i]);
#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
        int j;

        for (j = 0; j < rx_lengths[i]; j++) {
          LOG_T(MAC, "%x.", (uint8_t) payload_ptr[j]);
        }

        LOG_T(MAC, "\n");
#endif
        mac_rrc_data_ind_ue(module_idP,
                            CC_id,
                            frameP,subframeP,
                            UE_mac_inst[module_idP].crnti,
                            CCCH,
                            (uint8_t *)payload_ptr,
                            rx_lengths[i],
                            eNB_index,
                            0
                           );
      } else if ((rx_lcids[i] == DCCH) || (rx_lcids[i] == DCCH1)) {
        LOG_D(MAC,"[UE %d] Frame %d : DLSCH -> DL-DCCH%d, RRC message (eNB %d, %d bytes)\n", module_idP, frameP, rx_lcids[i],eNB_index,rx_lengths[i]);
        mac_rlc_data_ind(module_idP,
                         UE_mac_inst[module_idP].crnti,
                         eNB_index,
                         frameP,
                         ENB_FLAG_NO,
                         MBMS_FLAG_NO,
                         rx_lcids[i],
                         (char *)payload_ptr,
                         rx_lengths[i],
                         1,
                         NULL);
      } else if ((rx_lcids[i]  < NB_RB_MAX) && (rx_lcids[i] > DCCH1 )) {
        LOG_D(MAC,"[UE %d] Frame %d : DLSCH -> DL-DTCH%d (eNB %d, %d bytes)\n", module_idP, frameP,rx_lcids[i], eNB_index,rx_lengths[i]);
#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
        int j;

        for (j = 0; j < rx_lengths[i]; j++)
          LOG_T(MAC, "%x.", (unsigned char) payload_ptr[j]);

        LOG_T(MAC, "\n");
#endif
        mac_rlc_data_ind(module_idP,
                         UE_mac_inst[module_idP].crnti,
                         eNB_index,
                         frameP,
                         ENB_FLAG_NO,
                         MBMS_FLAG_NO,
                         rx_lcids[i],
                         (char *) payload_ptr, rx_lengths[i], 1,
                         NULL);
      } else {
        LOG_E(MAC, "[UE %d] Frame %d : unknown LCID %d (eNB %d)\n",
              module_idP, frameP, rx_lcids[i], eNB_index);
      }

      payload_ptr += rx_lengths[i];
    }
  }       // end if (payload_ptr != NULL)

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].rx_dlsch_sdu);
#endif
}

void ue_decode_si_mbms(module_id_t module_idP, int CC_id, frame_t frameP,
                       uint8_t eNB_index, void *pdu, uint16_t len) {
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].rx_si);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_SI, VCD_FUNCTION_IN);
  LOG_D(MAC, "[UE %d] Frame %d Sending SI MBMS to RRC (LCID Id %d,len %d)\n",
        module_idP, frameP, BCCH, len);
  mac_rrc_data_ind_ue(module_idP, CC_id, frameP, 0,  // unknown subframe
                      SI_RNTI,
                      BCCH_SI_MBMS, (uint8_t *) pdu, len, eNB_index,
                      0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_SI, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].rx_si);
#endif

  trace_pdu(DIRECTION_UPLINK,
            (uint8_t *) pdu,
            len,
            module_idP,
            WS_SI_RNTI,
            0xffff,
            UE_mac_inst[module_idP].rxFrame,
            UE_mac_inst[module_idP].rxSubframe,
            0,
            0);
}


void
ue_decode_si(module_id_t module_idP, int CC_id, frame_t frameP,
             uint8_t eNB_index, void *pdu, uint16_t len) {
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].rx_si);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_SI, VCD_FUNCTION_IN);
  LOG_D(MAC, "[UE %d] Frame %d Sending SI to RRC (LCID Id %d,len %d)\n",
        module_idP, frameP, BCCH, len);
  mac_rrc_data_ind_ue(module_idP, CC_id, frameP, 0, // unknown subframe
                      SI_RNTI,
                      BCCH, (uint8_t *) pdu, len, eNB_index,
                      0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_SI, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].rx_si);
#endif
  trace_pdu(DIRECTION_UPLINK,
            (uint8_t *) pdu,
            len,
            module_idP,
            WS_SI_RNTI,
            0xffff,
            UE_mac_inst[module_idP].rxFrame,
            UE_mac_inst[module_idP].rxSubframe, 0, 0);
}

void
ue_decode_p(module_id_t module_idP, int CC_id, frame_t frameP,
            uint8_t eNB_index, void *pdu, uint16_t len) {
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].rx_p);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_PCCH, VCD_FUNCTION_IN);
  LOG_D(MAC,
        "[UE %d] Frame %d Sending Paging message to RRC (LCID Id %d,len %d)\n",
        module_idP, frameP, PCCH, len);
  mac_rrc_data_ind_ue(module_idP, CC_id, frameP, 0, // unknown subframe
                      P_RNTI,
                      PCCH, (uint8_t *) pdu, len, eNB_index,
                      0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_PCCH, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].rx_p);
#endif
  trace_pdu(DIRECTION_UPLINK,
            (uint8_t *) pdu,
            len,
            module_idP,
            WS_SI_RNTI,
            P_RNTI,
            UE_mac_inst[module_idP].rxFrame,
            UE_mac_inst[module_idP].rxSubframe, 0, 0);
}

unsigned char *parse_mch_header(unsigned char *mac_header,
                                unsigned char *num_sdu,
                                unsigned char *rx_lcids,
                                unsigned short *rx_lengths,
                                unsigned short tb_length) {
  unsigned char not_done = 1, num_sdus = 0, lcid, i;
  unsigned char *mac_header_ptr = mac_header;
  unsigned short length;

  while (not_done == 1) {
    if (((SCH_SUBHEADER_FIXED *) mac_header_ptr)->E == 0) {
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->LCID;

    if (lcid < SHORT_PADDING) { // subheader for MSI, MCCH or MTCH
      if (not_done == 0) {  // last MAC SDU, length is implicit
        mac_header_ptr++;
        length = tb_length - (mac_header_ptr - mac_header);

        for (i = 0; i < num_sdus; i++) {
          length -= rx_lengths[i];
        }
      } else {    // not the last MAC SDU
        if (((SCH_SUBHEADER_LONG *) mac_header_ptr)->F == 1) {  // subheader has length of 3octets
          //    length = ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L;
          length =
            ((((SCH_SUBHEADER_LONG *) mac_header_ptr)->
              L_MSB & 0x7f)
             << 8) | (((SCH_SUBHEADER_LONG *) mac_header_ptr)->
                      L_LSB & 0xff);
          mac_header_ptr += 3;
        } else {  // subheader has length of 2octets
          length = ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L;
          mac_header_ptr += 2;
        }
      }

      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else {    // subheader for padding
      //     if (lcid == SHORT_PADDING)
      mac_header_ptr++;
    }
  }

  *num_sdu = num_sdus;
  return (mac_header_ptr);
}

// this function is for sending mch_sdu from phy to mac
void
ue_send_mch_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
                uint8_t *sdu, uint16_t sdu_len, uint8_t eNB_index,
                uint8_t sync_area) {
  unsigned char num_sdu, i, j, *payload_ptr;
  unsigned char rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].rx_mch_sdu);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_MCH_SDU, VCD_FUNCTION_IN);
  LOG_D(MAC,
        "[UE %d] Frame %d : process the mch PDU for sync area %d \n",
        module_idP, frameP, sync_area);
  LOG_D(MAC, "[UE %d] sdu: %x.%x\n", module_idP, sdu[0], sdu[1]);
  LOG_D(MAC, "[UE %d] parse_mch_header, demultiplex\n", module_idP);
  payload_ptr =
    parse_mch_header(sdu, &num_sdu, rx_lcids, rx_lengths, sdu_len);
  LOG_D(MAC, "[UE %d] parse_mch_header, found %d sdus\n", module_idP,
        num_sdu);

  if(sdu[0]==0 && sdu[1]==0)
        num_sdu=0;

  for (i = 0; i < num_sdu; i++) {
    if (rx_lcids[i] == MCH_SCHDL_INFO) {
      if (rx_lengths[i] & 0x01) {
        LOG_E(MAC,"MCH Scheduling Information MAC Control Element should have an even size\n");
      }


      LOG_D(MAC,"MCH Scheduling Information, len(%d)\n",rx_lengths[i]);
     
      for (j=0; j<rx_lengths[i]/2; j++) {
        uint16_t stop_mtch_val = ((uint16_t)(payload_ptr[2*j] & 0x07) << 8) | (uint16_t)payload_ptr[2*j+1];
        UE_mac_inst[module_idP].pmch_lcids[j] = (payload_ptr[2*j] & 0xF8) >> 3;
        UE_mac_inst[module_idP].pmch_stop_mtch[j] = stop_mtch_val;
        LOG_D(MAC,"lcid(%d),stop_mtch_val %d frameP(%d)\n", UE_mac_inst[module_idP].pmch_lcids[j], stop_mtch_val, frameP);

        if ((stop_mtch_val >= 2043) && (stop_mtch_val <= 2046)) {
          LOG_D(MAC,"(reserved)\n");
        }

        UE_mac_inst[module_idP].msi_status_v[j] = 0;

        if (UE_mac_inst[module_idP].mcch_status==1) {
          LOG_D(MAC,"[UE %d] Frame %d : MCH->MSI for sync area %d (eNB %d, %d bytes), LCID(%d)(%d)\n", module_idP, frameP, sync_area, eNB_index, rx_lengths[i], UE_mac_inst[module_idP].pmch_lcids[j] , UE_mac_inst[module_idP].pmch_stop_mtch[j]);

          if (UE_mac_inst[module_idP].pmch_stop_mtch[j] < 2043) {
            UE_mac_inst[module_idP].pmch_stop_mtch[j] += UE_mac_inst[module_idP].msi_current_alloc;
            UE_mac_inst[module_idP].msi_status_v[j] = 1;
          }
        }
      }
    } else if (rx_lcids[i] == MCCH_LCHANID) {
      LOG_D(MAC,"[UE %d] Frame %d : SDU %d MCH->MCCH for sync area %d (eNB %d, %d bytes)\n",module_idP,frameP, i, sync_area, eNB_index, rx_lengths[i]);
      mac_rrc_data_ind_ue(module_idP,
                          CC_id,
                          frameP,0, // unknown subframe
                          M_RNTI,
                          MCCH,
                          payload_ptr + 1, // Skip RLC layer 1st byte
                          rx_lengths[i] - 1,
                          eNB_index,
                          sync_area);
    } else if (rx_lcids[i] <= 28) {
      for (j=0; j<28; j++) {
        if (rx_lcids[i] == UE_mac_inst[module_idP].pmch_lcids[j])
          break;
      }

      if (j<28 && UE_mac_inst[module_idP].msi_status_v[j]==1) {
        LOG_D(MAC,"[UE %d] Frame %d : MCH->MTCH for sync area %d (eNB %d, %d bytes), j=%d lcid %d\n", module_idP, frameP, sync_area, eNB_index, rx_lengths[i], j,rx_lcids[i]);
        //This sucks I know ... workaround !
       mbms_rab_id = rx_lcids[i];
        //end sucks  :-(
        mac_rlc_data_ind(
          module_idP,
          UE_mac_inst[module_idP].crnti,
          eNB_index,
          frameP,
          ENB_FLAG_NO,
          MBMS_FLAG_YES,
          rx_lcids[i], /*+ (maxDRB + 3),*/
          (char *)payload_ptr,
          rx_lengths[i],
          1,
          NULL);
      }
    } else {
      LOG_W(MAC,
            "[UE %d] Frame %d : unknown sdu %d rx_lcids[%d]=%d mcch status %d eNB %d \n",
            module_idP, frameP, rx_lengths[i], i, rx_lcids[i],
            UE_mac_inst[module_idP].mcch_status, eNB_index);
    }

    payload_ptr += rx_lengths[i];
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_MCH_SDU, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].rx_mch_sdu);
#endif
}

void ue_send_sl_sdu(module_id_t module_idP,
                    uint8_t CC_id,
                    frame_t frameP,
                    sub_frame_t subframeP,
                    uint8_t *sdu,
                    uint16_t sdu_len,
                    uint8_t eNB_index,
                    sl_discovery_flag_t sl_discovery_flag
                   ) {
  int rlc_sdu_len;
  char *rlc_sdu;
  uint32_t destinationL2Id =0x00000000;

  if (sl_discovery_flag == SL_DISCOVERY_FLAG_NO) {
    // Notes: 1. no control elements are supported yet
    //        2. we exit with error if LCID != 3
    //        3. we exit with error if E=1 (more than one SDU/CE)
    // extract header
    SLSCH_SUBHEADER_24_Bit_DST_LONG *longh = (SLSCH_SUBHEADER_24_Bit_DST_LONG *)sdu;
    AssertFatal(longh->E==0,"E is non-zero\n");
    AssertFatal(((longh->LCID==3)|(longh->LCID==10)),"LCID is %d (not 3 or 10)\n",longh->LCID);
    //filter incoming packet based on destination address
    destinationL2Id = (longh->DST07<<16) | (longh->DST815 <<8) | (longh->DST1623);
    LOG_I( MAC, "[DestinationL2Id:  0x%08x]  \n", destinationL2Id );
    //in case of 1-n communication, verify that UE belongs to that group
    int i=0;

    for (i=0; i< MAX_NUM_DEST; i++)
      if (UE_mac_inst[module_idP].destinationList[i] == destinationL2Id) break;

    //match the destinationL2Id with UE L2Id or groupL2ID
    if (!((destinationL2Id == UE_mac_inst[module_idP].sourceL2Id) | (i < MAX_NUM_DEST))) {
      LOG_I( MAC, "[Destination Id is neither matched with Source Id nor with Group Id, drop the packet!!! \n");
      return;
    }

    if (longh->F==1) {
      rlc_sdu_len = ((longh->L_MSB<<8)&0x7F00)|(longh->L_LSB&0xFF);
      rlc_sdu = (char *)sdu+sizeof(SLSCH_SUBHEADER_24_Bit_DST_LONG);
    } else {
      rlc_sdu_len = ((SLSCH_SUBHEADER_24_Bit_DST_SHORT *)sdu)->L;
      rlc_sdu = (char *)sdu+sizeof(SLSCH_SUBHEADER_24_Bit_DST_SHORT);
    }

    mac_rlc_data_ind(
      module_idP,
      0x1234,
      eNB_index,
      frameP,
      ENB_FLAG_NO,
      MBMS_FLAG_NO,
      longh->LCID, //3/10
      rlc_sdu,
      rlc_sdu_len,
      1,
      NULL);
  } else { //SL_DISCOVERY
    uint16_t len = sdu_len;
    LOG_I( MAC, "SL DISCOVERY \n");
    mac_rrc_data_ind_ue(module_idP,
                        CC_id,
                        frameP,subframeP,
                        UE_mac_inst[module_idP].crnti,
                        SL_DISCOVERY,
                        sdu, //(uint8_t*)&UE_mac_inst[Mod_id].SL_Discovery[0].Rx_buffer.Payload[0],
                        len,
                        eNB_index,
                        0);
  }
}


int8_t ue_get_mbsfn_sf_alloction (module_id_t module_idP, uint8_t mbsfn_sync_area, unsigned char eNB_index)

{
  // currently there is one-to-one mapping between sf allocation pattern and sync area
  if (mbsfn_sync_area >= MAX_MBSFN_AREA) {
    LOG_W(MAC,
          "[UE %" PRIu8 "] MBSFN synchronization area %" PRIu8
          " out of range for eNB %" PRIu8 "\n", module_idP,
          mbsfn_sync_area, eNB_index);
    return -1;
  } else if (UE_mac_inst[module_idP].
             mbsfn_SubframeConfig[mbsfn_sync_area] != NULL) {
    return mbsfn_sync_area;
  } else {
    LOG_W(MAC,
          "[UE %" PRIu8 "] MBSFN Subframe Config pattern %" PRIu8
          " not found \n", module_idP, mbsfn_sync_area);
    return -1;
  }
}

int ue_query_p_mch_info(module_id_t module_idP, uint32_t frameP, uint32_t subframe, int commonSFAlloc_period, int commonSFAlloc_offset, int num_sf_alloc, int *mtch_active, int *msi_active,
                        uint8_t *mch_lcid) {
  int i, mtch_mcs = -1;
  int mtch_flag = 0;
  int msi_flag = 0;

  for (i = 0; i < 4; i++) {
    if (UE_mac_inst[module_idP].pmch_Config[i] == NULL)
      continue;

    int mch_scheduling_period = 8 << UE_mac_inst[module_idP].pmch_Config[i]->mch_SchedulingPeriod_r9;
    uint8_t  sf_AllocEnd_r9   = UE_mac_inst[module_idP].pmch_Config[i]->sf_AllocEnd_r9;

    if (sf_AllocEnd_r9 == 2047) {
      msi_flag = 1;
      break;
    }

    if ((frameP % mch_scheduling_period) == (commonSFAlloc_offset + (frameP % 4))) {
      if (i == 0) {
        //msi and mtch are mutally excluded then the break is safe
        if ((num_sf_alloc == 0) && (sf_AllocEnd_r9 >= 1)) {
          msi_flag = 1;
          LOG_D(MAC,"msi(%d) should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),sf_AllocEnd_r9(%d),common_num_sf_alloc(%d)\n",i,frameP,subframe,num_sf_alloc,sf_AllocEnd_r9,
                UE_mac_inst[module_idP].common_num_sf_alloc);
          UE_mac_inst[module_idP].msi_current_alloc = num_sf_alloc;
          UE_mac_inst[module_idP].msi_pmch = i;
        }
      } else { //more that one MCH ?? check better this condition
        //msi and mtch are mutally excluded then the break is safe
	//TODO
        if ((num_sf_alloc ==  UE_mac_inst[module_idP].pmch_Config[i-1]->sf_AllocEnd_r9 + 1) && (sf_AllocEnd_r9 >= (num_sf_alloc+1))) {
        //if ((num_sf_alloc ==  UE_mac_inst[module_idP].pmch_Config[i-1]->sf_AllocEnd_r9) && (sf_AllocEnd_r9 >= (num_sf_alloc))) {
          //msi should be just after
          msi_flag = 1;
          LOG_D(MAC,"msi(%d) should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),sf_AllocEnd_r9(%d),common_num_sf_alloc(%d)\n",i,frameP,subframe,num_sf_alloc,sf_AllocEnd_r9,
                UE_mac_inst[module_idP].common_num_sf_alloc);
          UE_mac_inst[module_idP].msi_current_alloc = num_sf_alloc;
          UE_mac_inst[module_idP].msi_pmch = i;
        }
      }
    }
  }

  for (i = 0; i < 28; i++) {
    if (UE_mac_inst[module_idP].pmch_stop_mtch[i] >= num_sf_alloc) {
      if (UE_mac_inst[module_idP].pmch_stop_mtch[i] != 2047) {
        mtch_flag = 1;

        if (UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch] != NULL)
          mtch_mcs = UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch]->dataMCS_r9;
        else
          mtch_mcs = -1;

        *mch_lcid = (uint8_t)i;
        LOG_D(MAC,"mtch should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),mtch_mcs(%d),pmch_stop_mtch(%d),lcid(%d),msi_pmch(%d)\n",frameP,subframe,num_sf_alloc,mtch_mcs,
              UE_mac_inst[module_idP].pmch_stop_mtch[i],UE_mac_inst[module_idP].pmch_lcids[i],UE_mac_inst[module_idP].msi_pmch);
        break;
      }
    }
  }

  *mtch_active = mtch_flag;
  *msi_active = msi_flag;
  return mtch_mcs;
}

//static int common_num_sf_alloc=0;
//static int x=0;
int ue_query_mch_fembms(module_id_t module_idP, uint8_t CC_id, uint32_t frameP, uint32_t subframe, uint8_t eNB_index,uint8_t *sync_area, uint8_t *mcch_active) {
  int i = 0, j = 0,/* ii = 0,*/ msi_pos = -1, mcch_mcs = -1, mtch_mcs = -1, l=0/*,ii=0*/;
  int mcch_flag = 0, mtch_flag = 0, msi_flag = 0;
  int alloc_offset=0;
  uint32_t period;
  //uint16_t num_non_mbsfn_SubframeConfig=0;
  long mch_scheduling_period = -1;
  uint8_t mch_lcid = 0;
  //int first_pos=0;

  if(UE_mac_inst[module_idP].non_mbsfn_SubframeConfig == NULL ) 
	return -1;

  int non_mbsfn_SubframeConfig = (UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]>>7);

  period = 4<<UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->radioFrameAllocationPeriod_r14;
  alloc_offset = UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->radioFrameAllocationOffset_r14;

  long mcch_period  = 32 << UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9;
  long mcch_offset        = UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9;

  if (UE_mac_inst[module_idP].pmch_Config[0]) {
    mch_scheduling_period = 8 << UE_mac_inst[module_idP].pmch_Config[0]->mch_SchedulingPeriod_r9;
  }

 
//  for(l=0;l<8;l++){
//     if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]!=NULL){
//             if(frameP%(4<<UE_mac_inst[module_idP].commonSF_AllocPeriod_r9)==0){
//		    first_pos=0;
//  		    if((frameP&3)==0)
//			first_pos++;
//  		    while((non_mbsfn_SubframeConfig & (0x100 >> msi_pos)) == (0x100>>msi_pos))
//			first_pos++;
//		    if(first_pos == subframe){
//                        ii=0;
//                        while(UE_mac_inst[module_idP].pmch_Config[ii]!=NULL){
//                                num_msi_per_CSA[ii]= ( ( 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9) / (8 << UE_mac_inst[module_idP].pmch_Config[ii]->mch_SchedulingPeriod_r9)  );
//                                ii++;
//                                LOG_D(MAC,"frameP %d subframe %d num_msi_per_CSA[%d] %d\n",frameP,subframe,ii,num_msi_per_CSA[ii]);
//                        }
//		    }
//
//             }
//     }
//  }
//

  // get the real MCS value
  switch (UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.signallingMCS_r9) {
    case 0:
      mcch_mcs = 2;
      break;

    case 1:
      mcch_mcs = 7;
      break;

    case 2:
      mcch_mcs = 13;
      break;

    case 3:
      mcch_mcs = 19;
      break;
  }
  LOG_D(MAC,"frameP %d subframe %d period %d alloc_offset %d mcch_mcs %d mcch_period %ld mcch_offset %ld buf %x mch_scheduling_period %ld\n",frameP, subframe, period, alloc_offset,mcch_mcs, mcch_period, mcch_offset,(UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0]),mch_scheduling_period);

  if((frameP % period ) == alloc_offset){
	switch(subframe){
	case 0:
		return (-1);
	   break;
	case 1:
           if ((non_mbsfn_SubframeConfig & 0x100) > 0)
		return (-1);
	   break;
	case 2:
           if ((non_mbsfn_SubframeConfig & 0x80) > 0)
		return (-1);
	   break;
	case 3:
           if ((non_mbsfn_SubframeConfig & 0x40) > 0)
		return (-1);
	   break;
	case 4:
           if ((non_mbsfn_SubframeConfig & 0x20) > 0)
		return (-1);
	   break;
	case 5:
           if ((non_mbsfn_SubframeConfig & 0x10) > 0)
		return (-1);
	   break;
	case 6:
           if ((non_mbsfn_SubframeConfig & 0x8) > 0)
		return (-1);
	   break;
	case 7:
           if ((non_mbsfn_SubframeConfig & 0x4) > 0)
		return (-1);
	   break;
	case 8:
           if ((non_mbsfn_SubframeConfig & 0x2) > 0)
		return (-1);
	   break;
	case 9:
           if ((non_mbsfn_SubframeConfig & 0x1) > 0)
		return (-1);
	   break;
	}
  }

  if (UE_mac_inst[module_idP].pmch_Config[0]) {
    //  Find the first subframe in this MCH to transmit MSI
    if (frameP % 1 == 0) {
      if (frameP % mch_scheduling_period == 0) {
	msi_pos=0;
	if((frameP&3)==0)
		msi_pos++;
	while((non_mbsfn_SubframeConfig & (0x100 >> msi_pos)) == (0x100>>msi_pos))
		msi_pos++;
	if(msi_pos == subframe){
		UE_mac_inst[module_idP].common_num_sf_alloc=0;
		mbms_mch_i=0;
		//LOG_W(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	}
      }
    }
  }

  if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		msi_pos=subframe;
		mbms_mch_i++;
	//	LOG_W(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	}
  }


        // Check if the subframe is for MSI, MCCH or MTCHs and Set the correspoding flag to 1
  switch (subframe) {
    case 0:
          if (msi_pos == 0) {
            msi_flag = 1;
          }
          mtch_flag = 1;
      break;
    case 1:
          if (msi_pos == 1) {
            msi_flag = 1;
          }
          if ((frameP % mcch_period == mcch_offset) &&
              ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1)) {
            mcch_flag = 1;
          }

          mtch_flag = 1;
      break;

    case 2:
          if (msi_pos == 2) {
            msi_flag = 1;
          }
          if ((frameP % mcch_period == mcch_offset) &&
              ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF2) == MBSFN_FDD_SF2)) {
            mcch_flag = 1;
          }

          mtch_flag = 1;

      break;

    case 3:
          if (msi_pos == 3) {
            msi_flag = 1;
          }
          if ((frameP % mcch_period == mcch_offset) &&
              ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3)) {
            mcch_flag = 1;
          }

          mtch_flag = 1;

      break;
    case 4:
          if (msi_pos == 4) {
            msi_flag = 1;
          }
          mtch_flag = 1;
      break;
    case 5:
          if (msi_pos == 5) {
            msi_flag = 1;
          }
          mtch_flag = 1;
      break;

    case 6:
          if (msi_pos == 6) {
            msi_flag = 1;
          }
          if ((frameP % mcch_period == mcch_offset) &&
              ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF6) == MBSFN_FDD_SF6)) {
            mcch_flag = 1;
          }

          mtch_flag = 1;

      break;

    case 7:
          if (msi_pos == 7) {
            msi_flag = 1;
          }
          if ((frameP % mcch_period == mcch_offset) &&
              ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7)) {
            mcch_flag = 1;
          }

          mtch_flag = 1;

      break;

    case 8:
          if (msi_pos == 8) {
            msi_flag = 1;
          }
          if ((frameP % mcch_period == mcch_offset) &&
              ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8)) {
            mcch_flag = 1;
          }

          mtch_flag = 1;
      break;

    case 9:
          if (msi_pos == 9) {
            msi_flag = 1;
          }
          mtch_flag = 1;
      break;
  }// end switch

  if( (msi_flag == 1) || (mcch_flag == 1) || (mtch_flag == 1) ){
	UE_mac_inst[module_idP].common_num_sf_alloc++;
	//if( (msi_flag!=1 && mcch_flag!=1) || (msi_flag!=1 && mcch_flag!=1 && mtch_flag!=1)  ){
		//UE_mac_inst[module_idP].common_num_sf_alloc++;
                //if(msi_flag == 1)
                //   if((num_msi_per_CSA[mbms_mch_i]--)==0){
                //           LOG_E(MAC,"Both MSI and MTCH Should be reset?\n");
                //           msi_flag=0;
                //}
	//}
 
  }



  
#ifdef OLD
          // Acount for sf_allocable in CSA
        int num_sf_alloc = 0;

        for (l = 0; l < 8; l++) {
          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
            continue;

          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.present != LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame)
            continue;

          uint32_t common_mbsfn_SubframeConfig = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.choice.oneFrame.buf[0];


          for (j = 0; j < 6; j++)
            num_sf_alloc += ((common_mbsfn_SubframeConfig & (0x80 >> j)) == (0x80 >> j));
            //num_sf_alloc=1;
        }
	
	num_sf_alloc = 10;

        num_non_mbsfn_SubframeConfig = (UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]>>7);
	int ones=0;
	for(j=0; j < 16; j++){
		if(num_non_mbsfn_SubframeConfig & 1)
			ones++;
		num_non_mbsfn_SubframeConfig>>=1;
	}

        for (l = 0; l < 28; l++) {
          if (UE_mac_inst[module_idP].pmch_stop_mtch[l] >= 1/*num_sf_alloc*/) {
            if (UE_mac_inst[module_idP].pmch_stop_mtch[l] != 2047) {
              if (UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch] != NULL){
                mtch_mcs = UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch]->dataMCS_r9;
    	       //int common_mbsfn_alloc_offset = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]->radioframeAllocationOffset;
               long common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]->radioframeAllocationPeriod;
               long commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;
               if(UE_mac_inst[module_idP].common_num_sf_alloc >= UE_mac_inst[module_idP].pmch_stop_mtch[l]){
                       //LOG_E(MAC,"Attemp to UE_mac_inst[module_idP].common_num_sf_alloc %d\n",UE_mac_inst[module_idP].common_num_sf_alloc);

                       mtch_mcs = -1;
               }/*else
                       OG_W(MAC,"Attemp to UE_mac_inst[module_idP].common_num_sf_alloc %d\n",UE_mac_inst[module_idP].common_num_sf_alloc);*/
                       LOG_D(MAC,"Attemp to UE_mac_inst[module_idP].common_num_sf_alloc %d num_sf_alloc %d commonSF_AllocPeriod %ld, common_mbsfn_period %ld num_non_mbsfn_SubframeConfig %x ones %d\n",UE_mac_inst[module_idP].common_num_sf_alloc,num_sf_alloc, commonSF_AllocPeriod,common_mbsfn_period,num_non_mbsfn_SubframeConfig,ones);
                UE_mac_inst[module_idP].common_num_sf_alloc++;
                UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period-(1+ones)*(commonSF_AllocPeriod/4)); //TODO
              }
              else
                mtch_mcs = -1;

              mch_lcid = (uint8_t)l;
              break;
            }
          }
        }
#endif

{
 	 // Acount for sf_allocable in Common SF Allocation
 	 int num_sf_alloc = 0;

	 for (l = 0; l < 8; l++) {
          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
            continue;

          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.present != LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame)
            continue;

    	  //int common_mbsfn_alloc_offset = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationOffset;
          long common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationPeriod;
          long commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;


  	  num_sf_alloc =10*commonSF_AllocPeriod/common_mbsfn_period;
  	  num_sf_alloc -=1*((commonSF_AllocPeriod/common_mbsfn_period)/4); //CAS sfs
          uint32_t num_non_mbsfn_SubframeConfig2 = (UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | UE_mac_inst[module_idP].non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]>>7);
  	  for (j = 0; j < 10; j++)
      		num_sf_alloc -= ((num_non_mbsfn_SubframeConfig2 & (0x200 >> j)) == (0x200 >> j))*((commonSF_AllocPeriod/common_mbsfn_period)/period);

	  }
	
	 for (l = 0; l < 8; l++) {
          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
            continue;

          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.present != LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame)
            continue;

    	  //int common_mbsfn_alloc_offset = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationOffset;
          long common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationPeriod;
          //long commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;

         
	  if(msi_flag==1 && UE_mac_inst[module_idP].pmch_Config[mbms_mch_i] != NULL){
                LOG_D(MAC,"msi(%d) should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),sf_AllocEnd_r9(%ld),common_num_sf_alloc(%d)\n",mbms_mch_i,frameP,subframe,num_sf_alloc,UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9,
                UE_mac_inst[module_idP].common_num_sf_alloc);
          	UE_mac_inst[module_idP].msi_current_alloc = UE_mac_inst[module_idP].common_num_sf_alloc;//num_sf_alloc;
          	UE_mac_inst[module_idP].msi_pmch = mbms_mch_i;
	  }else if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i] != NULL){
	          if (UE_mac_inst[module_idP].pmch_stop_mtch[mbms_mch_i] >= UE_mac_inst[module_idP].common_num_sf_alloc/*num_sf_alloc*/) {
	            if (UE_mac_inst[module_idP].pmch_stop_mtch[mbms_mch_i] != 2047) {
	              mtch_flag = 1;
	              if (UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch] != NULL)
	                mtch_mcs = UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch]->dataMCS_r9;
	              else
	                mtch_mcs = -1;
	              mch_lcid = (uint8_t)mbms_mch_i;
	              LOG_D(MAC,"mtch should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),mtch_mcs(%d),pmch_stop_mtch(%d),lcid(%d),msi_pmch(%d)\n",frameP,subframe,num_sf_alloc,mtch_mcs,
	                    UE_mac_inst[module_idP].pmch_stop_mtch[mbms_mch_i],UE_mac_inst[module_idP].pmch_lcids[mbms_mch_i],UE_mac_inst[module_idP].msi_pmch);
	            }
	          }
	  }
  	  UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % ((num_sf_alloc-mbms_mch_i) /** commonSF_AllocPeriod *// common_mbsfn_period);//48;

	  if(mtch_flag)
		break;
	}
	if(msi_flag==1){
		 for (l = 0; l < 8; l++) {
          		if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
            		continue;

		        if ( (/*AJ*/ (/*V*/ ( /*U*/ (frameP %(4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9)) ) / 8 ) % ((8 <<UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->mch_SchedulingPeriod_r9) / 8 ) ) != 0 ){
                                       msi_flag=0;
                                       LOG_D(MAC,"frameP %d subframeP %d reset(%d)\n",frameP, subframe, mbms_mch_i);
                        }
		}
	}
}

  // sf allocation is non-overlapping
  if ((msi_flag==1) || (mcch_flag==1) || (mtch_flag==1)) {
    LOG_D(MAC,"[UE %d] Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d\n",
          module_idP, frameP, subframe,l,j,msi_flag,mcch_flag,mtch_flag);
    *sync_area=i;
  }

  //if(msi_flag == 1)
  //   if((num_msi_per_CSA[mbms_mch_i]--)==0){
  //           LOG_E(MAC,"Both MSI and MTCH Should be reset?\n");
  //           msi_flag=mtch_flag=0;
  //}


  if ((mcch_flag == 1)) { // || (msi_flag==1))
    *mcch_active = 1;
  }

  if ( (mcch_flag==1) || ((msi_flag==1) && (UE_mac_inst[module_idP].mcch_status==1)) ) {
    if (msi_flag!=1) {
      for (i=0; i<8; i++)
        UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i] = NULL;

      for (i=0; i<15; i++)
        UE_mac_inst[module_idP].pmch_Config[i] = NULL;

      for (i=0; i<28 ; i++) {
        UE_mac_inst[module_idP].pmch_lcids[i] = -1;
        UE_mac_inst[module_idP].pmch_stop_mtch[i] = 2047;
        UE_mac_inst[module_idP].msi_status_v[i] = 0;
      }
    } else {
      for (i=0; i<28; i++) {
        UE_mac_inst[module_idP].pmch_lcids[i] = -1;
        UE_mac_inst[module_idP].pmch_stop_mtch[i] = 2047;
        UE_mac_inst[module_idP].msi_status_v[i] = 0;
      }
    }

    return mcch_mcs;
  } else if ((mtch_flag==1) && (UE_mac_inst[module_idP].msi_status_v[(mch_lcid > 27) ? 27 : mch_lcid] == 1)) {
    return mtch_mcs;
  } else {
    return -1;
  }
  


 // if(mcch_flag == 1 || msi_flag == 1 ){
 //       *mcch_active = 1;
 //       return (mcch_mcs);
 // }else if(mtch_flag == 1){
 //       mtch_mcs=-1;
 //       return (mtch_mcs);
 // } 

 // return -1;

}


int ue_query_p_mch(module_id_t module_idP, uint32_t frameP, uint32_t subframe, int *mtch_active, int *msi_active, uint8_t *mch_lcid) {
  int i, j, mtch_mcs = -1;
  int mtch_flag = 0;
  // Acount for sf_allocable in CSA
  int num_sf_alloc = 0;

  for (i = 0; i < 8; i++) {
    if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i] == NULL)
      continue;

    if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) // one-frameP format
      continue;

    // four-frameP format
    uint32_t common_mbsfn_SubframeConfig = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[2] |
                                           (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[1]<<8) |
                                           (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[0]<<16);

    for (j = 0; j < 24; j++)
      num_sf_alloc += ((common_mbsfn_SubframeConfig & (0x800000 >> j)) == (0x800000 >> j));
  }

  for (i = 0; i < 8; i++ ) {
    if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i] == NULL)
      continue;

    if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) // one-frameP format
      continue;

    // four-frameP format
    int common_mbsfn_alloc_offset = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->radioframeAllocationOffset;
    int common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->radioframeAllocationPeriod;
    int commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;
    uint32_t common_mbsfn_SubframeConfig = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[2] |
                                           (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[1]<<8) |
                                           (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[0]<<16);
    int jj = frameP % 4;

    if ((frameP % common_mbsfn_period) != (common_mbsfn_alloc_offset + jj))
      continue;

    if(UE_mac_inst[module_idP].tdd_Config == NULL) {
      switch (subframe) {
        case 1:
          if ((common_mbsfn_SubframeConfig & (0x800000 >> (jj*6))) == (0x800000 >> (jj*6))) {
            mtch_flag = 1;
            mtch_mcs = ue_query_p_mch_info(module_idP,frameP,subframe,common_mbsfn_period,common_mbsfn_alloc_offset,UE_mac_inst[module_idP].common_num_sf_alloc,mtch_active,msi_active,mch_lcid);
            UE_mac_inst[module_idP].common_num_sf_alloc++;
            UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period);//48;
          }

          break;

        case 2:
          if ((common_mbsfn_SubframeConfig & (0x400000 >> (jj*6))) == (0x400000 >> (jj*6))) {
            mtch_flag = 1;
            mtch_mcs = ue_query_p_mch_info(module_idP,frameP,subframe,common_mbsfn_period,common_mbsfn_alloc_offset,UE_mac_inst[module_idP].common_num_sf_alloc,mtch_active,msi_active,mch_lcid);
            UE_mac_inst[module_idP].common_num_sf_alloc++;
            UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period);//48;
          }

          break;

        case 3:
          if ((common_mbsfn_SubframeConfig & (0x200000 >> (jj*6))) == (0x200000 >> (jj*6))) {
            mtch_flag = 1;
            mtch_mcs = ue_query_p_mch_info(module_idP,frameP,subframe,common_mbsfn_period,common_mbsfn_alloc_offset,UE_mac_inst[module_idP].common_num_sf_alloc,mtch_active,msi_active,mch_lcid);
            UE_mac_inst[module_idP].common_num_sf_alloc++;
            UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period);//48;
          }

          break;

        case 6:
          if ((common_mbsfn_SubframeConfig & (0x100000 >> (jj*6))) == (0x100000 >> (jj*6))) {
            mtch_flag = 1;
            mtch_mcs = ue_query_p_mch_info(module_idP,frameP,subframe,common_mbsfn_period,common_mbsfn_alloc_offset,UE_mac_inst[module_idP].common_num_sf_alloc,mtch_active,msi_active,mch_lcid);
            UE_mac_inst[module_idP].common_num_sf_alloc++;
            UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period);//48;
          }

          break;

        case 7:
          if ((common_mbsfn_SubframeConfig & (0x80000 >> (jj*6))) == (0x80000 >> (jj*6))) {
            mtch_flag = 1;
            mtch_mcs = ue_query_p_mch_info(module_idP,frameP,subframe,common_mbsfn_period,common_mbsfn_alloc_offset,UE_mac_inst[module_idP].common_num_sf_alloc,mtch_active,msi_active,mch_lcid);
            UE_mac_inst[module_idP].common_num_sf_alloc++;
            UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period);//48;
          }

          break;

        case 8:
          if ((common_mbsfn_SubframeConfig & (0x40000 >> (jj*6))) == (0x40000 >> (jj*6))) {
            mtch_flag = 1;
            mtch_mcs = ue_query_p_mch_info(module_idP,frameP,subframe,common_mbsfn_period,common_mbsfn_alloc_offset,UE_mac_inst[module_idP].common_num_sf_alloc,mtch_active,msi_active,mch_lcid);
            UE_mac_inst[module_idP].common_num_sf_alloc++;
            UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc% ( num_sf_alloc*commonSF_AllocPeriod/common_mbsfn_period);//48;
          }

          break;
      }
    } else {
      // TODO TDD
    }

    if (mtch_flag == 1)
      break;
  }

  return mtch_mcs;
}


int ue_query_mch(module_id_t module_idP, uint8_t CC_id, uint32_t frameP, uint32_t subframe, uint8_t eNB_index,uint8_t *sync_area, uint8_t *mcch_active) {
  int i = 0, j = 0, ii = 0, jj = 0, msi_pos = 0, mcch_mcs = -1, mtch_mcs = -1;
  int l =0;
  int mcch_flag = 0, mtch_flag = 0, msi_flag = 0;
  long mch_scheduling_period = -1;
  uint8_t mch_lcid = 0;
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].ue_query_mch);
#endif

  if (UE_mac_inst[module_idP].pmch_Config[0]) {
    mch_scheduling_period = 8 << UE_mac_inst[module_idP].pmch_Config[0]->mch_SchedulingPeriod_r9;
  }

  for (i = 0;
       i < UE_mac_inst[module_idP].num_active_mbsfn_area;
       i++ ) {
    // assume, that there is always a mapping
    if ((j = ue_get_mbsfn_sf_alloction(module_idP,i,eNB_index)) == -1) {
      return -1; // continue;
    }

    ii = 0;
    msi_pos = 0;
    long mbsfn_period =  1 << UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->radioframeAllocationPeriod;
    long mbsfn_alloc_offset = UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->radioframeAllocationOffset;
    long mcch_period  = 32 << UE_mac_inst[module_idP].mbsfn_AreaInfo[j]->mcch_Config_r9.mcch_RepetitionPeriod_r9;
    long mcch_offset        = UE_mac_inst[module_idP].mbsfn_AreaInfo[j]->mcch_Config_r9.mcch_Offset_r9;
    LOG_D(MAC,
          "[UE %d] Frame %d subframe %d: Checking MBSFN Sync Area %d/%d with SF allocation %d/%d for MCCH and MTCH (mbsfn period %ld, mcch period %ld,mac sched period (%ld,%ld))\n",
          module_idP,frameP, subframe,i,UE_mac_inst[module_idP].num_active_mbsfn_area,
          j,UE_mac_inst[module_idP].num_sf_allocation_pattern,mbsfn_period,mcch_period,
          mcch_offset,mbsfn_alloc_offset);

    // get the real MCS value
    switch (UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.signallingMCS_r9) {
      case 0:
        mcch_mcs = 2;
        break;

      case 1:
        mcch_mcs = 7;
        break;

      case 2:
        mcch_mcs = 13;
        break;

      case 3:
        mcch_mcs = 19;
        break;
    }

    if (UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) { // one-frameP format
      if (frameP % mbsfn_period == mbsfn_alloc_offset) { // MBSFN frameP
        if (UE_mac_inst[module_idP].pmch_Config[0]) {
          //  Find the first subframe in this MCH to transmit MSI
          if (frameP % mch_scheduling_period == mbsfn_alloc_offset) {
            while (ii == 0) {
              ii = UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & (0x80 >> msi_pos);
              msi_pos++;
            }
	    if(msi_pos == subframe){
	    	UE_mac_inst[module_idP].common_num_sf_alloc=0;
	    	mbms_mch_i=0;
	    	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	    }
          }
        }

        // Check if the subframe is for MSI, MCCH or MTCHs and Set the correspoding flag to 1
        switch (subframe) {
          case 1:
            if (UE_mac_inst[module_idP].tdd_Config == NULL) {
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1) {
		if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	      	   if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
	      	   	msi_pos=1;
	      	   	mbms_mch_i++;
	      	   	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	      	   }
		}

                if (msi_pos == 1) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 2:
            if (UE_mac_inst[module_idP].tdd_Config == NULL) {
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF2) == MBSFN_FDD_SF2) {
		if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	      	   if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
	      	   	msi_pos=2;
	      	   	mbms_mch_i++;
	      	   	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	      	   }
		}

                if (msi_pos == 2) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF2) == MBSFN_FDD_SF2)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 3:
            if (UE_mac_inst[module_idP].tdd_Config != NULL) { // TDD
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF3) == MBSFN_TDD_SF3) {
                if (msi_pos == 1) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF3) == MBSFN_TDD_SF3)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            } else { // FDD
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3) {
		if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	      	   if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
	      	   	msi_pos=3;
	      	   	mbms_mch_i++;
	      	   	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	      	   }
		}

                if (msi_pos == 3) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 4:
            if (UE_mac_inst[module_idP].tdd_Config != NULL) {
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF4) == MBSFN_TDD_SF4) {
                if (msi_pos == 2) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF4) == MBSFN_TDD_SF4)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 6:
            if (UE_mac_inst[module_idP].tdd_Config == NULL) {
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF6) == MBSFN_FDD_SF6) {
		if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	      	   if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
	      	   	msi_pos=6;
	      	   	mbms_mch_i++;
	      	   	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	      	   }
		}

                if (msi_pos == 4) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF6) == MBSFN_FDD_SF6)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 7:
            if (UE_mac_inst[module_idP].tdd_Config != NULL) { // TDD
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF7) == MBSFN_TDD_SF7) {
                if (msi_pos == 3) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF7) == MBSFN_TDD_SF7)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            } else { // FDD
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7) {
		if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	      	   if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
	      	   	msi_pos=5;
	      	   	mbms_mch_i++;
	      	   	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	      	   }
		}

                if (msi_pos == 5) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 8:
            if (UE_mac_inst[module_idP].tdd_Config != NULL) { //TDD
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF8) == MBSFN_TDD_SF8) {
                if (msi_pos == 4) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF8) == MBSFN_TDD_SF8)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            } else { // FDD
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8) {
		if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i+1]!=NULL){
	      	   if( UE_mac_inst[module_idP].common_num_sf_alloc == UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
	      	   	msi_pos=6;
	      	   	mbms_mch_i++;
	      	   	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframe, msi_pos,mbms_mch_i);
	      	   }
		}
                if (msi_pos == 6) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;

          case 9:
            if (UE_mac_inst[module_idP].tdd_Config != NULL) {
              if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF9) == MBSFN_TDD_SF9) {
                if (msi_pos == 5) {
                  msi_flag = 1;
                }

                if ((frameP % mcch_period == mcch_offset) &&
                    ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF9) == MBSFN_TDD_SF9)) {
                  mcch_flag = 1;
                }

                mtch_flag = 1;
              }
            }

            break;
        }// end switch

 if( (msi_flag == 1) || (mcch_flag == 1) || (mtch_flag == 1) ){
	UE_mac_inst[module_idP].common_num_sf_alloc++;
	//if( (msi_flag!=1 && mcch_flag!=1) || (msi_flag!=1 && mcch_flag!=1 && mtch_flag!=1)  ){
		//UE_mac_inst[module_idP].common_num_sf_alloc++;
	//}
  }

#ifdef OLD
        // Acount for sf_allocable in CSA
        int num_sf_alloc = 0;

        for (l = 0; l < 8; l++) {
          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
            continue;

          if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.present != LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame)
            continue;

          uint32_t common_mbsfn_SubframeConfig = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.choice.oneFrame.buf[0];

          for (j = 0; j < 6; j++)
            num_sf_alloc += ((common_mbsfn_SubframeConfig & (0x80 >> j)) == (0x80 >> j));
	    //num_sf_alloc=1;
        }

        for (l = 0; l < 28; l++) {
          if (UE_mac_inst[module_idP].pmch_stop_mtch[l] >= 1/*num_sf_alloc*/) {
            if (UE_mac_inst[module_idP].pmch_stop_mtch[l] != 2047) {
              if (UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch] != NULL){
                mtch_mcs = UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch]->dataMCS_r9;
               long common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]->radioframeAllocationPeriod;
               long commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;
               if(UE_mac_inst[module_idP].common_num_sf_alloc >= UE_mac_inst[module_idP].pmch_stop_mtch[l]){
		       //LOG_E(MAC,"Attemp to UE_mac_inst[module_idP].common_num_sf_alloc %d\n",UE_mac_inst[module_idP].common_num_sf_alloc);

                       mtch_mcs = -1;
	       }/*else
		       LOG_W(MAC,"Attemp to UE_mac_inst[module_idP].common_num_sf_alloc %d\n",UE_mac_inst[module_idP].common_num_sf_alloc);*/
               	UE_mac_inst[module_idP].common_num_sf_alloc++;
           	UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % (num_sf_alloc * commonSF_AllocPeriod / common_mbsfn_period);
	      }
              else
                mtch_mcs = -1;

              mch_lcid = (uint8_t)l;
              break;
            }
          }
        }
#endif

{
 	 // Acount for sf_allocable in Common SF Allocation
 	 int num_sf_alloc = 0;

	 for (l = 0; l < 8; l++) {
             if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
               continue;

             if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.present != LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame)
               continue;

    	     //int common_mbsfn_alloc_offset = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationOffset;
             //long common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationPeriod;
             //long commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;

 	     uint32_t common_mbsfn_SubframeConfig = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.choice.oneFrame.buf[0];

             for (j = 0; j < 6; j++)
               num_sf_alloc += ((common_mbsfn_SubframeConfig & (0x80 >> j)) == (0x80 >> j));
	  }

	
	 for (l = 0; l < 8; l++) {
            if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
              continue;

            if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->subframeAllocation.present != LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame)
              continue;

    	    //int common_mbsfn_alloc_offset = UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationOffset;
            long common_mbsfn_period  = 1 << UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l]->radioframeAllocationPeriod;
            long commonSF_AllocPeriod = 4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9;

           
	    if(msi_flag==1 && UE_mac_inst[module_idP].pmch_Config[mbms_mch_i] != NULL){
                  LOG_D(MAC,"msi(%d) should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),sf_AllocEnd_r9(%ld),common_num_sf_alloc(%d)\n",mbms_mch_i,frameP,subframe,num_sf_alloc,UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->sf_AllocEnd_r9,
                  UE_mac_inst[module_idP].common_num_sf_alloc);
            	  UE_mac_inst[module_idP].msi_current_alloc = UE_mac_inst[module_idP].common_num_sf_alloc;//num_sf_alloc;
            	  UE_mac_inst[module_idP].msi_pmch = mbms_mch_i;
	    }else if(UE_mac_inst[module_idP].pmch_Config[mbms_mch_i] != NULL){
	            if (UE_mac_inst[module_idP].pmch_stop_mtch[mbms_mch_i] >= UE_mac_inst[module_idP].common_num_sf_alloc/*num_sf_alloc*/) {
	              if (UE_mac_inst[module_idP].pmch_stop_mtch[mbms_mch_i] != 2047) {
	                mtch_flag = 1;
	                if (UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch] != NULL)
	                  mtch_mcs = UE_mac_inst[module_idP].pmch_Config[UE_mac_inst[module_idP].msi_pmch]->dataMCS_r9;
	                else
	                  mtch_mcs = -1;
	                mch_lcid = (uint8_t)mbms_mch_i;
	                LOG_D(MAC,"mtch should be allocated:frame(%d),submframe(%d),num_sf_alloc(%d),mtch_mcs(%d),pmch_stop_mtch(%d),lcid(%d),msi_pmch(%d)\n",frameP,subframe,num_sf_alloc,mtch_mcs,
	                      UE_mac_inst[module_idP].pmch_stop_mtch[mbms_mch_i],UE_mac_inst[module_idP].pmch_lcids[mbms_mch_i],UE_mac_inst[module_idP].msi_pmch);
	              }
	            }
	    }
	    //LOG_D(MAC,"UE_mac_inst[module_idP].common_num_sf_alloc: %d num_sf_alloc %d mbms_mch_i %d commonSF_AllocPeriod %d common_mbsfn_period %d\n",UE_mac_inst[module_idP].common_num_sf_alloc,num_sf_alloc,mbms_mch_i,commonSF_AllocPeriod,common_mbsfn_period);
  	    UE_mac_inst[module_idP].common_num_sf_alloc = UE_mac_inst[module_idP].common_num_sf_alloc % ((num_sf_alloc* commonSF_AllocPeriod-mbms_mch_i) / common_mbsfn_period);//48;
	    

	    if(mtch_flag)
	  	break;
	}
	if(msi_flag==1){
		 for (l = 0; l < 8; l++) {
          		if (UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[l] == NULL)
            		continue;

		        if ( (/*AJ*/ (/*V*/ ( /*U*/ (frameP %(4 << UE_mac_inst[module_idP].commonSF_AllocPeriod_r9)) ) / 8 ) % ((8 <<UE_mac_inst[module_idP].pmch_Config[mbms_mch_i]->mch_SchedulingPeriod_r9) / 8 ) ) != 0 ){
                                       msi_flag=0;
                                       LOG_D(MAC,"frameP %d subframeP %d reset(%d)\n",frameP, subframe, mbms_mch_i);
                        }
		}
	}

}

        // sf allocation is non-overlapping
        if ((msi_flag==1) || (mcch_flag==1) || (mtch_flag==1)) {
          LOG_D(MAC,"[UE %d] Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d x %d\n",
                module_idP, frameP, subframe,l,j,msi_flag,mcch_flag,mtch_flag,UE_mac_inst[module_idP].common_num_sf_alloc);
          *sync_area=i;
          break;
        }
      }
    } else { // four-frameP format
      uint32_t mbsfn_SubframeConfig = UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[2] |
                                      (UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[1]<<8) |
                                      (UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0]<<16);
      uint32_t MCCH_mbsfn_SubframeConfig = /* UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[2] |
                                          (UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[1]<<8) | */
        (UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0]<<16);
      jj=frameP%4;

      if ((frameP % mbsfn_period) == (mbsfn_alloc_offset+jj)) {
        if (UE_mac_inst[module_idP].tdd_Config == NULL) {
          switch (subframe) {
            case 1:
              if ((mbsfn_SubframeConfig & (0x800000>>(jj*6))) == (0x800000>>(jj*6))) {
                if ((frameP % mcch_period == (mcch_offset+jj)) && ((MCCH_mbsfn_SubframeConfig & (0x800000>>(jj*6))) == (0x800000>>(jj*6)))) {
                  mcch_flag=1;
                  LOG_D(MAC,"frameP(%d),mcch_period(%ld),mbsfn_SubframeConfig(%x),MCCH_mbsfn_SubframeConfig(%x),mask(%x),jj(%d),num_sf_alloc(%d)\n",
                        frameP, mcch_period, mbsfn_SubframeConfig, MCCH_mbsfn_SubframeConfig, (0x800000>>(jj*6)), jj, UE_mac_inst[module_idP].common_num_sf_alloc);

                  if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]==NULL)
                    UE_mac_inst[module_idP].common_num_sf_alloc++;
                }
              }

              break;

            case 2:
              if ((mbsfn_SubframeConfig & (0x400000>>(jj*6))) == (0x400000>>(jj*6))) {
                if ((frameP % mcch_period == (mcch_offset+jj)) && ((MCCH_mbsfn_SubframeConfig & (0x400000>>(jj*6))) == (0x400000>>(jj*6)))) {
                  mcch_flag=1;
                  LOG_D(MAC,"frameP(%d),mcch_period(%ld),mbsfn_SubframeConfig(%x),MCCH_mbsfn_SubframeConfig(%x),mask(%x),jj(%d),num_sf_alloc(%d)\n",
                        frameP, mcch_period, mbsfn_SubframeConfig, MCCH_mbsfn_SubframeConfig, (0x400000>>(jj*6)), jj, UE_mac_inst[module_idP].common_num_sf_alloc);

                  if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]==NULL)
                    UE_mac_inst[module_idP].common_num_sf_alloc++;
                }
              }

              break;

            case 3:
              if ((mbsfn_SubframeConfig & (0x200000>>(jj*6))) == (0x200000>>(jj*6))) {
                if ((frameP % mcch_period == (mcch_offset+jj)) && ((MCCH_mbsfn_SubframeConfig & (0x200000>>(jj*6))) == (0x200000>>(jj*6)))) {
                  LOG_D(MAC,"frameP(%d),mcch_period(%ld),mbsfn_SubframeConfig(%x),MCCH_mbsfn_SubframeConfig(%x),mask(%x),jj(%d),num_sf_alloc(%d)\n",
                        frameP, mcch_period, mbsfn_SubframeConfig, MCCH_mbsfn_SubframeConfig, (0x200000>>(jj*6)), jj, UE_mac_inst[module_idP].common_num_sf_alloc);
                  mcch_flag=1;

                  if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]==NULL)
                    UE_mac_inst[module_idP].common_num_sf_alloc++;
                }
              }

              break;

            case 6:
              if ((mbsfn_SubframeConfig & (0x100000>>(jj*6))) == (0x100000>>(jj*6))) {
                if ((frameP % mcch_period == (mcch_offset+jj)) && ((MCCH_mbsfn_SubframeConfig & (0x100000>>(jj*6))) == (0x100000>>(jj*6)))) {
                  LOG_D(MAC,"frameP(%d),mcch_period(%ld),mbsfn_SubframeConfig(%x),MCCH_mbsfn_SubframeConfig(%x),mask(%x),jj(%d),num_sf_alloc(%d)\n",
                        frameP, mcch_period, mbsfn_SubframeConfig, MCCH_mbsfn_SubframeConfig, (0x100000>>(jj*6)), jj, UE_mac_inst[module_idP].common_num_sf_alloc);
                  mcch_flag=1;

                  if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]==NULL)
                    UE_mac_inst[module_idP].common_num_sf_alloc++;
                }
              }

              break;

            case 7:
              if ((mbsfn_SubframeConfig & (0x80000>>(jj*6))) == (0x80000>>(jj*6))) {
                if ((frameP % mcch_period == (mcch_offset+jj)) && ((MCCH_mbsfn_SubframeConfig & (0x80000>>(jj*6))) == (0x80000>>(jj*6)))) {
                  LOG_D(MAC,"frameP(%d),mcch_period(%ld),mbsfn_SubframeConfig(%x),MCCH_mbsfn_SubframeConfig(%x),mask(%x),jj(%d),num_sf_alloc(%d)\n",
                        frameP, mcch_period, mbsfn_SubframeConfig, MCCH_mbsfn_SubframeConfig, (0x80000>>(jj*6)), jj, UE_mac_inst[module_idP].common_num_sf_alloc);

                  if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]==NULL)
                    UE_mac_inst[module_idP].common_num_sf_alloc++;

                  mcch_flag=1;
                }
              }

              break;

            case 8:
              if ((mbsfn_SubframeConfig & (0x40000>>(jj*6))) == (0x40000>>(jj*6))) {
                if ((frameP % mcch_period == (mcch_offset+jj)) && ((MCCH_mbsfn_SubframeConfig & (0x40000>>(jj*6))) == (0x40000>>(jj*6)))) {
                  LOG_D(MAC,"frameP(%d),mcch_period(%ld),mbsfn_SubframeConfig(%x),MCCH_mbsfn_SubframeConfig(%x),mask(%x),jj(%d),num_sf_alloc(%d)\n",
                        frameP, mcch_period, mbsfn_SubframeConfig, MCCH_mbsfn_SubframeConfig, (0x40000>>(jj*6)), jj, UE_mac_inst[module_idP].common_num_sf_alloc);

                  if(UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[0]==NULL)
                    UE_mac_inst[module_idP].common_num_sf_alloc++;

                  mcch_flag=1;
                }
              }

              break;
          }// end switch
        } else {
          // TODO TDD
        }

        mtch_mcs = ue_query_p_mch(module_idP, frameP, subframe, &mtch_flag, &msi_flag, &mch_lcid);

        // sf allocation is non-overlapping
        if ((msi_flag==1) || (mcch_flag==1) || (mtch_flag==1)) {
          LOG_D(MAC,"[UE %d] Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d\n",
                module_idP, frameP, subframe,i,j,msi_flag,mcch_flag,mtch_flag);
          *sync_area=i;
          break;
        }
      }
    }
  } // end of for

#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].ue_query_mch);
#endif

  if ((mcch_flag == 1)) { // || (msi_flag==1))
    *mcch_active = 1;
  }

  if ( (mcch_flag==1) || ((msi_flag==1) && (UE_mac_inst[module_idP].mcch_status==1)) ) {
    if (msi_flag!=1) {
      for (i=0; i<8; i++)
        UE_mac_inst[module_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i] = NULL;

      for (i=0; i<15; i++)
        UE_mac_inst[module_idP].pmch_Config[i] = NULL;

      for (i=0; i<28 ; i++) {
        UE_mac_inst[module_idP].pmch_lcids[i] = -1;
        UE_mac_inst[module_idP].pmch_stop_mtch[i] = 2047;
        UE_mac_inst[module_idP].msi_status_v[i] = 0;
      }
    } else {
      for (i=0; i<28; i++) {
        UE_mac_inst[module_idP].pmch_lcids[i] = -1;
        UE_mac_inst[module_idP].pmch_stop_mtch[i] = 2047;
        UE_mac_inst[module_idP].msi_status_v[i] = 0;
      }
    }

    return mcch_mcs;
  } else if ((mtch_flag==1) && (UE_mac_inst[module_idP].msi_status_v[(mch_lcid > 27) ? 27 : mch_lcid] == 1)) {
    return mtch_mcs;
  } else {
    return -1;
  }
}

unsigned char
generate_ulsch_header(uint8_t *mac_header,
                      uint8_t num_sdus,
                      uint8_t short_padding,
                      uint16_t *sdu_lengths,
                      uint8_t *sdu_lcids,
                      POWER_HEADROOM_CMD *power_headroom,
                      uint16_t *crnti,
                      BSR_SHORT *truncated_bsr,
                      BSR_SHORT *short_bsr,
                      BSR_LONG *long_bsr, unsigned short post_padding) {
  SCH_SUBHEADER_FIXED *mac_header_ptr =
    (SCH_SUBHEADER_FIXED *) mac_header;
  unsigned char first_element = 0, last_size = 0, i;
  unsigned char mac_header_control_elements[16], *ce_ptr;
  LOG_D(MAC, "[UE] Generate ULSCH : num_sdus %d\n", num_sdus);
#ifdef DEBUG_HEADER_PARSING

  for (i = 0; i < num_sdus; i++) {
    LOG_T(MAC, "[UE] sdu %d : lcid %d length %d", i, sdu_lcids[i],
          sdu_lengths[i]);
  }

  LOG_T(MAC, "\n");
#endif
  ce_ptr = &mac_header_control_elements[0];

  if ((short_padding == 1) || (short_padding == 2)) {
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    first_element = 1;
    last_size = 1;
  }

  if (short_padding == 2) {
    mac_header_ptr->E = 1;
    mac_header_ptr++;
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    last_size = 1;
  }

  if (power_headroom) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = POWER_HEADROOM;
    last_size = 1;
    *((POWER_HEADROOM_CMD *) ce_ptr) = (*power_headroom);
    ce_ptr += sizeof(POWER_HEADROOM_CMD);
    LOG_D(MAC, "phr header size %zu\n", sizeof(POWER_HEADROOM_CMD));
  }

  if (crnti) {
#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC, "[UE] CRNTI : %x (first_element %d)\n", *crnti,
          first_element);
#endif

    if (first_element > 0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = CRNTI;
    last_size = 1;
    *((uint16_t *) ce_ptr) = (*crnti);
    ce_ptr += sizeof(uint16_t);
    //    printf("offset %d\n",ce_ptr-mac_header_control_elements);
  }

  if (truncated_bsr) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      /*
         printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
       */
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC, "[UE] Scheduler Truncated BSR Header\n");
#endif
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = TRUNCATED_BSR;
    last_size = 1;
    *((BSR_TRUNCATED *) ce_ptr) = (*truncated_bsr);
    ce_ptr += sizeof(BSR_TRUNCATED);
    //    printf("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  } else if (short_bsr) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      /*
         printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
       */
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC, "[UE] Scheduler SHORT BSR Header\n");
#endif
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_BSR;
    last_size = 1;
    *((BSR_SHORT *) ce_ptr) = (*short_bsr);
    ce_ptr += sizeof(BSR_SHORT);
    //    printf("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  } else if (long_bsr) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      /*
         printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
         ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
       */
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC, "[UE] Scheduler Long BSR Header\n");
#endif
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = LONG_BSR;
    last_size = 1;
    *(ce_ptr) =
      (long_bsr->
       Buffer_size0 << 2) | ((long_bsr->Buffer_size1 & 0x30) >> 4);
    *(ce_ptr + 1) =
      ((long_bsr->Buffer_size1 & 0x0F) << 4) | ((long_bsr->
          Buffer_size2 & 0x3C)
          >> 2);
    *(ce_ptr + 2) =
      ((long_bsr->
        Buffer_size2 & 0x03) << 2) | (long_bsr->Buffer_size3 & 0x3F);
    ce_ptr += BSR_LONG_SIZE;
    //    printf("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  }

  //  printf("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

  for (i = 0; i < num_sdus; i++) {
#ifdef DEBUG_HEADER_PARSING
    LOG_T(MAC, "[UE] sdu subheader %d (lcid %d, %d bytes)\n", i,
          sdu_lcids[i], sdu_lengths[i]);
#endif

    if ((i == (num_sdus - 1))
        && ((short_padding) || (post_padding == 0))) {
      if (first_element > 0) {
        mac_header_ptr->E = 1;
#ifdef DEBUG_HEADER_PARSING
        LOG_D(MAC, "[UE] last subheader : %x (R%d,E%d,LCID%d)\n",
              *(unsigned char *) mac_header_ptr,
              ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->R,
              ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->E,
              ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->LCID);
#endif
        mac_header_ptr += last_size;
      }

      mac_header_ptr->R = 0;
      mac_header_ptr->E = 0;
      mac_header_ptr->LCID = sdu_lcids[i];
    } else {
      if ((first_element > 0)) {
        mac_header_ptr->E = 1;
#ifdef DEBUG_HEADER_PARSING
        LOG_D(MAC, "[UE] last subheader : %x (R%d,E%d,LCID%d)\n",
              *(unsigned char *) mac_header_ptr,
              ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->R,
              ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->E,
              ((SCH_SUBHEADER_FIXED *) mac_header_ptr)->LCID);
#endif
        mac_header_ptr += last_size;
        //      printf("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);
      } else {
        first_element = 1;
      }

      if (sdu_lengths[i] < 128) {
        ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->R = 0;  // 3
        ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->E = 0;
        ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->F = 0;
        ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->LCID =
          sdu_lcids[i];
        ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L =
          (unsigned char) sdu_lengths[i];
        last_size = 2;
#ifdef DEBUG_HEADER_PARSING
        LOG_D(MAC, "[UE] short sdu\n");
        LOG_T(MAC,
              "[UE] last subheader : %x (R%d,E%d,LCID%d,F%d,L%d)\n",
              ((uint16_t *) mac_header_ptr)[0],
              ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->R,
              ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->E,
              ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->LCID,
              ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->F,
              ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L);
#endif
      } else {
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->R = 0;
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->E = 0;
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->F = 1;
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->LCID =
          sdu_lcids[i];
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_MSB =
          ((unsigned short) sdu_lengths[i] >> 8) & 0x7f;
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_LSB =
          (unsigned short) sdu_lengths[i] & 0xff;
        ((SCH_SUBHEADER_LONG *) mac_header_ptr)->padding = 0x00;
        last_size = 3;
#ifdef DEBUG_HEADER_PARSING
        LOG_D(MAC, "[UE] long sdu\n");
#endif
      }
    }
  }

  if (post_padding > 0) { // we have lots of padding at the end of the packet
    mac_header_ptr->E = 1;
    mac_header_ptr += last_size;
    // add a padding element
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    mac_header_ptr++;
  } else {      // no end of packet padding
    // last SDU subhead is of fixed type (sdu length implicitly to be computed at UE)
    mac_header_ptr++;
    //mac_header_ptr=last_size; // FIXME: should be ++
  }

  if ((ce_ptr - mac_header_control_elements) > 0) {
    memcpy((void *) mac_header_ptr, mac_header_control_elements,
           ce_ptr - mac_header_control_elements);
    mac_header_ptr +=
      (unsigned char) (ce_ptr - mac_header_control_elements);
  }

#ifdef DEBUG_HEADER_PARSING
  LOG_T(MAC, " [UE] header : ");

  for (i = 0; i < ((unsigned char *) mac_header_ptr - mac_header); i++) {
    LOG_T(MAC, "%2x.", mac_header[i]);
  }

  LOG_T(MAC, "\n");
#endif
  return ((unsigned char *) mac_header_ptr - mac_header);
}

void
ue_get_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
           sub_frame_t subframe, uint8_t eNB_index,
           uint8_t *ulsch_buffer, uint16_t buflen, uint8_t *access_mode) {
  uint8_t total_rlc_pdu_header_len = 0, rlc_pdu_header_len_last = 0;
  uint16_t buflen_remain = 0;
  uint8_t bsr_len = 0, bsr_ce_len = 0, bsr_header_len = 0;
  uint8_t phr_header_len = 0, phr_ce_len = 0, phr_len = 0;
  uint8_t lcid = 0, lcid_rlc_pdu_count = 0;
  boolean_t is_lcid_processed = FALSE;
  boolean_t is_all_lcid_processed = FALSE;
  uint16_t sdu_lengths[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t sdu_lcids[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t payload_offset = 0, num_sdus = 0;
  uint8_t ulsch_buff[MAX_ULSCH_PAYLOAD_BYTES];
  uint16_t sdu_length_total = 0;
  BSR_SHORT bsr_short, bsr_truncated;
  BSR_LONG bsr_long;
  BSR_SHORT *bsr_s = &bsr_short;
  BSR_LONG *bsr_l = &bsr_long;
  BSR_SHORT *bsr_t = &bsr_truncated;
  POWER_HEADROOM_CMD phr;
  POWER_HEADROOM_CMD *phr_p = &phr;
  unsigned short short_padding = 0, post_padding = 0, padding_len = 0;
  int j;      // used for padding
  // Compute header length
  int lcg_id = 0;
  int lcg_id_bsr_trunc = 0;
  int highest_priority = 16;
  int num_lcg_id_with_data = 0;
  rlc_buffer_occupancy_t lcid_buffer_occupancy_old =
    0, lcid_buffer_occupancy_new = 0;
  LOG_D(MAC,
        "[UE %d] MAC PROCESS UL TRANSPORT BLOCK at frame%d subframe %d TBS=%d\n",
        module_idP, frameP, subframe, buflen);
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].tx_ulsch_sdu);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GET_SDU, VCD_FUNCTION_IN);
  bsr_header_len = 0;
  phr_header_len = 1;   //sizeof(SCH_SUBHEADER_FIXED);

  while (lcg_id < MAX_NUM_LCGID) {
    if (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcg_id]) {
      num_lcg_id_with_data++;
    }

    lcg_id++;
  }

  if (num_lcg_id_with_data) {
    LOG_D(MAC,
          "[UE %d] MAC Tx data pending at frame%d subframe %d nb LCG =%d Bytes for LCG0=%d LCG1=%d LCG2=%d LCG3=%d BSR Trigger status =%d TBS=%d\n",
          module_idP, frameP, subframe, num_lcg_id_with_data,
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[0],
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[1],
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[2],
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[3],
          UE_mac_inst[module_idP].BSR_reporting_active, buflen);
  }

  //Restart ReTxBSR Timer at new grant indication (36.321)
  if (UE_mac_inst[module_idP].scheduling_info.retxBSR_SF !=
      MAC_UE_BSR_TIMER_NOT_RUNNING) {
    UE_mac_inst[module_idP].scheduling_info.retxBSR_SF =
      get_sf_retxBSRTimer(UE_mac_inst[module_idP].
                          scheduling_info.retxBSR_Timer);
  }

  // periodicBSR-Timer expires, trigger BSR
  if ((UE_mac_inst[module_idP].scheduling_info.periodicBSR_Timer !=
       LTE_PeriodicBSR_Timer_r12_infinity)
      && (UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF == 0)) {
    // Trigger BSR Periodic
    UE_mac_inst[module_idP].BSR_reporting_active |=
      BSR_TRIGGER_PERIODIC;
    LOG_D(MAC,
          "[UE %d] MAC BSR Triggered PeriodicBSR Timer expiry at frame%d subframe %d TBS=%d\n",
          module_idP, frameP, subframe, buflen);
  }

  //Compute BSR Length if Regular or Periodic BSR is triggered
  //WARNING: if BSR long is computed, it may be changed to BSR short during or after multiplexing if there remains less than 1 LCGROUP with data after Tx
  if (UE_mac_inst[module_idP].BSR_reporting_active) {
    AssertFatal((UE_mac_inst[module_idP].BSR_reporting_active &
                 BSR_TRIGGER_PADDING) == 0,
                "Inconsistent BSR Trigger=%d !\n",
                UE_mac_inst[module_idP].BSR_reporting_active);

    if (buflen >= 4) {
      //A Regular or Periodic BSR can only be sent if TBS >= 4 as transmitting only a BSR is not allowed if UE has data to transmit
      bsr_header_len = 1;

      if (num_lcg_id_with_data <= 1) {
        bsr_ce_len = sizeof(BSR_SHORT); //1 byte
      } else {
        bsr_ce_len = BSR_LONG_SIZE; //3 bytes
      }
    }
  }

  bsr_len = bsr_ce_len + bsr_header_len;
  phr_ce_len =
    (UE_mac_inst[module_idP].PHR_reporting_active ==
     1) ? 1 /* sizeof(POWER_HEADROOM_CMD) */ : 0;

  if ((phr_ce_len > 0)
      && ((phr_ce_len + phr_header_len + bsr_len) <= buflen)) {
    phr_len = phr_ce_len + phr_header_len;
    LOG_D(MAC,
          "[UE %d] header size info: PHR len %d (ce%d,hdr%d) buff_len %d\n",
          module_idP, phr_len, phr_ce_len, phr_header_len, buflen);
  } else {
    phr_len = 0;
    phr_header_len = 0;
    phr_ce_len = 0;
  }

  // check for UL bandwidth requests and add SR control element

  // check for UL bandwidth requests and add SR control element

  // Check for DCCH first
  // TO DO: Multiplex in the order defined by the logical channel prioritization
  for (lcid = DCCH;
       (lcid < MAX_NUM_LCID) && (is_all_lcid_processed == FALSE); lcid++) {
    if (UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid] ==
        LCID_NOT_EMPTY) {
      lcid_rlc_pdu_count = 0;
      is_lcid_processed = FALSE;
      lcid_buffer_occupancy_old =
        mac_rlc_get_buffer_occupancy_ind(module_idP,
                                         UE_mac_inst[module_idP].
                                         crnti, eNB_index, frameP,
                                         subframe, ENB_FLAG_NO,
                                         lcid);
      lcid_buffer_occupancy_new = lcid_buffer_occupancy_old;
#if 0
      /* TODO: those assert crash the L2 simulator with the new RLC.
       *       Are they necessary?
       */
      AssertFatal(lcid_buffer_occupancy_new ==
                  UE_mac_inst[module_idP].
                  scheduling_info.LCID_buffer_remain[lcid],
                  "LCID=%d RLC has BO %d bytes but MAC has stored %d bytes\n",
                  lcid, lcid_buffer_occupancy_new,
                  UE_mac_inst[module_idP].
                  scheduling_info.LCID_buffer_remain[lcid]);
      AssertFatal(lcid_buffer_occupancy_new <=
                  UE_mac_inst[module_idP].
                  scheduling_info.BSR_bytes[UE_mac_inst[module_idP].
                                            scheduling_info.LCGID
                                            [lcid]],
                  "LCID=%d RLC has more BO %d bytes than BSR = %d bytes\n",
                  lcid, lcid_buffer_occupancy_new,
                  UE_mac_inst[module_idP].
                  scheduling_info.BSR_bytes[UE_mac_inst[module_idP].
                                            scheduling_info.LCGID
                                            [lcid]]);
#endif

      //Multiplex all available DCCH RLC PDUs considering to multiplex the last PDU each time for maximize the data
      //Adjust at the end of the loop
      while ((!is_lcid_processed) && (lcid_buffer_occupancy_new)
             && (bsr_len + phr_len + total_rlc_pdu_header_len +
                 sdu_length_total + MIN_MAC_HDR_RLC_SIZE <= buflen)) {
        // Workaround for issue in OAI eNB or EPC which are not able to process SRB2 message multiplexed with SRB1 on the same MAC PDU
        if (( get_softmodem_params()->usim_test == 0) && (lcid == DCCH1)
            && (lcid_rlc_pdu_count == 0) && (num_sdus)) {
          // Skip SRB2 multiplex if at least one SRB1 SDU is already multiplexed
          break;
        }

        buflen_remain =
          buflen - (bsr_len + phr_len +
                    total_rlc_pdu_header_len + sdu_length_total +
                    1);
        LOG_D(MAC,
              "[UE %d] Frame %d : UL-DXCH -> ULSCH, RLC %d has %d bytes to "
              "send (Transport Block size %d BSR size=%d PHR=%d SDU Length Total %d , mac header len %d BSR byte before Tx=%d)\n",
              module_idP, frameP, lcid, lcid_buffer_occupancy_new,
              buflen, bsr_len, phr_len, sdu_length_total,
              total_rlc_pdu_header_len,
              UE_mac_inst[module_idP].
              scheduling_info.BSR_bytes[UE_mac_inst[module_idP].
                                        scheduling_info.LCGID
                                        [lcid]]);
        AssertFatal(num_sdus < sizeof(sdu_lengths) / sizeof(sdu_lengths[0]), "num_sdus %d > num sdu_length elements\n", num_sdus);
        sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                UE_mac_inst
                                [module_idP].
                                crnti, eNB_index,
                                frameP,
                                ENB_FLAG_NO,
                                MBMS_FLAG_NO,
                                lcid,
                                buflen_remain,
                                (char *)&ulsch_buff[sdu_length_total],0,
                                0
                                                );
        AssertFatal(buflen_remain >= sdu_lengths[num_sdus],
                    "LCID=%d RLC has segmented %d bytes but MAC has max=%d\n",
                    lcid, sdu_lengths[num_sdus], buflen_remain);

        if (sdu_lengths[num_sdus]) {
          sdu_length_total += sdu_lengths[num_sdus];
          sdu_lcids[num_sdus] = lcid;
          //LOG_I(MAC,
          //      "[UE %d] TX Multiplex RLC PDU TX Got %d bytes for LcId%d\n",
          //      module_idP, sdu_lengths[num_sdus], lcid);

          if (buflen ==
              (bsr_len + phr_len + total_rlc_pdu_header_len +
               sdu_length_total + 1)) {
            //No more remaining TBS after this PDU
            //exit the function
            rlc_pdu_header_len_last = 1;
            is_lcid_processed = TRUE;
            is_all_lcid_processed = TRUE;
          } else {
            rlc_pdu_header_len_last =
              (sdu_lengths[num_sdus] > 128) ? 3 : 2;

            //Change to 1 byte if it does not fit in the TBS, ie last PDU
            if (buflen <=
                (bsr_len + phr_len + total_rlc_pdu_header_len +
                 rlc_pdu_header_len_last + sdu_length_total)) {
              rlc_pdu_header_len_last = 1;
              is_lcid_processed = TRUE;
              is_all_lcid_processed = TRUE;
            }
          }

          //Update number of SDU
          num_sdus++;
          //Update total MAC Header size for RLC PDUs and save last one
          total_rlc_pdu_header_len += rlc_pdu_header_len_last;
          lcid_rlc_pdu_count++;
        } else {
          /* avoid infinite loop ... */
          is_lcid_processed = TRUE;
        }

        /* Get updated BO after multiplexing this PDU */
        lcid_buffer_occupancy_new =
          mac_rlc_get_buffer_occupancy_ind(module_idP,
                                           UE_mac_inst
                                           [module_idP].crnti,
                                           eNB_index, frameP,
                                           subframe, ENB_FLAG_NO,
                                           lcid);
        is_lcid_processed = (is_lcid_processed)
                            || (lcid_buffer_occupancy_new <= 0);
      }

      //Update Buffer remain and BSR bytes after transmission
      UE_mac_inst[module_idP].scheduling_info.LCID_buffer_remain[lcid] = lcid_buffer_occupancy_new;
      UE_mac_inst[module_idP].scheduling_info.BSR_bytes[UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]] += (lcid_buffer_occupancy_new - lcid_buffer_occupancy_old);
      if (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]] < 0)
        UE_mac_inst[module_idP].scheduling_info.BSR_bytes[UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]] = 0;

      //Update the number of LCGID with data as BSR shall reflect status after BSR transmission
      if ((num_lcg_id_with_data > 1)
          && (UE_mac_inst[module_idP].
              scheduling_info.BSR_bytes[UE_mac_inst[module_idP].
                                        scheduling_info.LCGID[lcid]]
              == 0)) {
        num_lcg_id_with_data--;

        // Change BSR size to BSR SHORT if num_lcg_id_with_data becomes to 1
        if ((bsr_len) && (num_lcg_id_with_data == 1)) {
          bsr_ce_len = sizeof(BSR_SHORT);
          bsr_len = bsr_ce_len + bsr_header_len;
        }
      }

      UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid] =
        LCID_EMPTY;
    }
  }

  // Compute BSR Values and update Nb LCGID with data after multiplexing
  num_lcg_id_with_data = 0;
  lcg_id_bsr_trunc = 0;

  for (lcg_id = 0; lcg_id < MAX_NUM_LCGID; lcg_id++) {
    UE_mac_inst[module_idP].scheduling_info.BSR[lcg_id] =
      locate_BsrIndexByBufferSize(BSR_TABLE, BSR_TABLE_SIZE,
                                  UE_mac_inst
                                  [module_idP].scheduling_info.
                                  BSR_bytes[lcg_id]);

    if (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcg_id]) {
      num_lcg_id_with_data++;
      lcg_id_bsr_trunc = lcg_id;
    }
  }

  if (bsr_ce_len) {
    //Print updated BSR when sent
    LOG_D(MAC,
          "[UE %d] Remaining Buffer after Tx frame%d subframe %d nb LCG =%d Bytes for LCG0=%d LCG1=%d LCG2=%d LCG3=%d BSR Trigger status =%d TBS=%d\n",
          module_idP, frameP, subframe, num_lcg_id_with_data,
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[0],
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[1],
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[2],
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[3],
          UE_mac_inst[module_idP].BSR_reporting_active, buflen);
    LOG_D(MAC,
          "[UE %d] Frame %d Subframe %d TX BSR Regular or Periodic size=%d BSR0=%d BSR1=%d BSR2=%d BSR3=%d\n",
          module_idP, frameP, subframe, bsr_ce_len,
          UE_mac_inst[module_idP].scheduling_info.BSR[0],
          UE_mac_inst[module_idP].scheduling_info.BSR[1],
          UE_mac_inst[module_idP].scheduling_info.BSR[2],
          UE_mac_inst[module_idP].scheduling_info.BSR[3]);
  }

  // build PHR and update the timers
  if (phr_ce_len == sizeof(POWER_HEADROOM_CMD)) {
    if(NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) {
      //Substitute with a static value for the MAC layer abstraction (phy_stub mode)
      phr_p->PH = 60;
    } else {
      phr_p->PH = get_phr_mapping(module_idP, CC_id, eNB_index);
    }

    phr_p->R = 0;
    LOG_D(MAC,
          "[UE %d] Frame %d report PHR with mapping (%d->%d) for LCID %d\n",
          module_idP, frameP, get_PHR(module_idP, CC_id, eNB_index),
          phr_p->PH, POWER_HEADROOM);
    update_phr(module_idP, CC_id);
  } else {
    phr_p = NULL;
  }

  LOG_T(MAC, "[UE %d] Frame %d: bsr s %p bsr_l %p, phr_p %p\n",
        module_idP, frameP, bsr_s, bsr_l, phr_p);

  // Check BSR padding: it is done after PHR according to Logical Channel Prioritization order
  // Check for max padding size, ie MAC Hdr for last RLC PDU = 1
  /* For Padding BSR:
     -  if the number of padding bits is equal to or larger than the size of the Short BSR plus its subheader but smaller than the size of the Long BSR plus its subheader:
     -  if more than one LCG has data available for transmission in the TTI where the BSR is transmitted: report Truncated BSR of the LCG with the highest priority logical channel with data available for transmission;
     -  else report Short BSR.
     -  else if the number of padding bits is equal to or larger than the size of the Long BSR plus its subheader, report Long BSR.
   */
  if (sdu_length_total) {
    padding_len =
      buflen - (bsr_len + phr_len + total_rlc_pdu_header_len -
                rlc_pdu_header_len_last + sdu_length_total + 1);
  } else {
    padding_len = buflen - (bsr_len + phr_len);
  }

  if ((padding_len) && (bsr_len == 0)) {
    /* if the number of padding bits is equal to or larger than the size of the Long BSR plus its subheader, report Long BSR */
    if (padding_len >= (1 + BSR_LONG_SIZE)) {
      bsr_ce_len = BSR_LONG_SIZE;
      bsr_header_len = 1;
      // Trigger BSR Padding
      UE_mac_inst[module_idP].BSR_reporting_active |=
        BSR_TRIGGER_PADDING;
    } else if (padding_len >= (1 + sizeof(BSR_SHORT))) {
      bsr_ce_len = sizeof(BSR_SHORT);
      bsr_header_len = 1;

      if (num_lcg_id_with_data > 1) {
        // REPORT TRUNCATED BSR
        //Get LCGID of highest priority LCID with data
        for (lcid = DCCH; lcid < MAX_NUM_LCID; lcid++) {
          if (UE_mac_inst[module_idP].
              logicalChannelConfig[lcid] != NULL) {
            lcg_id =
              UE_mac_inst[module_idP].scheduling_info.
              LCGID[lcid];

            if ((lcg_id < MAX_NUM_LCGID)
                && (UE_mac_inst[module_idP].
                    scheduling_info.BSR_bytes[lcg_id])
                &&
                (UE_mac_inst[module_idP].logicalChannelConfig
                 [lcid]->ul_SpecificParameters->priority <=
                 highest_priority)) {
              highest_priority =
                UE_mac_inst[module_idP].
                logicalChannelConfig[lcid]->
                ul_SpecificParameters->priority;
              lcg_id_bsr_trunc = lcg_id;
            }
          }
        }
      } else {
        //Report SHORT BSR, clear bsr_t
        bsr_t = NULL;
      }

      // Trigger BSR Padding
      UE_mac_inst[module_idP].BSR_reporting_active |=
        BSR_TRIGGER_PADDING;
    }

    bsr_len = bsr_header_len + bsr_ce_len;
  }

  //Fill BSR Infos
  if (bsr_ce_len == 0) {
    bsr_s = NULL;
    bsr_l = NULL;
    bsr_t = NULL;
  } else if (bsr_ce_len == BSR_LONG_SIZE) {
    bsr_s = NULL;
    bsr_t = NULL;
    bsr_l->Buffer_size0 =
      UE_mac_inst[module_idP].scheduling_info.BSR[LCGID0];
    bsr_l->Buffer_size1 =
      UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1];
    bsr_l->Buffer_size2 =
      UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2];
    bsr_l->Buffer_size3 =
      UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3];
    LOG_D(MAC,
          "[UE %d] Frame %d subframe %d BSR Trig=%d report long BSR (level LCGID0 %d,level LCGID1 %d,level LCGID2 %d,level LCGID3 %d)\n",
          module_idP, frameP, subframe,
          UE_mac_inst[module_idP].BSR_reporting_active,
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID0],
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1],
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2],
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]);
  } else if (bsr_ce_len == sizeof(BSR_SHORT)) {
    bsr_l = NULL;

    if ((bsr_t != NULL)
        && (UE_mac_inst[module_idP].BSR_reporting_active &
            BSR_TRIGGER_PADDING)) {
      //Truncated BSR
      bsr_s = NULL;
      bsr_t->LCGID = lcg_id_bsr_trunc;
      bsr_t->Buffer_size =
        UE_mac_inst[module_idP].scheduling_info.
        BSR[lcg_id_bsr_trunc];
      LOG_D(MAC,
            "[UE %d] Frame %d subframe %d BSR Trig=%d report TRUNCATED BSR with level %d for LCGID %d\n",
            module_idP, frameP, subframe,
            UE_mac_inst[module_idP].BSR_reporting_active,
            UE_mac_inst[module_idP].
            scheduling_info.BSR[lcg_id_bsr_trunc], lcg_id_bsr_trunc);
    } else {
      bsr_t = NULL;
      bsr_s->LCGID = lcg_id_bsr_trunc;
      bsr_s->Buffer_size =
        UE_mac_inst[module_idP].scheduling_info.
        BSR[lcg_id_bsr_trunc];
      LOG_D(MAC,
            "[UE %d] Frame %d subframe %d BSR Trig=%d report SHORT BSR with level %d for LCGID %d\n",
            module_idP, frameP, subframe,
            UE_mac_inst[module_idP].BSR_reporting_active,
            UE_mac_inst[module_idP].
            scheduling_info.BSR[lcg_id_bsr_trunc], lcg_id_bsr_trunc);
    }
  }

  // 1-bit padding or 2-bit padding  special padding subheader
  // Check for max padding size, ie MAC Hdr for last RLC PDU = 1
  if (sdu_length_total) {
    padding_len =
      buflen - (bsr_len + phr_len + total_rlc_pdu_header_len -
                rlc_pdu_header_len_last + sdu_length_total + 1);
  } else {
    padding_len = buflen - (bsr_len + phr_len);
  }

  if (padding_len <= 2) {
    short_padding = padding_len;
    // only add padding header
    post_padding = 0;

    //update total MAC Hdr size for RLC data
    if (sdu_length_total) {
      total_rlc_pdu_header_len =
        total_rlc_pdu_header_len - rlc_pdu_header_len_last + 1;
      rlc_pdu_header_len_last = 1;
    }
  } else if (sdu_length_total) {
    post_padding =
      buflen - (bsr_len + phr_len + total_rlc_pdu_header_len +
                sdu_length_total + 1);

    // If by adding MAC Hdr for last RLC PDU the padding is 0 then set MAC Hdr for last RLC PDU = 1 and compute 1 or 2 byte padding
    if (post_padding == 0) {
      total_rlc_pdu_header_len -= rlc_pdu_header_len_last;
      padding_len =
        buflen - (bsr_len + phr_len + total_rlc_pdu_header_len +
                  sdu_length_total + 1);
      short_padding = padding_len;
      total_rlc_pdu_header_len++;
    }
  } else {
    if (padding_len == buflen) {  // nona mac pdu
      *access_mode = CANCELED_ACCESS;
    }

    short_padding = 0;
    post_padding =
      buflen - (bsr_len + phr_len + total_rlc_pdu_header_len +
                sdu_length_total + 1);
  }

  // Generate header
  // if (num_sdus>0) {
  payload_offset = generate_ulsch_header(ulsch_buffer,  // mac header
                                         num_sdus,  // num sdus
                                         short_padding, // short pading
                                         sdu_lengths, // sdu length
                                         sdu_lcids, // sdu lcid
                                         phr_p, // power headroom
                                         NULL,  // crnti
                                         bsr_t, // truncated bsr
                                         bsr_s, // short bsr
                                         bsr_l, post_padding);  // long_bsr
  LOG_D(MAC,
        "[UE %d] Generate header :bufflen %d  sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,  total_rlc_pdu_header_len %d, padding %d,post_padding %d, bsr len %d, phr len %d, reminder %d \n",
        module_idP, buflen, sdu_length_total, num_sdus, sdu_lengths[0],
        sdu_lcids[0], payload_offset, total_rlc_pdu_header_len,
        short_padding, post_padding, bsr_len, phr_len,
        buflen - sdu_length_total - payload_offset);

  // cycle through SDUs and place in ulsch_buffer
  if (sdu_length_total) {
    memcpy(&ulsch_buffer[payload_offset], ulsch_buff,
           sdu_length_total);
  }

  // fill remainder of DLSCH with random data
  if (post_padding) {
    for (j = 0; j < (buflen - sdu_length_total - payload_offset); j++) {
      ulsch_buffer[payload_offset + sdu_length_total + j] =
        (char) (taus() & 0xff);
    }
  }

  LOG_D(MAC,
        "[UE %d][SR] Gave SDU to PHY, clearing any scheduling request\n",
        module_idP);
  UE_mac_inst[module_idP].scheduling_info.SR_pending = 0;
  UE_mac_inst[module_idP].scheduling_info.SR_COUNTER = 0;

  /* Actions when a BSR is sent */
  if (bsr_ce_len) {
    LOG_D(MAC,
          "[UE %d] MAC BSR Sent !! bsr (ce%d,hdr%d) buff_len %d\n",
          module_idP, bsr_ce_len, bsr_header_len, buflen);
    // Reset ReTx BSR Timer
    UE_mac_inst[module_idP].scheduling_info.retxBSR_SF =
      get_sf_retxBSRTimer(UE_mac_inst[module_idP].
                          scheduling_info.retxBSR_Timer);
    LOG_D(MAC, "[UE %d] MAC ReTx BSR Timer Reset =%d\n", module_idP,
          UE_mac_inst[module_idP].scheduling_info.retxBSR_SF);

    // Reset Periodic Timer except when BSR is truncated
    if ((bsr_t == NULL)
        && (UE_mac_inst[module_idP].scheduling_info.
            periodicBSR_Timer != LTE_PeriodicBSR_Timer_r12_infinity)) {
      UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF =
        get_sf_periodicBSRTimer(UE_mac_inst
                                [module_idP].scheduling_info.
                                periodicBSR_Timer);
      LOG_D(MAC, "[UE %d] MAC Periodic BSR Timer Reset =%d\n",
            module_idP,
            UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF);
    }

    // Reset BSR Trigger flags
    UE_mac_inst[module_idP].BSR_reporting_active = BSR_TRIGGER_NONE;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GET_SDU, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].tx_ulsch_sdu);
#endif
  trace_pdu(DIRECTION_UPLINK, ulsch_buffer, buflen, module_idP, WS_C_RNTI,
            UE_mac_inst[module_idP].crnti,
            UE_mac_inst[module_idP].txFrame,
            UE_mac_inst[module_idP].txSubframe, 0, 0);
}



//------------------------------------------------------------------------------
// called at each subframe
// Performs :
// 1. Trigger PDCP every 5ms
// 2. Call RRC for link status return to PHY
// 3. Perform SR/BSR procedures for scheduling feedback
// 4. Perform PHR procedures

UE_L2_STATE_t
ue_scheduler(const module_id_t module_idP,
             const frame_t rxFrameP,
             const sub_frame_t rxSubframeP,
             const frame_t txFrameP,
             const sub_frame_t txSubframeP,
             const lte_subframe_t directionP,
             const uint8_t eNB_indexP, const int CC_id)
//------------------------------------------------------------------------------
{
  int lcid;     // lcid index
  int TTI = 1;
  int bucketsizeduration = -1;
  int bucketsizeduration_max = -1;
  // mac_rlc_status_resp_t rlc_status[MAX_NUM_LCGID]; // 4
  // int8_t lcg_id;
  struct LTE_RACH_ConfigCommon *rach_ConfigCommon =
    (struct LTE_RACH_ConfigCommon *) NULL;
  protocol_ctxt_t ctxt;
  MessageDef *msg_p;
  int result;
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_IN);
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_NO,
                                 UE_mac_inst[module_idP].crnti, txFrameP,
                                 txSubframeP, eNB_indexP);

  if(module_idP == 0) {
    do {
      // Checks if a message has been sent to MAC sub-task
      itti_poll_msg(TASK_MAC_UE, &msg_p);

      if (msg_p != NULL) {
        switch (ITTI_MSG_ID(msg_p)) {
          case RRC_MAC_CCCH_DATA_REQ:
            LOG_I(MAC,
                  "Received %s from %s: instance %ld, frameP %d, eNB_index %d\n",
                  ITTI_MSG_NAME(msg_p), ITTI_MSG_ORIGIN_NAME(msg_p), ITTI_MSG_DESTINATION_INSTANCE(msg_p),
                  RRC_MAC_CCCH_DATA_REQ(msg_p).frame,
                  RRC_MAC_CCCH_DATA_REQ(msg_p).enb_index);
            // TODO process CCCH data req.
            break;

          default:
            LOG_E(MAC, "Received unexpected message %s\n", ITTI_MSG_NAME(msg_p));
            break;
        }

        result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
        AssertFatal(result == EXIT_SUCCESS,
                    "Failed to free memory (%d)!\n", result);
      }
    } while (msg_p != NULL);
  }

  //Mac_rlc_xface->frameP=frameP;
  //Rrc_xface->Frame_index=Mac_rlc_xface->frameP;
  //if (subframe%5 == 0)
  //LG#ifdef EXMIMO

  // data to/from NETLINK is treated in pdcp_run.
  // one socket is used in multiple UE's L2 FAPI simulator and
  // only first UE need to do this.
  if (UE_NAS_USE_TUN) {
    pdcp_run(&ctxt);
  } else {
    if(module_idP == 0) {
      pdcp_run(&ctxt);
    }
  }

  void rlc_tick(int, int);
  rlc_tick(rxFrameP % 1024, rxSubframeP);

  //#endif
  UE_mac_inst[module_idP].txFrame = txFrameP;
  UE_mac_inst[module_idP].txSubframe = txSubframeP;
  UE_mac_inst[module_idP].rxFrame = rxFrameP;
  UE_mac_inst[module_idP].rxSubframe = rxSubframeP;
#ifdef CELLULAR
  rrc_rx_tx_ue(module_idP, txFrameP, 0, eNB_indexP);
#else

  switch (rrc_rx_tx_ue(&ctxt, eNB_indexP, CC_id)) {
    case RRC_OK:
      break;

    case RRC_ConnSetup_failed:
      LOG_E(MAC, "RRCConnectionSetup failed, returning to IDLE state\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
      stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
      return (CONNECTION_LOST);
      break;

    case RRC_PHY_RESYNCH:
      LOG_E(MAC, "RRC Loss of synch, returning PHY_RESYNCH\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
      stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
      return (PHY_RESYNCH);

    case RRC_Handover_failed:
      LOG_I(MAC, "Handover failure for UE %d eNB_index %d\n", module_idP,
            eNB_indexP);
      //Invalid...need to add another MAC UE state for re-connection procedure
      phy_config_afterHO_ue(module_idP, 0, eNB_indexP,
                            (LTE_MobilityControlInfo_t *) NULL, 1);
      //return(3);
      break;

    case RRC_HO_STARTED:
      LOG_I(MAC,
            "RRC handover, Instruct PHY to start the contention-free PRACH and synchronization\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
      stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
      return (PHY_HO_PRACH);

    default:
      break;
  }

#endif

  // Check Contention resolution timer (put in a function later)
  if (UE_mac_inst[module_idP].RA_contention_resolution_timer_active == 1) {
    if (UE_mac_inst[module_idP].radioResourceConfigCommon) {
      rach_ConfigCommon =
        &UE_mac_inst[module_idP].
        radioResourceConfigCommon->rach_ConfigCommon;
    } else {
      LOG_E(MAC, "FATAL: radioResourceConfigCommon is NULL!!!\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER,
       VCD_FUNCTION_OUT);
      stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
      AssertFatal(1 == 0, "");
#if UE_TIMING_TRACE
      stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
      //return(RRC_OK);
    }

    LOG_I(MAC, "Frame %d: Contention resolution timer %d/%ld\n",
          txFrameP,
          UE_mac_inst[module_idP].RA_contention_resolution_cnt,
          ((1 +
            rach_ConfigCommon->
            ra_SupervisionInfo.mac_ContentionResolutionTimer) << 3));
    UE_mac_inst[module_idP].RA_contention_resolution_cnt++;

    if (UE_mac_inst[module_idP].RA_contention_resolution_cnt ==
        ((1 +
          rach_ConfigCommon->
          ra_SupervisionInfo.mac_ContentionResolutionTimer) << 3)) {
      UE_mac_inst[module_idP].RA_active = 0;
      UE_mac_inst[module_idP].RA_contention_resolution_timer_active =
        0;
      // Signal PHY to quit RA procedure
      LOG_E(MAC,
            "Module id %u Contention resolution timer expired, RA failed\n",
            module_idP);

      if(NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) { // phy_stub mode
        // Modification for phy_stub mode operation here. We only need to make sure that the ue_mode is back to
        // PRACH state.
        LOG_I(MAC, "nfapi_mode3: Setting UE_mode to PRACH 2 \n");
        UE_mac_inst[module_idP].UE_mode[eNB_indexP] = PRACH;
        //ra_failed(module_idP,CC_id,eNB_index);UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
      } else {
        ra_failed(module_idP, CC_id, eNB_indexP);
      }

      //ra_failed(module_idP, 0, eNB_indexP);
    }
  }

  // Get RLC status info and update Bj for all lcids that are active
  for (lcid = DCCH; lcid < MAX_NUM_LCID; lcid++) {
    if (UE_mac_inst[module_idP].logicalChannelConfig[lcid]) {
      // meausre the Bj
      if ((directionP == SF_UL)
          && (UE_mac_inst[module_idP].scheduling_info.Bj[lcid] >= 0)) {
        if (UE_mac_inst[module_idP].
            logicalChannelConfig[lcid]->ul_SpecificParameters) {
          bucketsizeduration =
            UE_mac_inst[module_idP].logicalChannelConfig
            [lcid]->ul_SpecificParameters->prioritisedBitRate *
            TTI;
          bucketsizeduration_max =
            get_ms_bucketsizeduration(UE_mac_inst
                                      [module_idP].logicalChannelConfig
                                      [lcid]->ul_SpecificParameters->bucketSizeDuration);
        } else {
          LOG_E(MAC,
                "[UE %d] lcid %d, NULL ul_SpecificParameters\n",
                module_idP, lcid);
          AssertFatal(1 == 0, "");
        }

        if (UE_mac_inst[module_idP].scheduling_info.Bj[lcid] >
            bucketsizeduration_max) {
          UE_mac_inst[module_idP].scheduling_info.Bj[lcid] =
            bucketsizeduration_max;
        } else {
          UE_mac_inst[module_idP].scheduling_info.Bj[lcid] =
            bucketsizeduration;
        }
      }

      /*
         if (lcid == DCCH) {
         LOG_D(MAC,"[UE %d][SR] Frame %d subframe %d Pending data for SRB1=%d for LCGID %d \n",
         module_idP, txFrameP,txSubframeP,UE_mac_inst[module_idP].scheduling_info.BSR[UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]],
         //         UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]);
         }
       */
    }
  }

  // Call BSR procedure as described in Section 5.4.5 in 36.321

  // First check ReTxBSR Timer because it is always configured
  // Decrement ReTxBSR Timer if it is running and not null
  if ((UE_mac_inst[module_idP].scheduling_info.retxBSR_SF !=
       MAC_UE_BSR_TIMER_NOT_RUNNING)
      && (UE_mac_inst[module_idP].scheduling_info.retxBSR_SF != 0)) {
    UE_mac_inst[module_idP].scheduling_info.retxBSR_SF--;
  }

  // Decrement Periodic Timer if it is running and not null
  if ((UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF !=
       MAC_UE_BSR_TIMER_NOT_RUNNING)
      && (UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF != 0)) {
    UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF--;
  }

  //Check whether Regular BSR is triggered
  if (update_bsr(module_idP, txFrameP, txSubframeP, eNB_indexP) == TRUE) {
    // call SR procedure to generate pending SR and BSR for next PUCCH/PUSCH TxOp.  This should implement the procedures
    // outlined in Sections 5.4.4 an 5.4.5 of 36.321
    UE_mac_inst[module_idP].scheduling_info.SR_pending = 1;
    // Regular BSR trigger
    UE_mac_inst[module_idP].BSR_reporting_active |=
      BSR_TRIGGER_REGULAR;
    LOG_D(MAC,
          "[UE %d][BSR] Regular BSR Triggered Frame %d subframe %d SR for PUSCH is pending\n",
          module_idP, txFrameP, txSubframeP);
  }

  // UE has no valid phy config dedicated ||  no valid/released  SR
  if ((UE_mac_inst[module_idP].physicalConfigDedicated == NULL)) {
    // cancel all pending SRs
    UE_mac_inst[module_idP].scheduling_info.SR_pending = 0;
    UE_mac_inst[module_idP].ul_active = 0;
    LOG_T(MAC, "[UE %d] Release all SRs \n", module_idP);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
    (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
    stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
    return (CONNECTION_OK);
  }

  if ((UE_mac_inst[module_idP].
       physicalConfigDedicated->schedulingRequestConfig == NULL)
      || (UE_mac_inst[module_idP].
          physicalConfigDedicated->schedulingRequestConfig->present ==
          LTE_SchedulingRequestConfig_PR_release)) {
    // initiate RA with CRNTI included in msg3 (no contention) as descibed in 36.321 sec 5.1.5
    // cancel all pending SRs
    UE_mac_inst[module_idP].scheduling_info.SR_pending = 0;
    UE_mac_inst[module_idP].ul_active = 0;
    LOG_T(MAC, "[UE %d] Release all SRs \n", module_idP);
  }

  // Put this in a function
  // Call PHR procedure as described in Section 5.4.6 in 36.321
  if (UE_mac_inst[module_idP].PHR_state == LTE_MAC_MainConfig__phr_Config_PR_setup) { // normal operation
    if (UE_mac_inst[module_idP].PHR_reconfigured == 1) {  // upon (re)configuration of the power headroom reporting functionality by upper layers
      UE_mac_inst[module_idP].PHR_reporting_active = 1;
      UE_mac_inst[module_idP].PHR_reconfigured = 0;
    } else {
      //LOG_D(MAC,"PHR normal operation %d active %d \n", UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF, UE_mac_inst[module_idP].PHR_reporting_active);
      if ((UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF <=
           0)
          &&
          ((get_PL(module_idP, 0, eNB_indexP) <
            UE_mac_inst[module_idP].scheduling_info.
            PathlossChange_db)
           || (UE_mac_inst[module_idP].power_backoff_db[eNB_indexP] >
               UE_mac_inst[module_idP].
               scheduling_info.PathlossChange_db)))
        // trigger PHR and reset the timer later when the PHR report is sent
      {
        UE_mac_inst[module_idP].PHR_reporting_active = 1;
      } else if (UE_mac_inst[module_idP].PHR_reporting_active == 0) {
        UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF--;
      }

      if (UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF <=
          0)
        // trigger PHR and reset the timer later when the PHR report is sent
      {
        UE_mac_inst[module_idP].PHR_reporting_active = 1;
      } else if (UE_mac_inst[module_idP].PHR_reporting_active == 0) {
        UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF--;
      }
    }
  } else {      // release / nothing
    UE_mac_inst[module_idP].PHR_reporting_active = 0; // release PHR
  }

  //If the UE has UL resources allocated for new transmission for this TTI here:
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
#endif
  return (CONNECTION_OK);
}

// to be improved


boolean_t
update_bsr(module_id_t module_idP, frame_t frameP,
           sub_frame_t subframeP, eNB_index_t eNB_index) {
  mac_rlc_status_resp_t rlc_status;
  boolean_t bsr_regular_triggered = FALSE;
  uint8_t lcid;
  uint8_t lcgid;
  uint8_t num_lcid_with_data = 0; // for LCID with data only if LCGID is defined
  uint16_t lcgid_buffer_remain[MAX_NUM_LCGID] = {0,0,0,0};
  int32_t lcid_bytes_in_buffer[MAX_NUM_LCID];
  /* Array for ordering LCID with data per decreasing priority order */
  uint8_t lcid_reordered_array[MAX_NUM_LCID]=
  {MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID,MAX_NUM_LCID};
  uint8_t pos_next = 0;
  uint8_t highest_priority = 16;
  uint8_t array_index = 0;
  // Reset All BSR Infos
  lcid_bytes_in_buffer[0] = 0;

  for (lcid=DCCH; lcid < MAX_NUM_LCID; lcid++) {
    // Reset transmission status
    lcid_bytes_in_buffer[lcid] = 0;
    UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid]=LCID_EMPTY;
  }

  for (lcgid=0; lcgid < MAX_NUM_LCGID; lcgid++) {
    // Reset Buffer Info
    UE_mac_inst[module_idP].scheduling_info.BSR[lcgid]=0;
    UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid]=0;
  }

  //Get Buffer Occupancy and fill lcid_reordered_array
  for (lcid=DCCH; lcid < MAX_NUM_LCID; lcid++) {
    if (UE_mac_inst[module_idP].logicalChannelConfig[lcid]) {
      lcgid = UE_mac_inst[module_idP].scheduling_info.LCGID[lcid];

      // Store already available data to transmit per Group
      if (lcgid < MAX_NUM_LCGID) {
        lcgid_buffer_remain[lcgid] += UE_mac_inst[module_idP].scheduling_info.LCID_buffer_remain[lcid];
      }

      rlc_status = mac_rlc_status_ind(module_idP, UE_mac_inst[module_idP].crnti,eNB_index,frameP,subframeP,ENB_FLAG_NO,MBMS_FLAG_NO,
                                      lcid,
                                      0, 0
                                     );
      lcid_bytes_in_buffer[lcid] = rlc_status.bytes_in_buffer;

      if (rlc_status.bytes_in_buffer > 0) {
        LOG_D(MAC,"[UE %d] PDCCH Tick : LCID%d LCGID%d has data to transmit =%d bytes at frame %d subframe %d\n",
              module_idP, lcid,lcgid,rlc_status.bytes_in_buffer,frameP,subframeP);
        UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid] = LCID_NOT_EMPTY;

        //Update BSR_bytes and position in lcid_reordered_array only if Group is defined
        if (lcgid < MAX_NUM_LCGID) {
          num_lcid_with_data ++;
          // sum lcid buffer which has same lcgid
          UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid] += rlc_status.bytes_in_buffer;
          //Fill in the array
          array_index = 0;

          do {
            if (UE_mac_inst[module_idP].logicalChannelConfig[lcid]->ul_SpecificParameters->priority <= highest_priority) {
              //Insert if priority is higher or equal (lower or equal in value)
              for (pos_next=num_lcid_with_data-1; pos_next > array_index; pos_next--) {
                lcid_reordered_array[pos_next] = lcid_reordered_array[pos_next - 1];
              }

              lcid_reordered_array[array_index] = lcid;
              break;
            }

            array_index ++;
          } while ((array_index < num_lcid_with_data) && (array_index < MAX_NUM_LCID));
        }
      }
    }
  }

  // Check whether a regular BSR can be triggered according to the first cases in 36.321
  if (num_lcid_with_data) {
    LOG_D(MAC,
          "[UE %d] PDCCH Tick at frame %d subframe %d: NumLCID with data=%d Reordered LCID0=%d LCID1=%d LCID2=%d\n",
          module_idP, frameP, subframeP, num_lcid_with_data,
          lcid_reordered_array[0], lcid_reordered_array[1],
          lcid_reordered_array[2]);

    for (array_index = 0; array_index < num_lcid_with_data;
         array_index++) {
      lcid = lcid_reordered_array[array_index];

      /* UL data, for a logical channel which belongs to a LCG, becomes available for transmission in the RLC entity
         either the data belongs to a logical channel with higher priority than the priorities of the logical channels
         which belong to any LCG and for which data is already available for transmission
       */
      if ((UE_mac_inst[module_idP].
           scheduling_info.LCID_buffer_remain[lcid] == 0)
          /* or there is no data available for any of the logical channels which belong to a LCG */
          ||
          (lcgid_buffer_remain
           [UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]] ==
           0)) {
        bsr_regular_triggered = TRUE;
        LOG_D(MAC,
              "[UE %d] PDCCH Tick : MAC BSR Triggered LCID%d LCGID%d data become available at frame %d subframe %d\n",
              module_idP, lcid,
              UE_mac_inst[module_idP].scheduling_info.LCGID[lcid],
              frameP, subframeP);
        break;
      }
    }

    // Trigger Regular BSR if ReTxBSR Timer has expired and UE has data for transmission
    if (UE_mac_inst[module_idP].scheduling_info.retxBSR_SF == 0) {
      bsr_regular_triggered = TRUE;

      if ((UE_mac_inst[module_idP].BSR_reporting_active &
           BSR_TRIGGER_REGULAR) == 0) {
        LOG_I(MAC,
              "[UE %d] PDCCH Tick : MAC BSR Triggered ReTxBSR Timer expiry at frame %d subframe %d\n",
              module_idP, frameP, subframeP);
      }
    }
  }

  //Store Buffer Occupancy in remain buffers for next TTI
  for (lcid = DCCH; lcid < MAX_NUM_LCID; lcid++) {
    UE_mac_inst[module_idP].scheduling_info.LCID_buffer_remain[lcid] =
      lcid_bytes_in_buffer[lcid];
  }

  return bsr_regular_triggered;
}

uint8_t
locate_BsrIndexByBufferSize(const uint32_t *table, int size, int value) {
  uint8_t ju, jm, jl;
  int ascend;
  DevAssert(size > 0);
  DevAssert(size <= 256);

  if (value == 0) {
    return 0;   //elseif (value > 150000) return 63;
  }

  jl = 0;     // lower bound
  ju = size - 1;    // upper bound
  ascend = (table[ju] >= table[jl]) ? 1 : 0;  // determine the order of the the table:  1 if ascending order of table, 0 otherwise

  while (ju - jl > 1) { //If we are not yet done,
    jm = (ju + jl) >> 1;  //compute a midpoint,

    if ((value >= table[jm]) == ascend) {
      jl = jm;    // replace the lower limit
    } else {
      ju = jm;    //replace the upper limit
    }

    LOG_T(MAC,
          "[UE] searching BSR index %d for (BSR TABLE %d < value %d)\n",
          jm, table[jm], value);
  }

  if (value == table[jl]) {
    return jl;
  } else {
    return jl + 1;    //equally  ju
  }
}

int get_sf_periodicBSRTimer(uint8_t sf_offset) {
  switch (sf_offset) {
    case LTE_PeriodicBSR_Timer_r12_sf5:
      return 5;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf10:
      return 10;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf16:
      return 16;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf20:
      return 20;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf32:
      return 32;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf40:
      return 40;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf64:
      return 64;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf80:
      return 80;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf128:
      return 128;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf160:
      return 160;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf320:
      return 320;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf640:
      return 640;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf1280:
      return 1280;
      break;

    case LTE_PeriodicBSR_Timer_r12_sf2560:
      return 2560;
      break;

    case LTE_PeriodicBSR_Timer_r12_infinity:
    default:
      return 0xFFFF;
      break;
  }
}

int get_sf_retxBSRTimer(uint8_t sf_offset) {
  switch (sf_offset) {
    case LTE_RetxBSR_Timer_r12_sf320:
      return 320;
      break;

    case LTE_RetxBSR_Timer_r12_sf640:
      return 640;
      break;

    case LTE_RetxBSR_Timer_r12_sf1280:
      return 1280;
      break;

    case LTE_RetxBSR_Timer_r12_sf2560:
      return 2560;
      break;

    case LTE_RetxBSR_Timer_r12_sf5120:
      return 5120;
      break;

    case LTE_RetxBSR_Timer_r12_sf10240:
      return 10240;
      break;

    default:
      return -1;
      break;
  }
}

int get_ms_bucketsizeduration(uint8_t bucketsizeduration) {
  switch (bucketsizeduration) {
    case LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50:
      return
        50;
      break;

    case LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms100:
      return
        100;
      break;

    case LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms150:
      return
        150;
      break;

    case LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms300:
      return
        300;
      break;

    case LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms500:
      return
        500;
      break;

    case LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms1000:
      return
        1000;
      break;

    default:
      return 0;
      break;
  }
}

void update_phr(module_id_t module_idP, int CC_id) {
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");
  UE_mac_inst[module_idP].PHR_reporting_active = 0;
  UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF =
    get_sf_perioidicPHR_Timer(UE_mac_inst[module_idP].
                              scheduling_info.periodicPHR_Timer);
  UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF =
    get_sf_prohibitPHR_Timer(UE_mac_inst[module_idP].
                             scheduling_info.prohibitPHR_Timer);
  // LOG_D(MAC,"phr %d %d\n ",UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF, UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF);
}

uint8_t
get_phr_mapping(module_id_t module_idP, int CC_id, uint8_t eNB_index) {
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");

  //power headroom reporting range is from -23 ...+40 dB, as described in 36313
  //note: mac_xface->get_Po_NOMINAL_PUSCH(module_idP) is float
  if (get_PHR(module_idP, CC_id, eNB_index) < -23) {
    return 0;
  } else if (get_PHR(module_idP, CC_id, eNB_index) >= 40) {
    return 63;
  } else {      // -23 to 40
    return (uint8_t) get_PHR(module_idP, CC_id,
                             eNB_index) + PHR_MAPPING_OFFSET;
  }
}

int get_sf_perioidicPHR_Timer(uint8_t perioidicPHR_Timer) {
  return (perioidicPHR_Timer + 1) * 10;
}


int get_sf_prohibitPHR_Timer(uint8_t prohibitPHR_Timer) {
  return (prohibitPHR_Timer) * 10;
}

int get_db_dl_PathlossChange(uint8_t dl_PathlossChange) {
  switch (dl_PathlossChange) {
    case LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1:
      return 1;
      break;

    case LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB3:
      return 3;
      break;

    case LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB6:
      return 6;
      break;

    case LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_infinity:
    default:
      return -1;
      break;
  }
}


SLSS_t *ue_get_slss(module_id_t Mod_id,int CC_id,frame_t frame_tx,sub_frame_t subframe_tx) {
  return((SLSS_t *)NULL);
}

SLDCH_t *ue_get_sldch(module_id_t Mod_id,int CC_id,frame_t frame_tx,sub_frame_t subframe_tx) {
  //UE_MAC_INST *ue = &UE_mac_inst[Mod_id];
  SLDCH_t *sldch = &UE_mac_inst[Mod_id].sldch;
  sldch->payload_length = mac_rrc_data_req_ue(Mod_id,
                          CC_id,
                          frame_tx,
                          SL_DISCOVERY,
                          1,
                          (uint8_t *)(sldch->payload), //&UE_mac_inst[Mod_id].SL_Discovery[0].Tx_buffer.Payload[0],
                          0, //eNB_indexP
                          0);

  if (sldch->payload_length >0 ) {
    LOG_I(MAC,"Got %d bytes from RRC for SLDCH @ %p\n",sldch->payload_length,sldch);
    return (sldch);
  } else
    return((SLDCH_t *)NULL);
}


SLSCH_t *ue_get_slsch(module_id_t module_idP,int CC_id,frame_t frameP,sub_frame_t subframeP) {
  mac_rlc_status_resp_t rlc_status = {0,0,0,0,0}; //, rlc_status_data;
  uint32_t absSF = (frameP*10)+subframeP;
  UE_MAC_INST *ue = &UE_mac_inst[module_idP];
  int rvtab[4] = {0,2,3,1};
  int sdu_length;
  //uint8_t sl_lcids[2] = {3, 10}; //list of lcids for SL - hardcoded
  int i = 0;
  // Note: this is hard-coded for now for the default SL configuration (4 SF PSCCH, 36 SF PSSCH)
  SLSCH_t *slsch = &UE_mac_inst[module_idP].slsch;
  LOG_D(MAC,"Checking SLSCH for absSF %d\n",absSF);

  if ((absSF%40) == 0) { // fill PSCCH data later in first subframe of SL period
    ue->sltx_active = 0;

    for (i = 0; i < MAX_NUM_LCID; i++) {
      if (ue->SL_LCID[i] > 0) {
        for (int j = 0; j < ue->numCommFlows; j++) {
          if ((ue->sourceL2Id > 0) && (ue->destinationList[j] >0) ) {
            rlc_status = mac_rlc_status_ind(module_idP, 0x1234,0,frameP,subframeP,ENB_FLAG_NO,MBMS_FLAG_NO,
                                            ue->SL_LCID[i], ue->sourceL2Id, ue->destinationList[j]);

            if (rlc_status.bytes_in_buffer > 2) {
              LOG_I(MAC,"SFN.SF %d.%d: Scheduling for %d bytes in Sidelink buffer\n",frameP,subframeP,rlc_status.bytes_in_buffer);
              // Fill in group id for off-network communications
              ue->sltx_active = 1;
              //store LCID, destinationL2Id
              ue->slsch_lcid =  ue->SL_LCID[i];
              ue->destinationL2Id = ue->destinationList[j];
              break;
            }
          }
        }
      }

      if ( ue->sltx_active == 1) break;
    }
  } // we're not in the SCCH period
  else if (((absSF & 3) == 0 ) &&
           (ue->sltx_active == 1)) { // every 4th subframe, check for new data from RLC
    // 10 PRBs, mcs 19
    int TBS = 4584/8;
    int req = (TBS <= rlc_status.bytes_in_buffer) ? TBS : rlc_status.bytes_in_buffer;

    if (req>0) {
      sdu_length = mac_rlc_data_req(module_idP,
                                    0x1234,
                                    0,
                                    frameP,
                                    ENB_FLAG_NO,
                                    MBMS_FLAG_NO,
                                    ue->slsch_lcid,
                                    req,
                                    (char *)(ue->slsch_pdu.payload + sizeof(SLSCH_SUBHEADER_24_Bit_DST_LONG)),
                                    ue->sourceL2Id,
                                    ue->destinationL2Id
                                   );

      // Notes: 1. hard-coded to 24-bit destination format for now
      if (sdu_length > 0) {
        LOG_I(MAC,"SFN.SF %d.%d : got %d bytes from Sidelink buffer (%d requested)\n",frameP,subframeP,sdu_length,req);
        LOG_I(MAC,"sourceL2Id: 0x%08x \n",ue->sourceL2Id);
        LOG_I(MAC,"groupL2Id/destinationL2Id: 0x%08x \n",ue->destinationL2Id);
        slsch->payload = (unsigned char *)ue->slsch_pdu.payload;

        if (sdu_length < 128) {
          slsch->payload++;
          SLSCH_SUBHEADER_24_Bit_DST_SHORT *shorth= (SLSCH_SUBHEADER_24_Bit_DST_SHORT *)slsch->payload;
          shorth->F = 0;
          shorth->L = sdu_length;
          shorth->E = 0;
          shorth->LCID = ue->slsch_lcid;
          shorth->SRC07 = (ue->sourceL2Id>>16) & 0x000000ff;
          shorth->SRC815 = (ue->sourceL2Id>>8) & 0x000000ff;
          shorth->SRC1623 = ue->sourceL2Id & 0x000000ff;
          shorth->DST07 = (ue->destinationL2Id >>16) & 0x000000ff;
          shorth->DST815 = (ue->destinationL2Id >>8) & 0x000000ff;
          shorth->DST1623 = ue->destinationL2Id & 0x000000ff;
          shorth->V = 0x1;
        } else {
          SLSCH_SUBHEADER_24_Bit_DST_LONG *longh= (SLSCH_SUBHEADER_24_Bit_DST_LONG *)slsch->payload;
          longh->F = 1;
          longh->L_LSB = sdu_length&0xff;
          longh->L_MSB = (sdu_length>>8)&0x7f;
          longh->E = 0;
          longh->LCID = ue->slsch_lcid;
          longh->SRC07 = (ue->sourceL2Id >>16) & 0x000000ff;
          longh->SRC815 = (ue->sourceL2Id>>8) & 0x000000ff;
          longh->SRC1623 = ue->sourceL2Id & 0x000000ff;
          longh->DST07 = (ue->destinationL2Id >>16) & 0x000000ff;
          longh->DST815 = (ue->destinationL2Id>>8) & 0x000000ff;
          longh->DST1623 = ue->destinationL2Id & 0x000000ff;
          longh->V = 0x1;
        }

        slsch->rvidx = 0;
        slsch->payload_length = TBS;
        // fill in SLSCH configuration
        return(&ue->slsch);
      } else ue->sltx_active = 0;
    }
  } else if ((absSF%40)>3 && ue->sltx_active == 1) { // handle retransmission of SDU
    LOG_I(MAC,"SFN.SF %d.%d : retransmission\n",frameP,subframeP);
    slsch->rvidx = rvtab[absSF&3];
    return(&ue->slsch);
  }

  return(NULL);
}
