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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sim.h"
#include "SIMULATION/RF/rf.h"
#include <complex.h>

void tv_channel(channel_desc_t *desc,double complex ***H,uint16_t length);
double frand_a_b(double a, double b);
void tv_conv(double complex **h, double complex *x, double complex *y, uint16_t nb_samples, uint8_t nb_taps, int delay);

void multipath_tv_channel(channel_desc_t *desc,
                          double **tx_sig_re,
                          double **tx_sig_im,
                          double **rx_sig_re,
                          double **rx_sig_im,
                          uint16_t length,
                          uint8_t keep_channel) {
  double complex **tx,**rx,* **H_t,*rx_temp; //, *tv_H_t;
  double path_loss = pow(10,desc->path_loss_dB/20);
  int i,j,k,dd;
  dd = abs(desc->channel_offset);
#ifdef DEBUG_CH
  printf("[TV CHANNEL] keep = %d : path_loss = %g (%f), nb_rx %d, nb_tx %d, dd %d, len %d max_doppler %g\n",keep_channel,path_loss,desc->path_loss_dB,desc->nb_rx,desc->nb_tx,dd,desc->channel_length,
         desc->max_Doppler);
#endif
  tx = (double complex **)malloc(desc->nb_tx*sizeof(double complex));
  rx = (double complex **)malloc(desc->nb_rx*sizeof(double complex));
  H_t= (double complex ** *) malloc(desc->nb_tx*desc->nb_rx*sizeof(double complex **));
  //  tv_H_t = (double complex *) malloc(length*sizeof(double complex));
  rx_temp= (double complex *) calloc(length,sizeof(double complex));

  for(i=0; i<desc->nb_tx; i++) {
    tx[i] = (double complex *)calloc(length,sizeof(double complex));
  }

  for(i=0; i<desc->nb_rx; i++) {
    rx[i] = (double complex *)calloc(length,sizeof(double complex));
  }

  for(i=0; i<desc->nb_tx*desc->nb_rx; i++) {
    H_t[i] = (double complex **) malloc(length*sizeof(double complex *));

    for(j=0; j<length; j++) {
      H_t[i][j] = (double complex *) calloc (desc->nb_taps,sizeof(double complex));
    }
  }

  for (i=0; i<desc->nb_tx; i++) {
    for(j=0; j<length; j++) {
      tx[i][j] = tx_sig_re[i][j] +I*tx_sig_im[i][j];
    }
  }

  if (keep_channel) {
    // do nothing - keep channel
  } else {
    tv_channel(desc,H_t,length);
  }

  for(i=0; i<desc->nb_rx; i++) {
    for(j=0; j<desc->nb_tx; j++) {
      tv_conv(H_t[i+(j*desc->nb_rx)],tx[j],rx_temp,length,desc->nb_taps,dd);

      for(k=0; k<length; k++) {
        rx[i][k] += rx_temp[k];
      }
    }
  }

  for(i=0; i<desc->nb_rx; i++) {
    for(j=0; j<length; j++) {
      rx_sig_re[i][j] = creal(rx[i][j])*path_loss;
      rx_sig_im[i][j] = cimag(rx[i][j])*path_loss;
    }
  }

  /*  for(k=0;k<length;k++) {
      tv_H_t[k] = H_t[0][k][0];
      }*/

  for(i=0; i<desc->nb_tx; i++) {
    free(tx[i]);
  }

  free(tx);

  for(i=0; i<desc->nb_rx; i++) {
    free(rx[i]);
  }

  free(rx);

  for(i=0; i<desc->nb_rx*desc->nb_tx; i++) {
    for(j=0; j<length; j++) {
      free(H_t[i][j]);
    }

    free(H_t[i]);
  }

  free(H_t);
  free(rx_temp);
}

//TODO: make phi_rad a parameter of this function
void tv_channel(channel_desc_t *desc,double complex ***H,uint16_t length) {
  int i,j,p,l,k;
  double *alpha,*phi_rad,pi=acos(-1),*w_Hz;
  alpha = (double *)calloc(desc->nb_paths,sizeof(double));
  phi_rad = (double *)calloc(desc->nb_paths,sizeof(double));
  w_Hz = (double *)calloc(desc->nb_paths,sizeof(double));

  for(i=0; i<desc->nb_paths; i++) {
    w_Hz[i]=desc->max_Doppler*cos(frand_a_b(0,2*pi));
    phi_rad[i]=frand_a_b(0,2*pi);
  }

  if(desc->ricean_factor == 1) {
    for(i=0; i<desc->nb_paths; i++) {
      alpha[i]=1/sqrt(desc->nb_paths);
    }
  } else {
    alpha[0]=sqrt(desc->ricean_factor/(desc->ricean_factor+1));

    for(i=1; i<desc->nb_paths; i++) {
      alpha[i] = (1/sqrt(desc->nb_paths-1))*(sqrt(1/(desc->ricean_factor+1)));
    }
  }

  /*
  // This is the code when we only consider a SISO case
  for(i=0;i<length;i++)
  {
  for(j=0;j<desc->nb_taps;j++)
     {
    for(p=0;p<desc->nb_paths;p++)
       {
         H[i][j] += sqrt(desc->amps[j]/2)*alpha[p]*cexp(-I*(2*pi*w_Hz[p]*i*(1/(desc->sampling_rate*1e6))+phi_rad[p]));
       }
       }
   }
  for(j=0;j<desc->nb_paths;j++)
   {
  phi_rad[j] = fmod(2*pi*w_Hz[j]*(length-1)*(1/desc->sampling_rate)+phi_rad[j],2*pi);
   }
  */

  // if MIMO
  for (i=0; i<desc->nb_rx; i++) {
    for(j=0; j<desc->nb_tx; j++) {
      for(k=0; k<length; k++) {
        for(l=0; l<desc->nb_taps; l++) {
          H[i+(j*desc->nb_rx)][k][l] = 0;

          for(p=0; p<desc->nb_paths; p++) {
            H[i+(j*desc->nb_rx)][k][l] += sqrt(desc->amps[l]/2)*alpha[p]*cexp(I*(2*pi*w_Hz[p]*k*(1/(desc->sampling_rate*1e6))+phi_rad[p]));
          }
        }
      }

      for(j=0; j<desc->nb_paths; j++) {
        phi_rad[j] = fmod(2*pi*w_Hz[j]*(length-1)*(1/desc->sampling_rate)+phi_rad[j],2*pi);
      }
    }
  }

  free(alpha);
  free(w_Hz);
  free(phi_rad);
}

// time varying convolution
void tv_conv(double complex **h, double complex *x, double complex *y, uint16_t nb_samples, uint8_t nb_taps, int dd) {
  int i,j;

  for(i=0; i<((int)nb_samples-dd); i++) {
    for(j=0; j<nb_taps; j++) {
      if(i>j)
        y[i+dd] += creal(h[i][j])*creal(x[i-j])-cimag(h[i][j])*cimag(x[i-j]) + I*(creal(h[i][j])*cimag(x[i-j])+cimag(h[i][j])*creal(x[i-j]));
    }
  }
}

double frand_a_b(double a, double b) {
  return (rand()/(double)RAND_MAX)*(b-a)+a;
}
