#include <openair3/UICC/usim_interface.h>

typedef struct {
  uicc_t * uicc;
} nr_user_nas_t;

int identityResponse(void **msg, nr_user_nas_t *UE);
