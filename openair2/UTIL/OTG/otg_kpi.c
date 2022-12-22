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

/*! \file otg_kpi.c main used funstions to compute KPIs
* \brief define KPIs to compute for performance analysis
* \author Navid Nikaein and A. Hafsaoui
* \date 2012
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning

*/

unsigned int start_log_latency=0;
unsigned int start_log_latency_bg=0;
unsigned int start_log_GP=0;
unsigned int start_log_GP_bg=0;
unsigned int start_log_jitter=0;

#include "otg_kpi.h"
#include "otg_externs.h"

extern uint16_t NB_UE_INST;

void tx_throughput(int src, int dst, int application) {
  if (otg_info->tx_num_bytes[src][dst][application]>0) {
    //  otg_info->tx_throughput[src][dst][application]=((double)otg_info->tx_num_bytes[src][dst][application] *1000*8)/ (get_ctime()*1024); // unit Kbit/sec, if ctime in ms
    if ((g_otg->flow_start[src][dst][application]+g_otg->flow_duration[src][dst][application]) < get_ctime() )
      otg_info->tx_throughput[src][dst][application]=((double)otg_info->tx_num_bytes[src][dst][application] *1000*8)/ ((g_otg->flow_start[src][dst][application]+g_otg->flow_duration[src][dst][application])
          *1024); // unit Kbit/sec, if ctime in ms
    else if (g_otg->flow_start[src][dst][application] < get_ctime() )
      otg_info->tx_throughput[src][dst][application]=((double)otg_info->tx_num_bytes[src][dst][application] *1000*8)/ ((get_ctime() - g_otg->flow_start[src][dst][application])*1024);
    else
      LOG_W(OTG, "[src %d][dst %d][app %d] flow start time less than the simu time (start %d, duration %d, ctime %d)\n",
            src, dst, application,
            g_otg->flow_start[src][dst][application],
            g_otg->flow_duration[src][dst][application],
            get_ctime());
  }

  if (otg_info->tx_num_bytes_background[src][dst]>0)
    otg_info->tx_throughput_background[src][dst]=((double)otg_info->tx_num_bytes_background[src][dst]*1000*8)/ (get_ctime()*1024); // unit Kbit/sec, if ctime in ms

  if (otg_multicast_info->tx_num_bytes[src][dst][application]>0) {
    if (application < 2)
      otg_multicast_info->tx_throughput[src][dst]=((double)otg_multicast_info->tx_num_bytes[src][dst][application]*1000*8)/ (get_ctime()*1024); // unit Kbit/sec, if ctime in ms

    //otg_multicast_info->tx_num_bytes[src][dst][app] is 3 dimension param in otg_tx.c, only use [app]=0
    //    LOG_I(OTG,"DUY, get_ctime() [i=%d,j=%d,k=%d] = %.d\n", src,dst,application,get_ctime());
    LOG_I(OTG,"otg_multicast_info->tx_num_bytes[i=%d,j=%d,k=%d] = %.d\n", src,dst,application,otg_multicast_info->tx_num_bytes[src][dst][application]);
  }
}



void rx_goodput(int src, int dst, int application) {
  if (otg_info->rx_num_bytes[src][dst][application]>0) {
    // otg_info->rx_goodput[src][dst][application]=((double)otg_info->rx_num_bytes[src][dst][application]*1000*8)/(get_ctime()*1024); // unit kB/sec, if ctime in ms
    if ((g_otg->flow_start[src][dst][application]+g_otg->flow_duration[src][dst][application]) < get_ctime() )
      otg_info->rx_goodput[src][dst][application]=((double)otg_info->rx_num_bytes[src][dst][application] *1000*8)/ ((g_otg->flow_start[src][dst][application]+g_otg->flow_duration[src][dst][application])
          *1024); // unit Kbit/sec, if ctime in ms
    else if (g_otg->flow_start[src][dst][application] < get_ctime() )
      otg_info->rx_goodput[src][dst][application]=((double)otg_info->rx_num_bytes[src][dst][application] *1000*8)/ ((get_ctime() - g_otg->flow_start[src][dst][application])*1024);
    else
      LOG_W(OTG, "[src %d][dst %d][app %d] flow start time less than the simu time (start %d, duration %d, ctime %d)\n",
            src, dst, application,
            g_otg->flow_start[src][dst][application],
            g_otg->flow_duration[src][dst][application],
            get_ctime());
  }

  if (otg_info->rx_num_bytes_background[src][dst]>0)
    otg_info->rx_goodput_background[src][dst]=((double)otg_info->rx_num_bytes_background[src][dst] *1000*8)/(get_ctime()*1024); // unit kB/sec, if ctime in ms

  if (otg_multicast_info->rx_num_bytes[src][dst][application]>0) {
    otg_multicast_info->rx_goodput[src][dst]=((double)otg_multicast_info->rx_num_bytes[src][dst][application] *1000*8)/(get_ctime()*1024); // unit kB/sec, if ctime in ms
    LOG_I(OTG,"otg_multicast_info->rx_num_bytes[i=%d,j=%d,k=%d] = %.d\n", src,dst,application,otg_multicast_info->rx_num_bytes[src][dst][application]);
  }
}




void rx_loss_rate_pkts(int src, int dst, int application) {
  if (otg_info->rx_num_pkt[src][dst][application]<otg_info->tx_num_pkt[src][dst][application])
    otg_info->rx_loss_rate[src][dst][application]= 1 - ((double)otg_info->rx_num_pkt[src][dst][application]/otg_info->tx_num_pkt[src][dst][application]);
  else
    otg_info->rx_loss_rate[src][dst][application]=0;

  if (otg_info->rx_num_pkt_background[src][dst]<otg_info->tx_num_pkt_background[src][dst])
    otg_info->rx_loss_rate_background[src][dst]= 1 - ((double)otg_info->rx_num_pkt_background[src][dst]/otg_info->tx_num_pkt_background[src][dst]);
  else
    otg_info->rx_loss_rate_background[src][dst]=0;

  /*
  if (otg_info->rx_loss_rate[src][dst][application]>0)
  LOG_I(OTG, "LOSS_RATE (src=%d, dst=%d, appli %d ):: = %lf(pkts) [TX %d] [RX %d], [NB UL %d] [NB DL %d]\n",src, dst, application, otg_info->rx_loss_rate[src][dst][application], otg_info->tx_num_pkt[src][dst][application],otg_info->rx_num_pkt[src][dst][application],otg_info->nb_loss_pkts_ul[src][dst][application], otg_info->nb_loss_pkts_dl[src][dst][application]);
  */

  if (otg_multicast_info->rx_num_pkt[src][dst] < otg_multicast_info->tx_num_pkt[src][dst])
    otg_multicast_info->rx_loss_rate[src][dst]= 1 - ((double)otg_multicast_info->rx_num_pkt[src][dst][application]/otg_multicast_info->tx_num_pkt[src][dst][0]);
  else
    otg_multicast_info->rx_loss_rate[src][dst]=0;
}


