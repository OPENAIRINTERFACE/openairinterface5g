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

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

/**
* @file threadpool.h
* @brief Threadpool Header file
*/

/**
* @struct threadpool
* @brief The threadpool struct
*
* @var notify Condition variable to notify worker threads.
* @var threads Array containing worker threads ID.
* @var thread_count Number of threads
* @var job_queue Array containing the task queue.
* @var shutdown Flag indicating if the pool is shutting down
*/
struct threadpool_t {
  pthread_mutex_t lock; // used to get / put a job in the queue, protect job_queue
  pthread_mutex_t sync_lock; // used when there is a dependancy between two consecutive jobs, protect active
  pthread_cond_t notify; // wakup threads
  pthread_cond_t sync_notify; // last worker sends this
  pthread_t *threads;
  Job_List job_queue;
  int thread_count;
  int count; // ????
  int shutdown; // end of sim/emu
  int started; // total number of thread started , after init: started == count
  int active; // num active threads
};

typedef struct threadpool_t threadpool_t;

typedef enum {
  threadpool_invalid = -1,
  threadpool_lock_failure = -2,
  threadpool_queue_full = -3,
  threadpool_shutdown = -4,
  threadpool_thread_failure = -5
} threadpool_error_t;

/**
* @function threadpool_create
* @brief Creates a threadpool_t object.
* @param thread_count Number of worker threads.
* @param queue_size Size of the queue.
* @param flags Unused parameter.
* @return a newly created thread pool or NULL
*/
threadpool_t *threadpool_create(int thread_count);

/**
* @function threadpool_add
* @brief add a new task in the queue of a thread pool
* @param pool Thread pool to which add the task.
* @param function Pointer to the function that will perform the task.
* @param argument Argument to be passed to the function.
* @param flags Unused parameter.
* @return 0 if all goes well, negative values in case of error (@see
* threadpool_error_t for codes).
*/
int threadpool_add(threadpool_t *pool, Job_elt *job);

/**
* @function threadpool_destroy
* @brief Stops and destroys a thread pool.
* @param pool Thread pool to destroy.
* @param flags Unused parameter.
*/
int threadpool_destroy(threadpool_t *pool);

#endif /* _THREADPOOL_H_ */
