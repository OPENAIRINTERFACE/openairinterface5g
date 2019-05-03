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

/*!\file PHY/CODING/nrPolar_tools/nr_polar_procedures.h
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

void nr_polar_bit_insertion(uint8_t *input,
			    uint8_t *output,
			    uint16_t N,
			    uint16_t K,
			    int16_t *Q_I_N,
			    int16_t *Q_PC_N,
			    uint8_t n_PC)
{
  uint16_t k=0;
  uint8_t flag;

  if (n_PC>0) {
    /*
     *
     */
  } else {
    for (int n=0; n<=N-1; n++) {
      flag=0;
      for (int m=0; m<=(K+n_PC)-1; m++) {
	if ( n == Q_I_N[m]) {
	  flag=1;
	  break;
	}
      }
      if (flag) { // n ϵ Q_I_N
	output[n]=input[k];
	k++;
      } else {
	output[n] = 0;
      }
    }
  }

}


uint32_t nr_polar_output_length(uint16_t K,
				uint16_t E,
				uint8_t n_max)
{
  uint8_t n_1, n_2, n_min=5, n;
  double R_min=1.0/8;
  
  if ( (E <= (9.0/8)*pow(2,ceil(log2(E))-1)) && (K/E < 9.0/16) ) {
    n_1 = ceil(log2(E))-1;
  } else {
    n_1 = ceil(log2(E));
  }
  
  n_2 = ceil(log2(K/R_min));
  
  n=n_max;
  if (n>n_1) n=n_1;
  if (n>n_2) n=n_2;
  if (n<n_min) n=n_min;

  /*printf("nr_polar_output_length: K %d, E %d, n %d (n_max %d,n_min %d, n_1 %d,n_2 %d)\n",
	 K,E,n,n_max,n_min,n_1,n_2);
	 exit(-1);*/
  return ((uint32_t) pow(2.0,n)); //=polar_code_output_length
}


void nr_polar_channel_interleaver_pattern(uint16_t *cip,
					  uint8_t I_BIL,
					  uint16_t E)
{
  if (I_BIL == 1) {
    uint16_t T=0, k;
    while( ((T/2)*(T+1)) < E ) T++;
    
    int16_t **v = malloc(T * sizeof(*v));
    for (int i = 0; i <= T-1; i++) v[i] = malloc((T-i) * sizeof(*(v[i])));
    
    k=0;
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
    
    for (int i = 0; i <= T-1; i++) free(v[i]);
    free(v);
    
  } else {
    for (int i=0; i<=E-1; i++) cip[i]=i;
  }
}


void nr_polar_info_bit_pattern(uint8_t *ibp,
			       int16_t *Q_I_N,
			       int16_t *Q_F_N,
			       uint16_t *J,
			       const uint16_t *Q_0_Nminus1,
			       uint16_t K,
			       uint16_t N,
			       uint16_t E,
			       uint8_t n_PC)
{
  int16_t *Q_Ftmp_N = malloc(sizeof(int16_t) * (N + 1)); // Last element shows the final
  int16_t *Q_Itmp_N = malloc(sizeof(int16_t) * (N + 1)); // array index assigned a value.

  for (int i = 0; i <= N; i++) {
    Q_Ftmp_N[i] = -1; // Empty array.
    Q_Itmp_N[i] = -1;
  }

  uint8_t flag;
  uint16_t limit, ind;

  if (E < N) {
    if ((K / (double) E) <= (7.0 / 16)) { //puncturing
      for (int n = 0; n <= N - E - 1; n++) {
	ind = Q_Ftmp_N[N] + 1;
	Q_Ftmp_N[ind] = J[n];
	Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
      }

      if ((E / (double) N) >= (3.0 / 4)) {
	limit = ceil((double) (3 * N - 2 * E) / 4);
	for (int n = 0; n <= limit - 1; n++) {
	  ind = Q_Ftmp_N[N] + 1;
	  Q_Ftmp_N[ind] = n;
	  Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
	}
      } else {
	limit = ceil((double) (9 * N - 4 * E) / 16);
	for (int n = 0; n <= limit - 1; n++) {
	  ind = Q_Ftmp_N[N] + 1;
	  Q_Ftmp_N[ind] = n;
	  Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
	}
      }
    } else { //shortening
      for (int n = E; n <= N - 1; n++) {
	ind = Q_Ftmp_N[N] + 1;
	Q_Ftmp_N[ind] = J[n];
	Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
      }
    }
  }

  //Q_I,tmp_N = Q_0_N-1 \ Q_F,tmp_N
  for (int n = 0; n <= N - 1; n++) {
    flag = 1;
    for (int m = 0; m <= Q_Ftmp_N[N]; m++) {
      if (Q_0_Nminus1[n] == Q_Ftmp_N[m]) {
	flag = 0;
	break;
      }
    }
    if (flag) {
      Q_Itmp_N[Q_Itmp_N[N] + 1] = Q_0_Nminus1[n];
      Q_Itmp_N[N]++;
    }
  }

  //Q_I_N comprises (K+n_PC) most reliable bit indices in Q_I,tmp_N
  for (int n = 0; n <= (K + n_PC) - 1; n++) {
    ind = Q_Itmp_N[N] + n - ((K + n_PC) - 1);
    Q_I_N[n] = Q_Itmp_N[ind];
  }

  //Q_F_N = Q_0_N-1 \ Q_I_N
  for (int n = 0; n <= N - 1; n++) {
    flag = 1;
    for (int m = 0; m <= (K + n_PC) - 1; m++) {
      if (Q_0_Nminus1[n] == Q_I_N[m]) {
	flag = 0;
	break;
      }
    }
    if (flag) {
      Q_F_N[Q_F_N[N] + 1] = Q_0_Nminus1[n];
      Q_F_N[N]++;
    }
  }

  //Information Bit Pattern
  for (int n = 0; n <= N - 1; n++) {
    ibp[n] = 0;

    for (int m = 0; m <= (K + n_PC) - 1; m++) {
      if (n == Q_I_N[m]) {
	ibp[n] = 1;
	break;
      }
    }
  }

  free(Q_Ftmp_N);
  free(Q_Itmp_N);
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
  uint8_t i;
  uint16_t *d, *y, ind;
  d = (uint16_t *)malloc(sizeof(uint16_t) * N);
  y = (uint16_t *)malloc(sizeof(uint16_t) * N);

  for (int m=0; m<=N-1; m++) d[m]=m;

  for (int m=0; m<=N-1; m++){
    i=floor((32*m)/N);
    J[m] = (P_i_[i]*(N/32)) + (m%(N/32));
    y[m] = d[J[m]];
  }

  if (E>=N) { //repetition
    for (int k=0; k<=E-1; k++) {
      ind = (k%N);
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

  free(d);
  free(y);
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

void nr_polar_rate_matching_int16(int16_t *input,
				  int16_t *output,
				  uint16_t *rmp,
				  uint16_t K,
				  uint16_t N,
				  uint16_t E)
{
  if (E>=N) { //repetition
    memset((void*)output,0,N*sizeof(int16_t));
    for (int i=0; i<=E-1; i++) output[rmp[i]]+=input[i];
  } else {
    if ( (K/(double)E) <= (7.0/16) ) memset((void*)output,0,N*sizeof(int16_t)); //puncturing
    else { //shortening
      for (int i=0; i<=N-1; i++) output[i]=32767;//instead of INFINITY, to prevent [-Woverflow]
    }

    for (int i=0; i<=E-1; i++) output[rmp[i]]=input[i];
   
  }
}