void rx_loss_rate_bytes(int src, int dst, int application) {
  if (otg_info->rx_num_pkt[src][dst][application]<otg_info->tx_num_pkt[src][dst][application])
    otg_info->rx_loss_rate[src][dst][application]= 1 - ((double)otg_info->rx_num_bytes[src][dst][application]/otg_info->tx_num_bytes[src][dst][application]);
  else
    otg_info->rx_loss_rate[src][dst][application]=0;

  LOG_I(OTG, "loss rate (src=%d, dst=%d, appli %d ):: = %lf(bytes) \n",src, dst, application, otg_info->rx_loss_rate[src][dst][application]);
}

void otg_kpi_nb_loss_pkts(void) {
  unsigned int i,j,k;
  otg_info->total_loss_dl=0;
  otg_info->total_loss_ul=0;

  for (i=0; i<(NB_eNB_INST + NB_UE_INST); i++) {
    for (j=0; j<(NB_eNB_INST + NB_UE_INST); j++) {
      for (k=0; k<MAX_EMU_TRAFFIC; k++) {
        if (i<NB_eNB_INST)
          otg_info->total_loss_dl+=(otg_info->nb_loss_pkts_dl[i][j][k] + otg_info->nb_loss_pkts_background_dl[i][j]);
        else
          otg_info->total_loss_ul+=(otg_info->nb_loss_pkts_ul[i][j][k] + otg_info->nb_loss_pkts_background_ul[i][j]);
      }
    }
  }
}

void average_pkt_jitter(int src, int dst, int application) {
  if (otg_info->rx_jitter_sample[src][dst][application] > 0 ) {
    otg_info->rx_jitter_avg[src][dst][application]/= otg_info->rx_jitter_sample[src][dst][application];
    otg_info->rx_jitter_avg_e2e[src][dst][application]/= otg_info->rx_jitter_sample[src][dst][application];
  } else {
    LOG_T(OTG,"[src %d][dst %d][app %d]number of samples for jitter calculation is %d\n",src, dst, application, otg_info->rx_jitter_sample[src][dst][application]);
  }

  if (otg_info->rx_jitter_avg[src][dst][application] > 0) {
    if (src<NB_eNB_INST)
      otg_info->average_jitter_dl+=otg_info->rx_jitter_avg[src][dst][application];
    else
      otg_info->average_jitter_ul+=otg_info->rx_jitter_avg[src][dst][application];

    LOG_T(OTG,"average_jitter_dl %lf average_jitter_ul %lf \n",otg_info->average_jitter_dl,otg_info->average_jitter_ul  );
  }

  if (otg_info->rx_jitter_avg_e2e[src][dst][application] > 0) {
    if (src<NB_eNB_INST)
      otg_info->average_jitter_dl_e2e+=otg_info->rx_jitter_avg_e2e[src][dst][application];
    else
      otg_info->average_jitter_ul_e2e+=otg_info->rx_jitter_avg_e2e[src][dst][application];

    LOG_T(OTG,"average_jitter_dl %lf average_jitter_ul %lf \n",otg_info->average_jitter_dl_e2e,otg_info->average_jitter_ul_e2e);
  }
}

void average_total_jitter(void) {
  unsigned int i,j,k;
  otg_info->average_jitter_dl=0;
  otg_info->average_jitter_ul=0;
  otg_info->average_jitter_dl_e2e=0;
  otg_info->average_jitter_ul_e2e=0;

  for (i=0; i<(NB_eNB_INST + NB_UE_INST); i++) {
    for (j=0; j<(NB_eNB_INST + NB_UE_INST); j++) {
      for (k=0; k<MAX_EMU_TRAFFIC; k++) {
        if (i<NB_eNB_INST) {
          otg_info->average_jitter_dl+=otg_info->rx_jitter_avg[i][j][k];
          otg_info->average_jitter_dl_e2e+=otg_info->rx_jitter_avg_e2e[i][j][k];
        } else {
          otg_info->average_jitter_ul+=otg_info->rx_jitter_avg[i][j][k];
          otg_info->average_jitter_ul_e2e+=otg_info->rx_jitter_avg_e2e[i][j][k];
        }
      }
    }
  }

  LOG_I(OTG,"average_jitter_dl %lf average_jitter_ul %lf \n",otg_info->average_jitter_dl,otg_info->average_jitter_ul  );
  //  otg_info->average_jitter_dl/= (float)NB_UE_INST;
  // otg_info->average_jitter_ul/= (float)NB_UE_INST;
}

