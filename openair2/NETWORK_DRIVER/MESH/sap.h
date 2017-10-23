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

/***************************************************************************
                          graal_sap.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : michelle.wetterwald@eurecom.fr
                           yan.moret@eurecom.fr
 ***************************************************************************

 ***************************************************************************/

#ifndef _NAS_SAP_H
#define _NAS_SAP_H


// RT-FIFO identifiers ** must be identical to Access Stratum as_sap.h and rrc_sap.h

#define RRC_DEVICE_GC          RRC_SAPI_GCSAP
#define RRC_DEVICE_NT          RRC_SAPI_NTSAP
#define RRC_DEVICE_DC_INPUT0   RRC_SAPI_DCSAP_IN
#define RRC_DEVICE_DC_OUTPUT0  RRC_SAPI_DCSAP_OUT


//FIFO indexes in control blocks

#define NAS_DC_INPUT_SAPI   0
#define NAS_DC_OUTPUT_SAPI  1
#define NAS_SAPI_CX_MAX     2

#define NAS_GC_SAPI         0
#define NAS_NT_SAPI         1

#define NAS_RAB_INPUT_SAPI      2
#define NAS_RAB_OUTPUT_SAPI     3


#define NAS_SAPI_MAX       4




#endif



