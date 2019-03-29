/*
  Author: Laurent THOMAS, Open Cells
  copyleft: OpenAirInterface Software Alliance and it's licence
*/

#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <assertions.h>
#include <LOG/log.h>

#ifdef DEBUG
  #define THREADINIT   PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#else
  #define THREADINIT   PTHREAD_MUTEX_INITIALIZER
#endif
#define mutexinit(mutex)   AssertFatal(pthread_mutex_init(&mutex,NULL)==0,"");
#define condinit(signal)   AssertFatal(pthread_cond_init(&signal,NULL)==0,"");
#define mutexlock(mutex)   AssertFatal(pthread_mutex_lock(&mutex)==0,"");
#define mutextrylock(mutex)   pthread_mutex_trylock(&mutex)
#define mutexunlock(mutex) AssertFatal(pthread_mutex_unlock(&mutex)==0,"");
#define condwait(condition, mutex) AssertFatal(pthread_cond_wait(&condition, &mutex)==0,"");
#define condbroadcast(signal) AssertFatal(pthread_cond_broadcast(&signal)==0,"");
#define condsignal(signal)    AssertFatal(pthread_cond_broadcast(&signal)==0,"");

typedef struct notifiedFIFO_elt_s {
  struct notifiedFIFO_elt_s *next;
  uint64_t key; //To filter out elements
  struct notifiedFIFO_s *reponseFifo;
  void (*processingFunc)(void *);
  bool malloced;
  uint64_t creationTime;
  uint64_t startProcessingTime;
  uint64_t endProcessingTime;
  uint64_t returnTime;
  void *msgData;
}  notifiedFIFO_elt_t;

typedef struct notifiedFIFO_s {
  notifiedFIFO_elt_t *outF;
  notifiedFIFO_elt_t *inF;
  pthread_mutex_t lockF;
  pthread_cond_t  notifF;
} notifiedFIFO_t;

// You can use this allocator or use any piece of memory
static inline notifiedFIFO_elt_t *newNotifiedFIFO_elt(int size,
    uint64_t key,
    notifiedFIFO_t *reponseFifo,
    void (*processingFunc)(void *)) {
  notifiedFIFO_elt_t *ret;
  AssertFatal( NULL != (ret=(notifiedFIFO_elt_t *) malloc(sizeof(notifiedFIFO_elt_t)+size+32)), "");
  ret->next=NULL;
  ret->key=key;
  ret->reponseFifo=reponseFifo;
  ret->processingFunc=processingFunc;
  // We set user data piece aligend 32 bytes to be able to process it with SIMD
  ret->msgData=(void *)ret+(sizeof(notifiedFIFO_elt_t)/32+1)*32;
  ret->malloced=true;
  return ret;
}

static inline void *NotifiedFifoData(notifiedFIFO_elt_t *elt) {
  return elt->msgData;
}

static inline void delNotifiedFIFO_elt(notifiedFIFO_elt_t *elt) {
  if (elt->malloced) {
    elt->malloced=false;
    free(elt);
  } else
    printf("delNotifiedFIFO on something not allocated by newNotifiedFIFO\n");

  //LOG_W(UTIL,"delNotifiedFIFO on something not allocated by newNotifiedFIFO\n");
}

static inline void initNotifiedFIFO(notifiedFIFO_t *nf) {
  mutexinit(nf->lockF);
  condinit (nf->notifF);
  nf->inF=NULL;
  nf->outF=NULL;
  // No delete function: the creator has only to free the memory
}

static inline void pushNotifiedFIFO(notifiedFIFO_t *nf, notifiedFIFO_elt_t *msg) {
  mutexlock(nf->lockF);
  msg->next=NULL;

  if (nf->outF == NULL)
    nf->outF = msg;

  if (nf->inF)
    nf->inF->next = msg;

  nf->inF = msg;
  condbroadcast(nf->notifF);
  mutexunlock(nf->lockF);
}

