/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  lfds700_pal_uint_t
    number_of_elements_per_thread;

  struct lfds700_queue_state
    *qs;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueue_dequeuer_with_free( void *util_thread_starter_thread_state );
static void queue_element_cleanup_callback( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag );





/****************************************************************************/
void test_lfds700_queue_enqueuing_and_dequeuing_with_free( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements,
    number_logical_processors,
    number_of_elements_per_thread;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    *qe;

  struct lfds700_queue_state
    qs;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct test_state
    *ts;

  struct util_thread_starter_state
    *tts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : the M&Q queue supports free()ing queue elements after they've been dequeued
           we need to test this
           we spawn one thread per logical core
           there's one master queue which all threads work on
           we create one freelist per thread
           and allocate as many queue elements as we can (no payload)
           - but note each allocate is its own malloc()
           each freelist receives an equal share (i.e. we get the mallocs out of the way)
           each thread enqueues as rapidly as possible
           and dequeues as rapidly as possible
           (i.e. each thread loops, doing an enqueue and a dequeue)
           when the dequeue is done, the element is free()ed
  */

  internal_display_test_name( "Enqueuing and dequeuing with free" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct lfds700_freelist_element) + sizeof(struct lfds700_queue_element) );
  number_of_elements_per_thread = number_elements / number_logical_processors;
  qe = util_aligned_malloc( sizeof(struct lfds700_queue_element), (lfds700_pal_uint_t) LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );
  lfds700_queue_init_valid_on_current_logical_core( &qs, qe, &ps, NULL );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->qs = &qs;
    (ts+loop)->number_of_elements_per_thread = number_of_elements_per_thread;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_enqueue_dequeuer_with_free, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = 0;
  vi.max_elements = 0;

  lfds700_queue_query( &qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  lfds700_queue_cleanup( &qs, queue_element_cleanup_callback );

  free( ts );

  internal_display_test_result( 1, "queue", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueue_dequeuer_with_free( void *util_thread_starter_thread_state )
{
  enum flag
    finished_flag = LOWERED;

  lfds700_pal_uint_t
    loop;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_freelist_element
    *fe,
    *fe_array;

  struct lfds700_freelist_state
    fs;

  struct lfds700_queue_element
    *qe;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  lfds700_freelist_init_valid_on_current_logical_core( &fs, NULL );

  fe_array = util_malloc_wrapper( sizeof(struct lfds700_freelist_element) * ts->number_of_elements_per_thread );

  for( loop = 0 ; loop < ts->number_of_elements_per_thread ; loop++ )
  {
    qe = util_aligned_malloc( sizeof(struct lfds700_queue_element), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );
    LFDS700_FREELIST_SET_VALUE_IN_ELEMENT( fe_array[loop], qe );
    lfds700_freelist_push( &fs, &fe_array[loop], &ps );
  }

  util_thread_starter_ready_and_wait( tsts );

  while( finished_flag == LOWERED )
  {
    loop = 0;
    while( loop++ < 1000 and lfds700_freelist_pop(&fs, &fe, &ps) )
    {
      qe = LFDS700_FREELIST_GET_VALUE_FROM_ELEMENT( *fe );
      lfds700_queue_enqueue( ts->qs, qe, &ps );
    }

    if( loop < 1000 )
      finished_flag = RAISED;

    loop = 0;
    while( loop++ < 1000 and lfds700_queue_dequeue(ts->qs, &qe, &ps) )
      util_aligned_free( qe );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  lfds700_freelist_cleanup( &fs, NULL );

  free( fe_array );

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static void queue_element_cleanup_callback( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag )
{
  assert( qs != NULL );
  assert( qe != NULL );
  // TRD : dummy_element_flag can be any value in its range

  util_aligned_free( qe );

  return;
}

#pragma warning( default : 4100 )

