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

#define DEFAULT_REMOTE_IP   "127.0.0.1"
#define DEFAULT_REMOTE_PORT 2021

#include "defs.h"

#include "../T_defs.h"

T_cache_t *T_cache;
int T_busylist_head;
int T_pos;

int get_connection(char *addr, int port)
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

void wait_message(void)
{
  while (T_cache[T_busylist_head].busy == 0) usleep(1000);
}

void init_shm(void)
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

void usage(void)
{
  printf(
"tracer - local side\n"
"options:\n"
"    -r <IP address> <port>    forwards packets to remote IP:port\n"
"                              (default %s:%d)\n",
    DEFAULT_REMOTE_IP, DEFAULT_REMOTE_PORT
  );
  exit(1);
}

int main(int n, char **v)
{
  int s;
  int i;
  char *remote_ip = DEFAULT_REMOTE_IP;
  int remote_port = DEFAULT_REMOTE_PORT;
  int port = 2020;
  void *f;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-l")) { if (i > n-3) usage();
      remote_ip = v[++i]; remote_port = atoi(v[++i]); continue; }
    printf("ERROR: unknown option %s\n", v[i]);
    usage();
  }

  f = forwarder(remote_ip, remote_port);
  init_shm();
  s = get_connection("127.0.0.1", port);

  forward_start_client(f, s);

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
  return 0;
}
