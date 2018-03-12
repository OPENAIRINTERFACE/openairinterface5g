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
* \brief Networking Device Driver for OpenAirInterface MESH
* \author  navid.nikaein, yan.moret(no longer valid), michelle.wetterwald, raymond.knopp
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr, michelle.wetterwald@eurecom.fr,

*/
/*******************************************************************************/

#include "constant.h"
#include "local.h"
#include "proto_extern.h"

#include <linux/module.h>
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


//#define DEBUG_DEVICE 1
//#define DEBUG_INTERRUPT 1

struct net_device *nasdev[NB_INSTANCES_MAX];

#ifdef PDCP_USE_NETLINK
extern void nas_netlink_release(void);
extern int nas_netlink_init(void);
#endif

//int bytes_wrote;
//int bytes_read;
uint8_t NULL_IMEI[14]= {0x05, 0x04, 0x03, 0x01, 0x02 ,0x00, 0x00, 0x00, 0x05, 0x04, 0x03 ,0x00, 0x01, 0x08};


static unsigned int  nas_IMEI[6]= {0x03, 0x01, 0x02 ,0x00, 0x00, 0x00}; // may change to char
static int m_arg=0;
static unsigned int nas_is_clusterhead=0;


int find_inst(struct net_device *dev)
{

  int i;

  for (i=0; i<NB_INSTANCES_MAX; i++)
    if (nasdev[i] == dev)
      return(i);

  return(-1);
}

//---------------------------------------------------------------------------

#ifndef PDCP_USE_NETLINK
//void interrupt(void){
void *nas_interrupt(void)
{
  //---------------------------------------------------------------------------
  uint8_t cxi;

  //  struct nas_priv *priv=netdev_priv(dev_id);
  //  unsigned int flags;

  //  priv->lock = SPIN_LOCK_UNLOCKED;

#ifdef DEBUG_INTERRUPT
  printk("INTERRUPT - begin\n");
#endif
  //  spin_lock_irqsave(&priv->lock,flags);
  cxi=0;
  //  mesh_GC_receive();
  //  mesh_DC_receive(naspriv->cx+cxi);
#ifndef PDCP_USE_NETLINK
  nas_COMMON_QOS_receive();
#endif
  //  spin_unlock_irqrestore(&priv->lock,flags);
#ifdef DEBUG_INTERRUPT
  printk("INTERRUPT: end\n");
#endif
  //  return 0;
  return NULL;
}
#endif //NETLINK

//---------------------------------------------------------------------------
// Called by ifconfig when the device is activated by ifconfig
int nas_open(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  printk("OPEN: begin\n");
  //  MOD_INC_USE_COUNT;
  // Address has already been set at init
#ifndef PDCP_USE_NETLINK

  if (pdcp_2_nas_irq==-EBUSY) {
    printk("OPEN: irq failure\n");
    return -EBUSY;
  }

#endif //PDCP_USE_NETLINK

  /*
  netif_start_queue(dev);
  //
  init_timer(&priv->timer);
  (priv->timer).expires=jiffies+NAS_TIMER_TICK;
  (priv->timer).data=0L;
  (priv->timer).function=nas_mesh_timer;
  //  add_timer(&priv->timer);
  //
  */
  printk("OPEN: name = %s, end\n", dev->name);
  return 0;
}

//---------------------------------------------------------------------------
// Called by ifconfig when the device is desactivated
int nas_stop(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  struct nas_priv *priv = netdev_priv(dev);
  printk("STOP: begin\n");
  del_timer(&(priv->timer));
  netif_stop_queue(dev);
  //  MOD_DEC_USE_COUNT;
  printk("STOP: name = %s, end\n", dev->name);
  return 0;
}

//---------------------------------------------------------------------------
void nas_teardown(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  int cxi;

  struct nas_priv *priv;
  int inst;

  if (dev) {
    priv = netdev_priv(dev);
    inst = find_inst(dev);

    if (inst<0) {
      printk("nas_teardown: ERROR, couldn't find instance\n");
    }

    printk("nas_teardown instance %d: begin\n",inst);



    nas_CLASS_flush_rclassifier(priv);

    for (cxi=0; cxi<NAS_CX_MAX; cxi++) {
      nas_COMMON_flush_rb(priv->cx+cxi);
      nas_CLASS_flush_sclassifier(priv->cx+cxi);
    }

    printk("nas_teardown: end\n");
  } // check dev
  else {
    printk("nas_teardown: Device is null\n");
  }
}

