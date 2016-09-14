#ifndef TIMER_H_
#define TIMER_H_

#include <signal.h>

#define SIGTIMER SIGRTMIN

typedef enum timer_type_s {
  TIMER_PERIODIC,
  TIMER_ONE_SHOT,
  TIMER_TYPE_MAX,
} timer_type_t;

int timer_handle_signal(siginfo_t *info);

/** \brief Request a new timer
 *  \param interval_sec timer interval in seconds
 *  \param interval_us  timer interval in micro seconds
 *  \param task_id      task id of the task requesting the timer
 *  \param instance     instance of the task requesting the timer
 *  \param type         timer type
 *  \param timer_id     unique timer identifier
 *  @returns -1 on failure, 0 otherwise
 **/
int timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id);

/** \brief Remove the timer from list
 *  \param timer_id unique timer id
 *  @returns -1 on failure, 0 otherwise
 **/

int timer_remove(long timer_id);
#define timer_stop timer_remove

/** \brief Initialize timer task and its API
 *  \param mme_config MME common configuration
 *  @returns -1 on failure, 0 otherwise
 **/
int timer_init(void);

#endif
