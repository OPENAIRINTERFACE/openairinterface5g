/***** public prototypes *****/
#include "liblfds611.h"

/***** defines *****/
#define and &&
#define or  ||

#define RAISED   1
#define LOWERED  0

#define NO_FLAGS 0x0

/***** private prototypes *****/
void *lfds611_liblfds_aligned_malloc( size_t size, size_t align_in_bytes );
void lfds611_liblfds_aligned_free( void *memory );

static LFDS611_INLINE lfds611_atom_t lfds611_abstraction_cas( volatile lfds611_atom_t *destination, lfds611_atom_t exchange, lfds611_atom_t compare );
static LFDS611_INLINE unsigned char lfds611_abstraction_dcas( volatile lfds611_atom_t *destination, lfds611_atom_t *exchange, lfds611_atom_t *compare );
static LFDS611_INLINE lfds611_atom_t lfds611_abstraction_increment( volatile lfds611_atom_t *value );

/***** inlined code *****/
#include "lfds611_abstraction/lfds611_abstraction_cas.c"
#include "lfds611_abstraction/lfds611_abstraction_dcas.c"
#include "lfds611_abstraction/lfds611_abstraction_increment.c"

