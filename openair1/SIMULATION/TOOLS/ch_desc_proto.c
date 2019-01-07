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
#include <math.h>
#include <string.h>
#include "defs.h"
#define PI 3.1416
#define Am 20
#define MCS_COUNT 24
#define MCL 70 /*minimum coupling loss (MCL) in dB*/
#define theta_3dB (65*PI/180)
enum sector {SEC1, SEC2, SEC3};
scenario_desc_t scenario;

void get_chan_desc(node_desc_t* node_tx, node_desc_t* node_rx, channel_desc_t *ch_desc, scenario_desc_t* scenario)
{
  double dist;
  dist = sqrt(pow((node_tx->x - node_rx->x), 2) + pow((node_tx->y - node_rx->y), 2));

  /* conversion of distance into KM 3gpp (36-942)*/
  ch_desc->path_loss_dB = (128.1 + 37.6 * log10(dist/1000));
}

int main()
{
  int enb_count = 16;
  int ue_count = 50;
  double sect_angle[3]= {0,2*PI/3,4*PI/3};
  double gain_max;
  double theta;
  double min_path_loss = 0;
  int att_enb_index;
  node_desc_t *enb_data[enb_count];
  node_desc_t *ue_data[ue_count];
  channel_desc_t *ul_channel[ue_count][enb_count];
  channel_desc_t *dl_channel[ue_count][enb_count];
  int count;
  int mcs;
  int ue_index, enb_index;
  int return_value;
  int nb_rb = 25; //No. of resource blocks
  double sinr[enb_count][2*nb_rb];
  double sinr_eff[ue_count][MCS_COUNT];
  double bler[ue_count][MCS_COUNT];
  double gain_sec[3];
  double thermal_noise;
  double interference;
  double coupling;
  FILE *fp;
  char buffer[100];
  char *sinr_bler;
  double tlu_sinr;
  double tlu_bler;
  int line_num;
  char *file_name[]= {"bler_1.csv", "bler_2.csv", "bler_3.csv", "bler_4.csv", "bler_5.csv", "bler_6.csv", "bler_7.csv", "bler_8.csv",
                      "bler_9.csv", "bler_10.csv", "bler_11.csv", "bler_12.csv", "bler_13.csv", "bler_14.csv", "bler_15.csv", "bler_16.csv",
                      "bler_17.csv", "bler_18.csv", "bler_19.csv", "bler_20.csv", "bler_21.csv", "bler_22.csv"
                     };

  double beta[MCS_COUNT] = {0, 0, 0, 0, 0.9459960937499999, 1.2912109374999994, 1.0133789062499998, 1.000390625,
                            1.02392578125, 1.8595703124999998, 2.424389648437498, 2.3946533203124982, 2.5790039062499988,
                            2.4084960937499984, 2.782617187499999, 2.7868652343749996, 3.92099609375, 4.0392578125,
                            4.56109619140625, 5.03338623046875, 5.810888671875, 6.449108886718749
                           };

  double enb_position[][2] = {{1100,1100},{1100,2100},{1100,3100},{1100,4100},
    {2100,1100},{2100,2100},{2100,3100},{2100,4100},
    {3100,1100},{3100,2100},{3100,3100},{3100,4100},
    {4100,1100},{4100,2100},{4100,3100},{4100,4100}
  };

  double ue_position[][2] = {{3340,4740},{1500,620},{1780,4220},{1300,3540},{780,3100},
    {1140,540},{1340,3660},{860,1220},{2700,2140},{3860,3060},
    {3740,1060},{1700,3060},{2180,1620},{4420,1060},{1300,3340},
    {3700,3180},{3780,540},{1700,4380},{4140,4740},{820,4380},
    {3300,1540},{2100,1780},{1780,2260},{1940,2620},{1580,1700},
    {1460,1940},{940,1340},{2100,3540},{1260,4340},{2940,4060},
    {3980,940},{540,2220},{3060,2140},{4620,3940},{4260,2820},
    {3860,3500},{4140,4140},{3900,3500},{1500,2140},{2620,3820},
    {3420,2820},{1580,3940},{660,2100},{2740,1180},{2500,2500},
    {3580,3580},{3740,3140},{3020,3020},{4340,4140},{980,4300}
  };

  randominit(0);

  /////////////////////////////////////////////////////////////////////////////////////////////////
  int tabl_len=0;
  double local_table[MCS_COUNT][9][9];

  for (mcs = 5; mcs <= MCS_COUNT; mcs++) {

    fp = fopen(file_name[mcs - 1],"r");

    if (fp == NULL) {
      printf("ERROR: Unable to open the file\n");
    } else {
      fgets(buffer, 100, fp);
      tabl_len=0;

      while (!feof(fp)) {

        sinr_bler = strtok(buffer, ";");
        local_table[mcs-1][0][tabl_len] = atof(sinr_bler);
        sinr_bler = strtok(NULL,";");
        local_table[mcs-1][1][tabl_len] = atof(sinr_bler);
        tabl_len++;
        fgets(buffer, 100, fp);
      }

      fclose(fp);
    }

    printf("\n table for mcs %d\n",mcs);

    for (tabl_len=0; tabl_len<9; tabl_len++)
      printf("%lf  %lf \n ",local_table[mcs-1][0][tabl_len],local_table[mcs-1][1][tabl_len]);


  }



  ////////////////////////////////////////////////////////////////////////////////////////////////////

  for (enb_index = 0; enb_index < enb_count; enb_index++)
    enb_data[enb_index] = (node_desc_t *)(malloc(sizeof(node_desc_t)));

  for (ue_index = 0; ue_index < ue_count; ue_index++)
    ue_data[ue_index] = (node_desc_t *)(malloc(sizeof(node_desc_t)));

  for (enb_index = 0; enb_index < enb_count; enb_index++)  {
    enb_data[enb_index]->x = enb_position[enb_index][0];
    enb_data[enb_index]->y = enb_position[enb_index][1];
    enb_data[enb_index]->tx_power_dBm = 40;
    enb_data[enb_index]->ant_gain_dBi = 15;
    enb_data[enb_index]->rx_noise_level = 5; //value in db
    enb_data[enb_index]->n_sectors = 3;
  }

  for (ue_index = 0; ue_index < ue_count; ue_index++)  {
    ue_data[ue_index]->x = ue_position[ue_index][0];
    ue_data[ue_index]->y = ue_position[ue_index][1];
    ue_data[ue_index]->phi_rad = 2 * PI;
    ue_data[ue_index]->tx_power_dBm = 20;
    ue_data[ue_index]->ant_gain_dBi = 0;
    ue_data[ue_index]->rx_noise_level = 9; //value in db
  }

  for (ue_index = 0; ue_index < ue_count; ue_index++) {
    min_path_loss = 10000;

    for (enb_index = 0; enb_index < enb_count; enb_index++) {
      ul_channel[ue_index][enb_index] = new_channel_desc_scm(1, 1, SCM_C, 7.68, 0, 0, 0);
      dl_channel[ue_index][enb_index] = new_channel_desc_scm(1, 1, SCM_C, 7.68, 0, 0, 0);
      //printf("ue %d enb %d\n", ue_index, enb_index);

      /* Calculating the angle in the range -pi to pi from the slope */
      //(ue_data[ue_index])->alpha_rad[enb_index] = (double)(atan2((ue_data[ue_index]->x - enb_data[enb_index]->x), (ue_data[ue_index]->y - enb_data[enb_index]->y)));
      ue_data[ue_index]->alpha_rad[enb_index] = atan2((ue_data[ue_index]->x - enb_data[enb_index]->x), (ue_data[ue_index]->y - enb_data[enb_index]->y));
      //printf("angle is tan %lf\n", ue_data[ue_index]->alpha_rad[enb_index]);

      if ((ue_data[ue_index]->alpha_rad[enb_index]) < 0) {
        ue_data[ue_index]->alpha_rad[enb_index] = 2*PI + ue_data[ue_index]->alpha_rad[enb_index];
        //printf("angle in radians is %lf\n", ue_data[ue_index]->alpha_rad[enb_index]);
      }

      for(count = 0; count < enb_data[enb_index]->n_sectors; count++) {
        theta = sect_angle[count] - ue_data[ue_index]->alpha_rad[enb_index];
        gain_sec[count] = -(Am < (12 * pow((theta/theta_3dB),2)) ? Am : (12 * pow((theta/theta_3dB),2)));
      }

      /* gain = -min(Am , 12 * (theta/theta_3dB)^2) */
      gain_max = (gain_sec[SEC1] > gain_sec[SEC2]) ? ((gain_sec[SEC1] > gain_sec[SEC3]) ? gain_sec[SEC1]:gain_sec[SEC3]) :
                   ((gain_sec[SEC2] > gain_sec[SEC3]) ? gain_sec[SEC2]:gain_sec[SEC3]);

      get_chan_desc(enb_data[enb_index], ue_data[ue_index], ul_channel[ue_index][enb_index], &scenario);
      get_chan_desc(enb_data[enb_index], ue_data[ue_index], dl_channel[ue_index][enb_index], &scenario);

      //printf("Path loss for link between ue %d and enb %d is %lf and gain is %lf \n", ue_index, enb_index, dl_channel[ue_index][enb_index]->path_loss_dB, gain_max);

      if (dl_channel[ue_index][enb_index]->path_loss_dB < min_path_loss) {
        min_path_loss = dl_channel[ue_index][enb_index]->path_loss_dB;
        att_enb_index = enb_index;
      }

      //return_value = random_channel(ul_channel[ue_index][enb_index]);
      return_value = random_channel(dl_channel[ue_index][enb_index]);

      /* Thermal noise is calculated using 10log10(K*T*B) K = Boltzmann´s constant T = room temperature B = bandwidth */
      /* Taken as constant for the time being since the BW is not changing */
      thermal_noise = -105; //value in dBm

      if (0 == return_value) {
        //freq_channel(ul_channel[ue_index][enb_index], nb_rb);
        freq_channel(dl_channel[ue_index][enb_index], nb_rb);
        coupling = MCL > (dl_channel[ue_index][enb_index]->path_loss_dB-(enb_data[enb_index]->ant_gain_dBi + gain_max)) ?
                   MCL : (dl_channel[ue_index][enb_index]->path_loss_dB-(enb_data[enb_index]->ant_gain_dBi + gain_max));

        //printf ("coupling factor is %lf\n", coupling);
        for (count = 0; count < (2 * nb_rb); count++) {
          sinr[enb_index][count] = enb_data[enb_index]->tx_power_dBm
                                   - coupling
                                   - (thermal_noise + ue_data[ue_index]->rx_noise_level)
                                   + 10 * log10 (pow(dl_channel[ue_index][enb_index]->chF[0][count].r, 2)
                                                 + pow(dl_channel[ue_index][enb_index]->chF[0][count].i, 2));

          //printf("Dl_link SNR for res. block %d is %lf\n", count, sinr[enb_index][count]);
        }
      }
    }

    for (count = 0; count < 2 * nb_rb; count++) {
      interference = 0;

      for (enb_index = 0; enb_index < enb_count; enb_index++) {
        if (att_enb_index != enb_index) {
          interference += pow(10, 0.1 * sinr[enb_index][count]);
        }
      }

      sinr[att_enb_index][count] -= 10*log10(1 + interference);

      //printf("***Dl_link SINR for res. block %d is %lf\n", count, sinr[att_enb_index][count]);

      for (mcs = 5; mcs <= MCS_COUNT; mcs++) {
        sinr_eff[ue_index][mcs-1] += exp(-(pow(10, (sinr[att_enb_index][count])/10))/beta[mcs-1]);
        //printf("Effective snr   %lf\n",sinr_eff[ue_index][mcs-1]);
        //sinr_eff[ue_index][mcs] += exp(-(sinr[att_enb_index][count])/beta[mcs]);
      }
    }

    for (mcs = 5; mcs <= MCS_COUNT; mcs++) {
      //printf("mcs value  %d \n",mcs);
      //printf("beta value  %lf \n",-beta[mcs-1]);
      //printf("snr_eff value  %lf \n",log(sinr_eff[ue_index][mcs-1]));

      sinr_eff[ue_index][mcs-1] =  -beta[mcs-1] *log((sinr_eff[ue_index][mcs-1])/(2*nb_rb));//
      //printf("snr_eff value  %lf \n",sinr_eff[ue_index][mcs-1]);
      sinr_eff[ue_index][mcs-1] = 10 * log10(sinr_eff[ue_index][mcs-1]);
      sinr_eff[ue_index][mcs-1] *= 10;
      sinr_eff[ue_index][mcs-1] = floor(sinr_eff[ue_index][mcs-1]);

      if ((int)sinr_eff[ue_index][mcs-1]%2) {
        sinr_eff[ue_index][mcs-1] += 1;
      }

      sinr_eff[ue_index][mcs-1] /= 10;

      //printf("Effective snr   %lf  \n",sinr_eff[ue_index][mcs-1]);

      bler[ue_index][mcs-1] = 0;

      /*line_num = 0;
      fp = fopen(file_name[mcs - 1],"r");
      if (fp == NULL) {
        printf("ERROR: Unable to open the file\n");
      }
      else {
        fgets(buffer, 100, fp);
        while (!feof(fp)) {
      line_num++;
      sinr_bler = strtok(buffer, ";");
      tlu_sinr = atof(sinr_bler);
      sinr_bler = strtok(NULL,";");
      tlu_bler = atof(sinr_bler);
      if (1 == line_num) {
      if (sinr_eff[ue_index][mcs-1] < tlu_sinr) {
      bler[ue_index][mcs-1] = 1;
      break;
      }
      }
      if (sinr_eff[ue_index][mcs-1] == tlu_sinr) {
      bler[ue_index][mcs-1] = tlu_bler;
      }
      fgets(buffer, 100, fp);
        }
        fclose(fp);*/
      for (tabl_len=0; tabl_len<9; tabl_len++) {

        if(tabl_len==0)
          if (sinr_eff[ue_index][mcs-1] < local_table[mcs-1][0][tabl_len]) {
            bler[ue_index][mcs-1] = 1;
            break;
          }


        if (sinr_eff[ue_index][mcs-1] == local_table[mcs-1][0][tabl_len]) {
          bler[ue_index][mcs-1] = local_table[mcs-1][1][tabl_len];
        }

      }

      //printf("\n###Dl_link UE %d attached to eNB %d \n MCS %d effective SNR %lf BLER %lf", ue_index, att_enb_index, mcs,sinr_eff[ue_index][mcs-1],bler[ue_index][mcs-1]);
    }

    //printf("\n\n");

    printf("\n     Ue_ix enb_ix  mcs5    mcs6    mcs7    mcs8    mcs9   mcs10   mcs11   mcs12   mcs13\
   mcs14   mcs15   mcs16   mcs17   mcs18   mcs19   mcs20   mcs21   mcs22\n");

    printf("SINR %4d   %4d  %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f\
   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f\n",
           ue_index, att_enb_index, sinr_eff[ue_index][4], sinr_eff[ue_index][5], sinr_eff[ue_index][6], sinr_eff[ue_index][7],
           sinr_eff[ue_index][8], sinr_eff[ue_index][9], sinr_eff[ue_index][10], sinr_eff[ue_index][11], sinr_eff[ue_index][12],
           sinr_eff[ue_index][13], sinr_eff[ue_index][14], sinr_eff[ue_index][15], sinr_eff[ue_index][16], sinr_eff[ue_index][17],
           sinr_eff[ue_index][18], sinr_eff[ue_index][19], sinr_eff[ue_index][20], sinr_eff[ue_index][21], sinr_eff[ue_index][22]);

    printf("BLER %4d   %4d  %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f\
   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f   %+4.2f\n",
           ue_index, att_enb_index, bler[ue_index][4], bler[ue_index][5], bler[ue_index][6], bler[ue_index][7],
           bler[ue_index][8], bler[ue_index][9], bler[ue_index][10], bler[ue_index][11], bler[ue_index][12],
           bler[ue_index][13], bler[ue_index][14], bler[ue_index][15], bler[ue_index][16], bler[ue_index][17],
           bler[ue_index][18], bler[ue_index][19], bler[ue_index][20], bler[ue_index][21], bler[ue_index][22]);
  }
}

