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


#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/sysinfo.h>
#include <threadPool/thread-pool.h>

void displayList(notifiedFIFO_t *nf) {
  int n=0;
  notifiedFIFO_elt_t *ptr=nf->outF;

  while(ptr) {
    printf("element: %d, key: %lu\n",++n,ptr->key);
    ptr=ptr->next;
  }

  printf("End of list: %d elements\n",n);
}

static inline  notifiedFIFO_elt_t *pullNotifiedFifoRemember( notifiedFIFO_t *nf, struct one_thread *thr) {
  mutexlock(nf->lockF);

  while(!nf->outF && !thr->terminate)
    condwait(nf->notifF, nf->lockF);

  if (thr->terminate) {
    mutexunlock(nf->lockF);
    return NULL;
  }

  notifiedFIFO_elt_t *ret=nf->outF;
  nf->outF=nf->outF->next;

  if (nf->outF==NULL)
    nf->inF=NULL;

  // For abort feature
  thr->runningOnKey=ret->key;
  thr->dropJob = false;
  mutexunlock(nf->lockF);
  return ret;
}

void *one_thread(void *arg) {
  struct  one_thread *myThread=(struct  one_thread *) arg;
  struct  thread_pool *tp=myThread->pool;

  // Infinite loop to process requests
  do {
    notifiedFIFO_elt_t *elt=pullNotifiedFifoRemember(&tp->incomingFifo, myThread);
    if (elt == NULL) {
      AssertFatal(myThread->terminate, "pullNotifiedFifoRemember() returned NULL although thread not aborted\n");
      break;
    }

    if (tp->measurePerf) elt->startProcessingTime=rdtsc_oai();

    elt->processingFunc(NotifiedFifoData(elt));

    if (tp->measurePerf) elt->endProcessingTime=rdtsc_oai();

    if (elt->reponseFifo) {
      // Check if the job is still alive, else it has been aborted
      mutexlock(tp->incomingFifo.lockF);

      if (myThread->dropJob)
        delNotifiedFIFO_elt(elt);
      else
        pushNotifiedFIFO(elt->reponseFifo, elt);
      myThread->runningOnKey=-1;
      mutexunlock(tp->incomingFifo.lockF);
    }
  } while (!myThread->terminate);
  return NULL;
}

void initNamedTpool(char *params,tpool_t *pool, bool performanceMeas, char *name) {
  memset(pool,0,sizeof(*pool));
  char *measr=getenv("threadPoolMeasurements");
  pool->measurePerf=performanceMeas;
  // force measurement if the output is defined
  pool->measurePerf |= measr!=NULL;

  if (measr) {
    mkfifo(measr,0666);
    AssertFatal(-1 != (pool->dummyTraceFd=
                         open(measr, O_RDONLY| O_NONBLOCK)),"");
    AssertFatal(-1 != (pool->traceFd=
                         open(measr, O_WRONLY|O_APPEND|O_NOATIME|O_NONBLOCK)),"");
  } else
    pool->traceFd=-1;

  pool->activated=true;
  initNotifiedFIFO(&pool->incomingFifo);
  char *saveptr, * curptr;
  char *parms_cpy=strdup(params);
  pool->nbThreads=0;
  pool->restrictRNTI=false;
  curptr=strtok_r(parms_cpy,",",&saveptr);
  struct one_thread * ptr;
  char *tname = (name == NULL ? "Tpool" : name);
  while ( curptr!=NULL ) {
    int c=toupper(curptr[0]);

    switch (c) {
      case 'U':
        pool->restrictRNTI=true;
        break;

      case 'N':
        pool->activated=false;
        break;

      default:
        ptr=pool->allthreads;
        pool->allthreads=(struct one_thread *)malloc(sizeof(struct one_thread));
        pool->allthreads->next=ptr;
        printf("create a thread for core %d\n", atoi(curptr));
        pool->allthreads->coreID=atoi(curptr);
        pool->allthreads->id=pool->nbThreads;
        pool->allthreads->pool=pool;
        pool->allthreads->dropJob = false;
        pool->allthreads->terminate = false;
        //Configure the thread scheduler policy for Linux
        // set the thread name for debugging
        sprintf(pool->allthreads->name,"%s%d_%d",tname,pool->nbThreads,pool->allthreads->coreID);
        threadCreate(&pool->allthreads->threadID, one_thread, (void *)pool->allthreads,
                     pool->allthreads->name, pool->allthreads->coreID, OAI_PRIORITY_RT);
        pool->nbThreads++;
    }

    curptr=strtok_r(NULL,",",&saveptr);
  }
  free(parms_cpy);
  if (pool->activated && pool->nbThreads==0) {
    printf("No servers created in the thread pool, exit\n");
    exit(1);
  }
}

