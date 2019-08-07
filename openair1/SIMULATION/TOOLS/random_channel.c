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

#include <math.h>
#include <cblas.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "PHY/TOOLS/tools_defs.h"
#include "sim.h"
#include "scm_corrmat.h"
#include "common/utils/LOG/log.h"
#include "common/config/config_userapi.h"
#include "common/utils/telnetsrv/telnetsrv.h"
#include "common/utils/load_module_shlib.h"


//#define DEBUG_CH

#include "assertions.h"

extern void print_shorts(char *s,__m128i *x);
static mapping channelmod_names[] = {
  CHANNELMOD_MAP_INIT
};

static int channelmod_show_cmd(char *buff, int debug, telnet_printfunc_t prnt);
static telnetshell_cmddef_t channelmod_cmdarray[] = {
  {"show","",channelmod_show_cmd},
  {"","",NULL},
};

static telnetshell_vardef_t channelmod_vardef[] = {
  {"",0,NULL}
};

static double snr_dB=25;
static double sinr_dB=0;

void fill_channel_desc(channel_desc_t *chan_desc,
                       uint8_t nb_tx,
                       uint8_t nb_rx,
                       uint8_t nb_taps,
                       uint8_t channel_length,
                       double *amps,
                       double *delays,
                       struct complex **R_sqrt,
                       double Td,
                       double sampling_rate,
                       double channel_bandwidth,
                       double ricean_factor,
                       double aoa,
                       double forgetting_factor,
                       double max_Doppler,
                       int32_t channel_offset,
                       double path_loss_dB,
                       uint8_t random_aoa) {
  uint16_t i,j;
  double delta_tau;
  LOG_I(OCM,"[CHANNEL] Getting new channel descriptor, nb_tx %d, nb_rx %d, nb_taps %d, channel_length %d\n",
        nb_tx,nb_rx,nb_taps,channel_length);
  chan_desc->nb_tx          = nb_tx;
  chan_desc->nb_rx          = nb_rx;
  chan_desc->nb_taps        = nb_taps;
  chan_desc->channel_length = channel_length;
  chan_desc->amps           = amps;
  LOG_D(OCM,"[CHANNEL] Doing delays ...\n");

  if (delays==NULL) {
    chan_desc->delays = (double *) malloc(nb_taps*sizeof(double));
    delta_tau = Td/nb_taps;

    for (i=0; i<nb_taps; i++)
      chan_desc->delays[i] = ((double)i)*delta_tau;
  } else
    chan_desc->delays              = delays;

  chan_desc->Td                         = Td;
  chan_desc->sampling_rate              = sampling_rate;
  chan_desc->channel_bandwidth          = channel_bandwidth;
  chan_desc->ricean_factor              = ricean_factor;
  chan_desc->aoa                        = aoa;
  chan_desc->random_aoa                 = random_aoa;
  chan_desc->forgetting_factor          = forgetting_factor;
  chan_desc->channel_offset             = channel_offset;
  chan_desc->path_loss_dB               = path_loss_dB;
  chan_desc->first_run                  = 1;
  chan_desc->ip                         = 0.0;
  chan_desc->max_Doppler                = max_Doppler;
  chan_desc->ch                         = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
  chan_desc->chF                        = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
  chan_desc->a                          = (struct complex **) malloc(nb_taps*sizeof(struct complex *));
  LOG_D(OCM,"[CHANNEL] Filling ch \n");

  for (i = 0; i<nb_tx*nb_rx; i++)
    chan_desc->ch[i] = (struct complex *) malloc(channel_length * sizeof(struct complex));

  for (i = 0; i<nb_tx*nb_rx; i++)
    chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex)); // allocate for up to 100 RBs, 12 samples per RB

  LOG_D(OCM,"[CHANNEL] Filling a (nb_taps %d)\n",nb_taps);

  for (i = 0; i<nb_taps; i++) {
    LOG_D(OCM,"tap %d (%p,%zu)\n",i,&chan_desc->a[i],nb_tx*nb_rx * sizeof(struct complex));
    chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));
  }

  LOG_D(OCM,"[CHANNEL] Doing R_sqrt ...\n");

  if (R_sqrt == NULL) {
    chan_desc->R_sqrt         = (struct complex **) calloc(nb_taps,sizeof(struct complex *));

    for (i = 0; i<nb_taps; i++) {
      chan_desc->R_sqrt[i]    = (struct complex *) calloc(nb_tx*nb_rx*nb_tx*nb_rx,sizeof(struct complex));

      for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
        chan_desc->R_sqrt[i][j].x = 1.0;
        chan_desc->R_sqrt[i][j].y = 0.0;
      }
    }
  } else {
    chan_desc->R_sqrt = (struct complex **) calloc(nb_taps,sizeof(struct complex *));

    for (i = 0; i<nb_taps; i++) {
      //chan_desc->R_sqrt[i]    = (struct complex*) calloc(nb_tx*nb_rx*nb_tx*nb_rx,sizeof(struct complex));
      //chan_desc->R_sqrt = (struct complex*)&R_sqrt[i][0];
      /* all chan_desc share the same R_sqrt, coming from caller */
      chan_desc->R_sqrt[i] = R_sqrt[0];
    }
  }

  for (i = 0; i<nb_taps; i++) {
    for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
      LOG_D(OCM,"Rsqrt[%d][%d] %f %f\n",i,j,chan_desc->R_sqrt[i][j].x,chan_desc->R_sqrt[i][j].y);
    }
  }

  LOG_D(OCM,"[CHANNEL] RF %f\n",chan_desc->ricean_factor);

  for (i=0; i<chan_desc->nb_taps; i++)
    LOG_D(OCM,"[CHANNEL] tap %d: amp %f, delay %f\n",i,chan_desc->amps[i],chan_desc->delays[i]);

  chan_desc->nb_paths=10;
  reset_meas(&chan_desc->random_channel);
  reset_meas(&chan_desc->interp_time);
  reset_meas(&chan_desc->interp_freq);
  reset_meas(&chan_desc->convolution);
}

