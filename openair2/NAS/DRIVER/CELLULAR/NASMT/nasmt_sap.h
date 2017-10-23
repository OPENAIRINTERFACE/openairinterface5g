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

/*! \file nasmt_sap.h
* \brief SAP constants for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef _NASMTD_SAP_H
#define _NASMTD_SAP_H

// RT-FIFO identifiers ** must be identical to Access Stratum as_sap.h and rrc_sap.h
#define RRC_DEVICE_GC          RRC_SAPI_UE_GCSAP
#define RRC_DEVICE_NT          RRC_SAPI_UE_NTSAP
#define RRC_DEVICE_DC_INPUT0   RRC_SAPI_UE_DCSAP_IN
#define RRC_DEVICE_DC_OUTPUT0  RRC_SAPI_UE_DCSAP_OUT

//#define QOS_DEVICE_CONVERSATIONAL_INPUT  QOS_SAPI_CONVERSATIONAL_INPUT_MT
//#define QOS_DEVICE_CONVERSATIONAL_OUTPUT QOS_SAPI_CONVERSATIONAL_OUTPUT_MT

#define PDCP2PDCP_USE_RT_FIFO 21
#define NAS2PDCP_FIFO 22

//FIFO indexes in control blocks
#define NAS_DC_INPUT_SAPI    0
#define NAS_DC_OUTPUT_SAPI   1
#define NAS_SAPI_CX_MAX      2

#define NAS_GC_SAPI         0
#define NAS_NT_SAPI         1
#define NAS_DRB_INPUT_SAPI  2  //NAS_CO_INPUT_SAPI
#define NAS_DRB_OUTPUT_SAPI 3  //NAS_CO_OUTPUT_SAPI
#define NAS_SAPI_MAX        4

//#define NAS_QOS_CONVERSATIONAL UMTS_TRAFFIC_CONVERSATIONAL
//

/* Defined in RRC
#define RRC_NAS_GC_IN   0
#define RRC_NAS_GC_OUT  1
#define RRC_NAS_NT_IN   2
#define RRC_NAS_NT_OUT  3
#define RRC_NAS_DC0_IN  4
#define RRC_NAS_DC0_OUT 5
#define RRC_NAS_DC1_IN  6
#define RRC_NAS_DC1_OUT 7
#define RRC_NAS_DC2_IN  8
#define RRC_NAS_DC2_OUT 9
*/
#endif