//---------------------------------------------------------------------------
int nas_set_config(struct net_device *dev, struct ifmap *map)
{
  //---------------------------------------------------------------------------
  printk("SET_CONFIG: begin\n");

  if (dev->flags & IFF_UP)
    return -EBUSY;

  if (map->base_addr != dev->base_addr) {
    printk(KERN_WARNING "SET_CONFIG: Can't change I/O address\n");
    return -EOPNOTSUPP;
  }

  if (map->irq != dev->irq)
    dev->irq = map->irq;

  return 0;
}

//---------------------------------------------------------------------------
//
int nas_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
  //---------------------------------------------------------------------------
  // Start debug information

  int inst;

  if (dev)
    inst = find_inst(dev);
  else {
    printk("nas_hard_start_xmit: ERROR, device is null\n");
    return -1;
  }

  if (inst>=0) {
#ifdef DEBUG_DEVICE
    printk("HARD_START_XMIT: inst %d,  begin\n",inst);
#endif

    if (!skb) {
      printk("HARD_START_XMIT - input parameter skb is NULL \n");
      return -1;
    }

    // End debug information
    netif_stop_queue(dev);
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0) || RHEL_RELEASE_CODE>=1796
    netif_trans_update(dev);
#else
    dev->trans_start = jiffies;
#endif
#ifdef DEBUG_DEVICE
    printk("HARD_START_XMIT: step 1\n");
#endif
    nas_CLASS_send(skb,inst);
#ifdef DEBUG_DEVICE
    printk("HARD_START_XMIT: step 2\n");
#endif
    //  if (skb==NULL){
    //    printk("HARD_START_XMIT - parameter skb is NULL \n");
    //    return -1;
    //  }else
    dev_kfree_skb(skb);
#ifdef DEBUG_DEVICE
    printk("HARD_START_XMIT: step 3\n");
#endif
    netif_wake_queue(dev);
#ifdef DEBUG_DEVICE
    printk("HARD_START_XMIT: end\n");
#endif
  } else {
    printk("nas_hard_start_xmit: ERROR, couldn't find instnace\n");
    return(-1);
  }

  return 0;
}

//---------------------------------------------------------------------------
struct net_device_stats *nas_get_stats(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  //  return &((struct nas_priv *)dev->priv)->stats;
  struct nas_priv *priv = netdev_priv(dev);
  return &priv->stats;
}

//---------------------------------------------------------------------------
int nas_change_mtu(struct net_device *dev, int mtu)
{
  //---------------------------------------------------------------------------

  printk("CHANGE_MTU: begin\n");

  if ((mtu<50) || (mtu>1500))
    return -EINVAL;

  dev->mtu = mtu;
  return 0;
}

//---------------------------------------------------------------------------
void nas_tx_timeout(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  // Transmitter timeout, serious problems.
  struct nas_priv *priv =  netdev_priv(dev);

  printk("TX_TIMEOUT: begin\n");
  //  (struct nas_priv *)(dev->priv)->stats.tx_errors++;
  (priv->stats).tx_errors++;
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0) || RHEL_RELEASE_CODE>=1796
  netif_trans_update(dev);
#else
  dev->trans_start = jiffies;
#endif
  netif_wake_queue(dev);
  printk("TX_TIMEOUT: transmit timed out %s\n",dev->name);
}

static const struct net_device_ops nasmesh_netdev_ops = {

  .ndo_open   = nas_open,
  .ndo_stop   = nas_stop,
  .ndo_start_xmit   = nas_hard_start_xmit,
  .ndo_validate_addr  = NULL,
  .ndo_set_mac_address  = NULL,
  .ndo_set_config     = nas_set_config,
  .ndo_do_ioctl       = nas_CTL_ioctl,
  .ndo_change_mtu   = nas_change_mtu,
  .ndo_tx_timeout   = nas_tx_timeout,
};

