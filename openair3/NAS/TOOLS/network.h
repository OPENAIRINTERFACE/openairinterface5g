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

/*****************************************************************************
Source    network.h

Version   0.1

Date    2013/03/26

Product   USIM data generator

Subsystem PLMN network operators

Author    Frederic Maurel

Description Defines a list of PLMN network operators

*****************************************************************************/
#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "commonDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/
/*
 * PLMN network operator record index
 */
#define TEST1 0
#define SFR1  1
#define SFR2  2
#define SFR3  3
#define OAI_LTEBOX 4
#define TM1   5
#define FCT1  6
#define VDF1  7
#define VDF2  8
#define VDF3  9
#define VDF4  10
#define VDF5  11


#define SELECTED_PLMN TEST1 //SFR1

#define TEST_PLMN {0,0,0x0f,1,1,0}  // 00101
#define SFR_PLMN_1  {0,2,0x0f,8,0,1}  // 20810
#define SFR_PLMN_2  {0,2,0x0f,8,1,1}  // 20811
#define SFR_PLMN_3  {0,2,0x0f,8,3,1}  // 20813
#define OAI_LTEBOX_PLMN  {0,2,0x0f,8,3,9}  //20893
#define TM_PLMN_1   {1,3,0,0,8,2}       // 310280
#define FCT_PLMN_1  {1,3,8,0,2,0}       // 310028
#define VDF_PLMN_1  {2,2,0x0f,2,0,1}  // 22210
#define VDF_PLMN_2  {1,2,0x0f,4,0x0f,1} // 2141
#define VDF_PLMN_3  {1,2,0x0f,4,0x0f,6} // 2146
#define VDF_PLMN_4  {6,2,0x0f,2,0x0f,2} // 2622
#define VDF_PLMN_5  {6,2,0x0f,2,0x0f,4} // 2624


/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * PLMN network operator record
 */
typedef struct {
  unsigned int num;
  plmn_t plmn;
  char fullname[NET_FORMAT_LONG_SIZE + 1];
  char shortname[NET_FORMAT_SHORT_SIZE + 1];
  tac_t tac_start;
  tac_t tac_end;
} network_record_t;


/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/*
 * The list of PLMN network operator records
 */
network_record_t network_records[] = {
  {00101, TEST_PLMN, "Test network",     "OAI4G",     0x0001, 0xfffd},
  {20810, SFR_PLMN_1, "SFR France",      "SFR",       0x0001, 0xfffd},
  {20811, SFR_PLMN_2, "SFR France",      "SFR",       0x0001, 0xfffd},
  {20813, SFR_PLMN_3, "SFR France",      "SFR",       0x0001, 0xfffd},
  {20893, OAI_LTEBOX_PLMN, "OAI LTEBOX",   "OAIALU",  0x0001, 0xfffd},
  {310280,TM_PLMN_1,  "T-Mobile USA",    "T-Mobile",  0x0001, 0xfffd},
  {310028,FCT_PLMN_1, "FICTITIOUS USA",  "FICTITIO",  0x0001, 0xfffd},
  {22210, VDF_PLMN_1, "Vodafone Italia", "VODAFONE",  0x0001, 0xfffd},
  {2141,  VDF_PLMN_2, "Vodafone Spain",  "VODAFONE",  0x0001, 0xfffd},
  {2146,  VDF_PLMN_3, "Vodafone Spain",  "VODAFONE",  0x0001, 0xfffd},
  {2622,  VDF_PLMN_4, "Vodafone Germ",   "VODAFONE",  0x0001, 0xfffd},
  {2624,  VDF_PLMN_5, "Vodafone Germ",   "VODAFONE",  0x0001, 0xfffd},
};

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __NETWORK_H__*/
