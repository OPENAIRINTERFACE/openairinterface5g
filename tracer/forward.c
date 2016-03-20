#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
  int s;
  int sc;
  pthread_mutex_t lock;
} forward_data;

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
  forward_data *f = _f;
  do_forward(f, f->sc, f->s, 1);
  printf("INFO: forwarder exits\n");
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

  f->s = socket(AF_INET, SOCK_STREAM, 0);
  if (f->s == -1) { perror("socket"); exit(1); }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(ip);

  if (connect(f->s, (struct sockaddr *)&a, sizeof(a)) == -1)
    { perror("connect"); exit(1); }

  return f;
}

void forward(void *_forwarder, char *buf, int size)
{
  forward_data *f = _forwarder;

  if (pthread_mutex_lock(&f->lock)) abort();

  while (size) {
    int l = write(f->s, buf, size);
    if (l <= 0) { printf("forward error\n"); exit(1); }
    size -= l;
    buf += l;
  }

  if (pthread_mutex_unlock(&f->lock)) abort();
}
