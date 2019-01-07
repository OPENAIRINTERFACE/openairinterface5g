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

static irqreturn_t irq_handler(int irq, void *dev_id)
{
  uint32_t irqval;
  irqreturn_t res = IRQ_NONE;
  struct em1_private_s *pv = dev_id;

  spin_lock(&pv->lock);
  irqval = ioread32(pv->base + REG_ISTATUS);

  if (irqval & IRQ_MSK) {
    //    printk(KERN_DEBUG "em1 irqval 0x%x", irqval);

    if ((irqval & EM1_NOTFULL_IRQ) && (pv->busy & EM1_BUSY_FIFO_W)) {
      wake_up_all(&pv->rq_wait_fifo_w);
      res = IRQ_HANDLED;
    }

    if ((irqval & EM1_NOTEMPTY_IRQ) && (pv->busy & EM1_BUSY_FIFO_R)) {
      wake_up_all(&pv->rq_wait_fifo_r);
      res = IRQ_HANDLED;
    }

    if (irqval & EM1_DMA_IRQ) {
      pv->dma_done = irqval >> 16;
      wake_up_all(&pv->rq_wait_dma);
      res = IRQ_HANDLED;
    }
  }

  spin_unlock(&pv->lock);
  return res;
}

int __devinit em1_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
  int ret, i;
  resource_size_t len, start;
  struct em1_private_s *pv;

  for (i = 0; i < MAX_EM1_DEVICES; i++)
    if (em1_devices[i] == NULL)
      goto found;

  printk("exmimo1 : Too many devices");
  return -ENOENT;
found:

  pv = kmalloc(sizeof(struct em1_private_s), GFP_KERNEL);
  em1_devices[i] = pv;

  if (pv == NULL) {
    printk(KERN_ERR "exmimo1 : Out of memory\n");
    return -ENOMEM;
  }

  pci_set_drvdata(dev, pv);

  ret = pci_enable_device(dev);

  if (ret <0) {
    printk(KERN_WARNING "exmimo1 : Problem when enabling PCI device\n");
    goto err_kfree;
  }

  /* Set interrupt handler function */
  ret = request_irq(dev->irq, &irq_handler, IRQF_SHARED, "exmimo1", pv);

  if (ret <0) {
    printk(KERN_WARNING "exmimo1 : Problem when requesting interrupt for PCI device\n");
    goto err_disable;
  }

  /* Address of first PCI BAR region */
  len = pci_resource_len(dev, 0);
  start = pci_resource_start(dev, 0);
  printk("exmimo1 : len : %d\n", len);
  printk("exmimo1 : start : %x\n", start);

  if (check_mem_region(start, len) < 0) {
    printk(KERN_ERR "exmimo1 : Cannot check memory region 0, already in use\n");
    ret = -ENOMEM;
    goto err_req_irq;
  }

  if (request_mem_region(start, len, "exmimo") == NULL) {
    printk(KERN_ERR "exmimo1 : Cannot get memory region 0, already in use\n");
    ret = -ENOMEM;
    goto err_req_irq;
  }

  pv->base = pci_iomap(dev, 0, len);
  printk(KERN_DEBUG "exmimo1 : base address := 0x%x\n", (unsigned int)pv->base);

  pci_set_master(dev);

  init_waitqueue_head(&pv->rq_wait_fifo_r);
  init_waitqueue_head(&pv->rq_wait_fifo_w);
  init_waitqueue_head(&pv->rq_wait_dma);

  pv->ie = 0;
  pv->busy = 0;
  pv->fifo_read_left = 0;
  pv->fifo_write_left = 0;
  spin_lock_init(&pv->lock);
  pv->dev = &dev->dev;
  pv->pdev = dev;

  /* Get DMA done counter */
  pv->dma_started = pv->dma_done = ioread32(pv->base + REG_ISTATUS) >> 16;

  /* Disable interrupt */
  iowrite32(pv->ie, pv->base + REG_FSTATUS);

  return 0;

err_req_irq:
  free_irq(dev->irq, pv);
err_disable:
  pci_disable_device(dev);
err_kfree:
  kfree(pv);
  em1_devices[i] = NULL;
  return ret;
}

void __devexit em1_remove(struct pci_dev *dev)
{
  int i;

  struct em1_private_s *pv = pci_get_drvdata(dev);
  resource_size_t len, start;

  printk(KERN_DEBUG "exmimo1 : remove()\n");

  iounmap((void*)pv->base);
  /* Address of first PCI BAR region */

  len = pci_resource_len(dev, 0);
  start = pci_resource_start(dev, 0);
  release_mem_region(start, len);

  // Unregister interrupt
  free_irq(dev->irq, pv);

  pci_disable_device(dev);

  //Free private data
  kfree(pv);

  for (i = 0; i < MAX_EM1_DEVICES; i++)
    if (em1_devices[i] == pv)
      em1_devices[i] = NULL;
}

/*
 * Local Variables:
 * c-file-style: "linux"
 * indent-tabs-mode: t
 * tab-width: 8
 * End:
 */

