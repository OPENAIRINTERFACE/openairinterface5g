/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
#include "emm_timers.h"

#include "emm_sap.h"
#include "emm_cause.h"
#include "emm_timers.h"

#include "usim_api.h"
#include "secu_defs.h"
#include "Authentication.h"
#include "targets/RT/USER/lte-softmodem.h"


/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the authentication procedure in the UE
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
static void *_authentication_t3416_handler(void *);
static void *_authentication_t3418_handler(void *);
static void *_authentication_t3420_handler(void *);

/*
 * Abnormal case authentication procedure
 */
static int _authentication_abnormal_cases_cde(nas_user_t *user, int emm_cause,
    const OctetString *auts);
static int _authentication_abnormal_case_f(nas_user_t *user);

static int _authentication_stop_timers(nas_user_t *user);
static int _authentication_start_timers(nas_user_t *user);
static int _authentication_kasme(const OctetString *autn,
                                 const OctetString *ck, const OctetString *ik, const plmn_t *plmn,
                                 OctetString *kasme);

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the authentication procedure in the MME
 * --------------------------------------------------------------------------
 */

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      Authentication procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication_request()                         **
 **                                                                        **
 ** Description: Performs the MME requested authentication procedure.      **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.3                           **
 **      Upon receiving the AUTHENTICATION REQUEST message, the UE **
 **      shall store the received RAND together with the RES re-   **
 **      turned from the USIM in the volatile memory of the ME, to **
 **      avoid a synchronisation failure. The UE shall process the **
 **      authentication challenge data and respond with an AUTHEN- **
 **      TICATION RESPONSE message to the network.                 **
 **                                                                        **
 ** Inputs:  native_ksi:    true if the security context is of type    **
 **             native (for KSIASME)                       **
 **      ksi:       The NAS ket sey identifier                 **
 **      rand:      Authentication parameter RAND              **
 **      autn:      Authentication parameter AUTN              **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **             T3418, T3420                               **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication_request(nas_user_t *user, int native_ksi, int ksi,
                                    const OctetString *rand,
                                    const OctetString *autn)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  LOG_TRACE(INFO, "EMM-PROC  - Authentication requested ksi type = %s, ksi = %d", native_ksi ? "native" : "mapped", ksi);

  /* 3GPP TS 24.301, section 5.4.2.1
   * The UE shall proceed with an EPS authentication challenge only if a
   * USIM is present
   */
  if (!user->emm_data->usim_is_valid) {
    LOG_TRACE(WARNING, "EMM-PROC  - USIM is not present or not valid");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Stop timer T3418, if running */
  if (emm_timers->T3418.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3418 (%d)", emm_timers->T3418.id);
    emm_timers->T3418.id = nas_timer_stop(emm_timers->T3418.id);
  }

  /* Stop timer T3420, if running */
  if (emm_timers->T3420.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3420 (%d)", emm_timers->T3420.id);
    emm_timers->T3420.id = nas_timer_stop(emm_timers->T3420.id);
  }

  /* Setup security keys */
  OctetString ck = {AUTH_CK_SIZE, authentication_data->ck};
  OctetString ik = {AUTH_IK_SIZE, authentication_data->ik};
  OctetString res = {AUTH_RES_SIZE, authentication_data->res};

  if ((memcmp(authentication_data->rand, rand->value, AUTH_CK_SIZE) != 0) ||
      (authentication_data->auth_process_started == false)) {
    /*
     * There is no valid stored RAND in the ME or the stored RAND is
     * different from the new received value in the AUTHENTICATION
     * REQUEST message OR if this is first time UE starting the
     * Authentication process
     */
	authentication_data->auth_process_started = true;
    OctetString auts;
    auts.length = 0;
    auts.value = (uint8_t *)malloc(AUTH_AUTS_SIZE);

    if (auts.value == NULL) {
      LOG_TRACE(WARNING, "EMM-PROC  - Failed to allocate AUTS parameter");
      LOG_FUNC_RETURN (RETURNerror);
    }

    /* 3GPP TS 33.401, section 6.1.1
     * Get the "separation bit" of the AMF field of AUTN */
    int sbit = AUTH_AMF_SEPARATION_BIT(autn->value[AUTH_AMF_INDEX]);

    if (sbit != 0) {
      /* LW: only 64 bits from the response field are used for the authentication response for this algorithms */
      res.length = 8; /* Bytes */

      /*
       * Perform EPS authentication challenge to check the authenticity
       * of the core network by means of the received AUTN parameter and
       * request the USIM to compute RES, CK and IK for given RAND
       */
      if(get_softmodem_params()->usim_test == 0)
      {
        rc = usim_api_authenticate(&user->usim_data, rand, autn, &auts, &res, &ck, &ik);
      }
      else
      {
        rc = usim_api_authenticate_test(&user->usim_data, rand, autn, &auts, &res, &ck, &ik);
      }
    }

    if (rc != RETURNok) {
      /*
       * Network authentication not accepted by the UE
       */
      LOG_TRACE(WARNING, "EMM-PROC  - Network authentication failed (%s)",
                (auts.length > 0) ? "SQN failure" :
                (sbit == 0) ? "Non-EPS authentication unacceptable" :
                "MAC code failure");
      /* Delete any previously stored RAND and RES and stop timer T3416 */
      emm_proc_authentication_delete(user);

      /* Proceed authentication abnormal cases procedure */
      if (auts.length > 0) {
        /* 3GPP TS 24.301, section 5.4.2.6, case e
         * SQN failure */
        rc = _authentication_abnormal_cases_cde(
               user, EMM_CAUSE_SYNCH_FAILURE, &auts);
      } else if (sbit == 0) {
        /* 3GPP TS 24.301, section 5.4.2.6, case d
         * Non-EPS authentication unacceptable */
        rc = _authentication_abnormal_cases_cde(
               user, EMM_CAUSE_NON_EPS_AUTH_UNACCEPTABLE, NULL);
      } else {
        /* 3GPP TS 24.301, section 5.4.2.6, case c
         * MAC code failure */
        rc = _authentication_abnormal_cases_cde(
               user, EMM_CAUSE_MAC_FAILURE, NULL);
      }

      /* Free the AUTS parameter */
      free(auts.value);
      LOG_FUNC_RETURN (rc);
    }

    /* Free the AUTS parameter */
    free(auts.value);

    /* Store the new RAND in the volatile memory */
    if (rand->length <= AUTH_RAND_SIZE) {
      memcpy(authentication_data->rand, rand->value, rand->length);
    }

    /* Start, or reset and restart timer T3416 */
    if (emm_timers->T3416.id != NAS_TIMER_INACTIVE_ID) {
      LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3416 (%d)", emm_timers->T3416.id);
      emm_timers->T3416.id = nas_timer_stop(emm_timers->T3416.id);
    }

    emm_timers->T3416.id = nas_timer_start(emm_timers->T3416.sec, _authentication_t3416_handler, NULL);
    LOG_TRACE(INFO, "EMM-PROC  - Timer T3416 (%d) expires in  %ld seconds",
              emm_timers->T3416.id, emm_timers->T3416.sec);
  }

  /*
   * The stored RAND value is equal to the new received value in the
   * AUTHENTICATION REQUEST message, or the UE has successfully checked
   * the authenticity of the core network
   */
  /* Start any retransmission timers */
  rc = _authentication_start_timers(user);

  if (rc != RETURNok) {
    LOG_TRACE(WARNING, "EMM-PROC  - Failed to start retransmission timers");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Setup EMM procedure handler to be executed upon receiving
   * lower layer notification */
  rc = emm_proc_lowerlayer_initialize(user->lowerlayer_data, NULL, NULL, NULL, NULL);

  if (rc != RETURNok) {
    LOG_TRACE(WARNING,
              "EMM-PROC  - Failed to initialize EMM procedure handler");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Notify EMM-AS SAP that Authentication Response message has to be sent
   * to the network
   */
  emm_sap_t emm_sap;
  emm_sap.primitive = EMMAS_SECURITY_RES;
  emm_sap.u.emm_as.u.security.guti = user->emm_data->guti;
  emm_sap.u.emm_as.u.security.ueid = user->ueid;
  emm_sap.u.emm_as.u.security.msgType = EMM_AS_MSG_TYPE_AUTH;
  emm_sap.u.emm_as.u.security.emm_cause = EMM_CAUSE_SUCCESS;
  emm_sap.u.emm_as.u.security.res = &res;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.security.sctx,
                           user->emm_data->security, false, true);
  rc = emm_sap_send(user, &emm_sap);

  if (rc != RETURNerror) {
    /* Reset the authentication failure counters */
    authentication_data->mac_count = 0;
    authentication_data->umts_count = 0;
    authentication_data->sync_count = 0;

    /* Create non-current EPS security context */
    if (user->emm_data->non_current == NULL) {
      user->emm_data->non_current =
        (emm_security_context_t *)malloc(sizeof(emm_security_context_t));
    }

    if (user->emm_data->non_current) {
      memset(user->emm_data->non_current, 0, sizeof(emm_security_context_t));

      /* Set the security context type */
      if (native_ksi) {
        user->emm_data->non_current->type = EMM_KSI_NATIVE;
      } else {
        user->emm_data->non_current->type = EMM_KSI_MAPPED;
      }

      /* Set the EPS key set identifier */
      user->emm_data->non_current->eksi = ksi;
      /* Derive the Kasme from the authentication challenge using
       * the PLMN identity of the selected PLMN */
      user->emm_data->non_current->kasme.length = AUTH_KASME_SIZE;
      user->emm_data->non_current->kasme.value  = malloc(32);
      _authentication_kasme(autn, &ck, &ik, &user->emm_data->splmn,
                            &user->emm_data->non_current->kasme);
      /* NAS integrity and cyphering keys are not yet available */
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication_reject()                          **
 **                                                                        **
 ** Description: Performs the authentication procedure not accepted by the **
 **      network.                                                  **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.5                           **
 **      Upon receiving an AUTHENTICATION REJECT message, the UE   **
 **      shall abort any EMM signalling procedure, stop any of the **
 **      timers T3410, T3417 or T3430 (if running) and enter state **
 **      EMM-DEREGISTERED.                                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **             T3417, T3430                               **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication_reject(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  LOG_TRACE(WARNING, "EMM-PROC  - Authentication not accepted by the network");

  /* Delete any previously stored RAND and RES and stop timer T3416 */
  (void) emm_proc_authentication_delete(user);

  /* Set the EPS update status to EU3 ROAMING NOT ALLOWED */
  user->emm_data->status = EU3_ROAMING_NOT_ALLOWED;
  /* Delete the stored GUTI */
  user->emm_data->guti = NULL;
  /* Delete the TAI list */
  user->emm_data->ltai.n_tais = 0;
  /* Delete the last visited registered TAI */
  user->emm_data->tai = NULL;

  /* Delete the eKSI */
  if (user->emm_data->security) {
    user->emm_data->security->type = EMM_KSI_NOT_AVAILABLE;
  }

  /* Consider the USIM invalid */
  user->emm_data->usim_is_valid = false;

  /* Stop timer T3410 */
  if (emm_timers->T3410.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3410 (%d)", emm_timers->T3410.id);
    emm_timers->T3410.id = nas_timer_stop(emm_timers->T3410.id);
  }

  /* Stop timer T3417 */
  if (emm_timers->T3417.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3417 (%d)", emm_timers->T3417.id);
    emm_timers->T3417.id = nas_timer_stop(emm_timers->T3417.id);
  }

  /* Stop timer T3430 */
  if (emm_timers->T3430.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3430 (%d)", emm_timers->T3430.id);
    emm_timers->T3430.id = nas_timer_stop(emm_timers->T3430.id);
  }

  /* Abort any EMM signalling procedure (prevent the retransmission timers to
   * be restarted) */
  authentication_data->timers = 0x00;

  /*
   * Notify EMM that authentication is not accepted by the network
   */
  emm_sap.primitive = EMMREG_AUTH_REJ;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication_delete()                          **
 **                                                                        **
 ** Description: Deletes the RAND and RES values stored into the volatile  **
 **      memory of the Mobile Equipment and stop timer T3416, if   **
 **      running, upon receipt of a SECURITY MODE COMMAND, SERVICE **
 **      REJECT, TRACKING AREA UPDATE REJECT, TRACKING AREA UPDATE **
 **      ACCEPT or AUTHENTICATION REJECT message; upon expiry of   **
 **      timer  T3416; or if the UE  enters  the  EMM  state  EMM- **
 **      DEREGISTERED or EMM-NULL.                                 **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.3                           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication_delete(nas_user_t *user)
{
  LOG_FUNC_IN;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  LOG_TRACE(INFO, "EMM-PROC  - Delete authentication data RAND and RES");

  /* Stop timer T3416, if running */
  if (emm_timers->T3416.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3416 (%d)", emm_timers->T3416.id);
    emm_timers->T3416.id = nas_timer_stop(emm_timers->T3416.id);
  }

  /* Delete any previously stored RAND and RES */
  memset(authentication_data->rand, 0, AUTH_RAND_SIZE);
  memset(authentication_data->res, 0, AUTH_RES_SIZE);

  LOG_FUNC_RETURN (RETURNok);
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
 ** Name:    _authentication_t3416_handler()                           **
 **                                                                        **
 ** Description: T3416 timeout handler                                     **
 **      Upon T3416 timer expiration, the RAND and RES values sto- **
 **      red in the ME shall be deleted.                           **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.3                           **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    T3416                                      **
 **                                                                        **
 ***************************************************************************/
static void *_authentication_t3416_handler(void *args)
{
  LOG_FUNC_IN;
  nas_user_t *user=args;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  LOG_TRACE(WARNING, "EMM-PROC  - T3416 timer expired");

  /* Stop timer T3416 */
  emm_timers->T3416.id = nas_timer_stop(emm_timers->T3416.id);
  /* Delete previouly stored RAND and RES authentication data */
  (void) emm_proc_authentication_delete(user);

  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_t3418_handler()                           **
 **                                                                        **
 ** Description: T3418 timeout handler                                     **
 **      Upon T3418 timer expiration, the UE shall deem that the   **
 **      source of the  authentication  challenge is not  genuine  **
 **      (authentication not accepted by the UE).                  **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.7, case c                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_authentication_t3418_handler(void *args)
{
  LOG_FUNC_IN;

  int rc;
  nas_user_t *user=args;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;
  authentication_data_t *authentication_data = user->authentication_data;

  LOG_TRACE(WARNING, "EMM-PROC  - T3418 timer expired");

  /* Stop timer T3418 */
  emm_timers->T3418.id = nas_timer_stop(emm_timers->T3418.id);
  /* Reset the MAC failure and UMTS challenge failure counters */
  authentication_data->mac_count = 0;
  authentication_data->umts_count = 0;
  /* 3GPP TS 24.301, section 5.4.2.7, case f */
  rc = _authentication_abnormal_case_f(user);

  if (rc != RETURNok) {
    LOG_TRACE(WARNING, "EMM-PROC  - Failed to proceed abnormal case f");
  }

  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_t3420_handler()                           **
 **                                                                        **
 ** Description: T3420 timeout handler                                     **
 **      Upon T3420 timer expiration, the UE shall deem that the   **
 **      network has failed the authentication check.              **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.7, case e                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_authentication_t3420_handler(void *args)
{
  LOG_FUNC_IN;

  int rc;
  nas_user_t *user=args;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  LOG_TRACE(WARNING, "EMM-PROC  - T3420 timer expired");

  /* Stop timer T3420 */
  emm_timers->T3420.id = nas_timer_stop(emm_timers->T3420.id);
  /* Reset the sync failure counter */
  authentication_data->sync_count = 0;
  /* 3GPP TS 24.301, section 5.4.2.7, case f */
  rc = _authentication_abnormal_case_f(user);

  if (rc != RETURNok) {
    LOG_TRACE(WARNING, "EMM-PROC  - Failed to proceed abnormal case f");
  }

  LOG_FUNC_RETURN (NULL);
}

/*
 * --------------------------------------------------------------------------
 *              Abnormal cases in the UE
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_abnormal_cases_cde()                      **
 **                                                                        **
 ** Description: Performs the abnormal case authentication procedure.      **
 **                                                                        **
 **      3GPP TS 24.301, section 5.4.2.7, cases c, d and e         **
 **                                                                        **
 ** Inputs:  emm_cause: EMM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_abnormal_cases_cde(nas_user_t *user, int emm_cause,
    const OctetString *auts)
{
  LOG_FUNC_IN;

  int rc;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  LOG_TRACE(WARNING, "EMM-PROC  - "
            "Abnormal case, authentication counters c/d/e = %d/%d/%d",
            authentication_data->mac_count, authentication_data->umts_count,
            authentication_data->sync_count);

  /*
   * Notify EMM-AS SAP that Authentication Failure message has to be sent
   * to the network
   */
  emm_sap_t emm_sap;
  emm_sap.primitive = EMMAS_SECURITY_RES;
  emm_sap.u.emm_as.u.security.guti = user->emm_data->guti;
  emm_sap.u.emm_as.u.security.ueid = user->ueid;
  emm_sap.u.emm_as.u.security.msgType = EMM_AS_MSG_TYPE_AUTH;
  emm_sap.u.emm_as.u.security.emm_cause = emm_cause;
  emm_sap.u.emm_as.u.security.auts = auts;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.security.sctx,
                           user->emm_data->security, false, true);
  rc = emm_sap_send(user, &emm_sap);

  if (rc != RETURNerror) {
    /*
     * Update the authentication failure counters
     */
    switch (emm_cause) {
    case EMM_CAUSE_MAC_FAILURE:
      /* 3GPP TS 24.301, section 5.4.2.6, case c
       * Update the MAC failure counter */
      authentication_data->mac_count += 1;
      /* Start timer T3418 */
      emm_timers->T3418.id = nas_timer_start(emm_timers->T3418.sec,
                                 _authentication_t3418_handler, user);
      LOG_TRACE(INFO,"EMM-PROC  - Timer T3418 (%d) expires in "
                "%ld seconds", emm_timers->T3418.id, emm_timers->T3418.sec);
      break;

    case EMM_CAUSE_NON_EPS_AUTH_UNACCEPTABLE:
      /* 3GPP TS 24.301, section 5.4.2.6, case d
       * Update the UMTS challenge failure counter */
      authentication_data->umts_count += 1;
      /* Start timer T3418 */
      emm_timers->T3418.id = nas_timer_start(emm_timers->T3418.sec,
                                 _authentication_t3418_handler, user);
      LOG_TRACE(INFO,"EMM-PROC  - Timer T3418 (%d) expires in "
                "%ld seconds", emm_timers->T3418.id, emm_timers->T3418.sec);
      break;

    case EMM_CAUSE_SYNCH_FAILURE:
      /* 3GPP TS 24.301, section 5.4.2.6, case e
       * Update the synch failure counter */
      authentication_data->sync_count += 1;
      /* Start timer T3420 */
      emm_timers->T3420.id = nas_timer_start(emm_timers->T3420.sec,
                                 _authentication_t3420_handler, user);
      LOG_TRACE(INFO,"EMM-PROC  - Timer T3420 (%d) expires in "
                "%ld seconds", emm_timers->T3420.id, emm_timers->T3420.sec);
      break;

    default:
      LOG_TRACE(WARNING, "EMM cause code is not valid (%d)",
                emm_cause);
      LOG_FUNC_RETURN (RETURNerror);
    }

    /*
     * Stop any retransmission timers that are running
     */
    rc = _authentication_stop_timers(user);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-PROC  - "
                "Failed to stop retransmission timers");
      LOG_FUNC_RETURN (RETURNerror);
    }

    /*
     * Check whether the network has failed the authentication check
     */
    int failure_counter = 0;

    if (emm_cause == EMM_CAUSE_MAC_FAILURE) {
      failure_counter = authentication_data->mac_count
                        + authentication_data->sync_count;
    } else if (emm_cause == EMM_CAUSE_SYNCH_FAILURE) {
      failure_counter = authentication_data->mac_count
                        + authentication_data->umts_count
                        + authentication_data->sync_count;
    }

    if (failure_counter >= AUTHENTICATION_COUNTER_MAX) {
      /* 3GPP TS 24.301, section 5.4.2.6, case f */
      rc = _authentication_abnormal_case_f(user);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING, "EMM-PROC  - "
                  "Failed to proceed abnormal case f");
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_abnormal_case_f()                         **
 **                                                                        **
 ** Description: Performs the abnormal case authentication procedure.      **
 **                                                                        **
 **      3GPP TS 24.301, section 5.4.2.7, case f                   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _authentication_abnormal_case_f(nas_user_t *user)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(WARNING, "EMM-PROC  - Authentication abnormal case f");

  /*
   * Request RRC to locally release the RRC connection and treat
   * the active cell as barred
   */
  emm_sap_t emm_sap;
  emm_sap.primitive = EMMAS_RELEASE_REQ;
  emm_sap.u.emm_as.u.release.guti = user->emm_data->guti;
  emm_sap.u.emm_as.u.release.cause = EMM_AS_CAUSE_AUTHENTICATION;
  rc = emm_sap_send(user, &emm_sap);

  if (rc != RETURNerror) {
    /* Start any retransmission timers (e.g. T3410, T3417, T3421 or
     * T3430), if they were running and stopped when the UE received
     * the first AUTHENTICATION REQUEST message containing an invalid
     * MAC or SQN */
    rc = _authentication_start_timers(user);
  }

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 *              UE specific local functions
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_stop_timers()                             **
 **                                                                        **
 ** Description: Stops any retransmission timers (e.g. T3410, T3417, T3421 **
 **      or T3430) that are running                                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **             T3430                                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_stop_timers(nas_user_t *user)
{
  LOG_FUNC_IN;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  /* Stop attach timer */
  if (emm_timers->T3410.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3410 (%d)", emm_timers->T3410.id);
    emm_timers->T3410.id = nas_timer_stop(emm_timers->T3410.id);
    authentication_data->timers |= AUTHENTICATION_T3410;
  }

  /* Stop service request timer */
  if (emm_timers->T3417.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3417 (%d)", emm_timers->T3417.id);
    emm_timers->T3417.id = nas_timer_stop(emm_timers->T3417.id);
    authentication_data->timers |= AUTHENTICATION_T3417;
  }

  /* Stop detach timer */
  if (emm_timers->T3421.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3421 (%d)", emm_timers->T3421.id);
    emm_timers->T3421.id = nas_timer_stop(emm_timers->T3421.id);
    authentication_data->timers |= AUTHENTICATION_T3421;
  }

  /* Stop tracking area update timer */
  if (emm_timers->T3430.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3430 (%d)", emm_timers->T3430.id);
    emm_timers->T3430.id = nas_timer_stop(emm_timers->T3430.id);
    authentication_data->timers |= AUTHENTICATION_T3430;
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_start_timers()                            **
 **                                                                        **
 ** Description: Starts any retransmission timers (e.g. T3410, T3417,      **
 **      T3421 or T3430), if they were running and stopped when    **
 **      the UE received the first AUTHENTICATION REQUEST message  **
 **      containing an invalid MAC or SQN                          **
 **                                                                        **
 **      3GPP TS 24.301, section 5.4.2.7, case f                   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3410, T3417, T3421, T3430                 **
 **                                                                        **
 ***************************************************************************/
static int _authentication_start_timers(nas_user_t *user)
{
  LOG_FUNC_IN;
  authentication_data_t *authentication_data = user->authentication_data;
  emm_timers_t *emm_timers = user->emm_data->emm_timers;

  if (authentication_data->timers & AUTHENTICATION_T3410) {
    /* Start attach timer */
    emm_timers->T3410.id = nas_timer_start(emm_timers->T3410.sec, emm_attach_t3410_handler, NULL);
    LOG_TRACE(INFO,"EMM-PROC  - Timer T3410 (%d) expires in "
              "%ld seconds", emm_timers->T3410.id, emm_timers->T3410.sec);
  }

  if (authentication_data->timers & AUTHENTICATION_T3417) {
    /* Start service request timer */
    emm_timers->T3417.id = nas_timer_start(emm_timers->T3417.sec, emm_service_t3417_handler, NULL);
    LOG_TRACE(INFO,"EMM-PROC  - Timer T3417 (%d) expires in "
              "%ld seconds", emm_timers->T3417.id, emm_timers->T3417.sec);
  }

  if (authentication_data->timers & AUTHENTICATION_T3421) {
    /* Start detach timer */
    emm_timers->T3421.id = nas_timer_start(emm_timers->T3421.sec, emm_detach_t3421_handler, NULL);
    LOG_TRACE(INFO,"EMM-PROC  - Timer T3421 (%d) expires in "
              "%ld seconds", emm_timers->T3421.id, emm_timers->T3421.sec);
  }

  if (authentication_data->timers & AUTHENTICATION_T3430) {
    /* Start tracking area update timer */
    emm_timers->T3430.id = nas_timer_start(emm_timers->T3430.sec, emm_tau_t3430_handler, NULL);
    LOG_TRACE(INFO,"EMM-PROC  - Timer T3430 (%d) expires in "
              "%ld seconds", emm_timers->T3430.id, emm_timers->T3430.sec);
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_kasme()                                   **
 **                                                                        **
 ** Description: Computes the Key Access Security Management Entity Kasme  **
 **      from the provided authentication challenge data.          **
 **                                                                        **
 **              3GPP TS 33.401, Annex A.2                                 **
 **                                                                        **
 ** Inputs:  autn:      Authentication token                       **
 **      ck:        Cipherig key                               **
 **      ik:        Integrity key                              **
 **      plmn:      Identifier of the currently selected PLMN  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     kasme:     Key Access Security Management Entity      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _authentication_kasme(const OctetString *autn,
                                 const OctetString *ck, const OctetString *ik,
                                 const plmn_t *plmn, OctetString *kasme)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO,"EMM-PROC  _authentication_kasme INPUT CK %s",
            dump_octet_string(ck));
  LOG_TRACE(INFO,"EMM-PROC  _authentication_kasme INPUT IK %s",
            dump_octet_string(ik));
  LOG_TRACE(INFO,"EMM-PROC  _authentication_kasme INPUT AUTN %s",
            dump_octet_string(autn));
  LOG_TRACE(INFO,"EMM-PROC  _authentication_kasme INPUT KASME LENGTH %u",
            kasme->length);

  /* Compute the derivation key KEY = CK || IK */
  uint8_t key[ck->length + ik->length];
  memcpy(key, ck->value, ck->length);
  memcpy(key + ck->length, ik->value, ik->length);

  /* Compute the KDF input_s parameter
   * S = FC(0x10) || SNid(MCC, MNC) || 0x00 0x03 || SQN ⊕ AK || 0x00 0x06
   */
  uint8_t  input_s[16]; // less than 16
  uint8_t  sn_id[AUTH_SNID_SIZE]; // less than 16
  uint16_t length;
  int      offset         = 0;
  int      size_of_length = sizeof(length);

  // FC
  input_s[offset] = 0x10;
  offset       += 1;

  // P0=SN id
  length        = AUTH_SNID_SIZE;
  sn_id[0] = (plmn->MCCdigit2 << 4) | plmn->MCCdigit1;
  sn_id[1] = (plmn->MNCdigit3 << 4) | plmn->MCCdigit3;
  sn_id[2] = (plmn->MNCdigit2 << 4) | plmn->MNCdigit1;

  memcpy(input_s + offset, sn_id, length);
  LOG_TRACE(INFO,"EMM-PROC  _authentication_kasme P0 MCC,MNC %02X %02X %02X",
            input_s[offset],
            input_s[offset+1],
            input_s[offset+2]);
  offset += length;
  // L0=SN id length
  length = htons(length);
  memcpy(input_s + offset, &length, size_of_length);
  offset += size_of_length;

  // P1=Authentication token
  length = AUTH_SQN_SIZE;
  memcpy(input_s + offset, autn->value, length);
  offset += length;
  // L1
  length = htons(length);
  memcpy(input_s + offset, &length, size_of_length);
  offset += size_of_length;

  LOG_TRACE(INFO,
            "EMM-PROC  _authentication_kasme input S to KFD (length %u)%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            offset,
            input_s[0],input_s[1],input_s[2],input_s[3],
            input_s[4],input_s[5],input_s[6],input_s[7],
            input_s[8],input_s[9],input_s[10],input_s[11],
            input_s[12],input_s[13]);
  /* TODO !!! Compute the Kasme key */
  // todo_hmac_256(key, input_s, kasme->value);
  kdf(key,
      ck->length + ik->length , /*key_length*/
      input_s,
      offset,
      kasme->value,
      kasme->length);

  LOG_TRACE(INFO,"EMM-PROC  KASME (l=%d)%s",
            kasme->length,
            dump_octet_string(kasme));

  LOG_FUNC_RETURN (RETURNok);
}
