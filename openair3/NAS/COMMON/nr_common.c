
#include <openair3/NAS/COMMON/NR_NAS_defs.h>
#include <openair3/SECU/secu_defs.h>

void servingNetworkName(uint8_t *msg, char * imsiStr, int nmc_size) {
  //SNN-network-identifier in TS 24.501
  // TS 24.501: If the MNC of the serving PLMN has two digits, then a zero is added at the beginning.
  const char *format="5G:mnc000.mcc000.3gppnetwork.org";
  memcpy(msg,format, strlen(format));

  if (nmc_size == 2)
    memcpy(msg+7, imsiStr+3, 2);
  else
    memcpy(msg+6, imsiStr+3, 3);

  memcpy(msg+13, imsiStr, 3);
}

int resToresStar(uint8_t *msg, uicc_t* uicc) {
  // TS 33.220  annex B.2 => FC=0x6B in TS 33.501 annex A.4
  //input S to KDF
  uint8_t S[128]= {0};
  S[0]=0x6B;
  uint8_t *ptr=S+1;
  servingNetworkName(ptr, uicc->imsiStr, uicc->nmc_size);
  *(uint16_t *)(ptr+strlen((char *)ptr))=htons(strlen((char *)ptr));
  ptr+=strlen((char *)ptr)+sizeof(uint16_t);
  // add rand
  memcpy(ptr, uicc->rand, sizeof(uicc->rand) ) ;
  *(uint16_t *)(ptr+sizeof(uicc->rand))=htons(sizeof(uicc->rand));
  ptr+=sizeof(uicc->rand)+sizeof(uint16_t);
  // add res
  memcpy(ptr, uicc->milenage_res, sizeof(uicc->milenage_res) ) ;
  *(uint16_t *)(ptr+sizeof(uicc->milenage_res))=htons(sizeof(uicc->milenage_res));
  ptr+=sizeof(uicc->milenage_res)+sizeof(uint16_t);
  // S is done
  uint8_t ckik[sizeof(uicc->ck) +sizeof(uicc->ik)];
  memcpy(ckik, uicc->ck, sizeof(uicc->ck));
  memcpy(ckik+sizeof(uicc->ck),uicc->ik, sizeof(uicc->ik));
  uint8_t out[32];
  kdf(S, ptr-S, ckik, 32, out, sizeof(out));
  memcpy(msg, out+16, 16);
  return 16;
}
