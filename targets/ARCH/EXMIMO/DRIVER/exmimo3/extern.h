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

#ifndef __EXTERN_H__
#define __EXTERN_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

#ifdef KERNEL2_6
#include <linux/slab.h>
#endif

#include "defs.h"
#include "pcie_interface.h"

extern char number_of_cards;

extern int major;

extern struct pci_dev *pdev[MAX_CARDS];
extern void __iomem *bar[MAX_CARDS];

extern void *bigshm_head[MAX_CARDS];
extern void *bigshm_currentptr[MAX_CARDS];
extern dma_addr_t bigshm_head_phys[MAX_CARDS];

extern dma_addr_t                      pphys_exmimo_pci_phys[MAX_CARDS];
extern exmimo_pci_interface_bot_t         *p_exmimo_pci_phys[MAX_CARDS];
extern exmimo_pci_interface_bot_virtual_t    exmimo_pci_kvirt[MAX_CARDS];

#endif // __EXTERN_H__
