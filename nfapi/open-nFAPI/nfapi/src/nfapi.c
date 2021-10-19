/*
 * Copyright (c) 2001-2016, Cisco Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * Neither the name of the Cisco Systems, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <nfapi_interface.h>
#include <nfapi.h>
#include <debug.h>


// Fundamental routines

uint8_t push8(uint8_t in, uint8_t **out, uint8_t *end) {
  uint8_t *pOut = *out;

  if((end - pOut) >= 1) {
    pOut[0] = in;
    (*out)+=1;
    return 1;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pushs8(int8_t in, uint8_t **out, uint8_t *end) {
  uint8_t *pOut = *out;

  if((end - pOut) >= 1) {
    pOut[0] = in;
    (*out)+=1;
    return 1;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t push16(uint16_t in, uint8_t **out, uint8_t *end) {
  uint8_t *pOut = *out;

  if((end - pOut) >= 2) {
    pOut[0] = (in & 0xFF00) >> 8;
    pOut[1] = (in & 0xFF);
    (*out)+=2;
    return 2;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pushs16(int16_t in, uint8_t **out, uint8_t *end) {
  uint8_t *pOut = *out;

  if((end - pOut) >= 2) {
    pOut[0] = (in & 0xFF00) >> 8;
    pOut[1] = (in & 0xFF);
    (*out)+=2;
    return 2;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t push32(uint32_t in, uint8_t **out, uint8_t *end) {
  uint8_t *pOut = *out;

  if((end - pOut) >= 4) {
    pOut[0] = (in & 0xFF000000) >> 24;
    pOut[1] = (in & 0xFF0000) >> 16;
    pOut[2] = (in & 0xFF00) >> 8;
    pOut[3] = (in & 0xFF);
    (*out)+=4;
    return 4;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pushs32(int32_t in, uint8_t **out, uint8_t *end) {
  uint8_t *pOut = *out;

  if((end - pOut) >= 4) {
    pOut[0] = (in & 0xFF000000) >> 24;
    pOut[1] = (in & 0xFF0000) >> 16;
    pOut[2] = (in & 0xFF00) >> 8;
    pOut[3] = (in & 0xFF);
    (*out)+=4;
    return 4;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pull8(uint8_t **in, uint8_t *out, uint8_t *end) {
  uint8_t *pIn = *in;

  if((end - pIn) >= 1 ) {
    *out = *pIn;
    (*in)+=1;
    return 1;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pulls8(uint8_t **in, int8_t *out, uint8_t *end) {
  uint8_t *pIn = *in;

  if((end - pIn) >= 1 ) {
    *out = *pIn;
    (*in)+=1;
    return 1;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pull16(uint8_t **in, uint16_t *out, uint8_t *end) {
  uint8_t *pIn = *in;

  if((end - pIn) >=2 ) {
    *out = ((pIn[0]) << 8) | pIn[1];
    (*in)+=2;
    return 2;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pulls16(uint8_t **in, int16_t *out, uint8_t *end) {
  uint8_t *pIn = *in;

  if((end - pIn) >=2 ) {
    *out = ((pIn[0]) << 8) | pIn[1];
    (*in)+=2;
    return 2;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t pull32(uint8_t **in, uint32_t *out, uint8_t *end) {
  uint8_t *pIn = *in;

  if((end - pIn) >=4 ) {
    *out = (pIn[0] << 24) | (pIn[1] << 16) | (pIn[2] << 8) | pIn[3];
    (*in)+=4;
    return 4;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n",  __FUNCTION__);
    return 0;
  }
}

uint8_t pulls32(uint8_t **in, int32_t *out, uint8_t *end) {
  uint8_t *pIn = *in;

  if((end - pIn) >=4 ) {
    *out = (pIn[0] << 24) | (pIn[1] << 16) | (pIn[2] << 8) | pIn[3];
    (*in)+=4;
    return 4;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

/*
inline void pusharray16(uint8_t **, uint16_t, uint32_t len)
{
}
*/

uint32_t pullarray16(uint8_t **in, uint16_t out[], uint32_t max_len, uint32_t len, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*in)) >= sizeof(uint16_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      if(!pull16(in, &out[idx], end))
        return 0;
    }

    return sizeof(uint16_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint32_t pullarrays16(uint8_t **in, int16_t out[], uint32_t max_len, uint32_t len, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*in)) >= sizeof(uint16_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      if(!pulls16(in, &out[idx], end))
        return 0;
    }

    return sizeof(uint16_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}
