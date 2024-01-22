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

#include "openair2/RRC/NR_UE/rrc_proto.h"

void nr_rrc_SI_timers(NR_UE_RRC_SI_INFO *SInfo)
{
  // delete any stored version of a SIB after 3 hours
  // from the moment it was successfully confirmed as valid
  if (SInfo->sib1 && SInfo->sib1_timer >= 0) {
    SInfo->sib1_timer += 10;
    if (SInfo->sib1_timer > 10800000)
      SInfo->sib1_timer = -1;
  }
  if (SInfo->sib2 && SInfo->sib2_timer >= 0) {
    SInfo->sib2_timer += 10;
    if (SInfo->sib2_timer > 10800000)
      SInfo->sib2_timer = -1;
  }
  if (SInfo->sib3 && SInfo->sib3_timer >= 0) {
    SInfo->sib3_timer += 10;
    if (SInfo->sib3_timer > 10800000)
      SInfo->sib3_timer = -1;
  }
  if (SInfo->sib4 && SInfo->sib4_timer >= 0) {
    SInfo->sib4_timer += 10;
    if (SInfo->sib4_timer > 10800000)
      SInfo->sib4_timer = -1;
  }
  if (SInfo->sib5 && SInfo->sib5_timer >= 0) {
    SInfo->sib5_timer += 10;
    if (SInfo->sib5_timer > 10800000)
      SInfo->sib5_timer = -1;
  }
  if (SInfo->sib6 && SInfo->sib6_timer >= 0) {
    SInfo->sib6_timer += 10;
    if (SInfo->sib6_timer > 10800000)
      SInfo->sib6_timer = -1;
  }
  if (SInfo->sib7 && SInfo->sib7_timer >= 0) {
    SInfo->sib7_timer += 10;
    if (SInfo->sib7_timer > 10800000)
      SInfo->sib7_timer = -1;
  }
  if (SInfo->sib8 && SInfo->sib8_timer >= 0) {
    SInfo->sib8_timer += 10;
    if (SInfo->sib8_timer > 10800000)
      SInfo->sib8_timer = -1;
  }
  if (SInfo->sib9 && SInfo->sib9_timer >= 0) {
    SInfo->sib9_timer += 10;
    if (SInfo->sib9_timer > 10800000)
      SInfo->sib9_timer = -1;

  }
  if (SInfo->sib10 && SInfo->sib10_timer >= 0) {
    SInfo->sib10_timer += 10;
    if (SInfo->sib10_timer > 10800000)
      SInfo->sib10_timer = -1;
  }
  if (SInfo->sib11 && SInfo->sib11_timer >= 0) {
    SInfo->sib11_timer += 10;
    if (SInfo->sib11_timer > 10800000)
      SInfo->sib11_timer = -1;
  }
  if (SInfo->sib12 && SInfo->sib12_timer >= 0) {
    SInfo->sib12_timer += 10;
    if (SInfo->sib12_timer > 10800000)
      SInfo->sib12_timer = -1;
  }
  if (SInfo->sib13 && SInfo->sib13_timer >= 0) {
    SInfo->sib13_timer += 10;
    if (SInfo->sib13_timer > 10800000)
      SInfo->sib13_timer = -1;
  }
  if (SInfo->sib14 && SInfo->sib14_timer >= 0) {
    SInfo->sib14_timer += 10;
    if (SInfo->sib14_timer > 10800000)
      SInfo->sib14_timer = -1;
  }
}

void nr_rrc_handle_timers(NR_UE_RRC_INST_t *rrc)
{
  NR_UE_Timers_Constants_t *timers = &rrc->timers_and_constants;

  if (timers->T300_active == true) {
    timers->T300_cnt += 10;
    if(timers->T300_cnt >= timers->T300_k) {
      timers->T300_active = false;
      timers->T300_cnt = 0;
      handle_t300_expiry(rrc);
    }
  }
  if (timers->T304_active == true) {
    timers->T304_cnt += 10;
    if(timers->T304_cnt >= timers->T304_k) {
      // TODO
      // For T304 of MCG, in case of the handover from NR or intra-NR
      // handover, initiate the RRC re-establishment procedure;
      // In case of handover to NR, perform the actions defined in the
      // specifications applicable for the source RAT.
    }
  }
  if (timers->T310_active == true) {
    timers->T310_cnt += 10;
    if(timers->T310_cnt >= timers->T310_k) {
      // TODO
      // handle detection of radio link failure
      // as described in 5.3.10.3 of 38.331
      AssertFatal(false, "Radio link failure! Not handled yet!\n");
    }
  }
  if (timers->T311_active == true) {
    timers->T311_cnt += 10;
    if(timers->T311_cnt >= timers->T311_k) {
      // Upon T311 expiry, the UE shall perform the actions upon going to RRC_IDLE
      // with release cause 'RRC connection failure'
      nr_rrc_going_to_IDLE(rrc, RRC_CONNECTION_FAILURE, NULL);
    }
  }
}

