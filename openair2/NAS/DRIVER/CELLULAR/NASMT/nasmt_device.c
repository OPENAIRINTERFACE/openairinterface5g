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

/*! \file nasmt_device.c
* \brief Networking Device Driver for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef PDCP_USE_NETLINK
#ifdef RTAI
#include "rtai_posix.h"
#define RTAI_IRQ 30 //try to get this irq with RTAI
#endif // RTAI
#endif // PDCP_USE_NETLINK
//:::::::::::::::::::::::::::::::::::::::;;
#include "nasmt_variables.h"
#include "nasmt_proto.h"
//:::::::::::::::::::::::::::::::::::::::;;
//#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>

#ifdef NAS_DRIVER_TYPE_ETHERNET
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
#ifdef NAS_DRIVER_TYPE_ETHERNET
#include <linux/etherdevice.h>
#endif
//:::::::::::::::::::::::::::::::::::::::;;
struct net_device *gdev;
struct nas_priv *gpriv;
//int bytes_wrote;
//int bytes_read;
uint8_t NAS_NULL_IMEI[14]= {0x00, 0x00, 0x00, 0x00, 0x00 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00, 0x00, 0x00};
uint8_t NAS_RG_IMEI[14]= {0x00, 0x00, 0x00, 0x00, 0x00 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00, 0x00, 0x01};

// TEMP
//uint8_t nas_IMEI[14];
static unsigned char nas_IMEI[14];
static int m_arg=0;


#ifdef PDCP_USE_NETLINK
extern void nasmt_netlink_release(void);
extern int nasmt_netlink_init(void);
#endif
extern void nasmt_ASCTL_timer(unsigned long data);


#ifndef PDCP_USE_NETLINK
//---------------------------------------------------------------------------
//void nasmt_interrupt(void){
void *nasmt_interrupt(void)
{
  //---------------------------------------------------------------------------
  uint8_t cxi;
  char *buffer = NULL;
#ifdef NAS_DEBUG_INTERRUPT
  printk("nasmt_interrupt - begin\n");
#endif
  //spin_lock(&gpriv->lock);
  cxi=0;
  nasmt_COMMON_QOS_receive(gpriv->cx+cxi);
  nasmt_ASCTL_GC_receive(buffer);
  nasmt_ASCTL_DC_receive(gpriv->cx+cxi, buffer);
  //spin_unlock(&gpriv->lock);
#ifdef NAS_DEBUG_INTERRUPT
  printk("nasmt_interrupt: end\n");
#endif
}
#endif //NETLINK

//---------------------------------------------------------------------------
// Called by ifconfig when the device is activated by ifconfig
int nasmt_open(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  printk("nasmt_open: begin\n");

  gpriv=netdev_priv(dev);

  // Address has already been set at init
#ifndef PDCP_USE_NETLINK

  if (gpriv->irq==-EBUSY) {
    printk("nasmt_open: irq failure\n");
    return -EBUSY;
  }

#endif //NETLINK

  // next lines prevent compilation of the driver with kernel version under 2.6.29
  // ATTENTION !!!!!! NASMT is not usable with these versions

  if(!netif_queue_stopped(dev))
    netif_start_queue(dev);
  else
    netif_wake_queue(dev);

  //
  init_timer(&gpriv->timer);
  (gpriv->timer).expires=jiffies+NAS_TIMER_TICK;
  (gpriv->timer).data=0L;
  (gpriv->timer).function=nasmt_ASCTL_timer;
  // ??LITE comments: add_timer(&gpriv->timer);
  add_timer(&gpriv->timer);
  //
  printk("nasmt_open: name = %s, end\n", dev->name);
  return 0;
}

//---------------------------------------------------------------------------
// Called by ifconfig when the device is desactivated by ifconfig
int nasmt_stop(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  struct nas_priv *priv = netdev_priv(dev);
  printk("nasmt_stop: begin\n");
  del_timer(&priv->timer);
  netif_stop_queue(dev);

  printk("nasmt_stop: name = %s, end\n", dev->name);
  return 0;
}

//---------------------------------------------------------------------------
void nasmt_teardown(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  int cxi;
#ifndef PDCP_USE_NETLINK
  struct nas_priv *priv = netdev_priv(dev);
#endif //PDCP_USE_NETLINK

  printk("nasmt_teardown: begin\n");
  //  priv=(struct nas_priv *)(gdev.priv);

  if (dev) {
#ifndef PDCP_USE_NETLINK

    if (priv->irq!=-EBUSY) {
      *pt_nas_ue_irq=-1;
      rt_free_srq(priv->irq);
    }

#endif //PDCP_USE_NETLINK

#ifdef PDCP_USE_NETLINK
    nasmt_netlink_release();
#endif //PDCP_USE_NETLINK
    //  for (sapi=0; sapi<NAS_SAPI_MAX; ++sapi)
    //    close(priv->sap[sapi]);
    nasmt_CLASS_flush_rclassifier();
    cxi=0;
    nasmt_COMMON_flush_rb(gpriv->cx+cxi);
    nasmt_CLASS_flush_sclassifier(gpriv->cx+cxi);
    // for (sapi=0; sapi<NAS_SAPI_CX_MAX; ++sapi)
    // close(priv->cx[cxi].sap[sapi]);
  } // check dev
  else {
    printk("nasmt_teardown: Device is null\n");
  }

  printk("nasmt_teardown: end\n");
}

//---------------------------------------------------------------------------
int nasmt_set_config(struct net_device *dev, struct ifmap *map)
{
  //---------------------------------------------------------------------------
  printk("nasmt_set_config: begin\n");

  if (dev->flags & IFF_UP)
    return -EBUSY;

  if (map->base_addr != dev->base_addr) {
    printk(KERN_WARNING "nasmt_set_config: Can't change I/O address\n");
    return -EOPNOTSUPP;
  }

  if (map->irq != dev->irq)
    dev->irq = map->irq;

  printk("nasmt_set_config: end\n");
  return 0;
}

//---------------------------------------------------------------------------
//
int nasmt_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_DEVICE
  printk("nasmt_hard_start_xmit: begin\n");
#endif

  if ((!skb)||(!dev)) {
    printk("nasmt_hard_start_xmit - input parameter skb or dev is NULL \n");
    return -1;
  }

  // End debug information
  netif_stop_queue(dev);
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
  netif_trans_update(dev);
#else
  dev->trans_start = jiffies;
#endif
#ifdef NAS_DEBUG_SEND_DETAIL
  printk("nasmt_hard_start_xmit: step 1\n");
#endif
  nasmt_CLASS_send(skb);
#ifdef NAS_DEBUG_SEND_DETAIL
  printk("nasmt_hard_start_xmit: step 2\n");
#endif
  dev_kfree_skb(skb);
#ifdef NAS_DEBUG_SEND_DETAIL
  printk("nasmt_hard_start_xmit: step 3\n");
#endif
  netif_wake_queue(dev);
#ifdef NAS_DEBUG_DEVICE
  printk("nasmt_hard_start_xmit: end\n");
#endif
  return 0;
}

//---------------------------------------------------------------------------
struct net_device_stats *nasmt_get_stats(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  //  return &((struct nas_priv *)dev->priv)->stats;
  struct nas_priv *npriv = netdev_priv(dev);
  return &npriv->stats;
}

//---------------------------------------------------------------------------
// New function from LITE DRIVER
int nasmt_set_mac_address(struct net_device *dev, void *mac)
{
  //---------------------------------------------------------------------------
  struct sockaddr *addr = mac;
  printk("nasmt_set_mac_address: begin\n");
  memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
  return 0;
}


//---------------------------------------------------------------------------
int nasmt_change_mtu(struct net_device *dev, int mtu)
{
  //---------------------------------------------------------------------------
  printk("nasmt_change_mtu: begin\n");

  if ((mtu<50) || (mtu>1500))
    //  if ((mtu<1280) || (mtu>1500))
    return -EINVAL;

  dev->mtu = mtu;
  return 0;
}

//---------------------------------------------------------------------------
int nasmt_change_rx_flags(struct net_device *dev, int flags)
{
  //---------------------------------------------------------------------------
  //struct nas_priv *priv =  netdev_priv(dev);
  printk("nasmt_change_rx_flags %08X\n", flags);
  gpriv->rx_flags ^= flags;
  return 0;
}

//---------------------------------------------------------------------------
void nasmt_tx_timeout(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  /* Transmitter timeout, serious problems. */
  printk("nasmt_tx_timeout: begin\n");
  //((struct nas_priv *)(dev->priv))->stats.tx_errors++;
  (gpriv->stats).tx_errors++;
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
  netif_trans_update(dev);
