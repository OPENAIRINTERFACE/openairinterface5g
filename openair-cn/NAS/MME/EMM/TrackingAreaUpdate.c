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
Source      TrackingAreaUpdate.c

Version     0.1

Date        2013/05/07

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the tracking area update EMM procedure executed by the
        Non-Access Stratum.

        The tracking area updating procedure is always initiated by the
        UE and is used to update the registration of the actual tracking
        area of a UE in the network, to periodically notify the availa-
        bility of the UE to the network, for MME load balancing, to up-
        date certain UE specific parameters in the network.

*****************************************************************************/

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "emm_cause.h"

#include <string.h> // memcmp, memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *   Internal data handled by the tracking area update procedure in the MME
 * --------------------------------------------------------------------------
 */
static int _emm_tracking_area_update_reject(void *args);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:        emm_proc_tracking_area_update_reject()                    **
 **                                                                        **
 ** Description:                                                           **
 **                                                                        **
 ** Inputs:  ueid:              UE lower layer identifier                  **
 **                  emm_cause: EMM cause code to be reported              **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_tracking_area_update_reject(unsigned int ueid, int emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  /* Create temporary UE context */
  emm_data_context_t ue_ctx;
  memset(&ue_ctx, 0 , sizeof(emm_data_context_t));
  ue_ctx.is_dynamic = FALSE;
  ue_ctx.ueid = ueid;

  /* Update the EMM cause code */
#if defined(NAS_BUILT_IN_EPC)

  if (ueid > 0)
#else
  if (ueid < EMM_DATA_NB_UE_MAX)
#endif
  {
    ue_ctx.emm_cause = emm_cause;
  } else {
    ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /* Do not accept attach request with protocol error */
  rc = _emm_tracking_area_update_reject(&ue_ctx);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/



/****************************************************************************
 **                                                                        **
 ** Name:        _emm_tracking_area_update_reject()                        **
 **                                                                        **
 ** Description: Performs the tracking area update procedure not accepted  **
 **               by the network.                                         **
 **                                                                        **
 ** Inputs:      args:          UE context data                            **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **              Return:        RETURNok, RETURNerror                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_tracking_area_update_reject(void *args)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  emm_data_context_t *emm_ctx = (emm_data_context_t *)(args);

  if (emm_ctx) {
    emm_sap_t emm_sap;
    LOG_TRACE(WARNING, "EMM-PROC  - EMM tracking area update procedure not accepted "
              "by the network (ueid="NAS_UE_ID_FMT", cause=%d)",
              emm_ctx->ueid, emm_ctx->emm_cause);
    /*
     * Notify EMM-AS SAP that Tracking Area Update Reject message has to be sent
     * onto the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REJ;
    emm_sap.u.emm_as.u.establish.ueid = emm_ctx->ueid;
    emm_sap.u.emm_as.u.establish.UEid.guti = NULL;

    if (emm_ctx->emm_cause == EMM_CAUSE_SUCCESS) {
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    }

    emm_sap.u.emm_as.u.establish.emm_cause = emm_ctx->emm_cause;
    emm_sap.u.emm_as.u.establish.NASinfo = EMM_AS_NAS_INFO_TAU;
    emm_sap.u.emm_as.u.establish.NASmsg.length = 0;
    emm_sap.u.emm_as.u.establish.NASmsg.value = NULL;

    /* Setup EPS NAS security data */
    emm_as_set_security_data(&emm_sap.u.emm_as.u.establish.sctx,
                             emm_ctx->security, FALSE, TRUE);
    rc = emm_sap_send(&emm_sap);
  }

  LOG_FUNC_RETURN(rc);
}

