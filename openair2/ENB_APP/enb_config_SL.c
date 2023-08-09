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

/*
  enb_config_SL.c
  -------------------
  AUTHOR  : T.T.Nguyen / R. Knopp
  COMPANY : EURECOM
  EMAIL   : raymond.knopp@eurecom.fr
*/

#include <string.h>
#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "enb_config.h"
#include "intertask_interface.h"
#include "LTE_SystemInformationBlockType2.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "enb_paramdef.h"
#include "enb_paramdef_sidelink.h"

void fill_SL_configuration(RrcConfigurationReq *RRCcfg, ccparams_sidelink_t *SLconfig, int cell_idx, int cc_idx, char *config_fname)
{
  printf("Configuring SL\n");
  //SIB18
  if (strcmp(SLconfig->rxPool_sc_CP_Len,"normal")==0) {
    RRCcfg->rxPool_sc_CP_Len[cc_idx] = LTE_SL_CP_Len_r12_normal;
  } else if (strcmp(SLconfig->rxPool_sc_CP_Len,"extended")==0) {
    RRCcfg->rxPool_sc_CP_Len[cc_idx] = LTE_SL_CP_Len_r12_extended;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_sc_CP_Len choice: normal,extended!\n",
		 config_fname, cell_idx, SLconfig->rxPool_sc_CP_Len);

  if (strcmp(SLconfig->rxPool_sc_Period,"sf40")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf40;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf60")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf60;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf70")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf70;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf80")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf80;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf120")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf120;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf140")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf140;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf160")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf160;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf240")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf240;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf280")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf280;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"sf320")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_sf320;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"spare6")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_spare6;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"spare5")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_spare5;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"spare4")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_spare4;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"spare3")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_spare3;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"spare2")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_spare2;
  } else if (strcmp(SLconfig->rxPool_sc_Period,"spare")==0) {
    RRCcfg->rxPool_sc_Period[cc_idx] = LTE_SL_PeriodComm_r12_spare;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_sc_Period choice: sf40,sf60,sf70,sf80,sf120,sf140,sf160,sf240,sf280,sf320,spare6,spare5,spare4,spare3,spare2,spare!\n",
		 config_fname, cell_idx, SLconfig->rxPool_sc_Period);

  if (strcmp(SLconfig->rxPool_data_CP_Len,"normal")==0) {
    RRCcfg->rxPool_data_CP_Len[cc_idx] = LTE_SL_CP_Len_r12_normal;
  } else if (strcmp(SLconfig->rxPool_data_CP_Len,"extended")==0) {
    RRCcfg->rxPool_data_CP_Len[cc_idx] = LTE_SL_CP_Len_r12_extended;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_data_CP_Len choice: normal,extended!\n",
		 config_fname, cell_idx, SLconfig->rxPool_data_CP_Len);

  RRCcfg->rxPool_ResourceConfig_prb_Num[cc_idx] = SLconfig->rxPool_ResourceConfig_prb_Num;
  RRCcfg->rxPool_ResourceConfig_prb_Start[cc_idx] = SLconfig->rxPool_ResourceConfig_prb_Start;
  RRCcfg->rxPool_ResourceConfig_prb_End[cc_idx] = SLconfig->rxPool_ResourceConfig_prb_End;

  if (strcmp(SLconfig->rxPool_ResourceConfig_offsetIndicator_present,"prNothing")==0) {
    RRCcfg->rxPool_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_NOTHING;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_offsetIndicator_present,"prSmall")==0) {
    RRCcfg->rxPool_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_small_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_offsetIndicator_present,"prLarge")==0) {
    RRCcfg->rxPool_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_large_r12;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_ResourceConfig_offsetIndicator_present choice: prNothing,prSmal,prLarge!\n",
		 config_fname, cell_idx, SLconfig->rxPool_ResourceConfig_offsetIndicator_present);

  RRCcfg->rxPool_ResourceConfig_offsetIndicator_choice[cc_idx] = SLconfig->rxPool_ResourceConfig_offsetIndicator_choice;

  if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prNothing")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_NOTHING;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs4")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs4_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs8")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs8_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs12")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs12_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs16")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs16_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs30")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs30_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs40")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs40_r12;
  } else if (strcmp(SLconfig->rxPool_ResourceConfig_subframeBitmap_present,"prBs42")==0) {
    RRCcfg->rxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs42_r12;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_ResourceConfig_subframeBitmap_present choice: prNothing,prBs4,prBs8,prBs12,prBs16,prBs30,prBs40,prBs42!\n",
		 config_fname, cell_idx, SLconfig->rxPool_ResourceConfig_subframeBitmap_present);

  RRCcfg->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[cc_idx] = SLconfig->rxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  RRCcfg->rxPool_ResourceConfig_subframeBitmap_choice_bs_size[cc_idx] = SLconfig->rxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  RRCcfg->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[cc_idx] = SLconfig->rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;

  //SIB19 - for discRxPool
  if (strcmp(SLconfig->discRxPool_cp_Len,"normal")==0) {
    RRCcfg->discRxPool_cp_Len[cc_idx] = LTE_SL_CP_Len_r12_normal;
  } else if (strcmp(SLconfig->discRxPool_cp_Len,"extended")==0) {
    RRCcfg->discRxPool_cp_Len[cc_idx] = LTE_SL_CP_Len_r12_extended;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_cp_Len choice: normal,extended!\n",
		 config_fname, cell_idx, SLconfig->discRxPool_cp_Len);

  if (strcmp(SLconfig->discRxPool_discPeriod,"rf32")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf32;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"rf64")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf64;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"rf128")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf128;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"rf256")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf256;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"rf512")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf512;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"rf1024")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf1024;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"rf16")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf16_v1310;
  } else if (strcmp(SLconfig->discRxPool_discPeriod,"spare")==0) {
    RRCcfg->discRxPool_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_spare;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_discPeriod choice: rf32,rf64,rf128,rf512,rf1024,rf16,spare!\n",
		 config_fname, cell_idx, SLconfig->discRxPool_discPeriod);

  RRCcfg->discRxPool_numRetx[cc_idx] = SLconfig->discRxPool_numRetx;
  RRCcfg->discRxPool_numRepetition[cc_idx] = SLconfig->discRxPool_numRepetition;
  RRCcfg->discRxPool_ResourceConfig_prb_Num[cc_idx] = SLconfig->discRxPool_ResourceConfig_prb_Num;
  RRCcfg->discRxPool_ResourceConfig_prb_Start[cc_idx] = SLconfig->discRxPool_ResourceConfig_prb_Start;
  RRCcfg->discRxPool_ResourceConfig_prb_End[cc_idx] = SLconfig->discRxPool_ResourceConfig_prb_End;

  if (strcmp(SLconfig->discRxPool_ResourceConfig_offsetIndicator_present,"prNothing")==0) {
    RRCcfg->discRxPool_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_NOTHING;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_offsetIndicator_present,"prSmall")==0) {
    RRCcfg->discRxPool_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_small_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_offsetIndicator_present,"prLarge")==0) {
    RRCcfg->discRxPool_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_large_r12;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_ResourceConfig_offsetIndicator_present choice: prNothing,prSmal,prLarge!\n",
		 config_fname, cell_idx, SLconfig->discRxPool_ResourceConfig_offsetIndicator_present);

  RRCcfg->discRxPool_ResourceConfig_offsetIndicator_choice[cc_idx] = SLconfig->discRxPool_ResourceConfig_offsetIndicator_choice;

  if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prNothing")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_NOTHING;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs4")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs4_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs8")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs8_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs12")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs12_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs16")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs16_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs30")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs30_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs40")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs40_r12;
  } else if (strcmp(SLconfig->discRxPool_ResourceConfig_subframeBitmap_present,"prBs42")==0) {
    RRCcfg->discRxPool_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs42_r12;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_ResourceConfig_subframeBitmap_present choice: prNothing,prBs4,prBs8,prBs12,prBs16,prBs30,prBs40,prBs42!\n",
		 config_fname, cell_idx, SLconfig->discRxPool_ResourceConfig_subframeBitmap_present);

  RRCcfg->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[cc_idx] = SLconfig->discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  RRCcfg->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[cc_idx] = SLconfig->discRxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  RRCcfg->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[cc_idx] = SLconfig->discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;

  //SIB19 - For discRxPoolPS
  if (strcmp(SLconfig->discRxPoolPS_cp_Len,"normal")==0) {
    RRCcfg->discRxPoolPS_cp_Len[cc_idx] = LTE_SL_CP_Len_r12_normal;
  } else if (strcmp(SLconfig->discRxPoolPS_cp_Len,"extended")==0) {
    RRCcfg->discRxPoolPS_cp_Len[cc_idx] = LTE_SL_CP_Len_r12_extended;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_cp_Len choice: normal,extended!\n",
		 config_fname, cell_idx, SLconfig->discRxPoolPS_cp_Len);

  if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf32")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf32;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf64")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf64;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf128")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf128;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf256")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf256;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf512")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf512;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf1024")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf1024;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"rf16")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_rf16_v1310;
  } else if (strcmp(SLconfig->discRxPoolPS_discPeriod,"spare")==0) {
    RRCcfg->discRxPoolPS_discPeriod[cc_idx] = LTE_SL_DiscResourcePool_r12__discPeriod_r12_spare;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_discPeriod choice: rf32,rf64,rf128,rf512,rf1024,rf16,spare!\n",
		 config_fname, cell_idx, SLconfig->discRxPoolPS_discPeriod);

  RRCcfg->discRxPoolPS_numRetx[cc_idx] = SLconfig->discRxPoolPS_numRetx;
  RRCcfg->discRxPoolPS_numRepetition[cc_idx] = SLconfig->discRxPoolPS_numRepetition;
  RRCcfg->discRxPoolPS_ResourceConfig_prb_Num[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_prb_Num;
  RRCcfg->discRxPoolPS_ResourceConfig_prb_Start[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_prb_Start;
  RRCcfg->discRxPoolPS_ResourceConfig_prb_End[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_prb_End;

  if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_offsetIndicator_present,"prNothing")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_NOTHING;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_offsetIndicator_present,"prSmall")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_small_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_offsetIndicator_present,"prLarge")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_offsetIndicator_present[cc_idx] = LTE_SL_OffsetIndicator_r12_PR_large_r12;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_ResourceConfig_offsetIndicator_present choice: prNothing,prSmal,prLarge!\n",
		 config_fname, cell_idx, SLconfig->discRxPoolPS_ResourceConfig_offsetIndicator_present);

  RRCcfg->discRxPoolPS_ResourceConfig_offsetIndicator_choice[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_offsetIndicator_choice;

  if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prNothing")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_NOTHING;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs4")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs4_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs8")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs8_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs12")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs12_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs16")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs16_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs30")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs30_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs40")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs40_r12;
  } else if (strcmp(SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs42")==0) {
    RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_present[cc_idx] = LTE_SubframeBitmapSL_r12_PR_bs42_r12;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_ResourceConfig_subframeBitmap_present choice: prNothing,prBs4,prBs8,prBs12,prBs16,prBs30,prBs40,prBs42!\n",
		 config_fname, cell_idx, SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_present);

  RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf;
  RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size;
  RRCcfg->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[cc_idx] = SLconfig->discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
} // sidelink_configured==1
