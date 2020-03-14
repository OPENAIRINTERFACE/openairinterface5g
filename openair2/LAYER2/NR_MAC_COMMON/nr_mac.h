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

/* \file       nr_mac.h
 * \brief      common MAC data structures, constant, and function prototype
 * \author     R. Knopp, K.H. HSU, G. Casati
 * \date       2019
 * \version    0.1
 * \company    Eurecom / NTUST / Fraunhofer IIS 
 * \email:     knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr, guido.casati@iis.fraunhofer.de 
 * \note
 * \warning
 */

#ifndef __LAYER2_NR_MAC_H__
#define __LAYER2_NR_MAC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NR_BCCH_DL_SCH 3 // SI

#define NR_BCCH_BCH 5    // MIB

//  For both DL/UL-SCH
//  Except:
//   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
//   - UL/DL-SCH: padding
//   - UL-SCH:    MSG3 48-bits
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|F|   LCID    |
//  |       L       |
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|F|   LCID    |
//  |       L       |
//  |       L       |

//  For both DL/UL-SCH
//  For:
//   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
//   - UL/DL-SCH: padding, for single/multiple 1-oct padding CE(s)
//   - UL-SCH:    MSG3 48-bits
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|R|   LCID    |
//  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
//  L: The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
//  F: lenght of L is 0:8 or 1:16 bits wide
//  R: Reserved bit, set to zero.

typedef struct {
    uint8_t LCID:6;     // octet 1 [5:0]
    uint8_t F:1;        // octet 1 [6]
    uint8_t R:1;        // octet 1 [7]
    uint8_t L:8;        // octet 2 [7:0]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_SHORT;

typedef struct {
    uint8_t LCID:6;     // octet 1 [5:0]
    uint8_t F:1;        // octet 1 [6]
    uint8_t R:1;        // octet 1 [7]
    uint8_t L1:8;       // octet 2 [7:0]
    uint8_t L2:8;       // octet 3 [7:0]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_LONG;

typedef struct {
    uint8_t LCID:6;     // octet 1 [5:0]
    uint8_t R:2;        // octet 1 [7:6]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_FIXED;

// 38.321 ch. 6.1.3.4
typedef struct {
    uint8_t TA_COMMAND:6;   // octet 1 [5:0]
    uint8_t TAGID:2;        // octet 1 [7:6]
} __attribute__ ((__packed__)) NR_MAC_CE_TA;

//  38.321 ch6.2.1, 38.331
#define DL_SCH_LCID_CCCH                           0x00
#define DL_SCH_LCID_DCCH                           0x01
#define DL_SCH_LCID_DCCH1                          0x02
#define DL_SCH_LCID_DTCH                           0x03
#define DL_SCH_LCID_RECOMMENDED_BITRATE            0x2F
#define DL_SCH_LCID_SP_ZP_CSI_RS_RES_SET_ACT       0x30
#define DL_SCH_LCID_PUCCH_SPATIAL_RELATION_ACT     0x31
#define DL_SCH_LCID_SP_SRS_ACTIVATION              0x32
#define DL_SCH_LCID_SP_CSI_REP_PUCCH_ACT           0x33
#define DL_SCH_LCID_TCI_STATE_IND_UE_SPEC_PDCCH    0x34
#define DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH    0x35
#define DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL 0x36
#define DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT   0X37 
#define DL_SCH_LCID_DUPLICATION_ACT                0X38
#define DL_SCH_LCID_SCell_ACT_4_OCT                0X39
#define DL_SCH_LCID_SCell_ACT_1_OCT                0X3A
#define DL_SCH_LCID_L_DRX                          0x3B
#define DL_SCH_LCID_DRX                            0x3C
#define DL_SCH_LCID_TA_COMMAND                     0x3D
#define DL_SCH_LCID_CON_RES_ID                     0x3E
#define DL_SCH_LCID_PADDING                        0x3F

#define UL_SCH_LCID_CCCH                           0x00
#define UL_SCH_LCID_SRB1                           0x01
#define UL_SCH_LCID_SRB2                           0x02
#define UL_SCH_LCID_DTCH						   0x03
#define UL_SCH_LCID_SRB3                           0x04
#define UL_SCH_LCID_CCCH_MSG3                      0x21
#define UL_SCH_LCID_RECOMMENDED_BITRATE_QUERY      0x35
#define UL_SCH_LCID_MULTI_ENTRY_PHR_4_OCT          0x36
#define UL_SCH_LCID_CONFIGURED_GRANT_CONFIRMATION  0x37
#define UL_SCH_LCID_MULTI_ENTRY_PHR_1_OCT          0x38
#define UL_SCH_LCID_SINGLE_ENTRY_PHR               0x39
#define UL_SCH_LCID_C_RNTI                         0x3A
#define UL_SCH_LCID_S_TRUNCATED_BSR                0x3B
#define UL_SCH_LCID_L_TRUNCATED_BSR                0x3C
#define UL_SCH_LCID_S_BSR                          0x3D
#define UL_SCH_LCID_L_BSR                          0x3E
#define UL_SCH_LCID_PADDING                        0x3F

#define NR_MAX_NUM_LCID							   32
#define NR_MAX_NUM_LCGID							8
#define MAX_RLC_SDU_SUBHEADER_SIZE					3


#endif /*__LAYER2_MAC_H__ */

