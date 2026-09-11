/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

//Dummy NR defs to avoid linking errors

#include "PHY/defs_gNB.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface_scf.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "openair1/PHY/LTE_TRANSPORT/transport_common.h"
#include "vnf_lte.h"
#include "nfapi/open-nFAPI/pnf/inc/pnf.h"
int l1_north_init_gNB(void){return 0;}

int slot_ahead = 6;
//uint8_t nfapi_mode=0;
NR_IF_Module_t *NR_IF_Module_init(int Mod_id) {return NULL;}

void  nr_phy_config_request(NR_PHY_Config_t *gNB){}

void nr_dump_frame_parms(NR_DL_FRAME_PARMS *fp){}

bool vnf_nr_send_p5_msg(vnf_t *vnf, uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t *msg, uint32_t msg_len)
{
  return false;
}

bool vnf_nr_send_p7_msg(vnf_p7_t *vnf_p7, nfapi_nr_p7_message_header_t *header)
{
  return false;
}

void *vnf_nr_start_p7_thread(void *ptr)
{
  return 0;
}

bool pnf_nr_send_p5_message(pnf_t *pnf, nfapi_nr_p4_p5_message_header_t *msg, uint32_t msg_len)
{
  return false;
}

bool pnf_nr_send_p7_message(pnf_p7_t *pnf_p7, nfapi_nr_p7_message_header_t *header, uint32_t msg_len)
{
  return false;
}

void *pnf_start_p5_thread(void *ptr)
{
  return 0;
}

void vnf_start_p5_thread(void *ptr)
{
}

void *pnf_nr_p7_thread_start(void *ptr)
{
  return 0;
}
void socket_nfapi_nr_pnf_stop()
{
}

void socket_nfapi_send_stop_request(vnf_t *vnf)
{
}

typedef struct gNB_MAC_INST_s gNB_MAC_INST;
typedef struct nr_cell_sched_s nr_cell_sched_t;
nr_cell_sched_t *nr_mac_get_cell_by_phy_id(gNB_MAC_INST *mac, uint16_t phy_id)
{
  return NULL;
}
