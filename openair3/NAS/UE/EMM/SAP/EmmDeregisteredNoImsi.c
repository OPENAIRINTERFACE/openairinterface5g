/*****************************************************************************
Source      EmmDeregisteredNoImsi.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED.NO-IMSI state.

        In EMM-DEREGISTERED.NO-IMSI state, the UE is switched on
        without a valid USIM inserted.

*****************************************************************************/


#include "emm_fsm.h"
#include "commonDef.h"
#include "nas_log.h"

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
 ** Name:    EmmDeregisteredNoImsi()                                   **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-DEREGISTERED.NO-IMSI state.                           **
 **                                                                        **
 **              3GPP TS 24.301, section 5.2.2.3.5                         **
 **      The UE shall perform cell selection and may initiate      **
 **      attach for emergency bearer services.                     **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregisteredNoImsi(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  assert(emm_fsm_get_status() == EMM_DEREGISTERED_NO_IMSI);

  LOG_TRACE(ERROR, "EMM-FSM   - USIM is not present or not valid");

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

