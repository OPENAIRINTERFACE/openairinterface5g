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

/*! \file prs_nr_paramdef.h
 * \brief definition of configuration parameters for PRS
 * \author
 * \date 2022
 * \version 0.1
 * \company EURECOM
 * \email:
 * \note
 * \warning
 */

#ifndef __PRS_NR_PARAMDEF__H__
#define __PRS_NR_PARAMDEF__H__

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* PRS configuration section names */
#define CONFIG_STRING_PRS_LIST                              "PRSs"
#define CONFIG_STRING_PRS_CONFIG                            "prs_config"


/* Global parameters */
#define CONFIG_STRING_ACTIVE_GNBs                           "Active_gNBs"
#define HELP_STRING_ACTIVE_GNBs                             "Number of active gNBs simultaneously transmitting PRS signal to a UE\n"
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            PRS configuration          parameters                                                                   */
/*   optname                                         helpstr            paramflags    XXXptr              defXXXval                type        numelt */
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define PRS_GLOBAL_PARAMS_DESC { \
  {CONFIG_STRING_ACTIVE_GNBs,  HELP_STRING_ACTIVE_GNBs, 0,        .uptr=NULL,         .defuintval=0,           TYPE_UINT,     0}    \
}
// clang-format on

#define PRS_ACTIVE_GNBS_IDX                          0
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

/* PRS configuration parameters names */
#define CONFIG_STRING_GNB_ID                                "gNB_id"
#define CONFIG_STRING_NUM_PRS_RESOURCES                     "NumPRSResources"
#define CONFIG_STRING_PRS_RESOURCE_SET_PERIOD_LIST          "PRSResourceSetPeriod"
#define CONFIG_STRING_PRS_SYMBOL_START_LIST                 "SymbolStart"
#define CONFIG_STRING_PRS_NUM_SYMBOLS_LIST                  "NumPRSSymbols"
#define CONFIG_STRING_PRS_NUM_RB                            "NumRB"
#define CONFIG_STRING_PRS_RB_OFFSET                         "RBOffset"
#define CONFIG_STRING_PRS_COMB_SIZE                         "CombSize"
#define CONFIG_STRING_PRS_RE_OFFSET_LIST                    "REOffset"
#define CONFIG_STRING_PRS_RESOURCE_OFFSET_LIST              "PRSResourceOffset"
#define CONFIG_STRING_PRS_RESOURCE_REPETITION               "PRSResourceRepetition"
#define CONFIG_STRING_PRS_RESOURCE_TIME_GAP                 "PRSResourceTimeGap"
#define CONFIG_STRING_PRS_ID_LIST                           "NPRS_ID"
#define CONFIG_STRING_PRS_MUTING_PATTERN1_LIST              "MutingPattern1"
#define CONFIG_STRING_PRS_MUTING_PATTERN2_LIST              "MutingPattern2"
#define CONFIG_STRING_PRS_MUTING_BIT_REPETITION             "MutingBitRepetition"

/* Help string for PRS parameters */
#define HELP_STRING_GNB_ID                                  "gNB index for UE (<= CombSize)\n"
#define HELP_STRING_NUM_PRS_RESOURCES                       "Number of PRS resources in a PRS resource set\n"
#define HELP_STRING_PRS_RESOURCE_SET_PERIOD_LIST            "[slot period, slot offset] of a PRS resource set\n"
#define HELP_STRING_PRS_SYMBOL_START_LIST                   "Starting OFDM symbol of each PRS resource in a PRS resource set\n"
#define HELP_STRING_PRS_NUM_SYMBOLS_LIST                    "Number of OFDM symbols in a slot for each PRS resource in a PRS resource set\n"
#define HELP_STRING_PRS_NUM_RB                              "Number of PRBs allocated to all PRS resources in a PRS resource set (<= 272 and multiples of 4)\n"
#define HELP_STRING_PRS_RB_OFFSET                           "Starting PRB index of all PRS resources in a PRS resource set\n"
#define HELP_STRING_PRS_COMB_SIZE                           "RE density of all PRS resources in a PRS resource set (2, 4, 6, 12)\n"
#define HELP_STRING_PRS_RE_OFFSET_LIST                      "Starting RE offset in the first OFDM symbol of each PRS resource in a PRS resource set\n"
#define HELP_STRING_PRS_RESOURCE_OFFSET_LIST                "Slot offset of each PRS resource defined relative to the slot offset of the PRS resource set (0...511)\n"
#define HELP_STRING_PRS_RESOURCE_REPETITION                 "Repetition factor for all PRS resources in resource set (1 /*default*/, 2, 4, 6, 8, 16, 32)\n"
#define HELP_STRING_PRS_RESOURCE_TIME_GAP                   "Slot offset between two consecutive repetition indices of all PRS resources in a PRS resource set (1 /*default*/, 2, 4, 6, 8, 16, 32)\n"
#define HELP_STRING_PRS_ID_LIST                             "Sequence identity of each PRS resource in a PRS resource set, specified in the range [0, 4095]\n"
#define HELP_STRING_PRS_MUTING_PATTERN1_LIST                "Muting bit pattern option-1, specified as [] or a binary-valued vector of length 2, 4, 6, 8, 16, or 32\n"
#define HELP_STRING_PRS_MUTING_PATTERN2_LIST                "Muting bit pattern option-2, specified as [] or a binary-valued vector of length 2, 4, 6, 8, 16, or 32\n"
#define HELP_STRING_PRS_MUTING_BIT_REPETITION               "Muting bit repetition factor, specified as 1, 2, 4, or 8\n"


