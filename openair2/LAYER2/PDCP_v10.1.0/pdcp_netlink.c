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

/*! \file pdcp_netlink.c
 * \brief pdcp communication with linux IP interface,
 * have a look at http://man7.org/linux/man-pages/man7/netlink.7.html for netlink.
 * Read operation from netlink should be achieved in an asynchronous way to avoid
 * subframe overload, netlink congestion...
 * \author Sebastien ROUX
 * \date 2013
 * \version 0.1
 * @ingroup pdcp
 */

#define _GNU_SOURCE // required for pthread_setname_np()
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <error.h>
#include <unistd.h>

/* Bugfix for version of GCC = 4.4.3 (Ubuntu 10.04) */
#if GCC_VERSION <= 40403
  #include <sys/socket.h>
#endif

#include <linux/netlink.h>

#include "assertions.h"
#include "queue.h"
#include "liblfds611.h"

#include "common/utils/LOG/log.h"
#include "common/utils/system.h"
#include "LAYER2/MAC/mac_extern.h"

#include "pdcp.h"
#include "pdcp_primitives.h"


#define PDCP_QUEUE_NB_ELEMENTS 200

extern char nl_rx_buf[NL_MAX_PAYLOAD];
extern struct nlmsghdr *nas_nlh_rx;
extern struct iovec nas_iov_rx;
extern int nas_sock_fd;
extern struct msghdr nas_msg_rx;


static pthread_t pdcp_netlink_thread;

/* We use lock-free queues between the User-plane driver running in kernel-space
 * and the corresponding entity in User-space.
 * one queue for eNBs (index 0)/one queue for UEs (index 1)
 */
static struct lfds611_queue_state **pdcp_netlink_queue_enb = NULL;
static struct lfds611_queue_state **pdcp_netlink_queue_ue = NULL;
static uint32_t *pdcp_netlink_nb_element_enb = NULL;
static uint32_t *pdcp_netlink_nb_element_ue = NULL;

time_stats_t ip_pdcp_stats_tmp;

static void *pdcp_netlink_thread_fct(void *arg);

//-----------------------------------------------------------------------------
int
pdcp_netlink_init(
  void
)
//-----------------------------------------------------------------------------
{
  int                i;
  int                nb_inst_enb;
  int                nb_inst_ue;

  reset_meas(&ip_pdcp_stats_tmp);
  nb_inst_enb = 1;
  nb_inst_ue  = 1;

  if (LINK_ENB_PDCP_TO_GTPV1U) {
    nb_inst_enb = 0;
    LOG_I(PDCP, "[NETLINK] Creating 0 queues for eNB Netlink -> PDCP communication\n");
  } else {
    /* #warning " LG: When there will be handover in, there will problems because dim is based on local nums of ues" */
    pdcp_netlink_queue_enb      = calloc(nb_inst_enb, sizeof(struct lfds611_queue_state *));
    pdcp_netlink_nb_element_enb = malloc(nb_inst_enb * sizeof(uint32_t));
    LOG_I(PDCP, "[NETLINK] Creating %d queues for eNB Netlink -> PDCP communication\n", nb_inst_enb);

    for (i = 0; i < nb_inst_enb; i++) {
      pdcp_netlink_nb_element_enb[i] = 0;

      if (lfds611_queue_new(&pdcp_netlink_queue_enb[i], PDCP_QUEUE_NB_ELEMENTS) < 0) {
        LOG_E(PDCP, "Failed to create new FIFO for eNB Netlink -> PDCP communcation instance %d\n", i);
        exit(EXIT_FAILURE);
      }
    }
  }

  if (nb_inst_ue  > 0) {
    pdcp_netlink_queue_ue       = calloc(nb_inst_ue, sizeof(struct lfds611_queue_state *));
    pdcp_netlink_nb_element_ue  = malloc(nb_inst_ue * sizeof(uint32_t));
    LOG_I(PDCP, "[NETLINK] Creating %d queues for UE Netlink -> PDCP communication\n", nb_inst_ue);

    for (i = 0; i < nb_inst_ue; i++) {
      pdcp_netlink_nb_element_ue[i] = 0;

      if (lfds611_queue_new(&pdcp_netlink_queue_ue[i], PDCP_QUEUE_NB_ELEMENTS) < 0) {
        LOG_E(PDCP, "Failed to create new FIFO for UE Netlink -> PDCP communcation instance %d\n", i);
        exit(EXIT_FAILURE);
      }
    }
  }

  if ((nb_inst_ue + nb_inst_enb) > 0) {
    /* Create one thread that fetchs packets from the netlink.
     * When the netlink fifo is full, packets are silently dropped, this behaviour
     * should be avoided if we want a reliable link.
     */
    threadCreate(&pdcp_netlink_thread, pdcp_netlink_thread_fct, (void*)NULL, "PDCP netlink", -1, OAI_PRIORITY_RT_LOW );
  }

  return 0;
}