//---------------------------------------------------------------------------
// Initialisation of the network device
void nas_init(struct net_device *dev)
{
  //---------------------------------------------------------------------------
  uint8_t cxi, dscpi;
  struct nas_priv *priv;
  //  int inst;

  if (dev) {
    priv = netdev_priv(dev);
    //memset(dev->priv, 0, sizeof(struct nas_priv));
    memset(priv, 0, sizeof(struct nas_priv));
    //  priv=(struct nas_priv *)(dev->priv);
    priv=netdev_priv(dev);
    //
    dev->netdev_ops = &nasmesh_netdev_ops;
    // dev->type = ARPHRD_EUROPENAIRMESH;
    //dev->type = ARPHRD_ETHER;
    //  dev->features = NETIF_F_NO_CSUM;
    dev->hard_header_len = 0;
    dev->addr_len = NAS_ADDR_LEN;
    dev->flags = IFF_BROADCAST|IFF_MULTICAST|IFF_NOARP;
    dev->tx_queue_len = NAS_TX_QUEUE_LEN;
    dev->mtu = NAS_MTU;
    //
    // Initialize private structure
    //  priv->sap[NAS_GC_SAPI] = RRC_DEVICE_GC;
    //  priv->sap[NAS_NT_SAPI] = RRC_DEVICE_NT;
    priv->sap[NAS_RAB_INPUT_SAPI] = PDCP2PDCP_USE_RT_FIFO;//QOS_DEVICE_CONVERSATIONAL_INPUT;
    priv->sap[NAS_RAB_OUTPUT_SAPI] = NAS2PDCP_FIFO;//QOS_DEVICE_STREAMING_INPUT;

    //  priv->retry_limit=RETRY_LIMIT_DEFAULT;
    //  priv->timer_establishment=TIMER_ESTABLISHMENT_DEFAULT;
    //  priv->timer_release=TIMER_RELEASE_DEFAULT;

    for (dscpi=0; dscpi<65; ++dscpi)
      priv->rclassifier[dscpi]=NULL;

    priv->nrclassifier=0;

    //
    for (cxi=0; cxi<NAS_CX_MAX; cxi++) {
#ifdef DEBUG_DEVICE
      printk("INIT: init classifiers, state and timer for MT %u\n", cxi);
#endif

      //    priv->cx[cxi].sap[NAS_DC_INPUT_SAPI] = RRC_DEVICE_DC_INPUT0;
      //    priv->cx[cxi].sap[NAS_DC_OUTPUT_SAPI] = RRC_DEVICE_DC_OUTPUT0;
      priv->cx[cxi].state=NAS_IDLE;
      priv->cx[cxi].countimer=NAS_TIMER_IDLE;
      priv->cx[cxi].retry=0;
      priv->cx[cxi].lcr=cxi;
      priv->cx[cxi].rb=NULL;
      priv->cx[cxi].num_rb=0;

      // initialisation of the classifier
      for (dscpi=0; dscpi<65; ++dscpi) {
        priv->cx[cxi].sclassifier[dscpi]=NULL;
        priv->cx[cxi].fclassifier[dscpi]=NULL;
      }

      priv->cx[cxi].nsclassifier=0;
      priv->cx[cxi].nfclassifier=0;
      // initialisation of the IP address
      //  TOOL_imei2iid(IMEI, (uint8_t *)priv->cx[cxi].iid6);
      priv->cx[cxi].iid4=0;
      //
    }

    spin_lock_init(&priv->lock);
    //this requires kernel patch for OAI driver: typically for RF/hard realtime emu experimentation
#ifdef ADDRCONF
#ifdef NETLINK
    nas_TOOL_imei2iid(IMEI, dev->dev_addr);// IMEI to device address (for stateless autoconfiguration address)
    nas_TOOL_imei2iid(IMEI, (uint8_t *)priv->cx[0].iid6);
#else
    nas_TOOL_imei2iid((uint8_t *)nas_IMEI, dev->dev_addr); // IMEI to device address (for stateless autoconfiguration address)
    nas_TOOL_imei2iid((uint8_t *)nas_IMEI, (uint8_t *)priv->cx[0].iid6);
#endif
    // this is more appropriate for user space soft realtime emulation
#else
    memcpy(dev->dev_addr,&nas_IMEI[0],8);
    printk("Setting HW addr to : %X%X\n",*((unsigned int *)&dev->dev_addr[0]),*((unsigned int *)&dev->dev_addr[4]));

    ((unsigned char*)dev->dev_addr)[7] = (unsigned char)find_inst(dev);

    memcpy((uint8_t *)priv->cx[0].iid6,&nas_IMEI[0],8);

    printk("INIT: init IMEI to IID\n");
#endif

    printk("INIT: end\n");
    return;
  }  // instance value check
  else {
    printk("nas_init(): ERROR, Device is NULL!!\n");
    return;
  }
}
//---------------------------------------------------------------------------
int init_module (void)
{
  //---------------------------------------------------------------------------
  int err,inst;
  struct nas_priv *priv;
  char devicename[100];


  // Initialize parameters shared with RRC

  printk("Starting NASMESH, number of IMEI paramters %d, IMEI %X%X\n",m_arg,nas_IMEI[0],nas_IMEI[1]);

#ifndef PDCP_USE_NETLINK

  if (pdcp_2_nas_irq == -EBUSY || pdcp_2_nas_irq == -EINVAL) {
    printk("[NAS][INIT] No interrupt resource available\n");
    return -EBUSY;
  } else
    printk("[NAS][INIT]: Interrupt %d\n", pdcp_2_nas_irq);

#endif //NETLINK

  for (inst=0; inst<NB_INSTANCES_MAX; inst++) {
    printk("[NAS][INIT] nasmesh_init_module: begin init instance %d\n",inst);


    sprintf(devicename,"oai%d",inst);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
    nasdev[inst]  = alloc_netdev(sizeof(struct nas_priv),devicename, nas_init);
#else
    nasdev[inst]  = alloc_netdev(sizeof(struct nas_priv),devicename, NET_NAME_PREDICTABLE, nas_init);
#endif
    priv = netdev_priv(nasdev[inst]);

    if (nasdev[inst]) {
      nas_mesh_init(inst);
      //memcpy(nasdev[inst]->dev_addr,&nas_IMEI[0],8);
      nas_TOOL_imei2iid((uint8_t *)nas_IMEI, nasdev[inst]->dev_addr);// IMEI to device address (for stateless autoconfiguration address)
      nas_TOOL_imei2iid((uint8_t *)nas_IMEI, (uint8_t *)priv->cx[0].iid6);
      // TO HAVE DIFFERENT HW @
      ((unsigned char*)nasdev[inst]->dev_addr)[7] = ((unsigned char*)nasdev[inst]->dev_addr)[7] + (unsigned char)inst + 1;
      printk("Setting HW addr for INST %d to : %X%X\n",inst,*((unsigned int *)&nasdev[inst]->dev_addr[0]),*((unsigned int *)&nasdev[inst]->dev_addr[4]));

    }



    err= register_netdev(nasdev[inst]);

    if (err) {
      printk("[NAS][INIT] nasmesh_init_module (inst %d): error %i registering device %s\n", inst,err, nasdev[inst]->name);
    } else {
      printk("nasmesh_init_module: registering device %s, ifindex = %d\n\n",nasdev[inst]->name, nasdev[inst]->ifindex);
    }
  }

#ifdef PDCP_USE_NETLINK

  if ((err=nas_netlink_init()) == -1)
    printk("[NAS][INIT] NETLINK failed\n");

  printk("[NAS][INIT] NETLINK INIT\n");

#endif //NETLINK

  return err;

}

