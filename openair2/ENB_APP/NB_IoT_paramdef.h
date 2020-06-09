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

/*! \file openair2/ENB_APP/enb_paramdef.f
 * \brief definition of configuration parameters for all eNodeB modules 
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifndef __ENB_APP_NB_IOT_PARAMDEF__H__
#define __ENB_APP_NB_IOT_PARAMDEF__H__

#include "common/config/config_paramdesc.h"
#include "SystemInformationBlockType2.h"
#include "DL-GapConfig-NB-r13.h"
#include "NPRACH-Parameters-NB-r13.h"
#include "PowerRampingParameters.h"		  
#include "BCCH-Config-NB-r13.h"
#include "PCCH-Config-NB-r13.h"
#include "ACK-NACK-NumRepetitions-NB-r13.h"
#include "TDD-Config.h"




 
/*
  int16_t                 eutra_band;
  uint32_t                downlink_frequency;
  int32_t                 uplink_frequency_offset;
  int16_t                 Nid_cell;// for testing, change later
  int16_t                 N_RB_DL;// for testing, change later
*/
/*-------------------------------------------------------------------------------------------------------------------*/
/* SIB1 parameters possibly coming  from LTE RRC (in band deployment)                                                */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     component carriers configuration parameters                                                                                                                   */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  checked_param */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define NBIOT_LTECCPARAMS_CHECK_DESC { \
             { .s3a= { config_checkstr_assign_integer,  FRAMETYPE_OKVALUES, FRAMETYPE_MODVALUES,2}} ,     \
             { .s2=  { config_check_intrange,           TDDCONFIG_OKRANGE}},                              \
             { .s2=  { config_check_intrange,           TDDCONFIGS_OKRANGE}},                             \
             { .s3a= { config_checkstr_assign_integer,  PREFIX_OKVALUES,    PREFIX_MODVALUES,2}} ,        \
             { .s3a= { config_checkstr_assign_integer,  PREFIXUL_OKVALUES,  PREFIXUL_MODVALUES,2}} ,      \
             { .s5= {NULL }} ,						                                  \
             { .s5= {NULL }} ,						                                  \
             { .s5= {NULL }} ,						                                  \
             { .s5= {NULL }} ,						                                  \
             { .s1= { config_check_intval,              NRBDL_OKVALUES,6}} ,                              \
}


#define NBIOT_LTECCPARAMS_DESC { \
{ENB_CONFIG_STRING_FRAME_TYPE,                        NULL,   0,	   strptr:NULL,       defstrval:"FDD",  	 TYPE_STRING,	  0},  \
{ENB_CONFIG_STRING_TDD_CONFIG,                        NULL,   0,	   iptr:NULL,	      defintval:3,		 TYPE_UINT,	  0},  \
{ENB_CONFIG_STRING_TDD_CONFIG_S,                      NULL,   0,	   iptr:NULL,	      defintval:0,		 TYPE_UINT,	  0},  \
{ENB_CONFIG_STRING_PREFIX_TYPE,                       NULL,   0,	   strptr:NULL,       defstrval:"NORMAL",	 TYPE_STRING,	  0},  \
{ENB_CONFIG_STRING_PREFIX_TYPE_UL,                    NULL,   0,	   strptr:NULL,       defstrval:"NORMAL",	 TYPE_STRING,	  0},  \
{ENB_CONFIG_STRING_EUTRA_BAND,                        NULL,   0,	   iptr:NULL,	      defintval:7,		 TYPE_UINT,	  0},  \
{ENB_CONFIG_STRING_DOWNLINK_FREQUENCY,                NULL,   0,	   i64ptr:NULL,       defint64val:2680000000,	 TYPE_UINT64,	  0},  \
{ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET,           NULL,   0,	   iptr:NULL,	      defintval:-120000000,	 TYPE_INT,	  0},  \
{ENB_CONFIG_STRING_NID_CELL,                          NULL,   0,	   iptr:NULL,	      defintval:0,		 TYPE_UINT,	  0},  \
{ENB_CONFIG_STRING_N_RB_DL,                           NULL,   0,	   iptr:NULL,	      defintval:25,		 TYPE_UINT,	  0},  \
}
#define LTECCPARAMS_FRAME_TYPE_IDX                  0
#define LTECCPARAMS_TDD_CONFIG_IDX                  1
#define LTECCPARAMS_TDD_CONFIG_S_IDX                2
#define LTECCPARAMS_PREFIX_TYPE_IDX                 3
#define LTECCPARAMS_PREFIX_TYPE_UL_IDX              4
#define LTECCPARAMS_EUTRA_BAND_IDX                  5
#define LTECCPARAMS_DOWNLINK_FREQUENCY_IDX          6
#define LTECCPARAMS_UPLINK_FREQUENCY_OFFSET_IDX     7
#define LTECCPARAMS_NID_CELL_IDX                    8
#define LTECCPARAMS_N_RB_DL_IDX                     9


