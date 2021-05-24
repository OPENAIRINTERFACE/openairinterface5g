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



extern int otg_enabled;

#include "pdcp.h"
#include "pdcp_primitives.h"

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/ip.h>
#define rtf_put write
#define rtf_get read

#include "../MAC/mac_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "NETWORK_DRIVER/LITE/constant.h"
//#include "SIMULATION/ETH_TRANSPORT/extern.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OCG/OCG_extern.h"
#include "common/utils/LOG/log.h"
#include "UTIL/OTG/otg_tx.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "platform_constants.h"
#include "msc.h"
#include "pdcp.h"

#include "assertions.h"


#include <sys/socket.h>
#include <linux/netlink.h>
#include "NETWORK_DRIVER/UE_IP/constant.h"

extern char nl_rx_buf[NL_MAX_PAYLOAD];
extern struct sockaddr_nl nas_src_addr, nas_dest_addr;
extern struct nlmsghdr *nas_nlh_tx;
extern struct nlmsghdr *nas_nlh_rx;
extern struct iovec nas_iov_tx;
extern struct iovec nas_iov_rx;

extern int nas_sock_fd[MAX_MOBILES_PER_ENB];

extern int nas_sock_mbms_fd;

extern struct msghdr nas_msg_tx;
extern struct msghdr nas_msg_rx;



#  include "gtpv1u_eNB_task.h"
#  include "gtpv1u_eNB_defs.h"


extern int gtpv1u_new_data_req( uint8_t  enb_module_idP, rnti_t   ue_rntiP, uint8_t  rab_idP, uint8_t *buffer_pP, uint32_t buf_lenP, uint32_t buf_offsetP);


void debug_pdcp_pc5s_sdu(sidelink_pc5s_element *sl_pc5s_msg, char *title) {
  LOG_I(PDCP,"%s: \nPC5S message, header traffic_type: %d)\n", title, sl_pc5s_msg->pc5s_header.traffic_type);
  LOG_I(PDCP,"PC5S message, header rb_id: %ld)\n", sl_pc5s_msg->pc5s_header.rb_id);
  LOG_I(PDCP,"PC5S message, header data_size: %d)\n", sl_pc5s_msg->pc5s_header.data_size);
  LOG_I(PDCP,"PC5S message, header inst: %d)\n", sl_pc5s_msg->pc5s_header.inst);
  LOG_I(PDCP,"PC5-S message, sourceL2Id: 0x%08x\n)\n", sl_pc5s_msg->pc5s_header.sourceL2Id);
  LOG_I(PDCP,"PC5-S message, destinationL1Id: 0x%08x\n)\n", sl_pc5s_msg->pc5s_header.destinationL2Id);
}
//-----------------------------------------------------------------------------
int pdcp_fifo_flush_sdus(const protocol_ctxt_t *const  ctxt_pP) {
  notifiedFIFO_elt_t  *sdu_p;
  int              pdcp_nb_sdu_sent = 0;
  int              ret=0;
  while ((sdu_p = pollNotifiedFIFO(&pdcp_sdu_list)) != NULL ) {
    pdcp_data_ind_header_t * pdcpHead=(pdcp_data_ind_header_t *)NotifiedFifoData(sdu_p);

    /* Note: we ignore the instance ID in the context and use the one in the
     * PDCP packet to pick the right socket below */

    int rb_id = pdcpHead->rb_id;
    void * pdcpData=(void*)(pdcpHead+1);
    if (rb_id == 10) { //hardcoded for PC5-Signaling
      if( LOG_DEBUGFLAG(DEBUG_PDCP) ) {
        debug_pdcp_pc5s_sdu((sidelink_pc5s_element *)pdcpData,
                            "pdcp_fifo_flush_sdus sends a aPC5S message");
      }

      ret = sendto(pdcp_pc5_sockfd, pdcpData,
                   sizeof(sidelink_pc5s_element), 0, (struct sockaddr *)&prose_pdcp_addr,sizeof(prose_pdcp_addr) );

    } else if (UE_NAS_USE_TUN) {
      //ret = write(nas_sock_fd[pdcpHead->inst], &(sdu_p->data[sizeof(pdcp_data_ind_header_t)]),sizeToWrite );
       if(rb_id == mbms_rab_id){
       ret = write(nas_sock_mbms_fd, pdcpData, pdcpHead->data_size );
       LOG_I(PDCP,"[PDCP_FIFOS] ret %d TRIED TO PUSH MBMS DATA TO rb_id %d handle %d sizeToWrite %d\n",
	     ret,rb_id,nas_sock_fd[pdcpHead->inst],pdcpHead->data_size);
        }
       else
       {
	 if( LOG_DEBUGFLAG(DEBUG_PDCP) ) 
	   log_dump(PDCP, pdcpData, pdcpHead->data_size, LOG_DUMP_CHAR,"PDCP output to be sent to TUN interface: \n");
	 ret = write(nas_sock_fd[pdcpHead->inst], pdcpData,pdcpHead->data_size );
	 LOG_T(PDCP,"[UE PDCP_FIFOS] ret %d TRIED TO PUSH DATA TO rb_id %d handle %d sizeToWrite %d\n",
	       ret,rb_id,nas_sock_fd[pdcpHead->inst],pdcpHead->data_size);
       }
    } else if (ENB_NAS_USE_TUN) {
      if( LOG_DEBUGFLAG(DEBUG_PDCP) ) 
	log_dump(PDCP, pdcpData, pdcpHead->data_size, LOG_DUMP_CHAR,"PDCP output to be sent to TUN interface: \n");
      ret = write(nas_sock_fd[0], pdcpData, pdcpHead->data_size);
       LOG_T(PDCP,"[NB PDCP_FIFOS] ret %d TRIED TO PUSH DATA TO rb_id %d handle %d sizeToWrite %d\n",ret,rb_id,nas_sock_fd[0],pdcpHead->data_size);
    } else if (PDCP_USE_NETLINK) {
      int sizeToWrite= sizeof (pdcp_data_ind_header_t) + pdcpHead->data_size;
      memcpy(NLMSG_DATA(nas_nlh_tx), (uint8_t *) pdcpHead,  sizeToWrite);
      nas_nlh_tx->nlmsg_len = sizeToWrite;
      ret = sendmsg(nas_sock_fd[0],&nas_msg_tx,0);
    }  //  PDCP_USE_NETLINK
    
    AssertFatal(ret >= 0,"[PDCP_FIFOS] pdcp_fifo_flush_sdus (errno: %d %s), nas_sock_fd[0]: %d\n", errno, strerror(errno), nas_sock_fd[0]);

    if( LOG_DEBUGFLAG(DEBUG_PDCP) )
      log_dump(PDCP, pdcpData, min(pdcpHead->data_size,30) , LOG_DUMP_CHAR,
	       "Printing first bytes of PDCP SDU before removing it from the list: \n");
    delNotifiedFIFO_elt (sdu_p);
    pdcp_nb_sdu_sent ++;
  }

  return pdcp_nb_sdu_sent;
}

