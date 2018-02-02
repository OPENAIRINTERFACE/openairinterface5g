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

void em1_unlock_user_page(struct em1_page_s *ep)
{
  if (ep->addr != 0) {
    //    printk(KERN_DEBUG "unlocking user read page");
    page_cache_release(ep->page);
    ep->addr = 0;
  }
}

int em1_lock_user_page(struct em1_page_s *ep, uintptr_t addr, int direction)
{
  int res;

  if (ep->addr == addr)
    return 0;

  em1_unlock_user_page(ep);

  //  printk(KERN_DEBUG "locking user page for addr %p", addr);

  down_read(&current->mm->mmap_sem);
  res = get_user_pages(current, current->mm, addr & ~(PAGE_SIZE-1),
                       1, direction == DMA_FROM_DEVICE,
                       0, /* don't force */ &ep->page, NULL);
  up_read(&current->mm->mmap_sem);

  if (res != 1) {
    printk(KERN_ERR "exmimo1: get_user_pages error");
    return -EFAULT;
  }

  ep->addr = addr;

  return 0;
}

static ssize_t em1_dma_access(struct em1_file_s *fpv, char __user *buf,
                              size_t size, int direction, int opcode)
{
  DECLARE_WAITQUEUE(wait, current);
  struct em1_private_s *pv = fpv->pv;
  uint32_t cmd[4];
  uintptr_t addr = (uintptr_t)buf;
  uintptr_t end = (uintptr_t)buf + size - 1;
  struct em1_page_s *page;
  int fres = -ERESTARTSYS;
  uint16_t dma_this;

  /* check single page span */
  if ((addr & ~(PAGE_SIZE-1)) != (end & ~(PAGE_SIZE-1)))
    return -EINVAL;

  if (!access_ok(direction == DMA_FROM_DEVICE ? VERIFY_WRITE : VERIFY_READ, buf, size))
    return -EFAULT;

  page = direction == DMA_FROM_DEVICE ? &fpv->read : &fpv->write;

  if (em1_lock_user_page(page, addr, direction))
    return -EFAULT;

  addr = dma_map_page(pv->dev, page->page, 0, PAGE_SIZE, direction);

  /* prepare command for em1 */
  cmd[0] = opcode | 3;
  cmd[1] = sizeof(addr) > 4 ? (uint32_t)(addr >> 32) : 0;
  cmd[2] = (uint32_t)addr;
  cmd[3] = size;

  if (!em1_user_op_enter(pv, &wait, &pv->rq_wait_fifo_w,
                         EM1_BUSY_FIFO_W, TASK_INTERRUPTIBLE)) {
    /* send command */
    fres = em1_fifo_write(pv, cmd, 4);
    pv->busy &= ~EM1_BUSY_FIFO_W;
    dma_this = ++pv->dma_started;
  }

  em1_user_op_leave(pv, &wait, &pv->rq_wait_fifo_w);

  //  printk(KERN_DEBUG "exmimo1: dma fifo write %i, addr %x", fres, addr);

  if (!fres) {
    if (!em1_user_op_enter(pv, &wait, &pv->rq_wait_dma,
                           EM1_BUSY_DMA, TASK_UNINTERRUPTIBLE)) {
      //      printk(KERN_DEBUG "wait dma");

      /* wait for DMA end irq */
      while ((int16_t)(dma_this - pv->dma_done) > 0) {
        set_current_state(TASK_UNINTERRUPTIBLE);
        spin_unlock_irq(&pv->lock);
        schedule();
        spin_lock_irq(&pv->lock);
      }

      pv->busy &= ~EM1_BUSY_DMA;
      fres = size;
    }

    em1_user_op_leave(pv, &wait, &pv->rq_wait_dma);
  }

  dma_unmap_page(pv->dev, addr, PAGE_SIZE, direction);

  if (direction == DMA_FROM_DEVICE)
    SetPageDirty(page->page);

  return fres;
}

ssize_t em1_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
  return em1_dma_access(file->private_data, (char*)buf, size,
                        DMA_TO_DEVICE, EM1_CMD_OP_DMA_PC2EM1);
}

ssize_t em1_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
  return em1_dma_access(file->private_data, buf, size,
                        DMA_FROM_DEVICE, EM1_CMD_OP_DMA_EM12PC);
}

/*
 * Local Variables:
 * c-file-style: "linux"
 * indent-tabs-mode: t
 * tab-width: 8
 * End:
 */

