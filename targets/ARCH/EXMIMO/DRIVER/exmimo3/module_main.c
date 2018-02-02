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

/** module_main.c (was: device.c)
*
*  Main Kernel module functions for load/init and cleanup of the kernel driver
*
*  Supports multiple ExpressMIMO cards
*
*  Authors: Matthias Ihmig <matthias.ihmig@mytum.de>, 2012, 2013
*           Riadh Ghaddab <riadh.ghaddab@eurecom.fr>
*           Raymond Knopp <raymond.knopp@eurecom.fr>
*
*  Changelog:
*  14.01.2013: removed remaining of BIGPHYS stuff and replaced with pci_alloc_consistent
*  20.01.2013: added support for multiple cards
*  24.01.2013: restructured interfaces & structures of how physical and virtual pointers are handled
*/

#include "openair_device.h"
#include "defs.h"
#include "vars.h"

#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/aer.h>
#include <linux/pci_regs.h>
#include <linux/delay.h>

static void openair_cleanup(void);

extern irqreturn_t openair_irq_handler(int irq, void *cookie);

static int irq = 0;
module_param(irq,int,S_IRUSR|S_IWUSR);


static struct file_operations openair_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
    unlocked_ioctl:openair_device_ioctl,
#else
    ioctl:  openair_device_ioctl,
#endif
    open:   openair_device_open,
    release:openair_device_release,
    mmap:   openair_device_mmap
};

