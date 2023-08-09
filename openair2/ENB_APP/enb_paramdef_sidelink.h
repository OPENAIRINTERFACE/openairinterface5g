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

/*! \file openair2/ENB_APP/enb_paramdef_sidelink.h
 * \brief definition of configuration parameters for sidelink eNodeB modules
 * \author Raymond KNOPP
 * \date 2018
 * \version 0.1
 * \company EURECOM France
 * \email: raymond.knopp@eurecom.fr
 * \note
 * \warning
 */

#ifndef ENB_PARAMDEF_SIDELINK_H_
#define ENB_PARAMDEF_SIDELINK_H_
#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"
#include <libconfig.h>

#define ENB_CONFIG_STRING_SL_PARAMETERS                                 "SLparameters"
//SIB18
#define ENB_CONFIG_STRING_RXPOOL_SC_CP_LEN                              "rxPool_sc_CP_Len"
#define ENB_CONFIG_STRING_RXPOOL_SC_PRIOD                               "rxPool_sc_Period"
#define ENB_CONFIG_STRING_RXPOOL_DATA_CP_LEN                            "rxPool_data_CP_Len"
#define ENB_CONFIG_STRING_RXPOOL_RC_PRB_NUM                             "rxPool_ResourceConfig_prb_Num"
#define ENB_CONFIG_STRING_RXPOOL_RC_PRB_START                           "rxPool_ResourceConfig_prb_Start"
#define ENB_CONFIG_STRING_RXPOOL_RC_PRB_END                             "rxPool_ResourceConfig_prb_End"
#define ENB_CONFIG_STRING_RXPOOL_RC_OFFSETIND_PRESENT                   "rxPool_ResourceConfig_offsetIndicator_present"
#define ENB_CONFIG_STRING_RXPOOL_RC_OFFSETIND_CHOICE                    "rxPool_ResourceConfig_offsetIndicator_choice"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_PRESENT                    "rxPool_ResourceConfig_subframeBitmap_present"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_BUF              "rxPool_ResourceConfig_subframeBitmap_choice_bs_buf"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_SIZE             "rxPool_ResourceConfig_subframeBitmap_choice_bs_size"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED  "rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused"
//SIB19 for DiscRxPool
#define ENB_CONFIG_STRING_DISCRXPOOL_CP_LEN                                 "discRxPool_cp_Len"
#define ENB_CONFIG_STRING_DISCRXPOOL_DISCPERIOD                             "discRxPool_discPeriod"
#define ENB_CONFIG_STRING_DISCRXPOOL_NUMRETX                                "discRxPool_numRetx"
#define ENB_CONFIG_STRING_DISCRXPOOL_NUMREPETITION                          "discRxPool_numRepetition"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_NUM                             "discRxPool_ResourceConfig_prb_Num"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_START                           "discRxPool_ResourceConfig_prb_Start"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_END                             "discRxPool_ResourceConfig_prb_End"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_OFFSETIND_PRESENT                   "discRxPool_ResourceConfig_offsetIndicator_present"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_OFFSETIND_CHOICE                    "discRxPool_ResourceConfig_offsetIndicator_choice"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_PRESENT                    "discRxPool_ResourceConfig_subframeBitmap_present"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_BUF              "discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_SIZE             "discRxPool_ResourceConfig_subframeBitmap_choice_bs_size"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED  "discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused"

//SIB19 for DiscRxPoolPS
#define ENB_CONFIG_STRING_DISCRXPOOLPS_CP_LEN                                 "DISCRXPOOLPS_cp_Len"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_DISCPERIOD                             "DISCRXPOOLPS_discPeriod"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_NUMRETX                                "DISCRXPOOLPS_numRetx"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_NUMREPETITION                          "DISCRXPOOLPS_numRepetition"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_NUM                             "DISCRXPOOLPS_ResourceConfig_prb_Num"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_START                           "DISCRXPOOLPS_ResourceConfig_prb_Start"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_END                             "DISCRXPOOLPS_ResourceConfig_prb_End"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_OFFSETIND_PRESENT                   "DISCRXPOOLPS_ResourceConfig_offsetIndicator_present"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_OFFSETIND_CHOICE                    "DISCRXPOOLPS_ResourceConfig_offsetIndicator_choice"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_PRESENT                    "DISCRXPOOLPS_ResourceConfig_subframeBitmap_present"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_BUF              "DISCRXPOOLPS_ResourceConfig_subframeBitmap_choice_bs_buf"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_SIZE             "DISCRXPOOLPS_ResourceConfig_subframeBitmap_choice_bs_size"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED  "DISCRXPOOLPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused"


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     component carriers configuration parameters                                                                                                     */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* init for checkedparam_t structure */

