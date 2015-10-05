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
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_success(unsigned int ueid)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  emm_sap.primitive = EMMREG_LOWERLAYER_SUCCESS;
  emm_sap.u.emm_reg.ueid = ueid;
  emm_sap.u.emm_reg.ctx = NULL;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_failure()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that lower la-  **
 **      yers failed to deliver data to the network                **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_failure(unsigned int ueid)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  emm_sap.primitive = EMMREG_LOWERLAYER_FAILURE;
  emm_sap.u.emm_reg.ueid = ueid;
#if defined(NAS_BUILT_IN_EPC)
  emm_data_context_t *emm_ctx = NULL;
  if (ueid > 0) {
    emm_ctx = emm_data_context_get(&_emm_data, ueid);
  }
  emm_sap.u.emm_reg.ctx = emm_ctx;
#else
  emm_sap.u.emm_reg.ctx = NULL;
#endif
  rc = emm_sap_send(&emm_sap);

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
int lowerlayer_establish(void)
{
  LOG_FUNC_IN;
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
int lowerlayer_release(int cause)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  emm_sap.primitive = EMMREG_LOWERLAYER_RELEASE;
  emm_sap.u.emm_reg.ueid = 0;
  emm_sap.u.emm_reg.ctx = NULL;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_data_ind()                                     **
 **                                                                        **
 ** Description: Notify the EPS Session Management entity that data have   **
 **      been received from lower layers                           **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      data:      Data transfered from lower layers          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_data_ind(unsigned int ueid, const OctetString *data)
{
  esm_sap_t esm_sap;
  int rc;
  emm_data_context_t *emm_ctx = NULL;

  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  if (ueid > 0) {
    emm_ctx = emm_data_context_get(&_emm_data, ueid);
  }

#endif

  esm_sap.primitive = ESM_UNITDATA_IND;
  esm_sap.is_standalone = TRUE;
  esm_sap.ueid = ueid;
  esm_sap.ctx  = emm_ctx;
  esm_sap.recv = data;
  rc = esm_sap_send(&esm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_data_req()                                     **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that data have  **
 **      to be transfered to lower layers                          **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **          data:      Data to be transfered to lower layers      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_data_req(unsigned int ueid, const OctetString *data)
{
  LOG_FUNC_IN;

  int rc;
  emm_sap_t emm_sap;
  emm_security_context_t    *sctx = NULL;
  struct emm_data_context_s *ctx  = NULL;

  emm_sap.primitive = EMMAS_DATA_REQ;
  emm_sap.u.emm_as.u.data.guti = NULL;
  emm_sap.u.emm_as.u.data.ueid = ueid;
# if defined(NAS_BUILT_IN_EPC)

  if (ueid > 0) {
    ctx = emm_data_context_get(&_emm_data, ueid);
  }

# else

  if (ueid < EMM_DATA_NB_UE_MAX) {
    ctx = _emm_data.ctx[ueid];
  }

# endif

  if (ctx) {
    sctx = ctx->security;
  }
  emm_sap.u.emm_as.u.data.NASinfo = 0;
  emm_sap.u.emm_as.u.data.NASmsg.length = data->length;
  emm_sap.u.emm_as.u.data.NASmsg.value = data->value;
  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_sap.u.emm_as.u.data.sctx, sctx, FALSE, TRUE);
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 *              EMM procedure handlers
 * --------------------------------------------------------------------------
 */

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
    data->sqn    = context->dl_count.seq_num;
    // LG data->count = *(uint32_t *)(&context->ul_count);
    data->count  = 0x00000000 | (context->dl_count.overflow << 8 ) | context->dl_count.seq_num;
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

