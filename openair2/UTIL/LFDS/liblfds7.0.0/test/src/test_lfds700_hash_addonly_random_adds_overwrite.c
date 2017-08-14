/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  struct lfds700_hash_a_element
    hae;

  lfds700_pal_uint_t
    key;
};

struct test_state
{
  lfds700_pal_uint_t
    number_elements_per_thread,
    overwrite_count;

  struct lfds700_hash_a_state
    *has;

  struct test_element
    *element_array;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_adding( void *util_thread_starter_thread_state );
static int key_compare_function( void const *new_key, void const *existing_key );
static void key_hash_function( void const *key, lfds700_pal_uint_t *hash );
static int qsort_and_bsearch_key_compare_function( void const *e1, void const *e2 );





/****************************************************************************/
void test_lfds700_hash_a_random_adds_overwrite_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  int
    rv;

  lfds700_pal_uint_t
    actual_sum_overwrite_existing_count,
    expected_sum_overwrite_existing_count,
    *key_count_array,
    loop,
    number_elements_per_thread,
    number_elements_total,
    number_logical_processors,
    random_value;

  struct lfds700_hash_a_iterate
    hai;

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

  void
    *key_pointer,
    *key;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single hash_a
           we generate n elements per thread
           each element contains a key value, which is set to a random value
           (we don't use value, so it's just set to 0)
           the threads then run, putting
           the threads count their number of overwrite hits
           once the threads are done, then we
           count the number of each key
           from this we figure out the min/max element for hash_a validation, so we call validation
           we check the sum of overwrites for each thread is what it should be
           then using the hash_a get() we check all the elements we expect are present
           and then we iterate over the hash_a
           checking we see each key once
  */

  internal_display_test_name( "Random adds, get and iterate (overwrite on existing key)" );

  lfds700_misc_prng_init( &ps );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  baus = util_aligned_malloc( sizeof(struct lfds700_btree_au_state) * 1000, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_hash_a_init_valid_on_current_logical_core( &has, baus, 1000, key_compare_function, key_hash_function, LFDS700_HASH_A_EXISTING_KEY_OVERWRITE, NULL );

  // TRD : we divide by 2 beccause we have to allocate a second array of this size later
  number_elements_per_thread = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors ) / 2;
  number_elements_total = number_elements_per_thread * number_logical_processors;

  // TRD : created an ordered list of unique numbers
  element_array = util_aligned_malloc( sizeof(struct test_element) * number_elements_total, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_elements_total ; loop++ )
  {
    random_value = LFDS700_MISC_PRNG_GENERATE( &ps );
    (element_array+loop)->key = (lfds700_pal_uint_t) floor( (number_elements_total/2) * ((double) random_value / (double) LFDS700_MISC_PRNG_MAX) );
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->has = &has;
    (ts+loop)->element_array = element_array + number_elements_per_thread*loop;
    (ts+loop)->overwrite_count = 0;
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
  key_count_array = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_elements_total );
  for( loop = 0 ; loop < number_elements_total ; loop++ )
    *(key_count_array+loop) = 0;

  for( loop = 0 ; loop < number_elements_total ; loop++ )
    ( *(key_count_array + (element_array+loop)->key) )++;

  vi.min_elements = number_elements_total;

  for( loop = 0 ; loop < number_elements_total ; loop++ )
    if( *(key_count_array+loop) == 0 )
      vi.min_elements--;

  vi.max_elements = vi.min_elements;

  lfds700_hash_a_query( &has, LFDS700_HASH_A_QUERY_SINGLETHREADED_VALIDATE, (void *) &vi, (void *) &dvs );

  expected_sum_overwrite_existing_count = 0;

  for( loop = 0 ; loop < number_elements_total ; loop++ )
    if( *(key_count_array+loop) != 0 )
      expected_sum_overwrite_existing_count += *(key_count_array+loop) - 1;

  actual_sum_overwrite_existing_count = 0;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    actual_sum_overwrite_existing_count += (ts+loop)->overwrite_count;

  if( expected_sum_overwrite_existing_count != actual_sum_overwrite_existing_count )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  // TRD : now loop over the expected array and check we can get() every element
  for( loop = 0 ; loop < number_elements_total ; loop++ )
    if( *(key_count_array+loop) > 0 )
    {
      rv = lfds700_hash_a_get_by_key( &has, (void *) loop, &hae );

      if( rv != 1 )
        dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
    }

  /* TRD : now iterate, checking we find every element and no others
           to do this in a timely manner, we need to qsort() the key values
           and use bsearch() to check for items in the array
  */

  for( loop = 0 ; loop < number_elements_total ; loop++ )
    if( *(key_count_array+loop) != 0 )
      *(key_count_array+loop) = loop;
    else
      *(key_count_array+loop) = 0;

  qsort( key_count_array, number_elements_total, sizeof(lfds700_pal_uint_t), qsort_and_bsearch_key_compare_function );

  lfds700_hash_a_iterate_init( &has, &hai );

  while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_hash_a_iterate(&hai, &hae) )
  {
    key = LFDS700_HASH_A_GET_KEY_FROM_ELEMENT( *hae );

    key_pointer = bsearch( &key, key_count_array, number_elements_total, sizeof(lfds700_pal_uint_t), qsort_and_bsearch_key_compare_function );

    if( key_pointer == NULL )
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
  }

  // TRD : cleanup
  lfds700_hash_a_cleanup( &has, NULL );

  util_aligned_free( baus );

  free( ts );

  util_aligned_free( element_array );

  free( key_count_array );

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
    LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( (ts->element_array+index)->hae, 0 );
    apr = lfds700_hash_a_insert( ts->has, &(ts->element_array+index)->hae, NULL, &ps );

    if( apr == LFDS700_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE )
      ts->overwrite_count++;

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





/****************************************************************************/
static int qsort_and_bsearch_key_compare_function( void const *e1, void const *e2 )
{
  int
    cr = 0;

  lfds700_pal_uint_t
    s1,
    s2;

  s1 = *(lfds700_pal_uint_t *) e1;
  s2 = *(lfds700_pal_uint_t *) e2;

  if( s1 > s2 )
    cr = 1;

  if( s1 < s2 )
    cr = -1;

  return( cr );
}

