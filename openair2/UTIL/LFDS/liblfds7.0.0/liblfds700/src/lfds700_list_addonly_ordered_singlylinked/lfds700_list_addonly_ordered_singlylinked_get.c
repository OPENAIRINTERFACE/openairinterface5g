/***** includes *****/
#include "lfds700_list_addonly_ordered_singlylinked_internal.h"





/****************************************************************************/
int lfds700_list_aos_get_by_key( struct lfds700_list_aos_state *laoss,
                                  void *key,
                                  struct lfds700_list_aos_element **laose )
{
  int
    cr = !0,
    rv = 1;

  LFDS700_PAL_ASSERT( laoss != NULL );
  LFDS700_PAL_ASSERT( key != NULL );
  LFDS700_PAL_ASSERT( laose != NULL );

  while( cr != 0 and LFDS700_LIST_AOS_GET_START_AND_THEN_NEXT(*laoss, *laose) )
    cr = laoss->key_compare_function( key, (*laose)->key );

  if( *laose == NULL )
    rv = 0;

  return( rv );
}

