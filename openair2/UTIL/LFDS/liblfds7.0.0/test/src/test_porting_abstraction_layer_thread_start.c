/***** includes *****/
#include "internal.h"





/****************************************************************************/
#if( defined _WIN32 && !defined _KERNEL_MODE && NTDDI_VERSION >= NTDDI_WIN7 )

  /* TRD : _WIN32         indicates 32-bit or 64-bit Windows
           !_KERNEL_MODE  indicates Windows user-mode
           NTDDI_VERSION  indicates Windows version
                            - GetCurrentProcess requires XP
                            - InitializeProcThreadAttributeList requires Windows 7
                            - CreateRemoteThreadEx requires Windows 7
  */

  #ifdef TEST_PAL_THREAD_START
    #error More than one porting abstraction layer matches the current platform in test_porting_abstraction_layer_thread_start.c
  #endif

  #define TEST_PAL_THREAD_START

  int test_pal_thread_start( test_pal_thread_state_t *thread_state,
                             struct test_pal_logical_processor *lp, 
                             test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state),
                             void *thread_user_state )
  {
    BOOL
      brv;

    DWORD
      thread_id;

    GROUP_AFFINITY
      ga;

    int
      rv = 0;

    LPPROC_THREAD_ATTRIBUTE_LIST
      attribute_list;

    SIZE_T
      attribute_list_length;

    assert( thread_state != NULL );
    assert( lp != NULL );
    assert( thread_function != NULL );
    // TRD : thread_user_state can be NULL

    /* TRD : here we're using CreateRemoteThreadEx() to start a thread in our own process
             we do this because as a function, it allows us to specify processor and processor group affinity in the create call
    */

    brv = InitializeProcThreadAttributeList( NULL, 1, 0, &attribute_list_length );
    attribute_list = malloc( attribute_list_length );
    brv = InitializeProcThreadAttributeList( attribute_list, 1, 0, &attribute_list_length );

    ga.Mask = ( (KAFFINITY) 1 << lp->logical_processor_number );
    ga.Group = (WORD) lp->windows_logical_processor_group_number;
    memset( ga.Reserved, 0, sizeof(WORD) * 3 );

    brv = UpdateProcThreadAttribute( attribute_list, 0, PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY, &ga, sizeof(GROUP_AFFINITY), NULL, NULL );
    *thread_state = CreateRemoteThreadEx( GetCurrentProcess(), NULL, 0, thread_function, thread_user_state, NO_FLAGS, attribute_list, &thread_id );

    DeleteProcThreadAttributeList( attribute_list );
    free( attribute_list );

    if( *thread_state != NULL )
      rv = 1;

    return( rv );
  }

#endif





/****************************************************************************/
#if( defined _WIN32 && !defined _KERNEL_MODE && NTDDI_VERSION >= NTDDI_WINXP && NTDDI_VERSION < NTDDI_WIN7 )

  /* TRD : _WIN32         indicates 64-bit or 32-bit Windows
           NTDDI_VERSION  indicates Windows version
                            - CreateThread requires XP
                            - SetThreadAffinityMask requires XP
                            - ResumeThread requires XP
  */

  #ifdef TEST_PAL_THREAD_START
    #error More than one porting abstraction layer matches the current platform in test_porting_abstraction_layer_thread_start.c
  #endif

  #define TEST_PAL_THREAD_START

  int test_pal_thread_start( test_pal_thread_state_t *thread_state,
                             struct test_pal_logical_processor *lp,
                             test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state),
                             void *thread_user_state )
  {
    int
      rv = 0;

    DWORD
      thread_id;

    DWORD_PTR
      affinity_mask,
      result;

    assert( thread_state != NULL );
    assert( lp != NULL );
    assert( thread_function != NULL );
    // TRD : thread_user_state can be NULL

    /* TRD : Vista and earlier do not support processor groups
             as such, there is a single implicit processor group
             also, there's no support for actually starting a thread in its correct NUMA node / logical processor
             so we make the best of it; we start suspended, set the affinity, and then resume
             the thread itself internally is expected to be making allocs from the correct NUMA node
    */

    *thread_state = CreateThread( NULL, 0, thread_function, thread_user_state, CREATE_SUSPENDED, &thread_id );

    affinity_mask = (DWORD_PTR) (1 << lp->logical_processor_number);

    SetThreadAffinityMask( *thread_state, affinity_mask );

    ResumeThread( *thread_state );

    if( *thread_state != NULL )
      rv = 1;

    return( rv );
  }

#endif





