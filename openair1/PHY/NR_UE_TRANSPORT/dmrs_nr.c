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
* FILENAME    :  dmrs_nr.c
*
* MODULE      :  demodulation reference signals
*
* DESCRIPTION :  generation of dmrs sequences
*                3GPP TS 38.211
*
************************************************************************/

#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"

/***********************************************************************/

// TS 38.211 Table 6.4.1.1.3-3: PUSCH DMRS positions l' within a slot for single-symbol DMRS and intra-slot frequency hopping disabled.
// The first 4 colomns are PUSCH mapping type A and the last 4 colomns are PUSCH mapping type B.
// When l' = l0, it is represented by 1
// E.g. when symbol duration is 12 in colomn 7, value 1057 ('10000100001') which means l' =  l0, 5, 10.

int32_t table_6_4_1_1_3_3_pusch_dmrs_positions_l [12][8] = {                             // Duration in symbols
{-1,          -1,          -1,         -1,          1,          1,         1,         1},       //<4              // (DMRS l' position)
{1,            1,           1,          1,          1,          1,         1,         1},       //4               // (DMRS l' position)
{1,            1,           1,          1,          1,          5,         5,         5},       //5               // (DMRS l' position)
{1,            1,           1,          1,          1,          5,         5,         5},       //6               // (DMRS l' position)
{1,            1,           1,          1,          1,          5,         5,         5},       //7               // (DMRS l' position)
{1,          129,         129,        129,          1,         65,        73,        73},       //8               // (DMRS l' position)
{1,          129,         129,        129,          1,         65,        73,        73},       //9               // (DMRS l' position)
{1,          513,         577,        577,          1,        257,       273,       585},       //10              // (DMRS l' position)
{1,          513,         577,        577,          1,        257,       273,       585},       //11              // (DMRS l' position)
{1,          513,         577,       2337,          1,       1025,      1057,       585},       //12              // (DMRS l' position)
{1,         2049,        2177,       2337,          1,       1025,      1057,       585},       //13              // (DMRS l' position)
{1,         2049,        2177,       2337,          1,       1025,      1057,       585},       //14              // (DMRS l' position)
};

int32_t get_l_prime(uint8_t duration_in_symbols, uint8_t mapping_type, pusch_dmrs_AdditionalPosition_t additional_pos) {

  uint8_t row, colomn;
  int32_t l_prime;

  colomn = additional_pos;

  if (mapping_type == typeB)
    colomn += 4;

  if (duration_in_symbols < 4)
    row = 0;
  else
    row = duration_in_symbols - 3;

  l_prime = table_6_4_1_1_3_3_pusch_dmrs_positions_l[row][colomn];

  AssertFatal(l_prime>0,"invalid l_prime < 0\n");

  return l_prime;
}

/*******************************************************************
*
* NAME :         is_dmrs_symbol
*
* PARAMETERS : l                      ofdm symbol index within slot
*              k                      subcarrier index
*              start_sc               first subcarrier index
*              k_prime                index alternating 0 and 1
*              n                      index starting 0,1,...
*              delta                  see Table 6.4.1.1.3
*              duration_in_symbols    number of scheduled PUSCH ofdm symbols
*              dmrs_UplinkConfig      DMRS uplink configuration
*              mapping_type           PUSCH mapping type (A or B)
*              ofdm_symbol_size       IFFT size
*
* RETURN :       0 if symbol(k,l) is data, or 1 if symbol(k,l) is dmrs
*
* DESCRIPTION :  3GPP TS 38.211 6.4.1.1 Demodulation reference signal for PUSCH
*
*********************************************************************/

