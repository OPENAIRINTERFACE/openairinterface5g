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

/*! \file PHY/LTE_TRANSPORT/prach_common.c
 * \brief Common routines for UE/eNB PRACH physical channel V8.6 2009-03
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#ifndef __PHY_LTE_TRANSPORT_PRACH_EXTERN__H__
#define __PHY_LTE_TRANSPORT_PRACH_EXTERN__H__

#include "PHY/sse_intrin.h"
#include "PHY/defs_eNB.h"
//#include "PHY/phy_extern.h"

//#define PRACH_DEBUG 1
//#define PRACH_WRITE_OUTPUT_DEBUG 1

extern uint16_t NCS_unrestricted[16];
extern uint16_t NCS_restricted[15];
extern uint16_t NCS_4[7];

extern int16_t ru[2*839]; // quantized roots of unity
extern uint32_t ZC_inv[839]; // multiplicative inverse for roots u
extern uint16_t du[838];



// This is table 5.7.1-4 from 36.211
extern PRACH_TDD_PREAMBLE_MAP tdd_preamble_map[64][7];




extern uint16_t prach_root_sequence_map0_3[838];
 

extern uint16_t prach_root_sequence_map4[138];

void dump_prach_config(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);


// This function computes the du
void fill_du(uint8_t prach_fmt);


uint8_t get_num_prach_tdd(module_id_t Mod_id);


uint8_t get_fid_prach_tdd(module_id_t Mod_id,uint8_t tdd_map_index);


int is_prach_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint32_t frame, uint8_t subframe);


void compute_prach_seq(uint16_t rootSequenceIndex,
		       uint8_t prach_ConfigIndex,
		       uint8_t zeroCorrelationZoneConfig,
		       uint8_t highSpeedFlag,
		       lte_frame_type_t frame_type,
		       uint32_t X_u[64][839]);

#endif
