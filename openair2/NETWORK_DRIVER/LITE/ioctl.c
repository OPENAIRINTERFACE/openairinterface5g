/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

#include "local.h"
#include "ioctl.h"
#include "proto_extern.h"

#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <asm/uaccess.h>

#define NIP6ADDR(addr) \
        ntohs((addr)->s6_addr16[0]), \
        ntohs((addr)->s6_addr16[1]), \
        ntohs((addr)->s6_addr16[2]), \
        ntohs((addr)->s6_addr16[3]), \
        ntohs((addr)->s6_addr16[4]), \
        ntohs((addr)->s6_addr16[5]), \
        ntohs((addr)->s6_addr16[6]), \
        ntohs((addr)->s6_addr16[7])

uint8_t g_msgrep[OAI_NW_DRV_LIST_CLASS_MAX*sizeof(struct oai_nw_drv_msg_class_list_reply)+1];

// Statistic
//---------------------------------------------------------------------------
void oai_nw_drv_set_msg_statistic_reply(struct oai_nw_drv_msg_statistic_reply *msgrep,
                                        struct oai_nw_drv_priv *priv)
{
  //---------------------------------------------------------------------------
  msgrep->rx_packets=priv->stats.rx_packets;
  msgrep->tx_packets=priv->stats.tx_packets;
  msgrep->rx_bytes=priv->stats.rx_bytes;
  msgrep->tx_bytes=priv->stats.tx_bytes;
  msgrep->rx_errors=priv->stats.rx_errors;
  msgrep->tx_errors=priv->stats.tx_errors;
  msgrep->rx_dropped=priv->stats.rx_dropped;
  msgrep->tx_dropped=priv->stats.tx_dropped;
}

//---------------------------------------------------------------------------
int oai_nw_drv_ioCTL_statistic_request(struct oai_nw_drv_ioctl *gifr,
                                       struct oai_nw_drv_priv *priv)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_msg_statistic_reply msgrep;
  printk("NAS_IOCTL_STATISTIC: stat requested\n");
  oai_nw_drv_set_msg_statistic_reply(&msgrep,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_STATISTIC: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}



///////////////////////////////////////////////////////////////////////////////
// IMEI
// Messages for IMEI transfer



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IOCTL command
//---------------------------------------------------------------------------
int oai_nw_drv_CTL_ioctl(struct net_device *dev,
                         struct ifreq *ifr,
                         int cmd)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_ioctl *gifr;
  struct oai_nw_drv_priv *priv=netdev_priv(dev);

  int r;

  //  printk("NAS_CTL_IOCTL: begin ioctl for instance %d\n",find_inst(dev));

  switch(cmd) {
  case OAI_NW_DRV_IOCTL_RRM:
    gifr=(struct oai_nw_drv_ioctl *)ifr;

    switch(gifr->type) {
    case OAI_NW_DRV_MSG_STATISTIC_REQUEST:
      r=oai_nw_drv_ioCTL_statistic_request(gifr,priv);
      break;


    default:
      //  printk("NAS_IOCTL_RRM: unkwon request type, type=%x\n", gifr->type);
      r=-EFAULT;
    }

    break;

  default:
    //      printk("NAS_CTL_IOCTL: Unknown ioctl command, cmd=%x\n", cmd);
    r=-EFAULT;
  }

  //  printk("NAS_CTL_IOCTL: end\n");
  return r;
}

//---------------------------------------------------------------------------
void oai_nw_drv_CTL_send(struct sk_buff *skb, int inst)
{
  //---------------------------------------------------------------------------
  printk("NAS_CTL_SEND - void \n");
}