uint8_t is_dmrs_symbol(uint8_t l,
                       uint16_t k,
                       uint16_t start_sc,
                       uint8_t k_prime,
                       uint16_t n,
                       uint8_t delta,
                       uint8_t duration_in_symbols,
                       dmrs_UplinkConfig_t *dmrs_UplinkConfig,
                       uint8_t mapping_type,
                       uint16_t ofdm_symbol_size) {

  uint8_t is_dmrs_freq, is_dmrs_time, dmrs_type, l0;
  int32_t l_prime_mask;
  pusch_dmrs_AdditionalPosition_t additional_pos;

  is_dmrs_freq = 0;
  is_dmrs_time = 0;
  dmrs_type = dmrs_UplinkConfig->pusch_dmrs_type;
  additional_pos = dmrs_UplinkConfig->pusch_dmrs_AdditionalPosition;


  l0 = get_l0_ul(mapping_type, 2);
  l_prime_mask = get_l_prime(duration_in_symbols, mapping_type, additional_pos);

  if (k == ((start_sc+get_dmrs_freq_idx_ul(n, k_prime, delta, dmrs_type))%ofdm_symbol_size))
    is_dmrs_freq = 1;


  if (l_prime_mask == 1){

    if (l == l0)
      is_dmrs_time = 1;

  } else if ( (l==l0) || (((l_prime_mask>>l)&1) == 1 && l!=0) )
    is_dmrs_time = 1;

  if (dmrs_UplinkConfig->pusch_maxLength == pusch_len2){

    if (((l_prime_mask>>(l-1))&1) == 1 && l!=0 && l!=1)
      is_dmrs_time = 1;

    if (l-1 == l0)
      is_dmrs_time = 1;

  }

  if (is_dmrs_time && is_dmrs_freq)
    return 1;
  else
    return 0;

}

/*******************************************************************
*
* NAME :         pseudo_random_gold_sequence
*
* PARAMETERS :
*
* RETURN :       generate pseudo-random sequence which is a length-31 Gold sequence
*
* DESCRIPTION :  3GPP TS 38.211 5.2.1 Pseudo-random sequence generation
*                Sequence generation is a length-31 Gold sequence
*
*********************************************************************/

#define NC                     (1600)
#define GOLD_SEQUENCE_LENGTH   (31)

int pseudo_random_sequence(int M_PN, uint32_t *c, uint32_t cinit)
{
  int n;
  int size_x =  NC + GOLD_SEQUENCE_LENGTH + M_PN;
  uint32_t *x1;
  uint32_t *x2;

  x1 = calloc(size_x, sizeof(uint32_t));

  if (x1 == NULL) {
    msg("Fatal error: memory allocation problem \n");
    assert(0);
  }

  x2 = calloc(size_x, sizeof(uint32_t));

  if (x2 == NULL) {
    free(x1);
    msg("Fatal error: memory allocation problem \n");
    assert(0);
  }

  x1[0] = 1;  /* init first m sequence */

  /* cinit allows to initialise second m-sequence x2 */
  for (n = 0; n < GOLD_SEQUENCE_LENGTH; n++) {
     x2[n] = (cinit >> n) & 0x1;
  }

  for (n = 0; n < (NC + M_PN); n++) {
    x1[n+31] = (x1[n+3] + x1[n])%2;
    x2[n+31] = (x2[n+3] + x2[n+2] + x2[n+1] + x2[n])%2;
  }

  for (int n = 0; n < M_PN; n++) {
    c[n] = (x1[n+NC] + x2[n+NC])%2;
  }

  free(x1);
  free(x2);

  return 0;
}

/*******************************************************************
*
* NAME :         pseudo_random_sequence_optimised
*
* PARAMETERS :
*
* RETURN :       generate pseudo-random sequence which is a length-31 Gold sequence
*
* DESCRIPTION :  3GPP TS 38.211 5.2.1 Pseudo-random sequence generation
*                Sequence generation is a length-31 Gold sequence
*                This is an optimized function based on bitmap variables
*
*                x1(0)=1,x1(1)=0,...x1(30)=0,x1(31)=1
*                x2 <=> cinit, x2(31) = x2(3)+x2(2)+x2(1)+x2(0)
*                x2 <=> cinit = sum_{i=0}^{30} x2(i)2^i
*                c(n) = x1(n+Nc) + x2(n+Nc) mod 2
*
*                                             equivalent to
* x1(n+31) = (x1(n+3)+x1(n))mod 2                   <=>      x1(n) = x1(n-28) + x1(n-31)
* x2(n+31) = (x2(n+3)+x2(n+2)+x2(n+1)+x2(n))mod 2   <=>      x2(n) = x2(n-28) + x2(n-29) + x2(n-30) + x2(n-31)
*
*********************************************************************/

