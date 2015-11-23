/***************************************************************************
                          nasmt_netlink.c  -  description
                             -------------------
 ***************************************************************************
  Eurecom OpenAirInterface 2
  Copyright(c) 1999 - 2013 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 450 route des Chappes, 06410 Biot Sophia Antipolis, France
*******************************************************************************/
/*! \file nasmt_netlink.c
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

#include "nasmt_variables.h"
#include "nasmt_proto.h"

static struct sock *nas_nl_sk = NULL;
static struct sock *nas_rrcnl_sk = NULL;

//static int exit_netlink_thread=0;
static DEFINE_MUTEX(nasmt_mutex);

// This can also be implemented using thread to get the data from PDCP without blocking.
//---------------------------------------------------------------------------
// Function for transfer with PDCP (from NASLITE)
static void nasmt_nl_data_ready (struct sk_buff *skb)
{
  //---------------------------------------------------------------------------
  struct nlmsghdr *nlh = NULL;

  // Start debug information
#ifdef NETLINK_DEBUG
  printk("nasmt_nl_data_ready - begin \n");
#endif

  if (!skb) {
    printk("nasmt_nl_data_ready - input parameter skb is NULL \n");
    return;
  }

  // End debug information

#ifdef NETLINK_DEBUG
  printk("nasmt_nl_data_ready - Received socket from PDCP\n");
#endif //NETLINK_DEBUG
  nlh = (struct nlmsghdr *)skb->data;
  nasmt_COMMON_QOS_receive(nlh);
}

//---------------------------------------------------------------------------
//  Function for transfer with RRC
static void nasmt_rrcnl_data_ready (struct sk_buff *skb)
{
  //---------------------------------------------------------------------------
  struct nlmsghdr *nlh = NULL;
  char target_sap;
  uint8_t cxi = 0;

  // Start debug information
#ifdef NAS_DEBUG_RRCNL
  printk("nasmt_rrcnl_data_ready - begin \n");
#endif

  if (!skb) {
    printk("nasmt_rrcnl_data_ready - input parameter skb is NULL \n");
    return;
  }

  // End debug information

  nlh = (struct nlmsghdr *)skb->data;
  //pdcph = (struct pdcp_data_ind_header_t *)NLMSG_DATA(nlh);
  //nasmt_TOOL_print_buffer((char *)NLMSG_DATA(nlh), 48);

  target_sap = ((char*)NLMSG_DATA(nlh))[0];
#ifdef NAS_DEBUG_RRCNL
  printk("nasmt_rrcnl_data_ready - Received on socket from RRC, SAP %d\n", target_sap);
#endif //NAS_DEBUG_RRCNL

  switch (target_sap) {
  case RRC_NAS_GC_OUT:
    //printk("nasmt_rrcnl_data_ready - Calling  nasmt_ASCTL_GC_receive\n");
    nasmt_ASCTL_GC_receive(&((char*)NLMSG_DATA(nlh))[1]);
    break;

  case RRC_NAS_DC0_OUT:
    //printk("nasmt_rrcnl_data_ready - Calling  nasmt_ASCTL_DC_receive\n");
    nasmt_ASCTL_DC_receive(gpriv->cx+cxi, &((char*)NLMSG_DATA(nlh))[1]);
    break;

  default:
    printk("nasmt_rrcnl_data_ready - Invalid SAP value received\n");
  }

}

//---------------------------------------------------------------------------
int nasmt_netlink_init(void)
{
  //---------------------------------------------------------------------------
  printk("nasmt_netlink_init - begin \n");

  nas_nl_sk = netlink_kernel_create(&init_net,OAI_IP_DRIVER_NETLINK_ID, 0, nasmt_nl_data_ready,
                                    &nasmt_mutex, // NULL
                                    THIS_MODULE);

  if (!nas_nl_sk) {
    printk("nasmt_netlink_init - netlink_kernel_create failed for PDCP socket\n");
    return(-1);
  }

  nas_rrcnl_sk = netlink_kernel_create(&init_net,NAS_RRCNL_ID, 0, nasmt_rrcnl_data_ready,
                                       &nasmt_mutex, // NULL
                                       THIS_MODULE);

  if (!nas_rrcnl_sk) {
    printk("nasmt_rrcnl_init - netlink_kernel_create failed for RRC socket\n");
    return(-1);
  }

  return(0);
}

//---------------------------------------------------------------------------
void nasmt_netlink_release(void)
{
  //---------------------------------------------------------------------------
  printk("nasmt_netlink_release - begin \n");

  //exit_netlink_thread=1;
  printk("nasmt_netlink_release - Releasing netlink sockets\n");

  if(nas_nl_sk) {
    netlink_kernel_release(nas_nl_sk);
  }

  if(nas_rrcnl_sk) {
    netlink_kernel_release(nas_rrcnl_sk);
  }

}

//---------------------------------------------------------------------------
int nasmt_netlink_send(unsigned char *data_buffer, unsigned int data_length, int destination)
{
  //---------------------------------------------------------------------------
  struct sk_buff *nl_skb;
  struct nlmsghdr *nlh;
  int status;
  // Start debug information
#ifdef NETLINK_DEBUG
  printk("nasmt_netlink_send - begin \n");
#endif

  if (!data_buffer) {
    printk("nasmt_netlink_send - ERROR - input parameter data is NULL \n");
    return(0);
  }

  if (!nas_nl_sk || !nas_rrcnl_sk) {
    printk("nasmt_netlink_send - ERROR - socket is NULL\n");
    return(0);
  }

  // End debug information

  nl_skb = alloc_skb(NLMSG_SPACE(data_length),GFP_ATOMIC);

  if (!nl_skb) {
    printk("nasmt_netlink_send - ERROR - could not allocate skbuffer\n");
    return(0);
  }

  nlh = (struct nlmsghdr *)nl_skb->data;

  //  printk("nasmt_netlink_send Sending %d bytes (%d)\n",data_length,NLMSG_SPACE(data_length));
  skb_put(nl_skb, NLMSG_SPACE(data_length));
  memcpy(NLMSG_DATA(nlh), data_buffer, data_length);
  nlh->nlmsg_len = NLMSG_SPACE(data_length);

  nlh->nlmsg_pid = 0;      /* from kernel */
  NETLINK_CB(nl_skb).pid = 0;

  // destination 0 = PDCP, 1 = RRC
  if (!destination) {
#ifdef NETLINK_DEBUG
    printk("nasmt_netlink_send - nl_skb %p, nl_sk %p, nlh %p, nlh->nlmsg_len %d\n", nl_skb, nas_nl_sk, nlh, nlh->nlmsg_len);
#endif //DEBUG_NETLINK
    status = netlink_unicast(nas_nl_sk, nl_skb, NL_DEST_PID, MSG_DONTWAIT);
  } else {
#ifdef NAS_DEBUG_RRCNL
    printk("nasmt_rrcnl_send - nl_skb %p, nas_rrcnl_sk %p, nlh %p, nlh->nlmsg_len %d\n", nl_skb, nas_rrcnl_sk, nlh, nlh->nlmsg_len);
#endif //NAS_DEBUG_RRCNL
    status = netlink_unicast(nas_rrcnl_sk, nl_skb, NL_DEST_RRC_PID, MSG_DONTWAIT);
  }

  if (status < 0) {
    printk("nasmt_netlink_send - SEND status is %d\n",status);
    return(0);
  } else {
#ifdef NETLINK_DEBUG
    printk("nasmt_netlink_send - SEND status is %d, data_length %d\n",status, data_length);
#endif
    return data_length;
  }
}