static int __init openair_init_module( void )
{
  int res = 0;
  unsigned int readback;
  unsigned int card, j;
  unsigned int vid,did;
  unsigned short vendor, subid;
  exmimo_id_t exmimo_id_tmp[MAX_CARDS];

  //------------------------------------------------
  // Find and enable ExpressMIMO PCI cards
  //------------------------------------------------
  //
  card = 0;

  pdev[card] = pci_get_device(XILINX_VENDOR, XILINX_ID, NULL);

  if( pdev[card] ) {
    // This print does not work for 64 bit kernels
    //  printk("[openair][INIT_MODULE][INFO]:  openair card (ExpressMIMO) %d found, bus 0x%x, primary 0x%x, secondary 0x%x\n",card,
    //           pdev[card]->bus->number, pdev[card]->bus->primary,pdev[card]->bus->secondary);

    pci_read_config_word(pdev[card], PCI_SUBSYSTEM_ID, &subid);
    pci_read_config_word(pdev[card], PCI_SUBSYSTEM_VENDOR_ID, &vendor);
    exmimo_id_tmp[card].board_vendor = vendor;

    if ( exmimo_id_tmp[card].board_vendor == XILINX_VENDOR )
      exmimo_id_tmp[card].board_vendor = EURECOM_VENDOR; // set default to EURECOM

    exmimo_id_tmp[card].board_exmimoversion = (subid >> 12) & 0x0F;

    if (exmimo_id_tmp[card].board_exmimoversion == 0)
      exmimo_id_tmp[card].board_exmimoversion = 1;   // default (for old bitstreams) is ExpressMIMO-1

    exmimo_id_tmp[card].board_hwrev  = (subid >>  8) & 0x0F;
    exmimo_id_tmp[card].board_swrev  = (subid      ) & 0xFF;

    printk("[openair][INIT_MODULE][INFO]: card %d: ExpressMIMO-%i (HW Rev %i), Bitstream: %s, SW/Protocol Revision: 0x%02X\n", card,
           exmimo_id_tmp[card].board_exmimoversion, exmimo_id_tmp[card].board_hwrev, ( (exmimo_id_tmp[card].board_vendor == EURECOM_VENDOR) ? "Eurecom" : "Telecom Paristech"), exmimo_id_tmp[card].board_swrev );

    card++;
    vid = XILINX_VENDOR;
    did = XILINX_ID;
  } else {
    printk("[openair][INIT_MODULE][INFO]:  no card found, stopping.\n");
    return -ENODEV;
  }

  // Now look for more cards on the same bus
  while (card<MAX_CARDS) {
    pdev[card] = pci_get_device(vid,did, pdev[card-1]);

    if(pdev[card]) {
      // This print does not work for 64 bit kernels
      //  printk("[openair][INIT_MODULE][INFO]: openair card %d found, bus 0x%x, primary 0x%x, secondary 0x%x\n",card,
      //          pdev[card]->bus->number,pdev[card]->bus->primary,pdev[card]->bus->secondary);

      pci_read_config_word(pdev[card], PCI_SUBSYSTEM_ID, &subid);
      pci_read_config_word(pdev[card], PCI_SUBSYSTEM_VENDOR_ID, &vendor);
      exmimo_id_tmp[card].board_vendor = vendor;

      if ( exmimo_id_tmp[card].board_vendor == XILINX_VENDOR )
        exmimo_id_tmp[card].board_vendor = EURECOM_VENDOR;     // default (for old bitstreams) is EURECOM_VENDOR

      exmimo_id_tmp[card].board_exmimoversion = (subid >> 12) & 0x0F;

      if (exmimo_id_tmp[card].board_exmimoversion == 0)
        exmimo_id_tmp[card].board_exmimoversion = 1;   // default (for old bitstreams) is ExpressMIMO-1

      exmimo_id_tmp[card].board_hwrev  = (subid >>  8) & 0x0F;
      exmimo_id_tmp[card].board_swrev  = (subid      ) & 0xFF;
      printk("[openair][INIT_MODULE][INFO]: card %d: ExpressMIMO-%i (HW Rev %i), Bitstream: %s, SW/Protocol Revision: 0x%02X\n", card,
             exmimo_id_tmp[card].board_exmimoversion, exmimo_id_tmp[card].board_hwrev, ( (exmimo_id_tmp[card].board_vendor == EURECOM_VENDOR) ? "Eurecom" : "Telecom Paristech"), exmimo_id_tmp[card].board_swrev );

      card++;
    } else
      break;
  }

  // at least one device found, enable it
  number_of_cards = card;

  for (card=0; card<number_of_cards; card++) {
    if( pci_enable_device(pdev[card]) ) {
      printk("[openair][INIT_MODULE][INFO]: Could not enable PCI card device %d\n",card);
      openair_cleanup();
      return -ENODEV;
    } else {
      printk("[openair][INIT_MODULE][INFO]: *** CARD DEVICE %d (pdev=%p) ENABLED, irq %d\n",card,pdev[card],irq);
      printk("[openair][INIT_MODULE][INFO]: *** CARD DEVICE %d (pdev=%p) ENABLED, irq %d\n",card,pdev[card],pdev[card]->irq);
      openair_pci_device_enabled[card] = 1;
    }

    // Make the FPGA to a PCI master
    pci_set_master(pdev[card]);

    // set DMA mask
    if (!pci_set_dma_mask(pdev[card], DMA_BIT_MASK(32))) {
      if (pci_set_consistent_dma_mask(pdev[card], DMA_BIT_MASK(32))) {
        printk(KERN_INFO "[openair][INIT_MODULE]: Unable to obtain 32bit DMA for consistent allocations\n");
        openair_cleanup();
        return -EIO;
      }
    }

    //if (pci_enable_pcie_error_reporting(pdev[card]) > 0)
    //    printk("[openair][INIT_MODULE][INFO]: Enabled PCIe error reporting\n");
    //else
    //    printk("[openair][INIT_MODULE][INFO]: Failed to enable PCIe error reporting\n");

    //pci_cleanup_aer_uncorrect_error_status(pdev[card]);

    mmio_start[card]  = pci_resource_start(pdev[card], 0); // get start of BAR0
    mmio_length[card] = pci_resource_len  (pdev[card], 0);
    mmio_flags[card]  = pci_resource_flags(pdev[card], 0);

    if (check_mem_region(mmio_start[card],256) < 0) {
      printk("[openair][INIT_MODULE][FATAL] : Cannot get memory region 0, aborting\n");
      mmio_start[card] = 0;
      openair_cleanup();
      return -EBUSY;
    } else
      printk("[openair][INIT_MODULE][INFO] : Reserving memory region 0 : mmio_start = 0x%x\n",(unsigned int)mmio_start[card]);

    request_mem_region(mmio_start[card], 256, "openair_rf");

    bar[card] = pci_iomap( pdev[card], 0, mmio_length[card] );   // get virtual kernel address for BAR0

    printk("[openair][INIT_MODULE][INFO]: BAR0 card %d = 0x%p\n", card, bar[card]);

    printk("[openair][INIT_MODULE][INFO]: Writing 0x%x to BAR0+0x1c (PCIBASEL)\n", 0x12345678);

    iowrite32( 0x12345678, (bar[card]+PCIE_PCIBASEL) );
    udelay(100);
    readback = ioread32( bar[card]+PCIE_PCIBASEL );

    if (readback != 0x12345678) {
      printk("[openair][INIT_MODULE][INFO]: Readback of FPGA register failed (%x)\n",readback);
      openair_cleanup();
      return -EIO;
    }

    iowrite32((1<<8), bar[card] +PCIE_CONTROL0 ); // bit8=AHBPCIE_CTL0_SOFTRESET, but what is bit9 and bit10?
    udelay(1000);
    readback = ioread32( bar[card] +PCIE_CONTROL0);
    printk("CONTROL0 readback %x\n",readback);

    // allocating buffers
    if ( exmimo_memory_alloc( card ) ) {
      printk("[openair][MODULE][ERROR] exmimo_memory_alloc() failed!\n");
      openair_cleanup();
      return -ENOMEM;
    }

    exmimo_pci_kvirt[card].exmimo_id_ptr->board_vendor = exmimo_id_tmp[card].board_vendor;
    exmimo_pci_kvirt[card].exmimo_id_ptr->board_exmimoversion = exmimo_id_tmp[card].board_exmimoversion;
    exmimo_pci_kvirt[card].exmimo_id_ptr->board_hwrev = exmimo_id_tmp[card].board_hwrev;
    exmimo_pci_kvirt[card].exmimo_id_ptr->board_swrev = exmimo_id_tmp[card].board_swrev;

    if (irq!=0)
      printk("[OPENAIR][SCHED][INIT] card %d: Trying to get IRQ %d\n", card,irq);
    else
      printk("[OPENAIR][SCHED][INIT] card %d: Trying to get IRQ %d\n", card, pdev[card]->irq);

    openair_irq_enabled[card] = 0;

    // #ifdef CONFIG_PREEMPT_RT doesn't work -> fix misconfigured header files?
#if 1

    if (irq!=0) {
      if ( (res = request_irq(irq, openair_irq_handler,
                              IRQF_SHARED , "openair_rf", pdev[card] )) == 0)        {
        openair_irq_enabled[card] = 1;
      } else {
        printk("[EXMIMO][SCHED][INIT] Cannot get IRQ %d for HW, error: %d\n", irq, res);
        openair_cleanup();
        return -EBUSY;
      }
    } else {
      if ( (res = request_irq(pdev[card]->irq, openair_irq_handler,
                              IRQF_SHARED , "openair_rf", pdev[card] )) == 0)        {
        openair_irq_enabled[card] = 1;
      } else {
        printk("[EXMIMO][SCHED][INIT] Cannot get IRQ %d for HW, error: %d\n", pdev[card]->irq, res);
        openair_cleanup();
        return -EBUSY;
      }
    }


#else
    printk("Warning: didn't request IRQ for PREEMPT_REALTIME\n");
#endif

  } // for (i=0; i<number_of_cards; i++)

  //------------------------------------------------
  // Register the device in /dev
  //------------------------------------------------
  //
  major = openair_MAJOR;

  if( (res = register_chrdev(major, "openair", &openair_fops )) < 0) {
    printk("[openair][INIT_MODULE][ERROR]:  can't register char device driver, major : %d, error: %d\n", major, res);

    for (j=0; j<=number_of_cards; j++)
      release_mem_region(mmio_start[j],256);

    return -EBUSY;
  } else {
    printk("[openair][INIT_MODULE][INFO]:  char device driver registered major : %d\n", major);
    openair_chrdev_registered = 1;
  }

  printk("[openair][MODULE][INFO] Initializing Leon (EXMIMO_PCIE_INIT) on all cards...\n");

  for (card=0; card<number_of_cards; card++)
    exmimo_firmware_init( card );

  printk("[openair][MODULE][INFO] Done module init\n");

  return 0;
}


