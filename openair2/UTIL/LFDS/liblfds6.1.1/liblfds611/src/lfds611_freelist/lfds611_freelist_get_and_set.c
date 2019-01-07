#include "lfds611_freelist_internal.h"





/****************************************************************************/
void *lfds611_freelist_get_user_data_from_element( struct lfds611_freelist_element *fe, void **user_data )
{
  assert( fe != NULL );
  // TRD : user_data can be NULL

  LFDS611_BARRIER_LOAD;

  if( user_data != NULL )
    *user_data = fe->user_data;

  return( fe->user_data );
}





/****************************************************************************/
void lfds611_freelist_set_user_data_in_element( struct lfds611_freelist_element *fe, void *user_data )
{
  assert( fe != NULL );
  // TRD : user_data can be NULL

  fe->user_data = user_data;

  LFDS611_BARRIER_STORE;

  return;
}

