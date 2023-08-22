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

Source    at_response.c

Version   0.1

Date    2012/03/13

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines structure of the AT command response returned after
    completion of AT command processing.

*****************************************************************************/

#include "at_response.h"
#include "nas_log.h"

#include <stdio.h>  // sprintf, snprintf
#include <string.h> // strncpy
#include <assert.h> // assert

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/*
 * Response format indicator (set by ATV0 and ATV1)
 * -----------------------------------------------
 *  true  - <CR><LF><text><CR><LF>, <CR><LF><verbose code><CR><LF>
 *  false - <text><CR><LF>, <numeric code><CR><LF>
 */
bool at_response_format_v1 = true;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static int _at_response_encode_cgsn   (char* buffer, const at_response_t* data);
static int _at_response_encode_cgmi   (char* buffer, const at_response_t* data);
static int _at_response_encode_cgmm   (char* buffer, const at_response_t* data);
static int _at_response_encode_cgmr   (char* buffer, const at_response_t* data);
static int _at_response_encode_cimi   (char* buffer, const at_response_t* data);
static int _at_response_encode_cfun   (char* buffer, const at_response_t* data);
static int _at_response_encode_cpin   (char* buffer, const at_response_t* data);
static int _at_response_encode_csq    (char* buffer, const at_response_t* data);
static int _at_response_encode_cesq   (char* buffer, const at_response_t* data);
static int _at_response_encode_clac   (char* buffer, const at_response_t* data);
static int _at_response_encode_cmee   (char* buffer, const at_response_t* data);
static int _at_response_encode_cnum   (char* buffer, const at_response_t* data);
static int _at_response_encode_clck   (char* buffer, const at_response_t* data);
static int _at_response_encode_cops   (char* buffer, const at_response_t* data);
static int _at_response_encode_cgatt  (char* buffer, const at_response_t* data);
static int _at_response_encode_creg   (char* buffer, const at_response_t* data);
static int _at_response_encode_cgreg  (char* buffer, const at_response_t* data);
static int _at_response_encode_cereg  (char* buffer, const at_response_t* data);
static int _at_response_encode_cgdcont(char* buffer, const at_response_t* data);
static int _at_response_encode_cgact  (char* buffer, const at_response_t* data);
static int _at_response_encode_cgpaddr(char* buffer, const at_response_t* data);
static int _at_response_encode_cgev   (char* buffer, const at_response_t* data);

/* Encoding functions for AT command response messages */
typedef int (*_at_response_encode_function_t) (char* buffer, const at_response_t*);

static _at_response_encode_function_t _at_response_encode_function[AT_RESPONSE_ID_MAX] = {
    NULL,
    _at_response_encode_cgsn, /* CGSN    */
    _at_response_encode_cgmi, /* CGMI    */
    _at_response_encode_cgmm, /* CGMM    */
    _at_response_encode_cgmr, /* CGMR    */
    _at_response_encode_cimi, /* CIMI    */
    _at_response_encode_cfun, /* CFUN    */
    _at_response_encode_cpin, /* CPIN    */
    _at_response_encode_csq, /* CSQ     */
    _at_response_encode_cesq, /* CESQ    */
    _at_response_encode_clac, /* CLAC    */
    _at_response_encode_cmee, /* CMEE    */
    _at_response_encode_cnum, /* CNUM    */
    _at_response_encode_clck, /* CLCK    */
    _at_response_encode_cops, /* COPS    */
    _at_response_encode_creg, /* CREG    */
    _at_response_encode_cgatt, /* CGATT   */
    _at_response_encode_cgreg, /* CGREG   */
    _at_response_encode_cereg, /* CEREG   */
    _at_response_encode_cgdcont, /* CGDCONT */
    _at_response_encode_cgact, /* CGACT   */
    _at_response_encode_cgpaddr, /* CGPADDR */
    _at_response_encode_cgev, /* CGEV: unsolicited result */
};

