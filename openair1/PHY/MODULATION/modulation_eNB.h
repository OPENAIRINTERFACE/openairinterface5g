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

#ifndef __MODULATION_ENB__H__
#define __MODULATION_ENB__H__
#include "PHY/defs_common.h"
#include "modulation_common.h"

/** @addtogroup _PHY_MODULATION_
 * @{
*/

int slot_fep_ul(RU_t *ru,
                unsigned char l,
                unsigned char Ns,
                int no_prefix);

void remove_7_5_kHz(RU_t *ru,uint8_t subframe);

/** \brief This function performs beamforming precoding for common
 * data
    @param txdataF Table of pointers for frequency-domain TX signals
    @param txdataF_BF Table of pointers for frequency-domain TX signals
    @param subframe subframe index
    @param frame_parms Frame descriptor structure
after beamforming
    @param beam_weights Beamforming weights applied on each
antenna element and each carrier
    @param slot Slot number
    @param symbol Symbol index on which to act
    @param aa physical antenna index
    @param p logical antenna index
    @param l1_id L1 instance id*/
int beam_precoding(int32_t **txdataF,
	           int32_t **txdataF_BF,
		   int subframe,
                   LTE_DL_FRAME_PARMS *frame_parms,
                   int32_t **beam_weights[NUMBER_OF_eNB_MAX+1][15],
                   int symbol,
		   int aa,
		   int p,
                   int l1_id);
				   
/** \brief This function performs beamforming precoding for common
 * data for only one eNB, fdragon
    @param txdataF Table of pointers for frequency-domain TX signals
    @param txdataF_BF Table of pointers for frequency-domain TX signals
    @param frame_parms Frame descriptor structure
after beamforming
    @param beam_weights Beamforming weights applied on each
antenna element and each carrier
    @param slot Slot number
    @param symbol Symbol index on which to act
    @param aa physical antenna index*/
int beam_precoding_one_eNB(int32_t **txdataF,
                           int32_t **txdataF_BF,
                           int32_t **beam_weights[NUMBER_OF_eNB_MAX+1][15],
			   int subframe,
			   int nb_antenna_ports,
			   int nb_tx, // total physical antenna
			   LTE_DL_FRAME_PARMS *frame_parms);

int f_read(char *calibF_fname, int nb_ant, int nb_freq, int32_t **tdd_calib_coeffs);

int estimate_DLCSI_from_ULCSI(int32_t **calib_dl_ch_estimates, int32_t **ul_ch_estimates, int32_t **tdd_calib_coeffs, int nb_ant, int nb_freq);

int compute_BF_weights(int32_t **beam_weights, int32_t **calib_dl_ch_estimates, PRECODE_TYPE_t precode_type, int nb_ant, int nb_freq);



/** @}*/
#endif
