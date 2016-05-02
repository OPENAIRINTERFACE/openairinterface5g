#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

void new_thread(void *(*f)(void *), void *data)
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

void sleepms(int ms)
{
  struct timespec t;

  t.tv_sec = ms / 1000;
  t.tv_nsec = (ms % 1000) * 1000000L;

  /* TODO: deal with EINTR */
  if (nanosleep(&t, NULL)) abort();
}

/****************************************************************************/
/* list                                                                     */
/****************************************************************************/

list *list_remove_head(list *l)
{
  list *ret;
  if (l == NULL) return NULL;
  ret = l->next;
  if (ret != NULL) ret->last = l->last;
  free(l);
  return ret;
}

list *list_append(list *l, void *data)
{
  list *new = calloc(1, sizeof(list));
  if (new == NULL) abort();
  new->data = data;
  if (l == NULL) {
    new->last = new;
    return new;
  }
  l->last->next = new;
  l->last = new;
  return l;
}

/****************************************************************************/
/* socket                                                                   */
/****************************************************************************/

void socket_send(int socket, void *buffer, int size)
{
  char *x = buffer;
  int ret;
  while (size) {
    ret = write(socket, x, size);
    if (ret <= 0) abort();
    size -= ret;
    x += ret;
  }
}

/****************************************************************************/
/* buffer                                                                   */
/****************************************************************************/

void PUTC(OBUF *o, char c)
{
  if (o->osize == o->omaxsize) {
    o->omaxsize += 512;
    o->obuf = realloc(o->obuf, o->omaxsize);
    if (o->obuf == NULL) abort();
  }
  o->obuf[o->osize] = c;
  o->osize++;
}

void PUTS(OBUF *o, char *s)
{
  while (*s) PUTC(o, *s++);
}

static int clean(char c)
{
  if (!isprint(c)) c = ' ';
  return c;
}

void PUTS_CLEAN(OBUF *o, char *s)
{
  while (*s) PUTC(o, clean(*s++));
}

void PUTI(OBUF *o, int i)
{
  char s[64];
  sprintf(s, "%d", i);
  PUTS(o, s);
}
