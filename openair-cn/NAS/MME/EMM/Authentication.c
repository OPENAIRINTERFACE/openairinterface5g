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
Source      Authentication.c

Version     0.1

Date        2013/03/04

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the authentication EMM procedure executed by the
        Non-Access Stratum.

        The purpose of the EPS authentication and key agreement (AKA)
        procedure is to provide mutual authentication between the user
        and the network and to agree on a key KASME. The procedure is
        always initiated and controlled by the network. However, the
        UE can reject the EPS authentication challenge sent by the
        network.

        A partial native EPS security context is established in the
        UE and the network when an EPS authentication is successfully
        performed. The computed key material KASME is used as the
        root for the EPS integrity protection and ciphering key
        hierarchy.

*****************************************************************************/

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy, memcmp, memset
#include <arpa/inet.h> // htons

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"
#include "emmData.h"
#include "emm_sap.h"
#include "emm_cause.h"

# if defined(NAS_BUILT_IN_EPC)
#   include "nas_itti_messaging.h"
# endif
#include "msc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the authentication procedure in the UE
 * --------------------------------------------------------------------------
 */

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the authentication procedure in the MME
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
static void *_authentication_t3460_handler(void *);

/*
 * Function executed whenever the ongoing EMM procedure that initiated
 * the authentication procedure is aborted or the maximum value of the
 * retransmission timer counter is exceed
 */
static int _authentication_abort(void *);

/*
 * Internal data used for authentication procedure
 */
typedef struct {
  unsigned int ueid;          /* UE identifier        */
#define AUTHENTICATION_COUNTER_MAX  5
  unsigned int retransmission_count;  /* Retransmission counter   */
  int ksi;                /* NAS key set identifier   */
  OctetString rand;           /* Random challenge number  */
  OctetString autn;           /* Authentication token     */
  int notify_failure;         /* Indicates whether the authentication
                     * procedure failure shall be notified
                     * to the ongoing EMM procedure */
} authentication_data_t;

static int _authentication_request(authentication_data_t *data);
static int _authentication_reject(unsigned int ueid);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
 * --------------------------------------------------------------------------
 *      Authentication procedure executed by the MME
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication()                                 **
 **                                                                        **
 ** Description: Initiates authentication procedure to establish partial   **
 **      native EPS security context in the UE and the MME.        **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.2                           **
 **      The network initiates the authentication procedure by     **
 **      sending an AUTHENTICATION REQUEST message to the UE and   **
 **      starting the timer T3460. The AUTHENTICATION REQUEST mes- **
 **      sage contains the parameters necessary to calculate the   **
 **      authentication response.                                  **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      ksi:       NAS key set identifier                     **
 **      rand:      Random challenge number                    **
 **      autn:      Authentication token                       **
 **      success:   Callback function executed when the authen-**
 **             tication procedure successfully completes  **
 **      reject:    Callback function executed when the authen-**
 **             tication procedure fails or is rejected    **
 **      failure:   Callback function executed whener a lower  **
 **             layer failure occured before the authenti- **
 **             cation procedure comnpletes                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication(void *ctx, unsigned int ueid, int ksi,
                            const OctetString *_rand, const OctetString *autn,
                            emm_common_success_callback_t success,
                            emm_common_reject_callback_t reject,
                            emm_common_failure_callback_t failure)