/*-------------------------------------------------------------------------------------------------------------------*/

/* NB-Iot RRC list section name */		
#define NBIOT_RRCLIST_CONFIG_STRING          "NB-IoT_RRCs"		 


#define RACH_RARESPONSEWINDOWSIZE_NB_OKVALUES                   {20,50,80,120,180,240,320,400}
#define PREF1(A)                                                 RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_ ## A
#define RACH_RARESPONSEWINDOWSIZE_NB_MODVALUES                  { PREF1(sf20),PREF1(sf50),PREF1(sf80),PREF1(sf120),    \
                                                                  PREF1(sf180),PREF1(sf240),PREF1(sf320),PREF1(sf400) }


#define RACH_MACCONTENTIONRESOLUTIONTIMER_NB_OKVALUES           {80,100,120,160,200,240,480,960} 
#define PREF2(A)                                                 RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_ ## A 
#define RACH_MACCONTENTIONRESOLUTIONTIMER_NB_MODVALUES          { PREF2(sf80),PREF2(sf100),PREF2(sf120),PREF2(sf160),    \
                                                                  PREF2(sf200),PREF2(sf240),PREF2(sf480),PREF2(sf960) }


#define RACH_POWERRAMPINGSTEP_NB_OKVALUES                       {0,2,4,6}
#define PREF3(A)                                                 PowerRampingParameters__powerRampingStep_ ## A
#define RACH_POWERRAMPINGSTEP_NB_MODVALUES                      { PREF3(dB0),PREF3(dB2),PREF3(dB4),PREF3(dB6) }


#define RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER_NB_OKRANGE      {-120, -90}

#define RACH_PREAMBLETRANSMAX_CE_NB_OKVALUES                    {3,4,5,6,7,8,10,20,50,100,200}
#define PREF4(A)                                                PreambleTransMax_ ## A 
#define RACH_PREAMBLETRANSMAX_CE_NB_MODVALUES                   { PREF4(n3), PREF4(n4), PREF4(n5), PREF4(n6),  PREF4(n7), PREF4(n8), \
                                                                  PREF4(n10),PREF4(n20),PREF4(n50),PREF4(n100),PREF4(n200) }

#define BCCH_MODIFICATIONPERIODCOEFF_NB_OKVALUES                {16,32,64,128}
#define PREF5(A)                                                BCCH_Config_NB_r13__modificationPeriodCoeff_r13_ ## A 
#define BCCH_MODIFICATIONPERIODCOEFF_NB_MODVALUES              { PREF5(n16), PREF5(n32), PREF5(n64),PREF5(n128) }

#define PCCH_DEFAULTPAGINGCYCLE_NB_OKVALUES                     {128,256,512,1024}
#define PREF6(A)                                                PCCH_Config_NB_r13__defaultPagingCycle_r13_ ## A 
#define PCCH_DEFAULTPAGINGCYCLE_NB_MODVALUES                    { PREF6(rf128), PREF6(rf256), PREF6(rf512), PREF6(rf1024) }

#define NPRACH_CP_LENGTH_OKVALUES                               {0,1}
#define NPRACH_RSRP_RANGE_OKVALUES                              {0,96}

#define MSG3RANGESTART_OKVALUES                                 {"zero","oneThird","twoThird","one"}
#define MSG3RANGESTART_MODVALUES {NPRACH_Parameters_NB_r13__nprach_SubcarrierMSG3_RangeStart_r13_zero,     NPRACH_Parameters_NB_r13__nprach_SubcarrierMSG3_RangeStart_r13_oneThird, \
                                  NPRACH_Parameters_NB_r13__nprach_SubcarrierMSG3_RangeStart_r13_twoThird, NPRACH_Parameters_NB_r13__nprach_SubcarrierMSG3_RangeStart_r13_one}


