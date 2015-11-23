#include "lfds611_liblfds_internal.h"





/****************************************************************************/
void lfds611_liblfds_abstraction_test_helper_increment_non_atomic( lfds611_atom_t *shared_counter )
{
  /* TRD : lfds611_atom_t must be volatile or the compiler
           optimizes it away into a single store
  */

  volatile lfds611_atom_t
  count = 0;

  assert( shared_counter != NULL );

  while( count++ < 10000000 )
    (*(lfds611_atom_t *) shared_counter)++;

  return;
}





/****************************************************************************/
void lfds611_liblfds_abstraction_test_helper_increment_atomic( volatile lfds611_atom_t *shared_counter )
{
  lfds611_atom_t
  count = 0;

  assert( shared_counter != NULL );

  while( count++ < 10000000 )
    lfds611_abstraction_increment( shared_counter );

  return;
}





/****************************************************************************/
void lfds611_liblfds_abstraction_test_helper_cas( volatile lfds611_atom_t *shared_counter, lfds611_atom_t *local_counter )
{
  lfds611_atom_t
  loop = 0,
  original_destination;

  LFDS611_ALIGN(LFDS611_ALIGN_SINGLE_POINTER) lfds611_atom_t
  exchange,
  compare;

  assert( shared_counter != NULL );
  assert( local_counter != NULL );

  while( loop++ < 1000000 ) {
    do {
      compare = *shared_counter;
      exchange = compare + 1;

      original_destination = lfds611_abstraction_cas( shared_counter, exchange, compare );
    } while( original_destination != compare );

    (*local_counter)++;
  }

  return;
}





/****************************************************************************/
void lfds611_liblfds_abstraction_test_helper_dcas( volatile lfds611_atom_t *shared_counter, lfds611_atom_t *local_counter )
{
  lfds611_atom_t
  loop = 0;

  LFDS611_ALIGN(LFDS611_ALIGN_DOUBLE_POINTER) lfds611_atom_t
  exchange[2],
           compare[2];

  assert( shared_counter != NULL );
  assert( local_counter != NULL );

  while( loop++ < 1000000 ) {
    compare[0] = *shared_counter;
    compare[1] = *(shared_counter+1);

    do {
      exchange[0] = compare[0] + 1;
      exchange[1] = compare[1];
    } while( 0 == lfds611_abstraction_dcas(shared_counter, exchange, compare) );

    (*local_counter)++;
  }

  return;
}