static inline  notifiedFIFO_elt_t *pullNotifiedFIFO(notifiedFIFO_t *nf) {
  mutexlock(nf->lockF);

  while(!nf->outF)
    condwait(nf->notifF, nf->lockF);

  notifiedFIFO_elt_t *ret=nf->outF;
  nf->outF=nf->outF->next;

  if (nf->outF==NULL)
    nf->inF=NULL;

  mutexunlock(nf->lockF);
  return ret;
}

static inline  notifiedFIFO_elt_t *pollNotifiedFIFO(notifiedFIFO_t *nf) {
  int tmp=mutextrylock(nf->lockF);

  if (tmp != 0 )
    return NULL;

  notifiedFIFO_elt_t *ret=nf->outF;

  if (ret!=NULL)
    nf->outF=nf->outF->next;

  if (nf->outF==NULL)
    nf->inF=NULL;

  mutexunlock(nf->lockF);
  return ret;
}

// This function aborts all messages matching the key
// If the queue is used in thread pools, it doesn't cancels already running processing
// because the message has already been picked
static inline void abortNotifiedFIFO(notifiedFIFO_t *nf, uint64_t key) {
  mutexlock(nf->lockF);
  notifiedFIFO_elt_t **start=&nf->outF;

  while(*start!=NULL) {
    if ( (*start)->key == key ) {
      notifiedFIFO_elt_t *request=*start;
      *start=(*start)->next;
      delNotifiedFIFO_elt(request);
    }

    if (*start != NULL)
      start=&(*start)->next;
  }

  mutexunlock(nf->lockF);
}

struct one_thread {
  pthread_t  threadID;
  int id;
  int coreID;
  char name[256];
  uint64_t runningOnKey;
  bool abortFlag;
  struct thread_pool *pool;
  struct one_thread *next;
};

typedef struct thread_pool {
  int activated;
  bool measurePerf;
  int traceFd;
  int dummyTraceFd;
  uint64_t cpuCyclesMicroSec;
  uint64_t startProcessingUE;
  int nbThreads;
  bool restrictRNTI;
  notifiedFIFO_t incomingFifo;
  struct one_thread *allthreads;
} tpool_t;

static inline void pushTpool(tpool_t *t, notifiedFIFO_elt_t *msg) {
  if (t->measurePerf) msg->creationTime=rdtsc();

  pushNotifiedFIFO(&t->incomingFifo, msg);
}

static inline notifiedFIFO_elt_t *pullTpool(notifiedFIFO_t *responseFifo, tpool_t *t) {
  notifiedFIFO_elt_t *msg= pullNotifiedFIFO(responseFifo);

  if (t->measurePerf)
    msg->returnTime=rdtsc();

  if (t->traceFd)
    if(write(t->traceFd, msg, sizeof(*msg)));

  return msg;
}

static inline notifiedFIFO_elt_t *tryPullTpool(notifiedFIFO_t *responseFifo, tpool_t *t) {
  notifiedFIFO_elt_t *msg= pollNotifiedFIFO(responseFifo);

  if (msg == NULL)
    return NULL;

  if (t->measurePerf)
    msg->returnTime=rdtsc();

  if (t->traceFd)
    if(write(t->traceFd, msg, sizeof(*msg)));

  return msg;
}

static inline void abortTpool(tpool_t *t, uint64_t key) {
  notifiedFIFO_t *nf=&t->incomingFifo;
  mutexlock(nf->lockF);
  notifiedFIFO_elt_t **start=&nf->outF;

  while(*start!=NULL) {
    if ( (*start)->key == key ) {
      notifiedFIFO_elt_t *request=*start;
      *start=(*start)->next;
      delNotifiedFIFO_elt(request);
    }

    if (*start != NULL)
      start=&(*start)->next;
  }

  struct one_thread *ptr=t->allthreads;

  while(ptr!=NULL) {
    if (ptr->runningOnKey==key)
      ptr->abortFlag=true;

    ptr=ptr->next;
  }

  mutexunlock(nf->lockF);
}
void initTpool(char *params,tpool_t *pool, bool performanceMeas);

#endif
