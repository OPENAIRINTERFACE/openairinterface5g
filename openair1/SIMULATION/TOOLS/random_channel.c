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
static int channelmod_modify_cmd(char *buff, int debug, telnet_printfunc_t prnt);
static int channelmod_print_help(char *buff, int debug, telnet_printfunc_t prnt);
static telnetshell_cmddef_t channelmod_cmdarray[] = {
  {"help","",channelmod_print_help},
  {"show","<predef,current>",channelmod_show_cmd},
  {"modify","<channelid> <param> <value>",channelmod_modify_cmd},
  {"","",NULL},
};

static telnetshell_vardef_t channelmod_vardef[] = {
  {"",0,NULL}
};

static double snr_dB=25;
static double sinr_dB=0;
static unsigned int max_chan;
static channel_desc_t **defined_channels;
static char modellist_name[MAX_OPTNAME_SIZE]= {0};


void fill_channel_desc(channel_desc_t *chan_desc,
                       uint8_t nb_tx,
                       uint8_t nb_rx,
                       uint8_t nb_taps,
                       uint8_t channel_length,
                       double *amps,
                       double *delays,
                       struct complexd *R_sqrt,
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
    chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_DELAY ;
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
  chan_desc->ch                         = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
  chan_desc->chF                        = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
  chan_desc->a                          = (struct complexd **) malloc(nb_taps*sizeof(struct complexd *));
  LOG_D(OCM,"[CHANNEL] Filling ch \n");

  for (i = 0; i<nb_tx*nb_rx; i++)
    chan_desc->ch[i] = (struct complexd *) malloc(channel_length * sizeof(struct complexd));

  for (i = 0; i<nb_tx*nb_rx; i++)
    chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd)); // allocate for up to 100 RBs, 12 samples per RB

  LOG_D(OCM,"[CHANNEL] Filling a (nb_taps %d)\n",nb_taps);

  for (i = 0; i<nb_taps; i++) {
    LOG_D(OCM,"tap %d (%p,%zu)\n",i,&chan_desc->a[i],nb_tx*nb_rx * sizeof(struct complexd));
    chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));
  }

  LOG_D(OCM,"[CHANNEL] Doing R_sqrt ...\n");

  if (R_sqrt == NULL) {
    chan_desc->R_sqrt         = (struct complexd **) calloc(nb_taps,sizeof(struct complexd *));
    chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_NTAPS ;

    for (i = 0; i<nb_taps; i++) {
      chan_desc->R_sqrt[i]    = (struct complexd *) calloc(nb_tx*nb_rx*nb_tx*nb_rx,sizeof(struct complexd));

      for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
        chan_desc->R_sqrt[i][j].r = 1.0;
        chan_desc->R_sqrt[i][j].i = 0.0;
      }
    }
  } else {
    chan_desc->R_sqrt = (struct complexd **) calloc(nb_taps,sizeof(struct complexd *));

    for (i = 0; i<nb_taps; i++) {
      //chan_desc->R_sqrt[i]    = (struct complexd*) calloc(nb_tx*nb_rx*nb_tx*nb_rx,sizeof(struct complexd));
      //chan_desc->R_sqrt = (struct complexd*)&R_sqrt[i][0];
      /* all chan_desc share the same R_sqrt, coming from caller */
      chan_desc->R_sqrt[i] = R_sqrt;
    }
  }

  for (i = 0; i<nb_taps; i++) {
    for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
      LOG_D(OCM,"Rsqrt[%d][%d] %f %f\n",i,j,chan_desc->R_sqrt[i][j].r,chan_desc->R_sqrt[i][j].i);
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

static double mbsfn_delays[] = {0,.03,.15,.31,.37,1.09,12.490,12.52,12.64,12.80,12.86,13.58,27.49,27.52,27.64,27.80,27.86,28.58};
static double mbsfn_amps_dB[] = {0,-1.5,-1.4,-3.6,-0.6,-7.0,-10,-11.5,-11.4,-13.6,-10.6,-17.0,-20,-21.5,-21.4,-23.6,-20.6,-27};

static double scm_c_delays[] = {0, 0.0125, 0.0250, 0.3625, 0.3750, 0.3875, 0.2500, 0.2625, 0.2750, 1.0375, 1.0500, 1.0625, 2.7250, 2.7375, 2.7500, 4.6000, 4.6125, 4.6250};
static double scm_c_amps_dB[] = {0.00, -2.22, -3.98, -1.86, -4.08, -5.84, -1.08, -3.30, -5.06, -9.08, -11.30, -13.06, -15.14, -17.36, -19.12, -20.64, -22.85, -24.62};

static double tdl_a_delays[] = {0.0000,
                         0.3819,
                         0.4025,
                         0.5868,
                         0.4610,
                         0.5375,
                         0.6708,
                         0.5750,
                         0.7618,
                         1.5375,
                         1.8978,
                         2.2242,
                         2.1718,
                         2.4942,
                         2.5119,
                         3.0582,
                         4.0810,
                         4.4579,
                         4.5695,
                         4.7966,
                         5.0066,
                         5.3043,
                         9.6586
                        };
static double tdl_a_amps_dB[] = {-13.4,
                          0,
                          -2.2,
                          -4,
                          -6,
                          -8.2,
                          -9.9,
                          -10.5,
                          -7.5,
                          -15.9,
                          -6.6,
                          -16.7,
                          -12.4,
                          -15.2,
                          -10.8,
                          -11.3,
                          -12.7,
                          -16.2,
                          -18.3,
                          -18.9,
                          -16.6,
                          -19.9,
                          -29.7
                          };

static double tdl_b_delays[] = {0.0000,
                         0.1072,
                         0.2155,
                         0.2095,
                         0.2870,
                         0.2986,
                         0.3752,
                         0.5055,
                         0.3681,
                         0.3697,
                         0.5700,
                         0.5283,
                         1.1021,
                         1.2756,
                         1.5474,
                         1.7842,
                         2.0169,
                         2.8294,
                         3.0219,
                         3.6187,
                         4.1067,
                         4.2790,
                         4.7834
                        };

static double tdl_b_amps_dB[] = {0,
                          -2.2,
                          -4,
                          -3.2,
                          -9.8,
                          -1.2,
                          -3.4,
                          -5.2,
                          -7.6,
                          -3,
                          -8.9,
                          -9,
                          -4.8,
                          -5.7,
                          -7.5,
                          -1.9,
                          -7.6,
                          -12.2,
                          -9.8,
                          -11.4,
                          -14.9,
                          -9.2,
                          -11.3
                          };

static double tdl_c_delays[] = {0,
                         0.2099,
                         0.2219,
                         0.2329,
                         0.2176,
                         0.6366,
                         0.6448,
                         0.6560,
                         0.6584,
                         0.7935,
                         0.8213,
                         0.9336,
                         1.2285,
                         1.3083,
                         2.1704,
                         2.7105,
                         4.2589,
                         4.6003,
                         5.4902,
                         5.6077,
                         6.3065,
                         6.6374,
                         7.0427,
                         8.6523
                        };

static double tdl_c_amps_dB[] = {-4.4,
                          -1.2,
                          -3.5,
                          -5.2,
                          -2.5,
                          0,
                          -2.2,
                          -3.9,
                          -7.4,
                          -7.1,
                          -10.7,
                          -11.1,
                          -5.1,
                          -6.8,
                          -8.7,
                          -13.2,
                          -13.9,
                          -13.9,
                          -15.8,
                          -17.1,
                          -16,
                          -15.7,
                          -21.6,
                          -22.8
                          };

static double tdl_d_delays[] = {//0,
  0,
  0.035,
  0.612,
  1.363,
  1.405,
  1.804,
  2.596,
  1.775,
  4.042,
  7.937,
  9.424,
  9.708,
  12.525
};

static double tdl_d_amps_dB[] = {//-0.2,
  //-13.5,
  -.00147,
    -18.8,
    -21,
    -22.8,
    -17.9,
    -20.1,
    -21.9,
    -22.9,
    -27.8,
    -23.6,
    -24.8,
    -30.0,
    -27.7
  };

#define TDL_D_RICEAN_FACTOR .046774

static double tdl_e_delays[] = {0,
                         0.5133,
                         0.5440,
                         0.5630,
                         0.5440,
                         0.7112,
                         1.9092,
                         1.9293,
                         1.9589,
                         2.6426,
                         3.7136,
                         5.4524,
                         12.0034,
                         20.6519
                        };

static double tdl_e_amps_dB[] = {//-0.03,
  //-22.03,
  -.00433,
    -15.8,
    -18.1,
    -19.8,
    -22.9,
    -22.4,
    -18.6,
    -20.8,
    -22.6,
    -22.3,
    -25.6,
    -20.2,
    -29.8,
    -29.2
  };

#define TDL_E_RICEAN_FACTOR 0.0063096

static double epa_delays[] = { 0,.03,.07,.09,.11,.19,.41};
static double epa_amps_dB[] = {0.0,-1.0,-2.0,-3.0,-8.0,-17.2,-20.8};

static double eva_delays[] = { 0,.03,.15,.31,.37,.71,1.09,1.73,2.51};
static double eva_amps_dB[] = {0.0,-1.5,-1.4,-3.6,-0.6,-9.1,-7.0,-12.0,-16.9};

static double etu_delays[] = { 0,.05,.12,.2,.23,.5,1.6,2.3,5.0};
static double etu_amps_dB[] = {-1.0,-1.0,-1.0,0.0,0.0,0.0,-3.0,-5.0,-7.0};

static double default_amps_lin[] = {0.3868472, 0.3094778, 0.1547389, 0.0773694, 0.0386847, 0.0193424, 0.0096712, 0.0038685};
static double default_amp_lin[] = {1};

//correlation matrix for a 2x2 channel with full Tx correlation
static struct complexd R_sqrt_22_corr[16] = {{0.70711,0}, {0.0, 0.0}, {0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0}, {0.0, 0.0}, {0.70711,0},
  {0.70711,0}, {0.0, 0.0}, {0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0}, {0.0, 0.0}, {0.70711,0}
};

//correlation matrix for a fully correlated 2x1 channel (h1==h2)
static struct complexd R_sqrt_21_corr[]  = {{0.70711,0}, {0.70711,0}, {0.70711,0}, {0.70711,0}};

//correlation matrix for a 2x2 channel with full Tx anti-correlation
static struct complexd R_sqrt_22_anticorr[16] = {{0.70711,0}, {0.0, 0.0}, {-0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0}, {0.0, 0.0}, {-0.70711,0},
  {-0.70711,0}, {0.0, 0.0}, {0.70711,0}, {0.0, 0.0},
  {0.0, 0.0}, {-0.70711,0}, {0.0, 0.0}, {0.70711,0}
};

//correlation matrix for a fully anti-correlated 2x1 channel (h1==-h2)
static struct complexd R_sqrt_21_anticorr[4]  = {{0.70711,0}, {-0.70711,0}, {-0.70711,0}, {0.70711,0}};

// full correlation matrix in vectorized form for 2x2 channel, where h1 is  perfectly orthogonal to h2

static struct complexd R_sqrt_22_orthogonal[16] = {{0.70711,0.0}, {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0}, {0.0,0.0},
  {0.0,0.0}, {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.0,0.0}, {0.0, 0.0}, {0.70711,0.0}
};

// full correlation matrix for TM4 to make orthogonal effective channel
static struct complexd R_sqrt_22_orth_eff_ch_TM4_prec_real[16] = {{0.70711,0.0}, {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0}, {-0.70711,0.0},
  {0.70711,0.0}, {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {-0.70711,0.0}, {0.0, 0.0}, {0.70711,0.0}
};

static struct complexd R_sqrt_22_orth_eff_ch_TM4_prec_imag[16] = {{0.70711,0.0}, {0.0,0.0}, {0.0, -0.70711}, {0.0,0.0},
  {0.0, 0.0}, {0.70711,0.0}, {0.0, 0.0}, {0.0,0.70711},
  {0.0,-0.70711}, {0.0, 0.0}, {-0.70711,0.0}, {0.0, 0.0},
  {0.0, 0.0}, {0.0,0.70711}, {0.0, 0.0}, {-0.70711,0.0}
};

//Correlation matrix for EPA channel
static struct complexd R_sqrt_22_EPA_low[16] = {{1.0,0.0}, {0.0,0.0}, {0.0,0.0}, {0.0,0.0},
  {0.0,0.0}, {1.0,0.0}, {0.0,0.0}, {0.0,0.0},
  {0.0,0.0}, {0.0,0.0}, {1.0,0.0}, {0.0,0.0},
  {0.0,0.0}, {0.0,0.0}, {0.0,0.0}, {1.0,0.0}
};

static struct complexd R_sqrt_22_EPA_high[16] = {
  {0.7179,0.0}, {0.4500,0.0}, {0.4500,0.0}, {0.2821,0.0},
  {0.4500,0.0}, {0.7179,0.0}, {0.2821,0.0}, {0.4500,0.0},
  {0.4500,0.0}, {0.2821,0.0}, {0.7179,0.0}, {0.4500,0.0},
  {0.2821,0.0}, {0.4500,0.0}, {0.4500,0.0}, {0.7179,0.0}
};

static struct complexd R_sqrt_22_EPA_medium[16] = {{0.8375,0.0}, {0.5249,0.0}, {0.1286,0.0}, {0.0806,0.0},
  {0.5249,0.0}, {0.8375,0.0}, {0.0806,0.0}, {0.1286,0.0},
  {0.1286,0.0}, {0.0806,0.0}, {0.8375,0.0}, {0.5249,0.0},
  {0.0806,0.0}, {0.1286,0.0}, {0.5249,0.0}, {0.8375,0.0}
};

//Rayleigh1_orth_eff_ch_TM4

void tdlModel(int  tdl_paths, double *tdl_delays, double *tdl_amps_dB, double DS_TDL, channel_desc_t *chan_desc ) {
  int nb_rx=chan_desc-> nb_rx;
  int nb_tx=chan_desc-> nb_tx;
  int tdl_pathsby3 = tdl_paths/3;

  if ((tdl_paths%3)>0)
    tdl_pathsby3++;

  chan_desc->nb_taps        = tdl_paths;
  chan_desc->Td             = tdl_delays[tdl_paths-1]*DS_TDL;
  printf("last path (%d) at %f * %e = %e\n",tdl_paths-1,tdl_delays[tdl_paths-1],DS_TDL,chan_desc->Td);
  chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td +
                                     1 +
                                     2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
  printf("TDL : %f Ms/s, nb_taps %d, Td %e, channel_length %d\n",chan_desc->sampling_rate,tdl_paths,chan_desc->Td,chan_desc->channel_length);
  double sum_amps = 0;
  chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));

  for (int i = 0; i<chan_desc->nb_taps; i++) {
    chan_desc->amps[i]      = pow(10,.1*tdl_amps_dB[i]);
    sum_amps += chan_desc->amps[i];
  }

  for (int i = 0; i<chan_desc->nb_taps; i++) {
    chan_desc->amps[i] /= sum_amps;
    tdl_delays[i] *= DS_TDL;
  }

  chan_desc->delays         = tdl_delays;
  chan_desc->aoa            = 0;
  chan_desc->random_aoa     = 0;
  chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
  chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
  chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

  for (int i = 0; i<nb_tx*nb_rx; i++)
    chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

  for (int i = 0; i<nb_tx*nb_rx; i++)
    chan_desc->chF[i] = (struct complexd *) malloc((2+(275*12)) * sizeof(struct complexd));

  for (int i = 0; i<chan_desc->nb_taps; i++)
    chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

  chan_desc->R_sqrt  = (struct complexd **) malloc(tdl_pathsby3*sizeof(struct complexd **));

  if (nb_tx==2 && nb_rx==2) {
    for (int i = 0; i<(tdl_pathsby3); i++)
      chan_desc->R_sqrt[i] = (struct complexd *) &R22_sqrt[i][0];
  } else if (nb_tx==2 && nb_rx==1) {
    for (int i = 0; i<(tdl_pathsby3); i++)
      chan_desc->R_sqrt[i] = (struct complexd *) &R21_sqrt[i][0];
  } else if (nb_tx==1 && nb_rx==2) {
    for (int i = 0; i<(tdl_pathsby3); i++)
      chan_desc->R_sqrt[i] = (struct complexd *) &R12_sqrt[i][0];
  } else {
    for (int i = 0; i<(tdl_pathsby3); i++) {
      chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

      for (int j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
        chan_desc->R_sqrt[i][j].r = 1.0;
        chan_desc->R_sqrt[i][j].i = 0.0;
      }

      LOG_W(OCM,"correlation matrix not implemented for nb_tx==%d and nb_rx==%d, using identity\n", nb_tx, nb_rx);
    }
  }
}

