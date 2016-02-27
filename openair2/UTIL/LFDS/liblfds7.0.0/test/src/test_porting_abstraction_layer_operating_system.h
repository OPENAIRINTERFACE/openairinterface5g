/****************************************************************************/
#if( defined _MSC_VER )
  /* TRD : MSVC compiler

           an unfortunately necessary hack for MSVC
           MSVC only defines __STDC__ if /Za is given, where /Za turns off MSVC C extensions - 
           which prevents Windows header files from compiling.
  */

  #define __STDC__         1
  #define __STDC_HOSTED__  1
#endif

#if( defined __linux__ )
  #define _GNU_SOURCE
  #include <unistd.h>
#endif





/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1310 && NTDDI_VERSION >= NTDDI_WINXP && defined _WIN32 )

  #ifdef TEST_PAL_PORTING_ABSTRACTION_LAYER
    #error More than one porting abstraction layer matches current platform.
  #endif

  #define TEST_PAL_PORTING_ABSTRACTION_LAYER

  #define TEST_PAL_OS_STRING "Windows"

  #include <windows.h>

  typedef HANDLE  test_pal_thread_state_t;
  typedef DWORD   test_pal_thread_return_t;

  #define TEST_PAL_CALLING_CONVENTION  WINAPI

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __linux__ && _POSIX_THREADS > 0 )

  #ifdef TEST_PAL_PORTING_ABSTRACTION_LAYER
    #error More than one porting abstraction layer matches current platform.
  #endif

  #define TEST_PAL_PORTING_ABSTRACTION_LAYER

  #define TEST_PAL_OS_STRING "Linux"

  #define _GNU_SOURCE

  #include <pthread.h>
  #include <sched.h>
  #include <sys/syscall.h>
  #include <sys/types.h>

  typedef pthread_t  test_pal_thread_state_t;
  typedef void *     test_pal_thread_return_t;

  #define TEST_PAL_CALLING_CONVENTION  

#endif





/****************************************************************************/
#if( !defined TEST_PAL_PORTING_ABSTRACTION_LAYER )

  #error No matching porting abstraction layer in test_porting_abstraction_layer_operating_system.h

#endif

