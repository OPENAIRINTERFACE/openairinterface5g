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
Source      Identification.c

Version     0.1

Date        2013/04/09

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the identification EMM procedure executed by the
        Non-Access Stratum.

        The identification procedure is used by the network to request
        a particular UE to provide specific identification parameters
        (IMSI, IMEI).

*****************************************************************************/

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "msc.h"

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the requested identity type */
static const char *_emm_identity_type_str[] = {
  "NOT AVAILABLE", "IMSI", "IMEI", "IMEISV", "TMSI"
};

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the identification procedure in the UE
 * --------------------------------------------------------------------------
 */


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      Identification procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_identification_request()                         **
 **                                                                        **
 ** Description: Performs the MME requested identification procedure.      **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.4.3                           **
 **      Upon receiving the IDENTITY REQUEST message, the UE shall **
 **      send  an  IDENTITY  RESPONSE  message to the network. The **
 **      IDENTITY RESPONSE message shall contain the identifica-   **
 **      tion parameters as requested by the network.              **
 **                                                                        **
 ** Inputs:  type:      Type of the requested identity             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_identification_request(emm_proc_identity_type_t type)
{
  LOG_FUNC_IN;

  int rc;
  emm_sap_t emm_sap;

  LOG_TRACE(INFO, "EMM-PROC  - Identification requested type = %s (%d)",
            _emm_identity_type_str[type], type);

  /* Setup EMM procedure handler to be executed upon receiving
   * lower layer notification */
  rc = emm_proc_lowerlayer_initialize(NULL, NULL, NULL, NULL);

  if (rc != RETURNok) {
    LOG_TRACE(WARNING,
              "EMM-PROC  - Failed to initialize EMM procedure handler");
    LOG_FUNC_RETURN (RETURNerror);
  }

  emm_sap.u.emm_as.u.security.identType = EMM_IDENT_TYPE_NOT_AVAILABLE;

  switch (type) {
  case EMM_IDENT_TYPE_IMSI: {
    imsi_t modified_imsi;

    /* International Mobile Subscriber Identity is requested */
    if (_emm_data.imsi) {
      memcpy (&modified_imsi, _emm_data.imsi, sizeof (modified_imsi));

      /* LW: Eventually replace the 0xF value set in MNC digit 3 by a 0 to avoid IMSI to be truncated before reaching HSS */
      if (modified_imsi.u.num.digit6 == 0xF) {
        modified_imsi.u.num.digit6 = 0;
      }

      emm_sap.u.emm_as.u.security.identType = EMM_IDENT_TYPE_IMSI;
      emm_sap.u.emm_as.u.security.imsi = &modified_imsi;

      LOG_TRACE(INFO, "EMM-PROC  - IMSI = %u%u%u %u%u%u %u%u%u%u%x%x%x%x%x",
                emm_sap.u.emm_as.u.security.imsi->u.num.digit1,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit2,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit3,

                emm_sap.u.emm_as.u.security.imsi->u.num.digit4,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit5,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit6,

                emm_sap.u.emm_as.u.security.imsi->u.num.digit7,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit8,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit9,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit10,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit11,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit12,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit13,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit14,
                emm_sap.u.emm_as.u.security.imsi->u.num.digit15);
    }

    break;
  }

  case EMM_IDENT_TYPE_IMEI:

    /* International Mobile Equipment Identity is requested */
    if (_emm_data.imei) {
      emm_sap.u.emm_as.u.security.identType = EMM_IDENT_TYPE_IMEI;
      emm_sap.u.emm_as.u.security.imei = _emm_data.imei;
    }

    break;

  case EMM_IDENT_TYPE_TMSI:

    /* Temporary Mobile Subscriber Identity is requested */
    if (_emm_data.guti) {
      emm_sap.u.emm_as.u.security.identType = EMM_IDENT_TYPE_TMSI;
      emm_sap.u.emm_as.u.security.tmsi = _emm_data.guti->m_tmsi;
    }

    break;

  default:
    /* Other identities are not available */
    break;
  }

  /*
   * Notify EMM-AS SAP that Identity Response message has to be sent
   * to the MME
   */
  emm_sap.primitive = EMMAS_SECURITY_RES;
  emm_sap.u.emm_as.u.security.guti = _emm_data.guti;
  emm_sap.u.emm_as.u.security.ueid = 0;
  emm_sap.u.emm_as.u.security.msgType = EMM_AS_MSG_TYPE_IDENT;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.security.sctx,
                           _emm_data.security, FALSE, TRUE);
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


