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

Source      Attach.c

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the attach related EMM procedure executed by the
        Non-Access Stratum.

        To get internet connectivity from the network, the network
        have to know about the UE. When the UE is switched on, it
        has to initiate the attach procedure to get initial access
        to the network and register its presence to the Evolved
        Packet Core (EPC) network in order to receive EPS services.

        As a result of a successful attach procedure, a context is
        created for the UE in the MME, and a default bearer is esta-
        blished between the UE and the PDN-GW. The UE gets the home
        agent IPv4 and IPv6 addresses and full connectivity to the
        IP network.

        The network may also initiate the activation of additional
        dedicated bearers for the support of a specific service.

*****************************************************************************/

#include "emm_proc.h"
#include "networkDef.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "esm_sap.h"
#include "emm_cause.h"

#include "NasSecurityAlgorithms.h"

#include "mme_api.h"
#include "mme_config.h"
# if defined(NAS_BUILT_IN_EPC)
#   include "nas_itti_messaging.h"
# endif

#include <string.h> // memcmp, memcpy
#include <stdlib.h> // malloc, free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the EPS attach type */
static const char *_emm_attach_type_str[] = {
  "EPS", "IMSI", "EMERGENCY", "RESERVED"
};


/*
 * --------------------------------------------------------------------------
 *      Internal data handled by the attach procedure in the MME
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
static void *_emm_attach_t3450_handler(void *);

/*
 * Functions that may initiate EMM common procedures
 */
static int _emm_attach_identify(void *);
static int _emm_attach_security(void *);
static int _emm_attach(void *);

/*
 * Abnormal case attach procedures
 */
static int _emm_attach_release(void *);
static int _emm_attach_reject(void *);
static int _emm_attach_abort(void *);

static int _emm_attach_have_changed(const emm_data_context_t *ctx,
                                    emm_proc_attach_type_t type, int ksi,
                                    GUTI_t *guti, imsi_t *imsi, imei_t *imei,
                                    int eea, int eia, int ucs2, int uea, int uia, int gea,
                                    int umts_present, int gprs_present);
static int _emm_attach_update(emm_data_context_t *ctx, unsigned int ueid,
                              emm_proc_attach_type_t type, int ksi,
                              GUTI_t *guti, imsi_t *imsi, imei_t *imei,
                              int eea, int eia, int ucs2, int uea, int uia, int gea,
                              int umts_present, int gprs_present,
                              const OctetString *esm_msg_pP);

/*
 * Internal data used for attach procedure
 */
typedef struct attach_data_s {
  unsigned int ueid;          /* UE identifier        */
#define ATTACH_COUNTER_MAX  5
  unsigned int retransmission_count;  /* Retransmission counter   */
  OctetString esm_msg;        /* ESM message to be sent within
                     * the Attach Accept message    */
} attach_data_t;

