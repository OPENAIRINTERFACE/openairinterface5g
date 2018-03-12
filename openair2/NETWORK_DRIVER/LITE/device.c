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
* \brief Networking Device Driver for OpenAirInterface Ethernet
* \author  navid.nikaein,  Lionel Gauthier, raymond.knopp,
* \company Eurecom
* \email:navid.nikaein@eurecom.fr, lionel.gauthier@eurecom.fr,  raymond.knopp@eurecom.fr

*/
/*******************************************************************************/

#include "constant.h"
#include "local.h"
#include "proto_extern.h"

//#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
#include <linux/if_ether.h>
#endif
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/delay.h>
#include <asm/unistd.h>
#include <linux/netdevice.h>
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
#include <linux/etherdevice.h>
#endif


struct net_device *oai_nw_drv_dev[OAI_NW_DRV_NB_INSTANCES_MAX];

#ifdef OAI_NW_DRIVER_USE_NETLINK
extern void oai_nw_drv_netlink_release(void);
extern int oai_nw_drv_netlink_init(void);
#endif

uint8_t NULL_IMEI[14]= {0x05, 0x04, 0x03, 0x01, 0x02 ,0x00, 0x00, 0x00, 0x05, 0x04, 0x03 ,0x00, 0x01, 0x08};

static unsigned char oai_nw_drv_IMEI[14];
static int           m_arg=0;
static unsigned int  oai_nw_drv_is_clusterhead=0;
//---------------------------------------------------------------------------
int oai_nw_drv_find_inst(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  int i;

  for (i=0; i<OAI_NW_DRV_NB_INSTANCES_MAX; i++)
    if (oai_nw_drv_dev[i] == dev)
      return(i);

  return(-1);
}

//---------------------------------------------------------------------------

#ifndef OAI_NW_DRIVER_USE_NETLINK
void *oai_nw_drv_interrupt(void)
{
  //---------------------------------------------------------------------------
  uint8_t cxi;

  //  struct oai_nw_drv_priv *priv=netdev_priv(dev_id);
  //  unsigned int flags;
  //  priv->lock = SPIN_LOCK_UNLOCKED;

#ifdef OAI_DRV_DEBUG_INTERRUPT
  printk("INTERRUPT - begin\n");
#endif
  //  spin_lock_irqsave(&priv->lock,flags);
  cxi=0;
  //    mesh_GC_receive();
  //    mesh_DC_receive(naspriv->cx+cxi);
#ifndef OAI_NW_DRIVER_USE_NETLINK
  oai_nw_drv_common_wireless2ip();
#endif
  //  spin_unlock_irqrestore(&priv->lock,flags);
#ifdef OAI_DRV_DEBUG_INTERRUPT
  printk("INTERRUPT: end\n");
#endif
  //  return 0;
}
#endif //NETLINK
//---------------------------------------------------------------------------
void oai_nw_drv_timer(unsigned long data)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_priv *priv=(struct oai_nw_drv_priv *)data;
  spin_lock(&priv->lock);
  (priv->timer).function=oai_nw_drv_timer;
  (priv->timer).expires=jiffies+OAI_NW_DRV_TIMER_TICK;
  (priv->timer).data=data;
  add_timer(&priv->timer);
  spin_unlock(&priv->lock);
  return;
  //  add_timer(&gpriv->timer);
  //  spin_unlock(&gpriv->lock);
}

//---------------------------------------------------------------------------
// Called by ifconfig when the device is activated by ifconfig
int oai_nw_drv_open(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_priv *priv=netdev_priv(dev);

  // Address has already been set at init
#ifndef OAI_NW_DRIVER_USE_NETLINK

  if (pdcp_2_oai_nw_drv_irq==-EBUSY) {
    printk("[OAI_IP_DRV][%s] : irq failure\n", __FUNCTION__);
    return -EBUSY;
  }

#endif //OAI_NW_DRIVER_USE_NETLINK

  if(!netif_queue_stopped(dev))
    netif_start_queue(dev);
  else
    netif_wake_queue(dev);

  init_timer(&priv->timer);
  (priv->timer).expires   = jiffies+OAI_NW_DRV_TIMER_TICK;
  (priv->timer).data      = (unsigned long)priv;
  (priv->timer).function  = oai_nw_drv_timer;
  //add_timer(&priv->timer);

  printk("[OAI_IP_DRV][%s] name = %s\n", __FUNCTION__, dev->name);
  return 0;
}

