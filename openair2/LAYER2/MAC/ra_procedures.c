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

/*! \file ra_procedures.c
 * \brief Routines for UE MAC-layer Random-access procedures (36.321) V8.6 2009-03
 * \author R. Knopp and Navid Nikaein
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#include "mac_extern.h"
#include "mac.h"
#include "mac_proto.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY_INTERFACE/phy_interface_extern.h"
#include "SCHED_UE/sched_UE.h"
#include "COMMON/mac_rrc_primitives.h"
#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "SIMULATION/TOOLS/sim.h" // for taus
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"


extern UE_MODE_t get_ue_mode(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

/// This routine implements Section 5.1.2 (UE Random Access Resource Selection) from 36.321
void
get_prach_resources(module_id_t module_idP,
                    int CC_id,
                    uint8_t eNB_index,
                    uint8_t t_id,
                    uint8_t first_Msg3,
                    LTE_RACH_ConfigDedicated_t *rach_ConfigDedicated) {
  uint8_t Msg3_size = UE_mac_inst[module_idP].RA_Msg3_size;
  PRACH_RESOURCES_t *prach_resources =
    &UE_mac_inst[module_idP].RA_prach_resources;
  LTE_RACH_ConfigCommon_t *rach_ConfigCommon = NULL;
  uint8_t noGroupB = 0;
  uint8_t f_id = 0, num_prach = 0;
  int numberOfRA_Preambles;
  int messageSizeGroupA;
  int sizeOfRA_PreamblesGroupA;
  int messagePowerOffsetGroupB;
  int PLThreshold;
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");
  AssertFatal(UE_mac_inst[module_idP].radioResourceConfigCommon != NULL,
              "[UE %d] FATAL  radioResourceConfigCommon is NULL !!!\n",
              module_idP);
  rach_ConfigCommon =
    &UE_mac_inst[module_idP].radioResourceConfigCommon->
    rach_ConfigCommon;
  numberOfRA_Preambles =
    (1 + rach_ConfigCommon->preambleInfo.numberOfRA_Preambles) << 2;

  if (rach_ConfigDedicated) { // This is for network controlled Mobility, later
    if (rach_ConfigDedicated->ra_PRACH_MaskIndex != 0) {
      prach_resources->ra_PreambleIndex =
        rach_ConfigDedicated->ra_PreambleIndex;
      prach_resources->ra_RACH_MaskIndex =
        rach_ConfigDedicated->ra_PRACH_MaskIndex;
      return;
    }
  }

  /* TODO: gcc warns if this variable is not always set, let's put -1 for no more warning */
  messageSizeGroupA = -1;

  if (!rach_ConfigCommon->preambleInfo.preamblesGroupAConfig) {
    noGroupB = 1;
  } else {
    sizeOfRA_PreamblesGroupA =
      (rach_ConfigCommon->preambleInfo.
       preamblesGroupAConfig->sizeOfRA_PreamblesGroupA + 1) << 2;

    switch (rach_ConfigCommon->preambleInfo.
            preamblesGroupAConfig->messageSizeGroupA) {
      case 0:
        messageSizeGroupA = 56;
        break;

      case 1:
        messageSizeGroupA = 144;
        break;

      case 2:
        messageSizeGroupA = 208;
        break;

      case 3:
        messageSizeGroupA = 256;
        break;
    }

    /* TODO: what value to use as default? */
    messagePowerOffsetGroupB = -9999;

    switch (rach_ConfigCommon->preambleInfo.
            preamblesGroupAConfig->messagePowerOffsetGroupB) {
      case 0:
        messagePowerOffsetGroupB = -9999;
        break;

      case 1:
        messagePowerOffsetGroupB = 0;
        break;

      case 2:
        messagePowerOffsetGroupB = 5;
        break;

      case 3:
        messagePowerOffsetGroupB = 8;
        break;

      case 4:
        messagePowerOffsetGroupB = 10;
        break;

      case 5:
        messagePowerOffsetGroupB = 12;
        break;

      case 6:
        messagePowerOffsetGroupB = 15;
        break;

      case 7:
        messagePowerOffsetGroupB = 18;
        break;
    }

    PLThreshold =
      0 - get_DELTA_PREAMBLE(module_idP,
                             CC_id) -
      get_Po_NOMINAL_PUSCH(module_idP,
                           CC_id) - messagePowerOffsetGroupB;
    // Note Pcmax is set to 0 here, we have to fix this

    if (sizeOfRA_PreamblesGroupA == numberOfRA_Preambles) {
      noGroupB = 1;
    }
  }

  if (first_Msg3 == 1) {
    if (noGroupB == 1) {
      // use Group A procedure
      UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex =
        (taus()) % numberOfRA_Preambles;
      UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex =
        0;
      UE_mac_inst[module_idP].RA_usedGroupA = 1;
    } else if ((Msg3_size < messageSizeGroupA) ||
               (get_PL(module_idP, 0, eNB_index) > PLThreshold)) {
      // use Group A procedure
      UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex =
        (taus()) % sizeOfRA_PreamblesGroupA;
      UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex =
        0;
      UE_mac_inst[module_idP].RA_usedGroupA = 1;
    } else {    // use Group B
      UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex =
        sizeOfRA_PreamblesGroupA +
        (taus()) % (numberOfRA_Preambles -
                    sizeOfRA_PreamblesGroupA);
      UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex =
        0;
      UE_mac_inst[module_idP].RA_usedGroupA = 0;
    }

    UE_mac_inst[module_idP].
    RA_prach_resources.ra_PREAMBLE_RECEIVED_TARGET_POWER =
      get_Po_NOMINAL_PUSCH(module_idP, CC_id);
  } else {      // Msg3 is being retransmitted
    if (UE_mac_inst[module_idP].RA_usedGroupA == 1) {
      if (rach_ConfigCommon->preambleInfo.preamblesGroupAConfig) {
        UE_mac_inst[module_idP].RA_prach_resources.
        ra_PreambleIndex =
          (taus()) %
          rach_ConfigCommon->preambleInfo.
          preamblesGroupAConfig->sizeOfRA_PreamblesGroupA;
      } else {
        UE_mac_inst[module_idP].RA_prach_resources.
        ra_PreambleIndex = (taus()) & 0x3f;
      }

      UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex =
        0;
    } else {
      // FIXME rach_ConfigCommon->preambleInfo.preamblesGroupAConfig may be zero
      UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex =
        rach_ConfigCommon->preambleInfo.
        preamblesGroupAConfig->sizeOfRA_PreamblesGroupA +
        (taus()) %
        (rach_ConfigCommon->preambleInfo.numberOfRA_Preambles -
         rach_ConfigCommon->preambleInfo.
         preamblesGroupAConfig->sizeOfRA_PreamblesGroupA);
      UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex =
        0;
    }
  }

  // choose random PRACH resource in TDD
  if (UE_mac_inst[module_idP].tdd_Config) {
    num_prach = get_num_prach_tdd(module_idP);

    if ((num_prach > 0) && (num_prach < 6)) {
      UE_mac_inst[module_idP].RA_prach_resources.ra_TDD_map_index =
        (taus() % num_prach);
    }

    f_id = get_fid_prach_tdd(module_idP,
                             UE_mac_inst
                             [module_idP].RA_prach_resources.
                             ra_TDD_map_index);
  }

  // choose RA-RNTI
  UE_mac_inst[module_idP].RA_prach_resources.ra_RNTI =
    1 + t_id + 10 * f_id;
}

