/*! \file PHY/LTE_TRANSPORT/if4_tools.h
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#include "PHY/defs.h"

/// Macro for IF4 packet type
#define IF4p5_PACKET_TYPE 0x080A 
#define IF4p5_PULFFT 0x0019 
#define IF4p5_PDLFFT 0x0020
#define IF4p5_PRACH 0x0021

struct IF4p5_header {  
  /// Type
  uint16_t type; 
  /// Sub-Type
  uint16_t sub_type;
  /// Reserved
  uint32_t rsvd;
  /// Frame Status
  uint32_t frame_status;

} __attribute__ ((__packed__));

typedef struct IF4p5_header IF4p5_header_t;
#define sizeof_IF4p5_header_t 12 

void gen_IF4p5_dl_header(IF4p5_header_t*, int, int);

void gen_IF4p5_ul_header(IF4p5_header_t*, int, int);

void gen_IF4p5_prach_header(IF4p5_header_t*, int, int);

void send_IF4p5(PHY_VARS_eNB*, int, int, uint16_t, int);

void recv_IF4p5(PHY_VARS_eNB*, int*, int*, uint16_t*, uint32_t*);

void malloc_IF4p5_buffer(PHY_VARS_eNB*);
