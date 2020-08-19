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
* Author and copyright: Laurent Thomas, open-cells.com
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

#include <openair3/UICC/usim_interface.h>

#define UICC_SECTION    "uicc"
#define UICC_CONFIG_HELP_OPTIONS     " list of comma separated options to interface a simulated (real UICC to be developped). Available options: \n"\
  "        imsi: user imsi\n"\
  "        key:  cyphering key\n"\
  "        opc:  cyphering OPc\n"
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            configuration parameters for the rfsimulator device                                                                              */
/*   optname                     helpstr                     paramflags           XXXptr                               defXXXval                          type         numelt  */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define UICC_PARAMS_DESC {\
    {"imsi",             "USIM IMSI\n",          0,         strptr:&(uicc->imsiStr),              defstrval:"",           TYPE_STRING,    0 },\
    {"nmc_size"          "number of digits in NMC", 0,      iptr:&(uicc->nmc_size),               defintval:2,            TYPE_INT,       0 },\
    {"key",              "USIM Ki\n",            0,         strptr:&(uicc->keyStr),               defstrval:"",           TYPE_STRING,    0 },\
    {"opc",              "USIM OPc\n",           0,         strptr:&(uicc->opcStr),               defstrval:"",           TYPE_STRING,    0 },\
    {"amf",              "USIM amf\n",           0,         strptr:&(uicc->amfStr),               defstrval:"8000",       TYPE_STRING,    0 },\
    {"sqn",              "USIM sqn\n",           0,         strptr:&(uicc->sqnStr),               defstrval:"000000",     TYPE_STRING,    0 },\
  };

const char *hexTable="0123456789abcdef";
static inline void to_hex(char *in, uint8_t *out, bool swap) {
  if (swap)
    for (size_t i=0; in[i]!=0; i++) {
      out+=hexTable[in[i]    & 0xf];
      out+=hexTable[in[i]>>4 & 0xf];
    } else {
    for (size_t i=0; in[i]!=0; i++) {
      out+=hexTable[in[i]>>4 & 0xf];
      out+=hexTable[in[i]    & 0xf];
    }
  }
}

uicc_t *init_uicc(char *sectionName) {
  uicc_t *uicc=(uicc_t *)calloc(sizeof(uicc_t),1);
  paramdef_t uicc_params[] = UICC_PARAMS_DESC;
  int ret = config_get( uicc_params,sizeof(uicc_params)/sizeof(paramdef_t),sectionName);
  AssertFatal(ret >= 0, "configuration couldn't be performed");
  LOG_I(HW, "UICC simulation: IMSI=%s, Ki=%s, OPc=%s\n", uicc->imsiStr, uicc->keyStr, uicc->opcStr);
  to_hex(uicc->keyStr,uicc->key, false);
  to_hex(uicc->opcStr,uicc->opc, false);
  to_hex(uicc->sqnStr,uicc->sqn, false);
  to_hex(uicc->amfStr,uicc->amf, false);
  return uicc;
}