//---------------------------------------------------------------------------
// Called by ifconfig when the device is desactivated
int oai_nw_drv_stop(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_priv *priv = netdev_priv(dev);

  printk("[OAI_IP_DRV][%s] Begin\n", __FUNCTION__);
  del_timer(&(priv->timer));
  netif_stop_queue(dev);
  //    MOD_DEC_USE_COUNT;
  printk("[OAI_IP_DRV][%s] End\n", __FUNCTION__);
  return 0;
}

//---------------------------------------------------------------------------
void oai_nw_drv_teardown(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_priv *priv;
  int              inst;

  printk("[OAI_IP_DRV][%s] Begin\n", __FUNCTION__);

  if (dev) {
    priv = netdev_priv(dev);
    inst = oai_nw_drv_find_inst(dev);

    if ((inst<0) || (inst>=OAI_NW_DRV_NB_INSTANCES_MAX)) {
      printk("[OAI_IP_DRV][%s] ERROR, couldn't find instance\n", __FUNCTION__);
      return;
    }

    /*oai_nw_drv_class_flush_recv_classifier(priv);

    for (cxi=0;cxi<OAI_NW_DRV_CX_MAX;cxi++) {
        oai_nw_drv_common_flush_rb(priv->cx+cxi);
        oai_nw_drv_class_flush_send_classifier(priv->cx+cxi);
    }*/
    printk("[OAI_IP_DRV][%s] End\n", __FUNCTION__);
  } // check dev
  else {
    printk("[OAI_IP_DRV][%s] Device is null\n", __FUNCTION__);
  }
}
//---------------------------------------------------------------------------
int oai_nw_drv_set_config(struct net_device *dev, struct ifmap *map)
{
  //---------------------------------------------------------------------------
  printk("[OAI_IP_DRV][%s] Begin\n", __FUNCTION__);

  if (dev->flags & IFF_UP)
    return -EBUSY;

  if (map->base_addr != dev->base_addr) {
    printk(KERN_WARNING "[OAI_IP_DRV][%s] Can't change I/O address\n", __FUNCTION__);
    return -EOPNOTSUPP;
  }

  if (map->irq != dev->irq)
    dev->irq = map->irq;

  printk("[OAI_IP_DRV][%s] End\n", __FUNCTION__);
  return 0;
}

//---------------------------------------------------------------------------
//
int oai_nw_drv_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
  //---------------------------------------------------------------------------
  int inst;

  if (dev) {
    inst = oai_nw_drv_find_inst(dev);
  } else {
    printk("[OAI_IP_DRV][%s] ERROR, device is null\n", __FUNCTION__);
    return -1;
  }

  if ((inst>=0) && (inst<OAI_NW_DRV_NB_INSTANCES_MAX)) {
#ifdef OAI_DRV_OAI_DRV_DEBUG_DEVICE
    printk("[OAI_IP_DRV][%s] inst %d,  begin\n", __FUNCTION__,inst);
#endif

    if (!skb) {
      printk("[OAI_IP_DRV][%s] input parameter skb is NULL\n", __FUNCTION__);
      return -1;
    }

    // End debug information
    netif_stop_queue(dev);
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
    netif_trans_update(dev);
#else
    dev->trans_start = jiffies;
#endif
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[OAI_IP_DRV][%s] step 1\n", __FUNCTION__);
#endif
    oai_nw_drv_common_ip2wireless(skb,inst);
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[OAI_IP_DRV][%s] step 2\n", __FUNCTION__);
#endif
    dev_kfree_skb(skb);
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[OAI_IP_DRV][%s] step 3\n", __FUNCTION__);
#endif
    netif_wake_queue(dev);
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("[OAI_IP_DRV][%s] end\n", __FUNCTION__);
#endif
  } else {
    printk("[OAI_IP_DRV][%s] ERROR, couldn't find instance\n", __FUNCTION__);
    return(-1);
  }

  return 0;
}

