/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_process.h

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem AS message processing

Author    Frederic Maurel

Description Defines functions executed by the Access-Stratum sublayer
    upon receiving AS messages from the Non-Access-Stratum.

*****************************************************************************/

#ifndef __AS_PROCESS_H__
#define __AS_PROCESS_H__

#include "as_message.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *  Functions used to process messages received from the UE NAS process
 * -----------------------------------------------------------------------------
 */

int process_cell_info_req(int msg_id, const cell_info_req_t* req,
                          cell_info_cnf_t* cnf);
int process_nas_establish_req(int msg_id, const nas_establish_req_t* req,
                              nas_establish_ind_t* ind, nas_establish_cnf_t* cnf);
int process_ul_info_transfer_req(int msg_id, const ul_info_transfer_req_t* req,
                                 ul_info_transfer_ind_t* ind, ul_info_transfer_cnf_t* cnf);
int process_nas_release_req(int msg_id, const nas_release_req_t* req);

/*
 * -----------------------------------------------------------------------------
 *  Functions used to process messages received from the MME NAS process
 * -----------------------------------------------------------------------------
 */

int process_nas_establish_rsp(int msg_id, const nas_establish_rsp_t* rsp,
                              nas_establish_cnf_t* cnf);
int process_dl_info_transfer_req(int msg_id, const dl_info_transfer_req_t* req,
                                 dl_info_transfer_ind_t* ind, dl_info_transfer_cnf_t* cnf);
int process_nas_release_ind(int msg_id, const nas_release_req_t* req,
                            nas_release_ind_t* ind);

#endif /* __AS_PROCESS_H__*/
