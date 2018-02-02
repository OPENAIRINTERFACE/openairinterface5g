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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <cblas.h>

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "oaisim_config.h"

#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OMG/omg.h"
#include "UTIL/OPT/opt.h" // to test OPT
#endif

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "oaisim.h"

#define PI 3.1416
#define Am 20

#define MCL (-70) /*minimum coupling loss (MCL) in dB*/
//double sinr[NUMBER_OF_eNB_MAX][2*25];
/*
extern double sinr_bler_map[MCS_COUNT][2][16];
extern double sinr_bler_map_up[MCS_COUNT][2][16];
double SINRpost_eff[301];
extern double MI_map_4qam[3][162];
extern double MI_map_16qam[3][197];
extern double MI_map_64qam[3][227];
*/
// Extract the positions of UE and ENB from the mobility model

void extract_position (node_list* input_node_list, node_desc_t **node_data, int nb_nodes)
{

  int i;

  for (i=0; i<nb_nodes; i++) {
    if ((input_node_list != NULL) &&  (node_data[i] != NULL)) {

      node_data[i]->x = input_node_list->node->x_pos;

      if (node_data[i]->x <0.0)
        node_data[i]->x = 0.0;

      node_data[i]->y = input_node_list->node->y_pos;

      if (node_data[i]->y <0.0)
        node_data[i]->y = 0.0;

      LOG_D(OCM, "extract_position: added node_data %d with position X: %f and Y: %f \n", i,input_node_list->node->x_pos, input_node_list->node->y_pos );
      input_node_list = input_node_list->next;
    } else {
      LOG_E(OCM, "extract_position: Null pointer!!!\n");
      //exit(-1);
    }
  }
}
void extract_position_fixed_enb  (node_desc_t **node_data, int nb_nodes, frame_t frame)
{
  int i;

  for (i=0; i<nb_nodes; i++) {
    if (i==0) {
      node_data[i]->x = 0;
      node_data[i]->y = 500;
    } else if (i == 1 ) {
      node_data[i]->x = 866;//
      node_data[i]->y = 1000;
    } else if (i == 2 ) {
      node_data[i]->x = 866;
      node_data[i]->y = 0;
    }
  }
}

void extract_position_fixed_ue  (node_desc_t **node_data, int nb_nodes, frame_t frame)
{
  int i;

  if(frame<50)
    for (i=0; i<nb_nodes; i++) {
      if (i==0) {
        node_data[i]->x = 2050;
        node_data[i]->y = 1500;
      } else {
        node_data[i]->x = 2150;
        node_data[i]->y = 1500;
      }
    }
  else {
    for (i=0; i<nb_nodes; i++) {
      if (i==0) {
        node_data[i]->x = 1856 - (frame - 49);
        // if(node_data[i]->x > 2106)
        //  node_data[i]->x = 2106;
        node_data[i]->y = 1813 + (frame - 49);
        // if(node_data[i]->y < 1563)
        //  node_data[i]->y = 1563;
        // if( node_data[i]->x == 2106)
        //   node_data[i]->x = 2106 - (frame - 49);
      } else {
        node_data[i]->x = 2106 - (frame - 49);
        // if(node_data[i]->x < 1856)
        //  node_data[i]->x = 1856;
        node_data[i]->y = 1563 + (frame - 49);
        // if(node_data[i]->y < 1813)
        //  node_data[i]->y = 1813;
      }
    }
  }

}

void init_ue(node_desc_t  *ue_data, UE_Antenna ue_ant)  //changed from node_struct
{

  ue_data->n_sectors = 1;
  ue_data->phi_rad = 2 * PI;
  ue_data->ant_gain_dBi = ue_ant.antenna_gain_dBi;
  ue_data->tx_power_dBm = ue_ant.tx_power_dBm;
  ue_data->rx_noise_level = ue_ant.rx_noise_level_dB; //value in db

}

