/***** includes *****/
#include "lfds700_list_addonly_ordered_singlylinked_internal.h"





/****************************************************************************/
void lfds700_list_aos_cleanup( struct lfds700_list_aos_state *laoss,
                               void (*element_cleanup_callback)(struct lfds700_list_aos_state *laoss, struct lfds700_list_aos_element *laose) )
{
  struct lfds700_list_aos_element
    *laose,
    *temp;

  LFDS700_PAL_ASSERT( laoss != NULL );
  // TRD : element_cleanup_callback can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( element_cleanup_callback == NULL )
    return;

  laose = LFDS700_LIST_AOS_GET_START( *laoss );

  while( laose != NULL )
  {
    temp = laose;

    laose = LFDS700_LIST_AOS_GET_NEXT( *laose );

    element_cleanup_callback( laoss, temp );
  }

  return;
}

