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

#ifndef __NR_TRANSPORT__H__
#define __NR_TRANSPORT__H__

#include "PHY/defs_gNB.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"

#define NR_PBCH_PDU_BITS 24

/*!
\fn int nr_generate_pss
\brief Generation of the NR PSS
@param
@returns 0 on success
 */
int nr_generate_pss(  int16_t *d_pss,
                      int32_t *txdataF,
                      int16_t amp,
                      uint8_t ssb_start_symbol,
                      nfapi_nr_config_request_scf_t *config,
                      NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_generate_sss
\brief Generation of the NR SSS
@param
@returns 0 on success
 */
int nr_generate_sss(  int16_t *d_sss,
                      int32_t *txdataF,
                      int16_t amp,
                      uint8_t ssb_start_symbol,
                      nfapi_nr_config_request_scf_t *config,
                      NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_generate_pbch_dmrs
\brief Generation of the DMRS for the PBCH
@param
@returns 0 on success
 */
int nr_generate_pbch_dmrs(uint32_t *gold_pbch_dmrs,
                          int32_t *txdataF,
                          int16_t amp,
                          uint8_t ssb_start_symbol,
                          nfapi_nr_config_request_scf_t *config,
                          NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_pbch_scrambling
\brief PBCH scrambling function
@param
 */
void nr_pbch_scrambling(NR_gNB_PBCH *pbch,
                        uint32_t Nid,
                        uint8_t nushift,
                        uint16_t M,
                        uint16_t length,
                        uint8_t encoded,
                        uint32_t unscrambling_mask);

/*!
\fn int nr_generate_pbch
\brief Generation of the PBCH
@param
@returns 0 on success
 */
int nr_generate_pbch(NR_gNB_PBCH *pbch,
		     nfapi_nr_dl_tti_ssb_pdu *ssb_pdu,
                     uint8_t *interleaver,
                     int32_t *txdataF,
                     int16_t amp,
                     uint8_t ssb_start_symbol,
                     uint8_t n_hf,
                     int sfn,
                     nfapi_nr_config_request_scf_t *config,
                     NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_generate_pbch
\brief PBCH interleaving function
@param bit index i of the input payload
@returns the bit index of the output
 */
void nr_init_pbch_interleaver(uint8_t *interleaver);

NR_gNB_DLSCH_t *new_gNB_dlsch(NR_DL_FRAME_PARMS *frame_parms,
                              unsigned char Kmimo,
                              unsigned char Mdlharq,
                              uint32_t Nsoft,
                              uint8_t abstraction_flag,
                              uint16_t N_RB);

void rx_nr_prach(PHY_VARS_gNB *gNB,
		 int frame,
		 int subframe,
		 uint16_t *max_preamble,
		 uint16_t *max_preamble_energy,
		 uint16_t *max_preamble_delay
		 );

void rx_nr_prach_ru(RU_t *ru,
		    int frame,
		    int subframe);

void compute_nr_prach_seq(uint16_t rootSequenceIndex,
			  uint8_t prach_ConfigIndex,
			  uint8_t zeroCorrelationZoneConfig,
			  uint8_t highSpeedFlag,
			  lte_frame_type_t frame_type,
			  nr_frequency_range_e fr,
			  uint32_t X_u[64][839]);

void nr_decode_pucch1(int32_t **rxdataF,
                      pucch_GroupHopping_t pucch_GroupHopping,
                      uint32_t n_id,       // hoppingID higher layer parameter
                      uint64_t *payload,
                      NR_DL_FRAME_PARMS *frame_parms,
                      int16_t amp,
                      int nr_tti_tx,
                      uint8_t m0,
                      uint8_t nrofSymbols,
                      uint8_t startingSymbolIndex,
                      uint16_t startingPRB,
                      uint16_t startingPRB_intraSlotHopping,
                      uint8_t timeDomainOCC,
                      uint8_t nr_bit);

void nr_decode_pucch0(PHY_VARS_gNB *gNB,
		      int slot,
                      nfapi_nr_uci_pucch_pdu_format_0_1_t* uci_pdu,
                      nfapi_nr_pucch_pdu_t* pucch_pdu);

#endif /*__NR_TRANSPORT__H__*/
