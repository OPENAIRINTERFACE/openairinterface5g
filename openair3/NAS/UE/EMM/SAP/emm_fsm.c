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

Source      emm_fsm.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EPS Mobility Management procedures executed at
        the EMMREG Service Access Point.

*****************************************************************************/

#include "emm_fsm.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emmData.h"



/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

#define EMM_FSM_NB_UE_MAX   1

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *          Data used for trace logging
 * -----------------------------------------------------------------------------
 */

/* String representation of EMM events */
static const char *_emm_fsm_event_str[] = {
  "S1_ENABLED",
  "S1_DISABLED",
  "NO_IMSI",
  "NO_CELL",
  "REGISTER_REQ",
  "REGISTER_CNF",
  "REGISTER_REJ",
  "ATTACH_INIT",
  "ATTACH_REQ",
  "ATTACH_FAILED",
  "ATTACH_EXCEEDED",
  "AUTHENTICATION_REJ",
  "ATTACH_CNF",
  "ATTACH_REJ",
  "DETACH_INIT",
  "DETACH_REQ",
  "DETACH_FAILED",
  "DETACH_CNF",
  "TAU_REQ",
  "TAU_CNF",
  "TAU_REJ",
  "SERVICE_REQ",
  "SERVICE_CNF",
  "SERVICE_REJ",
  "LOWERLAYER_SUCCESS",
  "LOWERLAYER_FAILURE",
  "LOWERLAYER_RELEASE",
};

/* String representation of EMM status */
static const char *_emm_fsm_status_str[EMM_STATE_MAX] = {
  "INVALID",
  "NULL",
  "DEREGISTERED",
  "REGISTERED",
  "DEREGISTERED-INITIATED",
  "DEREGISTERED.NORMAL-SERVICE",
  "DEREGISTERED.LIMITED-SERVICE",
  "DEREGISTERED.ATTEMPTING-TO-ATTACH",
  "DEREGISTERED.PLMN-SEARCH",
  "DEREGISTERED.NO-IMSI",
  "DEREGISTERED.ATTACH-NEEDED",
  "DEREGISTERED.NO-CELL-AVAILABLE",
  "REGISTERED-INITIATED",
  "REGISTERED.NORMAL-SERVICE",
  "REGISTERED.ATTEMPTING-TO-PDATE",
  "REGISTERED.LIMITED-SERVICE",
  "REGISTERED.PLMN-SEARCH",
  "REGISTERED.UPDATE-NEEDED",
  "REGISTERED.NO-CELL-AVAILABLE",
  "REGISTERED.ATTEMPTING-TO-UPDATE-MM",
  "REGISTERED.IMSI-DETACH-INITIATED",
  "TRACKING-AREA-UPDATING-INITIATED",
  "SERVICE-REQUEST-INITIATED",
};

/*
 * -----------------------------------------------------------------------------
 *      EPS Mobility Management state machine handlers
 * -----------------------------------------------------------------------------
 */

/* Type of the EPS Mobility Management state machine handler */
typedef int(*emm_fsm_handler_t)(const emm_reg_t *);

int EmmNull(const emm_reg_t *);
int EmmDeregistered(const emm_reg_t *);
int EmmRegistered(const emm_reg_t *);
int EmmDeregisteredInitiated(const emm_reg_t *);
int EmmDeregisteredNormalService(const emm_reg_t *);
int EmmDeregisteredLimitedService(const emm_reg_t *);
int EmmDeregisteredAttemptingToAttach(const emm_reg_t *);
int EmmDeregisteredPlmnSearch(const emm_reg_t *);
int EmmDeregisteredNoImsi(const emm_reg_t *);
int EmmDeregisteredAttachNeeded(const emm_reg_t *);
int EmmDeregisteredNoCellAvailable(const emm_reg_t *);
int EmmRegisteredInitiated(const emm_reg_t *);
int EmmRegisteredNormalService(const emm_reg_t *);
int EmmRegisteredAttemptingToUpdate(const emm_reg_t *);
int EmmRegisteredLimitedService(const emm_reg_t *);
int EmmRegisteredPlmnSearch(const emm_reg_t *);
int EmmRegisteredUpdateNeeded(const emm_reg_t *);
int EmmRegisteredNoCellAvailable(const emm_reg_t *);
int EmmRegisteredAttemptingToUpdate(const emm_reg_t *);
int EmmRegisteredImsiDetachInitiated(const emm_reg_t *);
int EmmTrackingAreaUpdatingInitiated(const emm_reg_t *);
int EmmServiceRequestInitiated(const emm_reg_t *);