void pseudo_random_sequence_optimised(unsigned int size, uint32_t *c, uint32_t cinit)
{
  unsigned int n,x1,x2;

  /* init of m-sequences */
  x1 = 1+ (1<<31);
  x2 = cinit;
  x2=x2 ^ ((x2 ^ (x2>>1) ^ (x2>>2) ^ (x2>>3))<<31);

  /* skip first 50 double words of uint32_t (1600 bits) */
  for (n=1; n<50; n++) {
    x1 = (x1>>1) ^ (x1>>4);
    x1 = x1 ^ (x1<<31) ^ (x1<<28);
    x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
    x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
  }

  for (n=0; n<size; n++) {
    x1 = (x1>>1) ^ (x1>>4);
    x1 = x1 ^ (x1<<31) ^ (x1<<28);
    x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
    x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
    c[n] = x1^x2;
  }
}

/*******************************************************************
*
* NAME :         lte_gold_new
*
* PARAMETERS :
*
* RETURN :       generate pseudo-random sequence which is a length-31 Gold sequence
*
* DESCRIPTION :  This function is the same as "lte_gold" function in file lte_gold.c
*                It allows checking that optimization works fine.
*                generated sequence is given in an array as a bit map.
*
*********************************************************************/

#define CELL_DMRS_LENGTH   (224*2)
#define CHECK_GOLD_SEQUENCE

void lte_gold_new(LTE_DL_FRAME_PARMS *frame_parms, uint32_t lte_gold_table[20][2][14], uint16_t Nid_cell)
{
  unsigned char ns,l,Ncp=1-frame_parms->Ncp;
  uint32_t cinit;

#ifdef CHECK_GOLD_SEQUENCE

  uint32_t dmrs_bitmap[20][2][14];
  uint32_t *dmrs_sequence =  calloc(CELL_DMRS_LENGTH, sizeof(uint32_t));
  if (dmrs_sequence == NULL) {
    msg("Fatal error: memory allocation problem \n");
  	assert(0);
  }
  else
  {
    printf("Check of demodulation reference signal of pbch sequence \n");
  }

#endif

  /* for each slot number */
  for (ns=0; ns<20; ns++) {

  /* for each ofdm position */
    for (l=0; l<2; l++) {

      cinit = Ncp +
             (Nid_cell<<1) +
             (((1+(Nid_cell<<1))*(1 + (((frame_parms->Ncp==0)?4:3)*l) + (7*(1+ns))))<<10);

      pseudo_random_sequence_optimised(14, &(lte_gold_table[ns][l][0]), cinit);

#ifdef CHECK_GOLD_SEQUENCE

      pseudo_random_sequence(CELL_DMRS_LENGTH, dmrs_sequence, cinit);

      int j = 0;
      int k = 0;

      /* format for getting bitmap from uint32_t */
      for (int i=0; i<14; i++) {
        dmrs_bitmap[ns][l][i] = 0;
        for (; j < k + 32; j++) {
          dmrs_bitmap[ns][l][i] |= (dmrs_sequence[j]<<j);
        }
        k = j;
      }

      for (int i=0; i<14; i++) {
        if (lte_gold_table[ns][l][i] != dmrs_bitmap[ns][l][i]) {
          printf("Error in gold sequence computation for ns %d l %d and index %i : 0x%x 0x%x \n", ns, l, i, lte_gold_table[ns][l][i], dmrs_bitmap[ns][l][i]);
          assert(0);
        }
      }

#endif

    }
  }

#ifdef CHECK_GOLD_SEQUENCE
  free(dmrs_sequence);
#endif
}

/*******************************************************************
*
* NAME :         get_l0_ul
*
* PARAMETERS :   mapping_type : PUSCH mapping type
*                dmrs_typeA_position  : higher layer parameter
*
* RETURN :       demodulation reference signal for PUSCH
*
* DESCRIPTION :  see TS 38.211 V15.4.0 Demodulation reference signals for PUSCH
*
*********************************************************************/

uint8_t get_l0_ul(uint8_t mapping_type, uint8_t dmrs_typeA_position) {

  return ((mapping_type==typeA)?dmrs_typeA_position:0);

}

