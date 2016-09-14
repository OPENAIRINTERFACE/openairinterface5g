#include "fundamental_type.h"
#include "struct_type.h"

#ifndef INTERTASK_CONTEXTS_H_
#define INTERTASK_CONTEXTS_H_

typedef struct {
    /* List of fundamental types that can contruct any other type */
    fundamental_type_t *ft_list;

    struct_type_t *struct_list;
} intertask_context_t;

#endif /* INTERTASK_CONTEXTS_H_ */
