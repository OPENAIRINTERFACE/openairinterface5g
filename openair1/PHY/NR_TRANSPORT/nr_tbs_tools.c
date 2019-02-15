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

/*! \file PHY/LTE_TRANSPORT/dlsch_coding.c
* \brief Top-level routines for implementing LDPC-coded (DLSCH) transport channels from 38-212, 15.2
* \author H.Wang
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "nr_dlsch.h"

/// Target code rate tables indexed by Imcs
uint16_t nr_target_code_rate_table1[29] = {120, 157, 193, 251, 308, 379, 449, 526, 602, 679, 340, 378, 434, 490, 553, \
                                            616, 658, 438, 466, 517, 567, 616, 666, 719, 772, 822, 873, 910, 948};
  // Imcs values 20 and 26 have been multiplied by 2 to avoid the floating point
uint16_t nr_target_code_rate_table2[28] = {120, 193, 308, 449, 602, 378, 434, 490, 553, 616, 658, 466, 517, 567, \
                                            616, 666, 719, 772, 822, 873, 1365, 711, 754, 797, 841, 885, 1833, 948};
uint16_t nr_target_code_rate_table3[29] = {30, 40, 50, 64, 78, 99, 120, 157, 193, 251, 308, 379, 449, 526, 602, 340, \
                                            378, 434, 490, 553, 616, 438, 466, 517, 567, 616, 666, 719, 772};
uint16_t nr_tbs_table[93] = {24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 208, 224, 240, 256, 272, 288, 304, 320, \
                              336, 352, 368, 384, 408, 432, 456, 480, 504, 528, 552, 576, 608, 640, 672, 704, 736, 768, 808, 848, 888, 928, 984, 1032, 1064, 1128, 1160, 1192, 1224, 1256, \
                              1288, 1320, 1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800, 1864, 1928, 2024, 2088, 2152, 2216, 2280, 2408, 2472, 2536, 2600, 2664, 2728, 2792, 2856, 2976, \
                              3104, 3240, 3368, 3496, 3624, 3752, 3824};

uint8_t nr_get_Qm(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (((Imcs<10)||(Imcs==29))?2:((Imcs<17)||(Imcs==30))?4:((Imcs<29)||(Imcs==31))?6:-1);
    break;

    case 2:
      return (((Imcs<5)||(Imcs==28))?2:((Imcs<11)||(Imcs==29))?4:((Imcs<20)||(Imcs==30))?6:((Imcs<28)||(Imcs==31))?8:-1);
    break;

    case 3:
      return (((Imcs<15)||(Imcs==29))?2:((Imcs<21)||(Imcs==30))?4:((Imcs<29)||(Imcs==31))?6:-1);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

uint32_t nr_get_code_rate(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (nr_target_code_rate_table1[Imcs]);
    break;

    case 2:
      return (nr_target_code_rate_table2[Imcs]);
    break;

    case 3:
      return (nr_target_code_rate_table3[Imcs]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

static inline uint8_t is_codeword_disabled(uint8_t format, uint8_t Imcs, uint8_t rv) {
  return ((format==NFAPI_NR_DL_DCI_FORMAT_1_1)&&(Imcs==26)&&(rv==1));
}

static inline uint8_t get_table_idx(uint8_t mcs_table, uint8_t dci_format, uint8_t rnti_type, uint8_t ss_type) {
  if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM256) && (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1) && ((rnti_type==NFAPI_NR_RNTI_C)||(rnti_type==NFAPI_NR_RNTI_CS)))
    return 2;
  else if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM64_LOW_SE) && (rnti_type!=NFAPI_NR_RNTI_new) && (rnti_type==NFAPI_NR_RNTI_C) && (ss_type==NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC))
    return 3;
  else if (rnti_type==NFAPI_NR_RNTI_new)
    return 3;
  else if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM256) && (rnti_type==NFAPI_NR_RNTI_CS) && (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1))
    return 2; // Condition mcs_table not configured in sps_config necessary here but not yet implemented
  /*else if((mcs_table == NFAPI_NR_MCS_TABLE_QAM64_LOW_SE) &&  (rnti_type==NFAPI_NR_RNTI_CS))
   *  table_idx = 3;
   * Note: the commented block refers to the case where the mcs_table is from sps_config*/
  else
    return 1;
}

