/*
   Adaptatipn of SW from hereafter license
   Author: laurent.thomas@open-cells.com
   The OpenAirInterface project uses this copy under the terms of BSD
   license.
*/
/*
   3GPP AKA - Milenage algorithm (3GPP TS 35.205, .206, .207, .208)
   Copyright (c) 2006-2007 <j@w1.fi>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   Alternatively, this software may be distributed under the terms of BSD
   license.

   See README and COPYING for more details.

   This file implements an example authentication algorithm defined for 3GPP
   AKA. This can be used to implement a simple HLR/AuC into hlr_auc_gw to allow
   EAP-AKA to be tested properly with real USIM cards.

   This implementations assumes that the r1..r5 and c1..c5 constants defined in
   TS 35.206 are used, i.e., r1=64, r2=0, r3=32, r4=64, r5=96, c1=00..00,
   c2=00..01, c3=00..02, c4=00..04, c5=00..08. The block cipher is assumed to
   be AES (Rijndael).
*/

#ifndef MILENAGE_H
#define MILENAGE_H
#include <openssl/aes.h>


#define u8 uint8_t
/**
   milenage_f1 - Milenage f1 and f1* algorithms
   @opc: OPc = 128-bit value derived from OP and K
   @k: K = 128-bit subscriber key
   @_rand: RAND = 128-bit random challenge
   @sqn: SQN = 48-bit sequence number
   @amf: AMF = 16-bit authentication management field
   @mac_a: Buffer for MAC-A = 64-bit network authentication code, or %NULL
   @mac_s: Buffer for MAC-S = 64-bit resync authentication code, or %NULL
   Returns: true on success, false on failure
*/

void aes_128_encrypt_block(const u8* key, const u8* in, u8* out) {
   AES_KEY k;
  u8 tmp[16];
  AES_set_encrypt_key(key, 128, &k);
  AES_encrypt(in, tmp, &k); 
  memcpy(out, tmp, sizeof(tmp));
}
static bool milenage_f1(const u8 *opc, const u8 *k, const u8 *_rand,
                 const u8 *sqn, const u8 *amf, u8 *mac_a, u8 *mac_s) {
  u8 tmp1[16], tmp2[16], tmp3[16];
  int i;

  /* tmp1 = TEMP = E_K(RAND XOR OP_C) */
  for (i = 0; i < 16; i++)
    tmp1[i] = _rand[i] ^ opc[i];

  aes_128_encrypt_block(k, tmp1, tmp1);
  AES_KEY kTab;
  uint8_t cyphered[16];
  AES_set_encrypt_key(k, 128, &kTab);
  AES_encrypt(tmp1, cyphered, &kTab);
  /* tmp2 = IN1 = SQN || AMF || SQN || AMF */
  memcpy(tmp2, sqn, 6);
  memcpy(tmp2 + 6, amf, 2);
  memcpy(tmp2 + 8, tmp2, 8);

  /* OUT1 = E_K(TEMP XOR rot(IN1 XOR OP_C, r1) XOR c1) XOR OP_C */

  /* rotate (tmp2 XOR OP_C) by r1 (= 0x40 = 8 bytes) */
  for (i = 0; i < 16; i++)
    tmp3[(i + 8) % 16] = tmp2[i] ^ opc[i];

  /* XOR with TEMP = E_K(RAND XOR OP_C) */
  for (i = 0; i < 16; i++)
    tmp3[i] ^= tmp1[i];

  /* XOR with c1 (= ..00, i.e., NOP) */
  /* f1 || f1* = E_K(tmp3) XOR OP_c */
  aes_128_encrypt_block(k, tmp3, tmp1);

  for (i = 0; i < 16; i++)
    tmp1[i] ^= opc[i];

  if (mac_a)
    memcpy(mac_a, tmp1, 8); /* f1 */

  if (mac_s)
    memcpy(mac_s, tmp1 + 8, 8); /* f1* */

  return true;
}


