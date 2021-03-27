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
Source      SecurityModeControl.c

Version     0.1

Date        2013/04/22

Product     NAS stack

Subsystem   Template body file

Author      Frederic Maurel

Description Defines the security mode control EMM procedure executed by the
        Non-Access Stratum.

        The purpose of the NAS security mode control procedure is to
        take an EPS security context into use, and initialise and start
        NAS signalling security between the UE and the MME with the
        corresponding EPS NAS keys and EPS security algorithms.

        Furthermore, the network may also initiate a SECURITY MODE COM-
        MAND in order to change the NAS security algorithms for a cur-
        rent EPS security context already in use.

*****************************************************************************/

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <inttypes.h>

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "emm_cause.h"

#include "UeSecurityCapability.h"

# include "assertions.h"
#include "secu_defs.h"
#include "msc.h"
#include "SecurityModeControl.h"

#if  defined(NAS_BUILT_IN_UE)
#include "nas_itti_messaging.h"
#endif

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the security mode control procedure in the UE
 * --------------------------------------------------------------------------
 */
static int _security_kdf(const OctetString *kasme, OctetString *key,
                         uint8_t algo_dist, uint8_t algo_id);

static int _security_knas_enc(const OctetString *kasme, OctetString *knas_enc,
                              uint8_t eia);
static int _security_knas_int(const OctetString *kasme, OctetString *knas_int,
                              uint8_t eea);
static int _security_kenb(const OctetString *kasme, OctetString *kenb,
                          uint32_t count);

static void _security_release(emm_security_context_t *ctx);

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the security mode control procedure in the MME
 * --------------------------------------------------------------------------
 */

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      Security mode control procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_security_mode_command()                              **
 **                                                                        **
 ** Description: Performs the MME requested security mode control proce-   **
 **      dure.                                                             **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.3.3                           **
 **      Upon receiving the SECURITY MODE COMMAND message, the UE          **
 **      shall check whether the message can be accepted or not.           **
 **      If accepted the UE shall send a SECURITY MODE COMPLETE            **
 **      message integrity protected with the selected NAS inte-           **
 **      grity algorithm and ciphered with the selected NAS ciphe-         **
 **      ring algorithm.                                                   **
 **                                                                        **
 ** Inputs:  native_ksi:    TRUE if the security context is of type        **
 **             native (for KSIASME)                                       **
 **      ksi:       The NAS ket sey identifier                             **
 **      seea:      Selected EPS cyphering algorithm                       **
 **      seia:      Selected EPS integrity algorithm                       **
 **      reea:      Replayed EPS cyphering algorithm                       **
 **      reia:      Replayed EPS integrity algorithm                       **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    None                                                   **
 **                                                                        **
 ***************************************************************************/
