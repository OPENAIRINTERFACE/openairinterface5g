#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

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

static void *forward_sc_to_s(void *_f)
{
#if 0
  forward_data *f = _f;
  do_forward(f, f->sc, f->s, 1);
  printf("INFO: forwarder exits\n");
#endif
  return NULL;
}

void forward_start_client(void *_f, int s)
{
  forward_data *f = _f;
  f->sc = s;
  new_thread(forward_s_to_sc, f);
  new_thread(forward_sc_to_s, f);
}

void *forwarder(char *ip, int port)
{
  forward_data *f;
  struct sockaddr_in a;

  f = malloc(sizeof(*f)); if (f == NULL) abort();

  pthread_mutex_init(&f->lock, NULL);
  pthread_mutex_init(&f->datalock, NULL);
  pthread_cond_init(&f->datacond, NULL);

  f->sc = -1;
  f->head = f->tail = NULL;

  f->s = socket(AF_INET, SOCK_STREAM, 0);
  if (f->s == -1) { perror("socket"); exit(1); }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(ip);

  if (connect(f->s, (struct sockaddr *)&a, sizeof(a)) == -1)
    { perror("connect"); exit(1); }

  new_thread(data_sender, f);

  return f;
}

void forward(void *_forwarder, char *buf, int size)
{
  forward_data *f = _forwarder;
  databuf *new;

  new = malloc(sizeof(*new)); if (new == NULL) abort();

  if (pthread_mutex_lock(&f->datalock)) abort();

  new->d = malloc(size); if (new->d == NULL) abort();
  memcpy(new->d, buf, size);
  new->l = size;
  new->next = NULL;
  if (f->head == NULL) f->head = new;
  if (f->tail != NULL) f->tail->next = new;
  f->tail = new;

  if (pthread_cond_signal(&f->datacond)) abort();
  if (pthread_mutex_unlock(&f->datalock)) abort();
}
