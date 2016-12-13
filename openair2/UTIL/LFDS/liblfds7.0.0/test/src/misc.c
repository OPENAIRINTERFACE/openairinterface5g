/***** includes *****/
#include "internal.h"





/****************************************************************************/
void internal_display_test_name( char *format_string, ... )
{
  va_list
    va;

  assert( format_string != NULL );

  va_start( va, format_string );

  vprintf( format_string, va );

  printf( "..." );

  va_end( va );

  fflush( stdout );

  return;
}





/****************************************************************************/
void internal_display_test_result( lfds700_pal_uint_t number_name_dvs_pairs, ... )
{
  char
    *name;

  enum flag
    passed_flag = RAISED;

  enum lfds700_misc_validity
    dvs;

  lfds700_pal_uint_t
    loop;

  va_list
    va;

  // TRD : number_name_dvs_pairs can be any value in its range

  va_start( va, number_name_dvs_pairs );

  for( loop = 0 ; loop < number_name_dvs_pairs ; loop++ )
  {
    name = va_arg( va, char * );
    dvs = va_arg( va, enum lfds700_misc_validity );

    if( dvs != LFDS700_MISC_VALIDITY_VALID )
    {
      passed_flag = LOWERED;
      break;
    }
  }

  va_end( va );

  if( passed_flag == RAISED )
    puts( "passed" );

  if( passed_flag == LOWERED )
  {
    printf( "failed (" );

    va_start( va, number_name_dvs_pairs );

    for( loop = 0 ; loop < number_name_dvs_pairs ; loop++ )
    {
      name = va_arg( va, char * );
      dvs = va_arg( va, enum lfds700_misc_validity );

      printf( "%s ", name );
      internal_display_data_structure_validity( dvs );

      if( loop+1 < number_name_dvs_pairs )
        printf( ", " );
    }

    va_end( va );

    printf( ")\n" );

    /* TRD : quick hack
             the whole test programme needs rewriting
             and for now I just want to make it so we
             exit with failure upon any test failing
    */

    exit( EXIT_FAILURE );
  }

  return;
}





/****************************************************************************/
void internal_display_data_structure_validity( enum lfds700_misc_validity dvs )
{
  char
    *string = NULL;

  switch( dvs )
  {
    case LFDS700_MISC_VALIDITY_VALID:
      string = "valid";
    break;

    case LFDS700_MISC_VALIDITY_INVALID_LOOP:
      string = "invalid - loop detected";
    break;

    case LFDS700_MISC_VALIDITY_INVALID_ORDER:
      string = "invalid - invalid order detected";
    break;

    case LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS:
      string = "invalid - missing elements";
    break;

    case LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS:
      string = "invalid - additional elements";
    break;

    case LFDS700_MISC_VALIDITY_INVALID_TEST_DATA:
      string = "invalid - invalid test data";
    break;
  }

  printf( "%s", string );

  return;
}





/****************************************************************************/
void internal_show_version()
{
  char const
    *version_and_build_string;

  printf( "test %s (%s, %s) (" __DATE__ " " __TIME__ ")\n", LFDS700_TEST_VERSION_STRING, BUILD_TYPE_STRING, MODE_TYPE_STRING );

  lfds700_misc_query( LFDS700_MISC_QUERY_GET_BUILD_AND_VERSION_STRING, NULL, (void **) &version_and_build_string );

  printf( "%s\n", version_and_build_string );

  return;
}





/****************************************************************************/
#pragma warning( disable : 4100 )

void internal_logical_core_id_element_cleanup_callback( struct lfds700_list_asu_state *lasus, struct lfds700_list_asu_element *lasue )
{
  struct test_pal_logical_processor
    *lp;

  assert( lasus != NULL );
  assert( lasue != NULL );

  lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );

  util_aligned_free( lp );

  return;
}

#pragma warning( default : 4100 )


