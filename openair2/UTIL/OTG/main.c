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

/*! \file main.c
* \brief main program for OTG CX + RX
* \author A. Hafsaoui
* \date 2011
* \version 0.1
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "otg_tx.h"
#include "otg_rx.h"
#include "otg_kpi.h"
#include "otg.h"
#include "otg_tx_socket.h"
#include "otg_vars.h"
#include "traffic_config.h"

#include "../MATH/oml.h"



int SIMU_TIME=1200000;
int simu_time=0, duration=0, seed=0, simu_mode=0;


// init OTG with config parameters

void init_config_otg(char *protocol, char *ip_version) {
  int i, j, k;

  if (simu_time>0)
    SIMU_TIME=simu_time;

  printf("OTG :: INIT CONFIG\n");

  if (seed>0)
    g_otg->seed=seed;
  else
    g_otg->seed=ceil(uniform_rng()*67534);

  printf("duration %d, seeds %d \n", duration, g_otg->seed);

  for (i=0; i<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); i++) {
    if (duration>0)
      g_otg->duration[i]=duration;
    else
      g_otg->duration[i]=10000;

    g_otg->dst_port[i]=DST_PORT;
    g_otg->dst_ip[i]=(char *)malloc(100*sizeof(char *));
    g_otg->dst_ip[i]=DST_IP;
    g_otg->dst_ip[i]=DST_IP;

    //config ip version

    if (ip_version !=NULL) {
      if ((strcmp(ip_version,"IP4")==0) ||(strcmp(ip_version,"ip4")==0))
        g_otg->ip_v[i]=IPV4;
      else if ((strcmp(ip_version,"IP6")==0) ||(strcmp(ip_version,"ip6")==0))
        g_otg->ip_v[i]=IPV6;
    } else
      g_otg->ip_v[i]=IPV4;

    //config transport protocol version
    if (protocol!=NULL) {
      if ((strcmp(protocol,"TCP")==0) ||(strcmp(protocol,"tcp")==0))
        g_otg->trans_proto[i]=TCP;
      else if ((strcmp(protocol,"UDP")==0) ||(strcmp(protocol,"udp")==0))
        g_otg->trans_proto[i]=UDP;
    } else
      g_otg->trans_proto[i]=TCP;

    for (j=0; j<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); j++) {
      g_otg->application_type[i][j]=OPENARENA;

      for (k=0; k<MAX_NUM_TRAFFIC_STATE; k++) {
        LOG_I(OTG,"INIT CONFIG Source =%d, Destination =%d,State =%d \n",i, j, k);
        g_otg->idt_dist[i][j][k]=IDT_DIST;
        g_otg->idt_min[i][j][k]=IDT_MIN;
        g_otg->idt_max[i][j][k]=IDT_MAX;
        g_otg->idt_std_dev[i][j][k]=IDT_STD_DEV;
        g_otg->idt_lambda[i][j][k]=IDT_LAMBDA;
        g_otg->size_dist[i][j][k]=PKTS_SIZE_DIST;
        g_otg->size_min[i][j][k]=PKTS_SIZE_MIN;
        g_otg->size_max[i][j][k]=PKTS_SIZE_MAX;
        g_otg->size_std_dev[i][j][k]=PKTS_SIZE_STD_DEV;
        g_otg->size_lambda[i][j][k]=PKTS_SIZE_LAMBDA;
      }
    }
  }
}



int main_below_ip() {
  int i, j, k, l, rtt_owd ,rx_otg=0, simu_time=0, ctime=0, nb_round=0;
  float p;
  char *packet;
  int rx_packet_out;
  int pkt_size;
  printf(" max enb %d, max ue %d \n", NUMBER_OF_eNB_MAX, NUMBER_OF_UE_MAX);

  do {
    nb_round=nb_round+1;

    // for (stime=0; stime < SIMU_TIME; stime++) // discrete event generation : tick, stime generate the ctime
    for (i=0; i<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); i++) {
      for (j=0; j<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); j++) {
        for (k=0; k<MAX_NUM_TRAFFIC_STATE; k++) {
          LOG_I(OTG,"OTG emulation src=%d, dst=%d, state=%d \n", i, j, k);
          ctime=0; // set the ctime to 0

          do {
            if (simu_time> SIMU_TIME) {
              otg_info->ctime=SIMU_TIME;
              return(0);
            }

            LOG_I(OTG,"val :: ctime=%d\n", ctime);
            char *packet=NULL;
            /*packet=packet_gen(i, j, k, ctime);*/ packet=packet_gen(i, j, ctime, &pkt_size);

            if (packet!=NULL) {
              if ((ceil(g_otg->duration[i]*uniform_rng()))==ctime)  {
                printf("DROP PACKET (i=%d,j=%d) seq num=%d\n",i, j, (int)otg_info->seq_num[i][j]);
              } else  {
                printf("SEND PACKET (i=%d,j=%d) seq num=%d\n",i, j, (int)otg_info->seq_num[i][j]);
                rtt_owd=ceil(uniform_rng()*8.56);
                LOG_I(OTG,"one way delay= %d , (src=%d, dst=%d, state=%d)\n", rtt_owd, i, j, k);
                ctime+=rtt_owd;
                otg_info->rx_pkt_owd[i][j]=rtt_owd;
                simu_time+=rtt_owd;
                //rx_packet_out=check_packet(i, j, ctime, packet);
                rx_packet_out=otg_rx_pkt(i,j, ctime, packet, pkt_size);
                //if (rx_packet_out==NULL)
                //  LOG_I(OTG,"PKTS INFO:: DROPED\n");
                //else{
                //  if (rx_packet_out!=NULL){
                //    rx_packet_out=NULL;
                free(packet);
                //  }
                //}
                //Do not increase the ctime and simu_time with the one way delay.
                ctime-=rtt_owd;
                simu_time-=rtt_owd;
                LOG_I(OTG,"PKTS INFO:: (src=%d, dst=%d, state=%d),NB PKTS=%d  ,sequence NB=%d,  RTT (one way)ms= %d \n ",
                      i, j, k, (int)otg_info->tx_num_pkt[i][j], (int)otg_info->seq_num[i][j], (int)otg_info->rx_pkt_owd[i][j]);
              }
            } else
              printf("Node (i=%d,j=%d) seq num=%d, ctime %d, prb %lf\n",i, j, (int)otg_info->seq_num[i][j], ctime,(ceil(g_otg->duration[i]*uniform_rng())));

            LOG_I(OTG,"Time:: ctime=%d, duration=%d, simu_time=%d, max=%d, (src=%d, dst=%d, state=%d) \n", ctime,  g_otg->duration[i],simu_time, SIMU_TIME, i, j,k);
            ctime+=1;
            simu_time+=1;
          } while (ctime<=g_otg->duration[i]) ;
        }

        if  (otg_info->tx_num_pkt[i][j]>otg_info->rx_num_pkt[i][j])
          LOG_I(OTG,"STAT: (LOSS):: (src=%d, dst=%d) NB packet TX= %d,  NB packet RX = %d, seq NUM=%d\n ",
                i, j, (int)otg_info->tx_num_pkt[i][j], (int)otg_info->rx_num_pkt[i][j],(int)otg_info->seq_num[i][j] );
        else
          LOG_I(OTG,"STAT: :: (src=%d, dst=%d) NB packet TX= %d,  NB packet RX= %d, seq NUM=%d \n ",
                i, j, (int)otg_info->tx_num_pkt[i][j], (int)otg_info->rx_num_pkt[i][j], (int)otg_info->seq_num[i][j]);
      }
    }
  } while (simu_time<=SIMU_TIME);
}


