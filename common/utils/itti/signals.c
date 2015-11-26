/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <time.h>
#include <errno.h>

#include "intertask_interface.h"
#include "timer.h"
#include "backtrace.h"
#include "assertions.h"

#include "signals.h"
#include "log.h"

#if defined (LOG_D) && defined (LOG_E)
# define SIG_DEBUG(x, args...)  LOG_D(EMU, x, ##args)
# define SIG_ERROR(x, args...)  LOG_E(EMU, x, ##args)
#endif

#ifndef SIG_DEBUG
# define SIG_DEBUG(x, args...)  do { fprintf(stdout, "[SIG][D]"x, ##args); } while(0)
#endif
#ifndef SIG_ERROR
# define SIG_ERROR(x, args...)  do { fprintf(stdout, "[SIG][E]"x, ##args); } while(0)
#endif

static sigset_t set;

int signal_mask(void)
{
  /* We set the signal mask to avoid threads other than the main thread
   * to receive the timer signal. Note that threads created will inherit this
   * configuration.
   */
  sigemptyset(&set);

  sigaddset (&set, SIGTIMER);
  sigaddset (&set, SIGUSR1);
  sigaddset (&set, SIGABRT);
  sigaddset (&set, SIGSEGV);
  sigaddset (&set, SIGINT);

  if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
    perror ("sigprocmask");
    return -1;
  }

  return 0;
}

int signal_handle(int *end)
{
  int ret;
  siginfo_t info;

  sigemptyset(&set);

  sigaddset (&set, SIGTIMER);
  sigaddset (&set, SIGUSR1);
  sigaddset (&set, SIGABRT);
  sigaddset (&set, SIGSEGV);
  sigaddset (&set, SIGINT);

  if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
    perror ("sigprocmask");
    return -1;
  }

  /* Block till a signal is received.
   * NOTE: The signals defined by set are required to be blocked at the time
   * of the call to sigwait() otherwise sigwait() is not successful.
   */
  if ((ret = sigwaitinfo(&set, &info)) == -1) {
    perror ("sigwait");
    return ret;
  }

  //printf("Received signal %d\n", info.si_signo);

  /* Real-time signals are non constant and are therefore not suitable for
   * use in switch.
   */
  if (info.si_signo == SIGTIMER) {
    timer_handle_signal(&info);
  } else {
    /* Dispatch the signal to sub-handlers */
    switch(info.si_signo) {
    case SIGUSR1:
      SIG_DEBUG("Received SIGUSR1\n");
      *end = 1;
      break;

    case SIGSEGV:   /* Fall through */
    case SIGABRT:
      SIG_DEBUG("Received SIGABORT\n");
      backtrace_handle_signal(&info);
      break;

    case SIGINT:
      printf("Received SIGINT\n");
      itti_send_terminate_message(TASK_UNKNOWN);
      *end = 1;
      break;

    default:
      SIG_ERROR("Received unknown signal %d\n", info.si_signo);
      break;
    }
  }

  return 0;
}
