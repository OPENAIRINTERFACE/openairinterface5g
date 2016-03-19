#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct {
  int s;
} forward_data;

void *forwarder(char *ip, int port)
{
  forward_data *f;
  struct sockaddr_in a;

  f = malloc(sizeof(*f)); if (f == NULL) abort();

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

  while (size) {
    int l = write(f->s, buf, size);
    if (l <= 0) { printf("forward error\n"); exit(1); }
    size -= l;
    buf += l;
  }
}
