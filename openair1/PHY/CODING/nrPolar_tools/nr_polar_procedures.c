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

/*! \file nr_polar_procedures.c
 * \brief
 * \author Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"


// TS 38.212 - Section 5.3.1.2 Polar encoding
void nr_polar_generate_u(uint64_t *u,
                         const uint64_t *Cprime,
                         const uint8_t *information_bit_pattern,
                         const uint8_t *parity_check_bit_pattern,
                         uint16_t N,
                         uint8_t n_pc)
{
  int N_array = N >> 6;
  int k = 0;

  if (n_pc > 0) {
    uint64_t y0 = 0, y1 = 0, y2 = 0, y3 = 0, y4 = 0;

    for (int n = 0; n < N; n++) {
      uint64_t yt = y0;
      y0 = y1;
      y1 = y2;
      y2 = y3;
      y3 = y4;
      y4 = yt;

      if (information_bit_pattern[n] == 1) {
        int n1 = n >> 6;
        int n2 = n - (n1 << 6);
        if (parity_check_bit_pattern[n] == 1) {
          u[N_array - 1 - n1] |= y0 << (63 - n2);
        } else {
          int k1 = k >> 6;
          int k2 = k - (k1 << 6);
          uint64_t bit = (Cprime[k1] >> k2) & 1;
          u[N_array - 1 - n1] |= bit << (63 - n2);
          k++;
          y0 = y0 ^ (int)bit;
        }
      }
    }

  } else {
    for (int n = 0; n < N; n++) {
      if (information_bit_pattern[n] == 1) {
        int n1 = n >> 6;
        int n2 = n - (n1 << 6);
        int k1 = k >> 6;
        int k2 = k - (k1 << 6);
        uint64_t bit = (Cprime[k1] >> k2) & 1;
        u[N_array - 1 - n1] |= bit << (63 - n2);
        k++;
      }
    }
  }
}

void nr_polar_info_extraction_from_u(uint64_t *Cprime,
                                     const uint8_t *u,
                                     const uint8_t *information_bit_pattern,
                                     const uint8_t *parity_check_bit_pattern,
                                     uint16_t N,
                                     uint8_t n_pc)
{
  int k = 0;

  if (n_pc > 0) {
    for (int n = 0; n < N; n++) {
      if (information_bit_pattern[n] == 1) {
        if (parity_check_bit_pattern[n] == 0) {
          int k1 = k >> 6;
          int k2 = k - (k1 << 6);
          Cprime[k1] |= (uint64_t)u[n] << k2;
          k++;
        }
      }
    }

  } else {
    for (int n = 0; n < N; n++) {
      if (information_bit_pattern[n] == 1) {
        int k1 = k >> 6;
        int k2 = k - (k1 << 6);
        Cprime[k1] |= (uint64_t)u[n] << k2;
        k++;
      }
    }
  }
}

void nr_polar_uxG(uint64_t *D, const uint64_t *u, const fourDimArray_t *G_N_tab, uint16_t N)
{
  const int N64 = N / 64;
  cast2Darray(g_n, uint64_t, G_N_tab);
  for (int n = 0; n < N; n++) {
    const uint64_t *Gn = g_n[N - 1 - n];

    int n_ones = 0;
    for (int a = 0; a < N64; a++)
      n_ones += count_bits_set(u[a] & Gn[a]);

    int n1 = n / 64;
    int n2 = n - (n1 * 64);
    D[n1] |= ((uint64_t)n_ones & 1) << n2;
  }
}

void nr_polar_bit_insertion(uint8_t *input,
			    uint8_t *output,
			    uint16_t N,
			    uint16_t K,
			    int16_t *Q_I_N,
			    int16_t *Q_PC_N,
			    uint8_t n_PC)
{
  int k = 0;

  if (n_PC>0) {
    /*
     *
     */
  } else {
    for (int n=0; n<=N-1; n++) {
      output[n] = 0;
      for (int m=0; m<=(K+n_PC)-1; m++) {
	if ( n == Q_I_N[m]) {
          output[n] = input[k];
          k++;
          break;
  }
      }
    }
  }
}


uint32_t nr_polar_output_length(uint16_t K,
				uint16_t E,
				uint8_t n_max)
{
  int n_1, n_2, n_min = 5;
  double R_min=1.0/8;
  
  if ( (E <= (9.0/8)*pow(2,ceil(log2(E))-1)) && (K/E < 9.0/16) ) {
    n_1 = ceil(log2(E))-1;
  } else {
    n_1 = ceil(log2(E));
  }
  
  n_2 = ceil(log2(K/R_min));

  int n = n_max;
  if (n>n_1) n=n_1;
  if (n>n_2) n=n_2;
  if (n<n_min) n=n_min;

  /*printf("nr_polar_output_length: K %d, E %d, n %d (n_max %d,n_min %d, n_1 %d,n_2 %d)\n",
	 K,E,n,n_max,n_min,n_1,n_2);
	 exit(-1);*/
  return ((uint32_t) pow(2.0,n)); //=polar_code_output_length
}