typedef struct ccparams_sidelink_s {
  int               sidelink_configured;
  //SIB18
  char             *rxPool_sc_CP_Len;
  char             *rxPool_sc_Period;
  char             *rxPool_data_CP_Len;
  libconfig_int     rxPool_ResourceConfig_prb_Num;
  libconfig_int     rxPool_ResourceConfig_prb_Start;
  libconfig_int     rxPool_ResourceConfig_prb_End;
  char             *rxPool_ResourceConfig_offsetIndicator_present;
  libconfig_int     rxPool_ResourceConfig_offsetIndicator_choice;
  char             *rxPool_ResourceConfig_subframeBitmap_present;
  char             *rxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //SIB19
  //For discRxPool
  char             *discRxPool_cp_Len;
  char             *discRxPool_discPeriod;
  libconfig_int     discRxPool_numRetx;
  libconfig_int     discRxPool_numRepetition;
  libconfig_int     discRxPool_ResourceConfig_prb_Num;
  libconfig_int     discRxPool_ResourceConfig_prb_Start;
  libconfig_int     discRxPool_ResourceConfig_prb_End;
  char             *discRxPool_ResourceConfig_offsetIndicator_present;
  libconfig_int     discRxPool_ResourceConfig_offsetIndicator_choice;
  char             *discRxPool_ResourceConfig_subframeBitmap_present;
  char             *discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //For discRxPoolPS
  char             *discRxPoolPS_cp_Len;
  char       *discRxPoolPS_discPeriod;
  libconfig_int     discRxPoolPS_numRetx;
  libconfig_int     discRxPoolPS_numRepetition;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Num;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Start;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_End;
  char       *discRxPoolPS_ResourceConfig_offsetIndicator_present;
  libconfig_int     discRxPoolPS_ResourceConfig_offsetIndicator_choice;
  char       *discRxPoolPS_ResourceConfig_subframeBitmap_present;
  char             *discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
} ccparams_sidelink_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     component carriers configuration parameters                                                                                                                   */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  checked_param */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define CCPARAMS_SIDELINK_DESC(SLparams) {   \
  {"sidelink_configured",                                                NULL,   0,   .iptr=&SLparams.sidelink_configured,                                               .defintval=0,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_RXPOOL_SC_CP_LEN,                                   NULL,   0,   .strptr=&SLparams.rxPool_sc_CP_Len,                                                .defstrval="normal",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_RXPOOL_SC_PRIOD,                                    NULL,   0,   .strptr=&SLparams.rxPool_sc_Period,                                                .defstrval="sf40",       TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_RXPOOL_DATA_CP_LEN,                                 NULL,   0,   .strptr=&SLparams.rxPool_data_CP_Len,                                              .defstrval="normal",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_PRB_NUM,                                  NULL,   0,   .iptr=&SLparams.rxPool_ResourceConfig_prb_Num,                                     .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_PRB_START,                                NULL,   0,   .iptr=&SLparams.rxPool_ResourceConfig_prb_Start,                                   .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_PRB_END,                                  NULL,   0,   .iptr=&SLparams.rxPool_ResourceConfig_prb_End,                                     .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_OFFSETIND_PRESENT,                        NULL,   0,   .strptr=&SLparams.rxPool_ResourceConfig_offsetIndicator_present,                   .defstrval="prNothing",  TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_OFFSETIND_CHOICE,                         NULL,   0,   .iptr=&SLparams.rxPool_ResourceConfig_offsetIndicator_choice,                      .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_PRESENT,                         NULL,   0,   .strptr=&SLparams.rxPool_ResourceConfig_subframeBitmap_present,                    .defstrval="prNothing",  TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_BUF,                   NULL,   0,   .strptr=&SLparams.rxPool_ResourceConfig_subframeBitmap_choice_bs_buf,              .defstrval="001001",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_SIZE,                  NULL,   0,   .iptr=&SLparams.rxPool_ResourceConfig_subframeBitmap_choice_bs_size,               .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED,       NULL,   0,   .iptr=&SLparams.rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused,        .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_CP_LEN,                                  NULL,   0,   .strptr=&SLparams.discRxPool_cp_Len,                                               .defstrval="normal",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_DISCPERIOD,                              NULL,   0,   .strptr=&SLparams.discRxPool_discPeriod,                                           .defstrval="rf32",       TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_NUMRETX,                                 NULL,   0,   .iptr=&SLparams.discRxPool_numRetx,                                                .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_NUMREPETITION,                           NULL,   0,   .iptr=&SLparams.discRxPool_numRepetition,                                          .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_NUM,                              NULL,   0,   .iptr=&SLparams.discRxPool_ResourceConfig_prb_Num,                                 .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_START,                            NULL,   0,   .iptr=&SLparams.discRxPool_ResourceConfig_prb_Start,                               .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_END,                              NULL,   0,   .iptr=&SLparams.discRxPool_ResourceConfig_prb_End,                                 .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_OFFSETIND_PRESENT,                    NULL,   0,   .strptr=&SLparams.discRxPool_ResourceConfig_offsetIndicator_present,               .defstrval="prNothing",  TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_OFFSETIND_CHOICE,                     NULL,   0,   .iptr=&SLparams.discRxPool_ResourceConfig_offsetIndicator_choice,                  .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_PRESENT,                     NULL,   0,   .strptr=&SLparams.discRxPool_ResourceConfig_subframeBitmap_present,                .defstrval="prNothing",  TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_BUF,               NULL,   0,   .strptr=&SLparams.discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf,          .defstrval="001001",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_SIZE,              NULL,   0,   .iptr=&SLparams.discRxPool_ResourceConfig_subframeBitmap_choice_bs_size,           .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED,   NULL,   0,   .iptr=&SLparams.discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused,    .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_CP_LEN,                                NULL,   0,   .strptr=&SLparams.discRxPoolPS_cp_Len,                                             .defstrval="normal",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_DISCPERIOD,                            NULL,   0,   .strptr=&SLparams.discRxPoolPS_discPeriod,                                         .defstrval="rf32",       TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_NUMRETX,                               NULL,   0,   .iptr=&SLparams.discRxPoolPS_numRetx,                                              .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_NUMREPETITION,                         NULL,   0,   .iptr=&SLparams.discRxPoolPS_numRepetition,                                        .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_NUM,                            NULL,   0,   .iptr=&SLparams.discRxPoolPS_ResourceConfig_prb_Num,                               .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_START,                          NULL,   0,   .iptr=&SLparams.discRxPoolPS_ResourceConfig_prb_Start,                             .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_END,                            NULL,   0,   .iptr=&SLparams.discRxPoolPS_ResourceConfig_prb_End,                               .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_OFFSETIND_PRESENT,                  NULL,   0,   .strptr=&SLparams.discRxPoolPS_ResourceConfig_offsetIndicator_present,             .defstrval="prNothing",  TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_OFFSETIND_CHOICE,                   NULL,   0,   .iptr=&SLparams.discRxPoolPS_ResourceConfig_offsetIndicator_choice,                .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_PRESENT,                   NULL,   0,   .strptr=&SLparams.discRxPoolPS_ResourceConfig_subframeBitmap_present,              .defstrval="prNothing",  TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_BUF,             NULL,   0,   .strptr=&SLparams.discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf,        .defstrval="001001",     TYPE_STRING,  0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_SIZE,            NULL,   0,   .iptr=&SLparams.discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size,         .defintval=1,            TYPE_UINT,    0}, \
  {ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED, NULL,   0,   .iptr=&SLparams.discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused,  .defintval=1,            TYPE_UINT,    0}, \
}
// clang-format on
#endif
