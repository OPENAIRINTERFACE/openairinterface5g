/***** includes *****/
#include "lfds700_misc_internal.h"





/****************************************************************************/
void lfds700_misc_query( enum lfds700_misc_query query_type, void *query_input, void *query_output )
{
  // TRD : query type can be any value in its range
  // TRD : query_input can be NULL in some cases
  // TRD : query_outputput can be NULL in some cases

  switch( query_type )
  {
    case LFDS700_MISC_QUERY_GET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_CAS:
      *(lfds700_pal_atom_t *) query_output = lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_cas;
    break;

    case LFDS700_MISC_QUERY_SET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_CAS:
      LFDS700_PAL_ATOMIC_EXCHANGE( &lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_cas, (lfds700_pal_atom_t *) query_input );
    break;

    case LFDS700_MISC_QUERY_GET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_DWCAS:
      *(lfds700_pal_atom_t *) query_output = lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas;
    break;

    case LFDS700_MISC_QUERY_SET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_DWCAS:
      LFDS700_PAL_ATOMIC_EXCHANGE( &lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas, (lfds700_pal_atom_t *) query_input );
    break;

    case LFDS700_MISC_QUERY_GET_BUILD_AND_VERSION_STRING:
    {
      char static const
        * const build_and_version_string = "liblfds " LFDS700_MISC_VERSION_STRING " (" BUILD_TYPE_STRING ", " LFDS700_PAL_OS_STRING ", " MODE_TYPE_STRING ", " LFDS700_PAL_PROCESSOR_STRING ", " LFDS700_PAL_COMPILER_STRING ")";

      LFDS700_PAL_ASSERT( query_input == NULL );
      LFDS700_PAL_ASSERT( query_output != NULL );

      *(char const **) query_output = build_and_version_string;
    }
    break;
  }

  return;
}

