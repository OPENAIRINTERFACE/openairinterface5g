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

#include "em1_drv.h"


int em1_mmap(struct file *filp, struct vm_area_struct *vma)
{

  int i;
  struct em1_file_s *fpv = filp->private_data;
  struct em1_mmap_ctx * mctx;
  size_t size = vma->vm_end - vma->vm_start;
  void * virt;
  dma_addr_t phys;

  printk(KERN_DEBUG "exmimo1 : mmap()\n");

  if (vma->vm_end <= vma->vm_start)
    return -EINVAL;

  for (i = 0; i < MAX_EM1_MMAP; i++) {
    mctx = fpv->mmap_list + i;

    if (mctx->virt == 0)
      break;
  }

  if (i == MAX_EM1_MMAP)
    return -ENOMEM;

  virt = pci_alloc_consistent(fpv->pv->pdev, size, &phys);

  if (virt < 0)
    return -EINVAL;

  if (remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT, size, vma->vm_page_prot))
    return -EAGAIN;

  mctx->virt = virt;
  mctx->phys = phys;
  mctx->size = size;

  return 0;
}


/*
 * Local Variables:
 * c-file-style: "linux"
 * indent-tabs-mode: t
 * tab-width: 8
 * End:
 */

