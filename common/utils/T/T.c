#include "T.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

#define QUIT(x) do { \
  printf("T tracer: QUIT: %s\n", x); \
  exit(1); \
} while (0)

/* array used to activate/disactivate a log */
static int T_IDs[T_NUMBER_OF_IDS];
int *T_active = T_IDs;

static int T_socket;

/* T_cache
 * - the T macro picks up the head of freelist and marks it busy
 * - the T sender thread periodically wakes up and sends what's to be sent
 */
volatile int _T_freelist_head;
volatile int *T_freelist_head = &_T_freelist_head;
T_cache_t *T_cache;

static void get_message(int s)
{
  char t;
  int l;
  int id;
  int is_on;

  if (read(s, &t, 1) != 1) QUIT("get_message fails");
printf("T tracer: got mess %d\n", t);
  switch (t) {
  case 0:
    /* toggle all those IDs */
    /* optimze? (too much syscalls) */
    if (read(s, &l, sizeof(int)) != sizeof(int)) QUIT("get_message fails");
    while (l) {
      if (read(s, &id, sizeof(int)) != sizeof(int)) QUIT("get_message fails");
      T_IDs[id] = 1 - T_IDs[id];
      l--;
    }
    break;
  case 1:
    /* set IDs as given */
    /* optimize? */
    if (read(s, &l, sizeof(int)) != sizeof(int)) QUIT("get_message fails");
    id = 0;
    while (l) {
      if (read(s, &is_on, sizeof(int)) != sizeof(int))
        QUIT("get_message fails");
      T_IDs[id] = is_on;
      id++;
      l--;
    }
    break;
  case 2: break; /* do nothing, this message is to wait for local tracer */
  }
}

static void *T_receive_thread(void *_)
{
  while (1) get_message(T_socket);
  return NULL;
}

static void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att))
    { fprintf(stderr, "pthread_attr_init err\n"); exit(1); }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED))
    { fprintf(stderr, "pthread_attr_setdetachstate err\n"); exit(1); }
  if (pthread_create(&t, &att, f, data))
    { fprintf(stderr, "pthread_create err\n"); exit(1); }
  if (pthread_attr_destroy(&att))
    { fprintf(stderr, "pthread_attr_destroy err\n"); exit(1); }
}

/* defined in local_tracer.c */
void T_local_tracer_main(int remote_port, int wait_for_tracer,
    int local_socket);

/* We monitor the tracee and the local tracer processes.
 * When one dies we forcefully kill the other.
 */
#include <sys/types.h>
#include <sys/wait.h>
static void monitor_and_kill(int child1, int child2)
{
  int child;
  int status;

  child = wait(&status);
  if (child == -1) perror("wait");
  kill(child1, SIGKILL);
  kill(child2, SIGKILL);
  exit(0);
}

void T_init(int remote_port, int wait_for_tracer, int dont_fork)
{
  int socket_pair[2];
  int s;
  int T_shm_fd;
  int child1, child2;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair))
    { perror("socketpair"); abort(); }

  /* child1 runs the local tracer and child2 (or main) runs the tracee */

  child1 = fork(); if (child1 == -1) abort();
  if (child1 == 0) {
    close(socket_pair[1]);
    T_local_tracer_main(remote_port, wait_for_tracer, socket_pair[0]);
    exit(0);
  }
  close(socket_pair[0]);

  if (dont_fork == 0) {
    child2 = fork(); if (child2 == -1) abort();
    if (child2 != 0) {
      close(socket_pair[1]);
      monitor_and_kill(child1, child2);
    }
  }

  s = socket_pair[1];
  /* wait for first message - initial list of active T events */
  get_message(s);

  T_socket = s;

  /* setup shared memory */
  T_shm_fd = shm_open(T_SHM_FILENAME, O_RDWR /*| O_SYNC*/, 0666);
  shm_unlink(T_SHM_FILENAME);
  if (T_shm_fd == -1) { perror(T_SHM_FILENAME); abort(); }
  T_cache = mmap(NULL, T_CACHE_SIZE * sizeof(T_cache_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED, T_shm_fd, 0);
  if (T_cache == NULL)
    { perror(T_SHM_FILENAME); abort(); }
  close(T_shm_fd);

  new_thread(T_receive_thread, NULL);
}