#define MAXNUMPREAMBLEATTEMPTCE_OKVALUES    {3,4,5,6,7,8,10}
#define MAXNUMPREAMBLEATTEMPTCE_MODVALUES   {  NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n3, NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n4, \
                                               NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n5, NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n6, \
                                               NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n7, NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n8, \
                                               NPRACH_Parameters_NB_r13__maxNumPreambleAttemptCE_r13_n10 }

#define NPDSCH_NRS_POWER_OKRANGE                                {-60,50}

#define NPUSCH_ACK_NACK_NUMREPETITIONS_NB_OKVALUES              {1,2,4,8,16,32,64,128}
#define PREF9(A)                                                ACK_NACK_NumRepetitions_NB_r13_ ## A 
#define NPUSCH_ACK_NACK_NUMREPETITIONS_NB_MODVALUES             { PREF9(r1),   PREF9(r2),   PREF9(r4),   PREF9(r8), \
                                                                  PREF9(r16),  PREF9(r32),  PREF9(r64),  PREF9(r128) }

#define NPUSCH_SRS_SUBFRAMECONFIG_NB_OKRANGE                    {0,15}
#define NPUSCH_THREETONE_CYCLICSHIFT_R13_OKRANGE                {0,2}
#define NPUSCH_SIXTONE_CYCLICSHIFT_R13_OKRANGE                  {0,3}

#define NPUSCH_GROUPHOPPINGENABLED_OKVALUES                     {"enable","disable"}
#define NPUSCH_GROUPHOPPINGENABLED_MODVALUES                    {1,0}

#define NPUSCH_GROUPASSIGNMENTNPUSCH_R13_OKRANGE                {0,29}

#define DLGAPTHRESHOLD_OKVALUES	       {32,64,128,256}
#define DLGAPTHRESHOLD_MODVALUES       { DL_GapConfig_NB_r13__dl_GapThreshold_r13_n32,  DL_GapConfig_NB_r13__dl_GapThreshold_r13_n64,  \
                                         DL_GapConfig_NB_r13__dl_GapThreshold_r13_n128, DL_GapConfig_NB_r13__dl_GapThreshold_r13_n256} \

#define DLGAPPERIODICITY_OKVALUES      {64,128,256,512}
#define DLGAPPERIODICITY_MODVALUES     { DL_GapConfig_NB_r13__dl_GapPeriodicity_r13_sf64, DL_GapConfig_NB_r13__dl_GapPeriodicity_r13_sf128, \
          		 	       DL_GapConfig_NB_r13__dl_GapPeriodicity_r13_sf256,DL_GapConfig_NB_r13__dl_GapPeriodicity_r13_sf512}

#define DLGAPDURATION_OKVALUES         {"oneEighth","oneFourth","threeEighth","oneHalf"}
#define DLGAPDURATION_MODVALUES        {DL_GapConfig_NB_r13__dl_GapDurationCoeff_r13_oneEighth,   DL_GapConfig_NB_r13__dl_GapDurationCoeff_r13_oneFourth, \
                                        DL_GapConfig_NB_r13__dl_GapDurationCoeff_r13_threeEighth, DL_GapConfig_NB_r13__dl_GapDurationCoeff_r13_oneHalf}

#define NPUSCH_P0_NOMINALNPUSCH_OKRANGE                         {-126,24}

#define NPUSCH_ALPHA_OKVALUES                                    {"AL0","AL04","AL05","AL06","AL07","AL08","AL09","AL1"} 
#define NPUSCH_ALPHA_MODVALUES                                   { Alpha_r12_al0, Alpha_r12_al04,  Alpha_r12_al05,  Alpha_r12_al06, \
                                                                   Alpha_r12_al07, Alpha_r12_al08,  Alpha_r12_al09,  Alpha_r12_al1}

#define DELTAPREAMBLEMSG3_OKRANGE                               {-1,6}	





