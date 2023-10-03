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

/* file: nr_compute_tbs.c
   purpose: Compute NR TBS
   author: Hongzhi WANG (TCL)
*/
#define INDEX_MAX_TBS_TABLE (93)

#include "common/utils/nr/nr_common.h"
#include "openair1/PHY/defs_nr_common.h"
#include <math.h>

//Table 5.1.2.2-2
static const uint16_t Tbstable_nr[INDEX_MAX_TBS_TABLE] = {
    24,   32,   40,   48,   56,   64,   72,   80,   88,   96,   104,  112,  120,  128,  136,  144,  152,  160,  168,
    176,  184,  192,  208,  224,  240,  256,  272,  288,  304,  320,  336,  352,  368,  384,  408,  432,  456,  480,
    504,  528,  552,  576,  608,  640,  672,  704,  736,  768,  808,  848,  888,  928,  984,  1032, 1064, 1128, 1160,
    1192, 1224, 1256, 1288, 1320, 1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800, 1864, 1928, 2024, 2088, 2152, 2216,
    2280, 2408, 2472, 2536, 2600, 2664, 2728, 2792, 2856, 2976, 3104, 3240, 3368, 3496, 3624, 3752, 3824};

static const uint16_t NPRB_LBRM[7] = {32, 66, 107, 135, 162, 217, 273};

// Transport block size determination according to 6.1.4.2 of TS 38.214
// returns the TBS in bits
uint32_t nr_compute_tbs(uint16_t Qm,
                        uint16_t R,
                        uint16_t nb_rb,
                        uint16_t nb_symb_sch,
                        uint16_t nb_dmrs_prb,
                        uint16_t nb_rb_oh,
                        uint8_t tb_scaling,
                        uint8_t Nl)
{

  LOG_D(NR_MAC, "In %s: nb_symb_sch %d, nb_dmrs_prb %d, nb_rb %d, nb_rb_oh %d, tb_scaling %d Nl %d\n", __FUNCTION__, nb_symb_sch, nb_dmrs_prb, nb_rb, nb_rb_oh, tb_scaling, Nl);

  const int nb_subcarrier_per_rb = 12;
  const uint32_t nbp_re =  nb_subcarrier_per_rb * nb_symb_sch - nb_dmrs_prb - nb_rb_oh;
  const uint32_t nb_re = min(156, nbp_re) * nb_rb;
  // Intermediate number of information bits
  // Rx1024 is tabulated as 10 times the actual code rate
  const uint32_t R_5 = R/5; // R can be fractional so we can't divide by 10
  // So we ned to right shift by 11 (10 for x1024 and 1 additional as above)
  const uint32_t Ninfo = ((nb_re * R_5 * Qm * Nl)>>11)>>tb_scaling;

  uint32_t nr_tbs=0;
  uint32_t Np_info, C, n;

  if (Ninfo <= NR_MAX_PDSCH_TBS) {
    n = max(3, floor(log2(Ninfo)) - 6);
      Np_info = max(24, (Ninfo>>n)<<n);
      for (int i=0; i<INDEX_MAX_TBS_TABLE; i++) {
        if (Tbstable_nr[i] >= Np_info){
          nr_tbs = Tbstable_nr[i];
          break;
        }
      }
  } else {
    n = log2(Ninfo-24)-5;
    Np_info = max(3840, (ROUNDIDIV((Ninfo-24),(1<<n)))<<n);

    if (R <= 2560) {
        C = CEILIDIV((Np_info+24),3816);
        nr_tbs = (C<<3)*CEILIDIV((Np_info+24),(C<<3)) - 24;
    } else {
      if (Np_info > 8424){
          C = CEILIDIV((Np_info+24),8424);
          nr_tbs = (C<<3)*CEILIDIV((Np_info+24),(C<<3)) - 24;
      } else {
        nr_tbs = ((CEILIDIV((Np_info+24),8))<<3) - 24;
      }
    }
  }

  LOG_D(NR_MAC, "In %s: Ninfo %u nbp_re %d nb_re %d Qm %d, R %d, tbs %d bits\n", __FUNCTION__, Ninfo, nbp_re, nb_re, Qm, R, nr_tbs);

  return nr_tbs;

}

//tbslbrm calculation according to 5.4.2.1 of 38.212
uint32_t nr_compute_tbslbrm(uint16_t table,
			    uint16_t nb_rb,
		            uint8_t Nl) {

  uint16_t R, nb_re;
  uint16_t nb_rb_lbrm=0;
  uint8_t Qm;
  int i;
  uint32_t nr_tbs=0;
  uint32_t Ninfo, Np_info, C;
  uint8_t n;

  for (i=0; i<7; i++) {
    if (NPRB_LBRM[i] >= nb_rb){
    nb_rb_lbrm = NPRB_LBRM[i];
      break;
    }
  }

  Qm = ((table == 1)? 8 : 6);
  R = 948;
  nb_re = 156 * nb_rb_lbrm;

  // Intermediate number of information bits
  Ninfo = (nb_re * R * Qm * Nl)>>10;

  if (Ninfo <= NR_MAX_PDSCH_TBS) {
    n = max(3, floor(log2(Ninfo)) - 6);
    Np_info = max(24, (Ninfo>>n)<<n);
    for (int i=0; i<INDEX_MAX_TBS_TABLE; i++) {
      if (Tbstable_nr[i] >= Np_info){
        nr_tbs = Tbstable_nr[i];
        break;
      }
    }
  } else {
    n = log2(Ninfo-24)-5;
    Np_info = max(3840, (ROUNDIDIV((Ninfo-24),(1<<n)))<<n);

    if (R <= 256) {
      C = CEILIDIV((Np_info+24),3816);
      nr_tbs = (C<<3)*CEILIDIV((Np_info+24),(C<<3)) - 24;
    }
    else {
      if (Np_info > 8424){
        C = CEILIDIV((Np_info+24),8424);
        nr_tbs = (C<<3)*CEILIDIV((Np_info+24),(C<<3)) - 24;
      }
      else
        nr_tbs = ((CEILIDIV((Np_info+24),8))<<3) - 24;
    }
  }
  return nr_tbs;
}
