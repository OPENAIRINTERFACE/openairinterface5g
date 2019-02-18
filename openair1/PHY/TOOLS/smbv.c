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

/*
  This library offers some functions to remotely program the R&S SMBV100A.
*/

#include "smbv.h"

#ifndef CONFIG_SMBV

int smbv_post_processing(const char* fname)
{
  /*FILE *f_ptr;
   Issue seems to be fixed in latest firmware

  int sleep_time = 1e6; // this is a second

  // this command appends all occurrences of CW:DSEL at the end of the config file.
  // this is necessary because as soon as a user is configured in the SMBV, the already selected data list get reset to PN9
  // maybe that is a bug in the SMBV...
  char shell_cmd[256] = "cat ";
  strcat(shell_cmd,fname);
  strcat(shell_cmd," | grep \"CW:DATA DLIS\" | cat >> ");
  strcat(shell_cmd,fname);

  //    msg("Command: %s\n",shell_cmd);

  popen(shell_cmd,"r");
  usleep(sleep_time); // make sure shell command completes


  // Turn on EUTRA
  //    fprintf(f_ptr, "BB:EUTR:STAT ON\n");
  fclose(f_ptr);    */
  return 0;
}

int smbv_init_config(const char* fname, uint16_t sequence_length)
{

  FILE *f_ptr;
  int k;

  f_ptr = fopen(fname,"w");

  if (sequence_length>4) {
    msg("ERROR invalid sequence length: %d, maximum is 4\n", sequence_length);
    fclose(f_ptr);
    return -1;
  } else
    slen = sequence_length;

  fprintf(f_ptr, "*RST;*CLS\n");
  fprintf(f_ptr, "BB:EUTR:PRES\n");
  fprintf(f_ptr, "FREQ 1.9076 GHZ\n");
  fprintf(f_ptr, "BB:EUTR:LINK DOWN\n");
  fprintf(f_ptr, "BB:EUTR:DL:BUR DTX\n"); // unallocated resource no dummy

  // K55 Configuration Mode
  fprintf(f_ptr, "BB:EUTR:DL:CONF:MODE AUTO\n");

  // disable all PDCCH and delete all DCI entries
  for (k=0; k<(10*slen); k++) {
    // turn PDCCH off
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:STAT OFF\n",k);
    // set USER to P-RNTI -> is not used in oai, so no risk of accidental decoding
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM0:USER PRNT\n",k);
    // deleting the only item in DCI table for all SFs makes SMBV crash somehow
    // selects row 0 in the DCI table
    //fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:SIT 0\n",k);
    // deletes row 0 in the DCI table
    // fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:DEL\n",k);
  }

  fclose(f_ptr);
  return 0;
}