void nr_polar_channel_interleaver_pattern(uint16_t *cip, const uint8_t I_BIL, const uint16_t E)
{
  if (I_BIL == 1) {
    int T = E;
    while( ((T/2)*(T+1)) < E ) T++;

    int16_t v[T][T];
    int k = 0;
    for (int i = 0; i <= T-1; i++) {
      for (int j = 0; j <= (T-1)-i; j++) {
	if (k<E) {
	  v[i][j] = k;
	} else {
	  v[i][j] = (-1);
	}
	k++;
      }
    }
    
    k=0;
    for (int j = 0; j <= T-1; j++) {
      for (int i = 0; i <= (T-1)-j; i++) {
	if ( v[i][j] != (-1) ) {
	  cip[k]=v[i][j];
	  k++;
	}
      }
    }
  } else {
    for (int i=0; i<=E-1; i++) cip[i]=i;
  }
}

void nr_polar_info_bit_pattern(uint8_t *ibp,
                               uint8_t *pcbp,
                               int16_t *Q_I_N,
                               int16_t *Q_F_N,
                               int16_t *Q_PC_N,
                               const uint16_t *J,
                               const uint16_t *Q_0_Nminus1,
                               uint16_t K,
                               uint16_t N,
                               const uint16_t E,
                               uint8_t n_PC,
                               uint8_t n_pc_wm)
{
  int Q_Ftmp_N[N + 1]; // Last element shows the final
  int Q_Itmp_N[N + 1]; // array index assigned a value.

  for (int i = 0; i <= N; i++) {
    Q_Ftmp_N[i] = -1; // Empty array.
    Q_Itmp_N[i] = -1;
  }

  int limit;

  if (E < N) {
    if ((K / (double)E) <= (7.0 / 16)) { // puncturing
      for (int n = 0; n <= N - E - 1; n++) {
  int ind = Q_Ftmp_N[N] + 1;
  Q_Ftmp_N[ind] = J[n];
  Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
      }

      if ((E / (double)N) >= (3.0 / 4)) {
        limit = ceil((double)(3 * N - 2 * E) / 4);
        for (int n = 0; n <= limit - 1; n++) {
    int ind = Q_Ftmp_N[N] + 1;
    Q_Ftmp_N[ind] = n;
    Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
        }
      } else {
        limit = ceil((double)(9 * N - 4 * E) / 16);
        for (int n = 0; n <= limit - 1; n++) {
    int ind = Q_Ftmp_N[N] + 1;
    Q_Ftmp_N[ind] = n;
    Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
        }
      }
    } else { // shortening
      for (int n = E; n <= N - 1; n++) {
        int ind = Q_Ftmp_N[N] + 1;
        Q_Ftmp_N[ind] = J[n];
        Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
      }
    }
  }

  // Q_I,tmp_N = Q_0_N-1 \ Q_F,tmp_N
  for (int n = 0; n <= N - 1; n++) {
    bool flag = true;
    for (int m = 0; m <= Q_Ftmp_N[N]; m++)
      if (Q_0_Nminus1[n] == Q_Ftmp_N[m]) {
        flag = false;
        break;
      }
    if (flag) {
      Q_Itmp_N[Q_Itmp_N[N] + 1] = Q_0_Nminus1[n];
      Q_Itmp_N[N]++;
    }
  }

  // Q_I_N comprises (K+n_PC) most reliable bit indices in Q_I,tmp_N
  for (int n = 0; n <= (K + n_PC) - 1; n++) {
    int ind = Q_Itmp_N[N] + n - ((K + n_PC) - 1);
    Q_I_N[n] = Q_Itmp_N[ind];
  }

  if (n_PC > 0) {
    AssertFatal(n_pc_wm == 0, "Q_PC_N computation for n_pc_wm = %i is not implemented yet!\n", n_pc_wm);
    for (int n = 0; n < n_PC - n_pc_wm; n++) {
      Q_PC_N[n] = Q_I_N[n];
    }
  }

  // Q_F_N = Q_0_N-1 \ Q_I_N
  for (int n = 0; n <= N - 1; n++) {
    bool flag = true;
    for (int m = 0; m <= (K + n_PC) - 1; m++)
      if (Q_0_Nminus1[n] == Q_I_N[m]) {
        flag = false;
        break;
      }
    if (flag) {
      Q_F_N[Q_F_N[N] + 1] = Q_0_Nminus1[n];
      Q_F_N[N]++;
    }
  }

  // Information and Parity Chack Bit Pattern
  for (int n = 0; n <= N - 1; n++) {

    ibp[n] = 0;
    for (int m = 0; m <= (K + n_PC) - 1; m++) {
      if (n == Q_I_N[m]) {
        ibp[n] = 1;
        break;
      }
    }

    pcbp[n] = 0;
    for (int m = 0; m < n_PC - n_pc_wm; m++) {
      if (n == Q_PC_N[m]) {
        pcbp[n] = 1;
        break;
      }
    }
  }
}