int pdcp_fifo_flush_mbms_sdus(const protocol_ctxt_t *const  ctxt_pP) {
  notifiedFIFO_elt_t     *sdu_p;
  int              pdcp_nb_sdu_sent = 0;
  //int              ret=0;

  while ((sdu_p = pollNotifiedFIFO(&pdcp_sdu_list)) != NULL ) {
    pdcp_data_ind_header_t * pdcpHead=(pdcp_data_ind_header_t *)NotifiedFifoData(sdu_p);
    AssertFatal(pdcpHead->inst==ctxt_pP->module_id, "To implement correctly multi module id\n");
    //int rb_id = ((pdcp_data_ind_header_t *)(sdu_p->data))->rb_id;
    //int sizeToWrite= sizeof (pdcp_data_ind_header_t) +
                     //((pdcp_data_ind_header_t *) sdu_p->data)->data_size;

    //if (rb_id == 10) { //hardcoded for PC5-Signaling
    //  if( LOG_DEBUGFLAG(DEBUG_PDCP) ) {
    //    debug_pdcp_pc5s_sdu((sidelink_pc5s_element *)&(sdu_p->data[sizeof(pdcp_data_ind_header_t)]),
    //                        "pdcp_fifo_flush_sdus sends a aPC5S message");
    //  }

    //  ret = sendto(pdcp_pc5_sockfd, &(sdu_p->data[sizeof(pdcp_data_ind_header_t)]),
    //               sizeof(sidelink_pc5s_element), 0, (struct sockaddr *)&prose_pdcp_addr,sizeof(prose_pdcp_addr) );
    //} else if (UE_NAS_USE_TUN) {
    //  ret = write(nas_sock_fd[ctxt_pP->module_id], &(sdu_p->data[sizeof(pdcp_data_ind_header_t)]),sizeToWrite );
    //  LOG_I(PDCP,"[PDCP_FIFOS] ret %d TRIED TO PUSH DATA TO rb_id %d handle %d sizeToWrite %d\n",ret,rb_id,nas_sock_fd[ctxt_pP->module_id],sizeToWrite);

    //} else if (ENB_NAS_USE_TUN) {
    //  ret = write(nas_sock_fd[0], &(sdu_p->data[sizeof(pdcp_data_ind_header_t)]),sizeToWrite );
    //} else if (PDCP_USE_NETLINK) {
    //  memcpy(NLMSG_DATA(nas_nlh_tx), (uint8_t *) sdu_p->data,  sizeToWrite);
    //  nas_nlh_tx->nlmsg_len = sizeToWrite;
    //  ret = sendmsg(nas_sock_fd[0],&nas_msg_tx,0);
    //}  //  PDCP_USE_NETLINK

    //AssertFatal(ret >= 0,"[PDCP_FIFOS] pdcp_fifo_flush_sdus (errno: %d %s)\n", errno, strerror(errno));

    //AssertFatal(ret >= 0,"[PDCP_FIFOS] pdcp_fifo_flush_sdus (errno: %d %s)\n", errno, strerror(errno));
    delNotifiedFIFO_elt (sdu_p);
    pdcp_nb_sdu_sent ++;
  }

  return pdcp_nb_sdu_sent;
}