int smbv_write_config_from_frame_parms(const char* fname, LTE_DL_FRAME_PARMS *frame_parms)
{

  FILE *f_ptr;

  f_ptr = fopen(fname,"a");

  fprintf(f_ptr, "BB:EUTR:SLEN %d\n",slen);

  if(frame_parms->frame_type == TDD) {
    fprintf(f_ptr, "BB:EUTR:DUPL TDD\n");
  } else {
    fprintf(f_ptr, "BB:EUTR:DUPL FDD\n");
  }

  // Physical Settings
  fprintf(f_ptr, "BB:EUTR:DL:BW USER\n");
  fprintf(f_ptr, "BB:EUTR:DL:NORB %d\n",frame_parms->N_RB_DL);
  fprintf(f_ptr, "BB:EUTR:DL:FFT %d\n",frame_parms->ofdm_symbol_size);

  // TDD Frame Structure
  fprintf(f_ptr, "BB:EUTR:TDD:UDC %d\n",frame_parms->tdd_config);
  fprintf(f_ptr, "BB:EUTR:TDD:SPSC %d\n",frame_parms->tdd_config_S);

  // MIMO
  fprintf(f_ptr, "BB:EUTR:DL:MIMO:CONF TX%d\n",frame_parms->nb_antenna_ports_eNB);
  fprintf(f_ptr, "BB:EUTR:DL:MIMO:ANTA ANT1\n");
  fprintf(f_ptr, "BB:EUTR:DL:MIMO:ANTB NONE\n");

  // Cell Specific Settings
  fprintf(f_ptr, "BB:EUTR:DL:PLC:CID %d\n",frame_parms->Nid_cell);

  if (frame_parms->Ncp == NORMAL) {
    fprintf(f_ptr, "BB:EUTR:DL:CPC NORM\n");
  } else {
    fprintf(f_ptr, "BB:EUTR:DL:CPC EXT\n");
  }

  fprintf(f_ptr, "BB:EUTR:DL:PDSC:PB %d\n",frame_parms->pdsch_config_common.p_b);
  fprintf(f_ptr, "BB:EUTR:DL:PDSC:RATB %d\n",0);
  fprintf(f_ptr, "BB:EUTR:DL:PDCC:RATB %d\n",0);
  fprintf(f_ptr, "BB:EUTR:DL:PBCH:RATB %d\n",0);

  if (frame_parms->phich_config_common.phich_duration == normal) {
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:DUR NORM\n");
  } else {
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:DUR EXT\n");
  }

  switch (frame_parms->phich_config_common.phich_resource) {
  case oneSixth:
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:NGP NG1_6\n");
    break;

  case half:
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:NGP NG1_2\n");
    break;

  case one:
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:NGP NG1\n");
    break;

  case two:
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:NGP NG2\n");
    break;

  default:
    fprintf(f_ptr, "BB:EUTR:DL:PHIC:NGP NG1_6\n");
    break;
  }

  // Synchronization
  fprintf(f_ptr, "BB:EUTR:DL:SYNC:TXAN ALL\n");
  fprintf(f_ptr, "BB:EUTR:DL:SYNC:PPOW 0\n");
  fprintf(f_ptr, "BB:EUTR:DL:SYNC:SPOW 0\n");

  // Number of configurable SF
  if (frame_parms->tdd_config == 3) {
    fprintf(f_ptr, "BB:EUTR:DL:CONS %d\n",7*((slen>4) ? 4 : slen));
  } else {
    fprintf(f_ptr, "BB:EUTR:DL:CONS 1\n");
  }

  fclose(f_ptr);
  return 0;
}

// This function creates a datalist for an allocation containing the data in buffer
// len is in bytes
int smbv_configure_datalist_for_alloc(const char* fname, uint8_t alloc, uint8_t subframe, uint8_t *buffer, uint32_t len)
{
  FILE *f_ptr;
  f_ptr = fopen(fname,"a");
  char strbuf[(8<<1)+1];
  int i,k;
  strbuf[0] = '\0';

  // set data list as source
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ALL%d:CW:DATA DLIS\n",subframe,alloc);

  // create data list
  fprintf(f_ptr, "BB:DM:DLIS:SEL \"alloc%dsf%d\"\n",alloc,subframe);

  for (k=7; k>0; k--) {
    strcat(strbuf,(buffer[0]&(1<<k))?"1,":"0,");
  }

  strcat(strbuf,(buffer[0]&1)?"1":"0");
  fprintf(f_ptr, "BB:DM:DLIS:DATA %s\n",strbuf);

  strbuf[0] = '\0';

  for (i=1; i<len; i++) {
    for (k=7; k>0; k--) {
      strcat(strbuf,(buffer[i]&(1<<k))?"1,":"0,");
    }

    strcat(strbuf,(buffer[i]&1)?"1":"0");
    fprintf(f_ptr, "BB:DM:DLIS:DATA:APP %s\n",strbuf);
    strbuf[0] = '\0';
  }

  // select the created data list
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ALL%d:CW:DSEL \"alloc%dsf%d\"\n",subframe,alloc,alloc,subframe);

  fclose(f_ptr);
  return 0;
}

int smbv_is_config_frame(uint32_t frame)
{

  extern const unsigned short config_frames[MAX_CONFIG_FRAMES];
  int i;

  for(i=0; i<MAX_CONFIG_FRAMES; i++) {
    if (config_frames[i] == frame) {
      return 1;
    }
  }

  return 0;
}

