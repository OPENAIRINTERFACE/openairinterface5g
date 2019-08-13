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
#include <signal.h>

#include "T.h"
#include "T_messages.txt.h"
#include "T_defs.h"
#include "T_IDs.h"

static T_cache_t *T_local_cache;
static int T_busylist_head;

typedef struct databuf {
  char *d;
  int l;
  struct databuf *next;
} databuf;

typedef struct {
  int socket_local;
  volatile int socket_remote;
  int remote_port;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  databuf *volatile head, *tail;
  uint64_t memusage;
  uint64_t last_warning_memusage;
} forward_data;

/****************************************************************************/
/*                      utility functions                                   */
/****************************************************************************/

static void new_thread(void *(*f)(void *), void *data) {
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

  if (pthread_attr_setstacksize(&att, 10000000)) {
    fprintf(stderr, "pthread_attr_setstacksize err\n");
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

static int get_connection(char *addr, int port) {
  struct sockaddr_in a;
  socklen_t alen;
  int s, t;
  printf("T tracer: waiting for connection on %s:%d\n", addr, port);
  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s == -1) {
    perror("socket");
    exit(1);
  }

  t = 1;

  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int))) {
    perror("setsockopt");
    exit(1);
  }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(addr);

  if (bind(s, (struct sockaddr *)&a, sizeof(a))) {
    perror("bind");
    exit(1);
  }

  if (listen(s, 5)) {
    perror("bind");
    exit(1);
  }

  alen = sizeof(a);
  t = accept(s, (struct sockaddr *)&a, &alen);

  if (t == -1) {
    perror("accept");
    exit(1);
  }

  close(s);
  printf("T tracer: connected\n");
  return t;
}

static void forward(void *_forwarder, char *buf, int size);

void send_T_messages_txt(void *forwarder) {
  char buf[T_BUFFER_MAX];
  char *T_LOCAL_buf = buf;
  int T_LOCAL_size;
  unsigned char *src;
  int src_len;
  /* trace T_message.txt
   * Send several messages -1 with content followed by message -2.
   */
  src = T_messages_txt;
  src_len = T_messages_txt_len;

  while (src_len) {
    int send_size = src_len;

    if (send_size > T_PAYLOAD_MAXSIZE - sizeof(int))
      send_size = T_PAYLOAD_MAXSIZE - sizeof(int);

    /* TODO: be careful, we use internal T stuff, to rewrite? */
    T_LOCAL_size = 0;
    T_HEADER(T_ID(-1));
    T_PUT_buffer(1, ((T_buffer) {
addr:
(src), length:
      (send_size)
    }));
    forward(forwarder, buf, T_LOCAL_size);
    src += send_size;
    src_len -= send_size;
  }

  T_LOCAL_size = 0;
  T_HEADER(T_ID(-2));
  forward(forwarder, buf, T_LOCAL_size);
}

/****************************************************************************/
/*                      forward functions                                   */
/****************************************************************************/

static void *data_sender(void *_f) {
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

  if (f->socket_remote != -1)
    while (size) {
      int l = write(f->socket_remote, b, size);

      if (l <= 0) {
        printf("T tracer: forward error\n");
        close(f->socket_remote);
        f->socket_remote = -1;
        break;
      }

      size -= l;
      b += l;
    }

  free(buf);
  goto wait;
}


static void *forward_remote_messages(void *_f) {
#define PUT(x) do { \
    if (bufsize == bufmaxsize) { \
      bufmaxsize += 4096; \
      buf = realloc(buf, bufmaxsize); \
      if (buf == NULL) abort(); \
    } \
    buf[bufsize] = x; \
    bufsize++; \
  } while (0)
#define PUT_BUF(x, l) do { \
    char *zz = (char *)(x); \
    int len = l; \
    while (len) { PUT(*zz); zz++; len--; } \
  } while (0)
  forward_data *f = _f;
  int from;
  int to;
  int l, len;
  char *b;
  char *buf = NULL;
  int bufsize = 0;
  int bufmaxsize = 0;
  char t;
again:

  while (1) {
    from = f->socket_remote;
    to = f->socket_local;
    bufsize = 0;
    /* let's read and process messages */
    len = read(from, &t, 1);

    if (len <= 0) goto dead;

    PUT(t);

    switch (t) {
      case 0:
      case 1:

        /* message 0 and 1: get a length and then 'length' numbers */
        if (read(from, &len, sizeof(int)) != sizeof(int)) goto dead;

        PUT_BUF(&len, 4);

        while (len) {
          if (read(from, &l, sizeof(int)) != sizeof(int)) goto dead;

          PUT_BUF(&l, 4);
          len--;
        }

        break;

      case 2:
        break;

      default:
        printf("%s:%d:%s: unhandled message type %d\n",
               __FILE__, __LINE__, __FUNCTION__, t);
        abort();
    }

    b = buf;

    while (bufsize) {
      l = write(to, b, bufsize);

      if (l <= 0) abort();

      bufsize -= l;
      b += l;
    }
  }

