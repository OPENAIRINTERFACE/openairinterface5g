#include "internal.h"





/****************************************************************************/
void test_lfds611_slist( void )
{
  printf( "\n"
          "SList Tests\n"
          "===========\n" );

  test_slist_new_delete_get();
  test_slist_get_set_user_data();
  test_slist_delete_all_elements();

  return;
}





/****************************************************************************/
void test_slist_new_delete_get( void )
{
  unsigned int
  loop,
  cpu_count;

  struct lfds611_slist_state
      *ss;

  struct lfds611_slist_element
      *se = NULL;

  struct slist_test_state
      *sts;

  thread_state_t
  *thread_handles;

  size_t
  total_create_count = 0,
  total_delete_count = 0,
  element_count = 0;

  enum lfds611_data_structure_validity
  dvs = LFDS611_VALIDITY_VALID;

  /* TRD : two threads per CPU
           first simply alternates between new_head() and new_next() (next on element created by head)
           second calls get_next, if NULL, then calls get_head, and deletes the element
           both threads keep count of created and deleted
           validate is to reconcile created, deleted and remaining in list
  */

  internal_display_test_name( "New head/next, delete and get next" );

  cpu_count = abstraction_cpu_count();

  lfds611_slist_new( &ss, NULL, NULL );

  sts = malloc( sizeof(struct slist_test_state) * cpu_count * 2 );

  for( loop = 0 ; loop < cpu_count * 2 ; loop++ ) {
    (sts+loop)->ss = ss;
    (sts+loop)->create_count = 0;
    (sts+loop)->delete_count = 0;
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count * 2 );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    abstraction_thread_start( &thread_handles[loop], loop, slist_test_internal_thread_new_delete_get_new_head_and_next, sts+loop );
    abstraction_thread_start( &thread_handles[loop+cpu_count], loop, slist_test_internal_thread_new_delete_get_delete_and_get, sts+loop+cpu_count );
  }

  for( loop = 0 ; loop < cpu_count * 2 ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  // TRD : now validate
  for( loop = 0 ; loop < cpu_count * 2 ; loop++ ) {
    total_create_count += (sts+loop)->create_count;
    total_delete_count += (sts+loop)->delete_count;
  }

  while( NULL != lfds611_slist_get_head_and_then_next(ss, &se) )
    element_count++;

  if( total_create_count - total_delete_count - element_count != 0 )
    dvs = LFDS611_VALIDITY_INVALID_TEST_DATA;

  free( sts );

  lfds611_slist_delete( ss );

  internal_display_test_result( 1, "slist", dvs );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION slist_test_internal_thread_new_delete_get_new_head_and_next( void *slist_test_state )
{
  struct slist_test_state
      *sts;

  time_t
  start_time;

  struct lfds611_slist_element
      *se = NULL;

  assert( slist_test_state != NULL );

  sts = (struct slist_test_state *) slist_test_state;

  lfds611_slist_use( sts->ss );

  time( &start_time );

  while( time(NULL) < start_time + 1 ) {
    if( sts->create_count % 2 == 0 )
      se = lfds611_slist_new_head( sts->ss, NULL );
    else
      lfds611_slist_new_next( se, NULL );

    sts->create_count++;
  }

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION slist_test_internal_thread_new_delete_get_delete_and_get( void *slist_test_state )
{
  struct slist_test_state
      *sts;

  time_t
  start_time;

  struct lfds611_slist_element
      *se = NULL;

  assert( slist_test_state != NULL );

  sts = (struct slist_test_state *) slist_test_state;

  lfds611_slist_use( sts->ss );

  time( &start_time );

  while( time(NULL) < start_time + 1 ) {
    if( se == NULL )
      lfds611_slist_get_head( sts->ss, &se );
    else
      lfds611_slist_get_next( se, &se );

    if( se != NULL ) {
      if( 1 == lfds611_slist_logically_delete_element(sts->ss, se) )
        sts->delete_count++;
    }
  }

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void test_slist_get_set_user_data( void )
{
  unsigned int
  loop,
  cpu_count;

  struct lfds611_slist_state
      *ss;

  struct lfds611_slist_element
      *se = NULL;

  struct slist_test_state
      *sts;

  thread_state_t
  *thread_handles;

  lfds611_atom_t
  thread_and_count,
  thread,
  count,
  *per_thread_counters,
  *per_thread_drop_flags;

  enum lfds611_data_structure_validity
  dvs = LFDS611_VALIDITY_VALID;

  /* TRD : create a list of (cpu_count*10) elements, user data 0
           one thread per CPU
           each thread loops, setting user_data to ((thread_number << (sizeof(lfds611_atom_t)*8-8)) | count)
           validation is to scan list, count on a per thread basis should go down only once
  */

  internal_display_test_name( "Get and set user data" );

  cpu_count = abstraction_cpu_count();

  lfds611_slist_new( &ss, NULL, NULL );

  for( loop = 0 ; loop < cpu_count * 10 ; loop++ )
    lfds611_slist_new_head( ss, NULL );

  sts = malloc( sizeof(struct slist_test_state) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (sts+loop)->ss = ss;
    (sts+loop)->thread_and_count = (lfds611_atom_t) loop << (sizeof(lfds611_atom_t)*8-8);
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, slist_test_internal_thread_get_set_user_data, sts+loop );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  // now validate
  per_thread_counters = malloc( sizeof(lfds611_atom_t) * cpu_count );
  per_thread_drop_flags = malloc( sizeof(lfds611_atom_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    *(per_thread_counters+loop) = 0;
    *(per_thread_drop_flags+loop) = 0;
  }

  while( dvs == LFDS611_VALIDITY_VALID and NULL != lfds611_slist_get_head_and_then_next(ss, &se) ) {
    lfds611_slist_get_user_data_from_element( se, (void **) &thread_and_count );

    thread = thread_and_count >> (sizeof(lfds611_atom_t)*8-8);
    count = (thread_and_count << 8) >> 8;

    if( thread >= cpu_count ) {
      dvs = LFDS611_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( per_thread_counters[thread] == 0 ) {
      per_thread_counters[thread] = count;
      continue;
    }

    per_thread_counters[thread]++;

    if( count < per_thread_counters[thread] and per_thread_drop_flags[thread] == 1 ) {
      dvs = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
      break;
    }

    if( count < per_thread_counters[thread] and per_thread_drop_flags[thread] == 0 ) {
      per_thread_drop_flags[thread] = 1;
      per_thread_counters[thread] = count;
      continue;
    }

    if( count < per_thread_counters[thread] )
      dvs = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( count >= per_thread_counters[thread] )
      per_thread_counters[thread] = count;
  }

  free( per_thread_drop_flags );
  free( per_thread_counters );

  free( sts );

  lfds611_slist_delete( ss );

  internal_display_test_result( 1, "slist", dvs );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION slist_test_internal_thread_get_set_user_data( void *slist_test_state )
{
  struct slist_test_state
      *sts;

  time_t
  start_time;

  struct lfds611_slist_element
      *se = NULL;

  assert( slist_test_state != NULL );

  sts = (struct slist_test_state *) slist_test_state;

  lfds611_slist_use( sts->ss );

  time( &start_time );

  while( time(NULL) < start_time + 1 ) {
    if( se == NULL )
      lfds611_slist_get_head( sts->ss, &se );

    lfds611_slist_set_user_data_in_element( se, (void *) sts->thread_and_count++ );

    lfds611_slist_get_next( se, &se );
  }

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void test_slist_delete_all_elements( void )
{
  struct lfds611_slist_state
      *ss;

  struct lfds611_slist_element
      *se = NULL;

  size_t
  element_count = 0;

  unsigned int
  loop;

  enum lfds611_data_structure_validity
  dvs = LFDS611_VALIDITY_VALID;

  /* TRD : this test creates a list of 100,000 elements
           then simply calls delete_all_elements()
           we then count the number of elements remaining
           should be zero :-)
  */

  internal_display_test_name( "Delete all elements" );

  lfds611_slist_new( &ss, NULL, NULL );

  for( loop = 0 ; loop < 1000000 ; loop++ )
    lfds611_slist_new_head( ss, NULL );

  lfds611_slist_single_threaded_physically_delete_all_elements( ss );

  while( NULL != lfds611_slist_get_head_and_then_next(ss, &se) )
    element_count++;

  if( element_count != 0 )
    dvs = LFDS611_VALIDITY_INVALID_TEST_DATA;

  lfds611_slist_delete( ss );

  internal_display_test_result( 1, "slist", dvs );

  return;
}

