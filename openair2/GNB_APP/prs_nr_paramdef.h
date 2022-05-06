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

/*! \file openair2/GNB_APP/prs_nr_paramdef.f
 * \brief definition of configuration parameters for PRS 
 * \author
 * \date 2022
 * \version 0.1
 * \company EURECOM
 * \email:
 * \note
 * \warning
 */

#ifndef __GNB_APP_PRS_NR_PARAMDEF__H__
#define __GNB_APP_PRS_NR_PARAMDEF__H__

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* PRS configuration section names   */
#define CONFIG_STRING_PRS_LIST                              "PRSs"
#define CONFIG_STRING_PRS_CONFIG                            "prs_config"
#define CONFIG_STRING_MQTT_CONFIG                           "mqtt_config"


/* Global parameters */
#define CONFIG_STRING_ACTIVE_GNBs                         "Active_gNBs"
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            PRS configuration parameters                                                                             */
/*   optname                                         helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define PRS_GLOBAL_PARAMS_DESC { \
{CONFIG_STRING_ACTIVE_GNBs,                          NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,     0}          \
}

#define PRS_ACTIVE_GNBS_IDX                              0
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

/* PRS configuration parameters names   */
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


/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            PRS configuration parameters                                                                             */
/*   optname                                         helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define PRS_PARAMS_DESC { \
{CONFIG_STRING_GNB_ID,                             NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_NUM_PRS_RESOURCES,                  NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_PRS_RESOURCE_SET_PERIOD_LIST,       NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_SYMBOL_START_LIST,              NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_NUM_SYMBOLS_LIST,               NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_NUM_RB,                         NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_PRS_RB_OFFSET,                      NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_PRS_COMB_SIZE,                      NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_PRS_RE_OFFSET_LIST,                 NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_RESOURCE_OFFSET_LIST,           NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_RESOURCE_REPETITION,            NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_PRS_RESOURCE_TIME_GAP,              NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0},         \
{CONFIG_STRING_PRS_ID_LIST,                        NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_MUTING_PATTERN1_LIST,           NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_MUTING_PATTERN2_LIST,           NULL,      0,         uptr:NULL,           defintarrayval:0,           TYPE_UINTARRAY,  0},         \
{CONFIG_STRING_PRS_MUTING_BIT_REPETITION,          NULL,      0,         uptr:NULL,           defuintval:0,               TYPE_UINT,       0}          \
}

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
