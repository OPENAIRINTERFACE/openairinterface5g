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

/*! \file otg_rx.c
 * \brief function containing the OTG traffic generation functions
 * \author Navid Nikaein and A. Hafsaoui
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#include "otg_rx.h"
#include "otg_externs.h"
#include "UTIL/MATH/oml.h"
#include <math.h>
#include "otg_form.h"
#include "otg_kpi.h"

#ifdef ENABLE_DB_STATS
#include <mysql.h>
#include <m_ctype.h>
#include <sql_common.h>
#endif

extern unsigned char NB_eNB_INST;
extern unsigned char NB_UE_INST;

//#include "LAYER2/MAC/extern.h"

#undef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#undef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))

// Check if the packet is well received or not and extract data
int otg_rx_pkt(const int dst_instanceP, const int ctime, const char * const buffer_tx, const unsigned int size)
{

  int              bytes_read      = 0;
  otg_hdr_info_t * otg_hdr_info_rx;
  otg_hdr_t *      otg_hdr_rx;
  int              is_size_ok      = 0;
  unsigned int     seq_num_rx;
  unsigned int     nb_loss_pkts;
  unsigned int     lost_packet     = 0;
  unsigned int     src_instance;
  unsigned int     dst_instance;

  if (buffer_tx!=NULL) {
    otg_hdr_info_rx = (otg_hdr_info_t *) (&buffer_tx[bytes_read]);
    bytes_read += sizeof (otg_hdr_info_t);

    LOG_D(OTG,"otg_rx_pkt functions: destination %d, size %d, otg_hdr_info_rx->flag %.4x, otg_hdr_info_rx->size %d \n",
          dst_instanceP,
          size,
          otg_hdr_info_rx->flag,
          otg_hdr_info_rx->size);


    if (((otg_hdr_info_rx->flag == 0xffff) ||
         (otg_hdr_info_rx->flag == 0xbbbb) ||
         (otg_hdr_info_rx->flag == 0x1000)) &&
        (otg_hdr_info_rx->size == size )) { //data traffic

      LOG_I(OTG,"MAX_RX_INFO %d %d \n",NB_eNB_INST,  NB_UE_INST);

      /*is_size_ok= 0;
      if (( otg_hdr_info_rx->size ) == size ) {*/
      is_size_ok= 1;
      otg_hdr_rx = (otg_hdr_t *) (&buffer_tx[bytes_read]);
      LOG_I(OTG,"[SRC %d][DST %d] [FLOW_idx %d][APP TYPE %d] RX INFO pkt at time %d: flag 0x %x, seq number %d, tx time %d, size (hdr %d, pdcp %d) \n",
            otg_hdr_rx->src_instance,
            otg_hdr_rx->dst_instance,
            otg_hdr_rx->flow_id,
            otg_hdr_rx->traffic_type,
            ctime,
            otg_hdr_info_rx->flag,
            otg_hdr_rx->seq_num,
            otg_hdr_rx->time,
            otg_hdr_info_rx->size, size);

      bytes_read += sizeof (otg_hdr_t);

      src_instance = otg_hdr_rx->src_instance;
      dst_instance = otg_hdr_rx->dst_instance;

      if (dst_instance != dst_instanceP) {
//#warning "LG: TODO think about multicast traffic"
        /* TODO: fix this LOG, a lot of missing parameters, replaced by a simple basic version */
        /*LOG_W(OTG,"[SRC %d][DST %d] [FLOW_idx %d][APP TYPE %d] RX INFO pkt at time %d: flag 0x %x, seq number %d, tx time %d, size (hdr %d, pdcp %d)  not for dest instance %u\n",
              dst_instanceP);*/
        LOG_W(OTG,"dst_instance != dst_instanceP\n");
      }

      if(otg_hdr_rx->traffic_type > MAX_NUM_APPLICATION) {
        LOG_W(OTG,"RX packet: application type out of range %d for the pair of (src %d, dst %d) \n",
              otg_hdr_rx->traffic_type, src_instance, dst_instance);
        otg_hdr_rx->traffic_type=0;
      }

      /** unicast traffic **/
      if (otg_hdr_info_rx->flag == 0xffff) {
        seq_num_rx=otg_info->seq_num_rx[src_instance][dst_instance][otg_hdr_rx->traffic_type];

        if (src_instance<NB_eNB_INST)
          nb_loss_pkts=otg_info->nb_loss_pkts_dl[src_instance][dst_instance][otg_hdr_rx->traffic_type];
        else
          nb_loss_pkts=otg_info->nb_loss_pkts_ul[src_instance][dst_instance][otg_hdr_rx->traffic_type];
      }  /** multicast  traffic **/
      else if (otg_hdr_info_rx->flag == 0x1000) {
        seq_num_rx = otg_multicast_info->rx_sn[src_instance][dst_instance][otg_hdr_rx->traffic_type];
        nb_loss_pkts = otg_multicast_info->loss_pkts_dl[src_instance][dst_instance][otg_hdr_rx->traffic_type];
        //  otg_multicast_info->ran_owd[src_instance][dst_instance][otg_hdr_rx->traffic_type] = ctime- otg_hdr_rx->time;
        //  rx_check_loss(src_instance, dst_instance, otg_hdr_info_rx->flag, otg_hdr_rx->seq_num, &seq_num_rx, &nb_loss_pkts);
        //  otg_multicast_info->loss_rate[src_instance][dst_instance][otg_hdr_rx->traffic_type]=nb_loss_pkts;
        //otg_multicast_info->rx_sn[src_instance][dst_instance][otg_hdr_rx->traffic_type]=seq_num_rx;
        LOG_I(OTG,"received a multicast packet with size %d sn %d ran owd %d loss rate %d\n",
              otg_hdr_info_rx->size, seq_num_rx, ctime- otg_hdr_rx->time, nb_loss_pkts);
        //return 0;
      }  /** background traffic **/
      else {
        seq_num_rx=otg_info->seq_num_rx_background[src_instance][dst_instance];

        if (src_instance<NB_eNB_INST)
          nb_loss_pkts=otg_info->nb_loss_pkts_background_dl[src_instance][dst_instance];
        else
          nb_loss_pkts=otg_info->nb_loss_pkts_background_ul[src_instance][dst_instance];
      }


      LOG_D(OTG,"[%d][%d] AGGREGATION LEVEL (RX) %d \n", src_instance, dst_instance, otg_hdr_rx->aggregation_level);
      otg_info->aggregation_level[src_instance][dst_instance]=otg_hdr_rx->aggregation_level;

      /* Loss and out of sequence data management */
      lost_packet= rx_check_loss(
                     src_instance,
                     dst_instance,
                     otg_hdr_info_rx->flag,
                     otg_hdr_rx->seq_num,
                     &seq_num_rx,
                     &nb_loss_pkts);


      if (otg_info->owd_const[src_instance][dst_instance][otg_hdr_rx->flow_id]==0)
        owd_const_gen(src_instance,dst_instance,otg_hdr_rx->flow_id, otg_hdr_rx->traffic_type);


      /******/
      /*
      float owd_const_capillary_v=owd_const_capillary()/2;
      float owd_const_mobile_core_v=owd_const_mobile_core()/2;
      float owd_const_IP_backbone_v=owd_const_IP_backbone()/2;
      float owd_const_application_v=owd_const_application()/2;

      FILE *file;
      file = fopen("/tmp/log_latency_m2m.txt", "a");
      fprintf(file," %d %d [%d] [%d]  %.2f %.2f %.2f %.2f %.2f %.2f\n", otg_hdr_rx->time, otg_hdr_info_rx->size, src, dst, owd_const_capillary_v,otg_info->radio_access_delay[src][dst],owd_const_mobile_core_v, owd_const_IP_backbone_v, owd_const_application_v, owd_const_capillary_v + otg_info->radio_access_delay[src][dst]+owd_const_mobile_core_v + owd_const_IP_backbone_v+ owd_const_application_v);
      fclose(file);
       */
      /******/
      //  }

      if (otg_hdr_rx->time<=ctime) {
        otg_info->radio_access_delay[src_instance][dst_instance]=(float) (ctime- otg_hdr_rx->time);
        otg_multicast_info->radio_access_delay[src_instance][dst_instance]=(float) (ctime- otg_hdr_rx->time);
      } else {
        LOG_N(OTG,"received packet has tx time %d greater than the current time %d\n",otg_hdr_rx->time,ctime );
        otg_info->radio_access_delay[src_instance][dst_instance] = 0;
        otg_multicast_info->radio_access_delay[src_instance][dst_instance]=0;
      }

      /* actual radio OWD*/
      otg_info->rx_pkt_owd[src_instance][dst_instance]=otg_info->radio_access_delay[src_instance][dst_instance];
      /* estimated E2E OWD based on the emulated delays for the other part of the network */
      otg_info->rx_pkt_owd_e2e[src_instance][dst_instance]=
        otg_info->owd_const[src_instance][dst_instance][otg_hdr_rx->flow_id] + otg_info->radio_access_delay[src_instance][dst_instance];
      otg_multicast_info->rx_pkt_owd[src_instance][dst_instance]=otg_multicast_info->radio_access_delay[src_instance][dst_instance];

      LOG_D(OTG, "[src %d][dst %d] ctime %d tx time %d: OWD %lf E2E OWD %lf \n",
            src_instance,
            dst_instance,
            ctime,
            otg_hdr_rx->time,
            otg_info->rx_pkt_owd[src_instance][dst_instance],
            otg_info->rx_pkt_owd_e2e[src_instance][dst_instance] );

      // compute the jitter by ignoring the packet loss
      if (lost_packet == 0) {
        // radio access
        otg_info->rx_pkt_owd_history[src_instance][dst_instance][1] = otg_info->rx_pkt_owd_history[src_instance][dst_instance][0]; // the previous owd
        otg_info->rx_pkt_owd_history[src_instance][dst_instance][0] = otg_info->rx_pkt_owd[src_instance][dst_instance]; // the current owd

        if (otg_info->rx_pkt_owd_history[src_instance][dst_instance][1] == 0) // first packet
          otg_info->rx_pkt_jitter[src_instance][dst_instance]=0;
        else // for the consecutive packets
          otg_info->rx_pkt_jitter[src_instance][dst_instance]=
            abs(otg_info->rx_pkt_owd_history[src_instance][dst_instance][0] - otg_info->rx_pkt_owd_history[src_instance][dst_instance][1]);

        LOG_D(OTG,"The packet jitter for the pair (src %d, dst %d)) at %d is %lf (current %lf, previous %lf) \n",
              src_instance, dst_instance, ctime, otg_info->rx_pkt_jitter[src_instance][dst_instance],
              otg_info->rx_pkt_owd_history[src_instance][dst_instance][0], otg_info->rx_pkt_owd_history[src_instance][dst_instance][1]);
        // e2e
        otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][1] = otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][0]; // the previous owd
        otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][0] = otg_info->rx_pkt_owd_e2e[src_instance][dst_instance]; // the current owd

        if (otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][1] == 0) // first packet
          otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance]=0;
        else // for the consecutive packets
          otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance]=
            abs(otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][0] - otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][1]);

        LOG_D(OTG,"The packet jitter for the pair (src %d, dst %d)) at %d is %lf (current %lf, previous %lf) \n",
              src_instance,
              dst_instance,
              ctime,
              otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance],
              otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][0],
              otg_info->rx_pkt_owd_history_e2e[src_instance][dst_instance][1]);

      }

      if (otg_hdr_info_rx->flag == 0x1000) {
        LOG_I(OTG,"[SRC%d -> DST %d] Received a multicast packet at time %d with size %d, seq num %d, ran owd %d number loss packet %d\n",
              src_instance,
              dst_instance,
              ctime,otg_hdr_info_rx->size,
              otg_hdr_rx->seq_num,
              ctime - otg_hdr_rx->time,
              nb_loss_pkts);

        LOG_I(OTG,"INFO LATENCY :: [SRC %d][DST %d] radio access %.2f (tx time %d, ctime %d), OWD:%d (ms):\n",
              src_instance,
              dst_instance,
              otg_multicast_info->radio_access_delay[src_instance][dst_instance],
              otg_hdr_rx->time, ctime ,
              otg_multicast_info->rx_pkt_owd[src_instance][dst_instance]);

        if (otg_multicast_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]==0) {
          otg_multicast_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_multicast_info->rx_pkt_owd[src_instance][dst_instance];
          otg_multicast_info->rx_owd_min[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_multicast_info->rx_pkt_owd[src_instance][dst_instance];
        } else {
          otg_multicast_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MAX(
                otg_multicast_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_multicast_info->rx_pkt_owd[src_instance][dst_instance] );
          otg_multicast_info->rx_owd_min[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MIN(
                otg_multicast_info->rx_owd_min[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_multicast_info->rx_pkt_owd[src_instance][dst_instance] );
        }

        if (g_otg->curve==1) {
          if (g_otg->owd_radio_access==0)
            add_tab_metric(
              src_instance,
              dst_instance,
              otg_multicast_info->rx_pkt_owd[src_instance][dst_instance],
              ((otg_hdr_info_rx->size*1000*8)/(otg_multicast_info->rx_pkt_owd[src_instance][dst_instance]*1024 )),
              otg_hdr_rx->time);
          else
            add_tab_metric(src_instance,
                           dst_instance,
                           otg_multicast_info->radio_access_delay[src_instance][dst_instance],
                           ((otg_hdr_info_rx->size*1000*8)/(otg_multicast_info->rx_pkt_owd[src_instance][dst_instance]*1024 )),
                           otg_hdr_rx->time);
        }

        otg_multicast_info->rx_total_bytes_dl+=otg_hdr_info_rx->size;
      } else {

        LOG_I(OTG,"[SRC %d][DST %d] Stats :: radio access latency %.2f (tx time %d, ctime %d) jitter %.2f, Estimated E2E OWD:%.2f (ms):\n",
              src_instance,
              dst_instance,
              otg_info->radio_access_delay[src_instance][dst_instance],
              otg_hdr_rx->time, ctime ,
              otg_info->rx_pkt_jitter[src_instance][dst_instance],
              otg_info->rx_pkt_owd_e2e[src_instance][dst_instance]);

        if (otg_hdr_info_rx->flag == 0xffff) {

          if (otg_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]==0) {
            otg_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_owd[src_instance][dst_instance];
            otg_info->rx_owd_min[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_owd[src_instance][dst_instance];
            otg_info->rx_owd_max_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_owd_e2e[src_instance][dst_instance];
            otg_info->rx_owd_min_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_owd_e2e[src_instance][dst_instance];
          } else {
            otg_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MAX(otg_info->rx_owd_max[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_owd[src_instance][dst_instance] );
            otg_info->rx_owd_min[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MIN(otg_info->rx_owd_min[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_owd[src_instance][dst_instance] );
            otg_info->rx_owd_max_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MAX(otg_info->rx_owd_max_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_owd_e2e[src_instance][dst_instance] );
            otg_info->rx_owd_min_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MIN(otg_info->rx_owd_min_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_owd_e2e[src_instance][dst_instance] );
          }

          if (otg_info->rx_jitter_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]==0) {
            otg_info->rx_jitter_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_jitter[src_instance][dst_instance];
            otg_info->rx_jitter_min[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_jitter[src_instance][dst_instance];
            otg_info->rx_jitter_max_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance];
            otg_info->rx_jitter_min_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance];
          } else if (lost_packet==0) {
            otg_info->rx_jitter_max[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MAX(otg_info->rx_jitter_max[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_jitter[src_instance][dst_instance] );
            otg_info->rx_jitter_min[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MIN(otg_info->rx_jitter_min[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_jitter[src_instance][dst_instance] );
            otg_info->rx_jitter_max_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MAX(otg_info->rx_jitter_max_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance] );
            otg_info->rx_jitter_min_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type]=MIN(otg_info->rx_jitter_min_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type],
                otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance] );
            // avg jitter
            otg_info->rx_jitter_avg[src_instance][dst_instance][otg_hdr_rx->traffic_type] +=  otg_info->rx_pkt_jitter[src_instance][dst_instance];
            otg_info->rx_jitter_avg_e2e[src_instance][dst_instance][otg_hdr_rx->traffic_type] +=  otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance];
            otg_info->rx_jitter_sample[src_instance][dst_instance][otg_hdr_rx->traffic_type] +=1;
          }

        }

        if (g_otg->curve==1) {
          if (g_otg->owd_radio_access==0)
            add_tab_metric(
              src_instance,
              dst_instance,
              otg_info->rx_pkt_owd[src_instance][dst_instance],
              ((otg_hdr_info_rx->size*1000*8)/(otg_info->rx_pkt_owd[src_instance][dst_instance]*1024 )),
              otg_hdr_rx->time);
          else
            add_tab_metric(
              src_instance,
              dst_instance, otg_info->radio_access_delay[src_instance][dst_instance],
              ((otg_hdr_info_rx->size*1000*8)/(otg_info->rx_pkt_owd[src_instance][dst_instance]*1024 )),
              otg_hdr_rx->time);
        }

        if (src_instance<NB_eNB_INST)
          otg_info->rx_total_bytes_dl+=otg_hdr_info_rx->size;
        else
          otg_info->rx_total_bytes_ul+=otg_hdr_info_rx->size;

      }

      //LOG_I(OTG,"RX INFO :: RTT MIN(one way) ms: %.2f, RTT MAX(one way) ms: %.2f \n", otg_info->rx_owd_min[src][dst], otg_info->rx_owd_max[src][dst]);

      /* xforms part: add metrics  */

      //printf("payload_size %d, header_size %d \n", otg_hdr_rx->pkts_size, otg_hdr_rx->hdr_type);
      LOG_I(OTG,"[RX] OTG packet, PACKET SIZE [SRC %d][DST %d]: Flag (0x%x), Traffic %d, time(%d), Seq num (%d), Total size (%d)\n",
            src_instance,
            dst_instance,
            otg_hdr_info_rx->flag,
            otg_hdr_rx->traffic_type,
            ctime,
            otg_hdr_rx->seq_num, size);
      /*LOG_I(OTG,"details::RX [SRC %d][DST %d]: Flag (0x%x), time(%d), Seq num (%d), Total size (%d), header(%d), payload (%d) \n",  src, dst, otg_hdr_info_rx->flag, ctime, otg_hdr_rx->seq_num, size, strlen(packet_rx->header), strlen(packet_rx->payload));*/


      if (otg_hdr_info_rx->flag == 0xffff) {
        otg_info->rx_num_pkt[src_instance][dst_instance][otg_hdr_rx->traffic_type]+=1;
        otg_info->rx_num_bytes[src_instance][dst_instance][otg_hdr_rx->traffic_type]+=otg_hdr_info_rx->size;
        otg_info->seq_num_rx[src_instance][dst_instance][otg_hdr_rx->traffic_type]=seq_num_rx;

        if (src_instance<NB_eNB_INST)
          otg_info->nb_loss_pkts_dl[src_instance][dst_instance][otg_hdr_rx->traffic_type]=nb_loss_pkts;
        else
          otg_info->nb_loss_pkts_ul[src_instance][dst_instance][otg_hdr_rx->traffic_type]=nb_loss_pkts;

        /*Plots of latency and goodput are only plotted for the data traffic*/
        /*measurements are done for the data and background traffic */

        if (g_otg->latency_metric) {
          if (g_otg->owd_radio_access==0) {
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_info->rx_pkt_owd[src_instance][dst_instance], OTG_LATENCY);
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_info->rx_pkt_jitter_e2e[src_instance][dst_instance], OTG_JITTER);
          } else {
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_info->radio_access_delay[src_instance][dst_instance], OTG_LATENCY);
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_info->rx_pkt_jitter[src_instance][dst_instance], OTG_JITTER);
          }
        }

        if (g_otg->throughput_metric)
          add_log_metric(src_instance,
                         dst_instance,
                         otg_hdr_rx->time,
                         ((otg_hdr_info_rx->size*1000*8)/(otg_info->rx_pkt_owd[src_instance][dst_instance]*1024 )),
                         OTG_GP); /* compute the throughput in Kbit/s  */

      } else if (otg_hdr_info_rx->flag == 0x1000) {
        otg_multicast_info->rx_num_pkt[src_instance][dst_instance][otg_hdr_rx->traffic_type]+=1;
        otg_multicast_info->rx_num_bytes[src_instance][dst_instance][otg_hdr_rx->traffic_type]+=otg_hdr_info_rx->size;
        //  LOG_D(OTG,"DUY: otg_multicast_info->rx_num_bytes[%d][%d][%d] is %d \nn",src,dst,otg_hdr_rx->traffic_type,otg_multicast_info->rx_num_bytes[src][dst][otg_hdr_rx->traffic_type]);
        otg_multicast_info->rx_sn[src_instance][dst_instance][otg_hdr_rx->traffic_type]=seq_num_rx;
        otg_multicast_info->loss_pkts_dl[src_instance][dst_instance][otg_hdr_rx->traffic_type]=nb_loss_pkts;

        /*Plots of latency and goodput are only plotted for the data traffic*/
        /*measurements are done for the data and background traffic */

        if (g_otg->latency_metric) {
          if (g_otg->owd_radio_access==0)
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_multicast_info->rx_pkt_owd[src_instance][dst_instance], OTG_LATENCY);
          else
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_multicast_info->radio_access_delay[src_instance][dst_instance], OTG_LATENCY);
        }

        if (g_otg->throughput_metric)
          add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, ((otg_hdr_info_rx->size*1000*8)/(otg_multicast_info->rx_pkt_owd[src_instance][dst_instance]*1024 )),
                         OTG_GP); /* compute the throughput in Kbit/s  */
      } else {
        otg_info->rx_num_pkt_background[src_instance][dst_instance]+=1;
        otg_info->rx_num_bytes_background[src_instance][dst_instance]+=otg_hdr_info_rx->size;
        otg_info->seq_num_rx_background[src_instance][dst_instance]=seq_num_rx;

        if (src_instance<NB_eNB_INST)
          otg_info->nb_loss_pkts_background_dl[src_instance][dst_instance]=nb_loss_pkts;
        else
          otg_info->nb_loss_pkts_background_ul[src_instance][dst_instance]=nb_loss_pkts;

        if (g_otg->latency_metric) {
          if (g_otg->owd_radio_access==0)
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_info->rx_pkt_owd[src_instance][dst_instance], OTG_LATENCY_BG);
          else
            add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, otg_info->radio_access_delay[src_instance][dst_instance], OTG_LATENCY_BG);
        }

        if (g_otg->throughput_metric)
          add_log_metric(src_instance, dst_instance, otg_hdr_rx->time, ((otg_hdr_info_rx->size*1000*8)/(otg_info->rx_pkt_owd[src_instance][dst_instance]*1024 )), OTG_GP_BG);
      }

      if (is_size_ok == 0) {
        otg_hdr_rx = (otg_hdr_t *) (&buffer_tx[bytes_read]);
        LOG_W(OTG,"[SRC %d][DST %d] RX pkt: seq number %d size mis-matche (hdr %d, pdcp %d) \n",
              src_instance, dst_instance, otg_hdr_rx->seq_num, otg_hdr_info_rx->size, size);
        otg_info->nb_loss_pkts_otg[src_instance][dst_instance]++;
      }

      return(0);
    } else {
      LOG_W(OTG,"RX: Not an OTG pkt, forward to upper layer (flag %x, size %d, pdcp_size %d) FIX ME \n",
            otg_hdr_info_rx->flag, otg_hdr_info_rx->size, size);
      return(1); //to be fixed on the real case to one
    }

  }

  return(0);
}




