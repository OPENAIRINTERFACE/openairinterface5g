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

/* This module provides a separate process to run system().
 * The communication between this process and the main processing
 * is done through unix pipes.
 *
 * Motivation: the UE sets its IP address using system() and
 * that disrupts realtime processing in some cases. Having a
 * separate process solves this problem.
 */

#define _GNU_SOURCE
#include "system.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#define MAX_COMMAND 4096

static int command_pipe_read;
static int command_pipe_write;
static int result_pipe_read;
static int result_pipe_write;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int module_initialized = 0;

/********************************************************************/
/* util functions                                                   */
/********************************************************************/

static void lock_system(void) {
  if (pthread_mutex_lock(&lock) != 0) {
    printf("pthread_mutex_lock fails\n");
    abort();
  }
}

static void unlock_system(void) {
  if (pthread_mutex_unlock(&lock) != 0) {
    printf("pthread_mutex_unlock fails\n");
    abort();
  }
}

static void write_pipe(int p, char *b, int size) {
  while (size) {
    int ret = write(p, b, size);

    if (ret <= 0) exit(0);

    b += ret;
    size -= ret;
  }
}

static void read_pipe(int p, char *b, int size) {
  while (size) {
    int ret = read(p, b, size);

    if (ret <= 0) exit(0);

    b += ret;
    size -= ret;
  }
}

/********************************************************************/
/* background process                                               */
/********************************************************************/

/* This function is run by background process. It waits for a command,
 * runs it, and reports status back. It exits (in normal situations)
 * when the main process exits, because then a "read" on the pipe
 * will return 0, in which case "read_pipe" exits.
 */
static void background_system_process(void) {
  int len;
  int ret;
  char command[MAX_COMMAND+1];

  while (1) {
    read_pipe(command_pipe_read, (char *)&len, sizeof(int));
    read_pipe(command_pipe_read, command, len);
    ret = system(command);
    write_pipe(result_pipe_write, (char *)&ret, sizeof(int));
  }
}

/********************************************************************/
/* background_system()                                              */
/*     return -1 on error, 0 on success                             */
/********************************************************************/

int background_system(char *command) {
  int res;
  int len;

  if (module_initialized == 0) {
    printf("FATAL: calling 'background_system' but 'start_background_system' was not called\n");
    abort();
  }

  len = strlen(command)+1;

  if (len > MAX_COMMAND) {
    printf("FATAL: command too long. Increase MAX_COMMAND (%d).\n", MAX_COMMAND);
    printf("command was: '%s'\n", command);
    abort();
  }

  /* only one command can run at a time, so let's lock/unlock */
  lock_system();
  write_pipe(command_pipe_write, (char *)&len, sizeof(int));
  write_pipe(command_pipe_write, command, len);
  read_pipe(result_pipe_read, (char *)&res, sizeof(int));
  unlock_system();

  if (res == -1 || !WIFEXITED(res) || WEXITSTATUS(res) != 0) return -1;

  return 0;
}

/********************************************************************/
/* start_background_system()                                        */
/*     initializes the "background system" module                   */
/*     to be called very early by the main processing               */
/********************************************************************/

void start_background_system(void) {
  int p[2];
  pid_t son;
  module_initialized = 1;

  if (pipe(p) == -1) {
    perror("pipe");
    exit(1);
  }

  command_pipe_read  = p[0];
  command_pipe_write = p[1];

  if (pipe(p) == -1) {
    perror("pipe");
    exit(1);
  }

  result_pipe_read  = p[0];
  result_pipe_write = p[1];
  son = fork();

  if (son == -1) {
    perror("fork");
    exit(1);
  }

  if (son) {
    close(result_pipe_write);
    close(command_pipe_read);
    return;
  }

  close(result_pipe_read);
  close(command_pipe_write);
  background_system_process();
}


void threadCreate(pthread_t* t, void * (*func)(void*), void * param, char* name, int affinity, int priority){
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  struct sched_param sparam={0};
  sparam.sched_priority = priority;
  pthread_attr_setschedparam(&attr, &sparam);

  pthread_create(t, &attr, func, param);

  pthread_setname_np(*t, name);
  if (affinity != -1 ) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(affinity, &cpuset);
    AssertFatal( pthread_setaffinity_np(*t, sizeof(cpu_set_t), &cpuset) == 0, "Error setting processor affinity");
  }

  pthread_attr_destroy(&attr);
}

// Block CPU C-states deep sleep
void configure_linux(void) {
  int ret;
  static int latency_target_fd=-1;
  uint32_t latency_target_value=10; // in microseconds
  if (latency_target_fd == -1) {
    if ( (latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR)) != -1 ) {
      ret = write(latency_target_fd, &latency_target_value, sizeof(latency_target_value));
      if (ret == 0) {
	printf("# error setting cpu_dma_latency to %d!: %s\n", latency_target_value, strerror(errno));
	close(latency_target_fd);
	latency_target_fd=-1;
	return;
      }
    }
  }
  if (latency_target_fd != -1) 
    LOG_I(HW,"# /dev/cpu_dma_latency set to %dus\n", latency_target_value);
  else
    LOG_E(HW,"Can't set /dev/cpu_dma_latency to %dus\n", latency_target_value);

  // Set CPU frequency to it's maximum
  if ( 0 != system("for d in /sys/devices/system/cpu/cpu[0-9]*; do cat $d/cpufreq/cpuinfo_max_freq > $d/cpufreq/scaling_min_freq; done"))
	  LOG_W(HW,"Can't set cpu frequency\n");
  
}