// This function creates a datalist for user containing the data in buffer
// len is in bytes
int smbv_configure_datalist_for_user(const char* fname, uint8_t user, uint8_t *buffer, uint32_t len)
{
  FILE *f_ptr;
  f_ptr = fopen(fname,"a");
  char strbuf[(8<<1)+1];
  int i,k;

  strbuf[0] = '\0';

  // create data list
  fprintf(f_ptr, "BB:DM:DLIS:SEL \"user%d\"\n",user);

  for (k=7; k>0; k--) {
    strcat(strbuf,(buffer[0]&(1<<k))?"1,":"0,");
  }

  strcat(strbuf,(buffer[0]&1)?"1":"0");
  fprintf(f_ptr, "BB:DM:DLIS:DATA %s\n",strbuf);

  strbuf[0] = '\0';

  for (i=1; i<len; i++) {
    for (k=7; k>0; k--) {
      strcat(strbuf,(buffer[i]&(1<<k))?"1,":"0,");
    }

    strcat(strbuf,(buffer[i]&1)?"1":"0");
    fprintf(f_ptr, "BB:DM:DLIS:DATA:APP %s\n",strbuf);
    strbuf[0] = '\0';
  }


  fprintf(f_ptr, "BB:EUTR:DL:USER%d:DATA DLIS\n",user);
  fprintf(f_ptr, "BB:EUTR:DL:USER%d:DSEL 'user%d'\n",user,user);

  fclose(f_ptr);
  return 0;
}

int smbv_configure_user(const char* fname, uint8_t user, uint8_t transmission_mode, uint16_t rnti)
{
  FILE *f_ptr;
  f_ptr = fopen(fname,"a");

  fprintf(f_ptr, "BB:EUTR:DL:USER%d:TXM M1\n",user);
  fprintf(f_ptr, "BB:EUTR:DL:USER%d:UEC USER\n",user);
  fprintf(f_ptr, "BB:EUTR:DL:USER%d:CCOD:STAT ON\n",user);
  fprintf(f_ptr, "BB:EUTR:DL:USER%d:UEID %d\n",user,rnti);
  //    fprintf(f_ptr, "BB:EUTR:DL:USER2:PA \n");

  fclose(f_ptr);
  return 0;
};

int smbv_configure_pdcch(const char* fname,uint8_t subframe,uint8_t num_pdcch_symbols,uint8_t num_dci)
{

  int k;
  FILE *f_ptr;

  f_ptr = fopen(fname,"a");

  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PCF:CREG %d\n",subframe,num_pdcch_symbols);
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:FORM VAR\n",subframe);
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:DCR:TRS DATA\n",subframe);

  // add as many rows in the DCI table as there are number of DCIs
  if (num_dci>0) {
    for (k=0; k<num_dci; k++) {
      fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:APP\n",subframe);
    }
  }

  fclose(f_ptr);
  return 0;
}

int smbv_configure_common_dci(const char* fname, uint8_t subframe, const char* type, DCI_ALLOC_t *dci_alloc, uint8_t item)
{

  FILE *f_ptr;
  void *dci_pdu = &dci_alloc->dci_pdu[0];

  f_ptr = fopen(fname,"a");

  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:STAT ON\n",subframe);

  if (strcmp(type,"SI")==0)
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:USER SIRN\n",subframe,item);
  else if (strcmp(type,"RA")==0) {
    //        msg("[SMBV] RA-RNTI %d\n",dci_alloc->rnti);
    fprintf(f_ptr, "BB:EUTR:DL:CSET:RARN %d\n",dci_alloc->rnti);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:USER RARN\n",subframe,item);
  } else if (strcmp(type,"PA")==0)
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:USER PRNT\n",subframe,item);
  else
    msg("Unknown DCI source\n");

  // set PDCCH format (equivalent to log2(aggregation level))
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:PFMT %d\n",subframe,item,dci_alloc->L);
  // set first CCE index
  //    msg("[SMBV] nCCE %d\n",dci_alloc->nCCE);
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:CIND %d\n",subframe,item,dci_alloc->nCCE);

  // Configure DCI
  switch (dci_alloc->format) {
  case format1A:
    //   msg("[SMBV] SF %d, Item %d, format 1A\n",subframe,item);
    //   msg("[SMBV] HPN %d, mcs %d, ndi %s, rba %d, rv %d, tpc %d, dai %d\n",((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->harq_pid,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->mcs,(((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->ndi==0)?"OFF":"ON",((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->rballoc,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->rv,((DCI1A_5MHz_TDD_1_6_t *)&dci_pdu)->TPC,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->dai);

    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F1A\n",subframe,item);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:F1AM PDSC\n",subframe,item);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:HPN %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->harq_pid);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:MCSR %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->mcs);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:NDI %s\n",subframe,item,(((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->ndi==0)?"OFF":"ON");
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RBA %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->rballoc);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RV %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->rv);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:TPCC %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->TPC);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:DLA %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->dai);

    break;

  case format1C:
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F1C\n",subframe,item);
    break;

  default:
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F1A\n",subframe,item);
    break;
  }

  fclose(f_ptr);
  return 0;
}

