/*
  Author: Laurent THOMAS, Open Cells
  copyleft: OpenAirInterface Software Alliance and it's licence
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

  while(!nf->outF)
    condwait(nf->notifF, nf->lockF);

  notifiedFIFO_elt_t *ret=nf->outF;
  nf->outF=nf->outF->next;

  if (nf->outF==NULL)
    nf->inF=NULL;

  // For abort feature
  thr->runningOnKey=ret->key;
  thr->abortFlag=false;
  mutexunlock(nf->lockF);
  return ret;
}

void *one_thread(void *arg) {
  struct  one_thread *myThread=(struct  one_thread *) arg;
  struct  thread_pool *tp=myThread->pool;

  // configure the thread core assignment
  // TBD: reserve the core for us exclusively
  if ( myThread->coreID >= 0 &&  myThread->coreID < get_nprocs_conf()) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(myThread->coreID, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  }

  //Configure the thread scheduler policy for Linux
  struct sched_param sparam= {0};
  sparam.sched_priority = sched_get_priority_max(SCHED_RR);
  pthread_setschedparam(pthread_self(), SCHED_RR, &sparam);
  // set the thread name for debugging
  sprintf(myThread->name,"Tpool_%d",myThread->coreID);
  pthread_setname_np(pthread_self(), myThread->name );

  // Infinite loop to process requests
  do {
    notifiedFIFO_elt_t *elt=pullNotifiedFifoRemember(&tp->incomingFifo, myThread);

    if (tp->measurePerf) elt->startProcessingTime=rdtsc();

    elt->processingFunc(NotifiedFifoData(elt));

    if (tp->measurePerf) elt->endProcessingTime=rdtsc();

    if (elt->reponseFifo) {
      // Check if the job is still alive, else it has been aborted
      mutexlock(tp->incomingFifo.lockF);

      if (myThread->abortFlag)
        delNotifiedFIFO_elt(elt);
      else
        pushNotifiedFIFO(elt->reponseFifo, elt);

      mutexunlock(tp->incomingFifo.lockF);
    }
  } while (true);
}

void initTpool(char *params,tpool_t *pool, bool performanceMeas) {
  memset(pool,0,sizeof(*pool));
  char *measr=getenv("threadPoolMeasurements");
  pool->measurePerf=performanceMeas;
  // force measurement if the output is defined
  pool->measurePerf=measr!=NULL;

  if (measr) {
    mkfifo(measr,0666);
    AssertFatal(-1 != (pool->dummyTraceFd=
                         open(measr, O_RDONLY| O_NONBLOCK)),"");
    AssertFatal(-1 != (pool->traceFd=
                         open(measr, O_WRONLY|O_APPEND|O_NOATIME|O_NONBLOCK)),"");
  } else
    pool->traceFd=-1;

  //Configure the thread scheduler policy for Linux
  struct sched_param sparam= {0};
  sparam.sched_priority = sched_get_priority_max(SCHED_RR)-1;
  pthread_setschedparam(pthread_self(), SCHED_RR, &sparam);
  pool->activated=true;
  initNotifiedFIFO(&pool->incomingFifo);
  char *saveptr, * curptr;
  pool->nbThreads=0;
  pool->restrictRNTI=false;
  curptr=strtok_r(params,",",&saveptr);

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
        pool->allthreads=(struct one_thread *)malloc(sizeof(struct one_thread));
        pool->allthreads->next=pool->allthreads;
        printf("create a thread for core %d\n", atoi(curptr));
        pool->allthreads->coreID=atoi(curptr);
        pool->allthreads->id=pool->nbThreads;
        pool->allthreads->pool=pool;
        pthread_create(&pool->allthreads->threadID, NULL, one_thread, (void *)pool->allthreads);
        pool->nbThreads++;
    }

    curptr=strtok_r(NULL,",",&saveptr);
  }

  if (pool->activated && pool->nbThreads==0) {
    printf("No servers created in the thread pool, exit\n");
    exit(1);
  }
}

#ifdef TEST_THREAD_POOL

struct testData {
  int id;
  char txt[30];
};

void processing(void *arg) {
  struct testData *in=(struct testData *)arg;
  printf("doing: %d, %s, in thr %ld\n",in->id, in->txt,pthread_self() );
  sprintf(in->txt,"Done by %ld, job %d", pthread_self(), in->id);
  usleep(rand()%100);
  printf("done: %d, %s, in thr %ld\n",in->id, in->txt,pthread_self() );
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
  abortNotifiedFIFO(&myFifo,1005);
  printf("aborted 1005\n");
  displayList(&myFifo);
  pushNotifiedFIFO(&myFifo,newNotifiedFIFO_elt(sizeof(struct testData), 12345678, NULL, NULL));
  displayList(&myFifo);
  abortNotifiedFIFO(&myFifo,12345678);
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
  char params[]="1,2,3,u";
  initTpool(params,&pool, true);
  notifiedFIFO_t worker_back;
  initNotifiedFIFO(&worker_back);

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

    abortTpool(&pool,510);
  } while(tmp);

  return 0;
}
#endif
