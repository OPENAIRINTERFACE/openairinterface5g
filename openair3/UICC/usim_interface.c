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
#include <ctype.h>

#include <openair3/UICC/usim_interface.h>
#include <openair3/NAS/COMMON/milenage.h>
extern uint16_t NB_UE_INST;

#define UICC_SECTION    "uicc"
#define IMEISV_STR_MAX_LENGTH 16
#define UICC_CONFIG_HELP_OPTIONS     " list of comma separated options to interface a simulated (real UICC to be developped). Available options: \n"\
  "        imsi: user imsi\n"\
  "        key:  cyphering key\n"\
  "        opc:  cyphering OPc\n"\
  "        imiesv: string with IMEISV value\n"
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            configuration parameters for the rfsimulator device                                                                              */
/*   optname                     helpstr                     paramflags           XXXptr                               defXXXval                          type         numelt  */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define UICC_PARAMS_DESC {                                              \
      {"imsi",             "USIM IMSI\n",          0,         .strptr=&uicc->imsiStr,              .defstrval="2089900007487",           TYPE_STRING,    0 }, \
      {"nmc_size"          "number of digits in NMC", 0,      .iptr=&uicc->nmc_size,               .defintval=2,         TYPE_INT,       0 }, \
      {"key",              "USIM Ki\n",            0,         .strptr=&uicc->keyStr,               .defstrval="fec86ba6eb707ed08905757b1bb44b8f", TYPE_STRING,    0 }, \
      {"opc",              "USIM OPc\n",           0,         .strptr=&uicc->opcStr,               .defstrval="c42449363bbad02b66d16bc975d77cc1", TYPE_STRING,    0 }, \
      {"amf",              "USIM amf\n",           0,         .strptr=&uicc->amfStr,               .defstrval="8000",    TYPE_STRING,    0 }, \
      {"sqn",              "USIM sqn\n",           0,         .strptr=&uicc->sqnStr,               .defstrval="000000",  TYPE_STRING,    0 }, \
      {"dnn",              "UE dnn (apn)\n",       0,         .strptr=&uicc->dnnStr,               .defstrval="oai",     TYPE_STRING,    0 }, \
      {"nssai_sst",        "UE nssai\n",           0,         .iptr=&uicc->nssai_sst,              .defintval=1,    TYPE_INT,    0 }, \
      {"nssai_sd",         "UE nssai\n",           0,         .iptr=&uicc->nssai_sd,               .defintval=0xffffff,  TYPE_INT,       0 }, \
      {"imeisv",           "IMEISV\n",             0,         .strptr=&uicc->imeisvStr,            .defstrval="6754567890123413",           TYPE_STRING,    0 }, \
  };

static uicc_t** uiccArray=NULL;

const char *const hexTable = "0123456789abcdef";
static inline uint8_t mkDigit(unsigned char in) {
  for (int i=0; i<16; i++)
    if (tolower(in)==hexTable[i])
      return i;
  LOG_E(SIM,"Impossible hexa input: %c\n",in);
  return 0;
}

static inline void to_hex(char *in, uint8_t *out, int size) {
  for (size_t i=0; in[i]!=0 && i < size*2 ; i+=2) {
    *out++=(mkDigit(in[i]) << 4) | mkDigit(in[i+1]);
  }
}

uicc_t *init_uicc(char *sectionName) {
  uicc_t *uicc=(uicc_t *)calloc(sizeof(uicc_t),1);
  paramdef_t uicc_params[] = UICC_PARAMS_DESC;
  // here we call usim simulation, but calling actual usim is quite simple
  // the code is in open-cells.com => program_uicc open source
  // we can read the IMSI from the USIM
  // key, OPc, sqn, amf don't need to be read from the true USIM
  int ret = config_get(config_get_if(), uicc_params, sizeofArray(uicc_params), sectionName);
  AssertFatal(ret >= 0, "configuration couldn't be performed for uicc name: %s", sectionName);
  LOG_I(SIM,
        "UICC simulation: IMSI=%s, IMEISV=%s, Ki=%s, OPc=%s, DNN=%s, SST=0x%02x, SD=0x%06x\n",
        uicc->imsiStr, uicc->imeisvStr, uicc->keyStr, uicc->opcStr,
        uicc->dnnStr, uicc->nssai_sst, uicc->nssai_sd);
  to_hex(uicc->keyStr,uicc->key, sizeof(uicc->key) );
  to_hex(uicc->opcStr,uicc->opc, sizeof(uicc->opc) );
  to_hex(uicc->sqnStr,uicc->sqn, sizeof(uicc->sqn) );
  to_hex(uicc->amfStr,uicc->amf, sizeof(uicc->amf) );
  return uicc;
}

void uicc_milenage_generate(uint8_t *autn, uicc_t *uicc) {
  // here we call usim simulation, but calling actual usim is quite simple
  // the code is in open-cells.com => program_uicc open source
  milenage_generate(uicc->opc, uicc->amf, uicc->key,
                    uicc->sqn, uicc->rand, autn, uicc->ik, uicc->ck, uicc->milenage_res);
  log_dump(SIM,uicc->opc,sizeof(uicc->opc), LOG_DUMP_CHAR,"opc:");
  log_dump(SIM,uicc->amf,sizeof(uicc->amf), LOG_DUMP_CHAR,"amf:");
  log_dump(SIM,uicc->key,sizeof(uicc->key), LOG_DUMP_CHAR,"key:");
  log_dump(SIM,uicc->sqn,sizeof(uicc->sqn), LOG_DUMP_CHAR,"sqn:");
  log_dump(SIM,uicc->rand,sizeof(uicc->rand), LOG_DUMP_CHAR,"rand:");
  log_dump(SIM,uicc->ik,sizeof(uicc->ik), LOG_DUMP_CHAR,"milenage output ik:");
  log_dump(SIM,uicc->ck,sizeof(uicc->ck), LOG_DUMP_CHAR,"milenage output ck:");
  log_dump(SIM,uicc->milenage_res,sizeof(uicc->milenage_res), LOG_DUMP_CHAR,"milenage output res:");
  log_dump(SIM,autn,sizeof(autn), LOG_DUMP_CHAR,"milenage output autn:");
}

uicc_t * checkUicc(int Mod_id) {
  AssertFatal(Mod_id < NB_UE_INST, "Mod_id must be less than NB_UE_INST. Mod_id:%d NB_UE_INST:%d", Mod_id,NB_UE_INST);
  if(uiccArray==NULL){
    uiccArray=(uicc_t **)calloc(1,sizeof(uicc_t*)*NB_UE_INST);
  }
  if (!uiccArray[Mod_id]) {
    char uiccName[64];
    sprintf(uiccName,"uicc%d",  Mod_id);
    uiccArray[Mod_id]=(void*)init_uicc(uiccName);
  }
  return (uicc_t*) uiccArray[Mod_id];  
}

uint8_t getImeisvDigit(const uicc_t *uicc, uint8_t i)
{
  uint8_t r = 0;
  uint8_t l = strlen(uicc->imeisvStr);
  if (l > IMEISV_STR_MAX_LENGTH) {
    l=IMEISV_STR_MAX_LENGTH;
  }
  if((uicc->imeisvStr!=NULL) && (i<l))
  {
    char c = uicc->imeisvStr[i];
    if (isdigit(c))
    {
      r=c-'0';
    }
  }
  return (0x0f & r);
}
