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

/**********************************************************************
*
* FILENAME    :  ptrs_nr.c
*
* MODULE      :  phase tracking reference signals
*
* DESCRIPTION :  resource element mapping of ptrs sequences
*                3GPP TS 38.211 and 3GPP TS 38.214
*
************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include "dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"

/***********************************************************************/


 //#define max(a,b) (((a) > (b)) ? (a) : (b))

// TS 38.211 Table 6.4.1.2.2.1-1: The parameter kRE_ref.
// The first 4 colomns are DM-RS Configuration type 1 and the last 4 colomns are DM-RS Configuration type 2.

int16_t table_6_4_1_2_2_1_1_pusch_ptrs_kRE_ref [6][8] = {
{ 0,            2,           6,          8,          0,          1,         6,         7},
{ 2,            4,           8,         10,          1,          6,         7,         0},
{ 1,            3,           7,          9,          2,          3,         8,         9},
{ 3,            5,           9,         11,          3,          8,         9,         2},
{-1,           -1,          -1,         -1,          4,          5,        10,        11},
{-1,           -1,          -1,         -1,          5,         10,        11,         4},
};


/*******************************************************************
*
* NAME :         get_kRE_ref
*
* PARAMETERS :   dmrs_antenna_port      DMRS antenna port
*                pusch_dmrs_type        PUSCH DMRS type
*                resourceElementOffset  the parameter resourceElementOffset
*
* RETURN :       the parameter k_RE_ref
*
* DESCRIPTION :  3GPP TS 38.211 Table 6.4.1.2.2.1-1
*
*********************************************************************/

int16_t get_kRE_ref(uint8_t dmrs_antenna_port, uint8_t pusch_dmrs_type, uint8_t resourceElementOffset) {

  uint8_t colomn;
  int16_t k_RE_ref;

  colomn = resourceElementOffset;

  if (pusch_dmrs_type == 2)
    colomn += 4;

  k_RE_ref = table_6_4_1_2_2_1_1_pusch_ptrs_kRE_ref[dmrs_antenna_port][colomn];

  AssertFatal(k_RE_ref>=0,"invalid k_RE_ref < 0. Check PTRS Configuration\n");

  return k_RE_ref;
}


/*******************************************************************
*
* NAME :         get_K_ptrs
*
* PARAMETERS :   ptrs_UplinkConfig      PTRS uplink configuration
*                N_RB                   number of RBs scheduled for PUSCH
*
* RETURN :       the parameter K_ptrs
*
* DESCRIPTION :  3GPP TS 38.214 6.2.3 Table 6.2.3.1-2
*
*********************************************************************/

uint8_t get_K_ptrs(ptrs_UplinkConfig_t *ptrs_UplinkConfig, uint16_t N_RB){

	uint16_t nrb0, nrb1;

	nrb0 = ptrs_UplinkConfig->frequencyDensity.n_rb0;
	nrb1 = ptrs_UplinkConfig->frequencyDensity.n_rb1;

	if (nrb0 == 0 || nrb0 == 0)
		return 2;

	if (N_RB < nrb0){
		LOG_I(PHY,"PUSH PT-RS is not present.\n");
		return 0;
	}
	else if (N_RB >= nrb0 && N_RB < nrb1)
		return 2;
	else
		return 4;
}

/*******************************************************************
*
* NAME :         set_ptrs_symb_idx
*
* PARAMETERS :   ptrs_symbols           PTRS OFDM symbol indicies bit mask
*                ptrs_UplinkConfig      PTRS uplink configuration
*                dmrs_UplinkConfig      DMRS uplink configuration
*                mapping_type           PUSCH time domain mapping type
*                duration_in_symbols    number of scheduled PUSCH ofdm symbols
*                start_symbol           first ofdm symbol of PUSCH within slot
*                L_ptrs                 the parameter L_ptrs
*                ofdm_symbol_size       FFT size
*
* RETURN :       sets the bit map of PTRS ofdm symbol indicies
*
* DESCRIPTION :  3GPP TS 38.211 6.4.1.2.2.1
*
*********************************************************************/

