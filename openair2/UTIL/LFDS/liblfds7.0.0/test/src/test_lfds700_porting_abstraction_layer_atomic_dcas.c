/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_lfds700_pal_atomic_dwcas_state
{
  lfds700_pal_uint_t
    local_counter;

  lfds700_pal_atom_t volatile
    (*shared_counter)[2];
};

/***** private prototyps *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_dwcas( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_pal_atomic_dwcas( struct lfds700_list_asu_state *list_of_logical_processors )
{
  lfds700_pal_uint_t
    local_total = 0,
    loop,
    number_logical_processors;

  lfds700_pal_atom_t volatile LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    shared_counter[2] = { 0, 0 };

  struct lfds700_list_asu_element
    *lasue;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_lfds700_pal_atomic_dwcas_state
    *atds;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );

  /* TRD : here we test pal_dwcas

           we run one thread per CPU
           we use pal_dwcas() to increment a shared counter
           every time a thread successfully increments the counter,
           it increments a thread local counter
           the threads run for ten seconds
           after the threads finish, we total the local counters
           they should equal the shared counter
  */

  internal_display_test_name( "Atomic DWCAS" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  atds = util_malloc_wrapper( sizeof(struct test_lfds700_pal_atomic_dwcas_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (atds+loop)->shared_counter = &shared_counter;
    (atds+loop)->local_counter = 0;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_dwcas, atds+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  // TRD : results
  LFDS700_MISC_BARRIER_LOAD;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    local_total += (atds+loop)->local_counter;

  if( local_total == shared_counter[0] )
    puts( "passed" );

  if( local_total != shared_counter[0] )
  {
    printf( "%llu != %llu\n", (int long long unsigned) local_total, (int long long unsigned) shared_counter[0] );
    puts( "failed" );
    exit( EXIT_FAILURE );
  }

  // TRD : cleanup
  free( atds );

  return;
}

#pragma warning( disable : 4702 )





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_dwcas( void *util_thread_starter_thread_state )
{
  char unsigned
    result;

  lfds700_pal_uint_t
    loop = 0;

  lfds700_pal_atom_t LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_DOUBLE_POINTER)
    exchange[2],
    compare[2];

  struct test_lfds700_pal_atomic_dwcas_state
    *atds;

  struct util_thread_starter_thread_state
    *tsts;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  atds = (struct test_lfds700_pal_atomic_dwcas_state *) tsts->thread_user_state;

  LFDS700_MISC_BARRIER_LOAD;

  util_thread_starter_ready_and_wait( tsts );

  while( loop++ < 10000000 )
  {
    compare[0] = (*atds->shared_counter)[0];
    compare[1] = (*atds->shared_counter)[1];

    do
    {
      exchange[0] = compare[0] + 1;
      exchange[1] = compare[1];
      LFDS700_PAL_ATOMIC_DWCAS( atds->shared_counter, compare, exchange, LFDS700_MISC_CAS_STRENGTH_WEAK, result );
    }
    while( result == 0 );

    atds->local_counter++;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

