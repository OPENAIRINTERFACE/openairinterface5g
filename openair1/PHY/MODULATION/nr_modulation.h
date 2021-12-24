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

#ifndef __NR_MODULATION_H__
#define __NR_MODULATION_H__

#include <stdint.h>
#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"

#define DMRS_MOD_ORDER 2
/*Precoding matices: W[pmi][antenna_port][layer]*/
extern char nr_W_1l_2p[6][2][1];
extern char nr_W_2l_2p[3][2][2];
extern char nr_W_1l_4p[28][4][1];
extern char nr_W_2l_4p[22][4][2];
extern char nr_W_3l_4p[7][4][3];
extern char nr_W_4l_4p[5][4][4];
/*! \brief Perform NR modulation. TS 38.211 V15.4.0 subclause 5.1
  @param[in] in, Pointer to input bits
  @param[in] length, size of input bits
  @param[in] modulation_type, modulation order
  @param[out] out, complex valued modulated symbols
*/

void nr_modulation(uint32_t *in,
                   uint32_t length,
                   uint16_t mod_order,
                   int16_t *out);

/*! \brief Perform NR layer mapping. TS 38.211 V15.4.0 subclause 7.3.1.3
  @param[in] mod_symbs, double Pointer to modulated symbols for each codeword
  @param[in] n_layers, number of layers
  @param[in] n_symbs, number of modulated symbols
  @param[out] tx_layers, modulated symbols for each layer
*/

void nr_layer_mapping(int16_t **mod_symbs,
                         uint8_t n_layers,
                         uint16_t n_symbs,
                         int16_t **tx_layers);

/*! \brief Perform NR layer mapping. TS 38.211 V15.4.0 subclause 7.3.1.3
  @param[in] ulsch_ue, double Pointer to NR_UE_ULSCH_t struct
  @param[in] n_layers, number of layers
  @param[in] n_symbs, number of modulated symbols
  @param[out] tx_layers, modulated symbols for each layer
*/

void nr_ue_layer_mapping(NR_UE_ULSCH_t **ulsch_ue,
                         uint8_t n_layers,
                         uint16_t n_symbs,
                         int16_t **tx_layers);


/*!
\brief This function implements the OFDM front end processor on reception (FEP)
\param frame_parms Pointer to frame parameters
\param rxdata Pointer to input data in time domain
\param rxdataF Pointer to output data in frequency domain
\param symbol symbol within slot (0..12/14)
\param Ns Slot number (0..19)
\param sample_offset offset within rxdata (points to beginning of subframe)
*/

int nr_slot_fep_ul(NR_DL_FRAME_PARMS *frame_parms,
                   int32_t *rxdata,
                   int32_t *rxdataF,
                   unsigned char symbol,
                   unsigned char Ns,
                   int sample_offset);

/*!
\brief This function implements the dft transform precoding in PUSCH
\param z Pointer to output in frequnecy domain
\param d Pointer to input in time domain
\param Msc_PUSCH number of allocated data subcarriers
*/
void nr_dft(int32_t *z,int32_t *d, uint32_t Msc_PUSCH);

int nr_beam_precoding(int32_t **txdataF,
	              int32_t **txdataF_BF,
                      NR_DL_FRAME_PARMS *frame_parms,
	              int32_t ***beam_weights,
                      int slot,
                      int symbol,
                      int aa,
                      int nb_antenna_ports,
                      int offset
);

void apply_nr_rotation(NR_DL_FRAME_PARMS *fp,
		       int16_t* txdata,
		       int slot,
		       int first_symbol,
		       int nsymb,
		       int length);

void init_symbol_rotation(NR_DL_FRAME_PARMS *fp);

void init_timeshift_rotation(NR_DL_FRAME_PARMS *fp);

void apply_nr_rotation_ul(NR_DL_FRAME_PARMS *frame_parms,
			  int32_t *rxdataF,
			  int slot,
			  int first_symbol,
			  int nsymb,
			  int length);

/*! \brief Perform NR precoding. TS 38.211 V15.4.0 subclause 6.3.1.5
  @param[in] datatx_F_precoding, Pointer to n_layers*re data array
  @param[in] prec_matrix, Pointer to precoding matrix
  @param[in] n_layers, number of DLSCH layers
*/
int nr_layer_precoder(int16_t **datatx_F_precoding,
		char *prec_matrix,
		uint8_t n_layers,
		int32_t re_offset);
#endif
