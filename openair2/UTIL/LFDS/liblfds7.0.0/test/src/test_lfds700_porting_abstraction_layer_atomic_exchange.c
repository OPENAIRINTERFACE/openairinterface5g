/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  lfds700_pal_uint_t
    counter,
    *counter_array,
    number_elements,
    number_logical_processors;

  lfds700_pal_uint_t volatile
    *shared_exchange;
};

/***** private prototyps *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_exchange( void *util_thread_starter_thread_state );
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_atomic_exchange( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_pal_atomic_exchange( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum flag
    atomic_exchange_success_flag = RAISED,
    exchange_success_flag = RAISED;

  lfds700_pal_uint_t
    loop,
    *merged_counter_arrays,
    number_elements,
    number_logical_processors,
    subloop;

  lfds700_pal_uint_t volatile LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    exchange;

  struct lfds700_list_asu_element
    *lasue;

  struct test_state
    *ts;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : here we test pal_atomic_exchange

           we have one thread per logical core
           there is one variable which every thread will exchange to/from 
           we know the number of logical cores
           the threads have a counter each, which begins with their logical core number plus one
           (plus one because the exchange counter begins with 0 already in place)
           (e.g. thread 0 begins with its counter at 1, thread 1 begins with its counter at 2, etc)

           there is an array per thread of 1 million elements, each a counter, set to 0

           when running, each thread increments its counter by the number of threads
           the threads busy loop, exchanging
           every time aa thread pulls a number off the central, shared exchange variable,
           it increments the counter for that variable in its thread-local counter array

           (we're not using a global array, because we'd have to be atomic in our increments,
            which is a slow-down we don't want)

           at the end, we merge all the counter arrays and if the frequency for a counter is a value
           other than 1, the exchange was not atomic

           we perform the test twice, once with pal_atomic_exchange, once with a non-atomic exchange

           we expect the atomic to pass and the non-atomic to fail
  */

  internal_display_test_name( "Atomic exchange" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(lfds700_pal_uint_t) * (number_logical_processors + 1) );

  merged_counter_arrays = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_elements );

  for( loop = 0 ; loop < number_elements ; loop++ )
    *(merged_counter_arrays+loop) = 0;

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->counter = loop + 1;
    (ts+loop)->counter_array = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_elements );
    for( subloop = 0 ; subloop < number_elements ; subloop++ )
      *((ts+loop)->counter_array+subloop) = 0;
    (ts+loop)->number_logical_processors = number_logical_processors;
    (ts+loop)->shared_exchange = &exchange;
    (ts+loop)->number_elements = number_elements;
  }

  exchange = 0;

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  // TRD : non-atomic
  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_exchange, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  LFDS700_MISC_BARRIER_LOAD;

  for( loop = 0 ; loop < number_elements ; loop++ )
    for( subloop = 0 ; subloop < number_logical_processors ; subloop++ )
      *(merged_counter_arrays+loop) += *( (ts+subloop)->counter_array+loop );

  /* TRD : the worker threads exit when their per-thread counter exceeds 1,000,000
           as such the final number_logical_processors numbers are not read
           we could change the threads to exit when the number they read exceeds 1,000,000
           but then we'd need an if() in their work-loop,
           and we need to go as fast as possible
  */

  for( loop = 0 ; loop < number_elements - number_logical_processors ; loop++ )
    if( *(merged_counter_arrays+loop) != 1 )
      exchange_success_flag = LOWERED;

  // TRD : now for atomic exchange - we need to re-init the data structures

  for( loop = 0 ; loop < number_elements ; loop++ )
    *(merged_counter_arrays+loop) = 0;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    for( subloop = 0 ; subloop < number_elements ; subloop++ )
      *((ts+loop)->counter_array+subloop) = 0;

  exchange = 0;

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_atomic_exchange, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  for( loop = 0 ; loop < number_elements ; loop++ )
    for( subloop = 0 ; subloop < number_logical_processors ; subloop++ )
      *(merged_counter_arrays+loop) += *( (ts+subloop)->counter_array+loop );

  for( loop = 0 ; loop < number_elements - number_logical_processors ; loop++ )
    if( *(merged_counter_arrays+loop) != 1 )
      atomic_exchange_success_flag = LOWERED;

  // TRD : cleanup
  free( merged_counter_arrays );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    free( (ts+loop)->counter_array );

  util_thread_starter_delete( tts );
  free( thread_handles );
  free( ts );

  /* TRD : results

           on a single core, atomic and non-atomic exchange should both work

           if we find our non-atomic test passes, then we can't really say anything
           about whether or not the atomic test is really working
  */

  LFDS700_MISC_BARRIER_LOAD;

  if( number_logical_processors == 1 )
  {
    if( exchange_success_flag == RAISED and atomic_exchange_success_flag == RAISED )
      puts( "passed" );

    if( exchange_success_flag != RAISED or atomic_exchange_success_flag != RAISED )
      puts( "failed (atomic and non-atomic both failed)" );
  }

  if( number_logical_processors >= 2 )
  {
    if( atomic_exchange_success_flag == RAISED and exchange_success_flag == LOWERED )
      puts( "passed" );

    if( atomic_exchange_success_flag == RAISED and exchange_success_flag == RAISED )
      puts( "indeterminate (atomic and non-atomic both passed)" );

    if( atomic_exchange_success_flag == LOWERED )
    {
      puts( "failed (atomic failed)" );
      exit( EXIT_FAILURE );
    }
  }

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_exchange( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    local_counter,
    exchange;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_BARRIER_LOAD;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  util_thread_starter_ready_and_wait( tsts );

  local_counter = ts->counter;

  while( local_counter < ts->number_elements )
  {
    exchange = *ts->shared_exchange;
    *ts->shared_exchange = local_counter;

    ( *(ts->counter_array + exchange) )++;

    local_counter += ts->number_logical_processors;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_atomic_exchange( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    local_counter,
    exchange;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_BARRIER_LOAD;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  util_thread_starter_ready_and_wait( tsts );

  local_counter = ts->counter;

  while( local_counter < ts->number_elements )
  {
    exchange = local_counter;

    LFDS700_PAL_ATOMIC_EXCHANGE( ts->shared_exchange, &exchange );

    ( *(ts->counter_array + exchange) )++;

    local_counter += ts->number_logical_processors;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

