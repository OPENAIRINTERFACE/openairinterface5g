/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include "sim.h"

void tv_channel(channel_desc_t *desc,double complex ***H,uint32_t length);
double frand_a_b(double a, double b);
void tv_conv(double complex **h, double complex *x, double complex *y, uint32_t nb_samples, uint8_t nb_taps, uint64_t delay);

void multipath_tv_channel(channel_desc_t *desc,
                          double **tx_sig_re,
                          double **tx_sig_im,
                          double **rx_sig_re,
                          double **rx_sig_im,
                          uint32_t length,
                          uint8_t keep_channel)
{

  double complex **tx,**rx,***H_t;
  double path_gain = pow(10, -desc->path_loss_dB / 20);
  int i,j;
  uint64_t dd = desc->channel_offset;
#ifdef DEBUG_CH
  printf("[TV CHANNEL] keep = %d : path_gain = %g (path_loss_dB %f), nb_rx %d, nb_tx %d, dd %lu, len %d max_doppler %g\n",keep_channel,path_gain,desc->path_loss_dB,desc->nb_rx,desc->nb_tx,dd,desc->channel_length,
         desc->max_Doppler);
#endif
  tx = (double complex **)malloc(desc->nb_tx*sizeof(double complex *));
  rx = (double complex **)malloc(desc->nb_rx*sizeof(double complex *));
  H_t= (double complex ***) malloc(desc->nb_tx*desc->nb_rx*sizeof(double complex **));

  for(i=0; i<desc->nb_tx; i++) {
    tx[i] = (double complex *)calloc(length,sizeof(double complex));
  }

  for(i=0; i<desc->nb_rx; i++) {
    rx[i] = (double complex *)calloc(length,sizeof(double complex));
  }

  for(i=0; i<desc->nb_tx*desc->nb_rx; i++) {
    H_t[i] = (double complex **)malloc(desc->nb_taps*sizeof(double complex *));

    for(j=0; j<desc->nb_taps; j++) {
      H_t[i][j] = (double complex *)calloc(length,sizeof(double complex));
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
      tv_conv(H_t[i+(j*desc->nb_rx)],tx[j],rx[i],length,desc->nb_taps,dd);
    }
  }

  for(i=0; i<desc->nb_rx; i++) {
    for(j=0; j<length; j++) {
      rx_sig_re[i][j] = creal(rx[i][j])*path_gain;
      rx_sig_im[i][j] = cimag(rx[i][j])*path_gain;
    }
  }

  for(i=0; i<desc->nb_tx; i++) {
    free(tx[i]);
  }

  free(tx);

  for(i=0; i<desc->nb_rx; i++) {
    free(rx[i]);
  }

  free(rx);

  for(i=0; i<desc->nb_rx*desc->nb_tx; i++) {
    for(j=0; j<desc->nb_taps; j++) {
      free(H_t[i][j]);
    }

    free(H_t[i]);
  }

  free(H_t);
}

//TODO: make phi_rad a parameter of this function
void tv_channel(channel_desc_t *desc,double complex ***H,uint32_t length){

  int i,j,p,l,k;
  double *alpha,***phi_rad,pi=acos(-1),***w_Hz;
  alpha = (double *)calloc(desc->nb_paths,sizeof(double));
  phi_rad = (double ***)malloc(desc->nb_rx*desc->nb_tx*sizeof(double **));
  w_Hz = (double ***)malloc(desc->nb_rx*desc->nb_tx*sizeof(double **));

  for(i=0; i<desc->nb_tx*desc->nb_rx; i++) {
    phi_rad[i]   = (double **) malloc(desc->nb_taps*sizeof(double *));
    w_Hz[i]      = (double **) malloc(desc->nb_taps*sizeof(double *));
    for(j=0; j<desc->nb_taps; j++) {
      phi_rad[i][j]   = (double *) malloc(desc->nb_paths*sizeof(double));
      w_Hz[i][j]      = (double *) malloc(desc->nb_paths*sizeof(double));
    }
  }

  for(i=0; i<desc->nb_tx*desc->nb_rx; i++) {
    for (j = 0; j<desc->nb_taps; j++) {
      for(k=0; k<desc->nb_paths; k++) {
        w_Hz[i][j][k] = desc->max_Doppler*cos(frand_a_b(0,2*M_PI));
        phi_rad[i][j][k] = frand_a_b(0,2*M_PI);
        //printf("w_hz[%d][%d][%d]=f_d*cos(theta) = %f\n",i,j,k,w_Hz[i][j][k]);
        //printf("phi_rad[%d][%d][%d] = %f\n",i,j,k,phi_rad[i][j][k]);
        }
    }
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

  // SISO or MIMO
  for (i=0; i<desc->nb_rx; i++) {
    for(j=0; j<desc->nb_tx; j++) {
      for(k=0; k<desc->nb_taps; k++) {
        for(l=0; l<length; l++) {
          for(p=0; p<desc->nb_paths; p++) {
            H[i+(j*desc->nb_rx)][k][l] += sqrt(desc->amps[k])*alpha[p]*cexp(I*(2*pi*w_Hz[i+(j*desc->nb_rx)][k][p]*l*(1/(desc->sampling_rate*1e6))+phi_rad[i+(j*desc->nb_rx)][k][p]));
          }
        }
        //printf("H[tx%d][rx%d][k%d][l%d] = %f+j%f \n",j,i,k,0,creal(H[i+(j*desc->nb_rx)][k][0]),cimag(H[i+(j*desc->nb_rx)][k][0]));
      }
    }
  }
  //accumlate the phase
  /*for(k=0; k<desc->nb_taps; k++) {
   * for(j=0; j<desc->nb_paths; j++) {
   * desc->random_phase[k][j] = fmod(2*pi*w_Hz[k][j]*(length-1)*(1/(desc->sampling_rate*1e6))+phi_rad[k][j],2*pi);
   * }
   * }*/
  free(alpha);
  for(i=0; i<desc->nb_rx*desc->nb_tx; i++) {
    for (j=0; j<desc->nb_taps; j++) {
      free(w_Hz[i][j]);
      free(phi_rad[i][j]);
    }
    free(w_Hz[i]);
    free(phi_rad[i]);
  }
  free(w_Hz);
  free(phi_rad);
}

// time varying convolution
void tv_conv(double complex **h, double complex *x, double complex *y, uint32_t nb_samples, uint8_t nb_taps, uint64_t dd)
{
  for(int i = 0; i < ((int)nb_samples-dd); i++) {
    for(int j = 0; j < nb_taps; j++) {
      if(i >= j)
        y[i+dd] += h[j][i] * x[i-j];
    }
  }
}

double frand_a_b(double a, double b) {
  return (rand()/(double)RAND_MAX)*(b-a)+a;
}