{
  int rc = RETURNerror;
  authentication_data_t *data;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMM-PROC  - Initiate authentication KSI = %d, ctx = %p", ksi, ctx);

  /* Allocate parameters of the retransmission timer callback */
  data = (authentication_data_t *)malloc(sizeof(authentication_data_t));

  if (data != NULL) {
    /* Setup ongoing EMM procedure callback functions */
    rc = emm_proc_common_initialize(ueid, success, reject, failure,
                                    _authentication_abort, data);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "Failed to initialize EMM callback functions");
      LOG_FUNC_RETURN (RETURNerror);
    }

    /* Set the UE identifier */
    data->ueid = ueid;
    /* Reset the retransmission counter */
    data->retransmission_count = 0;
    /* Set the key set identifier */
    data->ksi = ksi;

    /* Set the authentication random challenge number */
    if (_rand->length > 0) {
      data->rand.value = (uint8_t *)malloc(_rand->length);
      data->rand.length = 0;

      if (data->rand.value) {
        memcpy(data->rand.value, _rand->value, _rand->length);
        data->rand.length = _rand->length;
      }
    }

    /* Set the authentication token */
    if (autn->length > 0) {
      data->autn.value = (uint8_t *)malloc(autn->length);
      data->autn.length = 0;

      if (data->autn.value) {
        memcpy(data->autn.value, autn->value, autn->length);
        data->autn.length = autn->length;
      }
    }

    /* Set the failure notification indicator */
    data->notify_failure = FALSE;
    /* Send authentication request message to the UE */
    rc = _authentication_request(data);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that common procedure has been initiated
       */


      MSC_LOG_TX_MESSAGE(
    		MSC_NAS_EMM_MME,
    	  	MSC_NAS_EMM_MME,
    	  	NULL,0,
    	  	"0 EMMREG_COMMON_PROC_REQ ue id "NAS_UE_ID_FMT" (authentication)", ueid);

      emm_sap_t emm_sap;
      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ueid = ueid;
      emm_sap.u.emm_reg.ctx  = ctx;
      rc = emm_sap_send(&emm_sap);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication_complete()                            **
 **                                                                        **
 ** Description: Performs the authentication completion procedure executed **
 **      by the network.                                                   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.4                           **
 **      Upon receiving the AUTHENTICATION RESPONSE message, the           **
 **      MME shall stop timer T3460 and check the correctness of           **
 **      the RES parameter.                                                **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                          **
 **      emm_cause: Authentication failure EMM cause code                  **
 **      res:       Authentication response parameter. or auts             **
 **                 in case of sync failure                                **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    _emm_data, T3460                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication_complete(unsigned int ueid, int emm_cause,
                                     const OctetString *res)
{
  int rc;
  emm_sap_t emm_sap;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMM-PROC  - Authentication complete (ueid="NAS_UE_ID_FMT", cause=%d)",
            ueid, emm_cause);


  /* Release retransmission timer paramaters */
  authentication_data_t *data =
    (authentication_data_t *)(emm_proc_common_get_args(ueid));

  if (data) {
    if (data->rand.length > 0) {
      free(data->rand.value);
    }

    if (data->autn.length > 0) {
      free(data->autn.value);
    }

    free(data);
  }

  /* Get the UE context */
  emm_data_context_t *emm_ctx = NULL;
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
	/* Stop timer T3460 */
	LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3460 (%d)", emm_ctx->T3460.id);
	emm_ctx->T3460.id = nas_timer_stop(emm_ctx->T3460.id);
	MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3460 stopped UE "NAS_UE_ID_FMT" ", data->ueid);
  }
  if (emm_cause == EMM_CAUSE_SUCCESS) {
    /* Check the received RES parameter */
    if ( (emm_ctx == NULL) ||
         (memcmp(res->value, &emm_ctx->vector.xres, res->length) != 0) ) {
      /* RES does not match the XRES parameter */
      LOG_TRACE(WARNING, "EMM-PROC  - Failed to authentify the UE");
      emm_cause = EMM_CAUSE_ILLEGAL_UE;
    } else {
      LOG_TRACE(DEBUG, "EMM-PROC  - Success to authentify the UE  RESP XRES == XRES UE CONTEXT");
    }
  }

  if (emm_cause != EMM_CAUSE_SUCCESS) {
    switch (emm_cause) {

#if defined(NAS_BUILT_IN_EPC)

    case EMM_CAUSE_SYNCH_FAILURE:
      /* USIM has detected a mismatch in SQN.
       * Ask for a new vector.
       */
      MSC_LOG_EVENT(
      		MSC_NAS_EMM_MME,
      	  	"0 SQN SYNCH_FAILURE ue id "NAS_UE_ID_FMT" ", ueid);

      LOG_TRACE(DEBUG, "EMM-PROC  - USIM has detected a mismatch in SQN Ask for a new vector");
      nas_itti_auth_info_req(ueid, emm_ctx->imsi, 0, res->value);

      rc = RETURNok;
      LOG_FUNC_RETURN (rc);
      break;
#endif

    default:
      LOG_TRACE(DEBUG, "EMM-PROC  - The MME received an authentication failure message or the RES does not match the XRES parameter computed by the network");
      /* The MME received an authentication failure message or the RES
       * contained in the Authentication Response message received from
       * the UE does not match the XRES parameter computed by the network */
      (void) _authentication_reject(ueid);
      /*
       * Notify EMM that the authentication procedure failed
       */
      MSC_LOG_TX_MESSAGE(
    		MSC_NAS_EMM_MME,
    	  	MSC_NAS_EMM_MME,
    	  	NULL,0,
    	  	"0 EMMREG_COMMON_PROC_REJ ue id "NAS_UE_ID_FMT" ", ueid);


      emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      emm_sap.u.emm_reg.ueid = ueid;
      emm_sap.u.emm_reg.ctx  = emm_ctx;
      break;
    }
  } else {
    /*
     * Notify EMM that the authentication procedure successfully completed
     */
    MSC_LOG_TX_MESSAGE(
    		MSC_NAS_EMM_MME,
    	  	MSC_NAS_EMM_MME,
    	  	NULL,0,
    	  	"0 EMMREG_COMMON_PROC_CNF ue id "NAS_UE_ID_FMT" ", ueid);

    LOG_TRACE(DEBUG, "EMM-PROC  - Notify EMM that the authentication procedure successfully completed");
    emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
    emm_sap.u.emm_reg.ueid = ueid;
    emm_sap.u.emm_reg.ctx  = emm_ctx;
    emm_sap.u.emm_reg.u.common.is_attached = emm_ctx->is_attached;
  }

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
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
 ** Name:    _authentication_t3460_handler()                           **
 **                                                                        **
 ** Description: T3460 timeout handler                                     **
 **      Upon T3460 timer expiration, the authentication request   **
 **      message is retransmitted and the timer restarted. When    **
 **      retransmission counter is exceed, the MME shall abort the **
 **      authentication procedure and any ongoing EMM specific     **
 **      procedure and release the NAS signalling connection.      **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.7, case b                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_authentication_t3460_handler(void *args)
{
  LOG_FUNC_IN;

  int rc;

  authentication_data_t *data = (authentication_data_t *)(args);

  /* Increment the retransmission counter */
  data->retransmission_count += 1;

  LOG_TRACE(WARNING, "EMM-PROC  - T3460 timer expired, retransmission "
            "counter = %d", data->retransmission_count);

  if (data->retransmission_count < AUTHENTICATION_COUNTER_MAX) {
    /* Send authentication request message to the UE */
    rc = _authentication_request(data);
  } else {
    unsigned int ueid = data->ueid;
    /* Set the failure notification indicator */
    data->notify_failure = TRUE;
    /* Abort the authentication procedure */
    rc = _authentication_abort(data);

    /* Release the NAS signalling connection */
    if (rc != RETURNerror) {
      emm_sap_t emm_sap;
      emm_sap.primitive = EMMAS_RELEASE_REQ;
      emm_sap.u.emm_as.u.release.guti = NULL;
      emm_sap.u.emm_as.u.release.ueid = ueid;
      emm_sap.u.emm_as.u.release.cause = EMM_AS_CAUSE_AUTHENTICATION;
      rc = emm_sap_send(&emm_sap);
    }
  }

  LOG_FUNC_RETURN (NULL);
}

