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

#ifndef __VARS_H__
#define __VARS_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

#ifdef KERNEL2_6
#include <linux/slab.h>
#endif

#include "defs.h"
#include "pcie_interface.h"

unsigned int openair_irq_enabled[MAX_CARDS] = INIT_ZEROS;
unsigned int openair_chrdev_registered = 0;
unsigned int openair_pci_device_enabled[MAX_CARDS] = INIT_ZEROS;

struct pci_dev *pdev[MAX_CARDS] = INIT_ZEROS;
void __iomem   *bar[MAX_CARDS]  = INIT_ZEROS;

resource_size_t mmio_start[MAX_CARDS] = INIT_ZEROS;
resource_size_t mmio_length[MAX_CARDS];
unsigned int    mmio_flags[MAX_CARDS];

int major;

char number_of_cards;

// bigshm allocs a single larger block, used for shared structures and pointers
dma_addr_t bigshm_head_phys[MAX_CARDS] = INIT_ZEROS;
void      *bigshm_head[MAX_CARDS]      = INIT_ZEROS;
void      *bigshm_currentptr[MAX_CARDS];

dma_addr_t                      pphys_exmimo_pci_phys[MAX_CARDS];  // phys pointer to pci_bot structure in shared mem

exmimo_pci_interface_bot_t         *p_exmimo_pci_phys[MAX_CARDS] = INIT_ZEROS;  // inside struct has physical (DMA) pointers to pci_bot memory blocks

exmimo_pci_interface_bot_virtual_t    exmimo_pci_kvirt[MAX_CARDS]; // has virtual pointers to pci_bot memory blocks

#endif