uint32_t pusharray16(uint16_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*out)) >= sizeof(uint16_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      if(!push16(in[idx], out, end))
        return 0;
    }

    return sizeof(uint16_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}
uint32_t pusharrays16(int16_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*out)) >= sizeof(uint16_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      pushs16(in[idx], out, end);
    }

    return sizeof(uint16_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}
uint32_t pullarray32(uint8_t **in, uint32_t out[], uint32_t max_len, uint32_t len, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*in)) >= sizeof(uint32_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      if(!pull32(in, &out[idx], end))
        return 0;
    }

    return sizeof(uint32_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint32_t pullarrays32(uint8_t **in, int32_t out[], uint32_t max_len, uint32_t len, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*in)) >= sizeof(uint32_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      if(!pulls32(in, &out[idx], end))
        return 0;
    }

    return sizeof(uint32_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}
uint32_t pusharray32(uint32_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*out)) >= sizeof(uint32_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      if(!push32(in[idx], out, end))
        return 0;
    }

    return sizeof(uint32_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}
uint32_t pusharrays32(int32_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*out)) >= sizeof(uint32_t) * len) {
    uint32_t idx;

    for(idx = 0; idx < len; ++idx) {
      pushs32(in[idx], out, end);
    }

    return sizeof(uint32_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}
uint32_t pullarray8(uint8_t **in, uint8_t out[], uint32_t max_len, uint32_t len, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*in)) >= sizeof(uint8_t) * len) {
    memcpy(out, (*in), len);
    (*in)+=len;
    return sizeof(uint8_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint32_t pusharray8(uint8_t in[], uint32_t max_len, uint32_t len, uint8_t **out, uint8_t *end) {
  if(len == 0)
    return 1;

  if(len > max_len) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, len, max_len);
    return 0;
  }

  if((end - (*out)) >= sizeof(uint8_t) * len) {
    memcpy((*out), in, len);
    (*out)+=len;
    return sizeof(uint8_t) * len;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s no space in buffer\n", __FUNCTION__);
    return 0;
  }
}

uint8_t packarray(void *array, uint16_t array_element_size, uint16_t max_count, uint16_t count, uint8_t **ppwritepackedmsg, uint8_t *end, pack_array_elem_fn fn) {
  if(count > max_count) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, count, max_count);
    return 0;
  }

  uint16_t i = 0;

  for(i = 0; i < count; ++i) {
    if((fn)(array, ppwritepackedmsg, end) == 0)
      return 0;

    array += array_element_size;
  }

  return 1;
}

uint8_t unpackarray(uint8_t **ppReadPackedMsg, void *array, uint16_t array_element_size, uint16_t max_count, uint16_t count, uint8_t *end, unpack_array_elem_fn fn) {
  if(count > max_count) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s exceed array size (%d > %d)\n", __FUNCTION__, count, max_count);
    return 0;
  }

  uint16_t i = 0;

  for(i = 0; i < count; ++i) {
    if((fn)(array, ppReadPackedMsg, end) == 0)
      return 0;

    array += array_element_size;
  }

  return 1;
}


uint32_t pack_vendor_extension_tlv(nfapi_tl_t *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  if(ve != 0 && config != 0) {
    if(config->pack_vendor_extension_tlv) {
      uint8_t *pStartOfTlv = *ppWritePackedMsg;

      if(pack_tl(ve, ppWritePackedMsg, end) == 0)
        return 0;

      uint8_t *pStartOfValue = *ppWritePackedMsg;

      if((config->pack_vendor_extension_tlv)(ve, ppWritePackedMsg, end, config) == 0)
        return 0;

      ve->length = (*ppWritePackedMsg) - pStartOfValue;
      pack_tl(ve, &pStartOfTlv, end);
      return 1;
    }
  }

  return 1;
}

uint32_t unpack_vendor_extension_tlv(nfapi_tl_t *tl, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config, nfapi_tl_t **ve_tlv) {
  if(ve_tlv != 0 && config != 0) {
    if(config->unpack_vendor_extension_tlv) {
      return (config->unpack_vendor_extension_tlv)(tl, ppReadPackedMsg, end, (void **)ve_tlv, config);
    }
  }

  return 1;
}

uint32_t pack_p7_vendor_extension_tlv(nfapi_tl_t *ve, uint8_t **ppWritePackedMsg, uint8_t *end,nfapi_p7_codec_config_t *config) {
  if(ve != 0 && config != 0) {
    if(config->pack_vendor_extension_tlv) {
      uint8_t *pStartOfTlv = *ppWritePackedMsg;

      if(pack_tl(ve, ppWritePackedMsg, end) == 0)
        return 0;

      uint8_t *pStartOfValue = *ppWritePackedMsg;

      if((config->pack_vendor_extension_tlv)(ve, ppWritePackedMsg, end, config) == 0)
        return 0;

      ve->length = (*ppWritePackedMsg) - pStartOfValue;
      pack_tl(ve, &pStartOfTlv, end);
      return 1;
    }
  }

  return 1;
}

