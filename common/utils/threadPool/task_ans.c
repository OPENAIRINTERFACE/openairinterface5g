/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "task_ans.h"
#include "assertions.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "pthread_utils.h"
#include "errno.h"
#include <string.h>

#define seminit(sem)                                                                           \
  {                                                                                            \
    int ret = sem_init(&sem, 0, 0);                                                            \
    AssertFatal(ret == 0, "sem_init(): ret=%d, errno=%d (%s)\n", ret, errno, strerror(errno)); \
  }
#define sempost(sem)                                                                           \
  {                                                                                            \
    int ret = sem_post(&sem);                                                                  \
    AssertFatal(ret == 0, "sem_post(): ret=%d, errno=%d (%s)\n", ret, errno, strerror(errno)); \
  }
/* POSIX sem_wait(), ERRORS: retry EINTR before treating the task as complete. */
#define semwait(sem)                                                                           \
  {                                                                                            \
    int ret;                                                                                 \
    do {                                                                                     \
      ret = sem_wait(&sem);                                                                   \
    } while (ret == -1 && errno == EINTR);                                                     \
    AssertFatal(ret == 0, "sem_wait(): ret=%d, errno=%d (%s)\n", ret, errno, strerror(errno)); \
  }
#define semdestroy(sem)                                                                           \
  {                                                                                               \
    int ret = sem_destroy(&sem);                                                                  \
    AssertFatal(ret == 0, "sem_destroy(): ret=%d, errno=%d (%s)\n", ret, errno, strerror(errno)); \
  }

void init_task_ans(task_ans_t* ans, uint num_jobs)
{
  ans->counter = num_jobs;
  seminit(ans->sem);
}

void completed_many_task_ans(task_ans_t* ans, uint num_completed_jobs)
{
  DevAssert(ans != NULL);
  // Using atomic counter in contention scenario to avoid locking in producers
  int num_jobs = atomic_fetch_sub_explicit(&ans->counter, num_completed_jobs, memory_order_acq_rel);
  if (num_jobs == num_completed_jobs) {
    // Using semaphore to enable blocking call in join_task_ans
    sempost(ans->sem);
  }
}

void join_task_ans(task_ans_t* ans)
{
  semwait(ans->sem);
  semdestroy(ans->sem);
}