#define NBIOT_RRCPARAMS_CHECK_DESC { \
             { .s1a= { config_check_modify_integer,     RACH_RARESPONSEWINDOWSIZE_NB_OKVALUES,          RACH_RARESPONSEWINDOWSIZE_NB_MODVALUES,          8}}, 	 \
             { .s1a= { config_check_modify_integer,     RACH_MACCONTENTIONRESOLUTIONTIMER_NB_OKVALUES,  RACH_MACCONTENTIONRESOLUTIONTIMER_NB_MODVALUES,  8}},	 \
             { .s1a= { config_check_modify_integer,     RACH_POWERRAMPINGSTEP_NB_OKVALUES,              RACH_POWERRAMPINGSTEP_NB_MODVALUES,              4}} ,    \
             { .s2=  { config_check_intrange,           RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER_NB_OKRANGE}},   	    	  \
             { .s1a= { config_check_modify_integer,     RACH_PREAMBLETRANSMAX_CE_NB_OKVALUES,           RACH_PREAMBLETRANSMAX_CE_NB_MODVALUES,          11}} ,    \
             { .s1a= { config_check_modify_integer,     BCCH_MODIFICATIONPERIODCOEFF_NB_OKVALUES,       BCCH_MODIFICATIONPERIODCOEFF_NB_MODVALUES,       4}} ,    \
             { .s1a= { config_check_modify_integer,     PCCH_DEFAULTPAGINGCYCLE_NB_OKVALUES,            PCCH_DEFAULTPAGINGCYCLE_NB_MODVALUES,            4}} ,    \
             { .s1=  { NULL,		           NPRACH_CP_LENGTH_OKVALUES ,4}} ,			     	    	  \
             { .s2=  { config_check_intrange,	        NPRACH_RSRP_RANGE_OKVALUES}} ,			     	    	  \
             { .s3a= { config_checkstr_assign_integer,  MSG3RANGESTART_OKVALUES,                        MSG3RANGESTART_MODVALUES,                       4}} ,    \
             { .s1a= { config_check_modify_integer,     MAXNUMPREAMBLEATTEMPTCE_OKVALUES,               MAXNUMPREAMBLEATTEMPTCE_MODVALUES,              7}} ,    \
             { .s1=  { config_check_intval,             NPDSCH_NRS_POWER_OKRANGE,4}} ,			     	     	  \
             { .s1a= { config_check_modify_integer,     NPUSCH_ACK_NACK_NUMREPETITIONS_NB_OKVALUES,     NPUSCH_ACK_NACK_NUMREPETITIONS_NB_MODVALUES,    8}} ,    \
             { .s2=  { config_check_intrange,           NPUSCH_SRS_SUBFRAMECONFIG_NB_OKRANGE}} , 	             	     	  \
             { .s2=  { config_check_intrange,           NPUSCH_THREETONE_CYCLICSHIFT_R13_OKRANGE}} ,	     	     	  \
             { .s2=  { config_check_intrange,           NPUSCH_SIXTONE_CYCLICSHIFT_R13_OKRANGE}} ,	             	     	  \
             { .s3a= { config_checkstr_assign_integer,  NPUSCH_GROUPHOPPINGENABLED_OKVALUES,          NPUSCH_GROUPHOPPINGENABLED_MODVALUES,            2}} ,    \
             { .s2=  { config_check_intrange,           NPUSCH_GROUPASSIGNMENTNPUSCH_R13_OKRANGE}} ,	     	     	  \
             { .s1a= { config_check_modify_integer,     DLGAPTHRESHOLD_OKVALUES,                       DLGAPTHRESHOLD_MODVALUES,                        4}} ,    \
             { .s1a= { config_check_modify_integer,     DLGAPPERIODICITY_OKVALUES,                     DLGAPPERIODICITY_MODVALUES,                      4}} ,    \
             { .s3a= { config_checkstr_assign_integer,  DLGAPDURATION_OKVALUES,                        DLGAPDURATION_MODVALUES ,                        4}} ,    \
             { .s2=  { config_check_intrange,           NPUSCH_P0_NOMINALNPUSCH_OKRANGE}} ,		     	     	  \
             { .s3a= { config_checkstr_assign_integer,  NPUSCH_ALPHA_OKVALUES,                        NPUSCH_ALPHA_MODVALUES,                          8}} ,    \
             { .s2=  { config_check_intrange,           DELTAPREAMBLEMSG3_OKRANGE}} ,			     	     	  \
             { .s1a= { config_check_modify_integer,    UETIMER_T300_OKVALUES, UETIMER_T300_MODVALUES,8}} ,	     \
             { .s1a= { config_check_modify_integer,    UETIMER_T301_OKVALUES, UETIMER_T301_MODVALUES,8}} ,	     \
             { .s1a= { config_check_modify_integer,    UETIMER_T310_OKVALUES, UETIMER_T310_MODVALUES,7}} ,	     \
             { .s1a= { config_check_modify_integer,    UETIMER_T311_OKVALUES, UETIMER_T311_MODVALUES,7}} ,	     \
             { .s1a= { config_check_modify_integer,    UETIMER_N310_OKVALUES, UETIMER_N310_MODVALUES,8}} ,	     \
             { .s1a= { config_check_modify_integer,    UETIMER_N311_OKVALUES, UETIMER_N311_MODVALUES,8}} ,	     \
}
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                                           NB-IoT RRC  configuration parameters                                          */
/*   optname                                       helpstr   paramflags    XXXptr	 defXXXval		      type	   numelt  */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define NBIOTRRCPARAMS_DESC { \
{"rach_raResponseWindowSize_NB",                   NULL,   0,		   uptr:NULL,	 defintval:20,  	TYPE_UINT,	 0},  \
{"rach_macContentionResolutionTimer_NB",           NULL,   0,		   uptr:NULL,	 defintval:80,  	TYPE_UINT,	 0},  \
{"rach_powerRampingStep_NB",                       NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"rach_preambleInitialReceivedTargetPower_NB",     NULL,   0,		   iptr:NULL,	 defintval:-112,	TYPE_INT32,	 0},  \
{"rach_preambleTransMax_CE_NB",                    NULL,   0,		   uptr:NULL,	 defintval:3,		TYPE_UINT,	 0},  \
{"bcch_modificationPeriodCoeff_NB",                NULL,   0,		   uptr:NULL,	 defintval:16,		TYPE_UINT,	 0},  \
{"pcch_defaultPagingCycle_NB",                     NULL,   0,		   uptr:NULL,	 defintval:256,		TYPE_UINT,	 0},  \
{"nprach_CP_Length",                               NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"nprach_rsrp_range",                              NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"nprach_SubcarrierMSG3_RangeStart",               NULL,   0,		   strptr:NULL,  defstrval:"one",	TYPE_STRING,	 0},  \
{"maxNumPreambleAttemptCE_NB",                     NULL,   0,		   uptr:NULL,	 defintval:10,  	TYPE_UINT,	 0},  \
{"npdsch_nrs_Power",                               NULL,   0,		   iptr:NULL,	 defintval:0,		TYPE_INT,	 0},  \
{"npusch_ack_nack_numRepetitions_NB",              NULL,   0,		   uptr:NULL,	 defintval:1,		TYPE_UINT,	 0},  \
{"npusch_srs_SubframeConfig_NB",                   NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"npusch_threeTone_CyclicShift_r13",               NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"npusch_sixTone_CyclicShift_r13",                 NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"npusch_groupHoppingEnabled",                     NULL,   0,              strptr:NULL,	 defstrval:"disable",	TYPE_STRING,	 0},  \
{"npusch_groupAssignmentNPUSCH_r13",               NULL,   0,		   uptr:NULL,	 defintval:0,		TYPE_UINT,	 0},  \
{"dl_GapThreshold_NB",                             NULL,   0,		   uptr:NULL,	 defintval:32,  	TYPE_UINT,	 0},  \
{"dl_GapPeriodicity_NB",                           NULL,   0,		   uptr:NULL,	 defintval:64,  	TYPE_UINT,	 0},  \
{"dl_GapDurationCoeff_NB",                         NULL,   0,		   strptr:NULL,  defstrval:"oneEighth", TYPE_STRING,	 0},  \
{"npusch_p0_NominalNPUSCH",                        NULL,   0,		   iptr:NULL,	 defintval:0,		TYPE_INT32,	 0},  \
{"npusch_alpha",                                   NULL,   0,		   strptr:NULL,	 defstrval:"AL0",  	TYPE_STRING,	 0},  \
{"deltaPreambleMsg3",                              NULL,   0,		   iptr:NULL,	 defintval:0,		TYPE_INT32,	 0},  \
{"ue_TimersAndConstants_t300_NB",                  NULL,   0,		   uptr:NULL,	 defintval:1000,	TYPE_UINT,	 0},  \
{"ue_TimersAndConstants_t301_NB",                  NULL,   0,		   uptr:NULL,	 defintval:1000,	TYPE_UINT,	 0},  \
{"ue_TimersAndConstants_t310_NB",                  NULL,   0,		   uptr:NULL,	 defintval:1000,	TYPE_UINT,	 0},  \
{"ue_TimersAndConstants_t311_NB",                  NULL,   0,		   uptr:NULL,	 defintval:10000,	TYPE_UINT,	 0},  \
{"ue_TimersAndConstants_n310_NB",                  NULL,   0,		   uptr:NULL,	 defintval:20,  	TYPE_UINT,	 0},  \
{"ue_TimersAndConstants_n311_NB",                  NULL,   0,		   uptr:NULL,	 defintval:1,		TYPE_UINT,	 0},  \
}								    