void init_enb(node_desc_t  *enb_data, eNB_Antenna enb_ant)  //changed from node_struct
{

  int i;
  double sect_angle[3]= {0,2*PI/3,4*PI/3};

  enb_data->n_sectors = enb_ant.number_of_sectors;

  for (i=0; i<enb_data->n_sectors; i++)
    enb_data->alpha_rad[i] = sect_angle[i]; //enb_ant.alpha_rad[i];

  enb_data->phi_rad = enb_ant.beam_width_dB;
  enb_data->ant_gain_dBi = enb_ant.antenna_gain_dBi;
  enb_data->tx_power_dBm = enb_ant.tx_power_dBm;
  enb_data->rx_noise_level = enb_ant.rx_noise_level_dB;

}



void calc_path_loss(node_desc_t* enb_data, node_desc_t* ue_data, channel_desc_t *ch_desc, Environment_System_Config env_desc, double **Shad_Fad)
{

  double dist;
  double path_loss;
  double gain_max;
  double gain_sec[3];
  double alpha, theta;

  int count;


  dist = sqrt(pow((enb_data->x - ue_data->x), 2) + pow((enb_data->y - ue_data->y), 2));

  path_loss = env_desc.fading.free_space_model_parameters.pathloss_0_dB -
              10*env_desc.fading.free_space_model_parameters.pathloss_exponent * log10(dist/1000);
  LOG_D(OCM,"dist %f, Path loss %f\n",dist,ch_desc->path_loss_dB);

  /* Calculating the angle in the range -pi to pi from the slope */
  alpha = atan2((ue_data->y - enb_data->y),(ue_data->x - enb_data->x));

  if (alpha < 0)
    alpha += 2*PI;

  //printf("angle in radians is %lf\n", ue_data[UE_id]->alpha_rad[eNB_id]);
  ch_desc->aoa = alpha;
  ch_desc->random_aoa = 0;

  if (enb_data->n_sectors==1) //assume omnidirectional antenna
    gain_max = 0;
  else {
    gain_max = -1000;

    for(count = 0; count < enb_data->n_sectors; count++) {
      theta = enb_data->alpha_rad[count] - alpha;
      /* gain = -min(Am , 12 * (theta/theta_3dB)^2) */
      gain_sec[count] = -(Am < (12 * pow((theta/enb_data->phi_rad),2)) ? Am : (12 * pow((theta/enb_data->phi_rad),2)));

      if (gain_sec[count]>gain_max)  //take the sector that we are closest too (or where the gain is maximum)
        gain_max = gain_sec[count];
    }
  }

  path_loss += enb_data->ant_gain_dBi + gain_max + ue_data->ant_gain_dBi;

  if (Shad_Fad!=NULL)
    path_loss += Shad_Fad[(int)ue_data->x][(int)ue_data->y];

  ch_desc->path_loss_dB = MCL < path_loss ?  MCL : path_loss;
  //LOG_D(OCM,"x_coordinate\t%f\t,y_coordinate\t%f\t, path_loss %f\n",ue_data->x,ue_data->y,ch_desc->path_loss_dB);
}