void nr_polar_info_bit_extraction(uint8_t *input,
				  uint8_t *output,
				  uint8_t *pattern,
				  uint16_t size)
{
  uint16_t j = 0;
  for (int i = 0; i < size; i++) {
    if (pattern[i] > 0) {
      output[j] = input[i];
      j++;
    }
  }
}


void nr_polar_rate_matching_pattern(uint16_t *rmp,
				    uint16_t *J,
				    const uint8_t *P_i_,
				    uint16_t K,
				    uint16_t N,
				    uint16_t E)
{
  uint16_t d[N];
  for (int m = 0; m < N; m++)
    d[m] = m;
  uint16_t y[N];
  memset(y, 0, sizeof(y));

  for (int m=0; m<=N-1; m++){
    int i = floor((32 * m) / N);
    J[m] = (P_i_[i]*(N/32)) + (m%(N/32));
    y[m] = d[J[m]];
  }

  if (E>=N) { //repetition
    for (int k=0; k<=E-1; k++) {
      int ind = (k % N);
      rmp[k]=y[ind];
    }
  } else {
    if ( (K/(double)E) <= (7.0/16) ) { //puncturing
      for (int k=0; k<=E-1; k++) {
        rmp[k]=y[k+N-E];
      }
    } else { //shortening
      for (int k=0; k<=E-1; k++) {
        rmp[k]=y[k];
      }
    }
  }
}


void nr_polar_rate_matching(double *input,
			    double *output,
			    uint16_t *rmp,
			    uint16_t K,
			    uint16_t N,
			    uint16_t E)
{
  if (E>=N) { //repetition
    for (int i=0; i<=N-1; i++) output[i]=0;
    for (int i=0; i<=E-1; i++){
      output[rmp[i]]+=input[i];
    }
  } else {
    if ( (K/(double)E) <= (7.0/16) ) { //puncturing
      for (int i=0; i<=N-1; i++) output[i]=0;
    } else { //shortening
      for (int i=0; i<=N-1; i++) output[i]=INFINITY;
    }

    for (int i=0; i<=E-1; i++){
      output[rmp[i]]=input[i];
    }
  }
}

/*
 * De-interleaving of coded bits implementation
 * TS 138.212: Section 5.4.1.3 - Interleaving of coded bits
 */
void nr_polar_rm_deinterleaving_cb(const int16_t *in, int16_t *out, const uint16_t E)
{
  int T = ceil((sqrt(8 * E + 1) - 1) / 2);
  int v_tab[T][T];
  memset(v_tab, 0, sizeof(v_tab));
  int k = 0;
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < T - i; j++) {
      if (k < E) {
        v_tab[i][j] = k + 1;
      }
      k++;
    }
  }

  int v[T][T];
  k = 0;
  for (int j = 0; j < T; j++) {
    for (int i = 0; i < T - j; i++) {
      if (k < E && v_tab[i][j] != 0) {
        v[i][j] = in[k];
        k++;
      } else {
        v[i][j] = INT_MAX;
      }
    }
  }

  k = 0;
  memset(out, 0, E * sizeof(*out));
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < T - i; j++) {
      if (v[i][j] != INT_MAX) {
        out[k] = v[i][j];
        k++;
      }
    }
  }
}

void nr_polar_rate_matching_int16(int16_t *input,
                                  int16_t *output,
                                  const uint16_t *rmp,
                                  const uint16_t K,
                                  const uint16_t N,
                                  const uint16_t E,
                                  const uint8_t i_bil)
{
  if (i_bil == 1) {
    nr_polar_rm_deinterleaving_cb(input, input, E);
  }

  if (E >= N) { // repetition
    memset(output, 0, N * sizeof(*output));
    for (int i = 0; i <= E - 1; i++)
      output[rmp[i]] += input[i];
  } else {
    if ((K / (double)E) <= (7.0 / 16))
      memset(output, 0, N * sizeof(*output)); // puncturing
    else { // shortening
      for (int i = 0; i <= N - 1; i++)
        output[i] = 32767; // instead of INFINITY, to prevent [-Woverflow]
    }

    for (int i = 0; i <= E - 1; i++)
      output[rmp[i]] = input[i];
  }
}
