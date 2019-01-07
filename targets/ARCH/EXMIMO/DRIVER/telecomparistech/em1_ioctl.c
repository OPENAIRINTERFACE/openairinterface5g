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

static int em1_ioctl_fifo_write(struct em1_private_s *pv, size_t size, uint32_t *words)
{
  DECLARE_WAITQUEUE(wait, current);
  int res = -ERESTARTSYS;

  if (!em1_user_op_enter(pv, &wait, &pv->rq_wait_fifo_w,
                         EM1_BUSY_FIFO_W, TASK_INTERRUPTIBLE)) {
    res = em1_fifo_write(pv, words, size);
    res = 0;
    pv->busy &= ~EM1_BUSY_FIFO_W;
  }

  em1_user_op_leave(pv, &wait, &pv->rq_wait_fifo_w);

  return res;
}

static int em1_ioctl_fifo_read(struct em1_private_s *pv, size_t size, uint32_t *words)
{
  DECLARE_WAITQUEUE(wait, current);
  int res = -ERESTARTSYS;

  if (!em1_user_op_enter(pv, &wait, &pv->rq_wait_fifo_r,
                         EM1_BUSY_FIFO_R, TASK_INTERRUPTIBLE)) {
    res = em1_fifo_read(pv, words, size);
    pv->busy &= ~EM1_BUSY_FIFO_R;
  }

  em1_user_op_leave(pv, &wait, &pv->rq_wait_fifo_r);

  return res;
}

int em1_ioctl(struct inode *inode, struct file *file,
              unsigned int cmd, unsigned long arg)
{
  struct em1_file_s *fpv = file->private_data;
  struct em1_private_s *pv = fpv->pv;

  switch ((enum em1_ioctl_cmd)cmd) {
  case EM1_IOCTL_FIFO_WRITE: {
      struct em1_ioctl_fifo_params p;
      uint32_t w[EM1_MAX_FIFO_PAYLOAD];

      if (copy_from_user(&p, (void*)arg, sizeof(p)))
      return -EFAULT;

      if (p.count > EM1_MAX_FIFO_PAYLOAD)
        return -EINVAL;

        if (copy_from_user(w, p.words, p.count * sizeof(uint32_t)))
          return -EFAULT;

          return em1_ioctl_fifo_write(pv, p.count, w);
        }

        case EM1_IOCTL_FIFO_READ: {
            struct em1_ioctl_fifo_params p;
            uint32_t w[EM1_MAX_FIFO_PAYLOAD];
            int res;

            if (copy_from_user(&p, (void*)arg, sizeof(p)))
              return -EFAULT;

              if (p.count > EM1_MAX_FIFO_PAYLOAD)
                return -EINVAL;

                res = em1_ioctl_fifo_read(pv, p.count, w);

                if (!res && copy_to_user(p.words, w, p.count * sizeof(uint32_t)))
                  return -EFAULT;

                  return res;
                }
                }

return 0;
}


/*
 * Local Variables:
 * c-file-style: "linux"
 * indent-tabs-mode: t
 * tab-width: 8
 * End:
 */