void init_snr(channel_desc_t* eNB2UE, node_desc_t *enb_data, node_desc_t *ue_data, double* sinr_dB, double* N0, uint8_t transmission_mode, uint16_t q, uint8_t dl_power_off, uint16_t nb_rb)
{

  double thermal_noise,abs_channel,channelx, channely,channelx_i, channely_i ;
  int count;
  int aarx,aatx;
  uint8_t qq;

  /* Thermal noise is calculated using 10log10(K*T*B) K = Boltzmann's constant T = room temperature B = bandwidth*/
  thermal_noise = -174 + 10*log10(15000); //per RE; value in dBm

  //for (aarx=0; aarx<eNB2UE->nb_rx; aarx++)
  *N0 = thermal_noise + ue_data->rx_noise_level;

  LOG_D(OCM,"Path loss %lf, noise (N0) %lf, signal %lf, snr %lf\n",
        eNB2UE->path_loss_dB,
        thermal_noise + ue_data->rx_noise_level,
        enb_data->tx_power_dBm + eNB2UE->path_loss_dB,
        enb_data->tx_power_dBm + eNB2UE->path_loss_dB - (thermal_noise + ue_data->rx_noise_level));

  if(transmission_mode == 5 && dl_power_off==1)
    transmission_mode = 6;

  switch(transmission_mode) {
  case 1:

    //printf ("coupling factor is %lf\n", coupling);
    for (count = 0; count < (12 * nb_rb); count++) {
      sinr_dB[count] = enb_data->tx_power_dBm
                       + eNB2UE->path_loss_dB
                       - (thermal_noise + ue_data->rx_noise_level)
                       + 10 * log10 (pow(eNB2UE->chF[0][count].x, 2)
                                     + pow(eNB2UE->chF[0][count].y, 2));
      //printf("sinr_dB[%d]: %1f\n",count,sinr_dB[count]);
      //printf("Dl_link SNR for res. block %d is %lf\n", count, sinr[eNB_id][count]);
    }

    break;

  case 2:

    for (count = 0; count < (12 * nb_rb); count++) {
      abs_channel=0;

      for (aarx=0; aarx<eNB2UE->nb_rx; aarx++) {
        for (aatx=0; aatx<eNB2UE->nb_tx; aatx++) {
          abs_channel += (pow(eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x, 2) + pow(eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y, 2));
        }
      }

      sinr_dB[count] = enb_data->tx_power_dBm
                       + eNB2UE->path_loss_dB
                       - (thermal_noise + ue_data->rx_noise_level)
                       + 10 * log10 (abs_channel/2);
      // printf("sinr_dB[%d]: %1f\n",count,sinr_dB[count]);
    }

    break;

  case 5:
    for (count = 0; count < (12 * nb_rb); count++) {
      channelx=0;
      channely=0;
      channelx_i=0;
      channely_i=0;
      qq = (q>>(((count/12)>>2)<<1))&3;
      //printf("pmi_alloc %d: rb %d, pmi %d\n",q,count/12,qq);



      //      qq = q;
      for (aarx=0; aarx<eNB2UE->nb_rx; aarx++) {
        for (aatx=0; aatx<eNB2UE->nb_tx; aatx++) {
          switch(qq) {
          case 0:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channelx_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channelx_i -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely_i -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            }

            break;

          case 1:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channelx_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channelx_i += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely_i += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            }

            break;

          case 2:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channelx_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channely += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channelx_i += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channely_i -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;

            }

            break;

          case 3:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channelx_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely_i = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channely -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channelx_i -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channely_i += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
            }

            break;

          default:
            LOG_E(EMU,"Problem in SINR Calculation for TM5 \n");
            break;

          }//switch(q)

        }//aatx
      }//aarx

      /*  sinr_dB[count] = enb_data->tx_power_dBm
         + eNB2UE->path_loss_dB
        - (thermal_noise + ue_data->rx_noise_level)
        + 10 * log10 ((pow(channelx,2) + pow(channely,2))/2) - 10 * log10 ((pow(channelx_i,2) + pow(channely_i,2))/2);
      */
      sinr_dB[count] = enb_data->tx_power_dBm
                       + eNB2UE->path_loss_dB
                       - (thermal_noise + ue_data->rx_noise_level)
                       + 10 * log10 ((pow(channelx,2) + pow(channely,2))) - 10 * log10 ((pow(channelx_i,2) + pow(channely_i,2))) - 3; // 3dB is subtracted as the tx_power_dBm is to be adjusted on per user basis
      // printf("sinr_dB[%d]: %1f\n",count,sinr_dB[count]);
    }

    break;

  case 6:
    for (count = 0; count < (12 * nb_rb); count++) {
      channelx=0;
      channely=0;
      qq = (q>>(((count/12)>>2)<<1))&3;
      //printf("pmi_alloc %d: rb %d, pmi %d\n",q,count/12,qq);



      //      qq = q;
      for (aarx=0; aarx<eNB2UE->nb_rx; aarx++) {
        for (aatx=0; aatx<eNB2UE->nb_tx; aatx++) {
          switch(qq) {
          case 0:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            }

            break;

          case 1:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            }

            break;

          case 2:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channely += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
            }

            break;

          case 3:
            if (channelx==0 || channely==0) {
              channelx = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
              channely = eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
            } else {
              channelx += eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].y;
              channely -= eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][count].x;
            }

            break;

          default:
            LOG_E(EMU,"Problem in SINR Calculation for TM6 \n");
            break;

          }//switch(q)

        }//aatx
      }//aarx

      sinr_dB[count] = enb_data->tx_power_dBm
                       + eNB2UE->path_loss_dB
                       - (thermal_noise + ue_data->rx_noise_level)
                       + 10 * log10 ((pow(channelx,2) + pow(channely,2))/2);

      // printf("sinr_dB[%d]: %1f\n",count,sinr_dB[count]);
    }

    break;

  default:
    LOG_E(EMU,"Problem in SINR Initialization in sinr_sim.c\n");
    break;
  }//switch
}//function ends