#define NBIOT_RACH_RARESPONSEWINDOWSIZE_NB_IDX                         0	   
#define NBIOT_RACH_MACCONTENTIONRESOLUTIONTIMER_NB_IDX                 1
#define NBIOT_RACH_POWERRAMPINGSTEP_NB_IDX                             2
#define NBIOT_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER_NB_IDX           3
#define NBIOT_RACH_PREAMBLETRANSMAX_CE_NB_IDX                          4		   
#define NBIOT_BCCH_MODIFICATIONPERIODCOEFF_NB_IDX                      5
#define NBIOT_PCCH_DEFAULTPAGINGCYCLE_NB_IDX                           6
#define NBIOT_NPRACH_CP_LENGTH_IDX                                     7
#define NBIOT_NPRACH_RSRP_RANGE_IDX                                    8
#define NBIOT_NPRACH_SUBCARRIERMSG3_RANGESTART_IDX                     9
#define NBIOT_MAXNUMPREAMBLEATTEMPTCE_NB_IDX                           10
#define NBIOT_NPDSCH_NRS_POWER_IDX                                     11
#define NBIOT_NPUSCH_ACK_NACK_NUMREPETITIONS_NB_IDX                    12
#define NBIOT_NPUSCH_SRS_SUBFRAMECONFIG_NB_IDX                         13
#define NBIOT_NPUSCH_THREETONE_CYCLICSHIFT_R13_IDX                     14
#define NBIOT_NPUSCH_SIXTONE_CYCLICSHIFT_R13_IDX                       15
#define NBIOT_NPUSCH_GROUPHOPPINGENABLED_IDX                           16
#define NBIOT_NPUSCH_GROUPASSIGNMENTNPUSCH_R13_IDX                     17
#define NBIOT_DL_GAPTHRESHOLD_NB_IDX                                   18
#define NBIOT_DL_GAPPERIODICITY_NB_IDX                                 19
#define NBIOT_DL_GAPDURATIONCOEFF_NB_IDX                               20
#define NBIOT_NPUSCH_P0_NOMINALNPUSCH_IDX                              21
#define NBIOT_NPUSCH_ALPHA_IDX                                         22
#define NBIOT_DELTAPREAMBLEMSG3_IDX                                    23
#define NBIOT_UE_TIMERSANDCONSTANTS_T300_NB_IDX                        24
#define NBIOT_UE_TIMERSANDCONSTANTS_T301_NB_IDX                        25
#define NBIOT_UE_TIMERSANDCONSTANTS_T310_NB_IDX                        26
#define NBIOT_UE_TIMERSANDCONSTANTS_T311_NB_IDX                        27
#define NBIOT_UE_TIMERSANDCONSTANTS_N310_NB_IDX                        28
#define NBIOT_UE_TIMERSANDCONSTANTS_N311_NB_IDX                        29