channel_desc_t *new_channel_desc_scm(uint8_t nb_tx,
                                     uint8_t nb_rx,
                                     SCM_t channel_model,
                                     double sampling_rate,
                                     double channel_bandwidth,
                                     double DS_TDL,
                                     double forgetting_factor,
                                     int32_t channel_offset,
                                     double path_loss_dB,
                                     float  noise_power_dB) {
  channel_desc_t *chan_desc = (channel_desc_t *)calloc(1,sizeof(channel_desc_t));

  for(int i=0; i<max_chan; i++) {
    if (defined_channels[i] == NULL) {
      defined_channels[i]=chan_desc;
      chan_desc->chan_idx=i;
      break;
    } else {
      AssertFatal(i<(max_chan-1),
                  "No more channel descriptors available, increase channelmod.max_chan parameter above %u\n",max_chan);
    }
  }

  uint16_t i,j;
  double sum_amps;
  double aoa,ricean_factor,Td,maxDoppler;
  int channel_length,nb_taps;
  struct complexd *R_sqrt_ptr2;
  chan_desc->modelid                   = channel_model;
  chan_desc->nb_tx                      = nb_tx;
  chan_desc->nb_rx                      = nb_rx;
  chan_desc->sampling_rate              = sampling_rate;
  chan_desc->channel_bandwidth          = channel_bandwidth;
  chan_desc->forgetting_factor          = forgetting_factor;
  chan_desc->channel_offset             = channel_offset;
  chan_desc->path_loss_dB               = path_loss_dB;
  chan_desc->first_run                  = 1;
  chan_desc->ip                                 = 0.0;
  chan_desc->noise_power_dB             = noise_power_dB;
  LOG_I(OCM,"Channel Model (inside of new_channel_desc_scm)=%d\n\n", channel_model);
  int tdl_paths=0;
  double *tdl_amps_dB;
  double *tdl_delays;

  /*  Spatial Channel Models (SCM)  channel model from TR 38.901 Section 7.7.2 */
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      chan_desc->R_sqrt  = (struct complexd **) malloc(6*sizeof(struct complexd **));

      if (nb_tx==2 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R22_sqrt[i][0];
      } else if (nb_tx==2 && nb_rx==1) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R21_sqrt[i][0];
      } else if (nb_tx==1 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R12_sqrt[i][0];
      } else {
        chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_6 ;

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].r = 1.0;
            chan_desc->R_sqrt[i][j].i = 0.0;
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      chan_desc->R_sqrt  = (struct complexd **) malloc(6*sizeof(struct complexd **));

      if (nb_tx==2 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R22_sqrt[i][0];
      } else if (nb_tx==2 && nb_rx==1) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R21_sqrt[i][0];
      } else if (nb_tx==1 && nb_rx==2) {
        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R12_sqrt[i][0];
      } else {
        chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_6 ;

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].r = 1.0;
            chan_desc->R_sqrt[i][j].i = 0.0;
          }

          LOG_W(OCM,"correlation matrix not implemented for nb_tx==%d and nb_rx==%d, using identity\n", nb_tx, nb_rx);
        }
      }

      break;
      /*  tapped delay line (TDL)  channel model from TR 38.901 Section 7.7.2 */