void calculate_sinr(channel_desc_t* eNB2UE, node_desc_t *enb_data, node_desc_t *ue_data, double *sinr_dB, uint16_t nb_rb)
{

  double sir, thermal_noise;
  short count;

  /* Thermal noise is calculated using 10log10(K*T*B) K = Boltzmann's constant T = room temperature B = bandwidth */
  thermal_noise = -174 + 10*log10(15000); //per RE, value in dBm

  for (count = 0; count < 12 * nb_rb; count++) {
    sir = enb_data->tx_power_dBm
          + eNB2UE->path_loss_dB
          - (thermal_noise + ue_data->rx_noise_level)
          + 10 * log10 (pow(eNB2UE->chF[0][count].x, 2)
                        + pow(eNB2UE->chF[0][count].y, 2));

    if (sir > 0)
      sinr_dB[count] -= sir;

    //printf("*****sinr% lf \n",sinr_dB[count]);
  }
}
void get_beta_map()
{
  char *file_path = NULL;
  //int table_len = 0;
  int t,u;
  int mcs = 0;
  char *sinr_bler;
  char buffer[1000];
  FILE *fp;
  double perf_array[13];

  file_path = (char*) malloc(512);

  for (mcs = 0; mcs < MCS_COUNT; mcs++) {
    snprintf( file_path, 512, "%s/SIMULATION/LTE_PHY/BLER_SIMULATIONS/AWGN/AWGN_results/bler_tx1_chan18_nrx1_mcs%d.csv", getenv("OPENAIR1_DIR"), mcs );
    fp = fopen(file_path,"r");

    if (fp == NULL) {
      LOG_E(OCM,"ERROR: Unable to open the file %s! Exitng.\n", file_path);
      exit(-1);
    }

    // else {
    if (fgets (buffer, 1000, fp) != NULL) {
      if (fgets (buffer, 1000, fp) != NULL) {
        table_length[mcs] = 0;

        while (!feof (fp)) {
          u = 0;
          sinr_bler = strtok (buffer, ";");

          while (sinr_bler != NULL) {
            perf_array[u] = atof (sinr_bler);
            u++;
            sinr_bler = strtok (NULL, ";");
          }

          if ((perf_array[4] / perf_array[5]) < 1) {
            sinr_bler_map[mcs][0][table_length[mcs]] = perf_array[0];
            sinr_bler_map[mcs][1][table_length[mcs]] = (perf_array[4] / perf_array[5]);
            table_length[mcs]++;

            if (table_length[mcs]>MCS_TABLE_LENGTH_MAX) {
              LOG_E(OCM,"Error reading MCS table. Increase MCS_TABLE_LENGTH_MAX (mcs %d)!\n",mcs);
              exit(-1);
            }
          }

          if (fgets (buffer, 1000, fp) != NULL) {
          }
        }
      }
    }

    fclose(fp);
    //   }
    LOG_D(OCM,"Print the table for mcs %d\n",mcs);

    for (t = 0; t<table_length[mcs]; t++)
      LOG_D(OCM,"%lf  %lf \n ",sinr_bler_map[mcs][0][t],sinr_bler_map[mcs][1][t]);
  }

  free(file_path);
}

