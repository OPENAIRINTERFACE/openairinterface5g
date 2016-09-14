#define RLC_AM_MODULE 1
#define RLC_AM_WINDOWS_C 1
//-----------------------------------------------------------------------------
#if USER_MODE
#include <assert.h>
#endif
//-----------------------------------------------------------------------------
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "rlc_am.h"
#include "UTIL/LOG/log.h"
//-----------------------------------------------------------------------------
signed int rlc_am_in_tx_window(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t snP)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if (snP >= RLC_AM_SN_MODULO) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(snP - rlc_pP->vt_a)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(rlc_pP->vt_ms - rlc_pP->vt_a)) % RLC_AM_SN_MODULO;

  if ((shifted_sn >= 0) && (shifted_sn < upper_bound)) {
    return 1;
  } else {
    return 0;
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_in_rx_window(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t snP)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if (snP >= RLC_AM_SN_MODULO) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(snP - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(rlc_pP->vr_mr - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;

  if ((shifted_sn >= 0) && (shifted_sn < upper_bound)) {
    return 1;
  } else {
    return 0;
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_sn_gte_vr_h(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t snP)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if (snP >= RLC_AM_SN_MODULO) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(snP - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(rlc_pP->vr_h - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;

  if (shifted_sn >= upper_bound) {
    return 1;
  } else {
    return 0;
  }
}
//-----------------------------------------------------------------------------
signed int rlc_am_sn_gte_vr_x(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t snP)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if (snP >= RLC_AM_SN_MODULO) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(snP - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(rlc_pP->vr_x - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;

  if (shifted_sn >= upper_bound) {
    return 1;
  } else {
    return 0;
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_sn_gt_vr_ms(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t snP)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if (snP >= RLC_AM_SN_MODULO) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(snP - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(rlc_pP->vr_ms - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;

  if (shifted_sn > upper_bound) {
    return 1;
  } else {
    return 0;
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_tx_sn1_gt_sn2(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t sn1P,
  const rlc_sn_t sn2P)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if ((sn1P >= RLC_AM_SN_MODULO) || (sn2P >= RLC_AM_SN_MODULO)) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(sn1P - rlc_pP->vt_a)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(sn2P - rlc_pP->vt_a)) % RLC_AM_SN_MODULO;

  if (shifted_sn > upper_bound) {
    return 1;
  } else {
    return 0;
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_rx_sn1_gt_sn2(
  const protocol_ctxt_t* const  ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  const rlc_sn_t sn1P,
  const rlc_sn_t sn2P)
{
  rlc_usn_t shifted_sn;
  rlc_usn_t upper_bound;

  if ((sn1P >= RLC_AM_SN_MODULO) || (sn2P >= RLC_AM_SN_MODULO)) {
    return 0;
  }

  shifted_sn  = ((rlc_usn_t)(sn1P - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;
  upper_bound = ((rlc_usn_t)(sn2P - rlc_pP->vr_r)) % RLC_AM_SN_MODULO;

  if (shifted_sn > upper_bound) {
    return 1;
  } else {
    return 0;
  }
}
