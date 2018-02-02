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
Source      emm_main.c

Version     0.1

Date        2012/10/10

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EPS Mobility Management procedure call manager,
        the main entry point for elementary EMM processing.

*****************************************************************************/

#include "emm_main.h"
#include "nas_log.h"
#include "utils.h"
#include "emmData.h"
#include "MobileIdentity.h"
#include "emm_proc_defs.h"

#include "memory.h"
#include "usim_api.h"
#include "IdleMode.h"

#include <string.h> // memset, memcpy, strlen
#include <stdio.h>  // sprintf
#include <stdlib.h> // malloc, free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/
extern uint8_t usim_test;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static int _emm_main_get_imei(imei_t *imei, const char *imei_str);

static int _emm_main_imsi_cmp(imsi_t *imsi1, imsi_t *imsi2);

static const char *_emm_main_get_plmn(emm_plmn_list_t *emm_plmn_list, const plmn_t *plmn, int index,
                                      int format, size_t *size);

static int _emm_main_get_plmn_index(emm_plmn_list_t *emm_plmn_list, const char *plmn, int format);

/*
 * Callback executed whenever a change in the network has to be notified
 * to the user application
 */
static emm_indication_callback_t _emm_main_user_callback;
static int _emm_main_callback(user_api_id_t *user_api_id, emm_data_t *emm_data, int size);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

  /*
   * Initialize EMM timers
   */
void _emm_timers_initialize(emm_timers_t *emm_timers) {
  emm_timers->T3410.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3410.sec = T3410_DEFAULT_VALUE;
  emm_timers->T3411.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3411.sec = T3411_DEFAULT_VALUE;
  emm_timers->T3402.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3402.sec = T3402_DEFAULT_VALUE;
  emm_timers->T3416.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3416.sec = T3416_DEFAULT_VALUE;
  emm_timers->T3417.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3417.sec = T3417_DEFAULT_VALUE;
  emm_timers->T3418.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3418.sec = T3418_DEFAULT_VALUE;
  emm_timers->T3420.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3420.sec = T3420_DEFAULT_VALUE;
  emm_timers->T3421.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3421.sec = T3421_DEFAULT_VALUE;
  emm_timers->T3423.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3423.sec = T3423_DEFAULT_VALUE;
  emm_timers->T3430.id = NAS_TIMER_INACTIVE_ID;
  emm_timers->T3430.sec = T3430_DEFAULT_VALUE;
}

void _emm_attach_initialize(emm_attach_data_t *emm_attach_data) {
  emm_attach_data->attempt_count = 0;
}

