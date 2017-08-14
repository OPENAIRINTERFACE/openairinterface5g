#include "internal.h"





/****************************************************************************/
void test_lfds611_ringbuffer( void )
{
  printf( "\n"
          "Ringbuffer Tests\n"
          "================\n" );

  ringbuffer_test_reading();
  ringbuffer_test_writing();
  ringbuffer_test_reading_and_writing();

  return;
}





/****************************************************************************/
void ringbuffer_test_reading( void )
{
  unsigned int
  loop,
  cpu_count;

  thread_state_t
  *thread_handles;

  struct lfds611_ringbuffer_state
      *rs;

  struct lfds611_freelist_element
      *fe;

  struct ringbuffer_test_reading_state
      *rtrs;

  struct lfds611_validation_info
      vi = { 0, 0 };

  enum lfds611_data_structure_validity
  dvs[3];

  lfds611_atom_t
  total_read = 0;

  /* TRD : we create a single ringbuffer
           with 1,000,000 elements
           we populate the ringbuffer, where the
           user data is an incrementing counter

           we create one thread per CPU
           where each thread busy-works,
           reading until the ringbuffer is empty

           each thread keeps track of the number of reads it manages
           and that each user data it reads is greater than the
           previous user data that was read
  */

  internal_display_test_name( "Reading" );

  cpu_count = abstraction_cpu_count();

  lfds611_ringbuffer_new( &rs, 1000000, NULL, NULL );

  for( loop = 0 ; loop < 1000000 ; loop++ ) {
    lfds611_ringbuffer_get_write_element( rs, &fe, NULL );
    lfds611_freelist_set_user_data_in_element( fe, (void *) (lfds611_atom_t) loop );
    lfds611_ringbuffer_put_write_element( rs, fe );
  }

  rtrs = malloc( sizeof(struct ringbuffer_test_reading_state) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (rtrs+loop)->rs = rs;
    (rtrs+loop)->read_count = 0;
    (rtrs+loop)->error_flag = LOWERED;
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, ringbuffer_test_thread_simple_reader, rtrs+loop );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  lfds611_ringbuffer_query( rs, LFDS611_RINGBUFFER_QUERY_VALIDATE, (void *) &vi, (void *) dvs );

  // TRD : check for raised error flags
  for( loop = 0 ; loop < cpu_count ; loop++ )
    if( (rtrs+loop)->error_flag == RAISED )
      dvs[0] = LFDS611_VALIDITY_INVALID_TEST_DATA;

  // TRD : check thread reads total to 1,000,000
  for( loop = 0 ; loop < cpu_count ; loop++ )
    total_read += (rtrs+loop)->read_count;

  if( total_read < 1000000 )
    dvs[0] = LFDS611_VALIDITY_INVALID_MISSING_ELEMENTS;

  if( total_read > 1000000 )
    dvs[0] = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

  free( rtrs );

  lfds611_ringbuffer_delete( rs, NULL, NULL );

  internal_display_test_result( 3, "queue", dvs[0], "queue freelist", dvs[1], "freelist", dvs[2] );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION ringbuffer_test_thread_simple_reader( void *ringbuffer_test_reading_state )
{
  struct ringbuffer_test_reading_state
      *rtrs;

  struct lfds611_freelist_element
      *fe;

  lfds611_atom_t
  *prev_user_data,
  *user_data;

  assert( ringbuffer_test_reading_state != NULL );

  rtrs = (struct ringbuffer_test_reading_state *) ringbuffer_test_reading_state;

  lfds611_ringbuffer_use( rtrs->rs );

  /* TRD : read an initial element to load a value into prev_user_data
           it may be (under valgrind for example) that by the time we start
           there are no elements remaining to read
  */

  lfds611_ringbuffer_get_read_element( rtrs->rs, &fe );

  if( fe == NULL )
    return( (thread_return_t) EXIT_SUCCESS );

  lfds611_freelist_get_user_data_from_element( fe, (void **) &prev_user_data );
  lfds611_ringbuffer_put_read_element( rtrs->rs, fe );

  rtrs->read_count++;

  while( lfds611_ringbuffer_get_read_element(rtrs->rs, &fe) ) {
    lfds611_freelist_get_user_data_from_element( fe, (void **) &user_data );
    lfds611_ringbuffer_put_read_element( rtrs->rs, fe );

    if( user_data <= prev_user_data )
      rtrs->error_flag = RAISED;

    prev_user_data = user_data;

    rtrs->read_count++;
  }

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void ringbuffer_test_writing( void )
{
  unsigned int
  loop,
  cpu_count;

  thread_state_t
  *thread_handles;

  struct lfds611_ringbuffer_state
      *rs;

  struct lfds611_freelist_element
      *fe;

  struct ringbuffer_test_writing_state
      *rtws;

  struct lfds611_validation_info
      vi = { 100000, 100000 };

  enum lfds611_data_structure_validity
  dvs[3];

  lfds611_atom_t
  thread,
  count,
  user_data,
  *per_thread_counters;

  /* TRD : we create a single ringbuffer
           with 100000 elements
           the ringbuffers starts empty

           we create one thread per CPU
           where each thread busy-works writing
           for ten seconds

           the user data in each written element is a combination
           of the thread number and the counter

           after the threads are complete, we validate by
           checking the user data counters increment on a per thread
           basis
  */

  internal_display_test_name( "Writing (10 seconds)" );

  cpu_count = abstraction_cpu_count();

  lfds611_ringbuffer_new( &rs, 100000, NULL, NULL );

  rtws = malloc( sizeof(struct ringbuffer_test_writing_state) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (rtws+loop)->rs = rs;
    (rtws+loop)->write_count = (lfds611_atom_t) loop << (sizeof(lfds611_atom_t)*8-8);
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, ringbuffer_test_thread_simple_writer, rtws+loop );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  // TRD : now check results
  per_thread_counters = malloc( sizeof(lfds611_atom_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    *(per_thread_counters+loop) = 0;

  lfds611_ringbuffer_query( rs, LFDS611_RINGBUFFER_QUERY_VALIDATE, (void *) &vi, (void *) dvs );

  while( dvs[0] == LFDS611_VALIDITY_VALID and dvs[1] == LFDS611_VALIDITY_VALID and dvs[2] == LFDS611_VALIDITY_VALID and lfds611_ringbuffer_get_read_element(rs, &fe) ) {
    lfds611_freelist_get_user_data_from_element( fe, (void *) &user_data );

    thread = user_data >> (sizeof(lfds611_atom_t)*8-8);
    count = (user_data << 8) >> 8;

    if( thread >= cpu_count ) {
      dvs[0] = LFDS611_VALIDITY_INVALID_TEST_DATA;
      lfds611_ringbuffer_put_read_element( rs, fe );
      break;
    }

    if( per_thread_counters[thread] == 0 )
      per_thread_counters[thread] = count;

    if( count < per_thread_counters[thread] )
      dvs[0] = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( count >= per_thread_counters[thread] )
      per_thread_counters[thread] = count+1;

    lfds611_ringbuffer_put_read_element( rs, fe );
  }

  free( per_thread_counters );

  free( rtws );

  lfds611_ringbuffer_delete( rs, NULL, NULL );

  internal_display_test_result( 3, "queue", dvs[0], "queue freelist", dvs[1], "freelist", dvs[2] );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION ringbuffer_test_thread_simple_writer( void *ringbuffer_test_writing_state )
{
  struct ringbuffer_test_writing_state
      *rtws;

  struct lfds611_freelist_element
      *fe;

  time_t
  start_time;

  assert( ringbuffer_test_writing_state != NULL );

  rtws = (struct ringbuffer_test_writing_state *) ringbuffer_test_writing_state;

  lfds611_ringbuffer_use( rtws->rs );

  time( &start_time );

  while( time(NULL) < start_time + 10 ) {
    lfds611_ringbuffer_get_write_element( rtws->rs, &fe, NULL );
    lfds611_freelist_set_user_data_in_element( fe, (void *) (lfds611_atom_t) (rtws->write_count++) );
    lfds611_ringbuffer_put_write_element( rtws->rs, fe );
  }

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void ringbuffer_test_reading_and_writing( void )
{
  unsigned int
  loop,
  subloop,
  cpu_count;

  thread_state_t
  *thread_handles;

  struct lfds611_ringbuffer_state
      *rs;

  struct ringbuffer_test_reading_and_writing_state
      *rtrws;

  struct lfds611_validation_info
      vi = { 0, 0 };

  enum lfds611_data_structure_validity
  dvs[3];

  /* TRD : we create a single ringbuffer
           with 100000 elements
           the ringbuffers starts empty

           we create one thread per CPU
           where each thread busy-works writing
           and then immediately reading
           for ten seconds

           the user data in each written element is a combination
           of the thread number and the counter

           while a thread runs, it keeps track of the
           counters for the other threads and throws an error
           if it sees the number stay the same or decrease
  */

  internal_display_test_name( "Reading and writing (10 seconds)" );

  cpu_count = abstraction_cpu_count();

  lfds611_ringbuffer_new( &rs, 100000, NULL, NULL );

  rtrws = malloc( sizeof(struct ringbuffer_test_reading_and_writing_state) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (rtrws+loop)->rs = rs;
    (rtrws+loop)->counter = (lfds611_atom_t) loop << (sizeof(lfds611_atom_t)*8-8);
    (rtrws+loop)->cpu_count = cpu_count;
    (rtrws+loop)->error_flag = LOWERED;
    (rtrws+loop)->per_thread_counters = malloc( sizeof(lfds611_atom_t) * cpu_count );

    for( subloop = 0 ; subloop < cpu_count ; subloop++ )
      *((rtrws+loop)->per_thread_counters+subloop) = 0;
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, ringbuffer_test_thread_reader_writer, rtrws+loop );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  lfds611_ringbuffer_query( rs, LFDS611_RINGBUFFER_QUERY_VALIDATE, (void *) &vi, (void *) dvs );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    if( (rtrws+loop)->error_flag == RAISED )
      dvs[0] = LFDS611_VALIDITY_INVALID_TEST_DATA;

  for( loop = 0 ; loop < cpu_count ; loop++ )
    free( (rtrws+loop)->per_thread_counters );

  free( rtrws );

  lfds611_ringbuffer_delete( rs, NULL, NULL );

  internal_display_test_result( 3, "queue", dvs[0], "queue freelist", dvs[1], "freelist", dvs[2] );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION ringbuffer_test_thread_reader_writer( void *ringbuffer_test_reading_and_writing_state )
{
  struct ringbuffer_test_reading_and_writing_state
      *rtrws;

  struct lfds611_freelist_element
      *fe;

  lfds611_atom_t
  user_data,
  thread,
  count;

  time_t
  start_time;

  assert( ringbuffer_test_reading_and_writing_state != NULL );

  rtrws = (struct ringbuffer_test_reading_and_writing_state *) ringbuffer_test_reading_and_writing_state;

  lfds611_ringbuffer_use( rtrws->rs );

  time( &start_time );

  while( time(NULL) < start_time + 10 ) {
    lfds611_ringbuffer_get_write_element( rtrws->rs, &fe, NULL );
    lfds611_freelist_set_user_data_in_element( fe, (void *) (lfds611_atom_t) (rtrws->counter++) );
    lfds611_ringbuffer_put_write_element( rtrws->rs, fe );

    lfds611_ringbuffer_get_read_element( rtrws->rs, &fe );
    lfds611_freelist_get_user_data_from_element( fe, (void *) &user_data );

    thread = user_data >> (sizeof(lfds611_atom_t)*8-8);
    count = (user_data << 8) >> 8;

    if( thread >= rtrws->cpu_count )
      rtrws->error_flag = RAISED;
    else {
      if( count < rtrws->per_thread_counters[thread] )
        rtrws->error_flag = RAISED;

      if( count >= rtrws->per_thread_counters[thread] )
        rtrws->per_thread_counters[thread] = count+1;
    }

    lfds611_ringbuffer_put_read_element( rtrws->rs, fe );
  }

  return( (thread_return_t) EXIT_SUCCESS );
}

