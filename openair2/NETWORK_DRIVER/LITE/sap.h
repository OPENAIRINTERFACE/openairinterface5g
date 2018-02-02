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

#ifndef OAI_SAP_H
#define OAI_SAP_H
typedef unsigned short     OaiNwDrvRadioBearerId_t;
typedef unsigned int       OaiNwDrvSapId_t;              // Id of the QoS SAP to use
typedef unsigned short     OaiNwDrvQoSTrafficClass_t;    // QoS traffic class requested
typedef unsigned int       OaiNwDrvLocalConnectionRef_t; // local identifier
typedef unsigned short     OaiNwDrvCellID_t;             // ID of the cell for connection
typedef unsigned short     OaiNwDrvNumRGsMeas_t;         // number of RGs that could be measured
typedef unsigned int       OaiNwDrvSigLevel_t;           // Signal level measured

#define OAI_NW_DRV_SAPI_CX_MAX                 2
#define OAI_NW_DRV_MAX_MEASURE_NB              5
#define OAI_NW_DRV_PRIMITIVE_MAX_LENGTH        180  // maximum length of a NAS primitive
#define OAI_NW_DRV_SAPI_MAX                    4
#define OAI_NW_DRV_RAB_INPUT_SAPI     2
#define OAI_NW_DRV_RAB_OUTPUT_SAPI      3
#define OAI_NW_DRV_MAX_RABS     8 * 64 //NB_RAB_MAX * MAX_MOBILES_PER_RG  //27   // = MAXURAB
#define OAI_NW_DRV_LIST_RB_MAX 32
#endif
