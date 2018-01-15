/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _NFAPI_H_
#define _NFAPI_H_

#if defined(__cplusplus)
extern "C" {
#endif

// todo : move to public_inc so can be used by vendor extensions

#define MAX_BAD_TAG 3

uint8_t push8(uint8_t in, uint8_t **out, uint8_t *end);
uint8_t pushs8(int8_t in, uint8_t **out, uint8_t *end);
uint8_t push16(uint16_t in, uint8_t **out, uint8_t *end);
uint8_t pushs16(int16_t in, uint8_t **out, uint8_t *end);
uint8_t push32(uint32_t in, uint8_t **out, uint8_t *end);
uint8_t pushs32(int32_t in, uint8_t **out, uint8_t *end);

uint8_t pull8(uint8_t **in, uint8_t *out, uint8_t *end);
uint8_t pulls8(uint8_t **in, int8_t *out, uint8_t *end);
uint8_t pull16(uint8_t **in, uint16_t *out, uint8_t *end);
uint8_t pulls16(uint8_t **in, int16_t *out, uint8_t *end);
uint8_t pull32(uint8_t **in, uint32_t *out, uint8_t *end);
uint8_t pulls32(uint8_t **in, int32_t *out, uint8_t *end);


uint32_t pullarray8(uint8_t **in, uint8_t out[], uint32_t max_len, uint32_t len, uint8_t *end);
uint32_t pullarray16(uint8_t **in, uint16_t out[], uint32_t max_len, uint32_t len, uint8_t *end);
uint32_t pullarrays16(uint8_t **in, int16_t out[], uint32_t max_len, uint32_t len, uint8_t *end);

uint32_t pusharray8(uint8_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end);
uint32_t pusharray16(uint16_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end);
uint32_t pusharrays16(int16_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end);

typedef uint8_t (*pack_array_elem_fn)(void* elem, uint8_t **ppWritePackedMsg, uint8_t *end);
uint8_t packarray(void* array, uint16_t elem_size, uint16_t max_count, uint16_t count, uint8_t **ppWritePackedMsg, uint8_t *end, pack_array_elem_fn fn);

typedef uint8_t (*unpack_array_elem_fn)(void* elem, uint8_t **ppReadPackedMsg, uint8_t *end);
uint8_t unpackarray(uint8_t **ppReadPackedMsg, void* array, uint16_t elem_size, uint16_t max_count, uint16_t count, uint8_t *end, unpack_array_elem_fn fn);

uint8_t pack_tl(nfapi_tl_t *tl, uint8_t **ppWritePackedMsg, uint8_t *end);
uint8_t unpack_tl(uint8_t **ppReadPackedMsg, nfapi_tl_t *tl, uint8_t *end);

typedef uint8_t (*pack_tlv_fn)(void* tlv, uint8_t **ppWritePackedMsg, uint8_t *end);
uint8_t pack_tlv(uint16_t tag, void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end, pack_tlv_fn fn);

uint32_t pack_vendor_extension_tlv(nfapi_tl_t* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* config);
int unpack_vendor_extension(nfapi_tl_t* tl, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* config, nfapi_tl_t** ve_tlv);

typedef uint8_t (*unpack_tlv_fn)(void* tlv, uint8_t **ppWritePackedMsg, uint8_t *end);
typedef struct
{
	uint16_t tag;
	void* tlv;
	unpack_tlv_fn unpack_func;
} unpack_tlv_t;

int unpack_tlv_list(unpack_tlv_t unpack_fns[], uint16_t size, uint8_t **ppReadPackedMsg, uint8_t* packedMsgEnd, nfapi_p4_p5_codec_config_t* config, nfapi_tl_t** ve);

uint32_t pack_p7_vendor_extension_tlv(nfapi_tl_t *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t* config);
typedef uint8_t (*unpack_p7_tlv_fn)(void* tlv, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t*);
typedef struct
{
	uint16_t tag;
	void* tlv;
	unpack_p7_tlv_fn unpack_func;
} unpack_p7_tlv_t;
int unpack_p7_tlv_list(unpack_p7_tlv_t unpack_fns[], uint16_t size, uint8_t **ppReadPackedMsg, uint8_t* packedMsgEnd, nfapi_p7_codec_config_t* config, nfapi_tl_t** ve);

#if defined(__cplusplus)
}
#endif

#endif /* _NFAPI_H_ */