/**
   milenage_f2345 - Milenage f2, f3, f4, f5, f5* algorithms
   @opc: OPc = 128-bit value derived from OP and K
   @k: K = 128-bit subscriber key
   @_rand: RAND = 128-bit random challenge
   @res: Buffer for RES = 64-bit signed response (f2), or %NULL
   @ck: Buffer for CK = 128-bit confidentiality key (f3), or %NULL
   @ik: Buffer for IK = 128-bit integrity key (f4), or %NULL
   @ak: Buffer for AK = 48-bit anonymity key (f5), or %NULL
   @akstar: Buffer for AK = 48-bit anonymity key (f5*), or %NULL
   Returns: true on success, false on failure
*/
static bool milenage_f2345(const u8 *opc, const u8 *k, const u8 *_rand,
                    u8 *res, u8 *ck, u8 *ik, u8 *ak, u8 *akstar) {
  u8 tmp1[16], tmp2[16], tmp3[16];
  int i;

  /* tmp2 = TEMP = E_K(RAND XOR OP_C) */
  for (i = 0; i < 16; i++)
    tmp1[i] = _rand[i] ^ opc[i];

  aes_128_encrypt_block(k, tmp1, tmp2);

  /* OUT2 = E_K(rot(TEMP XOR OP_C, r2) XOR c2) XOR OP_C */
  /* OUT3 = E_K(rot(TEMP XOR OP_C, r3) XOR c3) XOR OP_C */
  /* OUT4 = E_K(rot(TEMP XOR OP_C, r4) XOR c4) XOR OP_C */
  /* OUT5 = E_K(rot(TEMP XOR OP_C, r5) XOR c5) XOR OP_C */

  /* f2 and f5 */
  /* rotate by r2 (= 0, i.e., NOP) */
  for (i = 0; i < 16; i++)
    tmp1[i] = tmp2[i] ^ opc[i];

  tmp1[15] ^= 1; /* XOR c2 (= ..01) */
  /* f5 || f2 = E_K(tmp1) XOR OP_c */
  aes_128_encrypt_block(k, tmp1, tmp3);

  for (i = 0; i < 16; i++)
    tmp3[i] ^= opc[i];

  if (res)
    memcpy(res, tmp3 + 8, 8); /* f2 */

  if (ak)
    memcpy(ak, tmp3, 6); /* f5 */

  /* f3 */
  if (ck) {
    /* rotate by r3 = 0x20 = 4 bytes */
    for (i = 0; i < 16; i++)
      tmp1[(i + 12) % 16] = tmp2[i] ^ opc[i];

    tmp1[15] ^= 2; /* XOR c3 (= ..02) */
    aes_128_encrypt_block(k, tmp1, ck);

    for (i = 0; i < 16; i++)
      ck[i] ^= opc[i];
  }

  /* f4 */
  if (ik) {
    /* rotate by r4 = 0x40 = 8 bytes */
    for (i = 0; i < 16; i++)
      tmp1[(i + 8) % 16] = tmp2[i] ^ opc[i];

    tmp1[15] ^= 4; /* XOR c4 (= ..04) */
    aes_128_encrypt_block(k, tmp1, ik);

    for (i = 0; i < 16; i++)
      ik[i] ^= opc[i];
  }

  /* f5* */
  if (akstar) {
    /* rotate by r5 = 0x60 = 12 bytes */
    for (i = 0; i < 16; i++)
      tmp1[(i + 4) % 16] = tmp2[i] ^ opc[i];

    tmp1[15] ^= 8; /* XOR c5 (= ..08) */
    aes_128_encrypt_block(k, tmp1, tmp1);

    for (i = 0; i < 6; i++)
      akstar[i] = tmp1[i] ^ opc[i];
  }

  return true;
}


/**
   milenage_generate - Generate AKA AUTN,IK,CK,RES
   @opc: OPc = 128-bit operator variant algorithm configuration field (encr.)
   @amf: AMF = 16-bit authentication management field
   @k: K = 128-bit subscriber key
   @sqn: SQN = 48-bit sequence number
   @_rand: RAND = 128-bit random challenge
   @autn: Buffer for AUTN = 128-bit authentication token
   @ik: Buffer for IK = 128-bit integrity key (f4), or %NULL
   @ck: Buffer for CK = 128-bit confidentiality key (f3), or %NULL
   @res: Buffer for RES = 64-bit signed response (f2), or %NULL
   @res_len: Max length for res; set to used length or 0 on failure
*/
static bool milenage_generate(const u8 *opc, const u8 *amf, const u8 *k,
                       const u8 *sqn, const u8 *_rand, u8 *autn, u8 *ik,
                       u8 *ck, u8 *res) {
  int i;
  u8 mac_a[8], ak[6];

  if (!milenage_f1(opc, k, _rand, sqn, amf, mac_a, NULL) ||
      !milenage_f2345(opc, k, _rand, res, ck, ik, ak, NULL))
    return false;

  /* AUTN = (SQN ^ AK) || AMF || MAC */
  for (i = 0; i < 6; i++)
    autn[i] = sqn[i] ^ ak[i];

  memcpy(autn + 6, amf, 2);
  memcpy(autn + 8, mac_a, 8);
  return true;
}


