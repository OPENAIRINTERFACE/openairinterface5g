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
#include <sys/types.h>
#include <signal.h>

#include "common/config/config_userapi.h"

/* array used to activate/disactivate a log */
static int T_IDs[T_NUMBER_OF_IDS];
int *T_active = T_IDs;

int T_stdout = 1;

static int T_socket;
static int local_tracer_pid;

/* T_cache
 * - the T macro picks up the head of freelist and marks it busy
 * - the T sender thread periodically wakes up and sends what's to be sent
 */
volatile int _T_freelist_head;
volatile int *T_freelist_head = &_T_freelist_head;
T_cache_t *T_cache;

#define GET_MESSAGE_FAIL do { \
    printf("T tracer: get_message failed\n"); \
    return -1; \
  } while (0)

/* return -1 if error, 0 if no error */
static int get_message(int s)
{
  char t;
  int l;
  int id;
  int is_on;

  if (read(s, &t, 1) != 1) GET_MESSAGE_FAIL;

  printf("T tracer: got mess %d\n", t);

  switch (t) {
    case 0:
      /* toggle all those IDs */
      /* optimze? (too much syscalls) */
      if (read(s, &l, sizeof(int)) != sizeof(int)) GET_MESSAGE_FAIL;

      while (l) {
        if (read(s, &id, sizeof(int)) != sizeof(int)) GET_MESSAGE_FAIL;

        T_IDs[id] = 1 - T_IDs[id];
        l--;
      }
      break;
    case 1:
      /* set IDs as given */
      /* optimize? */
      if (read(s, &l, sizeof(int)) != sizeof(int)) GET_MESSAGE_FAIL;

      id = 0;

      while (l) {
        if (read(s, &is_on, sizeof(int)) != sizeof(int))
          GET_MESSAGE_FAIL;

        T_IDs[id] = is_on;
        id++;
        l--;
      }
      break;
    case 2:
      break; /* do nothing, this message is to wait for local tracer */
  }

  return 0;
}

static void *T_receive_thread(void *_)
{
  int err = 0;
  while (!err) err = get_message(T_socket);

  printf("T tracer: fatal: T tracer disabled\n");

  shutdown(T_socket, SHUT_RDWR);
  close(T_socket);
  kill(local_tracer_pid, SIGKILL);

  /* disable all traces */
  memset(T_IDs, 0, sizeof(T_IDs));

  return NULL;
}

static void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att)) {
    fprintf(stderr, "pthread_attr_init err\n");
    exit(1);
  }

  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED)) {
    fprintf(stderr, "pthread_attr_setdetachstate err\n");
    exit(1);
  }

  if (pthread_create(&t, &att, f, data)) {
    fprintf(stderr, "pthread_create err\n");
    exit(1);
  }

  if (pthread_attr_destroy(&att)) {
    fprintf(stderr, "pthread_attr_destroy err\n");
    exit(1);
  }
}

/* defined in local_tracer.c */
void T_local_tracer_main(int remote_port, int wait_for_tracer,
                         int local_socket, void *shm_array);

void T_init(int remote_port, int wait_for_tracer)
{
  int socket_pair[2];
  int s;
  int child;
  int i;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair)) {
    perror("socketpair");
    abort();
  }

  /* setup shared memory */
  T_cache = mmap(NULL, T_CACHE_SIZE * sizeof(T_cache_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (T_cache == MAP_FAILED) {
    perror("mmap");
    abort();
  }

  /* let's garbage the memory to catch some potential problems
   * (think multiprocessor sync issues, barriers, etc.)
   */
  memset(T_cache, 0x55, T_CACHE_SIZE * sizeof(T_cache_t));

  for (i = 0; i < T_CACHE_SIZE; i++) T_cache[i].busy = 0;

  /* child runs the local tracer and main runs the tracee */
  child = fork();

  if (child == -1) abort();

  if (child == 0) {
    close(socket_pair[1]);
    T_local_tracer_main(remote_port, wait_for_tracer, socket_pair[0],
                        T_cache);
    exit(0);
  }

  local_tracer_pid = child;

  close(socket_pair[0]);

  s = socket_pair[1];
  T_socket = s;

  /* wait for first message - initial list of active T events */
  if (get_message(s) == -1) {
    kill(local_tracer_pid, SIGKILL);
    return;
  }

  new_thread(T_receive_thread, NULL);
}

void T_Config_Init(void)
{
  int T_port = TTRACER_DEFAULT_PORTNUM;
  int T_nowait = 0;
  paramdef_t ttraceparams[] = CMDLINE_TTRACEPARAMS_DESC;

  config_get(config_get_if(), ttraceparams, sizeofArray(ttraceparams), TTRACER_CONFIG_PREFIX);
  /* compatibility: look for TTracer command line options in root section */
  config_process_cmdline(config_get_if(), ttraceparams, sizeofArray(ttraceparams), NULL);

  if (T_stdout < 0 || T_stdout > 2) {
    printf("fatal error: T_stdout = %d but only values 0, 1, or 2 are allowed\n", T_stdout);
    exit(1);
  }

  if (T_stdout == 0 || T_stdout == 2)
    T_init(T_port, 1-T_nowait);
}