#define tdl_m(MoDel)\
  DevAssert(sizeof(tdl_ ## MoDel ## _amps_dB) == sizeof(tdl_ ## MoDel ## _delays)); \
  tdl_paths=sizeof(tdl_ ## MoDel ## _amps_dB)/sizeof(*tdl_ ## MoDel ## _amps_dB);\
  tdl_delays=tdl_ ## MoDel ## _delays;\
  tdl_amps_dB=tdl_ ## MoDel ## _amps_dB

    case TDL_A:
      chan_desc->ricean_factor  = 1;
      tdl_m(a);
      tdlModel(tdl_paths,  tdl_delays, tdl_amps_dB,  DS_TDL, chan_desc);
      break;

    case TDL_B:
      chan_desc->ricean_factor  = 1;
      tdl_m(b);
      tdlModel(tdl_paths,  tdl_delays, tdl_amps_dB,  DS_TDL, chan_desc);
      break;

    case TDL_C:
      chan_desc->ricean_factor  = 1;
      tdl_m(c);
      tdlModel(tdl_paths,  tdl_delays, tdl_amps_dB,  DS_TDL, chan_desc);
      break;

    case TDL_D:
      chan_desc->ricean_factor  = TDL_D_RICEAN_FACTOR;
      tdl_m(d);
      tdlModel(tdl_paths,  tdl_delays, tdl_amps_dB,  DS_TDL, chan_desc);
      break;

    case TDL_E:
      chan_desc->ricean_factor  = TDL_E_RICEAN_FACTOR;
      tdl_m(e);
      tdlModel(tdl_paths,  tdl_delays, tdl_amps_dB,  DS_TDL, chan_desc);
      break;

    case EPA:
      chan_desc->nb_taps        = 7;
      chan_desc->Td             = .410;
      chan_desc->channel_length = (int) (2*chan_desc->sampling_rate*chan_desc->Td + 1 + 2/(M_PI*M_PI)*log(4*M_PI*chan_desc->sampling_rate*chan_desc->Td));
      sum_amps = 0;
      chan_desc->amps           = (double *) malloc(chan_desc->nb_taps*sizeof(double));
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complexd **) malloc(6*sizeof(struct complexd **));

        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R22_sqrt[i][0];
      } else {
        chan_desc->R_sqrt         = (struct complexd **) malloc(6*sizeof(struct complexd **));
        chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_6 ;

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].r = 1.0;
            chan_desc->R_sqrt[i][j].i = 0.0;
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd **));

        for (i = 0; i<chan_desc->nb_taps; i++)
          chan_desc->R_sqrt[i] = R_sqrt_22_EPA_low;
      } else {
        printf("Correlation matrices are implemented for 2 x 2 only");
      }

      /*else {
        chan_desc->R_sqrt         = (struct complexd**) malloc(6*sizeof(struct complexd**));
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd*) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd **));

        for (i = 0; i<chan_desc->nb_taps; i++)
          chan_desc->R_sqrt[i] = R_sqrt_22_EPA_high;
      } else {
        printf("Correlation matrices are implemented for 2 x 2 only");
      }

      /*else {
        chan_desc->R_sqrt         = (struct complexd**) malloc(6*sizeof(struct complexd**));
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd*) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd **));

        for (i = 0; i<chan_desc->nb_taps; i++)
          chan_desc->R_sqrt[i] = R_sqrt_22_EPA_medium;
      } else {
        printf("Correlation matrices are implemented for 2 x 2 only");
      }

      /*else {
        chan_desc->R_sqrt         = (struct complexd**) malloc(6*sizeof(struct complexd**));
        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd*) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complexd **) malloc(6*sizeof(struct complexd **));

        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R22_sqrt[i][0];
      } else {
        chan_desc->R_sqrt         = (struct complexd **) malloc(6*sizeof(struct complexd **));
        chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_6 ;

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].r = 1.0;
            chan_desc->R_sqrt[i][j].i = 0.0;
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      if (nb_tx==2 && nb_rx==2) {
        chan_desc->R_sqrt  = (struct complexd **) malloc(6*sizeof(struct complexd **));

        for (i = 0; i<6; i++)
          chan_desc->R_sqrt[i] = (struct complexd *) &R22_sqrt[i][0];
      } else {
        chan_desc->R_sqrt         = (struct complexd **) malloc(6*sizeof(struct complexd **));
        chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_6 ;

        for (i = 0; i<6; i++) {
          chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

          for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
            chan_desc->R_sqrt[i][j].r = 1.0;
            chan_desc->R_sqrt[i][j].i = 0.0;
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
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_AMPS ;

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
      chan_desc->ch             = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->chF            = (struct complexd **) malloc(nb_tx*nb_rx*sizeof(struct complexd *));
      chan_desc->a              = (struct complexd **) malloc(chan_desc->nb_taps*sizeof(struct complexd *));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->ch[i] = (struct complexd *) malloc(chan_desc->channel_length * sizeof(struct complexd));

      for (i = 0; i<nb_tx*nb_rx; i++)
        chan_desc->chF[i] = (struct complexd *) malloc(1200 * sizeof(struct complexd));

      for (i = 0; i<chan_desc->nb_taps; i++)
        chan_desc->a[i]         = (struct complexd *) malloc(nb_tx*nb_rx * sizeof(struct complexd));

      chan_desc->R_sqrt  = (struct complexd **) malloc(6*sizeof(struct complexd *));
      chan_desc->free_flags=chan_desc->free_flags|CHANMODEL_FREE_RSQRT_6;

      for (i = 0; i<6; i++) {
        chan_desc->R_sqrt[i]    = (struct complexd *) malloc(nb_tx*nb_rx*nb_tx*nb_rx * sizeof(struct complexd));

        for (j = 0; j<nb_tx*nb_rx*nb_tx*nb_rx; j+=(nb_tx*nb_rx+1)) {
          chan_desc->R_sqrt[i][j].r = 1.0;
          chan_desc->R_sqrt[i][j].i = 0.0;
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

    case Rayleigh1://MIMO Test uses Rayleigh1
      nb_taps = 1;
      Td = 0;
      channel_length = 1;
      ricean_factor = 0.0;
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
      double ts_shift_delays[] = {0, 1/7.68};
      Td = ts_shift_delays[1];
      channel_length = 10;
      ricean_factor = 0.0;
      aoa = 0.0;
      maxDoppler = 0;
      double ts_shift_amps[] = {0, 1};
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
} /* channel_desc_t *new_channel_desc_scm  */

