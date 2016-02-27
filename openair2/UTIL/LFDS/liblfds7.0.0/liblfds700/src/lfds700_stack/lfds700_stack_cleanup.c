/***** includes *****/
#include "lfds700_stack_internal.h"





/****************************************************************************/
void lfds700_stack_cleanup( struct lfds700_stack_state *ss,
                            void (*element_cleanup_callback)(struct lfds700_stack_state *ss, struct lfds700_stack_element *se) )
{
  struct lfds700_stack_element
    *se,
    *se_temp;

  LFDS700_PAL_ASSERT( ss != NULL );
  // TRD : element_cleanup_callback can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( element_cleanup_callback != NULL )
  {
    se = ss->top[POINTER];

    while( se != NULL )
    {
      se_temp = se;
      se = se->next;

      element_cleanup_callback( ss, se_temp );
    }
  }

  return;
}

