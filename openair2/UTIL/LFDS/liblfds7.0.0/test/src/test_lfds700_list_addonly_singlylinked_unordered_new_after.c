/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  struct lfds700_list_asu_element
    lasue;

  lfds700_pal_uint_t
    element_number,
    thread_number;
};

struct test_state
{
  lfds700_pal_uint_t
    number_elements;

  struct lfds700_list_asu_state
    *lasus;

  struct test_element
    *element_array;

  struct lfds700_list_asu_element
    *first_element;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION new_after_thread( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_list_asu_new_after( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements,
    number_logical_processors,
    *per_thread_counters,
    subloop;

  struct lfds700_list_asu_element
    *lasue,
    first_element;

  struct lfds700_list_asu_state
    lasus;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *element_array,
    *element;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : run one thread per logical processor
           run for 250k elements
           we put a single first element into the list and
           each thread loops, calling lfds700_list_asu_new_element_by_position( LFDS700_LIST_ASU_POSITION_AFTER ),
           inserting after the single first element
           data element contain s thread_number and element_number
           verification should show element_number decreasing on a per thread basis
  */

  internal_display_test_name( "New after" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  lfds700_list_asu_init_valid_on_current_logical_core( &lasus, NULL, NULL );

  LFDS700_LIST_ASU_SET_KEY_IN_ELEMENT( first_element, NULL );
  LFDS700_LIST_ASU_SET_VALUE_IN_ELEMENT( first_element, NULL );
  lfds700_list_asu_insert_at_position( &lasus, &first_element, NULL, LFDS700_LIST_ASU_POSITION_START, &ps );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors );

  element_array = util_aligned_malloc( sizeof(struct test_element) * number_logical_processors * number_elements, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    for( subloop = 0 ; subloop < number_elements ; subloop++ )
    {
      (element_array+(loop*number_elements)+subloop)->thread_number = loop;
      (element_array+(loop*number_elements)+subloop)->element_number = subloop;
    }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {\
    (ts+loop)->lasus = &lasus;
    (ts+loop)->element_array = element_array + (loop*number_elements);
    (ts+loop)->first_element = &first_element;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, new_after_thread, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  free( ts );

  /* TRD : validate the resultant list
           iterate over each element
           we expect to find element numbers increment on a per thread basis
  */

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = vi.max_elements = number_elements * number_logical_processors + 1;

  lfds700_list_asu_query( &lasus, LFDS700_LIST_ASU_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    *(per_thread_counters+loop) = number_elements - 1;

  /* TRD : we have a leading element, after which all inserts occurred
           we need to get past that element for validation
           this is why we're not using lfds700_list_asu_get_start_and_then_next()
  */

  lasue = LFDS700_LIST_ASU_GET_START( lasus );

  lasue = LFDS700_LIST_ASU_GET_NEXT( *lasue );

  while( dvs == LFDS700_MISC_VALIDITY_VALID and lasue != NULL )
  {
    element = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );

    if( element->thread_number >= number_logical_processors )
    {
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( element->element_number < per_thread_counters[element->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( element->element_number > per_thread_counters[element->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( element->element_number == per_thread_counters[element->thread_number] )
      per_thread_counters[element->thread_number]--;

    lasue = LFDS700_LIST_ASU_GET_NEXT( *lasue );
  }

  free( per_thread_counters );

  lfds700_list_asu_cleanup( &lasus, NULL );

  util_aligned_free( element_array );

  internal_display_test_result( 1, "list_asu", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION new_after_thread( void *util_thread_starter_thread_state )
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

  util_thread_starter_ready_and_wait( tsts );

  for( loop = 0 ; loop < ts->number_elements ; loop++ )
  {
    LFDS700_LIST_ASU_SET_KEY_IN_ELEMENT( (ts->element_array+loop)->lasue, ts->element_array+loop );
    LFDS700_LIST_ASU_SET_VALUE_IN_ELEMENT( (ts->element_array+loop)->lasue, ts->element_array+loop );
    lfds700_list_asu_insert_at_position( ts->lasus, &(ts->element_array+loop)->lasue, ts->first_element, LFDS700_LIST_ASU_POSITION_AFTER, &ps );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

