/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
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
#include <stdbool.h>
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

  if (module_initialized == 1)
    return;

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

int rt_sleep_ns (uint64_t x)
{
  struct timespec myTime;
  clock_gettime(CLOCK_MONOTONIC, &myTime);
  myTime.tv_sec += x/1000000000ULL;
  myTime.tv_nsec = x%1000000000ULL;
  if (myTime.tv_nsec>=1000000000) {
    myTime.tv_nsec -= 1000000000;
    myTime.tv_sec++;
  }

  return clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &myTime, NULL);
}

#ifdef HAVE_LIB_CAP
#include <sys/capability.h>
/* \brief reports if the current thread has capability CAP_SYS_NICE, i.e. */
bool has_cap_sys_nice(void)
{
  /* get capabilities of calling PID */
  cap_t cap = cap_get_pid(0);
  cap_flag_value_t val;
  /* check to what CAP_SYS_NICE is currently ("effective capability") set */
  int ret = cap_get_flag(cap, CAP_SYS_NICE, CAP_EFFECTIVE, &val);
  AssertFatal(ret == 0, "Error in cap_get_flag(): ret %d errno %d\n", ret, errno);
  cap_free(cap);
  /* return true if CAP_SYS_NICE is currently set */
  return val == CAP_SET;
}
#else
/* libcap has not been detected on this system. We do not need to require it --
 * we can try to read directly via a syscall. This is discouraged, though; from
 * the man page: "The portable interfaces are cap_set_proc(3) and
 * cap_get_proc(3); if possible, you should use those interfaces in
 * applications". */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <linux/capability.h> /* capabilities used below */
/* \brief reports if the current thread has capability CAP_SYS_NICE, i.e. */
bool has_cap_sys_nice(void)
{
  struct __user_cap_header_struct hdr = {.version = _LINUX_CAPABILITY_VERSION_3};
  struct __user_cap_data_struct cap[2];
  if (syscall(SYS_capget, &hdr, cap) == -1)
    return false;
  return (cap[0].effective & (1 << CAP_SYS_NICE)) != 0;
}
#endif

void threadCreate(pthread_t* t, void * (*func)(void*), void * param, char* name, int affinity, int priority)
{
  int ret;
  bool set_prio = has_cap_sys_nice();

  pthread_attr_t attr;
  ret=pthread_attr_init(&attr);
  AssertFatal(ret == 0, "Error in pthread_attr_init(): ret: %d, errno: %d\n", ret, errno);

  if (set_prio) {
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    AssertFatal(ret == 0, "Error in pthread_attr_setinheritsched(): ret: %d, errno: %d\n", ret, errno);
    ret = pthread_attr_setschedpolicy(&attr, SCHED_OAI);
    AssertFatal(ret == 0, "Error in pthread_attr_setschedpolicy(): ret: %d, errno: %d\n", ret, errno);
    AssertFatal(priority >= sched_get_priority_min(SCHED_OAI) && priority <= sched_get_priority_max(SCHED_OAI),
                "Scheduling priority %d not possible: must be within [%d, %d]\n",
                priority,
                sched_get_priority_min(SCHED_OAI),
                sched_get_priority_max(SCHED_OAI));
    AssertFatal(priority <= sched_get_priority_max(SCHED_OAI), "not possible priority %d", priority);
    struct sched_param sparam = {0};
    sparam.sched_priority = priority;
    ret = pthread_attr_setschedparam(&attr, &sparam);
    AssertFatal(ret == 0, "Error in pthread_attr_setschedparam(): ret: %d errno: %d\n", ret, errno);
    LOG_I(UTIL, "%s() for %s: creating thread with affinity %x, priority %d\n", __func__, name, affinity, priority);
  } else {
    affinity = -1;
    priority = -1;
    LOG_I(UTIL, "%s() for %s: creating thread (no affinity, default priority)\n", __func__, name);
  }

  if (affinity != -1) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(affinity, &cpuset);
    /* Pin before create; post-create setaffinity can hang on a busy RT thread. */
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
    AssertFatal(ret == 0, "Error in pthread_attr_setaffinity_np(): ret: %d, errno: %d\n", ret, errno);
  }

  ret=pthread_create(t, &attr, func, param);
  AssertFatal(ret == 0, "Error in pthread_create(): ret: %d, errno: %d\n", ret, errno);

  char short_name[16];
  strncpy(short_name, name, sizeof(short_name) - 1);
  short_name[sizeof(short_name) - 1] = '\0';
  ret = pthread_setname_np(*t, short_name);
  // pthread_create() can initialize and complete before setting a name
  AssertFatal(ret == 0 || ret == ENOENT || ret == ESRCH, "Error in pthread_setname_np(): ret: %d, errno: %d\n", ret, errno);
  if (ret != 0) {
    LOG_W(UTIL,
          "pthread_setname_np() failed with ret: %d (%s) when naming %s - \
                thread likely already exited, continuing\n",
          ret,
          strerror(ret),
          name);
  }

  pthread_attr_destroy(&attr);
}

// Legacy, pthread_create + thread_top_init() should be replaced by threadCreate
// threadCreate encapsulates the posix pthread api
void thread_top_init(char *thread_name)
{
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;
  int settingPriority = 1;

  /* Set affinity mask to include CPUs 2 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  /* CPU 1 is reserved for all RX_TX threads */
  /* Enable CPU Affinity only if number of CPUs > 2 */
  CPU_ZERO(&cpuset);

  /* Check the actual affinity mask assigned to the thread */
  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity,0,sizeof(cpu_affinity));
  for (j = 0; j < 1024; j++)
  {
    if (CPU_ISSET(j, &cpuset))
    {  
      char temp[1024];
      sprintf (temp, " CPU_%d", j);
      strcat(cpu_affinity, temp);
    }
  }

  if (settingPriority) {
    memset(&sparam, 0, sizeof(sparam));
    sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
    policy = SCHED_FIFO;
  
    s = pthread_setschedparam(pthread_self(), policy, &sparam);
    if (s != 0) {
      perror("pthread_setschedparam : ");
      exit_fun("Error setting thread priority");
    }
  
    s = pthread_getschedparam(pthread_self(), &policy, &sparam);
    if (s != 0) {
      perror("pthread_getschedparam : ");
      exit_fun("Error getting thread priority");
    }

    pthread_setname_np(pthread_self(), thread_name);

    LOG_I(HW, "[SCHED][eNB] %s started on CPU %d, sched_policy = %s , priority = %d, CPU Affinity=%s \n",thread_name,sched_getcpu(),
                     (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                     (policy == SCHED_RR)    ? "SCHED_RR" :
                     (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                     "???",
                     sparam.sched_priority, cpu_affinity );
  }
}

/* \brief lock memory to RAM to avoid delays */
void lock_memory_to_ram(void)
{
  int rc = mlockall(MCL_CURRENT | MCL_FUTURE);
  if (rc != 0)
    LOG_W(UTIL, "mlockall() failed: %d, %s\n", errno, strerror(errno));
}