/*
 * --------------------------------------------------------------------------
 *              MME specific local functions
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_request()                                 **
 **                                                                        **
 ** Description: Sends AUTHENTICATION REQUEST message and start timer T3460**
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
int _authentication_request(authentication_data_t *data)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;
  struct emm_data_context_s *emm_ctx;

  /*
   * Notify EMM-AS SAP that Authentication Request message has to be sent
   * to the UE
   */
  emm_sap.primitive = EMMAS_SECURITY_REQ;
  emm_sap.u.emm_as.u.security.guti = NULL;
  emm_sap.u.emm_as.u.security.ueid = data->ueid;
  emm_sap.u.emm_as.u.security.msgType = EMM_AS_MSG_TYPE_AUTH;
  emm_sap.u.emm_as.u.security.ksi = data->ksi;
  emm_sap.u.emm_as.u.security.rand = &data->rand;
  emm_sap.u.emm_as.u.security.autn = &data->autn;

  /* TODO: check for pointer validity */
#if defined(NAS_BUILT_IN_EPC)
  emm_ctx = emm_data_context_get(&_emm_data, data->ueid);
#else
  emm_ctx = _emm_data.ctx[data->ueid];
#endif

  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.security.sctx,
                           emm_ctx->security, FALSE, TRUE);

  MSC_LOG_TX_MESSAGE(
		  MSC_NAS_EMM_MME,
		  MSC_NAS_EMM_MME,
		  NULL,0,
		  "0 EMMAS_SECURITY_REQ ue id "NAS_UE_ID_FMT" ", data->ueid);

  rc = emm_sap_send(&emm_sap);

  if (rc != RETURNerror) {
	if (emm_ctx) {
	  if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
        /* Re-start T3460 timer */
        emm_ctx->T3460.id = nas_timer_restart(emm_ctx->T3460.id);
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3460 restarted UE "NAS_UE_ID_FMT" ", data->ueid);
      } else {
        /* Start T3460 timer */
        emm_ctx->T3460.id = nas_timer_start(emm_ctx->T3460.sec, _authentication_t3460_handler,
                                 data);
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3460 started UE "NAS_UE_ID_FMT" ", data->ueid);
      }
    }

    LOG_TRACE(INFO,"EMM-PROC  - Timer T3460 (%d) expires in %ld seconds",
    		emm_ctx->T3460.id, emm_ctx->T3460.sec);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_reject()                                  **
 **                                                                        **
 ** Description: Sends AUTHENTICATION REJECT message                       **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _authentication_reject(unsigned int ueid)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;
  struct emm_data_context_s *emm_ctx;

  /*
   * Notify EMM-AS SAP that Authentication Reject message has to be sent
   * to the UE
   */
  emm_sap.primitive = EMMAS_SECURITY_REJ;
  emm_sap.u.emm_as.u.security.guti = NULL;
  emm_sap.u.emm_as.u.security.ueid = ueid;
  emm_sap.u.emm_as.u.security.msgType = EMM_AS_MSG_TYPE_AUTH;

