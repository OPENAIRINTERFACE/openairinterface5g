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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

#include "Asn1Utils.h"
#include "Exceptions.h"

#define ASN1_UTILS_SERIALIZE_BUFFER_SIZE 512

static uint8_t    g_asn1_utils_serialize_buffer[ASN1_UTILS_SERIALIZE_BUFFER_SIZE];
//-----------------------------------------------------------------
DRB_Identity_t* Asn1Utils::Clone(DRB_Identity_t* asn1_objP)
//-----------------------------------------------------------------
{
    if (asn1_objP != NULL) {

        DRB_Identity_t    *clone = static_cast<DRB_Identity_t*>(CALLOC(1,sizeof(DRB_Identity_t)));
        *clone = *asn1_objP;
        return clone;
    } else {
        return NULL;
    }
}
//-----------------------------------------------------------------
DRB_ToAddMod_t* Asn1Utils::Clone(DRB_ToAddMod_t* asn1_objP)
//-----------------------------------------------------------------
{
    if (asn1_objP != NULL) {
        //------------------------------------------------------------------------------
        // ENCODING
        //------------------------------------------------------------------------------
        asn_enc_rval_t enc_rval;
        memset(&g_asn1_utils_serialize_buffer[0], 0, ASN1_UTILS_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_DRB_ToAddMod,
                    (void*)asn1_objP,
                    &g_asn1_utils_serialize_buffer[0],
                    ASN1_UTILS_SERIALIZE_BUFFER_SIZE);
        cout <<"[RRM][Asn1Utils::Clone(DRB_ToAddMod_t*)] asn_DEF_DRB_ToAddMod Encoded "<< enc_rval.encoded <<" bits (" << (enc_rval.encoded+7)/8 << " bytes)" << endl;

        if (enc_rval.encoded==-1) {
            cerr << "[RRM][Asn1Utils::Clone(DRB_ToAddMod_t*)] asn_DEF_DRB_ToAddMod encoding FAILED, EXITING" << endl;
            throw asn1_encoding_error();
        }
        //------------------------------------------------------------------------------
        // DECODING
        //------------------------------------------------------------------------------
        asn_dec_rval_t     rval;
        asn_codec_ctx_t   *opt_codec_ctx = 0;
        DRB_ToAddMod_t    *clone = 0;

        rval = uper_decode(opt_codec_ctx,
                        &asn_DEF_DRB_ToAddMod,/* Type to decode */
                        (void **)&clone,     /* Pointer to a target structure's pointer */
                        &g_asn1_utils_serialize_buffer[0],              /* Data to be decoded */
                        ASN1_UTILS_SERIALIZE_BUFFER_SIZE,              /* Size of data buffer */
                        0,                       /* Number of unused leading bits, 0..7 */
                        0);   /* Number of unused tailing bits, 0..7 */

        if (rval.code != RC_OK) {
            cerr << "[RRM][Asn1Utils::Clone(DRB_ToAddMod_t*)] ASN1 :  ERROR IN ASN1 DECODING" << endl;
            throw asn1_encoding_error();
        }
        return clone;
    } else {
        return NULL;
    }
}
//-----------------------------------------------------------------
SRB_ToAddMod_t* Asn1Utils::Clone(SRB_ToAddMod_t* asn1_objP)
//-----------------------------------------------------------------
{
    if (asn1_objP != NULL) {
        //------------------------------------------------------------------------------
        // ENCODING
        //------------------------------------------------------------------------------
        asn_enc_rval_t enc_rval;
        memset(&g_asn1_utils_serialize_buffer[0], 0, ASN1_UTILS_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_SRB_ToAddMod,
                    (void*)asn1_objP,
                    &g_asn1_utils_serialize_buffer[0],
                    ASN1_UTILS_SERIALIZE_BUFFER_SIZE);
        cout <<"[RRM][Asn1Utils::Clone(SRB_ToAddMod_t*)] asn_DEF_SRB_ToAddMod Encoded "<< enc_rval.encoded <<" bits (" << (enc_rval.encoded+7)/8 << " bytes)" << endl;

        if (enc_rval.encoded==-1) {
            cerr << "[RRM][Asn1Utils::Clone(SRB_ToAddMod_t*)] asn_DEF_SRB_ToAddMod encoding FAILED, EXITING" << endl;
            throw asn1_encoding_error();
        }
        //------------------------------------------------------------------------------
        // DECODING
        //------------------------------------------------------------------------------
        asn_dec_rval_t     rval;
        asn_codec_ctx_t   *opt_codec_ctx = 0;
        SRB_ToAddMod_t    *clone = 0;

        rval = uper_decode(opt_codec_ctx,
                        &asn_DEF_SRB_ToAddMod,/* Type to decode */
                        (void **)&clone,     /* Pointer to a target structure's pointer */
                        &g_asn1_utils_serialize_buffer[0],              /* Data to be decoded */
                        ASN1_UTILS_SERIALIZE_BUFFER_SIZE,              /* Size of data buffer */
                        0,                       /* Number of unused leading bits, 0..7 */
                        0);   /* Number of unused tailing bits, 0..7 */

        if (rval.code != RC_OK) {
            cerr << "[RRM][Asn1Utils::Clone(SRB_ToAddMod_t*)] ASN1 :  ERROR IN ASN1 DECODING" << endl;
            throw asn1_encoding_error();
        }
        return clone;
    } else {
        return NULL;
    }
}
//-----------------------------------------------------------------
MAC_MainConfig_t* Asn1Utils::Clone(MAC_MainConfig_t* asn1_objP)
//-----------------------------------------------------------------
{
    if (asn1_objP != NULL) {
        //------------------------------------------------------------------------------
        // ENCODING
        //------------------------------------------------------------------------------
        asn_enc_rval_t enc_rval;
        memset(&g_asn1_utils_serialize_buffer[0], 0, ASN1_UTILS_SERIALIZE_BUFFER_SIZE);
        enc_rval = uper_encode_to_buffer(&asn_DEF_MAC_MainConfig,
                    (void*)asn1_objP,
                    &g_asn1_utils_serialize_buffer[0],
                    ASN1_UTILS_SERIALIZE_BUFFER_SIZE);
        cout <<"[RRM][Asn1Utils::Clone(MAC_MainConfig_t*)] asn_DEF_MAC_MainConfig Encoded "<< enc_rval.encoded <<" bits (" << (enc_rval.encoded+7)/8 << " bytes)" << endl;

        if (enc_rval.encoded==-1) {
            cerr << "[RRM][Asn1Utils::Clone(MAC_MainConfig_t*)] asn_DEF_MAC_MainConfig encoding FAILED, EXITING" << endl;
            throw asn1_encoding_error();
        }
        //------------------------------------------------------------------------------
        // DECODING
        //------------------------------------------------------------------------------
        asn_dec_rval_t     rval;
        asn_codec_ctx_t   *opt_codec_ctx = 0;
        MAC_MainConfig_t  *clone = 0;

        rval = uper_decode(opt_codec_ctx,
                        &asn_DEF_MAC_MainConfig,/* Type to decode */
                        (void **)&clone,     /* Pointer to a target structure's pointer */
                        &g_asn1_utils_serialize_buffer[0],              /* Data to be decoded */
                        ASN1_UTILS_SERIALIZE_BUFFER_SIZE,              /* Size of data buffer */
                        0,                       /* Number of unused leading bits, 0..7 */
                        0);   /* Number of unused tailing bits, 0..7 */

        if (rval.code != RC_OK) {
            cerr << "[RRM][Asn1Utils::Clone(MAC_MainConfig_t*)] ASN1 :  ERROR IN ASN1 DECODING" << endl;
            throw asn1_encoding_error();
        }
        return clone;
    } else {
        return NULL;
    }
}
