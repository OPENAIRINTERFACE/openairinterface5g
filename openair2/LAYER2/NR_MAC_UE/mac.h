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

/* \file mac.h
 * \brief MAC data structures, constant, and function prototype
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __LAYER2_NR_UE_MAC_DEFS_H__
#define __LAYER2_NR_UE_MAC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NR_BCCH_DL_SCH 3            // SI

#define NR_BCCH_BCH 5           // MIB

/*!\brief UE layer 2 status */
typedef enum {
    CONNECTION_OK = 0,
    CONNECTION_LOST,
    PHY_RESYNCH,
    PHY_HO_PRACH
} NR_UE_L2_STATE_t;

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
    uint8_t LCID:5;     // octet 1 [5:0]
    uint8_t R:2;        // octet 1 [7:6]
} __attribute__ ((__packed__)) NR_MAC_SUBHEADER_FIXED;

#define DL_SCH_LCID_CCCH 0x0
#define DL_SCH_LCID_R_BITRATE 0x2f
#define DL_SCH_LCID_L_DRX 0x3b
#define DL_SCH_LCID_DRX 0x3c
#define DL_SCH_LCID_TA 0x3d
#define DL_SCH_LCID_CONTENTION_RESOLUTION_ID 0x3e
#define DL_SCH_LCID_PADDING 0x3f

#endif /*__LAYER2_MAC_DEFS_H__ */