/* String representation of Packet Domain events (cf. network_pdn_state_t) */
static const char* _at_response_event_str[] = {
    "UNKNOWN EVENT",
    "ME PDN ACT",
    "NW PDN DEACT",
    "ME PDN DEACT",
    "NW ACT",
    "ME ACT",
    "NW DEACT",
    "ME DEACT",
};

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  at_response_encode()                                      **
 **                                                                        **
 ** Description: Encodes AT command response message                       **
 **                                                                        **
 ** Inputs   at_response: AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters in the buffer     **
 **       when data have been successfully encoded;  **
 **       RETURNerror otherwise.                     **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int at_response_encode(char* buffer, const at_response_t* at_response)
{
    LOG_FUNC_IN;

    int bytes = RETURNerror;
    _at_response_encode_function_t encode;

    if (at_response->id < AT_RESPONSE_ID_MAX) {
        /* Call encoding function applicable to the AT command response */
        encode = _at_response_encode_function[at_response->id];

        if (encode != NULL) {
            bytes = (*encode)(buffer, at_response);
        } else {
            /* Generic encoding: OK, ERROR */
            bytes = 0;
        }
    }

    LOG_FUNC_RETURN (bytes);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgsn()                                **
 **                                                                        **
 ** Description: Encodes AT CGSN command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgsn(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        const at_cgsn_resp_t * cgsn = &(data->response.cgsn);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGSN: %s\r\n", cgsn->sn);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgmi()                                **
 **                                                                        **
 ** Description: Encodes AT CGMI command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgmi(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        const at_cgmi_resp_t * cgmi = &(data->response.cgmi);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGMI: %s\r\n", cgmi->manufacturer);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgmm()                                **
 **                                                                        **
 ** Description: Encodes AT CGMM command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgmm(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        const at_cgmm_resp_t * cgmm = &(data->response.cgmm);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGMM: %s\r\n", cgmm->model);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgmr()                                **
 **                                                                        **
 ** Description: Encodes AT CGMR command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgmr(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        const at_cgmr_resp_t * cgmr = &(data->response.cgmr);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGMR: %s\r\n", cgmr->revision);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cimi()                                **
 **                                                                        **
 ** Description: Encodes AT CIMI command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cimi(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        const at_cimi_resp_t * cimi = &(data->response.cimi);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CIMI: %s\r\n", cimi->IMSI);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cfun()                                **
 **                                                                        **
 ** Description: Encodes AT CFUN command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cfun(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    const at_cfun_resp_t * cfun = &(data->response.cfun);
    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CFUN: %d\r\n", cfun->fun);
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CFUN: (%d-%d),(%d,%d)\r\n",
                          AT_CFUN_MIN, AT_CFUN_MAX, AT_CFUN_NORST, AT_CFUN_RST);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cpin()                                **
 **                                                                        **
 ** Description: Encodes AT CPIN command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cpin(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cpin_resp_t * cpin = &(data->response.cpin);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CPIN: %s\r\n", cpin->code);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_csq()                                 **
 **                                                                        **
 ** Description: Encodes AT CSQ command response message                   **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_csq(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    const at_csq_resp_t * csq = &(data->response.csq);
    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CSQ: %d,%d\r\n",
                          csq->rssi, csq->ber);
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CSQ: (%d-%d),(%d-%d)\r\n",
                          AT_CSQ_RSSI_0, AT_CSQ_RSSI_31,
                          AT_CSQ_BER_0, AT_CSQ_BER_7);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cesq()                                **
 **                                                                        **
 ** Description: Encodes AT CESQ command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cesq(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    const at_cesq_resp_t * cesq = &(data->response.cesq);
    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CESQ: %d,%d,%d,%d,%d,%d\r\n",
                          cesq->rssi, cesq->ber, cesq->rscp,
                          cesq->ecno, cesq->rsrq, cesq->rsrp);
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CESQ: (%d-%d),(%d-%d),(%d-%d),"
                          "(%d-%d),(%d-%d),(%d-%d)\r\n",
                          AT_CESQ_RSSI_0, AT_CESQ_RSSI_31,
                          AT_CESQ_BER_0, AT_CESQ_BER_7,
                          AT_CESQ_RSCP_0, AT_CESQ_RSCP_96,
                          AT_CESQ_ECNO_0, AT_CESQ_ECNO_49,
                          AT_CESQ_RSRQ_0, AT_CESQ_RSRQ_34,
                          AT_CESQ_RSRP_0, AT_CESQ_RSRP_97);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_clac()                                **
 **                                                                        **
 ** Description: Encodes AT CLAC command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_clac(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    const at_clac_resp_t * clac = &(data->response.clac);
    int offset = 0;
    int i;

    if (data->type == AT_COMMAND_ACT) {
        if (clac->n_acs > 0) {
            if (at_response_format_v1) {
                offset += sprintf(buffer, "\r\n");
            }

            offset += sprintf(buffer+offset, "%s", clac->ac[0]);

            for (i = 1; i < clac->n_acs; i++) {
                offset += sprintf(buffer+offset, "\r\n%s", clac->ac[i]);
            }
        }
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cnum()                                **
 **                                                                        **
 ** Description: Encodes AT CNUM command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cnum(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_ACT) {
        const at_cnum_resp_t * cnum = &(data->response.cnum);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CNUM: ,%s,%u\r\n",
                          cnum->number, (unsigned int)cnum->type);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_clck()                                **
 **                                                                        **
 ** Description: Encodes AT CLCK command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_clck(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    const at_clck_resp_t * clck = &(data->response.clck);
    int offset = 0;

    if (data->type == AT_COMMAND_SET) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CLCK: %d\r\n", clck->status);
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CLCK: %s\r\n", AT_CLCK_SC);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cops()                                **
 **                                                                        **
 ** Description: Encodes AT COPS command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cops(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cops_get_t * cops = &(data->response.cops.get);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+COPS: %d", cops->mode);

        if (data->mask & AT_COPS_RESP_OPER_MASK) {
            /* If <oper> is present <format> must be given */
            assert(data->mask & AT_COPS_RESP_FORMAT_MASK);
            offset += sprintf(buffer+offset, ",%d", cops->format);

            if (cops->format == AT_COPS_LONG) {
                offset += sprintf(buffer+offset, ",%s",
                                  (char*)cops->plmn.id.alpha_long);
            } else if (cops->format == AT_COPS_SHORT) {
                offset += sprintf(buffer+offset, ",%s",
                                  (char*)cops->plmn.id.alpha_short);
            } else if (cops->format == AT_COPS_NUM) {
                offset += sprintf(buffer+offset, ",%s",
                                  (char*)cops->plmn.id.num);
            }
        }

        if (data->mask & AT_COPS_RESP_ACT_MASK) {
            offset += sprintf(buffer+offset, ",%d", cops->AcT);
        }

        offset += sprintf(buffer+offset, "\r\n");
    } else if (data->type == AT_COMMAND_TST) {
        const at_cops_tst_t * cops = &(data->response.cops.tst);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+COPS: ");

        /* Display the list of operators present in the network */
        strncpy(buffer+offset, cops->data, cops->size);
        offset += cops->size;

        /* Display the list of supported network registration modes and
         * supported representation formats of network operators */
        //offset += sprintf(buffer+offset, ",,(%d-%d),(%d,%d,%d)",
        //      AT_COPS_AUTO, AT_COPS_MANAUTO,
        //      AT_COPS_LONG, AT_COPS_SHORT, AT_COPS_NUM);
        offset += sprintf(buffer+offset, "\r\n");
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgatt()                               **
 **                                                                        **
 ** Description: Encodes AT CGATT command response message                 **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgatt(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cgatt_resp_t * cgatt = &(data->response.cgatt);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGATT: %d\r\n", cgatt->state);
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGATT: (%d,%d)\r\n",
                          AT_CGATT_STATE_MIN, AT_CGATT_STATE_MAX);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_creg()                                **
 **                                                                        **
 ** Description: Encodes AT CGREG command response message                 **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_creg(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_creg_resp_t * creg = &(data->response.creg);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CREG: %d,%d",
                          creg->n, creg->stat);

        if (data->mask & AT_CREG_RESP_LAC_MASK) {
            offset += sprintf(buffer+offset, ",%s", creg->lac);
        }

        if (data->mask & AT_CREG_RESP_CI_MASK) {
            offset += sprintf(buffer+offset, ",%s", creg->ci);
        }

        if (data->mask & AT_CREG_RESP_ACT_MASK) {
            offset += sprintf(buffer+offset, ",%d", creg->AcT);
        }

        offset += sprintf(buffer+offset, "\r\n");
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CREG: (%d-%d)\r\n",
                          AT_CREG_N_MIN, AT_CREG_N_MAX);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgreg()                               **
 **                                                                        **
 ** Description: Encodes AT CGREG command response message                 **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgreg(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cgreg_resp_t * cgreg = &(data->response.cgreg);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGREG: %d,%d",
                          cgreg->n, cgreg->stat);

        if (data->mask & AT_CGREG_RESP_LAC_MASK) {
            offset += sprintf(buffer+offset, ",%s", cgreg->lac);
        }

        if (data->mask & AT_CGREG_RESP_CI_MASK) {
            offset += sprintf(buffer+offset, ",%s", cgreg->ci);
        }

        if (data->mask & AT_CGREG_RESP_ACT_MASK) {
            offset += sprintf(buffer+offset, ",%d", cgreg->AcT);
        }

        if (data->mask & AT_CGREG_RESP_RAC_MASK) {
            offset += sprintf(buffer+offset, ",%s", cgreg->rac);
        }

        offset += sprintf(buffer+offset, "\r\n");
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGREG: (%d-%d)\r\n",
                          AT_CGREG_N_MIN, AT_CGREG_N_MAX);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cereg()                               **
 **                                                                        **
 ** Description: Encodes AT CEREG command response message                 **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cereg(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cereg_resp_t * cereg = &(data->response.cereg);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CEREG: %d,%d",
                          cereg->n, cereg->stat);

        if (data->mask & AT_CEREG_RESP_TAC_MASK) {
            offset += sprintf(buffer+offset, ",%s", cereg->tac);
        }

        if (data->mask & AT_CEREG_RESP_CI_MASK) {
            offset += sprintf(buffer+offset, ",%s", cereg->ci);
        }

        if (data->mask & AT_CEREG_RESP_ACT_MASK) {
            offset += sprintf(buffer+offset, ",%d", cereg->AcT);
        }

        offset += sprintf(buffer+offset, "\r\n");
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CEREG: (%d-%d)\r\n",
                          AT_CEREG_N_MIN, AT_CEREG_N_MAX);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgdcont()                             **
 **                                                                        **
 ** Description: Encodes AT CGDCONT command response message               **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgdcont(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;
    int i;

    if (data->type == AT_COMMAND_GET) {
        const at_cgdcont_get_t * cgdcont = &(data->response.cgdcont.get);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        /* Display the list of defined PDN contexts */
        for (i = 0; i < cgdcont->n_pdns; i++) {
            offset += sprintf(buffer+offset, "+CGDCONT: %u", (unsigned int)cgdcont->cid[i]);

            if (cgdcont->PDP_type[i] == NET_PDN_TYPE_IPV4) {
                offset += sprintf(buffer+offset, ",IP");
            } else if (cgdcont->PDP_type[i] == NET_PDN_TYPE_IPV6) {
                offset += sprintf(buffer+offset, ",IPV6");
            } else if (cgdcont->PDP_type[i] == NET_PDN_TYPE_IPV4V6) {
                offset += sprintf(buffer+offset, ",IPV4V6");
            }

            offset += sprintf(buffer+offset, ",%s", cgdcont->APN[i]);
            /* No data/header compression */
            offset += sprintf(buffer+offset, ",%u,%u\r\n",
                              (unsigned int)(AT_CGDCONT_D_COMP_OFF), (unsigned int)(AT_CGDCONT_H_COMP_OFF));
        }
    } else if (data->type == AT_COMMAND_TST) {
        const at_cgdcont_tst_t * cgdcont = &(data->response.cgdcont.tst);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        /* IPv4 PDN type */
        offset += sprintf(buffer+offset, "+CGDCONT: ");
        offset += sprintf(buffer+offset, "(1-%d),IP,,,(%d-%d),(%d-%d)",
                          cgdcont->n_cid,
                          AT_CGDCONT_D_COMP_MIN, AT_CGDCONT_D_COMP_MAX,
                          AT_CGDCONT_H_COMP_MIN, AT_CGDCONT_H_COMP_MAX);
        /* IPv6 PDN type */
        offset += sprintf(buffer+offset, "\r\n+CGDCONT: ");
        offset += sprintf(buffer+offset, "(1-%d),IPV6,,,(%d-%d),(%d-%d)",
                          cgdcont->n_cid,
                          AT_CGDCONT_D_COMP_MIN, AT_CGDCONT_D_COMP_MAX,
                          AT_CGDCONT_H_COMP_MIN, AT_CGDCONT_H_COMP_MAX);
        /* IPv4v6 PDN type */
        offset += sprintf(buffer+offset, "\r\n+CGDCONT: ");
        offset += sprintf(buffer+offset, "(1-%d),IPV4V6,,,(%d-%d),(%d-%d)",
                          cgdcont->n_cid,
                          AT_CGDCONT_D_COMP_MIN, AT_CGDCONT_D_COMP_MAX,
                          AT_CGDCONT_H_COMP_MIN, AT_CGDCONT_H_COMP_MAX);

        offset += sprintf(buffer+offset, "\r\n");
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgact()                               **
 **                                                                        **
 ** Description: Encodes AT CGACT command response message                 **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgact(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;
    int i;

    if (data->type == AT_COMMAND_GET) {
        const at_cgact_resp_t * cgact = &(data->response.cgact);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        /* Display the list of defined PDN status */
        for (i = 0; i < cgact->n_pdns; i++) {
            offset += sprintf(buffer+offset, "+CGACT: %u,%u\r\n",
                              (unsigned int)cgact->cid[i], (unsigned int)cgact->state[i]);
        }
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CGACT: (%d,%d)\r\n",
                          AT_CGACT_STATE_MIN, AT_CGACT_STATE_MAX);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgpaddr()                             **
 **                                                                        **
 ** Description: Encodes AT CGPADDR command response message               **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgpaddr(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;
    const at_cgpaddr_resp_t * cgpaddr = &(data->response.cgpaddr);
    int i;

    if (data->type == AT_COMMAND_SET) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        /* Display the list of IP addresses assigned to each defined PDN
         * connections */
        for (i = 0; i < cgpaddr->n_pdns; i++) {
            offset += sprintf(buffer+offset, "+CGPADDR: %u", (unsigned int)cgpaddr->cid[i]);

            if (cgpaddr->PDP_addr_1[i] != NULL) {
                /* IPv4 address */
                offset += sprintf(buffer+offset, ",%c.%c.%c.%c",
                                  cgpaddr->PDP_addr_1[i][0],
                                  cgpaddr->PDP_addr_1[i][1],
                                  cgpaddr->PDP_addr_1[i][2],
                                  cgpaddr->PDP_addr_1[i][3]);
            }

            if (cgpaddr->PDP_addr_2[i] != NULL) {
                /* IPv6 Link-local address prefixe */
                offset += sprintf(buffer+offset,
                                  ",%u.%u.%u.%u.%u.%u.%u.%u",
                                  0xfeU, 0x80U,
                                  0U, 0U,
                                  0U, 0U,
                                  0U, 0U);
                /* IPv6 Link-local address */
                offset += sprintf(buffer+offset,
                                  ".%c.%c.%c.%c.%c.%c.%c.%c",
                                  cgpaddr->PDP_addr_2[i][0],
                                  cgpaddr->PDP_addr_2[i][1],
                                  cgpaddr->PDP_addr_2[i][2],
                                  cgpaddr->PDP_addr_2[i][3],
                                  cgpaddr->PDP_addr_2[i][4],
                                  cgpaddr->PDP_addr_2[i][5],
                                  cgpaddr->PDP_addr_2[i][6],
                                  cgpaddr->PDP_addr_2[i][7]);
            }

            offset += sprintf(buffer+offset, "\r\n");
        }
    } else if (data->type == AT_COMMAND_TST) {
        /* Display the list of defined PDN contexts */
        if (cgpaddr->n_pdns > 0) {
            if (at_response_format_v1) {
                offset += sprintf(buffer, "\r\n");
            }

            offset += sprintf(buffer+offset, "+CGPADDR: %u", (unsigned int)cgpaddr->cid[0]);

            for (i = 1; i < cgpaddr->n_pdns; i++) {
                offset += sprintf(buffer+offset, ",%u", (unsigned int)cgpaddr->cid[i]);
            }

            offset += sprintf(buffer+offset, "\r\n");
        }
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cmee()                                **
 **                                                                        **
 ** Description: Encodes AT CMEE command response message                  **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cmee(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cmee_resp_t * cmee = &(data->response.cmee);

        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CMEE: %d\r\n", cmee->n);
    } else if (data->type == AT_COMMAND_TST) {
        if (at_response_format_v1) {
            offset += sprintf(buffer, "\r\n");
        }

        offset += sprintf(buffer+offset, "+CMEE: (%d,%d)\r\n",
                          AT_CMEE_N_MIN, AT_CMEE_N_MAX);
    }

    LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_response_encode_cgev()                                **
 **                                                                        **
 ** Description: Encodes AT CGEV unsolicited result response message       **
 **                                                                        **
 ** Inputs   data:    AT response data to encode                 **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters encoded in the    **
 **       data buffer                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_response_encode_cgev(char* buffer, const at_response_t* data)
{
    LOG_FUNC_IN;

    int offset = 0;

    if (data->type == AT_COMMAND_GET) {
        const at_cgev_resp_t * cgev = &(data->response.cgev);
        offset += sprintf(buffer+offset, "+CGEV: %s %u\r\n",
                          _at_response_event_str[cgev->code], (unsigned int)cgev->cid);
    }

    LOG_FUNC_RETURN (offset);
}

