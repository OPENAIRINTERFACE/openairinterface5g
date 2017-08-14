/***** includes *****/
#include "internal.h"





/****************************************************************************/
void util_thread_starter_new( struct util_thread_starter_state **tts, lfds700_pal_uint_t number_threads )
{
  lfds700_pal_uint_t
    loop;

  assert( tts != NULL );
  // TRD : number_threads cam be any value in its range

  *tts = util_malloc_wrapper( sizeof(struct util_thread_starter_state) );

  (*tts)->tsts = util_malloc_wrapper( sizeof(struct util_thread_starter_thread_state) * number_threads );
  (*tts)->thread_start_flag = LOWERED;
  (*tts)->number_thread_states = number_threads;

  for( loop = 0 ; loop < number_threads ; loop++ )
  {
    ((*tts)->tsts+loop)->thread_ready_flag = LOWERED;
    ((*tts)->tsts+loop)->thread_start_flag = &(*tts)->thread_start_flag;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}





/****************************************************************************/
void util_thread_starter_start( struct util_thread_starter_state *tts,
                                     test_pal_thread_state_t *thread_state,
                                     lfds700_pal_uint_t thread_number,
                                     struct test_pal_logical_processor *lp,
                                     test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)( void *thread_user_state ),
                                     void *thread_user_state )
{
  assert( tts != NULL );
  assert( thread_state != NULL );
  assert( lp != NULL );
  assert( thread_function != NULL );
  // TRD : thread_user_state can be NULL

  (tts->tsts+thread_number)->thread_user_state = thread_user_state;

  util_thread_start_wrapper( thread_state, lp, thread_function, tts->tsts+thread_number );

  // TRD : wait for the thread to indicate it is ready and waiting
  while( (tts->tsts+thread_number)->thread_ready_flag == LOWERED );

  return;
}





/****************************************************************************/
void util_thread_starter_ready_and_wait( struct util_thread_starter_thread_state *tsts )
{
  assert( tsts != NULL );

  tsts->thread_ready_flag = RAISED;

  LFDS700_MISC_BARRIER_FULL;

  // TRD : threads here are all looping, so we don't need to force a store

  while( *tsts->thread_start_flag == LOWERED )
    LFDS700_MISC_BARRIER_LOAD;

  return;
}





/****************************************************************************/
void util_thread_starter_run( struct util_thread_starter_state *tts )
{
  assert( tts != NULL );

  /* TRD : all threads at this point are ready to go
           as we wait for their ready flag immediately after their spawn
  */

  tts->thread_start_flag = RAISED;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}





/****************************************************************************/
void util_thread_starter_delete( struct util_thread_starter_state *tts )
{
  assert( tts != NULL );

  free( tts->tsts );

  free( tts );

  return;
}





/****************************************************************************/
void util_thread_start_wrapper( test_pal_thread_state_t *thread_state,
                                     struct test_pal_logical_processor *lp,
                                     test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state),
                                     void *thread_user_state )
{
  int
    rv;

  assert( thread_state != NULL );
  assert( lp != NULL );
  assert( thread_function != NULL );
  // TRD : thread_user_state can be NULL

  rv = test_pal_thread_start( thread_state, lp, thread_function, thread_user_state );

  if( rv == 0 )
  {
    puts( "test_pal_thread_start() failed." );
    exit( EXIT_FAILURE );
  }

  return;
}