double mbsfn_delays[] = {0,.03,.15,.31,.37,1.09,12.490,12.52,12.64,12.80,12.86,13.58,27.49,27.52,27.64,27.80,27.86,28.58};
double mbsfn_amps_dB[] = {0,-1.5,-1.4,-3.6,-0.6,-7.0,-10,-11.5,-11.4,-13.6,-10.6,-17.0,-20,-21.5,-21.4,-23.6,-20.6,-27};

double scm_c_delays[] = {0, 0.0125, 0.0250, 0.3625, 0.3750, 0.3875, 0.2500, 0.2625, 0.2750, 1.0375, 1.0500, 1.0625, 2.7250, 2.7375, 2.7500, 4.6000, 4.6125, 4.6250};
double scm_c_amps_dB[] = {0.00, -2.22, -3.98, -1.86, -4.08, -5.84, -1.08, -3.30, -5.06, -9.08, -11.30, -13.06, -15.14, -17.36, -19.12, -20.64, -22.85, -24.62};

double epa_delays[] = { 0,.03,.07,.09,.11,.19,.41};
double epa_amps_dB[] = {0.0,-1.0,-2.0,-3.0,-8.0,-17.2,-20.8};

double eva_delays[] = { 0,.03,.15,.31,.37,.71,1.09,1.73,2.51};
double eva_amps_dB[] = {0.0,-1.5,-1.4,-3.6,-0.6,-9.1,-7.0,-12.0,-16.9};

double etu_delays[] = { 0,.05,.12,.2,.23,.5,1.6,2.3,5.0};
double etu_amps_dB[] = {-1.0,-1.0,-1.0,0.0,0.0,0.0,-3.0,-5.0,-7.0};

double default_amps_lin[] = {0.3868472, 0.3094778, 0.1547389, 0.0773694, 0.0386847, 0.0193424, 0.0096712, 0.0038685};
double default_amp_lin[] = {1};

double ts_shift_delays[] = {0, 1/7.68};
double ts_shift_amps[] = {0, 1};

//correlation matrix for a 2x2 channel with full Tx correlation
struct complex R_sqrt_22_corr_tap[16] = {{0.70711,0}, {0.0, 0.0}, {0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0}, {0.0, 0.0}, {0.70711,0},
  {0.70711,0}, {0.0, 0.0}, {0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0}, {0.0, 0.0}, {0.70711,0}
};
struct complex *R_sqrt_22_corr[1]     = {R_sqrt_22_corr_tap};

//correlation matrix for a fully correlated 2x1 channel (h1==h2)
struct complex R_sqrt_21_corr_tap[4]  = {{0.70711,0}, {0.70711,0}, {0.70711,0}, {0.70711,0}};
struct complex *R_sqrt_21_corr[1]      = {R_sqrt_21_corr_tap};