//---------------------------------------------------------------------------
struct net_device_stats *oai_nw_drv_get_stats(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  //    return &((struct oai_nw_drv_priv *)dev->priv)->stats;
  struct oai_nw_drv_priv *priv = netdev_priv(dev);
  //printk("[OAI_IP_DRV][%s]\n", __FUNCTION__);
  return &priv->stats;
}
//---------------------------------------------------------------------------
int oai_nw_drv_set_mac_address(struct net_device *dev, void *mac)
{
  //---------------------------------------------------------------------------
  struct sockaddr *addr = mac;
  printk("[OAI_IP_DRV][%s] CHANGE MAC ADDRESS\n", __FUNCTION__);
  memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
  return 0;
}
//---------------------------------------------------------------------------
int oai_nw_drv_change_mtu(struct net_device *dev, int mtu)
{
  //---------------------------------------------------------------------------
  printk("[OAI_IP_DRV][%s] CHANGE MTU %d bytes\n", __FUNCTION__, mtu);

  if ((mtu<50) || (mtu>1500))
    return -EINVAL;

  dev->mtu = mtu;
  return 0;
}
//---------------------------------------------------------------------------
void oai_nw_drv_change_rx_flags(struct net_device *dev, int flags)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_priv *priv =  netdev_priv(dev);
  printk("[OAI_IP_DRV][%s] CHANGE RX FLAGS %08X\n", __FUNCTION__, flags);
  priv->rx_flags ^= flags;
}

//---------------------------------------------------------------------------
void oai_nw_drv_tx_timeout(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  // Transmitter timeout, serious problems.
  struct oai_nw_drv_priv *priv =  netdev_priv(dev);

  printk("[OAI_IP_DRV][%s] begin\n", __FUNCTION__);
  //  (struct oai_nw_drv_priv *)(dev->priv)->stats.tx_errors++;
  (priv->stats).tx_errors++;
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
  netif_trans_update(dev);
#else
  dev->trans_start = jiffies;
#endif
  netif_wake_queue(dev);
  printk("[OAI_IP_DRV][%s] transmit timed out %s\n", __FUNCTION__,dev->name);
}

static const struct net_device_ops nasmesh_netdev_ops = {
  .ndo_open               = oai_nw_drv_open,
  .ndo_stop               = oai_nw_drv_stop,
  .ndo_start_xmit         = oai_nw_drv_hard_start_xmit,
  .ndo_validate_addr      = NULL,
  .ndo_get_stats          = oai_nw_drv_get_stats,
  .ndo_set_mac_address    = oai_nw_drv_set_mac_address,
  .ndo_set_config         = oai_nw_drv_set_config,
  .ndo_do_ioctl           = oai_nw_drv_CTL_ioctl,
  .ndo_change_mtu         = oai_nw_drv_change_mtu,
  .ndo_tx_timeout         = oai_nw_drv_tx_timeout,
  .ndo_change_rx_flags    = oai_nw_drv_change_rx_flags,
};
/*.ndo_set_multicast_list = NULL,*/

