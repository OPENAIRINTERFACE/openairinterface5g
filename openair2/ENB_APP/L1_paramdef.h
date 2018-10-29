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

/*! \file openair2/ENB_APP/L1_paramdef.f
 * \brief definition of configuration parameters for all eNodeB modules 
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


/* L1 configuration parameters names   */
#define CONFIG_STRING_L1_CC                                "num_cc"
#define CONFIG_STRING_L1_LOCAL_N_IF_NAME                   "local_n_if_name"
#define CONFIG_STRING_L1_LOCAL_N_ADDRESS                   "local_n_address"
#define CONFIG_STRING_L1_REMOTE_N_ADDRESS                  "remote_n_address"
#define CONFIG_STRING_L1_LOCAL_N_PORTC                     "local_n_portc"
#define CONFIG_STRING_L1_REMOTE_N_PORTC                    "remote_n_portc"
#define CONFIG_STRING_L1_LOCAL_N_PORTD                     "local_n_portd"
#define CONFIG_STRING_L1_REMOTE_N_PORTD                    "remote_n_portd"
#define CONFIG_STRING_L1_TRANSPORT_N_PREFERENCE            "tr_n_preference"

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            L1 configuration parameters                                                                             */
/*   optname                                         helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define L1PARAMS_DESC { \
{CONFIG_STRING_L1_CC,                                NULL,      0,         uptr:NULL,           defintval:1,               TYPE_UINT,     0},         \
{CONFIG_STRING_L1_TRANSPORT_N_PREFERENCE,            NULL,      0,         strptr:NULL,         defstrval:"local_mac",     TYPE_STRING,   0},         \
{CONFIG_STRING_L1_LOCAL_N_IF_NAME,                   NULL,      0,         strptr:NULL,         defstrval:"lo",            TYPE_STRING,   0},         \
{CONFIG_STRING_L1_LOCAL_N_ADDRESS,                   NULL,      0,         strptr:NULL,         defstrval:"127.0.0.1",     TYPE_STRING,   0},         \
{CONFIG_STRING_L1_REMOTE_N_ADDRESS,                  NULL,      0,         strptr:NULL,         defstrval:"127.0.0.2",     TYPE_STRING,   0},         \
{CONFIG_STRING_L1_LOCAL_N_PORTC,                     NULL,      0,         uptr:NULL,           defintval:50030,           TYPE_UINT,     0},         \
{CONFIG_STRING_L1_REMOTE_N_PORTC,                    NULL,      0,         uptr:NULL,           defintval:50030,           TYPE_UINT,     0},         \
{CONFIG_STRING_L1_LOCAL_N_PORTD,                     NULL,      0,         uptr:NULL,           defintval:50031,           TYPE_UINT,     0},         \
{CONFIG_STRING_L1_REMOTE_N_PORTD,                    NULL,      0,         uptr:NULL,           defintval:50031,           TYPE_UINT,     0},         \
}
#define L1_CC_IDX                                          0
#define L1_TRANSPORT_N_PREFERENCE_IDX                      1
#define L1_LOCAL_N_IF_NAME_IDX                             2
#define L1_LOCAL_N_ADDRESS_IDX                             3
#define L1_REMOTE_N_ADDRESS_IDX                            4
#define L1_LOCAL_N_PORTC_IDX                               5
#define L1_REMOTE_N_PORTC_IDX                              6
#define L1_LOCAL_N_PORTD_IDX                               7
#define L1_REMOTE_N_PORTD_IDX                              8

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
