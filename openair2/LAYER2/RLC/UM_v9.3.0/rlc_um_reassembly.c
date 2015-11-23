/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#define RLC_UM_MODULE 1
#define RLC_UM_REASSEMBLY_C 1
#include "platform_types.h"
//-----------------------------------------------------------------------------
#if USER_MODE
#include <string.h>
#endif
#if ENABLE_ITTI
# include "platform_types.h"
# include "intertask_interface.h"
#endif
#include "assertions.h"
#include "rlc.h"
#include "rlc_um.h"
#include "rlc_primitives.h"
#include "list.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "msc.h"

//-----------------------------------------------------------------------------
inline void
rlc_um_clear_rx_sdu (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t* rlc_pP)
{
  rlc_pP->output_sdu_size_to_write = 0;
}

//-----------------------------------------------------------------------------
void
rlc_um_reassembly (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t *rlc_pP, uint8_t * src_pP, int32_t lengthP)
{
  sdu_size_t      sdu_max_size;

  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[REASSEMBLY] reassembly()  %d bytes %d bytes already reassemblied\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
        lengthP,
        rlc_pP->output_sdu_size_to_write);

  if (lengthP <= 0) {
    return;
  }

  if ((rlc_pP->is_data_plane)) {
    sdu_max_size = RLC_SDU_MAX_SIZE_DATA_PLANE;
  } else {
    sdu_max_size = RLC_SDU_MAX_SIZE_CONTROL_PLANE;
  }

  if (rlc_pP->output_sdu_in_construction == NULL) {
    //    msg("[RLC_UM_LITE] Getting mem_block ...\n");
    rlc_pP->output_sdu_in_construction = get_free_mem_block (sdu_max_size);
    rlc_pP->output_sdu_size_to_write = 0;
  }

  if ((rlc_pP->output_sdu_in_construction)) {
    // check if no overflow in size
    if ((rlc_pP->output_sdu_size_to_write + lengthP) <= sdu_max_size) {
      memcpy (&rlc_pP->output_sdu_in_construction->data[rlc_pP->output_sdu_size_to_write], src_pP, lengthP);
      rlc_pP->output_sdu_size_to_write += lengthP;
#if TRACE_RLC_UM_DISPLAY_ASCII_DATA
      rlc_pP->output_sdu_in_construction->data[rlc_pP->output_sdu_size_to_write] = 0;
      LOG_T(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[REASSEMBLY] DATA :",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
      rlc_util_print_hex_octets(RLC, (unsigned char*)rlc_pP->output_sdu_in_construction->data, rlc_pP->output_sdu_size_to_write);
#endif
    } else {
#if STOP_ON_IP_TRAFFIC_OVERLOAD
      AssertFatal(0, PROTOCOL_RLC_UM_CTXT_FMT" RLC_UM_DATA_IND, SDU TOO BIG, DROPPED\n",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
      LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[REASSEMBLY] [max_sdu size %d] ERROR  SDU SIZE OVERFLOW SDU GARBAGED\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
            sdu_max_size);
      // erase  SDU
      rlc_pP->output_sdu_size_to_write = 0;
    }
  } else {
    LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[REASSEMBLY]ERROR  OUTPUT SDU IS NULL\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
#if STOP_ON_IP_TRAFFIC_OVERLOAD
    AssertFatal(0, PROTOCOL_RLC_UM_CTXT_FMT" RLC_UM_DATA_IND, SDU DROPPED, OUT OF MEMORY\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
  }

}
//-----------------------------------------------------------------------------
void
rlc_um_send_sdu (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t *rlc_pP)
{
  if ((rlc_pP->output_sdu_in_construction)) {
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" SEND_SDU to upper layers %d bytes sdu %p\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->output_sdu_size_to_write,
          rlc_pP->output_sdu_in_construction);

    if (rlc_pP->output_sdu_size_to_write > 0) {
      rlc_pP->stat_rx_pdcp_sdu += 1;
      rlc_pP->stat_rx_pdcp_bytes += rlc_pP->output_sdu_size_to_write;

      MSC_LOG_TX_MESSAGE(
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
        (const char*)(rlc_pP->output_sdu_in_construction->data),
        rlc_pP->output_sdu_size_to_write,
        MSC_AS_TIME_FMT" "PROTOCOL_RLC_UM_MSC_FMT" DATA-IND size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        PROTOCOL_RLC_UM_MSC_ARGS(ctxt_pP,rlc_pP),
        rlc_pP->output_sdu_size_to_write
      );

#if TEST_RLC_UM
#if TRACE_RLC_UM_DISPLAY_ASCII_DATA
      rlc_pP->output_sdu_in_construction->data[rlc_pP->output_sdu_size_to_write] = 0;
      LOG_T(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[SEND_SDU] DATA :",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
      rlc_util_print_hex_octets(RLC, rlc_pP->output_sdu_in_construction->data, rlc_pP->output_sdu_size_to_write);
#endif
      rlc_um_v9_3_0_test_data_ind (rlc_pP->module_id, rlc_pP->rb_id, rlc_pP->output_sdu_size_to_write, rlc_pP->output_sdu_in_construction);
#else
      // msg("[RLC] DATA IND ON MOD_ID %d RB ID %d, size %d\n",rlc_pP->module_id, rlc_pP->rb_id, ctxt_pP->frame,rlc_pP->output_sdu_size_to_write);
      rlc_data_ind (
        ctxt_pP,
        BOOL_NOT(rlc_pP->is_data_plane),
        rlc_pP->is_mxch,
        rlc_pP->rb_id,
        rlc_pP->output_sdu_size_to_write,
        rlc_pP->output_sdu_in_construction);
#endif
      rlc_pP->output_sdu_in_construction = NULL;
    } else {
      LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[SEND_SDU] ERROR SIZE <= 0 ... DO NOTHING, SET SDU SIZE TO 0\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
    }

    rlc_pP->output_sdu_size_to_write = 0;
  }
}
