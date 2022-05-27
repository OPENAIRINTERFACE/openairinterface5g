
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

/*! \file PHY/NR_TRANSPORT/nr_transport_common_proto.h
* \brief Some support routines
* \author
* \date 2019
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */

#ifndef __NR_TRANSPORT_COMMON_PROTO__H__
#define __NR_TRANSPORT_COMMON_PROTO__H__

#include "PHY/defs_nr_common.h"


void nr_group_sequence_hopping(pucch_GroupHopping_t PUCCH_GroupHopping,
                               uint32_t n_id,
                               uint8_t n_hop,
                               int nr_slot_tx,
                               uint8_t *u,
                               uint8_t *v);

double nr_cyclic_shift_hopping(uint32_t n_id,
                               uint8_t m0,
                               uint8_t mcs,
                               uint8_t lnormal,
                               uint8_t lprime,
                               int nr_slot_tx);

/** \brief Computes available bits G. */
uint32_t nr_get_G(uint16_t nb_rb, uint16_t nb_symb_sch, uint8_t nb_re_dmrs, uint16_t length_dmrs, uint8_t Qm, uint8_t Nl);

uint32_t nr_get_E(uint32_t G, uint8_t C, uint8_t Qm, uint8_t Nl, uint8_t r);

void compute_nr_prach_seq(uint8_t short_sequence,
                          uint8_t num_sequences,
                          uint8_t rootSequenceIndex,
                          uint32_t X_u[64][839]);

void nr_fill_du(uint16_t N_ZC,uint16_t *prach_root_sequence_map);

void init_nr_prach_tables(int N_ZC);

void nr_codeword_scrambling(uint8_t *in,
                            uint32_t size,
                            uint8_t q,
                            uint32_t Nid,
                            uint32_t n_RNTI,
                            uint32_t* out);

void nr_codeword_unscrambling(int16_t* llr, uint32_t size, uint8_t q, uint32_t Nid, uint32_t n_RNTI);

/**@}*/

void init_pucch2_luts(void);
#endif
