
#include <stdint.h>
#include "PHY/defs.h"

#define IF5_MOBIPASS 0x0050

struct IF5_mobipass_header {  
  /// Type
  uint16_t flags; 
  /// Sub-Type
  uint16_t fifo_status;
  /// Reserved
  uint8_t seqno;
  
  uint8_t ack;

  uint32_t rsvd;
  /// Frame Status
  uint32_t time_stamp;

} __attribute__ ((__packed__));

typedef struct IF5_mobipass_header IF5_mobipass_header_t;
#define sizeof_IF5_mobipass_header_t 14

uint8_t send_IF5(PHY_VARS_eNB*, eNB_rxtx_proc_t*, uint8_t);
