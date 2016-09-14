#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <execinfo.h>

#include "backtrace.h"

/* Obtain a backtrace and print it to stdout. */
void display_backtrace(void)
{
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace(array, 10);
  strings = backtrace_symbols(array, size);

  printf("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
    printf("%s\n", strings[i]);

  free(strings);
}

void backtrace_handle_signal(siginfo_t *info)
{
  display_backtrace();
  //exit(EXIT_FAILURE);
}