//correlation matrix for a 2x2 channel with full Tx anti-correlation
struct complex R_sqrt_22_anticorr_tap[16] = {{0.70711,0}, {0.0, 0.0}, {-0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0}, {0.0, 0.0}, {-0.70711,0},
  {-0.70711,0}, {0.0, 0.0}, {0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {-0.70711,0}, {0.0, 0.0}, {0.70711,0}
};
struct complex *R_sqrt_22_anticorr[1]     = {R_sqrt_22_anticorr_tap};

//correlation matrix for a fully anti-correlated 2x1 channel (h1==-h2)
struct complex R_sqrt_21_anticorr_tap[4]  = {{0.70711,0}, {-0.70711,0}, {-0.70711,0}, {0.70711,0}};
struct complex *R_sqrt_21_anticorr[1]     = {R_sqrt_21_anticorr_tap};

struct complex **R_sqrt_ptr2;

// full correlation matrix in vectorized form for 2x2 channel, where h1 is  perfectly orthogonal to h2

struct complex R_sqrt_22_orthogonal_tap[16] = {{0.70711,0.0}, {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0}, {0.0,0.0},
  {0.0,0.0}, {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0}, {0.70711,0.0}
};
struct complex *R_sqrt_22_orthogonal[1]     = {R_sqrt_22_orthogonal_tap};

// full correlation matrix for TM4 to make orthogonal effective channel




struct complex R_sqrt_22_orth_eff_ch_TM4_prec_real_tap[16] = {{0.70711,0.0}, {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0}, {-0.70711,0.0},
  {0.70711,0.0}, {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {-0.70711,0.0}, {0.0, 0.0}, {0.70711,0.0}
};
struct complex *R_sqrt_22_orth_eff_ch_TM4_prec_real[1]     = {R_sqrt_22_orth_eff_ch_TM4_prec_real_tap};




struct complex R_sqrt_22_orth_eff_ch_TM4_prec_imag_tap[16] = {{0.70711,0.0}, {0.0,0.0}, {0.0, -0.70711}, {0.0,0.0},
  {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0}, {0.0,0.70711},
  {0.0,-0.70711}, {0.0, 0.0}, {-0.70711,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.0,0.70711}, {0.0, 0.0}, {-0.70711,0.0}
};
struct complex *R_sqrt_22_orth_eff_ch_TM4_prec_imag[1]     = {R_sqrt_22_orth_eff_ch_TM4_prec_imag_tap};

//Correlation matrix for EPA channel
struct complex R_sqrt_22_EPA_low_tap[16] = {{1.0,0.0}, {0.0,0.0}, {0.0,0.0}, {0.0,0.0},
  {0.0,0.0}, {1.0,0.0}, {0.0,0.0}, {0.0,0.0},
  {0.0,0.0}, {0.0,0.0}, {1.0,0.0}, {0.0,0.0},
  {0.0,0.0}, {0.0,0.0}, {0.0,0.0}, {1.0,0.0}
};
struct complex *R_sqrt_22_EPA_low[1]     = {R_sqrt_22_EPA_low_tap};

struct complex R_sqrt_22_EPA_high_tap[16] = {
  {0.7179,0.0}, {0.4500,0.0}, {0.4500,0.0}, {0.2821,0.0},
  {0.4500,0.0}, {0.7179,0.0}, {0.2821,0.0}, {0.4500,0.0},
  {0.4500,0.0}, {0.2821,0.0}, {0.7179,0.0}, {0.4500,0.0},
  {0.2821,0.0}, {0.4500,0.0}, {0.4500,0.0}, {0.7179,0.0}
};
struct complex *R_sqrt_22_EPA_high[1]     = {R_sqrt_22_EPA_high_tap};

struct complex R_sqrt_22_EPA_medium_tap[16] = {{0.8375,0.0}, {0.5249,0.0}, {0.1286,0.0}, {0.0806,0.0},
  {0.5249,0.0}, {0.8375,0.0}, {0.0806,0.0}, {0.1286,0.0},
  {0.1286,0.0}, {0.0806,0.0}, {0.8375,0.0}, {0.5249,0.0},
  {0.0806,0.0}, {0.1286,0.0}, {0.5249,0.0}, {0.8375,0.0}
};
struct complex *R_sqrt_22_EPA_medium[1]     = {R_sqrt_22_EPA_medium_tap};



//Rayleigh1_orth_eff_ch_TM4

channel_desc_t *new_channel_desc_scm(uint8_t nb_tx,
                                     uint8_t nb_rx,
                                     SCM_t channel_model,
                                     double sampling_rate,
                                     double channel_bandwidth,
                                     double forgetting_factor,
                                     int32_t channel_offset,
                                     double path_loss_dB) {
  channel_desc_t *chan_desc = (channel_desc_t *)malloc(sizeof(channel_desc_t));
  uint16_t i,j;
  double sum_amps;
  double aoa,ricean_factor,Td,maxDoppler;
  int channel_length,nb_taps;
  chan_desc->nb_tx                      = nb_tx;
  chan_desc->nb_rx                      = nb_rx;
  chan_desc->sampling_rate              = sampling_rate;
  chan_desc->channel_bandwidth          = channel_bandwidth;
  chan_desc->forgetting_factor          = forgetting_factor;
  chan_desc->channel_offset             = channel_offset;
  chan_desc->path_loss_dB               = path_loss_dB;
  chan_desc->first_run                  = 1;
  chan_desc->ip                                 = 0.0;
  LOG_I(OCM,"Channel Model (inside of new_channel_desc_scm)=%d\n\n", channel_model);

  switch (channel_model) {
    case SCM_A:
      LOG_W(OCM,"channel model not yet supported\n");
      free(chan_desc);
      return(NULL);

    case SCM_B:
      LOG_W(OCM,"channel model not yet supported\n");
      free(chan_desc);
      return(NULL);

    case SCM_C:
      chan_desc->nb_taps        = 18;
      chan_desc->Td             = 4.625;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*scm_c_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = scm_c_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      chan_desc->R_sqrt  = (struct complex **) malloc(6*sizeof(struct complex **));

      if (nb_tx==2 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R22_sqrt[i][0];
      } else if (nb_tx==2 && nb_rx==1) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R21_sqrt[i][0];
      } else if (nb_tx==1 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R12_sqrt[i][0];
      } else {
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }

          LOG_W(OCM,"correlation matrix not implemented for nb_tx==%d and nb_rx==%d, using identity\n", nb_tx, nb_rx);
        }
      }

