/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  lfds700_pal_uint_t
    number_elements,
    thread_number;

  struct lfds700_stack_state
    *ss;

  struct test_element
    *te_array;
};

struct test_element
{
  struct lfds700_stack_element
    se;

  lfds700_pal_uint_t
    datum,
    thread_number;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_pushing( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_stack_pushing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements,
    number_logical_processors,
    *per_thread_counters;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_stack_element
    *se;

  struct lfds700_stack_state
    ss;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *te,
    *first_te = NULL;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create an empty stack

           we then create one thread per CPU, where each thread
           pushes 100,000 elements each as quickly as possible to the stack
           (the threads themselves alloc these elements, to obtain NUMA closeness)

           the data pushed is a counter and a thread ID

           the threads exit when the stack is full

           we then validate the stack;

           checking that the counts increment on a per unique ID basis
           and that the number of elements we pop equals 100,000 per thread
           (since each element has an incrementing counter which is
            unique on a per unique ID basis, we can know we didn't lose
            any elements)

           there's no CAS+GC code, as we only push
  */

  internal_display_test_name( "Pushing" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  // TRD : the main stack
  lfds700_stack_init_valid_on_current_logical_core( &ss, NULL );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->ss = &ss;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_pushing, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  // TRD : the stack is now fully pushed; time to verify
  per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    *(per_thread_counters+loop) = number_elements - 1;

  vi.min_elements = vi.max_elements = number_elements * number_logical_processors;

  lfds700_stack_query( &ss, LFDS700_STACK_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_stack_pop(&ss, &se, &ps) )
  {
    te = LFDS700_STACK_GET_VALUE_FROM_ELEMENT( *se );

    if( first_te == NULL )
      first_te = te;

    if( te->thread_number >= number_logical_processors )
    {
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( te->datum > per_thread_counters[te->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( te->datum < per_thread_counters[te->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( te->datum == per_thread_counters[te->thread_number] )
      per_thread_counters[te->thread_number]--;
  }

  // TRD : clean up
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    util_aligned_free( (ts+loop)->te_array );

  free( per_thread_counters );

  free( ts );

  lfds700_stack_cleanup( &ss, NULL );

  // TRD : print the test result
  internal_display_test_result( 1, "stack", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_pushing( void *util_thread_starter_thread_state )
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

  // TRD : alloc local 100,000 elements
  ts->te_array = util_aligned_malloc( sizeof(struct test_element) * ts->number_elements, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < ts->number_elements ; loop++ )
  {
    (ts->te_array+loop)->thread_number = ts->thread_number;
    (ts->te_array+loop)->datum = loop;
  }

  util_thread_starter_ready_and_wait( tsts );

  for( loop = 0 ; loop < ts->number_elements ; loop++ )
  {
    LFDS700_STACK_SET_VALUE_IN_ELEMENT( (ts->te_array+loop)->se, ts->te_array+loop );
    lfds700_stack_push( ts->ss, &(ts->te_array+loop)->se, &ps );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