/****************************************************************************/
#if( defined __linux__ && _POSIX_THREADS > 0 )

  /* TRD : __linux__       indicates Linux
                             - gettid requires Linux
                             - sched_setaffinity requires Linux
           _POSIX_THREADS  indicates POSIX threads
                             - pthread_create requires POSIX
  */

  #ifdef TEST_PAL_THREAD_START
    #error More than one porting abstraction layer matches the current platform in test_porting_abstraction_layer_thread_start.c
  #endif

  #define TEST_PAL_THREAD_START

  /***** structs *****/
  struct test_pal_internal_thread_state
  {
    struct test_pal_logical_processor
      lp;

    test_pal_thread_return_t
      (TEST_PAL_CALLING_CONVENTION *thread_function)( void *thread_user_state );

    void
      *thread_user_state;
  };

  /***** prototypes *****/
  test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION test_pal_internal_thread_function( void *thread_user_state );

  /****************************************************************************/
  int test_pal_thread_start( test_pal_thread_state_t *thread_state,
                             struct test_pal_logical_processor *lp,
                             test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state),
                             void *thread_user_state )
  {
    int
      rv;

    struct test_pal_internal_thread_state
      *its;

    /* TRD : this implementation works on Linux only as it uses sched_setaffinity(), which is Linux specific
             although I cannot currently test, I believe this function also works on Android

             this implementation exists because the pthreads function for setting thread affinity,
             pthread_attr_setaffinity_np(), works on Linux, but not Android
    */

    assert( thread_state != NULL );
    assert( lp != NULL );
    assert( thread_function != NULL );
    // TRD : thread_user_state can be NULL

    its = malloc( sizeof(struct test_pal_internal_thread_state) );

    its->lp = *lp;
    its->thread_function = thread_function;
    its->thread_user_state = thread_user_state;

    rv = pthread_create( thread_state, NULL, test_pal_internal_thread_function, its );

    if( rv == 0 )
      rv = 1;

    return( rv );
  }

  /****************************************************************************/
  test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION test_pal_internal_thread_function( void *thread_user_state )
  {
    cpu_set_t
      cpuset;

    pid_t
      tid;

    struct test_pal_internal_thread_state
      *its;

    test_pal_thread_return_t
      rv;

    assert( thread_user_state != NULL );

    /* TRD : the APIs under Linux/POSIX for setting thread affinity are in a mess
             pthreads offers pthread_attr_setaffinity_np(), which glibc supports,
             but which is not supported by Android
             Linux offers sched_setaffinity(), but this needs a *thread pid*,
             and the only API to get a thread pid is gettid(), which works for
             and only for *the calling thread*

             so we come to this - a wrapper thread function, which is the function used
             when starting a thread; this calls gettid() and then sched_setaffinity(),
             and then calls into the actual thread function

             generally shaking my head in disbelief at this point
    */

    assert( thread_user_state != NULL );

    its = (struct test_pal_internal_thread_state *) thread_user_state;

    CPU_ZERO( &cpuset );
    CPU_SET( its->lp.logical_processor_number, &cpuset );

    tid = syscall( SYS_gettid );

    sched_setaffinity( tid, sizeof(cpu_set_t), &cpuset );

    rv = its->thread_function( its->thread_user_state );

    free( its );

    return( rv );
  }

#endif





/****************************************************************************/
#if( !defined __linux__ && _POSIX_THREADS > 0 )

  /* TRD : !__linux__      indicates not Linux
           _POSIX_THREADS  indicates POSIX threads
                             - pthread_attr_init requires POSIX
                             - pthread_attr_setaffinity_np requires POSIX
                             - pthread_create requires POSIX
                             - pthread_attr_destroy requires POSIX
  */

  #ifdef TEST_PAL_THREAD_START
    #error More than one porting abstraction layer matches the current platform in test_porting_abstraction_layer_thread_start.c
  #endif

  #define TEST_PAL_THREAD_START

  int test_pal_thread_start( test_pal_thread_state_t *thread_state,
                             struct test_pal_logical_processor *lp,
                             test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state),
                             void *thread_user_state )
  {
    int
      rv = 0,
      rv_create;

    cpu_set_t
      cpuset;

    pthread_attr_t
      attr;

    assert( thread_state != NULL );
    assert( lp != NULL );
    assert( thread_function != NULL );
    // TRD : thread_user_state can be NULL

    pthread_attr_init( &attr );

    CPU_ZERO( &cpuset );
    CPU_SET( lp->logical_processor_number, &cpuset );
    pthread_attr_setaffinity_np( &attr, sizeof(cpuset), &cpuset );

    rv_create = pthread_create( thread_state, &attr, thread_function, thread_user_state );

    if( rv_create == 0 )
      rv = 1;

    pthread_attr_destroy( &attr );

    return( rv );
  }

#endif





/****************************************************************************/
#if( !defined TEST_PAL_THREAD_START )

  #error test_pal_thread_start() not implemented for this platform.

#endif