static int _emm_attach_accept(emm_data_context_t *emm_ctx, attach_data_t *data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
 * --------------------------------------------------------------------------
 *          Attach procedure executed by the MME
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_request()                                 **
 **                                                                        **
 ** Description: Performs the UE requested attach procedure                **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.3                         **
 **      The network may initiate EMM common procedures, e.g. the  **
 **      identification, authentication and security mode control  **
 **      procedures during the attach procedure, depending on the  **
 **      information received in the ATTACH REQUEST message (e.g.  **
 **      IMSI, GUTI and KSI).                                      **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      type:      Type of the requested attach               **
 **      native_ksi:    TRUE if the security context is of type    **
 **             native (for KSIASME)                       **
 **      ksi:       The NAS ket sey identifier                 **
 **      native_guti:   TRUE if the provided GUTI is native GUTI   **
 **      guti:      The GUTI if provided by the UE             **
 **      imsi:      The IMSI if provided by the UE             **
 **      imei:      The IMEI if provided by the UE             **
 **      tai:       Identifies the last visited tracking area  **
 **             the UE is registered to                    **
 **      eea:       Supported EPS encryption algorithms        **
 **      eia:       Supported EPS integrity algorithms         **
 **      esm_msg_pP:   PDN connectivity request ESM message       **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_request(
  unsigned int ueid,
  emm_proc_attach_type_t type,
  int native_ksi, int ksi,
  int native_guti,
  GUTI_t *guti,
  imsi_t *imsi,
  imei_t *imei,
  tai_t    *tai,
  int eea, int eia, int ucs2, int uea, int uia, int gea,
  int umts_present, int gprs_present,
  const OctetString *esm_msg_pP)
{
  LOG_FUNC_IN;

  int rc;
  emm_data_context_t ue_ctx;

  LOG_TRACE(INFO, "EMM-PROC  - EPS attach type = %s (%d) requested (ueid="NAS_UE_ID_FMT")",
            _emm_attach_type_str[type], type, ueid);
  LOG_TRACE(INFO, "EMM-PROC  - umts_present = %u umts_present = %u",
            umts_present, gprs_present);

  /* Initialize the temporary UE context */
  memset(&ue_ctx, 0 , sizeof(emm_data_context_t));
  ue_ctx.is_dynamic = FALSE;
  ue_ctx.ueid = ueid;

#if !defined(NAS_BUILT_IN_EPC)

  /* UE identifier sanity check */
  if (ueid >= EMM_DATA_NB_UE_MAX) {
    ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /* Do not accept UE with invalid identifier */
    rc = _emm_attach_reject(&ue_ctx);
    LOG_FUNC_RETURN(rc);
  }

#endif

  /* 3GPP TS 24.301, section 5.5.1.1
   * MME not configured to support attach for emergency bearer services
   * shall reject any request to attach with an attach type set to "EPS
   * emergency attach".
   */
  if ( !(_emm_data.conf.features & MME_API_EMERGENCY_ATTACH) &&
       (type == EMM_ATTACH_TYPE_EMERGENCY) ) {
    ue_ctx.emm_cause = EMM_CAUSE_IMEI_NOT_ACCEPTED;
    /* Do not accept the UE to attach for emergency services */
    rc = _emm_attach_reject(&ue_ctx);
    LOG_FUNC_RETURN(rc);
  }

  /* Get the UE's EMM context if it exists */
  emm_data_context_t **emm_ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
  emm_data_context_t *temp = NULL;

  temp    = emm_data_context_get(&_emm_data, ueid);
  emm_ctx = &temp;
#else
  emm_ctx = &_emm_data.ctx[ueid];
#endif

  if (*emm_ctx != NULL) {
    /* An EMM context already exists for the UE in the network */
    if (_emm_attach_have_changed(*emm_ctx, type, ksi, guti, imsi, imei,
                                 eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present)) {
      /*
       * 3GPP TS 24.301, section 5.5.1.2.7, abnormal case e
       * The attach parameters have changed from the one received within
       * the previous Attach Request message;
       * the previously initiated attach procedure shall be aborted and
       * the new attach procedure shall be executed;
       */
      LOG_TRACE(WARNING, "EMM-PROC  - Attach parameters have changed");
      /*
       * Notify EMM that the attach procedure is aborted
       */
      emm_sap_t emm_sap;
      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ueid = ueid;
      emm_sap.u.emm_reg.ctx  = *emm_ctx;

      rc = emm_sap_send(&emm_sap);

      if (rc != RETURNerror) {
        /* Process new attach procedure */
        LOG_TRACE(WARNING, "EMM-PROC  - Initiate new attach procedure");
        rc = emm_proc_attach_request(ueid, type, native_ksi, ksi,
                                     native_guti, guti, imsi, imei,
                                     tai, eea, eia, ucs2, uea, uia, gea,
                                     umts_present, gprs_present, esm_msg_pP);
      }

      LOG_FUNC_RETURN(rc);
    } else {
      /* Continue with the previous attach procedure */
      LOG_TRACE(WARNING, "EMM-PROC  - Received duplicated Attach Request");
      LOG_FUNC_RETURN(RETURNok);
    }
  } else {
    /* Create UE's EMM context */
    *emm_ctx = (emm_data_context_t *)calloc(1, sizeof(emm_data_context_t));

    if (emm_ctx == NULL) {
      LOG_TRACE(WARNING, "EMM-PROC  - Failed to create EMM context");
      ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE;
      /* Do not accept the UE to attach to the network */
      rc = _emm_attach_reject(&ue_ctx);
      LOG_FUNC_RETURN(rc);
    }

    (*emm_ctx)->is_dynamic = TRUE;
    (*emm_ctx)->guti = NULL;
    (*emm_ctx)->old_guti = NULL;
    (*emm_ctx)->imsi = NULL;
    (*emm_ctx)->imei = NULL;
    (*emm_ctx)->security = NULL;
    (*emm_ctx)->esm_msg.length = 0;
    (*emm_ctx)->esm_msg.value = NULL;
    (*emm_ctx)->emm_cause = EMM_CAUSE_SUCCESS;
    (*emm_ctx)->_emm_fsm_status = EMM_INVALID;
    (*emm_ctx)->ueid = ueid;
    /*
     * Initialize EMM timers
     */
    (*emm_ctx)->T3450.id = NAS_TIMER_INACTIVE_ID;
    (*emm_ctx)->T3450.sec = T3450_DEFAULT_VALUE;
    (*emm_ctx)->T3460.id = NAS_TIMER_INACTIVE_ID;
    (*emm_ctx)->T3460.sec = T3460_DEFAULT_VALUE;
    (*emm_ctx)->T3470.id = NAS_TIMER_INACTIVE_ID;
    (*emm_ctx)->T3470.sec = T3470_DEFAULT_VALUE;

    emm_fsm_set_status(ueid, *emm_ctx, EMM_DEREGISTERED);
#if defined(NAS_BUILT_IN_EPC)
    emm_data_context_add(&_emm_data, *(emm_ctx));
#endif

#warning "TRICK TO SET TAC, BUT LOOK AT SPEC"

    if (tai) {
      LOG_TRACE(WARNING,
                "EMM-PROC  - Set tac %u in context",
                tai->tac);
      (*emm_ctx)->tac = tai->tac;
    } else {
      LOG_TRACE(WARNING,
                "EMM-PROC  - Could not set tac in context, cause tai is NULL ");
    }
  }

  /* Update the EMM context with the current attach procedure parameters */
  rc = _emm_attach_update(*emm_ctx, ueid, type, ksi, guti, imsi, imei,
                          eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present, esm_msg_pP);

  if (rc != RETURNok) {
    LOG_TRACE(WARNING, "EMM-PROC  - Failed to update EMM context");
    /* Do not accept the UE to attach to the network */
    (*emm_ctx)->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    rc = _emm_attach_reject(*emm_ctx);
  } else {
    /*
     * Performs UE identification
     */
    rc = _emm_attach_identify(*emm_ctx);
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:        emm_proc_attach_reject()                                  **
 **                                                                        **
 ** Description: Performs the protocol error abnormal case                 **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.7, case b                 **
 **              If the ATTACH REQUEST message is received with a protocol **
 **              error, the network shall return an ATTACH REJECT message. **
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
int emm_proc_attach_reject(unsigned int ueid, int emm_cause)
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
  rc = _emm_attach_reject(&ue_ctx);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_complete()                                **
 **                                                                        **
 ** Description: Terminates the attach procedure upon receiving Attach     **
 **      Complete message from the UE.                             **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.4                         **
 **      Upon receiving an ATTACH COMPLETE message, the MME shall  **
 **      stop timer T3450, enter state EMM-REGISTERED and consider **
 **      the GUTI sent in the ATTACH ACCEPT message as valid.      **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      esm_msg_pP:   Activate default EPS bearer context accept **
 **             ESM message                                **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data, T3450                           **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_complete(unsigned int ueid, const OctetString *esm_msg_pP)
{
  emm_data_context_t *emm_ctx = NULL;
  int rc = RETURNerror;
  emm_sap_t emm_sap;
  esm_sap_t esm_sap;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMM-PROC  - EPS attach complete (ueid="NAS_UE_ID_FMT")", ueid);

  /* Release retransmission timer parameters */
  attach_data_t *data = (attach_data_t *)(emm_proc_common_get_args(ueid));

  if (data) {
    if (data->esm_msg.length > 0) {
      free(data->esm_msg.value);
    }
    free(data);
  }

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

  if (emm_ctx) {
	/* Stop timer T3450 */
	LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3450 (%d)", emm_ctx->T3450.id);
	emm_ctx->T3450.id = nas_timer_stop(emm_ctx->T3450.id);
	MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3450 stopped UE "NAS_UE_ID_FMT" ", ueid);

	/* Delete the old GUTI and consider the GUTI sent in the Attach
     * Accept message as valid */
    emm_ctx->guti_is_new = FALSE;
    emm_ctx->old_guti = NULL;
    /*
     * Forward the Activate Default EPS Bearer Context Accept message
     * to the EPS session management sublayer
     */
    esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF;
    esm_sap.is_standalone = FALSE;
    esm_sap.ueid = ueid;
    esm_sap.recv = esm_msg_pP;
    esm_sap.ctx  = emm_ctx;
    rc = esm_sap_send(&esm_sap);
  } else {
    LOG_TRACE(ERROR, "EMM-PROC  - No EMM context exists");
  }

  if ( (rc != RETURNerror) && (esm_sap.err == ESM_SAP_SUCCESS) ) {
    /* Set the network attachment indicator */
    emm_ctx->is_attached = TRUE;
    /*
     * Notify EMM that attach procedure has successfully completed
     */
    emm_sap.primitive = EMMREG_ATTACH_CNF;
    emm_sap.u.emm_reg.ueid = ueid;
    emm_sap.u.emm_reg.ctx  = emm_ctx;
    rc = emm_sap_send(&emm_sap);
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /*
     * Notify EMM that attach procedure failed
     */
    emm_sap.primitive = EMMREG_ATTACH_REJ;
    emm_sap.u.emm_reg.ueid = ueid;
    emm_sap.u.emm_reg.ctx  = emm_ctx;
    rc = emm_sap_send(&emm_sap);
  } else {
    /*
     * ESM procedure failed and, received message has been discarded or
     * Status message has been returned; ignore ESM procedure failure
     */
    rc = RETURNok;
  }

  LOG_FUNC_RETURN(rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/



/*
 * --------------------------------------------------------------------------
 *              Timer handlers
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_t3450_handler()                               **
 **                                                                        **
 ** Description: T3450 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.7, case c                 **
 **      On the first expiry of the timer T3450, the network shall **
 **      retransmit the ATTACH ACCEPT message and shall reset and  **
 **      restart timer T3450. This retransmission is repeated four **
 **      times, i.e. on the fifth expiry of timer T3450, the at-   **
 **      tach procedure shall be aborted and the MME enters state  **
 **      EMM-DEREGISTERED.                                         **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_emm_attach_t3450_handler(void *args)
{
  LOG_FUNC_IN;

  int rc;

  attach_data_t *data = (attach_data_t *)(args);

  /* Increment the retransmission counter */
  data->retransmission_count += 1;

  LOG_TRACE(WARNING, "EMM-PROC  - T3450 timer expired, retransmission "
            "counter = %d", data->retransmission_count);

  /* Get the UE's EMM context */
  emm_data_context_t *emm_ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
  emm_ctx = emm_data_context_get(&_emm_data, data->ueid);
#else
  emm_ctx = _emm_data.ctx[data->ueid];
#endif

  if (data->retransmission_count < ATTACH_COUNTER_MAX) {
    /* Send attach accept message to the UE */
    rc = _emm_attach_accept(emm_ctx, data);
  } else {
    /* Abort the attach procedure */
    rc = _emm_attach_abort(data);
  }

  LOG_FUNC_RETURN(NULL);
}

/*
 * --------------------------------------------------------------------------
 *              Abnormal cases in the MME
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_release()                                     **
 **                                                                        **
 ** Description: Releases the UE context data.                             **
 **                                                                        **
 ** Inputs:  args:      Data to be released                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_release(void *args)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  emm_data_context_t *emm_ctx = (emm_data_context_t *)(args);

  if (emm_ctx) {
    LOG_TRACE(WARNING, "EMM-PROC  - Release UE context data (ueid="NAS_UE_ID_FMT")",
              emm_ctx->ueid);

    unsigned int ueid = emm_ctx->ueid;

    if (emm_ctx->guti) {
      free(emm_ctx->guti);
      emm_ctx->guti = NULL;
    }

    if (emm_ctx->imsi) {
      free(emm_ctx->imsi);
      emm_ctx->imsi = NULL;
    }

    if (emm_ctx->imei) {
      free(emm_ctx->imei);
      emm_ctx->imei = NULL;
    }

    if (emm_ctx->esm_msg.length > 0) {
      free(emm_ctx->esm_msg.value);
      emm_ctx->esm_msg.value = NULL;
    }

    /* Release NAS security context */
    if (emm_ctx->security) {
      emm_security_context_t *security = emm_ctx->security;

      if (security->kasme.value) {
        free(security->kasme.value);
        security->kasme.value  = NULL;
        security->kasme.length = 0;
      }

      if (security->knas_enc.value) {
        free(security->knas_enc.value);
        security->knas_enc.value  = NULL;
        security->knas_enc.length = 0;
      }

      if (security->knas_int.value) {
        free(security->knas_int.value);
        security->knas_int.value  = NULL;
        security->knas_int.length = 0;
      }

      free(emm_ctx->security);
      emm_ctx->security = NULL;
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
#else
    free(_emm_data.ctx[ueid]);
    _emm_data.ctx[ueid] = NULL;
#endif
    /*
     * Notify EMM that the attach procedure is aborted
     */
    emm_sap_t emm_sap;
    emm_sap.primitive = EMMREG_PROC_ABORT;
    emm_sap.u.emm_reg.ueid = ueid;
    emm_sap.u.emm_reg.ctx  = emm_ctx;
    rc = emm_sap_send(&emm_sap);
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_reject()                                      **
 **                                                                        **
 ** Description: Performs the attach procedure not accepted by the network.**
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.5                         **
 **      If the attach request cannot be accepted by the network,  **
 **      the MME shall send an ATTACH REJECT message to the UE in- **
 **      including an appropriate EMM cause value.                 **
 **                                                                        **
 ** Inputs:  args:      UE context data                            **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_reject(void *args)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  emm_data_context_t *emm_ctx = (emm_data_context_t *)(args);

  if (emm_ctx) {
    emm_sap_t emm_sap;
    LOG_TRACE(WARNING, "EMM-PROC  - EMM attach procedure not accepted "
              "by the network (ueid="NAS_UE_ID_FMT", cause=%d)",
              emm_ctx->ueid, emm_ctx->emm_cause);
    /*
     * Notify EMM-AS SAP that Attach Reject message has to be sent
     * onto the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REJ;
    emm_sap.u.emm_as.u.establish.ueid = emm_ctx->ueid;
    emm_sap.u.emm_as.u.establish.UEid.guti = NULL;

    if (emm_ctx->emm_cause == EMM_CAUSE_SUCCESS) {
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    }

    emm_sap.u.emm_as.u.establish.emm_cause = emm_ctx->emm_cause;
    emm_sap.u.emm_as.u.establish.NASinfo = EMM_AS_NAS_INFO_ATTACH;

    if (emm_ctx->emm_cause != EMM_CAUSE_ESM_FAILURE) {
      emm_sap.u.emm_as.u.establish.NASmsg.length = 0;
      emm_sap.u.emm_as.u.establish.NASmsg.value = NULL;
    } else if (emm_ctx->esm_msg.length > 0) {
      emm_sap.u.emm_as.u.establish.NASmsg = emm_ctx->esm_msg;
    } else {
      LOG_TRACE(ERROR, "EMM-PROC  - ESM message is missing");
      LOG_FUNC_RETURN(RETURNerror);
    }

    /* Setup EPS NAS security data */
    emm_as_set_security_data(&emm_sap.u.emm_as.u.establish.sctx,
                             emm_ctx->security, FALSE, TRUE);
    rc = emm_sap_send(&emm_sap);

    /* Release the UE context, even if the network failed to send the
     * ATTACH REJECT message */
    if (emm_ctx->is_dynamic) {
      rc = _emm_attach_release(emm_ctx);
    }
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_abort()                                       **
 **                                                                        **
 ** Description: Aborts the attach procedure                               **
 **                                                                        **
 ** Inputs:  args:      Attach procedure data to be released       **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3450                                      **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_abort(void *args)
{
  int rc = RETURNerror;
  emm_data_context_t *ctx = NULL;
  attach_data_t *data;

  LOG_FUNC_IN;

  data = (attach_data_t *)(args);

  if (data) {
    unsigned int ueid = data->ueid;
    esm_sap_t esm_sap;

    LOG_TRACE(WARNING, "EMM-PROC  - Abort the attach procedure (ueid="NAS_UE_ID_FMT")",
              ueid);
#if defined(NAS_BUILT_IN_EPC)
    ctx = emm_data_context_get(&_emm_data, ueid);
#else
    ctx = _emm_data.ctx[ueid];
#endif
    if (ctx) {
      /* Stop timer T3450 */
      if (ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
        LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3450 (%d)", ctx->T3450.id);
        ctx->T3450.id = nas_timer_stop(ctx->T3450.id);
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3450 stopped UE "NAS_UE_ID_FMT" ", data->ueid);
      }
    }

    /* Release retransmission timer parameters */
    if (data->esm_msg.length > 0) {
      free(data->esm_msg.value);
    }

    free(data);



    /*
     * Notify ESM that the network locally refused PDN connectivity
     * to the UE
     */
    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
    esm_sap.ueid = ueid;
    esm_sap.ctx  = ctx;
    esm_sap.recv = NULL;
    rc = esm_sap_send(&esm_sap);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that EPS attach procedure failed
       */
      emm_sap_t emm_sap;
      emm_sap.primitive = EMMREG_ATTACH_REJ;
      emm_sap.u.emm_reg.ueid = ueid;
      emm_sap.u.emm_reg.ctx  = ctx;
      rc = emm_sap_send(&emm_sap);

      if (rc != RETURNerror) {
        /* Release the UE context */
        rc = _emm_attach_release(ctx);
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 *      Functions that may initiate EMM common procedures
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_identify()                                    **
 **                                                                        **
 ** Description: Performs UE's identification. May initiates identifica-   **
 **      tion, authentication and security mode control EMM common **
 **      procedures.                                               **
 **                                                                        **
 ** Inputs:  args:      Identification argument parameters         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_identify(void *args)
{
  int rc = RETURNerror;
  emm_data_context_t *emm_ctx = (emm_data_context_t *)(args);
  int guti_reallocation = FALSE;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMM-PROC  - Identify incoming UE (ueid="NAS_UE_ID_FMT") using %s",
            emm_ctx->ueid, (emm_ctx->imsi)? "IMSI" : (emm_ctx->guti)? "GUTI" :
            (emm_ctx->imei)? "IMEI" : "none");

  /*
   * UE's identification
   * -------------------
   */
  if (emm_ctx->imsi) {
    /* The UE identifies itself using an IMSI */
#if defined(NAS_BUILT_IN_EPC)
    if (!emm_ctx->security) {
      /* Ask upper layer to fetch new security context */
      nas_itti_auth_info_req(emm_ctx->ueid, emm_ctx->imsi, 1, NULL);

      rc = RETURNok;
    } else
#endif
    {
      rc = mme_api_identify_imsi(emm_ctx->imsi, &emm_ctx->vector);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING, "EMM-PROC  - "
                  "Failed to identify the UE using provided IMSI");
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      }

      guti_reallocation = TRUE;
    }
  } else if (emm_ctx->guti) {
    /* The UE identifies itself using a GUTI */
    rc = mme_api_identify_guti(emm_ctx->guti, &emm_ctx->vector);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-PROC  - Failed to identify the UE using "
                "provided GUTI (tmsi=%u)", emm_ctx->guti->m_tmsi);
      /* 3GPP TS 24.401, Figure 5.3.2.1-1, point 4
       * The UE was attempting to attach to the network using a GUTI
       * that is not known by the network; the MME shall initiate an
       * identification procedure to retrieve the IMSI from the UE.
       */
      rc = emm_proc_identification(emm_ctx->ueid,
                                   emm_ctx,
                                   EMM_IDENT_TYPE_IMSI,
                                   _emm_attach_identify,
                                   _emm_attach_release,
                                   _emm_attach_release);

      if (rc != RETURNok) {
        /* Failed to initiate the identification procedure */
        LOG_TRACE(WARNING, "EMM-PROC  - "
                  "Failed to initiate identification procedure");
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        /* Do not accept the UE to attach to the network */
        rc = _emm_attach_reject(emm_ctx);
      }

      /* Relevant callback will be executed when identification
       * procedure completes */
      LOG_FUNC_RETURN(rc);
    }
  } else if ( (emm_ctx->imei) && (emm_ctx->is_emergency) ) {
    /* The UE is attempting to attach to the network for emergency
     * services using an IMEI */
    rc = mme_api_identify_imei(emm_ctx->imei, &emm_ctx->vector);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-PROC  - "
                "Failed to identify the UE using provided IMEI");
      emm_ctx->emm_cause = EMM_CAUSE_IMEI_NOT_ACCEPTED;
    }
  } else {
    LOG_TRACE(WARNING, "EMM-PROC  - UE's identity is not available");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /*
   * GUTI reallocation
   * -----------------
   */
  if ( (rc != RETURNerror) && guti_reallocation ) {
    /* Release the old GUTI */
    if (emm_ctx->old_guti) {
      free(emm_ctx->old_guti);
    }

    /* Save the GUTI previously used by the UE to identify itself */
    emm_ctx->old_guti = emm_ctx->guti;
    /* Allocate a new GUTI */
    emm_ctx->guti = (GUTI_t *)malloc(sizeof(GUTI_t));
    /* Request the MME to assign a GUTI to the UE */
    rc = mme_api_new_guti(emm_ctx->imsi, emm_ctx->guti,
                          &emm_ctx->tac, &emm_ctx->n_tacs);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-PROC  - Failed to assign new GUTI");
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    } else {
      LOG_TRACE(WARNING, "EMM-PROC  - New GUTI assigned to the UE "
                "(tmsi=%u)", emm_ctx->guti->m_tmsi);
      /* Update the GUTI indicator as new */
      emm_ctx->guti_is_new = TRUE;
    }
  }

  /*
   * UE's authentication
   * -------------------
   */
  if (rc != RETURNerror) {
    if (emm_ctx->security) {
      /* A security context exists for the UE in the network;
       * proceed with the attach procedure.
       */
      rc = _emm_attach(emm_ctx);
    } else if ( (emm_ctx->is_emergency) &&
                (_emm_data.conf.features & MME_API_UNAUTHENTICATED_IMSI) ) {
      /* 3GPP TS 24.301, section 5.5.1.2.3
       * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
       * MME configured to support Emergency Attach for unauthenticated
       * IMSIs may choose to skip the authentication procedure even if
       * no EPS security context is available and proceed directly to the
       * execution of the security mode control procedure.
       */
      rc = _emm_attach_security(emm_ctx);
    }

#if !defined(NAS_BUILT_IN_EPC)
    else {
      /* 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
       * No EMM context exists for the UE in the network; authentication
       * and NAS security setup to activate integrity protection and NAS
       * ciphering are mandatory.
       */
      auth_vector_t *auth = &emm_ctx->vector;
      const OctetString loc_rand = {AUTH_RAND_SIZE, (uint8_t *)auth->rand};
      const OctetString autn = {AUTH_AUTN_SIZE, (uint8_t *)auth->autn};
      rc = emm_proc_authentication(emm_ctx, emm_ctx->ueid, 0, // TODO: eksi != 0
                                   &loc_rand, &autn,
                                   _emm_attach_security,
                                   _emm_attach_release,
                                   _emm_attach_release);

      if (rc != RETURNok) {
        /* Failed to initiate the authentication procedure */
        LOG_TRACE(WARNING, "EMM-PROC  - "
                  "Failed to initiate authentication procedure");
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      }
    }

#endif
  }

  if (rc != RETURNok) {
    /* Do not accept the UE to attach to the network */
    rc = _emm_attach_reject(emm_ctx);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:        _emm_attach_security()                                    **
 **                                                                        **
 ** Description: Initiates security mode control EMM common procedure.     **
 **                                                                        **
 ** Inputs:          args:      security argument parameters               **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
#if defined(NAS_BUILT_IN_EPC)
int emm_attach_security(void *args)
{
  return _emm_attach_security(args);
}
#endif

static int _emm_attach_security(void *args)
{
  LOG_FUNC_IN;

  int rc;
  emm_data_context_t *emm_ctx = (emm_data_context_t *)(args);

  LOG_TRACE(INFO, "EMM-PROC  - Setup NAS security (ueid="NAS_UE_ID_FMT")", emm_ctx->ueid);

  /* Create new NAS security context */
  if (emm_ctx->security == NULL) {
    emm_ctx->security =
      (emm_security_context_t *)malloc(sizeof(emm_security_context_t));
  }

  if (emm_ctx->security) {
    memset(emm_ctx->security, 0, sizeof(emm_security_context_t));
    emm_ctx->security->type = EMM_KSI_NOT_AVAILABLE;
    emm_ctx->security->selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
    emm_ctx->security->selected_algorithms.integrity  = NAS_SECURITY_ALGORITHMS_EIA0;
  } else {
    LOG_TRACE(WARNING, "EMM-PROC  - Failed to create security context");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /* Do not accept the UE to attach to the network */
    rc = _emm_attach_reject(emm_ctx);
    LOG_FUNC_RETURN(rc);
  }

  /* Initialize the security mode control procedure */
  rc = emm_proc_security_mode_control(emm_ctx->ueid, 0, // TODO: eksi != 0
                                      emm_ctx->eea, emm_ctx->eia,emm_ctx->ucs2,
                                      emm_ctx->uea, emm_ctx->uia, emm_ctx->gea,
                                      emm_ctx->umts_present, emm_ctx->gprs_present,
                                      _emm_attach, _emm_attach_release,
                                      _emm_attach_release);

  if (rc != RETURNok) {
    /* Failed to initiate the security mode control procedure */
    LOG_TRACE(WARNING, "EMM-PROC  - "
              "Failed to initiate security mode control procedure");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /* Do not accept the UE to attach to the network */
    rc = _emm_attach_reject(emm_ctx);
  }

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 *              MME specific local functions
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach()                                             **
 **                                                                        **
 ** Description: Performs the attach signalling procedure while a context  **
 **      exists for the incoming UE in the network.                **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.4                         **
 **      Upon receiving the ATTACH REQUEST message, the MME shall  **
 **      send an ATTACH ACCEPT message to the UE and start timer   **
 **      T3450.                                                    **
 **                                                                        **
 ** Inputs:  args:      attach argument parameters                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach(void *args)
{
  LOG_FUNC_IN;

  esm_sap_t esm_sap;
  int rc;

  emm_data_context_t *emm_ctx = (emm_data_context_t *)(args);

  LOG_TRACE(INFO, "EMM-PROC  - Attach UE (ueid="NAS_UE_ID_FMT")", emm_ctx->ueid);

  /* 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
   * At this point, all NAS messages shall be protected by the NAS security
   * functions (integrity and ciphering) indicated by the MME unless the UE
   * is emergency attached and not successfully authenticated.
   */

  /*
   * Notify ESM that PDN connectivity is requested
   */
  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
  esm_sap.is_standalone = FALSE;
  esm_sap.ueid = emm_ctx->ueid;
  esm_sap.ctx  = emm_ctx;
  esm_sap.recv = &emm_ctx->esm_msg;
  rc = esm_sap_send(&esm_sap);

  if ( (rc != RETURNerror) && (esm_sap.err == ESM_SAP_SUCCESS) ) {
    /*
     * The attach request is accepted by the network
     */

    /* Delete the stored UE radio capability information, if any */
    /* Store the UE network capability */
    /* Assign the TAI list the UE is registered to */

    /* Allocate parameters of the retransmission timer callback */
    attach_data_t *data = (attach_data_t *)calloc(1,sizeof(attach_data_t));

    if (data != NULL) {
      /* Setup ongoing EMM procedure callback functions */
      rc = emm_proc_common_initialize(emm_ctx->ueid, NULL, NULL, NULL,
                                      _emm_attach_abort, data);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING,
                  "Failed to initialize EMM callback functions");
        free(data);
        LOG_FUNC_RETURN (RETURNerror);
      }

      /* Set the UE identifier */
      data->ueid = emm_ctx->ueid;
      /* Reset the retransmission counter */
      data->retransmission_count = 0;
#if defined(ORIGINAL_CODE)
      /* Setup the ESM message container */
      data->esm_msg.value = (uint8_t *)malloc(esm_sap.send.length);

      if (data->esm_msg.value) {
        data->esm_msg.length = esm_sap.send.length;
        memcpy(data->esm_msg.value, esm_sap.send.value,
               esm_sap.send.length);
      } else {
        data->esm_msg.length = 0;
      }

      /* Send attach accept message to the UE */
      rc = _emm_attach_accept(emm_ctx, data);

      if (rc != RETURNerror) {
        if (emm_ctx->guti_is_new && emm_ctx->old_guti) {
          /* Implicit GUTI reallocation;
           * Notify EMM that common procedure has been initiated
           */
          emm_sap_t emm_sap;
          emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
          emm_sap.u.emm_reg.ueid = data->ueid;
          rc = emm_sap_send(&emm_sap);
        }
      }

#else
      rc = RETURNok;
#endif
    }
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /*
     * The attach procedure failed due to an ESM procedure failure
     */
    emm_ctx->emm_cause = EMM_CAUSE_ESM_FAILURE;

    /* Setup the ESM message container to include PDN Connectivity Reject
     * message within the Attach Reject message */
    if (emm_ctx->esm_msg.length > 0) {
      free(emm_ctx->esm_msg.value);
    }

    emm_ctx->esm_msg.value = (uint8_t *)malloc(esm_sap.send.length);

    if (emm_ctx->esm_msg.value) {
      emm_ctx->esm_msg.length = esm_sap.send.length;
      memcpy(emm_ctx->esm_msg.value, esm_sap.send.value,
             esm_sap.send.length);
      /* Send Attach Reject message */
      rc = _emm_attach_reject(emm_ctx);
    } else {
      emm_ctx->esm_msg.length = 0;
    }
  } else {
    /*
     * ESM procedure failed and, received message has been discarded or
     * Status message has been returned; ignore ESM procedure failure
     */
    rc = RETURNok;
  }

  if (rc != RETURNok) {
    /* The attach procedure failed */
    LOG_TRACE(WARNING, "EMM-PROC  - Failed to respond to Attach Request");
    emm_ctx->emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    /* Do not accept the UE to attach to the network */
    rc = _emm_attach_reject(emm_ctx);
  }

  LOG_FUNC_RETURN (rc);
}

int emm_cn_wrapper_attach_accept(emm_data_context_t *emm_ctx, void *data)
{
  return _emm_attach_accept(emm_ctx,(attach_data_t *)data);
}
/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_accept()                                      **
 **                                                                        **
 ** Description: Sends ATTACH ACCEPT message and start timer T3450         **
 **                                                                        **
 ** Inputs:  data:      Attach accept retransmission data          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3450                                      **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_accept(emm_data_context_t *emm_ctx, attach_data_t *data)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  // may be caused by timer not stopped when deleted context
  if (emm_ctx) {
    /*
     * Notify EMM-AS SAP that Attach Accept message together with an Activate
     * Default EPS Bearer Context Request message has to be sent to the UE
     */

    emm_sap.primitive = EMMAS_ESTABLISH_CNF;
    emm_sap.u.emm_as.u.establish.ueid = emm_ctx->ueid;

    if (emm_ctx->guti_is_new && emm_ctx->old_guti) {
      /* Implicit GUTI reallocation;
       * include the new assigned GUTI in the Attach Accept message  */
      LOG_TRACE(INFO,"EMM-PROC  - Implicit GUTI reallocation, include the new assigned GUTI in the Attach Accept message");
      emm_sap.u.emm_as.u.establish.UEid.guti = emm_ctx->old_guti;
      emm_sap.u.emm_as.u.establish.new_guti  = emm_ctx->guti;
    } else if (emm_ctx->guti_is_new && emm_ctx->guti) {
      /* include the new assigned GUTI in the Attach Accept message  */
      LOG_TRACE(INFO,"EMM-PROC  - Include the new assigned GUTI in the Attach Accept message");
      emm_sap.u.emm_as.u.establish.UEid.guti = emm_ctx->guti;
      emm_sap.u.emm_as.u.establish.new_guti  = emm_ctx->guti;
    } else {
      emm_sap.u.emm_as.u.establish.UEid.guti = emm_ctx->guti;
#warning "TEST LG FORCE GUTI IE IN ATTACH ACCEPT"
      emm_sap.u.emm_as.u.establish.new_guti  = emm_ctx->guti;
      //emm_sap.u.emm_as.u.establish.new_guti  = NULL;
    }

    emm_sap.u.emm_as.u.establish.n_tacs  = emm_ctx->n_tacs;
    emm_sap.u.emm_as.u.establish.tac     = emm_ctx->tac;
    emm_sap.u.emm_as.u.establish.NASinfo = EMM_AS_NAS_INFO_ATTACH;
    /* Setup EPS NAS security data */
    emm_as_set_security_data(&emm_sap.u.emm_as.u.establish.sctx,
                           emm_ctx->security, FALSE, TRUE);

    LOG_TRACE(INFO,"EMM-PROC  - encryption = 0x%X ", emm_sap.u.emm_as.u.establish.encryption);
    LOG_TRACE(INFO,"EMM-PROC  - integrity  = 0x%X ", emm_sap.u.emm_as.u.establish.integrity);
    emm_sap.u.emm_as.u.establish.encryption = emm_ctx->security->selected_algorithms.encryption;
    emm_sap.u.emm_as.u.establish.integrity  = emm_ctx->security->selected_algorithms.integrity;
    LOG_TRACE(INFO,"EMM-PROC  - encryption = 0x%X (0x%X)",
            emm_sap.u.emm_as.u.establish.encryption,
            emm_ctx->security->selected_algorithms.encryption);
    LOG_TRACE(INFO,"EMM-PROC  - integrity  = 0x%X (0x%X)",
            emm_sap.u.emm_as.u.establish.integrity,
            emm_ctx->security->selected_algorithms.integrity);

  /* Get the activate default EPS bearer context request message to
   * transfer within the ESM container of the attach accept message */
    emm_sap.u.emm_as.u.establish.NASmsg = data->esm_msg;
    LOG_TRACE(INFO,"EMM-PROC  - NASmsg  src size = %d NASmsg  dst size = %d ",
            data->esm_msg.length, emm_sap.u.emm_as.u.establish.NASmsg.length);

    rc = emm_sap_send(&emm_sap);

    if (rc != RETURNerror) {
      if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
        /* Re-start T3450 timer */
    	emm_ctx->T3450.id = nas_timer_restart(emm_ctx->T3450.id);
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3450 restarted UE "NAS_UE_ID_FMT"", data->ueid);
      } else {
        /* Start T3450 timer */
    	emm_ctx->T3450.id = nas_timer_start(emm_ctx->T3450.sec, _emm_attach_t3450_handler, data);
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3450 started UE "NAS_UE_ID_FMT" ", data->ueid);
      }

      LOG_TRACE(INFO,"EMM-PROC  - Timer T3450 (%d) expires in %ld seconds",
    		  emm_ctx->T3450.id, emm_ctx->T3450.sec);
    }
  } else {
    LOG_TRACE(WARNING,"EMM-PROC  - emm_ctx NULL");
  }
  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_have_changed()                                **
 **                                                                        **
 ** Description: Check whether the given attach parameters differs from    **
 **      those previously stored when the attach procedure has     **
 **      been initiated.                                           **
 **                                                                        **
 ** Inputs:  ctx:       EMM context of the UE in the network       **
 **      type:      Type of the requested attach               **
 **      ksi:       Security ket sey identifier                **
 **      guti:      The GUTI provided by the UE                **
 **      imsi:      The IMSI provided by the UE                **
 **      imei:      The IMEI provided by the UE                **
 **      eea:       Supported EPS encryption algorithms        **
 **      eia:       Supported EPS integrity algorithms         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if at least one of the parameters     **
 **             differs; FALSE otherwise.                  **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_have_changed(const emm_data_context_t *ctx,
                                    emm_proc_attach_type_t type, int ksi,
                                    GUTI_t *guti, imsi_t *imsi, imei_t *imei,
                                    int eea, int eia, int ucs2, int uea, int uia, int gea,
                                    int umts_present, int gprs_present)
{
  LOG_FUNC_IN;

  /* Emergency bearer services indicator */
  if ( (type == EMM_ATTACH_TYPE_EMERGENCY) != ctx->is_emergency) {
    LOG_FUNC_RETURN (TRUE);
  }

  /* Security key set identifier */
  if (ksi != ctx->ksi) {
    LOG_FUNC_RETURN (TRUE);
  }

  /* Supported EPS encryption algorithms */
  if (eea != ctx->eea) {
    LOG_FUNC_RETURN (TRUE);
  }

  /* Supported EPS integrity algorithms */
  if (eia != ctx->eia) {
    LOG_FUNC_RETURN (TRUE);
  }

  if (umts_present != ctx->umts_present) {
    LOG_FUNC_RETURN (TRUE);
  }

  if (ctx->umts_present) {
    if (ucs2 != ctx->ucs2) {
      LOG_FUNC_RETURN (TRUE);
    }

    /* Supported UMTS encryption algorithms */
    if (uea != ctx->uea) {
      LOG_FUNC_RETURN (TRUE);
    }

    /* Supported UMTS integrity algorithms */
    if (uia != ctx->uia) {
      LOG_FUNC_RETURN (TRUE);
    }
  }

  if (gprs_present != ctx->gprs_present) {
    LOG_FUNC_RETURN (TRUE);
  }

  if (ctx->gprs_present) {
    if (gea != ctx->gea) {
      LOG_FUNC_RETURN (TRUE);
    }
  }

  /* The GUTI if provided by the UE */
  if ( (guti) && (ctx->guti == NULL) ) {
    LOG_FUNC_RETURN (TRUE);
  }

  if ( (guti == NULL) && (ctx->guti) ) {
    LOG_FUNC_RETURN (TRUE);
  }

  if ( (guti) && (ctx->guti) ) {
    if (guti->m_tmsi != ctx->guti->m_tmsi) {
      LOG_FUNC_RETURN (TRUE);
    }

    if ( memcmp(&guti->gummei, &ctx->guti->gummei, sizeof(gummei_t)) != 0 ) {
      LOG_FUNC_RETURN (TRUE);
    }
  }

  /* The IMSI if provided by the UE */
  if ( (imsi) && (ctx->imsi == NULL) ) {
    LOG_FUNC_RETURN (TRUE);
  }

  if ( (imsi == NULL) && (ctx->imsi) ) {
    LOG_FUNC_RETURN (TRUE);
  }

  if ( (imsi) && (ctx->imsi) ) {
    if ( memcmp(imsi, ctx->imsi, sizeof(imsi_t)) != 0 ) {
      LOG_FUNC_RETURN (TRUE);
    }
  }

  /* The IMEI if provided by the UE */
  if ( (imei) && (ctx->imei == NULL) ) {
    LOG_FUNC_RETURN (TRUE);
  }

  if ( (imei == NULL) && (ctx->imei) ) {
    LOG_FUNC_RETURN (TRUE);
  }

  if ( (imei) && (ctx->imei) ) {
    if ( memcmp(imei, ctx->imei, sizeof(imei_t)) != 0 ) {
      LOG_FUNC_RETURN (TRUE);
    }
  }

  LOG_FUNC_RETURN (FALSE);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_update()                                      **
 **                                                                        **
 ** Description: Update the EMM context with the given attach procedure    **
 **      parameters.                                               **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      type:      Type of the requested attach               **
 **      ksi:       Security ket sey identifier                **
 **      guti:      The GUTI provided by the UE                **
 **      imsi:      The IMSI provided by the UE                **
 **      imei:      The IMEI provided by the UE                **
 **      eea:       Supported EPS encryption algorithms        **
 **      eia:       Supported EPS integrity algorithms         **
 **      esm_msg_pP:   ESM message contained with the attach re-  **
 **             quest                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     ctx:       EMM context of the UE in the network       **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_update(emm_data_context_t *ctx, unsigned int ueid,
                              emm_proc_attach_type_t type, int ksi,
                              GUTI_t *guti, imsi_t *imsi, imei_t *imei,
                              int eea, int eia, int ucs2, int uea, int uia, int gea,
                              int umts_present, int gprs_present,
                              const OctetString *esm_msg_pP)
{
  int mnc_length;
  LOG_FUNC_IN;
  /* UE identifier */
  ctx->ueid = ueid;
  /* Emergency bearer services indicator */
  ctx->is_emergency = (type == EMM_ATTACH_TYPE_EMERGENCY);
  /* Security key set identifier */
  ctx->ksi  = ksi;
  /* Supported EPS encryption algorithms */
  ctx->eea  = eea;
  /* Supported EPS integrity algorithms */
  ctx->eia  = eia;
  ctx->ucs2 = ucs2;
  ctx->uea  = uea;
  ctx->uia  = uia;
  ctx->gea  = gea;
  ctx->umts_present  = umts_present;
  ctx->gprs_present  = gprs_present;

  /* The GUTI if provided by the UE */
  if (guti) {
    LOG_TRACE(INFO, "EMM-PROC  - GUTI NOT NULL");

    if (ctx->guti == NULL) {
      ctx->guti = (GUTI_t *)malloc(sizeof(GUTI_t));
    }

    if (ctx->guti != NULL) {
      memcpy(ctx->guti, guti, sizeof(GUTI_t));
    } else {
      LOG_FUNC_RETURN (RETURNerror);
    }
  } else {
    if (ctx->guti == NULL) {
      ctx->guti = (GUTI_t *)calloc(1, sizeof(GUTI_t));
    }

#warning "LG: We should assign the GUTI accordingly to the visited plmn id"

    if ((ctx->guti != NULL) && (imsi)) {
      ctx->tac                         = mme_config.gummei.plmn_tac[0];
      ctx->guti->gummei.MMEcode        = mme_config.gummei.mmec[0];
      ctx->guti->gummei.MMEgid         = mme_config.gummei.mme_gid[0];
      ctx->guti->m_tmsi                = (uint32_t) ctx;

      mnc_length =  mme_config_find_mnc_length(
                      imsi->u.num.digit1,
                      imsi->u.num.digit2,
                      imsi->u.num.digit3,
                      imsi->u.num.digit4,
                      imsi->u.num.digit5,
                      imsi->u.num.digit6);

      if ((mnc_length == 2) || (mnc_length == 3)) {
        ctx->guti->gummei.plmn.MCCdigit1 = imsi->u.num.digit1;
        ctx->guti->gummei.plmn.MCCdigit2 = imsi->u.num.digit2;
        ctx->guti->gummei.plmn.MCCdigit3 = imsi->u.num.digit3;

        if (mnc_length == 2) {
          ctx->guti->gummei.plmn.MNCdigit1 = imsi->u.num.digit4;
          ctx->guti->gummei.plmn.MNCdigit2 = imsi->u.num.digit5;
          ctx->guti->gummei.plmn.MNCdigit3 = 15;
          LOG_TRACE(WARNING, "EMM-PROC  - Assign GUTI from IMSI %01X%01X%01X.%01X%01X.%04X.%02X.%08X to emm_data_context",
                    ctx->guti->gummei.plmn.MCCdigit1,
                    ctx->guti->gummei.plmn.MCCdigit2,
                    ctx->guti->gummei.plmn.MCCdigit3,
                    ctx->guti->gummei.plmn.MNCdigit1,
                    ctx->guti->gummei.plmn.MNCdigit2,
                    ctx->guti->gummei.MMEgid,
                    ctx->guti->gummei.MMEcode,
                    ctx->guti->m_tmsi
                   );
        } else {
          ctx->guti->gummei.plmn.MNCdigit1 = imsi->u.num.digit5;
          ctx->guti->gummei.plmn.MNCdigit2 = imsi->u.num.digit6;
          ctx->guti->gummei.plmn.MNCdigit3 = imsi->u.num.digit4;
          LOG_TRACE(WARNING, "EMM-PROC  - Assign GUTI from IMSI %01X%01X%01X.%01X%01X%01X.%04X.%02X.%08X to emm_data_context",
                    ctx->guti->gummei.plmn.MCCdigit1,
                    ctx->guti->gummei.plmn.MCCdigit2,
                    ctx->guti->gummei.plmn.MCCdigit3,
                    ctx->guti->gummei.plmn.MNCdigit1,
                    ctx->guti->gummei.plmn.MNCdigit2,
                    ctx->guti->gummei.plmn.MNCdigit3,
                    ctx->guti->gummei.MMEgid,
                    ctx->guti->gummei.MMEcode,
                    ctx->guti->m_tmsi
                   );
        }

        LOG_TRACE(WARNING, "EMM-PROC  - Set ctx->guti_is_new to emm_data_context");
        ctx->guti_is_new                 = TRUE;
      } else {
        LOG_FUNC_RETURN (RETURNerror);
      }
    } else {
      LOG_FUNC_RETURN (RETURNerror);
    }
  }

  /* The IMSI if provided by the UE */
  if (imsi) {
    if (ctx->imsi == NULL) {
      ctx->imsi = (imsi_t *)malloc(sizeof(imsi_t));
    }

    if (ctx->imsi != NULL) {
      memcpy(ctx->imsi, imsi, sizeof(imsi_t));
    } else {
      LOG_FUNC_RETURN (RETURNerror);
    }
  }

  /* The IMEI if provided by the UE */
  if (imei) {
    if (ctx->imei == NULL) {
      ctx->imei = (imei_t *)malloc(sizeof(imei_t));
    }

    if (ctx->imei != NULL) {
      memcpy(ctx->imei, imei, sizeof(imei_t));
    } else {
      LOG_FUNC_RETURN (RETURNerror);
    }
  }

  /* The ESM message contained within the attach request */
  if (esm_msg_pP->length > 0) {
    if (ctx->esm_msg.value != NULL) {
      free(ctx->esm_msg.value);
      ctx->esm_msg.value  = NULL;
      ctx->esm_msg.length = 0;
    }

    ctx->esm_msg.value = (uint8_t *)malloc(esm_msg_pP->length);

    if (ctx->esm_msg.value != NULL) {
      memcpy((char *)ctx->esm_msg.value,
              (char *)esm_msg_pP->value, esm_msg_pP->length);

    } else {
      LOG_FUNC_RETURN (RETURNerror);
    }
  }

  ctx->esm_msg.length = esm_msg_pP->length;
  /* Attachment indicator */
  ctx->is_attached = FALSE;

  LOG_FUNC_RETURN (RETURNok);
}
