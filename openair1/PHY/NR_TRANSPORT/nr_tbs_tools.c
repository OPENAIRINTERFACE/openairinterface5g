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

/*! \file PHY/NR_TRANSPORT/nr_tbs_tools.c
* \brief Top-level routines for implementing LDPC-coded (DLSCH) transport channels from 38-212, 15.2
* \author H.Wang
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "nr_transport_common_proto.h"
#include "PHY/CODING/coding_defs.h"

//Table 5.1.3.1-1 of 38.214
uint16_t Table_51311[29][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};

//Table 5.1.3.1-2 of 38.214
// Imcs values 20 and 26 have been multiplied by 2 to avoid the floating point
uint16_t Table_51312[28][2] = {{2,120},{2,193},{2,308},{2,449},{2,602},{4,378},{4,434},{4,490},{4,553},{4,616},{4,658},{6,466},{6,517},{6,567},{6,616},{6,666},
		{6,719},{6,772},{6,822},{6,873},{8,1365},{8,711},{8,754},{8,797},{8,841},{8,885},{8,1833},{8,948}};

//Table 5.1.3.1-3 of 38.214
uint16_t Table_51313[29][2] = {{2,30},{2,40},{2,50},{2,64},{2,78},{2,99},{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{4,340},
		{4,378},{4,434},{4,490},{4,553},{4,616},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666}, {6,719}, {6,772}};

//Table 6.1.4.1-1 of 38.214 TODO fix for tp-pi2BPSK
uint16_t Table_61411[28][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};

//Table 6.1.4.1-2 of 38.214 TODO fix for tp-pi2BPSK
uint16_t Table_61412[28][2] = {{2,30},{2,40},{2,50},{2,64},{2,78},{2,99},{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},
		{4,378},{4,434},{4,490},{4,553},{4,616},{4,658},{4,699},{4,772},{6,567},{6,616},{6,666}, {6,772}};


uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][0]);
    break;

    case 1:
      return (Table_51312[Imcs][0]);
    break;

    case 2:
      return (Table_51313[Imcs][0]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][1]);
    break;

    case 1:
      return (Table_51312[Imcs][1]);
    break;

    case 2:
      return (Table_51313[Imcs][1]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][0]);
    break;

    case 1:
      return (Table_51312[Imcs][0]);
    break;

    case 2:
      return (Table_51313[Imcs][0]);
    break;

    case 3:
      return (Table_61411[Imcs][0]);
    break;

    case 4:
      return (Table_61412[Imcs][0]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,2])\n", table_idx);
  }
}

uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][1]);
    break;

    case 1:
      return (Table_51312[Imcs][1]);
    break;

    case 2:
      return (Table_51313[Imcs][1]);
    break;

    case 3:
      return (Table_61411[Imcs][1]);
    break;

    case 4:
      return (Table_61412[Imcs][1]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,2])\n", table_idx);
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

void nr_get_tbs_dl(nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu,
                   nfapi_nr_dl_config_dci_dl_pdu dci_pdu,
                   nfapi_nr_config_request_t config) {

  LOG_D(MAC, "TBS calculation\n");

  nfapi_nr_dl_config_pdcch_parameters_rel15_t params_rel15 = dci_pdu.pdcch_params_rel15;
  nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_rel15 = &dlsch_pdu->dlsch_pdu_rel15;
  uint8_t rnti_type = params_rel15.rnti_type;
  uint16_t N_PRB_oh = ((rnti_type==NFAPI_NR_RNTI_SI)||(rnti_type==NFAPI_NR_RNTI_RA)||(rnti_type==NFAPI_NR_RNTI_P))? 0 : \
  (config.pdsch_config.x_overhead.value);
  uint8_t N_PRB_DMRS = (config.pdsch_config.dmrs_type.value == NFAPI_NR_DMRS_TYPE1)?6:4; //This only works for antenna port 1000
  uint8_t N_sh_symb = dlsch_rel15->nb_symbols;
  uint8_t Imcs = dlsch_rel15->mcs_idx;
  uint16_t N_RE_prime = NR_NB_SC_PER_RB*N_sh_symb - N_PRB_DMRS - N_PRB_oh;
  LOG_D(MAC, "N_RE_prime %d for %d symbols %d DMRS per PRB and %d overhead\n", N_RE_prime, N_sh_symb, N_PRB_DMRS, N_PRB_oh);

  uint16_t R, TBS=0;
  uint8_t table_idx, Qm;

  /*uint8_t mcs_table = config.pdsch_config.mcs_table.value;
  uint8_t ss_type = params_rel15.search_space_type;
  uint8_t dci_format = params_rel15.dci_format;
  get_table_idx(mcs_table, dci_format, rnti_type, ss_type);*/
  table_idx = 0;
  R = nr_get_code_rate_dl(Imcs, table_idx);
  Qm = nr_get_Qm_dl(Imcs, table_idx);

  TBS = nr_compute_tbs(Qm,
                       R,
		       dlsch_rel15->n_prb,
		       N_sh_symb,
		       N_PRB_DMRS,
		       N_PRB_oh,
		       dlsch_rel15->nb_layers);

  dlsch_rel15->coding_rate = R;
  dlsch_rel15->modulation_order = Qm;
  dlsch_rel15->transport_block_size = TBS;
  dlsch_rel15->nb_re_dmrs = N_PRB_DMRS;
  dlsch_rel15->nb_mod_symbols = N_RE_prime*dlsch_rel15->n_prb*dlsch_rel15->nb_codewords;
  dlsch_rel15->mcs_table = table_idx;

  LOG_D(MAC, "TBS %d : N_PRB_DMRS %d N_sh_symb %d N_PRB_oh %d R %d Qm %d table %d nb_symbols %d\n",
  TBS, N_PRB_DMRS, N_sh_symb, N_PRB_oh, R, Qm, table_idx, dlsch_rel15->nb_mod_symbols);
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