void kpi_gen() {
  int i, j,k;
  int tx_total_bytes_dl=0;
  int tx_total_pkts_dl=0;
  int rx_total_bytes_dl=0;
  int rx_total_pkts_dl=0;
  int tx_total_bytes_ul=0;
  int tx_total_pkts_ul=0;
  int rx_total_bytes_ul=0;
  int rx_total_pkts_ul=0;
  float min_owd_dl=0;
  float max_owd_dl=0;
  float min_owd_dl_e2e=0;
  float max_owd_dl_e2e=0;
  int tx_total_bytes_dl_background=0;
  int tx_total_pkts_dl_background=0;
  int rx_total_bytes_dl_background=0;
  int rx_total_pkts_dl_background=0;
  int tx_total_bytes_ul_background=0;
  int tx_total_pkts_ul_background=0;
  int rx_total_bytes_ul_background=0;
  int rx_total_pkts_ul_background=0;
  float min_owd_ul=0;
  float max_owd_ul=0;
  float min_owd_ul_e2e=0;
  float max_owd_ul_e2e=0;
  int tx_total_bytes_dl_multicast=0;
  int tx_total_pkts_dl_multicast=0;
  int rx_total_bytes_dl_multicast=0;
  int rx_total_pkts_dl_multicast=0;
  int num_active_source=0;
  int dl_ok=1,ul_ok=1;
  char traffic_type[12];
  char traffic[30];
#ifdef STANDALONE
  FILE *file;
  file = fopen("log_OTG.txt", "w");
#else   // Maybe to do modifo log function in order to clear file before a new write !!!! 
  FILE *fc;
  fc=fopen("/tmp/otg.log","w");;

  if(fc!=0)
    fclose(fc);

#endif

  for (i=0; i<(NB_eNB_INST + NB_UE_INST); i++) {
    for (j=0; j<(NB_eNB_INST + NB_UE_INST); j++) {
      /*background stats*/
      if (i<NB_eNB_INST) {
        tx_total_bytes_dl_background+=otg_info->tx_num_bytes_background[i][j];
        tx_total_pkts_dl_background+=otg_info->tx_num_pkt_background[i][j];
        rx_total_bytes_dl_background+=otg_info->rx_num_bytes_background[i][j];
        rx_total_pkts_dl_background+=otg_info->rx_num_pkt_background[i][j];
      } else {
        tx_total_bytes_ul_background+=otg_info->tx_num_bytes_background[i][j];
        tx_total_pkts_ul_background+=otg_info->tx_num_pkt_background[i][j];
        rx_total_bytes_ul_background+=otg_info->rx_num_bytes_background[i][j];
        rx_total_pkts_ul_background+=otg_info->rx_num_pkt_background[i][j];
      }

      for (k=0; k<MAX_NUM_APPLICATION; k++) {
        tx_throughput(i,j,k);
        rx_goodput(i,j,k);
        rx_loss_rate_pkts(i,j,k);
        average_pkt_jitter(i,j,k);

        /*
              LOG_I(OTG,"KPI: (src=%d, dst=%d, traffic=%d) NB TX Bytes= %d, NB packet TX= %d,  NB packet RX= %d\n ",
              i, j,  k,
              otg_info->tx_num_bytes[i][j][k],
              otg_info->tx_num_pkt[i][j][k],
              otg_info->rx_num_pkt[i][j][k]);
        */
        if (otg_multicast_info->tx_throughput[i][j]>0)  {
          //multicast
          tx_total_bytes_dl_multicast+=otg_multicast_info->tx_num_bytes[i][j][k];
          tx_total_pkts_dl_multicast+=otg_multicast_info->tx_num_pkt[i][j][k];
          rx_total_bytes_dl_multicast+=otg_multicast_info->rx_num_bytes[i][j][k];
          rx_total_pkts_dl_multicast+=otg_multicast_info->rx_num_pkt[i][j][k];
#ifdef STANDALONE
          LOG_I(OTG,"No stats for multicast ");       // do nothing
#else

          // Multicast
          //no multicast_status
          if (otg_multicast_info->tx_num_bytes[i][j][k]>0) {
            LOG_I(OTG,"----------------------------------------------------------\n");
            LOG_I(OTG,"Total Time (multicast) (ms)= %d \n", otg_info->ctime+1);
            //   LOG_I(OTG,"[%s] Multicast [eNB:%d -> UE:%d] \n",traffic_type, i, j);
            LOG_I(OTG,"[MULTICAST] Total packets(TX)= %d \n", otg_multicast_info->tx_num_pkt[i][j][k]);
            LOG_I(OTG,"[MULTICAST] Total bytes(TX)= %d \n", otg_multicast_info->tx_num_bytes[i][j][k]);
            LOG_I(OTG,"[MULTICAST] Total packets(RX)= %d \n", otg_multicast_info->rx_num_pkt[i][j][k]);
            LOG_I(OTG,"[MULTICAST] Total bytes(RX)= %d \n", otg_multicast_info->rx_num_bytes[i][j][k]);
            LOG_I(OTG,"[MULTICAST] TX throughput = %.7f (Kbit/s) \n", otg_multicast_info->tx_throughput[i][j]);
            LOG_I(OTG,"[MULTICAST] RX goodput = %.7f (Kbit/s) \n", otg_multicast_info->rx_goodput[i][j]);
            //   if (otg_multicast_info->rx_loss_rate[i][j]>0){
            //  LOG_I(OTG,"[MULTICAST] Loss rate = %lf \n", (otg_multicast_info->rx_loss_rate[i][j]));
            //  LOG_I(OTG,"[MULTICAST] NB Lost  packets=%d \n", (otg_multicast_info->tx_num_pkt[i][j][k]-otg_multicast_info->rx_num_pkt[i][j][k]));
            //}
            // if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)){
            LOG_F(OTG,"----------------------------------------------------------\n");
            LOG_F(OTG,"Total Time (multicast) (ms)= %d \n", otg_info->ctime+1);
            //  if (i<NB_eNB_INST){
            LOG_F(OTG,"[%s] DL [eNB:%d -> UE:%d] \n",traffic_type ,i, j);
            //else
            //    LOG_F(OTG,"[%s] UL [Ue:%d -> eNB:%d] \n",traffic_type, i, j);
            LOG_F(OTG,"[MULTICAST] Total packets(TX)= %d \n", otg_multicast_info->tx_num_pkt[i][j][k]);
            LOG_F(OTG,"[MULTICAST] Total bytes(TX)= %d \n", otg_multicast_info->tx_num_bytes[i][j][k]);
            LOG_F(OTG,"[MULTICAST] TX throughput = %.7f(Kbit/s) \n", otg_multicast_info->tx_throughput[i][j]);
            LOG_F(OTG,"[MULTICAST] RX goodput= %.7f (Kbit/s) \n", otg_multicast_info->rx_goodput[i][j]);

            if (otg_multicast_info->rx_loss_rate[i][j]>0) {
              LOG_F(OTG,"[MULTICAST] Loss rate = %lf \n", (otg_multicast_info->rx_loss_rate[i][j]));
              LOG_F(OTG,"[MULTICAST] NB Lost  packets=%d \n", (otg_multicast_info->tx_num_pkt[i][j][k]-otg_multicast_info->rx_num_pkt[i][j][k]));
            }
          }

#endif
        }// end for multicast

        if ((otg_info->tx_throughput[i][j][k]>0)||((otg_info->tx_throughput_background[i][j]>0) && (otg_info->tx_num_bytes[i][j][k]>0)))  {
          num_active_source+=1;

          if (i<NB_eNB_INST) { // DL
            tx_total_bytes_dl+=otg_info->tx_num_bytes[i][j][k];
            tx_total_pkts_dl+=otg_info->tx_num_pkt[i][j][k];
            rx_total_bytes_dl+=otg_info->rx_num_bytes[i][j][k];
            rx_total_pkts_dl+=otg_info->rx_num_pkt[i][j][k];
            /*tx_total_bytes_dl_background+=otg_info->tx_num_bytes_background[i][j];
              tx_total_pkts_dl_background+=otg_info->tx_num_pkt_background[i][j];
              rx_total_bytes_dl_background+=otg_info->rx_num_bytes_background[i][j];
              rx_total_pkts_dl_background+=otg_info->rx_num_pkt_background[i][j]; */

            if ((min_owd_dl>otg_info->rx_owd_min[i][j][k]) || (min_owd_dl==0))
              min_owd_dl=otg_info->rx_owd_min[i][j][k];

            if ((max_owd_dl<otg_info->rx_owd_max[i][j][k]) || (max_owd_dl==0))
              max_owd_dl=otg_info->rx_owd_max[i][j][k];

            if ((min_owd_dl_e2e>otg_info->rx_owd_min_e2e[i][j][k]) || (min_owd_dl_e2e==0))
              min_owd_dl_e2e=otg_info->rx_owd_min_e2e[i][j][k];

            if ((max_owd_dl_e2e<otg_info->rx_owd_max_e2e[i][j][k]) || (max_owd_dl_e2e==0))
              max_owd_dl_e2e=otg_info->rx_owd_max_e2e[i][j][k];
          } else { // UL
            tx_total_bytes_ul+=otg_info->tx_num_bytes[i][j][k];
            tx_total_pkts_ul+=otg_info->tx_num_pkt[i][j][k];
            rx_total_bytes_ul+=otg_info->rx_num_bytes[i][j][k];
            rx_total_pkts_ul+=otg_info->rx_num_pkt[i][j][k];

            /*tx_total_bytes_ul_background+=otg_info->tx_num_bytes_background[i][j];
              tx_total_pkts_ul_background+=otg_info->tx_num_pkt_background[i][j];
              rx_total_bytes_ul_background+=otg_info->rx_num_bytes_background[i][j];
              rx_total_pkts_ul_background+=otg_info->rx_num_pkt_background[i][j];*/
            if ((min_owd_ul>otg_info->rx_owd_min[i][j][k]) || (min_owd_ul==0))
              min_owd_ul=otg_info->rx_owd_min[i][j][k];

            if ((max_owd_ul<otg_info->rx_owd_max[i][j][k]) || (max_owd_ul==0))
              max_owd_ul=otg_info->rx_owd_max[i][j][k];

            if ((min_owd_ul_e2e>otg_info->rx_owd_min_e2e[i][j][k]) || (min_owd_ul_e2e==0))
              min_owd_ul_e2e=otg_info->rx_owd_min_e2e[i][j][k];

            if ((max_owd_ul_e2e<otg_info->rx_owd_max_e2e[i][j][k]) || (max_owd_ul_e2e==0))
              max_owd_ul_e2e=otg_info->rx_owd_max_e2e[i][j][k];
          }

          //LOG_D(OTG,"KPI: (src=%d, dst=%d, appli %d) NB packet TX= %d,  NB packet RX= %d\n ",i, j,  otg_info->tx_num_pkt[i][j][k],  otg_info->rx_num_pkt[i][j][k]);

          if (g_otg->application_idx[i][j]>1)
            strcpy(traffic_type,"GATEWAY");
          else
            strcpy(traffic_type,"APPLICATION");

          if (map_int_to_str(otg_app_type_names,g_otg->application_type[i][j][k]) == 0) {
            LOG_E(OTG,"(src=%d, dst=%d, appli %d) : Unknown traffic \n ", i, j,k);
            strcpy(traffic,"UKNOWN TRAFFIC");
          } else {
            strncpy (traffic, map_int_to_str(otg_app_type_names,g_otg->application_type[i][j][k]), sizeof(traffic));
            traffic[sizeof(traffic) - 1] = 0; // terminate string
          }

#ifdef STANDALONE
          fprintf(file,"----------------------------------------------------------\n");
          fprintf(file,"Total Time (ms)= %d \n", otg_info->ctime+1);

          if (i<NB_eNB_INST) {
            fprintf(file,"[%s] DL [eNB:%d, UE:%d][%s] \n", traffic_type,i,j,traffic);
          } else
            fprintf(file,"[%s] UL [eNB:%d, UE:%d][%s] \n",traffic_type,j,i,traffic);

          fprintf(file,"[%s] Total packets(TX)= %d \n",traffic_type, otg_info->tx_num_pkt[i][j][k]);
          fprintf(file,"[%s] Total bytes(TX)= %d \n",traffic_type, otg_info->tx_num_bytes[i][j][k]);
          fprintf(file,"[%s] OWD MIN (one way)ms= %.2f \n",traffic_type, otg_info->rx_owd_min[i][j][k]);
          fprintf(file,"[%s] OWD MAX (one way)ms= %.2f \n",traffic_type, otg_info->rx_owd_max[i][j][k]);
          fprintf(file,"[%s] TX throughput = %.7f(Kbit/s) \n",traffic_type, otg_info->tx_throughput[i][j][k]);
          fprintf(file,"[%s] RX goodput= %.7f (K/s) \n",traffic_type, otg_info->rx_goodput[i][j][k]);

          if (otg_info->rx_loss_rate[i][j[k]]>0) {
            fprintf(file,"[%s] Loss rate = %.2f \n",traffic_type, (otg_info->rx_loss_rate[i][j][k]));
            fprintf(file,"[%s] NB Lost  packets=%d \n",traffic_type, (otg_info->tx_num_pkt[i][j][k]-otg_info->rx_num_pkt[i][j][k]));
            //fprintf(file,"[%s] NB Lost [OTG] packets=%d \n",traffic_type, otg_info->nb_loss_pkts_otg[i][j]);
          }

          /* if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)){
             fprintf(file,"[BACKGROUND] Total packets(TX)= %d \n", otg_info->tx_num_pkt_background[i][j]);
            fprintf(file,"[BACKGROUND] Total bytes(TX)= %d \n", otg_info->tx_num_bytes_background[i][j]);
            fprintf(file,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", otg_info->tx_throughput_background[i][j]);
            fprintf(file,"[BACKGROUND] RX goodput= %.7f (Kbit/s) \n", otg_info->rx_goodput_background[i][j]);
            if (otg_info->rx_loss_rate_background[i][j]>0){
              fprintf(file,"[BACKGROUND] Loss rate(percentage)= %lf \n", (otg_info->rx_loss_rate_background[i][j]*100));
              fprintf(file,"[BACKGROUND] NB Lost  packets=%d \n", (otg_info->tx_num_pkt_background[i][j]-otg_info->rx_num_pkt_background[i][j]));
            }
          }
          */
#else
          LOG_I(OTG,"----------------------------------------------------------\n");
          LOG_I(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);

          if (i<NB_eNB_INST)
            LOG_I(OTG,"[%s] DL [eNB:%d -> UE:%d][%s] \n",traffic_type, i, j, traffic);
          else
            LOG_I(OTG,"[%s] UL [UE:%d -> eNB:%d][%s] \n",traffic_type, i, j, traffic);

          LOG_I(OTG,"[%s] Total packets(TX)           = %d \n",traffic_type, otg_info->tx_num_pkt[i][j][k]);
          LOG_I(OTG,"[%s] Total packets(RX)           = %d \n",traffic_type, otg_info->rx_num_pkt[i][j][k]);
          LOG_I(OTG,"[%s] Total bytes(TX)             = %d \n",traffic_type, otg_info->tx_num_bytes[i][j][k]);
          LOG_I(OTG,"[%s] Total bytes(RX)             = %d \n",traffic_type, otg_info->rx_num_bytes[i][j][k]);
          LOG_I(OTG,"[%s] OWD MIN (one way)ms         = %.2f \n",traffic_type, otg_info->rx_owd_min[i][j][k]);
          LOG_I(OTG,"[%s] OWD MAX (one way)ms         = %.2f \n",traffic_type, otg_info->rx_owd_max[i][j][k]);
          LOG_I(OTG,"[%s] Estimated E2E OWD MIN ms    = %.2f \n",traffic_type, otg_info->rx_owd_max_e2e[i][j][k]);
          LOG_I(OTG,"[%s] Estimated E2E OWD MAX ms    = %.2f \n",traffic_type, otg_info->rx_owd_max_e2e[i][j][k]);
          LOG_I(OTG,"[%s] JITTER AVG ms               = %.2f \n",traffic_type, otg_info->rx_jitter_avg[i][j][k]);
          LOG_I(OTG,"[%s] JITTER MIN ms               = %.2f \n",traffic_type, otg_info->rx_jitter_min[i][j][k]);
          LOG_I(OTG,"[%s] JITTER MAX ms               = %.2f \n",traffic_type, otg_info->rx_jitter_max[i][j][k]);
          LOG_I(OTG,"[%s] Estimated E2E JITTER AVG ms = %.2f \n",traffic_type, otg_info->rx_jitter_avg_e2e[i][j][k]);
          LOG_I(OTG,"[%s] Estimated E2E JITTER MIN ms = %.2f \n",traffic_type, otg_info->rx_jitter_min_e2e[i][j][k]);
          LOG_I(OTG,"[%s] Estimated E2E JITTER MAX ms = %.2f \n",traffic_type, otg_info->rx_jitter_max_e2e[i][j][k]);
          LOG_I(OTG,"[%s] TX throughput               = %.7f(Kbit/s) \n",traffic_type, otg_info->tx_throughput[i][j][k]);
          LOG_I(OTG,"[%s] RX goodput                  = %.7f (Kbit/s) \n",traffic_type, otg_info->rx_goodput[i][j][k]);

          if (otg_info->rx_loss_rate[i][j][k]>0) {
            LOG_I(OTG,"[%s] Loss rate               = %lf \n",traffic_type, (otg_info->rx_loss_rate[i][j][k]));
            LOG_I(OTG,"[%s] NB Lost packets         = %d \n",traffic_type, (otg_info->tx_num_pkt[i][j][k]-otg_info->rx_num_pkt[i][j][k]));
            //LOG_D(OTG,"[%s] NB Lost [OTG] packets=%d \n",traffic_type, otg_info->nb_loss_pkts_otg[i][j]);
          }

          /*  if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)){
              LOG_I(OTG,"[BACKGROUND] Total packets(TX)= %d \n", otg_info->tx_num_pkt_background[i][j]);
              LOG_I(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", otg_info->tx_num_bytes_background[i][j]);
              LOG_I(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", otg_info->tx_throughput_background[i][j]);
              LOG_I(OTG,"[BACKGROUND] RX goodput= %.7f (Kbit/s) \n", otg_info->rx_goodput_background[i][j]);
              if (otg_info->rx_loss_rate_background[i][j]>0){
                LOG_I(OTG,"[BACKGROUND] Loss rate(percentage)= %.2f \n", (otg_info->rx_loss_rate_background[i][j]*100));
                LOG_I(OTG,"[BACKGROUND] NB Lost  packets=%d \n", (otg_info->tx_num_pkt_background[i][j]-otg_info->rx_num_pkt_background[i][j]));
              }
            }
          */
          LOG_F(OTG,"----------------------------------------------------------\n");
          LOG_F(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);

          if (i<NB_eNB_INST) {
            LOG_F(OTG,"[%s] DL [eNB:%d -> UE:%d][%s] \n",traffic_type ,i, j, traffic);
          } else
            LOG_F(OTG,"[%s] UL [UE:%d -> eNB:%d][%s] \n",traffic_type, i, j,traffic);

          LOG_F(OTG,"[%s] Total packets(TX)= %d \n",traffic_type, otg_info->tx_num_pkt[i][j][k]);
          LOG_F(OTG,"[%s] Total packets(RX)= %d \n",traffic_type, otg_info->rx_num_pkt[i][j][k]);
          LOG_F(OTG,"[%s] Total bytes(TX)= %d \n",traffic_type, otg_info->tx_num_bytes[i][j][k]);
          LOG_F(OTG,"[%s] Total bytes(RX)= %d \n",traffic_type, otg_info->rx_num_bytes[i][j][k]);
          LOG_F(OTG,"[%s] OWD MIN (one way)ms= %.2f \n",traffic_type, otg_info->rx_owd_min[i][j][k]);
          LOG_F(OTG,"[%s] OWD MAX (one way)ms= %.2f \n",traffic_type, otg_info->rx_owd_max[i][j][k]);
          LOG_F(OTG,"[%s] Estimated E2E OWD MIN ms = %.2f \n",traffic_type, otg_info->rx_owd_max_e2e[i][j][k]);
          LOG_F(OTG,"[%s] Estimated E2E OWD MAX ms = %.2f \n",traffic_type, otg_info->rx_owd_max_e2e[i][j][k]);
          LOG_F(OTG,"[%s] JITTER AVG ms= %.2f \n",traffic_type, otg_info->rx_jitter_avg[i][j][k]);
          LOG_F(OTG,"[%s] JITTER MIN ms= %.2f \n",traffic_type, otg_info->rx_jitter_min[i][j][k]);
          LOG_F(OTG,"[%s] JITTER MAX ms= %.2f \n",traffic_type, otg_info->rx_jitter_max[i][j][k]);
          LOG_F(OTG,"[%s] Estimated E2E JITTER AVG ms = %.2f \n",traffic_type, otg_info->rx_jitter_avg_e2e[i][j][k]);
          LOG_F(OTG,"[%s] Estimated E2E JITTER MIN ms = %.2f \n",traffic_type, otg_info->rx_jitter_min_e2e[i][j][k]);
          LOG_F(OTG,"[%s] Estimated E2E JITTER MAX ms = %.2f \n",traffic_type, otg_info->rx_jitter_max_e2e[i][j][k]);
          LOG_F(OTG,"[%s] TX throughput = %.7f(Kbit/s) \n",traffic_type, otg_info->tx_throughput[i][j][k]);
          LOG_F(OTG,"[%s] RX goodput= %.7f (Kbit/s) \n",traffic_type, otg_info->rx_goodput[i][j][k]);

          if (otg_info->rx_loss_rate[i][j][k]>0) {
            LOG_F(OTG,"[%s] Loss rate = %lf \n",traffic_type, (otg_info->rx_loss_rate[i][j][k]));
            LOG_F(OTG,"[%s] NB Lost  packets= %d \n",traffic_type,(otg_info->tx_num_pkt[i][j][k]-otg_info->rx_num_pkt[i][j][k]));
            //LOG_D(OTG,"[%s] NB Lost [OTG] packets=%d \n",traffic_type, otg_info->nb_loss_pkts_otg[i][j]);
          }

          /*  if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)){
              LOG_F(OTG,"[BACKGROUND] Total packets(TX)= %d \n", otg_info->tx_num_pkt_background[i][j]);
              LOG_F(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", otg_info->tx_num_bytes_background[i][j]);
              LOG_F(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", otg_info->tx_throughput_background[i][j]);
              LOG_F(OTG,"[BACKGROUND] RX goodput= %.7f (Kbit/s) \n", otg_info->rx_goodput_background[i][j]);
              if (otg_info->rx_loss_rate_background[i][j]>0){
                LOG_F(OTG,"[BACKGROUND] Loss rate(percentage)= %.2f \n", (otg_info->rx_loss_rate_background[i][j]*100));
                LOG_F(OTG,"[BACKGROUND] NB Lost  packets=%d \n", (otg_info->tx_num_pkt_background[i][j]-otg_info->rx_num_pkt_background[i][j]));
              }
            }
          */
#endif
        }

        //     if ((otg_multicast_info->tx_throughput[i][j]>0) && (otg_info->tx_num_bytes[i][j][k]>0))  {
      }// end loop of k

#ifdef STANDALONE

      if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)) {
        fprintf(file,"----------------------------------------------------------\n");
        fprintf(file,"Total Time (ms)= %d \n", otg_info->ctime+1);

        if (i<NB_eNB_INST) {
          fprintf(file,"[%s] DL [eNB:%d, UE:%d] \n", traffic_type,i,j);
        } else
          fprintf(file,"[%s] UL [eNB:%d, UE:%d] \n",traffic_type,j,i);

        fprintf(file,"[BACKGROUND] Total packets(TX)= %d \n", otg_info->tx_num_pkt_background[i][j]);
        fprintf(file,"[BACKGROUND] Total bytes(TX)= %d \n", otg_info->tx_num_bytes_background[i][j]);
        fprintf(file,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", otg_info->tx_throughput_background[i][j]);
        fprintf(file,"[BACKGROUND] RX goodput= %.7f (Kbit/s) \n", otg_info->rx_goodput_background[i][j]);

        if (otg_info->rx_loss_rate_background[i][j]>0) {
          fprintf(file,"[BACKGROUND] Loss rate = %lf \n", (otg_info->rx_loss_rate_background[i][j]));
          fprintf(file,"[BACKGROUND] NB Lost  packets=%d \n", (otg_info->tx_num_pkt_background[i][j]-otg_info->rx_num_pkt_background[i][j]));
        }
      }

#else

      if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)) {
        LOG_I(OTG,"----------------------------------------------------------\n");
        LOG_I(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);

        if (i<NB_eNB_INST) {
          LOG_I(OTG,"[%s] DL [eNB:%d -> UE:%d] \n",traffic_type, i, j);
        } else
          LOG_I(OTG,"[%s] UL [UE:%d -> eNB:%d] \n",traffic_type, i, j);

        LOG_I(OTG,"[BACKGROUND] Total packets(TX)= %d \n", otg_info->tx_num_pkt_background[i][j]);
        LOG_I(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", otg_info->tx_num_bytes_background[i][j]);
        LOG_I(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", otg_info->tx_throughput_background[i][j]);
        LOG_I(OTG,"[BACKGROUND] RX goodput= %.7f (Kbit/s) \n", otg_info->rx_goodput_background[i][j]);

        if (otg_info->rx_loss_rate_background[i][j]>0) {
          LOG_I(OTG,"[BACKGROUND] Loss rate = %lf \n", (otg_info->rx_loss_rate_background[i][j]));
          LOG_I(OTG,"[BACKGROUND] NB Lost  packets=%d \n", (otg_info->tx_num_pkt_background[i][j]-otg_info->rx_num_pkt_background[i][j]));
        }
      }

      if ((g_otg->background_stats==1)&&(otg_info->tx_num_bytes_background[i][j]>0)) {
        LOG_F(OTG,"----------------------------------------------------------\n");
        LOG_F(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);

        if (i<NB_eNB_INST) {
          LOG_F(OTG,"[%s] DL [eNB:%d -> UE:%d] \n",traffic_type ,i, j);
        } else
          LOG_F(OTG,"[%s] UL [UE:%d -> eNB:%d] \n",traffic_type, i, j);

        LOG_F(OTG,"[BACKGROUND] Total packets(TX)= %d \n", otg_info->tx_num_pkt_background[i][j]);
        LOG_F(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", otg_info->tx_num_bytes_background[i][j]);
        LOG_F(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", otg_info->tx_throughput_background[i][j]);
        LOG_F(OTG,"[BACKGROUND] RX goodput= %.7f (Kbit/s) \n", otg_info->rx_goodput_background[i][j]);

        if (otg_info->rx_loss_rate_background[i][j]>0) {
          LOG_F(OTG,"[BACKGROUND] Loss rate = %lf \n", (otg_info->rx_loss_rate_background[i][j]));
          LOG_F(OTG,"[BACKGROUND] NB Lost  packets=%d \n", (otg_info->tx_num_pkt_background[i][j]-otg_info->rx_num_pkt_background[i][j]));
        }
      }

#endif
    }// end loop of j
  }// end loop of i

  //  average_total_jitter();
#ifdef STANDALONE
  fprintf (file,"**************** TOTAL DL RESULTS ******************\n");
  fprintf(file,"Total Time= %d \n", otg_info->ctime+1);
  fprintf(file,"[DATA] Total packets(TX)= %d \n", tx_total_pkts_dl);
  fprintf(file,"[DATA] Total bytes(TX)= %d \n", tx_total_bytes_dl);
  fprintf(file,"[DATA] Total packets(RX)= %d \n", rx_total_pkts_dl);
  fprintf(file,"[DATA] Total bytes(RX)= %d \n", rx_total_bytes_dl);
  fprintf(file,"[DATA] OWD MIN (one way)ms= %.2f \n", min_owd_dl);
  fprintf(file,"[DATA] OWD MAX (one way)ms= %.2f \n", max_owd_dl);
  fprintf(file,"[DATA] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl*1000*8)/(otg_info->ctime*1024));
  fprintf(file,"[DATA] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl*1000*8)/(otg_info->ctime*1024));

  if ((g_otg->background_stats==1)&&(tx_total_pkts_dl_background>0)) {
    fprintf(file,"[BACKGROUND] Total packets(TX)= %d \n", tx_total_pkts_dl_background);
    fprintf(file,"[BACKGROUND] Total bytes(TX)= %d \n", tx_total_bytes_dl_background);
    fprintf(file,"[BACKGROUND] Total packets(RX)= %d \n", rx_total_pkts_dl_background);
    fprintf(file,"[BACKGROUND] Total bytes(RX)= %d \n", rx_total_bytes_dl_background);
    fprintf(file,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl_background*1000*8)/(otg_info->ctime*1024));
    fprintf(file,"[BACKGROUND] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl_background*1000*8)/(otg_info->ctime*1024));
  }

  fprintf (file,"**************** TOTAL UL RESULTS ******************\n");
  fprintf(file,"Total Time (ms)= %d \n", otg_info->ctime+1);
  fprintf(file,"[DATA] Total packets(TX)= %d \n", tx_total_pkts_ul);
  fprintf(file,"[DATA] Total bytes(TX)= %d \n", tx_total_bytes_ul);
  fprintf(file,"[DATA] Total packets(RX)= %d \n", rx_total_pkts_ul);
  fprintf(file,"[DATA] Total bytes(RX)= %d \n", rx_total_bytes_ul);
  fprintf(file,"[DATA] OWD MIN (one way)ms= %.2f \n", min_owd_ul);
  fprintf(file,"[DATA] OWD MAX (one way)ms= %.2f \n", max_owd_ul);
  fprintf(file,"[DATA] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_ul*1000*8)/(otg_info->ctime*1024));
  fprintf(file,"[DATA] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_ul*1000*8)/(otg_info->ctime*1024));

  if ((g_otg->background_stats==1)&&(tx_total_pkts_ul_background>0)) {
    fprintf(file,"[BACKGROUND] Total packets(TX)= %d \n", tx_total_pkts_ul_background);
    fprintf(file,"[BACKGROUND] Total bytes(TX)= %d \n", tx_total_bytes_ul_background);
    fprintf(file,"[BACKGROUND] Total packets(RX)= %d \n", rx_total_pkts_ul_background);
    fprintf(file,"[BACKGROUND] Total bytes(RX)= %d \n", rx_total_bytes_ul_background);
    fprintf(file,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_ul_background*1000*8)/(otg_info->ctime*1024));
    fprintf(file,"[BACKGROUND] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_ul_background*1000*8)/(otg_info->ctime*1024));
  }

  fclose(file);
#else
  LOG_I(OTG,"**************** TOTAL DL RESULTS ******************\n");
  LOG_I(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);
  LOG_I(OTG,"[DATA] Total packets(TX)= %d \n", tx_total_pkts_dl);
  LOG_I(OTG,"[DATA] Total packets(RX)= %d \n", rx_total_pkts_dl);
  LOG_I(OTG,"[DATA] Total bytes(TX)= %d \n", tx_total_bytes_dl);
  LOG_I(OTG,"[DATA] Total bytes(RX)= %d \n", rx_total_bytes_dl);
  LOG_I(OTG,"[DATA] OWD MIN (one way)ms= %.2f \n", min_owd_dl);
  LOG_I(OTG,"[DATA] OWD MAX (one way)ms= %.2f \n", max_owd_dl);
  LOG_I(OTG,"[DATA] Estimated E2E OWD MIN (one way)ms= %.2f \n", min_owd_dl_e2e);
  LOG_I(OTG,"[DATA] Estimated E2E OWD MAX (one way)ms= %.2f \n", max_owd_dl_e2e);

  if (num_active_source != 0) {
    LOG_I(OTG,"[DATA] JITTER AVG ms= %lf \n", otg_info->average_jitter_dl/(float)num_active_source );
    LOG_I(OTG,"[DATA] Estimated E2E JITTER AVG ms= %lf \n", otg_info->average_jitter_dl_e2e/(float)num_active_source );
  }

  LOG_I(OTG,"[DATA] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl*1000*8)/(otg_info->ctime*1024));
  LOG_I(OTG,"[DATA] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl*1000*8)/(otg_info->ctime*1024));
  LOG_I(OTG,"[DATA] NB lost packet = %d \n", tx_total_pkts_dl - rx_total_pkts_dl);

  if ( (tx_total_pkts_dl > 0) &&
       ((10*(tx_total_pkts_dl - rx_total_pkts_dl))  >  tx_total_pkts_dl)) // below 10% of loss
    dl_ok=0;

  if ((g_otg->background_stats==1)&&(tx_total_pkts_dl_background>0)) {
    LOG_I(OTG,"[BACKGROUND] Total packets(TX)= %d \n", tx_total_pkts_dl_background);
    LOG_I(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", tx_total_bytes_dl_background);
    LOG_I(OTG,"[BACKGROUND] Total packets(RX)= %d \n", rx_total_pkts_dl_background);
    LOG_I(OTG,"[BACKGROUND] Total bytes(RX)= %d \n", rx_total_bytes_dl_background);
    LOG_I(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl_background*1000*8)/(otg_info->ctime*1024));
    LOG_I(OTG,"[BACKGROUND] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl_background*1000*8)/(otg_info->ctime*1024));
  }

  if (tx_total_pkts_dl_multicast>0) {
    LOG_I(OTG,"[MULTICAST] Total packets(TX)= %d \n", tx_total_pkts_dl_multicast);
    LOG_I(OTG,"[MULTICAST] Total bytes(TX)= %d \n", tx_total_bytes_dl_multicast);
    LOG_I(OTG,"[MULTICAST] Total packets(RX)= %d \n", rx_total_pkts_dl_multicast);
    LOG_I(OTG,"[MULTICAST] Total bytes(RX)= %d \n", rx_total_bytes_dl_multicast);
    LOG_I(OTG,"[MULTICAST] otg_multicast_info->ctime = %d \n", otg_multicast_info->ctime);
    LOG_I(OTG,"[MULTICAST] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl_multicast*1000*8)/(otg_info->ctime*1024));
    LOG_I(OTG,"[MULTICAST] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl_multicast*1000*8)/(otg_info->ctime*1024));
  }

  LOG_F(OTG,"**************** TOTAL DL RESULTS ******************\n");
  LOG_F(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);
  LOG_F(OTG,"[DATA] Total packets(TX)= %d \n", tx_total_pkts_dl);
  LOG_F(OTG,"[DATA] Total packets(RX)= %d \n", rx_total_pkts_dl);
  LOG_F(OTG,"[DATA] Total bytes(TX)= %d \n", tx_total_bytes_dl);
  LOG_F(OTG,"[DATA] Total bytes(RX)= %d \n", rx_total_bytes_dl);
  LOG_F(OTG,"[DATA] OWD MIN (one way)ms= %.2f \n", min_owd_dl);
  LOG_F(OTG,"[DATA] OWD MAX (one way)ms= %.2f \n", max_owd_dl);
  LOG_F(OTG,"[DATA] ESTIMATED E2E OWD MIN (one way)ms= %.2f \n", min_owd_dl_e2e);
  LOG_F(OTG,"[DATA] ESTIMATED E2E OWD MAX (one way)ms= %.2f \n", max_owd_dl_e2e);

  if (num_active_source != 0) {
    LOG_F(OTG,"[DATA] JITTER AVG ms= %lf \n",  otg_info->average_jitter_dl/(float)num_active_source);
    LOG_F(OTG,"[DATA]  ESTIMATED E2E JITTER AVG ms= %lf \n",  otg_info->average_jitter_dl_e2e/(float)num_active_source);
  }

  LOG_F(OTG,"[DATA] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl*1000*8)/(otg_info->ctime*1024));
  LOG_F(OTG,"[DATA] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl*1000*8)/(otg_info->ctime*1024));
  LOG_F(OTG,"[DATA] NB lost packet = %d \n", tx_total_pkts_dl - rx_total_pkts_dl );

  if ((g_otg->background_stats==1)&&(tx_total_pkts_dl_background>0)) {
    LOG_F(OTG,"[BACKGROUND] Total packets(TX)= %d \n", tx_total_pkts_dl_background);
    LOG_F(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", tx_total_bytes_dl_background);
    LOG_F(OTG,"[BACKGROUND] Total packets(RX)= %d \n", rx_total_pkts_dl_background);
    LOG_F(OTG,"[BACKGROUND] Total bytes(RX)= %d \n", rx_total_bytes_dl_background);
    LOG_F(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl_background*1000*8)/(otg_info->ctime*1024));
    LOG_F(OTG,"[BACKGROUND] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl_background*1000*8)/(otg_info->ctime*1024));
  }

  if (tx_total_pkts_dl_multicast>0) {
    LOG_F(OTG,"[MULTICAST] Total packets(TX)= %d \n", tx_total_pkts_dl_multicast);
    LOG_F(OTG,"[MULTICAST] Total bytes(TX)= %d \n", tx_total_bytes_dl_multicast);
    LOG_F(OTG,"[MULTICAST] Total packets(RX)= %d \n", rx_total_pkts_dl_multicast);
    LOG_F(OTG,"[MULTICAST] Total bytes(RX)= %d \n", rx_total_bytes_dl_multicast);
    LOG_F(OTG,"[MULTICAST] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_dl_multicast*1000*8)/(otg_info->ctime*1024));
    LOG_F(OTG,"[MULTICAST] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_dl_multicast*1000*8)/(otg_info->ctime*1024));
  }

  LOG_I(OTG,"**************** TOTAL UL RESULTS ******************\n");
  LOG_I(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);
  LOG_I(OTG,"[DATA] Total packets(TX)= %d \n", tx_total_pkts_ul);
  LOG_I(OTG,"[DATA] Total packets(RX)= %d \n", rx_total_pkts_ul);
  LOG_I(OTG,"[DATA] Total bytes(TX)= %d \n", tx_total_bytes_ul);
  LOG_I(OTG,"[DATA] Total bytes(RX)= %d \n", rx_total_bytes_ul);
  LOG_I(OTG,"[DATA] OWD MIN (one way)ms= %.2f \n", min_owd_ul);
  LOG_I(OTG,"[DATA] OWD MAX (one way)ms= %.2f \n", max_owd_ul);
  LOG_I(OTG,"[DATA] ESTIMATED E2E OWD MIN (one way)ms= %.2f \n", min_owd_ul_e2e);
  LOG_I(OTG,"[DATA] ESTIMATED E2E OWD MAX (one way)ms= %.2f \n", max_owd_ul_e2e);

  if (num_active_source != 0) {
    LOG_I(OTG,"[DATA] JITTER AVG ms= %lf \n",  otg_info->average_jitter_ul_e2e/(float)num_active_source);
    LOG_I(OTG,"[DATA] ESTIMATED E2E JITTER AVG ms= %lf \n",  otg_info->average_jitter_ul/(float)num_active_source);
  }

  LOG_I(OTG,"[DATA] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_ul*1000*8)/(otg_info->ctime*1024));
  LOG_I(OTG,"[DATA] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_ul*1000*8)/(otg_info->ctime*1024));
  LOG_I(OTG,"[DATA] NB lost packet = %d \n", tx_total_pkts_ul - rx_total_pkts_ul );

  if ( (tx_total_pkts_ul > 0 ) &&
       ((10*(tx_total_pkts_ul - rx_total_pkts_ul)) > tx_total_pkts_ul))
    ul_ok=0;

  if ((g_otg->background_stats==1)&&(tx_total_pkts_ul_background>0)) {
    LOG_I(OTG,"[BACKGROUND] Total packets(TX)= %d \n", tx_total_pkts_ul_background);
    LOG_I(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", tx_total_bytes_ul_background);
    LOG_I(OTG,"[BACKGROUND] Total packets(RX)= %d \n", rx_total_pkts_ul_background);
    LOG_I(OTG,"[BACKGROUND] Total bytes(RX)= %d \n", rx_total_bytes_ul_background);
    LOG_I(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_ul_background*1000*8)/(otg_info->ctime*1024));
    LOG_I(OTG,"[BACKGROUND] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_ul_background*1000*8)/(otg_info->ctime*1024));
  }

  LOG_F(OTG,"**************** TOTAL UL RESULTS ******************\n");
  LOG_F(OTG,"Total Time (ms)= %d \n", otg_info->ctime+1);
  LOG_F(OTG,"[DATA] Total packets(TX)= %d \n", tx_total_pkts_ul);
  LOG_F(OTG,"[DATA] Total packets(RX)= %d \n", rx_total_pkts_ul);
  LOG_F(OTG,"[DATA] Total bytes(TX)= %d \n", tx_total_bytes_ul);
  LOG_F(OTG,"[DATA] Total bytes(RX)= %d \n", rx_total_bytes_ul);
  LOG_F(OTG,"[DATA] OWD MIN (one way)ms= %.2f \n", min_owd_ul);
  LOG_F(OTG,"[DATA] OWD MAX (one way)ms= %.2f \n", max_owd_ul);
  LOG_F(OTG,"[DATA] ESTIMATED E2E OWD MIN (one way)ms= %.2f \n", min_owd_ul_e2e);
  LOG_F(OTG,"[DATA] ESTIMATED E2E OWD MAX (one way)ms= %.2f \n", max_owd_ul_e2e);

  if (num_active_source != 0) {
    LOG_F(OTG,"[DATA] JITTER AVG ms= %lf \n",  otg_info->average_jitter_ul/(float)num_active_source);
    LOG_F(OTG,"[DATA] ESTIMATED E2E JITTER AVG ms= %lf \n",  otg_info->average_jitter_ul_e2e/(float)num_active_source);
  }

  LOG_F(OTG,"[DATA] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_ul*1000*8)/(otg_info->ctime*1024));
  LOG_F(OTG,"[DATA] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_ul*1000*8)/(otg_info->ctime*1024));
  LOG_F(OTG,"[DATA] NB lost packet = %d \n", tx_total_pkts_ul - rx_total_pkts_ul );

  if ((g_otg->background_stats==1)&&(tx_total_pkts_ul_background>0)) {
    LOG_F(OTG,"[BACKGROUND] Total packets(TX)= %d \n", tx_total_pkts_ul_background);
    LOG_F(OTG,"[BACKGROUND] Total bytes(TX)= %d \n", tx_total_bytes_ul_background);
    LOG_F(OTG,"[BACKGROUND] Total packets(RX)= %d \n", rx_total_pkts_ul_background);
    LOG_F(OTG,"[BACKGROUND] Total bytes(RX)= %d \n", rx_total_bytes_ul_background);
    LOG_F(OTG,"[BACKGROUND] TX throughput = %.7f(Kbit/s) \n", ((double)tx_total_bytes_ul_background*1000*8)/(otg_info->ctime*1024));
    LOG_F(OTG,"[BACKGROUND] RX throughput = %.7f(Kbit/s) \n", ((double)rx_total_bytes_ul_background*1000*8)/(otg_info->ctime*1024));
  }

  if ((dl_ok == 1 ) && (ul_ok ==1))
    LOG_I(OTG,"************ DL and UL loss rate below 10 *************\n");
  else
    LOG_I(OTG,"************ DL(%d) and UL(%d) loss rate above 10 *************\n",dl_ok,ul_ok);

#endif
}


void add_log_metric(int src, int dst, int ctime, double metric, unsigned int label) {
  unsigned int i;
  unsigned int j;
  unsigned int node_actif=0;

  //LOG_I(OTG,"[%d][%d] LOGG_ADDED ctime=%d, metric=%.2f  \n", src, dst, ctime, metric);

  switch (label) {
    case OTG_LATENCY:
      add_log_label(label, &start_log_latency);
      break;

    case OTG_LATENCY_BG:
      add_log_label(label, &start_log_latency_bg);
      break;

    case OTG_GP:
      add_log_label(label, &start_log_GP);
      break;

    case OTG_GP_BG:
      add_log_label(label, &start_log_GP_bg);
      break;

    case OTG_JITTER:
      add_log_label(label, &start_log_GP_bg);
      break;
    default:
      LOG_E(OTG, "File label unknown %u \n", label);
  }

  LOG_F(label,"%d ", ctime);

  for (i=0; i<=(NB_eNB_INST + NB_UE_INST); i++) {
    for (j=0; j<=(NB_eNB_INST + NB_UE_INST); j++) {
      node_actif=0;

      if ((g_otg->idt_dist[i][j][0][PE_STATE]>0) || (g_otg->application_type[i][j][0] >0))
        node_actif=1;

      if ((node_actif>0) && ((i==src) && (j==dst)))
        LOG_F(label,"%f ", metric);

      if  ((node_actif>0) && ((i!=src) || (j!=dst)))
        LOG_F(label,"%d ", 0);
    }
  }

  LOG_F(label,"%f\n", metric);
}



void  add_log_label(unsigned int label, unsigned int *start_log_metric) {
  unsigned int i;
  unsigned int j;
  unsigned int node_actif=0;

  if (*start_log_metric==0) {
    *start_log_metric=1;
    LOG_F(label,"Time ");

    for (i=0; i<=(NB_eNB_INST + NB_UE_INST); i++) {
      for (j=0; j<=(NB_eNB_INST + NB_UE_INST); j++) {
        node_actif=0;

        if (g_otg->idt_dist[i][j][0][PE_STATE]>0 )
          node_actif++;

        if (node_actif>0)
          LOG_F(label,"%u->%u ", i, j);
      }
    }

    LOG_F(label,"Aggregated\n");
  }
}

