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
Source      Emmstatus.c

Version     0.1

Date        2013/06/26

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EMM status procedure executed by the Non-Access
        Stratum.

        The purpose of the sending of the EMM STATUS message is to
        report at any time certain error conditions detected upon
        receipt of EMM protocol data. The EMM STATUS message can be
        sent by both the MME and the UE.

*****************************************************************************/

#include "emm_proc.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emm_cause.h"
#include "emmData.h"

#include "emm_sap.h"

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
 ** Name:    emm_proc_status_ind()                                     **
 **                                                                        **
 ** Description: Processes received EMM status message.                    **
 **                                                                        **
 **      3GPP TS 24.301, section 5.7                               **
 **      On receipt of an EMM STATUS message no state transition   **
 **      and no specific action shall be taken. Local actions are  **
 **      possible and are implementation dependent.                **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **          emm_cause: Received EMM cause code                    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_status_ind(unsigned int ueid, int emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO,"EMM-PROC  - EMM status procedure requested (cause=%d)",
            emm_cause);

  LOG_TRACE(DEBUG, "EMM-PROC  - To be implemented");

  /* TODO */
  rc = RETURNok;

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_status()                                         **
 **                                                                        **
 ** Description: Initiates EMM status procedure.                           **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      emm_cause: EMM cause code to be reported              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_status(unsigned int ueid, int emm_cause)
{
  LOG_FUNC_IN;

  int rc;
  emm_sap_t emm_sap;

  emm_security_context_t    *sctx = NULL;
  //struct emm_data_context_s *ctx  = NULL;

  LOG_TRACE(INFO,"EMM-PROC  - EMM status procedure requested");

  /*
   * Notity EMM that EMM status indication has to be sent to lower layers
   */
  emm_sap.primitive = EMMAS_STATUS_IND;
  emm_sap.u.emm_as.u.status.emm_cause = emm_cause;
  emm_sap.u.emm_as.u.status.ueid = ueid;

  emm_sap.u.emm_as.u.status.guti = _emm_data.guti;
  sctx = _emm_data.security;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.status.sctx, sctx,
                           FALSE, TRUE);

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
