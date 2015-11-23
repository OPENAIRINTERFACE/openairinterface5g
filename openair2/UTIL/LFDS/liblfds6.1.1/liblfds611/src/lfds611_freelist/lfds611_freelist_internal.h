/***** the library wide include file *****/
#include "liblfds611_internal.h"

/***** defines *****/
#define LFDS611_FREELIST_POINTER 0
#define LFDS611_FREELIST_COUNTER 1
#define LFDS611_FREELIST_PAC_SIZE 2

/***** structures *****/
#pragma pack( push, LFDS611_ALIGN_DOUBLE_POINTER )

struct lfds611_freelist_state {
  struct lfds611_freelist_element
      *volatile top[LFDS611_FREELIST_PAC_SIZE];

  int
  (*user_data_init_function)( void **user_data, void *user_state );

  void
  *user_state;

  lfds611_atom_t
  aba_counter,
  element_count;
};

struct lfds611_freelist_element {
  struct lfds611_freelist_element
      *next[LFDS611_FREELIST_PAC_SIZE];

  void
  *user_data;
};

#pragma pack( pop )

/***** private prototypes *****/
lfds611_atom_t lfds611_freelist_internal_new_element( struct lfds611_freelist_state *fs, struct lfds611_freelist_element **fe );
void lfds611_freelist_internal_validate( struct lfds611_freelist_state *fs, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *lfds611_freelist_validity );

