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


/* PRS configuration parameters names   */
#define CONFIG_STRING_PRS_RESOURCE_SET_PERIOD0              "PRSResourceSetPeriod0"
#define CONFIG_STRING_PRS_RESOURCE_SET_PERIOD1              "PRSResourceSetPeriod1"
#define CONFIG_STRING_PRS_SYMBOL_START                      "SymbolStart"
#define CONFIG_STRING_PRS_NUM_SYMBOLS                       "NumPRSSymbols"
#define CONFIG_STRING_PRS_NUM_RB                            "NumRB"
#define CONFIG_STRING_PRS_RB_OFFSET                         "RBOffset"
#define CONFIG_STRING_PRS_COMB_SIZE                         "CombSize"
#define CONFIG_STRING_PRS_RE_OFFSET                         "REOffset"
#define CONFIG_STRING_PRS_RESOURCE_OFFSET                   "PRSResourceOffset"
#define CONFIG_STRING_PRS_RESOURCE_REPETITION               "PRSResourceRepetition"
#define CONFIG_STRING_PRS_RESOURCE_TIME_GAP                 "PRSResourceTimeGap"
#define CONFIG_STRING_PRS_ID                                "NPRS_ID"


/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            PRS configuration parameters                                                                             */
/*   optname                                         helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define PRS_PARAMS_DESC { \
{CONFIG_STRING_PRS_RESOURCE_SET_PERIOD0,           NULL,      0,         uptr:NULL,           defintval:40,              TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_RESOURCE_SET_PERIOD1,           NULL,      0,         uptr:NULL,           defintval:0,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_SYMBOL_START,                   NULL,      0,         uptr:NULL,           defintval:7,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_NUM_SYMBOLS,                    NULL,      0,         uptr:NULL,           defintval:4,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_NUM_RB,                         NULL,      0,         uptr:NULL,           defintval:106,             TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_RB_OFFSET,                      NULL,      0,         uptr:NULL,           defintval:0,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_COMB_SIZE,                      NULL,      0,         uptr:NULL,           defintval:4,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_RE_OFFSET,                      NULL,      0,         uptr:NULL,           defintval:0,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_RESOURCE_OFFSET,                NULL,      0,         uptr:NULL,           defintval:0,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_RESOURCE_REPETITION,            NULL,      0,         uptr:NULL,           defintval:1,               TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_RESOURCE_TIME_GAP,              NULL,      0,         uptr:NULL,           defuintval:1,              TYPE_UINT,     0},         \
{CONFIG_STRING_PRS_ID,                             NULL,      0,         uptr:NULL,           defintval:0,               TYPE_UINT,     0}          \
}

#define PRS_RESOURCE_SET_PERIOD0                     0
#define PRS_RESOURCE_SET_PERIOD1                     1
#define PRS_SYMBOL_START                             2
#define PRS_NUM_SYMBOLS                              3
#define PRS_NUM_RB                                   4
#define PRS_RB_OFFSET                                5
#define PRS_COMB_SIZE                                6
#define PRS_RE_OFFSET                                7
#define PRS_RESOURCE_OFFSET                          8
#define PRS_RESOURCE_REPETITION                      9
#define PRS_RESOURCE_TIME_GAP                        10
#define PRS_ID                                       11

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#endif
