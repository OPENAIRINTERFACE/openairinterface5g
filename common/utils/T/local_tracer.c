#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <inttypes.h>

#include "T_defs.h"

static T_cache_t *T_cache;
static int T_busylist_head;

typedef struct databuf {
  char *d;
  int l;
  struct databuf *next;
} databuf;

typedef struct {
  int socket_local;
  int socket_remote;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  databuf * volatile head, *tail;
  uint64_t memusage;
  uint64_t last_warning_memusage;
} forward_data;

/****************************************************************************/
/*                      utility functions                                   */
/****************************************************************************/

static void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att))
    { fprintf(stderr, "pthread_attr_init err\n"); exit(1); }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED))
    { fprintf(stderr, "pthread_attr_setdetachstate err\n"); exit(1); }
  if (pthread_attr_setstacksize(&att, 10000000))
    { fprintf(stderr, "pthread_attr_setstacksize err\n"); exit(1); }
  if (pthread_create(&t, &att, f, data))
    { fprintf(stderr, "pthread_create err\n"); exit(1); }
  if (pthread_attr_destroy(&att))
    { fprintf(stderr, "pthread_attr_destroy err\n"); exit(1); }
}

static int get_connection(char *addr, int port)
{
  struct sockaddr_in a;
  socklen_t alen;
  int s, t;

  printf("waiting for connection on %s:%d\n", addr, port);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) { perror("socket"); exit(1); }
  t = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)))
    { perror("setsockopt"); exit(1); }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(addr);

  if (bind(s, (struct sockaddr *)&a, sizeof(a))) { perror("bind"); exit(1); }
  if (listen(s, 5)) { perror("bind"); exit(1); }
  alen = sizeof(a);
  t = accept(s, (struct sockaddr *)&a, &alen);
  if (t == -1) { perror("accept"); exit(1); }
  close(s);

  printf("connected\n");

  return t;
}

/****************************************************************************/
/*                      forward functions                                   */
/****************************************************************************/

static void *data_sender(void *_f)
{
  forward_data *f = _f;
  databuf *cur;
  char *buf, *b;
  int size;

wait:
  if (pthread_mutex_lock(&f->lock)) abort();
  while (f->head == NULL)
    if (pthread_cond_wait(&f->cond, &f->lock)) abort();
  cur = f->head;
  buf = cur->d;
  size = cur->l;
  f->head = cur->next;
  f->memusage -= size;
  if (f->head == NULL) f->tail = NULL;
  if (pthread_mutex_unlock(&f->lock)) abort();
  free(cur);
  goto process;

process:
  b = buf;
  while (size) {
    int l = write(f->socket_remote, b, size);
    if (l <= 0) { printf("forward error\n"); exit(1); }
    size -= l;
    b += l;
  }

  free(buf);

  goto wait;
}

static void *forward_remote_messages(void *_f)
{
  forward_data *f = _f;
  int from = f->socket_remote;
  int to = f->socket_local;
  int l, len;
  char *b;
  char buf[1024];
  while (1) {
    len = read(from, buf, 1024);
    if (len <= 0) break;
    b = buf;

    while (len) {
      l = write(to, b, len);
      if (l <= 0) break;
      len -= l;
      b += l;
    }
  }
  return NULL;
}

static void *forwarder(int port, int s)
{
  forward_data *f;

  f = malloc(sizeof(*f)); if (f == NULL) abort();

  pthread_mutex_init(&f->lock, NULL);
  pthread_cond_init(&f->cond, NULL);

  f->socket_local = s;
  f->head = f->tail = NULL;

  f->memusage = 0;
  f->last_warning_memusage = 0;

  printf("waiting for remote tracer on port %d\n", port);

  f->socket_remote = get_connection("127.0.0.1", port);

  new_thread(data_sender, f);
  new_thread(forward_remote_messages, f);

  return f;
}

static void forward(void *_forwarder, char *buf, int size)
{
  forward_data *f = _forwarder;
  int32_t ssize = size;
  databuf *new;

  new = malloc(sizeof(*new)); if (new == NULL) abort();

  if (pthread_mutex_lock(&f->lock)) abort();

  new->d = malloc(size + 4); if (new->d == NULL) abort();
  /* put the size of the message at the head */
  memcpy(new->d, &ssize, 4);
  memcpy(new->d+4, buf, size);
  new->l = size+4;
  new->next = NULL;
  if (f->head == NULL) f->head = new;
  if (f->tail != NULL) f->tail->next = new;
  f->tail = new;

  f->memusage += size+4;
  /* warn every 100MB */
  if (f->memusage > f->last_warning_memusage &&
      f->memusage - f->last_warning_memusage > 100000000) {
    f->last_warning_memusage += 100000000;
    printf("WARNING: memory usage is over %"PRIu64"MB\n",
           f->last_warning_memusage / 1000000);
  } else
  if (f->memusage < f->last_warning_memusage &&
      f->last_warning_memusage - f->memusage > 100000000) {
    f->last_warning_memusage = (f->memusage/100000000) * 100000000;
  }

  if (pthread_cond_signal(&f->cond)) abort();
  if (pthread_mutex_unlock(&f->lock)) abort();
}

/****************************************************************************/
/*                      local functions                                     */
/****************************************************************************/

static void wait_message(void)
{
  while (T_cache[T_busylist_head].busy == 0) usleep(1000);
}

static void init_shm(void)
{
  int i;
  int s = shm_open(T_SHM_FILENAME, O_RDWR | O_CREAT /*| O_SYNC*/, 0666);
  if (s == -1) { perror(T_SHM_FILENAME); abort(); }
  if (ftruncate(s, T_CACHE_SIZE * sizeof(T_cache_t)))
    { perror(T_SHM_FILENAME); abort(); }
  T_cache = mmap(NULL, T_CACHE_SIZE * sizeof(T_cache_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED, s, 0);
  if (T_cache == NULL)
    { perror(T_SHM_FILENAME); abort(); }
  close(s);

  /* let's garbage the memory to catch some potential problems
   * (think multiprocessor sync issues, barriers, etc.)
   */
  memset(T_cache, 0x55, T_CACHE_SIZE * sizeof(T_cache_t));
  for (i = 0; i < T_CACHE_SIZE; i++) T_cache[i].busy = 0;
}

void T_local_tracer_main(int remote_port, int wait_for_tracer,
    int local_socket)
{
  int s;
  int port = remote_port;
  int dont_wait = wait_for_tracer ? 0 : 1;
  void *f;

  init_shm();
  s = local_socket;

  if (dont_wait) {
    char t = 2;
    if (write(s, &t, 1) != 1) abort();
  }

  f = forwarder(port, s);

  /* read messages */
  while (1) {
    wait_message();
    __sync_synchronize();
    forward(f, T_cache[T_busylist_head].buffer,
            T_cache[T_busylist_head].length);
    T_cache[T_busylist_head].busy = 0;
    T_busylist_head++;
    T_busylist_head &= T_CACHE_SIZE - 1;
  }
}
