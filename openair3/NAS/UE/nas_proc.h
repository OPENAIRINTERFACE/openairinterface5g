/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*****************************************************************************
Source      nas_proc.h

Version     0.1

Date        2012/09/20

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure call manager

*****************************************************************************/
#ifndef __NAS_PROC_H__
#define __NAS_PROC_H__

#include "commonDef.h"
#include "networkDef.h"

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

void nas_proc_initialize(emm_indication_callback_t emm_cb,
                         esm_indication_callback_t esm_cb, const char *imei);

void nas_proc_cleanup(void);

/*
 * --------------------------------------------------------------------------
 *          NAS procedures triggered by the user
 * --------------------------------------------------------------------------
 */

int nas_proc_enable_s1_mode(void);
int nas_proc_disable_s1_mode(void);
int nas_proc_get_eps(int *stat);

int nas_proc_get_imsi(char *imsi_str);
int nas_proc_get_msisdn(char *msisdn_str, int *ton_npi);

int nas_proc_get_signal_quality(int *rsrq, int *rsrp);

int nas_proc_register(int mode, int format, const network_plmn_t *oper, int AcT);
int nas_proc_deregister(void);
int nas_proc_get_reg_data(int *mode, int *selected, int format,
                          network_plmn_t *oper, int *AcT);
int nas_proc_get_oper_list(const char **oper_list);

int nas_proc_get_reg_status(int *stat);
int nas_proc_get_loc_info(char *tac, char *ci, int *AcT);

int nas_proc_detach(int switch_off);
int nas_proc_attach(void);
int nas_proc_get_attach_status(void);

int nas_proc_reset_pdn(int cid);
int nas_proc_set_pdn(int cid, int type, const char *apn, int ipv4_addr,
                     int emergency, int p_cscf, int im_cn_signal);
int nas_proc_get_pdn_range(void);
int nas_proc_get_pdn_status(int *cids, int *states, int n_pdn_max);
int nas_proc_get_pdn_param(int *cids, int *types, const char **apns,
                           int n_pdn_max);
int nas_proc_get_pdn_addr(int cid, int *cids, const char **addr1,
                          const char **addr2, int n_addr_max);
int nas_proc_deactivate_pdn(int cid);
int nas_proc_activate_pdn(int cid);

/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the network
 * --------------------------------------------------------------------------
 */

int nas_proc_cell_info(int found, tac_t tac, ci_t ci, AcT_t rat, uint8_t rsrp,
                       uint8_t rsrq);

int nas_proc_establish_cnf(const Byte_t *data, uint32_t len);
int nas_proc_establish_rej(void);

int nas_proc_release_ind(int cause);

int nas_proc_ul_transfer_cnf(void);
int nas_proc_ul_transfer_rej(void);
int nas_proc_dl_transfer_ind(const Byte_t *data, uint32_t len);



#endif /* __NAS_PROC_H__*/