int emm_proc_security_mode_command(nas_user_t *user, int native_ksi, int ksi,
                                   int seea, int seia, int reea, int reia, int imeisv_request)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  int emm_cause = EMM_CAUSE_SUCCESS;
  int security_context_is_new = FALSE;
  security_data_t *security_data = user->security_data;

  LOG_TRACE(INFO, "EMM-PROC  - Security mode control requested (ksi=%d)",
            ksi);

  /* Delete any previously stored RAND and RES and stop timer T3416 */
  (void) emm_proc_authentication_delete(user);

  /*
   * Check the replayed UE security capabilities
   */
  uint8_t eea = (0x80 >> user->emm_data->security->capability.eps_encryption);
  uint8_t eia = (0x80 >> user->emm_data->security->capability.eps_integrity);

  if ( (reea != eea) || (reia != eia) ) {
    LOG_TRACE(WARNING, "EMM-PROC  - Replayed UE security capabilities "
              "rejected");
    emm_cause = EMM_CAUSE_UE_SECURITY_MISMATCH;

    /* XXX - For testing purpose UE always accepts EIA0
     * The UE shall accept "null integrity protection algorithm" EIA0 only
     * if a PDN connection for emergency bearer services is established or
     * the UE is establishing a PDN connection for emergency bearer services
     */
  }
  /*
   * Check the non-current EPS security context
   */
  else if (user->emm_data->non_current == NULL) {
    LOG_TRACE(WARNING, "EMM-PROC  - Non-current EPS security context "
              "is not valid");
    emm_cause = EMM_CAUSE_SECURITY_MODE_REJECTED;
  }
  /*
   * Update the non-current EPS security context
   */
  else {
    LOG_TRACE(INFO, "EMM-PROC  - Update the non-current EPS security context seea=%u seia=%u", seea, seia);
    /* Update selected cyphering and integrity algorithms */
    //LG COMENTED user->emm_data->non_current->capability.encryption = seea;
    //LG COMENTED user->emm_data->non_current->capability.integrity  = seia;

    user->emm_data->non_current->selected_algorithms.encryption = seea;
    user->emm_data->non_current->selected_algorithms.integrity = seia;

    /* Derive the NAS cyphering key */
    if (user->emm_data->non_current->knas_enc.value == NULL) {
      user->emm_data->non_current->knas_enc.value =
        (uint8_t *)calloc(1,AUTH_KNAS_ENC_SIZE);
      user->emm_data->non_current->knas_enc.length = AUTH_KNAS_ENC_SIZE;
    }

    if (user->emm_data->non_current->knas_enc.value != NULL) {
      LOG_TRACE(INFO, "EMM-PROC  - Update the non-current EPS security context knas_enc");
      rc = _security_knas_enc(&user->emm_data->non_current->kasme,
                              &user->emm_data->non_current->knas_enc, seea);
    }

    /* Derive the NAS integrity key */
    if (user->emm_data->non_current->knas_int.value == NULL) {
      user->emm_data->non_current->knas_int.value =
        (uint8_t *)calloc(1,AUTH_KNAS_INT_SIZE);
      user->emm_data->non_current->knas_int.length = AUTH_KNAS_INT_SIZE;
    }

    if (user->emm_data->non_current->knas_int.value != NULL) {
      if (rc != RETURNerror) {
        LOG_TRACE(INFO, "EMM-PROC  - Update the non-current EPS security context knas_int");
        rc = _security_knas_int(&user->emm_data->non_current->kasme,
                                &user->emm_data->non_current->knas_int, seia);
      }
    }

    /* Derive the eNodeB key */
    if (security_data->kenb.value == NULL) {
      security_data->kenb.value = (uint8_t *)calloc(1,AUTH_KENB_SIZE);
      security_data->kenb.length = AUTH_KENB_SIZE;
    }

    if (security_data->kenb.value != NULL) {
      if (rc != RETURNerror) {
        LOG_TRACE(INFO, "EMM-PROC  - Update the non-current EPS security context kenb");
        // LG COMMENT rc = _security_kenb(&user->emm_data->security->kasme,
        rc = _security_kenb(&user->emm_data->non_current->kasme,
                            &security_data->kenb,
                            *(uint32_t *)(&user->emm_data->non_current->ul_count));
      }
    }

    /*
     * NAS security mode command accepted by the UE
     */
    if (rc != RETURNerror) {
      LOG_TRACE(INFO, "EMM-PROC  - NAS security mode command accepted by the UE");

      /* Update the current EPS security context */
      if ( native_ksi && (user->emm_data->security->type != EMM_KSI_NATIVE) ) {
        /* The type of security context flag included in the SECURITY
         * MODE COMMAND message is set to "native security context" and
         * the UE has a mapped EPS security context as the current EPS
         * security context */
        if ( (user->emm_data->non_current->type == EMM_KSI_NATIVE) &&
             (user->emm_data->non_current->eksi == ksi) ) {
          /* The KSI matches the non-current native EPS security
           * context; the UE shall take the non-current native EPS
           * security context into use which then becomes the
           * current native EPS security context and delete the
           * mapped EPS security context */
          LOG_TRACE(INFO,
                    "EMM-PROC  - Update Current security context");
          /* Release non-current security context */
          _security_release(user->emm_data->security);
          user->emm_data->security = user->emm_data->non_current;
          /* Reset the uplink NAS COUNT counter */
          user->emm_data->security->ul_count.overflow = 0;
          user->emm_data->security->ul_count.seq_num = 0;
          /* Set new security context indicator */
          security_context_is_new = TRUE;
        }
      }

      if ( !native_ksi && (user->emm_data->security->type != EMM_KSI_NATIVE) ) {
        /* The type of security context flag included in the SECURITY
         * MODE COMMAND message is set to "mapped security context" and
         * the UE has a mapped EPS security context as the current EPS
         * security context */
        if (ksi != user->emm_data->security->eksi) {
          /* The KSI does not match the current EPS security context;
           * the UE shall reset the uplink NAS COUNT counter */
          LOG_TRACE(INFO,
                    "EMM-PROC  - Reset uplink NAS COUNT counter");
          user->emm_data->security->ul_count.overflow = 0;
          user->emm_data->security->ul_count.seq_num = 0;
        }
      }

      user->emm_data->security->selected_algorithms.encryption = seea;
      user->emm_data->security->selected_algorithms.integrity  = seia;
#if  defined(NAS_BUILT_IN_UE)
      nas_itti_kenb_refresh_req(security_data->kenb.value, user->ueid);
#endif

    }
    /*
     * NAS security mode command not accepted by the UE
     */
    else {
      /* Setup EMM cause code */
      emm_cause = EMM_CAUSE_SECURITY_MODE_REJECTED;
    }
  }

  /* Release security mode control internal data */
  if (security_data->kenb.value) {
    free(security_data->kenb.value);
    security_data->kenb.value = NULL;
    security_data->kenb.length = 0;
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
   * Notify EMM-AS SAP that Security Mode response message has to be sent
   * to the network
   */
  emm_sap_t emm_sap;
  emm_sap.primitive = EMMAS_SECURITY_RES;
  emm_sap.u.emm_as.u.security.guti = user->emm_data->guti;
  emm_sap.u.emm_as.u.security.ueid = user->ueid;
  emm_sap.u.emm_as.u.security.msgType = EMM_AS_MSG_TYPE_SMC;
  emm_sap.u.emm_as.u.security.imeisv_request = imeisv_request;
  emm_sap.u.emm_as.u.security.emm_cause = emm_cause;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.security.sctx,
                           user->emm_data->security, security_context_is_new, TRUE);
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              UE specific local functions
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _security_release()                                       **
 **                                                                        **
 ** Description: Releases the given EPS NAS security context               **
 **                                                                        **
 ** Inputs:  ctx:       The EPS NAS security context to release    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void _security_release(emm_security_context_t *ctx)
{
  LOG_FUNC_IN;

  if (ctx) {
    /* Release Kasme security key */
    if (ctx->kasme.value) {
      free(ctx->kasme.value);
      ctx->kasme.value  = NULL;
      ctx->kasme.length = 0;
    }

    /* Release NAS cyphering key */
    if (ctx->knas_enc.value) {
      free(ctx->knas_enc.value);
      ctx->knas_enc.value  = NULL;
      ctx->knas_enc.length = 0;
    }

    /* Release NAS integrity key */
    if (ctx->knas_int.value) {
      free(ctx->knas_int.value);
      ctx->knas_int.value  = NULL;
      ctx->knas_int.length = 0;
    }

    /* Release the NAS security context */
    free(ctx);
  }

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    _security_knas_enc()                                      **
 **                                                                        **
 ** Description: Algorithm Key generation function used for the derivation **
 **      of NAS encryption key Knas-enc from the Kasme.            **
 **                                                                        **
 **              3GPP TS 33.401, Annex A.7                                 **
 **                                                                        **
 ** Inputs:  kasme:     Key Access Security Management Entity      **
 **      eea:       Cyphering algorithm identity               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     knas_enc:  Derived key for NAS cyphering algorithm    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _security_knas_enc(const OctetString *kasme, OctetString *knas_enc,
                              uint8_t eea)
{
  LOG_FUNC_IN;
  LOG_TRACE(INFO, "%s  with algo dist %d algo id %d", __FUNCTION__,0x01, eea);
  LOG_FUNC_RETURN (_security_kdf(kasme, knas_enc, 0x01, eea));
}

/****************************************************************************
 **                                                                        **
 ** Name:    _security_knas_int()                                      **
 **                                                                        **
 ** Description: Algorithm Key generation function used for the derivation **
 **      of NAS integrity key Knas-int from the Kasme.             **
 **                                                                        **
 **              3GPP TS 33.401, Annex A.7                                 **
 **                                                                        **
 ** Inputs:  kasme:     Key Access Security Management Entity      **
 **      eia:       Integrity algorithm identity               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     knas_int:  Derived key for NAS integrity algorithm    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _security_knas_int(const OctetString *kasme, OctetString *knas_int,
                              uint8_t eia)
{
  LOG_FUNC_IN;
  LOG_TRACE(INFO, "%s  with algo dist %d algo id %d", __FUNCTION__,0x02, eia);
  LOG_FUNC_RETURN (_security_kdf(kasme, knas_int, 0x02, eia));
}

/****************************************************************************
 **                                                                        **
 ** Name:    _security_kenb()                                          **
 **                                                                        **
 ** Description: Computes the eNodeB key from Kasme and the given value of **
 **      uplink NAS counter.                                       **
 **                                                                        **
 **              3GPP TS 33.401, Annex A.3                                 **
 **                                                                        **
 ** Inputs:  kasme:     Key Access Security Management Entity      **
 **      count:     Uplink NAS counter value                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     kenb:      eNodeB security key                        **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _security_kenb(const OctetString *kasme, OctetString *kenb,
                          uint32_t count)
{
  /* Compute the KDF input parameter
   * S = FC(0x11) || UL NAS Count || 0x00 0x04
   */
  uint8_t  input[32];
  //    uint16_t length    = 4;
  //    int      offset    = 0;

  LOG_TRACE(INFO, "%s  with count= %d", __FUNCTION__, count);
  memset(input, 0, 32);
  input[0] = 0x11;
  // P0
  input[1] = count >> 24;
  input[2] = (uint8_t)(count >> 16);
  input[3] = (uint8_t)(count >> 8);
  input[4] = (uint8_t)count;
  // L0
  input[5] = 0;
  input[6] = 4;

  kdf(kasme->value, 32, input, 7, kenb->value, 32);
  kenb->length = 32;
  return (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _security_kdf()                                           **
 **                                                                        **
 ** Description: Algorithm Key generation function used for the derivation **
 **      of keys for NAS integrity and NAS encryption algorithms   **
 **      from Kasme, algorithm types and algorithm identities.     **
 **                                                                        **
 **              3GPP TS 33.401, Annex A.7                                 **
 **                                                                        **
 ** Inputs:  kasme:     Key Access Security Management Entity      **
 **      algo_dist: Algorithm type distinguisher               **
 **      algo_id:   Algorithm identity                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     key:       Derived key for NAS security protection    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _security_kdf(const OctetString *kasme, OctetString *key,
                         uint8_t algo_dist, uint8_t algo_id)
{
  /* Compute the KDF input parameter
   * S = FC(0x15) || Algorithm distinguisher || 0x00 0x01
          || Algorithm identity || 0x00 0x01
  */
  uint8_t input[32];
  uint8_t output[32];
  LOG_TRACE(DEBUG, "%s:%u output key mem %p lenth %u",
            __FUNCTION__, __LINE__,
            key->value,
            key->length);
  memset(input, 0, 32);
  // FC
  input[0] = 0x15;
  // P0 = Algorithm distinguisher
  input[1] = algo_dist;
  // L0 = 0x00 01
  input[2] = 0x00;
  input[3] = 0x01;
  // P1 = Algorithm identity
  input[4] = algo_id;
  // L1 = length of Algorithm identity 0x00 0x01
  input[5] = 0x00;
  input[6] = 0x01;

  /* Compute the derived key */
  kdf(kasme->value, kasme->length, input, 7, output, 32);
  memcpy(key->value, &output[31 - key->length + 1], key->length);
  return (RETURNok);
}

