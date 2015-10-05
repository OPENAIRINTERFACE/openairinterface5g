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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

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

#include "mme_api.h"
#include "emmData.h"

#if defined(NAS_BUILT_IN_EPC)
# include "assertions.h"
#endif
# include "msc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/


#define EMM_FSM_NB_UE_MAX   (MME_API_NB_UE_MAX + 1)

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
  "COMMON_PROC_REQ",
  "COMMON_PROC_CNF",
  "COMMON_PROC_REJ",
  "PROC_ABORT",
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
  "DEREGISTERED",
  "REGISTERED",
  "DEREGISTERED-INITIATED",
  "COMMON-PROCEDURE-INITIATED",
};

/*
 * -----------------------------------------------------------------------------
 *      EPS Mobility Management state machine handlers
 * -----------------------------------------------------------------------------
 */

/* Type of the EPS Mobility Management state machine handler */
typedef int(*emm_fsm_handler_t)(const emm_reg_t *);

int EmmDeregistered(const emm_reg_t *);
int EmmRegistered(const emm_reg_t *);
int EmmDeregisteredInitiated(const emm_reg_t *);
int EmmCommonProcedureInitiated(const emm_reg_t *);

/* EMM state machine handlers */
static const emm_fsm_handler_t _emm_fsm_handlers[EMM_STATE_MAX] = {
  NULL,
  EmmDeregistered,
  EmmRegistered,
  EmmDeregisteredInitiated,
  EmmCommonProcedureInitiated,
};

/*
 * -----------------------------------------------------------------------------
 *          Current EPS Mobility Management status
 * -----------------------------------------------------------------------------
 */

#if !defined(NAS_BUILT_IN_EPC)
emm_fsm_state_t _emm_fsm_status[EMM_FSM_NB_UE_MAX];
#endif

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
  int ueid;
  LOG_FUNC_IN;


#if !defined(NAS_BUILT_IN_EPC)

  for (ueid = 0; ueid < EMM_FSM_NB_UE_MAX; ueid++) {
    _emm_fsm_status[ueid] = EMM_DEREGISTERED;
  }

#endif

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
  unsigned int     ueid,
  void            *ctx,
  emm_fsm_state_t  status)
{
  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  emm_data_context_t *emm_ctx = (emm_data_context_t *)ctx;

  DevAssert(emm_ctx != NULL);

  if ((status < EMM_STATE_MAX) && (ueid > 0)) {

    if (status != emm_ctx->_emm_fsm_status) {
      LOG_TRACE(INFO, "EMM-FSM   - Status changed: %s ===> %s",
                  _emm_fsm_status_str[emm_ctx->_emm_fsm_status],
                  _emm_fsm_status_str[status]);
      MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 EMM state %s ", _emm_fsm_status_str[status]);
      emm_ctx->_emm_fsm_status = status;
    }

    LOG_FUNC_RETURN (RETURNok);
  }

#else

  if ( (status < EMM_STATE_MAX) && (ueid < EMM_FSM_NB_UE_MAX) ) {
    if (status != _emm_fsm_status[ueid]) {
      LOG_TRACE(INFO, "EMM-FSM   - Status changed: %s ===> %s",
                  _emm_fsm_status_str[_emm_fsm_status[ueid]],
                  _emm_fsm_status_str[status]);

      _emm_fsm_status[ueid] = status;
    }

    LOG_FUNC_RETURN (RETURNok);
  }

#endif

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
emm_fsm_state_t emm_fsm_get_status(unsigned int ueid, void *ctx)
{
# if defined(NAS_BUILT_IN_EPC)
  emm_data_context_t *emm_ctx = (emm_data_context_t *)ctx;
  if (emm_ctx == NULL) {
	LOG_TRACE(INFO, "EMM-FSM   - try again get context ueid "NAS_UE_ID_FMT" ",
				ueid);
    emm_ctx = emm_data_context_get(&_emm_data, ueid);
  }
  if (emm_ctx != NULL) {
    return emm_ctx->_emm_fsm_status;
  }

# else

  if (ueid < EMM_FSM_NB_UE_MAX) {
    return (_emm_fsm_status[ueid]);
  }
#endif
  return EMM_STATE_MAX;
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

# if defined(NAS_BUILT_IN_EPC)
  emm_data_context_t *emm_ctx = (emm_data_context_t *)evt->ctx;
  DevAssert(emm_ctx != NULL);

  status = emm_fsm_get_status(0, emm_ctx);
# else

  if (evt->ueid >= EMM_FSM_NB_UE_MAX) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  status = _emm_fsm_status[evt->ueid];
# endif

  LOG_TRACE(INFO, "EMM-FSM   - Received event %s (%d) in state %s",
            _emm_fsm_event_str[primitive - _EMMREG_START - 1], primitive,
            _emm_fsm_status_str[status]);

#if defined(NAS_BUILT_IN_EPC)
  DevAssert(status != EMM_INVALID);
#endif

  /* Execute the EMM state machine */
  rc = (_emm_fsm_handlers[status])(evt);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

