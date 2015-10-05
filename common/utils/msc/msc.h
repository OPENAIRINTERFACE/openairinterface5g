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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

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
    MSC_RRC_ENB,
    MSC_IP_ENB,
    MSC_S1AP_ENB,
    MSC_GTPU_ENB,
    MSC_GTPU_SGW,
    MSC_S1AP_MME,
    MSC_MMEAPP_MME,
    MSC_NAS_MME,
    MSC_NAS_EMM_MME,
    MSC_NAS_ESM_MME,
    MSC_SP_GWAPP_MME,
    MSC_S11_MME,
    MSC_S6A_MME,
    MSC_HSS,
    MAX_MSC_PROTOS,
} msc_proto_t;



// Access stratum
#define MSC_AS_TIME_FMT "%05u:%02u"

#define MSC_AS_TIME_ARGS(CTXT_Pp) \
    (CTXT_Pp)->frame, \
    (CTXT_Pp)->subframe
#if defined(MESSAGE_CHART_GENERATOR)
int msc_init(const msc_env_t envP, const int max_threadsP);
void msc_start_use(void);
void msc_flush_messages(void);
void msc_end(void);
void msc_log_declare_proto(const msc_proto_t  protoP);
void msc_log_event(const msc_proto_t  protoP,char *format, ...);
void msc_log_message(
	const char * const message_operationP,
    const msc_proto_t  receiverP,
    const msc_proto_t  senderP,
    const uint8_t* const bytesP,
    const unsigned int num_bytes,
    char *format, ...);

#define MSC_INIT(arg1,arg2)                                      msc_init(arg1,arg2)
#define MSC_START_USE                                            msc_start_use
#define MSC_END                                                  msc_end
#define MSC_LOG_EVENT(mScPaRaMs, fORMAT, aRGS...)                msc_log_event(mScPaRaMs, fORMAT, ##aRGS)
#define MSC_LOG_RX_MESSAGE(rECEIVER, sENDER, bYTES, nUMbYTES, fORMAT, aRGS...)           msc_log_message("<-",rECEIVER, sENDER, bYTES, nUMbYTES, fORMAT, ##aRGS)
#define MSC_LOG_RX_DISCARDED_MESSAGE(rECEIVER, sENDER, bYTES, nUMbYTES, fORMAT, aRGS...) msc_log_message("x-",rECEIVER, sENDER, bYTES, nUMbYTES, fORMAT, ##aRGS)
#define MSC_LOG_TX_MESSAGE(sENDER, rECEIVER, bYTES, nUMbYTES, fORMAT, aRGS...)           msc_log_message("->",sENDER, rECEIVER, bYTES, nUMbYTES, fORMAT, ##aRGS)
#define MSC_LOG_TX_MESSAGE_FAILED(sENDER, rECEIVER, bYTES, nUMbYTES, fORMAT, aRGS...)    msc_log_message("-x",sENDER, rECEIVER, bYTES, nUMbYTES, fORMAT, ##aRGS)
#else
#define MSC_INIT(arg1,arg2)
#define MSC_START_USE(mScPaRaMs)
#define MSC_END(mScPaRaMs)
#define MSC_LOG_EVENT(mScPaRaMs, fORMAT, aRGS...)
#define MSC_LOG_RX_MESSAGE(mScPaRaMs, fORMAT, aRGS...)
#define MSC_LOG_RX_DISCARDED_MESSAGE(mScPaRaMs, fORMAT, aRGS...)
#define MSC_LOG_TX_MESSAGE(mScPaRaMs, fORMAT, aRGS...)
#define MSC_LOG_TX_MESSAGE_FAILED(mScPaRaMs, fORMAT, aRGS...)
#endif
#endif