void _emm_detach_initialize(emm_detach_data_t *emm_detach) {
  emm_detach->count = 0;
  emm_detach->switch_off = FALSE;
  emm_detach->type = EMM_DETACH_TYPE_RESERVED;
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes EMM internal data                             **
 **                                                                        **
 ** Inputs:  cb:        The user notification callback             **
 **      imei:      The IMEI read from the UE's non-volatile   **
 **             memory                                     **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ***************************************************************************/
void emm_main_initialize(nas_user_t *user, emm_indication_callback_t cb, const char *imei)
{
  LOG_FUNC_IN;
  user->emm_data = calloc_or_fail(sizeof(emm_data_t));
  /* USIM validity indicator */
  user->emm_data->usim_is_valid = FALSE;
  /* The IMEI read from the UE's non-volatile memory  */
  user->emm_data->imei = (imei_t *)malloc(sizeof(imei_t));
  user->emm_data->imei->length = _emm_main_get_imei(user->emm_data->imei, imei);
  /* The IMSI, valid only if USIM is present */
  user->emm_data->imsi = NULL;
  /* EPS location information */
  user->emm_data->guti = NULL;
  user->emm_data->tai = NULL;
  user->emm_data->ltai.n_tais = 0;
  /* EPS Connection Management status */
  user->emm_data->ecm_status = ECM_IDLE;
  /* Network selection mode of operation */
  user->emm_data->plmn_mode = EMM_DATA_PLMN_AUTO;
  /* Index of the PLMN manually selected by the user */
  user->emm_data->plmn_index = -1;
  /* Selected Radio Access Technology */
  user->emm_data->plmn_rat = NET_ACCESS_UNAVAILABLE;
  /* Selected PLMN */
  memset(&user->emm_data->splmn, 0xFF, sizeof(plmn_t));
  user->emm_data->is_rplmn = FALSE;
  user->emm_data->is_eplmn = FALSE;
  /* Radio Access Technology of the serving cell */
  user->emm_data->rat = NET_ACCESS_UNAVAILABLE;
  /* Network registration status */
  user->emm_data->stat = NET_REG_STATE_OFF;
  user->emm_data->is_attached = FALSE;
  user->emm_data->is_emergency = FALSE;
  /* Location/Tracking area code */
  user->emm_data->tac = 0;  // two byte in hexadecimal format
  /* Identifier of the serving cell */
  user->emm_data->ci = 0;   // four byte in hexadecimal format
  /* List of operators present in the network */
  memset(user->emm_data->plist.buffer, 0, EMM_DATA_BUFFER_SIZE + 1);
  /* Home PLMN */
  memset(&user->emm_data->hplmn, 0xFF, sizeof(plmn_t));
  /* List of Forbidden PLMNs */
  user->emm_data->fplmn.n_plmns = 0;
  /* List of Forbidden PLMNs for GPRS service */
  user->emm_data->fplmn_gprs.n_plmns = 0;
  /* List of Equivalent HPLMNs */
  user->emm_data->ehplmn.n_plmns = 0;
  /* List of user controlled PLMNs */
  user->emm_data->plmn.n_plmns = 0;
  /* List of operator controlled PLMNs */
  user->emm_data->oplmn.n_plmns = 0;
  /* List of operator network name records */
  user->emm_data->n_opnns = 0;
  /* List of Forbidden Tracking Areas */
  user->emm_data->ftai.n_tais = 0;
  /* List of Forbidden Tracking Areas for roaming */
  user->emm_data->ftai_roaming.n_tais = 0;

  /*
   * Get USIM application data
   */
  if ( usim_api_read(user->usim_data_store, &user->usim_data) != RETURNok ) {
    /* The USIM application may not be present or not valid */
    LOG_TRACE(WARNING, "EMM-MAIN  - Failed to read USIM application data");
  } else {
    int i;

    /* The USIM application is present and valid */
    LOG_TRACE(INFO, "EMM-MAIN  - USIM application data successfully read");
    user->emm_data->usim_is_valid = TRUE;

    /* print keys (for debugging) */
    {
      char usim_api_k[256];
      char opc[256];
      sprintf(usim_api_k,
              "%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x "
              "%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x",
              user->usim_data.keys.usim_api_k[0],
              user->usim_data.keys.usim_api_k[1],
              user->usim_data.keys.usim_api_k[2],
              user->usim_data.keys.usim_api_k[3],
              user->usim_data.keys.usim_api_k[4],
              user->usim_data.keys.usim_api_k[5],
              user->usim_data.keys.usim_api_k[6],
              user->usim_data.keys.usim_api_k[7],
              user->usim_data.keys.usim_api_k[8],
              user->usim_data.keys.usim_api_k[9],
              user->usim_data.keys.usim_api_k[10],
              user->usim_data.keys.usim_api_k[11],
              user->usim_data.keys.usim_api_k[12],
              user->usim_data.keys.usim_api_k[13],
              user->usim_data.keys.usim_api_k[14],
              user->usim_data.keys.usim_api_k[15]);
      sprintf(opc,
              "%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x "
              "%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x",
              user->usim_data.keys.opc[0],
              user->usim_data.keys.opc[1],
              user->usim_data.keys.opc[2],
              user->usim_data.keys.opc[3],
              user->usim_data.keys.opc[4],
              user->usim_data.keys.opc[5],
              user->usim_data.keys.opc[6],
              user->usim_data.keys.opc[7],
              user->usim_data.keys.opc[8],
              user->usim_data.keys.opc[9],
              user->usim_data.keys.opc[10],
              user->usim_data.keys.opc[11],
              user->usim_data.keys.opc[12],
              user->usim_data.keys.opc[13],
              user->usim_data.keys.opc[14],
              user->usim_data.keys.opc[15]);
      LOG_TRACE(INFO, "EMM-MAIN  - usim_api_k: %s", usim_api_k);
      LOG_TRACE(INFO, "EMM-MAIN  - opc:        %s", opc);
    }

    /* Get the Home PLMN derived from the IMSI */
    user->emm_data->hplmn.MCCdigit1 = user->usim_data.imsi.u.num.digit1;
    user->emm_data->hplmn.MCCdigit2 = user->usim_data.imsi.u.num.digit2;
    user->emm_data->hplmn.MCCdigit3 = user->usim_data.imsi.u.num.digit3;
    user->emm_data->hplmn.MNCdigit1 = user->usim_data.imsi.u.num.digit4;
    user->emm_data->hplmn.MNCdigit2 = user->usim_data.imsi.u.num.digit5;
    user->emm_data->hplmn.MNCdigit3 = user->usim_data.imsi.u.num.digit6;

    /* Get the list of forbidden PLMNs */
    for (i=0; (i < EMM_DATA_FPLMN_MAX) && (i < USIM_FPLMN_MAX); i++) {
      if ( PLMN_IS_VALID(user->usim_data.fplmn[i]) ) {
        user->emm_data->fplmn.plmn[i] = user->usim_data.fplmn[i];
        user->emm_data->fplmn.n_plmns += 1;
      }
    }

    /* Get the list of Equivalent HPLMNs */
    for (i=0; (i < EMM_DATA_EHPLMN_MAX) && (i < USIM_EHPLMN_MAX); i++) {
      if ( PLMN_IS_VALID(user->usim_data.ehplmn[i]) ) {
        user->emm_data->ehplmn.plmn[i] = user->usim_data.ehplmn[i];
        user->emm_data->ehplmn.n_plmns += 1;
      }
    }

    /* Get the list of User controlled PLMN Selector */
    for (i=0; (i < EMM_DATA_PLMN_MAX) && (i < USIM_PLMN_MAX); i++) {
      if ( PLMN_IS_VALID(user->usim_data.plmn[i].plmn) ) {
        user->emm_data->plmn.plmn[i] = user->usim_data.plmn[i].plmn;
        user->emm_data->userAcT[i] = user->usim_data.plmn[i].AcT;
        user->emm_data->plmn.n_plmns += 1;
      }
    }

    /* Get the list of Operator controlled PLMN Selector */
    for (i=0; (i < EMM_DATA_OPLMN_MAX) && (i < USIM_OPLMN_MAX); i++) {
      if ( PLMN_IS_VALID(user->usim_data.oplmn[i].plmn) ) {
        user->emm_data->oplmn.plmn[i] = user->usim_data.oplmn[i].plmn;
        user->emm_data->operAcT[i] = user->usim_data.oplmn[i].AcT;
        user->emm_data->oplmn.n_plmns += 1;
      }
    }

    /* Get the list of Operator network name records */
    for (i=0; (i < EMM_DATA_OPNN_MAX) && (i < USIM_OPL_MAX); i++) {
      if ( PLMN_IS_VALID(user->usim_data.opl[i].plmn) ) {
        int pnn_id = user->usim_data.opl[i].record_id;
        user->emm_data->opnn[i].plmn = &user->usim_data.opl[i].plmn;
        user->emm_data->opnn[i].fullname = (char *)user->usim_data.pnn[pnn_id].fullname.value;
        user->emm_data->opnn[i].shortname = (char *)user->usim_data.pnn[pnn_id].shortname.value;
        user->emm_data->n_opnns += 1;
      }
    }

    /* TODO: Get the Higher Priority PLMN search period parameter */

    /* Get the EPS location information */
    if (PLMN_IS_VALID(user->usim_data.epsloci.guti.gummei.plmn)) {
      user->emm_data->guti = &user->usim_data.epsloci.guti;
    }

    if (TAI_IS_VALID(user->usim_data.epsloci.tai)) {
      user->emm_data->tai = &user->usim_data.epsloci.tai;
    }

    user->emm_data->status = user->usim_data.epsloci.status;

    /* Get NAS configuration parameters */
    user->emm_data->NAS_SignallingPriority =
      user->usim_data.nasconfig.NAS_SignallingPriority.value[0];
    user->emm_data->NMO_I_Behaviour = user->usim_data.nasconfig.NMO_I_Behaviour.value[0];
    user->emm_data->AttachWithImsi = user->usim_data.nasconfig.AttachWithImsi.value[0];
    user->emm_data->MinimumPeriodicSearchTimer =
      user->usim_data.nasconfig.MinimumPeriodicSearchTimer.value[0];
    user->emm_data->ExtendedAccessBarring =
      user->usim_data.nasconfig.ExtendedAccessBarring.value[0];
    user->emm_data->Timer_T3245_Behaviour =
      user->usim_data.nasconfig.Timer_T3245_Behaviour.value[0];

    /*
     * Get EPS NAS security context
     */
    /* Create NAS security context */
    user->emm_data->security =
      (emm_security_context_t *)malloc(sizeof(emm_security_context_t));

    if (user->emm_data->security != NULL) {
      memset(user->emm_data->security, 0, sizeof(emm_security_context_t));

      /* Type of security context */
      if (user->usim_data.securityctx.KSIasme.value[0] !=
          USIM_KSI_NOT_AVAILABLE) {
        user->emm_data->security->type = EMM_KSI_NATIVE;
      } else {
        user->emm_data->security->type = EMM_KSI_NOT_AVAILABLE;
      }

      /* EPS key set identifier */
      user->emm_data->security->eksi = user->usim_data.securityctx.KSIasme.value[0];
      /* ASME security key */
      user->emm_data->security->kasme.length =
        user->usim_data.securityctx.Kasme.length;
      user->emm_data->security->kasme.value =
        (uint8_t *)malloc(user->emm_data->security->kasme.length);

      if (user->emm_data->security->kasme.value) {
        memcpy(user->emm_data->security->kasme.value,
               user->usim_data.securityctx.Kasme.value,
               user->emm_data->security->kasme.length);
      }

      /* Downlink count parameter */
      if (user->usim_data.securityctx.dlNAScount.length <= sizeof(uint32_t)) {
        memcpy(&user->emm_data->security->dl_count,
               user->usim_data.securityctx.dlNAScount.value,
               user->usim_data.securityctx.dlNAScount.length);
      }

      /* Uplink count parameter */
      if (user->usim_data.securityctx.ulNAScount.length <= sizeof(uint32_t)) {
        memcpy(&user->emm_data->security->ul_count,
               user->usim_data.securityctx.ulNAScount.value,
               user->usim_data.securityctx.ulNAScount.length);
      }

      /* Ciphering algorithm */
      user->emm_data->security->capability.eps_encryption =
        ((user->usim_data.securityctx.algorithmID.value[0] >> 4) & 0xf);
      /* Identity protection algorithm */
      user->emm_data->security->capability.eps_integrity =
        (user->usim_data.securityctx.algorithmID.value[0] & 0xf);
      /* NAS integrity and cyphering keys are not available */
    } else {
      LOG_TRACE(WARNING,
                "EMM-PROC  - Failed to create security context");
    }

    /*
     * Get EMM data from the UE's non-volatile memory
     */
    memset(&user->emm_data->nvdata.rplmn, 0xFF, sizeof(plmn_t));
    user->emm_data->nvdata.eplmn.n_plmns = 0;

    /* Get EMM data stored in the non-volatile memory device */
    int rc = memory_read(user->emm_nvdata_store, &user->emm_data->nvdata, sizeof(emm_nvdata_t));

    if (rc != RETURNok) {
      LOG_TRACE(ERROR, "EMM-MAIN  - Failed to read %s", user->emm_nvdata_store);
      exit(EXIT_FAILURE);
    }

    /* Check the IMSI */
    LOG_TRACE(INFO, "EMM-MAIN  - EMM data successfully read");
    user->emm_data->imsi = &user->usim_data.imsi;
    int imsi_ok = _emm_main_imsi_cmp(&user->emm_data->nvdata.imsi,
                                     &user->usim_data.imsi);

    if (!imsi_ok) {
      LOG_TRACE(WARNING, "EMM-MAIN  - IMSI checking failed nvram: "
                "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x, "
                "usim: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
                user->emm_data->nvdata.imsi.u.value[0],
                user->emm_data->nvdata.imsi.u.value[1],
                user->emm_data->nvdata.imsi.u.value[2],
                user->emm_data->nvdata.imsi.u.value[3],
                user->emm_data->nvdata.imsi.u.value[4],
                user->emm_data->nvdata.imsi.u.value[5],
                user->emm_data->nvdata.imsi.u.value[6],
                user->emm_data->nvdata.imsi.u.value[7],
                user->usim_data.imsi.u.value[0],
                user->usim_data.imsi.u.value[1],
                user->usim_data.imsi.u.value[2],
                user->usim_data.imsi.u.value[3],
                user->usim_data.imsi.u.value[4],
                user->usim_data.imsi.u.value[5],
                user->usim_data.imsi.u.value[6],
                user->usim_data.imsi.u.value[7]);
      memset(&user->emm_data->nvdata.rplmn, 0xFF, sizeof(plmn_t));
      user->emm_data->nvdata.eplmn.n_plmns = 0;
    }
  }

  /*
   * Initialize EMM timers
   */
  user->emm_data->emm_timers = calloc_or_fail(sizeof(emm_timers_t));
  _emm_timers_initialize(user->emm_data->emm_timers);

  /*
   * Initialize Internal data used for detach procedure
   */
  user->emm_data->emm_detach_data = calloc_or_fail(sizeof(emm_detach_data_t));
  _emm_detach_initialize(user->emm_data->emm_detach_data);

  /*
   * Initialize Internal data used for attach procedure
   */
  user->emm_data->emm_attach_data = calloc_or_fail(sizeof(emm_attach_data_t));
  _emm_attach_initialize(user->emm_data->emm_attach_data);

  /*
   * Initialize the user notification callback
   */
  _emm_main_user_callback = *cb;

  /*
   * Initialize EMM internal data used for UE in idle mode
   */
  IdleMode_initialize(user, &_emm_main_callback);

  LOG_FUNC_OUT;
}


/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Mobility Management clean up procedure   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **          Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void emm_main_cleanup(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_data_t *emm_data = user->emm_data;

  if (emm_data->usim_is_valid) {
    /*
     * TODO: Update USIM application data
     */
  }

  /*
   * Store EMM data into the UE's non-volatile memory
   * - Registered PLMN
   * - List of equivalent PLMNs
   */
  int rc = memory_write(user->emm_nvdata_store, &emm_data->nvdata, sizeof(emm_nvdata_t));

  if (rc != RETURNok) {
    LOG_TRACE(ERROR, "EMM-MAIN  - Failed to write %s", user->emm_nvdata_store);
  }

  /* Release dynamically allocated memory */
  if (emm_data->imei) {
    free(emm_data->imei);
    emm_data->imei = NULL;
  }

  if (emm_data->security) {
    emm_security_context_t *security = emm_data->security;

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

    free(emm_data->security);
    emm_data->security = NULL;
  }
  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_imsi()                                       **
 **                                                                        **
 ** Description: Get the International Mobile Subscriber Identity number   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    Pointer to the IMSI                        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const imsi_t *emm_main_get_imsi(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (&emm_data->nvdata.imsi);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_msisdn()                                     **
 **                                                                        **
 ** Description: Get the Mobile Subscriber Dialing Number from the USIM    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    Pointer to the subscriber dialing number   **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const msisdn_t *emm_main_get_msisdn(nas_user_t *user)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (&user->usim_data.msisdn.number);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_set_plmn_selection_mode()                        **
 **                                                                        **
 ** Description: Set the network selection mode of operation to the given  **
 **      mode and update the manually selected network selection   **
 **      data                                                      **
 **                                                                        **
 ** Inputs:  mode:      The specified network selection mode of    **
 **             operation                                  **
 **      format:    The representation format of the PLMN      **
 **             identifier                                 **
 **      plmn:      Identifier of the selected PLMN            **
 **      rat:       The selected Radio Access Techonology      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ***************************************************************************/
int emm_main_set_plmn_selection_mode(nas_user_t *user, int mode, int format,
                                     const network_plmn_t *plmn, int rat)
{
  LOG_FUNC_IN;

  int index;
  emm_data_t *emm_data = user->emm_data;
  emm_plmn_list_t *emm_plmn_list = user->emm_plmn_list;

  LOG_TRACE(INFO, "EMM-MAIN  - PLMN selection: mode=%d, format=%d, plmn=%s, "
            "rat=%d", mode, format, (const char *)&plmn->id, rat);

  emm_data->plmn_mode = mode;

  if (mode != EMM_DATA_PLMN_AUTO) {
    /* Get the index of the PLMN in the list of available PLMNs */
    index = _emm_main_get_plmn_index(emm_plmn_list, (const char *)&plmn->id, format);

    if (index < 0) {
      LOG_TRACE(WARNING, "EMM-MAIN  - PLMN %s not available",
                (const char *)&plmn->id);
    } else {
      /* Update the manually selected network selection data */
      emm_data->plmn_index = index;
      emm_data->plmn_rat = rat;
    }
  } else {
    /*
     * Get the index of the last PLMN the UE already tried to automatically
     * register to when switched on; the equivalent PLMNs list shall not be
     * applied to the user reselection in Automatic Network Selection Mode.
     */
    index = IdleMode_get_hplmn_index(emm_plmn_list);
  }

  LOG_FUNC_RETURN (index);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_selection_mode()                        **
 **                                                                        **
 ** Description: Get the current value of the network selection mode of    **
 **      operation                                                 **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The value of the network selection mode    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_get_plmn_selection_mode(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->plmn_mode);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_list()                                  **
 **                                                                        **
 ** Description: Get the list of available PLMNs                           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ** Outputs:     plist:     Pointer to the list of available PLMNs     **
 **      Return:    The size of the list in bytes              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_get_plmn_list(emm_plmn_list_t *emm_plmn_list, emm_data_t *emm_data, const char **plist)
{
  LOG_FUNC_IN;

  int size = IdleMode_update_plmn_list(emm_plmn_list, emm_data, 0);
  *plist = emm_data->plist.buffer;

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_selected_plmn()                              **
 **                                                                        **
 ** Description: Get the identifier of the currently selected PLMN         **
 **                                                                        **
 ** Inputs:  format:    The requested format of the string repre-  **
 **             sentation of the PLMN identifier           **
 **                                                                        **
 ** Outputs:     plmn:      The selected PLMN identifier coded in the  **
 **             requested format                           **
 **      Return:    A pointer to the string representation of  **
 **             the selected PLMN                          **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *emm_main_get_selected_plmn(emm_plmn_list_t *emm_plmn_list, emm_data_t *emm_data, network_plmn_t *plmn, int format)
{
  LOG_FUNC_IN;

  size_t size = 0;
  /*
   * Get the identifier of the selected PLMN in the list of available PLMNs
   */
  int index = IdleMode_get_splmn_index(emm_plmn_list);

  if ( !(index < 0) ) {
    const char *name = _emm_main_get_plmn(emm_plmn_list, &emm_data->splmn, index,
                                          format, &size);

    if (size > 0) {
      LOG_FUNC_RETURN ((char *) memcpy(&plmn->id, name, size));
    }
  }

  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_registered_plmn()                            **
 **                                                                        **
 ** Description: Get the identifier of the currently registered PLMN       **
 **                                                                        **
 ** Inputs:  format:    The requested format of the string repre-  **
 **             sentation of the PLMN identifier           **
 **                                                                        **
 ** Outputs:     plmn:      The registered PLMN identifier coded in    **
 **             the requested format                       **
 **      Return:    A pointer to the string representation of  **
 **             the registered PLMN                        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *emm_main_get_registered_plmn(emm_plmn_list_t *emm_plmn_list, emm_data_t *emm_data, network_plmn_t *plmn, int format)
{
  LOG_FUNC_IN;

  size_t size = 0;

  /*
   * Get the identifier of the registered PLMN in the list of available PLMNs
   */
  int index = IdleMode_get_rplmn_index(emm_plmn_list);

  if ( !(index < 0) ) {
    const char *name = _emm_main_get_plmn(emm_plmn_list, &emm_data->nvdata.rplmn,
                                          index, format, &size);

    if (size > 0) {
      LOG_FUNC_RETURN ((char *) memcpy(&plmn->id, name, size));
    }
  }

  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_status()                                **
 **                                                                        **
 ** Description: Get the value of the network registration status which    **
 **      shows whether the network has currently indicated the     **
 **      registration of the UE                                    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The current network registration status    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
Stat_t emm_main_get_plmn_status(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->stat);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_tac()                                   **
 **                                                                        **
 ** Description: Get the code of the Tracking area the registered PLMN     **
 **      belongs to                                                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The Location/Tracking area code            **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
tac_t emm_main_get_plmn_tac(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->tac);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_ci()                                    **
 **                                                                        **
 ** Description: Get the identifier of the serving cell                    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The serving cell identifier                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
ci_t emm_main_get_plmn_ci(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->ci);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_rat()                                   **
 **                                                                        **
 ** Description: Get the value of the Radio Access Technology of the ser-  **
 **      ving cell                                                 **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The value of the Radio Access Technology   **
 **             of the serving cell                        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
AcT_t emm_main_get_plmn_rat(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->rat);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_is_attached()                                    **
 **                                                                        **
 ** Description: Indicates whether the UE is currently attached to the     **
 **      network for EPS services or emergency service only        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if the UE is currently attached to    **
 **             the network; FALSE otherwise.              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_is_attached(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->is_attached);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_rat()                                   **
 **                                                                        **
 ** Description: Indicates whether the UE is currently attached to the     **
 **      network for emergency bearer services                     **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    user->emm_data->                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if the UE is currently attached or is **
 **             attempting to attach to the network for    **
 **             emergency bearer services; FALSE otherwise **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_is_emergency(emm_data_t *emm_data)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (emm_data->is_attached && emm_data->is_emergency);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_main_callback()                                      **
 **                                                                        **
 ** Description: Forwards the network indication to the upper control la-  **
 **      yer (user API) to notify that network registration and/or **
 **      location information has changed.                         **
 **                                                                        **
 ** Inputs:  size:      Size in byte of the list of operators      **
 **             present in the network. The list has to be **
 **             displayed to the user application when     **
 **             size > 0.                                  **
 **          Others:    user->emm_data->                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_main_callback(user_api_id_t *user_api_id, emm_data_t *emm_data, int size)
{
  LOG_FUNC_IN;

  /* Forward the notification to the user API */
  int rc = (*_emm_main_user_callback)(user_api_id, emm_data->stat, emm_data->tac,
                                      emm_data->ci, emm_data->rat,
                                      emm_data->plist.buffer, size);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_main_get_imei()                                      **
 **                                                                        **
 ** Description: Returns the International Mobile Equipment Identity con-  **
 **      tained in the given string representation                 **
 **                                                                        **
 ** Inputs:  imei:      The string representation of the IMEI      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     imei:      The IMEI of the UE                         **
 **      Return:    The number of digits in the IMEI           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_main_get_imei(imei_t *imei, const char *imei_str)
{
  int len = strlen(imei_str);

  if (len % 2) {
    imei->u.num.parity = IMEI_ODD_PARITY;
  } else {
    imei->u.num.parity = EVEN_PARITY;
  }

  imei->u.num.digit1 = imei_str[0] - '0';
  imei->u.num.digit2 = imei_str[1] - '0';
  imei->u.num.digit3 = imei_str[2] - '0';
  imei->u.num.digit4 = imei_str[3] - '0';
  imei->u.num.digit5 = imei_str[4] - '0';
  imei->u.num.digit6 = imei_str[5] - '0';
  imei->u.num.digit7 = imei_str[6] - '0';
  imei->u.num.digit8 = imei_str[7] - '0';
  imei->u.num.digit9 = imei_str[8] - '0';
  imei->u.num.digit10 = imei_str[9] - '0';
  imei->u.num.digit11 = imei_str[10] - '0';
  imei->u.num.digit12 = imei_str[11] - '0';
  imei->u.num.digit13 = imei_str[12] - '0';
  imei->u.num.digit14 = imei_str[13] - '0';
  imei->u.num.digit15 = imei_str[14] - '0';
  return (len);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_main_imsi_cmp()                                      **
 **                                                                        **
 ** Description: Compares two International Mobile Subscriber Identifiers  **
 **                                                                        **
 ** Inputs:  imsi1:     The first IMSI                             **
 **      imsi2:     The second IMSI to compare to              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if the first IMSI is found to match   **
 **             the second; FALSE otherwise.               **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_main_imsi_cmp(imsi_t *imsi1, imsi_t *imsi2)
{
  int i;

  if (imsi1->length != imsi2->length) {
    return FALSE;
  }

  for (i = 0; i < imsi1->length; i++) {
    if (imsi1->u.value[i] != imsi2->u.value[i]) {
      return FALSE;
    }
  }

  return TRUE;
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_main_get_plmn()                                      **
 **                                                                        **
 ** Description: Get the identifier of the PLMN at the given index in the  **
 **      list of available PLMNs.                                  **
 **                                                                        **
 ** Inputs:  plmn:      The PLMN to search for                     **
 **      index:     The index of the PLMN in the list of PLMNs **
 **      format:    The requested representation format of the **
 **             PLMN identifier                            **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     size:      The size in bytes of the PLMN identifier   **
 **             coded in the requested format              **
 **      Return:    A pointer to the identifier of the PLMN    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static const char *_emm_main_get_plmn(emm_plmn_list_t *emm_plmn_list, const plmn_t *plmn, int index,
                                      int format, size_t *size)
{
  if ( PLMN_IS_VALID(*plmn) ) {
    switch (format) {
    case NET_FORMAT_LONG:
      /* Get the long alpha-numeric representation of the PLMN */
      return IdleMode_get_plmn_fullname(emm_plmn_list, plmn, index, size);
      break;

    case NET_FORMAT_SHORT:
      /* Get the short alpha-numeric representation of the PLMN */
      return IdleMode_get_plmn_shortname(emm_plmn_list, plmn, index, size);
      break;

    case NET_FORMAT_NUM:
      /* Get the numeric representation of the PLMN */
      return IdleMode_get_plmn_id(emm_plmn_list, plmn, index, size);
      break;

    default:
      LOG_TRACE(WARNING, "EMM-MAIN  - Format is not valid (%d)",
                format);
      *size = 0;
      break;
    }
  }

  return (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_main_get_plmn_index()                                **
 **                                                                        **
 ** Description: Get the index of the given PLMN in the ordered list of    **
 **      available PLMNs                                           **
 **                                                                        **
 ** Inputs:  plmn:      Identifier of the PLMN                     **
 **      format:    The representation format of the PLMN      **
 **             identifier                                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the selected PLMN in the list **
 **             of available PLMNs; -1 if the PLMN is not  **
 **             found                                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_main_get_plmn_index(emm_plmn_list_t *emm_plmn_list, const char *plmn, int format)
{
  int index = -1;

  switch (format) {
  case NET_FORMAT_LONG:
    /* Get the index of the long alpha-numeric PLMN identifier */
    index = IdleMode_get_plmn_fullname_index(emm_plmn_list, plmn);
    break;

  case NET_FORMAT_SHORT:
    /* Get the index of the short alpha-numeric PLMN identifier */
    index = IdleMode_get_plmn_shortname_index(emm_plmn_list, plmn);
    break;

  case NET_FORMAT_NUM:
    /* Get the index of the numeric PLMN identifier */
    index = IdleMode_get_plmn_id_index(emm_plmn_list, plmn);
    break;

  default:
    LOG_TRACE(WARNING, "EMM-MAIN  - Format is not valid (%d)", format);
    break;
  }

  return (index);
}

