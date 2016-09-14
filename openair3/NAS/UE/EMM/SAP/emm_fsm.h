/*****************************************************************************

Source      emm_fsm.h

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EPS Mobility Management procedures executed at
        the EMMREG Service Access Point.

*****************************************************************************/
#ifndef __EMM_FSM_H__
#define __EMM_FSM_H__

#include "emm_regDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * States of the EPS Mobility Management sublayer
 * ----------------------------------------------
 * The EMM protocol of the UE and the network is described by means of two
 * different state machines.
 */
typedef enum {
  EMM_INVALID,
  EMM_NULL,
  EMM_DEREGISTERED,
  EMM_REGISTERED,
  EMM_DEREGISTERED_INITIATED,
  EMM_DEREGISTERED_NORMAL_SERVICE,
  EMM_DEREGISTERED_LIMITED_SERVICE,
  EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH,
  EMM_DEREGISTERED_PLMN_SEARCH,
  EMM_DEREGISTERED_NO_IMSI,
  EMM_DEREGISTERED_ATTACH_NEEDED,
  EMM_DEREGISTERED_NO_CELL_AVAILABLE,
  EMM_REGISTERED_INITIATED,
  EMM_REGISTERED_NORMAL_SERVICE,
  EMM_REGISTERED_ATTEMPTING_TO_UPDATE,
  EMM_REGISTERED_LIMITED_SERVICE,
  EMM_REGISTERED_PLMN_SEARCH,
  EMM_REGISTERED_UPDATE_NEEDED,
  EMM_REGISTERED_NO_CELL_AVAILABLE,
  EMM_REGISTERED_ATTEMPTING_TO_UPDATE_MM,
  EMM_REGISTERED_IMSI_DETACH_INITIATED,
  EMM_TRACKING_AREA_UPDATING_INITIATED,
  EMM_SERVICE_REQUEST_INITIATED,
  EMM_STATE_MAX
} emm_fsm_state_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void emm_fsm_initialize(void);

int emm_fsm_set_status(emm_fsm_state_t status);
emm_fsm_state_t emm_fsm_get_status(void);


int emm_fsm_process(const emm_reg_t *evt);

#endif /* __EMM_FSM_H__*/
