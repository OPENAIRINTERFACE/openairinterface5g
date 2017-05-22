/***** includes *****/
#include "lfds700_freelist_internal.h"





/****************************************************************************/
void lfds700_freelist_cleanup( struct lfds700_freelist_state *fs,
                               void (*element_cleanup_callback)(struct lfds700_freelist_state *fs, struct lfds700_freelist_element *fe) )
{
  struct lfds700_freelist_element
    *fe,
    *fe_temp;

  LFDS700_PAL_ASSERT( fs != NULL );
  // TRD : element_cleanup_callback can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( element_cleanup_callback != NULL )
  {
    fe = fs->top[POINTER];

    while( fe != NULL )
    {
      fe_temp = fe;
      fe = fe->next;

      element_cleanup_callback( fs, fe_temp );
    }
  }

  return;
}

