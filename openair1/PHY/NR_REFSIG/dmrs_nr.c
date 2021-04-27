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



uint8_t allowed_xlsch_re_in_dmrs_symbol(uint16_t k,
                                        uint16_t start_sc,
                                        uint16_t ofdm_symbol_size,
                                        uint8_t numDmrsCdmGrpsNoData,
                                        uint8_t dmrs_type) {
  uint8_t delta;
  uint16_t diff;
  if (k>start_sc)
    diff = k-start_sc;
  else
    diff = (ofdm_symbol_size-start_sc)+k;
  for (int i = 0; i<numDmrsCdmGrpsNoData; i++){
    if  (dmrs_type==NFAPI_NR_DMRS_TYPE1) {
      delta = i;
      if (((diff)%2)  == delta)
        return (0);
    }
    else {
      delta = i<<1;
      if (((diff%6) == delta) || ((diff%6) == (delta+1)))
        return (0);
    }
  }
  return (1);
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
  x1 = 1+ (1U<<31);
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
/* return the position of next dmrs symbol in a slot */
int8_t get_next_dmrs_symbol_in_slot(uint16_t  ul_dmrs_symb_pos, uint8_t counter, uint8_t end_symbol)
{
  for(uint8_t symbol = counter; symbol < end_symbol; symbol++) {
    if((ul_dmrs_symb_pos >> symbol) & 0x01 ) {
      return symbol;
    }
  }
  return -1;
}


/* return the total number of dmrs symbol in a slot */
uint8_t get_dmrs_symbols_in_slot(uint16_t l_prime_mask,  uint16_t nb_symb)
{
  uint8_t tmp = 0;
  for (int i = 0; i < nb_symb; i++) {
    tmp += (l_prime_mask >> i) & 0x01;
  }
  return tmp;
}

/* return the position of valid dmrs symbol in a slot for channel compensation */
int8_t get_valid_dmrs_idx_for_channel_est(uint16_t  dmrs_symb_pos, uint8_t counter)
{
  int8_t  symbIdx = -1;
  /* if current symbol is DMRS then return this index */
  if(is_dmrs_symbol(counter,  dmrs_symb_pos ) ==1) {
    return counter;
  }
  /* find previous DMRS symbol */
  for(int8_t symbol = counter;symbol >=0 ; symbol--) {
    if((1<<symbol & dmrs_symb_pos)> 0) {
      symbIdx = symbol;
      break;
    }
  }
  /* if there is no previous dmrs available then find the next possible*/
  if(symbIdx == -1) {
    symbIdx = get_next_dmrs_symbol_in_slot(dmrs_symb_pos,counter,15);
  }
  return symbIdx;
}
