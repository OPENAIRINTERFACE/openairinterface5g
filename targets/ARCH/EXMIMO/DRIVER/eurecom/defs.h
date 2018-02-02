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