#else
  dev->trans_start = jiffies;
#endif
  netif_wake_queue(dev);
  printk("nasmt_tx_timeout: transmit timed out %s\n",dev->name);
}

//---------------------------------------------------------------------------
// Define pointers for the module
static const struct net_device_ops nasmt_netdev_ops = {
  // ?? nasmt_interrupt
  //
  .ndo_open               = nasmt_open,
  .ndo_stop               = nasmt_stop,
  .ndo_start_xmit         = nasmt_hard_start_xmit,
  .ndo_validate_addr      = NULL,
  .ndo_get_stats          = nasmt_get_stats,
  //#ifdef  KERNEL_VERSION_GREATER_THAN_32
  //    .ndo_set_multicast_list = NULL,
  .ndo_set_mac_address    = nasmt_set_mac_address,
  .ndo_set_config         = nasmt_set_config,
  .ndo_do_ioctl           = nasmt_CTL_ioctl,
  .ndo_change_mtu         = nasmt_change_mtu,
  .ndo_tx_timeout         = nasmt_tx_timeout,
  .ndo_change_rx_flags    = nasmt_change_rx_flags,
  //#endif
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Initialisation of the network device
void nasmt_init(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  uint8_t cxi, dscpi;

  printk("nasmt_init: begin\n");

  if (dev) {
    gpriv=netdev_priv(dev);
    memset(gpriv, 0, sizeof(struct nas_priv));
    // Initialize function pointers
    dev->netdev_ops = &nasmt_netdev_ops;

#ifndef NAS_DRIVER_TYPE_ETHERNET
    //  Update driver properties
    dev->type = ARPHRD_EURUMTS;
    dev->features = NETIF_F_NO_CSUM;
    dev->hard_header_len = 0;
    dev->addr_len = NAS_ADDR_LEN;
    dev->flags = IFF_BROADCAST|IFF_MULTICAST|IFF_NOARP;
    dev->tx_queue_len = NAS_TX_QUEUE_LEN;
    dev->mtu = NAS_MTU;
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

#ifdef NAS_DRIVER_TYPE_ETHERNET
    // overwrite values written above ( header_ops,type,hard_header_len,mtu,addr_len,tx_queue_len,flags,broadcast)
    printk("\nnasmt_init: WARNING Driver type ETHERNET\n");
    ether_setup(dev);
#endif

    //
    // Initialize private structure
    gpriv->rx_flags = NAS_RESET_RX_FLAGS;

    gpriv->sap[NAS_GC_SAPI] = RRC_DEVICE_GC;
    gpriv->sap[NAS_NT_SAPI] = RRC_DEVICE_NT;
    gpriv->cx[0].sap[NAS_DC_INPUT_SAPI] = RRC_DEVICE_DC_INPUT0;
    gpriv->cx[0].sap[NAS_DC_OUTPUT_SAPI] = RRC_DEVICE_DC_OUTPUT0;

    //gpriv->sap[NAS_CO_INPUT_SAPI] = QOS_DEVICE_CONVERSATIONAL_INPUT;
    //gpriv->sap[NAS_CO_OUTPUT_SAPI] = QOS_DEVICE_CONVERSATIONAL_OUTPUT;
    gpriv->sap[NAS_DRB_INPUT_SAPI]  = PDCP2PDCP_USE_RT_FIFO;//QOS_DEVICE_CONVERSATIONAL_INPUT;
    gpriv->sap[NAS_DRB_OUTPUT_SAPI] = NAS2PDCP_FIFO;//QOS_DEVICE_STREAMING_INPUT;

    gpriv->retry_limit = NAS_RETRY_LIMIT_DEFAULT;
    gpriv->timer_establishment = NAS_TIMER_ESTABLISHMENT_DEFAULT;
    gpriv->timer_release = NAS_TIMER_RELEASE_DEFAULT;

    for (dscpi=0; dscpi<NAS_DSCP_MAX; ++dscpi)
      gpriv->rclassifier[dscpi] = NULL;

    gpriv->nrclassifier = 0;
    //
    cxi=0;
#ifdef NAS_DEBUG_DEVICE
    printk("nasmt_init: init classifiers, state and timer for MT %u\n", cxi);
#endif
    gpriv->cx[cxi].state = NAS_IDLE;
    gpriv->cx[cxi].countimer = NAS_TIMER_IDLE;
    gpriv->cx[cxi].retry = 0;
    gpriv->cx[cxi].lcr = cxi;
    gpriv->cx[cxi].rb = NULL;
    gpriv->cx[cxi].num_rb = 0;

    // initialisation of the classifiers
    for (dscpi=0; dscpi<NAS_DSCP_MAX; ++dscpi) {
      gpriv->cx[cxi].sclassifier[dscpi]=NULL;
    }

    gpriv->cx[cxi].nsclassifier=0;
    // initialisation of the IP address
    nasmt_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)gpriv->cx[cxi].iid6);
    gpriv->cx[cxi].iid4=0;
    //
    spin_lock_init(&gpriv->lock);
    printk("nasmt_init: init IMEI to IID\n");

#ifdef NAS_DRIVER_TYPE_ETHERNET
    nasmt_TOOL_eth_imei2iid(nas_IMEI, dev->dev_addr ,(uint8_t *)gpriv->cx[0].iid6, dev->addr_len);
#else
    nasmt_TOOL_imei2iid(nas_IMEI, dev->dev_addr);// IMEI to device address (for stateless autoconfiguration address)
    nasmt_TOOL_imei2iid(nas_IMEI, (uint8_t *)gpriv->cx[0].iid6);
#endif

    nasmt_ASCTL_init();
  } else {
    printk("\n\nnasmt_init: ERROR, Device is NULL!!\n");
  }

  printk("nasmt_init: end\n");
  return;
}