void set_ptrs_symb_idx(uint16_t *ptrs_symbols,
                       ptrs_UplinkConfig_t *ptrs_UplinkConfig,
                       dmrs_UplinkConfig_t *dmrs_UplinkConfig,
                       uint8_t mapping_type,
                       uint8_t duration_in_symbols,
                       uint8_t start_symbol,
                       uint8_t L_ptrs,
                       uint16_t ofdm_symbol_size) {

	uint8_t i, last_symbol, is_dmrs_symbol1, is_dmrs_symbol2;
	int16_t l_ref;

	*ptrs_symbols = 0;
	i = 0;
	is_dmrs_symbol1 = 0;
	is_dmrs_symbol2 = 0;
	l_ref = start_symbol;
	last_symbol = start_symbol + duration_in_symbols - 1;

	while ( (l_ref + i*L_ptrs) <= last_symbol) {

		is_dmrs_symbol1 = is_dmrs_symbol(max((l_ref + (i-1)*L_ptrs + 1), l_ref),
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         duration_in_symbols,
                                         dmrs_UplinkConfig,
                                         mapping_type,
                                         ofdm_symbol_size);

		is_dmrs_symbol2 = is_dmrs_symbol(l_ref + i*L_ptrs,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         duration_in_symbols,
                                         dmrs_UplinkConfig,
                                         mapping_type,
                                         ofdm_symbol_size);

		if ( is_dmrs_symbol1 + is_dmrs_symbol2 > 0 ) {

			if (dmrs_UplinkConfig->pusch_maxLength == 2)
				l_ref = l_ref + i*L_ptrs + 1;
			else
				l_ref = l_ref + i*L_ptrs;

			i = 1;

			continue;

		}

		*ptrs_symbols = *ptrs_symbols | (1<<(l_ref + i*L_ptrs));
		i++;
	}
}

/*******************************************************************
*
* NAME :         get_L_ptrs
*
* PARAMETERS :   ptrs_UplinkConfig      PTRS uplink configuration
*                I_mcs                  MCS index used for PUSCH
*
* RETURN :       the parameter L_ptrs
*
* DESCRIPTION :  3GPP TS 38.214 6.2.3 Table 6.2.3.1-1
*
*********************************************************************/

uint8_t get_L_ptrs(ptrs_UplinkConfig_t *ptrs_UplinkConfig, uint8_t I_mcs) {

	uint8_t mcs1, mcs2, mcs3;

	mcs1 = ptrs_UplinkConfig->timeDensity.ptrs_mcs1;
	mcs2 = ptrs_UplinkConfig->timeDensity.ptrs_mcs2;
	mcs3 = ptrs_UplinkConfig->timeDensity.ptrs_mcs3;

	if (mcs1 == 0 || mcs2 == 0 || mcs3 == 0)
		return 1;

	if (I_mcs < mcs1){
		LOG_I(PHY,"PUSH PT-RS is not present.\n");
		return 0;
	}
	else if (I_mcs >= mcs1 && I_mcs < mcs2)
		return 4;
	else if (I_mcs >= mcs2 && I_mcs < mcs3)
		return 2;
	else
		return 1;
}

/*******************************************************************
*
* NAME :         is_ptrs_subcarrier
*
* PARAMETERS : k                      subcarrier index
*              K_ptrs                 the parameter K_ptrs
*              n_rnti                 UE CRNTI
*              N_RB                   number of RBs scheduled for PUSCH
*              k_RE_ref               the parameter k_RE_ref
*              start_sc               first subcarrier index
*              ofdm_symbol_size       FFT size
*
* RETURN :       1 if subcarrier k is PTRS, or 0 otherwise
*
* DESCRIPTION :  3GPP TS 38.211 6.4.1.2 Phase-tracking reference signal for PUSCH
*
*********************************************************************/

uint8_t is_ptrs_subcarrier(uint16_t k, uint8_t K_ptrs, uint16_t n_rnti, uint16_t N_RB, int16_t k_RE_ref, uint16_t start_sc, uint16_t ofdm_symbol_size) {

	uint16_t k_RB_ref, i, sc;

	i = 0;
	sc = 0;
	k_RB_ref = 0;

	if (N_RB % K_ptrs == 0)
		k_RB_ref = n_rnti % K_ptrs;
	else
		k_RB_ref = n_rnti % (N_RB % K_ptrs);

	while (k > sc){

		sc = (start_sc + k_RE_ref + (i*K_ptrs + k_RB_ref)*NR_NB_SC_PER_RB)%ofdm_symbol_size;
		i++;

	}

	if (k == sc)
        return 1;
	else
        return 0;

}

/*******************************************************************
*
* NAME :         is_ptrs_symbol
*
* PARAMETERS : l                      ofdm symbol index within slot
*              k                      subcarrier index
*              n_rnti                 UE CRNTI
*              N_RB                   number of RBs scheduled for PUSCH
*              duration_in_symbols    number of scheduled PUSCH ofdm symbols
*              dmrs_antenna_port      DMRS antenna port
*              K_ptrs                 the parameter K_ptrs
*              ptrs_symbols           bit mask of ptrs
*              start_sc               first subcarrier index
*              ofdm_symbol_size       FFT size
*              pusch_dmrs_type        PUSCH DMRS type (1 or 2)
*              ptrs_UplinkConfig      PTRS uplink configuration
*
* RETURN :       0 if symbol(k,l) is data, or 1 if symbol(k,l) is ptrs
*
* DESCRIPTION :  3GPP TS 38.211 6.4.1.2 Phase-tracking reference signal for PUSCH
*
*********************************************************************/

