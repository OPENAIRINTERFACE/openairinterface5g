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

#ifndef USER_APP

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/blkdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/spinlock_types.h>
#include <linux/interrupt.h>


/*
  Operations:

  - DMA from pc to em1: The Linux driver writes a 4 words command to
    the control fifo [0x103 phys_hi phys_lo size] and waits for a DMA
    end interrupt from the em1 board.

    This operation is requested by the user application using the
    write syscall.  The buffer provided by the application must not
    span across multiple pages.

  - DMA from em1 to pc: The Linux driver writes a 4 words command to
    the control fifo [0x203 phys_hi phys_lo size] and waits for a DMA
    end interrupt from the em1 board.

    This operation is requested by the user application using the read
    syscall.  The buffer provided by the application must not span
    across multiple pages.
*/

#define XILINX_VENDOR 0x10ee
#define XILINX_ID     0x0007

#define REG_FCMD       0x0
#define REG_FSTATUS    0x4
#define REG_ISTATUS    0x8
#define REG_STATUS     0x10
#define REG_PCI_BASE_L 0x28
#define REG_PCI_BASE_H 0x2c

#define PUSH_FULL_STATUS_MSK (1 << 3)
#define POP_EMPTY_STATUS_MSK (1 << 2)

#define EM1_NOTFULL_IRQ (1 << 0)
#define EM1_NOTEMPTY_IRQ (1 << 1)
#define EM1_DMA_IRQ (1 << 2)

#define PUSH_NO_FULL_IRQ_MSK (1 << 19)
#define POP_NO_EMPTY_IRQ_MSK (1 << 18)

#define IRQ_MSK 7

#define EM1_CMD_OP_DMA_PC2EM1 0x0100
#define EM1_CMD_OP_DMA_EM12PC 0x0200

#define MAX_EM1_DEVICES 8
#define MAX_EM1_MMAP 4

#define EM1_BUSY_FIFO_R 1
#define EM1_BUSY_FIFO_W 2
#define EM1_BUSY_DMA    4

struct  em1_private_s;

struct em1_page_s {
  uintptr_t addr;
  struct page *page;
};

struct em1_mmap_ctx {
  void* virt;
  dma_addr_t phys;
  size_t size;
};

struct em1_file_s {
  struct em1_private_s *pv;
  struct em1_page_s read;
  struct em1_page_s write;
  struct em1_mmap_ctx mmap_list[MAX_EM1_MMAP];
};

struct  em1_private_s {
  struct pci_dev *pdev;
  struct device *dev;
  spinlock_t lock;

  /* memeory mapped device base address */
  void * base;

  /* wait queues */
  wait_queue_head_t rq_wait_fifo_r;
  wait_queue_head_t rq_wait_fifo_w;
  wait_queue_head_t rq_wait_dma;

  /* Number of fifo words left to read or write */
  size_t fifo_read_left;
  size_t fifo_write_left;

  /* Cached interrupt enabled value */
  uint32_t ie;
  /* Driver busy operations flags */
  int busy;

  /* Completed DMA transfer counters */
  uint16_t dma_started, dma_done;
};

extern struct em1_private_s *em1_devices[MAX_EM1_DEVICES];

/* em1_drv.c */
int em1_user_op_enter(struct em1_private_s *pv, wait_queue_t *wait,
                      wait_queue_head_t *wh, int busy_flag, int new_state);
void em1_user_op_leave(struct em1_private_s *pv, wait_queue_t *wait,
                       wait_queue_head_t *wh);

/* em1_rw.c: read and write syscalls */
int em1_lock_user_page(struct em1_page_s *ep, uintptr_t addr, int direction);
void em1_unlock_user_page(struct em1_page_s *fpv);
ssize_t em1_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos);
ssize_t em1_read(struct file *file, char __user *buf, size_t size, loff_t *ppos);

/* em1_dev.c */
int __devinit em1_probe(struct pci_dev *dev, const struct pci_device_id *id);
void __devexit em1_remove(struct pci_dev *dev);

/* em1_fifos.c */
int em1_fifo_write(struct em1_private_s *pv, const uint32_t *buf, size_t count);
int em1_fifo_read(struct em1_private_s *pv, uint32_t *buf, size_t count);

/* em1_ioctl.c */
int em1_ioctl(struct inode *inode, struct file *file,
              unsigned int cmd, unsigned long arg);

/* em1_mmap.c */
int em1_mmap(struct file *filp, struct vm_area_struct *vma);

#endif

enum em1_ioctl_cmd {
  EM1_IOCTL_FIFO_WRITE,
  EM1_IOCTL_FIFO_READ,
};

#define EM1_MAX_FIFO_PAYLOAD 128

struct em1_ioctl_fifo_params {
  uint32_t *words;
  size_t count;
};

/*
 * Local Variables:
 * c-file-style: "linux"
 * indent-tabs-mode: t
 * tab-width: 8
 * End:
 */

