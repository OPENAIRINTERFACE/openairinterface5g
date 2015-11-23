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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source      esm_ebr_context.h

Version     0.1

Date        2013/05/28

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle EPS bearer contexts.

*****************************************************************************/
#ifndef __ESM_EBR_CONTEXT_H__
#define __ESM_EBR_CONTEXT_H__

#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* Traffic flow template operation */
typedef enum {
  ESM_EBR_CONTEXT_TFT_CREATE,
  ESM_EBR_CONTEXT_TFT_DELETE,
  ESM_EBR_CONTEXT_TFT_ADD_PACKET,
  ESM_EBR_CONTEXT_TFT_REPLACE_PACKET,
  ESM_EBR_CONTEXT_TFT_DELETE_PACKET,
} esm_ebr_context_tft_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int esm_ebr_context_create(int pid, int ebi, int is_default,
                           const network_qos_t *qos, const network_tft_t *tft);

int esm_ebr_context_release(int ebi, int *pid, int *bid);

int esm_ebr_context_get_pid(int ebi);

int esm_ebr_context_check_tft(int pid, int ebi, const network_tft_t *tft,
                              esm_ebr_context_tft_t operation);


#endif /* __ESM_EBR_CONTEXT_H__ */
