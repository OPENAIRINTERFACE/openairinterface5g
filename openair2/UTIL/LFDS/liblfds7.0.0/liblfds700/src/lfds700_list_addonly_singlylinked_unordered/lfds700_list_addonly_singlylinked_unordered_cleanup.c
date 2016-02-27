/***** includes *****/
#include "lfds700_list_addonly_singlylinked_unordered_internal.h"





/****************************************************************************/
void lfds700_list_asu_cleanup( struct lfds700_list_asu_state *lasus,
                               void (*element_cleanup_callback)(struct lfds700_list_asu_state *lasus, struct lfds700_list_asu_element *lasue) )
{
  struct lfds700_list_asu_element
    *lasue,
    *temp;

  LFDS700_PAL_ASSERT( lasus != NULL );
  // TRD : element_cleanup_callback can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( element_cleanup_callback == NULL )
    return;

  lasue = LFDS700_LIST_ASU_GET_START( *lasus );

  while( lasue != NULL )
  {
    temp = lasue;

    lasue = LFDS700_LIST_ASU_GET_NEXT( *lasue );

    element_cleanup_callback( lasus, temp );
  }

  return;
}