//-----------------------------------------------------------------------------
int
pdcp_netlink_dequeue_element(
  const protocol_ctxt_t *const  ctxt_pP,
  struct pdcp_netlink_element_s **data_ppP
)
//-----------------------------------------------------------------------------
{
  int ret = 0;

  if (ctxt_pP->enb_flag) {
    ret = lfds611_queue_dequeue(pdcp_netlink_queue_enb[ctxt_pP->module_id], (void **)data_ppP);

    if (ret != 0) {
      LOG_D(PDCP,"[NETLINK]De-queueing packet for eNB instance %d\n", ctxt_pP->module_id);
    }
  } else {
    ret = lfds611_queue_dequeue(pdcp_netlink_queue_ue[ctxt_pP->module_id], (void **)data_ppP);

    if (ret != 0) {
      LOG_D(PDCP, "[NETLINK]De-queueing packet for UE instance %d\n", ctxt_pP->module_id);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------
static
void *pdcp_netlink_thread_fct(void *arg)
//-----------------------------------------------------------------------------
{
  int                            len             = 0;
  struct pdcp_netlink_element_s *new_data_p      = NULL;
  uint8_t                        pdcp_thread_read_state ;
  eNB_flag_t                     eNB_flag        = 0;
  module_id_t                    module_id       = 0;
  pdcp_thread_read_state = 0;
  memset(nl_rx_buf, 0, NL_MAX_PAYLOAD);
  LOG_I(PDCP, "[NETLINK_THREAD] binding to fd  %d\n",nas_sock_fd);

  while (1) {
    len = recvmsg(nas_sock_fd, &nas_msg_rx, 0);

    if (len == 0) {
      /* Other peer (kernel) has performed an orderly shutdown
       */
      LOG_E(PDCP, "[NETLINK_THREAD] Kernel module has closed the netlink\n");
      exit(EXIT_FAILURE);
    } else if (len < 0) {
      /* There was an error */
      LOG_E(PDCP, "[NETLINK_THREAD] An error occured while reading netlink (%d:%s)\n",
            errno, strerror(errno));
      exit(EXIT_FAILURE);
    } else {
      /* Normal read.
       * NOTE: netlink messages can be assembled to form a multipart message
       */
      for (nas_nlh_rx = (struct nlmsghdr *) nl_rx_buf;
           NLMSG_OK(nas_nlh_rx, (unsigned int)len);
           nas_nlh_rx = NLMSG_NEXT (nas_nlh_rx, len)) {
        start_meas(&ip_pdcp_stats_tmp);

        /* There is no need to check for nlmsg_type because
         * the header is not set in our drivers.
         */
        if (pdcp_thread_read_state == 0) {
          new_data_p = malloc(sizeof(struct pdcp_netlink_element_s));

          if (nas_nlh_rx->nlmsg_len == sizeof (pdcp_data_req_header_t) + sizeof(struct nlmsghdr)) {
            pdcp_thread_read_state = 1;
            memcpy((void *)&new_data_p->pdcp_read_header, (void *)NLMSG_DATA(nas_nlh_rx), sizeof(pdcp_data_req_header_t));
            LOG_I(PDCP, "[NETLINK_THREAD] RX pdcp_data_req_header_t inst %u, "
                  "rb_id %ld data_size %d\n",
                  new_data_p->pdcp_read_header.inst,
                  new_data_p->pdcp_read_header.rb_id,
                  new_data_p->pdcp_read_header.data_size);
          } else {
            LOG_E(PDCP, "[NETLINK_THREAD] WRONG size %d should be sizeof "
                  "%lu ((pdcp_data_req_header_t) + sizeof(struct nlmsghdr))\n",
                  nas_nlh_rx->nlmsg_len,
                  sizeof (pdcp_data_req_header_t) + sizeof(struct nlmsghdr));
          }
        } else {
          pdcp_thread_read_state = 0;
          module_id = 0;
          new_data_p->data = malloc(sizeof(uint8_t) * new_data_p->pdcp_read_header.data_size);
          /* Copy the data */
          memcpy(new_data_p->data, NLMSG_DATA(nas_nlh_rx), new_data_p->pdcp_read_header.data_size);

          if (eNB_flag) {
            if (pdcp_netlink_nb_element_enb[module_id]
                > PDCP_QUEUE_NB_ELEMENTS) {
              LOG_E(PDCP, "[NETLINK_THREAD][Mod %02x] We reached maximum number of elements in eNB pdcp queue (%lu)\n",
                    module_id, (intptr_t)pdcp_netlink_nb_element_enb);
            }

            LOG_I(PDCP,"[NETLINK_THREAD] IP->PDCP : En-queueing packet for eNB module id %d\n", module_id);
            /* Enqueue the element in the right queue */
            lfds611_queue_guaranteed_enqueue(pdcp_netlink_queue_enb[module_id], new_data_p);
            stop_meas(&ip_pdcp_stats_tmp);
            copy_meas(&eNB_pdcp_stats[module_id].pdcp_ip,&ip_pdcp_stats_tmp);
          } else {
            if (pdcp_netlink_nb_element_ue[module_id]
                > PDCP_QUEUE_NB_ELEMENTS) {
              LOG_E(PDCP, "[NETLINK_THREAD][Mod %02x] We reached maximum number of elements in UE pdcp queue (%lu)\n",
                    module_id, (intptr_t)pdcp_netlink_nb_element_ue);
            }

            LOG_I(PDCP,"[NETLINK_THREAD] IP->PDCP : En-queueing packet for UE module id  %d\n", module_id);
            /* Enqueue the element in the right queue */
            lfds611_queue_guaranteed_enqueue(pdcp_netlink_queue_ue[module_id], new_data_p);
            stop_meas(&ip_pdcp_stats_tmp);
            copy_meas(&UE_pdcp_stats[module_id].pdcp_ip,&ip_pdcp_stats_tmp);
          }
        }
      }
    }
  }

  return NULL;
}