int rx_check_loss(
  const int src,
  const int dst,
  const unsigned int flag,
  const int seq_num,
  unsigned int * const seq_num_rx,
  unsigned int * const nb_loss_pkts)
{

  /* Loss and out of sequence data management, we have 3 case : */
  /* (1) Receieved packet corresponds to the expected one, in terms of the sequence number*/
  int lost_packet=0;

  if (seq_num==*seq_num_rx) {
    LOG_D(OTG,"check_packet :: (src=%d,dst=%d, flag=0x%x) packet seq_num TX=%d, seq_num RX=%d \n",src,dst,flag, seq_num, *seq_num_rx);
    *seq_num_rx+=1;
  }
  /* (2) Receieved packet with a sequence number higher than the expected sequence number (there is a gap): packet loss */
  else if (seq_num>*seq_num_rx) { // out of sequence packet:  previous packet lost
    LOG_D(OTG,"check_packet :: (src=%d,dst=%d, flag=0x%x) :: out of sequence :: packet seq_num TX=%d > seq_num RX=%d \n",src,dst,flag, seq_num, *seq_num_rx);
    *nb_loss_pkts+=seq_num-(*seq_num_rx);
    *seq_num_rx=seq_num+1;
    lost_packet=1;
  }
  /* (3) Receieved packet with a sequence number less than the expected sequence number: recovery after loss/out of sequence  */
  else if (seq_num< *seq_num_rx) { //the received packet arrived late
    *nb_loss_pkts-=1;
    LOG_D(OTG,"check_packet :: (src=%d,dst=%d, flag=0x%x) :: recovery after loss or out of sequence :: packet seq_num TX=%d < seq_num RX=%d \n",src,dst,flag, seq_num, *seq_num_rx);
  } else {
    LOG_D(OTG,"check_packet :: (src=%d,dst=%d, flag=0x%x) ::  packet seq_num TX=%d , seq_num RX=%d (ERROR)\n",src,dst,flag, seq_num, *seq_num_rx);
    lost_packet=1;
  }

  return lost_packet;
}