void
Msg1_transmitted(module_id_t module_idP, uint8_t CC_id,
                 frame_t frameP, uint8_t eNB_id) {
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");
  // start contention resolution timer
  UE_mac_inst[module_idP].RA_attempt_number++;

  if (opt_enabled) {
    trace_pdu(DIRECTION_UPLINK, NULL, 0, module_idP, WS_NO_RNTI,
              UE_mac_inst[module_idP].RA_prach_resources.
              ra_PreambleIndex, UE_mac_inst[module_idP].txFrame,
              UE_mac_inst[module_idP].txSubframe, 0,
              UE_mac_inst[module_idP].RA_attempt_number);
    LOG_D(OPT,
          "[UE %d][RAPROC] TX MSG1 Frame %d trace pdu for rnti %x  with size %d\n",
          module_idP, frameP, 1, UE_mac_inst[module_idP].RA_Msg3_size);
  }
}


void
Msg3_transmitted(module_id_t module_idP, uint8_t CC_id,
                 frame_t frameP, uint8_t eNB_id) {
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");
  // start contention resolution timer
  LOG_D(MAC,
        "[UE %d][RAPROC] Frame %d : Msg3_tx: Setting contention resolution timer\n",
        module_idP, frameP);
  UE_mac_inst[module_idP].RA_contention_resolution_cnt = 0;
  UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 1;

  if (opt_enabled) {    // msg3
    trace_pdu(DIRECTION_UPLINK, &UE_mac_inst[module_idP].CCCH_pdu.payload[0],
              UE_mac_inst[module_idP].RA_Msg3_size, module_idP, WS_C_RNTI,
              UE_mac_inst[module_idP].crnti,
              UE_mac_inst[module_idP].txFrame,
              UE_mac_inst[module_idP].txSubframe, 0, 0);
    LOG_D(OPT,
          "[UE %d][RAPROC] MSG3 Frame %d trace pdu Preamble %d   with size %d\n",
          module_idP, frameP, UE_mac_inst[module_idP].crnti
          /*UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex */
          , UE_mac_inst[module_idP].RA_Msg3_size);
  }
}


