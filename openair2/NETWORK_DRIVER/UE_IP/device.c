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

/*! \file device.c
* \brief Networking Device Driver for OpenAirInterface
* \author  navid nikaein,  Lionel Gauthier, raymond knopp
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr, lionel.gauthier@eurecom.fr

*/
/*******************************************************************************/

#include "constant.h"
#include "local.h"
#include "proto_extern.h"

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/delay.h>
#include <asm/unistd.h>
#include <linux/netdevice.h>



struct net_device *ue_ip_dev[UE_IP_NB_INSTANCES_MAX];

#ifdef OAI_NW_DRIVER_USE_NETLINK
extern void ue_ip_netlink_release(void);
extern int ue_ip_netlink_init(void);
#endif

//---------------------------------------------------------------------------
int ue_ip_find_inst(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  int i;

  for (i=0; i<UE_IP_NB_INSTANCES_MAX; i++)
    if (ue_ip_dev[i] == dev_pP) {
      return(i);
    }

  return(-1);
}

//---------------------------------------------------------------------------

#ifndef OAI_NW_DRIVER_USE_NETLINK
void *ue_ip_interrupt(void)
{
  //---------------------------------------------------------------------------
  uint8_t cxi;

  //  ue_ip_priv_t *priv_p=netdev_priv(dev_id);
  //  unsigned int flags;
  //  priv_p->lock = SPIN_LOCK_UNLOCKED;

#ifdef OAI_DRV_DEBUG_INTERRUPT
  printk("INTERRUPT - begin\n");
#endif
  //  spin_lock_irqsave(&priv_p->lock,flags);
  cxi=0;
  //    mesh_GC_receive();
  //    mesh_DC_receive(naspriv->cx+cxi);
#ifndef OAI_NW_DRIVER_USE_NETLINK
  ue_ip_common_wireless2ip();
#endif
  //  spin_unlock_irqrestore(&priv_p->lock,flags);
#ifdef OAI_DRV_DEBUG_INTERRUPT
  printk("INTERRUPT: end\n");
#endif
  //  return 0;
}
#endif //NETLINK
//---------------------------------------------------------------------------
void ue_ip_timer(unsigned long dataP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p=(ue_ip_priv_t *)dataP;
  spin_lock(&priv_p->lock);
  (priv_p->timer).function=ue_ip_timer;
  (priv_p->timer).expires=jiffies+UE_IP_TIMER_TICK;
  (priv_p->timer).data=dataP;
  add_timer(&priv_p->timer);
  spin_unlock(&priv_p->lock);
  return;
  //  add_timer(&gpriv->timer);
  //  spin_unlock(&gpriv->lock);
}

//---------------------------------------------------------------------------
// Called by ifconfig when the device is activated by ifconfig
int ue_ip_open(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p=netdev_priv(dev_pP);

  // Address has already been set at init
#ifndef OAI_NW_DRIVER_USE_NETLINK

  if (pdcp_2_ue_ip_irq==-EBUSY) {
    printk("[UE_IP_DRV][%s] : irq failure\n", __FUNCTION__);
    return -EBUSY;
  }

#endif //OAI_NW_DRIVER_USE_NETLINK

  if(!netif_queue_stopped(dev_pP)) {
    netif_start_queue(dev_pP);
  } else {
    netif_wake_queue(dev_pP);
  }

  init_timer(&priv_p->timer);
  (priv_p->timer).expires   = jiffies+UE_IP_TIMER_TICK;
  (priv_p->timer).data      = (unsigned long)priv_p;
  (priv_p->timer).function  = ue_ip_timer;
  //add_timer(&priv_p->timer);

  printk("[UE_IP_DRV][%s] name = %s\n", __FUNCTION__, dev_pP->name);
  return 0;
}

//---------------------------------------------------------------------------
// Called by ifconfig when the device is desactivated
int ue_ip_stop(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p = netdev_priv(dev_pP);

  printk("[UE_IP_DRV][%s] Begin\n", __FUNCTION__);
  del_timer(&(priv_p->timer));
  netif_stop_queue(dev_pP);
  //    MOD_DEC_USE_COUNT;
  printk("[UE_IP_DRV][%s] End\n", __FUNCTION__);
  return 0;
}

