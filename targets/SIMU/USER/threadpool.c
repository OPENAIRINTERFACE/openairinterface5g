/*
* Copyright (c) 2011, Mathias Brossard <mathias@brossard.org>.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
* @file threadpool.c
* @brief Threadpool implementation file
*/

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


#include "oaisim.h"
#include "UTIL/OCG/OCG_extern.h"
#include "UTIL/FIFO/pad_list.h"
#include "threadpool.h"

extern uint8_t abstraction_flag;
extern Signal_buffers *signal_buffers_g;

extern channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX];
extern channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX];

extern node_desc_t *enb_data[NUMBER_OF_eNB_MAX];
extern node_desc_t *ue_data[NUMBER_OF_UE_MAX];

extern LTE_DL_FRAME_PARMS *frame_parms;

/**
* @function void *threadpool_thread(void *threadpool)
* @brief the worker thread
* @param threadpool the pool which own the thread
*/
void *threadpool_thread(void *threadpool);

int threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(int thread_count)
{
  threadpool_t *pool;
  int i;

  /* TODO: Check for negative or otherwise very big input parameters */

  if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
    goto err;
  }

  // memset
  /* Initialize */
  pool->thread_count = thread_count;
  job_list_init(&(pool->job_queue));
  pool->shutdown = pool->started = pool->active = 0;

  /* Allocate thread and task queue */
  pool->threads = (pthread_t *)malloc(sizeof (pthread_t) * thread_count);

  // memset
  /* Initialize mutex and conditional variable first */
  if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
      (pthread_mutex_init(&(pool->sync_lock), NULL) != 0) ||
      (pthread_cond_init(&(pool->notify), NULL) != 0) ||
      (pthread_cond_init(&(pool->sync_notify), NULL) != 0) ||
      (pool->threads == NULL)) {
    goto err;
  }

  /* Start worker threads */
  for(i = 0; i < thread_count; i++) {
    if(pthread_create(&(pool->threads[i]), NULL,
                      threadpool_thread, (void*)pool) != 0) {
      threadpool_destroy(pool);
      return NULL;
    } else {
      pool->started++;
    }
  }

  return pool;

err:

  if(pool) {
    threadpool_free(pool);
  }

  return NULL;
}

int threadpool_add(threadpool_t *pool, Job_elt *job)
{
  int err = 0;

  if(pool == NULL || job == NULL) {
    return threadpool_invalid;
  }

  if(pthread_mutex_lock(&(pool->lock)) != 0) {
    return threadpool_lock_failure;
  }

  do {

    /* Are we shutting down ? */
    if(pool->shutdown) {
      err = threadpool_shutdown;
      break;
    }

    /* Add task to queue */
    job_list_add_tail_eurecom(job, &(pool->job_queue));

    /* pthread_cond_broadcast */
    if(pthread_cond_signal(&(pool->notify)) != 0) {
      err = threadpool_lock_failure;
      break;
    }
  } while(0);

  if(pthread_mutex_unlock(&pool->lock) != 0) {
    err = threadpool_lock_failure;
  }

  return err;
}

int threadpool_destroy(threadpool_t *pool)
{
  int i, err = 0;

  if(pool == NULL) {
    return threadpool_invalid;
  }

  if(pthread_mutex_lock(&(pool->lock)) != 0) {
    return threadpool_lock_failure;
  }

  do {
    /* Already shutting down */
    if(pool->shutdown) {
      err = threadpool_shutdown;
      break;
    }

    pool->shutdown = 1;

    /* Wake up all worker threads */
    if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
      err = threadpool_lock_failure;
      break;
    }

    /* Join all worker thread */
    for(i = 0; i < pool->thread_count; i++) {
      if(pthread_join(pool->threads[i], NULL) != 0) {
        err = threadpool_thread_failure;
      }
    }
  } while(0);

  if(pthread_mutex_unlock(&pool->lock) != 0) {
    err = threadpool_lock_failure;
  }

  /* Only if everything went well do we deallocate the pool */
  if(!err) {
    threadpool_free(pool);
  }

  return err;
}

int threadpool_free(threadpool_t *pool)
{
  if(pool == NULL || pool->started > 0) {
    return -1;
  }

  /* Did we manage to allocate ? */
  if(pool->threads) {
    free(pool->threads);
    job_list_free (&(pool->job_queue));

    /* Because we allocate pool->threads after initializing the
    mutex and condition variable, we're sure they're
    initialized. Let's lock the mutex just in case. */
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_mutex_destroy(&(pool->sync_lock));
    pthread_cond_destroy(&(pool->notify));
    pthread_cond_destroy(&(pool->sync_notify));
  }

  free(pool);
  return 0;
}


