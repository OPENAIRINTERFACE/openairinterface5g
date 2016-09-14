/*****************************************************************************
Source      esm_ebr.h

Version     0.1

Date        2013/01/29

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle state of EPS bearer contexts
        and manage ESM messages re-transmission.

*****************************************************************************/
#ifndef __ESM_EBR_H__
#define __ESM_EBR_H__

#include "OctetString.h"

#include "networkDef.h"

#include "nas_timer.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Unassigned EPS bearer identity value */
#define ESM_EBI_UNASSIGNED  (EPS_BEARER_IDENTITY_UNASSIGNED)

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int esm_ebr_is_reserved(int ebi);

void esm_ebr_initialize(esm_indication_callback_t cb);
int esm_ebr_assign(int ebi, int cid, int default_ebr);
int esm_ebr_release(int ebi);

int esm_ebr_set_status(int ebi, esm_ebr_state status, int ue_requested);
esm_ebr_state esm_ebr_get_status(int ebi);

int esm_ebr_is_not_in_use(int ebi);

#endif /* __ESM_EBR_H__*/
