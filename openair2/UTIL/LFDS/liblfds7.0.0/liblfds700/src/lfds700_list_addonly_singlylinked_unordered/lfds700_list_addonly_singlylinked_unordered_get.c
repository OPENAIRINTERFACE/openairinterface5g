/***** includes *****/
#include "lfds700_list_addonly_singlylinked_unordered_internal.h"





/****************************************************************************/
int lfds700_list_asu_get_by_key( struct lfds700_list_asu_state *lasus,
                                 void *key,
                                 struct lfds700_list_asu_element **lasue )
{
  int
    cr = !0,
    rv = 1;

  LFDS700_PAL_ASSERT( lasus != NULL );
  LFDS700_PAL_ASSERT( key != NULL );
  LFDS700_PAL_ASSERT( lasue != NULL );

  while( cr != 0 and LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*lasus, *lasue) )
    cr = lasus->key_compare_function( key, (*lasue)->key );

  if( *lasue == NULL )
    rv = 0;

  return( rv );
}