/* NB-Iot RRC: link to LTE RRC section name */		
#define NBIOT_LTERRCREF_CONFIG_STRING          "LTERRC_Ref"
/*---------------------------------------------------------------------------------------------------------------*/
/*          NB-IoT RRC configuration parameters to link to a LTE RRC instance (in-guard, in-band)                */
/*   optname                        helpstr   paramflags    XXXptr	 defXXXval	   type	         numelt  */
/*---------------------------------------------------------------------------------------------------------------*/
#define NBIOTRRCPARAMS_RRCREF_DESC { \
{"RRC_inst",                         NULL,   0,            uptr:NULL,     defintval:0,	  TYPE_UINT,	   0},  \
{"CC_inst",                          NULL,   0,            uptr:NULL,     defintval:0, 	  TYPE_UINT,	   0},  \
}
/*--------------------------------------------------------------------------------------------------------------*/
#define NBIOT_RRCINST_IDX             0
#define NBIOT_CCINST_IDX              1

#define NBIOT_RRCLIST_NPRACHPARAMS_CONFIG_STRING        "NPRACH-NB-r13"


#define  NPRACH_PERIODICITY_OKVALUES   {40,80,160,240,320,640,1280,2560}
#define  NPRACH_PERIODICITY_MODVALUES  { NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms40,   NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms80,  \
                                         NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms160,  NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms240, \
                                         NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms320,  NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms640, \
                                         NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms1280, NPRACH_Parameters_NB_r13__nprach_Periodicity_r13_ms2560 }