//this function reads and stores the Mutual information tables for the MIESM abstraction.
void get_MIESM_param()
{
  char *file_path = NULL;
  char buffer[10000];
  FILE *fp;
  int qam[3] = {4,16,64};
  int q,cnt;
  char *result = NULL;
  int table_len=0;
  int t;
  file_path = (char*) malloc(512);

  for (q=0; q<3; q++) {
    sprintf(file_path,"%s/SIMU/USER/files/MI_%dqam.csv",getenv("OPENAIR_TARGETS"),qam[q]);
    fp = fopen(file_path,"r");

    if (fp == NULL) {
      printf("ERROR: Unable to open the file %s\n", file_path);
      exit(-1);
    } else {
      cnt=-1;

      switch(qam[q]) {
      case 4:
        while (!feof(fp)) {
          table_len =0;
          cnt++;

          if (fgets(buffer, 10000, fp) != NULL) {
            result = strtok (buffer, ",");

            while (result != NULL) {
              MI_map_4qam[cnt][table_len] = atof (result);
              result = strtok (NULL, ",");
              table_len++;
            }
          }
        }

        for (t = 0; t < 162; t++) {
          // MI_map_4Qam[0][t] = pow(10,0.1*(MI_map_4Qam[0][t]));
          LOG_D(OCM, "MIESM 4QAM Table: %lf  %lf  %1f\n ",MI_map_4qam[0][t],MI_map_4qam[1][t], MI_map_4qam[2][t]);
          // printf("MIESM 4QAM Table: %lf  %lf  %1f\n ",MI_map_4qam[0][t],MI_map_4qam[1][t], MI_map_4qam[2][t]);
        }

        break;

      case 16:
        while (!feof(fp)) {
          table_len =0;
          cnt++;

          if (fgets (buffer, 10000, fp) != NULL) {
            result = strtok (buffer, ",");

            while (result != NULL) {
              MI_map_16qam[cnt][table_len] = atof (result);
              result = strtok (NULL, ",");
              table_len++;
            }
          }
        }

        for (t = 0; t < 197; t++) {
          // MI_map_16Qam[0][t] = pow(10,0.1*(MI_map_16Qam[0][t]));
          LOG_D(OCM, "MIESM 16 QAM Table: %lf  %lf  %1f\n ",MI_map_16qam[0][t],MI_map_16qam[1][t], MI_map_16qam[2][t]);
          // printf("MIESM 16 QAM Table: %lf  %lf  %1f\n ",MI_map_16qam[0][t],MI_map_16qam[1][t], MI_map_16qam[2][t]);
        }

        break;

      case 64:
        while (!feof(fp)) {
          table_len=0;
          cnt++;

          if(cnt==3)
            break;

          if (fgets (buffer, 10000, fp) != NULL) {
            result = strtok(buffer, ",");

            while (result != NULL) {
              MI_map_64qam[cnt][table_len]= atof(result);
              result = strtok(NULL, ",");
              table_len++;
            }
          }
        }

        for (t = 0; t < 227; t++) {
          //MI_map_64Qam[0][t] = pow(10,0.1*(MI_map_64Qam[0][t]));
          LOG_D(OCM, "MIESM 64QAM Table: %lf  %lf  %1f\n ",MI_map_64qam[0][t],MI_map_64qam[1][t], MI_map_64qam[2][t]);
          // printf("MIESM 64QAM Table: %lf  %lf  %1f\n ",MI_map_64qam[0][t],MI_map_64qam[1][t], MI_map_64qam[2][t]);
        }

        break;

      default:
        LOG_E(EMU,"Error, bad input, quitting\n");
        break;
      }

    }

    fclose(fp);
  }

  free(file_path);
}