channel_desc_t *find_channel_desc_fromname( char *modelname ) {
  for(int i=0; i<max_chan; i++) {
    if (defined_channels[i] != NULL) {
      if (strcmp(defined_channels[i]->model_name,modelname) == 0)
        return defined_channels[i];
    }
  }

  LOG_E(OCM,"Model %s not found \n", modelname);
  return NULL;
} /* channel_desc_t * new_channel_desc_fromconfig */



void free_channel_desc_scm(channel_desc_t *ch) {
  // Must be made cleanly, a lot of leaks...
  if (max_chan != 0) defined_channels[ch->chan_idx]=NULL;
  if(ch->free_flags&CHANMODEL_FREE_AMPS)
    free(ch->amps);

  for (int i = 0; i<ch->nb_tx*ch->nb_rx; i++) {
    free(ch->ch[i]);
    free(ch->chF[i]);
  }

  for (int i = 0; i<ch->nb_taps; i++) {
    free(ch->a[i]);
  }

  if(ch->free_flags&CHANMODEL_FREE_DELAY)
    free(ch->delays);

  if(ch->free_flags&CHANMODEL_FREE_RSQRT_6)
    for (int i = 0; i<6; i++)
      free(ch->R_sqrt[i]);

  if(ch->free_flags&CHANMODEL_FREE_RSQRT_NTAPS)
    for (int i = 0; i<ch->nb_taps; i++)
      free(ch->R_sqrt[i]);

  free(ch->R_sqrt);
  free(ch->ch);
  free(ch->chF);
  free(ch->a);
  free(ch->model_name);
  free(ch);
}

