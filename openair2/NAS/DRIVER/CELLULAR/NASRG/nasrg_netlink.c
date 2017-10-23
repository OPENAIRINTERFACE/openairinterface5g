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

/*! \file nasrg_netlink.c
* \brief Netlink socket functions for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
//#include <linux/config.h>
#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/kthread.h>

#include <linux/mutex.h>

#include "nasrg_variables.h"
#include "nasrg_proto.h"

static struct sock *nas_nl_sk = NULL;
static struct sock *nas_rrcnl_sk = NULL;

//static int exit_netlink_thread=0;
static DEFINE_MUTEX(nasrg_mutex);

// This can also be implemented using thread to get the data from PDCP without blocking.
//---------------------------------------------------------------------------
// Function for transfer with PDCP (from NASLITE)
static void nasrg_nl_data_ready (struct sk_buff *skb)
{
  //---------------------------------------------------------------------------
  struct nlmsghdr *nlh = NULL;

  // Start debug information
#ifdef NETLINK_DEBUG
  printk("nasrg_nl_data_ready - begin \n");
#endif

  if (!skb) {
    printk("nasrg_nl_data_ready - input parameter skb is NULL \n");
    return;
  }

  // End debug information

#ifdef NETLINK_DEBUG
  printk("nasrg_nl_data_ready - Received socket from PDCP\n");
#endif //NETLINK_DEBUG
  nlh = (struct nlmsghdr *)skb->data;
  nasrg_COMMON_QOS_receive(nlh);
}

//---------------------------------------------------------------------------
//  Function for transfer with RRC
static void nasrg_rrcnl_data_ready (struct sk_buff *skb)
{
  //---------------------------------------------------------------------------
  struct nlmsghdr *nlh = NULL;
  char target_sap;
  uint8_t cxi;

  // Start debug information
#ifdef NAS_DEBUG_RRCNL
  printk("nasrg_rrcnl_data_ready - begin \n");
#endif

  if (!skb) {
    printk("nasrg_rrcnl_data_ready - input parameter skb is NULL \n");
    return;
  }

  // End debug information

  nlh = (struct nlmsghdr *)skb->data;
  //pdcph = (struct pdcp_data_ind_header_t *)NLMSG_DATA(nlh);
  //nasrg_TOOL_print_buffer((char *)NLMSG_DATA(nlh), 48);

  target_sap = ((char*)NLMSG_DATA(nlh))[0];
#ifdef NAS_DEBUG_RRCNL
  printk("nasrg_rrcnl_data_ready - Received on socket from RRC, SAP %d\n", target_sap);
#endif //NAS_DEBUG_RRCNL

  switch (target_sap) {
  case RRC_NAS_DC0_OUT:
    //printk("nasrg_rrcnl_data_ready - Calling  nasrg_ASCTL_DC_receive\n");
    cxi = 0;
    nasrg_ASCTL_DC_receive(gpriv->cx+cxi, &((char*)NLMSG_DATA(nlh))[1]);
    break;

  case RRC_NAS_DC1_OUT:
    //printk("nasrg_rrcnl_data_ready - Calling  nasrg_ASCTL_DC_receive\n");
    cxi = 1;
    nasrg_ASCTL_DC_receive(gpriv->cx+cxi, &((char*)NLMSG_DATA(nlh))[1]);
    break;

  case RRC_NAS_DC2_OUT:
    //printk("nasrg_rrcnl_data_ready - Calling  nasrg_ASCTL_DC_receive\n");
    cxi = 2;
    nasrg_ASCTL_DC_receive(gpriv->cx+cxi, &((char*)NLMSG_DATA(nlh))[1]);
    break;

  default:
    printk("nasrg_rrcnl_data_ready - Invalid SAP value received\n");
  }

}

//---------------------------------------------------------------------------
int nasrg_netlink_init(void)
{
  //---------------------------------------------------------------------------
  printk("nasrg_netlink_init - begin \n");

  nas_nl_sk = netlink_kernel_create(&init_net,OAI_IP_DRIVER_NETLINK_ID, 0, nasrg_nl_data_ready,
                                    &nasrg_mutex, // NULL
                                    THIS_MODULE);

  if (!nas_nl_sk) {
    printk("nasrg_netlink_init - netlink_kernel_create failed for PDCP socket\n");
    // TEMP printk("nasrg_netlink_init - netlink_kernel_create failed for PDCP socket %d\n", errno);
    return(-1);
  }

  nas_rrcnl_sk = netlink_kernel_create(&init_net,NAS_RRCNL_ID, 0, nasrg_rrcnl_data_ready,
                                       &nasrg_mutex, // NULL
                                       THIS_MODULE);

  if (!nas_rrcnl_sk) {
    printk("nasrg_rrcnl_init - netlink_kernel_create failed for RRC socket\n");
    return(-1);
  }

  return(0);
}

//---------------------------------------------------------------------------
void nasrg_netlink_release(void)
{
  //---------------------------------------------------------------------------
  printk("nasrg_netlink_release - begin \n");

  //exit_netlink_thread=1;
  printk("nasrg_netlink_release - Releasing netlink sockets\n");

  if(nas_nl_sk) {
    netlink_kernel_release(nas_nl_sk);
  }

  if(nas_rrcnl_sk) {
    netlink_kernel_release(nas_rrcnl_sk);
  }

}

//---------------------------------------------------------------------------
int nasrg_netlink_send(unsigned char *data_buffer, unsigned int data_length, int destination)
{
  //---------------------------------------------------------------------------
  struct sk_buff *nl_skb;
  struct nlmsghdr *nlh;
  int status;
  // Start debug information
#ifdef NETLINK_DEBUG
  printk("nasrg_netlink_send - begin \n");
#endif

  if (!data_buffer) {
    printk("nasrg_netlink_send - ERROR - input parameter data is NULL \n");
    return(0);
  }

  if (!nas_nl_sk || !nas_rrcnl_sk) {
    printk("nasrg_netlink_send - ERROR - socket is NULL\n");
    return(0);
  }

  // End debug information

  nl_skb = alloc_skb(NLMSG_SPACE(data_length),GFP_ATOMIC);

  if (!nl_skb) {
    printk("nasrg_netlink_send - ERROR - could not allocate skbuffer\n");
    return(0);
  }

  nlh = (struct nlmsghdr *)nl_skb->data;

  //  printk("nasrg_netlink_send Sending %d bytes (%d)\n",data_length,NLMSG_SPACE(data_length));
  skb_put(nl_skb, NLMSG_SPACE(data_length));
  memcpy(NLMSG_DATA(nlh), data_buffer, data_length);
  nlh->nlmsg_len = NLMSG_SPACE(data_length);

  nlh->nlmsg_pid = 0;      /* from kernel */
  NETLINK_CB(nl_skb).pid = 0;

  // destination 0 = PDCP, 1 = RRC
  if (destination== NASNL_DEST_PDCP) {
#ifdef NETLINK_DEBUG
    printk("nasrg_netlink_send - Sending to PDCP - nl_skb %p, nl_sk %p, nlh %p, nlh->nlmsg_len %d\n", nl_skb, nas_nl_sk, nlh, nlh->nlmsg_len);
#ifdef NAS_DEBUG_SEND_DETAIL
    nasrg_TOOL_print_buffer(NLMSG_DATA(nlh),48);
#endif
#endif //DEBUG_NETLINK
    status = netlink_unicast(nas_nl_sk, nl_skb, NL_DEST_PID, MSG_DONTWAIT);
  } else {
#ifdef NAS_DEBUG_RRCNL
    printk("nasrg_netlink_send - Sending to RRC - nl_skb %p, nas_rrcnl_sk %p, nlh %p, nlh->nlmsg_len %d\n", nl_skb, nas_rrcnl_sk, nlh, nlh->nlmsg_len);
    nasrg_TOOL_print_buffer(NLMSG_DATA(nlh),data_length);
#endif //NAS_DEBUG_RRCNL
    status = netlink_unicast(nas_rrcnl_sk, nl_skb, NL_DEST_RRC_PID, MSG_DONTWAIT);
  }

  if (status < 0) {
    printk("nasrg_netlink_send - SEND status is %d\n",status);
    return(0);
  } else {
#ifdef NETLINK_DEBUG
    printk("nasrg_netlink_send - SEND status is %d, data_length %d\n",status, data_length);
#endif
    return data_length;
  }
}


