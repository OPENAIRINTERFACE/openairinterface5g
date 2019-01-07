/***** includes *****/
#include "internal.h"





/****************************************************************************/
#if( defined _WIN32 && NTDDI_VERSION >= NTDDI_WINXP )

  /* TRD : _WIN32         indicates 32-bit or 64-bit Windows
           NTDDI_VERSION  indicates Windows version
                            - WaitForSingleObject requires XP
  */

  #ifdef TEST_PAL_THREAD_WAIT
    #error More than one porting abstraction layer matches current platform in test_porting_abstraction_layer_thread_wait.c
  #endif

  #define TEST_PAL_THREAD_WAIT

  void test_pal_thread_wait( test_pal_thread_state_t thread_state )
  {
    WaitForSingleObject( thread_state, INFINITE );

    return;
  }

#endif





/****************************************************************************/
#if( _POSIX_THREADS > 0 )

  /* TRD : POSIX threads

           _POSIX_THREADS  indicates POSIX threads
                           - pthread_join requires POSIX
  */

  #ifdef TEST_PAL_THREAD_WAIT
    #error More than one porting abstraction layer matches current platform in test_porting_abstraction_layer_thread_wait.c
  #endif

  #define TEST_PAL_THREAD_WAIT

  void test_pal_thread_wait( test_pal_thread_state_t thread_state )
  {
    pthread_join( thread_state, NULL );

    return;
  }

#endif





/****************************************************************************/
#if( !defined TEST_PAL_THREAD_WAIT )

  #error test_pal_thread_wait() not implemented for this platform.

#endif