//---------------------------------------------------------------------------
void ue_ip_teardown(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t    *priv_p;
  int              inst;

  printk("[UE_IP_DRV][%s] Begin\n", __FUNCTION__);

  if (dev_pP) {
    priv_p = netdev_priv(dev_pP);
    inst = ue_ip_find_inst(dev_pP);

    if ((inst<0) || (inst>=UE_IP_NB_INSTANCES_MAX)) {
      printk("[UE_IP_DRV][%s] ERROR, couldn't find instance\n", __FUNCTION__);
      return;
    }


    printk("[UE_IP_DRV][%s] End\n", __FUNCTION__);
  } // check dev_pP
  else {
    printk("[UE_IP_DRV][%s] Device is null\n", __FUNCTION__);
  }
}
//---------------------------------------------------------------------------
int ue_ip_set_config(struct net_device *dev_pP, struct ifmap *map_pP)
{
  //---------------------------------------------------------------------------
  printk("[UE_IP_DRV][%s] Begin\n", __FUNCTION__);

  if (dev_pP->flags & IFF_UP) {
    return -EBUSY;
  }

  if (map_pP->base_addr != dev_pP->base_addr) {
    printk(KERN_WARNING "[UE_IP_DRV][%s] Can't change I/O address\n", __FUNCTION__);
    return -EOPNOTSUPP;
  }

  if (map_pP->irq != dev_pP->irq) {
    dev_pP->irq = map_pP->irq;
  }

  printk("[UE_IP_DRV][%s] End\n", __FUNCTION__);
  return 0;
}

//---------------------------------------------------------------------------
//
int ue_ip_hard_start_xmit(struct sk_buff *skb_pP, struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  int inst;

  if (dev_pP) {
    inst = ue_ip_find_inst(dev_pP);
  } else {
    printk("[UE_IP_DRV][%s] ERROR, device is null\n", __FUNCTION__);
    return -1;
  }

  if ((inst>=0) && (inst<UE_IP_NB_INSTANCES_MAX)) {
#ifdef OAI_DRV_OAI_DRV_DEBUG_DEVICE
    printk("[UE_IP_DRV][%s] inst %d,  begin\n", __FUNCTION__,inst);
#endif

    if (!skb_pP) {
      printk("[UE_IP_DRV][%s] input parameter skb is NULL\n", __FUNCTION__);
      return -1;
    }

    // End debug information
    netif_stop_queue(dev_pP);
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0) || RHEL_RELEASE_CODE >= 1796
    netif_trans_update(dev_pP);
#else
    dev_pP->trans_start = jiffies;
#endif
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[UE_IP_DRV][%s] step 1\n", __FUNCTION__);
#endif
    ue_ip_common_ip2wireless(skb_pP,inst);
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[UE_IP_DRV][%s] step 2\n", __FUNCTION__);
#endif
    dev_kfree_skb(skb_pP);
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[UE_IP_DRV][%s] step 3\n", __FUNCTION__);
#endif
    netif_wake_queue(dev_pP);
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[UE_IP_DRV][%s] end\n", __FUNCTION__);
#endif
  } else {
    printk("[UE_IP_DRV][%s] ERROR, couldn't find instance\n", __FUNCTION__);
    return(-1);
  }

  return 0;
}

//---------------------------------------------------------------------------
struct net_device_stats *ue_ip_get_stats(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p = netdev_priv(dev_pP);
  return &priv_p->stats;
}
//---------------------------------------------------------------------------
int ue_ip_set_mac_address(struct net_device *dev_pP, void *mac_pP)
{
  //---------------------------------------------------------------------------
  //struct sockaddr *addr = mac_pP;
  printk("[UE_IP_DRV][%s] CHANGE MAC ADDRESS UNSUPPORTED\n", __FUNCTION__);
  //memcpy(dev_pP->dev_addr, addr->sa_data, dev_pP->addr_len);
  return 0;
}
//---------------------------------------------------------------------------
int ue_ip_change_mtu(struct net_device *dev_pP, int mtuP)
{
  //---------------------------------------------------------------------------
  printk("[UE_IP_DRV][%s] CHANGE MTU %d bytes\n", __FUNCTION__, mtuP);

  if ((mtuP<50) || (mtuP>1500)) {
    return -EINVAL;
  }

  dev_pP->mtu = mtuP;
  return 0;
}
//---------------------------------------------------------------------------
void ue_ip_change_rx_flags(struct net_device *dev_pP, int flagsP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p =  netdev_priv(dev_pP);
  printk("[UE_IP_DRV][%s] CHANGE RX FLAGS %08X\n", __FUNCTION__, flagsP);
  priv_p->rx_flags ^= flagsP;
}

//---------------------------------------------------------------------------
void ue_ip_tx_timeout(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  // Transmitter timeout, serious problems.
  ue_ip_priv_t *priv_p =  netdev_priv(dev_pP);

  printk("[UE_IP_DRV][%s] begin\n", __FUNCTION__);
  //  (ue_ip_priv_t *)(dev_pP->priv_p)->stats.tx_errors++;
  (priv_p->stats).tx_errors++;
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0) || RHEL_RELEASE_CODE >= 1796
  netif_trans_update(dev_pP);
#else
  dev_pP->trans_start = jiffies;
#endif
  netif_wake_queue(dev_pP);
  printk("[UE_IP_DRV][%s] transmit timed out %s\n", __FUNCTION__,dev_pP->name);
}

