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
