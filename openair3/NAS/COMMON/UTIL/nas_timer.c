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

/*****************************************************************************
Source      nas_timer.c

Version     0.1

Date        2012/10/09

Product     NAS stack

Subsystem   Utilities

Author      Frederic Maurel

Description Timer utilities

*****************************************************************************/

#include <pthread.h>
#include <assert.h>
#include <stdint.h>

#include <string.h> // memset
#include <stdlib.h> // malloc, free
#include <sys/time.h>   // setitimer

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# include "timer.h"
#else
# include <signal.h>
# include <time.h>   // clock_gettime
#endif
#include "nas_timer.h"
#include "commonDef.h"
#include "msc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* Structure of an interval timer entry
 * ------------------------------------
 * The callback function is scheduled to be executed upon expiration of
 * the timer that has been previously setup to the initial interval time
 * value when the timer entry was allocated.
 */
typedef struct {
#if defined(ENABLE_ITTI)
  long timer_id;          /* Timer id returned by the timer API from ITTI */
#else
  pthread_t pid;          /* Thread identifier of the callback    */
#endif

  struct timeval itv;     /* Initial interval timer value         */
  struct timeval tv;      /* Interval timer value                 */

  nas_timer_callback_t cb;    /* Callback executed at timer expiration */
  void *args;                 /* Callback argument parameters          */
} nas_timer_entry_t;

/* Structure of a timer queue - list of active interval timer entries
 * ------------------------------------------------------------------
 * At any time, the first entry of the queue is always associated to the
 * first timer that will come to expire. Upon timer expiration, the first
 * entry is removed from the queue and freed.
 */
typedef struct _nas_timer_queue_t {
  int id;         /* Identifier of the current timer entry */
  nas_timer_entry_t *entry;   /* The current timer entry       */
  struct _nas_timer_queue_t *prev;/* The previous timer entry in the queue */
  struct _nas_timer_queue_t *next;/* The next timer entry in the queue     */
} timer_queue_t;

/* Structure of a timer database
 * -----------------------------
 * The timer database is managed to provide unique identifier to timer at
 * startup and to maintain an ordered queue of active timer entries.
 */
typedef struct {
  int timer_id;   /* Identifier of the first available timer entry */
#define TIMER_DATABASE_SIZE 256
  timer_queue_t tq[TIMER_DATABASE_SIZE];
  timer_queue_t *head;/* Pointer to the first timer entry to be fired  */

#if !defined(ENABLE_ITTI)
  pthread_mutex_t mutex;
#endif
} nas_timer_database_t;

/*
 * The timer database
 */
static nas_timer_database_t _nas_timer_db = {
  0,
  {},
  NULL
#if !defined(ENABLE_ITTI)
  , PTHREAD_MUTEX_INITIALIZER
#endif
};

#if defined(ENABLE_ITTI)
#define nas_timer_lock_db()
#define nas_timer_unlock_db()
#else
#define nas_timer_lock_db()     pthread_mutex_lock(&_nas_timer_db.mutex)
#define nas_timer_unlock_db()   pthread_mutex_unlock(&_nas_timer_db.mutex)
#endif

/*
 * The handler executed whenever the system timer expires
 */
#if !defined(ENABLE_ITTI)
static void _nas_timer_handler(int signal);
#endif

/*
 * -----------------------------------------------------------------------------
 *      Functions used to manage the timer database
 * -----------------------------------------------------------------------------
 */
static void _nas_timer_db_init(void);

static int _nas_timer_db_get_id(void);
static int _nas_timer_db_is_active(int id);
static nas_timer_entry_t *_nas_timer_db_create_entry(long sec,
    nas_timer_callback_t cb, void *args);
static void _nas_timer_db_delete_entry(int id);

static void _nas_timer_db_insert_entry(int id, nas_timer_entry_t *te);
static int _nas_timer_db_insert(timer_queue_t *entry);

static nas_timer_entry_t *_nas_timer_db_remove_entry(int id);
static int _nas_timer_db_remove(timer_queue_t *entry);

/*
 * -----------------------------------------------------------------------------
 *      Operator functions for timeval structures
 * -----------------------------------------------------------------------------
 */
static int _nas_timer_cmp(const struct timeval *a, const struct timeval *b);
static void _nas_timer_add(const struct timeval *a, const struct timeval *b,
                           struct timeval *result);
static int _nas_timer_sub(const struct timeval *a, const struct timeval *b,
                          struct timeval *result);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    timer_init()                                              **
 **                                                                        **
 ** Description: Initializes internal data used to manage timers           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
