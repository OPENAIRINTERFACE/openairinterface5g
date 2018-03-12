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

/*! \file pdcp_fifo.c
 * \brief pdcp interface with linux IP interface, have a look at http://man7.org/linux/man-pages/man7/netlink.7.html for netlink
 * \author  Navid Nikaein and Lionel GAUTHIER
 * \date 2009 - 2016
 * \version 0.5
 * \email navid.nikaein@eurecom.fr
 * \warning This component can be runned only in user-space
 * @ingroup pdcp
 */

#define PDCP_FIFO_C
#define PDCP_DEBUG 1
//#define DEBUG_PDCP_FIFO_FLUSH_SDU

extern int otg_enabled;

#include "pdcp.h"
#include "pdcp_primitives.h"

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define rtf_put write
#define rtf_get read

#include "../MAC/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "NETWORK_DRIVER/LITE/constant.h"
#include "SIMULATION/ETH_TRANSPORT/extern.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OCG/OCG_extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/FIFO/pad_list.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "platform_constants.h"
#include "msc.h"

#include "assertions.h"

#ifdef PDCP_USE_NETLINK
#include <sys/socket.h>
#include <linux/netlink.h>
#include "NETWORK_DRIVER/UE_IP/constant.h"

extern char nl_rx_buf[NL_MAX_PAYLOAD];
extern struct sockaddr_nl nas_src_addr, nas_dest_addr;
extern struct nlmsghdr *nas_nlh_tx;
extern struct nlmsghdr *nas_nlh_rx;
extern struct iovec nas_iov_tx;
extern struct iovec nas_iov_rx;
extern int nas_sock_fd;
extern struct msghdr nas_msg_tx;
extern struct msghdr nas_msg_rx;

unsigned char pdcp_read_state_g = 0;
#endif

extern Packet_OTG_List_t *otg_pdcp_buffer;

#if defined(LINK_ENB_PDCP_TO_GTPV1U)
#  include "gtpv1u_eNB_task.h"
#  include "gtpv1u_eNB_defs.h"
#endif

extern int gtpv1u_new_data_req( uint8_t  enb_module_idP, rnti_t   ue_rntiP, uint8_t  rab_idP, uint8_t *buffer_pP, uint32_t buf_lenP, uint32_t buf_offsetP);

/* Prevent de-queueing the same PDCP SDU from the queue twice
 * by multiple threads. This has happened in TDD when thread-odd
 * is flushing a PDCP SDU after UE_RX() processing; whereas
 * thread-even is at a special-subframe, skips the UE_RX() process
 * and goes straight to the PDCP SDU flushing. The 2nd flushing
 * dequeues the same SDU again causing unexpected behavior.
 *
 * comment out the MACRO below to disable this protection
 */
#define PDCP_SDU_FLUSH_LOCK

#ifdef PDCP_SDU_FLUSH_LOCK
static pthread_mutex_t mtex = PTHREAD_MUTEX_INITIALIZER;
#endif

pdcp_data_req_header_t pdcp_read_header_g;