int unpack_p7_vendor_extension_tlv(nfapi_tl_t *tl, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config, nfapi_tl_t **ve_tlv) {
  if(ve_tlv != 0 && config != 0) {
    if(config->unpack_vendor_extension_tlv) {
      return (config->unpack_vendor_extension_tlv)(tl, ppReadPackedMsg, end, (void **)ve_tlv, config);
    }
  }

  return 1;
}


uint8_t pack_tl(nfapi_tl_t *tl, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return (push16(tl->tag, ppWritePackedMsg, end) &&
          push16(tl->length, ppWritePackedMsg, end));
}

uint8_t unpack_tl(uint8_t **ppReadPackedMsg, nfapi_tl_t *tl, uint8_t *end) {
  return (pull16(ppReadPackedMsg, &tl->tag, end) &&
          pull16(ppReadPackedMsg, &tl->length, end));
}

int unpack_tlv_list(unpack_tlv_t unpack_fns[], uint16_t size, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config, nfapi_tl_t **ve) {
  nfapi_tl_t generic_tl;
  uint8_t numBadTags = 0;
  uint16_t idx = 0;

  while ((uint8_t *)(*ppReadPackedMsg) < end) {
    // unpack the tl and process the values accordingly
    if(unpack_tl(ppReadPackedMsg, &generic_tl, end) == 0)
      return 0;

    uint8_t tagMatch = 0;
    uint8_t *pStartOfValue = *ppReadPackedMsg;

    for(idx = 0; idx < size; ++idx) {
      if(unpack_fns[idx].tag == generic_tl.tag) { // match the extracted tag value with all the tags in unpack_fn list
        tagMatch = 1;
        nfapi_tl_t *tl = (nfapi_tl_t *)(unpack_fns[idx].tlv);
        tl->tag = generic_tl.tag;
        tl->length = generic_tl.length;
        int result = (*unpack_fns[idx].unpack_func)(tl, ppReadPackedMsg, end);

        if(result == 0) {
          return 0;
        }

        // check if the length was right;
        if(tl->length != (*ppReadPackedMsg - pStartOfValue)) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Warning tlv tag 0x%x length %d not equal to unpack %ld\n", tl->tag, tl->length, (*ppReadPackedMsg - pStartOfValue));
        }
      }
    }

    if(tagMatch == 0) {
      if(generic_tl.tag >= NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE &&
          generic_tl.tag <= NFAPI_VENDOR_EXTENSION_MAX_TAG_VALUE) {
        int result = unpack_vendor_extension_tlv(&generic_tl, ppReadPackedMsg, end, config, ve);

        if(result == 0) {
          // got tot the end.
          return 0;
        } else if(result < 0) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown VE TAG value: 0x%04x\n", generic_tl.tag);

          if (++numBadTags > MAX_BAD_TAG) {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Supplied message has had too many bad tags\n");
            return 0;
          }

          if((end - *ppReadPackedMsg) >= generic_tl.length) {
            // Advance past the unknown TLV
            (*ppReadPackedMsg) += generic_tl.length;
          } else {
            // go to the end
            return 0;
          }
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown TAG value: 0x%04x\n", generic_tl.tag);

        if (++numBadTags > MAX_BAD_TAG) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Supplied message has had too many bad tags\n");
          return 0;
        }

        if((end - *ppReadPackedMsg) >= generic_tl.length) {
          // Advance past the unknown TLV
          (*ppReadPackedMsg) += generic_tl.length;
        } else {
          // go to the end
          return 0;
        }
      }
    }
  }

  return 1;
}
int unpack_p7_tlv_list(unpack_p7_tlv_t unpack_fns[], uint16_t size, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config, nfapi_tl_t **ve) {
  nfapi_tl_t generic_tl;
  uint8_t numBadTags = 0;
  uint16_t idx = 0;

  while ((uint8_t *)(*ppReadPackedMsg) < end) {
    // unpack the tl and process the values accordingly
    if(unpack_tl(ppReadPackedMsg, &generic_tl, end) == 0)
      return 0;

    uint8_t tagMatch = 0;
    uint8_t *pStartOfValue = *ppReadPackedMsg;

    for(idx = 0; idx < size; ++idx) {
      if(unpack_fns[idx].tag == generic_tl.tag) {
        tagMatch = 1;
        nfapi_tl_t *tl = (nfapi_tl_t *)(unpack_fns[idx].tlv);
        tl->tag = generic_tl.tag;
        tl->length = generic_tl.length;
        int result = (*unpack_fns[idx].unpack_func)(tl, ppReadPackedMsg, end, config);

        if(result == 0) {
          return  0;
        }

        // check if the length was right;
        if(tl->length != (*ppReadPackedMsg - pStartOfValue)) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Warning tlv tag 0x%x length %d not equal to unpack %ld\n", tl->tag, tl->length, (*ppReadPackedMsg - pStartOfValue));
        }
      }
    }

    if(tagMatch == 0) {
      if(generic_tl.tag >= NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE &&
          generic_tl.tag <= NFAPI_VENDOR_EXTENSION_MAX_TAG_VALUE) {
        int result = unpack_p7_vendor_extension_tlv(&generic_tl, ppReadPackedMsg, end, config, ve);

        if(result == 0) {
          // got to end
          return 0;
        } else if(result < 0) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown TAG value: 0x%04x\n", generic_tl.tag);

          if (++numBadTags > MAX_BAD_TAG) {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Supplied message has had too many bad tags\n");
            return -1;
          }

          if((end - *ppReadPackedMsg) >= generic_tl.length) {
            // Advance past the unknown TLV
            (*ppReadPackedMsg) += generic_tl.length;
          } else {
            // got ot the dn
            return 0;
          }
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown TAG value: 0x%04x\n", generic_tl.tag);

        if (++numBadTags > MAX_BAD_TAG) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Supplied message has had too many bad tags\n");
          return -1;
        }

        if((end - *ppReadPackedMsg) >= generic_tl.length) {
          // Advance past the unknown TLV
          (*ppReadPackedMsg) += generic_tl.length;
        } else {
          // got ot the dn
          return 0;
        }
      }
    }
  }

  return 1;
}