static void __exit openair_cleanup_module(void)
{
  //int card;
  printk("[openair][CLEANUP MODULE]\n");

  // stop any ongoing acquisition
  //for (card = 0; card < number_of_cards; card++)
  //    exmimo_send_pccmd(card, EXMIMO_STOP);

  openair_cleanup();
}

static void  openair_cleanup(void)
{
  int card;

  if ( openair_chrdev_registered )
    unregister_chrdev(major,"openair");

  openair_chrdev_registered = 0;

  for (card=0; card<number_of_cards; card++) {
    // unregister interrupt
    if ( openair_irq_enabled[card] ) {
      printk("[openair][CLEANUP] disabling interrupt card %d\n", card);

      if (irq!=0)
        free_irq(irq, pdev[card] );
      else
        free_irq( pdev[card]->irq, pdev[card] );

      openair_irq_enabled[card] = 0;
    }

    exmimo_firmware_cleanup( card );

    if ( bar[card] ) {
      printk("unmap bar[%d] %p\n", card, bar[card]);
      iounmap((void *)bar[card]);
    }

    if ( mmio_start[card] ) {
      printk("release mem[%d] %x\n", card, (unsigned int)mmio_start[card]);
      release_mem_region(mmio_start[card],256);
    }

    if ( openair_pci_device_enabled[card] ) {
      printk("pci_disable_device %i\n", card);
      pci_disable_device( pdev[card] );
    }
  }
}


MODULE_AUTHOR ("Raymond KNOPP <raymond.knopp@eurecom.fr>, Matthias IHMIG <matthias.ihmig@eurecom.fr>, Florian KALTENBERGER <florian.kaltenberger@eurecom.fr>, Riadh GHADDAB <riadh.ghaddab@eurecom.fr>");
MODULE_DESCRIPTION ("openair ExpressMIMO/ExpressMIMO3 driver");
MODULE_LICENSE ("GPL");
module_init (openair_init_module);
module_exit (openair_cleanup_module);