void nr_rrc_set_T304(NR_UE_Timers_Constants_t *tac, NR_ReconfigurationWithSync_t *reconfigurationWithSync)
{
  if(reconfigurationWithSync) {
    switch (reconfigurationWithSync->t304) {
      case NR_ReconfigurationWithSync__t304_ms50 :
        tac->T304_k = 50;
        break;
      case NR_ReconfigurationWithSync__t304_ms100 :
        tac->T304_k = 100;
        break;
      case NR_ReconfigurationWithSync__t304_ms150 :
        tac->T304_k = 150;
        break;
      case NR_ReconfigurationWithSync__t304_ms200 :
        tac->T304_k = 200;
        break;
      case NR_ReconfigurationWithSync__t304_ms500 :
        tac->T304_k = 500;
        break;
      case NR_ReconfigurationWithSync__t304_ms1000 :
        tac->T304_k = 1000;
        break;
      case NR_ReconfigurationWithSync__t304_ms2000 :
        tac->T304_k = 2000;
        break;
      case NR_ReconfigurationWithSync__t304_ms10000 :
        tac->T304_k = 10000;
        break;
      default :
        AssertFatal(false, "Invalid T304 %ld\n", reconfigurationWithSync->t304);
    }
  }
}

void set_rlf_sib1_timers_and_constants(NR_UE_Timers_Constants_t *tac, NR_SIB1_t *sib1)
{
  if(sib1 && sib1->ue_TimersAndConstants) {
    switch (sib1->ue_TimersAndConstants->t301) {
      case NR_UE_TimersAndConstants__t301_ms100 :
        tac->T301_k = 100;
        break;
      case NR_UE_TimersAndConstants__t301_ms200 :
        tac->T301_k = 200;
        break;
      case NR_UE_TimersAndConstants__t301_ms300 :
        tac->T301_k = 300;
        break;
      case NR_UE_TimersAndConstants__t301_ms400 :
        tac->T301_k = 400;
        break;
      case NR_UE_TimersAndConstants__t301_ms600 :
        tac->T301_k = 600;
        break;
      case NR_UE_TimersAndConstants__t301_ms1000 :
        tac->T301_k = 1000;
        break;
      case NR_UE_TimersAndConstants__t301_ms1500 :
        tac->T301_k = 1500;
        break;
      case NR_UE_TimersAndConstants__t301_ms2000 :
        tac->T301_k = 2000;
        break;
      default :
        AssertFatal(false, "Invalid T301 %ld\n", sib1->ue_TimersAndConstants->t301);
    }
    switch (sib1->ue_TimersAndConstants->t310) {
      case NR_UE_TimersAndConstants__t310_ms0 :
        tac->T310_k = 0;
        break;
      case NR_UE_TimersAndConstants__t310_ms50 :
        tac->T310_k = 50;
        break;
      case NR_UE_TimersAndConstants__t310_ms100 :
        tac->T310_k = 100;
        break;
      case NR_UE_TimersAndConstants__t310_ms200 :
        tac->T310_k = 200;
        break;
      case NR_UE_TimersAndConstants__t310_ms500 :
        tac->T310_k = 500;
        break;
      case NR_UE_TimersAndConstants__t310_ms1000 :
        tac->T310_k = 1000;
        break;
      case NR_UE_TimersAndConstants__t310_ms2000 :
        tac->T310_k = 2000;
        break;
      default :
        AssertFatal(false, "Invalid T310 %ld\n", sib1->ue_TimersAndConstants->t310);
    }
    switch (sib1->ue_TimersAndConstants->t311) {
      case NR_UE_TimersAndConstants__t311_ms1000 :
        tac->T311_k = 1000;
        break;
      case NR_UE_TimersAndConstants__t311_ms3000 :
        tac->T311_k = 3000;
        break;
      case NR_UE_TimersAndConstants__t311_ms5000 :
        tac->T311_k = 5000;
        break;
      case NR_UE_TimersAndConstants__t311_ms10000 :
        tac->T311_k = 10000;
        break;
      case NR_UE_TimersAndConstants__t311_ms15000 :
        tac->T311_k = 15000;
        break;
      case NR_UE_TimersAndConstants__t311_ms20000 :
        tac->T311_k = 20000;
        break;
      case NR_UE_TimersAndConstants__t311_ms30000 :
        tac->T311_k = 30000;
        break;
      default :
        AssertFatal(false, "Invalid T311 %ld\n", sib1->ue_TimersAndConstants->t311);
    }
    switch (sib1->ue_TimersAndConstants->n310) {
      case NR_UE_TimersAndConstants__n310_n1 :
        tac->N310_k = 1;
        break;
      case NR_UE_TimersAndConstants__n310_n2 :
        tac->N310_k = 2;
        break;
      case NR_UE_TimersAndConstants__n310_n3 :
        tac->N310_k = 3;
        break;
      case NR_UE_TimersAndConstants__n310_n4 :
        tac->N310_k = 4;
        break;
      case NR_UE_TimersAndConstants__n310_n6 :
        tac->N310_k = 6;
        break;
      case NR_UE_TimersAndConstants__n310_n8 :
        tac->N310_k = 8;
        break;
      case NR_UE_TimersAndConstants__n310_n10 :
        tac->N310_k = 10;
        break;
      case NR_UE_TimersAndConstants__n310_n20 :
        tac->N310_k = 20;
        break;
      default :
        AssertFatal(false, "Invalid N310 %ld\n", sib1->ue_TimersAndConstants->n310);
    }
    switch (sib1->ue_TimersAndConstants->n311) {
      case NR_UE_TimersAndConstants__n311_n1 :
        tac->N311_k = 1;
        break;
      case NR_UE_TimersAndConstants__n311_n2 :
        tac->N311_k = 2;
        break;
      case NR_UE_TimersAndConstants__n311_n3 :
        tac->N311_k = 3;
        break;
      case NR_UE_TimersAndConstants__n311_n4 :
        tac->N311_k = 4;
        break;
      case NR_UE_TimersAndConstants__n311_n5 :
        tac->N311_k = 5;
        break;
      case NR_UE_TimersAndConstants__n311_n6 :
        tac->N311_k = 6;
        break;
      case NR_UE_TimersAndConstants__n311_n8 :
        tac->N311_k = 8;
        break;
      case NR_UE_TimersAndConstants__n311_n10 :
        tac->N311_k = 10;
        break;
      default :
        AssertFatal(false, "Invalid N311 %ld\n", sib1->ue_TimersAndConstants->n311);
    }
  }
  else
    LOG_E(NR_RRC,"SIB1 should not be NULL and neither UE_Timers_Constants\n");
}

