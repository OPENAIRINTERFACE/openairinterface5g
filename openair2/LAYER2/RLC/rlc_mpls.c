/*
                                rlc_mpls.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
*/

#define RLC_MPLS_C
#include "rlc.h"


//-----------------------------------------------------------------------------
rlc_op_status_t mpls_rlc_data_req     (
  const protocol_ctxt_t* const ctxtP,
  const rb_id_t rb_idP,
  const sdu_size_t sdu_sizeP,
  mem_block_t* const sduP)
{
  //-----------------------------------------------------------------------------
  // third arg should be set to 1 or 0
  return rlc_data_req(ctxtP, SRB_FLAG_NO, MBMS_FLAG_NO, rb_idP, RLC_MUI_UNDEFINED, RLC_SDU_CONFIRM_NO, sdu_sizeP, sduP);
}