#ifdef TEST_THREAD_POOL
volatile int oai_exit=0;

void exit_function(const char *file, const char *function, const int line, const char *s) {
}

struct testData {
  int id;
  int sleepTime;
  char txt[30];
};

void processing(void *arg) {
  struct testData *in=(struct testData *)arg;
  //printf("doing: %d, %s, in thr %ld\n",in->id, in->txt,pthread_self() );
  sprintf(in->txt,"Done by %ld, job %d", pthread_self(), in->id);
  in->sleepTime=rand()%1000;
  usleep(in->sleepTime);
  //printf("done: %d, %s, in thr %ld\n",in->id, in->txt,pthread_self() );
}

int main() {
  notifiedFIFO_t myFifo;
  initNotifiedFIFO(&myFifo);
  pushNotifiedFIFO(&myFifo,newNotifiedFIFO_elt(sizeof(struct testData), 1234,NULL,NULL));

  for(int i=10; i>1; i--) {
    pushNotifiedFIFO(&myFifo,newNotifiedFIFO_elt(sizeof(struct testData), 1000+i,NULL,NULL));
  }

  displayList(&myFifo);
  notifiedFIFO_elt_t *tmp=pullNotifiedFIFO(&myFifo);
  printf("pulled: %lu\n", tmp->key);
  displayList(&myFifo);
  tmp=pullNotifiedFIFO(&myFifo);
  printf("pulled: %lu\n", tmp->key);
  displayList(&myFifo);
  abortNotifiedFIFOJob(&myFifo,1005);
  printf("aborted 1005\n");
  displayList(&myFifo);
  pushNotifiedFIFO(&myFifo,newNotifiedFIFO_elt(sizeof(struct testData), 12345678, NULL, NULL));
  displayList(&myFifo);
  abortNotifiedFIFOJob(&myFifo,12345678);
  printf("aborted 12345678\n");
  displayList(&myFifo);

  do {
    tmp=pollNotifiedFIFO(&myFifo);

    if (tmp) {
      printf("pulled: %lu\n", tmp->key);
      displayList(&myFifo);
    } else
      printf("Empty list \n");
  } while(tmp);

  tpool_t  pool;
  char params[]="1,2,3,4,5";
  initTpool(params,&pool, true);
  notifiedFIFO_t worker_back;
  initNotifiedFIFO(&worker_back);

  sleep(1);
  int cumulProcessTime=0, cumulTime=0;
  struct timespec st,end;
  clock_gettime(CLOCK_MONOTONIC, &st);
  int nb_jobs=4;
  for (int i=0; i <1000 ; i++) {
    int parall=nb_jobs;
    for (int j=0; j <parall ; j++) {
      notifiedFIFO_elt_t *work=newNotifiedFIFO_elt(sizeof(struct testData), i, &worker_back, processing);
      struct testData *x=(struct testData *)NotifiedFifoData(work);
      x->id=i;
      pushTpool(&pool, work);
    }
    int sleepmax=0;
    while (parall) {
      tmp=pullTpool(&worker_back,&pool);
      if (tmp) {
	parall--;
	struct testData *dd=NotifiedFifoData(tmp);
	if (dd->sleepTime > sleepmax)
	  sleepmax=dd->sleepTime;
	delNotifiedFIFO_elt(tmp);
      }
    }
    cumulProcessTime+=sleepmax;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  long long dur=(end.tv_sec-st.tv_sec)*1000*1000+(end.tv_nsec-st.tv_nsec)/1000;
  printf("In µs, Total time per group of %d job:%lld, work time per job %d, overhead per job %lld\n",
	 nb_jobs, dur/1000, cumulProcessTime/1000, (dur-cumulProcessTime)/(1000*nb_jobs));

	/*	
  for (int i=0; i <1000 ; i++) {
    notifiedFIFO_elt_t *work=newNotifiedFIFO_elt(sizeof(struct testData), i, &worker_back, processing);
    struct testData *x=(struct testData *)NotifiedFifoData(work);
    x->id=i;
    pushTpool(&pool, work);
  }

  do {
    tmp=pullTpool(&worker_back,&pool);

    if (tmp) {
      struct testData *dd=NotifiedFifoData(tmp);
      printf("Result: %s\n",dd->txt);
      delNotifiedFIFO_elt(tmp);
    } else
      printf("Empty list \n");

    abortTpoolJob(&pool,510);
  } while(tmp);
	*/
  return 0;
}
#endif
