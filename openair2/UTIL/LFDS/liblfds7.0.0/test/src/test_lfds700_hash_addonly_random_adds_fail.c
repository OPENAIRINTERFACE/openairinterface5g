/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  struct lfds700_hash_a_element
    hae;

  lfds700_pal_uint_t
    datum,
    key;
};

struct test_state
{
  enum flag
    error_flag;

  lfds700_pal_uint_t
    number_elements_per_thread;

  struct lfds700_hash_a_state
    *has;

  struct test_element
    *element_array;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_adding( void *util_thread_starter_thread_state );
static int key_compare_function( void const *new_key, void const *existing_key );
static void key_hash_function( void const *key, lfds700_pal_uint_t *hash );





/****************************************************************************/
void test_lfds700_hash_a_random_adds_fail_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements_per_thread,
    number_elements_total,
    number_logical_processors,
    offset,
    temp,
    value;

  struct lfds700_hash_a_element
    *hae;

  struct lfds700_hash_a_state
    has;

  struct lfds700_list_asu_element
    *lasue = NULL;

  struct lfds700_btree_au_state
    *baus;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *element_array;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single hash_a
           we generate 100k elements per thread (with one thread per logical processor) in an array
           each element is unique
           we randomly sort the elements
           then each thread loops, adds those elements into the hash_a
           we check that each datum inserts okay - failure will occur on non-unique data, i.e. two identical keys
           we should have no failures
           we then call the hash_a validation function
           then using the hash_a get() we check all the elements we added are present
  */

  internal_display_test_name( "Random adds and get (fail on existing key)" );

  lfds700_misc_prng_init( &ps );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  baus = util_aligned_malloc( sizeof(struct lfds700_btree_au_state) * 1000, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_hash_a_init_valid_on_current_logical_core( &has, baus, 1000, key_compare_function, key_hash_function, LFDS700_HASH_A_EXISTING_KEY_FAIL, NULL );

  number_elements_per_thread = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors );
  number_elements_total = number_elements_per_thread * number_logical_processors;

  // TRD : created an ordered list of unique numbers
  element_array = util_aligned_malloc( sizeof(struct test_element) * number_elements_total, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_elements_total ; loop++ )
  {
    (element_array+loop)->key = loop;
    // TRD : + number_elements just to make it different to the key
    (element_array+loop)->datum = loop + number_elements_total;
  }

  for( loop = 0 ; loop < number_elements_total ; loop++ )
  {
    offset = LFDS700_MISC_PRNG_GENERATE( &ps );
    offset %= number_elements_total;
    temp = (element_array + offset)->key;
    (element_array + offset)->key = (element_array + loop)->key;
    (element_array + loop)->key = temp;
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->has = &has;
    (ts+loop)->element_array = element_array + number_elements_per_thread*loop;
    (ts+loop)->error_flag = LOWERED;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_adding, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  // TRD : now for validation
  vi.min_elements = vi.max_elements = number_elements_total;
  lfds700_hash_a_query( &has, LFDS700_HASH_A_QUERY_SINGLETHREADED_VALIDATE, (void *) &vi, (void *) &dvs );

  /* TRD : now we attempt to lfds700_hash_a_get_by_key() for every element in number_array
           any failure to find is an error
           we also check we've obtained the correct element
  */

  for( loop = 0 ; dvs == LFDS700_MISC_VALIDITY_VALID and loop < number_elements_total ; loop++ )
    if( 0 == lfds700_hash_a_get_by_key(&has, (void *) (ts->element_array+loop)->key, &hae) )
      dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;
    else
    {
      value = (lfds700_pal_uint_t) LFDS700_HASH_A_GET_VALUE_FROM_ELEMENT( *hae );
      if( (ts->element_array+loop)->datum != value )
        dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
    }

  // TRD : just check error_flags weren't raised
  if( dvs == LFDS700_MISC_VALIDITY_VALID )
    for( loop = 0 ; loop < number_logical_processors ; loop++ )
      if( (ts+loop)->error_flag == RAISED )
        dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  // TRD : cleanup
  lfds700_hash_a_cleanup( &has, NULL );

  util_aligned_free( baus );

  free( ts );

  util_aligned_free( element_array );

  // TRD : print the test result
  internal_display_test_result( 1, "hash_a", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_adding( void *util_thread_starter_thread_state )
{
  enum lfds700_hash_a_insert_result
    apr;

  lfds700_pal_uint_t
    index = 0;

  struct test_state
    *ts;

  struct lfds700_misc_prng_state
    ps;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  util_thread_starter_ready_and_wait( tsts );

  while( index < ts->number_elements_per_thread )
  {
    LFDS700_HASH_A_SET_KEY_IN_ELEMENT( (ts->element_array+index)->hae, (ts->element_array+index)->key );
    LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( (ts->element_array+index)->hae, (ts->element_array+index)->datum );
    apr = lfds700_hash_a_insert( ts->has, &(ts->element_array+index)->hae, NULL, &ps );

    if( apr == LFDS700_HASH_A_PUT_RESULT_FAILURE_EXISTING_KEY )
      ts->error_flag = RAISED;

    index++;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static int key_compare_function( void const *new_key, void const *existing_key )
{
  int
    cr = 0;

  // TRD : new_key can be NULL (i.e. 0)
  // TRD : existing_key can be NULL (i.e. 0)

  if( (lfds700_pal_uint_t) new_key < (lfds700_pal_uint_t) existing_key )
    cr = -1;

  if( (lfds700_pal_uint_t) new_key > (lfds700_pal_uint_t) existing_key )
    cr = 1;

  return( cr );
}

#pragma warning( default : 4100 )





/****************************************************************************/
#pragma warning( disable : 4100 )

static void key_hash_function( void const *key, lfds700_pal_uint_t *hash )
{
  // TRD : key can be NULL
  assert( hash != NULL );

  *hash = 0;

  /* TRD : this function iterates over the user data
           and we are using the void pointer *as* key data
           so here we need to pass in the addy of key
  */

  LFDS700_HASH_A_32BIT_HASH_FUNCTION( (void *) &key, sizeof(lfds700_pal_uint_t), *hash );

  return;
}

#pragma warning( default : 4100 )

