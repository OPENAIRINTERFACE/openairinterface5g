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

#ifndef MSC_H_
#define MSC_H_
#include <stdarg.h>
#include <stdint.h>

typedef enum {
  MIN_MSC_ENV = 0,
  MSC_E_UTRAN = MIN_MSC_ENV,
  MSC_E_UTRAN_LIPA,
  MSC_MME_GW,
  MSC_MME,
  MSC_SP_GW,
  MAX_MSC_ENV
} msc_env_t;


typedef enum {
  MIN_MSC_PROTOS = 0,
  MSC_IP_UE = MIN_MSC_PROTOS,
  MSC_NAS_UE,
  MSC_RRC_UE,
  MSC_PDCP_UE,
  MSC_RLC_UE,
  MSC_MAC_UE,
  MSC_PHY_UE,
  MSC_PHY_ENB,
  MSC_MAC_ENB,
  MSC_RLC_ENB,
  MSC_PDCP_ENB,
  MSC_PDCP_GNB,
  MSC_RRC_ENB,
  MSC_RRC_GNB,
  MSC_IP_ENB,
  MSC_S1AP_ENB,
  MSC_NGAP_GNB,
  MSC_GTPU_ENB,
  MSC_GTPU_GNB,
  MSC_GTPU_SGW,
  MSC_S1AP_MME,
  MSC_NGAP_AMF,
  MSC_MMEAPP_MME,
  MSC_NAS_MME,
  MSC_NAS_EMM_MME,
  MSC_NAS_ESM_MME,
  MSC_SP_GWAPP_MME,
  MSC_S11_MME,
  MSC_S6A_MME,
  MSC_HSS,
  MSC_F1AP_CU,
  MSC_F1AP_DU,
  MSC_X2AP_SRC_ENB,
  MSC_X2AP_TARGET_ENB,
  MAX_MSC_PROTOS,
} msc_proto_t;



// Access stratum
#define MSC_AS_TIME_FMT "%05u:%02u"

#define MSC_AS_TIME_ARGS(CTXT_Pp) \
  (CTXT_Pp)->frame, \
  (CTXT_Pp)->subframe

typedef int(*msc_init_t)(const msc_env_t, const int );
typedef void(*msc_start_use_t)(void );
typedef void(*msc_end_t)(void);
typedef void(*msc_log_event_t)(const msc_proto_t,char *, ...);
typedef void(*msc_log_message_t)(const char    *const, const msc_proto_t, const msc_proto_t,
                                 const uint8_t *const, const unsigned int, char *, ...);
typedef struct msc_interface {
  int               msc_loaded;
  msc_init_t        msc_init;
  msc_start_use_t   msc_start_use;
  msc_end_t         msc_end;
  msc_log_event_t   msc_log_event;
  msc_log_message_t msc_log_message;
} msc_interface_t;

#ifdef MSC_LIBRARY
int msc_init(const msc_env_t envP, const int max_threadsP);
void msc_start_use(void);
void msc_flush_messages(void);
void msc_end(void);
void msc_log_declare_proto(const msc_proto_t  protoP);
void msc_log_event(const msc_proto_t  protoP,char *format, ...);
void msc_log_message(
  const char *const message_operationP,
  const msc_proto_t  receiverP,
  const msc_proto_t  senderP,
  const uint8_t *const bytesP,
  const unsigned int num_bytes,
  char *format, ...);

#else

#define MESSAGE_CHART_GENERATOR  msc_interface.msc_loaded

extern msc_interface_t msc_interface;
#define MSC_INIT(arg1,arg2)                                     if(msc_interface.msc_loaded) msc_interface.msc_init(arg1,arg2)
#define MSC_START_USE                                           if(msc_interface.msc_loaded) msc_interface.msc_start_use
#define MSC_END                                                 if(msc_interface.msc_loaded) msc_interface.msc_end
#define MSC_LOG_EVENT(mScPaRaMs, fORMAT, aRGS...)               if(msc_interface.msc_loaded) msc_interface.msc_log_event(mScPaRaMs, fORMAT, ##aRGS)
#define MSC_LOG_RX_MESSAGE(rECEIVER, sENDER, bYTES, nUMbYTES, fORMAT, aRGS...)           if(msc_interface.msc_loaded) msc_interface.msc_log_message("<-",rECEIVER, sENDER, (const uint8_t *)bYTES, nUMbYTES, fORMAT, ##aRGS)
#define MSC_LOG_RX_DISCARDED_MESSAGE(rECEIVER, sENDER, bYTES, nUMbYTES, fORMAT, aRGS...) if(msc_interface.msc_loaded) msc_interface.msc_log_message("x-",rECEIVER, sENDER, (const uint8_t *)bYTES, nUMbYTES, fORMAT, ##aRGS)
#define MSC_LOG_TX_MESSAGE(sENDER, rECEIVER, bYTES, nUMbYTES, fORMAT, aRGS...)           if(msc_interface.msc_loaded) msc_interface.msc_log_message("->",sENDER, rECEIVER, (const uint8_t *)bYTES, nUMbYTES, fORMAT, ##aRGS)
#define MSC_LOG_TX_MESSAGE_FAILED(sENDER, rECEIVER, bYTES, nUMbYTES, fORMAT, aRGS...)    if(msc_interface.msc_loaded) msc_interface.msc_log_message("-x",sENDER, rECEIVER, (const uint8_t *)bYTES, nUMbYTES, fORMAT, ##aRGS)
#endif

#endif
