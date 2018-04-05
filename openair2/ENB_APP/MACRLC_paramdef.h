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

/*! \file openair2/ENB_APP/MACRLC_paramdef.f
 * \brief definition of configuration parameters for all eNodeB modules 
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */



/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/* MACRLC configuration parameters names   */
#define CONFIG_STRING_MACRLC_CC                            "num_cc"
#define CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE        "tr_n_preference"
#define CONFIG_STRING_MACRLC_LOCAL_N_IF_NAME               "local_n_if_name"
#define CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS               "local_n_address"
#define CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS              "remote_n_address"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTC                 "local_n_portc"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTC                "remote_n_portc"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTD                 "local_n_portd"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTD                "remote_n_portd"
#define CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define CONFIG_STRING_MACRLC_LOCAL_S_IF_NAME               "local_s_if_name"
#define CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS               "local_s_address"
#define CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS              "remote_s_address"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTC                 "local_s_portc"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTC                "remote_s_portc"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTD                 "local_s_portd"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTD                "remote_s_portd"


/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            MacRLC  configuration parameters                                                                           */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define MACRLCPARAMS_DESC { \
{CONFIG_STRING_MACRLC_CC,                                NULL,     0,          uptr:NULL,           defintval:50011,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE,            NULL,     0,          strptr:NULL,         defstrval:"local_L1",      TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_LOCAL_N_IF_NAME,                   NULL,     0,          strptr:NULL,         defstrval:"lo",            TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS,                   NULL,     0,          strptr:NULL,         defstrval:"127.0.0.1",     TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS,                  NULL,     0,          uptr:NULL,           defstrval:"127.0.0.2",     TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_LOCAL_N_PORTC,                     NULL,     0,          uptr:NULL,           defintval:50010,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_REMOTE_N_PORTC,                    NULL,     0,          uptr:NULL,           defintval:50010,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_LOCAL_N_PORTD,                     NULL,     0,          uptr:NULL,           defintval:50011,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_REMOTE_N_PORTD,                    NULL,     0,          uptr:NULL,           defintval:50011,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE,            NULL,     0,          strptr:NULL,         defstrval:"local_RRC",     TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_LOCAL_S_IF_NAME,                   NULL,     0,          strptr:NULL,         defstrval:"lo",            TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS,                   NULL,     0,          uptr:NULL,           defstrval:"127.0.0.1",     TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS,                  NULL,     0,          uptr:NULL,           defstrval:"127.0.0.2",     TYPE_STRING,   0},        \
{CONFIG_STRING_MACRLC_LOCAL_S_PORTC,                     NULL,     0,          uptr:NULL,           defintval:50020,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_REMOTE_S_PORTC,                    NULL,     0,          uptr:NULL,           defintval:50020,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_LOCAL_S_PORTD,                     NULL,     0,          uptr:NULL,           defintval:50021,           TYPE_UINT,     0},        \
{CONFIG_STRING_MACRLC_REMOTE_S_PORTD,                    NULL,     0,          uptr:NULL,           defintval:50021,           TYPE_UINT,     0},        \
}
#define MACRLC_CC_IDX                                          0
#define MACRLC_TRANSPORT_N_PREFERENCE_IDX                      1
#define MACRLC_LOCAL_N_IF_NAME_IDX                             2
#define MACRLC_LOCAL_N_ADDRESS_IDX                             3
#define MACRLC_REMOTE_N_ADDRESS_IDX                            4
#define MACRLC_LOCAL_N_PORTC_IDX                               5
#define MACRLC_REMOTE_N_PORTC_IDX                              6
#define MACRLC_LOCAL_N_PORTD_IDX                               7
#define MACRLC_REMOTE_N_PORTD_IDX                              8
#define MACRLC_TRANSPORT_S_PREFERENCE_IDX                      9
#define MACRLC_LOCAL_S_IF_NAME_IDX                             10
#define MACRLC_LOCAL_S_ADDRESS_IDX                             11
#define MACRLC_REMOTE_S_ADDRESS_IDX                            12
#define MACRLC_LOCAL_S_PORTC_IDX                               13
#define MACRLC_REMOTE_S_PORTC_IDX                              14
#define MACRLC_LOCAL_S_PORTD_IDX                               15
#define MACRLC_REMOTE_S_PORTD_IDX                              16
/*---------------------------------------------------------------------------------------------------------------------------------------------------------*/
