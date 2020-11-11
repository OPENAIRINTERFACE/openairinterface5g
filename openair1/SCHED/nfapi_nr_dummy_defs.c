//Dummy NR defs to avoid linking errors

#include "PHY/defs_gNB.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface_scf.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "openair1/PHY/LTE_TRANSPORT/transport_common.h"

void handle_nfapi_nr_pdcch_pdu(PHY_VARS_gNB *gNB,
			       int frame, int slot,
			       nfapi_nr_dl_tti_pdcch_pdu *pdcch_pdu){}

void handle_nr_nfapi_ssb_pdu(PHY_VARS_gNB *gNB,int frame,int slot,
                             nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdu){}

int16_t find_nr_dlsch(uint16_t rnti, PHY_VARS_gNB *gNB,find_type_t type){}
void handle_nr_nfapi_pdsch_pdu(PHY_VARS_gNB *gNB,int frame,int slot,
                            nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
                            uint8_t *sdu){
                            }
int l1_north_init_gNB(void){}

uint8_t slot_ahead=0;
uint8_t nfapi_mode=0;
NR_IF_Module_t *NR_IF_Module_init(int Mod_id) {}

void handle_nfapi_nr_ul_dci_pdu(PHY_VARS_gNB *gNB,
			       int frame, int slot,
			       nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu){}
                   
void  nr_phy_config_request(NR_PHY_Config_t *gNB){}

void install_nr_schedule_handlers(NR_IF_Module_t *if_inst){}

//void nr_dump_frame_parms(NR_DL_FRAME_PARMS *fp){}

                   