int main_above_ip() {
  int i, j, k,  simu_time=0, ctime=0, nb_round=0;
  char *packet=NULL;
  printf(" max enb %d, max ue %d \n", NUMBER_OF_eNB_MAX, NUMBER_OF_UE_MAX);

  do {
    nb_round=nb_round+1;

    for (i=0; i<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); i++) {
      for (j=0; j<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); j++) {
        for (k=0; k<MAX_NUM_TRAFFIC_STATE; k++) {
          LOG_I(OTG,"SOCKET:: OTG emulation src=%d, dst=%d, state=%d \n", i, j, k);
          ctime=0;
          //    socket_packet_send(i, j, k, ctime);
        }
      }
    }

    ctime+=1;
    simu_time+=1;
  } while (simu_time<=SIMU_TIME);
}





void config_traffic_type(char *traffic) {
  Application application;
  int i,j;

  if (strcmp(traffic, "SCBR")==0)
    application=SCBR;
  else if (strcmp(traffic, "OPENARENA")==0)
    application=OPENARENA;
  else if (strcmp(traffic, "BICYCLE_RACE")==0)
    application=BICYCLE_RACE;
  else if (strcmp(traffic, "AUTO_PILOT")==0)
    application=AUTO_PILOT;
  else if (strcmp(traffic, "TEAM_FORTRESS")==0)
    application=TEAM_FORTRESS;
  else
    application=NO_PREDEFINED_TRAFFIC;

  printf("Config Application: %d \n", application);

  for (i=0; i<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); i++) {
    for (j=0; j<(NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX); j++) {
      g_otg->application_type[i][j] =application;
    }
  }
}