      break;

    case SCM_D:
      LOG_W(OCM,"This is not the real SCM-D model! It is just SCM-C with an additional Rice factor!\n");
      chan_desc->nb_taps        = 18;
      chan_desc->Td             = 4.625;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*scm_c_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = scm_c_delays;
      chan_desc->ricean_factor  = 0.1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      chan_desc->R_sqrt  = (struct complex **) malloc(6*sizeof(struct complex **));

      if (nb_tx==2 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R22_sqrt[i][0];
      } else if (nb_tx==2 && nb_rx==1) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R21_sqrt[i][0];
      } else if (nb_tx==1 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R12_sqrt[i][0];
      } else {
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }

          LOG_W(OCM,"correlation matrix not implemented for nb_tx==%d and nb_rx==%d, using identity\n", nb_tx, nb_rx);
        }
      }

      break;

    case EPA:
      chan_desc->nb_taps        = 7;
      chan_desc->Td             = .410;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*epa_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = epa_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complex **) malloc(6*sizeof(struct complex **));

        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R22_sqrt[i][0];
      } else {
        chan_desc->R_sqrt         = (struct complex **) malloc(6*sizeof(struct complex **));

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }

          LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
        }
      }

      break;

    case EPA_low:
      chan_desc->nb_taps        = 7;
      chan_desc->Td             = .410;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*epa_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = epa_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex **));

        for (i = 0; i<chan_desc->nb_taps; i++)
          chan_desc->R_sqrt[i] = R_sqrt_22_EPA_low[0];
      } else {
        printf("Correlation matrices are implemented for 2 x 2 only");
      }

      /*else {
        chan_desc->R_sqrt         = (struct complex**) malloc(6*sizeof(struct complex**));
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex*) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));
          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }
          LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
        }
      }*/
      break;

    case EPA_high:
      chan_desc->nb_taps        = 7;
      chan_desc->Td             = .410;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*epa_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = epa_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex **));

        for (i = 0; i<chan_desc->nb_taps; i++)
          chan_desc->R_sqrt[i] = R_sqrt_22_EPA_high[0];
      } else {
        printf("Correlation matrices are implemented for 2 x 2 only");
      }

      /*else {
        chan_desc->R_sqrt         = (struct complex**) malloc(6*sizeof(struct complex**));
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex*) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));
          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }
          LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
        }
      }*/
      break;

    case EPA_medium:
      chan_desc->nb_taps        = 7;
      chan_desc->Td             = .410;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*epa_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = epa_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex **));

        for (i = 0; i<chan_desc->nb_taps; i++)
          chan_desc->R_sqrt[i] = R_sqrt_22_EPA_medium[0];
      } else {
        printf("Correlation matrices are implemented for 2 x 2 only");
      }

      /*else {
        chan_desc->R_sqrt         = (struct complex**) malloc(6*sizeof(struct complex**));
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex*) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));
          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }
          LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
        }
      }*/
      break;

    case EVA:
      chan_desc->nb_taps        = 9;
      chan_desc->Td             = 2.51;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*eva_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = eva_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complex **) malloc(6*sizeof(struct complex **));

        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R22_sqrt[i][0];
      } else {
        chan_desc->R_sqrt         = (struct complex **) malloc(6*sizeof(struct complex **));

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }

          LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
        }
      }

      break;

    case ETU:
      chan_desc->nb_taps        = 9;
      chan_desc->Td             = 5.0;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*etu_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = etu_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complex **) malloc(6*sizeof(struct complex **));

        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complex *) &R22_sqrt[i][0];
      } else {
        chan_desc->R_sqrt         = (struct complex **) malloc(6*sizeof(struct complex **));

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complex *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].x = 1.0;
            chan_desc->R_sqrt[i][j].y = 0.0;
          }

          LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
        }
      }

      break;

    case MBSFN:
      chan_desc->nb_taps        = 18;
      chan_desc->Td             = 28.58;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

      for (i = 0; i<chan_desc->nb_taps; i++) {
        chan_desc->amps[i]      = pow(10,.1*mbsfn_amps_dB[i]);
        sum_amps += chan_desc->amps[i];
      }

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->amps[i] /= sum_amps;

      chan_desc->delays         = mbsfn_delays;
      chan_desc->ricean_factor  = 1;
      chan_desc->aoa            = 0;
      chan_desc->random_aoa     = 0;
      chan_desc->ch             = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->chF            = (struct complex **) malloc(nb_tx*nb_rx*sizeof(struct complex *));
      chan_desc->a              = (struct complex **) malloc(chan_desc->nb_taps*sizeof(struct complex *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complex *) malloc(chan_desc->channel_length * sizeof(struct complex));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complex *) malloc(1200 * sizeof(struct complex));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complex *) malloc(nb_tx*nb_rx * sizeof(struct complex));

      chan_desc->R_sqrt  = (struct complex **) malloc(6*sizeof(struct complex *));

      for (i = 0; i<6; i++) {
        chan_desc->R_sqrt[i]    = (struct complex *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complex));

        for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
          chan_desc->R_sqrt[i][j].x = 1.0;
          chan_desc->R_sqrt[i][j].y = 0.0;
        }

        LOG_W(OCM,"correlation matrix only implemented for nb_tx==2 and nb_rx==2, using identity\n");
      }

      break;

    case Rayleigh8:
      nb_taps = 8;
      Td = 0.8;
      channel_length = (int)11+2*sampling_rate*Td;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,
                        nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amps_lin,
                        NULL,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rice8:
      nb_taps = 8;
      Td = 0.8;
      channel_length = (int)11+2*sampling_rate*Td;
      ricean_factor = 0.1;
      aoa = 0.7854;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amps_lin,
                        NULL,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        1);
      break;

    case Rayleigh1:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rayleigh1_800:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 800;
      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rayleigh1_corr:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==1)) {
        R_sqrt_ptr2 = R_sqrt_21_corr;
      } else if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_corr;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rayleigh1_anticorr:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==1)) { //check this
        R_sqrt_ptr2 = R_sqrt_21_anticorr;
      } else if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_anticorr;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rice1:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 0.1;
      aoa = 0.7854;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case AWGN:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 0.0;
      aoa = 0.0;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      printf("AWGN: ricean_factor %f\n",chan_desc->ricean_factor);
      break;

    case TS_SHIFT:
      nb_taps = 2;
      Td = ts_shift_delays[1];
      channel_length = 10;
      ricean_factor = 0.0;
      aoa = 0.0;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        ts_shift_amps,
                        ts_shift_delays,
                        NULL,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      printf("TS_SHIFT: ricean_factor %f\n",chan_desc->ricean_factor);
      break;

    case Rice1_corr:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 0.1;
      aoa = .03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==1)) {
        R_sqrt_ptr2 = R_sqrt_21_corr;
      } else if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_corr;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        1);
      break;

    case Rice1_anticorr:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 0.1;
      aoa = .03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==1)) {
        R_sqrt_ptr2 = R_sqrt_21_anticorr;
      } else if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_anticorr;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        1);
      break;

    case Rayleigh1_orthogonal:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = 0.03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_orthogonal;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rayleigh1_orth_eff_ch_TM4_prec_real:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = 0.03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_orth_eff_ch_TM4_prec_real;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        1);
      break;

    case Rayleigh1_orth_eff_ch_TM4_prec_imag:
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 1;
      aoa = 0.03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_orth_eff_ch_TM4_prec_imag;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amp_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rayleigh8_orth_eff_ch_TM4_prec_real:
      if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_orth_eff_ch_TM4_prec_real;
        //R_sqrt_ptr2 = NULL;
      } else
        R_sqrt_ptr2 = NULL;

      nb_taps = 8;
      Td = 0.8;
      channel_length = (int)11+2*sampling_rate*Td;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 0;
      fill_channel_desc(chan_desc,
                        nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amps_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    case Rayleigh8_orth_eff_ch_TM4_prec_imag:
      nb_taps = 8;
      Td = 0.8;
      channel_length = (int)11+2*sampling_rate*Td;
      ricean_factor = 1;
      aoa = .03;
      maxDoppler = 0;

      if ((nb_tx==2) && (nb_rx==2)) {
        R_sqrt_ptr2 = R_sqrt_22_orth_eff_ch_TM4_prec_imag;
      } else
        R_sqrt_ptr2 = NULL;

      fill_channel_desc(chan_desc,
                        nb_tx,
                        nb_rx,
                        nb_taps,
                        channel_length,
                        default_amps_lin,
                        NULL,
                        R_sqrt_ptr2,
                        Td,
                        sampling_rate,
                        channel_bandwidth,
                        ricean_factor,
                        aoa,
                        forgetting_factor,
                        maxDoppler,
                        channel_offset,
                        path_loss_dB,
                        0);
      break;

    default:
      LOG_W(OCM,"channel model not yet supported\n");
      free(chan_desc);
      return(NULL);
  }

  LOG_D(OCM,"[CHANNEL] RF %f\n",chan_desc->ricean_factor);

  for (i=0; i<chan_desc->nb_taps; i++)
    LOG_D(OCM,"[CHANNEL] tap %d: amp %f, delay %f\n",i,chan_desc->amps[i],chan_desc->delays[i]);

  chan_desc->nb_paths = 10;
  return(chan_desc);
}

