/*! \file rlc_tm_structs.h
* \brief This file defines structures used inside the RLC TM.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
* \note
* \bug
* \warning
*/
#    ifndef __RLC_TM_STRUCTS_H__
#        define __RLC_TM_STRUCTS_H__
#        include "platform_types.h"
#        include "list.h"
#        include "mac_primitives.h"
#        include "rlc_primitives.h"
#        include "mac_rlc_primitives.h"
//-----------------------
struct rlc_tm_tx_sdu_management {
  uint8_t             *first_byte;
  int32_t             sdu_creation_time;
  uint16_t             sdu_size;
};
//-----------------------
struct rlc_tm_tx_pdu_management {
  uint8_t             *first_byte;
  uint8_t              dummy[MAC_HEADER_MAX_SIZE];
};
//-----------------------
struct rlc_tm_rx_pdu_management {
  uint8_t             *first_byte;
};
//-----------------------
struct rlc_tm_tx_data_pdu_struct {
  union {
    struct rlc_tm_tx_pdu_management tx_pdu_mngmnt;
    struct mac_tb_req tb_req;
    struct mac_tx_tb_management tb_mngt;
#        ifdef BYPASS_L1
    struct rlc_tm_rx_pdu_management dummy1;
    struct mac_tb_ind dummy2;
    struct mac_rx_tb_management dummy3;
    struct rlc_indication dummy4;
#        endif
  } dummy;
  uint8_t              data[1];
};
//-----------------------
struct rlc_tm_data_req_alloc {  // alloc enought bytes for sdu mngt also
  union {
    struct rlc_tm_data_req dummy1;
    struct rlc_tm_tx_sdu_management dummy2;
  } dummy;
};
#    endif
