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

int em1_fifo_write(struct em1_private_s *pv, const uint32_t *buf, size_t count)
{
  int retval = 0;
  size_t cnt = count;

  while (cnt > 0) {
    //read fifo state register
    uint32_t data = ioread32(pv->base + REG_FSTATUS);

    if (data & PUSH_FULL_STATUS_MSK) {
      /* Enable push fifo no full interrupt */
      pv->ie |= PUSH_NO_FULL_IRQ_MSK;
      iowrite32(pv->ie, pv->base + REG_FSTATUS);

      set_current_state(TASK_UNINTERRUPTIBLE);
      spin_unlock_irq(&pv->lock);
      //          printk(KERN_DEBUG "sleep %08x %i", data, current->state);
      schedule();
      //          printk(KERN_DEBUG "wake");

      spin_lock_irq(&pv->lock);
      continue;
    }

    data = *buf++;
    cnt--;
    iowrite32(data, pv->base + REG_FCMD);

    if (!pv->fifo_write_left) {
      /* Size must be retrieved from POP fifo entry */
      pv->fifo_write_left = data & 0xFF;
    } else {
      pv->fifo_write_left--;
    }

    if (!pv->fifo_write_left)
      break;
  }

  pv->ie &= ~PUSH_NO_FULL_IRQ_MSK;
  iowrite32(pv->ie, pv->base + REG_FSTATUS);

  return retval;
}

int em1_fifo_read(struct em1_private_s *pv, uint32_t *buf, size_t count)
{
  int retval = 0;
  size_t cnt = count;

  while (cnt > 0) {
    //read fifo state register
    uint32_t data = ioread32(pv->base + REG_FSTATUS);

    if (data & POP_EMPTY_STATUS_MSK) {
      /* Enable pop fifo no empty interrupt */
      pv->ie |= POP_NO_EMPTY_IRQ_MSK;
      iowrite32(pv->ie, pv->base + REG_FSTATUS);

      set_current_state(TASK_UNINTERRUPTIBLE);
      spin_unlock_irq(&pv->lock);
      schedule();
      spin_lock_irq(&pv->lock);
      continue;
    }

    data = ioread32(pv->base + REG_FCMD);
    *buf++ = data;
    cnt--;

    if (!pv->fifo_read_left) {
      /* Size must be retrieved from POP fifo entry */
      pv->fifo_read_left = data & 0xFF;
    } else {
      pv->fifo_read_left--;
    }

    if (!pv->fifo_read_left)
      break;
  }

  pv->ie &= ~POP_NO_EMPTY_IRQ_MSK;
  iowrite32(pv->ie, pv->base + REG_FSTATUS);

  return retval;
}