int pdcp_fifo_read_input_sdus_fromtun (const protocol_ctxt_t *const  ctxt_pP) {
  protocol_ctxt_t ctxt = *ctxt_pP;
  hash_key_t key = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t h_rc;
  pdcp_t *pdcp_p = NULL;
  int len;
  rb_id_t rab_id = DEFAULT_RAB_ID;

  do {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ_BUFFER, 1 );
    len = read(UE_NAS_USE_TUN?nas_sock_fd[ctxt_pP->module_id]:nas_sock_fd[0], &nl_rx_buf, NL_MAX_PAYLOAD);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ_BUFFER, 0 );

    if (len<=0) continue;

    if (UE_NAS_USE_TUN) {
      key = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
      h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
    } else { // => ENB_NAS_USE_TUN
      /* Get the IP from a packet */
      struct ip *ip_pack = (struct ip *) nl_rx_buf;
      /* Use last octet of destination IP to get index of UE */
      int ue_indx = ((ip_pack->ip_dst.s_addr >> 24) -  2) % MAX_MOBILES_PER_ENB;
      ctxt.rnti=pdcp_eNB_UE_instance_to_rnti[ue_indx];
      ctxt.enb_flag=ENB_FLAG_YES;
      ctxt.module_id=0;
      key = PDCP_COLL_KEY_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
      h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
    }

    LOG_D(PDCP, "PDCP_COLL_KEY_DEFAULT_DRB_VALUE(module_id=%d, rnti=%x, enb_flag=%d)\n",
          ctxt.module_id, ctxt.rnti, ctxt.enb_flag);

    if (h_rc == HASH_TABLE_OK) {
      LOG_D(PDCP, "[FRAME %5u][UE][NETLINK][IP->PDCP] INST %ld: Received socket with length %d on Rab %ld \n",
            ctxt.frame, ctxt.instance, len, rab_id);
      LOG_D(PDCP, "[FRAME %5u][UE][IP][INSTANCE %ld][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %04x][RB %ld]\n",
            ctxt.frame, ctxt.instance, rab_id, len, ctxt.module_id,
            ctxt.rnti, rab_id);

#if defined  ENABLE_PDCP_PAYLOAD_DEBUG
      LOG_I(PHY, "TUN interface output received from PDCP: \n");
      for (int i = 0; i < 128; i++) {
    	  printf("%02x ",(unsigned char)nl_rx_buf[i]);
      }
      printf("\n");
#endif

      pdcp_data_req(&ctxt, SRB_FLAG_NO, rab_id, RLC_MUI_UNDEFINED,
                    RLC_SDU_CONFIRM_NO, len, (unsigned char *)nl_rx_buf,
                    PDCP_TRANSMISSION_MODE_DATA
                    , NULL, NULL
                   );
    } else {
      LOG_D(PDCP,
            "[FRAME %5u][UE][IP][INSTANCE %ld][RB %ld][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %04x][RB %ld] TUN NON INSTANCIATED INSTANCE key 0x%"PRIx64", DROPPED\n",
            ctxt.frame, ctxt.instance, rab_id, len, ctxt.module_id,
            ctxt.rnti, rab_id, key);
    }
  } while (len > 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ, 0 );
  return len;
}