/*----------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            PRS configuration                parameters                                                                         */
/*   optname                                         helpstr                  paramflags    XXXptr              defXXXval                  type           numelt  */
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define PRS_PARAMS_DESC { \
  {CONFIG_STRING_GNB_ID,                        HELP_STRING_GNB_ID,                       0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_NUM_PRS_RESOURCES,             HELP_STRING_NUM_PRS_RESOURCES,            0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_PRS_RESOURCE_SET_PERIOD_LIST,  HELP_STRING_PRS_RESOURCE_SET_PERIOD_LIST, 0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_SYMBOL_START_LIST,         HELP_STRING_PRS_SYMBOL_START_LIST,        0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_NUM_SYMBOLS_LIST,          HELP_STRING_PRS_NUM_SYMBOLS_LIST,         0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_NUM_RB,                    HELP_STRING_PRS_NUM_RB,                   0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_PRS_RB_OFFSET,                 HELP_STRING_PRS_RB_OFFSET,                0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_PRS_COMB_SIZE,                 HELP_STRING_PRS_COMB_SIZE,                0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_PRS_RE_OFFSET_LIST,            HELP_STRING_PRS_RE_OFFSET_LIST,           0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_RESOURCE_OFFSET_LIST,      HELP_STRING_PRS_RESOURCE_OFFSET_LIST,     0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_RESOURCE_REPETITION,       HELP_STRING_PRS_RESOURCE_REPETITION,      0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_PRS_RESOURCE_TIME_GAP,         HELP_STRING_PRS_RESOURCE_TIME_GAP,        0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0},  \
  {CONFIG_STRING_PRS_ID_LIST,                   HELP_STRING_PRS_ID_LIST,                  0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_MUTING_PATTERN1_LIST,      HELP_STRING_PRS_MUTING_PATTERN1_LIST,     0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_MUTING_PATTERN2_LIST,      HELP_STRING_PRS_MUTING_PATTERN2_LIST,     0,  .uptr=NULL,         .defintarrayval=0,          TYPE_UINTARRAY,  0},  \
  {CONFIG_STRING_PRS_MUTING_BIT_REPETITION,     HELP_STRING_PRS_MUTING_BIT_REPETITION,    0,  .uptr=NULL,         .defuintval=0,              TYPE_UINT,       0}   \
}
// clang-format on

#define PRS_GNB_ID                                   0
#define NUM_PRS_RESOURCES                            1
#define PRS_RESOURCE_SET_PERIOD_LIST                 2
#define PRS_SYMBOL_START_LIST                        3
#define PRS_NUM_SYMBOLS_LIST                         4
#define PRS_NUM_RB                                   5
#define PRS_RB_OFFSET                                6
#define PRS_COMB_SIZE                                7
#define PRS_RE_OFFSET_LIST                           8
#define PRS_RESOURCE_OFFSET_LIST                     9
#define PRS_RESOURCE_REPETITION                      10
#define PRS_RESOURCE_TIME_GAP                        11
#define PRS_ID_LIST                                  12
#define PRS_MUTING_PATTERN1_LIST                     13
#define PRS_MUTING_PATTERN2_LIST                     14
#define PRS_MUTING_BIT_REPETITION                    15

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

#endif