void nr_rrc_set_sib1_timers_and_constants(NR_UE_Timers_Constants_t *tac, NR_SIB1_t *sib1)
{
  set_rlf_sib1_timers_and_constants(tac, sib1);
  if(sib1 && sib1->ue_TimersAndConstants) {
    switch (sib1->ue_TimersAndConstants->t300) {
      case NR_UE_TimersAndConstants__t300_ms100 :
        tac->T300_k = 100;
        break;
      case NR_UE_TimersAndConstants__t300_ms200 :
        tac->T300_k = 200;
        break;
      case NR_UE_TimersAndConstants__t300_ms300 :
        tac->T300_k = 300;
        break;
      case NR_UE_TimersAndConstants__t300_ms400 :
        tac->T300_k = 400;
        break;
      case NR_UE_TimersAndConstants__t300_ms600 :
        tac->T300_k = 600;
        break;
      case NR_UE_TimersAndConstants__t300_ms1000 :
        tac->T300_k = 1000;
        break;
      case NR_UE_TimersAndConstants__t300_ms1500 :
        tac->T300_k = 1500;
        break;
      case NR_UE_TimersAndConstants__t300_ms2000 :
        tac->T300_k = 2000;
        break;
      default :
        AssertFatal(false, "Invalid T300 %ld\n", sib1->ue_TimersAndConstants->t300);
    }
    switch (sib1->ue_TimersAndConstants->t319) {
      case NR_UE_TimersAndConstants__t319_ms100 :
        tac->T319_k = 100;
        break;
      case NR_UE_TimersAndConstants__t319_ms200 :
        tac->T319_k = 200;
        break;
      case NR_UE_TimersAndConstants__t319_ms300 :
        tac->T319_k = 300;
        break;
      case NR_UE_TimersAndConstants__t319_ms400 :
        tac->T319_k = 400;
        break;
      case NR_UE_TimersAndConstants__t319_ms600 :
        tac->T319_k = 600;
        break;
      case NR_UE_TimersAndConstants__t319_ms1000 :
        tac->T319_k = 1000;
        break;
      case NR_UE_TimersAndConstants__t319_ms1500 :
        tac->T319_k = 1500;
        break;
      case NR_UE_TimersAndConstants__t319_ms2000 :
        tac->T319_k = 2000;
        break;
      default :
        AssertFatal(false, "Invalid T319 %ld\n", sib1->ue_TimersAndConstants->t319);
    }
  }
  else
    LOG_E(NR_RRC,"SIB1 should not be NULL and neither UE_Timers_Constants\n");
}