void free_channel_desc_scm(channel_desc_t *ch) {
  // Must be made cleanly, a lot of leaks...
  free(ch);
}

int random_channel(channel_desc_t *desc, uint8_t abstraction_flag) {
  double s;
  int i,k,l,aarx,aatx;
  struct complex anew[NB_ANTENNAS_TX*NB_ANTENNAS_RX],acorr[NB_ANTENNAS_TX*NB_ANTENNAS_RX];
  struct complex phase, alpha, beta;
  AssertFatal(desc->nb_tx<=NB_ANTENNAS_TX && desc->nb_rx <= NB_ANTENNAS_RX,
              "random_channel.c: Error: temporary buffer for channel not big enough (%d,%d)\n",desc->nb_tx,desc->nb_rx);
  start_meas(&desc->random_channel);

  for (i=0; i<(int)desc->nb_taps; i++) {
    for (aarx=0; aarx<desc->nb_rx; aarx++) {
      for (aatx=0; aatx<desc->nb_tx; aatx++) {
        anew[aarx+(aatx*desc->nb_rx)].x = sqrt(desc->ricean_factor*desc->amps[i]/2) * gaussdouble(0.0,1.0);
        anew[aarx+(aatx*desc->nb_rx)].y = sqrt(desc->ricean_factor*desc->amps[i]/2) * gaussdouble(0.0,1.0);

        if ((i==0) && (desc->ricean_factor != 1.0)) {
          if (desc->random_aoa==1) {
            desc->aoa = uniformrandom()*2*M_PI;
          }

          // this assumes that both RX and TX have linear antenna arrays with lambda/2 antenna spacing.
          // Furhter it is assumed that the arrays are parallel to each other and that they are far enough apart so
          // that we can safely assume plane wave propagation.
          phase.x = cos(M_PI*((aarx-aatx)*sin(desc->aoa)));
          phase.y = sin(M_PI*((aarx-aatx)*sin(desc->aoa)));
          anew[aarx+(aatx*desc->nb_rx)].x += phase.x * sqrt(1.0-desc->ricean_factor);
          anew[aarx+(aatx*desc->nb_rx)].y += phase.y * sqrt(1.0-desc->ricean_factor);
        }

#ifdef DEBUG_CH
        printf("(%d,%d,%d) %f->(%f,%f) (%f,%f) phase (%f,%f)\n",aarx,aatx,i,desc->amps[i],anew[aarx+(aatx*desc->nb_rx)].x,anew[aarx+(aatx*desc->nb_rx)].y,desc->aoa,desc->ricean_factor,phase.x,phase.y);
#endif
      } //aatx
    } //aarx

    /*
    // for debugging set a=anew;
    for (aarx=0;aarx<desc->nb_rx;aarx++) {
      for (aatx=0;aatx<desc->nb_tx;aatx++) {
        desc->a[i][aarx+(aatx*desc->nb_rx)].x = anew[aarx+(aatx*desc->nb_rx)].x;
        desc->a[i][aarx+(aatx*desc->nb_rx)].y = anew[aarx+(aatx*desc->nb_rx)].y;
        printf("anew(%d,%d) = %f+1j*%f\n",aatx,aarx,anew[aarx+(aatx*desc->nb_rx)].x, anew[aarx+(aatx*desc->nb_rx)].y);
     }
    }
    */
    //apply correlation matrix
    //compute acorr = R_sqrt[i] * anew
    alpha.x = 1.0;
    alpha.y = 0.0;
    beta.x = 0.0;
    beta.y = 0.0;
    cblas_zgemv(CblasRowMajor, CblasNoTrans, desc->nb_tx*desc->nb_rx, desc->nb_tx*desc->nb_rx,
                (void *) &alpha, (void *) desc->R_sqrt[i/3], desc->nb_rx*desc->nb_tx,
                (void *) anew, 1, (void *) &beta, (void *) acorr, 1);

    /*
    for (aarx=0;aarx<desc->nb_rx;aarx++) {
      for (aatx=0;aatx<desc->nb_tx;aatx++) {
        desc->a[i][aarx+(aatx*desc->nb_rx)].x = acorr[aarx+(aatx*desc->nb_rx)].x;
        desc->a[i][aarx+(aatx*desc->nb_rx)].y = acorr[aarx+(aatx*desc->nb_rx)].y;
        printf("tap %d, acorr1(%d,%d) = %f+1j*%f\n",i,aatx,aarx,acorr[aarx+(aatx*desc->nb_rx)].x, acorr[aarx+(aatx*desc->nb_rx)].y);
      }
    }
    */

    if (desc->first_run==1) {
      cblas_zcopy(desc->nb_tx*desc->nb_rx, (void *) acorr, 1, (void *) desc->a[i], 1);
    } else {
      // a = alpha*acorr+beta*a
      // a = beta*a
      // a = a+alpha*acorr
      alpha.x = sqrt(1-desc->forgetting_factor);
      alpha.y = 0;
      beta.x = sqrt(desc->forgetting_factor);
      beta.y = 0;
      cblas_zscal(desc->nb_tx*desc->nb_rx, (void *) &beta, (void *) desc->a[i], 1);
      cblas_zaxpy(desc->nb_tx*desc->nb_rx, (void *) &alpha, (void *) acorr, 1, (void *) desc->a[i], 1);
      //  desc->a[i][aarx+(aatx*desc->nb_rx)].x = (sqrt(desc->forgetting_factor)*desc->a[i][aarx+(aatx*desc->nb_rx)].x) + sqrt(1-desc->forgetting_factor)*anew.x;
      //  desc->a[i][aarx+(aatx*desc->nb_rx)].y = (sqrt(desc->forgetting_factor)*desc->a[i][aarx+(aatx*desc->nb_rx)].y) + sqrt(1-desc->forgetting_factor)*anew.y;
    }

    /*
    for (aarx=0;aarx<desc->nb_rx;aarx++) {
      for (aatx=0;aatx<desc->nb_tx;aatx++) {
        //desc->a[i][aarx+(aatx*desc->nb_rx)].x = acorr[aarx+(aatx*desc->nb_rx)].x;
        //desc->a[i][aarx+(aatx*desc->nb_rx)].y = acorr[aarx+(aatx*desc->nb_rx)].y;
        printf("tap %d, a(%d,%d) = %f+1j*%f\n",i,aatx,aarx,desc->a[i][aarx+(aatx*desc->nb_rx)].x, desc->a[i][aarx+(aatx*desc->nb_rx)].y);
      }
    }
    */
  } //nb_taps

  stop_meas(&desc->random_channel);

  //memset((void *)desc->ch[aarx+(aatx*desc->nb_rx)],0,(int)(desc->channel_length)*sizeof(struct complex));

  if (abstraction_flag==0) {
    start_meas(&desc->interp_time);

    for (aarx=0; aarx<desc->nb_rx; aarx++) {
      for (aatx=0; aatx<desc->nb_tx; aatx++) {
        if (desc->channel_length == 1) {
          desc->ch[aarx+(aatx*desc->nb_rx)][0].x = desc->a[0][aarx+(aatx*desc->nb_rx)].x;
          desc->ch[aarx+(aatx*desc->nb_rx)][0].y = desc->a[0][aarx+(aatx*desc->nb_rx)].y;
        } else {
          for (k=0; k<(int)desc->channel_length; k++) {
            desc->ch[aarx+(aatx*desc->nb_rx)][k].x = 0.0;
            desc->ch[aarx+(aatx*desc->nb_rx)][k].y = 0.0;

            for (l=0; l<desc->nb_taps; l++) {
              if ((k - (desc->delays[l]*desc->sampling_rate) - NB_SAMPLES_CHANNEL_OFFSET) == 0)
                s = 1.0;
              else
                s = sin(M_PI*(k - (desc->delays[l]*desc->sampling_rate) - NB_SAMPLES_CHANNEL_OFFSET))/
                    (M_PI*(k - (desc->delays[l]*desc->sampling_rate) - NB_SAMPLES_CHANNEL_OFFSET));

              desc->ch[aarx+(aatx*desc->nb_rx)][k].x += s*desc->a[l][aarx+(aatx*desc->nb_rx)].x;
              desc->ch[aarx+(aatx*desc->nb_rx)][k].y += s*desc->a[l][aarx+(aatx*desc->nb_rx)].y;
              //    printf("l %d : desc->ch.x %f\n",l,desc->a[l][aarx+(aatx*desc->nb_rx)].x);
            } //nb_taps

#ifdef DEBUG_CH
            k=0;
            printf("(%d,%d,%d)->(%f,%f)\n",k,aarx,aatx,desc->ch[aarx+(aatx*desc->nb_rx)][k].x,desc->ch[aarx+(aatx*desc->nb_rx)][k].y);
#endif
          }
        } //channel_length
      } //aatx
    } //aarx

    stop_meas(&desc->interp_time);
  }

  if (desc->first_run==1)
    desc->first_run = 0;

  return (0);
}