void owd_const_gen(
  const int src,
  const int dst,
  const int flow_id,
  const unsigned int flag)
{
  otg_info->owd_const[src][dst][flow_id]=(owd_const_mobile_core()+owd_const_IP_backbone()+owd_const_application())/2;

  if ((flag==M2M)||(flag==M2M_TRAFFIC)||(flag==AUTO_PILOT_L)||(flag==AUTO_PILOT_M)||(flag==AUTO_PILOT_H)||(flag==VIRTUAL_GAME_L)||(flag==VIRTUAL_GAME_M)|| (flag==VIRTUAL_GAME_H)||(flag==VIRTUAL_GAME_F)
      ||(flag==ALARM_HUMIDITY)||(flag==ALARM_SMOKE)||(flag==ALARM_TEMPERATURE)||(flag==OPENARENA)||(flag==IQSIM_MANGO)||(flag==IQSIM_NEWSTEO)) {
    otg_info->owd_const[src][dst][flow_id]+=(owd_const_capillary()/2);
    LOG_D(OTG,"(RX) [src %d] [dst %d] [ID %d] TRAFFIC_TYPE IS M2M [Add Capillary const]\n", src, dst, flow_id);
  } else
    LOG_T(OTG,"(RX) [src %d] [dst %d] [ID %d] TRAFFIC_TYPE WITHOUT M2M [Capillary const]\n", src, dst, flow_id);
}



