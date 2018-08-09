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

#ifndef __MODULATION_DEFS__H__
#define __MODULATION_DEFS__H__
#include "PHY/defs_common.h"
#include "modulation_common.h"
/** @addtogroup _PHY_MODULATION_
 * @{
*/



/*!
\brief This function implements the OFDM front end processor on reception (FEP)
\param phy_vars_ue Pointer to PHY variables
\param l symbol within slot (0..6/7)
\param Ns Slot number (0..19)
\param sample_offset offset within rxdata (points to beginning of subframe)
\param no_prefix if 1 prefix is removed by HW
\param reset_freq_est if non-zero it resets the frequency offset estimation loop
*/

int slot_fep(PHY_VARS_UE *phy_vars_ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix,
	     int reset_freq_est);

int slot_fep_mbsfn(PHY_VARS_UE *phy_vars_ue,
                   unsigned char l,
                   int subframe,
                   int sample_offset,
                   int no_prefix);

int front_end_fft(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix);

int front_end_chanEst(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
            int reset_freq_est);

void apply_7_5_kHz(PHY_VARS_UE *phy_vars_ue,int32_t*txdata,uint8_t subframe);


int compute_BF_weights(int32_t **beam_weights, int32_t **calib_dl_ch_estimates, PRECODE_TYPE_t precode_type, int nb_ant, int nb_freq);



/** @}*/
#endif
