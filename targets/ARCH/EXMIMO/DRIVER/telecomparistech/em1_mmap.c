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

