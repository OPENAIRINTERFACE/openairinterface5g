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

#ifndef __DEFS_H__
#define __DEFS_H__

#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/delay.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/mman.h>

#include <linux/slab.h>
//#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/errno.h>

#ifdef KERNEL2_6
#include <linux/slab.h>
#endif

#include "openair_device.h"

#include "linux/moduleparam.h"


/*------------------------------------------------*/
/*   Prototypes                                   */
/*------------------------------------------------*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
long openair_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
int openair_device_ioctl(struct inode *inode,struct file *filp, unsigned int cmd, unsigned long arg);
#endif
int openair_device_open    (struct inode *inode,struct file *filp);
int openair_device_release (struct inode *inode,struct file *filp);
int openair_device_mmap    (struct file *filp, struct vm_area_struct *vma);

int exmimo_memory_alloc(int card);
int exmimo_firmware_init(int card);
int exmimo_firmware_cleanup(int card_id);

int exmimo_send_pccmd(int card_id, unsigned int cmd);

#endif
