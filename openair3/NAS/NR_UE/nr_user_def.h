#ifndef NR_USER_DEF_H
#define NR_USER_DEF_H
#include <openair3/UICC/usim_interface.h>

typedef struct {
  uicc_t *uicc;
} nr_user_nas_t;

#define STATIC_ASSERT(test_for_true) _Static_assert((test_for_true), "(" #test_for_true ") failed")
#define myCalloc(var, type) type * var=(type*)calloc(sizeof(type),1);
#define arrayCpy(tO, FroM)  STATIC_ASSERT(sizeof(tO) == sizeof(FroM)) ; memcpy(tO, FroM, sizeof(tO))

int identityResponse(void **msg, nr_user_nas_t *UE);
#endif