uint8_t is_ptrs_symbol(uint8_t l,
                       uint16_t k,
                       uint16_t n_rnti,
                       uint16_t N_RB,
                       uint8_t duration_in_symbols,
                       uint8_t dmrs_antenna_port,
                       uint8_t K_ptrs,
                       uint16_t ptrs_symbols,
                       uint16_t start_sc,
                       uint16_t ofdm_symbol_size,
                       pusch_dmrs_type_t pusch_dmrs_type,
                       ptrs_UplinkConfig_t *ptrs_UplinkConfig) {

  uint8_t is_ptrs_freq, is_ptrs_time;
  int16_t k_RE_ref;

  is_ptrs_freq = 0;
  is_ptrs_time = 0;

  k_RE_ref = get_kRE_ref(dmrs_antenna_port, pusch_dmrs_type, ptrs_UplinkConfig->resourceElementOffset);

  is_ptrs_freq = is_ptrs_subcarrier(k, K_ptrs, n_rnti, N_RB, k_RE_ref, start_sc, ofdm_symbol_size);

  if (is_ptrs_freq == 0)
    return 0;

  if (((ptrs_symbols>>l)&1) == 1)
    is_ptrs_time = 1;

  if (is_ptrs_time && is_ptrs_freq)
    return 1;
  else
    return 0;

}

/*
int main(int argc, char const *argv[])
{

	dmrs_UplinkConfig_t dmrs_Uplink_Config;
	ptrs_UplinkConfig_t ptrs_Uplink_Config;
	uint8_t resourceElementOffset;
	uint8_t dmrs_antenna_port;
	uint8_t L_ptrs, K_ptrs;
	int16_t k_RE_ref;
	uint16_t N_RB, ptrs_symbols, ofdm_symbol_size, k;
	uint8_t duration_in_symbols, I_mcs;
	uint8_t start_symbol, l;
	uint8_t ptrs_symbol_flag;
	uint16_t n_rnti;

    dmrs_Uplink_Config.pusch_dmrs_type = pusch_dmrs_type1;
    dmrs_Uplink_Config.pusch_dmrs_AdditionalPosition = pusch_dmrs_pos1;
    dmrs_Uplink_Config.pusch_maxLength = pusch_len2;

    ptrs_Uplink_Config.timeDensity.ptrs_mcs1 = 0; // setting MCS values to 0 indicate abscence of time_density field in the configuration
    ptrs_Uplink_Config.timeDensity.ptrs_mcs2 = 0;
    ptrs_Uplink_Config.timeDensity.ptrs_mcs3 = 0;
    ptrs_Uplink_Config.frequencyDensity.n_rb0 = 0;     // setting N_RB values to 0 indicate abscence of frequency_density field in the configuration
    ptrs_Uplink_Config.frequencyDensity.n_rb1 = 0;
    ptrs_Uplink_Config.resourceElementOffset = 0;

    n_rnti = 0x1234;
    resourceElementOffset = 0;
    ptrs_symbols = 0;
	dmrs_antenna_port = 0;
	N_RB = 50;
	duration_in_symbols = 14;
	ofdm_symbol_size = 2048;
	I_mcs = 9;
	start_symbol = 0;
	ptrs_symbol_flag = 0;

    k_RE_ref = get_kRE_ref(dmrs_antenna_port, dmrs_Uplink_Config.pusch_dmrs_type, resourceElementOffset);

    K_ptrs = get_K_ptrs(&ptrs_Uplink_Config, N_RB);

    L_ptrs = get_L_ptrs(&ptrs_Uplink_Config, I_mcs);

    set_ptrs_symb_idx(&ptrs_symbols,
                      &ptrs_Uplink_Config,
                      &dmrs_Uplink_Config,
                      1,
                      duration_in_symbols,
                      start_symbol,
                      L_ptrs,
                      ofdm_symbol_size);

    printf("PTRS OFDM symbol indicies: ");

    for (l = start_symbol; l < start_symbol + duration_in_symbols; l++){

        ptrs_symbol_flag = is_ptrs_symbol(l,
                                          0,
                                          n_rnti,
                                          N_RB,
                                          duration_in_symbols,
                                          dmrs_antenna_port,
                                          K_ptrs,
                                          ptrs_symbols,
                                          dmrs_Uplink_Config.pusch_dmrs_type,
                                          &ptrs_Uplink_Config);

        if (ptrs_symbol_flag == 1)
            printf(" %d ", l);

    }

    printf("\n");

    printf("PTRS subcarrier indicies: ");

    for (k = 0; k < N_RB*12; k++){

        if (is_ptrs_subcarrier(k, K_ptrs, n_rnti, N_RB, k_RE_ref) == 1)
            printf(" %d ", k);

    }

    printf("\n");

	return 0;
}
*/