dead:
  /* socket died, let's stop all traces and wait for another tracer */
  /* TODO: be careful with those write, they might write less than wanted */
  buf[0] = 1;

  if (write(to, buf, 1) != 1) abort();

  len = T_NUMBER_OF_IDS;

  if (write(to, &len, sizeof(int)) != sizeof(int)) abort();

  l = 0;

  while (len) {
    if (write(to, &l, sizeof(int)) != sizeof(int)) abort();

    len--;
  };

  close(f->socket_remote);

  f->socket_remote = get_connection("0.0.0.0", f->remote_port);

  send_T_messages_txt(f);

  goto again;

  return NULL;
}

static void *forwarder(int port, int s) {
  forward_data *f;
  f = malloc(sizeof(*f));

  if (f == NULL) abort();

  pthread_mutex_init(&f->lock, NULL);
  pthread_cond_init(&f->cond, NULL);
  f->socket_local = s;
  f->head = f->tail = NULL;
  f->memusage = 0;
  f->last_warning_memusage = 0;
  printf("T tracer: waiting for remote tracer on port %d\n", port);
  f->remote_port = port;
  f->socket_remote = get_connection("0.0.0.0", port);
  send_T_messages_txt(f);
  new_thread(data_sender, f);
  new_thread(forward_remote_messages, f);
  return f;
}

static void forward(void *_forwarder, char *buf, int size) {
  forward_data *f = _forwarder;
  int32_t ssize = size;
  databuf *new;
  new = malloc(sizeof(*new));

  if (new == NULL) abort();

  if (pthread_mutex_lock(&f->lock)) abort();

  new->d = malloc(size + 4);

  if (new->d == NULL) abort();

  /* put the size of the message at the head */
  memcpy(new->d, &ssize, 4);
  memcpy(new->d+4, buf, size);
  new->l = size+4;
  new->next = NULL;

  if (f->head == NULL) f->head = new;

  if (f->tail != NULL) f->tail->next = new;

  f->tail = new;
#if BASIC_SIMULATOR

  /* When runnng the basic simulator, the tracer may be too slow.
   * Let's not take too much memory in the tracee and
   * wait if there is too much data to send. 200MB is
   * arbitrary.
   */
  while (f->memusage > 200 * 1024 * 1024) {
    if (pthread_cond_signal(&f->cond)) abort();

    if (pthread_mutex_unlock(&f->lock)) abort();

    usleep(1000);

    if (pthread_mutex_lock(&f->lock)) abort();
  }

#endif /* BASIC_SIMULATOR */
  f->memusage += size+4;

  /* warn every 100MB */
  if (f->memusage > f->last_warning_memusage &&
      f->memusage - f->last_warning_memusage > 100000000) {
    f->last_warning_memusage += 100000000;
    printf("T tracer: WARNING: memory usage is over %"PRIu64"MB\n",
           f->last_warning_memusage / 1000000);
  } else if (f->memusage < f->last_warning_memusage &&
             f->last_warning_memusage - f->memusage > 100000000) {
    f->last_warning_memusage = (f->memusage/100000000) * 100000000;
  }

  if (pthread_cond_signal(&f->cond)) abort();

  if (pthread_mutex_unlock(&f->lock)) abort();
}

/****************************************************************************/
/*                      local functions                                     */
/****************************************************************************/

static void wait_message(void) {
  while ((T_local_cache[T_busylist_head].busy & 0x02) == 0) usleep(1000);
}

void T_local_tracer_main(int remote_port, int wait_for_tracer,
                         int local_socket, void *shm_array) {
  int s;
  int port = remote_port;
  int dont_wait = wait_for_tracer ? 0 : 1;
  void *f;

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    printf("local tracer received SIGPIPE\n");
    abort();
  }

  T_local_cache = shm_array;
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
    forward(f, T_local_cache[T_busylist_head].buffer,
            T_local_cache[T_busylist_head].length);
    T_local_cache[T_busylist_head].busy = 0;
    T_busylist_head++;
    T_busylist_head &= T_CACHE_SIZE - 1;
  }
}
