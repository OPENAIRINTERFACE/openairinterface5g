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
Source      Detach.c

Version     0.1

Date        2013/05/07

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the detach related EMM procedure executed by the
        Non-Access Stratum.

        The detach procedure is used by the UE to detach for EPS servi-
        ces, to disconnect from the last PDN it is connected to; by the
        network to inform the UE that it is detached for EPS services
        or non-EPS services or both, to disconnect the UE from the last
        PDN to which it is connected and to inform the UE to re-attach
        to the network and re-establish all PDN connections.

*****************************************************************************/

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "esm_sap.h"
#include "msc.h"

#include <stdlib.h> // free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the detach type */
static const char *_emm_detach_type_str[] = {
  "EPS", "IMSI", "EPS/IMSI",
  "RE-ATTACH REQUIRED", "RE-ATTACH NOT REQUIRED", "RESERVED"
};

/*
 * --------------------------------------------------------------------------
 *      Internal data handled by the detach procedure in the UE
 * --------------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------------
 *      Internal data handled by the detach procedure in the MME
 * --------------------------------------------------------------------------
 */


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *          Detach procedure executed by the UE
 * --------------------------------------------------------------------------
 */

/*
 * --------------------------------------------------------------------------
 *          Detach procedure executed by the MME
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_detach()                                         **
 **                                                                        **
 ** Description: Initiate the detach procedure to inform the UE that it is **
 **      detached for EPS services, or to re-attach to the network **
 **      and re-establish all PDN connections.                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.2.3.1                         **
 **      In state EMM-REGISTERED the network initiates the detach  **
 **      procedure by sending a DETACH REQUEST message to the UE,  **
 **      starting timer T3422 and entering state EMM-DEREGISTERED- **
 **      INITIATED.                                                **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      type:      Type of the requested detach               **
 **      Others:    _emm_detach_type_str                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3422                                      **
 **                                                                        **
 ***************************************************************************/
int emm_proc_detach(unsigned int ueid, emm_proc_detach_type_t type)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMM-PROC  - Initiate detach type = %s (%d)",
            _emm_detach_type_str[type], type);

  /* TODO */

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_detach_request()                                 **
 **                                                                        **
 ** Description: Performs the UE initiated detach procedure for EPS servi- **
 **      ces only When the DETACH REQUEST message is received by   **
 **      the network.                                              **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.2.2.2                         **
 **      Upon receiving the DETACH REQUEST message the network     **
 **      shall send a DETACH ACCEPT message to the UE and store    **
 **      the current EPS security context, if the detach type IE   **
 **      does not indicate "switch off". Otherwise, the procedure  **
 **      is completed when the network receives the DETACH REQUEST **
 **      message.                                                  **
 **      The network shall deactivate the EPS bearer context(s)    **
 **      for this UE locally without peer-to-peer signalling and   **
 **      shall enter state EMM-DEREGISTERED.                       **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      type:      Type of the requested detach               **
 **      switch_off:    Indicates whether the detach is required   **
 **             because the UE is switched off or not      **
 **      native_ksi:    TRUE if the security context is of type    **
 **             native                                     **
 **      ksi:       The NAS ket sey identifier                 **
 **      guti:      The GUTI if provided by the UE             **
 **      imsi:      The IMSI if provided by the UE             **
 **      imei:      The IMEI if provided by the UE             **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_detach_request(unsigned int ueid, emm_proc_detach_type_t type,
                            int switch_off, int native_ksi, int ksi,
                            GUTI_t *guti, imsi_t *imsi, imei_t *imei)
{
  LOG_FUNC_IN;

  int rc;
  emm_data_context_t *emm_ctx = NULL;

  LOG_TRACE(INFO, "EMM-PROC  - Detach type = %s (%d) requested (ueid="NAS_UE_ID_FMT")",
            _emm_detach_type_str[type], type, ueid);

  /* Get the UE context */
#if defined(NAS_BUILT_IN_EPC)

  if (ueid > 0) {
    emm_ctx = emm_data_context_get(&_emm_data, ueid);
  }

#else

  if (ueid < EMM_DATA_NB_UE_MAX) {
    emm_ctx = _emm_data.ctx[ueid];
  }

#endif

  if (emm_ctx == NULL) {
    LOG_TRACE(WARNING, "No EMM context exists for the UE (ueid="NAS_UE_ID_FMT")", ueid);
    LOG_FUNC_RETURN(RETURNok);
  }

  if (switch_off) {
	MSC_LOG_EVENT(MSC_NAS_EMM_MME,
		    	  "0 Removing UE context ue id "NAS_UE_ID_FMT" ", ueid);
    /* The UE is switched off */
    if (emm_ctx->guti) {
      free(emm_ctx->guti);
    }

    if (emm_ctx->imsi) {
      free(emm_ctx->imsi);
    }

    if (emm_ctx->imei) {
      free(emm_ctx->imei);
    }

    if (emm_ctx->esm_msg.length > 0) {
      free(emm_ctx->esm_msg.value);
    }

    /* Release NAS security context */
    if (emm_ctx->security) {
      emm_security_context_t *security = emm_ctx->security;

      if (security->kasme.value) {
        free(security->kasme.value);
      }

      if (security->knas_enc.value) {
        free(security->knas_enc.value);
      }

      if (security->knas_int.value) {
        free(security->knas_int.value);
      }

      free(emm_ctx->security);
    }
    /* Stop timer T3450 */
    if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
      LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3450 (%d)", emm_ctx->T3450.id);
      emm_ctx->T3450.id = nas_timer_stop(emm_ctx->T3450.id);
      MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3450 stopped UE "NAS_UE_ID_FMT" ", emm_ctx->ueid);
    }
    /* Stop timer T3460 */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3460 (%d)", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop(emm_ctx->T3460.id);
      MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3460 stopped UE "NAS_UE_ID_FMT" ", emm_ctx->ueid);
    }
    /* Stop timer T3470 */
    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3470 (%d)", emm_ctx->T3460.id);
      emm_ctx->T3470.id = nas_timer_stop(emm_ctx->T3470.id);
      MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3470 stopped UE "NAS_UE_ID_FMT" ", emm_ctx->ueid);
    }
    /* Release the EMM context */