// This intermediate function deals with calculating the length of the value
// and writing into the tlv header.
uint8_t pack_tlv(uint16_t tag, void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end, pack_tlv_fn fn) {
  nfapi_tl_t *tl = (nfapi_tl_t *)tlv;

  // If the tag is defined
  if(tl->tag == tag) {
    uint8_t *pStartOfTlv = *ppWritePackedMsg;

    // write a dumy tlv header
    if(pack_tl(tl, ppWritePackedMsg, end) == 0)
      return 0;

    // Record the start of the value
    uint8_t *pStartOfValue = *ppWritePackedMsg;

    // pack the tlv value
    if(fn(tlv, ppWritePackedMsg, end) == 0)
      return 0;

    // calculate the length of the value and rewrite the tl header
    tl->length = (*ppWritePackedMsg) - pStartOfValue;
    // rewrite the header with the correct length
    pack_tl(tl, &pStartOfTlv, end);
  } else {
    if(tl->tag != 0) {
      NFAPI_TRACE(NFAPI_TRACE_WARN, "Warning pack_tlv tag 0x%x does not match expected 0x%x\n", tl->tag, tag);
    } else {
      //NFAPI_TRACE(NFAPI_TRACE_ERROR, "Warning pack_tlv tag 0x%x ZERO does not match expected 0x%x\n", tl->tag, tag);
    }
  }

  return 1;
}

const char *nfapi_error_code_to_str(nfapi_error_code_e value) {
  switch(value) {
    case NFAPI_MSG_OK:
      return "NFAPI_MSG_OK";

    case NFAPI_MSG_INVALID_STATE:
      return "NFAPI_MSG_INVALID_STATE";

    case NFAPI_MSG_INVALID_CONFIG:
      return "NFAPI_MSG_INVALID_CONFIG";

    case NFAPI_SFN_OUT_OF_SYNC:
      return "NFAPI_SFN_OUT_OF_SYNC";

    case NFAPI_MSG_SUBFRAME_ERR:
      return "NFAPI_MSG_SUBFRAME_ERR";

    case NFAPI_MSG_BCH_MISSING:
      return "NFAPI_MSG_BCH_MISSING";

    case NFAPI_MSG_INVALID_SFN:
      return "NFAPI_MSG_INVALID_SFN";

    case NFAPI_MSG_HI_ERR:
      return "NFAPI_MSG_HI_ERR";

    case NFAPI_MSG_TX_ERR:
      return "NFAPI_MSG_TX_ERR";

    default:
      return "UNKNOWN";
  }
}
