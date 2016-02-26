/***** includes *****/
#include "internal.h"





/****************************************************************************/
int main( int argc, char **argv )
{
  enum flag
    run_flag = LOWERED,
    show_error_flag = LOWERED,
    show_help_flag = LOWERED,
    show_version_flag = LOWERED;

  int
    rv;

  lfds700_pal_uint_t
    loop,
    iterations = 1,
    memory_in_megabytes = DEFAULT_TEST_MEMORY_IN_MEGABYTES;

  struct lfds700_list_asu_state
    list_of_logical_processors;

  struct util_cmdline_state
    cs;

  union util_cmdline_arg_data
    *arg_data;

  assert( argc >= 1 );
  assert( argv != NULL );

  lfds700_misc_library_init_valid_on_current_logical_core();

  util_cmdline_init( &cs );

  util_cmdline_add_arg( &cs, 'h', LIBCOMMON_CMDLINE_ARG_TYPE_FLAG );
  util_cmdline_add_arg( &cs, 'i', LIBCOMMON_CMDLINE_ARG_TYPE_INTEGER );
  util_cmdline_add_arg( &cs, 'm', LIBCOMMON_CMDLINE_ARG_TYPE_INTEGER );
  util_cmdline_add_arg( &cs, 'r', LIBCOMMON_CMDLINE_ARG_TYPE_FLAG );
  util_cmdline_add_arg( &cs, 'v', LIBCOMMON_CMDLINE_ARG_TYPE_FLAG );

  rv = util_cmdline_process_args( &cs, argc, argv );

  if( rv == 0 )
    show_error_flag = RAISED;

  if( rv == 1 )
  {
    util_cmdline_get_arg_data( &cs, 'h', &arg_data );
    if( arg_data != NULL )
      show_help_flag = RAISED;

    util_cmdline_get_arg_data( &cs, 'i', &arg_data );
    if( arg_data != NULL )
      iterations = (lfds700_pal_uint_t) arg_data->integer.integer;

    util_cmdline_get_arg_data( &cs, 'm', &arg_data );
    if( arg_data != NULL )
      memory_in_megabytes = (lfds700_pal_uint_t) arg_data->integer.integer;

    util_cmdline_get_arg_data( &cs, 'r', &arg_data );
    if( arg_data != NULL )
      run_flag = RAISED;

    util_cmdline_get_arg_data( &cs, 'v', &arg_data );
    if( arg_data != NULL )
      show_version_flag = RAISED;
  }

  util_cmdline_cleanup( &cs );

  if( argc == 1 or (run_flag == LOWERED and show_version_flag == LOWERED) )
    show_help_flag = RAISED;

  if( show_error_flag == RAISED )
  {
    printf( "\nInvalid arguments.  Sorry - it's a simple parser, so no clues.\n"
            "-h or run with no args to see the help text.\n" );

    return( EXIT_SUCCESS );
  }

  if( show_help_flag == RAISED )
  {
    printf( "test -h -i [n] -m [n] -r -v\n"
            "  -h     : help\n"
            "  -i [n] : number of iterations     (default : 1)\n"
            "  -m [n] : memory for tests, in mb  (default : %u)\n"
            "  -r     : run (causes test to run; present so no args gives help)\n"
            "  -v     : version\n", DEFAULT_TEST_MEMORY_IN_MEGABYTES );

    return( EXIT_SUCCESS );
  }

  if( show_version_flag == RAISED )
  {
    internal_show_version();
    return( EXIT_SUCCESS );
  }

  if( run_flag == RAISED )
  {
    test_pal_get_logical_core_ids( &list_of_logical_processors );

    for( loop = 0 ; loop < (lfds700_pal_uint_t) iterations ; loop++ )
    {
      printf( "\n"
              "Test Iteration %02llu\n"
              "=================\n", (int long long unsigned) (loop+1) );

      test_lfds700_pal_atomic( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_btree_au( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_freelist( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_hash_a( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_list_aos( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_list_asu( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_queue( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_queue_bss( &list_of_logical_processors );
      test_lfds700_ringbuffer( &list_of_logical_processors, memory_in_megabytes );
      test_lfds700_stack( &list_of_logical_processors, memory_in_megabytes );
    }

    lfds700_list_asu_cleanup( &list_of_logical_processors, internal_logical_core_id_element_cleanup_callback );
  }

  lfds700_misc_library_cleanup();

  return( EXIT_SUCCESS );
}