#if defined(NAS_BUILT_IN_EPC)
  emm_ctx = emm_data_context_get(&_emm_data, ueid);
#else
  emm_ctx = _emm_data.ctx[ueid];
#endif
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.security.sctx,
                           emm_ctx->security, FALSE, TRUE);
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_abort()                                   **
 **                                                                        **
 ** Description: Aborts the authentication procedure currently in progress **
 **                                                                        **
 ** Inputs:  args:      Authentication data to be released         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_abort(void *args)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  struct emm_data_context_s *emm_ctx;

  authentication_data_t *data = (authentication_data_t *)(args);

  if (data) {
    unsigned int ueid = data->ueid;
    int notify_failure = data->notify_failure;

    LOG_TRACE(WARNING, "EMM-PROC  - Abort authentication procedure "
              "(ueid="NAS_UE_ID_FMT")", ueid);
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, ueid);
#else
    emm_ctx = _emm_data.ctx[ueid];
#endif
    if (emm_ctx) {
      /* Stop timer T3460 */
      if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
        LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3460 (%d)", emm_ctx->T3460.id);
        emm_ctx->T3460.id = nas_timer_stop(emm_ctx->T3460.id);
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 T3460 stopped UE "NAS_UE_ID_FMT" ", data->ueid);
      }
    }

    /* Release retransmission timer paramaters */
    if (data->rand.length > 0) {
      free(data->rand.value);
    }

    if (data->autn.length > 0) {
      free(data->autn.value);
    }

    free(data);

    /*
     * Notify EMM that the authentication procedure failed
     */
    if (notify_failure) {
      emm_sap_t emm_sap;
      emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      emm_sap.u.emm_reg.ueid = ueid;
      rc = emm_sap_send(&emm_sap);
    } else {
      rc = RETURNok;
    }
  }

  LOG_FUNC_RETURN (rc);
}


