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
* Author and copyright: Laurent Thomas, open-cells.com
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


#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "assertions.h"
#include "common/utils/time_meas.h"
#include "common/utils/system.h"

#ifdef DEBUG
  #define THREADINIT   PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#else
  #define THREADINIT   PTHREAD_MUTEX_INITIALIZER
#endif
#define mutexinit(mutex)   {int ret=pthread_mutex_init(&mutex,NULL); \
                            AssertFatal(ret==0,"ret=%d\n",ret);}
#define condinit(signal)   {int ret=pthread_cond_init(&signal,NULL); \
                            AssertFatal(ret==0,"ret=%d\n",ret);}
#define mutexlock(mutex)   {int ret=pthread_mutex_lock(&mutex); \
                            AssertFatal(ret==0,"ret=%d\n",ret);}
#define mutextrylock(mutex)   pthread_mutex_trylock(&mutex)
#define mutexunlock(mutex) {int ret=pthread_mutex_unlock(&mutex); \
                            AssertFatal(ret==0,"ret=%d\n",ret);}
#define condwait(condition, mutex) {int ret=pthread_cond_wait(&condition, &mutex); \
                                    AssertFatal(ret==0,"ret=%d\n",ret);}
#define condbroadcast(signal) {int ret=pthread_cond_broadcast(&signal); \
                               AssertFatal(ret==0,"ret=%d\n",ret);}
#define condsignal(signal)    {int ret=pthread_cond_signal(&signal); \
                               AssertFatal(ret==0,"ret=%d\n",ret);}
#define tpool_nbthreads(tpool)   (tpool.nbThreads)
typedef struct notifiedFIFO_elt_s {
  struct notifiedFIFO_elt_s *next;
  uint64_t key; //To filter out elements
  struct notifiedFIFO_s *reponseFifo;
  void (*processingFunc)(void *);
  bool malloced;
  oai_cputime_t creationTime;
  oai_cputime_t startProcessingTime;
  oai_cputime_t endProcessingTime;
  oai_cputime_t returnTime;
  void *msgData;
}  notifiedFIFO_elt_t;

typedef struct notifiedFIFO_s {
  notifiedFIFO_elt_t *outF;
  notifiedFIFO_elt_t *inF;
  pthread_mutex_t lockF;
  pthread_cond_t  notifF;
  bool abortFIFO; // if set, the FIFO always returns NULL -> abort condition
} notifiedFIFO_t;

// You can use this allocator or use any piece of memory
static inline notifiedFIFO_elt_t *newNotifiedFIFO_elt(int size,
    uint64_t key,
    notifiedFIFO_t *reponseFifo,
    void (*processingFunc)(void *)) {
  notifiedFIFO_elt_t *ret;
  AssertFatal( NULL != (ret=(notifiedFIFO_elt_t *) calloc(1, sizeof(notifiedFIFO_elt_t)+size+32)), "");
  ret->next=NULL;
  ret->key=key;
  ret->reponseFifo=reponseFifo;
  ret->processingFunc=processingFunc;
  // We set user data piece aligend 32 bytes to be able to process it with SIMD
  ret->msgData=(void *)((uint8_t*)ret+(sizeof(notifiedFIFO_elt_t)/32+1)*32);
  ret->malloced=true;
  return ret;
}

static inline void *NotifiedFifoData(notifiedFIFO_elt_t *elt) {
  return elt->msgData;
}

static inline void delNotifiedFIFO_elt(notifiedFIFO_elt_t *elt) {
  if (elt->malloced) {
    elt->malloced = false;
    free(elt);
  }
  /* it is allowed to call delNotifiedFIFO_elt when the memory is managed by
   * the caller */
}

static inline void initNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf) {
  nf->inF=NULL;
  nf->outF=NULL;
  nf->abortFIFO = false;
}
static inline void initNotifiedFIFO(notifiedFIFO_t *nf) {
  mutexinit(nf->lockF);
  condinit (nf->notifF);
  initNotifiedFIFO_nothreadSafe(nf);
  // No delete function: the creator has only to free the memory
}

static inline void pushNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf, notifiedFIFO_elt_t *msg) {
  msg->next=NULL;

  if (nf->outF == NULL)
    nf->outF = msg;

  if (nf->inF != NULL)
    nf->inF->next = msg;

  nf->inF = msg;
}