/**
   milenage_auts - Milenage AUTS validation
   @opc: OPc = 128-bit operator variant algorithm configuration field (encr.)
   @k: K = 128-bit subscriber key
   @_rand: RAND = 128-bit random challenge
   @auts: AUTS = 112-bit authentication token from client
   @sqn: Buffer for SQN = 48-bit sequence number
   Returns: 0 = success (sqn filled), -1 on failure
*/
#define p(a) printf("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", (int)((a)[0]), (int)((a)[1]), (int)((a)[2]), (int)((a)[3]),(int)((a)[4]), (int)((a)[5]));
// valid code but put in comment as it is not used right now
#if 0
static bool milenage_auts(const u8 *opc, const u8 *k, const u8 *_rand, const u8 *auts,
                   u8 *sqn) {
  u8 amf[2] = { 0x00, 0x00 }; /* TS 33.102 v7.0.0, 6.3.3 */
  u8 ak[6], mac_s[8];
  int i;

  if (!milenage_f2345(opc, k, _rand, NULL, NULL, NULL, NULL, ak))
    return false;

  for (i = 0; i < 6; i++)
    sqn[i] = auts[i] ^ ak[i];

  //p(sqn);
  if (!milenage_f1(opc, k, _rand, sqn, amf, NULL, mac_s) ||
      memcmp(mac_s, auts + 6, 8) != 0)
    return false;

  return true;
}


/**
   gsm_milenage - Generate GSM-Milenage (3GPP TS 55.205) authentication triplet
   @opc: OPc = 128-bit operator variant algorithm configuration field (encr.)
   @k: K = 128-bit subscriber key
   @_rand: RAND = 128-bit random challenge
   @sres: Buffer for SRES = 32-bit SRES
   @kc: Buffer for Kc = 64-bit Kc
   Returns: 0 on success, -1 on failure
*/
static bool gsm_milenage(const u8 *opc, const u8 *k, const u8 *_rand, u8 *sres, u8 *kc) {
  u8 res[8], ck[16], ik[16];
  int i;

  if (milenage_f2345(opc, k, _rand, res, ck, ik, NULL, NULL))
    return false;

  for (i = 0; i < 8; i++)
    kc[i] = ck[i] ^ ck[i + 8] ^ ik[i] ^ ik[i + 8];

#ifdef GSM_MILENAGE_ALT_SRES
  memcpy(sres, res, 4);
#else /* GSM_MILENAGE_ALT_SRES */

  for (i = 0; i < 4; i++)
    sres[i] = res[i] ^ res[i + 4];

#endif /* GSM_MILENAGE_ALT_SRES */
  return true;
}


/**
   milenage_generate - Generate AKA AUTN,IK,CK,RES
   @opc: OPc = 128-bit operator variant algorithm configuration field (encr.)
   @k: K = 128-bit subscriber key
   @sqn: SQN = 48-bit sequence number
   @_rand: RAND = 128-bit random challenge
   @autn: AUTN = 128-bit authentication token
   @ik: Buffer for IK = 128-bit integrity key (f4), or %NULL
   @ck: Buffer for CK = 128-bit confidentiality key (f3), or %NULL
   @res: Buffer for RES = 64-bit signed response (f2), or %NULL
   @res_len: Variable that will be set to RES length
   @auts: 112-bit buffer for AUTS
   Returns: 0 on success, -1 on failure, or -2 on synchronization failure
*/
static int milenage_check(const u8 *opc, const u8 *k, const u8 *sqn, const u8 *_rand,
                   const u8 *autn, u8 *ik, u8 *ck, u8 *res, size_t *res_len,
                   u8 *auts) {
  int i;
  u8 mac_a[8], ak[6], rx_sqn[6];
  const u8 *amf;

  if (milenage_f2345(opc, k, _rand, res, ck, ik, ak, NULL))
    return -1;

  *res_len = 8;

  /* AUTN = (SQN ^ AK) || AMF || MAC */
  for (i = 0; i < 6; i++)
    rx_sqn[i] = autn[i] ^ ak[i];

  if (memcmp(rx_sqn, sqn, 6) <= 0) {
    u8 auts_amf[2] = { 0x00, 0x00 }; /* TS 33.102 v7.0.0, 6.3.3 */

    if (milenage_f2345(opc, k, _rand, NULL, NULL, NULL, NULL, ak))
      return -1;

    for (i = 0; i < 6; i++)
      auts[i] = sqn[i] ^ ak[i];

    if (milenage_f1(opc, k, _rand, sqn, auts_amf, NULL, auts + 6))
      return -1;

    return -2;
  }

  amf = autn + 6;

  if (milenage_f1(opc, k, _rand, rx_sqn, amf, mac_a, NULL))
    return -1;

  if (memcmp(mac_a, autn + 8, 8) != 0) {
    printf( "Milenage: MAC mismatch\n");
    return -1;
  }

  return 0;
}

static void milenage_opc_gen(const u8 *k, const u8 *op, u8 *opc) {
  int i;
  /* Encrypt OP using K */
  aes_128_encrypt_block(k, op, opc);

  /* XOR the resulting Ek(OP) with OP */
  for (i = 0; i < 16; i++)
    opc[i] = opc[i] ^ op[i];
}
#endif
#endif