float owd_const_capillary(void)
{
  /*return (uniform_dist(MIN_APPLICATION_PROCESSING_GATEWAY_DELAY, MAX_APPLICATION_PROCESSING_GATEWAY_DELAY) +
   uniform_dist(MIN_FORMATING_TRANSFERRING_DELAY, MAX_FORMATING_TRANSFERRING_DELAY) +
   uniform_dist(MIN_ACCESS_DELAY, MAX_ACCESS_DELAY) +
   TERMINAL_ACCESS_DELAY);*/
  return ((double)MIN_APPLICATION_PROCESSING_GATEWAY_DELAY+ (double)MAX_APPLICATION_PROCESSING_GATEWAY_DELAY + (double)MIN_FORMATING_TRANSFERRING_DELAY+ (double)MAX_FORMATING_TRANSFERRING_DELAY+
          (double)MIN_ACCESS_DELAY+(double)MAX_ACCESS_DELAY) /2 + (double)TERMINAL_ACCESS_DELAY;
}


float owd_const_mobile_core(void)
{
  /*double delay;
  // this is a delay model for a loaded GGSN according to
  //"M. Laner, P. Svoboda and M. Rupp, Latency Analysis of 3G Network Components, EW'12, Poznan, Poland, 2012", table 2, page 6.
  if(uniform_rng ()<0.3){
  delay=uniform_dist (0.4,1.2);
  }else{
  // in this case, according to the fit in the paper,
  //     the delay is generalized pareto: GP(k=0.75,s=0.55,t=1.2)
  //     using inverse cdf method we have CDF(x)=1-(k(x-t)/s+1)^(-1/k),
  //  x=CDF^(-1)(u)=t+s/k*((1-u)^(-k)-1) , hence when u~uniform, than x~GP(k,s,t)

  double k,s,t,u;
  k=0.75;
  s=0.55;
  t=1.2;
  u=uniform_rng();
  delay= t + s/k*(pow(1-u,-k)-1);
  }
  return delay; */
  /*return ( uniform_dist(MIN_U_PLANE_CORE_IP_ACCESS_DELAY, MAX_U_PLANE_CORE_IP_ACCESS_DELAY) + uniform_dist(MIN_FW_PROXY_DELAY,MAX_FW_PROXY_DELAY)); */
  return ((double)MIN_U_PLANE_CORE_IP_ACCESS_DELAY+ (double)MAX_U_PLANE_CORE_IP_ACCESS_DELAY + (double)MIN_FW_PROXY_DELAY + (double)MAX_FW_PROXY_DELAY)/2;
}

float owd_const_IP_backbone(void)
{
  /*return uniform_dist(MIN_NETWORK_ACCESS_DELAY,MAX_NETWORK_ACCESS_DELAY);*/
  return ((double)MIN_NETWORK_ACCESS_DELAY+(double)MAX_NETWORK_ACCESS_DELAY)/2;

}

float owd_const_application(void)
{
  /*return uniform_dist(MIN_APPLICATION_ACESS_DELAY, MAX_APPLICATION_ACESS_DELAY);*/
  return ((double)MIN_APPLICATION_ACESS_DELAY+(double)MAX_APPLICATION_ACESS_DELAY)/2;
}