//---------------------------------------------------------------------------
// Initialisation of the network device
void oai_nw_drv_init(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  uint8_t               cxi;
  struct oai_nw_drv_priv *priv;
  int              index;

  if (dev) {

    priv = netdev_priv(dev);

    memset(priv, 0, sizeof(struct oai_nw_drv_priv));


#ifndef OAI_NW_DRIVER_TYPE_ETHERNET
    dev->type              = ARPHRD_EURECOM_LTE;
    dev->features          = NETIF_F_NO_CSUM;
    dev->hard_header_len   = 0;
    dev->addr_len          = ETH_ALEN; // 6
    dev->flags             = IFF_BROADCAST | IFF_MULTICAST | IFF_NOARP;
    dev->tx_queue_len      = NAS_TX_QUEUE_LEN;
    dev->mtu               = NAS_MTU;
#endif
    // Can be one of the following enum defined in include/linux/netdevice.h:
    // enum netdev_state_t {
    // __LINK_STATE_START,
    // __LINK_STATE_PRESENT,
    // __LINK_STATE_NOCARRIER,
    // __LINK_STATE_LINKWATCH_PENDING,
    // __LINK_STATE_DORMANT,
    // };
    set_bit(__LINK_STATE_PRESENT, &dev->state);

    //
    dev->netdev_ops = &nasmesh_netdev_ops;
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
    printk("[OAI_IP_DRV][%s] Driver type ETHERNET\n", __FUNCTION__);
    // Memo: linux-source-3.2.0/net/ethernet/eth.c:
    //334 void ether_setup(struct net_device *dev)
    //335 {
    //336         dev->header_ops         = &eth_header_ops;
    //337         dev->type               = ARPHRD_ETHER;
    //338         dev->hard_header_len    = ETH_HLEN;
    //339         dev->mtu                = ETH_DATA_LEN;
    //340         dev->addr_len           = ETH_ALEN;
    //341         dev->tx_queue_len       = 1000; /* Ethernet wants good queues */
    //342         dev->flags              = IFF_BROADCAST|IFF_MULTICAST;
    //343         dev->priv_flags         |= IFF_TX_SKB_SHARING;
    //344
    //345         memset(dev->broadcast, 0xFF, ETH_ALEN);
    //346 }
    ether_setup(dev);
#endif
    //
    // Initialize private structure
    priv->rx_flags                 = OAI_NW_DRV_RESET_RX_FLAGS;
    priv->sap[OAI_NW_DRV_RAB_INPUT_SAPI]  = PDCP2IP_FIFO;//QOS_DEVICE_CONVERSATIONAL_INPUT;
    priv->sap[OAI_NW_DRV_RAB_OUTPUT_SAPI] = IP2PDCP_FIFO;//QOS_DEVICE_STREAMING_INPUT;

    //  priv->retry_limit=RETRY_LIMIT_DEFAULT;
    //  priv->timer_establishment=TIMER_ESTABLISHMENT_DEFAULT;
    //  priv->timer_release=TIMER_RELEASE_DEFAULT;

    /*for (dscpi=0; dscpi<OAI_NW_DRV_DSCP_MAX; ++dscpi) {
        priv->rclassifier[dscpi]=NULL;
    }
    priv->nrclassifier=0;*/
    //
    for (cxi=0; cxi<OAI_NW_DRV_CX_MAX; cxi++) {
#ifdef OAI_DRV_DEBUG_DEVICE
      printk("[OAI_IP_DRV][%s] init classifiers, state and timer for MT %u\n", __FUNCTION__, cxi);
#endif

      //    priv->cx[cxi].sap[NAS_DC_INPUT_SAPI] = RRC_DEVICE_DC_INPUT0;
      //    priv->cx[cxi].sap[NAS_DC_OUTPUT_SAPI] = RRC_DEVICE_DC_OUTPUT0;
      priv->cx[cxi].state      = OAI_NW_DRV_IDLE;
      priv->cx[cxi].countimer  = OAI_NW_DRV_TIMER_IDLE;
      priv->cx[cxi].retry      = 0;
      priv->cx[cxi].lcr        = cxi;
      /*priv->cx[cxi].rb         = NULL;
      priv->cx[cxi].num_rb     = 0;
      // initialisation of the classifier
      for (dscpi=0; dscpi<65; ++dscpi) {
          priv->cx[cxi].sclassifier[dscpi] = NULL;
          priv->cx[cxi].fclassifier[dscpi] = NULL;
      }

      priv->cx[cxi].nsclassifier=0;
      priv->cx[cxi].nfclassifier=0;
      */
      // initialisation of the IP address
      oai_nw_drv_TOOL_eNB_imei2iid(oai_nw_drv_IMEI, (uint8_t *)priv->cx[cxi].iid6, dev->addr_len);
      priv->cx[cxi].iid4=0;
      //
    }

    spin_lock_init(&priv->lock);
    //this requires kernel patch for OAI driver: typically for RF/hard realtime emu experimentation
#define ADDRCONF
#ifdef ADDRCONF

#ifdef OAI_NW_DRIVER_USE_NETLINK
    oai_nw_drv_TOOL_eNB_imei2iid(oai_nw_drv_IMEI, dev->dev_addr, dev->addr_len);// IMEI to device address (for stateless autoconfiguration address)

    oai_nw_drv_TOOL_eNB_imei2iid(oai_nw_drv_IMEI, (uint8_t *)priv->cx[0].iid6, dev->addr_len);
#else
    oai_nw_drv_TOOL_imei2iid(oai_nw_drv_IMEI, dev->dev_addr);// IMEI to device address (for stateless autoconfiguration address)
    oai_nw_drv_TOOL_imei2iid(oai_nw_drv_IMEI, (uint8_t *)priv->cx[0].iid6);
#endif
    // this is more appropriate for user space soft realtime emulation
#else

    // LG: strange
    for (index = 0; index < dev->addr_len;  index++) {
      dev->dev_addr[index] = 16*oai_nw_drv_IMEI[index]+oai_nw_drv_IMEI[index+1]);
    }

    memcpy((uint8_t *)priv->cx[0].iid6,&oai_nw_drv_IMEI[0],dev->addr_len);

    printk("INIT: init IMEI to IID\n");
#endif
    printk("[OAI_IP_DRV][%s] Setting HW addr to : ", __FUNCTION__);

    for (index = 0; index < dev->addr_len;  index++) {
      printk("%02X", dev->dev_addr[index]);
    }

    printk("\n[OAI_IP_DRV][%s] Setting priv->cx to : ", __FUNCTION__);

    for (index = 0; index < 8;  index++) {
      printk("%02X", ((uint8_t *)(priv->cx[0].iid6))[index]);
    }

    printk("\n");
    printk("[OAI_IP_DRV][%s] INIT: end\n", __FUNCTION__);
    return;
  } else {
    printk("[OAI_IP_DRV][%s] ERROR, Device is NULL!!\n", __FUNCTION__);
    return;
  }
}
//---------------------------------------------------------------------------
int init_module (void)
{
  //---------------------------------------------------------------------------
  int err,inst, index;
  struct oai_nw_drv_priv *priv;
  char devicename[100];


  // Initialize parameters shared with RRC
  printk("[OAI_IP_DRV][%s] Starting NASMESH, number of IMEI parameters %d, IMEI ", __FUNCTION__, m_arg);

  for (index = 0; index < m_arg;  index++) {
    printk("%02X", oai_nw_drv_IMEI[index]);
  }

  printk("\n");

#ifndef OAI_NW_DRIVER_USE_NETLINK

  if (pdcp_2_oai_nw_drv_irq == -EBUSY || pdcp_2_oai_nw_drv_irq == -EINVAL) {
    printk("[OAI_IP_DRV][%s] No interrupt resource available\n", __FUNCTION__);
    return -EBUSY;
  } else
    printk("[OAI_IP_DRV][%s] Interrupt %d\n", __FUNCTION__, pdcp_2_oai_nw_drv_irq);

#endif //NETLINK

  for (inst=0; inst<OAI_NW_DRV_NB_INSTANCES_MAX; inst++) {
    printk("[OAI_IP_DRV][%s] begin init instance %d\n", __FUNCTION__,inst);


    sprintf(devicename,"oai%d",inst);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
    oai_nw_drv_dev[inst]  = alloc_netdev(sizeof(struct oai_nw_drv_priv),devicename, oai_nw_drv_init);
#else
    oai_nw_drv_dev[inst]  = alloc_netdev(sizeof(struct oai_nw_drv_priv),devicename, NET_NAME_PREDICTABLE, oai_nw_drv_init);
#endif
    //netif_stop_queue(oai_nw_drv_dev[inst]);

    if (oai_nw_drv_dev[inst] == NULL) {
      printk("[OAI_IP_DRV][%s][INST %02d] alloc_netdev FAILED\n", __FUNCTION__,inst);
    } else {

      priv = netdev_priv(oai_nw_drv_dev[inst]);

      if (oai_nw_drv_dev[inst]) {
        oai_nw_drv_IMEI[9] += 1;

        if (oai_nw_drv_IMEI[9] > 0x0F) {
          oai_nw_drv_IMEI[8] = oai_nw_drv_IMEI[9] >> 4;
          oai_nw_drv_IMEI[9] = oai_nw_drv_IMEI[9] & 0x0F;
        }
      }

      // linux/net/core/dev.c line 4767
      err= register_netdev(oai_nw_drv_dev[inst]);

      if (err) {
        printk("[OAI_IP_DRV][%s] (inst %d): error %i registering device %s\n", __FUNCTION__, inst,err, oai_nw_drv_dev[inst]->name);
      } else {
        printk("[OAI_IP_DRV][%s] registering device %s, ifindex = %d\n\n", __FUNCTION__,oai_nw_drv_dev[inst]->name, oai_nw_drv_dev[inst]->ifindex);
      }
    }
  }

#ifdef OAI_NW_DRIVER_USE_NETLINK
  printk("[OAI_IP_DRV][%s] NETLINK INIT\n", __FUNCTION__);

  if ((err=oai_nw_drv_netlink_init()) == -1)
    printk("[OAI_IP_DRV][%s] NETLINK failed\n", __FUNCTION__);

#endif //NETLINK

  return err;

}

