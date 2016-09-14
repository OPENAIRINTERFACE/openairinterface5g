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