int pdcp_fifo_read_input_mbms_sdus_fromtun (const protocol_ctxt_t *const  ctxt_pP) {
  protocol_ctxt_t ctxt = *ctxt_pP;
  hash_key_t key = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t h_rc = HASH_TABLE_OK;
  //pdcp_t *pdcp_p = NULL;
  int len;
  rb_id_t rab_id = mbms_rab_id;//DEFAULT_RAB_ID;
  if(mbms_rab_id > 9 || mbms_rab_id < 4)
       h_rc = 2;

  if(UE_NAS_USE_TUN)
       return 0;

  do {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_MBMS_FIFO_READ, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_MBMS_FIFO_READ_BUFFER, 1 );
    len = read(nas_sock_mbms_fd, &nl_rx_buf, NL_MAX_PAYLOAD);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_MBMS_FIFO_READ_BUFFER, 0 );

    if (len<=0) continue;

    if (UE_NAS_USE_TUN) {
      //key = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
      //h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
    } else { // => ENB_NAS_USE_TUN
    //ctxt.rnti=pdcp_eNB_UE_instance_to_rnti[0];
     // ctxt.enb_flag=ENB_FLAG_YES;
     // ctxt.module_id=0;
     // key = PDCP_COLL_KEY_VALUE(ctxt.module_id, /*ctxt.rnti*/0, ctxt.enb_flag, /*mbms_rab_id*/8, SRB_FLAG_YES);
     // h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
     // LOG_W(PDCP,"h_rc %d %d\n",h_rc,rab_id);
    }

    LOG_D(PDCP, "PDCP_COLL_KEY_DEFAULT_DRB_VALUE(module_id=%d, rnti=%x, enb_flag=%d)\n",
          ctxt.module_id, ctxt.rnti, ctxt.enb_flag);

    if (h_rc == HASH_TABLE_OK) {
      LOG_D(PDCP, "[FRAME %5u][UE][NETLINK][IP->PDCP] INST %ld: Received socket with length %d on Rab %ld \n",
            ctxt.frame, ctxt.instance, len, rab_id);
      LOG_D(PDCP, "[FRAME %5u][UE][IP][INSTANCE %ld][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %04x][RB %ld]\n",
            ctxt.frame, ctxt.instance, rab_id, len, ctxt.module_id,
            ctxt.rnti, rab_id);
      MSC_LOG_RX_MESSAGE((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                         (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
                         NULL, 0,
                         MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
                         MSC_AS_TIME_ARGS(ctxt_pP),
                         ctxt.instance, rab_id, rab_id, len);
      pdcp_data_req(
                &ctxt,
                SRB_FLAG_NO,
                //DEFAULT_RAB_ID,
               rab_id,
                RLC_MUI_UNDEFINED,
                RLC_SDU_CONFIRM_NO,
                len,
                (unsigned char *)nl_rx_buf,
                    PDCP_TRANSMISSION_MODE_TRANSPARENT
                    , NULL, NULL
                );

      //pdcp_data_req(&ctxt, SRB_FLAG_NO, rab_id, RLC_MUI_UNDEFINED,
      //              RLC_SDU_CONFIRM_NO, len, (unsigned char *)nl_rx_buf,
      //              PDCP_TRANSMISSION_MODE_DATA
      //              , NULL, NULL
      //             );
    } else {
      MSC_LOG_RX_DISCARDED_MESSAGE(
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
        NULL,
        0,
        MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        ctxt.instance, rab_id, rab_id, len);
      LOG_D(PDCP,
            "[FRAME %5u][UE][IP][INSTANCE %ld][RB %ld][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %04x][RB %ld] NON INSTANCIATED INSTANCE key 0x%"PRIx64", DROPPED\n",
            ctxt.frame, ctxt.instance, rab_id, len, ctxt.module_id,
            ctxt.rnti, rab_id, key);
       //if (!UE_NAS_USE_TUN) {
       //    pdcp_data_req(
        //        &ctxt,
        //        SRB_FLAG_NO,
        //        DEFAULT_RAB_ID,
        //        RLC_MUI_UNDEFINED,
        //        RLC_SDU_CONFIRM_NO,
        //        len,
        //        (unsigned char *)nl_rx_buf,
        //            PDCP_TRANSMISSION_MODE_TRANSPARENT
        //            , NULL, NULL
        //        );
        //}
    }
  } while (len > 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_MBMS_FIFO_READ, 0 );
  return len;
}