//---------------------------------------------------------------------------
void cleanup_module(void)
{
  //---------------------------------------------------------------------------
  int inst;

  printk("[OAI_IP_DRV][CLEANUP]nasmesh_cleanup_module: begin\n");

#ifndef OAI_NW_DRIVER_USE_NETLINK

  if (pdcp_2_oai_nw_drv_irq!=-EBUSY) {
    pdcp_2_oai_nw_drv_irq=0;
    // Start IRQ linux
    //free_irq(priv->irq, NULL);
    // End IRQ linux
  }

#else // NETLINK
#endif //NETLINK

  for (inst=0; inst<OAI_NW_DRV_NB_INSTANCES_MAX; inst++) {
#ifdef OAI_DRV_DEBUG_DEVICE
    printk("nasmesh_cleanup_module: unregister and free net device instance %d\n",inst);
#endif

    if (oai_nw_drv_dev[inst]) {
      unregister_netdev(oai_nw_drv_dev[inst]);
      oai_nw_drv_teardown(oai_nw_drv_dev[inst]);
      free_netdev(oai_nw_drv_dev[inst]);
    }
  }

#ifdef OAI_NW_DRIVER_USE_NETLINK
  oai_nw_drv_netlink_release();
#endif //OAI_NW_DRIVER_USE_NETLINK
  printk("nasmesh_cleanup_module: end\n");
}

