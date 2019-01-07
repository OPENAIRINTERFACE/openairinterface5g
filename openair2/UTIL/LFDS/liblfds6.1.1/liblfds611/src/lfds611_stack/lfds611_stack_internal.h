/***** the library wide include file *****/
#include "liblfds611_internal.h"

/***** pragmas *****/

/***** defines *****/
#define LFDS611_STACK_POINTER 0
#define LFDS611_STACK_COUNTER 1
#define LFDS611_STACK_PAC_SIZE 2

/***** structures *****/
#pragma pack( push, LFDS611_ALIGN_DOUBLE_POINTER )

struct lfds611_stack_state {
  // TRD : must come first for alignment
  struct lfds611_stack_element
      *volatile top[LFDS611_STACK_PAC_SIZE];

  lfds611_atom_t
  aba_counter;

  struct lfds611_freelist_state
      *fs;
};

struct lfds611_stack_element {
  struct lfds611_stack_element
      *next[LFDS611_STACK_PAC_SIZE];

  struct lfds611_freelist_element
      *fe;

  void
  *user_data;
};

#pragma pack( pop )

/***** private prototypes *****/
int lfds611_stack_internal_freelist_init_function( void **user_data, void *user_state );
void lfds611_stack_internal_freelist_delete_function( void *user_data, void *user_state );

void lfds611_stack_internal_new_element_from_freelist( struct lfds611_stack_state *ss, struct lfds611_stack_element *se[LFDS611_STACK_PAC_SIZE], void *user_data );
void lfds611_stack_internal_new_element( struct lfds611_stack_state *ss, struct lfds611_stack_element *se[LFDS611_STACK_PAC_SIZE], void *user_data );
void lfds611_stack_internal_init_element( struct lfds611_stack_state *ss, struct lfds611_stack_element *se[LFDS611_STACK_PAC_SIZE], struct lfds611_freelist_element *fe, void *user_data );

void lfds611_stack_internal_push( struct lfds611_stack_state *ss, struct lfds611_stack_element *se[LFDS611_STACK_PAC_SIZE] );

void lfds611_stack_internal_validate( struct lfds611_stack_state *ss, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *stack_validity,
                                      enum lfds611_data_structure_validity *freelist_validity );

