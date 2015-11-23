#include "internal.h"





/****************************************************************************/
void test_lfds611_stack( void )
{
  printf( "\n"
          "Stack Tests\n"
          "===========\n" );

  stack_test_internal_popping();
  stack_test_internal_pushing();
  stack_test_internal_popping_and_pushing();
  stack_test_internal_rapid_popping_and_pushing();

  return;
}





/****************************************************************************/
void stack_test_internal_popping( void )
{
  unsigned int
  loop,
  *found_count,
  cpu_count;

  lfds611_atom_t
  count;

  thread_state_t
  *thread_handles;

  enum lfds611_data_structure_validity
  dvs = LFDS611_VALIDITY_VALID;

  struct lfds611_stack_state
      *ss;

  struct stack_test_popping_state
      *stps;

  /* TRD : we create a stack with 1,000,000 elements

           we then populate the stack, where each element is
           set to contain a void pointer which is its element number

           we then run one thread per CPU
           where each thread loops, popping as quickly as possible
           each popped element is pushed onto a thread-local stack

           the threads run till the source stack is empty

           we then check the thread-local stacks
           we should find we have every element

           then tidy up
  */

  internal_display_test_name( "Popping" );

  cpu_count = abstraction_cpu_count();

  lfds611_stack_new( &ss, 1000000 );

  for( loop = 0 ; loop < 1000000 ; loop++ )
    lfds611_stack_push( ss, (void *) (lfds611_atom_t) loop );

  stps = malloc( sizeof(struct stack_test_popping_state) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (stps+loop)->ss = ss;
    lfds611_stack_new( &(stps+loop)->ss_thread_local, 1000000 );
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, stack_test_internal_thread_popping, stps+loop );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  // TRD : now we check the thread-local stacks
  found_count = malloc( sizeof(unsigned int) * 1000000 );

  for( loop = 0 ; loop < 1000000 ; loop++ )
    *(found_count+loop) = 0;

  for( loop = 0 ; loop < cpu_count ; loop++ )
    while( lfds611_stack_pop((stps+loop)->ss_thread_local, (void **) &count) )
      (*(found_count+count))++;

  for( loop = 0 ; loop < 1000000 and dvs == LFDS611_VALIDITY_VALID ; loop++ ) {
    if( *(found_count+loop) == 0 )
      dvs = LFDS611_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( *(found_count+loop) > 1 )
      dvs = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
  }

  // TRD : cleanup
  free( found_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    lfds611_stack_delete( (stps+loop)->ss_thread_local, NULL, NULL );

  free( stps );
  lfds611_stack_delete( ss, NULL, NULL );

  // TRD : print the test result
  internal_display_test_result( 1, "stack", dvs );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION stack_test_internal_thread_popping( void *stack_test_popping_state )
{
  struct stack_test_popping_state
      *stps;

  lfds611_atom_t
  count;

  assert( stack_test_popping_state != NULL );

  stps = (struct stack_test_popping_state *) stack_test_popping_state;

  lfds611_stack_use( stps->ss );

  while( lfds611_stack_pop(stps->ss, (void **) &count) )
    lfds611_stack_push( stps->ss_thread_local, (void *) count );

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void stack_test_internal_pushing( void )
{
  unsigned int
  loop,
  cpu_count;

  thread_state_t
  *thread_handles;

  enum lfds611_data_structure_validity
  dvs[2];

  struct stack_test_pushing_state
      *stps;

  struct lfds611_stack_state
      *ss;

  lfds611_atom_t
  user_data,
  thread,
  count,
  *per_thread_counters;

  struct lfds611_validation_info
      vi = { 1000000, 1000000 };

  /* TRD : we create a stack with 1,000,000 elements

           we then create one thread per CPU, where each thread
           pushes as quickly as possible to the stack

           the data pushed is a counter and a thread ID

           the threads exit when the stack is full

           we then validate the stack;

           checking that the counts increment on a per unique ID basis
           and that the number of elements we pop equals 1,000,000
           (since each element has an incrementing counter which is
            unique on a per unique ID basis, we can know we didn't lose
            any elements)
  */

  internal_display_test_name( "Pushing" );

  cpu_count = abstraction_cpu_count();

  stps = malloc( sizeof(struct stack_test_pushing_state) * cpu_count );

  // TRD : the main stack
  lfds611_stack_new( &ss, 1000000 );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (stps+loop)->thread_number = (lfds611_atom_t) loop;
    (stps+loop)->ss = ss;
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, stack_test_internal_thread_pushing, stps+loop );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  // TRD : the stack is now fully pushed; time to verify
  per_thread_counters = malloc( sizeof(lfds611_atom_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    *(per_thread_counters+loop) = 1000000;

  lfds611_stack_query( ss, LFDS611_STACK_QUERY_VALIDATE, &vi, (void *) dvs );

  while( dvs[0] == LFDS611_VALIDITY_VALID and lfds611_stack_pop(ss, (void **) &user_data) ) {
    thread = user_data >> (sizeof(lfds611_atom_t)*8-8);
    count = (user_data << 8) >> 8;

    if( thread >= cpu_count ) {
      dvs[0] = LFDS611_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( count > per_thread_counters[thread] )
      dvs[0] = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( count < per_thread_counters[thread] )
      per_thread_counters[thread] = count-1;
  }

  // TRD : clean up
  free( per_thread_counters );

  free( stps );

  lfds611_stack_delete( ss, NULL, NULL );

  // TRD : print the test result
  internal_display_test_result( 2, "stack", dvs[0], "stack freelist", dvs[1] );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION stack_test_internal_thread_pushing( void *stack_test_pushing_state )
{
  struct stack_test_pushing_state
      *stps;

  lfds611_atom_t
  counter = 0;

  assert( stack_test_pushing_state != NULL );

  stps = (struct stack_test_pushing_state *) stack_test_pushing_state;

  lfds611_stack_use( stps->ss );

  // TRD : we write (thread_number | counter), where thread_number is the top 8 bits of the lfds611_atom_t
  while( lfds611_stack_push(stps->ss, (void **) ((stps->thread_number << (sizeof(lfds611_atom_t)*8-8)) | counter++)) );

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void stack_test_internal_popping_and_pushing( void )
{
  unsigned int
  loop,
  subloop,
  cpu_count;

  thread_state_t
  *thread_handles;

  enum lfds611_data_structure_validity
  dvs[2];

  struct lfds611_stack_state
      *ss;

  struct stack_test_popping_and_pushing_state
      *stpps;

  struct lfds611_validation_info
      vi;

  /* TRD : we have two threads per CPU
           the threads loop for ten seconds
           the first thread pushes 100000 elements then pops 100000 elements
           the second thread pops 100000 elements then pushes 100000 elements
           all pushes and pops go onto the single main stack

           after time is up, all threads push what they have remaining onto
           the main stack

           we then validate the main stack
  */

  internal_display_test_name( "Popping and pushing (10 seconds)" );

  cpu_count = abstraction_cpu_count();

  // TRD : just some initial elements so the pushing threads can start immediately
  lfds611_stack_new( &ss, 100000 * cpu_count * 2 );

  for( loop = 0 ; loop < 100000 * cpu_count ; loop++ )
    lfds611_stack_push( ss, (void *) (lfds611_atom_t) loop );

  stpps = malloc( sizeof(struct stack_test_popping_and_pushing_state) * cpu_count * 2 );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    (stpps+loop)->ss = ss;
    lfds611_stack_new( &(stpps+loop)->local_ss, 100000 );

    (stpps+loop+cpu_count)->ss = ss;
    lfds611_stack_new( &(stpps+loop+cpu_count)->local_ss, 100000 );

    // TRD : fill the pushing thread stacks
    for( subloop = 0 ; subloop < 100000 ; subloop++ )
      lfds611_stack_push( (stpps+loop+cpu_count)->local_ss, (void *) (lfds611_atom_t) subloop );
  }

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count * 2 );

  for( loop = 0 ; loop < cpu_count ; loop++ ) {
    abstraction_thread_start( &thread_handles[loop], loop, stack_test_internal_thread_popping_and_pushing_start_popping, stpps+loop );
    abstraction_thread_start( &thread_handles[loop+cpu_count], loop, stack_test_internal_thread_popping_and_pushing_start_pushing, stpps+loop+cpu_count );
  }

  for( loop = 0 ; loop < cpu_count * 2 ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  for( loop = 0 ; loop < cpu_count * 2 ; loop++ )
    lfds611_stack_delete( (stpps+loop)->local_ss, NULL, NULL );

  free( stpps );

  vi.min_elements = vi.max_elements = 100000 * cpu_count * 2;

  lfds611_stack_query( ss, LFDS611_STACK_QUERY_VALIDATE, (void *) &vi, (void *) dvs );

  lfds611_stack_delete( ss, NULL, NULL );

  // TRD : print the test result
  internal_display_test_result( 2, "stack", dvs[0], "stack freelist", dvs[1] );

  return;
}






/****************************************************************************/
thread_return_t CALLING_CONVENTION stack_test_internal_thread_popping_and_pushing_start_popping( void *stack_test_popping_and_pushing_state )
{
  struct stack_test_popping_and_pushing_state
      *stpps;

  void
  *user_data;

  time_t
  start_time;

  unsigned int
  count;

  assert( stack_test_popping_and_pushing_state != NULL );

  stpps = (struct stack_test_popping_and_pushing_state *) stack_test_popping_and_pushing_state;

  lfds611_stack_use( stpps->ss );
  lfds611_stack_use( stpps->local_ss );

  time( &start_time );

  while( time(NULL) < start_time + 10 ) {
    count = 0;

    while( count < 100000 )
      if( lfds611_stack_pop(stpps->ss, &user_data) ) {
        lfds611_stack_push( stpps->local_ss, user_data );
        count++;
      }

    // TRD : return our local stack to the main stack
    while( lfds611_stack_pop(stpps->local_ss, &user_data) )
      lfds611_stack_push( stpps->ss, user_data );
  }

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION stack_test_internal_thread_popping_and_pushing_start_pushing( void *stack_test_popping_and_pushing_state )
{
  struct stack_test_popping_and_pushing_state
      *stpps;

  void
  *user_data;

  time_t
  start_time;

  unsigned int
  count;

  assert( stack_test_popping_and_pushing_state != NULL );

  stpps = (struct stack_test_popping_and_pushing_state *) stack_test_popping_and_pushing_state;

  lfds611_stack_use( stpps->ss );
  lfds611_stack_use( stpps->local_ss );

  time( &start_time );

  while( time(NULL) < start_time + 10 ) {
    // TRD : return our local stack to the main stack
    while( lfds611_stack_pop(stpps->local_ss, &user_data) )
      lfds611_stack_push( stpps->ss, user_data );

    count = 0;

    while( count < 100000 )
      if( lfds611_stack_pop(stpps->ss, &user_data) ) {
        lfds611_stack_push( stpps->local_ss, user_data );
        count++;
      }
  }

  // TRD : now push whatever we have in our local stack
  while( lfds611_stack_pop(stpps->local_ss, &user_data) )
    lfds611_stack_push( stpps->ss, user_data );

  return( (thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
void stack_test_internal_rapid_popping_and_pushing( void )
{
  unsigned int
  loop,
  cpu_count;

  thread_state_t
  *thread_handles;

  struct lfds611_stack_state
      *ss;

  struct lfds611_validation_info
      vi;

  enum lfds611_data_structure_validity
  dvs[2];

  /* TRD : in these tests there is a fundamental antagonism between
           how much checking/memory clean up that we do and the
           likelyhood of collisions between threads in their lock-free
           operations

           the lock-free operations are very quick; if we do anything
           much at all between operations, we greatly reduce the chance
           of threads colliding

           so we have some tests which do enough checking/clean up that
           they can tell the stack is valid and don't leak memory
           and here, this test now is one of those which does minimal
           checking - in fact, the nature of the test is that you can't
           do any real checking - but goes very quickly

           what we do is create a small stack and then run one thread
           per CPU, where each thread simply pushes and then immediately
           pops

           the test runs for ten seconds

           after the test is done, the only check we do is to traverse
           the stack, checking for loops and ensuring the number of
           elements is correct
  */

  internal_display_test_name( "Rapid popping and pushing (10 seconds)" );

  cpu_count = abstraction_cpu_count();

  lfds611_stack_new( &ss, cpu_count );

  thread_handles = malloc( sizeof(thread_state_t) * cpu_count );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_start( &thread_handles[loop], loop, stack_test_internal_thread_rapid_popping_and_pushing, ss );

  for( loop = 0 ; loop < cpu_count ; loop++ )
    abstraction_thread_wait( thread_handles[loop] );

  free( thread_handles );

  vi.min_elements = 0;
  vi.max_elements = 0;

  lfds611_stack_query( ss, LFDS611_STACK_QUERY_VALIDATE, (void *) &vi, (void *) dvs );

  lfds611_stack_delete( ss, NULL, NULL );

  // TRD : print the test result
  internal_display_test_result( 2, "stack", dvs[0], "stack freelist", dvs[1] );

  return;
}





/****************************************************************************/
thread_return_t CALLING_CONVENTION stack_test_internal_thread_rapid_popping_and_pushing( void *stack_state )
{
  struct lfds611_stack_state
      *ss;

  void
  *user_data = NULL;

  time_t
  start_time;

  assert( stack_state != NULL );

  ss = (struct lfds611_stack_state *) stack_state;

  lfds611_stack_use( ss );

  time( &start_time );

  while( time(NULL) < start_time + 10 ) {
    lfds611_stack_push( ss, user_data );
    lfds611_stack_pop( ss, &user_data );
  }

  return( (thread_return_t) EXIT_SUCCESS );
}

