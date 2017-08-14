#include "lfds611_freelist_internal.h"





/****************************************************************************/
struct lfds611_freelist_element *lfds611_freelist_pop( struct lfds611_freelist_state *fs, struct lfds611_freelist_element **fe )
{
  LFDS611_ALIGN(LFDS611_ALIGN_DOUBLE_POINTER) struct lfds611_freelist_element
      *fe_local[LFDS611_FREELIST_PAC_SIZE];

  assert( fs != NULL );
  assert( fe != NULL );

  LFDS611_BARRIER_LOAD;

  fe_local[LFDS611_FREELIST_COUNTER] = fs->top[LFDS611_FREELIST_COUNTER];
  fe_local[LFDS611_FREELIST_POINTER] = fs->top[LFDS611_FREELIST_POINTER];

  /* TRD : note that lfds611_abstraction_dcas loads the original value of the destination (fs->top) into the compare (fe_local)
           (this happens of course after the CAS itself has occurred inside lfds611_abstraction_dcas)
  */

  do {
    if( fe_local[LFDS611_FREELIST_POINTER] == NULL ) {
      *fe = NULL;
      return( *fe );
    }
  } while( 0 == lfds611_abstraction_dcas((volatile lfds611_atom_t *) fs->top, (lfds611_atom_t *) fe_local[LFDS611_FREELIST_POINTER]->next, (lfds611_atom_t *) fe_local) );

  *fe = (struct lfds611_freelist_element *) fe_local[LFDS611_FREELIST_POINTER];

  return( *fe );
}





/****************************************************************************/
struct lfds611_freelist_element *lfds611_freelist_guaranteed_pop( struct lfds611_freelist_state *fs, struct lfds611_freelist_element **fe )
{
  assert( fs != NULL );
  assert( fe != NULL );

  lfds611_freelist_internal_new_element( fs, fe );

  return( *fe );
}





/****************************************************************************/
void lfds611_freelist_push( struct lfds611_freelist_state *fs, struct lfds611_freelist_element *fe )
{
  LFDS611_ALIGN(LFDS611_ALIGN_DOUBLE_POINTER) struct lfds611_freelist_element
      *fe_local[LFDS611_FREELIST_PAC_SIZE],
      *original_fe_next[LFDS611_FREELIST_PAC_SIZE];

  assert( fs != NULL );
  assert( fe != NULL );

  LFDS611_BARRIER_LOAD;

  fe_local[LFDS611_FREELIST_POINTER] = fe;
  fe_local[LFDS611_FREELIST_COUNTER] = (struct lfds611_freelist_element *) lfds611_abstraction_increment( (lfds611_atom_t *) &fs->aba_counter );

  original_fe_next[LFDS611_FREELIST_POINTER] = fs->top[LFDS611_FREELIST_POINTER];
  original_fe_next[LFDS611_FREELIST_COUNTER] = fs->top[LFDS611_FREELIST_COUNTER];

  /* TRD : note that lfds611_abstraction_dcas loads the original value of the destination (fs->top) into the compare (original_fe_next)
           (this happens of course after the CAS itself has occurred inside lfds611_abstraction_dcas)
           this then causes us in our loop, should we repeat it, to update fe_local->next to a more
           up-to-date version of the head of the lfds611_freelist
  */

  do {
    fe_local[LFDS611_FREELIST_POINTER]->next[LFDS611_FREELIST_POINTER] = original_fe_next[LFDS611_FREELIST_POINTER];
    fe_local[LFDS611_FREELIST_POINTER]->next[LFDS611_FREELIST_COUNTER] = original_fe_next[LFDS611_FREELIST_COUNTER];
  } while( 0 == lfds611_abstraction_dcas((volatile lfds611_atom_t *) fs->top, (lfds611_atom_t *) fe_local, (lfds611_atom_t *) original_fe_next) );

  return;
}