void nr_rrc_handle_SetupRelease_RLF_TimersAndConstants(NR_UE_RRC_INST_t *rrc,
                                                       struct NR_SetupRelease_RLF_TimersAndConstants *rlf_TimersAndConstants)
{
  if(rlf_TimersAndConstants == NULL)
    return;

  NR_UE_Timers_Constants_t *tac = &rrc->timers_and_constants;
  NR_RLF_TimersAndConstants_t *rlf_tac = NULL;
  switch(rlf_TimersAndConstants->present){
    case NR_SetupRelease_RLF_TimersAndConstants_PR_release :
      // use values for timers T301, T310, T311 and constants N310, N311, as included in ue-TimersAndConstants received in SIB1
      set_rlf_sib1_timers_and_constants(tac, rrc->perNB[0].SInfo.sib1);
      break;
    case NR_SetupRelease_RLF_TimersAndConstants_PR_setup :
      rlf_tac = rlf_TimersAndConstants->choice.setup;
      if (rlf_tac == NULL)
        return;
      // (re-)configure the value of timers and constants in accordance with received rlf-TimersAndConstants
      switch (rlf_tac->t310) {
        case NR_RLF_TimersAndConstants__t310_ms0 :
          tac->T310_k = 0;
          break;
        case NR_RLF_TimersAndConstants__t310_ms50 :
          tac->T310_k = 50;
          break;
        case NR_RLF_TimersAndConstants__t310_ms100 :
          tac->T310_k = 100;
          break;
        case NR_RLF_TimersAndConstants__t310_ms200 :
          tac->T310_k = 200;
          break;
        case NR_RLF_TimersAndConstants__t310_ms500 :
          tac->T310_k = 500;
          break;
        case NR_RLF_TimersAndConstants__t310_ms1000 :
          tac->T310_k = 1000;
          break;
        case NR_RLF_TimersAndConstants__t310_ms2000 :
          tac->T310_k = 2000;
          break;
        case NR_RLF_TimersAndConstants__t310_ms4000 :
          tac->T310_k = 4000;
          break;
        case NR_RLF_TimersAndConstants__t310_ms6000 :
          tac->T310_k = 6000;
          break;
        default :
          AssertFatal(false, "Invalid T310 %ld\n", rlf_tac->t310);
      }
      switch (rlf_tac->n310) {
        case NR_RLF_TimersAndConstants__n310_n1 :
          tac->N310_k = 1;
          break;
        case NR_RLF_TimersAndConstants__n310_n2 :
          tac->N310_k = 2;
          break;
        case NR_RLF_TimersAndConstants__n310_n3 :
          tac->N310_k = 3;
          break;
        case NR_RLF_TimersAndConstants__n310_n4 :
          tac->N310_k = 4;
          break;
        case NR_RLF_TimersAndConstants__n310_n6 :
          tac->N310_k = 6;
          break;
        case NR_RLF_TimersAndConstants__n310_n8 :
          tac->N310_k = 8;
          break;
        case NR_RLF_TimersAndConstants__n310_n10 :
          tac->N310_k = 10;
          break;
        case NR_RLF_TimersAndConstants__n310_n20 :
          tac->N310_k = 20;
          break;
        default :
          AssertFatal(false, "Invalid N310 %ld\n", rlf_tac->n310);
      }
      switch (rlf_tac->n311) {
        case NR_RLF_TimersAndConstants__n311_n1 :
          tac->N311_k = 1;
          break;
        case NR_RLF_TimersAndConstants__n311_n2 :
          tac->N311_k = 2;
          break;
        case NR_RLF_TimersAndConstants__n311_n3 :
          tac->N311_k = 3;
          break;
        case NR_RLF_TimersAndConstants__n311_n4 :
          tac->N311_k = 4;
          break;
        case NR_RLF_TimersAndConstants__n311_n5 :
          tac->N311_k = 5;
          break;
        case NR_RLF_TimersAndConstants__n311_n6 :
          tac->N311_k = 6;
          break;
        case NR_RLF_TimersAndConstants__n311_n8 :
          tac->N311_k = 8;
          break;
        case NR_RLF_TimersAndConstants__n311_n10 :
          tac->N311_k = 10;
          break;
        default :
          AssertFatal(false, "Invalid N311 %ld\n", rlf_tac->n311);
      }
      if (rlf_tac->ext1) {
        switch (rlf_tac->ext1->t311) {
          case NR_RLF_TimersAndConstants__ext1__t311_ms1000 :
            tac->T311_k = 1000;
            break;
          case NR_RLF_TimersAndConstants__ext1__t311_ms3000 :
            tac->T311_k = 3000;
            break;
          case NR_RLF_TimersAndConstants__ext1__t311_ms5000 :
            tac->T311_k = 5000;
            break;
          case NR_RLF_TimersAndConstants__ext1__t311_ms10000 :
            tac->T311_k = 10000;
            break;
          case NR_RLF_TimersAndConstants__ext1__t311_ms15000 :
            tac->T311_k = 15000;
            break;
          case NR_RLF_TimersAndConstants__ext1__t311_ms20000 :
            tac->T311_k = 20000;
            break;
          case NR_RLF_TimersAndConstants__ext1__t311_ms30000 :
            tac->T311_k = 30000;
            break;
          default :
            AssertFatal(false, "Invalid T311 %ld\n", rlf_tac->ext1->t311);
        }
      }
      reset_rlf_timers_and_constants(tac);
      break;
    default :
      AssertFatal(false, "Invalid rlf_TimersAndConstants\n");
  }
}