/*******************************************************************
*
* NAME :         get_dmrs_freq_idx_ul
*
* PARAMETERS :   n : index of DMRS symbol
*                k_prime  : k_prime = {0,1}
*                delta : given by Tables 6.4.1.1.3-1 and 6.4.1.1.3-2
*                dmrs_type  : DMRS configuration type
*
* RETURN :       demodulation reference signal for PUSCH
*
* DESCRIPTION :  see TS 38.211 V15.4.0 Demodulation reference signals for PUSCH
*
*********************************************************************/

uint16_t get_dmrs_freq_idx_ul(uint16_t n, uint8_t k_prime, uint8_t delta, uint8_t dmrs_type) {

  uint16_t dmrs_idx;

  if (dmrs_type == pusch_dmrs_type1)
    dmrs_idx = ((n<<2)+(k_prime<<1)+delta);
  else
    dmrs_idx = (6*n+k_prime+delta);

  return dmrs_idx;
}

/*******************************************************************
*
* NAME :         get_dmrs_pbch
*
* PARAMETERS :   i_ssb : index of ssb/pbch beam
*                n_hf  : number of the half frame in which PBCH is transmitted in frame
*
* RETURN :       demodulation reference signal for PBCH
*
* DESCRIPTION :  see TS 38.211 7.4.1.4 Demodulation reference signals for PBCH
*
*********************************************************************/

#define CHECK_DMRS_PBCH_SEQUENCE

void generate_dmrs_pbch(uint32_t dmrs_pbch_bitmap[DMRS_PBCH_I_SSB][DMRS_PBCH_N_HF][DMRS_BITMAP_SIZE], uint16_t Nid_cell)
{
  uint32_t cinit;
  int i_ssb;
  int n_hf;
  int _i_ssb;

#ifdef CHECK_DMRS_PBCH_SEQUENCE

  uint32_t dmrs_bitmap[DMRS_PBCH_I_SSB][DMRS_PBCH_N_HF][DMRS_BITMAP_SIZE];
  uint32_t *dmrs_sequence =  calloc(CELL_DMRS_LENGTH, sizeof(uint32_t));
  if (dmrs_sequence == NULL) {
    msg("Fatal error: memory allocation problem \n");
  	assert(0);
  }
  else
  {
    printf("Check of demodulation reference signal of pbch sequence \n");
  }

#endif

  /* for each slot number */
  for (i_ssb = 0; i_ssb<DMRS_PBCH_I_SSB; i_ssb++) {

    /* for each ofdm position */
    for (n_hf=0; n_hf<DMRS_PBCH_N_HF; n_hf++) {

      _i_ssb = i_ssb + 4*n_hf;

      cinit = (((_i_ssb + 1)*((Nid_cell>>4) + 1))<<11) + ((_i_ssb + 1)<<6) + (Nid_cell%4);

      pseudo_random_sequence_optimised(DMRS_BITMAP_SIZE, &(dmrs_pbch_bitmap[i_ssb][n_hf][0]), cinit);

#ifdef CHECK_DMRS_PBCH_SEQUENCE

      /* it allows checking generated with standard generation code */
      pseudo_random_sequence(DMRS_BITMAP_SIZE*sizeof(uint32_t), dmrs_sequence, cinit);

      int j = 0;
      int k = 0;

      /* format for getting bitmap from uint32_t */
      for (int i=0; i<DMRS_BITMAP_SIZE; i++) {
    	dmrs_bitmap[i_ssb][n_hf][i] = 0;
    	/* convert to bitmap */
      	for (; j < k + 32; j++) {
          dmrs_bitmap[i_ssb][n_hf][i] |= (dmrs_sequence[j]<<j);
      	}
      	k = j;
      }

      for (int i=0; i<DMRS_BITMAP_SIZE; i++) {
        if (dmrs_pbch_bitmap[i_ssb][n_hf][i] != dmrs_bitmap[i_ssb][n_hf][i]) {
          printf("Error in gold sequence computation for ns %d l %d and index %i : 0x%x 0x%x \n", i_ssb, n_hf, i, dmrs_pbch_bitmap[i_ssb][n_hf][i], dmrs_bitmap[i_ssb][n_hf][i]);
      	  assert(0);
        }
      }

#endif

    }
  }

#ifdef CHECK_DMRS_PBCH_SEQUENCE
  free(dmrs_sequence);
#endif
}

