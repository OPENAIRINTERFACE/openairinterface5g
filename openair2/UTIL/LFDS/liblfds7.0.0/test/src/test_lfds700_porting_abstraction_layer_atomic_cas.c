/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_lfds700_pal_atomic_cas_state
{
  lfds700_pal_uint_t
    local_counter;

  lfds700_pal_atom_t volatile
    *shared_counter;
};

/***** private prototyps *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_cas( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_pal_atomic_cas( struct lfds700_list_asu_state *list_of_logical_processors )
{
  lfds700_pal_atom_t volatile LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    shared_counter;

  lfds700_pal_uint_t
    local_total = 0;

  lfds700_pal_uint_t
    loop,
    number_logical_processors;

  struct lfds700_list_asu_element
    *lasue;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_lfds700_pal_atomic_cas_state
    *atcs;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );

  /* TRD : here we test pal_cas

           we run one thread per CPU
           we use pal_cas() to increment a shared counter
           every time a thread successfully increments the counter,
           it increments a thread local counter
           the threads run for ten seconds
           after the threads finish, we total the local counters
           they should equal the shared counter
  */

  internal_display_test_name( "Atomic CAS" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  shared_counter = 0;

  atcs = util_malloc_wrapper( sizeof(struct test_lfds700_pal_atomic_cas_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (atcs+loop)->shared_counter = &shared_counter;
    (atcs+loop)->local_counter = 0;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_cas, atcs+loop );
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
    local_total += (atcs+loop)->local_counter;

  if( local_total == shared_counter )
    puts( "passed" );

  if( local_total != shared_counter )
  {
    puts( "failed" );
    exit( EXIT_FAILURE );
  }

  // TRD : cleanup
  free( atcs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_cas( void *util_thread_starter_thread_state )
{
  char unsigned 
    result;

  lfds700_pal_uint_t
    loop = 0;

  lfds700_pal_atom_t LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_SINGLE_POINTER)
    exchange,
    compare;

  struct test_lfds700_pal_atomic_cas_state
    *atcs;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_BARRIER_LOAD;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  atcs = (struct test_lfds700_pal_atomic_cas_state *) tsts->thread_user_state;

  util_thread_starter_ready_and_wait( tsts );

  while( loop++ < 10000000 )
  {
    compare = *atcs->shared_counter;

    do
    {
      exchange = compare + 1;
      LFDS700_PAL_ATOMIC_CAS( atcs->shared_counter, &compare, exchange, LFDS700_MISC_CAS_STRENGTH_WEAK, result );
    }
    while( result == 0 );

    atcs->local_counter++;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