int smbv_configure_ue_spec_dci(const char* fname, uint8_t subframe, uint8_t user, DCI_ALLOC_t *dci_alloc, uint8_t item)
{

  FILE *f_ptr;
  void *dci_pdu = &dci_alloc->dci_pdu[0];

  f_ptr = fopen(fname,"a");

  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:STAT ON\n",subframe);
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:USER USER%d\n",subframe,item,user);

  // set PDCCH format (equivalent to log2(aggregation level))
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:PFMT %d\n",subframe,item,dci_alloc->L);
  // set first CCE index
  //    msg("[SMBV] nCCE %d\n",dci_alloc->nCCE);
  fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:CIND %d\n",subframe,item,dci_alloc->nCCE);

  // Configure DCI
  switch (dci_alloc->format) {
  case format0:
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F0\n",subframe,item);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:CSIR %d\n",subframe,item,(((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->cqi_req));
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:CSDM %d\n",subframe,item,((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->cshift);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:DLA %d\n",subframe,item,((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->dai);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:MCSR %d\n",subframe,item,((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->mcs);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:NDI %s\n",subframe,item,(((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->ndi==0)?"OFF":"ON");
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:PFH %s\n",subframe,item,(((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->hopping==0)?"OFF":"ON");
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RAHR %d\n",subframe,item,((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->rballoc);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:TPCC %d\n",subframe,item,((DCI0_5MHz_TDD_1_6_t *)dci_pdu)->TPC);
    break;

  case format1:
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F1\n",subframe,item);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:DLA %d\n",subframe,item,((DCI1_5MHz_TDD_t *)dci_pdu)->dai);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:HPN %d\n",subframe,item,((DCI1_5MHz_TDD_t *)dci_pdu)->harq_pid);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:MCSR %d\n",subframe,item,((DCI1_5MHz_TDD_t *)dci_pdu)->mcs);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:NDI %s\n",subframe,item,(((DCI1_5MHz_TDD_t *)dci_pdu)->ndi==0)?"OFF":"ON");
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RAH %s\n",subframe,item,(((DCI1_5MHz_TDD_t *)dci_pdu)->rah==0)?"OFF":"ON");
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RBA %d\n",subframe,item,((DCI1_5MHz_TDD_t *)dci_pdu)->rballoc);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RV %d\n",subframe,item,((DCI1_5MHz_TDD_t *)dci_pdu)->rv);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:TPCC %d\n",subframe,item,((DCI1_5MHz_TDD_t *)dci_pdu)->TPC);
    break;

  case format1A:
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F1A\n",subframe,item);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:F1AM PDSC\n",subframe,item);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:HPN %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->harq_pid);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:MCSR %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->mcs);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:NDI %s\n",subframe,item,(((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->ndi==0)?"OFF":"ON");
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RBA %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->rballoc);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:RV %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->rv);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:TPCC %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->TPC);
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIC:DLA %d\n",subframe,item,((DCI1A_5MHz_TDD_1_6_t *)dci_pdu)->dai);
    break;

  default:
    fprintf(f_ptr, "BB:EUTR:DL:SUBF%d:ENCC:PDCC:EXTC:ITEM%d:DCIF F1\n",subframe,item);
    break;
  }

  fclose(f_ptr);
  return 0;
}
#endif