cunstom_config(simu_time, duration) {
  int i;

  //--config introduced Global simulation time for each node
  if (simu_time>0)
    SIMU_TIME=simu_time;

  //--config introduced duartion for each node
  if (duration>0)
    for (i=0; i<NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
      g_otg->duration[i] = duration;
    }
}



int main (int argc, char **argv) {
  int i,j, tx;
  char *protocol=NULL;
  char *ip_version=NULL;
  char *traffic=NULL;
  init_all_otg();
  otg_info->ctime=0;
  LOG_I(OTG,"Emulation time %d \n ", otg_info->ctime);
  g_otg->num_nodes=NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX;
  LOG_I(OTG,"OTG emulation number of nodes= %d \n", g_otg->num_nodes);

  for (i = 1; i <argc ; i ++) {
    if ('-' == argv[i][0]) {
      if(('h' == argv[i][1]) || ('H' == argv[i][1])) {
        printf("Help OTG: \n./otg [-M [s (socket mode)] [b (Below IP)]] [-T (Simu Time)] [-d (duration per node)] [-s (seed)] [-P (protocol: TCP or UDP)] [-I (ip version: IP4 or IP6)] -A [(application: CBR, AUTO_PILOT, BICYCLE_RACE, OPENARENA, TEAM_FORTRESS)]\n");
        return(0);
      } else if ('M' == argv[i][1]) {
        if (strcmp("s",argv[i+1])==0) {
          printf("Above IP: SOCKET MODE \n");
          simu_mode=1;
        } else if (strcmp("b",argv[i+1])==0) {
          printf("Below IP: IPC/RPC MODE\n");
          simu_mode=0;
        }
      } else if ('T' == argv[i][1]) {
        simu_time=atoi(argv[i+1]);
        printf("simu_time=%d\n", simu_time);
      } else if ('d' == argv[i][1]) {
        duration=atoi(argv[i+1]);
        printf("duration node=%d\n", duration);
      } else if ('s' == argv[i][1]) {
        seed=atoi(argv[i+1]);
        printf("seed val =%d\n", seed);
      } else if ('P' == argv[i][1]) {
        protocol=argv[i+1];

        if ((strcmp(argv[i+1],"TCP")==0) || (strcmp(argv[i+1],"UDP")==0) || (strcmp(argv[i+1],"tcp")==0) || (strcmp(argv[i+1],"udp")==0)) {
          protocol=argv[i+1];
          printf("Protocol=%s\n", protocol);
        }
      } else if ('I' == argv[i][1]) {
        if ((strcmp(argv[i+1],"IP4")==0) || (strcmp(argv[i+1],"IP6")==0) || (strcmp(argv[i+1],"ip4")==0) || (strcmp(argv[i+1],"ip6")==0)) {
          ip_version=argv[i+1];
          printf("IP version=%s\n", ip_version);
        }
      } else if ('A' == argv[i][1]) {
        if ((strcmp(argv[i+1],"CBR")==0) || (strcmp(argv[i+1],"AUTO_PILOT")==0) || (strcmp(argv[i+1],"BICYCLE_RACE")==0) || (strcmp(argv[i+1],"OPENARENA")==0) || (strcmp(argv[i+1],"TEAM_FORTRESS")==0))
          traffic=argv[i+1];

        config_traffic_type(traffic);
      }
    }
  }

  if (traffic!=NULL)
    init_predef_traffic();
  else
    init_config_otg(protocol, ip_version);

  init_seeds(g_otg->seed);
  cunstom_config(simu_time, duration);

  if (simu_mode==0)
    tx=main_below_ip();
  else if (simu_mode==1)
    tx=main_above_ip();

  // Compute KPI after the end of the simu
  kpi_gen();
  free_otg();
  return 0;
}


/*
int main(void) {

int i, min=40, max=1500, std_dev=100;
int n;

for(i=1; i<=100000; i++)
{

init_seeds(i);
n = uniform_dist(min, max);
//n = ceil(gaussian_dist((min + max)/2 , std_dev));
//n = ceil(exponential_dist(0.05));
//n =ceil(poisson_dist(4));
//n =ceil(weibull_dist(0,0));
//n =ceil(pareto_dist(16,4));
//n =ceil(gamma_dist(0.5,25));
//n =ceil(cauchy_dist(1,10));

if ((n<min) || (n>max))
 printf("number=%d out of range \n", n);
 else
  {printf("gen_nb=%d \n", n);
// printf("ceil gen_nb=%lf \n", ceil(n));
   }
}

printf("%d\n", n);

return 0;
    }
*/