//-----------------------------------------------------------------------------
int pdcp_fifo_flush_sdus(const protocol_ctxt_t* const  ctxt_pP)
{
  //-----------------------------------------------------------------------------

//#if defined(PDCP_USE_NETLINK) && defined(LINUX)
  int ret = 0;
//#endif

#ifdef DEBUG_PDCP_FIFO_FLUSH_SDU
#define THREAD_NAME_LEN 16
  static char threadname[THREAD_NAME_LEN];
  ret = pthread_getname_np(pthread_self(), threadname, THREAD_NAME_LEN);
  if (ret != 0)
  {
   perror("pthread_getname_np : ");
   exit_fun("Error getting thread name");
  }
#undef THREAD_NAME_LEN
#endif

#ifdef PDCP_SDU_FLUSH_LOCK
  ret = pthread_mutex_trylock(&mtex);
  if (ret == EBUSY) {
#ifdef DEBUG_PDCP_FIFO_FLUSH_SDU
    LOG_W(PDCP, "[%s] at SFN/SF=%d/%d wait for PDCP FIFO to be unlocked\n",
        threadname, ctxt_pP->frame, ctxt_pP->subframe);
#endif
    if (pthread_mutex_lock(&mtex)) {
      exit_fun("PDCP_SDU_FLUSH_LOCK lock error!");
    }
#ifdef DEBUG_PDCP_FIFO_FLUSH_SDU
    LOG_I(PDCP, "[%s] at SFN/SF=%d/%d PDCP FIFO is unlocked\n",
        threadname, ctxt_pP->frame, ctxt_pP->subframe);
#endif
  } else if (ret != 0) {
    exit_fun("PDCP_SDU_FLUSH_LOCK trylock error!");
  }

#endif

  mem_block_t     *sdu_p            = list_get_head (&pdcp_sdu_list);
  int              bytes_wrote      = 0;
  int              pdcp_nb_sdu_sent = 0;
  uint8_t          cont             = 1;
#if defined(LINK_ENB_PDCP_TO_GTPV1U)
  //MessageDef      *message_p        = NULL;
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_FLUSH, 1 );
  while (sdu_p && cont) {

#ifdef DEBUG_PDCP_FIFO_FLUSH_SDU
    LOG_D(PDCP, "[%s] SFN/SF=%d/%d inst=%d size=%d\n",
        threadname, ctxt_pP->frame, ctxt_pP->subframe,
        ((pdcp_data_ind_header_t*) sdu_p->data)->inst,
        ((pdcp_data_ind_header_t *) sdu_p->data)->data_size);
#endif

#if defined(LINK_ENB_PDCP_TO_GTPV1U)

    if (ctxt_pP->enb_flag) {
      AssertFatal(0, "Now execution should not go here");
      LOG_D(PDCP,"Sending to GTPV1U %d bytes\n", ((pdcp_data_ind_header_t *)(sdu_p->data))->data_size);
      gtpv1u_new_data_req(
			  ctxt_pP->module_id, //gtpv1u_data_t *gtpv1u_data_p,
			  ctxt_pP->rnti,//rb_id/maxDRB, TO DO UE ID
			  ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id + 4,
			  &(((uint8_t *) sdu_p->data)[sizeof (pdcp_data_ind_header_t)]),
			  ((pdcp_data_ind_header_t *)(sdu_p->data))->data_size,
			  0);

      list_remove_head (&pdcp_sdu_list);
      free_mem_block (sdu_p, __func__);
      cont = 1;
      pdcp_nb_sdu_sent += 1;
      sdu_p = list_get_head (&pdcp_sdu_list);
      LOG_D(OTG,"After  GTPV1U\n");
      continue; // loop again
    }

#endif /* defined(ENABLE_USE_MME) */
#ifdef PDCP_DEBUG
    LOG_D(PDCP, "PDCP->IP TTI %d INST %d: Preparing %d Bytes of data from rab %d to Nas_mesh\n",
          ctxt_pP->frame, ((pdcp_data_ind_header_t *)(sdu_p->data))->inst,
          ((pdcp_data_ind_header_t *)(sdu_p->data))->data_size, ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id);
#endif //PDCP_DEBUG
    cont = 0;

    if (!pdcp_output_sdu_bytes_to_write) {
      if (!pdcp_output_header_bytes_to_write) {
        pdcp_output_header_bytes_to_write = sizeof (pdcp_data_ind_header_t);
      }

#ifdef PDCP_USE_RT_FIFO
      bytes_wrote = rtf_put (PDCP2PDCP_USE_RT_FIFO,
                             &(((uint8_t *) sdu->data)[sizeof (pdcp_data_ind_header_t) - pdcp_output_header_bytes_to_write]),
                             pdcp_output_header_bytes_to_write);

#else
#ifdef PDCP_USE_NETLINK
#ifdef LINUX
      memcpy(NLMSG_DATA(nas_nlh_tx), &(((uint8_t *) sdu_p->data)[sizeof (pdcp_data_ind_header_t) - pdcp_output_header_bytes_to_write]),
             pdcp_output_header_bytes_to_write);
      nas_nlh_tx->nlmsg_len = pdcp_output_header_bytes_to_write;
#endif //LINUX
#endif //PDCP_USE_NETLINK

      bytes_wrote = pdcp_output_header_bytes_to_write;
#endif //PDCP_USE_RT_FIFO

#ifdef PDCP_DEBUG
      LOG_D(PDCP, "Frame %d Sent %d Bytes of header to Nas_mesh\n",
            ctxt_pP->frame,
            bytes_wrote);
#endif //PDCP_DEBUG

      if (bytes_wrote > 0) {
        pdcp_output_header_bytes_to_write = pdcp_output_header_bytes_to_write - bytes_wrote;

        if (!pdcp_output_header_bytes_to_write) { // continue with sdu
          pdcp_output_sdu_bytes_to_write = ((pdcp_data_ind_header_t *) sdu_p->data)->data_size;
          AssertFatal(pdcp_output_sdu_bytes_to_write >= 0, "invalid data_size!");

#ifdef PDCP_USE_RT_FIFO
          bytes_wrote = rtf_put (PDCP2PDCP_USE_RT_FIFO, &(sdu->data[sizeof (pdcp_data_ind_header_t)]), pdcp_output_sdu_bytes_to_write);
#else

#ifdef PDCP_USE_NETLINK
#ifdef LINUX
          memcpy(NLMSG_DATA(nas_nlh_tx)+sizeof(pdcp_data_ind_header_t), &(sdu_p->data[sizeof (pdcp_data_ind_header_t)]), pdcp_output_sdu_bytes_to_write);
          nas_nlh_tx->nlmsg_len += pdcp_output_sdu_bytes_to_write;
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_UE_PDCP_FLUSH_SIZE, pdcp_output_sdu_bytes_to_write);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_FLUSH_BUFFER, 1 );
          ret = sendmsg(nas_sock_fd,&nas_msg_tx,0);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_FLUSH_BUFFER, 0 );
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_UE_PDCP_FLUSH_ERR, ret );

          if (ret<0) {
            LOG_E(PDCP, "[PDCP_FIFOS] sendmsg returns %d (errno: %d)\n", ret, errno);
      	    MSC_LOG_TX_MESSAGE_FAILED(
      	      (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
      	      (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
      	      NULL,
      	      0,
      	      MSC_AS_TIME_FMT" DATA-IND RNTI %"PRIx16" rb %u size %u",
      	      MSC_AS_TIME_ARGS(ctxt_pP),
      	      ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id,
      	      ((pdcp_data_ind_header_t *)(sdu_p->data))->data_size);
	    AssertFatal(1==0,"sendmsg failed for nas_sock_fd\n");
            break;
          } else {
        	  MSC_LOG_TX_MESSAGE(
        	    (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
        	    (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
        	    NULL,
        	    0,
        	    MSC_AS_TIME_FMT" DATA-IND RNTI %"PRIx16" rb %u size %u",
        	    MSC_AS_TIME_ARGS(ctxt_pP),
        	    ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id,
        	    ((pdcp_data_ind_header_t *)(sdu_p->data))->data_size);
          }

#endif // LINUX
#endif //PDCP_USE_NETLINK
          bytes_wrote= pdcp_output_sdu_bytes_to_write;
#endif // PDCP_USE_RT_FIFO

#ifdef PDCP_DEBUG
          LOG_D(PDCP, "PDCP->IP Frame %d INST %d: Sent %d Bytes of data from rab %d to higher layers\n",
                ctxt_pP->frame,
                ((pdcp_data_ind_header_t *)(sdu_p->data))->inst,
                bytes_wrote,
                ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id);
#endif //PDCP_DEBUG

          if (bytes_wrote > 0) {
            pdcp_output_sdu_bytes_to_write -= bytes_wrote;

            if (!pdcp_output_sdu_bytes_to_write) { // OK finish with this SDU
              // LOG_D(PDCP, "rb sent a sdu qos_sap %d\n", sapiP);
              LOG_D(PDCP,
                    "[FRAME %05d][xxx][PDCP][MOD xx/xx][RB %u][--- PDCP_DATA_IND / %d Bytes --->][IP][INSTANCE %u][RB %u]\n",
                    ctxt_pP->frame,
                    ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id,
                    ((pdcp_data_ind_header_t *)(sdu_p->data))->data_size,
                    ((pdcp_data_ind_header_t *)(sdu_p->data))->inst,
                    ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id);

              list_remove_head (&pdcp_sdu_list);
              free_mem_block (sdu_p, __func__);
              cont = 1;
              pdcp_nb_sdu_sent += 1;
              sdu_p = list_get_head (&pdcp_sdu_list);
            } else {
              LOG_D(PDCP, "1 skip free_mem_block: pdcp_output_sdu_bytes_to_write = %d\n", pdcp_output_sdu_bytes_to_write);
              AssertFatal(pdcp_output_sdu_bytes_to_write > 0, "pdcp_output_sdu_bytes_to_write cannot be negative!");
            }
          } else {
            LOG_W(PDCP, "2: RADIO->IP SEND SDU CONGESTION!\n");
          }
        } else {
          LOG_W(PDCP, "3: RADIO->IP SEND SDU CONGESTION!\n");
        }
      } else {
        LOG_D(PDCP, "4 skip free_mem_block: bytes_wrote = %d\n", bytes_wrote);
      }
    } else {
      // continue writing sdu
#ifdef PDCP_USE_RT_FIFO
      bytes_wrote = rtf_put (PDCP2PDCP_USE_RT_FIFO,
                             (uint8_t *) (&(sdu_p->data[sizeof (pdcp_data_ind_header_t) + ((pdcp_data_ind_header_t *) sdu_p->data)->data_size - pdcp_output_sdu_bytes_to_write])),
                             pdcp_output_sdu_bytes_to_write);
#else  // PDCP_USE_RT_FIFO
      bytes_wrote = pdcp_output_sdu_bytes_to_write;
#endif  // PDCP_USE_RT_FIFO

      if (bytes_wrote > 0) {
        pdcp_output_sdu_bytes_to_write -= bytes_wrote;

        if (!pdcp_output_sdu_bytes_to_write) {     // OK finish with this SDU
          //PRINT_RB_SEND_OUTPUT_SDU ("[PDCP] RADIO->IP SEND SDU\n");
          list_remove_head (&pdcp_sdu_list);
          free_mem_block (sdu_p, __func__);
          cont = 1;
          pdcp_nb_sdu_sent += 1;
          sdu_p = list_get_head (&pdcp_sdu_list);
          // LOG_D(PDCP, "rb sent a sdu from rab\n");
        } else {
          LOG_D(PDCP, "5 skip free_mem_block: pdcp_output_sdu_bytes_to_write = %d\n", pdcp_output_sdu_bytes_to_write);
        }
      } else {
        LOG_D(PDCP, "6 skip free_mem_block: bytes_wrote = %d\n", bytes_wrote);
      }
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_FLUSH, 0 );

#ifdef PDCP_USE_RT_FIFO

  if ((pdcp_nb_sdu_sent)) {
    if ((pdcp_2_nas_irq > 0)) {
#ifdef PDCP_DEBUG
      LOG_D(PDCP, "Frame %d : Trigger NAS RX interrupt\n",
            ctxt_pP->frame);
#endif //PDCP_DEBUG
      rt_pend_linux_srq (pdcp_2_nas_irq);

    } else {
      LOG_E(PDCP, "Frame %d: ERROR IF IP STACK WANTED : NOTIF PACKET(S) pdcp_2_nas_irq not initialized : %d\n",
            ctxt_pP->frame,
            pdcp_2_nas_irq);
    }
  }

#endif  //PDCP_USE_RT_FIFO

#ifdef PDCP_SDU_FLUSH_LOCK
  if (pthread_mutex_unlock(&mtex)) exit_fun("PDCP_SDU_FLUSH_LOCK unlock error!");
#endif

  return pdcp_nb_sdu_sent;
}