//---------------------------------------------------------------------------
int init_module (void)
{
  //---------------------------------------------------------------------------
  int err;
  int inst = 0;
  int index;
  struct nas_priv *priv;
  char devicename[100];

  printk("\n\n\ninit_module: begin \n");

  // check IMEI parameter
  printk("number of IMEI parameters %d, IMEI ", m_arg);

  for (index = 0; index < m_arg;  index++) {
    printk("%02X ", nas_IMEI[index]);
  }

  printk("\n");


#ifndef PDCP_USE_NETLINK

  // Initialize parameters shared with RRC (done first to avoid going further)
  if (pt_nas_ue_irq==NULL) {
    printk("init_module: shared irq parameter not initialised\n");
    err =  -EBUSY;
    printk("init_module: returning %d \n\n", err);
    return err;
  }

  printk("init_module: pt_nas_ue_irq valid \n");
#endif

  // Allocate device structure
  sprintf(devicename,"oai%d",inst);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
  gdev = alloc_netdev(sizeof(struct nas_priv), devicename, nasmt_init);
#else
  gdev = alloc_netdev(sizeof(struct nas_priv), devicename, NET_NAME_PREDICTABLE, nasmt_init);
#endif
  priv = netdev_priv(gdev);
  ////
#ifndef PDCP_USE_NETLINK
  priv->irq=rt_request_srq(0, nasmt_interrupt, NULL);

  if (priv->irq == -EBUSY || priv->irq == -EINVAL) {
    printk("\n init_module: No interrupt resource available\n");

    if (gdev) {
      free_netdev(gdev);
      printk("init_module: free_netdev ..\n");
    }

    return -EBUSY;
  } else
    printk("init_module: Interrupt %d, ret = %d \n", priv->irq , ret);

  if (pt_nas_ue_irq==NULL) {
    printk("init_module: shared irq parameter has been reset\n");
  } else {
    *pt_nas_ue_irq=priv->irq;
  }

#endif

#ifdef PDCP_USE_NETLINK

  if ((err=nasmt_netlink_init()) == -1)
    printk("init_module: NETLINK failed\n");

  printk("init_module: NETLINK INIT successful\n");
#endif //NETLINK
  //
  err= register_netdev(gdev);

  if (err) {
    printk("init_module: error %i registering device %s\n", err, gdev->name);
  } else {
    printk("init_module: registering device %s, ifindex = %d\n\n",gdev->name, gdev->ifindex);
  }

  return err;
}

