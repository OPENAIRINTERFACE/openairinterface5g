/*****************************************************************************
Source      EmmDeregisteredPlmnSearch.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED.PLMN-SEARCH state.

        In EMM-DEREGISTERED.PLMN-SEARCH state, the UE with a valid
        USIM is switched on.

*****************************************************************************/


#include "emm_fsm.h"
#include "commonDef.h"
#include "networkDef.h"
#include "nas_log.h"

#include "emm_proc.h"

#include <assert.h>

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    EmmDeregisteredPlmnSearch()                               **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-DEREGISTERED.PLMN-SEARCH state.                       **
 **                                                                        **
 **              3GPP TS 24.301, section 5.2.2.3.4                         **
 **      The UE shall perform PLMN selection. If a new PLMN is     **
 **      selected, the UE shall reset the attach attempt counter   **
 **      and initiate the attach or combined attach procedure.     **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregisteredPlmnSearch(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status() == EMM_DEREGISTERED_PLMN_SEARCH);

  switch (evt->primitive) {
  case _EMMREG_NO_CELL:
    /*
     * No suitable cell of the selected PLMN has been found to camp on
     */
    rc = emm_proc_registration_notify(NET_REG_STATE_DENIED);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-FSM   - "
                "Failed to notify registration update");
    }

    rc = emm_fsm_set_status(EMM_DEREGISTERED_NO_CELL_AVAILABLE);
    break;

  case _EMMREG_REGISTER_REQ:
    /*
     * The UE has been switched on and is currently searching an
     * operator to register to. The particular PLMN to be contacted
     * may be selected either automatically or manually.
     * Or the user manually re-selected a PLMN to register to.
     */
    rc = emm_proc_registration_notify(NET_REG_STATE_ON);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-FSM   - "
                "Failed to notify registration update");
    }

    /*
     * Perform network selection procedure
     */
    rc = emm_proc_plmn_selection(evt->u.regist.index);
    break;

  case _EMMREG_REGISTER_REJ:
    /*
     * The selected cell is known not to be able to provide normal
     * service
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_LIMITED_SERVICE);
    break;

  case _EMMREG_REGISTER_CNF:
    /*
     * A suitable cell of the selected PLMN has been found to camp on
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_NORMAL_SERVICE);
    break;

  default:
    LOG_TRACE(ERROR, "EMM-FSM   - Primitive is not valid (%d)",
              evt->primitive);
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

