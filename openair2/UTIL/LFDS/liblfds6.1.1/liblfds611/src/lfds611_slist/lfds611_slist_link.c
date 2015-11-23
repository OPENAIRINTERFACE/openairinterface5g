#include "lfds611_slist_internal.h"





/****************************************************************************/
void lfds611_slist_internal_link_element_to_head( struct lfds611_slist_state *ss, struct lfds611_slist_element *volatile se )
{
  LFDS611_ALIGN(LFDS611_ALIGN_SINGLE_POINTER) struct lfds611_slist_element
      *se_next;

  assert( ss != NULL );
  assert( se != NULL );

  LFDS611_BARRIER_LOAD;

  se_next = ss->head;

  do {
    se->next = se_next;
  } while( se->next != (se_next = (struct lfds611_slist_element *) lfds611_abstraction_cas((volatile lfds611_atom_t *) &ss->head, (lfds611_atom_t) se, (lfds611_atom_t) se->next)) );

  return;
}





/****************************************************************************/
void lfds611_slist_internal_link_element_after_element( struct lfds611_slist_element *volatile lfds611_slist_in_list_element, struct lfds611_slist_element *volatile se )
{
  LFDS611_ALIGN(LFDS611_ALIGN_SINGLE_POINTER) struct lfds611_slist_element
      *se_prev,
      *se_next;

  assert( lfds611_slist_in_list_element != NULL );
  assert( se != NULL );

  LFDS611_BARRIER_LOAD;

  se_prev = (struct lfds611_slist_element *) lfds611_slist_in_list_element;

  se_next = se_prev->next;

  do {
    se->next = se_next;
  } while( se->next != (se_next = (struct lfds611_slist_element *) lfds611_abstraction_cas((volatile lfds611_atom_t *) &se_prev->next, (lfds611_atom_t) se, (lfds611_atom_t) se->next)) );

  return;
}

