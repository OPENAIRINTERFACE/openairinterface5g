#include "T.h"
#include "T_messages.txt.h"
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

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
int T_busylist_head;
T_cache_t *T_cache;

static void get_message(int s)
{
  char t;
  int l;
  int id;
  int is_on;

  if (read(s, &t, 1) != 1) abort();
printf("got mess %d\n", t);
  switch (t) {
  case 0:
    /* toggle all those IDs */
    /* optimze? (too much syscalls) */
    if (read(s, &l, sizeof(int)) != sizeof(int)) abort();
    while (l) {
      if (read(s, &id, sizeof(int)) != sizeof(int)) abort();
      T_IDs[id] = 1 - T_IDs[id];
      l--;
    }
    break;
  case 1:
    /* set IDs as given */
    /* optimize? */
    if (read(s, &l, sizeof(int)) != sizeof(int)) abort();
    id = 0;
    while (l) {
      if (read(s, &is_on, sizeof(int)) != sizeof(int)) abort();
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

void T_connect_to_tracer(char *addr, int port)
{
  struct sockaddr_in a;
  int s;
  int T_shm_fd;
  unsigned char *buf;
  int len;

  if (strcmp(addr, "127.0.0.1") != 0) {
    printf("error: local tracer must be on same host\n");
    abort();
  }

  printf("connecting to local tracer on port %d\n", port);
again:
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) { perror("socket"); exit(1); }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(s, (struct sockaddr *)&a, sizeof(a)) == -1) {
    perror("connect");
    close(s);
    printf("trying again in 1s\n");
    sleep(1);
    goto again;
  }

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

  /* trace T_message.txt
   * Send several messages -1 with content followed by message -2.
   * We can't use the T macro directly, events -1 and -2 are special.
   */
  buf = T_messages_txt;
  len = T_messages_txt_len;
  while (len) {
    int send_size = len;
    if (send_size > T_PAYLOAD_MAXSIZE - sizeof(int))
      send_size = T_PAYLOAD_MAXSIZE - sizeof(int);
    do {
      T_LOCAL_DATA
      T_HEADER(T_ID(-1));
      T_PUT_buffer(1, ((T_buffer){addr:(buf), length:(len)}));
      T_COMMIT();
    } while (0);
    buf += send_size;
    len -= send_size;
  }
  do {
    T_LOCAL_DATA
    T_HEADER(T_ID(-2));
    T_COMMIT();
  } while (0);
}