#define  NPRACH_STARTTIME_OKVALUES   {8,16,32,64,128,256,512,1024}
#define  NPRACH_STARTTIME_MODVALUES  {  NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms8,   NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms16,        \
                                        NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms32,  NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms64,        \
                                        NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms128, NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms256,       \
                                        NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms512, NPRACH_Parameters_NB_r13__nprach_StartTime_r13_ms1024 }
    
#define  NPRACH_SUBCARRIEROFFSET_OKVALUES      {0,12,24,36,2,18,34}
#define  NPRACH_SUBCARRIEROFFSET_MODVALUES     {  NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n0,  NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n12, \
                                                  NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n24, NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n36, \
                                                  NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n2,  NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n18, \
                                                  NPRACH_Parameters_NB_r13__nprach_SubcarrierOffset_r13_n34 }

#define  NPRACH_NUMSUBCARRIERS_OKVALUES      {12,24,36,48}
#define  NPRACH_NUMSUBCARRIERS_MODVALUES     {  NPRACH_Parameters_NB_r13__nprach_NumSubcarriers_r13_n12, NPRACH_Parameters_NB_r13__nprach_NumSubcarriers_r13_n24, \
                                                NPRACH_Parameters_NB_r13__nprach_NumSubcarriers_r13_n36, NPRACH_Parameters_NB_r13__nprach_NumSubcarriers_r13_n48 }


#define  NUMREPETITIONSPERPREAMBLEATTEMPT_OKVALUES  {1,2,4,8,16,32,64,128}
#define  NUMREPETITIONSPERPREAMBLEATTEMPT_MODVALUES {  NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n1,  NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n2,   \
                                                       NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n4,  NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n8,   \
                                                       NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n16, NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n32,  \
                                                       NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n64, NPRACH_Parameters_NB_r13__numRepetitionsPerPreambleAttempt_r13_n128} 

#define  NPDCCHNUMREPETITIONSRA_OKVALUES  {1,2,4,8,16,32,64,128,256,512,1024,2048}
#define  NPDCCHNUMREPETITIONSRA_MODVALUES {  NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r1,   NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r2,	\
                                    	     NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r4,   NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r8,	\
                                    	     NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r16,  NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r32,	\
                                    	     NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r64,  NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r128,  \
                                    	     NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r256, NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r512,  \
                                    	     NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r1024,NPRACH_Parameters_NB_r13__npdcch_NumRepetitions_RA_r13_r2048}

#define  NPDCCHSTARTSFCSSRA_OKVALUES  {1,2,4,8,16,32,48,64}
#define  NPDCCHSTARTSFCSSRA_MODVALUES {  NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v1dot5,  NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v2,	\
                                         NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v4,      NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v8,   \
                                         NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v16,     NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v32,  \
                                         NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v48,     NPRACH_Parameters_NB_r13__npdcch_StartSF_CSS_RA_r13_v64}

#define  NPDCCHOFFSETRA_OKVALUES    {"zero","oneEighth","oneFourth","threeEighth"}
#define  NPDCCHOFFSETRA_MODVALUES   { NPRACH_Parameters_NB_r13__npdcch_Offset_RA_r13_zero,	NPRACH_Parameters_NB_r13__npdcch_Offset_RA_r13_oneEighth,     \
                                      NPRACH_Parameters_NB_r13__npdcch_Offset_RA_r13_oneFourth, NPRACH_Parameters_NB_r13__npdcch_Offset_RA_r13_threeEighth}