int smbv_send_config (const char* fname, char* smbv_ip)
{

  struct addrinfo hints, *res;
  char *id_msg = "*IDN?\n";
  int bytes_sent, status, sockfd;
  char buf[BUFFER_LENGTH];

  memset(buf,0,BUFFER_LENGTH);

  // first, load up address structs with getaddrinfo():
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(smbv_ip, MYPORT, &hints, &res);

  if (status!=0) {
    msg("ERROR: getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

  // make a socket:
  msg("Connecting to socket with IP %s on port %s\n",smbv_ip,MYPORT);
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  // connect!
  status = connect(sockfd, res->ai_addr, res->ai_addrlen);

  if (status!=0) {
    msg("ERROR: connect: %s. Check IP address %s and port number %s\n", gai_strerror(status),smbv_ip, MYPORT);
  }

  // send command
  bytes_sent = send(sockfd, id_msg, strlen(id_msg), 0);
  //    msg("bytes sent: %d\n",bytes_sent);

  status = recv(sockfd, (void*) buf, BUFFER_LENGTH, 0);

  if (status==-1) {
    msg("ERROR receiving machine identification: %d\n", status);
    return -1;
  } else {
    msg("Connected to: %s\n",buf);
  }

  //    smbv_post_processing(fname);
  smbv_configure_from_file(sockfd,fname);

  // close connection
  msg("Closing connection...\n");
  freeaddrinfo(res);
  close(sockfd);

  return 0;
}

int smbv_configure_from_file(int sockfd, const char* fname)
{
  FILE *f_ptr;
  int ch;
  char inst[MAX_INST_LENGTH];
  char status[BUFFER_LENGTH];

  inst[0] = '\0';

  f_ptr = fopen(fname,"r");

  if (f_ptr!=NULL) {
    msg("Configuration file: %s\n",fname);
  } else {
    msg("ERROR: can't open SMBV config file: %s\n",fname);
    return -1;
  }

  while(!feof(f_ptr)) {
    ch = fgetc(f_ptr);
    //        putchar(ch);
    strcat(inst,(char*)&ch);

    if(ch == '\n') {
      //            strcat(inst,"*WAI\n");
      //            strcat(inst,"*OPC?\n");
      msg("sending command: %s",inst);

      //            strcat(inst,"\n");
      if (send(sockfd, inst, strlen(inst), 0) == -1) {
        msg("ERROR sending command!\n");
      }

      // This code checks the SMBV if the instruction has been carried out
      //            recv(sockfd, (void*)status, BUFFER_LENGTH, 0);
      //            msg("*OPC? = \"%s\"\n",status);
      // just compare first char since this must be one if the command has been applied
      //            usleep(100000);
      /*
      while(strncmp(status,"1",1)!=0) {
          msg("waiting...\n");
          usleep(10000);
          recv(sockfd, (void*) status, BUFFER_LENGTH, 0);
          //                msg("*OPC? = %s, cmp %d\n",status,strncmp(status,"1",1));
      }
      */
      inst[0] = '\0';
    }
  }

  msg("Done sending commands\n");

  fclose(f_ptr);
  return 0;
}

#ifdef CONFIG_SMBV
void help (void)
{
  printf ("Usage: smbv -h -W ip_address config_file_name\n");
  printf ("Compile with: gcc smbv.c -DCONFIG_SMBV -o smbv\n");
  printf ("Example: ./smbv -W192.168.12.23 test_config.smbv\n");
  printf ("Default IP: 192.168.12.201\n");
  printf ("Default config file name: smbv_config_file.smbv\n");
}

int main(int argc,char **argv)
{

  char c,smbv_ip[16],smbv_fname[256];
  extern char *optarg;
  int i;

  // set default values
  strcpy(smbv_ip,DEFAULT_SMBV_IP);
  strcpy(smbv_fname,DEFAULT_SMBV_FNAME);

  while ((c = getopt (argc, argv, "hW:")) != -1) {
    switch (c) {
    case 'W':
      strcpy(smbv_ip,optarg);
      break;

    case 'h':
      help();
      return 0;
    }
  }

  if (argc>2) {
    strcpy(smbv_fname,argv[2]);
  } else if (argc==2 && argv[1][0]!='-') {
    strcpy(smbv_fname,argv[1]);
  }

  printf("IP = %s\n",smbv_ip);
  printf("file = %s\n",smbv_fname);

  smbv_send_config (smbv_fname,smbv_ip);

  return 0;
}

#endif
