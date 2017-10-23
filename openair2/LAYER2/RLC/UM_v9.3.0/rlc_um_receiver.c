/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#define RLC_UM_MODULE 1
#define RLC_UM_RECEIVER_C 1
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "rlc.h"
#include "rlc_um.h"
#include "rlc_um_structs.h"
#include "rlc_primitives.h"
#include "mac_primitives.h"
#include "list.h"
#include "UTIL/LOG/log.h"

//-----------------------------------------------------------------------------
void
rlc_um_display_rx_window(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP
)
{
  unsigned long sn = 0;
  unsigned long end_sn = 0;
  char          str[4];
  char          time_out_str[11];
  int           str_index;
  char          color[32];

  LOG_T(RLC, "\n");
  LOG_T(RLC, "+-------------------------------------------------------------------------------------------------------+");
  LOG_T(RLC, "\n");
  sprintf(time_out_str, "%010d", rlc_pP->t_reordering.ms_duration);
  time_out_str[10] = 0;
  LOG_T(RLC, "| RLC UM RB %02d    VR(UR)=%03d    VR(UX)=%03d    VR(UH)=%03d    t-Reordering: %s %s %s             |",
        rlc_pP->rb_id, rlc_pP->vr_ur, rlc_pP->vr_ux, rlc_pP->vr_uh,
        (rlc_pP->t_reordering.running)?" ON":"OFF",
        (rlc_pP->t_reordering.running)?"Time-out frameP:":"               ",
        (rlc_pP->t_reordering.running)?time_out_str:"          ");
  LOG_T(RLC, "\n");
  LOG_T(RLC, "+------+------------------------------------------------------------------------------------------------+");
  LOG_T(RLC, "\n");
  LOG_T(RLC, "|      |00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 |");
  LOG_T(RLC, "\n");
  LOG_T(RLC, "+------+------------------------------------------------------------------------------------------------+");
  LOG_T(RLC, "\n");

  if (rlc_pP->rx_sn_length == 10) {
    end_sn = RLC_UM_SN_10_BITS_MODULO;
  } else {
    end_sn = RLC_UM_SN_5_BITS_MODULO;
  }


  for (sn = 0; sn < end_sn; sn++) {
    str[0]    = ' ';
    str[1]    = ' ';
    str[2]    = ' ';
    str[3]    = 0;
    str_index = 0;

    if ((sn % 32) == 0) {
      if ((sn != 0)) {
        LOG_T(RLC, "%s%s|", RLC_FG_COLOR_DEFAULT, RLC_NORMAL_VIDEO);
        LOG_T(RLC, "\n");
      }

      LOG_T(RLC, "%s%s| %04lu |", RLC_FG_COLOR_DEFAULT, RLC_NORMAL_VIDEO, sn);
    }

    strcpy(color, RLC_FG_COLOR_DEFAULT);

    if (sn == rlc_pP->vr_ur) {
      str[str_index++] = 'R';
      strcpy(color, RLC_FG_COLOR_BLUE);
    }

    if (sn == rlc_pP->vr_ux) {
      str[str_index++] = 'X';
      strcpy(color, RLC_FG_COLOR_ORANGE);
    }

    if (sn == rlc_pP->vr_uh) {
      str[str_index++] = 'H';
      strcpy(color, RLC_FG_COLOR_RED);
    }

    if (rlc_um_get_pdu_from_dar_buffer(ctxt_pP, rlc_pP, sn)) {
      // test RLC_REVERSE_VIDEO
      if (str_index <= 2) {
        str[str_index] = '.';
      }

      LOG_T(RLC, "%s%s%s", color, RLC_REVERSE_VIDEO, str);
    } else {
      LOG_T(RLC, "%s%s%s", color, RLC_NORMAL_VIDEO, str);
    }
  }

  LOG_T(RLC, "%s%s|", RLC_FG_COLOR_DEFAULT, RLC_NORMAL_VIDEO);
  LOG_T(RLC, "\n");
  LOG_T(RLC, "+------+------------------------------------------------------------------------------------------------+");
  LOG_T(RLC, "\n");
}

//-----------------------------------------------------------------------------
void
rlc_um_receive (
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP,
  struct mac_data_ind data_indP)
{

  mem_block_t        *tb_p             = NULL;
  uint8_t               *first_byte_p     = NULL;
  uint16_t               tb_size_in_bytes = 0;

  while ((tb_p = list_remove_head (&data_indP.data))) {

    first_byte_p = ((struct mac_tb_ind *) (tb_p->data))->data_ptr;
    tb_size_in_bytes = ((struct mac_tb_ind *) (tb_p->data))->size;

    rlc_pP->stat_rx_data_bytes += tb_size_in_bytes;
    rlc_pP->stat_rx_data_pdu   += 1;

    if (tb_size_in_bytes > 0) {
      rlc_um_receive_process_dar (ctxt_pP, rlc_pP, tb_p, (rlc_um_pdu_sn_10_t*)first_byte_p, tb_size_in_bytes);
#if defined(TRACE_RLC_UM_RX)
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" VR(UR)=%03d VR(UX)=%03d VR(UH)=%03d\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
            rlc_pP->vr_ur,
            rlc_pP->vr_ux,
            rlc_pP->vr_uh);
      //rlc_um_display_rx_window(rlc_pP); commented because bad display
#endif
    }
  }
}
