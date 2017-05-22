/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  lfds700_pal_uint_t
    number_elements,
    thread_number;

  struct lfds700_queue_state
    *qs;

  struct test_element
    *te_array;
};

struct test_element
{
  struct lfds700_queue_element
    qe;

  lfds700_pal_uint_t
    counter,
    thread_number;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_enqueuer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_queue_enqueuing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    *per_thread_counters,
    loop,
    number_elements,
    number_logical_processors;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    dummy_qe,
    *qe;

  struct lfds700_queue_state
    qs;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *te;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : create an empty queue
           then run one thread per CPU
           where each thread busy-works, enqueuing elements from a freelist (one local freelist per thread)
           until 100000 elements are enqueued, per thread
           each element's void pointer of user data is a struct containing thread number and element number
           where element_number is a thread-local counter starting at 0

           when we're done, we check that all the elements are present
           and increment on a per-thread basis
  */

  internal_display_test_name( "Enqueuing" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors );

  lfds700_queue_init_valid_on_current_logical_core( &qs, &dummy_qe, &ps, NULL );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->qs = &qs;
    (ts+loop)->thread_number = loop;
    (ts+loop)->number_elements = number_elements;
  }

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_simple_enqueuer, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  /* TRD : first, validate the queue

           then dequeue
           we expect to find element numbers increment on a per thread basis
  */

  vi.min_elements = vi.max_elements = number_elements * number_logical_processors;

  lfds700_queue_query( &qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    *(per_thread_counters+loop) = 0;

  while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_queue_dequeue(&qs, &qe, &ps) )
  {
    te = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );

    if( te->thread_number >= number_logical_processors )
    {
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( te->counter > per_thread_counters[te->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( te->counter < per_thread_counters[te->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( te->counter == per_thread_counters[te->thread_number] )
      per_thread_counters[te->thread_number]++;
  }

  free( per_thread_counters );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    util_aligned_free( (ts+loop)->te_array );

  free( ts );

  lfds700_queue_cleanup( &qs, NULL );

  internal_display_test_result( 1, "queue", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_enqueuer( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    loop;

  struct lfds700_misc_prng_state
    ps;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  ts->te_array = util_aligned_malloc( sizeof(struct test_element) * ts->number_elements, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < ts->number_elements ; loop++ )
  {
    (ts->te_array+loop)->thread_number = ts->thread_number;
    (ts->te_array+loop)->counter = loop;
  }

  util_thread_starter_ready_and_wait( tsts );

  for( loop = 0 ; loop < ts->number_elements ; loop++ )
  {
    LFDS700_QUEUE_SET_VALUE_IN_ELEMENT( (ts->te_array+loop)->qe, ts->te_array+loop );
    lfds700_queue_enqueue( ts->qs, &(ts->te_array+loop)->qe, &ps );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

