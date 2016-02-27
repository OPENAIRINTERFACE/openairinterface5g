/***** includes *****/
#include "internal.h"





/****************************************************************************/
#if( defined _WIN32 && !defined _KERNEL_MODE && NTDDI_VERSION >= NTDDI_WIN7 )

  /* TRD : _WIN32         indicates 64-bit or 32-bit Windows
           !_KERNEL_MODE  indicates Windows user-mode
           NTDDI_VERSION  indicates Windows version
                            - GetLogicalProcessorInformationEx requires Windows 7
  */

  #ifdef TEST_PAL_GET_LOGICAL_CORE_IDS
    #error More than one porting abstraction layer matches current platform in test_porting_abstraction_layer_get_logical_core_ids.c
  #endif

  #define TEST_PAL_GET_LOGICAL_CORE_IDS

  void test_pal_get_logical_core_ids( struct lfds700_list_asu_state *lasus )
  {
    BOOL
      rv;

    DWORD
      loop,
      number_slpie,
      slpie_length = 0;

    lfds700_pal_uint_t
      bitmask,
      logical_processor_number,
      windows_logical_processor_group_number;

    struct lfds700_misc_prng_state
      ps;

    struct test_pal_logical_processor
      *lp;

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX
      *slpie = NULL;

    assert( lasus != NULL );

    lfds700_misc_prng_init( &ps );

    lfds700_list_asu_init_valid_on_current_logical_core( lasus, NULL, NULL );

    rv = GetLogicalProcessorInformationEx( RelationGroup, slpie, &slpie_length );
    slpie = malloc( slpie_length );
    rv = GetLogicalProcessorInformationEx( RelationGroup, slpie, &slpie_length );
    number_slpie = slpie_length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);

    for( loop = 0 ; loop < number_slpie ; loop++ )
      if( (slpie+loop)->Relationship == RelationGroup )
        for( windows_logical_processor_group_number = 0 ; windows_logical_processor_group_number < (slpie+loop)->Group.ActiveGroupCount ; windows_logical_processor_group_number++ )
          for( logical_processor_number = 0 ; logical_processor_number < sizeof(KAFFINITY) * BITS_PER_BYTE ; logical_processor_number++ )
          {
            bitmask = (lfds700_pal_uint_t) 1 << logical_processor_number;

            // TRD : if we've found a processor for this group, add it to the list
            if( (slpie+loop)->Group.GroupInfo[windows_logical_processor_group_number].ActiveProcessorMask & bitmask )
            {
              lp = util_aligned_malloc( sizeof(struct test_pal_logical_processor), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

              lp->logical_processor_number = logical_processor_number;
              lp->windows_logical_processor_group_number = windows_logical_processor_group_number;

              LFDS700_LIST_ASU_SET_VALUE_IN_ELEMENT( lp->lasue, lp );
              lfds700_list_asu_insert_at_start( lasus, &lp->lasue, &ps );
            }
          }

    free( slpie );

    return;
  }

#endif





/****************************************************************************/
#if( defined _WIN32 && !defined _KERNEL_MODE && NTDDI_VERSION >= NTDDI_WINXP && NTDDI_VERSION < NTDDI_WIN7 )

  /* TRD : _WIN32         indicates 64-bit or 32-bit Windows
           !_KERNEL_MODE  indicates Windows user-mode
           NTDDI_VERSION  indicates Windows version
                            - GetLogicalProcessorInformation requires XP SP3
  */

  #ifdef TEST_PAL_GET_LOGICAL_CORE_IDS
    #error More than one porting abstraction layer matches current platform in test_porting_abstraction_layer_get_logical_core_ids.c
  #endif

  #define TEST_PAL_GET_LOGICAL_CORE_IDS

  void test_pal_get_logical_core_ids( struct lfds700_list_asu_state *lasus )
  {
    DWORD
      slpi_length = 0;

    lfds700_pal_uint_t
      number_slpi,
      loop;

    struct lfds700_misc_prng_state
      ps;

    struct test_pal_logical_processor
      *lp;

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION
      *slpi = NULL;

    ULONG_PTR
      mask;

    assert( lasus != NULL );

    lfds700_misc_prng_init( &ps );

    lfds700_list_asu_init_valid_on_current_logical_core( lasus, NULL, NULL );

    *number_logical_processors = 0;

    GetLogicalProcessorInformation( slpi, &slpi_length );
    slpi = malloc( slpi_length );
    GetLogicalProcessorInformation( slpi, &slpi_length );
    number_slpi = slpi_length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

    for( loop = 0 ; loop < number_slpi ; loop++ )
      if( (slpi+loop)->Relationship == RelationProcessorCore )
        for( logical_processor_number = 0 ; logical_processor_number < sizeof(ULONG_PTR) * BITS_PER_BYTE ; logical_processor_number++ )
        {
          bitmask = 1 << logical_processor_number;

          if( (slpi+loop)->ProcessorMask & bitmask )
          {
            lp = util_aligned_malloc( sizeof(struct test_pal_logical_processor), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

            lp->logical_processor_number = logical_processor_number;
            lp->windows_logical_processor_group_number = 0;

            LFDS700_LIST_ASU_SET_VALUE_IN_ELEMENT( lp->lasue, lp );
            lfds700_list_asu_insert_at_start( lasus, &lp->lasue, &ps );
          }

    free( slpi );

    return;
  }

#endif





/****************************************************************************/
#if( defined __linux__ )

  /* TRD : __linux__        indicates Linux
           __STDC__         indicates Standard Library
           __STDC_HOSTED__  indicates Standard Library hosted implementation
                              - fopen requires a Standard Library hosted environment
                              - setbuf requires a Standard Library hosted environment
                              - fgets requires a Standard Library hosted environment
                              - sscanf requires a Standard Library hosted environment
                              - fclose requires a Standard Library hosted environment
  */

  #ifdef TEST_PAL_GET_LOGICAL_CORE_IDS
    #error More than one porting abstraction layer matches current platform in test_porting_abstraction_layer_get_logical_core_ids.c
  #endif

  #define TEST_PAL_GET_LOGICAL_CORE_IDS

  void test_pal_get_logical_core_ids( struct lfds700_list_asu_state *lasus )
  {
    char
      diskbuffer[BUFSIZ],
      string[1024];

    FILE
      *diskfile;

    int long long unsigned
      logical_processor_number;

    struct lfds700_misc_prng_state
      ps;

    struct test_pal_logical_processor
      *lp;

    assert( lasus != NULL );

    lfds700_misc_prng_init( &ps );

    lfds700_list_asu_init_valid_on_current_logical_core( lasus, NULL, NULL );

    diskfile = fopen( "/proc/cpuinfo", "r" );

    if( diskfile != NULL )
    {
      setbuf( diskfile, diskbuffer );

      while( NULL != fgets(string, 1024, diskfile) )
        if( 1 == sscanf(string, "processor : %llu", &logical_processor_number) )
        {
          lp = util_aligned_malloc( sizeof(struct test_pal_logical_processor), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

          lp->logical_processor_number = (lfds700_pal_uint_t) logical_processor_number;
          lp->windows_logical_processor_group_number = 0;

          LFDS700_LIST_ASU_SET_VALUE_IN_ELEMENT( lp->lasue, lp );
          lfds700_list_asu_insert_at_start( lasus, &lp->lasue, &ps );
        }

      fclose( diskfile );
    }

    return;
  }

#endif





/****************************************************************************/
#if( !defined TEST_PAL_GET_LOGICAL_CORE_IDS )

  #error test_pal_get_logical_core_ids() not implemented for this platform.

#endif