#define DRV_NAME        "NASMESH"
#define DRV_VERSION     "3.0.2"DRV_NAME
#define DRV_DESCRIPTION "OPENAIR MESH Device Driver"
#define DRV_COPYRIGHT   "-Copyright(c) GNU GPL Eurecom 2009"
#define DRV_AUTHOR      "Raymond Knopp, and Navid Nikaein: <firstname.name@eurecom.fr>"DRV_COPYRIGHT


module_param_array_named(oai_nw_drv_IMEI,oai_nw_drv_IMEI,byte,&m_arg,0);
//module_param_array(oai_nw_drv_IMEI,byte,&m_arg,0444);
MODULE_PARM_DESC(oai_nw_drv_IMEI,"The IMEI Hardware address (64-bit, decimal nibbles)");

//module_param(oai_nw_drv_is_clusterhead,uint,0444);
module_param_named(oai_nw_drv_is_clusterhead,oai_nw_drv_is_clusterhead,uint,0444);
MODULE_PARM_DESC(oai_nw_drv_is_clusterhead,"The Clusterhead Indicator");

//MODULE_AUTHOR(DRV_AUTHOR);
//MODULE_DESCRIPTION(DRV_DESCRIPTION);
//MODULE_LICENSE("GPL");
//MODULE_VERSION(DRV_VERSION);