int nas_timer_init(void)
{
  /* Initialize the timer database */
  _nas_timer_db_init();

#if !defined(ENABLE_ITTI)
  /* Setup the timer database handler */
  struct sigaction act;

  (void) memset (&act, 0, sizeof (act));
  (void) sigfillset (&act.sa_mask);
  (void) sigdelset (&act.sa_mask, SIGHUP);
  (void) sigdelset (&act.sa_mask, SIGINT);
  (void) sigdelset (&act.sa_mask, SIGTERM);
  (void) sigdelset (&act.sa_mask, SIGILL);
  (void) sigdelset (&act.sa_mask, SIGTRAP);
  (void) sigdelset (&act.sa_mask, SIGIOT);
# ifndef LINUX
  (void) sigdelset (&act.sa_mask, SIGEMT);
# endif
  (void) sigdelset (&act.sa_mask, SIGFPE);
  (void) sigdelset (&act.sa_mask, SIGBUS);
  (void) sigdelset (&act.sa_mask, SIGSEGV);
  (void) sigdelset (&act.sa_mask, SIGSYS);

  act.sa_handler = _nas_timer_handler;

  if ( sigaction (SIGALRM, &act, 0) < 0 ) {
    return (RETURNerror);
  }

#endif
  return (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    timer_start()                                             **
 **                                                                        **
 ** Description: Schedules the execution of the given callback function    **
 **      upon expiration of the specified time interval            **
 **                                                                        **
 ** Inputs:  sec:       The value of the time interval in seconds  **
 **      cb:        Function executed upon timer expiration    **
 **      args:      Callback argument parameters               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The timer identifier when successfully     **
 **             started; NAS_TIMER_INACTIVE_ID otherwise.  **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_timer_start(long sec, nas_timer_callback_t cb, void *args)
{
  int id;
  nas_timer_entry_t *te;
#if defined(ENABLE_ITTI)
  int ret;
  long timer_id;
#endif

  /* Do not start null timer */
  if (sec == 0) {
    return (NAS_TIMER_INACTIVE_ID);
  }

  /* Get an identifier for the new timer entry */
  id = _nas_timer_db_get_id();

  if (id < 0) {
    /* No available timer entry found */
    return (NAS_TIMER_INACTIVE_ID);
  }

  /* Create a new timer entry */
  te = _nas_timer_db_create_entry(sec, cb, args);

  if (te == NULL) {
    return (NAS_TIMER_INACTIVE_ID);
  }

  /* Insert the new entry into the timer queue */
  _nas_timer_db_insert_entry(id, te);
#if defined(ENABLE_ITTI)
# if defined(NAS_MME)
  ret = timer_setup(sec, 0, TASK_NAS_MME, INSTANCE_DEFAULT, TIMER_PERIODIC, args, &timer_id);
# else
  ret = timer_setup(sec, 0, TASK_NAS_UE, INSTANCE_DEFAULT, TIMER_PERIODIC, args, &timer_id);
# endif

  if (ret == -1) {
    return NAS_TIMER_INACTIVE_ID;
  }

  te->timer_id = timer_id;
#endif

  return (id);
}

/****************************************************************************
 **                                                                        **
 ** Name:    timer_stop()                                              **
 **                                                                        **
 ** Description: Stop the timer with the specified identifier              **
 **                                                                        **
 ** Inputs:  id:        The identifier of the timer to be stopped  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    NAS_TIMER_INACTIVE_ID when successfully stop-  **
 **             ped; The timer identifier otherwise.       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_timer_stop(int id)
{
  /* Check if the timer entry is active */
  if (_nas_timer_db_is_active(id)) {
    nas_timer_entry_t *entry;
    /* Remove the entry from the timer queue */
    entry = _nas_timer_db_remove_entry(id);
#if defined(ENABLE_ITTI)
    timer_remove(entry->timer_id);
#else
    (void)entry;
#endif
    /* Delete the timer entry */
    _nas_timer_db_delete_entry(id);
    return (NAS_TIMER_INACTIVE_ID);
  }

  return (id);
}

/****************************************************************************
 **                                                                        **
 ** Name:    timer_restart()                                           **
 **                                                                        **
 ** Description: Restart the timer with the specified identifier. The ti-  **
 **      mer is scheduled to expire after the same period of time  **
 **      and will execute the callback function that has been set  **
 **      when it was started.                                      **
 **                                                                        **
 ** Inputs:  id:        The identifier of the timer to be started  **
 **             again                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The timer identifier when successfully     **
 **             re-started; NAS_TIMER_INACTIVE_ID otherwise.   **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_timer_restart(int id)
{
  /* Check if the timer entry is active */
  if (_nas_timer_db_is_active(id)) {
    /* Remove the entry from the timer queue */
    nas_timer_entry_t *te = _nas_timer_db_remove_entry(id);
    /* Initialize its interval timer value */
    te->tv = te->itv;
    /* Insert again the entry into the timer queue */
    _nas_timer_db_insert_entry(id, te);
    return (id);
  }

  return (NAS_TIMER_INACTIVE_ID);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_handler()                                      **
 **                                                                        **
 ** Description: The timer handler is executed whenever the system deli-   **
 **      vers signal SIGALARM. It starts execution of the callback **
 **      function of the first entry within the queue of active    **
 **      timer entries. The entry is not removed from the queue of **
 **      active timer entries and shall be explicitly removed when **
 **      the timer expires.                                        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
#if defined(ENABLE_ITTI)
void nas_timer_handle_signal_expiry(long timer_id, void *arg_p)
{
  /* Get the timer entry for which the system timer expired */
  nas_timer_entry_t *te = _nas_timer_db.head->entry;

  te->cb(te->args);
}
#else
static void _nas_timer_handler(int signal)
{
  /* At least one timer has been started */
  assert( (_nas_timer_db.head != NULL) && (_nas_timer_db.head->entry != NULL) );

  /* Get the timer entry for which the system timer expired */
  nas_timer_entry_t *te = _nas_timer_db.head->entry;

  /* Execute the callback function */
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  int rc = pthread_create (&te->pid, &attr, te->cb, te->args);
  pthread_attr_destroy(&attr);

  /* Wait for the thread to terminate before releasing the timer entry */
  if (rc == 0) {
    void *result = NULL;
    (void) pthread_join(te->pid, &result);

    /* TODO: Check returned result ??? */
    if (result) {
      free(result);
    }
  }
}
#endif

/*
 * -----------------------------------------------------------------------------
 *      Functions used to manage the timer database
 * -----------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_init()                                      **
 **                                                                        **
 ** Description: Initializes the timer database                            **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
static void _nas_timer_db_init(void)
{
  int i;

  for (i = 0; i < TIMER_DATABASE_SIZE; i++) {
    _nas_timer_db.tq[i].id = NAS_TIMER_INACTIVE_ID;
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_get_id()                                    **
 **                                                                        **
 ** Description: Gets the identifier of the first available timer entry in **
 **      the queue of active timer entries                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The identifier of the first available      **
 **             timer entry if the queue of active timers  **
 **             is not full; -1 otherwise.                 **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
static int _nas_timer_db_get_id(void)
{
  int i;

  /* Search from the current timer entry to the last timer entry */
  for (i = _nas_timer_db.timer_id; i < TIMER_DATABASE_SIZE; i++) {
    if (_nas_timer_db.tq[i].id < 0 ) {
      _nas_timer_db.timer_id = i+1;
      return i;
    }
  }

  /* Search from the first timer entry to the current timer entry */
  for (i = 0; i < _nas_timer_db.timer_id; i++) {
    if (_nas_timer_db.tq[i].id < 0 ) {
      _nas_timer_db.timer_id = i+1;
      return i;
    }
  }

  /* No available timer entry found */
  return (-1);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_is_active()                                 **
 **                                                                        **
 ** Description: Checks whether the entry with the given identifier is     **
 **      active within the queue of active timer entries           **
 **                                                                        **
 ** Inputs:  id:        Identifier of the timer entry to check     **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if the timer entry is active; FALSE   **
 **             if it is not an active timer entry.        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_timer_db_is_active(int id)
{
  return ( (id != NAS_TIMER_INACTIVE_ID) &&
           (_nas_timer_db.tq[id].id != NAS_TIMER_INACTIVE_ID) );
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_create_entry()                              **
 **                                                                        **
 ** Description: Creates a new timer entry                                 **
 **                                                                        **
 ** Inputs:  sec:       Time interval value                        **
 **      cb:        Function executed upon timer expiration    **
 **      args:      Callback argument parameters               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    A pointer to the new timer entry if        **
 **             successfully allocated; NULL otherwise     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static nas_timer_entry_t *_nas_timer_db_create_entry(
  long sec, nas_timer_callback_t cb,
  void *args)
{
  nas_timer_entry_t *te = (nas_timer_entry_t *)malloc(sizeof(nas_timer_entry_t));

  if (te != NULL) {
    te->itv.tv_sec = sec;
    te->itv.tv_usec = 0;
    te->tv.tv_sec  = te->itv.tv_sec;
    te->tv.tv_usec = te->itv.tv_usec;
    te->cb = cb;
    te->args = args;
  }

  return (te);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_delete_entry()                              **
 **                                                                        **
 ** Description: Deletes the entry with the given identifier from the ti-  **
 **      mer database.                                             **
 **                                                                        **
 ** Inputs:  id:        Identifier of the entry to be deleted      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
static void _nas_timer_db_delete_entry(int id)
{
  /* The identifier of the timer is valid within the timer queue */
  assert(_nas_timer_db.tq[id].id == id);

  /* Delete the timer entry from the queue */
  _nas_timer_db.tq[id].id = NAS_TIMER_INACTIVE_ID;
  free(_nas_timer_db.tq[id].entry);
  _nas_timer_db.tq[id].entry = NULL;
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_insert_entry()                              **
 **                                                                        **
 ** Description: Inserts the entry with the given identifier into the      **
 **      queue of active timer entries and restarts the system     **
 **      timer if the new entry is the next entry for which the    **
 **      timer should be scheduled to expire.                      **
 **                                                                        **
 ** Inputs:  id:        Identifier of the new entry                **
 **      te:        Pointer to the entry to be inserted        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
static void _nas_timer_db_insert_entry(int id, nas_timer_entry_t *te)
{
  struct itimerval it;
  struct timespec  ts;
  struct timeval   current_time;
  int restart;

  /* Enqueue the new timer entry */
  _nas_timer_db.tq[id].id = id;
  _nas_timer_db.tq[id].entry = te;

  /* Save its interval timer value */
  it.it_interval.tv_sec = it.it_interval.tv_usec = 0;
  it.it_value = te->tv;

  /* Update its interval timer value */
  clock_gettime(CLOCK_MONOTONIC, &ts);
  current_time.tv_sec = ts.tv_sec;
  current_time.tv_usec = ts.tv_nsec/1000;
  /* tv = tv + time() */
  _nas_timer_add(&te->tv, &current_time, &te->tv);

  /* Insert the new timer entry into the list of active entries */
  nas_timer_lock_db();
  restart = _nas_timer_db_insert(&_nas_timer_db.tq[id]);
  nas_timer_unlock_db();

#if !defined(ENABLE_ITTI)

  if (restart) {
    /* The new entry is the first entry of the list;
     * restart the system timer */
    setitimer(ITIMER_REAL, &it, 0);
  }

#else
  (void)(restart);
#endif
}

static int _nas_timer_db_insert(timer_queue_t *entry)
{
  timer_queue_t *prev, *next; /* previous and next entry in the list  */

  /*
   * Search the list of timer entries for the first entry with an interval
   * timer value greater than the interval timer value of the new timer entry
   */
  for (prev = NULL, next = _nas_timer_db.head; next != NULL; next = next->next) {
    if (_nas_timer_cmp(&next->entry->tv, &entry->entry->tv) > 0) {
      break;
    }

    prev = next;
  }

  /* Insert the new entry in the list of active timer entries */
  /* prev <-- entry --> next */
  entry->prev = prev;
  entry->next = next;

  /* Update the pointer from the previous entry */
  if (entry->next != NULL) {
    /* prev <-- entry <--> next */
    entry->next->prev = entry;
  }

  /* Update the pointer from the next entry */
  if (entry->prev != NULL) {
    /* prev <--> entry <--> next */
    entry->prev->next = entry;
  } else {
    /* The new entry is the first entry of the list */
    _nas_timer_db.head = entry;
    return TRUE;
  }

  /* The new entry is NOT the first entry of the list */
  return FALSE;
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_db_remove_entry()                              **
 **                                                                        **
 ** Description: Removes the entry with the given identifier from the      **
 **      queue of active timer entries and restarts the system     **
 **      timer if the entry was the next entry for which the timer **
 **      was scheduled to expire.                                  **
 **                                                                        **
 ** Inputs:  id:        Identifier of the entry to be removed      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    A pointer to the removed entry             **
 **      Others:    _nas_timer_db                              **
 **                                                                        **
 ***************************************************************************/
static nas_timer_entry_t *_nas_timer_db_remove_entry(int id)
{
  int restart;

  /* The identifier of the timer is valid within the timer queue */
  assert(_nas_timer_db.tq[id].id == id);

  /* Remove the timer entry from the list of active entries */
  nas_timer_lock_db();
  restart = _nas_timer_db_remove(&_nas_timer_db.tq[id]);
  nas_timer_unlock_db();

  if (restart) {
    int rc;
    /* The entry was the first entry of the list;
     * the system timer needs to be restarted */
    struct itimerval it;
    struct timeval tv;
    struct timespec ts;

    it.it_interval.tv_sec = it.it_interval.tv_usec = 0;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    tv.tv_sec = ts.tv_sec;
    tv.tv_usec = ts.tv_nsec/1000;
    /* tv = tv - time() */
    rc = _nas_timer_sub(&_nas_timer_db.head->entry->tv, &tv, &it.it_value);

#if defined(ENABLE_ITTI)
    timer_remove(_nas_timer_db.head->entry->timer_id);
    (void) (rc);
#else

    if (rc < 0) {
      /* The system timer should have already expired */
      _nas_timer_handler(SIGALRM);
    } else {
      /* Restart the system timer */
      setitimer(ITIMER_REAL, &it, 0);
    }

#endif
  }

  /* Return a pointer to the removed entry */
  return (_nas_timer_db.tq[id].entry);
}

static int _nas_timer_db_remove(timer_queue_t *entry)
{
  /* Update the pointer from the previous entry */
  /* prev ---> entry ---> next */
  /* prev <--- entry <--- next */
  if (entry->next != NULL) {
    /* prev ---> entry ---> next */
    /* prev <-------------- next */
    entry->next->prev = entry->prev;
  }

  /* Update the pointer from the next entry */
  if (entry->prev != NULL) {
    /* prev --------------> next */
    /* prev <-------------- next */
    entry->prev->next = entry->next;
  } else {
    /* The entry was the first entry of the list */
    _nas_timer_db.head = entry->next;

    if (_nas_timer_db.head != NULL) {
      /* Other timers are scheduled to expire */
      return TRUE;
    }

#if !defined(ENABLE_ITTI)
    {
      /* No more timer is scheduled to expire; stop the system timer */
      struct itimerval it;
      it.it_interval.tv_sec = it.it_interval.tv_usec = 0;
      it.it_value.tv_sec = it.it_value.tv_usec = 0;
      setitimer(ITIMER_REAL, &it, 0);
      return FALSE;
    }
#endif
  }

  /* The entry was NOT the first entry of the list */
  return FALSE;
}

/*
 * -----------------------------------------------------------------------------
 *      Operator functions for timeval structures
 * -----------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:        _nas_timer_cmp()                                          **
 **                                                                        **
 ** Description: Performs timeval comparaison                              **
 **                                                                        **
 ** Inputs:              a:     The first timeval structure                **
 **                      b:     The second timeval structure               **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    -1 if a < b; 1 if a > b; 0 if a == b       **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_timer_cmp(const struct timeval *a, const struct timeval *b)
{
  if (a->tv_sec < b->tv_sec) {
    return -1;
  } else if (a->tv_sec > b->tv_sec) {
    return 1;
  } else if (a->tv_usec < b->tv_usec) {
    return -1;
  } else if (a->tv_usec > b->tv_usec) {
    return 1;
  }

  return 0;
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_add()                                          **
 **                                                                        **
 ** Description: Performs timeval addition                                 **
 **                                                                        **
 ** Inputs:  a:     The first timeval structure                **
 **      b:     The second timeval structure               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     result:    result = timeval(a + b)                    **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void _nas_timer_add(const struct timeval *a, const struct timeval *b,
                           struct timeval *result)
{
  result->tv_sec = a->tv_sec + b->tv_sec;
  result->tv_usec = a->tv_usec + b->tv_usec;

  if (result->tv_usec > 1000000) {
    result->tv_sec++;
    result->tv_usec -= 1000000;
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_timer_sub()                                          **
 **                                                                        **
 ** Description: Performs timeval substraction                             **
 **                                                                        **
 ** Inputs:  a:     The first timeval structure                **
 **      b:     The second timeval structure               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     result:    a >= b, result = timeval(a - b)            **
 **      Return:    -1 if a < b; 0 otherwise                   **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_timer_sub(const struct timeval *a, const struct timeval *b,
                          struct timeval *result)
{
  if (_nas_timer_cmp(a,b) > 0 ) {
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_usec = a->tv_usec - b->tv_usec;

    if (result->tv_usec < 0) {
      result->tv_sec--;
      result->tv_usec += 1000000;
    }

    return 0;
  }

  return -1;
}