#define NBIOT_RRCLIST_NPRACHPARAMSCHECK_DESC { \
             { .s1a= { config_check_modify_integer,    NPRACH_PERIODICITY_OKVALUES,		  NPRACH_PERIODICITY_MODVALUES, 	     8 }},	\
             { .s1a= { config_check_modify_integer,    NPRACH_STARTTIME_OKVALUES,		  NPRACH_STARTTIME_MODVALUES,  	             8 }},	\
             { .s1a= { config_check_modify_integer,    NPRACH_SUBCARRIEROFFSET_OKVALUES,	  NPRACH_SUBCARRIEROFFSET_MODVALUES,	     7 }},	\
             { .s1a= { config_check_modify_integer,    NPRACH_NUMSUBCARRIERS_OKVALUES,  	  NPRACH_NUMSUBCARRIERS_MODVALUES,	     4 }},	\
             { .s1a= { config_check_modify_integer,    NUMREPETITIONSPERPREAMBLEATTEMPT_OKVALUES, NUMREPETITIONSPERPREAMBLEATTEMPT_MODVALUES,8 }},	\
             { .s1a= { config_check_modify_integer,    NPDCCHNUMREPETITIONSRA_OKVALUES, 	  NPDCCHNUMREPETITIONSRA_MODVALUES,	     12}},	\
             { .s1a= { config_check_modify_integer,    NPDCCHSTARTSFCSSRA_OKVALUES,		  NPDCCHSTARTSFCSSRA_MODVALUES,	             8 }},	\
             { .s3a= { config_checkstr_assign_integer, NPDCCHOFFSETRA_OKVALUES,                   NPDCCHOFFSETRA_MODVALUES,                  4 }},	\
}



/*------------------------------------------------------------------------------------------------------------------------------*/
/* NB-IoT NPrach parameters, there will be three ocuurences of these parameters in each RRC instance                            */
 /*   optname                                helpstr   paramflags    XXXptr	 defXXXval		  type	        numelt  */
/*------------------------------------------------------------------------------------------------------------------------------*/
#define NBIOTRRC_NPRACH_PARAMS_DESC { \
{"nprach_Periodicity",                        NULL,   0,            uptr:NULL,     defintval:320,	  TYPE_UINT,	   0},  \
{"nprach_StartTime",                          NULL,   0,            uptr:NULL,     defintval:8, 	  TYPE_UINT,	   0},  \
{"nprach_SubcarrierOffset",                   NULL,   0,            uptr:NULL,     defintval:0, 	  TYPE_UINT,	   0},  \
{"nprach_NumSubcarriers",                     NULL,   0,            uptr:NULL,     defintval:12,	  TYPE_UINT,	   0},  \
{"numRepetitionsPerPreambleAttempt",          NULL,   0,            uptr:NULL,     defintval:2, 	  TYPE_UINT,	   0},  \
{"npdcch_NumRepetitions_RA",                  NULL,   0,            uptr:NULL,     defintval:16,	  TYPE_UINT,	   0},  \
{"npdcch_StartSF_CSS_RA",                     NULL,   0,            uptr:NULL,     defintval:2,	          TYPE_UINT,       0},  \
{"npdcch_Offset_RA",                          NULL,   0,            strptr:NULL,   defstrval:"zero",      TYPE_STRING,     0},  \
}

#define NBIOT_NPRACH_PERIODICITY_IDX                                   0
#define NBIOT_NPRACH_STARTTIME_IDX                                     1
#define NBIOT_NPRACH_SUBCARRIEROFFSET_IDX                              2
#define NBIOT_NPRACH_NUMSUBCARRIERS_IDX                                3
#define NBIOT_NUMREPETITIONSPERPREAMBLEATTEMPT_NB_IDX                  4
#define NBIOT_NPDCCH_NUMREPETITIONS_RA_IDX                             5
#define NBIOT_NPDCCH_STARTSF_CSS_RA_IDX                                6
#define NBIOT_NPDCCH_OFFSET_RA_IDX                                     7

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* NB IoT MACRLC configuration list section name   */
#define NBIOT_MACRLCLIST_CONFIG_STRING                          "NB-IoT_MACRLCs"


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* NB IoT L1 configuration list section name   */
#define NBIOT_L1LIST_CONFIG_STRING                          "NB-IoT_L1s"

#endif
