/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
#include "emmData.h"
#include "MobileIdentity.h"

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

static const char *_emm_main_get_plmn(const plmn_t *plmn, int index,
                                      int format, size_t *size);

static int _emm_main_get_plmn_index(const char *plmn, int format);

/*
 * USIM application data
 */
static usim_data_t _usim_data;

/*
 * Callback executed whenever a change in the network has to be notified
 * to the user application
 */
static emm_indication_callback_t _emm_main_user_callback;
static int _emm_main_callback(int);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes EMM internal data                             **
 **                                                                        **
 ** Inputs:  cb:        The user notification callback             **
 **      imei:      The IMEI read from the UE's non-volatile   **
 **             memory                                     **
 **      Others:    _usim_data                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
void emm_main_initialize(emm_indication_callback_t cb, const char *imei)
{
  LOG_FUNC_IN;

  /* USIM validity indicator */
  _emm_data.usim_is_valid = FALSE;
  /* The IMEI read from the UE's non-volatile memory  */
  _emm_data.imei = (imei_t *)malloc(sizeof(imei_t));
  _emm_data.imei->length = _emm_main_get_imei(_emm_data.imei, imei);
  /* The IMSI, valid only if USIM is present */
  _emm_data.imsi = NULL;
  /* EPS location information */
  _emm_data.guti = NULL;
  _emm_data.tai = NULL;
  _emm_data.ltai.n_tais = 0;
  /* EPS Connection Management status */
  _emm_data.ecm_status = ECM_IDLE;
  /* Network selection mode of operation */
  _emm_data.plmn_mode = EMM_DATA_PLMN_AUTO;
  /* Index of the PLMN manually selected by the user */
  _emm_data.plmn_index = -1;
  /* Selected Radio Access Technology */
  _emm_data.plmn_rat = NET_ACCESS_UNAVAILABLE;
  /* Selected PLMN */
  memset(&_emm_data.splmn, 0xFF, sizeof(plmn_t));
  _emm_data.is_rplmn = FALSE;
  _emm_data.is_eplmn = FALSE;
  /* Radio Access Technology of the serving cell */
  _emm_data.rat = NET_ACCESS_UNAVAILABLE;
  /* Network registration status */
  _emm_data.stat = NET_REG_STATE_OFF;
  _emm_data.is_attached = FALSE;
  _emm_data.is_emergency = FALSE;
  /* Location/Tracking area code */
  _emm_data.tac = 0;  // two byte in hexadecimal format
  /* Identifier of the serving cell */
  _emm_data.ci = 0;   // four byte in hexadecimal format
  /* List of operators present in the network */
  memset(_emm_data.plist.buffer, 0, EMM_DATA_BUFFER_SIZE + 1);
  /* Home PLMN */
  memset(&_emm_data.hplmn, 0xFF, sizeof(plmn_t));
  /* List of Forbidden PLMNs */
  _emm_data.fplmn.n_plmns = 0;
  /* List of Forbidden PLMNs for GPRS service */
  _emm_data.fplmn_gprs.n_plmns = 0;
  /* List of Equivalent HPLMNs */
  _emm_data.ehplmn.n_plmns = 0;
  /* List of user controlled PLMNs */
  _emm_data.plmn.n_plmns = 0;
  /* List of operator controlled PLMNs */
  _emm_data.oplmn.n_plmns = 0;
  /* List of operator network name records */
  _emm_data.n_opnns = 0;
  /* List of Forbidden Tracking Areas */
  _emm_data.ftai.n_tais = 0;
  /* List of Forbidden Tracking Areas for roaming */
  _emm_data.ftai_roaming.n_tais = 0;

  /*
   * Get USIM application data
   */
  _usim_data.usimtestmode = usim_test;
  if ( usim_api_read(&_usim_data) != RETURNok ) {
    /* The USIM application may not be present or not valid */
    LOG_TRACE(WARNING, "EMM-MAIN  - Failed to read USIM application data");
  } else {
    int i;

    /* The USIM application is present and valid */
    LOG_TRACE(INFO, "EMM-MAIN  - USIM application data successfully read");
    _emm_data.usim_is_valid = TRUE;

    /* Get the Home PLMN derived from the IMSI */
    _emm_data.hplmn.MCCdigit1 = _usim_data.imsi.u.num.digit1;
    _emm_data.hplmn.MCCdigit2 = _usim_data.imsi.u.num.digit2;
    _emm_data.hplmn.MCCdigit3 = _usim_data.imsi.u.num.digit3;
    _emm_data.hplmn.MNCdigit1 = _usim_data.imsi.u.num.digit4;
    _emm_data.hplmn.MNCdigit2 = _usim_data.imsi.u.num.digit5;
    _emm_data.hplmn.MNCdigit3 = _usim_data.imsi.u.num.digit6;

    /* Get the list of forbidden PLMNs */
    for (i=0; (i < EMM_DATA_FPLMN_MAX) && (i < USIM_FPLMN_MAX); i++) {
      if ( PLMN_IS_VALID(_usim_data.fplmn[i]) ) {
        _emm_data.fplmn.plmn[i] = _usim_data.fplmn[i];
        _emm_data.fplmn.n_plmns += 1;
      }
    }

    /* Get the list of Equivalent HPLMNs */
    for (i=0; (i < EMM_DATA_EHPLMN_MAX) && (i < USIM_EHPLMN_MAX); i++) {
      if ( PLMN_IS_VALID(_usim_data.ehplmn[i]) ) {
        _emm_data.ehplmn.plmn[i] = _usim_data.ehplmn[i];
        _emm_data.ehplmn.n_plmns += 1;
      }
    }

    /* Get the list of User controlled PLMN Selector */
    for (i=0; (i < EMM_DATA_PLMN_MAX) && (i < USIM_PLMN_MAX); i++) {
      if ( PLMN_IS_VALID(_usim_data.plmn[i].plmn) ) {
        _emm_data.plmn.plmn[i] = _usim_data.plmn[i].plmn;
        _emm_data.userAcT[i] = _usim_data.plmn[i].AcT;
        _emm_data.plmn.n_plmns += 1;
      }
    }

    /* Get the list of Operator controlled PLMN Selector */
    for (i=0; (i < EMM_DATA_OPLMN_MAX) && (i < USIM_OPLMN_MAX); i++) {
      if ( PLMN_IS_VALID(_usim_data.oplmn[i].plmn) ) {
        _emm_data.oplmn.plmn[i] = _usim_data.oplmn[i].plmn;
        _emm_data.operAcT[i] = _usim_data.oplmn[i].AcT;
        _emm_data.oplmn.n_plmns += 1;
      }
    }

    /* Get the list of Operator network name records */
    for (i=0; (i < EMM_DATA_OPNN_MAX) && (i < USIM_OPL_MAX); i++) {
      if ( PLMN_IS_VALID(_usim_data.opl[i].plmn) ) {
        int pnn_id = _usim_data.opl[i].record_id;
        _emm_data.opnn[i].plmn = &_usim_data.opl[i].plmn;
        _emm_data.opnn[i].fullname = (char *)_usim_data.pnn[pnn_id].fullname.value;
        _emm_data.opnn[i].shortname = (char *)_usim_data.pnn[pnn_id].shortname.value;
        _emm_data.n_opnns += 1;
      }
    }

    /* TODO: Get the Higher Priority PLMN search period parameter */

    /* Get the EPS location information */
    if (PLMN_IS_VALID(_usim_data.epsloci.guti.gummei.plmn)) {
      _emm_data.guti = &_usim_data.epsloci.guti;
    }

    if (TAI_IS_VALID(_usim_data.epsloci.tai)) {
      _emm_data.tai = &_usim_data.epsloci.tai;
    }

    _emm_data.status = _usim_data.epsloci.status;

    /* Get NAS configuration parameters */
    _emm_data.NAS_SignallingPriority =
      _usim_data.nasconfig.NAS_SignallingPriority.value[0];
    _emm_data.NMO_I_Behaviour = _usim_data.nasconfig.NMO_I_Behaviour.value[0];
    _emm_data.AttachWithImsi = _usim_data.nasconfig.AttachWithImsi.value[0];
    _emm_data.MinimumPeriodicSearchTimer =
      _usim_data.nasconfig.MinimumPeriodicSearchTimer.value[0];
    _emm_data.ExtendedAccessBarring =
      _usim_data.nasconfig.ExtendedAccessBarring.value[0];
    _emm_data.Timer_T3245_Behaviour =
      _usim_data.nasconfig.Timer_T3245_Behaviour.value[0];

    /*
     * Get EPS NAS security context
     */
    /* Create NAS security context */
    _emm_data.security =
      (emm_security_context_t *)malloc(sizeof(emm_security_context_t));

    if (_emm_data.security != NULL) {
      memset(_emm_data.security, 0, sizeof(emm_security_context_t));

      /* Type of security context */
      if (_usim_data.securityctx.KSIasme.value[0] !=
          USIM_KSI_NOT_AVAILABLE) {
        _emm_data.security->type = EMM_KSI_NATIVE;
      } else {
        _emm_data.security->type = EMM_KSI_NOT_AVAILABLE;
      }

      /* EPS key set identifier */
      _emm_data.security->eksi = _usim_data.securityctx.KSIasme.value[0];
      /* ASME security key */
      _emm_data.security->kasme.length =
        _usim_data.securityctx.Kasme.length;
      _emm_data.security->kasme.value =
        (uint8_t *)malloc(_emm_data.security->kasme.length);

      if (_emm_data.security->kasme.value) {
        memcpy(_emm_data.security->kasme.value,
               _usim_data.securityctx.Kasme.value,
               _emm_data.security->kasme.length);
      }

      /* Downlink count parameter */
      if (_usim_data.securityctx.dlNAScount.length <= sizeof(uint32_t)) {
        memcpy(&_emm_data.security->dl_count,
               _usim_data.securityctx.dlNAScount.value,
               _usim_data.securityctx.dlNAScount.length);
      }

      /* Uplink count parameter */
      if (_usim_data.securityctx.ulNAScount.length <= sizeof(uint32_t)) {
        memcpy(&_emm_data.security->ul_count,
               _usim_data.securityctx.ulNAScount.value,
               _usim_data.securityctx.ulNAScount.length);
      }

      /* Ciphering algorithm */
      _emm_data.security->capability.eps_encryption =
        ((_usim_data.securityctx.algorithmID.value[0] >> 4) & 0xf);
      /* Identity protection algorithm */
      _emm_data.security->capability.eps_integrity =
        (_usim_data.securityctx.algorithmID.value[0] & 0xf);
      /* NAS integrity and cyphering keys are not available */
    } else {
      LOG_TRACE(WARNING,
                "EMM-PROC  - Failed to create security context");
    }

    /*
     * Get EMM data from the UE's non-volatile memory
     */
    memset(&_emm_data.nvdata.rplmn, 0xFF, sizeof(plmn_t));
    _emm_data.nvdata.eplmn.n_plmns = 0;
    /* Get EMM data pathname */
    char *path = memory_get_path(EMM_NVRAM_DIRNAME, EMM_NVRAM_FILENAME);

    if (path == NULL) {
      LOG_TRACE(ERROR, "EMM-MAIN  - Failed to get EMM data pathname");
    } else {
      /* Get EMM data stored in the non-volatile memory device */
      int rc = memory_read(path, &_emm_data.nvdata, sizeof(emm_nvdata_t));

      if (rc != RETURNok) {
        LOG_TRACE(ERROR, "EMM-MAIN  - Failed to read %s", path);
      } else {
        /* Check the IMSI */
        LOG_TRACE(INFO, "EMM-MAIN  - EMM data successfully read");
        _emm_data.imsi = &_usim_data.imsi;
        int imsi_ok = _emm_main_imsi_cmp(&_emm_data.nvdata.imsi,
                                         &_usim_data.imsi);

        if (!imsi_ok) {
          LOG_TRACE(WARNING, "EMM-MAIN  - IMSI checking failed nvram: "
                    "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x, "
                    "usim: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
                    _emm_data.nvdata.imsi.u.value[0],
                    _emm_data.nvdata.imsi.u.value[1],
                    _emm_data.nvdata.imsi.u.value[2],
                    _emm_data.nvdata.imsi.u.value[3],
                    _emm_data.nvdata.imsi.u.value[4],
                    _emm_data.nvdata.imsi.u.value[5],
                    _emm_data.nvdata.imsi.u.value[6],
                    _emm_data.nvdata.imsi.u.value[7],
                    _usim_data.imsi.u.value[0],
                    _usim_data.imsi.u.value[1],
                    _usim_data.imsi.u.value[2],
                    _usim_data.imsi.u.value[3],
                    _usim_data.imsi.u.value[4],
                    _usim_data.imsi.u.value[5],
                    _usim_data.imsi.u.value[6],
                    _usim_data.imsi.u.value[7]);
          memset(&_emm_data.nvdata.rplmn, 0xFF, sizeof(plmn_t));
          _emm_data.nvdata.eplmn.n_plmns = 0;
        }
      }

      free(path);
    }
  }

  /*
   * Initialize EMM timers
   */
  T3410.id = NAS_TIMER_INACTIVE_ID;
  T3410.sec = T3410_DEFAULT_VALUE;
  T3411.id = NAS_TIMER_INACTIVE_ID;
  T3411.sec = T3411_DEFAULT_VALUE;
  T3402.id = NAS_TIMER_INACTIVE_ID;
  T3402.sec = T3402_DEFAULT_VALUE;
  T3416.id = NAS_TIMER_INACTIVE_ID;
  T3416.sec = T3416_DEFAULT_VALUE;
  T3417.id = NAS_TIMER_INACTIVE_ID;
  T3417.sec = T3417_DEFAULT_VALUE;
  T3418.id = NAS_TIMER_INACTIVE_ID;
  T3418.sec = T3418_DEFAULT_VALUE;
  T3420.id = NAS_TIMER_INACTIVE_ID;
  T3420.sec = T3420_DEFAULT_VALUE;
  T3421.id = NAS_TIMER_INACTIVE_ID;
  T3421.sec = T3421_DEFAULT_VALUE;
  T3423.id = NAS_TIMER_INACTIVE_ID;
  T3423.sec = T3423_DEFAULT_VALUE;
  T3430.id = NAS_TIMER_INACTIVE_ID;
  T3430.sec = T3430_DEFAULT_VALUE;

  /*
   * Initialize the user notification callback
   */
  _emm_main_user_callback = *cb;

  /*
   * Initialize EMM internal data used for UE in idle mode
   */
  IdleMode_initialize(&_emm_main_callback);

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
void emm_main_cleanup(void)
{
  LOG_FUNC_IN;

  if (_emm_data.usim_is_valid) {
    /*
     * TODO: Update USIM application data
     */
#if 0
    int i;

    /* Update the list of Forbidden PLMNs */
    for (i=0; (i < _emm_data.fplmn.n_plmns) && (i < USIM_FPLMN_MAX); i++) {
      _usim_data.fplmn[i] = _emm_data.fplmn.plmn[i];
    }

    /* Update the list of Equivalent HPLMNs */
    for (i=0; (i < _emm_data.ehplmn.n_plmns) && (i < USIM_EHPLMN_MAX); i++) {
      _usim_data.ehplmn[i] = _emm_data.ehplmn.plmn[i];
    }

    /* Update the GUTI */
    if (_emm_data.guti) {
      _usim_data.epsloci.guti = *(_emm_data.guti);
    }

    /* Update the last visited registered TAI */
    if (_emm_data.tai) {
      _usim_data.epsloci.tai = *(_emm_data.tai);
    }

    /* Update the EPS location information */
    _usim_data.epsloci.status = _emm_data.status;

    if (_emm_data.security && (_emm_data.security->type == EMM_KSI_NATIVE)) {
      /* TODO: Update the EPS security context parameters from the full
       * native EPS security context */
    }

    /*
     * Store USIM application data
     * - List of forbidden PLMNs
     */
    if ( usim_api_write(&_usim_data) != RETURNok ) {
      /* The USIM application may not be present or not valid */
      LOG_TRACE(WARNING, "EMM-MAIN  - "
                "Failed to write USIM application data");
    }

#endif
  }

  /*
   * Store EMM data into the UE's non-volatile memory
   * - Registered PLMN
   * - List of equivalent PLMNs
   */
  char *path = memory_get_path(EMM_NVRAM_DIRNAME, EMM_NVRAM_FILENAME);

  if (path == NULL) {
    LOG_TRACE(ERROR, "EMM-MAIN  - Failed to get EMM data pathname");
  } else {
    int rc = memory_write(path, &_emm_data.nvdata, sizeof(emm_nvdata_t));

    if (rc != RETURNok) {
      LOG_TRACE(ERROR, "EMM-MAIN  - Failed to write %s", path);
    }
  }

  /* Release dynamically allocated memory */
  if (_emm_data.imei) {
    free(_emm_data.imei);
    _emm_data.imei = NULL;
  }

  if (_emm_data.security) {
    emm_security_context_t *security = _emm_data.security;

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

    free(_emm_data.security);
    _emm_data.security = NULL;
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
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    Pointer to the IMSI                        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const imsi_t *emm_main_get_imsi(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (&_emm_data.nvdata.imsi);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_msisdn()                                     **
 **                                                                        **
 ** Description: Get the Mobile Subscriber Dialing Number from the USIM    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _usim_data                                 **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    Pointer to the subscriber dialing number   **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const msisdn_t *emm_main_get_msisdn(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (&_usim_data.msisdn.number);
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
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_main_set_plmn_selection_mode(int mode, int format,
                                     const network_plmn_t *plmn, int rat)
{
  LOG_FUNC_IN;

  int index;

  LOG_TRACE(INFO, "EMM-MAIN  - PLMN selection: mode=%d, format=%d, plmn=%s, "
            "rat=%d", mode, format, (const char *)&plmn->id, rat);

  _emm_data.plmn_mode = mode;

  if (mode != EMM_DATA_PLMN_AUTO) {
    /* Get the index of the PLMN in the list of available PLMNs */
    index = _emm_main_get_plmn_index((const char *)&plmn->id, format);

    if (index < 0) {
      LOG_TRACE(WARNING, "EMM-MAIN  - PLMN %s not available",
                (const char *)&plmn->id);
    } else {
      /* Update the manually selected network selection data */
      _emm_data.plmn_index = index;
      _emm_data.plmn_rat = rat;
    }
  } else {
    /*
     * Get the index of the last PLMN the UE already tried to automatically
     * register to when switched on; the equivalent PLMNs list shall not be
     * applied to the user reselection in Automatic Network Selection Mode.
     */
    index = IdleMode_get_hplmn_index();
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
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The value of the network selection mode    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_get_plmn_selection_mode(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.plmn_mode);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_list()                                  **
 **                                                                        **
 ** Description: Get the list of available PLMNs                           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     plist:     Pointer to the list of available PLMNs     **
 **      Return:    The size of the list in bytes              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_get_plmn_list(const char **plist)
{
  LOG_FUNC_IN;

  int size = IdleMode_update_plmn_list(0);
  *plist = _emm_data.plist.buffer;

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
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     plmn:      The selected PLMN identifier coded in the  **
 **             requested format                           **
 **      Return:    A pointer to the string representation of  **
 **             the selected PLMN                          **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *emm_main_get_selected_plmn(network_plmn_t *plmn, int format)
{
  LOG_FUNC_IN;

  size_t size = 0;
  /*
   * Get the identifier of the selected PLMN in the list of available PLMNs
   */
  int index = IdleMode_get_splmn_index();

  if ( !(index < 0) ) {
    const char *name = _emm_main_get_plmn(&_emm_data.splmn, index,
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
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     plmn:      The registered PLMN identifier coded in    **
 **             the requested format                       **
 **      Return:    A pointer to the string representation of  **
 **             the registered PLMN                        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *emm_main_get_registered_plmn(network_plmn_t *plmn, int format)
{
  LOG_FUNC_IN;

  size_t size = 0;

  /*
   * Get the identifier of the registered PLMN in the list of available PLMNs
   */
  int index = IdleMode_get_rplmn_index();

  if ( !(index < 0) ) {
    const char *name = _emm_main_get_plmn(&_emm_data.nvdata.rplmn,
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
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The current network registration status    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
Stat_t emm_main_get_plmn_status(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.stat);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_tac()                                   **
 **                                                                        **
 ** Description: Get the code of the Tracking area the registered PLMN     **
 **      belongs to                                                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The Location/Tracking area code            **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
tac_t emm_main_get_plmn_tac(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.tac);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_ci()                                    **
 **                                                                        **
 ** Description: Get the identifier of the serving cell                    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The serving cell identifier                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
ci_t emm_main_get_plmn_ci(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.ci);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_rat()                                   **
 **                                                                        **
 ** Description: Get the value of the Radio Access Technology of the ser-  **
 **      ving cell                                                 **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The value of the Radio Access Technology   **
 **             of the serving cell                        **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
AcT_t emm_main_get_plmn_rat(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.rat);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_is_attached()                                    **
 **                                                                        **
 ** Description: Indicates whether the UE is currently attached to the     **
 **      network for EPS services or emergency service only        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if the UE is currently attached to    **
 **             the network; FALSE otherwise.              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_is_attached(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.is_attached);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_get_plmn_rat()                                   **
 **                                                                        **
 ** Description: Indicates whether the UE is currently attached to the     **
 **      network for emergency bearer services                     **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if the UE is currently attached or is **
 **             attempting to attach to the network for    **
 **             emergency bearer services; FALSE otherwise **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_main_is_emergency(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (_emm_data.is_attached && _emm_data.is_emergency);
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
 **          Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_main_callback(int size)
{
  LOG_FUNC_IN;

  /* Forward the notification to the user API */
  int rc = (*_emm_main_user_callback)(_emm_data.stat, _emm_data.tac,
                                      _emm_data.ci, _emm_data.rat,
                                      _emm_data.plist.buffer, size);

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
static const char *_emm_main_get_plmn(const plmn_t *plmn, int index,
                                      int format, size_t *size)
{
  if ( PLMN_IS_VALID(*plmn) ) {
    switch (format) {
    case NET_FORMAT_LONG:
      /* Get the long alpha-numeric representation of the PLMN */
      return IdleMode_get_plmn_fullname(plmn, index, size);
      break;

    case NET_FORMAT_SHORT:
      /* Get the short alpha-numeric representation of the PLMN */
      return IdleMode_get_plmn_shortname(plmn, index, size);
      break;

    case NET_FORMAT_NUM:
      /* Get the numeric representation of the PLMN */
      return IdleMode_get_plmn_id(plmn, index, size);
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
static int _emm_main_get_plmn_index(const char *plmn, int format)
{
  int index = -1;

  switch (format) {
  case NET_FORMAT_LONG:
    /* Get the index of the long alpha-numeric PLMN identifier */
    index = IdleMode_get_plmn_fullname_index(plmn);
    break;

  case NET_FORMAT_SHORT:
    /* Get the index of the short alpha-numeric PLMN identifier */
    index = IdleMode_get_plmn_shortname_index(plmn);
    break;

  case NET_FORMAT_NUM:
    /* Get the index of the numeric PLMN identifier */
    index = IdleMode_get_plmn_id_index(plmn);
    break;

  default:
    LOG_TRACE(WARNING, "EMM-MAIN  - Format is not valid (%d)", format);
    break;
  }

  return (index);
}

