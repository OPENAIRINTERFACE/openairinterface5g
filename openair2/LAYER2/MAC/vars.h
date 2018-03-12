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

/*! \file vars.h
* \brief mac vars
* \author  Navid Nikaein and Raymond Knopp
* \date 2010 - 2014
* \version 1.0
* \email navid.nikaein@eurecom.fr
* @ingroup _mac

*/


#ifndef __MAC_VARS_H__
#define __MAC_VARS_H__
#include "PHY/defs.h"
#include "defs.h"
#include "COMMON/mac_rrc_primitives.h"

const uint32_t BSR_TABLE[BSR_TABLE_SIZE] =
    { 0, 10, 12, 14, 17, 19, 22, 26, 31, 36, 42, 49, 57, 67, 78, 91,
    105, 125, 146, 171, 200, 234, 274, 321, 376, 440, 515, 603, 706, 826,
	967, 1132,
    1326, 1552, 1817, 2127, 2490, 2915, 3413, 3995, 4677, 5467, 6411, 7505,
	8787, 10287, 12043, 14099,
    16507, 19325, 22624, 26487, 31009, 36304, 42502, 49759, 58255, 68201,
	79846, 93479, 109439, 128125, 150000, 300000
};

// extended bsr table--currently not used                                                                                 
const uint32_t Extended_BSR_TABLE[BSR_TABLE_SIZE] =
    { 0, 10, 13, 16, 19, 23, 29, 35, 43, 53, 65, 80, 98, 120, 147,
    181, 223, 274, 337, 414, 509, 625, 769, 945, 1162, 1429,
    1757, 2161, 2657, 3267, 4017, 4940, 6074, 7469, 9185,
    11294, 13888, 17077, 20999, 25822, 31752, 39045, 48012,
    59039, 72598, 89272, 109774, 134986, 165989, 204111,
    250990, 308634, 379519, 466683, 573866, 705666, 867737,
    1067031, 1312097, 1613447, 1984009, 2439678, 3000000,
    6000000
};

//#define MAX_SIZE_OF_AGG3   576 
//#define MAX_SIZE_OF_AGG2   288
//#define MAX_SIZE_OF_AGG1   144
//#define MAX_SIZE_OF_AGG0   72

/*
 * If the CQI is low, then scheduler will use a higher aggregation level and lower aggregation level otherwise
 * this is also dependent to transmission mode, where an offset could be defined
 */
// the follwoing three tables are calibrated for TXMODE 1 and 2
const uint8_t cqi2fmt0_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE] = {
    {3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},	// 1.4_DCI0_CRC_Size= 37 bits
    //{3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0}, // 5_DCI0_CRC_SIZE = 41
    {3, 3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},	// 5_DCI0_CRC_SIZE = 41
    {3, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0},	// 10_DCI0_CRC_SIZE = 43
    {3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0}	// 20_DCI0_CRC_SIZE = 44
};

const uint8_t cqi2fmt1x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE] = {
    {3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},	// 1.4_DCI0_CRC_Size < 38 bits
    {3, 3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},	// 5_DCI0_CRC_SIZE  < 43
    {3, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0},	// 10_DCI0_CRC_SIZE  < 47
    {3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0}	// 20_DCI0_CRC_SIZE  < 55
};

const uint8_t cqi2fmt2x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE] = {
    {3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},	// 1.4_DCI0_CRC_Size= 47 bits
    {3, 3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},	// 5_DCI0_CRC_SIZE = 55
    {3, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0},	// 10_DCI0_CRC_SIZE = 59
    {3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0}	// 20_DCI0_CRC_SIZE = 64
};

//uint32_t EBSR_Level[63]={0,10,13,16,19,23,29,35,43,53,65,80,98,120,147,181};


uint32_t RRC_CONNECTION_FLAG;

UE_MAC_INST *UE_mac_inst;	//[NB_MODULE_MAX];
MAC_RLC_XFACE *Mac_rlc_xface;

/// Primary component carrier index of eNB
int pCC_id[NUMBER_OF_eNB_MAX];



eNB_ULSCH_INFO eNB_ulsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][NUMBER_OF_UE_MAX];	// eNBxUE = 8x8
eNB_DLSCH_INFO eNB_dlsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][NUMBER_OF_UE_MAX];	// eNBxUE = 8x8


#ifdef OPENAIR2
unsigned char NB_eNB_INST = 0;
unsigned char NB_UE_INST = 0;
unsigned char NB_RN_INST = 0;
unsigned char NB_INST = 0;
#endif


DCI0_5MHz_TDD_1_6_t UL_alloc_pdu;

DCI1A_5MHz_TDD_1_6_t DLSCH_alloc_pdu1A;
DCI1A_5MHz_TDD_1_6_t RA_alloc_pdu;
DCI1A_5MHz_TDD_1_6_t BCCH_alloc_pdu;

DCI1A_5MHz_TDD_1_6_t CCCH_alloc_pdu;
DCI1_5MHz_TDD_t DLSCH_alloc_pdu;

#if defined(Rel10) || defined(Rel14)
DCI1C_5MHz_t MCCH_alloc_pdu;
#endif

DCI0_5MHz_FDD_t UL_alloc_pdu_fdd;

DCI1A_5MHz_FDD_t DLSCH_alloc_pdu1A_fdd;
DCI1A_5MHz_FDD_t RA_alloc_pdu_fdd;
DCI1A_5MHz_FDD_t BCCH_alloc_pdu_fdd;

DCI1A_5MHz_FDD_t CCCH_alloc_pdu_fdd;
DCI1_5MHz_FDD_t DLSCH_alloc_pdu_fdd;

DCI2_5MHz_2A_TDD_t DLSCH_alloc_pdu1;
DCI2_5MHz_2A_TDD_t DLSCH_alloc_pdu2;

DCI1E_5MHz_2A_M10PRB_TDD_t DLSCH_alloc_pdu1E;

#endif