#if defined(NAS_BUILT_IN_EPC)
    emm_data_context_remove(&_emm_data, emm_ctx);
    free(emm_ctx);
#else
    free(_emm_data.ctx[ueid]);
    _emm_data.ctx[ueid] = NULL;
#endif
    rc = RETURNok;
  } else {
    /* Normal detach without UE switch-off */
    emm_sap_t emm_sap;
    emm_as_data_t *emm_as = &emm_sap.u.emm_as.u.data;
	MSC_LOG_TX_MESSAGE(
	    		MSC_NAS_EMM_MME,
	    		MSC_NAS_EMM_MME,
	    	  	NULL,0,
	    	  	"0 EMM_AS_NAS_INFO_DETACH ue id "NAS_UE_ID_FMT" ", ueid);

    /* Setup NAS information message to transfer */
    emm_as->NASinfo = EMM_AS_NAS_INFO_DETACH;
    emm_as->NASmsg.length = 0;
    emm_as->NASmsg.value = NULL;
    /* Set the UE identifier */
    emm_as->guti = NULL;
    emm_as->ueid = ueid;
    /* Setup EPS NAS security data */
    emm_as_set_security_data(&emm_as->sctx, emm_ctx->security, FALSE, TRUE);
    /*
     * Notify EMM-AS SAP that Detach Accept message has to
     * be sent to the network
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    rc = emm_sap_send(&emm_sap);
  }

  if (rc != RETURNerror) {
    /*
     * Notify ESM that all EPS bearer contexts allocated for this UE have
     * to be locally deactivated
     */
	MSC_LOG_TX_MESSAGE(
	    		MSC_NAS_EMM_MME,
	    	  	MSC_NAS_ESM_MME,
	    	  	NULL,0,
	    	  	"0 ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ ue id "NAS_UE_ID_FMT" ", ueid);
	esm_sap_t esm_sap;
    esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
    esm_sap.ueid = ueid;
    esm_sap.ctx  = emm_ctx;
    esm_sap.data.eps_bearer_context_deactivate.ebi = ESM_SAP_ALL_EBI;
    rc = esm_sap_send(&esm_sap);

    if (rc != RETURNerror) {
      emm_sap_t emm_sap;
      /*
       * Notify EMM that the UE has been implicitly detached
       */
  	  MSC_LOG_TX_MESSAGE(
  	    		MSC_NAS_EMM_MME,
  	    		MSC_NAS_EMM_MME,
  	    	  	NULL,0,
  	    	  	"0 EMMREG_DETACH_REQ ue id "NAS_UE_ID_FMT" ", ueid);
      emm_sap.primitive = EMMREG_DETACH_REQ;
      emm_sap.u.emm_reg.ueid = ueid;
      emm_sap.u.emm_reg.ctx  = emm_ctx;
      rc = emm_sap_send(&emm_sap);
    }
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