//---------------------------------------------------------------------------
void cleanup_module(void)
{
  //---------------------------------------------------------------------------
  printk("nasmt_cleanup_module: begin\n");
  unregister_netdev(gdev);
  nasmt_teardown(gdev);
  free_netdev(gdev);
  printk("nasmt_cleanup_module: end\n\n\n\n");
}

//---------------------------------------------------------------------------
// Replaced by init_module and cleanup_module
//module_init (nasmt_init_module);
//module_exit (nasmt_cleanup_module);
//---------------------------------------------------------------------------

#define DRV_NAME        "oai_nasmt"
#define DRV_VERSION     "3.0.1"DRV_NAME
#define DRV_DESCRIPTION "OPENAIR CELLULAR LTE NASMT Device Driver"
#define DRV_COPYRIGHT   "-Copyright(c) GNU GPL Eurecom 2013"
#define DRV_AUTHOR      "Michelle Wetterwald <michelle.wetterwald@eurecom.fr>"DRV_COPYRIGHT

module_param_array_named(nas_IMEI,nas_IMEI,byte,&m_arg,0);
//module_param_array(oai_nw_drv_IMEI,byte,&m_arg,0444);
MODULE_PARM_DESC(nas_IMEI,"Terminal IMEI Identifier (14 digits, only first 10 significant if ETH option ON)");


// MODULE_LICENSE("GPL");
// MODULE_DESCRIPTION("LTE Driver for Mobile Terminal, playing as Non Access Stratum");
// MODULE_AUTHOR("Michelle Wetterwald <michelle.wetterwald@eurecom.fr>");