int pdcp_fifo_read_input_sdus_fromnetlinksock (const protocol_ctxt_t *const  ctxt_pP) {
  int  len = 1;
  int rlc_data_req_flag = 3;

  while ((len > 0) && (rlc_data_req_flag !=0))  {
    pdcp_data_req_header_t pdcp_read_header_g;
    protocol_ctxt_t                ctxt_cpy = *ctxt_pP;
    protocol_ctxt_t                ctxt;
    hash_key_t                     key       = HASHTABLE_NOT_A_KEY_VALUE;
    hashtable_rc_t                 h_rc;
    pdcp_t                        *pdcp_p    = NULL;
    static unsigned char pdcp_read_state_g =0;
    rb_id_t          rab_id  = 0;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ_BUFFER, 1 );
    len = recvmsg(nas_sock_fd[0], &nas_msg_rx, 0);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ_BUFFER, 0 );

    if (len > 0) {
      for (nas_nlh_rx = (struct nlmsghdr *) nl_rx_buf;
           NLMSG_OK (nas_nlh_rx, len);
           nas_nlh_rx = NLMSG_NEXT (nas_nlh_rx, len)) {
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
            LOG_D(PDCP, "[PDCP][NETLINK] RX pdcp_data_req_header_t inst %u, rb_id %ld data_size %d, source L2Id 0x%08x, destination L2Id 0x%08x\n",
                  pdcp_read_header_g.inst, pdcp_read_header_g.rb_id, pdcp_read_header_g.data_size,pdcp_read_header_g.sourceL2Id, pdcp_read_header_g.destinationL2Id );
          } else {
            LOG_E(PDCP, "[PDCP][NETLINK] WRONG size %d should be sizeof (pdcp_data_req_header_t) + sizeof(struct nlmsghdr)\n",
                  nas_nlh_rx->nlmsg_len);
          }
        } else {
          pdcp_read_state_g = 0;
          // print_active_requests()
          LOG_D(PDCP, "[PDCP][NETLINK] Something in socket, length %zu\n",
                nas_nlh_rx->nlmsg_len - sizeof(struct nlmsghdr));
          /* TODO: do we have to reset to 0 or not? not for a scenario with 1 UE at least */
          //          pdcp_read_header_g.inst = 0;
          //#warning "TO DO CORRCT VALUES FOR ue mod id, enb mod id"
          ctxt.frame         = ctxt_cpy.frame;
          ctxt.enb_flag      = ctxt_cpy.enb_flag;
          LOG_D(PDCP, "[PDCP][NETLINK] pdcp_read_header_g.rb_id = %ld, source L2Id = 0x%08x, destination L2Id = 0x%08x \n", pdcp_read_header_g.rb_id, pdcp_read_header_g.sourceL2Id,
                pdcp_read_header_g.destinationL2Id);

          if (ctxt.enb_flag) {
            ctxt.module_id = 0;
            rab_id      = pdcp_read_header_g.rb_id % LTE_maxDRB;
            ctxt.rnti          = pdcp_eNB_UE_instance_to_rnti[pdcp_read_header_g.rb_id / LTE_maxDRB];

            if (rab_id != 0) {
              rab_id = rab_id % LTE_maxDRB;
              key = PDCP_COLL_KEY_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
              h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);

              if (h_rc == HASH_TABLE_OK) {
                LOG_D(PDCP, "[FRAME %5u][eNB][NETLINK][IP->PDCP] INST %d: Received socket with length %d (nlmsg_len = %zu) on Rab %ld for rnti: %d \n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      len,
                      nas_nlh_rx->nlmsg_len-sizeof(struct nlmsghdr),
                      pdcp_read_header_g.rb_id,
                      ctxt.rnti);
                LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u]UE %u][RB %ld]\n",
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
                              PDCP_TRANSMISSION_MODE_DATA
                              ,NULL, NULL
                             );
              } else {
                LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %u][RB %ld] NON INSTANCIATED INSTANCE, DROPPED\n",
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
              // never finished code, dropped
            }
          } else { // ctxt.enb_flag => UE
            if (NFAPI_MODE == NFAPI_UE_STUB_PNF) {
              ctxt.module_id = pdcp_read_header_g.inst;
            } else {
              ctxt.module_id = 0;
            }

            rab_id      = pdcp_read_header_g.rb_id % LTE_maxDRB;
            ctxt.rnti          = pdcp_UE_UE_module_id_to_rnti[ctxt.module_id];

            if (rab_id != 0) {
              if (rab_id == UE_IP_DEFAULT_RAB_ID) {
                LOG_D(PDCP, "PDCP_COLL_KEY_DEFAULT_DRB_VALUE(module_id=%d, rnti=%x, enb_flag=%d)\n",
                      ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
                key = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
                h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
                LOG_D(PDCP,"request key %x : (%d,%x,%d,%ld)\n",
                      (uint8_t)key,ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id);
              } else {
                rab_id = rab_id % LTE_maxDRB;
                LOG_D(PDCP, "PDCP_COLL_KEY_VALUE(module_id=%d, rnti=%x, enb_flag=%d, rab_id=%ld, SRB_FLAG=%d)\n",
                      ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
                key = PDCP_COLL_KEY_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
                h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
                LOG_D(PDCP,"request key %x : (%d,%x,%d,%ld)\n",
                      (uint8_t)key,ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id);
              }

              if (h_rc == HASH_TABLE_OK) {
                rab_id = pdcp_p->rb_id;
                LOG_D(PDCP, "[FRAME %5u][UE][NETLINK][IP->PDCP] INST %d: Received socket with length %d (nlmsg_len = %zu) on Rab %ld \n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      len,
                      nas_nlh_rx->nlmsg_len-sizeof(struct nlmsghdr),
                      pdcp_read_header_g.rb_id);
                LOG_D(PDCP, "[FRAME %5u][UE][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB %ld]\n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      pdcp_read_header_g.rb_id,
                      pdcp_read_header_g.data_size,
                      ctxt.module_id,
                      ctxt.rnti,
                      rab_id);
                pdcp_data_req(
                  &ctxt,
                  SRB_FLAG_NO,
                  rab_id,
                  RLC_MUI_UNDEFINED,
                  RLC_SDU_CONFIRM_NO,
                  pdcp_read_header_g.data_size,
                  (unsigned char *)NLMSG_DATA(nas_nlh_rx),
                  PDCP_TRANSMISSION_MODE_DATA,
                  (NFAPI_MODE == NFAPI_UE_STUB_PNF)?NULL:&pdcp_read_header_g.sourceL2Id,
                  (NFAPI_MODE == NFAPI_UE_STUB_PNF)?NULL:&pdcp_read_header_g.destinationL2Id
                );
              } else { /* else of h_rc == HASH_TABLE_OK */
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
                      "[FRAME %5u][UE][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %u][RB %ld] NON INSTANCIATED INSTANCE key 0x%"PRIx64", DROPPED\n",
                      ctxt.frame,
                      pdcp_read_header_g.inst,
                      pdcp_read_header_g.rb_id,
                      pdcp_read_header_g.data_size,
                      ctxt.module_id,
                      ctxt.rnti,
                      rab_id,
                      key);
              } /* h_rc != HASH_TABLE_OK */
            }  else {/* else of rab_id != 0 */
              LOG_D(PDCP, "Forcing send on DEFAULT_RAB_ID\n");
              LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB DEFAULT_RAB_ID %u]\n",
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
                PDCP_TRANSMISSION_MODE_DATA,
                (NFAPI_MODE == NFAPI_UE_STUB_PNF) ? NULL :&pdcp_read_header_g.sourceL2Id,
                (NFAPI_MODE == NFAPI_UE_STUB_PNF) ? NULL :&pdcp_read_header_g.destinationL2Id
              );
            } /* rab_id == 0 */
          } /*pdcp_read_state_g != 0 */
        } /* UE */
      } /* for loop on netlink buffers */

      return len;
    } /* len > 0 */

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_FIFO_READ, 0 );
  } /* while loop on reading from netlink socket */

  return 0;
} /* pdcp_fifo_read_input_sdus_fromnetlinksock */

