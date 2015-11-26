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
#define RLC_AM_MODULE 1
#define RLC_AM_RX_LIST_C 1
//-----------------------------------------------------------------------------
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "assertions.h"
#include "list.h"
#include "rlc_am.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"



// returns 0 if success
// returns neg value if failure
//-----------------------------------------------------------------------------
signed int
rlc_am_rx_list_insert_pdu(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP,
  mem_block_t* const tb_pP)
{
  rlc_am_pdu_info_t* pdu_info_p                  = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
  rlc_am_pdu_info_t* pdu_info_cursor_p           = NULL;
  rlc_am_pdu_info_t* pdu_info_previous_cursor_p  = NULL;
  mem_block_t*       cursor_p                    = NULL;
  mem_block_t*       previous_cursor_p           = NULL;
  cursor_p = rlc_pP->receiver_buffer.head;
  // it is assumed this pdu is in rx window

  if (cursor_p) {
    if (rlc_pP->vr_mr < rlc_pP->vr_r) {
      if (pdu_info_p->sn >= rlc_pP->vr_r) {
        pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

        while ((cursor_p != NULL)  && (pdu_info_cursor_p->sn >= rlc_pP->vr_r)) { // LG added =
          if (pdu_info_p->sn < pdu_info_cursor_p->sn) {
            if (previous_cursor_p != NULL) {
              pdu_info_previous_cursor_p = &((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info;

              if (pdu_info_previous_cursor_p->sn == pdu_info_p->sn) {
                if (pdu_info_p->rf != pdu_info_previous_cursor_p->rf) {
                  LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d WRONG RF -> DROPPED (vr(mr) < vr(r) and sn >= vr(r))\n",
                        ctxt_pP->frame,
                        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                        __LINE__,
                        pdu_info_p->sn);
                  return -2;
                } else if (pdu_info_p->rf == 1) {
                  if ((pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - 1) >= pdu_info_p->so) {
                    LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SO OVERLAP -> DROPPED (vr(mr) < vr(r) and sn >= vr(r))\n",
                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                          __LINE__,
                          pdu_info_p->sn);
                    return -2;
                  }
                }
              }
            }

            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) > vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
            return 0;

          } else if (pdu_info_p->sn == pdu_info_cursor_p->sn) {
            if (pdu_info_cursor_p->rf == 0) {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DUPLICATE -> DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            } else if (pdu_info_p->rf == 1) {
              if ((pdu_info_p->so + pdu_info_p->payload_size - 1) < pdu_info_cursor_p->so) {

                if (previous_cursor_p != NULL) {
                  pdu_info_previous_cursor_p = &((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info;

                  if (pdu_info_previous_cursor_p->sn == pdu_info_cursor_p->sn) {
                    if ((pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - 1) < pdu_info_p->so) {

                      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SEGMENT OFFSET %05d (vr(mr) < vr(r) and sn >= vr(r))\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                            __LINE__,
                            pdu_info_p->sn,
                            pdu_info_p->so);
                      list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
                      return 0;
                    } else {
                      LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP PREVIOUS SO DUPLICATE -> DROPPED\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                            __LINE__,
                            pdu_info_p->sn);
                      return -2;
                    }
                  }
                }

                LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SEGMENT OFFSET %05d (vr(mr) < vr(r) and sn >= vr(r))\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      pdu_info_p->sn,
                      pdu_info_p->so);
                list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
                return 0;

              } else if (pdu_info_p->so <= pdu_info_cursor_p->so) {
                LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP SO DUPLICATE -> DROPPED\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      pdu_info_p->sn);
                return -2;
              }
            } else {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            }
          }

          previous_cursor_p = cursor_p;
          cursor_p = cursor_p->next;

          if (cursor_p != NULL) {
            pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
          }
        }

        if (cursor_p != NULL) {
          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and sn >= vr(r))\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                pdu_info_p->sn);
          list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
          return 0;
        } else {
          if (pdu_info_cursor_p->rf == 0) {
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and vr(h) > vr(r) and sn >= vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
            return 0;
          } else if ((pdu_info_p->rf == 1) && (pdu_info_cursor_p->rf == 1) && (pdu_info_p->sn == pdu_info_cursor_p->sn)) {
            if ((pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1) < pdu_info_p->so) {
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and vr(h) > vr(r) and sn >= vr(r))\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
              return 0;
            } else {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            }
          } else if (pdu_info_p->sn != pdu_info_cursor_p->sn) {
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and vr(h) > vr(r) and sn >= vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
            return 0;
          }
        }

        LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              __LINE__,
              pdu_info_p->sn);
        return -2;
      } else { // (pdu_info_p->sn < rlc_pP->vr_r)
        cursor_p = rlc_pP->receiver_buffer.tail;
        pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

        while ((cursor_p != NULL) && (pdu_info_cursor_p->sn < rlc_pP->vr_r)) {
          //pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
          if (pdu_info_p->sn > pdu_info_cursor_p->sn) {
            if (previous_cursor_p != NULL) {
              pdu_info_previous_cursor_p = &((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info;

              if (pdu_info_previous_cursor_p->sn == pdu_info_cursor_p->sn) {
                if (pdu_info_p->rf != pdu_info_previous_cursor_p->rf) {
                  LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d WRONG RF -> DROPPED (vr(mr) < vr(r) and sn >= vr(r))\n",
                        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                        __LINE__, pdu_info_p->sn);
                  return -2;
                } else if (pdu_info_p->rf == 1) {
                  if ((pdu_info_p->so + pdu_info_p->payload_size - 1) >= pdu_info_previous_cursor_p->so) {
                    LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SO OVERLAP -> DROPPED (vr(mr) < vr(r) and sn >= vr(r))\n",
                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                          __LINE__,
                          pdu_info_p->sn);
                    return -2;
                  }
                }
              }
            }

            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            list2_insert_after_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
            return 0;
          } else if (pdu_info_p->sn == pdu_info_cursor_p->sn) {
            if (pdu_info_cursor_p->rf == 0) {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DUPLICATE -> DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            } else if (pdu_info_p->rf == 1) {
              if ((pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1) < pdu_info_p->so) {

                if (previous_cursor_p != NULL) {
                  pdu_info_previous_cursor_p = &((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info;

                  if (pdu_info_previous_cursor_p->sn == pdu_info_cursor_p->sn) {
                    if ((pdu_info_p->so + pdu_info_p->payload_size - 1) < pdu_info_previous_cursor_p->so) {

                      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SEGMENT OFFSET %05d (vr(mr) < vr(r) and sn < vr(r))\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                            __LINE__,
                            pdu_info_p->sn,
                            pdu_info_p->so);
                      list2_insert_after_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
                      return 0;
                    } else {
                      LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP PREVIOUS SO DUPLICATE -> DROPPED\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                            __LINE__,
                            pdu_info_p->sn);
                      return -2;
                    }
                  }
                }

                LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SEGMENT OFFSET %05d (vr(mr) < vr(r) and sn < vr(r))\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      pdu_info_p->sn,
                      pdu_info_p->so);
                list2_insert_after_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
                return 0;

              } else if (pdu_info_cursor_p->so <= pdu_info_p->so) {
                LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP SO DUPLICATE -> DROPPED\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      pdu_info_p->sn);
                return -2;
              }
            } else {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            }
          }

          previous_cursor_p = cursor_p;
          cursor_p = cursor_p->previous;

          if (cursor_p != NULL) {
            pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
          }
        }

        if (cursor_p != NULL) {
          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and sn < vr(r))\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                pdu_info_p->sn);
          list2_insert_after_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
          return 0;
        } else {
          if (pdu_info_cursor_p->rf == 0) {
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and vr(h) > vr(r) and sn >= vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
            return 0;
          } else if ((pdu_info_p->rf == 1) && (pdu_info_cursor_p->rf == 1) && (pdu_info_p->sn == pdu_info_cursor_p->sn)) {
            if ((pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1) < pdu_info_p->so) {
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and vr(h) > vr(r) and sn >= vr(r))\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
              return 0;
            } else {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            }
          } else if (pdu_info_p->sn != pdu_info_cursor_p->sn) {
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) < vr(r) and vr(h) > vr(r) and sn >= vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
            return 0;
          }
        }

        LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              __LINE__,
              pdu_info_p->sn);
        return -2;
      }
    } else { // (pdu_info_p->vr_mr > rlc_pP->vr_r), > and not >=
      // FAR MORE SIMPLE CASE
      while (cursor_p != NULL) {
        //msg ("[FRAME %05u][%s][RLC_AM][MOD %u/%u][RB %u][INSERT PDU] LINE %d cursor_p %p\n", ctxt_pP->frame, rlc_pP->module_id, rlc_pP->rb_id, __LINE__, cursor_p);
        pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

        if (pdu_info_p->sn < pdu_info_cursor_p->sn) {

          if (previous_cursor_p != NULL) {
            pdu_info_previous_cursor_p = &((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info;

            if (pdu_info_previous_cursor_p->sn == pdu_info_p->sn) {
              if (pdu_info_p->rf != pdu_info_previous_cursor_p->rf) {
                LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d WRONG RF -> DROPPED (vr(mr) > vr(r))\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      pdu_info_p->sn);
                return -2;
              } else if (pdu_info_p->rf == 1) {
                if ((pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - 1) >= pdu_info_p->so) {
                  LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SO OVERLAP -> DROPPED (vr(mr) > vr(r))\n",
                        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                        __LINE__,
                        pdu_info_p->sn);
                  return -2;
                }
              }
            }
          }

          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) > vr(r))\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                pdu_info_p->sn);
          list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
          return 0;

        } else if (pdu_info_p->sn == pdu_info_cursor_p->sn) {
          if (pdu_info_cursor_p->rf == 0) {
            LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d WRONG RF -> DROPPED (vr(mr) > vr(r))\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            return -2;
          } else if (pdu_info_p->rf == 1) {

            if ((pdu_info_p->so + pdu_info_p->payload_size - 1) < pdu_info_cursor_p->so) {

              if (previous_cursor_p != NULL) {
                pdu_info_previous_cursor_p = &((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info;

                if (pdu_info_previous_cursor_p->sn == pdu_info_cursor_p->sn) {
                  if ((pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - 1) < pdu_info_p->so) {

                    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SEGMENT OFFSET %05d (vr(mr) > vr(r) and sn >= vr(r))\n",
                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                          __LINE__,
                          pdu_info_p->sn,
                          pdu_info_p->so);
                    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] PREVIOUS SO %d PAYLOAD SIZE %d\n",
                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                          pdu_info_previous_cursor_p->so,
                          pdu_info_previous_cursor_p->payload_size);
                    list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
                    return 0;
                  } else {
                    LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP PREVIOUS SO DUPLICATE -> DROPPED\n",
                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                          __LINE__,
                          pdu_info_p->sn);
                    return -2;
                  }
                }
              }

              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d SEGMENT OFFSET %05d (vr(mr) > vr(r) and sn >= vr(r))\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn,
                    pdu_info_p->so);
              list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
              return 0;
            } else if (pdu_info_p->so <= pdu_info_cursor_p->so) {
              LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP SO DUPLICATE -> DROPPED\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    pdu_info_p->sn);
              return -2;
            }
          } else {
            LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  pdu_info_p->sn);
            return -2;
          }
        }

        previous_cursor_p = cursor_p;
        cursor_p = cursor_p->next;
      }

      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (vr(mr) > vr(r))(last inserted)\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            __LINE__,
            pdu_info_p->sn);

      // pdu_info_cursor_p can not be NULL here
      if  (pdu_info_p->sn == pdu_info_cursor_p->sn) {
        if ((pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1) < pdu_info_p->so) {
          list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
          return 0;
        } else {
          LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d OVERLAP SO DUPLICATE -> DROPPED\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                pdu_info_p->sn);
          return -2;
        }
      } else {
        list2_add_tail(tb_pP, &rlc_pP->receiver_buffer);
        return 0;
      }
    }
  } else {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (only inserted)\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          __LINE__,
          pdu_info_p->sn);
    list2_add_head(tb_pP, &rlc_pP->receiver_buffer);
    return 0;
  }

  LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d DROPPED @4\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        __LINE__,
        pdu_info_p->sn);
  return -1;
}
//-----------------------------------------------------------------------------
void
rlc_am_rx_check_all_byte_segments(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP,
  mem_block_t* const tb_pP)
{
  rlc_am_pdu_info_t  *pdu_info_p        = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
  mem_block_t        *cursor_p        = NULL;
  mem_block_t        *first_cursor_p  = NULL;
  rlc_sn_t            sn              = pdu_info_p->sn;
  sdu_size_t          next_waited_so;
  sdu_size_t          last_end_so;

  //msg("rlc_am_rx_check_all_byte_segments(%d) @0\n",sn);
  if (pdu_info_p->rf == 0) {
    ((rlc_am_rx_pdu_management_t*)(tb_pP->data))->all_segments_received = 1;
    return;
  }

  // for re-segmented AMD PDUS
  cursor_p = tb_pP;
  //list2_init(&list, NULL);
  //list2_add_head(cursor_p, &list);
  //msg("rlc_am_rx_check_all_byte_segments(%d) @1\n",sn);

  // get all previous PDU with same SN
  while (cursor_p->previous != NULL) {
    if (((rlc_am_rx_pdu_management_t*)(cursor_p->previous->data))->pdu_info.sn == sn) {
      //list2_add_head(cursor_p->previous, &list);
      cursor_p = cursor_p->previous;
      //msg("rlc_am_rx_check_all_byte_segments(%d) @2\n",sn);
    } else {
      break;
    }
  }

  // in case all first segments up to tb_pP are in list
  // the so field of the first PDU should be 0
  //cursor_p = list.head;
  pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

  if (pdu_info_p->so != 0) {
    return;
  }

  //msg("rlc_am_rx_check_all_byte_segments(%d) @3\n",sn);
  next_waited_so = pdu_info_p->payload_size;
  first_cursor_p = cursor_p;
  // then check if all segments are contiguous
  last_end_so = pdu_info_p->payload_size;

  while (cursor_p->next != NULL) {
    //msg("rlc_am_rx_check_all_byte_segments(%d) @4\n",sn);
    cursor_p = cursor_p->next;
    pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

    if (pdu_info_p->sn == sn) {
      // extra check normally not necessary
      if (
        !(pdu_info_p->rf == 1) ||
        !(pdu_info_p->so <= last_end_so)
      ) {
        //msg("rlc_am_rx_check_all_byte_segments(%d) @5 pdu_info_p->rf %d pdu_info_p->so %d\n",sn, pdu_info_p->rf, pdu_info_p->so);
        return;
      } else {
        if (pdu_info_p->so == next_waited_so) {
          next_waited_so = next_waited_so + pdu_info_p->payload_size;
          //msg("rlc_am_rx_check_all_byte_segments(%d) @6\n",sn);
        } else { // assumed pdu_info_p->so + pdu_info_p->payload_size > next_waited_so
          next_waited_so = (next_waited_so + pdu_info_p->payload_size) - (next_waited_so - pdu_info_p->so);
          //msg("rlc_am_rx_check_all_byte_segments(%d) @7\n",sn);
        }

        if (pdu_info_p->lsf > 0) {
          //msg("rlc_am_rx_check_all_byte_segments(%d) @8\n",sn);
          rlc_am_rx_mark_all_segments_received(ctxt_pP, rlc_pP,  first_cursor_p);
        }
      }

      last_end_so = pdu_info_p->so + pdu_info_p->payload_size;
    } else {
      //msg("rlc_am_rx_check_all_byte_segments(%d) @9\n",sn);
      return;
    }
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_rx_mark_all_segments_received(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const        rlc_pP,
  mem_block_t* const            fisrt_segment_tbP)
{
  rlc_am_pdu_info_t* pdu_info_p          = &((rlc_am_rx_pdu_management_t*)(fisrt_segment_tbP->data))->pdu_info;
  rlc_am_pdu_info_t* pdu_info_cursor_p = NULL;
  mem_block_t*       cursor_p          = NULL;
  rlc_sn_t           sn                = pdu_info_p->sn;

  cursor_p = fisrt_segment_tbP;

  if (cursor_p) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU] ALL SEGMENTS RECEIVED SN %04d:\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          sn);

    do {
      pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

      if (pdu_info_cursor_p->sn == sn) {
        ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received = 1;
      } else {
        return;
      }

      cursor_p = cursor_p->next;
    } while (cursor_p != NULL);
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_rx_list_reassemble_rlc_sdus(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const        rlc_pP)
{
  mem_block_t*                cursor_p                     = NULL;
  rlc_am_rx_pdu_management_t* rlc_am_rx_old_pdu_management = NULL;

  cursor_p = list2_get_head(&rlc_pP->receiver_buffer);

  if (cursor_p == NULL) {
    return;
  }

  rlc_am_rx_pdu_management_t* rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));

  do {
    if (rlc_am_rx_pdu_management_p->all_segments_received > 0) {
      cursor_p = list2_remove_head(&rlc_pP->receiver_buffer);
      rlc_am_reassemble_pdu(ctxt_pP, rlc_pP, cursor_p);
      rlc_am_rx_old_pdu_management = rlc_am_rx_pdu_management_p;
      cursor_p = list2_get_head(&rlc_pP->receiver_buffer);

      if (cursor_p == NULL) {
        return;
      }

      rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));
    } else {
#if RLC_STOP_ON_LOST_PDU

      if (list2_get_head(&rlc_pP->receiver_buffer) != cursor_p) {
        AssertFatal( 0 == 1,
                     PROTOCOL_RLC_AM_CTXT_FMT" LOST PDU DETECTED\n",
                     PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
      }

#endif
      return;
    }

  } while ((rlc_am_rx_pdu_management_p->pdu_info.sn == ((rlc_am_rx_old_pdu_management->pdu_info.sn + 1) & RLC_AM_SN_MASK))
           || ((rlc_am_rx_pdu_management_p->pdu_info.sn == rlc_am_rx_old_pdu_management->pdu_info.sn) && (rlc_am_rx_pdu_management_p->all_segments_received > 0)));
}
//-----------------------------------------------------------------------------
mem_block_t *
list2_insert_before_element (
  mem_block_t * element_to_insert_pP,
  mem_block_t * element_pP,
  list2_t * list_pP)
{
  if ((element_to_insert_pP != NULL) && (element_pP != NULL)) {
    list_pP->nb_elements = list_pP->nb_elements + 1;
    mem_block_t *previous = element_pP->previous;
    element_to_insert_pP->previous = previous;
    element_to_insert_pP->next     = element_pP;
    element_pP->previous           = element_to_insert_pP;

    if (previous != NULL) {
      previous->next = element_to_insert_pP;
    } else if (list_pP->head == element_pP) {
      list_pP->head = element_to_insert_pP;
    }

    return element_to_insert_pP;
  } else {
    assert(2==1);
    return NULL;
  }
}
//-----------------------------------------------------------------------------
mem_block_t *
list2_insert_after_element (
  mem_block_t * element_to_insert_pP,
  mem_block_t * element_pP,
  list2_t * list_pP)
{

  if ((element_to_insert_pP != NULL) && (element_pP != NULL)) {
    list_pP->nb_elements = list_pP->nb_elements + 1;
    mem_block_t *next = element_pP->next;
    element_to_insert_pP->previous = element_pP;
    element_to_insert_pP->next     = next;
    element_pP->next               = element_to_insert_pP;

    if (next != NULL) {
      next->previous = element_to_insert_pP;
    } else if (list_pP->tail == element_pP) {
      list_pP->tail = element_to_insert_pP;
    }

    return element_to_insert_pP;
  } else {
    assert(2==1);
    return NULL;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_rx_list_display (
  const rlc_am_entity_t* const rlc_pP,
  char* message_pP)
{
  mem_block_t      *cursor_p = NULL;
  unsigned int      loop     = 0;

  cursor_p = rlc_pP->receiver_buffer.head;

  if (message_pP) {
    LOG_T(RLC, "Display list %s %s VR(R)=%04d:\n", rlc_pP->receiver_buffer.name, message_pP, rlc_pP->vr_r);
  } else {
    LOG_T(RLC, "Display list %s VR(R)=%04d:\n", rlc_pP->receiver_buffer.name, rlc_pP->vr_r);
  }

  if (cursor_p) {
    // almost one element
    while (cursor_p != NULL) {
      //if (((loop % 16) == 0) && (loop > 0)) {
      if ((loop % 4) == 0) {
        LOG_T(RLC, "\nRX SN:\t");
      }

      if (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.rf) {
        if (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.lsf) {
          LOG_T(RLC, "%04d (%04d->%04d LSF)\t",
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so + ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.payload_size - 1);
        } else {
          LOG_T(RLC, "%04d (%04d->%04d)\t",
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so + ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.payload_size - 1);
        }
      } else {
        LOG_T(RLC, "%04d (%04d NOSEG)\t",
              ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn,
              ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.payload_size);
      }

      //if (cursor_p == cursor_p->next) {
      //    rlc_am_v9_3_0_test_print_trace();
      //}
      assert(cursor_p != cursor_p->next);
      cursor_p = cursor_p->next;
      loop++;
    }

    LOG_T(RLC, "\n");
  } else {
    LOG_T(RLC, "\nNO ELEMENTS\n");
  }
}