void set_channeldesc_owner(channel_desc_t *cdesc, uint32_t module_id) {
  cdesc->module_id=module_id;
}

void set_channeldesc_name(channel_desc_t *cdesc,char *modelname) {
  if(cdesc->model_name != NULL)
    free(cdesc->model_name);

  cdesc->model_name=strdup(modelname);
}

int random_channel(channel_desc_t *desc, uint8_t abstraction_flag) {
  double s;
  int i,k,l,aarx,aatx;
  struct complexd anew[desc->nb_tx*desc->nb_rx],acorr[desc->nb_tx*desc->nb_rx];
  struct complexd phase, alpha, beta;
  start_meas(&desc->random_channel);

  for (i=0; i<(int)desc->nb_taps; i++) {
    for (aarx=0; aarx<desc->nb_rx; aarx++) {
      for (aatx=0; aatx<desc->nb_tx; aatx++) {
        anew[aarx+(aatx*desc->nb_rx)].r = sqrt(desc->ricean_factor*desc->amps[i]/2) * gaussdouble(0.0,1.0);
        anew[aarx+(aatx*desc->nb_rx)].i = sqrt(desc->ricean_factor*desc->amps[i]/2) * gaussdouble(0.0,1.0);

        if ((i==0) && (desc->ricean_factor != 1.0)) {
          if (desc->random_aoa==1) {
            desc->aoa = uniformrandom()*2*M_PI;
          }

          // this assumes that both RX and TX have linear antenna arrays with lambda/2 antenna spacing.
          // Furhter it is assumed that the arrays are parallel to each other and that they are far enough apart so
          // that we can safely assume plane wave propagation.
          phase.r = cos(M_PI*((aarx-aatx)*sin(desc->aoa)));
          phase.i = sin(M_PI*((aarx-aatx)*sin(desc->aoa)));
          anew[aarx+(aatx*desc->nb_rx)].r += phase.r * sqrt(1.0-desc->ricean_factor);
          anew[aarx+(aatx*desc->nb_rx)].i += phase.i * sqrt(1.0-desc->ricean_factor);
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
    alpha.r = 1.0;
    alpha.i = 0.0;
    beta.r = 0.0;
    beta.i = 0.0;
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
      alpha.r = sqrt(1-desc->forgetting_factor);
      alpha.i = 0;
      beta.r = sqrt(desc->forgetting_factor);
      beta.i = 0;
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

  //memset((void *)desc->ch[aarx+(aatx*desc->nb_rx)],0,(int)(desc->channel_length)*sizeof(struct complexd));

  if (abstraction_flag==0) {
    start_meas(&desc->interp_time);

    for (aarx=0; aarx<desc->nb_rx; aarx++) {
      for (aatx=0; aatx<desc->nb_tx; aatx++) {
        if (desc->channel_length == 1) {
          desc->ch[aarx+(aatx*desc->nb_rx)][0].r = desc->a[0][aarx+(aatx*desc->nb_rx)].r;
          desc->ch[aarx+(aatx*desc->nb_rx)][0].i = desc->a[0][aarx+(aatx*desc->nb_rx)].i;
        } else {
          for (k=0; k<(int)desc->channel_length; k++) {
            desc->ch[aarx+(aatx*desc->nb_rx)][k].r = 0.0;
            desc->ch[aarx+(aatx*desc->nb_rx)][k].i = 0.0;

            for (l=0; l<desc->nb_taps; l++) {
              if ((k - (desc->delays[l]*desc->sampling_rate) - desc->channel_offset) == 0)
                s = 1.0;
              else
                s = sin(M_PI*(k - (desc->delays[l]*desc->sampling_rate) - desc->channel_offset))/
                    (M_PI*(k - (desc->delays[l]*desc->sampling_rate) - desc->channel_offset));

              desc->ch[aarx+(aatx*desc->nb_rx)][k].r += s*desc->a[l][aarx+(aatx*desc->nb_rx)].r;
              desc->ch[aarx+(aatx*desc->nb_rx)][k].i += s*desc->a[l][aarx+(aatx*desc->nb_rx)].i;
              //        printf("l %d : desc->ch.x %f, s %e, delay %f\n",l,desc->a[l][aarx+(aatx*desc->nb_rx)].x,s,desc->delays[l]);
            } //nb_taps

#ifdef DEBUG_CH
            printf("(%d,%d,%d)->(%e,%e)\n",k,aarx,aatx,desc->ch[aarx+(aatx*desc->nb_rx)][k].x,desc->ch[aarx+(aatx*desc->nb_rx)][k].y);
#endif
          } //channel_length
        }
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
      LOG_E(OCM,"Unknown N_PRB\n");
      return(-1);
  }

  return(channel_bandwidth);
}


static int channelmod_print_help(char *buff, int debug, telnet_printfunc_t prnt ) {
  prnt("channelmod commands can be used to display or modify channel models parameters\n");
  prnt("channelmod show predef: display predefined model algorithms available in oai\n");
  prnt("channelmod show current: display the currently used models in the running executable\n");
  prnt("channelmod modify <model index> <param name> <param value>: set the specified parameters in a current model to the given value\n");
  prnt("                  <model index> specifies the model, the show current model command can be used to list the current models indexes\n");
  prnt("                  <param name> can be one of \"riceanf\", \"aoa\", \"randaoa\", \"ploss\", \"noise_power_dB\", \"offset\", \"forgetf\"\n");
  return CMDSTATUS_FOUND;
}


static void display_channelmodel(channel_desc_t *cd,int debug, telnet_printfunc_t prnt) {
  char *module_id_str[]=MODULEID_STR_INIT;
  prnt("model owner: %s\n",(cd->module_id != 0)?module_id_str[cd->module_id]:"not set");
  prnt("nb_tx: %i    nb_rx: %i    taps: %i bandwidth: %lf    sampling: %lf\n",cd->nb_tx, cd->nb_rx, cd->nb_taps, cd->channel_bandwidth, cd->sampling_rate);
  prnt("channel length: %i    Max path delay: %lf   ricean fact.: %lf    angle of arrival: %lf (randomized:%s)\n",
       cd->channel_length, cd->Td, cd->ricean_factor, cd->aoa, (cd->random_aoa?"Yes":"No"));
  prnt("max Doppler: %lf    path loss: %lf  noise: %lf rchannel offset: %i    forget factor; %lf\n",
       cd->max_Doppler, cd->path_loss_dB, cd->noise_power_dB, cd->channel_offset, cd->forgetting_factor);
  prnt("Initial phase: %lf   nb_path: %i \n",
       cd->ip, cd->nb_paths);

  for (int i=0; i<cd->nb_taps ; i++) {
    prnt("taps: %i   lin. ampli. : %lf    delay: %lf \n",i,cd->amps[i], cd->delays[i]);
  }
}


static int channelmod_show_cmd(char *buff, int debug, telnet_printfunc_t prnt) {
  char *subcmd=NULL;
  int s = sscanf(buff,"%ms\n",&subcmd);

  if (s>0) {
    if ( strcmp(subcmd,"predef") == 0) {
      for (int i=0; channelmod_names[i].name != NULL ; i++) {
        prnt("  %i %s\n", i, map_int_to_str(channelmod_names,i ));
      }
    } else if ( strcmp(subcmd,"current") == 0) {
      for (int i=0; i < max_chan ; i++) {
        if (defined_channels[i] != NULL) {
          prnt("model %i %s type %s: \n----------------\n", i, (defined_channels[i]->model_name !=NULL)?defined_channels[i]->model_name:"(no name set)",
               map_int_to_str(channelmod_names,defined_channels[i]->modelid));
          display_channelmodel(defined_channels[i],debug,prnt);
        }
      }
    } else {
      channelmod_print_help(buff, debug, prnt);
    }

    free(subcmd);
  }

  return CMDSTATUS_FOUND;
}



static int channelmod_modify_cmd(char *buff, int debug, telnet_printfunc_t prnt) {
  char *param=NULL, *value=NULL;
  int cd_id= -1;
  int s = sscanf(buff,"%i %ms %ms \n",&cd_id,&param, &value);

  if (cd_id<0 || cd_id >= max_chan) {
    prnt("ERROR, %i: Channel model id outof range (0-%i)\n",cd_id,max_chan-1);
    return CMDSTATUS_FOUND;
  }

  if (defined_channels[cd_id]==NULL) {
    prnt("ERROR, %i: Channel model has not been set\n",cd_id);
    return CMDSTATUS_FOUND;
  }

  if (s==3) {
    if ( strcmp(param,"riceanf") == 0) {
      double dbl = atof(value);

      if (dbl <0 || dbl > 1)
        prnt("ERROR: ricean factor range: 0 to 1, %lf is outof range\n",dbl);
      else
        defined_channels[cd_id]->ricean_factor=dbl;
    } else if ( strcmp(param,"aoa") == 0) {
      double dbl = atof(value);

      if (dbl <0 || dbl >6.28)
        prnt("ERROR: angle of arrival range: 0 to 2*Pi,  %lf is outof range\n",dbl);
      else
        defined_channels[cd_id]->aoa=dbl;
    } else if ( strcmp(param,"randaoa") == 0) {
      int i = atoi(value);

      if (i!=0 && i!=1)
        prnt("ERROR: randaoa is a boolean, must be 0 or 1\n");
      else
        defined_channels[cd_id]->random_aoa=i;
    } else if ( strcmp(param,"ploss") == 0) {
      double dbl = atof(value);
      defined_channels[cd_id]->path_loss_dB=dbl;
    } else if ( strcmp(param,"noise_power_dB") == 0) {
      double dbl = atof(value);
      defined_channels[cd_id]->noise_power_dB=dbl;
    } else if ( strcmp(param,"offset") == 0) {
      int i = atoi(value);
      defined_channels[cd_id]->channel_offset=i;
    } else if ( strcmp(param,"forgetf") == 0) {
      double dbl = atof(value);

      if (dbl <0 || dbl > 1)
        prnt("ERROR: forgetting factor range: 0 to 1 (disable variation), %lf is outof range\n",dbl);
      else
        defined_channels[cd_id]->forgetting_factor=dbl;
    } else {
      prnt("ERROR: %s, unknown channel parameter\n",param);
      return CMDSTATUS_FOUND;
    }
    display_channelmodel(defined_channels[cd_id],debug,prnt);
    free(param);
    free(value);
    random_channel(defined_channels[cd_id],false);
  }

  return CMDSTATUS_FOUND;
}

int modelid_fromstrtype(char *modeltype) {
  int modelid=map_str_to_int(channelmod_names,modeltype);

  if (modelid < 0)
    LOG_E(OCM,"random_channel.c: Error channel model %s unknown\n",modeltype);

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
  int numparams=sizeof(channelmod_params)/sizeof(paramdef_t);
  int ret = config_get( channelmod_params,numparams,CHANNELMOD_SECTION);
  AssertFatal(ret >= 0, "configuration couldn't be performed");
  defined_channels=calloc(max_chan,sizeof( channel_desc_t *));
  AssertFatal(defined_channels!=NULL, "couldn't allocate %u channel descriptors\n",max_chan);
  /* look for telnet server, if it is loaded, add the channel modeling commands to it */
  add_telnetcmd_func_t addcmd = (add_telnetcmd_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_ADDCMD_FNAME);

  if (addcmd != NULL) {
    addcmd("channelmod",channelmod_vardef,channelmod_cmdarray);
  }
} /* init_channelmod */


int load_channellist(uint8_t nb_tx, uint8_t nb_rx, double sampling_rate, double channel_bandwidth) {
  paramdef_t achannel_params[] = CHANNELMOD_MODEL_PARAMS_DESC;
  paramlist_def_t channel_list;
  memset(&channel_list,0,sizeof(paramlist_def_t));
  memcpy(channel_list.listname,modellist_name,sizeof(channel_list.listname)-1);
  int numparams = sizeof(achannel_params)/sizeof(paramdef_t);
  config_getlist( &channel_list,achannel_params,numparams, CHANNELMOD_SECTION);
  AssertFatal(channel_list.numelt>0, "List %s.%s not found in config file\n",CHANNELMOD_SECTION,channel_list.listname);
  int pindex_NAME = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_NAME_PNAME);
  int pindex_DT = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_DT_PNAME );
  int pindex_FF = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_FF_PNAME );
  int pindex_CO = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_CO_PNAME );
  int pindex_PL = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_PL_PNAME );
  int pindex_NP = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_NP_PNAME );
  int pindex_TYPE = config_paramidx_fromname(achannel_params,numparams, CHANNELMOD_MODEL_TYPE_PNAME);

  for (int i=0; i<channel_list.numelt; i++) {
    int modid = modelid_fromstrtype( *(channel_list.paramarray[i][pindex_TYPE].strptr) );

    if (modid <0) {
      LOG_E(OCM,"Valid channel model types:\n");

      for (int m=0; channelmod_names[i].name != NULL ; m++) {
        printf(" %s ", map_int_to_str(channelmod_names,m ));
      }

      AssertFatal(0, "\n  Choose a valid model type\n");
    }

    channel_desc_t *channeldesc_p = new_channel_desc_scm(nb_tx,nb_rx,modid,sampling_rate,channel_bandwidth,
                                    *(channel_list.paramarray[i][pindex_DT].dblptr), *(channel_list.paramarray[i][pindex_FF].dblptr),
                                    *(channel_list.paramarray[i][pindex_CO].iptr), *(channel_list.paramarray[i][pindex_PL].dblptr),
                                    *(channel_list.paramarray[i][pindex_NP].dblptr) );
    AssertFatal( (channeldesc_p!= NULL), "Could not allocate channel %s type %s \n",*(channel_list.paramarray[i][pindex_NAME].strptr), *(channel_list.paramarray[i][pindex_TYPE].strptr));
    channeldesc_p->model_name = strdup(*(channel_list.paramarray[i][pindex_NAME].strptr));
    LOG_I(OCM,"Model %s type %s allocated from config file, list %s\n",*(channel_list.paramarray[i][pindex_NAME].strptr),
          *(channel_list.paramarray[i][pindex_TYPE].strptr), modellist_name);
  } /* for loop on channel_list */

  return channel_list.numelt;
} /* load_channelist */

#ifdef RANDOM_CHANNEL_MAIN
#define sampling_rate 5.0
#define Td 2.0
main(int argc,char **argv) {
  double amps[8] = {.8,.2,.1,.04,.02,.01,.005};
  struct complexd ch[(int)(1+2*sampling_rate*Td)],phase;
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