double N_RB2sampling_rate(uint16_t N_RB) {
  double sampling_rate;

  switch (N_RB) {
    case 6:
      sampling_rate = 1.92;
      break;

    case 25:
      sampling_rate = 7.68;
      break;

    case 50:
      sampling_rate = 15.36;
      break;

    case 100:
      sampling_rate = 30.72;
      break;

    default:
      AssertFatal(1==0,"Unknown N_PRB %d",N_RB);
  }

  return(sampling_rate);
}

double N_RB2channel_bandwidth(uint16_t N_RB) {
  double channel_bandwidth;

  switch (N_RB) {
    case 6:
      channel_bandwidth = 1.25;
      break;

    case 25:
      channel_bandwidth = 5.00;
      break;

    case 50:
      channel_bandwidth = 10.00;
      break;

    case 100:
      channel_bandwidth = 20.00;
      break;

    default:
      LOG_E(PHY,"Unknown N_PRB\n");
      return(-1);
  }

  return(channel_bandwidth);
}

static int channelmod_show_cmd(char *buff, int debug, telnet_printfunc_t prnt) {
  for(int i=0; channelmod_names[i].name != NULL ; i++) {
    prnt("  %i %s\n", i, map_int_to_str(channelmod_names,i ));
  }

  return 0;
}