void nr_get_tbs(nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu,
                nfapi_nr_dl_config_dci_dl_pdu dci_pdu,
                nfapi_nr_config_request_t config) {

  LOG_I(MAC, "TBS calculation\n");

  nfapi_nr_dl_config_pdcch_parameters_rel15_t params_rel15 = dci_pdu.pdcch_params_rel15;
  nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_rel15 = &dlsch_pdu->dlsch_pdu_rel15;
  uint8_t rnti_type = params_rel15.rnti_type;
  uint8_t dci_format = params_rel15.dci_format;
  uint8_t ss_type = params_rel15.search_space_type;
  uint8_t N_PRB_oh = ((rnti_type==NFAPI_NR_RNTI_SI)||(rnti_type==NFAPI_NR_RNTI_RA)||(rnti_type==NFAPI_NR_RNTI_P))? 0 : \
  (config.pdsch_config.x_overhead.value);
  uint8_t N_PRB_DMRS = (config.pdsch_config.dmrs_type.value == NFAPI_NR_DMRS_TYPE1)?6:4; //This only works for antenna port 1000
  uint8_t mcs_table = config.pdsch_config.mcs_table.value;
  uint8_t N_sh_symb = dlsch_rel15->nb_symbols;
  uint8_t Imcs = dlsch_rel15->mcs_idx;
  uint16_t N_RE_prime = NR_NB_SC_PER_RB*N_sh_symb - N_PRB_DMRS - N_PRB_oh;
  LOG_I(MAC, "N_RE_prime %d for %d symbols %d DMRS per PRB and %d overhead\n", N_RE_prime, N_sh_symb, N_PRB_DMRS, N_PRB_oh);

  uint16_t N_RE, Ninfo, Ninfo_prime, C, TBS=0, R;
  uint8_t table_idx, Qm, n, scale;

  table_idx = get_table_idx(mcs_table, dci_format, rnti_type, ss_type);
  scale = ((table_idx==2)&&((Imcs==20)||(Imcs==26)))?11:10;
  
  N_RE = min(156, N_RE_prime)*dlsch_rel15->n_prb;
  R = nr_get_code_rate(Imcs, table_idx);
  Qm = nr_get_Qm(Imcs, table_idx);
  Ninfo = (N_RE*R*Qm*dlsch_rel15->nb_layers)>>scale;

  if (Ninfo <= 3824) {
    n = max(3, (log2(Ninfo)-6));
    Ninfo_prime = max(24, (Ninfo>>n)<<n);
    for (int i=0; i<93; i++)
      if (nr_tbs_table[i] >= Ninfo_prime) {
        TBS = nr_tbs_table[i];
        break;
      }
  }
  else {
    n = log2(Ninfo-24)-5;
    Ninfo_prime = max(3840, (ROUNDIDIV((Ninfo-24),(1<<n)))<<n);

    if (R<256) {
      C = CEILIDIV((Ninfo_prime+24),3816);
      TBS = (C<<3)*CEILIDIV((Ninfo_prime+24),(C<<3)) - 24;
    }
    else {
      if (Ninfo_prime>8424) {
        C = CEILIDIV((Ninfo_prime+24),8424);
        TBS = (C<<3)*CEILIDIV((Ninfo_prime+24),(C<<3)) - 24;
      }
      else
        TBS = ((CEILIDIV((Ninfo_prime+24),8))<<3) - 24;
    }    
  }

  dlsch_rel15->coding_rate = R;
  dlsch_rel15->modulation_order = Qm;
  dlsch_rel15->transport_block_size = TBS;
  dlsch_rel15->nb_re_dmrs = N_PRB_DMRS;
  dlsch_rel15->nb_mod_symbols = N_RE_prime*dlsch_rel15->n_prb*dlsch_rel15->nb_codewords;

  LOG_I(MAC, "TBS %d : N_RE %d  N_PRB_DMRS %d N_sh_symb %d N_PRB_oh %d Ninfo %d Ninfo_prime %d R %d Qm %d table %d scale %d nb_symbols %d\n",
  TBS, N_RE, N_PRB_DMRS, N_sh_symb, N_PRB_oh, Ninfo, Ninfo_prime, R, Qm, table_idx, scale, dlsch_rel15->nb_mod_symbols);
}

uint32_t nr_get_G(uint16_t nb_rb, uint16_t nb_symb_sch,uint8_t nb_re_dmrs,uint16_t length_dmrs, uint8_t Qm, uint8_t Nl) {
	uint32_t G;
	G = ((NR_NB_SC_PER_RB*nb_symb_sch)-(nb_re_dmrs*length_dmrs))*nb_rb*Qm*Nl;
	return(G);
}

uint32_t nr_get_E(uint32_t G, uint8_t C, uint8_t Qm, uint8_t Nl, uint8_t r) {
  uint32_t E;
  uint8_t Cprime = C; //assume CBGTI not present

  AssertFatal(Nl>0,"Nl is 0\n");
  AssertFatal(Qm>0,"Qm is 0\n");

  if (r <= Cprime - ((G/(Nl*Qm))%Cprime) - 1)
      E = Nl*Qm*(G/(Nl*Qm*Cprime));
  else
      E = Nl*Qm*((G/(Nl*Qm*Cprime))+1);

  return E;
}
