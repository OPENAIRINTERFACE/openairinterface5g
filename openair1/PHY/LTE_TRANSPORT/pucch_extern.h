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

/*! \file PHY/LTE_TRANSPORT/pucch.c
* \brief Top-level routines for generating and decoding the PUCCH physical channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include <stdint.h>

/* PUCCH format3 >> */
#define D_I             0
#define D_Q             1
#define D_IQDATA        2
#define D_NSLT1SF       2
#define D_NSYM1SLT      7
#define D_NSYM1SF       2*7
#define D_NSC1RB        12
#define D_NRB1PUCCH     2
#define D_NPUCCH_SF5    5
#define D_NPUCCH_SF4    4

extern int16_t W4[3][4];

extern int16_t W3_re[3][6];


extern int16_t W3_im[3][6];

extern int16_t alpha_re[12];
extern int16_t alpha_im[12];

extern char *pucch_format_string[];

extern uint8_t chcod_tbl[128][48];

extern int16_t W5_fmt3_re[5][5];

extern int16_t W5_fmt3_im[5][5];

extern int16_t W4_fmt3[4][4];

extern int16_t W2[2];

extern int16_t RotTBL_re[4];
extern int16_t RotTBL_im[4];

//np4_tbl, np5_tbl
extern uint8_t Np5_TBL[5];
extern uint8_t Np4_TBL[4];

// alpha_TBL
extern int16_t alphaTBL_re[12];
extern int16_t alphaTBL_im[12];
