#include "T.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
  T_connect_to_tracer("127.0.0.1", 2020);
  printf("after connect\n");
  while (1) {
    char *buf = "hello world %s!\n";
    //T(T_first, T_PRINTF("hello world %s!\n", "yo"));
    T(T_buf_test, T_BUFFER("hello world %s!\n", strlen(buf)+1));
    usleep(1);
  }
  while (1) pause();

  return 0;
}
