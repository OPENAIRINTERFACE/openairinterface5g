#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "esm_proc.h"
#include "msc.h"

#ifndef NAS_ITTI_MESSAGING_H_
#define NAS_ITTI_MESSAGING_H_

# if (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE))
int nas_itti_plain_msg(
  const char *buffer,
  const nas_message_t *msg,
  const int lengthP,
  const int instance);

int nas_itti_protected_msg(
  const char *buffer,
  const nas_message_t *msg,
  const int lengthP,
  const int instance);
# endif


# if defined(NAS_BUILT_IN_UE)
int nas_itti_cell_info_req(const plmn_t plmnID, const Byte_t rat);

int nas_itti_nas_establish_req(as_cause_t cause, as_call_type_t type, as_stmsi_t s_tmsi, plmn_t plmnID, Byte_t *data_pP, uint32_t lengthP);

int nas_itti_ul_data_req(const uint32_t ue_idP, void *const data_pP, const uint32_t lengthP);

int nas_itti_rab_establish_rsp(const as_stmsi_t s_tmsi, const as_rab_id_t rabID, const nas_error_code_t errCode);
# endif
#endif /* NAS_ITTI_MESSAGING_H_ */