static const struct net_device_ops ue_ip_netdev_ops = {
  .ndo_open               = ue_ip_open,
  .ndo_stop               = ue_ip_stop,
  .ndo_start_xmit         = ue_ip_hard_start_xmit,
  .ndo_validate_addr      = NULL,
  .ndo_get_stats          = ue_ip_get_stats,
  .ndo_set_mac_address    = ue_ip_set_mac_address,
  .ndo_set_config         = ue_ip_set_config,
  .ndo_do_ioctl           = NULL,
  .ndo_change_mtu         = ue_ip_change_mtu,
  .ndo_tx_timeout         = ue_ip_tx_timeout,
  .ndo_change_rx_flags    = ue_ip_change_rx_flags,
};
/*.ndo_set_multicast_list = NULL,*/

//---------------------------------------------------------------------------
// Initialisation of the network device
void ue_ip_init(struct net_device *dev_pP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p = NULL;

  if (dev_pP) {
    priv_p = netdev_priv(dev_pP);
    memset(priv_p, 0, sizeof(ue_ip_priv_t));
    spin_lock_init(&priv_p->lock);
    dev_pP->netdev_ops = &ue_ip_netdev_ops;
    dev_pP->hard_header_len = 0;
    dev_pP->addr_len = UE_IP_ADDR_LEN;
    dev_pP->flags = IFF_BROADCAST|IFF_MULTICAST|IFF_NOARP;
    dev_pP->tx_queue_len = UE_IP_TX_QUEUE_LEN;
    dev_pP->mtu = UE_IP_MTU;
  } else {
    printk("[UE_IP_DRV][%s] ERROR, Device is NULL!!\n", __FUNCTION__);
    return;
  }
}
//---------------------------------------------------------------------------
int init_module (void)
{
  //---------------------------------------------------------------------------
  int err,inst;
  char devicename[100];


  // Initialize parameters shared with RRC
  printk("[UE_IP_DRV][%s] Starting OAI IP driver", __FUNCTION__);

  for (inst=0; inst<UE_IP_NB_INSTANCES_MAX; inst++) {
    printk("[UE_IP_DRV][%s] begin init instance %d\n", __FUNCTION__,inst);
    sprintf(devicename,"oip%d",inst);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
    ue_ip_dev[inst]  = alloc_netdev(sizeof(ue_ip_priv_t),devicename, ue_ip_init);
#else
    ue_ip_dev[inst]  = alloc_netdev(sizeof(ue_ip_priv_t),devicename, NET_NAME_PREDICTABLE,ue_ip_init);
#endif

    //netif_stop_queue(ue_ip_dev[inst]);
    if (ue_ip_dev[inst] == NULL) {
      printk("[UE_IP_DRV][%s][INST %02d] alloc_netdev FAILED\n", __FUNCTION__,inst);
    } else {
      // linux/net/core/dev.c line 4767
      err= register_netdev(ue_ip_dev[inst]);

      if (err) {
        printk("[UE_IP_DRV][%s] (inst %d): error %i registering device %s\n", __FUNCTION__, inst,err, ue_ip_dev[inst]->name);
      } else {
        printk("[UE_IP_DRV][%s] registering device %s, ifindex = %d\n\n", __FUNCTION__,ue_ip_dev[inst]->name, ue_ip_dev[inst]->ifindex);
      }
    }
  }

  printk("[UE_IP_DRV][%s] NETLINK INIT\n", __FUNCTION__);

  if ((err=ue_ip_netlink_init()) == -1) {
    printk("[UE_IP_DRV][%s] NETLINK failed\n", __FUNCTION__);
  }

  return err;

}

//---------------------------------------------------------------------------
void cleanup_module(void)
{
  //---------------------------------------------------------------------------
  int inst;

  printk("[UE_IP_DRV][CLEANUP] begin\n");

  for (inst=0; inst<UE_IP_NB_INSTANCES_MAX; inst++) {
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[UE_IP_DRV][CLEANUP]  unregister and free net device instance %d\n",inst);
#endif

    if (ue_ip_dev[inst]) {
      unregister_netdev(ue_ip_dev[inst]);
      ue_ip_teardown(ue_ip_dev[inst]);
      free_netdev(ue_ip_dev[inst]);
    }
  }

  ue_ip_netlink_release();
  printk("[UE_IP_DRV][CLEANUP] end\n");
}

#define DRV_NAME        "ue_ip"
#define DRV_VERSION     "1.0"DRV_NAME
#define DRV_DESCRIPTION "OPENAIR UE IP Device Driver"
#define DRV_COPYRIGHT   "-Copyright(c) GNU GPL Eurecom 2013"
#define DRV_AUTHOR      "Lionel GAUTHIER: <firstname.name@eurecom.fr>"DRV_COPYRIGHT


