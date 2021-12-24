/*
 * phy_stub_UE.h
 *
 *  Created on: Sep 14, 2017
 *      Author: montre
 */


#ifndef __PHY_STUB_UE__H__
#define __PHY_STUB_UE__H__

#include <stdint.h>
#include "openair2/PHY_INTERFACE/IF_Module.h"
#include "nfapi_interface.h"
#include "nfapi_pnf_interface.h"
//#include "openair1/PHY/LTE_TRANSPORT/defs.h"
//#include "openair1/PHY/defs.h"
//#include "openair1/PHY/LTE_TRANSPORT/defs.h"

//below 2 difinitions move to phy_stub_UE.c to add initialization when difinition.
extern UL_IND_t *UL_INFO;
extern nfapi_tx_request_pdu_t* tx_request_pdu_list;
// New
/// Pointers to config_request types. Used from nfapi callback functions.
//below 3 difinitions move to phy_stub_UE.c to add initialization when difinition.
extern nfapi_dl_config_request_t* dl_config_req;
extern nfapi_ul_config_request_t* ul_config_req;
extern nfapi_hi_dci0_request_t* hi_dci0_req;
extern int	tx_req_num_elems;

// This function should return all the sched_response config messages which concern a specific UE. Inside this
// function we should somehow make the translation of config message's rnti to Mod_ID.
Sched_Rsp_t get_nfapi_sched_response(uint8_t Mod_id);

// This function will be processing DL_config and Tx.requests and trigger all the MAC Rx related calls at the UE side,
// namely:ue_send_sdu(), or ue_decode_si(), or ue_decode_p(), or ue_process_rar() based on the rnti type.
//void handle_nfapi_UE_Rx(uint8_t Mod_id, Sched_Rsp_t *Sched_INFO, int eNB_id);

int pnf_ul_config_req_UE_MAC(nfapi_pnf_p7_config_t* pnf_p7, nfapi_ul_config_request_t* req);

// This function will be processing UL and HI_DCI0 config requests to trigger all the MAC Tx related calls
// at the UE side, namely: ue_get_SR(), ue_get_rach(), ue_get_sdu() based on the pdu configuration type.
// The output of these calls will be put to an UL_IND_t structure which will then be the input to
// send_nfapi_UL_indications().
UL_IND_t generate_nfapi_UL_indications(Sched_Rsp_t sched_response);

// This function should pass the UL indication messages to the eNB side through the socket interface.
void send_nfapi_UL_indications(UL_IND_t UL_INFO);

// This function should be filling the nfapi ULSCH indications at the MAC level of the UE in a similar manner
// as fill_rx_indication() does. It should get called from ue_get_SDU()

//void fill_rx_indication_UE_MAC(module_id_t Mod_id,int frame,int subframe);

void fill_rx_indication_UE_MAC(module_id_t Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t *ulsch_buffer, uint16_t buflen, uint16_t rnti, int index);


// This function should be indicating directly to the eNB when there is a planned scheduling request at the MAC layer
// of the UE. It should get called from ue_get_SR()
void fill_sr_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint16_t rnti);

// In our case the this function will be always indicating ACK to the MAC of the eNB (i.e. always assuming)
// successful decoding.
void fill_crc_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t crc_flag, int index, uint16_t rnti);


void fill_rach_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t ra_PreambleIndex, uint16_t ra_RNTI);


void fill_ulsch_cqi_indication_UE_MAC(int Mod_id, uint16_t frame,uint8_t subframe, UL_IND_t *UL_INFO, uint16_t rnti);


void fill_ulsch_harq_indication_UE_MAC(int Mod_id, int frame,int subframe, UL_IND_t *UL_INFO, nfapi_ul_config_ulsch_harq_information *harq_information, uint16_t rnti);

void fill_uci_harq_indication_UE_MAC(int Mod_id, int frame, int subframe, UL_IND_t *UL_INFO,nfapi_ul_config_harq_information *harq_information, uint16_t rnti
			      /*uint8_t tdd_mapping_mode,
			      uint16_t tdd_multiplexing_mask*/);

int ul_config_req_UE_MAC(nfapi_ul_config_request_t* req, int frame, int subframe, module_id_t Mod_id);

void handle_nfapi_ul_pdu_UE_MAC(module_id_t Mod_id,
                         nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame,uint8_t subframe,uint8_t srs_present, int index);

void dl_config_req_UE_MAC_dci(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *dci,
                              nfapi_dl_config_request_pdu_t *dlsch,
                              int num_ue);
void dl_config_req_UE_MAC_bch(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *bch,
                              int num_ue);
void dl_config_req_UE_MAC_mch(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *bch,
                              int num_ue);

int tx_req_UE_MAC(nfapi_tx_request_t* req);


void hi_dci0_req_UE_MAC(int sfn,
                        int sf,
                        nfapi_hi_dci0_request_pdu_t* bch,
                        int num_ue);

// The following set of memcpy functions should be getting called as callback functions from
// pnf_p7_subframe_ind.

int memcpy_dl_config_req (L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t* pnf_p7, nfapi_dl_config_request_t* req);


int memcpy_ul_config_req (L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t* pnf_p7, nfapi_ul_config_request_t* req);


int memcpy_tx_req (nfapi_pnf_p7_config_t* pnf_p7, nfapi_tx_request_t* req);


int memcpy_hi_dci0_req (L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t* pnf_p7, nfapi_hi_dci0_request_t* req);

void UE_config_stub_pnf(void);



#endif /* PHY_STUB_UE_H_ */
