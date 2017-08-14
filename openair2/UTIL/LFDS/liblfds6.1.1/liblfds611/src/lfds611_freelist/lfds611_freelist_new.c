#include "lfds611_freelist_internal.h"





/****************************************************************************/
int lfds611_freelist_new( struct lfds611_freelist_state **fs, lfds611_atom_t number_elements, int (*user_data_init_function)(void **user_data, void *user_state), void *user_state )
{
  int
  rv = 0;

  lfds611_atom_t
  element_count;

  assert( fs != NULL );
  // TRD : number_elements can be any value in its range
  // TRD : user_data_init_function can be NULL

  *fs = (struct lfds611_freelist_state *) lfds611_liblfds_aligned_malloc( sizeof(struct lfds611_freelist_state), LFDS611_ALIGN_DOUBLE_POINTER );

  if( (*fs) != NULL ) {
    (*fs)->top[LFDS611_FREELIST_POINTER] = NULL;
    (*fs)->top[LFDS611_FREELIST_COUNTER] = 0;
    (*fs)->user_data_init_function = user_data_init_function;
    (*fs)->user_state = user_state;
    (*fs)->aba_counter = 0;
    (*fs)->element_count = 0;

    element_count = lfds611_freelist_new_elements( *fs, number_elements );

    if( element_count == number_elements )
      rv = 1;

    if( element_count != number_elements ) {
      lfds611_liblfds_aligned_free( (*fs) );
      *fs = NULL;
    }
  }

  LFDS611_BARRIER_STORE;

  return( rv );
}





/****************************************************************************/
//#pragma warning( disable : 4100 )

void lfds611_freelist_use( struct lfds611_freelist_state *fs )
{
  assert( fs != NULL );

  LFDS611_BARRIER_LOAD;

  return;
}

//#pragma warning( default : 4100 )





/****************************************************************************/
lfds611_atom_t lfds611_freelist_new_elements( struct lfds611_freelist_state *fs, lfds611_atom_t number_elements )
{
  struct lfds611_freelist_element
      *fe;

  lfds611_atom_t
  loop,
  count = 0;

  assert( fs != NULL );
  // TRD : number_elements can be any value in its range
  // TRD : user_data_init_function can be NULL

  for( loop = 0 ; loop < number_elements ; loop++ )
    if( lfds611_freelist_internal_new_element(fs, &fe) ) {
      lfds611_freelist_push( fs, fe );
      count++;
    }

  return( count );
}





/****************************************************************************/
lfds611_atom_t lfds611_freelist_internal_new_element( struct lfds611_freelist_state *fs, struct lfds611_freelist_element **fe )
{
  lfds611_atom_t
  rv = 0;

  assert( fs != NULL );
  assert( fe != NULL );

  /* TRD : basically, does what you'd expect;

           allocates an element
           calls the user init function
           if anything fails, cleans up,
           sets *fe to NULL
           and returns 0
  */

  *fe = (struct lfds611_freelist_element *) lfds611_liblfds_aligned_malloc( sizeof(struct lfds611_freelist_element), LFDS611_ALIGN_DOUBLE_POINTER );

  if( *fe != NULL ) {
    if( fs->user_data_init_function == NULL ) {
      (*fe)->user_data = NULL;
      rv = 1;
    }

    if( fs->user_data_init_function != NULL ) {
      rv = fs->user_data_init_function( &(*fe)->user_data, fs->user_state );

      if( rv == 0 ) {
        lfds611_liblfds_aligned_free( *fe );
        *fe = NULL;
      }
    }
  }

  if( rv == 1 )
    lfds611_abstraction_increment( (lfds611_atom_t *) &fs->element_count );

  return( rv );
}

