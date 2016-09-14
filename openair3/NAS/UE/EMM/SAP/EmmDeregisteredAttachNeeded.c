/*****************************************************************************
Source      EmmDeregisteredAttachNeeded.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED.ATTACH-NEEDED state.

        In EMM-DEREGISTERED.ATTACH-NEEDED state, Valid subscriber
        data are available for the UE and for some reason an attach
        must be performed as soon as possible. The access class may
        be blocked due to access class control, or the network rejec-
        ted the NAS signalling connection establishment.

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
 ** Name:    EmmDeregisteredAttachNeeded()                             **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-DEREGISTERED.ATTACH-NEEDED state.                     **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregisteredAttachNeeded(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  assert(emm_fsm_get_status() == EMM_DEREGISTERED_ATTACH_NEEDED);

  /* TODO */

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