PRACH_RESOURCES_t *ue_get_rach(module_id_t module_idP, int CC_id,
                               frame_t frameP, uint8_t eNB_indexP,
                               sub_frame_t subframeP) {
  uint8_t Size = 0;
  UE_MODE_t UE_mode;

  // Modification for phy_stub_ue operation
  if(NFAPI_MODE == NFAPI_UE_STUB_PNF) { // phy_stub_ue mode
    UE_mode = UE_mac_inst[module_idP].UE_mode[0];
    LOG_D(MAC, "ue_get_rach , UE_mode: %d", UE_mode);
  } else { // Full stack mode
    UE_mode = get_ue_mode(module_idP,0,eNB_indexP);
  }

  uint8_t lcid = CCCH;
  uint16_t Size16;
  struct LTE_RACH_ConfigCommon *rach_ConfigCommon =
    (struct LTE_RACH_ConfigCommon *) NULL;
  int32_t frame_diff = 0;
  uint8_t dcch_header_len = 0;
  uint16_t sdu_lengths;
  uint8_t ulsch_buff[MAX_ULSCH_PAYLOAD_BYTES];
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");

  if (UE_mode == PRACH) {
    LOG_D(MAC, "ue_get_rach 3, RA_active value: %d", UE_mac_inst[module_idP].RA_active);

    if (UE_mac_inst[module_idP].radioResourceConfigCommon) {
      rach_ConfigCommon =
        &UE_mac_inst[module_idP].
        radioResourceConfigCommon->rach_ConfigCommon;
    } else {
      return (NULL);
    }

    if (UE_mac_inst[module_idP].RA_active == 0) {
      LOG_I(MAC, "RA not active\n");
      // check if RRC is ready to initiate the RA procedure
      Size = mac_rrc_data_req_ue(module_idP,
                                 CC_id,
                                 frameP,
                                 CCCH, 1,
                                 &UE_mac_inst[module_idP].
                                 CCCH_pdu.payload[sizeof
                                     (SCH_SUBHEADER_SHORT)
                                     + 1], eNB_indexP,
                                 0);
      Size16 = (uint16_t) Size;
      //  LOG_D(MAC,"[UE %d] Frame %d: Requested RRCConnectionRequest, got %d bytes\n",module_idP,frameP,Size);
      LOG_I(RRC,
            "[MSC_MSG][FRAME %05d][RRC_UE][MOD %02d][][--- MAC_DATA_REQ (RRCConnectionRequest eNB %d) --->][MAC_UE][MOD %02d][]\n",
            frameP, module_idP, eNB_indexP, module_idP);
      LOG_I(MAC,
            "[UE %d] Frame %d: Requested RRCConnectionRequest, got %d bytes\n",
            module_idP, frameP, Size);

      if (Size > 0) {
        UE_mac_inst[module_idP].RA_active = 1;
        UE_mac_inst[module_idP].RA_PREAMBLE_TRANSMISSION_COUNTER =
          1;
        UE_mac_inst[module_idP].RA_Msg3_size =
          Size + sizeof(SCH_SUBHEADER_SHORT) +
          sizeof(SCH_SUBHEADER_SHORT);
        UE_mac_inst[module_idP].RA_prachMaskIndex = 0;
        UE_mac_inst[module_idP].RA_prach_resources.Msg3 =
          UE_mac_inst[module_idP].CCCH_pdu.payload;
        UE_mac_inst[module_idP].RA_backoff_cnt = 0; // add the backoff condition here if we have it from a previous RA reponse which failed (i.e. backoff indicator)
        AssertFatal(rach_ConfigCommon != NULL,
                    "[UE %d] FATAL Frame %d: rach_ConfigCommon is NULL !!!\n",
                    module_idP, frameP);
        UE_mac_inst[module_idP].RA_window_cnt =
          2 +
          rach_ConfigCommon->ra_SupervisionInfo.
          ra_ResponseWindowSize;

        if (UE_mac_inst[module_idP].RA_window_cnt == 9) {
          UE_mac_inst[module_idP].RA_window_cnt = 10; // Note: 9 subframe window doesn't exist, after 8 is 10!
        }

        UE_mac_inst[module_idP].RA_tx_frame = frameP;
        UE_mac_inst[module_idP].RA_tx_subframe = subframeP;
        UE_mac_inst[module_idP].RA_backoff_frame = frameP;
        UE_mac_inst[module_idP].RA_backoff_subframe = subframeP;
        // Fill in preamble and PRACH resource
        get_prach_resources(module_idP, CC_id, eNB_indexP,
                            subframeP, 1, NULL);
        generate_ulsch_header((uint8_t *) & UE_mac_inst[module_idP].CCCH_pdu.payload[0],  // mac header
                              1,  // num sdus
                              0,  // short pading
                              &Size16,  // sdu length
                              &lcid,  // sdu lcid
                              NULL, // power headroom
                              NULL, // crnti
                              NULL, // truncated bsr
                              NULL, // short bsr
                              NULL, // long_bsr
                              1); //post_padding
        return (&UE_mac_inst[module_idP].RA_prach_resources);
      } else if (UE_mac_inst[module_idP].
                 scheduling_info.BSR_bytes[UE_mac_inst[module_idP].
                                           scheduling_info.LCGID
                                           [DCCH]] > 0) {
        // This is for triggering a transmission on DCCH using PRACH (during handover, or sending SR for example)
        dcch_header_len = 2 + 2;  /// SHORT Subheader + C-RNTI control element
        LOG_USEDINLOG_VAR(mac_rlc_status_resp_t,rlc_status)=mac_rlc_status_ind(module_idP,
            UE_mac_inst[module_idP].crnti,
            eNB_indexP, frameP, subframeP,
            ENB_FLAG_NO, MBMS_FLAG_NO, DCCH, 6
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
            ,0, 0
#endif
                                                                              );

        if (UE_mac_inst[module_idP].crnti_before_ho)
          LOG_D(MAC,
                "[UE %d] Frame %d : UL-DCCH -> ULSCH, HO RRCConnectionReconfigurationComplete (%x, %x), RRC message has %d bytes to send throug PRACH (mac header len %d)\n",
                module_idP, frameP,
                UE_mac_inst[module_idP].crnti,
                UE_mac_inst[module_idP].crnti_before_ho,
                rlc_status.bytes_in_buffer, dcch_header_len);
        else
          LOG_D(MAC,
                "[UE %d] Frame %d : UL-DCCH -> ULSCH, RRC message has %d bytes to send through PRACH(mac header len %d)\n",
                module_idP, frameP, rlc_status.bytes_in_buffer,
                dcch_header_len);

        sdu_lengths = mac_rlc_data_req(module_idP, UE_mac_inst[module_idP].crnti, eNB_indexP, frameP, ENB_FLAG_NO, MBMS_FLAG_NO, DCCH, 6, //not used
                                       (char *) &ulsch_buff[0]
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                       ,0,
                                       0
#endif
                                      );

        if(sdu_lengths > 0)
          LOG_D(MAC, "[UE %d] TX Got %d bytes for DCCH\n",
                module_idP, sdu_lengths);
        else
          LOG_E(MAC, "[UE %d] TX DCCH error\n",
                module_idP );

        update_bsr(module_idP, frameP, subframeP, eNB_indexP);
        UE_mac_inst[module_idP].
        scheduling_info.BSR[UE_mac_inst[module_idP].
                            scheduling_info.LCGID[DCCH]] =
                              locate_BsrIndexByBufferSize(BSR_TABLE, BSR_TABLE_SIZE,
                                  UE_mac_inst
                                  [module_idP].scheduling_info.BSR_bytes
                                  [UE_mac_inst
                                   [module_idP].scheduling_info.LCGID
                                   [DCCH]]);
        //TO DO: fill BSR infos in UL TBS
        //header_len +=2;
        UE_mac_inst[module_idP].RA_active = 1;
        UE_mac_inst[module_idP].RA_PREAMBLE_TRANSMISSION_COUNTER =
          1;
        UE_mac_inst[module_idP].RA_Msg3_size =
          Size + dcch_header_len;
        UE_mac_inst[module_idP].RA_prachMaskIndex = 0;
        UE_mac_inst[module_idP].RA_prach_resources.Msg3 =
          ulsch_buff;
        UE_mac_inst[module_idP].RA_backoff_cnt = 0; // add the backoff condition here if we have it from a previous RA reponse which failed (i.e. backoff indicator)
        AssertFatal(rach_ConfigCommon != NULL,
                    "[UE %d] FATAL Frame %d: rach_ConfigCommon is NULL !!!\n",
                    module_idP, frameP);
        UE_mac_inst[module_idP].RA_window_cnt =
          2 +
          rach_ConfigCommon->ra_SupervisionInfo.
          ra_ResponseWindowSize;

        if (UE_mac_inst[module_idP].RA_window_cnt == 9) {
          UE_mac_inst[module_idP].RA_window_cnt = 10; // Note: 9 subframe window doesn't exist, after 8 is 10!
        }

        UE_mac_inst[module_idP].RA_tx_frame = frameP;
        UE_mac_inst[module_idP].RA_tx_subframe = subframeP;
        UE_mac_inst[module_idP].RA_backoff_frame = frameP;
        UE_mac_inst[module_idP].RA_backoff_subframe = subframeP;
        // Fill in preamble and PRACH resource
        get_prach_resources(module_idP, CC_id, eNB_indexP,
                            subframeP, 1, NULL);
        generate_ulsch_header((uint8_t *) ulsch_buff, // mac header
                              1,  // num sdus
                              0,  // short pading
                              &Size16,  // sdu length
                              &lcid,  // sdu lcid
                              NULL, // power headroom
                              &UE_mac_inst[module_idP].crnti, // crnti
                              NULL, // truncated bsr
                              NULL, // short bsr
                              NULL, // long_bsr
                              0); //post_padding
        return (&UE_mac_inst[module_idP].RA_prach_resources);
      }
    } else {    // RACH is active
      LOG_D(MAC,
            "[MAC][UE %d][RAPROC] frameP %d, subframe %d: RA Active, window cnt %d (RA_tx_frame %d, RA_tx_subframe %d)\n",
            module_idP, frameP, subframeP,
            UE_mac_inst[module_idP].RA_window_cnt,
            UE_mac_inst[module_idP].RA_tx_frame,
            UE_mac_inst[module_idP].RA_tx_subframe);

      // compute backoff parameters
      if (UE_mac_inst[module_idP].RA_backoff_cnt > 0) {
        frame_diff =
          (sframe_t) frameP -
          UE_mac_inst[module_idP].RA_backoff_frame;

        if (frame_diff < 0) {
          frame_diff = -frame_diff;
        }

        UE_mac_inst[module_idP].RA_backoff_cnt -=
          ((10 * frame_diff) +
           (subframeP -
            UE_mac_inst[module_idP].RA_backoff_subframe));
        UE_mac_inst[module_idP].RA_backoff_frame = frameP;
        UE_mac_inst[module_idP].RA_backoff_subframe = subframeP;
      }

      // compute RA window parameters
      if (UE_mac_inst[module_idP].RA_window_cnt > 0) {
        frame_diff =
          (frame_t) frameP - UE_mac_inst[module_idP].RA_tx_frame;

        if (frame_diff < 0) {
          frame_diff = -frame_diff;
        }

        UE_mac_inst[module_idP].RA_window_cnt -=
          ((10 * frame_diff) +
           (subframeP - UE_mac_inst[module_idP].RA_tx_subframe));
        LOG_D(MAC,
              "[MAC][UE %d][RAPROC] frameP %d, subframe %d: RA Active, adjusted window cnt %d\n",
              module_idP, frameP, subframeP,
              UE_mac_inst[module_idP].RA_window_cnt);
      }

      if ((UE_mac_inst[module_idP].RA_window_cnt <= 0) &&
          (UE_mac_inst[module_idP].RA_backoff_cnt <= 0)) {
        UE_mac_inst[module_idP].RA_tx_frame = frameP;
        UE_mac_inst[module_idP].RA_tx_subframe = subframeP;
        UE_mac_inst[module_idP].RA_PREAMBLE_TRANSMISSION_COUNTER++;
        UE_mac_inst[module_idP].RA_prach_resources.ra_PREAMBLE_RECEIVED_TARGET_POWER += (rach_ConfigCommon->powerRampingParameters.powerRampingStep << 1);  // 2dB increments in ASN.1 definition
        int preambleTransMax = -1;

        switch (rach_ConfigCommon->ra_SupervisionInfo.
                preambleTransMax) {
          case LTE_PreambleTransMax_n3:
            preambleTransMax = 3;
            break;

          case LTE_PreambleTransMax_n4:
            preambleTransMax = 4;
            break;

          case LTE_PreambleTransMax_n5:
            preambleTransMax = 5;
            break;

          case LTE_PreambleTransMax_n6:
            preambleTransMax = 6;
            break;

          case LTE_PreambleTransMax_n7:
            preambleTransMax = 7;
            break;

          case LTE_PreambleTransMax_n8:
            preambleTransMax = 8;
            break;

          case LTE_PreambleTransMax_n10:
            preambleTransMax = 10;
            break;

          case LTE_PreambleTransMax_n20:
            preambleTransMax = 20;
            break;

          case LTE_PreambleTransMax_n50:
            preambleTransMax = 50;
            break;

          case LTE_PreambleTransMax_n100:
            preambleTransMax = 100;
            break;

          case LTE_PreambleTransMax_n200:
            preambleTransMax = 200;
            break;
        }

        if (UE_mac_inst[module_idP].
            RA_PREAMBLE_TRANSMISSION_COUNTER == preambleTransMax) {
          LOG_D(MAC,
                "[UE %d] Frame %d: Maximum number of RACH attempts (%d)\n",
                module_idP, frameP, preambleTransMax);
          // send message to RRC
          UE_mac_inst[module_idP].
          RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
          UE_mac_inst[module_idP].
          RA_prach_resources.ra_PREAMBLE_RECEIVED_TARGET_POWER
            = get_Po_NOMINAL_PUSCH(module_idP, CC_id);
        }

        UE_mac_inst[module_idP].RA_window_cnt =
          2 +
          rach_ConfigCommon->ra_SupervisionInfo.
          ra_ResponseWindowSize;
        UE_mac_inst[module_idP].RA_backoff_cnt = 0;
        // Fill in preamble and PRACH resource
        get_prach_resources(module_idP, CC_id, eNB_indexP,
                            subframeP, 0, NULL);
        return (&UE_mac_inst[module_idP].RA_prach_resources);
      }
    }
  } else if (UE_mode == PUSCH) {
    LOG_D(MAC,
          "[UE %d] FATAL: Should not have checked for RACH in PUSCH yet ...",
          module_idP);
    AssertFatal(1 == 0, "");
  }

  return (NULL);
}
