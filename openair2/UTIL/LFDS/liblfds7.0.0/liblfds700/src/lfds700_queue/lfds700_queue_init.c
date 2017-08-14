/***** includes *****/
#include "lfds700_queue_internal.h"





/****************************************************************************/
void lfds700_queue_init_valid_on_current_logical_core( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe_dummy, struct lfds700_misc_prng_state *ps, void *user_state )
{
  LFDS700_PAL_ASSERT( qs != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &qs->enqueue % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &qs->dequeue % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &qs->user_state % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( qe_dummy != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) qe_dummy->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &qe_dummy->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( ps != NULL );
  // TRD : user_state can be UNLL

  /* TRD : qe_dummy is a dummy element, needed for init
           the qs->enqueue and qs->dequeue counters do not need to be initialized
           but it does no harm to do so, and stops a valgrind complaint
  */

  qs->enqueue[POINTER] = qe_dummy;
  qs->enqueue[COUNTER] = (struct lfds700_queue_element *) 0;
  qs->dequeue[POINTER] = qe_dummy;
  qs->dequeue[COUNTER] = (struct lfds700_queue_element *) 0;

  qe_dummy->next[POINTER] = NULL;
  qe_dummy->next[COUNTER] = (struct lfds700_queue_element *) LFDS700_MISC_PRNG_GENERATE( ps );
  qe_dummy->value = NULL;

  qs->user_state = user_state;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