//---------------------------------------------------------------------------
void cleanup_module(void)
{
  //---------------------------------------------------------------------------

  int inst;


  printk("[NAS][CLEANUP]nasmesh_cleanup_module: begin\n");

#ifndef PDCP_USE_NETLINK

  if (pdcp_2_nas_irq!=-EBUSY) {
    pdcp_2_nas_irq=0;
    // Start IRQ linux
    //    free_irq(priv->irq, NULL);
    // End IRQ linux

  }

#else // NETLINK



#endif //NETLINK

  for (inst=0; inst<NB_INSTANCES_MAX; inst++) {
#ifdef DEBUG_DEVICE
    printk("nasmesh_cleanup_module: unregister and free net device instance %d\n",inst);
#endif
    unregister_netdev(nasdev[inst]);
    nas_teardown(nasdev[inst]);
    free_netdev(nasdev[inst]);
  }

#ifdef PDCP_USE_NETLINK
  nas_netlink_release();
#endif //PDCP_USE_NETLINK
  printk("nasmesh_cleanup_module: end\n");
}



#if  LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)

#define DRV_NAME    "NASMESH"
#define DRV_VERSION   "3.0.2"DRV_NAME
#define DRV_DESCRIPTION "OPENAIR MESH Device Driver"
#define DRV_COPYRIGHT "-Copyright(c) GNU GPL Eurecom 2009"
#define DRV_AUTHOR      "Raymond Knopp, and Navid Nikaein: <firstname.name@eurecom.fr>"DRV_COPYRIGHT

module_param_array(nas_IMEI,uint,&m_arg,0444);
MODULE_PARM_DESC(nas_IMEI,"The IMEI Hardware address (64-bit, decimal nibbles)");

module_param(nas_is_clusterhead,uint,0444);
MODULE_PARM_DESC(nas_is_clusterhead,"The Clusterhead Indicator");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,1)
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR(DRV_AUTHOR);
#endif
#endif



/*
//---------------------------------------------------------------------------
//module_init(init_nasmesh);
//module_exit(exit_nasmesh);
//---------------------------------------------------------------------------

*/
