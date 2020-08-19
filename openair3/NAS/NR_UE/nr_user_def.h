#ifndef NR_USER_DEF_H
#define NR_USER_DEF_H
#include <openair3/UICC/usim_interface.h>

typedef struct {
  uicc_t *uicc;
} nr_user_nas_t;

#define myCalloc(var, type) type * var=(type*)calloc(sizeof(type),1);

int identityResponse(void **msg, nr_user_nas_t *UE);
#endif
