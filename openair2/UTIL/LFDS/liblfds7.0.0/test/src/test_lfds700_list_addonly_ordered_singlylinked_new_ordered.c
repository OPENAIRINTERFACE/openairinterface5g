/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  struct lfds700_list_aos_element
    laose;

  lfds700_pal_uint_t
    element_number,
    thread_number;
};

struct test_state
{
  lfds700_pal_uint_t
    number_elements_per_thread;

  struct lfds700_list_aos_state
    *laoss;

  struct test_element
    *element_array;
};

/***** private prototypes *****/
static int new_ordered_compare_function( void const *value_new, void const *value_in_list );
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION new_ordered_thread( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_list_aos_new_ordered( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    expected_element_number,
    number_elements_per_thread,
    number_elements_total,
    number_logical_processors,
    offset,
    temp;

  struct lfds700_list_aos_element
    *laose = NULL;

  struct lfds700_list_asu_element
    *lasue = NULL;

  struct lfds700_list_aos_state
    laoss;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct test_element
    *element_array,
    *element;

  struct test_state
    *ts;

  struct util_thread_starter_state
    *tts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : run one thread per logical processor
           we have a single array of 10k elements per thread
           this is set to be randomly ordered (but with contigious numbers from 0 to n)
           we give 10k to each thread (a pointer into the array at the correct point)
           which then loops through that array
           calling lfds700_list_aos_insert_element_by_position( LFDS700_LIST_AOS_POSITION_ORDERED )
           verification should show list is sorted
  */

  internal_display_test_name( "New ordered" );

  lfds700_misc_prng_init( &ps );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_list_aos_init_valid_on_current_logical_core( &laoss, new_ordered_compare_function, LFDS700_LIST_AOS_INSERT_RESULT_FAILURE_EXISTING_KEY, NULL );

  /* TRD : create randomly ordered number array with unique elements

           unique isn't necessary - the list will sort anyway - but
           it permits slightly better validation
  */

  number_elements_per_thread = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors );

  // TRD : or the test takes a looooooong time...
  if( number_elements_per_thread > 10000 )
    number_elements_per_thread = 10000;

  number_elements_total = number_elements_per_thread * number_logical_processors;

  element_array = util_aligned_malloc( sizeof(struct test_element) * number_elements_total, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_elements_total ; loop++ )
    (element_array+loop)->element_number = loop;

  for( loop = 0 ; loop < number_elements_total ; loop++ )
  {
    offset = LFDS700_MISC_PRNG_GENERATE( &ps );
    offset %= number_elements_total;
    temp = (element_array + offset)->element_number;
    (element_array + offset)->element_number = (element_array + loop)->element_number;
    (element_array + loop)->element_number = temp;
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->laoss = &laoss;
    (ts+loop)->element_array = element_array + (loop*number_elements_per_thread);
    (ts+loop)->number_elements_per_thread = number_elements_per_thread;
  }

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, new_ordered_thread, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  free( ts );

  /* TRD : validate the resultant list
           iterate over the list
           we expect to find the list is sorted, 
           which means that element_number will
           increment from zero
  */

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = vi.max_elements = number_elements_total;

  lfds700_list_aos_query( &laoss, LFDS700_LIST_AOS_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  if( dvs == LFDS700_MISC_VALIDITY_VALID )
  {
    expected_element_number = 0;

    // TRD : traverse the list and check combined_data_array matches
    while( dvs == LFDS700_MISC_VALIDITY_VALID and LFDS700_LIST_AOS_GET_START_AND_THEN_NEXT(laoss, laose) )
    {
      element = LFDS700_LIST_AOS_GET_VALUE_FROM_ELEMENT( *laose );

      if( element->element_number != expected_element_number++ )
        dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
    }
  }

  lfds700_list_aos_cleanup( &laoss, NULL );

  util_aligned_free( element_array );

  internal_display_test_result( 1, "list_aos", dvs );

  return;
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static int new_ordered_compare_function( void const *value_new, void const *value_in_list )
{
  int
    cr = 0;

  struct test_element
    *e1,
    *e2;

  // TRD : value_new can be any value in its range
  // TRD : value_in_list can be any value in its range

  e1 = (struct test_element *) value_new;
  e2 = (struct test_element *) value_in_list;

  if( e1->element_number < e2->element_number )
    cr = -1;

  if( e1->element_number > e2->element_number )
    cr = 1;

  return( cr );
}

#pragma warning( default : 4100 )





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION new_ordered_thread( void *util_thread_starter_thread_state )
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

  for( loop = 0 ; loop < ts->number_elements_per_thread ; loop++ )
  {
    LFDS700_LIST_AOS_SET_KEY_IN_ELEMENT( (ts->element_array+loop)->laose, ts->element_array+loop );
    LFDS700_LIST_AOS_SET_VALUE_IN_ELEMENT( (ts->element_array+loop)->laose, ts->element_array+loop );
    lfds700_list_aos_insert( ts->laoss, &(ts->element_array+loop)->laose, NULL, &ps );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