int modelid_fromname(char *modelname) {
  int modelid=map_str_to_int(channelmod_names,modelname);
  AssertFatal(modelid>0,
              "random_channel.c: Error channel model %s unknown\n",modelname);
  return modelid;
}

double channelmod_get_snr_dB(void) {
  return snr_dB;
}

double channelmod_get_sinr_dB(void) {
  return sinr_dB;
}

void init_channelmod(void) {
  paramdef_t channelmod_params[] = CHANNELMOD_PARAMS_DESC;
  int ret = config_get( channelmod_params,sizeof(channelmod_params)/sizeof(paramdef_t),CHANNELMOD_SECTION);
  AssertFatal(ret >= 0, "configuration couldn't be performed");
  /* look for telnet server, if it is loaded, add the channel modeling commands to it */
  add_telnetcmd_func_t addcmd = (add_telnetcmd_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_ADDCMD_FNAME);

  if (addcmd != NULL) {
    addcmd("channelmod",channelmod_vardef,channelmod_cmdarray);
  }
}

#ifdef RANDOM_CHANNEL_MAIN
#define sampling_rate 5.0
#define Td 2.0
main(int argc,char **argv) {
  double amps[8] = {.8,.2,.1,.04,.02,.01,.005};
  struct complex ch[(int)(1+2*sampling_rate*Td)],phase;
  int i;
  randominit();
  phase.x = 1.0;
  phase.y = 0;
  random_channel(amps,Td, 8,sampling_rate,ch,(double)1.0,&phase);
  /*
  for (i=0;i<(11+2*sampling_rate*Td);i++){
    printf("%f + sqrt(-1)*%f\n",ch[i].x,ch[i].y);
  }
  */
}

#endif