void pdcp_fifo_read_input_sdus_frompc5s (const protocol_ctxt_t *const  ctxt_pP) {
  protocol_ctxt_t                ctxt_cpy = *ctxt_pP;
  protocol_ctxt_t                ctxt;
  hash_key_t                     key       = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t                 h_rc;
  struct pdcp_netlink_element_s *data_p    = NULL;
  /* avoid gcc warnings */
  (void)data_p;
  pdcp_t                        *pdcp_p    = NULL;
  //TTN for D2D (PC5S)
  int prose_addr_len = sizeof(prose_pdcp_addr);
  char send_buf[BUFSIZE], receive_buf[BUFSIZE];
  //int optval;
  int bytes_received;
  sidelink_pc5s_element *sl_pc5s_msg_send = NULL;
  pc5s_header_t *pc5s_header = NULL;
  rb_id_t          rab_id  = 0;
  //TTN for D2D (PC5S)
  // receive a message from ProSe App
  memset(receive_buf, 0, BUFSIZE);
  bytes_received = recvfrom(pdcp_pc5_sockfd, receive_buf, BUFSIZE, 0,
                            (struct sockaddr *) &prose_pdcp_addr, (socklen_t *)&prose_addr_len);

  if (bytes_received > 0) {
    pc5s_header = calloc(1, sizeof(pc5s_header_t));
    memcpy((void *)pc5s_header, (void *)receive_buf, sizeof(pc5s_header_t));

    switch(pc5s_header->traffic_type) {
      case TRAFFIC_PC5S_SESSION_INIT :
        //send reply to ProSe app
        LOG_D(PDCP,"Received a request to open PDCP socket and establish a new PDCP session ... send response to ProSe App \n");
        memset(send_buf, 0, BUFSIZE);
        sl_pc5s_msg_send = calloc(1, sizeof(sidelink_pc5s_element));
        sl_pc5s_msg_send->pc5s_header.traffic_type = TRAFFIC_PC5S_SESSION_INIT;
        sl_pc5s_msg_send->pc5sPrimitive.status = 1;
        memcpy((void *)send_buf, (void *)sl_pc5s_msg_send, sizeof(sidelink_pc5s_element));
        int prose_addr_len = sizeof(prose_pdcp_addr);
        int bytes_sent = sendto(pdcp_pc5_sockfd, (char *)send_buf, sizeof(sidelink_pc5s_element), 0, (struct sockaddr *)&prose_pdcp_addr, prose_addr_len);
        free (sl_pc5s_msg_send);

        if (bytes_sent < 0) {
          LOG_E(PDCP, "ERROR: Failed to send to ProSe App\n");
          exit(EXIT_FAILURE);
        }

        break;

      case TRAFFIC_PC5S_SIGNALLING:  /* PC5-S message -> send to other UE */
        if( LOG_DEBUGFLAG(DEBUG_PDCP) ) {
          debug_pdcp_pc5s_sdu((sidelink_pc5s_element *)send_buf,
                              "pdcp_fifo_read_input_sdus received aPC5S message");
        }

        /* TODO: do we have to reset to 0 or not? not for a scenario with 1 UE at least */
        //          pc5s_header.inst = 0;
        //#warning "TO DO CORRCT VALUES FOR ue mod id, enb mod id"
        ctxt.frame         = ctxt_cpy.frame;
        ctxt.enb_flag      = ctxt_cpy.enb_flag;
        LOG_I(PDCP, "[PDCP] pc5s_header->rb_id = %ld\n", pc5s_header->rb_id);

        if (ctxt_cpy.enb_flag) {
          ctxt.module_id = 0;
          rab_id      = pc5s_header->rb_id % LTE_maxDRB;
          ctxt.rnti          = pdcp_eNB_UE_instance_to_rnti[pdcp_eNB_UE_instance_to_rnti_index];
        } else {
          ctxt.module_id = 0;
          rab_id      = pc5s_header->rb_id % LTE_maxDRB;
          ctxt.rnti          = pdcp_UE_UE_module_id_to_rnti[ctxt.module_id];
        }

        //UE
        if (!ctxt.enb_flag) {
          if (rab_id != 0) {
            if (rab_id == UE_IP_DEFAULT_RAB_ID) {
              LOG_D(PDCP, "PDCP_COLL_KEY_DEFAULT_DRB_VALUE(module_id=%d, rnti=%x, enb_flag=%d)\n",
                    ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
              key = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag);
              h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
              LOG_D(PDCP,"request key %x : (%d,%x,%d,%ld)\n",
                    (uint8_t)key,ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id);
            } else {
              rab_id = rab_id % LTE_maxDRB;
              LOG_I(PDCP, "PDCP_COLL_KEY_VALUE(module_id=%d, rnti=%x, enb_flag=%d, rab_id=%ld, SRB_FLAG=%d)\n",
                    ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
              key = PDCP_COLL_KEY_VALUE(ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id, SRB_FLAG_NO);
              h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);
              LOG_I(PDCP,"request key %x : (%d,%x,%d,%ld)\n",
                    (uint8_t)key,ctxt.module_id, ctxt.rnti, ctxt.enb_flag, rab_id);
            }

            if (h_rc == HASH_TABLE_OK) {
              rab_id = pdcp_p->rb_id;
              LOG_I(PDCP, "[FRAME %5u][UE][NETLINK][IP->PDCP] INST %d: Received socket with length %d  on Rab %ld \n",
                    ctxt.frame,
                    pc5s_header->inst,
                    bytes_received,
                    pc5s_header->rb_id);
              LOG_I(PDCP, "[FRAME %5u][UE][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB %ld]\n",
                    ctxt.frame,
                    pc5s_header->inst,
                    pc5s_header->rb_id,
                    pc5s_header->data_size,
                    ctxt.module_id,
                    ctxt.rnti,
                    rab_id);
              MSC_LOG_RX_MESSAGE(
                (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
                NULL,
                0,
                MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
                MSC_AS_TIME_ARGS(ctxt_pP),
                pc5s_header->inst,
                pc5s_header->rb_id,
                rab_id,
                pc5s_header->data_size);
              pdcp_data_req(
                &ctxt,
                SRB_FLAG_NO,
                rab_id,
                RLC_MUI_UNDEFINED,
                RLC_SDU_CONFIRM_NO,
                pc5s_header->data_size,
                (unsigned char *)receive_buf,
                PDCP_TRANSMISSION_MODE_DATA,
                &pc5s_header->sourceL2Id,
                &pc5s_header->destinationL2Id
              );
            } else { /* else of h_rc == HASH_TABLE_OK */
              MSC_LOG_RX_DISCARDED_MESSAGE(
                (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
                (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
                NULL,
                0,
                MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u rab %u size %u",
                MSC_AS_TIME_ARGS(ctxt_pP),
                pc5s_header->inst,
                pc5s_header->rb_id,
                rab_id,
                pc5s_header->data_size);
              LOG_D(PDCP,
                    "[FRAME %5u][UE][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes ---X][PDCP][MOD %u][UE %u][RB %ld] NON INSTANCIATED INSTANCE key 0x%"PRIx64", DROPPED\n",
                    ctxt.frame,
                    pc5s_header->inst,
                    pc5s_header->rb_id,
                    pc5s_header->data_size,
                    ctxt.module_id,
                    ctxt.rnti,
                    rab_id,
                    key);
            }
          }  else { /* else of if (rab_id == 0) */
            LOG_D(PDCP, "Forcing send on DEFAULT_RAB_ID\n");
            LOG_D(PDCP, "[FRAME %5u][eNB][IP][INSTANCE %u][RB %ld][--- PDCP_DATA_REQ / %d Bytes --->][PDCP][MOD %u][UE %u][RB DEFAULT_RAB_ID %u]\n",
                  ctxt.frame,
                  pc5s_header->inst,
                  pc5s_header->rb_id,
                  pc5s_header->data_size,
                  ctxt.module_id,
                  ctxt.rnti,
                  DEFAULT_RAB_ID);
            MSC_LOG_RX_MESSAGE(
              (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
              (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_IP_ENB:MSC_IP_UE,
              NULL,0,
              MSC_AS_TIME_FMT" DATA-REQ inst %u rb %u default rab %u size %u",
              MSC_AS_TIME_ARGS(ctxt_pP),
              pc5s_header->inst,
              pc5s_header->rb_id,
              DEFAULT_RAB_ID,
              pc5s_header->data_size);
            pdcp_data_req (
              &ctxt,
              SRB_FLAG_NO,
              DEFAULT_RAB_ID,
              RLC_MUI_UNDEFINED,
              RLC_SDU_CONFIRM_NO,
              pc5s_header->data_size,
              (unsigned char *)receive_buf,
              PDCP_TRANSMISSION_MODE_DATA,
              &pc5s_header->sourceL2Id,
              &pc5s_header->destinationL2Id
            );
          }
        } /* end of !ctxt.enb_flag */

      default:
        LOG_D(PDCP, "pc5s message type %d, unknown...\n", pc5s_header->traffic_type);
        break;
    } /* end of switch */
  }/* end of bytes_received > 0 */
  if (pc5s_header != NULL) {
    free(pc5s_header);
    pc5s_header = NULL;
  }
} /* pdcp_fifo_read_input_sdus_frompc5s */

//-----------------------------------------------------------------------------
int pdcp_fifo_read_input_sdus (const protocol_ctxt_t *const  ctxt_pP) {
  if (UE_NAS_USE_TUN || ENB_NAS_USE_TUN) {
    return pdcp_fifo_read_input_sdus_fromtun (ctxt_pP);
  } else if (PDCP_USE_NETLINK) {
    pdcp_fifo_read_input_sdus_frompc5s (ctxt_pP);
    return pdcp_fifo_read_input_sdus_fromnetlinksock(ctxt_pP);
  } /* PDCP_USE_NETLINK */

  return 0;
}

//TTN for D2D (PC5S)

void
pdcp_pc5_socket_init() {
  //pthread_attr_t     attr;
  //struct sched_param sched_param;
  int optval; // flag value for setsockopt
  //int n; // message byte size
  //create PDCP socket
  pdcp_pc5_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (pdcp_pc5_sockfd < 0) {
    LOG_E(PDCP,"[pdcp_pc5_socket_init] Error opening socket %d (%d:%s)\n",pdcp_pc5_sockfd,errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  optval = 1;
  setsockopt(pdcp_pc5_sockfd, SOL_SOCKET, SO_REUSEADDR,
             (const void *)&optval, sizeof(int));
  fcntl(pdcp_pc5_sockfd,F_SETFL,O_NONBLOCK);
  bzero((char *) &pdcp_sin, sizeof(pdcp_sin));
  pdcp_sin.sin_family = AF_INET;
  pdcp_sin.sin_addr.s_addr = htonl(INADDR_ANY);
  pdcp_sin.sin_port = htons(PDCP_SOCKET_PORT_NO);

  // associate the parent socket with a port
  if (bind(pdcp_pc5_sockfd, (struct sockaddr *) &pdcp_sin,
           sizeof(pdcp_sin)) < 0) {
    LOG_E(PDCP,"[pdcp_pc5_socket_init] ERROR: Failed on binding the socket\n");
    exit(1);
  }
}

