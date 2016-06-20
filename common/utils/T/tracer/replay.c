#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "utils.h"
#include "../T_defs.h"

#define DEFAULT_REMOTE_PORT 2021

/* replay.c does not know anything about events - it just has the array
 * is_on that grows each time replay.c sees a type that does not fit in
 * there (the idea is to isolate replay.c as maximum by not using unneeded
 * information)
 */
int *is_on;
int is_on_size;

/* this lock is used to protect access to is_on/is_on_size */
pthread_mutex_t biglock = PTHREAD_MUTEX_INITIALIZER;

void set_is_on_size(int size)
{
  is_on = realloc(is_on, size * sizeof(int)); if (is_on == NULL) abort();
  memset(is_on + is_on_size, 0, (size - is_on_size) * sizeof(int));
  is_on_size = size;
}

void lock(void)
{
  if (pthread_mutex_lock(&biglock)) abort();
}

void unlock(void)
{
  if (pthread_mutex_unlock(&biglock)) abort();
}

#define QUIT(x) do { printf("%s\n", x); exit(1); } while(0)

void get_message(int s)
{
  char t;
  int l;
  int id;
  int on;

  if (read(s, &t, 1) != 1) QUIT("get_message fails");
  lock();
  switch (t) {
  case 0:
    /* toggle all those IDs */
    if (read(s, &l, sizeof(int)) != sizeof(int)) QUIT("get_message fails");
    while (l) {
      if (read(s, &id, sizeof(int)) != sizeof(int)) QUIT("get_message fails");
      if (id > is_on_size - 1) set_is_on_size(id + 1);
      is_on[id] = 1 - is_on[id];
      l--;
    }
    break;
  case 1:
    /* set IDs as given */
    /* optimize? */
    if (read(s, &l, sizeof(int)) != sizeof(int)) QUIT("get_message fails");
    if (l > is_on_size) set_is_on_size(l);
    id = 0;
    while (l) {
      if (read(s, &on, sizeof(int)) != sizeof(int))
        QUIT("get_message fails");
      is_on[id] = on;
      id++;
      l--;
    }
    break;
  case 2: break; /* do nothing, this message is to wait for local tracer */
  default: abort();
  }
  unlock();
}

void *get_message_thread(void *_socket)
{
  int socket = *(int *)_socket;

  while (1) get_message(socket);

  return NULL;
}

void usage(void)
{
  printf(
"options:\n"
"    -i <input file>           this option is mandatory\n"
"    -p <port>                 wait connection on given port (default %d)\n"
"    -w                        user must press a key after each sent event\n",
  DEFAULT_REMOTE_PORT
  );
  exit(1);
}

#define ERR printf("ERROR: read file %s failed\n", input_filename)

int main(int n, char **v)
{
  int port = DEFAULT_REMOTE_PORT;
  char *input_filename = NULL;
  int i;
  int socket;
  FILE *in;
  int do_send;
  int do_wait = 0;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-i"))
      { if (i > n-2) usage(); input_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-p"))
      { if (i > n-2) usage(); port = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-w")) { do_wait = 1; continue; }
    usage();
  }

  if (input_filename == NULL) {
    printf("ERROR: provide an input file (-i)\n");
    exit(1);
  }

  in = fopen(input_filename, "r");
  if (in == NULL) { perror(input_filename); abort(); }

  socket = get_connection("0.0.0.0", port);

  /* get first message to activate selected traces */
  get_message(socket);

  new_thread(get_message_thread, &socket);

  while (1) {
    int type;
    int32_t length;
    char v[T_BUFFER_MAX];
    int vpos = 0;

    /* read event from file */
    if (fread(&length, 4, 1, in) != 1) break;
    memcpy(v+vpos, &length, 4);
    vpos += 4;
#ifdef T_SEND_TIME
    if (length < sizeof(struct timespec)) { ERR; break; }
    if (fread(v+vpos, sizeof(struct timespec), 1, in) != 1) { ERR; break; }
    vpos += sizeof(struct timespec);
    length -= sizeof(struct timespec);
#endif
    if (length < sizeof(int)) { ERR; break; }
    if (fread(&type, sizeof(int), 1, in) != 1) { ERR; break; }
    memcpy(v+vpos, &type, sizeof(int));
    vpos += sizeof(int);
    length -= sizeof(int);
    if (length) if (fread(v+vpos, length, 1, in) != 1) { ERR; break; }
    vpos += length;

    /* only send if configured to do so */
    lock();
    if (type < 0) do_send = 1;
    else {
      if (type > is_on_size - 1) set_is_on_size(type+1);
      do_send = is_on[type];
    }
    unlock();

    if (do_send)
      if (socket_send(socket, v, vpos) != 0)
        { printf("ERROR: socket writing failed\n"); abort(); }

    if (do_send && do_wait) getchar();
  }

  fclose(in);
  close(socket);

  return 0;
}
