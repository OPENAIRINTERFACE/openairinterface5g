/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <execinfo.h>

#include "backtrace.h"

/* Obtain a backtrace and print it to stdout. */
void display_backtrace(void) {
  void *array[10];
  size_t size;
  char **strings;
  size_t i;
  char *test=getenv("NO_BACKTRACE");

  if (test!=0) abort();

  size = backtrace(array, 10);
  strings = backtrace_symbols(array, size);
  printf("Obtained %u stack frames.\n", (unsigned int)size);

  for (i = 0; i < size; i++)
    printf("%s\n", strings[i]);

  free(strings);
}

void backtrace_handle_signal(siginfo_t *info) {
  display_backtrace();
  //exit(EXIT_FAILURE);
}