//-----------------------------------------------------------------------------
int pdcp_fifo_read_input_sdus (const protocol_ctxt_t* const  ctxt_pP)
{
#ifdef PDCP_USE_NETLINK
  protocol_ctxt_t                ctxt_cpy = *ctxt_pP;
  protocol_ctxt_t                ctxt;
  hash_key_t                     key       = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t                 h_rc;
  struct pdcp_netlink_element_s* data_p    = NULL;
  /* avoid gcc warnings */
  (void)data_p;
  module_id_t                    ue_id     = 0;
  pdcp_t*                        pdcp_p    = NULL;
# if defined(PDCP_USE_NETLINK_QUEUES)
  rb_id_t                        rab_id    = 0;

  pdcp_transmission_mode_t       pdcp_mode = PDCP_TRANSMISSION_MODE_UNKNOWN;


  while (pdcp_netlink_dequeue_element(ctxt_pP, &data_p) != 0) {
    DevAssert(data_p != NULL);
    rab_id = data_p->pdcp_read_header.rb_id % maxDRB;
    // ctxt_pP->rnti is NOT_A_RNTI
    ctxt_cpy.rnti = pdcp_module_id_to_rnti[ctxt_cpy.module_id][data_p->pdcp_read_header.inst];
    key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_cpy.rnti, ctxt_pP->enb_flag, rab_id, SRB_FLAG_NO);
    h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

    if (h_rc != HASH_TABLE_OK) {
      LOG_W(PDCP, PROTOCOL_CTXT_FMT" Dropped IP PACKET cause no PDCP instanciated\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP));
      free(data_p->data);
      free(data_p);
      data_p = NULL;
      continue;
    }

    CHECK_CTXT_ARGS(&ctxt_cpy);

    AssertFatal (rab_id    < maxDRB,                       "RB id is too high (%u/%d)!\n", rab_id, maxDRB);

    if (rab_id != 0) {
        LOG_D(PDCP, "[FRAME %05d][%s][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ "
            "/ %d Bytes --->][PDCP][MOD %u][RB %u]\n",
              ctxt_cpy.frame,
              (ctxt_cpy.enb_flag) ? "eNB" : "UE",
            data_p->pdcp_read_header.inst,
            data_p->pdcp_read_header.rb_id,
            data_p->pdcp_read_header.data_size,
            ctxt_cpy.module_id,
              rab_id);
#ifdef  OAI_NW_DRIVER_TYPE_ETHERNET

      if ((data_p->pdcp_read_header.traffic_type == TRAFFIC_IPV6_TYPE_MULTICAST) /*TRAFFIC_IPV6_TYPE_MULTICAST */ ||
          (data_p->pdcp_read_header.traffic_type == TRAFFIC_IPV4_TYPE_MULTICAST) /*TRAFFIC_IPV4_TYPE_MULTICAST */ ||
          (data_p->pdcp_read_header.traffic_type == TRAFFIC_IPV4_TYPE_BROADCAST) /*TRAFFIC_IPV4_TYPE_BROADCAST */ ) {
#if defined(Rel10) || defined(Rel14)
          PDCP_TRANSMISSION_MODE_TRANSPARENT;
#else
          pdcp_mode= PDCP_TRANSMISSION_MODE_DATA;
#endif
      } else if ((data_p->pdcp_read_header.traffic_type == TRAFFIC_IPV6_TYPE_UNICAST) /* TRAFFIC_IPV6_TYPE_UNICAST */ ||
                 (data_p->pdcp_read_header.traffic_type == TRAFFIC_IPV4_TYPE_UNICAST) /*TRAFFIC_IPV4_TYPE_UNICAST*/ ) {
          pdcp_mode=  PDCP_TRANSMISSION_MODE_DATA;
        } else {
          pdcp_mode= PDCP_TRANSMISSION_MODE_DATA;
          LOG_W(PDCP,"unknown IP traffic type \n");
        }

#else // OAI_NW_DRIVER_TYPE_ETHERNET NASMESH driver does not curreenlty support multicast traffic
        pdcp_mode = PDCP_TRANSMISSION_MODE_DATA;
#endif
      pdcp_data_req(&ctxt_cpy,
                      SRB_FLAG_NO,
                      rab_id % maxDRB,
                      RLC_MUI_UNDEFINED,
                      RLC_SDU_CONFIRM_NO,
                    data_p->pdcp_read_header.data_size,
                    data_p->data,
                      pdcp_mode);
    } else if (ctxt_cpy.enb_flag) {
      /* rb_id = 0, thus interpreated as broadcast and transported as
       * multiple unicast is a broadcast packet, we have to send this
       * packet on all default RABS of all connected UEs
       */
      LOG_D(PDCP, "eNB Try Forcing send on DEFAULT_RAB_ID first_ue_local %u nb_ue_local %u\n", oai_emulation.info.first_ue_local, oai_emulation.info.nb_ue_local);

      for (ue_id = 0; ue_id < NB_UE_INST; ue_id++) {
        if (pdcp_module_id_to_rnti[ctxt_cpy.module_id][ue_id] != NOT_A_RNTI) {
          LOG_D(PDCP, "eNB Try Forcing send on DEFAULT_RAB_ID UE %d\n", ue_id);
          ctxt.module_id     = ctxt_cpy.module_id;
          ctxt.rnti          = ctxt_cpy.pdcp_module_id_to_rnti[ctxt_cpy.module_id][ue_id];
          ctxt.frame         = ctxt_cpy.frame;
          ctxt.enb_flag      = ctxt_cpy.enb_flag;

          pdcp_data_req(
            &ctxt,
            SRB_FLAG_NO,
            DEFAULT_RAB_ID,
            RLC_MUI_UNDEFINED,
            RLC_SDU_CONFIRM_NO,
            data_p->pdcp_read_header.data_size,
            data_p->data,
            PDCP_TRANSMISSION_MODE_DATA);
        }
      }
    } else {
      LOG_D(PDCP, "Forcing send on DEFAULT_RAB_ID\n");
      pdcp_data_req(
        &ctxt_cpy,
        SRB_FLAG_NO,
        DEFAULT_RAB_ID,
        RLC_MUI_UNDEFINED,
        RLC_SDU_CONFIRM_NO,
        data_p->pdcp_read_header.data_size,
        data_p->data,
        PDCP_TRANSMISSION_MODE_DATA);
    }

    free(data_p->data);
    free(data_p);
    data_p = NULL;
  }

  return 0;
# else /* PDCP_USE_NETLINK_QUEUES*/
  int              len = 1;
  int  msg_len;
  rb_id_t          rab_id  = 0;
  int rlc_data_req_flag = 3;
 
  while ((len > 0) && (rlc_data_req_flag !=0))  {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ_BUFFER, 1 );
    len = recvmsg(nas_sock_fd, &nas_msg_rx, 0);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ_BUFFER, 0 );

    if (len<=0) {
      // nothing in pdcp NAS socket
      //LOG_D(PDCP, "[PDCP][NETLINK] Nothing in socket, length %d \n", len);
    } else {
    
      msg_len = len;
      for (nas_nlh_rx = (struct nlmsghdr *) nl_rx_buf;
           NLMSG_OK (nas_nlh_rx, msg_len);
           nas_nlh_rx = NLMSG_NEXT (nas_nlh_rx, msg_len)) {

        if (nas_nlh_rx->nlmsg_type == NLMSG_DONE) {
          LOG_D(PDCP, "[PDCP][NETLINK] RX NLMSG_DONE\n");
          //return;
        }

        if (nas_nlh_rx->nlmsg_type == NLMSG_ERROR) {
          LOG_D(PDCP, "[PDCP][NETLINK] RX NLMSG_ERROR\n");
        }

        if (pdcp_read_state_g == 0) {
          if (nas_nlh_rx->nlmsg_len == sizeof (pdcp_data_req_header_t) + sizeof(struct nlmsghdr)) {
            pdcp_read_state_g = 1;  //get
            memcpy((void *)&pdcp_read_header_g, (void *)NLMSG_DATA(nas_nlh_rx), sizeof(pdcp_data_req_header_t));
            LOG_D(PDCP, "[PDCP][NETLINK] RX pdcp_data_req_header_t inst %u, rb_id %u data_size %d\n",
                  pdcp_read_header_g.inst, pdcp_read_header_g.rb_id, pdcp_read_header_g.data_size);
          } else {
            LOG_E(PDCP, "[PDCP][NETLINK] WRONG size %d should be sizeof (pdcp_data_req_header_t) + sizeof(struct nlmsghdr)\n",
                  nas_nlh_rx->nlmsg_len);
          }
        } else {
          pdcp_read_state_g = 0;
          // print_active_requests()
#ifdef PDCP_DEBUG
          LOG_D(PDCP, "[PDCP][NETLINK] Something in socket, length %zu\n",
                nas_nlh_rx->nlmsg_len - sizeof(struct nlmsghdr));
#endif

          /* TODO: do we have to reset to 0 or not? not for a scenario with 1 UE at least */
//          pdcp_read_header_g.inst = 0;
//#warning "TO DO CORRCT VALUES FOR ue mod id, enb mod id"
          ctxt.frame         = ctxt_cpy.frame;
          ctxt.enb_flag      = ctxt_cpy.enb_flag;

#ifdef PDCP_DEBUG
          LOG_D(PDCP, "[PDCP][NETLINK] pdcp_read_header_g.rb_id = %d\n", pdcp_read_header_g.rb_id);
#endif

          if (ctxt_cpy.enb_flag) {
            ctxt.module_id = 0;
            rab_id      = pdcp_read_header_g.rb_id % maxDRB;
            ctxt.rnti          = pdcp_eNB_UE_instance_to_rnti[pdcp_read_header_g.rb_id / maxDRB];
          } else {
            ctxt.module_id = 0;
            rab_id      = pdcp_read_header_g.rb_id % maxDRB;
            ctxt.rnti          = pdcp_UE_UE_module_id_to_rnti[ctxt.module_id];
          }

          if (ctxt.enb_flag) {
            if (rab_id != 0) {
              rab_id = rab_id % maxDRB;
              key = PDCP_COLL_KEY_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
              h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

              if (h_rc == HASH_TABLE_OK) {
#ifdef PDCP_DEBUG
                LOG_D(PDCP, "[FRAME %5u][eNB][NETLINK][IP->PDCP] INST %d: Received socket with length %d (nlmsg_len = %zu) on Rab %d \n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      len,
                      nas_nlh_rx->nlmsg_len-sizeof(struct nlmsghdr),
                      pdcp_read_header_g.rb_id);
#endif

          	    MSC_LOG_RX_MESSAGE(
          	      (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                  (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
          	      NULL,
          	      0,
          	      MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
          	      MSC_AS_TIME_ARGS(ctxt_pP),
                  pdcp_read_header_g.inst,
          	      pdcp_read_header_g.rb_id,
          	      rab_id,
          	      pdcp_read_header_g.data_size);
                LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u]UE %u][RB %u]\n",
                      ctxt_cpy.frame,
                      pdcp_read_header_g.inst,
                      pdcp_read_header_g.rb_id,
                      pdcp_read_header_g.data_size,
                      ctxt.module_id,
                      ctxt.rnti,
                      rab_id);

                pdcp_data_req(&ctxt,
                              SRB_FLAG_NO,
                              rab_id,
                              RLC_MUI_UNDEFINED,
                              RLC_SDU_CONFIRM_NO,
                              pdcp_read_header_g.data_size,
                              (unsigned char *)NLMSG_DATA(nas_nlh_rx),
                              PDCP_TRANSMISSION_MODE_DATA);
              } else {
                LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %u][RB %u] NON INSTANCIATED INSTANCE, DROPPED\n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      pdcp_read_header_g.rb_id,
                      pdcp_read_header_g.data_size,
                      ctxt.module_id,
                      ctxt.rnti,
                      rab_id);
              }
            } else  { // rb_id =0, thus interpreated as broadcast and transported as multiple unicast
              // is a broadcast packet, we have to send this packet on all default RABS of all connected UEs
//#warning CODE TO BE REVIEWED, ONLY WORK FOR SIMPLE TOPOLOGY CASES
              for (ue_id = 0; ue_id < NB_UE_INST; ue_id++) {
                if (oai_emulation.info.eNB_ue_module_id_to_rnti[ctxt_cpy.module_id][ue_id] != NOT_A_RNTI) {
                  ctxt.rnti = oai_emulation.info.eNB_ue_module_id_to_rnti[ctxt_cpy.module_id][ue_id];
                  LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB DEFAULT_RAB_ID %u]\n",
                        ctxt.frame,
                        pdcp_read_header_g.inst,
                        pdcp_read_header_g.rb_id,
                        pdcp_read_header_g.data_size,
                        ctxt.module_id,
                        ctxt.rnti,
                        DEFAULT_RAB_ID);
                  pdcp_data_req (
                    &ctxt,
                    SRB_FLAG_NO,
                    DEFAULT_RAB_ID,
                    RLC_MUI_UNDEFINED,
                    RLC_SDU_CONFIRM_NO,
                    pdcp_read_header_g.data_size,
                    (unsigned char *)NLMSG_DATA(nas_nlh_rx),
                    PDCP_TRANSMISSION_MODE_DATA);
                }
              }
            }
          } else { // enb_flag
            if (rab_id != 0) {
              if (rab_id == UE_IP_DEFAULT_RAB_ID) {
                LOG_D(PDCP, "PDCP_COLL_KEY_DEFAULT_DRB_VALUE(module_id=%d, rnti=%x, enb_flag=%d)\n",
                    ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
                key = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
                h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);
              } else {
                rab_id = rab_id % maxDRB;
                LOG_D(PDCP, "PDCP_COLL_KEY_VALUE(module_id=%d, rnti=%x, enb_flag=%d, rab_id=%d, SRB_FLAG=%d)\n",
                    ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
                key = PDCP_COLL_KEY_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
                h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);
              }

              if (h_rc == HASH_TABLE_OK) {
                rab_id = pdcp_p->rb_id;
#ifdef PDCP_DEBUG
                LOG_D(PDCP, "[FRAME %5u][UE][NETLINK][IP->PDCP] INST %d: Received socket with length %d (nlmsg_len = %zu) on Rab %d \n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      len,
                      nas_nlh_rx->nlmsg_len-sizeof(struct nlmsghdr),
                      pdcp_read_header_g.rb_id);

                LOG_D(PDCP, "[FRAME %5u][UE][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB %u]\n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      pdcp_read_header_g.rb_id,
                      pdcp_read_header_g.data_size,
                      ctxt.module_id,
                      ctxt.rnti,
                      rab_id);
#endif
          	    MSC_LOG_RX_MESSAGE(
          	      (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                  (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
          	      NULL,
          	      0,
          	      MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
          	      MSC_AS_TIME_ARGS(ctxt_pP),
                  pdcp_read_header_g.inst,
          	      pdcp_read_header_g.rb_id,
          	      rab_id,
          	      pdcp_read_header_g.data_size);

                pdcp_data_req(
                  &ctxt,
                  SRB_FLAG_NO,
                  rab_id,
                  RLC_MUI_UNDEFINED,
                  RLC_SDU_CONFIRM_NO,
                  pdcp_read_header_g.data_size,
                  (unsigned char *)NLMSG_DATA(nas_nlh_rx),
                  PDCP_TRANSMISSION_MODE_DATA);
              } else {
            	MSC_LOG_RX_DISCARDED_MESSAGE(
                  (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                  (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
                  NULL,
                  0,
                  MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
                  MSC_AS_TIME_ARGS(ctxt_pP),
                  pdcp_read_header_g.inst,
                  pdcp_read_header_g.rb_id,
                  rab_id,
                  pdcp_read_header_g.data_size);
                LOG_D(PDCP,
                      "[FRAME %5u][UE][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %u][RB %u] NON INSTANCIATED INSTANCE key 0x%"PRIx64", DROPPED\n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      pdcp_read_header_g.rb_id,
                      pdcp_read_header_g.data_size,
                      ctxt.module_id,
                      ctxt.rnti,
                      rab_id,
                      key);
              }
            }  else {
              LOG_D(PDCP, "Forcing send on DEFAULT_RAB_ID\n");
              LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %u][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB DEFAULT_RAB_ID %u]\n",
                    ctxt.frame,
                    pdcp_read_header_g.inst,
                    pdcp_read_header_g.rb_id,
                    pdcp_read_header_g.data_size,
                    ctxt.module_id,
                    ctxt.rnti,
                    DEFAULT_RAB_ID);
          	  MSC_LOG_RX_MESSAGE(
                (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
                NULL,0,
                MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u default rab %u size %u",
                MSC_AS_TIME_ARGS(ctxt_pP),
                pdcp_read_header_g.inst,
                pdcp_read_header_g.rb_id,
                DEFAULT_RAB_ID,
                pdcp_read_header_g.data_size);

              pdcp_data_req (
                &ctxt,
                SRB_FLAG_NO,
                DEFAULT_RAB_ID,
                RLC_MUI_UNDEFINED,
                RLC_SDU_CONFIRM_NO,
                pdcp_read_header_g.data_size,
                (unsigned char *)NLMSG_DATA(nas_nlh_rx),
                PDCP_TRANSMISSION_MODE_DATA);
            }
          }

        }
      }
    }
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ, 0 );
  }

  return len;
# endif
#else // neither PDCP_USE_NETLINK nor PDCP_USE_RT_FIFO
  return 0;
#endif // PDCP_USE_NETLINK
}


void pdcp_fifo_read_input_sdus_from_otg (const protocol_ctxt_t* const  ctxt_pP) {


  module_id_t          dst_id; // dst for otg
  protocol_ctxt_t      ctxt;
  // we need to add conditions to avoid transmitting data when the UE is not RRC connected.
  if ((otg_enabled==1) && (ctxt_pP->enb_flag == ENB_FLAG_YES)) { // generate DL traffic




    PROTOCOL_CTXT_SET_BY_MODULE_ID(
      &ctxt,
      ctxt_pP->module_id,
      ctxt_pP->enb_flag,
      NOT_A_RNTI,
      ctxt_pP->frame,
      ctxt_pP->subframe,
      ctxt_pP->module_id);

    for (dst_id = 0; dst_id<NUMBER_OF_UE_MAX; dst_id++) {
      ctxt.rnti = oai_emulation.info.eNB_ue_module_id_to_rnti[ctxt.module_id][dst_id];

    }
  }
}