static inline void pushNotifiedFIFO(notifiedFIFO_t *nf, notifiedFIFO_elt_t *msg) {
  mutexlock(nf->lockF);
  if (!nf->abortFIFO) {
    pushNotifiedFIFO_nothreadSafe(nf,msg);
    condsignal(nf->notifF);
  }
  mutexunlock(nf->lockF);
}

static inline  notifiedFIFO_elt_t *pullNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf) {
  if (nf->outF == NULL)
    return NULL;
  if (nf->abortFIFO)
    return NULL;

  notifiedFIFO_elt_t *ret=nf->outF;

  AssertFatal(nf->outF != nf->outF->next,"Circular list in thread pool: push several times the same buffer is forbidden\n");

  nf->outF=nf->outF->next;

  if (nf->outF==NULL)
    nf->inF=NULL;

  return ret;
}

static inline  notifiedFIFO_elt_t *pullNotifiedFIFO(notifiedFIFO_t *nf) {
  mutexlock(nf->lockF);
  notifiedFIFO_elt_t *ret = NULL;

  while((ret=pullNotifiedFIFO_nothreadSafe(nf)) == NULL && !nf->abortFIFO)
    condwait(nf->notifF, nf->lockF);

  mutexunlock(nf->lockF);
  return ret;
}

static inline  notifiedFIFO_elt_t *pollNotifiedFIFO(notifiedFIFO_t *nf) {
  int tmp=mutextrylock(nf->lockF);

  if (tmp != 0 )
    return NULL;

  if (nf->abortFIFO) {
    mutexunlock(nf->lockF);
    return NULL;
  }

  notifiedFIFO_elt_t *ret=pullNotifiedFIFO_nothreadSafe(nf);
  mutexunlock(nf->lockF);
  return ret;
}

static inline time_stats_t exec_time_stats_NotifiedFIFO(const notifiedFIFO_elt_t* elt)
{
  time_stats_t ts = {0};
  if (elt->startProcessingTime == 0 && elt->endProcessingTime == 0)
    return ts; /* no measurements done */
  ts.in = elt->startProcessingTime;
  ts.diff = elt->endProcessingTime - ts.in;
  ts.p_time = ts.diff;
  ts.diff_square = ts.diff * ts.diff;
  ts.max = ts.diff;
  ts.trials = 1;
  return ts;
}

// This function aborts all messages matching the key
// If the queue is used in thread pools, it doesn't cancels already running processing
// because the message has already been picked
static inline int abortNotifiedFIFOJob(notifiedFIFO_t *nf, uint64_t key) {
  mutexlock(nf->lockF);
  int nbDeleted=0;
  notifiedFIFO_elt_t **start=&nf->outF;

  while(*start!=NULL) {
    if ( (*start)->key == key ) {
      notifiedFIFO_elt_t *request=*start;
      *start=(*start)->next;
      delNotifiedFIFO_elt(request);
      nbDeleted++;
    } else
      start=&(*start)->next;
  }

  if (nf->outF == NULL)
    nf->inF=NULL;

  mutexunlock(nf->lockF);
  return nbDeleted;
}

// This functions aborts all messages in the queue, and marks the queue as
// "aborted", such that every call to it will return NULL
static inline void abortNotifiedFIFO(notifiedFIFO_t *nf) {
  mutexlock(nf->lockF);
  nf->abortFIFO = true;
  notifiedFIFO_elt_t **elt = &nf->outF;
  while(*elt != NULL) {
    notifiedFIFO_elt_t *p = *elt;
    *elt = (*elt)->next;
    delNotifiedFIFO_elt(p);
  }

  if (nf->outF == NULL)
    nf->inF = NULL;
  mutexunlock(nf->lockF);
  condsignal(nf->notifF);
}

struct one_thread {
  pthread_t  threadID;
  int id;
  int coreID;
  char name[256];
  uint64_t runningOnKey;
  bool dropJob;
  bool terminate;
  struct thread_pool *pool;
  struct one_thread *next;
};

typedef struct thread_pool {
  bool activated;
  bool measurePerf;
  int traceFd;
  int dummyTraceFd;
  uint64_t cpuCyclesMicroSec;
  uint64_t startProcessingUE;
  int nbThreads;
  notifiedFIFO_t incomingFifo;
  struct one_thread *allthreads;
} tpool_t;

