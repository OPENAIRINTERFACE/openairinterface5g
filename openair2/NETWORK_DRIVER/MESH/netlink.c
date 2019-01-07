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

/*! \file netlink.c
* \brief establish a netlink
* \author Navid Nikaein, Lionel Gauthier, and Raymond knopp
* \company Eurecom
* \email:navid.nikaein@eurecom.fr, lionel.gauthier@eurecom.fr, knopp@eurecom.fr
*/

//#include <linux/config.h>
#include <linux/version.h>
#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/kthread.h>

#include <linux/mutex.h>

#include "local.h"
#include "proto_extern.h"
//#include "platform_constants.h"

//#define NETLINK_DEBUG 1


#define OAI_IP_DRIVER_NETLINK_ID 31
#define NL_DEST_PID 1


static struct sock *nas_nl_sk = NULL;
static int exit_netlink_thread=0;
//static int nas_netlink_rx_thread(void *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
struct netlink_kernel_cfg oai_netlink_cfg;
#endif

static DEFINE_MUTEX(nasmesh_mutex);

static inline void nasmesh_lock(void)
{
  mutex_lock(&nasmesh_mutex);
}

static inline void nasmesh_unlock(void)
{
  mutex_unlock(&nasmesh_mutex);
}

// This can also be implemented using thread to get the data from PDCP without blocking.
static void nas_nl_data_ready (struct sk_buff *skb)
{
  // wake_up_interruptible(skb->sk->sk_sleep);
  //nasmesh_lock();
  //netlink_rcv_skb(skb, &my_rcv_msg);// my_rcv_msg is the call back func>
  //nasmesh_unlock();

  struct nlmsghdr *nlh = NULL;
  //int j;

  if (skb) {

#ifdef NETLINK_DEBUG
    printk("[NAS][NETLINK] Received socket from PDCP\n");
#endif //NETLINK_DEBUG
    nlh = (struct nlmsghdr *)skb->data;

    nas_COMMON_QOS_receive(nlh);

    //kfree_skb(skb); // not required,
  }

}

int nas_netlink_init()
{

  printk("[NAS][NETLINK] Running init ...\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
  oai_netlink_cfg.groups   = 0;
  oai_netlink_cfg.input    = nas_nl_data_ready;
  oai_netlink_cfg.cb_mutex = &nasmesh_mutex;
  oai_netlink_cfg.bind     = NULL;

  nas_nl_sk = netlink_kernel_create(
                &init_net,
                OAI_IP_DRIVER_NETLINK_ID,
# if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
                THIS_MODULE,
# endif
                &oai_netlink_cfg);
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */
  nas_nl_sk = netlink_kernel_create(
                &init_net,
                OAI_IP_DRIVER_NETLINK_ID,
                0,
                nas_nl_data_ready,
                &nasmesh_mutex, // NULL
                THIS_MODULE);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */

  if (nas_nl_sk == NULL) {

    printk("[NAS][NETLINK] netlink_kernel_create failed \n");
    return(-1);
  }



  return(0);

}


void nas_netlink_release(void)
{

  exit_netlink_thread=1;
  printk("[NAS][NETLINK] Releasing netlink socket\n");

  if(nas_nl_sk) {
    netlink_kernel_release(nas_nl_sk); //or skb->sk

  }

  //  printk("[NAS][NETLINK] Removing netlink_rx_thread\n");
  //kthread_stop(netlink_rx_thread);

}




int nas_netlink_send(unsigned char *data,unsigned int len)
{


  struct sk_buff *nl_skb = alloc_skb(NLMSG_SPACE(len),GFP_ATOMIC);
  struct nlmsghdr *nlh = (struct nlmsghdr *)nl_skb->data;
  int status;


  //  printk("[NAS][NETLINK] Sending %d bytes (%d)\n",len,NLMSG_SPACE(len));
  skb_put(nl_skb, NLMSG_SPACE(len));
  memcpy(NLMSG_DATA(nlh),data,len);

  nlh->nlmsg_len = NLMSG_SPACE(len);


  nlh->nlmsg_pid = 0;      /* from kernel */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
  NETLINK_CB(nl_skb).portid = 0;
#else
  NETLINK_CB(nl_skb).pid = 0;
#endif

#ifdef NETLINK_DEBUG
  printk("[NAS][NETLINK] In nas_netlink_send, nl_skb %p, nl_sk %x, nlh %p, nlh->nlmsg_len %d (OAI_IP_DRIVER_NETLINK_ID %d)\n",
         nl_skb,nas_nl_sk,nlh,nlh->nlmsg_len,
         OAI_IP_DRIVER_NETLINK_ID);
#endif //DEBUG_NETLINK

  if (nas_nl_sk) {

    //  nasmesh_lock();

    status = netlink_unicast(nas_nl_sk, nl_skb, NL_DEST_PID, MSG_DONTWAIT);
    // mutex_unlock(&nasmesh_mutex);

    if (status < 0) {
      printk("[NAS][NETLINK] SEND status is %d\n",status);
      return(0);
    } else {
#ifdef NETLINK_DEBUG
      printk("[NAS][NETLINK] SEND status is %d\n",status);
#endif
      return len;
    }
  } else {
    printk("[NAS][SEND] socket is NULL\n");
    return(0);
  }

  /*
    nlmsg_failure:  // Used by NLMSG_PUT
  if (nl_skb)
    kfree_skb(nl_skb);
  */

}
