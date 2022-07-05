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
Source      LowerLayer.c

Version     0.1

Date        2012/03/14

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines EMM procedures executed by the Non-Access Stratum
        upon receiving notifications from lower layers so that data
        transfer succeed or failed, or NAS signalling connection is
        released, or ESM unit data has been received from under layer,
        and to request ESM unit data transfer to under layer.

*****************************************************************************/

#include "LowerLayer.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emmData.h"

#include "emm_sap.h"
#include "esm_sap.h"
#include "nas_log.h"

#include <string.h> // memset

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *          Lower layer notification handlers
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_success()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that data have  **
 **      been successfully delivered to the network                **
 **                                                                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_success(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  emm_sap.primitive = EMMREG_LOWERLAYER_SUCCESS;
  emm_sap.u.emm_reg.ueid = user->ueid;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_failure()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that lower la-  **
 **      yers failed to deliver data to the network                **
 **                                                                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_failure(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  emm_sap.primitive = EMMREG_LOWERLAYER_FAILURE;
  emm_sap.u.emm_reg.ueid = user->ueid;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_establish()                                    **
 **                                                                        **
 ** Description: Update the EPS connection management status upon recei-   **
 **      ving indication so that the NAS signalling connection is  **
 **      established                                               **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_establish(nas_user_t *user)
{
  LOG_FUNC_IN;

  /* Update the EPS Connection Management status */
  user->emm_data->ecm_status = ECM_CONNECTED;

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_release()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that NAS signal-**
 **      ling connection has been released                         **
 **                                                                        **
 ** Inputs:  cause:     Release cause                              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_release(nas_user_t *user, int cause)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /* Update the EPS Connection Management status */
  user->emm_data->ecm_status = ECM_IDLE;

  emm_sap.primitive = EMMREG_LOWERLAYER_RELEASE;
  emm_sap.u.emm_reg.ueid = user->ueid;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_data_ind()                                     **
 **                                                                        **
 ** Description: Notify the EPS Session Management entity that data have   **
 **      been received from lower layers                           **
 **                                                                        **
 **      data:      Data transfered from lower layers          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_data_ind(nas_user_t *user, const OctetString *data)
{
  esm_sap_t esm_sap;
  int rc;


  LOG_FUNC_IN;

  esm_sap.primitive = ESM_UNITDATA_IND;
  esm_sap.is_standalone = true;
  esm_sap.ueid = user->ueid;

  esm_sap.recv = data;
  rc = esm_sap_send(user, &esm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_data_req()                                     **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that data have  **
 **      to be transfered to lower layers                          **
 **                                                                        **
 **          data:      Data to be transfered to lower layers      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_data_req(nas_user_t *user, const OctetString *data)
{
  LOG_FUNC_IN;

  int rc;
  emm_sap_t emm_sap;
  emm_security_context_t    *sctx = NULL;
  //struct emm_data_context_s *ctx  = NULL;

  emm_sap.primitive = EMMAS_DATA_REQ;
  emm_sap.u.emm_as.u.data.guti = user->emm_data->guti;
  emm_sap.u.emm_as.u.data.ueid = user->ueid;
  sctx = user->emm_data->security;

  emm_sap.u.emm_as.u.data.NASinfo = 0;
  emm_sap.u.emm_as.u.data.NASmsg.length = data->length;
  emm_sap.u.emm_as.u.data.NASmsg.value = data->value;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.data.sctx, sctx, false, true);
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 *              EMM procedure handlers
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_lowerlayer_initialize()                          **
 **                                                                        **
 ** Description: Initialize EMM procedure handler                          **
 **                                                                        **
 ** Inputs:  success:   EMM procedure executed when data have been **
 **             successfully delivered by lower layers     **
 **      failure:   EMM procedure executed upon transmission   **
 **             failure reported by lower layers           **
 **      release:   EMM procedure executed when lower layers   **
 **             report that NAS signalling connection has  **
 **             been released                              **
 **      args:      EMM procedure argument parameters          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _lowerlayer_data                           **
 **                                                                        **
 ***************************************************************************/
int emm_proc_lowerlayer_initialize(lowerlayer_data_t *lowerlayer_data, lowerlayer_success_callback_t success,
                                   lowerlayer_failure_callback_t failure,
                                   lowerlayer_release_callback_t release,
                                   void *args)
{
  LOG_FUNC_IN;

  lowerlayer_data->success = success;
  lowerlayer_data->failure = failure;
  lowerlayer_data->release = release;
  lowerlayer_data->args = args;

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_lowerlayer_success()                             **
 **                                                                        **
 ** Description: Handles EMM procedure to be executed upon receiving noti- **
 **      fication that data have been successfully delivered to    **
 **      the network.                                              **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_lowerlayer_success(lowerlayer_data_t *lowerlayer_data)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  lowerlayer_success_callback_t emm_callback = lowerlayer_data->success;

  if (emm_callback) {
    rc = (*emm_callback)(lowerlayer_data->args);
    lowerlayer_data->success = NULL;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_lowerlayer_failure()                             **
 **                                                                        **
 ** Description: Handles EMM procedure to be executed upon receiving noti- **
 **      fication that data failed to be delivered to the network. **
 **                                                                        **
 ** Inputs:  is_initial:    true if the NAS message that failed to be  **
 **             transfered is an initial NAS message       **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_lowerlayer_failure(lowerlayer_data_t *lowerlayer_data, bool is_initial)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  lowerlayer_failure_callback_t emm_callback = lowerlayer_data->failure;

  if (emm_callback) {
    rc = (*emm_callback)(is_initial, lowerlayer_data->args);
    lowerlayer_data->failure = NULL;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_lowerlayer_release()                             **
 **                                                                        **
 ** Description: Handles EMM procedure to be executed upon receiving noti- **
 **      fication that NAS signalling connection has been released **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_lowerlayer_release(lowerlayer_data_t *lowerlayer_data)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  lowerlayer_release_callback_t emm_callback = lowerlayer_data->release;

  if (emm_callback) {
    rc = (*emm_callback)(lowerlayer_data->args);
    lowerlayer_data->release = NULL;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_as_set_security_data()                                    **
 **                                                                        **
 ** Description: Setup security data according to the given EPS security   **
 **      context when data transfer to lower layers is requested   **
 **                                                                        **
 ** Inputs:  args:      EPS security context currently in use      **
 **      is_new:    Indicates whether a new security context   **
 **             has just been taken into use               **
 **      is_ciphered:   Indicates whether the NAS message has to   **
 **             be sent ciphered                           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     data:      EPS NAS security data to be setup          **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void emm_as_set_security_data(emm_as_security_data_t *data, const void *args,
                              int is_new, int is_ciphered)
{
  LOG_FUNC_IN;

  const emm_security_context_t *context = (emm_security_context_t *)(args);

  memset(data, 0, sizeof(emm_as_security_data_t));

  if ( context && (context->type != EMM_KSI_NOT_AVAILABLE) ) {
    /* 3GPP TS 24.301, sections 5.4.3.3 and 5.4.3.4
     * Once a valid EPS security context exists and has been taken
     * into use, UE and MME shall cipher and integrity protect all
     * NAS signalling messages with the selected NAS ciphering and
     * NAS integrity algorithms */
    LOG_TRACE(INFO,
              "EPS security context exists is new %u KSI %u SQN %u count %u",
              is_new,
              context->eksi,
              context->ul_count.seq_num,
              *(uint32_t *)(&context->ul_count));
    LOG_TRACE(INFO,
              "knas_int %s",dump_octet_string(&context->knas_int));
    LOG_TRACE(INFO,
              "knas_enc %s",dump_octet_string(&context->knas_enc));
    LOG_TRACE(INFO,
              "kasme %s",dump_octet_string(&context->kasme));

    data->is_new = is_new;
    data->ksi    = context->eksi;
    data->sqn    = context->ul_count.seq_num;
    // LG data->count = *(uint32_t *)(&context->ul_count);
    data->count  = 0x00000000 | (context->ul_count.overflow << 8 ) | context->ul_count.seq_num;
    /* NAS integrity and cyphering keys may not be available if the
     * current security context is a partial EPS security context
     * and not a full native EPS security context */
    data->k_int = &context->knas_int;

    if (is_ciphered) {
      /* 3GPP TS 24.301, sections 4.4.5
       * When the UE establishes a new NAS signalling connection,
       * it shall send initial NAS messages integrity protected
       * and unciphered */
      /* 3GPP TS 24.301, section 5.4.3.2
       * The MME shall send the SECURITY MODE COMMAND message integrity
       * protected and unciphered */
      LOG_TRACE(WARNING,
                "EPS security context exists knas_enc");
      data->k_enc = &context->knas_enc;
    }
  } else {
    LOG_TRACE(WARNING, "EMM_AS_NO_KEY_AVAILABLE");
    /* No valid EPS security context exists */
    data->ksi = EMM_AS_NO_KEY_AVAILABLE;
  }

  LOG_FUNC_OUT;
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