static inline void pushTpool(tpool_t *t, notifiedFIFO_elt_t *msg) {
  if (t->measurePerf) msg->creationTime=rdtsc_oai();

  if ( t->activated)
    pushNotifiedFIFO(&t->incomingFifo, msg);
  else {
    if (t->measurePerf)
      msg->startProcessingTime=rdtsc_oai();

    msg->processingFunc(NotifiedFifoData(msg));

    if (t->measurePerf)
      msg->endProcessingTime=rdtsc_oai();

    if (msg->reponseFifo)
      pushNotifiedFIFO(msg->reponseFifo, msg);
  }
}

static inline notifiedFIFO_elt_t *pullTpool(notifiedFIFO_t *responseFifo, tpool_t *t) {
  notifiedFIFO_elt_t *msg= pullNotifiedFIFO(responseFifo);
  if (msg == NULL)
    return NULL;
  AssertFatal(t->traceFd, "Thread pool used while not initialized");
  if (t->measurePerf)
    msg->returnTime=rdtsc_oai();

  if (t->traceFd > 0)
    if(write(t->traceFd, msg, sizeof(*msg)));

  return msg;
}

static inline notifiedFIFO_elt_t *tryPullTpool(notifiedFIFO_t *responseFifo, tpool_t *t) {
  notifiedFIFO_elt_t *msg= pollNotifiedFIFO(responseFifo);
  AssertFatal(t->traceFd, "Thread pool used while not initialized");
  if (msg == NULL)
    return NULL;

  if (t->measurePerf)
    msg->returnTime=rdtsc_oai();

  if (t->traceFd)
    if(write(t->traceFd, msg, sizeof(*msg)));

  return msg;
}

static inline int abortTpoolJob(tpool_t *t, uint64_t key) {
  int nbRemoved=0;
  notifiedFIFO_t *nf=&t->incomingFifo;

  mutexlock(nf->lockF);
  notifiedFIFO_elt_t **start=&nf->outF;

  while(*start!=NULL) {
    if ( (*start)->key == key ) {
      notifiedFIFO_elt_t *request=*start;
      *start=(*start)->next;
      delNotifiedFIFO_elt(request);
      nbRemoved++;
    } else
      start=&(*start)->next;
  }

  if (t->incomingFifo.outF==NULL)
    t->incomingFifo.inF=NULL;

  struct one_thread *thread = t->allthreads;
  while (thread != NULL) {
    if (thread->runningOnKey == key) {
      thread->dropJob = true;
      nbRemoved++;
    }

    thread = thread->next;
  }

  mutexunlock(nf->lockF);
  return nbRemoved;
}
static inline int abortTpool(tpool_t *t) {
  int nbRemoved=0;
  /* disables threading: if a message comes in now, we cannot have a race below
   * as each thread will simply execute the message itself */
  t->activated = false;
  notifiedFIFO_t *nf=&t->incomingFifo;
  mutexlock(nf->lockF);
  nf->abortFIFO = true;
  notifiedFIFO_elt_t **start=&nf->outF;

  /* mark threads to abort them */
  struct one_thread *thread = t->allthreads;
  while (thread != NULL) {
    thread->dropJob = true;
    thread->terminate = true;
    nbRemoved++;
    thread = thread->next;
  }

  /* clear FIFOs */
  while(*start!=NULL) {
    notifiedFIFO_elt_t **request=start;
    *start=(*start)->next;
    delNotifiedFIFO_elt(*request);
    *request = NULL;
    nbRemoved++;
  }

  if (t->incomingFifo.outF==NULL)
    t->incomingFifo.inF=NULL;

  condbroadcast(t->incomingFifo.notifF);
  mutexunlock(nf->lockF);

  /* join threads that are still runing */
  thread = t->allthreads;
  while (thread != NULL) {
    pthread_cancel(thread->threadID);
    thread = thread->next;
  }

  return nbRemoved;
}
void initNamedTpool(char *params,tpool_t *pool, bool performanceMeas, char *name);
#define  initTpool(PARAMPTR,TPOOLPTR, MEASURFLAG) initNamedTpool(PARAMPTR,TPOOLPTR, MEASURFLAG, NULL)
#endif
