#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

typedef struct databuf {
  char *d;
  int l;
  struct databuf *next;
} databuf;

typedef struct {
  int s;
  int sc;
  pthread_mutex_t lock;
  pthread_mutex_t datalock;
  pthread_cond_t datacond;
  databuf * volatile head, *tail;
  uint64_t memusage;
  uint64_t last_warning_memusage;
} forward_data;

static void *data_sender(void *_f)
{
  forward_data *f = _f;
  databuf *cur;
  char *buf, *b;
  int size;

wait:
  if (pthread_mutex_lock(&f->datalock)) abort();
  while (f->head == NULL)
    if (pthread_cond_wait(&f->datacond, &f->datalock)) abort();
  cur = f->head;
  buf = cur->d;
  size = cur->l;
  f->head = cur->next;
  f->memusage -= size;
  if (f->head == NULL) f->tail = NULL;
  if (pthread_mutex_unlock(&f->datalock)) abort();
  free(cur);
  goto process;

process:
  if (pthread_mutex_lock(&f->lock)) abort();

  b = buf;
  while (size) {
    int l = write(f->s, b, size);
    if (l <= 0) { printf("forward error\n"); exit(1); }
    size -= l;
    b += l;
  }

  if (pthread_mutex_unlock(&f->lock)) abort();

  free(buf);

  goto wait;
}

static void do_forward(forward_data *f, int from, int to, int lock)
{
  int l, len;
  char *b;
  char buf[1024];
  while (1) {
    len = read(from, buf, 1024);
    if (len <= 0) break;
    b = buf;

    if (lock) if (pthread_mutex_lock(&f->lock)) abort();

    while (len) {
      l = write(to, b, len);
      if (l <= 0) break;
      len -= l;
      b += l;
    }

    if (lock) if (pthread_mutex_unlock(&f->lock)) abort();
  }
}

static void *forward_s_to_sc(void *_f)
{
  forward_data *f = _f;
  do_forward(f, f->s, f->sc, 0);
  return NULL;
}

void forward_start_client(void *_f, int s)
{
  forward_data *f = _f;
  f->sc = s;
  new_thread(forward_s_to_sc, f);
}

int get_connection(char *addr, int port);

void *forwarder(int port)
{
  forward_data *f;

  f = malloc(sizeof(*f)); if (f == NULL) abort();

  pthread_mutex_init(&f->lock, NULL);
  pthread_mutex_init(&f->datalock, NULL);
  pthread_cond_init(&f->datacond, NULL);

  f->sc = -1;
  f->head = f->tail = NULL;

  f->memusage = 0;
  f->last_warning_memusage = 0;

  printf("waiting for remote tracer on port %d\n", port);

  f->s = get_connection("127.0.0.1", port);

  printf("connected\n");

  new_thread(data_sender, f);

  return f;
}

void forward(void *_forwarder, char *buf, int size)
{
  forward_data *f = _forwarder;
  int32_t ssize = size;
  databuf *new;

  new = malloc(sizeof(*new)); if (new == NULL) abort();

  if (pthread_mutex_lock(&f->datalock)) abort();

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

  if (pthread_cond_signal(&f->datacond)) abort();
  if (pthread_mutex_unlock(&f->datalock)) abort();
}