void handle_rlf_sync(NR_UE_Timers_Constants_t *tac,
                     nr_sync_msg_t sync_msg)
{
  if (sync_msg == IN_SYNC) {
    tac->N310_cnt = 0;
    if (tac->T310_active) {
      tac->N311_cnt++;
      // Upon receiving N311 consecutive "in-sync" indications
      if (tac->N311_cnt >= tac->N311_k) {
        // stop timer T310
        tac->T310_active = false;
        tac->T310_cnt = 0;
        tac->N311_cnt = 0;
      }
    }
  }
  else {
    // OUT_OF_SYNC
    tac->N311_cnt = 0;
    if(tac->T300_active ||
       tac->T301_active ||
       tac->T304_active ||
       tac->T310_active ||
       tac->T311_active ||
       tac->T319_active)
      return;
    tac->N310_cnt++;
    // upon receiving N310 consecutive "out-of-sync" indications
    if (tac->N310_cnt >= tac->N310_k) {
      // start timer T310
        tac->T310_active = true;
        tac->T310_cnt = 0;
        tac->N310_cnt = 0;
    }
  }
}

void set_default_timers_and_constants(NR_UE_Timers_Constants_t *tac)
{
  // 38.331 9.2.3 Default values timers and constants
  tac->T310_k = 1000;
  tac->N310_k = 1;
  tac->T311_k = 30000;
  tac->N311_k = 1;
}

void reset_rlf_timers_and_constants(NR_UE_Timers_Constants_t *tac)
{
  // stop timer T310 for this cell group, if running
  tac->T310_active = false;
  tac->T310_cnt = 0;
  // reset the counters N310 and N311
  tac->N310_cnt = 0;
  tac->N311_cnt = 0;
}