void *threadpool_thread(void *threadpool)
{
  threadpool_t *pool = (threadpool_t *)threadpool;
  Job_elt * job_elt;
  int err;


  printf("The ID of this thread is: %u\n", (unsigned int)pthread_self());

  for(;;) {
    /* Lock must be taken to wait on conditional variable */
    pthread_mutex_lock(&(pool->lock));

    /* Wait on condition variable, check for spurious wakeups.
       When returning from pthread_cond_wait(), we own the lock. */

    while(((pool->job_queue).nb_elements == 0) && (!pool->shutdown)) {
      pthread_cond_wait(&(pool->notify), &(pool->lock));
    }

    if(pool->shutdown) {
      break;
    }

    /* Grab our task */
    job_elt = job_list_remove_head(&(pool->job_queue));


    if((err = pthread_mutex_lock(&(pool->sync_lock))) != 0) {
      printf("Mutex Error \n");
      return err;
    }

    pool->active++;

    if((err = pthread_mutex_unlock(&pool->sync_lock)) != 0) {
      printf("Mutex Error \n");
      return err;
    }


    /* Unlock */
    pthread_mutex_unlock(&(pool->lock));

    /* Get to work */
    switch((job_elt->job).type) {
    case JT_OTG:
      if ((job_elt->job).eNB_flag)
        update_otg_eNB((job_elt->job).nid, (unsigned int) oai_emulation.info.time_ms);
      else
        update_otg_UE((job_elt->job).nid, (unsigned int) oai_emulation.info.time_ms);

      break;

    case JT_PDCP://do not forget adding flag enb
      if ((job_elt->job).eNB_flag)
        pdcp_run(frame, 1, 0, (job_elt->job).nid);
      else
        pdcp_run(frame, 0, (job_elt->job).nid, 0);

      break;

    case JT_PHY_MAC:
      if ((job_elt->job).eNB_flag)
        phy_procedures_eNB_lte ((job_elt->job).last_slot, (job_elt->job).next_slot, PHY_vars_eNB_g[(job_elt->job).nid], abstraction_flag);
      else
        phy_procedures_UE_lte ((job_elt->job).last_slot, (job_elt->job).next_slot, PHY_vars_UE_g[(job_elt->job).nid], 0, abstraction_flag, normal_txrx);

      ue_data[(job_elt->job).nid]->tx_power_dBm = PHY_vars_UE_g[(job_elt->job).nid]->tx_power_dBm;
      break;

    case JT_INIT_SYNC:
      initial_sync(PHY_vars_UE_g[(job_elt->job).nid],normal_txrx);
      break;

    case JT_DL:
      do_DL_sig(signal_buffers_g[(job_elt->job).nid].r_re0,signal_buffers_g[(job_elt->job).nid].r_im0,signal_buffers_g[(job_elt->job).nid].r_re,signal_buffers_g[(job_elt->job).nid].r_im,
                signal_buffers_g[(job_elt->job).nid].s_re,signal_buffers_g[(job_elt->job).nid].s_im,eNB2UE,enb_data,ue_data,(job_elt->job).next_slot,abstraction_flag,
                frame_parms,(job_elt->job).nid);
      break;

    case JT_UL:
      do_UL_sig(signal_buffers_g[(job_elt->job).nid].r_re0,signal_buffers_g[(job_elt->job).nid].r_im0,signal_buffers_g[(job_elt->job).nid].r_re,signal_buffers_g[(job_elt->job).nid].r_im,
                signal_buffers_g[(job_elt->job).nid].s_re,signal_buffers_g[(job_elt->job).nid].s_im,UE2eNB,enb_data,ue_data,(job_elt->job).next_slot,abstraction_flag,
                frame_parms,frame);
      break;

    }

    if((err = pthread_mutex_lock(&(pool->sync_lock))) != 0) {
      printf("Mutex Error \n");
      return err;
    }

    pool->active--;

    if ((pool->active <= 0) && ((pool->job_queue).nb_elements == 0)) {
      pool->active=0;

      if(pthread_cond_signal(&(pool->sync_notify)) != 0) {
        printf("Condition Error \n");
        break;
      }
    }


    if((err = pthread_mutex_unlock(&pool->sync_lock)) != 0) {
      printf("Mutex Error \n");
      return err;
    }


  }

  pool->started--;

  pthread_mutex_unlock(&(pool->lock));
  pthread_exit(NULL);
  return(NULL);
}