/* EMM state machine handlers */
static const emm_fsm_handler_t _emm_fsm_handlers[EMM_STATE_MAX] = {
  NULL,
  EmmNull,
  EmmDeregistered,
  EmmRegistered,
  EmmDeregisteredInitiated,
  EmmDeregisteredNormalService,
  EmmDeregisteredLimitedService,
  EmmDeregisteredAttemptingToAttach,
  EmmDeregisteredPlmnSearch,
  EmmDeregisteredNoImsi,
  EmmDeregisteredAttachNeeded,
  EmmDeregisteredNoCellAvailable,
  EmmRegisteredInitiated,
  EmmRegisteredNormalService,
  EmmRegisteredAttemptingToUpdate,
  EmmRegisteredLimitedService,
  EmmRegisteredPlmnSearch,
  EmmRegisteredUpdateNeeded,
  EmmRegisteredNoCellAvailable,
  EmmRegisteredAttemptingToUpdate,
  EmmRegisteredImsiDetachInitiated,
  EmmTrackingAreaUpdatingInitiated,
  EmmServiceRequestInitiated,
};

/*
 * -----------------------------------------------------------------------------
 *          Current EPS Mobility Management status
 * -----------------------------------------------------------------------------
 */

emm_fsm_state_t _emm_fsm_status[EMM_FSM_NB_UE_MAX];

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_fsm_initialize()                                      **
 **                                                                        **
 ** Description: Initializes the EMM state machine                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_fsm_status                            **
 **                                                                        **
 ***************************************************************************/
void emm_fsm_initialize(void)
{
  //int ueid;
  LOG_FUNC_IN;

  _emm_fsm_status[0] = EMM_NULL;

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_fsm_set_status()                                      **
 **                                                                        **
 ** Description: Set the EPS Mobility Management status to the given state **
 **                                                                        **
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      status:    The new EMM status                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_fsm_status                            **
 **                                                                        **
 ***************************************************************************/
int emm_fsm_set_status(
  emm_fsm_state_t  status)
{
  LOG_FUNC_IN;

  unsigned int ueid = 0;



  if ( (status < EMM_STATE_MAX) && (ueid < EMM_FSM_NB_UE_MAX) ) {
    LOG_TRACE(INFO, "EMM-FSM   - Status changed: %s ===> %s",
              _emm_fsm_status_str[_emm_fsm_status[ueid]],
              _emm_fsm_status_str[status]);

    if (status != _emm_fsm_status[ueid]) {
      _emm_fsm_status[ueid] = status;
    }

    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_fsm_get_status()                                      **
 **                                                                        **
 ** Description: Get the current value of the EPS Mobility Management      **
 **      status                                                    **
 **                                                                        **
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      Others:    _emm_fsm_status                            **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The current value of the EMM status        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
emm_fsm_state_t emm_fsm_get_status(void)
{
  return (_emm_fsm_status[0]);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_fsm_process()                                         **
 **                                                                        **
 ** Description: Executes the EMM state machine                            **
 **                                                                        **
 ** Inputs:  evt:       The EMMREG-SAP event to process            **
 **      Others:    _emm_fsm_status                            **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_fsm_process(const emm_reg_t *evt)
{
  int rc;
  emm_fsm_state_t status;
  emm_reg_primitive_t primitive;

  LOG_FUNC_IN;

  primitive = evt->primitive;

  status = _emm_fsm_status[0];

  LOG_TRACE(INFO, "EMM-FSM   - Received event %s (%d) in state %s",
            _emm_fsm_event_str[primitive - _EMMREG_START - 1], primitive,
            _emm_fsm_status_str[status]);


  /* Execute the EMM state machine */
  rc = (_emm_fsm_handlers[status])(evt);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

