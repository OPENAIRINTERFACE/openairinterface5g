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
#include <pthread.h>
#include <semaphore.h>
#include "nfapi/oai_integration/vendor_ext.h"
//#include "openair1/PHY/LTE_TRANSPORT/defs.h"
//#include "openair1/PHY/defs.h"
//#include "openair1/PHY/LTE_TRANSPORT/defs.h"
#include "queue_t.h"

#define NUM_MCS 28
#define NUM_SINR 100
#define NUM_BLER_COL 13

// this mutex is used to set multiple UE's UL value in L2 FAPI simulator.
extern FILL_UL_INFO_MUTEX_t fill_ul_mutex;
//below 2 difinitions move to phy_stub_UE.c to add initialization when difinition.
extern UL_IND_t *UL_INFO;
// New
/// Pointers to config_request types. Used from nfapi callback functions.
//below 3 difinitions move to phy_stub_UE.c to add initialization when difinition.

//below 2 difinitions move to lte-ue.c to add initialization when difinition.
//int next_ra_frame;
//module_id_t next_Mod_id;

typedef struct
{
    uint8_t sf;
    uint16_t rnti[256];
    uint8_t mcs[256];
    float sinr;
    uint16_t pdu_size;
    bool drop_flag[256];
    bool latest;

} sf_rnti_mcs_s;

typedef struct
{
    uint16_t length;
    float bler_table[NUM_SINR][NUM_BLER_COL];
} bler_struct;

extern bler_struct bler_data[NUM_MCS];


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

void fill_rx_indication_UE_MAC(module_id_t Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t *ulsch_buffer, uint16_t buflen, uint16_t rnti, int index,
                            nfapi_ul_config_request_t *ul_config_req);


// This function should be indicating directly to the eNB when there is a planned scheduling request at the MAC layer
// of the UE. It should get called from ue_get_SR()
void fill_sr_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint16_t rnti,
                            nfapi_ul_config_request_t *ul_config_req);

// In our case the this function will be always indicating ACK to the MAC of the eNB (i.e. always assuming)
// successful decoding.
void fill_crc_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t crc_flag, int index, uint16_t rnti,
                                nfapi_ul_config_request_t *ul_config_req);


void fill_rach_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t ra_PreambleIndex, uint16_t ra_RNTI);


void fill_ulsch_cqi_indication_UE_MAC(int Mod_id, uint16_t frame,uint8_t subframe, UL_IND_t *UL_INFO, uint16_t rnti);


void fill_ulsch_harq_indication_UE_MAC(int Mod_id, int frame,int subframe, UL_IND_t *UL_INFO, nfapi_ul_config_ulsch_harq_information *harq_information, uint16_t rnti,
                                    nfapi_ul_config_request_t *ul_config_req);

void fill_uci_harq_indication_UE_MAC(int Mod_id, int frame, int subframe, UL_IND_t *UL_INFO,nfapi_ul_config_harq_information *harq_information, uint16_t rnti,
                                    nfapi_ul_config_request_t *ul_config_req
			      /*uint8_t tdd_mapping_mode,
			      uint16_t tdd_multiplexing_mask*/);

int ul_config_req_UE_MAC(nfapi_ul_config_request_t* req, int frame, int subframe, module_id_t Mod_id);

void handle_nfapi_ul_pdu_UE_MAC(module_id_t Mod_id,
                         nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame,uint8_t subframe,uint8_t srs_present, int index,
                         nfapi_ul_config_request_t *ul_config_req);

typedef struct phy_channel_params_t
{
    uint16_t sfn_sf;
    float sinr;
    // Incomplete, need all channel parameters
} phy_channel_params_t;

typedef struct nfapi_tx_req_pdu_list_t
{
    int num_pdus;                  /* number .pdus[] objects */
    nfapi_tx_request_pdu_t pdus[]; /* see "struct hack" */
} nfapi_tx_req_pdu_list_t;

typedef struct nfapi_dl_config_req_tx_req_t
{
    nfapi_dl_config_request_t *dl_config_req;
    nfapi_tx_req_pdu_list_t *tx_req_pdu_list;
} nfapi_dl_config_req_tx_req_t;

void nfapi_free_tx_req_pdu_list(nfapi_tx_req_pdu_list_t *);

void dl_config_req_UE_MAC_dci(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *dci,
                              nfapi_dl_config_request_pdu_t *dlsch,
                              int num_ue,
                              nfapi_tx_req_pdu_list_t *);
void dl_config_req_UE_MAC_bch(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *bch,
                              int num_ue);
void dl_config_req_UE_MAC_mch(int sfn,
                              int sf,
                              nfapi_dl_config_request_pdu_t *bch,
                              int num_ue,
                              nfapi_tx_req_pdu_list_t *);

int tx_req_UE_MAC(nfapi_tx_request_t* req);


void hi_dci0_req_UE_MAC(int sfn,
                        int sf,
                        nfapi_hi_dci0_request_pdu_t* bch,
                        int num_ue);

// The following set of memcpy functions should be getting called as callback functions from
// pnf_p7_subframe_ind.

int memcpy_ul_config_req (L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t* pnf_p7, nfapi_ul_config_request_t* req);

int memcpy_hi_dci0_req (L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t* pnf_p7, nfapi_hi_dci0_request_t* req);

void UE_config_stub_pnf(void);

void *memcpy_tx_req_standalone(nfapi_tx_request_t *tx_req);

void *memcpy_dl_config_req_standalone(nfapi_dl_config_request_t *dl_config_req);

// open an SCTP socket with a standalone PNF module
void ue_init_standalone_socket(int tx_port, int rx_port);

// read from standalone pnf socket call corresponding memcpy functions
void *ue_standalone_pnf_task(void *context);
void send_standalone_msg(UL_IND_t *UL, nfapi_message_id_e msg_type);
void send_standalone_dummy(void);
void enqueue_dl_config_req_tx_req(nfapi_dl_config_request_t *dl_config_req, nfapi_tx_request_t *tx_req);

// Convert downlink nfapi messages to a string.
// Returned memory is malloc'ed, caller is responsible for freeing.
char *nfapi_dl_config_req_to_string(nfapi_dl_config_request_t *req);
char *nfapi_ul_config_req_to_string(nfapi_ul_config_request_t *req);

// Convert downlink nfapi messages to a string.
// Returned memory is statically allocated.
const char *dl_pdu_type_to_string(uint8_t pdu_type);
const char *ul_pdu_type_to_string(uint8_t pdu_type);

extern queue_t dl_config_req_tx_req_queue;
extern queue_t ul_config_req_queue;
extern queue_t hi_dci0_req_queue;

extern nfapi_ul_config_request_t* ul_config_req;
extern nfapi_hi_dci0_request_t* hi_dci0_req;

extern int current_sfn_sf;

extern sem_t sfn_semaphore;

#endif /* PHY_STUB_UE_H_ */
